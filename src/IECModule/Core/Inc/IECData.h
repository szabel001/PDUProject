#include "modbusSlave.h"
#include "IECRegisterMap.h"
#include "main.h"

#define NUM_HOLDING_REGISTERS 256
#define NUM_INPUT_REGISTERS 256
#define NUM_COILS 256
#define NUM_DISCRETEINPUTS 256

void readCurrentData(ADC_HandleTypeDef* hadc);
void readPowerData(ADC_HandleTypeDef* hadc);
void readVoltageData();
uint8_t setRelayStatus(uint8_t prevRelayState);
void setRelayStatusLed(uint8_t status);
void setCustCurrWarningLimit();
void setCustCurrErrorLimit();
void addFloatToRegister(uint16_t *reg, uint8_t dataStart, float addedFloat);
float convertIEEE754ToFloat(uint32_t hex);

extern float actualCurrent;
