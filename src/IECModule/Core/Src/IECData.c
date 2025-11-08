#include "IECData.h"

void readCurrentData(ADC_HandleTypeDef* hadc){
  HAL_ADC_Start(hadc); // Poll ADC1 Perihperal & TimeOut = 1mSec
  HAL_ADC_PollForConversion(hadc, 1); // Read The ADC Conversion Result & Map It To PWM DutyCycle
  uint16_t ADCRaw = HAL_ADC_GetValue(hadc);
  float adcVal = ((float)(ADCRaw)/4096.0f)*3.3f;
  addFloatToRegister(Input_Registers_Database, CURRENT_DATA_START, adcVal);
}

void readVoltageData(){
	float voltageVal = 230.0f;
	addFloatToRegister(Input_Registers_Database, VOLTAGE_DATA_START, voltageVal);
}

void readPowerData(ADC_HandleTypeDef* hadc){
	readCurrentData(hadc);
	readVoltageData();
	uint32_t currHex = (Input_Registers_Database[CURRENT_DATA_START] << 16) + Input_Registers_Database[CURRENT_DATA_START+1];
	uint32_t voltHex = (Input_Registers_Database[VOLTAGE_DATA_START] << 16) + Input_Registers_Database[VOLTAGE_DATA_START+1];
	float powerData = convertIEEE754ToFloat(currHex) * convertIEEE754ToFloat(voltHex);
	addFloatToRegister(Input_Registers_Database, POWER_DATA_START, powerData);
}

void addFloatToRegister(uint16_t *reg, uint8_t dataStart, float addedFloat){
	//reg[dataStart + 0] = ((uint16_t*)&addedFloat)[0]; //TODO this would be better
	//reg[dataStart + 1] = ((uint16_t*)&addedFloat)[1];
	union
	{
	    float f;
        uint16_t u16[2];
	} float_union;

	float_union.f = addedFloat;

	//Big Endian
	reg[dataStart + 0] = float_union.u16[1];
	reg[dataStart + 1] = float_union.u16[0];
}

float convertIEEE754ToFloat(uint32_t hex) {
    union {
        uint32_t i;
        float f;
    } u;
    u.i = hex;
    return u.f;
}
