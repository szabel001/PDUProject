#ifndef PDUMODULEINFO_H
#define PDUMODULEINFO_H

#include <cstdint>
#include <IECRegisterMap.h>
#include <IECModuleInfo.h>
#include <Esp.h>

#define BROADCAST_ADDRESS 0
#define MAX_SLAVES 50
#define MODBUS_BAUD 115200

class PDUModuleInfo {
  public:    
  uint64_t ID;  // 48 bit MAC addr
  uint8_t version;

  bool isWifiActive;  // Wi-Fi is currently active
  bool isEthernetActive; // Ethernet is currently active
  bool isAHT10Active; // AHT10 is connected and sending valid data

  uint64_t getID();    // set the capabilities of the IEC module
  void setID(uint16_t _id);    // set the capabilities of the IEC module
};
#endif