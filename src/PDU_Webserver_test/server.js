const express = require("express");
const http = require("http");
const WebSocket = require("ws");

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

// ----------------------
// Fake module database
// ----------------------

let modules = [
  {
    modbus_id: 1,
    id: "IEC-1",
    voltage: 230,
    current: 1.2,
    power: 276,
    relays: [true, false],
    relay_count: 2,
    version: "1.0",
    current_limit: 10,
    is_current_measured: true,
    is_voltage_measured: true
  },
  {
    modbus_id: 2,
    id: "IEC-2",
    voltage: 231,
    current: 0.7,
    power: 161,
    relays: [false],
    relay_count: 1,
    version: "1.1",
    current_limit: 5,
    is_current_measured: true,
    is_voltage_measured: true
  },
    {
    modbus_id: 3,
    id: "IEC-2",
    voltage: 231,
    current: 0.7,
    power: 161,
    relays: [false],
    relay_count: 1,
    version: "1.1",
    current_limit: 5,
    is_current_measured: true,
    is_voltage_measured: true
  },
    {
    modbus_id: 4,
    id: "IEC-2",
    voltage: 231,
    current: 0.7,
    power: 161,
    relays: [false],
    relay_count: 1,
    version: "1.1",
    current_limit: 5,
    is_current_measured: true,
    is_voltage_measured: true
  },
    {
    modbus_id: 5,
    id: "IEC-2",
    voltage: 231,
    current: 0.7,
    power: 161,
    relays: [false],
    relay_count: 1,
    version: "1.1",
    current_limit: 5,
    is_current_measured: true,
    is_voltage_measured: true
  },
    {
    modbus_id: 6,
    id: "IEC-2",
    voltage: 231,
    current: 0.7,
    power: 161,
    relays: [false],
    relay_count: 1,
    version: "1.1",
    current_limit: 5,
    is_current_measured: true,
    is_voltage_measured: true
  },
    {
    modbus_id: 7,
    id: "IEC-2",
    voltage: 231,
    current: 0.7,
    power: 161,
    relays: [false],
    relay_count: 1,
    version: "1.1",
    current_limit: 5,
    is_current_measured: true,
    is_voltage_measured: true
  },
    {
    modbus_id: 8,
    id: "IEC-2",
    voltage: 231,
    current: 0.7,
    power: 161,
    relays: [false],
    relay_count: 1,
    version: "1.1",
    current_limit: 5,
    is_current_measured: true,
    is_voltage_measured: true
  }
];

// ----------------------
// Helpers
// ----------------------

function randomizeMeasurements() {
  modules.forEach(m => {
    m.voltage = 225 + Math.random() * 10;
    m.current = Math.random() * 5;
    m.power = m.voltage * m.current;
  });
}

// ----------------------
// REST API
// ----------------------

app.use(express.static("public"));
app.use(express.json()); // JSON formátumú kérések (POST) feldolgozásához

// Fake beállítások adatbázis
let currentSettings = {
  pdu: {
    ip: "192.168.1.100",
    gateway: "192.168.1.1",
    maxCurrent: 16,
    maxPower: 3500
  },
  iec: {
    baudRate: 9600,
    parity: "None"
  }
};

app.get("/api/data", (req, res) => {
  res.json(modules);
});

// Beállítások lekérése
app.get("/api/settings", (req, res) => {
  res.json(currentSettings);
});

// Beállítások mentése
app.post("/api/settings", (req, res) => {
  const newSettings = req.body;
  
  // Részleges frissítés (deep merge egyszerűsítve)
  if (newSettings.pdu) currentSettings.pdu = { ...currentSettings.pdu, ...newSettings.pdu };
  if (newSettings.iec) currentSettings.iec = { ...currentSettings.iec, ...newSettings.iec };
  
  console.log("Új beállítások mentve:", currentSettings);
  res.json({ ok: true, message: "Settings saved successfully" });
});

app.get("/api/toggle", (req, res) => {
  const mod = Number(req.query.mod);
  const relay = Number(req.query.relay);

  const m = modules.find(x => x.modbus_id === mod);
  if (!m) return res.status(404).send("Module not found");

  m.relays[relay] = !m.relays[relay];
  res.json({ ok: true });
});

// ----------------------
// WebSocket
// ----------------------

wss.on("connection", ws => {
  console.log("Client connected");

  ws.send(JSON.stringify({
    type: "full",
    modules
  }));
});

function broadcastUpdate() {
  const msg = JSON.stringify({
    type: "update",
    modules
  });

  wss.clients.forEach(client => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(msg);
    }
  });
}

// ----------------------
// Periodic update
// ----------------------

setInterval(() => {
  randomizeMeasurements();
  broadcastUpdate();
}, 1000);

// ----------------------

const PORT = 3000;
server.listen(PORT, () => {
  console.log(`Mock server running: http://localhost:${PORT}`);
});
