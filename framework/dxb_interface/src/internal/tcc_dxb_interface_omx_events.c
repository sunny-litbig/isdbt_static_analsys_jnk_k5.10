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
#define LOG_TAG	"DXB_INTERFACE_OMX_EVENTS"
#include <utils/Log.h>

#include "globals.h"
#include <fcntl.h>
#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_Types.h>
#include <OMX_Audio.h>
#include "OMX_Other.h"

#include <user_debug_levels.h>
#include <OMX_TCC_Index.h>
#include <omx_base_component.h>
#include "omx_videodec_component.h"
#include "tcc_dxb_interface_omxil.h"
#include "tcc_dxb_interface_omx_events.h"

#include "tcc_dxb_interface_tuner.h"
#include "tcc_dxb_interface_demux.h"
#include "tcc_dxb_interface_audio.h"
#include "tcc_dxb_interface_video.h"

#ifdef      ENABLE_REMOTE_PLAYER
#include "tcc_dxb_interface_remoteplay.h"
#endif

OMX_ERRORTYPE dxbtunerEventHandler (OMX_OUT OMX_HANDLETYPE hComponent,
									OMX_OUT OMX_PTR pAppData,
									OMX_OUT OMX_EVENTTYPE eEvent,
									OMX_OUT OMX_U32 Data1, OMX_OUT OMX_U32 Data2, OMX_OUT OMX_PTR pEventData)
{
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = (Omx_Dxb_Dec_App_Private_Type*) pAppData;
	OMX_ERRORTYPE err = OMX_ErrorNone;

	//DEBUG (DEB_LEV_SIMPLE_SEQ, "in %s callback  eEvent = %x  Data1 = %lu  Data2 = %lu \n", __func__, eEvent, Data1, Data2);

	if (eEvent == OMX_EventCmdComplete)
	{
		if (Data1 == OMX_CommandStateSet)
		{
			DEBUG (DEB_LEV_SIMPLE_SEQ, "Tuner State changed in ");
			switch ((int) Data2)
			{
			case OMX_StateInvalid:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateInvalid\n");
				break;
			case OMX_StateLoaded:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateLoaded\n");
				break;
			case OMX_StateIdle:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateIdle\n");
				break;
			case OMX_StateExecuting:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateExecuting\n");
				break;
			case OMX_StatePause:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StatePause\n");
				break;
			case OMX_StateWaitForResources:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateWaitForResources\n");
				break;
			}
			tsem_up (OMX_Dxb_Dec_AppPriv->tunerEventSem);
		}
		else if (Data1 == OMX_CommandPortEnable)
		{
			DEBUG (DEB_LEV_SIMPLE_SEQ, "In %s Received Port Enable/Disable Event\n", __func__);
			tsem_up (OMX_Dxb_Dec_AppPriv->tunerEventSem);
		}
		else if (Data1 == OMX_CommandPortDisable)
		{
			DEBUG (DEB_LEV_SIMPLE_SEQ, "In %s Received Port Enable/Disable Event\n", __func__);
			tsem_up (OMX_Dxb_Dec_AppPriv->tunerEventSem);
		}
		else if (Data1 == OMX_CommandFlush)
		{
//			tsem_up (OMX_Dxb_Dec_AppPriv->tunerEventSem);
		}
	}
	else if (eEvent == OMX_EventDynamicResourcesAvailable)
	{
		switch (Data1)
		{
			case OMX_VENDOR_EVENTDATA_DEMUX:
			{
				TCC_DxB_DEMUX_SendEvent(OMX_Dxb_Dec_AppPriv->pvParent, DxB_DEMUX_EVENT_PACKET_STATUS_UPDATE, (void *)Data2);
				break;
			}

			default:
				break;
		}
	}
	else if(eEvent == OMX_EventVendorSpecificEvent)
	{
		switch(Data1)
		{
		case OMX_VENDOR_EVENTDATA_TUNER:
			{
				TCC_DxB_TUNER_SendEvent(OMX_Dxb_Dec_AppPriv->pvParent, Data2, pEventData);
			}
			break;
		default:
			break;
		}
	}

	return err;
}

OMX_ERRORTYPE dxbtunerEmptyBufferDone (OMX_OUT OMX_HANDLETYPE hComponent,
									   OMX_OUT OMX_PTR pAppData, OMX_OUT OMX_BUFFERHEADERTYPE * pBuffer)
{
	return OMX_ErrorNone;
}

OMX_ERRORTYPE dxbtunerFillBufferDone (OMX_OUT OMX_HANDLETYPE hComponent,
									  OMX_OUT OMX_PTR pAppData, OMX_OUT OMX_BUFFERHEADERTYPE * pBuffer)
{
	return OMX_ErrorNone;
}

OMX_ERRORTYPE dxbdemuxEventHandler (OMX_OUT OMX_HANDLETYPE hComponent,
									OMX_OUT OMX_PTR pAppData,
									OMX_OUT OMX_EVENTTYPE eEvent,
									OMX_OUT OMX_U32 Data1, OMX_OUT OMX_U32 Data2, OMX_OUT OMX_PTR pEventData)
{
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = (Omx_Dxb_Dec_App_Private_Type*) pAppData;
	OMX_ERRORTYPE err = OMX_ErrorNone;
	OMX_AUDIO_PARAM_PCMMODETYPE pcmParam;
	OMX_AUDIO_CONFIG_INFOTYPE parser_audio_info;
	int       i, total_num = 0;
	OMX_INDEXTYPE eExtentionIndex;

	//DEBUG (DEB_LEV_SIMPLE_SEQ, "in %s callback  eEvent = %x  Data1 = %x  Data2 = %x \n", __func__, eEvent, Data1, Data2);

	if (eEvent == OMX_EventCmdComplete)
	{
		if (Data1 == OMX_CommandStateSet)
		{
			DEBUG (DEB_LEV_SIMPLE_SEQ, "Demux State changed in ");

			switch ((int) Data2)
			{
			case OMX_StateInvalid:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateInvalid\n");
				break;

			case OMX_StateLoaded:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateLoaded\n");
				break;

			case OMX_StateIdle:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateIdle\n");
				break;

			case OMX_StateExecuting:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateExecuting\n");
				break;

			case OMX_StatePause:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StatePause\n");
				break;

			case OMX_StateWaitForResources:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateWaitForResources\n");
				break;
			}

			tsem_up (OMX_Dxb_Dec_AppPriv->demuxEventSem);

		}
		else if (Data1 == OMX_CommandPortEnable)
		{
			DEBUG (DEB_LEV_SIMPLE_SEQ, "In %s Received Port Enable/Disable Event\n", __func__);
			tsem_up (OMX_Dxb_Dec_AppPriv->demuxEventSem);
		}
		else if (Data1 == OMX_CommandPortDisable)
		{
			DEBUG (DEB_LEV_SIMPLE_SEQ, "In %s Received Port Enable/Disable Event\n", __func__);
			tsem_up (OMX_Dxb_Dec_AppPriv->demuxEventSem);
		}
		else if (Data1 == OMX_CommandFlush)
		{
//			tsem_up (OMX_Dxb_Dec_AppPriv->demuxEventSem);
		}
	}
	else if (eEvent == OMX_EventBufferFlag)
	{
		DEBUG (DEB_LEV_ERR, "In %s OMX_BUFFERFLAG_EOS\n", __func__);
		if ((int) Data2 == OMX_BUFFERFLAG_EOS)
		{

		}
	}
	else if (eEvent == OMX_EventDynamicResourcesAvailable)
	{
		switch (Data1)
		{
		case OMX_VENDOR_EVENTDATA_SET_AV_CODEC_TYPE:
		{
			TCC_DEMUX_ES_STREAM_TYPE *pAVCodecType;
			pAVCodecType = (TCC_DEMUX_ES_STREAM_TYPE *) pEventData;

			ALOGE("%s::AudioCodec[0x%X]VideoCodec[0x%X]", __func__, pAVCodecType->uiAudioCodecType, pAVCodecType->uiVideoCodecType);
			TCC_DxB_AUDIO_Start(OMX_Dxb_Dec_AppPriv->pvParent, 0, pAVCodecType->uiAudioCodecType);
			TCC_DxB_VIDEO_Start(OMX_Dxb_Dec_AppPriv->pvParent, 0, pAVCodecType->uiVideoCodecType);
		}
		break;
		case OMX_VENDOR_EVENTDATA_SET_PMT_PID:
		{
			TCC_DxB_DEMUX_SendEvent(OMX_Dxb_Dec_AppPriv->pvParent, DxB_DEMUX_EVENT_CAS_CHANNELCHANGE, (void *)pEventData);
		}
		break;
		case OMX_VENDOR_EVENTDATA_TUNER_RESYNC:
		{
			if(OMX_Dxb_Dec_AppPriv->tunerhandle)
				OMX_SetParameter(OMX_Dxb_Dec_AppPriv->tunerhandle,OMX_IndexVendorParamSetResync, (OMX_PTR)1);
		}
		break;
		case OMX_VENDOR_EVENTDATA_DEMUX:
		{
			TCC_DxB_DEMUX_SendEvent(OMX_Dxb_Dec_AppPriv->pvParent, DxB_DEMUX_EVENT_PACKET_STATUS_UPDATE, (void *)Data2);
		}
		break;
		default:
		{
		}
		break;
		}
	}
	else if(eEvent == OMX_EventVendorBufferingStart)
	{
		TCC_DxB_DEMUX_SendEvent(OMX_Dxb_Dec_AppPriv->pvParent, DxB_DEMUX_NOTIFY_FIRSTFRAME_DISPLAYED, (void *)Data1);
	}
	else if (eEvent == OMX_EventError)
	{
		switch (Data1)
		{
		case OMX_VENDOR_EVENTDATA_BUFFER_CONTROL:
			DEBUG (DEB_LEV_SIMPLE_SEQ, "%s :: Buffer Control(Level %lu) !!!\n", __func__, Data2);
			if(OMX_Dxb_Dec_AppPriv->videohandle)
				OMX_SetParameter(OMX_Dxb_Dec_AppPriv->videohandle, OMX_IndexParamVideoSkipFrames, (OMX_PTR)Data2);
			break;
		default:
			break;
		}
	}
	else if(eEvent == OMX_EventRecordingStop)
	{
		TCC_DxB_DEMUX_SendEvent(OMX_Dxb_Dec_AppPriv->pvParent, DxB_DEMUX_EVENT_RECORDING_STOP, (void *)Data1);
	}
	else if(eEvent == OMX_EventVendorSpecificEvent)
	{
		switch(Data1)
		{
		case OMX_VENDOR_EVENTDATA_DEMUX:
			{
				TCC_DxB_DEMUX_SendEvent(OMX_Dxb_Dec_AppPriv->pvParent, Data2, pEventData);
			}
			break;
		default:
			break;
		}
	}

	return err;
}

OMX_ERRORTYPE dxbdemuxEmptyBufferDone (OMX_OUT OMX_HANDLETYPE hComponent,
									   OMX_OUT OMX_PTR pAppData, OMX_OUT OMX_BUFFERHEADERTYPE * pBuffer)
{
	return OMX_ErrorNone;
}

OMX_ERRORTYPE dxbdemuxFillBufferDone (OMX_OUT OMX_HANDLETYPE hComponent,
									  OMX_OUT OMX_PTR pAppData, OMX_OUT OMX_BUFFERHEADERTYPE * pBuffer)
{
	return OMX_ErrorNone;
}

OMX_ERRORTYPE dxbaudioEventHandler (OMX_OUT OMX_HANDLETYPE hComponent,
									OMX_OUT OMX_PTR pAppData,
									OMX_OUT OMX_EVENTTYPE eEvent,
									OMX_OUT OMX_U32 Data1, OMX_OUT OMX_U32 Data2, OMX_OUT OMX_PTR pEventData)
{
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = (Omx_Dxb_Dec_App_Private_Type*) pAppData;
	OMX_ERRORTYPE err = OMX_ErrorNone;
	OMX_PARAM_PORTDEFINITIONTYPE param;
	OMX_AUDIO_PARAM_PCMMODETYPE pcmParam;

	DEBUG (DEB_LEV_SIMPLE_SEQ, "in %s callback  eEvent = %x  Data1 = %lu  Data2 = %lu \n", __func__, eEvent, Data1,
		   Data2);

	if (eEvent == OMX_EventCmdComplete)
	{
		if (Data1 == OMX_CommandStateSet)
		{
			DEBUG (DEB_LEV_SIMPLE_SEQ, "Audio State changed in ");

			switch ((int) Data2)
			{
			case OMX_StateInvalid:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateInvalid\n");
				break;

			case OMX_StateLoaded:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateLoaded\n");
				break;

			case OMX_StateIdle:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateIdle\n");
				break;

			case OMX_StateExecuting:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateExecuting\n");
				break;

			case OMX_StatePause:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StatePause\n");
				break;

			case OMX_StateWaitForResources:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateWaitForResources\n");
				break;
			}

			tsem_up (OMX_Dxb_Dec_AppPriv->audioEventSem);

		}
		else if (Data1 == OMX_CommandPortEnable)
		{
			DEBUG (DEB_LEV_SIMPLE_SEQ, "In %s Received Port Enable/Disable Event\n", __func__);
			tsem_up (OMX_Dxb_Dec_AppPriv->audioEventSem);
		}
		else if (Data1 == OMX_CommandPortDisable)
		{
			DEBUG (DEB_LEV_SIMPLE_SEQ, "In %s Received Port Enable/Disable Event\n", __func__);
			tsem_up (OMX_Dxb_Dec_AppPriv->audioEventSem);
		}
		else if (Data1 == OMX_CommandFlush)
		{
//			tsem_up (OMX_Dxb_Dec_AppPriv->audioEventSem);
		}
	}
	else if (eEvent == OMX_EventPortSettingsChanged)
	{
		if (Data2 == 2)
		{
			pcmParam.nPortIndex = 2;
			setHeader (&pcmParam, sizeof (OMX_AUDIO_PARAM_PCMMODETYPE));
			if(OMX_Dxb_Dec_AppPriv->audiohandle)
				err = OMX_GetParameter (OMX_Dxb_Dec_AppPriv->audiohandle, OMX_IndexParamAudioPcm, &pcmParam);

			// param setting..
#ifdef      ENABLE_REMOTE_PLAYER
	    	if(TCC_DxB_Get_Remote_Service_Status() == OMX_TRUE)
	            TCC_DxB_REMOTEPLAY_SetAudioInfo(pcmParam.nSamplingRate, pcmParam.nChannels);
#endif
			pcmParam.nPortIndex = 0;
		}
		else if (Data2 == 0)
		{
		}
		DEBUG (DEB_LEV_SIMPLE_SEQ, "\n port settings change event handler in %s \n", __func__);
	}
	else if (eEvent == OMX_EventBufferFlag)
	{
		DEBUG (DEB_LEV_ERR, "In %s OMX_BUFFERFLAG_EOS\n", __func__);
		if ((int) Data2 == OMX_BUFFERFLAG_EOS)
		{

		}
	}
	else if(eEvent == OMX_EventVendorCompenSTCDelay)
	{
		if(OMX_Dxb_Dec_AppPriv->demuxhandle)
			OMX_SetParameter (OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamSetTimeDelay, pEventData);
	}
	return err;
}

OMX_ERRORTYPE dxbaudioEmptyBufferDone (OMX_OUT OMX_HANDLETYPE hComponent,
									   OMX_OUT OMX_PTR pAppData, OMX_OUT OMX_BUFFERHEADERTYPE * pBuffer)
{
	return OMX_ErrorNone;
}

OMX_ERRORTYPE dxbaudioFillBufferDone (OMX_OUT OMX_HANDLETYPE hComponent,
									  OMX_OUT OMX_PTR pAppData, OMX_OUT OMX_BUFFERHEADERTYPE * pBuffer)
{
	return OMX_ErrorNone;
}

OMX_ERRORTYPE dxbvideoEventHandler (OMX_OUT OMX_HANDLETYPE hComponent,
									OMX_OUT OMX_PTR pAppData,
									OMX_OUT OMX_EVENTTYPE eEvent,
									OMX_OUT OMX_U32 Data1, OMX_OUT OMX_U32 Data2, OMX_OUT OMX_PTR pEventData)
{
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = (Omx_Dxb_Dec_App_Private_Type*) pAppData;
	OMX_ERRORTYPE err;
	OMX_PARAM_PORTDEFINITIONTYPE param;

	//DEBUG(DEB_LEV_SIMPLE_SEQ, "in %s callback  eEvent = %x  Data1 = %x  Data2 = %x \n", __func__ , eEvent, Data1, Data2);

	if (eEvent == OMX_EventCmdComplete)
	{
		if (Data1 == OMX_CommandStateSet)
		{
			DEBUG (DEB_LEV_SIMPLE_SEQ /*SIMPLE_SEQ */ , "Video Decoder State changed in ");
			switch ((int) Data2)
			{
			case OMX_StateInvalid:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateInvalid\n");
				break;
			case OMX_StateLoaded:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateLoaded\n");
				break;
			case OMX_StateIdle:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateIdle\n");
				break;
			case OMX_StateExecuting:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateExecuting\n");
				break;
			case OMX_StatePause:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StatePause\n");
				break;
			case OMX_StateWaitForResources:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateWaitForResources\n");
				break;
			}

			tsem_up (OMX_Dxb_Dec_AppPriv->videoEventSem);
		}
		else if (Data1 == OMX_CommandPortEnable)
		{
			DEBUG (DEB_LEV_SIMPLE_SEQ, "In %s Received Port Enable  Event\n", __func__);
			tsem_up (OMX_Dxb_Dec_AppPriv->videoEventSem);
		}
		else if (Data1 == OMX_CommandPortDisable)
		{
			DEBUG (DEB_LEV_SIMPLE_SEQ, "In %s Received Port Disable Event\n", __func__);
			tsem_up (OMX_Dxb_Dec_AppPriv->videoEventSem);
		}
		else if (Data1 == OMX_CommandFlush)
		{
			DEBUG (DEB_LEV_SIMPLE_SEQ, "In %s Received Port Flush Event\n", __func__);
//			tsem_up (OMX_Dxb_Dec_AppPriv->videoEventSem);
		}
	}
	else if (eEvent == OMX_EventVendorSetSTCDelay)
	{
		if(OMX_Dxb_Dec_AppPriv->demuxhandle)
			err = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorSetSTCDelay, pEventData);
		if(OMX_Dxb_Dec_AppPriv->fbdevhandle)
			err = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_IndexVendorSetSTCDelay, pEventData);
		if(OMX_Dxb_Dec_AppPriv->alsasinkhandle)
			err = OMX_SetParameter (OMX_Dxb_Dec_AppPriv->alsasinkhandle, OMX_IndexVendorSetSTCDelay, pEventData);
    }
	else if (eEvent == OMX_EventPortSettingsChanged)
	{
		if(Data2 == OMX_IndexConfigCommonOutputCrop)
		{
			if(OMX_Dxb_Dec_AppPriv->fbdevhandle)
				err = OMX_SetConfig (OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_IndexConfigCommonOutputCrop, (OMX_PTR)pEventData);
		}
		else
		{
			OMX_BOOL bPIXEL_FORMAT_YV12 = OMX_FALSE;
			omx_base_PortType *pvideo_port = (omx_base_PortType *)pEventData;
			OMX_VIDEO_PORTDEFINITIONTYPE *pvideo_port_param = &pvideo_port->sPortParam.format.video;
			/*format : 0:YUV Interleaved  1:YUV Seperlated
			* flag :   0:Interlaced frame 1:Progressive frame
			*/
			int is_progressive;
			/*Be carefull!!!
			* You must set AspectRatio first.
			* Otherwise AspectRatio doesn't work
			*/
			OMX_RESOLUTIONTYPE resolution_param;

			DEBUG (DEB_LEV_SIMPLE_SEQ, "In %s Received Port Settings Changed Event\n", __func__);

			setHeader ((OMX_PTR)&resolution_param, sizeof(OMX_RESOLUTIONTYPE));
			resolution_param.nPortIndex = pvideo_port->nTunneledPort;
			resolution_param.nWidth = pvideo_port_param->nFrameWidth;
			resolution_param.nHeight = pvideo_port_param->nFrameHeight;
			resolution_param.nFrameRate = pvideo_port_param->xFramerate;
			if(OMX_Dxb_Dec_AppPriv->fbdevhandle)	
				err = OMX_SetConfig (OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_IndexVendorChangeResolution, (OMX_PTR)&resolution_param);
		}
	}
	else if (eEvent == OMX_EventVendorVideoChange)
	{
#ifdef      ENABLE_REMOTE_PLAYER
		omx_base_PortType *pvideo_port = (omx_base_PortType *)pEventData;
		OMX_VIDEO_PORTDEFINITIONTYPE *pvideo_port_param = &pvideo_port->sPortParam.format.video;

		if(TCC_DxB_Get_Remote_Service_Status() == OMX_TRUE)
	        TCC_DxB_REMOTEPLAY_SetVideoInfo(pvideo_port_param->nFrameWidth, pvideo_port_param->nFrameHeight, pvideo_port_param->xFramerate);
#endif
	}
	else if (eEvent == OMX_EventBufferFlag)
	{
		DEBUG (DEB_LEV_SIMPLE_SEQ, "In %s OMX_BUFFERFLAG_EOS\n", __func__);
		if ((int) Data2 == OMX_BUFFERFLAG_EOS)
		{
		}
	}
	else if (eEvent == OMX_EventVendorClearVPUDisplayIndex)
	{
		int       displayindex = *(int *) pEventData;
		if(OMX_Dxb_Dec_AppPriv->fbdevhandle)
			OMX_SetConfig (OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_IndexVendorQueueVpuDisplayIndex, &displayindex);
	}
	else if (eEvent == OMX_EventVendorVideoUserDataAvailable)
	{
		if ((int) Data1 == OMX_BUFFERFLAG_EXTRADATA)
			TCC_DxB_VIDEO_SendEvent(OMX_Dxb_Dec_AppPriv->pvParent, DxB_VIDEO_NOTIFY_VIDEO_DEFINITION_UPDATE, (void *)pEventData);
		else if((int)Data1 == 2)	// for ATSC User Data
			TCC_DxB_VIDEO_SendEvent(OMX_Dxb_Dec_AppPriv->pvParent, DxB_VIDEO_NOTIFY_USER_DATA_AVAILABLE, (void *)pEventData);
	}
	else if(eEvent == OMX_EventVendorBufferingStart)
	{
		TCC_DxB_VIDEO_SendEvent(OMX_Dxb_Dec_AppPriv->pvParent, DxB_VIDEO_NOTIFY_FIRSTFRAME_DISPLAYED, (void *)Data1);
	}
	else if(eEvent == OMX_EventVendorNotifyStartTimeSyncWithVideo)
	{
		if(OMX_Dxb_Dec_AppPriv->alsasinkhandle)
			OMX_SetParameter(OMX_Dxb_Dec_AppPriv->alsasinkhandle, OMX_IndexVendorNotifyStartTimeSyncWithVideo, (OMX_PTR)1);
	}
	else if (eEvent == OMX_EventVendorVideoFirstDisplay)
	{
		TCC_DxB_DEMUX_SetFirstDsiplaySet(OMX_Dxb_Dec_AppPriv->pvParent, 0, 1);
	}
	else if (eEvent == OMX_EventVendorStreamRollBack)
	{
		TCC_DxB_AUDIO_StreamRollBack(OMX_Dxb_Dec_AppPriv->pvParent, 0);
	}
	else if (eEvent == OMX_EventVendorVideoError)
	{
		TCC_DxB_VIDEO_SendEvent(OMX_Dxb_Dec_AppPriv->pvParent, DxB_VIDEO_NOTIFY_VIDEO_ERROR, (void *)Data1);
	}
	else if (eEvent == OMX_EventError)
	{
		switch (Data1)
		{
		case OMX_ErrorHardware:
			DEBUG (DEB_LEV_SIMPLE_SEQ, "Video H/W Error\n");
			TCC_DxB_DEMUX_ResetVideo(OMX_Dxb_Dec_AppPriv->pvParent, 0);
			break;
		default:
			break;
		}
	}

	return OMX_ErrorNone;
}

OMX_ERRORTYPE dxbvideoEmptyBufferDone (OMX_OUT OMX_HANDLETYPE hComponent,
									   OMX_OUT OMX_PTR pAppData, OMX_OUT OMX_BUFFERHEADERTYPE * pBuffer)
{
	return OMX_ErrorNone;
}

OMX_ERRORTYPE dxbvideoFillBufferDone (OMX_OUT OMX_HANDLETYPE hComponent,
									  OMX_OUT OMX_PTR pAppData, OMX_OUT OMX_BUFFERHEADERTYPE * pBuffer)
{
	return OMX_ErrorNone;
}

OMX_ERRORTYPE dxbalsasinkEventHandler (OMX_OUT OMX_HANDLETYPE hComponent,
									   OMX_OUT OMX_PTR pAppData,
									   OMX_OUT OMX_EVENTTYPE eEvent,
									   OMX_OUT OMX_U32 Data1, OMX_OUT OMX_U32 Data2, OMX_OUT OMX_PTR pEventData)
{
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = (Omx_Dxb_Dec_App_Private_Type*) pAppData;

	//DEBUG (DEB_LEV_SIMPLE_SEQ, "in %s callback  eEvent = %x  Data1 = %x  Data2 = %x \n", __func__, eEvent, Data1,  Data2);

	if (eEvent == OMX_EventCmdComplete)
	{
		if (Data1 == OMX_CommandStateSet)
		{
			DEBUG (DEB_LEV_SIMPLE_SEQ, "Audio Sink State changed in ");
			switch ((int) Data2)
			{
			case OMX_StateInvalid:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateInvalid\n");
				break;
			case OMX_StateLoaded:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateLoaded\n");
				break;
			case OMX_StateIdle:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateIdle\n");
				break;
			case OMX_StateExecuting:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateExecuting\n");
				break;
			case OMX_StatePause:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StatePause\n");
				break;
			case OMX_StateWaitForResources:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateWaitForResources\n");
				break;
			}
			tsem_up (OMX_Dxb_Dec_AppPriv->alsasinkEventSem);
		}
		else if (Data1 == OMX_CommandPortEnable)
		{
			DEBUG (DEB_LEV_SIMPLE_SEQ, "In %s Received Port Enable  Event\n", __func__);
			tsem_up (OMX_Dxb_Dec_AppPriv->alsasinkEventSem);
		}
		else if (Data1 == OMX_CommandPortDisable)
		{
			DEBUG (DEB_LEV_SIMPLE_SEQ, "In %s Received Port Disable Event\n", __func__);
			tsem_up (OMX_Dxb_Dec_AppPriv->alsasinkEventSem);
		}
		else if (Data1 == OMX_CommandFlush)
		{
			DEBUG (DEB_LEV_SIMPLE_SEQ, "In %s Received Port Flush Event\n", __func__);
//			tsem_up (OMX_Dxb_Dec_AppPriv->alsasinkEventSem);
		}
	}
	else if (eEvent == OMX_EventBufferFlag)
	{
		DEBUG (DEB_LEV_ERR, "In %s OMX_BUFFERFLAG_EOS\n", __func__);
		if ((int) Data2 == OMX_BUFFERFLAG_EOS)
		{

		}
	}
	else if (eEvent == OMX_EventDynamicResourcesAvailable)
	{
#ifdef      ENABLE_REMOTE_PLAYER
		if(TCC_DxB_Get_Remote_Service_Status() == OMX_TRUE)
		{
			TCC_DxB_REMOTEPLAY_WriteAudio(pEventData,Data1, Data2);
			return OMX_ErrorNone;
		}
#endif
		return OMX_ErrorInvalidState;
	}
	else if(eEvent == OMX_EventVendorSetSTCDelay)
	{
		if(OMX_Dxb_Dec_AppPriv->demuxhandle)
		OMX_SetParameter (OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorSetSTCDelay, pEventData);
		if(OMX_Dxb_Dec_AppPriv->fbdevhandle)
		OMX_SetParameter (OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_IndexVendorSetSTCDelay, pEventData);
	}

	return OMX_ErrorNone;
}

OMX_ERRORTYPE dxbalsasinkEmptyBufferDone (OMX_OUT OMX_HANDLETYPE hComponent,
										  OMX_OUT OMX_PTR pAppData, OMX_OUT OMX_BUFFERHEADERTYPE * pBuffer)
{
	return OMX_ErrorNone;
}

OMX_ERRORTYPE dxbrecsinkEventHandler (OMX_OUT OMX_HANDLETYPE hComponent,
									   OMX_OUT OMX_PTR pAppData,
									   OMX_OUT OMX_EVENTTYPE eEvent,
									   OMX_OUT OMX_U32 Data1, OMX_OUT OMX_U32 Data2, OMX_OUT OMX_PTR pEventData)
{
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = (Omx_Dxb_Dec_App_Private_Type*) pAppData;

	//DEBUG (DEB_LEV_SIMPLE_SEQ, "in %s callback  eEvent = %x  Data1 = %x  Data2 = %x \n", __func__, eEvent, Data1,  Data2);

	if (eEvent == OMX_EventCmdComplete)
	{
		if (Data1 == OMX_CommandStateSet)
		{
			DEBUG (DEB_LEV_SIMPLE_SEQ, "Rec Sink State changed in ");
			switch ((int) Data2)
			{
			case OMX_StateInvalid:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateInvalid\n");
				break;
			case OMX_StateLoaded:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateLoaded\n");
				break;
			case OMX_StateIdle:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateIdle\n");
				break;
			case OMX_StateExecuting:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateExecuting\n");
				break;
			case OMX_StatePause:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StatePause\n");
				break;
			case OMX_StateWaitForResources:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateWaitForResources\n");
				break;
			}
		}
		else if (Data1 == OMX_CommandPortEnable)
		{
			DEBUG (DEB_LEV_SIMPLE_SEQ, "In %s Received Port Enable  Event\n", __func__);
		}
		else if (Data1 == OMX_CommandPortDisable)
		{
			DEBUG (DEB_LEV_SIMPLE_SEQ, "In %s Received Port Disable Event\n", __func__);
		}
		else if (Data1 == OMX_CommandFlush)
		{
			DEBUG (DEB_LEV_SIMPLE_SEQ, "In %s Received Port Flush Event\n", __func__);
		}
	}
	else if (eEvent == OMX_EventBufferFlag)
	{
		DEBUG (DEB_LEV_ERR, "In %s OMX_BUFFERFLAG_EOS\n", __func__);
		if ((int) Data2 == OMX_BUFFERFLAG_EOS)
		{

		}
	}
	else if (eEvent == OMX_EventDynamicResourcesAvailable)
	{
	}
	else if(eEvent == OMX_EventVendorSpecificEvent)
	{
		/*TO-DO*/
	}

	return OMX_ErrorNone;
}

OMX_ERRORTYPE dxbrecsinkEmptyBufferDone (OMX_OUT OMX_HANDLETYPE hComponent,
										  OMX_OUT OMX_PTR pAppData, OMX_OUT OMX_BUFFERHEADERTYPE * pBuffer)
{
	return OMX_ErrorNone;
}

OMX_ERRORTYPE dxbfbdevEventHandler (OMX_OUT OMX_HANDLETYPE hComponent,
									OMX_OUT OMX_PTR pAppData,
									OMX_OUT OMX_EVENTTYPE eEvent,
									OMX_OUT OMX_U32 Data1, OMX_OUT OMX_U32 Data2, OMX_OUT OMX_PTR pEventData)
{
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = (Omx_Dxb_Dec_App_Private_Type*) pAppData;

	//DEBUG(DEB_LEV_SIMPLE_SEQ, "in %s callback  eEvent = %x  Data1 = %x  Data2 = %x \n", __func__ , eEvent, Data1, Data2);

	if (eEvent == OMX_EventCmdComplete)
	{
		if (Data1 == OMX_CommandStateSet)
		{
			DEBUG (DEB_LEV_SIMPLE_SEQ, "Fbdev State changed in ");
			switch ((int) Data2)
			{
			case OMX_StateInvalid:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateInvalid\n");
				break;
			case OMX_StateLoaded:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateLoaded\n");
				break;
			case OMX_StateIdle:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateIdle ---- fbsink\n");
				break;
			case OMX_StateExecuting:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateExecuting\n");
				break;
			case OMX_StatePause:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StatePause\n");
				break;
			case OMX_StateWaitForResources:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateWaitForResources\n");
				break;
			}
			tsem_up (OMX_Dxb_Dec_AppPriv->fbdevEventSem);
		}
		else if ((Data1 == OMX_CommandPortEnable) || (Data1 == OMX_CommandPortDisable))
		{
			DEBUG (DEB_LEV_SIMPLE_SEQ, "In %s Received Port Enable/Disable Event\n", __func__);
			tsem_up (OMX_Dxb_Dec_AppPriv->fbdevEventSem);
		}
		else if (Data1 == OMX_CommandFlush)
		{
			DEBUG (DEB_LEV_SIMPLE_SEQ, "In %s Received Port Flush Event\n", __func__);
//			tsem_up (OMX_Dxb_Dec_AppPriv->fbdevEventSem);
		}
	}
	else if (eEvent == OMX_EventBufferFlag)
	{
		DEBUG (DEB_LEV_ERR, "In %s OMX_BUFFERFLAG_EOS\n", __func__);
		if ((int) Data2 == OMX_BUFFERFLAG_EOS)
		{

		}
	}
	else if (eEvent == OMX_EventDynamicResourcesAvailable)
	{
#ifdef      ENABLE_REMOTE_PLAYER
		if(TCC_DxB_Get_Remote_Service_Status() == OMX_TRUE)
        {
	        TCC_DxB_REMOTEPLAY_WriteVideo(pEventData, sizeof(int)*3, ((int *)pEventData)[5], (Data2==0) ? 1:0);
        }
#endif
	}
	else if (eEvent == OMX_EventVendorClearVPUDisplayIndex)
	{
		int       displayindex = *(int *) pEventData;
		if(OMX_Dxb_Dec_AppPriv->videohandle)
			OMX_SetConfig (OMX_Dxb_Dec_AppPriv->videohandle, OMX_IndexVendorQueueVpuDisplayIndex, &displayindex);
	}
	else if(eEvent == OMX_EventVendorSubtitleEvent)
	{
	#ifdef ENABLE_REMOTE_PLAYER
		if(TCC_DxB_Get_Remote_Service_Status() == OMX_TRUE)
			TCC_DxB_REMOTEPLAY_WriteSubtitle(pEventData, 0);
	#endif
	}

	return OMX_ErrorNone;
}

OMX_ERRORTYPE dxbfbdevEmptyBufferDone (OMX_OUT OMX_HANDLETYPE hComponent,
									   OMX_OUT OMX_PTR pAppData, OMX_OUT OMX_BUFFERHEADERTYPE * pBuffer)
{
	return OMX_ErrorNone;
}


OMX_ERRORTYPE dxbpvrEventHandler (OMX_OUT OMX_HANDLETYPE hComponent,
									   OMX_OUT OMX_PTR pAppData,
									   OMX_OUT OMX_EVENTTYPE eEvent,
									   OMX_OUT OMX_U32 Data1, OMX_OUT OMX_U32 Data2, OMX_OUT OMX_PTR pEventData)
{
	Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv = (Omx_Dxb_Dec_App_Private_Type*) pAppData;

	//DEBUG (DEB_LEV_SIMPLE_SEQ, "in %s callback  eEvent = %x  Data1 = %x  Data2 = %x \n", __func__, eEvent, Data1,  Data2);

	if (eEvent == OMX_EventCmdComplete)
	{
		if (Data1 == OMX_CommandStateSet)
		{
			DEBUG (DEB_LEV_SIMPLE_SEQ, "PVR State changed in ");
			switch ((int) Data2)
			{
			case OMX_StateInvalid:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateInvalid\n");
				break;
			case OMX_StateLoaded:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateLoaded\n");
				break;
			case OMX_StateIdle:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateIdle\n");
				break;
			case OMX_StateExecuting:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateExecuting\n");
				break;
			case OMX_StatePause:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StatePause\n");
				break;
			case OMX_StateWaitForResources:
				DEBUG (DEB_LEV_SIMPLE_SEQ, "OMX_StateWaitForResources\n");
				break;
			}
			tsem_up (OMX_Dxb_Dec_AppPriv->pvrEventSem);
		}
		else if (Data1 == OMX_CommandPortEnable)
		{
			DEBUG (DEB_LEV_SIMPLE_SEQ, "In %s Received Port Enable  Event\n", __func__);
			tsem_up (OMX_Dxb_Dec_AppPriv->pvrEventSem);
		}
		else if (Data1 == OMX_CommandPortDisable)
		{
			DEBUG (DEB_LEV_SIMPLE_SEQ, "In %s Received Port Disable Event\n", __func__);
			tsem_up (OMX_Dxb_Dec_AppPriv->pvrEventSem);
		}
		else if (Data1 == OMX_CommandFlush)
		{
			DEBUG (DEB_LEV_SIMPLE_SEQ, "In %s Received Port Flush Event\n", __func__);
//			tsem_up (OMX_Dxb_Dec_AppPriv->pvrEventSem);
		}
	}
	else if(eEvent == OMX_EventGetVideoParser)
	{
		if(OMX_Dxb_Dec_AppPriv->demuxhandle)
			OMX_GetParameter (OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_IndexVendorParamDxBVideoParser, pEventData);
	}
	else if(eEvent == OMX_EventSetPID)
	{
		TCC_DxB_TUNER_RegisterPID(OMX_Dxb_Dec_AppPriv->pvParent, 0, (UINT32)Data1);
	}

	return OMX_ErrorNone;
}

OMX_ERRORTYPE dxbpvrEmptyBufferDone (OMX_OUT OMX_HANDLETYPE hComponent,
										  OMX_OUT OMX_PTR pAppData, OMX_OUT OMX_BUFFERHEADERTYPE * pBuffer)
{
	return OMX_ErrorNone;
}

OMX_CALLBACKTYPE dxbtunercallbacks = {
	.EventHandler = dxbtunerEventHandler,
	.EmptyBufferDone = dxbtunerEmptyBufferDone,
	.FillBufferDone = dxbtunerFillBufferDone
};

OMX_CALLBACKTYPE dxbdemuxcallbacks = {
	.EventHandler = dxbdemuxEventHandler,
	.EmptyBufferDone = dxbdemuxEmptyBufferDone,
	.FillBufferDone = dxbdemuxFillBufferDone
};

OMX_CALLBACKTYPE dxbaudiocallbacks = {
	.EventHandler = dxbaudioEventHandler,
	.EmptyBufferDone = dxbaudioEmptyBufferDone,
	.FillBufferDone = dxbaudioFillBufferDone
};

OMX_CALLBACKTYPE dxbvideocallbacks = {
	.EventHandler = dxbvideoEventHandler,
	.EmptyBufferDone = dxbvideoEmptyBufferDone,
	.FillBufferDone = dxbvideoFillBufferDone
};

OMX_CALLBACKTYPE dxbalsasinkcallbacks = {
	.EventHandler = dxbalsasinkEventHandler,
	.EmptyBufferDone = dxbalsasinkEmptyBufferDone,
	.FillBufferDone = NULL
};

OMX_CALLBACKTYPE dxbrecsinkcallbacks = {
	.EventHandler = dxbrecsinkEventHandler,
	.EmptyBufferDone = dxbrecsinkEmptyBufferDone,
	.FillBufferDone = NULL
};

OMX_CALLBACKTYPE dxbfbdevcallbacks = {
	.EventHandler = dxbfbdevEventHandler,
	.EmptyBufferDone = dxbfbdevEmptyBufferDone,
	.FillBufferDone = NULL
};

OMX_CALLBACKTYPE dxbpvrcallbacks = {
	.EventHandler = dxbpvrEventHandler,
	.EmptyBufferDone = dxbpvrEmptyBufferDone,
	.FillBufferDone = NULL
};

