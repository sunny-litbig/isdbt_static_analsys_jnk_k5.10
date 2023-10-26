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
#define LOG_TAG	"subtitle_charset"
#include <utils/Log.h>
#endif

#include <stdio.h>
#include <ISDBT_Caption.h>
#include <TsParser_Subtitle.h>
#include <TsParse_ISDBT_PngDec.h>
#include <subtitle_main.h>
#include <subtitle_display.h>
#include <subtitle_draw.h>
#include <subtitle_memory.h>
#include <subtitle_queue.h>
#include <subtitle_queue_pos.h>
#include <subtitle_system.h>
#include <subtitle_scaler.h>
#include <subtitle_debug.h>

#include "tcc_font.h"

/****************************************************************************
DEFINITION
****************************************************************************/
#define CC_PRINTF	//LOGE
#define ERR_DBG		LOGE

typedef struct{
	unsigned short *phy_addr;
	unsigned short *vir_addr;
	int x;
	int y;
	int w;
	int h;
	int pw;
	int ph;
}SCALE_DATA_TYPE;

/* -- definition of following two struct should be coinsistent with one of middleware/lib_isdbt/src/font/include/isdbt_font.h -- */
typedef struct _VerRotCode_ {
	unsigned short usIn;
	unsigned short usOut;
} VerRotCode;
struct _VerRotTable_ {
	unsigned char	char_set;
	int total_no;
	VerRotCode *pVerRotCode;
};

/****************************************************************************
DEFINITION OF EXTERNAL VARIABLES
****************************************************************************/
/* declaration of these external variables are in isdbt_font.c */
extern VerRotCode KanjiVerRotCode[];
extern VerRotCode HiraganaVerRotCode[];
extern VerRotCode KatakanaVerRotCode[];
extern struct _VerRotTable_ stVerRotTable[];

/****************************************************************************
DEFINITION OF EXTERNAL FUNCTION
****************************************************************************/
extern int isdbt_font_VerticalTableSize(void);

/****************************************************************************
DEFINITION OF GLOVAL VARIABLES
****************************************************************************/

/****************************************************************************
DEFINITION OF LOCAL FUNCTIONS
****************************************************************************/
static int subtitle_charset_make_pair_bitmap(unsigned int *pBuf, int x, int y, int w, int h, int pitch_w, int flc_clr_num, char *p_flc_data, unsigned int color)
{
	int i, j, k, found=0, color_index=0;
	unsigned int pixel = 0;
	int src_index=0, dst_index=0;
	unsigned int y_pos;

	if((pBuf == NULL) || (flc_clr_num == 0) ){
		LOGE("[%s] pBuf:%p, flc_clr_num:%d - error\n", __func__, pBuf, flc_clr_num);
		return -1;
	}

	for(i=0; i<h ; i++){
		y_pos = ((y + i) * pitch_w) + x;
		for(j=0; j<w ; j++){
			src_index = y_pos + j;
			pixel = *(pBuf+src_index);
			for(k=0 ; k<flc_clr_num ; k++){
				color_index = (p_flc_data[k]>(MAX_COLOR-1))?0:p_flc_data[k];		
				if(pixel == g_png_CMLA_Table[color_index]){
					*(pBuf+src_index) = g_png_CMLA_Table[65];
					continue;
				}
			}
		}
	}

	return 0;
}

int subtitle_charset_make_bitmap
(
	T_BITMAP_DATA_TYPE *pPng, double ratio_x, double ratio_y,
	int origin_x, int origin_y, unsigned int color, ISDBT_BMP_INFO *pBmpInfo,
	E_DTV_MODE_TYPE dtv_type
)
{
	int ret = -1, i=0;
	unsigned int png_w = 0, png_h = 0;
	unsigned int buf_pitch_w = 0, buf_pitch_h = 0;
	int png_dec_src_handle=-1, png_dec_dst_handle=-1;
	unsigned int *vir_png_dec_buf=NULL, *vir_dst_buf;
	unsigned int dst_pos_x=0, dst_pos_y=0;
	int coutry_code = -1;

	if((pPng == NULL) || (pBmpInfo == NULL)){
		LOGE("[%s] Null pointer inputted.(%p, %p)\n", __func__, pPng, pBmpInfo);
		return -1;
	}

	buf_pitch_w = subtitle_memory_sub_width();
	buf_pitch_h = subtitle_memory_sub_height();

	for(i=0 ; i<pBmpInfo->total_bmp_num ; i++){
		png_dec_src_handle = subtitle_memory_get_handle();
		if(png_dec_src_handle == -1){
			LOGE("[%s] png_dec_src_handle get fail.\n", __func__);
			ret = -1;
			goto ERROR;
		}

		png_dec_dst_handle = subtitle_memory_get_handle();
		if(png_dec_dst_handle == -1){
			LOGE("[%s] png_dec_dst_handle get fail.\n", __func__);
			ret = -1;
			goto ERROR;
		}

		ret = tcc_isdbt_png_init(pPng->p_data, pPng->size, &png_w, &png_h);
		if(ret){
			LOGE("[%s] image init fail(%d)\n", __func__, ret);
			goto ERROR;
		}

		vir_png_dec_buf = subtitle_memory_get_vaddr(png_dec_src_handle);

		if((dtv_type == ONESEG_ISDB_T) || (dtv_type == FULLSEG_ISDB_T)){
			coutry_code = 0;
		}else{
			coutry_code = 1;
		}

		ret = tcc_isdbt_png_dec(pPng->p_data, pPng->size, \
								vir_png_dec_buf, buf_pitch_w, 0, 0, png_w, png_h, ISDBT_PNGDEC_ARGB8888, coutry_code, 0);
		if(ret){
			LOGE("[%s] image dec fail(%d)\n", __func__, ret);
			goto ERROR;
		}

		if(i==1){
			subtitle_charset_make_pair_bitmap(vir_png_dec_buf,
				0, 0, png_w, png_h, buf_pitch_w,
				pPng->flc_idx_num, pPng->p_flc_idx_grp, color);
		}

		dst_pos_x = ROUNDING_UP((pPng->x + origin_x) * ratio_x);
		dst_pos_y = ROUNDING_UP((pPng->y + origin_y) * ratio_y);
		pBmpInfo->bmp[i].x = dst_pos_x;
		pBmpInfo->bmp[i].y = dst_pos_y;
		pBmpInfo->bmp[i].w = png_w;
		pBmpInfo->bmp[i].h = png_h;

		vir_dst_buf = subtitle_memory_get_vaddr(png_dec_dst_handle);
		if(vir_dst_buf != NULL && buf_pitch_w != 0 && buf_pitch_h != 0){
			memset(vir_dst_buf, 0x0, sizeof(int)*buf_pitch_w*buf_pitch_h);
		}

		subtitle_display_mix_memcpy_ex(vir_png_dec_buf, vir_dst_buf);

		pBmpInfo->bmp[i].handle = png_dec_dst_handle;
		subtitle_memory_put_handle(png_dec_src_handle);
		png_dec_src_handle = -1;
		png_dec_dst_handle = -1;
	}

	if(i == pBmpInfo->total_bmp_num){
		ret = 0;
		goto END;
	}

ERROR:
	ret = -1;
	if(png_dec_src_handle != -1) subtitle_memory_put_handle(png_dec_src_handle);
	if(png_dec_dst_handle != -1) subtitle_memory_put_handle(png_dec_dst_handle);

	for(i=0 ; i<pBmpInfo->total_bmp_num ; i++){
		if(pBmpInfo->bmp[i].handle != -1){
			subtitle_memory_put_handle(pBmpInfo->bmp[i].handle);
			pBmpInfo->bmp[i].handle = -1;
		}
	}

END:
	return ret;
}

int subtitle_charset_make_linux_bitmap
(
	T_BITMAP_DATA_TYPE *pPng, double ratio_x, double ratio_y,
	int origin_x, int origin_y, unsigned short color, ISDBT_BMP_INFO *pBmpInfo,
	E_DTV_MODE_TYPE dtv_type
)
{
	ALOGE("[%s]",__func__);
	int ret = -1, i=0, scale_down=0;
	unsigned int png_w = 0, png_h = 0;
	unsigned int buf_pitch_w = 0, buf_pitch_h = 0;
	int png_dec_src_handle=-1, png_dec_dst_handle=-1;
	unsigned int *phy_png_dec_buf=NULL, *vir_png_dec_buf=NULL, *vir_dst_buf;
	SCALE_DATA_TYPE scale_src, scale_dst;
	unsigned int dst_pos_x=0, dst_pos_y=0;
	int coutry_code = -1;

	if((pPng == NULL) || (pBmpInfo == NULL)){
		LOGE("[%s] Null pointer inputted.(%p, %p)\n", __func__, pPng, pBmpInfo);
		return -1;
	}

	buf_pitch_w = subtitle_memory_sub_width();
	buf_pitch_h = subtitle_memory_sub_height();

	for(i=0 ; i<pBmpInfo->total_bmp_num ; i++){
		png_dec_src_handle = subtitle_memory_get_handle();
		if(png_dec_src_handle == -1){
			LOGE("[%s] png_dec_src_handle get fail.\n", __func__);
			ret = -1;
			goto ERROR;
		}

		png_dec_dst_handle = subtitle_memory_get_handle();
		if(png_dec_dst_handle == -1){
			LOGE("[%s] png_dec_dst_handle get fail.\n", __func__);
			ret = -1;
			goto ERROR;
		}

		ret = tcc_isdbt_png_init(pPng->p_data, pPng->size, &png_w, &png_h);
		if(ret){
			LOGE("[%s] image init fail(%d)\n", __func__, ret);
			goto ERROR;
		}

		phy_png_dec_buf = subtitle_memory_get_paddr(png_dec_src_handle);
		vir_png_dec_buf = subtitle_memory_get_vaddr(png_dec_src_handle);

		if((dtv_type == ONESEG_ISDB_T) || (dtv_type == FULLSEG_ISDB_T)){
			coutry_code = 0;
		}else{
			coutry_code = 1;
		}

		ret = tcc_isdbt_png_dec(pPng->p_data, pPng->size, \
								vir_png_dec_buf, buf_pitch_w, 0, 0, png_w, png_h, ISDBT_PNGDEC_ARGB8888, coutry_code, 0);
		
		if(ret){
			LOGE("[%s] image dec fail(%d)\n", __func__, ret);
			goto ERROR;
		}

		if(i==1){
			subtitle_charset_make_pair_bitmap(vir_png_dec_buf,
				0, 0, png_w, png_h, buf_pitch_w,
				pPng->flc_idx_num, pPng->p_flc_idx_grp, color);
		}

		memset(&scale_src, 0x0, sizeof(SCALE_DATA_TYPE));
		memset(&scale_dst, 0x0, sizeof(SCALE_DATA_TYPE));

		scale_src.phy_addr = phy_png_dec_buf;
		scale_src.vir_addr = vir_png_dec_buf;
		scale_src.x = 0;
		scale_src.y = 0;
		scale_src.w = png_w;
		scale_src.h = png_h;
		scale_src.pw = buf_pitch_w;
		scale_src.ph = buf_pitch_h;

		scale_dst.phy_addr = phy_png_dec_buf;
		scale_dst.vir_addr = vir_png_dec_buf;
		scale_dst.x = 0;
		scale_dst.y = 0;
		scale_dst.w = ROUNDING_UP(png_w * ratio_x);
		scale_dst.h = ROUNDING_UP(png_h * ratio_y);
		scale_dst.pw = buf_pitch_w;
		scale_dst.ph = buf_pitch_h;

		scale_down = ((ratio_x == 1.0) && (ratio_y == 1.0))?0:1;
/*
		if(scale_down){
			ret = subtitle_hw_scaler(scale_src.phy_addr, scale_src.x, scale_src.y, scale_src.w, scale_src.h, buf_pitch_w, buf_pitch_h,
									scale_dst.phy_addr, scale_dst.x, scale_dst.y, scale_dst.w, scale_dst.h, buf_pitch_w, buf_pitch_h);
			if(ret){
				LOGE("[%s] scale fail.\n", __func__);
				goto ERROR;
			}
		}
*/
		dst_pos_x = ROUNDING_UP((pPng->x+ origin_x) * ratio_x);
		dst_pos_y = ROUNDING_UP((pPng->y + origin_y) * ratio_y);

		vir_dst_buf = subtitle_memory_get_vaddr(png_dec_dst_handle);
		if(vir_dst_buf != NULL && buf_pitch_w != 0 && buf_pitch_h != 0){
			memset(vir_dst_buf, 0x0, sizeof(int)*buf_pitch_w*buf_pitch_h);
		}

		subtitle_display_memcpy_ex(scale_dst.vir_addr, 0, 0, scale_dst.w, scale_dst.h, buf_pitch_w,
									vir_dst_buf, dst_pos_x, dst_pos_y, buf_pitch_w);

		pBmpInfo->bmp[i].handle = png_dec_dst_handle;
		subtitle_memory_put_handle(png_dec_src_handle);
		png_dec_src_handle = -1;
		png_dec_dst_handle = -1;
	}

	if(i == pBmpInfo->total_bmp_num){
		ret = 0;
		goto END;
	}

ERROR:
	ret = -1;
	if(png_dec_src_handle != -1) subtitle_memory_put_handle(png_dec_src_handle);
	if(png_dec_dst_handle != -1) subtitle_memory_put_handle(png_dec_dst_handle);

	for(i=0 ; i<pBmpInfo->total_bmp_num ; i++){
		if(pBmpInfo->bmp[i].handle != -1){
			subtitle_memory_put_handle(pBmpInfo->bmp[i].handle);
			pBmpInfo->bmp[i].handle = -1;
		}
	}

END:
	return ret;
}

int subtitle_charset_check_disp_spec(T_SUB_CONTEXT *p_sub_ctx)
{
	int ret = -1;
	int char_path_len = 0, line_dir_len = 0;
	int font_ratio_w = 1, font_ratio_h =1;
	int vs_ratio = 1;
	T_CHAR_DISP_INFO *param = NULL;

	if(p_sub_ctx == NULL){
		LOGE("[%s] Null pointer exception\n", __func__);
		return -1;
	}

	param = (T_CHAR_DISP_INFO*)&p_sub_ctx->disp_info;

	CCPARS_Parse_Pos_CRLF(p_sub_ctx, 0, 0, 1);

	if(((param->dtv_type == ONESEG_ISDB_T)||(param->dtv_type == ONESEG_SBTVD_T)) && (param->uiLineNum > 2)){
		param->uiRepeatCharCount = 1;
		return -2;
	}

	if(param->usDispMode & CHAR_DISP_MASK_FLASHING){
		if(param->usDispMode & CHAR_DISP_MASK_DOUBLE_WIDTH){	font_ratio_w = 2;	}
		if(param->usDispMode & CHAR_DISP_MASK_DOUBLE_HEIGHT){	font_ratio_h = vs_ratio = 2;	}

		if(param->usDispMode & CHAR_DISP_MASK_HWRITE){
			char_path_len = (param->act_font_w * font_ratio_w + param->act_hs);
			line_dir_len = (param->act_font_h * font_ratio_h + param->act_vs * vs_ratio);
		}else{
			char_path_len = (param->act_font_w * font_ratio_w + param->act_vs * vs_ratio);
			line_dir_len = (param->act_font_h * font_ratio_h + param->act_hs);
		}

		if(param->usDispMode & CHAR_DISP_MASK_HALF_WIDTH){		char_path_len >>= 1;	}
		if(param->usDispMode & CHAR_DISP_MASK_HALF_HEIGHT){		line_dir_len >>= 1;	}

		ret = subtitle_queue_pos_put(param->data_type, param->act_pos_x, param->act_pos_y, char_path_len, line_dir_len);
		if(ret == -1){
			LOGE("[%s] ERROR - Can't add flash position data\n", __func__);
		}
	}

	return 0;
}

int	ISDBTCC_Handle_CheckVerticalRotate(unsigned short usDispMode, unsigned char *pInCharCode, unsigned int *pOutCharCode, unsigned char uiCharSet)
{
	int 	chk_result = 0;
	unsigned short	usIn, usOut;
	int	cnt_table, cnt_code;
	const struct _VerRotTable_ *pTable;
	int	VerRotTableCount;

	if (usDispMode & CHAR_DISP_MASK_HWRITE) {
		chk_result = 0;
	} else {
		if (uiCharSet == CS_KANJI) {	// kanji
			usIn = (pInCharCode[0] & 0x7F) << 8;
			usIn |= pInCharCode[1] & 0x7F;
		} else {	// hiragana & katakaga. not considered for others.
			usIn = pInCharCode[0] & 0x7F;
		}
		VerRotTableCount = isdbt_font_VerticalTableSize();
		for (cnt_table=0; cnt_table < VerRotTableCount; cnt_table++)
		{
			if (stVerRotTable[cnt_table].char_set == uiCharSet) {
				pTable = &stVerRotTable[cnt_table];
				for (cnt_code = 0; cnt_code < pTable->total_no; cnt_code++)
				{
					if (pTable->pVerRotCode[cnt_code].usIn == usIn) {
						chk_result = 1;
						usOut= pTable->pVerRotCode[cnt_code].usOut;
						break;
					}
				}
			}
			if (chk_result)
				break;
		}
	}
	if (chk_result)
		*pOutCharCode = usOut;

	return chk_result;
}

/* this functin is not used at this time */
int ISDBTCC_Handle_CheckVerticalRotateNonSpace (unsigned short usDispMode, unsigned int uiNonSpaceChar, unsigned int *pOutCharCode, unsigned char uiCharSet)
{
	int chk_result = 0;

	if (usDispMode & CHAR_DISP_MASK_HWRITE) {
		chk_result = 0;
	} else {
		if (uiCharSet == CS_KANJI || uiCharSet == CS_HIRAGANA || uiCharSet == CS_KATAKANA) {
			if (uiNonSpaceChar == 0xffe3) {
				chk_result = 1;
				*pOutCharCode = 0xfe1a;
			} else if (uiNonSpaceChar == 0xff3f) {
				chk_result = 1;
				*pOutCharCode = 0xfe33;
			}
		}
	}
	return chk_result;
}

void ISDBTCC_Handle_KanjiCharSet(T_SUB_CONTEXT *p_sub_ctx, unsigned char *pCharCode)
{
	int ret = -1;
	int iCharData;
	int iDispCharWidth = 0;
	unsigned int i;
	unsigned int iDispCharNums = 0;
	unsigned char ucFirstByte;
	unsigned char ucSecondByte;
	unsigned int *pDst = NULL;

	T_CHAR_DISP_INFO *param = NULL;

	CC_PRINTF("------> Kanji Characters Set (2-Byte)\n");

	param = (T_CHAR_DISP_INFO*)&p_sub_ctx->disp_info;

	/* Convert the kanji character set to unicode. */
	if ((ret=ISDBT_CC_Convert_Kanji2Unicode(pCharCode, (unsigned int *)&iCharData)) == 1)
	{
		if (param->usDispMode & CHAR_DISP_MASK_VWRITE) {
			ISDBTCC_Handle_CheckVerticalRotate (param->usDispMode, pCharCode, (unsigned int *)&iCharData, CS_KANJI);
		}
		iDispCharNums = 2;

		CC_PRINTF("[%s] ret:%d, 0x%X:0x%X-->0x%X\n", \
			__func__, ret, pCharCode[0]&0x7F, pCharCode[1]&0x7F, iCharData);

		if(ISDBT_CC_Is_NonSpace(pCharCode)){
			param->nonspace_char = iCharData;
			param->usDispMode |= CHAR_DISP_MASK_NONSPACE;
			CC_PRINTF("[%s] Non-space char found(0x%04X)\n", __func__, (unsigned short)*pCharCode);
			return;
		}

		while (param->uiRepeatCharCount > 0)
		{
			CC_PRINTF("[%s] Start[%d:%d], Active[%d:%d:%d:%d], Font[%d:%d:%d:%d], ChNum:%d, Mode:%d, Rep:%d\n", __func__,
				param->origin_pos_x, param->origin_pos_y,
				param->act_pos_x, param->act_pos_y,
				param->act_disp_w, param->act_disp_h,
				param->act_font_w,param->act_font_h,
				param->act_hs,param->act_vs,
				param->uiCharDispCount, param->usDispMode,
				param->uiRepeatCharCount);

			subtitle_charset_check_disp_spec(p_sub_ctx);

			if (param->usDispMode & CHAR_DISP_MASK_HWRITE) {
				if(ISDBT_CC_Is_RuledLineChar((unsigned int)iCharData)){
					param->act_font_w = param->act_font_w + param->act_hs;
					param->act_font_h = param->act_font_h + param->act_vs;
					param->act_hs =0;
					param->act_vs =0;
					CC_PRINTF("[check] [%s] RuledLine char found(0x%X)\n", __func__, iCharData);
				}
			}

			if(p_sub_ctx->disp_handle != -1)
			{
				pDst = subtitle_memory_get_vaddr(p_sub_ctx->disp_handle);

				/* Display the unicode character. */
				iDispCharWidth = ISDBTCap_DisplayUnicode(param, pDst,
											param->act_pos_x,
											param->act_pos_y,
											(const char*)&iCharData,
											(const char *)&param->nonspace_char,
											param->uiForeColor,
											param->uiBackColor,
											param->usDispMode );

				CC_PRINTF("[%s] E_DISP_CTRL_ACTPOS_FORWARD(%d)\n", __func__, iDispCharWidth);

				/* Move to the next position. */
				CCPARS_Parse_Pos_Forward(p_sub_ctx, 0, 1, 0);
			}

			param->uiRepeatCharCount--;

			/* Increase the character display counter. */
			param->uiCharDispCount++;
		}

		if(param->usDispMode & CHAR_DISP_MASK_NONSPACE){
			param->usDispMode &= ~CHAR_DISP_MASK_NONSPACE;
			param->nonspace_char = 0;
		}

		param->uiRepeatCharCount = 1;
	}
	else
	{
		CC_PRINTF("[%s] It's a bad conversion code! (0x%02X%02X)\n", \
			__func__, (pCharCode[0] & 0x7F), (pCharCode[1] & 0x7F));
	}
}

void ISDBTCC_Handle_AlphanumericCharSet(T_SUB_CONTEXT *p_sub_ctx, unsigned char *pCharCode)
{
	int ret = -1;
	int iCharData, char_path_len = 0, line_dir_len = 0;
	int iDispCharWidth = 0;
	unsigned int iDispCharNums = 0, i;
	unsigned int *pDst = NULL;

	T_CHAR_DISP_INFO *param = NULL;

	CC_PRINTF("------> Alphanumeric Characters Set (1-Byte)\n");

	param = (T_CHAR_DISP_INFO*)&p_sub_ctx->disp_info;

	/* Convert the latin extension character set to unicode. */
	iDispCharNums = (param->usDispMode & CHAR_DISP_MASK_HALF_WIDTH)?1:2;

	iCharData = (int)(pCharCode[0] & 0x7F);
	if ((0x21 <= iCharData) && (iCharData <= 0x7E))
	{
		CC_PRINTF("[%s] ret:%d, 0x%X-->0x%X\n", \
			__func__, ret, pCharCode[0]&0x7F, iCharData);

		if((p_sub_ctx->disp_info.dtv_type == ONESEG_ISDB_T) || \
			(p_sub_ctx->disp_info.dtv_type == FULLSEG_ISDB_T)){
			if(iCharData == 0x5C){
				iCharData = 0xFFE5;
			}else if(iCharData == 0x7D){
				iCharData = 0xFF5D;
			}else if(iCharData == 0x7E){
				iCharData = 0xFFE3;
			}else{
				iCharData = 0xFF01 + (iCharData-0x21);
			}
		}

		/* Display the caption character. */
		while (param->uiRepeatCharCount > 0)
		{
			CC_PRINTF("[%s] Start[%d:%d], Active[%d:%d:%d:%d], Font[%d:%d:%d:%d], ChNum:%d, Mode:%d, Rep:%d\n", __func__,
				param->origin_pos_x, param->origin_pos_y,
				param->act_pos_x, param->act_pos_y,
				param->act_disp_w, param->act_disp_h,
				param->act_font_w,param->act_font_h,
				param->act_hs,param->act_vs,
				param->uiCharDispCount, param->usDispMode,
				param->uiRepeatCharCount);

			subtitle_charset_check_disp_spec(p_sub_ctx);

			if(p_sub_ctx->disp_handle != -1)
			{
				pDst = subtitle_memory_get_vaddr(p_sub_ctx->disp_handle);

				/* Display the unicode character. */
				iDispCharWidth = ISDBTCap_DisplayUnicode( param, pDst,
										param->act_pos_x,
										param->act_pos_y,
										(const char*)&iCharData,
										(const char *)&param->nonspace_char,
										param->uiForeColor,
										param->uiBackColor,
										param->usDispMode );

				/* Move to the next position. */
				CCPARS_Parse_Pos_Forward(p_sub_ctx, 0, 1, 0);
			}

			param->uiRepeatCharCount--;

			/* Increase the character display counter. */
			param->uiCharDispCount++;
		}

		if(param->usDispMode & CHAR_DISP_MASK_NONSPACE){
			param->usDispMode &= ~CHAR_DISP_MASK_NONSPACE;
			param->nonspace_char = 0;
		}

		param->uiRepeatCharCount = 1;
	}
	else
	{
		CC_PRINTF(" ALPHANUMERIC][%d] It's a bad conversion code! (0x%02X)\n", __LINE__, (pCharCode[0] & 0x7F));
	}
}

void ISDBTCC_Handle_LatinExtensionCharSet(T_SUB_CONTEXT *p_sub_ctx, unsigned char *pCharCode)
{
	int ret = -1;
	int iCharData, char_path_len = 0, line_dir_len = 0;
	int iDispCharWidth = 0;
	unsigned int iDispCharNums = 0, i;
	unsigned int *pDst = NULL;

	T_CHAR_DISP_INFO *param = NULL;

	CC_PRINTF("------> Latin Extension Characters Set (1-Byte)\n");

	param = (T_CHAR_DISP_INFO*)&p_sub_ctx->disp_info;

	/* Convert the latin extension character set to unicode. */
	if (ISDBT_CC_Convert_LatinExtension2Unicode(pCharCode, (unsigned int *)&iCharData) == 1)
	{
		iDispCharNums = (param->usDispMode & CHAR_DISP_MASK_HALF_WIDTH)?1:2;
		while (param->uiRepeatCharCount > 0)
		{
			CC_PRINTF("[%s] Start[%d:%d], Active[%d:%d:%d:%d], Font[%d:%d:%d:%d], ChNum:%d, Mode:0x%X, Rep:%d\n", __func__,
				param->origin_pos_x, param->origin_pos_y,
				param->act_pos_x, param->act_pos_y,
				param->act_disp_w, param->act_disp_h,
				param->act_font_w,param->act_font_h,
				param->act_hs,param->act_vs,
				param->uiCharDispCount, param->usDispMode,
				param->uiRepeatCharCount);

			subtitle_charset_check_disp_spec(p_sub_ctx);

			if(p_sub_ctx->disp_handle != -1)
			{
				pDst = subtitle_memory_get_vaddr(p_sub_ctx->disp_handle);

				/* Display the unicode character. */
				iDispCharWidth = ISDBTCap_DisplayUnicode( param, pDst,
										param->act_pos_x,
										param->act_pos_y,
										(const char*)&iCharData,
										(const char *)&param->nonspace_char,
										param->uiForeColor,
										param->uiBackColor,
										param->usDispMode );

				/* Move to the next position. */
				CCPARS_Parse_Pos_Forward(p_sub_ctx, 0, 1, 0);
			}

			param->uiRepeatCharCount--;

			/* Increase the character display counter. */
			param->uiCharDispCount++;
		}

		if(param->usDispMode & CHAR_DISP_MASK_NONSPACE){
			param->usDispMode &= ~CHAR_DISP_MASK_NONSPACE;
			param->nonspace_char = 0;
		}

		param->uiRepeatCharCount = 1;
	}
	else
	{
		CC_PRINTF("[%s] It's a bad conversion code! (0x%02X)\n", __func__, (pCharCode[0] & 0x7F));
	}
}

void ISDBTCC_Handle_SpecialCharSet(T_SUB_CONTEXT *p_sub_ctx, unsigned char *pCharCode)
{
	int ret = -1;
	int iCharData, char_path_len = 0, line_dir_len = 0;
	int iDispCharWidth = 0;
	unsigned int iDispCharNums = 0, i;
	unsigned int *pDst = NULL;

	T_CHAR_DISP_INFO *param = NULL;

	CC_PRINTF("------> Specail Characters Set (1-Byte)\n");

	param = (T_CHAR_DISP_INFO*)&p_sub_ctx->disp_info;

	/* Convert the special characters set to unicode. */
	if (ISDBT_CC_Convert_Special2Unicode(pCharCode, (unsigned int *)&iCharData) == 1)
	{
		iDispCharNums = (param->usDispMode & CHAR_DISP_MASK_HALF_WIDTH)?1:2;
		while (param->uiRepeatCharCount > 0)
		{
			CC_PRINTF("[%s] Start[%d:%d], Active[%d:%d:%d:%d], Font[%d:%d:%d:%d], ChNum:%d, Mode:0x%X, Rep:%d\n", __func__,
				param->origin_pos_x, param->origin_pos_y,
				param->act_pos_x, param->act_pos_y,
				param->act_disp_w, param->act_disp_h,
				param->act_font_w,param->act_font_h,
				param->act_hs,param->act_vs,
				param->uiCharDispCount, param->usDispMode,
				param->uiRepeatCharCount);

			subtitle_charset_check_disp_spec(p_sub_ctx);

			if(p_sub_ctx->disp_handle != -1)
			{
				pDst = subtitle_memory_get_vaddr(p_sub_ctx->disp_handle);

				/* Display the unicode character. */
				iDispCharWidth = ISDBTCap_DisplayUnicode( param, pDst,
										param->act_pos_x,
										param->act_pos_y,
										(const char*)&iCharData,
										(const char*)&param->nonspace_char,
										param->uiForeColor,
										param->uiBackColor,
										param->usDispMode );

				/* Move to the next position. */
				CCPARS_Parse_Pos_Forward(p_sub_ctx, 0, 1, 0);
			}

			param->uiRepeatCharCount--;

			/* Increase the character display counter. */
			param->uiCharDispCount++;
		}

		if(param->usDispMode & CHAR_DISP_MASK_NONSPACE){
			param->usDispMode &= ~CHAR_DISP_MASK_NONSPACE;
			param->nonspace_char = 0;
		}

		param->uiRepeatCharCount = 1;
	}
	else
	{
		CC_PRINTF("[%s] It's a bad conversion code! (0x%02X)\n", __func__, (pCharCode[0] & 0x7F));
	}
}

void ISDBTCC_Handle_HiraganaCharSet(T_SUB_CONTEXT *p_sub_ctx, unsigned char *pCharCode)
{
	int ret = -1;
	int iCharData, char_path_len = 0, line_dir_len = 0;
	int iDispCharWidth = 0;
	unsigned int i;
	unsigned int iDispCharNums = 0;
	unsigned int *pDst = NULL;

	T_CHAR_DISP_INFO *param = NULL;

	param = (T_CHAR_DISP_INFO*)&p_sub_ctx->disp_info;

	CC_PRINTF("------> Hiragana Characters Set (1-Byte)\n");

	/* Convert the kanji character set to unicode. */
	if ((ret=ISDBT_CC_Convert_Hiragana2Unicode(pCharCode, (unsigned int *)&iCharData)) == 1)
	{
		if (param->usDispMode & CHAR_DISP_MASK_VWRITE) {
			ISDBTCC_Handle_CheckVerticalRotate (param->usDispMode, pCharCode, (unsigned int *)&iCharData, CS_HIRAGANA);
		}
		iDispCharNums = 2;

		CC_PRINTF("[%s] ret:%d, 0x%X:0x%X-->0x%X\n", \
			__func__, ret, pCharCode[0]&0x7F, pCharCode[1]&0x7F, iCharData);

		while (param->uiRepeatCharCount > 0)
		{
			CC_PRINTF("[%s] Start[%d:%d], Active[%d:%d:%d:%d], Font[%d:%d:%d:%d], ChNum:%d, Mode:0x%X, Rep:%d\n", __func__,
				param->origin_pos_x, param->origin_pos_y,
				param->act_pos_x, param->act_pos_y,
				param->act_disp_w, param->act_disp_h,
				param->act_font_w,param->act_font_h,
				param->act_hs,param->act_vs,
				param->uiCharDispCount, param->usDispMode,
				param->uiRepeatCharCount);

			subtitle_charset_check_disp_spec(p_sub_ctx);

			if(p_sub_ctx->disp_handle != -1)
			{
				pDst = subtitle_memory_get_vaddr(p_sub_ctx->disp_handle);

				/* Display the unicode character. */
				iDispCharWidth = ISDBTCap_DisplayUnicode(param, pDst,
										param->act_pos_x,
										param->act_pos_y,
										(const char*)&iCharData,
										(const char*)&param->nonspace_char,
										param->uiForeColor,
										param->uiBackColor,
										param->usDispMode );

				/* Move to the next position. */
				CCPARS_Parse_Pos_Forward(p_sub_ctx, 0, 1, 0);
			}

			param->uiRepeatCharCount--;

			/* Increase the character display counter. */
			param->uiCharDispCount++;
		}

		if(param->usDispMode & CHAR_DISP_MASK_NONSPACE){
			param->usDispMode &= ~CHAR_DISP_MASK_NONSPACE;
			param->nonspace_char = 0;
		}

		param->uiRepeatCharCount = 1;
	}
	else
	{
		CC_PRINTF("[%s] It's a bad conversion code! (0x%02X%02X)\n", __func__, (pCharCode[0] & 0x7F), (pCharCode[1] & 0x7F));
	}
}

void ISDBTCC_Handle_KataganaCharSet(T_SUB_CONTEXT *p_sub_ctx, unsigned char *pCharCode)
{
	int ret = -1;
	int iCharData, char_path_len = 0, line_dir_len = 0;
	int iDispCharWidth = 0;
	unsigned int i;
	unsigned int iDispCharNums = 0;
	unsigned int *pDst = NULL;

	T_CHAR_DISP_INFO *param = NULL;

	CC_PRINTF("------> Katagana Characters Set (2-Byte)\n");

	param = (T_CHAR_DISP_INFO*)&p_sub_ctx->disp_info;

	/* Convert the kanji character set to unicode. */
	if ((ret=ISDBT_CC_Convert_Katakana2Unicode(pCharCode, (unsigned int *)&iCharData)) == 1)
	{
		if (param->usDispMode & CHAR_DISP_MASK_VWRITE) {
			ISDBTCC_Handle_CheckVerticalRotate (param->usDispMode, pCharCode, (unsigned int *)&iCharData, CS_KATAKANA);
		}
		iDispCharNums = 2;
		while (param->uiRepeatCharCount > 0)
		{
			CC_PRINTF("[%s] Start[%d:%d], Active[%d:%d:%d:%d], Font[%d:%d:%d:%d], ChNum:%d, Mode:0x%X, Rep:%d\n", __func__,
				param->origin_pos_x, param->origin_pos_y,
				param->act_pos_x, param->act_pos_y,
				param->act_disp_w, param->act_disp_h,
				param->act_font_w,param->act_font_h,
				param->act_hs,param->act_vs,
				param->uiCharDispCount, param->usDispMode,
				param->uiRepeatCharCount);

			subtitle_charset_check_disp_spec(p_sub_ctx);

			if(p_sub_ctx->disp_handle != -1)
			{
				pDst = subtitle_memory_get_vaddr(p_sub_ctx->disp_handle);

				/* Display the unicode character. */
				iDispCharWidth = ISDBTCap_DisplayUnicode( param, pDst,
										param->act_pos_x,
										param->act_pos_y,
										(const char*)&iCharData,
										(const char*)&param->nonspace_char,
										param->uiForeColor,
										param->uiBackColor,
										param->usDispMode );

				/* Move to the next position. */
				CCPARS_Parse_Pos_Forward(p_sub_ctx, 0, 1, 0);
			}

			param->uiRepeatCharCount--;

			/* Increase the character display counter. */
			param->uiCharDispCount++;
		}

		if(param->usDispMode & CHAR_DISP_MASK_NONSPACE){
			param->usDispMode &= ~CHAR_DISP_MASK_NONSPACE;
			param->nonspace_char = 0;
		}

		param->uiRepeatCharCount = 1;
	}
	else
	{
		CC_PRINTF("[%s] It's a bad conversion code! (0x%02X%02X)\n", __func__, (pCharCode[0] & 0x7F), (pCharCode[1] & 0x7F));
	}
}

void ISDBTCC_Handle_DRCSCharSet(T_SUB_CONTEXT *p_sub_ctx, unsigned char *pCharCode, unsigned int iCharSet)
{
	int ret = -1;
	DRCS_FONT_DATA *pDRCSFontData = NULL;
	int iFontWidth, iFontHeight;
	unsigned int *pDst = NULL;

	int char_path_len = 0, line_dir_len = 0;
	T_CHAR_DISP_INFO *param = NULL;

	param = (T_CHAR_DISP_INFO*)&p_sub_ctx->disp_info;

	iFontWidth = param->act_font_w;
	if(param->usDispMode & CHAR_DISP_MASK_HALF_WIDTH)
	{
		iFontWidth /= 2;
	}

	iFontHeight = param->act_font_h;
	if(param->usDispMode & CHAR_DISP_MASK_HALF_HEIGHT)
	{
		iFontHeight /= 2;
	}

	/* Get the DRCS data. */
	if (param->dtv_type == ONESEG_ISDB_T || param->dtv_type == ONESEG_SBTVD_T)
		pDRCSFontData = CCPARS_Get_DRCS(pCharCode, iCharSet, 16, 18);		//in 1seg, DRCS size is fixed as 16x18
	else
		pDRCSFontData = CCPARS_Get_DRCS(pCharCode, iCharSet, iFontWidth, iFontHeight);
	if (pDRCSFontData != NULL)
	{
		if (pDRCSFontData->patternData != NULL)
		{
			while (param->uiRepeatCharCount > 0)
			{
				CC_PRINTF("[%s] Start[%d:%d], Active[%d:%d:%d:%d], Font[%d:%d:%d:%d], ChNum:%d, Mode:0x%X, Rep:%d\n", __func__,
					param->origin_pos_x, param->origin_pos_y,
					param->act_pos_x, param->act_pos_y,
					param->act_disp_w, param->act_disp_h,
					param->act_font_w,param->act_font_h,
					param->act_hs,param->act_vs,
					param->uiCharDispCount, param->usDispMode,
					param->uiRepeatCharCount);

				subtitle_charset_check_disp_spec(p_sub_ctx);

				CC_PRINTF("[%s] [x:%d, y:%d, w:%d, h:%d, m:%d]\n", __func__,
							param->act_pos_x, param->act_pos_y,
							(int)pDRCSFontData->width,
							(int)pDRCSFontData->height,
							(int)pDRCSFontData->mode);


				if(p_sub_ctx->disp_handle != -1)
				{
					pDst = subtitle_memory_get_vaddr(p_sub_ctx->disp_handle);

					ISDBTCap_DisplayDRCS(param, pDst,
										param->act_pos_x,
										param->act_pos_y,
										(int)pDRCSFontData->width,
										(int)pDRCSFontData->height,
										pDRCSFontData->patternData,
										pDRCSFontData->mode);

					/* Move to the next position. */
					CCPARS_Parse_Pos_Forward(p_sub_ctx, 0, 1, 0);
				}

				param->uiRepeatCharCount--;

				/* Increase the character display counter. */
				param->uiCharDispCount++;
			}
			param->uiRepeatCharCount = 1;
		}
		else
		{
			CC_PRINTF(" DRCS][%d] There is NOT The DRCS Pattern Data!\n", __LINE__);
		}
	}
	else
	{
		CC_PRINTF(" DRCS][%d] Failed to get The DRCS Data!\n", __LINE__);
	}
}

int ISDBTCap_ControlCharDisp(T_SUB_CONTEXT *p_sub_ctx, int mngr, unsigned int ucControlType, unsigned int param1, unsigned int param2, unsigned int param3)
{
	unsigned int uiForeColor, uiBackColor;
	unsigned int i;
	int iRet, iCharData = 0, iDispCharWidth = 0;
	unsigned int iDispCharNums = 0;
	unsigned char CharCode[4];
	unsigned int *pSrc = NULL, *pDst = NULL;

	T_CHAR_DISP_INFO *param = (T_CHAR_DISP_INFO*)&p_sub_ctx->disp_info;

	/* Handle a display control type. */
	switch (ucControlType)
	{
		case E_DISP_CTRL_INIT:
			if(param1){
				p_sub_ctx->disp_handle = subtitle_memory_get_handle();
				if(p_sub_ctx->disp_handle == -1){
					LOGE("[%s:%d] subtitle_memory_get_handle fail.\n", __func__, __LINE__);
				}
			}else{
				int handle = -1;
				if(p_sub_ctx->backup_handle != -1){
					p_sub_ctx->disp_handle = subtitle_memory_get_handle();
					if(p_sub_ctx->disp_handle == -1){
						LOGE("[%s:%d] subtitle_memory_get_handle fail.\n", __func__, __LINE__);
					}else{
						int w=0, h=0;
						pSrc = subtitle_memory_get_vaddr(p_sub_ctx->backup_handle);
						pDst = subtitle_memory_get_vaddr(p_sub_ctx->disp_handle);
						w = subtitle_memory_sub_width();
						h = subtitle_memory_sub_height();
						if( pDst != NULL && pSrc != NULL )
							memcpy(pDst, pSrc, w*h*sizeof(int));
					}
				}
			}
			break;

		case E_DISP_CTRL_ALERT:
			CC_PRINTF(" ISDB-T] Calling Attention! (Alarm or Signal)\n");
			break;

		case E_DISP_CTRL_SET_POLARITY:
			if(param1 == E_POLARITY_NORMAL){
				;
			}else if(param1 == E_INV_POLARITY_1){
				;
			}else if(param1 == E_INV_POLARITY_2){
				;
			}
			break;

		case E_DISP_CTRL_CLR_SCR:
			{
				unsigned int size = (param->sub_plane_w * param->sub_plane_h);
				if(p_sub_ctx->disp_handle != -1)
				{
					pSrc = subtitle_memory_get_vaddr(p_sub_ctx->disp_handle);
					ISDBTCap_FillBox(param, pSrc, 0, 0,
										param->sub_plane_w, param->sub_plane_h,
										param->uiClearColor);

					tcc_font_setFontSize(TCC_FONT_SIZE_STANDARD,
									ROUNDING_UP(param->act_font_w* param->ratio_x),
									ROUNDING_UP(param->act_font_h* param->ratio_y),
									0,0);
				}
				param->uiCharDispCount++;
			}
			break;

		case E_DISP_CTRL_SPACE:
		case E_DISP_CTRL_DELETE:
			CC_PRINTF("E_DISP_CTRL_SPACE_DEL - Rep:%d, param1:%d",
							param->uiRepeatCharCount, param1);

			CharCode[0] = 0xA1; // or 0x21;
			CharCode[1] = 0xA1; // or 0x21;
			if (ISDBT_CC_Convert_Kanji2Unicode((unsigned char *)CharCode, (unsigned int*)&iCharData) == 1)
			{
				iDispCharNums = (param->usDispMode & CHAR_DISP_MASK_HALF_WIDTH)?1:2;
				//uiBackColor = uiForeColor = (ucControlType == E_DISP_CTRL_SPACE)?param->uiBackColor:param->uiForeColor;
				if (ucControlType == E_DISP_CTRL_SPACE) {
					if(param->uiRasterColor != 0x0){
						if(param->uiBackColor != 0x0){
							uiBackColor = param->uiBackColor;
						}else{
							uiBackColor = param->uiRasterColor;
						}
					}else{
						uiBackColor = param->uiBackColor;
					}
					if (param->usDispMode & CHAR_DISP_MASK_NONSPACE) {
							uiForeColor = param->uiForeColor;
					} else {
							uiForeColor =param->uiBackColor;
					}
				} else {
					uiBackColor = uiForeColor = param->uiForeColor;
				}
				CC_PRINTF("[%s:%d] color(0x%X, 0x%X), handle:%d\n", __func__, __LINE__, \
							uiForeColor, uiBackColor, p_sub_ctx->disp_handle);

				if(p_sub_ctx->disp_handle !=  -1)
				{
					while (param->uiRepeatCharCount > 0)
					{
						subtitle_charset_check_disp_spec(p_sub_ctx);

						pSrc = subtitle_memory_get_vaddr(p_sub_ctx->disp_handle);

						/* Display the unicode character. */
						iDispCharWidth = ISDBTCap_DisplayUnicode( param, pSrc,
												param->act_pos_x,
												param->act_pos_y,
												(const char *)&iCharData,
												(const char *)&param->nonspace_char,
												uiForeColor,
												uiBackColor,
												param->usDispMode );

						/* Move to the next position. */
						CCPARS_Parse_Pos_Forward(p_sub_ctx, mngr, 1, 0);

						param->uiRepeatCharCount--;

						/* Increase the character display counter. */
						param->uiCharDispCount++;
					}
					if (ucControlType == E_DISP_CTRL_SPACE) {
						if(param->usDispMode & CHAR_DISP_MASK_NONSPACE){
							param->usDispMode &= ~CHAR_DISP_MASK_NONSPACE;
							param->nonspace_char = 0;
						}
					}
				}
				param->uiRepeatCharCount = 1;
			}
			else
			{
				ERR_DBG(" ISDB-T] It's a bad conversion code! (0x%02X%02X)\n", (CharCode[0] & 0x7F), (CharCode[1] & 0x7F));
			}
			break;

		case E_DISP_CTRL_NEW_FIFO:
			{
				int cc_handle = -1, disp_handle = -1;
				unsigned int size= 0;

				/* make carbon copy from original disp_handle */
				cc_handle = subtitle_memory_get_handle();
				if((p_sub_ctx->disp_handle != -1) &&(cc_handle != -1))
				{
					size = (param->sub_plane_w * param->sub_plane_h * sizeof(int));

					pSrc = subtitle_memory_get_vaddr(p_sub_ctx->disp_handle);
					pDst = subtitle_memory_get_vaddr(cc_handle);

					if((pDst == NULL) || (size == 0)){
						disp_handle = p_sub_ctx->disp_handle;
					}else{
						if( pDst != NULL && pSrc != NULL )
						{
							memcpy(pDst, pSrc, size);
						}
						disp_handle = cc_handle;
					}
					#if defined(HAVE_LINUX_PLATFORM)
					subtitle_demux_linux_process(param, disp_handle, p_sub_ctx->pts, p_sub_ctx->disp_info.disp_flash, 1);
					#endif
				}
			}
			break;

		case E_DISP_CTRL_CHANGE_PTS:
			if (param1 == (unsigned int)E_PTS_CHANGE_INC)
			{
				/* Increase the caption PTS. */
				p_sub_ctx->pts += (unsigned long long)param2;
			}
			else if (param1 == (unsigned int)E_PTS_CHANGE_DEC)
			{
				/* Decrease the caption PTS. */
				if((unsigned long long)param2 >= p_sub_ctx->pts){
					p_sub_ctx->pts = 0;
				}else{
					p_sub_ctx->pts -= (unsigned long long)param2;
				}
			}
			break;

		case E_DISP_CTRL_CLR_LINE:
			{
				if(p_sub_ctx->disp_handle != -1)
				{
					pSrc = subtitle_memory_get_vaddr(p_sub_ctx->disp_handle);

					if (param1 == (unsigned int)E_CLR_LINE_FROM_FIRST_POS)
					{
						/* Clear the line from front of line to end of line. */
						ISDBTCap_FillBox( param, pSrc,
									ROUNDING_UP(param->origin_pos_x * param->ratio_x),
									ROUNDING_UP(param->act_pos_y * param->ratio_y),
									ROUNDING_UP((param->origin_pos_x + param->act_disp_w) * param->ratio_x),
									ROUNDING_UP((param->origin_pos_x + param->act_font_h) * param->ratio_y),
						                  	param->uiBackColor );
					}
					else if (param1 == (unsigned int)E_CLR_LINE_FROM_CUR_POS)
					{
						/* Clear the line from current position to end of line. */
						ISDBTCap_FillBox( param, pSrc,
									ROUNDING_UP(param->act_pos_x * param->ratio_x),
									ROUNDING_UP(param->act_pos_y * param->ratio_y),
									ROUNDING_UP((param->origin_pos_x + param->act_disp_w) * param->ratio_x),
									ROUNDING_UP((param->act_pos_y + param->act_font_h) * param->ratio_y),
									param->uiBackColor );
					}
				}
			}
			break;

		case E_DISP_CTRL_CLR_REC:
			if(p_sub_ctx->disp_handle != -1)
			{
				pSrc = subtitle_memory_get_vaddr(p_sub_ctx->disp_handle);

				ISDBTCap_FillBox( param, pSrc,
							ROUNDING_UP(param->origin_pos_x * param->ratio_x),
							ROUNDING_UP(param->origin_pos_y * param->ratio_y),
							ROUNDING_UP((param->origin_pos_x + param->act_disp_w) * param->ratio_x),
							ROUNDING_UP((param->origin_pos_y + param->act_disp_h) * param->ratio_y),
				                  	param->uiRasterColor );
			}
			break;

		case E_DISP_CTRL_BMP:
			{
				T_BITMAP_DATA_TYPE	*pBmpData = (T_BITMAP_DATA_TYPE*)param3;
				int ret = -1;
				ISDBT_BMP_INFO	*pBmp = NULL;
				T_CHAR_DISP_INFO *pDispInfo = NULL;

				if(pBmpData != NULL){
					pDispInfo = &p_sub_ctx->disp_info;

					pBmp = (ISDBT_BMP_INFO*)&pDispInfo->bmp_info;

					memset(pBmp, 0x0, sizeof(ISDBT_BMP_INFO));
					pBmp->bmp[0].handle = -1;
					pBmp->bmp[1].handle = -1;

					pBmp->total_bmp_num = (pBmpData->flc_idx_num>0)?2:1;

					#if defined(HAVE_LINUX_PLATFORM)
					ret = subtitle_charset_make_linux_bitmap(pBmpData,
							pDispInfo->ratio_x, pDispInfo->ratio_y,
							pDispInfo->origin_pos_x, pDispInfo->origin_pos_y,
							pDispInfo->uiBackColor, pBmp, pDispInfo->dtv_type);
					#endif
					if(ret){
						pBmp->total_bmp_num = 0;
					}else{
						param->uiCharDispCount += pBmp->total_bmp_num;
					}
				}
			}
			break;
	}

	return (1);
}
