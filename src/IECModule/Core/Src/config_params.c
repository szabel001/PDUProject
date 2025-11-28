#include "stm32g0xx_hal.h"
#include <stdint.h>
#include "config_params.h"

__attribute__((section(".config_params")))
__attribute__((used))
Config_Params_t Config_Params_Image = {
    .magic = 0xFFFFFFFF,
    .ID = 0xFFFFFFFF,
    .VERSION = 0xFFFFFFFF,
    .AVAILABLE_LEDS = 0xFFFFFFFF,
    .CURRENT_LIMIT = -1.0f,
    .RELAY_COUNT = 0xFFFFFFFF,
    .IS_CURRENT_MEASURED = 0xFF,
    .IS_VOLTAGE_MEASURED = 0xFF,
    .MODBUS_ID = 0xFF,
    .reserved = 0xFF,
};

Config_Params_t *Config_Params = (Config_Params_t*)CONFIG_FLASH_ADDR;
