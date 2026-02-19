#include "stm32g0xx_hal.h"
#include <stdint.h>
#include "config_params.h"

__attribute__((section(".config_params")))
__attribute__((used))
Config_Params_t Config_Params_Image = {
    .magic = 0xFFFFFFFF,
    .MODBUS_ID = 0xFF,
    .ID = 0xFFFFFFFF,
    .VERSION = 0xFFFFFFFF,
    .AVAILABLE_LEDS = 0xFFFFFFFF,
    .CURRENT_LIMIT = 0xFFFFFFFF,
    .RELAY_COUNT = 0xFFFFFFFF,
	.IS_RMS_VOLTAGE_MEASURED = 0xFF,
	.IS_RMS_CURRENT_MEASURED = 0xFF,
	.IS_ACTIVE_POWER_MEASURED = 0xFF,
	.IS_REACTIVE_POWER_MEASURED = 0xFF,
	.IS_APPARENT_POWER_MEASURED = 0xFF,
	.IS_POWER_FACTOR_MEASURED = 0xFF,
	.IS_AC_FREQUENCY_MEASURED = 0xFF,
};

Config_Params_t *Config_Params = (Config_Params_t*)CONFIG_FLASH_ADDR;
