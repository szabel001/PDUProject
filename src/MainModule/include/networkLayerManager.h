#ifndef networkLayerManager_h
#define networkLayerManager_h

#if !( defined(ESP32) )
  #error This code is designed for (ESP32 + W5500) to run on ESP32 platform! Please check your Tools->Board setting.
#endif

#include <AsyncWebServer_ESP32_W5500.h>
#include <AsyncTCP.h>
#include <pinouts.h>
#include <variables.h>
#include <debug.h>
#include <ArduinoJson.h>

IPAddress convertStringToIPAddress(const String& str);
String convertIPAddressToString(IPAddress ip);

enum wifiMode {
  OFF = WIFI_MODE_NULL, // No Wi-Fi mode
  STA = WIFI_MODE_STA, // Station mode
  AP = WIFI_MODE_AP, // Access Point mode
};

class networkLayerManager {
  public:
  networkLayerManager(){}
  void initInternetProtocol();
  void setupAPWifi(bool status);

  String scanNetworksJSON();

  bool getWifiSTA_Status();
  bool setWifiSTA_Status(bool status);
  void configureWifiSTA_SSID(String ssid);
  void configureWifiSTA_Password(String password);
  void configureWifiSTA_IP(IPAddress ip);
  void configureWifiSTA_Gateway(IPAddress gateway);
  void configureWifiSTA_Subnet(IPAddress subnet);
  void configureWifiSTA_DNS(IPAddress dnsIP);

  bool getWiFiAP_Status();
  bool setWifiAP_Status(bool status);
  void configureWifiAP_SSID(String ssid);
  void configureWifiAP_Password(String password);
  void configureWifiAP_IP(IPAddress ip);
  void configureWifiAP_Gateway(IPAddress gateway);
  void configureWifiAP_Subnet(IPAddress subnet);
  void configureWifiAP_DNS(IPAddress dnsIP);


  String getEthernetIP();
  void setEthernetDHCP(bool status);
  bool getEthernetDHCPStatus();
  void setEthernet_IP(IPAddress ip);
  void setEthernet_Gateway(IPAddress gateway);
  void setEthernet_Subnet(IPAddress subnet);
  void setEthernet_DNS(IPAddress dnsIP);
  void configureEthernet();
  void configureEthernet(uint8_t ip[], uint8_t gateway[], uint8_t subnet[]);

  bool getNetworkStatus();

  WiFiClass WiFi;
  WiFiClient client;
  AsyncWebServer* webServer;

  private:
  bool _checkEthernetConnection();       // Check if the Ethernet connection is available - connected cable and hardware
  bool _networkStatus;                   //
  bool WiFiSTAStatus;
  bool WiFiAPStatus;

  float _currentData;                    //

  wifiMode _actualWifiMode;              // Wi-Fi mode - OFF, STA, AP

  String _WifiAP_SSID;                // Wi-Fi SSID. password -> when it is used as AP (client can be connected to this AP with these credentials)
  String _WifiAP_Password;            // belonding to the ESP's network

  String _WifiSTA_SSID;               // Wi-Fi SSID. password -> when it is used as STA (ESP can connect to a router with these credentials)
  String _WifiSTA_Password;           // external network's credentials

  IPAddress _WifiAP_IP;
  IPAddress _WifiAP_Subnet;
  IPAddress _WifiAP_Gateway;
  IPAddress _WifiAP_DNS;
  
  IPAddress _WifiSTA_IP;
  IPAddress _WifiSTA_Subnet;
  IPAddress _WifiSTA_Gateway;
  IPAddress _WifiSTA_DNS;

  IPAddress _EthernetIP;
  IPAddress _EthernetSubnet;
  IPAddress _EthernetGateway;
  IPAddress _EthernetDNS;

  IPAddress _actMQTTBroker;
  uint8_t _actMQTTPort;
  String _actMQTTUser;
  String _actMQTTPassword;
  String _actMQTTClientID;
};
#endif