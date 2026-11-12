# FoodBridge — Complete Project, Local Setup & Deployment Guide

Last updated: 11 November 2026

This is the handover document for the current FoodBridge project. It explains what was built, how the pieces fit together, how to run everything locally, and what must change before showing the app to other people on a temporary or permanent domain.

Do not put real passwords, API keys, or client secrets in this file. They belong only in `.env` or your hosting provider's environment settings.

---

## 1. What FoodBridge is

FoodBridge connects hotels that have surplus food with NGOs that can collect it.

There are three account roles:

| Role | What they can do |
|---|---|
| `hotel` | Publish food listings with photos and an exact pickup point, review NGO requests, accept or decline them |
| `ngo` | Browse available food, see pickup locations and photos, request quantities, track request status |
| `admin` | Monitor accounts, listings, and orders; no public admin registration |

The app is a single Flask website. It serves the public landing page, email/password sign-in, Google sign-in, and the role-specific workspace.

---

## 2. What runs where

This project intentionally uses **MySQL as the main database** and **Supabase only for Google sign-in and photo storage**.

| Part | Technology | Responsibility |
|---|---|---|
| Web server | Python + Flask | Routes, sessions, validation, business logic, serving HTML/CSS/JS |
| Main database | MySQL 8 + `mysql-connector-python` | Accounts, food listings, orders — the source of truth |
| Google login | Supabase Auth free tier | Verifies the Google identity, issues a Supabase access token |
| Photo storage | Supabase Storage free tier | Stores listing photos; public bucket for image URLs |
| Local photo fallback | Flask static folder | Used automatically when Supabase Storage is not configured |
| Maps | Leaflet + OpenStreetMap tiles | Interactive worldwide pickup-point maps |
| Place search | Nominatim (OpenStreetMap) | Free city/landmark search |
| Browser libraries | Vendored in `static/vendor/` | No paid CDN or map API |

### Data flow

```text
Browser
  ├─ email/password ──────────────► Flask ─────────────► MySQL
  ├─ Continue with Google ────────► Supabase Auth ─────► Google
  │                                      │
  │                                      └─ redirect to /auth/callback
  │                                                    │
  │                                      Flask verifies the Supabase token
  │                                                    │
  │                                                    ▼
  │                                                   MySQL
  ├─ photo upload ────────────────► Flask ─────────────► Supabase Storage
  │                                      └─ or local static/uploads/
  └─ map tiles / place search ────► OpenStreetMap
```

### Why both Supabase and MySQL?

MySQL already stores the app's accounts, listings, and orders. Replacing it would mean rewriting the whole data layer. Supabase is used only where it saves the most work on the free tier:

- Google OAuth without building a custom OAuth server
- Object storage for photos without renting a separate file host

Supabase's own Postgres database is **not used**. The `supabase_uid` column in MySQL links an existing FoodBridge account to its Google identity.

---

## 3. Feature status

### Working and verified

- Email/password registration and sign-in
- Google sign-in through Supabase Auth
- Existing-account linking by email
- New Google accounts complete role and organization name on the callback page
- Food listings stored in MySQL
- NGO requests and hotel accept/decline flow
- Admin dashboard
- Camera or file photo upload with client-side resizing
- Server-side image signature checks and size limits
- Supabase Storage uploads, with local folder fallback
- Leaflet map picker, Nominatim search, browser geolocation, draggable pin
- Mini maps and OpenStreetMap links on listings
- Worldwide latitude/longitude validation
- Database migration for existing installations
- 20 unit tests (2 MySQL workflow tests skipped without a test DB)
- Full MySQL 8.0.46 integration run passed
- Supabase bucket creation, public photo read, and photo deletion tested
- Supabase OAuth authorize endpoint redirects to Google for both localhost callback URLs
- Publishable key exposed to the browser; secret key never rendered

### Still needs a browser test

- A real Google account completing the full sign-in and account-creation flow
- Creating a test hotel listing with a photo and map pin through the UI
- Creating a test NGO request and accepting it as the hotel
- Running the app from a temporary public domain

---

## 4. Project structure

```text
.
├── app.py                     # Flask app, APIs, auth, uploads, maps, CLI commands
├── PROJECT_GUIDE.md           # This handover document
├── README.md                  # Shorter public setup guide
├── requirements.txt           # Python dependencies
├── start.sh                   # Linux/macOS local launcher
├── .env                       # Private local configuration (never commit)
├── .env.example               # Shareable configuration template
├── .gitignore                 # Excludes .env, .venv, uploads, caches
├── database/
│   ├── schema.sql             # Full MySQL schema for new installs
│   └── migrations/
│       └── 001_google_auth_photos_maps.sql
├── static/
│   ├── assets/images/logo.svg
│   ├── css/style.css          # All site styles
│   ├── js/
│   │   ├── auth.js            # Login/signup and Google OAuth launch
│   │   ├── callback.js        # OAuth callback, token check, new-account form
│   │   └── portal.js          # Workspace, listings, photos, maps
│   ├── uploads/               # Local photo fallback (created on first upload)
│   └── vendor/
│       ├── leaflet/           # Leaflet 1.9.4
│       └── supabase/          # Supabase JS 2.x
├── templates/
│   ├── index.html             # Workspace shell
│   ├── landing.html           # Public landing page
│   ├── login.html
│   ├── signup.html
│   ├── auth_callback.html     # Google callback / completion screen
│   └── admin.html
└── tests/
    ├── test_app.py            # Unit tests
    └── test_mysql_workflow.py # Optional full MySQL workflow tests
```

---

## 5. Local setup from zero

The examples below target Windows with MySQL 8.x. Linux/macOS equivalents are noted where useful.

### 5.1 Install prerequisites

- Python 3.10 or newer
- MySQL Server 8.x running as the `MySQL80` service
- MySQL Workbench (optional, but easier for creating users and importing SQL)
- Git (only needed for GitHub)
- A modern browser

Check Python:

```powershell
python --version
```

### 5.2 Create the MySQL database and app user

Open MySQL Workbench, connect as `root`, and run:

```sql
CREATE DATABASE IF NOT EXISTS foodbridge
  CHARACTER SET utf8mb4
  COLLATE utf8mb4_unicode_ci;

CREATE USER 'foodbridge_app'@'localhost'
  IDENTIFIED BY 'use-a-strong-local-password';

GRANT ALL PRIVILEGES ON foodbridge.* TO 'foodbridge_app'@'localhost';

FLUSH PRIVILEGES;
```

If the app connects through TCP and MySQL resolves clients as `127.0.0.1`, also create the same user for that host:

```sql
CREATE USER 'foodbridge_app'@'127.0.0.1'
  IDENTIFIED BY 'use-a-strong-local-password';

GRANT ALL PRIVILEGES ON foodbridge.* TO 'foodbridge_app'@'127.0.0.1';
FLUSH PRIVILEGES;
```

`GRANT ALL` is limited to the `foodbridge` database, which is enough for local development and schema imports. For a public deployment, use a strong password and consider reducing the grants to `SELECT, INSERT, UPDATE, DELETE`.

### 5.3 Create the Python environment

From the project root:

```powershell
python -m venv .venv
.\.venv\Scripts\python -m pip install -r requirements.txt
```

Linux/macOS:

```sh
python3 -m venv .venv
.venv/bin/python -m pip install -r requirements.txt
```

### 5.4 Create `.env`

```powershell
Copy-Item .env.example .env
```

Generate a Flask secret key:

```powershell
.\.venv\Scripts\python -c "import secrets; print(secrets.token_urlsafe(48))"
```

Paste the output into `.env`:

```dotenv
FOODBRIDGE_SECRET_KEY=the-generated-value

FOODBRIDGE_DB_HOST=localhost
FOODBRIDGE_DB_PORT=3306
FOODBRIDGE_DB_NAME=foodbridge
FOODBRIDGE_DB_USER=foodbridge_app
FOODBRIDGE_DB_PASSWORD=use-a-strong-local-password
FOODBRIDGE_COOKIE_SECURE=0

SUPABASE_URL=https://your-project-ref.supabase.co
SUPABASE_PUBLISHABLE_KEY=sb_publishable_...
SUPABASE_SECRET_KEY=sb_secret_...
SUPABASE_STORAGE_BUCKET=food-photos

FOODBRIDGE_SMTP_HOST=smtp-relay.brevo.com
FOODBRIDGE_SMTP_PORT=587
FOODBRIDGE_SMTP_USERNAME=your-brevo-login-email
FOODBRIDGE_SMTP_PASSWORD=your-brevo-smtp-key
FOODBRIDGE_SMTP_FROM_EMAIL=your-verified-sender@example.com
FOODBRIDGE_SMTP_FROM_NAME=FoodBridge
FOODBRIDGE_SMTP_SECURITY=starttls
```

What each value means:

| Variable | Meaning |
|---|---|
| `FOODBRIDGE_SECRET_KEY` | Signs Flask session cookies. Keep private. Changing it signs everyone out. |
| `FOODBRIDGE_DB_*` | MySQL connection. These are read on every request. |
| `FOODBRIDGE_COOKIE_SECURE` | `0` for local HTTP, `1` for HTTPS deployments. |
| `SUPABASE_URL` | `https://<project-ref>.supabase.co` — not the dashboard URL. |
| `SUPABASE_PUBLISHABLE_KEY` | Public browser key (`sb_publishable_...`). Safe to expose. |
| `SUPABASE_SECRET_KEY` | Private server key (`sb_secret_...`). Never expose. |
| `SUPABASE_STORAGE_BUCKET` | Bucket name, default `food-photos`. |
| `FOODBRIDGE_SMTP_*` | SMTP relay used to send 6-digit signup verification codes. `FOODBRIDGE_SMTP_FROM_EMAIL` must be a sender verified with the provider. |
| `FOODBRIDGE_OTP_DEBUG` | Local development only. Set to `1` to print the OTP in the server console when email delivery fails. Never enable in production. |

#### New keys vs legacy keys

Supabase is retiring the old `anon` and `service_role` keys. The app supports both:

```dotenv
# New style (preferred)
SUPABASE_PUBLISHABLE_KEY=sb_publishable_...
SUPABASE_SECRET_KEY=sb_secret_...

# Legacy style (still works)
SUPABASE_ANON_KEY=eyJ...
SUPABASE_SERVICE_ROLE_KEY=eyJ...
```

If both are present, the new names win. For `sb_secret_...` keys the server sends only the `apikey` header; for legacy JWT keys it also sends `Authorization: Bearer ...`, because that is what each key type requires.

### 5.5 Create the Supabase project

1. Sign in at <https://supabase.com/dashboard>.
2. Create a project on the free plan.
3. The database password Supabase asks for is for Supabase's own Postgres. FoodBridge does not use it. Save it separately if you like.

### 5.6 Create the Google OAuth credentials

1. Open <https://console.cloud.google.com/>.
2. Create or select a project.
3. Open the Google Auth Platform:
   - <https://console.cloud.google.com/auth/overview>
4. Click **Get started** if shown:
   - App name: `FoodBridge`
   - User support email: your Gmail
   - Audience: **External**
   - Contact email: your Gmail
   - Agree and create
5. If the app is in **Testing**, add your Gmail under **Audience → Test users**.
6. Under **Data Access**, ensure these scopes exist:
   - `openid`
   - `.../auth/userinfo.email`
   - `.../auth/userinfo.profile`
7. Open **Clients** → **Create client**:
   - Application type: **Web application**
   - Name: `FoodBridge Web`
   - **Authorized JavaScript origins:**
     - `http://localhost:5000`
     - `http://127.0.0.1:5000`
   - **Authorized redirect URI:**
     - `https://<your-project-ref>.supabase.co/auth/v1/callback`
8. Create the client and copy the **Client ID** and **Client Secret** immediately. Google shows the full secret only once; if it is lost, open the client and use **Add secret** to create a new one.

**Important:** The Google redirect URI is Supabase's callback, not the Flask callback.

```text
Google Cloud redirect URI  → https://<ref>.supabase.co/auth/v1/callback
Supabase redirect URL      → http://localhost:5000/auth/callback
```

### 5.7 Enable Google in Supabase

1. Open <https://supabase.com/dashboard/project/_/auth/providers?provider=Google>.
2. Enable the Google provider.
3. Paste the Google Client ID and Client Secret.
4. Save.
5. Open **Authentication → URL Configuration**:
   - **Site URL:** `http://localhost:5000`
   - **Redirect URLs:**
     - `http://localhost:5000/auth/callback`
     - `http://127.0.0.1:5000/auth/callback`
6. If testing from a phone on the same Wi-Fi, also add the LAN URL, for example:
   - `http://192.168.1.20:5000/auth/callback`

Keep user sign-ups enabled so first-time Google users can create a FoodBridge account.

### 5.8 Create the photo bucket

1. Open **Storage → Buckets → New bucket**.
2. Name: `food-photos`
3. Enable **Public bucket**.
4. Create the bucket.

The server uses the private secret key for uploads, so no storage write policy is needed. Public read access is required so `<img>` tags can load the photos.

You can also verify the bucket from the command line:

```powershell
.\.venv\Scripts\python -c "import app, requests; r = requests.get(f'{app.SUPABASE_URL}/storage/v1/bucket/{app.SUPABASE_STORAGE_BUCKET}', headers={'apikey': app.SUPABASE_SERVICE_ROLE_KEY}); print(r.status_code, r.text)"
```

Expected: `200` and a JSON object containing `"public":true`.

### 5.9 Import the database schema

New installation:

```powershell
& "C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe" -u foodbridge_app -p foodbridge -e "SOURCE C:/path/to/Foodbridge-main/database/schema.sql;"
```

Or in Workbench:

1. Open `database/schema.sql`.
2. Select the `foodbridge` schema.
3. Execute the script.

Existing installation from before Google/photos/maps:

```powershell
& "C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe" -u foodbridge_app -p foodbridge -e "SOURCE C:/path/to/Foodbridge-main/database/migrations/001_google_auth_photos_maps.sql;"
```

Verify:

```sql
USE foodbridge;
SHOW TABLES;
-- Expected: accounts, foods, orders
```

### 5.10 Create the administrator account

There is deliberately no public admin signup.

```powershell
.\.venv\Scripts\flask --app app create-admin admin@example.org
```

The command prompts for the administrator name and password. The password must be 10–256 characters. Sign in at `/admin`.

### 5.11 Start the app

Check the database connection first:

```powershell
.\.venv\Scripts\flask --app app check-db
```

Expected:

```text
FoodBridge database connection is ready.
```

Start the server:

```powershell
.\.venv\Scripts\python app.py
```

Open:

- <http://localhost:5000>
- <http://localhost:5000/login>
- <http://localhost:5000/signup>
- <http://localhost:5000/admin>

On Windows, if port 5000 is busy:

```powershell
$env:FOODBRIDGE_PORT = "5001"
.\.venv\Scripts\python app.py
```

If you change the port, update the Supabase Site URL and Redirect URLs to match.

---

## 6. How the main flows work

### 6.1 Email/password authentication

- `POST /api/register` validates name, email, password, and role, hashes the password with Werkzeug, and inserts the account as **unverified**.
- It generates a random 6-digit code, stores only an HMAC-SHA256 digest, and emails the code through the configured SMTP relay.
- `POST /api/verify-email` checks the code, the 10-minute expiry, and the 5-attempt limit, then marks `email_verified = 1`.
- `POST /api/resend-otp` issues a new code after a 60-second cooldown. It allows at most 5 sends per account per hour.
- `POST /api/login` verifies the password hash. If the email is unverified it returns `403` with `code: "email_unverified"` and the browser shows the same OTP step.
- Google sign-in marks the email verified automatically, because Google already proved ownership.
- `POST /api/admin/login` only allows the `admin` role. Admin accounts are created verified by the CLI.
- Every protected endpoint uses `@require_user(...)` to check the session and role server-side.
- The browser never receives password hashes or stored OTP values.

### 6.2 Google sign-in

1. The browser creates a Supabase client with the public URL and publishable/anon key.
2. `signInWithOAuth({ provider: "google" })` redirects to Supabase, then to Google.
3. Google redirects back to Supabase's callback:
   `https://<ref>.supabase.co/auth/v1/callback`
4. Supabase redirects the browser to:
   `http://localhost:5000/auth/callback`
5. The callback page uses the implicit flow, reads the Supabase session, and sends the access token to:
   `POST /api/auth/google`
6. Flask calls:
   `GET https://<ref>.supabase.co/auth/v1/user`
   with the publishable key in `apikey` and the user's token in `Authorization: Bearer ...`.
7. Flask confirms the identity is Google-based by checking:
   - `app_metadata.provider == "google"`, or
   - an identity in `identities[]` with provider `google`
8. Account handling:
   - Existing account by email: link `supabase_uid` if missing, sign in, return `200`.
   - Existing account by `supabase_uid`: sign in, return `200`.
   - New account without role/name: return `409` with `code: "registration_required"`.
   - New account with role/name: create it in MySQL and return `201`.
9. Flask writes its own session cookie. Supabase tokens are not stored in MySQL.

The browser library is vendored at `static/vendor/supabase/supabase.js`. The app uses the implicit flow for compatibility with plain-HTTP LAN testing.

### 6.3 Photo uploads

1. The hotel chooses **Take a photo** or **Choose a photo**.
2. The browser validates the file type and resizes large images to a maximum of 1600 px, JPEG quality 0.86.
3. The file is sent to `POST /api/uploads/food-photo`.
4. Flask checks:
   - Only hotels can upload
   - Maximum file size is 5 MB (request cap 8 MB)
   - File signature must be JPEG, PNG, or WebP
5. Storage:
   - If `SUPABASE_URL` and a server key are configured: upload to Supabase Storage and return a public URL.
   - Otherwise: save to `static/uploads/food-photos/` and return a local URL.
6. The listing form sends the returned URL as `photo_url`. Flask only accepts URLs under the local upload path or the configured Supabase public bucket path.
7. Photos display on food cards with `loading="lazy"`.

### 6.4 Maps and exact pickup points

- Leaflet is vendored locally.
- Tiles come from OpenStreetMap: `https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png`.
- Place search uses Nominatim:
  `https://nominatim.openstreetmap.org/search`
- The hotel can:
  - Search for a city, area, or landmark
  - Use browser geolocation
  - Tap the map to drop a pin
  - Drag the pin to fine-tune it
- Coordinates are optional, but both latitude and longitude must be supplied together.
- Latitude is limited to ±90 and longitude to ±180.
- Coordinates are stored as `DECIMAL(10,7)` in MySQL and serialized as strings for JavaScript.
- NGO listings show a small Leaflet map and a link to the exact OpenStreetMap point.

Browser geolocation normally requires HTTPS or `localhost`. On a plain-HTTP LAN address, the button may fail; tapping the map still works.

### 6.5 Role-based workspace

- `portal.js` builds the sidebar and pages for the signed-in role.
- Every API call includes the Flask session cookie.
- Every server endpoint checks the session and role; the UI alone is not trusted.
- Open workspace pages refresh every 30 seconds. Interactive objects (maps, photo previews, object URLs) are cleaned up before each re-render.

---

## 7. Database

### Tables

**`accounts`**

| Column | Notes |
|---|---|
| `id` | Primary key |
| `supabase_uid` | Unique Google/Supabase user ID; NULL for email-only accounts |
| `name` | Organization name |
| `email` | Unique |
| `password_hash` | Always present; Google-only accounts get an unusable random hash |
| `email_verified` | `1` after the 6-digit code is confirmed; pre-existing and Google accounts are `1` |
| `otp_hash` | HMAC-SHA256 digest of the current code, never the plain code |
| `otp_expires_at` | Code expiry, 10 minutes after sending |
| `otp_attempts` | Incorrect guesses for the current code; maximum 5 |
| `otp_last_sent_at` | Enforces the 60-second resend cooldown |
| `otp_send_window_start`, `otp_send_count` | Limit each account to 5 code sends per hour |
| `role` | `hotel`, `ngo`, or `admin` |
| `created_at` | Timestamp |

**`foods`**

| Column | Notes |
|---|---|
| `hotel_id` | Foreign key to `accounts` |
| `name`, `category`, `description` | Listing details |
| `total_quantity`, `available_quantity` | `DECIMAL(10,2)` |
| `unit` | Meals, kg, boxes, etc. |
| `pickup_address` | Required text |
| `photo_url` | Optional local or Supabase public URL |
| `latitude`, `longitude` | Optional `DECIMAL(10,7)`, with range checks |
| `best_before` | Date, not in the past |
| `status` | `available`, `reserved`, `completed`, etc. |
| `created_at` | Timestamp |

**`orders`**

Stores the NGO request, hotel decision, quantity, status, and timestamps.

### JSON handling

MySQL returns `Decimal` and `date`/`datetime` values. Flask's `json_safe` converts:

- `Decimal` → string (preserves precision)
- `date` / `datetime` → ISO string

The JavaScript converts coordinate strings back to numbers for maps.

---

## 8. Testing

### Unit tests

```powershell
.\.venv\Scripts\python -m unittest discover -s tests -v
```

Expected on a machine without a test database:

- 20 tests run
- 18 pass
- 2 MySQL workflow tests skipped

Covered:

- Authentication pages and callback page
- Coordinate validation
- Image signatures
- Upload permissions and local fallback
- Supabase header behavior for new and legacy keys
- Google token/configuration checks
- Non-Google identity rejection
- Service-role key never rendered
- Protected endpoints
- Admin registration restrictions

### Full MySQL integration tests

Use a separate, disposable database. Never point this at real data.

```powershell
$env:FOODBRIDGE_TEST_DB_NAME = "foodbridge_test"
$env:FOODBRIDGE_TEST_DB_HOST = "127.0.0.1"
$env:FOODBRIDGE_TEST_DB_PORT = "3306"
$env:FOODBRIDGE_TEST_DB_USER = "foodbridge_app"
$env:FOODBRIDGE_TEST_DB_PASSWORD = "your-password"

.\.venv\Scripts\python -m unittest discover -s tests -v
```

The tests create accounts, link/create Google accounts, publish a listing with photo and coordinates, order it as an NGO, and accept it as the hotel.

### What tests do not cover

- A real Google browser login
- Real Supabase OAuth token exchange
- Production hosting behavior

Those require valid credentials and a browser session.

---

## 9. Local-only behavior and limits

This setup is designed to work on one machine first:

- MySQL runs on `localhost:3306`. Only the Flask process on this machine can reach it.
- Supabase provides cloud auth and photos, so those work from any browser that can reach the app.
- The Google OAuth redirect targets `http://localhost:5000/auth/callback`, so Google login only works while you open the app at that origin.
- Browser geolocation is allowed on `localhost`; a plain-HTTP LAN address may block it.
- If Supabase is not configured:
  - Google buttons stay hidden.
  - Photos are saved locally.
  - Email/password login and maps still work.
- The Flask development server is not a production server.
- OpenStreetMap tiles and Nominatim are free public services. Respect their usage policies; for high traffic, use a dedicated tile provider or self-host.

---

## 10. Going live

There are three realistic paths. Option A gives you a link in minutes. Option C is the recommended all-free permanent setup.

### Option A — Temporary demo through a tunnel (easiest)

Keep the app and MySQL running on your PC, then expose port 5000 through a free tunnel.

**Steps**

1. Start FoodBridge locally:

   ```powershell
   $env:FOODBRIDGE_COOKIE_SECURE = "1"
   .\.venv\Scripts\python app.py
   ```

   Set `FOODBRIDGE_COOKIE_SECURE=1` only if the tunnel URL uses HTTPS. If you also need to keep testing at `http://localhost:5000`, secure cookies will not be sent over plain HTTP; switch it back to `0` for local-only testing.

2. Start a tunnel to `http://localhost:5000`:

   Cloudflare Tunnel example:

   ```powershell
   cloudflared tunnel --url http://localhost:5000
   ```

   ngrok example:

   ```powershell
   ngrok http 5000
   ```

3. Copy the public HTTPS URL, for example:

   ```text
   https://random-name.trycloudflare.com
   ```

4. In Supabase → **Authentication → URL Configuration**:
   - **Site URL:** `https://random-name.trycloudflare.com`
   - Add redirect URL:
     `https://random-name.trycloudflare.com/auth/callback`

5. In Google Cloud → your OAuth client:
   - Add to **Authorized JavaScript origins:**
     `https://random-name.trycloudflare.com`
   - The **Authorized redirect URI does not change**. It stays:
     `https://<ref>.supabase.co/auth/v1/callback`

6. Open the tunnel URL and test Google sign-in.

**Advantages**

- Local MySQL keeps working; no cloud database needed.
- Supabase Storage and Google auth already work from the cloud.

**Limitations**

- Your PC, app, and tunnel must stay running.
- Free quick-tunnel URLs change when restarted; update Supabase and Google each time.
- Use a named Cloudflare tunnel or a paid ngrok domain for a stable URL.
- This is a demo, not production hosting.

### Option B — Deploy to a cloud host

GitHub stores code; it does not run Flask or MySQL. GitHub Pages cannot host this app because it is static-only.

A cloud deployment needs:

1. **A Python host** such as Render, Railway, Fly.io, PythonAnywhere, or a VPS.
2. **A cloud MySQL database** reachable from that host.
3. **Environment variables** configured in the host's dashboard, not a committed `.env`.
4. **A production WSGI server**, for example Gunicorn on Linux:

   ```sh
   pip install gunicorn
   gunicorn app:app
   ```

   On Windows, Waitress is an alternative:

   ```powershell
   pip install waitress
   waitress-serve --listen=0.0.0.0:5000 app:app
   ```

5. **HTTPS** and `FOODBRIDGE_COOKIE_SECURE=1`.
6. **Updated Supabase URLs** for the deployed domain.
7. **Updated Google origins** for the deployed domain.

Cloud MySQL steps:

1. Create a database and a dedicated user on the cloud MySQL provider.
2. Import `database/schema.sql`.
3. Use those credentials in the host's environment variables.
4. Do not expose MySQL port 3306 publicly; restrict access to the app host if possible.
5. Back up the database.

Deployment checklist:

- [ ] `FOODBRIDGE_SECRET_KEY` set to a new random value
- [ ] Database host, port, name, user, password set
- [ ] `SUPABASE_URL`, publishable key, secret key set
- [ ] `SUPABASE_STORAGE_BUCKET=food-photos`
- [ ] `FOODBRIDGE_COOKIE_SECURE=1`
- [ ] Schema imported
- [ ] Admin account created
- [ ] Supabase Site URL and redirect URLs updated
- [ ] Google authorized origin updated
- [ ] Google redirect URI still points to Supabase
- [ ] HTTPS working
- [ ] Google sign-in tested in an incognito window
- [ ] Photo upload tested
- [ ] Map picker tested

### Option C — All-free permanent deployment: Render + TiDB Cloud Starter

This is the recommended persistent free setup as of November 2026. It uses only free plans, needs no credit card, and gives you a public `https://<name>.onrender.com` address.

| Part | Free service | What it does |
|---|---|---|
| Code | GitHub Free | Stores the repository and triggers deploys |
| Web app | Render Free web service | Runs Flask with Gunicorn; 750 instance hours/month; sleeps after 15 minutes idle |
| Database | TiDB Cloud Starter | MySQL-compatible database; 5 GiB free storage; TLS required |
| Google login | Supabase Free | Already configured |
| Photos | Supabase Storage Free | Already configured |
| HTTPS URL | Render `onrender.com` subdomain | Free HTTPS, no custom domain needed |

**Important limits**

- The first request after 15 idle minutes can take 30–60 seconds while Render wakes the app.
- Render free instances have 512 MB RAM and 0.1 CPU.
- TiDB Cloud Starter has a monthly free quota. Check the current quota in the TiDB console.
- Render's outbound IP addresses are not fixed. TiDB's public endpoint firewall must allow all IPs (`0.0.0.0/0`) for this setup. Use a strong database password.
- Free plans have no production SLA. Keep backups and do not treat this as permanent infrastructure for real users.

#### C1. Install Git and create a GitHub repository

Git is not currently installed on this machine. Install it once:

```powershell
winget install --id Git.Git -e --source winget
```

Close and reopen PowerShell, then check:

```powershell
git --version
```

Create an empty repository on <https://github.com/new>:

- Name: `foodbridge`
- Visibility: Public or Private (both work with Render)
- Do **not** add a README, `.gitignore`, or license; the project already has them.

Then push the project from its folder:

```powershell
git init
git add .
git status
```

Before committing, confirm `.env` and `.venv/` are **not** in the list. The included `.gitignore` should exclude them.

```powershell
git commit -m "FoodBridge ready for deployment"
git branch -M main
git remote add origin https://github.com/<your-username>/foodbridge.git
git push -u origin main
```

#### C2. Create the free TiDB Cloud Starter database

1. Sign in at <https://tidbcloud.com/>.
2. Open **My TiDB** and click **Create Resource**.
3. Select **Starter**.
4. Compatibility mode: **MySQL Compatible**.
5. Name: `foodbridge`.
6. Choose the region closest to you.
7. Set the **spending limit to 0** so the instance stays free. Do not add a card.
8. Click **Create** and wait about 30 seconds.
9. Open the instance, click **Connect**, keep **Public**, and generate a password.
10. Save these values somewhere private:
    - Host (looks like `gateway01.<region>.prod.aws.tidbcloud.com`)
    - Port: `4000`
    - User: the full prefix plus `.root`, for example `3f9a2b7c.root`
    - Password: the generated 16-character password

11. Open **Settings → Networking** (or the firewall/IP access list link in the Connect dialog) and allow all IPs:
    - `0.0.0.0/0`

    This is needed because Render does not provide a fixed outbound IP on the free plan. The database is still protected by TLS and the password.

12. Create the database. Use the TiDB **SQL Editor** if available, or connect with the MySQL client:

    ```sql
    CREATE DATABASE IF NOT EXISTS foodbridge
      CHARACTER SET utf8mb4
      COLLATE utf8mb4_unicode_ci;
    ```

13. Import the schema with the local MySQL 8 client:

    ```powershell
    & "C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe" `
      -h <TIDB_HOST> -P 4000 -u "<PREFIX>.root" -p `
      --ssl-mode=REQUIRED foodbridge `
      -e "SOURCE C:/Users/rupam/Downloads/Foodbridge-main/database/schema.sql;"
    ```

    Enter the TiDB password when prompted. You can also use MySQL Workbench with **SSL mode: Required**.

    Verify:

    ```sql
    USE foodbridge;
    SHOW TABLES;
    -- Expected: accounts, foods, orders
    ```

#### C3. Create the Render web service

1. Sign in at <https://render.com/> with GitHub. No credit card is required.
2. Click **New → Web Service**.
3. Connect the `foodbridge` GitHub repository.
4. Fill in:
   - **Name:** a unique name, for example `foodbridge-demo-rupam`
   - **Region:** closest to you
   - **Branch:** `main`
   - **Runtime:** Python 3
   - **Build Command:** `pip install -r requirements.txt`
   - **Start Command:** `gunicorn app:app --bind 0.0.0.0:$PORT --workers 1 --threads 4 --timeout 120`
   - **Instance Type:** Free
5. Add these environment variables:

   | Key | Value |
   |---|---|
   | `FOODBRIDGE_SECRET_KEY` | A new random value; generate it with the command below |
   | `FOODBRIDGE_DB_HOST` | TiDB host |
   | `FOODBRIDGE_DB_PORT` | `4000` |
   | `FOODBRIDGE_DB_NAME` | `foodbridge` |
   | `FOODBRIDGE_DB_USER` | `<PREFIX>.root` (no quotes) |
   | `FOODBRIDGE_DB_PASSWORD` | TiDB password |
   | `FOODBRIDGE_DB_SSL_CA` | `/etc/ssl/certs/ca-certificates.crt` |
   | `FOODBRIDGE_COOKIE_SECURE` | `1` |
   | `SUPABASE_URL` | `https://yugxwsqoxlgkhsohthyf.supabase.co` |
   | `SUPABASE_PUBLISHABLE_KEY` | Your current publishable key |
   | `SUPABASE_SECRET_KEY` | Your current secret key |
   | `SUPABASE_STORAGE_BUCKET` | `food-photos` |
   | `FOODBRIDGE_SMTP_HOST` | `smtp-relay.brevo.com` |
   | `FOODBRIDGE_SMTP_PORT` | `587` |
   | `FOODBRIDGE_SMTP_USERNAME` | Your Brevo login email |
   | `FOODBRIDGE_SMTP_PASSWORD` | Your Brevo SMTP key |
   | `FOODBRIDGE_SMTP_FROM_EMAIL` | The sender address verified in Brevo |
   | `FOODBRIDGE_SMTP_FROM_NAME` | `FoodBridge` |
   | `FOODBRIDGE_SMTP_SECURITY` | `starttls` |

   Generate the Flask secret locally:

   ```powershell
   .\.venv\Scripts\python -c "import secrets; print(secrets.token_urlsafe(48))"
   ```

   The repository also contains a `Procfile`, so Render can detect the start command automatically. Setting it explicitly in the dashboard is still recommended.

   `FOODBRIDGE_DB_SSL_CA` makes the app verify TiDB's TLS certificate. If the first deploy fails with a certificate error, remove that variable and redeploy. The connection will still be encrypted, but the certificate will not be verified.

6. Click **Create Web Service**. Wait for the build and deploy to finish.
7. Copy the public URL, for example:

   ```text
   https://foodbridge-demo-rupam.onrender.com
   ```

   If the name is already taken, Render asks you to choose another one.

#### C4. Run the email OTP migration

Existing databases need the new account columns before the updated app is deployed. In MySQL Workbench connected to TiDB, open and run:

```text
database/migrations/002_email_otp.sql
```

Or run the SQL directly:

```sql
ALTER TABLE accounts
  ADD COLUMN email_verified TINYINT(1) NOT NULL DEFAULT 0 AFTER password_hash,
  ADD COLUMN otp_hash CHAR(64) NULL AFTER email_verified,
  ADD COLUMN otp_expires_at DATETIME NULL AFTER otp_hash,
  ADD COLUMN otp_attempts SMALLINT UNSIGNED NOT NULL DEFAULT 0 AFTER otp_expires_at,
  ADD COLUMN otp_last_sent_at DATETIME NULL AFTER otp_attempts,
  ADD COLUMN otp_send_window_start DATETIME NULL AFTER otp_last_sent_at,
  ADD COLUMN otp_send_count SMALLINT UNSIGNED NOT NULL DEFAULT 0 AFTER otp_send_window_start;

UPDATE accounts SET email_verified = 1;
```

The `UPDATE` marks all accounts that existed before this feature as verified. Google accounts are marked verified automatically by the app.

#### C5. Create the admin account in the cloud database

Run this locally against the TiDB database. The environment variables below override the values in `.env` for this terminal only.

```powershell
$env:FOODBRIDGE_DB_HOST = "<TIDB_HOST>"
$env:FOODBRIDGE_DB_PORT = "4000"
$env:FOODBRIDGE_DB_NAME = "foodbridge"
$env:FOODBRIDGE_DB_USER = "<PREFIX>.root"
$env:FOODBRIDGE_DB_PASSWORD = "<TIDB_PASSWORD>"
$env:FOODBRIDGE_DB_SSL_CA = "C:\path\to\isrgrootx1.pem"

.\.venv\Scripts\flask --app app create-admin admin@example.org
```

If you do not have the CA file on Windows, download it from <https://letsencrypt.org/certs/isrgrootx1.pem> and save it as `C:\certs\isrgrootx1.pem`. On Linux/Render, use the system bundle path shown in C3 instead.

Close that PowerShell window afterward so the cloud credentials are not left in your environment.

#### C6. Point Supabase and Google at the Render URL

In Supabase → **Authentication → URL Configuration**:

- **Site URL:** `https://foodbridge-demo-rupam.onrender.com`
- Add redirect URL:
  `https://foodbridge-demo-rupam.onrender.com/auth/callback`
- Keep the localhost redirect URLs so local development still works.

In Google Cloud → your OAuth client:

- Add to **Authorized JavaScript origins:**
  `https://foodbridge-demo-rupam.onrender.com`
- The **Authorized redirect URI** does not change:
  `https://<ref>.supabase.co/auth/v1/callback`

#### C7. Test the live site

1. Open the Render URL.
2. Wait for the first wake-up if the service was idle.
3. Open `/login` in an incognito window.
4. Sign in with Google.
5. Complete the new-account form if prompted.
6. Sign up with a real email address you can open, enter the 6-digit code, and confirm the code expires after 10 minutes and Resend enforces a 60-second cooldown.
7. Create a hotel listing with a photo and a map pin.
8. Create or use an NGO account and request the listing.
9. Accept the request as the hotel.
10. Sign in at `/admin` with the admin account.
11. Confirm that photos load from Supabase and that the map tiles load.

#### C8. Updating the live site

- Push changes to GitHub. Render redeploys automatically.
- Database changes must be applied separately to TiDB.
- To back up TiDB, export the `foodbridge` database from the TiDB console or with `mysqldump`.
- If the free service sleeps, the first request after idle is slow. That is expected.
- If you outgrow the free quota, upgrade or move to a paid host. Do not add a card unless you are ready to pay.

#### Optional: keep using your local MySQL

You do not have to migrate data for a demo. Option A (tunnel) uses local MySQL. Option C uses TiDB. The two databases are independent unless you export and import data between them.

### Custom domain

For a real domain such as `foodbridge.example.com`:

1. Buy or use an existing domain.
2. Point DNS to your host:
   - `A` record for an IP, or
   - `CNAME` for a hostname
3. Enable HTTPS on the host (most platforms do this automatically).
4. In Supabase → URL Configuration:
   - Site URL: `https://foodbridge.example.com`
   - Redirect URL: `https://foodbridge.example.com/auth/callback`
5. In Google Cloud → OAuth client:
   - Authorized JavaScript origin: `https://foodbridge.example.com`
   - Redirect URI remains `https://<ref>.supabase.co/auth/v1/callback`
6. Set `FOODBRIDGE_COOKIE_SECURE=1` and restart the app.
7. Test in incognito mode.

### Google OAuth testing mode

If the Google app is in **Testing**:

- Only Gmail accounts listed under **Audience → Test users** can sign in.
- Other users see an access-blocked message.
- Add each tester's Gmail, or publish the app.

Publishing may require Google verification depending on the scopes and branding. Basic `openid`, email, and profile scopes are usually enough for this app, but check the current Google policy.

### GitHub safety checklist

Before pushing:

```powershell
git init
git add .
git status
```

You should **not** see:

- `.env`
- `.venv/`
- `__pycache__/`
- `static/uploads/` contents

The included `.gitignore` excludes them. Verify:

```powershell
Get-Content .gitignore
```

If a secret is ever committed:

1. Rotate the Supabase secret key.
2. Rotate the Flask secret key.
3. Change the MySQL password.
4. Remove the secret from Git history before publishing.

GitHub is not a hosting provider for this app. Use a tunnel or a Python host.

---

## 11. Troubleshooting

| Problem | Likely cause | Fix |
|---|---|---|
| `Access denied for user 'x'@'localhost'` | Wrong user/password in `.env`, or the user was created for a different host | Check `FOODBRIDGE_DB_*`; create the user for `localhost` and/or `127.0.0.1`; restart |
| `Can't connect to MySQL server` | MySQL80 service stopped or wrong port | Start the service; check port 3306; run `check-db` |
| `FOODBRIDGE_SECRET_KEY` error | Missing key in `.env` | Generate and add one, then restart |
| Google button is missing | Supabase URL or publishable key missing | Fill both in `.env`, restart, refresh |
| Google login says “not configured” | Same as above | Check the app logs and `/login` page source for the meta tags |
| `redirect_uri_mismatch` | Google redirect URI is wrong or has a trailing slash | Use exactly `https://<ref>.supabase.co/auth/v1/callback` |
| `origin_mismatch` | App origin is not in Google's JavaScript origins | Add `http://localhost:5000` and `http://127.0.0.1:5000` |
| “Google hasn't verified this app” | App is in Testing | Advanced → continue, and add the Gmail as a test user |
| “Access blocked” | Google account is not a test user | Add it under Audience → Test users |
| Callback shows “No Google session was found” | Redirect URL mismatch or cookies blocked | Check Supabase redirect URLs; try incognito; allow cookies |
| New Google user gets `registration_required` | Expected first-time behavior | The callback form asks for organization name and role |
| Photo upload returns 502 | Supabase bucket/key/storage issue | Verify bucket exists and is public; check server logs; test the bucket endpoint |
| Photo upload returns 413 | File too large | Keep photos under 5 MB; the browser should resize automatically |
| Photo upload returns 401 | Session expired | Sign in again |
| 413 on the whole request | Request exceeds 8 MB | Use a smaller photo |
| Map is blank | No internet or OSM tiles blocked | Check network/ad-blocker; tiles require internet |
| “Use my location” fails | Browser geolocation blocked or page is HTTP on a LAN | Use HTTPS or localhost; tap the map instead |
| Map pin not saved | Only one coordinate supplied | Supply both latitude and longitude, or clear both |
| Session resets after restart | `FOODBRIDGE_SECRET_KEY` changed | Keep a stable key in `.env` |
| Admin login fails | No admin account | Run `flask --app app create-admin ...` |
| Migration fails | Columns already exist or old MySQL version | Check `SHOW COLUMNS`; run on MySQL 8; apply only once |
| `utf8mb4` errors | Database/table charset wrong | Recreate with `CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci` |
| App works locally but not via tunnel | Supabase/Google URLs not updated | Add the tunnel domain to Supabase Site URL/Redirect URLs and Google origins |

---

## 12. Security checklist

- [ ] `.env` is never committed.
- [ ] Supabase secret/service-role key is only on the server.
- [ ] Google Client Secret is only in the Supabase dashboard.
- [ ] Database password is strong and not reused.
- [ ] MySQL port 3306 is not exposed to the internet.
- [ ] The app database user has only the privileges it needs.
- [ ] HTTPS is enabled for public use.
- [ ] `FOODBRIDGE_COOKIE_SECURE=1` on HTTPS.
- [ ] Admin accounts are created only through the CLI.
- [ ] Uploads are limited by size and validated by signature.
- [ ] Rotate keys if anything leaks.
- [ ] Keep MySQL and Python dependencies updated.
- [ ] Back up the MySQL database.
- [ ] Review OpenStreetMap/Nominatim usage policies before high traffic.
- [ ] Check current Supabase free-plan limits before a large launch.

---

## 13. Current state

- MySQL schema ready: `accounts`, `foods`, `orders`
- Python virtual environment created and dependencies installed
- Database connection check configured through `.env`
- Google sign-in and photo storage implemented; configure your own Supabase project using Sections 5.5–5.8
- App can be started with:
  ```powershell
  .\.venv\Scripts\python app.py
  ```
- Local URL: <http://localhost:5000>

Do not commit `.env`. It contains the MySQL password, Flask secret, and Supabase secret key.

---

## 14. Quick command reference

### Environment

```powershell
python -m venv .venv
.\.venv\Scripts\python -m pip install -r requirements.txt
```

### Database

```powershell
.\.venv\Scripts\flask --app app check-db
```

Import schema:

```powershell
& "C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe" -u foodbridge_app -p foodbridge -e "SOURCE C:/path/to/Foodbridge-main/database/schema.sql;"
```

### Admin

```powershell
.\.venv\Scripts\flask --app app create-admin admin@example.org
```

### Run

```powershell
.\.venv\Scripts\python app.py
```

### Tests

```powershell
.\.venv\Scripts\python -m unittest discover -s tests -v
```

### Generate a Flask secret

```powershell
.\.venv\Scripts\python -c "import secrets; print(secrets.token_urlsafe(48))"
```

### Check a Supabase bucket

```powershell
.\.venv\Scripts\python -c "import app, requests; r = requests.get(f'{app.SUPABASE_URL}/storage/v1/bucket/{app.SUPABASE_STORAGE_BUCKET}', headers={'apikey': app.SUPABASE_SERVICE_ROLE_KEY}); print(r.status_code, r.text)"
```

### Tunnel

```powershell
cloudflared tunnel --url http://localhost:5000
```

or

```powershell
ngrok http 5000
```

---

## 15. Glossary

| Term | Meaning |
|---|---|
| **Supabase** | A hosted backend platform. Here it provides Google OAuth and photo storage. |
| **MySQL** | The relational database that stores FoodBridge accounts, foods, and orders. |
| **OAuth** | The protocol that lets Google confirm a user's identity without sharing their Google password. |
| **Publishable key / anon key** | Public Supabase key safe to send to the browser. |
| **Secret key / service-role key** | Private Supabase key with elevated access. Server only. |
| **Supabase callback** | `https://<ref>.supabase.co/auth/v1/callback` — Google redirects here. |
| **App callback** | `http://localhost:5000/auth/callback` — Supabase redirects here after login. |
| **Publishable vs secret** | Supabase's new key names; `anon`/`service_role` are the legacy names. |
| **Leaflet** | Open-source JavaScript map library. |
| **OpenStreetMap** | Free map data and tiles used by Leaflet. |
| **Nominatim** | OpenStreetMap's free place-search service. |
| **Tunnel** | A service that exposes a local port through a public HTTPS URL. |
| **WSGI** | The Python interface used by production servers such as Gunicorn. |

---

## 16. Next steps

1. Open <http://localhost:5000/login>.
2. Click **Continue with Google** and complete the account chooser.
3. Finish the new-account form if it appears.
4. Create a test hotel account and publish a listing with a photo and map pin.
5. Create a test NGO account and request the listing.
6. Accept the request as the hotel.
7. Create an admin account and verify the dashboard.
8. Only after the local flow works, choose a tunnel or cloud host and update the Supabase/Google URLs.

If something fails, capture the exact error message (never a secret key) and check the troubleshooting table above.
