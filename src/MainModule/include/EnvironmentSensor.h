#ifndef EnvironmentSensor_h
#define EnvironmentSensor_h

#include <cstdint>
#include <Adafruit_AHTX0.h>
#include "variables.h"

#pragma once

struct EnvironmentSensorData {
    float temperature;
    float humidity;
};

class EnvironmentSensor {
  public:
    EnvironmentSensor();
    void setupEnvironmentSensor();
    void setTemperatureScale(bool isFahrenheit);
    bool isFahrenheit();
    EnvironmentSensorData getData();
    void setSamplingTime(uint16_t cycleTime);

  private:
    bool isfahrenheit = false;
    Adafruit_AHTX0 aht;
    sensors_event_t humidity, temp;
};
#endif