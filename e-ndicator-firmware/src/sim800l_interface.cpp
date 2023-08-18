#include "sim800l_interface.h"
#include "utils.h"

#define GPRS_SERIAL_BUAD_RATE ( 9600 )
#define MID_DELAY             ( 2000 )
#define INTERVAL_MESSAGE_1    ( 100 )
#define INTERVAL_MESSAGE_2    ( 1100 )
#define INTERVAL_MESSAGE1_DOWNLOAD ( 1000 )
#define INTERVAL_MESSAGE2_DOWNLOAD ( 1100 )

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

    if(millis() > timeSetupDownload + INTERVAL_MESSAGE1_DOWNLOAD)
    { 
        timeSetupDownload = millis();
        stateDownload += 1;
        
        if(stateDownload == 1)
        { // @100ms
            gprsSerial.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
        }

        if(stateDownload == 2)
        { // @200ms
            gprsSerial.println("AT+SAPBR=3,1,\"APN\",\"smartlte\"");
        }

        if(stateDownload == 3)
        { // @300ms
            gprsSerial.println("AT+SAPBR=3,1,\"USER\",\"\"");
        }

        if(stateDownload == 4)
        { // @400ms
            gprsSerial.println("AT+SAPBR=3,1,\"PWD\",\"\"");
        }

        if(stateDownload == 5)
        { // @500ms
            gprsSerial.println("AT+SAPBR=1,1");
        }

        if(stateDownload == 6)
        { // @600ms
            gprsSerial.println("AT+SAPBR=2,1");
        }

        if(stateDownload == 7)
        { // @700ms
            gprsSerial.println("AT+HTTPINIT");
        }

        if(stateDownload == 8)
        { //@800ms
            gprsSerial.println("AT+HTTPPARA=\"CID\",1");
        }

        if(stateDownload == 9)
        { //@900ms
            gprsSerial.println(sim_str);
        }

        if(stateDownload == 10)
        { //@1000ms
            gprsSerial.println("AT+HTTPACTION=0");//get local IP adress
        }

        if(stateDownload == 11)
        { //@1100ms
            gprsSerial.println("AT+HTTPREAD");//get local IP adress

        }

        if(stateDownload == 12)
        { //@1200ms
            gprsSerial.println("AT+HTTPTERM");
            gprsSerial.println("AT+SAPBR=0,1");
            stateDownload = 0;
        }

        ShowSerialData();
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
