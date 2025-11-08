#ifndef IECRegisterMap_h
#define IECRegisterMap_h

/*
----------------Coils------------------
*/
#define RELAY_STATE_ADDR 0x0000 // The actual status of the relay (closed or open)
#define IS_VOLTAGE_MEASURED_ADDR 0x0001 // Voltage measuring is available
#define IS_CURRENT_MEASURED_ADDR 0x0002 // Current measuring is available

/*
-------------Input registers---------------
*/
#define ID_ADDR 0x0000 // The ID of the IEC module
#define VERSION_ADDR 0x0001 // The VERSION of the IEC module
#define IEC_STATUS_ADDR 0x0002 // 
#define AVAILABLE_LEDS_ADDR 0x0003 // 
#define CURRENT_LIMIT_ADDR 0x0004 // 
#define RELAY_COUNT_ADDR 0x0005 // 
#define RMS_CURRENT_ADDR 0x0006 // 
#define RMS_VOLTAGE_ADDR 0x0007 // 
#define ACTIVE_POWER_ADDR 0x0008 // 
#define REACTIVE_POWER_ADDR 0x0009 // 
#define APPARENT_POWER_ADDR 0x000A // 
#define POWER_FACTOR_ADDR 0x000B // 
#define AC_FREQUENCY_ADDR 0x000C // 

/*
-------------Holding registers---------------
*/
#define HOLDING_REGISTER_POWER_DATA_UPDATE_CYCLE_TIME_ADDR 0x0000 // 
#define CUSTCURR_LIMIT_ADDR 0x0001 // 
#define CUSTCURR_WARNING_LIMIT_ADDR 0x0002 // 
#define CUSTCURR_ERROR_LIMIT_ADDR 0x0003 // 

#endif