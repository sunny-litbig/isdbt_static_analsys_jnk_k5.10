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
#ifndef _TCC_DXB_INTERFACE_DEMUX_H_
#define _TCC_DXB_INTERFACE_DEMUX_H_
#include "tcc_dxb_interface_type.h"

#define	PID_BITMASK_GEN(X) (0x00000001 << X)
#define PID_BITMASK_PCR  PID_BITMASK_GEN(0)
#define PID_BITMASK_VIDEO_MAIN PID_BITMASK_GEN(1)
#define PID_BITMASK_VIDEO_SUB PID_BITMASK_GEN(2)
#define PID_BITMASK_AUDIO_MAIN PID_BITMASK_GEN(3)
#define PID_BITMASK_AUDIO_SUB PID_BITMASK_GEN(4)
#define PID_BITMASK_AUDIO_SPDIF PID_BITMASK_GEN(5)
#define	PID_BITMASK_PCR_SUB	PID_BITMASK_GEN(6)

typedef	enum DxB_ELEMENTARYSTREAM
{
    STREAMTYPE_NONE=0x00, 
    STREAMTYPE_MPEG1_VIDEO=0x01, 
    STREAMTYPE_VIDEO=0x02, 
    STREAMTYPE_AUDIO1=0x03, // MPEG-1 Audio
    STREAMTYPE_AUDIO2=0x04, // MPEG-2 Audio
    STREAMTYPE_PRIVATE=0x06, 
    STREAMTYPE_AUDIO_AAC=0x0f, 
    STREAMTYPE_AUDIO_AAC_LATM = 0x11,				 /*0x11 ISO/IEC 14496-3(MPEG-4 AAC)*/
    STREAMTYPE_AUDIO_AAC_PLUS = 0x12,
    STREAMTYPE_VIDEO_AVCH264=0x1b, 
    STREAMTYPE_AUDIO_BSAC = 0x22,					 /*0x22 ISO/IEC 14496-3 ER BASC Audio Object*/
    STREAMTYPE_AUDIO_AC3_1=0x80, 
    STREAMTYPE_AUDIO_AC3_2=0x81, // AC3
}DxB_ELEMENTARYSTREAM;

typedef enum DxB_DEMUX_PESTYPE
{
	DxB_DEMUX_PESTYPE_SUBTITLE_0 = 0,
	DxB_DEMUX_PESTYPE_SUBTITLE_1,
	DxB_DEMUX_PESTYPE_TELETEXT_0,
	DxB_DEMUX_PESTYPE_TELETEXT_1,
	DxB_DEMUX_PESTYPE_USERDEFINE,
	DxB_DEMUX_PESTYPE_MAX
} DxB_DEMUX_PESTYPE;

#define DxB_DEMUX_PESTYPE_SUBTITLE  DxB_DEMUX_PESTYPE_SUBTITLE_0
#define DxB_DEMUX_PESTYPE_TELETEXT  DxB_DEMUX_PESTYPE_TELETEXT_0

typedef enum DxB_DEMUX_TSSTATE
{
	DxB_DMX_TS_Scramble,
	DxB_DMX_TS_Clean,
	DxB_DMX_TS_UnKnown
}DxB_DEMUX_TSSTATE;

typedef struct DxB_PID_INFO
{
	UINT32 pidBitmask;
	UINT16 usPCRPid;
	UINT16 usPCRSubPid;
	UINT16 usVideoMainPid;
	UINT16 usVideoSubPid;
	INT16 usVideoMainType;
	INT16 usVideoSubType;
	UINT16 usAudioMainPid;
	UINT16 usAudioSubPid;
	INT16 usAudioMainType;
	INT16 usAudioSubType;
	UINT16 usAudioSpdifPid;
} DxB_Pid_Info;

typedef enum
{
	DxB_DEMUX_NOTIFY_FIRSTFRAME_DISPLAYED,
	DxB_DEMUX_EVENT_RECORDING_STOP,
	DxB_DEMUX_EVENT_CAS_CHANNELCHANGE,
	DxB_DEMUX_EVENT_REGISTER_PID,
	DxB_DEMUX_EVENT_UNREGISTER_PID,
	DxB_DEMUX_EVENT_SCRAMBLED_STATUS,
	DxB_DEMUX_EVENT_AUDIO_CODEC_INFORMATION,
	DxB_DEMUX_EVENT_DLS_UPDATE,
	DxB_DEMUX_EVENT_PACKET_STATUS_UPDATE,
	DxB_DEMUX_EVENT_NOTIFY_END
} DxB_DEMUX_NOTIFY_EVT;

/* For A/V sync parameters and returns
 */
enum
{
    DxB_SYNC_AUDIO,
    DxB_SYNC_VIDEO,
};

enum
{
    DxB_SYNC_DROP = -1,
    DxB_SYNC_OK = 0,
    DxB_SYNC_WAIT = 1,
    DxB_SYNC_BYPASS = 2,
    DxB_SYNC_LONG_WAIT = 3,
};
/////////////////////////////////////
//
typedef INT64			(*pfnDxB_DEMUX_GetSTC)(INT32 itype, INT64 lpts, unsigned int index, int log, void *pvApp);
typedef int				(*pfnDxB_DEMUX_UpdatePCR)(INT32 itype, int index, INT64 lpts, void *pvApp);
typedef int				(*pfnDxB_DEMUX_UpdateBuffer)(UINT8 *pucBuff, INT32 iSize, void *pvApp);

/////////////////////////////////////
//
typedef DxB_ERR_CODE	(*pfnDxB_DEMUX_PES_Notify)(UINT8 *pucBuf, UINT32 ulBufSize, UINT64 ullPTS, UINT32 ulRequestID, void *pUserData);
typedef DxB_ERR_CODE	(*pfnDxB_DEMUX_Notify)(UINT8 *pucBuf, UINT32 ulBufsize, UINT32 ulRequestId, void *pUserData);
typedef DxB_ERR_CODE	(*pfnDxB_DEMUX_AllocBuffer)(UINT32 usBufLen, UINT8 **ppucBuf);
typedef DxB_ERR_CODE	(*pfnDxB_DEMUX_FreeBufferForError)( UINT8 *ppucBuf);
typedef DxB_ERR_CODE	(*pfnDxB_DEMUX_NotifyEx)(UINT32 SectionHandle, UINT8 *pucBuf, UINT32 size, UINT32 ulPid, UINT32 ulRequestId, void *pUserData);
typedef void            (*pfnDxB_DEMUX_EVENT_Notify)(DxB_DEMUX_NOTIFY_EVT nEvent, void *pCallbackData, void *pUserData);
typedef int             (*pfnDxb_DEMUX_CasDecrypt)(unsigned int type, unsigned char *pBuf, unsigned int uiSize, void *pUserData);


/********************************************************************************************/
/********************************************************************************************
						FOR MW LAYER FUNCTION
********************************************************************************************/
/********************************************************************************************/
DxB_ERR_CODE TCC_DxB_DEMUX_Init(DxBInterface *hInterface, DxB_STANDARDS Standard, UINT32 ulDevId, UINT32 ulBaseBandType);
DxB_ERR_CODE TCC_DxB_DEMUX_Deinit(DxBInterface *hInterface);

DxB_ERR_CODE TCC_DxB_DEMUX_GetNumOfDemux(DxBInterface *hInterface, UINT32 *ulNumOfDemux);
DxB_ERR_CODE TCC_DxB_DEMUX_StartAVPID(DxBInterface *hInterface, DxB_Pid_Info * ppidInfo);
DxB_ERR_CODE TCC_DxB_DEMUX_StopAVPID(DxBInterface *hInterface, UINT32 pidBitmask);
DxB_ERR_CODE TCC_DxB_DEMUX_ChangeAVPID(DxBInterface *hInterface, UINT32 pidBitmask, DxB_Pid_Info * ppidInfo);
DxB_ERR_CODE TCC_DxB_DEMUX_RegisterPESCallback(DxBInterface *hInterface,
											DxB_DEMUX_PESTYPE etPesType,
											pfnDxB_DEMUX_PES_Notify pfnNotify,
											pfnDxB_DEMUX_AllocBuffer pfnAllocBuffer,
											pfnDxB_DEMUX_FreeBufferForError pfnErrorFreeBuffer);
DxB_ERR_CODE TCC_DxB_DEMUX_StartPESFilter(DxBInterface *hInterface, UINT16 ulPid, DxB_DEMUX_PESTYPE etPESType, UINT32 ulRequestID, void *pUserData);
DxB_ERR_CODE TCC_DxB_DEMUX_StopPESFilter(DxBInterface *hInterface, UINT32 ulRequestID);
DxB_ERR_CODE TCC_DxB_DEMUX_RegisterSectionCallback(DxBInterface *hInterface, pfnDxB_DEMUX_Notify pfnNotify, pfnDxB_DEMUX_AllocBuffer pfnAllocBuffer);
DxB_ERR_CODE TCC_DxB_DEMUX_StartSectionFilter(DxBInterface *hInterface, UINT16 usPid, UINT32 ulRequestID, BOOLEAN bIsOnce,
											UINT32 ulFilterLen,UINT8 * pucValue,UINT8 * pucMask, UINT8 * pucExclusion,BOOLEAN bCheckCRC, void *pUserData);
DxB_ERR_CODE TCC_DxB_DEMUX_StopSectionFilter(DxBInterface *hInterface, UINT32 ulRequestID);
DxB_ERR_CODE TCC_DxB_DEMUX_StartSectionFilterEx(DxBInterface *hInterface, UINT16 usPid, UINT32 ulRequestID, BOOLEAN bIsOnce,
											UINT32 ulFilterLen, UINT8 * pucValue, UINT8 * pucMask,
											UINT8 * pucExclusion, BOOLEAN bCheckCRC,
											pfnDxB_DEMUX_NotifyEx	pfnNotify, 
											pfnDxB_DEMUX_AllocBuffer pfnAllocBuffer,
											UINT32 *SectionHandle);
DxB_ERR_CODE TCC_DxB_DEMUX_StopSectionFilterEx(DxBInterface *hInterface, UINT32 SectionHandle);
DxB_ERR_CODE TCC_DxB_DEMUX_GetSTC(DxBInterface *hInterface, UINT32 * pulHighBitSTC, UINT32 * pulLowBitSTC, unsigned int index);
DxB_ERR_CODE TCC_DxB_DEMUX_GetSTCFunctionPTR(DxBInterface *hInterface, void **ppfnGetSTC, void **ppvApp);
DxB_ERR_CODE TCC_DxB_DEMUX_GetPVRFuncPTR(DxBInterface *hInterface, void **ppfnPCRCallback, void **ppfnTSCallback, void **ppvApp);
DxB_ERR_CODE TCC_DxB_DEMUX_ResetVideo(DxBInterface *hInterface, UINT32 ulDevId);
DxB_ERR_CODE TCC_DxB_DEMUX_ResetAudio(DxBInterface *hInterface, UINT32 ulDevId);
DxB_ERR_CODE TCC_DxB_DEMUX_RecordStart(DxBInterface *hInterface, UINT32 ulDevId, UINT8 * pucFileName);
DxB_ERR_CODE TCC_DxB_DEMUX_RecordStop(DxBInterface *hInterface, UINT32 ulDevId);
DxB_ERR_CODE TCC_DxB_DEMUX_SetParentLock(DxBInterface *hInterface, UINT32 ulDevId, UINT32 uiParentLock);
DxB_ERR_CODE TCC_DxB_DEMUX_SetESType(DxBInterface *hInterface, UINT32 ulDevId, DxB_ELEMENTARYSTREAM ulAudioType, DxB_ELEMENTARYSTREAM ulVideoType);
DxB_ERR_CODE TCC_DxB_DEMUX_StartDemuxing(DxBInterface *hInterface, UINT32 ulDevId);
DxB_ERR_CODE TCC_DxB_DEMUX_SetActiveMode(DxBInterface *hInterface, UINT32 ulDevId, UINT32 activemode);
DxB_ERR_CODE TCC_DxB_DEMUX_SetFirstDsiplaySet (DxBInterface *hInterface, UINT32 ulDevId, UINT32 displayflag);
DxB_ERR_CODE TCC_DxB_DEMUX_SendData(DxBInterface *hInterface, UINT8 *data, UINT32 datasize);
DxB_ERR_CODE TCC_DxB_DEMUX_SetAudioCodecInformation(DxBInterface *hInterface, UINT32 ulDevId, void *pAudioInfo);
DxB_ERR_CODE TCC_DxB_DEMUX_TDMB_SetContentsType(DxBInterface *hInterface, UINT32 ulDevId, UINT32 ContentsType);
DxB_ERR_CODE TCC_DxB_DEMUX_RegisterEventCallback(DxBInterface *hInterface, pfnDxB_DEMUX_EVENT_Notify pfnEventCallback, void *pUserData);
DxB_ERR_CODE TCC_DxB_DEMUX_SendEvent(DxBInterface *hInterface, DxB_DEMUX_NOTIFY_EVT nEvent, void *pCallbackData);
DxB_ERR_CODE TCC_DxB_DEMUX_RegisterCasDecryptCallback(DxBInterface *hInterface, UINT32 ulDevId, pfnDxb_DEMUX_CasDecrypt pfnCasCallback);
DxB_ERR_CODE TCC_DxB_DEMUX_EnableAudioDescription(DxBInterface *hInterface, BOOLEAN isEnableAudioDescription);
DxB_ERR_CODE TCC_DxB_DEMUX_SetSTCOffset(DxBInterface *hInterface, UINT32 ulDevId, INT32 iOffset, INT32 iOption);
DxB_ERR_CODE TCC_DxB_DEMUX_SetDualchannel(DxBInterface *hInterface, UINT32 ulDemuxId, UINT32 uiIndex);

#endif

