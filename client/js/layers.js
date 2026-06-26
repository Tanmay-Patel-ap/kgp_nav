// ── LAYER REGISTRY ───────────────────────────────────────────────
const buildingLayers = [];

// ── STYLE HELPERS (reads current theme) ──────────────────────────
function isDarkMode() {
  return document.documentElement.getAttribute('data-theme') !== 'light';
}

function getBuildingStyle(type = 'default') {
  const dark = isDarkMode();
  const styles = {
    default:  dark
      ? { color:'#2a4a6b', weight:1,   fillColor:'#1a3550', fillOpacity:0.55 }
      : { color:'#90a8c0', weight:1,   fillColor:'#c8d8e8', fillOpacity:0.55 },
    hover:    dark
      ? { color:'#4f8ef7', weight:1.5, fillColor:'#1e3a60', fillOpacity:0.75 }
      : { color:'#2563eb', weight:1.5, fillColor:'#bfcfe8', fillOpacity:0.75 },
    selected: dark
      ? { color:'#4f8ef7', weight:2,   fillColor:'#1d3a70', fillOpacity:0.85 }
      : { color:'#1d4ed8', weight:2,   fillColor:'#bfdbfe', fillOpacity:0.85 },
    route:    dark
      ? { color:'#f59e0b', weight:2,   fillColor:'#78350f', fillOpacity:0.7 }
      : { color:'#d97706', weight:2,   fillColor:'#fde68a', fillOpacity:0.7 },
  };
  return styles[type];
}

function getRoadStyle(hw) {
  const dark = isDarkMode();
  const styles = dark ? {
    tertiary:    { color:'#2a3f58', weight:4,   opacity:1 },
    secondary:   { color:'#223350', weight:3,   opacity:1 },
    residential: { color:'#1e2d45', weight:2,   opacity:0.9 },
    service:     { color:'#182540', weight:1.5, opacity:0.8 },
    footway:     { color:'#141e33', weight:1,   opacity:0.6 },
    pedestrian:  { color:'#1e2d45', weight:2,   opacity:0.9 },
    default:     { color:'#181e2e', weight:1,   opacity:0.7 },
  } : {
    tertiary:    { color:'#a8b8c8', weight:4,   opacity:1 },
    secondary:   { color:'#b8c8d8', weight:3,   opacity:1 },
    residential: { color:'#c8d4de', weight:2,   opacity:0.9 },
    service:     { color:'#d0dae4', weight:1.5, opacity:0.8 },
    footway:     { color:'#d8e0e8', weight:1,   opacity:0.6 },
    pedestrian:  { color:'#c8d4de', weight:2,   opacity:0.9 },
    default:     { color:'#ccd4dc', weight:1,   opacity:0.7 },
  };
  return styles[hw] || styles.default;
}

// ── LOAD BUILDINGS ────────────────────────────────────────────────
function loadBuildingShapes(map, buildingDatabase) {
  fetch('./data/outdoor/building_shapes.geojson')
    .then(r => {
      if (!r.ok) throw new Error(`HTTP ${r.status} loading buildings`);
      return r.json();
    })
    .then(data => {
      if (!data || !data.features) throw new Error('Invalid GeoJSON structure');
      L.geoJSON(data, {
        style: () => getBuildingStyle('default'),
        onEachFeature(feature, layer) {
          const shapeName = feature.properties && feature.properties.name;
          const dbEntry = (shapeName && shapeName !== 'nan')
            ? buildingDatabase.find(b => b.name === shapeName)
            : null;

          if (dbEntry) buildingLayers.push({ name: dbEntry.name, id: dbEntry.id, layer });

          if (shapeName && shapeName !== 'nan') {
            layer.bindPopup(`<b>${shapeName}</b>`, { closeButton: false });
          }

          layer.on('mouseover', () => {
            if (!layer._isSelected && !layer._isRoute)
              layer.setStyle(getBuildingStyle('hover'));
          });
          layer.on('mouseout', () => {
            if (!layer._isSelected && !layer._isRoute)
              layer.setStyle(getBuildingStyle('default'));
          });
        },
      }).addTo(map);
    })
    .catch(err => console.error('[layers] Building shapes load error:', err.message));
}

// ── LOAD ROADS ────────────────────────────────────────────────────
function loadRoads(map) {
  fetch('./data/outdoor/roads.geojson')
    .then(r => {
      if (!r.ok) throw new Error(`HTTP ${r.status} loading roads`);
      return r.json();
    })
    .then(data => {
      if (!data || !data.features) return;
      L.geoJSON(data, {
        style: f => getRoadStyle((f.properties || {}).highway),
      }).addTo(map);
    })
    .catch(err => console.error('[layers] Roads load error:', err.message));
}

// ── LOAD BOUNDARY ─────────────────────────────────────────────────
function loadBoundary(map) {
  fetch('./data/outdoor/campus_boundary.geojson')
    .then(r => {
      if (!r.ok) throw new Error(`HTTP ${r.status} loading boundary`);
      return r.json();
    })
    .then(data => {
      if (!data || !data.features) return;
      const dark = isDarkMode();
      L.geoJSON(data, {
        style: () => ({
          color:       dark ? '#34d399' : '#059669',
          weight:      1.5,
          fillColor:   dark ? '#34d399' : '#059669',
          fillOpacity: 0.02,
          dashArray:   '6 5',
        }),
      }).addTo(map);
    })
    .catch(err => console.error('[layers] Boundary load error:', err.message));
}

// ── HIGHLIGHT HELPERS ─────────────────────────────────────────────
function resetAllBuildings() {
  buildingLayers.forEach(({ layer }) => {
    try {
      layer._isSelected = false;
      layer._isRoute    = false;
      layer.setStyle(getBuildingStyle('default'));
      layer.closePopup();
    } catch (_) {}
  });
}

function highlightBuilding(name, type = 'selected') {
  const entry = buildingLayers.find(b => b.name === name);
  if (!entry) return;
  try {
    entry.layer._isSelected = type === 'selected';
    entry.layer._isRoute    = type === 'route';
    entry.layer.setStyle(getBuildingStyle(type));
    entry.layer.openPopup();
  } catch (_) {}
}