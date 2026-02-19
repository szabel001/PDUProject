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

// Initialize WebSocket connection
function initWebSocket() {
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

// Update modules array (replace)
function updateAllModules(arr) {
  arr.forEach(m => upsertModule(m));
  renderDashboard();
}

// Insert/update a single module
function upsertModule(m) {
  modulesCache[m.modbus_id] = m;
  // update small chart data
  updateSmallChartData(m);
  // if module page open for this id, update its detail UI
  if (moduleView.classList.contains('active') && currentModuleId === m.modbus_id) {
    renderModuleDetail(m);
  }
}

function updateSmallChartData(m) {
  const ch = chartsSmall[m.modbus_id];
  if (!ch) return;
  const t = new Date().toLocaleTimeString();
  ch.data.labels.push(t);
  ch.data.datasets[0].data.push(typeof m.voltage === 'number' ? m.voltage : null);
  if (ch.data.labels.length > 30) {
    ch.data.labels.shift(); ch.data.datasets[0].data.shift();
  }
  ch.update('none');
}


// ---------------- DASHBOARD RENDER ----------------
function renderDashboard() {
  modulesGrid.innerHTML = '';
  Object.values(modulesCache).forEach(m => {
    const card = document.createElement('div');
    card.className = 'card';
    card.dataset.mod = m.modbus_id;

    const relayLabel = (Array.isArray(m.relays) && m.relays.length > 0) ? (m.relays[0] ? 'ON' : 'OFF') : 'N/A';
    const relayClass = (Array.isArray(m.relays) && m.relays.length > 0 && m.relays[0]) ? 'relay-on' : 'relay-off';

    card.innerHTML = `
      <div class="top">
        <div>
          <h2>Modul #${m.modbus_id} <span class="small">(${m.version || 'v?'})</span></h2>
          <div class="meta small">ID: ${m.id} • Relays: ${m.relay_count}</div>
        </div>
        <div>
          <button class="btn ghost" data-action="view" data-mod="${m.modbus_id}">Megnyit</button>
          <button class="btn" data-action="toggle" data-mod="${m.modbus_id}" data-rel="0">Relay: <span class="${relayClass}">${relayLabel}</span></button>
        </div>
      </div>
      <div class="values">
        <div class="value">V ${typeof m.voltage === 'number' ? m.voltage.toFixed(2) : '--'}</div>
        <div class="value">A ${typeof m.current === 'number' ? m.current.toFixed(2) : '--'}</div>
        <div class="value">W ${typeof m.power === 'number' ? m.power.toFixed(2) : '--'}</div>
      </div>
      <div style="margin-top:8px">
        <canvas id="mini_${m.modbus_id}" class="smallchart"></canvas>
      </div>
    `;

    // attach listeners
    card.querySelectorAll('[data-action]').forEach(btn => {
      btn.addEventListener('click', (ev) => {
        const act = btn.dataset.action;
        const mod = parseInt(btn.dataset.mod, 10);
        const rel = parseInt(btn.dataset.rel || '0', 10);
        if (act === 'view') {
          history.pushState({}, '', `#module-${mod}`);
          openModulePage(mod);
        } else if (act === 'toggle') {
          toggleRelay(mod, rel, btn);
        }
      });
    });

    modulesGrid.appendChild(card);

    // init mini chart if needed
    if (!chartsSmall[m.modbus_id]) {
      initMiniChart(m.modbus_id);
    }
  });
}

// Small chart helpers
function initMiniChart(modId) {
  const ctx = document.getElementById(`mini_${modId}`);
  if (!ctx) return;
  const chart = new Chart(ctx, {
    type: 'line',
    data: {
      labels: [], datasets: [
        { label: 'V', data: [], tension: 0.3, borderWidth: 1, pointRadius: 0 }
      ]
    },
    options: { animation: false, plugins: { legend: { display: false } }, scales: { x: { display: false }, y: { display: false } } }
  });
  chartsSmall[modId] = chart;
}

// ---------------- MODULE DETAIL PAGE ----------------
let currentModuleId = null;

function openModulePage(id) {
  const m = modulesCache[id];
  if (!m) {
    fetchOnce(); // fallback
    return;
  }
  dashboard.style.display = 'none';
  moduleView.classList.add('active');
  moduleView.scrollIntoView({ behavior: 'smooth' });
  currentModuleId = id;
  renderModuleDetail(m);
}

function showDashboard() {
  moduleView.classList.remove('active');
  dashboard.style.display = '';
  currentModuleId = null;
}

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

// Ensure dashboard clickable modules open detail on direct hash navigation
if (location.hash.startsWith('#module-')) {
  const id = parseInt(location.hash.replace('#module-', ''), 10);
  setTimeout(() => openModulePage(id), 300);
}