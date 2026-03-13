#ifndef MQTTHANDLER_H
#define MQTTHANDLER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include "IECControl.h"
#include "variables.h"
#include <ArduinoJson.h>

class MqttHandler {
public:
    // A konstruktorban átadjuk az IECControl pointerét
    MqttHandler(IECControl* iecControl);
    
    void setupMQTT();
    void handleMQTT();
    
    // Állapot lekérdező függvények
    String getMQTTStatusString();
    bool isMQTTEnabled();

private:
    IECControl* _iec;            // Belső pointer a modulokhoz
    WiFiClient espClient;
    PubSubClient mqttClient;

    String mqttServerIP;
    int mqttServerPort;
    bool mqttEnabled;
    bool firstTry;

    unsigned long lastMqttPublish;
    unsigned long lastConnectAttempt;
    unsigned long lastNvsCheck;

    // A PubSubClient C-stílusú callbacket vár, ezért ennek static-nak kell lennie
    static void mqttCallback(char* topic, byte* payload, unsigned int length);
};

extern MqttHandler* mqttManager;

#endif