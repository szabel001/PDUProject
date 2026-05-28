#ifndef IECRegisterMap_h
#define IECRegisterMap_h

/*
----------------Coils------------------
*/
#define RELAY_STATE_ADDR 0x0000 // The actual status of the relay (1 = open, 0 = closed).
#define IS_RMS_VOLTAGE_MEASURED_ADDR 0x0001 // Indicates if RMS voltage measuring is available/active.
#define IS_RMS_CURRENT_MEASURED_ADDR 0x0002 // Indicates if RMS current measuring is available/active.
#define IS_ACTIVE_POWER_MEASURED_ADDR 0x0003 // Indicates if active power measuring is available/active.
#define IS_REACTIVE_POWER_MEASURED_ADDR 0x0004 // Indicates if reactive power measuring is available/active.
#define IS_APPARENT_POWER_MEASURED_ADDR 0x0005 // Indicates if apparent power measuring is available/active.
#define IS_POWER_FACTOR_MEASURED_ADDR 0x0006 // Indicates if power factor measuring is available/active.
#define IS_AC_FREQUENCY_MEASURED_ADDR 0x0007 // Indicates if AC frequency measuring is available/active.

/*
-------------Input registers---------------
*/
#define ID_ADDR 0x0000 // The unique identifier of the IEC module.
#define VERSION_ADDR 0x0002 // The firmware or hardware version of the IEC module.
#define IEC_STATUS_ADDR 0x0004 // The current status of the IEC module
#define AVAILABLE_LEDS_ADDR 0x0006 // Number of available LEDs on the module.
#define CURRENT_LIMIT_ADDR 0x0008 // The configured maximum current limit.
#define RELAY_COUNT_ADDR 0x000A // Total number of relays present in the module.
#define RMS_CURRENT_ADDR 0x000C // The measured root mean square (RMS) current value.
#define RMS_VOLTAGE_ADDR 0x000E // The measured root mean square (RMS) voltage value.
#define ACTIVE_POWER_ADDR 0x0010 // The measured active (real) power.
#define REACTIVE_POWER_ADDR 0x0012 // The measured reactive power.
#define APPARENT_POWER_ADDR 0x0014 // The measured apparent power.
#define POWER_FACTOR_ADDR 0x0016 // The measured power factor (ratio of active to apparent power).
#define AC_FREQUENCY_ADDR 0x0018 // The measured AC grid frequency.

/*
-------------Holding registers---------------
*/
#define CUSTCURR_WARNING_LIMIT_ADDR 0x0000 // Custom current limit threshold that triggers a warning.
#define CUSTCURR_ERROR_LIMIT_ADDR 0x0002 // Custom current limit threshold that triggers an error.
#define OC_TRESHOLD_ADDR 0x0004 // Overcurrent threshold limit for protection mechanisms.

#endif