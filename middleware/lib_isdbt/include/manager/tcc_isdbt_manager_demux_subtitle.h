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

#ifndef __TCC_ISDBT_MANAGER_DEMUX_SUBTITLE_H__
#define __TCC_ISDBT_MANAGER_DEMUX_SUBTITLE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "tcc_dxb_interface_type.h"
#include "tcc_dxb_interface_demux.h"

extern DxB_ERR_CODE tcc_demux_subtitle_notify(UINT8 *pucBuf, UINT32 uiBufSize, UINT64 ullPTS, UINT32 ulRequestId, void *pUserData);
extern DxB_ERR_CODE tcc_demux_superimpose_notify(UINT8 *pucBuf, UINT32 uiBufSize, UINT64 ullPTS, UINT32 ulRequestId, void *pUserData);
extern void* tcc_demux_subtitle_decoder(void *arg);
extern int tcc_manager_demux_subtitle_init(void);
extern int tcc_manager_demux_subtitle_deinit(void);
extern int tcc_manager_demux_subtitle_stop(int init);
extern int tcc_manager_demux_subtitle_play(int iStPID, int iSiPID, int iSegType, int iCountryCode, int iRaw_w, int iRaw_h, int iView_w, int iView_h, int init);
	
#ifdef __cplusplus
}
#endif

#endif 	//__TCC_ISDBT_MANAGER_DEMUX_SUBTITLE_H__