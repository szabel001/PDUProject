#include "IECData.h"

enum IECStatus {
 noError = 0,
 currentWarning = 1,
 currentError = 2,
 currentOverTreshold = 3,
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

    // Float konvertálása a 2 regiszterből az átlagoláshoz
    uint32_t avgHex = (Holding_Registers_Database[MEAS_AVG_NUM_ADDR] << 16) | Holding_Registers_Database[MEAS_AVG_NUM_ADDR+1];
    float avgFloat = convertIEEE754ToFloat(avgHex);
    uint16_t samples = (uint16_t)avgFloat;

    // Védelem nulla osztás ellen, vagy ha még nem jött adat az ESP-től
    if(samples == 0 || samples > 1000) samples = 10;

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

    // Limitek Float-ként való kiolvasása a regiszterekből
    float oc_thresh = convertIEEE754ToFloat((Holding_Registers_Database[OC_TRESHOLD_ADDR] << 16) | Holding_Registers_Database[OC_TRESHOLD_ADDR+1]);
    float err_lim   = convertIEEE754ToFloat((Holding_Registers_Database[CUSTCURR_ERROR_LIMIT_ADDR] << 16) | Holding_Registers_Database[CUSTCURR_ERROR_LIMIT_ADDR+1]);
    float warn_lim  = convertIEEE754ToFloat((Holding_Registers_Database[CUSTCURR_WARNING_LIMIT_ADDR] << 16) | Holding_Registers_Database[CUSTCURR_WARNING_LIMIT_ADDR+1]);

    if (actualCurrent > oc_thresh) {
        setStatus(currentOverTreshold);
        setRelayStatus(0);
        setRedStatusLed(1);
    }
    else if (actualCurrent > err_lim) {
        setStatus(currentError);
        setRedStatusLed(1);
    }
    else if (actualCurrent > warn_lim){
        setStatus(currentWarning);
        setRedStatusLed(0);
    }
    else {
        setStatus(noError);
        setRedStatusLed(0);
    }
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

uint8_t setRelayStatus(uint8_t relayState){
	if(relayState != Coils_Database[RELAY_STATE_ADDR]) {
		HAL_GPIO_WritePin(RELAY_CTRL_GPIO_Port, RELAY_CTRL_Pin, Coils_Database[RELAY_STATE_ADDR]);
		setGreenStatusLed(Coils_Database[RELAY_STATE_ADDR]);
		return Coils_Database[RELAY_STATE_ADDR];
	}
	return relayState;
}

void setStatus(uint8_t status){
	if (status != Input_Registers_Database[IEC_STATUS_ADDR]) Input_Registers_Database[IEC_STATUS_ADDR] = status;
}

void setGreenStatusLed(uint8_t status){
	HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, status);
}

void setRedStatusLed(uint8_t status){
	HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, status);
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
