/*
 * SmartMatrix Library - Multiplexed Panel Refresh Class
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

#ifndef SmartMatrixFlexBusSimpleRefresh_h
#define SmartMatrixFlexBusSimpleRefresh_h

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
class SmartMatrix3RefreshFlexBusSimple {
public:
    
    struct rowBitStruct {
		uint16_t data[WRITES_PER_LATCH_16];
        //uint8_t rowAddress; // must be directly after data - DMA transfers data[] + rowAddress continuous
    };

    struct rowDataStruct {
        rowBitStruct rowbits[refreshDepth/COLOR_CHANNELS_PER_PIXEL];
    };

    typedef void (*matrix_underrun_callback)(void);
    typedef void (*matrix_calc_callback)(void); //(bool initial);

    static int timerValues[COLOR_DEPTH_BITS];

    // init
    SmartMatrix3RefreshFlexBusSimple(uint8_t bufferrows, rowDataStruct * rowDataBuffer);
    static void begin(void);

    // refresh API
    static rowDataStruct * getNextRowBufferPtr(uint8_t row);
    static void setBrightness(uint8_t newBrightness);
    static uint16_t getRefreshRate();

    static volatile bool canSwitchBuffers;

    static void swapRefreshBuffers();

private:
    // enable ISR access to private member variables
    template <int refreshDepth1, int matrixWidth1, int matrixHeight1, unsigned char panelType1, unsigned char optionFlags1>
    friend void rowShiftStartISR(void);

    template <int refreshDepth1, int matrixWidth1, int matrixHeight1, unsigned char panelType1, unsigned char optionFlags1>
    friend void rowShiftCompleteISR(void);

    // configuration helper functions
    static void calculateTimerLUT(void);

    static int brightness;
    static uint16_t rowBitStructBytesToShift;
    static uint16_t refreshRate;
    static uint8_t dmaBufferNumRows;
    static rowDataStruct * matrixUpdateRows;

    static matrix_calc_callback matrixCalcCallback;
    static matrix_underrun_callback matrixUnderrunCallback;

    //static CircularBuffer dmaBuffer;
    /*
    A bufferben 2x annyi sor van, mint amennyi kell - az egyik feléből olvas és küld foylamatosan, a másik felébe írhat
    váltás az actReadOffset és actWriteOffset cseréjével lehetséges. Az aktuális olvasási index = actReadOffset + actRowRead
    */
    volatile static uint16_t actRowRead;
    volatile static uint8_t actRowReadBit;
    volatile static uint8_t actRowWrite;
    volatile static uint8_t actReadOffset;
    volatile static uint8_t actWriteOffset;

};

#endif
