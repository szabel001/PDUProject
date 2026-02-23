#ifndef PDU_webserver_h
#define PDU_webserver_h

#include <networkLayerManager.h>
#include <SPIFFS.h>
#include <AsyncWebSocket.h>
#include <IECControl.h>
#include <ArduinoJson.h>
#include "variables.h"     // Elérjük az NVS író/olvasó függvényeket

class PDU_webserver {
public:
    PDU_webserver(AsyncWebServer *server, IECControl *iecControl);

    void runServer();
    void broadcastModules();          // WS frissítés
    void setUpdateInterval(uint32_t ms); // IEC frissítési ciklus

private:
    AsyncWebServer *webServer;
    IECControl *iec;
    AsyncWebSocket ws;

    bool hasModuleId(uint8_t id);
    void setRelayStatusWeb(uint8_t id, uint8_t relay, bool status);

    uint32_t updateInterval = 1000; // ms, alapértelmezett 1s
    uint32_t lastMillis = 0;
};

#endif
