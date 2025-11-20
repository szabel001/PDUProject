#include "PDU_webserver.h"

PDU_webserver::PDU_webserver(AsyncWebServer *server, IECControl *iecControl) {
  webServer = server;
  iec = iecControl;
  toggleStatus = iec->getIECRelayStatus(1);

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount hiba!");
    // tovább futunk, de a static fájlok nem lesznek elérhetők
  }
}

bool PDU_webserver::hasModuleId(uint8_t id) {
  for (auto m : iec->getFoundIECIDs()) if (m == id) return true;
  return false;
}

void PDU_webserver::setRelayStatusWeb(uint8_t id, bool status) {
  // Példa implementáció - valós implementáció az IECControl osztályban van
  Serial.println("Relay " + String(id) + " status set to: " + String(status));
  iec->setRelayStatus(1, status);
}

void PDU_webserver::runServer() {
  // --- API: összes modul adatai tömbként ---
  webServer->on("/api/data", HTTP_GET, [this](AsyncWebServerRequest *request){
    String json = "[";
    for (auto id: this->iec->getFoundIECIDs()) {
      json += "{";
      json += "\"modbus_id\":" + String(id) + ",";
      json += "\"id\":" + String(this->iec->getIECID(id)) + ",";
      json += "\"voltage\":" + String(this->iec->getVoltageData(id)) + ",";
      json += "\"current\":" + String(this->iec->getCurrentData(id)) + ",";
      json += "\"power\":" + String(this->iec->getPowerData(id)) + ",";
      json += "\"version\":\"" + String(this->iec->getIECVersion(id)) + "\",";
      json += "\"available_leds\":0,";
      json += "\"current_limit\":" + String(this->iec->getIECCurrentLimit(id)) + ",";
      json += "\"relay_count\":" + String(this->iec->getIECRelayCount(id)) + ",";
      json += "\"is_current_measured\":" + String(this->iec->getIEC_IS_CURRENT_MEASURED(id) ? "true" : "false") + ",";
      json += "\"is_voltage_measured\":" + String(this->iec->getIEC_IS_VOLTAGE_MEASURED(id) ? "true" : "false") + ",";
      // relays tömb
      json += "\"relays\":[";
      int rcount = 1;//this->iec->getIECRelayCount(id);
      for (int r = 0; r < rcount; r++) {
        bool st = this->iec->getIECRelayStatus(id);
        json += (st ? "true" : "false");
        if (r < rcount - 1) json += ",";
      }
      json += "]";
      json += "}";
      //json += ",";
    }
    json += "]";

    request->send(200, "application/json", json);
  });

  // --- Relay toggle endpoint ---
  // Útvonal: /api/module/<modid>/relay/<relayid>/toggle
  webServer->on("^\/api\/module\/([0-9]+)\/relay\/([0-9]+)\/toggle$", HTTP_GET,
    [this](AsyncWebServerRequest *request){
    // pathArg(0) = module id, pathArg(1) = relay id;
    
    // DEBUG: írd ki az URL-t a Serial-ra (segít megtalálni, ha mégis rossz)
    Serial.print("Toggle request URL: ");
    Serial.println(request->url());

    // Kinyerés sscanf-pel: megbízható, egyszerű
    const String url = request->url();
    int modId = -1;
    int relayId = -1;
    if (sscanf(url.c_str(), "/api/module/%d/relay/%d/toggle", &modId, &relayId) != 2) {
      Serial.println("Failed to parse module/relay from URL");
      request->send(400, "text/plain", "Bad request format");
      return;
    }

    if (!this->hasModuleId(modId)) {
      request->send(400, "text/plain", "Invalid module id" + String(modId));
      return;
    }
    // int relayCount = this->iec->getIECRelayCount(modId);
    // if (relayId >= relayCount) {
    //   request->send(400, "text/plain", "Invalid relay id");
    //   return;
    // }
    this->setRelayStatusWeb(modId, !this->iec->getIECRelayStatus(modId));
    request->send(200, "text/plain", "OK");
  });

  // --- Static fájlok kiszolgálása SPIFFS-ből ---
  webServer->serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  webServer->begin();
  Serial.println("Webserver is running!");
}
