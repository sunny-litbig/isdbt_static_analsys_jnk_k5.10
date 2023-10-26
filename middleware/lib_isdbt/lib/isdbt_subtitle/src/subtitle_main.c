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
#define LOG_TAG	"subtitle_main"
#include <utils/Log.h>
#endif

#ifndef uint32_t
typedef unsigned int uint32_t;
#endif

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <linux/fb.h>
#include <cutils/properties.h>
#include <tccfb_ioctrl.h>
#include <BitParser.h>
#include <ISDBT_Caption.h>
#include <TsParser_Subtitle.h>
#include <subtitle_debug.h>
#include <subtitle_main.h>
#include <subtitle_parser.h>
#include <subtitle_charset.h>
#include <subtitle_display.h>
#include <subtitle_system.h>
#include <subtitle_queue.h>
#include <subtitle_queue_pos.h>
#include <subtitle_memory.h>
#include <subtitle_draw.h>
#include <tcc_pthread_cond.h>

/****************************************************************************
DEFINITION
****************************************************************************/
//#define DUMP_INPUT_TO_FILE

/****************************************************************************
DEFINITION OF TYPE
****************************************************************************/

/****************************************************************************
DEFINITION OF EXTERNAL VARIABLES
****************************************************************************/
extern ISDBT_CAPTION_INFO gIsdbtCapInfo[2];

/****************************************************************************
DEFINITION OF GLOBAL VARIABLES
****************************************************************************/
static SUBTITLE_CONTEXT_TYPE g_sub_ctx;
static unsigned long long g_statement_time = 0;
static int subtitleIndex, superimposeIndex = 0;
static int total_subtitle_cnt, subtitle_handle_info=0, g_statement_in = 0;
static int mgmt_num_languages[2];	//0 - caption, 1 - super-impose
static int TotalCapNum, TotalSuperNum;
static unsigned int CapLang1, CapLang2, SuperLang1, SuperLang2;

//00 - caption/1st Language, 01 - caption/2nd Language
//11 - superimpose/1st Language, 11 - superimpose/2nd Language
static unsigned int mgmt_langcode[2][2];

/****************************************************************************
DEFINITION OF EXTERNAL FUNCTIONS
****************************************************************************/
extern void CCPARS_Parse_Pos_Init(T_SUB_CONTEXT *p_sub_ctx, int mngr, int index, int param);


/****************************************************************************
DEFINITION OF LOCAL FUNCTIONS
****************************************************************************/
SUB_SYS_INFO_TYPE* get_sub_context(void)
{
	return (SUB_SYS_INFO_TYPE*)&g_sub_ctx.sys_info;
}

#if defined(DUMP_INPUT_TO_FILE)
static FILE *gp_file_handle = NULL;
void dump_file_open(int data_type)
{
	static unsigned int index = 0;
	char szName[40] = {0,};

	if(gp_file_handle == NULL){
		sprintf(szName, "/mnt/sdcard/dump_%s_%03d.raw", (data_type==0)?"st":"si", index++);
		gp_file_handle = fopen(szName, "wb");
		if(gp_file_handle == NULL){
			LOGE("===>> subtitle dump file open error %d\n", gp_file_handle);
		}else{
			LOGE("===>> subtitle dump file open success(%s)\n", szName);
		}
	}
}

void dump_file_write(char *data, int size)
{
	if( gp_file_handle ){
		fwrite(data, 1, size, gp_file_handle);
		LOGE("===>> write size %d - handle %d\n", size, gp_file_handle);
		fclose(gp_file_handle);
		sync();
		gp_file_handle = NULL;
	}
}
#endif /* DUMP_INPUT_TO_FILE */

void subtitle_demux_set_handle_info(int handle_info)
{
	subtitle_handle_info = handle_info;
}

int subtitle_demux_get_handle_info(void)
{
	return subtitle_handle_info;
}

int subtitle_demux_get_statement_in(void)
{
	return g_statement_in;
}
int subtitle_fb_type()
{
/* 0:CCFB, 1:SURFACE */
#if defined(HAVE_LINUX_PLATFORM)
	return 0;
#endif
}

void subtitle_info_clear(void)
{
	subtitle_handle_info = 0;
	g_statement_in = 0;

	mgmt_num_languages[0] = -1;
	mgmt_num_languages[1] = -1;

	//subtitle
	mgmt_langcode[0][0] = 0;
	mgmt_langcode[0][1] = 0;
	//superimpose
	mgmt_langcode[1][0] = 0;
	mgmt_langcode[1][1] = 0;

	gIsdbtCapInfo[0].ucNumsOfLangCode =0;
	gIsdbtCapInfo[0].LanguageInfo[0].ISO_639_language_code =0;
	gIsdbtCapInfo[0].LanguageInfo[1].ISO_639_language_code =0;

	gIsdbtCapInfo[1].ucNumsOfLangCode =0;
	gIsdbtCapInfo[1].LanguageInfo[0].ISO_639_language_code =0;
	gIsdbtCapInfo[1].LanguageInfo[1].ISO_639_language_code =0;
}

void subtitle_set_num_languages(int data_type, int cnt)
{
	if(data_type < 0 || data_type > 1) {
		LOGE("[%s] type is wrong(%d)", __func__, data_type);
	} else {
		if(cnt > MAX_NUM_OF_ISDBT_CAP_LANG) {
			LOGE("[%s] too much captions(%d:%d)", __func__, data_type, cnt);
			mgmt_num_languages[data_type] = 0;
		} else {
			mgmt_num_languages[data_type] = cnt;
		}
	}
}

int subtitle_get_num_languages(int data_type)
{
	if (data_type < 0 || data_type > 1) {
		LOGE("[%s] type is wrong(%d)", __func__, data_type);
		return 0;
	} else {
		return mgmt_num_languages[data_type];
	}
}

int subtitle_get_lang_info(int data_type, int *NumofLang, unsigned int* subtitleLang1, unsigned int* subtitleLang2)
{
	unsigned int subtLang1 = 0, subtLang2 = 0;

	if (data_type < 0 || data_type > 1) {
		LOGE("[%s] type is wrong(%d)", __func__, data_type);
		return -1;
	}

	*NumofLang = subtitle_get_num_languages(data_type);
	*subtitleLang1 = mgmt_langcode[data_type][0];
	*subtitleLang2 = mgmt_langcode[data_type][1];
	if(*NumofLang == -1){
		*NumofLang = 0;
		//LOGE("[%s] can't get subtitleinfo from mgmt data\n", __func__);
		return -1;
	}
	return 0;
}

void subtitle_set_langcode(int data_type, unsigned int subtitleLang1, unsigned int subtitleLang2)
{
	if(data_type < 0 || data_type > 1) {
		LOGE("[%s] type is wrong(%d)", __func__, data_type);
	} else {
		mgmt_langcode[data_type][0] = subtitleLang1;
		mgmt_langcode[data_type][1] = subtitleLang2;
	}
}

void subtitle_demux_set_statement_in(void)
{
	g_statement_time = subtitle_system_get_systime();
}

int subtitle_demux_check_statement_in(void)
{
	unsigned long long diff = 0;
	unsigned long long stc = 0;
	int ret = 0;

	if(g_statement_time == 0)
		return 0;

	stc = subtitle_system_get_systime();
	diff = stc - g_statement_time;

	if(diff > (2*1000)){
		LOGE("===> stc:%lld, old:%lld, diff:%lld\n", stc, g_statement_time, diff);
		g_statement_time = 0;
		ret = 1;
	}else{
		ret = 0;
	}

	return ret;
}

int subtitle_demux_select_subtitle_lang(int index, int sel_cnt)
{
	if(index > MAX_NUM_OF_ISDBT_CAP_LANG){
		LOGE("[%s] ERROR - Invalid language index selected.\n", __func__);
		return -1;
	}

	if((gIsdbtCapInfo[0].LanguageInfo[0].DMF == 0x0
		|| gIsdbtCapInfo[0].LanguageInfo[0].DMF == 0x1
		|| gIsdbtCapInfo[0].LanguageInfo[0].DMF == 0x2
		|| gIsdbtCapInfo[0].LanguageInfo[0].DMF == 0x3) && index == 0){
		index = 1;
	}else if((gIsdbtCapInfo[0].LanguageInfo[0].DMF == 0xC
		|| gIsdbtCapInfo[0].LanguageInfo[0].DMF == 0xD
		|| gIsdbtCapInfo[0].LanguageInfo[0].DMF == 0xE)
		&& gIsdbtCapInfo[0].LanguageInfo[0].DC == 0x0 && index == 0 ){
		index = 1;
	}
	subtitleIndex = index;
	total_subtitle_cnt = sel_cnt;

	return 0;
}

void subtitle_demux_set_subtitle_lang(int cnt)
{
	int temp_index = 0;

	if(subtitleIndex > MAX_NUM_OF_ISDBT_CAP_LANG){
		LOGE("[%s] ERROR - Invalid language index selected.\n", __func__);
		return;
	}

	if(total_subtitle_cnt){
		if (cnt >= 2)
			temp_index = subtitleIndex;
		else {
			if(subtitleIndex == 0)
				temp_index = 0;
			else
				temp_index = 1;
		}
	} else{
		temp_index = 0;
	}

	if(subtitle_get_dmf_flag() == 1 && subtitle_get_dmf_st_flag() == 1 && temp_index == 0){
		subtitleIndex = temp_index = 1;
	}

	//ALOGE("subtitle [%d:%d:%d:%d]", cnt, subtitleIndex, temp_index, total_subtitle_cnt);
	CCPARS_Set_Lang(0, temp_index);
}

int subtitle_demux_select_superimpose_lang(int index, int sel_cnt)
{
	if(index > MAX_NUM_OF_ISDBT_CAP_LANG){
		LOGE("[%s] ERROR - Invalid language index selected.\n", __func__);
		return -1;
	}

	if((gIsdbtCapInfo[1].LanguageInfo[0].DMF == 0x0
		|| gIsdbtCapInfo[1].LanguageInfo[0].DMF == 0x1
		|| gIsdbtCapInfo[1].LanguageInfo[0].DMF == 0x2
		|| gIsdbtCapInfo[1].LanguageInfo[0].DMF == 0x3) && index == 0){
		index = 1;
	}else if((gIsdbtCapInfo[1].LanguageInfo[0].DMF == 0xC
		|| gIsdbtCapInfo[1].LanguageInfo[0].DMF == 0xD
		|| gIsdbtCapInfo[1].LanguageInfo[0].DMF == 0xE)
		&& gIsdbtCapInfo[1].LanguageInfo[0].DC == 0x0 && index == 0 ){
		index = 1;
	}

	superimposeIndex = index;
	total_subtitle_cnt = sel_cnt;

	return 0;
}

void subtitle_demux_set_superimpose_lang(int cnt)
{
	int temp_index = 0;

	if(superimposeIndex > MAX_NUM_OF_ISDBT_CAP_LANG){
		LOGE("[%s] ERROR - Invalid language index selected.\n", __func__);
		return;
	}

	if(total_subtitle_cnt){
		if (cnt >= 2)
			temp_index = superimposeIndex;
		else {
			if(superimposeIndex == 0)
				temp_index = 0;
			else
				temp_index = 1;
		}
	} else{
		temp_index = 0;
	}

	if(subtitle_get_dmf_flag() == 1 && subtitle_get_dmf_si_flag() == 1 && temp_index == 0){
		superimposeIndex = temp_index = 1;
	}

	//ALOGE("superimpose [%d:%d:%d:%d]\n", cnt, superimposeIndex, temp_index, total_subtitle_cnt);
	CCPARS_Set_Lang(1, temp_index);
}

int subtitle_demux_change_sub_res(SUBTITLE_CONTEXT_TYPE *p_sub_ctx)
{
	ALOGE("[%s]", __func__);
	int ret = -1, i=0;

	if(p_sub_ctx == NULL){
		LOGE("[%s] Null pointer exception.\n", __func__);
		return -1;
	}

	for(i=0 ; i<2 ; i++){
		ret = subtitle_display_init(i, p_sub_ctx);
		if(ret < 0)
			return -1;
	}

#if 1    // Noah, To avoid Codesonar's warning, Redundant Condition
	subtitle_system_get_disp_info((void*)p_sub_ctx);
#else
	ret = subtitle_system_get_disp_info((void*)p_sub_ctx);
	if(ret < 0)
		return -1;
#endif

#if defined (HAVE_LINUX_PLATFORM)    // Noah, To avoid Codesonar's warning, Redundant Condition
//	if(subtitle_fb_type() == 0)
	{
		if(p_sub_ctx->sys_info.disp_open == 1 && subtitle_display_get_memory_sync_flag() == 1){
			//Format changed and memory create state
			ALOGE("subtitle memory re-create\n");
            subtitle_queue_remove_all(0);
            subtitle_queue_remove_all(1);

            subtitle_queue_pos_remove_all(0);
            subtitle_queue_pos_remove_all(1);

            /* dummy display for clear screen */
            subtitle_display_clear();

            /* Old subtitle memory unmap for new one */
            subtitle_memory_destroy();
		}

		ret = subtitle_memory_linux_create(p_sub_ctx->sys_info.res.sub_buf_w, p_sub_ctx->sys_info.res.sub_buf_h);
		if(ret < 0)
			return -1;

		ret = subtitle_display_open(p_sub_ctx->sys_info.act_lcdc_num,
									p_sub_ctx->sys_info.res.disp_x,
									p_sub_ctx->sys_info.res.disp_y,
									p_sub_ctx->sys_info.res.disp_w,
									p_sub_ctx->sys_info.res.disp_h,
									p_sub_ctx->sys_info.res.disp_m,
									p_sub_ctx->sys_info.res.sub_buf_w,
									p_sub_ctx->sys_info.res.sub_buf_h);
		if(ret < 0){
			ERR_DBG("[%s] Can't open display device.\n", __func__);
			return -1;
		}
	}
#endif

	return 0;
}

int subtitle_demux_init(int seg_type, int country, int raw_w, int raw_h, int view_w, int view_h, int init)
{
	int ret, fb_type;
	E_DTV_MODE_TYPE	isdbt_type;
	SUB_ARCH_TYPE arch_type;
//	LOGE("In [%s]\n", __func__);
	subtitle_info_clear();

	g_statement_time = 0;
	memset(&g_sub_ctx, 0x0, sizeof(SUBTITLE_CONTEXT_TYPE));

	fb_type = subtitle_fb_type();
	ret = subtitle_system_init(&g_sub_ctx.sys_info, seg_type, country, \
							fb_type, \
							raw_w, raw_h, view_w, view_h);
	if(ret < 0) return -1;

#if 1
	// Noah, To avoid Codesonar's warning, Redundant Condition
	// CCPARS_Init returns always '0'.
	CCPARS_Init(g_sub_ctx.sys_info.arch_type);
#else
	ret = CCPARS_Init(g_sub_ctx.sys_info.arch_type);
	if(ret < 0) return -1;
#endif

	ret = subtitle_queue_init();
	if(ret < 0) return -1;

	ret = subtitle_queue_pos_init();
	if(ret < 0) return -1;

	ret = subtitle_display_pre_init(g_sub_ctx.sys_info.arch_type, init);
	if(ret < 0) return -1;

	ret = subtitle_display_open_thread();
	if(ret < 0) return -1;

	CCPARS_Set_DtvMode(0, g_sub_ctx.sys_info.isdbt_type);
	CCPARS_Set_DtvMode(1, g_sub_ctx.sys_info.isdbt_type);
//	LOGE("Out [%s]\n", __func__);

	return 0;
}

int subtitle_demux_exit(int init)
{
//	LOGE("In [%s]\n", __func__);
	subtitle_set_dmf_flag(0);

	CCPARS_Set_DtvMode(0, SEG_UNKNOWN);
	CCPARS_Set_DtvMode(1, SEG_UNKNOWN);

	if(init == 1){
		subtitle_display_close_thread(0);
		subtitle_display_close();
	}else{
		subtitle_display_close_thread(1);
	}

	subtitle_queue_pos_exit();
	subtitle_queue_exit();

	subtitle_memory_destroy();
	subtitle_display_exit();

	CCPARS_Exit();
	subtitle_info_clear();

//	LOGE("Out [%s]\n", __func__);

	return 0;
}

int subtitle_demux_dec(int data_type, unsigned char *pData, unsigned int uiDataSize, unsigned long long iPts)
{
	void *pSubBitPars = NULL;
	int j, ret, iRet, iDataUnitSize, result = -1;
	int cur_lang_index =  0, need_update = 0;
	int dgrp_num = 0, dunit_num = 0;
	int old_disp_mode;
	int bmp_count =-1, bmp_handle = -1;
	int control_cnt=0;
	long long idelay;
	unsigned char	*pCurPos, *pCapDataTmp;
	T_CC_CHAR_INFO			stCharInfo;
	TS_PES_CAP_PARM 		stCapParm;
	TS_PES_CAP_DATAGROUP	*pCurDGrp;
	TS_PES_CAP_DATAUNIT 	*pCurDUnit, *pNextDUnit;
	T_SUB_CONTEXT			*p_sub_ctx;
	SUB_SYS_INFO_TYPE		*p_sys_info=NULL;

	if((data_type < 0) || (1 < data_type)){
		LOGE("[%s] Unknown data_type(%d,%lld)\n", __func__, data_type, iPts);
		return -1;
	}

	memset(&stCharInfo, 0x0, sizeof(T_CC_CHAR_INFO));
	memset(&stCapParm, 0x0, sizeof(TS_PES_CAP_PARM));

	subtitle_set_type(data_type);

	p_sub_ctx = &g_sub_ctx.sub_ctx[data_type];
	p_sub_ctx->disp_handle = -1;
	p_sys_info = (SUB_SYS_INFO_TYPE*)&g_sub_ctx.sys_info;

	ret = BITPARS_OpenBitParser( &pSubBitPars );
	if(ret == BITPARSER_DRV_ERR_ALLOC_MEM){
		ERR_DBG(">>> Not enough memory for bitparser\n");
		goto END;
	}

	ret = BITPARS_InitBitParser(pSubBitPars, pData, uiDataSize);
	if(ret != BITPARSER_DRV_NO_ERROR){
		ERR_DBG(">>> Invalid handle for bitparser\n");
		goto END;
	}

	if(uiDataSize > MAX_SIZE_OF_CAPTION_PES){
		CC_PRINTF("pes size(%d) is more than %d\n", uiDataSize, MAX_SIZE_OF_CAPTION_PES);
		goto END;
	}

	dgrp_num = CCPARS_Parse_CCPES(p_sub_ctx, pSubBitPars, pData, uiDataSize, &stCapParm,  &dunit_num);
	if (dgrp_num < 0){
		ERR_DBG("[%s] Fail to parse the PES! [%d]\n", __func__, dgrp_num);
		goto END;
	}

	if(p_sys_info->disp_mode != p_sub_ctx->disp_info.disp_format){
		old_disp_mode = p_sys_info->disp_mode;
		p_sys_info->disp_mode = p_sub_ctx->disp_info.disp_format;

		ret = subtitle_demux_change_sub_res(&g_sub_ctx);
		if(ret < 0){
			p_sys_info->disp_mode = old_disp_mode;
			ERR_DBG("[%s] subtitle resolution change error.\n", __func__);
			goto END;
		}
		p_sys_info->disp_open = 1;

		ALOGE("[%s] disp_format changed(%d -> %d), is_opened:%d\n", __func__,
			old_disp_mode, p_sys_info->disp_mode, p_sys_info->disp_open);
	}

	if(p_sys_info->disp_mode == -1){
		ERR_DBG("[%s] display mode is not initialized yet!! Waiting...\n", __func__);
		goto END;
	}

#ifdef DUMP_INPUT_TO_FILE
	dump_file_open(data_type);
	dump_file_write(pData, uiDataSize);
#endif

	pCurDGrp = (TS_PES_CAP_DATAGROUP *)stCapParm.DGroup;
	iRet = ISDBTCC_Check_DataGroupData(p_sub_ctx, pCurDGrp);
	cur_lang_index = CCPARS_Get_Lang(data_type);

	if (iRet != ISDBT_CAPTION_SUCCESS){
		ERR_DBG("[%s] ISDBTCC_Check_DataGroupData ERROR(%d)\n", __func__, iRet);
		goto END;
	}else if ((pCurDGrp->data_group_id & ISDBT_CAP_LANGUAGE_MASK) == ISDBT_CAP_MANAGE_DATA){
		if (dunit_num != 0){
			CC_PRINTF("[%s] Data Unit Number = [%d]\n", __func__, dunit_num);
		}
		ISDBTCC_Handle_CaptionManagementData(data_type, pCurDGrp);

		TotalCapNum = -1;
		TotalSuperNum = -1;
		if (data_type == 0) {
			TotalCapNum = gIsdbtCapInfo[data_type].ucNumsOfLangCode;
			CapLang1 = gIsdbtCapInfo[data_type].LanguageInfo[0].ISO_639_language_code;
			CapLang2 = gIsdbtCapInfo[data_type].LanguageInfo[1].ISO_639_language_code;
			subtitle_demux_set_subtitle_lang(TotalCapNum);
			subtitle_set_num_languages(data_type, TotalCapNum);
			subtitle_set_langcode(data_type, CapLang1, CapLang2);
		}
		if (data_type == 1) {
			TotalSuperNum = gIsdbtCapInfo[data_type].ucNumsOfLangCode;
			SuperLang1 = gIsdbtCapInfo[data_type].LanguageInfo[0].ISO_639_language_code;
			SuperLang2 = gIsdbtCapInfo[data_type].LanguageInfo[1].ISO_639_language_code;
			subtitle_demux_set_superimpose_lang(TotalSuperNum);
			subtitle_set_num_languages(data_type, TotalSuperNum);
			subtitle_set_langcode(data_type, SuperLang1, SuperLang2);
		}
	}else if((ISDBT_CAP_1ST_STATE_DATA <= (pCurDGrp->data_group_id & ISDBT_CAP_LANGUAGE_MASK))
		&& ((pCurDGrp->data_group_id & ISDBT_CAP_LANGUAGE_MASK) <= ISDBT_CAP_8TH_STATE_DATA)
	 	&& ((pCurDGrp->DState.TMD == TMD_REAL_TIME) || (pCurDGrp->DState.TMD == TMD_OFFSET_TIME))){
			ISDBTCC_Handle_CaptionStatementData(data_type, pCurDGrp);
			if((data_type == 1)&& (gIsdbtCapInfo[data_type].State.new_pts > 0)){
				// The time control mode is only apply to superimpose's playback time.
				// because the caption is program synchronous. (Ref. ARIB TR-B14)
				iPts += gIsdbtCapInfo[data_type].State.new_pts;
			}
	}else if ((pCurDGrp->data_group_id & ISDBT_CAP_LANGUAGE_MASK) != cur_lang_index){
		//ERR_DBG("[%s] type:%d, supported lang of current ES:%d (user select:%d)\n", __func__,
		//	data_type, (pCurDGrp->data_group_id & ISDBT_CAP_LANGUAGE_MASK), cur_lang_index);
	#if defined(SUBTITLE_DATA_DUMP)
		dump_data_group(pCurDGrp, dgrp_num);
	#endif
		goto END;
	}

	if((p_sub_ctx->disp_info.dtv_type == ONESEG_SBTVD_T) || (p_sub_ctx->disp_info.dtv_type == FULLSEG_SBTVD_T)){
		subtitle_display_set_skip_pts(1);
	}else{
		subtitle_display_set_skip_pts(0);
	}

	p_sub_ctx->pts = iPts;

	if(subtitle_lib_mngr_grp_changed(data_type)){
		CCPARS_Parse_Pos_Init(p_sub_ctx, 0, E_INIT_DATA_HEAD_MNG, 0);
		subtitle_display_set_cls(data_type, 1);
		subtitle_lib_mngr_grp_set(data_type, 0);
	}

	if(subtitle_lib_sts_grp_changed(data_type)){
		subtitle_display_set_cls(data_type, 1);
		subtitle_lib_sts_grp_set(data_type, 0);
	}

	p_sub_ctx->ctrl_char_num = 0;
	p_sub_ctx->disp_info.uiCalcPos = 0;
	p_sub_ctx->disp_info.uiCharDispCount = 0;
	p_sub_ctx->disp_info.uiRepeatCharCount = 1;
	p_sub_ctx->disp_info.uiLineNum = 0;
	p_sub_ctx->disp_info.uiMaxLineNum = 0;
	p_sub_ctx->disp_info.uiCharNumInLine = 0;
	p_sub_ctx->disp_info.uiMaxCharInLine = 0;
	memset(&p_sub_ctx->disp_info.bmp_info, 0x0, sizeof(ISDBT_BMP_INFO));
	if(subtitle_fb_type() == 1){
		memset(&p_sub_ctx->disp_info.multi_bmp, 0x0, sizeof(ISDBT_MULTI_BMP_INFO));
	}

	subtitle_queue_pos_remove_all(data_type);

	while (pCurDGrp != NULL)
	{
		if (dgrp_num == 0){
			break;
		}
		dgrp_num--;

		/* Check a caption management data. */
		if (pCurDGrp->DMnge.DUnit != NULL)
		{
			pCurDUnit = (TS_PES_CAP_DATAUNIT *)pCurDGrp->DMnge.DUnit;
			if((pCurDUnit->pData == NULL)||(pCurDUnit->data_unit_size <= 0))
				goto END;

			while (pCurDUnit != NULL)
			{
				pNextDUnit = (TS_PES_CAP_DATAUNIT *)pCurDUnit->ptrNextDUnit;

			#if defined(SUBTITLE_DATA_DUMP)
				  dump_data_unit("MGR", pCurDUnit, iDataUnitSize);
			#endif /* SUBTITLE_DATA_DUMP */

				iDataUnitSize = (int)pCurDUnit->data_unit_size;
				if (pCurDUnit->data_unit_parameter == ISDBT_DUNIT_STATEMENT_BODY)
				{
					pCapDataTmp = pCurDUnit->pData;
					for (j = 0; j < iDataUnitSize; j++)
					{
						pCurPos = CCPARS_Control_Process(p_sub_ctx, 1, pSubBitPars, pCapDataTmp, &stCharInfo);
						if (NULL == pCurPos){
							ERR_DBG("[%s] Control Code Handling Error!\n", __func__);
							goto END;
						}
						if ((unsigned int)(pCurPos) <= (unsigned int)(pCapDataTmp)){
							ERR_DBG("[%s] Invalid Pointer After Statement Data Unit Parsing!\n", __func__);
							goto END;
						}
						j += pCurPos - pCapDataTmp - 1;
						if (j >= iDataUnitSize){
							ERR_DBG("[%s] Parsing Data Size(%d) exceeds Data Unit Size(%d)!\n", __func__, j, iDataUnitSize);
							goto END;
						}
						pCapDataTmp = pCurPos;
					}
				}else if((pCurDUnit->data_unit_parameter == ISDBT_DUNIT_1BYTE_DRCS) || (pCurDUnit->data_unit_parameter == ISDBT_DUNIT_2BYTE_DRCS)){
					pCurPos = CCPARS_Control_DRCS(pSubBitPars, pCurDUnit->pData, iDataUnitSize, pCurDUnit->data_unit_parameter);
					if (pCurPos == NULL){
						LOGE("[%s] Failed to control The DRCS Data(%d)!\n", __func__, pCurDUnit->data_unit_parameter);
					}
				}else {
					LOGE("caption management data is 0x%02X\n", pCurDUnit->data_unit_parameter);
				}
				pCurDUnit = pNextDUnit;
			}
		}
		/* Check a caption statement data */
		else if (pCurDGrp->DState.DUnit != NULL)
		{
			if((p_sub_ctx->disp_info.dtv_type == ONESEG_SBTVD_T) || (p_sub_ctx->disp_info.dtv_type == FULLSEG_SBTVD_T)){
				subtitle_demux_set_statement_in();
			}

			CCPARS_Parse_Pos_Init(p_sub_ctx, 0, E_INIT_DATA_HEAD_STATE, 0);

			if((p_sub_ctx->disp_info.dtv_type == FULLSEG_ISDB_T) || (p_sub_ctx->disp_info.dtv_type == FULLSEG_SBTVD_T)){
				need_update = subtitle_lib_comp_grp_type(data_type);
				if(need_update == 0) goto END;
			}

			pCurDUnit = (TS_PES_CAP_DATAUNIT *)pCurDGrp->DState.DUnit;

			while (pCurDUnit != NULL)
			{
				pNextDUnit = (TS_PES_CAP_DATAUNIT *)pCurDUnit->ptrNextDUnit;
				iDataUnitSize = (int)pCurDUnit->data_unit_size;

			#if defined(SUBTITLE_DATA_DUMP)
				dump_data_unit("STATE", pCurDUnit, iDataUnitSize);
			#endif /* SUBTITLE_DATA_DUMP */

				if (pCurDUnit->data_unit_parameter == ISDBT_DUNIT_STATEMENT_BODY)
				{
					g_statement_in = 1;
					pCapDataTmp = pCurDUnit->pData;
					for (j = 0; j < iDataUnitSize; j++)
					{
						pCurPos = CCPARS_Control_Process(p_sub_ctx, 0, pSubBitPars, pCapDataTmp, &stCharInfo);
						control_cnt++;
						if (NULL == pCurPos){
							ERR_DBG("[%s] Control Code Handling Error!\n", __func__);
							goto END;
						}
						if ((unsigned int)(pCurPos) <= (unsigned int)(pCapDataTmp)){
							ERR_DBG("[%s] Invalid Pointer After Statement Data Unit Parsing!\n", __func__);
							goto END;
						}
						j += pCurPos - pCapDataTmp - 1;
						if (j >= iDataUnitSize){
							ERR_DBG("[%s] Parsing Data Size(%d) exceeds Data Unit Size(%d)!\n", __func__, j, iDataUnitSize);
							goto END;
						}
						pCapDataTmp = pCurPos;

#if 1 //IS020A-226 1SEG SUBTITLE SIZE(704*132 -> 960X540)
						if(p_sub_ctx->disp_info.dtv_type == ONESEG_ISDB_T){
							p_sub_ctx->disp_info.origin_pos_x = 128;
							p_sub_ctx->disp_info.origin_pos_y = 408;
						}
#endif
						if (stCharInfo.CharSet == CS_KANJI){
							ISDBTCC_Handle_KanjiCharSet(p_sub_ctx, (unsigned char *)stCharInfo.CharCode);
						}else if (stCharInfo.CharSet == CS_ALPHANUMERIC){
							ISDBTCC_Handle_AlphanumericCharSet(p_sub_ctx, (unsigned char *)stCharInfo.CharCode);
						}else if (stCharInfo.CharSet == CS_LATIN_EXTENSION){
							ISDBTCC_Handle_LatinExtensionCharSet(p_sub_ctx, (unsigned char *)stCharInfo.CharCode);
						}else if (stCharInfo.CharSet == CS_SPECIAL){
							ISDBTCC_Handle_SpecialCharSet(p_sub_ctx, (unsigned char *)stCharInfo.CharCode);
						}else if (stCharInfo.CharSet == CS_HIRAGANA){
							ISDBTCC_Handle_HiraganaCharSet(p_sub_ctx, (unsigned char *)stCharInfo.CharCode);
						}else if (stCharInfo.CharSet == CS_KATAKANA){
							ISDBTCC_Handle_KataganaCharSet(p_sub_ctx, (unsigned char *)stCharInfo.CharCode);
						}else if ((stCharInfo.CharSet >= CS_DRCS_0) && (stCharInfo.CharSet <= CS_DRCS_15)){
							ISDBTCC_Handle_DRCSCharSet(p_sub_ctx, (unsigned char *)stCharInfo.CharCode, stCharInfo.CharSet);
						}else if(stCharInfo.CharSet == CS_MACRO){
							CCPARS_Check_SS(data_type);
							CCPARS_Macro_Process(p_sub_ctx, (unsigned char *)stCharInfo.CharCode);
						}else if (stCharInfo.CharSet != CS_NONE){
							LOGE("[%s] Unknown Character Set Type:%d\n", __func__, stCharInfo.CharSet);
						}

						if(stCharInfo.CharSet != CS_NONE){
							CCPARS_Check_SS(data_type);
						}
					}
				}else if((pCurDUnit->data_unit_parameter == ISDBT_DUNIT_1BYTE_DRCS) ||
													(pCurDUnit->data_unit_parameter == ISDBT_DUNIT_2BYTE_DRCS)){
					pCurPos = CCPARS_Control_DRCS(pSubBitPars, pCurDUnit->pData, iDataUnitSize, pCurDUnit->data_unit_parameter);
					if (pCurPos == NULL){
						LOGE("[%s] Failed to control The DRCS Data(%d)!\n", __func__, pCurDUnit->data_unit_parameter);
					}
				}else if(pCurDUnit->data_unit_parameter == ISDBT_DUNIT_BIT_MAP){
					if(subtitle_fb_type() == 0){
						CCPARS_Bitmap_Process(p_sub_ctx, pSubBitPars, pCurDUnit->pData, pCurDUnit->data_unit_size);
					}else{
						bmp_count++;
						CCPARS_Bitmap_Process(p_sub_ctx, pSubBitPars, pCurDUnit->pData, pCurDUnit->data_unit_size);
						p_sub_ctx->disp_info.multi_bmp.mbmp[0].element[bmp_count].handle = p_sub_ctx->disp_info.bmp_info.bmp[0].handle;
						p_sub_ctx->disp_info.multi_bmp.mbmp[0].element[bmp_count].x = p_sub_ctx->disp_info.bmp_info.bmp[0].x;
						p_sub_ctx->disp_info.multi_bmp.mbmp[0].element[bmp_count].y = p_sub_ctx->disp_info.bmp_info.bmp[0].y;

						//re-setting of bitmap display range.
						if(p_sub_ctx->disp_info.bmp_info.bmp[0].x + p_sub_ctx->disp_info.bmp_info.bmp[0].w >=  p_sub_ctx->disp_info.origin_pos_x + p_sub_ctx->disp_info.act_disp_w){
							p_sub_ctx->disp_info.multi_bmp.mbmp[0].bmp_diff_x = (p_sub_ctx->disp_info.bmp_info.bmp[0].x + p_sub_ctx->disp_info.bmp_info.bmp[0].w) - (p_sub_ctx->disp_info.origin_pos_x + p_sub_ctx->disp_info.act_disp_w);
							p_sub_ctx->disp_info.bmp_info.bmp[0].w -= p_sub_ctx->disp_info.multi_bmp.mbmp[0].bmp_diff_x;
						}
						if(p_sub_ctx->disp_info.bmp_info.bmp[0].y + p_sub_ctx->disp_info.bmp_info.bmp[0].h >=  p_sub_ctx->disp_info.origin_pos_y + p_sub_ctx->disp_info.act_disp_h){
							p_sub_ctx->disp_info.multi_bmp.mbmp[0].bmp_diff_y = (p_sub_ctx->disp_info.bmp_info.bmp[0].y + p_sub_ctx->disp_info.bmp_info.bmp[0].h) - (p_sub_ctx->disp_info.origin_pos_y + p_sub_ctx->disp_info.act_disp_h);
							p_sub_ctx->disp_info.bmp_info.bmp[0].h -= p_sub_ctx->disp_info.multi_bmp.mbmp[0].bmp_diff_y;
						}

						p_sub_ctx->disp_info.multi_bmp.mbmp[0].element[bmp_count].w = p_sub_ctx->disp_info.bmp_info.bmp[0].w;
						p_sub_ctx->disp_info.multi_bmp.mbmp[0].element[bmp_count].h = p_sub_ctx->disp_info.bmp_info.bmp[0].h;

						if(p_sub_ctx->disp_info.bmp_info.total_bmp_num == 2){
							p_sub_ctx->disp_info.multi_bmp.mbmp[1].element[bmp_count].handle = p_sub_ctx->disp_info.bmp_info.bmp[1].handle;
							p_sub_ctx->disp_info.multi_bmp.mbmp[1].element[bmp_count].x = p_sub_ctx->disp_info.bmp_info.bmp[1].x;
							p_sub_ctx->disp_info.multi_bmp.mbmp[1].element[bmp_count].y = p_sub_ctx->disp_info.bmp_info.bmp[1].y;

							//re-setting of bitmap display range.
							if(p_sub_ctx->disp_info.bmp_info.bmp[1].x + p_sub_ctx->disp_info.bmp_info.bmp[1].w >=  p_sub_ctx->disp_info.origin_pos_x + p_sub_ctx->disp_info.act_disp_w){
								p_sub_ctx->disp_info.multi_bmp.mbmp[1].bmp_diff_x = (p_sub_ctx->disp_info.bmp_info.bmp[1].x + p_sub_ctx->disp_info.bmp_info.bmp[1].w) - (p_sub_ctx->disp_info.origin_pos_x + p_sub_ctx->disp_info.act_disp_w);
								p_sub_ctx->disp_info.bmp_info.bmp[1].w -= p_sub_ctx->disp_info.multi_bmp.mbmp[1].bmp_diff_x;
							}
							if(p_sub_ctx->disp_info.bmp_info.bmp[1].y + p_sub_ctx->disp_info.bmp_info.bmp[1].h >=  p_sub_ctx->disp_info.origin_pos_y + p_sub_ctx->disp_info.act_disp_h){
								p_sub_ctx->disp_info.multi_bmp.mbmp[1].bmp_diff_y = (p_sub_ctx->disp_info.bmp_info.bmp[1].y + p_sub_ctx->disp_info.bmp_info.bmp[1].h) - (p_sub_ctx->disp_info.origin_pos_y + p_sub_ctx->disp_info.act_disp_h);
								p_sub_ctx->disp_info.bmp_info.bmp[1].h -= p_sub_ctx->disp_info.multi_bmp.mbmp[1].bmp_diff_y;
							}

							p_sub_ctx->disp_info.multi_bmp.mbmp[1].element[bmp_count].w = p_sub_ctx->disp_info.bmp_info.bmp[1].w;
							p_sub_ctx->disp_info.multi_bmp.mbmp[1].element[bmp_count].h = p_sub_ctx->disp_info.bmp_info.bmp[1].h;
						}
						p_sub_ctx->disp_info.multi_bmp.bmp_num = bmp_count;
					}
				}else{
					CC_PRINTF("[%s] Unknown Data Unit Type = 0x%02X\n", __func__, pCurDUnit->data_unit_parameter);
				}
				pCurDUnit = pNextDUnit;
			}
		}
		pCurDGrp = (TS_PES_CAP_DATAGROUP *)pCurDGrp->ptrNextDGroup;
	}

	if (p_sub_ctx->disp_handle != -1)
	{
		if(p_sub_ctx->disp_info.uiCharDispCount > 1){
			subtitle_demux_set_handle_info(1);
		}else{
			if(control_cnt == 1)
				subtitle_demux_set_handle_info(0);
		}
		#if defined(HAVE_LINUX_PLATFORM)
			subtitle_demux_linux_process( (void*)&p_sub_ctx->disp_info,
									p_sub_ctx->disp_handle,
									p_sub_ctx->pts,
									(int)p_sub_ctx->disp_info.disp_flash, 0);
		#endif
		p_sub_ctx->backup_handle = p_sub_ctx->disp_handle;
		result = 0;
		goto FIN;
	}

END:
	if(p_sub_ctx->disp_handle != -1) {
		subtitle_memory_put_handle(p_sub_ctx->disp_handle);
	}

FIN:
	if(stCapParm.DGroup) CCPARS_Deallocate_CCData(stCapParm.DGroup);
	if(pSubBitPars) BITPARS_CloseBitParser( pSubBitPars );

	return result;
}

#if defined(HAVE_LINUX_PLATFORM)
int subtitle_demux_linux_process(void *p_disp_info, int disp_handle, unsigned long long pts, int flash_flag, int time_flag)
{
	int x, y, w, h;
	int i, ret, total_count = 0;
	int flash_disp_handle = -1, tmp0_handle = -1, tmp1_handle = -1;
	int png0_handle=-1, png1_handle=1;
	unsigned int *pSrc = NULL, *pDst = NULL;
	int sub_plane_w, sub_plane_h;
	T_CHAR_DISP_INFO *param;
	int data_type;
	SUB_SYS_INFO_TYPE *pInfo = NULL;
	long long tmp_pts = 0;
	ISDBT_BMP_INFO *p_bmp_info = NULL;

	param = (T_CHAR_DISP_INFO*)p_disp_info;

	data_type = param->data_type;

	p_bmp_info = (ISDBT_BMP_INFO*)&param->bmp_info;

	x = y = w = h = -1;
	total_count = subtitle_queue_pos_getcount(data_type);

	sub_plane_w = subtitle_memory_sub_width();
	sub_plane_h = subtitle_memory_sub_height();

	if (flash_flag && total_count)
	{
		if (flash_flag && total_count)
		{
			flash_disp_handle = subtitle_memory_get_handle();
			if (flash_disp_handle != -1)
			{
				pSrc = subtitle_memory_get_vaddr(disp_handle);
				pDst = subtitle_memory_get_vaddr(flash_disp_handle);

				if (pDst == NULL)
				{
					subtitle_memory_put_handle(flash_disp_handle);
					flash_disp_handle = -1;
				}
				else
				{
					sub_plane_w = subtitle_memory_sub_width();
					sub_plane_h = subtitle_memory_sub_height();

					if ((sub_plane_w != 0) && (sub_plane_h != 0))
					{
						memcpy(pDst, pSrc, sub_plane_w * sub_plane_h * sizeof(unsigned int));

						for (i = 0; i < total_count; i++)
						{
							ret = subtitle_queue_pos_peek_nth(data_type, i, &x, &y, &w, &h);
							if (ret == 0)
							{
								ISDBTCap_DisplayClear(param, pDst, x, y, w, h);
							}
						}
					}
					else
					{
						subtitle_memory_put_handle(flash_disp_handle);
						flash_disp_handle = -1;
					}
				}
			}
		}

		if (p_bmp_info->total_bmp_num)
		{
			if (p_bmp_info->total_bmp_num == 2)
			{
				png0_handle = p_bmp_info->bmp[0].handle;
				if (png0_handle != -1)
				{
					tmp0_handle = subtitle_memory_get_handle();
					if (tmp0_handle != -1)
					{
#if defined(USE_HW_MIXER)						
						subtitle_display_mixer(png0_handle, 0, 0, sub_plane_w, sub_plane_h, 
											disp_handle, 0, 0, sub_plane_w, sub_plane_h, 
											tmp0_handle,
											0);
#else
						png_data_sw_mixer(png0_handle, disp_handle, tmp0_handle, 0, 0, sub_plane_w, sub_plane_h);
#endif
					}

					subtitle_memory_put_handle(png0_handle);
				}

				png1_handle = p_bmp_info->bmp[1].handle;
				if (png1_handle != -1)
				{
					tmp1_handle = subtitle_memory_get_handle();
					if (tmp1_handle != -1)
					{
#if defined(USE_HW_MIXER)	
						subtitle_display_mixer(png1_handle, 0, 0, sub_plane_w, sub_plane_h, 
											flash_disp_handle, 0, 0, sub_plane_w, sub_plane_h, 
											tmp1_handle,
											0);
#else
						png_data_sw_mixer(png1_handle, flash_disp_handle, tmp1_handle, 0, 0, sub_plane_w, sub_plane_h);
#endif
					}

					subtitle_memory_put_handle(png1_handle);
				}
			}
			else
			{
				png0_handle = p_bmp_info->bmp[0].handle;
				if (png0_handle != -1)
				{
					tmp0_handle = subtitle_memory_get_handle();
					if (tmp0_handle != -1)
					{
#if defined(USE_HW_MIXER)	
						subtitle_display_mixer(png0_handle, 0, 0, sub_plane_w, sub_plane_h, 
												disp_handle, 0, 0, sub_plane_w, sub_plane_h, 
												tmp0_handle,
												0);
#else
						png_data_sw_mixer(png0_handle, disp_handle, tmp0_handle, 0, 0, sub_plane_w, sub_plane_h);
#endif
						subtitle_memory_put_handle(disp_handle);					
						disp_handle = tmp0_handle;						
					}
				}

				png0_handle = p_bmp_info->bmp[0].handle;
				if (png0_handle != -1)
				{
					tmp1_handle = subtitle_memory_get_handle();
					if (tmp1_handle != -1)
					{
#if defined(USE_HW_MIXER)	
						subtitle_display_mixer(png0_handle, 0, 0, sub_plane_w, sub_plane_h, 
												flash_disp_handle, 0, 0, sub_plane_w, sub_plane_h, 
												tmp1_handle,
												0);
#else						
						png_data_sw_mixer(png0_handle, flash_disp_handle, tmp1_handle, 0, 0, sub_plane_w, sub_plane_h);
#endif
						subtitle_memory_put_handle(flash_disp_handle);					
						disp_handle = tmp0_handle;
					}
				}
			}

			subtitle_memory_put_handle(disp_handle);
			subtitle_memory_put_handle(flash_disp_handle);

			if ((tmp0_handle != -1) && (tmp1_handle != -1))
			{
				subtitle_queue_put(data_type, tmp0_handle, x, y, w, h, pts, tmp1_handle);
			}
			else
			{
				if (tmp0_handle != -1)
				{
					subtitle_memory_put_handle(tmp0_handle);
				}

				if (tmp1_handle != -1)
				{
					subtitle_memory_put_handle(tmp1_handle);
				}
			}
		}
		else
		{
			subtitle_queue_put(data_type, disp_handle, x, y, w, h, pts, flash_disp_handle);
		}
	}
	else
	{
		if (p_bmp_info->total_bmp_num)
		{
			if (p_bmp_info->total_bmp_num == 2)
			{
				png0_handle = p_bmp_info->bmp[0].handle;
				if (png0_handle != -1)
				{
					tmp0_handle = subtitle_memory_get_handle();
					if (tmp0_handle != -1)
					{
#if defined(USE_HW_MIXER)	
						subtitle_display_mixer(png0_handle, 0, 0, sub_plane_w, sub_plane_h, 
											disp_handle, 0, 0, sub_plane_w, sub_plane_h, 
											tmp0_handle,
											0);
#else
						png_data_sw_mixer(png0_handle, disp_handle, tmp0_handle, 0, 0, sub_plane_w, sub_plane_h);
#endif
					}

					subtitle_memory_put_handle(png0_handle);
				}

				png1_handle = p_bmp_info->bmp[1].handle;
				if (png1_handle != -1)
				{
					tmp1_handle = subtitle_memory_get_handle();
					if (tmp1_handle != -1)
					{
#if defined(USE_HW_MIXER)	
						subtitle_display_mixer(png1_handle, 0, 0, sub_plane_w, sub_plane_h, 
											disp_handle, 0, 0, sub_plane_w, sub_plane_h, 
											tmp1_handle,
											0);
#else						
						png_data_sw_mixer(png1_handle, disp_handle, tmp1_handle, 0, 0, sub_plane_w, sub_plane_h);
#endif
					}

					subtitle_memory_put_handle(png1_handle);
				}

				subtitle_memory_put_handle(disp_handle);

				if ((tmp0_handle != -1) && (tmp1_handle != -1))
				{
					subtitle_queue_put(data_type, tmp0_handle, x, y, w, h, pts, tmp1_handle);
				}
				else
				{
					if (tmp0_handle != -1)
					{
						subtitle_memory_put_handle(tmp0_handle);
					}

					if (tmp1_handle != -1)
					{
						subtitle_memory_put_handle(tmp1_handle);
					}
				}
			}
			else
			{
				png0_handle = p_bmp_info->bmp[0].handle;
				if (png0_handle != -1)
				{
					tmp0_handle = subtitle_memory_get_handle();
					if (tmp0_handle != -1)
					{
#if defined(USE_HW_MIXER)	
						subtitle_display_mixer(png0_handle, 0, 0, sub_plane_w, sub_plane_h, 
												disp_handle, 0, 0, sub_plane_w, sub_plane_h, 
												tmp0_handle,
												0);
#else						
						png_data_sw_mixer(png0_handle, disp_handle, tmp0_handle, 0, 0, sub_plane_w, sub_plane_h);
#endif
						subtitle_memory_put_handle(disp_handle);
						disp_handle = tmp0_handle;
					}

					subtitle_memory_put_handle(png0_handle);
				}

				subtitle_queue_put(data_type, disp_handle, x, y, w, h, pts, -1);
			}
		}
		else
		{
			subtitle_queue_put(data_type, disp_handle, x, y, w, h, pts, -1);
		}
	}

	return 0;
}
#endif
