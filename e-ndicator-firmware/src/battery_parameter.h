
#ifndef BATTERY_PARAMETER_H
#define BATTERY_PARAMETER_H

#include <Arduino.h>

void batteryParameter_internalResistanceSetup( void );
float batteryParameter_getSoh( void );
float batteryParameter_getSoc( float currentReading );
float batteryParameter_getInternalResistance( void );

#endif // BATTERY_PARAMETER_H