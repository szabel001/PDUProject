// ---------------- DASHBOARD RENDER ----------------

// ---------------- DASHBOARD RENDER (Optimalizált) ----------------

function renderDashboard() {
  const grid = document.getElementById('modulesGrid');
  if (!grid) return;

  Object.values(modulesCache).forEach(m => {
    // Ellenőrizzük, létezik-e már a kártya
    let card = document.querySelector(`[data-mod="${m.modbus_id}"]`);
    
    if (!card) {
      // Ha még nincs, létrehozzuk a DOM elemet (csak egyszer fut le modulonként)
      card = document.createElement('div');
      card.className = 'card modern-card';
      card.setAttribute("data-mod", m.modbus_id);

      card.innerHTML = `
        <div class="card-header">
          <div>
            <h2>Module #${m.modbus_id}</h2>
            <div class="small">Firmware: ${m.version || '—'}</div>
            <div class="small" style="font-weight: 600; margin-top: 2px;">
              Status: <span class="mod-status" style="color: ${getStatusColor(m.status)};">${getStatusText(m.status)}</span>
            </div>
          </div>
        </div>

        <div class="metrics">
          <div><span>RMS Voltage</span><strong class="voltage">...</strong></div>
          <div><span>RMS Current</span><strong class="current">...</strong></div>
          <div><span>Apparent Power</span><strong class="power">...</strong></div>
        </div>

        <div class="mini-chart-container">
          <canvas id="mini_${m.modbus_id}" class="mini-chart"></canvas>
          <div style="position: absolute; top: 15px; left: 15px; font-size: 10px; color: var(--muted); z-index: 1;">
            Current Trend (A)
          </div>
        </div>

        <div class="card-actions">
          <button class="btn ghost viewBtn">Details</button>
          <button class="btn ghost viewBtn">Settings</button>
          <button class="btn toggleBtn">...</button>
        </div>
      `;

      grid.appendChild(card);

      // Listenerek hozzáfűzése a gombokhoz
      //card.querySelector('.viewBtn').onclick = () => openModulePage(m.modbus_id);
      card.querySelector('.viewBtn:nth-child(1)').onclick = () => openModulePage(m.modbus_id); // Details
      card.querySelector('.viewBtn:nth-child(2)').onclick = () => openIecSettings(m.modbus_id); // Settings gomb
      card.querySelector('.toggleBtn').onclick = (e) => toggleRelay(m.modbus_id, 0, e.target);

      // Csak az első alkalommal inicializáljuk a grafikont
      initMiniChart(m.modbus_id);
    }
  });
}

function openIecSettings(id) {
    const m = modulesCache[id];
    const detailCard = document.getElementById('moduleDetailCard');
    document.getElementById('dashboard').style.display = 'none';
    document.getElementById('moduleView').classList.add('active');
    
    detailCard.innerHTML = `
        <h2>IEC Settings - Module #${id}</h2>
        <div class="settings-card">
            <label>Warning Current Limit (A)</label>
            <input type="number" id="iec_warn_limit" value="${m.curr_warning !== undefined ? m.curr_warning : ''}">
            
            <label>Error Current Limit (A)</label>
            <input type="number" id="iec_err_limit" value="${m.curr_error !== undefined ? m.curr_error : ''}">
            
            <label>Measuring Average Points</label>
            <input type="number" id="iec_avg_num" value="${m.meas_avg_num !== undefined ? m.meas_avg_num : ''}">
            
            <button class="btn" onclick="saveIecModuleSettings(${id})">Save IEC Settings</button>
        </div>
    `;
}

// Az updateModuleCard cseréli a szöveget anélkül, hogy Canvas/DOM újragenerálást végezne
function updateModuleCard(m) {
  const card = document.querySelector(`[data-mod="${m.modbus_id}"]`);
  if (!card) return;

  card.querySelector('.voltage').textContent = typeof m.voltage === 'number' ? m.voltage.toFixed(2) + ' V' : '--';
  card.querySelector('.current').textContent = typeof m.current === 'number' ? m.current.toFixed(2) + ' A' : '--';
  card.querySelector('.power').textContent = typeof m.power === 'number' ? m.power.toFixed(2) + ' W' : '--';

  // Relé gomb és badge frissítése
  const relayState = m.relays?.[0] ? 'ON' : 'OFF';
  const isRelayOn = m.relays?.[0];
  
  const toggleBtn = card.querySelector('.toggleBtn');
  toggleBtn.textContent = relayState;
  toggleBtn.className = `btn toggleBtn ${isRelayOn ? 'relay-on' : 'relay-off'}`;

  // Status frissítése valós időben
  const statusSpan = card.querySelector('.mod-status');
  if (statusSpan) {
    statusSpan.textContent = getStatusText(m.status);
    statusSpan.style.color = getStatusColor(m.status);
  }
}

// -------- MINI CHART --------

function initMiniChart(modId) {

  const ctx = document.getElementById(`mini_${modId}`);
  if (!ctx) return;

  if (chartsSmall[modId]) return;

  chartsSmall[modId] = new Chart(ctx, {
    type: 'line',
    data: {
      labels: [],
      datasets: [{
        data: [],
        tension: 0.5,
        borderWidth: 1.5,
        borderColor: '#00a651', // Áramhoz illő zöld szín
        backgroundColor: 'rgba(0, 166, 81, 0.1)',
        pointRadius: 0,
        fill: true
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false, // Engedi, hogy kitöltse a rendelkezésre álló magasságot
      animation: {
        duration: 300,
        easing: 'easeOutCubic'
      },
      plugins: { legend: { display: false } },
      scales: {
        x: { display: false },
        y: { display: false,
              beginAtZero: true,}
      }
    }
  });
}

// --- updateSmallChartData és toggleRelay változatlan maradhat, ha az a korábbi fájlban már jól működött ---
function updateSmallChartData(m) {
  const ch = chartsSmall[m.modbus_id];
  if (!ch) return;

  const value = m.current ?? 0;
  const t = new Date().toLocaleTimeString();

  ch.data.labels.push(t);
  ch.data.datasets[0].data.push(value);

  if (ch.data.labels.length > 40) {
    ch.data.labels.shift();
    ch.data.datasets[0].data.shift();
  }

  ch.update('none');
}