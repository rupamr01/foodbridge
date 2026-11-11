const googleProfileKey = "foodbridge_google_signup";
let googleClient;

async function submitAuth(form, endpoint, values) {
  const message = document.querySelector("#formMessage");
  const button = form.querySelector('button[type="submit"]');
  if (message) message.textContent = "";
  button.disabled = true;
  try {
    const response = await fetch(endpoint, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(values)
    });
    let result = {};
    try {
      result = await response.json();
    } catch {
      throw new Error("The server returned an unreadable response. Please try again.");
    }
    if (!response.ok) throw new Error(result.error || "Unable to complete sign in.");
    window.location.assign("/app");
  } catch (error) {
    if (message) message.textContent = error.message || "The server could not be reached. Please try again.";
  } finally {
    button.disabled = false;
  }
}

function readGoogleConfig() {
  return {
    url: document.querySelector('meta[name="supabase-url"]')?.content.trim() || "",
    anonKey: document.querySelector('meta[name="supabase-anon-key"]')?.content.trim() || ""
  };
}

function googleClientOrNull() {
  const { url, anonKey } = readGoogleConfig();
  if (!url || !anonKey || !window.supabase?.createClient) return null;
  if (!googleClient) {
    googleClient = window.supabase.createClient(url, anonKey, {
      auth: {
        flowType: "implicit",
        detectSessionInUrl: true,
        persistSession: true,
        autoRefreshToken: true
      }
    });
  }
  return googleClient;
}

function storeGoogleProfile(profile) {
  try {
    if (profile) sessionStorage.setItem(googleProfileKey, JSON.stringify(profile));
    else sessionStorage.removeItem(googleProfileKey);
  } catch {
    // The callback page will ask for the organization details if storage is unavailable.
  }
}

function revealGoogleControls() {
  if (!googleClientOrNull()) return;
  for (const [buttonId, dividerId] of [
    ["googleLoginButton", "googleLoginDivider"],
    ["googleSignupButton", "googleSignupDivider"]
  ]) {
    const button = document.querySelector(`#${buttonId}`);
    const divider = document.querySelector(`#${dividerId}`);
    if (button) button.hidden = false;
    if (divider) divider.hidden = false;
  }
  const signupNote = document.querySelector("#googleSignupNote");
  if (signupNote) signupNote.hidden = false;
}

async function startGoogleSignIn(profile, button) {
  const client = googleClientOrNull();
  const message = document.querySelector("#formMessage");
  if (!client) {
    if (message) message.textContent = "Google sign-in is not configured. Use email and password instead.";
    return;
  }

  if (message) message.textContent = "";
  button.disabled = true;
  storeGoogleProfile(profile);

  try {
    // Drop any stale Supabase session so the Google account chooser is always shown.
    await client.auth.signOut({ scope: "local" });
    const { error } = await client.auth.signInWithOAuth({
      provider: "google",
      options: {
        redirectTo: `${window.location.origin}/auth/callback`,
        queryParams: { prompt: "select_account" }
      }
    });
    if (error) throw error;
  } catch (error) {
    button.disabled = false;
    if (message) message.textContent = error.message || "Google sign-in could not start. Please try again.";
  }
}

const signupForm = document.querySelector("#signupForm");
if (signupForm) {
  const requestedRole = new URLSearchParams(window.location.search).get("role");
  if (requestedRole === "hotel" || requestedRole === "ngo") {
    document.querySelector("#role").value = requestedRole;
  }
  signupForm.addEventListener("submit", event => {
    event.preventDefault();
    const form = new FormData(signupForm);
    submitAuth(signupForm, "/api/register", {
      name: form.get("name"),
      role: form.get("role"),
      email: form.get("email"),
      password: form.get("password")
    });
  });
}

const googleSignupButton = document.querySelector("#googleSignupButton");
if (googleSignupButton && signupForm) {
  googleSignupButton.addEventListener("click", () => {
    const name = signupForm.querySelector("#name").value.trim();
    const role = signupForm.querySelector("#role").value;
    const message = document.querySelector("#formMessage");
    if (name.length < 2) {
      if (message) message.textContent = "Enter your organization name before continuing with Google.";
      signupForm.querySelector("#name").focus();
      return;
    }
    if (role !== "hotel" && role !== "ngo") {
      if (message) message.textContent = "Choose Hotel or NGO account type.";
      return;
    }
    startGoogleSignIn({ name, role }, googleSignupButton);
  });
}

const loginForm = document.querySelector("#loginForm");
if (loginForm) {
  loginForm.addEventListener("submit", event => {
    event.preventDefault();
    const form = new FormData(loginForm);
    submitAuth(loginForm, "/api/login", {
      email: form.get("email"),
      password: form.get("password")
    });
  });
}

const googleLoginButton = document.querySelector("#googleLoginButton");
if (googleLoginButton) {
  googleLoginButton.addEventListener("click", () => startGoogleSignIn(null, googleLoginButton));
}

const adminLoginForm = document.querySelector("#adminLoginForm");
if (adminLoginForm) {
  adminLoginForm.addEventListener("submit", event => {
    event.preventDefault();
    const form = new FormData(adminLoginForm);
    submitAuth(adminLoginForm, "/api/admin/login", {
      email: form.get("email"),
      password: form.get("password")
    });
  });
}

revealGoogleControls();
