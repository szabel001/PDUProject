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
#include <variables.h>

struct configData {
  uint16_t id;
  uint16_t version;
  uint16_t relayCount;
  uint16_t currentLimit;
  uint16_t isRMSVoltageMeasured;
  uint16_t isRMSCurrentMeasured;
  uint16_t isActivePowerMeasured;
  uint16_t isReactivePowerMeasured;
  uint16_t isApparentPowerMeasured;
  uint16_t isPowerFactorMeasured;
  uint16_t isACFrequencyMeasured;
};

struct MeasurementData {
  float rmsCurrent;
  float rmsVoltage;
  float activePower;
  float reactivePower;
  float apparentPower;
  float powerFactor;
  float acFrequency;
};

class IECControl {
  public:
    IECControl(HardwareSerial& RS485Serial);      // Constructor to initialize the Modbus master with the given serial port
    void IECSetup();
    void IECReadLoop();

    bool setRelayStatus(uint8_t id, bool status); // Set the relay status for the given slave ID
    bool getRelayStatus(uint8_t id); // Get the relay status for the given slave ID

    float getRMSCurrentData(uint8_t id);       // [A] Get the current data from the IECModuleInfo's local register
    float getRMSVoltageData(uint8_t id);       // [V] Get the voltage data from the IECModuleInfo's local register
    float getActivePowerData(uint8_t id);         // [W] Get the power data from the IECModuleInfo's local register
    float getReactivePowerData(uint8_t id);       // [VAR] Get the reactive power data from the IECModuleInfo's local register
    float getApparentPowerData(uint8_t id);       // [VA] Get the apparent power data from the IECModuleInfo's local registers
    float getFrequencyData(uint8_t id);     // [Hz] Get the frequency data from the IECModuleInfo's local register
    float getPowerFactorData(uint8_t id);   // [1] Get the power factor data from the IECModuleInfo's local register

    MeasurementData getMeasurementData(uint8_t id);       // Get the relay status from all slaves
    configData getConfigData(uint8_t id);

    uint16_t getIECID(uint8_t id); // Get the Modbus ID from the given slave ID
    uint16_t getIECVersion(uint8_t id); // Get the version from the given slave ID
    uint16_t getIECRelayCount(uint8_t id); // Get the relay count from the given slave ID
    uint16_t getIECCurrentLimit(uint8_t id); // Get the current limit from the given slave ID
    uint16_t getIEC_IS_VOLTAGE_MEASURED(uint8_t id); // Get the measure voltage count from the given slave ID
    uint16_t getIEC_IS_CURRENT_MEASURED(uint8_t id); // Get the measure current count from the given slave ID
    uint16_t getIECRelayStatus(uint8_t id); // Get the relay status from the given slave ID

    uint16_t getIEC_IS_RMS_CURRENT_MEASURED(uint8_t id);
    uint16_t getIEC_IS_RMS_VOLTAGE_MEASURED(uint8_t id);
    uint16_t getIEC_IS_ACTIVE_POWER_MEASURED(uint8_t id);
    uint16_t getIEC_IS_REACTIVE_POWER_MEASURED(uint8_t id);
    uint16_t getIEC_IS_APPARENT_POWER_MEASURED(uint8_t id);
    uint16_t getIEC_IS_POWER_FACTOR_MEASURED(uint8_t id);
    uint16_t getIEC_IS_AC_FREQUENCY_MEASURED(uint8_t id);
    uint16_t getIEC_AVAILABLE_LEDS(uint8_t id);

    void setCurrWarningLimit(uint8_t id, float level);
    void setOverCurrentTreshold(uint8_t id, float level);
    uint16_t getOverCurrentTreshold();

    uint16_t getPowerDataUpdateCycleTime();

    float getCurrWarningLimit(uint8_t id); // Get the warning level for the given slave ID
    float getCurrErrorLimit(uint8_t id); // Get the error level for the given slave ID

    void collectIECModuleInfos();

    IECStatus getIECStatus(uint8_t id);
    void setpowerDataUpdateCycleTime(uint16_t cycleTime);  // Set the global cycle time for the power data acquisition 

    uint16_t powerDataUpdateCycleTime = 1; // [sec]

    std::vector<uint8_t> getFoundIECIDs(); // Vector to store discovered slave IDs
    std::vector<uint8_t> discoverIECs();

    float getSumIECCurrentData();       // [A] Get the sum of current data from all IECModuleInfo's local register
    float getAvgIECVoltageData();       // [V] Get the sum of voltage data from all IECModuleInfo's local register
    float getSumIECPowerData();         // [W] Get the sum of power
    float getAvgIECFrequencyData();     // [Hz] Get the average of frequency data from all IECModuleInfo's local register

    void setAllIecRelaysOff(); // Set all IEC module relays off
    void setAllIecRelaysOn();  // Set all IEC module relays on

    void setDelayedTurnOnTimeO(uint16_t delayTime);  // Set the given IEC module relay on after delay time
    uint16_t getDelayedTurnOnTime();  // Get the given IEC module relay on delay time
    void setDelayedTurnOffTime(uint16_t delayTime); // Set the given IEC module
    uint16_t getDelayedTurnOffTime(); // Get the given IEC module relay off delay time

private:
    bool _isIECCommunicate(uint8_t id);
    uint16_t _getIntDataFromInputRegister(uint8_t id, uint16_t startOfData);
    bool _updateMeasurement();
    bool _updateMeasurement(uint8_t id);

    float _getFloatDataFromInputRegister(uint8_t id, uint16_t startOfData);
    uint32_t _convertFloatToIEEE754(float value);
    void _readIECFloatInputRegister(uint16_t id, uint16_t startOfData);
    void _readIECUINT16InputRegister(uint16_t id, uint16_t startOfData);

    void _readIECCoil(uint8_t id, uint16_t startOfData);

    void _readIECCapabilities(uint8_t id);        //Read all capabilities in one function
    void _readID(uint8_t id);
    void _readIECVersion(uint8_t id);
    void _readIECRelayCount(uint8_t id);
    void _readIECCurrentLimit(uint8_t id);
    void _readIECMeasureVoltageCount(uint8_t id);
    void _readIECMeasureCurrentCount(uint8_t id);
    void _readIECAvailableLEDs(uint8_t id);
    void _readIEC_IS_RMS_VOLTAGE_MEASURED(uint8_t id);
    void _readIEC_IS_RMS_CURRENT_MEASURED(uint8_t id);
    void _readIEC_IS_ACTIVE_POWER_MEASURED(uint8_t id);
    void _readIEC_IS_REACTIVE_POWER_MEASURED(uint8_t id);
    void _readIEC_IS_APPARENT_POWER_MEASURED(uint8_t id);
    void _readIEC_IS_POWER_FACTOR_MEASURED(uint8_t id);
    void _readIEC_IS_AC_FREQUENCY_MEASURED(uint8_t id);
    void _readIECStatus(uint8_t id); // IEC's status is Error/Off/Standby/Active TODO implement

    void _readCustCurrWarningLimit(uint8_t id);
    void _readCustCurrErrorLimit(uint8_t id);

    void _writeCustCurrWarningLimit(uint8_t id, float value);
    void _writeCustCurrErrorLimit(uint8_t id, float value);

    void _readRelayStatus(uint8_t id);            //Read the relay status from the given slave ID over the RS485 bus

    void _read_RMS_VOLTAGE(uint8_t id);
    void _read_RMS_CURRENT(uint8_t id);
    void _read_ACTIVE_POWER(uint8_t id);
    void _read_REACTIVE_POWER(uint8_t id);
    void _read_APPARENT_POWER(uint8_t id);
    void _read_POWER_FACTOR(uint8_t id);
    void _read_AC_FREQUENCY(uint8_t id);

    ModbusRTUMaster _mbMaster;
    String _debugString = "";
    std::map<uint8_t, IECModuleInfo> _iecModules;
    std::vector<uint8_t> _foundIECIDs; // Vector to store discovered slave IDs
    unsigned long _startMillis_powerRead;
};
#endif