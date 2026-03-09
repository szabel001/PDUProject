#include "mqttHandler.h"
#include <WiFi.h>

WiFiClient espClient;
PubSubClient mqttClient(espClient);

String mqttServerIP = ""; 
int mqttServerPort = 1883;
bool mqttEnabled = false;

unsigned long lastMqttPublish = 0;
static unsigned long lastConnectAttempt = 0;
static unsigned long lastNvsCheck = 0;

void setupMQTT() {
  // Induláskor beolvassuk az aktuális NVS értékeket
  mqttServerIP = readStringFromNVS(NVSKeys::MQTT_SERVER, "");
  mqttServerPort = readIntFromNVS(NVSKeys::MQTT_PORT, 1883);
  mqttEnabled = readIntFromNVS(NVSKeys::MQTT_ENA, 0) == 1;

  if (mqttServerIP.length() > 0) {
    mqttClient.setServer(mqttServerIP.c_str(), mqttServerPort);
    mqttClient.setCallback(mqttCallback); 
    Serial.printf("MQTT beállítva: %s:%d\n", mqttServerIP.c_str(), mqttServerPort);
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("MQTT Üzenet érkezett [");
  Serial.print(topic);
  Serial.print("]: ");
  
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  // Itt lehet majd lekezelni a bejövő MQTT parancsokat (pl. relé kapcsolás)
}

void handleMQTT() {
  // 1. Beállítások frissítése az NVS-ből 5 másodpercenként
  // (Így a weben módosított adatok újraindítás nélkül is érvénybe lépnek)
  if (millis() - lastNvsCheck > 5000) {
    lastNvsCheck = millis();
    bool newEnabled = readIntFromNVS(NVSKeys::MQTT_ENA, 0) == 1;
    String newIp = readStringFromNVS(NVSKeys::MQTT_SERVER, "");
    int newPort = readIntFromNVS(NVSKeys::MQTT_PORT, 1883);

    // Ha módosították a szerver IP-t vagy portot a webes felületen:
    if (newIp != mqttServerIP || newPort != mqttServerPort) {
      mqttServerIP = newIp;
      mqttServerPort = newPort;
      if (mqttServerIP.length() > 0) {
        mqttClient.setServer(mqttServerIP.c_str(), mqttServerPort);
      }
      if (mqttClient.connected()) mqttClient.disconnect(); // Bontjuk a régit
    }
    
    // Ha időközben kikapcsolták az MQTT-t:
    if (!newEnabled && mqttEnabled && mqttClient.connected()) {
      mqttClient.disconnect();
      Serial.println("MQTT kikapcsolva, kapcsolat bontva.");
    }
    mqttEnabled = newEnabled;
  }

  // 2. Ha az MQTT funkció ki van kapcsolva vagy nincs megadva IP, nem csinálunk semmit
  if (!mqttEnabled || mqttServerIP.length() == 0) return;

  // 3. Biztonsági ellenőrzés: Csak akkor csatlakozzunk az MQTT-hez, ha a WiFi már él!
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  // 4. MQTT Csatlakozás kezelése
  if (!mqttClient.connected()) {
    // Ne próbálkozzunk minden ciklusban, mert az lefagyasztja a fő loop-ot. Csak 5 másodpercenként.
    if (millis() - lastConnectAttempt > 5000) {
      lastConnectAttempt = millis();
      Serial.println("Próbálkozás az MQTT csatlakozással...");
      
      // Egyedi kliens azonosító generálása az ESP-nek
      String clientId = "PDU_ESP32_" + String(random(0xffff), HEX);
      
      if (mqttClient.connect(clientId.c_str())) {
        Serial.println("MQTT csatlakozva!");
        mqttClient.subscribe("pdu/config/#"); 
      } else {
        Serial.print("MQTT csatlakozás sikertelen, rc=");
        Serial.println(mqttClient.state());
      }
    }
  } else {
    // 5. Ha a kapcsolat él, futtatjuk a klienst és küldjük az adatokat
    mqttClient.loop(); // MQTT keep-alive és bejövő üzenetek fogadása

    // Publikálás másodpercenként (vagy ahogy beállítod)
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

// --- ÚJ: Státusz lekérdező függvények ---

bool isMQTTEnabled() {
  // Gyors lekérdezés az NVS-ből, hogy mindig a legfrissebb beállítást lássuk
  return readIntFromNVS(NVSKeys::MQTT_ENA, 0) == 1;
}

String getMQTTStatusString() {
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