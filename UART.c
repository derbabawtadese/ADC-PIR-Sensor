#include "Timer.h"
#include "UART.h"

// *************** initUART0 ********************
// UART0 initialization
// Baudrate User select, 8bit, 1 stop bit, no pairity
// Inputs: non
// Outputs: none

void initUART0 (uint32_t baudrate){
	float intBRD = SYS_CLOCK_HZ / 16.0 / (float)baudrate;
	uint16_t fracBRD = (intBRD - (int)intBRD) * 64;
	intBRD = (uint16_t)intBRD;
	SYSCTL->RCGCUART |= 1<<0;		// Enable clock for UART0
	while((SYSCTL->RCGCUART & 0x1) == 0){};
	UART0->CTL |= 0<<0;		// Diable uart for intilization and set LBE bit for Loopback - 1<<77
	UART0->IBRD = intBRD;				// Baudrate = 50MHz/16/9600 -> IBDR = 325; FBDR=0.520*64=33
	UART0->FBRD = fracBRD;
	UART0->LCRH = 0x70;				// 8-bits, no pairty, 1-stop bit, FIFO enable
	UART0->CC = 0;					// Set system clock 
	UART0->CTL |= 1<<0 | 1<<8 | 1<<9;	// UART enable; TX/RX enable
}

// *************** initUART1 ********************
// UART1 initialization
// Baudrate 9600, 8bit, 1 stop bit, no pairity
// Inputs: non
// Outputs: none

void initUART1 (uint32_t baudrate){
	float intBRD = (float)SYS_CLOCK_HZ / (float)16U / (float)baudrate;
	uint16_t fracBRD = (intBRD - (int)intBRD) * 64;
	intBRD = (uint16_t)intBRD;
	SYSCTL->RCGCUART |= 1<<1;		// Enable clock for UART1 
	while((SYSCTL->RCGCUART & 0x2) == 0){};
	UART1->CTL |= 0<<0;				// Diable uart for intilization
	UART1->IBRD = intBRD;				// Baudrate = 50MHz/16/9600 -> IBDR = 325; FBDR=0.520*64=33
	UART1->FBRD = fracBRD;
	UART1->LCRH = 0x70;				// 8-bits, no pairty, 1-stop bit, FIFO enable
	UART1->CC = 0;					// Set system clock 
	UART1->CTL |= 1<<0 | 1<<8 | 1<<9;	// UART enable; TX/RX enable
}

// *************** initGPIOA ********************
// GPIOA initialization
// PA0 Input, PA1 Output mode

void initGPIOA (void){
	SYSCTL->RCGCGPIO |= 1<<0;		// Enable clock for GPIOA
	while((SYSCTL->RCGCGPIO & 0x1) == 0){};
	GPIOA->DEN |= 1<<1 | 1<<0;		// PA0->Rx, PA1->Tx
	GPIOA->AFSEL |= 1<<0 | 1<<1;	// Set alternet function for PBA,PA1 as UART0
	GPIOA->PCTL |= 0x11;			// Set PMC0=pin0=PA0, PMC1=pin1=PA1
}

// *************** initGPIOA ********************
// GPIOB initialization
// PB0 Input, PB1 Output mode

void initGPIOB (void){
	SYSCTL->RCGCGPIO |= 1<<1;		// Enable clock for GPIOB
	while((SYSCTL->RCGCGPIO & 0x2) == 0){};
	GPIOB->DEN = 1<<1 | 1<<0;		// PB0->Rx, PB1->Tx
	GPIOB->AFSEL |= 1<<0 | 1<<1;	// Set alternet function for PB0,PB1 as UART1
	GPIOB->PCTL |= 0x11;			// Set PMC0=pin0=PB0, PMC1=pin1=PB1
}


// *************** initGPIOF ********************
// GPIOF initialization
// PF0,4 Input, PB1,2,3 Output mode

void initGPIOF (void){
	SYSCTL->RCGCGPIO |= 1<<5;		// Enable clock for GPIOB
	while((SYSCTL->RCGCGPIO & 0x20) == 0){};
	GPIOF->LOCK = 0x4C4F434B;		// Unlock GPIOF Port PF0,4
	GPIOF->CR = 0x11;				// Commit PF0,4
	GPIOF->DEN = 1<<0 | 1<<1 | 1<<2 | 1<<3 | 1<<4;		// Enable PF0-4 as digital inputs/outputs
	GPIOF->DIR |= 1<<1 | 1<<2 | 1<<3;	// Set diraction for PF1-3 as outputs
	GPIOF->PUR |= 0x11;				// Set Pull-Up for PF0,4 (Switches) 
}

// *************** UART0Tx ********************
// Output one character via UART0 (Busy-wait)
// Inputs: 8-bit ASCII character array and size of the array
// Outputs: none

void UART0Tx (unsigned char data){
	while ((UART0->FR & 0x20) != 0){};
	UART0->DR = data;
	Delay1Ms(1);
	
}

// *************** UART1Tx ********************
// Output one character via UART1 (Busy-wait)
// Inputs: 8-bit ASCII character array and size of the array
// Outputs: none

void UART1Tx (unsigned char data){
	while ((UART0->FR & 0x20) != 0){};
	UART1->DR = data;
}

// *************** UART0TxNonBlocking ********************
// Output one character via UART0 (Busy-wait)
// Inputs: 8-bit ASCII character array and size of the array
// Outputs: none

void UART0TxNonBlocking (char data[][25], unsigned char color){
	unsigned short i = 0;
	while ((UART0->FR & 0x20) != 0) {};
	while (data[color][i] != '\0'){
		UART0->DR = data[color][i];
		i++;
	}
}

// *************** UART1TxNonBlocking ********************
// Output one character via UART1 (Busy-wait)
// Inputs: 8-bit ASCII character array and size of the array
// Outputs: none

void UART1TxNonBlocking (char data[][25], unsigned char color){
	unsigned short i = 0;
	while ((UART1->FR & 0x20) == 0)  {};
	while (data[color][i] != '\0'){
		UART1->DR = data[color][i];
		i++;
	}
}

// *************** UART0Rx ********************
// Input one character via UART0 (Busy-wait)
// Inputs: non
// Outputs: 8-bit ASCII character

unsigned char UART0Rx (void){
	while ((UART0->FR & 10) != 0){};
	return (UART0->DR & 0xff);	// mask 8-bits only
}

// *************** UART1Rx ********************
// Input one character via UART1 (Busy-wait)
// Inputs: non
// Outputs: 8-bit ASCII character

unsigned char UART1Rx (void){
	while ((UART1->FR & 10) != 0){};
	return (UART1->DR & 0xff);
}

// *************** UART0RxNonBlocking ********************
// Input one character via UART0 (Non-blocking)
// Inputs: 8-bit ASCII charcter 
// Outputs: none

unsigned char UART0RxNonBlocking(void){
	if ((UART0->FR & 10) != 0) return 0;
	return UART0->DR & 0xff;	// mask 8-bits only
}

// *************** UART1RxNonBlocking ********************
// Input one character via UART0 (Non-blocking)
// Inputs: 8-bit ASCII charcter 
// Outputs: none

unsigned char UART1RxNonBlocking(void){
	if ((UART1->FR & 10) != 0) return 0;
	return UART1->DR & 0xff;	// mask 8-bits only
}

// iterative method
void OutUDec(unsigned long n){
    unsigned cnt=0;
    static char buffer[11];
      do{
        buffer[cnt] = n%10;// digit
        n = n/10;
        cnt++;
      } 
      while(n);// repeat until n==0
      for(; cnt; cnt--){
        UART0Tx(buffer[cnt-1]+'0');
      }
      UART0Tx('\n');
}

//// *************** writeCommand ********************
//// Output one character to the LCD
//// Position determined by ST7735_SetCursor command
//// Color set by ST7735_SetTextColor
//// Inputs: 8-bit ASCII character
//// Outputs: none
//void writeCommand (char cmd){
//	while((SSI0->SR & 0x10) != 0){}; // wait until SSI0 not busy/transmit FIFO empty
//	GPIOA->DATA = 0<<6;	// D/C pin configured for CMD		
//	SSI0->DR = cmd;	// Write the command 
//	while((SSI0->SR & 0x10) != 0){}; // wait until SSI0 not busy/transmit FIFO empty
//}

//// *************** writeData ********************
//// Output one character to the LCD
//// Position determined by ST7735_SetCursor command
//// Color set by ST7735_SetTextColor
//// Inputs: 8-bit ASCII character
//// Outputs: none
//void writeData (char data){
//	while((SSI0->SR & 0x01) != 1){}; // wait for transmit FIFO not full
//	GPIOA->DATA = 1<<6;	// D/C pin configured for DATA
//	SSI0->DR = data; // Write the 8-bit data 
//	
//}

//// *************** ST7735_OutChar ********************
//// Output one character to the LCD
//// Position determined by ST7735_SetCursor command
//// Color set by ST7735_SetTextColor
//// Inputs: 8-bit ASCII character
//// Outputs: none
//void ST7735_OutChar (char ch){

//}

////********ST7735_OutString*****************
//// Print a string of characters to the ST7735 LCD.
//// Position determined by ST7735_SetCursor command
//// Color set by ST7735_SetTextColor
//// Inputs: ptr  pointer to NULL-terminated ASCII string
//// Outputs: none
//void ST7735_OutString (char *ptr){

//}
