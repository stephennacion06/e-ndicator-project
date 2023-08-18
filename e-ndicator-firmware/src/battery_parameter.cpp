#include "battery_parameter.h"
#include "sim800l_interface.h"
#include "sensors.h"
#include "oled_display.h"
#include "utils.h"

#define TOUCH_DEBOUNCE_THRESHOLD ( 30 )
#define CHARGE_STATE_DISCHARGE   ( -1 )
#define BATTERY_EFFICIENCY       ( 0.9 )
#define LITHIUM_ION_INTERNAL_RESISTANCE     0.05 //0.01 ohms for lithium ion typical initial resitance
#define DEBOUNCE_DELAY ( 100 )    // the debounce time; increase if the output flickers
#define DELAY_VALUE  ( 2500 )

// SOC PARAMETERS
static float batteryCapacity = 1000; // Ah
static float batterySoh = 0.99;
static float batterySoc = 0;
static float m_lastSoc = 100;

// SOH SETUP 
static float m_openCircuitVoltageValue = 0;
float m_voltageWithLoad = 0; // voltage with load
float m_currentWithLoad = 0; // current with laod
float m_internalResistance = 0; 

static unsigned long lastDebounceUpdateTime = 0;  // the last time the output pin was toggled

/* Private Function Declaration */
static int determineBatteryType(float voltage);
static float batteryParameter_getBatteryCapacity( void );

/* Public Function Definition */
void batteryParameter_internalResistanceSetup( void )
{
    int touchValue = 90;       // initial value of touch sensor
    int updatingState = true;  // the current state of update
    
    //Determine Initial SOC
    m_lastSoc = determineBatteryType( m_openCircuitVoltageValue );

    while( updatingState ) 
    {
        touchValue = touchRead( GPIO_TOUCH_SENSOR_PIN );

        if( touchValue > TOUCH_DEBOUNCE_THRESHOLD )
        {
            lastDebounceUpdateTime = millis();
        }
        if( ( millis() - lastDebounceUpdateTime ) > DEBOUNCE_DELAY )
        {
            updatingState = false;
        }
        m_openCircuitVoltageValue = sensors_getVoltage();
        
        if (m_openCircuitVoltageValue < 4)
        {
            m_openCircuitVoltageValue = 0;
        }
        
        oledDisplay_irSetupDisplay(m_openCircuitVoltageValue, m_voltageWithLoad, m_currentWithLoad);
        DEBUG_PRINT("OCV:");
        DEBUG_PRINT(m_openCircuitVoltageValue);
        DEBUG_PRINT_LN("V");
    }

    DEBUG_PRINT("*****************");
    DEBUG_PRINT_LN("");
    DEBUG_PRINT("OCV:");
    DEBUG_PRINT(m_openCircuitVoltageValue);
    DEBUG_PRINT_LN("V");

    //CURRENT CAPTURE AND CIRCUT VOLTAGE
    touchValue = 90;
    updatingState = true;
    
    delay( DELAY_VALUE );
    
    while (updatingState) 
    {
    touchValue = touchRead( GPIO_TOUCH_SENSOR_PIN );
    m_voltageWithLoad = sensors_getVoltage();
    m_currentWithLoad = sensors_getCurrent();

            if (touchValue > TOUCH_DEBOUNCE_THRESHOLD) {
                lastDebounceUpdateTime = millis();
            }
            if ((millis() - lastDebounceUpdateTime) > DEBOUNCE_DELAY) {
                updatingState = false;
            }
    if (m_voltageWithLoad < 4){
        m_voltageWithLoad = 0;
    }

    oledDisplay_irSetupDisplay(m_openCircuitVoltageValue, m_voltageWithLoad, m_currentWithLoad);
    DEBUG_PRINT("m_voltageWithLoad:");
    DEBUG_PRINT(m_voltageWithLoad);
    DEBUG_PRINT("V");
    DEBUG_PRINT("; m_voltageWithLoad:");
    DEBUG_PRINT(m_currentWithLoad);
    DEBUG_PRINT_LN("A");
    }
    DEBUG_PRINT_LN("*****************");
    DEBUG_PRINT("m_voltageWithLoad:");
    DEBUG_PRINT(m_voltageWithLoad);
    DEBUG_PRINT("V");
    DEBUG_PRINT("; m_currentWithLoad:");
    DEBUG_PRINT(m_currentWithLoad);
    DEBUG_PRINT_LN("A");
    touchValue=90;

    m_internalResistance = (m_openCircuitVoltageValue-m_voltageWithLoad)/m_currentWithLoad;
    DEBUG_PRINT("Internal Resistance");
    DEBUG_PRINT_LN(m_internalResistance);

    oledDisplay_irValue(m_internalResistance);
}

float batteryParameter_getSoc( float currentReading )
{
    // TODO: SOC cleanup
    return ( m_lastSoc  + CHARGE_STATE_DISCHARGE*( currentReading / batteryParameter_getBatteryCapacity() )*0.000277778*100 + sim800lInterface_getSocCalibration() );
}

float batteryParameter_getSoh( void )
{
    // TODO: SOH Cleanup
    float endOfLifeInternalResitance = (165*LITHIUM_ION_INTERNAL_RESISTANCE)/100;
    
    return ( ( m_openCircuitVoltageValue - m_voltageWithLoad)/(endOfLifeInternalResitance*m_currentWithLoad)*100 + sim800lInterface_getSohCalibration() );
}

float batteryParameter_getInternalResistance( void )
{
    return m_internalResistance;
}

/* Private function Definition */
static float batteryParameter_getBatteryCapacity( void )
{
    // TODO: get initial battery capacity
    return ( batteryCapacity * batterySoh * BATTERY_EFFICIENCY );
}

static int determineBatteryType(float voltage)
{
    int updated_soc = 0;
    
    if(voltage <= 14.4)
    {
        updated_soc = map(voltage,11.51,12.73,0,100);
        
        if(updated_soc < 0)
        {
            updated_soc=10;
        }
        return updated_soc ;
    }

    else if(voltage > 14.5 && voltage < 26.4)
    {
        updated_soc = map(voltage, 23.02, 25.46,0,100);
    if(updated_soc < 0)
    {
        updated_soc=10;
    }

        return updated_soc;
    }

    else if(voltage > 26.5 && voltage < 38.4)
    { 
        updated_soc = map(voltage, 35.02, 38.4,0,100);
        if(updated_soc < 0)
        {
        updated_soc=10;
        }
        return updated_soc;
    }

    else if(voltage > 38.5)
    { 
        updated_soc = map(voltage, 46.04, 50.92,0,100);

        if(updated_soc < 0)
        {
            updated_soc=10;
        }
    else if(updated_soc > 100)
    {
        updated_soc=100;
    }
        return updated_soc;
    }

    else if(voltage > 51)
    { 
        updated_soc = map(voltage, 60, 65,0,100);

        if(updated_soc < 0)
        {
            updated_soc=10;
        }
    else if(updated_soc > 100)
    {
        updated_soc=100;
    }
        return updated_soc;
    }

    else
    {
        return 0;
    }
}