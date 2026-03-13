#ifndef TFTDisplay_h
#define TFTDisplay_h

#include <TFT_eSPI.h>
#include <IECControl.h>
#include <vector>
#include "EditableField.h"
#include "networkLayerManager.h"
#include "EnvironmentSensor.h"
#include <mqttHandler.h>


#define DEBOUNCE_DELAY 150

enum class MenuActionType {
  NONE,
  NAVIGATE,   // go to another menu id
  CALLBACK    // run callback
};

struct MenuItem {
  String name;
  String value;                 // visible value
  MenuActionType actionType;
  int targetMenuId;            // for NAVIGATE
  std::function<void()> cb;    // for CALLBACK

  MenuItem(String n = "", MenuActionType t = MenuActionType::NONE, int target = -1, std::function<void()> f = nullptr)
    : name(n), value(""), actionType(t), targetMenuId(target), cb(f) {}
};

struct Menu {
  int id;                      // unique id
  const char* title;
  std::vector<MenuItem> items;
  int parentId;                // -1 for root

  Menu(int _id = -1, const char* t = "", int p = -1) : id(_id), title(t), parentId(p) {}
};


class TFTDisplay {
public:
  TFTDisplay();
  void setupDisplay(IECControl &iec, networkLayerManager &networkMgr, EnvironmentSensor &envSensor);
  void drawEditorScreen();
  void drawDataViewScreen(const String &title, const std::vector<String> &dataLines);
  void setupMenu();
  void processButton();
  void updateIECValues(int id);
  void updateActiveMenuPeriodic();
  void updateDynamicValues();
  void drawMenuWindow();
  void updateIECDetailMenus(int id);
  void updateSystemValues();
  void drawSavedSuccessScreen();
  void updateCursor();

  static int getXOffset(String menuTitle);

  private:
  std::vector<Menu> menus;
  int addMenu(const char* title,int parentId=-1);
  void addMenuItem(int menuId,const MenuItem& item);
  int findMenuIndexById(int id);
  Menu *getMenuById(int id);
  void startEditing(int initialValue, std::function<void(const String &)> saveCb);
  void buildNetworkingMenu(int settingsMenuId);

  int currentMenuId=-1;
  volatile int currentSelection=0;
  int lastSelection=-1;
  int selectedIECModuleID=-1;
  unsigned long lastAutoUpdate=0;
  const unsigned long autoUpdateInterval=1000;

  TFT_eSPI _tft;
  IECControl* _iec=nullptr;
  networkLayerManager* _networkMgr=nullptr;
  EnvironmentSensor* _envSensor=nullptr;
  
  int windowStart=0;
  const int maxVisible=4;
  const int rowHeight=20;
  uint8_t startOfTexts=35;
  uint8_t startOfTextsLeft=5;
  uint8_t startOfTextsFromBlue=3;

  static TFTDisplay* _instance;
  static void IRAM_ATTR isr_handleUp();
  static void IRAM_ATTR isr_handleDown();
  static void IRAM_ATTR isr_handleBack();
  static void IRAM_ATTR isr_handleConfirm();
  bool interruptTimer();
  volatile unsigned long lastButtonPressed=0;
  volatile bool UpPressed=false;
  volatile bool DownPressed=false;
  volatile bool BackPressed=false;
  volatile bool ConfirmPressed=false;

  bool backSelected = false;
  bool saveSelected = false;

  bool editing = false;
  bool dataview = false;
  EditableField editor;
  std::function<void(const String&)> editorSaveCallback;
  unsigned long confirmPressStart = 0;
  unsigned long backPressStart = 0;
  bool confirmLongPressed = false;
  bool backLongPressed = false;
};

#endif