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
#ifndef _TCC_DXB_INTERFACE_VIDEO_H_
#define _TCC_DXB_INTERFACE_VIDEO_H_
#include "tcc_dxb_interface_type.h"

/**
 * Define video's notifying event
 */
typedef	enum
{
    DxB_VIDEO_NOTIFY_FIRSTFRAME_DISPLAYED,
    DxB_VIDEO_NOTIFY_CAPTURE_DONE,
    DxB_VIDEO_NOTIFY_USER_DATA_AVAILABLE,
    DxB_VIDEO_NOTIFY_VIDEO_DEFINITION_UPDATE,
    DxB_VIDEO_NOTIFY_VIDEO_ERROR,
	DxB_VIDEO_NOTIFY_EVT_END
} DxB_VIDEO_NOTIFY_EVT;

typedef void (*pfnDxB_VIDEO_EVENT_Notify)(DxB_VIDEO_NOTIFY_EVT nEvent, void *pCallbackData, void *pUserData);


/********************************************************************************************/
/********************************************************************************************
						FOR MW LAYER FUNCTION
********************************************************************************************/
/********************************************************************************************/
DxB_ERR_CODE TCC_DxB_VIDEO_Init(DxBInterface *hInterface);
DxB_ERR_CODE TCC_DxB_VIDEO_Deinit(DxBInterface *hInterface);
DxB_ERR_CODE TCC_DxB_VIDEO_Start(DxBInterface *hInterface, UINT32 ulDevId, UINT32 ulVideoFormat);
DxB_ERR_CODE TCC_DxB_VIDEO_Stop(DxBInterface *hInterface, UINT32 ulDevId);
DxB_ERR_CODE TCC_DxB_VIDEO_Pause(DxBInterface *hInterface, UINT32 ulDevId, BOOLEAN bOn);
DxB_ERR_CODE TCC_DxB_VIDEO_Flush(DxBInterface *hInterface);
DxB_ERR_CODE TCC_DxB_VIDEO_StartCapture(DxBInterface *hInterface, UINT32 ulDevId, UINT16 usTargetWidth, UINT16 usTargetHeight, UINT8 *pucFileName);

DxB_ERR_CODE TCC_DxB_VIDEO_SelectDisplayOutput(DxBInterface *hInterface, UINT32 ulDevId);
DxB_ERR_CODE TCC_DxB_VIDEO_RefreshDisplay(DxBInterface *hInterface, UINT32 ulDevId);
DxB_ERR_CODE TCC_DxB_VIDEO_IsSupportCountry(DxBInterface *hInterface, UINT32 ulDevId, UINT32 ulCountry);
//DxB_ERR_CODE TCC_DxB_VIDEO_EnableDisplay(DxBInterface *hInterface, UINT32 ulDevId);
//DxB_ERR_CODE TCC_DxB_VIDEO_DisableDisplay(DxBInterface *hInterface, UINT32 ulDevId);
DxB_ERR_CODE TCC_DxB_VIDEO_LCDUpdate(DxBInterface *hInterface, UINT32 ulContentsType);
DxB_ERR_CODE TCC_DxB_VIDEO_GetVideoInfo(DxBInterface *hInterface, UINT32 ulDevId, UINT32 *videoWidth, UINT32 *videoHeight);
DxB_ERR_CODE TCC_DxB_VIDEO_RegisterEventCallback(DxBInterface *hInterface, pfnDxB_VIDEO_EVENT_Notify pfnEventCallback, void *pUserData);
DxB_ERR_CODE TCC_DxB_VIDEO_SetPause(DxBInterface *hInterface, UINT32 ulDevId, UINT32 pause);
DxB_ERR_CODE TCC_DxB_VIDEO_IFrameSearchEnable (DxBInterface *hInterface, UINT32 ulDevId);
DxB_ERR_CODE TCC_DxB_VIDEO_SetSinkByPass(DxBInterface *hInterface, UINT32 ulDevId, UINT32 sink);
DxB_ERR_CODE TCC_DxB_VIDEO_SendEvent(DxBInterface *hInterface, DxB_VIDEO_NOTIFY_EVT nEvent, void *pCallbackData);
DxB_ERR_CODE TCC_DxB_VIDEO_InitSurface(DxBInterface *hInterface, UINT32 ulDevId, int arg);
DxB_ERR_CODE TCC_DxB_VIDEO_DeinitSurface(DxBInterface *hInterface, UINT32 ulDevId);
DxB_ERR_CODE TCC_DxB_VIDEO_SetSurface(DxBInterface *hInterface, UINT32 ulDevId, void *nativeWidow);
DxB_ERR_CODE TCC_DxB_VIDEO_UseSurface(DxBInterface *hInterface);
DxB_ERR_CODE TCC_DxB_VIDEO_ReleaseSurface(DxBInterface *hInterface);
DxB_ERR_CODE TCC_DxB_VIDEO_Use(DxBInterface *hInterface, INT32 arg);
DxB_ERR_CODE TCC_DxB_VIDEO_Release(DxBInterface *hInterface);
DxB_ERR_CODE TCC_DxB_VIDEO_Subtitle(DxBInterface *hInterface, void *arg);
DxB_ERR_CODE TCC_DxB_VIDEO_SupportFieldDecoding(DxBInterface *hInterface, UINT32 OnOff);
DxB_ERR_CODE TCC_DxB_VIDEO_SupportIFrameSearch(DxBInterface *hInterface, UINT32 OnOff);
DxB_ERR_CODE TCC_DxB_VIDEO_SupportUsingErrorMB(DxBInterface *hInterface, UINT32 OnOff);
DxB_ERR_CODE TCC_DxB_VIDEO_SupportDirectDisplay(DxBInterface *hInterface, UINT32 OnOff);
unsigned int TCC_DxB_VIDEO_GetDisplayFlag(DxBInterface *hInterface);
DxB_ERR_CODE TCC_DxB_VIDEO_SetProprietaryData (DxBInterface *hInterface, UINT32 channel_index);
DxB_ERR_CODE TCC_DxB_VIDEO_SetVpuReset(DxBInterface *hInterface, UINT32 ulDevId);
DxB_ERR_CODE TCC_DxB_VIDEO_SetActiveMode(DxBInterface *hInterface, UINT32 ulDevId, UINT32 activemode);
DxB_ERR_CODE TCC_DxB_VIDEO_ServiceIDDisableDisplay(DxBInterface *hInterface, UINT32 check_flag);
DxB_ERR_CODE TCC_DxB_VIDEO_GetDisplayedFirstFrame(DxBInterface *hInterface, UINT32 ulDevId, INT32 *displayedfirstframe);
DxB_ERR_CODE TCC_DxB_VIDEO_SetFirstFrameByPass(DxBInterface *hInterface, UINT32 OnOff);
DxB_ERR_CODE TCC_DxB_VIDEO_SetFirstFrameAfterSeek(DxBInterface *hInterface, UINT32 OnOff);
DxB_ERR_CODE TCC_DxB_VIDEO_SetFrameDropFlag(DxBInterface *hInterface, UINT32 check_flag);
DxB_ERR_CODE TCC_DxB_VIDEO_SetSTCFunction(DxBInterface *hInterface, void *pfnGetSTCFunc, void *pvApp);
DxB_ERR_CODE TCC_DxB_VIDEO_CtrlLastFrame(DxBInterface *hInterface, UINT32 OnOff);
DxB_ERR_CODE TCC_DxB_VIDEO_SetDisplayPosition(DxBInterface *hInterface, INT32 iPosX, INT32 iPosY, INT32 iWidth, INT32 iHeight);
#endif

