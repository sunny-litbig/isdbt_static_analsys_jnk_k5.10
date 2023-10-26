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
#ifndef __SUBTITLE_DRAW_H__
#define __SUBTITLE_DRAW_H__

#ifdef __cplusplus
extern "C" {
#endif

#define ROUNDING_UP(x)			(int)((x)+0.9)

#define	LCD_ROT0L			0
#define	LCD_ROT90L			1
#define	LCD_ROT180L		2
#define	LCD_ROT270L		3
#define	LCD_MIR_ROT0L		10
#define	LCD_MIR_ROT90L	11
#define	LCD_MIR_ROT180L	12
#define	LCD_MIR_ROT270L	13

/* Temprary Half Intensity Color */
#define COLOR_LIGHT_RED		COLOR_RED
#define COLOR_LIGHT_GREEN		COLOR_GREEN
#define COLOR_LIGHT_YELLOW		COLOR_YELLOW
#define COLOR_LIGHT_BLUE		COLOR_BLUE
#define COLOR_LIGHT_MAGENTA	COLOR_MAGENTA
#define COLOR_LIGHT_CYAN		COLOR_CYAN
#define COLOR_LIGHT_WHITE		COLOR_WHITE


typedef struct{
	int act_x;
	int act_y;
	int font_w;
	int font_h;
	int font_hs;
	int font_vs;
	int disp_w;
	int disp_h;
	int pitch_w;
	int pitch_h;
	double ratio_x;
	double ratio_y;
	unsigned int foreColor;
	unsigned int backColor;
	unsigned short dispMode;
}T_CHAR_DISP_INFO_TYPE;

/****************************************************************************
DEFINITION OF STRUCTURE
****************************************************************************/
extern void ISDBTCap_FillBox(void *p_disp_info, unsigned int *pDst, int sx, int sy, int ex, int ey, unsigned int clr);
extern int ISDBTCap_DisplayClear(void *p_disp_info, unsigned int *pDst, int x, int y, int w, int h);
extern int ISDBTCap_DisplayUnicode(void *p_disp_info, unsigned int *pDst, int x, int y, const char *string, const char *non_string, unsigned int fclr, unsigned int bclr, unsigned short usMode);
extern void ISDBTCap_DisplayDRCS(void *p_disp_info, unsigned int *pDst, int x, int y, int width, int height, unsigned char *bitmap, unsigned short usMode);
extern int ISDBTString_MakeUnicodeString(char *pCharStr, short *pUniStr, int lang);
extern int ISDBTString_MakeUnicodeString_For_Message(char *pCharStr, short *pUniStr, int lang);


#ifdef __cplusplus
}
#endif

#endif	/* __SUBTITLE_DRAW_H__ */

