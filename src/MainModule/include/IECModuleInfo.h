#ifndef IECModuleInfo_h
#define IECModuleInfo_h

#include <cstdint>
#include <IECRegisterMap.h>

#define BROADCAST_ADDRESS 0
#define MAX_SLAVES 50
#define MODBUS_BAUD 19200

#define NUM_INPUT_REGISTERS 256
#define NUM_HOLDING_REGISTERS 256
#define NUM_COILS 256
#define NUM_DISCRETEINPUTS 256

enum ledStates {                //collect which type of LEDs are available on the IEC module:
  noSupport = 0,  // none
  R = 1,          // only a red
  RG = 2,         // red and green
  RYG = 3,        // red, yellow and green
 };

 enum IECStatus {              // collect the status of the IEC module:
  noError = 0,     // no error
  error = 1,       // error
  warning = 2,     // warning
  info = 3,        // info
 };

enum currentLimit {             // collect which current limit is available on the IEC module:
  limit16A = 16,   // 16A
  limit32A = 32,   // 32A
 };

 struct Capabilities {          // collect which capabilities the IEC module has:
  currentLimit currLim;         // 16A or 32A
  uint8_t relayCount;           // up to 8 relays
  ledStates ledState;           // which type of LEDs are available on the IEC module
  uint8_t measureVoltageCount;  // number of voltage measurements (up to 8)
  uint8_t measureCurrentCount;  // number of current measurements (up to 8)
  uint8_t reserved;             // reserved for future use  
 };
 
class IECModuleInfo {
  public:    
  uint8_t id;                // ID of the IEC module
  uint8_t version;
  Capabilities capabilities;

  uint16_t inputRegisters[NUM_INPUT_REGISTERS] = {0};           // array to store the input registers of the IEC module
  uint16_t holdingRegisters[NUM_HOLDING_REGISTERS] = {0};       // array to store the holding registers of the IEC module
  bool coils[NUM_COILS] = {0};                                  // array to store the coils of the IEC module
  bool discreteInputs[NUM_DISCRETEINPUTS] = {0};                // array to store the coils of the IEC module

  bool setIECCapabilities();    // set the capabilities of the IEC module
};
#endif