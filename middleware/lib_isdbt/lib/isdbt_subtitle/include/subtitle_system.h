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
#ifndef __SUBTITLE_SYSTEM_H__
#define __SUBTITLE_SYSTEM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <ISDBT_Caption.h>
#include <subtitle_main.h>

/****************************************************************************
DEFINITION OF STRUCTURE
****************************************************************************/

extern void subtitle_system_set_stc_index(int index);
extern int	subtitle_system_get_stc_index(void);
extern unsigned long long subtitle_system_get_systime(void);
extern int subtitle_system_check_noupdate(unsigned long long cur_pts);
extern int subtitle_system_get_disp_info(void *p);
extern int subtitle_system_get_output_disp_info(void *p);
extern int subtitle_system_init(SUB_SYS_INFO_TYPE *p_sys_info, int seg_type, int country, int fb_type, int raw_w, int raw_h, int view_w, int view_h);
extern long long subtitle_system_get_delay(unsigned long long cur_pts);
extern void subtitle_set_res_changed(int flag);
extern int subtitle_get_res_changed(void);
extern unsigned long long subtitle_system_get_stc_delay_time(void);
extern int subtitle_system_get_stc_delay_excute(void);
#ifdef __cplusplus
}
#endif

#endif	/* __SUBTITLE_SYSTEM_H__ */

