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
        obj["voltage"] = iec->getRMSVoltageData(id);
        obj["current"] = iec->getRMSCurrentData(id);
        obj["power"] = iec->getApparentPowerData(id);
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
    webServer->on("/api/settings/getData", HTTP_GET, [this](AsyncWebServerRequest *request){
        JsonDocument doc; 
        
        // --- WIFI STA ---
        doc["sta_ssid"] = readStringFromNVS(NVSKeys::WIFI_STA_SSID, "");
        doc["sta_pass"] = readStringFromNVS(NVSKeys::WIFI_STA_PWD, "");
        doc["ap_ip"]  = readStringFromNVS(NVSKeys::WIFI_STA_IP, "");
        doc["ap_gw"]  = readStringFromNVS(NVSKeys::WIFI_STA_GATEWAY, "");
        doc["ap_sn"]  = readStringFromNVS(NVSKeys::WIFI_STA_SUBNET, "");
        doc["ap_dns"] = readStringFromNVS(NVSKeys::WIFI_STA_DNS, "");
        if (WiFi.status() == WL_CONNECTED) {
            doc["sta_status"] = "Csatlakozva (" + WiFi.localIP().toString() + ")";
        } else {
            doc["sta_status"] = "Nincs csatlakozva (vagy folyamatban...)";
        }

        // --- WIFI AP ---
        doc["ap_ssid"] = readStringFromNVS(NVSKeys::WIFI_AP_SSID, "");
        doc["ap_pwd"] = readStringFromNVS(NVSKeys::WIFI_AP_PWD, "");
        doc["ap_ip"]  = readStringFromNVS(NVSKeys::WIFI_AP_IP, "");
        doc["ap_gw"]  = readStringFromNVS(NVSKeys::WIFI_AP_GATEWAY, "");
        doc["ap_sn"]  = readStringFromNVS(NVSKeys::WIFI_AP_SUBNET, "");
        doc["ap_dns"] = readStringFromNVS(NVSKeys::WIFI_AP_DNS, "");

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
        doc["meas_oc"]    = readStringFromNVS(NVSKeys::MEAS_OC, "0");
        doc["meas_temp"]  = readStringFromNVS(NVSKeys::MEAS_TEMP, "C");
        doc["meas_cycle"] = readIntFromNVS(NVSKeys::MEAS_CYCLE, 1);
        doc["meas_delay"] = readIntFromNVS(NVSKeys::MEAS_DELAY, 0);

        // --- MQTT ---
        doc["mqtt_server"] = readStringFromNVS(NVSKeys::MQTT_SERVER, "");
        doc["mqtt_port"] = readStringFromNVS(NVSKeys::MQTT_PORT, "");

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // ==========================================
    // Beállítások MENTÉSE (POST) a klienstől
    // ==========================================
    webServer->on("/api/settings/setSta", HTTP_POST, 
        [](AsyncWebServerRequest *request) {}, 
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            // Buffer összeállítása (kezeljük, ha több csomagban jön)
            static String jsonBuffer = "";
            if (index == 0) jsonBuffer = "";
            for (size_t i = 0; i < len; i++) jsonBuffer += (char)data[i];
            Serial.println("Received JSON for STA settings: " + jsonBuffer);

            // Ha megérkezett az összes adat
            if (index + len == total) {
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, jsonBuffer);
                
                if (!error) {
                    if (doc["sta_ssid"].is<String>()) _networkLayer->configureWifiSTA_SSID(doc["sta_ssid"].as<String>());
                    if (doc["sta_pass"].is<String>()) _networkLayer->configureWifiSTA_Password(doc["sta_pass"].as<String>());
                    if (doc["sta_ip"].is<String>())   _networkLayer->configureWifiSTA_IP(convertStringToIPAddress(doc["sta_ip"].as<String>()));
                    if (doc["sta_gw"].is<String>())   _networkLayer->configureWifiSTA_Gateway(convertStringToIPAddress(doc["sta_gw"].as<String>()));
                    if (doc["sta_sn"].is<String>())   _networkLayer->configureWifiSTA_Subnet(convertStringToIPAddress(doc["sta_sn"].as<String>()));
                    if (doc["sta_dns"].is<String>())  _networkLayer->configureWifiSTA_DNS(convertStringToIPAddress(doc["sta_dns"].as<String>()));
                    Serial.println("STA beállítások mentve!");
                    request->send(200, "application/json", "{\"ok\":true}");
                } else {
                    Serial.println("JSON deserialization error: " + String(error.c_str()));
                    request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                }
            }
        }
    );

    webServer->on("/api/settings/setAp", HTTP_POST, 
        [](AsyncWebServerRequest *request) {}, 
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            static String jsonBuffer = "";
            if (index == 0) jsonBuffer = "";
            for (size_t i = 0; i < len; i++) jsonBuffer += (char)data[i];
            if (index + len == total) {
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, jsonBuffer);
                
                if (!error) {
                    if (doc["ap_ssid"].is<String>()) _networkLayer->configureWifiAP_SSID(doc["ap_ssid"].as<String>());
                    if (doc["ap_pwd"].is<String>()) _networkLayer->configureWifiAP_Password(doc["ap_pwd"].as<String>());
                    if (doc["ap_ip"].is<String>())   _networkLayer->configureWifiAP_IP(convertStringToIPAddress(doc["ap_ip"].as<String>()));
                    if (doc["ap_gw"].is<String>())   _networkLayer->configureWifiAP_Gateway(convertStringToIPAddress(doc["ap_gw"].as<String>()));
                    if (doc["ap_sn"].is<String>())   _networkLayer->configureWifiAP_Subnet(convertStringToIPAddress(doc["ap_sn"].as<String>()));
                    if (doc["ap_dns"].is<String>())  _networkLayer->configureWifiAP_DNS(convertStringToIPAddress(doc["ap_dns"].as<String>()));
                    if (doc["ap_ena"].is<String>()) _networkLayer->setWifiAP_Status(doc["ap_ena"].as<String>() == "true");

                    Serial.println("AP Settings saved!");
                    request->send(200, "application/json", "{\"ok\":true}");
                } else {
                    request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                }
            }
        }
    );

    webServer->on("/api/settings/setEth", HTTP_POST, 
        [](AsyncWebServerRequest *request) {}, 
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            static String jsonBuffer = "";
            if (index == 0) jsonBuffer = "";
            for (size_t i = 0; i < len; i++) jsonBuffer += (char)data[i];
            if (index + len == total) {
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, jsonBuffer);
                
                if (!error) {
                    if (doc["eth_dhcp"].is<int>())   _networkLayer->setEthernetDHCP(doc["eth_dhcp"].as<int>() == 1);
                    if (doc["eth_ip"].is<String>())  _networkLayer->setEthernet_IP(convertStringToIPAddress(doc["eth_ip"].as<String>()));
                    if (doc["eth_gw"].is<String>())  _networkLayer->setEthernet_Gateway(convertStringToIPAddress(doc["eth_gw"].as<String>()));
                    if (doc["eth_sn"].is<String>())  _networkLayer->setEthernet_Subnet(convertStringToIPAddress(doc["eth_sn"].as<String>()));
                    if (doc["eth_dns"].is<String>()) _networkLayer->setEthernet_DNS(convertStringToIPAddress(doc["eth_dns"].as<String>()));
                    _networkLayer->configureEthernet(); // Az új beállítások érvényesítése
                    Serial.println("Ethernet beállítások mentve!");
                    request->send(200, "application/json", "{\"ok\":true}");
                } else {
                    request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                }
            }
        }
    );

    webServer->on("/api/settings/setMeas", HTTP_POST, 
        [](AsyncWebServerRequest *request) {}, 
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            static String jsonBuffer = "";
            if (index == 0) jsonBuffer = "";
            for (size_t i = 0; i < len; i++) jsonBuffer += (char)data[i];
            if (index + len == total) {
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, jsonBuffer);
                
                  if (!error) {
                if (doc["meas_oc"].is<int>()) writeIntToNVS(NVSKeys::MEAS_OC, doc["meas_oc"].as<int>());
                
                if (doc["meas_temp"].is<String>()) {
                    String t = doc["meas_temp"].as<String>();
                    _envSensor->setTemperatureScale((t == "C" || t == "Celsius") ? 0 : 1);
                }

                if (doc["meas_cycle"].is<int>()) {
                    int cycle = doc["meas_cycle"].as<int>();
                    writeIntToNVS(NVSKeys::MEAS_CYCLE, cycle);
                    this->setUpdateInterval(cycle); // Ne szorozd 1000-el, ha a függvény sec-et vár!
                }
                    Serial.println("Mérési beállítások mentve!");
                    request->send(200, "application/json", "{\"ok\":true}");
                } else {
                    request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                }
            }
        }
    );

    
    webServer->on("/api/settings/setMqtt", HTTP_POST, 
        [](AsyncWebServerRequest *request) {}, 
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            static String jsonBuffer = "";
            if (index == 0) jsonBuffer = "";
            for (size_t i = 0; i < len; i++) jsonBuffer += (char)data[i];
            if (index + len == total) {
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, jsonBuffer);
                
                if (!error) {
                    if (doc["mqtt_ena"].is<int>()) writeIntToNVS(NVSKeys::MQTT_ENA, doc["mqtt_ena"].as<int>());
                    if (doc["mqtt_server"].is<String>()) writeStringToNVS(NVSKeys::MQTT_SERVER, doc["mqtt_server"].as<String>());
                    if (doc["mqtt_port"].is<int>()) writeIntToNVS(NVSKeys::MQTT_PORT, doc["mqtt_port"].as<int>());
                    Serial.println("MQTT beállítások mentve!");

                    request->send(200, "application/json", "{\"ok\":true}");
                } else {
                    request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
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

            obj["voltage"] = iec->getRMSVoltageData(id);
            obj["current"] = iec->getRMSCurrentData(id);
            obj["power"] = iec->getApparentPowerData(id);
            obj["version"] = iec->getIECVersion(id);
            obj["relay_count"] = iec->getIECRelayCount(id);

            obj["overcurrent"] = iec->getOverCurrentTreshold();
            obj["curr_warning"] = iec->getCurrWarningLimit(id);

            obj["isRMSVoltageMeasured"] = iec->getIEC_IS_RMS_VOLTAGE_MEASURED(id);
            obj["isRMSCurrentMeasured"] = iec->getIEC_IS_RMS_CURRENT_MEASURED(id);
            obj["isActivePowerMeasured"] = iec->getIEC_IS_ACTIVE_POWER_MEASURED(id);
            obj["isReactivePowerMeasured"] = iec->getIEC_IS_REACTIVE_POWER_MEASURED(id);
            obj["isApparentPowerMeasured"] = iec->getIEC_IS_APPARENT_POWER_MEASURED(id);
            obj["isPowerFactorMeasured"] = iec->getIEC_IS_POWER_FACTOR_MEASURED(id);
            obj["isACFrequencyMeasured"] = iec->getIEC_IS_AC_FREQUENCY_MEASURED(id);

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