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
    if (millis() - lastMillis < updateInterval) return;
    lastMillis = millis();
    
    auto ids = iec->getFoundIECIDs();
    JsonDocument doc; 

    // 1. A kliens JS elvárja, hogy megmondjuk, milyen típusú az üzenet
    doc["type"] = "update";
    
    // 2. Létrehozunk egy "modules" nevű tömböt (ArduinoJson 7 szintaxis)
    JsonArray modulesArray = doc["modules"].to<JsonArray>();

    for(size_t i=0; i<ids.size(); i++) {
        uint8_t id = ids[i];
        
        // 3. A tömbhöz hozzáadunk egy új objektumot (ArduinoJson 7 szintaxis)
        JsonObject obj = modulesArray.add<JsonObject>();
        
        obj["modbus_id"] = id;
        obj["id"] = iec->getIECID(id);
        obj["voltage"] = iec->getVoltageData(id);
        obj["current"] = iec->getCurrentData(id);
        obj["power"] = iec->getPowerData(id);
        obj["version"] = iec->getIECVersion(id);
        
        int rcount = iec->getIECRelayCount(id);
        obj["relay_count"] = rcount;
        
        // 4. Létrehozzuk a "relays" tömböt az objektumon belül (ArduinoJson 7 szintaxis)
        JsonArray relays = obj["relays"].to<JsonArray>();
        
        // Végigmegyünk a reléken és betöltjük az állapotukat a tömbbe
        if(rcount > 8) rcount = 8; // Biztonsági limit
        for(int r = 0; r < rcount; r++) {
            // Megjegyzés: Ha az IECControl-od támogatja a relé index lekérését, 
            // érdemes így használni: iec->getIECRelayStatus(id, r)
            relays.add(iec->getIECRelayStatus(id)); 
        }
    } // <-- A for ciklus vége

    String json;
    serializeJson(doc, json);
    ws.textAll(json);
}

// Webserver inicializálás
void PDU_webserver::runServer() {
    
    // ==========================================
    // Beállítások LEKÉRÉSE (GET) a klienstől
    // ==========================================
    webServer->on("/api/settings", HTTP_GET, [this](AsyncWebServerRequest *request){
        JsonDocument doc; 
        
        // --- WIFI STA ---
        doc["sta_ssid"] = readStringFromNVS("sta_ssid", "");
        doc["sta_pass"] = readStringFromNVS("sta_pass", "");
        if (WiFi.status() == WL_CONNECTED) {
            doc["sta_status"] = "Csatlakozva (" + WiFi.localIP().toString() + ")";
        } else {
            doc["sta_status"] = "Nincs csatlakozva (vagy folyamatban...)";
        }

        // --- WIFI AP ---
        doc["ap_ip"]  = readStringFromNVS("ap_ip", "192.168.4.1");
        doc["ap_gw"]  = readStringFromNVS("ap_gw", "192.168.4.1");
        doc["ap_sn"]  = readStringFromNVS("ap_sn", "255.255.255.0");
        doc["ap_dns"] = readStringFromNVS("ap_dns", "8.8.8.8");
        if ((WiFi.getMode() & WIFI_AP) != 0) {
            doc["ap_status"] = "Aktív (Kliensek: " + String(WiFi.softAPgetStationNum()) + ")";
        } else {
            doc["ap_status"] = "Kikapcsolva";
        }

        // --- ETHERNET ---
        doc["eth_dhcp"] = readIntFromNVS("eth_dhcp", 1);
        doc["eth_ip"]   = readStringFromNVS("eth_ip", "");
        doc["eth_gw"]   = readStringFromNVS("eth_gw", "");
        doc["eth_sn"]   = readStringFromNVS("eth_sn", "");
        doc["eth_dns"]  = readStringFromNVS("eth_dns", "");

        // --- MEASURING ---
        doc["meas_oc"]    = readIntFromNVS("meas_oc", 16);
        doc["meas_temp"]  = readStringFromNVS("meas_temp", "C");
        doc["meas_cycle"] = readIntFromNVS("meas_cycle", 1);
        doc["meas_delay"] = readIntFromNVS("meas_delay", 0);

        // --- MQTT ---
        doc["mqtt_server"] = readStringFromNVS("mqtt_srv", "");
        doc["mqtt_port"] = readIntFromNVS("mqtt_port", 1883);

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // ==========================================
    // Beállítások MENTÉSE (POST) a klienstől
    // ==========================================
    webServer->on("/api/settings", HTTP_POST, 
        // 1. Request lezáró callback (Válasz küldése a kliensnek)
        [](AsyncWebServerRequest *request) {
            request->send(200, "application/json", "{\"ok\":true}");
        },
        // 2. Upload callback (Fájlfeltöltéshez, nekünk most NULL)
        NULL,
        // 3. Body callback (Itt dolgozzuk fel a bejövő JSON-t)
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            
            // Ha a JSON belefért egyetlen adatcsomagba
            if (index == 0 && len == total) {
                // Nyers adat konvertálása String-gé
                String body = String((char*)data, len);

                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, body);

                if (!error) {
                    // --- WiFi STA ---
                    if (doc["sta_ssid"].is<String>()) writeStringToNVS("sta_ssid", doc["sta_ssid"].as<String>());
                    if (doc["sta_pass"].is<String>()) writeStringToNVS("sta_pass", doc["sta_pass"].as<String>());

                    // --- WiFi AP ---
                    if (doc["ap_ip"].is<String>())  writeStringToNVS("ap_ip", doc["ap_ip"].as<String>());
                    if (doc["ap_gw"].is<String>())  writeStringToNVS("ap_gw", doc["ap_gw"].as<String>());
                    if (doc["ap_sn"].is<String>())  writeStringToNVS("ap_sn", doc["ap_sn"].as<String>());
                    if (doc["ap_dns"].is<String>()) writeStringToNVS("ap_dns", doc["ap_dns"].as<String>());

                    // --- ETHERNET ---
                    if (doc["eth_dhcp"].is<int>())   writeIntToNVS("eth_dhcp", doc["eth_dhcp"].as<int>());
                    if (doc["eth_ip"].is<String>())  writeStringToNVS("eth_ip", doc["eth_ip"].as<String>());
                    if (doc["eth_gw"].is<String>())  writeStringToNVS("eth_gw", doc["eth_gw"].as<String>());
                    if (doc["eth_sn"].is<String>())  writeStringToNVS("eth_sn", doc["eth_sn"].as<String>());
                    if (doc["eth_dns"].is<String>()) writeStringToNVS("eth_dns", doc["eth_dns"].as<String>());

                    // --- MEASURING ---
                    if (doc["meas_oc"].is<int>())       writeIntToNVS("meas_oc", doc["meas_oc"].as<int>());
                    if (doc["meas_temp"].is<String>())  writeStringToNVS("meas_temp", doc["meas_temp"].as<String>());
                    if (doc["meas_delay"].is<int>())    writeIntToNVS("meas_delay", doc["meas_delay"].as<int>());
                    
                    if (doc["meas_cycle"].is<int>()) {
                        int cycle = doc["meas_cycle"].as<int>();
                        writeIntToNVS("meas_cycle", cycle);
                        
                        // Dinamikus frissítés újraindulás nélkül
                        this->setUpdateInterval(cycle * 1000); 
                    }

                    // --- MQTT ---
                    if (doc["mqtt_server"].is<String>()) writeStringToNVS("mqtt_srv", doc["mqtt_server"].as<String>());
                    if (doc["mqtt_port"].is<int>()) writeIntToNVS("mqtt_port", doc["mqtt_port"].as<int>());
                    
                    Serial.println("Rendszer beállítások sikeresen mentve az NVS-be!");
                } else {
                    Serial.print("JSON feldolgozási hiba: ");
                    Serial.println(error.c_str());
                }
            }
        }
    );

    // ==========================================
    // API: Modulok adatainak lekérése
    // ==========================================
    webServer->on("/api/data", HTTP_GET, [this](AsyncWebServerRequest *request){
        JsonDocument doc; 
        auto ids = iec->getFoundIECIDs();
        for(size_t i=0;i<ids.size();i++){
            uint8_t id = ids[i];
            JsonObject obj = doc.add<JsonObject>();
            obj["modbus_id"] = id;
            obj["id"] = iec->getIECID(id);

            obj["voltage"] = iec->getVoltageData(id);
            obj["current"] = iec->getCurrentData(id);
            obj["power"] = iec->getPowerData(id);
            obj["version"] = iec->getIECVersion(id);
            obj["relay_count"] = iec->getIECRelayCount(id);

            JsonArray relays = obj["relays"].to<JsonArray>();
            
            int rcount = iec->getIECRelayCount(id);
            if(rcount>8) rcount=8;
            for(int r=0;r<rcount;r++){
                bool st = iec->getIECRelayStatus(id);
                relays.add(st);
            }
        }
        String json;
        serializeJson(doc, json);
        request->send(200,"application/json",json);
        });

    // ==========================================
    // API: Relé billentése
    // ==========================================
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

    // --- Static fájlok kiszolgálása a belső flash-ről (SPIFFS)
    webServer->serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    // --- WebSocket beállítása
    ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len){
        if(type==WS_EVT_CONNECT) {
            Serial.printf("WS client #%u connected\n", client->id());
        } else if(type==WS_EVT_DISCONNECT) {
            Serial.printf("WS client #%u disconnected\n", client->id());
        } else if(type==WS_EVT_DATA){
            // kliens üzenet feldolgozása, ha szükséges
        }
    });
    webServer->addHandler(&ws);

    // Szerver indítása
    webServer->begin();
    Serial.println("Webserver is running!");
}