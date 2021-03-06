/*
 * SmartMatrix Library - Hardware-Specific Header File (for SmartMatrix Shield V4)
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

 // Note: only one MatrixHardware_*.h file should be included per project

 // This is an experimental version for FlexBus output by Sandor Wortmann

#ifndef MATRIX_HARDWARE_H
#define MATRIX_HARDWARE_H

#define DMA_UPDATES_PER_CLOCK           1  
#define ADDX_UPDATE_ON_DATA_PINS
#define ADDX_UPDATE_BEFORE_LATCH_BYTES  0  

/* an advanced user may need to tweak these values */

// size of latch pulse - can be short for V4 which doesn't need to update ADDX lines during latch pulse
#define LATCH_TIMER_PULSE_WIDTH_NS  100

// max delay from rising edge of latch pulse to falling edge of clock
// increase this value if DMA use is delaying clock
// using largest delay seen at slowest supported clock speed (48MHz) 1400ns, saw 916ns at 96MHz
#define LATCH_TO_CLK_DELAY_NS       1400

// measured <78ns to transfer 1 pixels at 180MHz, 
// for now, until DMA sharing complications (brought to light by Teensy 3.6 SDIO) can be worked out, enable DMA Bandwidth Control, which approximately doubles this estimated time
#define PANEL_PIXELDATA_TRANSFER_MAXIMUM_NS  (uint32_t)((78 * 180000000.0) / F_CPU)

/* this section describes how the microcontroller is attached to the display */

// change for SmartMatrix Shield V4: G2 moves from Teensy pin 7 (D2) to 8 (D3)

// defines data bit order from bit 0-7, four times to fit in uint32_t
#define GPIO_WORD_ORDER p0r1:1, p0clk:1, p0pad:1, p0g2:1, p0b1:1, p0b2:1, p0r2:1, p0g1:1, \
    p1r1:1, p1clk:1, p1pad:1, p1g2:1, p1b1:1, p1b2:1, p1r2:1, p1g1:1, \
    p2r1:1, p2clk:1, p2pad:1, p2g2:1, p2b1:1, p2b2:1, p2r2:1, p2g1:1, \
    p3r1:1, p3clk:1, p3pad:1, p3g2:1, p3b1:1, p3b2:1, p3r2:1, p3g1:1

//#define GPIO_WORD_ORDER_8BIT p0r1:1, p0clk:1, p0pad:1, p0g2:1, p0b1:1, p0b2:1, p0r2:1, p0g1:1
#define GPIO_WORD_ORDER_8BIT p0r2:1, p0b2:1, p0b1:1, p0g2:1, p0clk:1, p0pad:1, p0g1:1, p0r1:1   //FlexBushoz
#define GPIO_WORD_ORDER_16BIT p0r1a:1, p0g1a:1, p0b1a:1, p0r2a:1, p0g2a:1, p0b2a:1, p0r1b:1, p0g1b:1, p0b1b:1, p0r2b:1, p0pad:1, p0g2b:1, p0b2b:1   //16 bites FlexBushoz

//#define DEBUG_PINS_ENABLED
#define DEBUG_PIN_1 15
#define DEBUG_PIN_2 16
#define DEBUG_PIN_3 17

#define Buffer_OE                   4  //FTM1
#define Buffer_Latch                3
#define GPIO_PIN_CLK_TEENSY_PIN     14  //D1
#define GPIO_PIN_B0_TEENSY_PIN      6   //D4
#define GPIO_PIN_R0_TEENSY_PIN      2   //D0
#define GPIO_PIN_R1_TEENSY_PIN      21  //D6
#define GPIO_PIN_G0_TEENSY_PIN      5   //D7
#define GPIO_PIN_G1_TEENSY_PIN      8   //D3
#define GPIO_PIN_B1_TEENSY_PIN      20  //D5
#define GPIO_PIN_ROWSEL_TEENSY_PIN  7   //D2

#define SMARTLED_APA_ENABLE_PIN     17
#define SMARTLED_APA_CLK_PIN        13
#define SMARTLED_APA_DAT_PIN        7

// output latch signal on two pins, to trigger two different GPIO port interrupts
#define ENABLE_LATCH_PWM_OUTPUT() {                                     \
        CORE_PIN3_CONFIG |= PORT_PCR_MUX(3) | PORT_PCR_DSE | PORT_PCR_SRE;  \
    }                                                                                       //FTM1_CH0 | Drive strengt enable | slew rate enable

//TPM1_CH1 | Drive strengt enable | slew rate enable
//FTM1_CH1 | Drive strengt enable | slew rate enable
#if defined (PWM_ON_OE)
    #define ENABLE_OE_PWM_OUTPUT() {                                        \
            CORE_PIN4_CONFIG = PORT_PCR_MUX(7) | PORT_PCR_DSE | PORT_PCR_SRE;   \
    }                                                                                                                               
#else
    #define ENABLE_OE_PWM_OUTPUT() {                                        \
            CORE_PIN4_CONFIG = PORT_PCR_MUX(3) | PORT_PCR_DSE | PORT_PCR_SRE;   \
    }
#endif

// pin 3 (PORT A) triggers based on latch signal, on rising edge
#define ENABLE_LATCH_RISING_EDGE_GPIO_INT() {              \
        CORE_PIN3_CONFIG |= PORT_PCR_MUX(1) | PORT_PCR_IRQC(1); \
    }                                                                                       //PTA12 | ISF flag and DMA request on rising edge.

// unused for SmartMatrix Shield V4
#define ENABLE_LATCH_FALLING_EDGE_GPIO_INT() {              \
    }

#define DMAMUX_SOURCE_LATCH_FALLING_EDGE     DMAMUX_SOURCE_PORTA

#endif
