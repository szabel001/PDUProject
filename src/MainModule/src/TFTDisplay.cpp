#include "TFTDisplay.h"

TFTDisplay *TFTDisplay::_instance = nullptr;

TFTDisplay::TFTDisplay()
{
  _instance = this;
}

int TFTDisplay::addMenu(const char *title, int parentId)
{
  int id = menus.size();
  menus.emplace_back(id, title, parentId);
  return id;
}

void TFTDisplay::addMenuItem(int menuId, const MenuItem &item)
{
  int idx = findMenuIndexById(menuId);
  if (idx >= 0)
  {
    menus[idx].items.push_back(item);
  }
}

int TFTDisplay::findMenuIndexById(int id)
{
  for (size_t i = 0; i < menus.size(); i++)
  {
    if (menus[i].id == id)
      return i;
  }
  return -1;
}

Menu *TFTDisplay::getMenuById(int id)
{
  for (size_t i = 0; i < menus.size(); i++)
  {
    if (menus[i].id == id)
      return &menus[i];
  }
  return nullptr;
}

void TFTDisplay::startEditing(int initialValue,
                              std::function<void(const String &)> saveCb)
{
  editing = true;
  editor.begin(initialValue);
  editorSaveCallback = saveCb;

  _tft.fillScreen(TFT_BLACK);
  _tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  _tft.drawString("Edit mode", 10, 10);

  drawEditorScreen();
}

void TFTDisplay::drawEditorScreen()
{
  _tft.fillRect(0, 40, _tft.width(), 40, TFT_BLACK);
  _tft.setTextColor(TFT_WHITE, TFT_BLACK);
  String displayStr = editor.get();
  _tft.drawString(displayStr, (_tft.width() / 2) - (_tft.textWidth(displayStr)), 50);
  _tft.setTextColor(TFT_WHITE, TFT_BLUE);
  _tft.drawString(String(displayStr), (_tft.width() / 2) - (_tft.textWidth(displayStr)), 50);
}

void TFTDisplay::drawDataViewScreen(const String &title, const std::vector<String> &dataLines)
{
  dataview = true;
  _tft.fillScreen(TFT_BLACK);
  _tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  _tft.setTextFont(1);
  _tft.drawString(title, 10, 10);
  _tft.setTextFont(1);

  _tft.setTextColor(TFT_WHITE, TFT_BLACK);
  for (size_t i = 0; i < dataLines.size(); i++)
  {
    _tft.drawString(dataLines[i], 10, 40 + i * 15);
  }
}

// --- Setup menu (modular, dynamic) ---
void TFTDisplay::setupMenu()
{
  menus.clear();

  // Main menu
  int mainId = addMenu("Main Menu", -1);

  // Settings
  int settingsId = addMenu("Settings", mainId);
  addMenuItem(mainId, MenuItem("Settings", MenuActionType::NAVIGATE, settingsId));
  buildNetworkingMenu(settingsId);
  int measuringId = addMenu("Measuring", settingsId);
  addMenuItem(settingsId, MenuItem("Measuring Settings", MenuActionType::NAVIGATE, measuringId));
  addMenuItem(measuringId, MenuItem("Temp. scale", MenuActionType::CALLBACK, -1, [this, measuringId]()
                                    { 
                                    _envSensor->setTemperatureScale(!(_envSensor->isFahrenheit()));
                                    getMenuById(measuringId)->items[0].value = String((_envSensor->isFahrenheit()) ?  "`F" :  "`C");
                                    drawMenuWindow(); }));
                                  getMenuById(measuringId)->items[0].value = String((_envSensor->isFahrenheit()) ? "`F" : "`C");

  addMenuItem(measuringId, MenuItem("IEC meas. cycleTime:", MenuActionType::CALLBACK, -1, [this]()
                                    { startEditing(_iec->getPowerDataUpdateCycleTime(), [this](const String &val) {
                                    uint16_t cycleTime = val.toInt();
                                    _iec->setpowerDataUpdateCycleTime(cycleTime); }); }));

  addMenuItem(measuringId, MenuItem("IEC switching delay:", MenuActionType::CALLBACK, -1, [this]()
                                    { startEditing(_iec->getIECSwitchingDelay(), [this](const String &val) {
                                      uint16_t delay = val.toInt();
                                      _iec->setIECSwitchingDelay(delay); }); }));

  // IEC modules
  int selectIecId = addMenu("Select IEC Module", mainId);
  addMenuItem(mainId, MenuItem("IEC Modules Menu", MenuActionType::NAVIGATE, selectIecId));

  // PDU Status
  int pduStatusId = addMenu("PDU Status", mainId);
  addMenuItem(mainId, MenuItem("PDU Status", MenuActionType::NAVIGATE, pduStatusId));
  addMenuItem(pduStatusId, MenuItem("Turn on all relays", MenuActionType::CALLBACK, -1, [this](){ _iec->setAllIecRelaysOn();}));
  addMenuItem(pduStatusId, MenuItem("Turn off all relays", MenuActionType::CALLBACK, -1, [this](){ _iec->setAllIecRelaysOff();}));
  addMenuItem(pduStatusId, MenuItem("SUM Curr.:", MenuActionType::NONE));
  addMenuItem(pduStatusId, MenuItem("AVG Volt.::", MenuActionType::NONE));
  addMenuItem(pduStatusId, MenuItem("SUM P:", MenuActionType::NONE));
  addMenuItem(pduStatusId, MenuItem("OC trsh.:", MenuActionType::NONE));
  addMenuItem(pduStatusId, MenuItem("Frw ver.:", MenuActionType::NONE));

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
  addMenuItem(iecInfoId, MenuItem("ID:", MenuActionType::NONE));
  addMenuItem(iecInfoId, MenuItem("Version:", MenuActionType::NONE));
  addMenuItem(iecInfoId, MenuItem("Available LEDs:", MenuActionType::NONE));
  addMenuItem(iecInfoId, MenuItem("Current limit:", MenuActionType::NONE));
  addMenuItem(iecInfoId, MenuItem("Relay count:", MenuActionType::NONE));
  addMenuItem(iecInfoId, MenuItem("A measured:", MenuActionType::NONE));
  addMenuItem(iecInfoId, MenuItem("V measured:", MenuActionType::NONE));
  addMenuItem(iecInfoId, MenuItem("Act. P meas.:", MenuActionType::NONE));
  addMenuItem(iecInfoId, MenuItem("React. P meas.:", MenuActionType::NONE));
  addMenuItem(iecInfoId, MenuItem("App. P meas.:", MenuActionType::NONE));
  addMenuItem(iecInfoId, MenuItem("P Factor meas.:", MenuActionType::NONE));
  addMenuItem(iecInfoId, MenuItem("Freq. meas.:", MenuActionType::NONE));


  int iecStatusId = addMenu("IEC Status", iecMenuId);
  addMenuItem(iecMenuId, MenuItem("IEC Status", MenuActionType::NAVIGATE, iecStatusId));

  int iecSettingsId = addMenu("IEC Settings", iecMenuId);
  addMenuItem(iecMenuId, MenuItem("IEC Settings", MenuActionType::NAVIGATE, iecSettingsId));

  // Set starting menu to main
  currentMenuId = mainId;
  currentSelection = 0;
  windowStart = 0;
}

void TFTDisplay::buildNetworkingMenu(int settingsMenuId)
{
  // Create networking subtree
  int wifiId = addMenu("WiFi Settings", settingsMenuId);
  int ethId = addMenu("Ethernet Settings", settingsMenuId);
  int mqttId = addMenu("MQTT Settings", settingsMenuId);

  addMenuItem(settingsMenuId, MenuItem("Wifi", MenuActionType::NAVIGATE, wifiId));
  addMenuItem(settingsMenuId, MenuItem("Ethernet", MenuActionType::NAVIGATE, ethId));
  addMenuItem(settingsMenuId, MenuItem("MQTT", MenuActionType::NAVIGATE, mqttId));


    addMenuItem(wifiId, MenuItem("Set WiFi STA:", MenuActionType::CALLBACK, -1, [this, wifiId]()
                               {
                                _networkMgr->setWifiSTA_Status(!_networkMgr->getWifiSTA_Status());
                                getMenuById(wifiId)->items[0].value = _networkMgr->getWifiSTA_Status() ? "[ON]" : "[OFF]";
                                drawMenuWindow(); 
                              }));

  addMenuItem(wifiId, MenuItem("View WiFi STA status", MenuActionType::CALLBACK, -1,
                               [this]()
                               {
                                String status = _networkMgr->getSTAStatusString();
                                 drawDataViewScreen("WiFi STA Status",  {"Status:", status});
                               }));

  addMenuItem(wifiId, MenuItem("View WiFi STA config", MenuActionType::CALLBACK, -1,
                               [this]()
                               {
                                 String ssid = readStringFromNVS(NVSKeys::WIFI_STA_SSID, "");
                                 String password = readStringFromNVS(NVSKeys::WIFI_STA_PWD, "");
                                 String ip = readStringFromNVS(NVSKeys::WIFI_STA_IP, "");
                                 String gateway = readStringFromNVS(NVSKeys::WIFI_STA_GATEWAY, "");
                                 String subnet = readStringFromNVS(NVSKeys::WIFI_STA_SUBNET, "");
                                 String dns = readStringFromNVS(NVSKeys::WIFI_STA_DNS, "");
                                 drawDataViewScreen("WiFi STA Config",  {"SSID:" + ssid, "PWD:" + password, "IP:" + ip, "Gateway:" + gateway, "Subnet:" + subnet, "DNS:" + dns});
                               }));

  addMenuItem(wifiId, MenuItem("Set WiFi AP:", MenuActionType::CALLBACK, -1, [this, wifiId]() { 
                                _networkMgr->setWifiAP_Status(!_networkMgr->getWifiAP_Status());
                                getMenuById(wifiId)->items[3].value = _networkMgr->getWifiAP_Status() ? "[ON]" : "[OFF]";
                                drawMenuWindow(); }));

  getMenuById(wifiId)->items.back().value = _networkMgr->getWifiAP_Status() ? "[ON]" : "[OFF]";
  addMenuItem(wifiId, MenuItem("View WiFi AP config", MenuActionType::CALLBACK, -1,
                               [this]()
                               {
                                 String ssid = readStringFromNVS(NVSKeys::WIFI_AP_SSID, "");
                                 String password = readStringFromNVS(NVSKeys::WIFI_AP_PWD, "");
                                 String ip = readStringFromNVS(NVSKeys::WIFI_AP_IP, "");
                                 String gateway = readStringFromNVS(NVSKeys::WIFI_AP_GATEWAY, "");
                                 String subnet = readStringFromNVS(NVSKeys::WIFI_AP_SUBNET, "");
                                 String dns = readStringFromNVS(NVSKeys::WIFI_AP_DNS, "");
                                 drawDataViewScreen("WiFi AP Config", {"SSID:" + ssid, "PWD:" + password, "IP:" + ip, "Gateway:" + gateway, "Subnet:" + subnet, "DNS:" + dns});
                               }));

  // Ethernet items
  addMenuItem(ethId, MenuItem("DHCP status: ", MenuActionType::CALLBACK, -1, [this, ethId]()
                              { 
                                _networkMgr->setEthernetDHCP(!_networkMgr->getEthernetDHCPStatus());
                                getMenuById(ethId)->items[0].value = _networkMgr->getEthernetDHCPStatus() ? "[ON]" : "[OFF]";
                                drawMenuWindow(); }));
                                getMenuById(ethId)->items[0].value = _networkMgr->getEthernetDHCPStatus() ? "[ON]" : "[OFF]";

  addMenuItem(ethId, MenuItem("View Ethernet config", MenuActionType::CALLBACK, -1,
                              [this]()
                              {
                                String ip = readStringFromNVS(NVSKeys::ETHERNET_IP, "");
                                String gateway = readStringFromNVS(NVSKeys::ETHERNET_GATEWAY, "");
                                String subnet = readStringFromNVS(NVSKeys::ETHERNET_SUBNET, "");
                                String dns = readStringFromNVS(NVSKeys::ETHERNET_DNS, "");
                                drawDataViewScreen("Ethernet Config", {"IP: " + ip, "Gateway:" + gateway, "Subnet:" + subnet, "DNS:" + dns});
                              }));

  // --- ÚJ: MQTT menüpontok ---
  addMenuItem(mqttId, MenuItem("SET", MenuActionType::CALLBACK, -1, [this, mqttId]()
                               { 
                                bool currentState = mqttManager->isMQTTEnabled();
                                writeIntToNVS(NVSKeys::MQTT_ENA, !currentState ? 1 : 0);
                                drawMenuWindow(); 
                              }));
  addMenuItem(mqttId, MenuItem("Stat:", MenuActionType::NONE));
  addMenuItem(mqttId, MenuItem("IP:", MenuActionType::NONE));
}

void TFTDisplay::setupDisplay(IECControl &iec, networkLayerManager &networkMgr, EnvironmentSensor &envSensor)
{
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
  for (auto &m : menus)
    if (String(m.title) == "Select IEC Module")
      selectIecId = m.id;
  for (auto &m : menus)
    if (String(m.title) == "IEC Module Menu")
      iecMenuId = m.id;
  if (selectIecId >= 0)
  {
    int idx = findMenuIndexById(selectIecId);
    if (idx >= 0)
    {
      std::vector<uint8_t> ids = _iec->getFoundIECIDs();
      if (ids.size() == 0)
      {
        menus[idx].items.clear();
        menus[idx].items.push_back(MenuItem("No IEC modules found!", MenuActionType::NONE));
      }
      else
      {
        menus[idx].items.clear();
        for (size_t i = 0; i < ids.size(); ++i)
        {
          uint8_t moduleId = ids[i];
          addMenuItem(selectIecId, MenuItem("IEC Module " + String(moduleId), MenuActionType::NAVIGATE, iecMenuId, [this, moduleId]()
                                            {
            selectedIECModuleID = moduleId;
            updateIECValues(selectedIECModuleID);
            int _iecMenuId = -1; 
            for (auto &m : menus) if (String(m.title) == "IEC Module Menu") { _iecMenuId = m.id; break; }
            if (_iecMenuId >= 0) currentMenuId = _iecMenuId; }));
          currentSelection = 0;
          windowStart = 0;
        }
      }
      addMenuItem(selectIecId, MenuItem("Refresh IEC modules", MenuActionType::CALLBACK, -1, [this, selectIecId]()
                                        {
          int idx = findMenuIndexById(selectIecId);
          int iecMenuId = -1;
          for (auto &m : menus) if (String(m.title) == "IEC Module Menu") iecMenuId = m.id;
          
          // 1. MENTSÜK MEG A REFRESH GOMBOT (ami épp le lett nyomva) mielőtt mindent törlünk!
          MenuItem refreshBtn = menus[idx].items[currentSelection];

          // 2. Most már biztonságosan üríthetjük a listát
          menus[idx].items.clear();
          selectedIECModuleID = -1;
          
          // 3. Modulok újrakeresése
          _iec->discoverIECs();
          std::vector<uint8_t> ids = _iec->getFoundIECIDs();
          
          if (ids.size() == 0) {
            menus[idx].items.push_back(MenuItem("No IEC modules found!", MenuActionType::NONE));
          } else {
            for (size_t i = 0; i < ids.size(); ++i) {
              uint8_t moduleId = ids[i];
              menus[idx].items.push_back(MenuItem("IEC Module " + String(moduleId), MenuActionType::NAVIGATE, iecMenuId, [this, moduleId](){
                selectedIECModuleID = moduleId;
                updateIECValues(selectedIECModuleID);
                int _iecMenuId = -1; 
                for (auto &m : menus) if (String(m.title) == "IEC Module Menu") { _iecMenuId = m.id; break; }
                if (_iecMenuId >= 0) currentMenuId = _iecMenuId;
              }));
            }
          }

          // 4. TEGYÜK VISSZA A REFRESH GOMBOT A LISTA VÉGÉRE!
          menus[idx].items.push_back(refreshBtn);

          // 5. Kurzort visszaállítjuk a legelső elemre
          currentSelection = 0; 
          windowStart = 0; }));
    }
  }
  drawMenuWindow();
}

bool TFTDisplay::interruptTimer()
{
  if (millis() - lastButtonPressed > DEBOUNCE_DELAY)
  {
    lastButtonPressed = millis();
    return true;
  }
  return false;
}

// --- ISR handlers ---
void IRAM_ATTR TFTDisplay::isr_handleUp()
{
  if (_instance && _instance->interruptTimer())
    _instance->UpPressed = true;
}
void IRAM_ATTR TFTDisplay::isr_handleDown()
{
  if (_instance && _instance->interruptTimer())
    _instance->DownPressed = true;
}
void IRAM_ATTR TFTDisplay::isr_handleBack()
{
  if (_instance && _instance->interruptTimer())
    _instance->BackPressed = true;
}
void IRAM_ATTR TFTDisplay::isr_handleConfirm()
{
  if (_instance && _instance->interruptTimer())
    _instance->ConfirmPressed = true;
}

void TFTDisplay::processButton()
{
  if (UpPressed)
  {
    UpPressed = false; // Rögtön töröljük a flaget
    if (digitalRead(BTN_UP) == LOW)
    { // Zajszűrés: Tényleg le van nyomva?
      if (editing)
      {
        editor.nextInt();
        drawEditorScreen();
        return;
      }
      if (currentSelection > 0 && !dataview && !editing)
        currentSelection--;
    }
  }

  if (DownPressed)
  {
    DownPressed = false; // Rögtön töröljük a flaget
    if (digitalRead(BTN_DOWN) == LOW)
    { // Zajszűrés: Tényleg le van nyomva?
      if (editing)
      {
        editor.prevInt();
        drawEditorScreen();
        return;
      }
      int mIdx = findMenuIndexById(currentMenuId);
      if (mIdx >= 0 && !dataview && !editing)
      {
        if (currentSelection < (int)menus[mIdx].items.size() - 1)
          currentSelection++;
      }
    }
  }

  if (BackPressed)
  {
    BackPressed = false;
    if (digitalRead(BTN_BACK) == LOW)
    {
      int mIdx = findMenuIndexById(currentMenuId);
      if (mIdx >= 0 && menus[mIdx].parentId >= 0 && !editing && !dataview)
      {
        currentMenuId = menus[mIdx].parentId;
        currentSelection = 0;
        windowStart = 0;
        lastSelection = -1;
      }
      editing = false;
      dataview = false;
      drawMenuWindow();
    }
  }

  if (ConfirmPressed)
  {
    ConfirmPressed = false;
    if (digitalRead(BTN_CONFIRM) == LOW)
    {
      if (editing)
      {
        if (editorSaveCallback)
        {
          editorSaveCallback(editor.get());
        }
        editorSaveCallback = nullptr;
        editing = false;
        drawSavedSuccessScreen();
        drawMenuWindow();
        return;
      }

      int idx = findMenuIndexById(currentMenuId);
      if (idx < 0)
        return;
      if (currentSelection < 0 || currentSelection >= (int)menus[idx].items.size())
        return;
      MenuItem it = menus[idx].items[currentSelection];

      if (it.actionType == MenuActionType::NAVIGATE && !editing && !dataview)
      {
        if (it.targetMenuId >= 0)
        {
          currentMenuId = it.targetMenuId;
          currentSelection = 0;
          windowStart = 0;
          lastSelection = -1;
          if (it.cb)
            it.cb();
          drawMenuWindow();
        }
      }
      else if (it.actionType == MenuActionType::CALLBACK)
      {
        if (it.cb && !editing && !dataview)
          it.cb();
      }
    }
  }
}

//-----------------------------------------------------------------------------------------------------------------//
// Updating and drawing
//-----------------------------------------------------------------------------------------------------------------//

void TFTDisplay::updateActiveMenuPeriodic()
{
  if (millis() - lastAutoUpdate < autoUpdateInterval) return;
  lastAutoUpdate = millis();
  if (selectedIECModuleID != -1) updateIECValues(selectedIECModuleID);
  updateSystemValues();
  updateDynamicValues();
}

void TFTDisplay::updateDynamicValues()
{
  int mIdx = findMenuIndexById(currentMenuId);
  if (mIdx < 0 || dataview || editing)
    return;

  if (currentSelection < windowStart)
  {
    windowStart = currentSelection;
  }
  else if (currentSelection >= windowStart + maxVisible)
  {
    windowStart = currentSelection - maxVisible + 1;
  }

  int end = windowStart + maxVisible;
  if (end > (int)menus[mIdx].items.size())
    end = menus[mIdx].items.size();

  for (int i = windowStart; i < end; i++)
  {
    int y = startOfTexts + (i - windowStart) * rowHeight;

    if (i == currentSelection)
    {
      _tft.setTextColor(TFT_WHITE, TFT_BLUE);
      if((menus[mIdx].items[i].value).length() > 0) {
        _tft.fillRect(getXOffset(menus[mIdx].title), y, _tft.width(), rowHeight, TFT_BLUE);
        _tft.drawString(menus[mIdx].items[i].value, getXOffset(menus[mIdx].title), y);
      }
    }
    else
    {
      _tft.setTextColor(TFT_GREEN, TFT_BLACK);
      if((menus[mIdx].items[i].value).length() > 0) {
        _tft.fillRect(getXOffset(menus[mIdx].title), y, _tft.width(), rowHeight, TFT_BLACK);
        _tft.drawString(menus[mIdx].items[i].value, getXOffset(menus[mIdx].title), y);
      }
    }
  }
}

void TFTDisplay::drawMenuWindow()
{
  _tft.fillScreen(TFT_BLACK);
  _tft.drawWideLine(0, 5, _tft.width(), 5, 2, TFT_WHITE);
  _tft.drawWideLine(0, 29, _tft.width(), 29, 2, TFT_WHITE);

  _tft.setTextFont(2);
  _tft.setTextDatum(TL_DATUM);

  int idx = findMenuIndexById(currentMenuId);
  if (idx < 0 || dataview || editing)
    return;

  _tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  _tft.drawString(menus[idx].title, 10, 10);

  int end = windowStart + maxVisible;
  if (end > (int)menus[idx].items.size())
    end = menus[idx].items.size();

  for (int i = windowStart; i < end; i++)
  {
    int y = startOfTexts + (i - windowStart) * rowHeight;
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.drawString(menus[idx].items[i].name, startOfTextsLeft, y + startOfTextsFromBlue);
    if (menus[idx].items[i].value.length() > 0)
    {
      _tft.setTextColor(TFT_GREEN, TFT_BLACK);
      _tft.drawString(menus[idx].items[i].value, getXOffset(menus[idx].title), y);
    }
  }

  lastSelection = -1; // ready for cursor redraw
}

void TFTDisplay::updateIECValues(int id)
{
  if (id == -1)
    return;
  for (auto &m : menus)
  {
    if (String(m.title) == "IEC Info")
    {
      m.items[0].value = String(_iec->getIECID(id));
      m.items[1].value = String(_iec->getIECVersion(id));
      m.items[2].value = String(_iec->getIEC_AVAILABLE_LEDS(id));
      m.items[3].value = String(_iec->getIECCurrentLimit(id));
      m.items[4].value = String(_iec->getIECRelayCount(id));
      m.items[5].value = _iec->getIEC_IS_RMS_CURRENT_MEASURED(id) ? "YES" : "NO";
      m.items[6].value = _iec->getIEC_IS_RMS_VOLTAGE_MEASURED(id) ? "YES" : "NO";
      m.items[7].value = _iec->getIEC_IS_ACTIVE_POWER_MEASURED(id) ? "YES" : "NO";
      m.items[8].value = _iec->getIEC_IS_REACTIVE_POWER_MEASURED(id) ? "YES" : "NO";
      m.items[9].value = _iec->getIEC_IS_APPARENT_POWER_MEASURED(id) ? "YES" : "NO";
      m.items[10].value = _iec->getIEC_IS_POWER_FACTOR_MEASURED(id) ? "YES" : "NO";
      m.items[11].value = _iec->getIEC_IS_AC_FREQUENCY_MEASURED(id) ? "YES" : "NO";
    }

    if (String(m.title) == "IEC Status") {
      m.items.clear();
      addMenuItem(m.id, MenuItem("Status:", MenuActionType::NONE)); m.items.back().value = String(_iec->getIECStatus(id));
      addMenuItem(m.id, MenuItem("RMS Curr.:", MenuActionType::NONE)); m.items.back().value = String(_iec->getRMSCurrentData(id)) + "A";
      addMenuItem(m.id, MenuItem("RMS Volt.:", MenuActionType::NONE)); m.items.back().value = String(_iec->getRMSVoltageData(id)) + "V";
      addMenuItem(m.id, MenuItem("App. Power:", MenuActionType::NONE)); m.items.back().value = String(_iec->getApparentPowerData(id)) + "VA";
      // Relay toggler
      addMenuItem(m.id, MenuItem("Relay:", MenuActionType::CALLBACK, -1, [this, id](){
        _iec->setRelayStatus(id, !_iec->getIECRelayStatus(id));
      }));
      m.items.back().value = _iec->getIECRelayStatus(id) ? "[ON]" : "[OFF]";
    }
    
    if (String(m.title) == "IEC Settings")
    {
        m.items.clear();
        addMenuItem(m.id, MenuItem("Set Warning Curr.:", MenuActionType::CALLBACK, -1, [this, id]() { 
                                      startEditing(_iec->getCustCurrWarningLimit(id), [this, id](const String &val) {
                                      uint16_t currLimit = val.toInt();
                                      _iec->setCustCurrWarningLimit(id, currLimit); 
                                      }); 
                                  })); m.items.back().value = String(_iec->getCustCurrWarningLimit(id)) + "A";

        addMenuItem(m.id, MenuItem("Set Error Curr.:", MenuActionType::CALLBACK, -1, [this, id]() { 
                                      startEditing(_iec->getCustCurrErrorLimit(id), [this, id](const String &val) {
                                      uint16_t currLimit = val.toInt();
                                      _iec->setCustCurrErrorLimit(id, currLimit); 
                                      }); 
                                  })); m.items.back().value = String(_iec->getCustCurrErrorLimit(id)) + "A";
       
        addMenuItem(m.id, MenuItem("Meas avg num:", MenuActionType::CALLBACK, -1, [this, id]() { 
                                      startEditing(_iec->getIECAVGNum(id), [this, id](const String &val) {
                                      uint16_t num = val.toInt();
                                      _iec->setIECAVGNum(id, num); 
                                      }); 
                                  })); m.items.back().value = String(_iec->getIECAVGNum(id));
    }
  }
}

void TFTDisplay::updateSystemValues()
{
  for (auto &m : menus)
  {
    if (String(m.title) == "PDU Status")
    {
      m.items[2].value = String(_iec->getSumIECCurrentData()) + "A";
      m.items[3].value = String(_iec->getAvgIECVoltageData()) + "V";
      m.items[4].value = String(_iec->getSumIECPowerData()) + "VA";
      m.items[5].value = String(_iec->getOverCurrentTreshold()) + "A";
      m.items[6].value = "0.0.1";
    }

    if (String(m.title) == "AHT10 Sensor")
    {
      m.items[0].value = "--" + String((_envSensor->isFahrenheit()) ? "`F" : "`C");
      m.items[1].value = "-- %";
    }

    if (String(m.title) == "MQTT Settings")
    {
      m.items[0].value = mqttManager->isMQTTEnabled() ? "[ON]" : "[OFF]";
      m.items[1].value = mqttManager->getMQTTStatusString();
      m.items[2].value = readStringFromNVS(NVSKeys::MQTT_SERVER, "Not set");
    }
    if (String(m.title) == "WiFi Settings")
    {
      m.items[0].value = _networkMgr->getWifiSTA_Status() ? "[ON]" : "[OFF]";
      m.items[3].value = _networkMgr->getWifiAP_Status() ? "[ON]" : "[OFF]";
    }

    if (String(m.title) == "Measuring")
    {
      m.items[0].value = String((_envSensor->isFahrenheit()) ? "`F" : "`C");
      m.items[1].value = String(_iec->getPowerDataUpdateCycleTime()) + "s";
      m.items[2].value = String(_iec->getIECSwitchingDelay()) + "s";
    }
  }
}

void TFTDisplay::drawSavedSuccessScreen()
{
  _tft.fillScreen(TFT_BLACK);
  _tft.setTextColor(TFT_GREEN, TFT_BLACK);
  _tft.setTextFont(2);
  _tft.drawString("Value saved!", 60, 50);
  _tft.setTextFont(1);
  delay(1000);
  drawMenuWindow();
}

void TFTDisplay::updateCursor()
{
  int mIdx = findMenuIndexById(currentMenuId);
  if (mIdx < 0)
    return;

  if (currentSelection < windowStart)
  {
    windowStart = currentSelection;
    drawMenuWindow();
  }
  else if (currentSelection >= windowStart + maxVisible)
  {
    windowStart = currentSelection - maxVisible + 1;
    drawMenuWindow();
  }

  if (currentSelection == lastSelection)
    return;

  // old cursor redraw
  if (lastSelection >= windowStart && lastSelection < windowStart + maxVisible)
  {
    int yOld = startOfTexts + (lastSelection - windowStart) * rowHeight;
    _tft.fillRect(0, yOld, _tft.width(), rowHeight, TFT_BLACK);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.drawString(menus[mIdx].items[lastSelection].name, startOfTextsLeft, yOld + startOfTextsFromBlue);
    if (menus[mIdx].items[lastSelection].value.length() > 0)
    {
      _tft.setTextColor(TFT_GREEN, TFT_BLACK);
      _tft.drawString(menus[mIdx].items[lastSelection].value, getXOffset(menus[mIdx].title), yOld);
    }
  }

  // new cursor
  if (currentSelection >= windowStart && currentSelection < windowStart + maxVisible)
  {
    int yNew = startOfTexts + (currentSelection - windowStart) * rowHeight;
    _tft.fillRect(0, yNew, _tft.width(), rowHeight, TFT_BLUE);
    _tft.setTextColor(TFT_WHITE, TFT_BLUE);
    _tft.drawString(menus[mIdx].items[currentSelection].name, startOfTextsLeft, yNew + startOfTextsFromBlue);
    if (menus[mIdx].items[currentSelection].value.length() > 0)
    {
      _tft.setTextColor(TFT_WHITE, TFT_BLUE);
      _tft.drawString(menus[mIdx].items[currentSelection].value, getXOffset(menus[mIdx].title), yNew);
    }
  }

  lastSelection = currentSelection;
}

int TFTDisplay::getXOffset(String menuTitle = ""){
  if (menuTitle == "PDU Status") return 100;
  else if (menuTitle == "MQTT Settings") return 40;
  else if (menuTitle == "IEC Status") return 100;
  else if (menuTitle == "Measuring") return 135;
  else return 120;
}