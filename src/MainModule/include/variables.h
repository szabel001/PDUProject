#ifndef VARIABLES_H
#define VARIABLES_H

#include <Preferences.h>

namespace NVSKeys {
  constexpr const char* WIFI_AP_SSID = "ssid_ap";
  constexpr const char* WIFI_AP_PWD  = "pwd_ap";

  constexpr const char* WIFI_STA_SSID = "ssid_sta";
  constexpr const char* WIFI_STA_PWD  = "pwd_sta";

  constexpr const char* WIFI_IP       = "wifi_ip";
  constexpr const char* WIFI_GATEWAY  = "wifi_gateway";
  constexpr const char* WIFI_SUBNET   = "wifi_subnet";
  constexpr const char* WIFI_DNS      = "wifi_dns";

  constexpr const char* ETHERNET_IP       = "eth_ip";
  constexpr const char* ETHERNET_GATEWAY  = "eth_gateway";
  constexpr const char* ETHERNET_SUBNET   = "eth_subnet";
  constexpr const char* ETHERNET_DNS      = "eth_dns";
}

void writeStringToNVS(const char* key, String value);
String readStringFromNVS(const char* key, String defaultValue);
void writeIntToNVS(const char* key, int32_t value);
int32_t readIntFromNVS(const char* key, int32_t defaultValue);

#endif