#include "mqttHandler.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);
String mqttServerIP = "";
int mqttServerPort = 1883;
unsigned long lastMqttPublish = 0;


void setupMQTT() {
  mqttServerIP = readStringFromNVS("mqtt_srv", "");
  mqttServerPort = readIntFromNVS("mqtt_port", 1883);

  if (mqttServerIP.length() > 0) {
    mqttClient.setServer(mqttServerIP.c_str(), mqttServerPort);
    Serial.printf("MQTT beállítva: %s:%d\n", mqttServerIP.c_str(), mqttServerPort);
  }
}

void handleMQTT() {
  if (mqttServerIP.length() == 0) return; // Ha nincs beállítva szerver, kilépünk

  if (!mqttClient.connected()) {
    // Ne blokkoljuk a loop-ot sokáig, ha nem tud csatlakozni
    static unsigned long lastConnectAttempt = 0;
    if (millis() - lastConnectAttempt > 5000) {
      lastConnectAttempt = millis();
      Serial.println("Próbálkozás az MQTT csatlakozással...");
      if (mqttClient.connect("PDU_ESP32_Client")) {
        Serial.println("MQTT csatlakozva!");
      }
    }
  } else {
    mqttClient.loop(); // MQTT keep-alive

    // Publikálás másodpercenként
    if (millis() - lastMqttPublish > 1000) {
      lastMqttPublish = millis();
      
      std::vector<uint8_t> ids = globalIEC->getFoundIECIDs();
      for (uint8_t id : ids) {
        String topic = "pdu/module/" + String(id);
        
        // JSON payload összerakása
        String payload = "{";
        payload += "\"voltage\":" + String(globalIEC->getVoltageData(id)) + ",";
        payload += "\"current\":" + String(globalIEC->getCurrentData(id)) + ",";
        payload += "\"power\":" + String(globalIEC->getPowerData(id));
        payload += "}";

        mqttClient.publish(topic.c_str(), payload.c_str());
      }
    }
  }
}