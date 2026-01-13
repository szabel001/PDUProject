#include "networkLayerManager.h"

#if !( defined(ESP32) )
  #error This code is designed for (ESP32 + W5500) to run on ESP32 platform! Please check your Tools->Board setting.
#endif

void networkLayerManager::initInternetProtocol() {  
  _actWifiAP_SSID = "PDUMain-AP"; // default AP SSID
  if (readStringFromNVS("ssid_ap", "") == "") writeStringToNVS("ssid_ap", _actWifiAP_SSID);
  _actWifiAP_Password = "12345678"; // default AP password
  if (readStringFromNVS("pwd_ap", "") == "") writeStringToNVS("pwd_ap", _actWifiAP_Password);

  _actEthernetIP = IPAddress(192, 168, 55, 10);
  _actEthernetGateway = IPAddress(192, 168, 55, 1);
  _actEthernetSubnet = IPAddress(255, 255, 255, 0);

  byte mac[] = {0x4C, 0x75, 0x25, 0xE4, 0xA0, 0x3F}; // TODO: use a random MAC address generator or the MAC address of the ESP32

  #ifdef DEBUG
    Serial.print(F("\nStart AsyncSimpleServer_ESP32_W5500 on "));Serial.print(ARDUINO_BOARD);Serial.print(F(" with "));Serial.println(SHIELD_TYPE);
    Serial.println(ASYNC_WEBSERVER_ESP32_W5500_VERSION);

    AWS_LOGWARN(F("Default SPI pinout:"));
    AWS_LOGWARN1(F("SPI_HOST:"), ETH_SPI_HOST);
    AWS_LOGWARN1(F("MOSI:"), MOSI_GPIO);
    AWS_LOGWARN1(F("MISO:"), MISO_GPIO);
    AWS_LOGWARN1(F("SCK:"),  SCK_GPIO);
    AWS_LOGWARN1(F("CS:"),   CS_GPIO);
    AWS_LOGWARN1(F("INT:"),  INT_GPIO);
    AWS_LOGWARN1(F("SPI Clock (MHz):"), SPI_CLOCK_MHZ);
    AWS_LOGWARN(F("========================="));
  #endif

  ESP32_W5500_onEvent();
  if (ETH.begin(ETH_MISO, ETH_MOSI, ETH_SCK, ETH_CS, ETH_INTn, ETH_SPI_FREQ, SPI2_HOST, mac)) { //SPI must be SPI2_HOST ()
    ETH.config(_actEthernetIP, _actEthernetGateway, _actEthernetSubnet);

    delay(100);

    Serial.println("Ethernet is working.");
    Serial.print("Server is at "); Serial.println(ETH.localIP());
    delay(100);
  }
  #ifdef DEBUG
    else {
      Serial.println("Ethernet shield was not found or cable is not connected.");
    }
  #endif
  WiFiSTAStatus = false;
  WiFi.mode(WIFI_OFF); // Disable Wi-Fi to start with
}

void networkLayerManager::setupAPWifi(bool status) {
  WiFi.disconnect(true); // Disconnect from the current network and stop the Wi-Fi
  delay(200);
  if(status == true) {
    WiFi.softAP(_actWifiAP_SSID, _actWifiAP_Password);
    #ifdef DEBUG
      Serial.println("Wi-Fi AP mode started.");
      Serial.print("IP address: ");
      Serial.println(WiFi.softAPIP());
    #endif
    WiFiAPStatus = true;
  } else if (status == false) {      // TODO handle not given SSID and password
    WiFi.softAPdisconnect(true); // Disable AP mode
    WiFi.mode(WIFI_OFF); // Turn off Wi-Fi
    WiFiAPStatus = false;
    #ifdef DEBUG
      Serial.println("Wi-Fi AP mode stopped.");
    #endif
  }
}

void networkLayerManager::configureWifiSSID(String ssid) {
  _actWifiSTA_SSID = ssid;
  writeStringToNVS("ssid_sta", ssid);
}

void networkLayerManager::configureWifiPassword(String password) {
  _actWifiSTA_Password = password;
  writeStringToNVS("pwd_sta", password);
}

bool networkLayerManager::getWiFiAPStatus() {
  return WiFiAPStatus;
}

bool networkLayerManager::getWiFiSTAStatus() {
  return WiFiSTAStatus;
}

bool networkLayerManager::setfWifiSTA(bool status) {
  if (status) {
    WiFi.begin(_actWifiSTA_SSID, _actWifiSTA_Password);
    if (WiFi.mode(WIFI_STA) == false) {
      #ifdef DEBUG
        Serial.println("Failed to set Wi-Fi mode to STA.");
      #endif
      WiFiSTAStatus = false;
      return false;
    }
    #ifdef DEBUG
      Serial.println("Wi-Fi mode is STA. Connecting to SSID: " + _actWifiSTA_SSID);
    #endif
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
      delay(100);
    }
    if (WiFi.status() == WL_CONNECTED) {
      #ifdef DEBUG
        Serial.println("Connected to Wi-Fi network. IP address: " + WiFi.localIP().toString());
      #endif
      WiFiSTAStatus = true;
      return true;
    } else {
      #ifdef DEBUG
        Serial.println("Failed to connect to Wi-Fi network.");
      #endif
      WiFiSTAStatus = false;
      return false;
    }

  } else {
    WiFiSTAStatus = false;
    WiFi.mode(WIFI_OFF);
    #ifdef DEBUG
      Serial.println("Wi-Fi mode is OFF.");
    #endif
    return false;
  }
}

void networkLayerManager::configureWifiAP(uint8_t ip[], uint8_t gateway[], uint8_t subnet[], wifiMode mode) {
  _actWifiIP = IPAddress(ip[0], ip[1], ip[2], ip[3]);
  _actWifiGateway = IPAddress(gateway[0], gateway[1], gateway[2], gateway[3]);
  _actWifiSubnet = IPAddress(subnet[0], subnet[1], subnet[2], subnet[3]);
  if (mode == wifiMode::STA) {
    WiFi.config(_actWifiIP, _actWifiGateway, _actWifiSubnet);
  } else if (mode == wifiMode::AP) {
    WiFi.softAPConfig(_actWifiIP, _actWifiGateway, _actWifiSubnet);
  }
}

void networkLayerManager::configureEthernet(uint8_t ip[], uint8_t gateway[], uint8_t subnet[]) {
  IPAddress myIP(ip[0], ip[1], ip[2], ip[3]);
  IPAddress myGW(gateway[0], gateway[1], gateway[2], gateway[3]);
  IPAddress mySN(subnet[0], subnet[1], subnet[2], subnet[3]);
  ETH.config(myIP, myGW, mySN);
  #ifdef DEBUG
    Serial.println("Ethernet configuration done.");
    Serial.print("IP address: ");
    Serial.println(ETH.localIP());
    Serial.print("Gateway: ");
    Serial.println(ETH.gatewayIP());
    Serial.print("Subnet: ");
    Serial.println(ETH.subnetMask());
  #endif
}

bool networkLayerManager::getNetworkStatus() {
  return ESP32_W5500_isConnected();
}