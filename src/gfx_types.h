/*
 * gfx_types.h
 *
 *  Created on: Nov 14, 2018
 *      Author: bkg2018
 */

#ifndef SRC_GFX_TYPES_H_
#define SRC_GFX_TYPES_H_

/** Color type */
typedef unsigned char GFX_COL;

/** Drawing modes for text and bitmaps */
typedef enum
{
	drawingNORMAL,			// foreground color on background color (normal text mode)
	drawingXOR,				// pixel xored with background (text only)
	drawingTRANSPARENT,		// non null pixel drawn on background (text only)
} DRAWING_MODE;

/** Standard color codes */
typedef enum
{
	BLACK			= 0x00,
	DARKRED			= 0x01,
	DARKGREEN		= 0x02,
	DARKYELLOW		= 0x03,
	DARKBLUE		= 0x04,
	DARKMAGENTA		= 0x05,
	DARKCYAN		= 0x06,
	GRAY			= 0x07,
	DARKGRAY		= 0x08,
	RED				= 0x09,
	GREEN			= 0x0A,
	YELLOW			= 0x0B,
	BLUE			= 0x0C,
	MAGENTA			= 0x0D,
	CYAN			= 0x0E,
	WHITE			= 0x0F
} DRAWING_COLOR; // compatible with GFX_COL

// Function type for the functions drawing a character in each mode (normal, xor, transparent)
typedef void draw_putc_fun( unsigned int row, unsigned int col, unsigned char c );





#endif /* SRC_GFX_TYPES_H_ */
