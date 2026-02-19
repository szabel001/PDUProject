let currentModuleId = null;

// Update modules array (replace)
function updateAllModules(arr) {
  arr.forEach(m => upsertModule(m));
  renderDashboard();
}

function upsertModule(m) {
  modulesCache[m.modbus_id] = m;
  updateSmallChartData(m);
  updateModuleCard(m);

  if (currentModuleId === m.modbus_id) {
    addDetailPoint(m.modbus_id, m.voltage, m.current, m.power);
  }
}


function openModulePage(id) {

  const m = modulesCache[id];
  if (!m) return;

  const dashboard = document.getElementById('dashboard');
  const moduleView = document.getElementById('moduleView');

  dashboard.style.display = 'none';
  moduleView.classList.add('active');

  currentModuleId = id;
  renderModuleDetail(m);
}

function showDashboard() {
  document.getElementById('moduleView').classList.remove('active');
  document.getElementById('dashboard').style.display = '';
  currentModuleId = null;
}


// ---------------- DETAIL RENDER ----------------

function renderModuleDetail(m) {
  moduleDetailCard.innerHTML = '';

  const html = document.createElement('div');
  html.innerHTML = `
    <div style="display:flex;align-items:center;justify-content:space-between;gap:12px;flex-wrap:wrap">
      <div>
        <h2>Modul #${m.modbus_id} <span class="small">(${m.version || 'v?'})</span></h2>
        <div class="small">
          ID: ${m.id} • Relays: ${m.relay_count} • Current limit: ${m.current_limit}A
        </div>
        <div class="small">
          Mérés: ${m.is_current_measured ? 'Áram' : '-'} / ${m.is_voltage_measured ? 'Feszültség' : '-'}
        </div>
      </div>
      <div style="text-align:right">
        <div class="small">V: ${typeof m.voltage === 'number' ? m.voltage.toFixed(2) : '--'} V</div>
        <div class="small">A: ${typeof m.current === 'number' ? m.current.toFixed(2) : '--'} A</div>
        <div class="small">W: ${typeof m.power === 'number' ? m.power.toFixed(2) : '--'} W</div>
      </div>
    </div>

    <div style="margin-top:12px">
      <h3>Grafikonok</h3>
      <div id="chartContainer" style="display:flex;flex-direction:column;gap:8px">
        <button class="btn ghost chart-toggle" data-target="chart_volt">Feszültség (V)</button>
        <canvas id="chart_volt" class="detail-chart"></canvas>

        <button class="btn ghost chart-toggle" data-target="chart_amp">Áram (A)</button>
        <canvas id="chart_amp" class="detail-chart"></canvas>

        <button class="btn ghost chart-toggle" data-target="chart_watt">Teljesítmény (W)</button>
        <canvas id="chart_watt" class="detail-chart"></canvas>
      </div>
    </div>

    <div style="margin-top:12px">
      <h3>Relék</h3>
      <div class="relays" id="relaysContainer"></div>
    </div>
  `;
  moduleDetailCard.appendChild(html);

  // ----------------- Collapsible charts -----------------
  moduleDetailCard.querySelectorAll('.chart-toggle').forEach(btn => {
    const target = moduleDetailCard.querySelector('#' + btn.dataset.target);
    target.style.display = 'none'; // initially hidden
    btn.onclick = () => {
      if (target.style.display === 'none') {
        target.style.display = 'block';
        // Force Chart.js resize
        const ch = chartsDetail[m.modbus_id][btn.dataset.target];
        if (ch) ch.resize();
      } else {
        target.style.display = 'none';
      }
    };
  });

  // create or update charts
  if (!chartsDetail[m.modbus_id]) {
    chartsDetail[m.modbus_id] = {};
    chartsDetail[m.modbus_id]['chart_volt'] = new Chart(document.getElementById('chart_volt'), {
      type: 'line',
      data: { labels: [], datasets: [{ label: 'Feszültség (V)', data: [], borderColor: '#0066ff', backgroundColor: 'rgba(0,102,255,0.2)', tension: 0.3 }] },
      options: { animation: true, maintainAspectRatio: false, scales: { y: { beginAtZero: true } } }
    });
    chartsDetail[m.modbus_id]['chart_amp'] = new Chart(document.getElementById('chart_amp'), {
      type: 'line',
      data: { labels: [], datasets: [{ label: 'Áram (A)', data: [], borderColor: '#00a651', backgroundColor: 'rgba(0,166,81,0.2)', tension: 0.3 }] },
      options: { animation: false, maintainAspectRatio: false, scales: { y: { beginAtZero: true } } }
    });
    chartsDetail[m.modbus_id]['chart_watt'] = new Chart(document.getElementById('chart_watt'), {
      type: 'line',
      data: { labels: [], datasets: [{ label: 'Teljesítmény (W)', data: [], borderColor: '#d64545', backgroundColor: 'rgba(214,69,69,0.2)', tension: 0.3 }] },
      options: { animation: false, maintainAspectRatio: false, scales: { y: { beginAtZero: true } } }
    });
  }

  // render relays
  const rc = document.getElementById('relaysContainer');
  rc.innerHTML = '';
  if (Array.isArray(m.relays)) {
    m.relays.forEach((st, idx) => {
      const b = document.createElement('button');
      b.className = 'relay-btn ' + (st ? 'relay-on' : 'relay-off');
      b.textContent = `Relay ${idx}: ${st ? 'ON' : 'OFF'}`;
      b.onclick = () => toggleRelay(m.modbus_id, idx, b);
      rc.appendChild(b);
    });
  } else {
    rc.innerHTML = '<div class="small">Nincsenek relay állapotok</div>';
  }

  // seed charts with current value
  addDetailPoint(m.modbus_id, m.voltage, m.current, m.power);
}

// add new point to charts
function addDetailPoint(id, v, a, p) {
  const chs = chartsDetail[id];
  if (!chs) return;
  const t = new Date().toLocaleTimeString();

  const updateChart = (chart, val) => {
    chart.data.labels.push(t);
    chart.data.datasets[0].data.push(val);
    if (chart.data.labels.length > MAX_POINTS) {
      chart.data.labels.shift();
      chart.data.datasets[0].data.shift();
    }
    chart.update('none');
  };

  updateChart(chs['chart_volt'], v);
  updateChart(chs['chart_amp'], a);
  updateChart(chs['chart_watt'], p);
}

// -------- DETAIL CHARTS --------

function initDetailCharts(id) {

  chartsDetail[id] = {};

  ['volt','amp','watt'].forEach(type => {

    const ctx = document.getElementById(`chart_${type}`);

    chartsDetail[id][type] = new Chart(ctx, {
      type: 'line',
      data: {
        labels: [],
        datasets: [{
          data: [],
          tension: 0.45,
          borderWidth: 2,
          pointRadius: 0,
          fill: true
        }]
      },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        animation: {
          duration: 400,
          easing: 'easeOutQuart'
        },
        plugins: { legend: { display: false } }
      }
    });

  });
}


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

  ch.update();
}

// ---------------- TOGGLE RELAY ----------------
let toggleLock = {}; // per-modul debounce lock

async function toggleRelay(modId, relayIdx, btn = null) {
  // debounce lock
  const key = `${modId}_${relayIdx}`;
  if (toggleLock[key]) return;
  toggleLock[key] = true;
  setTimeout(() => toggleLock[key] = false, 600);

  if (btn) { btn.disabled = true; }

  try {
    // recommended endpoint (robosztus): /api/toggle?mod=<>&relay=<>
    const res = await fetch(`/api/toggle?mod=${modId}&relay=${relayIdx}`, { method: 'GET' });
    if (!res.ok) {
      // try path fallback if query fails
      await fetch(`/api/module/${modId}/relay/${relayIdx}/toggle`);
    }
    // optimistic: fetch updated data (or rely on WS push)
    setTimeout(() => fetchOnce(), 150);
  } catch (e) {
    console.error('toggle error', e);
    alert('Relay váltás hiba');
  } finally {
    if (btn) btn.disabled = false;
  }
}