#include <Arduino.h>
#include <String.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "user.h"
#include "sim800l_interface.h"
#include "sensors.h"

// SOC PARAMETERS
float coulomb_count = 0; // A
float hour_converter = 0.00028; // h
float battery_capacity = 1000; // Ah
float actual_capacity = 0;
float battery_efficiency = 0.9;
float battery_soh = 0.99;
float battery_soc = 0;
float last_soc = 100;
int charge_state = -1;

//SOH SETUP 
int touch_pin = 90; // initial value of touch sensor
float ocv = 0;//open circuit voltage value
float vbat = 0; // voltage with load
float cbat = 0; // current with laod
float internal_resistance = 0; 

float rinit = 0.05; //0.01 ohms for lithium ion typical initial resitance
float reol = 320; //320 ohms for lithion ion typical end of life resistance

int ledState = true;         // the current state of the output pin
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 100;    // the debounce time; increase if the output flickers
#define touch_threshold 30

//OLED 
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void ir_setup();
float sensors_getCurrent();
int determine_battery_type(float voltage);
void AllPixels();
void TextDisplay();
void DownloadDisplay();
void InvertedTextDisplay();
void InternalResistanceSetup();
void IR_value(float ir);
void Display_parameters(float voltage, float current, float test,  float battery_soh);
void ir_setup_display(float ocv, float vbat, float cbat);

void setup()
{
  Serial.begin(9600);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  gprsSerialInitialize();

  display.clearDisplay();
  display.display();
  delay(1000);

  AllPixels();
  TextDisplay();
  InvertedTextDisplay();
  InternalResistanceSetup();

  //COMPUTE ACTUAL BATTERY CAPACITY
  battery_capacity = battery_capacity * battery_soh * battery_efficiency;

  //DOWNLOAD PARAMATERS FROM SERVER
  DownloadDisplay();
  while( 0 == getVoltageCalibration() ) 
  {
    sim800Interface_downloadFromServer( USER_ID ); 
  }
  
  ir_setup();
  

}

void loop()
{

  //get Voltage
  float voltageReading = sensors_getVoltage();
  //get Current
  float currentReading = sensors_getCurrent_2();

  if (voltageReading < 4)
  {
    voltageReading = 0;
  }

  //compute SOC
  battery_soc = last_soc  + charge_state*( currentReading / battery_capacity )*0.000277778*100 + getSocCalibration();

  //compute SOH
  reol = (165*rinit)/100;
  battery_soh = (ocv - vbat)/(reol*cbat)*100 + getSohCalibration();


  Display_parameters( voltageReading, currentReading , battery_soc,battery_soh);
  transmit_to_server( USER_ID, voltageReading, currentReading, battery_soh, battery_soc, internal_resistance );


}

void ir_setup()
{
  //Determine Initial SOC
  last_soc = determine_battery_type(ocv);
  
  while (ledState) 
  {
          touch_pin = touchRead(T3);

          if (touch_pin > touch_threshold) {
            lastDebounceTime = millis();
          }
          if ((millis() - lastDebounceTime) > debounceDelay) {
            ledState = false;
          }
          ocv = sensors_getVoltage();
          
         
          if (ocv < 4){
            ocv = 0;
          }
          
          ir_setup_display(ocv, vbat, cbat);
          Serial.print("OCV:");
          Serial.print(ocv);
          Serial.println("V");
  
  }
  Serial.print("*****************");
  Serial.println("");
  Serial.print("OCV:");
  Serial.print(ocv);
  Serial.println("V");

  //CURRENT CAPTURE AND CIRCUT VOLTAGE
  touch_pin = 90;
  ledState = true;
  delay(2500);
  
  while (ledState) 
  {
  touch_pin = touchRead(T3);
  vbat = sensors_getVoltage();
  cbat = sensors_getCurrent();

           if (touch_pin > touch_threshold) {
            lastDebounceTime = millis();
          }
          if ((millis() - lastDebounceTime) > debounceDelay) {
            ledState = false;
          }
  if (vbat < 4){
    vbat = 0;
  }
  
  ir_setup_display(ocv, vbat, cbat);
  Serial.print("vbat:");
  Serial.print(vbat);
  Serial.print("V");
  Serial.print("; vbat:");
  Serial.print(cbat);
  Serial.println("A");
  
  }
  Serial.println("*****************");
  Serial.print("vbat:");
  Serial.print(vbat);
  Serial.print("V");
  Serial.print("; cbat:");
  Serial.print(cbat);
  Serial.println("A");
  touch_pin=90;

  internal_resistance = (ocv-vbat)/cbat;
  Serial.print("Internal Resistance");
  Serial.println(internal_resistance);
  IR_value(internal_resistance);
}

int determine_battery_type(float voltage){

float updated_soc;
  if(voltage <= 14.4){
    
    updated_soc = map(voltage,11.51,12.73,0,100);
    
    if(updated_soc < 0){
      updated_soc=10;
    }
    
    return updated_soc ;
  }
  else if(voltage > 14.5 && voltage < 26.4)
  {
    updated_soc = map(voltage, 23.02, 25.46,0,100);
    
    if(updated_soc < 0){
      updated_soc=10;
    }
    
    return updated_soc;
  }
  else if(voltage > 26.5 && voltage < 38.4)
  { 
    updated_soc = map(voltage, 35.02, 38.4,0,100);
    if(updated_soc < 0){
      updated_soc=10;
    }
    
    return updated_soc;
  }
   else if(voltage > 38.5)
  { 
    updated_soc = map(voltage, 46.04, 50.92,0,100);

    if(updated_soc < 0){
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

    if(updated_soc < 0){
      updated_soc=10;
    }
    else if(updated_soc > 100)
    {
      updated_soc=100;
    }
    
    return updated_soc;
  }

  
}




void AllPixels()
{
  int i;
  int j;
  display.clearDisplay();
  for(i=0;i<64;i++)
  {
    for(j=0;j<128;j++)
    {
      display.drawPixel(j, i, SSD1306_WHITE);
      
    }
    display.display();
    delay(30);
  }
  
  for(i=0;i<64;i++)
  {
    for(j=0;j<128;j++)
    {
      display.drawPixel(j, i, SSD1306_BLACK);
      
    }
    display.display();
    delay(30);
  }
  
}

void TextDisplay()
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(24,28);
  display.println("IoT BMS");
  display.display();
  delay(3000);
}
void DownloadDisplay()
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(24,28);
  display.println("DOWNLOADING");
  display.display();
  delay(3000);
}


void InvertedTextDisplay()
{
  display.clearDisplay();
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.setCursor(24,28);
  display.println("IoT BMS");
  display.display();
  delay(3000);
}



void InternalResistanceSetup()
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(24,28);
  display.println("IR SETUP");
  display.display();
  delay(3000);
}

void IR_value(float ir)
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(24,28);
  display.print("IR= ");
  display.print(ir);
  display.println(" ohms");
  display.display();
  delay(3000);
}

void Display_parameters(float voltage, float current, float test,  float battery_soh)
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  
  display.print(F("Voltage: "));
  display.print(voltage,1);
  display.println(F(" V"));

  display.print(F("Current: "));
  display.print(current);
  display.println(F(" A"));

  if(voltage < 4){
    battery_soc = 0.0;
    battery_soh = 0.0;
  }

  if(battery_soc == 0){
    battery_soh;
  }

  
  display.setTextSize(2);
  display.print(F("SOC: "));
  display.print(battery_soc,0);
  display.println(F(" %"));

  display.print(F("SOH: "));
  display.print(battery_soh,0);
  display.println(F(" %"));

  


  display.display();

}

void ir_setup_display(float ocv, float vbat, float cbat)
{
  display.clearDisplay();
  display.setTextSize(1.5);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  
  display.print(F("OCV: "));
  display.print(ocv,1);
  display.println(F(" V"));

  display.print(F("VBAT: "));
  display.print(vbat,1);
  display.println(F(" V"));


  display.print(F("CBAT: "));
  display.print(cbat);
  display.println(F(" A"));

  display.display();
}
