#include "IECData.h"


enum IECStatus{
	STANDBY,
	NORMAL,
	FAULT,
};

float adcToCurrent(int adc)
{
    const int adc0 = 2150;

    // Negatív áram tükrözése
    if (adc < adc0) {
        int mirrored = 2 * adc0 - adc;
        return -adcToCurrent(mirrored);
    }

    float x = adc - adc0;

    // Másodfokú polinom
    float current =
        -2.17167128e-7f * x * x +
         1.20380534e-2f * x +
         3.33630450e-3f;

    // Alsó korlát
    if (current < 0.0f)
        current = 0.0f;

    return current;
}

void readCurrentData(ADC_HandleTypeDef* hadc)
{
    uint32_t sum = 0;
    uint16_t samples = Holding_Registers_Database[MEAS_AVG_NUM_ADDR];
    if(samples == 0)
        samples = 1;   // védelem 0 osztás ellen
    for(uint16_t i = 0; i < samples; i++)
    {
        HAL_ADC_Start(hadc);
        HAL_ADC_PollForConversion(hadc, 10);
        uint16_t ADCRaw = HAL_ADC_GetValue(hadc);
        HAL_ADC_Stop(hadc);

        sum += ADCRaw;
    }
    float avgRaw = (float)sum / samples;
    actualCurrent = adcToCurrent(avgRaw);
    addFloatToRegister(Input_Registers_Database, RMS_CURRENT_ADDR, actualCurrent);
}

void readVoltageData(){
	float voltageVal = 230.0f;
	addFloatToRegister(Input_Registers_Database, RMS_VOLTAGE_ADDR, voltageVal);
}

void readPowerData(ADC_HandleTypeDef* hadc){
	readCurrentData(hadc);
	readVoltageData();
	uint32_t currHex = (Input_Registers_Database[RMS_CURRENT_ADDR] << 16) + Input_Registers_Database[RMS_CURRENT_ADDR+1];
	uint32_t voltHex = (Input_Registers_Database[RMS_VOLTAGE_ADDR] << 16) + Input_Registers_Database[RMS_VOLTAGE_ADDR+1];
	float powerData = convertIEEE754ToFloat(currHex) * convertIEEE754ToFloat(voltHex);
	addFloatToRegister(Input_Registers_Database, APPARENT_POWER_ADDR, powerData);
}

uint8_t setRelayStatus(uint8_t prevRelayState){
	if(prevRelayState != Coils_Database[RELAY_STATE_ADDR]) {
		HAL_GPIO_WritePin(RELAY_CTRL_GPIO_Port, RELAY_CTRL_Pin, Coils_Database[RELAY_STATE_ADDR]);
		setRelayStatusLed(Coils_Database[RELAY_STATE_ADDR]);
		return Coils_Database[RELAY_STATE_ADDR];
	}
	return prevRelayState;
}

void setStatus(uint8_t status){
	Input_Registers_Database[IEC_STATUS_ADDR] = status;
}

void setRelayStatusLed(uint8_t status){
	HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, status);
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
