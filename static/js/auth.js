const googleProfileKey = "foodbridge_google_signup";
let googleClient;

function formMessage(form) {
  return form?.querySelector(".form-message") || document.querySelector("#formMessage");
}

function setMessage(form, text, success = false) {
  const message = formMessage(form);
  if (!message) return;
  message.textContent = text || "";
  message.classList.toggle("success", Boolean(success && text));
}

async function postJson(endpoint, values) {
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
  return { response, result };
}

async function submitAuth(form, endpoint, values, options = {}) {
  const button = form.querySelector('button[type="submit"]');
  setMessage(form, "");
  if (button) button.disabled = true;
  try {
    const { response, result } = await postJson(endpoint, values);
    if (!response.ok) {
      if (options.onError && options.onError(response, result)) return;
      throw new Error(result.error || "Unable to complete sign in.");
    }
    if (options.onSuccess && options.onSuccess(response, result)) return;
    window.location.assign("/app");
  } catch (error) {
    setMessage(form, error.message || "The server could not be reached. Please try again.");
  } finally {
    if (button) button.disabled = false;
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

function setupOtpStep(mainStep) {
  const step = document.querySelector("#otpStep");
  const form = document.querySelector("#otpForm");
  if (!step || !form) return null;

  const emailLabel = document.querySelector("#otpEmail");
  const resendButton = document.querySelector("#resendOtpButton");
  const codeInput = form.querySelector("#otpCode");
  let email = "";
  let countdownTimer = null;

  function startCountdown(seconds) {
    if (!resendButton) return;
    if (countdownTimer) clearInterval(countdownTimer);
    let remaining = Math.max(0, Number(seconds) || 0);
    const render = () => {
      if (remaining > 0) {
        resendButton.disabled = true;
        resendButton.textContent = `Resend code in ${remaining}s`;
        remaining -= 1;
      } else {
        resendButton.disabled = false;
        resendButton.textContent = "Resend code";
        if (countdownTimer) {
          clearInterval(countdownTimer);
          countdownTimer = null;
        }
      }
    };
    render();
    if (remaining > 0) countdownTimer = setInterval(render, 1000);
  }

  function show(nextEmail, message = "", resendAfter = 0) {
    email = nextEmail || email;
    if (mainStep) mainStep.hidden = true;
    step.hidden = false;
    if (emailLabel) emailLabel.textContent = email;
    setMessage(form, message, true);
    codeInput.value = "";
    startCountdown(resendAfter);
    codeInput.focus();
  }

  codeInput?.addEventListener("input", () => {
    codeInput.value = codeInput.value.replace(/\D/g, "").slice(0, 6);
  });

  form.addEventListener("submit", event => {
    event.preventDefault();
    const code = codeInput.value.trim();
    if (!/^\d{6}$/.test(code)) {
      setMessage(form, "Enter the 6-digit code from your email.");
      return;
    }
    submitAuth(form, "/api/verify-email", { email, code }, {
      onSuccess: (response, result) => {
        if (result.logged_in) window.location.assign("/app");
        else window.location.assign("/login?verified=1");
        return true;
      }
    });
  });

  resendButton?.addEventListener("click", async () => {
    if (!email) return;
    setMessage(form, "");
    resendButton.disabled = true;
    try {
      const { response, result } = await postJson("/api/resend-otp", { email });
      if (!response.ok) {
        if (response.status === 429) {
          startCountdown(result.retry_after || 60);
        } else {
          resendButton.disabled = false;
        }
        setMessage(form, result.error || "Could not send a new code. Please try again.");
        return;
      }
      setMessage(
        form,
        result.otp_sent
          ? "A new code has been sent. Check your inbox and spam folder."
          : result.message || "If this email needs verification, a new code was sent.",
        true
      );
      startCountdown(result.retry_after || 60);
    } catch (error) {
      resendButton.disabled = false;
      setMessage(form, error.message || "Could not send a new code. Please try again.");
    }
  });

  return { show };
}

const signupForm = document.querySelector("#signupForm");
const signupOtp = setupOtpStep(document.querySelector("#signupStep"));
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
    }, {
      onSuccess: (response, result) => {
        if (!result.verification_required) return false;
        signupOtp?.show(
          result.email || form.get("email"),
          result.message || "Enter the 6-digit code we emailed you.",
          result.otp_sent ? 60 : result.retry_after || 0
        );
        return true;
      }
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
const loginOtp = setupOtpStep(document.querySelector("#loginStep"));
if (loginForm) {
  loginForm.addEventListener("submit", event => {
    event.preventDefault();
    const form = new FormData(loginForm);
    submitAuth(loginForm, "/api/login", {
      email: form.get("email"),
      password: form.get("password")
    }, {
      onError: (response, result) => {
        if (result.code !== "email_unverified") return false;
        loginOtp?.show(
          result.email || form.get("email"),
          "Enter the code we emailed you. It expires in 10 minutes.",
          0
        );
        return true;
      }
    });
  });

  if (new URLSearchParams(window.location.search).get("verified") === "1") {
    setMessage(loginForm, "Your email is verified. Sign in to continue.", true);
  }
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
