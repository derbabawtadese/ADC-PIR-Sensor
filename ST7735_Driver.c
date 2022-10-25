#include "ST7735_Driver.h"

// 16 rows (0 to 15) and 21 characters (0 to 20)
// Requires (11 + size*size*6*8) bytes of transmission for each character
uint32_t StX=0; // position along the horizonal axis 0 to 20
uint32_t StY=0; // position along the vertical axis 0 to 15
uint16_t StTextColor = ST7735_GREEN;

// Subroutine to wait 1 msec
// Inputs: None
// Outputs: None
// Notes: ...
void Delay(uint32_t n){uint32_t volatile time;
  while(n){
    time = 72724*2/91;  // 1msec, tuned at 80 MHz
    while(time){
      time--;
    }
    n--;
  }
}


// Rather than a bazillion WriteCommand() and WriteData() calls, screen
// initialization commands and arguments are organized in these tables
// stoST7735_RED in ROM.  The table may look bulky, but that's mostly the
// formatting -- storage-wise this is hundST7735_REDs of bytes more compact
// than the equivalent code.  Companion function follows.
#define DELAY 0x80
static const uint8_t
  Bcmd[] = {                  // Initialization commands for 7735B screens
    18,                       // 18 commands in list:
    ST7735_SWRESET,   DELAY,  //  1: Software reset, no args, w/delay
      50,                     //     50 ms delay
    ST7735_SLPOUT ,   DELAY,  //  2: Out of sleep mode, no args, w/delay
      255,                    //     255 = 500 ms delay
    ST7735_COLMOD , 1+DELAY,  //  3: Set color mode, 1 arg + delay:
      0x05,                   //     16-bit color
      10,                     //     10 ms delay
    ST7735_FRMCTR1, 3+DELAY,  //  4: Frame rate control, 3 args + delay:
      0x00,                   //     fastest refresh
      0x06,                   //     6 lines front porch
      0x03,                   //     3 lines back porch
      10,                     //     10 ms delay
    ST7735_MADCTL , 1      ,  //  5: Memory access ctrl (directions), 1 arg:
      0x08,                   //     Row addr/col addr, bottom to top refresh
    ST7735_DISSET5, 2      ,  //  6: Display settings #5, 2 args, no delay:
      0x15,                   //     1 clk cycle nonoverlap, 2 cycle gate
                              //     rise, 3 cycle osc equalize
      0x02,                   //     Fix on VTL
    ST7735_INVCTR , 1      ,  //  7: Display inversion control, 1 arg:
      0x0,                    //     Line inversion
    ST7735_PWCTR1 , 2+DELAY,  //  8: Power control, 2 args + delay:
      0x02,                   //     GVDD = 4.7V
      0x70,                   //     1.0uA
      10,                     //     10 ms delay
    ST7735_PWCTR2 , 1      ,  //  9: Power control, 1 arg, no delay:
      0x05,                   //     VGH = 14.7V, VGL = -7.35V
    ST7735_PWCTR3 , 2      ,  // 10: Power control, 2 args, no delay:
      0x01,                   //     Opamp current small
      0x02,                   //     Boost frequency
    ST7735_VMCTR1 , 2+DELAY,  // 11: Power control, 2 args + delay:
      0x3C,                   //     VCOMH = 4V
      0x38,                   //     VCOML = -1.1V
      10,                     //     10 ms delay
    ST7735_PWCTR6 , 2      ,  // 12: Power control, 2 args, no delay:
      0x11, 0x15,
    ST7735_GMCTRP1,16      ,  // 13: Magical unicorn dust, 16 args, no delay:
      0x09, 0x16, 0x09, 0x20, //     (seriously though, not sure what
      0x21, 0x1B, 0x13, 0x19, //      these config values represent)
      0x17, 0x15, 0x1E, 0x2B,
      0x04, 0x05, 0x02, 0x0E,
    ST7735_GMCTRN1,16+DELAY,  // 14: Sparkles and rainbows, 16 args + delay:
      0x0B, 0x14, 0x08, 0x1E, //     (ditto)
      0x22, 0x1D, 0x18, 0x1E,
      0x1B, 0x1A, 0x24, 0x2B,
      0x06, 0x06, 0x02, 0x0F,
      10,                     //     10 ms delay
    ST7735_CASET  , 4      ,  // 15: Column addr set, 4 args, no delay:
      0x00, 0x02,             //     XSTART = 2
      0x00, 0x81,             //     XEND = 129
    ST7735_RASET  , 4      ,  // 16: Row addr set, 4 args, no delay:
      0x00, 0x02,             //     XSTART = 1
      0x00, 0x81,             //     XEND = 160
    ST7735_NORON  ,   DELAY,  // 17: Normal display on, no args, w/delay
      10,                     //     10 ms delay
    ST7735_DISPON ,   DELAY,  // 18: Main screen turn on, no args, w/delay
      255 };                  //     255 = 500 ms delay
static const uint8_t
  Rcmd1[] = {                 // Init for 7735R, part 1 (ST7735_RED or green tab)
    15,                       // 15 commands in list:
    ST7735_SWRESET,   DELAY,  //  1: Software reset, 0 args, w/delay
      150,                    //     150 ms delay
    ST7735_SLPOUT ,   DELAY,  //  2: Out of sleep mode, 0 args, w/delay
      255,                    //     500 ms delay
    ST7735_FRMCTR1, 3      ,  //  3: Frame rate ctrl - normal mode, 3 args:
      0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR2, 3      ,  //  4: Frame rate control - idle mode, 3 args:
      0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR3, 6      ,  //  5: Frame rate ctrl - partial mode, 6 args:
      0x01, 0x2C, 0x2D,       //     Dot inversion mode
      0x01, 0x2C, 0x2D,       //     Line inversion mode
    ST7735_INVCTR , 1      ,  //  6: Display inversion ctrl, 1 arg, no delay:
      0x07,                   //     No inversion
    ST7735_PWCTR1 , 3      ,  //  7: Power control, 3 args, no delay:
      0xA2,
      0x02,                   //     -4.6V
      0x84,                   //     AUTO mode
    ST7735_PWCTR2 , 1      ,  //  8: Power control, 1 arg, no delay:
      0xC5,                   //     VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
    ST7735_PWCTR3 , 2      ,  //  9: Power control, 2 args, no delay:
      0x0A,                   //     Opamp current small
      0x00,                   //     Boost frequency
    ST7735_PWCTR4 , 2      ,  // 10: Power control, 2 args, no delay:
      0x8A,                   //     BCLK/2, Opamp current small & Medium low
      0x2A,
    ST7735_PWCTR5 , 2      ,  // 11: Power control, 2 args, no delay:
      0x8A, 0xEE,
    ST7735_VMCTR1 , 1      ,  // 12: Power control, 1 arg, no delay:
      0x0E,
    ST7735_INVOFF , 0      ,  // 13: Don't invert display, no args, no delay
    ST7735_MADCTL , 1      ,  // 14: Memory access control (directions), 1 arg:
      0xC8,                   //     row addr/col addr, bottom to top refresh
    ST7735_COLMOD , 1      ,  // 15: set color mode, 1 arg, no delay:
      0x05 };                 //     16-bit color
static const uint8_t
  Rcmd2green[] = {            // Init for 7735R, part 2 (green tab only)
    2,                        //  2 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x02,             //     XSTART = 0
      0x00, 0x7F+0x02,        //     XEND = 127
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x01,             //     XSTART = 0
      0x00, 0x9F+0x01 };      //     XEND = 159
static const uint8_t
  Rcmd2ST7735_RED[] = {              // Init for 7735R, part 2 (ST7735_RED tab only)
    2,                        //  2 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x7F,             //     XEND = 127
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x9F };           //     XEND = 159
static const uint8_t
  Rcmd3[] = {                 // Init for 7735R, part 3 (ST7735_RED or green tab)
    4,                        //  4 commands in list:
    ST7735_GMCTRP1, 16      , //  1: Magical unicorn dust, 16 args, no delay:
      0x02, 0x1c, 0x07, 0x12,
      0x37, 0x32, 0x29, 0x2d,
      0x29, 0x25, 0x2B, 0x39,
      0x00, 0x01, 0x03, 0x10,
    ST7735_GMCTRN1, 16      , //  2: Sparkles and rainbows, 16 args, no delay:
      0x03, 0x1d, 0x07, 0x06,
      0x2E, 0x2C, 0x29, 0x2D,
      0x2E, 0x2E, 0x37, 0x3F,
      0x00, 0x00, 0x02, 0x10,
    ST7735_NORON  ,    DELAY, //  3: Normal display on, no args, w/delay
      10,                     //     10 ms delay
    ST7735_DISPON ,    DELAY, //  4: Main screen turn on, no args w/delay
      100 };                  //     100 ms delay


// Companion code to the above tables.  Reads and issues
// a series of LCD commands stoST7735_RED in ROM byte array.
void static commandList(const uint8_t *addr) {

  uint8_t numCommands, numArgs;
  uint16_t ms;

  numCommands = *(addr++);               // Number of commands to follow
  while(numCommands--) {                 // For each command...
    WriteCommand(*(addr++));             //   Read, issue command
    numArgs  = *(addr++);                //   Number of args to follow
    ms       = numArgs & DELAY;          //   If hibit set, delay follows args
    numArgs &= ~DELAY;                   //   Mask out delay bit
    while(numArgs--) {                   //   For each argument...
      WriteData(*(addr++));              //     Read, issue argument
    }

    if(ms) {
      ms = *(addr++);             // Read post-command delay time (ms)
      if(ms == 255) ms = 500;     // If 255, delay for 500 ms
      Delay(ms);
    }
  }
}


// Initialization code common to both 'B' and 'R' type displays
void static commonInit(const uint8_t *cmdList) {
  volatile uint32_t delay;
  ColStart  = RowStart = 0; // May be overridden in init func

  SYSCTL->RCGCSSI |= 0x01;  // activate SSI0
  SYSCTL->RCGCGPIO |= 0x01; // activate port A
  while((SYSCTL->RCGCGPIO &0x01)==0){}; // allow time for clock to start

  // toggle RST low to reset; CS low so it'll listen to us
  // SSI0Fss is temporarily used as GPIO
    
    /* Initialize LCD - PA3,6,7 = 0xC8*/
    GPIOA->DIR |= 0xC8;             
    GPIOA->AFSEL &= ~0xC8;
    GPIOA->DEN |= 0xC8;
    GPIOA->PCTL = ((GPIOA->PCTL & 0x00FF0FFF) + 0x00000000);
    GPIOA->AMSEL &= ~0xC8;
  TFT_CS = TFT_CS_LOW;
  RESET = RESET_HIGH;
  Delay(500);
  RESET = RESET_LOW;
  Delay(500);
  RESET = RESET_HIGH;
  Delay(500);
    
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

  if(cmdList) commandList(cmdList);
}


//------------ST7735_InitB------------
// Initialization for ST7735B screens.
// Input: none
// Output: none
void ST7735_InitB(void) {
  commonInit(Bcmd);
  ST7735_SetCursor(0,0);
  StTextColor = ST7735_GREEN;
  ST7735_FillScreen(0);                 // set screen to ST7735_BLACK
}


//------------ST7735_InitR------------
// Initialization for ST7735R screens (green or ST7735_RED tabs).
// Input: option one of the enumerated options depending on tabs
// Output: none
void ST7735_InitR(enum initRFlags option) {
  commonInit(Rcmd1);
  if(option == INITR_GREENTAB) {
    commandList(Rcmd2green);
    ColStart = 2;
    RowStart = 1;
  } else {
    // colstart, rowstart left at default '0' values
    commandList(Rcmd2ST7735_RED);
  }
  commandList(Rcmd3);

  // if ST7735_BLACK, change MADCTL color filter
  if (option == INITR_BLACKTAB) {
    WriteCommand(ST7735_MADCTL);
    WriteData(0xC0);
  }
  TabColor = option;
  ST7735_SetCursor(0,0);
  StTextColor = ST7735_GREEN;
  ST7735_FillScreen(0);                 // set screen to ST7735_BLACK
}

void ST7735_SetCursor(uint32_t newX, uint32_t newY){
  if((newX > 20) || (newY > 15)){       
    return;                           
  }
  StX = newX;
  StY = newY;
}

void ST7735_FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  uint8_t hi = color >> 8, lo = color;

  if((x >= _width) || (y >= _height)) return;
  if((x + w - 1) >= _width)  w = _width  - x;
  if((y + h - 1) >= _height) h = _height - y;

  setAddrWindow(x, y, x+w-1, y+h-1);

  for(y=h; y>0; y--) {
    for(x=w; x>0; x--) {
      WriteData(hi);
      WriteData(lo);
    }
  }
}

void ST7735_FillScreen(uint16_t color) {
  ST7735_FillRect(0, 0, _width, _height, color);  
}


void static setAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {

  WriteCommand(ST7735_CASET); // Column addr set
  WriteData(0x00);
  WriteData(x0+ColStart);     // XSTART
  WriteData(0x00);
  WriteData(x1+ColStart);     // XEND

  WriteCommand(ST7735_RASET); // Row addr set
  WriteData(0x00);
  WriteData(y0+RowStart);     // YSTART
  WriteData(0x00);
  WriteData(y1+RowStart);     // YEND

  WriteCommand(ST7735_RAMWR); // write to RAM
}


void StandartInitCmd(void)
{
    LCD_CS0;            // CS=0   
    LCD_RS0;           // RST=0 

    WriteCommand(10);      

    LCD_RS1;           // RST=1
    Delay(10);      


    WriteCommand(0x11); 

    Delay(120);      

    WriteCommand (0x3A); //Set Color mode
    WriteData(0x05); //16 bits
    WriteCommand (0x36);
    WriteData(0x14);
    WriteCommand (0x29);//Display on
}

void static pushColor(uint16_t color) {
  WriteData((uint8_t)(color >> 8));
  WriteData((uint8_t)color);
}

void ST7735_DrawPixel(int16_t x, int16_t y, uint16_t color) {

  if((x < 0) || (x >= _width) || (y < 0) || (y >= _height)) return;

  setAddrWindow(x,y,x,y);

  pushColor(color);
}

void ST7735_DrawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
  uint8_t hi = color >> 8, lo = color;

  // Rudimentary clipping
  if((x >= _width) || (y >= _height)) return;
  if((y+h-1) >= _height) h = _height-y;
  setAddrWindow(x, y, x, y+h-1);

  while (h--) {
    WriteData(hi);
    WriteData(lo);
  }
}

void ST7735_DrawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
  uint8_t hi = color >> 8, lo = color;

  // Rudimentary clipping
  if((x >= _width) || (y >= _height)) return;
  if((x+w-1) >= _width)  w = _width-x;
  setAddrWindow(x, y, x+w-1, y);

  while (w--) {
    WriteData(hi);
    WriteData(lo);
  }
}


//------------ST7735_DrawBitmap------------
// Displays a 16-bit color BMP image.  A bitmap file that is created
// by a PC image processing program has a header and may be padded
// with dummy columns so the data have four byte alignment.  This
// function assumes that all of that has been stripped out, and the
// array image[] has one 16-bit halfword for each pixel to be
// displayed on the screen (encoded in reverse order, which is
// standard for bitmap files).  An array can be created in this
// format from a 24-bit-per-pixel .bmp file using the associated
// converter program.
// (x,y) is the screen location of the lower left corner of BMP image
// Requires (11 + 2*w*h) bytes of transmission (assuming image fully on screen)
// Input: x     horizontal position of the bottom left corner of the image, columns from the left edge
//        y     vertical position of the bottom left corner of the image, rows from the top edge
//        image pointer to a 16-bit color BMP image
//        w     number of pixels wide
//        h     number of pixels tall
// Output: none
// Must be less than or equal to 128 pixels wide by 160 pixels high
void ST7735_DrawBitmap(int16_t x, int16_t y, const uint16_t *image, int16_t w, int16_t h){
  int16_t skipC = 0;                      // non-zero if columns need to be skipped due to clipping
  int16_t originalWidth = w;              // save this value; even if not all columns fit on the screen, the image is still this width in ROM
  int i = w*(h - 1);

  if((x >= _width) || ((y - h + 1) >= _height) || ((x + w) <= 0) || (y < 0)){
    return;                             // image is totally off the screen, do nothing
  }
  if((w > _width) || (h > _height)){    // image is too wide for the screen, do nothing
    //***This isn't necessarily a fatal error, but it makes the
    //following logic much more complicated, since you can have
    //an image that exceeds multiple boundaries and needs to be
    //clipped on more than one side.
    return;
  }
  if((x + w - 1) >= _width){            // image exceeds right of screen
    skipC = (x + w) - _width;           // skip cut off columns
    w = _width - x;
  }
  if((y - h + 1) < 0){                  // image exceeds top of screen
    i = i - (h - y - 1)*originalWidth;  // skip the last cut off rows
    h = y + 1;
  }
  if(x < 0){                            // image exceeds left of screen
    w = w + x;
    skipC = -1*x;                       // skip cut off columns
    i = i - x;                          // skip the first cut off columns
    x = 0;
  }
  if(y >= _height){                     // image exceeds bottom of screen
    h = h - (y - _height + 1);
    y = _height - 1;
  }

  setAddrWindow(x, y-h+1, x+w-1, y);

  for(y=0; y<h; y=y+1){
    for(x=0; x<w; x=x+1){
                                        // send the top 8 bits
      WriteData((uint8_t)(image[i] >> 8));
                                        // send the bottom 8 bits
      WriteData((uint8_t)image[i]);
      i = i + 1;                        // go to the next pixel
    }
    i = i + skipC;
    i = i - 2*originalWidth;
  }

  deselect();
}

void ST7735_DrawCharS(int16_t x, int16_t y, char c, int16_t textColor, int16_t bgColor, uint8_t size){
  uint8_t line; 
  int32_t i, j;
  if((x >= _width)            || 
     (y >= _height)           || 
     ((x + 5 * size - 1) < 0) || 
     ((y + 8 * size - 1) < 0))   
    return;

  for (i=0; i<6; i++ ) {
    if (i == 5)
      line = 0x0;
    else
      line = Font[(c*5)+i];
    for (j = 0; j<8; j++) {
      if (line & 0x1) {
        if (size == 1) 
          ST7735_DrawPixel(x+i, y+j, textColor);
        else {  
          ST7735_FillRect(x+(i*size), y+(j*size), size, size, textColor);
        }
      } else if (bgColor != textColor) {
        if (size == 1) // default size
          ST7735_DrawPixel(x+i, y+j, bgColor);
        else {  // big size
          ST7735_FillRect(x+i*size, y+j*size, size, size, bgColor);
        }
      }
      line >>= 1;
    }
  }
}

void ST7735_DrawChar(int16_t x, int16_t y, char c, int16_t textColor, int16_t bgColor, uint8_t size){
  uint8_t line; // horizontal row of pixels of character
  int32_t col, row, i, j;// loop indices
  if(((x + 5*size - 1) >= _width)  || // Clip right
     ((y + 8*size - 1) >= _height) || // Clip bottom
     ((x + 5*size - 1) < 0)        || // Clip left
     ((y + 8*size - 1) < 0)){         // Clip top
    return;
  }

  setAddrWindow(x, y, x+6*size-1, y+8*size-1);

  line = 0x01;        // print the top row first
  // print the rows, starting at the top
  for(row=0; row<8; row=row+1){
    for(i=0; i<size; i=i+1){
      // print the columns, starting on the left
      for(col=0; col<5; col=col+1){
        if(Font[(c*5)+col]&line){
          // bit is set in Font, print pixel(s) in text color
          for(j=0; j<size; j=j+1){
            pushColor(textColor);
          }
        } else{
          // bit is cleaST7735_RED in Font, print pixel(s) in background color
          for(j=0; j<size; j=j+1){
            pushColor(bgColor);
          }
        }
      }
      // print blank column(s) to the right of character
      for(j=0; j<size; j=j+1){
        pushColor(bgColor);
      }
    }
    line = line<<1;   // move up to the next row
  }
}

uint32_t ST7735_DrawString(uint16_t x, uint16_t y, char *pt, int16_t textColor){
  uint32_t count = 0;
  if(y>15) return 0;
  while(*pt){
    ST7735_DrawCharS(x*6, y*10, *pt, textColor, ST7735_BLACK, 1);
    pt++;
    x = x+1;
    if(x>20) return count;  // number of characters printed
    count++;
  }
  return count;  // number of characters printed
}

int32_t Ymax,Ymin,X;        // X goes from 0 to 127
int32_t Yrange; //YrangeDiv2;

int TimeIndex;               
int32_t Ymax, Ymin, Yrange;  
uint16_t PlotBGColor; 

void ST7735_SimplePlotPoint(int32_t y)
{
		int32_t j;
  if(y<Ymin) y=Ymin;
  if(y>Ymax) y=Ymax;
  j = 32+(127*(Ymax-y))/Yrange;
  if(j<32) j = 32;
  if(j>159) j = 159;
  ST7735_DrawPixel(X,   j,  ST7735_BLUE);
  ST7735_DrawPixel(X+1, j,  ST7735_BLUE);
  ST7735_DrawPixel(X,   j+1,ST7735_BLUE);
  ST7735_DrawPixel(X+1, j+1,ST7735_BLUE);
}

void ST7735_PlotIncrement(void){
  TimeIndex = TimeIndex + 1;
  if(TimeIndex > 113){	// x axis
    TimeIndex = 0;
  }
  ST7735_DrawFastVLine(TimeIndex + 11, 11, 129, PlotBGColor);	// x - start from 11, y ditto as x, h - 129 h(y top)
}

int32_t lastj=0;

void ST7735_PlotLine(int32_t y)
{
	int32_t i,j;
	if(y<Ymin) y=Ymin;
	if(y>Ymax) y=Ymax;
	// X goes from 0 to 127
	// j goes from 159 to 32
	// y=Ymax maps to j=32
	// y=Ymin maps to j=159
	j = 32+(127*(Ymax-y))/Yrange;
	if(j < 32) j = 32;
	if(j > 159) j = 159;
	if(lastj < 32) lastj = j;
	if(lastj > 159) lastj = j;
	if(lastj < j){
	for(i = lastj+1; i<=j ; i++){
		ST7735_DrawPixel(X,   i,   ST7735_BLUE) ;
		ST7735_DrawPixel(X+1, i,   ST7735_BLUE) ;
	}
	}else if(lastj > j){
	for(i = j; i<lastj ; i++){
		ST7735_DrawPixel(X,   i,   ST7735_BLUE) ;
		ST7735_DrawPixel(X+1, i,   ST7735_BLUE) ;
	}
	}else{
		ST7735_DrawPixel(X,   j,   ST7735_BLUE) ;
		ST7735_DrawPixel(X+1, j,   ST7735_BLUE) ;
	}
	lastj = j;
}

void ST7735_PlotBar(int32_t y){
	int32_t j;
	if(y<Ymin) y=Ymin;
	if(y>Ymax) y=Ymax;

	j = 32+(127*(Ymax-y))/Yrange;
	ST7735_DrawFastVLine(X, j, 159-j,ST7735_BLACK);
}

//Define Screen area
void lcd7735_at(unsigned char startX, unsigned char startY, unsigned char stopX, unsigned char stopY) {
	WriteCommand(0x2A);
	LCD_DC1;
	WriteData(0x00);
	WriteData(startX);
	WriteData(0x00);
	WriteData(stopX);

	WriteCommand(0x2B);
	LCD_DC1;
	WriteData(0x00);
	WriteData(startY);
	WriteData(0x00); 
	WriteData(stopY);
}

void ST7735_Drawaxes(uint16_t axisColor, uint16_t bgColor, char *xLabel,char *yLabel1, uint16_t label1Color, char *yLabel2, uint16_t label2Color,int32_t ymax, int32_t ymin)
{
	int i;

	Ymax = ymax;
	Ymin = ymin;
	Yrange = Ymax - Ymin;
	TimeIndex = 0;
	PlotBGColor = bgColor;
	ST7735_FillRect(0, 0, 128, 160, bgColor);
	ST7735_DrawFastHLine(10, 140, 115, axisColor);
	ST7735_DrawFastVLine(10, 10, 131, axisColor);
		
	for(i=10; i<=120; i=i+5){		// x axis - Ver'
		ST7735_DrawPixel(i, 141, axisColor);
	}
	for(i=10; i<140; i=i+5){	// y axis - Hor'
		ST7735_DrawPixel(9, i, axisColor);
	}
	i = 50;
	while((*xLabel) && (i < 100)){
		ST7735_DrawChar(i, 145, *xLabel, label1Color, bgColor, 1);
		i = i + 6;
		xLabel++;
	}
	if(*yLabel2){ // two labels
	i = 26;
	while((*yLabel2) && (i < 50)){
		ST7735_DrawChar(0, i, *yLabel2, label1Color, bgColor, 1);
		i = i + 8;
		yLabel2++;
	}
		i = 82;
	}else{ // one label
		i = 42;
	}
	while((*yLabel1) && (i < 120)){
		ST7735_DrawChar(0, i, *yLabel1, label1Color, bgColor, 1);
		i = i + 8;
		yLabel1++;
	}
}
   

//------------ST7735_DrawSmallCircle------------
// Draw a small circle (diameter of 6 pixels)
// rectangle at the given coordinates with the given color.
// Requires (11*6+24*2)=114 bytes of transmission (assuming image on screen)
// Input: x     horizontal position of the top left corner of the circle, columns from the left edge
//        y     vertical position of the top left corner of the circle, rows from the top edge
//        color 16-bit color, which can be produced by ST7735_Color565()
// Output: none
int16_t const smallCircle[6][3]=
{
    {2,3,    2},
  {1  ,  4,  4},
 {0   ,   5, 6},
 {0   ,   5, 6},
  {1  ,  4,  4},
    {2,3,    2}
};

void ST7735_DrawSmallCircle(int16_t x, int16_t y, uint16_t color) {
	uint32_t i,w;
	uint8_t hi = color >> 8, lo = color;
	// rudimentary clipping 
	if((x>_width-5)||(y>_height-5)) return; // doesn't fit
	for(i=0; i<6; i++)
	{
	setAddrWindow(x+smallCircle[i][0], y+i, x+smallCircle[i][1], y+i);
	w = smallCircle[i][2];
	while (w--) {
		WriteData(hi);
		WriteData(lo);
	}
	}
	deselect();
}
//------------ST7735_DrawCircle------------
// Draw a small circle (diameter of 10 pixels)
// rectangle at the given coordinates with the given color.
// Requires (11*10+68*2)=178 bytes of transmission (assuming image on screen)
// Input: x     horizontal position of the top left corner of the circle, columns from the left edge
//        y     vertical position of the top left corner of the circle, rows from the top edge
//        color 16-bit color, which can be produced by ST7735_Color565()
// Output: none
int16_t const circle[10][3]=
{
      { 4,5,         2},
    {2   ,   7,      6},
  {1     ,      8,   8},
  {1     ,      8,   8},
 {0      ,       9, 10},
 {0      ,       9, 10},
  {1     ,      8,   8},
  {1     ,      8,   8},
    {2   ,    7,     6},
     {  4,5,         2}
};

void ST7735_DrawCircle(int16_t x, int16_t y, uint16_t color) 
{
	uint32_t i,w;
	uint8_t hi = color >> 8, lo = color;
	// rudimentary clipping 
	if((x>_width-9)||(y>_height-9)) return; // doesn't fit
	for(i=0; i<10; i++)
	{
		setAddrWindow(x+circle[i][0], y+i, x+circle[i][1], y+i);
		w = circle[i][2];
		while (w--) 
		{
			WriteData(hi);
			WriteData(lo);
		}
	}
	deselect();
}

//------------ST7735_Color565------------
// Pass 8-bit (each) R,G,B and get back 16-bit packed color.
// Input: r ST7735_RED value
//        g green value
//        b ST7735_BLUE value
// Output: 16-bit color
uint16_t ST7735_Color565(uint8_t r, uint8_t g, uint8_t b) 
{
	return ((b & 0xF8) << 8) | ((g & 0xFC) << 3) | (r >> 3);
}


//------------ST7735_SwapColor------------
// Swaps the ST7735_RED and ST7735_BLUE values of the given 16-bit packed color;
// green is unchanged.
// Input: x 16-bit color in format B, G, R
// Output: 16-bit color in format R, G, B
uint16_t ST7735_SwapColor(uint16_t x) 
{
	return (x << 11) | (x & 0x07E0) | (x >> 11);
}


//-----------------------fillmessage-----------------------
// Output a 32-bit number in unsigned decimal format
// Input: 32-bit number to be transferST7735_RED
// Output: none
// Variable format 1-10 digits with no space before or after

char Message[12];
uint32_t Messageindex;

void fillmessage(uint32_t n)
{
	// This function uses recursion to convert decimal number
	//   of unspecified length as an ASCII string
	if(n >= 10){
		fillmessage(n/10);
		n = n%10;
	}
	
	Message[Messageindex] = (n+'0'); /* n is between 0 and 9 */
	
	if(Messageindex<11) Messageindex++;
}

//-----------------------ST7735_OutUDec-----------------------
// Output a 32-bit number in unsigned decimal format
// Position determined by ST7735_SetCursor command
// Color set by ST7735_SetTextColor
// Input: 32-bit number to be transferST7735_RED
// Output: none
// Variable format 1-10 digits with no space before or after
void ST7735_OutUDec(uint32_t n)
{
	Messageindex = 0;
	fillmessage(n);
	Message[Messageindex] = 0; // terminate
	ST7735_DrawString(StX,StY,Message,StTextColor);
	StX = StX+Messageindex;
	
	if(StX>20)
	{
		StX = 20;
		ST7735_DrawCharS(StX*6,StY*10,'*',ST7735_RED,ST7735_BLACK, 1);
	}
}



#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH  0x04

//------------ST7735_SetRotation------------
// Change the image rotation.
// Requires 2 bytes of transmission
// Input: m new rotation value (0 to 3)
// Output: none
void ST7735_SetRotation(uint8_t m) 
{
	WriteCommand(ST7735_MADCTL);
	Rotation = m % 4; // can't be higher than 3
	
	switch (Rotation) 
	{
	case 0:
		if (TabColor == INITR_BLACKTAB) 
		{
			WriteData(MADCTL_MX | MADCTL_MY | MADCTL_RGB);
		}
		else 
		{
			WriteData(MADCTL_MX | MADCTL_MY | MADCTL_BGR);
		}
		
		_width  = ST7735_TFTWIDTH;
		_height = ST7735_TFTHEIGHT;
		
		break;
	case 1:
		if (TabColor == INITR_BLACKTAB) 
		{
			WriteData(MADCTL_MY | MADCTL_MV | MADCTL_RGB);
		} else 
		{
			WriteData(MADCTL_MY | MADCTL_MV | MADCTL_BGR);
		}
		
		_width  = ST7735_TFTHEIGHT;
		_height = ST7735_TFTWIDTH;
		
	break;
	case 2:
		if (TabColor == INITR_BLACKTAB) 
		{
			WriteData(MADCTL_RGB);
		} 
		else 
		{
			WriteData(MADCTL_BGR);
		}
		
		_width  = ST7735_TFTWIDTH;
		_height = ST7735_TFTHEIGHT;
		
		break;
	case 3:
		if (TabColor == INITR_BLACKTAB) 
		{
			WriteData(MADCTL_MX | MADCTL_MV | MADCTL_RGB);
		} else 
		{
			WriteData(MADCTL_MX | MADCTL_MV | MADCTL_BGR);
		}
		
		_width  = ST7735_TFTHEIGHT;
		_height = ST7735_TFTWIDTH;
		
	break;
	}

	deselect();
}


//------------ST7735_InvertDisplay------------
// Send the command to invert all of the colors.
// Requires 1 byte of transmission
// Input: i 0 to disable inversion; non-zero to enable inversion
// Output: none
void ST7735_InvertDisplay(int i) 
{
	if(i)
	{
		WriteCommand(ST7735_INVON);
	} 
	else
	{
		WriteCommand(ST7735_INVOFF);
	}  
	
	deselect();
}
// graphics routines
// y coordinates 0 to 31 used for labels and messages
// y coordinates 32 to 159  128 pixels high
// x coordinates 0 to 127   128 pixels wide

int32_t Ymax,Ymin,X;        // X goes from 0 to 127
int32_t Yrange; //YrangeDiv2;

// *************** ST7735_PlotClear ********************
// Clear the graphics buffer, set X coordinate to 0
// This routine clears the display
// Inputs: ymin and ymax are range of the plot
// Outputs: none
void ST7735_PlotClear(int32_t ymin, int32_t ymax)
{
	ST7735_FillRect(0, 32, 128, 128, ST7735_Color565(228,228,228)); // light grey
	
	if(ymax>ymin)
	{
		Ymax = ymax;
		Ymin = ymin;
		Yrange = ymax-ymin;
	} 
	else
	{
		Ymax = ymin;
		Ymin = ymax;
		Yrange = ymax-ymin;
	}
	
	//YrangeDiv2 = Yrange/2;
	X = 0;
}

// *************** ST7735_PlotPoint ********************
// Used in the voltage versus time plot, plot one point at y
// It does output to display
// Inputs: y is the y coordinate of the point plotted
// Outputs: none
void ST7735_PlotPoint2(int32_t y)
{
	int32_t j;
	if(y<Ymin) y=Ymin;
	if(y>Ymax) y=Ymax;
	
	// X goes from 0 to 127
	// j goes from 159 to 32
	// y=Ymax maps to j=32
	// y=Ymin maps to j=159
	
	j = 32+(127*(Ymax-y))/Yrange;
	
	if(j<32) j = 32;
	if(j>159) j = 159;
	
	ST7735_DrawPixel(X,   j,   ST7735_BLUE);
	ST7735_DrawPixel(X+1, j,   ST7735_BLUE);
	ST7735_DrawPixel(X,   j+1, ST7735_BLUE);
	ST7735_DrawPixel(X+1, j+1, ST7735_BLUE);
}

// *************** ST7735_PlotPoints ********************
// Used in the voltage versus time plot, plot two points at y1, y2
// It does output to display
// Inputs: y1 is the y coordinate of the first point plotted
//         y2 is the y coordinate of the second point plotted
// Outputs: none
void ST7735_PlotPoints(int32_t y1,int32_t y2)
{	
	int32_t j;
	
	if(y1<Ymin) y1=Ymin;
	if(y1>Ymax) y1=Ymax;
	
	// X goes from 0 to 127
	// j goes from 159 to 32
	// y=Ymax maps to j=32
	// y=Ymin maps to j=159
	
	j = 32+(127*(Ymax-y1))/Yrange;
	
	if(j<32) j = 32;
	if(j>159) j = 159;
	
	ST7735_DrawPixel(X, j, ST7735_BLUE);
	
	if(y2<Ymin) y2=Ymin;
	if(y2>Ymax) y2=Ymax;
	
	j = 32+(127*(Ymax-y2))/Yrange;
	
	if(j<32) j = 32;
	if(j>159) j = 159;
	
	ST7735_DrawPixel(X, j, ST7735_BLACK);
}


// full scaled defined as 3V
// Input is 0 to 511, 0 => 159 and 511 => 32
uint8_t const dBfs[512]=
{
	159, 159, 145, 137, 131, 126, 123, 119, 117, 114, 112, 110, 108, 107, 105, 104, 103, 101,
	100, 99, 98, 97, 96, 95, 94, 93, 93, 92, 91, 90, 90, 89, 88, 88, 87, 87, 86, 85, 85, 84,
	84, 83, 83, 82, 82, 81, 81, 81, 80, 80, 79, 79, 79, 78, 78, 77, 77, 77, 76, 76, 76, 75,
	75, 75, 74, 74, 74, 73, 73, 73, 72, 72, 72, 72, 71, 71, 71, 71, 70, 70, 70, 70, 69, 69,
	69, 69, 68, 68, 68, 68, 67, 67, 67, 67, 66, 66, 66, 66, 66, 65, 65, 65, 65, 65, 64, 64,
	64, 64, 64, 63, 63, 63, 63, 63, 63, 62, 62, 62, 62, 62, 62, 61, 61, 61, 61, 61, 61, 60,
	60, 60, 60, 60, 60, 59, 59, 59, 59, 59, 59, 59, 58, 58, 58, 58, 58, 58, 58, 57, 57, 57,
	57, 57, 57, 57, 56, 56, 56, 56, 56, 56, 56, 56, 55, 55, 55, 55, 55, 55, 55, 55, 54, 54,
	54, 54, 54, 54, 54, 54, 53, 53, 53, 53, 53, 53, 53, 53, 53, 52, 52, 52, 52, 52, 52, 52,
	52, 52, 52, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 50, 50, 50, 50, 50, 50, 50, 50, 50,
	50, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
	48, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 46, 46, 46, 46, 46, 46, 46, 46, 46,
	46, 46, 46, 46, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 44, 44, 44, 44, 44,
	44, 44, 44, 44, 44, 44, 44, 44, 44, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43,
	43, 43, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 41, 41, 41, 41, 41,
	41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
	40, 40, 40, 40, 40, 40, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
	39, 39, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 37,
	37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 36, 36, 36, 36,
	36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 35, 35, 35, 35, 35,
	35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 34, 34, 34, 34, 34, 34,
	34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 33, 33, 33, 33, 33,
	33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 32, 32, 32,
	32, 32, 32, 32, 32, 32, 32, 32, 32, 32
};

// *************** ST7735_PlotdBfs ********************
// Used in the amplitude versus frequency plot, plot bar point at y
// 0 to 0.625V scaled on a log plot from min to max
// It does output to display
// Inputs: y is the y ADC value of the bar plotted
// Outputs: none
void ST7735_PlotdBfs(int32_t y){
	
	int32_t j;
	y = y/2; // 0 to 2047
	
	if(y<0) y=0;
	if(y>511) y=511;
	
	// X goes from 0 to 127
	// j goes from 159 to 32
	// y=511 maps to j=32
	// y=0 maps to j=159
	
	j = dBfs[y];
	
	ST7735_DrawFastVLine(X, j, 159-j, ST7735_BLACK);
}

// *************** ST7735_PlotNext ********************
// Used in all the plots to step the X coordinate one pixel
// X steps from 0 to 127, then back to 0 again
// It does not output to display
// Inputs: none
// Outputs: none
void ST7735_PlotNext(void){
	
	if(X==127)
	{
		X = 0;
	} 
	else
	{
		X++;
	}
}

// *************** ST7735_PlotNextErase ********************
// Used in all the plots to step the X coordinate one pixel
// X steps from 0 to 127, then back to 0 again
// It clears the vertical space into which the next pixel will be drawn
// Inputs: none
// Outputs: none
void ST7735_PlotNextErase(void){
  if(X==127){
    X = 0;
  } else{
    X++;
  }
  ST7735_DrawFastVLine(X,32,128,ST7735_Color565(228,228,228));
}

void ST7735_PlotPoint(int32_t data1, uint16_t color1){
  data1 = ((data1 - Ymin)*100)/Yrange;
  if(data1 > 139){
    data1 = 139;
    color1 = ST7735_RED;
  }
  if(data1 < 0){
    data1 = 0;
    color1 = ST7735_RED;
  }
  ST7735_DrawPixel(TimeIndex + 11, 139 - data1, color1);
//  ST7735_DrawPixel(TimeIndex + 11, 138 - data1, color1);	// Maki line width bold
//  ST7735_DrawPixel(TimeIndex + 11, 137 - data1, color1);
}
// Used in all the plots to write buffer to LCD
// Example 1 Voltage versus time
//    ST7735_PlotClear(0,4095);  // range from 0 to 4095
//    ST7735_PlotPoint(data); ST7735_PlotNext(); // called 128 times

// Example 2a Voltage versus time (N data points/pixel, time scale)
//    ST7735_PlotClear(0,4095);  // range from 0 to 4095
//    {   for(j=0;j<N;j++){
//          ST7735_PlotPoint(data[i++]); // called N times
//        }
//        ST7735_PlotNext();
//    }   // called 128 times

// Example 2b Voltage versus time (N data points/pixel, time scale)
//    ST7735_PlotClear(0,4095);  // range from 0 to 4095
//    {   for(j=0;j<N;j++){
//          ST7735_PlotLine(data[i++]); // called N times
//        }
//        ST7735_PlotNext();
//    }   // called 128 times

// Example 3 Voltage versus frequency (512 points)
//    perform FFT to get 512 magnitudes, mag[i] (0 to 4095)
//    ST7735_PlotClear(0,1023);  // clip large magnitudes
//    {
//        ST7735_PlotBar(mag[i++]); // called 4 times
//        ST7735_PlotBar(mag[i++]);
//        ST7735_PlotBar(mag[i++]);
//        ST7735_PlotBar(mag[i++]);
//        ST7735_PlotNext();
//    }   // called 128 times

// Example 4 Voltage versus frequency (512 points), dB scale
//    perform FFT to get 512 magnitudes, mag[i] (0 to 4095)
//    ST7735_PlotClear(0,511);  // parameters ignoST7735_RED
//    {
//        ST7735_PlotdBfs(mag[i++]); // called 4 times
//        ST7735_PlotdBfs(mag[i++]);
//        ST7735_PlotdBfs(mag[i++]);
//        ST7735_PlotdBfs(mag[i++]);
//        ST7735_PlotNext();
//    }   // called 128 times

// *************** ST7735_OutChar ********************
// Output one character to the LCD
// Position determined by ST7735_SetCursor command
// Color set by ST7735_SetTextColor
// Inputs: 8-bit ASCII character
// Outputs: none
void ST7735_OutChar(char ch){
  if((ch == 10) || (ch == 13) || (ch == 27)){
    StY++; StX=0;
    if(StY>15){
      StY = 0;
    }
    ST7735_DrawString(0,StY,"                     ",StTextColor);
    return;
  }
  ST7735_DrawCharS(StX*6,StY*10,ch,ST7735_YELLOW,ST7735_BLACK, 1);
  StX++;
  if(StX>20){
    StX = 20;
    ST7735_DrawCharS(StX*6,StY*10,'*',ST7735_RED,ST7735_BLACK, 1);
  }
  return;
}
//********ST7735_OutString*****************
// Print a string of characters to the ST7735 LCD.
// Position determined by ST7735_SetCursor command
// Color set by ST7735_SetTextColor
// The string will not automatically wrap.
// inputs: ptr  pointer to NULL-terminated ASCII string
// outputs: none
void ST7735_OutString(char *ptr){
  while(*ptr){
    ST7735_OutChar(*ptr);
    ptr = ptr + 1;
  }
}
// ************** ST7735_SetTextColor ************************
// Sets the color in which the characters will be printed
// Background color is fixed at ST7735_BLACK
// Input:  16-bit packed color
// Output: none
// ********************************************************
void ST7735_SetTextColor(uint16_t color){
  StTextColor = color;
}

// Print a character to ST7735 LCD.
int fputc(int ch, FILE *f){
  ST7735_OutChar(ch);
  return 1;
}
// No input from Nokia, always return data.
int fgetc (FILE *f){
  return 0;
}
// Function called when file error occurs.
int ferror(FILE *f){
  /* Your implementation of ferror */
  return EOF;
}
// Abstraction of general output device
// Volume 2 section 3.4.5

// *************** Output_Init ********************
// Standard device driver initialization function for printf
// Initialize ST7735 LCD
// Inputs: none
// Outputs: none
void Output_Init(void){
  ST7735_InitR(INITR_REDTAB);
  ST7735_FillScreen(0);                 // set screen to ST7735_BLACK
}

// Clear display
void Output_Clear(void){ // Clears the display
  ST7735_FillScreen(0);    // set screen to ST7735_BLACK
}
// Turn off display (low power)
void Output_Off(void){   // Turns off the display
  Output_Clear();  // not implemented
}
// Turn on display
void Output_On(void){ // Turns on the display
  Output_Init();      // reinitialize
}
// set the color for future output
// Background color is fixed at ST7735_BLACK
// Input:  16-bit packed color
// Output: none
void Output_Color(uint32_t newColor){ // Set color of future output
  ST7735_SetTextColor(newColor);
}
void static deselect(void) {
                                        // wait until SSI0 not busy/transmit FIFO empty
  while((SSI0->SR&SSI_SR_BSY)==SSI_SR_BSY){};
  TFT_CS = TFT_CS_HIGH;    
}

void DrawGrid(uint16_t axisColor)
{
	int i, j;
	for(j=0; j < 160; j=j+10)
	{
	for(i = 0; i <= 128; i = i + 10){
		ST7735_DrawFastHLine(i, j, 115, axisColor);
		ST7735_DrawFastVLine(j, i, 131, axisColor);
	}
}
}

