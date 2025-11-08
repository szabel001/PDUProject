#ifndef IECCONTROL_H
#define IECCONTROL_H

#include <cstdint>
#include <vector>
#include <map>
#include <cstring>

#include <pinouts.h>
#include <ModbusRTUMaster.h>
#include <debug.h>
#include <IECModuleInfo.h>

struct MeasurementData {
  float currentData;
  float voltageData;
  float powerData;
  float frequencyData;
  float powerFactorData;
};

class IECControl {
  public:
    IECControl(HardwareSerial& RS485Serial);      // Constructor to initialize the Modbus master with the given serial port
    void IECSetup();
    void IECReadLoop();

    bool setRelayStatus(uint8_t id, bool status); // Set the relay status for the given slave ID
    bool getRelayStatus(uint8_t id); // Get the relay status for the given slave ID

    float getCurrentData(uint8_t id);       // [A] Get the current data from the IECModuleInfo's local register
    float getVoltageData(uint8_t id);       // [V] Get the voltage data from the IECModuleInfo's local register
    float getPowerData(uint8_t id);         // [W] Get the power data from the IECModuleInfo's local register
    float getFrequencyData(uint8_t id);     // [Hz] Get the frequency data from the IECModuleInfo's local register
    float getPowerFactorData(uint8_t id);   // [1] Get the power factor data from the IECModuleInfo's local register

    MeasurementData getMeasurementData(uint8_t id);       // Get the relay status from all slaves

    uint16_t getIECID(uint8_t id); // Get the Modbus ID from the given slave ID
    uint16_t getIECVersion(uint8_t id); // Get the version from the given slave ID
    uint16_t getIECRelayCount(uint8_t id); // Get the relay count from the given slave ID
    uint16_t getIECCurrentLimit(uint8_t id); // Get the current limit from the given slave ID
    uint16_t getIEC_IS_VOLTAGE_MEASURED(uint8_t id); // Get the measure voltage count from the given slave ID
    uint16_t getIEC_IS_CURRENT_MEASURED(uint8_t id); // Get the measure current count from the given slave ID
    uint16_t getIECRelayStatus(uint8_t id); // Get the relay status from the given slave ID

    uint16_t getIEC_IS_RMS_CURRENT_MEASURED(uint8_t id); // Get the relay status from the given slave ID
    uint16_t getIEC_IS_RMS_VOLTAGE_MEASURED(uint8_t id); // Get the relay status from the given slave ID
    uint16_t getIEC_IS_ACTIVE_POWER_MEASURED(uint8_t id); // Get the relay status from the given slave ID
    uint16_t getIEC_IS_REACTIVE_POWER_MEASURED(uint8_t id);       // Get the relay status from the given slave ID
    uint16_t getIEC_IS_APPARENT_POWER_MEASURED(uint8_t id); // Get the relay status from the given slave ID
    uint16_t getIEC_IS_POWER_FACTOR_MEASURED(uint8_t id); // Get the relay status from the given slave ID
    uint16_t getIEC_IS_AC_FREQUENCY_MEASURED(uint8_t id); // Get the relay status from the given slave ID
    uint16_t getIEC_AVAILABLE_LEDS(uint8_t id); // Get the available leds from the given slave ID

    bool setCurrWarningLimit(uint8_t id, float level);

    float getCurrWarningLimit(uint8_t id); // Get the warning level for the given slave ID
    float getCurrErrorLimit(uint8_t id); // Get the error level for the given slave ID

    void collectIECModuleInfos();

    uint16_t getIECStatus(uint8_t id);
    void setpowerDataUpdateCycleTime(uint16_t cycleTime);  // Set the global cycle time for the power data acquisition 

    uint16_t powerDataUpdateCycleTime = 1000; //ms -> default 1s

    std::vector<uint8_t> getFoundIECIDs(); // Vector to store discovered slave IDs

private:
    std::vector<uint8_t> _discoverIECs();
    bool _isIECCommunicate(uint8_t id);
    uint16_t _getIntDataFromInputRegister(uint8_t id, uint16_t startOfData);
    bool _updateMeasurement();
    bool _updateMeasurement(uint8_t id);

    float _getFloatDataFromInputRegister(uint8_t id, uint16_t startOfData);
    uint32_t _convertFloatToIEEE754(float value);
    void _readIECFloatInputRegister(uint16_t id, uint16_t startOfData);
    void _readIECUINT16InputRegister(uint16_t id, uint16_t startOfData);

    void _readIECCapabilities(uint8_t id);        //Read all capabilities in one function
    void _readID(uint8_t id);
    void _readIECVersion(uint8_t id);
    void _readIECRelayCount(uint8_t id);
    void _readIECCurrentLimit(uint8_t id);
    void _readIECMeasureVoltageCount(uint8_t id);
    void _readIECMeasureCurrentCount(uint8_t id);
    void _readIECStatus(uint8_t id);              // IEC's status is Error/Off/Standby/Active TODO implement

    void _readCustCurrWarningLimit(uint8_t id);
    void _readCustCurrErrorLimit(uint8_t id);

    void _writeCustCurrWarningLimit(uint8_t id, float value);
    void _writeCustCurrErrorLimit(uint8_t id, float value);

    void _readRelayStatus(uint8_t id);            //Read the relay status from the given slave ID over the RS485 bus

    void _readCurrentData(uint8_t id);            //Read the current from the given slave ID over the RS485 bus
    void _readVoltageData(uint8_t id);            //Read the voltage from the given slave ID over the RS485 bus
    void _readPowerData(uint8_t id);              //Read the power from the given slave ID over the RS485 bus
    void _readFrequencyData(uint8_t id);          //Read the frequency from the given slave ID over the RS485 bus
    void _readPowerFactorData(uint8_t id);        //Read the power factor from the given slave ID over the RS485 bus

    ModbusRTUMaster _mbMaster;
    String _debugString = "";
    std::map<uint8_t, IECModuleInfo> _iecModules;
    std::vector<uint8_t> _foundIECIDs; // Vector to store discovered slave IDs
    unsigned long _startMillis_powerRead;
};
#endif