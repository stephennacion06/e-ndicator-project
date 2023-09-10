#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include "user.h"
#include "sim800l_interface.h"
#include "sensors.h"
#include "battery_parameter.h"
#include "oled_display.h"
#include "utils.h"


#define MINIMUM_VOLTAGE ( 4 )

// Wifi Manager local Initialization
static bool wifiRes;
static bool wifiEnabled = false;
static WiFiManager wm;

void setup()
{
  Serial.begin(9600);

  sim800lInterface_gprsSerialInitialize();

  oledDisplay_initialize();
  
  // DOWNLOAD PARAMATERS FROM SERVER
  oledDisplay_downloadDisplay();
  sim800Interface_downloadFromServer( USER_ID ); 

  // IF Download Parameter failed start WiFi manager
  if( 0 == sim800lInterface_getVoltageCalibration() )
  {
    // TODO: Display Wifi Manager Started

    // Launch WiFi Manager
    wifiRes = wm.autoConnect("Endicator Wifi","password"); // password protected ap

    if(!wifiRes) 
    {
      // TODO: Restart ESP32 and Dispaly failed wifi connection
      ESP.restart();
    }
    else
    {
      wifiEnabled = true;
    }
  } 
  
  // batteryParameter_internalResistanceSetup();
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
  if(wifiEnabled)
  {
    sim800Interface_wifiTransmission( USER_ID, voltageReading, currentReading, batterySoh, batterySoc, internalResistance );
  }
  else
  { 
    sim800lInterface_transmitToServer( USER_ID, voltageReading, currentReading, batterySoh, batterySoc, internalResistance );
  }
}