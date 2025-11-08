#include "PDU_webserver.h"
#include "IECControl.h"

PDU_webserver::PDU_webserver(AsyncWebServer *server, IECControl *iecModule) {
  webServer = server; // Initialize the web server on port 80
  module = iecModule;
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount hiba!");
    return;
  }
}

void PDU_webserver::runServer() {
        
  Serial.print("IP address: ");
  Serial.println(ETH.localIP());

    webServer->on("/api/data", HTTP_GET, [this](AsyncWebServerRequest *request){
    uint8_t id = 1;
    String json = "{";
    json += "\"modbus_id\":1,";
    json += "\"id\":" +                   String(this->module->getIECID(id)) + ",";
    json += "\"voltage\":" +              String(this->module->getVoltageData(id)) + ",";
    json += "\"current\":" +              String(this->module->getCurrentData(id)) + ",";
    json += "\"power\":" +                String(this->module->getPowerData(id)) + ",";
    json += "\"version\":\"" +            String(this->module->getIECVersion(id)) + "\",";
    json += "\"available_leds\":" +       String(0) + ",";
    json += "\"current_limit\":" +        String(this->module->getIECCurrentLimit(id)) + ",";
    json += "\"relay_count\":" +          String(this->module->getIECRelayCount(id)) + ",";
    json += "\"is_current_measured\":" +  String(this->module->getIEC_IS_CURRENT_MEASURED(id) ? "true" : "false") + ",";
    json += "\"is_voltage_measured\":" +  String(this->module->getIEC_IS_VOLTAGE_MEASURED(id) ? "true" : "false") + ",";
    json += "\"relays\":[";
    for (int i=0; i<this->module->getIECRelayCount(id); i++) {
      json += (this->module->getIECRelayStatus(id) ? "true" : "false");
      if (i < this->module->getIECRelayCount(id) - 1) json += ",";
    }
    json += "]}";
    request->send(200, "application/json", json);
  });

  // --- Relay toggle API ---
  webServer->on("^\\/api\\/relay\\/([0-9]+)\\/toggle$", HTTP_GET, [this](AsyncWebServerRequest *request){
    uint8_t id = 1;
    String idStr = request->pathArg(0);
    int relayId = idStr.toInt();
    if (relayId >= 0 && relayId < this->module->getIECRelayCount(id)) {
      this->module->setRelayStatus(relayId, !this->module->getIECRelayStatus(id));
    }
    request->send(200, "text/plain", "OK");
  });

  webServer->serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  webServer->begin();
  Serial.println("Webszerver is running!");
}