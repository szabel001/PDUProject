#include "PDUModuleInfo.h"

uint64_t PDUModuleInfo::getID() {
  return _id;
}

void PDUModuleInfo::setID(uint16_t id) {
  _id = id;
}

String PDUModuleInfo::getHWVersion() {
  return _hwversion;
}

void PDUModuleInfo::setHWVersion(String hwversion) {
  _hwversion = hwversion;
}

String PDUModuleInfo::getSWVersion() {
  return _swVersion;
}

void PDUModuleInfo::setSWVersion(String swversion) {
  _swVersion = swversion;
}

float PDUModuleInfo::getCurrentData() {
  return _currentData;
}

void PDUModuleInfo::setCurrentData(float currentData) {
  _currentData = currentData;
}

float PDUModuleInfo::getVoltageData() {
  return _voltageData;
}

void PDUModuleInfo::setVoltageData(float voltageData) {
  _voltageData = voltageData;
}

float PDUModuleInfo::getPowerData() {
  return _powerData;
}

void PDUModuleInfo::setPowerData(float powerData) {
  _powerData = powerData;
}

float PDUModuleInfo::getEnergyData() {
  return _energyData;
}

void PDUModuleInfo::setEnergyData(float energyData) {
  _energyData = energyData;
}

uint16_t PDUModuleInfo::getOvercurrThreshold() {
  return _overcurrThreshold;
}

void PDUModuleInfo::setOvercurrThreshold(uint16_t overcurrThreshold) {
  _overcurrThreshold = overcurrThreshold;
  writeIntToNVS(NVSKeys::MEAS_OC, overcurrThreshold);
}