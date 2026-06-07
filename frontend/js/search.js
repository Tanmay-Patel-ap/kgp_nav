// ── STATE ─────────────────────────────────────────────────────────
const searchState = {
  origin:  { building: null },
  dest:    { building: null },
};

// ── ELEMENTS ──────────────────────────────────────────────────────
const originInput    = document.getElementById('origin-input');
const destInput      = document.getElementById('dest-input');
const originDropdown = document.getElementById('origin-dropdown');
const destDropdown   = document.getElementById('dest-dropdown');

// ── WIRE UP BOTH FIELDS ───────────────────────────────────────────
function initSearch(buildingDatabase) {
  wireField(originInput, originDropdown, 'origin', buildingDatabase);
  wireField(destInput,   destDropdown,   'dest',   buildingDatabase);

  // swap button
  document.getElementById('swap-btn').addEventListener('click', () => {
    const tmp = { ...searchState.origin };
    searchState.origin = { ...searchState.dest };
    searchState.dest   = tmp;

    originInput.value = searchState.origin.building?.name ?? '';
    destInput.value   = searchState.dest.building?.name   ?? '';

    clearDropdown(originDropdown);
    clearDropdown(destDropdown);
  });
}

// ── WIRE A SINGLE FIELD ───────────────────────────────────────────
function wireField(input, dropdown, key, db) {

  input.addEventListener('input', () => {
    const q = input.value.trim().toLowerCase();
    searchState[key].building = null; // clear on any edit
    clearDropdown(dropdown);
    if (!q) return;

    const matches = db
      .filter(b => b.name.toLowerCase().includes(q))
      .slice(0, 8);

    matches.forEach(b => {
      const div = document.createElement('div');
      div.className = 'dropdown-item';
      div.textContent = b.name;
      div.addEventListener('mousedown', (e) => {
        e.preventDefault(); // don't blur input first
        selectBuilding(input, dropdown, key, b);
      });
      dropdown.appendChild(div);
    });
  });

  input.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') {
      clearDropdown(dropdown);
      input.blur();
    }
    if (e.key === 'Enter') {
      // pick first dropdown item if available
      const first = dropdown.querySelector('.dropdown-item');
      if (first) first.dispatchEvent(new Event('mousedown'));
    }
  });

  // close on blur (after mousedown fires)
  input.addEventListener('blur', () => {
    setTimeout(() => clearDropdown(dropdown), 150);
  });
}

// ── SELECT A BUILDING ─────────────────────────────────────────────
function selectBuilding(input, dropdown, key, building) {
  searchState[key].building = building;
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
}

// ── HELPERS ───────────────────────────────────────────────────────
function clearDropdown(dropdown) {
  dropdown.innerHTML = '';
}

function getSearchState() {
  return searchState;
}