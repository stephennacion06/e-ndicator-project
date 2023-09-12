#include "oled_display.h"
#include "utils.h"

#define QUICK_DELAY  ( 30 )
#define MID_DELAY    ( 1000 )
#define LONG_DELAY   ( 3000 )

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/* Private Function Declaration */
static void m_allPixelsDisplay( void );
static void m_textDisplay( void );
static void m_invertedTextDisplay( void );
static void m_internalResistanceSetupDisplay( void );
/* Private Function Definition */

void m_allPixelsDisplay( void )
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
    delay(QUICK_DELAY);
    }

    for(i=0;i<64;i++)
    {
        for(j=0;j<128;j++)
        {
            display.drawPixel(j, i, SSD1306_BLACK);
        
        }
        display.display();
        delay(QUICK_DELAY);
    }
}

void m_textDisplay( void )
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(24,28);
    display.println("IoT BMS");
    display.display();
    delay(LONG_DELAY);
}

void m_invertedTextDisplay( void )
{
    display.clearDisplay();
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.setCursor(24,28);
    display.println("IoT BMS");
    display.display();
    delay(LONG_DELAY);
}

void m_internalResistanceSetupDisplay( void )
{
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(24,28);
    display.println("IR SETUP");
    display.display();
    delay(LONG_DELAY);
}


/* Public Function Definition */
void oledDisplay_initialize( void )
{
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    {
        DEBUG_PRINT_LN(F("SSD1306 allocation failed"));
        for(;;); // Don't proceed, loop forever
    }

    display.clearDisplay();
    display.display();
    delay(MID_DELAY);

    m_allPixelsDisplay();
    m_textDisplay();
    m_invertedTextDisplay();
    m_internalResistanceSetupDisplay();
}

void oledDisplay_irValue(float ir)
{
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(24,28);
    display.print("IR= ");
    display.print(ir);
    display.println(" ohms");
    display.display();
    delay(LONG_DELAY);
}

void oledDisplay_showParameters(float voltage, float current, float batterySoh, float batterySoc)
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

    if(voltage < 4)
    {
        batterySoc = 0.0;
        batterySoh = 0.0;
    }

    if(batterySoc == 0)
    {
        batterySoh;
    }

    display.setTextSize(2);
    display.print(F("SOC: "));
    display.print(batterySoc,0);
    display.println(F(" %"));

    display.print(F("SOH: "));
    display.print(batterySoh,0);
    display.println(F(" %"));

    display.display();
}

void oledDisplay_irSetupDisplay(float m_openCircuitVoltageValue, float m_voltageWithLoad, float m_currentWithLoad)
{
    display.clearDisplay();
    display.setTextSize(1.5);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    
    display.print(F("OCV: "));
    display.print(m_openCircuitVoltageValue,1);
    display.println(F(" V"));

    display.print(F("VBAT: "));
    display.print(m_voltageWithLoad,1);
    display.println(F(" V"));


    display.print(F("CBAT: "));
    display.print(m_currentWithLoad);
    display.println(F(" A"));

    display.display();
}

void oledDisplay_downloadDisplay( void )
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(24,28);
    display.println("DOWNLOADING");
    display.display();
    delay(LONG_DELAY);
}

void oledDisplay_wifiTextDisplay( String text )
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(24,28);
    display.println(text);
    display.display();
    delay(LONG_DELAY);
}