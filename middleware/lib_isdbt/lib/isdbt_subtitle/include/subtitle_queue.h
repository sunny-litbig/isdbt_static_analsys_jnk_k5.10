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
#ifndef __SUBTITLE_QUEUE_H__
#define __SUBTITLE_QUEUE_H__

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
DEFINITION OF STRUCTURE
****************************************************************************/
extern int subtitle_queue_init(void);
extern void subtitle_queue_exit(void);
extern int subtitle_queue_put(int type, int handle, int x, int y, int w, int h, unsigned long long pts, int flash_handle);
extern int subtitle_queue_put_first(int type, int handle, int x, int y, int w, int h, unsigned long long pts, int flash_handle);
extern int subtitle_queue_get(int type, int *p_handle, int *p_x, int *p_y, int *p_w, int *p_h, unsigned long long *p_pts, int *p_flash_handle);
extern int subtitle_queue_peek(int type, int *p_handle, int *p_x, int *p_y, int *p_w, int *p_h, unsigned long long *p_pts, int *p_flash_handle);
extern int subtitle_queue_remove_all(int type);
extern int subtitle_queue_put_disp(int handle, unsigned long long pts, int png_flag);
extern int subtitle_queue_get_disp(int *p_handle, unsigned long long *p_pts, int *p_png_flag);
extern int subtitle_queue_remove_disp(void);

#ifdef __cplusplus
}
#endif

#endif	/* __SUBTITLE_QUEUE_H__ */

