#ifndef SIM800L_INTERFACE_H
#define SIM800L_INTERFACE_H

#include <SoftwareSerial.h>
#include <Arduino.h>

/* Public function Definition */
void sim800lInterface_gprsSerialInitialize( void );
float sim800lInterface_getVoltageCalibration( void );
float sim800lInterface_getCurrentCalibration( void );
float sim800lInterface_getSohCalibration( void );
float sim800lInterface_getSocCalibration( void );
void sim800Interface_downloadFromServer(int user_num);
void sim800lInterface_transmitToServer(int user_num, float voltage, float current, float soh, float soc, float internalResistance);
void sim800Interface_wifiTransmission( int user_num, float voltage, float current, float soh, float soc, float internalResistance );
#endif // SIM800L_INTERFACE_H
