#ifndef VARIABLES_H
#define VARIABLES_H

#include <Preferences.h>

namespace NVSKeys {
  constexpr const char* WIFI_AP_ENA = "wifi_ap_ena";
  constexpr const char* WIFI_AP_SSID = "ssid_ap";
  constexpr const char* WIFI_AP_PWD  = "pwd_ap";

  constexpr const char* WIFI_AP_IP       = "wifi_ap_ip";
  constexpr const char* WIFI_AP_GATEWAY  = "wifi_ap_gw";
  constexpr const char* WIFI_AP_SUBNET   = "wifi_ap_sn";
  constexpr const char* WIFI_AP_DNS      = "wifi_ap_dns";

  constexpr const char* WIFI_STA_SSID = "ssid_sta";
  constexpr const char* WIFI_STA_PWD  = "pwd_sta";

  constexpr const char* WIFI_STA_IP       = "wifi_sta_ip";
  constexpr const char* WIFI_STA_GATEWAY  = "wifi_sta_gw";
  constexpr const char* WIFI_STA_SUBNET   = "wifi_sta_sn";
  constexpr const char* WIFI_STA_DNS      = "wifi_sta_dns";

  constexpr const char* ETHERNET_DHCP     = "eth_dhcp";
  constexpr const char* ETHERNET_IP       = "eth_ip";
  constexpr const char* ETHERNET_GATEWAY  = "eth_gw";
  constexpr const char* ETHERNET_SUBNET   = "eth_sn";
  constexpr const char* ETHERNET_DNS      = "eth_dns";

  constexpr const char* MEAS_OC       = "meas_oc";
  constexpr const char* MEAS_TEMP     = "meas_temp";
  constexpr const char* MEAS_CYCLE    = "meas_cycle";
  constexpr const char* MEAS_DELAY    = "meas_delay";

  constexpr const char* MQTT_ENA      = "mqtt_ena";
  constexpr const char* MQTT_SERVER   = "mqtt_srv";
  constexpr const char* MQTT_PORT     = "mqtt_port";
}

String ensureNVSString(const char* key, const String& value);
uint32_t ensureNVSInt(const char* key, int32_t value);
void writeStringToNVS(const char* key, String value);
String readStringFromNVS(const char* key, String defaultValue);
void writeIntToNVS(const char* key, int32_t value);
int32_t readIntFromNVS(const char* key, int32_t defaultValue);

#endif