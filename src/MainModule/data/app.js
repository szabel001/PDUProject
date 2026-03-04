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
let currentSettings = {};

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
    const res = await fetch('/api/settings/getData');
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
  document.getElementById('saveStaBtn').onclick = () => saveSettings('/api/settings/setSta', {
  sta_ssid: document.getElementById('set_sta_ssid').value,
  sta_pass: document.getElementById('set_sta_pass').value
});
  }

  // WIFI AP MENTÉS
  if(document.getElementById('saveApBtn')) {
    document.getElementById('saveApBtn').onclick = () => saveSettings('/api/settings/setAp', {
      ap_ip: document.getElementById('set_ap_ip').value,
      ap_gw: document.getElementById('set_ap_gw').value,
      ap_sn: document.getElementById('set_ap_sn').value,
      ap_dns: document.getElementById('set_ap_dns').value
    });
  }

  // ETHERNET MENTÉS
  if(document.getElementById('saveEthBtn')) {
    document.getElementById('saveEthBtn').onclick = () => saveSettings('/api/settings/setEth', {
      eth_dhcp: Number(document.getElementById('set_eth_dhcp').value),
      eth_ip: document.getElementById('set_eth_ip').value,
      eth_gw: document.getElementById('set_eth_gw').value,
      eth_sn: document.getElementById('set_eth_sn').value,
      eth_dns: document.getElementById('set_eth_dns').value
    });
  }

  // MEASURING MENTÉS
  if(document.getElementById('saveMeasBtn')) {
    document.getElementById('saveMeasBtn').onclick = () => saveSettings('/api/settings/setMeas', {
      meas_oc: Number(document.getElementById('set_meas_oc').value),
      meas_temp: document.getElementById('set_meas_temp').value,
      meas_cycle: Number(document.getElementById('set_meas_cycle').value),
      meas_delay: Number(document.getElementById('set_meas_delay').value)
    });
  }

  // MQTT MENTÉS
  if(document.getElementById('saveMqttBtn')) {
    document.getElementById('saveMqttBtn').onclick = () => saveSettings('/api/settings/setMqtt', {
      mqtt_server: document.getElementById('set_mqtt_ip').value,
      mqtt_port: Number(document.getElementById('set_mqtt_port').value)
    });
  }

  // AP gomb
  if(document.getElementById('toggleApBtn')) {
      document.getElementById('toggleApBtn').onclick = async () => {
          const apActive = currentSettings.ap_status && currentSettings.ap_status.includes("Aktív");
          await saveSettings('/api/settings/ap_toggle', { active: !apActive }).then(updateAPSettingsUI);
          await fetchSettings();
      };
  }

    // MQTT gomb
  if(document.getElementById('toggleMqttBtn')) {
      document.getElementById('toggleMqttBtn').onclick = async () => {
          const mqttActive = currentSettings.mqtt_ena === 1;
          await saveSettings('/api/settings/mqttConnect', { mqtt_ena: mqttActive ? 0 : 1 }).then(updateMQTTSettingsUI);
          await fetchSettings();
      };
  }

  if(document.getElementById('connectStaBtn')) {
    document.getElementById('connectStaBtn').onclick = () => {
        saveSettings('/api/settings/sta_connect', { connect: true });
    };
  }
});

async function saveSettings(url, payload) {
  console.log("Küldés ide:", url, "Adat:", payload); // Debug info
  try {
    const res = await fetch(url, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });
    
    const result = await res.json(); // Válasz megvárása
    console.log("Válasz:", result); // Debug info
    console.log("HTTP státusz:", res.status, "OK?", res.ok); // Debug info
    console.log("API válasz 'ok' mező:", result.ok); // Debug info
    console.log("API válasz 'error' mező:", result.error); // Debug info
    if (res.ok && result.ok) {
      alert("Beállítások sikeresen mentve!");
    } else {
      alert("Szerver hiba: " + (result.error || "Ismeretlen hiba"));
    }
  } catch (error) {
    console.error("Hiba:", error);
    alert("Hálózati hiba!");
  }
}

async function updateMQTTSettingsUI() {
    const res = await fetch('/api/settings/mqttConnect');
    currentSettings = await res.json();

    // MQTT Gomb stílusa és felirata
    const mqttBtn = document.getElementById('toggleMqttBtn');
    const mqttEnabled = currentSettings.mqtt_ena === 1;
    mqttBtn.textContent = mqttEnabled ? "MQTT: ON" : "MQTT: OFF";
    mqttBtn.style.backgroundColor = mqttEnabled ? "#00a651" : "#d64545";
}

async function updateAPSettingsUI() {
    const res = await fetch('/api/settings/ap_toggle');
    currentSettings = await res.json();

    // AP Gomb stílusa és felirata
    const apBtn = document.getElementById('toggleApBtn');
    const apActive = currentSettings.ap_status && currentSettings.ap_status.includes("Aktív");
    apBtn.textContent = apActive ? "AP: ON" : "AP: OFF";
    apBtn.style.backgroundColor = apActive ? "#00a651" : "#d64545";
}


let selectedSSID = "";

// WiFi Be/Ki gomb kezelése
const staBtn = document.getElementById('sta_enable_toggle');
const staControls = document.getElementById('sta_controls');

staBtn.onclick = async () => {
  const isCurrentlyOn = staBtn.classList.contains('on');
  const newState = !isCurrentlyOn;
  
  // UI frissítése
  updateWifiBtnUI(newState);
  
  // ESP értesítése
  await fetch('/api/settings/setStaMode', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ enabled: newState })
  });
};

function updateWifiBtnUI(enabled) {
  staBtn.className = `btn wifi-toggle-btn ${enabled ? 'on' : 'off'}`;
  staBtn.textContent = enabled ? "WiFi: ON" : "WiFi: OFF";
  staControls.style.display = enabled ? 'block' : 'none';
}

// WiFi Szkennelés és listázás
document.getElementById('scanWifiBtn').onclick = async () => {
  const btn = document.getElementById('scanWifiBtn');
  const list = document.getElementById('wifiList');
  
  btn.disabled = true;
  btn.textContent = "Scanning...";
  list.innerHTML = '<div class="small" style="padding: 15px; text-align: center;">Searching for networks...</div>';

  try {
    const res = await fetch('/api/wifi/scan');
    const networks = await res.json();
    list.innerHTML = "";

    if (networks.length === 0) {
      list.innerHTML = '<div class="small" style="padding: 15px; text-align: center;">No networks found</div>';
    } else {
      networks.forEach(net => {
        const item = document.createElement('div');
        item.style = "padding: 10px; cursor: pointer; border-bottom: 1px solid rgba(0,0,0,0.05); display: flex; justify-content: space-between;";
        item.innerHTML = `
          <span style="font-size: 13px; font-weight: 500;">${net.ssid}</span>
          <span class="small">${net.rssi} dBm ${net.secure ? '🔒' : ''}</span>
        `;
        item.onclick = () => {
          // Kiemelés a listában
          Array.from(list.children).forEach(c => c.style.background = "");
          item.style.background = "rgba(0, 102, 255, 0.1)";
          
          selectedSSID = net.ssid;
          document.getElementById('selected_ssid_label').textContent = "Selected: " + net.ssid;
          document.getElementById('wifi_password_area').style.display = 'block';
        };
        list.appendChild(item);
      });
    }
  } catch (e) {
    list.innerHTML = '<div class="small" style="padding: 15px; text-align: center; color: red;">Scan failed</div>';
  } finally {
    btn.disabled = false;
    btn.textContent = "Scan 🔍";
  }
};

// Csatlakozás gomb
document.getElementById('connectStaBtn').onclick = () => {
  const pass = document.getElementById('set_sta_pass').value;
  saveSettings('/api/settings/setSta', {
    sta_ssid: selectedSSID,
    sta_pass: pass,
    sta_connect: "connect"
  });
};