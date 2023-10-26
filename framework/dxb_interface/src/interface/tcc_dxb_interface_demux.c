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
#define LOG_NDEBUG 0
#define LOG_TAG	"DXB_INTERFACE_DEMUX"
#include <utils/Log.h>

#include "globals.h"
#include <OMX_Types.h>
#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_Audio.h>
#include "OMX_Other.h"
#include <OMX_TCC_Index.h>
#include <user_debug_levels.h>
#include <TCCMemory.h>

#include "tcc_dxb_interface_type.h"
#include "tcc_dxb_interface_omxil.h"
#include "tcc_dxb_interface_demux.h"
#include "tcc_dxb_interface_tuner.h"
#include "tcc_dxb_interface_audio.h"
#include "tcc_dxb_interface_video.h"

#ifndef SUPPORT_ALWAYS_IN_OMX_EXECUTING
#define SUPPORT_SMART_STATE_CHANGE
#endif

#define	MAX_FILTER_NUMBER	64
#define DEFAULT_FILTERID_VIDEO1     -10
#define DEFAULT_FILTERID_VIDEO2     -20
#define DEFAULT_FILTERID_AUDIO1     -30
#define DEFAULT_FILTERID_AUDIO2     -40

typedef struct {
	pfnDxB_DEMUX_EVENT_Notify pfnEventCallback;
	void *pUserData;
	pthread_mutex_t tLock;
	int iActiveFilters[MAX_FILTER_NUMBER];
} TCC_DxB_DEMUX_PRIVATE;

//////////////////////////////////////////////////////////
void tcc_demux_lock_init(TCC_DxB_DEMUX_PRIVATE *pPrivate)
{
	pthread_mutex_init(&pPrivate->tLock, NULL);
}

void tcc_demux_lock_deinit(TCC_DxB_DEMUX_PRIVATE *pPrivate)
{
	pthread_mutex_destroy(&pPrivate->tLock);
}

void tcc_demux_lock_on(TCC_DxB_DEMUX_PRIVATE *pPrivate)
{
	pthread_mutex_lock(&pPrivate->tLock);
}
void tcc_demux_lock_off(TCC_DxB_DEMUX_PRIVATE *pPrivate)
{
	pthread_mutex_unlock(&pPrivate->tLock);
}
//////////////////////////////////////////////////////////
#ifdef  SUPPORT_SMART_STATE_CHANGE
void tcc_dxb_demux_init_filter(TCC_DxB_DEMUX_PRIVATE *pPrivate)
{
	int i;
	for (i = 0; i < MAX_FILTER_NUMBER; i++) {
		pPrivate->iActiveFilters[i] = -1;
	}
}

int tcc_dxb_demux_add_filter(TCC_DxB_DEMUX_PRIVATE *pPrivate, int iFilterID)
{
	int i;
	for (i = 0; i < MAX_FILTER_NUMBER; i++) {
		if (pPrivate->iActiveFilters[i] == -1) {
			pPrivate->iActiveFilters[i] = iFilterID;
			break;
		}
	}
	if (i == MAX_FILTER_NUMBER)
		return 1;
	return 0;
}

int tcc_dxb_demux_delete_filter(TCC_DxB_DEMUX_PRIVATE *pPrivate, int iFilterID)
{
	int i;
	for (i = 0; i < MAX_FILTER_NUMBER; i++) {
		if (pPrivate->iActiveFilters[i] == iFilterID) {
			pPrivate->iActiveFilters[i] = -1;
			break;
		}
	}
	if (i == MAX_FILTER_NUMBER)
		return 1;

	return 0;
}

int tcc_dxb_demux_get_number_of_filter(TCC_DxB_DEMUX_PRIVATE *pPrivate)
{
	int ifilternum = 0, i;
	for (i = 0; i < MAX_FILTER_NUMBER; i++) {
		if (pPrivate->iActiveFilters[i] != -1) {
			ifilternum++;
		}
	}
	return ifilternum;
}

int tcc_dxb_demux_do_change_state(TCC_DxB_DEMUX_PRIVATE *pPrivate, Omx_Dxb_Dec_App_Private_Type *AppPriv)
{
	if (tcc_dxb_demux_get_number_of_filter(pPrivate) == 0) {
		if (tcc_omx_stop(pPrivate) == 0) {
			tcc_omx_load_component(pPrivate, COMP_ALL & (~COMP_TUNER));
		} else
			return 1;
	} else {
		if (tcc_omx_execute(AppPriv) != 0)
			return 1;
	}
	return 0;
}
#endif

DxB_ERR_CODE TCC_DxB_DEMUX_Init(DxBInterface *hInterface, DxB_STANDARDS Standard, UINT32 ulDevId, UINT32 ulBaseBandType)
{
#if 0
	TCC_DxB_DEMUX_PRIVATE *pPrivate = TCC_fo_malloc(__func__, __LINE__,sizeof(TCC_DxB_DEMUX_PRIVATE));
#else
	TCC_DxB_DEMUX_PRIVATE *pPrivate = TCC_fo_calloc(__func__, __LINE__, 1, sizeof(TCC_DxB_DEMUX_PRIVATE));
#endif
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv;
	INT32 ierr = 0;
	void *pfnGetSTC = NULL, *pfnPCRCallback, *pfnTSCallback;
	void *pvApp = NULL;

	tcc_demux_lock_init(pPrivate);
#ifdef  SUPPORT_SMART_STATE_CHANGE
	tcc_dxb_demux_init_filter();
#endif

	OMX_Dxb_Dec_AppPriv = tcc_omx_init(hInterface);
	if(OMX_Dxb_Dec_AppPriv == NULL){
		return DxB_ERR_ERROR;
	}

	hInterface->pOpenMaxIL    = OMX_Dxb_Dec_AppPriv;
	hInterface->pDemuxPrivate = pPrivate;

	ierr = tcc_omx_select_demuxer(OMX_Dxb_Dec_AppPriv, Standard);
	if(ierr != OMX_ErrorNone){
		return DxB_ERR_ERROR;
	}
	ierr = tcc_omx_select_baseband(OMX_Dxb_Dec_AppPriv, hInterface->iTunerType);
	if(ierr != OMX_ErrorNone){
		return DxB_ERR_ERROR;
	}
	tcc_omx_set_contents_type(OMX_Dxb_Dec_AppPriv, 2);
	tcc_omx_load_component(OMX_Dxb_Dec_AppPriv, COMP_ALL);

#if 1
	if (OMX_Dxb_Dec_AppPriv->tunerhandle)
	{
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamTunerDeviceSet, &ulDevId);
	}
	if (OMX_Dxb_Dec_AppPriv->demuxhandle)
	{
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamTunerDeviceSet, &ulDevId);
	}
	if (OMX_Dxb_Dec_AppPriv->videohandle)
	{
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->videohandle, OMX_IndexVendorParamTunerDeviceSet, &ulDevId);
	}
	if (OMX_Dxb_Dec_AppPriv->fbdevhandle)
	{
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_IndexVendorParamTunerDeviceSet, &ulDevId);
	}
#endif

#ifdef  SUPPORT_ALWAYS_IN_OMX_EXECUTING
	if (tcc_omx_get_dxb_type(OMX_Dxb_Dec_AppPriv) == DxB_STANDARD_TDMB) {
		ierr = tcc_omx_select_audiovideo_type(OMX_Dxb_Dec_AppPriv, -1, STREAMTYPE_VIDEO_AVCH264);
		if(ierr != OMX_ErrorNone){
			return DxB_ERR_ERROR;
		}
		ierr = tcc_omx_select_audiovideo_type(OMX_Dxb_Dec_AppPriv, STREAMTYPE_AUDIO_BSAC, -1);
		if(ierr != OMX_ErrorNone){
			return DxB_ERR_ERROR;
		}
		if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle)
			ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBStartDemuxing, &ulBaseBandType);
		else
		{
			ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
			ierr = OMX_ErrorInvalidComponent;
		}

		if(ierr != OMX_ErrorNone)
			return DxB_ERR_ERROR;
#if 0
		// alsasink mute config
		// 0 : call audio track mute function
		// 1 : disable pcm output
	#if defined(_USE_AUDIO_TRACK_MUTE_CONTROL_F_) || defined(_NOT_USE_AUDIO_TRACK_MUTE_CONTROL_M_)
		piArg[0] = 0;
	#else
		piArg[0] = 1;
	#endif
		if(OMX_Dxb_Dec_AppPriv->alsasinkhandle)
			ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->alsasinkhandle, OMX_IndexVendorParamAlsaSinkMuteConfig, piArg);
		else
			ierr = OMX_ErrorInvalidComponent;

		if(ierr != OMX_ErrorNone)
			return DxB_ERR_ERROR;
#endif
	}
#endif
	tcc_omx_disable_unused_port(OMX_Dxb_Dec_AppPriv);

#ifdef  SUPPORT_ALWAYS_IN_OMX_EXECUTING
	tcc_omx_execute(OMX_Dxb_Dec_AppPriv);
#endif
	//Set STC Function
	TCC_DxB_DEMUX_GetSTCFunctionPTR(hInterface, &pfnGetSTC, &pvApp);
	TCC_DxB_AUDIO_SetSTCFunction(hInterface, pfnGetSTC, pvApp);
	TCC_DxB_VIDEO_SetSTCFunction(hInterface, pfnGetSTC, pvApp);

#ifdef SUPPORT_PVR
	if(tcc_omx_get_dxb_type(OMX_Dxb_Dec_AppPriv) != DxB_STANDARD_IPTV)
  	{
		TCC_DxB_DEMUX_GetPVRFuncPTR(hInterface, &pfnPCRCallback, &pfnTSCallback, &pvApp);
		TCC_DxB_PVR_SetPVRFunction(hInterface, pfnPCRCallback, pfnTSCallback, pvApp);
	}

#endif

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_DEMUX_Deinit(DxBInterface *hInterface)
{
	TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;

	tcc_demux_lock_on(pPrivate);
#ifdef  SUPPORT_ALWAYS_IN_OMX_EXECUTING
	if (tcc_omx_stop(OMX_Dxb_Dec_AppPriv) == 0) {
		tcc_omx_component_free(OMX_Dxb_Dec_AppPriv, COMP_TUNER);
	}
#else
	tcc_omx_component_free(OMX_Dxb_Dec_AppPriv, COMP_ALL);
#endif

	tcc_demux_lock_off(pPrivate);
	tcc_omx_deinit(OMX_Dxb_Dec_AppPriv);
	tcc_demux_lock_deinit(pPrivate);

	hInterface->pOpenMaxIL = NULL;
	hInterface->pDemuxPrivate = NULL;

	TCC_fo_free(__func__, __LINE__,pPrivate);

	return DxB_ERR_OK;
}

#if 0   // sunny : not use.
DxB_ERR_CODE TCC_DxB_DEMUX_GetNumOfDemux(DxBInterface *hInterface, UINT32 *ulNumOfDemux)
{
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;
	*ulNumOfDemux = 0;
#if 0
	ierr = OMX_GetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBGetNumOfDemux, ulNumOfDemux);
	if (ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
#endif
	return DxB_ERR_OK;
}
#endif

DxB_ERR_CODE TCC_DxB_DEMUX_StartAVPID(DxBInterface *hInterface, DxB_Pid_Info * ppidInfo)
{
	TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;
	INT32       piArg[2];

    if (hInterface->pDemuxPrivate == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pDemuxPrivate NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	if (ppidInfo == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] ppidInfo is NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	tcc_demux_lock_on(pPrivate);

	piArg[0] = (INT32) 0;
	piArg[1] = (INT32) ppidInfo;

	////////////////////
#ifdef      SUPPORT_SMART_STATE_CHANGE
	if (ppidInfo->pidBitmask & PID_BITMASK_VIDEO_MAIN) {
		ALOGE("%s:VideoMain PID[0x%X]", __func__, ppidInfo->usVideoMainPid);
		if (ppidInfo->usVideoMainPid < 0x1fff && ppidInfo->usVideoMainPid)
			tcc_dxb_demux_add_filter(DEFAULT_FILTERID_VIDEO1);
		else
			ppidInfo->pidBitmask &= ~PID_BITMASK_VIDEO_MAIN;
	}
	if (ppidInfo->pidBitmask & PID_BITMASK_VIDEO_SUB) {
		ALOGE("%s:VideoSub PID[0x%X]", __func__, ppidInfo->usVideoSubPid);
		if (ppidInfo->usVideoSubPid < 0x1fff && ppidInfo->usVideoSubPid)
			tcc_dxb_demux_add_filter(DEFAULT_FILTERID_VIDEO2);
		else
			ppidInfo->pidBitmask &= ~PID_BITMASK_VIDEO_SUB;
	}
	if (ppidInfo->pidBitmask & PID_BITMASK_AUDIO_MAIN) {
		ALOGE("%s:AudioMain PID[0x%X]", __func__, ppidInfo->usAudioMainPid);
		if (ppidInfo->usAudioMainPid < 0x1fff && ppidInfo->usAudioMainPid)
			tcc_dxb_demux_add_filter(DEFAULT_FILTERID_AUDIO1);
		else
			ppidInfo->pidBitmask &= ~PID_BITMASK_AUDIO_MAIN;
	}
	if (ppidInfo->pidBitmask & PID_BITMASK_AUDIO_SUB) {
		ALOGE("%s:AudioSub PID[0x%X]", __func__, ppidInfo->usAudioSubPid);
		if (ppidInfo->usAudioSubPid < 0x1fff && ppidInfo->usAudioSubPid)
			tcc_dxb_demux_add_filter(DEFAULT_FILTERID_AUDIO2);
		else
			ppidInfo->pidBitmask &= ~PID_BITMASK_AUDIO_SUB;
	}
	tcc_dxb_demux_do_change_state(hInterface->pDemuxPrivate, OMX_Dxb_Dec_AppPriv);
#endif

	if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBStartPID, piArg);
	else
	{
		ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
		ierr = OMX_ErrorInvalidComponent;
	}

	tcc_demux_lock_off(pPrivate);
	if (ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_DEMUX_StopAVPID(DxBInterface *hInterface, UINT32 pidBitmask)
{
	TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;
	INT32       piArg[2];

    if (hInterface->pDemuxPrivate == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pDemuxPrivate NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	tcc_demux_lock_on(pPrivate);

	piArg[0] = (INT32) 0;
	piArg[1] = (INT32) pidBitmask;

	if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBStopPID, piArg);
	else
	{
		ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
		ierr = OMX_ErrorInvalidComponent;
	}
	if (ierr != OMX_ErrorNone) {
		tcc_demux_lock_off(pPrivate);
		return DxB_ERR_ERROR;
	}

#ifdef      SUPPORT_SMART_STATE_CHANGE
	if (pidBitmask & PID_BITMASK_VIDEO_MAIN) {
		tcc_dxb_demux_delete_filter(DEFAULT_FILTERID_VIDEO1);
	}
	if (pidBitmask & PID_BITMASK_VIDEO_SUB) {
		tcc_dxb_demux_delete_filter(DEFAULT_FILTERID_VIDEO2);
	}
	if (pidBitmask & PID_BITMASK_AUDIO_MAIN) {
		tcc_dxb_demux_delete_filter(DEFAULT_FILTERID_AUDIO1);
	}
	if (pidBitmask & PID_BITMASK_AUDIO_SUB) {
		tcc_dxb_demux_delete_filter(DEFAULT_FILTERID_AUDIO2);
	}
	tcc_dxb_demux_do_change_state(hInterface->pDemuxPrivate, OMX_Dxb_Dec_AppPriv);
#else
	if (pidBitmask & (PID_BITMASK_VIDEO_MAIN | PID_BITMASK_VIDEO_SUB | PID_BITMASK_AUDIO_MAIN | PID_BITMASK_AUDIO_SUB))
		tcc_omx_clear_port_buffer(OMX_Dxb_Dec_AppPriv);
#endif

	tcc_demux_lock_off(pPrivate);
	return DxB_ERR_OK;
}

#if 0   // sunny : not use
DxB_ERR_CODE TCC_DxB_DEMUX_ChangeAVPID(DxBInterface *hInterface, UINT32 pidBitmask, DxB_Pid_Info * ppidInfo)
{
	TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;
	INT32       piStopArg[2];
	INT32       piStartArg[2];

    if (hInterface->pDemuxPrivate == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pDemuxPrivate NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	tcc_demux_lock_on(pPrivate);
	piStopArg[0]	= (INT32)0;
	piStopArg[1]	= (INT32)pidBitmask;

	piStartArg[0]	= (INT32)0;
	piStartArg[1]	= (INT32)ppidInfo;

	if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle)
	{
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBStopPID, piStopArg);
		if (ierr != OMX_ErrorNone) {
        	tcc_demux_lock_off(pPrivate);
		return DxB_ERR_ERROR;
		}
	}
	else
	{
		ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
		tcc_demux_lock_off(pPrivate);
		return OMX_ErrorInvalidComponent;
	}

	if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle)
	{
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBStartPID, piStartArg);
		if (ierr != OMX_ErrorNone ) {
			tcc_demux_lock_off(pPrivate);
			return DxB_ERR_ERROR;
		}
	}
	else
	{
		ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
		tcc_demux_lock_off(pPrivate);
		return OMX_ErrorInvalidComponent;
	}

	tcc_demux_lock_off(pPrivate);
	return DxB_ERR_OK;
}
#endif

DxB_ERR_CODE TCC_DxB_DEMUX_RegisterPESCallback(DxBInterface *hInterface,
		DxB_DEMUX_PESTYPE etPesType,
		pfnDxB_DEMUX_PES_Notify pfnNotify,
		pfnDxB_DEMUX_AllocBuffer pfnAllocBuffer,
		pfnDxB_DEMUX_FreeBufferForError pfnErrorFreeBuffer)
{
	TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;
	INT32       piArg[5];

    if (hInterface->pDemuxPrivate == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pDemuxPrivate NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	tcc_demux_lock_on(pPrivate);

	piArg[0] = 0;
	piArg[1] = (INT32) etPesType;
	piArg[2] = (INT32) pfnNotify;
	piArg[3] = (INT32) pfnAllocBuffer;
	piArg[4] = (INT32) pfnErrorFreeBuffer;

	if(OMX_Dxb_Dec_AppPriv->demuxhandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBRegisterPESCallback, piArg);
	else
	{
		ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
		ierr = OMX_ErrorInvalidComponent;
	}
	tcc_demux_lock_off(pPrivate);
	if (ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_DEMUX_StartPESFilter(DxBInterface *hInterface, UINT16 ulPid, DxB_DEMUX_PESTYPE etPESType, UINT32 ulRequestID, void *pUserData)
{
	TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;
	INT32       piArg[4];

    if (hInterface->pDemuxPrivate == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pDemuxPrivate NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }
	
    if (hInterface->pOpenMaxIL == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	tcc_demux_lock_on(pPrivate);

	piArg[0] = (INT32) pUserData;
	piArg[1] = (INT32) ulPid;
	piArg[2] = (INT32) etPESType;
	piArg[3] = (INT32) ulRequestID;

#ifdef      SUPPORT_SMART_STATE_CHANGE
	tcc_dxb_demux_add_filter(ulRequestID);
	tcc_dxb_demux_do_change_state(hInterface->pDemuxPrivate, OMX_Dxb_Dec_AppPriv);
#endif
	if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBStartPESFilter, piArg);
	else
	{
		ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
		ierr = OMX_ErrorInvalidComponent;
	}
	tcc_demux_lock_off(pPrivate);
	if (ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_DEMUX_StopPESFilter(DxBInterface *hInterface, UINT32 ulRequestID)
{
	TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;
	INT32       piArg[2];

    if (hInterface->pDemuxPrivate == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pDemuxPrivate NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	tcc_demux_lock_on(pPrivate);

	piArg[0] = (INT32) 0;
	piArg[1] = (INT32) ulRequestID;

	if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBStopPESFilter, piArg);
	else
	{
		ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
		ierr = OMX_ErrorInvalidComponent;
	}
	if (ierr != OMX_ErrorNone) {
		tcc_demux_lock_off(pPrivate);
		return DxB_ERR_ERROR;
	}
#ifdef      SUPPORT_SMART_STATE_CHANGE
	tcc_dxb_demux_delete_filter(ulRequestID);
	tcc_dxb_demux_do_change_state(hInterface->pDemuxPrivate, OMX_Dxb_Dec_AppPriv);
#endif
	tcc_demux_lock_off(pPrivate);
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_DEMUX_RegisterSectionCallback(DxBInterface *hInterface, pfnDxB_DEMUX_Notify pfnNotify, pfnDxB_DEMUX_AllocBuffer pfnAllocBuffer)
{
	TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;
	INT32       piArg[3];

    if (hInterface->pDemuxPrivate == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pDemuxPrivate NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	tcc_demux_lock_on(pPrivate);
	DEBUG(DEB_LEV_PARAMS, "In %s\n", __func__);
	piArg[0] = 0;
	piArg[1] = (INT32) pfnNotify;
	piArg[2] = (INT32) pfnAllocBuffer;

	if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBRegisterSectionCallback, piArg);
	else
	{
		ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
		ierr = OMX_ErrorInvalidComponent;
	}

	tcc_demux_lock_off(pPrivate);
	if (ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_DEMUX_StartSectionFilter(DxBInterface *hInterface, UINT16 usPid, UINT32 ulRequestID, BOOLEAN bIsOnce,
		UINT32 ulFilterLen, UINT8 * pucValue, UINT8 * pucMask,
		UINT8 * pucExclusion, BOOLEAN bCheckCRC, void *pUserData)
{
	TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;
	INT32       piArg[9];

    if (hInterface->pDemuxPrivate == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pDemuxPrivate NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }
	
    if (hInterface->pOpenMaxIL == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	tcc_demux_lock_on(pPrivate);
	DEBUG(DEB_LEV_PARAMS, "In %s\n", __func__);
	piArg[0] = (INT32) pUserData;
	piArg[1] = (INT32) usPid;
	piArg[2] = (INT32) ulRequestID;
	piArg[3] = (INT32) bIsOnce;
	piArg[4] = (INT32) ulFilterLen;
	piArg[5] = (INT32) pucValue;
	piArg[6] = (INT32) pucMask;
	piArg[7] = (INT32) pucExclusion;
	piArg[8] = (INT32) bCheckCRC;

#ifdef      SUPPORT_SMART_STATE_CHANGE
	tcc_dxb_demux_add_filter(ulRequestID);
	tcc_dxb_demux_do_change_state(hInterface->pDemuxPrivate, OMX_Dxb_Dec_AppPriv);
#endif
	if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBSetSectionFilter, piArg);
	else
	{
		ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
		ierr = OMX_ErrorInvalidComponent;
	}

	tcc_demux_lock_off(pPrivate);
	if (ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_DEMUX_StopSectionFilter(DxBInterface *hInterface, UINT32 ulRequestID)
{
	if(hInterface == NULL || hInterface->pDemuxPrivate == NULL)
	{
		ALOGE("[%s] hInterface == NULL || hInterface->pDemuxPrivate == NULL Error !!!!!\n", __func__);
		return DxB_ERR_ERROR;
	}

	TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;
	INT32       piArg[2];

	piArg[0] = (INT32) 0;
	piArg[1] = (INT32) ulRequestID;

	tcc_demux_lock_on(pPrivate);
	if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBReleaseSectionFilter, piArg);
	else
	{
		ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
		ierr = OMX_ErrorInvalidComponent;
	}
	if (ierr != OMX_ErrorNone) {
		tcc_demux_lock_off(pPrivate);
		return DxB_ERR_ERROR;
	}
#ifdef      SUPPORT_SMART_STATE_CHANGE
	tcc_dxb_demux_delete_filter(ulRequestID);
	tcc_dxb_demux_do_change_state(hInterface->pDemuxPrivate, OMX_Dxb_Dec_AppPriv);
#endif
	tcc_demux_lock_off(pPrivate);
	return DxB_ERR_OK;
}

#if 0   // sunny : not use
DxB_ERR_CODE TCC_DxB_DEMUX_StartSectionFilterEx(DxBInterface *hInterface, UINT16 usPid, UINT32 ulRequestID, BOOLEAN bIsOnce,
		UINT32 ulFilterLen, UINT8 * pucValue, UINT8 * pucMask,
		UINT8 * pucExclusion, BOOLEAN bCheckCRC,
		pfnDxB_DEMUX_NotifyEx pfnNotify,
		pfnDxB_DEMUX_AllocBuffer pfnAllocBuffer,
		UINT32 *SectionHandle)
{
	TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;
	INT32       piArg[11];

	DEBUG(DEB_LEV_PARAMS, "In %s\n", __func__);

    if (hInterface->pDemuxPrivate == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pDemuxPrivate NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	piArg[0] = (INT32) pPrivate->pUserData;
	piArg[1] = (INT32) usPid;
	piArg[2] = (INT32) ulRequestID;
	piArg[3] = (INT32) bIsOnce;
	piArg[4] = (INT32) ulFilterLen;
	piArg[5] = (INT32) pucValue;
	piArg[6] = (INT32) pucMask;
	piArg[7] = (INT32) pucExclusion;
	piArg[8] = (INT32) bCheckCRC;
	piArg[9] = (INT32) pfnNotify;
	piArg[10] = (INT32) pfnAllocBuffer;

	tcc_demux_lock_on(pPrivate);
	if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBStartSectionFilterEx, piArg);
	else
	{
		ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
		ierr = OMX_ErrorInvalidComponent;
	}
	if (ierr != OMX_ErrorNone) {
		tcc_demux_lock_off(pPrivate);
		return DxB_ERR_ERROR;
	}
	if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle)
		ierr = OMX_GetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBGetSectionHandleEx, SectionHandle);
	else
	{
		ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
		ierr = OMX_ErrorInvalidComponent;
	}
	tcc_demux_lock_off(pPrivate);
	if (ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_DEMUX_StopSectionFilterEx(DxBInterface *hInterface, UINT32 SectionHandle)
{
	TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;
	INT32       piArg[2];

    if (hInterface->pDemuxPrivate == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pDemuxPrivate NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	piArg[0] = (INT32) 0;
	piArg[1] = (UINT32) SectionHandle;

	tcc_demux_lock_on(pPrivate);
	if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBStopSectionFilterEx, piArg);
	else
	{
		ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
		ierr = OMX_ErrorInvalidComponent;
	}
	tcc_demux_lock_off(pPrivate);
	if (ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}
#endif

DxB_ERR_CODE TCC_DxB_DEMUX_GetSTC(DxBInterface *hInterface, UINT32 * pulHighBitSTC, UINT32 * pulLowBitSTC, unsigned int index)
{
	TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;
	INT32       piArg[4];

    if (hInterface->pDemuxPrivate == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pDemuxPrivate NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	piArg[0] = (INT32) 0;
	piArg[1] = (INT32) pulHighBitSTC;
	piArg[2] = (INT32) pulLowBitSTC;
	piArg[3] = (INT32) index;

	tcc_demux_lock_on(pPrivate);
	if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle)
		ierr = OMX_GetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBGetSTC, piArg);
	else
	{
		ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
		ierr = OMX_ErrorInvalidComponent;
	}

	tcc_demux_lock_off(pPrivate);
	if (ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_DEMUX_GetSTCFunctionPTR(DxBInterface *hInterface, void **ppfnGetSTC, void **ppvApp)
{
	TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;
	INT32       piArg[2];

    if (hInterface->pDemuxPrivate == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pDemuxPrivate NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	tcc_demux_lock_on(pPrivate);
	if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle)
		ierr = OMX_GetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBGetSTCFunction, piArg);
	else
	{
		ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
		ierr = OMX_ErrorInvalidComponent;
	}
	if (ierr != OMX_ErrorNone)
	{
		tcc_demux_lock_off(pPrivate);
		return DxB_ERR_ERROR;
	}
	*ppfnGetSTC = (void*) piArg[0];
	*ppvApp = (void*) piArg[1];

	tcc_demux_lock_off(pPrivate);
	return DxB_ERR_OK;
}

#if 0   // sunny : not use
DxB_ERR_CODE TCC_DxB_DEMUX_GetPVRFuncPTR(DxBInterface *hInterface, void **ppfnPCRCallback, void **ppfnTSCallback, void **ppvApp)
{
	TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;
	INT32       piArg[3];

    if (hInterface->pDemuxPrivate == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pDemuxPrivate NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	tcc_demux_lock_on(pPrivate);
	if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle)
		ierr = OMX_GetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBGetPVRFunction, piArg);
	else
	{
		ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
		ierr = OMX_ErrorInvalidComponent;
	}

	if (ierr != OMX_ErrorNone)
	{
		tcc_demux_lock_off(pPrivate);
		return DxB_ERR_ERROR;
	}
	*ppfnPCRCallback = (void*) piArg[0];
	*ppfnTSCallback = (void*) piArg[1];
	*ppvApp = (void*) piArg[2];

	tcc_demux_lock_off(pPrivate);
	return DxB_ERR_OK;
}
#endif

DxB_ERR_CODE TCC_DxB_DEMUX_ResetVideo(DxBInterface *hInterface, UINT32 ulDevId)
{
	TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;

    if (hInterface->pDemuxPrivate == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pDemuxPrivate NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	tcc_demux_lock_on(pPrivate);
	if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBResetVideo, (OMX_PTR) ulDevId);
	else
	{
		ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
		ierr = OMX_ErrorInvalidComponent;
	}

	if (ierr != OMX_ErrorNone)
	{
		tcc_demux_lock_off(pPrivate);
		return DxB_ERR_ERROR;
	}

	tcc_demux_lock_off(pPrivate);
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_DEMUX_ResetAudio(DxBInterface *hInterface, UINT32 ulDevId)
{
	TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;

    if (hInterface->pDemuxPrivate == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pDemuxPrivate NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	tcc_demux_lock_on(pPrivate);
	if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBResetAudio, (OMX_PTR)ulDevId);
	else
	{
		ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
		ierr = OMX_ErrorInvalidComponent;
	}

	if (ierr != OMX_ErrorNone)
	{
		tcc_demux_lock_off(pPrivate);
		return DxB_ERR_ERROR;
	}

	tcc_demux_lock_off(pPrivate);
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_DEMUX_RecordStart(DxBInterface *hInterface, UINT32 ulDevId, UINT8 * pucFileName)
{
	TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;
	INT32       piArg[2];

    if (hInterface->pDemuxPrivate == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pDemuxPrivate NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	tcc_demux_lock_on(pPrivate);
	piArg[0] = (INT32) ulDevId;
	piArg[1] = (INT32) pucFileName;

	if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamRecStartRequest, piArg);
	else
	{
		ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
		ierr = OMX_ErrorInvalidComponent;
	}

	tcc_demux_lock_off(pPrivate);
	if (ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_DEMUX_RecordStop(DxBInterface *hInterface, UINT32 ulDevId)
{
	TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;

    if (hInterface->pDemuxPrivate == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pDemuxPrivate NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	tcc_demux_lock_on(pPrivate);
	if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamRecStopRequest, (OMX_PTR)ulDevId);
	else
	{
		ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
		ierr = OMX_ErrorInvalidComponent;
	}

	tcc_demux_lock_off(pPrivate);
	if (ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_DEMUX_SetParentLock(DxBInterface *hInterface, UINT32 ulDevId, UINT32 uiParentLock)
{
	TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;
	INT32		piArg[2];

    if (hInterface->pDemuxPrivate == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pDemuxPrivate NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	piArg[0] = (INT32) ulDevId;
	piArg[1] = (INT32) uiParentLock;

	tcc_demux_lock_on(pPrivate);
	if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBSetParentLock, piArg);
	else
	{
		ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
		ierr = OMX_ErrorInvalidComponent;
	}
	tcc_demux_lock_off(pPrivate);
	if (ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_DEMUX_SetESType(DxBInterface *hInterface, UINT32 ulDevId, DxB_ELEMENTARYSTREAM ulAudioType, DxB_ELEMENTARYSTREAM ulVideoType)
{
	TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;
	INT32       piArg[2];

    if (hInterface->pDemuxPrivate == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pDemuxPrivate NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	piArg[0] = (INT32) ulAudioType;
	piArg[1] = (INT32) ulVideoType;

	tcc_demux_lock_on(pPrivate);
	if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamSetDemuxAudioVideoStreamType, piArg);
	else
	{
		ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
		ierr = OMX_ErrorInvalidComponent;
	}
	tcc_demux_lock_off(pPrivate);
	if (ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}

#if 0   // sunny : not use
DxB_ERR_CODE TCC_DxB_DEMUX_StartDemuxing(DxBInterface *hInterface, UINT32 ulDevId)
{
	TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;
	INT32       piArg[1];
	void        *pfnGetSTC = NULL;
	void        *pvApp = NULL;

    if (hInterface->pDemuxPrivate == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pDemuxPrivate NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	if (tcc_omx_get_dxb_type(OMX_Dxb_Dec_AppPriv) != DxB_STANDARD_TDMB) {
		return DxB_ERR_ERROR;
	}

	tcc_demux_lock_on(pPrivate);
	piArg[0] = (INT32) ulDevId;
	////////////////////
#ifdef      SUPPORT_SMART_STATE_CHANGE
	tcc_dxb_demux_add_filter(DEFAULT_FILTERID_VIDEO1);
	tcc_dxb_demux_add_filter(DEFAULT_FILTERID_AUDIO1);
	tcc_dxb_demux_do_change_state(hInterface->pDemuxPrivate, OMX_Dxb_Dec_AppPriv);
#else
	tcc_omx_clear_port_buffer(OMX_Dxb_Dec_AppPriv);
#endif
	if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBStartDemuxing, piArg);
	else
	{
		ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
		ierr = OMX_ErrorInvalidComponent;
	}
	//Set STC Function
	TCC_DxB_DEMUX_GetSTCFunctionPTR(hInterface, &pfnGetSTC, &pvApp);
	TCC_DxB_AUDIO_SetSTCFunction(hInterface, pfnGetSTC, pvApp);
	TCC_DxB_VIDEO_SetSTCFunction(hInterface, pfnGetSTC, pvApp);
	tcc_demux_lock_off(pPrivate);
	if (ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_DEMUX_SetActiveMode(DxBInterface *hInterface, UINT32 ulDevId, UINT32 activemode)
{
	TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;
	INT32       piArg[2];

    if (hInterface->pDemuxPrivate == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pDemuxPrivate NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	piArg[0] = (INT32) ulDevId;
	piArg[1] = (INT32) activemode;

	tcc_demux_lock_on(pPrivate);
	if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBActiveModeSet, piArg);
	else
	{
		ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
		ierr = OMX_ErrorInvalidComponent;
	}
	tcc_demux_lock_off(pPrivate);

	if (ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}
#endif

DxB_ERR_CODE TCC_DxB_DEMUX_SetFirstDsiplaySet (DxBInterface *hInterface, UINT32 ulDevId, UINT32 displayflag)
{
	TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;
	INT32       piArg[2];

    if (hInterface->pDemuxPrivate == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pDemuxPrivate NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	piArg[0] = (INT32) ulDevId;
	piArg[1] = (INT32) displayflag;

	tcc_demux_lock_on(pPrivate);
	if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle)
		ierr = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBFirstDisplaySet, piArg);
	else
	{
		ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
		ierr = OMX_ErrorInvalidComponent;
	}
	tcc_demux_lock_off(pPrivate);

	if(ierr != OMX_ErrorNone )
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}


#if 0   // sunny : not use
DxB_ERR_CODE TCC_DxB_DEMUX_SendData(DxBInterface *hInterface, UINT8 *data, UINT32 datasize)
{
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_DEMUX_SetAudioCodecInformation(DxBInterface *hInterface, UINT32 ulDevId, void *pAudioInfo)
{
	return TCC_DxB_AUDIO_SetAudioInfomation(hInterface, ulDevId, pAudioInfo);
}

DxB_ERR_CODE TCC_DxB_DEMUX_TDMB_SetContentsType(DxBInterface *hInterface, UINT32 ulDevId, UINT32 ContentsType)
{
	//TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	DxB_ERR_CODE ierr;
	UINT32 old_contents_type;

    if (hInterface->pOpenMaxIL == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	old_contents_type = tcc_omx_get_contents_type(OMX_Dxb_Dec_AppPriv);
#if 1
// Noah, To avoid CodeSonar warning, Redundant Condition
// tcc_omx_set_contents_type returns always '0'.
	tcc_omx_set_contents_type(OMX_Dxb_Dec_AppPriv, ContentsType);
#else
	ierr = tcc_omx_set_contents_type(OMX_Dxb_Dec_AppPriv, ContentsType);
	if (ierr != DxB_ERR_OK)
		return DxB_ERR_ERROR;
#endif

#ifdef  SUPPORT_ALWAYS_IN_OMX_EXECUTING
	if (tcc_omx_get_dxb_type(OMX_Dxb_Dec_AppPriv) == DxB_STANDARD_TDMB) {
		if (old_contents_type != tcc_omx_get_contents_type(OMX_Dxb_Dec_AppPriv)) {
			if (tcc_omx_stop(OMX_Dxb_Dec_AppPriv) == 0) {
				tcc_omx_load_component(OMX_Dxb_Dec_AppPriv, COMP_ALL & (~COMP_TUNER));
			}
			if (tcc_omx_get_contents_type(OMX_Dxb_Dec_AppPriv) == 2) {
				ierr = tcc_omx_select_audiovideo_type(OMX_Dxb_Dec_AppPriv, STREAMTYPE_AUDIO_BSAC, -1);
				if (ierr != DxB_ERR_OK)
					return DxB_ERR_ERROR;
			} else {
				if(tcc_omx_get_contents_type(OMX_Dxb_Dec_AppPriv) == 1){
					ierr = tcc_omx_select_audiovideo_type(OMX_Dxb_Dec_AppPriv, STREAMTYPE_AUDIO2, -1);
					if (ierr != DxB_ERR_OK)
						return DxB_ERR_ERROR;
				}
				else if(tcc_omx_get_contents_type(OMX_Dxb_Dec_AppPriv) == 4){
					ierr = tcc_omx_select_audiovideo_type(OMX_Dxb_Dec_AppPriv, STREAMTYPE_AUDIO_AAC_PLUS, -1);
					if (ierr != DxB_ERR_OK)
						return DxB_ERR_ERROR;
				}
			}
			tcc_omx_disable_unused_port(OMX_Dxb_Dec_AppPriv);
			tcc_omx_execute(OMX_Dxb_Dec_AppPriv);
		}
	}
#endif
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_DEMUX_RegisterEventCallback(DxBInterface *hInterface, pfnDxB_DEMUX_EVENT_Notify pfnEventCallback, void *pUserData)
{
	TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	//Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;

    if (hInterface->pDemuxPrivate == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pDemuxPrivate NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	tcc_demux_lock_on(pPrivate);
	pPrivate->pfnEventCallback = pfnEventCallback;
	pPrivate->pUserData = pUserData;
	tcc_demux_lock_off(pPrivate);

	return DxB_ERR_OK;
}
#endif

DxB_ERR_CODE TCC_DxB_DEMUX_SendEvent(DxBInterface *hInterface, DxB_DEMUX_NOTIFY_EVT nEvent, void *pCallbackData)
{
	TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;

    if (hInterface->pDemuxPrivate == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pDemuxPrivate NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	if (pPrivate->pfnEventCallback) {
		// Modified by ReversJ - B060961
		//tcc_demux_lock_on(pPrivate);
		pPrivate->pfnEventCallback(nEvent, pCallbackData, pPrivate->pUserData);
		//tcc_demux_lock_off(pPrivate);
		return DxB_ERR_OK;
	}
	return DxB_ERR_ERROR;
}


DxB_ERR_CODE TCC_DxB_DEMUX_RegisterCasDecryptCallback(DxBInterface *hInterface, UINT32 ulDevId, pfnDxb_DEMUX_CasDecrypt pfnCasCallback)
{
	TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;
	INT32       piArg[3];

	DEBUG(DEB_LEV_PARAMS, "In %s\n", __func__);

    if (hInterface->pDemuxPrivate == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pDemuxPrivate NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	piArg[0] = 0;
	piArg[1] = (INT32) ulDevId;
	piArg[2] = (INT32) pfnCasCallback;

	tcc_demux_lock_on(pPrivate);
	if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBRegisterCasDecryptCallback, piArg);
	else
	{
		ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
		ierr = OMX_ErrorInvalidComponent;
	}
	tcc_demux_lock_off(pPrivate);

	if (ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}

#if 0   // sunny : not use
DxB_ERR_CODE TCC_DxB_DEMUX_EnableAudioDescription(DxBInterface *hInterface, BOOLEAN isEnableAudioDescription)
{
	TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;
	INT32       piArg[1];

    if (hInterface->pDemuxPrivate == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pDemuxPrivate NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	tcc_demux_lock_on(pPrivate);
	piArg[0] = (INT32) isEnableAudioDescription;

	if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBEnableAudioDescription, piArg);
	else
	{
		ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
		ierr = OMX_ErrorInvalidComponent;
	}
	tcc_demux_lock_off(pPrivate);

	if (ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_DEMUX_SetSTCOffset(DxBInterface *hInterface, UINT32 ulDevId, INT32 iOffset, INT32 iOption)
{
	TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;
	INT32       piArg[4];

    if (hInterface->pDemuxPrivate == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pDemuxPrivate NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	tcc_demux_lock_on(pPrivate);
	piArg[0] = 2;
	piArg[1] = ulDevId;
	piArg[2] = iOffset;
	piArg[3] = iOption;

	if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorSetSTCDelay, piArg);
	else
	{
		ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
		ierr = OMX_ErrorInvalidComponent;
	}
	tcc_demux_lock_off(pPrivate);

	if (ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}
#endif

DxB_ERR_CODE TCC_DxB_DEMUX_SetDualchannel(DxBInterface *hInterface, UINT32 ulDemuxId, UINT32 uiIndex)
{
    TCC_DxB_DEMUX_PRIVATE *pPrivate = hInterface->pDemuxPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;
	INT32		piArg[2];

    if (hInterface->pDemuxPrivate == NULL)
    {
		DEBUG(DEB_LEV_ERR, "[%s] hInterface->pDemuxPrivate NULL!!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	tcc_demux_lock_on(pPrivate);
	piArg[0] = (INT32) ulDemuxId;
	piArg[1] = (INT32) uiIndex;
	if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBSetDualchannel, piArg);
	else
	{
		ALOGE("[%s] OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->demuxhandle NULL!!!!!\n", __func__);
		ierr = OMX_ErrorInvalidComponent;
	}
	tcc_demux_lock_off(pPrivate);
	if (ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}

