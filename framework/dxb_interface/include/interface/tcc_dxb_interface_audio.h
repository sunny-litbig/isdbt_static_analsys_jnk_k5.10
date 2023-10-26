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
#ifndef _TCC_DXB_INTERFACE_AUDIO_H_
#define _TCC_DXB_INTERFACE_AUDIO_H_
#include "tcc_dxb_interface_type.h"

typedef enum
{
	DxB_AUDIO_DUAL_Stereo,
	DxB_AUDIO_DUAL_LeftOnly,
	DxB_AUDIO_DUAL_RightOnly,
	DxB_AUDIO_DUAL_Mix
} DxB_AUDIO_STEREO_MODE;

typedef enum
{
	DxB_AUDIO_NOTIFY_FIRSTFRAME_SOUNDED,
	DxB_AUDIO_EVENT_NOTIFY_END
} DxB_AUDIO_NOTIFY_EVT;


typedef void (*pfnDxB_AUDIO_EVENT_Notify)(DxB_AUDIO_NOTIFY_EVT nEvent, void *pEventData, void *pUserData); // pEvent = DxB_AUDIO_CALLBACK_DATA *


/********************************************************************************************/
/********************************************************************************************
						FOR MW LAYER FUNCTION
********************************************************************************************/
/********************************************************************************************/
DxB_ERR_CODE TCC_DxB_AUDIO_Init(DxBInterface *hInterface);
DxB_ERR_CODE TCC_DxB_AUDIO_Deinit(DxBInterface *hInterface);
DxB_ERR_CODE TCC_DxB_AUDIO_Start(DxBInterface *hInterface, UINT32 ulDevId, UINT32 ulAudioFormat);
DxB_ERR_CODE TCC_DxB_AUDIO_Stop(DxBInterface *hInterface, UINT32 ulDevId);
DxB_ERR_CODE TCC_DxB_AUDIO_SetStereo(DxBInterface *hInterface, UINT32 ulDevId, DxB_AUDIO_STEREO_MODE eMode);
//DxB_ERR_CODE TCC_DxB_AUDIO_SetVolume(DxBInterface *hInterface, UINT32 ulDevId, UINT32 iVolume);
DxB_ERR_CODE TCC_DxB_AUDIO_SetVolumeF(DxBInterface *hInterface, UINT32 ulDevId, REAL32 leftVolume, REAL32 rightVolume);
DxB_ERR_CODE TCC_DxB_AUDIO_SetMute(DxBInterface *hInterface, UINT32 ulDevId, BOOLEAN bMute);
DxB_ERR_CODE TCC_DxB_AUDIO_SetAudioStartSyncWithVideo(DxBInterface *hInterface, BOOLEAN bEnable);
DxB_ERR_CODE TCC_DxB_AUDIO_SetAudioPatternToCheckPTSnSTC(DxBInterface *hInterface, int pattern, int waittime_ms, int droptime_ms, int jumptime_ms);
DxB_ERR_CODE TCC_DxB_AUDIO_Flush(DxBInterface *hInterface);
DxB_ERR_CODE TCC_DxB_AUDIO_DelayOutput(DxBInterface *hInterface, UINT32 ulDevId, INT32 delayMs);
DxB_ERR_CODE TCC_DxB_AUDIO_RegisterEventCallback(DxBInterface *hInterface, pfnDxB_AUDIO_EVENT_Notify pfnEventCallback, void *pUserData);
DxB_ERR_CODE TCC_DxB_AUDIO_SetPause(DxBInterface *hInterface, UINT32 ulDevId, UINT32 pause);
DxB_ERR_CODE TCC_DxB_AUDIO_SetSinkByPass(DxBInterface *hInterface, UINT32 ulDevId, UINT32 sink);
DxB_ERR_CODE TCC_DxB_AUDIO_SetAudioInfomation(DxBInterface *hInterface, UINT32 ulDevId, void* pAudioInfo);
DxB_ERR_CODE TCC_DxB_AUDIO_RecordStart(DxBInterface *hInterface, UINT32 ulDevId, UINT8 * pucFileName);
DxB_ERR_CODE TCC_DxB_AUDIO_RecordStop(DxBInterface *hInterface, UINT32 ulDevId);
DxB_ERR_CODE TCC_DxB_AUDIO_SelectOutput (DxBInterface *hInterface, UINT32 ulDevId, UINT32 isEnableAudioOutput);
DxB_ERR_CODE TCC_DxB_AUDIO_IsSupportCountry(DxBInterface *hInterface, UINT32 ulDevId, UINT32 uiCountry);
DxB_ERR_CODE TCC_DxB_AUDIO_StreamRollBack(DxBInterface *hInterface, UINT32 ulDevId);
DxB_ERR_CODE TCC_DxB_AUDIO_SetDualMono(DxBInterface *hInterface, UINT32 ulDevId, int audio_mode);
DxB_ERR_CODE TCC_DxB_AUDIO_GetAudioType(DxBInterface *hInterface, int iDeviceID, int *piNumCh, int *piAudioMode);
DxB_ERR_CODE TCC_DxB_AUDIO_SetAudioStartNotify(DxBInterface *hInterface, UINT32 ulDevId);
DxB_ERR_CODE TCC_DxB_AUDIO_ServiceIDDisableOutput(DxBInterface *hInterface, UINT32 check_flag);
DxB_ERR_CODE TCC_DxB_AUDIO_SetFrameDropFlag(DxBInterface *hInterface, UINT32 check_flag);
DxB_ERR_CODE TCC_DxB_AUDIO_SetSTCFunction(DxBInterface *hInterface, void *pfnGetSTCFunc, void *pvApp);
DxB_ERR_CODE TCC_DxB_AUDIO_setSeamlessSwitchCompensation(DxBInterface *hInterface, INT32 iOnOff, INT32 iInterval, INT32 iStrength, INT32 iNtimes, INT32 iRange, INT32 iGapadjust, INT32 iGapadjust2, INT32 iMuliplier);
DxB_ERR_CODE TCC_DxB_AUDIO_SetProprietaryData (DxBInterface *hInterface, UINT32 channel_index, UINT32 service_id, UINT32 sub_service_id, INT32 dual_mode, INT32 supportPrimary);
DxB_ERR_CODE TCC_DxB_AUDIO_GetSeamlessValue (DxBInterface *hInterface, int *state, int *cval, int *pval);
DxB_ERR_CODE TCC_DxB_AUDIO_SetSwitchSeamless (DxBInterface *hInterface, UINT32 flag);
DxB_ERR_CODE TCC_DxB_AUDIO_CloseAlsaFlag(DxBInterface *hInterface, UINT32 CloseFlag);
#endif

