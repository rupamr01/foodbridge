import io
import tempfile
import unittest
from datetime import date, datetime
from decimal import Decimal
from pathlib import Path
from unittest.mock import Mock, patch
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import app as foodbridge_app


class FoodBridgeAppTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        foodbridge_app.app.config.update(
            TESTING=True,
            SECRET_KEY="test-only-secret-key",
        )

    def setUp(self):
        self.client = foodbridge_app.app.test_client()

    def test_authentication_pages_are_served_without_an_admin_signup_option(self):
        for path in ("/login", "/signup", "/admin", "/", "/auth/callback"):
            response = self.client.get(path)
            self.assertEqual(response.status_code, 200, path)
            page = response.get_data(as_text=True)
            if path == "/":
                self.assertIn("Join as a Hotel", page)
                self.assertIn("Join as an NGO", page)
                self.assertIn("Administrator sign in", page)
            if path == "/auth/callback":
                self.assertIn("GOOGLE SIGN-IN", page)
                self.assertIn("/static/js/callback.js", page)
            response.close()

        response = self.client.get("/signup")
        signup = response.get_data(as_text=True)
        response.close()
        self.assertIn('value="hotel"', signup)
        self.assertIn('value="ngo"', signup)
        self.assertNotIn('value="admin"', signup)

    def test_workspace_requires_login_and_static_assets_are_served(self):
        response = self.client.get("/app")
        self.assertEqual(response.status_code, 302)
        self.assertEqual(response.headers["Location"], "/login")

        response = self.client.get("/static/js/portal.js")
        self.assertEqual(response.status_code, 200)
        response.get_data()
        response.close()

        with self.client.session_transaction() as browser_session:
            browser_session["user_id"] = 1
            browser_session["role"] = "hotel"
        self.assertEqual(self.client.get("/app").status_code, 200)

    def test_public_api_rejects_admin_registration(self):
        response = self.client.post(
            "/api/register",
            json={
                "name": "Private Admin",
                "email": "admin@example.org",
                "password": "long-enough-password",
                "role": "admin",
            },
        )
        self.assertEqual(response.status_code, 400)
        self.assertIn("Hotel or NGO", response.get_json()["error"])

    def test_public_api_rejects_short_password_before_database_access(self):
        response = self.client.post(
            "/api/register",
            json={
                "name": "Community Hotel",
                "email": "hotel@example.org",
                "password": "short",
                "role": "hotel",
            },
        )
        self.assertEqual(response.status_code, 400)
        self.assertIn("10 characters", response.get_json()["error"])

    def test_protected_endpoints_require_an_authorized_session(self):
        for path in ("/api/me", "/api/foods", "/api/orders", "/api/admin/summary"):
            response = self.client.get(path)
            self.assertEqual(response.status_code, 401, path)

    def test_public_user_cannot_access_admin_data(self):
        with self.client.session_transaction() as browser_session:
            browser_session["user_id"] = 1
            browser_session["role"] = "hotel"

        response = self.client.get("/api/admin/summary")
        self.assertEqual(response.status_code, 403)

    def test_quantity_validation_accepts_only_supported_amounts(self):
        self.assertEqual(foodbridge_app.parse_quantity("2.5"), Decimal("2.5"))
        self.assertEqual(foodbridge_app.parse_quantity("10.00"), Decimal("10"))
        for invalid in ("", "0", "-3", "abc", "NaN", "Infinity", "1.001", "1000000"):
            self.assertIsNone(foodbridge_app.parse_quantity(invalid), invalid)

    def test_coordinate_validation_accepts_worldwide_points_only(self):
        self.assertEqual(
            foodbridge_app.parse_coordinate("30.3165", Decimal("90")),
            (Decimal("30.3165"), True),
        )
        self.assertEqual(
            foodbridge_app.parse_coordinate("-78.0322", Decimal("180")),
            (Decimal("-78.0322"), True),
        )
        self.assertEqual(foodbridge_app.parse_coordinate("", Decimal("90")), (None, True))
        self.assertEqual(foodbridge_app.parse_coordinate(None, Decimal("180")), (None, True))
        for invalid, limit in (
            ("90.0001", Decimal("90")),
            ("-90.0001", Decimal("90")),
            ("180.0001", Decimal("180")),
            ("abc", Decimal("90")),
            ("NaN", Decimal("90")),
            ("Infinity", Decimal("180")),
        ):
            self.assertEqual(
                foodbridge_app.parse_coordinate(invalid, limit),
                (None, False),
                invalid,
            )

    def test_image_signature_detection_supports_free_photo_formats(self):
        self.assertEqual(
            foodbridge_app.detect_image_type(b"\xff\xd8\xff\xe0jpeg-data"),
            ("image/jpeg", "jpg"),
        )
        self.assertEqual(
            foodbridge_app.detect_image_type(b"\x89PNG\r\n\x1a\npng-data"),
            ("image/png", "png"),
        )
        self.assertEqual(
            foodbridge_app.detect_image_type(b"RIFF\x10\x00\x00\x00WEBPVP8 "),
            ("image/webp", "webp"),
        )
        self.assertIsNone(foodbridge_app.detect_image_type(b"GIF89a not supported"))

    def test_food_photo_upload_requires_a_hotel_account(self):
        response = self.client.post(
            "/api/uploads/food-photo",
            data={},
            content_type="multipart/form-data",
        )
        self.assertEqual(response.status_code, 401)

        with self.client.session_transaction() as browser_session:
            browser_session["user_id"] = 1
            browser_session["role"] = "ngo"
        response = self.client.post(
            "/api/uploads/food-photo",
            data={},
            content_type="multipart/form-data",
        )
        self.assertEqual(response.status_code, 403)

    def test_food_photo_upload_uses_local_storage_when_supabase_is_off(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            with (
                patch.object(foodbridge_app, "SUPABASE_URL", ""),
                patch.object(foodbridge_app, "SUPABASE_SERVICE_ROLE_KEY", ""),
                patch.object(foodbridge_app, "PHOTO_UPLOAD_DIR", Path(temp_dir)),
            ):
                with self.client.session_transaction() as browser_session:
                    browser_session["user_id"] = 1
                    browser_session["role"] = "hotel"
                response = self.client.post(
                    "/api/uploads/food-photo",
                    data={
                        "photo": (
                            io.BytesIO(b"\xff\xd8\xff\xe0" + b"photo-data"),
                            "meal.jpg",
                        )
                    },
                    content_type="multipart/form-data",
                )
                self.assertEqual(response.status_code, 201)
                upload_url = response.get_json()["url"]
                self.assertTrue(upload_url.startswith("/static/uploads/food-photos/"))
                filename = upload_url.rsplit("/", 1)[-1]
                self.assertTrue((Path(temp_dir) / filename).is_file())

    def test_supabase_photo_upload_uses_apikey_only_for_new_secret_keys(self):
        response = Mock(status_code=201)
        with (
            patch.object(foodbridge_app, "SUPABASE_URL", "https://example.supabase.co"),
            patch.object(foodbridge_app, "SUPABASE_SERVICE_ROLE_KEY", "sb_secret_test"),
            patch.object(foodbridge_app, "SUPABASE_STORAGE_BUCKET", "food-photos"),
            patch.object(foodbridge_app.requests, "post", return_value=response) as post,
        ):
            upload_url = foodbridge_app.save_food_photo(
                b"\xff\xd8\xff\xe0photo-data",
                "image/jpeg",
                "jpg",
            )
        self.assertTrue(
            upload_url.startswith(
                "https://example.supabase.co/storage/v1/object/public/food-photos/"
            )
        )
        headers = post.call_args.kwargs["headers"]
        self.assertEqual(headers["apikey"], "sb_secret_test")
        self.assertNotIn("Authorization", headers)

    def test_supabase_photo_upload_keeps_bearer_for_legacy_service_role_keys(self):
        response = Mock(status_code=201)
        with (
            patch.object(foodbridge_app, "SUPABASE_URL", "https://example.supabase.co"),
            patch.object(
                foodbridge_app,
                "SUPABASE_SERVICE_ROLE_KEY",
                "legacy-service-role-key",
            ),
            patch.object(foodbridge_app, "SUPABASE_STORAGE_BUCKET", "food-photos"),
            patch.object(foodbridge_app.requests, "post", return_value=response) as post,
        ):
            foodbridge_app.save_food_photo(
                b"\xff\xd8\xff\xe0photo-data",
                "image/jpeg",
                "jpg",
            )
        headers = post.call_args.kwargs["headers"]
        self.assertEqual(
            headers["Authorization"],
            "Bearer legacy-service-role-key",
        )

    def test_food_photo_upload_rejects_non_images(self):
        with self.client.session_transaction() as browser_session:
            browser_session["user_id"] = 1
            browser_session["role"] = "hotel"
        response = self.client.post(
            "/api/uploads/food-photo",
            data={"photo": (io.BytesIO(b"GIF89a not supported"), "food.gif")},
            content_type="multipart/form-data",
        )
        self.assertEqual(response.status_code, 400)
        self.assertIn("JPEG, PNG, or WebP", response.get_json()["error"])

    def test_google_sign_in_requires_a_token_and_configuration(self):
        response = self.client.post("/api/auth/google", json={})
        self.assertEqual(response.status_code, 400)
        self.assertIn("token", response.get_json()["error"])

        with (
            patch.object(foodbridge_app, "SUPABASE_URL", ""),
            patch.object(foodbridge_app, "SUPABASE_ANON_KEY", ""),
        ):
            response = self.client.post(
                "/api/auth/google",
                json={"access_token": "test-token"},
            )
        self.assertEqual(response.status_code, 503)
        self.assertIn("not configured", response.get_json()["error"])

    def test_google_sign_in_rejects_non_google_identities(self):
        user = {
            "id": "email-user-id",
            "email": "user@example.org",
            "app_metadata": {"provider": "email"},
            "identities": [{"provider": "email"}],
        }
        with patch.object(
            foodbridge_app,
            "verify_supabase_token",
            return_value=(user, None),
        ):
            response = self.client.post(
                "/api/auth/google",
                json={"access_token": "test-token"},
            )
        self.assertEqual(response.status_code, 403)
        self.assertIn("Continue with Google", response.get_json()["error"])

    def test_public_google_config_never_exposes_the_service_role_key(self):
        with (
            patch.object(foodbridge_app, "SUPABASE_URL", "https://example.supabase.co"),
            patch.object(foodbridge_app, "SUPABASE_ANON_KEY", "public-anon-key"),
            patch.object(foodbridge_app, "SUPABASE_SERVICE_ROLE_KEY", "private-service-key"),
        ):
            response = self.client.get("/login")
            page = response.get_data(as_text=True)
            response.close()
        self.assertIn('content="https://example.supabase.co"', page)
        self.assertIn('content="public-anon-key"', page)
        self.assertNotIn("private-service-key", page)

    def test_decimal_and_date_database_values_are_json_serializable(self):
        value = {
            "quantity": Decimal("12.50"),
            "best_before": date(2026, 10, 17),
            "created_at": datetime(2026, 10, 16, 12, 0),
        }
        self.assertEqual(
            foodbridge_app.json_safe(value),
            {
                "quantity": "12.50",
                "best_before": "2026-10-17",
                "created_at": "2026-10-16T12:00:00",
            },
        )


if __name__ == "__main__":
    unittest.main()
