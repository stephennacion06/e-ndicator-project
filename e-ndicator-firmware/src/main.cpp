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

extern batteryCalibration g_batteryCalibrationData;

void setup()
{
  Serial.begin(9600);
  
  // sim800lInterface_gprsSerialInitialize();

  oledDisplay_initialize();
  
  // DOWNLOAD PARAMATERS FROM SERVER
  oledDisplay_downloadDisplay();
  sim800Interface_downloadFromServer( USER_ID ); 

  // IF Download Parameter failed start WiFi manager
  if( 0 == sim800lInterface_getVoltageCalibration() )
  {
    // Display Wifi Manager Started
    oledDisplay_CenterTextDisplay("Trying WiFi");
    
    // Launch WiFi Manager
    wifiRes = wm.autoConnect("Endicator Wifi","password"); // password protected ap

    if(!wifiRes) 
    {
      // TODO: Restart ESP32 and Dispaly failed wifi connection
      ESP.restart();
    }
    else
    {
      // TODO: WIFI DOWNLOAD DISPLAY
      wifiEnabled = true;
      oledDisplay_CenterTextDisplay("WiFi Connected");
      sim800Interface_wifiDownload( USER_ID );
      // Print the parsed values
      DEBUG_PRINT("Voltage Calibration: ");
      DEBUG_PRINT_LN(g_batteryCalibrationData.voltageCalibration, 2); // Print with 2 decimal places
      DEBUG_PRINT("Current Calibration: ");
      DEBUG_PRINT_LN(g_batteryCalibrationData.currentCalibration, 2); // Print with 2 decimal places
      DEBUG_PRINT("SOH Calibration: ");
      DEBUG_PRINT_LN(g_batteryCalibrationData.sohCalibration, 2); // Print with 2 decimal places
      DEBUG_PRINT("SOC Calibration: ");
      DEBUG_PRINT_LN(g_batteryCalibrationData.socCalibration, 2); // Print with 2 decimal places
      oledDisplay_CenterTextDisplay("Download Done");
    }
  } 
  sensors_initializeAllSensors();
  xTaskCreate(sensors_voltageCurrentTask, "VoltageCurrentTask", 2048 , NULL, 1, NULL);
  batteryParameter_internalResistanceSetup();
  batteryParameter_initializeSohParam();
  // NOTE: Initialize first SOH before SOC parameters
  batteryParameter_initializeSocParam();
}

void loop()
{
  //get Voltage
  float voltageReading = sensors_getVoltage();
  
  //get Current
  float currentReading = sensors_getCurrentReading();
  
  // get SOC
  float batterySoc = batteryParameter_getSoc( currentReading, voltageReading );

  // get SOH
  float batterySoh = batteryParameter_getSoh();

  // get Internal Resistance
  float internalResistance =  batteryParameter_getInternalResistance();

  if( voltageReading < MINIMUM_VOLTAGE )
  {
    voltageReading = 0;
    currentReading = 0;
  }

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