#include "networkLayerManager.h"

#if !( defined(ESP32) )
  #error This code is designed for (ESP32 + W5500) to run on ESP32 platform! Please check your Tools->Board setting.
#endif

void networkLayerManager::initInternetProtocol() {  
  _actWifiAP_SSID = "PDUMain-AP"; // default AP SSID
  _actWifiAP_Password = "12345678"; // default AP password

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
  setupWifi(wifiMode::OFF);
}

void networkLayerManager::setupWifi(wifiMode mode) {
  WiFi.disconnect(true); // Disconnect from the current network and stop the Wi-Fi
  delay(200);
  if(mode == wifiMode::AP) {
    WiFi.softAP(_actWifiAP_SSID, _actWifiAP_Password);
    #ifdef DEBUG
      Serial.println("Wi-Fi AP mode started.");
      Serial.print("IP address: ");
      Serial.println(WiFi.softAPIP());
    #endif
  } else if (mode == wifiMode::STA) {      // TODO handle not given SSID and password
    WiFi.mode(WIFI_STA);
    #ifdef DEBUG
      WiFi.begin(_actWifiSTA_SSID, _actWifiSTA_Password);
      Serial.println("Connecting to Wi-Fi...");
    #endif
    while (WiFi.status() != WL_CONNECTED) {
      #ifdef DEBUG
        delay(500);
        Serial.print(".");
      #endif
    }
    #ifdef DEBUG
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());  
    #endif
  }
  else if (mode == wifiMode::OFF) {
    WiFi.mode(WIFI_OFF);
    #ifdef DEBUG
      Serial.println("Wi-Fi mode is OFF.");
    #endif
  }
}

void networkLayerManager::configureWifiSSID(String ssid) {
  _actWifiSTA_SSID = ssid;
  setupWifi(wifiMode::STA);
}

void networkLayerManager::configureWifiPassword(String password) {
  _actWifiSTA_Password = password;
  setupWifi(wifiMode::STA);
}

void networkLayerManager::configureWifiCredentials(String ssid, String password, wifiMode mode) {
  if (mode == wifiMode::STA) {
    _actWifiSTA_SSID = ssid;
    _actWifiSTA_Password = password;
    setupWifi(wifiMode::STA);
  } else if (mode == wifiMode::AP) {
    _actWifiAP_SSID = ssid;
    _actWifiAP_Password = ssid;
    setupWifi(wifiMode::AP);
  }
}

void networkLayerManager::configureWifi(uint8_t ip[], uint8_t gateway[], uint8_t subnet[], wifiMode mode) {
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