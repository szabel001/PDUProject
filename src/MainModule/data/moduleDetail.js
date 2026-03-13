let currentModuleId = null;

// Globális archívum az adatoknak a kliens csatlakozásától
const moduleHistory = {};
const MAX_HISTORY_POINTS = 10000; // Ekkora memóriát engedünk a böngészőnek (kb. 3 óra adat másodpercenként)

function updateAllModules(arr) {
  // Csak akkor próbáljuk feldolgozni, ha arr egy létező tömb
  if (!arr || !Array.isArray(arr)) {
    console.warn("updateAllModules: Érvénytelen vagy hiányzó adatok érkeztek", arr);
    return;
  }

  arr.forEach(m => upsertModule(m));
  if (!currentModuleId) {
    renderDashboard();
  }
}

function updateModuleConfig(arr){
    arr.forEach(m => upsertModuleConfig(m));
}

function upsertModuleConfig(m){
  modulesConfigCache[m.modbus_id] = m;
}


function upsertModule(m) {
  modulesCache[m.modbus_id] = m;
  updateSmallChartData(m);
  updateModuleCard(m);

  // --- ADATOK MENTÉSE A HISTORY-BA ---
  if (!moduleHistory[m.modbus_id]) {
    moduleHistory[m.modbus_id] = [];
  }

  const timeStr = new Date().toLocaleTimeString();
  moduleHistory[m.modbus_id].push({
    t: timeStr,
    v: m.voltage,
    a: m.current,
    p: m.power
  });

  // Ne engedjük végtelenül nőni a tömböt, megelőzzük a böngésző fagyását
  if (moduleHistory[m.modbus_id].length > MAX_HISTORY_POINTS) {
    moduleHistory[m.modbus_id].shift();
  }

  // Ha pont ez a modul van nyitva, frissítjük a grafikont is
  if (currentModuleId === m.modbus_id) {
    addDetailPoint(m.modbus_id, timeStr, m.voltage, m.current, m.power);
    updateDetailMetrics(m);
  }
}

function openModulePage(id) {
  const m = modulesCache[id];
  const mConf = modulesConfigCache[id] || m; 

  if (!m) return;

  document.getElementById('dashboard').style.display = 'none';
  const moduleView = document.getElementById('moduleView');
  moduleView.classList.add('active');

  currentModuleId = id;
  renderModuleDetail(m, mConf);
}

function showDashboard() {
  document.getElementById('moduleView').classList.remove('active');
  document.getElementById('dashboard').style.display = '';
  currentModuleId = null;
}

// ---------------- DETAIL RENDER ÉS XML EXPORT ----------------

function renderModuleDetail(m, mConf) {
  moduleDetailCard.innerHTML = '';

  const html = document.createElement('div');
  html.innerHTML = `
  <div style="display:flex; align-items:center; justify-content:space-between; gap:20px; flex-wrap:wrap; border-bottom: 1px solid rgba(0,0,0,0.1); padding-bottom:15px;">
    <div style="flex: 1; min-width: 150px;">
      <h3 style="margin:0;">IEC Module #${mConf.modbus_id}</h3>
      <div class="small">Firmware: (${mConf.version || 'v?'})</div>
      <div class="small" style="font-weight: 600; margin-top: 2px;">
        Status: <span id="detail_status_val" style="color: ${getStatusColor(m.status)};">${getStatusText(m.status)}</span>
      </div>
      <div class="small">ID: ${mConf.id}</div>
      <div class="small">Modbus ID: ${mConf.modbus_id}</div>
    </div>

    <div style="flex: 2; display:flex; flex-direction: column; align-items: center; gap: 4px; text-align: center; border-left: 1px solid rgba(253, 4, 4, 0.05); border-right: 1px solid rgba(0,0,0,0.05);">
      <div class="small" id="detail_volt" style="font-weight:600;">Actual RMS Voltage: ">${typeof m.voltage === 'number' ? m.voltage.toFixed(2) : '--'} V</div>
      <div class="small" id="detail_amp" style="font-weight:600;">Actual RMS Current: ">${typeof m.current === 'number' ? m.current.toFixed(2) : '--'} A</div>
      <div class="small" id="detail_watt" style="font-weight:600;">Actual Apparent Power: ">${typeof m.power === 'number' ? m.power.toFixed(2) : '--'} W</div>
    </div>
    
    <div style="flex: 1;"></div>
  </div>

  <div style="margin-top: 25px; margin-bottom: 10px;">
    <button class="btn ghost data-toggle" data-target="config_details" style="width: 100%;">Module Configuration</button>
  </div>
  <div id="config_details" class="detail-panel">
      <div class="settings-card" style="margin-top:10px;">
        <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 20px;">          
          <ul style="list-style:none; padding:0; display:flex; flex-direction:column; gap:8px;">
            <li><strong>Available LEDs:</strong> ${mConf.availableLeds || 'N/A'}</li>
            <li><strong>Current Limit:</strong> ${mConf.currentLimit || 'N/A'} A</li>
            <li><strong>Relay Count:</strong> ${mConf.relay_count}</li>
          </ul>
          <ul style="list-style:none; padding:0; display:flex; flex-direction:column; gap:8px;">
            <li><strong>Voltage Measured:</strong> ${mConf.isRMSVoltageMeasured ? 'Yes' : 'No'}</li>
            <li><strong>Current Measured:</strong> ${mConf.isRMSCurrentMeasured ? 'Yes' : 'No'}</li>
            <li><strong>Active Power Measured:</strong> ${mConf.isActivePowerMeasured ? 'Yes' : 'No'}</li>
            <li><strong>Reactive Power Measured:</strong> ${mConf.isReactivePowerMeasured ? 'Yes' : 'No'}</li>
            <li><strong>Frequency Measured:</strong> ${mConf.isACFrequencyMeasured ? 'Yes' : 'No'}</li>
          </ul>
        </div>
      </div>
  </div>

  <h3 style="margin:0;">Relay status:</h3>
  <button class="btn toggleBtn" id = "detailRelayBtn" style="margin:10px";>Relay Status: ...</button>

  <div style="margin-top:20px">
    <div style="display:flex; justify-content:space-between; align-items:center;">
      <h3>Charts</h3>
        <div style="display:flex; align-items:center; gap:10px; background:var(--card); padding:5px 15px; border-radius:10px; box-shadow:0 2px 5px rgba(0,0,0,0.05);">
          <span class="small" style="font-weight:600;">Visible points:</span>
          <input type="range" id="point_limit_slider" min="10" max="500" value="50" style="cursor:pointer;">
          <span id="point_limit_val" class="small" style="font-weight:bold; min-width:30px;">50</span>
        </div>
    </div>
    <div id="chartContainer" style="display:flex;flex-direction:column;gap:12px;">
      
      <div class="chart-header" style="display:flex; gap:10px; align-items: center;">
        <button class="btn ghost chart-toggle" data-target="chart_volt" style="flex-grow: 1; text-align: center; background-color: #67a4ff;">Voltage (V)</button>
        <button class="btn xml-btn" onclick="exportXML(${mConf.modbus_id}, 'volt')" style="white-space: nowrap;">⬇ XML Export</button>
      </div>
      <canvas id="chart_volt" class="detail-chart"></canvas>

      <div class="chart-header" style="display:flex; gap:10px; align-items: center;">
        <button class="btn ghost chart-toggle" data-target="chart_amp" style="flex-grow: 1; text-align: center; background-color: #59fc7d8a">Current (A)</button>
        <button class="btn xml-btn" onclick="exportXML(${mConf.modbus_id}, 'amp')" style="white-space: nowrap;">⬇ XML Export</button>
      </div>
      <canvas id="chart_amp" class="detail-chart"></canvas>

      <div class="chart-header" style="display:flex; gap:10px; align-items: center;">
        <button class="btn ghost chart-toggle" data-target="chart_watt" style="flex-grow: 1; text-align: center; background-color: #f8623449">Power (W)</button>
        <button class="btn xml-btn" onclick="exportXML(${mConf.modbus_id}, 'watt')" style="white-space: nowrap;">⬇ XML Export</button>
      </div>
      <canvas id="chart_watt" class="detail-chart"></canvas>
    </div>
  </div>
  `;
  moduleDetailCard.appendChild(html);

  const slider = document.getElementById('point_limit_slider');
  const sliderVal = document.getElementById('point_limit_val');

  slider.oninput = () => {
      sliderVal.textContent = slider.value;
  };

  moduleDetailCard.querySelector('.toggleBtn').onclick = (e) => toggleRelay(m.modbus_id, 0, e.target);  
  const toggleBtn = moduleDetailCard.querySelector('.toggleBtn');
  toggleBtn.textContent = m.relays?.[0] ? 'ON' : 'OFF';
  toggleBtn.className = `btn toggleBtn ${m.relays?.[0] ? 'relay-on' : 'relay-off'}`;

  // Lenyíló animációk
  moduleDetailCard.querySelectorAll('.chart-toggle').forEach(btn => {
    const target = moduleDetailCard.querySelector('#' + btn.dataset.target);
    btn.onclick = () => {
      target.classList.toggle('open');
      const targetId = btn.dataset.target;
      const ch = chartsDetail[m.modbus_id]?.[targetId];
      if (ch) { setTimeout(() => { ch.resize(); ch.update(); }, 350); }
    };
  });

  // Lenyíló logika hozzáadása:
  html.querySelector('.data-toggle').onclick = (e) => {
    const target = document.getElementById(e.target.dataset.target);
    target.classList.toggle('open');
  };

  // Előző chartok törlése
  if (chartsDetail[m.modbus_id]) {
    if (chartsDetail[m.modbus_id]['chart_volt']) chartsDetail[m.modbus_id]['chart_volt'].destroy();
    if (chartsDetail[m.modbus_id]['chart_amp']) chartsDetail[m.modbus_id]['chart_amp'].destroy();
    if (chartsDetail[m.modbus_id]['chart_watt']) chartsDetail[m.modbus_id]['chart_watt'].destroy();
  }

  chartsDetail[m.modbus_id] = {};

  // Történeti adatok betöltése a Chart JS formátumába
  const hist = moduleHistory[m.modbus_id] || [];
  const labels = hist.map(d => d.t);
  const dataV = hist.map(d => d.v);
  const dataA = hist.map(d => d.a);
  const dataW = hist.map(d => d.p);

  const commonOptions = { animation: true, maintainAspectRatio: true, plugins: { legend: {display: false}}};

  chartsDetail[m.modbus_id]['chart_volt'] = new Chart(document.getElementById('chart_volt'), {
    type: 'line',
    data: { labels: [...labels], datasets: [{ label: 'V', data: [...dataV], borderColor: '#0066ff', tension: 0.3 }] },
    options: { ...commonOptions, scales: {
      x: {title: {display: true, text: 'Time [hh:mm:ss]'}},
      y: { beginAtZero: true, title: {display: true,text: 'Voltage (V)'}}}}
  });

  chartsDetail[m.modbus_id]['chart_amp'] = new Chart(document.getElementById('chart_amp'), {
    type: 'line',
    data: { labels: [...labels], datasets: [{ label: 'A', data: [...dataA], borderColor: '#00a651', tension: 0.3 }] },
    options: { ...commonOptions, scales: {
      x: {title: {display: true, text: 'Time [hh:mm:ss]'}}, 
      y: { beginAtZero: true, title: {display: true,text: 'Current (A)'}}}}
  });

  chartsDetail[m.modbus_id]['chart_watt'] = new Chart(document.getElementById('chart_watt'), {
    type: 'line',
    data: { labels: [...labels], datasets: [{ label: 'W', data: [...dataW], borderColor: '#d64545', tension: 0.3 }] },
    options: { ...commonOptions, scales: {
      x: {title: {display: true, text: 'Time [hh:mm:ss]'}},
      y: { beginAtZero: true, title: {display: true,text: 'Power (W)'}} } }
  });
}

function updateDetailMetrics(m) {
  const vEl = document.getElementById('detail_volt');
  if (vEl) vEl.textContent = `Actual RMS Voltage: ${typeof m.voltage === 'number' ? m.voltage.toFixed(2) : '--'} V`;
  const aEl = document.getElementById('detail_amp');
  if (aEl) aEl.textContent = `Actual RMS Current: ${typeof m.current === 'number' ? m.current.toFixed(2) : '--'} A`;
  const wEl = document.getElementById('detail_watt');
  if (wEl) wEl.textContent = `Actual Apparent Power: ${typeof m.power === 'number' ? m.power.toFixed(2) : '--'} W`;
   // RELÉ GOMB FRISSÍTÉSE
  const relayBtn = document.getElementById('detailRelayBtn');
  if (relayBtn && m.relays) {
    const isRelayOn = m.relays[0];
    const newStateText = isRelayOn ? 'ON' : 'OFF';
    
    if (relayBtn.textContent !== newStateText) {
      relayBtn.textContent = newStateText;
      relayBtn.className = `btn toggleBtn ${isRelayOn ? 'relay-on' : 'relay-off'}`;
    }
  }
  // Státusz frissítése
  const statEl = document.getElementById('detail_status_val');
  if (statEl) {
    statEl.textContent = getStatusText(m.status);
    statEl.style.color = getStatusColor(m.status);
  }
}

function addDetailPoint(id, t, v, a, p) {
  const chs = chartsDetail[id];
  if (!chs) return;

  const slider = document.getElementById('point_limit_slider');
  const pointLimit = slider ? parseInt(slider.value) : 50;

  const updateChart = (chart, val) => {
    if (!chart) return;
    chart.data.labels.push(t);
    chart.data.datasets[0].data.push(val);

    // Dinamikus vágás a csúszka alapján
    while (chart.data.labels.length > pointLimit) {
      chart.data.labels.shift();
      chart.data.datasets[0].data.shift();
    }

    chart.update('none'); // A 'none' mód gyorsabb frissítést tesz lehetővé animáció nélkül


    chart.update();
  };

  updateChart(chs['chart_volt'], v);
  updateChart(chs['chart_amp'], a);
  updateChart(chs['chart_watt'], p);
}

// =========================================
// XML EXPORT FUNKCIÓ
// =========================================
function exportXML(modId, type) {
  const hist = moduleHistory[modId];
  if (!hist || hist.length === 0) {
    alert("Nincs még elég adat az exportáláshoz!");
    return;
  }

  let xmlString = '<?xml version="1.0" encoding="UTF-8"?>\n<measurements>\n';

  hist.forEach(point => {
    let val = 0;
    if (type === 'volt') val = point.v;
    if (type === 'amp') val = point.a;
    if (type === 'watt') val = point.p;

    xmlString += `  <dataPoint>\n`;
    xmlString += `    <time>${point.t}</time>\n`;
    xmlString += `    <value>${val}</value>\n`;
    xmlString += `  </dataPoint>\n`;
  });

  xmlString += '</measurements>';

  // Letöltés indítása böngészőben Blob segítségével
  const blob = new Blob([xmlString], { type: 'application/xml' });
  const url = URL.createObjectURL(blob);

  const a = document.createElement('a');
  a.href = url;
  a.download = `Modul_${modId}_${type}_export.xml`;
  document.body.appendChild(a);
  a.click();

  // Takarítás
  document.body.removeChild(a);
  URL.revokeObjectURL(url);
}

let toggleLock = {};

async function toggleRelay(modId, relayIdx, btn = null) {
  const key = `${modId}_${relayIdx}`;
  if (toggleLock[key]) return;
  toggleLock[key] = true;
  setTimeout(() => toggleLock[key] = false, 600);

  if (btn) { btn.disabled = true; }

  // 1. Jelenlegi állapot lekérése a cache-ből és invertálása
  const m = modulesCache[modId];
  let newState = 0; // Alapértelmezett kikapcsolás
  if (m && m.relays && m.relays.length > relayIdx) {
    newState = m.relays[relayIdx] ? 0 : 1; // Ha 1 volt, 0 lesz, és fordítva
  }

  try {
    // 2. A HELYES C++ backend végpont meghívása
    const res = await fetch(`/api/relay/set?mod=${modId}&relay=${relayIdx}&state=${newState}`, { method: 'GET' });
    
    if (!res.ok) {
        console.error("Hiba a relé kapcsolásakor, szerver válasz:", res.status);
    }
    setTimeout(() => typeof fetchOnce === 'function' ? fetchOnce() : null, 150);
  } catch (e) {
    console.error('toggle error', e);
  } finally {
    if (btn) btn.disabled = false;
  }
}

async function saveIecModuleSettings(modId) {
  const warningLimit = document.getElementById('iec_warn_limit').value;
  const errorLimit = document.getElementById('iec_err_limit').value;
  const avgNum = document.getElementById('iec_avg_num').value;

  const payload = {
    mod: modId,
    warn: parseFloat(warningLimit),
    err: parseFloat(errorLimit),
    avg: parseInt(avgNum)
  };

  console.log("Saving IEC Module settings:", payload);

  try {
    const res = await fetch('/api/module/settings', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });

    const result = await res.json();
    if (res.ok && result.ok) {
      alert(`Module #${modId} settings saved successfully!`);
      showDashboard(); // Visszatérés a főoldalra
    } else {
      alert("Error: " + (result.error || "Unknown error"));
    }
  } catch (error) {
    console.error("Network error while saving IEC settings:", error);
    alert("Network error occurred while saving settings!");
  }
}