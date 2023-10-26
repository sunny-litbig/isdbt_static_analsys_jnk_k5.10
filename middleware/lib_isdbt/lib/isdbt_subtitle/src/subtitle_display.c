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
#define LOG_TAG	"subtitle_display"
#include <utils/Log.h>

#define DISP_DBG	//ALOGE
#define FLUSHING_TIME 500
//use for all mixer (Even if the subtitle only or superimpose only, will use the wmixer.)
#define SUBTITLE_WMIXER_ALL_USE

#else
#define DISP_DBG
#endif

#ifndef uint32_t
typedef unsigned int uint32_t;
#endif

#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <cutils/properties.h>
#include <tcc_isdbt_event.h>
#include <tcc_isdbt_manager_demux.h>
#include <tcc_dxb_thread.h>
#include <TsParser_Subtitle.h>
#include <ISDBT_Common.h>
#include <subtitle_display.h>
#include <subtitle_queue.h>
#include <subtitle_system.h>
#include <subtitle_memory.h>
#include <subtitle_draw.h>
#include <subtitle_debug.h>
#include <subtitle_main.h>
#include "tcc_font.h"
#include <tcc_dxb_semaphore.h>
#include <sys/time.h>

/**********************************************************************************
For backtrace
***********************************************************************************/
//#include <stdio.h>
//#include <execinfo.h>

/**********************************************************************************
CCFB DEFINITIONS
***********************************************************************************/
#if defined(SUBTITLE_DEMUX_DUMP_INPUT_TO_FILE)
#include <tcc_ccfb_ioctl.h>
#endif
#ifdef HAVE_LINUX_PLATFORM
#include <tcc_wmixer_ioctrl.h>
#endif
#define DEV_CCFB "/dev/ccfb"
#define DEV_WMIXER	"/dev/wmixer1"

static int g_ccfb_fd = -1;
static int g_wmixer_fd = -1;
static SUB_OP_TYPE	g_sub_op_ftn;
//int subtitle_display_get_state_ccfb(void);

/****************************************************************************
DEFINITION
****************************************************************************/
//#define SUBTITLE_DEMUX_DUMP_INPUT_TO_FILE
#if defined(SUBTITLE_DEMUX_DUMP_INPUT_TO_FILE)
#include <string.h>
#include <sys/mman.h>

static FILE *gp_file_handle = NULL;
static FILE *gp_file_handle2 = NULL;
void subtitle_demux_dump_file_open()
{
	static unsigned int index = 0;
	char szName[1024] = {0,};

	if(gp_file_handle == NULL){
		sprintf(szName, "/opt/data/subtitle_src0_dump_%03d.raw", index++);
		gp_file_handle = fopen(szName, "wb");
		if(gp_file_handle == NULL){
			printf("===>> subtitle dump file open error %d\n", gp_file_handle);
		}else{
			printf("===>> subtitle dump file open success(%s)\n", szName);
		}
	}
}

int subtitle_demux_dump_file_write(int *data, int size)
{
	if(gp_file_handle != NULL){
		printf("===>> write start size [%d]\n", size);
		fwrite(data, 1, size, gp_file_handle);
		printf("===>> write end size [%d]\n", size);
		fclose(gp_file_handle);
		gp_file_handle = NULL;
		return 0;
	}

	if(gp_file_handle == NULL){
		sync();
	}
}

void subtitle_demux_dump_file_open2()
{
	static unsigned int index = 0;
	char szName[1024] = {0,};

	if(gp_file_handle2 == NULL){
		sprintf(szName, "/opt/data/subtitle_src1_dump_%03d.raw", index++);
		gp_file_handle2 = fopen(szName, "wb");
		if(gp_file_handle2 == NULL){
			printf("===>> subtitle dump file open error %d\n", gp_file_handle2);
		}else{
			printf("===>> subtitle dump file open success(%s)\n", szName);
		}
	}
}

int subtitle_demux_dump_file_write2(int *data, int size)
{
	if(gp_file_handle2 != NULL){
		printf("===>> write start size [%d]\n", size);
		fwrite(data, 1, size, gp_file_handle2);
		printf("===>> write end size [%d]\n", size);
		fclose(gp_file_handle2);
		gp_file_handle2 = NULL;
		return 0;
	}

	if(gp_file_handle2 == NULL){
		sync();
	}
}
#endif /* DUMP_INPUT_TO_FILE */


/****************************************************************************
DEFINITION OF EXTERNAL VARIABLES
****************************************************************************/
extern SUB_SYS_INFO_TYPE* get_sub_context(void);
extern void subtitle_get_internal_sound(int index);
extern void subtitle_get_data_type(int type);
extern int subtitle_set_data_type(int type);

/****************************************************************************
DEFINITION OF EXTERNAL FUNCTION
****************************************************************************/


/****************************************************************************
DEFINITION OF GLOBAL VARIABLES
****************************************************************************/
static unsigned int g_SubtitleVisible = 0;
static unsigned int g_SuperimposeVisible = 0;
static unsigned int	g_PngVisible = 0;
static unsigned int	g_Caption_AutoDisplay = 0;
static unsigned int	g_Superimpose_AutoDisplay = 0;
static unsigned int g_DMF_Flag = 0, g_st_DMF_Flag = 0, g_si_DMF_Flag = 0;
static unsigned int g_st_changed_onoff = 0, g_si_changed_onoff = 0;
static unsigned int g_force_clear = 0;


static pthread_t g_thread;
static pthread_mutex_t g_thread_run_mutex;
static pthread_mutex_t g_subtitle_mutex;
static pthread_mutex_t g_superimpose_mutex;
static pthread_mutex_t g_png_mutex;
static pthread_mutex_t g_bitmap_mutex;
static pthread_mutex_t g_mixer_mutex;
static pthread_mutex_t g_caption_enable_mutex;
static pthread_mutex_t g_super_enable_mutex;


static int g_run_thread = 0, g_memory_sync_thread = 0;
static int g_cls_flag[2] = {0, 0};

static int g_skip_pts_check = 0;
static int internal_sound_index[2] = {-1,-1};
static int caption_type, dtv_type;
static int g_st_handle = -1, g_si_handle = -1;
static int g_st_clear = 0, g_si_clear = 0, g_all_clear = 0;
static int update_sync_drawing = -1;

static tcc_dxb_sem_t gSubtitleWait;
/****************************************************************************
DEFINITION OF LOCAL FUNCTIONS
****************************************************************************/
int subtitle_display_set_thread_flag(int set)
{
	g_run_thread = set;
	return 0;
}

int subtitle_display_get_thread_flag(void)
{
	return g_run_thread;
}

int subtitle_display_set_memory_sync_flag(int set)
{
//	LOGE("[%s][%d:%d]\n", __func__, set, g_memory_sync_thread);
	//When the creation of memory, set to 1.
	g_memory_sync_thread = set;
	return 0;
}

int subtitle_display_get_memory_sync_flag(void)
{
	return g_memory_sync_thread;
}

int subtitle_display_get_cls(int type)
{
	if((type<0) || (type >1)){
		LOGE("[%s] Invalid type(%d)\n", __func__, type);
		return 0;
	}

	return g_cls_flag[type];
}

void subtitle_display_set_cls(int type, int set)
{
	if((type<-1) || (type >1)){
		LOGE("[%s] Invalid type(%d)\n", __func__, type);
		return;
	}

	if(type == -1){
		g_cls_flag[0] = set;
		g_cls_flag[1] = set;
	} else {
		g_cls_flag[type] = set;
	}
}

void subtitle_display_set_skip_pts(int skip)
{
	g_skip_pts_check = skip;
}

int subtitle_display_get_skip_pts(void)
{
	return g_skip_pts_check;
}

static void subtitle_display_set_param_1seg_japan(void *p, T_CHAR_DISP_INFO **pp_disp_info)
{
	double ratio_w=1.0f, ratio_h=1.0f, zoom = 1.0f;
	T_CHAR_DISP_INFO *param = (T_CHAR_DISP_INFO*)*pp_disp_info;

	param->origin_pos_x			= 0;
	param->origin_pos_y 		= 0;
	param->act_pos_x 			= 0;
	param->act_pos_y 			= 0;

	param->def_font_w			= (42*zoom*ratio_w);
	param->def_font_h			= (42*zoom*ratio_h);
	param->act_font_w			= param->def_font_w;
	param->act_font_h			= param->def_font_h;

	param->def_hs				= (2*zoom*ratio_w);
	param->def_vs				= (2*zoom*ratio_h);
	param->act_hs 				= param->def_hs;
	param->act_vs				= param->def_vs;

	//IS020A-226 1SEG SUBTITLE SIZE(704*132 -> 960X540)
	param->sub_plane_w 			= 960;//(704*zoom*ratio_w);
	param->sub_plane_h 			= 540;//(132*zoom*ratio_h);
	param->act_disp_w			= param->sub_plane_w;
	param->act_disp_h			= param->sub_plane_h;

	param->mngr_font_w			= 0;
	param->mngr_font_h			= 0;
	param->mngr_vs				= -1;
	param->mngr_hs				= -1;

	param->uiLineNum 			= 0;
	param->uiCharNumInLine 		= 0;
	param->uiFontType 			= E_FONT_NORMAL;
	param->uiFontEffect 		= E_FONT_EFFECT_NONE;
	param->uiPaletteNumber 		= 0;

	param->uiForeColor 			= g_CMLA_Table[7];
	param->uiBackColor 			= g_CMLA_Table[0];
	param->uiHalfForeColor		= g_CMLA_Table[7];
	param->uiHalfBackColor		= g_CMLA_Table[0];
	param->uiClearColor			= g_CMLA_Table[65];
	param->uiRasterColor		= g_CMLA_Table[8];

	param->uiRepeatCharCount 	= 1;
	param->uiCharDispCount 		= 0;
	param->usDispMode 			= CHAR_DISP_MASK_HWRITE;
	param->disp_flash 			= 0;
	param->nonspace_char		= 0;

	param->ratio_drcs_x			= (double)((double)(param->act_font_w) / 16.0f);
	param->ratio_drcs_y			= (double)((double)(param->act_font_h) / 18.0f);
	param->internal_sound		= -1;
}

static void subtitle_display_set_param_1seg_brazil(void *p, T_CHAR_DISP_INFO **pp_disp_info)
{
	double ratio_w=1.0f, ratio_h=1.0f, zoom = 1.0f;
	T_CHAR_DISP_INFO *param = (T_CHAR_DISP_INFO*)*pp_disp_info;

	param->origin_pos_x			= 0;
	param->origin_pos_y 		= 0;
	param->act_pos_x 			= 0;
	param->act_pos_y 			= 0;

	param->def_font_w			= (42*zoom*ratio_w);
	param->def_font_h			= (42*zoom*ratio_h);
	param->act_font_w			= param->def_font_w;
	param->act_font_h			= param->def_font_h;

	param->def_hs				= (2*zoom*ratio_w);
	param->def_vs				= (2*zoom*ratio_h);
	param->act_hs 				= param->def_hs;
	param->act_vs				= param->def_vs;

	param->sub_plane_w 			= (704*zoom*ratio_w);
	param->sub_plane_h 			= (132*zoom*ratio_h);
	param->act_disp_w			= param->sub_plane_w;
	param->act_disp_h			= param->sub_plane_h;

	param->mngr_font_w			= 0;
	param->mngr_font_h			= 0;
	param->mngr_vs				= -1;
	param->mngr_hs				= -1;

	param->uiLineNum 			= 0;
	param->uiCharNumInLine 		= 0;
	param->uiFontType 			= E_FONT_MIDDLE;
	param->uiFontEffect 		= E_FONT_EFFECT_NONE;
	param->uiPaletteNumber 		= 0;

	param->uiForeColor 			= g_CMLA_Table[7];
	param->uiBackColor 			= g_CMLA_Table[0];
	param->uiHalfForeColor		= g_CMLA_Table[7];
	param->uiHalfBackColor		= g_CMLA_Table[0];
	param->uiClearColor			= g_CMLA_Table[65];
	param->uiRasterColor		= g_CMLA_Table[8];

	param->uiRepeatCharCount 	= 1;
	param->uiCharDispCount 		= 0;
	param->usDispMode 			= CHAR_DISP_MASK_HWRITE|CHAR_DISP_MASK_HALF_WIDTH;
	param->disp_flash 			= 0;
	param->nonspace_char		= 0;

	param->ratio_drcs_x			= (double)((double)(param->act_font_w) / 16.0f);
	param->ratio_drcs_y			= (double)((double)(param->act_font_h) / 18.0f);
	param->internal_sound		= -1;
}

static void subtitle_display_set_param_13seg_japan(void *p, T_CHAR_DISP_INFO **pp_disp_info)
{
	T_CHAR_DISP_INFO *param = (T_CHAR_DISP_INFO*)*pp_disp_info;

	param->sub_plane_w 			= 960;
	param->sub_plane_h 			= 540;
	param->act_disp_w			= param->sub_plane_w;
	param->act_disp_h			= param->sub_plane_h;

	param->origin_pos_x			= 0;
	param->origin_pos_y 		= 0;
	param->act_pos_x 			= 0;
	param->act_pos_y 			= 0;

	param->def_font_w			= 36;
	param->def_font_h			= 36;
	param->act_font_w			= param->def_font_w;
	param->act_font_h			= param->def_font_h;

	param->def_hs				= 4;
	param->def_vs				= 24;
	param->act_hs 				= param->def_hs;
	param->act_vs				= param->def_vs;

	param->mngr_font_w			= 0;
	param->mngr_font_h			= 0;
	param->mngr_vs				= -1;
	param->mngr_hs				= -1;

	param->uiLineNum 			= 0;
	param->uiCharNumInLine 		= 0;
	param->uiFontType 			= E_FONT_MIDDLE;
	param->uiFontEffect 		= E_FONT_EFFECT_NONE;
	param->uiPaletteNumber 		= 0;

	param->uiForeColor 			= g_CMLA_Table[7];
	param->uiBackColor 			= g_CMLA_Table[8];
	param->uiHalfForeColor		= g_CMLA_Table[7];
	param->uiHalfBackColor		= g_CMLA_Table[8];
	param->uiClearColor			= g_CMLA_Table[8];
	param->uiRasterColor		= g_CMLA_Table[8];

	param->uiRepeatCharCount 	= 1;
	param->uiCharDispCount 		= 0;
	param->usDispMode 			= CHAR_DISP_MASK_HWRITE;
	param->disp_flash 			= 0;
	param->nonspace_char		= 0;

	param->ratio_drcs_x			= 1.0f;
	param->ratio_drcs_y			= 1.0f;
	param->internal_sound		= -1;
}

static void subtitle_display_set_param_13seg_brazil(void *p, T_CHAR_DISP_INFO **pp_disp_info)
{
	T_CHAR_DISP_INFO *param = (T_CHAR_DISP_INFO*)*pp_disp_info;

	param->sub_plane_w 			= 960;
	param->sub_plane_h 			= 540;
	param->act_disp_w			= param->sub_plane_w;
	param->act_disp_h			= param->sub_plane_h;

	param->origin_pos_x			= 0;
	param->origin_pos_y 		= 0;
	param->act_pos_x 			= 0;
	param->act_pos_y 			= 0;

	param->def_font_w			= 36;
	param->def_font_h			= 36;
	param->act_font_w			= param->def_font_w;
	param->act_font_h			= param->def_font_h;

	param->def_hs				= 4;
	param->def_vs				= 24;
	param->act_hs 				= param->def_hs;
	param->act_vs				= param->def_vs;

	param->mngr_font_w			= 0;
	param->mngr_font_h			= 0;
	param->mngr_vs				= -1;
	param->mngr_hs				= -1;

	param->uiLineNum 			= 0;
	param->uiCharNumInLine 		= 0;
	param->uiFontType 			= E_FONT_MIDDLE;
	param->uiFontEffect 		= E_FONT_EFFECT_NONE;
	param->uiPaletteNumber 		= 0;

	param->uiForeColor 			= g_CMLA_Table[7];
	param->uiBackColor 			= g_CMLA_Table[8];
	param->uiHalfForeColor		= g_CMLA_Table[7];
	param->uiHalfBackColor		= g_CMLA_Table[8];
	param->uiClearColor			= g_CMLA_Table[8];
	param->uiRasterColor		= g_CMLA_Table[8];

	param->uiRepeatCharCount 	= 1;
	param->uiCharDispCount 		= 0;
	param->usDispMode 			= CHAR_DISP_MASK_HWRITE|CHAR_DISP_MASK_HALF_WIDTH;
	param->disp_flash 			= 0;
	param->nonspace_char		= 0;

	param->ratio_drcs_x			= 1.0f;
	param->ratio_drcs_y			= 1.0f;
	param->internal_sound		= -1;
}

int subtitle_display_pre_init(SUB_ARCH_TYPE arch_type, int init)
{
	ALOGE("In [%s] init[%d]\n", __func__, init);
	int ret = -1;

#if defined (HAVE_LINUX_PLATFORM)
	if(init == 1){
		g_sub_op_ftn.pInit = subtitle_display_init_ccfb;
		g_sub_op_ftn.pOpen = subtitle_display_open_ccfb;
		g_sub_op_ftn.pClose = subtitle_display_close_ccfb;
		g_sub_op_ftn.pClear = subtitle_display_clear_ccfb;
		ret = (g_sub_op_ftn.pInit)((int)arch_type);
		if(ret == -1) return -1;
	}else{
		g_sub_op_ftn.pOpen = subtitle_display_open_ccfb;
		g_sub_op_ftn.pClear = subtitle_display_clear_ccfb;
	}

	#if defined(SUBTITLE_CCFB_DISPLAY_ENABLE)
	g_sub_op_ftn.pUpdate = subtitle_display_update_ccfb;
	g_sub_op_ftn.pEnable = subtitle_display_enable_ccfb;
	#endif

	pthread_mutex_init (&g_mixer_mutex, NULL);
	pthread_mutex_init (&g_caption_enable_mutex, NULL);
	pthread_mutex_init (&g_super_enable_mutex, NULL);
	pthread_mutex_init (&g_bitmap_mutex, NULL);
#endif

	tcc_font_init();
	//tcc_font_setDirection(TCC_FONT_DIRECTION_HORIZONTAL, TCC_FONT_BASE_POSITION_TOP_LEFT);
	tcc_font_setDirection(TCC_FONT_DIRECTION_HORIZONTAL, TCC_FONT_BASE_POSITION_BOTTOM);

	tcc_dxb_sem_init(&gSubtitleWait, 0);

	ALOGE("Out [%s]\n", __func__);
	return 0;
}

int subtitle_display_init(int data_type, void *p)
{
//	ALOGE("In [%s]\n", __func__);
	SUBTITLE_CONTEXT_TYPE *p_sub_ctx = (SUBTITLE_CONTEXT_TYPE*)p;
	T_CHAR_DISP_INFO *p_disp_info = NULL;
	SUB_SYS_INFO_TYPE *p_sys_info = NULL;

	if(p_sub_ctx == NULL){
		LOGE("[%s] NULL pointer exception\n", __func__);
		return -1;
	}

	p_sys_info = (SUB_SYS_INFO_TYPE*)&p_sub_ctx->sys_info;
	p_disp_info = (T_CHAR_DISP_INFO*)&p_sub_ctx->sub_ctx[data_type].disp_info;

	p_disp_info->data_type = data_type;
	p_disp_info->dtv_type = p_sys_info->isdbt_type;
	dtv_type = p_disp_info->dtv_type;
	if(p_disp_info->dtv_type == ONESEG_ISDB_T){
		subtitle_display_set_param_1seg_japan(p_sys_info, &p_disp_info);
	}else if(p_disp_info->dtv_type == ONESEG_SBTVD_T){
		subtitle_display_set_param_1seg_brazil(p_sys_info, &p_disp_info);
	}else if(p_disp_info->dtv_type == FULLSEG_ISDB_T){
		subtitle_display_set_param_13seg_japan(p_sys_info, &p_disp_info);
	}else if(p_disp_info->dtv_type == FULLSEG_SBTVD_T){
		subtitle_display_set_param_13seg_brazil(p_sys_info, &p_disp_info);
	}else{
		LOGE("[%s] unknown dtv type(%d)\n", __func__, p_disp_info->dtv_type);
		return -1;
	}

//	ALOGE("Out [%s]\n", __func__);
	return 0;
}

int subtitle_display_exit(void)
{
//	ALOGE("In [%s]\n", __func__);
	tcc_font_close();

#if 1    // Noah, To avoid CodeSonar warning, CodeSonar thinks subtitle_fb_type() == 0 always evaluates to true.

	/* 0:CCFB, 1:SURFACE */
	#if defined(HAVE_LINUX_PLATFORM)
		pthread_mutex_destroy (&g_mixer_mutex);
		pthread_mutex_destroy (&g_caption_enable_mutex);
		pthread_mutex_destroy (&g_super_enable_mutex);
		pthread_mutex_destroy (&g_bitmap_mutex);
	#endif
	tcc_dxb_sem_deinit(&gSubtitleWait);

#else    // Following codes are original.

	if(subtitle_fb_type() == 0){
		pthread_mutex_destroy (&g_mixer_mutex);
		pthread_mutex_destroy (&g_caption_enable_mutex);
		pthread_mutex_destroy (&g_super_enable_mutex);
	}else{
		pthread_mutex_destroy (&g_subtitle_mutex);
		pthread_mutex_destroy (&g_superimpose_mutex);
		pthread_mutex_destroy (&g_png_mutex);
		pthread_mutex_destroy (&g_bitmap_mutex);
	}
	tcc_dxb_sem_deinit(&gSubtitleWait);

#endif

//	ALOGE("Out [%s]\n", __func__);
	return 0;
}
/****************************************************************************/
static int subtitle_display_set_info(DISP_CTX_TYPE *p, int handle, int flash_handle, int x, int y, int w, int h, unsigned long long pts)
{
	if(p == NULL){
		LOGE("[%s] NULL pointer exception\n", __func__);
		return -1;
	}

	p->handle = handle;
	p->flash_handle = flash_handle;
	p->x = x;
	p->y = y;
	p->w = w;
	p->h = h;
	p->pts = pts;

	return 0;
}

static int subtitle_display_set_info_disp(PNG_DISP_CTX_TYPE *p, int handle, unsigned long long pts, int png_flag)
{
	if(p == NULL){
		LOGE("[%s] NULL pointer exception\n", __func__);
		return -1;
	}

	p->handle = handle;
	p->pts = pts;
	p->png_flag = png_flag;

	return 0;
}

static void subtitle_display_init_disp_ctx(DISP_CTX_TYPE *p)
{
	if(p == NULL){
		LOGE("[%s] NULL pointer exception\n", __func__);
		return;
	}

	p->handle = -1;
	p->flash_handle = -1;
	p->x = 0;
	p->y = 0;
	p->w = 0;
	p->h = 0;
	p->pts = 0;
}

static void subtitle_display_init_disp_ctx_png(PNG_DISP_CTX_TYPE *p)
{
	if(p == NULL){
		LOGE("[%s] NULL pointer exception\n", __func__);
		return;
	}

	p->handle = -1;
	p->pts = 0;
	p->png_flag = 0;
}

#if defined (HAVE_LINUX_PLATFORM)
/* Noah, 20210611, I added final_mix for IM856A-14([8974][8030] Subtitle & superimpose & bitmap were not appeared in rear monitor(AVOUT).)
	Parameter final_mix
		0 : Mixing is incomplete. Do not use the mixed image for rear monitor.
		1 : Mixing is complete. The mixed image can be used for rear monitor.

	Refer to the comment below.
		Mixing Process
			Case 1) Subtitle/Superimpose/Png exists all.
				- the number of times of mixing = total 2 times.
				- 1st mixing  : Superimpose[src1] + PNG[src2] --> SI(Superimpose with PNG) [dst]
				- 2nd mixing : ST[src1] + SI(Superimpose with PNG)[src2] --> ST+SI [dst]
			Case 2) Subtitle only (Superimpose and Png does not exist)
				- the number of times of mixing = total 1 time.
				- 1st mixing : ST[src1] + NULL[src2] --> ST[dst] 
			Case 3) Superimpose only (Subtitle and Png does not exist)
				- the number of times of mixing = total 1 time.
				- 1st mixing : NULL[src1] + Superimpose[src2]	--> Superimpose[dst] 
			Case 4) Subtitle/Superimpose exists (Png does not exist)
				- the number of times of mixing = total 2 times.
				- 1st mixing  : Superimpose[src1] + NULL(without PNG)[src2] --> Superimpose[dst] 
				- 2nd mixing : ST[src1] + Superimpose[src2] --> ST + Superimpose [dst] 
			Case 5) Superimpose/Png exists (Subtitle does not exist)
				- the number of times of mixing = total 2 times.
				- 1st mixing  : Superimpose[src1] + PNG[src2]	--> SI(Superimpose with PNG) [dst]
				- 2nd mixing : NULL[src1] + SI(Superimpose with PNG)[src2]  --> SI[dst]
		JVCKenwood Comment
			[Example of our problem]
				In Case1/Case4/Case5 (the number of mixing = total 2 times), our BSP driver will output an incomplete mixing image to AVOUT only for a moment.
				For example, In Case1, our BSP driver outputs an image mixed only with superimpose and PNG to AVOUT for a moment.
				In other words, it means that the subtitles that should have been displayed disappear for a moment.
			[Our BSP engineer's idea]
				He suggests that adding a new IOCTL and letting it know that the mix is complete could solve this problem.
		My Idea
			Add new parameter to WMIXER_ALPHABLENDING_TYPE struct.
*/
int subtitle_display_mixer
(
	int st_handle, int st_x, int st_y, int st_w, int st_h,
	int si_handle, int si_x, int si_y, int si_w, int si_h,
	int dst_handle,
	int final_mix
)
{
	unsigned int buf_w=0, buf_h=0, buf_size=0;
	int mix_ret = 0;

	if(st_handle == si_handle){
		LOGE("[ERROR] (%d,%d)handle conflicted or need more memory.\n", st_handle, si_handle);
		return -1;
	}

	if(st_handle < -1 || si_handle < -1){
		LOGE("[ERROR] (%d,%d) invalid handle\n", st_handle, si_handle);
		return -1;
	}

	if(subtitle_display_get_memory_sync_flag() == 0){
		return -1;
	}

	buf_w = subtitle_memory_sub_width();
	buf_h = subtitle_memory_sub_height();
	buf_size = buf_w * buf_h;
	if(buf_size == 0){
		LOGE("[%s] buffer size is zero\n", __func__);
		return -1;
	}

	if(!subtitle_display_get_enable_flag() && subtitle_get_dmf_st_flag() == 0){
		st_handle = -1;
	}

	if(!superimpose_display_get_enable_flag() && subtitle_get_dmf_si_flag() == 0){
		si_handle = -1;
	}

	DISP_DBG("[%s] st(%d) + si(%d) => mix(%d), st_rect[%d:%d:%d:%d], si_rect[%d:%d:%d:%d], buf[%d:%d], final_mix(%d)\n", __func__,
			st_handle, si_handle, dst_handle,
			st_x, st_y, st_w, st_h,
			si_x, si_y, si_w, si_h,
			buf_w, buf_h,
			final_mix);

	if(dst_handle != -1){
		mix_ret = subtitle_memory_memset(dst_handle, buf_size);
		if(mix_ret != 0){
			ALOGE("[%s] memory not prepared => memset fail\n", __func__);
			return -1;
		}
	}

#if defined(SUBTITLE_DEMUX_DUMP_INPUT_TO_FILE)
	subtitle_demux_dump_file_open();
	subtitle_demux_dump_file_write((int*)subtitle_memory_get_vaddr(st_handle), buf_size*4);
	subtitle_demux_dump_file_open2();
	subtitle_demux_dump_file_write2((int*)subtitle_memory_get_vaddr(si_handle), buf_size*4);
#endif
	int ret = -1;

	struct pollfd poll_event[1];

	pthread_mutex_lock (&g_mixer_mutex);

	int wmixer_fd = -1;
	WMIXER_ALPHABLENDING_TYPE	alpha_info;

	wmixer_fd = subtitle_display_get_wmixer_fd();
	if(wmixer_fd == -1){
		pthread_mutex_unlock (&g_mixer_mutex);
		LOGE("[%s] Invalid wmixer_fd found\n", __func__);
		return -1;
	}

	DISP_DBG("subtitle_display_mixer alpha size[%d]", sizeof(WMIXER_ALPHABLENDING_TYPE));
	memset(&alpha_info, 0x0, sizeof(WMIXER_ALPHABLENDING_TYPE));

	alpha_info.rsp_type 		= WMIXER_INTERRUPT;
	alpha_info.region 			= 2; 	/* Region C */

	alpha_info.src0_fmt 		= 12;//CCFB_IMG_FMT_ARGB8888;
	alpha_info.src0_layer		= 0;
	alpha_info.src0_acon0 		= 1;
	alpha_info.src0_acon1 		= 2;
	alpha_info.src0_ccon0 		= 0;
	alpha_info.src0_ccon1 		= 2;
	alpha_info.src0_rop_mode 	= 0x18; 	/* pixel alpha */
	alpha_info.src0_asel 		= 3; 	/* image alpha (0% ~ 99.6%) */
	alpha_info.src0_alpha0 		= 0x0;
	alpha_info.src0_alpha1 		= 0x0;
	alpha_info.src0_Yaddr 		= (unsigned int)subtitle_memory_get_paddr(st_handle);
	alpha_info.src0_Uaddr 		= (unsigned int)NULL;
	alpha_info.src0_Vaddr 		= (unsigned int)NULL;
	alpha_info.src0_width 		= buf_w;
	alpha_info.src0_height 		= buf_h;
	alpha_info.src0_winLeft 	= 0;
	alpha_info.src0_winTop 		= 0;
	alpha_info.src0_winRight 	= 0;
	alpha_info.src0_winBottom 	= 0;

	alpha_info.src1_fmt 		= 12;//CCFB_IMG_FMT_ARGB8888;
	alpha_info.src1_layer 		= 1;
	alpha_info.src1_acon0 		= 0;
	alpha_info.src1_acon1 		= 1;
	alpha_info.src1_ccon0 		= 0;
	alpha_info.src1_ccon1 		= 1;
	alpha_info.src1_rop_mode 	= 0x18;
	alpha_info.src1_asel 		= 3;
	alpha_info.src1_alpha0 		= 0x0;
	alpha_info.src1_alpha1 		= 0x0;
	alpha_info.src1_Yaddr 		= (unsigned int)subtitle_memory_get_paddr(si_handle);
	alpha_info.src1_Uaddr 		= (unsigned int)NULL;
	alpha_info.src1_Vaddr 		= (unsigned int)NULL;
	alpha_info.src1_width 		= buf_w;
	alpha_info.src1_height 		= buf_h;
	alpha_info.src1_winLeft 	= 0;
	alpha_info.src1_winTop 		= 0;
	alpha_info.src1_winRight 	= 0;
	alpha_info.src1_winBottom 	= 0;

	alpha_info.dst_fmt 			= 12;//CCFB_IMG_FMT_ARGB8888;
	alpha_info.dst_Yaddr 		= (unsigned int)subtitle_memory_get_paddr(dst_handle);
	alpha_info.dst_Uaddr 		= (unsigned int)NULL;
	alpha_info.dst_Vaddr 		= (unsigned int)NULL;
	alpha_info.dst_width 		= buf_w;
	alpha_info.dst_height 		= buf_h;
	alpha_info.dst_winLeft 		= 0;
	alpha_info.dst_winTop 		= 0;
	alpha_info.dst_winRight 	= 0;
	alpha_info.dst_winBottom 	= final_mix;

	if(subtitle_display_get_thread_flag() == 0){
		goto END;
	}

	if(update_sync_drawing == 0){
		goto END;
	}

	if(subtitle_display_get_memory_sync_flag() == 0 || alpha_info.dst_Yaddr == NULL){
		ALOGE("[%s] memory not prepared[%d] => skip mixing\n", __func__, subtitle_display_get_memory_sync_flag());
		goto END;
	}

	ret = ioctl(wmixer_fd, TCC_WMIXER_ALPHA_MIXING, &alpha_info);
	if(ret != 0){
		LOGE("[%s] MIXER ERROR(%d)\n", __func__, ret);
	}

	DISP_DBG("subtitle_display_mixer poll_event size[%d]", sizeof(poll_event));
	memset(&poll_event[0], 0, sizeof(poll_event));

	poll_event[0].fd = wmixer_fd;
	poll_event[0].events = POLLIN;

	while(1){
		ret = poll((struct pollfd*)poll_event, 1, 100 /* ms */);
		if (ret < 0){
			LOGE("[%s] mixer poll error\n", __func__);
			break;
		}else if (ret == 0){
			LOGE("[%s] mixer poll timeout\n", __func__);
			break;
		}else if (ret > 0){
			if (poll_event[0].revents & POLLERR){
				LOGE("[%s] mixer POLLERR\n", __func__);
				break;
			}else if (poll_event[0].revents & POLLIN){
				break;
			}
		}
	}
END:
	pthread_mutex_unlock (&g_mixer_mutex);
	return 0;
}
#endif

static void subtitle_display_check_flash_condition(int type, DISP_CTX_TYPE *p_cur, unsigned long long cur_pts)
{
	int ret = -1;
	DISP_CTX_TYPE next;

	ret = subtitle_queue_peek(type, &next.handle, &next.x, &next.y, &next.w, &next.h, &next.pts, &next.flash_handle);

	if(ret == -1){
		LOGE("[%s] flash> type:%d, ERROR - subtitle queue is not initialized yet!\n", __func__, type);
	}else if(ret == -2){
		//DISP_DBG("[%s] flash> type:%d, No data found in queue\n", __func__, type);
		subtitle_queue_put_first(type, p_cur->flash_handle, \
								p_cur->x, p_cur->y, p_cur->w, p_cur->h, \
								cur_pts+500, p_cur->handle);
	}else{
		/* found next queue data */
		if(next.pts > (cur_pts+500)){
			DISP_DBG("[%s] flash> In flash processing...(type:%d, cur[%d,%d])\n", \
				__func__, type, p_cur->flash_handle, p_cur->handle);

			/* next pts is smaller than current pts +500(ms), put in first place of queue */
			subtitle_queue_put_first(type, p_cur->flash_handle,
									p_cur->x, p_cur->y, p_cur->w, p_cur->h, \
									p_cur->pts+500, p_cur->handle);
		}
	}
}

static int subtitle_display_check_clear_cond(DISP_CTX_TYPE *pSt, DISP_CTX_TYPE *pSi)
{
	int cleared = 0;
	int st_clear=0, si_clear=0;

	st_clear = subtitle_display_get_cls(0);
	si_clear = subtitle_display_get_cls(1);

	if(st_clear && si_clear){
		subtitle_display_set_cls(-1, 0);
		subtitle_display_clear_screen(-1, pSt, pSi);
	}else if(st_clear){
		subtitle_display_set_cls(0, 0);
		subtitle_display_clear_screen(0, pSt, pSi);
	}else if(si_clear){
		subtitle_display_set_cls(1, 0);
		subtitle_display_clear_screen(1, pSt, pSi);
	}

	return (st_clear|si_clear);
}

#if defined (HAVE_LINUX_PLATFORM)
void subtitle_current_set_st_handle(int handle)
{
	g_st_handle = handle;
}

void subtitle_current_set_si_handle(int handle)
{
	g_si_handle = handle;
}

int subtitle_current_get_st_handle(void)
{
	return g_st_handle;
}

int subtitle_current_get_si_handle(void)
{
	return g_si_handle;
}

#if defined(USE_HW_MIXER)
void* subtitle_display_linux_thread(void *data)
{
	int st_ret, si_ret;
	int tmp_handle, cur_handle, caption_handle, super_handle, mix_handle;
	int st_check, si_check;
	int stc_index, stc_delay_enable, stc_wait_state;
	DISP_CTX_TYPE st, si, last_st, last_si, next_st, next_si;
	long long idelay, stc_gap;
	unsigned long long cur_pts, stc_delay_time, cur_stc_time, prev_stc_time;
	
	cur_handle = tmp_handle = caption_handle = super_handle = mix_handle = -1;
	cur_pts = st_check = si_check = 0;
	stc_delay_enable = stc_delay_time = stc_index = stc_wait_state = cur_stc_time = prev_stc_time = stc_gap = 0;

	subtitle_display_init_disp_ctx(&st);
	subtitle_display_init_disp_ctx(&si);
	subtitle_display_init_disp_ctx(&last_st);
	subtitle_display_init_disp_ctx(&last_si);
	subtitle_display_init_disp_ctx(&next_st);
	subtitle_display_init_disp_ctx(&next_si);
	
	while (subtitle_display_get_thread_flag() == 1)
	{
		if (subtitle_display_get_thread_flag() == 0)
		{
			break;
		}

		if (subtitle_display_get_enable_flag() == 0)
		{
			if ((super_handle == -1) && (st_check == 0))
			{
				subtitle_display_clear();
				st_check = 1;
			}
			else if ((super_handle != -1) && (st_check == 0) && (superimpose_display_get_enable_flag() == 1))
			{
				subtitle_display_update(super_handle);
				st_check = 1;
			}			
		}

		if (superimpose_display_get_enable_flag() == 0)
		{
			if ((caption_handle == -1) && (si_check == 0))
			{
				subtitle_display_clear();
				si_check = 1;
			}
			else if ((caption_handle != -1) && (si_check == 0) && (subtitle_display_get_enable_flag() == 1))
			{
				subtitle_display_update(caption_handle);
				si_check = 1;
			}
		}
		
		if (subtitle_display_get_enable_flag() == 1)
		{
			st_check = 0;
		}
		else if (superimpose_display_get_enable_flag() == 1)
		{
			si_check = 0;
		}
		
		tmp_handle = cur_handle = -1;
		st_ret = st.handle = st.flash_handle = -1;
		si_ret = si.handle = si.flash_handle = -1;
		st.pts = si.pts = 0;
		
		st_ret = subtitle_queue_get(0, &st.handle, &st.x, &st.y, &st.w, &st.h, &st.pts, &st.flash_handle);
		si_ret = subtitle_queue_get(1, &si.handle, &si.x, &si.y, &si.w, &si.h, &si.pts, &si.flash_handle);

		if ((st_ret < 0) && (si_ret < 0))
		{
			subtitle_display_check_clear_cond(&last_st, &last_si);
			usleep(1000);
		}
		else
		{
			if ((st.handle == -1) && (si.handle == -1))
			{
				LOGE("[%s] invalid handle (%d, %d)\n", __func__, st.handle, si.handle);
				continue;
			}

			DISP_DBG("[%s:%d] st[%d, %d], last_st[%d, %d], st.pts:%lld, st_rect[%d:%d:%d:%d]\n",
						__func__, __LINE__, st.handle, st.flash_handle, last_st.handle, last_st.flash_handle, st.pts, st.x, st.y, st.w, st.h);
			DISP_DBG("[%s:%d] si[%d, %d], last_si[%d, %d], si.pts:%lld, si_rect[%d:%d:%d:%d]\n",
						__func__, __LINE__, si.handle, si.flash_handle, last_si.handle, last_si.flash_handle, si.pts, si.x, si.y, si.w, si.h);

			stc_index = subtitle_system_get_stc_index();			
			if (stc_index == 0)
			{
				stc_delay_enable = subtitle_system_get_stc_delay_excute();
				if (stc_delay_enable == 1)
				{
					stc_delay_time = subtitle_system_get_stc_delay_time();
					DISP_DBG("\033[31m stc wait time(%lld) thread[%d]\033[m\n", stc_delay_time, subtitle_display_get_thread_flag());
					if ((0 < stc_delay_time) && (subtitle_display_get_thread_flag() == 1))
					{
						stc_wait_state = 1;
						tcc_dxb_sem_down_timewait_msec(&gSubtitleWait, (int)(stc_delay_time) + 352);
						if (subtitle_display_get_thread_flag() == 0)
						{
							break;
						}
						stc_wait_state = 0;
					}
				}
			}
			else
			{
				tcc_dxb_sem_signal(&gSubtitleWait);
			}

			DISP_DBG("[%s] index(%d)enable(%d)time(%lld)\n", __func__, stc_index, stc_delay_enable, stc_delay_time);

			if ((st.flash_handle != -1) && (st.handle != -1) && (si.handle != -1) && (si.flash_handle != -1))
			{
				DISP_DBG("CASE-0 : ST & SI & Flash Inputted.\n");
				
				tmp_handle = subtitle_memory_get_handle();
				if (tmp_handle == -1)
				{
					cur_handle = st.handle;
				}
				else
				{
					caption_handle = st.handle;
					super_handle = si.handle;
					subtitle_display_mixer(st.handle, st.x, st.y, st.w, st.h,
											si.handle, si.x, si.y, si.w, si.h,
											tmp_handle,
											1);
					cur_handle = tmp_handle;
					if (subtitle_display_get_enable_flag() == 0)
					{
						cur_handle = super_handle;
					}
					else if (superimpose_display_get_enable_flag() == 0)
					{
						cur_handle = caption_handle;
					}
				}

				if ((st.handle != last_st.flash_handle) && (st.flash_handle != last_st.handle))
				{
					DISP_DBG("[%s:%d] st[%d, %d], last_st[%d, %d]\n", __func__, __LINE__, st.handle, st.flash_handle, last_st.handle, last_st.flash_handle);
					
					if (last_st.handle != -1)
					{
						subtitle_memory_put_handle(last_st.handle);
					}

					if (last_st.flash_handle != -1)
					{
						subtitle_memory_put_handle(last_st.flash_handle);
					}
				}

				if ((si.handle != last_si.flash_handle) && (si.flash_handle != last_si.handle))
				{
					DISP_DBG("[%s:%d] si[%d, %d], last_si[%d, %d]\n", __func__, __LINE__, si.handle, si.flash_handle, last_si.handle, last_si.flash_handle);
					
					if (last_si.handle != -1)
					{
						subtitle_memory_put_handle(last_si.handle);
					}

					if (last_si.flash_handle != -1)
					{
						subtitle_memory_put_handle(last_si.flash_handle);
					}
				}

				if (mix_handle != -1)
				{
					subtitle_memory_put_handle(mix_handle);
				}

				if (tmp_handle != -1)
				{
					mix_handle = tmp_handle;
				}

				cur_pts = st.pts;			

 				subtitle_display_set_info(&last_st, st.handle, st.flash_handle, st.x, st.y, st.w, st.h, st.pts);
				subtitle_display_set_info(&last_si, si.handle, si.flash_handle, si.x, si.y, si.w, si.h, si.pts);
			}
			else if ((st.flash_handle != -1) && (st.handle != -1) && (si.handle == -1) && (si.flash_handle == -1))
			{
				DISP_DBG("CASE-1 : ST Flash Inputted.\n");

#if defined(SUBTITLE_WMIXER_ALL_USE)
				if (last_si.handle != -1)
				{
					caption_handle = st.handle;
					super_handle = last_si.handle;
					tmp_handle = subtitle_memory_get_handle();
					if (tmp_handle == -1)
					{
						cur_handle = st.handle;
					}
					else
					{
						subtitle_display_mixer(st.handle, st.x, st.y, st.w, st.h, 
												last_si.handle, last_si.x, last_si.y, last_si.w, last_si.h,
												tmp_handle,
												1);
						cur_handle = tmp_handle;
					}
					
					if (subtitle_display_get_enable_flag() == 0)
					{
						cur_handle = super_handle;
					}
					else if (superimpose_display_get_enable_flag() == 0)
					{
						cur_handle = caption_handle;
					}
				}
				else
				{
					tmp_handle = subtitle_memory_get_handle();
					if (tmp_handle == -1)
					{
						cur_handle = st.handle;
					}
					else
					{
						subtitle_display_mixer(st.handle, st.x, st.y, st.w, st.h, 
												-1, st.x, st.y, st.w, st.h,
												tmp_handle,
												1);
						cur_handle = tmp_handle;
					}
					caption_handle = st.handle;
					super_handle = -1;
				}
#else
				if (last_si.handle != -1)
				{
					caption_handle = st.handle;
					super_handle = last_si.handle;
					tmp_handle = subtitle_memory_get_handle();
					if (tmp_handle == -1)
					{
						cur_handle = st.handle;
					}
					else
					{
						subtitle_display_mixer(st.handle, st.x, st.y, st.w, st.h, 
												last_si.handle, last_si.x, last_si.y, last_si.w, last_si.h,
												tmp_handle,
												1);
						cur_handle = tmp_handle;
					}
					
					if (subtitle_display_get_enable_flag() == 0)
					{
						cur_handle = super_handle;
					}
					else if (superimpose_display_get_enable_flag() == 0)
					{
						cur_handle = caption_handle;
					}
				}
				else
				{
					cur_handle = st.handle;
					caption_handle = st.handle;
				}
#endif

				if ((last_st.handle != -1) && (st.handle != last_st.flash_handle))
				{
					subtitle_memory_put_handle(last_st.handle);				
					if (last_st.flash_handle != -1)
					{
						subtitle_memory_put_handle(last_st.flash_handle);
					}
				}

				if (mix_handle != -1)
				{
					subtitle_memory_put_handle(mix_handle);
				}

				if (tmp_handle != -1)
				{
					mix_handle = tmp_handle;
				}

				cur_pts = st.pts;		

				subtitle_display_set_info(&last_st, st.handle, st.flash_handle, st.x, st.y, st.w, st.h, st.pts);
			}
			else if ((st.flash_handle == -1) && (st.handle == -1) && (si.handle != -1) && (si.flash_handle != -1))
			{
				DISP_DBG("CASE-2 : SI Flash Inputted.\n");

#if defined(SUBTITLE_WMIXER_ALL_USE)
				if (last_st.handle != -1)
				{
					caption_handle = last_st.handle;
					super_handle = si.handle;
					tmp_handle = subtitle_memory_get_handle();
					if (tmp_handle == -1)
					{
						cur_handle = si.handle;
					}
					else
					{
						subtitle_display_mixer(last_st.handle, last_st.x, last_st.y, last_st.w, last_st.h,
												si.handle, si.x, si.y, si.w, si.h, 												
												tmp_handle,
												1);
						cur_handle = tmp_handle;						
					}
					
					if (subtitle_display_get_enable_flag() == 0)
					{
						cur_handle = super_handle;
					}
					else if (superimpose_display_get_enable_flag() == 0)
					{
						cur_handle = caption_handle;
					}
				}
				else
				{
					tmp_handle = subtitle_memory_get_handle();
					if (tmp_handle == -1)
					{
						cur_handle = si.handle;
					}
					else
					{
						subtitle_display_mixer(-1, si.x, si.y, si.w, si.h,
												si.handle, si.x, si.y, si.w, si.h,
												tmp_handle,
												1);
						cur_handle = tmp_handle;						
					}
					super_handle = si.handle;
					caption_handle = -1;
				}	
#else
				if (last_st.handle != -1)
				{
					caption_handle = last_st.handle;
					super_handle = si.handle;
					tmp_handle = subtitle_memory_get_handle();
					if (tmp_handle == -1)
					{
						cur_handle = si.handle;
					}
					else
					{
						subtitle_display_mixer(last_st.handle, last_st.x, last_st.y, last_st.w, last_st.h,
												si.handle, si.x, si.y, si.w, si.h,												
												tmp_handle,
												1);
						cur_handle = tmp_handle;						
					}
					
					if (subtitle_display_get_enable_flag() == 0)
					{
						cur_handle = super_handle;
					}
					else if (superimpose_display_get_enable_flag() == 0)
					{
						cur_handle = caption_handle;
					}
				}
				else
				{
					cur_handle = si.handle;
					super_handle = si.handle;
				}
#endif

				if ((last_si.handle != -1) && (si.handle != last_si.flash_handle))
				{ 
					subtitle_memory_put_handle(last_si.handle);
					if (last_si.flash_handle != -1)
					{
						subtitle_memory_put_handle(last_si.flash_handle);
					}
				}

				if (mix_handle != -1)
				{
					subtitle_memory_put_handle(mix_handle);
				}

				if (tmp_handle != -1)
				{
					mix_handle = tmp_handle;
				}

				cur_pts = (si.pts == 0) ? subtitle_system_get_systime() : si.pts;

 				subtitle_display_set_info(&last_si, si.handle, si.flash_handle, si.x, si.y, si.w, si.h, si.pts);
			}
			else if ((st.handle != -1) && (si.handle != -1))
			{
				DISP_DBG("CASE-3 : ST & SI Inputted.\n");
				
				tmp_handle = subtitle_memory_get_handle();
				if (tmp_handle == -1)
				{
					cur_handle = st.handle;
				}
				else
				{
					caption_handle = st.handle;
					super_handle = si.handle;
					subtitle_display_mixer(st.handle, st.x, st.y, st.w, st.h,
											si.handle, si.x, si.y, si.w, si.h,											
											tmp_handle,
											1);
					cur_handle = tmp_handle;
					if (subtitle_display_get_enable_flag() == 0)
					{
						cur_handle = super_handle;
					}
					else if (superimpose_display_get_enable_flag() == 0)
					{
						cur_handle = caption_handle;
					}
				}

				if ((st.handle != last_st.flash_handle) && (st.flash_handle != last_st.handle))
				{
					DISP_DBG("[%s:%d] st[%d, %d], last_st[%d, %d]\n", __func__, __LINE__, st.handle, st.flash_handle, last_st.handle, last_st.flash_handle);
					
					if (last_st.handle != -1)
					{
						subtitle_memory_put_handle(last_st.handle);
					}

					if (last_st.flash_handle != -1)
					{
						subtitle_memory_put_handle(last_st.flash_handle);
					}
				}

				if ((si.handle != last_si.flash_handle) && (si.flash_handle != last_si.handle))
				{
					DISP_DBG("[%s:%d] si[%d, %d], last_si[%d, %d]\n", __func__, __LINE__, si.handle, si.flash_handle, last_si.handle, last_si.flash_handle);
					
					if (last_si.handle != -1)
					{
						subtitle_memory_put_handle(last_si.handle);
					}

					if (last_si.flash_handle != -1)
					{
						subtitle_memory_put_handle(last_si.flash_handle);
					}
				}
				
				if (mix_handle != -1)
				{
					subtitle_memory_put_handle(mix_handle);
				}

				if (tmp_handle != -1)
				{
					mix_handle = tmp_handle;
				}
				
				cur_pts = st.pts;			

				subtitle_display_set_info(&last_st, st.handle, st.flash_handle, st.x, st.y, st.w, st.h, st.pts);
				subtitle_display_set_info(&last_si, si.handle, si.flash_handle, si.x, si.y, si.w, si.h, si.pts);
			}
			else if ((st.handle != -1) && (si.handle == -1))
			{
				DISP_DBG("CASE-4 : ST Inputted.\n");
				
#if defined(SUBTITLE_WMIXER_ALL_USE)
				if (last_si.handle != -1)
				{
					caption_handle = st.handle;
					super_handle = last_si.handle;
					tmp_handle = subtitle_memory_get_handle();
					if (tmp_handle == -1)
					{
						cur_handle = st.handle;
					}
					else
					{
						subtitle_display_mixer(st.handle, st.x, st.y, st.w, st.h, 
												last_si.handle, last_si.x, last_si.y, last_si.w, last_si.h,
												tmp_handle,
												1);
						cur_handle = tmp_handle;												
					}

					if (subtitle_display_get_enable_flag() == 0)
					{
						cur_handle = super_handle;
					}
					else if (superimpose_display_get_enable_flag() == 0)
					{
						cur_handle = caption_handle;
					}
				}
				else
				{
					tmp_handle = subtitle_memory_get_handle();
					if (tmp_handle == -1)
					{
						cur_handle = st.handle;
					}
					else
					{
						subtitle_display_mixer(st.handle, st.x, st.y, st.w, st.h, 
												-1, st.x, st.y, st.w, st.h,
												tmp_handle,
												1);
						cur_handle = tmp_handle;												
					}
					caption_handle = st.handle;
					super_handle = -1;
				}
#else
				if (last_si.handle != -1)
				{
					caption_handle = st.handle;
					super_handle = last_si.handle;
					tmp_handle = subtitle_memory_get_handle();
					if (tmp_handle == -1)
					{
						cur_handle = st.handle;
					}
					else
					{
						subtitle_display_mixer(st.handle, st.x, st.y, st.w, st.h, 
												last_si.handle, last_si.x, last_si.y, last_si.w, last_si.h,
												tmp_handle,
												1);
						cur_handle = tmp_handle;												
					}

					if (subtitle_display_get_enable_flag() == 0)
					{
						cur_handle = super_handle;
					}
					else if (superimpose_display_get_enable_flag() == 0)
					{
						cur_handle = caption_handle;
					}
				}
				else
				{
					cur_handle = st.handle;
					caption_handle = st.handle;
				}
#endif				

				if (last_st.handle != -1)
				{
					subtitle_memory_put_handle(last_st.handle);
				}

				if (last_st.flash_handle != -1)
				{
					subtitle_memory_put_handle(last_st.flash_handle);
				}

				if (mix_handle != -1)
				{
					subtitle_memory_put_handle(mix_handle);
				}

				if (tmp_handle != -1)
				{
					mix_handle = tmp_handle;
				}

				cur_pts = st.pts;			

				subtitle_display_set_info(&last_st, st.handle, st.flash_handle, st.x, st.y, st.w, st.h, st.pts);
			}
			else if ((st.handle == -1) && (si.handle != -1))
			{
				DISP_DBG("CASE-5 : SI Inputted.\n");

				if (stc_wait_state == 1)
				{
					DISP_DBG("CASE-5 : SI Inputted. wait state\n");	
					continue;
				}

#if defined(SUBTITLE_WMIXER_ALL_USE)
				if (last_st.handle != -1)
				{
					caption_handle = last_st.handle;
					super_handle = si.handle;
					tmp_handle = subtitle_memory_get_handle();

					if (tmp_handle == -1)
					{
						cur_handle = si.handle;
					}
					else
					{
						subtitle_display_mixer(last_st.handle, last_st.x, last_st.y, last_st.w, last_st.h,
												si.handle, si.x, si.y, si.w, si.h, 												
												tmp_handle,
												1);

						cur_handle = tmp_handle;
					}

					if (subtitle_display_get_enable_flag() == 0)
					{
						cur_handle = super_handle;
					}
					else if (superimpose_display_get_enable_flag() == 0)
					{
						cur_handle = caption_handle;
					}
				}
				else
				{
					tmp_handle = subtitle_memory_get_handle();
					if (tmp_handle == -1)
					{
						cur_handle = si.handle;
					}
					else
					{
						subtitle_display_mixer(-1, si.x, si.y, si.w, si.h,
												si.handle, si.x, si.y, si.w, si.h, 												
												tmp_handle,
												1);

						cur_handle = tmp_handle;
					}
					super_handle = si.handle;
					caption_handle = -1;
				}
#else
				if (last_st.handle != -1)
				{
					caption_handle = last_st.handle;
					super_handle = si.handle;
					tmp_handle = subtitle_memory_get_handle();

					if (tmp_handle == -1)
					{
						cur_handle = si.handle;
					}
					else
					{
						subtitle_display_mixer(last_st.handle, last_st.x, last_st.y, last_st.w, last_st.h,
												si.handle, si.x, si.y, si.w, si.h,												
												tmp_handle,
												1);

						cur_handle = tmp_handle;
					}

					if (subtitle_display_get_enable_flag() == 0)
					{
						cur_handle = super_handle;
					}
					else if (superimpose_display_get_enable_flag() == 0)
					{
						cur_handle = caption_handle;
					}
				}
				else
				{
					cur_handle = si.handle;
					super_handle = si.handle;
				}
#endif

				if (last_si.handle != -1)
				{
					subtitle_memory_put_handle(last_si.handle);
				}

				if (last_si.flash_handle != -1)
				{
					subtitle_memory_put_handle(last_si.flash_handle);
				}

				if (mix_handle != -1)
				{
					subtitle_memory_put_handle(mix_handle);
				}

				if (tmp_handle != -1)
				{
					mix_handle = tmp_handle;
				}

				cur_pts = (si.pts == 0) ? subtitle_system_get_systime() : (si.pts + subtitle_system_get_systime());

 				subtitle_display_set_info(&last_si, si.handle, si.flash_handle, si.x, si.y, si.w, si.h, si.pts);
			}
			else
			{
				LOGE("[%s:%d] Invalid state(%d,%d)\n", __func__, __LINE__, st.handle, si.handle);
				continue;
			}

			if (-2 < caption_handle) //protect invalid handle
			{
				subtitle_current_set_st_handle(caption_handle);
			}

			if (-2 < super_handle)
			{
				subtitle_current_set_si_handle(super_handle);
			}

			if (subtitle_display_get_skip_pts() == 0)
			{
				cur_stc_time = subtitle_system_get_systime();
				DISP_DBG("cur_stc_time (%lld) prev_stc_time(%lld) pts(%lld, %lld)\n", cur_stc_time, prev_stc_time, subtitle_system_get_delay(cur_pts), cur_pts);				
				if ((0 < cur_stc_time) && (0 < prev_stc_time))
				{
					stc_gap = cur_stc_time - prev_stc_time;
					idelay = subtitle_system_get_delay(cur_pts);
					DISP_DBG("\033[32m waittime(%lld) stc_gap(%lld) thread(%d)\033[m\n", idelay, stc_gap, subtitle_display_get_thread_flag());
					if ((0 < idelay) && (0 < stc_gap) && (subtitle_display_get_thread_flag() == 1))
					{
						DISP_DBG("waittime process(%lld)\n", idelay);
						tcc_dxb_sem_down_timewait_msec(&gSubtitleWait, (int)idelay);
						if (subtitle_display_get_thread_flag() == 0)
						{
							break; 
						}
					}
				}
				prev_stc_time = subtitle_system_get_systime();
 			}

			subtitle_display_update(cur_handle);

			DISP_DBG("Display out(%d)\n", cur_handle);

 			last_st.flash_handle = st.flash_handle;
			last_si.flash_handle = si.flash_handle;

			if ((last_st.flash_handle != -1) && (last_si.flash_handle != -1))
			{
				DISP_DBG("FLASH CASE-0 : ST & SI Flash Inputted.\n");
				subtitle_display_check_flash_condition(0, &last_st, cur_pts);				
				subtitle_display_check_flash_condition(1, &last_si, cur_pts);
			}
			else if (last_st.flash_handle != -1)
			{
				DISP_DBG("FLASH CASE-1 : ST Flash Inputted.\n");
				subtitle_display_check_flash_condition(0, &last_st, cur_pts);
			}
			else if (last_si.flash_handle != -1)
			{
				DISP_DBG("FLASH CASE-2 : SI Flash Inputted.\n");
				subtitle_display_check_flash_condition(1, &last_si, cur_pts);
			}

			subtitle_display_check_clear_cond(&last_st, &last_si);
			DISP_DBG("\n");
		}
	}

	DISP_DBG("[%s] exit\n", __func__);
	subtitle_display_set_thread_flag(-1);

	return (void*)NULL;
}
#else
void* subtitle_display_linux_thread(void *data)
{
	int st_ret, si_ret;
	int tmp_handle, cur_handle, caption_handle, super_handle, mix_handle;
	int st_check, si_check;
	int stc_index, stc_delay_enable, stc_wait_state;
	DISP_CTX_TYPE st, si, last_st, last_si, next_st, next_si;
	long long idelay, stc_gap;
	unsigned long long cur_pts, stc_delay_time, cur_stc_time, prev_stc_time;
	
	cur_handle = tmp_handle = caption_handle = super_handle = mix_handle = -1;
	cur_pts = st_check = si_check = 0;
	stc_delay_enable = stc_delay_time = stc_index = stc_wait_state = cur_stc_time = prev_stc_time = stc_gap = 0;

	subtitle_display_init_disp_ctx(&st);
	subtitle_display_init_disp_ctx(&si);
	subtitle_display_init_disp_ctx(&last_st);
	subtitle_display_init_disp_ctx(&last_si);
	subtitle_display_init_disp_ctx(&next_st);
	subtitle_display_init_disp_ctx(&next_si);
	
	while(subtitle_display_get_thread_flag() == 1)
	{
		if(subtitle_display_get_thread_flag() == 0){ 
			break;
		}

		if(subtitle_display_get_enable_flag() == 0){
			if(super_handle == -1 && st_check == 0){
				subtitle_display_clear();
				st_check = 1;
			}else if(super_handle != -1 && st_check == 0 && superimpose_display_get_enable_flag() == 1){
				subtitle_display_update(super_handle);
				st_check = 1;
			}			
		}

		if(superimpose_display_get_enable_flag() == 0){
			if(caption_handle == -1 && si_check == 0){
				subtitle_display_clear();
				si_check = 1;
			}else if(caption_handle != -1 && si_check == 0 && subtitle_display_get_enable_flag() == 1){
				subtitle_display_update(caption_handle);
				si_check = 1;
			}
		}
		
		if(subtitle_display_get_enable_flag() == 1){
			st_check = 0;
		}else if(superimpose_display_get_enable_flag() == 1){
			si_check = 0;
		}
		
		tmp_handle = cur_handle = -1;
		st_ret = st.handle = st.flash_handle = -1;
		si_ret = si.handle = si.flash_handle = -1;
		st.pts = si.pts = 0;
		
		st_ret = subtitle_queue_get(0, &st.handle, &st.x, &st.y, &st.w, &st.h, &st.pts, &st.flash_handle);
		si_ret = subtitle_queue_get(1, &si.handle, &si.x, &si.y, &si.w, &si.h, &si.pts, &si.flash_handle);

		if((st_ret < 0) && (si_ret < 0)){
			subtitle_display_check_clear_cond(&last_st, &last_si);
			usleep(1000);
		}else{
			if((st.handle == -1) && (si.handle == -1)){
				LOGE("[%s] invalid handle (%d, %d)\n", __func__, st.handle, si.handle);
				continue;
			}

			DISP_DBG("[%s:%d] st[%d, %d], last_st[%d, %d], st.pts:%lld, st_rect[%d:%d:%d:%d]\n", \
						__func__, __LINE__, st.handle, st.flash_handle, last_st.handle, last_st.flash_handle, st.pts, st.x, st.y, st.w, st.h);
			DISP_DBG("[%s:%d] si[%d, %d], last_si[%d, %d], si.pts:%lld, si_rect[%d:%d:%d:%d]\n", \
						__func__, __LINE__, si.handle, si.flash_handle, last_si.handle, last_si.flash_handle, si.pts, si.x, si.y, si.w, si.h);

			stc_index = subtitle_system_get_stc_index();			
			if(stc_index == 0){
				stc_delay_enable = subtitle_system_get_stc_delay_excute();
				if(stc_delay_enable == 1){
					stc_delay_time = subtitle_system_get_stc_delay_time();
					DISP_DBG("\033[31m stc wait time(%lld) thread[%d]\033[m\n", stc_delay_time, subtitle_display_get_thread_flag());
					if(stc_delay_time > 0 && subtitle_display_get_thread_flag()==1) {
						stc_wait_state = 1;
						tcc_dxb_sem_down_timewait_msec(&gSubtitleWait, (int)(stc_delay_time)+352);
						if(subtitle_display_get_thread_flag() == 0){
							break;
						}
						stc_wait_state = 0;
					}
				}
			}else{
				tcc_dxb_sem_signal(&gSubtitleWait);
			}

			DISP_DBG("[subtitle_display_linux_thread] index(%d)enable(%d)time(%lld)", stc_index, stc_delay_enable, stc_delay_time);

			if((st.flash_handle != -1) && (st.handle != -1) && (si.handle != -1) && (si.flash_handle != -1)){
				DISP_DBG("CASE-0 : ST & SI & Flash Inputted.");
				
				tmp_handle = subtitle_memory_get_handle();
				if(tmp_handle == -1){
					cur_handle = st.handle;
				}else{
					caption_handle = st.handle;
					super_handle = si.handle;
					
					subtitle_data_sw_mixer(st.handle, si.handle, tmp_handle, 0, 0, st.w, st.h);
					
					cur_handle = tmp_handle;
					if(subtitle_display_get_enable_flag() == 0){
						cur_handle = super_handle;
					}else if(superimpose_display_get_enable_flag() == 0){
						cur_handle = caption_handle;
					}
				}

				if((st.handle != last_st.flash_handle) && (st.flash_handle != last_st.handle)){
					DISP_DBG("[%s:%d] st[%d, %d], last_st[%d, %d]\n", __func__, __LINE__, 
						st.handle, st.flash_handle, last_st.handle, last_st.flash_handle);
					
					if(last_st.handle != -1) subtitle_memory_put_handle(last_st.handle);
					if(last_st.flash_handle != -1) subtitle_memory_put_handle(last_st.flash_handle);
				}

				if((si.handle != last_si.flash_handle) && (si.flash_handle != last_si.handle)){
					DISP_DBG("[%s:%d] si[%d, %d], last_si[%d, %d]\n", __func__, __LINE__, 
						si.handle, si.flash_handle, last_si.handle, last_si.flash_handle);
					
					if(last_si.handle != -1) subtitle_memory_put_handle(last_si.handle);
					if(last_si.flash_handle != -1) subtitle_memory_put_handle(last_si.flash_handle);
				}

				if(mix_handle != -1) subtitle_memory_put_handle(mix_handle);
				if(tmp_handle != -1) mix_handle = tmp_handle;
				cur_pts = st.pts;			
 				subtitle_display_set_info(&last_st, st.handle, st.flash_handle, st.x, st.y, st.w, st.h, st.pts);
				subtitle_display_set_info(&last_si, si.handle, si.flash_handle, si.x, si.y, si.w, si.h, si.pts);
			}else if((st.flash_handle != -1) && (st.handle != -1) && (si.handle == -1) && (si.flash_handle == -1)){
				DISP_DBG("CASE-1 : ST Flash Inputted.");

#if defined(SUBTITLE_WMIXER_ALL_USE)
				if(last_si.handle != -1){
					caption_handle = st.handle;
					super_handle = last_si.handle;
					tmp_handle = subtitle_memory_get_handle();
					if(tmp_handle == -1){
						cur_handle = st.handle;
					}else{
						subtitle_data_sw_mixer(st.handle, last_si.handle, tmp_handle, 0, 0, st.w, st.h);
						cur_handle = tmp_handle;
					}
					
					if(subtitle_display_get_enable_flag() == 0){
						cur_handle = super_handle;
					}else if(superimpose_display_get_enable_flag() == 0){
						cur_handle = caption_handle;
					}
				}else{
					tmp_handle = subtitle_memory_get_handle();
					if(tmp_handle == -1){
						cur_handle = st.handle;
					}else{
						subtitle_data_sw_mixer(st.handle, -1, tmp_handle, 0, 0, st.w, st.h);
						cur_handle = tmp_handle;
					}
					caption_handle = st.handle;
					super_handle = -1;
				}

#else
				if(last_si.handle != -1){
					caption_handle = st.handle;
					super_handle = last_si.handle;
					tmp_handle = subtitle_memory_get_handle();
					if(tmp_handle == -1){
						cur_handle = st.handle;
					}else{
						subtitle_data_sw_mixer(st.handle, last_si.handle, tmp_handle, 0, 0, st.w, st.h);
						cur_handle = tmp_handle;
					}
					
					if(subtitle_display_get_enable_flag() == 0){
						cur_handle = super_handle;
					}else if(superimpose_display_get_enable_flag() == 0){
						cur_handle = caption_handle;
					}
				}else{
					cur_handle = st.handle;
					caption_handle = st.handle;
				}

#endif

				if((last_st.handle != -1) && (st.handle != last_st.flash_handle)) {
					subtitle_memory_put_handle(last_st.handle);				
					if(last_st.flash_handle != -1) subtitle_memory_put_handle(last_st.flash_handle);
				}

				if(mix_handle != -1) subtitle_memory_put_handle(mix_handle);
				if(tmp_handle != -1) mix_handle = tmp_handle;
				cur_pts = st.pts;		
				subtitle_display_set_info(&last_st, st.handle, st.flash_handle, st.x, st.y, st.w, st.h, st.pts);
			}else if((st.flash_handle == -1) && (st.handle == -1) && (si.handle != -1) && (si.flash_handle != -1)){
				DISP_DBG("CASE-2 : SI Flash Inputted.");

#if defined(SUBTITLE_WMIXER_ALL_USE)
				if(last_st.handle != -1){
					caption_handle = last_st.handle;
					super_handle = si.handle;
					tmp_handle = subtitle_memory_get_handle();
					if(tmp_handle == -1){
						cur_handle = si.handle;
					}else{
						subtitle_data_sw_mixer(last_st.handle, si.handle, tmp_handle, 0, 0, last_st.w, last_st.h);						
						cur_handle = tmp_handle;
					}
					
					if(subtitle_display_get_enable_flag() == 0){
						cur_handle = super_handle;
					}else if(superimpose_display_get_enable_flag() == 0){
						cur_handle = caption_handle;
					}
				}else{
					tmp_handle = subtitle_memory_get_handle();
					if(tmp_handle == -1){
						cur_handle = si.handle;
					}else{
						subtitle_data_sw_mixer(-1, si.handle, tmp_handle, 0, 0, si.w, si.h);												
						cur_handle = tmp_handle;						
					}
					super_handle = si.handle;
					caption_handle = -1;
				}	
#else
				if(last_st.handle != -1){
					caption_handle = last_st.handle;
					super_handle = si.handle;
					tmp_handle = subtitle_memory_get_handle();
					if(tmp_handle == -1){
						cur_handle = si.handle;
					}else{
						subtitle_data_sw_mixer(last_st.handle, si.handle, tmp_handle, 0, 0, last_st.w, last_st.h);																		
						cur_handle = tmp_handle;
					}
					
					if(subtitle_display_get_enable_flag() == 0){
						cur_handle = super_handle;
					}else if(superimpose_display_get_enable_flag() == 0){
						cur_handle = caption_handle;
					}
				}else{
					cur_handle = si.handle;
					super_handle = si.handle;
				}
#endif
				if((last_si.handle != -1) && (si.handle != last_si.flash_handle)){ 
					subtitle_memory_put_handle(last_si.handle);
					if(last_si.flash_handle != -1) subtitle_memory_put_handle(last_si.flash_handle);
				}

				if(mix_handle != -1) subtitle_memory_put_handle(mix_handle);
				if(tmp_handle != -1) mix_handle = tmp_handle;
				cur_pts = (si.pts==0)?subtitle_system_get_systime():si.pts;
 				subtitle_display_set_info(&last_si, si.handle, si.flash_handle, si.x, si.y, si.w, si.h, si.pts);
			}else if((st.handle != -1) && (si.handle != -1)){
				DISP_DBG("CASE-3 : ST & SI Inputted.");
				
				tmp_handle = subtitle_memory_get_handle();
				if(tmp_handle == -1){
					cur_handle = st.handle;
				}else{
					caption_handle = st.handle;
					super_handle = si.handle;
					subtitle_data_sw_mixer(st.handle, si.handle, tmp_handle, 0, 0, st.w, st.h);
					cur_handle = tmp_handle;
					if(subtitle_display_get_enable_flag() == 0){
						cur_handle = super_handle;
					}else if(superimpose_display_get_enable_flag() == 0){
						cur_handle = caption_handle;
					}
				}

				if((st.handle != last_st.flash_handle) && (st.flash_handle != last_st.handle)){
					DISP_DBG("[%s:%d] st[%d, %d], last_st[%d, %d]\n", __func__, __LINE__, 
						st.handle, st.flash_handle, last_st.handle, last_st.flash_handle);
					
					if(last_st.handle != -1) subtitle_memory_put_handle(last_st.handle);
					if(last_st.flash_handle != -1) subtitle_memory_put_handle(last_st.flash_handle);
				}

				if((si.handle != last_si.flash_handle) && (si.flash_handle != last_si.handle)){
					DISP_DBG("[%s:%d] si[%d, %d], last_si[%d, %d]\n", __func__, __LINE__, 
						si.handle, si.flash_handle, last_si.handle, last_si.flash_handle);
					
					if(last_si.handle != -1) subtitle_memory_put_handle(last_si.handle);
					if(last_si.flash_handle != -1) subtitle_memory_put_handle(last_si.flash_handle);
				}
				
				if(mix_handle != -1) subtitle_memory_put_handle(mix_handle);
				if(tmp_handle != -1) mix_handle = tmp_handle;
				
				cur_pts = st.pts;			
 		
				subtitle_display_set_info(&last_st, st.handle, st.flash_handle, st.x, st.y, st.w, st.h, st.pts);
				subtitle_display_set_info(&last_si, si.handle, si.flash_handle, si.x, si.y, si.w, si.h, si.pts);
			}else if((st.handle != -1) && (si.handle == -1)){
				DISP_DBG("CASE-4 : ST Inputted.");
				
#if defined(SUBTITLE_WMIXER_ALL_USE)
				if(last_si.handle != -1){
					caption_handle = st.handle;
					super_handle = last_si.handle;
					tmp_handle = subtitle_memory_get_handle();
					if(tmp_handle == -1){
						cur_handle = st.handle;
					}else{
						subtitle_data_sw_mixer(st.handle, last_si.handle, tmp_handle, 0, 0, st.w, st.h);
						cur_handle = tmp_handle;
					}

					if(subtitle_display_get_enable_flag() == 0){
						cur_handle = super_handle;
					}else if(superimpose_display_get_enable_flag() == 0){
						cur_handle = caption_handle;
					}
				}else{
					tmp_handle = subtitle_memory_get_handle();
					if(tmp_handle == -1){
						cur_handle = st.handle;
					}else{
						subtitle_data_sw_mixer(st.handle, -1, tmp_handle, 0, 0, st.w, st.h);						
						cur_handle = tmp_handle;
					}
					caption_handle = st.handle;
					super_handle = -1;
				}
#else
				if(last_si.handle != -1){
					caption_handle = st.handle;
					super_handle = last_si.handle;
					tmp_handle = subtitle_memory_get_handle();
					if(tmp_handle == -1){
						cur_handle = st.handle;
					}else{
						subtitle_data_sw_mixer(st.handle, last_si.handle, tmp_handle, 0, 0, st.w, st.h);						
						cur_handle = tmp_handle;
					}

					if(subtitle_display_get_enable_flag() == 0){
						cur_handle = super_handle;
					}else if(superimpose_display_get_enable_flag() == 0){
						cur_handle = caption_handle;
					}
				}else{
					cur_handle = st.handle;
					caption_handle = st.handle;
				}
#endif				

				if(last_st.handle != -1) subtitle_memory_put_handle(last_st.handle);
				if(last_st.flash_handle != -1) subtitle_memory_put_handle(last_st.flash_handle);
				if(mix_handle != -1) subtitle_memory_put_handle(mix_handle);
				if(tmp_handle != -1) mix_handle = tmp_handle;
				cur_pts = st.pts;			
				subtitle_display_set_info(&last_st, st.handle, st.flash_handle, st.x, st.y, st.w, st.h, st.pts);
			}else if((st.handle == -1) && (si.handle != -1)){
				DISP_DBG("CASE-5 : SI Inputted.");
				if(stc_wait_state == 1){
					DISP_DBG("CASE-5 : SI Inputted. wait state");	
					continue;
				}
#if defined(SUBTITLE_WMIXER_ALL_USE)
				if(last_st.handle != -1){
					caption_handle = last_st.handle;
					super_handle = si.handle;
					tmp_handle = subtitle_memory_get_handle();

					if(tmp_handle == -1){
						cur_handle = si.handle;
					}else{
						subtitle_data_sw_mixer(last_st.handle, si.handle, tmp_handle, 0, 0, last_st.w, last_st.h);
						cur_handle = tmp_handle;
					}

					if(subtitle_display_get_enable_flag() == 0){
						cur_handle = super_handle;
					}else if(superimpose_display_get_enable_flag() == 0){
						cur_handle = caption_handle;
					}
				}else{
					tmp_handle = subtitle_memory_get_handle();
					if(tmp_handle == -1){
						cur_handle = si.handle;
					}else{
						subtitle_data_sw_mixer(-1, si.handle, tmp_handle, 0, 0, si.w, si.h);		
						cur_handle = tmp_handle;
					}
					super_handle = si.handle;
					caption_handle = -1;
				}
#else
				if(last_st.handle != -1){
					caption_handle = last_st.handle;
					super_handle = si.handle;
					tmp_handle = subtitle_memory_get_handle();

					if(tmp_handle == -1){
						cur_handle = si.handle;
					}else{
						subtitle_data_sw_mixer(last_st.handle, si.handle, tmp_handle, 0, 0, last_st.w, last_st.h);
						cur_handle = tmp_handle;
					}

					if(subtitle_display_get_enable_flag() == 0){
						cur_handle = super_handle;
					}else if(superimpose_display_get_enable_flag() == 0){
						cur_handle = caption_handle;
					}
				}else{
					cur_handle = si.handle;
					super_handle = si.handle;
				}
#endif

				if(last_si.handle != -1) subtitle_memory_put_handle(last_si.handle);
				if(last_si.flash_handle != -1) subtitle_memory_put_handle(last_si.flash_handle);
				if(mix_handle != -1) subtitle_memory_put_handle(mix_handle);
				if(tmp_handle != -1) mix_handle = tmp_handle;
				cur_pts = (si.pts==0)?subtitle_system_get_systime():(si.pts+subtitle_system_get_systime());
 				subtitle_display_set_info(&last_si, si.handle, si.flash_handle, si.x, si.y, si.w, si.h, si.pts);
			}else{
				LOGE("[%s:%d] Invalid state(%d,%d)\n", __func__, __LINE__, st.handle, si.handle);
				continue;
			}

			if(caption_handle > -2){ //protect invalid handle
				subtitle_current_set_st_handle(caption_handle);
			}

			if(super_handle > -2){
				subtitle_current_set_si_handle(super_handle);
			}

			if(subtitle_display_get_skip_pts() == 0){
				cur_stc_time = subtitle_system_get_systime();
				DISP_DBG("cur_stc_time (%lld) prev_stc_time(%lld) pts(%lld, %lld)\n", cur_stc_time, prev_stc_time, subtitle_system_get_delay(cur_pts), cur_pts);				
				if(cur_stc_time > 0 && prev_stc_time > 0){			 
					stc_gap = cur_stc_time - prev_stc_time;
					idelay = subtitle_system_get_delay(cur_pts);
					DISP_DBG("\033[32m waittime(%lld) stc_gap(%lld) thread(%d)\033[m\n", idelay, stc_gap, subtitle_display_get_thread_flag());
					if(idelay > 0 && stc_gap > 0 && subtitle_display_get_thread_flag()==1){
						DISP_DBG("waittime process(%lld)\n", idelay);
						tcc_dxb_sem_down_timewait_msec(&gSubtitleWait, (int)idelay);
						if(subtitle_display_get_thread_flag() == 0){
							break; 
						}
					}
				}
				prev_stc_time = subtitle_system_get_systime();
 			}

			subtitle_display_update(cur_handle);

			DISP_DBG("Display out(%d)\n", cur_handle);

 			last_st.flash_handle = st.flash_handle;
			last_si.flash_handle = si.flash_handle;

			if((last_st.flash_handle != -1) && (last_si.flash_handle != -1)){
				DISP_DBG("FLASH CASE-0 : ST & SI Flash Inputted.");
				subtitle_display_check_flash_condition(0, &last_st, cur_pts);				
				subtitle_display_check_flash_condition(1, &last_si, cur_pts);
			}else if(last_st.flash_handle != -1){
				DISP_DBG("FLASH CASE-1 : ST Flash Inputted.");
				subtitle_display_check_flash_condition(0, &last_st, cur_pts);
			}else if(last_si.flash_handle != -1){
				DISP_DBG("FLASH CASE-2 : SI Flash Inputted.");
				subtitle_display_check_flash_condition(1, &last_si, cur_pts);
			}

			subtitle_display_check_clear_cond(&last_st, &last_si);
			DISP_DBG("\n");
		}
	}
	DISP_DBG("[%s] exit\n", __func__);
	subtitle_display_set_thread_flag(-1);

	return (void*)NULL;
}
#endif
#endif

int subtitle_display_open_thread(void)
{
	int status;

	subtitle_display_set_thread_flag(1);
	#if defined(HAVE_LINUX_PLATFORM)
	status = tcc_dxb_thread_create((void *)&g_thread,
								(pThreadFunc)subtitle_display_linux_thread,
								(unsigned char*)"subtitle_display_linux_thread", LOW_PRIORITY_11,
								NULL);
	#endif
	if(status){
		LOGE("[%s] thread create error.", __func__);
		subtitle_display_set_thread_flag(0);
		return -1;
	}
	subtitle_set_force_clear(0);

	return 0;
}

int subtitle_display_close_thread(int reinit)
{
	int status, ret;
	void *pStatus = &status;

	LOGE("In [%s] \n", __func__);
	if(subtitle_display_get_thread_flag()==1){
		subtitle_display_set_thread_flag(0);
		while(1){
			tcc_dxb_sem_signal(&gSubtitleWait);
			if(subtitle_display_get_thread_flag() == -1) break;
			usleep(5000);
		}
		ret = tcc_dxb_thread_join((void *)g_thread,(void **)&pStatus);

		subtitle_display_clear();
		subtitle_set_force_clear(1);
		if(reinit == 0){
			subtitle_visible_clear();
		}
	}
	LOGE("Out [%s] \n", __func__);
	return 0;
}

void subtitle_visible_clear(void)
{
	subtitle_display_set_enable_flag(0);
	superimpose_display_set_enable_flag(0);
	g_Caption_AutoDisplay = 0;
	g_Superimpose_AutoDisplay = 0;
	g_all_clear = 1;
}

int subtitle_display_close(void)
{
//	LOGE("In [%s]\n", __func__);

	SUB_SYS_INFO_TYPE *pInfo = (SUB_SYS_INFO_TYPE*)get_sub_context();

#if 1    // Noah, To avoid CodeSonar warning, CodeSonar thinks subtitle_fb_type() == 0 always evaluates to true.

	/* 0:CCFB, 1:SURFACE */
	#if defined(HAVE_LINUX_PLATFORM)

		#if defined(SUBTITLE_CCFB_DISPLAY_ENABLE)
		if(g_sub_op_ftn.pEnable) { (g_sub_op_ftn.pEnable)(0); }
		#endif
		if(g_sub_op_ftn.pClose) { (g_sub_op_ftn.pClose)(); }

	#endif

#else    // Following codes are original.

	if(subtitle_fb_type() == 0){
		#if defined(SUBTITLE_CCFB_DISPLAY_ENABLE)
		if(g_sub_op_ftn.pEnable) { (g_sub_op_ftn.pEnable)(0); }
		#endif
		if(g_sub_op_ftn.pClose) { (g_sub_op_ftn.pClose)(); }
	}else{
		pthread_mutex_lock (&g_subtitle_mutex);
		pInfo->subtitle_addr = NULL;
		pthread_mutex_unlock (&g_subtitle_mutex);

		pthread_mutex_lock (&g_superimpose_mutex);
		pInfo->superimpose_addr = NULL;
		pthread_mutex_unlock (&g_superimpose_mutex);

		pthread_mutex_lock (&g_png_mutex);
		pInfo->png_addr = NULL;
		pthread_mutex_unlock (&g_png_mutex);
	}

#endif

//	LOGE("Out [%s]\n", __func__);
	return 0;
}

int subtitle_data_enable(int enable)
{
	int ret = 0;
	LOGE("[%s] enable:%d saved_enable:%d\n", __func__, enable, subtitle_display_get_enable_flag());
	if(subtitle_display_get_enable_flag() != enable && enable == 0){
		subtitle_set_st_change_onoff_flag(1);
	}

	subtitle_display_set_enable_flag(enable);
	if(subtitle_fb_type() == 0){
		if(subtitle_display_get_enable_flag() == 0 && g_Caption_AutoDisplay == 1){
			enable = 1;
			subtitle_display_set_enable_flag(1);
			if(tcc_manager_demux_get_validOutput()==0){
				//SerivceID check => other serviceID so, OFF
				enable = 0;
				subtitle_display_set_enable_flag(0);
			}
			if(subtitle_get_dmf_flag() == 0){
				//DMF parsing not yet, selective mode
				enable = 0;
				subtitle_display_set_enable_flag(0);
			}
		}

		if(enable == 0){
			LOGE("[%s] caption off => clear, super[%d], memory[%d], handle[%d]\n", __func__, superimpose_display_get_enable_flag(), subtitle_display_get_memory_sync_flag(), subtitle_current_get_si_handle());
			subtitle_display_clear_st_set(1);
			if(subtitle_current_get_si_handle() == -1){
				subtitle_display_clear();
			}else{
				if(superimpose_display_get_enable_flag() == 1 && subtitle_display_get_thread_flag() == 1){
					ret = subtitle_display_update(subtitle_current_get_si_handle());
				}else{
					subtitle_display_clear();
				}
			}
		}else{
			subtitle_display_clear_st_set(0);
		}

#if defined(SUBTITLE_CCFB_DISPLAY_ENABLE)
		if(g_sub_op_ftn.pEnable){
			if(enable){
				(g_sub_op_ftn.pEnable)(enable);
			}
		}
#endif
	}

	return ret;
}

int superimpose_data_enable(int enable)
{
	int ret = 0;
	LOGE("[%s] enable:%d saved_enable:%d\n", __func__, enable, superimpose_display_get_enable_flag());
	if(superimpose_display_get_enable_flag() != enable && enable == 0){
		subtitle_set_si_change_onoff_flag(1);
	}

	superimpose_display_set_enable_flag(enable);
	if(subtitle_fb_type() == 0){
		if(superimpose_display_get_enable_flag() == 0 && g_Superimpose_AutoDisplay == 1){
			enable = 1;
			superimpose_display_set_enable_flag(1);
			if(tcc_manager_demux_get_validOutput()==0){
				//SerivceID check => other serviceID so, OFF
				enable = 0;
				superimpose_display_set_enable_flag(0);
			}

			if(subtitle_get_dmf_flag() == 0){
				//DMF parsing not yet, selective mode
				enable = 0;
				superimpose_display_set_enable_flag(0);
			}
		}

		if(enable == 0){
			LOGE("[%s] superimpose(png) off => clear, caption[%d], memory[%d] handle[%d]\n", __func__, subtitle_display_get_enable_flag(), subtitle_display_get_memory_sync_flag(), subtitle_current_get_st_handle());
			subtitle_display_clear_si_set(1);
			if(subtitle_current_get_st_handle() == -1){
				subtitle_display_clear();
			}else{
				if(subtitle_display_get_enable_flag() == 1 && subtitle_display_get_thread_flag() == 1){
					ret = subtitle_display_update(subtitle_current_get_st_handle());
				}else{
					subtitle_display_clear();
				}
			}
		}else{
			subtitle_display_clear_si_set(0);
		}
	}

	return ret;
}

void png_data_enable(int enable)
{
	DISP_DBG("[%s] %d\n", __func__, enable);
	g_PngVisible = enable;
}

void subtitle_set_dmf_display(int type, int autodisplay)
{
	ALOGE("subtitle_set_dmf_display[%d][%d] flag[%d:%d]\n", type, autodisplay, g_st_DMF_Flag, g_si_DMF_Flag);
	if(type == 0){
		g_Caption_AutoDisplay = autodisplay;
		if(g_Caption_AutoDisplay == 1){
			subtitle_data_enable(1);
		}
		g_st_DMF_Flag = 0;
	}else if(type == 1){
		g_Superimpose_AutoDisplay = autodisplay;
		if(g_Superimpose_AutoDisplay == 1){
			superimpose_data_enable(1);
		}
		g_si_DMF_Flag = 0;
	}
	subtitle_set_dmf_flag(1);
}

void subtitle_set_dmf_flag(int set)
{
	g_DMF_Flag = set;
}

int subtitle_get_dmf_flag(void)
{
	return g_DMF_Flag;
}

int subtitle_get_dmf_st_flag(void)
{
	return g_Caption_AutoDisplay;
}

int subtitle_get_dmf_si_flag(void)
{
	return g_Superimpose_AutoDisplay;
}

void subtitle_set_st_change_onoff_flag(int set)
{
	g_st_changed_onoff = set;
}

int subtitle_get_st_change_onoff_flag(void)
{
	return g_st_changed_onoff;
}

void subtitle_set_si_change_onoff_flag(int set)
{
	g_si_changed_onoff = set;
}

int subtitle_get_si_change_onoff_flag(void)
{
	return g_si_changed_onoff;
}

void subtitle_set_force_clear(int set)
{
	g_force_clear = set;
}

int subtitle_get_force_clear(void)
{
	return g_force_clear;
}

/**********************************************************************************
SUBTITLE/SUPERIMPOSE/PNG MEMCPY ROUTINES
***********************************************************************************/
void subtitle_data_memcpy()
{
	SUB_SYS_INFO_TYPE *pInfo = (SUB_SYS_INFO_TYPE*)get_sub_context();
	unsigned int i, j, x, y, w, h;
	unsigned int *pSrc = NULL, *pDst = NULL, *pTgt = NULL;

	x = pInfo->res.disp_x;
	y = pInfo->res.disp_y;
	w = pInfo->res.disp_w;
	h = pInfo->res.disp_h;

	if((x==0)&&(y==0)&&(w==0)&&(h==0)) {
		return;
	}

	if(pthread_mutex_trylock (&g_subtitle_mutex) != 0) {
		ALOGE("[%s] pthread_mutex_trylock failed\n", __func__);
		return;
	}

	pSrc = (unsigned int *)pInfo->subtitle_addr;
	pDst = (unsigned int *)pInfo->subtitle_pixels;

	if((pSrc == NULL) || (pDst == NULL)){
		pthread_mutex_unlock (&g_subtitle_mutex);
		return;
	}
#if 0
	if((pInfo->isdbt_type == ONESEG_ISDB_T)||(pInfo->isdbt_type == ONESEG_SBTVD_T)){
		for(i=0; i<h; i++)
		{
			pTgt = pDst + ((i + y) * 960) + x;
			for(j=0; j<w; j++){
				*(pTgt+j) = *pSrc++;
			}
		}
	}else {
		if((pDst != NULL) && (pSrc != NULL)){
			memcpy(pDst, pSrc, (sizeof(int)*pInfo->subtitle_size));
		}
	}
#else
	memcpy(pDst, pSrc, (sizeof(int)*pInfo->subtitle_size));
#endif

	pthread_mutex_unlock (&g_subtitle_mutex);
}

void superimpose_data_memcpy()
{
	SUB_SYS_INFO_TYPE *pInfo = (SUB_SYS_INFO_TYPE*)get_sub_context();
	unsigned int i, j, x, y, w, h;
	unsigned int *pSrc = NULL, *pDst = NULL, *pTgt = NULL;

	x = pInfo->res.disp_x;
	y = pInfo->res.disp_y;
	w = pInfo->res.disp_w;
	h = pInfo->res.disp_h;

	if((x==0)&&(y==0)&&(w==0)&&(h==0)) {
		return;
	}

	if(pthread_mutex_trylock (&g_superimpose_mutex) != 0) {
		ALOGE("[%s] pthread_mutex_trylock failed\n", __func__);
		return;
	}

	pSrc = (unsigned int *)pInfo->superimpose_addr;
	pDst = (unsigned int *)pInfo->superimpose_pixels;

	if((pSrc == NULL) || (pDst == NULL)){
		pthread_mutex_unlock (&g_superimpose_mutex);
		return;
	}

	if((pInfo->isdbt_type == ONESEG_ISDB_T)||(pInfo->isdbt_type == ONESEG_SBTVD_T)){
		for(i=0; i<h; i++)
		{
			pTgt = pDst + ((i + y) * 960) + x;
			for(j=0; j<w; j++){
				*(pTgt+j) = *pSrc++;
			}
		}
	}
	else {
		memcpy(pDst, pSrc, (sizeof(int)*pInfo->superimpose_size));
	}

	pthread_mutex_unlock (&g_superimpose_mutex);
}

void subtitle_data_sw_mixer(int src0_handle, int src1_handle, int dst_handle, int dst_x, int dst_y, int src_w, int src_h)
{
	unsigned int *pSrc0 = NULL, *pSrc1 = NULL, *pDst = NULL;
	unsigned int buf_w=0, buf_h=0, buf_size=0;
	int i=0, j=0, x=0, y=0;
	unsigned int sy_pos=0, dy_pos=0;

	if(src0_handle == src1_handle){
		DISP_DBG("[ERROR] (%d,%d)handle conflicted or need more memory.\n", src0_handle, src1_handle);
		return -1;
	}

	if(src0_handle < -1 || src1_handle < -1){
		DISP_DBG("[ERROR] (%d,%d) invalid handle\n", src0_handle, src1_handle);
		return -1;
	}

	if(subtitle_display_get_memory_sync_flag() == 0){
		return -1;
	}

	buf_w = src_w = subtitle_memory_sub_width();
	buf_h = src_h = subtitle_memory_sub_height();
	buf_size = buf_w * buf_h;
	if(buf_size == 0){
		DISP_DBG("[%s] buffer size is zero\n", __func__);
		return -1;
	}

	if(!subtitle_display_get_enable_flag() && subtitle_get_dmf_st_flag() == 0){
		src0_handle = -1;
	}
	
	if(!superimpose_display_get_enable_flag() && subtitle_get_dmf_si_flag() == 0){
		src1_handle = -1;
	}

	DISP_DBG("[%s] st(%d) + si(%d) => mix(%d), \n", __func__, src0_handle, src1_handle, dst_handle);

	if(dst_handle == -1){
		DISP_DBG("[%s] dst_handle failed\n", __func__);
		return;
	}

	if(dst_handle != -1){
		pDst = subtitle_memory_get_vaddr(dst_handle);
		if(pDst != NULL){
			memset((void*)pDst, 0x0, (sizeof(int)*buf_size));
		}
	}

	if(pthread_mutex_trylock (&g_mixer_mutex) != 0) {
		DISP_DBG("[%s] pthread_mutex_trylock failed\n", __func__);
		return;
	}


	if(src0_handle != -1)
	{
		pSrc0 = (unsigned int *)subtitle_memory_get_vaddr(src0_handle);
		if((pSrc0 == NULL) || (pDst == NULL)){
			pthread_mutex_unlock (&g_mixer_mutex);
			return;
		}
		
		for(i=0; i<src_h ; i++){
			sy_pos = (i * buf_w);
			dy_pos = ((dst_y + i) * buf_w) + dst_x;
			for(j=0; j<src_w ; j++){
				*(pDst+dy_pos+j) |= *(pSrc0+sy_pos+j);
			}
		}
	}

	if(src1_handle != -1)
	{
		pSrc1 = (unsigned int *)subtitle_memory_get_vaddr(src1_handle);
		if((pSrc1 == NULL) || (pDst == NULL)){
			pthread_mutex_unlock (&g_mixer_mutex);
			return;
		}
		
		for(x=0; x<src_h ; x++){
			sy_pos = (x * buf_w);
			dy_pos = ((dst_y + x) * buf_w) + dst_x;
			for(y=0; y<src_w ; y++){
				*(pDst+dy_pos+y) |= *(pSrc1+sy_pos+y);
			}
		}
	}

	pthread_mutex_unlock (&g_mixer_mutex);
}

void png_data_sw_mixer(int src0_handle, int src1_handle, int dst_handle, int dst_x, int dst_y, int src_w, int src_h)
{
	unsigned int *pSrc0 = NULL, *pSrc1 = NULL, *pDst = NULL;
	unsigned int buf_w=0, buf_h=0, buf_size=0;
	int i=0, j=0, x=0, y=0;
	unsigned int sy_pos=0, dy_pos=0;

	if(src0_handle == src1_handle){
		DISP_DBG("[ERROR] (%d,%d)handle conflicted or need more memory.\n", src0_handle, src1_handle);
		return -1;
	}

	if(src0_handle < -1 || src1_handle < -1){
		DISP_DBG("[ERROR] (%d,%d) invalid handle\n", src0_handle, src1_handle);
		return -1;
	}

	if(subtitle_display_get_memory_sync_flag() == 0){
		return -1;
	}

	buf_w = src_w = subtitle_memory_sub_width();
	buf_h = src_h = subtitle_memory_sub_height();
	buf_size = buf_w * buf_h;
	if(buf_size == 0){
		DISP_DBG("[%s] buffer size is zero\n", __func__);
		return -1;
	}

	if(!subtitle_display_get_enable_flag() && subtitle_get_dmf_st_flag() == 0){
		src0_handle = -1;
	}
	
	if(!superimpose_display_get_enable_flag() && subtitle_get_dmf_si_flag() == 0){
		src1_handle = -1;
	}

	DISP_DBG("[%s] st(%d) + si(%d) => mix(%d), \n", __func__, src0_handle, src1_handle, dst_handle);

	if(dst_handle == -1){
		DISP_DBG("[%s] dst_handle failed\n", __func__);
		return;
	}

	if(dst_handle != -1){
		pDst = subtitle_memory_get_vaddr(dst_handle);
		if(pDst != NULL){
			memset((void*)pDst, 0x0, (sizeof(int)*buf_size));
		}
	}

	if(pthread_mutex_trylock (&g_bitmap_mutex) != 0) {
		DISP_DBG("[%s] pthread_mutex_trylock failed\n", __func__);
		return;
	}


	if(src0_handle != -1)
	{
		pSrc0 = (unsigned int *)subtitle_memory_get_vaddr(src0_handle);
		if((pSrc0 == NULL) || (pDst == NULL)){
			pthread_mutex_unlock (&g_bitmap_mutex);
			return;
		}
		
		for(i=0; i<src_h ; i++){
			sy_pos = (i * buf_w);
			dy_pos = ((dst_y + i) * buf_w) + dst_x;
			for(j=0; j<src_w ; j++){
				*(pDst+dy_pos+j) |= *(pSrc0+sy_pos+j);
			}
		}
	}

	if(src1_handle != -1)
	{
		pSrc1 = (unsigned int *)subtitle_memory_get_vaddr(src1_handle);
		if((pSrc1 == NULL) || (pDst == NULL)){
			pthread_mutex_unlock (&g_bitmap_mutex);
			return;
		}
		
		for(x=0; x<src_h ; x++){
			sy_pos = (x * buf_w);
			dy_pos = ((dst_y + x) * buf_w) + dst_x;
			for(y=0; y<src_w ; y++){
				*(pDst+dy_pos+y) |= *(pSrc1+sy_pos+y);
			}
		}
	}

	pthread_mutex_unlock (&g_bitmap_mutex);
}

void png_data_mixer(int png_handle, unsigned int *pDst, int dst_x, int dst_y, int src_w, int src_h)
{
	//ALOGE("===> png_data_mixer png_size[%d:%d] coordinate[%d:%d]\n", src_w, src_h, dst_x, dst_y);
	unsigned int *pSrc = NULL;
	int buf_w=0;
	int i=0, j=0;
	unsigned int sy_pos=0, dy_pos=0;

	buf_w = subtitle_memory_sub_width();
	if(buf_w == 0){
		LOGE("[%s] buffer width is zero\n", __func__);
		return;
	}

	if(pthread_mutex_trylock (&g_bitmap_mutex) != 0) {
		ALOGE("[%s] pthread_mutex_trylock failed\n", __func__);
		return;
	}

	if(png_handle != -1)
	{
		pSrc = (unsigned int *)subtitle_memory_get_vaddr(png_handle);
		subtitle_memory_put_handle(png_handle);
		png_handle = -1;
		if((pSrc == NULL) || (pDst == NULL)){
			pthread_mutex_unlock (&g_bitmap_mutex);
			return;
		}

		//src_w, src_h: original png image size.
		for(i=0; i<src_h ; i++){
			sy_pos = (i * buf_w);
			dy_pos = ((dst_y + i) * buf_w) + dst_x;
			for(j=0; j<src_w ; j++){
				*(pDst+dy_pos+j) |= *(pSrc+sy_pos+j);
			}
		}
	}

	pthread_mutex_unlock (&g_bitmap_mutex);
}

void png_data_memcpy()
{
	SUB_SYS_INFO_TYPE *pInfo = (SUB_SYS_INFO_TYPE*)get_sub_context();
	unsigned int *pSrc = NULL, *pDst = NULL;

	if(pthread_mutex_trylock (&g_png_mutex) != 0) {
		ALOGE("[%s] pthread_mutex_trylock failed\n", __func__);
		return;
	}

	pSrc = (unsigned int *)pInfo->png_addr;
	pDst = (unsigned int *)pInfo->png_pixels;

	if((pSrc == NULL) || (pDst == NULL)) {
		pthread_mutex_unlock (&g_png_mutex);
		return;
	}

	memcpy(pDst, pSrc, (sizeof(int)*pInfo->png_size));

	pthread_mutex_unlock (&g_png_mutex);
}

void subtitle_display_mix_memcpy_ex(unsigned int *pSrc, unsigned int *pDst)
{
	unsigned int buf_w=0, buf_h=0, buf_size=0;

	buf_w = subtitle_memory_sub_width();
	buf_h = subtitle_memory_sub_height();
	buf_size = buf_w * buf_h;

	if(buf_size == 0){
		LOGE("[%s] buffer size is zero\n", __func__);
		return;
	}

	if((pSrc == NULL) || (pDst == NULL))
		return;

	memcpy(pDst, pSrc, sizeof(int)*buf_size);
}

void subtitle_display_memcpy_ex
(
	unsigned int *pSrc, int src_x, int src_y, int src_w, int src_h, int src_pitch_w,
	unsigned int *pDst, int dst_x, int dst_y, int dst_pitch_w
)
{
	int i, j;
	int src_index=0, dst_index=0;
	unsigned int sy_pos=0, dy_pos=0;

	if((pSrc == NULL) || (pDst == NULL))
		return;

	for(i=0; i<src_h ; i++){
		sy_pos = ((src_y + i) * src_pitch_w) + src_x;
		dy_pos = ((dst_y + i) * dst_pitch_w) + dst_x;

		for(j=0; j<src_w ; j++){
			*(pDst+dy_pos + j) = *(pSrc+sy_pos + j);
		}
	}
}
/**********************************************************************************
SUBTITLE/SUPERIMPOSE/PNG UPDATE ROUTINES
***********************************************************************************/
void mixed_subtitle_update(void)
{
	int subt[3];
	int internal_sound_subtitle_index = -1;

	SUB_SYS_INFO_TYPE *pInfo = (SUB_SYS_INFO_TYPE*)get_sub_context();
/* Noah, To avoid CodeSonar warning. 'get_sub_context' returns &g_sub_ctx.sys_info that has always memory address, not NULL because it is not dynimic memory.
	if(pInfo == NULL){
		ALOGE("[mixed_subtitle_update] pInfo is null");
		return;
	}
*/
	if(subtitle_display_get_enable_flag() == 0 && superimpose_display_get_enable_flag() == 0){
		;
	}else{
		g_all_clear = 0;
	}

	ALOGE("[%s] subtitle_get_dmf_flag[%d] auto[%d:%d] visible[%d:%d]", __func__, \
		subtitle_get_dmf_flag(), g_Caption_AutoDisplay, g_Superimpose_AutoDisplay, subtitle_display_get_enable_flag(), superimpose_display_get_enable_flag());


	//After DMF parsing, resetting of subtitle ON/OFF.
	if(subtitle_get_dmf_flag() == 1){
		if(g_Caption_AutoDisplay == 1){
			if(subtitle_display_get_enable_flag() == 0 && g_st_DMF_Flag == 0){
				subtitle_display_set_enable_flag(1);
				g_st_DMF_Flag = 1;
				ALOGE("Caption ON/OFF resetting");
			}
		}

		if(g_Superimpose_AutoDisplay == 1){
			if(superimpose_display_get_enable_flag() == 0 && g_si_DMF_Flag == 0){
				superimpose_display_set_enable_flag(1);
				g_si_DMF_Flag = 1;
				ALOGE("Superimpose ON/OFF resetting");
			}
		}
	}

	ALOGE("[%s] update_sync_drawing[%d] visible[%d:%d] clear[%d:%d] allclear[%d] auto[%d:%d] handle[%d:%d] change[%d:%d] memory[%d]", __func__, \
	update_sync_drawing, subtitle_display_get_enable_flag(), superimpose_display_get_enable_flag(),
	subtitle_display_clear_st_get(), subtitle_display_clear_si_get(), g_all_clear, g_Caption_AutoDisplay, g_Superimpose_AutoDisplay, \
	subtitle_current_get_st_handle(), subtitle_current_get_si_handle(), subtitle_get_st_change_onoff_flag(), subtitle_get_si_change_onoff_flag(), \
	subtitle_display_get_memory_sync_flag());

	//case1: caption off
	if(subtitle_display_get_enable_flag() == 0 && subtitle_display_clear_st_get() == 2) {
		//first clear update, from second no update
		if(superimpose_display_get_enable_flag() == 1 && subtitle_current_get_si_handle() != -1){
			//test stream: TS20ST002_020b, 72, st:-1, st:auto
			update_sync_drawing = 1;
		}else if(superimpose_display_get_enable_flag() == 0 && g_all_clear != 1){
			update_sync_drawing = 1;
		}else{
			ALOGE("CASE-1 return");
			update_sync_drawing = 0;
			return;
		}
	}

	//case2: caption off, superimpose on, si handle = -1
	if(subtitle_display_get_enable_flag() == 0 && superimpose_display_get_enable_flag() == 1 && subtitle_current_get_si_handle() == -1 && subtitle_get_st_change_onoff_flag() == 0){
		update_sync_drawing = 0;
		ALOGE("CASE-2 (caption off, superimpose on, si handle = -1) return");
		return;
	}

	//case3: superimpose off
	if(superimpose_display_get_enable_flag() == 0 && subtitle_display_clear_si_get() == 2){
		//first clear update, from second no update
		if(subtitle_display_get_enable_flag() == 1 && subtitle_current_get_st_handle() != -1){
			update_sync_drawing = 1;
		}else if(subtitle_display_get_enable_flag() == 0 && g_all_clear != 1){
			update_sync_drawing = 1;
		}else{
			update_sync_drawing = 0;
			ALOGE("CASE-3 return");
			return;
		}
	}

	//case4: superimpose off, caption on, st handle = -1
	if(superimpose_display_get_enable_flag() == 0 && subtitle_display_get_enable_flag() == 1 && subtitle_current_get_st_handle() == -1 && subtitle_get_si_change_onoff_flag() == 0){
		update_sync_drawing = 0;
		ALOGE("CASE-4(superimpose off, caption on, st handle = -1) return");
		return;
	}

	//case5: all off
	if(subtitle_display_get_enable_flag() == 0 && superimpose_display_get_enable_flag() == 0 && g_all_clear == 1){
		update_sync_drawing = 0;
		ALOGE("CASE-5(all off) return");
		return;
	}

	//case6: invalid handle
	if(subtitle_current_get_st_handle() < -1 || subtitle_current_get_si_handle() < -1){
		update_sync_drawing = 0;
		ALOGE("CASE-6(Invalid handle:(%d,%d)) return", subtitle_current_get_st_handle(), subtitle_current_get_si_handle());
		return;
	}

	if(internal_sound_index[0] != -1){
		internal_sound_subtitle_index = internal_sound_index[0];
	}else if(internal_sound_index[1] != -1){
		internal_sound_subtitle_index = internal_sound_index[1];
	}

	if(pInfo->mixed_subtitle_phy_addr == NULL || pInfo->mixed_subtitle_phy_addr == 0x1){
		ALOGE("mixed_subtitle_phy_addr is fault[%p]", pInfo->mixed_subtitle_phy_addr);
		return;
	}

	subt[0] = internal_sound_subtitle_index;
	subt[1] = (int)&pInfo->mixed_subtitle_phy_addr;
	subt[2] = (pInfo->subtitle_size*4);

	if(subt[2] > 0 && subtitle_display_get_memory_sync_flag() == 1){
		update_sync_drawing = 1;
		TCCDxBEvent_SubtitleUpdateLinux((void *)subt);
	}
	if(subtitle_display_get_enable_flag() == 0 && superimpose_display_get_enable_flag() == 0){
		g_all_clear = 1;
	}else{
		if(subtitle_display_get_enable_flag() == 0){
			subtitle_display_clear_st_set(2);
		}
		if(superimpose_display_get_enable_flag() == 0){
			subtitle_display_clear_si_set(2);
		}
		g_all_clear = 0;
	}
	internal_sound_index[0] = -1;
	internal_sound_index[1] = -1;
}

void subtitle_update(int update_flag)
{
// David, To avoid Codesonar's warning, Buffer Overrun
// pArg[5] parameter is not recieved from subtitle_update()-function
// subtitle_update()-function do not use arg[5], and this function is not used recently
// This function is also not used, so disable this function
/*
	int subt[5];
	if(subtitle_display_get_enable_flag() == 0) {
		update_flag =0;
		if(g_Caption_AutoDisplay == 1)
			update_flag =1;
	}

	subt[0] = 0;
	subt[1] = update_flag;
	subt[2] = internal_sound_index[0];
	subt[3] = subtitle_get_res_changed();
	subt[4] = g_Caption_AutoDisplay;
	TCCDxBEvent_SubtitleUpdate((void *)subt);
	internal_sound_index[0] = -1;
*/
}

void subtitle_data_update(int handle)
{
	//ALOGE("--->[subtitle_data_update]");
	SUB_SYS_INFO_TYPE *pInfo = (SUB_SYS_INFO_TYPE*)get_sub_context();
	unsigned int buf_w=0, buf_h=0, buf_size=0;
	unsigned int *vir_addr;

	vir_addr = subtitle_memory_get_vaddr(handle);
	buf_w = subtitle_memory_sub_width();
	buf_h = subtitle_memory_sub_height();
	buf_size = buf_w * buf_h;

	if(buf_size == 0){
		LOGE("[%s] buffer size is zero\n", __func__);
		return;
	}

	pthread_mutex_lock (&g_subtitle_mutex);
	pInfo->subtitle_addr = vir_addr;
	pInfo->subtitle_size = buf_size;
	pthread_mutex_unlock (&g_subtitle_mutex);
}

void superimpose_data_update(int handle)
{
	//ALOGE("--->[superimpose_data_update]");
	SUB_SYS_INFO_TYPE *pInfo = (SUB_SYS_INFO_TYPE*)get_sub_context();
	unsigned int  buf_w=0, buf_h=0, buf_size=0;
	unsigned int *vir_addr;

	vir_addr = subtitle_memory_get_vaddr(handle);
	buf_w = subtitle_memory_sub_width();
	buf_h = subtitle_memory_sub_height();
	buf_size = buf_w * buf_h;

	if(buf_size == 0){
		LOGE("[%s] buffer size is zero\n", __func__);
		return;
	}

	pthread_mutex_lock (&g_superimpose_mutex);
	pInfo->superimpose_addr = vir_addr;
	pInfo->superimpose_size = buf_size;
	pthread_mutex_unlock (&g_superimpose_mutex);
}

void png_data_update(int handle)
{
	//ALOGE("--->[png_data_update]");
	SUB_SYS_INFO_TYPE *pInfo = (SUB_SYS_INFO_TYPE*)get_sub_context();
	unsigned int  buf_w=0, buf_h=0, buf_size=0;
	unsigned int *vir_addr;

	vir_addr = subtitle_memory_get_vaddr(handle);
	buf_w = subtitle_memory_sub_width();
	buf_h = subtitle_memory_sub_height();
	buf_size = buf_w * buf_h;

	if(buf_size == 0){
		LOGE("[%s] buffer size is zero\n", __func__);
		return;
	}

	pthread_mutex_lock (&g_png_mutex);
	pInfo->png_addr = vir_addr;
	pInfo->png_size = buf_size;
	pthread_mutex_unlock (&g_png_mutex);
}

void subtitle_set_type(int data_type){
	caption_type = data_type;
}

int subtitle_get_type(int *data_type, int *seg_type){
	*data_type = caption_type;
	*seg_type = dtv_type;

	return 0;
}

void subtitle_get_internal_sound(int index){
	internal_sound_index[caption_type] = index;
}

void subtitle_display_clear_screen(int type, DISP_CTX_TYPE *pSt, DISP_CTX_TYPE *pSi)
{
	DISP_DBG("[%s]\n", __func__);
	int tmp_handle = -1;
	unsigned int *phy_addr = NULL, *vir_addr = NULL, *vir_flash_addr = NULL;
	int width=0, height=0;

	width = subtitle_memory_sub_width();
	height = subtitle_memory_sub_height();

	if((width == 0) || (height == 0)){
		return;
	}

	switch(type)
	{
		case -1:
		{
			/* Both clear */
			if(pSt->handle != -1){
				vir_addr = subtitle_memory_get_vaddr(pSt->handle);
				if(vir_addr != NULL){
					memset(vir_addr, 0x0, sizeof(int)*width*height);
				}
			}

			if(pSt->flash_handle != -1){
				vir_flash_addr = subtitle_memory_get_vaddr(pSt->flash_handle);
				if(vir_flash_addr != NULL){
					memset(vir_flash_addr, 0x0, sizeof(int)*width*height);
				}
			}

			if(pSi->handle != -1){
				vir_addr = subtitle_memory_get_vaddr(pSi->handle);
				if(vir_addr != NULL){
					memset(vir_addr, 0x0, sizeof(int)*width*height);
				}
			}

			if(pSi->flash_handle != -1){
				vir_flash_addr = subtitle_memory_get_vaddr(pSi->flash_handle);
				if(vir_flash_addr != NULL){
					memset(vir_flash_addr, 0x0, sizeof(int)*width*height);
				}
			}

			if((pSt->handle != -1) || (pSi->handle != -1)){
				tmp_handle = subtitle_memory_get_handle();
				if(tmp_handle != -1){
					phy_addr = (unsigned int*)subtitle_memory_get_paddr(tmp_handle);
					vir_addr = subtitle_memory_get_vaddr(tmp_handle);
					#if defined(HAVE_LINUX_PLATFORM)
					#if defined(USE_HW_MIXER)
					 subtitle_display_mixer(pSt->handle, pSt->x, pSt->y, pSt->w, pSt->h,
                                            pSi->handle, pSi->x, pSi->y, pSi->w, pSi->h,
                                            tmp_handle,
											1);
					#else
					subtitle_data_sw_mixer(pSt->handle, pSi->handle, tmp_handle, 0, 0, pSt->w, pSt->h);
					#endif
					subtitle_display_update(tmp_handle);
					#endif

					subtitle_memory_put_handle(tmp_handle);
				}
			}
			break;
		}

		case 0:
		{
			/* st clear */
			if(pSt->handle != -1){
				phy_addr = subtitle_memory_get_paddr(pSt->handle);
				vir_addr = subtitle_memory_get_vaddr(pSt->handle);
				if(vir_addr != NULL){
					memset(vir_addr, 0x0, sizeof(int)*width*height);
				}
			}

			if(pSt->flash_handle != -1){
				vir_flash_addr = subtitle_memory_get_vaddr(pSt->flash_handle);
				if(vir_flash_addr != NULL){
					memset(vir_flash_addr, 0x0, sizeof(int)*width*height);
				}
			}

			if(pSi->handle != -1){
				phy_addr = subtitle_memory_get_paddr(pSi->handle);
				vir_addr = subtitle_memory_get_vaddr(pSi->handle);
//				#if defined(HAVE_LINUX_PLATFORM)
//				#if defined(SUBTITLE_CCFB_DISPLAY_ENABLE)
				subtitle_display_update(pSi->handle);
//				#endif
//				#endif
			}else if(pSt->handle != -1){
//				#if defined(HAVE_LINUX_PLATFORM)
//				#if defined(SUBTITLE_CCFB_DISPLAY_ENABLE)
				subtitle_display_update(pSt->handle);
//				#endif
//				#endif
			}
			break;
		}

		case 1:
		{
			/* si clear */
			if(pSi->handle != -1){
				phy_addr = subtitle_memory_get_paddr(pSi->handle);
				vir_addr = subtitle_memory_get_vaddr(pSi->handle);
				if(vir_addr != NULL){
					memset(vir_addr, 0x0, sizeof(int)*width*height);
				}
			}

			if(pSi->flash_handle != -1){
				vir_flash_addr = subtitle_memory_get_vaddr(pSi->flash_handle);
				if(vir_flash_addr != NULL){
					memset(vir_flash_addr, 0x0, sizeof(int)*width*height);
				}
			}

			if(pSt->handle != -1){
				phy_addr = subtitle_memory_get_paddr(pSt->handle);
				vir_addr = subtitle_memory_get_vaddr(pSt->handle);
//				#if defined(HAVE_LINUX_PLATFORM)
//				#if defined(SUBTITLE_CCFB_DISPLAY_ENABLE)
				subtitle_display_update(pSt->handle);
//				#endif
//				#endif
			}else if(pSi->handle != -1){
//				#if defined(HAVE_LINUX_PLATFORM)
//				#if defined(SUBTITLE_CCFB_DISPLAY_ENABLE)
				subtitle_display_update(pSi->handle);
//				#endif
//				#endif
			}
			break;
		}
	}
}

/**********************************************************************************
CCFB DRAW ROUTINES
***********************************************************************************/
int subtitle_display_get_enable_flag(void)
{
	int enable = 0;
	pthread_mutex_lock (&g_caption_enable_mutex);
	enable = g_SubtitleVisible;
	pthread_mutex_unlock (&g_caption_enable_mutex);
	return enable;
}

int superimpose_display_get_enable_flag(void)
{
	int enable = 0;
	pthread_mutex_lock (&g_super_enable_mutex);
	enable = g_SuperimposeVisible;
	pthread_mutex_unlock (&g_super_enable_mutex);
	return enable;
}

void subtitle_display_set_enable_flag(unsigned int set)
{
	pthread_mutex_lock (&g_caption_enable_mutex);
	g_SubtitleVisible = set;
	pthread_mutex_unlock (&g_caption_enable_mutex);
}

void superimpose_display_set_enable_flag(unsigned int set)
{
	pthread_mutex_lock (&g_super_enable_mutex);
	g_SuperimposeVisible = set;
	pthread_mutex_unlock (&g_super_enable_mutex);
}


int subtitle_display_get_ccfb_fd(void)
{
	return g_ccfb_fd;
}

void subtitle_display_set_ccfb_fd(int fd)
{
	g_ccfb_fd = fd;
}

int subtitle_display_get_wmixer_fd(void)
{
	DISP_DBG("[%s]\n", __func__);
	return g_wmixer_fd;
}
/*
int subtitle_display_get_scaler_fd(void)
{
	return g_scaler_fd;
}

void subtitle_display_set_scaler_fd(int fd)
{
	g_scaler_fd = fd;
}

int subtitle_display_scaler_open(void)
{
	DISP_DBG("[%s]\n", __func__);
	int scaler_fd = -1;
	if ((scaler_fd = open(DEV_SCALER, O_RDWR)) < 0){
		LOGE("[%s] can't open '%s'", __func__, DEV_SCALER);
		return -1;
	}

	subtitle_display_set_scaler_fd(scaler_fd);

	return 0;
}

int subtitle_display_scaler_close(void)
{
	DISP_DBG("[%s]\n", __func__);
	int scaler_fd = -1;

	scaler_fd = subtitle_display_get_scaler_fd();
	if(scaler_fd >= 0) close(scaler_fd);

	return 0;
}
*/
void subtitle_display_set_wmixer_fd(int fd)
{
	DISP_DBG("[%s]\n", __func__);
	g_wmixer_fd = fd;
}

int subtitle_display_wmixer_open(void)
{
	DISP_DBG("[%s]\n", __func__);
	int wmixer_fd = -1;

	if(subtitle_display_get_wmixer_fd() == -1){
		if ((wmixer_fd = open(DEV_WMIXER, O_RDWR)) < 0){
			LOGE("[%s] can't open '%s'", __func__, DEV_WMIXER);
			return -1;
		}

		subtitle_display_set_wmixer_fd(wmixer_fd);
	}else{
		LOGE("[subtitle_display_wmixer_open] multi(%d)\n", subtitle_display_get_wmixer_fd());
	}

	return 0;
}

int subtitle_display_wmixer_close(void)
{
    pthread_mutex_lock (&g_mixer_mutex);

	DISP_DBG("[%s]\n", __func__);
	int wmixer_fd = -1;

	wmixer_fd = subtitle_display_get_wmixer_fd();
	if(wmixer_fd >= 0) {
		close(wmixer_fd);
		subtitle_display_set_wmixer_fd(-1);
	}

    pthread_mutex_unlock (&g_mixer_mutex);
	return 0;
}

int subtitle_display_open(int lcdc, int dx, int dy, int dw, int dh, int dm, int mw, int mh)
{
	int ret = -1, handle = -1;
	int status;

	ret = (g_sub_op_ftn.pOpen)(lcdc, dx, dy, dw, dh, dm, mw, mh);
	if(ret) return -1;

	return 0;
}

void subtitle_display_clear(void)
{
	DISP_DBG("[%s]\n", __func__);
	if(g_sub_op_ftn.pClear){ (g_sub_op_ftn.pClear)(); }
}

int subtitle_display_update(int handle)
{
	DISP_DBG("[%s] handle[%d]\n", __func__, handle);
	int ret = -1;
	unsigned int disp_condition = 0;
	unsigned int *phy_addr, *vir_addr;
	unsigned int buf_w=0, buf_h=0, buf_size=0;
	SUB_SYS_INFO_TYPE *pInfo = (SUB_SYS_INFO_TYPE*)get_sub_context();

	phy_addr = subtitle_memory_get_paddr(handle);
	vir_addr = subtitle_memory_get_vaddr(handle);

	DISP_DBG("[%s] PHY[%p]VIR[%p]\n", __func__, subtitle_memory_get_paddr(handle), subtitle_memory_get_vaddr(handle));

	buf_w = subtitle_memory_sub_width();
	buf_h = subtitle_memory_sub_height();
	buf_size = buf_w * buf_h;

	if(phy_addr == NULL){
		ALOGE("subtitle_display_update phy_addr NULL");
		return -1;
	}

	if(buf_size == 0){
		LOGE("[%s] buffer size is zero\n", __func__);
		return -2;
	}

	if((pInfo->isdbt_type == ONESEG_ISDB_T)||(pInfo->isdbt_type == ONESEG_SBTVD_T)){
		if(pInfo->disp_mode == (int)SUB_DISP_H_320X60){
			disp_condition =1;
		}else{
			LOGE("[%s] PES Drop in ONESEG\n", __func__);
		}
	}else if((pInfo->isdbt_type == FULLSEG_ISDB_T)||(pInfo->isdbt_type == FULLSEG_SBTVD_T)){
		if((pInfo->disp_mode == (int)SUB_DISP_H_960X540)
			||(pInfo->disp_mode == (int)SUB_DISP_V_960X540)
			||(pInfo->disp_mode == (int)SUB_DISP_H_720X480)
			||(pInfo->disp_mode == (int)SUB_DISP_V_720X480)){
			disp_condition =1;
		}else{
			LOGE("[%s] PES Drop in FULLSEG\n", __func__);
		}
	}

	if(subtitle_display_get_thread_flag() == 0){
		return -3;
	}

	if(subtitle_display_get_memory_sync_flag() == 0){
		ALOGE("[%s] memory not prepared", __func__);
		return -4;
	}

	if(disp_condition){
		pInfo->subtitle_size = buf_size;
		pInfo->mixed_subtitle_phy_addr = phy_addr;
		mixed_subtitle_update();
		DISP_DBG("[%s] [%p]\n", __func__, phy_addr);
		#if defined(SUBTITLE_CCFB_DISPLAY_ENABLE)
		(g_sub_op_ftn.pUpdate)(phy_addr);
		#endif
	}

	return 0;
}

int subtitle_display_init_ccfb(int arch_type)
{
#if defined(SUBTITLE_CCFB_DISPLAY_ENABLE)
	int ccfb_fd = -1;
	SUB_SYS_INFO_TYPE *pInfo = get_sub_context();
	

	if ((ccfb_fd = open(DEV_CCFB, O_RDWR)) < 0)
	{
		LOGE("[%s] can't open '%s' [%d]", __func__, DEV_CCFB, ccfb_fd);
		return -1;
	}

	subtitle_display_set_ccfb_fd(ccfb_fd);
#endif
	return 0;
}

#if defined(HAVE_LINUX_PLATFORM)
int subtitle_display_open_ccfb(int act_lcdc, int dx, int dy, int dw, int dh, int dm, int mw, int mh)
{
#if defined(SUBTITLE_CCFB_DISPLAY_ENABLE)
	DISP_DBG("[%s]\n", __func__);
	int ccfb_fd = -1;
	ccfb_config_t	ccfb_cfg;
	SUB_SYS_INFO_TYPE *pInfo = get_sub_context();

	//LOGD("[%s] %d, %d, %d, %d, %d, %d, %d, %d\n", __func__, act_lcdc, dx, dy, dw, dh, dm, mw, mh);
	if(pInfo->res.fb_type == 0){
		ccfb_fd = subtitle_display_get_ccfb_fd();
		if(ccfb_fd == -1){
			LOGE("[%s] ccfb_fd is not initialized yet!!!\n", __func__);
			return -1;
		}

		ccfb_cfg.curLcdc = act_lcdc;
		ccfb_cfg.res.disp_fmt = CCFB_IMG_FMT_ARGB888;
		ccfb_cfg.res.disp_x = dx;
		ccfb_cfg.res.disp_y = dy;
		ccfb_cfg.res.disp_w = dw;
		ccfb_cfg.res.disp_h = dh;
		ccfb_cfg.res.disp_m = dm;
		ccfb_cfg.res.mem_w = mw;
		ccfb_cfg.res.mem_h = mh;
		ccfb_cfg.res.chroma_en = 0;
		ioctl(ccfb_fd, CCFB_SET_CONFIG, &ccfb_cfg);
	}
#endif
	return 0;
}
#endif

int subtitle_display_close_ccfb(void)
{
#if defined(SUBTITLE_CCFB_DISPLAY_ENABLE)
	DISP_DBG("[%s]\n", __func__);
	int is_lcd_disable = 0;
	int ccfb_fd = -1;
	SUB_SYS_INFO_TYPE *pInfo = get_sub_context();

	ccfb_fd = subtitle_display_get_ccfb_fd();
	if(ccfb_fd>=0) close(ccfb_fd);

	subtitle_display_set_ccfb_fd(-1);
#endif
	return 0;
}

void subtitle_display_clear_st_set(int clear)
{
	g_st_clear	= clear;
}

int subtitle_display_clear_st_get(void)
{
	return g_st_clear;
}

void subtitle_display_clear_si_set(int clear)
{
	g_si_clear	= clear;
}

int subtitle_display_clear_si_get(void)
{
	return g_si_clear;
}

int subtitle_display_clear_ccfb(void)
{
	DISP_DBG("[%s]\n", __func__);
	int ccfb_fd = -1, ret, handle;
	unsigned int width, height;
	unsigned int *p = NULL;
	SUB_SYS_INFO_TYPE *pInfo = get_sub_context();

	if(pInfo->res.fb_type == 0){
		#if defined(SUBTITLE_CCFB_DISPLAY_ENABLE)
		ccfb_fd = subtitle_display_get_ccfb_fd();
		if(ccfb_fd == -1){
			LOGE("ccfb_fd is not initialized yet!\n");
			return -1;
		}
		#endif

		handle = subtitle_memory_get_clear_handle();
		ALOGE("[%s] clear_handle[%d]\n", __func__, handle);
		if(handle != -1 && subtitle_display_get_memory_sync_flag() == 1){
			p = subtitle_memory_get_paddr(handle);
			if(subtitle_display_get_thread_flag() == 1){
				if(subtitle_display_clear_st_get()==1){
					subtitle_display_update(handle);
				}
				if(subtitle_display_clear_si_get()==1){
					subtitle_display_update(handle);
				}

				//After scan, subtitle force clear
				if(subtitle_get_force_clear()==1){
					ALOGE("[%s] force clear", __func__);
					subtitle_display_update(handle);
				}
			}

			#if defined(SUBTITLE_CCFB_DISPLAY_ENABLE)
			if(update_sync_drawing == 1){
				ioctl(ccfb_fd, CCFB_DISP_UPDATE, &p);
				LOGE("[%s] physical address[%p]\n", __func__, p);
			}
			#endif
		}
	}
	return 0;
}

#if defined(SUBTITLE_CCFB_DISPLAY_ENABLE)
int subtitle_display_update_ccfb(unsigned int *pAddr)
{
	DISP_DBG("[%s]\n", __func__);
	int ccfb_fd = -1;
	SUB_SYS_INFO_TYPE *pInfo = get_sub_context();

	if(pAddr == 0){
		LOGE("[%s] WARNING - Try to display NULL pointer\n", __func__);
		return -1;
	}

	if(subtitle_display_get_enable_flag() == 0 && superimpose_display_get_enable_flag() == 0)
		return -1;

	if(update_sync_drawing == 0)
		return -1;

	if(pInfo->res.fb_type == 0){
		ccfb_fd = subtitle_display_get_ccfb_fd();
		if(ccfb_fd == -1){
			LOGE("[%s] ccfb_fd is not initialized yet!\n", __func__);
			return -1;
		}
		if(update_sync_drawing == 1){
			ioctl(ccfb_fd, CCFB_DISP_UPDATE, (void*)&pAddr);
			LOGE("[%s] physical address[%p]\n", __func__, pAddr);
		}
	}
	return 0;
}
#endif

#if 0
int subtitle_display_get_state_ccfb(void)
{
	int ccfb_fd;
	unsigned long state;

	ccfb_fd = subtitle_display_get_ccfb_fd();
	if(ccfb_fd == -1){
		LOGE("ccfb_fd is not initialized yet!\n");
		return -1;
	}

	ioctl(ccfb_fd, CCFB_GET_CONFIG, &state);
	return (int) state;
}
#endif

int subtitle_display_enable_ccfb(int enable)
{
	int fb_fd, ccfb_fd;
	unsigned int is_lcd_disable;
	SUB_SYS_INFO_TYPE *pInfo = get_sub_context();

	DISP_DBG("-->> %s (%d)\n", __func__, enable);

	if(pInfo->res.fb_type == 0){
		/* Do not enable both ch2 and ch1 layer at the same time. */
		if(enable){
			ccfb_fd = subtitle_display_get_ccfb_fd();
			if(ccfb_fd == -1){
				LOGE("ccfb_fd is not initialized yet!\n");
				return -1;
			}
			#if defined(SUBTITLE_CCFB_DISPLAY_ENABLE)
			ioctl(ccfb_fd, CCFB_DISP_ENABLE, NULL);
			#endif
		}else{
			ccfb_fd = subtitle_display_get_ccfb_fd();
			if(ccfb_fd == -1){
				LOGE("ccfb_fd is not initialized yet!\n");
				return -1;
			}
			#if defined(SUBTITLE_CCFB_DISPLAY_ENABLE)
			ioctl(ccfb_fd, CCFB_DISP_DISABLE, NULL);
			#endif
		}
	}

	return 0;
}

/**********************************************************************************
For debug (backtrace)
***********************************************************************************/
void print_backtrace(void)
{
	void *frame_addrs[16];
	char **frame_strings;
	size_t backtrace_size;
	int i;

	backtrace_size = backtrace(frame_addrs, 16);
	frame_strings = backtrace_symbols(frame_addrs, backtrace_size);

	for(i=0;i<backtrace_size;i++)
	{
		printf("%d : [0x%x]%s\n", i, frame_addrs[i], frame_strings[i]);
	}
	free(frame_strings);
}
