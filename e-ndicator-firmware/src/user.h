#ifndef USER_H
#define USER_H

#define USER_ID    ( 1 ) // USER ID
#define CALIBRATION_ENABLED ( 0 )

// DEVICE 1 CALIBRATION PARAMETERS
#if USER_ID == 1
#define VOLTAGE_CALIBRATION            ( 3.25 )
#define CURRENT_CALIBRATION            ( 0.0 )
#define CURRENT_SENSOR_MID_VOLTAGE     ( 2496U )
#define ADC_CURRENT_SENSOR_OFFSET      ( 0 )
#elif USER_ID == 2
#define VOLTAGE_CALIBRATION            ( 4.55 )
#define CURRENT_CALIBRATION            ( 0.0 )
#define CURRENT_SENSOR_MID_VOLTAGE     ( 2467U )
#define ADC_CURRENT_SENSOR_OFFSET      ( 221 )
#endif // USER_ID

#endif // USER_H