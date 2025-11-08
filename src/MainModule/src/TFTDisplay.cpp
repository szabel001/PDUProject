#include "TFTDisplay.h"

#define DEBOUNCE_DELAY 150 // debounce time (ms)

TFTDisplay* TFTDisplay::_instance = nullptr;

TFTDisplay::TFTDisplay() : _tft(TFT_eSPI()){
  _instance = this;

  rawUpPressed = rawDownPressed = rawBackPressed = rawConfirmPressed = false;
  upPressed = downPressed = backPressed = confirmPressed = false;

  currentMenu = 0;
  selectedOutlet = 10;
  relayState = false;

  lastUpTime = lastDownTime = lastBackTime = lastConfirmTime = 0;
  startMillis = 0;

  currentMenu = 0;
  selectedOutlet = 10;
  relayState = false;

  lastUpTime = 0;
  lastDownTime = 0;
  lastBackTime = 0;
  lastConfirmTime = 0;
}

void IRAM_ATTR TFTDisplay::isr_handleUp()      { if (_instance) _instance->rawUpPressed = true; }
void IRAM_ATTR TFTDisplay::isr_handleDown()    { if (_instance) _instance->rawDownPressed = true; }
void IRAM_ATTR TFTDisplay::isr_handleBack()    { if (_instance) _instance->rawBackPressed = true; }
void IRAM_ATTR TFTDisplay::isr_handleConfirm() { if (_instance) _instance->rawConfirmPressed = true; }

void IRAM_ATTR debounceButton(volatile bool &buttonFlag, volatile unsigned long &lastPressTime) {
  unsigned long currentTime = millis();
  if (currentTime - lastPressTime > DEBOUNCE_DELAY) {
      buttonFlag = true;
      lastPressTime = currentTime;
  }
}

struct currentMenuState {
  int menu; // Current menu index
  int subMenu;
  float currentData; // Current data value
};

enum menuState {
  WELCOME,
  MENU,
  SETTINGS,
  SUBSETTING,
  IEC_STATUS,
  PDU_STATUS,
  AHT10
};

enum subMenuState {
  OUTLET_SETTINGS,
  OUTLET_STATUS,
  AHT10_SETTINGS,
  AHT10_STATUS
};

void TFTDisplay::setupDisplay(IECControl& iec){
  _iec = &iec; // Store a pointer to the IECControl object
  _tft.init();
  _tft.setRotation(1);
  _tft.fillScreen(TFT_WHITE);
  _tft.setTextColor(TFT_BLACK);
  _tft.drawString("PDU PROJECT", 10, 50);

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);
  pinMode(BTN_CONFIRM, INPUT_PULLUP);

  attachInterrupt(BTN_UP, TFTDisplay::isr_handleUp, FALLING);
  attachInterrupt(BTN_DOWN, TFTDisplay::isr_handleDown, FALLING);
  attachInterrupt(BTN_BACK, TFTDisplay::isr_handleBack, FALLING);
  attachInterrupt(BTN_CONFIRM, TFTDisplay::isr_handleConfirm, FALLING);

  startMillis = millis();
}

void TFTDisplay::processButtonDebounce() {
  unsigned long now = millis();

  if (rawUpPressed) {
    rawUpPressed = false;
    if (now - lastUpTime > DEBOUNCE_DELAY) {
      upPressed = true;
      lastUpTime = now;
    }
  }
  if (rawDownPressed) {
    rawDownPressed = false;
    if (now - lastDownTime > DEBOUNCE_DELAY) {
      downPressed = true;
      lastDownTime = now;
    }
  }
  if (rawBackPressed) {
    rawBackPressed = false;
    if (now - lastBackTime > DEBOUNCE_DELAY) {
      backPressed = true;
      lastBackTime = now;
    }
  }
  if (rawConfirmPressed) {
    rawConfirmPressed = false;
    if (now - lastConfirmTime > DEBOUNCE_DELAY) {
      confirmPressed = true;
      lastConfirmTime = now;
    }
  }
}

void TFTDisplay::runDisplay(String str) {
  _tft.fillScreen(TFT_WHITE);
  _tft.drawString(str, 10, 50);
}

void TFTDisplay::drawScreen_welcome() {
}

void TFTDisplay::rollScreen() {
}

void TFTDisplay::drawScreen_menu() {

  processButtonDebounce();
  
  if (upPressed) {
    upPressed = false;
    if (currentMenu == 0) selectedOutlet = max(1, selectedOutlet - 1);
    Serial.printf("Selected Outlet: %d\n", selectedOutlet);
    runDisplay("Selected Outlet: " + String(selectedOutlet));
  }

  if (downPressed) {
      downPressed = false;
      if (currentMenu == 0) selectedOutlet++;
      Serial.printf("Selected Outlet: %d\n", selectedOutlet);
      runDisplay("Selected Outlet: " + String(selectedOutlet));
    }

  if (backPressed) {
      backPressed = false;
      if (currentMenu > 0) currentMenu--;
      Serial.printf("Menu: %d\n", currentMenu);
      runDisplay("Menu:Enter outlet sett " + String(currentMenu));
  }

  if (confirmPressed) {
      confirmPressed = false;
      if (currentMenu == 0) {
          currentMenu = 1; // Enter outlet settings
      } else if (currentMenu == 1) {
        bool relayState = _iec->getRelayStatus(7); // Get the relay status for the selected outlet
          relayState = !relayState; // Toggle the relay state
          Serial.printf("Relay State: %s\n", relayState ? "ON" : "OFF");
          runDisplay("Relay State: " + relayState ? "ON" : "OFF");
      }
  }
}

void TFTDisplay::drawScreen_settings() {
}

void TFTDisplay::drawScreen_subSetting() {
}

void TFTDisplay::drawScreen_IECStatus() {
}

void TFTDisplay::drawScreen_PDUStatus() {
}

void TFTDisplay::drawScreen_AHT10() {
}

