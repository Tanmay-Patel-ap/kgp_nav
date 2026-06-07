// ── MAP INIT ──────────────────────────────────────────────────────
const MAP_CENTER = [22.3149, 87.3105];
const MAP_ZOOM   = 16;

const CAMPUS_BOUNDS = [
  [22.3000, 87.2850],
  [22.3300, 87.3300],
];

window.map = L.map('map', {
  maxBoundsViscosity: 1.0,
  minZoom: 15,
  maxZoom: 21,
  zoomControl: true,
}).setView(MAP_CENTER, MAP_ZOOM);

window.map.setMaxBounds(CAMPUS_BOUNDS);

// ── TILE LAYER ────────────────────────────────────────────────────
L.tileLayer(
  'https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png',
  { attribution: '© OpenStreetMap contributors © CARTO' }
).addTo(window.map);

// ── COORDS HUD ────────────────────────────────────────────────────
window.map.on('mousemove', (e) => {
  updateCoordsHud(e.latlng.lat, e.latlng.lng, window.map.getZoom());
});
window.map.on('zoomend', () => {
  const c = window.map.getCenter();
  updateCoordsHud(c.lat, c.lng, window.map.getZoom());
});

// ── CLOSE DROPDOWNS ON MAP CLICK ──────────────────────────────────
window.map.on('click', () => {
  document.getElementById('origin-dropdown').innerHTML = '';
  document.getElementById('dest-dropdown').innerHTML   = '';
});

// ── LOAD DATA + INIT EVERYTHING ───────────────────────────────────
loadRoads(window.map);
loadBoundary(window.map);

fetch('./public/data/outdoor/navigation/campus_buildings.json')
  .then(r => r.json())
  .then(buildingDatabase => {

    // Load building shapes (needs db for name matching)
    loadBuildingShapes(window.map, buildingDatabase);

    // Init search with full database
    initSearch(buildingDatabase);

    setStatus('READY');
  })
  .catch(err => {
    console.error('Failed to load building database', err);
    setStatus('DATA LOAD ERROR');
  });