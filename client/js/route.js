// ── STATE ─────────────────────────────────────────────────────────
let routeLines = [];

// ── DRAW ROUTE ────────────────────────────────────────────────────
async function fetchAndDrawRoute(originBuilding, destBuilding, originRoom, destRoom) {
  clearRoute();
  setStatus('ROUTING...');

  try {
    // Decide which endpoint to call
    const hasIndoor = (originRoom || destRoom);
    const data = hasIndoor
      ? await API.getCombinedRoute(originBuilding.id, originRoom || '', destBuilding.id, destRoom || '')
      : await API.getRoute(originBuilding.id, destBuilding.id);

    if (!data || !data.path || data.path.length === 0) {
      setStatus('NO ROUTE FOUND');
      return;
    }
    const path = data.path;

    drawRouteLine(path);
    showSidebar();

    // Highlight origin + dest buildings
    resetAllBuildings();
    highlightBuilding(originBuilding.name, 'route');
    highlightBuilding(destBuilding.name,   'route');

    // Fit map to route (outdoor path only)
    const bounds = path.map(p => [p.lat, p.lon]);
    window.map.fitBounds(bounds, { padding: [60, 60] });

    // Show combined steps if available, else use traditional format
    if (data.steps) {
      updateSidebarCombined(originBuilding, destBuilding, data);
    } else {
      updateSidebar(originBuilding, destBuilding, data);
    }

    // Collapse search section
    const oc = document.getElementById('outdoor-content');
    if (oc) oc.classList.add('search-collapsed');

    setStatus('ROUTE ACTIVE');

  } catch (err) {
    console.error(err);
    setStatus('ERROR — CHECK SERVER');
  }
}

// ── POLYLINE ──────────────────────────────────────────────────────
function drawRouteLine(path) {
  if (!path || !Array.isArray(path) || path.length < 2 || !window.map) return;

  const latlngs = [];
  for (const p of path) {
    if (p && typeof p.lat === 'number' && typeof p.lon === 'number') {
      latlngs.push([p.lat, p.lon]);
    }
  }
  if (latlngs.length < 2) { console.warn('[route] Too few valid coords'); return; }

  routeLines.push(L.polyline(latlngs, {
    color:   '#00d4ff',
    weight:  4,
    opacity: 0.9,
    lineJoin: 'round',
    lineCap:  'round',
  }).addTo(window.map));

  routeLines.push(L.polyline(latlngs, {
    color:     '#00d4ff',
    weight:    2,
    opacity:   0.5,
    dashArray: '8 8',
  }).addTo(window.map));
}

// ── CLEAR ROUTE ───────────────────────────────────────────────────
function clearRoute() {
  if (window.map) {
    routeLines.forEach(line => { try { window.map.removeLayer(line); } catch (_) {} });
    routeLines = [];
  }
  hideSidebar();
  resetAllBuildings();
}