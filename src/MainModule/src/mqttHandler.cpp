#include "mqttHandler.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);
String mqttServerIP = "";
int mqttServerPort = 1883;
unsigned long lastMqttPublish = 0;
static unsigned long lastConnectAttempt = 0;


void setupMQTT() {
  mqttServerIP = readStringFromNVS("mqtt_srv", "192.168.0.2");
  mqttServerPort = readIntFromNVS("mqtt_port", 1883);

  if (mqttServerIP.length() > 0) {
    mqttClient.setServer(mqttServerIP.c_str(), mqttServerPort);
    mqttClient.setCallback(mqttCallback); 
    Serial.printf("MQTT beállítva: %s:%d\n", mqttServerIP.c_str(), mqttServerPort);
  }
}

// 1. A callback függvény
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Üzenet érkezett [");
  Serial.print(topic);
  Serial.print("]: ");
  
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  // Itt tudsz logikát váltani: pl. ha a topic "pdu/command", kapcsolj relét
}

void handleMQTT(bool enabled = false) {
  if (mqttServerIP.length() == 0 || !enabled) return; // Ha nincs beállítva szerver, kilépünk

  if (!mqttClient.connected()) {
    // Ne blokkoljuk a loop-ot sokáig, ha nem tud csatlakozni
    if (millis() - lastConnectAttempt > 5000) {
      lastConnectAttempt = millis();
      Serial.println("Próbálkozás az MQTT csatlakozással...");
      if (mqttClient.connect("PDU_ESP32_Client")) {
        Serial.println("MQTT csatlakozva!");
        mqttClient.subscribe("pdu/config/#"); 
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
        
        StaticJsonDocument<200> doc;
        doc["voltage"] = globalIEC->getVoltageData(id);
        doc["current"] = globalIEC->getCurrentData(id);
        doc["power"] = globalIEC->getPowerData(id);

        char buffer[200];
        serializeJson(doc, buffer);
        mqttClient.publish(topic.c_str(), buffer);
      }
    }
  }
}