#include "PDU_webserver.h"

PDU_webserver::PDU_webserver(AsyncWebServer *server, IECControl *iecControl, EnvironmentSensor *envSensor, networkLayerManager *networkLayer)
: webServer(server), iec(iecControl), ws("/ws"), _envSensor(envSensor), _networkLayer(networkLayer)
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
void PDU_webserver::setUpdateInterval(uint32_t sec) {
    if(sec < 1) sec = 1;
    if(sec > 60) sec = 60;
    updateInterval = sec;
    writeIntToNVS(NVSKeys::MEAS_CYCLE, sec);
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
        doc["sta_ssid"] = readStringFromNVS(NVSKeys::WIFI_STA_SSID, "");
        doc["sta_pass"] = readStringFromNVS(NVSKeys::WIFI_STA_PWD, "");
        if (WiFi.status() == WL_CONNECTED) {
            doc["sta_status"] = "Csatlakozva (" + WiFi.localIP().toString() + ")";
        } else {
            doc["sta_status"] = "Nincs csatlakozva (vagy folyamatban...)";
        }

        // --- WIFI AP ---
        doc["ap_ssid"] = readStringFromNVS(NVSKeys::WIFI_AP_SSID, "");
        doc["ap_pwd"] = readStringFromNVS(NVSKeys::WIFI_AP_PWD, "");
        doc["ap_ip"]  = readStringFromNVS(NVSKeys::WIFI_IP, "");
        doc["ap_gw"]  = readStringFromNVS(NVSKeys::WIFI_GATEWAY, "");
        doc["ap_sn"]  = readStringFromNVS(NVSKeys::WIFI_SUBNET, "");
        doc["ap_dns"] = readStringFromNVS(NVSKeys::WIFI_DNS, "");

        if ((WiFi.getMode() & WIFI_AP) != 0) {
            doc["ap_status"] = "Aktív (Kliensek: " + String(WiFi.softAPgetStationNum()) + ")";
        } else {
            doc["ap_status"] = "Kikapcsolva";
        }

        // --- ETHERNET ---
        // doc["eth_dhcp"] = ; TODO: DHCP támogatás hozzáadása
        doc["eth_ip"]   = readStringFromNVS(NVSKeys::ETHERNET_IP, "");
        doc["eth_gw"]   = readStringFromNVS(NVSKeys::ETHERNET_GATEWAY, "");
        doc["eth_sn"]   = readStringFromNVS(NVSKeys::ETHERNET_SUBNET, "");
        doc["eth_dns"]  = readStringFromNVS(NVSKeys::ETHERNET_DNS, "");

        // --- MEASURING ---
        doc["meas_oc"]    = readIntFromNVS(NVSKeys::MEAS_OC, 0);
        doc["meas_temp"]  = readStringFromNVS(NVSKeys::MEAS_TEMP, "");
        doc["meas_cycle"] = readIntFromNVS(NVSKeys::MEAS_CYCLE, 1);
        doc["meas_delay"] = readIntFromNVS(NVSKeys::MEAS_DELAY, 0);

        // --- MQTT ---
        doc["mqtt_server"] = readStringFromNVS(NVSKeys::MQTT_SERVER, "");
        doc["mqtt_port"] = readIntFromNVS(NVSKeys::MQTT_PORT, 1883);

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // ==========================================
    // Beállítások MENTÉSE (POST) a klienstől
    // ==========================================
    webServer->on("/api/settings/sta", HTTP_POST, 
        [](AsyncWebServerRequest *request) {request->send(200, "application/json", "{\"ok\":true}");}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (index == 0 && len == total) {
                String body = String((char*)data, len);
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, body);
                if (!error) {
                    // --- WiFi STA ---
                    if (doc["sta_ssid"].is<String>()) {
                        _networkLayer->configureWifiSSID(doc["sta_ssid"].as<String>());
                    }
                    if (doc["sta_pass"].is<String>()) {
                        _networkLayer->configureWifiPassword(doc["sta_pass"].as<String>());
                    }
                    Serial.println("Rendszer beállítások sikeresen mentve az NVS-be!");
                } else {
                    Serial.print("JSON feldolgozási hiba: ");
                    Serial.println(error.c_str());
                }
            }
        }
    );

    webServer->on("/api/settings/ap", HTTP_POST, 
        [](AsyncWebServerRequest *request) {request->send(200, "application/json", "{\"ok\":true}");}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (index == 0 && len == total) {
                String body = String((char*)data, len);
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, body);
                if (!error) {
                    // --- WiFi AP ---
                    if (doc["ap_ip"].is<String>()) _networkLayer->configureWifiIP(convertStringToIPAddress(doc["ap_ip"].as<String>()));
                    if (doc["ap_gw"].is<String>())  _networkLayer->configureWifiGateway(convertStringToIPAddress(doc["ap_gw"].as<String>()));
                    if (doc["ap_sn"].is<String>())  _networkLayer->configureWifiSubnet(convertStringToIPAddress(doc["ap_sn"].as<String>()));
                    if (doc["ap_dns"].is<String>()) _networkLayer->configureWifiDNS(convertStringToIPAddress(doc["ap_dns"].as<String>()));
                    if (doc["ap_ssid"].is<String>()) _networkLayer->configureWifiAP_SSID(doc["ap_ssid"].as<String>());
                    if (doc["ap_pwd"].is<String>()) _networkLayer->configureWifiAP_Password(doc["ap_pwd"].as<String>());
                    Serial.println("AP beállítások sikeresen mentve az NVS-be!");
                } else {
                    Serial.print("JSON feldolgozási hiba: ");
                    Serial.println(error.c_str());
                }
            }
        }
    );

    webServer->on("/api/settings/eth", HTTP_POST, 
        [](AsyncWebServerRequest *request) {request->send(200, "application/json", "{\"ok\":true}");}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (index == 0 && len == total) {
                String body = String((char*)data, len);
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, body);
                if (!error) {
                    // --- ETHERNET ---
                    if (doc["eth_dhcp"].is<int>())   _networkLayer->setEthernetDHCP(doc["eth_dhcp"].as<int>() == 1);
                    if (doc["eth_ip"].is<String>())  _networkLayer->setEthernet_IP(convertStringToIPAddress(doc["eth_ip"].as<String>()));
                    if (doc["eth_gw"].is<String>())  _networkLayer->setEthernet_Gateway(convertStringToIPAddress(doc["eth_gw"].as<String>()));
                    if (doc["eth_sn"].is<String>())  _networkLayer->setEthernet_Subnet(convertStringToIPAddress(doc["eth_sn"].as<String>()));
                    if (doc["eth_dns"].is<String>()) _networkLayer->setEthernet_DNS(convertStringToIPAddress(doc["eth_dns"].as<String>()));
                    _networkLayer->configureEthernet();
                    Serial.println("Ethernet beállítások sikeresen mentve az NVS-be!");
                } else {
                    Serial.print("JSON feldolgozási hiba: ");
                    Serial.println(error.c_str());
                }
            }
        }
    );

    webServer->on("/api/settings/meas", HTTP_POST, 
        [](AsyncWebServerRequest *request) {request->send(200, "application/json", "{\"ok\":true}");}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (index == 0 && len == total) {
                String body = String((char*)data, len);
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, body);
                if (!error) {
                    // --- MEASURING ---
                    if (doc["meas_oc"].is<int>())       writeIntToNVS(NVSKeys::MEAS_OC, doc["meas_oc"].as<int>());
                    if (doc["meas_temp"].is<String>()) _envSensor->setTemperatureScale(doc["meas_temp"].as<String>() == "Celsius" ? 0 : 1);
                    if (doc["meas_delay"].is<int>())    writeIntToNVS(NVSKeys::MEAS_DELAY, doc["meas_delay"].as<int>());
                    if (doc["meas_cycle"].is<int>()) {
                        int cycle = doc["meas_cycle"].as<int>();
                        writeIntToNVS(NVSKeys::MEAS_CYCLE, cycle);
                        this->setUpdateInterval(cycle * 1000); 
                    }
                    Serial.println("Mérési beállítások sikeresen mentve az NVS-be!");
                } else {
                    Serial.print("JSON feldolgozási hiba: ");
                    Serial.println(error.c_str());
                }
            }
        }
    );

    webServer->on("/api/settings/mqtt", HTTP_POST, 
        [](AsyncWebServerRequest *request) {request->send(200, "application/json", "{\"ok\":true}");}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (index == 0 && len == total) {
                String body = String((char*)data, len);
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, body);
                if (!error) {
                    // --- MQTT ---
                    if (doc["mqtt_ena"].is<int>()) writeIntToNVS(NVSKeys::MQTT_ENA, doc["mqtt_ena"].as<int>());
                    if (doc["mqtt_server"].is<String>()) writeStringToNVS(NVSKeys::MQTT_SERVER, doc["mqtt_server"].as<String>());
                    if (doc["mqtt_port"].is<int>()) writeIntToNVS(NVSKeys::MQTT_PORT, doc["mqtt_port"].as<int>());
                    Serial.println("MQTT beállítások sikeresen mentve az NVS-be!");
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