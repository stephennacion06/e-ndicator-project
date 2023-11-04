#include "sensors.h"
#include "sim800l_interface.h"
#include "utils.h"
#include "user.h"

#define VPP_SAMPLING_TIME ( 700 )
#define RESISTOR_ONE_VALUE ( 100000 )
#define RESISTOR_TWO_VALUE  ( 3300 )
#define VOLTAGE_NUM_READINGS ( 1000U )
#define CURRENT_NUM_READINGS ( 1000U )

#define CURRENT_SENSOR_SENSITIVITY_MV ( 66U ) // ACS712 30A 66mV/A sensitivity
#define ADC_SAMPLING_NUM       ( 10U )

const float  DENOMINATOR  = (RESISTOR_TWO_VALUE ) / (float) ( RESISTOR_ONE_VALUE + RESISTOR_TWO_VALUE );

static float voltageReadings[VOLTAGE_NUM_READINGS];
static int voltageReadIndex  = 0;
static float voltageTotal  = 0;

static float currentReadings[CURRENT_NUM_READINGS];
static int currentReadIndex  = 0;
static float currentTotal  = 0;

static float m_computedVoltage = 0;
static float m_computedCurrent = 0;
static float m_debugValue = 0;
static const int sampleInterval = 1; // Sample interval in milliseconds (1 ms)
static unsigned long lastSampleTime = 0;

/* Private Function Declaration */
static float voltageMovingAverage( float_t rawVoltage );
static float currentMovingAverage( float_t rawCurrent );
static void computeVoltage( void );
static void computeCurrent(void);
static int movingAverage(  int analogPin );

/* Public Function Definition */
void sensors_voltageCurrentTask( void *parameter )
{
    while(1)
    {
        computeVoltage();
        computeCurrent();
        vTaskDelay( 1 / portTICK_PERIOD_MS);
    }
}

float sensors_getVoltage( void )
{
    return m_computedVoltage;
}

float sensors_getCurrentReading( void )
{
    return m_computedCurrent;
}

float sensors_getDebugValue( void )
{
    return m_debugValue;
}

void sensors_initializeAllSensors( void )
{
    // Touch Sensor
    pinMode(GPIO_TOUCH_SENSOR_PIN, INPUT);
    
    // Current and Voltage Sensor
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
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

static float currentMovingAverage( float_t rawCurrent )
{  
    ////Perform average on sensor readings
    float average;
    // subtract the last reading:
    currentTotal = currentTotal - currentReadings[currentReadIndex];
    // read the sensor:
    currentReadings[currentReadIndex] = rawCurrent;
    // add value to total:
    currentTotal = currentTotal + currentReadings[currentReadIndex];
    // handle index
    currentReadIndex = currentReadIndex + 1;
    if (currentReadIndex >= CURRENT_NUM_READINGS)
    {
        currentReadIndex = 0;
    }
    // calculate the average:
    average = currentTotal / CURRENT_NUM_READINGS;

    return average;
}

static void computeVoltage( void )
{
    float currentVoltageValue = 0;
    float estimated_value = 0;
    float adcVoltageValue = 0;
    float filteredVoltageValue = 0;

    adcVoltageValue = analogRead( GPIO_VOLTAGE_PIN );
    if ( DENOMINATOR > 0)
    {
        currentVoltageValue = ( ( (float) adcVoltageValue * 3.3 ) / (4095.0) ) / DENOMINATOR;

        filteredVoltageValue = voltageMovingAverage(currentVoltageValue);
    }
    // return ( estimated_value + sim800lInterface_getVoltageCalibration() );
    if( filteredVoltageValue > VOLTAGE_CALIBRATION )
    {
        m_computedVoltage = filteredVoltageValue  + VOLTAGE_CALIBRATION;
    }
    else
    {
        m_computedVoltage = 0;
    }
}

int movingAverage(int analogPin)
{
    int total = 0; // Running total of readings
    int count = 0; // Number of readings counted
    int index = 0; // Index of the current reading

    for (int i = 0; i < ADC_SAMPLING_NUM; i++) {
        // Read the ADC value from the specified analog pin
        int rawValue = analogRead(analogPin);

        // Add the new reading to the total
        total += rawValue;

        // Increment the count of readings
        count++;

        // If we have enough readings, calculate the average
        if (count >= ADC_SAMPLING_NUM) {
        // Calculate the average
        int average = total / ADC_SAMPLING_NUM;

        // Subtract the oldest reading to maintain a rolling total
        total -= analogRead(analogPin);

        return average;
        }
    }
    // Return 0 if there are not enough readings yet
    return 0;
}

static void computeCurrent(void)
{
    float filteredCurrent = 0;
    int rawValue = movingAverage( GPIO_CURRENT_PIN ) + ADC_CURRENT_SENSOR_OFFSET;
    float voltage = (rawValue * 3300.0) / 4095; // ESP32 ADC is 12-bit, and 3.3V reference voltage
    float current = (voltage - CURRENT_SENSOR_MID_VOLTAGE) / CURRENT_SENSOR_SENSITIVITY_MV;
    filteredCurrent = currentMovingAverage(current);
    m_debugValue = rawValue;
    filteredCurrent += CURRENT_CALIBRATION;
    
    // Ensure m_computedCurrent is always negative
    if (filteredCurrent >= 0) {
        m_computedCurrent = -filteredCurrent;
    } else {
        m_computedCurrent = filteredCurrent;
    }
}