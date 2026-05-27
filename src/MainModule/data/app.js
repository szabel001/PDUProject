// --- Status formázó függvények ---
function getStatusText(code) {
  switch(code) {
    case 0: return "OK";
    case 1: return "Warning (Warning Limit Reached)";
    case 2: return "Error (Error Limit Reached)";
    case 3: return "Error (Global Overcurrent Reached)";
    default: return "Unknown";
  }
}

function getStatusColor(code) {
  switch(code) {
    case 0: return "var(--ok)";
    case 1: return "orange";
    case 2: return "red";
    case 3: return "darkred";
    default: return "var(--muted)";
  }
}

const MAX_POINTS = 60;
let modulesCache = {};
let modulesConfigCache = {};
let chartsSmall = {};
let chartsDetail = {};
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
document.getElementById('refreshBtn').onclick = async () => {
  const btn = document.getElementById('refreshBtn');
  btn.disabled = true;
  btn.textContent = 'Scanning for IEC modules...';

  try {
    // 1. Hardveres szkennelés indítása a backendben
    const res = await fetch('/api/scan', { method: 'POST' });
    if (!res.ok) throw new Error("Scan failed");

    // 2. Frontend cache-ek törlése
    modulesCache = {};
    modulesConfigCache = {};
    for (const id in chartsSmall) {
      if (chartsSmall[id]) chartsSmall[id].destroy();
    }
    chartsSmall = {};
    document.getElementById('modulesGrid').innerHTML = '';

    // 3. Friss adatok lekérése
    btn.textContent = 'Fetching...';
    await fetchOnce();
    
  } catch (e) {
    console.error("Refresh error:", e);
    alert("Hiba történt a frissítés során!");
  } finally {
    btn.textContent = 'Refresh';
    btn.disabled = false;
  }
};
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
    const data = await res.json();

    const tcEl = document.getElementById('total_current');
    const tpEl = document.getElementById('total_power');
    const teEl = document.getElementById('total_energy');

    if ((tcEl && data.total_current !== undefined) && (tpEl && data.total_power !== undefined) && (teEl && data.total_energy !== undefined)) {
      tcEl.textContent = data.total_current.toFixed(2) + ' A';
      tpEl.textContent = data.total_power.toFixed(2) + ' VA';
      teEl.textContent = data.total_energy.toFixed(4) + ' kVAh';
    }
    try {
      const envRes = await fetch('/api/env');
      if (envRes.ok) {
        const envData = await envRes.json();
        
        const tempEl = document.getElementById('dash_env_temp');
        const humEl = document.getElementById('dash_env_hum');

        const isTempValid = envData.temperature !== 0 && envData.temperature !== null && envData.temperature !== undefined && !isNaN(envData.temperature);
        const isHumValid = envData.humidity !== 0 && envData.humidity !== null && envData.humidity !== undefined && !isNaN(envData.humidity);

        if (tempEl) {
          tempEl.innerHTML = isTempValid ? `${envData.temperature.toFixed(1)} <span style="font-size:16px;">°${envData.unit}</span>` : `-- <span style="font-size:16px;">°${envData.unit || 'C'}</span>`;
        }

        if (humEl) {
          humEl.innerHTML = isHumValid ? `${envData.humidity.toFixed(1)} <span style="font-size:16px;">%</span>` : `-- <span style="font-size:16px;">%</span>`;
        }
      }
    } catch (envError) {
      console.warn('Hiba a környezeti adatok lekérésekor:', envError);
    }

    const modulesArr = data.modules ? data.modules : data;
    updateAllModules(modulesArr);
    updateModuleConfig(modulesArr);
    updateDetailConfig(modulesArr);
    
  } catch (e) {
    console.error('Fetch /api/data failed', e);
    connStatusEl.textContent = 'API error';
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
      connStatusEl.textContent = 'WebSocket is not available — Polling...';
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
    connStatusEl.textContent = 'WebSocket: connected';
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
    connStatusEl.textContent = 'WebSocket: disconnected — Polling...';
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
  if (msg.total_current !== undefined) {
     const tcEl = document.getElementById('total_current');
     if (tcEl) tcEl.textContent = msg.total_current.toFixed(2) + ' A';
  }
  if (msg.total_power !== undefined) {
     const tpEl = document.getElementById('total_power');
     if (tpEl) tpEl.textContent = msg.total_power.toFixed(2) + ' VA';
  }
    if (msg.total_energy !== undefined) {
     const teEl = document.getElementById('total_energy');
     if (teEl) teEl.textContent = msg.total_energy.toFixed(4) + ' kVAh';
  }

  // Modulok frissítése
  if (msg.type === 'update' && Array.isArray(msg.modules)) {
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
    if (!res.ok) throw new Error("API error");
    const settings = await res.json();

    Object.assign(currentSettings, settings);

    // STA
    if(document.getElementById('set_sta_ssid')) document.getElementById('set_sta_ssid').value = settings.sta_ssid || '';
    if(document.getElementById('set_sta_pass')) document.getElementById('set_sta_pass').value = settings.sta_pass || '';
    if(document.getElementById('sta_status')) document.getElementById('sta_status').textContent = settings.sta_status || 'N/A';

    // AP
    if(document.getElementById('set_ap_ssid')) document.getElementById('set_ap_ssid').value = settings.ap_ssid || '';
    if(document.getElementById('set_ap_pass')) document.getElementById('set_ap_pass').value = settings.ap_pass || '';
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

    updateEthernetFieldsVisibility();

    // MEASURING
    if(document.getElementById('set_meas_oc')) document.getElementById('set_meas_oc').value = settings.meas_oc || 16;
    if(document.getElementById('set_meas_temp')) document.getElementById('set_meas_temp').value = settings.meas_temp || 'C';
    if(document.getElementById('set_meas_cycle')) document.getElementById('set_meas_cycle').value = settings.meas_cycle || 1;
    if(document.getElementById('set_meas_delay')) document.getElementById('set_meas_delay').value = settings.meas_delay || 0;

    // MQTT
    if(document.getElementById('mqtt_ena')) document.getElementById('mqtt_ena').value = settings.mqtt_ena || '';
    if(document.getElementById('set_mqtt_ip')) document.getElementById('set_mqtt_ip').value = settings.mqtt_server || '';
    if(document.getElementById('set_mqtt_port')) document.getElementById('set_mqtt_port').value = settings.mqtt_port || 1883;

  } catch (error) {
    console.warn("Nem sikerült lekérni a beállításokat az ESP-ről:", error);
  }
}

document.addEventListener("DOMContentLoaded", () => {
  fetchSettings();

  // WIFI AP MENTÉS
  if(document.getElementById('saveApBtn')) {
    document.getElementById('saveApBtn').onclick = () => saveSettings('/api/settings/setAp', {
      ap_ssid: document.getElementById('set_ap_ssid').value,
      ap_pass: document.getElementById('set_ap_pass').value,
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
    document.getElementById('saveMeasBtn').onclick = () => {
      const measDelayInput = document.getElementById('set_meas_delay');
      currentSettings.meas_delay = Number(measDelayInput.value);

      if (!measDelayInput.checkValidity()) {
        measDelayInput.reportValidity();
          alert("Please fill all numeric fields with valid numbers (from 0 to 60)!");
        return;
      }

      const measOcInput = document.getElementById('set_meas_oc');
      if (!measOcInput.checkValidity()) {
        measOcInput.reportValidity();
          alert("Please fill all numeric fields with valid numbers (from 0 to 32)!");
        return;
      }

      saveSettings('/api/settings/setMeas', {
        meas_oc: Number(document.getElementById('set_meas_oc').value),
        meas_temp: document.getElementById('set_meas_temp').value,
        meas_cycle: Number(document.getElementById('set_meas_cycle').value),
        meas_delay: currentSettings.meas_delay
      });
    };
  }

  // MQTT MENTÉS
  if(document.getElementById('saveMqttBtn')) {
    document.getElementById('saveMqttBtn').onclick = () => saveSettings('/api/settings/setMqtt', {
      mqtt_server: document.getElementById('set_mqtt_ip').value,
      mqtt_port: Number(document.getElementById('set_mqtt_port').value)
    });
  }
});

async function saveSettings(url, payload) {
  console.log("Send to:", url, "Data:", payload); 
  try {
    const res = await fetch(url, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });
    
    const result = await res.json();
    console.log("Response:", result);
    console.log("HTTP status:", res.status, "OK?", res.ok); 
    console.log("API response 'ok' field:", result.ok); 
    console.log("API response 'error' field:", result.error); 
    if (res.ok && result.ok) {
      alert("Settings has been saved successfully!");
    } else {
      alert("Server error: " + (result.error || "Unknown error"));
    }
  } catch (error) {
    console.error("Error:", error);
    alert("Network error!");
  }
}

let selectedSSID = "";

// Regular status query from the server (STA, AP, MQTT)
setInterval(async () => {
  const staStatusEl = document.getElementById('sta_status');
  if (!staStatusEl) return;

  try {
    // --- STA STATUS ---
    const resSta = await fetch('/api/settings/sta_status');
    const dataSta = await resSta.json();
    staStatusEl.textContent = dataSta.status;
    const staBtn = document.getElementById('sta_enable_toggle');
    const staControls = document.getElementById('sta_controls');
    
    if (dataSta.enabled) {
      if(staBtn) { staBtn.className = 'btn wifi-toggle-btn on'; staBtn.textContent = "WiFi: ON"; }
      if(staControls) staControls.style.display = 'block';
    } else {
      if(staBtn) { staBtn.className = 'btn wifi-toggle-btn off'; staBtn.textContent = "WiFi: OFF"; }
      if(staControls) staControls.style.display = 'none';
    }

    // --- AP STATUS ---
    const resAp = await fetch('/api/settings/ap_status');
    const dataAp = await resAp.json();
    const apStatusEl = document.getElementById('ap_status');
    if(apStatusEl) apStatusEl.textContent = dataAp.status;
    
    const apBtn = document.getElementById('toggleApBtn');
    if (apBtn) {
      if (dataAp.enabled) {
        apBtn.className = 'btn wifi-toggle-btn on';
        apBtn.textContent = "AP: ON";
      } else {
        apBtn.className = 'btn wifi-toggle-btn off';
        apBtn.textContent = "AP: OFF";
      }
    }

    // --- MQTT STATUS ---
    const resMqtt = await fetch('/api/settings/mqtt_status');
    const dataMqtt = await resMqtt.json();
    const mqttStatusEl = document.getElementById('mqtt_status');
    if(mqttStatusEl) mqttStatusEl.textContent = dataMqtt.status;
    
    const mqttBtn = document.getElementById('toggleMqttBtn');
    if (mqttBtn) {
      if (dataMqtt.enabled) {
        mqttBtn.className = 'btn wifi-toggle-btn on';
        mqttBtn.textContent = "MQTT: ON";
      } else {
        mqttBtn.className = 'btn wifi-toggle-btn off';
        mqttBtn.textContent = "MQTT: OFF";
      }
    }

  } catch (error) {}
}, 2000);

// --- BTN CLICK EVENTS ---
// WiFi STA ONOFF
document.getElementById('sta_enable_toggle').onclick = async () => {
  const btn = document.getElementById('sta_enable_toggle');
  const newState = !btn.classList.contains('on');
  await fetch('/api/settings/setStaMode', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ enabled: newState })
  });
};

// WiFi AP ONOFF
if(document.getElementById('toggleApBtn')) {
    document.getElementById('toggleApBtn').onclick = async () => {
        const btn = document.getElementById('toggleApBtn');
        const newState = !btn.classList.contains('on');
        await fetch('/api/settings/ap_toggle', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ enabled: newState })
        });
    };
}

// MQTT ONOFF
if(document.getElementById('toggleMqttBtn')) {
    document.getElementById('toggleMqttBtn').onclick = async () => {
        const btn = document.getElementById('toggleMqttBtn');
        const newState = !btn.classList.contains('on');
        await fetch('/api/settings/mqtt_toggle', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ enabled: newState })
        });
    };
}

// WiFi ONOFF
document.getElementById('sta_enable_toggle').onclick = async () => {
  const staBtn = document.getElementById('sta_enable_toggle');
  const isCurrentlyOn = staBtn.classList.contains('on');
  const newState = !isCurrentlyOn;
  await fetch('/api/settings/setStaMode', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ enabled: newState })
  });
};

// Async WiFi Scan
document.getElementById('scanWifiBtn').onclick = async () => {
  const btn = document.getElementById('scanWifiBtn');
  const list = document.getElementById('wifiList');
  
  btn.disabled = true;
  btn.textContent = "Scanning...";
  list.innerHTML = '<div class="small" style="padding: 15px; text-align: center;">Searching for networks...</div>';

  try {
    // 1. Start scanning
    await fetch('/api/wifi/scan_start', { method: 'POST' });

    // 2. Polling
    const pollScan = setInterval(async () => {
      const res = await fetch('/api/wifi/scan_results');
      const data = await res.json();
      
      if (data.status === 'done') {
        clearInterval(pollScan);
        btn.disabled = false;
        btn.textContent = "Scan 🔍";
        list.innerHTML = "";

        if (data.networks.length === 0) {
          list.innerHTML = '<div class="small" style="padding: 15px; text-align: center;">No networks found</div>';
        } else {
          data.networks.forEach(net => {
            const item = document.createElement('div');
            item.style = "padding: 10px; cursor: pointer; border-bottom: 1px solid rgba(0,0,0,0.05); display: flex; justify-content: space-between;";
            item.innerHTML = `
              <span style="font-size: 13px; font-weight: 500;">${net.ssid}</span>
              <span class="small">${net.rssi} dBm ${net.secure ? '🔒' : ''}</span>
            `;
            item.onclick = () => {
              Array.from(list.children).forEach(c => c.style.background = "");
              item.style.background = "rgba(0, 102, 255, 0.1)";
              
              selectedSSID = net.ssid;
              document.getElementById('selected_ssid_label').textContent = "Selected: " + net.ssid;
              document.getElementById('wifi_password_area').style.display = 'block';
            };
            list.appendChild(item);
          });
        }
      }
    }, 500);

  } catch (e) {
    list.innerHTML = '<div class="small" style="padding: 15px; text-align: center; color: red;">Scan request failed</div>';
    btn.disabled = false;
    btn.textContent = "Scan 🔍";
  }
};

document.getElementById('connectStaBtn').onclick = () => {
  const pass = document.getElementById('set_sta_pass').value;
  
  document.getElementById('sta_status').textContent = "Connecting...";

  fetch('/api/settings/sta_connect', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      sta_ssid: selectedSSID,
      sta_pass: pass
    })
  });
};

// Sequential switching (including delay)
async function sequenceRelays(turnOn) {
  const moduleIds = Object.keys(modulesCache);
  if (moduleIds.length === 0) {
    alert("No modules connected!");
    return;
  }
  const state = turnOn ? 1 : 0;

  const turnAllOnBtn = document.getElementById('turnAllOnBtn');
  const turnAllOffBtn = document.getElementById('turnAllOffBtn');

  if (turnAllOnBtn) turnAllOnBtn.disabled = true;
  if (turnAllOffBtn) turnAllOffBtn.disabled = true;

  try {
    await fetch(`/api/relay/setall?state=${state}`);
    if (typeof fetchOnce === 'function') fetchOnce();
  } catch (e) {
    console.error(`Error during setting relay state: ${modId}`, e);
  }

  if (turnAllOnBtn) turnAllOnBtn.disabled = false;
  if (turnAllOffBtn) turnAllOffBtn.disabled = false;
}

document.addEventListener("DOMContentLoaded", () => {
  const btnOn = document.getElementById('turnAllOnBtn');
  const btnOff = document.getElementById('turnAllOffBtn');
  if (btnOn) btnOn.onclick = () => sequenceRelays(true);
  if (btnOff) btnOff.onclick = () => sequenceRelays(false);
});

const ethDhcpSelect = document.getElementById('set_eth_dhcp');
const ethStaticFields = document.getElementById('eth_static_fields');

function updateEthernetFieldsVisibility() {
  if (ethDhcpSelect.value === "1") {
    ethStaticFields.style.display = 'none';
  } else {
    ethStaticFields.style.display = 'flex';
    ethStaticFields.style.flexDirection = 'column';
    ethStaticFields.style.gap = '5px'; // Egy kis térköz az elemek közé
  }
}

ethDhcpSelect.addEventListener('change', updateEthernetFieldsVisibility);