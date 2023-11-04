#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include "user.h"
#include "sim800l_interface.h"
#include "sensors.h"
#include "battery_parameter.h"
#include "oled_display.h"
#include "utils.h"
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <WiFi.h>
#include <AsyncTCP.h>

#define MINIMUM_VOLTAGE ( 4 )

// Wifi Manager local Initialization
static bool wifiRes;
static bool wifiEnabled = false;
static WiFiManager wm;

extern batteryCalibration g_batteryCalibrationData;

AsyncWebServer server(80);

static void startOTA( void );

void setup()
{
  Serial.begin(9600);
  // Initialize Sensors
  sensors_initializeAllSensors();
  oledDisplay_initialize();

#if CALIBRATION_ENABLED == 0
  sim800lInterface_gprsSerialInitialize();

  // DOWNLOAD PARAMATERS FROM SERVER

  oledDisplay_downloadDisplay();
  sim800Interface_downloadFromServer( USER_ID ); 
#endif // CALIBRATION_ENABLED

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
      
      // Display OTA
      startOTA();
      oledDisplay_CenterTextDisplay("OTA Enabled");
      oledDisplay_CenterTextDisplay( WiFi.localIP().toString() );
    }
  }
  xTaskCreate(sensors_voltageCurrentTask, "VoltageCurrentTask", 2048 , NULL, 1, NULL);

#if CALIBRATION_ENABLED == 1
  batteryParameter_Calibration();
#endif // CALIBRATION_ENABLED

  // Start Internal Resistance Setup
  oledDisplay_internalResistanceSetupDisplay();
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

static void startOTA( void )
{
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", "Hi! This is a sample response.");
    });

    AsyncElegantOTA.begin(&server);    // Start AsyncElegantOTA
    server.begin();
}