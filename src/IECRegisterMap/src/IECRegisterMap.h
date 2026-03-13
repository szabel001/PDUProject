#ifndef IECRegisterMap_h
#define IECRegisterMap_h

/*
----------------Coils------------------
*/
#define RELAY_STATE_ADDR 0x0000 // The actual status of the relay (closed or open)
#define IS_RMS_VOLTAGE_MEASURED_ADDR 0x0001 // Voltage measuring is available
#define IS_RMS_CURRENT_MEASURED_ADDR 0x0002 // Current measuring is available
#define IS_ACTIVE_POWER_MEASURED_ADDR 0x0003 // Current measuring is available
#define IS_REACTIVE_POWER_MEASURED_ADDR 0x0004 // Current measuring is available
#define IS_APPARENT_POWER_MEASURED_ADDR 0x0005 // Current measuring is available
#define IS_POWER_FACTOR_MEASURED_ADDR 0x0006 // Current measuring is available
#define IS_AC_FREQUENCY_MEASURED_ADDR 0x0007 // Current measuring is available

/*
-------------Input registers---------------
*/
#define ID_ADDR 0x0000 // The ID of the IEC module
#define VERSION_ADDR 0x0002 // The VERSION of the IEC module
#define IEC_STATUS_ADDR 0x0006 // Standby - , Normal -, Off -
#define AVAILABLE_LEDS_ADDR 0x0008 //
#define CURRENT_LIMIT_ADDR 0x000A //
#define RELAY_COUNT_ADDR 0x000C //
#define RMS_CURRENT_ADDR 0x0012 //
#define RMS_VOLTAGE_ADDR 0x0014 //
#define ACTIVE_POWER_ADDR 0x0016 //
#define REACTIVE_POWER_ADDR 0x0018 //
#define APPARENT_POWER_ADDR 0x001A //
#define POWER_FACTOR_ADDR 0x001C //
#define AC_FREQUENCY_ADDR 0x001E //

/*
-------------Holding registers---------------
*/
#define MEAS_AVG_NUM_ADDR 0x0000 // 
#define CUSTCURR_WARNING_LIMIT_ADDR 0x0002 //
#define CUSTCURR_ERROR_LIMIT_ADDR 0x0004 //
#define OC_TRESHOLD_ADDR 0x0006 //

#endif
