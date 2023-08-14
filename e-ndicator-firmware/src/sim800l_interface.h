#ifndef SIM800L_INTERFACE_H
#define SIM800L_INTERFACE_H

#include <SoftwareSerial.h>
#include <Arduino.h>

#define delay_sms 100

#define INTERVAL_MESSAGE_1 100
#define INTERVAL_MESSAGE_2 1100
#define INTERVAL_MESSAGE1_DOWNLOAD 1000
#define INTERVAL_MESSAGE2_DOWNLOAD 1100


/* Public function Definition */
void gprsSerialInitialize( void );
float getVoltageCalibration( void );
float getCurrentCalibration( void );
float getSohCalibration( void );
float getSocCalibration( void );
void ShowSerialData( void );
void sim800Interface_downloadFromServer(int user_num);
void transmit_to_server(int user_num, float voltage, float current, float soh, float soc, float internal_resistance);
#endif // SIM800L_INTERFACE_H
