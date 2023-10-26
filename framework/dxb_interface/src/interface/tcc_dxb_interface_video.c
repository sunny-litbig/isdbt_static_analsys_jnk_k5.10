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
#define LOG_TAG	"DXB_INTERFACE_VIDEO"
#include <utils/Log.h>
#include <cutils/properties.h>

#include "globals.h"
#include <OMX_Types.h>
#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_Audio.h>
#include "OMX_Other.h"
#include <user_debug_levels.h>
#include <omx_base_component.h>
#include "omx_fbdev_sink_component.h"
#include "omx_videodec_component.h"
#include "tcc_dxb_interface_type.h"
#include "tcc_dxb_interface_omxil.h"
#include "tcc_dxb_interface_video.h"
#include "tcc_dxb_interface_demux.h"

#include <OMX_TCC_Index.h>
#include "OMX_Core.h"

typedef struct {
	pfnDxB_VIDEO_EVENT_Notify pfnEventCallback;
	void *pUserData;
	unsigned int uiDisplayEnableFlag;
} TCC_DxB_VIDEO_PRIVATE;

DxB_ERR_CODE TCC_DxB_VIDEO_Init(DxBInterface *hInterface)
{
	TCC_DxB_VIDEO_PRIVATE *pPrivate = TCC_fo_malloc(__func__, __LINE__,sizeof(TCC_DxB_VIDEO_PRIVATE));
	if(pPrivate == NULL){
		return DxB_ERR_ERROR;
	}
	else
	{
		// initialize TCC_DxB_VIDEO_PRIVATE structure
		pPrivate->uiDisplayEnableFlag = 0;
		pPrivate->pfnEventCallback = NULL;
		pPrivate->pUserData = NULL;
		hInterface->pVideoPrivate = pPrivate;
	}

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_VIDEO_Deinit(DxBInterface *hInterface)
{
	TCC_fo_free(__func__, __LINE__,hInterface->pVideoPrivate);

	hInterface->pVideoPrivate = NULL;

	return DxB_ERR_OK;
}

#if 0   // sunny : not use.
unsigned int TCC_DxB_VIDEO_GetDisplayFlag(DxBInterface *hInterface)
{
	TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;

    if (hInterface->pVideoPrivate == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pVideoPrivate is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	return pPrivate->uiDisplayEnableFlag;
}
#endif

DxB_ERR_CODE TCC_DxB_VIDEO_Start(DxBInterface *hInterface, UINT32 ulDevId, UINT32 ulVideoFormat)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr =0;
	OMX_VIDEO_PARAM_STARTTYPE stStartType;
	DEBUG (DEB_LEV_PARAMS, "In %s, VideoFormat: 0x%08x\n", __func__, ulVideoFormat);

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	if(OMX_Dxb_Dec_AppPriv->fbdevhandle)
	{
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_IndexVendorParamFBSetFirstFrame, &ulDevId);
		if(ierr != OMX_ErrorNone)
		{
			DEBUG (DEB_LEV_PARAMS, "FB (%d) SetFirstFrame error", ulDevId);
		}
	}

#ifdef  SUPPORT_ALWAYS_IN_OMX_EXECUTING
	stStartType.nDevID = ulDevId;

	if(ulVideoFormat == STREAMTYPE_VIDEO_AVCH264)
		stStartType.nVideoFormat = TCC_DXBVIDEO_OMX_Codec_H264;
	else if((ulVideoFormat == STREAMTYPE_MPEG1_VIDEO) || (ulVideoFormat == STREAMTYPE_VIDEO))
		stStartType.nVideoFormat = TCC_DXBVIDEO_OMX_Codec_MPEG2;
	else
	{
		DEBUG (DEB_LEV_PARAMS, "[Error] Video format(0x%08x) is not supported\n", ulVideoFormat);
		return DxB_ERR_ERROR;
	}

	TCC_DxB_DEMUX_SetESType(hInterface, 0, -1, ulVideoFormat);
	stStartType.bStartFlag = OMX_TRUE;
	if(OMX_Dxb_Dec_AppPriv->videohandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->videohandle, OMX_IndexParamVideoSetStart, &stStartType);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
#else
	if(tcc_omx_get_dxb_type(OMX_Dxb_Dec_AppPriv) == DxB_STANDARD_IPTV)
	{
		if(ulVideoFormat == STREAMTYPE_VIDEO_AVCH264)
			stStartType.nVideoFormat = TCC_DXBVIDEO_OMX_Codec_H264;
		else if((ulVideoFormat == STREAMTYPE_MPEG1_VIDEO) || (ulVideoFormat == STREAMTYPE_VIDEO))
			stStartType.nVideoFormat = TCC_DXBVIDEO_OMX_Codec_MPEG2;
		else
		{
			DEBUG (DEB_LEV_PARAMS, "[Error] Video format(0x%08x) is not supported\n", ulVideoFormat);
			return DxB_ERR_ERROR;
		}
		stStartType.bStartFlag = OMX_TRUE;

		if(OMX_Dxb_Dec_AppPriv->videohandle)
			ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->videohandle, OMX_IndexParamVideoSetStart, &stStartType);
		else
			ierr = OMX_ErrorInvalidComponent;
		if(ierr != OMX_ErrorNone)
			return DxB_ERR_ERROR;
	}
	else
	{
		ierr = tcc_omx_check_video_type(ulVideoFormat);
		if(ierr != 0)
			return DxB_ERR_ERROR;
		ierr = tcc_omx_select_audiovideo_type(-1, ulVideoFormat);
		if(ierr != 0)
			return DxB_ERR_ERROR;
		tcc_omx_disable_unused_port(OMX_Dxb_Dec_AppPriv);
	}
#endif
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_VIDEO_Stop(DxBInterface *hInterface, UINT32 ulDevId)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr = DxB_ERR_OK;
	OMX_VIDEO_PARAM_STARTTYPE stStartType;

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	DEBUG (DEB_LEV_PARAMS, "In %s\n", __func__);
#ifdef  SUPPORT_ALWAYS_IN_OMX_EXECUTING
	stStartType.nDevID = ulDevId;
	stStartType.nVideoFormat = TCC_DXBVIDEO_OMX_Codec_H264;
	stStartType.bStartFlag = OMX_FALSE;

	if(OMX_Dxb_Dec_AppPriv->videohandle)
// Noah, 20190916, IM478A-73, Sometimes OMX_SetParameter returns OMX_ErrorBadParameter.	
//		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->videohandle, OMX_IndexParamVideoSetStart, &stStartType);
		OMX_SetParameter(OMX_Dxb_Dec_AppPriv->videohandle, OMX_IndexParamVideoSetStart, &stStartType);	
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
#endif
	return DxB_ERR_OK;
}

#if 0   // sunny : not use
DxB_ERR_CODE TCC_DxB_VIDEO_Pause(DxBInterface *hInterface, UINT32 ulDevId, BOOLEAN bOn)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr;
	OMX_VIDEO_PARAM_PAUSETYPE stPauseType;
	DEBUG (DEB_LEV_PARAMS, "In %s\n", __func__);

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

#ifdef  SUPPORT_ALWAYS_IN_OMX_EXECUTING
	if(tcc_omx_get_dxb_type(OMX_Dxb_Dec_AppPriv) == DxB_STANDARD_TDMB)
		return DxB_ERR_ERROR;

	if(bOn)
		stPauseType.bPauseFlag = OMX_TRUE;
	else
		stPauseType.bPauseFlag = OMX_FALSE;

	if(OMX_Dxb_Dec_AppPriv->videohandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->videohandle, OMX_IndexParamVideoSetPause, &stPauseType);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
#endif
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_VIDEO_Flush(DxBInterface *hInterface)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr;

	DEBUG(DEB_LEV_PARAMS, "In %s\n", __func__);

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	if(OMX_Dxb_Dec_AppPriv->fbdevhandle)
		ierr = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_CommandFlush, OMX_ALL, NULL);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;

	if(OMX_Dxb_Dec_AppPriv->videohandle)
		ierr = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->videohandle, OMX_CommandFlush, OMX_ALL, NULL);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}
#endif

DxB_ERR_CODE TCC_DxB_VIDEO_SetSTCFunction(DxBInterface *hInterface, void *pfnGetSTCFunc, void *pvApp)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr = OMX_ErrorNone;
	INT32 piArg[2];

	DEBUG (DEB_LEV_PARAMS, "In %s\n", __func__);

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	piArg[0] = (INT32) pfnGetSTCFunc;
	piArg[1] = (INT32) pvApp;

	if(OMX_Dxb_Dec_AppPriv->fbdevhandle)
	{
	        ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_IndexVendorParamDxBGetSTCFunction, piArg);
	        if(ierr != OMX_ErrorNone)
		        return DxB_ERR_ERROR;
	}

	if(OMX_Dxb_Dec_AppPriv->videohandle) {
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->videohandle, OMX_IndexVendorParamDxBGetSTCFunction, piArg);
		if(ierr != OMX_ErrorNone)
			return DxB_ERR_ERROR;
	}

	return DxB_ERR_OK;
}

#if 0   // sunny : not use
DxB_ERR_CODE TCC_DxB_VIDEO_StartCapture(DxBInterface *hInterface, UINT32 ulDevId, UINT16 usTargetWidth, UINT16 usTargetHeight, UINT8 *pucFileName)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
   	INT32 ierr;
	DEBUG (DEB_LEV_PARAMS, "In %s\n", __func__);

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	TCC_DxB_VIDEO_DisableDisplay(hInterface, ulDevId);

	if(OMX_Dxb_Dec_AppPriv->fbdevhandle)
   	    ierr = OMX_GetParameter(OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_IndexVendorParamFBCapture, pucFileName);
	else
		ierr = OMX_ErrorInvalidComponent;

	TCC_DxB_VIDEO_EnableDisplay(hInterface, ulDevId);

	if(ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}
#endif

DxB_ERR_CODE TCC_DxB_VIDEO_SelectDisplayOutput(DxBInterface *hInterface, UINT32 ulDevId)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr = OMX_ErrorNone;

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	if(tcc_omx_get_dxb_type(OMX_Dxb_Dec_AppPriv) == DxB_STANDARD_TDMB)
		return DxB_ERR_ERROR;

	if(OMX_Dxb_Dec_AppPriv->videohandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->videohandle, OMX_IndexVendorParamVideoDisplayOutputCh, &ulDevId);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone)	return DxB_ERR_ERROR;

	if(OMX_Dxb_Dec_AppPriv->fbdevhandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_IndexVendorParamVideoDisplayOutputCh, &ulDevId);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone)	return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_VIDEO_IsSupportCountry(DxBInterface *hInterface, UINT32 ulDevId, UINT32 ulCountry)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
   	INT32 ierr;
	INT32       piArg[2];

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	piArg[0] = (INT32) ulDevId;
	piArg[1] = ulCountry;

	DEBUG (DEB_LEV_PARAMS, "In %s\n", __func__);
	if(OMX_Dxb_Dec_AppPriv->videohandle)
		ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->videohandle, OMX_IndexVendorParamVideoIsSupportCountry, piArg);
	else
		ierr = OMX_ErrorInvalidComponent;
	if(ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}

#if 0   // sunny : not use
DxB_ERR_CODE TCC_DxB_VIDEO_RefreshDisplay(DxBInterface *hInterface, UINT32 ulDevId)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
   	INT32 ierr;
	DEBUG (DEB_LEV_PARAMS, "In %s\n", __func__);

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	if(OMX_Dxb_Dec_AppPriv->fbdevhandle)
		ierr = OMX_GetParameter(OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_IndexConfigLcdcUpdate, NULL);
	else
		ierr = OMX_ErrorInvalidComponent;
	if(ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_VIDEO_EnableDisplay(DxBInterface *hInterface, UINT32 ulDevId)
{
	TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	//Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	DEBUG (DEB_LEV_PARAMS, "In %s\n", __func__);

    if (hInterface->pVideoPrivate == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pVideoPrivate is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	pPrivate->uiDisplayEnableFlag = 1;
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_VIDEO_DisableDisplay(DxBInterface *hInterface, UINT32 ulDevId)
{
	TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	//Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	DEBUG (DEB_LEV_PARAMS, "In %s\n", __func__);

    if (hInterface->pVideoPrivate == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pVideoPrivate is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	pPrivate->uiDisplayEnableFlag = 0;
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_VIDEO_LCDUpdate(DxBInterface *hInterface, UINT32 ulContentsType)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
   	INT32 ierr=0;
	DEBUG (DEB_LEV_PARAMS, "In %s\n", __func__);

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	if(ulContentsType == 0) //Main contents, ISDB, DVB, TDMB, ATSC
	{
		if(OMX_Dxb_Dec_AppPriv->fbdevhandle)
			ierr = OMX_SetConfig(OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_IndexConfigLcdcUpdate, (OMX_PTR)1);
		else
			ierr = OMX_ErrorInvalidComponent;
	}
	if(ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_VIDEO_GetVideoInfo(DxBInterface *hInterface, UINT32 ulDevId, UINT32 *videoWidth, UINT32 *videoHeight)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;
	INT32       piArg[3];

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	piArg[0] = (INT32) ulDevId;
	piArg[1] = (INT32) videoWidth;
	piArg[2] = (INT32) videoHeight;

	if(OMX_Dxb_Dec_AppPriv->videohandle)
		ierr = OMX_GetParameter(OMX_Dxb_Dec_AppPriv->videohandle, OMX_IndexVendorParamDxBGetVideoInfo, piArg);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}
#endif

DxB_ERR_CODE TCC_DxB_VIDEO_RegisterEventCallback(DxBInterface *hInterface, pfnDxB_VIDEO_EVENT_Notify pfnEventCallback, void *pUserData)
{
	TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	//Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;

	DEBUG (DEB_LEV_PARAMS, "In %s\n", __func__);

    if (hInterface->pVideoPrivate == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pVideoPrivate is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	pPrivate->pfnEventCallback = pfnEventCallback;
	pPrivate->pUserData = pUserData;

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_VIDEO_SetVpuReset(DxBInterface *hInterface, UINT32 ulDevId)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32		ierr;
	INT32		piArg[1];
	piArg[0] = (INT32) ulDevId;

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	if(OMX_Dxb_Dec_AppPriv->videohandle)
		ierr = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->videohandle, OMX_IndexVendorParamSetVpuReset, piArg);
	else
		ierr = OMX_ErrorInvalidComponent;
	if(ierr != OMX_ErrorNone )
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}

#if 0   // sunny : not use
DxB_ERR_CODE TCC_DxB_VIDEO_SetActiveMode (DxBInterface *hInterface, UINT32 ulDevId, UINT32 activemode)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32       ierr;
	INT32       piArg[2];

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	piArg[0] = (INT32) ulDevId;
	piArg[1] = (INT32) activemode;
	if(OMX_Dxb_Dec_AppPriv->videohandle)
		ierr = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->videohandle, OMX_IndexVendorParamDxBActiveModeSet, piArg);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone )
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}


DxB_ERR_CODE TCC_DxB_VIDEO_SetPause(DxBInterface *hInterface, UINT32 ulDevId, UINT32 pause)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32		ierr;
	INT32		piArg[2];
	piArg[0] = (INT32) ulDevId;
	piArg[1] = (INT32) pause;

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	if(OMX_Dxb_Dec_AppPriv->videohandle)
		ierr = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->videohandle, OMX_IndexVendorParamSetVideoPause, piArg);
	else
		ierr = OMX_ErrorInvalidComponent;
	if(ierr != OMX_ErrorNone )
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_VIDEO_IFrameSearchEnable (DxBInterface *hInterface, UINT32 ulDevId)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32		ierr;
	INT32		piArg[1];
	piArg[0] = (INT32) ulDevId;

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	if(OMX_Dxb_Dec_AppPriv->videohandle)
		ierr = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->videohandle, OMX_IndexVendorParamIframeSearchEnable, piArg);
	else
		ierr = OMX_ErrorInvalidComponent;
	if(ierr != OMX_ErrorNone )
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_VIDEO_SetSinkByPass(DxBInterface *hInterface, UINT32 ulDevId, UINT32 sink)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32		ierr;
	INT32		piArg[2];
	piArg[0] = (INT32) ulDevId;
	piArg[1] = (INT32) sink;

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	if(OMX_Dxb_Dec_AppPriv->fbdevhandle)
		ierr = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_IndexVendorParamSetSinkByPass, piArg);
	else
		ierr = OMX_ErrorInvalidComponent;
	if(ierr != OMX_ErrorNone )
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}
#endif

DxB_ERR_CODE TCC_DxB_VIDEO_SendEvent(DxBInterface *hInterface, DxB_VIDEO_NOTIFY_EVT nEvent, void *pCallbackData)
{
	TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	//Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;

    if (hInterface->pVideoPrivate == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pVideoPrivate is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if(pPrivate->pfnEventCallback)
    {
        pPrivate->pfnEventCallback(nEvent, pCallbackData, pPrivate->pUserData);
        return DxB_ERR_OK;
    }
    return DxB_ERR_ERROR;
}

DxB_ERR_CODE TCC_DxB_VIDEO_InitSurface(DxBInterface *hInterface, UINT32 ulDevId, int arg)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr = OMX_ErrorNone;
	INT32 piArg[2];

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	piArg[0] = (INT32) ulDevId;
	piArg[1] = (INT32) arg;

	if(OMX_Dxb_Dec_AppPriv->fbdevhandle)
		ierr = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_IndexVendorParamInitSurface, piArg);
	else
		ierr = OMX_ErrorInvalidComponent;

	if (ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_VIDEO_DeinitSurface(DxBInterface *hInterface, UINT32 ulDevId)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr = OMX_ErrorNone;
	INT32 piArg[1];

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	piArg[0] = (INT32) ulDevId;

	if(OMX_Dxb_Dec_AppPriv->fbdevhandle)
		ierr = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_IndexVendorParamDeinitSurface, piArg);
	else
		ierr = OMX_ErrorInvalidComponent;

	if (ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_VIDEO_SetSurface(DxBInterface *hInterface, UINT32 ulDevId, void *nativeWidow)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr = OMX_ErrorNone;
	INT32 piArg[2];

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	piArg[0] = (INT32) ulDevId;
	piArg[1] = (INT32) nativeWidow;

	if(OMX_Dxb_Dec_AppPriv->fbdevhandle)
		ierr = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_IndexVendorParamSetSurface, piArg);
	else
		ierr = OMX_ErrorInvalidComponent;

	if (ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}


DxB_ERR_CODE TCC_DxB_VIDEO_UseSurface(DxBInterface *hInterface)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32		ierr = OMX_ErrorNone;
	INT32		piArg = (INT32) OMX_TRUE;

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	if(OMX_Dxb_Dec_AppPriv->fbdevhandle)
		ierr = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_IndexVendorParamVideoSurfaceState, &piArg);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone )
		return DxB_ERR_ERROR;

	if(OMX_Dxb_Dec_AppPriv->videohandle)
		ierr = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->videohandle, OMX_IndexVendorParamVideoSurfaceState, &piArg);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone )
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_VIDEO_ReleaseSurface(DxBInterface *hInterface)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32		ierr =0;
	INT32		piArg = (INT32) OMX_FALSE;

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	if(OMX_Dxb_Dec_AppPriv->videohandle)
		ierr = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->videohandle, OMX_IndexVendorParamVideoSurfaceState, &piArg);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone )
		return DxB_ERR_ERROR;

	if(OMX_Dxb_Dec_AppPriv->fbdevhandle)
		ierr = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_IndexVendorParamVideoSurfaceState, &piArg);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone )
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}

#if 0   // sunny : not use
DxB_ERR_CODE TCC_DxB_VIDEO_Subtitle(DxBInterface *hInterface, void *arg)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32		ierr;
	INT32		*piArg = (INT32 *) arg;

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	if(OMX_Dxb_Dec_AppPriv->fbdevhandle)
		ierr = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_IndexVendorParamSetSubtitle, piArg);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone )
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_VIDEO_SupportFieldDecoding(DxBInterface *hInterface, UINT32 OnOff)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32		ierr;
	INT32		piArg = (INT32) OnOff;

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	if(OMX_Dxb_Dec_AppPriv->videohandle)
		ierr = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->videohandle, OMX_IndexVendorParamVideoSupportFieldDecoding, &piArg);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone )
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_VIDEO_SupportIFrameSearch(DxBInterface *hInterface, UINT32 OnOff)
{
	TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32		ierr;
	INT32		piArg = (INT32) OnOff;

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	if(OMX_Dxb_Dec_AppPriv->videohandle)
		ierr = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->videohandle, OMX_IndexVendorParamVideoSupportIFrameSearch, &piArg);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone )
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_VIDEO_SupportUsingErrorMB(DxBInterface *hInterface, UINT32 OnOff)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32		ierr;
	INT32		piArg = (INT32) OnOff;

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	if(OMX_Dxb_Dec_AppPriv->videohandle)
		ierr = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->videohandle, OMX_IndexVendorParamVideoSupportUsingErrorMB, &piArg);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone )
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_VIDEO_SupportDirectDisplay(DxBInterface *hInterface, UINT32 OnOff)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32		ierr;
	INT32		piArg = (INT32) OnOff;

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	if(OMX_Dxb_Dec_AppPriv->videohandle)
		ierr = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->videohandle, OMX_IndexVendorParamVideoSupportDirectDisplay, &piArg);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone )
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}
#endif

DxB_ERR_CODE TCC_DxB_VIDEO_SetProprietaryData (DxBInterface *hInterface, UINT32 channel_index)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32	ierr;
	INT32	*piArg = (INT32 *) &channel_index;

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (OMX_Dxb_Dec_AppPriv->videohandle)
	    ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->videohandle, OMX_IndexVendorParamISDBTProprietaryData, piArg);
	else
		ierr = OMX_ErrorInvalidComponent;

	if (ierr != OMX_ErrorNone) return DxB_ERR_ERROR;
	else return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_VIDEO_ServiceIDDisableDisplay(DxBInterface *hInterface, UINT32 check_flag)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr;
	DEBUG (DEB_LEV_PARAMS, "In %s [%d]\n", __func__, check_flag);

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (OMX_Dxb_Dec_AppPriv->fbdevhandle)
	    ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_IndexVendorParamISDBTSkipVideo, &check_flag);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_VIDEO_GetDisplayedFirstFrame(DxBInterface *hInterface, UINT32 ulDevId, INT32 *displayedfirstframe)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr;
	INT32 piArg[2];

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	piArg[0] = (INT32)ulDevId;
	piArg[1] = (INT32)displayedfirstframe;
	if(OMX_Dxb_Dec_AppPriv && OMX_Dxb_Dec_AppPriv->videohandle)
		ierr = OMX_GetParameter(OMX_Dxb_Dec_AppPriv->videohandle, OMX_IndexVendorParamDxBGetDisplayedFirstFrame, piArg);
	else
		return OMX_ErrorInvalidComponent;
	if(ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_VIDEO_SetFirstFrameByPass(DxBInterface *hInterface, UINT32 OnOff)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr;
	DEBUG (DEB_LEV_PARAMS, "In %s [%d]\n", __func__, OnOff);

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (OMX_Dxb_Dec_AppPriv->fbdevhandle)
	    ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_IndexVendorParamSetFirstFrameByPass, &OnOff);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}


DxB_ERR_CODE TCC_DxB_VIDEO_SetFirstFrameAfterSeek(DxBInterface *hInterface, UINT32 OnOff)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr;
	DEBUG (DEB_LEV_PARAMS, "In %s [%d]\n", __func__, OnOff);

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (OMX_Dxb_Dec_AppPriv->fbdevhandle)
	    ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_IndexVendorParamSetFirstFrameAfterSeek, &OnOff);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_VIDEO_SetFrameDropFlag(DxBInterface *hInterface, UINT32 check_flag)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr;
	DEBUG (DEB_LEV_PARAMS, "In %s [%d]\n", __func__, check_flag);

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (OMX_Dxb_Dec_AppPriv->fbdevhandle)
	    ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_IndexVendorParamSetFrameDropFlag, &check_flag);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}

#if 0   // sunny : not use
DxB_ERR_CODE TCC_DxB_VIDEO_CtrlLastFrame(DxBInterface *hInterface, UINT32 OnOff)
{
	TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr;
	INT32 piArg[1];

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	piArg[0] = (INT32) OnOff;
	if(OMX_Dxb_Dec_AppPriv->fbdevhandle)
		ierr = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_IndexVendorParamCtrlLastFrame, piArg);
	else
		ierr = OMX_ErrorInvalidComponent;

	if(ierr != OMX_ErrorNone )
		return DxB_ERR_ERROR;

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_VIDEO_SetDisplayPosition(DxBInterface *hInterface, INT32 iPosX, INT32 iPosY, INT32 iWidth, INT32 iHeight)
{
	//TCC_DxB_VIDEO_PRIVATE *pPrivate = hInterface->pVideoPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
	INT32 ierr;
	INT32 iarArg[4];

	DEBUG(DEB_LEV_PARAMS, "In %s [X:%d Y:%d W:%d H:%d]\n", __func__, iPosX, iPosY, iWidth, iHeight);

    if (hInterface->pOpenMaxIL == NULL)
    {
        DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!\n", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	iarArg[0] = iPosX;
	iarArg[1] = iPosY;
	iarArg[2] = iWidth;
	iarArg[3] = iHeight;

	ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_IndexVendorParamSetDisplayPosition, iarArg);
	if(ierr != OMX_ErrorNone)
		return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}
#endif
