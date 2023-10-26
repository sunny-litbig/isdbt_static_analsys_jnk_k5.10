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
#if defined(HAVE_LINUX_PLATFORM)
#define LOG_NDEBUG 0
#define LOG_TAG	"subtitle_draw"
#include <utils/Log.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <nls770.h>
#include <ISDBT_Caption.h>
#include <subtitle_draw.h>
#include <TsParser_Subtitle.h>
#include <subtitle_display.h>
#include <subtitle_system.h>
#include <subtitle_memory.h>
#include <subtitle_scaler.h>
#include <subtitle_debug.h>
#include "tcc_font.h"


/****************************************************************************
DEFINITION
****************************************************************************/
#define ALIGN_ADDR(a) 		(((a) + (4 - 1)) & -4)


/****************************************************************************
DEFINITION OF EXTERNAL VARIABLES
****************************************************************************/
extern unsigned int *g_pSubtitleDRCSBuffer;
extern void *g_pCaptionDisplayBuffer;
/****************************************************************************
DEFINITION OF EXTERNAL FUNCTIONS
****************************************************************************/
extern void tcc_memset16 (void *s,int c, int n);


/****************************************************************************
DEFINITION OF GLOBAL VARIABLES
****************************************************************************/


/****************************************************************************
DEFINITION OF LOCAL FUNCTIONS
****************************************************************************/
#if 0
void ISDBTCap_FillBox(void *p_disp_info, unsigned short *pDst, int sx, int sy, int ex, int ey, unsigned short clr)
{
	T_CHAR_DISP_INFO *param;
	unsigned int i, j, k, w, h;
	unsigned short *pTgt, *pTgtOld;
	unsigned int *pTgtPtr;
	unsigned int remain=0;
	unsigned short color_16 = clr;
	unsigned int color_32 = ((unsigned int)clr<<16)|(unsigned int)clr;

	param = (T_CHAR_DISP_INFO*)p_disp_info;

	if(pDst == NULL){
		LOGE("[%s] target buffer is NULL.\n", __func__);
		return;
	}

	w = ex - sx;
	h = ey - sy;

	remain = (w%2);
	w >>= 1;

	for(i=0 ; i<h ; i++){
		pTgt = pDst + ((i+sy)*param->sub_plane_w) + sx;
		pTgtPtr = ALIGN_ADDR((unsigned int)pTgt);

		// TODO : compensation of pTgtPtr and pTgt
		for(j=0 ; j<w; j++){
			*(pTgtPtr+j) = color_32;
		}
		if(remain){
			*(pTgtPtr+j) = color_16;
		}
	}
}
#else
void ISDBTCap_FillBox(void *p_disp_info, unsigned int *pDst, int sx, int sy, int ex, int ey, unsigned int clr)
{
	T_CHAR_DISP_INFO *param;
	unsigned int i, j, k, w, h;
	unsigned int *pTgt;
    SUB_SYS_INFO_TYPE *pInfo = (SUB_SYS_INFO_TYPE*)get_sub_context();

	param = (T_CHAR_DISP_INFO*)p_disp_info;

	if(pDst == NULL){
		LOGE("[%s] target buffer is NULL.\n", __func__);
		return;
	}

	w = ex - sx;
	h = ey - sy;

	for(i=0 ; i<h ; i++){
		pTgt = pDst + ((i+sy)*param->sub_plane_w) + sx;
                if(pInfo->isdbt_type == ONESEG_ISDB_T){
                		for(j=0; j<w; j++){
							*pTgt++ = 0x0;
                		}
//                        tcc_memset16 (pTgt, 0x0, (int)w);
                }else if ((pInfo->isdbt_type == ONESEG_SBTVD_T)
                        || (pInfo->isdbt_type == FULLSEG_ISDB_T)
                        || (pInfo->isdbt_type == FULLSEG_SBTVD_T)){
                        for(j=0; j<w; j++){
							*pTgt++ = clr;
                		}

//						tcc_memset16 (pTgt, (int)clr, (int)w);
				}

	}
}
#endif

int ISDBTCap_DisplayClear(void *p_disp_info, unsigned int *pDst, int x, int y, int w, int h)
{
	T_CHAR_DISP_INFO *param;
	unsigned int uiBitmapWD;
	int fclr, bclr;

	param = (T_CHAR_DISP_INFO*)p_disp_info;

	tcc_font_setDestBuf(pDst);

	tcc_font_setDestRegion(param->sub_plane_w, param->sub_plane_h, param->sub_plane_w);

	tcc_font_setFontSize(TCC_FONT_SIZE_STANDARD,
						ROUNDING_UP(w * param->ratio_x),
						ROUNDING_UP(h * param->ratio_y),
						0,0);

	fclr = param->uiForeColor;
	bclr = param->uiBackColor;

	tcc_font_setFontColor(fclr, fclr, bclr, bclr);

	uiBitmapWD = tcc_font_clearChar(TCC_FONT_TYPE_NONE,
						ROUNDING_UP(x * param->ratio_x),
						ROUNDING_UP(y * param->ratio_y));

	return (int)uiBitmapWD;
}

int ISDBTCap_DisplayUnicode(void *p_disp_info, unsigned int *pDst, int x, int y, const char *string, const char *non_string, unsigned int fclr, unsigned int bclr, unsigned short usMode)
{
	T_CHAR_DISP_INFO *param;

	TCCFONTSIZE FontSize;
	TCCFONTTYPE FontType;

	unsigned int uiChar = *(unsigned int*)string;
	unsigned int uiNonSpaceChar = *(unsigned int *)non_string;
	unsigned int uiBitmapWD;

	param = (T_CHAR_DISP_INFO*)p_disp_info;

/*
	if((param->act_disp_w > subtitle_memory_sub_width())
		||(param->act_disp_h > subtitle_memory_sub_height()) ){
		LOGE("Display plane is larger than allocated subtitle memory\n");
		return 0;
	}
*/

	if(param->usDispMode & CHAR_DISP_MASK_HWRITE){
		tcc_font_setDirection(TCC_FONT_DIRECTION_HORIZONTAL, TCC_FONT_BASE_POSITION_BOTTOM);
	}else{
		tcc_font_setDirection(TCC_FONT_DIRECTION_VERTICAL, TCC_FONT_BASE_POSITION_TOP_MIDDLE);
	}

	tcc_font_setDestBuf(pDst);

	tcc_font_setDestRegion(param->sub_plane_w, param->sub_plane_h, param->sub_plane_w);

	switch(usMode & (CHAR_DISP_MASK_HALF_WIDTH|CHAR_DISP_MASK_HALF_HEIGHT|CHAR_DISP_MASK_DOUBLE_WIDTH|CHAR_DISP_MASK_DOUBLE_HEIGHT))
	{
		case CHAR_DISP_MASK_HALF_WIDTH:
			FontSize = TCC_FONT_SIZE_MEDIUM;
		break;

		case (CHAR_DISP_MASK_HALF_WIDTH|CHAR_DISP_MASK_HALF_HEIGHT):
			FontSize = TCC_FONT_SIZE_SMALL;
		break;

		case CHAR_DISP_MASK_DOUBLE_WIDTH:
			FontSize = TCC_FONT_SIZE_DOUBLE_HORIZONTAL;
		break;

		case CHAR_DISP_MASK_DOUBLE_HEIGHT:
			FontSize = TCC_FONT_SIZE_DOUBLE_VERTICAL;
		break;

		case (CHAR_DISP_MASK_DOUBLE_WIDTH|CHAR_DISP_MASK_DOUBLE_HEIGHT):
			FontSize = TCC_FONT_SIZE_DOUBLE_HORIZONTAL_VERTICAL;
		break;

		default:
			FontSize = TCC_FONT_SIZE_STANDARD;
		break;
	}

	tcc_font_setFontSize(FontSize,
						ROUNDING_UP(param->act_font_w * param->ratio_x),
						ROUNDING_UP(param->act_font_h * param->ratio_y),
						ROUNDING_UP(param->act_hs * param->ratio_x),
						ROUNDING_UP(param->act_vs * param->ratio_y));

	tcc_font_setFontColor(fclr, fclr, bclr, bclr);
	if (usMode & (CHAR_DISP_MASK_UNDERLINE | CHAR_DISP_MASK_HIGHLIGHT))
		tcc_font_setFontLineEffectColor(param->uiForeColor);
	else
		tcc_font_setFontLineEffectColor(param->uiBackColor);

	FontType = TCC_FONT_TYPE_NONE;
	if (usMode & CHAR_DISP_MASK_UNDERLINE)
	{
		FontType |= TCC_FONT_TYPE_UNDERLINE;
	}
	if (usMode & CHAR_DISP_MASK_HIGHLIGHT)
	{
		FontType |= param->disp_highlight;
	}

	uiBitmapWD = tcc_font_displayChar(FontType, uiNonSpaceChar, uiChar,
										ROUNDING_UP(x * param->ratio_x),
										ROUNDING_UP(y * param->ratio_y));

	return (int)uiBitmapWD;
}

void ISDBTCap_DisplayDRCS(void *p_disp_info, unsigned int *pDst, int x, int y, int width, int height, unsigned char *bitmap, unsigned short usMode)
{
	T_CHAR_DISP_INFO *param;

	param = (T_CHAR_DISP_INFO*)p_disp_info;

	tcc_font_setDestBuf(pDst);

	tcc_font_setDestRegion(param->sub_plane_w, param->sub_plane_h, param->sub_plane_w);

	tcc_font_setFontSize(TCC_FONT_SIZE_STANDARD,
						ROUNDING_UP(width * param->ratio_x * param->ratio_drcs_x),
						ROUNDING_UP(height * param->ratio_y * param->ratio_drcs_y),
						ROUNDING_UP(param->act_hs * param->ratio_x),
						ROUNDING_UP(param->act_vs * param->ratio_y));

	tcc_font_setFontColor(param->uiForeColor, param->uiHalfForeColor, param->uiBackColor, param->uiHalfBackColor);

	if ((param->ratio_x * param->ratio_drcs_x == 1) && (param->ratio_y * param->ratio_drcs_y == 1))
	{
		tcc_font_displayDRCS(usMode,
							(int)(param->act_pos_x), (int)(param->act_pos_y),
							width, height, bitmap);
	}
	else
	{
		tcc_font_displayDRCSwithScale(usMode,
							ROUNDING_UP(x * param->ratio_x),
							ROUNDING_UP(y * param->ratio_y),
							ROUNDING_UP(width * param->ratio_x * param->ratio_drcs_x),
							ROUNDING_UP(height * param->ratio_y * param->ratio_drcs_y),
							width, height, bitmap);
	}

	return;
}

int ISDBTString_MakeUnicodeString(char *pCharStr, short *pUniStr, int lang)
{
	int iNameStringLength;
	unsigned short usOutStringSize, usUniStringSize;
	unsigned char outstring_buf[MAX_EPG_CONV_TXT_BYTE];

	if((pCharStr==NULL) || (pUniStr==NULL))
		return 0;

	iNameStringLength = strlen((char *)pCharStr);

	usOutStringSize = ISDBT_EPG_DecodeCharString((unsigned char*)pCharStr, outstring_buf, iNameStringLength, lang);
	usUniStringSize = ISDBT_EPG_ConvertCharString(outstring_buf, usOutStringSize, (unsigned short*)pUniStr);

	return usUniStringSize;
}

int ISDBTString_MakeUnicodeString_For_Message(char *pCharStr, short *pUniStr, int lang)
{
	int iNameStringLength;
	unsigned short usOutStringSize, usUniStringSize;
	unsigned char outstring_buf[2048];

	if((pCharStr==NULL) || (pUniStr==NULL))
		return 0;

	iNameStringLength = strlen((char *)pCharStr);

	usOutStringSize = ISDBT_EPG_DecodeCharString((unsigned char*)pCharStr, outstring_buf, iNameStringLength, lang);
	usUniStringSize = ISDBT_EPG_ConvertCharString(outstring_buf, usOutStringSize, (unsigned short*)pUniStr);

	return usUniStringSize;
}


