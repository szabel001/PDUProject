#include "EnvironmentSensor.h"

Adafruit_AHTX0 aht;
sensors_event_t humidity, temp;

void EnvironmentSensor::setupEnvironmentSensor() {
  if (! aht.begin()) Serial.println("Could not find AHT? Check wiring");
  else Serial.println("AHT10 or AHT20 found");
}

EnvironmentSensorData EnvironmentSensor::getData() {
  EnvironmentSensorData data = {NAN, NAN};
  aht.getEvent(&humidity, &temp);
  data.temperature = temp.temperature;
  data.humidity = humidity.relative_humidity;
  return data;
}

void EnvironmentSensor::setSamplingTime(uint16_t data) {
  return;
}