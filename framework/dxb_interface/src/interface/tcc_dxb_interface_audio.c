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
#define LOG_TAG	"DXB_INTERFACE_AUDIO"
#include <utils/Log.h>

#include "globals.h"
#include <OMX_Types.h>
#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_Audio.h>
#include "OMX_Other.h"
#include <user_debug_levels.h>
#include <omx_base_component.h>
#include <omx_audiodec_component.h>
#include <omx_alsasink_component.h>
#include "tcc_dxb_interface_type.h"
#include "tcc_dxb_interface_omxil.h"
#include "tcc_dxb_interface_audio.h"
#include "tcc_dxb_interface_demux.h"
#include <OMX_TCC_Index.h>

typedef struct {
	pfnDxB_AUDIO_EVENT_Notify pfnEventCallback;
	void *pUserData;
} TCC_DxB_AUDIO_PRIVATE;

DxB_ERR_CODE TCC_DxB_AUDIO_Init(DxBInterface *hInterface)
{
	TCC_DxB_AUDIO_PRIVATE *pPrivate;

	DEBUG (DEB_LEV_PARAMS, "In %s\n", __func__);

	pPrivate = TCC_fo_malloc(__func__, __LINE__, sizeof(TCC_DxB_AUDIO_PRIVATE));
	if(pPrivate == NULL){
		return DxB_ERR_NO_ALLOC;
	}
	else
	{
		// initialize TCC_DxB_AUDIO_PRIVATE structure
		pPrivate->pfnEventCallback = NULL;
		pPrivate->pUserData = NULL;
		hInterface->pAudioPrivate = pPrivate;
	}

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_AUDIO_Deinit(DxBInterface *hInterface)
{
	DEBUG (DEB_LEV_PARAMS, "In %s\n", __func__);

	TCC_fo_free(__func__, __LINE__,hInterface->pAudioPrivate);

	hInterface->pAudioPrivate = NULL;

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_AUDIO_StopRequest(DxBInterface *hInterface, UINT32 ulDevId, UINT32 bRequestStop)
{
	//TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32		ierr;
	INT32		piArg[2];

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

	piArg[0] = (INT32) ulDevId;
	piArg[1] = (INT32) bRequestStop;

	if(OMX_Dxb_Dec_AppPriv->alsasinkhandle)
	    ierr = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->alsasinkhandle, OMX_IndexVendorParamStopRequest, piArg);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone )
		return DxB_ERR_ERROR;
	
	return DxB_ERR_OK;	
}

DxB_ERR_CODE TCC_DxB_AUDIO_CloseAlsaFlag(DxBInterface *hInterface, UINT32 CloseFlag)
{
	//TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32		ierr;
	INT32		piArg[1];

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

	piArg[0] = (INT32) CloseFlag;

	if(OMX_Dxb_Dec_AppPriv->alsasinkhandle)
	    ierr = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->alsasinkhandle, OMX_IndexVendorParamCloseAlsaFlag, piArg);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone )
		return DxB_ERR_ERROR;
	
	return DxB_ERR_OK;	
}

DxB_ERR_CODE TCC_DxB_AUDIO_Start(DxBInterface *hInterface, UINT32 ulDevId, UINT32 ulAudioFormat)
{
	//TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    INT32 ierr;
    OMX_AUDIO_PARAM_STARTTYPE stStartType;
    DEBUG (DEB_LEV_PARAMS, "In %s, AudioFormat: 0x%08x\n", __func__, ulAudioFormat);

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

#ifdef  SUPPORT_ALWAYS_IN_OMX_EXECUTING
    if(tcc_omx_get_dxb_type(OMX_Dxb_Dec_AppPriv) == DxB_STANDARD_TDMB) {
        TCC_DxB_AUDIO_StopRequest(hInterface, ulDevId, FALSE);

		TCC_DxB_AUDIO_SetAudioStartNotify(hInterface, ulDevId);

        if(tcc_omx_get_contents_type(OMX_Dxb_Dec_AppPriv) != 2)
        {
            stStartType.nDevID = ulDevId;
            stStartType.nAudioFormat = (tcc_omx_get_contents_type(OMX_Dxb_Dec_AppPriv) == 1)? TCC_DXBAUDIO_OMX_Codec_MP2:TCC_DXBAUDIO_OMX_Codec_AAC_PLUS;
            stStartType.bStartFlag = OMX_TRUE;

            if(OMX_Dxb_Dec_AppPriv->audiohandle)
          	    ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->audiohandle, OMX_IndexVendorParamAudioSetStart, &stStartType);
            else
                ierr = OMX_ErrorInvalidComponent;

            if(ierr != OMX_ErrorNone) 
                return DxB_ERR_ERROR;
            }
            return DxB_ERR_OK;
        }

	stStartType.nDevID = ulDevId;
    if(ulAudioFormat == STREAMTYPE_AUDIO_AC3_1)
        stStartType.nAudioFormat = TCC_DXBAUDIO_OMX_Codec_AC3;
	else if(ulAudioFormat == STREAMTYPE_AUDIO_AC3_2)
		stStartType.nAudioFormat = TCC_DXBAUDIO_OMX_Codec_DDP ;
    else if(ulAudioFormat == STREAMTYPE_AUDIO_AAC)
            stStartType.nAudioFormat = TCC_DXBAUDIO_OMX_Codec_AAC_ADTS;
    else if(ulAudioFormat == STREAMTYPE_AUDIO_AAC_LATM)
            stStartType.nAudioFormat = TCC_DXBAUDIO_OMX_Codec_AAC_LATM;	
    else if((ulAudioFormat == STREAMTYPE_AUDIO1) || (ulAudioFormat == STREAMTYPE_AUDIO2))
            stStartType.nAudioFormat = TCC_DXBAUDIO_OMX_Codec_MP2;
    else
    {
        DEBUG (DEB_LEV_PARAMS, "[Error in %s] Audio format(0x%08x) is not supported\n", __func__, ulAudioFormat);
        return DxB_ERR_ERROR;
    }
    TCC_DxB_DEMUX_SetESType(hInterface, 0, ulAudioFormat, -1);
    stStartType.bStartFlag = OMX_TRUE;

    if(OMX_Dxb_Dec_AppPriv->audiohandle)
	    ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->audiohandle, OMX_IndexVendorParamAudioSetStart, &stStartType);
    else
        ierr = OMX_ErrorInvalidComponent;

    if(ierr != OMX_ErrorNone) 
        return DxB_ERR_ERROR;    
    TCC_DxB_AUDIO_StopRequest(hInterface, ulDevId, FALSE);
#else    
    if(tcc_omx_get_dxb_type(OMX_Dxb_Dec_AppPriv) == DxB_STANDARD_IPTV)
    {
       	if((ulAudioFormat == STREAMTYPE_AUDIO_AC3_1) || (ulAudioFormat == STREAMTYPE_AUDIO_AC3_2))
			stStartType.nAudioFormat = TCC_DXBAUDIO_OMX_Codec_AC3;
        else if(ulAudioFormat == STREAMTYPE_AUDIO_AAC)
                stStartType.nAudioFormat = TCC_DXBAUDIO_OMX_Codec_AAC_ADTS;
        else if(ulAudioFormat == STREAMTYPE_AUDIO_AAC_LATM)
                stStartType.nAudioFormat = TCC_DXBAUDIO_OMX_Codec_AAC_LATM;	
        else if((ulAudioFormat == STREAMTYPE_AUDIO1) || (ulAudioFormat == STREAMTYPE_AUDIO2))
                stStartType.nAudioFormat = TCC_DXBAUDIO_OMX_Codec_MP2;
        else
        {
            DEBUG (DEB_LEV_PARAMS, "[Error] Audio format(0x%08x) is not supported\n", __func__, ulAudioFormat);
            return DxB_ERR_ERROR;
        }
        stStartType.bStartFlag = OMX_TRUE;

        if(OMX_Dxb_Dec_AppPriv->audiohandle)
            ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->audiohandle, OMX_IndexVendorParamAudioSetStart, &stStartType);
        else
            ierr = OMX_ErrorInvalidComponent;

        if(ierr != OMX_ErrorNone) 
            return DxB_ERR_ERROR;
    }
    else
    {
        ierr = tcc_omx_check_audio_type(ulAudioFormat);
        if(ierr != 0)
            return DxB_ERR_ERROR;
        ierr = tcc_omx_select_audiovideo_type(ulAudioFormat, -1);
        if(ierr != 0)
            return DxB_ERR_ERROR;
        tcc_omx_disable_unused_port(OMX_Dxb_Dec_AppPriv);
    }
#endif    
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_AUDIO_Stop(DxBInterface *hInterface, UINT32 ulDevId)
{
	//TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    INT32 ierr = OMX_ErrorNone;
    OMX_AUDIO_PARAM_STARTTYPE stStartType;
	
    DEBUG (DEB_LEV_PARAMS, "In %s\n", __func__);

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

#ifdef  SUPPORT_ALWAYS_IN_OMX_EXECUTING	
    if(tcc_omx_get_dxb_type(OMX_Dxb_Dec_AppPriv) == DxB_STANDARD_TDMB) {
        TCC_DxB_AUDIO_StopRequest(hInterface, ulDevId, TRUE);
        if(tcc_omx_get_contents_type(OMX_Dxb_Dec_AppPriv) != 2)
        {
            stStartType.nDevID = ulDevId;
            stStartType.nAudioFormat = (tcc_omx_get_contents_type(OMX_Dxb_Dec_AppPriv) == 1)? TCC_DXBAUDIO_OMX_Codec_MP2:TCC_DXBAUDIO_OMX_Codec_AAC_PLUS;
            stStartType.bStartFlag = OMX_FALSE;

            if(OMX_Dxb_Dec_AppPriv->audiohandle)
                ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->audiohandle, OMX_IndexVendorParamAudioSetStart, &stStartType);
            else
                ierr = OMX_ErrorInvalidComponent;

            if(ierr != OMX_ErrorNone) 
                return DxB_ERR_ERROR;
        }
        return DxB_ERR_OK;
    }

    TCC_DxB_AUDIO_StopRequest(hInterface, ulDevId, TRUE);
    stStartType.nDevID = ulDevId;
    stStartType.nAudioFormat = TCC_DXBAUDIO_OMX_Codec_AAC_ADTS;
    stStartType.bStartFlag = OMX_FALSE;

    if(OMX_Dxb_Dec_AppPriv->audiohandle)
        ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->audiohandle, OMX_IndexVendorParamAudioSetStart, &stStartType);
    else
        ierr = OMX_ErrorInvalidComponent;
	
    if(ierr != OMX_ErrorNone) 
        return DxB_ERR_ERROR;
#endif
    return DxB_ERR_OK;
}

#if 0   // sunny : not use
DxB_ERR_CODE TCC_DxB_AUDIO_SetStereo(DxBInterface *hInterface, UINT32 ulDevId, DxB_AUDIO_STEREO_MODE eMode)
{
	//TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr = DxB_ERR_OK;
	TCC_DXBAUDIO_DUALMONO_TYPE dualmono;
	
	DEBUG (DEB_LEV_PARAMS, "In %s\n", __func__);
	
    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

	switch(eMode)
	{
		case DxB_AUDIO_DUAL_Stereo:
			dualmono = TCC_DXBAUDIO_DUALMONO_LeftNRight;
			break;
		case DxB_AUDIO_DUAL_LeftOnly:
			dualmono = TCC_DXBAUDIO_DUALMONO_Left;
			break;
		case DxB_AUDIO_DUAL_RightOnly:
			dualmono = TCC_DXBAUDIO_DUALMONO_Right;
			break;
		case DxB_AUDIO_DUAL_Mix:
			dualmono = TCC_DXBAUDIO_DUALMONO_Mix;
			break;
		default:
			ierr = DxB_ERR_ERROR;
			break;
	}

	if(ierr == DxB_ERR_ERROR)
	{
		return DxB_ERR_ERROR;
	}

	if(OMX_Dxb_Dec_AppPriv->audiohandle)
	    ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->audiohandle, OMX_IndexVendorParamStereoMode, &dualmono);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone) 
		return DxB_ERR_ERROR;
	
	return DxB_ERR_OK;
}
#endif

DxB_ERR_CODE TCC_DxB_AUDIO_SetDualMono (DxBInterface *hInterface, UINT32 ulDevId, int audio_mode)
{
	//TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr;
	TCC_DXBAUDIO_DUALMONO_TYPE dualmono;

	DEBUG (DEB_LEV_PARAMS, "in %s, %d\n", __func__, audio_mode);

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

	switch(audio_mode)
	{
		case 0:
			dualmono = TCC_DXBAUDIO_DUALMONO_Left;
			break;
		case 1:
			dualmono = TCC_DXBAUDIO_DUALMONO_Right;
			break;
		case 2:
			dualmono = TCC_DXBAUDIO_DUALMONO_LeftNRight;
			break;
	}

	if(OMX_Dxb_Dec_AppPriv->audiohandle)
	    ierr = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->audiohandle, OMX_IndexVendorParamAudioAacDualMono, &dualmono);
	else
		ierr = OMX_ErrorInvalidComponent;
	
	if (ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;		
}

#if 0   // sunny : not use
DxB_ERR_CODE TCC_DxB_AUDIO_SetVolume(DxBInterface *hInterface, UINT32 ulDevId, UINT32 iVolume)
{
	//TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr;
	int iArg[2]; //0:left, 1:right

	DEBUG (DEB_LEV_PARAMS, "In %s\n", __func__);
	
    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

	iArg[0] = (int) ulDevId;
	iArg[1] = (int) iVolume;

	if(OMX_Dxb_Dec_AppPriv->alsasinkhandle)
	    ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->alsasinkhandle, OMX_IndexVendorParamAlsaSinkSetVolume, iArg);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone) 
		return DxB_ERR_ERROR;
	
	return DxB_ERR_OK;
}
#endif

DxB_ERR_CODE TCC_DxB_AUDIO_SetVolumeF(DxBInterface *hInterface, UINT32 ulDevId, REAL32 leftVolume, REAL32 rightVolume)
{
	//TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr;
	float fArg[2]; //0:left, 1:right

	DEBUG (DEB_LEV_PARAMS, "In %s\n", __func__);
	
    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

	fArg[0] = (float) leftVolume;
	fArg[1] = (float) rightVolume;

	if(OMX_Dxb_Dec_AppPriv->alsasinkhandle)
	    ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->alsasinkhandle, OMX_IndexVendorParamAlsaSinkSetVolume, fArg);
	else
		ierr = OMX_ErrorInvalidComponent;
	
	if(ierr != OMX_ErrorNone) 
		return DxB_ERR_ERROR;
	
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_AUDIO_SetMute(DxBInterface *hInterface, UINT32 ulDevId, BOOLEAN bMute)
{
	//TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr;
	INT32 iArg[1];

	DEBUG (DEB_LEV_PARAMS, "In %s\n", __func__);
	
    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

	iArg[0] = (INT32) bMute;

	if(OMX_Dxb_Dec_AppPriv->alsasinkhandle)
	    ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->alsasinkhandle, OMX_IndexVendorParamAlsaSinkMute, iArg);
	else
		ierr = OMX_ErrorInvalidComponent;
	
	if(ierr != OMX_ErrorNone) 
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_AUDIO_SetAudioStartSyncWithVideo(DxBInterface *hInterface, BOOLEAN bEnable)
{
	//TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr;
	INT32 iArg[1];

	DEBUG (DEB_LEV_PARAMS, "In %s\n", __func__);
	
    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

	iArg[0] = (INT32) bEnable;

	if(OMX_Dxb_Dec_AppPriv->alsasinkhandle)
	    ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->alsasinkhandle, OMX_IndexVendorParamSetEnableAudioStartSyncWithVideo, iArg);
	else
		ierr = OMX_ErrorInvalidComponent;
	
	if(ierr != OMX_ErrorNone) 
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_AUDIO_SetAudioPatternToCheckPTSnSTC(DxBInterface *hInterface, int pattern, int waittime_ms, int droptime_ms, int jumptime_ms)
{
	//TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr;
	INT32 iArg[4];

	DEBUG (DEB_LEV_PARAMS, "In %s\n", __func__);
	
    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

	iArg[0] = (INT32) pattern;
	iArg[1] = (INT32) waittime_ms;
	iArg[2] = (INT32) droptime_ms;
	iArg[3] = (INT32) jumptime_ms;
	
	if(OMX_Dxb_Dec_AppPriv->alsasinkhandle)
	    ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->alsasinkhandle, OMX_IndexVendorParamSetPatternToCheckPTSnSTC, iArg);
	else
		ierr = OMX_ErrorInvalidComponent;
	
	if(ierr != OMX_ErrorNone) 
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_AUDIO_Flush(DxBInterface *hInterface)
{
	//TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr;

	DEBUG(DEB_LEV_PARAMS, "In %s\n", __func__);
	
    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

	if(OMX_Dxb_Dec_AppPriv->audiohandle)
	    ierr = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->audiohandle, OMX_CommandFlush, OMX_ALL, NULL);
	else
		ierr = OMX_ErrorInvalidComponent;
	if(ierr != OMX_ErrorNone) 
		return DxB_ERR_ERROR;
	
	if(OMX_Dxb_Dec_AppPriv->alsasinkhandle)
	    ierr = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->alsasinkhandle, OMX_CommandFlush, OMX_ALL, NULL);
	else
		ierr = OMX_ErrorInvalidComponent;
	if(ierr != OMX_ErrorNone) 
		return DxB_ERR_ERROR;
	
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_AUDIO_SetSTCFunction(DxBInterface *hInterface, void *pfnGetSTCFunc, void *pvApp)
{
	//TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr;
	DxB_STANDARDS	dxb_standard;
	INT32 iArg[2];

	DEBUG (DEB_LEV_PARAMS, "In %s\n", __func__);

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

	dxb_standard = tcc_omx_get_dxb_type(OMX_Dxb_Dec_AppPriv);
	if (dxb_standard == DxB_STANDARD_ISDBT) {
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->audiohandle, OMX_IndexVendorParamAudioSetISDBTMode, NULL);
	}

	iArg[0] = (INT32) pfnGetSTCFunc;
	iArg[1] = (INT32) pvApp;

	if(OMX_Dxb_Dec_AppPriv->alsasinkhandle)
	{
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->alsasinkhandle, OMX_IndexVendorParamDxBGetSTCFunction, iArg);
		if(ierr != OMX_ErrorNone) 
			return DxB_ERR_ERROR;
	}

	if(OMX_Dxb_Dec_AppPriv->audiohandle)
	{
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->audiohandle, OMX_IndexVendorParamDxBGetSTCFunction, iArg);
		if(ierr != OMX_ErrorNone)
			return DxB_ERR_ERROR;
	}

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_AUDIO_DelayOutput(DxBInterface *hInterface, UINT32 ulDevId, INT32 delayMs)
{
	DEBUG (DEB_LEV_PARAMS, "In %s\n", __func__);
	
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_AUDIO_SetPause(DxBInterface *hInterface, UINT32 ulDevId, UINT32 pause)
{
	//TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32		ierr;
	INT32		piArg[2];

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

	piArg[0] = (INT32) ulDevId;
	piArg[1] = (INT32) pause;

	if(OMX_Dxb_Dec_AppPriv->audiohandle)
		ierr = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->audiohandle, OMX_IndexVendorParamSetAudioPause, piArg);
	else
		ierr = OMX_ErrorInvalidComponent;
	
	if(ierr != OMX_ErrorNone )
		return DxB_ERR_ERROR;
	
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_AUDIO_SetSinkByPass(DxBInterface *hInterface, UINT32 ulDevId, UINT32 sink)
{
	//TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32		ierr;
	INT32		piArg[2];

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

	piArg[0] = (INT32) ulDevId;
	piArg[1] = (INT32) sink;

	if(OMX_Dxb_Dec_AppPriv->alsasinkhandle)
		ierr = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->alsasinkhandle, OMX_IndexVendorParamSetSinkByPass, piArg);
	else
		ierr = OMX_ErrorInvalidComponent;
	
	if(ierr != OMX_ErrorNone )
		return DxB_ERR_ERROR;
	
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_AUDIO_SetAudioInfomation(DxBInterface *hInterface, UINT32 ulDevId, void* pAudioInfo)
{
	//TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr;

	DEBUG (DEB_LEV_PARAMS, "In %s\n", __func__);

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

    if (pAudioInfo == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] pAudioInfo is  NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

	OMX_INDEXTYPE eExtentionIndex;

	OMX_AUDIO_CONFIG_INFOTYPE *pParser_audio_info = (OMX_AUDIO_CONFIG_INFOTYPE *)pAudioInfo;;
	pParser_audio_info->nBitPerSample = 0;
	pParser_audio_info->nTotalTime= 0;

	pParser_audio_info->nPortIndex = AUDIO_PORT_INDEX; //audio port of the parser
	setHeader(pParser_audio_info, sizeof(OMX_AUDIO_CONFIG_INFOTYPE));

	if(OMX_Dxb_Dec_AppPriv->audiohandle)
		ierr = OMX_GetExtensionIndex(OMX_Dxb_Dec_AppPriv->audiohandle, TCC_AUDIO_MEDIA_INFO_STRING, &eExtentionIndex);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr == OMX_ErrorNone) 
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->audiohandle, eExtentionIndex, pParser_audio_info);
	else
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_AUDIO_RecordStart(DxBInterface *hInterface, UINT32 ulDevId, UINT8 * pucFileName)
{
	//TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr = OMX_ErrorNone;
	INT32       piArg[2];

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

	piArg[0] = (INT32) ulDevId;
	piArg[1] = (INT32) pucFileName;
	if(OMX_Dxb_Dec_AppPriv->audiohandle)
		ierr = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->audiohandle, OMX_IndexVendorParamRecStartRequest, piArg);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone )
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_AUDIO_RecordStop (DxBInterface *hInterface, UINT32 ulDevId)
{
	//TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr = OMX_ErrorNone;

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

	if(OMX_Dxb_Dec_AppPriv->audiohandle)
		ierr = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->audiohandle, OMX_IndexVendorParamRecStopRequest, (OMX_PTR)ulDevId);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone ) return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_AUDIO_SelectOutput(DxBInterface *hInterface, UINT32 ulDevId, UINT32 isEnableAudioOutput)
{
	//TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr = OMX_ErrorNone;
	INT32       piArg[2];

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

	piArg[0] = ulDevId;
	piArg[1] = isEnableAudioOutput;

	if(OMX_Dxb_Dec_AppPriv->audiohandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->audiohandle, OMX_IndexVendorParamAudioOutputCh, &ulDevId);
	else
		ierr = OMX_ErrorInvalidComponent;
	
	if(ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;

	if(OMX_Dxb_Dec_AppPriv->alsasinkhandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->alsasinkhandle, OMX_IndexVendorParamAudioOutputCh, piArg);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_AUDIO_IsSupportCountry(DxBInterface *hInterface, UINT32 ulDevId, UINT32 uiCountry)
{
	//TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr = OMX_ErrorNone;
	INT32	piArg[2];

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

	piArg[0] = (INT32)ulDevId;
	piArg[1] = (INT32)uiCountry;

	if(tcc_omx_get_dxb_type(OMX_Dxb_Dec_AppPriv) == DxB_STANDARD_TDMB)
		return DxB_ERR_ERROR;

	if(OMX_Dxb_Dec_AppPriv->audiohandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->audiohandle, OMX_IndexVendorParamAudioIsSupportCountry, piArg);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone)
 		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_AUDIO_StreamRollBack(DxBInterface *hInterface, UINT32 ulDevId)
{
	//TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

	INT32       ierr = OMX_ErrorNone;
	INT32	piArg[1];
	piArg[0] = (INT32)ulDevId;

	if(OMX_Dxb_Dec_AppPriv->alsasinkhandle)
		ierr = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->alsasinkhandle, OMX_IndexVendorParamResetRequestForALSASink, piArg);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone ) return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_AUDIO_GetAudioType (DxBInterface *hInterface, int iDeviceID, int *piNumCh, int *piAudioMode)
{
	//TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

	INT32 ierr = OMX_ErrorNone;
	INT32 iArg[3];
	iArg[0] = iDeviceID;

	*piNumCh = 0;
	*piAudioMode = 0;
	if (tcc_omx_get_dxb_type(OMX_Dxb_Dec_AppPriv) == DxB_STANDARD_ISDBT) {
        if(OMX_Dxb_Dec_AppPriv->audiohandle)
		    ierr = OMX_GetParameter(OMX_Dxb_Dec_AppPriv->audiohandle, OMX_IndexVendorParamDxbGetAudioType, iArg);
        else
		    ierr = OMX_ErrorInvalidComponent;

		if (ierr == OMX_ErrorNone) {
			*piNumCh = iArg[1];
			*piAudioMode = iArg[2];
		} else
			return DxB_ERR_ERROR;
	}
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_AUDIO_RegisterEventCallback(DxBInterface *hInterface, pfnDxB_AUDIO_EVENT_Notify pfnEventCallback, void *pUserData)
{
	TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	//Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;

	DEBUG (DEB_LEV_PARAMS, "In %s\n", __func__);

    if (hInterface->pAudioPrivate == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pAudioPrivate is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

	pPrivate->pfnEventCallback = pfnEventCallback;
	pPrivate->pUserData = pUserData;

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_AUDIO_SendEvent(DxBInterface *hInterface, DxB_AUDIO_NOTIFY_EVT nEvent, void *pCallbackData)
{
	TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	//Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;

    if (hInterface->pAudioPrivate == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pAudioPrivate is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

    if(pPrivate->pfnEventCallback)
    {
        pPrivate->pfnEventCallback(nEvent, pCallbackData, pPrivate->pUserData);
        return DxB_ERR_OK;
    }
    return DxB_ERR_ERROR;
}

DxB_ERR_CODE TCC_DxB_AUDIO_SetAudioStartNotify(DxBInterface *hInterface, UINT32 ulDevId)
{
	//TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr=0;

	DEBUG (DEB_LEV_PARAMS, "In %s\n", __func__);

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

	if (tcc_omx_get_dxb_type(OMX_Dxb_Dec_AppPriv) == DxB_STANDARD_TDMB)
	{
		if(OMX_Dxb_Dec_AppPriv->alsasinkhandle)
			ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->alsasinkhandle, OMX_IndexVendorParamAlsaSinkAudioStartNotify, &ulDevId);
		else
			ierr = OMX_ErrorInvalidComponent;
	}

	if(ierr != OMX_ErrorNone) 
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_AUDIO_ServiceIDDisableOutput(DxBInterface *hInterface, UINT32 check_flag)
{
	//TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr;
	DEBUG (DEB_LEV_PARAMS, "In %s [%d]\n", __func__, check_flag);

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

    if(OMX_Dxb_Dec_AppPriv->alsasinkhandle)
        ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->alsasinkhandle, OMX_IndexVendorParamISDBTSkipAudio, &check_flag);
    else
        ierr = OMX_ErrorInvalidComponent;

    if(ierr != OMX_ErrorNone) 
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_AUDIO_SetFrameDropFlag(DxBInterface *hInterface, UINT32 check_flag)
{
	//TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr;
	DEBUG (DEB_LEV_PARAMS, "In %s [%d]\n", __func__, check_flag);

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

    if(OMX_Dxb_Dec_AppPriv->alsasinkhandle)
	    ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->alsasinkhandle, OMX_IndexVendorParamSetFrameDropFlag, &check_flag);
    else
        ierr = OMX_ErrorInvalidComponent;

    if(ierr != OMX_ErrorNone) 
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_AUDIO_setSeamlessSwitchCompensation(DxBInterface *hInterface, INT32 iOnOff, INT32 iInterval, INT32 iStrength, INT32 iNtimes, INT32 iRange, INT32 iGapadjust, INT32 iGapadjust2, INT32 iMuliplier)
{
    //TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr;
	INT32 iArg[8];

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

	iArg[0] = iOnOff;
	iArg[1] = iInterval;
	iArg[2] = iStrength;
	iArg[3] = iNtimes;
	iArg[4] = iRange;
	iArg[5] = iGapadjust;
	iArg[6] = iGapadjust2;
	iArg[7] = iMuliplier;

	DEBUG (DEB_LEV_PARAMS, "In %s [%d:%d:%d:%d:%d:%d:%d:%d]\n", __func__, iOnOff, iInterval, iStrength, iNtimes, iRange, iGapadjust, iGapadjust2, iMuliplier);

    if(OMX_Dxb_Dec_AppPriv->audiohandle)
	    ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->audiohandle, OMX_IndexVendorParamSetSeamlessSwitchCompensation, iArg);
    else
        ierr = OMX_ErrorInvalidComponent;

    if(ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_AUDIO_SetProprietaryData (DxBInterface *hInterface, UINT32 channel_index, UINT32 service_id, UINT32 sub_service_id, INT32 dual_mode, INT32 supportPrimary)
{
    //TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr = OMX_ErrorNone;
	INT32 iArg[5];

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

	iArg[0] = channel_index;
	iArg[1] = service_id;
	iArg[2] = sub_service_id;
	iArg[3] = dual_mode;
	iArg[4] = supportPrimary;

    if(OMX_Dxb_Dec_AppPriv->audiohandle)
	    ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->audiohandle, OMX_IndexVendorParamISDBTProprietaryData, iArg);
    else
        ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_AUDIO_GetSeamlessValue (DxBInterface *hInterface, int *state, int *cval, int *pval)
{
	//TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr = OMX_ErrorNone;
	INT32 iArg[3];

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

	*state = 0;
	*cval = 0;
	*pval = 0;

    if(OMX_Dxb_Dec_AppPriv->audiohandle)
	    ierr = OMX_GetParameter(OMX_Dxb_Dec_AppPriv->audiohandle, OMX_IndexVendorParamDxbGetSeamlessValue, iArg);
    else
        ierr = OMX_ErrorInvalidComponent;

	if (ierr == OMX_ErrorNone) {
		*state = iArg[0];
		*cval = iArg[1];
		*pval = iArg[2];
	} else
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_AUDIO_SetSwitchSeamless (DxBInterface *hInterface, UINT32 flag)
{
    //TCC_DxB_AUDIO_PRIVATE *pPrivate = hInterface->pAudioPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32	ierr;

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
		return DxB_ERR_INVALID_PARAM;
    }

    if(OMX_Dxb_Dec_AppPriv->audiohandle)
	    ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->audiohandle, OMX_IndexVendorParamDxBSetDualchannel, &flag);
    else
        ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}
