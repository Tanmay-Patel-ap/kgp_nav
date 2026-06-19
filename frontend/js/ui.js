// ── ELEMENTS ──────────────────────────────────────────────────────
const routeResult   = document.getElementById('route-result');
const routeDistance = document.getElementById('route-distance');
const routeTime     = document.getElementById('route-time');
const stepsList     = document.getElementById('steps-list');
const statusText    = document.getElementById('status-text');

// ── ROUTE RESULT SHOW/HIDE ────────────────────────────────────────
function showSidebar()  { routeResult.classList.remove('hidden'); }
function hideSidebar()  { routeResult.classList.add('hidden'); }

// ── CLOSE / CLEAR ─────────────────────────────────────────────────
document.getElementById('clear-route-btn').addEventListener('click', () => {
  hideSidebar();
  clearRoute();
  document.getElementById('origin-input').value = '';
  document.getElementById('dest-input').value   = '';
  setStatus('Ready');
});

// ── UPDATE ROUTE INFO ─────────────────────────────────────────────
function updateSidebar(origin, dest, routeData) {
  const m    = routeData.distance;
  const km   = (m / 1000).toFixed(2);
  const mins = Math.ceil(m / 80);

  routeDistance.innerHTML = `${km}<span class="unit"> km</span>`;
  routeTime.innerHTML     = `${mins}<span class="unit"> min</span>`;

  stepsList.innerHTML = '';
  const steps = [
    `Start at <b>${origin.name}</b>`,
    `Follow the highlighted path`,
    `Arrive at <b>${dest.name}</b>`,
  ];
  steps.forEach((s, i) => {
    const div = document.createElement('div');
    div.className = 'step-item';
    div.innerHTML = `<span class="step-num">0${i+1}</span><span>${s}</span>`;
    stepsList.appendChild(div);
  });
}

// ── STATUS ────────────────────────────────────────────────────────
function setStatus(msg) { statusText.textContent = msg; }

// ── NAVIGATE ──────────────────────────────────────────────────────
document.getElementById('route-btn').addEventListener('click', () => {
  const state  = getSearchState();
  const origin = state.origin.building;
  const dest   = state.dest.building;
  if (!origin) { setStatus('Select origin'); return; }
  if (!dest)   { setStatus('Select destination'); return; }
  if (origin.id === dest.id) { setStatus('Same location'); return; }
  fetchAndDrawRoute(origin, dest);
});

// ── THEME TOGGLE ──────────────────────────────────────────────────
function toggleTheme() {
  const html    = document.documentElement;
  const isDark  = html.getAttribute('data-theme') === 'dark';
  const newTheme = isDark ? 'light' : 'dark';
  html.setAttribute('data-theme', newTheme);
  localStorage.setItem('theme', newTheme);

  document.getElementById('theme-icon-moon').style.display = isDark ? 'none' : 'flex';
  document.getElementById('theme-icon-sun').style.display  = isDark ? 'flex' : 'none';

  // swap tile layer
  if (window.tileLayer) window.map.removeLayer(window.tileLayer);
  window.tileLayer = L.tileLayer(
    isDark
      ? 'https://{s}.basemaps.cartocdn.com/light_all/{z}/{x}/{y}{r}.png'
      : 'https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png',
    { attribution: '© OpenStreetMap © CARTO' }
  ).addTo(window.map);
}

// ── PANEL COLLAPSE ────────────────────────────────────────────────
function togglePanel() {
  const panel = document.getElementById('left-panel');
  const btn   = document.getElementById('collapse-btn');
  const expandBtn = document.getElementById('expand-btn');
  const collapsed = panel.classList.toggle('collapsed');

  btn.innerHTML = collapsed
    ? '<svg width="14" height="14" viewBox="0 0 14 14" fill="none"><path d="M5 2L10 7L5 12" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/></svg>'
    : '<svg width="14" height="14" viewBox="0 0 14 14" fill="none"><path d="M9 2L4 7L9 12" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/></svg>';
  btn.title = collapsed ? 'Expand sidebar' : 'Collapse sidebar';

  expandBtn.style.display = collapsed ? 'flex' : 'none';

  setTimeout(() => { window.map.invalidateSize(); }, 350);
}

// ── RESTORE THEME ON LOAD ─────────────────────────────────────────
(function() {
  const saved = localStorage.getItem('theme') || 'dark';
  document.documentElement.setAttribute('data-theme', saved);
  if (saved === 'light') {
    document.getElementById('theme-icon-moon').style.display = 'none';
    document.getElementById('theme-icon-sun').style.display  = 'flex';
  }
})();