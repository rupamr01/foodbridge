import hashlib
import hmac
import os
import re
import secrets
import smtplib
import ssl
import uuid
from contextlib import contextmanager
from datetime import date, datetime, timedelta, timezone
from decimal import Decimal, InvalidOperation
from email.message import EmailMessage
from email.utils import formataddr
from functools import wraps
from pathlib import Path

import click
import mysql.connector
import requests
from dotenv import load_dotenv
from flask import Flask, jsonify, redirect, render_template, request, session, url_for
from mysql.connector import Error as MySQLError
from mysql.connector.errors import IntegrityError
from werkzeug.security import check_password_hash, generate_password_hash


BASE_DIR = Path(__file__).resolve().parent
load_dotenv(BASE_DIR / ".env")

EMAIL_PATTERN = re.compile(r"^[^@\s]+@[^@\s]+\.[^@\s]+$")
FOOD_CATEGORIES = {
    "Fresh Produce",
    "Cooked Meals",
    "Bakery",
    "Dairy",
    "Packaged Food",
    "Other",
}
ACCOUNT_ROLES = ("hotel", "ngo")
MAX_QUANTITY = Decimal("999999")
MAX_PHOTO_BYTES = 5 * 1024 * 1024
OTP_TTL_MINUTES = 10
OTP_MAX_ATTEMPTS = 5
OTP_RESEND_SECONDS = 60
OTP_MAX_SENDS_PER_HOUR = 5
OTP_CODE_PATTERN = re.compile(r"^\d{6}$")
OTP_DEBUG = os.environ.get("FOODBRIDGE_OTP_DEBUG") == "1"
PHOTO_UPLOAD_DIR = BASE_DIR / "static" / "uploads" / "food-photos"
SUPABASE_URL = os.environ.get("SUPABASE_URL", "").rstrip("/")
# Support Supabase's new publishable/secret keys and the legacy anon/service_role keys.
SUPABASE_ANON_KEY = (
    os.environ.get("SUPABASE_PUBLISHABLE_KEY", "").strip()
    or os.environ.get("SUPABASE_ANON_KEY", "").strip()
)
SUPABASE_SERVICE_ROLE_KEY = (
    os.environ.get("SUPABASE_SECRET_KEY", "").strip()
    or os.environ.get("SUPABASE_SERVICE_ROLE_KEY", "").strip()
)
SUPABASE_STORAGE_BUCKET = (
    os.environ.get("SUPABASE_STORAGE_BUCKET", "food-photos").strip() or "food-photos"
)

app = Flask(__name__)
app.config.update(
    SECRET_KEY=os.environ.get("FOODBRIDGE_SECRET_KEY"),
    MAX_CONTENT_LENGTH=8 * 1024 * 1024,
    SESSION_COOKIE_HTTPONLY=True,
    SESSION_COOKIE_SAMESITE="Lax",
    SESSION_COOKIE_SECURE=os.environ.get("FOODBRIDGE_COOKIE_SECURE") == "1",
)


@app.context_processor
def public_frontend_config():
    return {
        "supabase_url": SUPABASE_URL,
        "supabase_anon_key": SUPABASE_ANON_KEY,
        "supabase_enabled": bool(SUPABASE_URL and SUPABASE_ANON_KEY),
    }


def database_connection():
    options = {
        "host": os.environ.get("FOODBRIDGE_DB_HOST", "localhost"),
        "port": int(os.environ.get("FOODBRIDGE_DB_PORT", "3306")),
        "user": os.environ.get("FOODBRIDGE_DB_USER", "root"),
        "password": os.environ.get("FOODBRIDGE_DB_PASSWORD", ""),
        "database": os.environ.get("FOODBRIDGE_DB_NAME", "foodbridge"),
        "charset": "utf8mb4",
        "autocommit": False,
        "use_pure": True,
        "connection_timeout": 10,
    }
    # Cloud MySQL providers such as TiDB Cloud require TLS. Pointing this at
    # the provider's CA bundle (or the Linux system bundle) also verifies the
    # server certificate instead of only encrypting the connection.
    ssl_ca = os.environ.get("FOODBRIDGE_DB_SSL_CA", "").strip()
    if ssl_ca:
        options.update(
            ssl_ca=ssl_ca,
            ssl_verify_cert=True,
            ssl_verify_identity=True,
        )
    return mysql.connector.connect(**options)


@contextmanager
def db_connection():
    connection = database_connection()
    try:
        yield connection
    except Exception:
        connection.rollback()
        raise
    finally:
        connection.close()


def api_error(message, status):
    return jsonify({"error": message}), status


def body_data():
    data = request.get_json(silent=True)
    return data if isinstance(data, dict) else None


def require_user(*roles):
    def decorator(handler):
        @wraps(handler)
        def wrapped(*args, **kwargs):
            user_id = session.get("user_id")
            user_role = session.get("role")
            if not user_id:
                return api_error("Please sign in with an authorized account.", 401)
            if roles and user_role not in roles:
                return api_error("This account is not allowed to access this resource.", 403)
            return handler(*args, **kwargs)

        return wrapped

    return decorator


def account_payload(account):
    return {
        "id": account["id"],
        "name": account["name"],
        "email": account["email"],
        "role": account["role"],
    }


def json_safe(value):
    if isinstance(value, Decimal):
        return str(value)
    if isinstance(value, (date, datetime)):
        return value.isoformat()
    if isinstance(value, dict):
        return {key: json_safe(item) for key, item in value.items()}
    if isinstance(value, (list, tuple)):
        return [json_safe(item) for item in value]
    return value


def parse_quantity(value):
    """Return a positive decimal with at most two decimal places, or None."""
    try:
        quantity = Decimal(str(value).strip()).normalize()
    except (InvalidOperation, ValueError):
        return None
    if (
        not quantity.is_finite()
        or quantity < Decimal("0.01")
        or quantity > MAX_QUANTITY
        or quantity.as_tuple().exponent < -2
    ):
        return None
    return quantity


def parse_coordinate(value, limit):
    """Return (decimal, True) for a valid coordinate, or (None, False)."""
    text = str(value).strip() if value is not None else ""
    if not text:
        return None, True
    try:
        coordinate = Decimal(text)
    except (InvalidOperation, ValueError):
        return None, False
    if not coordinate.is_finite() or abs(coordinate) > limit:
        return None, False
    return coordinate, True


def detect_image_type(data):
    """Return (mime type, extension) for a supported image, otherwise None."""
    if data.startswith(b"\xff\xd8\xff"):
        return "image/jpeg", "jpg"
    if data.startswith(b"\x89PNG\r\n\x1a\n"):
        return "image/png", "png"
    if len(data) >= 12 and data[:4] == b"RIFF" and data[8:12] == b"WEBP":
        return "image/webp", "webp"
    return None


def save_food_photo(data, mime_type, extension):
    """Store a food photo locally or in the configured Supabase bucket."""
    filename = f"{uuid.uuid4().hex}.{extension}"
    object_path = f"food-photos/{filename}"
    if SUPABASE_URL and SUPABASE_SERVICE_ROLE_KEY:
        headers = {
            "apikey": SUPABASE_SERVICE_ROLE_KEY,
            "Content-Type": mime_type,
            "x-upsert": "false",
        }
        # New secret keys (sb_secret_...) must use the apikey header only.
        # Legacy service_role JWTs also need an Authorization header.
        if not SUPABASE_SERVICE_ROLE_KEY.startswith("sb_secret_"):
            headers["Authorization"] = f"Bearer {SUPABASE_SERVICE_ROLE_KEY}"
        response = requests.post(
            f"{SUPABASE_URL}/storage/v1/object/"
            f"{SUPABASE_STORAGE_BUCKET}/{object_path}",
            headers=headers,
            data=data,
            timeout=30,
        )
        if response.status_code not in (200, 201):
            app.logger.error(
                "Supabase photo upload failed with status %s", response.status_code
            )
            raise RuntimeError("Supabase photo upload failed")
        return (
            f"{SUPABASE_URL}/storage/v1/object/public/"
            f"{SUPABASE_STORAGE_BUCKET}/{object_path}"
        )

    PHOTO_UPLOAD_DIR.mkdir(parents=True, exist_ok=True)
    (PHOTO_UPLOAD_DIR / filename).write_bytes(data)
    return f"/static/uploads/food-photos/{filename}"


def verify_supabase_token(access_token):
    """Return (Supabase user, error message) for an access token."""
    if not (SUPABASE_URL and SUPABASE_ANON_KEY):
        return None, "Google sign-in is not configured."
    try:
        response = requests.get(
            f"{SUPABASE_URL}/auth/v1/user",
            headers={
                "apikey": SUPABASE_ANON_KEY,
                "Authorization": f"Bearer {access_token}",
            },
            timeout=10,
        )
    except requests.RequestException:
        return None, "Could not reach the Google sign-in service. Please try again."
    if response.status_code != 200:
        return None, "Your Google session is invalid or has expired. Please sign in again."
    try:
        payload = response.json()
    except ValueError:
        return None, "The Google sign-in service returned an invalid response."
    if not isinstance(payload, dict):
        return None, "The Google sign-in service returned an invalid response."
    return payload, None


class EmailDeliveryError(Exception):
    """Raised when the verification email cannot be sent."""


class OtpCooldownError(Exception):
    """Raised when a new code is requested too soon."""

    def __init__(self, retry_after):
        super().__init__("Please wait before requesting another code.")
        self.retry_after = max(1, int(retry_after))


class OtpRateLimitError(OtpCooldownError):
    """Raised when too many codes were requested in one hour."""


def utc_now():
    return datetime.now(timezone.utc).replace(tzinfo=None)


def generate_otp():
    return f"{secrets.randbelow(1_000_000):06d}"


def otp_digest(account_id, otp):
    """Bind the code to one account and never store the plain code."""
    secret = (app.config.get("SECRET_KEY") or "").encode("utf-8")
    value = f"{account_id}:{otp}".encode("utf-8")
    return hmac.new(secret, value, hashlib.sha256).hexdigest()


def smtp_settings():
    security = os.environ.get("FOODBRIDGE_SMTP_SECURITY", "starttls").strip().lower()
    default_port = "465" if security == "ssl" else "587"
    return {
        "host": os.environ.get("FOODBRIDGE_SMTP_HOST", "").strip(),
        "port": os.environ.get("FOODBRIDGE_SMTP_PORT", default_port).strip(),
        "username": os.environ.get("FOODBRIDGE_SMTP_USERNAME", "").strip(),
        "password": os.environ.get("FOODBRIDGE_SMTP_PASSWORD", ""),
        "from_email": os.environ.get("FOODBRIDGE_SMTP_FROM_EMAIL", "").strip(),
        "from_name": os.environ.get("FOODBRIDGE_SMTP_FROM_NAME", "FoodBridge").strip()
        or "FoodBridge",
        "security": security,
    }


def email_delivery_configured():
    settings = smtp_settings()
    return bool(settings["host"] and settings["from_email"])


def send_email(to_email, subject, text_body, html_body=None):
    """Send one message through the configured SMTP relay."""
    settings = smtp_settings()
    if not settings["host"] or not settings["from_email"]:
        raise EmailDeliveryError("Email delivery is not configured.")
    if settings["security"] not in ("starttls", "ssl", "none"):
        raise EmailDeliveryError("Email security must be starttls, ssl, or none.")
    try:
        port = int(settings["port"])
    except ValueError as error:
        raise EmailDeliveryError("Email port must be a number.") from error

    message = EmailMessage()
    message["Subject"] = subject
    message["From"] = formataddr((settings["from_name"], settings["from_email"]))
    message["To"] = to_email
    message.set_content(text_body)
    if html_body:
        message.add_alternative(html_body, subtype="html")

    try:
        if settings["security"] == "ssl":
            client = smtplib.SMTP_SSL(
                settings["host"],
                port,
                timeout=20,
                context=ssl.create_default_context(),
            )
        else:
            client = smtplib.SMTP(settings["host"], port, timeout=20)
        with client:
            client.ehlo()
            if settings["security"] == "starttls":
                client.starttls(context=ssl.create_default_context())
                client.ehlo()
            if settings["username"]:
                client.login(settings["username"], settings["password"])
            client.send_message(message)
    except (OSError, smtplib.SMTPException) as error:
        raise EmailDeliveryError("The email could not be delivered.") from error


def send_otp_email(to_email, otp):
    subject = "Your FoodBridge verification code"
    text_body = (
        f"Your FoodBridge verification code is: {otp}\n\n"
        f"It expires in {OTP_TTL_MINUTES} minutes. If you did not create a "
        "FoodBridge account, you can ignore this email.\n\n"
        "FoodBridge - Less waste. More smiles."
    )
    html_body = (
        '<div style="font-family:Arial,sans-serif;max-width:480px;margin:auto;'
        'padding:24px;color:#163e30">'
        '<h2 style="margin:0 0 12px">Verify your FoodBridge email</h2>'
        "<p>Enter this 6-digit code to finish creating your account:</p>"
        '<p style="font-size:34px;font-weight:700;letter-spacing:8px;'
        f'margin:20px 0;color:#087f5b">{otp}</p>'
        f"<p>The code expires in {OTP_TTL_MINUTES} minutes. If you did not "
        "create a FoodBridge account, you can ignore this email.</p>"
        '<p style="color:#6b7c72;font-size:12px">FoodBridge - Less waste. '
        "More smiles.</p></div>"
    )
    send_email(to_email, subject, text_body, html_body)


def issue_otp(account_id, email):
    """Create, store, and email a fresh code. Raises if sending fails."""
    now = utc_now()
    otp = generate_otp()
    with db_connection() as connection:
        with connection.cursor(dictionary=True) as cursor:
            cursor.execute(
                "SELECT otp_last_sent_at, otp_send_window_start, otp_send_count "
                "FROM accounts WHERE id = %s FOR UPDATE",
                (account_id,),
            )
            account = cursor.fetchone()
            if not account:
                raise EmailDeliveryError("Account not found.")

            last_sent = account["otp_last_sent_at"]
            if last_sent:
                elapsed = (now - last_sent).total_seconds()
                if elapsed < OTP_RESEND_SECONDS:
                    raise OtpCooldownError(OTP_RESEND_SECONDS - elapsed)

            window_start = account["otp_send_window_start"]
            send_count = int(account["otp_send_count"] or 0)
            if not window_start or (now - window_start).total_seconds() >= 3600:
                window_start = now
                send_count = 0
            if send_count >= OTP_MAX_SENDS_PER_HOUR:
                raise OtpRateLimitError(
                    3600 - (now - window_start).total_seconds()
                )

            cursor.execute(
                "UPDATE accounts SET otp_hash = %s, otp_expires_at = %s, "
                "otp_attempts = 0, otp_send_window_start = %s, "
                "otp_send_count = %s WHERE id = %s",
                (
                    otp_digest(account_id, otp),
                    now + timedelta(minutes=OTP_TTL_MINUTES),
                    window_start,
                    send_count + 1,
                    account_id,
                ),
            )
        connection.commit()

    try:
        send_otp_email(email, otp)
    except EmailDeliveryError:
        app.logger.exception("Could not send the FoodBridge verification email")
        with db_connection() as connection:
            with connection.cursor() as cursor:
                cursor.execute(
                    "UPDATE accounts SET otp_last_sent_at = NULL WHERE id = %s",
                    (account_id,),
                )
            connection.commit()
        if OTP_DEBUG:
            app.logger.warning(
                "FOODBRIDGE_OTP_DEBUG is on. Code for %s: %s", email, otp
            )
            return False
        raise

    with db_connection() as connection:
        with connection.cursor() as cursor:
            cursor.execute(
                "UPDATE accounts SET otp_last_sent_at = %s WHERE id = %s",
                (utc_now(), account_id),
            )
        connection.commit()
    return True


@app.before_request
def require_secret_key():
    if not app.config["SECRET_KEY"]:
        return api_error("Set FOODBRIDGE_SECRET_KEY before starting the application.", 500)
    return None


@app.errorhandler(MySQLError)
def database_error(error):
    app.logger.exception("FoodBridge database operation failed", exc_info=error)
    return api_error("The database service is unavailable. Please try again shortly.", 503)


@app.errorhandler(413)
def upload_too_large(error):
    return api_error("The upload is too large. Choose a photo smaller than 5 MB.", 413)


@app.get("/")
def index():
    return render_template("landing.html")


@app.get("/app")
def workspace():
    if not session.get("user_id"):
        return redirect(url_for("login_page"))
    return render_template("index.html")


@app.get("/login")
def login_page():
    if session.get("user_id"):
        return redirect(url_for("workspace"))
    return render_template("login.html")


@app.get("/signup")
def signup_page():
    if session.get("user_id"):
        return redirect(url_for("workspace"))
    return render_template("signup.html")


@app.get("/auth/callback")
def auth_callback():
    return render_template("auth_callback.html")


@app.get("/admin")
def admin_page():
    if session.get("role") == "admin":
        return redirect(url_for("workspace"))
    return render_template("admin.html")


@app.post("/api/auth/google")
def google_login():
    data = body_data()
    if data is None:
        return api_error("Send Google sign-in details as JSON.", 400)

    access_token = str(data.get("access_token", "")).strip()
    if not access_token:
        return api_error("Google sign-in token is missing.", 400)

    user, error = verify_supabase_token(access_token)
    if error:
        return api_error(error, 503 if "not configured" in error else 401)

    email = str(user.get("email", "")).strip().lower()
    supabase_uid = str(user.get("id", "")).strip()
    provider = (user.get("app_metadata") or {}).get("provider")
    identities = user.get("identities") or []
    has_google_identity = any(
        str(identity.get("provider", "")) == "google"
        for identity in identities
        if isinstance(identity, dict)
    )
    if not email or not supabase_uid:
        return api_error("The Google account did not provide an email address.", 400)
    if provider != "google" and not has_google_identity:
        return api_error("Use the Continue with Google button to sign in.", 403)

    role = data.get("role")
    name = str(data.get("name", "")).strip()
    created = False
    try:
        with db_connection() as connection:
            with connection.cursor(dictionary=True) as cursor:
                cursor.execute(
                    "SELECT id, name, email, role, supabase_uid, email_verified "
                    "FROM accounts WHERE supabase_uid = %s LIMIT 1",
                    (supabase_uid,),
                )
                account = cursor.fetchone()
                if not account:
                    cursor.execute(
                        "SELECT id, name, email, role, supabase_uid, email_verified "
                        "FROM accounts WHERE email = %s LIMIT 1",
                        (email,),
                    )
                    account = cursor.fetchone()

                if account:
                    if account["role"] == "admin":
                        return api_error(
                            "Administrator accounts must use the administrator sign-in page.",
                            403,
                        )
                    if account["supabase_uid"] and account["supabase_uid"] != supabase_uid:
                        return api_error(
                            "This email is linked to a different Google account.",
                            409,
                        )
                    if not account["supabase_uid"] or not account["email_verified"]:
                        # Google has already verified this address, so link the
                        # account and clear any pending code.
                        cursor.execute(
                            "UPDATE accounts SET supabase_uid = COALESCE(supabase_uid, %s), "
                            "email_verified = 1, otp_hash = NULL, otp_expires_at = NULL, "
                            "otp_attempts = 0 WHERE id = %s",
                            (supabase_uid, account["id"]),
                        )
                        account["supabase_uid"] = (
                            account["supabase_uid"] or supabase_uid
                        )
                        account["email_verified"] = 1
                else:
                    if role not in ACCOUNT_ROLES:
                        return jsonify({
                            "error": "Choose Hotel or NGO account type to finish creating your account.",
                            "code": "registration_required",
                        }), 409
                    if len(name) < 2:
                        metadata = user.get("user_metadata") or {}
                        name = str(metadata.get("full_name", "")).strip()
                    if len(name) < 2:
                        name = email.split("@")[0]
                    if len(name) < 2 or len(name) > 160:
                        return api_error(
                            "Enter an organization name between 2 and 160 characters.",
                            400,
                        )
                    cursor.execute(
                        "INSERT INTO accounts "
                        "(supabase_uid, name, email, password_hash, role, email_verified) "
                        "VALUES (%s, %s, %s, %s, %s, 1)",
                        (
                            supabase_uid,
                            name,
                            email,
                            generate_password_hash(secrets.token_urlsafe(32)),
                            role,
                        ),
                    )
                    account = {
                        "id": cursor.lastrowid,
                        "name": name,
                        "email": email,
                        "role": role,
                        "supabase_uid": supabase_uid,
                        "email_verified": 1,
                    }
                    created = True
            connection.commit()
    except IntegrityError:
        return api_error("An account with that email already exists. Please sign in.", 409)

    session.clear()
    session["user_id"] = account["id"]
    session["role"] = account["role"]
    return jsonify({"account": account_payload(account)}), 201 if created else 200


@app.post("/api/register")
def register():
    data = body_data()
    if data is None:
        return api_error("Send registration details as JSON.", 400)

    name = str(data.get("name", "")).strip()
    email = str(data.get("email", "")).strip().lower()
    password = data.get("password", "")
    role = data.get("role")

    if len(name) < 2 or len(name) > 160:
        return api_error("Enter an organization name between 2 and 160 characters.", 400)
    if len(email) > 254 or not EMAIL_PATTERN.fullmatch(email):
        return api_error("Enter a valid email address.", 400)
    if not isinstance(password, str) or len(password) < 10 or len(password) > 256:
        return api_error("Password must contain at least 10 characters.", 400)
    if role not in ACCOUNT_ROLES:
        return api_error("Choose Hotel or NGO account type.", 400)

    try:
        with db_connection() as connection:
            with connection.cursor(dictionary=True) as cursor:
                cursor.execute(
                    "INSERT INTO accounts "
                    "(name, email, password_hash, role, email_verified) "
                    "VALUES (%s, %s, %s, %s, 0)",
                    (name, email, generate_password_hash(password), role),
                )
                account_id = cursor.lastrowid
            connection.commit()
    except IntegrityError:
        return api_error("An account with that email already exists.", 409)

    otp_sent = True
    retry_after = None
    message = "Enter the 6-digit code we emailed you."
    try:
        delivered = issue_otp(account_id, email)
        if not delivered:
            otp_sent = False
            message = (
                "Email delivery is not configured. The verification code was "
                "printed in the server console."
            )
    except OtpCooldownError as error:
        otp_sent = False
        retry_after = error.retry_after
        message = "Please wait a moment before requesting a new code."
    except EmailDeliveryError:
        otp_sent = False
        retry_after = OTP_RESEND_SECONDS
        message = (
            "Your account was created, but the verification email could not "
            "be sent. Use Resend code to try again."
        )

    # The account exists but is not signed in until the email is verified.
    session.clear()
    session["pending_user_id"] = account_id
    return jsonify({
        "verification_required": True,
        "otp_sent": otp_sent,
        "retry_after": retry_after,
        "message": message,
        "email": email,
        "account": {
            "id": account_id,
            "name": name,
            "email": email,
            "role": role,
        },
    }), 201


def authenticate(email, password, role=None):
    if role is None:
        query = (
            "SELECT id, name, email, password_hash, role, email_verified "
            "FROM accounts WHERE email = %s AND role IN ('hotel', 'ngo')"
        )
        params = (email,)
    else:
        query = (
            "SELECT id, name, email, password_hash, role, email_verified "
            "FROM accounts WHERE email = %s AND role = %s"
        )
        params = (email, role)

    with db_connection() as connection:
        with connection.cursor(dictionary=True) as cursor:
            cursor.execute(query, params)
            account = cursor.fetchone()

    if not account or not check_password_hash(account["password_hash"], password):
        return None
    return account


@app.post("/api/login")
def login():
    data = body_data()
    if data is None:
        return api_error("Send sign-in details as JSON.", 400)

    email = str(data.get("email", "")).strip().lower()
    password = data.get("password", "")
    if not email or not isinstance(password, str):
        return api_error("Enter your email and password.", 400)

    account = authenticate(email, password)
    if not account:
        return api_error("Email or password is incorrect.", 401)

    if not account["email_verified"]:
        session.clear()
        session["pending_user_id"] = account["id"]
        return jsonify({
            "error": "Verify your email address to continue.",
            "code": "email_unverified",
            "email": account["email"],
        }), 403

    session.clear()
    session["user_id"] = account["id"]
    session["role"] = account["role"]
    return jsonify({"account": account_payload(account)})


@app.post("/api/verify-email")
def verify_email():
    data = body_data()
    if data is None:
        return api_error("Send verification details as JSON.", 400)

    email = str(data.get("email", "")).strip().lower()
    code = str(data.get("code", "")).strip()
    if len(email) > 254 or not EMAIL_PATTERN.fullmatch(email):
        return api_error("Enter a valid email address.", 400)
    if not OTP_CODE_PATTERN.fullmatch(code):
        return api_error("Enter the 6-digit code from your email.", 400)

    with db_connection() as connection:
        with connection.cursor(dictionary=True) as cursor:
            cursor.execute(
                "SELECT id, name, email, role, email_verified, otp_hash, "
                "otp_expires_at, otp_attempts FROM accounts "
                "WHERE email = %s AND role IN ('hotel', 'ngo') FOR UPDATE",
                (email,),
            )
            account = cursor.fetchone()
            invalid = api_error(
                "The code is incorrect or has expired. Request a new code.", 400
            )
            if not account:
                return invalid
            if account["email_verified"]:
                return jsonify({
                    "verified": True,
                    "logged_in": session.get("user_id") == account["id"],
                    "message": "This email is already verified. Please sign in.",
                })
            if account["otp_attempts"] >= OTP_MAX_ATTEMPTS:
                return api_error(
                    "Too many incorrect attempts. Request a new code.", 429
                )
            if (
                not account["otp_hash"]
                or not account["otp_expires_at"]
                or account["otp_expires_at"] < utc_now()
            ):
                return invalid
            if not hmac.compare_digest(
                account["otp_hash"], otp_digest(account["id"], code)
            ):
                cursor.execute(
                    "UPDATE accounts SET otp_attempts = otp_attempts + 1 "
                    "WHERE id = %s",
                    (account["id"],),
                )
                connection.commit()
                return invalid
            cursor.execute(
                "UPDATE accounts SET email_verified = 1, otp_hash = NULL, "
                "otp_expires_at = NULL, otp_attempts = 0 WHERE id = %s",
                (account["id"],),
            )
        connection.commit()

    logged_in = False
    if session.get("pending_user_id") == account["id"]:
        session.clear()
        session["user_id"] = account["id"]
        session["role"] = account["role"]
        logged_in = True
    return jsonify({
        "verified": True,
        "logged_in": logged_in,
        "account": account_payload(account),
    })


@app.post("/api/resend-otp")
def resend_otp():
    data = body_data()
    if data is None:
        return api_error("Send the account email as JSON.", 400)

    email = str(data.get("email", "")).strip().lower()
    if len(email) > 254 or not EMAIL_PATTERN.fullmatch(email):
        return api_error("Enter a valid email address.", 400)

    with db_connection() as connection:
        with connection.cursor(dictionary=True) as cursor:
            cursor.execute(
                "SELECT id, email_verified FROM accounts "
                "WHERE email = %s AND role IN ('hotel', 'ngo') LIMIT 1",
                (email,),
            )
            account = cursor.fetchone()

    if not account or account["email_verified"]:
        # Do not reveal whether an address is registered.
        return jsonify({
            "ok": True,
            "otp_sent": False,
            "message": "If this email needs verification, a new code was sent.",
        })

    try:
        delivered = issue_otp(account["id"], email)
    except OtpCooldownError as error:
        return jsonify({
            "error": f"Please wait {error.retry_after} seconds before requesting another code.",
            "retry_after": error.retry_after,
        }), 429
    except EmailDeliveryError:
        return api_error(
            "The verification email could not be sent. Please try again shortly.",
            502,
        )

    return jsonify({
        "ok": True,
        "otp_sent": delivered,
        "retry_after": OTP_RESEND_SECONDS,
    })


@app.post("/api/admin/login")
def admin_login():
    data = body_data()
    if data is None:
        return api_error("Send sign-in details as JSON.", 400)

    email = str(data.get("email", "")).strip().lower()
    password = data.get("password", "")
    if not email or not isinstance(password, str):
        return api_error("Enter your email and password.", 400)

    account = authenticate(email, password, role="admin")
    if not account:
        return api_error("Admin email or password is incorrect.", 401)

    session.clear()
    session["user_id"] = account["id"]
    session["role"] = "admin"
    return jsonify({"account": account_payload(account)})


@app.get("/api/me")
@require_user()
def current_account():
    with db_connection() as connection:
        with connection.cursor(dictionary=True) as cursor:
            cursor.execute(
                "SELECT id, name, email, role FROM accounts WHERE id = %s",
                (session["user_id"],),
            )
            account = cursor.fetchone()
    if not account:
        session.clear()
        return api_error("Account no longer exists. Please sign in again.", 401)
    return jsonify({"account": account_payload(account)})


@app.post("/api/logout")
def logout():
    session.clear()
    return jsonify({"ok": True})


@app.get("/api/foods")
@require_user("hotel", "ngo", "admin")
def list_foods():
    role = session["role"]
    select = "SELECT f.*, a.name AS donor_name FROM foods f JOIN accounts a ON a.id = f.hotel_id "
    with db_connection() as connection:
        with connection.cursor(dictionary=True) as cursor:
            if role == "hotel":
                cursor.execute(
                    select + "WHERE f.hotel_id = %s ORDER BY f.created_at DESC",
                    (session["user_id"],),
                )
            elif role == "admin":
                cursor.execute(select + "ORDER BY f.created_at DESC")
            else:
                cursor.execute(
                    select + "WHERE f.status = 'available' AND f.available_quantity > 0 "
                    "AND f.best_before >= CURRENT_DATE() "
                    "ORDER BY f.best_before, f.created_at DESC"
                )
            foods = cursor.fetchall()
    return jsonify({"foods": json_safe(foods)})


@app.post("/api/foods")
@require_user("hotel")
def create_food():
    data = body_data()
    if data is None:
        return api_error("Send food details as JSON.", 400)

    name = str(data.get("name", "")).strip()
    category = str(data.get("category", "")).strip()
    description = str(data.get("description", "")).strip()
    unit = str(data.get("unit", "")).strip()
    pickup_address = str(data.get("pickup_address", "")).strip()
    photo_url = str(data.get("photo_url", "")).strip()
    quantity = parse_quantity(data.get("quantity", ""))
    latitude, latitude_valid = parse_coordinate(data.get("latitude"), Decimal("90"))
    longitude, longitude_valid = parse_coordinate(data.get("longitude"), Decimal("180"))
    try:
        best_before = date.fromisoformat(str(data.get("best_before", "")))
    except ValueError:
        best_before = None

    if len(name) < 2 or len(name) > 180:
        return api_error("Food name must be between 2 and 180 characters.", 400)
    if category not in FOOD_CATEGORIES:
        return api_error("Choose a valid food category.", 400)
    if quantity is None:
        return api_error("Quantity must be greater than zero.", 400)
    if not unit or len(unit) > 32:
        return api_error("Enter a quantity unit, such as meals or kg.", 400)
    if len(pickup_address) < 4 or len(pickup_address) > 500:
        return api_error("Enter a pickup address between 4 and 500 characters.", 400)
    if len(description) > 2000:
        return api_error("Description must be 2,000 characters or fewer.", 400)
    if best_before is None or best_before < date.today():
        return api_error("Enter a valid best-before date that is not in the past.", 400)
    if not latitude_valid or not longitude_valid:
        return api_error("Choose a valid pickup location on the map.", 400)
    if (latitude is None) != (longitude is None):
        return api_error("Choose both latitude and longitude on the map.", 400)
    if photo_url:
        allowed_photo_prefixes = ("/static/uploads/food-photos/",)
        if SUPABASE_URL:
            allowed_photo_prefixes += (
                f"{SUPABASE_URL}/storage/v1/object/public/"
                f"{SUPABASE_STORAGE_BUCKET}/",
            )
        if len(photo_url) > 500 or not photo_url.startswith(allowed_photo_prefixes):
            return api_error("Upload the food photo through FoodBridge.", 400)

    with db_connection() as connection:
        with connection.cursor(dictionary=True) as cursor:
            cursor.execute(
                "INSERT INTO foods "
                "(hotel_id, name, category, description, total_quantity, "
                "available_quantity, unit, pickup_address, photo_url, "
                "latitude, longitude, best_before) "
                "VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)",
                (
                    session["user_id"],
                    name,
                    category,
                    description,
                    quantity,
                    quantity,
                    unit,
                    pickup_address,
                    photo_url or None,
                    latitude,
                    longitude,
                    best_before,
                ),
            )
            food_id = cursor.lastrowid
        connection.commit()
    return jsonify({"id": food_id}), 201


@app.post("/api/uploads/food-photo")
@require_user("hotel")
def upload_food_photo():
    photo = request.files.get("photo")
    if not photo or not photo.filename:
        return api_error("Choose a photo to upload.", 400)
    data = photo.read(MAX_PHOTO_BYTES + 1)
    if not data:
        return api_error("The selected photo is empty.", 400)
    if len(data) > MAX_PHOTO_BYTES:
        return api_error("Choose a photo smaller than 5 MB.", 413)
    detected = detect_image_type(data)
    if not detected:
        return api_error("Upload a JPEG, PNG, or WebP image.", 400)
    mime_type, extension = detected
    try:
        photo_url = save_food_photo(data, mime_type, extension)
    except requests.RequestException:
        app.logger.exception("Supabase photo upload request failed")
        return api_error("Photo storage is temporarily unavailable. Please try again.", 502)
    except RuntimeError:
        return api_error("Photo storage is temporarily unavailable. Please try again.", 502)
    except OSError:
        app.logger.exception("Local photo upload failed")
        return api_error("The photo could not be saved. Please try again.", 500)
    return jsonify({"url": photo_url}), 201


@app.get("/api/orders")
@require_user("hotel", "ngo", "admin")
def list_orders():
    role = session["role"]
    if role == "hotel":
        predicate, params = "o.hotel_id = %s", (session["user_id"],)
    elif role == "ngo":
        predicate, params = "o.ngo_id = %s", (session["user_id"],)
    else:
        predicate, params = "1 = 1", ()

    with db_connection() as connection:
        with connection.cursor(dictionary=True) as cursor:
            cursor.execute(
                "SELECT o.*, f.name AS food_name, f.unit, "
                "h.name AS hotel_name, n.name AS ngo_name "
                "FROM orders o JOIN foods f ON f.id = o.food_id "
                "JOIN accounts h ON h.id = o.hotel_id "
                "JOIN accounts n ON n.id = o.ngo_id "
                f"WHERE {predicate} ORDER BY o.created_at DESC",
                params,
            )
            orders = cursor.fetchall()
    return jsonify({"orders": json_safe(orders)})


@app.post("/api/foods/<int:food_id>/orders")
@require_user("ngo")
def place_order(food_id):
    data = body_data()
    if data is None:
        return api_error("Send order details as JSON.", 400)

    quantity = parse_quantity(data.get("quantity", ""))
    note = str(data.get("note", "")).strip()
    if quantity is None:
        return api_error("Order quantity must be greater than zero.", 400)
    if len(note) > 1000:
        return api_error("Order note must be 1,000 characters or fewer.", 400)

    with db_connection() as connection:
        with connection.cursor(dictionary=True) as cursor:
            cursor.execute(
                "SELECT id, hotel_id, available_quantity, status, best_before FROM foods "
                "WHERE id = %s FOR UPDATE",
                (food_id,),
            )
            food = cursor.fetchone()
            if not food or food["status"] != "available" or food["best_before"] < date.today():
                return api_error("This food listing is no longer available.", 404)
            cursor.execute(
                "SELECT id FROM orders WHERE food_id = %s AND ngo_id = %s "
                "AND status IN ('pending', 'accepted') LIMIT 1",
                (food_id, session["user_id"]),
            )
            if cursor.fetchone():
                return api_error("You already have an active request for this listing.", 409)
            cursor.execute(
                "SELECT COALESCE(SUM(quantity), 0) AS pending_quantity FROM orders "
                "WHERE food_id = %s AND status = 'pending'",
                (food_id,),
            )
            pending = cursor.fetchone()["pending_quantity"]
            if quantity + pending > food["available_quantity"]:
                return api_error("Requested quantity exceeds the unreserved amount.", 409)
            cursor.execute(
                "INSERT INTO orders (food_id, hotel_id, ngo_id, quantity, note) "
                "VALUES (%s, %s, %s, %s, %s)",
                (food_id, food["hotel_id"], session["user_id"], quantity, note),
            )
            order_id = cursor.lastrowid
        connection.commit()
    return jsonify({"id": order_id, "status": "pending"}), 201


@app.patch("/api/orders/<int:order_id>")
@require_user("hotel")
def update_order(order_id):
    data = body_data()
    if data is None or data.get("status") not in ("accepted", "rejected"):
        return api_error("Choose accepted or rejected.", 400)
    new_status = data["status"]

    with db_connection() as connection:
        with connection.cursor(dictionary=True) as cursor:
            cursor.execute(
                "SELECT o.id, o.food_id, o.quantity, o.status, f.available_quantity "
                "FROM orders o JOIN foods f ON f.id = o.food_id "
                "WHERE o.id = %s AND o.hotel_id = %s FOR UPDATE",
                (order_id, session["user_id"]),
            )
            order = cursor.fetchone()
            if not order:
                return api_error("Order request not found.", 404)
            if order["status"] != "pending":
                return api_error("This order request has already been processed.", 409)
            if new_status == "accepted":
                if order["quantity"] > order["available_quantity"]:
                    return api_error("Not enough food remains to accept this request.", 409)
                remaining = order["available_quantity"] - order["quantity"]
                cursor.execute(
                    "UPDATE foods SET available_quantity = %s, "
                    "status = IF(%s = 0, 'fulfilled', status) WHERE id = %s",
                    (remaining, remaining, order["food_id"]),
                )
            cursor.execute("UPDATE orders SET status = %s WHERE id = %s", (new_status, order_id))
        connection.commit()
    return jsonify({"id": order_id, "status": new_status})


@app.get("/api/admin/summary")
@require_user("admin")
def admin_summary():
    with db_connection() as connection:
        with connection.cursor(dictionary=True) as cursor:
            cursor.execute(
                "SELECT id, name, email, role, created_at FROM accounts "
                "ORDER BY created_at DESC"
            )
            accounts = cursor.fetchall()
            cursor.execute(
                "SELECT f.*, a.name AS donor_name FROM foods f "
                "JOIN accounts a ON a.id = f.hotel_id ORDER BY f.created_at DESC"
            )
            foods = cursor.fetchall()
            cursor.execute(
                "SELECT o.*, f.name AS food_name, f.unit, "
                "h.name AS hotel_name, n.name AS ngo_name "
                "FROM orders o JOIN foods f ON f.id = o.food_id "
                "JOIN accounts h ON h.id = o.hotel_id "
                "JOIN accounts n ON n.id = o.ngo_id ORDER BY o.created_at DESC"
            )
            orders = cursor.fetchall()
    return jsonify(json_safe({
        "accounts": accounts,
        "foods": foods,
        "orders": orders,
        "counts": {
            "accounts": len(accounts),
            "hotels": sum(account["role"] == "hotel" for account in accounts),
            "ngos": sum(account["role"] == "ngo" for account in accounts),
            "foods": len(foods),
            "open_foods": sum(food["status"] == "available" for food in foods),
            "orders": len(orders),
            "pending_orders": sum(order["status"] == "pending" for order in orders),
        },
    }))


@app.cli.command("create-admin")
@click.argument("email")
def create_admin(email):
    """Create an administrator account; there is no public admin registration."""
    email = email.strip().lower()
    if not EMAIL_PATTERN.fullmatch(email):
        raise click.ClickException("Enter a valid email address.")
    name = click.prompt("Administrator name").strip()
    if len(name) < 2 or len(name) > 160:
        raise click.ClickException("Name must contain 2 to 160 characters.")
    password = click.prompt("Administrator password", hide_input=True, confirmation_prompt=True)
    if len(password) < 10 or len(password) > 256:
        raise click.ClickException("Password must contain 10 to 256 characters.")
    try:
        with db_connection() as connection:
            with connection.cursor(dictionary=True) as cursor:
                cursor.execute(
                    "INSERT INTO accounts "
                    "(name, email, password_hash, role, email_verified) "
                    "VALUES (%s, %s, %s, 'admin', 1)",
                    (name, email, generate_password_hash(password)),
                )
            connection.commit()
    except IntegrityError as error:
        raise click.ClickException("An account with that email already exists.") from error
    click.echo("Administrator account created. Sign in at /admin.")


@app.cli.command("check-db")
def check_database():
    """Check that FoodBridge can connect to its configured MySQL database."""
    with db_connection() as connection:
        with connection.cursor(dictionary=True) as cursor:
            cursor.execute("SELECT 1")
            cursor.fetchone()
    click.echo("FoodBridge database connection is ready.")


@app.cli.command("send-test-email")
@click.argument("recipient")
def send_test_email(recipient):
    """Send a test message using the configured SMTP settings."""
    recipient = recipient.strip().lower()
    if not EMAIL_PATTERN.fullmatch(recipient):
        raise click.ClickException("Enter a valid email address.")
    if not email_delivery_configured():
        raise click.ClickException(
            "Set FOODBRIDGE_SMTP_HOST and FOODBRIDGE_SMTP_FROM_EMAIL first."
        )
    try:
        send_email(
            recipient,
            "FoodBridge email test",
            "This is a test email from FoodBridge. Your SMTP settings work.",
            "<p>This is a test email from <strong>FoodBridge</strong>. "
            "Your SMTP settings work.</p>",
        )
    except EmailDeliveryError as error:
        raise click.ClickException(str(error)) from error
    click.echo(f"Test email sent to {recipient}.")


if __name__ == "__main__":
    app.run(
        host=os.environ.get("FOODBRIDGE_HOST", "127.0.0.1"),
        port=int(os.environ.get("FOODBRIDGE_PORT", "5000")),
        debug=False,
    )
