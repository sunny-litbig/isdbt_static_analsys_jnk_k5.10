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
#include <TsParse.h>
#include <ISDBT_Caption.h>
#include <TsParser_Subtitle.h>
#include "TsParser_Subtitle_Pos.h"
#include "TsParser_Subtitle_Debug.h"


void CCPARS_Parse_Default_Init(T_SUB_CONTEXT *p_sub_ctx, int mngr)
{
	T_CHAR_DISP_INFO *param = (T_CHAR_DISP_INFO*)&p_sub_ctx->disp_info;

	LIB_DBG(DBG_C0, "[%s] disp_format(%d)\n", __func__, param->disp_format);

	if((param->dtv_type == ONESEG_ISDB_T) || (param->dtv_type == ONESEG_SBTVD_T)){
		;
	}else{
		param->def_font_w = 36;
		param->def_font_h = 36;
		
		if(param->disp_format == SUB_DISP_H_960X540){
			param->usDispMode = CHAR_DISP_MASK_HWRITE;
			param->def_hs = 4;
			param->def_vs = 24;
		}else if(param->disp_format == SUB_DISP_V_960X540){
			param->usDispMode = CHAR_DISP_MASK_VWRITE;	
			param->def_hs = 12;
			param->def_vs = 24;
		}else if(param->disp_format == SUB_DISP_H_720X480){
			param->usDispMode = CHAR_DISP_MASK_HWRITE;	
			param->def_hs = 4;
			param->def_vs = 16;
		}else if(param->disp_format == SUB_DISP_V_720X480){
			param->usDispMode = CHAR_DISP_MASK_VWRITE;	
			param->def_hs = 8;
			param->def_vs = 24;
		}else{
			param->usDispMode = CHAR_DISP_MASK_HWRITE;
			param->def_hs = 4;
			param->def_vs = 24;
		}
	}

	LIB_DBG(DBG_C0, "subtitle[%s] def[%d:%d - %d:%d], mode:0x%X\n", __func__, \
		param->def_hs, param->def_vs, param->def_font_w, param->def_font_h, \
		param->usDispMode);
}

void CCPARS_Parse_Pos_Init(T_SUB_CONTEXT *p_sub_ctx, int mngr, int index, int data)
{
	int line_dir_len = 0, char_path_len = 0;
	T_CHAR_DISP_INFO *param = (T_CHAR_DISP_INFO*)&p_sub_ctx->disp_info;

	LIB_DBG(DBG_C0, "[%s] index:0x%x, mngr:%d, data:%d\n", __func__, index, mngr, data);

	if(index == E_INIT_DATA_HEAD_MNG){
		param->mngr_disp_w = param->sub_plane_w;
		param->mngr_disp_h = param->sub_plane_h;
		
		param->mngr_pos_x = 0;
		param->mngr_pos_y = 0;
		param->origin_pos_x = 0;
		param->origin_pos_y = 0;

		param->mngr_font_w = 0;
		param->mngr_font_h = 0;
		param->mngr_hs = -1;
		param->mngr_vs = -1;

		param->uiPolChanges = 0;
		param->uiOriForeColor = param->uiClearColor;
		param->uiOriBackColor = param->uiClearColor;
		param->uiOriHalfForeColor = param->uiClearColor;
		param->uiOriHalfBackColor = param->uiClearColor;
		
		param->uiBackColor = param->uiClearColor;
		param->uiRasterColor = param->uiClearColor;
		param->uiCharDispCount = 0;
	}else if((index == E_INIT_DATA_HEAD_STATE)||(index == E_INIT_CTRL_CODE_CS)){
		param->uiLineNum	= 0;
		param->uiCharNumInLine = 0;
		param->uiFontEffect = E_FONT_EFFECT_NONE;

		param->uiPolChanges = 0;
		param->uiOriForeColor = param->uiClearColor;
		param->uiOriBackColor = param->uiClearColor;
		param->uiOriHalfForeColor = param->uiClearColor;
		param->uiOriHalfBackColor = param->uiClearColor;

		param->uiForeColor	= g_CMLA_Table[7];
		param->uiBackColor	= param->uiClearColor;
		param->uiHalfForeColor = g_CMLA_Table[15];
		param->uiHalfBackColor = g_CMLA_Table[30];
		param->uiRasterColor = param->uiClearColor;
			
		param->uiRepeatCharCount = 1;
		param->uiPaletteNumber = 0;

		param->disp_flash = 0;
		param->uiCharDispCount = 0;
		param->nonspace_char = 0;

		param->uiPolarity = E_POLARITY_NORMAL;
		param->uiFontType = E_FONT_NORMAL;

		CCPARS_Parse_Default_Init(p_sub_ctx, mngr);

		param->act_font_w = (param->mngr_font_w != 0)?param->mngr_font_w:param->def_font_w;
		param->act_font_h = (param->mngr_font_h != 0)?param->mngr_font_h:param->def_font_h;
		
		if(param->mngr_hs != -1){
			param->act_hs = param->mngr_hs;
		}else{
			param->act_hs = param->def_hs;
		}

		if(param->mngr_vs != -1){
			param->act_vs = param->mngr_vs;
		}else{
			param->act_vs = param->def_vs;
		}

		if(param->usDispMode & CHAR_DISP_MASK_HWRITE){
			char_path_len = param->act_font_w + param->act_hs;
			line_dir_len = (param->act_font_h + param->act_vs);
		}else{
			char_path_len = param->act_font_w + param->act_vs;
			line_dir_len = (param->act_font_h + param->act_hs);
		}
			
		if(param->usDispMode & CHAR_DISP_MASK_VWRITE)
		{	
			if((param->mngr_disp_w != 0) || (param->mngr_disp_h != 0)){
				param->act_disp_w = param->mngr_disp_w;
				param->act_disp_h = param->mngr_disp_h;
			}else{
				param->act_disp_w = param->sub_plane_w;
				param->act_disp_h = param->sub_plane_h;
			}
			
			if((param->mngr_pos_x != 0) || (param->mngr_pos_y != 0)){
				param->origin_pos_x = param->mngr_pos_x;
				param->origin_pos_y = param->mngr_pos_y;
			}else{
				param->origin_pos_x = 0;
				param->origin_pos_y = 0;
			}

			param->act_pos_x = param->origin_pos_x + param->act_disp_w - (char_path_len>>1);
			param->act_pos_y = param->origin_pos_y;
			LIB_DBG(DBG_C0, "[%s:%d] ACT[%d,%d]\n", __func__, __LINE__, param->act_pos_x, param->act_pos_y);
		}else{
			if((param->mngr_disp_w != 0) || (param->mngr_disp_h != 0)){
				param->act_disp_w = param->mngr_disp_w;
				param->act_disp_h = param->mngr_disp_h;
			}else{
				param->act_disp_w = param->sub_plane_w;
				param->act_disp_h = param->sub_plane_h;
			}
			
			if((param->mngr_pos_x != 0) || (param->mngr_pos_y != 0)){
				param->origin_pos_x = param->mngr_pos_x;
				param->origin_pos_y = param->mngr_pos_y;
			}else{
				param->origin_pos_x = 0;
				param->origin_pos_y = 0;
			}

#if 1 //IS020A-226 1SEG SUBTITLE SIZE(704*132 -> 960X540)
			if(param->dtv_type == ONESEG_ISDB_T){
				param->act_pos_x = 128;
				param->act_pos_y = 408 + line_dir_len;
				param->act_disp_w = 704;
				param->act_disp_h = 132;
			}else{
				param->act_pos_x = param->origin_pos_x;
				param->act_pos_y = param->origin_pos_y + line_dir_len;
			}
#else
			param->act_pos_x = param->origin_pos_x;
			param->act_pos_y = param->origin_pos_y + line_dir_len;
#endif
			LIB_DBG(DBG_C0, "[%s:%d] ACT[%d,%d]\n", __func__, __LINE__, param->act_pos_x, param->act_pos_y);
		}
		
		CCPARS_Init_GSet(param->data_type, param->dtv_type);
	}else if(index == E_INIT_DATA_UNIT_TEXT){
		param->uiCharDispCount = 0;
	}else if(index == E_INIT_DATA_UNIT_GEO){
		param->uiCharDispCount = 0;
	}else if(index == E_INIT_CTRL_CODE_SWF){
		CCPARS_Parse_Default_Init(p_sub_ctx, mngr);
		
		param->usDispMode &= ~(CHAR_DISP_MASK_HWRITE|CHAR_DISP_MASK_VWRITE);
		if(data == 7){
			/* 7 : horizontal writing form in 960x540 */
			param->usDispMode |= CHAR_DISP_MASK_HWRITE;
			if(mngr){
				param->mngr_disp_w = 960;
				param->mngr_disp_h = 540;
			}else{
				param->swf_disp_w = 960;
				param->swf_disp_h = 540;
			}
			param->def_hs = 4;
			param->def_vs = 24;
		}else if(data == 8){
			/* 8 : vertical writing form in 960x540 */
			param->usDispMode |= CHAR_DISP_MASK_VWRITE;
			if(mngr){
				param->mngr_disp_w = 960;
				param->mngr_disp_h = 540;
			}else{
				param->swf_disp_w = 960;
				param->swf_disp_h = 540;
			}
			param->def_hs = 12;
			param->def_vs = 24;
		}else if(data == 9){
			/* 9 : horizontal writing form in 720x480 */
			param->usDispMode |= CHAR_DISP_MASK_HWRITE;
			if(mngr){
				param->mngr_disp_w = 720;
				param->mngr_disp_h = 480;
			}else{
				param->swf_disp_w = 720;
				param->swf_disp_h = 480;
			}
			param->def_hs = 4;
			param->def_vs = 16;
		}else if(data == 10){
			/* 10 : vertical writing form in 720x480 */
			param->usDispMode |= CHAR_DISP_MASK_VWRITE;
			if(mngr){
				param->mngr_disp_w = 720;
				param->mngr_disp_h = 480;
			}else{
				param->swf_disp_w = 720;
				param->swf_disp_h = 480;
			}
			param->def_hs = 8;
			param->def_vs = 24;
		}

		param->act_font_w = (param->mngr_font_w != 0)?param->mngr_font_w:param->def_font_w;
		param->act_font_h = (param->mngr_font_h != 0)?param->mngr_font_h:param->def_font_h;

		if(param->mngr_hs != -1){
			param->act_hs = param->mngr_hs;
		}else{
			param->act_hs = param->def_hs;
		}

		if(param->mngr_vs != -1){
			param->act_vs = param->mngr_vs;
		}else{
			param->act_vs = param->def_vs;
		}

		if(param->usDispMode & CHAR_DISP_MASK_HWRITE){
			char_path_len = param->act_font_w + param->act_hs;
			line_dir_len = (param->act_font_h + param->act_vs);
		}else{
			char_path_len = param->act_font_w + param->act_vs;
			line_dir_len = (param->act_font_h + param->act_hs);
		}

		if(param->usDispMode & CHAR_DISP_MASK_VWRITE){
			if((param->mngr_disp_w != 0) || (param->mngr_disp_h != 0)){
				param->act_disp_w = param->mngr_disp_w;
				param->act_disp_h = param->mngr_disp_h;
			}else{
				param->act_disp_w = param->swf_disp_w;
				param->act_disp_h = param->swf_disp_h;
			}
			
			if((param->mngr_pos_x != 0) || (param->mngr_pos_y != 0)){
				param->origin_pos_x = param->mngr_pos_x;
				param->origin_pos_y = param->mngr_pos_y;
			}else{
				param->origin_pos_x = 0;
				param->origin_pos_y = 0;				
			}

			param->act_pos_x = param->origin_pos_x + param->act_disp_w - (char_path_len>>1);
			param->act_pos_y = param->origin_pos_y;
			LIB_DBG(DBG_C0, "[%s:%d] ACT[%d,%d], font[%d:%d-%d:%d]\n", __func__, __LINE__, \
				param->act_pos_x, param->act_pos_y, param->act_font_w, param->act_font_h, param->act_hs, param->act_vs);
		}else{
			if((param->mngr_disp_w != 0) || (param->mngr_disp_h != 0)){
				param->act_disp_w = param->mngr_disp_w;
				param->act_disp_h = param->mngr_disp_h;
			}else{
				param->act_disp_w = param->swf_disp_w;
				param->act_disp_h = param->swf_disp_h;
			}
			
			if((param->mngr_pos_x != 0) || (param->mngr_pos_y != 0)){
				param->origin_pos_x = param->mngr_pos_x;
				param->origin_pos_y = param->mngr_pos_y;
			}else{
				param->origin_pos_x = 0;
				param->origin_pos_y = 0;
			}
			
			param->act_pos_x = param->origin_pos_x;
			param->act_pos_y = param->origin_pos_y + line_dir_len;
			LIB_DBG(DBG_C0, "[%s:%d] ACT[%d,%d], font[%d:%d-%d:%d]\n", __func__, __LINE__, \
				param->act_pos_x, param->act_pos_y, param->act_font_w, param->act_font_h, param->act_hs, param->act_vs);
		}		
		
		param->uiPolChanges = 0;
		param->uiOriForeColor = param->uiClearColor;
		param->uiOriBackColor = param->uiClearColor;
		param->uiOriHalfForeColor = param->uiClearColor;
		param->uiOriHalfBackColor = param->uiClearColor;
		
		param->uiForeColor	= g_CMLA_Table[7];
		param->uiBackColor	= param->uiClearColor;
		param->uiHalfForeColor = g_CMLA_Table[15];
		param->uiHalfBackColor = g_CMLA_Table[30];
		param->uiRasterColor = param->uiClearColor;
		param->uiPaletteNumber = 0;

		param->uiCharDispCount = 0;
		
		CCPARS_Init_GSet(param->data_type, param->dtv_type);
	}
		
	LIB_DBG(DBG_C0, "[%s] act_pos[%d,%d] font[%dx%d - %d:%d]\n", __func__, 
		param->act_pos_x, param->act_pos_y, param->act_font_w, param->act_font_h, param->act_hs, param->act_vs);
}

void CCPARS_Parse_Pos_CRLF(T_SUB_CONTEXT *p_sub_ctx, int mngr, int count, int dir_check)
{
	T_CHAR_DISP_INFO *param = NULL;
	int char_path_len=0, line_dir_len=0;
	int font_ratio_w = 1, font_ratio_h =1;
	int vs_ratio = 1;
	int do_crlf = 1;
	
	param = (T_CHAR_DISP_INFO*)&p_sub_ctx->disp_info;

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
	
	LIB_DBG(DBG_C0, "[CRLF] - Font[%d:%d,%d:%d], Start[%d:%d], Active[%d:%d:%d:%d], Mode:0x%X, count:%d, line:%d, chk:%d\n",  
			param->act_font_w, param->act_font_h,
			param->act_hs, param->act_vs,
			param->origin_pos_x, param->origin_pos_y,
			param->act_pos_x, param->act_pos_y, 
			param->act_disp_w, param->act_disp_h,
			param->usDispMode, count, param->uiLineNum, dir_check);

	if(param->usDispMode & CHAR_DISP_MASK_HWRITE){
		if(dir_check){
			do_crlf = ((unsigned int)(param->act_pos_x + char_path_len) > (param->origin_pos_x + param->act_disp_w))?1:0;
		}

		if(do_crlf){
			if((unsigned int)(param->act_pos_y + line_dir_len) > (param->origin_pos_y + param->act_disp_h)){
				param->act_pos_y = param->origin_pos_y + line_dir_len;
			}else{
				param->act_pos_y += line_dir_len;
			}
			param->uiLineNum++;
			param->act_pos_x = param->origin_pos_x;
		}
	}else if(param->usDispMode & CHAR_DISP_MASK_VWRITE){
		if(dir_check){
			do_crlf = ((unsigned int)(param->act_pos_y + line_dir_len) > (param->origin_pos_y + param->act_disp_h))?1:0;
		}

		if(do_crlf){
			param->act_pos_x -= char_path_len;
			if(param->act_pos_x < 0){
				param->act_pos_x = param->act_disp_w - char_path_len;
			}
			param->act_pos_y = param->origin_pos_y;
			param->uiLineNum++;
		}
	}
	LIB_DBG(DBG_C0, "[CRLF] - ACT[%d:%d], line:%d\n", \
		param->act_pos_x, param->act_pos_y, param->uiLineNum);
}

void CCPARS_Parse_Pos_Forward(T_SUB_CONTEXT *p_sub_ctx, int mngr, int count, int need_init)	
{
	T_CHAR_DISP_INFO *param = NULL;
	int char_path_len=0, line_dir_len=0, font_ratio_w = 1, font_ratio_h =1;
	int pos_x=0, pos_y=0, char_path_limit_check = 0, line_dir_limit_check = 0;
	int vs_ratio = 1;
	int iCount = 0;

	param = (T_CHAR_DISP_INFO*)&p_sub_ctx->disp_info;
	LIB_DBG(DBG_C0, "[FORWARD] - Font[%d:%d,%d:%d], Start[%d:%d], Active[%d:%d:%d:%d], Mode:0x%X, count:%d, line:%d\n",  
			param->act_font_w, param->act_font_h,
			param->act_hs, param->act_vs,
			param->origin_pos_x, param->origin_pos_y,
			param->act_pos_x, param->act_pos_y, 
			param->act_disp_w, param->act_disp_h,
			param->usDispMode, count, param->uiLineNum);

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

	if (need_init)
	{
		if(param->usDispMode & CHAR_DISP_MASK_HWRITE){
			/* Initialize the x position. */
			param->act_pos_x = param->origin_pos_x;
			param->uiCharNumInLine = 0;
		}else if(param->usDispMode & CHAR_DISP_MASK_VWRITE){
			param->act_pos_x = param->origin_pos_x + param->act_disp_w - (char_path_len>>1);
			param->uiCharNumInLine = 0;
		}else{
			LIB_DBG(DBG_ERR, "[FORWARD] Invalid direction writing\n");
			return;
		}
	}
	
	/* Get a forward count. */
	iCount = (int)count;	

	if(iCount > 0){
		if(param->usDispMode & CHAR_DISP_MASK_HWRITE){
			if(param->usDispMode & CHAR_DISP_MASK_REPEAT){
				param->uiRepeatCharCount = ((param->origin_pos_x + param->act_disp_w - param->act_pos_x)/char_path_len);
				param->usDispMode &= ~(CHAR_DISP_MASK_REPEAT);
			}
			
			/* Set the active position to forward one. - param1 is font width */
			do
			{
				pos_x = param->act_pos_x + char_path_len;

				if((param->dtv_type == ONESEG_ISDB_T) || (param->dtv_type == ONESEG_SBTVD_T)){
					/* APF is not used in 1Seg */
					char_path_limit_check = ((unsigned int)pos_x > param->origin_pos_x + param->act_disp_w)?1:0;
				}
				else{
					char_path_limit_check = ((unsigned int)pos_x >= param->origin_pos_x + param->act_disp_w)?1:0;
				}

				if(char_path_limit_check){
					param->act_pos_x = param->origin_pos_x;
					pos_y = param->act_pos_y + line_dir_len;

					if((unsigned int)pos_y > param->origin_pos_y + param->act_disp_h){
						param->act_pos_y = param->origin_pos_y + line_dir_len;
					}else{
						param->act_pos_y += line_dir_len;
					}
					param->uiLineNum++;
				}else{
					param->act_pos_x += char_path_len;
				}
				iCount--;
			} while (iCount > 0);
		}else if(param->usDispMode & CHAR_DISP_MASK_VWRITE){
			if((param->dtv_type == ONESEG_ISDB_T) || (param->dtv_type == ONESEG_SBTVD_T)){
				LIB_DBG(DBG_ERR, "[FORWARD] - Vertical writing not supported for 1seg.\n");
				return;
			}
			
			if(param->usDispMode & CHAR_DISP_MASK_REPEAT){
				param->uiRepeatCharCount = ((param->origin_pos_y + param->act_disp_h - param->act_pos_y)/line_dir_len);
				param->usDispMode &= ~(CHAR_DISP_MASK_REPEAT);
			}

			do
			{
				pos_y = param->act_pos_y + line_dir_len;

				if((param->dtv_type == ONESEG_ISDB_T) || (param->dtv_type == ONESEG_SBTVD_T)){
					/* APF is not used in 1Seg */
				line_dir_limit_check = ((unsigned int)pos_y > param->origin_pos_y + param->act_disp_h)?1:0;
				}
				else{
					line_dir_limit_check = ((unsigned int)pos_y >= param->origin_pos_y + param->act_disp_h)?1:0;
				}

				if(line_dir_limit_check){
					param->act_pos_y = param->origin_pos_y;
					pos_x = param->act_pos_x - char_path_len;

					if(pos_x <= (int)param->origin_pos_x){
						param->act_pos_x = param->origin_pos_x + param->act_disp_w - (char_path_len>>1);
					}else{
						param->act_pos_x -= char_path_len;
					}
					param->uiLineNum++;
				}else{
					param->act_pos_y += line_dir_len;
				}
				iCount--;
			} while (iCount > 0);
		}
	}	
	LIB_DBG(DBG_C0, "[FORWARD] - ACT[%d:%d], line:%d\n", \
		param->act_pos_x, param->act_pos_y, param->uiLineNum);
}

void CCPARS_Parse_Pos_Backward(T_SUB_CONTEXT *p_sub_ctx, int mngr, int count, int need_init)		
{
	T_CHAR_DISP_INFO *param = NULL;
	int char_path_len=0, line_dir_len=0, font_ratio_w = 1, font_ratio_h =1;
	int pos_x=0, pos_y=0;
	int char_path_limit_check = 0, line_dir_limit_check = 0;
	int vs_ratio = 1;
	int iCount = 0;

	param = (T_CHAR_DISP_INFO*)&p_sub_ctx->disp_info;

	LIB_DBG(DBG_C0, "[BACKWARD] - Font[%d:%d,%d:%d], Start[%d:%d], Active[%d:%d:%d:%d], Mode:0x%X, count:%d, line:%d, need_init:%d\n",  
			param->act_font_w, param->act_font_h,
			param->act_hs, param->act_vs,
			param->origin_pos_x, param->origin_pos_y,
			param->act_pos_x, param->act_pos_y, 
			param->act_disp_w, param->act_disp_h,
			param->usDispMode, count, param->uiLineNum, need_init);
	
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
	
	if (need_init == (unsigned int)E_SET_START_POS_INIT)
	{
		if(param->usDispMode & CHAR_DISP_MASK_HWRITE){
			param->act_pos_x = param->origin_pos_x;
			param->uiCharNumInLine = 0;
		}else if(param->usDispMode & CHAR_DISP_MASK_VWRITE){
			param->act_pos_x = param->origin_pos_x + param->act_disp_w - (char_path_len>>1);
			param->uiCharNumInLine = 0;
		}else{
			LIB_DBG(DBG_ERR, "[FORWARD] Invalid direction writing\n");
			return;
		}
	}

	iCount = (int)count;

	if(param->usDispMode & CHAR_DISP_MASK_HWRITE){
		/* Set the active position to forward one. - param1 is font width */
		do
		{
			pos_x = param->act_pos_x - char_path_len;

			if(pos_x < 0){
				param->act_pos_x = param->origin_pos_x + param->act_disp_w - char_path_len;
				pos_y = param->act_pos_y - line_dir_len;
				if(pos_y <= 0){
					param->act_pos_y = (param->origin_pos_y + param->act_disp_h);
				}else{
					param->act_pos_y -= line_dir_len;
				}
				param->uiLineNum--;
			}else{
				param->act_pos_x -= char_path_len;
			}
			iCount--;
		} while (iCount > 0);
	}else if(param->usDispMode & CHAR_DISP_MASK_VWRITE){
		if((param->dtv_type == ONESEG_ISDB_T) || (param->dtv_type == ONESEG_SBTVD_T)){
			LIB_DBG(DBG_ERR, "[BACKWARD] - Vertical writing not supported for 1seg.\n");
			return;
		}
		
		if(param->usDispMode & CHAR_DISP_MASK_REPEAT){
			param->uiRepeatCharCount = ((param->origin_pos_y - param->act_pos_y)/line_dir_len);
			param->usDispMode &= ~(CHAR_DISP_MASK_REPEAT);
		}

		do
		{
			pos_y = param->act_pos_y - line_dir_len;
			line_dir_limit_check = (pos_y < (int)param->origin_pos_y)?1:0;

			if(line_dir_limit_check){
				pos_x = param->act_pos_x + char_path_len;				

				if(pos_x >= (int)(param->origin_pos_x + param->act_disp_w)){
					param->act_pos_x = param->origin_pos_x + (char_path_len>>1);
				}else{
					param->act_pos_x += char_path_len;
				}
				param->act_pos_y = param->origin_pos_y + param->act_disp_h - line_dir_len;				
				param->uiLineNum--;
			}else{
				param->act_pos_y -= line_dir_len;
			}
			iCount--;
		} while (iCount > 0);
	}	
	
	LIB_DBG(DBG_C0, "[BACKWARD] - ACT[%d:%d], line:%d", param->act_pos_x, param->act_pos_y, param->uiLineNum);
}

void CCPARS_Parse_Pos_Up(T_SUB_CONTEXT *p_sub_ctx, int mngr, int count, int need_init)		
{
	T_CHAR_DISP_INFO *param = NULL;
	int char_path_len = 0, line_dir_len = 0, font_ratio_w = 1, font_ratio_h = 1;
	int pos_x = 0, pos_y = 0;
	int vs_ratio = 1;
	int iCount = 0;

	param = (T_CHAR_DISP_INFO*)&p_sub_ctx->disp_info;
	
	LIB_DBG(DBG_C0, "[UP] - Font[%d:%d,%d:%d], Start[%d:%d], Active[%d:%d:%d:%d], Mode:0x%X, count:%d, line:%d, need_init:%d\n",  
			param->act_font_w, param->act_font_h,
			param->act_hs, param->act_vs,
			param->origin_pos_x, param->origin_pos_y,
			param->act_pos_x, param->act_pos_y, 
			param->act_disp_w, param->act_disp_h,
			param->usDispMode, count, param->uiLineNum, need_init);

	iCount = (int)count;

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

	if(param->usDispMode & CHAR_DISP_MASK_HWRITE){
		do{
			pos_y = param->act_pos_y - line_dir_len;
			if(pos_y <= 0){
				param->act_pos_y = (param->origin_pos_y + param->act_disp_h);
			}else{
				param->act_pos_y -= line_dir_len;
			}
			param->uiLineNum--;
			iCount--;
		}while(iCount > 0);
	}else if(param->usDispMode & CHAR_DISP_MASK_VWRITE){
		if((param->dtv_type == ONESEG_ISDB_T) || (param->dtv_type == ONESEG_SBTVD_T)){
			LIB_DBG(DBG_ERR, "[UP] - Vertical writing not supported for 1seg.\n");
			return;
		}

		do{
			pos_x = param->act_pos_x + char_path_len;
			if(pos_x >= (int)(param->origin_pos_x + param->act_disp_w)){
				param->act_pos_x = param->origin_pos_x + (char_path_len>>1);
			}else{
				param->act_pos_x += char_path_len;
			}
			param->uiLineNum--;
			iCount--;
		}while(iCount > 0);
	}
	
	LIB_DBG(DBG_C0, "[UP] - ACT[%d:%d], line:%d", param->act_pos_x, param->act_pos_y, param->uiLineNum);
}

void CCPARS_Parse_Pos_Down(T_SUB_CONTEXT *p_sub_ctx, int mngr, int count, int need_init)
{
	T_CHAR_DISP_INFO *param = NULL;
	int char_path_len = 0, line_dir_len = 0, font_ratio_w = 1, font_ratio_h = 1;
	int pos_x = 0, pos_y = 0;
	int vs_ratio = 1;
	int iCount = 0;

	param = (T_CHAR_DISP_INFO*)&p_sub_ctx->disp_info;

	LIB_DBG(DBG_C0, "[DOWN] - Font[%d:%d,%d:%d], Start[%d:%d], Active[%d:%d:%d:%d], Mode:0x%X, count:%d, line:%d, need_init:%d\n",  
			param->act_font_w, param->act_font_h,
			param->act_hs, param->act_vs,
			param->origin_pos_x, param->origin_pos_y,
			param->act_pos_x, param->act_pos_y, 
			param->act_disp_w, param->act_disp_h,
			param->usDispMode, count, param->uiLineNum, need_init);

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
	
	if(need_init == E_SET_START_POS_INIT){
		if(param->usDispMode & CHAR_DISP_MASK_HWRITE){
			param->act_pos_x = param->origin_pos_x;
			param->act_pos_y = param->origin_pos_y + line_dir_len;
		}else if(param->usDispMode & CHAR_DISP_MASK_VWRITE){
			param->act_pos_x = param->origin_pos_x + param->act_disp_w - (char_path_len>>1);
			param->act_pos_y = param->origin_pos_y;
		}else{
			LIB_DBG(DBG_ERR, "[DOWN] Invalid direction writing\n");
			return;
		}
	}

	iCount = (int)count;

	if(iCount > 0){
		if(param->usDispMode & CHAR_DISP_MASK_HWRITE){
			do{
				pos_y = param->act_pos_y + line_dir_len;
				if ((unsigned int)pos_y > param->origin_pos_y + param->act_disp_h){
					param->act_pos_y = param->origin_pos_y + line_dir_len;	
				}
				else{
					param->act_pos_y += line_dir_len;
				}
				param->uiLineNum++;
				iCount--;
			}while(iCount > 0);
		}else if(param->usDispMode & CHAR_DISP_MASK_VWRITE){
			if((param->dtv_type == ONESEG_ISDB_T) || (param->dtv_type == ONESEG_SBTVD_T)){
				LIB_DBG(DBG_ERR, "[DOWN] - Vertical writing not supported for 1seg.\n");
				return;
			}

			do{
				pos_x = param->act_pos_x - char_path_len;
				if (pos_x <= (int)param->origin_pos_x){
					param->act_pos_x = param->origin_pos_x + param->act_disp_w - (char_path_len>>1);
				}
				else{
					param->act_pos_x -= char_path_len;
				}
				param->uiLineNum++;
				iCount--;
			}while(iCount > 0);
		}
	}
	
	LIB_DBG(DBG_C0, "[DOWN] - ACT[%d:%d], line:%d", param->act_pos_x, param->act_pos_y, param->uiLineNum);
}
