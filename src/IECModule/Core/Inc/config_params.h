#ifndef CONFIG_PARAMS_H
#define CONFIG_PARAMS_H

#include <stdint.h>

#define CONFIG_FLASH_ADDR  0x0800FC00  // (64kB - 2kB)

typedef struct {
    uint32_t magic;

    uint8_t MODBUS_ID;
    uint32_t ID;
    uint32_t VERSION;
    uint32_t AVAILABLE_LEDS;
    float CURRENT_LIMIT;
    uint32_t RELAY_COUNT;

    uint8_t IS_RMS_VOLTAGE_MEASURED;
    uint8_t IS_RMS_CURRENT_MEASURED;
    uint8_t IS_ACTIVE_POWER_MEASURED;
    uint8_t IS_REACTIVE_POWER_MEASURED;
    uint8_t IS_APPARENT_POWER_MEASURED;
    uint8_t IS_POWER_FACTOR_MEASURED;
    uint8_t IS_AC_FREQUENCY_MEASURED;
} Config_Params_t;

extern Config_Params_t *Config_Params;

#endif
