
#ifndef BATTERY_PARAMETER_H
#define BATTERY_PARAMETER_H

#include <Arduino.h>

/* NOTE: Rough Estimate for End of life Internal Resistance. 
* Remember, for accurate values, it's best to check with the manufacturer. 
*/
#define END_OF_LIFE_INTERNAL_RESISTANCE  ( 0.5f )

#define BATTERY_12V_MAX ( 14.8f )
#define BATTERY_24V_MIN ( 20.0f )
#define BATTERY_24V_MAX ( 29.4f )
#define BATTERY_36V_MIN ( 30.0f )
#define BATTERY_36V_MAX ( 44.1f )
#define BATTERY_48V_MIN ( 44.2f )
#define BATTERY_48V_MAX ( 58.8f )

#define BATTERY_12_V_NOMINAL_VOLTAGE_MIN ( 10.8f )
#define BATTERY_12_V_NOMINAL_VOLTAGE_MAX ( 12.6f )
#define BATTERY_24_V_NOMINAL_VOLTAGE_MIN ( 20.0f )
#define BATTERY_24_V_NOMINAL_VOLTAGE_MAX ( 29.4f )
#define BATTERY_36_V_NOMINAL_VOLTAGE_MIN ( 31.0f )
#define BATTERY_36_V_NOMINAL_VOLTAGE_MAX ( 44.0f )
#define BATTERY_48_V_NOMINAL_VOLTAGE_MIN ( 40.0f )
#define BATTERY_48_V_NOMINAL_VOLTAGE_MAX ( 58.8f )

#define VOLTAGE_READING_SMALLEST         ( 4 )

enum BatteryVoltage {
    VOLTAGE_NONE,
    VOLTAGE_12V,
    VOLTAGE_24V,
    VOLTAGE_36V,
    VOLTAGE_48V
};

void batteryParameter_internalResistanceSetup( void );
float batteryParameter_getSoh( void );
float batteryParameter_getSoc( float currentReading, float voltageReading );
float batteryParameter_getInternalResistance( void );
void batteryParameter_initializeSocParam( void );
void batteryParameter_initializeSohParam( void );
#endif // BATTERY_PARAMETER_H