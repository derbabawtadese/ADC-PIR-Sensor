#include "ADC.h"

//void InitADC_0(void)
//{
//	SYSCTL->RCGCADC = 0x01;		// Enable clock for ADC module 0
//	SYSCTL->RCGCGPIO |= 0x10;	// Enable clock for portE
//	while((SYSCTL->RCGCADC & 0x01) == 0);
//	GPIOE->DIR &= ~0x04;		// PE2 as an input
//	GPIOE->ADCCTL |= 0x01;
//	GPIOE->AFSEL |= 0x04;		// Set alternate function on PE2 used for ADCin
//	GPIOE->DEN &= ~0x04;		// Disable digital I/O on PE2
//	GPIOE->AMSEL |= 0x04;		// Enable analog function on PE2
//	
//	ADC0->ACTSS &= ~0x0F; 
//	ADC0->EMUX |= 0x00;
//	ADC0->SSMUX0 |= 0x01;
//	ADC0->SSCTL0 |= 0x20;
//	ADC0->ACTSS |= 0x01; 
//}

void InitADC_0(void)
{
  SYSCTL->RCGCADC |= 0x0001;   // 1) activate ADC0
  SYSCTL->RCGCGPIO |= 0x10;    // 2) activate clock for Port E
  while((SYSCTL->RCGCGPIO & 0x10) != 0x10){};  // 3 for stabilization
  GPIOE->DIR &= ~0x10;    // 4) make PE4 input
  GPIOE->AFSEL |= 0x10;   // 5) enable alternate function on PE4
  GPIOE->DEN &= ~0x10;    // 6) disable digital I/O on PE4
  GPIOE->AMSEL |= 0x10;   // 7) enable analog functionality on PE4
// while((SYSCTL_PRADC_R&0x0001) != 0x0001){}; // good code, but not implemented in simulator
  ADC0->PC &= ~0xF;
  ADC0->PC |= 0x1;             // 8) configure for 125K samples/sec
  ADC0->SSPRI = 0x0123;        // 9) Sequencer 3 is highest priority
  ADC0->ACTSS &= ~0x0008;      // 10) disable sample sequencer 3
  ADC0->EMUX &= ~0xF000;       // 11) seq3 is software trigger
  ADC0->SSMUX3 &= ~0x000F;
  ADC0->SSMUX3 += 9;           // 12) set channel
  ADC0->SSCTL3 = 0x0006;       // 13) no TS0 D0, yes IE0 END0
  ADC0->IM &= ~0x0008;         // 14) disable SS3 interrupts
  ADC0->ACTSS |= 0x0008;       // 15) enable sample sequencer 3
}

//uint16_t SampleADC_0(void)
//{
//	uint16_t Result;
//	ADC0->PSSI |= 0x01;			// Begin sampling on Sample Sequencer 0
//	while((ADC0->ACTSS & 0x1000) == 0x1000){}		// ADC Busy
//	Result = ADC0->SSFIFO0;
//	return Result;				// Conversion Result Data
//}


//------------SampleADC_0------------
// Busy-wait analog to digital conversion
// Input: none
// Output: 12-bit result of ADC conversion

uint16_t SampleADC_0(void)
{  
	uint32_t result;
  ADC0->PSSI = 0x0008;            // 1) initiate SS3
  while((ADC0->RIS&0x08)==0){};   // 2) wait for conversion done
  result = ADC0->SSFIFO3 & 0xFFF;   // 3) read result
  ADC0->ISC = 0x0008;             // 4) acknowledge completion
  return result;
}
void InitGPIOF(void)
{
	SYSCTL->RCGCGPIO |= 0x20;
	while((SYSCTL->RCGCGPIO & 0x20) == 0){}
	GPIOF->DIR |= 0x02;
	GPIOF->DEN |= 0x02;
}

uint32_t Data; // 0 to 4095
uint32_t Flag; // 1 means new data
void SysTick_Handler(void){
  GPIOF->DATA ^= 0x02; // toggle PF1
  Data = SampleADC_0();      // Sample ADC
  Flag = 1;                  // Synchronize with other threads
}
