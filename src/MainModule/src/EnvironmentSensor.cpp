#include "EnvironmentSensor.h"

EnvironmentSensor::EnvironmentSensor() {
}

void EnvironmentSensor::setupEnvironmentSensor() {
  if (! aht.begin()) Serial.println("Could not find AHT? Check wiring");
  else Serial.println("AHT10 or AHT20 found");
  setTemperatureScale(0); // default to Celsius
}

void EnvironmentSensor::setTemperatureScale(bool setfahrenheit) {
  writeStringToNVS(NVSKeys::MEAS_TEMP, setfahrenheit == true ? "F" : "C");
}

bool EnvironmentSensor::isFahrenheit() {
  return readStringFromNVS(NVSKeys::MEAS_TEMP, "C") == "F";
}

EnvironmentSensorData EnvironmentSensor::getData() {
  EnvironmentSensorData data = {NAN, NAN};
  aht.getEvent(&humidity, &temp);
  data.temperature = temp.temperature;
  data.humidity = humidity.relative_humidity;
  if (isFahrenheit()) {
    data.temperature = data.temperature * 9.0 / 5.0 + 32.0;
  }
  return data;
}

void EnvironmentSensor::setSamplingTime(uint16_t data) {
  return;
}