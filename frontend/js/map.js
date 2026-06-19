// ── MAP INIT ──────────────────────────────────────────────────────
const MAP_CENTER    = [22.3149, 87.3105];
const MAP_ZOOM      = 16;
const CAMPUS_BOUNDS = [[22.3000, 87.2850],[22.3300, 87.3300]];

window.map = L.map('map', {
  maxBoundsViscosity: 1.0,
  minZoom: 15,
  maxZoom: 21,
  zoomControl: false,
}).setView(MAP_CENTER, MAP_ZOOM);

window.map.setMaxBounds(CAMPUS_BOUNDS);
// ── SCALE BAR (bottom right) ──────────────────────────────────────
L.control.scale({
  position: 'bottomright',
  metric: true,
  imperial: false,
  maxWidth: 200,
}).addTo(window.map);

// ── ZOOM CONTROL (bottom right) ──────────────────────────────────
L.control.zoom({ position: 'bottomright' }).addTo(window.map);



// ── TILE LAYER (respects saved theme) ────────────────────────────
const isDark = document.documentElement.getAttribute('data-theme') !== 'light';
window.tileLayer = L.tileLayer(
  isDark
    ? 'https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png'
    : 'https://{s}.basemaps.cartocdn.com/light_all/{z}/{x}/{y}{r}.png',
  { attribution: '© OpenStreetMap © CARTO' }
).addTo(window.map);

// ── CLOSE DROPDOWNS ON MAP CLICK ──────────────────────────────────
window.map.on('click', () => {
  document.getElementById('origin-dropdown').innerHTML = '';
  document.getElementById('dest-dropdown').innerHTML   = '';
});

// ── LOAD DATA ─────────────────────────────────────────────────────
loadRoads(window.map);
loadBoundary(window.map);

fetch('./public/data/outdoor/navigation/campus_buildings.json')
  .then(r => r.json())
  .then(db => {
    loadBuildingShapes(window.map, db);
    initSearch(db);
    setStatus('Ready');
  })
  .catch(err => {
    console.error(err);
    setStatus('Data load error');
  });