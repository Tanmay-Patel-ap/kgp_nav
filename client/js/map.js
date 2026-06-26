// ── MAP INIT ──────────────────────────────────────────────────────
const MAP_CENTER    = [22.3149, 87.3105];
const MAP_ZOOM      = 16;
const CAMPUS_BOUNDS = [[22.3000, 87.2850],[22.3300, 87.3300]];

const mapEl = document.getElementById('map');
if (!mapEl) {
  console.error('[map] #map element not found');
} else {
  (window.map = L.map('map', {
    maxBoundsViscosity: 1.0,
    minZoom: 15,
    maxZoom: 21,
    zoomControl: false,
  })).setView(MAP_CENTER, MAP_ZOOM);

  window.map.setMaxBounds(CAMPUS_BOUNDS);

  L.control.scale({
    position: 'bottomright',
    metric: true,
    imperial: false,
    maxWidth: 200,
  }).addTo(window.map);

  L.control.zoom({ position: 'bottomright' }).addTo(window.map);

  const isDark = document.documentElement.getAttribute('data-theme') !== 'light';
  window.tileLayer = L.tileLayer(
    isDark
      ? 'https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png'
      : 'https://{s}.basemaps.cartocdn.com/light_all/{z}/{x}/{y}{r}.png',
    { attribution: '© OpenStreetMap © CARTO' }
  ).addTo(window.map);

  window.map.on('click', () => {
    const od = document.getElementById('origin-dropdown');
    const dd = document.getElementById('dest-dropdown');
    if (od) od.innerHTML = '';
    if (dd) dd.innerHTML = '';
  });

  loadRoads(window.map);
  loadBoundary(window.map);

  Promise.all([
    API.getBuildings(),
    API.getIndoorBuildings().catch(() => []),
  ])
    .then(([db, indoorList]) => {
      if (!Array.isArray(db)) throw new Error('Invalid buildings data format');
      loadBuildingShapes(window.map, db);
      initSearch(db, indoorList);
      setStatus('Ready');
    })
    .catch(err => {
      console.error('[map]', err.message);
      setStatus('Data load error — ' + err.message);
    });
}