# FoodBridge

FoodBridge connects hotels with surplus food to NGOs serving their communities. One Flask website provides the public landing page, email/password or Google sign-in, and a role-specific workspace for hotels, NGOs, and administrators.

## Features

- Hotel and NGO registration and sign-in with email/password
- Optional Google sign-in through Supabase Auth
- Camera or file photo uploads for food listings through Supabase Storage
- Exact worldwide pickup points with a Leaflet + OpenStreetMap pin picker
- Shared, MySQL-backed food listings
- NGO food requests with hotel accept/decline actions
- A separately provisioned administrator account and monitoring panel
- Responsive pages for desktop and mobile browsers
- Automatic refresh of open workspace pages every 30 seconds

## Technology

- Python 3.10+ and Flask
- MySQL 8 with the official `mysql-connector-python` driver
- Supabase free tier for Google sign-in and photo storage (optional)
- Leaflet, OpenStreetMap tiles, and Nominatim place search
- HTML, CSS, and JavaScript

The Leaflet and Supabase browser libraries are vendored under `static/vendor/`, so the app does not load them from a paid CDN or API. Map tiles and place search require an internet connection.

## Project structure

```text
.
├── app.py                  # Flask app, API routes, and CLI commands
├── database/
│   ├── schema.sql          # MySQL tables for new installations
│   └── migrations/         # Upgrade scripts for existing databases
├── static/
│   ├── assets/images/      # Logo
│   ├── css/style.css       # All site styles
│   ├── js/                 # Sign-in, OAuth callback, and workspace scripts
│   ├── uploads/            # Local photo fallback (created on first upload)
│   └── vendor/             # Leaflet and Supabase browser libraries
├── templates/              # Landing, auth, callback, admin, and workspace pages
├── tests/                  # Unit and optional MySQL workflow tests
├── .env.example            # Shareable configuration template
├── requirements.txt
└── start.sh                # Linux/macOS launcher
```

## Quick start

### 1. Create the MySQL database

```sh
mysql -u root -p
```

At the MySQL prompt:

```sql
CREATE DATABASE foodbridge CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE USER 'foodbridge'@'localhost' IDENTIFIED BY 'choose-a-strong-password';
GRANT SELECT, INSERT, UPDATE, DELETE ON foodbridge.* TO 'foodbridge'@'localhost';
EXIT;
```

Import the tables:

```sh
mysql -u root -p foodbridge < database/schema.sql
```

### 2. Configure private settings

```sh
cp .env.example .env
```

Set `FOODBRIDGE_SECRET_KEY` to a long random value and update the `FOODBRIDGE_DB_*` values for your MySQL setup. `.env` is private and ignored by Git.

### 3. Install dependencies

Linux/macOS:

```sh
python3 -m venv .venv
.venv/bin/python -m pip install -r requirements.txt
```

Windows PowerShell:

```powershell
py -3 -m venv .venv
.\.venv\Scripts\python -m pip install -r requirements.txt
```

### 4. Create the administrator account

Administrator registration is intentionally not public. Create it once from the project root:

Linux/macOS:

```sh
.venv/bin/flask --app app create-admin admin@example.org
```

Windows PowerShell:

```powershell
.\.venv\Scripts\flask --app app create-admin admin@example.org
```

## Free Google sign-in, photos, and maps

This setup uses only free services: Google OAuth, the Supabase free plan, and OpenStreetMap.

### 1. Create a Supabase project

1. Sign in at [supabase.com](https://supabase.com) and create a project on the free plan.
2. Wait for the project to finish provisioning.

### 2. Create the Google OAuth credentials

1. Open the [Google Cloud Console](https://console.cloud.google.com/) and create or select a project.
2. Open the [Google Auth Platform](https://console.cloud.google.com/auth/overview) and configure **Audience** (choose **External**, or keep it in **Testing** and add your own Google account under **Test users**) and **Branding**.
3. Open **Clients → Create client** (or **APIs & Services → Credentials → Create credentials → OAuth client ID**).
4. Choose **Web application**.
5. Under **Authorized JavaScript origins**, add:
   - `http://localhost:5000`
   - `http://127.0.0.1:5000`
6. Under **Authorized redirect URIs**, add the callback URL shown on Supabase's Google provider page:
   - `https://<your-project-ref>.supabase.co/auth/v1/callback`
7. Click **Create** and copy the client ID and client secret.

### 3. Enable Google in Supabase

1. In Supabase, open **Authentication → Providers → Google**.
2. Enable the provider and paste the Google client ID and client secret.
3. Keep user sign-ups enabled in **Authentication → Sign In / Up** so first-time Google users can create an account.
4. Open **Authentication → URL Configuration**.
5. Set the **Site URL** to `http://localhost:5000`.
6. Add these **Redirect URLs**:
   - `http://localhost:5000/auth/callback`
   - `http://127.0.0.1:5000/auth/callback`
   - `https://your-production-domain/auth/callback` when you deploy

If you test from a phone on your LAN, also add the LAN URL you actually use, for example `http://192.168.1.20:5000/auth/callback`.

### 4. Create the photo bucket

1. Open **Storage → New bucket**.
2. Name it `food-photos`.
3. Enable **Public bucket** so the uploaded photos can be displayed.
4. Keep the default bucket name, or update `SUPABASE_STORAGE_BUCKET` in `.env`.

Photo uploads use the server-side secret key, so no storage write policy is needed. Public read access is required for the image URLs.

### 5. Add the Supabase settings

Open **Project Settings → API Keys** (there is no separate **Settings → API** page anymore). Copy the **Project URL**, **publishable key**, and **secret key** into `.env`:

```dotenv
SUPABASE_URL=https://your-project-ref.supabase.co
SUPABASE_PUBLISHABLE_KEY=sb_publishable_...
SUPABASE_SECRET_KEY=sb_secret_...
SUPABASE_STORAGE_BUCKET=food-photos
```

The **Connect** dialog on the project dashboard also shows the URL and publishable key.

Older projects may still show **anon** and **service_role** keys under the **Legacy API keys** tab. They work too — use the original names:

```dotenv
SUPABASE_ANON_KEY=your-anon-public-key
SUPABASE_SERVICE_ROLE_KEY=your-service-role-key
```

The publishable/anon key is public and is sent to the browser. The secret/service-role key must stay private on the server. Never commit `.env`.

Restart the Flask app after changing `.env`. The Google buttons appear automatically when the Supabase URL and publishable (or anon) key are present.

### Existing databases

New installations already get the new columns from `database/schema.sql`. For an existing FoodBridge database, run the migration once:

```sh
mysql -u root -p foodbridge < database/migrations/001_google_auth_photos_maps.sql
```

### How it behaves

- **Google sign-in:** Supabase verifies the Google identity, and the Flask server verifies the Supabase access token before creating a FoodBridge session. Existing email/password accounts are linked by email on first Google sign-in.
- **New Google accounts:** the callback page asks for the organization name and account type before creating the FoodBridge account.
- **Photos:** the browser resizes large images, the server checks the file signature, and the file is stored in Supabase Storage. If Supabase is not configured, photos are saved under `static/uploads/food-photos/`.
- **Locations:** the hotel can search for a place, use the browser's current location, or tap the map to place a precise pin. NGOs see a mini map and an OpenStreetMap link for each listing.
- **Without Supabase:** email/password sign-in, local photo storage, and maps still work. The Google buttons stay hidden.

OpenStreetMap tiles and Nominatim search are free to use under their public usage policies. For a high-traffic production site, use a dedicated tile provider or self-host the services. Check the current Supabase free-plan limits before relying on it for a large deployment.

## Run the website

Linux/macOS:

```sh
./start.sh
```

Windows PowerShell:

```powershell
.\.venv\Scripts\flask --app app check-db
.\.venv\Scripts\python app.py
```

Open **http://127.0.0.1:5000**. The landing page links to hotel/NGO registration and sign-in; administrator sign-in is linked in the footer.

To listen on another interface or port, set `FOODBRIDGE_HOST` and `FOODBRIDGE_PORT`. For example, to test from a phone on the same trusted Wi-Fi network:

Linux/macOS:

```sh
FOODBRIDGE_HOST=0.0.0.0 FOODBRIDGE_PORT=5000 ./start.sh
```

Windows PowerShell:

```powershell
$env:FOODBRIDGE_HOST = "0.0.0.0"
$env:FOODBRIDGE_PORT = "5000"
.\.venv\Scripts\python app.py
```

Then open `http://<computer-LAN-IP>:5000` on the phone. Only the Python server connects to MySQL. Keep the server on a trusted network; do not expose the Flask development server to the public internet.

Browser geolocation usually requires HTTPS or `localhost`. On a plain HTTP LAN address, tap the map to choose the pickup point manually.

For production, use a production WSGI server, HTTPS, `FOODBRIDGE_COOKIE_SECURE=1`, and protected database credentials.

## Tests

```sh
.venv/bin/python -m unittest discover -s tests -v
```

Windows PowerShell:

```powershell
.\.venv\Scripts\python -m unittest discover -s tests -v
```

The unit tests do not require MySQL. The end-to-end workflow test runs only when `FOODBRIDGE_TEST_DB_NAME` points to a separate disposable database with `database/schema.sql` imported. Optionally provide `FOODBRIDGE_TEST_DB_HOST`, `FOODBRIDGE_TEST_DB_PORT`, `FOODBRIDGE_TEST_DB_USER`, and `FOODBRIDGE_TEST_DB_PASSWORD`. Never point this test at a database containing data you need to keep.

## Current status

Hotels can publish food listings with photos and exact map pins, then accept or decline NGO orders. NGOs can browse available listings, see pickup locations, request quantities, and track order status. Administrators can monitor accounts, listings, and orders. Server-side role checks protect every workspace endpoint. There are no seeded demo accounts or records.

Email verification for password accounts, password recovery, delivery tracking, automatic matching, and email notifications are not implemented. Google sign-in emails are verified by Google.
