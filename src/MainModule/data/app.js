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
    const url = (location.protocol === 'https:' ? 'wss://' : 'ws://') + location.host + '/ws';
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

// ==========================================
// BEÁLLÍTÁSOK KEZELÉSE (Settings)
// ==========================================

async function fetchSettings() {
  try {
    const res = await fetch('/api/settings');
    if (!res.ok) throw new Error("API Hiba");
    const settings = await res.json();
    
    // STA
    if(document.getElementById('set_sta_ssid')) document.getElementById('set_sta_ssid').value = settings.sta_ssid || '';
    if(document.getElementById('set_sta_pass')) document.getElementById('set_sta_pass').value = settings.sta_pass || '';
    if(document.getElementById('sta_status')) document.getElementById('sta_status').textContent = settings.sta_status || 'N/A';

    // AP
    if(document.getElementById('set_ap_ip')) document.getElementById('set_ap_ip').value = settings.ap_ip || '';
    if(document.getElementById('set_ap_gw')) document.getElementById('set_ap_gw').value = settings.ap_gw || '';
    if(document.getElementById('set_ap_sn')) document.getElementById('set_ap_sn').value = settings.ap_sn || '';
    if(document.getElementById('set_ap_dns')) document.getElementById('set_ap_dns').value = settings.ap_dns || '';
    if(document.getElementById('ap_status')) document.getElementById('ap_status').textContent = settings.ap_status || 'N/A';

    // ETH
    if(document.getElementById('set_eth_dhcp')) document.getElementById('set_eth_dhcp').value = settings.eth_dhcp !== undefined ? settings.eth_dhcp : 1;
    if(document.getElementById('set_eth_ip')) document.getElementById('set_eth_ip').value = settings.eth_ip || '';
    if(document.getElementById('set_eth_gw')) document.getElementById('set_eth_gw').value = settings.eth_gw || '';
    if(document.getElementById('set_eth_sn')) document.getElementById('set_eth_sn').value = settings.eth_sn || '';
    if(document.getElementById('set_eth_dns')) document.getElementById('set_eth_dns').value = settings.eth_dns || '';

    // MEASURING
    if(document.getElementById('set_meas_oc')) document.getElementById('set_meas_oc').value = settings.meas_oc || 16;
    if(document.getElementById('set_meas_temp')) document.getElementById('set_meas_temp').value = settings.meas_temp || 'C';
    if(document.getElementById('set_meas_cycle')) document.getElementById('set_meas_cycle').value = settings.meas_cycle || 1;
    if(document.getElementById('set_meas_delay')) document.getElementById('set_meas_delay').value = settings.meas_delay || 0;

    // MQTT
    if(document.getElementById('set_mqtt_ip')) document.getElementById('set_mqtt_ip').value = settings.mqtt_server || '';
    if(document.getElementById('set_mqtt_port')) document.getElementById('set_mqtt_port').value = settings.mqtt_port || 1883;

  } catch (error) {
    console.warn("Nem sikerült lekérni a beállításokat az ESP-ről:", error);
  }
}

document.addEventListener("DOMContentLoaded", () => {
  fetchSettings();

  // WIFI STA MENTÉS
  if(document.getElementById('saveStaBtn')) {
    document.getElementById('saveStaBtn').onclick = () => saveSettings({
      sta_ssid: document.getElementById('set_sta_ssid').value,
      sta_pass: document.getElementById('set_sta_pass').value
    });
  }

  // WIFI AP MENTÉS
  if(document.getElementById('saveApBtn')) {
    document.getElementById('saveApBtn').onclick = () => saveSettings({
      ap_ip: document.getElementById('set_ap_ip').value,
      ap_gw: document.getElementById('set_ap_gw').value,
      ap_sn: document.getElementById('set_ap_sn').value,
      ap_dns: document.getElementById('set_ap_dns').value
    });
  }

  // ETHERNET MENTÉS
  if(document.getElementById('saveEthBtn')) {
    document.getElementById('saveEthBtn').onclick = () => saveSettings({
      eth_dhcp: Number(document.getElementById('set_eth_dhcp').value),
      eth_ip: document.getElementById('set_eth_ip').value,
      eth_gw: document.getElementById('set_eth_gw').value,
      eth_sn: document.getElementById('set_eth_sn').value,
      eth_dns: document.getElementById('set_eth_dns').value
    });
  }

  // MEASURING MENTÉS
  if(document.getElementById('saveMeasBtn')) {
    document.getElementById('saveMeasBtn').onclick = () => saveSettings({
      meas_oc: Number(document.getElementById('set_meas_oc').value),
      meas_temp: document.getElementById('set_meas_temp').value,
      meas_cycle: Number(document.getElementById('set_meas_cycle').value),
      meas_delay: Number(document.getElementById('set_meas_delay').value)
    });
  }

  // MQTT MENTÉS
  if(document.getElementById('saveMqttBtn')) {
    document.getElementById('saveMqttBtn').onclick = () => saveSettings({
      mqtt_server: document.getElementById('set_mqtt_ip').value,
      mqtt_port: Number(document.getElementById('set_mqtt_port').value)
    });
  }
});

async function saveSettings(payload) {
  try {
    const res = await fetch('/api/settings', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });
    
    if (res.ok) {
      alert("Beállítások sikeresen mentve az ESP-re!");
    } else {
      alert("Hiba a mentés során.");
    }
  } catch (error) {
    console.error("Hiba a küldéskor:", error);
    alert("Hálózati hiba mentéskor.");
  }
}