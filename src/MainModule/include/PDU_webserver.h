#ifndef PDU_webserver_h
#define PDU_webserver_h

#include <networkLayerManager.h>
#include <SPIFFS.h>
#include <AsyncWebSocket.h>
#include <IECControl.h>
#include <EnvironmentSensor.h>

#include <ArduinoJson.h>
#include "variables.h"
#include <mqttHandler.h> 

class PDU_webserver {
public:
    PDU_webserver(AsyncWebServer *server, IECControl *iecControl, EnvironmentSensor *envSensor, networkLayerManager *networkLayer);

    void runServer(); 
    void broadcastModules();          // WS frissítés
    void setUpdateInterval(uint32_t ms); // IEC frissítési ciklus

private:
    AsyncWebServer *webServer;
    IECControl *iec;
    AsyncWebSocket ws;
    EnvironmentSensor *_envSensor; // Környezeti szenzor (hőmérséklet)
    networkLayerManager *_networkLayer; // Hálózati réteg kezelő

    bool hasModuleId(uint8_t id);
    void setRelayStatusWeb(uint8_t id, uint8_t relay, bool status);
    void setAllRelayStatusWeb(bool status);

    unsigned long lastModuleUpdate[256] = {0};
    uint32_t updateInterval = 1000; // ms, alapértelmezett 1s
    uint32_t lastMillis = 0;
};

#endif
