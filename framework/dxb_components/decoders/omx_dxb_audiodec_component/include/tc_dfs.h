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
#ifndef __TC_DFS_H__
#define __TC_DFS_H__

typedef unsigned char       u8;   /* 1 byte */
typedef signed char         s8;   /* 1 byte */
typedef unsigned short      u16;   /* 2 bytes */
typedef signed short        s16;   /* 2 bytes */
typedef unsigned long       u32;   /* 4 bytes */
typedef signed long         s32;   /* 4 bytes */
typedef signed long long    s64;   /* 8 bytes */
typedef unsigned long long  u64;   /* 8 bytes */

enum e_tc_dfs {
	TC_DFS_FAIL = -2,
	TC_DFS_ERROR = -1,
	TC_DFS_WAIT = 0,
	TC_DFS_FOUND = 1,
        TC_DFS_INTERVAL_PROCESS = 2
};

typedef struct _dfs_conf_t {
	u32 samples;
	u32 p_samples;
	u32 fs;
	s32 distance;
	float peak_ratio;
} dfs_conf_t;

typedef struct _dfs_out_t {
	s32 distance;
	s32 vol_ratio;
	u32 result;
	u32 peak_ratio;
	s32 timegap;
	float max_mag_ratio;
	u32 peak_ratio_threshold;
} dfs_out_t;

u32 tc_dfs_init(dfs_conf_t _conf, float search_range, u32 op_mode);
s32 tc_dfs_proc(u32 _handle, s16 * _dab_left, u32 _dab_samples, s16 *  _fm_left, u32 _fm_samples);
dfs_out_t * tc_dfs_get(u32 _handle, u32 _channel, u32 _order);
void tc_dfs_term(u32 _handle);
void tc_dfs_reinit(u32 _handle);

#endif
