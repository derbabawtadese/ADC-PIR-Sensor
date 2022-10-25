#include "SPI.h"
#include "ST7735_Driver.h"

void InitSSI_0(void)
{
    SYSCTL->RCGCSSI |= 0x01;
    SYSCTL->RCGCGPIO |= 0x01;
    while((SYSCTL->PRGPIO & 0x01) == 0){};
    
    /* Initialize LCD - PA3,6,7 = 0xC8*/
    GPIOA->DIR |= 0xC8;             
    GPIOA->AFSEL &= ~0xC8;
    GPIOA->DEN |= 0xC8;
    GPIOA->PCTL = ((GPIOA->PCTL & 0x00FF0FFF) + 0x00000000);
    GPIOA->AMSEL &= ~0xC8;
    
    LCD_CS0;                /* Set CS to 0 */
    LCD_RS1;                /* Set RS to 1 */
    Delay(500);
    LCD_RS0;
    Delay(500);
    LCD_RS1;
    Delay(500);
    
    /* Initialize SSI0 */
    GPIOA->AFSEL |= 0x2C;
    GPIOA->DEN |= 0x2C;
    GPIOA->PCTL = ((GPIOA->PCTL & 0xFF0F00FF) + 0x00202200);
    GPIOA->AMSEL &= ~0x2C;              /* Disable analog functionalitY on PA2,3,5*/
    
    SSI0->CR1 &= ~SSI_CR1_SSE;              /* Disable SSI */
    SSI0->CR1 &= ~SSI_CR1_MS;               /* TM4C Master mode*/
    
    /* Configure for System Clock/PLL buad clock source */
    SSI0->CC = ((SSI0->CC & (~SSI_CC_CS_M)) + SSI_CC_CS_SYSPLL);
    SSI0->CPSR = ((SSI0->CPSR & (~SSI_CPSR_CPSDVSR_M)) + 16);
    SSI0->CPSR = ((SSI0->CPSR & (~SSI_CPSR_CPSDVSR_M)) + 10);
    SSI0->CR0 &= ~(SSI_CR0_SCR_M | SSI_CR0_SPO | SSI_CR0_SPH);
    SSI0->CR0 = ((SSI0->CR0 & (~(SSI_CR0_FRF_M))) + SSI_CR0_FRF_MOTO);
    SSI0->CR0 = ((SSI0->CR0 & (~(SSI_CR0_DSS_M))) + SSI_CR0_DSS_8);
    SSI0->CR1 |= SSI_CR1_SSE;
    
    StandartInitCmd();
}

// The Data/Command pin must be valid when the eighth bit is
// sent.  The SSI module has hardware input and output FIFOs
// that are 8 locations deep.  Based on the observation that
// the LCD interface tends to send a few commands and then a
// lot of data, the FIFOs are not used when writing
// commands, and they are used when writing data.  This
// ensures that the Data/Command pin status matches the byte
// that is actually being transmitted.
// The write command operation waits until all data has been
// sent, configures the Data/Command pin for commands, sends
// the command, and then waits for the transmission to
// finish.
// The write data operation waits until there is room in the
// transmit FIFO, configures the Data/Command pin for data,
// and then adds the data to the transmit FIFO.
// NOTE: These functions will crash or stall indefinitely if
// the SSI0 module is not initialized and enabled.

void WriteCommand(uint8_t Command)
{
    while((SSI0->SR & SSI_SR_BSY) == 0x10){};  
    DC = DC_COMMAND;
    SSI0->DR = Command;
    while((SSI0->SR & SSI_SR_BSY) == 0x10){};
}

void WriteData(uint8_t Data)
{
    while((SSI0->SR & SSI_SR_TNF) == 0x00){};    
    DC = DC_DATA;
    SSI0->DR = Data;  
}


