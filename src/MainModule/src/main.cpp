#include <IECControl.h>
#include <networkLayerManager.h>
#include <variables.h>
#include <debug.h>
#include <PDU_webserver.h>

uint16_t deafultHttpPort = 80; // TODO need to be configurable from web server

HardwareSerial IEC_RS485Serial(2);
HardwareSerial PDU_RS485Serial(1);
IECControl* globalIEC;
AsyncWebServer asyncServer(deafultHttpPort);
networkLayerManager* networkLayer;
PDU_webserver* webserver;

void setup() {
  Serial.begin(115200);

  Serial.setDebugOutput(true);

  globalIEC = new IECControl(IEC_RS485Serial); // Initialize the IEC control with the RS485 serial port
  webserver = new PDU_webserver(&asyncServer, globalIEC);

  std::vector<uint8_t> IECNumber = globalIEC->getFoundIECIDs(); // Discover the IEC modules on the RS485 bus

  delay(1000);

  networkLayer = new networkLayerManager(); // Initialize the network layer manager
  networkLayer->initInternetProtocol();

  webserver->runServer(); // Initialize the web server

  globalIEC->setpowerDataUpdateCycleTime(1000); // Set the power data update cycle time to 1 second (1000 ms)
}

void loop() {

  //globalIEC->IECReadLoop(); // Read the IEC power data over modbus
}