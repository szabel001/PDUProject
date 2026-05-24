let currentModuleId = null;

const moduleHistory = {};
const MAX_HISTORY_POINTS = 10000;

function updateAllModules(arr) {
  if (!arr || !Array.isArray(arr)) {
    console.warn("updateAllModules: Invalid or missing data was received.", arr);
    return;
  }

  const incomingIds = arr.map(m => String(m.modbus_id));
  
  Object.keys(modulesCache).forEach(cachedId => {
    if (!incomingIds.includes(cachedId)) {
      delete modulesCache[cachedId];
      delete modulesConfigCache[cachedId];
      if (chartsSmall[cachedId]) {
        chartsSmall[cachedId].destroy();
        delete chartsSmall[cachedId];
      }
    }
  });
  // --- ÚJ RÉSZ VÉGE ---

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

  if (moduleHistory[m.modbus_id].length > MAX_HISTORY_POINTS) {
    moduleHistory[m.modbus_id].shift();
  }

  if (currentModuleId === m.modbus_id) {
    addDetailPoint(m.modbus_id, timeStr, m.voltage, m.current, m.power);
    updateDetailMetrics(m);
  }
}

function openModulePage(id) {
  const m = modulesCache[id];
  const mConf = { ...m, ...(modulesConfigCache[id] || {}) }; 

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

  const savedSlider = localStorage.getItem(`pdu_point_limit_${mConf.modbus_id}`) || '1000';

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
      <div class="small" id="detail_volt" style="font-weight:600;">RMS Voltage: ">${typeof m.voltage === 'number' ? m.voltage.toFixed(2) : '--'} V</div>
      <div class="small" id="detail_amp" style="font-weight:600;">RMS Current: ">${typeof m.current === 'number' ? m.current.toFixed(2) : '--'} A</div>
      <div class="small" id="detail_watt" style="font-weight:600;">Apparent Power: ">${typeof m.power === 'number' ? m.power.toFixed(2) : '--'} VA</div>
      <div class="small" id="detail_energy" style="font-weight:600; color: #00a651;">Total Consumption: ${typeof m.energy === 'number' ? m.energy.toFixed(4) : '--'} kVAh</div>
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
            <li><strong>Available LEDs:</strong> ${mConf.availableLeds !== undefined ? mConf.availableLeds : 'N/A'}</li>
            <li><strong>Hardware Current Limit:</strong> ${mConf.currentLimit !== undefined ? mConf.currentLimit : 'N/A'} A</li>
            <li><strong>Relay Count:</strong> ${mConf.relay_count !== undefined ? mConf.relay_count : 'N/A'}</li>
            <li><strong>Custom Warning Limit:</strong> <span id="config_warn_val">${m.curr_warning !== undefined ? m.curr_warning.toFixed(2) : '--'}</span> A</li>
            <li><strong>Custom Error Limit:</strong> <span id="config_err_val">${m.curr_error !== undefined ? m.curr_error.toFixed(2) : '--'}</span> A</li>

            </ul>
          <ul style="list-style:none; padding:0; display:flex; flex-direction:column; gap:8px;">
            <li><strong>Voltage Measured:</strong> ${mConf.isRMSVoltageMeasured ? 'Yes' : 'No'}</li>
            <li><strong>Current Measured:</strong> ${mConf.isRMSCurrentMeasured ? 'Yes' : 'No'}</li>
            <li><strong>Active Power Measured:</strong> ${mConf.isActivePowerMeasured ? 'Yes' : 'No'}</li>
            <li><strong>Reactive Power Measured:</strong> ${mConf.isReactivePowerMeasured ? 'Yes' : 'No'}</li>
            <li><strong>Frequency Measured:</strong> ${mConf.isACFrequencyMeasured ? 'Yes' : 'No'}</li>
          </ul>
        </div>
        
        <hr style="opacity: 0.1; margin: 15px 0;">
        <h4 style="margin: 0 0 10px 0;">Custom Current Limits</h4>
        <div style="display: flex; gap: 15px; align-items: center; flex-wrap: wrap;">
          <div>
            <label class="small" style="font-weight: 600;">Warning (A):</label><br>
            <input type="number" id="iec_warn_limit" step="0.1" min="0" max="32" value="${m.curr_warning !== undefined ? m.curr_warning : ''}" style="width: 100px; padding: 5px;">
          </div>
          <div>
            <label class="small" style="font-weight: 600;">Error (A):</label><br>
            <input type="number" id="iec_err_limit" step="0.1" min="0" max="32" value="${m.curr_error !== undefined ? m.curr_error : ''}" style="width: 100px; padding: 5px;">
          </div>
          <div style="margin-top: auto;">
            <button class="btn" onclick="saveIecModuleSettings(${mConf.modbus_id})">Save Limits</button>
          </div>
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
          <input type="range" id="point_limit_slider" min="100" max="50000" value="${savedSlider}" style="cursor:pointer;">
          <span id="point_limit_val" class="small" style="font-weight:bold; min-width:30px;">${savedSlider}</span>
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
        <button class="btn ghost chart-toggle" data-target="chart_watt" style="flex-grow: 1; text-align: center; background-color: #f8623449">Apparent Power (VA)</button>
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
      localStorage.setItem(`pdu_point_limit_${mConf.modbus_id}`, slider.value);
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

  html.querySelector('.data-toggle').onclick = (e) => {
    const target = document.getElementById(e.target.dataset.target);
    target.classList.toggle('open');
  };

  if (chartsDetail[m.modbus_id]) {
    if (chartsDetail[m.modbus_id]['chart_volt']) chartsDetail[m.modbus_id]['chart_volt'].destroy();
    if (chartsDetail[m.modbus_id]['chart_amp']) chartsDetail[m.modbus_id]['chart_amp'].destroy();
    if (chartsDetail[m.modbus_id]['chart_watt']) chartsDetail[m.modbus_id]['chart_watt'].destroy();
  }

  chartsDetail[m.modbus_id] = {};

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
      y: { beginAtZero: true, title: {display: true,text: 'Apparent Power (VA)'}} } }
  });
}

function updateDetailMetrics(m) {
  const vEl = document.getElementById('detail_volt');
  if (vEl) vEl.textContent = `RMS Voltage: ${typeof m.voltage === 'number' ? m.voltage.toFixed(2) : '--'} V`;
  const aEl = document.getElementById('detail_amp');
  if (aEl) aEl.textContent = `RMS Current: ${typeof m.current === 'number' ? m.current.toFixed(2) : '--'} A`;
  const wEl = document.getElementById('detail_watt');
  if (wEl) wEl.textContent = `Apparent Power: ${typeof m.power === 'number' ? m.power.toFixed(2) : '--'} VA`;
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

  const statEl = document.getElementById('detail_status_val');
  if (statEl) {
    statEl.textContent = getStatusText(m.status);
    statEl.style.color = getStatusColor(m.status);
  }
}

function updateDetailConfig(m) {
  const warnInput = document.getElementById('iec_warn_limit');
  if (warnInput && document.activeElement !== warnInput && m.curr_warning !== undefined) {
    warnInput.value = m.curr_warning;
  }
  
  const errInput = document.getElementById('iec_err_limit');
  if (errInput && document.activeElement !== errInput && m.curr_error !== undefined) {
    errInput.value = m.curr_error;
  }

  const warnText = document.getElementById('config_warn_val');
  if (warnText && m.curr_warning !== undefined) {
    warnText.textContent = typeof m.curr_warning === 'number' ? m.curr_warning.toFixed(2) : m.curr_warning;
  }
  
  const errText = document.getElementById('config_err_val');
  if (errText && m.curr_error !== undefined) {
    errText.textContent = typeof m.curr_error === 'number' ? m.curr_error.toFixed(2) : m.curr_error;
  }
}

function addDetailPoint(id, t, v, a, p) {
  const chs = chartsDetail[id];
  if (!chs) return;

  const slider = document.getElementById('point_limit_slider');
  const pointLimit = slider ? parseInt(slider.value) : 1000;

  const updateChart = (chart, val) => {
    if (!chart) return;
    chart.data.labels.push(t);
    chart.data.datasets[0].data.push(val);

    while (chart.data.labels.length > pointLimit) {
      chart.data.labels.shift();
      chart.data.datasets[0].data.shift();
    }

    chart.update('none');


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
    alert("There is not enough data to export yet!");
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

  const m = modulesCache[modId];
  let newState = 0;
  if (m && m.relays && m.relays.length > relayIdx) {
    newState = m.relays[relayIdx] ? 0 : 1;
  }

  try {
    const res = await fetch(`/api/relay/set?mod=${modId}&relay=${relayIdx}&state=${newState}`, { method: 'GET' });
    
    if (!res.ok) {
        console.error("Error when switching the relay, server response:", res.status);
    }
    setTimeout(() => typeof fetchOnce === 'function' ? fetchOnce() : null, 150);
  } catch (e) {
    console.error('toggle error', e);
  } finally {
    if (btn) btn.disabled = false;
  }
}

async function saveIecModuleSettings(modId) {
  const warningInput = document.getElementById('iec_warn_limit');
  const errorInput = document.getElementById('iec_err_limit');

  if (!warningInput.checkValidity() || !errorInput.checkValidity()) {
    warningInput.reportValidity();
    errorInput.reportValidity();
    return;
  }

  const warningLimit = parseFloat(warningInput.value);
  const errorLimit = parseFloat(errorInput.value);

  if (warningLimit >= errorLimit) {
    alert("Error: The warning limit must be less than the error limit!");
    return;
  }

  if (isNaN(warningLimit) || isNaN(errorLimit)) {
    alert("Please fill all numeric fields with valid numbers!");
    return;
  }

  const payload = {
    mod: modId,
    warn: parseFloat(warningLimit),
    err: parseFloat(errorLimit),
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
      if (typeof fetchOnce === 'function') fetchOnce();
    } else {
      alert("Error: " + (result.error || "Unknown error"));
    }
  } catch (error) {
    console.error("Network error while saving IEC settings:", error);
    alert("Network error occurred while saving settings!");
  }
}