// ── LAYER REGISTRY ───────────────────────────────────────────────
// buildingLayers: [{ name, nodeId, layer }]
const buildingLayers = [];

// ── STYLES ───────────────────────────────────────────────────────
const STYLES = {
  building: {
    default:  { color: '#00d4ff', weight: 1,   fillColor: '#00d4ff', fillOpacity: 0.08 },
    hover:    { color: '#00d4ff', weight: 1.5, fillColor: '#00d4ff', fillOpacity: 0.22 },
    selected: { color: '#00ff88', weight: 2,   fillColor: '#00ff88', fillOpacity: 0.35 },
    route:    { color: '#ffb347', weight: 2,   fillColor: '#ffb347', fillOpacity: 0.30 },
  },
  road: {
    tertiary:   { color: '#2a3f55', weight: 4, opacity: 1 },
    secondary:  { color: '#1e2d3d', weight: 3, opacity: 1 },
    residential:{ color: '#1a2a3a', weight: 2, opacity: 0.9 },
    service:    { color: '#141f2a', weight: 1, opacity: 0.8 },
    footway:    { color: '#111820', weight: 1, opacity: 0.6 },
    pedestrian: { color: '#1a2a3a', weight: 2, opacity: 0.9 },
    default:    { color: '#161f28', weight: 1, opacity: 0.7 },
  },
  boundary: { color: '#00ff88', weight: 2, fillColor: '#00ff88', fillOpacity: 0.02, dashArray: '6 4' },
  routeLine: { color: '#00d4ff', weight: 4, opacity: 0.9, dashArray: null },
};

// ── BUILDING LAYERS ──────────────────────────────────────────────
function loadBuildingShapes(map, buildingDatabase) {
  fetch('./public/data/outdoor/rendering/building_shapes.geojson')
    .then(r => r.json())
    .then(data => {

      L.geoJSON(data, {
        style: () => ({ ...STYLES.building.default }),

        onEachFeature(feature, layer) {
          const shapeName = feature.properties.name;

          // Find matching entry in buildings database (named ones)
          const dbEntry = (shapeName && shapeName !== 'nan')
            ? buildingDatabase.find(b => b.name === shapeName)
            : null;

          // Register named buildings for search highlight
          if (dbEntry) {
            buildingLayers.push({ name: dbEntry.name, nodeId: dbEntry.node_id, layer });
          }

          // Popup only for named shapes
          if (shapeName && shapeName !== 'nan') {
            layer.bindPopup(`<b>${shapeName}</b>`, { closeButton: false });
          }

          // Hover
          layer.on('mouseover', () => {
            if (!layer._isSelected && !layer._isRoute) {
              layer.setStyle({ ...STYLES.building.hover });
            }
          });
          layer.on('mouseout', () => {
            if (!layer._isSelected && !layer._isRoute) {
              layer.setStyle({ ...STYLES.building.default });
            }
          });
        },
      }).addTo(map);
    });
}

// ── ROAD LAYER ───────────────────────────────────────────────────
function loadRoads(map) {
  fetch('./public/data/outdoor/rendering/roads.geojson')
    .then(r => r.json())
    .then(data => {
      L.geoJSON(data, {
        style(feature) {
          const hw = feature.properties.highway;
          return STYLES.road[hw] || STYLES.road.default;
        },
      }).addTo(map);
    });
}

// ── CAMPUS BOUNDARY ──────────────────────────────────────────────
function loadBoundary(map) {
  fetch('./public/data/outdoor/rendering/campus_boundary.geojson')
    .then(r => r.json())
    .then(data => {
      L.geoJSON(data, { style: () => ({ ...STYLES.boundary }) }).addTo(map);
    });
}

// ── HIGHLIGHT HELPERS ─────────────────────────────────────────────
function resetAllBuildings() {
  buildingLayers.forEach(({ layer }) => {
    layer._isSelected = false;
    layer._isRoute = false;
    layer.setStyle({ ...STYLES.building.default });
    layer.closePopup();
  });
}

function highlightBuilding(name, styleKey = 'selected') {
  const entry = buildingLayers.find(b => b.name === name);
  if (!entry) return;
  entry.layer._isSelected = styleKey === 'selected';
  entry.layer._isRoute = styleKey === 'route';
  entry.layer.setStyle({ ...STYLES.building[styleKey] });
  entry.layer.openPopup();
}