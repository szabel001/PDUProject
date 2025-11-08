#ifndef PDU_webserver_h
#define PDU_webserver_h

#include <networkLayerManager.h>
#include <SPIFFS.h>
#include <IECControl.h>

class PDU_webserver {
  public:
  PDU_webserver(AsyncWebServer *server, IECControl *iecModule); // Constructor to initialize the web server with the given port
  void handleApiData(AsyncWebServerRequest *request);
  void initWeb();
  // and IEC module control reference
  void runServer(); // Initialize the web server with the given port
                    // configokat, beállításokat
                    // frissítés után adaptálódjon minden

  private:
  bool _checkEthernetConnection(); // Check if the Ethernet connection is available - connected cable and hardware
  AsyncWebServer *webServer;
  IECControl *module;
};
#endif