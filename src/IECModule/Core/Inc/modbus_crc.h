/*
 * modbus_crc.h
 *
 *  Created on: Sep 16, 2022
 *      Author: arunr
 */

#ifndef INC_MODBUS_CRC_H_
#define INC_MODBUS_CRC_H_
#include "stdint.h"

uint16_t calculateCrc(uint8_t *buffer, uint16_t len);

#endif /* INC_MODBUS_CRC_H_ */
