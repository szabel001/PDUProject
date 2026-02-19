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

void TFTDisplay::startEditing(int initialValue,
                              std::function<void(const String&)> saveCb)
{
    editing = true;
    editor.begin(initialValue);
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
    _tft.drawString(String(displayStr), (_tft.width()/2) - (_tft.textWidth(displayStr)), 50);
}

void TFTDisplay::drawDataViewScreen(const String &title, const std::vector<String> &dataLines) {
    dataview = true;
    _tft.fillScreen(TFT_BLACK);
    _tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    _tft.setTextFont(1);
    _tft.drawString(title, 10, 10);
    _tft.setTextFont(2);

    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    for (size_t i = 0; i < dataLines.size(); i++) {
        _tft.drawString(dataLines[i] + '\n', 10, 40 + i * 20);
    }
}

// --- Setup menu (modular, dynamic) ---
void TFTDisplay::setupMenu() {
  menus.clear();

  // Main menu
  int mainId = addMenu("Main Menu", -1);

  // Settings
  int settingsId = addMenu("Settings", mainId);
  addMenuItem(mainId, MenuItem("Settings", MenuActionType::NAVIGATE, settingsId));
  buildNetworkingMenu(settingsId);
  //int pduModbusId = addMenu("PDU Modbus", settingsId);
  //addMenuItem(settingsId, MenuItem("PDU Modbus", MenuActionType::NAVIGATE, pduModbusId));
  //addMenuItem(pduModbusId, MenuItem("Set Modbus Address", MenuActionType::CALLBACK, -1, [this](){ /* set modbus address */ }));
  int measuringId = addMenu("Measuring", settingsId);
  addMenuItem(settingsId, MenuItem("Measuring Settings", MenuActionType::NAVIGATE, measuringId));
  addMenuItem(measuringId, MenuItem("Temp. scale", MenuActionType::CALLBACK, -1, [this, measuringId](){ 
    _envSensor->setTemperatureScale(!(_envSensor->isFahrenheit()));
    getMenuById(measuringId)->items[0].value = String((_envSensor->isFahrenheit()) ?  "`F" :  "`C");
    drawMenuWindow();
  }));getMenuById(measuringId)->items[0].value = String((_envSensor->isFahrenheit()) ? "`F" : "`C");

  addMenuItem(measuringId, MenuItem("IEC meas. cycleTime [s]", MenuActionType::CALLBACK, -1, [this](){ 
    startEditing(_iec->getPowerDataUpdateCycleTime(), [this](const String& val) {
      uint16_t cycleTime = val.toInt();
      _iec->setpowerDataUpdateCycleTime(cycleTime);
    });
  }));
 
  // IEC modules
  int selectIecId = addMenu("Select IEC Module", mainId);
  addMenuItem(mainId, MenuItem("IEC Modules Menu", MenuActionType::NAVIGATE, selectIecId));

  // PDU Status
  int pduStatusId = addMenu("PDU Status", mainId);
  addMenuItem(mainId, MenuItem("PDU Status", MenuActionType::NAVIGATE, pduStatusId));
  addMenuItem(pduStatusId, MenuItem("Voltage[V]:", MenuActionType::NONE));
  addMenuItem(pduStatusId, MenuItem("Current[A]:", MenuActionType::NONE));
  addMenuItem(pduStatusId, MenuItem("Power[W]:", MenuActionType::NONE));
  addMenuItem(pduStatusId, MenuItem("Energy[kWh]:", MenuActionType::NONE));
  addMenuItem(pduStatusId, MenuItem("OC threshold[A]", MenuActionType::NONE));
  addMenuItem(pduStatusId, MenuItem("Current limit:", MenuActionType::NONE));
  addMenuItem(pduStatusId, MenuItem("Frw version:", MenuActionType::NONE));

  // AHT10
  int aht10Id = addMenu("AHT10 Sensor", mainId);
  addMenuItem(mainId, MenuItem("AHT10", MenuActionType::NAVIGATE, aht10Id));
  addMenuItem(aht10Id, MenuItem("Temperature:", MenuActionType::NONE));
  addMenuItem(aht10Id, MenuItem("Humidity:", MenuActionType::NONE));

  // --- IEC Modules dynamic menu will be built with real IDs when setupDisplay is called
  int iecMenuId = addMenu("IEC Module Menu", selectIecId);

  // create IEC module details menu (single template - will be updated dynamically)
  int iecInfoId = addMenu("IEC Info", iecMenuId);
  addMenuItem(iecMenuId, MenuItem("IEC Info", MenuActionType::NAVIGATE, iecInfoId));

  int iecStatusId = addMenu("IEC Status", iecMenuId);
  addMenuItem(iecMenuId, MenuItem("IEC Status", MenuActionType::NAVIGATE, iecStatusId));

  int iecSettingsId = addMenu("IEC Settings", iecMenuId);
  addMenuItem(iecMenuId, MenuItem("IEC Settings", MenuActionType::NAVIGATE, iecSettingsId));
  addMenuItem(iecSettingsId, MenuItem("DUMMY:", MenuActionType::NONE));

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

  addMenuItem(wifiId, MenuItem("View WIFI config", MenuActionType::CALLBACK, -1,
      [this]() {
          String ip = readStringFromNVS(NVSKeys::WIFI_IP, "");
          String gateway = readStringFromNVS(NVSKeys::WIFI_GATEWAY, "");
          String subnet = readStringFromNVS(NVSKeys::WIFI_SUBNET, "");
          String dns = readStringFromNVS(NVSKeys::WIFI_DNS, "");
          drawDataViewScreen("WIFI Config", { "IP: " + ip, "Gateway: " + gateway, "Subnet: " + subnet, "DNS: " + dns });
      }));

  addMenuItem(wifiId, MenuItem("View WiFi STA", MenuActionType::CALLBACK, -1,
      [this]() {
          String ssid = readStringFromNVS(NVSKeys::WIFI_STA_SSID, "");
          String password = readStringFromNVS(NVSKeys::WIFI_STA_PWD, "");
          drawDataViewScreen("WiFi STA SSID & Password", { "SSID: " + ssid, "Password: " + password });
      }));

  addMenuItem(wifiId, MenuItem("View WiFi AP", MenuActionType::CALLBACK, -1, 
          [this]() {
          String ssid = readStringFromNVS(NVSKeys::WIFI_AP_SSID, "");
          String password = readStringFromNVS(NVSKeys::WIFI_AP_PWD, "");
          drawDataViewScreen("WiFi AP SSID & Password", { "SSID: " + ssid, "Password: " + password });
      }));

  addMenuItem(wifiId, MenuItem("Set WiFi AP status:", MenuActionType::CALLBACK, -1, [this, wifiId](){ 
    _networkMgr->setupAPWifi(!_networkMgr->getWiFiAPStatus());
    getMenuById(wifiId)->items.back().value = _networkMgr->getWiFiAPStatus() ? "[ON]" : "[OFF]";
    drawMenuWindow();
  }));getMenuById(wifiId)->items.back().value = _networkMgr->getWiFiAPStatus() ? "[ON]" : "[OFF]";

  // Ethernet items 
  addMenuItem(ethId, MenuItem("DHCP status: ", MenuActionType::CALLBACK, -1, [this, ethId](){ 
    _networkMgr->setEthernetDHCP(!_networkMgr->getEthernetDHCPStatus());
    getMenuById(ethId)->items[0].value = _networkMgr->getEthernetDHCPStatus() ? "[ON]" : "[OFF]";
    drawMenuWindow();
  }));getMenuById(ethId)->items[0].value = _networkMgr->getEthernetDHCPStatus() ? "[ON]" : "[OFF]";

  addMenuItem(ethId, MenuItem("View Ethernet config", MenuActionType::CALLBACK, -1,
      [this]() {
          String ip = readStringFromNVS(NVSKeys::ETHERNET_IP, "");
          String gateway = readStringFromNVS(NVSKeys::ETHERNET_GATEWAY, "");
          String subnet = readStringFromNVS(NVSKeys::ETHERNET_SUBNET, "");
          String dns = readStringFromNVS(NVSKeys::ETHERNET_DNS, "");
          drawDataViewScreen("Ethernet Config", { "IP: " + ip, "Gateway: " + gateway, "Subnet: " + subnet, "DNS: " + dns });
      }));
}

void TFTDisplay::setupDisplay(IECControl& iec, networkLayerManager& networkMgr, EnvironmentSensor& envSensor) {
  _iec = &iec;
  _networkMgr = &networkMgr;
  _envSensor = &envSensor;
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

  int selectIecId = -1;
  int iecMenuId = -1;
  for (auto &m : menus) if (String(m.title) == "Select IEC Module") selectIecId = m.id;
  for (auto &m : menus) if (String(m.title) == "IEC Module Menu") iecMenuId = m.id;
  if (selectIecId >= 0) {
    int idx = findMenuIndexById(selectIecId);
    if (idx >= 0) {
      std::vector<uint8_t> ids = _iec->getFoundIECIDs();
      if (ids.size() == 0) {
        menus[idx].items.clear();
        menus[idx].items.push_back(MenuItem("No IEC modules found!", MenuActionType::NONE));
      } else {
          menus[idx].items.clear();
          for (size_t i = 0; i < ids.size(); ++i) {
            uint8_t moduleId = ids[i];
            addMenuItem(selectIecId, MenuItem("IEC Module " + String(moduleId), MenuActionType::NAVIGATE, iecMenuId, [this, moduleId](){
            selectedIECModuleID = moduleId;
            updateIECDetailMenus(selectedIECModuleID);
            int _iecMenuId = -1; 
            for (auto &m : menus) if (String(m.title) == "IEC Module Menu") { _iecMenuId = m.id; break; }
            if (_iecMenuId >= 0) currentMenuId = _iecMenuId;
            }));
            currentSelection = 0; 
            windowStart = 0; 
          }
        }
          addMenuItem(selectIecId, MenuItem("Refresh IEC modules", MenuActionType::CALLBACK, -1, [this, selectIecId](){
            int idx = findMenuIndexById(selectIecId);
            int iecMenuId = -1;
            for (auto &m : menus) if (String(m.title) == "IEC Module Menu") iecMenuId = m.id;
            menus[idx].items.clear();
            selectedIECModuleID = -1;
            _iec->discoverIECs();
            std::vector<uint8_t> ids = _iec->getFoundIECIDs();
            for (size_t i = 0; i < ids.size(); ++i) {
              uint8_t moduleId = ids[i];
              addMenuItem(selectIecId, MenuItem("IEC Module " + String(moduleId), MenuActionType::NAVIGATE, iecMenuId, [this, moduleId](){
              selectedIECModuleID = moduleId;
              updateIECDetailMenus(selectedIECModuleID);
              int _iecMenuId = -1; 
              for (auto &m : menus) if (String(m.title) == "IEC Module Menu") { _iecMenuId = m.id; break; }
              if (_iecMenuId >= 0) currentMenuId = _iecMenuId;
              }));
              currentSelection = 0; 
              windowStart = 0; 
          }
          }));
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

void TFTDisplay::processButton() {
  if(UpPressed){
    if (editing) {
        editor.nextInt();
        drawEditorScreen();
        UpPressed = false;
        return;
    }
    if(currentSelection>0) currentSelection--;
    UpPressed = false;
  }

  if(DownPressed){
    if (editing) {
        editor.prevInt();
        drawEditorScreen();
        DownPressed = false;
        return;
    }
    int mIdx = findMenuIndexById(currentMenuId);
    if (mIdx >= 0) {
      if(currentSelection < (int)menus[mIdx].items.size()-1) currentSelection++;
    }
    DownPressed = false;
  }

  if(BackPressed){
    int mIdx = findMenuIndexById(currentMenuId);
    if (mIdx >= 0 && menus[mIdx].parentId >= 0 && !editing && !dataview) {
      currentMenuId = menus[mIdx].parentId;
      currentSelection=0;
      windowStart=0;
      lastSelection=-1;
    }
    editing = false;
    dataview = false;
    BackPressed = false;
    drawMenuWindow();
  }

  if(ConfirmPressed){
    if (editing) {
      if (editorSaveCallback) {
        editorSaveCallback(editor.get());
      }
      editorSaveCallback = nullptr;
      editing = false;
      drawSavedSuccessScreen();
      drawMenuWindow();
      ConfirmPressed = false;
      return;
    }

    int idx = findMenuIndexById(currentMenuId);
    if (idx < 0) return;
    if (currentSelection < 0 || currentSelection >= (int)menus[idx].items.size()) return;
    MenuItem &it = menus[idx].items[currentSelection];

    if (it.actionType == MenuActionType::NAVIGATE && !editing) {
      if (it.targetMenuId >= 0) {
        currentMenuId = it.targetMenuId;
        currentSelection = 0;
        windowStart = 0;
        lastSelection = -1;
        if (it.cb) it.cb();  // callback can change currentMenuId} 
        drawMenuWindow();
      }
    } else if (it.actionType == MenuActionType::CALLBACK) {
      if (it.cb && !editing) it.cb();
    }
    ConfirmPressed = false;
  }
}

//-----------------------------------------------------------------------------------------------------------------//
// Updating and drawing
//-----------------------------------------------------------------------------------------------------------------//

void TFTDisplay::updateIECDetailMenus(int id) {
  if (id == -1) return;
  for (auto &m : menus) {
    if (String(m.title) == "IEC Info") {
      m.items.clear();
      addMenuItem(m.id, MenuItem("ID:", MenuActionType::NONE)); m.items.back().value = String(_iec->getIECID(id));
      addMenuItem(m.id, MenuItem("Version:", MenuActionType::NONE)); m.items.back().value = String(_iec->getIECVersion(id));
      addMenuItem(m.id, MenuItem("Available LEDs:", MenuActionType::NONE)); m.items.back().value = String(_iec->getIEC_AVAILABLE_LEDS(id));
      addMenuItem(m.id, MenuItem("Current limit:", MenuActionType::NONE)); m.items.back().value = String(_iec->getIECCurrentLimit(id));
      addMenuItem(m.id, MenuItem("Relay count:", MenuActionType::NONE)); m.items.back().value = String(_iec->getIECRelayCount(id));
      addMenuItem(m.id, MenuItem("A measured:", MenuActionType::NONE)); m.items.back().value = _iec->getIEC_IS_RMS_CURRENT_MEASURED(id) ? "YES" : "NO";
      addMenuItem(m.id, MenuItem("V measured:", MenuActionType::NONE)); m.items.back().value = _iec->getIEC_IS_RMS_VOLTAGE_MEASURED(id) ? "YES" : "NO";
      addMenuItem(m.id, MenuItem("Act. P meas.:", MenuActionType::NONE)); m.items.back().value = _iec->getIEC_IS_ACTIVE_POWER_MEASURED(id) ? "YES" : "NO";
      addMenuItem(m.id, MenuItem("React. P meas.:", MenuActionType::NONE)); m.items.back().value = _iec->getIEC_IS_REACTIVE_POWER_MEASURED(id) ? "YES" : "NO";
      addMenuItem(m.id, MenuItem("App. P meas.:", MenuActionType::NONE)); m.items.back().value = _iec->getIEC_IS_APPARENT_POWER_MEASURED(id) ? "YES" : "NO";
      addMenuItem(m.id, MenuItem("P Factor meas.:", MenuActionType::NONE)); m.items.back().value = _iec->getIEC_IS_POWER_FACTOR_MEASURED(id) ? "YES" : "NO";
      addMenuItem(m.id, MenuItem("Freq. meas.:", MenuActionType::NONE)); m.items.back().value = _iec->getIEC_IS_AC_FREQUENCY_MEASURED(id) ? "YES" : "NO";
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
    _tft.drawString(menus[mIdx].items[i].value, 120, y);
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
    _tft.drawString(menus[idx].items[i].name, startOfTextsLeft, y+startOfTextsFromBlue);
    if(menus[idx].items[i].value.length() > 0) {
      _tft.setTextColor(TFT_GREEN, TFT_BLACK);
      if (getMenuById(currentMenuId)->title == "IEC Info") {
      _tft.drawString(menus[idx].items[i].value, 120, y);
      } else {
       _tft.drawString(menus[idx].items[i].value, 120, y);
      }
    }
  }

  lastSelection = -1; // ready for cursor redraw
}

void TFTDisplay::updateMenuValues() {
  for (auto &m : menus) {
    if (String(m.title) == "PDU Status") {
      if (m.items.size() >= 6) {
        // stub: replace with real pdu status calls
        m.items[0].value = "-- V";
        m.items[1].value = "-- A";
        m.items[2].value = "-- W";
        m.items[3].value = "-- kWh";
      }
    }

    if (String(m.title) == "AHT10 Sensor") {
      if (m.items.size() >= 2) {
        // stub: replace with real aht10 calls
        m.items[0].value = "--" + String(( _envSensor->isFahrenheit()) ? "`F" : "`C");
        m.items[1].value = "-- %";
      }
    }
  }
}

void TFTDisplay::drawSavedSuccessScreen() {
  _tft.fillScreen(TFT_BLACK);
  _tft.setTextColor(TFT_GREEN, TFT_BLACK);
  _tft.setTextFont(2);
  _tft.drawString("Value saved!", 60, 50);
  _tft.setTextFont(1);
  delay(1000);
  drawMenuWindow();
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
      _tft.drawString(menus[mIdx].items[lastSelection].value, 120, yOld);
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
      _tft.drawString(menus[mIdx].items[currentSelection].value, 120, yNew);
    }
  }

  lastSelection = currentSelection;
}