#include "TFTDisplay.h"
#define DEBOUNCE_DELAY 150

TFTDisplay* TFTDisplay::_instance = nullptr;

TFTDisplay::TFTDisplay() {
    _instance = this;
    currentMenu = nullptr;
}

// --- Menü setup ---
void TFTDisplay::setupMenu() {
    // --- Főmenü ---
    mainItems[0] = {"Settings", SETTINGS};
    mainItems[1] = {"IEC Modules Menu", IEC};
    mainItems[2] = {"PDU Status", PDU_STATUS};
    mainItems[3] = {"AHT10", AHT10};
    mainMenu = {"Main Menu", mainItems, 4, nullptr};

    // --- Settings Menü ---
    settingsItems[0] = {"Wifi", SETTINGS};
    settingsItems[1] = {"Ethernet", SETTINGS};
    settingsItems[2] = {"PDU Modbus", SETTINGS};
    settingsItems[3] = {"IEC Modbus", SETTINGS};
    settingsItems[4] = {"Measuring", SETTINGS};

    settingsMenu = {"Settings", settingsItems, 5, &mainMenu};

    //wifiItems[0] = {"SSID:", SETTINGS};
    //wifiItems[1] = {"Password:", SETTINGS}; 
    //wifiMenu = {"Wifi Settings", wifiItems, 2, &settingsMenu};


    // --- IEC Module lista generálása ---
    std::vector<uint8_t> ids = _iec->getFoundIECIDs();
    uint8_t count = ids.size();

    for (int i = 0; i < count; i++) {
        String txt = "IEC Module " + String(ids[i]);
        iecModulesItems[i] = { strdup(txt.c_str()), IEC_INFO };  
        // IEC_INFO → tovább lépünk a modul részletes menübe
    }

    iecModulesMenu = {"Select an IEC Module!", iecModulesItems, count, &mainMenu};

    iecItems[0] = {"IEC Info", IEC_INFO};
    iecItems[1] = {"IEC Status", IEC_STATUS};
    iecItems[2] = {"IEC Settings", IEC_SETTINGS};
    iecMenu = {"IEC Modules", iecItems, 3, &iecModulesMenu};

    iecInfoDetailMenu = {"IEC Info", iecInfoDetailItems, 7, &iecMenu};
    iecStatusDetailMenu = {"IEC Status", iecStatusDetailItems, 7, &iecMenu};
    iecSettingDetailMenu = {"IEC Settings", iecSettingDetailItems, 5, &iecMenu};
    
    currentMenu = &mainMenu;
}

// --- Display setup ---
void TFTDisplay::setupDisplay(IECControl& iec) {
    _iec = &iec;
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

    drawMenuWindow(); // első ablak
}

bool TFTDisplay::interruptTimer() { 
    if(millis() - lastButtonPressed > DEBOUNCE_DELAY) {
        lastButtonPressed = millis();
        return true;
    }
    return false;
}

// --- ISR-ek ---
void IRAM_ATTR TFTDisplay::isr_handleUp() { if(_instance && _instance->interruptTimer()) _instance->rawUpPressed = true; _instance->rawAnyButton = true;}
void IRAM_ATTR TFTDisplay::isr_handleDown() { if(_instance && _instance->interruptTimer()) _instance->rawDownPressed = true; _instance->rawAnyButton = true;}
void IRAM_ATTR TFTDisplay::isr_handleBack() { if(_instance && _instance->interruptTimer()) _instance->rawBackPressed = true; _instance->rawAnyButton = true;}
void IRAM_ATTR TFTDisplay::isr_handleConfirm() { if(_instance && _instance->interruptTimer()) _instance->rawConfirmPressed = true; _instance->rawAnyButton = true;}

// --- Debounce ---
void TFTDisplay::processButtonDebounce() {
    unsigned long now = millis();
    if(rawAnyButton) rawAnyButton = false;
    else return;

    if(rawUpPressed && (now - lastUpTime > DEBOUNCE_DELAY)){
        lastUpTime = now;
        if(currentSelection>0) currentSelection--;
        rawUpPressed = false;
    }

    if(rawDownPressed && (now - lastDownTime > DEBOUNCE_DELAY)){
        lastDownTime = now;
        if(currentSelection<currentMenu->itemCount-1) currentSelection++;
        rawDownPressed = false;
    }

    if(rawBackPressed && (now - lastBackTime > DEBOUNCE_DELAY)){
        lastBackTime = now;
        if(currentMenu->parent) {
            currentMenu = currentMenu->parent;
            currentSelection=0;
            windowStart=0;
            lastSelection=-1;
            drawMenuWindow();
        }
        rawBackPressed = false;
    }

    if(rawConfirmPressed && (now - lastConfirmTime > DEBOUNCE_DELAY)){
        lastConfirmTime = now;
        menuState next = currentMenu->items[currentSelection].nextMenu;
        // 1) Ha az IEC Module listában vagyunk → ID kiválasztása
        if (currentMenu == &iecModulesMenu) {

            int id = _iec->getFoundIECIDs()[currentSelection];
            selectedIECModuleID = id;

            // Részletes értékek frissítése
            updateIECDetailMenus(id);

            currentMenu = &iecMenu;
        }
        // Relay toggle, ha a relay menüpontban vagyunk (IEC Status detail)
        if (currentMenu == &iecStatusDetailMenu && currentSelection == 3) {
            _iec->setRelayStatus(selectedIECModuleID, !_iec->getIECRelayStatus(selectedIECModuleID));
            updateIECDetailMenus(selectedIECModuleID);
            drawMenuWindow();
            rawConfirmPressed = false;
            return;
        }
        else {
            switch(next){
                case SETTINGS: currentMenu=&settingsMenu; break;
                case IEC: currentMenu=&iecModulesMenu; break;
                case PDU_STATUS: /*currentMenu=&pduStatusMenu;*/ break;
                case AHT10: /*currentMenu=&aht10Menu;*/ break;

                case IEC_INFO: currentMenu=&iecInfoDetailMenu; break;
                case IEC_STATUS: currentMenu=&iecStatusDetailMenu; break;
                case IEC_SETTINGS: currentMenu=&iecSettingDetailMenu; break;
                default: break;
            }
        }
        currentSelection=0;
        windowStart=0;
        lastSelection=-1;
        drawMenuWindow();
        rawConfirmPressed=false;
    }
}

void TFTDisplay::updateIECDetailMenus(int id) {

    // --- IEC INFO ---
    iecInfoDetailItems[0] = {"ID:", IEC_INFO, String(_iec->getIECID(id))};
    iecInfoDetailItems[1] = {"Version:", IEC_INFO, String(_iec->getIECVersion(id))};
    iecInfoDetailItems[2] = {"Available LEDs:", IEC_INFO, String(_iec->getIEC_AVAILABLE_LEDS(id))};
    iecInfoDetailItems[3] = {"Current limit:", IEC_INFO, String(_iec->getIECCurrentLimit(id))};
    iecInfoDetailItems[4] = {"Relay count:", IEC_INFO, String(_iec->getIECRelayCount(id))};
    iecInfoDetailItems[5] = {"Current measured:", IEC_INFO, _iec->getIEC_IS_CURRENT_MEASURED(id) ? "YES" : "NO"};
    iecInfoDetailItems[6] = {"Voltage measured:", IEC_INFO, _iec->getIEC_IS_VOLTAGE_MEASURED(id) ? "YES" : "NO"};

    // --- IEC STATUS ---
    iecStatusDetailItems[0] = {"Current (A):", IEC_STATUS, String(_iec->getCurrentData(id))};
    iecStatusDetailItems[1] = {"Voltage (V):", IEC_STATUS, String(_iec->getVoltageData(id))};
    iecStatusDetailItems[2] = {"Power (W):", IEC_STATUS, String(_iec->getPowerData(id))};
    iecStatusDetailItems[3] = {"Relay:", IEC_STATUS, _iec->getIECRelayStatus(id) ? "[ON]" : "[OFF]"};
}

void TFTDisplay::updateActiveMenuPeriodic() {

    if (millis() - lastAutoUpdate < autoUpdateInterval)
        return;

    lastAutoUpdate = millis();

    // Csak akkor frissítünk, ha az IEC Stato vagy IEC Info detail-ben vagyunk
    if (currentMenu == &iecStatusDetailMenu ||
        currentMenu == &iecInfoDetailMenu)
    {
        updateIECDetailMenus(selectedIECModuleID);

        // csak a változott értékeket rajzoljuk újra
        updateDynamicValues();
    }
}

void TFTDisplay::updateDynamicValues() {
    if(currentSelection < windowStart) {
    windowStart = currentSelection;
    }
    else if(currentSelection >= windowStart + maxVisible) {
        windowStart = currentSelection - maxVisible + 1;
    }

    int end = windowStart + maxVisible;
    if(end > currentMenu->itemCount) end = currentMenu->itemCount;

    for (int i = windowStart; i < end; i++) {
        int y = startOfTexts + (i - windowStart)*rowHeight;

        if(i == currentSelection) {
            _tft.setTextColor(TFT_WHITE, TFT_BLUE);
        }
        else {
            _tft.setTextColor(TFT_GREEN, TFT_BLACK);
        }
        _tft.fillRect(140, y, 50, rowHeight, TFT_BLACK);
        _tft.drawString(currentMenu->items[i].value, 100, y);

    }
}

void TFTDisplay::drawMenuWindow() {
    _tft.fillScreen(TFT_BLACK);

    _tft.drawWideLine(0,5,_tft.width(),5,2,TFT_WHITE);
    _tft.drawWideLine(0,29,_tft.width(),29,2,TFT_WHITE);

    _tft.setTextFont(2);
    _tft.setTextDatum(TL_DATUM);

    _tft.setTextColor(TFT_YELLOW,TFT_BLACK);
    _tft.drawString(currentMenu->title,10,10);

    int end = windowStart + maxVisible;
    if(end > currentMenu->itemCount) end = currentMenu->itemCount;

    for(int i = windowStart; i < end; i++){
        int y = startOfTexts + (i - windowStart)*rowHeight;
        _tft.setTextColor(TFT_WHITE,TFT_BLACK);
        _tft.drawString(currentMenu->items[i].name,startOfTextsLeft,y+startOfTextsFromBlue);
        if(currentMenu->items[i].value.length() > 0) {
            _tft.setTextColor(TFT_GREEN, TFT_BLACK);
            _tft.drawString(currentMenu->items[i].value, 100, y);
        }
    }

    lastSelection = -1; // újrarajzolásra készen
}

void TFTDisplay::updateMenuValues() {
    if (selectedIECModuleID != -1) {
            updateIECDetailMenus(selectedIECModuleID);
            if (currentMenu == &iecInfoDetailMenu || currentMenu == &iecStatusDetailMenu) {
                drawMenuWindow();
            }
    }
    //Temp sensor data
    // AHT10 szenzor
    // aht10Menu.items[0].value = String(aht10.getTemperature()) + "C";
    // aht10Menu.items[1].value = String(aht10.getHumidity()) + "%";
}


void TFTDisplay::updateCursor() {
    // Ha a kiválasztott menüpont kimegy a windowból → görgetés
    if(currentSelection < windowStart) {
        windowStart = currentSelection;
        drawMenuWindow();
    }
    else if(currentSelection >= windowStart + maxVisible) {
        windowStart = currentSelection - maxVisible + 1;
        drawMenuWindow();
    }

    // Ha nincs változás → semmit se rajzolunk
    if(currentSelection == lastSelection) return;

    // Régi kurzor visszaállítása
    if(lastSelection >= windowStart && lastSelection < windowStart + maxVisible){
        int yOld = startOfTexts + (lastSelection - windowStart)*rowHeight;
        _tft.fillRect(0, yOld, 200, rowHeight, TFT_BLACK);
        _tft.setTextColor(TFT_WHITE,TFT_BLACK);
        _tft.drawString(currentMenu->items[lastSelection].name, startOfTextsLeft, yOld + startOfTextsFromBlue);
        if(currentMenu->items[lastSelection].value.length() > 0) {
            _tft.setTextColor(TFT_GREEN, TFT_BLACK);
            _tft.drawString(currentMenu->items[lastSelection].value, 100, yOld);
        }
    }

    // Új kurzor rajzolása
    if(currentSelection >= windowStart && currentSelection < windowStart + maxVisible){
        int yNew = startOfTexts + (currentSelection - windowStart)*rowHeight;
        _tft.fillRect(0, yNew, 200, rowHeight, TFT_BLUE);
        _tft.setTextColor(TFT_WHITE,TFT_BLUE);
        _tft.drawString(currentMenu->items[currentSelection].name, startOfTextsLeft, yNew + startOfTextsFromBlue);
        if(currentMenu->items[currentSelection].value.length() > 0) {
            _tft.setTextColor(TFT_WHITE, TFT_BLUE);
            _tft.drawString(currentMenu->items[currentSelection].value, 100, yNew);
        }
    }
    lastSelection = currentSelection;
}

