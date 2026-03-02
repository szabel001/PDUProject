#include <variables.h>

Preferences prefs;

String ensureNVSString(const char* key, const String& value) {
  if (readStringFromNVS(key, "").isEmpty()) {
    writeStringToNVS(key, value);
    Serial.printf("NVS key '%s' was empty. Written value '%s'\n", key, value.c_str());
  }
  return readStringFromNVS(key, value);
}

uint32_t ensureNVSInt(const char* key, int32_t value) {
  if (readIntFromNVS(key, -1) == -1) {
    writeIntToNVS(key, value);
    Serial.printf("NVS key '%s' was empty. Written value '%d'\n", key, value);
  }
  return readIntFromNVS(key, value);
}

void writeStringToNVS(const char* key, String value) {
  if (readStringFromNVS(key, "") == value) return;
  Serial.printf("NVS: Writing key '%s' with value '%s'\n", key, value.c_str());
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
