#include <variables.h>

Preferences prefs;

void writeStringToNVS(const char* key, String value) {
  if (readStringFromNVS(key, "") == value) return;
  prefs.begin("config", false);
  prefs.putString(key, value);
  prefs.end();
}

String readStringFromNVS(const char* key, String defaultValue) {
  prefs.begin("config", true);
  String value = prefs.getString(key, defaultValue);
  prefs.end();
  return value;
}

void writeIntToNVS(const char* key, int32_t value) {
  if (readIntFromNVS(key, 0) == value) return;
  prefs.begin("config", false);
  prefs.putInt(key, value);
  prefs.end();
}

int32_t readIntFromNVS(const char* key, int32_t defaultValue) {
  prefs.begin("config", true);
  int32_t value = prefs.getInt(key, defaultValue);
  prefs.end();
  return value;
}
