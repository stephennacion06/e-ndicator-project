#include <Arduino.h>
#include "user.h"
#include "sim800l_interface.h"
#include "sensors.h"
#include "battery_parameter.h"
#include "oled_display.h"
#include "utils.h"

#define MINIMUM_VOLTAGE ( 4 )

void setup()
{
  Serial.begin(9600);

  sim800lInterface_gprsSerialInitialize();

  oledDisplay_initialize();
  
  //DOWNLOAD PARAMATERS FROM SERVER
  oledDisplay_downloadDisplay();
  while( 0 == sim800lInterface_getVoltageCalibration() ) 
  {
    sim800Interface_downloadFromServer( USER_ID ); 
  }
  
  batteryParameter_internalResistanceSetup();
}

void loop()
{
  //get Voltage
  float voltageReading = sensors_getVoltage();
  
  // TODO: Investigate why _2 is used
  //get Current
  float currentReading = sensors_getCurrent_2();

  if( voltageReading < MINIMUM_VOLTAGE )
  {
    voltageReading = 0;
  }
  
  // get SOC
  float batterySoc = batteryParameter_getSoc( currentReading );

  // get SOH
  float batterySoh = batteryParameter_getSoh();

  // get Internal Resistance
  float internalResistance =  batteryParameter_getInternalResistance();

  oledDisplay_showParameters( voltageReading, currentReading , batterySoh, batterySoc );
  sim800lInterface_transmitToServer( USER_ID, voltageReading, currentReading, batterySoh, batterySoc, internalResistance );
}