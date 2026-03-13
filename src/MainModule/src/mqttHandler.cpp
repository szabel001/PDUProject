#include "mqttHandler.h"
#include <WiFi.h>

// Konstruktor inicializálása
MqttHandler::MqttHandler(IECControl* iecControl) 
    : _iec(iecControl), 
      mqttClient(espClient),
      mqttServerIP(""), 
      mqttServerPort(1883), 
      mqttEnabled(false),
      lastMqttPublish(0), 
      lastConnectAttempt(0), 
      lastNvsCheck(0), 
      firstTry(true)
{
}

void MqttHandler::setupMQTT() {
    mqttServerIP = readStringFromNVS(NVSKeys::MQTT_SERVER, "");
    mqttServerPort = readIntFromNVS(NVSKeys::MQTT_PORT, 1883);
    mqttEnabled = readIntFromNVS(NVSKeys::MQTT_ENA, 0) == 1;

    if (mqttServerIP.length() > 0) {
        mqttClient.setServer(mqttServerIP.c_str(), mqttServerPort);
        mqttClient.setCallback(MqttHandler::mqttCallback); 
        Serial.printf("MQTT beállítva: %s:%d\n", mqttServerIP.c_str(), mqttServerPort);
    }
}

void MqttHandler::mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print("MQTT Üzenet érkezett [");
    Serial.print(topic);
    Serial.print("]: ");
    
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.println(message);
    // Itt lehet majd lekezelni a bejövő parancsokat
}

void MqttHandler::handleMQTT() {
    if (millis() - lastNvsCheck > 5000) {
        lastNvsCheck = millis();
        bool newEnabled = readIntFromNVS(NVSKeys::MQTT_ENA, 0) == 1;
        String newIp = readStringFromNVS(NVSKeys::MQTT_SERVER, "");
        int newPort = readIntFromNVS(NVSKeys::MQTT_PORT, 1883);

        if (newIp != mqttServerIP || newPort != mqttServerPort) {
            mqttServerIP = newIp;
            mqttServerPort = newPort;
            if (mqttServerIP.length() > 0) {
                mqttClient.setServer(mqttServerIP.c_str(), mqttServerPort);
            }
            if (mqttClient.connected()) mqttClient.disconnect();
        }
        
        if (!newEnabled && mqttEnabled && mqttClient.connected()) {
            mqttClient.disconnect();
            Serial.println("MQTT kikapcsolva, kapcsolat bontva.");
        }
        mqttEnabled = newEnabled;
    }

    if (!mqttEnabled || mqttServerIP.length() == 0) return;

    if (WiFi.status() != WL_CONNECTED) {
        firstTry = true;
        return;
    }

    if (!mqttClient.connected() & firstTry) {
        if (millis() - lastConnectAttempt > 5000) {
            lastConnectAttempt = millis();
            Serial.println("Próbálkozás az MQTT csatlakozással...");
            
            String clientId = "PDU_ESP32_" + String(random(0xffff), HEX);
            
            if (mqttClient.connect(clientId.c_str())) {
                Serial.println("MQTT csatlakozva!");
                mqttClient.subscribe("pdu/config/#"); 
            } else {
                Serial.print("MQTT csatlakozás sikertelen, rc=");
                Serial.println(mqttClient.state());
                firstTry = false;
            }
        }
    } else {
        mqttClient.loop(); 

        if (millis() - lastMqttPublish > 1000) {
            lastMqttPublish = millis();
            
            // Itt használjuk az osztálynak átadott _iec pointert
            if (_iec) {
                std::vector<uint8_t> ids = _iec->getFoundIECIDs();
                for (uint8_t id : ids) {
                    String topic = "pdu/module/" + String(id);
                    
                    StaticJsonDocument<200> doc;
                    doc["voltage"] = _iec->getRMSVoltageData(id);
                    doc["current"] = _iec->getRMSCurrentData(id);
                    doc["power"] = _iec->getApparentPowerData(id);

                    char buffer[200];
                    serializeJson(doc, buffer);
                    mqttClient.publish(topic.c_str(), buffer);
                }
            }
        }
    }
}

bool MqttHandler::isMQTTEnabled() {
    return readIntFromNVS(NVSKeys::MQTT_ENA, 0) == 1;
}

String MqttHandler::getMQTTStatusString() {
    if (!isMQTTEnabled()) {
        return "Disabled";
    }
    if (WiFi.status() != WL_CONNECTED) {
        return "Waiting for WiFi...";
    }
    if (mqttClient.connected()) {
        return "Connected (" + mqttServerIP + ")";
    }
    return "Disconnected / Reconnecting...";
}