/**
  OpenMAX ALSA sink component. This component is an audio sink that uses ALSA library.

  Copyright (C) 2007-2008  STMicroelectronics
  Copyright (C) 2007-2008 Nokia Corporation and/or its subsidiary(-ies).

  This library is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free
  Software Foundation; either version 2.1 of the License, or (at your option)
  any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
  details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St, Fifth Floor, Boston, MA
  02110-1301  USA

*/
/*******************************************************************************


*   Modified by Telechips Inc.


*   Modified date : 2020.05.26


*   Description : omx_alsasink_component.cpp


*******************************************************************************/ 
#define LOG_TAG "OMX_AudioSink"
#include <utils/Log.h>

#include <string.h>
#include <unistd.h>

#include <omxcore.h>
#include <omx_base_audio_port.h>
#include <omx_alsasink_component.h>
#include <tcc_video_common.h>
#include <OMX_TCC_Index.h>

#include "tcc_dxb_interface_demux.h"
#include "tcc_audio_render.h"
#include "omx_tcc_thread.h"

#ifdef TCC_DXB_TIMECHECK
#include "tcc_dxb_timecheck.h"
#endif

typedef long long (*pfnDemuxGetSTC)(int itype, long long lpts, unsigned int index, int log, void *pvApp);

#define DISABLE_AUDIO_OUTPUT 0
#define ENABLE_AUDIO_OUTPUT 1

/* Default Values of Time gaps for check pattern #1 to sync PTS & STC */
#define DEFAULT_SYNC_PATTERN1_PTSSTC_WAITTIME_GAP			32		// 32mS
#define DEFAULT_SYNC_PATTERN1_PTSSTC_DROPTIME_GAP			(-32)	// -32mS
#define DEFAULT_SYNC_PATTERN1_PTSSTC_JUMPTIME_GAP			5000	// 5000mS

//#define FILE_OUT_ALSA
#ifdef FILE_OUT_ALSA
/*for checking whether PCM data is valid or not*/
static FILE *pOutA;
static int gAlsaFileSize = 0;
#endif

typedef struct {
	pfnDemuxGetSTC  gfnDemuxGetSTCValue;
	void*           pvDemuxApp;
	TCCAudioRender* mRender;
	pthread_mutex_t lockAudioTrack;
	OMX_U32         nPVRFlags;

#ifdef HAVE_LINUX_PLATFORM
	unsigned int	videoFirstFrameDisplayFalg;
#endif
} TCC_ALSASINK_PRIVATE;

typedef struct ALSA_WRITE_QUEUE_ELE_T {
	OMX_U32 idx;
	OMX_U8 *data;
	OMX_U32 size;
}ALSA_WRITE_QUEUE_ELE_T;

void* dxb_omx_alsasink_BufferMgmtFunction (void* param);

static OMX_ERRORTYPE dxb_omx_alsasink_component_DoStateSet(
	OMX_COMPONENTTYPE *h_component,
	OMX_U32 ui_dest_state);

OMX_ERRORTYPE dxb_omx_alsasink_no_clockcomp_component_Constructor (OMX_COMPONENTTYPE * openmaxStandComp, OMX_STRING cComponentName)
{
	int       i;
	int       err;
	int       omxErr;
	omx_base_audio_PortType *pPort;
	omx_alsasink_component_PrivateType *omx_alsasink_component_Private;
	TCC_ALSASINK_PRIVATE *pPrivateData;
	char     *pcm_name;

	if (!openmaxStandComp->pComponentPrivate)
	{
		openmaxStandComp->pComponentPrivate = TCC_fo_calloc (__func__, __LINE__,1, sizeof (omx_alsasink_component_PrivateType));
		if (openmaxStandComp->pComponentPrivate == NULL)
		{
			return OMX_ErrorInsufficientResources;
		}
	}

#ifdef FILE_OUT_ALSA
	pOutA = fopen ("/sdcard/dvbt_audio.pcm", "wb");
	if (pOutA == NULL)
		DEBUG (DEB_LEV_ERR, "Cannot make alsa dump File\n");
	else
		DEBUG (DEB_LEV_SIMPLE_SEQ, "Making alsa dump File success\n");
	gAlsaFileSize = 0;
#endif

	omx_alsasink_component_Private = (omx_alsasink_component_PrivateType *) openmaxStandComp->pComponentPrivate;

	omx_alsasink_component_Private->ports = NULL;

	omxErr = dxb_omx_base_sink_Constructor (openmaxStandComp, cComponentName);
	if (omxErr != OMX_ErrorNone)
	{
		return OMX_ErrorInsufficientResources;
	}

	omx_alsasink_component_Private->sPortTypesParam[OMX_PortDomainAudio].nStartPortNumber = 0;
	omx_alsasink_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts = 2;

  /** Allocate Ports and call port constructor. */
	if ((omx_alsasink_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts)
		&& !omx_alsasink_component_Private->ports)
	{
		omx_alsasink_component_Private->ports =
			(omx_base_PortType **)TCC_fo_calloc (__func__, __LINE__,omx_alsasink_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts,
						sizeof (omx_base_PortType *));
		if (!omx_alsasink_component_Private->ports)
		{
			return OMX_ErrorInsufficientResources;
		}
		for (i = 0; i < (int)omx_alsasink_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts; i++)
		{
			omx_alsasink_component_Private->ports[i] = (omx_base_PortType *) TCC_fo_calloc (__func__, __LINE__,1, sizeof (omx_base_audio_PortType));
			if (!omx_alsasink_component_Private->ports[i])
			{
				return OMX_ErrorInsufficientResources;
			}
		}

		dxb_base_audio_port_Constructor (openmaxStandComp, &omx_alsasink_component_Private->ports[0], 0, OMX_TRUE);
		dxb_base_audio_port_Constructor (openmaxStandComp, &omx_alsasink_component_Private->ports[1], 1, OMX_TRUE);

	}

	pPort = (omx_base_audio_PortType *) omx_alsasink_component_Private->ports[OMX_BASE_SINK_INPUTPORT_INDEX];
	if(pPort == NULL){
		return OMX_ErrorInsufficientResources;
	}
	// set the pPort params, now that the ports exist
  /** Domain specific section for the ports. */
	pPort->sPortParam.format.audio.eEncoding = OMX_AUDIO_CodingPCM;
	/*Input pPort buffer size is equal to the size of the output buffer of the previous component */
	pPort->sPortParam.nBufferSize = AUDIO_DXB_OUT_BUFFER_SIZE;
	pPort->sPortParam.nBufferCountMin = 8;
	pPort->sPortParam.nBufferCountActual = 8;

	pPort = (omx_base_audio_PortType *) omx_alsasink_component_Private->ports[1];
	pPort->sPortParam.format.audio.eEncoding = OMX_AUDIO_CodingPCM;
	pPort->sPortParam.nBufferSize = AUDIO_DXB_OUT_BUFFER_SIZE;
	pPort->sPortParam.nBufferCountMin = 8;
	pPort->sPortParam.nBufferCountActual = 8;

	/* Initializing the function pointers */
	omx_alsasink_component_Private->BufferMgmtFunction =  dxb_omx_alsasink_BufferMgmtFunction;
	omx_alsasink_component_Private->BufferMgmtCallback = dxb_omx_alsasink_component_BufferMgmtCallback;
	omx_alsasink_component_Private->messageHandler = dxb_omx_alsasink_component_MessageHandler;
	omx_alsasink_component_Private->DoStateSet = dxb_omx_alsasink_component_DoStateSet;
	omx_alsasink_component_Private->destructor = dxb_omx_alsasink_component_Destructor;

	for(i=0; i<2; i++)
	{
		pPort = (omx_base_audio_PortType *) omx_alsasink_component_Private->ports[i];
		setHeader (&pPort->sAudioParam, sizeof (OMX_AUDIO_PARAM_PORTFORMATTYPE));
		pPort->sAudioParam.nPortIndex = 0;
		pPort->sAudioParam.nIndex = 0;
		pPort->sAudioParam.eEncoding = OMX_AUDIO_CodingPCM;
	}

	/* OMX_AUDIO_PARAM_PCMMODETYPE */
	for(i=0; i<TOTAL_AUDIO_TRACK; i++)
	{
		setHeader (&omx_alsasink_component_Private->sPCMModeParam[i], sizeof (OMX_AUDIO_PARAM_PCMMODETYPE));
		omx_alsasink_component_Private->sPCMModeParam[i].nPortIndex = 0;
		omx_alsasink_component_Private->sPCMModeParam[i].nChannels = 2;
		omx_alsasink_component_Private->sPCMModeParam[i].eNumData = OMX_NumericalDataSigned;
		omx_alsasink_component_Private->sPCMModeParam[i].eEndian = OMX_EndianLittle;
		omx_alsasink_component_Private->sPCMModeParam[i].bInterleaved = OMX_TRUE;
		omx_alsasink_component_Private->sPCMModeParam[i].nBitPerSample = 16;
		omx_alsasink_component_Private->sPCMModeParam[i].nSamplingRate = 48000;
		omx_alsasink_component_Private->sPCMModeParam[i].ePCMMode = OMX_AUDIO_PCMModeLinear;
		omx_alsasink_component_Private->sPCMModeParam[i].eChannelMapping[0] = OMX_AUDIO_ChannelNone;
		omx_alsasink_component_Private->enable_output_audio_ch_index[i] = DISABLE_AUDIO_OUTPUT;
		omx_alsasink_component_Private->setVolumeValue[i] = 100;
		omx_alsasink_component_Private->nOutputMode[i] = 0;
	}

	openmaxStandComp->SetParameter = dxb_omx_alsasink_component_SetParameter;
	openmaxStandComp->GetParameter = dxb_omx_alsasink_component_GetParameter;

	openmaxStandComp->GetConfig = dxb_omx_alsasink_component_GetConfig;
	openmaxStandComp->SetConfig = dxb_omx_alsasink_component_SetConfig;

	/* Write in the default parameters */
	omx_alsasink_component_Private->AudioPCMConfigured = 0;
	omx_alsasink_component_Private->eState = OMX_TIME_ClockStateStopped;
	omx_alsasink_component_Private->xScale = 1 << 16;
//	omx_alsasink_component_Private->hw_params = NULL;
//	omx_alsasink_component_Private->playback_handle = NULL;
	omx_alsasink_component_Private->bAudioOutStartSyncWithVideo = OMX_FALSE;
	omx_alsasink_component_Private->iPatternToCheckPTSnSTC = 0;
	omx_alsasink_component_Private->iAudioSyncWaitTimeGap = DEFAULT_SYNC_PATTERN1_PTSSTC_WAITTIME_GAP;
	omx_alsasink_component_Private->iAudioSyncDropTimeGap = DEFAULT_SYNC_PATTERN1_PTSSTC_DROPTIME_GAP;
	omx_alsasink_component_Private->iAudioSyncJumpTimeGap = DEFAULT_SYNC_PATTERN1_PTSSTC_JUMPTIME_GAP;

	// iAudioMuteConfig
	// 0 : use Mute function of AutioTrack
	// 1 : disable pcm output
	omx_alsasink_component_Private->iAudioMuteConfig = 0;
	//omx_alsasink_component_Private->iAudioMute_PCMDisable = 0;

	omx_alsasink_component_Private->balsasinkbypass = OMX_FALSE;
	omx_alsasink_component_Private->balsasinkStopRequested[0] = 0x1; //bit0:Stop Requested, bit1:Start Requested
	omx_alsasink_component_Private->balsasinkStopRequested[1] = 0x1;
	omx_alsasink_component_Private->audioskipOutput = 0;
	omx_alsasink_component_Private->iAlsaCloseFlag = 0;

#ifdef  SUPPORT_PVR
	omx_alsasink_component_Private->nPVRFlags = 0;
#endif//SUPPORT_PVR

	omx_alsasink_component_Private->pPrivateData = TCC_fo_calloc(__func__, __LINE__, 1, sizeof(TCC_ALSASINK_PRIVATE));
	if(omx_alsasink_component_Private->pPrivateData == NULL){
		return OMX_ErrorInsufficientResources;
	}
	pPrivateData = (TCC_ALSASINK_PRIVATE*) omx_alsasink_component_Private->pPrivateData;

#ifdef PCM_DIV_WRITE
	pPrivateData->pcm_division_flag[0] = 0;
	pPrivateData->pcm_division_flag[1] = 0;
	pPrivateData->pcm_div_offset = 0;
#endif
#ifdef HAVE_LINUX_PLATFORM
	pPrivateData->videoFirstFrameDisplayFalg = 0;
#endif
	pPrivateData->nPVRFlags = 0;
	pPrivateData->gfnDemuxGetSTCValue = NULL;
	pPrivateData->mRender = new TCCAudioRender(TOTAL_AUDIO_TRACK);
	pthread_mutex_init(&pPrivateData->lockAudioTrack, NULL);

	return OMX_ErrorNone;
}

OMX_ERRORTYPE dxb_omx_alsasink_no_clockcomp_component_Init (OMX_COMPONENTTYPE * openmaxStandComp)
{
	return (dxb_omx_alsasink_no_clockcomp_component_Constructor (openmaxStandComp, (OMX_STRING)ALSA_SINK_NAME));
}

/** The Destructor
 */
OMX_ERRORTYPE dxb_omx_alsasink_component_Destructor (OMX_COMPONENTTYPE * openmaxStandComp)
{
	omx_alsasink_component_PrivateType *omx_alsasink_component_Private = (omx_alsasink_component_PrivateType *)openmaxStandComp->pComponentPrivate;
	TCC_ALSASINK_PRIVATE *pPrivateData = (TCC_ALSASINK_PRIVATE*) omx_alsasink_component_Private->pPrivateData;
	OMX_U32   i;

	delete pPrivateData->mRender;

	DEBUG (DEFAULT_MESSAGES,"Free ports & comp.\n");
	/* frees port/s */
	if (omx_alsasink_component_Private->ports)
	{
		for (i = 0; i < (omx_alsasink_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
						 omx_alsasink_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts); i++)
		{
			if (omx_alsasink_component_Private->ports[i])
				omx_alsasink_component_Private->ports[i]->PortDestructor (omx_alsasink_component_Private->ports[i]);
		}
		TCC_fo_free (__func__, __LINE__, (omx_alsasink_component_Private->ports));
		omx_alsasink_component_Private->ports = NULL;
	}

	pthread_mutex_destroy(&pPrivateData->lockAudioTrack);
	TCC_fo_free(__func__, __LINE__, omx_alsasink_component_Private->pPrivateData);

	return dxb_omx_base_sink_Destructor (openmaxStandComp);

}

#ifdef  SUPPORT_PVR
static int CheckPVR(omx_alsasink_component_PrivateType *omx_private, OMX_U32 ulInputBufferFlags)
{
	OMX_U32 iPVRState, iBufferState;

	iPVRState = (omx_private->nPVRFlags & PVR_FLAG_START) ? 1 : 0;
	iBufferState = (ulInputBufferFlags & OMX_BUFFERFLAG_FILE_PLAY) ? 1 : 0;
	if (iPVRState != iBufferState)
	{
		return 1; // skip
	}

	iPVRState = (omx_private->nPVRFlags & PVR_FLAG_TRICK) ? 1 : 0;
	iBufferState = (ulInputBufferFlags & OMX_BUFFERFLAG_TRICK_PLAY) ? 1 : 0;
	if (iPVRState != iBufferState)
	{
		return 1; // skip
	}

	if (ulInputBufferFlags & OMX_BUFFERFLAG_TRICK_PLAY)
	{
		return 1; // skip
	}

	if (omx_private->nPVRFlags & PVR_FLAG_PAUSE)
	{
		return 2; // pause
	}

	return 0; // OK
}
#endif//SUPPORT_PVR

/**
 * This function plays the input buffer. When fully consumed it returns.
 */
void dxb_omx_alsasink_component_BufferMgmtCallback (OMX_COMPONENTTYPE * openmaxStandComp,
												OMX_BUFFERHEADERTYPE * inputbuffer)
{
    OMX_U32   uiBufferLen = inputbuffer->nFilledLen;
    OMX_U32   frameSize;
    OMX_S32   written;
    OMX_U32   nSpdifMode = 0;
    OMX_U32   nSamplingRate = 0;
    OMX_U32   nChannels = 0;
	OMX_U32	  nEnable = 0;
    OMX_S32   stc_delay[4];
    OMX_BOOL  first_frame = OMX_FALSE;
    OMX_S32   ret;
    //OMX_U32   latency = 0;
    
    long long llSTC=0, llPTS=0;

    omx_alsasink_component_PrivateType *omx_alsasink_component_Private =
        (omx_alsasink_component_PrivateType *) openmaxStandComp->pComponentPrivate;
	TCC_ALSASINK_PRIVATE *pPrivateData = (TCC_ALSASINK_PRIVATE*) omx_alsasink_component_Private->pPrivateData;

	nEnable = omx_alsasink_component_Private->enable_output_audio_ch_index[inputbuffer->nDecodeID];
	if(nEnable){
		if (omx_alsasink_component_Private->balsasinkStopRequested[inputbuffer->nDecodeID] == 0x1) //if Stop happens
		{	//No Start Operation
			inputbuffer->nFilledLen = 0;
			return;
		}

		if (omx_alsasink_component_Private->balsasinkStopRequested[inputbuffer->nDecodeID] == 0x2) //if Start happens
		{
			first_frame = OMX_TRUE;
			omx_alsasink_component_Private->balsasinkStopRequested[inputbuffer->nDecodeID] = 0x3; //set to first frame
		}
	}else{
		omx_alsasink_component_Private->balsasinkStopRequested[inputbuffer->nDecodeID] &= ~0x1; //Clear first frame
	}

    #ifdef PCM_INFO_SIZE
    if (uiBufferLen <= PCM_INFO_SIZE)
    {
        inputbuffer->nFilledLen = 0;
        return;
    }

    uiBufferLen -= 4;
    memcpy(&nSpdifMode, inputbuffer->pBuffer + uiBufferLen, 4);
    uiBufferLen -= 4;
    memcpy(&nChannels, inputbuffer->pBuffer + uiBufferLen, 4);
    uiBufferLen -= 4;
    memcpy(&nSamplingRate, inputbuffer->pBuffer + uiBufferLen, 4);

    *((int*)(inputbuffer->pBuffer + uiBufferLen)) = 0; // Clear the sample rate
    #endif//PCM_INFO_SIZE

    if (nSamplingRate != 0 && nChannels != 0)
    {
        if (omx_alsasink_component_Private->sPCMModeParam[inputbuffer->nDecodeID].nSamplingRate != nSamplingRate)
        {
            omx_alsasink_component_Private->alsa_configure_request = TRUE;
            omx_alsasink_component_Private->sPCMModeParam[inputbuffer->nDecodeID].nSamplingRate = nSamplingRate;
        }

        if (omx_alsasink_component_Private->sPCMModeParam[inputbuffer->nDecodeID].nChannels != nChannels)
        {
            omx_alsasink_component_Private->alsa_configure_request = TRUE;
            omx_alsasink_component_Private->sPCMModeParam[inputbuffer->nDecodeID].nChannels = nChannels;
        }

        if (omx_alsasink_component_Private->nOutputMode[inputbuffer->nDecodeID] != nSpdifMode)
        {
            omx_alsasink_component_Private->nOutputMode[inputbuffer->nDecodeID] = nSpdifMode;
            omx_alsasink_component_Private->alsa_configure_request = TRUE;
        }
    }

	if ((omx_alsasink_component_Private->alsa_configure_request == TRUE ||first_frame == OMX_TRUE) && omx_alsasink_component_Private->iAlsaCloseFlag == 0)
    {
        DEBUG (DEB_LEV_FULL_SEQ, "Framesize is %u chl=%d bufSize=%d\n", (int) frameSize,
                (int) omx_alsasink_component_Private->sPCMModeParam[inputbuffer->nDecodeID].nChannels, (int) uiBufferLen);

		ALOGD("\033[32m ALSA open alsa_configure_request[%d] first_frame[%d] iAlsaCloseFlag[%d] \033[m\n", 
			omx_alsasink_component_Private->alsa_configure_request, first_frame, omx_alsasink_component_Private->iAlsaCloseFlag);
        ret = pPrivateData->mRender->set(inputbuffer->nDecodeID,
                          omx_alsasink_component_Private->sPCMModeParam[inputbuffer->nDecodeID].nSamplingRate,
                          omx_alsasink_component_Private->sPCMModeParam[inputbuffer->nDecodeID].nChannels,
                          omx_alsasink_component_Private->sPCMModeParam[inputbuffer->nDecodeID].nBitPerSample,
                          omx_alsasink_component_Private->sPCMModeParam[inputbuffer->nDecodeID].eEndian,
                          uiBufferLen,
                          omx_alsasink_component_Private->nOutputMode[inputbuffer->nDecodeID],
                          (void *)(pPrivateData->gfnDemuxGetSTCValue), (void *)(pPrivateData->pvDemuxApp));
        if (ret != 0)
        {
            ALOGE ("TCCAudioRender init error");
            inputbuffer->nFilledLen = 0;
            return;
        }

        omx_alsasink_component_Private->alsa_configure_request = FALSE;
    }

	//int latency = mRender->latency_ex();
    if (pPrivateData->gfnDemuxGetSTCValue && inputbuffer->nTimeStamp && !omx_alsasink_component_Private->balsasinkbypass)
    {
        /* In This case We don't use clock components.
         * At this moments We need to compare between STC(Sytem Time Clock) and PTS (Present Time Stamp)
         * STC : ms, PTS : us
         */
        llPTS = inputbuffer->nTimeStamp/1000; //557 is Android Audio system delay

        //llPTS -= latency;

        /* itype 0:Audio, 1:Video Other :: Get STC Value
        * ret :: -1:drop frame, 0:display frame, 1:wait frame
        */
        llSTC = pPrivateData->gfnDemuxGetSTCValue(DxB_SYNC_AUDIO, llPTS, inputbuffer->nDecodeID, 1, pPrivateData->pvDemuxApp); //
        if (llSTC == DxB_SYNC_WAIT || llSTC == DxB_SYNC_LONG_WAIT)
        {
            usleep(2500); //Wait
            return;
        }
        else if (llSTC == DxB_SYNC_DROP)
        {
            inputbuffer->nFilledLen = 0;
            return;
        }
    }

    #ifdef FILE_OUT_ALSA
    if (pOutA)
    {
        fwrite (inputbuffer->pBuffer, 1, uiBufferLen, pOutA);
        gAlsaFileSize += uiBufferLen;
        DEBUG (DEB_LEV_ERR, "ALSA Dump File size %d\n", gAlsaFileSize);
        if (gAlsaFileSize > 4 * 1024 * 1024)
        {
            fclose (pOutA);
            sync();
            DEBUG (DEB_LEV_ERR, "ALSA Dump File Close success\n");
            pOutA = NULL;
            gAlsaFileSize = 0;
        }
    }
    #endif

	if (omx_alsasink_component_Private->enable_output_audio_ch_index[inputbuffer->nDecodeID] == DISABLE_AUDIO_OUTPUT
	|| omx_alsasink_component_Private->audioskipOutput == 1
	|| omx_alsasink_component_Private->frameDropFlag == 1
	|| omx_alsasink_component_Private->iAlsaCloseFlag == 1
	|| omx_alsasink_component_Private->balsasinkStopRequested[inputbuffer->nDecodeID] == 0x1)
	{
		inputbuffer->nFilledLen = 0;
		return;
	}

	//for Seamless Switching of ISDB-T
	if (pPrivateData->mRender->error_check(inputbuffer->nDecodeID) != 0)
	{
		pPrivateData->mRender->setCurrentIdx(inputbuffer->nDecodeID);
	}

    if (omx_alsasink_component_Private->balsasinkStopRequested[inputbuffer->nDecodeID] == 0x2)
    {
        if (omx_alsasink_component_Private->eState == OMX_TIME_ClockStateWaitingForStartTime)
        {
            omx_alsasink_component_Private->eState = OMX_TIME_ClockStateRunning;
            (*(omx_alsasink_component_Private->callbacks->EventHandler))
                    (openmaxStandComp,
                    omx_alsasink_component_Private->callbackData,
                    OMX_EventVendorBufferingStart,
                    0, 0, 0);
        }
    }

	pthread_mutex_lock(&pPrivateData->lockAudioTrack);
	written = pPrivateData->mRender->write(inputbuffer->nDecodeID, inputbuffer->pBuffer, uiBufferLen, llPTS);

    inputbuffer->nFilledLen = 0;
	pthread_mutex_unlock(&pPrivateData->lockAudioTrack);
}

OMX_ERRORTYPE dxb_omx_alsasink_component_MessageHandler (OMX_COMPONENTTYPE * openmaxStandComp,
													 internalRequestMessageType * message)
{
	omx_alsasink_component_PrivateType *omx_alsasink_component_Private =
		(omx_alsasink_component_PrivateType *) openmaxStandComp->pComponentPrivate;
	OMX_ERRORTYPE err;
	OMX_STATETYPE eCurrentState = omx_alsasink_component_Private->state;
	DEBUG (DEB_LEV_SIMPLE_SEQ, "In %s\n", __func__);

	if (message->messageType == OMX_CommandStateSet)
	{
		if ((message->messageParam == OMX_StateExecuting) && (omx_alsasink_component_Private->state == OMX_StateIdle))
		{
			;
		}
	}
	/** Execute the base message handling */
	err = dxb_omx_base_component_MessageHandler (openmaxStandComp, message);
	if (message->messageType == OMX_CommandStateSet)
	{
		if ((message->messageParam == OMX_StateIdle)
			&& (eCurrentState == OMX_StateExecuting || eCurrentState == OMX_StatePause))
		{
			;
		}
	}

	return err;
}

static OMX_ERRORTYPE dxb_omx_alsasink_component_DoStateSet(
	OMX_COMPONENTTYPE *h_component,
	OMX_U32 ui_dest_state)
{
	OMX_ERRORTYPE e_error = OMX_ErrorNone;

	OMX_STATETYPE e_state;

	omx_alsasink_component_PrivateType *p_priv = (omx_alsasink_component_PrivateType *)h_component->pComponentPrivate;
	TCC_ALSASINK_PRIVATE *pPrivateData = (TCC_ALSASINK_PRIVATE*) p_priv->pPrivateData;

//	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s\n", __func__);

	e_state = p_priv->state;

	e_error = dxb_omx_base_component_DoStateSet(h_component, ui_dest_state);
	if(e_error != OMX_ErrorNone)
	{
		DEBUG(DEB_LEV_ERR, "[ERROR] %s is failed\n", __func__);
		return e_error;
	}

	if((ui_dest_state == OMX_StateIdle) && (e_state == OMX_StateExecuting))
	{
		int i;
		pthread_mutex_lock(&pPrivateData->lockAudioTrack);
		for(i=0; i<TOTAL_AUDIO_TRACK; i++)
		{
			pPrivateData->mRender->close(i);
		}
		pthread_mutex_unlock(&pPrivateData->lockAudioTrack);
	}
	return OMX_ErrorNone;
}

OMX_ERRORTYPE dxb_omx_alsasink_component_SetConfig (OMX_IN OMX_HANDLETYPE hComponent,
												OMX_IN OMX_INDEXTYPE nIndex, OMX_IN OMX_PTR pComponentConfigStructure)
{
	OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *) hComponent;

	omx_alsasink_component_PrivateType *omx_alsasink_component_Private = (omx_alsasink_component_PrivateType *)openmaxStandComp->pComponentPrivate;

	if (pComponentConfigStructure == NULL)
	{
		return OMX_ErrorBadParameter;
	}

	switch (nIndex)
	{
	default:	// delegate to superclass
		return dxb_omx_base_component_SetConfig (hComponent, nIndex, pComponentConfigStructure);
	}
	return OMX_ErrorNone;

}

OMX_ERRORTYPE dxb_omx_alsasink_component_GetConfig (OMX_IN OMX_HANDLETYPE hComponent,
												OMX_IN OMX_INDEXTYPE nIndex,
												OMX_INOUT OMX_PTR pComponentConfigStructure)
{

	OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *) hComponent;
	omx_alsasink_component_PrivateType *omx_alsasink_component_Private = (omx_alsasink_component_PrivateType *)openmaxStandComp->pComponentPrivate;

	if (pComponentConfigStructure == NULL)
	{
		return OMX_ErrorBadParameter;
	}

	switch (nIndex)
	{
	default:	// delegate to superclass
		return dxb_omx_base_component_GetConfig (hComponent, nIndex, pComponentConfigStructure);
	}
	return OMX_ErrorNone;

}

OMX_ERRORTYPE dxb_omx_alsasink_component_SetParameter (OMX_IN OMX_HANDLETYPE hComponent,
												   OMX_IN OMX_INDEXTYPE nParamIndex,
												   OMX_IN OMX_PTR ComponentParameterStructure)
{
	OMX_S32   *piArg;
	int       omxErr = OMX_ErrorNone;
	OMX_AUDIO_PARAM_PORTFORMATTYPE *pAudioPortFormat;
	OMX_OTHER_PARAM_PORTFORMATTYPE *pOtherPortFormat;
	OMX_AUDIO_PARAM_MP3TYPE *pAudioMp3;
	OMX_U32   portIndex;


	/* Check which structure we are being fed and make control its header */
	OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *) hComponent;
	omx_alsasink_component_PrivateType *omx_alsasink_component_Private = (omx_alsasink_component_PrivateType *)openmaxStandComp->pComponentPrivate;
	omx_base_audio_PortType *pPort =
		(omx_base_audio_PortType *) omx_alsasink_component_Private->ports[OMX_BASE_SINK_INPUTPORT_INDEX];
	TCC_ALSASINK_PRIVATE *pPrivateData = (TCC_ALSASINK_PRIVATE*) omx_alsasink_component_Private->pPrivateData;

	if (ComponentParameterStructure == NULL)
	{
		return OMX_ErrorBadParameter;
	}

	DEBUG (DEB_LEV_SIMPLE_SEQ, "   Setting parameter %i\n", nParamIndex);

  /** Each time we are (re)configuring the hw_params thing
  * we need to reinitialize it, otherwise previous changes will not take effect.
  * e.g.: changing a previously configured sampling rate does not have
  * any effect if we are not calling this each time.
  */

	switch ((UINT32)nParamIndex)
	{
	case OMX_IndexParamAudioPortFormat:
		pAudioPortFormat = (OMX_AUDIO_PARAM_PORTFORMATTYPE *) ComponentParameterStructure;
		portIndex = pAudioPortFormat->nPortIndex;
		/*Check Structure Header and verify component state */
		omxErr =
			dxb_omx_base_component_ParameterSanityCheck (hComponent, portIndex, pAudioPortFormat,
													 sizeof (OMX_AUDIO_PARAM_PORTFORMATTYPE));
		if (omxErr != OMX_ErrorNone)
		{
			DEBUG (DEB_LEV_ERR, "In %s Parameter Check Error=%x\n", __func__, omxErr);
			break;
		}
		if (portIndex < 1)
		{
			memcpy (&pPort->sAudioParam, pAudioPortFormat, sizeof (OMX_AUDIO_PARAM_PORTFORMATTYPE));
		}
		else
		{
			return OMX_ErrorBadPortIndex;
		}
		break;

	case OMX_IndexParamAudioPcm:
		{
			unsigned int rate;
			int iChannels;
			OMX_AUDIO_PARAM_PCMMODETYPE *sPCMModeParam = (OMX_AUDIO_PARAM_PCMMODETYPE *) ComponentParameterStructure;

			portIndex = sPCMModeParam->nPortIndex;
			omx_alsasink_component_Private->AudioPCMConfigured = 1;
			if (sPCMModeParam->nPortIndex != omx_alsasink_component_Private->sPCMModeParam[0].nPortIndex)
			{
				DEBUG (DEB_LEV_ERR, "Error setting input pPort index\n");
				omxErr = OMX_ErrorBadParameter;
				break;
			}
			if(sPCMModeParam->nSamplingRate == 0)
			{
				ALOGE("Sampling Rate Change error to %d, set to 48000 forcibly", (int)sPCMModeParam->nSamplingRate);
				omxErr = OMX_ErrorBadParameter;
				break;
			}

			if(sPCMModeParam->nChannels == 0)
			{
				ALOGE("Channel Number Change error to %d, set to 2 forcibly", (int)sPCMModeParam->nChannels);
				omxErr = OMX_ErrorBadParameter;
				break;
			}
			memcpy (&omx_alsasink_component_Private->sPCMModeParam, ComponentParameterStructure, sizeof (OMX_AUDIO_PARAM_PCMMODETYPE));
			omx_alsasink_component_Private->alsa_configure_request = TRUE;
			ALOGD("Changing SamplingRate[%d]Channel[%d]\n", (int)sPCMModeParam->nSamplingRate, (int)sPCMModeParam->nChannels);
		}
		break;
	case OMX_IndexParamAudioMp3:
		pAudioMp3 = (OMX_AUDIO_PARAM_MP3TYPE *) ComponentParameterStructure;
		/*Check Structure Header and verify component state */
		omxErr =
			dxb_omx_base_component_ParameterSanityCheck (hComponent, pAudioMp3->nPortIndex, pAudioMp3,
													 sizeof (OMX_AUDIO_PARAM_MP3TYPE));
		if (omxErr != OMX_ErrorNone)
		{
			DEBUG (DEB_LEV_ERR, "In %s Parameter Check Error=%x\n", __func__, omxErr);
			break;
		}
		break;
	case OMX_IndexVendorParamAlsaSinkSetVolume:
		{
			int *pVolumeValue = (int*)ComponentParameterStructure;
			int volumeID = pVolumeValue[0];
			float fVolume = pVolumeValue[1]/100.0;

			omx_alsasink_component_Private->setVolumeValue[volumeID] = pVolumeValue[1];
			pPrivateData->mRender->setVolume(volumeID, fVolume, fVolume);
			ALOGD("Current Volume L:%f R:%f\n", fVolume, fVolume);
		}
		break;
	case OMX_IndexVendorParamDxBGetSTCFunction:
		{
			OMX_S32 *piArg = (OMX_S32*) ComponentParameterStructure;
			pPrivateData->gfnDemuxGetSTCValue = (pfnDemuxGetSTC) piArg[0];
			pPrivateData->pvDemuxApp = (void*) piArg[1];
		}
		break;
	case OMX_IndexVendorParamAlsaSinkMute:
		{
			int *piArg = (int*)ComponentParameterStructure;
			bool bMute;

			bMute = piArg[0];
			if(omx_alsasink_component_Private->iAudioMuteConfig == 0) {
				pPrivateData->mRender->mute(bMute);
			}
			else {
				//omx_alsasink_component_Private->iAudioMute_PCMDisable = bMute;
				omx_alsasink_component_Private->enable_output_audio_ch_index[0] = bMute? DISABLE_AUDIO_OUTPUT:ENABLE_AUDIO_OUTPUT;
				omx_alsasink_component_Private->enable_output_audio_ch_index[1] = bMute? DISABLE_AUDIO_OUTPUT:ENABLE_AUDIO_OUTPUT;
			}
			ALOGD("Current Mute State:%d\n", bMute);
		}
		break;

	case OMX_IndexVendorParamAlsaSinkMuteConfig:
		{
			int *piArg = (int*)ComponentParameterStructure;
			omx_alsasink_component_Private->iAudioMuteConfig = piArg[0];
			ALOGD("iAudioMuteConfig:%d\n", omx_alsasink_component_Private->iAudioMuteConfig);
		}
		break;

	case OMX_IndexVendorParamAlsaSinkAudioStartNotify:
		{
			if(omx_alsasink_component_Private->eState == OMX_TIME_ClockStateStopped) {
				omx_alsasink_component_Private->eState = OMX_TIME_ClockStateWaitingForStartTime;
			}
		}
		break;

	case OMX_IndexVendorParamSetSinkByPass:
		{
			OMX_U32 ulDevId;
			OMX_S32  *piArg;
			piArg = (OMX_S32 *) ComponentParameterStructure;
			ulDevId = (OMX_U32) piArg[0];
			omx_alsasink_component_Private->balsasinkbypass = (OMX_BOOL) piArg[1];
		}
		break;

   	case OMX_IndexVendorParamStopRequest:
		{
			OMX_U32 ulDevId;
			OMX_S32  *piArg;
			piArg = (OMX_S32 *) ComponentParameterStructure;
			ulDevId = (OMX_U32) piArg[0];
			if((ulDevId == 0)||(ulDevId == 1))
			{
				omx_alsasink_component_Private->eState = OMX_TIME_ClockStateStopped;
				if((OMX_BOOL) piArg[1] == TRUE) //if stop
				{
					omx_alsasink_component_Private->balsasinkStopRequested[ulDevId] = 0x1; //Stop Requested
					pthread_mutex_lock(&pPrivateData->lockAudioTrack);
					pPrivateData->mRender->close(ulDevId);
					pthread_mutex_unlock(&pPrivateData->lockAudioTrack);

					if(omx_alsasink_component_Private->bAudioOutStartSyncWithVideo == OMX_TRUE)
					{
						pPrivateData->mRender->mute(TRUE);
					}
					ALOGD("%s:audioTrack Stopped",__func__);
#ifdef HAVE_LINUX_PLATFORM
					pPrivateData->videoFirstFrameDisplayFalg = FALSE;
#endif
				}
				else //if start
				{
					omx_alsasink_component_Private->balsasinkStopRequested[ulDevId] = 0x2; //Start Requested
					if (omx_alsasink_component_Private->bAudioOutStartSyncWithVideo == OMX_FALSE)
					{
						pPrivateData->mRender->mute(FALSE);
					}
#ifdef HAVE_LINUX_PLATFORM
					pPrivateData->videoFirstFrameDisplayFalg = TRUE;
#endif
				}
				ALOGD("balsasinkStopRequested[dev:%d][flag:%d][%d]", ulDevId, omx_alsasink_component_Private->balsasinkStopRequested[ulDevId], (OMX_BOOL) piArg[1]);
			}
		}
		break;

	case OMX_IndexVendorParamCloseAlsaFlag:
		OMX_S32  *piArg;
		piArg = (OMX_S32 *) ComponentParameterStructure;
		omx_alsasink_component_Private->iAlsaCloseFlag = piArg[0];
		ALOGD("\033[32m OMX_IndexVendorParamStopRequest iAlsaCloseFlag[%d] \033[m\n", omx_alsasink_component_Private->iAlsaCloseFlag);
		break;


	case OMX_IndexVendorNotifyStartTimeSyncWithVideo:
		{
			if (omx_alsasink_component_Private->bAudioOutStartSyncWithVideo == OMX_TRUE)
			{
				pPrivateData->mRender->mute(FALSE);
			}
#ifdef HAVE_LINUX_PLATFORM
			pPrivateData->videoFirstFrameDisplayFalg = FALSE;
			ALOGE("Set videoFirstFrameDisplayFalg [%d]", pPrivateData->videoFirstFrameDisplayFalg);
#endif
		}
		break;

	case OMX_IndexVendorParamSetEnableAudioStartSyncWithVideo:
		{
			OMX_S32  *piArg;
			piArg = (OMX_S32 *) ComponentParameterStructure;
			omx_alsasink_component_Private->bAudioOutStartSyncWithVideo = (OMX_BOOL) piArg[0];
			ALOGD("Set Enable Audio Start Sync With Video [%d]", omx_alsasink_component_Private->bAudioOutStartSyncWithVideo);
		}
		break;

	case OMX_IndexVendorParamSetPatternToCheckPTSnSTC:
		{
			OMX_S32  *piArg;
			piArg = (OMX_S32 *) ComponentParameterStructure;
			omx_alsasink_component_Private->iPatternToCheckPTSnSTC = (int)piArg[0];
			omx_alsasink_component_Private->iAudioSyncWaitTimeGap = (int)piArg[1];	//mS
			omx_alsasink_component_Private->iAudioSyncDropTimeGap = (int)piArg[2];	//mS
			omx_alsasink_component_Private->iAudioSyncJumpTimeGap = (int)piArg[3];	//mS

			/* Sycn Pattern #1 */
			if ( omx_alsasink_component_Private->iPatternToCheckPTSnSTC == 1 )
			{
				if ( omx_alsasink_component_Private->iAudioSyncWaitTimeGap < DEFAULT_SYNC_PATTERN1_PTSSTC_WAITTIME_GAP )
					omx_alsasink_component_Private->iAudioSyncWaitTimeGap = DEFAULT_SYNC_PATTERN1_PTSSTC_WAITTIME_GAP;
				if ( omx_alsasink_component_Private->iAudioSyncDropTimeGap > DEFAULT_SYNC_PATTERN1_PTSSTC_DROPTIME_GAP )
					omx_alsasink_component_Private->iAudioSyncDropTimeGap = DEFAULT_SYNC_PATTERN1_PTSSTC_DROPTIME_GAP;
				if ( omx_alsasink_component_Private->iAudioSyncJumpTimeGap < DEFAULT_SYNC_PATTERN1_PTSSTC_JUMPTIME_GAP )
					omx_alsasink_component_Private->iAudioSyncJumpTimeGap = DEFAULT_SYNC_PATTERN1_PTSSTC_JUMPTIME_GAP;
			}

			ALOGD("Set Pattern #[%d] to check PTS&STC - WaitTimeGap[%dmS] DropTimeGap[%dmS] JumpTimeGap[%dmS] ",
				omx_alsasink_component_Private->iPatternToCheckPTSnSTC,
				omx_alsasink_component_Private->iAudioSyncWaitTimeGap,
				omx_alsasink_component_Private->iAudioSyncDropTimeGap,
				omx_alsasink_component_Private->iAudioSyncJumpTimeGap);
		}
		break;

	case OMX_IndexVendorParamAudioOutputCh:
		{
			OMX_S32 devID, isEnableAudioOutput;
			OMX_S32  *piArg;
			piArg = (OMX_S32 *)ComponentParameterStructure;
			devID = (OMX_S32)piArg[0];
			isEnableAudioOutput = (OMX_S32)piArg[1];

			ALOGD("[ALSASINK] Change Audio Output to Ch[%d:%d] \n", (int)devID, (int)isEnableAudioOutput);
			omx_alsasink_component_Private->enable_output_audio_ch_index[devID] = isEnableAudioOutput? ENABLE_AUDIO_OUTPUT:DISABLE_AUDIO_OUTPUT;
			//TCCALSA_SoundReset(hComponent);
		}
		break;

	case OMX_IndexVendorSetSTCDelay:
		{
		}
		break;

	case OMX_IndexVendorParamResetRequestForALSASink:
		omx_alsasink_component_Private->alsa_configure_request = TRUE;
		break;
	case OMX_IndexVendorParamISDBTSkipAudio:
		{
			OMX_U32 audioSkipCheck;
			piArg = (OMX_S32 *)ComponentParameterStructure;
			audioSkipCheck = (OMX_U32) piArg[0];
			if(audioSkipCheck == 0){
				omx_alsasink_component_Private->audioskipOutput = 1;
			}else{
				omx_alsasink_component_Private->audioskipOutput = 0;
			}
		}
		break;
	case OMX_IndexVendorParamSetFrameDropFlag:
		{
			piArg = (OMX_S32 *)ComponentParameterStructure;
			omx_alsasink_component_Private->frameDropFlag = (OMX_BOOL)piArg[0];
			ALOGD("[ALSASINK] frame drop flag [%d] \n", omx_alsasink_component_Private->frameDropFlag);
		}
		break;
	default:	/*Call the base component function */
		return dxb_omx_base_component_SetParameter (hComponent, nParamIndex, ComponentParameterStructure);
	}
	return (OMX_ERRORTYPE) omxErr;
}

OMX_ERRORTYPE dxb_omx_alsasink_component_GetParameter (OMX_IN OMX_HANDLETYPE hComponent,
												   OMX_IN OMX_INDEXTYPE nParamIndex,
												   OMX_INOUT OMX_PTR ComponentParameterStructure)
{
	OMX_AUDIO_PARAM_PORTFORMATTYPE *pAudioPortFormat;
	OMX_OTHER_PARAM_PORTFORMATTYPE *pOtherPortFormat;
	OMX_ERRORTYPE err = OMX_ErrorNone;
	OMX_AUDIO_CONFIG_INFOTYPE *info;

	OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *) hComponent;
	omx_alsasink_component_PrivateType *omx_alsasink_component_Private = (omx_alsasink_component_PrivateType *)openmaxStandComp->pComponentPrivate;
	omx_base_audio_PortType *pPort = (omx_base_audio_PortType *) omx_alsasink_component_Private->ports[OMX_BASE_SINK_INPUTPORT_INDEX];
	TCC_ALSASINK_PRIVATE *pPrivateData = (TCC_ALSASINK_PRIVATE*) omx_alsasink_component_Private->pPrivateData;

	if (ComponentParameterStructure == NULL)
	{
		return OMX_ErrorBadParameter;
	}
	DEBUG (DEB_LEV_SIMPLE_SEQ, "   Getting parameter %i\n", nParamIndex);
	/* Check which structure we are being fed and fill its header */
	switch ((UINT32)nParamIndex)
	{
	case OMX_IndexParamAudioInit:
		if ((err = checkHeader (ComponentParameterStructure, sizeof (OMX_PORT_PARAM_TYPE))) != OMX_ErrorNone)
		{
			break;
		}
		memcpy (ComponentParameterStructure, &omx_alsasink_component_Private->sPortTypesParam[OMX_PortDomainAudio],
				sizeof (OMX_PORT_PARAM_TYPE));
		break;
	case OMX_IndexParamAudioPortFormat:
		pAudioPortFormat = (OMX_AUDIO_PARAM_PORTFORMATTYPE *) ComponentParameterStructure;
		if ((err = checkHeader (ComponentParameterStructure, sizeof (OMX_AUDIO_PARAM_PORTFORMATTYPE))) != OMX_ErrorNone)
		{
			break;
		}
		if (pAudioPortFormat->nPortIndex < 1)
		{
			memcpy (pAudioPortFormat, &pPort->sAudioParam, sizeof (OMX_AUDIO_PARAM_PORTFORMATTYPE));
		}
		else
		{
			return OMX_ErrorBadPortIndex;
		}
		break;
	case OMX_IndexParamAudioPcm:
		if (((OMX_AUDIO_PARAM_PCMMODETYPE *) ComponentParameterStructure)->nPortIndex !=
			omx_alsasink_component_Private->sPCMModeParam[0].nPortIndex)
		{
//			return OMX_ErrorBadParameter;
		}
		if ((err = checkHeader (ComponentParameterStructure, sizeof (OMX_AUDIO_PARAM_PCMMODETYPE))) != OMX_ErrorNone)
		{
			break;
		}
		memcpy (ComponentParameterStructure, &omx_alsasink_component_Private->sPCMModeParam[0],
				sizeof (OMX_AUDIO_PARAM_PCMMODETYPE));
		break;
	default:	/*Call the base component function */
		return dxb_omx_base_component_GetParameter (hComponent, nParamIndex, ComponentParameterStructure);
	}
	return err;
}

void* dxb_omx_alsasink_BufferMgmtFunction (void* param)
{
  OMX_COMPONENTTYPE* openmaxStandComp = (OMX_COMPONENTTYPE*)param;
  omx_base_component_PrivateType* omx_base_component_Private  = (omx_base_component_PrivateType*)openmaxStandComp->pComponentPrivate;
  omx_base_sink_PrivateType*      omx_base_sink_Private       = (omx_base_sink_PrivateType*)omx_base_component_Private;
  omx_base_PortType               *pInPort[2]; //                    = (omx_base_PortType *)omx_base_sink_Private->ports[OMX_BASE_SINK_INPUTPORT_INDEX];
  tsem_t*                         pInputSem[2]; //                   = pInPort->pBufferSem;
  queue_t*                        pInputQueue[2]; //                 = pInPort->pBufferQueue;
  OMX_BUFFERHEADERTYPE*           pInputBuffer[2]; //                = NULL;
  OMX_COMPONENTTYPE*              target_component;
  OMX_BOOL                        isInputBufferNeeded[2]; //         = OMX_TRUE;
  int                             inBufExchanged[2]; //              = 0;
  int i;

  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s \n", __func__);

  for (i=0; i < 2; i++) {
	pInPort[i] = (omx_base_PortType *)omx_base_sink_Private->ports[i];
	pInputSem[i] = pInPort[i]->pBufferSem;
	pInputQueue[i] = pInPort[i]->pBufferQueue;
	pInputBuffer[i] = NULL;
	isInputBufferNeeded[i] = OMX_TRUE;
	inBufExchanged[i] = 0;
  }

  while(omx_base_component_Private->state == OMX_StateIdle || omx_base_component_Private->state == OMX_StateExecuting ||  omx_base_component_Private->state == OMX_StatePause ||
    omx_base_component_Private->transientState == OMX_TransStateLoadedToIdle){

    /*Wait till the ports are being flushed*/
    pthread_mutex_lock(&omx_base_sink_Private->flush_mutex);
    while( PORT_IS_BEING_FLUSHED(pInPort[0]) || PORT_IS_BEING_FLUSHED(pInPort[1])) {
      pthread_mutex_unlock(&omx_base_sink_Private->flush_mutex);

		for (i=0; i < 2; i++) {
			if(isInputBufferNeeded[i]==OMX_FALSE) {
				pInPort[i]->ReturnBufferFunction(pInPort[i],pInputBuffer[i]);
				inBufExchanged[i]--;
				pInputBuffer[i]=NULL;
				isInputBufferNeeded[i]=OMX_TRUE;
				DEBUG(DEB_LEV_FULL_SEQ, "Ports are flushing,so returning input buffer\n");
			}
		}
      DEBUG(DEB_LEV_FULL_SEQ, "In %s signalling flush all condition \n", __func__);

      tsem_up(omx_base_sink_Private->flush_all_condition);
      tsem_down(omx_base_sink_Private->flush_condition);
      pthread_mutex_lock(&omx_base_sink_Private->flush_mutex);
    }
    pthread_mutex_unlock(&omx_base_sink_Private->flush_mutex);

    /*No buffer to process. So wait here*/
    if((pInputSem[0]->semval==0 && isInputBufferNeeded[0]==OMX_TRUE ) && (pInputSem[1]->semval==0 && isInputBufferNeeded[1]==OMX_TRUE ) &&
      (omx_base_sink_Private->state != OMX_StateLoaded && omx_base_sink_Private->state != OMX_StateInvalid)) {
      DEBUG(DEB_LEV_SIMPLE_SEQ, "Waiting for input buffer \n");
      tsem_down(omx_base_sink_Private->bMgmtSem);
    }

    if(omx_base_sink_Private->state == OMX_StateLoaded || omx_base_sink_Private->state == OMX_StateInvalid) {
      DEBUG(DEB_LEV_FULL_SEQ, "In %s Buffer Management Thread is exiting\n",__func__);
      break;
    }

    DEBUG(DEB_LEV_SIMPLE_SEQ, "Waiting for input buffer semval=%d,%d \n",pInputSem[0]->semval, pInputSem[1]->semval);
	for (i=0; i < 2; i++) {
		if(pInputSem[i]->semval>0 && isInputBufferNeeded[i]==OMX_TRUE ) {
			tsem_down(pInputSem[i]);
			if(pInputQueue[i]->nelem>0){
				inBufExchanged[i]++;
				isInputBufferNeeded[i]=OMX_FALSE;
				pInputBuffer[i] = (OMX_BUFFERHEADERTYPE *)dxb_dequeue(pInputQueue[i]);
				if(pInputBuffer[i]== NULL){
					DEBUG(DEB_LEV_ERR, "Had NULL input buffer(%d)!!\n", i);
					goto omx_error_exit;
					//break;
				}
			}
		}
	}

	for (i=0; i < 2; i++) {
		if(isInputBufferNeeded[i]==OMX_FALSE) {
			if(pInputBuffer[i]->nFlags==OMX_BUFFERFLAG_EOS) {
				(*(omx_base_component_Private->callbacks->EventHandler))
					(openmaxStandComp,
					omx_base_component_Private->callbackData,
					OMX_EventBufferFlag, /* The command was completed */
					0, /* The commands was a OMX_CommandStateSet */
					pInputBuffer[i]->nFlags, /* The state has been changed in message->messageParam2 */
					NULL);
				pInputBuffer[i]->nFlags=0;
			}

			target_component=(OMX_COMPONENTTYPE*)pInputBuffer[i]->hMarkTargetComponent;
			if(target_component==(OMX_COMPONENTTYPE *)openmaxStandComp) {
				/*Clear the mark and generate an event*/
				(*(omx_base_component_Private->callbacks->EventHandler))
					(openmaxStandComp,
					omx_base_component_Private->callbackData,
					OMX_EventMark, /* The command was completed */
					1, /* The commands was a OMX_CommandStateSet */
					0, /* The state has been changed in message->messageParam2 */
					pInputBuffer[i]->pMarkData);
			} else if(pInputBuffer[i]->hMarkTargetComponent!=NULL){
				/*If this is not the target component then pass the mark*/
				DEBUG(DEB_LEV_FULL_SEQ, "Can't Pass Mark. This is a Sink!!\n");
			}

			if (omx_base_sink_Private->BufferMgmtCallback && pInputBuffer[i]->nFilledLen > 0) {
				if(omx_base_sink_Private->state == OMX_StateExecuting)
					(*(omx_base_sink_Private->BufferMgmtCallback))(openmaxStandComp, pInputBuffer[i]);
			}
			else {
				/*If no buffer management call back the explicitly consume input buffer*/
				pInputBuffer[i]->nFilledLen = 0;
			}
			/*Input Buffer has been completely consumed. So, get new input buffer*/

			if(omx_base_sink_Private->state==OMX_StatePause && !(PORT_IS_BEING_FLUSHED(pInPort[0]) || PORT_IS_BEING_FLUSHED(pInPort[1]))) {
				/*Waiting at paused state*/
				tsem_wait(omx_base_sink_Private->bStateSem);
			}

			/*Input Buffer has been completely consumed. So, return input buffer*/
			if(pInputBuffer[i]->nFilledLen==0) {
				pInPort[i]->ReturnBufferFunction(pInPort[i],pInputBuffer[i]);
				inBufExchanged[i]--;
				pInputBuffer[i]=NULL;
				isInputBufferNeeded[i] = OMX_TRUE;
			}
		}
	}
  }
omx_error_exit:
  DEBUG(DEB_LEV_SIMPLE_SEQ,"Exiting Buffer Management Thread\n");
  return NULL;
}


