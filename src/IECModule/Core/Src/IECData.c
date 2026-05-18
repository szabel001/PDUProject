#include "IECData.h"
#include <math.h>

enum IECStatus {
 noError = 0,
 currentWarning = 1,
 currentError = 2,
 currentOverTreshold = 3,
};

static float current_adc_offset = 2150.0f;

static float current_sum_for_avg = 0.0f;
static uint32_t current_meas_count = 0;

void readCurrentData(ADC_HandleTypeDef* hadc)
{
    double sq_sum = 0.0;
    uint32_t raw_sum = 0;
    uint32_t sample_count = 0;

    uint32_t measure_time_ms = 100;
    uint32_t start_time = HAL_GetTick();

    while((HAL_GetTick() - start_time) < measure_time_ms)
    {
        HAL_ADC_Start(hadc);
        HAL_ADC_PollForConversion(hadc, 10);
        uint16_t ADCRaw = HAL_ADC_GetValue(hadc);
        HAL_ADC_Stop(hadc);

        raw_sum += ADCRaw;

        float centered_sample = (float)ADCRaw - current_adc_offset;
        sq_sum += (double)(centered_sample * centered_sample);

        sample_count++;
    }

    if(sample_count == 0) return;

    float avgRaw = (float)raw_sum / sample_count;
    float actualCurrent = 0.0f;

    // =========================================================
    // Automatic calibration and Current meas
    // =========================================================
    if (Coils_Database[RELAY_STATE_ADDR] == 0) {
        current_adc_offset = (current_adc_offset * 0.8f) + (avgRaw * 0.2f);
        actualCurrent = 0.0f;
    }
    else {
        float rms_adc = sqrtf((float)(sq_sum / sample_count));

        float current = (0.01196f * rms_adc) - 25.718f;

        if (current < 0.0f) current = 0.0f;
        actualCurrent = current;
    }
    // =========================================================
    // Handling OverCurrent
    // =========================================================
    float oc_thresh = convertIEEE754ToFloat((Holding_Registers_Database[OC_TRESHOLD_ADDR] << 16) | Holding_Registers_Database[OC_TRESHOLD_ADDR+1]);

    if (actualCurrent > oc_thresh) {
        setStatus(currentOverTreshold);
        setRelayStatus(0);
        setRedStatusLed(1);

        current_sum_for_avg = 0.0f;
        current_meas_count = 0;
        return;
    }

    // =========================================================
    // 2. Averaging
    // =========================================================
    uint16_t avg_num = Holding_Registers_Database[MEAS_AVG_NUM_ADDR];
    if (avg_num == 0) avg_num = 1;

    current_sum_for_avg += actualCurrent;
    current_meas_count++;

    if (current_meas_count >= avg_num) {
        float final_avg_current = current_sum_for_avg / current_meas_count;
        addFloatToRegister(Input_Registers_Database, RMS_CURRENT_ADDR, final_avg_current);

        float err_lim   = convertIEEE754ToFloat((Holding_Registers_Database[CUSTCURR_ERROR_LIMIT_ADDR] << 16) | Holding_Registers_Database[CUSTCURR_ERROR_LIMIT_ADDR+1]);
        float warn_lim  = convertIEEE754ToFloat((Holding_Registers_Database[CUSTCURR_WARNING_LIMIT_ADDR] << 16) | Holding_Registers_Database[CUSTCURR_WARNING_LIMIT_ADDR+1]);

        if (final_avg_current > err_lim) {
            setStatus(currentError);
            setRelayStatus(0);
            setRedStatusLed(1);
        }
        else if (final_avg_current > warn_lim){
            setStatus(currentWarning);
            setRedStatusLed(0);
        }
        else {
            setStatus(noError);
            setRedStatusLed(0);
        }

        current_sum_for_avg = 0.0f;
        current_meas_count = 0;
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

void setRelayStatus(){
	uint8_t coilRelayState = Coils_Database[RELAY_STATE_ADDR];
	if(prevRelayState != coilRelayState) {
		prevRelayState = coilRelayState;
		HAL_GPIO_WritePin(RELAY_CTRL_GPIO_Port, RELAY_CTRL_Pin, coilRelayState);
		setGreenStatusLed(coilRelayState);
	}
}
void setRelayStatus(uint8_t relayState){
	uint8_t coilRelayState = Coils_Database[RELAY_STATE_ADDR];
	if(relayState != coilRelayState) {
		Coils_Database[RELAY_STATE_ADDR] = relayState;
		prevRelayState = relayState;
		HAL_GPIO_WritePin(RELAY_CTRL_GPIO_Port, RELAY_CTRL_Pin, relayState);
		setGreenStatusLed(relayState);
	}
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
	union
	{
	    float f;
        uint16_t u16[2];
	} float_union;

	float_union.f = addedFloat;

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
