#include "networkLayerManager.h"

#if !( defined(ESP32) )
  #error This code is designed for (ESP32 + W5500) to run on ESP32 platform! Please check your Tools->Board setting.
#endif

String convertIPAddressToString(IPAddress ip) {
  return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
}

IPAddress convertStringToIPAddress(const String& str) {
  IPAddress ip;
  ip.fromString(str);
  return ip;
}

void ensureNVSString(const char* key, const String& value) {
  if (readStringFromNVS(key, "").isEmpty()) {
    writeStringToNVS(key, value);
  }
}

void networkLayerManager::initInternetProtocol() {  
  _WifiAP_SSID = "PDUMain-AP"; // default AP SSID
  ensureNVSString(NVSKeys::WIFI_AP_SSID, _WifiAP_SSID);
  _WifiAP_Password = "12345678"; // default AP password
  ensureNVSString(NVSKeys::WIFI_AP_PWD, _WifiAP_Password);

  _WifiSTA_SSID = readStringFromNVS(NVSKeys::WIFI_STA_SSID, "");       // default STA SSID
  _WifiSTA_Password = readStringFromNVS(NVSKeys::WIFI_STA_PWD, "");   // default STA password

  _WifiIP = IPAddress(192, 168, 4, 1);
  ensureNVSString(NVSKeys::WIFI_IP, convertIPAddressToString(_WifiIP));
  _WifiGateway = IPAddress(192, 168, 4, 1);
  ensureNVSString(NVSKeys::WIFI_GATEWAY, convertIPAddressToString(_WifiGateway));
  _WifiSubnet = IPAddress(255, 255, 255, 0);
  ensureNVSString(NVSKeys::WIFI_SUBNET, convertIPAddressToString(_WifiSubnet));

  _EthernetIP = IPAddress(192, 168, 0, 5);
  ensureNVSString(NVSKeys::ETHERNET_IP, convertIPAddressToString(_EthernetIP));
  _EthernetGateway = IPAddress(192, 168, 0, 1);
  ensureNVSString(NVSKeys::ETHERNET_GATEWAY, convertIPAddressToString(_EthernetGateway));
  _EthernetSubnet = IPAddress(255, 255, 255, 0);
  ensureNVSString(NVSKeys::ETHERNET_SUBNET, convertIPAddressToString(_EthernetSubnet));
  _EthernetDNS = IPAddress(8, 8, 8, 8);
  ensureNVSString(NVSKeys::ETHERNET_DNS, convertIPAddressToString(_EthernetDNS));

  byte mac[] = {0x4C, 0x75, 0x25, 0xE4, 0xA0, 0x3F}; // TODO: use a random MAC address generator or the MAC address of the ESP32

  #ifdef DEBUG
    Serial.print(F("\nStart AsyncSimpleServer_ESP32_W5500 on "));Serial.print(ARDUINO_BOARD);Serial.print(F(" with "));Serial.println(SHIELD_TYPE);
    Serial.println(ASYNC_WEBSERVER_ESP32_W5500_VERSION);
  #endif

  ESP32_W5500_onEvent();
  if (ETH.begin(ETH_MISO, ETH_MOSI, ETH_SCK, ETH_CS, ETH_INTn, ETH_SPI_FREQ, SPI2_HOST, mac)) {
    ETH.config(_EthernetIP, _EthernetGateway, _EthernetSubnet);

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
  WiFiAPStatus = false;
  WiFiSTAStatus = false;
  WiFi.mode(WIFI_OFF); // Disable Wi-Fi to start with
}

///----------------------------------------------------------------------------------------------
///-------------------------------- Wi-Fi AP mode setup -----------------------------------------
///----------------------------------------------------------------------------------------------
void networkLayerManager::setupAPWifi(bool status) {
  WiFi.disconnect(true); // Disconnect from the current network and stop the Wi-Fi
  delay(200);
  if(status == true) {
    WiFi.softAP(_WifiAP_SSID, _WifiAP_Password);
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
  _WifiSTA_SSID = ssid;
  writeStringToNVS("ssid_sta", ssid);
}

void networkLayerManager::configureWifiPassword(String password) {
  _WifiSTA_Password = password;
  writeStringToNVS("pwd_sta", password);
}

bool networkLayerManager::getWiFiAPStatus() {
  return WiFiAPStatus;
}

void networkLayerManager::configureWifiIP(uint8_t ip[]) {
  _WifiIP = IPAddress(ip[0], ip[1], ip[2], ip[3]);
}

void networkLayerManager::configureWifiGateway(uint8_t gateway[]) {
  _WifiGateway = IPAddress(gateway[0], gateway[1], gateway[2], gateway[3]);
}

void networkLayerManager::configureWifiSubnet(uint8_t subnet[]) {
  _WifiSubnet = IPAddress(subnet[0], subnet[1], subnet[2], subnet[3]);
}

void networkLayerManager::configureWifiDNS(uint8_t dns[]) {
  IPAddress dnsIP(dns[0], dns[1], dns[2], dns[3]);
  WiFi.config(_WifiIP, _WifiGateway, _WifiSubnet, dnsIP);
}

bool networkLayerManager::turnOnWifiAP(bool status) {
  if (status == true && !WiFiAPStatus && !getWiFiSTAStatus() && _WifiAP_SSID != "" && _WifiAP_Password.length() >=8) {
    WiFi.softAPConfig(_WifiIP, _WifiGateway, _WifiSubnet);
    WiFi.softAP(_WifiAP_SSID, _WifiAP_Password);
    WiFiAPStatus = true;
    return true;
  } else {
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    WiFiAPStatus = false;
    return false;
  }
}

///----------------------------------------------------------------------------------------------
///-------------------------------- Wi-Fi STA mode setup ----------------------------------------
///----------------------------------------------------------------------------------------------

bool networkLayerManager::getWiFiSTAStatus() {
  return WiFiSTAStatus;
}

bool networkLayerManager::setfWifiSTA(bool status) {
  if (status) {
    WiFi.begin(_WifiSTA_SSID, _WifiSTA_Password);
    if (WiFi.mode(WIFI_STA) == false) {
      #ifdef DEBUG
        Serial.println("Failed to set Wi-Fi mode to STA.");
      #endif
      WiFiSTAStatus = false;
      return false;
    }
    #ifdef DEBUG
      Serial.println("Wi-Fi mode is STA. Connecting to SSID: " + _WifiSTA_SSID);
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

///----------------------------------------------------------------------------------------------
///------------------------------- Ethernet configuration --------------------------------------
///----------------------------------------------------------------------------------------------

String networkLayerManager::getEthernetIP() {
  _EthernetIP = ETH.localIP();
  return _EthernetIP.toString();
}

void networkLayerManager::setEthernetDHCP(bool status) {
  if (status) {
    _EthernetIP = IPAddress(0,0,0,0);
  } else {
    _EthernetIP = IPAddress(192, 168, 0, 5);
  }
}

bool networkLayerManager::getEthernetDHCPStatus() {
  return _EthernetIP == IPAddress(0,0,0,0); 
}

void networkLayerManager::configureEthernet_IP(uint8_t ip[]) {
  IPAddress _EthernetIP(ip[0], ip[1], ip[2], ip[3]);
}

void networkLayerManager::setEthernet_Gateway(uint8_t gateway[]) {
  IPAddress _EthernetGateway(gateway[0], gateway[1], gateway[2], gateway[3]);
}

void networkLayerManager::setEthernet_Subnet(uint8_t subnet[]) {
  IPAddress _EthernetSubnet(subnet[0], subnet[1], subnet[2], subnet[3]);
}

void networkLayerManager::setEthernet_DNS(uint8_t dns[]) {
  IPAddress _EthernetDNS(dns[0], dns[1], dns[2], dns[3]);
}

void networkLayerManager::configureEthernet() {
  ETH.config(_EthernetIP, _EthernetGateway, _EthernetSubnet, _EthernetDNS);
}

bool networkLayerManager::getNetworkStatus() {
  return ESP32_W5500_isConnected();
}