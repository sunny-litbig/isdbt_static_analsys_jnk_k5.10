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
#ifndef __SUBTITLE_QUEUE_POS_H__
#define __SUBTITLE_QUEUE_POS_H__

#ifdef __cplusplus
extern "C" {
#endif
/****************************************************************************
DEFINITION OF STRUCTURE
****************************************************************************/
extern int subtitle_queue_pos_init(void);
extern void subtitle_queue_pos_exit(void);
extern int subtitle_queue_pos_getcount(int type);
extern int subtitle_queue_pos_put(int type, int x, int y, int w, int h);
extern int subtitle_queue_pos_get(int type, int *x, int *y, int *w, int *h);
extern int subtitle_queue_pos_peek_nth(int type, int index, int *x, int *y, int *w, int *h);
extern int subtitle_queue_pos_remove_all(int type);

#ifdef __cplusplus
}
#endif

#endif	/* __SUBTITLE_QUEUE_POS_H__ */
