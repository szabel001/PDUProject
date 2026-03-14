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
  ensureNVSInt(NVSKeys::MEAS_CYCLE, 1);
  ensureNVSInt(NVSKeys::MEAS_OC, 32);
  _startMillis_powerRead = millis(); // Initialize the start time for power data update
}

void IECControl::IECSetup() {
  collectIECModuleInfos();
}

void IECControl::IECReadLoop() {
  processRelaySequence(); 
  unsigned long _currentMillis_powerRead = millis();
  if (_currentMillis_powerRead - _startMillis_powerRead >= (powerDataUpdateCycleTime * 1000)) {
    _updateMeasurement();
    _startMillis_powerRead = _currentMillis_powerRead;
  }
}

void IECControl::collectIECModuleInfos() {
  _iecModules.erase(_iecModules.begin(), _iecModules.end());
  std::vector<uint8_t> foundIDs = discoverIECs();
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
  // 1. Kiküldjük az írási parancsot a Modbuson
  ModbusRTUMasterError _error = _mbMaster.writeSingleCoil(id, 0, status);
  
  // 2. Ha a Modbus sikeresen nyugtázta, tudjuk, hogy átváltott a relé
  if (_error == MODBUS_RTU_MASTER_SUCCESS) {
    
    // Frissítjük a belső memóriákat, hogy a weboldal azonnal lássa a változást
    _iecModules[id].coils[RELAY_STATE_ADDR] = status;
    
    #ifdef DEBUG
      _debugString = "ID: " + String(id) + " success with a value of: " + String(status);
      Serial.println(_debugString);
    #endif
    
    return true; // Sikeres kapcsolás, az állapotgép léphet a következőre!
  }
  
  #ifdef DEBUG
    _debugString = "ID: " + String(id) + " error: " + errorStrings[_error];
    Serial.println(_debugString);
  #endif
  
  return false; // Hiba esetén a state machine vár 200ms-ot és újra megpróbálja
}

bool IECControl::setIECAVGNum(uint8_t id, float value) {
  _writeIECAVGNum(id, value);
  return getIECAVGNum(id) == value;
}

bool IECControl::setCustCurrWarningLimit(uint8_t id, float level){
  _writeCustCurrWarningLimit(id, level);
  return getCustCurrWarningLimit(id) == level;
}

bool IECControl::setCustCurrErrorLimit(uint8_t id, float level){
  _writeCustCurrErrorLimit(id, level);
  return getCustCurrErrorLimit(id) == level;
}

void IECControl::setOverCurrentTreshold(uint8_t id, float level) {
    writeIntToNVS(NVSKeys::MEAS_OC, level);
    for (auto& module : _iecModules) _writeOCTreshold(module.first, level);
}

uint16_t IECControl::getOverCurrentTreshold() {
    return readIntFromNVS(NVSKeys::MEAS_OC, 0);
}

uint16_t IECControl::getPowerDataUpdateCycleTime() {
  return readIntFromNVS(NVSKeys::MEAS_CYCLE, 1);
}

void IECControl::setpowerDataUpdateCycleTime(uint16_t cycleTime) {
  powerDataUpdateCycleTime = cycleTime;
  writeIntToNVS(NVSKeys::MEAS_CYCLE, cycleTime);
}

void IECControl::setIECSwitchingDelay(uint16_t delay) {
  _IECSwitchingDelay = delay;
  writeIntToNVS(NVSKeys::MEAS_DELAY, delay);
}

void IECControl::setAllIecRelayStatus(bool status) {
    if (_foundIECIDs.empty()) return;
    
    _relaySequenceActive = true;
    _relaySequenceTargetState = status;
    _relaySequenceIndex = 0;
    _relaySequenceDelayMs = readIntFromNVS(NVSKeys::MEAS_DELAY, 0) * 1000;
    _relaySequenceLastTime = 0;
    _lastRetryTime = 0;
}

void IECControl::processRelaySequence() {
    if (!_relaySequenceActive) return;
    if (_foundIECIDs.empty() || _relaySequenceIndex >= _foundIECIDs.size()) {
        _relaySequenceActive = false;
        return;
    }

    unsigned long currentMillis = millis();

    if (_relaySequenceTargetState == true && _relaySequenceLastTime != 0) {
        if (currentMillis - _relaySequenceLastTime < _relaySequenceDelayMs) {
            return;
        }
    }
    if (_lastRetryTime != 0 && (currentMillis - _lastRetryTime < 200)) {
        return;
    }

    uint8_t currentModuleId = _foundIECIDs[_relaySequenceIndex];
    bool success = setRelayStatus(currentModuleId, _relaySequenceTargetState);

    if (success) {
        _relaySequenceLastTime = currentMillis; 
        _lastRetryTime = 0;
        _relaySequenceIndex++;
        
        if (_relaySequenceIndex >= _foundIECIDs.size()) {
            _relaySequenceActive = false;
        }
    } else {
        _lastRetryTime = currentMillis; 
    }
}

void IECControl::setAllIecRelaysOn() {
    setAllIecRelayStatus(true);
}

void IECControl::setAllIecRelaysOff() {
    setAllIecRelayStatus(false);
}

uint16_t IECControl::getIECSwitchingDelay(){
  return readIntFromNVS(NVSKeys::MEAS_DELAY, 0);
}

//==========================================================//
//========================= Getters ========================//
//==========================================================//


float IECControl::getSumIECEnergyData() {
  float sum = 0;
  for (uint8_t id : _foundIECIDs) {
    sum += getEnergyKWh(id); // Vagy kVAh a korábbi számítások alapján
  }
  return sum;
}

float IECControl::getSumIECCurrentData() {
  float sum = 0;
  for (uint8_t id : _foundIECIDs) {
    sum += getRMSCurrentData(id);
  }
  return sum;
}

float IECControl::getAvgIECVoltageData() {
  float sum = 0;
  for (uint8_t id : _foundIECIDs) {
    sum += getRMSVoltageData(id);
  }
  return sum;
}

float IECControl::getSumIECPowerData() {
  float sum = 0;
  for (uint8_t id : _foundIECIDs) {
    sum += getApparentPowerData(id);
  }
  return sum;
}

float IECControl::getAvgIECFrequencyData() {
  if (_foundIECIDs.empty()) return 0.0f;
  float sum = 0;
  for (uint8_t id : _foundIECIDs) {
    sum += getFrequencyData(id);
  }
  return sum / _foundIECIDs.size();
}

std::vector<uint8_t> IECControl::getFoundIECIDs() {
  return _foundIECIDs;
}

MeasurementData IECControl::getMeasurementData(uint8_t id) {
  MeasurementData measData = {0, 0, 0, 0, 0, 0, 0};
  measData.rmsCurrent = getRMSCurrentData(id);
  measData.rmsVoltage = getRMSVoltageData(id);
  measData.activePower = getActivePowerData(id);
  measData.reactivePower = getReactivePowerData(id);
  measData.apparentPower = getApparentPowerData(id);
  measData.powerFactor = getPowerFactorData(id);
  measData.acFrequency = getFrequencyData(id);
  return measData;
}

configData IECControl::getConfigData(uint8_t id) {
  configData confData = {0, 0, 0, 0, 0, 0};
  confData.currentLimit = getIECCurrentLimit(id);
  confData.isRMSCurrentMeasured = getIEC_IS_RMS_CURRENT_MEASURED(id);
  confData.isRMSVoltageMeasured = getIEC_IS_RMS_VOLTAGE_MEASURED(id);
  confData.isActivePowerMeasured = getIEC_IS_ACTIVE_POWER_MEASURED(id);
  confData.isReactivePowerMeasured = getIEC_IS_REACTIVE_POWER_MEASURED(id);
  confData.isApparentPowerMeasured = getIEC_IS_APPARENT_POWER_MEASURED(id);
  confData.isPowerFactorMeasured = getIEC_IS_POWER_FACTOR_MEASURED(id);
  confData.isACFrequencyMeasured = getIEC_IS_AC_FREQUENCY_MEASURED(id);
  return confData;
}

uint16_t IECControl::getIECID(uint8_t id)
{
    return _getIntDataFromInputRegister(id, ID_ADDR);
}

uint16_t IECControl::getIECVersion(uint8_t id)
{
    return _getIntDataFromInputRegister(id, VERSION_ADDR);
}

uint16_t IECControl::getIECRelayCount(uint8_t id)
{
    return _getIntDataFromInputRegister(id, RELAY_COUNT_ADDR);
}

uint16_t IECControl::getIECCurrentLimit(uint8_t id)
{
    return _getIntDataFromInputRegister(id, CURRENT_LIMIT_ADDR);
}

uint16_t IECControl::getIEC_IS_RMS_VOLTAGE_MEASURED(uint8_t id)
{
    return _getIntDataFromInputRegister(id, IS_RMS_VOLTAGE_MEASURED_ADDR);
}
uint16_t IECControl::getIEC_IS_RMS_CURRENT_MEASURED(uint8_t id)
{
    return _getIntDataFromInputRegister(id, IS_RMS_CURRENT_MEASURED_ADDR);
}
uint16_t IECControl::getIEC_IS_ACTIVE_POWER_MEASURED(uint8_t id)
{
    return _getIntDataFromInputRegister(id, IS_ACTIVE_POWER_MEASURED_ADDR);
}
uint16_t IECControl::getIEC_IS_REACTIVE_POWER_MEASURED(uint8_t id)
{
    return _getIntDataFromInputRegister(id, IS_REACTIVE_POWER_MEASURED_ADDR);
}
uint16_t IECControl::getIEC_IS_APPARENT_POWER_MEASURED(uint8_t id)
{
    return _getIntDataFromInputRegister(id, IS_APPARENT_POWER_MEASURED_ADDR);
}
uint16_t IECControl::getIEC_IS_POWER_FACTOR_MEASURED(uint8_t id)
{
    return _getIntDataFromInputRegister(id, IS_POWER_FACTOR_MEASURED_ADDR);
}
uint16_t IECControl::getIEC_IS_AC_FREQUENCY_MEASURED(uint8_t id)
{
    return _getIntDataFromInputRegister(id, IS_AC_FREQUENCY_MEASURED_ADDR);
}

uint16_t IECControl::getIEC_AVAILABLE_LEDS(uint8_t id)
{
    return _getIntDataFromInputRegister(id, AVAILABLE_LEDS_ADDR);
}

uint16_t IECControl::getIECRelayStatus(uint8_t id)
{
  return _iecModules[id].coils[RELAY_STATE_ADDR];
}

float IECControl::getCustCurrWarningLimit(uint8_t id)
{
    return _getFloatDataFromHoldingRegister(id, CUSTCURR_WARNING_LIMIT_ADDR);
}

float IECControl::getCustCurrErrorLimit(uint8_t id)
{
    return _getFloatDataFromHoldingRegister(id, CUSTCURR_ERROR_LIMIT_ADDR);
}

float IECControl::getIECAVGNum(uint8_t id)
{
    return _getFloatDataFromHoldingRegister(id, MEAS_AVG_NUM_ADDR);
}

bool IECControl::getRelayStatus(uint8_t id) {
  return _iecModules[id].coils[RELAY_STATE_ADDR];
}

IECStatus IECControl::getIECStatus(uint8_t id) {
  uint16_t status = _getIntDataFromInputRegister(id, IEC_STATUS_ADDR);
  return static_cast<IECStatus>(status);
}

float IECControl::getRMSCurrentData(uint8_t id) {
  return _getFloatDataFromInputRegister(id, RMS_CURRENT_ADDR);
}

float IECControl::getRMSVoltageData(uint8_t id) {
  return _getFloatDataFromInputRegister(id, RMS_VOLTAGE_ADDR);
}

float IECControl::getActivePowerData(uint8_t id) {
  return _getFloatDataFromInputRegister(id, ACTIVE_POWER_ADDR);
}

float IECControl::getReactivePowerData(uint8_t id) {
  return _getFloatDataFromInputRegister(id, REACTIVE_POWER_ADDR);
}

float IECControl::getApparentPowerData(uint8_t id) {
  return _getFloatDataFromInputRegister(id, APPARENT_POWER_ADDR);
}

float IECControl::getFrequencyData(uint8_t id) {
  return _getFloatDataFromInputRegister(id, AC_FREQUENCY_ADDR);
}

float IECControl::getPowerFactorData(uint8_t id) {
  return _getFloatDataFromInputRegister(id, POWER_FACTOR_ADDR);
}

//==========================================================//
//==================== Private functions ===================//
//==========================================================//

//-----------------------Get data from local register----------------------------//

void floatToIEEE754Registers(float value, uint16_t* regs) {
    uint32_t raw;
    std::memcpy(&raw, &value, sizeof(uint32_t));
    regs[0] = static_cast<uint16_t>((raw >> 16) & 0xFFFF);  // High
    regs[1] = static_cast<uint16_t>(raw & 0xFFFF);          // Low
}

float IEEE754RegistersToFloat(const uint16_t* regs) {
    uint32_t raw = (static_cast<uint32_t>(regs[0]) << 16) | static_cast<uint32_t>(regs[1]);
    float value;
    std::memcpy(&value, &raw, sizeof(float));
    return value;
}

float IECControl::_getFloatDataFromInputRegister(uint8_t id, uint16_t startOfData) { //Read data from module's local input register
  uint32_t x = (_iecModules[id].inputRegisters[startOfData] << 16) + _iecModules[id].inputRegisters[startOfData + 1];
  return *(float*)&x;
}

float IECControl::_getFloatDataFromHoldingRegister(uint8_t id, uint16_t startOfData) { //Read data from module's local input register
  uint32_t x = (_iecModules[id].holdingRegisters[startOfData] << 16) + _iecModules[id].holdingRegisters[startOfData + 1];
  return *(float*)&x;
}

uint16_t IECControl::_getIntDataFromInputRegister(uint8_t id, uint16_t startOfData) { //Read data from module's local input register
  return _iecModules[id].inputRegisters[startOfData];
}

uint16_t IECControl::_getIntDataFromHoldingRegister(uint8_t id, uint16_t startOfData) { //Read data from module's local input register
  return _iecModules[id].inputRegisters[startOfData];
}

std::vector<uint8_t> IECControl::discoverIECs() {
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
  ModbusRTUMasterError _error = _mbMaster.readInputRegisters(id, ID_ADDR, buffer, 1);
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
  _read_RMS_CURRENT(id);
  _read_RMS_VOLTAGE(id);
  _read_ACTIVE_POWER(id);
  _read_REACTIVE_POWER(id);
  _read_APPARENT_POWER(id);
  _read_POWER_FACTOR(id);
  _read_AC_FREQUENCY(id);
  _readIECStatus(id);

   // ENERGIA SZÁMÍTÁSA:
    unsigned long now = millis();
    if (_iecModules[id].lastEnergyUpdate > 0) {
        float hoursPassed = (now - _iecModules[id].lastEnergyUpdate) / 3600000.0;
        // W * h = Wh (Wattóra)
        float power = getApparentPowerData(id);
        _iecModules[id].energyConsumptionWh += power * hoursPassed;
    }
    _iecModules[id].lastEnergyUpdate = now;

    return true;
}

float IECControl::getEnergyKWh(uint8_t id) {
    return _iecModules[id].energyConsumptionWh / 1000.0; // Wh -> kWh
}

void IECControl::_readIECCapabilities(uint8_t id) {
  _readID(id);
  _readIECVersion(id);
  _readIECRelayCount(id);
  _readIECCurrentLimit(id);
  _readIECAvailableLEDs(id);
  _readIEC_IS_RMS_VOLTAGE_MEASURED(id);
  _readIEC_IS_RMS_CURRENT_MEASURED(id);
  _readIEC_IS_ACTIVE_POWER_MEASURED(id);
  _readIEC_IS_REACTIVE_POWER_MEASURED(id);
  _readIEC_IS_APPARENT_POWER_MEASURED(id);
  _readIEC_IS_POWER_FACTOR_MEASURED(id);
  _readIEC_IS_AC_FREQUENCY_MEASURED(id);
}

void IECControl::_readIECStatus(uint8_t id) {
  _readIECUINT16InputRegister(id, IEC_STATUS_ADDR);
}

void IECControl::_readRelayStatus(uint8_t id) {
    bool buffer[1];
    ModbusRTUMasterError _error = _mbMaster.readCoils(id, RELAY_STATE_ADDR, buffer, 1);
    if (_error != MODBUS_RTU_MASTER_SUCCESS)
    {
#ifdef DEBUG
        _debugString = "ID: " + String(id) + " " + errorStrings[_error];
        Serial.println(_debugString);
#endif
    }
    else _iecModules[id].coils[RELAY_STATE_ADDR] = buffer[0];
}

void IECControl::_read_RMS_VOLTAGE(uint8_t id) {
  _readIECFloatInputRegister(id, RMS_VOLTAGE_ADDR);
}

void IECControl::_read_RMS_CURRENT(uint8_t id) {
  _readIECFloatInputRegister(id, RMS_CURRENT_ADDR);
}

void IECControl::_read_ACTIVE_POWER(uint8_t id){
  _readIECFloatInputRegister(id, ACTIVE_POWER_ADDR);
}

void IECControl::_read_REACTIVE_POWER(uint8_t id){
  _readIECFloatInputRegister(id, REACTIVE_POWER_ADDR);
}

void IECControl::_read_APPARENT_POWER(uint8_t id) {
  _readIECFloatInputRegister(id, APPARENT_POWER_ADDR);
}

void IECControl::_read_POWER_FACTOR(uint8_t id){
  _readIECFloatInputRegister(id, POWER_FACTOR_ADDR);
}

void IECControl::_read_AC_FREQUENCY(uint8_t id){
  _readIECFloatInputRegister(id, AC_FREQUENCY_ADDR);
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

void IECControl::_readIECFloatHoldingRegister(uint16_t id, uint16_t startOfData) { //Read float data from IEC module and stores it to the IEC's input register array
  uint16_t buffer[2];
  ModbusRTUMasterError _error = _mbMaster.readHoldingRegisters(id, startOfData, buffer, 2);

  if (_error != MODBUS_RTU_MASTER_SUCCESS) {
    #ifdef DEBUG
      _debugString = "ID: " + String(id) + " " + errorStrings[_error];
      Serial.println(_debugString);
    #endif
  }
  else {
    _iecModules[id].holdingRegisters[startOfData] = buffer[0];
    _iecModules[id].holdingRegisters[startOfData + 1] = buffer[1];
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

void IECControl::_readIECUINT16HoldingRegister(uint16_t id, uint16_t startOfData) { //Read float data from IEC module and stores it to the IEC's input register array
  uint16_t buffer[1];
  ModbusRTUMasterError _error = _mbMaster.readHoldingRegisters(id, startOfData, buffer, 1);

  if (_error != MODBUS_RTU_MASTER_SUCCESS) {
    #ifdef DEBUG
      _debugString = "Reading error UINT16 Holding Register from ID: " + String(id) + " " + errorStrings[_error];
      Serial.println(_debugString);
    #endif
  }
  else {
    _iecModules[id].holdingRegisters[startOfData] = buffer[0];
  }
}

void IECControl::_readIECCoil(uint8_t id, uint16_t startOfData) { //Read coil data from IEC module and stores it to the IEC's input register array
  bool buffer[1];
  ModbusRTUMasterError _error = _mbMaster.readCoils(id, startOfData, buffer, 1);

  if (_error != MODBUS_RTU_MASTER_SUCCESS) {
    #ifdef DEBUG
      _debugString = "Reading coil error from ID: " + String(id) + " " + errorStrings[_error];
      Serial.println(_debugString);
    #endif
  }
  else {
    _iecModules[id].coils[startOfData] = buffer[0];
  }
}

void IECControl::_readID(uint8_t id) { //Read the relay status from the given slave ID over
  _readIECUINT16InputRegister(id, ID_ADDR);
}

void IECControl::_readIECVersion(uint8_t id) { //Read the relay status from the given slave ID over
  _readIECUINT16InputRegister(id, VERSION_ADDR);
}

void IECControl::_readIECRelayCount(uint8_t id) { //Read the relay status from the given slave ID over
  _readIECUINT16InputRegister(id, RELAY_COUNT_ADDR);
}

void IECControl::_readIECCurrentLimit(uint8_t id) { //Read the relay status from the given slave ID over
  _readIECUINT16InputRegister(id, CURRENT_LIMIT_ADDR);
}

void IECControl::_readIECAvailableLEDs(uint8_t id) { //Read the relay status from the given slave ID over
  _readIECUINT16InputRegister(id, AVAILABLE_LEDS_ADDR);
}

void IECControl::_readIEC_IS_RMS_VOLTAGE_MEASURED(uint8_t id) { //Read the relay status from the given slave ID over
  _readIECCoil(id, IS_RMS_VOLTAGE_MEASURED_ADDR);
}

void IECControl::_readIEC_IS_RMS_CURRENT_MEASURED(uint8_t id) { //Read the relay status from the given slave ID over
  _readIECCoil(id, IS_RMS_CURRENT_MEASURED_ADDR);
}

void IECControl::_readIEC_IS_ACTIVE_POWER_MEASURED(uint8_t id) { //Read the relay status from the given slave ID over
  _readIECCoil(id, IS_ACTIVE_POWER_MEASURED_ADDR);
}

void IECControl::_readIEC_IS_REACTIVE_POWER_MEASURED(uint8_t id) { //Read the relay status from the given slave ID over
  _readIECCoil(id, IS_REACTIVE_POWER_MEASURED_ADDR);
}

void IECControl::_readIEC_IS_APPARENT_POWER_MEASURED(uint8_t id) { //Read the relay status from the given slave ID over
  _readIECCoil(id, IS_APPARENT_POWER_MEASURED_ADDR);
}

void IECControl::_readIEC_IS_POWER_FACTOR_MEASURED(uint8_t id) { //Read the relay status from the given slave ID over
  _readIECCoil(id, IS_POWER_FACTOR_MEASURED_ADDR);
}

void IECControl::_readIEC_IS_AC_FREQUENCY_MEASURED(uint8_t id) { //Read the relay status from the given slave ID over
  _readIECCoil(id, IS_AC_FREQUENCY_MEASURED_ADDR);
}

void IECControl::_readOCTreshold(uint8_t id) { //Read the relay status from the given slave ID over
  _readIECFloatHoldingRegister(id, OC_TRESHOLD_ADDR);
}

void IECControl::_readCustCurrWarningLimit(uint8_t id) { //Read the relay status from the given slave ID over
  _readIECFloatHoldingRegister(id, CUSTCURR_WARNING_LIMIT_ADDR);
}

void IECControl::_readCustCurrErrorLimit(uint8_t id) { //Read the relay status from the given slave ID over
  _readIECFloatHoldingRegister(id, CUSTCURR_ERROR_LIMIT_ADDR);
}

void IECControl::_readIECAVGNum(uint8_t id) { //Read the relay status from the given slave ID over
  _readIECFloatHoldingRegister(id, MEAS_AVG_NUM_ADDR);
}


void IECControl::_writeCustCurrWarningLimit(uint8_t id, float value) { //Read the relay status from the given slave ID over
  uint16_t buffer[2];
  floatToIEEE754Registers(value, buffer);
  ModbusRTUMasterError _error = _mbMaster.writeMultipleHoldingRegisters(id, CUSTCURR_WARNING_LIMIT_ADDR, buffer, 2);
  if (_error == MODBUS_RTU_MASTER_SUCCESS) {
    _readCustCurrWarningLimit(id);
  }
}

void IECControl::_writeCustCurrErrorLimit(uint8_t id, float value) { //Read the relay status from the given slave ID over
  uint16_t buffer[2];
  floatToIEEE754Registers(value, buffer);
  ModbusRTUMasterError _error = _mbMaster.writeMultipleHoldingRegisters(id, CUSTCURR_ERROR_LIMIT_ADDR, buffer, 2);
  if (_error == MODBUS_RTU_MASTER_SUCCESS) {
    _readCustCurrErrorLimit(id);
  }
}

void IECControl::_writeOCTreshold(uint8_t id, float value) { //Read the relay status from the given slave ID over
  uint16_t buffer[2];
  floatToIEEE754Registers(value, buffer);
  ModbusRTUMasterError _error = _mbMaster.writeMultipleHoldingRegisters(id, OC_TRESHOLD_ADDR, buffer, 2);
  if (_error == MODBUS_RTU_MASTER_SUCCESS) {
    _readOCTreshold(id);
  }
}

void IECControl::_writeIECAVGNum(uint8_t id, float value) { //Read the relay status from the given slave ID over
  uint16_t buffer[2];
  floatToIEEE754Registers(value, buffer);
  ModbusRTUMasterError _error = _mbMaster.writeMultipleHoldingRegisters(id, MEAS_AVG_NUM_ADDR, buffer, 2);
  if (_error == MODBUS_RTU_MASTER_SUCCESS) {
    _readIECAVGNum(id);
  }
}