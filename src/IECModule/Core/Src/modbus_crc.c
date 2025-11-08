#include "stdint.h"

uint16_t calculateCrc(uint8_t *buffer, uint16_t len) {
    uint16_t value = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        value ^= (uint16_t)buffer[i];
        for (uint8_t j = 0; j < 8; j++) {
            uint8_t lsb = value & 1;
            value >>= 1;
            if (lsb) {
                value ^= 0xA001;
            }
        }
    }
    return value;
}
