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
#ifndef __SUBTITLE_SCALER_H__
#define __SUBTITLE_SCALER_H__

#ifdef __cplusplus
extern "C" {
#endif

extern int subtitle_sw_scaler(unsigned long* srcBuf, int src_w, int src_h, unsigned long* dstBuf, int dst_w, int dst_h);
extern int subtitle_hw_scaler(unsigned int* srcBuf, int src_x, int src_y, int src_w, int src_h, int src_pw, int src_ph, unsigned int* dstBuf, int dst_x, int dst_y, int dst_w, int dst_h, int dst_pw, int dst_ph);

#ifdef __cplusplus
}
#endif

#endif	/* __SUBTITLE_SCALER_H__ */

