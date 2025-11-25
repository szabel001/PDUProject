#include "PDU_webserver.h"

PDU_webserver::PDU_webserver(AsyncWebServer *server, IECControl *iecControl)
: webServer(server), iec(iecControl), ws("/ws")
{
    if(!SPIFFS.begin(true)) {
        Serial.println("SPIFFS mount hiba!");
    }
}

// Modul ellenőrzés
bool PDU_webserver::hasModuleId(uint8_t id) {
    for(auto m: iec->getFoundIECIDs()) if(m == id) return true;
    return false;
}

// Relay állapot beállítása
void PDU_webserver::setRelayStatusWeb(uint8_t id, uint8_t relay, bool status) {
    if(id >= 1 && id <= 32 && relay < 8) {
        Serial.printf("Set module %d relay %d -> %s\n", id, relay, status ? "ON" : "OFF");
        iec->setRelayStatus(id, status); // IECControl implementáció
    }
}

// IEC frissítési ciklus
void PDU_webserver::setUpdateInterval(uint32_t ms) {
    if(ms < 100) ms = 100;
    if(ms > 5000) ms = 5000;
    updateInterval = ms;
    iec->setpowerDataUpdateCycleTime(updateInterval);
}

// WS broadcast minden modul állapotáról
void PDU_webserver::broadcastModules() {
    String json = "{\"type\":\"update\",\"modules\":[";
    auto ids = iec->getFoundIECIDs();
    for(size_t i=0; i<ids.size(); i++) {
        uint8_t id = ids[i];
        json += "{";
        json += "\"modbus_id\":" + String(id) + ",";
        json += "\"id\":" + String(iec->getIECID(id)) + ",";
        json += "\"voltage\":" + String(iec->getVoltageData(id)) + ",";
        json += "\"current\":" + String(iec->getCurrentData(id)) + ",";
        json += "\"power\":" + String(iec->getPowerData(id)) + ",";
        json += "\"version\":\"" + String(iec->getIECVersion(id)) + "\",";
        json += "\"relay_count\":" + String(iec->getIECRelayCount(id)) + ",";
        json += "\"relays\":[";
        int rcount = iec->getIECRelayCount(id);
        if(rcount > 8) rcount = 8;
        for(int r=0;r<rcount;r++){
            bool st = iec->getIECRelayStatus(id);
            json += st ? "true" : "false";
            if(r<rcount-1) json+=",";
        }
        json += "]";
        json += "}";
        if(i<ids.size()-1) json+=",";
    }
    json += "]}";

    ws.textAll(json);
}

// Webserver inicializálás
void PDU_webserver::runServer() {
    // --- REST API: összes modul
    webServer->on("/api/data", HTTP_GET, [this](AsyncWebServerRequest *request){
        String json = "[";
        auto ids = iec->getFoundIECIDs();
        for(size_t i=0;i<ids.size();i++){
            uint8_t id = ids[i];
            json += "{";
            json += "\"modbus_id\":" + String(id) + ",";
            json += "\"id\":" + String(iec->getIECID(id)) + ",";
            json += "\"voltage\":" + String(iec->getVoltageData(id)) + ",";
            json += "\"current\":" + String(iec->getCurrentData(id)) + ",";
            json += "\"power\":" + String(iec->getPowerData(id)) + ",";
            json += "\"version\":\"" + String(iec->getIECVersion(id)) + "\",";
            json += "\"relay_count\":" + String(iec->getIECRelayCount(id)) + ",";
            json += "\"relays\":[";
            int rcount = iec->getIECRelayCount(id);
            if(rcount>8) rcount=8;
            for(int r=0;r<rcount;r++){
                bool st = iec->getIECRelayStatus(id);
                json += st ? "true" : "false";
                if(r<rcount-1) json+=",";
            }
            json += "]";
            json += "}";
            if(i<ids.size()-1) json+=",";
        }
        json += "]";
        request->send(200,"application/json",json);
    });

    // --- Relay toggle REST
    webServer->on("/api/toggle", HTTP_GET, [this](AsyncWebServerRequest *request){
        if(!request->hasParam("mod") || !request->hasParam("relay")){
            request->send(400,"text/plain","Missing mod or relay parameter");
            return;
        }
        int mod = request->getParam("mod")->value().toInt();
        int relay = request->getParam("relay")->value().toInt();
        if(!hasModuleId(mod) || relay<0 || relay>=8){
            request->send(400,"text/plain","Invalid module or relay");
            return;
        }
        setRelayStatusWeb(mod,relay,!iec->getIECRelayStatus(mod));
        request->send(200,"text/plain","OK");
    });

    // --- Static fájlok
    webServer->serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    // --- WebSocket
    ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len){
        if(type==WS_EVT_CONNECT) {
            Serial.printf("WS client #%u connected\n", client->id());
        } else if(type==WS_EVT_DISCONNECT) {
            Serial.printf("WS client #%u disconnected\n", client->id());
        } else if(type==WS_EVT_DATA){
            // client üzenet feldolgozás (pl subscribe)
        }
    });
    webServer->addHandler(&ws);

    webServer->begin();
    Serial.println("Webserver is running!");
}
