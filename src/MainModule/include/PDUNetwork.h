#ifndef PDUNETWORK_H
#define PDUNETWORK_H

#include <cstdint>
#include <vector>
#include <map>
#include <set>

#include <pinouts.h>
#include <ModbusRTUMaster.h>
#include <ModbusRTUSlave.h>
#include <IECModuleInfo.h>
#include <IECRegisterMap.h>
#include <IECControl.h>
#include <networkLayerManager.h>
#include <debug.h>
#include <PDUModuleInfo.h>
#include "esp_mac.h"
#include <HTTPClient.h>

class PDUNetwork {
  public:
    PDUNetwork(IECControl* iec, networkLayerManager* nwLayer, HardwareSerial &RS485Serial);
    bool isPDUMaster() { return !_isSlave; } // Check if the current device is a PDU master

    void discoverPDUs(int availablePDUCount);

    void handleGetID(AsyncWebServerRequest *request);

    void requestNetworkID();

    void setSlaveStatus();

    void PDUNetworkLoop();

    bool setExtRelayStatus(uint8_t id, bool status);

private:
    void sendInitCommand(); // Send initialization command to the PDU 
    void sendIDToBus(uint8_t id); // Send the ID to the bus
    void sendEndOfInitCommand(); // Send end of initialization command to the PDU

    ModbusRTUMaster _mbMaster;
    ModbusRTUSlave _mbSlave;
    IECControl *_iec; // Instance of IECControl class to manage IEC modules
    networkLayerManager *_nwLayerManager; // Instance of networkLayerManager class to manage network connections

    uint8_t _slaveID; // ID of the slave device
    int _nextSlaveID;
    int _maxSlaveId;
    std::map<String, int> _assignedIDs;    // UID → Modbus ID hozzárendelés
    bool _isSlave; // Status of the slave device (true = active, false = inactive)
    bool _testState; // Test state for the slave device
};
#endif