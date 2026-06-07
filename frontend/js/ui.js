// ── ELEMENTS ──────────────────────────────────────────────────────
const sidebar       = document.getElementById('sidebar');
const routeDistance = document.getElementById('route-distance');
const routeTime     = document.getElementById('route-time');
const stepsList     = document.getElementById('steps-list');
const statusText    = document.getElementById('status-text');

// ── SIDEBAR SHOW/HIDE ─────────────────────────────────────────────
function showSidebar() { sidebar.classList.remove('hidden'); }
function hideSidebar()  { sidebar.classList.add('hidden'); }

// ── SIDEBAR CLOSE ─────────────────────────────────────────────────
document.getElementById('sidebar-close').addEventListener('click', () => {
  hideSidebar();
  clearRoute();
});

document.getElementById('clear-route-btn').addEventListener('click', () => {
  hideSidebar();
  clearRoute();
  document.getElementById('origin-input').value = '';
  document.getElementById('dest-input').value   = '';
  setStatus('READY');
});

// ── UPDATE SIDEBAR ────────────────────────────────────────────────
function updateSidebar(origin, dest, routeData) {
  const meters = routeData.distance;
  const km     = (meters / 1000).toFixed(2);
  const mins   = Math.ceil(meters / 80); // ~80 m/min walking

  routeDistance.innerHTML = `${km} <span class="unit">km</span>`;
  routeTime.innerHTML     = `${mins} <span class="unit">min</span>`;

  // Steps: origin → (via waypoints if any) → dest
  stepsList.innerHTML = '';

  const steps = buildSteps(origin, dest, routeData);
  steps.forEach((s, i) => {
    const div = document.createElement('div');
    div.className = 'step-item';
    div.innerHTML = `
      <span class="step-index">${String(i + 1).padStart(2, '0')}</span>
      <span class="step-name">${s}</span>
    `;
    stepsList.appendChild(div);
  });
}

function buildSteps(origin, dest, routeData) {
  // Basic steps for Stage 1 — origin and destination only
  // Stage 2/3 will inject intermediate steps (building entrances, floor changes)
  return [
    `Start at <b>${origin.name}</b>`,
    `Head towards destination`,
    `Arrive at <b>${dest.name}</b>`,
  ];
}

// ── STATUS BAR ────────────────────────────────────────────────────
function setStatus(msg) {
  statusText.textContent = msg;
}

// ── NAVIGATE BUTTON ───────────────────────────────────────────────
document.getElementById('route-btn').addEventListener('click', () => {
  const state = getSearchState();
  const origin = state.origin.building;
  const dest   = state.dest.building;

  if (!origin) { setStatus('SELECT ORIGIN'); return; }
  if (!dest)   { setStatus('SELECT DESTINATION'); return; }
  if (origin.node_id === dest.node_id) { setStatus('SAME LOCATION'); return; }

  fetchAndDrawRoute(origin, dest);
});

// ── COORDS HUD ─────────────────────────────────────────────────── 
// updated from map.js after map init
function updateCoordsHud(lat, lon, zoom) {
  document.getElementById('coords-text').textContent =
    `${lat.toFixed(4)}° N  ${lon.toFixed(4)}° E`;
  document.getElementById('zoom-text').textContent =
    `Z:${zoom}`;
}