/*
 * SmartMatrix Library - Methods for accessing bitmap fonts
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

#include "SmartMatrix3.h"

// depends on letters in font->Index table being arranged in ascending order
// save location of last lookup to speed up repeated lookups of the same letter
// TODO: use successive approximation to located index faster
int getBitmapFontLocation(unsigned char letter, const bitmap_font *font) {
    static int location = 0;

    if(location < 0)
        location = 0;
	else
		if (location >= font->Chars)
			location = font->Chars - 1;

    if(font->Index[location] == letter)
        return location;

    if(font->Index[location] < letter) {
        for (; location < font->Chars - 1; location++) {
            if (font->Index[location] == letter)
                return location;
        }
    } else {
        for (; location >= 0; location--) {
            if (font->Index[location] == letter)
                return location;
        }
    }

    return -1;
}

int getBitmapFontCharWidth(unsigned char letter, const bitmap_font *font) {
	
	if (!font->Widths)
    	return font->Width;
    int location = getBitmapFontLocation(letter, font);
	
	if (location >= 0) 
		return font->Widths[location]; 
    return 0;

    /*
	int location;
	location = getBitmapFontLocation(letter, font);
	
	if (strlen((char *)font->Widths)) {
		return font->Widths[location];
	} else {
		return font->Width;
	} */
}

bool getBitmapFontPixelAtXY(unsigned char letter, unsigned char x, unsigned char y, const bitmap_font *font)
{
    int location;
	uint8_t widthInBytes;
		
    if (y >= font->Height)
        return false;

    location = getBitmapFontLocation(letter, font);
    widthInBytes = (font->Width + 7) / 8;
	
    if (location < 0)
        return false;

    if (font->Bitmap[(location * font->Height * widthInBytes) + (y * widthInBytes) + (x >> 3)] & (0x80 >> (x % 8)))
		return true;
	else
		return false;
}

uint8_t getBitmapFontRowAtXY(unsigned char letter, unsigned char x, unsigned char y, const bitmap_font *font) {
    int location;
	uint8_t widthInBytes;
	uint8_t rowValue;
	
    if (y >= font->Height)
        return 0x00;

    location = getBitmapFontLocation(letter, font);
 
    if (location < 0)
        return 0x00;
	
	widthInBytes = (font->Width + 7) / 8;
    
	rowValue = font->Bitmap[(location * font->Height * widthInBytes) + (y * widthInBytes) + (x >> 3)];
	return(rowValue);
}

// order needs to match fontChoices enum
static const bitmap_font *fontArray[] = {
    &apple3x5,
    &apple5x7,
    &apple6x10,
    &apple8x13,
    &gohufont6x11,
    &gohufont6x11b,
	&vs12x16
};

const bitmap_font *fontLookup(fontChoices font) {
    return fontArray[font];
}
