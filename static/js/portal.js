const roleLabels = { hotel: "Hotel · Food donor", ngo: "NGO · Food recipient", admin: "Administrator" };
const navIcons = {
  overview: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="7" height="7" rx="1.5"/><rect x="14" y="3" width="7" height="7" rx="1.5"/><rect x="3" y="14" width="7" height="7" rx="1.5"/><rect x="14" y="14" width="7" height="7" rx="1.5"/></svg>',
  food: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round"><path d="M20 4c-9 0-14 5-14 11a5 5 0 0 0 5 5c6 0 9-7 9-16Z"/><path d="M4 20c2-6 6-9 11-11"/></svg>',
  search: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round"><circle cx="11" cy="11" r="7"/><path d="m20 20-4-4"/></svg>',
  orders: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round"><path d="M9 4H7a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2V6a2 2 0 0 0-2-2h-2"/><rect x="9" y="2" width="6" height="4" rx="1.2"/><path d="M9 11h6M9 15h4"/></svg>',
  profile: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="8" r="3.5"/><path d="M5 20c.8-3.8 3.5-6 7-6s6.2 2.2 7 6"/></svg>',
  accounts: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round"><circle cx="9" cy="8" r="3.2"/><path d="M2.8 20c.7-3.6 3.2-5.7 6.2-5.7s5.5 2.1 6.2 5.7"/><path d="M16 5.2a3.2 3.2 0 0 1 0 6.1"/><path d="M17.5 14.7c2.1.5 3.5 2.4 4 5.3"/></svg>'
};
const roleNavigation = {
  hotel: [
    ["overview", navIcons.overview, "Overview"],
    ["foods", navIcons.food, "My food listings"],
    ["orders", navIcons.orders, "NGO requests"],
    ["profile", navIcons.profile, "My account"]
  ],
  ngo: [
    ["overview", navIcons.overview, "Overview"],
    ["foods", navIcons.search, "Browse food"],
    ["orders", navIcons.orders, "My orders"],
    ["profile", navIcons.profile, "My account"]
  ],
  admin: [
    ["overview", navIcons.overview, "Platform overview"],
    ["accounts", navIcons.accounts, "Accounts"],
    ["foods", navIcons.food, "Food listings"],
    ["orders", navIcons.orders, "Orders"]
  ]
};

const content = document.querySelector("#pageContent");
const pageName = document.querySelector("#pageName");
const sidebar = document.querySelector("#sidebar");
const navigation = document.querySelector("#mainNavigation");
let account;
let activePage = "overview";
let toastTimeout;
let refreshTimer;
let renderVersion = 0;
let foodPhotoFile = null;
let photoPreviewUrl = null;
let pickupPoint = null;
let locationMap = null;
let locationMarker = null;
let foodMaps = [];

const mapTileUrl = "https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png";
const mapAttribution = '&copy; <a href="https://www.openstreetmap.org/copyright" target="_blank" rel="noopener noreferrer">OpenStreetMap</a> contributors';

function escapeHtml(value) {
  return String(value ?? "").replace(/[&<>"']/g, char => ({
    "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;"
  }[char]));
}

function statusLabel(status) {
  return String(status ?? "").replace(/_/g, " ").replace(/^./, char => char.toUpperCase());
}

function todayIso() {
  const today = new Date();
  const month = String(today.getMonth() + 1).padStart(2, "0");
  const day = String(today.getDate()).padStart(2, "0");
  return `${today.getFullYear()}-${month}-${day}`;
}

function cleanupInteractive() {
  foodMaps.forEach(map => map.remove());
  foodMaps = [];
  if (locationMap) {
    locationMap.remove();
    locationMap = null;
    locationMarker = null;
  }
  if (photoPreviewUrl) {
    URL.revokeObjectURL(photoPreviewUrl);
    photoPreviewUrl = null;
  }
  foodPhotoFile = null;
  pickupPoint = null;
}

function formatBytes(bytes) {
  if (bytes < 1024 * 1024) return `${Math.max(1, Math.round(bytes / 1024))} KB`;
  return `${(bytes / (1024 * 1024)).toFixed(1)} MB`;
}

function coordinateText(value) {
  return Number(value).toFixed(6).replace(/0+$/, "").replace(/\.$/, "");
}

function openStreetMapUrl(latitude, longitude) {
  return `https://www.openstreetmap.org/?mlat=${latitude}&mlon=${longitude}#map=17/${latitude}/${longitude}`;
}

function foodLocation(food) {
  if (
    food.latitude === null || food.latitude === undefined || food.latitude === ""
    || food.longitude === null || food.longitude === undefined || food.longitude === ""
  ) {
    return "";
  }
  const latitude = Number(food.latitude);
  const longitude = Number(food.longitude);
  if (
    !Number.isFinite(latitude) || !Number.isFinite(longitude)
    || Math.abs(latitude) > 90 || Math.abs(longitude) > 180
  ) {
    return "";
  }
  const coordinates = `${coordinateText(latitude)}, ${coordinateText(longitude)}`;
  const mapUrl = openStreetMapUrl(latitude, longitude);
  return `<div class="food-location">
    <div class="food-map-thumb" data-food-map data-lat="${latitude}" data-lng="${longitude}" aria-label="Map of the pickup point for ${escapeHtml(food.name)}" title="Open this pickup point in OpenStreetMap"></div>
    <div class="food-location-meta"><span>Exact pickup point</span><a class="map-link" href="${mapUrl}" target="_blank" rel="noopener noreferrer">${coordinates} · Open map ↗</a></div>
  </div>`;
}

async function api(url, options = {}) {
  const response = await fetch(url, {
    credentials: "same-origin",
    ...options,
    headers: { ...(options.headers || {}), ...(options.body ? { "Content-Type": "application/json" } : {}) }
  });
  let result;
  try {
    result = await response.json();
  } catch {
    throw new Error("The server returned an unreadable response.");
  }
  if (response.status === 401) {
    window.location.replace("/login");
    const error = new Error(result.error || "Please sign in again.");
    error.status = 401;
    throw error;
  }
  if (!response.ok) {
    const error = new Error(result.error || "The request could not be completed.");
    error.status = response.status;
    throw error;
  }
  return result;
}

function toast(message, isError = false) {
  const element = document.querySelector("#toast");
  element.textContent = message;
  element.classList.toggle("toast-error", isError);
  element.classList.add("show");
  window.clearTimeout(toastTimeout);
  toastTimeout = window.setTimeout(() => element.classList.remove("show"), 3500);
}

function renderNavigation() {
  navigation.innerHTML = roleNavigation[account.role].map(([page, icon, label]) => `
    <button class="nav-item${page === activePage ? " active" : ""}" type="button" data-page="${page}">
      <span aria-hidden="true">${icon}</span>${label}
    </button>`).join("");
}

function titleFor(page) {
  const item = roleNavigation[account.role].find(([key]) => key === page);
  return item ? item[2] : "Overview";
}

function heading(title, subtitle, action = "") {
  return `<div class="page-title"><div><p class="eyebrow">${escapeHtml(roleLabels[account.role])}</p><h1>${escapeHtml(title)}</h1><p>${escapeHtml(subtitle)}</p></div>${action}</div>`;
}

function statCard(label, value, note) {
  return `<article class="stat-card"><div class="stat-top">${escapeHtml(label)}</div><div class="stat-value">${escapeHtml(value)}</div><div class="stat-foot">${escapeHtml(note)}</div></article>`;
}

function emptyState(title, text) {
  return `<div class="empty-state"><h3>${escapeHtml(title)}</h3><p>${escapeHtml(text)}</p></div>`;
}

function foodCard(food, canOrder) {
  const quantity = Number(food.available_quantity);
  return `<article class="food-card">
    <div class="food-card-top"><span class="food-category">${escapeHtml(food.category)}</span><span class="status ${escapeHtml(food.status)}">${escapeHtml(statusLabel(food.status))}</span></div>
    ${food.photo_url ? `<img class="food-photo" src="${escapeHtml(food.photo_url)}" alt="Photo of ${escapeHtml(food.name)}" loading="lazy">` : ""}
    <h2>${escapeHtml(food.name)}</h2>
    <p class="food-description">${escapeHtml(food.description || "No additional details provided.")}</p>
    ${foodLocation(food)}
    <div class="food-details">
      <div><small>Available</small><strong>${escapeHtml(food.available_quantity)} ${escapeHtml(food.unit)}</strong></div>
      <div><small>Best before</small><strong>${escapeHtml(food.best_before)}</strong></div>
      <div class="food-detail-wide"><small>Pickup</small><strong>${escapeHtml(food.pickup_address)}</strong></div>
      <div class="food-detail-wide"><small>Donor</small><strong>${escapeHtml(food.donor_name)}</strong></div>
    </div>
    ${canOrder ? `<form class="order-form" data-order-food="${food.id}">
      <div class="field"><label for="order-quantity-${food.id}">Quantity to request</label><input id="order-quantity-${food.id}" name="quantity" type="number" min="0.01" max="${quantity}" step="0.01" required></div>
      <div class="field"><label for="order-note-${food.id}">Message to donor <span class="field-hint">(optional)</span></label><textarea id="order-note-${food.id}" name="note" rows="2" maxlength="1000"></textarea></div>
      <button class="btn btn-primary" type="submit">Request this food</button>
    </form>` : ""}
  </article>`;
}

function orderCard(order, canManage) {
  const actions = canManage && order.status === "pending" ? `
    <div class="order-actions">
      <button class="btn btn-primary" data-order-status="accepted" data-order-id="${order.id}">Accept request</button>
      <button class="btn btn-outline" data-order-status="rejected" data-order-id="${order.id}">Decline</button>
    </div>` : "";
  return `<article class="order-card">
    <div class="order-card-heading"><div><h3>${escapeHtml(order.food_name)}</h3><p>${canManage ? `From ${escapeHtml(order.ngo_name)}` : `Donor: ${escapeHtml(order.hotel_name)}`}</p></div><span class="status ${escapeHtml(order.status)}">${escapeHtml(statusLabel(order.status))}</span></div>
    <p><strong>Requested:</strong> ${escapeHtml(order.quantity)} ${escapeHtml(order.unit)}</p>
    ${order.note ? `<p><strong>Message:</strong> ${escapeHtml(order.note)}</p>` : ""}
    <p class="muted"><time datetime="${escapeHtml(order.created_at)}">${escapeHtml(order.created_at)}</time></p>
    ${actions}
  </article>`;
}

function donationForm() {
  return `<section class="panel form-panel">
    <div class="panel-heading"><div><h2>Share surplus food</h2><p>Post a listing with a photo and an exact pickup point for registered NGO accounts.</p></div></div>
    <form id="foodForm" class="form-grid">
      <div class="field"><label for="food-name">Food name</label><input id="food-name" name="name" maxlength="180" required></div>
      <div class="field"><label for="food-category">Category</label><select id="food-category" name="category" required>
        <option value="">Choose a category</option><option>Fresh Produce</option><option>Cooked Meals</option><option>Bakery</option><option>Dairy</option><option>Packaged Food</option><option>Other</option>
      </select></div>
      <div class="field"><label for="food-quantity">Quantity</label><input id="food-quantity" name="quantity" type="number" min="0.01" step="0.01" max="999999" required></div>
      <div class="field"><label for="food-unit">Unit</label><input id="food-unit" name="unit" maxlength="32" placeholder="meals, kg, boxes" required></div>
      <div class="field"><label for="food-best-before">Best before</label><input id="food-best-before" name="best_before" type="date" min="${todayIso()}" required></div>
      <div class="field"><label for="food-pickup">Pickup address</label><input id="food-pickup" name="pickup_address" maxlength="500" required></div>

      <div class="field field-wide">
        <label>Food photo <span class="field-hint">(optional)</span></label>
        <div class="photo-picker">
          <div class="photo-preview" id="photoPreview" hidden>
            <img id="photoPreviewImage" alt="Selected food photo preview">
            <button class="btn btn-outline photo-remove" id="removePhoto" type="button">Remove photo</button>
          </div>
          <div class="photo-choices" id="photoChoices">
            <label class="photo-choice">
              <span aria-hidden="true">📷</span><strong>Take a photo</strong><small>Use your device camera</small>
              <input id="food-photo-camera" class="photo-input" type="file" accept="image/*" capture="environment">
            </label>
            <label class="photo-choice">
              <span aria-hidden="true">🖼️</span><strong>Choose a photo</strong><small>JPEG, PNG, or WebP · up to 5 MB</small>
              <input id="food-photo-file" class="photo-input" type="file" accept="image/*">
            </label>
          </div>
          <p class="photo-status" id="photoStatus" role="status" aria-live="polite">A photo helps NGOs recognize the food.</p>
        </div>
      </div>

      <div class="field field-wide">
        <label>Exact pickup location <span class="field-hint">(optional)</span></label>
        <div class="map-picker">
          <div class="map-toolbar">
            <input id="mapSearch" type="search" placeholder="Search a city, area, or landmark" maxlength="200" autocomplete="off">
            <button class="btn btn-outline" id="mapSearchButton" type="button">Search</button>
            <button class="btn btn-outline" id="useLocationButton" type="button">Use my location</button>
          </div>
          <div class="food-map" id="foodMap" aria-label="Pickup location map"></div>
          <div class="map-footer">
            <span id="mapCoordinates">Tap the map or use your location to drop a pin.</span>
            <button class="btn btn-outline" id="clearLocationButton" type="button" hidden>Clear pin</button>
          </div>
        </div>
      </div>

      <div class="field field-wide"><label for="food-description">Description / pickup instructions</label><textarea id="food-description" name="description" maxlength="2000" rows="3"></textarea></div>
      <div class="field field-wide"><button class="btn btn-primary" type="submit">Publish food listing</button></div>
    </form>
  </section>`;
}

function photoStatus(message, isError = false) {
  const element = document.querySelector("#photoStatus");
  if (!element) return;
  element.textContent = message;
  element.classList.toggle("error", isError);
}

function loadImageFile(file) {
  return new Promise((resolve, reject) => {
    const image = new Image();
    const url = URL.createObjectURL(file);
    image.onload = () => {
      URL.revokeObjectURL(url);
      resolve(image);
    };
    image.onerror = () => {
      URL.revokeObjectURL(url);
      reject(new Error("That photo could not be read. Choose a JPEG, PNG, or WebP image."));
    };
    image.src = url;
  });
}

async function prepareFoodPhoto(file) {
  if (!["image/jpeg", "image/png", "image/webp"].includes(file.type)) {
    throw new Error("Choose a JPEG, PNG, or WebP photo.");
  }
  if (file.size > 25 * 1024 * 1024) {
    throw new Error("Choose a photo smaller than 25 MB.");
  }

  const image = await loadImageFile(file);
  const width = image.naturalWidth || image.width;
  const height = image.naturalHeight || image.height;
  const scale = Math.min(1, 1600 / Math.max(width, height));
  if (scale === 1 && file.size <= 5 * 1024 * 1024) return file;

  const canvas = document.createElement("canvas");
  canvas.width = Math.max(1, Math.round(width * scale));
  canvas.height = Math.max(1, Math.round(height * scale));
  const context = canvas.getContext("2d");
  if (!context) throw new Error("The photo could not be processed.");
  context.drawImage(image, 0, 0, canvas.width, canvas.height);

  const blob = await new Promise((resolve, reject) => {
    canvas.toBlob(
      result => result ? resolve(result) : reject(new Error("The photo could not be processed.")),
      "image/jpeg",
      0.86
    );
  });
  if (blob.size > 5 * 1024 * 1024) {
    throw new Error("The photo is still larger than 5 MB after resizing. Choose a smaller image.");
  }
  return new File([blob], "food-photo.jpg", { type: "image/jpeg" });
}

function resetFoodPhoto() {
  foodPhotoFile = null;
  if (photoPreviewUrl) {
    URL.revokeObjectURL(photoPreviewUrl);
    photoPreviewUrl = null;
  }
  const preview = document.querySelector("#photoPreview");
  const image = document.querySelector("#photoPreviewImage");
  const choices = document.querySelector("#photoChoices");
  if (preview) preview.hidden = true;
  if (image) image.removeAttribute("src");
  if (choices) choices.hidden = false;
  photoStatus("A photo helps NGOs recognize the food.");
}

async function selectFoodPhoto(input) {
  const file = input.files?.[0];
  input.value = "";
  if (!file) return;
  photoStatus("Preparing photo…");
  try {
    const prepared = await prepareFoodPhoto(file);
    foodPhotoFile = prepared;
    if (photoPreviewUrl) URL.revokeObjectURL(photoPreviewUrl);
    photoPreviewUrl = URL.createObjectURL(prepared);
    const preview = document.querySelector("#photoPreview");
    const image = document.querySelector("#photoPreviewImage");
    const choices = document.querySelector("#photoChoices");
    if (image) image.src = photoPreviewUrl;
    if (preview) preview.hidden = false;
    if (choices) choices.hidden = true;
    photoStatus(`Photo ready to upload (${formatBytes(prepared.size)}).`);
  } catch (error) {
    resetFoodPhoto();
    photoStatus(error.message || "That photo could not be used.", true);
  }
}

async function uploadFoodPhoto(file) {
  const formData = new FormData();
  formData.append("photo", file, file.name || "food-photo.jpg");
  const response = await fetch("/api/uploads/food-photo", {
    method: "POST",
    credentials: "same-origin",
    body: formData
  });
  let result = {};
  try {
    result = await response.json();
  } catch {
    throw new Error("The server returned an unreadable response while uploading the photo.");
  }
  if (response.status === 401) {
    window.location.replace("/login");
    throw new Error("Please sign in again.");
  }
  if (!response.ok) throw new Error(result.error || "The photo could not be uploaded.");
  if (!result.url) throw new Error("The photo upload did not return a file URL.");
  return result.url;
}

function setupLocationPicker() {
  const mapElement = document.querySelector("#foodMap");
  if (!mapElement) return;

  const coordinates = document.querySelector("#mapCoordinates");
  const clearButton = document.querySelector("#clearLocationButton");
  const searchInput = document.querySelector("#mapSearch");
  const searchButton = document.querySelector("#mapSearchButton");
  const locationButton = document.querySelector("#useLocationButton");

  if (!window.L) {
    mapElement.innerHTML = '<p class="map-fallback">The map could not load. You can still save the listing with the pickup address.</p>';
    if (searchButton) searchButton.disabled = true;
    if (locationButton) locationButton.disabled = true;
    return;
  }

  locationMap = L.map(mapElement, {
    center: [20, 0],
    zoom: 2,
    minZoom: 2,
    maxZoom: 19,
    worldCopyJump: true
  });
  L.tileLayer(mapTileUrl, { maxZoom: 19, attribution: mapAttribution }).addTo(locationMap);

  const updateCoordinateLabel = () => {
    if (!coordinates) return;
    coordinates.textContent = pickupPoint
      ? `${pickupPoint.lat.toFixed(6)}, ${pickupPoint.lng.toFixed(6)} · drag the pin to fine-tune`
      : "Tap the map or use your location to drop a pin.";
  };

  const placePin = (latitude, longitude, zoom = 16) => {
    if (
      !Number.isFinite(latitude) || !Number.isFinite(longitude)
      || Math.abs(latitude) > 90 || Math.abs(longitude) > 180
    ) {
      toast("That location is not valid.", true);
      return;
    }
    pickupPoint = { lat: latitude, lng: longitude };
    locationMap.setView([latitude, longitude], zoom);
    if (!locationMarker) {
      locationMarker = L.marker([latitude, longitude], { draggable: true }).addTo(locationMap);
      locationMarker.on("dragend", () => {
        const point = locationMarker.getLatLng();
        pickupPoint = { lat: point.lat, lng: point.lng };
        updateCoordinateLabel();
      });
    } else {
      locationMarker.setLatLng([latitude, longitude]);
    }
    updateCoordinateLabel();
    if (clearButton) clearButton.hidden = false;
  };

  locationMap.on("click", event => placePin(event.latlng.lat, event.latlng.lng, locationMap.getZoom()));

  clearButton?.addEventListener("click", () => {
    pickupPoint = null;
    if (locationMarker) {
      locationMarker.remove();
      locationMarker = null;
    }
    clearButton.hidden = true;
    updateCoordinateLabel();
  });

  const searchPlace = async () => {
    const query = searchInput?.value.trim() || "";
    if (query.length < 2) {
      toast("Enter a place to search.", true);
      searchInput?.focus();
      return;
    }
    if (searchButton) searchButton.disabled = true;
    try {
      const response = await fetch(
        `https://nominatim.openstreetmap.org/search?format=jsonv2&limit=1&q=${encodeURIComponent(query)}`,
        { headers: { Accept: "application/json" } }
      );
      if (!response.ok) throw new Error("Location search is unavailable right now.");
      const results = await response.json();
      if (!Array.isArray(results) || results.length === 0) {
        toast("No matching place found. Try a nearby city or landmark.", true);
        return;
      }
      const place = results[0];
      placePin(Number(place.lat), Number(place.lon), 15);
      const addressField = document.querySelector("#food-pickup");
      if (addressField && !addressField.value.trim() && place.display_name) {
        addressField.value = String(place.display_name).slice(0, 500);
      }
      toast("Pickup area set. Tap the map to fine-tune the exact spot.");
    } catch (error) {
      toast(error.message || "Could not search for that place.", true);
    } finally {
      if (searchButton) searchButton.disabled = false;
    }
  };

  searchButton?.addEventListener("click", searchPlace);
  searchInput?.addEventListener("keydown", event => {
    if (event.key === "Enter") {
      event.preventDefault();
      searchPlace();
    }
  });

  locationButton?.addEventListener("click", () => {
    if (!navigator.geolocation) {
      toast("This browser cannot access your location.", true);
      return;
    }
    const originalLabel = locationButton.textContent;
    locationButton.disabled = true;
    locationButton.textContent = "Locating…";
    navigator.geolocation.getCurrentPosition(
      position => {
        placePin(position.coords.latitude, position.coords.longitude, 17);
        locationButton.disabled = false;
        locationButton.textContent = originalLabel;
        toast("Location found. Tap the map to fine-tune the pin.");
      },
      () => {
        locationButton.disabled = false;
        locationButton.textContent = originalLabel;
        toast("Location access was blocked. Tap the map to choose the spot manually.", true);
      },
      { enableHighAccuracy: true, timeout: 10000, maximumAge: 60000 }
    );
  });
}

function setupFoodForm() {
  if (!document.querySelector("#foodForm")) return;
  for (const selector of ["#food-photo-camera", "#food-photo-file"]) {
    const input = document.querySelector(selector);
    input?.addEventListener("change", () => selectFoodPhoto(input));
  }
  document.querySelector("#removePhoto")?.addEventListener("click", resetFoodPhoto);
  setupLocationPicker();
}

function setupFoodMaps() {
  if (!window.L) return;
  document.querySelectorAll("[data-food-map]").forEach(element => {
    const latitude = Number(element.dataset.lat);
    const longitude = Number(element.dataset.lng);
    if (
      !Number.isFinite(latitude) || !Number.isFinite(longitude)
      || Math.abs(latitude) > 90 || Math.abs(longitude) > 180
    ) {
      return;
    }
    const map = L.map(element, {
      center: [latitude, longitude],
      zoom: 16,
      zoomControl: false,
      attributionControl: true,
      scrollWheelZoom: false,
      dragging: false,
      touchZoom: false,
      doubleClickZoom: false,
      boxZoom: false,
      keyboard: false
    });
    L.tileLayer(mapTileUrl, { maxZoom: 19, attribution: mapAttribution }).addTo(map);
    L.marker([latitude, longitude]).addTo(map);
    map.on("click", () => window.open(openStreetMapUrl(latitude, longitude), "_blank", "noopener"));
    foodMaps.push(map);
  });
}

function afterRender() {
  setupFoodForm();
  setupFoodMaps();
}

async function showOverview() {
  if (account.role === "admin") {
    const summary = await api("/api/admin/summary");
    const counts = summary.counts;
    return `${heading("Platform overview", "Monitor FoodBridge accounts, food listings, and order activity.")}
      <div class="stats-grid">${statCard("Accounts", counts.accounts, `${counts.hotels} Hotels · ${counts.ngos} NGOs`)}${statCard("Food listings", counts.foods, `${counts.open_foods} available`)}${statCard("Order requests", counts.orders, `${counts.pending_orders} awaiting response`)}</div>
      <section class="panel">${panelHeading("Recent orders")}<div class="record-list">${summary.orders.slice(0, 5).map(order => orderCard(order, false)).join("") || emptyState("No orders yet", "Orders will appear when NGOs request listed food.")}</div></section>`;
  }
  if (account.role === "hotel") {
    const [foodResult, orderResult] = await Promise.all([api("/api/foods"), api("/api/orders")]);
    const available = foodResult.foods.filter(food => food.status === "available");
    const pending = orderResult.orders.filter(order => order.status === "pending");
    return `${heading("Welcome, " + account.name, "Share surplus food and respond to NGO requests.")}
      <div class="stats-grid">${statCard("Your listings", foodResult.foods.length, `${available.length} currently available`)}${statCard("NGO requests", orderResult.orders.length, `${pending.length} awaiting your response`)}</div>
      <section class="panel">${panelHeading("Latest NGO requests")}<div class="record-list">${orderResult.orders.slice(0, 3).map(order => orderCard(order, true)).join("") || emptyState("No requests yet", "When an NGO requests your food, it will appear here.")}</div></section>`;
  }
  const [foodResult, orderResult] = await Promise.all([api("/api/foods"), api("/api/orders")]);
  const pending = orderResult.orders.filter(order => order.status === "pending");
  return `${heading("Welcome, " + account.name, "Browse food shared by Hotels and manage your requests.")}
    <div class="stats-grid">${statCard("Available food listings", foodResult.foods.length, "Shared by registered Hotels")}${statCard("Your requests", orderResult.orders.length, `${pending.length} awaiting a Hotel response`)}</div>
    <section class="panel">${panelHeading("Recently available food")}<div class="food-grid">${foodResult.foods.slice(0, 3).map(food => foodCard(food, true)).join("") || emptyState("No food listings yet", "New Hotel listings will appear here when available.")}</div></section>`;
}

function panelHeading(title) {
  return `<div class="panel-heading"><div><h2>${escapeHtml(title)}</h2></div></div>`;
}

async function showFoods() {
  const result = await api("/api/foods");
  if (account.role === "hotel") {
    return `${heading("My food listings", "Publish surplus food and see which listings are still available.", '<button class="btn btn-primary" data-scroll-to="foodForm">＋ Add food</button>')}
      ${donationForm()}
      <section class="panel listing-panel">${panelHeading("Your listings")}<div class="food-grid">${result.foods.map(food => foodCard(food, false)).join("") || emptyState("No listings yet", "Add your first food listing using the form above.")}</div></section>`;
  }
  return `${heading("Browse available food", "Request the amount your organization can collect.")}
    <div class="food-grid">${result.foods.map(food => foodCard(food, true)).join("") || `<section class="panel">${emptyState("No food currently available", "Check again later for new Hotel listings.")}</section>`}</div>`;
}

async function showOrders() {
  const result = await api("/api/orders");
  const canManage = account.role === "hotel";
  return `${heading(canManage ? "NGO requests" : "My food requests", canManage ? "Review requests from NGOs for your listings." : "Track your requests and responses from Hotels.")}
    <div class="record-list">${result.orders.map(order => orderCard(order, canManage)).join("") || `<section class="panel">${emptyState(canManage ? "No requests yet" : "No orders yet", canManage ? "NGO requests for your listings will appear here." : "Browse food listings and request what your organization needs.")}</section>`}</div>`;
}

async function showAdminPage(page) {
  const result = await api("/api/admin/summary");
  if (page === "accounts") {
    return `${heading("Accounts", "Registered Hotels, NGOs, and administrators.")}
      <section class="panel"><div class="table-wrap"><table class="data-table"><thead><tr><th>Organization</th><th>Email</th><th>Account type</th><th>Joined</th></tr></thead><tbody>${result.accounts.map(item => `<tr><td>${escapeHtml(item.name)}</td><td>${escapeHtml(item.email)}</td><td>${escapeHtml(item.role)}</td><td>${escapeHtml(item.created_at)}</td></tr>`).join("") || `<tr><td colspan="4">No accounts yet.</td></tr>`}</tbody></table></div></section>`;
  }
  if (page === "foods") {
    return `${heading("Food listings", "Monitor all food published on the platform.")}
      <div class="food-grid">${result.foods.map(food => foodCard(food, false)).join("") || `<section class="panel">${emptyState("No food listings yet", "Hotel listings will appear here.")}</section>`}</div>`;
  }
  return `${heading("All orders", "Monitor food requests between Hotels and NGOs.")}
    <div class="record-list">${result.orders.map(order => orderCard(order, false)).join("") || `<section class="panel">${emptyState("No orders yet", "NGO order requests will appear here.")}</section>`}</div>`;
}

async function showProfile() {
  return `${heading("My account", "Your FoodBridge account details.")}
    <section class="panel profile-cover"><div class="avatar">${escapeHtml(account.name.slice(0, 1).toUpperCase())}</div><h2>${escapeHtml(account.name)}</h2><p>${escapeHtml(account.email)}</p><span class="status">${escapeHtml(roleLabels[account.role])}</span></section>`;
}

async function render(page = activePage) {
  activePage = page;
  const currentRender = ++renderVersion;
  window.clearInterval(refreshTimer);
  cleanupInteractive();
  pageName.textContent = titleFor(page);
  renderNavigation();
  content.innerHTML = '<section class="panel empty-state"><p>Loading your FoodBridge account…</p></section>';
  try {
    let view;
    if (account.role === "admin" && page !== "overview") {
      view = await showAdminPage(page);
    } else if (page === "overview") {
      view = await showOverview();
    } else if (page === "foods") {
      view = await showFoods();
    } else if (page === "orders" && account.role !== "admin") {
      view = await showOrders();
    } else if (page === "profile" && account.role !== "admin") {
      view = await showProfile();
    } else {
      view = await showOverview();
    }
    if (currentRender !== renderVersion) return;
    content.innerHTML = view;
    afterRender();
  } catch (error) {
    if (currentRender !== renderVersion) return;
    content.innerHTML = `<section class="panel empty-state"><h3>We couldn't load this page</h3><p>${escapeHtml(error.message)}</p><button class="btn btn-outline" data-retry>Try again</button></section>`;
  }
  const refreshablePages = account.role === "admin"
    ? ["overview", "accounts", "foods", "orders"]
    : account.role === "ngo"
      ? ["overview", "foods", "orders"]
      : ["overview", "orders"];
  if (refreshablePages.includes(page)) {
    refreshTimer = window.setInterval(() => {
      if (!document.hidden && activePage === page) render(page);
    }, 30000);
  }
  sidebar.classList.remove("open");
}

document.addEventListener("click", async event => {
  const pageButton = event.target.closest("[data-page]");
  if (pageButton) {
    await render(pageButton.dataset.page);
    return;
  }
  const orderButton = event.target.closest("[data-order-status]");
  if (orderButton) {
    orderButton.disabled = true;
    try {
      await api(`/api/orders/${orderButton.dataset.orderId}`, {
        method: "PATCH",
        body: JSON.stringify({ status: orderButton.dataset.orderStatus })
      });
      toast(`Request ${orderButton.dataset.orderStatus}.`);
      await render(activePage);
    } catch (error) {
      toast(error.message, true);
      orderButton.disabled = false;
    }
    return;
  }
  if (event.target.closest("[data-retry]")) {
    if (!account) {
      await start();
      return;
    }
    await render(activePage);
    return;
  }
  const scrollButton = event.target.closest("[data-scroll-to]");
  if (scrollButton) document.getElementById(scrollButton.dataset.scrollTo)?.scrollIntoView({ behavior: "smooth" });
});

document.addEventListener("submit", async event => {
  const form = event.target;
  if (form.id === "foodForm") {
    event.preventDefault();
    const button = form.querySelector('button[type="submit"]');
    button.disabled = true;
    const fields = new FormData(form);
    const payload = {
      name: fields.get("name"),
      category: fields.get("category"),
      description: fields.get("description"),
      quantity: fields.get("quantity"),
      unit: fields.get("unit"),
      pickup_address: fields.get("pickup_address"),
      best_before: fields.get("best_before"),
      photo_url: "",
      latitude: pickupPoint ? pickupPoint.lat.toFixed(7) : "",
      longitude: pickupPoint ? pickupPoint.lng.toFixed(7) : ""
    };
    let photoUploaded = false;
    try {
      if (foodPhotoFile) {
        photoStatus("Uploading photo…");
        payload.photo_url = await uploadFoodPhoto(foodPhotoFile);
        photoUploaded = true;
        photoStatus("Photo uploaded. Publishing listing…");
      }
      await api("/api/foods", { method: "POST", body: JSON.stringify(payload) });
      toast("Food listing published. NGOs can now see it.");
      await render("foods");
    } catch (error) {
      if (foodPhotoFile && !photoUploaded) photoStatus(error.message, true);
      toast(error.message, true);
      button.disabled = false;
    }
  } else if (form.matches("[data-order-food]")) {
    event.preventDefault();
    const button = form.querySelector('button[type="submit"]');
    button.disabled = true;
    const fields = new FormData(form);
    try {
      await api(`/api/foods/${form.dataset.orderFood}/orders`, {
        method: "POST",
        body: JSON.stringify({ quantity: fields.get("quantity"), note: fields.get("note") })
      });
      toast("Request sent to the Hotel.");
      await render(activePage);
    } catch (error) {
      toast(error.message, true);
      button.disabled = false;
    }
  }
});

document.querySelector("#menuToggle").addEventListener("click", () => sidebar.classList.toggle("open"));
document.querySelector("#logoutButton").addEventListener("click", async () => {
  try {
    await api("/api/logout", { method: "POST" });
    window.location.replace("/");
  } catch (error) {
    toast(error.message, true);
  }
});

async function start() {
  try {
    const result = await api("/api/me");
    account = result.account;
    if (!roleNavigation[account.role]) {
      await api("/api/logout", { method: "POST" });
      window.location.replace("/login");
      return;
    }
    if (account.role === "admin") {
      document.title = "Admin panel | FoodBridge";
      document.querySelector(".brand small").textContent = "ADMINISTRATION";
    }
    document.querySelector("#userName").textContent = account.name;
    document.querySelector("#userRole").textContent = roleLabels[account.role];
    document.querySelector("#userAvatar").textContent = account.name.slice(0, 1).toUpperCase();
    document.querySelector("#topUserName").textContent = account.name;
    document.querySelector("#topUserRole").textContent = roleLabels[account.role];
    await render("overview");
  } catch (error) {
    if (error.status === 401) {
      window.location.replace("/login");
      return;
    }
    content.innerHTML = `<section class="panel empty-state"><h3>FoodBridge is temporarily unavailable</h3><p>${escapeHtml(error.message)}</p><button class="btn btn-outline" data-retry>Try again</button></section>`;
  }
}

start();
