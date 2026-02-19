// Fallback polling if WS not available

/*
  Feltételezett WS message formátum (JSON):
  {
    type: "full" | "update" | "module",
    modules: [ { modbus_id:1, id:..., voltage:..., current:..., power:..., relays:[true,false], version:"", relay_count:2, ... }, ... ],
    module: { ... } // ha type === "module"
  }

  Ha nincs websocket a szerveren, a client fallbackre poll-ol (GET /api/data).
  Toggle végpont: /api/toggle?mod=<id>&relay=<r>
*/

const MAX_POINTS = 60;            // grafikon pontok száma
let modulesCache = {};            // modbus_id -> modul objektum
let chartsSmall = {};             // kártya mini chartok
let chartsDetail = {};            // modul oldal nagy chartok
let ws = null;
let wsReconnectInterval = 2000;
let connected = false;

const connStatusEl = document.getElementById('connStatus');
const modulesGrid = document.getElementById('modulesGrid');
const moduleView = document.getElementById('moduleView');
const moduleDetailCard = document.getElementById('moduleDetailCard');
const dashboard = document.getElementById('dashboard');

// Theme
const themeBtn = document.getElementById('themeBtn');

function applyTheme(dark) {
  if (dark) document.body.classList.add('dark'); else document.body.classList.remove('dark');
  themeBtn.textContent = dark ? '☀️' : '🌙';
  localStorage.setItem('pdu_dark', dark ? '1' : '0');
}
themeBtn.onclick = () => applyTheme(!(document.body.classList.contains('dark')));
applyTheme(localStorage.getItem('pdu_dark') === '1');
// Refresh btn
document.getElementById('refreshBtn').onclick = () => fetchOnce();

// Back btn
document.getElementById('backBtn').onclick = () => {
  history.pushState({}, '', '/');
  showDashboard();
};

// Routing: hash or history
window.addEventListener('popstate', () => {
  if (location.hash.startsWith('#module-')) {
    const id = parseInt(location.hash.replace('#module-', ''), 10);
    openModulePage(id);
  } else {
    showDashboard();
  }
});

// Fallback polling if WS not available
let pollTimer = null;
function fallbackToPoll() {
  if (pollTimer) return;
  pollTimer = setInterval(() => fetchOnce(), 2000);
}

// stop polling if ws ok
function stopPoll() {
  if (pollTimer) { clearInterval(pollTimer); pollTimer = null; }
}

// Fetch once via HTTP
async function fetchOnce() {
  try {
    const res = await fetch('/api/data');
    const arr = await res.json();
    updateAllModules(arr);
  } catch (e) {
    console.error('Fetch /api/data failed', e);
    connStatusEl.textContent = 'API hiba';
    connStatusEl.style.color = 'red';
  }
}

// ---------------- Startup ----------------
function start() {
  // try websocket first
  initWebSocket();
  // also start polling initially until ws opens
  setTimeout(() => {
    if (!connected) {
      connStatusEl.textContent = 'WebSocket nem elérhető — Polling...';
      fallbackToPoll();
    } else {
      stopPoll();
    }
  }, 800);
  // initial http load (in case ws slower)
  fetchOnce();
}

start();

// Initialize WebSocket connection
function initWebSocket() {
    console.log('Initializing WebSocket connection...');
  if (ws) ws.close();
    const url = (location.protocol === 'https:' ? 'wss://' : 'ws://') + location.host;
  try {
    ws = new WebSocket(url);
  } catch (e) {
    console.warn('WS create failed', e);
    fallbackToPoll();
    return;
  }

  ws.onopen = () => {
    connected = true;
    connStatusEl.textContent = 'WebSocket: csatlakozva';
    connStatusEl.style.color = '';
    // ask full state
    ws.send(JSON.stringify({ action: 'subscribe' }));
  };

  ws.onmessage = (evt) => {
    try {
      const msg = JSON.parse(evt.data);
      handleWsMessage(msg);
    } catch (e) {
      console.error('WS msg parse error', e);
    }
  };

  ws.onclose = () => {
    connected = false;
    connStatusEl.textContent = 'WebSocket: bontva — Polling...';
    connStatusEl.style.color = 'orange';
    // reconnect after a while
    setTimeout(initWebSocket, wsReconnectInterval);
  };

  ws.onerror = (e) => {
    console.warn('WS error', e);
    ws.close();
  };
}

// Handle WS messages
function handleWsMessage(msg) {
  if (msg.type === 'full' && Array.isArray(msg.modules)) {
    updateAllModules(msg.modules);
  } else if (msg.type === 'update' && Array.isArray(msg.modules)) {
    // partial updates array
    msg.modules.forEach(m => upsertModule(m));
  } else if (msg.type === 'module' && msg.module) {
    upsertModule(msg.module);
  } else {
    console.log('Unknown WS message', msg);
  }
}

// Ensure dashboard clickable modules open detail on direct hash navigation
if (location.hash.startsWith('#module-')) {
  const id = parseInt(location.hash.replace('#module-', ''), 10);
  setTimeout(() => openModulePage(id), 300);
}
