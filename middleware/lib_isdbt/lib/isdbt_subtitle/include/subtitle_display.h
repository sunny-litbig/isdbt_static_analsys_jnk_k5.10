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
#ifndef __SUBTITLE_DISPLAY_H__
#define __SUBTITLE_DISPLAY_H__

#ifdef __cplusplus
	extern "C" {
#endif


#include <ISDBT_Caption.h>

//for subtitle display
//#define SUBTITLE_CCFB_DISPLAY_ENABLE

//for hw wmixer
#define USE_HW_MIXER

typedef struct{
	int (*pInit)(int);
	int (*pOpen)(int , int , int , int , int , int , int , int);
	int (*pClose)(void);
	int (*pClear)(void);
	int (*pUpdate)(unsigned int *);
	int (*pEnable)(int);
}SUB_OP_TYPE;

typedef struct{
	int handle;
	int flash_handle;
	int x;
	int y;
	int w;
	int h;
	unsigned long long pts;
}DISP_CTX_TYPE;

typedef struct{
	int handle;
	unsigned long long pts;
	int png_flag;
}PNG_DISP_CTX_TYPE;

extern int subtitle_display_get_cls(int type);
extern void subtitle_display_set_cls(int type, int set);
extern void subtitle_display_set_skip_pts(int skip);
extern int subtitle_display_open_thread(void);
extern int subtitle_display_close_thread(int reinit);

extern int subtitle_display_pre_init(SUB_ARCH_TYPE arch_type, int init);
extern int subtitle_display_init(int data_type, void *p);
extern int subtitle_display_exit(void);
extern int subtitle_display_close(void);

//Visible clear
extern void subtitle_visible_clear(void);

//Caption
extern int subtitle_data_enable(int enable);
extern void subtitle_data_update(int handle);
extern void subtitle_data_memcpy(void);
extern void subtitle_update(int update_flag);
//Superimpose
extern int superimpose_data_enable(int enable);
extern void superimpose_data_update(int handle);
extern void superimpose_data_memcpy(void);
extern void superimpose_update(int update_flag);
//PNG
extern void png_data_enable(int enable);
extern void png_data_update(int handle);
extern void png_data_memcpy(void);

//mixed subtitle
extern void mixed_subtitle_update(void);

//Subtitle internal sound
extern void subtitle_get_internal_sound(int index);
extern void subtitle_set_type(int data_type);
extern int subtitle_get_type(int *data_type, int *seg_type);

//DMF autodisplay setting
extern void subtitle_set_dmf_display(int type, int autodisplay);
extern void subtitle_set_dmf_flag(int set);
extern int subtitle_get_dmf_flag(void);
extern int subtitle_get_dmf_st_flag(void);
extern int subtitle_get_dmf_si_flag(void);


//subtitle clear update
extern void subtitle_display_clear_st_set(int clear);
extern void subtitle_display_clear_si_set(int clear);

extern void subtitle_display_clear_screen(int type, DISP_CTX_TYPE *pSt, DISP_CTX_TYPE *pSi);
extern void png_data_mixer(int png_handle, unsigned int *pDst, int dst_x, int dst_y, int src_w, int src_h);
extern void subtitle_data_sw_mixer(int src0_handle, int src1_handle, int dst_handle, int dst_x, int dst_y, int src_w, int src_h);
extern void png_data_sw_mixer(int src0_handle, int src1_handle, int dst_handle, int dst_x, int dst_y, int src_w, int src_h);
extern void subtitle_display_mix_memcpy_ex(unsigned int *pSrc, unsigned int *pDst);
extern void subtitle_display_memcpy_ex(unsigned int *pSrc, int src_x, int src_y, int src_w, int src_h, int src_pitch_w, unsigned int *pDst, int dst_x, int dst_y, int dst_pitch_w);

//CCFB function
extern int subtitle_display_get_ccfb_fd(void);
extern void subtitle_display_set_ccfb_fd(int fd);
extern int subtitle_display_get_wmixer_fd(void);
extern void subtitle_display_set_wmixer_fd(int fd);
extern int subtitle_display_wmixer_open(void);
extern int subtitle_display_wmixer_close(void);
extern int subtitle_display_init_ccfb(int arch_type);
extern int subtitle_display_open_ccfb(int act_lcdc, int dx, int dy, int dw, int dh, int dm, int mw, int mh);
extern int subtitle_display_close_ccfb(void);
extern int subtitle_display_clear_ccfb(void);
extern int subtitle_display_update_ccfb(unsigned int *pAddr);
extern int subtitle_display_enable_ccfb(int enable);
extern int subtitle_display_open(int lcdc, int dx, int dy, int dw, int dh, int dm, int mw, int mh);
extern int subtitle_display_close(void);
extern void subtitle_display_clear(void);

extern int subtitle_display_get_enable_flag(void);
extern int superimpose_display_get_enable_flag(void);
extern void subtitle_display_set_enable_flag(unsigned int set);
extern void superimpose_display_set_enable_flag(unsigned int set);

extern int subtitle_display_mixer(int st_handle, int st_x, int st_y, int st_w, int st_h, int si_handle, int si_x, int si_y, int si_w, int si_h, int dst_handle, int final_mix);

//memory sync
extern int subtitle_display_set_memory_sync_flag(int set);
extern int subtitle_display_get_memory_sync_flag(void);

#ifdef __cplusplus
}
#endif

#endif	/* __SUBTITLE_DISPLAY_H__ */

