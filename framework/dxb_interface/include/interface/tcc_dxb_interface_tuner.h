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
#ifndef _TCC_DXB_INTERFACE_TUNER_H_
#define _TCC_DXB_INTERFACE_TUNER_H_

#include "tcc_dxb_interface_type.h"

/*******************************************************************/
/****************************** typedef ****************************/
/*******************************************************************/

typedef enum
{
	DxB_TUNER_EVENT_NOTIFY_LOCK_TIMEOUT, 
	DxB_TUNER_EVENT_NOTIFY_SIGNAL_STATUS, 
	DxB_TUNER_EVENT_NOTIFY_NO_SIGNAL, 
	DxB_TUNER_EVENT_NOTIFY_SIGNAL, 
	DxB_TUNER_EVENT_NOTIFY_SEARCHED_CHANNEL,
	DxB_TUNER_EVENT_NOTIFY_SET_CHANNEL,
	DxB_TUNER_EVENT_NOTIFY_UPDATE_EWS,
	DxB_TUNER_EVENT_NOTIFY_UPDATE_DATA,
	DxB_TUNER_EVENT_NOTIFY_UPDATE_TSDATA,
	DxB_TUNER_EVENT_NOTIFY_END
} DxB_TUNER_NOTIFY_EVT;

typedef void (*pfnDxB_TUNER_EVENT_Notify)(DxB_TUNER_NOTIFY_EVT nEvent, void *pCallbackData, void *pUserData);
/*******************************************************************/
/****************************** functions ****************************/
/*******************************************************************/
DxB_ERR_CODE TCC_DxB_TUNER_Init(DxBInterface *hInterface, UINT32 ulStandard, UINT32 ulTunerType);
DxB_ERR_CODE TCC_DxB_TUNER_Deinit(DxBInterface *hInterface);
DxB_ERR_CODE TCC_DxB_TUNER_RegisterEventCallback(DxBInterface *hInterface, pfnDxB_TUNER_EVENT_Notify pfnEventCallback, void *pUserData);
DxB_ERR_CODE TCC_DxB_TUNER_SendEvent(DxBInterface *hInterface, DxB_TUNER_NOTIFY_EVT nEvent, void *pCallbackData);

DxB_ERR_CODE TCC_DxB_TUNER_SetNumberOfBB(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulNumberOfBB);
DxB_ERR_CODE TCC_DxB_TUNER_Open(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 *pulArg);
DxB_ERR_CODE TCC_DxB_TUNER_Close(DxBInterface *hInterface, UINT32 ulTunerID);
DxB_ERR_CODE TCC_DxB_TUNER_SetChannel(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulChannel, UINT32 lockOn);
DxB_ERR_CODE TCC_DxB_TUNER_GetLockStatus (DxBInterface *hInterface, UINT32 ulTunerID, int *pLockStatus);
DxB_ERR_CODE TCC_DxB_TUNER_GetChannelInfo(DxBInterface *hInterface, UINT32 ulTunerID, void *pChannelInfo);
DxB_ERR_CODE TCC_DxB_TUNER_GetSignalStrength(DxBInterface *hInterface, UINT32 ulTunerID, void *pStrength);
DxB_ERR_CODE TCC_DxB_TUNER_RegisterPID(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulPID);
DxB_ERR_CODE TCC_DxB_TUNER_UnRegisterPID(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulPID);

// For ATSC
DxB_ERR_CODE TCC_DxB_TUNER_SearchAnalogChannel(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulChannel);
DxB_ERR_CODE TCC_DxB_TUNER_SearchChannel(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulChannel);
DxB_ERR_CODE TCC_DxB_TUNER_SetModulation (DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulModulation);

// For DVB
DxB_ERR_CODE TCC_DxB_TUNER_SearchFrequencyWithBW(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulFrequencyKhz, UINT32 ulBandWidthKhz, UINT32 ulLockWait, UINT32 *pOptions,  UINT32 *pSnr);
DxB_ERR_CODE TCC_DxB_TUNER_SetAntennaPower(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulArg);
DxB_ERR_CODE TCC_DxB_TUNER_GetRFInformation(DxBInterface *hInterface, UINT32 ulTunerID, UINT8 *pvData, UINT32 *puiSize);

// For DVB-T2
DxB_ERR_CODE TCC_DxB_TUNER_GetDataPLPs(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 *pulPLPId, UINT32 *pulPLPNum);
DxB_ERR_CODE TCC_DxB_TUNER_SetDataPLP(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulPLPId);

// For DVB-S
DxB_ERR_CODE TCC_DxB_TUNER_SetLNBVoltage(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulArg);
DxB_ERR_CODE TCC_DxB_TUNER_SetTone(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulArg);
DxB_ERR_CODE TCC_DxB_TUNER_DiseqcSendBurst(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulCMD);
DxB_ERR_CODE TCC_DxB_TUNER_DiseqcSendCMD(DxBInterface *hInterface, UINT32 ulTunerID, unsigned char *pucCMD, UINT32 ulLen);
DxB_ERR_CODE TCC_DxB_TUNER_BlindScanReset(DxBInterface *hInterface, UINT32 ulTunerID);
DxB_ERR_CODE TCC_DxB_TUNER_BlindScanStart(DxBInterface *hInterface, UINT32 ulTunerID);
DxB_ERR_CODE TCC_DxB_TUNER_BlindScanCancel(DxBInterface *hInterface, UINT32 ulTunerID);
DxB_ERR_CODE TCC_DxB_TUNER_BlindScanGetState(DxBInterface *hInterface, UINT32 ulTunerID, unsigned int *puiState);
DxB_ERR_CODE TCC_DxB_TUNER_BlindScanGetInfo(DxBInterface *hInterface, UINT32 ulTunerID, unsigned int *puiInfo);

// For T-DMB
DxB_ERR_CODE TCC_DxB_TUNER_Init_Ex (DxBInterface *hInterface, UINT32 ulStandard, UINT32 ulTunerType, UINT32 ulTunerID);
DxB_ERR_CODE TCC_DxB_TUNER_SetChannel_Ex(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulChannel, UINT32 lockOn, UINT32 ulSubChCtrl);
DxB_ERR_CODE TCC_DxB_TUNER_ScanChannel(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulChannel);
DxB_ERR_CODE TCC_DxB_TUNER_ScanFreqChannel(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulChannel);
DxB_ERR_CODE TCC_DxB_TUNER_DataStreamStop(DxBInterface *hInterface, UINT32 ulTunerID);
DxB_ERR_CODE TCC_DxB_TUNER_DMBService_Mode(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 mode);

// For T-DMB & ISDB-T
DxB_ERR_CODE TCC_DxB_TUNER_SetCountrycode (DxBInterface *hInterface, UINT32 ulTunerID, UINT32 ulCountrycode);

// For ISDB-T
DxB_ERR_CODE TCC_DxB_TUNER_GetChannelValidity (DxBInterface *hInterface, UINT32 ulTunerID, int *pChannel);
DxB_ERR_CODE TCC_DxB_TUNER_GetSignalStrengthIndex (DxBInterface *hInterface, UINT32 ulTunerID, int *strength_index);
DxB_ERR_CODE TCC_DxB_TUNER_GetSignalStrengthIndexTime (DxBInterface *hInterface, UINT32 ulTumerID, int *tuner_str_idx_time);
DxB_ERR_CODE TCC_DxB_TUNER_Get_EWS_Flag(DxBInterface *hInterface, UINT32 ulTunerID, UINT32 *ulStartFlag);
DxB_ERR_CODE TCC_DxB_TUNER_CasOpen(DxBInterface *hInterface, unsigned char _casRound, unsigned char * _systemKey);
DxB_ERR_CODE TCC_DxB_TUNER_CasKeyMulti2(DxBInterface *hInterface, unsigned char _parity, unsigned char *_key, unsigned char _keyLength, unsigned char *_initVector, unsigned char _initVectorLength);
DxB_ERR_CODE TCC_DxB_TUNER_CasSetPID(DxBInterface *hInterface, unsigned int *_pids, unsigned int _numberOfPids);
DxB_ERR_CODE TCC_DxB_TUNER_SetFreqBand (DxBInterface *hInterface, UINT32 ulTunerID, int freq_band);
DxB_ERR_CODE TCC_DxB_TUNER_SetCustomTuner(DxBInterface *hInterface, UINT32 ulTunerID, int size, void *arg);
DxB_ERR_CODE TCC_DxB_TUNER_GetCustomTuner(DxBInterface *hInterface, UINT32 ulTunerID, int *size, void *arg);
DxB_ERR_CODE TCC_DxB_TUNER_UserLoopStopCmd(DxBInterface *hInterface, UINT32 ulModuleIndex);

/*
 * ISDB-T Only.
 * 
 * Description
 *     - This function is to send custom tuner event to Application through EVENT_UPDATE_CUSTOM_TUNER.
 *
 * Parameter
 *     pCustomEventData
 *         - NULL is no problem.
 *         - If not NULL, it must be a memory allocated and the memory must be freed by Application.
 *         - It is sent to Application through EVENT_UPDATE_CUSTOM_TUNER event.
 */
DxB_ERR_CODE TCC_DxB_TUNER_SendCustomEventToApp(void* pCustomEventData);

#endif
