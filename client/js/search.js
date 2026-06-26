// ── STATE ─────────────────────────────────────────────────────────
const searchState = {
  origin:  { building: null, room: null },
  dest:    { building: null, room: null },
};

let indoorBuildingsMap = {}; // buildingId -> { prefix, name }
let roomData = { origin: [], dest: [] }; // per-key array of room nodes

// ── ELEMENTS ──────────────────────────────────────────────────────
const originInput    = document.getElementById('origin-input')    || null;
const destInput      = document.getElementById('dest-input')      || null;
const originDropdown = document.getElementById('origin-dropdown') || null;
const destDropdown   = document.getElementById('dest-dropdown')   || null;
const originRoomInp  = document.getElementById('origin-room-input')    || null;
const destRoomInp    = document.getElementById('dest-room-input')      || null;
const originRoomDrop = document.getElementById('origin-room-dropdown') || null;
const destRoomDrop   = document.getElementById('dest-room-dropdown')   || null;

// ── WIRE UP BOTH FIELDS ───────────────────────────────────────────
function initSearch(buildingDatabase, indoorList) {
  if (!originInput || !destInput || !originDropdown || !destDropdown) {
    return console.error('[search] Missing DOM elements');
  }

  // Build indoor map: outdoor building id → indoor info
  if (Array.isArray(indoorList)) {
    indoorList.forEach(ib => { indoorBuildingsMap[ib.id] = ib; });
  }

  wireField(originInput, originDropdown, 'origin', buildingDatabase);
  wireField(destInput,   destDropdown,   'dest',   buildingDatabase);

  wireRoomField(originRoomInp, originRoomDrop, 'origin');
  wireRoomField(destRoomInp,   destRoomDrop,   'dest');
}

// ── WIRE A SINGLE FIELD ───────────────────────────────────────────
function wireField(input, dropdown, key, db) {
  if (!input || !dropdown || !Array.isArray(db)) return;

  input.addEventListener('input', () => {
    const q = input.value.trim().toLowerCase();
    searchState[key].building = null;
    clearDropdown(dropdown);
    if (!q) return;

    try {
      const matches = db
        .filter(b => b && b.name && b.name.toLowerCase().includes(q))
        .slice(0, 8);

      matches.forEach(b => {
        const div = document.createElement('div');
        div.className = 'dropdown-item';
        div.textContent = b.name;
        div.addEventListener('mousedown', (e) => {
          e.preventDefault();
          selectBuilding(input, dropdown, key, b);
        });
        dropdown.appendChild(div);
      });
    } catch (e) {
      console.error('[search] Error filtering:', e.message);
    }
  });

  input.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') {
      clearDropdown(dropdown);
      input.blur();
    }
    if (e.key === 'Enter' && dropdown) {
      const first = dropdown.querySelector('.dropdown-item');
      if (first) first.dispatchEvent(new Event('mousedown'));
    }
  });

  input.addEventListener('blur', () => {
    setTimeout(() => clearDropdown(dropdown), 150);
  });
}

// ── WIRE ROOM INPUT ───────────────────────────────────────────────
function wireRoomField(input, dropdown, key) {
  if (!input || !dropdown) return;

  const hide = () => { dropdown.innerHTML = ''; };

  input.addEventListener('input', () => {
    const q = input.value.trim().toLowerCase();
    hide();
    const nodes = roomData[key] || [];
    if (!q) { searchState[key].room = null; return; }
    if (!nodes.length) return;

    nodes.forEach(n => {
      if (n.name && n.name.toLowerCase().includes(q)) {
        const div = document.createElement('div');
        div.className = 'room-dropdown-item';
        const block = n.block || 'Block ' + ((n.blockIdx || 0) + 1);
        const floor = n.floor === 0 ? 'Ground Floor' : 'Floor ' + n.floor;
        div.innerHTML = escapeHtml(n.name) + '<span class="room-sub">' + escapeHtml(block) + ', ' + escapeHtml(floor) + '</span>';
        div.addEventListener('mousedown', (e) => {
          e.preventDefault();
          input.value = n.name;
          searchState[key].room = n.id;
          hide();
        });
        dropdown.appendChild(div);
      }
    });
  });

  input.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') { hide(); input.blur(); }
    if (e.key === 'Enter') {
      const first = dropdown.querySelector('.room-dropdown-item');
      if (first) first.dispatchEvent(new Event('mousedown'));
    }
  });

  input.addEventListener('blur', () => {
    setTimeout(hide, 150);
  });
}

// ── SELECT A BUILDING ─────────────────────────────────────────────
function selectBuilding(input, dropdown, key, building) {
  if (!building || !building.name) return;
  searchState[key].building = building;
  searchState[key].room = null;
  input.value = building.name;
  clearDropdown(dropdown);

  // Fly map to building
  if (window.map) {
    window.map.flyTo([building.lat, building.lon], 18, { duration: 1 });
  }

  // Highlight on map
  resetAllBuildings();
  highlightBuilding(building.name, 'selected');

  setStatus(`${key === 'origin' ? 'FROM' : 'TO'}: ${building.name}`);

  // Update room select for this building
  updateRoomWrap(key, building);
}

// ── UPDATE ROOM INPUT ─────────────────────────────────────────────
function updateRoomWrap(key, building) {
  const inp = key === 'origin' ? originRoomInp : destRoomInp;
  if (!inp) return;

  roomData[key] = [];

  if (!building) {
    inp.disabled = true;
    inp.placeholder = 'Select a building first';
    inp.value = '';
    searchState[key].room = null;
    return;
  }

  const indoorInfo = indoorBuildingsMap[building.id];
  if (!indoorInfo) {
    inp.disabled = true;
    inp.placeholder = 'No indoor data';
    inp.value = '';
    searchState[key].room = null;
    return;
  }

  // Building has indoor data — load rooms
  inp.disabled = true;
  inp.placeholder = 'Loading...';
  inp.value = '';
  searchState[key].room = null;

  API.getIndoorGraph(indoorInfo.prefix)
    .then(graph => {
      if (!graph || !graph.nodes || graph.nodes.length === 0) {
        inp.placeholder = 'No rooms';
        inp.disabled = true;
        return;
      }

      // Store non-entrance rooms for search
      roomData[key] = graph.nodes.filter(n => n.id && n.type !== 'entrance');

      inp.value = 'Main gate';
      inp.disabled = false;
    })
    .catch(() => {
      inp.placeholder = 'Failed to load';
    });
}

// ── HELPERS ───────────────────────────────────────────────────────
function clearDropdown(dropdown) {
  dropdown.innerHTML = '';
}

function getSearchState() {
  return searchState;
}

function escapeHtml(s) {
  if (typeof s !== 'string') return '';
  return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
}