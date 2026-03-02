#include "mqttHandler.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);
String mqttServerIP = "192.168.0.2"; 
String mqttServerPort = "1883";
unsigned long lastMqttPublish = 0;
static unsigned long lastConnectAttempt = 0;

void configureMQTT_Server(String serverIP, String port) {
  mqttServerIP = serverIP;
  mqttServerPort = port;
  writeStringToNVS(NVSKeys::MQTT_SERVER, serverIP);
  writeStringToNVS(NVSKeys::MQTT_PORT, String(port));
}

void setupMQTT() {
  writeStringToNVS(NVSKeys::MQTT_SERVER, mqttServerIP);
  writeStringToNVS(NVSKeys::MQTT_PORT, mqttServerPort);

  if (mqttServerIP.length() > 0) {
    mqttClient.setServer(mqttServerIP.c_str(), mqttServerPort.toInt());
    mqttClient.setCallback(mqttCallback); 
    Serial.printf("MQTT beállítva: %s:%s\n", mqttServerIP.c_str(), mqttServerPort.c_str());
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
        doc["voltage"] = globalIEC->getRMSVoltageData(id);
        doc["current"] = globalIEC->getRMSCurrentData(id);
        doc["power"] = globalIEC->getApparentPowerData(id);

        char buffer[200];
        serializeJson(doc, buffer);
        mqttClient.publish(topic.c_str(), buffer);
      }
    }
  }
}