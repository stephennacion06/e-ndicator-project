#ifndef SENSORS_H
#define SENSORS_H
#include <Arduino.h>
#include <SimpleKalmanFilter.h>

#define GPIO_CURRENT_PIN ( 35U )
#define GPIO_VOLTAGE_PIN ( 34U )

#define RESISTOR_ONE_VALUE ( 100000U )
#define RESISTOR_TWO_VALUE ( 5000U )

#define MV_VOLTAGE_PER_AMP ( 185U )         // this the 5A version of the ACS712 -use 100 for 20A Module and 66 for 30A Module

float sensors_getVoltage( void );
float sensors_getCurrent( void );
float sensors_getCurrent_2( void );

#endif // SENSORS_H