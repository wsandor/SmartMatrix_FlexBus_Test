/*
 * SmartMatrix Library - Refresh Code for Teensy 3.x Platform
 *
 * Copyright (c) 2015 Louis Beaudoin (Pixelmatix)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "SmartMatrix3FB.h"
#include "DMAChannel.h"
#include "FlexBus.h"

#define INLINE __attribute__( ( always_inline ) ) inline

// hardware-specific definitions
// prescale of 1 is F_BUS/2
#define LATCH_TIMER_PRESCALE  0x02   //1
#define NS_TO_TICKS(X)      (uint32_t)(TIMER_FREQUENCY * ((X) / 1000000000.0))

#define TIMER_FREQUENCY     (F_BUS/(1 << LATCH_TIMER_PRESCALE))  //2

#define LATCH_TIMER_PULSE_WIDTH_TICKS   NS_TO_TICKS(LATCH_TIMER_PULSE_WIDTH_NS)
#define TICKS_PER_ROW   (TIMER_FREQUENCY/refreshRate/MATRIX_SCAN_MOD)
#define IDEAL_MSB_BLOCK_TICKS  (TICKS_PER_ROW * (1 << (LATCHES_PER_ROW - 1))) / ((1 << LATCHES_PER_ROW) - 1)   //(TICKS_PER_ROW/2)
#define MIN_BLOCK_PERIOD_NS (LATCH_TO_CLK_DELAY_NS + ((PANEL_32_PIXELDATA_TRANSFER_MAXIMUM_NS*PIXELS_PER_LATCH)/32))
#define MIN_BLOCK_PERIOD_TICKS NS_TO_TICKS(MIN_BLOCK_PERIOD_NS)

#define PWM_Freq 300000               // Hz OE PWM freki  320KHz fölött már olyan a hatása, mintha nem lenne kikapcsolva a PWM a küldés alatt
#define MAX_REFRESH_RATE    (500)     // Hz This is the max phisical refresh rate on the LED matrix above it ghosting occurs - may depend on panel

extern DMAChannel dmaClockOutData;
static IntervalTimer rowSendTimer;

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
void rowShiftCompleteISR(void);

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
void rowShiftStartISR(void);

//template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
//CircularBuffer SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::dmaBuffer;

// dmaBufferNumRows = the size of the buffer that DMA pulls from to refresh the display
// must be minimum 2 rows so one can be updated while the other is refreshed
// increase beyond two to give more time for the update routine to complete
// (increase this number if non-DMA interrupts are causing display problems)
template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
uint8_t SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::dmaBufferNumRows;
template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
uint16_t SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::refreshRate = 200;

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
uint16_t SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::rowBitStructBytesToShift;
template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
int SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::timerValues[COLOR_DEPTH_BITS];
/*
    THIS IS NOW OUT OF DATE: data is now arranged linearly sorted first by pixels, then by color bit
  buffer contains:
    COLOR_DEPTH/COLOR_CHANNELS_PER_PIXEL/sizeof(int32_t) * (2 words for each pair of pixels: pixel data from n, and n+matrixRowPairOffset)
      first half of the words contain a byte for each shade, going from LSB to MSB
      second half of the words have the same data, plus a high bit in each byte for the clock
    there are MATRIX_WIDTH number of these in order to refresh a row (pair of rows)
    data is organized: matrixUpdateData[row][pixels within row][color depth * 2 updates per clock]

    DMA doesn't shift out the data sequentially, for each row, DMA goes through the buffer matrixUpdateData[currentrow][]

    layout of single matrixUpdateData row:
    in drawing, [] = byte, data is arranged as uint32_t going left to right in the drawing, "clk" is low, "CLK" is high
    DMA goes down one column of the drawing per latch signal trigger, resetting back to the top + shifting 1 byte to the right for the next latch trigger
    [pixel pair  0 - clk - MSB][pixel pair  0 - clk - MSB-1]...[pixel pair  0 - clk - LSB+1][pixel pair  0 - clk - LSB]
    [pixel pair  0 - CLK - MSB][pixel pair  0 - CLK - MSB-1]...[pixel pair  0 - CLK - LSB+1][pixel pair  0 - CLK - LSB]
    [pixel pair  1 - clk - MSB][pixel pair  1 - clk - MSB-1]...[pixel pair  1 - clk - LSB+1][pixel pair  1 - clk - LSB]
    [pixel pair  1 - CLK - MSB][pixel pair  1 - CLK - MSB-1]...[pixel pair  1 - CLK - LSB+1][pixel pair  1 - CLK - LSB]
    ...
    [pixel pair 15 - clk - MSB][pixel pair 15 - clk - MSB-1]...[pixel pair 15 - clk - LSB+1][pixel pair 15 - clk - LSB]
    [pixel pair 15 - CLK - MSB][pixel pair 15 - CLK - MSB-1]...[pixel pair 15 - CLK - LSB+1][pixel pair 15 - CLK - LSB]
 */
template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
typename SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::rowDataStruct * SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::matrixUpdateRows;

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::SmartMatrix3RefreshFlexBusSimple(uint8_t bufferrows, rowDataStruct * rowDataBuffer) {
    dmaBufferNumRows = bufferrows;
    matrixUpdateRows = rowDataBuffer;
}

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
typename SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::rowDataStruct * SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::getNextRowBufferPtr(uint8_t row) {
    SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::actRowWrite = row;
    return &(matrixUpdateRows[SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::actWriteOffset + row]);
}

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
uint16_t SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::getRefreshRate() {
    return refreshRate;
};

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
typename SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::matrix_underrun_callback SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::matrixUnderrunCallback;

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
typename SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::matrix_calc_callback SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::matrixCalcCallback;

#define MSB_BLOCK_TICKS_ADJUSTMENT_INCREMENT    10

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
void SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::calculateTimerLUT(void) {
 /*   int i;
    uint32_t ticksUsed;
    uint16_t msbBlockTicks = IDEAL_MSB_BLOCK_TICKS + MSB_BLOCK_TICKS_ADJUSTMENT_INCREMENT;

		
	// TICKS_PER_ROW   (TIMER_FREQUENCY/refreshRate/MATRIX_SCAN_MOD)
    // IDEAL_MSB_BLOCK_TICKS     (TICKS_PER_ROW/2)
	// MIN_BLOCK_PERIOD_NS (LATCH_TO_CLK_DELAY_NS + ((PANEL_32_PIXELDATA_TRANSFER_MAXIMUM_NS*PIXELS_PER_LATCH)/32))
	// MIN_BLOCK_PERIOD_TICKS NS_TO_TICKS(MIN_BLOCK_PERIOD_NS)
	
    // start with ideal width of the MSB, and keep lowering until the width of all bits fits within TICKS_PER_ROW
    do {
        ticksUsed = 0;
        msbBlockTicks -= MSB_BLOCK_TICKS_ADJUSTMENT_INCREMENT;
        for (i = 0; i < LATCHES_PER_ROW; i++) {
            uint16_t blockTicks = (msbBlockTicks >> (LATCHES_PER_ROW - i - 1)) + LATCH_TIMER_PULSE_WIDTH_TICKS;

            if (blockTicks < MIN_BLOCK_PERIOD_TICKS)
                blockTicks = MIN_BLOCK_PERIOD_TICKS;

            ticksUsed += blockTicks;
        }
    } while (ticksUsed > TICKS_PER_ROW);
		
    for (i = 0; i < LATCHES_PER_ROW; i++) {
        // set period and OE values for current block - going from smallest timer values to largest
        // order needs to be smallest to largest so the last update of the row has the largest time between
        // the falling edge of the latch and the rising edge of the latch on the next row - an ISR
        // updates the row in this time

        // period is max on time for this block, plus the dead time while the latch is high
        uint16_t period = (msbBlockTicks >> (LATCHES_PER_ROW - i - 1)) + LATCH_TIMER_PULSE_WIDTH_TICKS;
        // on-time is the max on-time * dimming factor, plus the dead time while the latch is high
        uint16_t ontime = (((msbBlockTicks >> (LATCHES_PER_ROW - i - 1)) * dimmingFactor) / dimmingMaximum) + LATCH_TIMER_PULSE_WIDTH_TICKS;
				
        if (period < MIN_BLOCK_PERIOD_TICKS) {
            uint16_t padding = (MIN_BLOCK_PERIOD_TICKS) - period;
            period += padding;
            ontime += padding;
        }
        
        // add extra padding once per latch to match refreshRate exactly?  Doesn't seem to make a big difference
#if 0        
        if(!i) {
            uint16_t padding = TICKS_PER_ROW/2 - msbBlockTicks;
            period += padding;
            ontime += padding;
        }
#endif
        timerLUT[i].timer_period = period;
        timerLUT[i].timer_oe = ontime;
        //Serial.printf("i: %i per: %i oe: %i\n", i, period, ontime);
    }
#if defined(PWM_ON_OE)
    TPM1_MOD = timerLUT[0].timer_period >> 2;
    TPM1_C1V = timerLUT[0].timer_oe >> 2;  //Set PWM according to smallest bit values
    //Serial.printf("mod: %i C1V: %i\n", TPM1_MOD, TPM1_C1V);
    //Serial.printf("SC: %X C1SC: %X\n", TPM1_SC, TPM1_C1SC);
#endif */
}

// 255 full brightness, default is half brightness
template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
int SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::brightness = 128;

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
void SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::setBrightness(uint8_t newBrightness) {
    brightness = newBrightness;   
}

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
void SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::swapRefreshBuffers() {
    //Serial.println("swapstart");
    while (!SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::canSwitchBuffers) {}
    uint8_t u = SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::actReadOffset;
    SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::actReadOffset = SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::actWriteOffset;
    SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::actWriteOffset = u;
    //Serial.println("swapend");
}


template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
volatile uint8_t SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::actReadOffset = 0;

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
volatile uint8_t SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::actWriteOffset = 0;

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
volatile uint16_t SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::actRowRead = 0;

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
volatile uint8_t SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::actRowReadBit = 0;

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
volatile uint8_t SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::actRowWrite = MATRIX_SCAN_MOD;

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
volatile bool SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::canSwitchBuffers = true;


template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
void SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::begin(void) {
    
    actRowRead = 0;
    actRowReadBit = 0;
    actRowWrite = 0;
    actReadOffset = 0;
    actWriteOffset = MATRIX_SCAN_MOD; //dmaBufferNumRows / 2;

    // fill timerLUT
//    calculateTimerLUT();

    // completely fill buffer with data before enabling DMA
    //matrixCalcCallback(true);

    // setup debug output
#ifdef DEBUG_PINS_ENABLED
    pinMode(DEBUG_PIN_1, OUTPUT);
    digitalWriteFast(DEBUG_PIN_1, HIGH); // oscilloscope trigger
    digitalWriteFast(DEBUG_PIN_1, LOW);
    pinMode(DEBUG_PIN_2, OUTPUT);
    digitalWriteFast(DEBUG_PIN_2, HIGH); // oscilloscope trigger
    digitalWriteFast(DEBUG_PIN_2, LOW);
    pinMode(DEBUG_PIN_3, OUTPUT);
    digitalWriteFast(DEBUG_PIN_3, HIGH); // oscilloscope trigger
    digitalWriteFast(DEBUG_PIN_3, LOW);
#endif
    /* FlexBus init */
    #define ALT5 (PORT_PCR_MUX(5) | PORT_PCR_DSE | PORT_PCR_SRE) // Alternative function 5 = FB enable
    SIM_CLKDIV1 |= SIM_CLKDIV1_OUTDIV3(1);        // FlexBus = Sysclk      //180MHz-nél: 0-ra nem jó, 1:12.8MHz, 2:12MHz, 3, 4:9MHz, 5: 7.5MHz
   // SIM_SCGC5 |= SIM_SCGC5_PORTA | SIM_SCGC5_PORTB | SIM_SCGC5_PORTC | SIM_SCGC5_PORTD | SIM_SCGC5_PORTE;
    
    PORTD_PCR1 = ALT5;  //CS0
    
    PORTB_PCR11 = ALT5; //ADDR  AD18     
    //PORTB_PCR16 = ALT5; //ADDR teszt 

    PORTD_PCR6 = ALT5;  //R1A   AD0
    PORTD_PCR5 = ALT5;  //G1A   AD1
    PORTD_PCR4 = ALT5;  //B1A   AD2
    PORTD_PCR3 = ALT5;  //R2A   AD3
    PORTD_PCR2 = ALT5;  //G2A   AD4
    PORTC_PCR10 = ALT5; //B2A   AD5
    PORTC_PCR9 = ALT5;  //R1B   AD6
    PORTC_PCR8 = ALT5;  //G1B   AD7
    PORTC_PCR7 = ALT5;  //B1B   AD8
    PORTC_PCR6 = ALT5;  //R2B   AD9
    PORTC_PCR4 = ALT5;  //G2B   AD10
    PORTC_PCR2 = ALT5;  //B2B   AD11

    // enable clock to the FlexBus module 
    SIM_SOPT2 |= SIM_SOPT2_FBSL(3);
    SIM_SCGC7 |= SIM_SCGC7_FLEXBUS;

    #define RGB_ADDRESS 0x60000000   //ide kell az RGB adatot írni
    #define LAT_ADDRESS 0x60040000   // ide kell írni a LATCH pulzushoz 
    
    FB_CSAR0 = RGB_ADDRESS; // CS0 base address

    // MUX mode + Wait States + right aligned data + 16 bit portsize + Auto-Acknowledge Enable
    FB_CSCR0 = FB_CSCR_BLS | FB_CSCR_AA | FB_CSCR_PS(2) | FB_CSCR_WS(0) | FB_CSCR_WRAH(0) | FB_CSCR_ASET(0); // FlexBus setup as fast as possible in multiplexed mode

    #define FLEX_ADRESS_MASK 0x00040000
    FB_CSMR0 = FLEX_ADRESS_MASK | FB_CSMR_V; // The address range is set to 128K

    
    // configure the 7 output pins (one pin is left as input, though it can't be used as GPIO output)
    //pinMode(GPIO_PIN_CLK_TEENSY_PIN, OUTPUT);
    //pinMode(GPIO_PIN_B0_TEENSY_PIN, OUTPUT);
    //pinMode(GPIO_PIN_R0_TEENSY_PIN, OUTPUT);
    //pinMode(GPIO_PIN_R1_TEENSY_PIN, OUTPUT);
    //pinMode(GPIO_PIN_G0_TEENSY_PIN, OUTPUT);
    //pinMode(GPIO_PIN_G1_TEENSY_PIN, OUTPUT);
    //pinMode(GPIO_PIN_B1_TEENSY_PIN, OUTPUT);
	//pinMode(GPIO_PIN_ROWSEL_TEENSY_PIN, OUTPUT);


    rowBitStructBytesToShift = WRITES_PER_LATCH_16; //(sizeof(SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::matrixUpdateRows[0].rowbits[0].data) / 2) + ADDX_UPDATE_BEFORE_LATCH_BYTES;
    
    if(optionFlags & SMARTMATRIX_OPTIONS_HUB12_MODE) {    
        // HUB12 format inverts the OE channel
      //  FTM1_POL = FTM_POL_POL1;
    }

    // enable clocks to the DMA controller and DMAMUX
    SIM_SCGC7 |= SIM_SCGC7_DMA;
    SIM_SCGC6 |= SIM_SCGC6_DMAMUX;

    // enable minor loop mapping so addresses can get reset after minor loops
	// not needed WS
    //DMA_CR |= DMA_CR_EMLM;

    // allocate all DMA channels up front so channels can link to each other
    dmaClockOutData.begin(false);


#define DMA_TCD_MLOFF_MASK  (0x3FFFFC00)

    // dmaClockOutData - repeatedly load gpio_array into GPIOD_PDOR, stop and int on major loop complete
    dmaClockOutData.TCD->SADDR = matrixUpdateRows[0].rowbits[0].data;
	dmaClockOutData.TCD->SOFF = 2;
    // SADDR will get updated by ISR, no need to set SLAST
    dmaClockOutData.TCD->SLAST = 0;
    dmaClockOutData.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);  //16bit source and destination size
	dmaClockOutData.TCD->NBYTES_MLNO = rowBitStructBytesToShift * sizeof(uint16_t);
/*	if (COLOR_DEPTH_BITS == 1) // 3-bit color  //WS
	// more than 1024 pixels can be shifted out wit 1bit/color
		dmaClockOutData.TCD->NBYTES_MLOFFNO = rowBitStructBytesToShift; 
	else {
    // after each minor loop, apply no offset to source data, it's pointing to the next buffer already
    // clock out (PIXELS_PER_LATCH * DMA_UPDATES_PER_CLOCK + ADDX_UPDATE_BEFORE_LATCH_BYTES) number of bytes per loop
	// Max 1024 pixels can be shifted out here!!!!
		dmaClockOutData.TCD->NBYTES_MLOFFYES = DMA_TCD_NBYTES_SMLOE | ((rowBitStructDataOffset << 10) & DMA_TCD_MLOFF_MASK) | (rowBitStructBytesToShift & 0x3FF); 
	}  */

    volatile void* flexBusAddr = (void *)RGB_ADDRESS;

    dmaClockOutData.TCD->DADDR = flexBusAddr; //FlexBus       &GPIOD_PDOR;  
    dmaClockOutData.TCD->DOFF = 0;
    dmaClockOutData.TCD->DLASTSGA = 0;
    dmaClockOutData.TCD->CITER_ELINKNO = 1; //LATCHES_PER_ROW;
    dmaClockOutData.TCD->BITER_ELINKNO = 1; //LATCHES_PER_ROW;
    // int after major loop is complete
    dmaClockOutData.TCD->CSR = DMA_TCD_CSR_INTMAJOR;
	    
    // for debugging - enable bandwidth control (space out GPIO updates so they can be seen easier on a low-bandwidth logic analyzer)
    // enable for now, until DMA sharing complications (brought to light by Teensy 3.6 SDIO) can be worked out - use bandwidth control to space out our DMA access and allow SD reads to not slow down shifting to the matrix
    // also enable for now, until it can be selectively enabled for higher clock speeds (140MHz+) where the data rate is too high for the panel
    
    dmaClockOutData.TCD->CSR |= (0x02 << 14);

    // enable a done interrupt when all DMA operations are complete
    //NVIC_SET_PRIORITY(IRQ_DMA_CH0 + dmaClockOutData.channel, 16);

    dmaClockOutData.attachInterrupt(rowShiftCompleteISR<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>);
    dmaClockOutData.enable();
    analogWriteRes(8);
    analogWriteFrequency(Buffer_OE, PWM_Freq);     //OE PWM freki 500KHz
        
    /*
      Úgy állítjuk be a bitidőket, hogy a legkissebbe beleférjen legalább 1 PWM ciklus, hogy a legkissebb bit fényereje is jó legyen,
      de a frissítési frekvencia ne legyen nagyobb a beéllított MAX_REFRESH_RATE-nél, mert tapasztalat szerint e fölött ghosting lehet.
      Ezt paneltípus határozza mg úgyhogy új paneleknél ki kell probálni mit bír max.
      MOD1-es paneleknél a MAX_REFRESH_RATE-et följebb lehet vinni, mert ott nem lesz ghosting!
    */

    int mul = 1;
    int sum = 0;
    do {
        sum = 0;
        for (int i = 0; i < COLOR_DEPTH_BITS; i++) {
            timerValues[i] = ((1000000 * (1 << i) * mul)/ PWM_Freq) + (((WRITES_PER_LATCH_16 * PANEL_PIXELDATA_TRANSFER_MAXIMUM_NS) + 999) / 1000);
            sum+= timerValues[i];
            //Serial.printf("%i - %i\n", i, timerValues[i]); 
        }
        //Serial.printf("Freq: %i\n", 1000000 / (sum * MATRIX_SCAN_MOD)); 
        mul++;
    }
    while ((1000000 / (sum * MATRIX_SCAN_MOD)) > MAX_REFRESH_RATE); 
    refreshRate = 1000000 / (sum * MATRIX_SCAN_MOD);
    rowSendTimer.priority(ROW_SEND_ISR_PRIORITY);
    rowSendTimer.begin(rowShiftStartISR<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>, timerValues[0]);  // rowSendIT to run as 1000 frames per second refresh rate
    
}


// interrupt triggered by timer
template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
void rowShiftStartISR(void) {
#ifdef DEBUG_PINS_ENABLED
    digitalWriteFast(DEBUG_PIN_2, HIGH); // oscilloscope trigger
#endif
    if (++SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::actRowReadBit >= COLOR_DEPTH_BITS) {
        SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::actRowReadBit = 0;
        SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::actRowRead = (SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::actRowRead + 1) % MATRIX_SCAN_MOD;
    }
    int t = SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::timerValues[(SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::actRowReadBit + 1) % COLOR_DEPTH_BITS];
    rowSendTimer.update(t); //ez a következő ciklust állítja, nem az aktuálisat!!
    analogWrite(Buffer_OE, 256);  //Turn Off PWM on OE 
    //digitalWriteFast(Buffer_OE, HIGH);
    //PORTA_PCR13 = PORT_PCR_MUX(1) | PORT_PCR_DSE | PORT_PCR_SRE;

    SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::canSwitchBuffers = false;   //indul a küldés, nem lehet váltani
    dmaClockOutData.TCD->SADDR = (uint16_t*)&SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::matrixUpdateRows[SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::actRowRead + SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::actReadOffset].rowbits[SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::actRowReadBit].data;    
    dmaClockOutData.triggerManual();

    /*Serial.printf("r:%i, b:%i\n", SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::actRowRead, SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::actRowReadBit);
    for (int i = 0; i<WRITES_PER_LATCH_16; i++) {
        Serial.printf("%4x,", *SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::matrixUpdateRows[SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::actRowRead + SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::actReadOffset + i].rowbits[SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::actRowReadBit].data);
    }
    Serial.println();
    */
#ifdef DEBUG_PINS_ENABLED
    digitalWriteFast(DEBUG_PIN_2, LOW);
#endif
}


// DMA transfer done (meaning data was shifted out)
// Make latch pulse, step row, turn pwm (off) and on
template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
void rowShiftCompleteISR(void) {

//Serial.println("|");
#ifdef DEBUG_PINS_ENABLED
    digitalWriteFast(DEBUG_PIN_1, HIGH); // oscilloscope trigger
#endif
    // row address + Latch generálása
    *((unsigned uint16_t*)LAT_ADDRESS) = SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::actRowRead;
  
    SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::canSwitchBuffers = true;  //nincs küldés éppen, mehet a váltás
    
    dmaClockOutData.clearComplete();  //ez nem tudom kell-e
    // clear pending int
    dmaClockOutData.clearInterrupt();  //de ez biztosan
    
    //tbd itt csak a timer kimenetét visszakapcsolni nem a PWM-et beállítani hátha úgy jobb bit0-nál
    //PORTA_PCR13 = PORT_PCR_MUX(3) | PORT_PCR_DSE | PORT_PCR_SRE;

    analogWrite(Buffer_OE, 255 - SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::brightness); //Turn On PWM on OE 

#ifdef DEBUG_PINS_ENABLED
    digitalWriteFast(DEBUG_PIN_1, LOW); // oscilloscope trigger
#endif
}