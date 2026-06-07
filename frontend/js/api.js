// ── API CONFIG ───────────────────────────────────────────────────
const API_BASE = 'http://127.0.0.1:8080';

// ── FETCH WRAPPER ────────────────────────────────────────────────
async function apiFetch(path) {
  const res = await fetch(API_BASE + path);
  if (!res.ok) throw new Error(`API error: ${res.status}`);
  return res.json();
}

// ── ENDPOINTS ────────────────────────────────────────────────────
const API = {

  // GET /buildings → full building list
  getBuildings: () =>
    apiFetch('/buildings'),

  // GET /search?q=... → filtered buildings
  searchBuildings: (query) =>
    apiFetch(`/search?q=${encodeURIComponent(query)}`),

  // GET /route?from=BUILDING_ID&to=BUILDING_ID → { path, distance }
  getRoute: (fromBuildingId, toBuildingId) =>
    apiFetch(`/route?from=${fromBuildingId}&to=${toBuildingId}`),
};