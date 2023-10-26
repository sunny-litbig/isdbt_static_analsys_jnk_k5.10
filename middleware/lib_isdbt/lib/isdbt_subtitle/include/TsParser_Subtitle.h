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
#ifndef __TSPARSER_SUBTITLE_H__
#define __TSPARSER_SUBTITLE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ISDBT_Caption.h"

#define 	MAX_EPG_CONV_TXT_BYTE	512
#define 	MAX_COLOR	128
#define 	MAX_SOUND	16

extern unsigned int g_CMLA_Table[MAX_COLOR];
extern unsigned int g_png_CMLA_Table[MAX_COLOR];

extern int ISDBT_EPG_DecodeCharString
( 
	unsigned char *pInBuf, 
	unsigned char *pOutBuf, 
	int nLen, 
	unsigned char ucLangCode
);

extern unsigned short ISDBT_EPG_ConvertCharString
(
	unsigned char * 	inbuf,	// euc-jp code buffer
	unsigned short	inlen,	// euc-jp code length
	unsigned short *	outbuf	// unicode buffer
);

extern int CCPARS_Init(SUB_ARCH_TYPE type);
extern int CCPARS_Exit(void);

extern void CCPARS_Parse_Init(T_SUB_CONTEXT *p_sub_ctx, int mngr, int index, int data);
extern int subtitle_lib_comp_grp_type(int data_type);

extern int subtitle_lib_mngr_grp_changed(int data_type);
extern void subtitle_lib_mngr_grp_set(int data_type, int set);
extern int subtitle_lib_mngr_grp_get(int type);

extern int subtitle_lib_sts_grp_changed(int data_type);
extern void subtitle_lib_sts_grp_set(int data_type, int set);
extern int subtitle_lib_sts_grp_get(int type);

extern void CCPARS_Set_DtvMode(int data_type, E_DTV_MODE_TYPE dtv_mode);
extern E_DTV_MODE_TYPE CCPARS_Get_DtvMode(int data_type);

extern int ISDBT_CC_Convert_Kanji2Unicode(const unsigned char *pInCharCode, unsigned int *pOutUCSCode);
extern int ISDBT_CC_Convert_Hiragana2Unicode(const unsigned char *pInCharCode, unsigned int *pOutUCSCode);
extern int ISDBT_CC_Convert_Katakana2Unicode(const unsigned char *pInCharCode, unsigned int *pOutUCSCode);
extern int ISDBT_CC_Convert_LatinExtension2Unicode(const unsigned char *pInCharCode, unsigned int *pOutUCSCode);
extern int ISDBT_CC_Convert_Special2Unicode(const unsigned char *pInCharCode, unsigned int *pOutUCSCode);

extern void CCPARS_Init_Process(T_SUB_CONTEXT *p_sub_ctx, int mngr, int index);

extern void CCPARS_Parse_Pos_CRLF(T_SUB_CONTEXT *p_sub_ctx, int mngr, int count, int need_init);
extern void CCPARS_Parse_Pos_Forward(T_SUB_CONTEXT *p_sub_ctx, int mngr, int count, int need_init);
extern void CCPARS_Parse_Pos_Backward(T_SUB_CONTEXT *p_sub_ctx, int mngr, int count, int need_init);
extern void CCPARS_Parse_Pos_Up(T_SUB_CONTEXT *p_sub_ctx, int mngr, int count, int need_init);
extern void CCPARS_Parse_Pos_Down(T_SUB_CONTEXT *p_sub_ctx, int mngr, int count, int need_init);

#ifdef __cplusplus
}
#endif

#endif	/* __TSPARSER_SUBTITLE_H__ */