//SIM800l
int user = 1;//KEVIN USER
#include <SoftwareSerial.h>
SoftwareSerial gprsSerial(27,26);
#include <String.h>
#define delay_sms 100
#define INTERVAL_MESSAGE1 100
#define INTERVAL_MESSAGE2 1100
unsigned long time_1 = 0;
unsigned long time_2 = 0;
int position_1 = 0;


#define INTERVAL_MESSAGE1_DOWNLOAD 1000
#define INTERVAL_MESSAGE2_DOWNLOAD 1100
unsigned long time_1_DOWNLOAD = 0;
unsigned long time_2_DOWNLOAD = 0;
int position_1_DOWNLOAD = 0;

const char start_char = '[';
const char end_char = ']';
const char del[] = "[],"; //Delimiters
float voltage_cal = 0;
float current_cal = 0;
float SOH_cal = 0;
float SOC_cal = 0;




//VOLTAGE AND CURRENT
#include <SimpleKalmanFilter.h>
float r1 = 100000;
float r2 = 5000;
float voltage_value = 0;
float ADC_VALUE;
const long SERIAL_REFRESH_TIME = 100;
long refresh_time;
SimpleKalmanFilter simpleKalmanFilter_voltage(2, 2, 0.1);
SimpleKalmanFilter simpleKalmanFilter_current(2, 2, 0.01);
const int sensorIn = 35;      // pin where the OUT pin from sensor is connected on Arduino
int mVperAmp = 185;           // this the 5A version of the ACS712 -use 100 for 20A Module and 66 for 30A Module
int Watt = 0;
double Voltage = 0;
double VRMS = 0;
double AmpsRMS = 0;
float current;


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
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(9600);
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  gprsSerial.begin(9600);
  
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
  while(voltage_cal == 0){
   download_from_server(user); 
  }
  
  ir_setup();
  

}

void loop() {

  //get voltage
  float voltage = get_voltage();
  if (voltage < 4)
  {
    voltage = 0;
  }
  //get current
  current = get_current_2();

  //compute SOC
  battery_soc = last_soc  + charge_state*(current/battery_capacity)*0.000277778*100 + SOC_cal;

  //compute SOH
  reol = (165*rinit)/100;
  battery_soh = (ocv - vbat)/(reol*cbat)*100 + SOH_cal;


 Display_parameters(voltage,current, battery_soc,battery_soh);
 transmit_to_server(user,voltage,current, battery_soh, battery_soc, internal_resistance);


}


void ShowSerialData()
{
  while(gprsSerial.available()!=0)
  Serial.write(gprsSerial.read());
  
}


void download_from_server(int user_num)
{
  String str= "http://endicatorapp.pythonanywhere.com/api/v1/batteries/get_values?user=" + String(user_num);
  String sim_str = "AT+HTTPPARA=\"URL\",\"" + str + "\"";
   
    if(millis() > time_1_DOWNLOAD + INTERVAL_MESSAGE1_DOWNLOAD){ 
        time_1_DOWNLOAD = millis();
        position_1_DOWNLOAD = position_1_DOWNLOAD + 1;
        
        if(position_1_DOWNLOAD == 1){ // @100ms
          gprsSerial.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
        }

         if(position_1_DOWNLOAD == 2){ // @200ms
          gprsSerial.println("AT+SAPBR=3,1,\"APN\",\"smartlte\"");
        }

        if(position_1_DOWNLOAD == 3){ // @300ms
          gprsSerial.println("AT+SAPBR=3,1,\"USER\",\"\"");
        }


        if(position_1_DOWNLOAD == 4){ // @400ms
          gprsSerial.println("AT+SAPBR=3,1,\"PWD\",\"\"");
        }

        if(position_1_DOWNLOAD == 5){ // @500ms
          gprsSerial.println("AT+SAPBR=1,1");
        }


        if(position_1_DOWNLOAD == 6){ // @600ms
          gprsSerial.println("AT+SAPBR=2,1");
        }

        if(position_1_DOWNLOAD == 7){ // @700ms
          gprsSerial.println("AT+HTTPINIT");
        }


        if(position_1_DOWNLOAD == 8){ //@800ms
          gprsSerial.println("AT+HTTPPARA=\"CID\",1");
        }

        if(position_1_DOWNLOAD == 9){ //@900ms
          gprsSerial.println(sim_str);
        }


        if(position_1_DOWNLOAD == 10){ //@1000ms
          gprsSerial.println("AT+HTTPACTION=0");//get local IP adress
          delay(2000);

        }


        if(position_1_DOWNLOAD == 11){ //@1100ms
          gprsSerial.println("AT+HTTPREAD=0,81");//get local IP adress
          ShowDownload();
        }

        if(position_1_DOWNLOAD == 12){ //@1200ms
          gprsSerial.println("AT+HTTPTERM");
          position_1_DOWNLOAD = 0;
        }

    }
  
}


void ShowDownload()
{ 
  int i=0;
  char *array_var[3]; // Number of variables to save
  char *token;
  String textMessage;
  while(gprsSerial.available()!=0) {

      textMessage = gprsSerial.readString();
      textMessage.remove(0, 98);  // greeting now contains "heo"
      textMessage.remove(textMessage.length()-7, 7);
      Serial.println(textMessage);
      char received_message[textMessage.length() + 1]; 
      textMessage.toCharArray(received_message, textMessage.length() + 1);
      Serial.println(received_message);
      token = strtok(received_message, del);
      while( token != NULL ) 
   {
     i++;
     Serial.println(i);
     if(i == 1){
      voltage_cal = atof(token);
     }
     else if ( i == 2){
      current_cal = atof(token);
    }
     else if (i == 3){
      SOH_cal = atof(token);
    }

    else if (i == 4){
      SOC_cal = atof(token);
    }

      token = strtok(NULL, del);
    }

    Serial.println("Saved");
    Serial.print("VOLTAGE CALIBRATION: ");
    Serial.println(voltage_cal);
    Serial.print("CURRENT CALIBRATION: ");
    Serial.println(current_cal);
    Serial.print("SOH CALIBRATION: ");
    Serial.println(SOH_cal);
    Serial.print("SOC CALIBRATION: ");
    Serial.println(SOC_cal);

  }
}


void transmit_to_server(int user_num, float voltage, float current, float soh, float soc, float internal_resistance)
{
  if (soc == 0){
    soh = 0;
  }
  String str= "http://endicatorapp.pythonanywhere.com/api/v1/batteries/update?user=" 
  + String(user_num) 
  + "&field1=" + String(voltage) 
  + "&field2=" + String(current) 
  + "&field3=" + String(soc)  
  + "&field4=" + String(soh)
  + "&field5=" + String(internal_resistance);
  String sim_str = "AT+HTTPPARA=\"URL\",\"" + str + "\"";

   if (gprsSerial.available())
    Serial.write(gprsSerial.read());
   
    if(millis() > time_1 + INTERVAL_MESSAGE1){ 
        time_1 = millis();
        position_1 = position_1 + 1;
        
        if(position_1 == 1){ // @100ms
          gprsSerial.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
        }

         if(position_1 == 2){ // @200ms
          gprsSerial.println("AT+SAPBR=3,1,\"APN\",\"smartlte\"");
        }

        if(position_1 == 3){ // @300ms
          gprsSerial.println("AT+SAPBR=3,1,\"USER\",\"\"");
        }


        if(position_1 == 4){ // @400ms
          gprsSerial.println("AT+SAPBR=3,1,\"PWD\",\"\"");
        }

        if(position_1 == 5){ // @500ms
          gprsSerial.println("AT+SAPBR=1,1");
        }


        if(position_1 == 6){ // @600ms
          gprsSerial.println("AT+SAPBR=2,1");
        }

        if(position_1 == 7){ // @700ms
          gprsSerial.println("AT+HTTPINIT");
        }


        if(position_1 == 8){ //@800ms
          gprsSerial.println("AT+HTTPPARA=\"CID\",1");
        }

        if(position_1 == 9){ //@900ms
          gprsSerial.println(sim_str);
        }


        if(position_1 == 10){ //@1000ms
          gprsSerial.println("AT+HTTPACTION=0");//get local IP adress
        }


        if(position_1 == 12){ //@1200ms
          gprsSerial.println("AT+HTTPTERM");
          position_1 = 0;
        }

    }


   if(millis() > time_2 + INTERVAL_MESSAGE2){ 
        time_2 = millis();
        ShowSerialData();
      }
  
}








void ir_setup()
{
  //Determine Initial SOC
  last_soc = determine_battery_type(ocv) ;
  
  while (ledState) 
  {
          touch_pin = touchRead(T3);

          if (touch_pin > touch_threshold) {
            lastDebounceTime = millis();
          }
          if ((millis() - lastDebounceTime) > debounceDelay) {
            ledState = false;
          }
          
          
          
          ocv = get_voltage();
          
         
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
  vbat = get_voltage();
  cbat = get_current();

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

float get_voltage()
{
    
ADC_VALUE = analogRead(34);
 voltage_value = ((ADC_VALUE * 3.3 ) / (4095))
                  /
                 (r2/(r1+r2));
 float estimated_value = simpleKalmanFilter_voltage.updateEstimate(voltage_value);
 return estimated_value + voltage_cal;
}

float get_current()
{
//  float I = sensor.getCurrentDC();
  Voltage = getVPP();
  VRMS = (Voltage/2.0) *0.707;   //root 2 is 0.707
  AmpsRMS = ((VRMS * 1000)/mVperAmp)-0.25; //0.3 is the error I got for my sensor
  float estimated_value = simpleKalmanFilter_current.updateEstimate(AmpsRMS);
  return estimated_value + current_cal;
}
float getVPP()
{
  float result;
  int readValue;                // value read from the sensor
  int maxValue = 0;             // store max value here
  int minValue = 4096;          // store min value here ESP32 ADC resolution
  
   uint32_t start_time = millis();

     while((millis()-start_time) < 700) //sample for 700 mSec
   {
       readValue = analogRead(sensorIn);
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

float get_current_2()
{
//  float I = sensor.getCurrentDC();
  Voltage = getVPP_2();
  VRMS = (Voltage/2.0) *0.707;   //root 2 is 0.707
  AmpsRMS = ((VRMS * 1000)/mVperAmp)-0.25; //0.3 is the error I got for my sensor
  float estimated_value = simpleKalmanFilter_current.updateEstimate(AmpsRMS);
  return estimated_value + current_cal;
}

float getVPP_2()
{
  float result;
  int readValue;                // value read from the sensor
  int maxValue = 0;             // store max value here
  int minValue = 4096;          // store min value here ESP32 ADC resolution
  
   uint32_t start_time = millis();

     while((millis()-start_time) < 10) //sample for 700 mSec
   {
       readValue = analogRead(sensorIn);
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
