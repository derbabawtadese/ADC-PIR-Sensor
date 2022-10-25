#ifndef __SPI_H_
#define __SPI_H_

#include "TM4C123.h"                    // Device header

void InitSSI_0(void);
void WriteCommand(uint8_t Command);    
void WriteData(uint8_t Data);

#endif
