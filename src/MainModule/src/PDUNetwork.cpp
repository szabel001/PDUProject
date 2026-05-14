#include <PDUNetwork.h>

//==========================================================//
//==================== Modbus parameters ===================//
//==========================================================//
#define PDU_ID_ADDR 0
#define PDU_DISCOVER_PHASE_STATUS_ADDR 0
#define MASTER_EXISTING_ADDR 1
#define BROADCAST_ADDR 0

const String errorStrings[] = {
  "success",
  "invalid id",
  "invalid buffer",
  "invalid quantity",
  "response timeout",
  "frame error",
  "crc error",
  "unknown comm error",
  "unexpected id",
  "exception response",
  "unexpected function code",
  "unexpected response length",
  "unexpected byte count",
  "unexpected address",
  "unexpected value",
  "unexpected quantity"
};
//==========================================================//
//=============== Constructor, init, loop ==================//
//==========================================================//

const uint8_t numCoils = 2;
bool coils[numCoils];

PDUNetwork::PDUNetwork(IECControl* iec, networkLayerManager* nwLayer, HardwareSerial& PDU_RS485Serial) : 
_mbMaster(PDU_RS485Serial, RS485_PDU_DE_RE_PIN, RS485_PDU_DE_RE_PIN), _mbSlave(PDU_RS485Serial, RS485_PDU_DE_RE_PIN, RS485_PDU_DE_RE_PIN) {
    PDU_RS485Serial.begin(MODBUS_BAUD, SERIAL_8N1, RS485_PDU_RX_GPIO, RS485_PDU_TX_GPIO);
    _testState = false;
    digitalWrite(RS485_PDU_DE_RE_PIN, LOW);
    _iec = iec;
    _nwLayerManager = nwLayer;
    _nextSlaveID = 1;
    _maxSlaveId = 50;
}

void PDUNetwork::discoverPDUs(int availablePDUCount){
    using namespace std::placeholders;

    const char* ssid = "PDUProject-Setup";
    const char* password = "pDumAsTeR_20250511";
    uint8_t defaultIP[4] = {10, 0, 0, 2};
    uint8_t defaultGW[4] = {10, 0, 0, 1};
    uint8_t defaultMask[4] = {255, 255, 255, 0};

    _nwLayerManager->WiFi.softAP(ssid, password, 1, 0, 10, false);

    _nwLayerManager->webServer->on("/get_id", HTTP_GET,  std::bind(&PDUNetwork::handleGetID, this, _1));    // HTTP GET /get_id?uid=...
    
    _nwLayerManager->webServer->begin();
    Serial.println("Async HTTP server is for PDU module discovery.");
    Serial.println("Searching for slaves...");
    while (_assignedIDs.size() < availablePDUCount) {
    }
    Serial.println("Every slave PDU has a modbus ID now!");

    _nwLayerManager->setWiFiSTA_Status(false);
    _nwLayerManager->webServer->end();
    Serial.println("Async HTTP server stopped.");
}

void PDUNetwork::handleGetID(AsyncWebServerRequest *request) {
    if (!request->hasParam("uid")) {
        request->send(400, "application/json", "{\"error\":\"Missing UID\"}");
        return;
    }
    String uid = request->getParam("uid")->value();
    Serial.printf("Received UID: %s\n", uid.c_str());
    int slaveId;
    if (_assignedIDs.find(uid) != _assignedIDs.end()) {
        slaveId = _assignedIDs[uid];
    } else {
        if (_nextSlaveID > _maxSlaveId) {
            Serial.println(_nextSlaveID);
            Serial.println(_maxSlaveId);
            request->send(500, "application/json", "{\"error\":\"Too many slaves requested for modbus ID! Maximum is 243\"}");
            return;
        }
        slaveId = _nextSlaveID++;
        _assignedIDs[uid] = slaveId;
        Serial.printf("Chosen slave ID: %d\n", slaveId);
    }
    String response = "{ \"slave_id\": " + String(slaveId) + " }";
    request->send(200, "application/json", response);
}

void PDUNetwork::requestNetworkID() {
    const char* ssid = "PDUProject-Setup";
    const char* password = "pDumAsTeR_20250511";
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    String macStr = "";
    for (int i = 0; i < 6; ++i) {
        if (mac[i] < 0x10) macStr += "0";  // leading 0, if needed
            macStr += String(mac[i], HEX);
    }
    macStr.toUpperCase();
    String myUID = macStr;
    int mySlaveId = -1;
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to Wi-Fi");

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWi-Fi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP().toString());

    HTTPClient http;
    Serial.print("myUID:    ");
    Serial.println(myUID);
    String url = "http://192.168.4.1:80/get_id?uid=" + myUID;
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == 200) {
        String payload = http.getString();
        Serial.print("Szerver válasza: ");
        Serial.println(payload);

        int pos = payload.indexOf("slave_id");
        if (pos != -1) {
        int start = payload.indexOf(":", pos) + 1;
        int end = payload.indexOf("}", start);
        String idStr = payload.substring(start, end);
        mySlaveId = idStr.toInt();
        Serial.printf("Given slave ID: %d\n", mySlaveId);
        }
    } else {
        Serial.printf("HTTP error: %d\n", httpCode);
    }
    http.end();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    Serial.println("Wi-Fi shut off, ready for RS485 communication.");
}

void PDUNetwork::setSlaveStatus() {
    if (false) { //_nwLayerManager->getNetworkStatus()){
        _mbMaster.begin(MODBUS_BAUD);
        _mbMaster.setTimeout(10);
        _isSlave = false;
        Serial.println("Modbus master started.");
    } else {
        _mbSlave.configureCoils(coils, numCoils);
        _mbSlave.begin(5, MODBUS_BAUD, SERIAL_8N1);
        _isSlave = true;
        Serial.println("Modbus slave started.");
    }
}

void PDUNetwork::sendInitCommand(){
}

void PDUNetwork::sendEndOfInitCommand(){
}

//==========================================================//
//==================== only for testing ====================//
//==========================================================//
void PDUNetwork::PDUNetworkLoop() {
    delay(1000);
    if (_isSlave) {
        _mbSlave.poll();
        setExtRelayStatus(7, coils[0]);
        Serial.print(F("Slave's Relay status: "));
        Serial.println(coils[0] ? "ON" : "OFF");
    } else {
        _testState = !_testState;
        _mbMaster.writeSingleCoil(5, 0, _testState);
        Serial.print(F("Master send Relay status: "));
        Serial.println(_testState ? "ON" : "OFF");
    }
}

bool PDUNetwork::setExtRelayStatus(uint8_t id, bool status) {
    _iec->setRelayStatus(id, status); // Set the relay status for the given slave ID
    return true;
}

void sendConfigData(uint8_t id, configData config) {
    
}

void sendMeasurementData(uint8_t id, MeasurementData data) {
    // Implement the logic to send measurement data to the slave device with the given ID
}

void handleReceivedIECSetting(uint8_t id, String command) {
    // Implement the logic to handle received commands from the slave device with the given ID
}

void handleReceivedPDUSetting(uint8_t id, String command) {
    // Implement the logic to handle received commands from the master device with the given ID
}