// ── STATE ─────────────────────────────────────────────────────────
let activeRouteLine = null;

// ── DRAW ROUTE ────────────────────────────────────────────────────
async function fetchAndDrawRoute(originBuilding, destBuilding) {
  clearRoute();
  setStatus('ROUTING...');

  try {
    const data = await API.getRoute(
      originBuilding.id,
      destBuilding.id
    );

    // data.path = [{ lat, lon }, ...]
    // data.distance = meters (number)

    if (!data.path || data.path.length === 0) {
      setStatus('NO ROUTE FOUND');
      return;
    }

    drawRouteLine(data.path);
    updateSidebar(originBuilding, destBuilding, data);
    showSidebar();

    // Highlight origin + dest buildings
    resetAllBuildings();
    highlightBuilding(originBuilding.name, 'route');
    highlightBuilding(destBuilding.name,   'route');

    // Fit map to route
    const bounds = data.path.map(p => [p.lat, p.lon]);
    window.map.fitBounds(bounds, { padding: [60, 60] });

    setStatus('ROUTE ACTIVE');

  } catch (err) {
    console.error(err);
    setStatus('ERROR — CHECK BACKEND');
  }
}

// ── POLYLINE ──────────────────────────────────────────────────────
function drawRouteLine(path) {
  const latlngs = path.map(p => [p.lat, p.lon]);

  activeRouteLine = L.polyline(latlngs, {
    color:   '#00d4ff',
    weight:  4,
    opacity: 0.9,
    lineJoin: 'round',
    lineCap:  'round',
  }).addTo(window.map);

  // Animated dashed overlay for the "moving" feel
  L.polyline(latlngs, {
    color:     '#00d4ff',
    weight:    2,
    opacity:   0.5,
    dashArray: '8 8',
  }).addTo(window.map);
}

// ── CLEAR ROUTE ───────────────────────────────────────────────────
function clearRoute() {
  if (activeRouteLine) {
    window.map.eachLayer(layer => {
      if (layer instanceof L.Polyline) window.map.removeLayer(layer);
    });
    activeRouteLine = null;
  }
  hideSidebar();
  resetAllBuildings();
}