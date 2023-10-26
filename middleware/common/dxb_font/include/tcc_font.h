/*******************************************************************************

*   Copyright (c) Telechips Inc.


*   TCC Version 1.0

This source code contains confidential information of Telechips.

Any unauthorized use without a written permission of Telechips including not
limited to re-distribution in source or binary form is strictly prohibited.

This source code is provided "AS IS" and nothing contained in this source code
shall constitute any express or implied warranty of any kind, including without
limitation, any warranty of merchantability, fitness for a particular purpose
or non-infringement of any patent, copyright or other third party intellectual
property right.
No warranty is made, express or implied, regarding the information's accuracy,
completeness, or performance.

In no event shall Telechips be liable for any claim, damages or other
liability arising from, out of or in connection with this source code or
the use in the source code.

This source code is provided subject to the terms of a Mutual Non-Disclosure
Agreement between Telechips and Company.
*
*******************************************************************************/
#ifndef _TCC_FONT_H_
#define _TCC_FONT_H_

#define TCC_FONT_SUCCESS								0
#define TCC_FONT_ERROR									(-1)
#define TCC_FONT_ERROR_INVALID_PARAM					(-2)
#define TCC_FONT_ERROR_INVALID_AREA						(-3)
#define TCC_FONT_ERROR_CANNOT_FIND_BITMAP				(-4)
#define TCC_FONT_ERROR_LINE_FULL						(-5)

#define TCC_FONT_SCALE_BUF_SIZE							20736

#define TCC_FONT_HIGHLIGHT_CORRECTION 					3

typedef enum
{
	TCC_FONT_DIRECTION_HORIZONTAL,
	TCC_FONT_DIRECTION_VERTICAL

} TCCFONTDIRECTION;

typedef enum
{
	TCC_FONT_BASE_POSITION_TOP_LEFT,
	TCC_FONT_BASE_POSITION_TOP_MIDDLE,
	TCC_FONT_BASE_POSITION_BOTTOM

} TCCFONTBASEPOSITION;

typedef enum
{
	TCC_FONT_SIZE_STANDARD,
	TCC_FONT_SIZE_MEDIUM,
	TCC_FONT_SIZE_SMALL,
	TCC_FONT_SIZE_DOUBLE_HORIZONTAL,
	TCC_FONT_SIZE_DOUBLE_VERTICAL,
	TCC_FONT_SIZE_DOUBLE_HORIZONTAL_VERTICAL

} TCCFONTSIZE;

typedef enum
{
	TCC_FONT_TYPE_NONE = 0x0,
	TCC_FONT_TYPE_HIGHLIGHTING = 0xF,
	TCC_FONT_TYPE_OUTLINE = 0x10,
	TCC_FONT_TYPE_UNDERLINE = 0x20,

} TCCFONTTYPE;

typedef enum
{
	TCC_FONT_DRCS_2_GRADATION,
	TCC_FONT_DRCS_MULTI_GRADATION,
	TCC_FONT_DRCS_2_COLOUR_WITH_COMPRESSION,
	TCC_FONT_DRCS_MULTI_COLOUR_WITH_COMPRESSION

} TCCFONTDRCSMODE;

int tcc_font_init(void);
int tcc_font_close(void);

int tcc_font_setDestBuf(unsigned int *pBuf);
int tcc_font_setDestRegion(unsigned int uiWidth, unsigned int uiHeight, unsigned int uiPitch);
int tcc_font_clearDestBuf(unsigned int uiColor);

int tcc_font_setDirection(TCCFONTDIRECTION Direction, TCCFONTBASEPOSITION BasePosition);

int tcc_font_setFontSize( TCCFONTSIZE Size, unsigned int uiWidth, unsigned int uiHeight, unsigned int uiHoriSpace, unsigned int uiVertSpace);
int tcc_font_setFontColor(unsigned int uiForeColor, unsigned int uiHalfForeColor, unsigned int uiBackColor, unsigned int uiHalfBackColor);
int tcc_font_setFontLineEffectColor (unsigned int uiLineEffectColor);

int tcc_font_clearChar(TCCFONTTYPE Type, unsigned int uiX, unsigned int uiY);
int tcc_font_displayChar(TCCFONTTYPE Type, unsigned int uiNonSpaceChar, unsigned int uiChar, unsigned int uiX, unsigned int uiY);

int tcc_font_displayDRCS(TCCFONTDRCSMODE Mode, unsigned int uiX, unsigned int uiY, unsigned int uiBitmapWidth, unsigned int uiBitmapHeight, unsigned char* pBitmap);
int tcc_font_displayDRCSwithScale(TCCFONTDRCSMODE Mode, unsigned int uiX, unsigned int uiY, unsigned int uiWidth, unsigned int uiHeight, unsigned int uiBitmapWidth, unsigned int uiBitmapHeight, unsigned char* pBitmap);

#endif //_TCC_FONT_H_
