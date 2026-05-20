#include "PDU_webserver.h"

PDU_webserver::PDU_webserver(AsyncWebServer *server, IECControl *iecControl, EnvironmentSensor *envSensor, networkLayerManager *networkLayer)
: webServer(server), iec(iecControl), ws("/ws"), _envSensor(envSensor), _networkLayer(networkLayer)
{
    if(!SPIFFS.begin(true)) {
        Serial.println("SPIFFS mount hiba!");
    }
}

bool PDU_webserver::hasModuleId(uint8_t id) {
    for(auto m: iec->getFoundIECIDs()) if(m == id) return true;
    return false;
}

void PDU_webserver::setRelayStatusWeb(uint8_t id, uint8_t relay, bool status) {
    if(id >= 1 && id <= 32 && relay < 8) {
        Serial.printf("Set module %d relay %d -> %s\n", id, relay, status ? "ON" : "OFF");
        iec->setRelayStatus(id, status);
    }
}

void PDU_webserver::setAllRelayStatusWeb(bool status) {
        iec->setAllIecRelayStatus(status);
}

void PDU_webserver::setUpdateInterval(uint32_t sec) {
    if(sec < 1) sec = 1;
    if(sec > 60) sec = 60;
    
    updateInterval = sec * 1000;
    iec->setpowerDataUpdateCycleTime(sec);
}

void PDU_webserver::broadcastModules() {
    if (millis() - lastMillis < updateInterval) return;
    lastMillis = millis();
    
    auto ids = iec->getFoundIECIDs();
    JsonDocument doc; 

    doc["type"] = "update";

    doc["total_current"] = iec->getSumIECCurrentData();
    doc["total_power"] = iec->getSumIECPowerData();
    doc["total_energy"] = iec->getSumIECEnergyData();

    JsonArray modulesArray = doc["modules"].to<JsonArray>();

    for(size_t i=0; i<ids.size(); i++) {
        uint8_t id = ids[i];
        
        JsonObject obj = modulesArray.add<JsonObject>();
        
        obj["modbus_id"] = id;
        obj["id"] = iec->getIECID(id);
        obj["voltage"] = iec->getRMSVoltageData(id);
        obj["current"] = iec->getRMSCurrentData(id);
        obj["power"] = iec->getApparentPowerData(id);
        obj["energy"] = iec->getEnergyKWh(id);
        obj["unit_power"] = "VA";
        obj["unit_energy"] = "kWh";
        obj["version"] = iec->getIECVersion(id);

        obj["status"] = (int)iec->getIECStatus(id);
        obj["curr_error"] = iec->getCustCurrErrorLimit(id);
        obj["curr_warning"] = iec->getCustCurrWarningLimit(id);
        obj["meas_avg_num"] = iec->getIECAVGNum(id);
        
        int rcount = iec->getIECRelayCount(id);
        obj["relay_count"] = rcount;
        
        JsonArray relays = obj["relays"].to<JsonArray>();
        
        if(rcount > 8) rcount = 8; // Biztonsági limit
        for(int r = 0; r < rcount; r++) {
            relays.add(iec->getIECRelayStatus(id)); 
        }
    }

    String json;
    serializeJson(doc, json);
    ws.textAll(json);
}

// Webserver init
void PDU_webserver::runServer() {
    
    // ==========================================
    // GET settings from client
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
        if (_networkLayer->checkSTAConnection()) {
            doc["sta_status"] = "Connected: " + WiFi.localIP().toString();
        } else  {
            doc["sta_status"] = "Disconnected";
        }
        // --- WIFI AP ---
        doc["ap_ssid"] = readStringFromNVS(NVSKeys::WIFI_AP_SSID, "PDU_MAIN_AP");
        doc["ap_pass"] = readStringFromNVS(NVSKeys::WIFI_AP_PWD, "admin");
        doc["ap_ip"]  = readStringFromNVS(NVSKeys::WIFI_AP_IP, "");
        doc["ap_gw"]  = readStringFromNVS(NVSKeys::WIFI_AP_GATEWAY, "");
        doc["ap_sn"]  = readStringFromNVS(NVSKeys::WIFI_AP_SUBNET, "");
        doc["ap_dns"] = readStringFromNVS(NVSKeys::WIFI_AP_DNS, "");

        if ((WiFi.getMode() & WIFI_AP) != 0) {
            doc["ap_status"] = "Enabled (Clients number: " + String(WiFi.softAPgetStationNum()) + ")";
        } else {
            doc["ap_status"] = "Disabled";
        }

        // --- ETHERNET ---
        // doc["eth_dhcp"] = ; TODO: DHCP támogatás hozzáadása
        doc["eth_ip"]   = readStringFromNVS(NVSKeys::ETHERNET_IP, "");
        doc["eth_gw"]   = readStringFromNVS(NVSKeys::ETHERNET_GATEWAY, "");
        doc["eth_sn"]   = readStringFromNVS(NVSKeys::ETHERNET_SUBNET, "");
        doc["eth_dns"]  = readStringFromNVS(NVSKeys::ETHERNET_DNS, "");

        // --- MEASURING ---
        doc["meas_oc"]    = readIntFromNVS(NVSKeys::MEAS_OC, 16);
        doc["meas_temp"]  = readStringFromNVS(NVSKeys::MEAS_TEMP, "C");
        doc["meas_cycle"] = readIntFromNVS(NVSKeys::MEAS_CYCLE, 1);
        doc["meas_delay"] = readIntFromNVS(NVSKeys::MEAS_DELAY, 0);

        // --- MQTT ---
        doc["mqtt_server"] = readStringFromNVS(NVSKeys::MQTT_SERVER, "");
        doc["mqtt_port"] = readStringFromNVS(NVSKeys::MQTT_PORT, "");
        doc["mqtt_ena"] = readIntFromNVS(NVSKeys::MQTT_ENA, 0);

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // ==========================================
    // Save settings from client (POST)
    // ==========================================
    webServer->on("/api/settings/setStaSettings", HTTP_POST, 
        [](AsyncWebServerRequest *request) {}, 
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            static String jsonBuffer = "";
            if (index == 0) jsonBuffer = "";
            for (size_t i = 0; i < len; i++) jsonBuffer += (char)data[i];
            Serial.println("Received JSON for STA settings: " + jsonBuffer);

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

    webServer->on("/api/settings/sta_status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["status"] = _networkLayer->getSTAStatusString();
        doc["enabled"] = _networkLayer->getSTAEnabled();
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // WiFi Mode (On/Off)
    webServer->on("/api/settings/setStaMode", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            deserializeJson(doc, data);
            bool enabled = doc["enabled"] | false;
            _networkLayer->setSTAEnabled(enabled);
            request->send(200, "application/json", "{\"ok\":true}");
        }
    );

    // Async wifi scan
    webServer->on("/api/wifi/scan_start", HTTP_POST, [this](AsyncWebServerRequest *request) {
        _networkLayer->startAsyncScan();
        request->send(200, "application/json", "{\"ok\":true}");
    });

    // Get the result of scan
    webServer->on("/api/wifi/scan_results", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!_networkLayer->isScanComplete()) {
            request->send(200, "application/json", "{\"status\":\"scanning\"}");
        } else {
            String res = _networkLayer->getScanResultsJSON();
            request->send(200, "application/json", "{\"status\":\"done\",\"networks\":" + res + "}");
        }
    });

    // Connecting to WiFi network
    webServer->on("/api/settings/sta_connect", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            deserializeJson(doc, data);
            String ssid = doc["sta_ssid"] | "";
            String pass = doc["sta_pass"] | "";
            
            if (ssid != "") {
                _networkLayer->connectSTA(ssid, pass);
            }
            request->send(200, "application/json", "{\"ok\":true}");
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
                    if (doc["ap_pass"].is<String>()) _networkLayer->configureWifiAP_Password(doc["ap_pass"].as<String>());
                    if (doc["ap_ip"].is<String>())   _networkLayer->configureWifiAP_IP(convertStringToIPAddress(doc["ap_ip"].as<String>()));
                    if (doc["ap_gw"].is<String>())   _networkLayer->configureWifiAP_Gateway(convertStringToIPAddress(doc["ap_gw"].as<String>()));
                    if (doc["ap_sn"].is<String>())   _networkLayer->configureWifiAP_Subnet(convertStringToIPAddress(doc["ap_sn"].as<String>()));
                    if (doc["ap_dns"].is<String>())  _networkLayer->configureWifiAP_DNS(convertStringToIPAddress(doc["ap_dns"].as<String>()));

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
                    // Overcurrent Threshold (OC - A)
                    if (doc["meas_oc"].is<int>()) {
                        int oc = doc["meas_oc"].as<int>();
                        writeIntToNVS(NVSKeys::MEAS_OC, oc);
                        
                        for (uint8_t id : iec->getFoundIECIDs()) {
                            iec->setOverCurrentTreshold(id, oc);
                        }
                    }
                
                    // Temperature Scale
                    if (doc["meas_temp"].is<String>()) {
                        String t = doc["meas_temp"].as<String>();
                        _envSensor->setTemperatureScale((t == "C" || t == "Celsius") ? 0 : 1);
                    }

                    // IEC Measurement Cycle Time (s)
                    if (doc["meas_cycle"].is<int>()) {
                        int cycle = doc["meas_cycle"].as<int>();
                        this->setUpdateInterval(cycle);
                    }

                    // IEC Switching Delay (s)
                    if (doc["meas_delay"].is<int>()) {
                        int delay = doc["meas_delay"].as<int>();
                        iec->setIECSwitchingDelay(delay);
                    }

                    Serial.println("Measuring settings has been saved!");
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
                    if (doc["mqtt_ena"].is<int>()) {
                        writeIntToNVS(NVSKeys::MQTT_ENA, doc["mqtt_ena"].as<int>());
                    }
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
    // API: Get data of all modules in JSON format
    // ==========================================
    webServer->on("/api/data", HTTP_GET, [this](AsyncWebServerRequest *request){
        JsonDocument doc; 
        doc["total_current"] = iec->getSumIECCurrentData();
        doc["total_power"] = iec->getSumIECPowerData();
        doc["total_energy"] = iec->getSumIECEnergyData();


        JsonArray modulesArray = doc["modules"].to<JsonArray>();

        auto ids = iec->getFoundIECIDs();
        for(size_t i=0;i<ids.size();i++){
            uint8_t id = ids[i];
            JsonObject obj = modulesArray.add<JsonObject>();
            obj["modbus_id"] = id;
            obj["id"] = iec->getIECID(id);

            obj["voltage"] = iec->getRMSVoltageData(id);
            obj["current"] = iec->getRMSCurrentData(id);
            obj["power"] = iec->getApparentPowerData(id);
            obj["activePower"] = iec->getActivePowerData(id);
            obj["reactivePower"] = iec->getReactivePowerData(id);
            
            obj["version"] = iec->getIECVersion(id);
            obj["relay_count"] = iec->getIECRelayCount(id);

            obj["curr_error"] = iec->getCustCurrErrorLimit(id);
            obj["curr_warning"] = iec->getCustCurrWarningLimit(id);
            obj["meas_avg_num"] = iec->getIECAVGNum(id);
            obj["status"] = (int)iec->getIECStatus(id);

            obj["availableLeds"] = iec->getIEC_AVAILABLE_LEDS(id);
            obj["currentLimit"] = iec->getIECCurrentLimit(id);

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
    // API: Set relay state directly (1 = ON, 0 = OFF)
    // ==========================================
    webServer->on("/api/relay/set", HTTP_GET, [this](AsyncWebServerRequest *request){
        if(!request->hasParam("mod") || !request->hasParam("relay") || !request->hasParam("state")){
            request->send(400,"text/plain","Missing mod, relay or state parameter");
            return;
        }
        int mod = request->getParam("mod")->value().toInt();
        int relay = request->getParam("relay")->value().toInt();
        bool state = request->getParam("state")->value().toInt() == 1;
        
        if(!hasModuleId(mod) || relay < 0 || relay >= 8){
            request->send(400,"text/plain","Invalid module or relay");
            return;
        }
        setRelayStatusWeb(mod, relay, state);
        request->send(200,"text/plain","OK");
    });

    webServer->on("/api/relay/setall", HTTP_GET, [this](AsyncWebServerRequest *request){
        if(!request->hasParam("state")){
            request->send(400,"text/plain","Missing state parameter");
            return;
        }
        bool state = request->getParam("state")->value().toInt() == 1;
        
        setAllRelayStatusWeb(state);
        request->send(200,"text/plain","OK");
    });

    // ==========================================
    // AP Status and Toggle API
    // ==========================================
    webServer->on("/api/settings/ap_status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        JsonDocument doc;
        bool apActive = _networkLayer->getWifiAP_Status();
        doc["enabled"] = apActive;
        if (apActive) {
            doc["status"] = "Enabled (" + String(WiFi.softAPgetStationNum()) + " clients)";
        } else {
            doc["status"] = "Disabled";
        }
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    webServer->on("/api/settings/ap_toggle", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, 
    [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        JsonDocument doc;
        deserializeJson(doc, data, len);
        bool enabled = doc["enabled"] | false;
        _networkLayer->setWifiAP_Status(enabled);
        request->send(200, "application/json", "{\"ok\":true}");
    });

    // ==========================================
    // MQTT Status and Toggle API
    // ==========================================
    webServer->on("/api/settings/mqtt_status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["enabled"] = mqttManager->isMQTTEnabled();
        doc["status"] = mqttManager->getMQTTStatusString();
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    webServer->on("/api/settings/mqtt_toggle", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, 
    [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        JsonDocument doc;
        deserializeJson(doc, data, len);
        bool enabled = doc["enabled"] | false;
        writeIntToNVS(NVSKeys::MQTT_ENA, enabled ? 1 : 0);
        request->send(200, "application/json", "{\"ok\":true}");
    });

    webServer->on("/api/module/settings", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            static String jsonBuffer = "";
            if (index == 0) jsonBuffer = "";
            for (size_t i = 0; i < len; i++) jsonBuffer += (char)data[i];

            if (index + len == total) {
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, jsonBuffer);
                
                if (!error && doc.containsKey("mod")) {
                    uint8_t modId = doc["mod"];
                    bool success = true;
                    
                if (doc.containsKey("warn")) {
                    if(iec->setCustCurrWarningLimit(modId, doc["warn"].as<float>()) == false) success = false;
                }
                if (doc.containsKey("err")) {
                    if(iec->setCustCurrErrorLimit(modId, doc["err"].as<float>()) == false) success = false;
                }
                if (doc.containsKey("avg")) {
                    if(iec->setIECAVGNum(modId, doc["avg"].as<uint16_t>()) == false) success = false;
                }

                    if(success) {
                        Serial.printf("IEC Module %d settings updated\n", modId);
                        request->send(200, "application/json", "{\"ok\":true}");
                    }
                    else request->send(400, "application/json", "{\"ok\":false,\"error\":\"Failed to write to IEC!\"}");
                   
                } else {
                    request->send(400, "application/json", "{\"ok\":false,\"error\":\"JSON error or missing mod ID!\"}");
                }
            }
        }
    );


    webServer->serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    // Setup websocket
    ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len){
        if(type==WS_EVT_CONNECT) {
            Serial.printf("WS client #%u connected\n", client->id());
        } else if(type==WS_EVT_DISCONNECT) {
            Serial.printf("WS client #%u disconnected\n", client->id());
        } else if(type==WS_EVT_DATA){
        }
    });
    webServer->addHandler(&ws);
    webServer->onNotFound([](AsyncWebServerRequest *request){
        request->send(404, "text/plain", "404: Not found");
    });

    // Setup web server
    webServer->begin();
    Serial.println("Webserver is running!");
}