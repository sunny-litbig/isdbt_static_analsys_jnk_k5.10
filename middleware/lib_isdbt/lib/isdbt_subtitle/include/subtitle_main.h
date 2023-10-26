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
#ifndef __SUBTITLE_MAIN_H__
#define __SUBTITLE_MAIN_H__

#include <ISDBT_Caption.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
	/* 0:ccfb(linux), 1:surface(android) */
	unsigned int fb_type;

	/* video view size in java application */
	unsigned int raw_w;
	unsigned int raw_h;
	unsigned int view_w;
	unsigned int view_h;
	
	/* max resolution is 1920x1080 for display : lcdc */
	unsigned int disp_x;
	unsigned int disp_y;
	unsigned int disp_w;
	unsigned int disp_h;

	/* For LCDC magnification
	  * TCC92xx 
	  * 0:no scale, 1:x2, 2:x3, 3:x4, 4:x8 - Only Up-scale supported 
	  *
	  * TCC93/88xx
	  * 0:no scale, 1:/2, 2:/3, 3:/4, 4-6:rsvd, 7:/8 - Down-scale
	  * 8:rsvd, 9:x2, 10:x3, 11:x4, 12-14:rsvd, 15:x8 - Up-scale
	  */
	unsigned int disp_m;

	/* max resolution is 960x540 for subtitle display : memory */
	unsigned int sub_buf_w;
	unsigned int sub_buf_h;

	double disp_ratio_x;
	double disp_ratio_y;
}SUB_SYS_RES_TYPE;

typedef struct{
	unsigned int 		act_lcdc_num;
	E_DTV_MODE_TYPE		isdbt_type;
	SUB_ARCH_TYPE 		arch_type;
	int 				stb_mode;
	int 				output_mode;
	int 				hdmi_mode;
	int 				hdmi_res;
	int					hdmi_cable_mode;
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE 
	int 				composite_mode;
	int 				component_mode;
#endif
	int					disp_mode;
	int					disp_open;
	int					disp_condition;
	int					disp_sub_condition;
	int					disp_super_condition;	
	unsigned int		subtitle_size;
	unsigned int		superimpose_size;
	unsigned int		png_size;	
	int 				out_res_w;
	int 				out_res_h;
	void*				subtitle_pixels;
	void*				superimpose_pixels;
	void*				png_pixels;	
	unsigned int*		subtitle_addr;	
	unsigned int*		superimpose_addr;
	unsigned int*		png_addr;
	unsigned int*		mixed_subtitle_phy_addr;
	SUB_SYS_RES_TYPE	res;
}SUB_SYS_INFO_TYPE;

typedef struct{
	SUB_SYS_INFO_TYPE sys_info;
	T_SUB_CONTEXT	sub_ctx[2];
}SUBTITLE_CONTEXT_TYPE;

extern SUB_SYS_INFO_TYPE* get_sub_context(void);

extern int subtitle_demux_check_statement_in(void);
extern void subtitle_demux_set_subtitle_lang(int cnt);
extern void subtitle_demux_set_superimpose_lang(int cnt);
extern int subtitle_demux_select_subtitle_lang(int index, int sel_cnt);
extern int subtitle_demux_select_superimpose_lang(int index, int sel_cnt);

extern int subtitle_fb_type();
extern int subtitle_demux_init(int seg_type, int country, int raw_w, int raw_h, int view_w, int view_h, int init);
extern int subtitle_demux_exit(int init);
extern int subtitle_demux_dec(int data_type, unsigned char *pData, unsigned int uiDataSize, unsigned long long iPts);
extern int subtitle_demux_linux_dec(int data_type, unsigned char *pData, unsigned int uiDataSize, unsigned long long iPts);
extern int subtitle_demux_process(void *p_disp_info, int disp_handle, unsigned long long  pts, int flash_flag, int time_flag);
extern int subtitle_demux_linux_process(void *p_disp_info, int disp_handle, unsigned long long pts, int flash_flag, int time_flag);
extern int subtitle_get_num_languages(int data_type);
extern int subtitle_get_lang_info(int data_type, int *NumofLang, unsigned int* subtitleLang1, unsigned int* subtitleLang2);
extern int subtitle_demux_get_statement_in(void);
extern int subtitle_demux_get_handle_info(void);
#ifdef __cplusplus
}
#endif

#endif	/* __SUBTITLE_MAIN_H__ */

