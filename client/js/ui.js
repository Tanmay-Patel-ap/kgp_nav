// ── ELEMENTS ──────────────────────────────────────────────────────
const routeResult   = document.getElementById('route-result');
const routeDistance = document.getElementById('route-distance');
const routeTime     = document.getElementById('route-time');
const stepsList     = document.getElementById('steps-list');
const statusText    = document.getElementById('status-text');

// ── ROUTE RESULT SHOW/HIDE ────────────────────────────────────────
function showSidebar()  { if (routeResult) routeResult.classList.remove('hidden'); }
function hideSidebar()  {
  if (routeResult) routeResult.classList.add('hidden');
  const oc = document.getElementById('outdoor-content');
  if (oc) oc.classList.remove('search-collapsed');
}

// ── SEARCH EXPAND BUTTON ──────────────────────────────────────────
const searchExpandBtn = document.getElementById('search-expand-btn');
if (searchExpandBtn) searchExpandBtn.addEventListener('click', () => {
  const oc = document.getElementById('outdoor-content');
  if (oc) oc.classList.remove('search-collapsed');
});

// ── CLOSE / CLEAR ─────────────────────────────────────────────────
const clearBtn = document.getElementById('clear-route-btn');
if (clearBtn) clearBtn.addEventListener('click', () => {
  hideSidebar();
  clearRoute();
  const oi = document.getElementById('origin-input'); if (oi) oi.value = '';
  const di = document.getElementById('dest-input');   if (di) di.value = '';
  const ori = document.getElementById('origin-room-input');
  const dri = document.getElementById('dest-room-input');
  if (ori) { ori.value = ''; searchState.origin.room = null; }
  if (dri) { dri.value = ''; searchState.dest.room = null; }
  setStatus('Ready');
});

// ── UPDATE ROUTE INFO ─────────────────────────────────────────────
function updateSidebar(origin, dest, routeData) {
  if (!routeDistance || !routeTime || !stepsList) return;
  const m    = routeData && routeData.distance != null ? routeData.distance : 0;
  const km   = (m / 1000).toFixed(2);
  const mins = Math.ceil(m / 80);

  routeDistance.innerHTML = `${km}<span class="unit"> km</span>`;
  routeTime.innerHTML     = `${mins}<span class="unit"> min</span>`;

  stepsList.innerHTML = '';
  const steps = [
    `Start at <b>${origin ? origin.name : '?'}</b>`,
    `Follow the highlighted path`,
    `Arrive at <b>${dest ? dest.name : '?'}</b>`,
  ];
  steps.forEach((s, i) => {
    const div = document.createElement('div');
    div.className = 'step-item';
    div.innerHTML = `<span class="step-num">0${i+1}</span><span>${s}</span>`;
    stepsList.appendChild(div);
  });
}

// ── COMBINED ROUTE SIDEBAR ────────────────────────────────────────
function updateSidebarCombined(origin, dest, routeData) {
  if (!routeDistance || !routeTime || !stepsList) return;
  const total = routeData && routeData.distance != null ? routeData.distance : 0;
  const km    = (total / 1000).toFixed(2);
  const mins  = Math.ceil(total / 80);
  routeDistance.innerHTML = `${km}<span class="unit"> km</span>`;
  routeTime.innerHTML     = `${mins}<span class="unit"> min</span>`;

  const steps = routeData.steps || [];
  const exitIdx = steps.findIndex(s => (s.text || '').startsWith('Exit'));
  const enterIdx = steps.findIndex(s => (s.text || '').startsWith('Enter'));

  stepsList.innerHTML = '';

  function renderSection(header, start, end) {
    if (start >= end) return;
    const hdr = document.createElement('div');
    hdr.className = 'section-divider';
    hdr.innerHTML = escapeHtml(header);
    stepsList.appendChild(hdr);
    for (let i = start; i < end; i++) {
      const s = steps[i];
      const div = document.createElement('div');
      div.className = 'step-item';
      let icon = '·';
      const t = (s.text || '');
      if (t.startsWith('Start'))       icon = '●';
      else if (t.startsWith('Walk'))   icon = '↑';
      else if (t.startsWith('Take') || t.startsWith('Turn')) icon = t.includes('right') ? '→' : '←';
      else if (t.startsWith('Exit'))   icon = '◁';
      else if (t.startsWith('Enter'))  icon = '▷';
      else if (t.startsWith('Arrive')) icon = '★';
      else if (/is (ahead|on your)/.test(t)) icon = '★';
      div.innerHTML = '<span class="step-num">' + icon + '</span><span>' + escapeHtml(t) + '</span>';
      stepsList.appendChild(div);
    }
  }

  if (exitIdx === -1 && enterIdx === -1) {
    // Plain outdoor route
    renderSection('in campus', 0, steps.length);
  } else {
    const originName = origin ? origin.name : 'building';
    const destName   = dest   ? dest.name   : 'building';

    if (exitIdx > 0) {
      renderSection('inside ' + originName, 0, exitIdx);
    }
    // "Exit XYZ" step itself skipped; start from exitIdx+1
    const campusStart = exitIdx >= 0 ? exitIdx + 1 : 0;
    const campusEnd   = enterIdx >= 0 ? enterIdx : steps.length;
    if (campusStart < campusEnd) {
      renderSection('in campus', campusStart, campusEnd);
    }
    if (enterIdx >= 0 && enterIdx + 1 < steps.length) {
      renderSection('inside ' + destName, enterIdx + 1, steps.length);
    }
  }
}

// ── STATUS ────────────────────────────────────────────────────────
function setStatus(msg) { if (statusText) statusText.textContent = msg; }

// ── NAVIGATE ──────────────────────────────────────────────────────
const routeBtn = document.getElementById('route-btn');
if (routeBtn) routeBtn.addEventListener('click', () => {
  const state     = getSearchState();
  const origin    = state.origin.building;
  const dest      = state.dest.building;
  const origRoom  = state.origin.room;
  const destRoom  = state.dest.room;
  if (!origin) { setStatus('Select origin'); return; }
  if (!dest)   { setStatus('Select destination'); return; }
  if (origin.id === dest.id && !origRoom && !destRoom) { setStatus('Same location'); return; }
  fetchAndDrawRoute(origin, dest, origRoom, destRoom);
});

// ── THEME TOGGLE ──────────────────────────────────────────────────
function toggleTheme() {
  const html    = document.documentElement;
  const isDark  = html.getAttribute('data-theme') === 'dark';
  const newTheme = isDark ? 'light' : 'dark';
  html.setAttribute('data-theme', newTheme);
  try { localStorage.setItem('theme', newTheme); } catch (_) {}

  const moon = document.getElementById('theme-icon-moon');
  const sun  = document.getElementById('theme-icon-sun');
  if (moon) moon.style.display = isDark ? 'none' : 'flex';
  if (sun)  sun.style.display  = isDark ? 'flex' : 'none';

  if (window.map && window.tileLayer) window.map.removeLayer(window.tileLayer);
  window.tileLayer = L.tileLayer(
    isDark
      ? 'https://{s}.basemaps.cartocdn.com/light_all/{z}/{x}/{y}{r}.png'
      : 'https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png',
    { attribution: '© OpenStreetMap © CARTO' }
  );
  if (window.map && window.tileLayer) window.tileLayer.addTo(window.map);
}

// ── PANEL COLLAPSE ────────────────────────────────────────────────
function togglePanel() {
  const panel = document.getElementById('left-panel');
  const btn   = document.getElementById('collapse-btn');
  const expandBtn = document.getElementById('expand-btn');
  if (!panel || !btn || !expandBtn) return;
  const collapsed = panel.classList.toggle('collapsed');

  btn.innerHTML = collapsed
    ? '<svg width="14" height="14" viewBox="0 0 14 14" fill="none"><path d="M5 2L10 7L5 12" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/></svg>'
    : '<svg width="14" height="14" viewBox="0 0 14 14" fill="none"><path d="M9 2L4 7L9 12" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/></svg>';
  btn.title = collapsed ? 'Expand sidebar' : 'Collapse sidebar';

  expandBtn.style.display = collapsed ? 'flex' : 'none';

  setTimeout(() => { if (window.map) window.map.invalidateSize(); }, 350);
}

// ── ESCAPE HTML ───────────────────────────────────────────────────
function escapeHtml(s) {
  if (typeof s !== 'string') return '';
  return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
}

// ── RESTORE THEME ON LOAD ─────────────────────────────────────────
(function() {
  let saved = 'dark';
  try { saved = localStorage.getItem('theme') || 'dark'; } catch (_) {}
  document.documentElement.setAttribute('data-theme', saved);
  if (saved === 'light') {
    const moon = document.getElementById('theme-icon-moon');
    const sun  = document.getElementById('theme-icon-sun');
    if (moon) moon.style.display = 'none';
    if (sun)  sun.style.display  = 'flex';
  }
})();