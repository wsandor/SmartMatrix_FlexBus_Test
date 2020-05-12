/*
 * SmartMatrix Library - Calculation Code for Teensy 3.x Platform
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

#define INLINE __attribute__( ( always_inline ) ) inline

static IntervalTimer calcTimer;

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
uint16_t SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::calc_refreshRate = 200;
template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
uint16_t SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::max_refreshRate = 1000;
template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
SM_Layer * SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::baseLayer;

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
bool SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::refreshRateLowered = false;

// set to true initially so all layers get the initial refresh rate
template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
bool SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::refreshRateChanged = true;

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
int SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::multiRowRefresh_mapIndex_CurrentRowGroups = 0;

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
int SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::multiRowRefresh_mapIndex_CurrentPixelGroup = -1;

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
int SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::multiRowRefresh_PixelOffsetFromPanelsAlreadyMapped = 0;

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
int SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::multiRowRefresh_NumPanelsAlreadyMapped = 0;

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::SmartMatrix3FB(uint8_t bufferrows, rowDataStruct * rowDataBuffer) {
}

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
void SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::addLayer(SM_Layer * newlayer) {
    if(baseLayer) {
        SM_Layer * templayer = baseLayer;
        while(templayer->nextLayer)
            templayer = templayer->nextLayer;
        templayer->nextLayer = newlayer;
    } else {
        baseLayer = newlayer;
    }
}

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
void SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::countFPS(void) {
  static long loops = 0;
  static long lastMillis = 0;
  long currentMillis = millis();

  loops++;
  if(currentMillis - lastMillis >= 1000){
#if defined(USB_SERIAL)
    if(Serial) {
        Serial.print("Loops last second:");
        Serial.println(loops);
    }
#endif    
    lastMillis = currentMillis;
    loops = 0;
  }
}


template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
void SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::matrixCalculations(void) { //bool initial) {
    
#ifdef DEBUG_PINS_ENABLED
    digitalWriteFast(DEBUG_PIN_3, HIGH); // oscilloscope trigger
#endif
    static unsigned char currentRow = 0;
    int t = micros();
    if (brightnessChange) {
        SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::setBrightness(brightness);
        brightnessChange = false;
    }
    SM_Layer * templayer = SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::baseLayer;
    while(templayer) {
        if (rotationChange) {
            templayer->setRotation(rotation);
        }  
        if(refreshRateChanged) {
            templayer->setRefreshRate(calc_refreshRate);
        }
        templayer->frameRefreshCallback();
        templayer = templayer->nextLayer;
    }
    refreshRateChanged = false;
    rotationChange = false;
    if (changed) {
        changed = false;
        for (currentRow = 0; currentRow < MATRIX_SCAN_MOD; currentRow++) {
            // do once-per-line updates
            SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::loadMatrixBuffers(currentRow);
        }
        SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::swapRefreshBuffers();
    }
    //Serial.printf("cal t: %i us\n", t1 - t);
    //Serial.printf("swap t: %i us\n", micros() - t);
    uint16_t refreshFromLast = 750000 / ((micros() + 1) - t);   //az idő 3/4-ét használhatja el max a számítás // + 1, hogy biztossan ne lehessen 0!
    if (max_refreshRate > refreshFromLast) {
        max_refreshRate = refreshFromLast;
    } 
    uint16_t phisical_rate = SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::getRefreshRate();
    if (max_refreshRate > phisical_rate) { //(uint16_t)SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::getRefreshRate) {
        max_refreshRate = phisical_rate;
        //Serial.printf("max set to phis: %i\n", max_refreshRate);
    }
    if (calc_refreshRate > max_refreshRate) {
        calc_refreshRate = max_refreshRate;
        calcTimer.update(1000000 / calc_refreshRate);  
        refreshRateChanged = true;
        refreshRateLowered = true; 
        Serial.printf("refr.max: %i\n", calc_refreshRate);
    }
#ifdef DEBUG_PINS_ENABLED
    digitalWriteFast(DEBUG_PIN_3, LOW); // oscilloscope trigger
#endif
}

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
void SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::setRotation(rotationDegrees newrotation) {
    rotation = newrotation;
    rotationChange = true;
}

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
uint16_t SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::getScreenWidth(void) const {
    if (rotation == rotation0 || rotation == rotation180) {
        return matrixWidth;
    } else {
        return matrixHeight;
    }
}

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
uint16_t SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::getScreenHeight(void) const {
    if (rotation == rotation0 || rotation == rotation180) {
        return matrixHeight;
    } else {
        return matrixWidth;
    }
}

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
volatile bool SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::brightnessChange = false;
template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
volatile bool SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::rotationChange = true;
template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
rotationDegrees SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::rotation = rotation0;

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
int SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::brightness;

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
void SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::setBrightness(uint8_t newBrightness) {
    brightness = newBrightness;
    brightnessChange = true;
    SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::setBrightness(brightness);
}

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
void SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::setRefreshRate(uint8_t newRefreshRate) {
    if(newRefreshRate != calc_refreshRate) {
        if(newRefreshRate < max_refreshRate)
            calc_refreshRate = newRefreshRate;
        else
            calc_refreshRate = max_refreshRate;
        refreshRateChanged = true;
        calcTimer.update(1000000 / calc_refreshRate); 
        //Serial.printf("new: %i, max: %i, refr: %i\n", newRefreshRate, max_refreshRate, calc_refreshRate);
    }
    //SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::setRefreshRate(calc_refreshRate);
}

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
uint8_t SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::getRefreshRate(void) {
    return calc_refreshRate;
}

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
bool SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::getRefreshRateLoweredFlag(void) {
    if(refreshRateLowered) {
        refreshRateLowered = false;
        return true;
    }
    return false;
}

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
void SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::begin(void)
{
    SM_Layer * templayer = baseLayer;
    while(templayer) {
        templayer->begin();
        templayer = templayer->nextLayer;
    }

    //SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::setMatrixCalculationsCallback(matrixCalculations);
    SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::begin();
    
    calcTimer.priority(ROW_CALCULATION_ISR_PRIORITY);   
    calcTimer.begin(matrixCalculations, 1000000 / calc_refreshRate);  
}

#define IS_LAST_PANEL_MAP_ENTRY(x) (!x.rowOffset && !x.bufferOffset && !x.numPixels)

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
void SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::resetMultiRowRefreshMapPosition(void) {   
    multiRowRefresh_mapIndex_CurrentRowGroups = 0;
    resetMultiRowRefreshMapPositionPixelGroupToStartOfRow();
}

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
void SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::resetMultiRowRefreshMapPositionPixelGroupToStartOfRow(void) {   
    multiRowRefresh_mapIndex_CurrentPixelGroup = multiRowRefresh_mapIndex_CurrentRowGroups;
    multiRowRefresh_PixelOffsetFromPanelsAlreadyMapped = 0;
    multiRowRefresh_NumPanelsAlreadyMapped = 0;
}

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
void SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::advanceMultiRowRefreshMapToNextRow(void) {   
    static const PanelMappingEntry * map = getMultiRowRefreshPanelMap(panelType);

    int currentRowOffset = map[multiRowRefresh_mapIndex_CurrentRowGroups].rowOffset;

    // advance until end of table, or entry with new row nubmer is found
    while(!IS_LAST_PANEL_MAP_ENTRY(map[multiRowRefresh_mapIndex_CurrentRowGroups])) {
        multiRowRefresh_mapIndex_CurrentRowGroups++;

        if(map[multiRowRefresh_mapIndex_CurrentRowGroups].rowOffset != currentRowOffset)
            break;
    }

    resetMultiRowRefreshMapPositionPixelGroupToStartOfRow();
}

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
void SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::advanceMultiRowRefreshMapToNextPixelGroup(void) {   
    static const PanelMappingEntry * map = getMultiRowRefreshPanelMap(panelType);

    int currentRowOffset = map[multiRowRefresh_mapIndex_CurrentPixelGroup].rowOffset;

    // don't change if we're already on the end
    if(IS_LAST_PANEL_MAP_ENTRY(map[multiRowRefresh_mapIndex_CurrentPixelGroup])) {
        return;
    }

    if(!IS_LAST_PANEL_MAP_ENTRY(map[multiRowRefresh_mapIndex_CurrentPixelGroup + 1]) &&
        // go to the next entry if it's in the same row offset
        (map[multiRowRefresh_mapIndex_CurrentPixelGroup + 1].rowOffset == currentRowOffset)) {
        multiRowRefresh_mapIndex_CurrentPixelGroup++;
    } else {
        // else we just finished mapping a panel and we're wrapping to the beginning of this row in the list
        // keep going back until we get to the first entry, or the first entry in this row
        while((multiRowRefresh_mapIndex_CurrentPixelGroup > 0) && (map[multiRowRefresh_mapIndex_CurrentPixelGroup - 1].rowOffset == currentRowOffset))
            multiRowRefresh_mapIndex_CurrentPixelGroup--;

        // we need to set the total offset to the beginning offset of the next panel.  Calculate what that would be
        multiRowRefresh_NumPanelsAlreadyMapped++;
        multiRowRefresh_PixelOffsetFromPanelsAlreadyMapped = multiRowRefresh_NumPanelsAlreadyMapped * PANEL_OFFSET; //COLS_PER_PANEL * PHYSICAL_ROWS_PER_REFRESH_ROW;
    }
}

// returns the row offset from the map, or -1 if we've gone through the whole map already
template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
int SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::getMultiRowRefreshRowOffset(void) {   
    static const PanelMappingEntry * map = getMultiRowRefreshPanelMap(panelType);

    if(IS_LAST_PANEL_MAP_ENTRY(map[multiRowRefresh_mapIndex_CurrentRowGroups])){
        //Serial.printf("c CurrentPixelGroup %i RowOffset last\n", multiRowRefresh_mapIndex_CurrentPixelGroup);
        return -1;
    }
    //Serial.printf("c CurrentPixelGroup %i RowOffset %i\n", multiRowRefresh_mapIndex_CurrentPixelGroup, map[multiRowRefresh_mapIndex_CurrentRowGroups].rowOffset);
    
    return map[multiRowRefresh_mapIndex_CurrentRowGroups].rowOffset;    
}

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
int SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::getMultiRowRefreshNumPixelsToMap(void) {        
    static const PanelMappingEntry * map = getMultiRowRefreshPanelMap(panelType);
    //Serial.printf("a CurrentPixelGroup %i NumPixelsToMap %i\n", multiRowRefresh_mapIndex_CurrentPixelGroup, map[multiRowRefresh_mapIndex_CurrentPixelGroup].numPixels);
    return map[multiRowRefresh_mapIndex_CurrentPixelGroup].numPixels;    
}

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
int SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::getMultiRowRefreshPixelGroupOffset(void) {        
    static const PanelMappingEntry * map = getMultiRowRefreshPanelMap(panelType);
    //Serial.printf("b CurrentPixelGroup %i GroupOffset %i\n", multiRowRefresh_mapIndex_CurrentPixelGroup, map[multiRowRefresh_mapIndex_CurrentPixelGroup].bufferOffset + multiRowRefresh_PixelOffsetFromPanelsAlreadyMapped);
    
    return map[multiRowRefresh_mapIndex_CurrentPixelGroup].bufferOffset + multiRowRefresh_PixelOffsetFromPanelsAlreadyMapped;
}


template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
INLINE void SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::loadMatrixBuffers24(rowDataStruct * currentRowDataPtr, unsigned char currentRow) {
    int i;
    int multiRowRefreshRowOffset = 0;
    //const int numPixelsPerTempRow = matrixWidth * (((matrixHeight / MATRIX_PANEL_HEIGHT) + 1) / 2); 
    //WRITES_PER_LATCH_16/PHYSICAL_ROWS_PER_REFRESH_ROW;
    const int numPixelsPerTempRow = PIXELS_PER_LATCH_16 / PHYSICAL_ROWS_PER_REFRESH_ROW;  //itt annyinak kell lennie ahány pixel valójában van egy sorban
    
    //Serial.println(numPixelsPerTempRow);
    
    //memset(currentRowDataPtr->rowbits, 0, sizeof(currentRowDataPtr->rowbits[0].data));
    memset(currentRowDataPtr->rowbits, 0, sizeof(currentRowDataPtr->rowbits));

    // static to avoid putting large buffer on the stack
    static rgb24 tempRow[4][numPixelsPerTempRow];
	
    int c = 0;
    resetMultiRowRefreshMapPosition();
	
	/*bool changed = false;
	SM_Layer * templayer = SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::baseLayer;
    while(templayer) {
		changed|= templayer->changed;
		templayer = templayer->nextLayer;        
    }*/
	int maskoffset = 8 - COLOR_DEPTH_BITS; 

    // go through this process for each physical row that is contained in the refresh row
    do {
        // clear buffer to prevent garbage data showing through transparent layers
        //Serial.printf("0 - curentRow: %i\n multiRowRefreshRowOffset :%i\n", currentRow, multiRowRefreshRowOffset);
        memset(tempRow, 0x00, sizeof(tempRow));
        //memset(tempRow1, 0x00, sizeof(tempRow1));

        // get pixel data from layers
        SM_Layer * templayer = SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::baseLayer;
        while(templayer) {
            for(i=0; i<MATRIX_STACK_HEIGHT_16; i++) {
                // fill data from top to bottom, so top panel is the one closest to Teensy
                for (int r=0; r<2; r++) {
                    int row = (currentRow + multiRowRefreshRowOffset) + (((r * MATRIX_STACK_HEIGHT_16) + i) * MATRIX_PANEL_HEIGHT);
                    templayer->fillRefreshRow(row, &tempRow[r * 2][i*matrixWidth]);
                    templayer->fillRefreshRow(row + ROW_PAIR_OFFSET, &tempRow[(r * 2) + 1][i*matrixWidth]);
                    //Serial.printf("r:%i i:%i\n", r, i);
                }
                /*
                // Z-shape, bottom to top
                if(!(optionFlags & SMARTMATRIX_OPTIONS_C_SHAPE_STACKING) &&
                    (optionFlags & SMARTMATRIX_OPTIONS_BOTTOM_TO_TOP_STACKING)) {
                    // fill data from bottom to top, so bottom panel is the one closest to Teensy
                    templayer->fillRefreshRow((currentRow + multiRowRefreshRowOffset) + (MATRIX_STACK_HEIGHT-i-1)*MATRIX_PANEL_HEIGHT, &tempRow0[i*matrixWidth]);
                    templayer->fillRefreshRow((currentRow + multiRowRefreshRowOffset) + ROW_PAIR_OFFSET + (MATRIX_STACK_HEIGHT-i-1)*MATRIX_PANEL_HEIGHT, &tempRow1[i*matrixWidth]);
                // Z-shape, top to bottom
                } else if(!(optionFlags & SMARTMATRIX_OPTIONS_C_SHAPE_STACKING) &&
                    !(optionFlags & SMARTMATRIX_OPTIONS_BOTTOM_TO_TOP_STACKING)) {
                    // fill data from top to bottom, so top panel is the one closest to Teensy
                    templayer->fillRefreshRow((currentRow + multiRowRefreshRowOffset) + i*MATRIX_PANEL_HEIGHT, &tempRow0[i*matrixWidth]);
                    templayer->fillRefreshRow((currentRow + multiRowRefreshRowOffset) + ROW_PAIR_OFFSET + i*MATRIX_PANEL_HEIGHT, &tempRow1[i*matrixWidth]);
                // C-shape, bottom to top
                } else if((optionFlags & SMARTMATRIX_OPTIONS_C_SHAPE_STACKING) &&
                    (optionFlags & SMARTMATRIX_OPTIONS_BOTTOM_TO_TOP_STACKING)) {
                    // alternate direction of filling (or loading) for each matrixwidth
                    // swap row order from top to bottom for each stack (tempRow1 filled with top half of panel, tempRow0 filled with bottom half)
                    if((MATRIX_STACK_HEIGHT-i+1)%2) {
                        templayer->fillRefreshRow((MATRIX_SCAN_MOD-(currentRow + multiRowRefreshRowOffset)-1) + ROW_PAIR_OFFSET + (i)*MATRIX_PANEL_HEIGHT, &tempRow0[i*matrixWidth]);
                        templayer->fillRefreshRow((MATRIX_SCAN_MOD-(currentRow + multiRowRefreshRowOffset)-1) + (i)*MATRIX_PANEL_HEIGHT, &tempRow1[i*matrixWidth]);
                    } else {
                        templayer->fillRefreshRow((currentRow + multiRowRefreshRowOffset) + (i)*MATRIX_PANEL_HEIGHT, &tempRow0[i*matrixWidth]);
                        templayer->fillRefreshRow((currentRow + multiRowRefreshRowOffset) + ROW_PAIR_OFFSET + (i)*MATRIX_PANEL_HEIGHT, &tempRow1[i*matrixWidth]);
                    }
                // C-shape, top to bottom
                } else if((optionFlags & SMARTMATRIX_OPTIONS_C_SHAPE_STACKING) && 
                    !(optionFlags & SMARTMATRIX_OPTIONS_BOTTOM_TO_TOP_STACKING)) {
                    if((MATRIX_STACK_HEIGHT-i)%2) {
                        templayer->fillRefreshRow((currentRow + multiRowRefreshRowOffset) + (MATRIX_STACK_HEIGHT-i-1)*MATRIX_PANEL_HEIGHT, &tempRow0[i*matrixWidth]);
                        templayer->fillRefreshRow((currentRow + multiRowRefreshRowOffset) + ROW_PAIR_OFFSET + (MATRIX_STACK_HEIGHT-i-1)*MATRIX_PANEL_HEIGHT, &tempRow1[i*matrixWidth]);
                    } else {
                        templayer->fillRefreshRow((MATRIX_SCAN_MOD-(currentRow + multiRowRefreshRowOffset)-1) + ROW_PAIR_OFFSET + (MATRIX_STACK_HEIGHT-i-1)*MATRIX_PANEL_HEIGHT, &tempRow0[i*matrixWidth]);
                        templayer->fillRefreshRow((MATRIX_SCAN_MOD-(currentRow + multiRowRefreshRowOffset)-1) + (MATRIX_STACK_HEIGHT-i-1)*MATRIX_PANEL_HEIGHT, &tempRow1[i*matrixWidth]);
                    }
                }
                */
            }
            templayer = templayer->nextLayer;        
        }

        union {
            uint16_t word;
            struct {
                // order of bits in word matches how GPIO connects to the display
                uint16_t GPIO_WORD_ORDER_16BIT;
            };
        } o0;
        
        
            
            int i=0;

            // reset pixel map offset so we start filling from the first panel again
            resetMultiRowRefreshMapPositionPixelGroupToStartOfRow();
			
            //Serial.printf("1 - mapIndex_CurrentPixelGroup: %i PixelOffsetFromPanelsAlreadyMapped: %i NumPanelsAlreadyMapped:%i\n", multiRowRefresh_mapIndex_CurrentPixelGroup, multiRowRefresh_PixelOffsetFromPanelsAlreadyMapped, multiRowRefresh_NumPanelsAlreadyMapped);
            
            //Serial.printf("numPixelsPerTempRow %i\n", numPixelsPerTempRow);
            while(i < numPixelsPerTempRow) {
                // get number of pixels to go through with current pass
                int numPixelsToMap = getMultiRowRefreshNumPixelsToMap();

                bool reversePixelBlock = false;
                if(numPixelsToMap < 0) {
                    reversePixelBlock = true;
                    numPixelsToMap = abs(numPixelsToMap);
                }

                // get offset where pixels are written in the refresh buffer
                int currentMapOffset = getMultiRowRefreshPixelGroupOffset();					
				//Serial.printf("2 - numPixelsToMap: %i, currentMapOffset: %i\n", numPixelsToMap, currentMapOffset);   

                // parse through grouping of pixels, loading from temp buffer and writing to refresh buffer
                for(int k=0; k < numPixelsToMap; k++) {

                    unsigned int refreshBufferPosition;
                    if(reversePixelBlock) {
                        refreshBufferPosition = currentMapOffset-k;
                    } else {
                        refreshBufferPosition = currentMapOffset+k;
                    }
                    //Serial.print(refreshBufferPosition);
                    //Serial.print(",");
                    for(int j=0; j<COLOR_DEPTH_BITS; j++) {   // <= 8-bit color  //WS
                        

                        uint8_t mask = (1 << (j + maskoffset));

                        o0.word = 0x0000;

                        if (tempRow[0][i+k].red & mask)
                            o0.p0r1a = 1;
                        if (tempRow[0][i+k].green & mask)
                            o0.p0g1a = 1;
                        if (tempRow[0][i+k].blue & mask)
                            o0.p0b1a = 1;
                        if (tempRow[1][i+k].red & mask)
                            o0.p0r2a = 1;
                        if (tempRow[1][i+k].green & mask)
                            o0.p0g2a = 1;
                        if (tempRow[1][i+k].blue & mask)
                            o0.p0b2a = 1;
                        
                        if (matrixHeight > MATRIX_PANEL_HEIGHT) {
                            if (tempRow[2][i+k].red & mask)
                                o0.p0r1b = 1;
                            if (tempRow[2][i+k].green & mask)
                                o0.p0g1b = 1;
                            if (tempRow[2][i+k].blue & mask)
                                o0.p0b1b = 1;
                            if (tempRow[3][i+k].red & mask)
                                o0.p0r2b = 1;
                            if (tempRow[3][i+k].green & mask)
                                o0.p0g2b = 1;
                            if (tempRow[3][i+k].blue & mask)
                                o0.p0b2b = 1;
                        }
                        currentRowDataPtr->rowbits[j].data[refreshBufferPosition] = o0.word;
                    }

                    /*if(optionFlags & SMARTMATRIX_OPTIONS_HUB12_MODE) {
                        // HUB12 format inverts the data (assume we're only using R1 for now)
                        if (tempRow0[i+k].red & mask)
                            o0.p0r1a = 0;
                        else
                            o0.p0r1a = 1;                        
                    }      
                    */
                    
                    					
                }
                i += numPixelsToMap; // keep track of current position on this temp buffer
                advanceMultiRowRefreshMapToNextPixelGroup();
            }
            //currentRowDataPtr->rowbits[0].rowAddress = currentRow;

/*#ifdef ADDX_UPDATE_ON_DATA_PINS
        o0.word = 0x00000000;
        o0.p0r1 = (currentRow & 0x01) ? 1 : 0;
        o0.p0g1 = (currentRow & 0x02) ? 1 : 0;
        o0.p0b1 = (currentRow & 0x04) ? 1 : 0;
        o0.p0r2 = (currentRow & 0x08) ? 1 : 0;
        o0.p0g2 = (currentRow & 0x10) ? 1 : 0;

        currentRowDataPtr->rowbits[0].rowAddress = o0.word;
        
#endif*/

        c += numPixelsPerTempRow; // keep track of cumulative number of pixels filled in refresh buffer before this temp buffer

        advanceMultiRowRefreshMapToNextRow();
        multiRowRefreshRowOffset = getMultiRowRefreshRowOffset();
    } while (multiRowRefreshRowOffset > 0); 	

    /*union {
            uint16_t word;
            struct {
                // order of bits in word matches how GPIO connects to the display
                uint16_t GPIO_WORD_ORDER_16BIT;
            };
        } o0;
    o0.word = 0x0000;
    o0.p0r1a = 1;
    o0.p0r2a = 1;
    o0.p0r1b = 1;
    o0.p0r2b = 1;
    for (int i=0; i< WRITES_PER_LATCH_16; i++) {
       currentRowDataPtr->rowbits[0].data[i] = o0.word; 
    }*/
    //Serial.println();
/*for (int j=0; j<8; j++) {
    Serial.printf("%x ", currentRowDataPtr->rowbits[j].data[0] & 1);
}
Serial.println();
*/
}

template <int refreshDepth, int matrixWidth, int matrixHeight, unsigned char panelType, unsigned char optionFlags>
INLINE void SmartMatrix3FB<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::loadMatrixBuffers(unsigned char currentRow) {
    rowDataStruct * currentRowDataPtr = SmartMatrix3RefreshFlexBusSimple<refreshDepth, matrixWidth, matrixHeight, panelType, optionFlags>::getNextRowBufferPtr(currentRow);

        //memset(currentRowDataPtr->rowbits, 0, sizeof(currentRowDataPtr->rowbits));
        //currentRowDataPtr->rowbits[0].data[111] = 0x7;
        loadMatrixBuffers24(currentRowDataPtr, currentRow);

        /*Serial.println(currentRow);
        for (int i = 0; i<WRITES_PER_LATCH_16; i++) {
            for (int j=0; j<COLOR_DEPTH_BITS;j++) {
                Serial.printf("%4x,", currentRowDataPtr->rowbits[j].data[i]);
            }
            Serial.println();
        }
        */
}
