#ifndef VARIABLES_H
#define VARIABLES_H

#include <Preferences.h>

namespace NVSKeys {
  constexpr const char* WIFI_AP_ENA = "wifi_ap_ena";
  constexpr const char* WIFI_AP_SSID = "ssid_ap";
  constexpr const char* WIFI_AP_PWD  = "pwd_ap";

  constexpr const char* WIFI_STA_SSID = "ssid_sta";
  constexpr const char* WIFI_STA_PWD  = "pwd_sta";

  constexpr const char* WIFI_IP       = "wifi_ip";
  constexpr const char* WIFI_GATEWAY  = "wifi_gateway";
  constexpr const char* WIFI_SUBNET   = "wifi_subnet";
  constexpr const char* WIFI_DNS      = "wifi_dns";

  constexpr const char* ETHERNET_DHCP     = "eth_dhcp";
  constexpr const char* ETHERNET_IP       = "eth_ip";
  constexpr const char* ETHERNET_GATEWAY  = "eth_gateway";
  constexpr const char* ETHERNET_SUBNET   = "eth_subnet";
  constexpr const char* ETHERNET_DNS      = "eth_dns";

  constexpr const char* MEAS_OC       = "meas_oc";
  constexpr const char* MEAS_TEMP     = "meas_temp";
  constexpr const char* MEAS_CYCLE    = "meas_cycle";
  constexpr const char* MEAS_DELAY    = "meas_delay";

  constexpr const char* MQTT_ENA   = "mqtt_ena";
  constexpr const char* MQTT_SERVER   = "mqtt_srv";
  constexpr const char* MQTT_PORT     = "mqtt_port";
}

void writeStringToNVS(const char* key, String value);
String readStringFromNVS(const char* key, String defaultValue);
void writeIntToNVS(const char* key, int32_t value);
int32_t readIntFromNVS(const char* key, int32_t defaultValue);

#endif