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

void networkLayerManager::initInternetProtocol() {  
  _WifiSTA_SSID = ensureNVSString(NVSKeys::WIFI_STA_SSID, "admin");       // default STA SSID
  _WifiSTA_Password = ensureNVSString(NVSKeys::WIFI_STA_PWD, "admin");   // default STA password
  _WifiSTA_IP = convertStringToIPAddress(ensureNVSString(NVSKeys::WIFI_STA_IP, convertIPAddressToString(IPAddress(192, 168, 0, 100))));
  _WifiSTA_Gateway = convertStringToIPAddress(ensureNVSString(NVSKeys::WIFI_STA_GATEWAY, convertIPAddressToString(IPAddress(192, 168, 0, 1))));
  _WifiSTA_Subnet = convertStringToIPAddress(ensureNVSString(NVSKeys::WIFI_STA_SUBNET, convertIPAddressToString(IPAddress(255, 255, 255, 0))));
  _WifiSTA_DNS = convertStringToIPAddress(ensureNVSString(NVSKeys::WIFI_STA_DNS, convertIPAddressToString(IPAddress(8, 8, 8, 8))));

  _WifiAP_SSID = ensureNVSString(NVSKeys::WIFI_AP_SSID, "PDU_MAIN_AP");
  _WifiAP_Password = ensureNVSString(NVSKeys::WIFI_AP_PWD, "admin");   // default AP password
  
  _WifiAP_IP = convertStringToIPAddress(ensureNVSString(NVSKeys::WIFI_AP_IP, convertIPAddressToString(IPAddress(192, 168, 4, 1))));
  _WifiAP_Gateway = convertStringToIPAddress(ensureNVSString(NVSKeys::WIFI_AP_GATEWAY, convertIPAddressToString(IPAddress(192, 168, 4, 1))));
  _WifiAP_Subnet = convertStringToIPAddress(ensureNVSString(NVSKeys::WIFI_AP_SUBNET, convertIPAddressToString(IPAddress(255, 255, 255, 0))));
  _WifiAP_DNS = convertStringToIPAddress(ensureNVSString(NVSKeys::WIFI_AP_DNS, convertIPAddressToString(IPAddress(8, 8, 8, 8))));

  _EthernetIP = convertStringToIPAddress(ensureNVSString(NVSKeys::ETHERNET_IP, convertIPAddressToString(IPAddress(192, 168, 0, 5))));
  _EthernetGateway = convertStringToIPAddress(ensureNVSString(NVSKeys::ETHERNET_GATEWAY, convertIPAddressToString(IPAddress(192, 168, 0, 1))));
  _EthernetSubnet = convertStringToIPAddress(ensureNVSString(NVSKeys::ETHERNET_SUBNET, convertIPAddressToString(IPAddress(255, 255, 255, 0))));
  _EthernetDNS = convertStringToIPAddress(ensureNVSString(NVSKeys::ETHERNET_DNS, convertIPAddressToString(IPAddress(8, 8, 8, 8))));

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

bool networkLayerManager::getWifiAP_Status() {
  return WiFiAPStatus;
}

void networkLayerManager::configureWifiAP_SSID(String ssid) {
  _WifiAP_SSID = ssid;
  writeStringToNVS(NVSKeys::WIFI_AP_SSID, ssid);
}

void networkLayerManager::configureWifiAP_Password(String password) {
  _WifiAP_Password = password;
  writeStringToNVS(NVSKeys::WIFI_AP_PWD, password);
}

void networkLayerManager::configureWifiAP_IP(IPAddress ip) {
   _WifiAP_IP = ip;
   writeStringToNVS(NVSKeys::WIFI_AP_IP, convertIPAddressToString(_WifiAP_IP));
}

void networkLayerManager::configureWifiAP_Gateway(IPAddress gateway) {
  _WifiAP_Gateway = gateway;
   writeStringToNVS(NVSKeys::WIFI_AP_GATEWAY, convertIPAddressToString(_WifiAP_Gateway));
}

void networkLayerManager::configureWifiAP_Subnet(IPAddress subnet) {
  _WifiAP_Subnet = subnet;
  writeStringToNVS(NVSKeys::WIFI_AP_SUBNET, convertIPAddressToString(_WifiAP_Subnet));
}

void networkLayerManager::configureWifiAP_DNS(IPAddress dnsIP) {
  WiFi.config(_WifiAP_IP, _WifiAP_Gateway, _WifiAP_Subnet, dnsIP);
  writeStringToNVS(NVSKeys::WIFI_AP_DNS, convertIPAddressToString(dnsIP));
}

bool networkLayerManager::setWifiAP_Status(bool status) {
  if (status == true && !WiFiAPStatus && !getWifiSTA_Status() && _WifiAP_SSID != "" && _WifiAP_Password.length() >=8) {
    WiFi.softAPConfig(_WifiAP_IP, _WifiAP_Gateway, _WifiAP_Subnet);
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

bool networkLayerManager::getSTAEnabled() {
    return _staEnabled;
}

void networkLayerManager::setSTAEnabled(bool enabled) {
    _staEnabled = enabled;
    if (enabled) {
        if (WiFiAPStatus) WiFi.mode(WIFI_AP_STA);
        else WiFi.mode(WIFI_STA);
    } else {
        WiFi.disconnect(true);
        if (WiFiAPStatus) WiFi.mode(WIFI_AP);
        else WiFi.mode(WIFI_OFF);
        _startConnecting = false;
        WiFiSTAStatus = false;
    }
}

void networkLayerManager::connectSTA(String ssid, String pass) {
    if (!_staEnabled) setSTAEnabled(true);
    _WifiSTA_SSID = ssid;
    _WifiSTA_Password = pass;
    writeStringToNVS(NVSKeys::WIFI_STA_SSID, ssid);
    writeStringToNVS(NVSKeys::WIFI_STA_PWD, pass);
    
    WiFi.begin(ssid.c_str(), pass.c_str());
    _startConnecting = true;
}

void networkLayerManager::startAsyncScan() {
    if (!_isScanningMode) {
        WiFi.scanNetworks(true); // true = async
        _isScanningMode = true;
        _scanJSON = "[]";
    }
}

bool networkLayerManager::isScanComplete() {
    return !_isScanningMode;
}

String networkLayerManager::getScanResultsJSON() {
    return _scanJSON;
}

String networkLayerManager::getSTAStatusString() {
    if (!_staEnabled) return "Disabled";
    if (WiFi.status() == WL_CONNECTED) return "Connected to " + WiFi.SSID() + " (IP: " + WiFi.localIP().toString() + ")";
    if (_startConnecting) return "Connecting...";
    return "Disconnected";
}

void networkLayerManager::handleAsyncTasks() {
    if (_isScanningMode) {
        int16_t scanRes = WiFi.scanComplete();
        if (scanRes == WIFI_SCAN_RUNNING) {
        } else if (scanRes == WIFI_SCAN_FAILED) {
            _isScanningMode = false;
            _scanJSON = "[]";
        } else if (scanRes >= 0) {
            JsonDocument doc;
            JsonArray array = doc.to<JsonArray>();
            for (int i = 0; i < scanRes; ++i) {
                JsonObject obj = array.add<JsonObject>();
                obj["ssid"] = WiFi.SSID(i);
                obj["rssi"] = WiFi.RSSI(i);
                obj["secure"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
            }
            serializeJson(doc, _scanJSON);
            _isScanningMode = false;
            WiFi.scanDelete();
        }
    }

    if (_startConnecting) {
        if (WiFi.status() == WL_CONNECTED) {
            _startConnecting = false;
            WiFiSTAStatus = true;
        } else if (WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_NO_SSID_AVAIL) {
            _startConnecting = false;
            WiFiSTAStatus = false;
        }
    }
}


void networkLayerManager::scanNetworksJSON() {
  _startScanning = false;
  int n = WiFi.scanNetworks();
  JsonDocument doc;
  JsonArray array = doc.to<JsonArray>();

  for (int i = 0; i < n; ++i) {
    JsonObject obj = array.add<JsonObject>();
    obj["ssid"] = WiFi.SSID(i);
    obj["rssi"] = WiFi.RSSI(i);
    obj["secure"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
  }
  
  String output;
  Serial.print(output);
  serializeJson(doc, output);

  _scannedNetwork = output;
}

String networkLayerManager::getScannedNetworksJSON(){
  return _scannedNetwork;
}

void networkLayerManager::startScanning(){
  _startScanning = true;
}

bool networkLayerManager::isScanning(){
  return _startScanning;
}

bool networkLayerManager::isConnecting(){
  return _startConnecting;
}

bool networkLayerManager::startConnecting(){
  if(getWifiSTA_Status() == false) _startConnecting = true;
}

bool networkLayerManager::getWifiSTA_Status() {
  return WiFiSTAStatus;
}

bool networkLayerManager::setWifiSTA_Status(bool status) {
  if (status) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(_WifiSTA_SSID, _WifiSTA_Password);
    WiFiSTAStatus = false;
    Serial.println("WiFi csatlakozás elindítva a háttérben...");
    _startConnecting = true; 
    return true;
  } else {
    _startConnecting = false;
    WiFiSTAStatus = false;
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    return false;
  }
}

bool networkLayerManager::checkSTAConnection() {
    if (_startConnecting) {
        if (WiFi.status() == WL_CONNECTED) {
            WiFiSTAStatus = true;
            _startConnecting = false;
            return true;
        } else if (WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_NO_SSID_AVAIL) {
            WiFiSTAStatus = false;
            _startConnecting = false;
            return false;
        }
    }
}

void networkLayerManager::configureWifiSTA_SSID(String ssid) {
  _WifiSTA_SSID = ssid;
  writeStringToNVS(NVSKeys::WIFI_STA_SSID, ssid);
}

void networkLayerManager::configureWifiSTA_Password(String password) {
  _WifiSTA_Password = password;
  writeStringToNVS(NVSKeys::WIFI_STA_PWD, password);
}

void networkLayerManager::configureWifiSTA_IP(IPAddress ip) {
  _WifiSTA_IP = ip;
  writeStringToNVS(NVSKeys::WIFI_STA_IP, convertIPAddressToString(_WifiSTA_IP));
}

void networkLayerManager::configureWifiSTA_Gateway(IPAddress gateway) {
  _WifiSTA_Gateway = gateway;
  writeStringToNVS(NVSKeys::WIFI_STA_GATEWAY, convertIPAddressToString(_WifiSTA_Gateway));
}

void networkLayerManager::configureWifiSTA_Subnet(IPAddress subnet) {
  _WifiSTA_Subnet = subnet;
  writeStringToNVS(NVSKeys::WIFI_STA_SUBNET, convertIPAddressToString(_WifiSTA_Subnet));
}

void networkLayerManager::configureWifiSTA_DNS(IPAddress dnsIP) {
  _WifiSTA_DNS = dnsIP;
  writeStringToNVS(NVSKeys::WIFI_STA_DNS, convertIPAddressToString(_WifiSTA_DNS));
}


///----------------------------------------------------------------------------------------------
///------------------------------- Ethernet configuration --------------------------------------
///----------------------------------------------------------------------------------------------


//TODO this need to be modified to save to local variables too correctly!
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

void networkLayerManager::setEthernet_IP(IPAddress ip) {
  _EthernetIP = ip;
  writeStringToNVS(NVSKeys::ETHERNET_IP, convertIPAddressToString(ip));
}

void networkLayerManager::setEthernet_Gateway(IPAddress gateway) {
  _EthernetGateway = gateway;
  writeStringToNVS(NVSKeys::ETHERNET_GATEWAY, convertIPAddressToString(gateway));
}

void networkLayerManager::setEthernet_Subnet(IPAddress subnet) {
  _EthernetSubnet = subnet;
  writeStringToNVS(NVSKeys::ETHERNET_SUBNET, convertIPAddressToString(subnet));
}

void networkLayerManager::setEthernet_DNS(IPAddress dnsIP) {
  _EthernetDNS = dnsIP;
  writeStringToNVS(NVSKeys::ETHERNET_DNS, convertIPAddressToString(dnsIP));
}

void networkLayerManager::configureEthernet() {
  ETH.config(_EthernetIP, _EthernetGateway, _EthernetSubnet, _EthernetDNS);
}

bool networkLayerManager::getNetworkStatus() {
  return ESP32_W5500_isConnected();
}