const AUTH_TOKEN_KEY = 'kgp_auth_token';
const AUTH_USER_KEY  = 'kgp_auth_user';
const AUTH_EMAIL_KEY = 'kgp_auth_email';

let isLoggedIn = false;
let currentUser = null;
let currentEmail = null;

// ── SIDEBAR ─────────────────────────────────────────────────────────
const getStartedBtn  = document.getElementById('get-started-btn');
const profileRow     = document.getElementById('auth-profile-row');
const avatarEl       = document.getElementById('auth-avatar');
const usernameLabel  = document.getElementById('auth-username-label');
const profileBtn     = document.getElementById('auth-profile-btn');

// ── AUTH MODAL ──────────────────────────────────────────────────────
const authModal      = document.getElementById('auth-modal');
const authClose      = document.getElementById('auth-modal-close');
const loginCard      = document.getElementById('auth-card-login');
const registerCard   = document.getElementById('auth-card-register');
const loginErr       = document.getElementById('auth-modal-error');
const regErr         = document.getElementById('auth-modal-error-reg');
const loginInput     = document.getElementById('auth-modal-login-input');
const loginPass      = document.getElementById('auth-modal-login-pass');
const regEmail       = document.getElementById('auth-modal-reg-email');
const regUsername    = document.getElementById('auth-modal-reg-username');
const regPass        = document.getElementById('auth-modal-reg-pass');
const regPass2       = document.getElementById('auth-modal-reg-pass2');
const loginBtn       = document.getElementById('auth-card-login-btn');
const regBtn         = document.getElementById('auth-card-reg-btn');
const switchToReg    = document.getElementById('auth-switch-to-register');
const switchToLogin  = document.getElementById('auth-switch-to-login');

// ── PROFILE MODAL ───────────────────────────────────────────────────
const profileModal   = document.getElementById('profile-modal');
const profileClose   = document.getElementById('profile-modal-close');
const profileAvatar  = document.getElementById('profile-avatar');
const profileNameEl  = document.getElementById('profile-username');
const profileEmailEl = document.getElementById('profile-email');
const logoutBtn      = document.getElementById('profile-logout-btn');

function userInitial(name) {
  return name && name.length > 0 ? name[0].toUpperCase() : '?';
}

function applyAvatar(el, name) {
  if (el) el.textContent = userInitial(name);
}

// ── COLLAPSE / RESTORE SIDEBAR ─────────────────────────────────────
function collapseSidebar() {
  const panel = document.getElementById('left-panel');
  const btn   = document.getElementById('collapse-btn');
  const ebtn  = document.getElementById('expand-btn');
  if (panel && !panel.classList.contains('collapsed')) {
    panel.classList.add('collapsed');
  }
  if (btn) btn.innerHTML = '<svg width="14" height="14" viewBox="0 0 14 14" fill="none"><path d="M5 2L10 7L5 12" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/></svg>';
  if (btn) btn.title = 'Expand sidebar';
  if (ebtn) ebtn.style.display = 'flex';
  setTimeout(() => { if (window.map) window.map.invalidateSize(); }, 350);
}

function restoreSidebar() {
  const panel = document.getElementById('left-panel');
  const btn   = document.getElementById('collapse-btn');
  const ebtn  = document.getElementById('expand-btn');
  if (panel && panel.classList.contains('collapsed')) {
    panel.classList.remove('collapsed');
  }
  if (btn) btn.innerHTML = '<svg width="14" height="14" viewBox="0 0 14 14" fill="none"><path d="M9 2L4 7L9 12" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/></svg>';
  if (btn) btn.title = 'Collapse sidebar';
  if (ebtn) ebtn.style.display = 'none';
  setTimeout(() => { if (window.map) window.map.invalidateSize(); }, 350);
}

function showModal(modal) {
  if (modal) modal.style.display = 'flex';
  collapseSidebar();
  // Clear inputs when opening modal
  clearLoginInputs();
  clearRegisterInputs();
}

function hideModal(modal) {
  if (modal) modal.style.display = 'none';
  restoreSidebar();
}

function clearLoginInputs() {
  if (loginInput) loginInput.value = '';
  if (loginPass)  loginPass.value  = '';
}

function clearRegisterInputs() {
  if (regEmail)    regEmail.value    = '';
  if (regUsername) regUsername.value  = '';
  if (regPass)     regPass.value     = '';
  if (regPass2)    regPass2.value    = '';
}

function showLoginCard() {
  clearRegisterInputs();
  if (loginCard) loginCard.style.display = 'block';
  if (registerCard) registerCard.style.display = 'none';
  if (loginErr) loginErr.textContent = '';
  if (regErr) regErr.textContent = '';
}

function showRegisterCard() {
  clearLoginInputs();
  if (loginCard) loginCard.style.display = 'none';
  if (registerCard) registerCard.style.display = 'block';
  if (loginErr) loginErr.textContent = '';
  if (regErr) regErr.textContent = '';
}

// ── INIT AUTH ──────────────────────────────────────────────────────
async function initAuth() {
  const savedToken  = localStorage.getItem(AUTH_TOKEN_KEY);
  const savedUser   = localStorage.getItem(AUTH_USER_KEY);
  const savedEmail  = localStorage.getItem(AUTH_EMAIL_KEY);

  if (savedToken && savedUser) {
    try {
      const resp = await fetch(API_BASE + '/auth/validate', {
        headers: { 'Authorization': 'Bearer ' + savedToken }
      });
      const data = await resp.json();
      if (data && data.valid) {
        isLoggedIn = true;
        currentUser = savedUser;
        currentEmail = savedEmail || '';
        applyAuthState();
        return;
      }
    } catch (_) {}
    localStorage.removeItem(AUTH_TOKEN_KEY);
    localStorage.removeItem(AUTH_USER_KEY);
    localStorage.removeItem(AUTH_EMAIL_KEY);
  }
  applyAuthState();
}

function applyAuthState() {
  if (isLoggedIn && currentUser) {
    if (getStartedBtn) getStartedBtn.style.display = 'none';
    if (profileRow)    profileRow.style.display    = 'flex';
    if (usernameLabel) usernameLabel.textContent   = currentUser;
    applyAvatar(avatarEl, currentUser);
    updateIndoorAccess(true);
  } else {
    if (getStartedBtn) getStartedBtn.style.display = 'block';
    if (profileRow)    profileRow.style.display    = 'none';
    updateIndoorAccess(false);
  }
}

function updateIndoorAccess(access) {
  const wraps = document.querySelectorAll('.room-select-wrap');
  wraps.forEach(wrap => {
    const inp = wrap.querySelector('input');
    const old = wrap.querySelector('.room-lock-overlay');
    if (old) old.remove();

    if (!access) {
      if (inp) {
        inp.dataset.origPlaceholder = inp.placeholder;
        inp.disabled = true;
        inp.placeholder = 'Log in for room navigation';
        inp.value = '';
      }
      const overlay = document.createElement('div');
      overlay.className = 'room-lock-overlay';
      overlay.addEventListener('click', (e) => {
        e.stopPropagation();
        if (getStartedBtn) getStartedBtn.click();
      });
      wrap.appendChild(overlay);
    } else {
      if (inp) {
        inp.disabled = false;
        inp.placeholder = inp.dataset.origPlaceholder || 'Main gate';
      }
    }
  });

  if (!access && typeof getSearchState === 'function') {
    const ss = getSearchState();
    if (ss) { ss.origin.room = null; ss.dest.room = null; }
  }

  // Re-fetch indoor graphs for buildings already selected after login
  if (access && typeof getSearchState === 'function') {
    const ss = getSearchState();
    ['origin', 'dest'].forEach(key => {
      const bld = ss[key] && ss[key].building;
      if (bld && window._roomUpdate) {
        window._roomUpdate.updateRoomWrap(key, bld);
      }
    });
  }
}

// ── GET STARTED ────────────────────────────────────────────────────
if (getStartedBtn) getStartedBtn.addEventListener('click', () => {
  showModal(authModal);
  showLoginCard();
});

// ── PROFILE BUTTON → OPEN PROFILE MODAL ───────────────────────────
if (profileBtn) profileBtn.addEventListener('click', () => {
  if (!isLoggedIn || !currentUser) return;
  applyAvatar(profileAvatar, currentUser);
  if (profileNameEl)  profileNameEl.textContent  = currentUser;
  if (profileEmailEl) profileEmailEl.textContent  = currentEmail || '';
  showModal(profileModal);
});

// ── CLOSE MODALS ────────────────────────────────────────────────────
if (authClose)    authClose.addEventListener('click',    () => hideModal(authModal));
if (profileClose) profileClose.addEventListener('click', () => hideModal(profileModal));
[authModal, profileModal].forEach(m => {
  if (m) m.addEventListener('click', (e) => {
    if (e.target === m) hideModal(m);
  });
});

// ── SWITCH BETWEEN LOGIN / REGISTER ────────────────────────────────
if (switchToReg)   switchToReg.addEventListener('click',   (e) => { e.preventDefault(); showRegisterCard(); });
if (switchToLogin) switchToLogin.addEventListener('click', (e) => { e.preventDefault(); showLoginCard(); });

// ── LOGIN ───────────────────────────────────────────────────────────
if (loginBtn) loginBtn.addEventListener('click', async () => {
  if (loginErr) loginErr.textContent = '';
  const login = (loginInput ? loginInput.value : '').trim();
  const pass  = loginPass ? loginPass.value : '';
  if (!login || !pass) { if (loginErr) loginErr.textContent = 'Fill all fields'; return; }
  try {
    const resp = await fetch(`${API_BASE}/auth/login?login=${encodeURIComponent(login)}&password=${encodeURIComponent(pass)}`);
    const data = await resp.json();
    if (!resp.ok) { if (loginErr) loginErr.textContent = data.error || 'Login failed'; return; }
    onAuthSuccess(data.token, data.username, login);
  } catch (_) {
    if (loginErr) loginErr.textContent = 'Server unreachable';
  }
});

// Enter key on login fields
if (loginInput) loginInput.addEventListener('keydown', (e) => { if (e.key === 'Enter' && loginBtn) loginBtn.click(); });
if (loginPass)  loginPass.addEventListener('keydown',  (e) => { if (e.key === 'Enter' && loginBtn) loginBtn.click(); });

function validateRegister(email, username, pass, pass2) {
  if (!email || !username || !pass || !pass2) return 'Fill all fields';
  if (!/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(email)) return 'Invalid email format';
  if (username.length < 3 || username.length > 20) return 'Username must be 3\u201320 characters';
  if (!/^[a-zA-Z]/.test(username)) return 'Username must start with a letter';
  if (!/^[a-zA-Z][a-zA-Z0-9_.]*$/.test(username)) return 'Username: letters, numbers, underscore only';
  if (pass.length < 8) return 'Password must be at least 8 characters';
  if (pass !== pass2) return 'Passwords do not match';
  return null;
}

// ── REGISTER ────────────────────────────────────────────────────────
if (regBtn) regBtn.addEventListener('click', async () => {
  if (regErr) regErr.textContent = '';
  const email    = (regEmail    ? regEmail.value    : '').trim();
  const username = (regUsername ? regUsername.value  : '').trim();
  const pass     = regPass  ? regPass.value  : '';
  const pass2    = regPass2 ? regPass2.value : '';
  const errMsg = validateRegister(email, username, pass, pass2);
  if (errMsg) { if (regErr) regErr.textContent = errMsg; return; }
  try {
    const resp = await fetch(`${API_BASE}/auth/register?email=${encodeURIComponent(email)}&username=${encodeURIComponent(username)}&password=${encodeURIComponent(pass)}`);
    const data = await resp.json();
    if (!resp.ok) { if (regErr) regErr.textContent = data.error || 'Registration failed'; return; }
    onAuthSuccess(data.token, data.username, email);
  } catch (_) {
    if (regErr) regErr.textContent = 'Server unreachable';
  }
});

// Enter key on register fields
[regEmail, regUsername, regPass, regPass2].forEach(el => {
  if (el) el.addEventListener('keydown', (e) => { if (e.key === 'Enter' && regBtn) regBtn.click(); });
});

// ── LOGOUT ──────────────────────────────────────────────────────────
if (logoutBtn) logoutBtn.addEventListener('click', async () => {
  const token = localStorage.getItem(AUTH_TOKEN_KEY);
  if (token) {
    try {
      await fetch(API_BASE + '/auth/logout', {
        headers: { 'Authorization': 'Bearer ' + token }
      });
    } catch (_) {}
  }
  localStorage.removeItem(AUTH_TOKEN_KEY);
  localStorage.removeItem(AUTH_USER_KEY);
  localStorage.removeItem(AUTH_EMAIL_KEY);
  isLoggedIn = false;
  currentUser = null;
  currentEmail = null;
  hideModal(profileModal);
  applyAuthState();
});

// ── HELPERS ─────────────────────────────────────────────────────────
function onAuthSuccess(token, username, loginId) {
  localStorage.setItem(AUTH_TOKEN_KEY, token);
  localStorage.setItem(AUTH_USER_KEY,  username);
  if (loginId && loginId.includes('@')) {
    localStorage.setItem(AUTH_EMAIL_KEY, loginId);
  }
  isLoggedIn = true;
  currentUser = username;
  currentEmail = loginId && loginId.includes('@') ? loginId : '';
  clearRegisterInputs();
  clearLoginInputs();
  hideModal(authModal);
  applyAuthState();
}

function getAuthToken() {
  return localStorage.getItem(AUTH_TOKEN_KEY);
}

function getAuthHeaders() {
  const token = getAuthToken();
  return token ? { 'Authorization': 'Bearer ' + token } : {};
}

async function fetchEmail(token) {
  try {
    const resp = await fetch(API_BASE + '/auth/me', {
      headers: { 'Authorization': 'Bearer ' + token }
    });
    const data = await resp.json();
    if (data && data.email) {
      localStorage.setItem(AUTH_EMAIL_KEY, data.email);
      currentEmail = data.email;
    }
  } catch (_) {}
}

document.addEventListener('DOMContentLoaded', () => {
  initAuth().then(() => {
    if (isLoggedIn && !currentEmail) {
      const token = localStorage.getItem(AUTH_TOKEN_KEY);
      if (token) fetchEmail(token);
    }
  });
});
