/*
 * SmartMatrix Library - Scrolling Layer Class
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

#include <string.h>

#define SCROLLING_BUFFER_ROW_SIZE   (this->localWidth / 8)
#define SCROLLING_BUFFER_SIZE       (SCROLLING_BUFFER_ROW_SIZE * this->localHeight)

template <typename RGB, unsigned int optionFlags>
SMLayerScrolling<RGB, optionFlags>::SMLayerScrolling(uint8_t * bitmap, uint16_t width, uint16_t height) {
    scrollingBitmap = bitmap;
	
	scrollingBackBitmap = (uint8_t *)malloc(width * (height / 8));
    memset(scrollingBackBitmap, 0x00, width * (height / 8));
	
	this->matrixWidth = width;
    this->matrixHeight = height;
    this->textcolor = rgb48(0xffff, 0xffff, 0xffff);
	this->backcolor = rgb48(0x0000, 0x0000, 0x0000);
	this->drawBackColor = false;
	this->xEnd = width;
	this->yEnd = height;
	changed = true;
}

template <typename RGB, unsigned int optionFlags>
SMLayerScrolling<RGB, optionFlags>::SMLayerScrolling(uint16_t width, uint16_t height) {
    scrollingBitmap = (uint8_t *)malloc(width * (height / 8));
#ifdef ESP32
    assert(scrollingBitmap != NULL);
#else
    this->assert(scrollingBitmap != NULL);
#endif
    memset(scrollingBitmap, 0x00, width * (height / 8));
	
	scrollingBackBitmap = (uint8_t *)malloc(width * (height / 8));
#ifdef ESP32
    assert(scrollingBackBitmap != NULL);
#else
    this->assert(scrollingBackBitmap != NULL);
#endif
    memset(scrollingBackBitmap, 0x00, width * (height / 8));
    this->matrixWidth = width;
    this->matrixHeight = height;
    this->textcolor = rgb48(0xffff, 0xffff, 0xffff);
	this->backcolor = rgb48(0x0000, 0x0000, 0x0000);
	this->drawBackColor = false;
	this->xEnd = width;
	this->yEnd = height;
	changed = true;
}

template <typename RGB, unsigned int optionFlags>
void SMLayerScrolling<RGB, optionFlags>::begin(void) {
    scrollFont = (bitmap_font *) &apple3x5;
}

template <typename RGB, unsigned int optionFlags>
void SMLayerScrolling<RGB, optionFlags>::frameRefreshCallback(void) {
    updateScrollingText();
}

// returns true and copies textcolor to xyPixel if pixel is opaque, copies backcolor if it is an opaque background pixel or returns false if not
template<typename RGB, unsigned int optionFlags> template <typename RGB_OUT>
bool SMLayerScrolling<RGB, optionFlags>::getPixel(uint16_t hardwareX, uint16_t hardwareY, RGB_OUT &xyPixel) {
    uint16_t localScreenX, localScreenY;

    // convert hardware x/y to the pixel in the local screen
    switch( this->rotation ) {
      case rotation0 :
        localScreenX = hardwareX;
        localScreenY = hardwareY;
        break;
      case rotation180 :
        localScreenX = (this->matrixWidth - 1) - hardwareX;
        localScreenY = (this->matrixHeight - 1) - hardwareY;
        break;
      case  rotation90 :
        localScreenX = hardwareY;
        localScreenY = (this->matrixWidth - 1) - hardwareX;
        break;
      case  rotation270 :
        localScreenX = (this->matrixHeight - 1) - hardwareY;
        localScreenY = hardwareX;
        break;
      default:
        // TODO: Should throw an error
        return false;
    };

    uint8_t bitmask = 0x80 >> (localScreenX % 8);

    if (scrollingBitmap[(localScreenY * SCROLLING_BUFFER_ROW_SIZE) + (localScreenX/8)] & bitmask) {
        xyPixel = textcolor;
        return true;
    } 
    if (drawBackColor && (scrollingBackBitmap[(localScreenY * SCROLLING_BUFFER_ROW_SIZE) + (localScreenX/8)] & bitmask)) {
        xyPixel = backcolor;
        return true;
    }
    
    return false;
}

//retrns 0 if empty, 1 if text, 2 if background
template<typename RGB, unsigned int optionFlags>
uint8_t SMLayerScrolling<RGB, optionFlags>::getPixel(uint16_t hardwareX, uint16_t hardwareY) {
    uint16_t localScreenX, localScreenY;

    // convert hardware x/y to the pixel in the local screen
    switch( this->rotation ) {
      case rotation0 :
        localScreenX = hardwareX;
        localScreenY = hardwareY;
        break;
      case rotation180 :
        localScreenX = (this->matrixWidth - 1) - hardwareX;
        localScreenY = (this->matrixHeight - 1) - hardwareY;
        break;
      case  rotation90 :
        localScreenX = hardwareY;
        localScreenY = (this->matrixWidth - 1) - hardwareX;
        break;
      case  rotation270 :
        localScreenX = (this->matrixHeight - 1) - hardwareY;
        localScreenY = hardwareX;
        break;
      default:
        // TODO: Should throw an error
        return 0;
    };

    uint8_t bitmask = 0x80 >> (localScreenX % 8);

    if (scrollingBitmap[(localScreenY * SCROLLING_BUFFER_ROW_SIZE) + (localScreenX/8)] & bitmask) {
        return 1;
    }
    if (drawBackColor && (scrollingBackBitmap[(localScreenY * SCROLLING_BUFFER_ROW_SIZE) + (localScreenX/8)] & bitmask)) {
        return 2;
    }
    return 0;
}


template <typename RGB, unsigned int optionFlags>
void SMLayerScrolling<RGB, optionFlags>::fillRefreshRow(uint16_t hardwareY, rgb48 refreshRow[]) {
    rgb48 currentPixel, currentBackPixel;
    int i;
    if(this->ccEnabled) {
        colorCorrection(textcolor, currentPixel);
		colorCorrection(backcolor, currentBackPixel);
	} else {
        currentPixel = textcolor;
		currentBackPixel = backcolor;
	}

    uint16_t localScreenX, localScreenY;
    uint8_t bitmask;
    uint16_t idx;

    // convert hardware x/y to the pixel in the local screen
    switch( this->rotation ) {
      case rotation0 :
        for(i= xStart; i<xEnd; i++) {
            bitmask = 0x80 >> (i % 8);
            idx = (hardwareY * SCROLLING_BUFFER_ROW_SIZE) + (i/8);
            if (scrollingBitmap[idx] & bitmask) {
                refreshRow[i] = currentPixel;
            } else {
                if (drawBackColor && (scrollingBackBitmap[idx] & bitmask)) {
                    refreshRow[i] = currentBackPixel;
                }
            }
        }
        break;
      case rotation180 :
        localScreenY = (this->matrixHeight - 1) - hardwareY;
        for(i= xStart; i<xEnd; i++) {
            localScreenX = (this->matrixWidth - 1) - i;
            bitmask = 0x80 >> (localScreenX % 8);
            idx = (localScreenY * SCROLLING_BUFFER_ROW_SIZE) + (localScreenX/8);
            if (scrollingBitmap[idx] & bitmask) {
                refreshRow[i] = currentPixel;
            } else {
                if (drawBackColor && (scrollingBackBitmap[idx] & bitmask)) {
                    refreshRow[i] = currentBackPixel;
                }
            }
        }
        break;
      case  rotation90 :
        for(i= xStart; i<xEnd; i++) {
            localScreenY = (this->matrixWidth - 1) - i;
            bitmask = 0x80 >> (hardwareY % 8);
            idx = (localScreenY * SCROLLING_BUFFER_ROW_SIZE) + (hardwareY/8);
            if (scrollingBitmap[idx] & bitmask) {
                refreshRow[i] = currentPixel;
            } else {
                if (drawBackColor && (scrollingBackBitmap[idx] & bitmask)) {
                    refreshRow[i] = currentBackPixel;
                }
            }
        }
        break;
      case  rotation270 :
        localScreenX = (this->matrixHeight - 1) - hardwareY;
        for(i= xStart; i<xEnd; i++) {
            bitmask = 0x80 >> (localScreenX % 8);
            idx = (i * SCROLLING_BUFFER_ROW_SIZE) + (localScreenX/8);
            if (scrollingBitmap[idx] & bitmask) {
                refreshRow[i] = currentPixel;
            } else {
                if (drawBackColor && (scrollingBackBitmap[idx] & bitmask)) {
                    refreshRow[i] = currentBackPixel;
                }
            }
        }   
        break;
      }
    
/*
           switch (getPixel(i, hardwareY))
        {
        case 1:
            refreshRow[i] = currentPixel;
            break;
        case 2:
            refreshRow[i] = currentBackPixel;
            break;
        default:
            break;
        }					
    }*/
}

template <typename RGB, unsigned int optionFlags>
void SMLayerScrolling<RGB, optionFlags>::fillRefreshRow(uint16_t hardwareY, rgb24 refreshRow[]) {
    rgb24 currentPixel, currentBackPixel;
    int i;
    //Serial.printf("2 h: %i, c:%i\n", scrollFont->Height, scrollFont->Chars);
    if(this->ccEnabled) {
        colorCorrection(textcolor, currentPixel);
		colorCorrection(backcolor, currentBackPixel);
	} else {
        currentPixel = textcolor;
		currentBackPixel = backcolor;
	}

    uint16_t localScreenX, localScreenY;
    uint8_t bitmask;
    uint16_t idx;

    // convert hardware x/y to the pixel in the local screen
    switch( this->rotation ) {
      case rotation0 :
        for(i= xStart; i<xEnd; i++) {
            bitmask = 0x80 >> (i % 8);
            idx = (hardwareY * SCROLLING_BUFFER_ROW_SIZE) + (i/8);
            if (scrollingBitmap[idx] & bitmask) {
                refreshRow[i] = currentPixel;
            } else {
                if (drawBackColor && (scrollingBackBitmap[idx] & bitmask)) {
                    refreshRow[i] = currentBackPixel;
                }
            }
        }
        break;
      case rotation180 :
        localScreenY = (this->matrixHeight - 1) - hardwareY;
        for(i= xStart; i<xEnd; i++) {
            localScreenX = (this->matrixWidth - 1) - i;
            bitmask = 0x80 >> (localScreenX % 8);
            idx = (localScreenY * SCROLLING_BUFFER_ROW_SIZE) + (localScreenX/8);
            if (scrollingBitmap[idx] & bitmask) {
                refreshRow[i] = currentPixel;
            } else {
                if (drawBackColor && (scrollingBackBitmap[idx] & bitmask)) {
                    refreshRow[i] = currentBackPixel;
                }
            }
        }
        break;
      case  rotation90 :
        for(i= xStart; i<xEnd; i++) {
            localScreenY = (this->matrixWidth - 1) - i;
            bitmask = 0x80 >> (hardwareY % 8);
            idx = (localScreenY * SCROLLING_BUFFER_ROW_SIZE) + (hardwareY/8);
            if (scrollingBitmap[idx] & bitmask) {
                refreshRow[i] = currentPixel;
            } else {
                if (drawBackColor && (scrollingBackBitmap[idx] & bitmask)) {
                    refreshRow[i] = currentBackPixel;
                }
            }
        }
        break;
      case  rotation270 :
        localScreenX = (this->matrixHeight - 1) - hardwareY;
        for(i= xStart; i<xEnd; i++) {
            bitmask = 0x80 >> (localScreenX % 8);
            idx = (i * SCROLLING_BUFFER_ROW_SIZE) + (localScreenX/8);
            if (scrollingBitmap[idx] & bitmask) {
                refreshRow[i] = currentPixel;
            } else {
                if (drawBackColor && (scrollingBackBitmap[idx] & bitmask)) {
                    refreshRow[i] = currentBackPixel;
                }
            }
        }   
        break;
      }

    /*for(i= xStart; i<xEnd; i++) {
        switch (getPixel(i, hardwareY))
        {
        case 1:
            refreshRow[i] = currentPixel;
            break;
        case 2:
            refreshRow[i] = currentBackPixel;
            break;
        default:
            break;
        }		
    }*/
}

template<typename RGB, unsigned int optionFlags>
void SMLayerScrolling<RGB, optionFlags>::setColor(const RGB & newColor) {
    textcolor = newColor;
}

template<typename RGB, unsigned int optionFlags>
void SMLayerScrolling<RGB, optionFlags>::setBackColor(const RGB & newColor) {
    backcolor = newColor;
}

template<typename RGB, unsigned int optionFlags>
void SMLayerScrolling<RGB, optionFlags>::enableColorCorrection(bool enabled) {
    this->ccEnabled = sizeof(RGB) <= 3 ? enabled : false;
}

// stops the scrolling text on the next refresh
template <typename RGB, unsigned int optionFlags>
void SMLayerScrolling<RGB, optionFlags>::stop(void) {
    // setup conditions for ending scrolling:
    // scrollcounter is next to zero
    scrollcounter = 1;
    // position text at the end of the cycle
    scrollPosition = scrollMin;
}

// returns 0 if stopped
// returns positive number indicating number of loops left if running
// returns -1 if continuously scrolling
template <typename RGB, unsigned int optionFlags>
int SMLayerScrolling<RGB, optionFlags>::getStatus(void) const {
    return scrollcounter;
}

template <typename RGB, unsigned int optionFlags>
void SMLayerScrolling<RGB, optionFlags>::setMinMax(void) {
   //Serial.printf("1 h: %i, c:%i\n", scrollFont->Height, scrollFont->Chars);
   switch (scrollmode) {
    case wrapForward:
    case bounceForward:
    case bounceReverse:
    case wrapForwardFromLeft:
        scrollMin = -textWidth + xStart;
        scrollMax = /*this->localWidth -*/ xEnd;

        scrollPosition = scrollMax;

        if (scrollmode == bounceReverse)
            scrollPosition = scrollMin;
        else if(scrollmode == wrapForwardFromLeft)
            scrollPosition = fontLeftOffset;

        // TODO: handle special case - put content in fixed location if wider than window

        break;

    case stopped:
    case off:
        scrollMin = scrollMax = scrollPosition = 0;
        break;
    }

}

template <typename RGB, unsigned int optionFlags>
void SMLayerScrolling<RGB, optionFlags>::start(const char inputtext[], int numScrolls) {
    //Serial.printf("st0 h: %i, c:%i\n", scrollFont->Height, scrollFont->Chars);
    int length = strlen((const char *)inputtext);
    if (length > textLayerMaxStringLength)
        length = textLayerMaxStringLength;
    strncpy(text, (const char *)inputtext, length);
    textlen = length;
    scrollcounter = numScrolls;

	textWidth = -1;
	for (int i = 0; i < textlen; i++) {
		textWidth+= getBitmapFontCharWidth(text[i], scrollFont);
	}
    //textWidth = (textlen * scrollFont->Width) - 1;

    //Serial.printf("st h: %i, c:%i\n", scrollFont->Height, scrollFont->Chars);

    setMinMax();

    //Serial.printf("st2 h: %i, c:%i\n", scrollFont->Height, scrollFont->Chars);
 }

//Updates the text that is currently scrolling to the new value
//Useful for a clock display where the time changes.
template <typename RGB, unsigned int optionFlags>
void SMLayerScrolling<RGB, optionFlags>::update(const char inputtext[]){
    int length = strlen((const char *)inputtext);
    if (length > textLayerMaxStringLength)
        length = textLayerMaxStringLength;
    strncpy(text, (const char *)inputtext, length);
    textlen = length;
	textWidth = -1;
	for (int i = 0; i < textlen; i++) {
		textWidth+= getBitmapFontCharWidth(text[i], scrollFont);
	}
    //textWidth = (textlen * scrollFont->Width) - 1;

    setMinMax();
}

// called once per frame to update (virtual) bitmap
// function needs major efficiency improvments
template <typename RGB, unsigned int optionFlags>
void SMLayerScrolling<RGB, optionFlags>::updateScrollingText(void) {
    bool resetScrolls = false;
    // return if not ready to update

    if (!scrollcounter || ++currentframe <= framesperscroll)
        return;

    currentframe = 0;

    switch (scrollmode) {
    case wrapForward:
    case wrapForwardFromLeft:
        scrollPosition--;
        if (scrollPosition <= scrollMin) {
            scrollPosition = scrollMax;
            if (scrollcounter > 0) scrollcounter--;
        }
        break;

    case bounceForward:
        scrollPosition--;
        if (scrollPosition <= scrollMin) {
            scrollmode = bounceReverse;
            if (scrollcounter > 0) scrollcounter--;
        }
        break;

    case bounceReverse:
        scrollPosition++;
        if (scrollPosition >= scrollMax) {
            scrollmode = bounceForward;
            if (scrollcounter > 0) scrollcounter--;
        }
        break;

    default:
    case stopped:
        scrollPosition = fontLeftOffset;
        resetScrolls = true;
        break;
    }

    // done scrolling - move text off screen and disable
    if (!scrollcounter) {
        resetScrolls = true;
    }

    // for now, fill the bitmap fresh with each update
    // TODO: reset only when necessary, and update just the pixels that need it
    resetScrolls = true;
    if (resetScrolls) {
        redrawScrollingText();
    }
}

// TODO: recompute stuff after changing mode, font, etc
template <typename RGB, unsigned int optionFlags>
void SMLayerScrolling<RGB, optionFlags>::setMode(ScrollMode mode) {
    scrollmode = mode;
}

template <typename RGB, unsigned int optionFlags>
void SMLayerScrolling<RGB, optionFlags>::setRefreshRate(uint8_t newRefreshRate) {
    this->refreshRate = newRefreshRate;
    framesperscroll = (this->refreshRate * 1.0) / pixelsPerSecond;
}

template <typename RGB, unsigned int optionFlags>
void SMLayerScrolling<RGB, optionFlags>::setSpeed(unsigned char pixels_per_second) {
    pixelsPerSecond = pixels_per_second;
    framesperscroll = (this->refreshRate * 1.0) / pixelsPerSecond;
    //Serial.printf("fps: %i refr. %i\n", framesperscroll, this->refreshRate);
}

template <typename RGB, unsigned int optionFlags>
void SMLayerScrolling<RGB, optionFlags>::setFont(fontChoices newFont) {
    scrollFont = (bitmap_font *)fontLookup(newFont);
}

template <typename RGB, unsigned int optionFlags>
void SMLayerScrolling<RGB, optionFlags>::setBitmapFont(bitmap_font *newFont) {
  scrollFont = newFont;
  //Serial.printf("stf h: %i, c:%i\n", scrollFont->Height, scrollFont->Chars);
 // Serial.printf("font: %p", (void *) &scrollFont);
}

template <typename RGB, unsigned int optionFlags>
void SMLayerScrolling<RGB, optionFlags>::setOffsetFromTop(int offset) {
    fontTopOffset = offset + yStart;
    majorScrollFontChange = true;
}

template <typename RGB, unsigned int optionFlags>
void SMLayerScrolling<RGB, optionFlags>::setStartOffsetFromLeft(int offset) {
    fontLeftOffset = offset;
}

template <typename RGB, unsigned int optionFlags>
void SMLayerScrolling<RGB, optionFlags>::setWindow(int x, int y, int w, int h) {
	fontTopOffset-= yStart;
    xStart = x;
	yStart = y;
	xEnd = x + w;
	yEnd = y + h;
	fontTopOffset+= y;
	majorScrollFontChange = true;
    //Serial.printf("yEnd: %i, y: %i, h: %i\n", yEnd, y, h);
}

// if font size or position changed since the last call, redraw the whole frame
template <typename RGB, unsigned int optionFlags>
void SMLayerScrolling<RGB, optionFlags>::redrawScrollingText(void) {
    int j, k;
    int charPosition, textPosition;
    uint16_t charY0, charY1;

   // Serial.printf("height: %i, chars:%i\n", scrollFont->Height, scrollFont->Chars);
    //for (j = 0; j < this->localHeight; j++) {
	for (j = yStart; j < yEnd; j++) {

        // skip rows without text
        if (j < fontTopOffset || j >= fontTopOffset + scrollFont->Height)
            continue;

        // now in row with text
        // find the position of the first char
        charPosition = scrollPosition;
        textPosition = 0;

        // move to first character at least partially on screen
        //while (charPosition + scrollFont->Width < 0 ) {
		uint8_t charWidth = getBitmapFontCharWidth(text[textPosition], scrollFont); 
		while ((charPosition + charWidth < 0) && (textPosition < textlen - 1)) {			
            //charPosition += scrollFont->Width;
			charPosition += charWidth;
			charWidth = getBitmapFontCharWidth(text[++textPosition], scrollFont);  
        }

        // find rows within character bitmap that will be drawn (0-font->height unless text is partially off screen)
        charY0 = j - fontTopOffset;

        if (yEnd < fontTopOffset + scrollFont->Height) {
            charY1 = yEnd - fontTopOffset;
        } else {
            charY1 = scrollFont->Height;
        }

        if(majorScrollFontChange) {
            // clear full refresh buffer before copying background over, size or position may have changed, can't just clear rows used by font
            memset(scrollingBitmap, 0x00, SCROLLING_BUFFER_SIZE);
			if (this->drawBackColor)
				memset(scrollingBackBitmap, 0x00, SCROLLING_BUFFER_SIZE);
            majorScrollFontChange = false;
        } else {
            // clear rows used by font before drawing on top
            for (k = 0; k < charY1 - charY0; k++) {
                //if (((j + k) * SCROLLING_BUFFER_ROW_SIZE) < this->matrixWidth * (this->matrixHeight / 8)) {
                memset(&scrollingBitmap[((j + k) * SCROLLING_BUFFER_ROW_SIZE)], 0x00, SCROLLING_BUFFER_ROW_SIZE);
				if (this->drawBackColor)
					memset(&scrollingBackBitmap[((j + k) * SCROLLING_BUFFER_ROW_SIZE)], 0x00, SCROLLING_BUFFER_ROW_SIZE);
			}
           // }
        }

        while ((textPosition < textlen) && (charPosition < xEnd)) {
            uint8_t tempBitmask = 0;
			uint8_t tempBackBitmask = 0xFF;
			int charWidth;
			uint16_t idxBitmap; 
			charWidth = getBitmapFontCharWidth(text[textPosition], scrollFont);//   scrollFont->Widths[getBitmapFontLocation(text[textPosition], scrollFont)];
            // draw character from top to bottom
            for (k = charY0; k < charY1; k++) {
                uint8_t xChar;			
				for (xChar = 0; xChar < charWidth; xChar+=8) {
					if ((charPosition + xChar > -8) && (charPosition + xChar < xEnd)) {
						tempBitmask = getBitmapFontRowAtXY(text[textPosition], xChar, k, scrollFont);
						if (this->drawBackColor) {
							tempBackBitmask = 0xFF;
							if (charWidth - xChar < 8)
								tempBackBitmask = tempBackBitmask << (8 - (charWidth - xChar));
						}
						idxBitmap = (j + k - charY0) * SCROLLING_BUFFER_ROW_SIZE;
						if (charPosition + xChar < 0) {
							scrollingBitmap[idxBitmap] |= tempBitmask << -(charPosition + xChar);
							if (this->drawBackColor) {
								scrollingBackBitmap[idxBitmap] |= tempBackBitmask << -(charPosition + xChar);
							}
						} else {
							int w = charWidth - charPosition - xChar;
							//if ((w < 8) && (w > 0))
							//	tempBitmask = tempBitmask & 0xFF << (8 - w);
							scrollingBitmap[idxBitmap + ((charPosition + xChar)/8)] |= tempBitmask >> ((charPosition + xChar)%8);
							if (this->drawBackColor) {
								//if ((w < 8) && (w > 0))
								//	tempBackBitmask = tempBackBitmask & 0xFF << (8 - w);
								scrollingBackBitmap[idxBitmap + ((charPosition + xChar)/8)] |= tempBackBitmask >> ((charPosition + xChar)%8);
							}
							// do two writes if the shifted 8-bit wide bitmask is still on the screen
							if((charPosition + xChar + 8 < (((xEnd + 7) >> 3) << 3)) && (charPosition % 8)) {
							    w+= 8;
								//if ((w < 8) && (w > 0))
								//	tempBitmask = tempBitmask & 0xFF << (8 - w);
								scrollingBitmap[idxBitmap + ((charPosition + xChar)/8) + 1] |= tempBitmask << (8-((charPosition + xChar)%8));
								if (this->drawBackColor) {
									//if ((w < 8) && (w > 0))
									//	tempBackBitmask = tempBackBitmask & 0xFF << (8 - w);
									scrollingBackBitmap[idxBitmap + ((charPosition + xChar)/8) + 1] |= tempBackBitmask << (8-((charPosition + xChar)%8));
								}
							}
						}	
					}								
                }
            }

            // get set up for next character
            charPosition += charWidth; //scrollFont->Width;
            textPosition++;
        }

        j += (charY1 - charY0) - 1;
    }
	changed = true;
}

template <typename RGB, unsigned int optionFlags>
bool SMLayerScrolling<RGB, optionFlags>::getBitmapPixelAtXY(uint8_t x, uint8_t y, uint8_t width, uint8_t height, const uint8_t *bitmap) {
    int cell = (y * ((width / 8) + 1)) + (x / 8);
	
	uint8_t mask = 0x80 >> (x % 8);
	return (mask & bitmap[cell]);
}

