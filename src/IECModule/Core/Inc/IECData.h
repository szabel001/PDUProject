#include "modbusSlave.h"
#include "IECRegisterMap.h"
#include "main.h"

void readCurrentData(ADC_HandleTypeDef* hadc);
void readPowerData(ADC_HandleTypeDef* hadc);
void readVoltageData();
void addFloatToRegister(uint16_t *reg, uint8_t dataStart, float addedFloat);
float convertIEEE754ToFloat(uint32_t hex);
