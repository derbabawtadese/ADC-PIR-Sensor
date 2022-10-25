#ifndef __UART_H
#define __UART_H

#include "TM4C123.h"                    // Device header

#define BSP_TICKS_PER_SEC 1000UL
#define SYS_CLOCK_HZ 50000000

// UART
void initUART0 (uint32_t baudrate);
void initUART1 (uint32_t baudrate);
void UART0Tx (unsigned char data);
void UART1Tx (unsigned char data);
void UART0TxNonBlocking (char data[][25], unsigned char color);
void UART1TxNonBlocking (char data[][25], unsigned char color);
unsigned char UART0Rx (void);
unsigned char UART1Rx (void);
unsigned char UART0RxNonBlocking(void);
void OutUDec(unsigned long n);
// GPIO
void initGPIOA (void);
void initGPIOB (void);
void initGPIOF (void);


#endif

