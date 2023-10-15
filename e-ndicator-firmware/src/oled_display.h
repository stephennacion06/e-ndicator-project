#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

void oledDisplay_initialize( void );
void oledDisplay_irValue(float ir);
void oledDisplay_showParameters(float voltage, float current, float batterySoh, float batterySoc);
void oledDisplay_irSetupDisplay(float m_openCircuitVoltageValue, float m_voltageWithLoad, float m_currentWithLoad);
void oledDisplay_downloadDisplay( void );
void oledDisplay_CenterTextDisplay( String text );
#endif // OLED_DISPLAY_H