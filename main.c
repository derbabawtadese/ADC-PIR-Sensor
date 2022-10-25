/*------------------------------------- PIR Sensor Application -------------------------------------

	H/W Connections :
		MCU->LCD Pins:
		RESET------->PA7
		CS---------->PA3
		DC/A0------->PA6 
		SCL--------->PA2
		SDA--------->PA5

		PIR sensor -> PA2 (ADC Input)
		
	uController - TM4C123
		
	Arthur:
	
		Derbabbaw Tadese
		
	Date:
	
		11/30/2021
	
------------------------------------------------------------------------------------------------------*/

#include "ADC.h"    
#include "SPI.h"
#include "UART.h"
#include "Timer.h"
#include "UART.h"
#include "ST7735_Driver.h"
#include <stdlib.h>

#define BUFFER_SIZE 128
#define YMAX    4096        /* Maximum ADC value range 12-Bits */      
#define YMIN    0

volatile uint32_t AnalogIn;

void ToogleLED(uint16_t AnalogIn);
void PlotADC(uint32_t AnalogIn);
void Send2MATLAB(void);

int main(void)
{
		// Drivers initialization 
    InitADC_0();
    InitSSI_0();
    initGPIOA();
    initUART0(230400);  // UART speed 230400 
    initGPIOF();
    
	
		// External functions taken from the manufactor ST7735
    ST7735_Drawaxes(ST7735_WHITE, ST7735_BLACK, "Time", "ADC", ST7735_GREEN, "", 0, YMAX, YMIN);  // Draw axis on LCD
    ST7735_DrawString(2, 0, "ADC Value = ", ST7735_GREEN);		// Draw string on LCD
    
//		Delay1Ms(4000);
    while(1)
    {
			AnalogIn = SampleADC_0(); 	// Sample and save result
			ToogleLED(AnalogIn);				// Toggle LED if its above the limit
			Delay1Ms(500);
			PlotADC(AnalogIn); 					// Plot the result to LCD screen (Decimal values)
      Send2MATLAB();							// Send all samples to MATLAB for future analysis such as FFT etc'...
			ST7735_PlotPoint(AnalogIn, ST7735_MAGENTA);		// Draw line on LCD screen 
			ST7735_PlotIncrement();			// Increament the line on the screen 
    }
    
}

/* Each step 0.01V */
void PlotADC(uint32_t AnalogIn)
{
	ST7735_DrawCharS(14*6, 0*10,0x30+(AnalogIn / 1000), ST7735_GREEN, ST7735_BLACK, 1);      
	AnalogIn = AnalogIn % 1000;                                                                    /* plot value 0 <-> 999 on the top of the screen */ 
	ST7735_DrawCharS(15*6, 0*10,'.', ST7735_GREEN, ST7735_BLACK, 1);                
	ST7735_DrawCharS(16*6, 0*10,0x30+(AnalogIn / 100), ST7735_GREEN, ST7735_BLACK, 1);       
	AnalogIn = AnalogIn % 100;                                                                        /* ditto 0 <-> 99 */
	ST7735_DrawCharS(17*6, 0*10,0x30+(AnalogIn / 10), ST7735_GREEN, ST7735_BLACK, 1);        
	AnalogIn = AnalogIn % 10;                                                                         /* ditto  0 <-> 9 */      
	ST7735_DrawCharS(18*6, 0*10,0x30 + AnalogIn, ST7735_GREEN, ST7735_BLACK, 1);
	//
    ST7735_DrawCharS(20*6, 0*10,'V', ST7735_GREEN, ST7735_BLACK, 1);                
}

void Send2MATLAB(void)
{
	int i = 0;
	for(; i < BUFFER_SIZE; i++)
	{
    AnalogIn = SampleADC_0() * 4095 / 3.3; 		// ADC converstion formula
		UART0Tx(AnalogIn/1000-'0');		// Convert to decimal values and send it thrught UART
		GPIOF->DATA ^= 0x08;			// Toogle green LED on uController
	}
}

void ToogleLED(uint16_t AnalogIn)
{	uint16_t i;
	if(AnalogIn > 3000)
	{
		GPIOF->DATA = 0x04;	// turn on the blue LED
	}
	else
	{
		GPIOF->DATA = 0x00; // turn of all LEDs
	}
	for(i = 0; i < AnalogIn; i++)
	{
		ST7735_PlotPoint(i+20, ST7735_BLUE);  // Fill the area with blue color below the graph
	}
}

