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
#define LOG_TAG "DXB_INTERFACE_TUNER"

#include <utils/Log.h>

#include <OMX_Types.h>
#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_Audio.h>
#include <OMX_TCC_Index.h>
#include <OMX_Other.h>

#include <user_debug_levels.h>
#include <omx_base_component.h>

#include "tcc_dxb_interface_type.h"
#include "tcc_dxb_interface_omxil.h"
#include "tcc_dxb_interface_demux.h"
#include "tcc_dxb_interface_tuner.h"

#define THE_NUMBER_OF_TUNER 2

////////////////////////////////////////////////////////////////////////////////////////////////////
// T-DMB Only
enum {
    TDMB_TSDUMP_MODE = 0,
    TDMB_PLAY_MODE = 1
};
////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
	pfnDxB_TUNER_EVENT_Notify pfnEventCallback;
	void *pUserData;
	UINT32 uiTunerErrorIndicator[THE_NUMBER_OF_TUNER];
	int iDMBServiceMode[2];
} TCC_DxB_TUNER_PRIVATE;

static DxB_ERR_CODE Check_TunerID(TCC_DxB_TUNER_PRIVATE *pPrivate, UINT32 ulTunerID)
{
    if (ulTunerID >= THE_NUMBER_OF_TUNER)
        return DxB_ERR_INVALID_PARAM;

    if (pPrivate->uiTunerErrorIndicator[ulTunerID])
        return DxB_ERR_ERROR;

    return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_TUNER_Init(DxBInterface *hInterface, UINT32 ulStandard, UINT32 ulTunerType)
{
	UINT32 i;
	TCC_DxB_TUNER_PRIVATE *pPrivate = TCC_fo_malloc(__func__, __LINE__,sizeof(TCC_DxB_TUNER_PRIVATE));
	if(pPrivate == NULL){
		return DxB_ERR_ERROR;
	}

	hInterface->iTunerType    = ulTunerType;
	hInterface->pTunerPrivate = pPrivate;
	
	// initialize TCC_DxB_TUNER_PRIVATE structure
	pPrivate->pfnEventCallback = NULL;
	pPrivate->pUserData = NULL;

	pPrivate->uiTunerErrorIndicator[0] = 0;
	pPrivate->uiTunerErrorIndicator[1] = 0;

	pPrivate->iDMBServiceMode[0] = TDMB_PLAY_MODE;
	pPrivate->iDMBServiceMode[0] = TDMB_PLAY_MODE; // 0:SMART DMB MODE, 1:TDMB MODE

    return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_TUNER_Deinit(DxBInterface *hInterface)
{
    TCC_fo_free(__func__, __LINE__,hInterface->pTunerPrivate);

    hInterface->pTunerPrivate = NULL;

    return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_TUNER_RegisterEventCallback(DxBInterface *hInterface, pfnDxB_TUNER_EVENT_Notify pfnEventCallback, void *pUserData)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

	pPrivate->pfnEventCallback = pfnEventCallback;
	pPrivate->pUserData = pUserData;

    return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_TUNER_SendEvent(DxBInterface *hInterface, DxB_TUNER_NOTIFY_EVT nEvent, void *pCallbackData)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
    OMX_S32* piParam = (OMX_S32*) pCallbackData;
    UINT32 ulTunerID;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    ulTunerID     = (UINT32) piParam[0];
    pCallbackData = (void*) piParam[1];

    if (pPrivate->pfnEventCallback)
    {
        pPrivate->pfnEventCallback(nEvent, pCallbackData, pPrivate->pUserData);
        return DxB_ERR_OK;
    }
    return DxB_ERR_ERROR;
}

#if 0   // sunny : not use.
DxB_ERR_CODE TCC_DxB_TUNER_SetNumberOfBB(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulNumberOfBB)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    OMX_S32       piParam[2];
    DxB_ERR_CODE  eCode;
    INT32         iErr;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    eCode = Check_TunerID(pPrivate, ulTunerID);
    if (eCode != DxB_ERR_OK)
        return eCode;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle == NULL)
        return OMX_ErrorInvalidComponent;

    piParam[0] = (OMX_S32) ulTunerID;
    piParam[1] = (OMX_S32) ulNumberOfBB;

    iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamTunerSetNumberOfBB, piParam);

    if (iErr != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_TCC_ERR, "[%s] Error(%d)", __func__, iErr);
        return DxB_ERR_ERROR;
    }
    return DxB_ERR_OK;
}
#endif

DxB_ERR_CODE TCC_DxB_TUNER_Open(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 *pulArg)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    OMX_S32       piParam[2];
    DxB_ERR_CODE  eCode;
    INT32         iErr;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    eCode = Check_TunerID(pPrivate, ulTunerID);
    if (eCode != DxB_ERR_OK)
        return eCode;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle == NULL)
        return OMX_ErrorInvalidComponent;

    piParam[0] = (OMX_S32) ulTunerID;
    piParam[1] = (OMX_S32) pulArg;

    iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamTunerOpen, piParam);

    if (iErr != OMX_ErrorNone)
    {
        pPrivate->uiTunerErrorIndicator[ulTunerID] = 1;

        DEBUG(DEB_LEV_TCC_ERR, "[%s] Error(%d)", __func__, iErr);
        return DxB_ERR_ERROR;
    }

    pPrivate->uiTunerErrorIndicator[ulTunerID] = 0;

    return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_TUNER_Close(DxBInterface *hInterface, UINT32 ulTunerID)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    OMX_S32       piParam[2];
    DxB_ERR_CODE  eCode;
    INT32         iErr;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    eCode = Check_TunerID(pPrivate, ulTunerID);
    if (eCode != DxB_ERR_OK)
        return eCode;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle == NULL)
        return OMX_ErrorInvalidComponent;

    piParam[0] = (OMX_S32) ulTunerID;

    iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamTunerClose, piParam);

    if (iErr != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_TCC_ERR, "[%s] Error(%d)", __func__, iErr);
        return DxB_ERR_ERROR;
    }

    return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_TUNER_SetChannel(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulChannel, UINT32 lockOn)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    OMX_S32       piParam[2];
    DxB_ERR_CODE  eCode;
    INT32         iErr;
    INT32         piArg[2];

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    eCode = Check_TunerID(pPrivate, ulTunerID);
    if (eCode != DxB_ERR_OK)
        return eCode;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle == NULL)
        return OMX_ErrorInvalidComponent;

    piParam[0] = (OMX_S32) ulTunerID;
    piParam[1] = (OMX_S32) piArg;

    piArg[0] = (INT32)ulChannel;
    piArg[1] = (INT32)lockOn;

    iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamTunerChannelSet, piParam);

    if (iErr == OMX_ErrorNone) eCode = DxB_ERR_OK;
    else if (iErr == OMX_ErrorBadParameter) eCode = DxB_ERR_INVALID_PARAM;
    else if (iErr == OMX_ErrorNotReady) eCode = DxB_ERR_TIMEOUT;
    else eCode = DxB_ERR_ERROR;

    return eCode;
}

DxB_ERR_CODE TCC_DxB_TUNER_GetLockStatus(DxBInterface *hInterface, UINT32 ulTunerID, int *pLockStatus)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    OMX_S32       piParam[2];
    DxB_ERR_CODE  eCode;
    INT32         iErr;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    eCode = Check_TunerID(pPrivate, ulTunerID);
    if (eCode != DxB_ERR_OK)
        return eCode;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle == NULL)
        return OMX_ErrorInvalidComponent;

    piParam[0] = (OMX_S32) ulTunerID;
    piParam[1] = (OMX_S32) pLockStatus;

    iErr = OMX_GetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamTunerGetLockStatus, piParam);

    if (iErr == OMX_ErrorNone) eCode = DxB_ERR_OK;
    else if (iErr == OMX_ErrorBadParameter) eCode = DxB_ERR_INVALID_PARAM;
    else if (iErr == OMX_ErrorNotReady) eCode = DxB_ERR_TIMEOUT;
    else eCode = DxB_ERR_ERROR;

    return eCode;
}

DxB_ERR_CODE TCC_DxB_TUNER_GetChannelInfo(DxBInterface *hInterface, UINT32 ulTunerID, void *pChannelInfo)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    OMX_S32       piParam[2];
    DxB_ERR_CODE  eCode;
    INT32         iErr;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    eCode = Check_TunerID(pPrivate, ulTunerID);
    if (eCode != DxB_ERR_OK)
        return eCode;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle == NULL)
        return OMX_ErrorInvalidComponent;

    piParam[0] = (OMX_S32) ulTunerID;
    piParam[1] = (OMX_S32) pChannelInfo;

    iErr = OMX_GetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamTunerChannelSet, piParam);

    if (iErr != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_TCC_ERR, "[%s] Error(%d)", __func__, iErr);
        return DxB_ERR_ERROR;
    }
    return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_TUNER_GetSignalStrength(DxBInterface *hInterface, UINT32 ulTunerID, void *pStrength)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    OMX_S32       piParam[2];
    DxB_ERR_CODE  eCode;
    INT32         iErr;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    eCode = Check_TunerID(pPrivate, ulTunerID);
    if (eCode != DxB_ERR_OK)
        return eCode;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle == NULL)
        return OMX_ErrorInvalidComponent;

    piParam[0] = (OMX_S32) ulTunerID;
    piParam[1] = (OMX_S32) pStrength;

    iErr = OMX_GetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamGetSignalStrength, piParam);

    if (iErr != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_TCC_ERR, "[%s] Error(%d)", __func__, iErr);
        return DxB_ERR_ERROR;
    }
    return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_TUNER_RegisterPID(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulPID)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    OMX_S32       piParam[2];
    DxB_ERR_CODE  eCode;
    INT32         iErr;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    eCode = Check_TunerID(pPrivate, ulTunerID);
    if (eCode != DxB_ERR_OK)
        return eCode;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle == NULL)
        return OMX_ErrorInvalidComponent;

    piParam[0] = (OMX_S32) ulTunerID;
    piParam[1] = (OMX_S32) ulPID;

    iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamRegisterPID, piParam);

    if (iErr != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_TCC_ERR, "[%s] Error(%d)", __func__, iErr);
        return DxB_ERR_ERROR;
    }
    return DxB_ERR_OK;
}

#if 0   // sunny : not use.
DxB_ERR_CODE TCC_DxB_TUNER_UnRegisterPID(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulPID)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    OMX_S32       piParam[2];
    DxB_ERR_CODE  eCode;
    INT32         iErr;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    eCode = Check_TunerID(pPrivate, ulTunerID);
    if (eCode != DxB_ERR_OK)
        return eCode;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle == NULL)
        return OMX_ErrorInvalidComponent;

    piParam[0] = (OMX_S32) ulTunerID;
    piParam[1] = (OMX_S32) ulPID;

    iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamUnRegisterPID, piParam);

    if (iErr != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_TCC_ERR, "[%s] Error(%d)", __func__, iErr);
        return DxB_ERR_ERROR;
    }
    return DxB_ERR_OK;
}

/*
 * ATSC Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_SearchAnalogChannel(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulChannel)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    OMX_S32       piParam[2];
    DxB_ERR_CODE  eCode;
    INT32         iErr;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    eCode = Check_TunerID(pPrivate, ulTunerID);
    if (eCode != DxB_ERR_OK)
        return eCode;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle == NULL)
        return OMX_ErrorInvalidComponent;

    piParam[0] = (OMX_S32) ulTunerID;
    piParam[1] = (OMX_S32) ulChannel;

    iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamTunerAnalogChannelSearch, piParam);

    if (iErr == OMX_ErrorBadParameter)
    {
        DEBUG(DEB_LEV_TCC_ERR, "[%s] Error(%d)", __func__, iErr);
        return DxB_ERR_ERROR;
    }
    return iErr;        //iErr => 0x00:not found & 0xff:found
}

/*
 * ATSC Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_SearchChannel(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulChannel)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    OMX_S32       piParam[2];
    DxB_ERR_CODE  eCode;
    INT32         iErr;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    eCode = Check_TunerID(pPrivate, ulTunerID);
    if (eCode != DxB_ERR_OK)
        return eCode;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle == NULL)
        return OMX_ErrorInvalidComponent;

    piParam[0] = (OMX_S32) ulTunerID;
    piParam[1] = (OMX_S32) ulChannel;

    iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamTunerChannelSearch, piParam);

    if (iErr != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_TCC_ERR, "[%s] Error(%d)", __func__, iErr);
        return DxB_ERR_ERROR;
    }
    return DxB_ERR_OK;
}

/*
 * ATSC Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_SetModulation (DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulModulation)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    OMX_S32       piParam[2];
    DxB_ERR_CODE  eCode;
    INT32         iErr;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    eCode = Check_TunerID(pPrivate, ulTunerID);
    if (eCode != DxB_ERR_OK)
        return eCode;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle == NULL)
        return OMX_ErrorInvalidComponent;

    piParam[0] = (OMX_S32) ulTunerID;
    piParam[1] = (OMX_S32) ulModulation;

    iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamTunerModulationSet, piParam);

    if (iErr != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_TCC_ERR, "[%s] Error(%d)", __func__, iErr);
        return DxB_ERR_ERROR;
    }
    return DxB_ERR_OK;
}

/*
 * DVB Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_SearchFrequencyWithBW(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulFrequencyKhz, UINT32 ulBandWidthKhz, UINT32 ulLockWait, UINT32 *pOptions, UINT32 *pSnr)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    OMX_S32       piParam[2];
    DxB_ERR_CODE  eCode;
    INT32         iErr;
    INT32         piArg[5];

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    eCode = Check_TunerID(pPrivate, ulTunerID);
    if (eCode != DxB_ERR_OK)
        return eCode;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle == NULL)
        return OMX_ErrorInvalidComponent;

    piParam[0] = (OMX_S32) ulTunerID;
    piParam[1] = (OMX_S32) piArg;

    piArg[0] = (INT32) ulFrequencyKhz;
    piArg[1] = (INT32) ulBandWidthKhz;
    piArg[2] = (INT32) ulLockWait; //0:No wait after setting, 1:wait after setting, return setting result
    piArg[3] = (INT32) pOptions;
    piArg[4] = 0;      // snr

    iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamTunerFrequencySet, piParam);

    if (pSnr)
    {
        *pSnr = piArg[4];
    }

    if (iErr != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_TCC_ERR, "[%s] Error(%d)", __func__, iErr);
        return DxB_ERR_ERROR;
    }
    return DxB_ERR_OK;
}

/*
 * DVB Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_SetAntennaPower(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulArg)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    OMX_S32       piParam[2];
    DxB_ERR_CODE  eCode;
    INT32         iErr;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    eCode = Check_TunerID(pPrivate, ulTunerID);
    if (eCode != DxB_ERR_OK)
        return eCode;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle == NULL)
        return OMX_ErrorInvalidComponent;

    piParam[0] = (OMX_S32) ulTunerID;
    piParam[1] = (OMX_S32) ulArg;

    iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamSetAntennaPower, piParam);

    if (iErr != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_TCC_ERR, "[%s] Error(%d)", __func__, iErr);
        return DxB_ERR_ERROR;
    }
    return DxB_ERR_OK;
}

/*
 * DVB Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_GetRFInformation(DxBInterface *hInterface, UINT32 ulTunerID, UINT8 *pvData, UINT32 *puiSize)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    OMX_S32       piParam[2];
    DxB_ERR_CODE  eCode;
    INT32         iErr;
    INT32         piArg[2];

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    eCode = Check_TunerID(pPrivate, ulTunerID);
    if (eCode != DxB_ERR_OK)
        return eCode;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle == NULL)
        return OMX_ErrorInvalidComponent;

    piParam[0] = (OMX_S32) ulTunerID;
    piParam[1] = (OMX_S32) piArg;

    piArg[0] = (INT32)pvData;
    piArg[1] = (INT32)puiSize;

    iErr = OMX_GetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamTunerInformation, piParam);

    if (iErr != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_TCC_ERR, "[%s] Error(%d)", __func__, iErr);
        return DxB_ERR_ERROR;
    }
    return DxB_ERR_OK;
}

/*
 * Get MPLP information, it returns number of PLP and PLP ids.
 * DVB-T2 Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_GetDataPLPs(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 *pulPLPId, UINT32 *pulPLPNum )
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    OMX_S32       piParam[2];
    DxB_ERR_CODE  eCode;
    INT32         iErr;
    INT32         piArg[2];

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    eCode = Check_TunerID(pPrivate, ulTunerID);
    if (eCode != DxB_ERR_OK)
        return eCode;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle == NULL)
        return OMX_ErrorInvalidComponent;

    piParam[0] = (OMX_S32) ulTunerID;
    piParam[1] = (OMX_S32) piArg;

    piArg[0] = (INT32) pulPLPId;
    piArg[1] = (INT32) pulPLPNum;

    iErr = OMX_GetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamTunerGetDataPLPs, piParam);

    if (iErr != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_TCC_ERR, "[%s] Error(%d)", __func__, iErr);
        return DxB_ERR_ERROR;
    }
    return DxB_ERR_OK;
}

/*
 * Set MPLP Id for playing.
 * DVB-T2 Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_SetDataPLP(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulPLPId )
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    OMX_S32       piParam[2];
    DxB_ERR_CODE  eCode;
    INT32         iErr;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    eCode = Check_TunerID(pPrivate, ulTunerID);
    if (eCode != DxB_ERR_OK)
        return eCode;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle == NULL)
        return OMX_ErrorInvalidComponent;

    piParam[0] = (OMX_S32) ulTunerID;
    piParam[1] = (OMX_S32) ulPLPId;

    iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamTunerSetDataPLP, piParam);

    if (iErr != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_TCC_ERR, "[%s] Error(%d)", __func__, iErr);
        return DxB_ERR_ERROR;
    }
    return DxB_ERR_OK;
}

/*
 * DVB-S Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_SetLNBVoltage(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulArg)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    OMX_S32       piParam[2];
    DxB_ERR_CODE  eCode;
    INT32         iErr;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    eCode = Check_TunerID(pPrivate, ulTunerID);
    if (eCode != DxB_ERR_OK)
        return eCode;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle == NULL)
        return OMX_ErrorInvalidComponent;

    piParam[0] = (OMX_S32) ulTunerID;
    piParam[1] = (OMX_S32) ulArg;

    iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamSetVoltage, piParam);

    if (iErr != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_TCC_ERR, "[%s] Error(%d)", __func__, iErr);
        return DxB_ERR_ERROR;
    }
    return DxB_ERR_OK;
}

/*
 * DVB-S Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_SetTone(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulArg)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    OMX_S32       piParam[2];
    DxB_ERR_CODE  eCode;
    INT32         iErr;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    eCode = Check_TunerID(pPrivate, ulTunerID);
    if (eCode != DxB_ERR_OK)
        return eCode;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle == NULL)
        return OMX_ErrorInvalidComponent;

    piParam[0] = (OMX_S32) ulTunerID;
    piParam[1] = (OMX_S32) ulArg;

    iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamSetTone, piParam);

    if (iErr != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_TCC_ERR, "[%s] Error(%d)", __func__, iErr);
        return DxB_ERR_ERROR;
    }
    return DxB_ERR_OK;
}

/*
 * DVB-S Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_DiseqcSendBurst(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulCMD)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    OMX_S32       piParam[2];
    DxB_ERR_CODE  eCode;
    INT32         iErr;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    eCode = Check_TunerID(pPrivate, ulTunerID);
    if (eCode != DxB_ERR_OK)
        return eCode;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle == NULL)
        return OMX_ErrorInvalidComponent;

    piParam[0] = (OMX_S32) ulTunerID;
    piParam[1] = (OMX_S32) ulCMD;

    iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamDiSEqCSendBurst, piParam);

    if (iErr != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_TCC_ERR, "[%s] Error(%d)", __func__, iErr);
        return DxB_ERR_ERROR;
    }
    return DxB_ERR_OK;
}

/*
 * DVB-S Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_DiseqcSendCMD(DxBInterface *hInterface, UINT32 ulTunerID, unsigned char *pucCMD, UINT32 ulLen)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    OMX_S32       piParam[2];
    DxB_ERR_CODE  eCode;
    INT32         iErr;
    INT32         piArg[2];

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    eCode = Check_TunerID(pPrivate, ulTunerID);
    if (eCode != DxB_ERR_OK)
        return eCode;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle == NULL)
        return OMX_ErrorInvalidComponent;

    piParam[0] = (OMX_S32) ulTunerID;
    piParam[1] = (OMX_S32) piArg;

    piArg[0] = (INT32) pucCMD;
    piArg[1] = (INT32) ulLen;

    iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamDiSEqCSendCMD, piParam);

    if (iErr != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_TCC_ERR, "[%s] Error(%d)", __func__, iErr);
        return DxB_ERR_ERROR;
    }
    return DxB_ERR_OK;
}

/*
 * DVB-S Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_BlindScanReset(DxBInterface *hInterface, UINT32 ulTunerID)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    OMX_S32       piParam[2];
    DxB_ERR_CODE  eCode;
    INT32         iErr;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    eCode = Check_TunerID(pPrivate, ulTunerID);
    if (eCode != DxB_ERR_OK)
        return eCode;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle == NULL)
        return OMX_ErrorInvalidComponent;

    piParam[0] = (OMX_S32) ulTunerID;

    iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamBlindScanReset, piParam);

    if (iErr != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_TCC_ERR, "[%s] Error(%d)", __func__, iErr);
        return DxB_ERR_ERROR;
    }
    return DxB_ERR_OK;
}

/*
 * DVB-S Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_BlindScanStart(DxBInterface *hInterface, UINT32 ulTunerID)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    OMX_S32       piParam[2];
    DxB_ERR_CODE  eCode;
    INT32         iErr;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    eCode = Check_TunerID(pPrivate, ulTunerID);
    if (eCode != DxB_ERR_OK)
        return eCode;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle == NULL)
        return OMX_ErrorInvalidComponent;

    piParam[0] = (OMX_S32) ulTunerID;

    iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamBlindScanStart, piParam);

    if (iErr != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_TCC_ERR, "[%s] Error(%d)", __func__, iErr);
        return DxB_ERR_ERROR;
    }
    return DxB_ERR_OK;
}

/*
 * DVB-S Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_BlindScanCancel(DxBInterface *hInterface, UINT32 ulTunerID)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    OMX_S32       piParam[2];
    DxB_ERR_CODE  eCode;
    INT32         iErr;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    eCode = Check_TunerID(pPrivate, ulTunerID);
    if (eCode != DxB_ERR_OK)
        return eCode;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle == NULL)
        return OMX_ErrorInvalidComponent;

    piParam[0] = (OMX_S32) ulTunerID;

    iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamBlindScanCancel, piParam);

    if (iErr != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_TCC_ERR, "[%s] Error(%d)", __func__, iErr);
        return DxB_ERR_ERROR;
    }
    return DxB_ERR_OK;
}

/*
 * DVB-S Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_BlindScanGetState(DxBInterface *hInterface, UINT32 ulTunerID, unsigned int *puiState)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    OMX_S32       piParam[2];
    DxB_ERR_CODE  eCode;
    INT32         iErr;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    eCode = Check_TunerID(pPrivate, ulTunerID);
    if (eCode != DxB_ERR_OK)
        return eCode;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle == NULL)
        return OMX_ErrorInvalidComponent;

    piParam[0] = (OMX_S32) ulTunerID;
    piParam[1] = (OMX_S32) puiState;

    iErr = OMX_GetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamBlindScanState, piParam);

    if (iErr != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_TCC_ERR, "[%s] Error(%d)", __func__, iErr);
        return DxB_ERR_ERROR;
    }
    return DxB_ERR_OK;
}

/*
 * DVB-S Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_BlindScanGetInfo(DxBInterface *hInterface, UINT32 ulTunerID, unsigned int *puiInfo)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    OMX_S32       piParam[2];
    DxB_ERR_CODE  eCode;
    INT32         iErr;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    eCode = Check_TunerID(pPrivate, ulTunerID);
    if (eCode != DxB_ERR_OK)
        return eCode;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle == NULL)
        return OMX_ErrorInvalidComponent;

    piParam[0] = (OMX_S32) ulTunerID;
    piParam[1] = (OMX_S32) puiInfo;

    iErr = OMX_GetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamBlindScanInfo, piParam);

    if (iErr != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_TCC_ERR, "[%s] Error(%d)", __func__, iErr);
        return DxB_ERR_ERROR;
    }
    return DxB_ERR_OK;
}

/*
 * T-DMB Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_Init_Ex (DxBInterface *hInterface, UINT32 ulStandard, UINT32 ulTunerType, UINT32 ulTunerID)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if(ulTunerID >= THE_NUMBER_OF_TUNER)
        return DxB_ERR_ERROR;

	pPrivate = TCC_fo_malloc(__func__, __LINE__,sizeof(TCC_DxB_TUNER_PRIVATE));
	if(pPrivate == NULL){
		return DxB_ERR_ERROR;
	}
	
	hInterface->iTunerType    = ulTunerType;
	hInterface->pTunerPrivate = pPrivate;

	pPrivate->uiTunerErrorIndicator[ulTunerID] = 0;

	pPrivate->iDMBServiceMode[ulTunerID] = TDMB_PLAY_MODE;

    return DxB_ERR_OK;
}

/*
 * T-DMB Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_SetChannel_Ex(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulChannel, UINT32 lockOn, UINT32 ulSubChCtrl)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    OMX_S32       piParam[2];
    INT32         iErr;
    INT32         piArg[3];

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    INT32 *piChInfo;
    int func_callback_addr;

    iErr = Check_TunerID(pPrivate, ulTunerID);
    if (iErr != OMX_ErrorNone)
        return iErr;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle == NULL)
        return OMX_ErrorInvalidComponent;

    piParam[0] = (OMX_S32) ulTunerID;
    piParam[1] = (OMX_S32) piArg;

    piArg[0] = (INT32)ulChannel;
    piArg[1] = (INT32)lockOn;
    piArg[2] = (INT32)ulSubChCtrl;        // 0(unreg old sub channel), 1(reg new sub channel), other(default-frequency set)
    piChInfo = (INT32 *)piArg[0];

    iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamTunerChannelSet, piParam);

    if( tcc_omx_get_dxb_type(OMX_Dxb_Dec_AppPriv) == DxB_STANDARD_TDMB && ulSubChCtrl != 0)
    {
        /*
        ContentsType = piChInfo[11];
        ModuleIdx = piChInfo[12];
        PlayMode = piChInfo[13];
        */
        if (ulSubChCtrl == (UINT32)-1)
        {
            if(piChInfo[12] == 0)
            {
                if(OMX_Dxb_Dec_AppPriv->demuxhandle)
                    iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBSetBitrate, (OMX_PTR)ulChannel);
                if(OMX_Dxb_Dec_AppPriv->tunerhandle)
                    iErr = OMX_GetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamSetGetPacketDataEntry, &func_callback_addr);
                if(OMX_Dxb_Dec_AppPriv->demuxhandle)
                {
                    iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamSetGetPacketDataEntry, (OMX_PTR)func_callback_addr);
                    if(pPrivate->iDMBServiceMode[ulTunerID] == TDMB_TSDUMP_MODE)
                    {
                        iErr = OMX_GetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamTunerSetDMBServiceMode, &func_callback_addr);
                        iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamTunerSetDMBServiceMode, (OMX_PTR)func_callback_addr);
                    }
                    else
                    {
                        func_callback_addr = 0;
                        iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamTunerSetDMBServiceMode, (OMX_PTR)func_callback_addr);
                    }

                    iErr = OMX_GetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamSetGetSignalStrengthEntry, &func_callback_addr);
                    iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamSetGetSignalStrengthEntry, (OMX_PTR)func_callback_addr);
                }
            }
        }
#if 1
// Noah, To avoid CodeSonar warning, Redundant Condition
// "ulSubChCtrl != 0" is always true because the condition has been already checked above.
		else
#else
        else if (ulSubChCtrl != 0)
#endif			
        {
            if(piChInfo[11] != 3/*CONTENTS_TYPE_DATA*/ && piChInfo[13] == 0/*DXBPROC_MODE_PLAYBACK*/)
            {
                if(OMX_Dxb_Dec_AppPriv->demuxhandle)
                    iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBSetBitrate, (OMX_PTR)ulChannel);
                if(OMX_Dxb_Dec_AppPriv->tunerhandle)
                    iErr = OMX_GetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamSetGetPacketDataEntry, &func_callback_addr);
                if(OMX_Dxb_Dec_AppPriv->demuxhandle)
                {
                    INT32 pParam[2] = {piChInfo[12], func_callback_addr};
                    iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamSetGetPacketDataEntryEx, pParam);
                    if(pPrivate->iDMBServiceMode[ulTunerID] == TDMB_TSDUMP_MODE)
                    {
                        iErr = OMX_GetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamTunerSetDMBServiceMode, &func_callback_addr);
                        iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamTunerSetDMBServiceMode, (OMX_PTR)func_callback_addr);
                    }
                    else
                    {
                        func_callback_addr = 0;
                        iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamTunerSetDMBServiceMode, (OMX_PTR)func_callback_addr);
                    }

                    iErr = OMX_GetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamSetGetSignalStrengthEntry, &func_callback_addr);
                    iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamSetGetSignalStrengthEntry, (OMX_PTR)func_callback_addr);
                }
            }
        }
    }

    if (iErr != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_TCC_ERR, "[%s] Error(%d)", __func__, iErr);
        return DxB_ERR_ERROR;
    }
    return DxB_ERR_OK;
}

/*
 * T-DMB Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_ScanChannel(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulChannel)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    INT32 ierr;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    ierr = Check_TunerID(pPrivate, ulTunerID);
    if(ierr != OMX_ErrorNone )
        return ierr;

    if(OMX_Dxb_Dec_AppPriv->tunerhandle)
        ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle,OMX_IndexVendorParamTunerChannelScan, (OMX_PTR)ulChannel);
    else
        ierr = OMX_ErrorInvalidComponent;

    if(ierr != OMX_ErrorNone )
        return DxB_ERR_ERROR;

    return DxB_ERR_OK;
}

/*
 * T-DMB Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_ScanFreqChannel(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulChannel)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    INT32 ierr;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    ierr = Check_TunerID(pPrivate, ulTunerID);
    if(ierr != OMX_ErrorNone )
        return ierr;

    if(OMX_Dxb_Dec_AppPriv->tunerhandle)
        ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle,OMX_IndexVendorParamTunerFreqScan, (OMX_PTR)ulChannel);
    else
        ierr = OMX_ErrorInvalidComponent;

    if(ierr != OMX_ErrorNone )
        return DxB_ERR_ERROR;
    return DxB_ERR_OK;
}

/*
 * T-DMB Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_DataStreamStop(DxBInterface *hInterface, UINT32 ulTunerID)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    INT32  ierr;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    ierr = Check_TunerID(pPrivate, ulTunerID);
    if(ierr != OMX_ErrorNone )
        return ierr;

    if(OMX_Dxb_Dec_AppPriv->tunerhandle)
        ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle,OMX_IndexVendorParamTunerPlayStop, (OMX_PTR)ulTunerID);
    else
        ierr = OMX_ErrorInvalidComponent;

    if(ierr != OMX_ErrorNone )
        return DxB_ERR_ERROR;

    return DxB_ERR_OK;
}

/*
 * T-DMB Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_DMBService_Mode(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 mode)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    int func_callback_addr = 0;
    pPrivate->iDMBServiceMode[ulTunerID] = mode;

    if(OMX_Dxb_Dec_AppPriv->demuxhandle)
    {
        if(pPrivate->iDMBServiceMode[ulTunerID] == TDMB_TSDUMP_MODE)
        {
            OMX_GetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamTunerSetDMBServiceMode, &func_callback_addr);
            OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamTunerSetDMBServiceMode, (OMX_PTR)func_callback_addr);
        }
        else
        {
            OMX_SetParameter(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamTunerSetDMBServiceMode, (OMX_PTR)func_callback_addr);
        }
    }

    return DxB_ERR_OK;
}
#endif

/*
 * T-DMB & ISDB-T Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_SetCountrycode (DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulCountrycode)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    INT32  ierr;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    ierr = Check_TunerID(pPrivate, ulTunerID);
    if(ierr != OMX_ErrorNone )
        return ierr;

    if(OMX_Dxb_Dec_AppPriv->tunerhandle)
        ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamTunerCountryCodeSet,  (OMX_PTR)ulCountrycode);
    else
        ierr = OMX_ErrorInvalidComponent;

    if(ierr != OMX_ErrorNone )
        return DxB_ERR_ERROR;

    return DxB_ERR_OK;
}

/*
 * ISDB-T Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_GetChannelValidity (DxBInterface *hInterface, UINT32 ulTunerID, int *pChannel)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    INT32 err = DxB_ERR_ERROR;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    err = Check_TunerID(pPrivate, ulTunerID);
    if(err != OMX_ErrorNone )
        return err;

    if (pChannel != NULL) {
        if (OMX_Dxb_Dec_AppPriv->tunerhandle)
            err = OMX_GetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamTunerGetChannelValidity, pChannel);
        else
            err = OMX_ErrorInvalidComponent;

        if (err != OMX_ErrorNone)
            err = DxB_ERR_ERROR;
    }
    return err;
}

/*
 * ISDB-T Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_GetSignalStrengthIndex (DxBInterface *hInterface, UINT32 ulTunerID, int *strength_index)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    INT32 ierr;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    ierr = Check_TunerID(pPrivate, ulTunerID);
    if(ierr != OMX_ErrorNone )
        return ierr;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle)
        ierr = OMX_GetParameter (OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamGetSignalStrengthIndex, strength_index);
    else
        ierr = OMX_ErrorInvalidComponent;

    if(ierr != OMX_ErrorNone)
        return DxB_ERR_ERROR;
    return DxB_ERR_OK;
}

/*
 * ISDB-T Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_GetSignalStrengthIndexTime (DxBInterface *hInterface, UINT32 ulTunerID, int *tuner_str_idx_time)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    INT32 ierr;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    ierr = Check_TunerID(pPrivate, ulTunerID);
    if(ierr != OMX_ErrorNone )
        return ierr;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle)
        ierr = OMX_GetParameter (OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamGetSignalStrengthIndexTime, tuner_str_idx_time);
    else
        ierr = OMX_ErrorInvalidComponent;

    if(ierr != OMX_ErrorNone)
        return DxB_ERR_ERROR;
    return DxB_ERR_OK;
}

/*
 * ISDB-T Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_Get_EWS_Flag(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 *ulStartFlag)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    INT32 ierr;

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    ierr = Check_TunerID(pPrivate, ulTunerID);
    if(ierr != OMX_ErrorNone )
        return ierr;

    if(OMX_Dxb_Dec_AppPriv->tunerhandle)
        ierr = OMX_GetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle,OMX_IndexVendorParamTunerGetEWSFlag, ulStartFlag);
    else
        ierr = OMX_ErrorInvalidComponent;

    if(ierr != OMX_ErrorNone )
        return DxB_ERR_ERROR;

    return DxB_ERR_OK;
}

/*
 * ISDB-T Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_CasOpen(DxBInterface *hInterface, unsigned char _casRound, unsigned char * _systemKey)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    unsigned char pcArg[60];

    if (hInterface->pTunerPrivate == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pTunerPrivate is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    pcArg[0] = _casRound;
    memcpy(&pcArg[1], _systemKey, 32);

    if(OMX_Dxb_Dec_AppPriv->tunerhandle)
    {
        OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle,OMX_IndexVendorParamTunerSetCasOpen,pcArg);
        return DxB_ERR_OK;
    }
    else
    {
        return DxB_ERR_ERROR;
    }

}

/*
 * ISDB-T Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_CasKeyMulti2(DxBInterface *hInterface, unsigned char _parity, unsigned char *_key, unsigned char _keyLength, unsigned char *_initVector, unsigned char _initVectorLength)
{
	//TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    unsigned char pcArg[60];

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    pcArg[0] = _parity;
    memcpy(&pcArg[1], _key, 8);

    pcArg[16] = _initVectorLength;
    if(_initVectorLength)
        memcpy(&pcArg[17], _initVector, 8);

    if(OMX_Dxb_Dec_AppPriv->tunerhandle)
    {
        OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle,OMX_IndexVendorParamTunerSetCasSetmulti2,pcArg);
        return DxB_ERR_OK;
    }
    else
    {
        return DxB_ERR_ERROR;
    }
}

/*
 * ISDB-T Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_CasSetPID(DxBInterface *hInterface, unsigned int *_pids, unsigned int _numberOfPids)
{
	//TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    unsigned int puiArg[6];

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    puiArg[0] = _numberOfPids;
    memcpy(&puiArg[1], _pids, sizeof(unsigned int)*4);

    if(OMX_Dxb_Dec_AppPriv->tunerhandle)
    {
        OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle,OMX_IndexVendorParamTunerSetCasSetPid,puiArg);
        return DxB_ERR_OK;
    }
    else
    {
        return DxB_ERR_ERROR;
    }
}

/*
 * ISDB-T Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_SetFreqBand (DxBInterface *hInterface, UINT32 ulTunerID, int freq_band)
{
	//TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    INT32  iErr;
    INT32  piArg[2];

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if(OMX_Dxb_Dec_AppPriv->tunerhandle)
           iErr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamTunerSetFreqBand,  (OMX_PTR)freq_band);
    else
        iErr = OMX_ErrorInvalidComponent;

    if(iErr != OMX_ErrorNone )
        return DxB_ERR_ERROR;

       return DxB_ERR_OK;
}

/*
 * ISDB-T Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_SetCustomTuner(DxBInterface *hInterface, UINT32 ulTunerID, int size, void *arg)
{
	//TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    INT32         iErr;
    INT32         piArg[2];

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    piArg[0] = (INT32)size;
    piArg[1] = (INT32)arg;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle)
        return OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamTunerCustom, piArg);
    else
        return DxB_ERR_ERROR;
}

/*
 * ISDB-T Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_GetCustomTuner(DxBInterface *hInterface, UINT32 ulTunerID, int *size, void *arg)
{
	//TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    INT32         iErr;
    INT32         piArg[2];

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    piArg[0] = (INT32)size;
    piArg[1] = (INT32)arg;

    if (OMX_Dxb_Dec_AppPriv->tunerhandle)
        return OMX_GetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamTunerCustom, piArg);
    else
        return DxB_ERR_ERROR;
}

/*
 * ISDB-T Only.
 */
DxB_ERR_CODE TCC_DxB_TUNER_UserLoopStopCmd(DxBInterface *hInterface, UINT32 ulModuleIndex)
{
	//TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;

    if (hInterface->pOpenMaxIL == NULL)
    {
	    DEBUG(DEB_LEV_ERR, "[%s] hInterface->pOpenMaxIL is NULL !!!!!", __func__);
        return DxB_ERR_INVALID_PARAM;
    }

    if (OMX_Dxb_Dec_AppPriv->tunerhandle)
	    return OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle, OMX_IndexVendorParamTunerUserLoopStopCmd, (OMX_PTR)ulModuleIndex);
    else
        return DxB_ERR_ERROR;
}

/*
 * Not Used.
 */
#if 0
DxB_ERR_CODE TCC_DxB_TUNER_GetChannelIndexByFrequency(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulFrequencyKhz, UINT32 *pulChannelIndex)
{
	TCC_DxB_TUNER_PRIVATE *pPrivate = hInterface->pTunerPrivate;
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = hInterface->pOpenMaxIL;
    INT32  ierr;
    INT32  piArg[2];

    ierr = Check_TunerID(pPrivate, ulTunerID);
    if(ierr != OMX_ErrorNone )
        return ierr;

    piArg[0] = (INT32) ulFrequencyKhz;
    piArg[1] = (INT32) pulChannelIndex;

    if(OMX_Dxb_Dec_AppPriv->tunerhandle)
        ierr = OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle,OMX_IndexVendorParamTunerGetChannelIndexByFrequency,piArg);
    else
        ierr = OMX_ErrorInvalidComponent;

    if(ierr != OMX_ErrorNone )
		return DxB_ERR_ERROR;
    return DxB_ERR_OK;
}
#endif

/*
 * ISDB-T Only.
 * Noah, 20170201, JK's request, IM478A-45
 * 
 * Description
 *     - This function is to send custom tuner event to Application through EVENT_UPDATE_CUSTOM_TUNER.
 *
 * Parameter
 *     pCustomEventData
 *         - NULL is no problem.
 *         - If not NULL, it must be a memory allocated and the memory must be freed by Application.
 *         - It is sent to Application through EVENT_UPDATE_CUSTOM_TUNER event.
 *
 * Return
 *     It is always DxB_ERR_OK.
 */
DxB_ERR_CODE TCC_DxB_TUNER_SendCustomEventToApp(void* pCustomEventData)
{
	DEBUG(DEB_LEV_TCC_ERR, "[%s] start / pCustomEventData is %s\n", __func__, pCustomEventData == NULL ? "NULL" : "not NULL");
	
	tcc_tuner_event(0, pCustomEventData, 0);

	return DxB_ERR_OK;
}

