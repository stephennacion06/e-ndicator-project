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

const float  DENOMINATOR  = (RESISTOR_TWO_VALUE ) / (float) ( RESISTOR_ONE_VALUE + RESISTOR_TWO_VALUE );

static float voltageReadings[VOLTAGE_NUM_READINGS];
static int voltageReadIndex  = 0;
static float voltageTotal  = 0;

static float currentReadings[CURRENT_NUM_READINGS];
static int currentReadIndex  = 0;
static float currentTotal  = 0;

static float m_computedVoltage = 0;
static float m_computedCurrent = 0;
static const int sampleInterval = 1; // Sample interval in milliseconds (1 ms)
static unsigned long lastSampleTime = 0;

/* Private Function Declaration */
static float voltageMovingAverage( float_t rawVoltage );
static float currentMovingAverage( float_t rawCurrent );
static void computeVoltage( void );
static void computeCurrent(void);

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
        if( currentVoltageValue > VOLTAGE_CALIBRATION )
        {
            currentVoltageValue += VOLTAGE_CALIBRATION;
        } 
        filteredVoltageValue = voltageMovingAverage(currentVoltageValue);
#if CALIBRATION_ENABLED
        DEBUG_PRINT_LN("CALIBRATION(DC VoltageValue): " + String(filteredVoltageValue));
#endif // CALIBRATION_ENABLED
    }
    // return ( estimated_value + sim800lInterface_getVoltageCalibration() );
    m_computedVoltage = filteredVoltageValue;
}

static void computeCurrent(void)
{
    float filteredCurrent = 0;
    int rawValue = analogRead(GPIO_CURRENT_PIN);
    float voltage = (rawValue / 4095.0) * 3300; // ESP32 ADC is 12-bit, and 3.3V reference voltage
    float current = (voltage - 2500) / CURRENT_SENSOR_SENSITIVITY_MV;
    current += CURRENT_CALIBRATION;
    filteredCurrent = currentMovingAverage(current);
    if(filteredCurrent < 0)
    {
        filteredCurrent = 0;
    }
#if CALIBRATION_ENABLED
    DEBUG_PRINT_LN("CALIBRATION(DC CurrentValue): " + String(filteredCurrent));
#endif // CALIBRATION_ENABLED
    m_computedCurrent = filteredCurrent; 
}