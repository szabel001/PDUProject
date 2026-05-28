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
    void broadcastModules();    // Brodcast via websocket the data of all modules
    void setUpdateInterval(uint32_t ms);

private:
    AsyncWebServer *webServer;
    IECControl *iec;
    AsyncWebSocket ws;
    EnvironmentSensor *_envSensor;
    networkLayerManager *_networkLayer;

    bool hasModuleId(uint8_t id);
    void setRelayStatusWeb(uint8_t id, uint8_t relay, bool status);
    void setAllRelayStatusWeb(bool status);

    unsigned long lastModuleUpdate[256] = {0};
    uint32_t updateInterval = 1000;
    uint32_t lastMillis = 0;
};

#endif
