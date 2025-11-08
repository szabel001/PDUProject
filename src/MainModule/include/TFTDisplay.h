#ifndef TFTDisplay_h
#define TFTDisplay_h

#include "TFT_eSPI.h"
#include <pinouts.h>
#include <IECControl.h>

class TFTDisplay : public TFT_eSPI {
  public:
    TFTDisplay();
    uint8_t rotation = 0;
    uint8_t textSize = 0;

    void setupDisplay(IECControl& iec);
    void runDisplay(String str);  // Display the given string on the screen for testing until full implementation is done
    void drawScreen_welcome();
    void drawScreen_menu();

  private:
    void rollScreen();
    //TODO need to be refined after frontend integration
    void drawScreen_settings();
    void drawScreen_subSetting();
    void drawScreen_IECStatus();
    void drawScreen_PDUStatus();
    void drawScreen_AHT10();

    TFT_eSPI _tft;
    IECControl* _iec;

    // singleton pointer, hogy a C-style ISR-ek elérjék az aktuális példányt
    static TFTDisplay* _instance;

    // statikus ISR-wrapper-ek (attachInterrupt-hez)
    static void IRAM_ATTR isr_handleUp();
    static void IRAM_ATTR isr_handleDown();
    static void IRAM_ATTR isr_handleBack();
    static void IRAM_ATTR isr_handleConfirm();

    // debouncing-et a fő ciklusban végezd el
    void processButtonDebounce();


        // gombokhoz: ISR által állított raw flag-ek (minimális ISR munka)
    volatile bool rawUpPressed = false;
    volatile bool rawDownPressed = false;
    volatile bool rawBackPressed = false;
    volatile bool rawConfirmPressed = false;

    // debounced állapotok (fő ciklus használja)
    volatile bool upPressed = false;
    volatile bool downPressed = false;
    volatile bool backPressed = false;
    volatile bool confirmPressed = false;

    // last press times (ms) — privát, a debouncinghez
    volatile unsigned long lastUpTime = 0;
    volatile unsigned long lastDownTime = 0;
    volatile unsigned long lastBackTime = 0;
    volatile unsigned long lastConfirmTime = 0;

    // menü/állapot mezők
    int currentMenu = 0;
    int selectedOutlet = 10;
    bool relayState = false;

    unsigned long startMillis = 0;
    unsigned long currentMillis;
  };
#endif