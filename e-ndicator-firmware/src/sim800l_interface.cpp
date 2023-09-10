#include "sim800l_interface.h"
#include "utils.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define GPRS_SERIAL_BUAD_RATE ( 9600 )
#define MID_DELAY             ( 2000 )
#define INTERVAL_MESSAGE_1    ( 100 )
#define INTERVAL_MESSAGE_2    ( 1100 )
#define INTERVAL_MESSAGE1_DOWNLOAD ( 1000 )
#define INTERVAL_MESSAGE2_DOWNLOAD ( 1100 )
#define COMMON_DELAY               ( 1000 )

static SoftwareSerial gprsSerial(27,26);

static unsigned long timeSetupDownload = 0;
static int stateDownload = 0;

static float voltageCalibration = 0;
static float currentCalibration = 0;
static float sohCalibration = 0;
static float socCalibration = 0;
static unsigned long time1 = 0;
static unsigned long time2 = 0;
static int position1 = 0;

/* Private Function Declaration*/
static void downloadParamaters( void );
static void ShowSerialData( void );

/* Public Function Definition */
void sim800lInterface_gprsSerialInitialize( void )
{
    gprsSerial.begin( GPRS_SERIAL_BUAD_RATE );

    delay( COMMON_DELAY );

    DEBUG_PRINT_LN( "Initializing... SIM800l" );
    gprsSerial.println("AT");

    delay( COMMON_DELAY );

    if( gprsSerial.find("OK") )
    {
        DEBUG_PRINT_LN("SIM800L ready");

        // Set APN and establish GPRS connection
        gprsSerial.println("AT+SAPBR=3,1,\"Contype\",\"GPRS\"");
        delay(COMMON_DELAY);
        gprsSerial.println("AT+SAPBR=3,1,\"APN\",\"smartlte\"");
        delay(COMMON_DELAY);
        gprsSerial.println("AT+SAPBR=1,1");
        delay(COMMON_DELAY);
    } 
    else 
    {
        // TODO: Update failed sim800l flag 
        DEBUG_PRINT_LN("Error: SIM800L not responding");
        while (1);
    }
}

float sim800lInterface_getVoltageCalibration( void )
{
    return voltageCalibration;
}

float sim800lInterface_getCurrentCalibration( void )
{
    return currentCalibration;
}

float sim800lInterface_getSohCalibration( void )
{
    return sohCalibration;
}

float sim800lInterface_getSocCalibration( void )
{
    return socCalibration;
}

void sim800Interface_downloadFromServer(int user_num)
{
    String str= "http://endicatorapp.pythonanywhere.com/api/v1/batteries/get_values?user=" + String(user_num);
    String sim_str = "AT+HTTPPARA=\"URL\",\"" + str + "\"";

    // Initialize HTTP service
    gprsSerial.println("AT+HTTPINIT");
    delay(COMMON_DELAY);
    // Set URL
    gprsSerial.println("AT+HTTPPARA=\"CID\",1");
    delay(COMMON_DELAY);
    
    gprsSerial.println(sim_str);
    delay(COMMON_DELAY);
    
    // Initiate GET request
    gprsSerial.println("AT+HTTPACTION=0");
    delay(3000);
    
    // Read HTTP response
    gprsSerial.println("AT+HTTPREAD");
    delay(COMMON_DELAY);
    
    // Download Parameer from Server
    downloadParamaters();

    // Terminate HTTP service
    gprsSerial.println("AT+HTTPTERM");
    delay(COMMON_DELAY);
    
    // Disable GPRS context
    gprsSerial.println("AT+SAPBR=0,1");
    delay(COMMON_DELAY);
    
    while (gprsSerial.available()) {
        char c = gprsSerial.read();
        Serial.write(c);
    }
}

void sim800lInterface_transmitToServer(int user_num, float voltage, float current, float soh, float soc, float internalResistance)
{
    if (soc == 0)
    {
        soh = 0;
    }
    String str= "http://endicatorapp.pythonanywhere.com/api/v1/batteries/update?user=" 
    + String(user_num) 
    + "&field1=" + String(voltage) 
    + "&field2=" + String(current) 
    + "&field3=" + String(soc)  
    + "&field4=" + String(soh)
    + "&field5=" + String(internalResistance);
    String sim_str = "AT+HTTPPARA=\"URL\",\"" + str + "\"";

    if(millis() > time1 + INTERVAL_MESSAGE_1)
    { 
        time1 = millis();
        position1 = position1 + 1;
        
        if(position1 == 1)
        { // @100ms
            gprsSerial.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
        }

        if(position1 == 2)
        { // @200ms
            gprsSerial.println("AT+SAPBR=3,1,\"APN\",\"smartlte\"");
        }

        if(position1 == 3)
        { // @300ms
            gprsSerial.println("AT+SAPBR=3,1,\"USER\",\"\"");
        }

        if(position1 == 4)
        { // @400ms
            gprsSerial.println("AT+SAPBR=3,1,\"PWD\",\"\"");
        }

        if(position1 == 5)
        { // @500ms
            gprsSerial.println("AT+SAPBR=1,1");
        }

        if(position1 == 6)
        { // @600ms
            gprsSerial.println("AT+SAPBR=2,1");
        }

        if(position1 == 7)
        { // @700ms
            gprsSerial.println("AT+HTTPINIT");
        }

        if(position1 == 8)
        { //@800ms
            gprsSerial.println("AT+HTTPPARA=\"CID\",1");
        }

        if(position1 == 9)
        { //@900ms
            gprsSerial.println(sim_str);
        }

        if(position1 == 10)
        { //@1000ms
            gprsSerial.println("AT+HTTPACTION=0");//get local IP adress
        }

        if(position1 == 12)
        { //@1200ms
            gprsSerial.println("AT+HTTPTERM");
            position1 = 0;
        }
    }

    if(millis() > time2 + INTERVAL_MESSAGE_2)
    { 
        time2 = millis();
        ShowSerialData();
    }
}

void sim800Interface_wifiTransmission( int user_num, float voltage, float current, float soh, float soc, float internalResistance )
{
    // Serialize JSON to a string and Create a JSON object
    HTTPClient http;
    String strUrl = "";
    if (soc == 0)
    {
        soh = 0;
    }
    strUrl = "http://endicatorapp.pythonanywhere.com/api/v1/batteries/update?user=" 
    + String(user_num) 
    + "&field1=" + String(voltage) 
    + "&field2=" + String(current) 
    + "&field3=" + String(soc)  
    + "&field4=" + String(soh)
    + "&field5=" + String(internalResistance);
    
    http.begin(strUrl); // Specify the URL to access
    int httpResponseCode = http.GET();
    if(httpResponseCode > 0) 
    {
        DEBUG_PRINT("HTTP Response Code: ");
        DEBUG_PRINT_LN(httpResponseCode);

        // If a successful response is received, you can read the content
        String payload = http.getString();
        DEBUG_PRINT_LN("Response:");
        DEBUG_PRINT_LN(payload);
    } 
    else 
    {
        DEBUG_PRINT("Error on HTTP request. Response Code: ");
        DEBUG_PRINT_LN(httpResponseCode);
    }
}

/* Private Function Definition */
static void downloadParamaters( void )
{ 
    int i=0;
    char *array_var[3]; // Number of variables to save
    char *token;
    const char del[] = "[],"; //Delimiters
    String textMessage;
    while( gprsSerial.available()!=0 )
    {
        textMessage = gprsSerial.readString();
        textMessage.remove(0, 98);  // greeting now contains "heo"
        textMessage.remove(textMessage.length()-7, 7);
        DEBUG_PRINT_LN(textMessage);
        char received_message[textMessage.length() + 1]; 
        textMessage.toCharArray(received_message, textMessage.length() + 1);
        DEBUG_PRINT_LN(received_message);
        token = strtok(received_message, del);
        while( token != NULL ) 
        {
            i++;
            DEBUG_PRINT_LN(i);
            if(i == 1)
            {
                voltageCalibration = atof(token);
            }
        else if ( i == 2 )
        {
            currentCalibration = atof(token);
        }
        else if (i == 3)
        {
            sohCalibration = atof(token);
        }
        else if (i == 4)
        {
            socCalibration = atof(token);
        }
        token = strtok(NULL, del);
    }
    DEBUG_PRINT_LN("Saved");
    DEBUG_PRINT("VOLTAGE CALIBRATION: ");
    DEBUG_PRINT_LN(voltageCalibration);
    DEBUG_PRINT("CURRENT CALIBRATION: ");
    DEBUG_PRINT_LN(currentCalibration);
    DEBUG_PRINT("SOH CALIBRATION: ");
    DEBUG_PRINT_LN(sohCalibration);
    DEBUG_PRINT("SOC CALIBRATION: ");
    DEBUG_PRINT_LN(socCalibration);
    }
}

static void ShowSerialData( void )
{
    while(gprsSerial.available()!=0)
    {
        Serial.write(gprsSerial.read());
    }
}
