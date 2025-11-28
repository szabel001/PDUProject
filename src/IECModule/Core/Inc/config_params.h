#ifndef CONFIG_PARAMS_H
#define CONFIG_PARAMS_H

#include <stdint.h>

#define CONFIG_FLASH_ADDR  0x0800FC00  // (64kB - 2kB)

typedef struct {
    uint32_t magic;
    uint32_t ID;
    uint32_t VERSION;
    uint32_t AVAILABLE_LEDS;
    float CURRENT_LIMIT;
    uint32_t RELAY_COUNT;
    uint8_t IS_CURRENT_MEASURED;
    uint8_t IS_VOLTAGE_MEASURED;
    uint8_t MODBUS_ID;
    uint8_t reserved;
} Config_Params_t;

extern Config_Params_t *Config_Params;

#endif
