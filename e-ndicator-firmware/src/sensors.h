#ifndef SENSORS_H
#define SENSORS_H
#include <Arduino.h>
#include <SimpleKalmanFilter.h>

#define GPIO_CURRENT_PIN ( 32U )
#define GPIO_VOLTAGE_PIN ( 35U )
#define GPIO_TOUCH_SENSOR_PIN ( 33 )

#define MV_VOLTAGE_PER_AMP ( 185U )         // this the 5A version of the ACS712 -use 100 for 20A Module and 66 for 30A Module

float sensors_getVoltage( void );
float sensors_getCurrentReading( void );
void sensors_initializeAllSensors( void );
void sensors_voltageCurrentTask( void *parameter );

#endif // SENSORS_H