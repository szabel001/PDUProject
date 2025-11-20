#ifndef PDU_webserver_h
#define PDU_webserver_h

#include <networkLayerManager.h>
#include <SPIFFS.h>
#include <IECControl.h>

// Webserver osztály — több IEC modul támogatásával
class PDU_webserver {
  public:
    // Konstruktor: átadod a webszerver példányt, az IECControl referenciát és a modul ID-k listáját
    PDU_webserver(AsyncWebServer *server, IECControl *iec);

    // Indítja a webservert és beállítja a route-okat
    void runServer();

  private:
    AsyncWebServer *webServer;
    IECControl *iec;

    // Segédfüggvény: ellenőrzi, hogy a moduleIds listában szerepel-e a megadott mod ID
    bool hasModuleId(uint8_t id);
    void setRelayStatusWeb(uint8_t id, bool status);
    bool currentRelayStatus = false;
    bool toggleStatus = false;
};

#endif // PDU_webserver_h
