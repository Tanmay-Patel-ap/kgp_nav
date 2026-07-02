// ── API CONFIG ───────────────────────────────────────────────────
const API_BASE = 'http://127.0.0.1:8080';

// ── FETCH WRAPPER ────────────────────────────────────────────────
async function apiFetch(path) {
  let res;
  const headers = {};
  const token = localStorage.getItem('kgp_auth_token');
  if (token) headers['Authorization'] = 'Bearer ' + token;
  try {
    res = await fetch(API_BASE + path, { headers });
  } catch (e) {
    throw new Error(`Network error — is the server running? (${e.message})`);
  }
  if (!res.ok) {
    let body = '';
    try { body = await res.text(); } catch (_) {}
    throw new Error(`API error ${res.status}: ${body || res.statusText}`);
  }
  let data;
  try {
    data = await res.json();
  } catch (e) {
    throw new Error(`Invalid JSON response: ${e.message}`);
  }
  if (data && data.error) throw new Error(data.error);
  return data;
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

  // GET /indoor → list of available indoor buildings
  getIndoorBuildings: async () => {
    try { return await apiFetch('/indoor');
    } catch (e) { throw new Error('Indoor buildings: ' + e.message); }
  },

  // GET /indoor/graph?b=PREFIX → full graph with nodes (for room dropdowns)
  getIndoorGraph: async (buildingPrefix) => {
    try { return await apiFetch(`/indoor/graph?b=${buildingPrefix}`);
    } catch (e) { throw new Error('Indoor graph: ' + e.message); }
  },

  // GET /indoor/navigate?b=PREFIX&from=NODE_ID&to=NODE_ID → { found, distance, steps, path }
  getIndoorNavigation: async (buildingPrefix, fromNodeId, toNodeId) => {
    try { return await apiFetch(`/indoor/navigate?b=${buildingPrefix}&from=${fromNodeId}&to=${toNodeId}`);
    } catch (e) { throw new Error('Indoor navigate: ' + e.message); }
  },

  // GET /combined-route?from_bld=ID&from_room=NODE_ID&to_bld=ID&to_room=NODE_ID
  getCombinedRoute: async (fromBldId, fromRoom, toBldId, toRoom) => {
    const params = `from_bld=${fromBldId}&from_room=${encodeURIComponent(fromRoom || '')}&to_bld=${toBldId}&to_room=${encodeURIComponent(toRoom || '')}`;
    try { return await apiFetch(`/combined-route?${params}`);
    } catch (e) { throw new Error('Combined route: ' + e.message); }
  },
};