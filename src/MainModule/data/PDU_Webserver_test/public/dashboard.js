// ---------------- DASHBOARD RENDER ----------------

function renderDashboard() {

  const grid = document.getElementById('modulesGrid');
  if (!grid) return;

  grid.innerHTML = '';

  Object.values(modulesCache).forEach(m => {

    const card = document.createElement('div');
    card.className = 'card modern-card';
    card.setAttribute("data-mod", m.modbus_id);

    const relayState = m.relays?.[0] ? 'ON' : 'OFF';
    const relayClass = m.relays?.[0] ? 'relay-on' : 'relay-off';

    card.innerHTML = `
      <div class="card-header">
        <div>
          <h2>Module #${m.modbus_id}</h2>
          <div class="small">FW ${m.version || '—'}</div>
        </div>
        <div class="badge">${relayState}</div>
      </div>

      <div class="metrics">
        <div><span>Voltage</span><strong class="voltage">...</strong></div>
        <div><span>Current</span><strong class="current">...</strong></div>
        <div><span>Power</span><strong class="power">...</strong></div>
      </div>

      <canvas id="mini_${m.modbus_id}" class="mini-chart"></canvas>

      <div class="card-actions">
        <button class="btn ghost viewBtn">Details</button>
        <button class="btn toggleBtn ${relayClass}">${relayState}</button>
      </div>
    `;

    grid.appendChild(card);

    // Listeners
    card.querySelector('.viewBtn').onclick = () => openModulePage(m.modbus_id);
    card.querySelector('.toggleBtn').onclick = (e) =>
      toggleRelay(m.modbus_id, 0, e.target);

    initMiniChart(m.modbus_id);
  });
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
        tension: 0.45,
        borderWidth: 2,
        pointRadius: 0,
        fill: true
      }]
    },
    options: {
      responsive: true,
      animation: {
        duration: 300,
        easing: 'easeOutCubic'
      },
      plugins: { legend: { display: false } },
      scales: {
        x: { display: false },
        y: { display: false }
      }
    }
  });
}

function updateModuleCard(m) {
  const card = document.querySelector(`[data-mod="${m.modbus_id}"]`);
  if (!card) return;

  card.querySelector('.voltage').textContent =
    typeof m.voltage === 'number' ? m.voltage.toFixed(2) + ' V' : '--';

  card.querySelector('.current').textContent =
    typeof m.current === 'number' ? m.current.toFixed(2) + ' A' : '--';

  card.querySelector('.power').textContent =
    typeof m.power === 'number' ? m.power.toFixed(2) + ' W' : '--';
}

