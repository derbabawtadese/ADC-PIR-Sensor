#ifndef __ADC_H
#define __ADC_H

#include "TM4C123.h"                    // Device header

void InitADC_0(void);
void InitGPIOF(void);
uint16_t SampleADC_0(void);

#endif
