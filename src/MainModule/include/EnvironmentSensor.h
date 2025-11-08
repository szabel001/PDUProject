#ifndef EnvironmentSensor_h
#define EnvironmentSensor_h

#include <cstdint>
#include <Adafruit_AHTX0.h>

#pragma once

struct EnvironmentSensorData {
    float temperature;
    float humidity;
};

class EnvironmentSensor {
  public:
    void setupEnvironmentSensor();
    EnvironmentSensorData getData();
    void setSamplingTime(uint16_t cycleTime);
private:
};
#endif