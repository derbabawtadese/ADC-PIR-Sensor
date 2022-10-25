#include "Timer.h"

void Delay1Ms(uint32_t TimeInMs)
{
    uint32_t i;
    SYSCTL->RCGCTIMER |= 0x01;
    TIMER0->CTL = 0x00;
    TIMER0->CFG = 0x04;
    TIMER0->TAMR = 0x02;
    TIMER0->TAILR = 120000 - 1;
    TIMER0->ICR = 0x01;
    TIMER0->CTL |= 0x01;
    
    for(i = 0; i < TimeInMs; i++)
    {
        while((TIMER0->RIS & 0x01) == 0){};
        TIMER0->ICR = 0x01;
    }
}

