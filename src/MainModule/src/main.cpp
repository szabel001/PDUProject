#include <IECControl.h>
#include <networkLayerManager.h>
#include <variables.h>
#include <debug.h>
#include <PDU_webserver.h>
#include <TFTDisplay.h>
#include <mqttHandler.h>

uint16_t deafultHttpPort = 80;

// Global instances
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

  // Initialize the network layer manager
  networkLayer = new networkLayerManager(); 
  networkLayer->initInternetProtocol();

  // Initialize the environment sensor
  envSensor = new EnvironmentSensor();
  envSensor->setupEnvironmentSensor();
  webserver = new PDU_webserver(&asyncServer, globalIEC, envSensor, networkLayer);

  // Initialize the MQTT handler
  mqttManager = new MqttHandler(globalIEC);
  mqttManager->setupMQTT();

  // Initialize the web server
  webserver->runServer(); 

  // Initialize the TFT display with the IEC control reference
  tftDisplay.setupDisplay(*globalIEC, *networkLayer, *envSensor); 
}

void loop() {
  globalIEC->IECReadLoop(); // Read the IEC power data over modbus
  tftDisplay.processButton(); // Process button inputs to avoid multiple clicks
  tftDisplay.updateCursor();  // Update the cursor position based on button inputs
  tftDisplay.updateActiveMenuPeriodic(); // Periodically update the dinamic values in TFT display

  webserver->broadcastModules();  // Broadcast the data of all modules via websocket to update the web interface

  networkLayer->handleAsyncTasks(); // Handle asynchronous tasks related to network operations (e.g., Wi-Fi scanning, connection management)
  mqttManager->handleMQTT(); // Handle MQTT communication, including publishing data and processing incoming messages
}