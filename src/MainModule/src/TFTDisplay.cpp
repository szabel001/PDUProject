// TFTDisplay.cpp (modular rewrite)

#include "TFTDisplay.h"

TFTDisplay* TFTDisplay::_instance = nullptr;

TFTDisplay::TFTDisplay() {
  _instance = this;
}

int TFTDisplay::addMenu(const char* title, int parentId) {
  int id = menus.size();
  menus.emplace_back(id, title, parentId);
  return id;
}

void TFTDisplay::addMenuItem(int menuId, const MenuItem& item) {
  int idx = findMenuIndexById(menuId);
  if (idx >= 0) {
    menus[idx].items.push_back(item);
  }
}

int TFTDisplay::findMenuIndexById(int id) {
  for (size_t i = 0; i < menus.size(); i++) {
    if (menus[i].id == id) return i;
  }
  return -1;
}

Menu *TFTDisplay::getMenuById(int id) {
    for (size_t i = 0; i < menus.size(); i++) {
    if (menus[i].id == id) return &menus[i];
  }
  return nullptr;
}

void TFTDisplay::drawBackButton(bool selected) {
    _tft.fillRect(0, 90, 60, 20, TFT_BLACK);
    if(selected)
        _tft.setTextColor(TFT_YELLOW, TFT_BLUE);
    else
        _tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    _tft.drawString("BACK", 10, 90);
}


void TFTDisplay::drawSaveButton(bool selected) {
    int charWidth = 7;
    _tft.fillRect(_tft.width() - 10 - _tft.textWidth("SAVE"), 90, _tft.textWidth("SAVE"), 20, TFT_BLACK);
        if(selected)
        _tft.setTextColor(TFT_YELLOW, TFT_BLUE);
    else
        _tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    _tft.drawString("SAVE", _tft.width() - 10 - _tft.textWidth("SAVE"), 90);
}

void TFTDisplay::startEditing(const String& initialValue,
                              const String& allowedChars,
                              std::function<void(const String&)> saveCb)
{
    editing = true;
    editor.begin(initialValue, allowedChars);
    editorSaveCallback = saveCb;

    _tft.fillScreen(TFT_BLACK);
    _tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    _tft.drawString("Edit mode", 10, 10);

    drawEditorScreen();
}

void TFTDisplay::drawEditorScreen() {
    _tft.fillRect(0, 40, _tft.width(), 40, TFT_BLACK);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    String displayStr = editor.get();
    _tft.drawString(displayStr, (_tft.width()/2) - (_tft.textWidth(displayStr)), 50);
    _tft.setTextColor(TFT_WHITE, TFT_BLUE);
    _tft.drawString(String(displayStr[editor.getCursor()]), (_tft.width()/2) - (_tft.textWidth(displayStr)) + _tft.textWidth(displayStr.substring(0, editor.getCursor())), 50);

    drawBackButton(backSelected);
    drawSaveButton(saveSelected);
}

void TFTDisplay::drawDataViewScreen(const String &title, const std::vector<String> &dataLines) {
    _tft.fillScreen(TFT_BLACK);
    _tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    _tft.drawString(title, 10, 10);

    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    for (size_t i = 0; i < dataLines.size(); i++) {
        _tft.drawString(dataLines[i], 10, 40 + i * 20);
    }

    drawBackButton(true); // Back button always selected in view mode
}

// --- Setup menu (modular, dynamic) ---
void TFTDisplay::setupMenu() {
  menus.clear();

  // Main menu
  int mainId = addMenu("Main Menu", -1);

  // Settings
  int settingsId = addMenu("Settings", mainId);
  addMenuItem(mainId, MenuItem("Settings", MenuActionType::NAVIGATE, settingsId));

  // IEC modules
  int selectIecId = addMenu("Select IEC Module", mainId);
  addMenuItem(mainId, MenuItem("IEC Modules Menu", MenuActionType::NAVIGATE, selectIecId));

  // PDU Status
  int pduStatusId = addMenu("PDU Status", mainId);
  addMenuItem(mainId, MenuItem("PDU Status", MenuActionType::NAVIGATE, pduStatusId));

  // AHT10
  int aht10Id = addMenu("AHT10 Sensor", mainId);
  addMenuItem(mainId, MenuItem("AHT10", MenuActionType::NAVIGATE, aht10Id));

  // --- Build settings subtree ---
  buildNetworkingMenu(settingsId);

  // Measuring / PDU Modbus / IEC Modbus / Security / OTA / System / Logs
  addMenuItem(settingsId, MenuItem("PDU Modbus", MenuActionType::CALLBACK, -1, [this](){ /* open PDU Modbus settings later */ }));
  addMenuItem(settingsId, MenuItem("IEC Modbus", MenuActionType::CALLBACK, -1, [this](){ /* open IEC Modbus settings later */ }));
  addMenuItem(settingsId, MenuItem("Measuring", MenuActionType::CALLBACK, -1, [this](){ /* measuring settings */ }));
  addMenuItem(settingsId, MenuItem("System", MenuActionType::CALLBACK, -1, [this](){ /* system */ }));

  // --- PDU Status contents ---
  addMenuItem(pduStatusId, MenuItem("Voltage (V):", MenuActionType::NONE));
  addMenuItem(pduStatusId, MenuItem("Current (A):", MenuActionType::NONE));
  addMenuItem(pduStatusId, MenuItem("Power (W):", MenuActionType::NONE));
  addMenuItem(pduStatusId, MenuItem("Energy (kWh):", MenuActionType::NONE));
  addMenuItem(pduStatusId, MenuItem("Temp sensor:", MenuActionType::NONE));

  // --- AHT10 ---
  addMenuItem(aht10Id, MenuItem("Temperature:", MenuActionType::NONE));
  addMenuItem(aht10Id, MenuItem("Humidity:", MenuActionType::NONE));

  // --- IEC Modules dynamic menu will be built with real IDs when setupDisplay is called
  int iecMenuId = addMenu("IEC Module Menu", selectIecId);

  // create IEC module details menu (single template - will be updated dynamically)
  int iecInfoId = addMenu("IEC Info", iecMenuId);
  int iecStatusId = addMenu("IEC Status", iecMenuId);
  int iecSettingsId = addMenu("IEC Settings", iecMenuId);

  addMenuItem(iecMenuId, MenuItem("IEC Info", MenuActionType::NAVIGATE, iecInfoId));
  addMenuItem(iecMenuId, MenuItem("IEC Status", MenuActionType::NAVIGATE, iecStatusId));
  addMenuItem(iecMenuId, MenuItem("IEC Settings", MenuActionType::NAVIGATE, iecSettingsId));

  // PDU advanced menu
  buildPduMenu(pduStatusId);

  // Set starting menu to main
  currentMenuId = mainId;
  currentSelection = 0;
  windowStart = 0;
}

void TFTDisplay::buildNetworkingMenu(int settingsMenuId) {
  // Create networking subtree
  int wifiId = addMenu("WiFi Settings", settingsMenuId);
  int ethId = addMenu("Ethernet Settings", settingsMenuId);

  addMenuItem(settingsMenuId, MenuItem("Wifi", MenuActionType::NAVIGATE, wifiId));
  addMenuItem(settingsMenuId, MenuItem("Ethernet", MenuActionType::NAVIGATE, ethId));

  // Wifi items (editable later)
  addMenuItem(wifiId, MenuItem("View WiFi STA", MenuActionType::CALLBACK, -1,
      [this]() {
          String ssid = readStringFromNVS("ssid_sta", "");
          String password = readStringFromNVS("pwd_sta", "");
          drawDataViewScreen("WiFi STA SSID & Password", { "SSID: " + ssid, "Password: " + password });
      }));

  addMenuItem(wifiId, MenuItem("View WiFi AP", MenuActionType::CALLBACK, -1, 
          [this]() {
          String ssid = readStringFromNVS("ssid_ap", "");
          String password = readStringFromNVS("pwd_ap", "");
          drawDataViewScreen("WiFi AP SSID & Password", { "SSID: " + ssid, "Password: " + password });
      }));

  addMenuItem(wifiId, MenuItem("Set WiFi AP status:", MenuActionType::CALLBACK, -1, [this](){ 
    _networkMgr->setupAPWifi(!_networkMgr->getWiFiAPStatus());
  }));getMenuById(wifiId)->items.back().value = _networkMgr->getWiFiAPStatus() ? "[ON]" : "[OFF]";

  // Ethernet items
  addMenuItem(ethId, MenuItem("DHCP:", MenuActionType::CALLBACK, -1, [this](){ /* toggle DHCP */ }));
  addMenuItem(ethId, MenuItem("IP:", MenuActionType::CALLBACK, -1,[this]() {}));
  addMenuItem(ethId, MenuItem("Gateway:", MenuActionType::CALLBACK, -1, [this](){ /* set GW */ }));
}

void TFTDisplay::buildPduMenu(int rootId) {
  // Add PDU-specific settings and limits
  int limitsId = addMenu("Limits & Protection", rootId);
  addMenuItem(rootId, MenuItem("Overcurrent threshold (A)", MenuActionType::CALLBACK, -1, [this](){ /* set threshold */ }));
  addMenuItem(rootId, MenuItem("Current limit", MenuActionType::CALLBACK, -1, [this](){ /* set current limit */ }));
  addMenuItem(rootId, MenuItem("Firmware version", MenuActionType::NONE));
}

void TFTDisplay::setupDisplay(IECControl& iec, networkLayerManager& networkMgr) {
  _iec = &iec;
  _networkMgr = &networkMgr;
  _tft.init();
  _tft.setRotation(1);
  _tft.fillScreen(TFT_BLACK);

  setupMenu();

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);
  pinMode(BTN_CONFIRM, INPUT_PULLUP);

  attachInterrupt(BTN_UP, TFTDisplay::isr_handleUp, FALLING);
  attachInterrupt(BTN_DOWN, TFTDisplay::isr_handleDown, FALLING);
  attachInterrupt(BTN_BACK, TFTDisplay::isr_handleBack, FALLING);
  attachInterrupt(BTN_CONFIRM, TFTDisplay::isr_handleConfirm, FALLING);

  // Now that IECControl is available, populate IEC modules menu with IDs
  // Instead of relying on index search by id here, we'll search for the menu titled "IEC Module Menu"
  int selectIecId = -1;
  int iecMenuId = -1;
  for (auto &m : menus) if (String(m.title) == "Select IEC Module") selectIecId = m.id;
  for (auto &m : menus) if (String(m.title) == "IEC Module Menu") iecMenuId = m.id;
  if (selectIecId >= 0) {
    int idx = findMenuIndexById(selectIecId);
    if (idx >= 0) {
      std::vector<uint8_t> ids = _iec->getFoundIECIDs();
      if (ids.size() == 0) {
        // No modules found
        menus[idx].items.clear();
        menus[idx].items.push_back(MenuItem("No IEC modules have found!", MenuActionType::NONE));
      } else {
          // Clear placeholder items
          menus[idx].items.clear();
          for (size_t i = 0; i < ids.size(); ++i) {
            uint8_t moduleId = ids[i];
            // Create a per-module menu that navigates into IEC submenu with selected module bound
            addMenuItem(selectIecId, MenuItem("IEC Module " + String(moduleId), MenuActionType::NAVIGATE, iecMenuId, [this, moduleId](){
            // set selected module and open module menu
            selectedIECModuleID = moduleId;
            updateIECDetailMenus(selectedIECModuleID);
            // navigate to IEC Module detail menu (we reuse "IEC Info" id)
            int _iecMenuId = -1; 
            for (auto &m : menus) if (String(m.title) == "IEC Module Menu") { _iecMenuId = m.id; break; }
            if (_iecMenuId >= 0) currentMenuId = _iecMenuId;
            currentSelection = 0; 
            windowStart = 0; 
            }));
          }
        }
    }
  }
  drawMenuWindow();
}

bool TFTDisplay::interruptTimer() { 
  if(millis() - lastButtonPressed > DEBOUNCE_DELAY) {
    lastButtonPressed = millis();
    return true;
  }
  return false;
}

// --- ISR handlers ---
void IRAM_ATTR TFTDisplay::isr_handleUp() { if(_instance && _instance->interruptTimer()) _instance->UpPressed = true;}
void IRAM_ATTR TFTDisplay::isr_handleDown() { if(_instance && _instance->interruptTimer()) _instance->DownPressed = true;}
void IRAM_ATTR TFTDisplay::isr_handleBack() { if(_instance && _instance->interruptTimer()) _instance->BackPressed = true;}
void IRAM_ATTR TFTDisplay::isr_handleConfirm() { if(_instance && _instance->interruptTimer()) _instance->ConfirmPressed = true;}

void TFTDisplay::processButtonEditing() {
  if (editing) {

    if (UpPressed && !saveSelected && !backSelected) {
        editor.nextChar();
        drawEditorScreen();
        UpPressed = false;
    }

    if (DownPressed && !saveSelected && !backSelected) {
        editor.prevChar();
        drawEditorScreen();
        DownPressed = false;
    }

    if (BackPressed) {
        saveSelected = false;
        if(!editor.moveLeft()){
          backSelected = true;
          return;
        }
        else backSelected = false;
        drawEditorScreen();
        BackPressed = false;

        if(backSelected) {
            editing = false;
            drawMenuWindow();
            backSelected = false;
        }
    }

    if (ConfirmPressed) {
      backSelected = false;
      if(!editor.moveRight()) {
        saveSelected = true;
        return;
      }
      drawEditorScreen();
      ConfirmPressed = false;
      if(saveSelected) {
          // Finished editing
          editing = false;
          drawMenuWindow();
          saveSelected = false;
          ConfirmPressed = false;
          if (editorSaveCallback) {
              editorSaveCallback(editor.get());
          }
          return; // DO NOT fall into normal processing
      }
    }
  return; // DO NOT fall into normal processing
  }
}

void TFTDisplay::processButton() {
  processButtonEditing();
  if(UpPressed){
    if(currentSelection>0) currentSelection--;
    UpPressed = false;
  }

  if(DownPressed){
    int mIdx = findMenuIndexById(currentMenuId);
    if (mIdx >= 0) {
      if(currentSelection < (int)menus[mIdx].items.size()-1) currentSelection++;
    }
    DownPressed = false;
  }

  if(BackPressed){
    int mIdx = findMenuIndexById(currentMenuId);
    if (mIdx >= 0 && menus[mIdx].parentId >= 0) {
      currentMenuId = menus[mIdx].parentId;
      currentSelection=0;
      windowStart=0;
      lastSelection=-1;
      drawMenuWindow();
    }
    BackPressed = false;
  }

  if(ConfirmPressed){
    handleConfirmButton();
  }
}

void TFTDisplay::handleConfirmButton() {
  performMenuItemAction(currentMenuId, currentSelection);
  ConfirmPressed = false;
}

void TFTDisplay::performMenuItemAction(int menuId, int itemIndex) {
  int idx = findMenuIndexById(menuId);
  if (idx < 0) return;
  if (itemIndex < 0 || itemIndex >= (int)menus[idx].items.size()) return;
  MenuItem &it = menus[idx].items[itemIndex];

  if (it.actionType == MenuActionType::NAVIGATE) {
    // if targetMenuId set, use it; else rely on callback binding used in IEC module entries
    if (it.targetMenuId >= 0) {
      currentMenuId = it.targetMenuId;
      currentSelection = 0;
      windowStart = 0;
      lastSelection = -1;
      if (it.cb) it.cb();  // callback can change currentMenuId} 
      drawMenuWindow();
    } 
  } else if (it.actionType == MenuActionType::CALLBACK) {
    if (it.cb) it.cb();
    else drawMenuWindow();
  }
}

//-----------------------------------------------------------------------------------------------------------------//
// Updating and drawing
//-----------------------------------------------------------------------------------------------------------------//

void TFTDisplay::updateIECDetailMenus(int id) {
  // Find IEC Info and Status menus and set their items' values
  if (id == -1) return;
  for (auto &m : menus) {
    if (String(m.title) == "IEC Info") {
      m.items.clear();
      addMenuItem(m.id, MenuItem("ID:", MenuActionType::NONE)); m.items.back().value = String(_iec->getIECID(id));
      addMenuItem(m.id, MenuItem("Version:", MenuActionType::NONE)); m.items.back().value = String(_iec->getIECVersion(id));
      addMenuItem(m.id, MenuItem("Available LEDs:", MenuActionType::NONE)); m.items.back().value = String(_iec->getIEC_AVAILABLE_LEDS(id));
      addMenuItem(m.id, MenuItem("Current limit:", MenuActionType::NONE)); m.items.back().value = String(_iec->getIECCurrentLimit(id));
      addMenuItem(m.id, MenuItem("Relay count:", MenuActionType::NONE)); m.items.back().value = String(_iec->getIECRelayCount(id));
      addMenuItem(m.id, MenuItem("Current measured:", MenuActionType::NONE)); m.items.back().value = _iec->getIEC_IS_CURRENT_MEASURED(id) ? "YES" : "NO";
      addMenuItem(m.id, MenuItem("Voltage measured:", MenuActionType::NONE)); m.items.back().value = _iec->getIEC_IS_VOLTAGE_MEASURED(id) ? "YES" : "NO";
    }

    if (String(m.title) == "IEC Status") {
      m.items.clear();
      addMenuItem(m.id, MenuItem("Current (A):", MenuActionType::NONE)); m.items.back().value = String(_iec->getCurrentData(id));
      addMenuItem(m.id, MenuItem("Voltage (V):", MenuActionType::NONE)); m.items.back().value = String(_iec->getVoltageData(id));
      addMenuItem(m.id, MenuItem("Power (W):", MenuActionType::NONE)); m.items.back().value = String(_iec->getPowerData(id));
      // Relay toggler
      addMenuItem(m.id, MenuItem("Relay:", MenuActionType::CALLBACK, -1, [this, id](){
        _iec->setRelayStatus(id, !_iec->getIECRelayStatus(id));
        updateIECDetailMenus(id);
      }));
      m.items.back().value = _iec->getIECRelayStatus(id) ? "[ON]" : "[OFF]";
    }
  }
}

void TFTDisplay::updateActiveMenuPeriodic() {
  if (millis() - lastAutoUpdate < autoUpdateInterval)
    return;

  lastAutoUpdate = millis();

// If we are viewing IEC detail menus, refresh
  int idx = findMenuIndexById(currentMenuId);
  if (idx < 0) return;
  String title = menus[idx].title;
  if (title == "IEC Status" || title == "IEC Info") {
  if (selectedIECModuleID != -1) updateIECDetailMenus(selectedIECModuleID);
    updateDynamicValues();
  }
  updateMenuValues();
}

void TFTDisplay::updateDynamicValues() {
  int mIdx = findMenuIndexById(currentMenuId);
  if (mIdx < 0) return;

  if(currentSelection < windowStart) {
    windowStart = currentSelection;
  }
  else if(currentSelection >= windowStart + maxVisible) {
    windowStart = currentSelection - maxVisible + 1;
  }

  int end = windowStart + maxVisible;
  if(end > (int)menus[mIdx].items.size()) end = menus[mIdx].items.size();

  for (int i = windowStart; i < end; i++) {
    int y = startOfTexts + (i - windowStart)*rowHeight;

    if(i == currentSelection) {
      _tft.setTextColor(TFT_WHITE, TFT_BLUE);
    }
    else {
      _tft.setTextColor(TFT_GREEN, TFT_BLACK);
    }
    _tft.fillRect(140, y, 80, rowHeight, TFT_BLACK);
    _tft.drawString(menus[mIdx].items[i].value, 110, y);
  }
}

void TFTDisplay::drawMenuWindow() {
  _tft.fillScreen(TFT_BLACK);
  _tft.drawWideLine(0,5,_tft.width(),5,2,TFT_WHITE);
  _tft.drawWideLine(0,29,_tft.width(),29,2,TFT_WHITE);

  _tft.setTextFont(2);
  _tft.setTextDatum(TL_DATUM);

  int idx = findMenuIndexById(currentMenuId);
  if (idx < 0) return;

  _tft.setTextColor(TFT_YELLOW,TFT_BLACK);
  _tft.drawString(menus[idx].title,10,10);

  int end = windowStart + maxVisible;
  if(end > (int)menus[idx].items.size()) end = menus[idx].items.size();

  for(int i = windowStart; i < end; i++){
    int y = startOfTexts + (i - windowStart)*rowHeight;
    _tft.setTextColor(TFT_WHITE,TFT_BLACK);
    _tft.drawString(menus[idx].items[i].name,startOfTextsLeft,y+startOfTextsFromBlue);
    if(menus[idx].items[i].value.length() > 0) {
      _tft.setTextColor(TFT_GREEN, TFT_BLACK);
      if (getMenuById(currentMenuId)->title == "IEC Info") {
      _tft.drawString(menus[idx].items[i].value, 130, y);
      } else {
       _tft.drawString(menus[idx].items[i].value, 110, y);
      }
    }
  }

  lastSelection = -1; // ready for cursor redraw
}

void TFTDisplay::updateMenuValues() {
  // Called by external loop to push updated values (e.g. sensors)
  // Example: update PDU status values
  for (auto &m : menus) {
    if (String(m.title) == "PDU Status") {
      // attempt to read from IEC or local sensors - here we just stub
    }

    if (String(m.title) == "AHT10 Sensor") {
      if (m.items.size() >= 2) {
        // stub: replace with real aht10 calls
        m.items[0].value = "-- C";
        m.items[1].value = "-- %";
      }
    }
  }
}

void TFTDisplay::updateCursor() {
  int mIdx = findMenuIndexById(currentMenuId);
  if (mIdx < 0) return;

  if(currentSelection < windowStart) {
    windowStart = currentSelection;
    drawMenuWindow();
  }
  else if(currentSelection >= windowStart + maxVisible) {
    windowStart = currentSelection - maxVisible + 1;
    drawMenuWindow();
  }

  if(currentSelection == lastSelection) return;

  // old cursor redraw
  if(lastSelection >= windowStart && lastSelection < windowStart + maxVisible){
    int yOld = startOfTexts + (lastSelection - windowStart)*rowHeight;
    _tft.fillRect(0, yOld, 220, rowHeight, TFT_BLACK);
    _tft.setTextColor(TFT_WHITE,TFT_BLACK);
    _tft.drawString(menus[mIdx].items[lastSelection].name, startOfTextsLeft, yOld + startOfTextsFromBlue);
    if(menus[mIdx].items[lastSelection].value.length() > 0) {
      _tft.setTextColor(TFT_GREEN, TFT_BLACK);
      _tft.drawString(menus[mIdx].items[lastSelection].value, 110, yOld);
    }
  }

  // new cursor
  if(currentSelection >= windowStart && currentSelection < windowStart + maxVisible){
    int yNew = startOfTexts + (currentSelection - windowStart)*rowHeight;
    _tft.fillRect(0, yNew, 220, rowHeight, TFT_BLUE);
    _tft.setTextColor(TFT_WHITE,TFT_BLUE);
    _tft.drawString(menus[mIdx].items[currentSelection].name, startOfTextsLeft, yNew + startOfTextsFromBlue);
    if(menus[mIdx].items[currentSelection].value.length() > 0) {
      _tft.setTextColor(TFT_WHITE, TFT_BLUE);
      _tft.drawString(menus[mIdx].items[currentSelection].value, 110, yNew);
    }
  }

  lastSelection = currentSelection;
}