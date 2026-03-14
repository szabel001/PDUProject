#include "IECData.h"
#include <math.h>

enum IECStatus {
 noError = 0,
 currentWarning = 1,
 currentError = 2,
 currentOverTreshold = 3,
};

// Globális statikus változó az offset tárolására.
// Kezdőértéknek megadjuk a névleges elméleti közepet.
static float current_adc_offset = 2150.0f;

void readCurrentData(ADC_HandleTypeDef* hadc)
{
    uint32_t sq_sum = 0;
    uint32_t raw_sum = 0;

    // Minták számának kiolvasása a regiszterből
    float avgFloat = convertIEEE754ToFloat((Holding_Registers_Database[MEAS_AVG_NUM_ADDR] << 16) | Holding_Registers_Database[MEAS_AVG_NUM_ADDR+1]);
    uint16_t samples = (uint16_t)avgFloat;

    // Biztonsági limit, ha érvénytelen adat lenne a regiszterben
    if(samples == 0 || samples > 2000) samples = 400;

    for(uint16_t i = 0; i < samples; i++)
    {
        HAL_ADC_Start(hadc);
        HAL_ADC_PollForConversion(hadc, 10);
        uint16_t ADCRaw = HAL_ADC_GetValue(hadc);
        HAL_ADC_Stop(hadc);

        raw_sum += ADCRaw;

        // AC komponens számítása az ELSZÁMOLT offset alapján
        float centered_sample = (float)ADCRaw - current_adc_offset;

        // Négyzetre emelés és összegzés (RMS-hez)
        sq_sum += (uint32_t)(centered_sample * centered_sample);
    }

    // Kiszámoljuk a mostani mérés DC átlagát (nyers átlag)
    float avgRaw = (float)raw_sum / samples;

    // =========================================================
    // AUTOMATIKUS KALIBRÁCIÓ ÉS ÁRAM SZÁMÍTÁS
    // =========================================================

    if (Coils_Database[RELAY_STATE_ADDR] == 0) {
        // HA A RELÉ KIKAPCSOLT ÁLLAPOTBAN VAN:
        // Garantáltan 0A folyik. A mért átlag a szenzor új nullpontja.
        // Egy mozgóátlagot (EMA - 80% régi, 20% új) használunk,
        // hogy egy hirtelen zajtüske ne rontsa el a kalibrációt.
        current_adc_offset = (current_adc_offset * 0.8f) + (avgRaw * 0.2f);

        // Az áramot kényszerítetten 0-ra állítjuk
        actualCurrent = 0.0f;
    }
    else {
        // HA A RELÉ BEKAPCSOLT ÁLLAPOTBAN VAN:
        // Kiszámoljuk az RMS ADC értéket a kikapcsolt állapotban megtanult offset használatával
        float rms_adc = sqrtf((float)sq_sum / (float)samples);

        // Áram konvertálása a kalibrációs polinommal (rms_adc az x)
        float x = rms_adc;
        float current = -2.17167128e-7f * x * x + 1.20380534e-2f * x + 3.33630450e-3f;

        // Nullpont alatti zaj levágása
        if (current < 0.0f) current = 0.0f;

        actualCurrent = current;
    }

    // Eredmény beírása a Modbus regiszterbe
    addFloatToRegister(Input_Registers_Database, RMS_CURRENT_ADDR, actualCurrent);

    // =========================================================
    // LIMIT CHECK
    // =========================================================
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
