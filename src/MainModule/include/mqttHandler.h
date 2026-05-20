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
    MqttHandler(IECControl* iecControl);
    
    void setupMQTT();
    void handleMQTT();
    
    String getMQTTStatusString();
    bool isMQTTEnabled();

private:
    IECControl* _iec;
    WiFiClient espClient;
    PubSubClient mqttClient;

    String mqttServerIP;
    int mqttServerPort;
    bool mqttEnabled;
    bool firstTry;

    unsigned long lastMqttPublish;
    unsigned long lastConnectAttempt;
    unsigned long lastNvsCheck;

    static void mqttCallback(char* topic, byte* payload, unsigned int length);
};

extern MqttHandler* mqttManager;

#endif