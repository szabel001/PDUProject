let currentModuleId = null;

// Globális archívum az adatoknak a kliens csatlakozásától
// Formátum: { modId: [ { t: "10:20:05", v: 230, a: 1.2, p: 276 }, ... ] }
const moduleHistory = {};
const MAX_HISTORY_POINTS = 10000; // Ekkora memóriát engedünk a böngészőnek (kb. 3 óra adat másodpercenként)

function updateAllModules(arr) {
  arr.forEach(m => upsertModule(m));
  if (!currentModuleId) {
    renderDashboard();
  }
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
  if (!m) return;

  document.getElementById('dashboard').style.display = 'none';
  const moduleView = document.getElementById('moduleView');
  moduleView.classList.add('active');

  currentModuleId = id;
  renderModuleDetail(m);
}

function showDashboard() {
  document.getElementById('moduleView').classList.remove('active');
  document.getElementById('dashboard').style.display = '';
  currentModuleId = null;
}

// ---------------- DETAIL RENDER ÉS XML EXPORT ----------------

function renderModuleDetail(m) {
  moduleDetailCard.innerHTML = '';

  const html = document.createElement('div');
  html.innerHTML = `
    <div style="display:flex;align-items:center;justify-content:space-between;gap:12px;flex-wrap:wrap">
      <div>
        <h2>IEC Module #${m.modbus_id}</h2>
        <span class="small"> (${m.version || 'v?'})</span>
        <div class="small">ID: ${m.id}</div>
        <div class="small">Relays: ${m.relay_count}</div>
      </div>
      <div class="chart-header" style="display:flex; justify-content:space-between;">
          <button class="btn ghost data-toggle" data-target="module-details">Module configuration</button>
      </div>
      <div style="text-align:right">
        <div class="small" id="detail_volt">Voltage: ${typeof m.voltage === 'number' ? m.voltage.toFixed(2) : '--'} V</div>
        <div class="small" id="detail_amp">Current: ${typeof m.current === 'number' ? m.current.toFixed(2) : '--'} A</div>
        <div class="small" id="detail_watt">Power: ${typeof m.power === 'number' ? m.power.toFixed(2) : '--'} W</div>
      </div>
    </div>

    <div style="margin-top:12px">
      <h3>Charts</h3>
      <div id="chartContainer" style="display:flex;flex-direction:column;gap:8px;">
        
        <div class="chart-header" style="display:flex; justify-content:space-between;">
          <button class="btn ghost chart-toggle" data-target="chart_volt">Voltage (V)</button>
          <button class="btn xml-btn" onclick="exportXML(${m.modbus_id}, 'volt')">⬇ XML Export</button>
        </div>
        <canvas id="chart_volt" class="detail-chart"></canvas>

        <div class="chart-header" style="display:flex; justify-content:space-between;">
          <button class="btn ghost chart-toggle" data-target="chart_amp">Current (A)</button>
          <button class="btn xml-btn" onclick="exportXML(${m.modbus_id}, 'amp')">⬇ XML Export</button>
        </div>
        <canvas id="chart_amp" class="detail-chart"></canvas>

        <div class="chart-header" style="display:flex; justify-content:space-between;">
          <button class="btn ghost chart-toggle" data-target="chart_watt">Power (W)</button>
          <button class="btn xml-btn" onclick="exportXML(${m.modbus_id}, 'watt')">⬇ XML Export</button>
        </div>
        <canvas id="chart_watt" class="detail-chart"></canvas>
      </div>
    </div>
  `;
  moduleDetailCard.appendChild(html);

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

  const commonOptions = { animation: false, maintainAspectRatio: false };

  chartsDetail[m.modbus_id]['chart_volt'] = new Chart(document.getElementById('chart_volt'), {
    type: 'line',
    data: { labels: [...labels], datasets: [{ label: 'V', data: [...dataV], borderColor: '#0066ff', tension: 0.3 }] },
    options: commonOptions
  });

  chartsDetail[m.modbus_id]['chart_amp'] = new Chart(document.getElementById('chart_amp'), {
    type: 'line',
    data: { labels: [...labels], datasets: [{ label: 'A', data: [...dataA], borderColor: '#00a651', tension: 0.3 }] },
    options: { ...commonOptions, scales: { y: { beginAtZero: true } } }
  });

  chartsDetail[m.modbus_id]['chart_watt'] = new Chart(document.getElementById('chart_watt'), {
    type: 'line',
    data: { labels: [...labels], datasets: [{ label: 'W', data: [...dataW], borderColor: '#d64545', tension: 0.3 }] },
    options: { ...commonOptions, scales: { y: { beginAtZero: true } } }
  });
}

function updateDetailMetrics(m) {
  const vEl = document.getElementById('detail_volt');
  if (vEl) vEl.textContent = `V: ${typeof m.voltage === 'number' ? m.voltage.toFixed(2) : '--'} V`;
  const aEl = document.getElementById('detail_amp');
  if (aEl) aEl.textContent = `A: ${typeof m.current === 'number' ? m.current.toFixed(2) : '--'} A`;
  const wEl = document.getElementById('detail_watt');
  if (wEl) wEl.textContent = `W: ${typeof m.power === 'number' ? m.power.toFixed(2) : '--'} W`;
}

function addDetailPoint(id, t, v, a, p) {
  const chs = chartsDetail[id];
  if (!chs) return;

  const updateChart = (chart, val) => {
    if (!chart) return;
    chart.data.labels.push(t);
    chart.data.datasets[0].data.push(val);
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

// --- updateSmallChartData és toggleRelay változatlan maradhat, ha az a korábbi fájlban már jól működött ---
function updateSmallChartData(m) {
  const ch = chartsSmall[m.modbus_id];
  if (!ch) return;

  const value = m.voltage ?? 0;
  const t = new Date().toLocaleTimeString();

  ch.data.labels.push(t);
  ch.data.datasets[0].data.push(value);

  if (ch.data.labels.length > 40) {
    ch.data.labels.shift();
    ch.data.datasets[0].data.shift();
  }

  ch.update('none');
}

let toggleLock = {};

async function toggleRelay(modId, relayIdx, btn = null) {
  const key = `${modId}_${relayIdx}`;
  if (toggleLock[key]) return;
  toggleLock[key] = true;
  setTimeout(() => toggleLock[key] = false, 600);

  if (btn) { btn.disabled = true; }

  try {
    const res = await fetch(`/api/toggle?mod=${modId}&relay=${relayIdx}`, { method: 'GET' });
    if (!res.ok) {
      await fetch(`/api/module/${modId}/relay/${relayIdx}/toggle`);
    }
    setTimeout(() => typeof fetchOnce === 'function' ? fetchOnce() : null, 150);
  } catch (e) {
    console.error('toggle error', e);
  } finally {
    if (btn) btn.disabled = false;
  }
}