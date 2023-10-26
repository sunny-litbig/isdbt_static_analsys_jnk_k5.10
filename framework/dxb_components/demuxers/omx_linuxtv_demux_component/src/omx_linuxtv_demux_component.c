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
#define LOG_TAG	"OMX_LINUXTV_DEMUX"
#include <utils/Log.h>
#include <cutils/properties.h>
#include "globals.h"
#include <omxcore.h>
#include <omx_base_video_port.h>
#include <omx_base_audio_port.h>
#include <omx_linuxtv_demux_component.h>
#include "omx_tcc_thread.h"
#include "tcc_dxb_interface_demux.h"
#include "LinuxTV_Demux.h"
#include "tcc_linuxtv_system.h"
#include "tcc_demux_ptscorrection.h"

#include <sys/ioctl.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "tcc_dxb_timecheck.h"

/****************************************************************************
DEFINITION
****************************************************************************/
#define TSIF_PACKETSIZE (188)
#define AUDIO_PES_TIMER 80 // ms


/****************************************************************************
DEFINITION OF EXTERNAL FUNCTION
****************************************************************************/


/****************************************************************************
DEFINITION OF STRUCTURE
****************************************************************************/
typedef struct {
	unsigned int      uiTSIFDataReadSize;
	unsigned int      uiMSCDMABufSize;
	unsigned char*    pucMSCDMABuf;
	FILE*             hTSFile;
	int               iDevDVR;
	int               iPrevPCR;
} TCC_DMX_FILEINPUT;

typedef struct {
	int               iDeviceIndex;
	DemuxSystem       tSystem;
	HandleDemuxMsg    hMessage;
	HandleDemux       hDemux;
	HandleDemuxFilter hPCRSlotMain;
	HandleDemuxFilter hPCRSlotSub;
	HandleDemuxFilter hVideoSlotMain;
	HandleDemuxFilter hVideoSlotSub;
	HandleDemuxFilter hAudioSlotMain;
	HandleDemuxFilter hAudioSlotSub;
	unsigned int      uiPVRFlags;
	int               iPlayMode;
	long long         llLatestSTC;
	OMX_BOOL          isVisual_Impaired;

	TCC_DMX_FILEINPUT tFileInput;
} TCC_DMX_PRIVATE;


/****************************************************************************
DEFINITION OF FUNCTIONS
****************************************************************************/
static int FileInput_Open(TCC_DMX_FILEINPUT *pFileInput)
{
	char value[PROPERTY_VALUE_MAX];
	unsigned int uiLen = 0;

	pFileInput->hTSFile = NULL;
	pFileInput->iDevDVR = -1;
	pFileInput->iPrevPCR = 0;
	pFileInput->uiTSIFDataReadSize = TSIF_PACKETSIZE;
	pFileInput->pucMSCDMABuf = TCC_fo_malloc(__func__, __LINE__, pFileInput->uiTSIFDataReadSize);
	pFileInput->uiMSCDMABufSize = pFileInput->uiTSIFDataReadSize;
	if (pFileInput->pucMSCDMABuf == NULL) {
		ALOGE("CANNOT Alloc TSIF Read Buffer : Size(%d)!!!\n", pFileInput->uiTSIFDataReadSize);
		return OMX_ErrorInsufficientResources;
	}

	uiLen = property_get("tcc.dxb.file.input", value, "");
	if (uiLen) {
		PRINTF("%s:File Input[%s]", __func__, value);
		pFileInput->hTSFile = fopen(value, "r");  //TSFILE_NAME
		if (pFileInput->hTSFile == NULL) {
			PRINTF("TS File(%s) Open Fail !!!\n", value);
		} else {
			pFileInput->iDevDVR = open("/dev/dvb0.dvr0", O_WRONLY);
		}
	}

	return 0;
}

static int FileInput_Close(TCC_DMX_FILEINPUT *pFileInput)
{
	if (pFileInput->hTSFile) {
		fclose(pFileInput->hTSFile);
		pFileInput->hTSFile = NULL;
	}

	if (pFileInput->iDevDVR >= 0) {
		close(pFileInput->iDevDVR);
		pFileInput->iDevDVR = -1;
	}

	if (pFileInput->pucMSCDMABuf)
	{
		TCC_fo_free(__func__, __LINE__, pFileInput->pucMSCDMABuf);
	}

	return 0;
}

static int FileInput_ReadData(TCC_DMX_FILEINPUT *pFileInput, unsigned char *ucBuffer, unsigned int uiSize)
{
	if (pFileInput->hTSFile) {
		int data_read;
		data_read = fread(ucBuffer, 1, uiSize, pFileInput->hTSFile);
		if (data_read <= 0) {
			PRINTF("TS File Read End.... \n");
			fseek(pFileInput->hTSFile, 0, SEEK_SET);
			return 0;
		}
		return data_read;
	}
	return 0;
}

OMX_ERRORTYPE dxb_omx_dxb_sendevent_recording_stop(OMX_COMPONENTTYPE * openmaxStandComp, int iArg)
{
	omx_dxb_demux_component_PrivateType *omx_dxb_demux_component_Private = openmaxStandComp->pComponentPrivate;
	int ieventcode = DxB_EVENT_ERR_OK;

	switch (iArg) {
		case 0:
			ieventcode = DxB_EVENT_ERR_OK;
			break;
		case -1: // unknown error
			ieventcode = DxB_EVENT_ERR_ERR;
			break;
		case -2: // no free space
			ieventcode = DxB_EVENT_ERR_NOFREESPACE;
			break;
		case -3: // file open error
			ieventcode = DxB_EVENT_ERR_FILEOPENERROR;
			break;
		default:
			ieventcode = DxB_EVENT_ERR_ERR;
			break;
	}

	(*(omx_dxb_demux_component_Private->callbacks->EventHandler))
	(openmaxStandComp,
	 omx_dxb_demux_component_Private->callbackData,
	 OMX_EventRecordingStop, /* The command was completed */
	 ieventcode, /* The commands was a OMX_CommandStateSet */
	 0, /* The state has been changed in message->messageParam2 */
	 NULL);

	return OMX_ErrorNone;
}

long long dxb_omx_dxb_get_stc(int itype, long long lpts, unsigned int index, int log, void *pvApp)
{
	OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE*) pvApp;
	omx_dxb_demux_component_PrivateType* omx_dxb_demux_component_Private = openmaxStandComp->pComponentPrivate;
	TCC_DMX_PRIVATE *pPrivateData = (TCC_DMX_PRIVATE*) omx_dxb_demux_component_Private->pPrivateData;
	int iMode = (pPrivateData->tFileInput.hTSFile) ? 1 : pPrivateData->iPlayMode;

	return TCCDEMUX_GetSTC(&pPrivateData->tSystem, pPrivateData->hDemux, itype, lpts, index, iMode, log);
}

int dxb_omx_dxb_update_pcr(int itype, int index, long long lpcr, void *pvApp)
{
	OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE*) pvApp;
	omx_dxb_demux_component_PrivateType* omx_dxb_demux_component_Private = openmaxStandComp->pComponentPrivate;
	TCC_DMX_PRIVATE *pPrivateData = (TCC_DMX_PRIVATE*) omx_dxb_demux_component_Private->pPrivateData;

	return TCCDemux_UpdatePCR(&pPrivateData->tSystem, itype, index, lpcr);
}

int dxb_omx_dxb_buffer_callback(unsigned char *pucBuff, int iSize, void *pvApp)
{
	OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE*) pvApp;
	omx_dxb_demux_component_PrivateType* omx_dxb_demux_component_Private = openmaxStandComp->pComponentPrivate;
	TCC_DMX_PRIVATE *pPrivateData = (TCC_DMX_PRIVATE*) omx_dxb_demux_component_Private->pPrivateData;

	return LinuxTV_Demux_InputBuffer(pPrivateData->hDemux, pucBuff, iSize);
}

int dxb_omx_dxb_GetBuffer(HandleDemuxFilter slot, OMX_BUFFERHEADERTYPE **portbuf, void *pvApp)
{
	OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE*) pvApp;
	omx_dxb_demux_component_PrivateType* omx_dxb_demux_component_Private = openmaxStandComp->pComponentPrivate;
	TCC_DMX_PRIVATE *pPrivateData = (TCC_DMX_PRIVATE*) omx_dxb_demux_component_Private->pPrivateData;
	omx_base_PortType *pOutPort;
	OMX_BUFFERHEADERTYPE* pOutputBuffer;
	int i;

	if (slot->eStreamType == STREAM_VIDEO_0) {
		pOutPort = (omx_base_PortType*)omx_dxb_demux_component_Private->ports[DxB_VIDEO_PORT];
	} else if (slot->eStreamType == STREAM_VIDEO_1) {
		pOutPort = (omx_base_PortType*)omx_dxb_demux_component_Private->ports[DxB_VIDEO2_PORT];
	} else if (slot->eStreamType == STREAM_AUDIO_0) {
		pOutPort = (omx_base_PortType*)omx_dxb_demux_component_Private->ports[DxB_AUDIO_PORT];
	} else if (slot->eStreamType == STREAM_AUDIO_1) {
		pOutPort = (omx_base_PortType*)omx_dxb_demux_component_Private->ports[DxB_AUDIO2_PORT];
	}else {
		ALOGE("[%s] Invalid Stream Type(%d)\n", __func__, slot->eStreamType);
		return -1;
	}

	if(pOutPort == NULL){
		ALOGE("[%s] Error : Invalid Port accessed(Stream Type : %d)\n", __func__, slot->eStreamType);
		return -1;
	}

#define RETRY_TIMES		5

	for (i = 0; i < RETRY_TIMES; i++)
	{
		if (PORT_IS_BEING_FLUSHED(pOutPort)) {
			ALOGE("%s:Port Flushed !!!", __func__);
			return -1;
		}

		if (tsem_down_timewait(pOutPort->pBufferSem, 1) == 0)	{
			continue;
		}else{
			break;
		}
		if (pPrivateData->uiPVRFlags & PVR_FLAG_PAUSE)
		{
			i--;
		}
	}

	if (i == RETRY_TIMES) {
		ALOGE("%s:Time out occur index[%d]streamtype[%s]", __func__, slot->eStreamType==STREAM_VIDEO_0||slot->eStreamType==STREAM_AUDIO_0?0:1,slot->eStreamType==STREAM_VIDEO_0||slot->eStreamType==STREAM_VIDEO_1?"video":"audio");
		return -1;
	}

	*portbuf = dxb_dequeue(pOutPort->pBufferQueue);
	if(*portbuf == NULL){
		return -1;
	}

	return 0;
}

int dxb_omx_dxb_ScrambleStateCallback(HandleDemuxFilter slot, int fScrambled, void *pvApp)
{
	OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE*) pvApp;
	omx_dxb_demux_component_PrivateType* omx_dxb_demux_component_Private = openmaxStandComp->pComponentPrivate;
	OMX_S32 iArgs[2];

	iArgs[0] = slot->pTsParser->nPID;
	iArgs[1] = fScrambled;

	(*(omx_dxb_demux_component_Private->callbacks->EventHandler))(
		openmaxStandComp,
		omx_dxb_demux_component_Private->callbackData,
		OMX_EventVendorSpecificEvent,
		OMX_VENDOR_EVENTDATA_DEMUX,
		DxB_DEMUX_EVENT_SCRAMBLED_STATUS,
		iArgs);
	return 0;
}

int dxb_omx_dxb_PCRCallback(HandleDemuxFilter slot, unsigned char *buf, int size, unsigned long long lpcr, int done, void *pvApp)
{
	OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE*) pvApp;
	omx_dxb_demux_component_PrivateType* omx_dxb_demux_component_Private = openmaxStandComp->pComponentPrivate;
	TCC_DMX_PRIVATE *pPrivateData = (TCC_DMX_PRIVATE*) omx_dxb_demux_component_Private->pPrivateData;
	int iret = 0;
	int index;

	if (slot->eStreamType == STREAM_PCR_0)	index = 0;
	else if (slot->eStreamType == STREAM_PCR_1) index = 1;
	else return -1;

	if (pPrivateData->tFileInput.iPrevPCR == 0 || pPrivateData->tFileInput.iPrevPCR > (int)lpcr)
		iret = TCCDemux_UpdatePCR(&pPrivateData->tSystem, 0, index, lpcr);

	pPrivateData->tFileInput.iPrevPCR = lpcr;

	return iret;
}

static int dxb_omx_dxb_check_stc(TCC_DMX_PRIVATE *pPrivateData, int eStreamType, unsigned long long pts)
{
    int index = 0, diff;
    long long stc = 0;

    if(pPrivateData->tFileInput.hTSFile)
        return 0;

    if(pPrivateData->isVisual_Impaired == OMX_TRUE && eStreamType == STREAM_AUDIO_1)
    {
        stc = pPrivateData->llLatestSTC;
    }
    else
    {
        if(eStreamType == STREAM_VIDEO_1 || eStreamType == STREAM_AUDIO_1)
        {
            index = 1;
        }
        if(pPrivateData->tSystem.guiPCRPid[index] >= 0x1fff)
        {
            return 0; //skip checking pcr because the pcr is invalid
        }

        LinuxTV_Demux_GetSTC(pPrivateData->hDemux, index, &stc);
        pPrivateData->llLatestSTC = stc;
    }

    if(stc)
    {
        pts = pts/90;
        diff = pts + 3000 - stc; //3sec margin
        if(diff >= 0 && diff < 6000)
        {
            return 0;
        }
        if(diff < -(3600*10*1000) )
        {
            //in order to skip general pcr rolling back - margin 10hour - every 26.38hour the pcr is rolling back (33bit - 90khz)
//        	ALOGD("%s:PCR is rolling back : PTS[%u] STC[%u] @ index[%u]\n", __func__, (unsigned int)pts, (unsigned int)stc, index);
            return 0;
        }
    }

    if (eStreamType == STREAM_VIDEO_1)
        TCCDEMUXMsg_ReInit(pPrivateData->hMessage);

//	ALOGD("%s:Invalid PTS[%u] STC[%u] @ index[%u]\n", __func__, (unsigned int)pts, (unsigned int)stc, index);
//	printf("%s:Invalid PTS[%u] STC[%u] @ index[%u]\n", __func__, (unsigned int)pts, (unsigned int)stc, index);

    return 1; //PTS is invalid.
}

static void SetBufferFlags(OMX_BUFFERHEADERTYPE* pBuffer, int flags)
{
	if (flags == 2)      // trick
	{
		pBuffer->nFlags |= OMX_BUFFERFLAG_FILE_PLAY;
		pBuffer->nFlags |= OMX_BUFFERFLAG_TRICK_PLAY;
	}
	else if (flags == 1) // pvr
	{
		pBuffer->nFlags |= OMX_BUFFERFLAG_FILE_PLAY;
		pBuffer->nFlags &= ~OMX_BUFFERFLAG_TRICK_PLAY;
	}
	else                 // air
	{
		pBuffer->nFlags &= ~OMX_BUFFERFLAG_FILE_PLAY;
		pBuffer->nFlags &= ~OMX_BUFFERFLAG_TRICK_PLAY;
	}
}

int dxb_omx_dxb_VideoBufferCallback(HandleDemuxFilter slot, unsigned char *buf, int size, unsigned long long pts, int flags, void *pvApp)
{
	OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE*) pvApp;
	omx_dxb_demux_component_PrivateType* omx_dxb_demux_component_Private = openmaxStandComp->pComponentPrivate;
	TCC_DMX_PRIVATE *pPrivateData = (TCC_DMX_PRIVATE*) omx_dxb_demux_component_Private->pPrivateData;
	omx_base_PortType *pOutPort;
	OMX_BUFFERHEADERTYPE* pOutputBuffer;
	int iDecoderID, iRequestID, index=0;

	if(TCCDemux_GetBrazilParentLock(&pPrivateData->tSystem)){
		size = 0;
	}

	if (flags == 0) // flags(0: air, 1: pvr, 2: trick)
	{
		if( dxb_omx_dxb_check_stc(pPrivateData, slot->eStreamType, pts) != 0 )
		{
			size = 0; //frame is valid because the pts is behind
		}
	}

	index = (slot->eStreamType == STREAM_VIDEO_0) ? DxB_VIDEO_PORT : DxB_VIDEO2_PORT;
	iDecoderID = (slot->eStreamType == STREAM_VIDEO_0) ? 0 : 1;

	pOutPort = (omx_base_PortType*)omx_dxb_demux_component_Private->ports[index];

	if (slot->eStreamType == STREAM_VIDEO_0)
	{
		pts = pts / 90;
		pOutputBuffer = slot->pPortBuffer;
		if (buf && (pOutputBuffer == NULL || (buf != pOutputBuffer->pBuffer))) {
			if (dxb_omx_dxb_GetBuffer(slot, &pOutputBuffer, pvApp) != 0){
				return 0;
			}

			if(!pOutputBuffer){
				return 0;
			}
			memcpy(pOutputBuffer->pBuffer, buf, size);
		} else {
			slot->pPortBuffer = NULL;
		}

		pOutputBuffer->pBuffer[size] = iDecoderID;
		pOutputBuffer->nTimeStamp = pts * 1000;
		pOutputBuffer->nFilledLen = size;

		if (slot->nSendCount == 0)
		{
			ALOGD("%s:Video Main send first frame pts[%lld]\n", __func__, pts);
			tcc_dxb_timecheck_set("switch_channel", "FVideoCB", TIMECHECK_STOP);
			tcc_dxb_timecheck_set("switch_channel", "FVideo_out", TIMECHECK_START);
		}
	}
	else
	{
		int	err =0;
		int 	i, id = 1, count =0;
		pOutputBuffer = slot->pPortBuffer;

		if (buf &&  (pOutputBuffer == NULL || (buf != pOutputBuffer->pBuffer)))
		{
	//		ALOGE("%s::%d\n",__func__, __LINE__);
			if (dxb_omx_dxb_GetBuffer(slot, &pOutputBuffer, pvApp) != 0){
				return 0;
			}

			if(!pOutputBuffer){
				return 0;
			}
			err = TCCDEMUXMsg_PutEs(pPrivateData->hMessage, id, buf, size, pts);
		}
		else
			err = TCCDEMUXMsg_PutEs(pPrivateData->hMessage, id, pOutputBuffer->pBuffer, size, pts);

		if(err == PTSCORRECTION_STATUS_PUT_FAIL)
		{
			pOutputBuffer->nFilledLen = 0;

			slot->pPortBuffer = NULL;
		   	slot->nSendCount++;
			pOutPort->ReturnBufferFunction((omx_base_PortType*)pOutPort, pOutputBuffer);
			return 0;
		}

		err =TCCDEMUXMsg_CorrectionReadyCheck(pPrivateData->hMessage, id, pts);
		if(err != PTSCORRECTION_STATUS_CORRECTION_READY)
		{
			pOutputBuffer->nFilledLen = 0;

			slot->pPortBuffer = NULL;
		   	slot->nSendCount++;
			pOutPort->ReturnBufferFunction((omx_base_PortType*)pOutPort, pOutputBuffer);
			return 0;
		}

		count = TCCDEMUXMsg_GetCount(pPrivateData->hMessage, id);

		err = TCCDEMUXMsg_GetEs(pPrivateData->hMessage, id, pOutputBuffer, pts);
		if(err != PTSCORRECTION_STATUS_OK)
		{
			pOutputBuffer->nFilledLen = 0;
			slot->pPortBuffer = NULL;
			pOutPort->ReturnBufferFunction((omx_base_PortType*)pOutPort, pOutputBuffer);
			return 0;
		}

		if( slot->nSendCount == 0)  {
			ALOGD("%s:Video Sub send first frame pts[%lld]\n", __func__, pOutputBuffer->nTimeStamp/1000);
		}

		slot->pPortBuffer = NULL;
	}

	if (size > 0)
	{
		SetBufferFlags(pOutputBuffer, flags);
	}
	if (slot->nSendCount == 0)
	{
		pOutputBuffer->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;
	}

   	slot->nSendCount++;
	pOutPort->ReturnBufferFunction((omx_base_PortType*)pOutPort, pOutputBuffer);

	return 0;
}

int dxb_omx_dxb_AudioBufferCallback(HandleDemuxFilter slot, unsigned char *buf, int size, unsigned long long pts, int flags, void *pvApp)
{
	OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE*) pvApp;
	omx_dxb_demux_component_PrivateType* omx_dxb_demux_component_Private = openmaxStandComp->pComponentPrivate;
	TCC_DMX_PRIVATE *pPrivateData = (TCC_DMX_PRIVATE*) omx_dxb_demux_component_Private->pPrivateData;
	omx_base_PortType *pOutPort;
	OMX_BUFFERHEADERTYPE* pOutputBuffer;
	int index;
	unsigned long long pts_ms;

	if(TCCDemux_GetBrazilParentLock(&pPrivateData->tSystem)){
		size = 0;
	}

	if (flags == 0) // flags(0: air, 1: pvr, 2: trick)
	{
		if( pts )
		{
			if( dxb_omx_dxb_check_stc(pPrivateData, slot->eStreamType, pts) != 0 )
		    {
			     size = 0; //frame is valid because the pts is behind
		    }
		}
	}
	index = (slot->eStreamType == STREAM_AUDIO_0) ? DxB_AUDIO_PORT : DxB_AUDIO2_PORT;
	pOutPort = (omx_base_PortType*)omx_dxb_demux_component_Private->ports[index];

	pts_ms = pts / 90;
	pOutputBuffer = slot->pPortBuffer;

	if (buf && (pOutputBuffer == NULL || (buf != pOutputBuffer->pBuffer))) {
		if (dxb_omx_dxb_GetBuffer(slot, &pOutputBuffer, pvApp) != 0){
			return 0;
		}

		if(!pOutputBuffer){
			return 0;
		}
		memcpy(pOutputBuffer->pBuffer, buf, size);
	} else {
		slot->pPortBuffer = NULL;
	}

    //0:main, 1:sub
    index = (slot->eStreamType == STREAM_AUDIO_0) ? 0 : 1;
	pOutputBuffer->pBuffer[size] = index;
	pOutputBuffer->nTimeStamp = pts*(1000/10)/(90/10);//pts_ms * 1000;
	pOutputBuffer->nFilledLen = size;
	pOutputBuffer->nDFScompensation = TCCDEMUX_Compensation_Enable();

    if(slot->nSendCount == 0)
    {
        ALOGD("%s:send first frame pts[%lld]", __func__, pts_ms);
		tcc_dxb_timecheck_set("switch_channel", "AudioCB", TIMECHECK_STOP);
		tcc_dxb_timecheck_set("switch_channel", "Audio_out", TIMECHECK_START);
    }

	if (size > 0)
	{
		SetBufferFlags(pOutputBuffer, flags);
		if( slot->fDiscontinuity==TRUE )
		{
			pOutputBuffer->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;
			slot->fDiscontinuity=FALSE;
		}
	}

    //slot->nPrevPTS = pts_ms;
   	slot->nSendCount++;
	pOutPort->ReturnBufferFunction((omx_base_PortType*)pOutPort, pOutputBuffer);

	return 0;
}

void dxb_omx_dxb_demux_component_BufferMgmtCallback(OMX_COMPONENTTYPE * openmaxStandComp,
		OMX_BUFFERHEADERTYPE * outputbuffer)
{
	int   data_read;
	omx_dxb_demux_component_PrivateType *omx_dxb_demux_component_Private = openmaxStandComp->pComponentPrivate;
	TCC_DMX_PRIVATE *pPrivateData = (TCC_DMX_PRIVATE*) omx_dxb_demux_component_Private->pPrivateData;
	TCC_DMX_FILEINPUT *pFileInput = &pPrivateData->tFileInput;
	omx_base_PortType *pAudioPort, *pVideoPort;
	pVideoPort = (omx_base_PortType *) omx_dxb_demux_component_Private->ports[DxB_VIDEO_PORT];
	pAudioPort = (omx_base_PortType *) omx_dxb_demux_component_Private->ports[DxB_AUDIO_PORT];

	if ((dxb_getquenelem(pVideoPort->pBufferQueue) < 3 && dxb_getquenelem(pAudioPort->pBufferQueue) < 3) || pFileInput->hTSFile == NULL) {
		usleep(30000);
		return;
	}

	if (pPrivateData->hVideoSlotMain || pPrivateData->hVideoSlotSub)
		pFileInput->uiTSIFDataReadSize = TSIF_PACKETSIZE * 256;
	else
		pFileInput->uiTSIFDataReadSize = TSIF_PACKETSIZE;
	if (pFileInput->uiTSIFDataReadSize > pFileInput->uiMSCDMABufSize) {
		TCC_fo_free(__func__, __LINE__, pFileInput->pucMSCDMABuf);
		pFileInput->pucMSCDMABuf = TCC_fo_malloc(__func__,__LINE__,pFileInput->uiTSIFDataReadSize);
		pFileInput->uiMSCDMABufSize = pFileInput->uiTSIFDataReadSize;
	}

	if (pFileInput->pucMSCDMABuf == NULL)
		return;

	data_read = FileInput_ReadData(pFileInput, pFileInput->pucMSCDMABuf, pFileInput->uiTSIFDataReadSize);
	//ALOGD ("read size %d : %d", data_read, pFileInput->uiTSIFDataReadSize);
	if (data_read <= 0) {
		if (data_read == 0) {
			ALOGE("TSIF Read Size 0");
			usleep(5000);
		}
		return;
	}

	if (pFileInput->iDevDVR >= 0 && data_read > 0) {
		if (write(pFileInput->iDevDVR, pFileInput->pucMSCDMABuf, data_read) != data_read) {
			//ALOGE("write size mis-match %d", data_read);
		}
	}
}

OMX_ERRORTYPE dxb_omx_dxb_demux_component_Constructor(OMX_COMPONENTTYPE * openmaxStandComp, OMX_STRING cComponentName)
{
	OMX_ERRORTYPE err = OMX_ErrorNone;
	omx_dxb_demux_component_PrivateType *omx_dxb_demux_component_Private;
	omx_base_audio_PortType *outPortAudio;
	omx_base_video_PortType *outPortVideo;
	TCC_DMX_PRIVATE *pPrivateData;
	char value[PROPERTY_VALUE_MAX];
	unsigned int uiLen, uiData;

	if (!openmaxStandComp->pComponentPrivate) {
		openmaxStandComp->pComponentPrivate = TCC_fo_calloc (__func__, __LINE__,1, sizeof(omx_dxb_demux_component_PrivateType));
		if (openmaxStandComp->pComponentPrivate == NULL) {
			return OMX_ErrorInsufficientResources;
		}
	}

	omx_dxb_demux_component_Private = openmaxStandComp->pComponentPrivate;
	omx_dxb_demux_component_Private->ports = NULL;

	err = dxb_omx_base_source_Constructor(openmaxStandComp, cComponentName);

	omx_dxb_demux_component_Private->sPortTypesParam[OMX_PortDomainAudio].nStartPortNumber = 0; //DxB_AUDIO_PORT;
	omx_dxb_demux_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts = 2;

	omx_dxb_demux_component_Private->sPortTypesParam[OMX_PortDomainVideo].nStartPortNumber = 2;//DxB_VIDEO_PORT;
	omx_dxb_demux_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts = 2;

	if ((omx_dxb_demux_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
		 omx_dxb_demux_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts)
		&& !omx_dxb_demux_component_Private->ports) {
		omx_dxb_demux_component_Private->ports =
			TCC_fo_calloc (__func__, __LINE__,(omx_dxb_demux_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
						omx_dxb_demux_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts),
					   sizeof(omx_base_PortType *));
		if (!omx_dxb_demux_component_Private->ports) {
			return OMX_ErrorInsufficientResources;
		}

		/* allocate video port */
		omx_dxb_demux_component_Private->ports[DxB_VIDEO_PORT] = TCC_fo_calloc (__func__, __LINE__,1, sizeof(omx_base_video_PortType));
		if (!omx_dxb_demux_component_Private->ports[DxB_VIDEO_PORT])
			return OMX_ErrorInsufficientResources;
		omx_dxb_demux_component_Private->ports[DxB_VIDEO2_PORT] = TCC_fo_calloc (__func__, __LINE__,1, sizeof(omx_base_video_PortType));
		if (!omx_dxb_demux_component_Private->ports[DxB_VIDEO2_PORT])
			return OMX_ErrorInsufficientResources;


		/* allocate audio port */
		omx_dxb_demux_component_Private->ports[DxB_AUDIO_PORT] = TCC_fo_calloc (__func__, __LINE__,1, sizeof(omx_base_audio_PortType));
		if (!omx_dxb_demux_component_Private->ports[DxB_AUDIO_PORT])
			return OMX_ErrorInsufficientResources;
		omx_dxb_demux_component_Private->ports[DxB_AUDIO2_PORT] = TCC_fo_calloc (__func__, __LINE__,1, sizeof(omx_base_audio_PortType));
		if (!omx_dxb_demux_component_Private->ports[DxB_AUDIO2_PORT])
			return OMX_ErrorInsufficientResources;

	}

	dxb_base_audio_port_Constructor(openmaxStandComp, &omx_dxb_demux_component_Private->ports[DxB_AUDIO_PORT],
								DxB_AUDIO_PORT, OMX_FALSE);
	dxb_base_audio_port_Constructor(openmaxStandComp, &omx_dxb_demux_component_Private->ports[DxB_AUDIO2_PORT],
								DxB_AUDIO2_PORT, OMX_FALSE);
	dxb_base_video_port_Constructor(openmaxStandComp, &omx_dxb_demux_component_Private->ports[DxB_VIDEO_PORT],
								DxB_VIDEO_PORT, OMX_FALSE);
	dxb_base_video_port_Constructor(openmaxStandComp, &omx_dxb_demux_component_Private->ports[DxB_VIDEO2_PORT],
								DxB_VIDEO2_PORT, OMX_FALSE);

	/*Input pPort buffer size is equal to the size of the output buffer of the previous component */
	outPortVideo = (omx_base_video_PortType *) omx_dxb_demux_component_Private->ports[DxB_VIDEO_PORT];
	outPortVideo->sPortParam.nBufferSize = VIDEO_DXB_IN_BUFFER_SIZE;
	outPortVideo->sPortParam.nBufferCountMin = 8;
	outPortVideo->sPortParam.nBufferCountActual = 8;

	outPortVideo = (omx_base_video_PortType *) omx_dxb_demux_component_Private->ports[DxB_VIDEO2_PORT];
	outPortVideo->sPortParam.nBufferSize = VIDEO_DXB_IN_BUFFER_SIZE;
	outPortVideo->sPortParam.nBufferCountMin = 8;
	outPortVideo->sPortParam.nBufferCountActual = 8;

	outPortAudio = (omx_base_audio_PortType *) omx_dxb_demux_component_Private->ports[DxB_AUDIO_PORT];
	outPortAudio->sPortParam.nBufferSize = AUDIO_DXB_IN_BUFFER_SIZE;
	outPortAudio->sPortParam.nBufferCountMin = 4;
	outPortAudio->sPortParam.nBufferCountActual = 4;

	outPortAudio = (omx_base_audio_PortType *) omx_dxb_demux_component_Private->ports[DxB_AUDIO2_PORT];
	outPortAudio->sPortParam.nBufferSize = AUDIO_DXB_IN_BUFFER_SIZE;
	outPortAudio->sPortParam.nBufferCountMin = 4;
	outPortAudio->sPortParam.nBufferCountActual = 4;

	omx_dxb_demux_component_Private->BufferMgmtCallback = dxb_omx_dxb_demux_component_BufferMgmtCallback;
	omx_dxb_demux_component_Private->messageHandler = dxb_omx_dxb_demux_component_MessageHandler;

	omx_dxb_demux_component_Private->destructor = dxb_omx_dxb_demux_component_Destructor;
	omx_dxb_demux_component_Private->BufferMgmtFunction = dxb_omx_dxb_demux_twoport_BufferMgmtFunction;

	openmaxStandComp->SetParameter = dxb_omx_dxb_demux_component_SetParameter;
	openmaxStandComp->GetParameter = dxb_omx_dxb_demux_component_GetParameter;
	openmaxStandComp->SetConfig = dxb_omx_dxb_demux_component_SetConfig;
	openmaxStandComp->GetConfig = dxb_omx_dxb_demux_component_GetConfig;
	openmaxStandComp->GetExtensionIndex = dxb_omx_dxb_demux_component_GetExtensionIndex;

	omx_dxb_demux_component_Private->pPrivateData = TCC_fo_calloc(__func__, __LINE__, 1, sizeof(TCC_DMX_PRIVATE));
	if(omx_dxb_demux_component_Private->pPrivateData == NULL){
		return OMX_ErrorInsufficientResources;
	}

	pPrivateData = (TCC_DMX_PRIVATE*) omx_dxb_demux_component_Private->pPrivateData;

	TCCDEMUX_Init(&pPrivateData->tSystem);
	TCCDEMUX_SetParentLock(&pPrivateData->tSystem, 0);

	uiLen = property_get("tcc.dxb.isdbt.sinfo", value, "");
	if (uiLen) {
		uiData = strtoul(value, 0, 16);
		ALOGD("[DEMUX]property_get[tcc.dxb.isdbt.sinfo]::0x%08X", uiData);
		TCCDEMUX_SetISDBTFeature(&pPrivateData->tSystem, uiData);
	} else {
		ALOGE("ISDB-T specific info error from framework\n");
	}

	pPrivateData->iDeviceIndex = 0;
	pPrivateData->uiPVRFlags = 0;
	pPrivateData->iPlayMode = 0;
	pPrivateData->llLatestSTC = 0;
	pPrivateData->isVisual_Impaired = OMX_FALSE;

	return err;
}

OMX_ERRORTYPE dxb_omx_dxb_demux_component_Destructor(OMX_COMPONENTTYPE * openmaxStandComp)
{
	omx_dxb_demux_component_PrivateType *omx_dxb_demux_component_Private = openmaxStandComp->pComponentPrivate;
	TCC_DMX_PRIVATE *pPrivateData = (TCC_DMX_PRIVATE*) omx_dxb_demux_component_Private->pPrivateData;
	OMX_U32   i;

	if (omx_dxb_demux_component_Private->ports) {
		for (i = 0; i < omx_dxb_demux_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts
			 + omx_dxb_demux_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts; i++) {
			if (omx_dxb_demux_component_Private->ports[i])
				omx_dxb_demux_component_Private->ports[i]->PortDestructor(omx_dxb_demux_component_Private->ports[i]);
		}
		TCC_fo_free(__func__,__LINE__,omx_dxb_demux_component_Private->ports);
		omx_dxb_demux_component_Private->ports = NULL;
	}

	TCCDEMUX_DeInit(&pPrivateData->tSystem);

	TCC_fo_free(__func__, __LINE__, omx_dxb_demux_component_Private->pPrivateData);

	return dxb_omx_base_source_Destructor(openmaxStandComp);
}

OMX_ERRORTYPE dxb_omx_dxb_linuxtv_demux_component_Init(OMX_COMPONENTTYPE * openmaxStandComp)
{
	return dxb_omx_dxb_demux_component_Constructor(openmaxStandComp, BROADCAST_DXB_DEMUX_NAME);
}

OMX_ERRORTYPE dxb_omx_dxb_demux_component_Deinit(OMX_COMPONENTTYPE * openmaxStandComp)
{
	return dxb_omx_dxb_demux_component_Destructor(openmaxStandComp);
}

OMX_ERRORTYPE dxb_omx_dxb_demux_component_MessageHandler(OMX_COMPONENTTYPE * openmaxStandComp,
		internalRequestMessageType * message)
{
	omx_dxb_demux_component_PrivateType *omx_dxb_demux_component_Private =
		(omx_dxb_demux_component_PrivateType *) openmaxStandComp->pComponentPrivate;
	OMX_STATETYPE eCurrentState = omx_dxb_demux_component_Private->state;
	OMX_ERRORTYPE err;
	TCC_DMX_PRIVATE *pPrivateData = (TCC_DMX_PRIVATE*) omx_dxb_demux_component_Private->pPrivateData;

	DEBUG (DEB_LEV_ERR, "[DxBDEMUX]in %s : type(%d) param(%d) currentstate(%d)\n", __func__, message->messageType,
		   message->messageParam, eCurrentState);

	if (message->messageType == OMX_CommandStateSet) {
		if ((eCurrentState == OMX_StateLoaded) && (message->messageParam == OMX_StateIdle)) {
			FileInput_Open(&pPrivateData->tFileInput);
		} else if ((message->messageParam == OMX_StateLoaded) && (eCurrentState == OMX_StateIdle)) {
			FileInput_Close(&pPrivateData->tFileInput);
		} else if ((eCurrentState == OMX_StateIdle) && (message->messageParam == OMX_StateLoaded)) {
			ALOGD("[%s] State changing to load state.\n", __func__);
		} else if ((eCurrentState == OMX_StateExecuting)&&(message->messageParam == OMX_StateIdle)) {
			DEBUG(DEB_LEV_ERR, "[DxBDEMUX]Current State is IDLE(%s)\n", __func__);
			pPrivateData->iPlayMode = 0;
			LinuxTV_Demux_DestroyFilter(pPrivateData->hVideoSlotMain);
			LinuxTV_Demux_DestroyFilter(pPrivateData->hAudioSlotMain);
			LinuxTV_Demux_DestroyFilter(pPrivateData->hPCRSlotMain);
			LinuxTV_Demux_DestroyFilter(pPrivateData->hVideoSlotSub);
			LinuxTV_Demux_DestroyFilter(pPrivateData->hAudioSlotSub);
			LinuxTV_Demux_DestroyFilter(pPrivateData->hPCRSlotSub);
			LinuxTV_Demux_DeInit(pPrivateData->hDemux);
			TCCDEMUXMsg_DeInit(pPrivateData->hMessage);
		} else if ((eCurrentState == OMX_StateIdle)&&(message->messageParam == OMX_StateExecuting)) {
			unsigned int iFlags = (FLAGS_PES | FLAGS_SECTION);

			if (PORT_IS_ENABLED(omx_dxb_demux_component_Private->ports[DxB_AUDIO_PORT]))
				iFlags |= (FLAGS_AUDIO_0);
			if (PORT_IS_ENABLED(omx_dxb_demux_component_Private->ports[DxB_VIDEO_PORT]))
				iFlags |= (FLAGS_VIDEO_0 | FLAGS_PCR_0);
			if (PORT_IS_ENABLED(omx_dxb_demux_component_Private->ports[DxB_AUDIO2_PORT]))
				iFlags |= (FLAGS_AUDIO_1);
			if (PORT_IS_ENABLED(omx_dxb_demux_component_Private->ports[DxB_VIDEO2_PORT]))
				iFlags |= (FLAGS_VIDEO_1 | FLAGS_PCR_1);
			if (pPrivateData->tFileInput.hTSFile)
				iFlags |= FLAGS_DVR_INPUT;

			TCCDEMUX_ReInit(&pPrivateData->tSystem);
			pPrivateData->hMessage = TCCDEMUXMsg_Init();
			pPrivateData->hDemux = LinuxTV_Demux_Init(pPrivateData->iDeviceIndex, iFlags);
			pPrivateData->hAudioSlotMain = LinuxTV_Demux_CreateFilter(pPrivateData->hDemux, STREAM_AUDIO_0, TCCSWDEMUXER_AUDIO_MAIN, openmaxStandComp, dxb_omx_dxb_AudioBufferCallback);
			pPrivateData->hVideoSlotMain = LinuxTV_Demux_CreateFilter(pPrivateData->hDemux, STREAM_VIDEO_0, TCCSWDEMUXER_VIDEO_MAIN, openmaxStandComp, dxb_omx_dxb_VideoBufferCallback);
			pPrivateData->hPCRSlotMain = LinuxTV_Demux_CreateFilter(pPrivateData->hDemux, STREAM_PCR_0, TCCSWDEMUXER_PCR_MAIN, openmaxStandComp, dxb_omx_dxb_PCRCallback);
			pPrivateData->hAudioSlotSub = LinuxTV_Demux_CreateFilter(pPrivateData->hDemux, STREAM_AUDIO_1, TCCSWDEMUXER_AUDIO_SUB, openmaxStandComp, dxb_omx_dxb_AudioBufferCallback);
			pPrivateData->hVideoSlotSub = LinuxTV_Demux_CreateFilter(pPrivateData->hDemux, STREAM_VIDEO_1, TCCSWDEMUXER_VIDEO_SUB, openmaxStandComp, dxb_omx_dxb_VideoBufferCallback);
			pPrivateData->hPCRSlotSub = LinuxTV_Demux_CreateFilter(pPrivateData->hDemux, STREAM_PCR_1, TCCSWDEMUXER_PCR_SUB, openmaxStandComp, dxb_omx_dxb_PCRCallback);
		}
	}

	/** Execute the base message handling */
	err = dxb_omx_base_component_MessageHandler(openmaxStandComp, message);

	if (message->messageType == OMX_CommandStateSet) {
		if ((message->messageParam == OMX_StateIdle) && (eCurrentState == OMX_StateLoaded)) {
		} else if ((message->messageParam == OMX_StateExecuting) && (eCurrentState == OMX_StateIdle)) {
		}
	}

	return err;
}

OMX_ERRORTYPE dxb_omx_dxb_demux_component_GetExtensionIndex(OMX_IN OMX_HANDLETYPE hComponent,
		OMX_IN OMX_STRING cParameterName,
		OMX_OUT OMX_INDEXTYPE * pIndexType)
{
	DEBUG(DEB_LEV_FUNCTION_NAME, "In  %s \n", __func__);
	return OMX_ErrorNone;
}

OMX_ERRORTYPE dxb_omx_dxb_demux_component_SetConfig
(OMX_IN OMX_HANDLETYPE hComponent, OMX_IN OMX_INDEXTYPE nIndex, OMX_IN OMX_PTR ComponentParameterStructure)
{
	OMX_ERRORTYPE err = OMX_ErrorNone;
	OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *) hComponent;
	omx_dxb_demux_component_PrivateType *omx_dxb_demux_component_Private = openmaxStandComp->pComponentPrivate;

	if (ComponentParameterStructure == NULL) {
		return OMX_ErrorBadParameter;
	}

	switch (nIndex) {
		default:
			return dxb_omx_base_component_SetConfig(hComponent, nIndex, ComponentParameterStructure);
	}
	return err;
}

OMX_ERRORTYPE dxb_omx_dxb_demux_component_GetConfig
(OMX_IN OMX_HANDLETYPE hComponent, OMX_IN OMX_INDEXTYPE nIndex, OMX_IN OMX_PTR ComponentParameterStructure)
{
	OMX_ERRORTYPE err = OMX_ErrorNone;
	OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *) hComponent;
	omx_dxb_demux_component_PrivateType *omx_dxb_demux_component_Private = openmaxStandComp->pComponentPrivate;

	if (ComponentParameterStructure == NULL) {
		return OMX_ErrorBadParameter;
	}

	switch (nIndex) {
		default:
			return dxb_omx_base_component_GetConfig(hComponent, nIndex, ComponentParameterStructure);
	}
	return err;
}

OMX_ERRORTYPE dxb_omx_dxb_demux_component_GetParameter(OMX_IN OMX_HANDLETYPE hComponent, OMX_IN OMX_INDEXTYPE nParamIndex,
		OMX_INOUT OMX_PTR ComponentParameterStructure)
{
	INT32    *piArg;
	UINT32    ulDemuxID;
	OMX_ERRORTYPE err = OMX_ErrorNone;
	OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *) hComponent;
	omx_dxb_demux_component_PrivateType *omx_dxb_demux_component_Private = openmaxStandComp->pComponentPrivate;
	TCC_DMX_PRIVATE *pPrivateData = (TCC_DMX_PRIVATE*) omx_dxb_demux_component_Private->pPrivateData;

	if (ComponentParameterStructure == NULL) {
		return OMX_ErrorBadParameter;
	}

	switch ((UINT32)nParamIndex)
	{
		case OMX_IndexVendorParamDxBGetNumOfDemux:
		{
			UINT32   *ulNumOfDemux = (UINT32 *) ComponentParameterStructure;
			*ulNumOfDemux = 1;
		}
		break;

		case OMX_IndexVendorParamDxBGetPath:
			break;

		case OMX_IndexVendorParamDxBGetCapability:
			break;

		case OMX_IndexVendorParamDxBGetSTC:
		{
			INT64    iSTC;
			UINT32   *pulHighBitSTC, *pulLowBitSTC, ulDecID;
			piArg = (INT32 *) ComponentParameterStructure;
			ulDemuxID = (UINT32) piArg[0];
			pulHighBitSTC = (UINT32 *) piArg[1];
			pulLowBitSTC = (UINT32 *) piArg[2];
			ulDecID = (UINT32)piArg[3];
			if (ulDecID > 2) ulDecID = 0;
			//iSTC = TSDEMUXApi_GetSTC(); //Unit ms
			iSTC = dxb_omx_dxb_get_stc(-1, 0, ulDecID, 1, openmaxStandComp);
			iSTC = iSTC * 90; //make stc unit to 90khz (air default)
			*pulHighBitSTC = (iSTC >> 32);
			*pulLowBitSTC = (iSTC & 0xffffffff);
		}
		break;

		case OMX_IndexVendorParamDxBGetSTCFunction:
		{
			void **ppvArg = (void**) ComponentParameterStructure;

			ppvArg[0] = (void*) dxb_omx_dxb_get_stc;
			ppvArg[1] = (void*) openmaxStandComp;
		}
		break;

		case OMX_IndexVendorParamDxBGetPVRFunction:
		{
			void **ppvArg;

			ppvArg = (void **) ComponentParameterStructure;
			ppvArg[0] = (void*) dxb_omx_dxb_update_pcr;
			ppvArg[1] = (void*) dxb_omx_dxb_buffer_callback;
			ppvArg[2] = (void*) openmaxStandComp;
		}
		break;

		case OMX_IndexVendorParamDxBVideoParser:
		{
			piArg = (INT32 *) ComponentParameterStructure;
			ulDemuxID = (UINT32) piArg[0];
			TS_PARSER **ppTsParser = (TS_PARSER **)piArg[1];
			ST_STREAM_PARSER **ppStreamParser = (ST_STREAM_PARSER **)piArg[2];
			TS_PARSER **ppPcrParser = (TS_PARSER **)piArg[3];

			if(ulDemuxID == 0) {
				if (pPrivateData->hAudioSlotMain == NULL || !LinuxTV_Demux_IsEnabledFilter(pPrivateData->hAudioSlotMain)) {
					break;
				}
				if (LinuxTV_Demux_IsEnabledFilter(pPrivateData->hVideoSlotMain)) {
					*ppTsParser = pPrivateData->hVideoSlotMain->pTsParser;
					*ppStreamParser = pPrivateData->hVideoSlotMain->pStreamParser;
				}
				if (pPrivateData->hPCRSlotMain) {
					*ppPcrParser = pPrivateData->hPCRSlotMain->pTsParser;
				}
			}
			else {
				if (pPrivateData->hAudioSlotSub == NULL || !LinuxTV_Demux_IsEnabledFilter(pPrivateData->hAudioSlotSub)) {
					break;
				}
				if(LinuxTV_Demux_IsEnabledFilter(pPrivateData->hVideoSlotSub)) {
					*ppTsParser = pPrivateData->hVideoSlotSub->pTsParser;
					*ppStreamParser = pPrivateData->hVideoSlotSub->pStreamParser;
				}
				if(pPrivateData->hPCRSlotSub) {
					*ppPcrParser = pPrivateData->hPCRSlotSub->pTsParser;
				}
			}
		}
		break;
		default:
			return dxb_omx_base_component_GetParameter(hComponent, nParamIndex, ComponentParameterStructure);
	}

	return err;
}

OMX_ERRORTYPE dxb_omx_dxb_demux_component_SetParameter(OMX_IN OMX_HANDLETYPE hComponent, OMX_IN OMX_INDEXTYPE nParamIndex,
		OMX_IN OMX_PTR ComponentParameterStructure)
{
	UINT32    ulDemuxID;
	OMX_S32   *piArg;
	OMX_ERRORTYPE err = OMX_ErrorNone;
	OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *) hComponent;
	omx_dxb_demux_component_PrivateType *omx_dxb_demux_component_Private = openmaxStandComp->pComponentPrivate;
	TCC_DMX_PRIVATE *pPrivateData = (TCC_DMX_PRIVATE*) omx_dxb_demux_component_Private->pPrivateData;

#if defined(RING_BUFFER_MODULE_ENABLE)
	char value[PROPERTY_VALUE_MAX];
	property_get("tcc.dxb.ringbuffer.enable", value, "1");
	OMX_BOOL isRingBuffer = (atoi(value)!=0);
#else
	OMX_BOOL isRingBuffer = OMX_FALSE;
#endif
	switch ((UINT32)nParamIndex)
	{
		case OMX_IndexVendorParamTunerDeviceSet:
			piArg = (OMX_S32*)ComponentParameterStructure;
			pPrivateData->iDeviceIndex = *piArg;
			break;

		case OMX_IndexVendorParamSetDemuxAudioVideoStreamType:
			break;

		case OMX_IndexVendorParamDxBSetPath:
			break;

		case OMX_IndexVendorParamDxBSetBitrate:
		case OMX_IndexVendorParamSetGetPacketDataEntry:
			break;

		case OMX_IndexVendorParamDxBReleasePath:
			ulDemuxID = (UINT32) ComponentParameterStructure;
			break;

		case OMX_IndexVendorParamDxBStartDemuxing:
			break;

		case OMX_IndexVendorParamDxBStartPID:
		{
			DxB_Pid_Info *pPidInfo;
			piArg = (OMX_S32 *) ComponentParameterStructure;
			ulDemuxID = (UINT32) piArg[0];
			pPidInfo = (DxB_Pid_Info *) piArg[1];

			if (pPidInfo->pidBitmask & PID_BITMASK_PCR) {
				if (pPrivateData->hPCRSlotMain) {
					pPrivateData->tSystem.guiPCRPid[0] = pPidInfo->usPCRPid;
					LinuxTV_Demux_SetPESFilter(pPrivateData->hPCRSlotMain, pPidInfo->usPCRPid, DMX_PES_PCR0);
					LinuxTV_Demux_EnableFilter(pPrivateData->hPCRSlotMain);
				}
			}

			if (pPidInfo->pidBitmask & PID_BITMASK_PCR_SUB) {
				if (pPrivateData->hPCRSlotSub) {
					pPrivateData->tSystem.guiPCRPid[1] = pPidInfo->usPCRSubPid;
					LinuxTV_Demux_SetPESFilter(pPrivateData->hPCRSlotSub, pPidInfo->usPCRSubPid, DMX_PES_PCR1);
					LinuxTV_Demux_EnableFilter(pPrivateData->hPCRSlotSub);
				}
			}

			if (pPidInfo->pidBitmask & PID_BITMASK_VIDEO_MAIN) {
				if (pPrivateData->hVideoSlotMain) {
					LinuxTV_Demux_SetPESFilter(pPrivateData->hVideoSlotMain, (unsigned int)pPidInfo->usVideoMainPid, DMX_PES_VIDEO0);
					if (pPidInfo->usVideoMainType != -1) {
						if(isRingBuffer) {
							ALOGI("==> RING_BUFFER MODE Main Stream Parser Disable");
							LinuxTV_Demux_SetStreamType(pPrivateData->hVideoSlotMain, pPidInfo->usVideoMainType, FALSE);
						} else {
							ALOGI("==> DEMUX MODE Main Stream Parser Enable");
							LinuxTV_Demux_SetStreamType(pPrivateData->hVideoSlotMain, pPidInfo->usVideoMainType, TRUE);
						}
					}
					TCCDEMUX_SetSTCOffset(&pPrivateData->tSystem, 0, 0, 0, 0);
					LinuxTV_Demux_EnableFilter(pPrivateData->hVideoSlotMain);
				}
			}

			if (pPidInfo->pidBitmask & PID_BITMASK_VIDEO_SUB) {
				if (pPrivateData->hVideoSlotSub) {
					LinuxTV_Demux_SetPESFilter(pPrivateData->hVideoSlotSub, (unsigned int)pPidInfo->usVideoSubPid, DMX_PES_VIDEO1);
					if (pPidInfo->usVideoSubType != -1) {
						if(isRingBuffer) {
							ALOGI("==> RING_BUFFER MODE Sub Stream Parser Enable");
							LinuxTV_Demux_SetStreamType(pPrivateData->hVideoSlotSub, pPidInfo->usVideoSubType, TRUE);
						} else {
							ALOGI("==> DEMUX MODE Sub Stream Parser Enable");
							LinuxTV_Demux_SetStreamType(pPrivateData->hVideoSlotSub, pPidInfo->usVideoSubType, TRUE);
						}
					}
					TCCDEMUX_SetSTCOffset(&pPrivateData->tSystem, 0, 1, 0, 0);
					LinuxTV_Demux_EnableFilter(pPrivateData->hVideoSlotSub);
					TCCDEMUXMsg_ReInit(pPrivateData->hMessage);
				}
			}

			if (pPidInfo->pidBitmask & PID_BITMASK_AUDIO_MAIN) {
				if (pPrivateData->hAudioSlotMain) {
					LinuxTV_Demux_SetPESFilter(pPrivateData->hAudioSlotMain, (unsigned int)pPidInfo->usAudioMainPid, DMX_PES_AUDIO0);
					if (pPidInfo->usAudioMainType != -1) {
						LinuxTV_Demux_SetStreamType(pPrivateData->hAudioSlotMain, (INT32)pPidInfo->usAudioMainType, FALSE);
					}
					LinuxTV_Demux_SetCallBackTimer(pPrivateData->hAudioSlotMain, TRUE, (AUDIO_PES_TIMER*1000) );
					TCCDEMUX_SetSTCOffset(&pPrivateData->tSystem, 1, 0, 0, 0);
					LinuxTV_Demux_EnableFilter(pPrivateData->hAudioSlotMain);
				}
			}

			if (pPidInfo->pidBitmask & PID_BITMASK_AUDIO_SUB) {
				if (pPrivateData->hAudioSlotSub) {
					LinuxTV_Demux_SetPESFilter(pPrivateData->hAudioSlotSub, (unsigned int)pPidInfo->usAudioSubPid, DMX_PES_AUDIO1);
					if (pPidInfo->usAudioSubType != -1) {
						LinuxTV_Demux_SetStreamType(pPrivateData->hAudioSlotSub, pPidInfo->usAudioSubType, FALSE);
					}
					LinuxTV_Demux_SetCallBackTimer(pPrivateData->hAudioSlotSub, TRUE, (AUDIO_PES_TIMER*1000) );
					TCCDEMUX_SetSTCOffset(&pPrivateData->tSystem, 1, 1, 0, 0);
					LinuxTV_Demux_EnableFilter(pPrivateData->hAudioSlotSub);
				}
			}
			pPrivateData->tFileInput.iPrevPCR = 0;
		}
		break;

		case OMX_IndexVendorParamDxBStopPID:
		{
			UINT32    pidBitmask;
			piArg = (OMX_S32 *) ComponentParameterStructure;
			ulDemuxID = (UINT32) piArg[0];
			pidBitmask = (UINT32) piArg[1];

			if (pidBitmask & PID_BITMASK_PCR) {
				if (pPrivateData->hPCRSlotMain) {
					LinuxTV_Demux_DisableFilter(pPrivateData->hPCRSlotMain);
				}
				pPrivateData->tSystem.guiPCRPid[0] = 0xffff;
			}

			if (pidBitmask & PID_BITMASK_PCR_SUB) {
				if (pPrivateData->hPCRSlotSub) {
					LinuxTV_Demux_DisableFilter(pPrivateData->hPCRSlotSub);
				}
				pPrivateData->tSystem.guiPCRPid[1] = 0xffff;
			}

			if (pidBitmask & PID_BITMASK_VIDEO_MAIN) {
				if (pPrivateData->hVideoSlotMain) {
					LinuxTV_Demux_DisableFilter(pPrivateData->hVideoSlotMain);
				}
			}

			if (pidBitmask & PID_BITMASK_VIDEO_SUB) {
				if (pPrivateData->hVideoSlotSub) {
					LinuxTV_Demux_DisableFilter(pPrivateData->hVideoSlotSub);
				}
			}

			if (pidBitmask & PID_BITMASK_AUDIO_MAIN) {
				if (pPrivateData->hAudioSlotMain) {
					LinuxTV_Demux_DisableFilter(pPrivateData->hAudioSlotMain);
				}
			}

			if (pidBitmask & PID_BITMASK_AUDIO_SUB) {
				if (pPrivateData->hAudioSlotSub) {
					LinuxTV_Demux_DisableFilter(pPrivateData->hAudioSlotSub);
				}
			}
		}
		break;

		case OMX_IndexVendorParamDxBRegisterPESCallback:
		{
			UINT32    ulDemuxId;
			DxB_DEMUX_PESTYPE etPesType;
			pfnDxB_DEMUX_PES_Notify pfnNotify;
			pfnDxB_DEMUX_AllocBuffer pfnAllocBuffer;
			pfnDxB_DEMUX_FreeBufferForError pfnErrorFreeBuffer;
			piArg = (OMX_S32 *) ComponentParameterStructure;
			etPesType = (DxB_DEMUX_PESTYPE) piArg[1];
			pfnNotify = (pfnDxB_DEMUX_PES_Notify) piArg[2];
			pfnAllocBuffer = (pfnDxB_DEMUX_AllocBuffer) piArg[3];
			pfnErrorFreeBuffer = (pfnDxB_DEMUX_FreeBufferForError) piArg[4];

			switch (etPesType)
			{
				case DxB_DEMUX_PESTYPE_SUBTITLE_0:
				case DxB_DEMUX_PESTYPE_SUBTITLE_1:
					pPrivateData->tSystem.gpfnSubtitleNotify = pfnNotify;
					pPrivateData->tSystem.gpfnSubtitleAllocBuffer = pfnAllocBuffer;
					pPrivateData->tSystem.gpfnSubtitleFreeBuffer = pfnErrorFreeBuffer;
					break;
				case DxB_DEMUX_PESTYPE_TELETEXT_0:
				case DxB_DEMUX_PESTYPE_TELETEXT_1:
					pPrivateData->tSystem.gpfnTeletextNotify = pfnNotify;
					pPrivateData->tSystem.gpfnTeletextAllocBuffer = pfnAllocBuffer;
					pPrivateData->tSystem.gpfnTeletextFreeBuffer = pfnErrorFreeBuffer;
					break;
				case DxB_DEMUX_PESTYPE_USERDEFINE:
					pPrivateData->tSystem.gpfnUserDefinedNotify = pfnNotify;
					pPrivateData->tSystem.gpfnUserDefinedAllocBuffer = pfnAllocBuffer;
					pPrivateData->tSystem.gpfnUserDefinedFreeBuffer = pfnErrorFreeBuffer;
					break;

				default:
					err = OMX_ErrorBadParameter;
					break;
			}
		}
		break;

		case OMX_IndexVendorParamDxBStartPESFilter:
		{
			UINT32    ulDemuxId, ulRequestID;
			UINT16    ulPid;
			DxB_DEMUX_PESTYPE etPESType;
			HandleDemuxFilter pPESFilter;
			void *pUserData;

			pPESFilter = NULL;
			piArg = (OMX_S32 *) ComponentParameterStructure;
			pUserData = (void*) piArg[0];
			ulPid = (UINT16) piArg[1];
			etPESType = (DxB_DEMUX_PESTYPE) piArg[2];
			ulRequestID = (UINT32) piArg[3];
			ulRequestID += TCCSWDEMUXER_PESFILTER_BEGIN;

			switch (etPESType) {
				case DxB_DEMUX_PESTYPE_SUBTITLE_0:
					pPESFilter =
						LinuxTV_Demux_CreateFilter(pPrivateData->hDemux, STREAM_PES, ulRequestID, &pPrivateData->tSystem, TCCDEMUX_CBSubtitlePES);
					LinuxTV_Demux_SetPESFilter(pPESFilter, ulPid, DMX_PES_SUBTITLE0);
					break;
				case DxB_DEMUX_PESTYPE_SUBTITLE_1:
					pPESFilter =
						LinuxTV_Demux_CreateFilter(pPrivateData->hDemux, STREAM_PES, ulRequestID, &pPrivateData->tSystem, TCCDEMUX_CBSubtitlePES);
					LinuxTV_Demux_SetPESFilter(pPESFilter, ulPid, DMX_PES_SUBTITLE1);
					break;
				case DxB_DEMUX_PESTYPE_TELETEXT_0:
					pPESFilter =
						LinuxTV_Demux_CreateFilter(pPrivateData->hDemux, STREAM_PES, ulRequestID, &pPrivateData->tSystem, TCCDEMUX_CBTeletextPES);
					LinuxTV_Demux_SetPESFilter(pPESFilter, ulPid, DMX_PES_TELETEXT0);
					break;
				case DxB_DEMUX_PESTYPE_TELETEXT_1:
					pPESFilter =
						LinuxTV_Demux_CreateFilter(pPrivateData->hDemux, STREAM_PES, ulRequestID, &pPrivateData->tSystem, TCCDEMUX_CBTeletextPES);
					LinuxTV_Demux_SetPESFilter(pPESFilter, ulPid, DMX_PES_TELETEXT1);
					break;
				case DxB_DEMUX_PESTYPE_USERDEFINE:
					pPESFilter =
						LinuxTV_Demux_CreateFilter(pPrivateData->hDemux, STREAM_PES, ulRequestID, &pPrivateData->tSystem, TCCDEMUX_CBUserDefinedPES);
					LinuxTV_Demux_SetPESFilter(pPESFilter, ulPid, DMX_PES_OTHER);
					break;
				default:
					err = OMX_ErrorBadParameter;
					break;
			}
			if (pPESFilter) {
				pPESFilter->pUserData = pUserData;
				LinuxTV_Demux_EnableFilter(pPESFilter);
			}
		}
		break;

		case OMX_IndexVendorParamDxBStopPESFilter:
		{
			UINT32    ulDemuxId, ulRequestID;
			piArg = (OMX_S32 *) ComponentParameterStructure;
			ulDemuxId = (UINT32) piArg[0];
			ulRequestID = (UINT32) piArg[1];
			ulRequestID += TCCSWDEMUXER_PESFILTER_BEGIN;

			HandleDemuxFilter pDemuxFilter= LinuxTV_Demux_GetFilter(pPrivateData->hDemux, ulRequestID);
			if (pDemuxFilter != NULL)
			{
				LinuxTV_Demux_DestroyFilter(pDemuxFilter);
			}
		}
		break;

		case OMX_IndexVendorParamDxBRegisterSectionCallback:
		{
			pfnDxB_DEMUX_Notify pfnNotify;
			pfnDxB_DEMUX_AllocBuffer pfnAllocBuffer;

			piArg = (OMX_S32 *) ComponentParameterStructure;
			pfnNotify = (pfnDxB_DEMUX_Notify) piArg[1];
			pfnAllocBuffer = (pfnDxB_DEMUX_AllocBuffer) piArg[2];
			pPrivateData->tSystem.gpfnSectionNotify = pfnNotify;
			pPrivateData->tSystem.gpfnSectionAllocBuffer = pfnAllocBuffer;
		}
		break;

		case OMX_IndexVendorParamDxBSetSectionFilter:
		{
			UINT16    usPid;
			UINT32    ulRequestID, bIsOnce, ulFilterLen, bCheckCRC;
			UINT8    *pucValue, *pucMask, *pucExclusion;
			dmx_filter_t *pSecFlt;
			HandleDemuxFilter pSectionFilter;
			INT32     iRepeat;
			void *pUserData;

			piArg = (OMX_S32 *) ComponentParameterStructure;
			pUserData = (void*) piArg[0];
			usPid = (UINT16) piArg[1];
			ulRequestID = (UINT32) piArg[2];
			bIsOnce = (UINT32) piArg[3];
			ulFilterLen = (UINT32) piArg[4];
			pucValue = (UINT8 *) piArg[5];
			pucMask = (UINT8 *) piArg[6];
			pucExclusion = (UINT8 *) piArg[7];
			bCheckCRC = (UINT32) piArg[8];
			ulRequestID += TCCSWDEMUXER_SECTIONFILTER_BEGIN;

			iRepeat = (bIsOnce == TRUE) ? 0 : 1;
			pSectionFilter = LinuxTV_Demux_CreateFilter(pPrivateData->hDemux, STREAM_SECTION, ulRequestID, &pPrivateData->tSystem, TCCDEMUX_CBSection);
			LinuxTV_Demux_SetSectionFilter(pSectionFilter, usPid, iRepeat, (int) bCheckCRC,
									(char *)pucValue, (char *)pucMask, (char *)pucExclusion, ulFilterLen);
			if(pSectionFilter)
				pSectionFilter->pUserData = pUserData;
			LinuxTV_Demux_EnableFilter(pSectionFilter);
		}
		break;

		case OMX_IndexVendorParamDxBReleaseSectionFilter:
		{
			UINT32    ulRequestID;
			piArg = (OMX_S32 *) ComponentParameterStructure;
			ulDemuxID = (UINT32) piArg[0];
			ulRequestID = (UINT32) piArg[1];
			ulRequestID += TCCSWDEMUXER_SECTIONFILTER_BEGIN;

			HandleDemuxFilter pDemuxFilter= LinuxTV_Demux_GetFilter(pPrivateData->hDemux, ulRequestID);
			if (pDemuxFilter != NULL)
			{
				LinuxTV_Demux_DestroyFilter(pDemuxFilter);
			}
		}
		break;

		case OMX_IndexVendorParamDxBRegisterCasDecryptCallback:
			break;

		case OMX_IndexVendorParamDxBSetTSIFReadSize:
		{
			UINT32    uiTSIFReadSize;
			uiTSIFReadSize = (UINT32) ComponentParameterStructure;
			//ALOGE("SET TSIF Read Size (%d)(%d)", uiTSIFReadSize, uiTSIFReadSize/188);
		}
		break;

		case OMX_IndexVendorParamDxBResetVideo:
		{
			;
		}
		break;

		case OMX_IndexVendorParamDxBResetAudio:
		{
			;
		}
		break;

		case OMX_IndexVendorParamRecStartRequest:
		{
			UINT32 ulDemuxId;
			char *pucFileName;
			piArg = (OMX_S32 *) ComponentParameterStructure;
			ulDemuxId = (UINT32) piArg[0];
			pucFileName = (char *) piArg[1];

			LinuxTV_Demux_Record_Start(pPrivateData->hDemux, pucFileName,
								pPrivateData->hVideoSlotMain->pTsParser->nPID,
								pPrivateData->hVideoSlotMain->pStreamParser->m_uiStreamType,
								pPrivateData->hAudioSlotMain->pTsParser->nPID,
								pPrivateData->hAudioSlotMain->pStreamParser->m_uiStreamType,
								pPrivateData->hPCRSlotMain->pTsParser->nPID);
		}
		break;

		case OMX_IndexVendorParamRecStopRequest:
			LinuxTV_Demux_Record_Stop(pPrivateData->hDemux);
			dxb_omx_dxb_sendevent_recording_stop(openmaxStandComp, 0);
		break;

		case OMX_IndexVendorParamDxBSetParentLock:
		{
			UINT32 ulDemuxId;
			UINT32 uiParentLock;
			piArg = (OMX_S32 *) ComponentParameterStructure;
			ulDemuxId = (UINT32) piArg[0];
			uiParentLock = (UINT32) piArg[1];
			TCCDEMUX_SetParentLock(&pPrivateData->tSystem, uiParentLock);
		}
		break;

#ifdef  SUPPORT_PVR
		case OMX_IndexVendorParamPlayStartRequest:
		{
			OMX_S32 ulPvrId;
			OMX_S8 *pucFileName;

			piArg = (OMX_S32 *) ComponentParameterStructure;
			ulPvrId = (UINT32) piArg[0];
			pucFileName = (OMX_S8 *) piArg[1];

			pPrivateData->iPlayMode = 1;

			if ((pPrivateData->uiPVRFlags & PVR_FLAG_START) == 0)
			{
				LinuxTV_Demux_SetPVR(pPrivateData->hDemux, 1);
				pPrivateData->uiPVRFlags = PVR_FLAG_START | PVR_FLAG_CHANGED;
			}
		}
		break;

		case OMX_IndexVendorParamPlayStopRequest:
		{
			OMX_S32 ulPvrId;

			piArg = (OMX_S32 *) ComponentParameterStructure;
			ulPvrId = (OMX_S32) piArg[0];

			if (pPrivateData->uiPVRFlags & PVR_FLAG_START)
			{
				LinuxTV_Demux_Resume(pPrivateData->hDemux);
				LinuxTV_Demux_SetPVR(pPrivateData->hDemux, 0);
				pPrivateData->uiPVRFlags &= ~PVR_FLAG_START;
				pPrivateData->uiPVRFlags |= PVR_FLAG_CHANGED;
			}

			pPrivateData->iPlayMode = 0;
		}
		break;

		case OMX_IndexVendorParamSeekToRequest:
		{
			OMX_S32 ulPvrId, nSeekTime;

			piArg = (OMX_S32 *) ComponentParameterStructure;
			ulPvrId = (OMX_S32) piArg[0];
			nSeekTime = (OMX_S32) piArg[1];

			if (pPrivateData->uiPVRFlags & PVR_FLAG_START)
			{
				if (nSeekTime < 0)
				{
					if (pPrivateData->uiPVRFlags & PVR_FLAG_TRICK)
					{
						LinuxTV_Demux_Resume(pPrivateData->hDemux);
						pPrivateData->uiPVRFlags &= ~PVR_FLAG_TRICK;
						pPrivateData->uiPVRFlags |= PVR_FLAG_CHANGED;
					}
				}
				else
				{
					if ((pPrivateData->uiPVRFlags & PVR_FLAG_TRICK) == 0)
					{
						LinuxTV_Demux_Stop(pPrivateData->hDemux);
						pPrivateData->uiPVRFlags |= (PVR_FLAG_TRICK | PVR_FLAG_CHANGED);
					}
				}
			}
		}
		break;

		case OMX_IndexVendorParamPlaySpeed:
		{
			OMX_S32 ulPvrId, nPlaySpeed;

			piArg = (OMX_S32 *) ComponentParameterStructure;
			ulPvrId = (OMX_S32) piArg[0];
			nPlaySpeed = (OMX_S32) piArg[1];

			if (pPrivateData->uiPVRFlags & PVR_FLAG_START)
			{
				if (nPlaySpeed == 1)
				{
					if (pPrivateData->uiPVRFlags & PVR_FLAG_TRICK)
					{
						LinuxTV_Demux_Resume(pPrivateData->hDemux);
						pPrivateData->uiPVRFlags &= ~PVR_FLAG_TRICK;
						pPrivateData->uiPVRFlags |= PVR_FLAG_CHANGED;
					}
				}
				else
				{
					if ((pPrivateData->uiPVRFlags & PVR_FLAG_TRICK) == 0)
					{
						LinuxTV_Demux_Stop(pPrivateData->hDemux);
						pPrivateData->uiPVRFlags |= PVR_FLAG_TRICK;
						pPrivateData->uiPVRFlags |= PVR_FLAG_CHANGED;
					}
				}
				pPrivateData->iPlayMode = nPlaySpeed;
			}
		}
		break;

		case OMX_IndexVendorParamPlayPause:
		{
			OMX_S32 ulPvrId, nPlayPause;

			piArg = (OMX_S32 *) ComponentParameterStructure;
			ulPvrId = (OMX_S32) piArg[0];
			nPlayPause = (OMX_S32) piArg[1];

			if (pPrivateData->uiPVRFlags & PVR_FLAG_START)
			{
				if (nPlayPause == 0)
				{
					LinuxTV_Demux_Pause(pPrivateData->hDemux);
					pPrivateData->uiPVRFlags |= PVR_FLAG_PAUSE;
				}
				else
				{
					LinuxTV_Demux_Resume(pPrivateData->hDemux);
					pPrivateData->uiPVRFlags &= ~PVR_FLAG_PAUSE;

				}
			}
		}
		break;
#endif//SUPPORT_PVR

		case OMX_IndexVendorParamDxBEnableAudioDescription:
		{
			OMX_BOOL isEnableAudioDescription;
			piArg = (OMX_S32 *) ComponentParameterStructure;
			isEnableAudioDescription = (OMX_BOOL) piArg[0];
			pPrivateData->isVisual_Impaired = isEnableAudioDescription;
		}
		break;

		case OMX_IndexVendorSetSTCDelay:
		{
/*
			int *stc_delay = (int *)ComponentParameterStructure;

			ALOGI("TYPE = %d, DECODER_ID = %d, DELAY = %d, OPTION = %d", stc_delay[0], stc_delay[1], stc_delay[2], stc_delay[3]);

			TCCDEMUX_SetSTCOffset(&pPrivateData->tSystem, stc_delay[0], stc_delay[1], stc_delay[2], stc_delay[3]);
*/
		}
		break;
		case OMX_IndexVendorParamSetTimeDelay:
		{
			piArg = (OMX_S32 *) ComponentParameterStructure;
			ALOGI("OMX_IndexVendorParamSetTimeDelay = %d", piArg[0]);
			TCCDEMUX_Compensation_STC(piArg[0]);
		}
		break;

		case OMX_IndexVendorParamDxBSetDualchannel:
		{
			UINT32 ulDemuxId;
			UINT32 uiIndex;
			piArg = (OMX_S32 *) ComponentParameterStructure;
			ulDemuxId = (UINT32) piArg[0];
			uiIndex = (UINT32) piArg[1];
			TCCDEMUX_SetDualchannel(uiIndex);
		}
		break;

		default:
			return dxb_omx_base_component_SetParameter(hComponent, nParamIndex, ComponentParameterStructure);
	}

	return err;
}

void     *dxb_omx_dxb_demux_twoport_BufferMgmtFunction(void *param)
{
	OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *) param;
	omx_dxb_demux_component_PrivateType *omx_dxb_demux_component_Private = openmaxStandComp->pComponentPrivate;
	omx_base_PortType *pOutPort[4];
	TCC_DMX_PRIVATE *pPrivateData = (TCC_DMX_PRIVATE*) omx_dxb_demux_component_Private->pPrivateData;

	pOutPort[0] = (omx_base_PortType *) omx_dxb_demux_component_Private->ports[DxB_AUDIO_PORT];
	pOutPort[1] = (omx_base_PortType *) omx_dxb_demux_component_Private->ports[DxB_AUDIO2_PORT];
	pOutPort[2] = (omx_base_PortType *) omx_dxb_demux_component_Private->ports[DxB_VIDEO_PORT];
	pOutPort[3] = (omx_base_PortType *) omx_dxb_demux_component_Private->ports[DxB_VIDEO2_PORT];

	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s \n", __func__);

	while (omx_dxb_demux_component_Private->state == OMX_StateIdle
			|| omx_dxb_demux_component_Private->state == OMX_StateExecuting
		   	|| omx_dxb_demux_component_Private->state == OMX_StatePause
		   	|| omx_dxb_demux_component_Private->transientState == OMX_TransStateLoadedToIdle)
	{
		/*Wait till the ports are being flushed */
		pthread_mutex_lock(&omx_dxb_demux_component_Private->flush_mutex);
		while (PORT_IS_BEING_FLUSHED(pOutPort[0]) || PORT_IS_BEING_FLUSHED(pOutPort[1]) ||
			   PORT_IS_BEING_FLUSHED(pOutPort[2]) || PORT_IS_BEING_FLUSHED(pOutPort[3])) {
			pthread_mutex_unlock(&omx_dxb_demux_component_Private->flush_mutex);
			if (PORT_IS_BEING_FLUSHED(pOutPort[0]))
				LinuxTV_Demux_Flush(pPrivateData->hAudioSlotMain);
			if (PORT_IS_BEING_FLUSHED(pOutPort[1]))
				LinuxTV_Demux_Flush(pPrivateData->hAudioSlotSub);
			if (PORT_IS_BEING_FLUSHED(pOutPort[2]))
				LinuxTV_Demux_Flush(pPrivateData->hVideoSlotMain);
			if (PORT_IS_BEING_FLUSHED(pOutPort[3]))
				LinuxTV_Demux_Flush(pPrivateData->hVideoSlotSub);
			tsem_up(omx_dxb_demux_component_Private->flush_all_condition);
			tsem_down(omx_dxb_demux_component_Private->flush_condition);
			pthread_mutex_lock(&omx_dxb_demux_component_Private->flush_mutex);
		}
		pthread_mutex_unlock(&omx_dxb_demux_component_Private->flush_mutex);

		/*No buffer to process. So wait here */
		if (omx_dxb_demux_component_Private->state == OMX_StateLoaded || omx_dxb_demux_component_Private->state == OMX_StateInvalid) {
			DEBUG(DEB_LEV_FULL_SEQ, "In %s Buffer Management Thread is exiting\n", __func__);
			break;
		}

		if (omx_dxb_demux_component_Private->BufferMgmtCallback) {
			if (omx_dxb_demux_component_Private->state == OMX_StateExecuting) {
				(*(omx_dxb_demux_component_Private->BufferMgmtCallback))(openmaxStandComp, NULL);
			} else
				usleep(5000);
		}

	}

	return NULL;
}
