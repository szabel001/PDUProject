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
  void configureWifiSSID(String ssid);
  void configureWifiPassword(String password);
  bool getWiFiAPStatus();
  void configureWifiIP(uint8_t ip[]);
  void configureWifiGateway(uint8_t gateway[]);
  void configureWifiSubnet(uint8_t subnet[]);
  void configureWifiDNS(uint8_t dns[]);
  bool turnOnWifiAP(bool status);
  bool getWiFiSTAStatus();
  bool setfWifiSTA(bool status);
  String getEthernetIP();
  void setEthernetDHCP(bool status);
  bool getEthernetDHCPStatus();
  void configureEthernet_IP(uint8_t ip[]);
  void setEthernet_Gateway(uint8_t gateway[]);
  void setEthernet_Subnet(uint8_t subnet[]);
  void setEthernet_DNS(uint8_t dns[]);
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

  IPAddress _WifiIP;
  IPAddress _WifiSubnet;
  IPAddress _WifiGateway;

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