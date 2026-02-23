#ifndef MQTTHANDLER_H
#define MQTTHANDLER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include "IECControl.h" // <-- Fontos! Be kell húzni a típust, hogy tudja mi az az IECControl
#include "variables.h"  // Elérjük az NVS író/olvasó függvényeket

// Ezzel jelezzük, hogy a globalIEC a main.cpp-ben van létrehozva!
extern IECControl* globalIEC;

void setupMQTT();
void handleMQTT();

#endif