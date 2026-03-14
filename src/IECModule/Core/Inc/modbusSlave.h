/*
 * modbusSlave.h
 *
 *  Created on: Oct 27, 2022
 *      Author: controllerstech.com
 */

#ifndef INC_MODBUSSLAVE_H_
#define INC_MODBUSSLAVE_H_

#include "modbus_crc.h"
#include "stm32g0xx_hal.h"
#include "main.h"
#include "IECData.h"
#include "config_params.h"

#define ILLEGAL_FUNCTION       0x01
#define ILLEGAL_DATA_ADDRESS   0x02
#define ILLEGAL_DATA_VALUE     0x03

extern uint16_t Holding_Registers_Database[NUM_HOLDING_REGISTERS];
extern uint16_t Input_Registers_Database[NUM_INPUT_REGISTERS];
extern uint8_t Coils_Database[NUM_COILS];
extern uint8_t Inputs_Database[NUM_DISCRETEINPUTS];

uint8_t readHoldingRegs (void);
uint8_t readInputRegs (void);
uint8_t readCoils (void);
uint8_t readInputs (void);

uint8_t writeSingleReg (void);
uint8_t writeHoldingRegs (void);
uint8_t writeSingleCoil (void);
uint8_t writeMultiCoils (void);

void modbusException (uint8_t exceptioncode);

#endif /* INC_MODBUSSLAVE_H_ */
