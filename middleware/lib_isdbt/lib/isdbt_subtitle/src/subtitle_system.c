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
#define LOG_TAG	"subtitle_system"
#include <utils/Log.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <cutils/properties.h>
#include <tcc_isdbt_manager_demux.h>
#include <tcc_dxb_interface_demux.h>
#include <ISDBT_Caption.h>
#include <subtitle_display.h>
#include <subtitle_system.h>
#include <subtitle_debug.h>
#include <linux/fb.h>


/****************************************************************************
DEFINITION
****************************************************************************/
#define CC_PRINTF	//ALOGE
#define ERR_LOG		ALOGE
#define PROPERTY_MAX 92
#define DEV_FB0 "/dev/graphics/fb0"
/****************************************************************************
DEFINITION OF EXTERNAL VARIABLES
****************************************************************************/
extern DxBInterface *hInterface;

/****************************************************************************
DEFINITION OF GLOBAL VARIABLES
****************************************************************************/
static SUB_SYS_INFO_TYPE	g_system_info;
static int g_stc_index_num = 0;
static int hd_flag = 1;

/****************************************************************************
DEFINITION OF LOCAL FUNCTIONS
****************************************************************************/
void subtitle_system_set_stc_index (int index)
{
	if(index) 	g_stc_index_num = 1;
	else		g_stc_index_num = 0;
}

int	subtitle_system_get_stc_index(void)
{
	int	index;
	index = g_stc_index_num;
	return index;
}

unsigned long long subtitle_system_get_systime(void)
{
	unsigned int uiSTCHigh = 0, uiSTCLow = 0;
	unsigned long long lstc = 0;
	int ret = 0;
	int index = subtitle_system_get_stc_index();

	ret = TCC_DxB_DEMUX_GetSTC(hInterface, &uiSTCHigh, &uiSTCLow, index);
	if(ret == 0){
		lstc = (unsigned long long)uiSTCHigh << 32 | uiSTCLow;
		lstc = lstc/90;
	}

	return lstc;
}

unsigned long long subtitle_system_get_stc_delay_time(void)
{
	unsigned long long stcdelaytime = 0;
	stcdelaytime = TCCDEMUX_Compensation_get_STC_Delay_Time();
	return stcdelaytime;
}

int subtitle_system_get_stc_delay_excute(void)
{
	int enable = 0;
	enable = TCCDEMUX_Compensation_get_Excute();
	return enable;
}

long long subtitle_system_get_delay(unsigned long long cur_pts)
{
	long long idelay = 0;
	unsigned int uiSTCHigh = 0, uiSTCLow = 0;
	unsigned long long lstc = 0;
	int ret = 0;
	int	index = subtitle_system_get_stc_index();

	ret = TCC_DxB_DEMUX_GetSTC(hInterface, &uiSTCHigh, &uiSTCLow, index);
	if(ret == 0){
		lstc = (unsigned long long)uiSTCHigh << 32 | uiSTCLow;
		lstc = lstc/90;
		idelay = cur_pts - lstc;

		//ARIB STD-B24 Time control code: wait time range (min: 0, max: 63sec)
		if(idelay > 63*1000){
			//CC_PRINTF("[%s] the delay is too longer than 63sec.\n", __func__);
			idelay = 0;
		}else if(idelay < 0){
			//CC_PRINTF("[%s] %lld - %lld = %lld", __func__, cur_pts, lstc, idelay);
		}
	}

	return idelay;
}

int subtitle_system_check_noupdate(unsigned long long cur_pts)
{
	int diff = 0, need_clear = 0;
	unsigned int uiSTCHigh = 0, uiSTCLow = 0;
	unsigned long long lstc = 0;
	SUB_SYS_INFO_TYPE *pInfo = NULL;
	int	pcr_index, ret = 0;

	if(cur_pts == 0) return 0;

	pInfo = get_sub_context();

	pcr_index = subtitle_system_get_stc_index();

	ret = TCC_DxB_DEMUX_GetSTC(hInterface, &uiSTCHigh, &uiSTCLow, pcr_index);
	if(ret == 0){
		lstc = (unsigned long long)uiSTCHigh << 32 | uiSTCLow;
		lstc = lstc/90;

		diff = (int)(lstc - cur_pts);

		if((pInfo->isdbt_type == ONESEG_ISDB_T)||(pInfo->isdbt_type == FULLSEG_ISDB_T)){
			need_clear = (diff >= (180*1000))?1:0;	/* 3min */
		}else{
			need_clear = (diff >= (20*1000))?1:0;	/* 20 sec */
		}
	}
	return need_clear;
}

static E_DTV_MODE_TYPE subtitle_system_get_isdbt_type(int seg_type, int country)
{
	E_DTV_MODE_TYPE	isdbt_type;

	if((seg_type == 1) && (country == 0)){
		isdbt_type = ONESEG_ISDB_T;
	}
	else if((seg_type == 1) && (country == 1)){
		isdbt_type = ONESEG_SBTVD_T;
	}
	else if((seg_type == 13) && (country == 0)){
		isdbt_type = FULLSEG_ISDB_T;
	}
	else if((seg_type == 13) && (country == 1)){
		isdbt_type = FULLSEG_SBTVD_T;
	}
	else{
		isdbt_type = SEG_UNKNOWN;
		ERR_LOG("[%s] unknow isdbt_type\n", __func__);
	}

	return isdbt_type;
}

static int subtitle_system_get_lcd_info(void *p)
{
	int margin = 10, diff = 0, fb_fd = -1;
	unsigned int disp_zoom=0;
	unsigned int disp_out_w=0, disp_out_h=0;
	unsigned int disp_sub_w=0, disp_sub_h=0;
	unsigned int disp_sub_x=0, disp_sub_y=0;
	double disp_ratio_w=0.0f, disp_ratio_h=0.0f;
	SUBTITLE_CONTEXT_TYPE *p_sub_ctx = (SUBTITLE_CONTEXT_TYPE*)p;
	SUB_SYS_INFO_TYPE *p_sys_info=NULL;
	struct   fb_var_screeninfo  fbvar;

	if(p_sub_ctx == NULL){
		ERR_LOG("[%s] Null pointer exception.\n", __func__);
		return -1;
	}

	p_sys_info = (SUB_SYS_INFO_TYPE*)&p_sub_ctx->sys_info;

	disp_sub_w = p_sys_info->res.sub_buf_w;
	disp_sub_h = p_sys_info->res.sub_buf_h;

	if(p_sys_info->res.fb_type == 0){
			disp_out_w = p_sub_ctx->sys_info.out_res_w;
			disp_out_h = p_sub_ctx->sys_info.out_res_h;
#if 0 // fixed subtitle plane size (960x540)
			if ((fb_fd = open(DEV_FB0, O_RDWR)) >= 0){
				if ( ioctl( fb_fd, FBIOGET_VSCREENINFO, &fbvar)){
					LOGE( "[%s] FBIOGET_VSCREENINFO error\n", __func__);
					disp_out_w = 800;
					disp_out_h = 480;
				}else{
					disp_out_w = fbvar.xres;
					disp_out_h = fbvar.yres;
					ALOGE("[%s] current LCD size[%d x %d]",  __func__, disp_out_w, disp_out_h);
				}

				close(fb_fd);
			} else {
				LOGE("[%s] can't open '%s'", __func__, DEV_FB0);
				disp_out_w = 800;
				disp_out_h = 480;
			}
#endif
		if(disp_out_w == 0){
			disp_out_w = 960;
			disp_out_h = 540;
		}
	}else{
		disp_out_w = 960;
		disp_out_h = 540;
	}

#if 0

	if((p_sys_info->isdbt_type == ONESEG_ISDB_T)||(p_sys_info->isdbt_type == ONESEG_SBTVD_T)){
		if(p_sys_info->res.fb_type == 1){
			diff = (p_sys_info->res.raw_h - p_sys_info->res.view_h);
		}

		disp_sub_x = (disp_out_w - disp_sub_w)>>1;
		disp_sub_y = (disp_out_h - disp_sub_h) - diff - margin;
		disp_out_w = disp_sub_w;
		disp_out_h = disp_sub_h;

		disp_ratio_w = (double)1.0f;
		disp_ratio_h = (double)1.0f;
	}else{
		if(p_sys_info->res.fb_type == 1){
			if(p_sys_info->res.raw_h != 0){
				diff = (disp_sub_h * (p_sys_info->res.raw_h - p_sys_info->res.view_h)) / p_sys_info->res.raw_h;
				disp_out_h -= diff;
			}
		}

		if(disp_out_w > disp_sub_w){
			disp_sub_x= (disp_out_w - disp_sub_w)>>1;
			disp_sub_y = (disp_out_h - disp_sub_h)>>1;

			disp_out_w = disp_sub_w;
			disp_out_h = disp_sub_h;
		}else{
			disp_sub_x = 0;
			disp_sub_y = 0;
		}

		disp_ratio_w = (double)disp_out_w/(double)disp_sub_w;
		disp_ratio_h = (double)disp_out_h/(double)disp_sub_h;
	}
#else
	if(p_sys_info->res.fb_type == 1){
			if(p_sys_info->res.raw_h != 0){
				diff = (disp_sub_h * (p_sys_info->res.raw_h - p_sys_info->res.view_h)) / p_sys_info->res.raw_h;
				disp_out_h -= diff;
			}
		}

		if(disp_out_w > disp_sub_w){
			disp_sub_x= (disp_out_w - disp_sub_w)>>1;
			disp_sub_y = (disp_out_h - disp_sub_h)>>1;

			disp_out_w = disp_sub_w;
			disp_out_h = disp_sub_h;
		}else{
			disp_sub_x = 0;
			disp_sub_y = 0;
		}

		disp_ratio_w = (double)disp_out_w/(double)disp_sub_w;
		disp_ratio_h = (double)disp_out_h/(double)disp_sub_h;

#endif
	p_sys_info->res.disp_m = 0;//disp_zoom;
	if((p_sys_info->isdbt_type == ONESEG_ISDB_T)||(p_sys_info->isdbt_type == ONESEG_SBTVD_T)){
		p_sys_info->res.disp_x = disp_sub_x;
		p_sys_info->res.disp_y = disp_sub_y;
	}else{
		p_sys_info->res.disp_x = disp_sub_x;
		p_sys_info->res.disp_y = disp_sub_y;
	}

	p_sys_info->res.disp_w = disp_out_w;
	p_sys_info->res.disp_h = disp_out_h;
	p_sys_info->res.disp_ratio_x = disp_ratio_w;
	p_sys_info->res.disp_ratio_y = disp_ratio_h;

	CC_PRINTF("[%s] disp(%d,%d,%d,%d), sub(%d,%d), ratio(%f,%f), m(0x%X), diff:%d ", __func__,
		p_sys_info->res.disp_x, p_sys_info->res.disp_y, p_sys_info->res.disp_w, p_sys_info->res.disp_h,
		p_sys_info->res.sub_buf_w, p_sys_info->res.sub_buf_h,
		p_sys_info->res.disp_ratio_x, p_sys_info->res.disp_ratio_y,
		p_sys_info->res.disp_m, diff);

	return 0;
}

int getfromsysfs(const char *sysfspath, char *val)
{
  int sysfs_fd;
  int read_count;

  if(sysfspath == NULL){
    ERR_LOG("[%s] sysfspath == NULL / Error !!!!!\n", __func__);
    return 0;
  }

  if(val == NULL){
    ERR_LOG("[%s] val == NULL / Error !!!!!\n", __func__);
    return 0;
  }

  sysfs_fd = open(sysfspath, O_RDONLY);
  read_count = read(sysfs_fd, val, PROPERTY_MAX);
  if(0 <= read_count && read_count < PROPERTY_MAX)    // Noah, To avoid Codesonar's warning
    val[read_count] = '\0';
  close(sysfs_fd);
  return read_count;
}

int subtitle_system_get_output_disp_info(void *p)
{
	int output_mode =0, disp_num=0;
	char value[PROPERTY_MAX] = { 0, };
	SUB_SYS_INFO_TYPE *p_sys_info=NULL;
	SUBTITLE_CONTEXT_TYPE *p_sub_ctx=(SUBTITLE_CONTEXT_TYPE*)p;

	if(p_sub_ctx == NULL){
		ERR_LOG("[%s] Null pointer exception.\n", __func__);
		return -1;
	}

	p_sys_info = (SUB_SYS_INFO_TYPE*)&p_sub_ctx->sys_info;

	getfromsysfs("/sys/class/tcc_dispman/tcc_dispman/persist_output_mode", value);
	output_mode = atoi(value);
	p_sub_ctx->sys_info.output_mode = output_mode;

	getfromsysfs("/sys/class/tcc_dispman/tcc_dispman/tcc_output_dispdev_id", value);
	disp_num = atoi(value);
	p_sub_ctx->sys_info.act_lcdc_num = disp_num;

	if(p_sub_ctx->sys_info.output_mode == 0){
		//LCD only
		getfromsysfs("/sys/class/tcc_dispman/tcc_dispman/tcc_output_panel_width", value);
	  	p_sub_ctx->sys_info.out_res_w = atoi(value);
	  	getfromsysfs("/sys/class/tcc_dispman/tcc_dispman/tcc_output_panel_height", value);
	  	p_sub_ctx->sys_info.out_res_h = atoi(value);
	  	if(p_sub_ctx->sys_info.act_lcdc_num == 0)
		  	p_sub_ctx->sys_info.act_lcdc_num = 1;
  	}else if(p_sub_ctx->sys_info.output_mode == 1){
  		//LCD + HDMI , default: LCD
		getfromsysfs("/sys/class/tcc_dispman/tcc_dispman/tcc_output_panel_width", value);
	  	p_sub_ctx->sys_info.out_res_w = atoi(value);
	  	getfromsysfs("/sys/class/tcc_dispman/tcc_dispman/tcc_output_panel_height", value);
	  	p_sub_ctx->sys_info.out_res_h = atoi(value);
	  	if(p_sub_ctx->sys_info.act_lcdc_num == 0)
		  	p_sub_ctx->sys_info.act_lcdc_num = 1;
  	}else{
		//HDMI/CVBS/Component
		getfromsysfs("/sys/class/tcc_dispman/tcc_dispman/tcc_output_dispdev_width", value);
	  	p_sub_ctx->sys_info.out_res_w = atoi(value);
	  	getfromsysfs("/sys/class/tcc_dispman/tcc_dispman/tcc_output_dispdev_height", value);
	  	p_sub_ctx->sys_info.out_res_h = atoi(value);
	  	if(p_sub_ctx->sys_info.act_lcdc_num == 1)
	  		p_sub_ctx->sys_info.act_lcdc_num = 0;
  	}

	CC_PRINTF("[%s] output mode:[%d], out[w:%d, h:%d], lcdc_num[%d] ", __func__, p_sub_ctx->sys_info.output_mode, \
  		p_sub_ctx->sys_info.out_res_w, p_sub_ctx->sys_info.out_res_h, p_sub_ctx->sys_info.act_lcdc_num);

	return 0;
}

int subtitle_system_get_disp_info(void *p)
{
	SUB_SYS_INFO_TYPE *p_sys_info=NULL;
	SUBTITLE_CONTEXT_TYPE *p_sub_ctx=(SUBTITLE_CONTEXT_TYPE*)p;

	if(p_sub_ctx == NULL){
		ERR_LOG("[%s] Null pointer exception.\n", __func__);
		return -1;
	}

	p_sys_info = (SUB_SYS_INFO_TYPE*)&p_sub_ctx->sys_info;

	if((p_sys_info->isdbt_type == FULLSEG_ISDB_T)||(p_sys_info->isdbt_type == FULLSEG_SBTVD_T)){
		if((p_sys_info->disp_mode == SUB_DISP_H_720X480)||\
			(p_sys_info->disp_mode == SUB_DISP_V_720X480)){
			p_sub_ctx->sub_ctx[0].disp_info.sub_plane_w = 720;
			p_sub_ctx->sub_ctx[0].disp_info.sub_plane_h = 480;
			p_sub_ctx->sub_ctx[1].disp_info.sub_plane_w = 720;
			p_sub_ctx->sub_ctx[1].disp_info.sub_plane_h = 480;
			subtitle_set_res_changed(0);
		}else{
			p_sub_ctx->sub_ctx[0].disp_info.sub_plane_w = 960;
			p_sub_ctx->sub_ctx[0].disp_info.sub_plane_h = 540;
			p_sub_ctx->sub_ctx[1].disp_info.sub_plane_w = 960;
			p_sub_ctx->sub_ctx[1].disp_info.sub_plane_h = 540;
			subtitle_set_res_changed(1);
		}
	}

	p_sys_info->res.sub_buf_w = p_sub_ctx->sub_ctx[0].disp_info.sub_plane_w;
	p_sys_info->res.sub_buf_h = p_sub_ctx->sub_ctx[0].disp_info.sub_plane_h;
	p_sys_info->res.sub_buf_w = p_sub_ctx->sub_ctx[1].disp_info.sub_plane_w;
	p_sys_info->res.sub_buf_h = p_sub_ctx->sub_ctx[1].disp_info.sub_plane_h;

	if(p_sys_info->res.fb_type == 0){
		//for display debugging
		#if defined (SUBTITLE_CCFB_DISPLAY_ENABLE)
			subtitle_system_get_output_disp_info(p_sys_info);
		#endif
	}
	subtitle_system_get_lcd_info(p_sys_info);

	p_sub_ctx->sub_ctx[0].disp_info.ratio_x = p_sys_info->res.disp_ratio_x;
	p_sub_ctx->sub_ctx[0].disp_info.ratio_y = p_sys_info->res.disp_ratio_y;
	p_sub_ctx->sub_ctx[1].disp_info.ratio_x = p_sys_info->res.disp_ratio_x;
	p_sub_ctx->sub_ctx[1].disp_info.ratio_y = p_sys_info->res.disp_ratio_y;

	CC_PRINTF("[%s] disp[%d:%d:%d:%d:0x%X], sub_buf[%d,%d], ratio(%f,%f)\n", __func__,
		p_sys_info->res.disp_x, p_sys_info->res.disp_y,
		p_sys_info->res.disp_w, p_sys_info->res.disp_h, p_sys_info->res.disp_m,
		p_sys_info->res.sub_buf_w, p_sys_info->res.sub_buf_h,
		p_sys_info->res.disp_ratio_x, p_sys_info->res.disp_ratio_y);

	return 0;
}

void subtitle_set_res_changed(int flag)
{
	hd_flag = flag;
}

int subtitle_get_res_changed()
{
	return hd_flag;
}

int subtitle_system_init(SUB_SYS_INFO_TYPE *p_sys_info, int seg_type, int country, int fb_type, int raw_w, int raw_h, int view_w, int view_h)
{
//	LOGD("In [%s]\n", __func__);

	int ret = 0;

	if(p_sys_info == NULL){
		ERR_LOG("[%s] Null pointer exception.\n", __func__);
		return -1;
	}

	p_sys_info->output_mode = -1;
	p_sys_info->res.fb_type = fb_type;
	if(p_sys_info->res.fb_type == 1){
		p_sys_info->res.raw_w = raw_w;	// whole screen width
		p_sys_info->res.raw_h = raw_h;		// whole screen height = active screen height + android status bar
		p_sys_info->res.view_w = view_w;	// active screen width
		p_sys_info->res.view_h = view_h;	// active screen height
	}

	p_sys_info->disp_mode = -1;
	p_sys_info->disp_open = 0;

	p_sys_info->isdbt_type = subtitle_system_get_isdbt_type(seg_type, country);
	if(p_sys_info->isdbt_type == SEG_UNKNOWN){
		ERR_LOG("[%s] unknown seg type (%d)\n", __func__, p_sys_info->isdbt_type);
		return -1;
	}

	CC_PRINTF("[%s] raw[%dx%d], view[%dx%d], isdbt_type:%d\n", __func__, \
			raw_w, raw_h, view_w, view_h, p_sys_info->isdbt_type);

//	LOGD("Out [%s]\n", __func__);

	return ret;
}
