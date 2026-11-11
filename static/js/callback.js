const googleProfileKey = "foodbridge_google_signup";

const callbackTitle = document.querySelector("#callbackTitle");
const callbackMessage = document.querySelector("#callbackMessage");
const callbackProgress = document.querySelector("#callbackProgress");
const callbackActions = document.querySelector("#callbackActions");
const callbackSwitch = document.querySelector("#callbackSwitch");
const completionForm = document.querySelector("#googleCompleteForm");
const completionMessage = document.querySelector("#googleFormMessage");
const completionName = document.querySelector("#googleName");
const completionRole = document.querySelector("#googleRole");

let supabaseClient;
let accessToken = "";

function readConfig() {
  return {
    url: document.querySelector('meta[name="supabase-url"]')?.content.trim() || "",
    anonKey: document.querySelector('meta[name="supabase-anon-key"]')?.content.trim() || ""
  };
}

function readStoredProfile() {
  try {
    const raw = sessionStorage.getItem(googleProfileKey);
    if (!raw) return null;
    const profile = JSON.parse(raw);
    const name = String(profile?.name || "").trim();
    if (!["hotel", "ngo"].includes(profile?.role) || name.length < 2 || name.length > 160) {
      return null;
    }
    return { name, role: profile.role };
  } catch {
    return null;
  }
}

function clearStoredProfile() {
  try {
    sessionStorage.removeItem(googleProfileKey);
  } catch {
    // The profile is optional; storage can be unavailable in private windows.
  }
}

function urlAuthError() {
  const query = new URLSearchParams(window.location.search);
  const hash = new URLSearchParams(window.location.hash.replace(/^#/, ""));
  for (const params of [hash, query]) {
    const description = params.get("error_description");
    if (description) return description.replace(/\+/g, " ");
    if (params.get("error")) return "Google sign-in was cancelled or could not be completed.";
  }
  return "";
}

function setStatus(title, message, { progress = false, actions = false } = {}) {
  callbackTitle.textContent = title;
  callbackMessage.textContent = message;
  callbackProgress.hidden = !progress;
  callbackActions.hidden = !actions;
  callbackSwitch.hidden = !actions;
  completionForm.hidden = true;
}

function showError(message) {
  setStatus("Google sign-in could not be completed", message, { actions: true });
}

function showCompletion(user, note = "") {
  callbackTitle.textContent = "One last step";
  callbackMessage.textContent = note
    || `Signed in as ${user?.email || "your Google account"}. Choose how you will use FoodBridge.`;
  callbackProgress.hidden = true;
  callbackActions.hidden = true;
  callbackSwitch.hidden = true;
  completionMessage.textContent = "";
  completionForm.hidden = false;

  const profile = readStoredProfile();
  if (!completionName.value) {
    completionName.value = String(
      profile?.name || user?.user_metadata?.full_name || ""
    ).slice(0, 160);
  }
  if (["hotel", "ngo"].includes(profile?.role)) {
    completionRole.value = profile.role;
  }
  completionName.focus();
}

async function submitGoogleAccount(profile = {}) {
  if (supabaseClient) {
    const { data } = await supabaseClient.auth.getSession();
    if (data.session?.access_token) accessToken = data.session.access_token;
  }

  const response = await fetch("/api/auth/google", {
    method: "POST",
    credentials: "same-origin",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ access_token: accessToken, ...profile })
  });

  let result = {};
  try {
    result = await response.json();
  } catch {
    throw new Error("The server returned an unreadable response. Please try again.");
  }

  if (!response.ok) {
    if (response.status === 409 && result.code === "registration_required") {
      const { data } = await supabaseClient.auth.getSession();
      showCompletion(data.session?.user, result.error);
      return;
    }
    throw new Error(result.error || "FoodBridge could not complete the sign-in.");
  }

  clearStoredProfile();
  completionForm.hidden = true;
  setStatus("You're signed in", "Opening your FoodBridge workspace…", { progress: true });
  window.location.replace("/app");
}

async function start() {
  const urlError = urlAuthError();
  if (urlError) {
    showError(urlError);
    return;
  }

  const { url, anonKey } = readConfig();
  if (!url || !anonKey) {
    showError("Google sign-in is not configured. Add the Supabase settings to .env and restart FoodBridge.");
    return;
  }
  if (!window.supabase?.createClient) {
    showError("The Google sign-in library could not load. Refresh the page and try again.");
    return;
  }

  supabaseClient = window.supabase.createClient(url, anonKey, {
    auth: {
      flowType: "implicit",
      detectSessionInUrl: true,
      persistSession: true,
      autoRefreshToken: true
    }
  });

  let session;
  try {
    const { data, error } = await supabaseClient.auth.getSession();
    if (error) throw error;
    session = data.session;
  } catch (error) {
    showError(error.message || "Could not read your Google session.");
    return;
  }

  if (!session?.access_token) {
    showError("No Google session was found. Start again from the sign-in page.");
    return;
  }

  accessToken = session.access_token;
  const storedProfile = readStoredProfile();

  try {
    await submitGoogleAccount(
      storedProfile ? { role: storedProfile.role, name: storedProfile.name } : {}
    );
  } catch (error) {
    try {
      await supabaseClient.auth.signOut({ scope: "local" });
    } catch {
      // The message below is more useful than a sign-out failure.
    }
    showError(error.message || "Google sign-in could not be completed.");
  }
}

completionForm.addEventListener("submit", async event => {
  event.preventDefault();
  const fields = new FormData(completionForm);
  const name = String(fields.get("name") || "").trim();
  const role = fields.get("role");

  if (name.length < 2 || name.length > 160) {
    completionMessage.textContent = "Enter an organization name between 2 and 160 characters.";
    completionName.focus();
    return;
  }
  if (!["hotel", "ngo"].includes(role)) {
    completionMessage.textContent = "Choose Hotel or NGO account type.";
    return;
  }

  const button = completionForm.querySelector('button[type="submit"]');
  button.disabled = true;
  completionMessage.textContent = "";
  try {
    await submitGoogleAccount({ name, role });
  } catch (error) {
    completionMessage.textContent = error.message || "The account could not be created.";
    button.disabled = false;
  }
});

start();
