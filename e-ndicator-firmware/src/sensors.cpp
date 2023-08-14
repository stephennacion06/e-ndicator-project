#include "sensors.h"
#include "sim800l_interface.h"

SimpleKalmanFilter simpleKalmanFilter_voltage(2, 2, 0.1);
SimpleKalmanFilter simpleKalmanFilter_current(2, 2, 0.01);

#define VPP_SAMPLING_TIME ( 700U ) 

/* Private Function Declaration */
static float getVpp( void );
static float getVpp_2( void );

/* Public Function Definition */
float sensors_getVoltage( void )
{
    float currentVoltageValue = 0;
    float estimated_value = 0;
    float adcVoltageValue = 0;

    adcVoltageValue = analogRead( GPIO_VOLTAGE_PIN );
    currentVoltageValue = ( ( adcVoltageValue * 3.3 ) / (4095) ) / ( RESISTOR_TWO_VALUE / ( RESISTOR_ONE_VALUE + RESISTOR_TWO_VALUE ) );
    estimated_value = simpleKalmanFilter_voltage.updateEstimate( currentVoltageValue );

    return ( estimated_value + getVoltageCalibration() );
}

float sensors_getCurrent( void )
{
    float Voltage = getVpp();
    double VRMS = (Voltage/2.0) *0.707;   //root 2 is 0.707
    double AmpsRMS = ((VRMS * 1000)/MV_VOLTAGE_PER_AMP)-0.25; //0.3 is the error I got for my sensor
    float estimated_value = simpleKalmanFilter_current.updateEstimate(AmpsRMS);

    return ( estimated_value + getCurrentCalibration() );
}

float sensors_getCurrent_2( void )
{
    float Voltage = getVpp_2();
    double VRMS = (Voltage/2.0) *0.707;   //root 2 is 0.707
    double AmpsRMS = ((VRMS * 1000)/MV_VOLTAGE_PER_AMP)-0.25; //0.3 is the error I got for my sensor
    float estimated_value = simpleKalmanFilter_current.updateEstimate(AmpsRMS);
    
    return  ( estimated_value + getCurrentCalibration() );
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