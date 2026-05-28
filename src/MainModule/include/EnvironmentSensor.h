#ifndef EnvironmentSensor_h
#define EnvironmentSensor_h

#include <cstdint>
#include <Adafruit_AHTX0.h>
#include "variables.h"

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

  private:
    bool _temSensorInitialized = false;
    bool isfahrenheit = false;
    Adafruit_AHTX0 aht;
    sensors_event_t humidity, temp;
};
#endif