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
let modulesConfigCache = {};      // modbus_id config -> modul objektum
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
    const data = await res.json();

    // Szumma mezők frissítése a backend adatai alapján
    const tcEl = document.getElementById('total_current');
    const tpEl = document.getElementById('total_power');
    if ((tcEl && data.total_current !== undefined) && (tpEl && data.total_power !== undefined)) {
      tcEl.textContent = data.total_current.toFixed(2) + ' A';
      tpEl.textContent = data.total_power.toFixed(2) + ' W';
      return;
    }
    // Modulok betöltése az új formátumból (vagy a régiből, ha cache-ből jön)
    const modulesArr = data.modules ? data.modules : data;
    updateAllModules(modulesArr);
    updateModuleConfig(modulesArr);
    
    if(document.getElementById('iec_warn_limit')) document.getElementById('iec_warn_limit').value = data.curr_warning || '';
    if(document.getElementById('iec_err_limit')) document.getElementById('iec_err_limit').value = data.curr_error || '';

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
  // ÚJ: Szumma frissítése, ha megérkezett a backendtől a websocketen
  if (msg.total_current !== undefined) {
     const tcEl = document.getElementById('total_current');
     if (tcEl) tcEl.textContent = msg.total_current.toFixed(2) + ' A';
  }
  if (msg.total_power !== undefined) {
     const tpEl = document.getElementById('total_power');
     if (tpEl) tpEl.textContent = msg.total_power.toFixed(2) + ' W';
  }

  // Modulok frissítése
  if (msg.type === 'full' && Array.isArray(msg.modules)) {
    updateAllModules(msg.modules);
  } else if (msg.type === 'update' && Array.isArray(msg.modules)) {
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

    // ÚJ: Eltároljuk a globális currentSettings-be az összes beállítást (a delay-t is!)
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
});

async function saveSettings(url, payload) {
  console.log("Send to:", url, "Data:", payload); // Debug info
  try {
    const res = await fetch(url, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });
    
    const result = await res.json(); // Válasz megvárása
    console.log("Response:", result); // Debug info
    console.log("HTTP status:", res.status, "OK?", res.ok); // Debug info
    console.log("API response 'ok' field:", result.ok); // Debug info
    console.log("API response 'error' field:", result.error); // Debug info
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

// Rendszeres állapot lekérdezés a szervertől (STA, AP, MQTT)
setInterval(async () => {
  const staStatusEl = document.getElementById('sta_status');
  if (!staStatusEl) return; // Ha nem a settings oldalon vagyunk, ne fusson feleslegesen

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

  } catch (error) {
    // Hálózati hiba esetén csendben maradunk
  }
}, 2000);

// --- GOMB KLIKK ESEMÉNYEK ---

// WiFi STA Be/Ki gomb
document.getElementById('sta_enable_toggle').onclick = async () => {
  const btn = document.getElementById('sta_enable_toggle');
  const newState = !btn.classList.contains('on');
  await fetch('/api/settings/setStaMode', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ enabled: newState })
  });
};

// WiFi AP Be/Ki gomb
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

// MQTT Be/Ki gomb
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

// WiFi Be/Ki gomb kezelése
document.getElementById('sta_enable_toggle').onclick = async () => {
  const staBtn = document.getElementById('sta_enable_toggle');
  const isCurrentlyOn = staBtn.classList.contains('on');
  const newState = !isCurrentlyOn;
  
  // ESP értesítése a váltásról
  await fetch('/api/settings/setStaMode', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ enabled: newState })
  });
};

// Aszinkron WiFi Szkennelés és listázás
document.getElementById('scanWifiBtn').onclick = async () => {
  const btn = document.getElementById('scanWifiBtn');
  const list = document.getElementById('wifiList');
  
  btn.disabled = true;
  btn.textContent = "Scanning...";
  list.innerHTML = '<div class="small" style="padding: 15px; text-align: center;">Searching for networks...</div>';

  try {
    // 1. Szkennelés elindítása
    await fetch('/api/wifi/scan_start', { method: 'POST' });

    // 2. Pollolás (lekérdezés) amíg be nem fejeződik
    const pollScan = setInterval(async () => {
      const res = await fetch('/api/wifi/scan_results');
      const data = await res.json();
      
      if (data.status === 'done') {
        clearInterval(pollScan); // Leállítjuk a pollingot
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
      }
    }, 1500); // 1.5 másodpercenként ellenőrzi, hogy kész-e a szkennelés

  } catch (e) {
    list.innerHTML = '<div class="small" style="padding: 15px; text-align: center; color: red;">Scan request failed</div>';
    btn.disabled = false;
    btn.textContent = "Scan 🔍";
  }
};

// Csatlakozás gomb elküldi az adatokat a szervernek (a státusz automatikusan fog frissülni)
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

// Szekvenciális kapcsolás (Delay figyelembevételével)
async function sequenceRelays(turnOn) {
  const moduleIds = Object.keys(modulesCache);
  if (moduleIds.length === 0) {
    alert("Nincsenek aktív modulok!");
    return;
  }

  // Késleltetés másodpercben (alapból 0, ha nincs beállítva), átváltva milliszekundumra
  const delayMs = (currentSettings.meas_delay ? currentSettings.meas_delay : 0) * 1000;
  const state = turnOn ? 1 : 0; // 1 = ON, 0 = OFF

  const turnAllOnBtn = document.getElementById('turnAllOnBtn');
  const turnAllOffBtn = document.getElementById('turnAllOffBtn');

  if (turnAllOnBtn) turnAllOnBtn.disabled = true;
  if (turnAllOffBtn) turnAllOffBtn.disabled = true;

  for (let i = 0; i < moduleIds.length; i++) {
    const modId = moduleIds[i];

    try {
      await fetch(`/api/relay/set?mod=${modId}&relay=0&state=${state}`);
      // Adatok azonnali frissítése a weboldalon
      if (typeof fetchOnce === 'function') fetchOnce();
    } catch (e) {
      console.error(`Hiba a modul kapcsolásakor: ${modId}`, e);
    }

    // Csak a bekapcsolásnál (turnOn === true) várakozunk, 
    // és az utolsó modul után már felesleges várni.
    if (turnOn && delayMs > 0 && i < moduleIds.length - 1) {
      // Várakozás a következő bekapcsolásáig
      await new Promise(resolve => setTimeout(resolve, delayMs));
    }
  }

  if (turnAllOnBtn) turnAllOnBtn.disabled = false;
  if (turnAllOffBtn) turnAllOffBtn.disabled = false;
}

// Eseménykezelők rákötése a gombokra betöltés után
document.addEventListener("DOMContentLoaded", () => {
  const btnOn = document.getElementById('turnAllOnBtn');
  const btnOff = document.getElementById('turnAllOffBtn');
  if (btnOn) btnOn.onclick = () => sequenceRelays(true);
  if (btnOff) btnOff.onclick = () => sequenceRelays(false);
});