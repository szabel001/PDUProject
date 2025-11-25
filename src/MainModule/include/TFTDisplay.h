#ifndef TFTDisplay_h
#define TFTDisplay_h

#include <TFT_eSPI.h>
#include <IECControl.h>

enum menuState {
  SETTINGS,
  IEC,
  PDU_STATUS,
  AHT10,

  IEC_INFO,
  IEC_STATUS,
  IEC_SETTINGS,
};

struct MenuItem {
  const char* name;
  menuState nextMenu;
  String value;
};

struct Menu {
  const char* title;
  MenuItem* items;
  uint8_t itemCount;
  Menu* parent;
};

class TFTDisplay {
public:
  TFTDisplay();

  void setupDisplay(IECControl& iec);
  void setupMenu();
  void processButtonDebounce();
  void updateIECDetailMenus(int id);
  void drawRelayIcon(bool state, int x, int y, uint16_t bg);
  void updateActiveMenuPeriodic();
  void updateDynamicValues();
  void drawMenuWindow(); // görgetett menü ablak
  void updateMenuValues();
  void updateCursor(); // kurzor mozgatás

  private:
  MenuItem mainItems[10];      // bővíthető
  MenuItem settingsItems[5];

  Menu mainMenu;
  Menu settingsMenu;

  MenuItem iecItems[10];
  Menu iecMenu;

  MenuItem iecModulesItems[20];   // max 20 modul
  Menu iecModulesMenu;

  Menu iecInfoDetailMenu;
  MenuItem iecInfoDetailItems[10];

  Menu iecStatusDetailMenu;
  MenuItem iecStatusDetailItems[10];

  Menu iecSettingDetailMenu;
  MenuItem iecSettingDetailItems[10];



  Menu* currentMenu;
  volatile int currentSelection = 0;
  int lastSelection = -1;
  int selectedIECModuleID = -1;

  unsigned long lastAutoUpdate = 0;
  const unsigned long autoUpdateInterval = 1000; // ms


  TFT_eSPI _tft;
  IECControl* _iec;

  int windowStart = 0;         // első látható menüpont a windowban
  const int maxVisible = 4;    // maximum menüpont a kijelzőn egyszerre
  const int rowHeight = 20;
  uint8_t startOfTexts = 35;
  uint8_t startOfTextsLeft = 5;
  uint8_t startOfTextsFromBlue = 3;

  static TFTDisplay* _instance;

  static void IRAM_ATTR isr_handleUp();
  static void IRAM_ATTR isr_handleDown();
  static void IRAM_ATTR isr_handleBack();
  static void IRAM_ATTR isr_handleConfirm();
  bool interruptTimer();

  volatile unsigned long lastButtonPressed = 0;

  volatile bool rawUpPressed = false;
  volatile bool rawDownPressed = false;
  volatile bool rawBackPressed = false;
  volatile bool rawConfirmPressed = false;
  volatile bool rawAnyButton = false;

  volatile unsigned long lastUpTime = 0;
  volatile unsigned long lastDownTime = 0;
  volatile unsigned long lastBackTime = 0;
  volatile unsigned long lastConfirmTime = 0;
};

#endif
