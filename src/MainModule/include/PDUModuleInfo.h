#ifndef PDUMODULEINFO_H
#define PDUMODULEINFO_H

#include <cstdint>
#include <IECRegisterMap.h>
#include <IECModuleInfo.h>
#include <Esp.h>
#include <variables.h>

#define BROADCAST_ADDRESS 0
#define MAX_SLAVES 50
#define MODBUS_BAUD 115200

class PDUModuleInfo {
  public:    
  uint64_t getID();    // set the capabilities of the IEC module
  void setID(uint16_t _id);    // set the capabilities of the IEC module

  String getHWVersion();    
  void setHWVersion(String hwversion);  

  String getSWVersion();    
  void setSWVersion(String swversion);  

  float getCurrentData();    
  void setCurrentData(float currentData);   

  float getVoltageData(); 
  void setVoltageData(float voltageData); 

  float getPowerData();
  void setPowerData(float powerData); 

  float getEnergyData();   
  void setEnergyData(float energyData);    

  uint16_t getOvercurrThreshold();  
  void setOvercurrThreshold(uint16_t overcurrThreshold); 

  private:
    uint16_t _id;
    String _hwversion;
    String _swVersion;

    float _currentData;
    float _voltageData;
    float _powerData;
    float _energyData;
    uint16_t _overcurrThreshold;
};
#endif