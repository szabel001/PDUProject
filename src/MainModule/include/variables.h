#ifndef VARIABLES_H
#define VARIABLES_H

#include <Preferences.h>

void setupNVS();
void writeStringToNVS(const char* key, String value);
String readStringFromNVS(const char* key, String defaultValue);
void writeIntToNVS(const char* key, int32_t value);
int32_t readIntFromNVS(const char* key, int32_t defaultValue);

#endif