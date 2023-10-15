#include "battery_parameter.h"
#include "sim800l_interface.h"
#include "sensors.h"
#include "oled_display.h"
#include "utils.h"

#define TOUCH_DEBOUNCE_THRESHOLD ( 30 )
#define CHARGE_STATE_DISCHARGE   ( -1 )
#define BATTERY_EFFICIENCY       ( 0.9 )
#define LITHIUM_ION_INTERNAL_RESISTANCE     0.05 //0.01 ohms for lithium ion typical initial resitance
#define DEBOUNCE_DELAY ( 1000 )    // the debounce time; increase if the output flickers
#define DELAY_VALUE  ( 2500 )

#define SOC_FAIL_VALUE ( 1000 )

// SOC PARAMETERS
static float m_batteryCapacity = 0; // Ah
static float batterySoh = 0.99;
static float batterySoc = 0;
static float m_lastSoc = 100;
static unsigned long m_lastTimeSoc;
static float m_accumulatedCapacity = 0.0; // Accumulated charge or discharge capacity in ampere-hours (Ah)
static float m_minVoltage = 0;
static float m_maxVoltage = 0;

// SOH SETUP 
static float m_openCircuitVoltageValue = 0;
static float m_voltageWithLoad = 0; // voltage with load
static float m_currentWithLoad = 0; // current with laod
static float m_internalResistance = 0; 
static float m_SohEolResistance = END_OF_LIFE_INTERNAL_RESISTANCE;
static float m_computedSoh = 0;

static unsigned long lastDebounceUpdateTime = 0;  // the last time the output pin was toggled

/* Private Function Declaration */
static void getInitialSOC( float voltage );
static void updateVoltageMaxAndMin( BatteryVoltage voltageType, float voltageReading );
static float updateBatteryCapacity( void );
static BatteryVoltage getOpenCircuitVoltage( void );
static BatteryVoltage determineVoltageRange( float voltage );
static void getCurrentAndVoltageWithLoad( void );

/* Public Function Definition */
void batteryParameter_initializeSocParam( void )
{
    if( sim800lInterface_getSocCalibration() > 0 )
    {
        m_batteryCapacity = sim800lInterface_getSocCalibration();
    }
    else
    {
        m_batteryCapacity = SOC_FAIL_VALUE;
    }
    getInitialSOC( m_openCircuitVoltageValue );
    m_lastTimeSoc = millis();
}

void batteryParameter_initializeSohParam( void )
{
    m_SohEolResistance = sim800lInterface_getSohCalibration();

    // NOTE: Encountered Issue using the given formula of SOH based on End of life internal resistance
    // float endOfLifeInternalResitance = (165*LITHIUM_ION_INTERNAL_RESISTANCE)/100;
    // return ( ( m_openCircuitVoltageValue - m_voltageWithLoad)/(endOfLifeInternalResitance*m_currentWithLoad)*100 + sim800lInterface_getSohCalibration() );
    
    // Calculate SOH based on the measured and hypothetical EOL internal resistance
    m_computedSoh = 100.0 - ( ( m_internalResistance / m_SohEolResistance) * 100.0 );
    DEBUG_PRINT_LN( "m_internalResistance" + String(m_internalResistance) );
    DEBUG_PRINT_LN( "m_SohEolResistance" + String(m_SohEolResistance) );

    // Constrain SOH within the valid range (0-100%)
    m_computedSoh = constrain(m_computedSoh, 0, 100);
}

void batteryParameter_internalResistanceSetup( void )
{
    bool irValueValid = false; 
    BatteryVoltage batteryVoltageType = VOLTAGE_NONE;
    while( !irValueValid )
    {
        // Get first Open Circuit Voltage
        while ( VOLTAGE_NONE == batteryVoltageType)
        {
            batteryVoltageType = getOpenCircuitVoltage();
            // Display Release Button Now
            oledDisplay_CenterTextDisplay("Release Button");
            if( VOLTAGE_NONE == batteryVoltageType )
            {
                oledDisplay_CenterTextDisplay("Invalid Battery");
                oledDisplay_CenterTextDisplay("Try Again");
            }
        }

        //CURRENT CAPTURE AND CIRCUT VOLTAGE WITH LOAD
        getCurrentAndVoltageWithLoad();

        // Display Release Button Now
        oledDisplay_CenterTextDisplay("Release Button");

        // Display Internal Resistance
        m_internalResistance = (m_openCircuitVoltageValue-m_voltageWithLoad)/m_currentWithLoad;
        DEBUG_PRINT("Internal Resistance");
        DEBUG_PRINT_LN(m_internalResistance);

        oledDisplay_irValue(m_internalResistance);

        if( m_internalResistance > 0 )
        {
            irValueValid = true;
        }
        else
        {
            oledDisplay_CenterTextDisplay("Invalid IR");
            oledDisplay_CenterTextDisplay("Try Again");
        }
    }
}

float batteryParameter_getSoc( float currentReading, float  voltageReading )
{
    float soc = 0;
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - m_lastTimeSoc;
    float voltageFactor = 0.0;

    m_accumulatedCapacity += (currentReading * elapsedTime) / 3600000.0; // Convert milliseconds to hours
    DEBUG_PRINT_LN("m_accumulatedCapacity:" + String(m_accumulatedCapacity));
     // Calculate SOC based on accumulated capacity, battery capacity, and voltage
    if (voltageReading > m_minVoltage)
    {
        voltageFactor = ( voltageReading - m_minVoltage ) / ( m_maxVoltage - m_minVoltage );

        // Ensure voltageFactor does not exceed 1
        if (voltageFactor > 1.0)
        {
            voltageFactor = 1.0;
        }
    }

    DEBUG_PRINT_LN("voltageFactor:" + String(voltageFactor));
    soc = (m_accumulatedCapacity / m_batteryCapacity) * voltageFactor * 100.0;
    DEBUG_PRINT_LN("m_batteryCapacity:" + String(m_batteryCapacity));
    DEBUG_PRINT_LN("soc:" + String(soc));
    
    // Update the last time
    m_lastTimeSoc = currentTime;

    soc = constrain(soc, 0.0, 100.0);

    return soc;
}

float batteryParameter_getSoh( void )
{    
    return m_computedSoh;
}

float batteryParameter_getInternalResistance( void )
{
    return m_internalResistance;
}

/* Private function Definition */
static BatteryVoltage getOpenCircuitVoltage( void )
{
    bool updatingState = true;  // the current state of update
    bool lastButtonState = HIGH;
    bool buttonState = HIGH;  // Current state of the button
    BatteryVoltage batteryVoltageType = VOLTAGE_NONE;
    
    // Get Open Circuit Voltage
    while( updatingState ) 
    {
        int reading = digitalRead(GPIO_TOUCH_SENSOR_PIN);
        m_openCircuitVoltageValue = sensors_getVoltage();
        
        if( reading  != lastButtonState )
        {
            lastDebounceUpdateTime = millis();
        }

        if( m_openCircuitVoltageValue < VOLTAGE_READING_SMALLEST )
        {
            m_openCircuitVoltageValue = 0;
        }

        if( ( millis() - lastDebounceUpdateTime ) > DEBOUNCE_DELAY )
        {
            if (reading != buttonState) 
            {
                buttonState = reading;
                if ( LOW  == buttonState ) 
                {
                    // Add your code to perform an action when the button is pressed
                    updatingState = false;
                    //Determine Initial SOC
                    batteryVoltageType = determineVoltageRange(m_openCircuitVoltageValue);
                }
            }
        }
        lastButtonState = reading;
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

    return batteryVoltageType;
}

static void getCurrentAndVoltageWithLoad( void )
{
    bool updatingState = true;  // the current state of update
    bool lastButtonState = HIGH;
    bool buttonState = HIGH;  // Current state of the button
    delay( DELAY_VALUE );
    
    while (updatingState) 
    {
        int reading = digitalRead(GPIO_TOUCH_SENSOR_PIN);
        
        m_voltageWithLoad = sensors_getVoltage();
        m_currentWithLoad = sensors_getCurrentReading();

        if (m_voltageWithLoad < VOLTAGE_READING_SMALLEST)
        {
            m_voltageWithLoad = 0;
        }

        if( reading  != lastButtonState )
        {
            lastDebounceUpdateTime = millis();
        }
        
        if ((millis() - lastDebounceUpdateTime) > DEBOUNCE_DELAY) 
        {
            if (reading != buttonState) 
            {
                buttonState = reading;
                if ( LOW  == buttonState ) 
                {
                    updatingState = false;
                }
            }
        }
        lastButtonState = reading;
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
}
static float updateBatteryCapacity( void )
{
    // TODO: get initial battery capacity
    return ( m_batteryCapacity * batterySoh * BATTERY_EFFICIENCY );
}

static void getInitialSOC(float voltage)
{
    int socValue = 0;
    BatteryVoltage batteryType = VOLTAGE_NONE;
    batteryType = determineVoltageRange( voltage );
    updateVoltageMaxAndMin(batteryType, voltage);

    float initialVoltageFactor = ((voltage - m_minVoltage) / (m_maxVoltage - m_minVoltage));
    initialVoltageFactor = constrain(initialVoltageFactor, 0.0, 1.0);
    float initialSOC = initialVoltageFactor * 100.0;

    // Set the initial accumulated capacity based on the approximate SOC
    m_accumulatedCapacity = ( (initialSOC / 100.0) * m_batteryCapacity ) * ( m_computedSoh / 100.0);
}

static BatteryVoltage determineVoltageRange( float voltage )
{
    BatteryVoltage batteryType = VOLTAGE_NONE;
    if( voltage <= BATTERY_12V_MAX )
    {
        batteryType = VOLTAGE_12V;
    }
    else if( voltage > BATTERY_24V_MIN && voltage < BATTERY_24V_MAX)
    {
        batteryType = VOLTAGE_24V;
    }
    else if( voltage > BATTERY_36V_MIN && voltage < BATTERY_36V_MAX )
    { 
        batteryType = VOLTAGE_36V;
    }
    else if( voltage > BATTERY_48V_MIN && voltage < BATTERY_48V_MAX )
    { 
        batteryType = VOLTAGE_48V;
    }
    return batteryType;
}

static void updateVoltageMaxAndMin( BatteryVoltage voltageType, float voltageReading )
{
    switch( voltageType )
    {
    case VOLTAGE_12V:
        m_minVoltage = BATTERY_12_V_NOMINAL_VOLTAGE_MIN;
        m_maxVoltage = BATTERY_12_V_NOMINAL_VOLTAGE_MAX;
        break;
    case VOLTAGE_24V:
        m_minVoltage = BATTERY_24_V_NOMINAL_VOLTAGE_MIN;
        m_maxVoltage = BATTERY_24_V_NOMINAL_VOLTAGE_MAX;
        break;
    case VOLTAGE_36V:
        m_minVoltage = BATTERY_36_V_NOMINAL_VOLTAGE_MIN;
        m_maxVoltage = BATTERY_36_V_NOMINAL_VOLTAGE_MAX;
        break;
    case VOLTAGE_48V:
        m_minVoltage = BATTERY_48_V_NOMINAL_VOLTAGE_MIN;
        m_maxVoltage = BATTERY_48_V_NOMINAL_VOLTAGE_MAX;
        break;
    default:
        break;
    }
}