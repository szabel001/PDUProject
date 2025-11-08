#include "IECControl.h"

//==========================================================//
//==================== Modbus parameters ===================//
//==========================================================//
const String errorStrings[] = {
  "success",
  "invalid id",
  "invalid buffer",
  "invalid quantity",
  "response timeout",
  "frame error",
  "crc error",
  "unknown comm error",
  "unexpected id",
  "exception response",
  "unexpected function code",
  "unexpected response length",
  "unexpected byte count",
  "unexpected address",
  "unexpected value",
  "unexpected quantity"
};

//==========================================================//
//=============== Constructor, init, loop ==================//
//==========================================================//

IECControl::IECControl(HardwareSerial& RS485Serial) : _mbMaster(RS485Serial, RS485_IEC_DE_RE_PIN, RS485_IEC_DE_RE_PIN) {
  RS485Serial.begin(MODBUS_BAUD, SERIAL_8N1, RS485_IEC_RX_GPIO, RS485_IEC_TX_GPIO);
  _mbMaster.begin(MODBUS_BAUD);
  _mbMaster.setTimeout(50); // Set modbus msg timeout to 10ms
  digitalWrite(RS485_IEC_DE_RE_PIN, LOW);
  collectIECModuleInfos();
  _startMillis_powerRead = millis(); // Initialize the start time for power data update
}

void IECControl::IECSetup() {
  collectIECModuleInfos();
}

void IECControl::IECReadLoop() {
  unsigned long _currentMillis_powerRead = millis();
  if (_currentMillis_powerRead - _startMillis_powerRead >= powerDataUpdateCycleTime) {
    _updateMeasurement();
    _startMillis_powerRead = _currentMillis_powerRead;
  }
}

void IECControl::collectIECModuleInfos() {
  _iecModules.erase(_iecModules.begin(), _iecModules.end());
  std::vector<uint8_t> foundIDs = _discoverIECs();
  for (uint8_t id : foundIDs) {
    IECModuleInfo module;
    module.id = id;
    _iecModules[id] = module;
    _readIECCapabilities(id);
  }
}

//==========================================================//
//========================== Setters =======================//
//==========================================================//

bool IECControl::setRelayStatus(uint8_t id, bool status) {
  ModbusRTUMasterError _error = _mbMaster.writeSingleCoil(id, 0, status);
  #ifdef DEBUG
    _debugString = "ID: " + String(id) + " " + errorStrings[_error] + " with a value of: " + String(status);
    Serial.println(_debugString);
  #endif
  if(getRelayStatus(id) == status) {
    #ifdef DEBUG
      Serial.println("Relay status set successfully.");
    #endif
  } else {
    #ifdef DEBUG
      Serial.println("Failed to set relay status.");
    #endif
    return false;
  }
  return true;
}

bool IECControl::setCurrWarningLimit(uint8_t id, float level)
{
    _writeCustCurrWarningLimit(id, level);
    if (getCurrWarningLimit(id) == level) {
        #ifdef DEBUG
            Serial.println("Custom current warning limit set successfully.");
        #endif
        return true;
    } else {
        #ifdef DEBUG
            Serial.println("Failed to set custom current warning limit.");
        #endif
        return false;
    }
}

void IECControl::setpowerDataUpdateCycleTime(uint16_t cycleTime) {
  powerDataUpdateCycleTime = cycleTime;
}

//==========================================================//
//========================= Getters ========================//
//==========================================================//

std::vector<uint8_t> IECControl::getFoundIECIDs() {
  return _foundIECIDs;
}

MeasurementData IECControl::getMeasurementData(uint8_t id) {
  MeasurementData measData = {0, 0, 0, 0, 0};
  measData.currentData = getCurrentData(id);
  measData.voltageData = getVoltageData(id);
  measData.powerData = getPowerData(id);
  measData.frequencyData = getFrequencyData(id);
  measData.powerFactorData = getPowerFactorData(id);
  return measData;
}

uint16_t IECControl::getIECID(uint8_t id)
{
    return _getIntDataFromInputRegister(id, ID_START);
}

uint16_t IECControl::getIECVersion(uint8_t id)
{
    return _getIntDataFromInputRegister(id, VERSION_START);
}

uint16_t IECControl::getIECRelayCount(uint8_t id)
{
    return _getIntDataFromInputRegister(id, RELAY_COUNT_START);
}

uint16_t IECControl::getIECCurrentLimit(uint8_t id)
{
    return _getIntDataFromInputRegister(id, CURRENT_LIMIT_START);
}

uint16_t IECControl::getIEC_IS_VOLTAGE_MEASURED(uint8_t id)
{
    return _getIntDataFromInputRegister(id, IS_CURRENT_MEASURED_START);
}

uint16_t IECControl::getIEC_IS_CURRENT_MEASURED(uint8_t id)
{
    return _getIntDataFromInputRegister(id, IS_CURRENT_MEASURED_START);
}


uint16_t IECControl::getIEC_IS_RMS_VOLTAGE_MEASURED(uint8_t id)
{
    return _getIntDataFromInputRegister(id, IS_RMS_VOLTAGE_MEASURED_START);
}
uint16_t IECControl::getIEC_IS_RMS_CURRENT_MEASURED(uint8_t id)
{
    return _getIntDataFromInputRegister(id, IS_RMS_CURRENT_MEASURED_START);
}
uint16_t IECControl::getIEC_IS_ACTIVE_POWER_MEASURED(uint8_t id)
{
    return _getIntDataFromInputRegister(id, IS_ACTIVE_POWER_MEASURED_START);
}
uint16_t IECControl::getIEC_IS_REACTIVE_POWER_MEASURED(uint8_t id)
{
    return _getIntDataFromInputRegister(id, IS_REACTIVE_POWER_MEASURED_START);
}
uint16_t IECControl::getIEC_IS_APPARENT_POWER_MEASURED(uint8_t id)
{
    return _getIntDataFromInputRegister(id, IS_APPARENT_POWER_MEASURED_START);
}
uint16_t IECControl::getIEC_IS_POWER_FACTOR_MEASURED(uint8_t id)
{
    return _getIntDataFromInputRegister(id, IS_POWER_FACTOR_MEASURED_START);
}
uint16_t IECControl::getIEC_IS_AC_FREQUENCY_MEASURED(uint8_t id)
{
    return _getIntDataFromInputRegister(id, IS_AC_FREQUENCY_MEASURED_START);
}




uint16_t IECControl::getIECRelayStatus(uint8_t id)
{
    return _getIntDataFromInputRegister(id, RELAY_STATE_START);
}

float IECControl::getCurrWarningLimit(uint8_t id)
{
    return _getFloatDataFromInputRegister(id, CUSTCURR_WARNING_LIMIT_START);
}

float IECControl::getCurrErrorLimit(uint8_t id)
{
    return _getFloatDataFromInputRegister(id, CUSTCURR_ERROR_LIMIT_START);
}

bool IECControl::getRelayStatus(uint8_t id) {
  return _iecModules[id].coils[RELAY_STATE_ADDR];
}

uint16_t IECControl::getIECStatus(uint8_t id) {
  uint16_t status = _getIntDataFromInputRegister(id, IEC_STATUS_START);
  return status;
}

float IECControl::getCurrentData(uint8_t id) {
  return _getFloatDataFromInputRegister(id, RMS_CURRENT_START);
}

float IECControl::getVoltageData(uint8_t id) {
  return _getFloatDataFromInputRegister(id, RMS_VOLTAGE_START);
}

float IECControl::getPowerData(uint8_t id) {
  return _getFloatDataFromInputRegister(id, APPARENT_POWER_START);
}

float IECControl::getFrequencyData(uint8_t id) {
  return _getFloatDataFromInputRegister(id, AC_FREQUENCY_START);
}

float IECControl::getPowerFactorData(uint8_t id) {
  return _getFloatDataFromInputRegister(id, POWER_FACTOR_START);
}

//==========================================================//
//==================== Private functions ===================//
//==========================================================//

//-----------------------Get data from local register----------------------------//

float IECControl::_getFloatDataFromInputRegister(uint8_t id, uint16_t startOfData) { //Read data from module's local input register
  uint32_t x = (_iecModules[id].inputRegisters[startOfData] << 16) + _iecModules[id].inputRegisters[startOfData + 1];
  return *(float*)&x;
}

void floatToIEEE754Registers(float value, uint16_t* regs) {
    uint32_t raw;
    std::memcpy(&raw, &value, sizeof(uint32_t));
    regs[0] = static_cast<uint16_t>((raw >> 16) & 0xFFFF);  // High
    regs[1] = static_cast<uint16_t>(raw & 0xFFFF);          // Low
}

uint16_t IECControl::_getIntDataFromInputRegister(uint8_t id, uint16_t startOfData) { //Read data from module's local input register
  return _iecModules[id].inputRegisters[startOfData];
}

std::vector<uint8_t> IECControl::_discoverIECs() {
  _foundIECIDs.clear();
  std::vector<uint8_t> slaveIDs;
  for (uint8_t i = 1; i <= 50; i++) {
    bool status = _isIECCommunicate(i); //TODO Random reading to get active modules, shall be implemented in the future as real status reading
    if (status) {
      #ifdef DEBUG
        Serial.print("Slave found with ID ");
        Serial.println(i);
      #endif
      slaveIDs.push_back(i);
    }
    else {
      #ifdef DEBUG
        Serial.print("No slave found with ID ");
        Serial.println(i);
      #endif
    }
  }
  _foundIECIDs = slaveIDs; // Store the found slave IDs in the class variable
  return slaveIDs;
}

bool IECControl::_isIECCommunicate(uint8_t id) { // Check if the given slave ID is communicating
  uint16_t buffer[1];
  ModbusRTUMasterError _error = _mbMaster.readInputRegisters(id, ID_START, buffer, 1);
    #ifdef DEBUG
      _debugString = "ID: " + String(id) + " " + errorStrings[_error];
      Serial.println(_debugString);
    #endif
  if (_error != MODBUS_RTU_MASTER_SUCCESS) {
    return false;
  }
  return true;
}

//--------------------Read data from IEC's register over modbus------------------------//

bool IECControl::_updateMeasurement() { // Update the measurement data for all discovered IEC modules
  for (auto& module : _iecModules) {
    _updateMeasurement(module.first);
  }
  return true;
}

bool IECControl::_updateMeasurement(uint8_t id) { // Update the measurement data for a specific IEC module
  _readIECFloatInputRegister(id, RMS_CURRENT_START);
  _readIECFloatInputRegister(id, RMS_VOLTAGE_START);
  _readIECFloatInputRegister(id, APPARENT_POWER_START);
  _readIECFloatInputRegister(id, AC_FREQUENCY_START);
  _readIECFloatInputRegister(id, POWER_FACTOR_START);
  return true;
}

void IECControl::_readIECCapabilities(uint8_t id) {
  _readID(id);
  _readIECVersion(id);
  _readIECRelayCount(id);
  _readIECCurrentLimit(id);
  _readIECMeasureVoltageCount(id);
  _readIECMeasureCurrentCount(id);
}

void IECControl::_readIECStatus(uint8_t id) { //TODO shall be implemented in the future as real status reading, return an IECStatus enum
  _readIECUINT16InputRegister(id, IEC_STATUS_START);
  _readRelayStatus(id);
}

void IECControl::_readRelayStatus(uint8_t id) {
    bool buffer[RELAY_STATE_LENGTH];
    ModbusRTUMasterError _error = _mbMaster.readCoils(id, RELAY_STATE_START, buffer, RELAY_STATE_LENGTH);
    if (_error != MODBUS_RTU_MASTER_SUCCESS)
    {
#ifdef DEBUG
        _debugString = "ID: " + String(id) + " " + errorStrings[_error];
        Serial.println(_debugString);
#endif
        _iecModules[id].inputRegisters[RELAY_STATE_START] = buffer[0];
    }
}

void IECControl::_readCurrentData(uint8_t id) {
  _readIECFloatInputRegister(id, RMS_CURRENT_START);
}

void IECControl::_readVoltageData(uint8_t id) {
  _readIECFloatInputRegister(id, RMS_VOLTAGE_START);
}

void IECControl::_readPowerData(uint8_t id) {
  _readIECFloatInputRegister(id, APPARENT_POWER_START);
}

void IECControl::_readFrequencyData(uint8_t id){
  _readIECFloatInputRegister(id, AC_FREQUENCY_START);
}

void IECControl::_readPowerFactorData(uint8_t id){
  _readIECFloatInputRegister(id, POWER_FACTOR_START);
}

void IECControl::_readIECFloatInputRegister(uint16_t id, uint16_t startOfData) { //Read float data from IEC module and stores it to the IEC's input register array
  uint16_t buffer[2];
  ModbusRTUMasterError _error = _mbMaster.readInputRegisters(id, startOfData, buffer, 2);

  if (_error != MODBUS_RTU_MASTER_SUCCESS) {
    #ifdef DEBUG
      _debugString = "ID: " + String(id) + " " + errorStrings[_error];
      Serial.println(_debugString);
    #endif
  }
  else {
    _iecModules[id].inputRegisters[startOfData] = buffer[0];
    _iecModules[id].inputRegisters[startOfData + 1] = buffer[1];
  }
}

void IECControl::_readIECUINT16InputRegister(uint16_t id, uint16_t startOfData) { //Read float data from IEC module and stores it to the IEC's input register array
  uint16_t buffer[1];
  ModbusRTUMasterError _error = _mbMaster.readInputRegisters(id, startOfData, buffer, 1);

  if (_error != MODBUS_RTU_MASTER_SUCCESS) {
    #ifdef DEBUG
      _debugString = "ID: " + String(id) + " " + errorStrings[_error];
      Serial.println(_debugString);
    #endif
  }
  else {
    _iecModules[id].inputRegisters[startOfData] = buffer[0];
  }
}

void IECControl::_readID(uint8_t id) { //Read the relay status from the given slave ID over
  _readIECUINT16InputRegister(id, ID_START);
}

void IECControl::_readIECVersion(uint8_t id) { //Read the relay status from the given slave ID over
  _readIECUINT16InputRegister(id, VERSION_START);
}

void IECControl::_readIECRelayCount(uint8_t id) { //Read the relay status from the given slave ID over
  _readIECUINT16InputRegister(id, RELAY_COUNT_START);
}

void IECControl::_readIECCurrentLimit(uint8_t id) { //Read the relay status from the given slave ID over
  _readIECUINT16InputRegister(id, CURRENT_LIMIT_START);
}

void IECControl::_readIECMeasureVoltageCount(uint8_t id) { //Read the relay status from the given slave ID over
  _readIECUINT16InputRegister(id, IS_VOLTAGE_MEASURED_START);
}

void IECControl::_readIECMeasureCurrentCount(uint8_t id) { //Read the relay status from the given slave ID over
  _readIECUINT16InputRegister(id, IS_CURRENT_MEASURED_START);
}

void IECControl::_readCustCurrWarningLimit(uint8_t id) { //Read the relay status from the given slave ID over
  _readIECFloatInputRegister(id, CUSTCURR_WARNING_LIMIT_START);
}

void IECControl::_readCustCurrErrorLimit(uint8_t id) { //Read the relay status from the given slave ID over
  _readIECFloatInputRegister(id, CUSTCURR_ERROR_LIMIT_START);
}

void IECControl::_writeCustCurrWarningLimit(uint8_t id, float value) { //Read the relay status from the given slave ID over
  uint16_t buffer[2];
  floatToIEEE754Registers(value, buffer);
  ModbusRTUMasterError _error = _mbMaster.writeMultipleHoldingRegisters(id, CUSTCURR_WARNING_LIMIT_START, buffer, CUSTCURR_WARNING_LIMIT_LENGTH);
  if (_error != MODBUS_RTU_MASTER_SUCCESS) {
    #ifdef DEBUG
      _debugString = "ID: " + String(id) + " " + errorStrings[_error];
      Serial.println(_debugString);
    #endif
    _readCustCurrWarningLimit(id);
  }
}

void IECControl::_writeCustCurrErrorLimit(uint8_t id, float value) { //Read the relay status from the given slave ID over
  uint16_t buffer[2];
  floatToIEEE754Registers(value, buffer);
  ModbusRTUMasterError _error = _mbMaster.writeMultipleHoldingRegisters(id, CUSTCURR_ERROR_LIMIT_START, buffer, CUSTCURR_ERROR_LIMIT_LENGTH);
  if (_error != MODBUS_RTU_MASTER_SUCCESS) {
    #ifdef DEBUG
      _debugString = "ID: " + String(id) + " " + errorStrings[_error];
      Serial.println(_debugString);
    #endif
    _readCustCurrErrorLimit(id);
  }
}