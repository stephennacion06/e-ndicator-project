#include "sim800l_interface.h"

#define GPRS_SERIAL_BUAD_RATE ( 9600 )

static SoftwareSerial gprsSerial(27,26);

static unsigned long timeDownload1 = 0;
static int positionDownload1 = 0;

static float voltageCalibration = 0;
static float currentCalibration = 0;
static float sohCalibration = 0;
static float socCalibration = 0;
static unsigned long time1 = 0;
static unsigned long time2 = 0;
static int position1 = 0;

/* Private Function Declaration*/
static void ShowDownload( void );

/* Public Function Definition */
void gprsSerialInitialize( void )
{
    gprsSerial.begin( GPRS_SERIAL_BUAD_RATE );
}

float getVoltageCalibration( void )
{
    return voltageCalibration;
}

float getCurrentCalibration( void )
{
    return currentCalibration;
}

float getSohCalibration( void )
{
    return sohCalibration;
}

float getSocCalibration( void )
{
    return socCalibration;
}

void ShowSerialData()
{
    while(gprsSerial.available()!=0)
    {
        Serial.write(gprsSerial.read());
    }
}

void sim800Interface_downloadFromServer(int user_num)
{
    String str= "http://endicatorapp.pythonanywhere.com/api/v1/batteries/get_values?user=" + String(user_num);
    String sim_str = "AT+HTTPPARA=\"URL\",\"" + str + "\"";

    if(millis() > timeDownload1 + INTERVAL_MESSAGE1_DOWNLOAD)
    { 
        timeDownload1 = millis();
        positionDownload1 = positionDownload1 + 1;
        
        if(positionDownload1 == 1)
        { // @100ms
            gprsSerial.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
        }

        if(positionDownload1 == 2)
        { // @200ms
            gprsSerial.println("AT+SAPBR=3,1,\"APN\",\"smartlte\"");
        }

        if(positionDownload1 == 3)
        { // @300ms
            gprsSerial.println("AT+SAPBR=3,1,\"USER\",\"\"");
        }

        if(positionDownload1 == 4)
        { // @400ms
            gprsSerial.println("AT+SAPBR=3,1,\"PWD\",\"\"");
        }

        if(positionDownload1 == 5)
        { // @500ms
            gprsSerial.println("AT+SAPBR=1,1");
        }

        if(positionDownload1 == 6)
        { // @600ms
            gprsSerial.println("AT+SAPBR=2,1");
        }

        if(positionDownload1 == 7)
        { // @700ms
            gprsSerial.println("AT+HTTPINIT");
        }

        if(positionDownload1 == 8)
        { //@800ms
            gprsSerial.println("AT+HTTPPARA=\"CID\",1");
        }

        if(positionDownload1 == 9)
        { //@900ms
            gprsSerial.println(sim_str);
        }


        if(positionDownload1 == 10)
        { //@1000ms
            gprsSerial.println("AT+HTTPACTION=0");//get local IP adress
            delay(2000);
        }

        if(positionDownload1 == 11)
        { //@1100ms
            gprsSerial.println("AT+HTTPREAD=0,81");//get local IP adress
            ShowDownload();
        }

        if(positionDownload1 == 12)
        { //@1200ms
            gprsSerial.println("AT+HTTPTERM");
            positionDownload1 = 0;
        }
    }
}

void transmit_to_server(int user_num, float voltage, float current, float soh, float soc, float internal_resistance)
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
    + "&field5=" + String(internal_resistance);
    String sim_str = "AT+HTTPPARA=\"URL\",\"" + str + "\"";

    if (gprsSerial.available())
    {
        Serial.write(gprsSerial.read());
    }

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
void ShowDownload( void )
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
        Serial.println(textMessage);
        char received_message[textMessage.length() + 1]; 
        textMessage.toCharArray(received_message, textMessage.length() + 1);
        Serial.println(received_message);
        token = strtok(received_message, del);
        while( token != NULL ) 
        {
            i++;
            Serial.println(i);
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
    Serial.println("Saved");
    Serial.print("VOLTAGE CALIBRATION: ");
    Serial.println(voltageCalibration);
    Serial.print("CURRENT CALIBRATION: ");
    Serial.println(currentCalibration);
    Serial.print("SOH CALIBRATION: ");
    Serial.println(sohCalibration);
    Serial.print("SOC CALIBRATION: ");
    Serial.println(socCalibration);
    }
}
