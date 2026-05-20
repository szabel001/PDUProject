#include <IECControl.h>
#include <networkLayerManager.h>
#include <variables.h>
#include <debug.h>
#include <PDU_webserver.h>
#include <TFTDisplay.h>
#include <mqttHandler.h>

uint16_t deafultHttpPort = 80;

HardwareSerial IEC_RS485Serial(2);
HardwareSerial PDU_RS485Serial(1);
IECControl* globalIEC;
AsyncWebServer asyncServer(deafultHttpPort);
networkLayerManager* networkLayer;
PDU_webserver* webserver;
TFTDisplay tftDisplay;
EnvironmentSensor* envSensor;
MqttHandler* mqttManager;

void setup() {
  Serial.begin(115200);
  globalIEC = new IECControl(IEC_RS485Serial); // Initialize the IEC control with the RS485 serial port
  std::vector<uint8_t> IECNumber = globalIEC->getFoundIECIDs(); // Discover the IEC modules on the RS485 bus

  networkLayer = new networkLayerManager(); // Initialize the network layer manager
  networkLayer->initInternetProtocol();

  envSensor = new EnvironmentSensor();
  envSensor->setupEnvironmentSensor();
  webserver = new PDU_webserver(&asyncServer, globalIEC, envSensor, networkLayer);

  // MQTT példányosítása és setup
  mqttManager = new MqttHandler(globalIEC);
  mqttManager->setupMQTT();

  webserver->runServer(); // Initialize the web server

  tftDisplay.setupDisplay(*globalIEC, *networkLayer, *envSensor); // Initialize the TFT display with the IEC control reference
}

void loop() {
  globalIEC->IECReadLoop(); // Read the IEC power data over modbus
  tftDisplay.processButton();
  tftDisplay.updateCursor();
  tftDisplay.updateActiveMenuPeriodic();

  webserver->broadcastModules();

  networkLayer->handleAsyncTasks();
  mqttManager->handleMQTT();
}