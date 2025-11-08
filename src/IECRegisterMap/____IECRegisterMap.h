#ifndef IECRegisterMap_h
#define IECRegisterMap_h

/*
----------------Coils------------------
relay, led állapotai
0x0000...0x0007: Relay status (1 bit)
*/
#define NUM_COILS 256

#define RELAY_STATE_START 0
#define RELAY_STATE_LENGTH 1
#define RELAY_STATE_ADDR 0x0000

#define IS_VOLTAGE_MEASURED_START 1
#define IS_VOLTAGE_MEASURED_LENGTH 1
#define IS_VOLTAGE_MEASURED_ADDR 0x0001

#define IS_CURRENT_MEASURED_START 2
#define IS_CURRENT_MEASURED_LENGTH 1
#define IS_CURRENT_MEASURED_ADDR 0x0002

/*
-------------Discrete inputs---------------
0x1000: Reserved (1 bit)
0x1001: Reserved (1 bit)
*/
#define NUM_DISCRETEINPUTS 256
/*

-------------Input registers---------------
......Settings, capabilities, status, etc.........
from 0x0000 to 0x00FF
0x0000: ID (1 registers, 2 bytes)
0x0001: Version (1 registers, 2 bytes)
0x0002: IEC status (1 registers, 2 bytes) // 0000: Off, 0001: Standby, 0010: , 0011: Active, 0100: Warning, 0101: Error
0x0003: Available LEDs (1 registers, 2 bytes) // 0000: No support, 0001: R, 0010: RG, 0011: RYG
0x0004: Current limit (1 registers, 2 bytes) //16A or 32A
0x0005: Relay count (1 registers, 2 bytes) // up to 8 relays
0x0006: Measure voltage count (1 registers, 2 bytes) // up to 8 
0x0007: Measure current count (1 registers, 2 bytes) // up to 8

............Power related data..............
from 0x0100 to 0x01FF
0x0000: RMS Current (2 registers, 4 bytes) --> FLOAT32
0x0002: RMS Voltage (2 registers, 4 bytes) --> FLOAT32
0x0004: Active Power (2 registers, 4 bytes) --> FLOAT32
0x0004: Reactive Power (2 registers, 4 bytes) --> FLOAT32
0x0004: Apperant Power (2 registers, 4 bytes) --> FLOAT32
0x0008: Power factor (2 registers, 4 bytes) --> FLOAT32
0x0006: AC Frequency (2 registers, 4 bytes) --> FLOAT32

0x0012: Reserved (1 registers, 2 bytes)
*/
#define NUM_INPUT_REGISTERS 256

#define ID_LENGTH 1
#define ID_START 0
#define ID_ADDR 0x0000

#define VERSION_LENGTH 1
#define VERSION_START 1
#define VERSION_ADDR 0x0001

#define IEC_STATUS_LENGTH 1
#define IEC_STATUS_START 2
#define IEC_STATUS_ADDR 0x0002

#define AVAILABLE_LEDS_LENGTH 1
#define AVAILABLE_LEDS_START 3
#define AVAILABLE_LEDS_ADDR 0x0003

#define CURRENT_LIMIT_LENGTH 1
#define CURRENT_LIMIT_START 4
#define CURRENT_LIMIT_ADDR 0x0004

#define RELAY_COUNT_LENGTH 1
#define RELAY_COUNT_START 5
#define RELAY_COUNT_ADDR 0x0005

#define CUSTCURR_LIMIT_LENGTH 2
#define CUSTCURR_LIMIT_START 8
#define CUSTCURR_LIMIT_ADDR 0x0008

#define CUSTCURR_WARNING_LIMIT_LENGTH 2
#define CUSTCURR_WARNING_LIMIT_START 10
#define CUSTCURR_WARNING_LIMIT_ADDR 0x000A

#define CUSTCURR_ERROR_LIMIT_LENGTH 2
#define CUSTCURR_ERROR_LIMIT_START 12
#define CUSTCURR_ERROR_LIMIT_ADDR 0x000C

//--------------- Power related data --------------
#define RMS_CURRENT_LENGTH 2
#define RMS_CURRENT_START 8
#define RMS_CURRENT_ADDR 0x0008

#define RMS_VOLTAGE_LENGTH 2
#define RMS_VOLTAGE_START 10
#define RMS_VOLTAGE_ADDR 0x000A

#define ACTIVE_POWER_LENGTH 2
#define ACTIVE_POWER_START 12
#define ACTIVE_POWER_ADDR 0x000C

#define REACTIVE_POWER_LENGTH 2
#define REACTIVE_POWER_START 14
#define REACTIVE_POWER_ADDR 0x000E

#define APPARENT_POWER_LENGTH 2
#define APPARENT_POWER_START 16
#define APPARENT_POWER_ADDR 0x0010

#define POWER_FACTOR_LENGTH 2
#define POWER_FACTOR_START 18
#define POWER_FACTOR_ADDR 0x0012

#define AC_FREQUENCY_LENGTH 2
#define AC_FREQUENCY_START 20
#define AC_FREQUENCY_ADDR 0x0014

#define RESERVED_LENGTH 1
#define RESERVED_START 22
#define RESERVED_ADDR 0x0016

#define BROADCAST_ADDRESS 0
#define MAX_SLAVES 50
#define MODBUS_BAUD 115200

/*
-------------Holding registers---------------
IDE kellenek a kívülről beállítható dolgok, amik nem férnek bele egy bitbe
0x0000: Power data update cycle time (1 registers, 2 bytes) // ms
0x0001: Reserved (1 registers, 2 bytes)
*/
#define HOLDING_REGISTER_POWER_DATA_UPDATE_CYCLE_TIME_LENGTH 1
#define HOLDING_REGISTER_POWER_DATA_UPDATE_CYCLE_TIME_START 0
#define HOLDING_REGISTER_POWER_DATA_UPDATE_CYCLE_TIME_ADDR 0x0000

#define HOLDING_REGISTER_RESERVED_LENGTH 1
#define HOLDING_REGISTER_RESERVED_START 1
#define HOLDING_REGISTER_RESERVED_ADDR 0x0001

#define NUM_HOLDING_REGISTERS 256

#endif
