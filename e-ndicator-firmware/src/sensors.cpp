#include "sensors.h"
#include "sim800l_interface.h"
#include "utils.h"
#include "user.h"

#define VPP_SAMPLING_TIME ( 700 ) 
#define RESISTOR_ONE_VALUE ( 100000 )
#define RESISTOR_TWO_VALUE  ( 3300 )
#define VOLTAGE_NUM_READINGS ( 100U )

const float  DENOMINATOR  = (RESISTOR_TWO_VALUE ) / (float) ( RESISTOR_ONE_VALUE + RESISTOR_TWO_VALUE );

static float voltageReadings[VOLTAGE_NUM_READINGS];
static int voltageReadIndex  = 0;
static float voltageTotal  = 0;

SimpleKalmanFilter simpleKalmanFilter_voltage(2, 2, 0.1);
SimpleKalmanFilter simpleKalmanFilter_current(2, 2, 0.01);

/* Private Function Declaration */
static float getVpp( void );
static float getVpp_2( void );
static float voltageMovingAverage( float_t rawVoltage );

/* Public Function Definition */
float sensors_getVoltage( void )
{
    float currentVoltageValue = 0;
    float estimated_value = 0;
    float adcVoltageValue = 0;
    float filteredVoltageValue = 0;

    adcVoltageValue = analogRead( GPIO_VOLTAGE_PIN );
    if ( DENOMINATOR > 0)
    {
        currentVoltageValue = ( ( (float) adcVoltageValue * 3.3 ) / (4095.0) ) / DENOMINATOR;
        if( currentVoltageValue > VOLTAGE_CALIBRATION )
        {
            currentVoltageValue += VOLTAGE_CALIBRATION;
        } 
        filteredVoltageValue = voltageMovingAverage(currentVoltageValue);
        DEBUG_PRINT_LN("CALIBRATION(DC VoltageValue): " + String(filteredVoltageValue));
    }

    estimated_value = simpleKalmanFilter_voltage.updateEstimate( currentVoltageValue );
    return ( estimated_value + sim800lInterface_getVoltageCalibration() );
}

float sensors_getCurrent( void )
{
    float Voltage = getVpp();
    double VRMS = (Voltage/2.0) *0.707;   //root 2 is 0.707
    double AmpsRMS = ((VRMS * 1000)/MV_VOLTAGE_PER_AMP)-0.25; //0.3 is the error I got for my sensor
    float estimated_value = simpleKalmanFilter_current.updateEstimate(AmpsRMS);

    return ( estimated_value + sim800lInterface_getCurrentCalibration() );
}

float sensors_getCurrent_2( void )
{
    float Voltage = getVpp_2();
    double VRMS = (Voltage/2.0) *0.707;   //root 2 is 0.707
    double AmpsRMS = ((VRMS * 1000)/MV_VOLTAGE_PER_AMP)-0.25; //0.3 is the error I got for my sensor
    float estimated_value = simpleKalmanFilter_current.updateEstimate(AmpsRMS);
    
    return  ( estimated_value + sim800lInterface_getCurrentCalibration() );
}

/* Private Function Definition */
static float getVpp( void )
{
    float result;
    int readValue;                // value read from the sensor
    int maxValue = 0;             // store max value here
    int minValue = 4096;          // store min value here ESP32 ADC resolution
    
    uint32_t start_time = millis();

    while( ( millis() - start_time ) < VPP_SAMPLING_TIME ) //sample for 700 mSec
    {
        readValue = analogRead(GPIO_CURRENT_PIN);
        // see if you have a new maxValue
        if (readValue > maxValue) 
        {
            /*record the maximum sensor value*/
            maxValue = readValue;
        }
        if (readValue < minValue) 
        {
            /*record the minimum sensor value*/
            minValue = readValue;
        }
    }
    
    // Subtract min from max
    result = ((maxValue - minValue) * 5)/4096.0; //ESP32 ADC resolution 4096
        
    return result;
}

static float getVpp_2( void )
{
    float result;
    int readValue;                // value read from the sensor
    int maxValue = 0;             // store max value here
    int minValue = 4096;          // store min value here ESP32 ADC resolution

    uint32_t start_time = millis();

    while((millis()-start_time) < 10) //sample for 700 mSec
    {
        readValue = analogRead( GPIO_CURRENT_PIN );
        // see if you have a new maxValue
        if (readValue > maxValue) 
        {
            /*record the maximum sensor value*/
            maxValue = readValue;
        }
        if (readValue < minValue) 
        {
            /*record the minimum sensor value*/
            minValue = readValue;
        }
    }
    // Subtract min from max
    result = ((maxValue - minValue) * 5)/4096.0; //ESP32 ADC resolution 4096

    return result;
}

static float voltageMovingAverage( float_t rawVoltage )
{  
    ////Perform average on sensor readings
    float average;
    // subtract the last reading:
    voltageTotal = voltageTotal - voltageReadings[voltageReadIndex];
    // read the sensor:
    voltageReadings[voltageReadIndex] = rawVoltage;
    // add value to total:
    voltageTotal = voltageTotal + voltageReadings[voltageReadIndex];
    // handle index
    voltageReadIndex = voltageReadIndex + 1;
    if (voltageReadIndex >= VOLTAGE_NUM_READINGS)
    {
    voltageReadIndex = 0;
    }
    // calculate the average:
    average = voltageTotal / VOLTAGE_NUM_READINGS;

    return average;
}