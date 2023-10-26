/**

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


*   Description : omx_videodec_component.c


*******************************************************************************/ 
#include <omxcore.h>
#include <omx_base_video_port.h>
#include <omx_videodec_component.h>
#include <OMX_Video.h>

#include <TCC_VPU_CODEC_EX.h>
#include <vdec.h>
#include <cdk.h>
#include <video_user_data.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <errno.h>

#ifndef uint32_t
typedef unsigned int uint32_t;
#endif

#include <tccfb_ioctrl.h>

#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
#include <tcc_vsync_ioctl.h>
#endif 

#define LOG_TAG	"OMX_TCC_VIDEO_DEC"
#include <utils/Log.h>
#include <cutils/properties.h>

#include "tcc_dxb_interface_demux.h"
#include "omx_tcc_thread.h"

#include "tcc_dxb_timecheck.h"

#if 1
/* This is not ready for tcc93xx.
 * Later We will use this option.
 */
#define		SUPPORT_SEARCH_IFRAME_ATSTARTING
#endif

//#define  SUPPORT_SAVE_OUTPUTBUFFER //Save output to files
#ifdef	SUPPORT_SAVE_OUTPUTBUFFER
static int giDumpVideoFileIndex = 0;
#endif

#define		BUFFER_INDICATOR	   "TDXB"
#define		BUFFER_INDICATOR_SIZE  4

#define		VIDEO_BUFFERCOUNT_MIN		20
#define		VIDEO_BUFFERCOUNT_ACTUAL	20

static int DEBUG_ON = 0;
#define DBUG_MSG(msg...)	if (DEBUG_ON) { ALOGD( ": " msg);/* sleep(1);*/}
#define DPRINTF_DEC(msg...)	//ALOGI( ": " msg);
#define DPRINTF_DEC_STATUS(msg...)	//ALOGD( ": " msg);

#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
#define MARK_BUFF_NOT_USED	0xFFFFFFFF
#define MAX_CHECK_COUNT_FOR_CLEAR	200
#endif

#define VSYNC_DEVICE      "/dev/tcc_vsync0"
#define UNIQUE_ID_OF_FIRST_FRAME 1

struct _IsdbtProprietaryData {
	int channel_index;
	int dummy;
};

static cdk_func_t *gspfVDecList[VCODEC_MAX] = {
	dxb_vdec_vpu,	//STD_AVC
	dxb_vdec_vpu,	//STD_VC1
	dxb_vdec_vpu,	//STD_MPEG2
	dxb_vdec_vpu,	//STD_MPEG4
	dxb_vdec_vpu,	//STD_H263
	dxb_vdec_vpu,	//STD_DIV3
	dxb_vdec_vpu,	//STD_RV
	dxb_vdec_vpu,	//
	0,			//STD_WMV78
	0			//STD_SORENSON263
};

/** The output decoded color format */
#define OUTPUT_DECODED_COLOR_FMT OMX_COLOR_FormatYUV420Planar

#ifdef VIDEO_USER_DATA_PROCESS
	#define FLAG_ENABLE_USER_DATA 1
#else
	#define FLAG_ENABLE_USER_DATA 0
#endif

#define VIDEO_PICTYPE_MASK  0x7FFFFFFF

#define MAX_DECODE_INSTANCE_ERROR_CHCEK_CNT	300

#define VPU_FIELDPICTURE_PENDTIME_MAX	2000	//ms

#define MAX_INSTANCE_CNT			2

typedef struct
{
	pthread_t VpuBufferClearhandle;
	int 	VpuBufferClearThreadRunning;
}t_Vpu_BufferClearInfo;

typedef struct
{
	unsigned int	iDecoderID;
	unsigned int	buffer_unique_id;
	int			ret;
	int			iStreamID;
	int			bVSyncEnabled;
}t_Vpu_BufferCearFrameInfo;

typedef struct
{
	unsigned int			iDecoderID;
	OMX_COMPONENTTYPE * openmaxStandComp;
}t_Vpu_ClearThreadArg;

typedef struct
{
	int uiPreVideo_DecoderID;
	int uiPreVideo_DecoderID_ChkCnt;
}delete_vpu_displayed_info_t;

//////////////////////////////////////////////////////////////////////////////////////
// state flags
#define STATE_SKIP_OUTPUT_B_FRAME           0x00000001

#define SET_FLAG(__p_vpu_private, __flag)    ((__p_vpu_private).ulFlags |= (__flag))
#define CLEAR_FLAG(__p_vpu_private, __flag)  ((__p_vpu_private).ulFlags &= ~(__flag))
#define CHECK_FLAG(__p_vpu_private, __flag)  ((__p_vpu_private).ulFlags & (__flag))
//////////////////////////////////////////////////////////////////////////////////////

/*Video Error*/
#define VDEC_INIT_ERROR 1
#define VDEC_SEQ_HEADER_ERROR 2
#define VDEC_DECODE_ERROR 3
#define VDEC_BUFFER_FULL_ERROR 4
#define VDEC_BUFFER_CLEAR_ERROR 5
#define VDEC_NO_OUTPUT_ERROR 6
static int vdec_init_error_flag = 0;
static int vdec_seq_header_error_flag = 0;
static int vdec_decode_error_flag = 0;
static int vdec_buffer_full_error_flag = 0;
static int vdec_buffer_clear_error_flag = 0;
static int vdec_no_input_error_flag = 0;

void* dxb_omx_video_twoport_filter_component_BufferMgmtFunction (void* param);
typedef long long (*pfnDemuxGetSTC)(int itype, long long lpts, unsigned int index, int log, void *pvApp);

typedef struct {
	int                         iDeviceIndex;
	struct _IsdbtProprietaryData stIsdbtProprietaryData;
	int                         mFBdevice; // = -1;
	OMX_BOOL                    bChangeDisplayID; // = OMX_FALSE;
	OMX_BOOL                    isRenderer; // = OMX_TRUE;
	OMX_BOOL                    isRefFrameBroken; // = OMX_FALSE;
	pfnDemuxGetSTC              gfnDemuxGetSTCValue; // = NULL
	void*                       pvDemuxApp;
	t_Vpu_BufferClearInfo       Vpu_BufferClearInfo[MAX_INSTANCE_CNT];
	t_Vpu_ClearThreadArg        Vpu_ClearThreadArg[MAX_INSTANCE_CNT];
	delete_vpu_displayed_info_t delete_vpu_displayed_info;
#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
	OMX_BOOL                    bRollback; // = OMX_FALSE;
#endif
} TCC_VDEC_PRIVATE;

extern void * dxb_vdec_alloc_instance(void);

static unsigned int dbg_get_time(void)
{
	struct timespec	time;
	unsigned int cur_time;

	clock_gettime(CLOCK_MONOTONIC , &time);
	cur_time = ((time.tv_sec * 1000) & 0x00ffffff) + (time.tv_nsec / 1000000);

	return cur_time;
}

static int getHWCID()
{
	char value[PROPERTY_VALUE_MAX];
	property_get("tcc.video.hwr.id", value, "");
	return atoi(value);
}

static int SendFirstFrameEvent(OMX_COMPONENTTYPE * openmaxStandComp, _VIDEO_DECOD_INSTANCE_ *pVDecInst)
{
	omx_videodec_component_PrivateType* omx_private = openmaxStandComp->pComponentPrivate;
	TCC_VDEC_PRIVATE *pPrivateData = (TCC_VDEC_PRIVATE*) omx_private->pPrivateData;
	OMX_U32 i;

	for (i = 0; i < MAX_INSTANCE_CNT; i++)
	{
		if (omx_private->pVideoDecodInstance[i].nDisplayedFirstFrame > 0)
		{
			return -1;
		}
	}

	pVDecInst->nDisplayedFirstFrame++;

	(*(omx_private->callbacks->EventHandler)) (openmaxStandComp,
						omx_private->callbackData,
						OMX_EventVendorNotifyStartTimeSyncWithVideo,
						0, 0, 0);

	(*(omx_private->callbacks->EventHandler)) (openmaxStandComp,
						omx_private->callbackData,
						OMX_EventVendorBufferingStart,
						pPrivateData->stIsdbtProprietaryData.channel_index, 0, 0);

	tcc_dxb_timecheck_set("switch_channel", "FVideo_out", TIMECHECK_STOP);

	return 0;
}

static int SendVideoErrorEvent(OMX_COMPONENTTYPE * openmaxStandComp, int error_type)
{
	omx_videodec_component_PrivateType* omx_private = openmaxStandComp->pComponentPrivate;

	(*(omx_private->callbacks->EventHandler)) (openmaxStandComp,
						omx_private->callbackData,
						OMX_EventVendorVideoError,
						error_type, 0, 0);

	return 0;
}

static int isVsyncEnabled(omx_videodec_component_PrivateType *omx_private, unsigned int uiDecoderID)
{
#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
	TCC_VDEC_PRIVATE *pPrivateData = (TCC_VDEC_PRIVATE*) omx_private->pPrivateData;
	if (*((int*)omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance) == 0
	|| *((int*)omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance) == 1)
	{
		return 1;
	}
#endif
	return 0;
}

#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
static int tcc_vsync_command(omx_videodec_component_PrivateType *omx_private, int arg1, int arg2)
{
	TCC_VDEC_PRIVATE *pPrivateData = (TCC_VDEC_PRIVATE*) omx_private->pPrivateData;
	int ret = -2;

	if(pPrivateData->mFBdevice >= 0)
	{
		if (arg1 & 0x80000000) {
			struct stTcc_last_frame lastframe_info;

			if( omx_private->Resolution_Change )
				lastframe_info.reason = LASTFRAME_FOR_RESOLUTION_CHANGE;
			else
				lastframe_info.reason = LASTFRAME_FOR_CODEC_CHANGE;
			ret = ioctl(pPrivateData->mFBdevice, arg1 & 0x7FFFFFFF, &lastframe_info);
		} else {
			ret = ioctl(pPrivateData->mFBdevice, arg1, arg2);
		}
		//ALOGE("%s:%d:%d:%d",__func__, arg1, command2, ret);
	}
	return ret;
}
#endif  

#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
static int clear_front_vpu_buffer(OMX_COMPONENTTYPE *openmaxStandComp, _VIDEO_DECOD_INSTANCE_ *pVDecInst)
{
	int ret = 0;
	int nDevID = pVDecInst->Display_index[pVDecInst->out_index] >> 16;
	int nClearDispIndex = pVDecInst->Display_index[pVDecInst->out_index] & 0xFFFF;

	pVDecInst->Display_index[pVDecInst->out_index] = 0;

	if(nClearDispIndex == 0){
		pVDecInst->out_index = (pVDecInst->out_index + 1) % pVDecInst->max_fifo_cnt;
		if (pVDecInst->used_fifo_count > 0)
		pVDecInst->used_fifo_count--;
		return 0;
	}

	//nClearDispIndex++; //CAUTION !!! to avoid NULL(0) data insertion.
	if(dxb_queue_ex(pVDecInst->pVPUDisplayedIndexQueue, (void *)nClearDispIndex) == 0 )
	{
		//DPRINTF_DEC_STATUS("@ DispIdx Queue %d", pVDecInst->Display_index[pVDecInst->in_index]);
		nClearDispIndex--;
		ret = pVDecInst->gspfVDec (VDEC_BUF_FLAG_CLEAR, NULL, &nClearDispIndex, NULL, pVDecInst->pVdec_Instance);
		ALOGE ("%s VDEC_BUF_FLAG_CLEAR Err : [%d] index : %lu",__func__, ret, pVDecInst->Display_index[pVDecInst->out_index]);
	}
	pVDecInst->out_index = (pVDecInst->out_index + 1) % pVDecInst->max_fifo_cnt;
	if (pVDecInst->used_fifo_count > 0)
	pVDecInst->used_fifo_count--;

	return ret;
}

static int clear_vpu_buffer(OMX_COMPONENTTYPE * openmaxStandComp, _VIDEO_DECOD_INSTANCE_ *pVDecInst, OMX_S32 iStreamID)
{
	omx_videodec_component_PrivateType* omx_private = openmaxStandComp->pComponentPrivate;
	TCC_VDEC_PRIVATE *pPrivateData = (TCC_VDEC_PRIVATE*) omx_private->pPrivateData;
	int cleared_buff_count = 0;
	int loopCount = 0;
	int buffer_id = 0;

	while (1)
	{
		tcc_vsync_command(omx_private,TCC_LCDC_VIDEO_GET_DISPLAYED, &buffer_id);
		if (pVDecInst->isVPUClosed == OMX_TRUE)
		{
			// Do not need to clear buffers.
			return 0;
		}
		if (pVDecInst->nDisplayedFirstFrame < UNIQUE_ID_OF_FIRST_FRAME && buffer_id > 0)
		{
			if (pVDecInst->iStreamID == iStreamID)
			{
				if (SendFirstFrameEvent(openmaxStandComp, pVDecInst) == 0)
				{
					ALOGD("Video Frame First Output nDecoderUID(%d), UniqueID(%d)", (int)iStreamID, buffer_id);
				}
				//pVDecInst->nDisplayedFirstFrame++;
			}
		}
		if (buffer_id < 0/*UNIQUE_ID_OF_FIRST_FRAME*/)
		{
			// Clear a frame by force.
			buffer_id = pVDecInst->Display_Buff_ID[pVDecInst->out_index];
		}
		if (buffer_id >= pVDecInst->Display_Buff_ID[pVDecInst->out_index])
		{
			// Displayed frames.
			break;
		}
		if (++loopCount >= MAX_CHECK_COUNT_FOR_CLEAR)
		{
			// Timeout.
			break;
		}
		usleep(5000);
	}

	while (buffer_id >= pVDecInst->Display_Buff_ID[pVDecInst->out_index] && pVDecInst->used_fifo_count > 0)
	{
		clear_front_vpu_buffer(openmaxStandComp, pVDecInst);
		cleared_buff_count++;
	}

	if (cleared_buff_count == 0)
	{
		ALOGE("Video Buffer Clear Sync Fail : %d %lu , loopcount(%d)\n", buffer_id, pVDecInst->Display_Buff_ID[pVDecInst->out_index], loopCount);
		tcc_vsync_command(omx_private,TCC_LCDC_VIDEO_CLEAR_FRAME, UNIQUE_ID_OF_FIRST_FRAME);//pVDecInst->Display_Buff_ID[pVDecInst->out_index]);

		if (loopCount >= MAX_CHECK_COUNT_FOR_CLEAR)
		{
			if(pVDecInst->isVPUClosed == OMX_TRUE)
			{
				pPrivateData->bRollback = OMX_FALSE;
			}
			else if(pVDecInst->isVPUClosed == OMX_FALSE)
			{
				pPrivateData->bRollback = OMX_TRUE;
			}
 		}
		if (pVDecInst->used_fifo_count > 0)
		pVDecInst->used_fifo_count --;

		tcc_vsync_command(omx_private,TCC_LCDC_VIDEO_GET_DISPLAYED, &buffer_id);
		while (buffer_id >= pVDecInst->Display_Buff_ID[pVDecInst->out_index] && pVDecInst->used_fifo_count > 0)
		{
			clear_front_vpu_buffer(openmaxStandComp, pVDecInst);
			cleared_buff_count++;
		}

		tcc_vsync_command(omx_private, TCC_LCDC_VIDEO_SKIP_FRAME_START, 0);
		tcc_vsync_command(omx_private, TCC_LCDC_VIDEO_SKIP_FRAME_END, 0);
	}

	return cleared_buff_count;
}
#endif

static
OMX_S32
GetFrameType(
	OMX_S32 lVideoType,
	OMX_S32 lPictureType,
	OMX_S32 lPictureStructure
	)
{
	int frameType = 0; //unknown

	switch ( lVideoType )
	{
	case STD_VC1 :
		switch( (lPictureType>>3) ) //Frame or // FIELD_INTERLACED(TOP FIELD)
		{
		case PIC_TYPE_I:	frameType = PIC_TYPE_I; break;//I
		case PIC_TYPE_P:	frameType = PIC_TYPE_P; break;//P
		case 2:				frameType = PIC_TYPE_B; break;//B //DSTATUS( "BI  :" );
		case 3:				frameType = PIC_TYPE_B; break;//B //DSTATUS( "B   :" );
		case 4:				frameType = PIC_TYPE_B; break;//B //DSTATUS( "SKIP:" );
		}
		if( lPictureStructure == 3)
		{
			switch( (lPictureType&0x7) ) // FIELD_INTERLACED(BOTTOM FIELD)
			{
			case PIC_TYPE_I:	frameType = PIC_TYPE_I; break;//I
			case PIC_TYPE_P:	frameType = PIC_TYPE_P; break;//P
			case 2:				frameType = PIC_TYPE_B; break;//B //DSTATUS( "BI  :" );
			case 3:				frameType = PIC_TYPE_B; break;//B //DSTATUS( "B   :" );
			case 4:				frameType = PIC_TYPE_B; break;//B //DSTATUS( "SKIP:" );
			}
		}
		break;
	case STD_MPEG4 :
		switch( lPictureType )
		{
		case PIC_TYPE_I:	frameType = PIC_TYPE_I;	break;//I
		case PIC_TYPE_P:	frameType = PIC_TYPE_P;	break;//P
		case PIC_TYPE_B:	frameType = PIC_TYPE_B;	break;//B
		case PIC_TYPE_B_PB: frameType = PIC_TYPE_B;	break;//B of Packed PB-frame
		}
		break;
	case STD_MPEG2 :
	default:
		switch( lPictureType & 0xF )
		{
		case PIC_TYPE_I:	frameType = PIC_TYPE_I;	break;//I
		case PIC_TYPE_P:	frameType = PIC_TYPE_P;	break;//P
		case PIC_TYPE_B:	frameType = PIC_TYPE_B;	break;//B
		}
	}
	return frameType;
}

static int for_iptv_info_init(OMX_COMPONENTTYPE * openmaxStandComp)
{
	omx_videodec_component_PrivateType *omx_private;
	unsigned int 	i;

	omx_private = openmaxStandComp->pComponentPrivate;

	omx_private->stbroken_frame_info.broken_frame_cnt =0;
	for (i=0; i<32; i++)
		omx_private->stbroken_frame_info.broken_frame_index[i] = 0x100;

	omx_private->stbframe_skipinfo.bframe_skipcnt = 0;
	for (i=0; i<32; i++)
		omx_private->stbframe_skipinfo.bframe_skipindex[i] = 0x100;

	omx_private->stfristframe_dsipInfo.FirstFrame_DispStatus =0;

	omx_private->stIPTVActiveModeInfo.IPTV_Activemode =1;
	omx_private->stIPTVActiveModeInfo.IPTV_Old_Activemode = 0;
	omx_private->stIPTVActiveModeInfo.IPTV_Mode_change_cnt =0;
	return 0;
}

static int DisplayedFrameProcess(OMX_COMPONENTTYPE * openmaxStandComp, _VIDEO_DECOD_INSTANCE_ *pVDecInst, OMX_BUFFERHEADERTYPE *pBuffer)
{
	omx_videodec_component_PrivateType* omx_private = openmaxStandComp->pComponentPrivate;

	OMX_U32	nDecodeID;

	if (pVDecInst->avcodecInited)
	{
		t_Vpu_BufferCearFrameInfo *ClearFrameInfo;
		ClearFrameInfo = (t_Vpu_BufferCearFrameInfo *)TCC_fo_malloc(__func__, __LINE__,sizeof(t_Vpu_BufferCearFrameInfo));
		if(ClearFrameInfo == NULL)
		{
			pVDecInst->used_fifo_count++;
            clear_front_vpu_buffer(openmaxStandComp, pVDecInst);
            ALOGE("%s %d Queue buf get fail \n", __func__, __LINE__);
		}
		else
		{
			ClearFrameInfo->iDecoderID = pBuffer->nDecodeID;
			ClearFrameInfo->iStreamID = *((OMX_U32*)(pBuffer->pBuffer + 52));
			memcpy (&ClearFrameInfo->buffer_unique_id, pBuffer->pBuffer + 24, 4);
			memcpy (&ClearFrameInfo->ret, pBuffer->pBuffer + 28, 4);
			memcpy (&ClearFrameInfo->bVSyncEnabled, pBuffer->pBuffer + 48, 4);

			if(dxb_queue_ex(pVDecInst->pVPUDisplayedClearQueue, (void *)ClearFrameInfo) == 0 )
			{
				ALOGE( "In %s %d: ERROR CAN NOT PUSH CLEAR QUEUE [iDecoderID = %d] \n", __func__, __LINE__, ClearFrameInfo->iDecoderID);
				int err_cnt=0;
				while(1)
				{
					usleep(5*1000);
					if(err_cnt>1000)
					{
						pVDecInst->used_fifo_count++;
						clear_front_vpu_buffer(openmaxStandComp, pVDecInst);
						TCC_fo_free(__func__, __LINE__, ClearFrameInfo);
						break;
					}
					if(dxb_queue_ex(pVDecInst->pVPUDisplayedClearQueue, (void *)ClearFrameInfo) == 0 )
					{
						err_cnt++;
					}
					else
						break;
				}
			}
		}
	}

	return 0;
}

OMX_ERRORTYPE dxb_omx_videodec_SendBufferFunction (omx_base_PortType * openmaxStandPort, OMX_BUFFERHEADERTYPE * pBuffer)
{

	OMX_ERRORTYPE err;
	OMX_U32   portIndex;
	OMX_COMPONENTTYPE *omxComponent = openmaxStandPort->standCompContainer;
	omx_base_component_PrivateType *omx_base_component_Private =
		(omx_base_component_PrivateType *) omxComponent->pComponentPrivate;
#if NO_GST_OMX_PATCH
	unsigned int i;
#endif
	omx_videodec_component_PrivateType *omx_private = (omx_videodec_component_PrivateType *)omx_base_component_Private;

	portIndex = (openmaxStandPort->sPortParam.eDir == OMX_DirInput) ? pBuffer->nInputPortIndex : pBuffer->nOutputPortIndex;
	DEBUG (DEB_LEV_FUNCTION_NAME, "In %s portIndex %lu (%s)\n", __func__, portIndex, omx_base_component_Private->name);

	if (portIndex != openmaxStandPort->sPortParam.nPortIndex)
	{
		DEBUG (DEB_LEV_ERR, "In %s: wrong port for this operation portIndex=%d port->portIndex=%d\n", __func__,
			   (int) portIndex, (int) openmaxStandPort->sPortParam.nPortIndex);
		return OMX_ErrorBadPortIndex;
	}

	if (omx_base_component_Private->state == OMX_StateInvalid)
	{
		DEBUG (DEB_LEV_ERR, "In %s: we are in OMX_StateInvalid\n", __func__);
		return OMX_ErrorInvalidState;
	}

	if (omx_base_component_Private->state != OMX_StateExecuting &&
		omx_base_component_Private->state != OMX_StatePause && omx_base_component_Private->state != OMX_StateIdle)
	{
		DEBUG (DEB_LEV_ERR, "In %s: we are not in executing/paused/idle state, but in %d\n", __func__,
			   omx_base_component_Private->state);
		return OMX_ErrorIncorrectStateOperation;
	}
	if (!PORT_IS_ENABLED (openmaxStandPort)
		|| (PORT_IS_BEING_DISABLED (openmaxStandPort) && !PORT_IS_TUNNELED_N_BUFFER_SUPPLIER (openmaxStandPort))
		|| (omx_base_component_Private->transientState == OMX_TransStateExecutingToIdle
			&& (PORT_IS_TUNNELED (openmaxStandPort) && !PORT_IS_BUFFER_SUPPLIER (openmaxStandPort))))
	{
		DEBUG (DEB_LEV_ERR, "In %s: Port %d is disabled comp = %s, transientState : %d \n", __func__, (int) portIndex,
			   omx_base_component_Private->name, omx_base_component_Private->transientState);
		return OMX_ErrorIncorrectStateOperation;
	}

	/* Temporarily disable this check for gst-openmax */
#if NO_GST_OMX_PATCH
	{
		OMX_BOOL  foundBuffer = OMX_FALSE;
		if (pBuffer != NULL && pBuffer->pBuffer != NULL)
		{
			for (i = 0; i < openmaxStandPort->sPortParam.nBufferCountActual; i++)
			{
				if (pBuffer->pBuffer == openmaxStandPort->pInternalBufferStorage[i]->pBuffer)
				{
					foundBuffer = OMX_TRUE;
					break;
				}
			}
		}
		if (!foundBuffer)
		{
			return OMX_ErrorBadParameter;
		}
	}
#endif

	if ((err = checkHeader (pBuffer, sizeof (OMX_BUFFERHEADERTYPE))) != OMX_ErrorNone)
	{
		DEBUG (DEB_LEV_ERR, "In %s: received wrong buffer header on input port\n", __func__);
		return err;
	}

	/* And notify the buffer management thread we have a fresh new buffer to manage */
	if (!PORT_IS_BEING_FLUSHED (openmaxStandPort)
		&& !(PORT_IS_BEING_DISABLED (openmaxStandPort) && PORT_IS_TUNNELED_N_BUFFER_SUPPLIER (openmaxStandPort)))
	{
		DisplayedFrameProcess(omxComponent, &omx_private->pVideoDecodInstance[pBuffer->nDecodeID], pBuffer);

		dxb_queue (openmaxStandPort->pBufferQueue, pBuffer);
		tsem_up (openmaxStandPort->pBufferSem);
		DEBUG (DEB_LEV_PARAMS, "In %s Signalling bMgmtSem Port Index=%d\n", __func__, (int) portIndex);
		tsem_up (omx_base_component_Private->bMgmtSem);
	}
	else if (PORT_IS_BUFFER_SUPPLIER (openmaxStandPort))
	{
		DEBUG (DEB_LEV_FULL_SEQ, "In %s: Comp %s received io:%d buffer\n",
			   __func__, omx_base_component_Private->name, (int) openmaxStandPort->sPortParam.nPortIndex);
		dxb_queue (openmaxStandPort->pBufferQueue, pBuffer);
		tsem_up (openmaxStandPort->pBufferSem);
	}
	else
	{	// If port being flushed and not tunneled then return error
		DEBUG (DEB_LEV_FULL_SEQ, "In %s \n", __func__);
		return OMX_ErrorIncorrectStateOperation;
	}
	return OMX_ErrorNone;
}

extern OMX_ERRORTYPE dxb_omx_ring_videodec_component_Constructor (OMX_COMPONENTTYPE * openmaxStandComp, OMX_STRING cComponentName);

OMX_ERRORTYPE dxb_omx_videodec_mpeg2_component_Init (OMX_COMPONENTTYPE * openmaxStandComp)
{
	OMX_ERRORTYPE err = OMX_ErrorNone;
#if defined(RING_BUFFER_MODULE_ENABLE)
	char value[PROPERTY_VALUE_MAX];
	property_get("tcc.dxb.ringbuffer.enable", value, "1");

	if( atoi(value) )
	{
		err = dxb_omx_ring_videodec_component_Constructor(openmaxStandComp, VIDEO_DEC_MPEG2_NAME);
	}
	else
	{
		err = dxb_omx_videodec_component_Constructor (openmaxStandComp, VIDEO_DEC_MPEG2_NAME);
	}
#else
	err = dxb_omx_videodec_component_Constructor (openmaxStandComp, VIDEO_DEC_MPEG2_NAME);
#endif
	return err;//dxb_omx_videodec_component_Constructor (openmaxStandComp, VIDEO_DEC_MPEG2_NAME);
}

OMX_ERRORTYPE dxb_omx_videodec_h264_component_Init (OMX_COMPONENTTYPE * openmaxStandComp)
{
	OMX_ERRORTYPE err = OMX_ErrorNone;
#if defined(RING_BUFFER_MODULE_ENABLE)
	char value[PROPERTY_VALUE_MAX];
	property_get("tcc.dxb.ringbuffer.enable", value, "1");

	if( atoi(value) )
	{
		err = dxb_omx_ring_videodec_component_Constructor(openmaxStandComp, VIDEO_DEC_H264_NAME);
	}
	else
	{
		err = dxb_omx_videodec_component_Constructor (openmaxStandComp, VIDEO_DEC_H264_NAME);
	}
#else
	err = dxb_omx_videodec_component_Constructor (openmaxStandComp, VIDEO_DEC_H264_NAME);
#endif
	return err;//dxb_omx_videodec_component_Constructor (openmaxStandComp, VIDEO_DEC_H264_NAME);
}

/** The Constructor of the video decoder component
  * @param openmaxStandComp the component handle to be constructed
  * @param cComponentName is the name of the constructed component
  */
OMX_ERRORTYPE dxb_omx_videodec_component_Constructor (OMX_COMPONENTTYPE * openmaxStandComp, OMX_STRING cComponentName)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	omx_videodec_component_PrivateType *omx_private;
	omx_base_video_PortType *inPort, *outPort;
	OMX_U32   i;
	TCC_VDEC_PRIVATE *pPrivateData;

	if (!openmaxStandComp->pComponentPrivate)
	{
		DBUG_MSG ("In %s, allocating component\n", __func__);
		openmaxStandComp->pComponentPrivate = TCC_fo_calloc (__func__, __LINE__, 1, sizeof (omx_videodec_component_PrivateType));
		if (openmaxStandComp->pComponentPrivate == NULL)
		{
			return OMX_ErrorInsufficientResources;
		}
	}
	else
	{
		DBUG_MSG ("In %s, Error Component %x Already Allocated\n", __func__, (int) openmaxStandComp->pComponentPrivate);
	}

	omx_private = openmaxStandComp->pComponentPrivate;

	omx_private->pVideoDecodInstance = (_VIDEO_DECOD_INSTANCE_*)TCC_fo_malloc(__func__, __LINE__,sizeof(_VIDEO_DECOD_INSTANCE_) * MAX_INSTANCE_CNT);

	omx_private->ports = NULL;

	eError = dxb_omx_base_filter_Constructor (openmaxStandComp, cComponentName);

	omx_private->sPortTypesParam[OMX_PortDomainVideo].nStartPortNumber = 0;
	omx_private->sPortTypesParam[OMX_PortDomainVideo].nPorts = 4;

		/** Allocate Ports and call port constructor. */
	if (omx_private->sPortTypesParam[OMX_PortDomainVideo].nPorts && !omx_private->ports)
	{
		omx_private->ports =
			TCC_fo_calloc (__func__, __LINE__,omx_private->sPortTypesParam[OMX_PortDomainVideo].nPorts, sizeof (omx_base_PortType *));
		if (!omx_private->ports)
		{
			return OMX_ErrorInsufficientResources;
		}
		for (i = 0; i < omx_private->sPortTypesParam[OMX_PortDomainVideo].nPorts; i++)
		{
			omx_private->ports[i] = TCC_fo_calloc (__func__, __LINE__,1, sizeof (omx_base_video_PortType));
			if (!omx_private->ports[i])
			{
				if(omx_private->ports){
					TCC_fo_free(__func__,__LINE__, omx_private->ports);
				}
				return OMX_ErrorInsufficientResources;
			}
		}
	}

	dxb_base_video_port_Constructor (openmaxStandComp, &omx_private->ports[0], 0, OMX_TRUE);
	dxb_base_video_port_Constructor (openmaxStandComp, &omx_private->ports[1], 1, OMX_TRUE);
	dxb_base_video_port_Constructor (openmaxStandComp, &omx_private->ports[2], 2, OMX_FALSE);
	dxb_base_video_port_Constructor (openmaxStandComp, &omx_private->ports[3], 3, OMX_FALSE);

	/** now it's time to know the video coding type of the component */
	if (!strcmp (cComponentName, VIDEO_DEC_H264_NAME))
	{
		for (i = 0; i < MAX_INSTANCE_CNT; i++)
		{
			omx_private->pVideoDecodInstance[i].video_coding_type = OMX_VIDEO_CodingAVC;
			omx_private->pVideoDecodInstance[i].gsVDecInit.m_iBitstreamFormat = STD_AVC;
		}
	}
	else if (!strcmp (cComponentName, VIDEO_DEC_MPEG2_NAME))
	{
		for (i = 0; i < MAX_INSTANCE_CNT; i++)
		{
			omx_private->pVideoDecodInstance[i].video_coding_type = OMX_VIDEO_CodingMPEG2;
			omx_private->pVideoDecodInstance[i].gsVDecInit.m_iBitstreamFormat = STD_MPEG2;
		}
	}
	else if (!strcmp (cComponentName, VIDEO_DEC_BASE_NAME))
	{
		//Do nothing!!
	}
	else
	{
		// IL client specified an invalid component name
		return OMX_ErrorInvalidComponentName;
	}

	for(i=0; i<MAX_INSTANCE_CNT; i++)
	{
		memset (&omx_private->pVideoDecodInstance[i].gsVDecInput, 0x00, sizeof (vdec_input_t));
		memset (&omx_private->pVideoDecodInstance[i].gsVDecOutput, 0x00, sizeof (vdec_output_t));
		memset (&omx_private->pVideoDecodInstance[i].gsVDecInit, 0x00, sizeof (vdec_init_t));
		//omx_private->pVideoDecodInstance[i].pVdec_Instance = dxb_vdec_alloc_instance();
		omx_private->pVideoDecodInstance[i].pVdec_Instance = NULL;
		omx_private->pVideoDecodInstance[i].gspfVDec = gspfVDecList[omx_private->pVideoDecodInstance[i].gsVDecInit.m_iBitstreamFormat];

		if (omx_private->pVideoDecodInstance[i].gspfVDec == 0)
		{
			return OMX_ErrorComponentNotFound;
		}
	}

	/** here we can override whatever defaults the base_component constructor set
	* e.g. we can override the function pointers in the private struct
	*/

	/** Domain specific section for the ports.
	* first we set the parameter common to both formats
	*/
	//common parameters related to input port.
	inPort = (omx_base_video_PortType *) omx_private->ports[OMX_DXB_VIDEOCOMPONENT_INPORT_MAIN];
	inPort->sPortParam.nBufferSize = VIDEO_DXB_IN_BUFFER_SIZE;
	inPort->sPortParam.format.video.xFramerate = 30;
	inPort->sPortParam.format.video.nFrameWidth = 0;//AVAILABLE_MAX_WIDTH;
	inPort->sPortParam.format.video.nFrameHeight = 0;//AVAILABLE_MAX_HEIGHT;
	inPort->sPortParam.nBufferCountMin = 6;
	inPort->sPortParam.nBufferCountActual = 8;

	inPort = (omx_base_video_PortType *) omx_private->ports[OMX_DXB_VIDEOCOMPONENT_INPORT_SUB];
	inPort->sPortParam.nBufferSize = VIDEO_DXB_IN_BUFFER_SIZE;
	inPort->sPortParam.format.video.xFramerate = 30;
	inPort->sPortParam.format.video.nFrameWidth = 0;//AVAILABLE_MAX_WIDTH;
	inPort->sPortParam.format.video.nFrameHeight = 0;//AVAILABLE_MAX_HEIGHT;
	inPort->sPortParam.nBufferCountMin = 6;
	inPort->sPortParam.nBufferCountActual = 8;

	//common parameters related to output port
	outPort = (omx_base_video_PortType *) omx_private->ports[OMX_DXB_VIDEOCOMPONENT_OUTPORT];
	outPort->sPortParam.format.video.eColorFormat = OUTPUT_DECODED_COLOR_FMT;
	outPort->sPortParam.format.video.nFrameWidth = 0;//AVAILABLE_MAX_WIDTH;
	outPort->sPortParam.format.video.nFrameHeight = 0;//AVAILABLE_MAX_HEIGHT;
	//outPort->sPortParam.nBufferSize = (OMX_U32) (AVAILABLE_MAX_WIDTH * AVAILABLE_MAX_HEIGHT * 3 / 2);
	outPort->sPortParam.format.video.xFramerate = 30;
	outPort->sPortParam.nBufferCountMin = VIDEO_BUFFERCOUNT_MIN;
	outPort->sPortParam.nBufferCountActual = VIDEO_BUFFERCOUNT_ACTUAL;
	outPort->sPortParam.nBufferSize = 1024;
	outPort->sVideoParam.xFramerate = 30;
	outPort->Port_SendBufferFunction = dxb_omx_videodec_SendBufferFunction;

	outPort = (omx_base_video_PortType *) omx_private->ports[OMX_DXB_VIDEOCOMPONENT_OUTPORT_SUB];
	outPort->sPortParam.format.video.eColorFormat = OUTPUT_DECODED_COLOR_FMT;
	outPort->sPortParam.format.video.nFrameWidth = 0;//AVAILABLE_MAX_WIDTH;
	outPort->sPortParam.format.video.nFrameHeight = 0;//AVAILABLE_MAX_HEIGHT;
	//outPort->sPortParam.nBufferSize = (OMX_U32) (AVAILABLE_MAX_WIDTH * AVAILABLE_MAX_HEIGHT * 3 / 2);
	outPort->sPortParam.format.video.xFramerate = 30;
	outPort->sPortParam.nBufferCountMin = VIDEO_BUFFERCOUNT_MIN;
	outPort->sPortParam.nBufferCountActual = VIDEO_BUFFERCOUNT_ACTUAL;
	outPort->sPortParam.nBufferSize = 1024;
	outPort->sVideoParam.xFramerate = 30;
	outPort->Port_SendBufferFunction = dxb_omx_videodec_SendBufferFunction;

	if (!omx_private->avCodecSyncSem)
	{
		omx_private->avCodecSyncSem = TCC_fo_malloc (__func__, __LINE__,sizeof (tsem_t));
		if (omx_private->avCodecSyncSem == NULL)
		{
			return OMX_ErrorInsufficientResources;
		}
		tsem_init (omx_private->avCodecSyncSem, 0);
	}

	/** general configuration irrespective of any video formats
	* setting other parameters of omx_videodec_component_private
	*/
	omx_private->BufferMgmtCallback = dxb_omx_videodec_component_BufferMgmtCallback;

	/** initializing the codec context etc that was done earlier by ffmpeglibinit function */
	omx_private->messageHandler = dxb_omx_videodec_component_MessageHandler;
	omx_private->destructor = dxb_omx_videodec_component_Destructor;
	//omx_private->BufferMgmtFunction = dxb_omx_twoport_filter_component_BufferMgmtFunction;
	omx_private->BufferMgmtFunction = dxb_omx_video_twoport_filter_component_BufferMgmtFunction;
	openmaxStandComp->SetParameter = dxb_omx_videodec_component_SetParameter;
	openmaxStandComp->GetParameter = dxb_omx_videodec_component_GetParameter;
	openmaxStandComp->SetConfig = dxb_omx_videodec_component_SetConfig;
	openmaxStandComp->ComponentRoleEnum = dxb_omx_videodec_component_ComponentRoleEnum;
	openmaxStandComp->GetExtensionIndex = dxb_omx_videodec_component_GetExtensionIndex;

	omx_private->gHDMIOutput = OMX_FALSE;

	OMX_U32 hwc_id = (getHWCID() <= MAX_INSTANCE_CNT) ? MAX_INSTANCE_CNT + 1 : 1;
	for(i=0; i<MAX_INSTANCE_CNT; i++)
	{
		pthread_mutex_init(&omx_private->pVideoDecodInstance[i].stVideoStart.mutex, NULL);

		memset(omx_private->pVideoDecodInstance[i].Display_index, 0x00, sizeof(OMX_U32)*VPU_BUFF_COUNT);
		memset(omx_private->pVideoDecodInstance[i].Display_Buff_ID, 0x00, sizeof(OMX_U32)*VPU_BUFF_COUNT);
		memset(omx_private->pVideoDecodInstance[i].Send_Buff_ID, 0x00, sizeof(OMX_U32)*VPU_REQUEST_BUFF_COUNT);

		omx_private->pVideoDecodInstance[i].max_fifo_cnt = VPU_BUFF_COUNT;
		omx_private->pVideoDecodInstance[i].buffer_unique_id = UNIQUE_ID_OF_FIRST_FRAME;
		omx_private->pVideoDecodInstance[i].iStreamID = hwc_id + i;
		omx_private->pVideoDecodInstance[i].nDisplayedFirstFrame = 0;
		omx_private->pVideoDecodInstance[i].nDisplayedLastFrame = 0;

		omx_private->pVideoDecodInstance[i].pVPUDisplayedIndexQueue = (queue_t *)TCC_fo_calloc (__func__, __LINE__,1,sizeof(queue_t));
		dxb_queue_init_ex(omx_private->pVideoDecodInstance[i].pVPUDisplayedIndexQueue, 32); //Max size is 32

		omx_private->pVideoDecodInstance[i].pVPUDisplayedClearQueue = (queue_t *)TCC_fo_calloc (__func__, __LINE__,1,sizeof(queue_t));
		dxb_queue_init_ex(omx_private->pVideoDecodInstance[i].pVPUDisplayedClearQueue, 32); //Max size is 32

		omx_private->pVideoDecodInstance[i].avcodecReady = OMX_FALSE;
		omx_private->pVideoDecodInstance[i].video_dec_idx = 0;
#ifdef VIDEO_USER_DATA_PROCESS
		omx_private->pVideoDecodInstance[i].pUserData_List_Array = TCC_fo_malloc(__func__, __LINE__,sizeof(video_userdata_list_t)*MAX_USERDATA_LIST_ARRAY);
		UserDataCtrl_Init(omx_private->pVideoDecodInstance[i].pUserData_List_Array);
#endif
		memset(omx_private->pVideoDecodInstance[i].dec_disp_info, 0x00, sizeof(dec_disp_info_t)*32);
		memset(&omx_private->pVideoDecodInstance[i].dec_disp_info_input, 0x00, sizeof(dec_disp_info_input_t));
		omx_private->pVideoDecodInstance[i].bVideoStarted = OMX_FALSE;
		omx_private->pVideoDecodInstance[i].bVideoPaused = OMX_FALSE;
#ifdef TS_TIMESTAMP_CORRECTION
		memset(&omx_private->pVideoDecodInstance[i].gsTSPtsInfo, 0x00, sizeof(ts_pts_ctrl));
#endif
		memset(&omx_private->pVideoDecodInstance[i].stVideoStart, 0x00, sizeof(VideoStartInfo));
		omx_private->pVideoDecodInstance[i].bitrate_mbps = 20; //default 20Mbps

		omx_private->pVideoDecodInstance[i].isVPUClosed  = OMX_TRUE;

#ifdef  SUPPORT_PVR
		omx_private->pVideoDecodInstance[i].nPVRFlags = 0;
		omx_private->pVideoDecodInstance[i].nDelayedOutputBufferCount = 0;
		omx_private->pVideoDecodInstance[i].nSkipOutputBufferCount = 0;
		omx_private->pVideoDecodInstance[i].nPlaySpeed = 1;
#endif//SUPPORT_PVR
#ifdef SET_FRAMEBUFFER_INTO_MAX
		omx_private->pVideoDecodInstance[i].rectCropParm.nLeft	= 0;
		omx_private->pVideoDecodInstance[i].rectCropParm.nTop		= 0;
		omx_private->pVideoDecodInstance[i].rectCropParm.nWidth	= 0;
		omx_private->pVideoDecodInstance[i].rectCropParm.nHeight	= 0;
#endif
	}
	memset(&(omx_private->gsMPEG2PtsInfo), 0x00, sizeof(mpeg2_pts_ctrl));

	omx_private->gVideo_FrameRate = 30;

	omx_private->iDisplayID = 0;
	omx_private->Resolution_Change = 0;

	omx_private->stVideoSubFunFlag.SupportFieldDecoding =0;
	omx_private->stVideoSubFunFlag.SupportIFrameSearchMode =1;
	omx_private->stVideoSubFunFlag.SupprotUsingErrorMB =0;
	omx_private->stVideoSubFunFlag.SupprotDirectDsiplay =0;

#if 1	//for Dual-decoding
	omx_private->stPreVideoInfo.iDecod_Instance = -1;
	omx_private->stPreVideoInfo.iDecod_status = -1;
	omx_private->stPreVideoInfo.uDecod_time = 0;
#endif

	omx_private->pPrivateData = TCC_fo_calloc(__func__, __LINE__, 1, sizeof(TCC_VDEC_PRIVATE));
	if(omx_private->pPrivateData == NULL)
	{
		ALOGE("[%s] pPrivateData TCC_calloc fail / Error !!!!!\n", __func__);
		return OMX_ErrorInsufficientResources;
	}

	pPrivateData = (TCC_VDEC_PRIVATE*) omx_private->pPrivateData;

	pPrivateData->mFBdevice = -1;

	pPrivateData->stIsdbtProprietaryData.channel_index = -1;
	pPrivateData->stIsdbtProprietaryData.dummy = 0;

	for_iptv_info_init(openmaxStandComp);
	return eError;
}


/** The destructor of the video decoder component
  */
OMX_ERRORTYPE dxb_omx_videodec_component_Destructor (OMX_COMPONENTTYPE * openmaxStandComp)
{
	omx_videodec_component_PrivateType *omx_private = NULL;
	TCC_VDEC_PRIVATE *pPrivateData = NULL;
	OMX_U32   i;

	vdec_init_error_flag = 0;
	vdec_seq_header_error_flag = 0;
	vdec_decode_error_flag = 0;
	vdec_buffer_full_error_flag = 0;
	vdec_buffer_clear_error_flag = 0;
	vdec_no_input_error_flag = 0;

	if(openmaxStandComp == NULL || openmaxStandComp->pComponentPrivate == NULL)
	{
		ALOGE("[%s] parameter is not valid / Error !!!!!\n", __func__);
		return OMX_ErrorBadParameter;
	}

	omx_private = openmaxStandComp->pComponentPrivate;
	pPrivateData = (TCC_VDEC_PRIVATE*) omx_private->pPrivateData;

	if (omx_private->avCodecSyncSem)
	{
		tsem_deinit (omx_private->avCodecSyncSem);
		TCC_fo_free (__func__, __LINE__,omx_private->avCodecSyncSem);
		omx_private->avCodecSyncSem = NULL;
	}

	/* frees port/s */
	if (omx_private->ports)
	{
		for (i = 0; i < omx_private->sPortTypesParam[OMX_PortDomainVideo].nPorts; i++)
		{
			if (omx_private->ports[i])
				omx_private->ports[i]->PortDestructor (omx_private->ports[i]);
		}
		TCC_fo_free (__func__, __LINE__,omx_private->ports);
		omx_private->ports = NULL;
	}

	DBUG_MSG ("Destructor of video decoder component is called\n");

	for(i=0; i<MAX_INSTANCE_CNT; i++)
	{
		dxb_queue_deinit(omx_private->pVideoDecodInstance[i].pVPUDisplayedIndexQueue);
		TCC_fo_free (__func__, __LINE__,omx_private->pVideoDecodInstance[i].pVPUDisplayedIndexQueue);

		dxb_queue_deinit(omx_private->pVideoDecodInstance[i].pVPUDisplayedClearQueue);
		TCC_fo_free (__func__, __LINE__,omx_private->pVideoDecodInstance[i].pVPUDisplayedClearQueue);

#ifdef VIDEO_USER_DATA_PROCESS
		if(omx_private->pVideoDecodInstance[i].pUserData_List_Array)
		{
			UserDataCtrl_ResetAll(omx_private->pVideoDecodInstance[i].pUserData_List_Array);
		}
#endif
		if(omx_private->pVideoDecodInstance[i].pVdec_Instance)
			dxb_vdec_release_instance((void*)omx_private->pVideoDecodInstance[i].pVdec_Instance);
		pthread_mutex_destroy(&omx_private->pVideoDecodInstance[i].stVideoStart.mutex);
	}
	if(omx_private->pVideoDecodInstance)
		TCC_fo_free (__func__, __LINE__,omx_private->pVideoDecodInstance);

	if(pPrivateData->mFBdevice >= 0)
	{
		close(pPrivateData->mFBdevice);
		pPrivateData->mFBdevice = -1;
	}

	pPrivateData->stIsdbtProprietaryData.channel_index = -1;
	pPrivateData->stIsdbtProprietaryData.dummy = 0;

	TCC_fo_free (__func__, __LINE__,omx_private->pPrivateData);

	dxb_omx_base_filter_Destructor (openmaxStandComp);

	return OMX_ErrorNone;
}

/** It initializates the FFmpeg framework, and opens an FFmpeg videodecoder of type specified by IL client
  */
OMX_ERRORTYPE dxb_omx_videodec_component_LibInit (omx_videodec_component_PrivateType * omx_private, OMX_U8 uiDecoderID)
{
	int i;
	OMX_U32   target_codecID;
	OMX_S8    value[PROPERTY_VALUE_MAX];
	OMX_U32   uiHDMIOutputMode = 0;

	tsem_up (omx_private->avCodecSyncSem);

	{
		omx_private->pVideoDecodInstance[uiDecoderID].avcodecInited = 0;
		omx_private->pVideoDecodInstance[uiDecoderID].container_type = 0;
		if (omx_private->pVideoDecodInstance[uiDecoderID].video_coding_type == OMX_VIDEO_CodingMPEG2)
		{
			/*below is default value fot broadcastring
			 */
			omx_private->pVideoDecodInstance[uiDecoderID].container_type = CONTAINER_TYPE_NONE;
			//omx_private->container_type = CONTAINER_TYPE_MPG;
			omx_private->pVideoDecodInstance[uiDecoderID].cdmx_info.m_sVideoInfo.m_iFrameRate = 25;
		}
		else if (omx_private->pVideoDecodInstance[uiDecoderID].video_coding_type == OMX_VIDEO_CodingAVC)
		{
			/*below is default value fot broadcastring
			 */
			omx_private->pVideoDecodInstance[uiDecoderID].container_type = CONTAINER_TYPE_NONE;
			//omx_private->container_type = CONTAINER_TYPE_TS;
			omx_private->pVideoDecodInstance[uiDecoderID].cdmx_info.m_sVideoInfo.m_iFrameRate = 25;
		}
		omx_private->pVideoDecodInstance[uiDecoderID].guiDisplayBufFullCount = 0;
	}

#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
	omx_private->pVideoDecodInstance[uiDecoderID].out_index = 0;
	omx_private->pVideoDecodInstance[uiDecoderID].in_index = 0;
	omx_private->pVideoDecodInstance[uiDecoderID].used_fifo_count = 0;
#endif

	property_get ("tcc.sys.output_mode_detected", (char*)value, "");
	uiHDMIOutputMode = atoi ((char*)value);
	if (uiHDMIOutputMode == OUTPUT_HDMI)
	{
		ALOGD ("HDMI output enabled");
		omx_private->gHDMIOutput = OMX_TRUE;
	}

	omx_private->guiSkipBframeEnable = 0;
	omx_private->guiSkipBFrameNumber = 0;
	return OMX_ErrorNone;
}

/** It Deinitializates the ffmpeg framework, and close the ffmpeg video decoder of selected coding type
  */
void dxb_omx_videodec_component_LibDeinit (omx_videodec_component_PrivateType * omx_private, OMX_U8 uiDecoderID)
{
	int ret;

	if (uiDecoderID >= MAX_INSTANCE_CNT) {
		ALOGE ("In %s, Invalid decoder id(%d)\n", __func__, (int)uiDecoderID);
		return;
	}

	if (omx_private->pVideoDecodInstance[uiDecoderID].avcodecInited)
	{
		if ((ret = omx_private->pVideoDecodInstance[uiDecoderID].gspfVDec (VDEC_CLOSE, uiDecoderID, NULL, &omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput, omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance)) < 0)
		{
			ALOGE ("[VDEC_CLOSE:%d] [Err:%4d] video decoder Deinit", uiDecoderID, ret);
		}
//		omx_private->pVideoDecodInstance[uiDecoderID].gspfVDec (VDEC_HW_RESET, NULL, NULL, &omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput, omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance);
	}
	omx_private->pVideoDecodInstance[uiDecoderID].isVPUClosed = OMX_TRUE;
}

/** The Initialization function of the video decoder
  */
OMX_ERRORTYPE dxb_omx_videodec_component_Initialize (OMX_COMPONENTTYPE * openmaxStandComp)
{
	omx_videodec_component_PrivateType *omx_private = openmaxStandComp->pComponentPrivate;
	OMX_ERRORTYPE eError = OMX_ErrorNone;

	/** Temporary First Output buffer size */
	omx_private->inputCurrBuffer = NULL;
	omx_private->inputCurrLength = 0;
	omx_private->isFirstBuffer = 1;
	return eError;
}

/** The Deinitialization function of the video decoder
  */
OMX_ERRORTYPE dxb_omx_videodec_component_Deinit (OMX_COMPONENTTYPE * openmaxStandComp)
{
	int i;
	omx_videodec_component_PrivateType *omx_private = openmaxStandComp->pComponentPrivate;
	OMX_ERRORTYPE eError = OMX_ErrorNone;

	for(i=0; i<MAX_INSTANCE_CNT; i++)
	{
		if (omx_private->pVideoDecodInstance[i].avcodecReady)
		{
			omx_private->pVideoDecodInstance[i].avcodecReady = OMX_FALSE;
			dxb_omx_videodec_component_LibDeinit (omx_private, i);
		}
	#ifdef VIDEO_USER_DATA_PROCESS
		if(omx_private->pVideoDecodInstance[i].pUserData_List_Array)
			UserDataCtrl_ResetAll(omx_private->pVideoDecodInstance[i].pUserData_List_Array);
	#endif
	}

	return eError;
}

OMX_ERRORTYPE dxb_omx_videodec_component_Change (OMX_COMPONENTTYPE * openmaxStandComp, OMX_STRING cComponentName, OMX_U8 uiDecoderID)
{

	OMX_ERRORTYPE eError = OMX_ErrorNone;
	omx_videodec_component_PrivateType *omx_private;
	omx_base_video_PortType *inPort, *outPort;
	OMX_U32   i;

	if (!openmaxStandComp->pComponentPrivate)
	{
		DBUG_MSG("In %s, Error Insufficient Resources\n", __func__);

		return OMX_ErrorInsufficientResources;
	}

	omx_private = openmaxStandComp->pComponentPrivate;

	memset (&omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput, 0x00, sizeof (vdec_input_t));
	memset (&omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput, 0x00, sizeof (vdec_output_t));
	memset (&omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit, 0x00, sizeof (vdec_init_t));
	omx_private->pVideoDecodInstance[uiDecoderID].bitrate_mbps = 20; //default 20Mbps

	/** now it's time to know the video coding type of the component */
	if (!strcmp (cComponentName, VIDEO_DEC_H264_NAME))
	{
		omx_private->pVideoDecodInstance[uiDecoderID].video_coding_type = OMX_VIDEO_CodingAVC;
		omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iBitstreamFormat = STD_AVC;
	}
	else if (!strcmp (cComponentName, VIDEO_DEC_MPEG2_NAME))
	{
		omx_private->pVideoDecodInstance[uiDecoderID].video_coding_type = OMX_VIDEO_CodingMPEG2;
		omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iBitstreamFormat = STD_MPEG2;
	}
	else
	{
		// IL client specified an invalid component name
		return OMX_ErrorInvalidComponentName;
	}

	omx_private->pVideoDecodInstance[uiDecoderID].gspfVDec = gspfVDecList[omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iBitstreamFormat];

	/* common parameters related to output port */
	if (uiDecoderID==0) outPort = (omx_base_video_PortType *) omx_private->ports[OMX_DXB_VIDEOCOMPONENT_OUTPORT];
	else outPort = (omx_base_video_PortType *) omx_private->ports[OMX_DXB_VIDEOCOMPONENT_OUTPORT_SUB];
	/* settings of output port parameter definition */
		outPort->sPortParam.format.video.eColorFormat = OUTPUT_DECODED_COLOR_FMT;

	if (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iBitstreamFormat >= STD_AVC)
		omx_private->pVideoDecodInstance[uiDecoderID].gspfVDec = gspfVDecList[omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iBitstreamFormat];
	else
		omx_private->pVideoDecodInstance[uiDecoderID].gspfVDec = 0;

	if (omx_private->pVideoDecodInstance[uiDecoderID].gspfVDec == 0)
	{
		return OMX_ErrorComponentNotFound;
	}

	if (!omx_private->avCodecSyncSem)
	{
		omx_private->avCodecSyncSem = TCC_fo_malloc (__func__, __LINE__,sizeof (tsem_t));
		if (omx_private->avCodecSyncSem == NULL)
		{
			return OMX_ErrorInsufficientResources;
		}
		tsem_init (omx_private->avCodecSyncSem, 0);
	}
	return eError;
}

/** Executes all the required steps after an output buffer frame-size has changed.
*/
#if 0
static inline void UpdateFrameSize (OMX_COMPONENTTYPE * openmaxStandComp)
{

	omx_videodec_component_PrivateType *omx_private = openmaxStandComp->pComponentPrivate;
	omx_base_video_PortType *outPort = (omx_base_video_PortType *) omx_private->ports[OMX_DXB_VIDEOCOMPONENT_OUTPORT];
	omx_base_video_PortType *inPort = (omx_base_video_PortType *) omx_private->ports[OMX_DXB_VIDEOCOMPONENT_INPORT_MAIN];
	outPort->sPortParam.format.video.nFrameWidth = inPort->sPortParam.format.video.nFrameWidth;
	outPort->sPortParam.format.video.nFrameHeight = inPort->sPortParam.format.video.nFrameHeight;
	outPort->sPortParam.format.video.xFramerate = inPort->sPortParam.format.video.xFramerate;
	switch (outPort->sVideoParam.eColorFormat)
	{
	case OMX_COLOR_FormatYUV420Planar:
		if (outPort->sPortParam.format.video.nFrameWidth && outPort->sPortParam.format.video.nFrameHeight)
		{
			outPort->sPortParam.nBufferSize =
				outPort->sPortParam.format.video.nFrameWidth * outPort->sPortParam.format.video.nFrameHeight * 3 / 2;
		}
		break;
	default:
		if (outPort->sPortParam.format.video.nFrameWidth && outPort->sPortParam.format.video.nFrameHeight)
		{
			outPort->sPortParam.nBufferSize =
				outPort->sPortParam.format.video.nFrameWidth * outPort->sPortParam.format.video.nFrameHeight * 3;
		}
		break;
	}

}
#endif

static int get_total_handle_cnt(OMX_COMPONENTTYPE * openmaxStandComp)
{
	int i, cnt=0;
	omx_videodec_component_PrivateType *omx_private = openmaxStandComp->pComponentPrivate;

	for(i=0; i<MAX_INSTANCE_CNT; i++)
	{
		if(omx_private->pVideoDecodInstance[i].avcodecInited)
		{
			cnt++;
		}
	}
	return cnt;
}

#ifdef SET_FRAMEBUFFER_INTO_MAX
static int CheckCrop_DecodedOut(OMX_COMPONENTTYPE *openmaxStandComp, OMX_U8 uiDecoderID, unsigned int mCropLeft, unsigned int mCropTop, unsigned int mCropWidth, unsigned int mCropHeight)
{
	omx_videodec_component_PrivateType* omx_private = openmaxStandComp->pComponentPrivate;
	OMX_CONFIG_RECTTYPE rectParm;
	OMX_U32 bCropChanged = OMX_FALSE;

	rectParm.nLeft 		= mCropLeft;
	rectParm.nTop 		= mCropTop;
	rectParm.nWidth 	= mCropWidth;
	rectParm.nHeight 	= mCropHeight;
	if(	mCropWidth > AVAILABLE_MAX_WIDTH
	||	mCropHeight > AVAILABLE_MAX_HEIGHT)
	{
		return 0;
	}
	if( rectParm.nLeft != omx_private->pVideoDecodInstance[uiDecoderID].rectCropParm.nLeft
	||	rectParm.nTop != omx_private->pVideoDecodInstance[uiDecoderID].rectCropParm.nTop
	||	rectParm.nWidth != omx_private->pVideoDecodInstance[uiDecoderID].rectCropParm.nWidth
	||	rectParm.nHeight != omx_private->pVideoDecodInstance[uiDecoderID].rectCropParm.nHeight)
	{
		ALOGD("[%d]Resolution is Changed with Crop :: %ldx%ld <= %ld,%ld - %ldx%ld", uiDecoderID, mCropWidth, mCropHeight
														,	omx_private->pVideoDecodInstance[uiDecoderID].rectCropParm.nLeft
														,	omx_private->pVideoDecodInstance[uiDecoderID].rectCropParm.nTop
														,	omx_private->pVideoDecodInstance[uiDecoderID].rectCropParm.nWidth
														,	omx_private->pVideoDecodInstance[uiDecoderID].rectCropParm.nHeight);

		omx_private->pVideoDecodInstance[uiDecoderID].rectCropParm.nLeft	= rectParm.nLeft;
		omx_private->pVideoDecodInstance[uiDecoderID].rectCropParm.nTop		= rectParm.nTop;
		omx_private->pVideoDecodInstance[uiDecoderID].rectCropParm.nWidth	= rectParm.nWidth;
		omx_private->pVideoDecodInstance[uiDecoderID].rectCropParm.nHeight	= rectParm.nHeight;

		return 1;
	}

	return 0;
}
#endif

static int isPortChange (OMX_COMPONENTTYPE * openmaxStandComp, OMX_U8 uiDecoderID)
{
	omx_videodec_component_PrivateType *omx_private = openmaxStandComp->pComponentPrivate;
	omx_base_video_PortType *outPort;
	TCC_VDEC_PRIVATE *pPrivateData = (TCC_VDEC_PRIVATE*) omx_private->pPrivateData;
	int left, top, width, height;
	int ret = 0;
	int i;
	OMX_U32 bCropChanged = OMX_FALSE;

	if (uiDecoderID == 0) outPort = (omx_base_video_PortType *)omx_private->ports[OMX_DXB_VIDEOCOMPONENT_OUTPORT];
	else outPort= (omx_base_video_PortType *)omx_private->ports[OMX_DXB_VIDEOCOMPONENT_OUTPORT_SUB];

#ifdef SET_FRAMEBUFFER_INTO_MAX
	if (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iBitstreamFormat == STD_AVC)
	{
		left 	= omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_CropInfo.m_iCropLeft;
		top		= omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_CropInfo.m_iCropTop;
	}
	else
	{
		left 	= 0;
		top		= 0;
	}
	width	= omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iWidth;
	height	= omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iHeight;

	if(width && height)
	{
		if (( omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iPicWidth != width)
		||	( omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iPicHeight != height))
		{
			/* if image size of encoded stream is changed ? */
			ALOGD("[%d]Resolution change detected (%dx%d ==> %dx%d)", uiDecoderID,
					omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iPicWidth,
					omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iPicHeight,
					width,
					height);
			ret = -2;
			if(omx_private->pVideoDecodInstance[uiDecoderID].gsVDecUserInfo.bMaxfbMode)
			{
				if(omx_private->pVideoDecodInstance[uiDecoderID].rectCropParm.nWidth == 0
				|| omx_private->pVideoDecodInstance[uiDecoderID].rectCropParm.nHeight == 0)
				{
					bCropChanged = OMX_TRUE;
					ret = 1;
				}
				outPort->sPortParam.format.video.nFrameWidth = AVAILABLE_MAX_WIDTH;
				outPort->sPortParam.format.video.nFrameHeight = AVAILABLE_MAX_HEIGHT;
				omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iPicWidth = omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iPicWidth = width;
				omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iPicHeight = omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iPicHeight = height;
			}
			else
			{
				outPort->sPortParam.format.video.nFrameWidth = 0;
				outPort->sPortParam.format.video.nFrameHeight = 0;
				omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iPicWidth = omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iPicWidth = width;
				omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iPicHeight = omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iPicHeight = height;
			}
			omx_private->Resolution_Change = OMX_TRUE;
		}
		else
		{
			if(omx_private->pVideoDecodInstance[uiDecoderID].gsVDecUserInfo.bMaxfbMode)
			{
				width	= AVAILABLE_MAX_WIDTH;
				height	= AVAILABLE_MAX_HEIGHT;
				if(omx_private->pVideoDecodInstance[uiDecoderID].rectCropParm.nWidth == 0
				|| omx_private->pVideoDecodInstance[uiDecoderID].rectCropParm.nHeight == 0)
				{
					bCropChanged = OMX_TRUE;
					ret = 1;
				}
			}
			else
			{
				if (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iBitstreamFormat == STD_AVC)
				{
					width	= width - omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_CropInfo.m_iCropLeft - omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_CropInfo.m_iCropRight;
					height	= height - omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_CropInfo.m_iCropBottom - omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_CropInfo.m_iCropTop;
				}
			}

			if( outPort->sPortParam.format.video.nFrameWidth && outPort->sPortParam.format.video.nFrameHeight )
			{
				if ((outPort->sPortParam.format.video.nFrameWidth != (unsigned long)width)
				||	(outPort->sPortParam.format.video.nFrameHeight != (unsigned long)height))
				{
					ALOGD("[%d]%d - %d - %d, %d - %d - %d", uiDecoderID,
							omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iWidth,
							omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_CropInfo.m_iCropLeft,
							omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_CropInfo.m_iCropRight,
							omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iHeight,
							omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_CropInfo.m_iCropTop,
							omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_CropInfo.m_iCropBottom);
					ALOGD("[%d]Resolution change detected (%ldx%ld ==> %dx%d)", uiDecoderID,
							outPort->sPortParam.format.video.nFrameWidth,
							outPort->sPortParam.format.video.nFrameHeight,
							width,
							height);
					ret = 1;
				}
			}
			else
			{
				ret =  1;
			}
		}
	}

#else
	width	= omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iWidth;
	height	= omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iHeight;
	if(width && height)
	{
		if (( omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iPicWidth != width)
		||	( omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iPicHeight != height))
		{
			/* if image size of encoded stream is changed ? */
			ALOGD("[%d]Resolution change detected (%dx%d ==> %dx%d)", uiDecoderID,
					omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iPicWidth,
					omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iPicHeight,
                        width,
                        height);
			outPort->sPortParam.format.video.nFrameWidth = 0;
			outPort->sPortParam.format.video.nFrameHeight = 0;

			omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iPicWidth = omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iPicWidth = width;
			omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iPicHeight = omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iPicHeight = height;
			omx_private->Resolution_Change = OMX_TRUE;

			ret = -2;
		}
		else
		{
			if( outPort->sPortParam.format.video.nFrameWidth && outPort->sPortParam.format.video.nFrameHeight )
			{
				if (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iBitstreamFormat == STD_AVC) {
					width -= omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_CropInfo.m_iCropLeft;
					width -= omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_CropInfo.m_iCropRight;
					height -= omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_CropInfo.m_iCropBottom;
					height -= omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_CropInfo.m_iCropTop;
				}
				if ((outPort->sPortParam.format.video.nFrameWidth != (unsigned long)width) ||
						(outPort->sPortParam.format.video.nFrameHeight != (unsigned long)height)) {
					ALOGD("[%d]%d - %d - %d, %d - %d - %d", uiDecoderID,
							omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iWidth,
							omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_CropInfo.m_iCropLeft,
							omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_CropInfo.m_iCropRight,
							omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iHeight,
							omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_CropInfo.m_iCropTop,
							omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_CropInfo.m_iCropBottom);
					ALOGD("[%d]Resolution change detected (%ldx%ld ==> %dx%d)", uiDecoderID,
							outPort->sPortParam.format.video.nFrameWidth,
							outPort->sPortParam.format.video.nFrameHeight,
							width,
							height);
					ret = 1;
				}
			}
			else
			{
				if (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iBitstreamFormat == STD_AVC) {
					width -= omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_CropInfo.m_iCropLeft;
					width -= omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_CropInfo.m_iCropRight;
					height -= omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_CropInfo.m_iCropBottom;
					height -= omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_CropInfo.m_iCropTop;
				}
				ret = 1;
			}
		}
	}
#endif

	if (width > AVAILABLE_MAX_WIDTH || ((width * height) > AVAILABLE_MAX_REGION))
	{
		ALOGE("[%d]%d x %d ==> MAX-Resolution(%d x %d) over!!", uiDecoderID,
			width, height, AVAILABLE_MAX_WIDTH, AVAILABLE_MAX_HEIGHT);
		ret = -1;
	}

	if (width < AVAILABLE_MIN_WIDTH || height < AVAILABLE_MIN_HEIGHT)
	{
		ALOGE("[%d]%d x %d ==> MIN-Resolution(%d x %d) less!!", (int)uiDecoderID,
			width, height, AVAILABLE_MIN_WIDTH, AVAILABLE_MIN_HEIGHT);
		ret = -1;
	}

	if (pPrivateData->bChangeDisplayID==OMX_TRUE && ret != 1) {
		//this routine shoud be run only when ret is not 1
		int is_progressive;
		int iaspect_ratio;

		if (omx_private->iDisplayID == uiDecoderID) {
			if (omx_private->pVideoDecodInstance[uiDecoderID].avcodecInited) {
				is_progressive = omx_private->stVideoDefinition.bProgressive;
				iaspect_ratio= omx_private->stVideoDefinition.nAspectRatio;

				(*(omx_private->callbacks->EventHandler)) (openmaxStandComp,
								omx_private->callbackData,
								OMX_EventVendorVideoChange,
								is_progressive, iaspect_ratio, outPort);

				(*(omx_private->callbacks->EventHandler)) (openmaxStandComp,
								omx_private->callbackData,
								OMX_EventPortSettingsChanged,
								is_progressive, iaspect_ratio, outPort);
			}
			pPrivateData->bChangeDisplayID = OMX_FALSE;
		}
	}

	if (ret == 1)
	{
		int  is_progressive, iaspect_ratio;
		ALOGD("Interlaced Info(%d,%d,%d,%d)",
			omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iInterlace,
			omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iM2vProgressiveFrame,
			omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iPictureStructure,
			omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iInterlacedFrame);

		/* Check interlaced/progressive frame */
		switch( omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iBitstreamFormat )
		{
			case STD_AVC :
				{
					if((omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iM2vProgressiveFrame == 0
			            && omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iPictureStructure == 0x3)
			                || omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iInterlacedFrame
						|| ((omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iPictureStructure  ==1)
						&& (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iInterlace ==0))
			                )
			        {
						if(omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iPictureStructure ==1)
							omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iTopFieldFirst = 1;

						is_progressive = 0;
					}
					else
					{
					    is_progressive = 1;
					}
				}
				break;
			case STD_MPEG2 :
			default :
				{
					if( omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iInterlace
			            || ( ( omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iM2vProgressiveFrame == 0
			                   && omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iPictureStructure == 0x3 )
			                 || omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iInterlacedFrame ) )
					{
						is_progressive = 0;
					}
					else
					{
						is_progressive = 1;
					}
				}
				break;
		}

		if( is_progressive )
		{
			ALOGD("NON-Interlaced Frame!!!");
		}
		else
		{
			ALOGD("Interlaced Frame!!!");
		}

		omx_private->iInterlaceInfo = is_progressive;

		//outPort->bIsPortChanged = OMX_TRUE;       //This make a trobule in DVBT Player.
#ifdef SET_FRAMEBUFFER_INTO_MAX
		if(omx_private->pVideoDecodInstance[uiDecoderID].gsVDecUserInfo.bMaxfbMode)
		{
			outPort->sPortParam.format.video.nFrameWidth = AVAILABLE_MAX_WIDTH;
			outPort->sPortParam.format.video.nFrameHeight = AVAILABLE_MAX_HEIGHT;
		}
		else
#endif
		{
			outPort->sPortParam.format.video.nFrameWidth = width;
			outPort->sPortParam.format.video.nFrameHeight = height;
		}

		iaspect_ratio = -1;
		if (omx_private->pVideoDecodInstance[uiDecoderID].video_coding_type == OMX_VIDEO_CodingMPEG2)
		{
			if(omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iAspectRateInfo)
			{
				iaspect_ratio = 0; // 16 : 9
				if(omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iAspectRateInfo == 2)
					iaspect_ratio = 1; // 4 : 3
			}
		}

		omx_private->stVideoDefinition.nAspectRatio = iaspect_ratio;
		ALOGD("[%d]Video Aspec Ratio %d (0[16:9], 1[4:3] :: %d)", uiDecoderID, iaspect_ratio, omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iAspectRateInfo);

		if (omx_private->iDisplayID == uiDecoderID) {
			(*(omx_private->callbacks->EventHandler)) (openmaxStandComp,
							omx_private->callbackData,
							OMX_EventVendorVideoChange,
							is_progressive, iaspect_ratio, outPort);
		}

		(*(omx_private->callbacks->EventHandler)) (openmaxStandComp,
						omx_private->callbackData,
						OMX_EventPortSettingsChanged,
						is_progressive, iaspect_ratio, outPort);

		omx_private->stVideoDefinition.nFrameWidth = width;
		omx_private->stVideoDefinition.nFrameHeight = height;
		omx_private->stVideoDefinition.nFrameRate = outPort->sPortParam.format.video.xFramerate;
		omx_private->stVideoDefinition.bProgressive = is_progressive;
		ALOGI ("ReSize Needed!! %d x %d -> %d x %d \n", (int)outPort->sPortParam.format.video.nFrameWidth,
			  (int)outPort->sPortParam.format.video.nFrameHeight, width, height);

		//adaption of DecoderID (for various resolution)
		if (uiDecoderID == 0){

			omx_private->stVideoDefinition_dual.nFrameWidth = width;
			omx_private->stVideoDefinition_dual.nFrameHeight = height;
			omx_private->stVideoDefinition_dual.MainDecoderID = uiDecoderID;
			omx_private->stVideoDefinition_dual.nAspectRatio= iaspect_ratio;
			omx_private->stVideoDefinition_dual.nFrameRate = outPort->sPortParam.format.video.xFramerate;

		} else {

			omx_private->stVideoDefinition_dual.nSubFrameWidth = width;
			omx_private->stVideoDefinition_dual.nSubFrameHeight = height;
			omx_private->stVideoDefinition_dual.SubDecoderID = uiDecoderID;
			omx_private->stVideoDefinition_dual.nSubAspectRatio= iaspect_ratio;

		}
		omx_private->stVideoDefinition_dual.DisplayID = omx_private->iDisplayID;

		if(!omx_private->Resolution_Change)
		{
#if 0 //def TCC_VIDEO_DISPLAY_BY_VSYNC_INT
			if (isVsyncEnabled(omx_private, uiDecoderID) == 1)
			{
				tcc_vsync_command(omx_private, TCC_LCDC_VIDEO_START_VSYNC, VPU_BUFF_COUNT);
			}
#endif
		}

		if (omx_private->stVideoDefinition_dual.MainDecoderID == omx_private->stVideoDefinition_dual.SubDecoderID) {

				(*(omx_private->callbacks->EventHandler)) (openmaxStandComp,
								omx_private->callbackData,
											OMX_EventVendorVideoUserDataAvailable,
											OMX_BUFFERFLAG_EXTRADATA, 0, &omx_private->stVideoDefinition);

		} else {

				(*(omx_private->callbacks->EventHandler)) (openmaxStandComp,
								omx_private->callbackData,
											OMX_EventVendorVideoUserDataAvailable,
											OMX_BUFFERFLAG_EXTRADATA, 0, &omx_private->stVideoDefinition_dual);
		}


		ALOGW ("[%d]VideoSize %d x %d -> %d x %d \n", uiDecoderID, (int)omx_private->stVideoDefinition.nFrameWidth,
			  (int)omx_private->stVideoDefinition.nFrameHeight, width, height);

	}

#ifdef SET_FRAMEBUFFER_INTO_MAX
		if (bCropChanged && CheckCrop_DecodedOut(openmaxStandComp,	uiDecoderID, left, top, width, height) > 0)
		{
			(*(omx_private->callbacks->EventHandler))(
						   openmaxStandComp,
						   omx_private->callbackData,
						   OMX_EventPortSettingsChanged,
						   OMX_DirOutput,
						   OMX_IndexConfigCommonOutputCrop,
						   (OMX_PTR)&omx_private->pVideoDecodInstance[uiDecoderID].rectCropParm);
		}
#endif

		if (ret != 1) {

			if (omx_private->stVideoDefinition_dual.MainDecoderID != omx_private->stVideoDefinition_dual.SubDecoderID) {

				if (omx_private->stVideoDefinition_dual.DisplayID != (OMX_U32)omx_private->iDisplayID) {

					omx_private->stVideoDefinition_dual.DisplayID = omx_private->iDisplayID;
					(*(omx_private->callbacks->EventHandler)) (openmaxStandComp,
											omx_private->callbackData,
														OMX_EventVendorVideoUserDataAvailable,
														OMX_BUFFERFLAG_EXTRADATA, 0, &omx_private->stVideoDefinition_dual);
					ALOGW ("send event check (when seamless switching)\n");
				}
			}
		}

	return ret;
}

static void dxb_omx_videodec_component_videoframedelay_set(OMX_COMPONENTTYPE * openmaxStandComp, OMX_U8 uiDecoderID)
{
	omx_videodec_component_PrivateType *omx_private = openmaxStandComp->pComponentPrivate;
	OMX_S32 stc_delay[4];
	int	decoder_delay, vsync_buffering_time;

	ALOGE("%s %d uiDecoderID =%d, m_iMinFrameBufferCount =%d \n", __func__, __LINE__, uiDecoderID, omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iMinFrameBufferCount);

		decoder_delay = omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iMinFrameBufferCount * \
				omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iPTSInterval;
		ALOGD("[%d]Video Decoder Delay %d us : [min buffer:%d, interval:%d(us)]", uiDecoderID, decoder_delay,\
                                                        omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iMinFrameBufferCount,\
                                                        omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iPTSInterval);
		vsync_buffering_time = omx_private->pVideoDecodInstance[uiDecoderID].max_fifo_cnt * \
				omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iPTSInterval;
		ALOGD("[%d]Video Vsync Buffering Time %d us : [max buffer:%d, interval:%d(us)]", uiDecoderID, vsync_buffering_time,\
                                                        omx_private->pVideoDecodInstance[uiDecoderID].max_fifo_cnt,\
                                                        omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iPTSInterval);

		stc_delay[0] = 0; // TYPE : 0 - VIDEO, 1 - AUDIO
		stc_delay[1] = uiDecoderID; // ID
		stc_delay[2] = decoder_delay/1000;
		stc_delay[3] = vsync_buffering_time / 1000;
		(*(omx_private->callbacks->EventHandler)) (openmaxStandComp,
						omx_private->callbackData,
						OMX_EventVendorSetSTCDelay,
						0, 0, (OMX_PTR) stc_delay);
}

static int
disp_pic_info_sort( void* pParam1 )
{
	int i;
	dec_disp_info_ctrl_t  *pInfoCtrl = (dec_disp_info_ctrl_t*)pParam1;
	int usedIdx, startIdx, regIdx;

	usedIdx=0;
	startIdx = -1;
	for( i=0 ; i<32 ; i++ )
	{
		if( pInfoCtrl->m_iRegIdxPTS[i] > -1 )
		{
			if( startIdx == -1 )
			{
				startIdx = i;
			}
			usedIdx++;
		}
	}
	pInfoCtrl->m_iUsedIdxPTS = usedIdx;

	if( usedIdx > 0 )
	{
		regIdx = 0;
		for( i=startIdx ; i<32 ; i++ )
		{
			if( pInfoCtrl->m_iRegIdxPTS[i] > -1 )
			{
				if( i != regIdx )
				{
					void * pswap;
					int iswap;

					iswap = pInfoCtrl->m_iRegIdxPTS[regIdx];
					pswap = pInfoCtrl->m_pRegInfoPTS[regIdx];

					pInfoCtrl->m_iRegIdxPTS[regIdx] = pInfoCtrl->m_iRegIdxPTS[i];
					pInfoCtrl->m_pRegInfoPTS[regIdx] = pInfoCtrl->m_pRegInfoPTS[i];

					pInfoCtrl->m_iRegIdxPTS[i] = iswap;
					pInfoCtrl->m_pRegInfoPTS[i] = pswap;
				}
				regIdx++;
				if( regIdx == usedIdx )
					break;
			}
		}
	}

	return usedIdx;
}

static void
disp_pic_info (int Opcode, void *pParam1, void *pParam2, void *pParam3,
			   omx_videodec_component_PrivateType * omx_private, OMX_U8 uiDecoderID)
{
	int       i;
	dec_disp_info_ctrl_t *pInfoCtrl = (dec_disp_info_ctrl_t *) pParam1;
	dec_disp_info_t *pInfo = (dec_disp_info_t *) pParam2;
	dec_disp_info_input_t *pInfoInput = (dec_disp_info_input_t *) pParam3;

	switch (Opcode)
	{
	case CVDEC_DISP_INFO_INIT:	//init.
		pInfoCtrl->m_iStdType = pInfoInput->m_iStdType;
		pInfoCtrl->m_iFmtType = pInfoInput->m_iFmtType;
		pInfoCtrl->m_iTimeStampType = pInfoInput->m_iTimeStampType;

#ifdef TS_TIMESTAMP_CORRECTION
        omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iLatestPTS = 0;
        omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iDecodedRealPTS = 0;
        if(omx_private->pVideoDecodInstance[uiDecoderID].cdmx_info.m_sVideoInfo.m_iFrameRate!=0)
            omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iPTSInterval = (((1000 * 1000) << 10) / omx_private->pVideoDecodInstance[uiDecoderID].cdmx_info.m_sVideoInfo.m_iFrameRate) >> 10;
#endif


	case CVDEC_DISP_INFO_RESET:	//reset
		for (i = 0; i < 32; i++)
		{
			pInfoCtrl->m_iRegIdxPTS[i] = -1;	//unused
			pInfoCtrl->m_pRegInfoPTS[i] = (void *) &pInfo[i];
		}
		pInfoCtrl->m_iUsedIdxPTS = 0;

		if (pInfoCtrl->m_iTimeStampType == CDMX_DTS_MODE)	//Decode Timestamp (Decode order)
		{
			pInfoCtrl->m_iDecodeIdxDTS = 0;
			pInfoCtrl->m_iDispIdxDTS = 0;
			for (i = 0; i < 32; i++)
			{
				pInfoCtrl->m_iDTS[i] = 0;
			}
		}

#ifdef EXT_V_DECODER_TR_TEST
		memset (&gsEXT_F_frame_time, 0, sizeof (EXT_F_frame_time_t));
		gsextReference_Flag = 1;
		gsextP_frame_cnt = 0;
#endif
#ifdef TS_TIMESTAMP_CORRECTION
        omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iLatestPTS = 0;
        omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iDecodedRealPTS = 0;
		omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iRealPTS = 0;
		omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iInterpolationCount = 0;
#endif
		break;

	case CVDEC_DISP_INFO_UPDATE:	//update
		{
			int       iDecodedIdx;
			int       usedIdx, startIdx, regIdx;
			dec_disp_info_t *pdec_disp_info;

			iDecodedIdx = pInfoInput->m_iFrameIdx;

			//save the side info.
			usedIdx = iDecodedIdx;
			if(pInfoCtrl->m_iRegIdxPTS[usedIdx] != -1)
			{
				ALOGE("Invalid Display Index [%d][%d] !!!", uiDecoderID, usedIdx);
			}

			pInfoCtrl->m_iRegIdxPTS[usedIdx] = iDecodedIdx;
			pdec_disp_info = (dec_disp_info_t *) pInfoCtrl->m_pRegInfoPTS[usedIdx];

			pdec_disp_info->m_iTimeStamp = pInfo->m_iTimeStamp;
			pdec_disp_info->m_iFrameType = pInfo->m_iFrameType;
			pdec_disp_info->m_iPicStructure = pInfo->m_iPicStructure;
			pdec_disp_info->m_iextTimeStamp = pInfo->m_iextTimeStamp;
			pdec_disp_info->m_iM2vFieldSequence = pInfo->m_iM2vFieldSequence;
			pdec_disp_info->m_iFrameDuration = pInfo->m_iFrameDuration;
			pdec_disp_info->m_iFrameSize = pInfo->m_iFrameSize;
			pdec_disp_info->m_iTopFieldFirst = pInfo->m_iTopFieldFirst;
			pdec_disp_info->m_iIsProgressive = pInfo->m_iIsProgressive;
			pdec_disp_info->m_iWidth = pInfo->m_iWidth;
			pdec_disp_info->m_iHeight = pInfo->m_iHeight;

			if (pInfoCtrl->m_iTimeStampType == CDMX_DTS_MODE)	//Decode Timestamp (Decode order)
			{
				if (iDecodedIdx >= 0 || (iDecodedIdx == -2 && pInfoCtrl->m_iStdType == STD_MPEG4))
				{
					pInfoCtrl->m_iDTS[pInfoCtrl->m_iDecodeIdxDTS] = pInfo->m_iTimeStamp;
					pInfoCtrl->m_iDecodeIdxDTS = (pInfoCtrl->m_iDecodeIdxDTS + 1) & 31;
				}
			}
#ifdef TS_TIMESTAMP_CORRECTION
			if (omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iDecodedRealPTS == pdec_disp_info->m_iTimeStamp)
				pdec_disp_info->m_iTimeStamp = 0;
			else
				omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iDecodedRealPTS = pdec_disp_info->m_iTimeStamp;
#endif
		}
		break;
	case CVDEC_DISP_INFO_GET:	//display
		{
			dec_disp_info_t **pInfo = (dec_disp_info_t **) pParam2;
			int       dispOutIdx = pInfoInput->m_iFrameIdx;

			//Presentation Timestamp (Display order)
			{
				*pInfo = 0;

				for (i = 0; i < 32; i++)
				{
					if (dispOutIdx == pInfoCtrl->m_iRegIdxPTS[i])
					{
						*pInfo = (dec_disp_info_t *) pInfoCtrl->m_pRegInfoPTS[i];
#ifdef TS_TIMESTAMP_CORRECTION
						#if 0
						ALOGE("m_iFrameType = %d, m_iTimeStamp = %d, m_iLatestPTS = %d, m_iRealPTS = %d, m_iInterpolationCount = %d",
								(*pInfo)->m_iFrameType,
								(*pInfo)->m_iTimeStamp,
								omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iLatestPTS,
								omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iRealPTS,
								omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iInterpolationCount);
						#endif
						if(
							((*pInfo)->m_iTimeStamp == 0)
							|| ((*pInfo)->m_iTimeStamp == omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iRealPTS)
						)
						{
							omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iInterpolationCount++;
							(*pInfo)->m_iTimeStamp = omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iRealPTS
					                     + omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iInterpolationCount * omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iPTSInterval/1000;
							//ALOGE("PTS Inter. %d intv %d cnt %d\n", (*pInfo)->m_iTimeStamp, omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iPTSInterval, omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iInterpolationCount );
						}
						else
						{
							if ((*pInfo)->m_iTimeStamp < omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iRealPTS)
							{
								ALOGE("Invalid Time Stamp (Previous = %d, Current = %d)", omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iRealPTS, (*pInfo)->m_iTimeStamp);
							}
							else if ((*pInfo)->m_iTimeStamp < omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iLatestPTS)
							{
								ALOGE("TS Correction Error (Previous = %d, Current = %d", omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iLatestPTS, (*pInfo)->m_iTimeStamp);
							}
							omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iInterpolationCount = 0;
							omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iRealPTS = (*pInfo)->m_iTimeStamp;
							//ALOGE("PTS First. %d \n", (*pInfo)->m_iTimeStamp );
						}

						omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iLatestPTS = (*pInfo)->m_iTimeStamp;
#endif
						//ALOGE("=====> %d", (*pInfo)->m_iTimeStamp);
						pInfoCtrl->m_iRegIdxPTS[i] = -1;	//unused
						break;
					}
				}
			}

			if (pInfoCtrl->m_iTimeStampType == CDMX_DTS_MODE)	//Decode Timestamp (Decode order)
			{
				if (*pInfo != 0)
				{
					(*pInfo)->m_iTimeStamp = (*pInfo)->m_iextTimeStamp = pInfoCtrl->m_iDTS[pInfoCtrl->m_iDispIdxDTS];
					pInfoCtrl->m_iDispIdxDTS = (pInfoCtrl->m_iDispIdxDTS + 1) & 31;
				}
			}
		}
		break;
	}

	return;
}

#ifdef VIDEO_USER_DATA_PROCESS
static void GetUserDataListFromCDK( unsigned char *pUserData, video_userdata_list_t *pUserDataList )
{
	unsigned int i;
	unsigned char * pTmpPTR;
	unsigned char * pRealData;
	unsigned int nNumUserData;
	unsigned int nTotalSize;
	unsigned int nDataSize;

	memset( pUserDataList, 0, sizeof(video_userdata_list_t) );

	pTmpPTR = pUserData;
	nNumUserData = (pTmpPTR[0] << 8) | pTmpPTR[1];
	nTotalSize = (pTmpPTR[2] << 8) | pTmpPTR[3];

	pTmpPTR = pUserData + 8;
	pRealData = pUserData + (8 * 17);

	if( nNumUserData > MAX_USERDATA )
	{
		DBUG_MSG("[vdec_userdata] user data num is over max. %d\n", nNumUserData );
		nNumUserData = MAX_USERDATA;
	}

	for(i = 0;i < nNumUserData;i++)
	{
		nDataSize = (pTmpPTR[2] << 8) | pTmpPTR[3];

		/* */
		pUserDataList->arUserData[i].pData = pRealData;
		pUserDataList->arUserData[i].iDataSize = nDataSize;

		pTmpPTR += 8;
		pRealData += nDataSize;
	}

	/* */
	pUserDataList->iUserDataNum = nNumUserData;
}

static void process_user_data( OMX_IN  OMX_COMPONENTTYPE *h_component, video_userdata_list_t *pUserDataList )
{
	omx_videodec_component_PrivateType *p_priv = h_component->pComponentPrivate;

	unsigned long long ullPTS = (pUserDataList->ullPTS)*9/100;
	unsigned char fDiscontinuity = pUserDataList->fDiscontinuity;

	int i;

	for( i=0; i<pUserDataList->iUserDataNum; i++ )
	{
		video_userdata_t *pUserData = &pUserDataList->arUserData[i];

		OMX_U32 args[5];

		args[0] = pUserData->pData;
		args[1] = pUserData->iDataSize;
		args[2] = (OMX_U32)(pUserDataList->ullPTS>>32);
		args[3] = (OMX_U32)(pUserDataList->ullPTS&0xFFFFFFFF);
		args[4] = pUserDataList->fDiscontinuity;

		(*(p_priv->callbacks->EventHandler))(
			h_component,
			p_priv->callbackData,
			OMX_EventVendorVideoUserDataAvailable,
			2,
			3,
			&args);

	}
}

static void print_user_data (unsigned char *pUserData)
{
	int       i, j;
	unsigned char *pTmpPTR;
	unsigned char *pRealData;
	unsigned int nNumUserData;
	unsigned int nTotalSize;
	unsigned int nDataSize;

	pTmpPTR = pUserData;
	nNumUserData = (pTmpPTR[0] << 8) | pTmpPTR[1];
	nTotalSize = (pTmpPTR[2] << 8) | pTmpPTR[3];

	pTmpPTR = pUserData + 8;
	pRealData = pUserData + (8 * 17);

	DBUG_MSG ("\n***User Data Print***\n");
	for (i = 0; i < nNumUserData; i++)
	{
		nDataSize = (pTmpPTR[2] << 8) | pTmpPTR[3];
		DBUG_MSG ("[User Data][Idx : %02d][Size : %05d]", i, nDataSize);
		for (j = 0; j < nDataSize; j++)
		{
			DBUG_MSG ("%02x ", pRealData[j]);
		}
		pTmpPTR += 8;
		pRealData += nDataSize;
	}
}
#endif //VIDEO_USER_DATA_PROCESS

static char * print_pic_type (int iVideoType, int iPicType, int iPictureStructure)
{
	switch (iVideoType)
	{
	case STD_MPEG2:
		if (iPicType == PIC_TYPE_I)
			return "I :";
		else if (iPicType == PIC_TYPE_P)
			return "P :";
		else if (iPicType == PIC_TYPE_B)
			return "B :";
		else
			return "D :";	//D_TYPE
	case STD_MPEG4:
		if (iPicType == PIC_TYPE_I)
			return "I :";
		else if (iPicType == PIC_TYPE_P)
			return "P :";
		else if (iPicType == PIC_TYPE_B)
			return "B :";
		else if (iPicType == PIC_TYPE_B_PB)	//MPEG-4 Packed PB-frame
			return "pB:";
		else
			return "S :";	//S_TYPE4
	default:
		if (iPicType == PIC_TYPE_I)
			return "I :";
		else if (iPicType == PIC_TYPE_P)
			return "P :";
		else if (iPicType == PIC_TYPE_B)
			return "B :";
		else
			return "U :";	//Unknown
	}
}

#if 1

#define FRAME_DROP_CNT			5

static int field_decoding_process(omx_videodec_component_PrivateType* omx_private, unsigned int decoderID, unsigned int fieldInfo)
{
	TCC_VDEC_PRIVATE *pPrivateData = (TCC_VDEC_PRIVATE*) omx_private->pPrivateData;

	if(omx_private->stIPTVActiveModeInfo.IPTV_PLAYMode)
	{
		if(omx_private->stIPTVActiveModeInfo.IPTV_Activemode == 2)
		{
			pPrivateData->isRefFrameBroken = OMX_FALSE;
		}
		else if(fieldInfo == 1 || fieldInfo == 2)
		{
			omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iOutputStatus == VPU_DEC_OUTPUT_FAIL;
			return -1;
		}
	}

	return 0;
}

static int for_iptv_trickModeEnd_process(omx_videodec_component_PrivateType* omx_private, unsigned int decoderID)
{
	int	frame_drop_flag = 0;

	if(omx_private->stIPTVActiveModeInfo.IPTV_Old_Activemode ==2 && omx_private->stIPTVActiveModeInfo.IPTV_Activemode ==1
		&& omx_private->stIPTVActiveModeInfo.IPTV_Mode_change_cnt<FRAME_DROP_CNT)  //for trick mode end
	{
		if((omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iPicType &0x00000003) == PIC_TYPE_B
			&& (omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iDecodingStatus == VPU_DEC_SUCCESS
			||omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iDecodingStatus == VPU_DEC_SUCCESS_FIELD_PICTURE))
		{
			omx_private->stbframe_skipinfo.bframe_skipindex[omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iDecodedIdx]
				= omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iDecodedIdx;
			omx_private->stbframe_skipinfo.bframe_skipcnt++;
//			ALOGE("%s %d omx_private->bframe_skipinfo.bframe_skipcnt = %d \n", __func__, __LINE__, omx_private->stbframe_skipinfo.bframe_skipcnt);
//			ALOGE("%s %d m_iDecodedIdx = %d \n", __func__, __LINE__, omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iDecodedIdx);
		}
		if(omx_private->stbframe_skipinfo.bframe_skipcnt>0
			&& omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iOutputStatus == VPU_DEC_OUTPUT_SUCCESS)
		{
			int i;
			for (i=0; i<32; i++)
			{
				if(omx_private->stbframe_skipinfo.bframe_skipindex[i] == (unsigned int)omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iDispOutIdx)
				{
					frame_drop_flag = -1;
					omx_private->stbframe_skipinfo.bframe_skipindex[i] = 0x100;
					omx_private->stbframe_skipinfo.bframe_skipcnt--;
					omx_private->stIPTVActiveModeInfo.IPTV_Mode_change_cnt++;
//					ALOGE("%s %d frame drop m_iDispOutIdx = %d  \n", __func__, __LINE__, omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iDispOutIdx);
//					ALOGE("%s %d omx_private->bframe_skipinfo.bframe_skipcnt= %d \n", __func__, __LINE__, omx_private->stbframe_skipinfo.bframe_skipcnt);
					break;
				}
			}
		}
	}
	return frame_drop_flag;
}


static int for_iptv_brokenframe_check(omx_videodec_component_PrivateType* omx_private, unsigned int decoderID)
{
	TCC_VDEC_PRIVATE *pPrivateData = (TCC_VDEC_PRIVATE*) omx_private->pPrivateData;
	int ret=0;

//	ALOGE("%s %d m_iNumOfErrMBs = %x \n", __func__, __LINE__, omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iNumOfErrMBs);

	if(omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iNumOfErrMBs >0 || pPrivateData->isRefFrameBroken == OMX_TRUE)
	{
		omx_private->stbroken_frame_info.broken_frame_index[omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iDecodedIdx]
			= omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iDecodedIdx;
		omx_private->stbroken_frame_info.broken_frame_iPicType[omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iDecodedIdx]
			= (unsigned char)(omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iPicType);
		omx_private->stbroken_frame_info.broken_frame_iNumOfErrMBs[omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iDecodedIdx]
			= omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iNumOfErrMBs;
		omx_private->stbroken_frame_info.broken_frame_cnt++;

//		ALOGE("%s %d omx_private->stbroken_frame_info.broken_frame_cnt = %d \n", __func__, __LINE__, omx_private->stbroken_frame_info.broken_frame_cnt);
//		ALOGE("%s %d m_iDecodedIdx = %d, m_iNumOfErrMBs = %d \n", __func__, __LINE__, omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iDecodedIdx, omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iNumOfErrMBs);
//		ALOGE("%s %d broken_iDecodedIdx = %x , broken_iNumOfErrMBs=%d \n", __func__, __LINE__, omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iDecodedIdx, omx_private->broken_frame_info.broken_frame_iNumOfErrMBs[omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iDecodedIdx] );
	}

//	ALOGE("%s %d m_iDispOutIdx = %d \n", __func__, __LINE__, omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDispOutIdx);
//	ALOGE("%s %d isRefFrameBroken = %d \n", __func__, __LINE__, pPrivateData->isRefFrameBroken);
	if(omx_private->stbroken_frame_info.broken_frame_cnt >0 && pPrivateData->isRefFrameBroken != OMX_TRUE)
	{
		int i=0;
		for (i=0; i<32; i++)
		{
			if(omx_private->stbroken_frame_info.broken_frame_index[i] == (unsigned int)omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iDispOutIdx
			&& omx_private->stbroken_frame_info.broken_frame_iNumOfErrMBs[i] != 0)
			{
				if(omx_private->stbroken_frame_info.broken_frame_iPicType[i] != PIC_TYPE_B)
				{
					pPrivateData->isRefFrameBroken = OMX_TRUE;
				}
				break;
			}
		}
	}
	else if(pPrivateData->isRefFrameBroken)
	{
		int i=0;
		for (i=0; i<32; i++)
		{
			if(omx_private->stbroken_frame_info.broken_frame_index[i] == (unsigned int)omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iDispOutIdx)
			{
				if(omx_private->stbroken_frame_info.broken_frame_iPicType[i] == PIC_TYPE_I )
				{
					pPrivateData->isRefFrameBroken = OMX_FALSE;
				}
			//	ALOGE("%s %d isRefFrameBroken = %d \n", __func__, __LINE__, pPrivateData->isRefFrameBroken);
			//	ALOGE("%s %d m_iPicType = %x, broken_frame_cnt = %d \n", __func__, __LINE__, iPicType, omx_private->stbroken_frame_info.broken_frame_cnt);
				break;
			}
		}
	}

	return ret;
}

static int for_iptv_brokenframe_process(omx_videodec_component_PrivateType* omx_private, unsigned int decoderID, unsigned int xFramerate)
{
	TCC_VDEC_PRIVATE *pPrivateData = (TCC_VDEC_PRIVATE*) omx_private->pPrivateData;
	int	ret =0;

	if(pPrivateData->isRefFrameBroken)
	{
		if(omx_private->stbroken_frame_info.broken_frame_cnt>xFramerate)
			pPrivateData->isRefFrameBroken =0;
		ret =-1;
	}

//	ALOGE("%s %d isRefFrameBroken = %d \n", __func__, __LINE__, pPrivateData->isRefFrameBroken);
//	ALOGE("%s %d broken_frame_cnt = %d \n", __func__, __LINE__, omx_private->stbroken_frame_info.broken_frame_cnt);

	if(omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iOutputStatus == VPU_DEC_OUTPUT_SUCCESS
		&& omx_private->stbroken_frame_info.broken_frame_cnt >0)
	{
		int i;
		for (i=0; i<32; i++)
		{
			if(omx_private->stbroken_frame_info.broken_frame_index[i] == (unsigned int)omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iDispOutIdx)
			{
				omx_private->stbroken_frame_info.broken_frame_index[i] = 0x100;
				omx_private->stbroken_frame_info.broken_frame_iNumOfErrMBs[i] = 0;
				omx_private->stbroken_frame_info.broken_frame_cnt--;
				ret  = -1;
	//			ALOGE("%s %d frame drop m_iDispOutIdx = %d  \n", __func__, __LINE__, omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDispOutIdx);
	//			ALOGE("%s %d omx_private->broken_frame_info.broken_frame_cnt = %d \n", __func__, __LINE__, omx_private->stbroken_frame_info.broken_frame_cnt);
	//			ALOGE("%s %d i = %d, broken_iDecodedIdx = %x , broken_iNumOfErrMBs=%d \n", __func__, __LINE__, i, omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodedIdx, omx_private->stbroken_frame_info.broken_frame_iNumOfErrMBs[i] );
	//			ALOGE("%s %d frame drop m_iDispOutIdx = %d  \n", __func__, __LINE__, omx_private->broken_frame_info.broken_frame_iPicType[omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodedIdx]);
				break;
			}
		}
	}

	return ret;
}

static int for_iptv_ChannelChange_process(OMX_COMPONENTTYPE * openmaxStandComp, unsigned int decoderID, unsigned int iPicType)
{
	omx_videodec_component_PrivateType *omx_private = openmaxStandComp->pComponentPrivate;
	int ret=0, Display_fieldInfo;

	Display_fieldInfo = ((omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iPicType& (1 << 15)) >> 15);
#if 0
	ALOGE("%s %d m_iPicType =%d \n", __func__, __LINE__, omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iPicType);
	ALOGE("%s %d Display_fieldInfo =%d \n", __func__, __LINE__, Display_fieldInfo);
	ALOGE("%s %d iPicType =%d \n", __func__, __LINE__, iPicType);
	ALOGE("%s %d FirstFrame_DispStatus =%d \n", __func__, __LINE__, omx_private->stfristframe_dsipInfo.FirstFrame_DispStatus);
	ALOGE("%s %d IPTV_Old_Activemode =%d, IPTV_Activemode = %d  \n", __func__, __LINE__, omx_private->stIPTVActiveModeInfo.IPTV_Old_Activemode, omx_private->stIPTVActiveModeInfo.IPTV_Activemode);
#endif

	if(omx_private->stIPTVActiveModeInfo.IPTV_Old_Activemode == 0 && omx_private->stIPTVActiveModeInfo.IPTV_Activemode==1)		//for IPTV channel change
	{
#if 0
		if(omx_private->stbroken_frame_info.broken_frame_cnt >0 && pPrivateData->isRefFrameBroken != OMX_TRUE)
		{
			for (i=0; i<32; i++)
			{
				if(omx_private->stbroken_frame_info.broken_frame_index[i] == omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDispOutIdx)
				{
					if((((omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iPicType &VIDEO_PICTYPE_MASK)<<4)>>4)!= PIC_TYPE_B)
						pPrivateData->isRefFrameBroken = OMX_TRUE;

					break;
				}
			}
			ALOGE("%s %d isRefFrameBroken = %d \n", __func__, __LINE__, pPrivateData->isRefFrameBroken);
		}
#endif
		if(omx_private->stfristframe_dsipInfo.FirstFrame_DispStatus==0)
		{
			if(iPicType!= PIC_TYPE_B &&
				(omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iDecodingStatus == VPU_DEC_SUCCESS
				||omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iDecodingStatus == VPU_DEC_SUCCESS_FIELD_PICTURE))
			{
				if(iPicType == PIC_TYPE_I)
				{
					if(Display_fieldInfo == 1) // top-field decoding
						omx_private->stfristframe_dsipInfo.TopFrame_DecStatus = 1;
					else
						omx_private->stfristframe_dsipInfo.BottomFrame_DecStatus = 1;
				}
				else if(iPicType == PIC_TYPE_P)
				{
					if(Display_fieldInfo == 1) // top-field decoding
						omx_private->stfristframe_dsipInfo.TopFrame_DecStatus = 1;
					else
						omx_private->stfristframe_dsipInfo.BottomFrame_DecStatus = 1;
				}

				if(omx_private->stfristframe_dsipInfo.TopFrame_DecStatus == 1 && omx_private->stfristframe_dsipInfo.BottomFrame_DecStatus == 1)
				{
					omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iOutputStatus = VPU_DEC_OUTPUT_SUCCESS;
					omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iDispOutIdx = 0;
					omx_private->stfristframe_dsipInfo.FirstFrame_DispStatus = 1;
					ret =1;
					ALOGE("%s %d first frame display \n", __func__, __LINE__);
					(*(omx_private->callbacks->EventHandler)) (openmaxStandComp, omx_private->callbackData,OMX_EventVendorVideoFirstDisplay, 0, 0, NULL);
				}
			}
			else if(omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iDecodingStatus == VPU_DEC_SUCCESS
				&& iPicType == PIC_TYPE_B)
			{
				omx_private->stfristframe_dsipInfo.FirstFrame_DispStatus = 1;
				omx_private->stfristframe_dsipInfo.TopFrame_DecStatus = 0;
				omx_private->stfristframe_dsipInfo.BottomFrame_DecStatus = 0;
			}
		}
		else if(omx_private->pVideoDecodInstance[decoderID].gsVDecOutput.m_DecOutInfo.m_iOutputStatus == VPU_DEC_OUTPUT_SUCCESS)
		{
	//		ALOGE("%s %d m_DecOutInfo.m_iDispOutIdx =%d \n", __func__, __LINE__, omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDispOutIdx);
			if(omx_private->stIPTVActiveModeInfo.IPTV_Mode_change_cnt<FRAME_DROP_CNT)
			{
				ret = -1;
				ALOGE("%s %d frame drop \n", __func__, __LINE__);
				omx_private->stIPTVActiveModeInfo.IPTV_Mode_change_cnt++;
			}
			else if(omx_private->stIPTVActiveModeInfo.IPTV_Mode_change_cnt ==FRAME_DROP_CNT)
			{
				omx_private->stIPTVActiveModeInfo.IPTV_Mode_change_cnt++;
			}
			omx_private->stfristframe_dsipInfo.TopFrame_DecStatus = 0;
			omx_private->stfristframe_dsipInfo.BottomFrame_DecStatus = 0;
			omx_private->stfristframe_dsipInfo.FirstFrame_DispStatus=1;
		}
	}

	return ret;
}

#endif

#define CODETYPE_NONE		(0x00000000)
#define CODETYPE_HEADER		(0x00000001)
#define CODETYPE_PICTURE	(0x00000002)
#define CODETYPE_ALL		(CODETYPE_HEADER | CODETYPE_PICTURE)

static OMX_U32 SearchCodeType(omx_videodec_component_PrivateType* omx_private, OMX_U32 *input_offset, OMX_U32 search_option, OMX_U8 uiDecoderID)
{
	unsigned int uiCode = 0xFFFFFFFF;
	OMX_U32 temp_input_offset = *input_offset;
	OMX_U32 code_type = CODETYPE_NONE;

	if (OMX_VIDEO_CodingMPEG2 == omx_private->pVideoDecodInstance[uiDecoderID].video_coding_type)
	{
		unsigned int SEQUENCE_HEADER = 0x000001B3;
		unsigned int PICTURE_START = 0x00000100;

		for (; temp_input_offset < omx_private->inputCurrLength; temp_input_offset++)
		{
			uiCode = (uiCode << 8) | omx_private->inputCurrBuffer[temp_input_offset];
			if ((search_option & CODETYPE_HEADER) && (uiCode == SEQUENCE_HEADER))
			{
				code_type = CODETYPE_HEADER;
				temp_input_offset -= 3;
				break;
			}
			if ((search_option & CODETYPE_PICTURE) && (uiCode == PICTURE_START))
			{
				code_type = CODETYPE_PICTURE;
				temp_input_offset -= 3;
				break;
			}
		}
	}
	else if (OMX_VIDEO_CodingAVC == omx_private->pVideoDecodInstance[uiDecoderID].video_coding_type)
	{
		unsigned int MASK = 0xFFFFFF1F;
		unsigned int P_FRAME = 0x00000101;
		unsigned int I_FRAME = 0x00000105;
		unsigned int SPS = 0x00000107;
		unsigned int uiMask;

		for (; temp_input_offset < omx_private->inputCurrLength; temp_input_offset++)
		{
			uiCode = (uiCode << 8) | omx_private->inputCurrBuffer[temp_input_offset];
			uiMask = uiCode & MASK;
			if ((search_option & CODETYPE_HEADER) && (uiMask == SPS))
			{
				code_type = CODETYPE_HEADER;
				temp_input_offset -= 3;
				break;
			}
			if ((search_option & CODETYPE_PICTURE) && ((uiMask == P_FRAME) || (uiMask == I_FRAME)))
			{
				code_type = CODETYPE_PICTURE;
				temp_input_offset -= 3;
				break;
			}
		}
	}
	*input_offset = temp_input_offset;

	return code_type;
}

static OMX_U32 CheckVideoStart(OMX_COMPONENTTYPE * openmaxStandComp, OMX_U8 uiDecoderID)
{
	OMX_U8 ucCount, i;
	OMX_U32 ulVideoStart;
	OMX_U32 ulVideoFormat;
	omx_videodec_component_PrivateType *omx_private = openmaxStandComp->pComponentPrivate;
	omx_base_video_PortType *outPort;
	TCC_VDEC_PRIVATE *pPrivateData = (TCC_VDEC_PRIVATE*) omx_private->pPrivateData;
	int protect = 0;

    if(omx_private->pVideoDecodInstance[uiDecoderID].stVideoStart.nState != TCC_DXBVIDEO_OMX_Dec_None)
    {
        ulVideoStart = omx_private->pVideoDecodInstance[uiDecoderID].stVideoStart.nState;
        ulVideoFormat = omx_private->pVideoDecodInstance[uiDecoderID].stVideoStart.nFormat;
        omx_private->pVideoDecodInstance[uiDecoderID].stVideoStart.nState = TCC_DXBVIDEO_OMX_Dec_None;
        //omx_private->pVideoDecodInstance[uiDecoderID].stVideoStart.nFormat = 0;

        pthread_mutex_lock(&omx_private->pVideoDecodInstance[uiDecoderID].stVideoStart.mutex);
        if(ulVideoStart == TCC_DXBVIDEO_OMX_Dec_Stop)
        {
			vdec_init_error_flag = 0;
			vdec_seq_header_error_flag = 0;
			vdec_decode_error_flag = 0;
			vdec_buffer_full_error_flag = 0;
			vdec_buffer_clear_error_flag = 0;
			vdec_no_input_error_flag = 0;

            if (uiDecoderID==0) outPort = (omx_base_video_PortType *) omx_private->ports[OMX_DXB_VIDEOCOMPONENT_OUTPORT];
            else outPort = (omx_base_video_PortType *) omx_private->ports[OMX_DXB_VIDEOCOMPONENT_OUTPORT_SUB];
            outPort->sPortParam.format.video.nFrameWidth = 0;
            outPort->sPortParam.format.video.nFrameHeight = 0;
            if (omx_private->pVideoDecodInstance[uiDecoderID].avcodecReady)
            {
                dxb_omx_videodec_component_LibDeinit (omx_private, uiDecoderID);
                omx_private->pVideoDecodInstance[uiDecoderID].avcodecReady = OMX_FALSE;
            }

            omx_private->pVideoDecodInstance[uiDecoderID].bVideoStarted = OMX_FALSE;
            omx_private->pVideoDecodInstance[uiDecoderID].buffer_unique_id = UNIQUE_ID_OF_FIRST_FRAME;
            omx_private->pVideoDecodInstance[uiDecoderID].iStreamID += MAX_INSTANCE_CNT;
            omx_private->pVideoDecodInstance[uiDecoderID].nDisplayedFirstFrame = 0;
            omx_private->pVideoDecodInstance[uiDecoderID].nDisplayedLastFrame = 0;

			if(omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance != NULL) {
				dxb_vdec_release_instance((void*)omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance);
				omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance = NULL;
#if 1 //Leak modification for videodecode component
				while(1){
					t_Vpu_BufferCearFrameInfo *ClearFrameInfo;
					ClearFrameInfo = (t_Vpu_BufferCearFrameInfo *)dxb_dequeue(omx_private->pVideoDecodInstance[uiDecoderID].pVPUDisplayedClearQueue);
					if(ClearFrameInfo){
						TCC_fo_free(__func__,__LINE__, ClearFrameInfo);
					}

					protect++;
					if(protect == 10)  //max 10ms
						break;

					if(ClearFrameInfo == NULL)
						break;
					usleep(1000);
				}
#endif
			}

            DBUG_MSG("%s - TCC_DXBVIDEO_OMX_Dec_Stop\n", __func__);
        }
        else if(ulVideoStart == TCC_DXBVIDEO_OMX_Dec_Start)
        {
			if(omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance == NULL) {
				omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance = dxb_vdec_alloc_instance();
			}

            if(uiDecoderID==omx_private->iDisplayID)
                pPrivateData->isRenderer = OMX_TRUE;

            if (omx_private->pVideoDecodInstance[uiDecoderID].avcodecReady)
            {
                if(uiDecoderID==0) outPort = (omx_base_video_PortType *) omx_private->ports[OMX_DXB_VIDEOCOMPONENT_OUTPORT];
                else outPort = (omx_base_video_PortType *) omx_private->ports[OMX_DXB_VIDEOCOMPONENT_OUTPORT_SUB];
                outPort->sPortParam.format.video.nFrameWidth = 0;
                outPort->sPortParam.format.video.nFrameHeight = 0;
                dxb_omx_videodec_component_LibDeinit (omx_private, uiDecoderID);
                omx_private->pVideoDecodInstance[uiDecoderID].avcodecReady = OMX_FALSE;
            }

            if (!omx_private->pVideoDecodInstance[uiDecoderID].avcodecReady)
            {
                dxb_omx_videodec_component_LibInit (omx_private, uiDecoderID);
                omx_private->pVideoDecodInstance[uiDecoderID].avcodecReady = OMX_TRUE;
            }

            if(ulVideoFormat == TCC_DXBVIDEO_OMX_Codec_H264)
            {
                if(omx_private->pVideoDecodInstance[uiDecoderID].video_coding_type != OMX_VIDEO_CodingAVC)
                    dxb_omx_videodec_component_Change(openmaxStandComp, VIDEO_DEC_H264_NAME, uiDecoderID);
            }
            else
            {
                if(omx_private->pVideoDecodInstance[uiDecoderID].video_coding_type != OMX_VIDEO_CodingMPEG2)
                    dxb_omx_videodec_component_Change(openmaxStandComp, VIDEO_DEC_MPEG2_NAME, uiDecoderID);
            }
            omx_private->pVideoDecodInstance[uiDecoderID].video_dec_idx = 0;
            omx_private->pVideoDecodInstance[uiDecoderID].bVideoStarted = OMX_TRUE;
            DBUG_MSG("%s - TCC_DXBVIDEO_OMX_Dec_Start, Format=0x%lx\n", __func__, ulVideoFormat);
        }
        pthread_mutex_unlock(&omx_private->pVideoDecodInstance[uiDecoderID].stVideoStart.mutex);
    }
    return 0;
}

static int SetIFrameSearch(omx_videodec_component_PrivateType *omx_private, OMX_U8 uiDecoderID, OMX_BOOL bEnable)
{
	if (bEnable == OMX_TRUE)
	{
		omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_iSkipFrameNum = 1;
		if(omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iBitstreamFormat == STD_MPEG2)
		{
			omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_iFrameSearchEnable = 0x001;
		}
		else
		{
//			omx_private->pVideoDecodInstance[omx_private->iDisplayID].gsVDecInput.m_iFrameSearchEnable = 0x001;	//I-frame (IDR-picture for H.264)
			omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_iFrameSearchEnable = 0x201;	//I-frame (I-slice for H.264) : Non IDR-picture
		}
		omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_iSkipFrameMode = VDEC_SKIP_FRAME_EXCEPT_I;

		SET_FLAG(omx_private->pVideoDecodInstance[uiDecoderID], STATE_SKIP_OUTPUT_B_FRAME);
	}
	else
	{
		omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_iSkipFrameMode = VDEC_SKIP_FRAME_DISABLE;
		omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_iFrameSearchEnable = 0;

		CLEAR_FLAG(omx_private->pVideoDecodInstance[uiDecoderID], STATE_SKIP_OUTPUT_B_FRAME);
	}
	return 0;
}

#ifdef SUPPORT_PVR
static void SetPVRFlags(omx_videodec_component_PrivateType *omx_private, OMX_U8 uiDecoderID, OMX_BUFFERHEADERTYPE * pOutputBuffer)
{
	if (omx_private->pVideoDecodInstance[uiDecoderID].nPVRFlags & PVR_FLAG_START)
	{
		pOutputBuffer->nFlags |= OMX_BUFFERFLAG_FILE_PLAY;

		if (omx_private->pVideoDecodInstance[uiDecoderID].nPVRFlags & PVR_FLAG_TRICK)
		{
			pOutputBuffer->nFlags |= OMX_BUFFERFLAG_TRICK_PLAY;
		}
		else
		{
			pOutputBuffer->nFlags &= ~OMX_BUFFERFLAG_TRICK_PLAY;
		}
	}
	else
	{
		pOutputBuffer->nFlags &= ~OMX_BUFFERFLAG_FILE_PLAY;
		pOutputBuffer->nFlags &= ~OMX_BUFFERFLAG_TRICK_PLAY;
	}
}

static int ResetFrame(OMX_COMPONENTTYPE * openmaxStandComp, OMX_U8 uiDecoderID)
{
	omx_videodec_component_PrivateType *omx_private = openmaxStandComp->pComponentPrivate;
#ifdef TS_TIMESTAMP_CORRECTION
		omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iLatestPTS = 0;
		omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iDecodedRealPTS = 0;
		omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iRealPTS = 0;
		omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iInterpolationCount = 0;
#endif

	if (omx_private->stPreVideoInfo.iDecod_status == VPU_DEC_SUCCESS_FIELD_PICTURE)
	{
		return 2; // VPU Reset
	}

#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
	if (isVsyncEnabled(omx_private, uiDecoderID) == 1)
	{
		tcc_vsync_command(omx_private, TCC_LCDC_VIDEO_SKIP_FRAME_START, 0); // clear vpu buffer
	}
#endif

	disp_pic_info(CVDEC_DISP_INFO_RESET,(void*)&(omx_private->pVideoDecodInstance[uiDecoderID].dec_disp_info_ctrl),
										(void*)omx_private->pVideoDecodInstance[uiDecoderID].dec_disp_info,
										(void*)&(omx_private->pVideoDecodInstance[uiDecoderID].dec_disp_info_input),
										omx_private,
										uiDecoderID);

#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
	if (isVsyncEnabled(omx_private, uiDecoderID) == 1)
	{
		while(omx_private->pVideoDecodInstance[uiDecoderID].used_fifo_count > 1)
		{
			if(0 > clear_front_vpu_buffer(openmaxStandComp, &omx_private->pVideoDecodInstance[uiDecoderID]))
			{
				tcc_vsync_command(omx_private, TCC_LCDC_VIDEO_SKIP_FRAME_END, 0);
				return 2; // VPU Reset
			}
		}
		tcc_vsync_command(omx_private, TCC_LCDC_VIDEO_SKIP_FRAME_END, 0);
	}
#endif

	SetIFrameSearch(omx_private, uiDecoderID, OMX_TRUE); // I-Frame Search

	omx_private->pVideoDecodInstance[uiDecoderID].nDelayedOutputBufferCount = 0;
	omx_private->pVideoDecodInstance[uiDecoderID].nSkipOutputBufferCount = 0;

	return 0;
}

static int CheckVPUState(omx_videodec_component_PrivateType *omx_private)
{
	if (omx_private->stPreVideoInfo.iDecod_status == VPU_DEC_SUCCESS_FIELD_PICTURE)
	{
		return 0; // Frame Decoding
	}
	return 1; // Skip Frame
}

static int CheckPVR(OMX_COMPONENTTYPE * openmaxStandComp, OMX_U8 uiDecoderID, OMX_U32 ulInputBufferFlags)
{
	omx_videodec_component_PrivateType *omx_private = openmaxStandComp->pComponentPrivate;
	OMX_U32 iPVRState, iBufferState;

	iPVRState = (omx_private->pVideoDecodInstance[uiDecoderID].nPVRFlags & PVR_FLAG_START) ? 1 : 0;
	iBufferState = (ulInputBufferFlags & OMX_BUFFERFLAG_FILE_PLAY) ? 1 : 0;
	if (iPVRState != iBufferState)
	{
		return CheckVPUState(omx_private); // skip
	}

	iPVRState = (omx_private->pVideoDecodInstance[uiDecoderID].nPVRFlags & PVR_FLAG_TRICK) ? 1 : 0;
	iBufferState = (ulInputBufferFlags & OMX_BUFFERFLAG_TRICK_PLAY) ? 1 : 0;
	if (iPVRState != iBufferState)
	{
		return CheckVPUState(omx_private); // skip
	}

	if (omx_private->pVideoDecodInstance[uiDecoderID].nPVRFlags & PVR_FLAG_CHANGED)
	{
		omx_private->pVideoDecodInstance[uiDecoderID].nPVRFlags &= ~PVR_FLAG_CHANGED;

		if (omx_private->pVideoDecodInstance[uiDecoderID].avcodecInited)
		{
			if (ResetFrame(openmaxStandComp, uiDecoderID))
			{
				return 2; // VPU Reset
			}
		}
	}

	return 0; // Decoding
}
#endif//SUPPORT_PVR

static void dxb_omx_displayClearqueue_init(omx_videodec_component_PrivateType *omx_private, OMX_U8 uiDecoderID)
{
	int i=0, queue_cnt =0;
	_VIDEO_DECOD_INSTANCE_ *pVDecInst;
	t_Vpu_BufferCearFrameInfo *ClearFrameInfo;
	pVDecInst = &omx_private->pVideoDecodInstance[uiDecoderID];

	queue_cnt = dxb_getquenelem(pVDecInst->pVPUDisplayedClearQueue);
	for(i=0; i<queue_cnt; i++)
	{
		ClearFrameInfo = (t_Vpu_BufferCearFrameInfo *)dxb_dequeue(pVDecInst->pVPUDisplayedClearQueue);
		TCC_fo_free(__func__, __LINE__, ClearFrameInfo);
	}
}

static void *dxb_omx_video_dec_component_vpu_deleteindex(void* param)
{
	t_Vpu_ClearThreadArg* pArg = (t_Vpu_ClearThreadArg*) param;
	OMX_COMPONENTTYPE* openmaxStandComp = (OMX_COMPONENTTYPE*)pArg->openmaxStandComp;
	omx_base_component_PrivateType* omx_base_component_Private=(omx_base_component_PrivateType*)openmaxStandComp->pComponentPrivate;
	omx_base_filter_PrivateType* omx_base_filter_Private = (omx_base_filter_PrivateType*)omx_base_component_Private;
	omx_videodec_component_PrivateType *omx_private = openmaxStandComp->pComponentPrivate;
	TCC_VDEC_PRIVATE *pPrivateData = (TCC_VDEC_PRIVATE*) omx_private->pPrivateData;
	_VIDEO_DECOD_INSTANCE_ *pVDecInst;
	t_Vpu_BufferCearFrameInfo *ClearFrameInfo;
	int	clear_buff_count =0, buffer_id =0, ret =0;
	unsigned int	curr_time =0;
	int i=0, queue_cnt =0;
	int iDecoderID = pArg->iDecoderID;
	OMX_S32 iStreamID;
	OMX_U32 nVsyncEnable;
	pVDecInst = &omx_private->pVideoDecodInstance[iDecoderID];

	ALOGE("%s %d iDecoderID =%d, state =%d,  transientState =%d \n", __func__, __LINE__, iDecoderID, omx_base_filter_Private->state, omx_base_filter_Private->transientState);

	while((omx_base_filter_Private->state == OMX_StateIdle || omx_base_filter_Private->state == OMX_StateExecuting || omx_base_filter_Private->transientState == OMX_TransStateIdleToExecuting)
		&& pPrivateData->Vpu_BufferClearInfo[iDecoderID].VpuBufferClearThreadRunning == 1)
	{
		if (omx_private->state != OMX_StateExecuting)
		{
			usleep(5*1000);
			ALOGE("%s %d omx_private->state =%x \n", __func__, __LINE__, omx_private->state);
		}
		else
		{
			if((pVDecInst->bVideoStarted == OMX_FALSE )
				||(pVDecInst->stVideoStart.nState == TCC_DXBVIDEO_OMX_Dec_Stop)
				||(!pVDecInst->avcodecInited)
				||(pVDecInst->isVPUClosed == OMX_TRUE)
				)
			{
				usleep(5*1000);
			}
			else
			{
				queue_cnt = dxb_getquenelem(pVDecInst->pVPUDisplayedClearQueue);
				if(queue_cnt)
				{
					OMX_U32	nDecodeID;
					OMX_U32 buffer_unique_id;
					ClearFrameInfo = (t_Vpu_BufferCearFrameInfo *)dxb_dequeue(pVDecInst->pVPUDisplayedClearQueue);
					if(ClearFrameInfo == NULL)
					{
						ALOGE("%s %d dequeue fail\n", __func__, __LINE__);
						usleep(5*1000);
						continue;
					}

					/* Clear Displayed Index */
					nDecodeID = ClearFrameInfo->iDecoderID;
					iStreamID = ClearFrameInfo->iStreamID;
					buffer_unique_id = ClearFrameInfo->buffer_unique_id;
					nVsyncEnable = ClearFrameInfo->bVSyncEnabled;

					OMX_U32 nBUfferIndex = buffer_unique_id % VPU_REQUEST_BUFF_COUNT;
					OMX_U32 nBufferID = pVDecInst->Send_Buff_ID[nBUfferIndex];
					OMX_U32 nDevID = nBufferID >> 16;
					OMX_U32 nClearDispIndex = nBufferID & 0xFFFF;
					OMX_S32 iRet = 0;

					if(pVDecInst->iStreamID != iStreamID) {
						TCC_fo_free (__func__, __LINE__,ClearFrameInfo);
						continue;
					}

					if (nClearDispIndex == 0)
					{
						usleep(5*1000);
						TCC_fo_free (__func__, __LINE__,ClearFrameInfo);
						continue;
					}
#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
					if (nVsyncEnable)
					{
						iRet = ClearFrameInfo->ret;
					}
					else
					{
						iRet = -2;
						if (pVDecInst->nDisplayedFirstFrame < 1)
						{
							if (pVDecInst->iStreamID == iStreamID)
							{
								if (SendFirstFrameEvent(openmaxStandComp, pVDecInst) == 0)
								{
									ALOGD("Video Frame First Output nDecoderUID(%d)", (int)iStreamID);
								}
								//pVDecInst->nDisplayedFirstFrame++;
							}
						}
					}
					if (iRet >= 0)
					{
						if (nVsyncEnable)
						{
							int cleared_buff_id;
							pVDecInst->Display_index[pVDecInst->in_index] = nBufferID;
							pVDecInst->Display_Buff_ID[pVDecInst->in_index] = buffer_unique_id;
							pVDecInst->in_index = (pVDecInst->in_index + 1) % pVDecInst->max_fifo_cnt;
							pVDecInst->used_fifo_count++;
							tcc_vsync_command(omx_private,TCC_LCDC_VIDEO_GET_DISPLAYED, &cleared_buff_id) ;	// TCC_LCDC_HDMI_GET_DISPLAYED
							if(cleared_buff_id >= 0 && cleared_buff_id>(int)buffer_unique_id) {
								tcc_vsync_command(omx_private,TCC_LCDC_VIDEO_SKIP_ONE_FRAME, omx_private->pVideoDecodInstance[omx_private->iDisplayID].buffer_unique_id);
							}

							if ((pVDecInst->used_fifo_count+queue_cnt) >= pVDecInst->max_fifo_cnt)
							{
								clear_vpu_buffer(openmaxStandComp, pVDecInst, iStreamID);
							}
							TCC_fo_free(__func__, __LINE__, ClearFrameInfo);
							continue;
						}
					}
					else if (iRet == -2) // Clear all frame
					{
						while (pVDecInst->used_fifo_count > 0)
						{
							clear_front_vpu_buffer(openmaxStandComp, pVDecInst);
						}
					}
#endif
					//nClearDispIndex++; //CAUTION !!! to avoid NULL(0) data insertion.
					if (dxb_queue_ex(pVDecInst->pVPUDisplayedIndexQueue, (void*)nClearDispIndex) == 0)
					{
						//DPRINTF_DEC_STATUS("@ DispIdx Queue %d", pVDecInst->Display_index[pVDecInst->in_index]);
						nClearDispIndex--;
						iRet = pVDecInst->gspfVDec (VDEC_BUF_FLAG_CLEAR, NULL, &nClearDispIndex, NULL, pVDecInst->pVdec_Instance);
						ALOGE ("%s VDEC_BUF_FLAG_CLEAR[%u] Err : [%lu] index : %lu",__func__, (unsigned int)nDevID, iRet, nClearDispIndex);
					}
					TCC_fo_free (__func__, __LINE__,ClearFrameInfo);
				}
				else
					usleep(5*1000);
			}
		}
	}
	pPrivateData->Vpu_BufferClearInfo[iDecoderID].VpuBufferClearThreadRunning =-1;
	return (void*)NULL;
}

static int delete_status_check(omx_videodec_component_PrivateType *omx_private, OMX_U8 uiDecoderID)
{
	TCC_VDEC_PRIVATE *pPrivateData = (TCC_VDEC_PRIVATE*) omx_private->pPrivateData;
	int queue_cnt = 0;

	if(uiDecoderID == TCC_DXBVIDEO_DISPLAY_MAIN)
	{
		if(omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_SUB].avcodecInited == 0)
			usleep(10*1000);
		else
		{
			queue_cnt = dxb_getquenelem(omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_SUB].pVPUDisplayedClearQueue);
			if((omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_SUB].used_fifo_count+queue_cnt) >= omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_SUB].max_fifo_cnt)
				usleep(10*1000);
			else
			{
				if(pPrivateData->delete_vpu_displayed_info.uiPreVideo_DecoderID != (int)uiDecoderID)
					pPrivateData->delete_vpu_displayed_info.uiPreVideo_DecoderID_ChkCnt =0;
				else
					pPrivateData->delete_vpu_displayed_info.uiPreVideo_DecoderID_ChkCnt ++;

				if(pPrivateData->delete_vpu_displayed_info.uiPreVideo_DecoderID_ChkCnt>2)
					usleep(10*1000);
			}
		}
	}
	else
	{
		if(omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_MAIN].avcodecInited == 0)
			usleep(10*1000);
		else
		{
			queue_cnt = dxb_getquenelem(omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_MAIN].pVPUDisplayedClearQueue);
			if((omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_MAIN].used_fifo_count+queue_cnt) >= omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_MAIN].max_fifo_cnt)
				usleep(10*1000);
			else
			{
				if(pPrivateData->delete_vpu_displayed_info.uiPreVideo_DecoderID != (int)uiDecoderID)
					pPrivateData->delete_vpu_displayed_info.uiPreVideo_DecoderID_ChkCnt =0;
				else
					pPrivateData->delete_vpu_displayed_info.uiPreVideo_DecoderID_ChkCnt ++;

				if(pPrivateData->delete_vpu_displayed_info.uiPreVideo_DecoderID_ChkCnt>2)
					usleep(10*1000);
			}
		}
	}
	pPrivateData->delete_vpu_displayed_info.uiPreVideo_DecoderID = uiDecoderID;
	return 0;
}

static int delete_vpu_displayed_index(OMX_COMPONENTTYPE * openmaxStandComp, OMX_U8 uiDecoderID)
{
    omx_videodec_component_PrivateType *omx_private = openmaxStandComp->pComponentPrivate;
	int ret, i;
	int nClearDispIndex;

	for(i = 0; i < 32; i++ ) //Max 32 elements
	{
		nClearDispIndex = (int)dxb_dequeue(omx_private->pVideoDecodInstance[uiDecoderID].pVPUDisplayedIndexQueue);
		if( nClearDispIndex ==  0 )
			break;

		nClearDispIndex--; //CAUTION !!! nClearDispIndex is added with 1 before. We must do -1.
		if ((ret = omx_private->pVideoDecodInstance[uiDecoderID].gspfVDec (VDEC_BUF_FLAG_CLEAR, NULL, &nClearDispIndex, NULL, omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance)) < 0)
		{
			ALOGE ("%s VDEC_BUF_FLAG_CLEAR[%d] Err : [%d] index : %d",__func__, uiDecoderID, ret, nClearDispIndex);
		}
	}

	return 0;
}

static void set_cpu_clock(OMX_COMPONENTTYPE * openmaxStandComp, OMX_U8 uiDecoderID)
{
	ALOGD("set_cpu_clock");
    omx_videodec_component_PrivateType *omx_private = openmaxStandComp->pComponentPrivate;
	omx_base_video_PortType *outPort = (omx_base_video_PortType *) omx_private->ports[OMX_DXB_VIDEOCOMPONENT_OUTPORT];

	if (omx_private->gHDMIOutput || (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iPicWidth >= 1440 || outPort->sPortParam.format.video.nFrameHeight >= 1080))
    {
        dxb_vpu_update_sizeinfo (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iBitstreamFormat, 40, 30, AVAILABLE_MAX_WIDTH, AVAILABLE_MAX_HEIGHT, omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance);	//max-clock!!
		omx_private->pVideoDecodInstance[uiDecoderID].bitrate_mbps = 40; //set max
    }
    else if (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iPicWidth >= 720 || outPort->sPortParam.format.video.nFrameHeight >= 576)
    {
        dxb_vpu_update_sizeinfo (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iBitstreamFormat, 30, 30, AVAILABLE_MAX_WIDTH, AVAILABLE_MAX_HEIGHT, omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance);	//max-clock!!
        omx_private->pVideoDecodInstance[uiDecoderID].bitrate_mbps = 30;
    }
    else
    {
        dxb_vpu_update_sizeinfo (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iBitstreamFormat, 15, 30,
                             omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iPicWidth,
                             omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iPicHeight,
                             omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance);
        omx_private->pVideoDecodInstance[uiDecoderID].bitrate_mbps = 15;
    }
}

#if 0 //jktest
static int videocnt = 0;
#endif

/** This function is used to process the input buffer and provide one output buffer
  */
void dxb_omx_videodec_component_BufferMgmtCallback (OMX_COMPONENTTYPE * openmaxStandComp,
												OMX_BUFFERHEADERTYPE * pInputBuffer,
												OMX_BUFFERHEADERTYPE * pOutputBuffer)
{
	omx_videodec_component_PrivateType *omx_private = openmaxStandComp->pComponentPrivate;
	TCC_VDEC_PRIVATE *pPrivateData = (TCC_VDEC_PRIVATE*) omx_private->pPrivateData;
	OMX_S32   ret;
	OMX_S32   nDisplayIndex = -1, nOutputFrameWidth, nOutputFrameHeight;
	OMX_U8   *outputCurrBuffer, *pInputBufer;
	int       decode_result;
	int       i, iSeqHeaderOffset;
	unsigned int uiDemuxID = 0, uiDecoderID = 0;
	dec_disp_info_t dec_disp_info_tmp;
	OMX_U8 uiDisplayID = omx_private->iDisplayID;
	int Decoding_fieldInfo = -1;
	unsigned char iPicType;
	int      nDeocodedBufferOut = 0;

	omx_base_video_PortType *outPort;

//ALOGE("FILE_BUFFER_MODE");

	if (omx_private->state != OMX_StateExecuting)
	{
		ALOGE ("=> omx_private->state != OMX_StateExecuting");
		return;
	}

	if( memcmp(pInputBuffer->pBuffer,BUFFER_INDICATOR,BUFFER_INDICATOR_SIZE)==0)
	{
		memcpy(&pInputBufer, pInputBuffer->pBuffer + BUFFER_INDICATOR_SIZE, 4);
		if(pInputBuffer->pBuffer[BUFFER_INDICATOR_SIZE+4] == 0xAA)
		{
			uiDemuxID = pInputBuffer->pBuffer[BUFFER_INDICATOR_SIZE+5];
			uiDecoderID = pInputBuffer->pBuffer[BUFFER_INDICATOR_SIZE+6];
			//LOGD("%s:Demuxer[%d]Decoder[%d]",__func__, uiDemuxID, uiDecoderID);
		}
	}
	else
	{
		pInputBufer = pInputBuffer->pBuffer;
		uiDecoderID = pInputBuffer->pBuffer[pInputBuffer->nFilledLen];
	}

	if (uiDecoderID==0)
	{
		outPort = (omx_base_video_PortType *)omx_private->ports[OMX_DXB_VIDEOCOMPONENT_OUTPORT];
	}
	else
	{
		outPort = (omx_base_video_PortType *)omx_private->ports[OMX_DXB_VIDEOCOMPONENT_OUTPORT_SUB];
	}

	nOutputFrameWidth = outPort->sPortParam.format.video.nFrameWidth;
	nOutputFrameHeight = outPort->sPortParam.format.video.nFrameHeight;

	pthread_mutex_lock(&omx_private->pVideoDecodInstance[uiDecoderID].stVideoStart.mutex);

#ifdef  SUPPORT_PVR
	ret = CheckPVR(openmaxStandComp, uiDecoderID, pInputBuffer->nFlags);
	switch (ret)
	{
	case 1:
		pInputBuffer->nFilledLen = 0;
		pOutputBuffer->nFilledLen = 0;
		pthread_mutex_unlock(&omx_private->pVideoDecodInstance[uiDecoderID].stVideoStart.mutex);
		return;
	case 2:
		goto ERR_PROCESS;
	}
#endif//SUPPORT_PVR

//	CheckVideoStart(openmaxStandComp, uiDecoderID);

//	if(uiDecoderID == 1 && omx_private->pVideoDecodInstance[0].avcodecInited == 0)
/*
	if(uiDecoderID == 1 && omx_private->pVideoDecodInstance[0].isVPUClosed == OMX_TRUE && uiDisplayID == 0)
	{
		pInputBuffer->nFilledLen = 0;
		pOutputBuffer->nFilledLen = 0;
		pthread_mutex_unlock(&omx_private->pVideoDecodInstance[uiDecoderID].stVideoStart.mutex);
		return;
	}
*/
	if(omx_private->pVideoDecodInstance[uiDecoderID].bVideoStarted == OMX_FALSE ||
		(omx_private->pVideoDecodInstance[uiDecoderID].stVideoStart.nState == TCC_DXBVIDEO_OMX_Dec_Stop))
	{
		pInputBuffer->nFilledLen = 0;
		pOutputBuffer->nFilledLen = 0;
		pthread_mutex_unlock(&omx_private->pVideoDecodInstance[uiDecoderID].stVideoStart.mutex);
		return;
	}

	if(pInputBufer[0] != 0 || pInputBufer[1] != 0)
	{
		pInputBuffer->nFilledLen = 0;
		pOutputBuffer->nFilledLen = 0;
		ALOGE("This(%d) is not video frame [%x][%x] !!!", uiDecoderID, pInputBufer[0], pInputBufer[1]);
		pthread_mutex_unlock(&omx_private->pVideoDecodInstance[uiDecoderID].stVideoStart.mutex);
		return;
	}

#if 1	// for dual-decoding
	if((omx_private->stPreVideoInfo.iDecod_Support_Country == TCC_DXBVIDEO_ISSUPPORT_BRAZIL
		&& omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_MAIN].gsVDecOutput.m_DecOutInfo.m_iDecodingStatus == VPU_DEC_SUCCESS)
		&& (uiDisplayID == TCC_DXBVIDEO_DISPLAY_SUB && uiDecoderID ==TCC_DXBVIDEO_DISPLAY_MAIN))
	{
		pInputBuffer->nFilledLen = 0;
		pOutputBuffer->nFilledLen = 0;
		pthread_mutex_unlock(&omx_private->pVideoDecodInstance[uiDecoderID].stVideoStart.mutex);
		return;
	}
#endif

#if 1 //for dual decoding
	if(omx_private->stPreVideoInfo.iDecod_status == VPU_DEC_SUCCESS_FIELD_PICTURE)
	{
		if(omx_private->stPreVideoInfo.iDecod_Instance != (int)uiDecoderID)
		{
			unsigned int cur_time;
			cur_time = dbg_get_time();
			if (cur_time >= omx_private->stPreVideoInfo.uDecod_time)
				cur_time -= omx_private->stPreVideoInfo.uDecod_time;
			else
				cur_time = 0;
			if (cur_time >= VPU_FIELDPICTURE_PENDTIME_MAX) {
				ALOGE("%s %d Multi Instance Error need to reset \n", __func__, __LINE__);
				goto ERR_PROCESS;
			} else {
				long long llPTS = pInputBuffer->nTimeStamp/1000;
				long long llSTC = DxB_SYNC_OK;

				if (pPrivateData->gfnDemuxGetSTCValue != NULL) {
					llSTC = pPrivateData->gfnDemuxGetSTCValue (DxB_SYNC_VIDEO, llPTS, uiDecoderID, 0, pPrivateData->pvDemuxApp);
				}
				if (llSTC == DxB_SYNC_DROP){
					pInputBuffer->nFilledLen = 0;
				}

				pOutputBuffer->nFilledLen = 0;
				pthread_mutex_unlock(&omx_private->pVideoDecodInstance[uiDecoderID].stVideoStart.mutex);
				return;
			}
		} else {
			omx_private->stPreVideoInfo.uDecod_time = dbg_get_time();
		}
	}
	else
	{
		if (dxb_getquenelem(omx_private->ports[uiDecoderID]->pBufferQueue) < 2)
		{
			pOutputBuffer->nFilledLen = 0;
			pthread_mutex_unlock(&omx_private->pVideoDecodInstance[uiDecoderID].stVideoStart.mutex);
			return;
		}
	}
#endif

	/** Fill up the current input buffer when a new buffer has arrived */
	omx_private->inputCurrBuffer = pInputBufer;
	omx_private->inputCurrLength = pInputBuffer->nFilledLen;

	outputCurrBuffer = pOutputBuffer->pBuffer;
	pOutputBuffer->nFilledLen = 0;
	pOutputBuffer->nOffset = 0;
	pOutputBuffer->nDecodeID = uiDecoderID;

	if (omx_private->isFirstBuffer)
	{
		tsem_down (omx_private->avCodecSyncSem);
		omx_private->isFirstBuffer = 0;
	}

#if 0 //jktest
	if(1) {
		if(videocnt++ >= 100) {
			ALOGE("\033[101m###jktest video [0x%02X%02X%02X%02X_%02X%02X%02X%02X] len[%d]\033[0m",
			pInputBufer[0], pInputBufer[1], pInputBufer[2], pInputBufer[3], pInputBufer[4], pInputBufer[5], pInputBufer[6], pInputBufer[7],
			pInputBuffer->nFilledLen);
			videocnt = 0;
		}
		pInputBuffer->nFilledLen = 0;
		pOutputBuffer->nFilledLen = 0;
		pthread_mutex_unlock(&omx_private->pVideoDecodInstance[uiDecoderID].stVideoStart.mutex);
		return;
	}
#elif 0
	if(1) {
		if(videocnt++ >= 100) {
			ALOGE("\033[101m###jktest video [0x%02X%02X%02X%02X_%02X%02X%02X%02X] len[%d]\033[0m",
			pInputBufer[0], pInputBufer[1], pInputBufer[2], pInputBufer[3], pInputBufer[4], pInputBufer[5], pInputBufer[6], pInputBufer[7],
			pInputBuffer->nFilledLen);
			videocnt = 0;
		}
	}
#else
	;
#endif

	//////////////////////////////////////////////////////////////////////////////////////////
	/*ZzaU :: remove NAL-STart Code when there are double codes. ex) AVI container */
	if (!omx_private->pVideoDecodInstance[uiDecoderID].avcodecInited)
	{
		omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iPicWidth = outPort->sPortParam.format.video.nFrameWidth;
		omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iPicHeight = outPort->sPortParam.format.video.nFrameHeight;
		omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_bEnableVideoCache = 0;	//1;    // Richard_20100507 Don't use video cache
		omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_bEnableUserData = FLAG_ENABLE_USER_DATA;
		omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_pExtraData = NULL;//omx_private->extradata;
		omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iExtraDataLen = 0;//omx_private->extradata_size;

		omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_bM4vDeblk = 0;	//pCdk->m_bM4vDeblk;
		omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_uiDecOptFlags = 0;

		if(omx_private->stVideoSubFunFlag.SupportFieldDecoding)
			omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_uiDecOptFlags |= (1 << 3);  //filed decoding enable

		omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_uiMaxResolution = 0;	//pCdk->m_uiVideoMaxResolution;
		omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_bFilePlayEnable = 1;

		omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_bCbCrInterleaveMode = 1;      //YUV420 sp Interleave
		//omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_bCbCrInterleaveMode = 0;	//YUV420 sp
		{
			omx_private->pVideoDecodInstance[uiDecoderID].dec_disp_info_input.m_iStdType = omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iBitstreamFormat;
			omx_private->pVideoDecodInstance[uiDecoderID].dec_disp_info_input.m_iTimeStampType = CDMX_PTS_MODE;	// Presentation Timestamp (Display order)
			omx_private->pVideoDecodInstance[uiDecoderID].dec_disp_info_input.m_iFmtType = omx_private->pVideoDecodInstance[uiDecoderID].container_type;
			disp_pic_info (CVDEC_DISP_INFO_INIT, (void *) &(omx_private->pVideoDecodInstance[uiDecoderID].dec_disp_info_ctrl), \
							(void *) omx_private->pVideoDecodInstance[uiDecoderID].dec_disp_info, \
						   (void *) &(omx_private->pVideoDecodInstance[uiDecoderID].dec_disp_info_input), omx_private, uiDecoderID);
		}

		iSeqHeaderOffset = 0;
		if( SearchCodeType(omx_private, (OMX_U32*) &iSeqHeaderOffset, (OMX_U32)CODETYPE_HEADER, (OMX_U8)uiDecoderID) != CODETYPE_HEADER )
		{
			ALOGE ("[VDEC_INIT:%d]There are no sequence header !!!", uiDecoderID);
			pInputBuffer->nFilledLen = 0;
			pthread_mutex_unlock(&omx_private->pVideoDecodInstance[uiDecoderID].stVideoStart.mutex);
			return;
		}

		#if 1
		//ALOGE("Header Offer %d", iSeqHeaderOffset);
		omx_private->inputCurrBuffer += iSeqHeaderOffset;
		omx_private->inputCurrLength -= iSeqHeaderOffset;
		pInputBuffer->nFilledLen -= iSeqHeaderOffset;
		#endif

		if(omx_private->Resolution_Change == 0 && get_total_handle_cnt(openmaxStandComp)<1)
		{
//			omx_private->pVideoDecodInstance[uiDecoderID].gspfVDec (VDEC_HW_RESET, uiDecoderID, &omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit, &omx_private->pVideoDecodInstance[uiDecoderID].bitrate_mbps, omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance);
		}
		else if(omx_private->Resolution_Change == 1)
		{
			omx_private->Resolution_Change = 0;
		}

#ifdef SET_FRAMEBUFFER_INTO_MAX
		if(uiDecoderID == TCC_DXBVIDEO_DISPLAY_SUB)
		{
			omx_private->pVideoDecodInstance[uiDecoderID].gsVDecUserInfo.bMaxfbMode = 0;
		}
		else
		{
			omx_private->pVideoDecodInstance[uiDecoderID].gsVDecUserInfo.bMaxfbMode = 1;
			omx_private->pVideoDecodInstance[uiDecoderID].gsVDecUserInfo.m_iFrameWidth = AVAILABLE_MAX_WIDTH;
			omx_private->pVideoDecodInstance[uiDecoderID].gsVDecUserInfo.m_iFrameHeight = AVAILABLE_MAX_HEIGHT;
		}
#else
		omx_private->pVideoDecodInstance[uiDecoderID].gsVDecUserInfo.bMaxfbMode = 0;
#endif
		omx_private->pVideoDecodInstance[uiDecoderID].rectCropParm.nLeft	= 0;
		omx_private->pVideoDecodInstance[uiDecoderID].rectCropParm.nTop		= 0;
		omx_private->pVideoDecodInstance[uiDecoderID].rectCropParm.nWidth	= 0;
		omx_private->pVideoDecodInstance[uiDecoderID].rectCropParm.nHeight	= 0;

		if ((ret = omx_private->pVideoDecodInstance[uiDecoderID].gspfVDec (VDEC_INIT, uiDecoderID, &omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit, &omx_private->pVideoDecodInstance[uiDecoderID].gsVDecUserInfo, omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance)) < 0)
		{
			ALOGE ("[VDEC_INIT:%d] [Err:%ld] video decoder init", *((int*)omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance), ret);
			if(vdec_init_error_flag == 0){
				SendVideoErrorEvent(openmaxStandComp, VDEC_INIT_ERROR);
				vdec_init_error_flag = 1;
			}
			goto ERR_PROCESS;
		}

		omx_private->pVideoDecodInstance[uiDecoderID].isVPUClosed = OMX_FALSE;

		omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_pInp[PA] = dxb_vpu_getBitstreamBufAddr (PA, omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance);
		omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_pInp[VA] = dxb_vpu_getBitstreamBufAddr (VA, omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance);

		memcpy (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_pInp[VA], omx_private->inputCurrBuffer, omx_private->inputCurrLength);
		omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_iInpLen = omx_private->inputCurrLength;

		#ifdef VIDEO_USER_DATA_PROCESS
		if(omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_bEnableUserData)
		{
			omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_UserDataAddr[PA]    = dxb_vpu_getUserDataBufAddr(PA, omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance);
			omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_UserDataAddr[VA]    = dxb_vpu_getUserDataBufAddr(VA, omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance);
			omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_iUserDataBufferSize = dxb_vpu_getUserDataBufSize(omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance);
		}
		#endif

		dxb_vpu_set_additional_frame_count (6, (void*)omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance);

		if ((ret = omx_private->pVideoDecodInstance[uiDecoderID].gspfVDec (VDEC_DEC_SEQ_HEADER, NULL, &omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput, &omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput, omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance)) < 0)
		{
			if(vdec_seq_header_error_flag == 0){
				SendVideoErrorEvent(openmaxStandComp, VDEC_SEQ_HEADER_ERROR);
				vdec_seq_header_error_flag = 1;
			}

			if(ret == -RETCODE_INVALID_STRIDE || ret == -RETCODE_CODEC_SPECOUT || ret == -RETCODE_CODEC_EXIT || ret == -RETCODE_MULTI_CODEC_EXIT_TIMEOUT)
			{
				ALOGE("[%d]don't support the resolution", *((int*)omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance));
				pInputBuffer->nFilledLen = 0;
				pOutputBuffer->nFilledLen = 0;
				pthread_mutex_unlock(&omx_private->pVideoDecodInstance[uiDecoderID].stVideoStart.mutex);
				return;
			}
			ALOGE ("[VDEC_DEC_SEQ_HEADER:%d] [Err:%d] video decoder init", *((int*)omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance), ret);
			ALOGE ("IN-Buffer :: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x",
			  pInputBufer[0], pInputBufer[1], pInputBufer[2],
			  pInputBufer[3], pInputBufer[4], pInputBufer[5],
			  pInputBufer[6], pInputBufer[7], pInputBufer[8],
			  pInputBufer[9]);
			goto ERR_PROCESS;
		}

//		omx_private->pVideoDecodInstance[uiDecoderID].video_dec_idx = 0;
		omx_private->pVideoDecodInstance[uiDecoderID].max_fifo_cnt = VPU_BUFF_COUNT;

//		omx_private->max_fifo_cnt = dxb_vpu_get_frame_count(VPU_BUFF_COUNT, omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance);// - omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iMinFrameBufferCount;
//		if(omx_private->pVideoDecodInstance[uiDecoderID].max_fifo_cnt > VPU_BUFF_COUNT)
//			omx_private->pVideoDecodInstance[uiDecoderID].max_fifo_cnt = VPU_BUFF_COUNT;

		omx_private->pVideoDecodInstance[uiDecoderID].avcodecInited = 1;
		set_cpu_clock (openmaxStandComp, uiDecoderID);
#ifdef TS_TIMESTAMP_CORRECTION
        {
            unsigned int fRateInfoRes = omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_uiFrameRateRes;
            unsigned int fRateInfoDiv = omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_uiFrameRateDiv;

            omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iPTSInterval = 40000;
            outPort->sPortParam.format.video.xFramerate = 25;
            if(fRateInfoRes && fRateInfoDiv)
            {
                omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iPTSInterval = 1000000*fRateInfoDiv/fRateInfoRes;
				ALOGD("[%d]f_res %d f_div %d m-intr %d fdur %d\n", uiDecoderID, fRateInfoRes, fRateInfoDiv, omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iInterlace, omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iPTSInterval );

                if (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iBitstreamFormat == STD_AVC)
                {
                    /* !!! Be sure below checking is right or not. */
                    if(omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iInterlace != 1) //1:Frame, 0:Frame or Field
                        omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iPTSInterval = omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iPTSInterval*2;
                }

                ALOGD("%s:%d::[%d][%d]",__func__, __LINE__, omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iPTSInterval, omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iInterlace );
                if(omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iPTSInterval)
                {
                    unsigned int framerate_hz;
                    framerate_hz = (int)(1000000.0/omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iPTSInterval + 0.5);
                    ALOGD("%s:%d::FrameRate[%d]Hz",__func__, __LINE__, framerate_hz);
                    if(framerate_hz < 15)
                    {
                        framerate_hz = 15;
                        omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iPTSInterval = 66666;
                    }
                    if(framerate_hz>=60)
                    {
                        framerate_hz = 60;
                        omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iPTSInterval = 33000;
                    }
                    else
                    if(framerate_hz > 30 && framerate_hz != 50)
                    {
                        framerate_hz = 30;
                        omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iPTSInterval = 33000;
                    }

                    outPort->sPortParam.format.video.xFramerate = framerate_hz;
#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
                    if (isVsyncEnabled(omx_private, uiDecoderID) == 1)
                    {
                        tcc_vsync_command(omx_private,TCC_LCDC_VIDEO_SET_FRAMERATE, framerate_hz) ;  // TCC_LCDC_HDMI_GET_DISPLAYED
                    }
#endif
                }
                else
                {
                    omx_private->pVideoDecodInstance[uiDecoderID].gsTSPtsInfo.m_iPTSInterval = 40000;
                    outPort->sPortParam.format.video.xFramerate = 25;
                }
            }
        }
#endif

#ifdef  SUPPORT_PVR
		omx_private->pVideoDecodInstance[uiDecoderID].nDelayedOutputBufferCount = 0;
		omx_private->pVideoDecodInstance[uiDecoderID].nSkipOutputBufferCount = 0;
#endif//SUPPORT_PVR

#ifdef SUPPORT_SEARCH_IFRAME_ATSTARTING
		if(omx_private->stVideoSubFunFlag.SupportIFrameSearchMode)
		{
			SetIFrameSearch(omx_private, uiDecoderID, OMX_TRUE);
		}
		else
		{
			SetIFrameSearch(omx_private, uiDecoderID, OMX_FALSE);
		}
#else
		SetIFrameSearch(omx_private, uiDecoderID, OMX_FALSE);
#endif

		dxb_omx_videodec_component_videoframedelay_set(openmaxStandComp, uiDecoderID);

	}

#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
	if(pPrivateData->bRollback == OMX_TRUE && omx_private->iDisplayID == uiDecoderID)
	{
		goto ERR_PROCESS;
	}
#endif

	memcpy (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_pInp[VA], omx_private->inputCurrBuffer, omx_private->inputCurrLength);
	omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_iInpLen = omx_private->inputCurrLength;

	if (omx_private->pVideoDecodInstance[uiDecoderID].isVPUClosed == OMX_TRUE)
	{
		ALOGE ("Now VPU[%d] has been closed , return ", uiDecoderID);
		pInputBuffer->nFilledLen = 0;
		pthread_mutex_unlock(&omx_private->pVideoDecodInstance[uiDecoderID].stVideoStart.mutex);
		return;
	}

	delete_vpu_displayed_index(openmaxStandComp, uiDecoderID);
	if(omx_private->stPreVideoInfo.iDecod_status != VPU_DEC_SUCCESS_FIELD_PICTURE && isVsyncEnabled(omx_private, uiDecoderID))
	{
		int queue_cnt = 0;
		queue_cnt = dxb_getquenelem(omx_private->pVideoDecodInstance[uiDecoderID].pVPUDisplayedClearQueue);
		if(((omx_private->pVideoDecodInstance[uiDecoderID].used_fifo_count)+queue_cnt) >= omx_private->pVideoDecodInstance[uiDecoderID].max_fifo_cnt)
		{
			delete_status_check(omx_private, uiDecoderID);
			pOutputBuffer->nFilledLen = 0;
			pthread_mutex_unlock(&omx_private->pVideoDecodInstance[uiDecoderID].stVideoStart.mutex);
			return;
		}
	}

	if(omx_private->guiSkipBframeEnable)
	{
		//B-frame Skip Enable
		//ALOGE("[VDEC_DECODE]SKIP(B) START!!!");
		omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_iSkipFrameMode = VDEC_SKIP_FRAME_ONLY_B;
		omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_iSkipFrameNum = 1000;
	}

	if ((ret = omx_private->pVideoDecodInstance[uiDecoderID].gspfVDec (VDEC_DECODE, NULL, &omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput, &omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput, omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance)) < 0)
	{
		ALOGE ("[VDEC_DECODE:%d] [Err:%ld] video decode :: Len %d", uiDecoderID, ret, (int)omx_private->inputCurrLength);
		if(vdec_decode_error_flag == 0){
			SendVideoErrorEvent(openmaxStandComp, VDEC_DECODE_ERROR);
			vdec_decode_error_flag = 1;
		}
		goto ERR_PROCESS;
	}

#if 1	//for Dual-decoding
	omx_private->stPreVideoInfo.iDecod_Instance = uiDecoderID;
	omx_private->stPreVideoInfo.iDecod_status = omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodingStatus;
	omx_private->stPreVideoInfo.uDecod_time = dbg_get_time();
#endif

	if(omx_private->guiSkipBframeEnable)
	{
		if( (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iPicType == PIC_TYPE_B)
			&& (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodedIdx == -2)
			&& (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodingStatus == VPU_DEC_SUCCESS) )
		{
			if( omx_private->guiSkipBFrameNumber-- == 0)
			{
				//B-frame Skip Disable
				//ALOGE("[VDEC_DECODE]SKIP(B) END!!!");
				omx_private->guiSkipBframeEnable = 0;
				omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_iSkipFrameMode = VDEC_SKIP_FRAME_DISABLE;
				omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_iSkipFrameNum = 0;
			}
			//else
			//	ALOGE("[VDEC_DECODE]SKIPPING(B) %d!!!", omx_private->guiSkipBFrameNumber);
		}
	}

	if(omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_iFrameSearchEnable != 0
		&& omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodedIdx >= 0)
	{
		if(omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iBitstreamFormat == STD_MPEG2)
		{
			omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_iSkipFrameMode = VDEC_SKIP_FRAME_ONLY_B;
			omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_iSkipFrameNum = 1;
		}
		else
		{
			omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_iSkipFrameMode = VDEC_SKIP_FRAME_DISABLE;
			omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_iSkipFrameNum = 0;
		}
		omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_iFrameSearchEnable = 0;
		SET_FLAG(omx_private->pVideoDecodInstance[uiDecoderID], STATE_SKIP_OUTPUT_B_FRAME);
	}
	else
	{
		if(omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iBitstreamFormat == STD_MPEG2)
		{
			if(omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_iSkipFrameMode == VDEC_SKIP_FRAME_ONLY_B
				&& omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodedIdx >= 0 )
			{
				omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_iSkipFrameMode = VDEC_SKIP_FRAME_DISABLE;
				omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_iSkipFrameNum = 0;

				ALOGE("[CHANGE:%d] m_iBitstreamFormat[%d] m_iSkipFrameMode[%d] m_iSkipFrameNum[%d]", uiDecoderID,
					omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iBitstreamFormat,
					omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_iSkipFrameMode,
					omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInput.m_iSkipFrameNum);
			}
		}
	}

#if 0//ndef MOVE_VPU_IN_KERNEL
	if( omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iInvalidDispCount > 100 )
	{
		//VPU Close & Init
		//m_iPicSideInfo :: bit0(H.264 Decoded Picture Buffer error notification) bit1(H.264 weighted prediction)
		//if(omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iPicSideInfo & 0x01)
		//This is error case for abnormal case(such as frame drop...)
		ALOGE("[VDEC_DECODE:%d]Invalid Display !!! try to re-initialize", uiDecoderID);
		goto ERR_PROCESS;
	}
#endif
	//m_iPicSideInfo :: bit0(H.264 Decoded Picture Buffer error notification) bit1(H.264 weighted prediction)
	//ALOGE("[VDEC_DECODE]iDecodingStatus(%d)iPicSideInfo(0x%x)",omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodingStatus, omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iPicSideInfo);
	if (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodingStatus == VPU_DEC_BUF_FULL)
	{
		ALOGE ("[%d] VPU_DEC_BUF_FULL :: Len %d, OutputStatus(%d)", (int)uiDecoderID, (int)omx_private->inputCurrLength, (int)omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iOutputStatus);
		if (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iOutputStatus == VPU_DEC_OUTPUT_SUCCESS)
			decode_result = 0;
		else
		{
			omx_private->pVideoDecodInstance[uiDecoderID].guiDisplayBufFullCount++;
			ALOGD("%s:%d:[%d]",__func__, __LINE__, (int)omx_private->pVideoDecodInstance[uiDecoderID].guiDisplayBufFullCount);
			if(omx_private->pVideoDecodInstance[uiDecoderID].guiDisplayBufFullCount > 15 )
			{
			    omx_private->pVideoDecodInstance[uiDecoderID].guiDisplayBufFullCount = 0;
				ALOGE("[VDEC_DECODE]Too many VPU_DEC_BUF_FULL !!! try to re-initialize");
				if(vdec_buffer_full_error_flag == 0){
					SendVideoErrorEvent(openmaxStandComp, VDEC_BUFFER_FULL_ERROR);
					vdec_buffer_full_error_flag = 1;
				}
				goto ERR_PROCESS;
			}
			decode_result = 1;
		}
	}
	else
	{
		omx_private->pVideoDecodInstance[uiDecoderID].guiDisplayBufFullCount = 0;
		if (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iOutputStatus == VPU_DEC_OUTPUT_SUCCESS)
			decode_result = 2;
		else
			decode_result = 3;
	}

	//Update TimeStamp!!
	if (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodingStatus == VPU_DEC_SUCCESS
		&& omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodedIdx >= 0)
	{
		OMX_U32   width, height;

		dec_disp_info_tmp.m_iTimeStamp = (int) (pInputBuffer->nTimeStamp / 1000);
		dec_disp_info_tmp.m_iFrameType = omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iPicType;
		dec_disp_info_tmp.m_iPicStructure = omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iPictureStructure;
		dec_disp_info_tmp.m_iextTimeStamp = 0;
		dec_disp_info_tmp.m_iM2vFieldSequence = 0;
		dec_disp_info_tmp.m_iFrameSize = omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iConsumedBytes;	// gsCDmxOutput.m_iPacketSize;
		dec_disp_info_tmp.m_iFrameDuration = 2;
		dec_disp_info_tmp.m_iTopFieldFirst = 1;

		width = omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iWidth;
		height = omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iHeight;
		if (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iBitstreamFormat == STD_AVC)
		{
			width -= omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_CropInfo.m_iCropLeft;
			width -= omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_CropInfo.m_iCropRight;
			height -= omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_CropInfo.m_iCropBottom;
			height -= omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_CropInfo.m_iCropTop;
		}
		dec_disp_info_tmp.m_iWidth = width;
		dec_disp_info_tmp.m_iHeight = height;

		switch (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iBitstreamFormat)
		{
		int  is_progressive;
		case STD_MPEG2:
#if 1 //defined(TCC_VPU_C7_INCLUDE)
			//tcc_dxb_surface : if(m_iTopFieldFirst==0) frame_pmem_info->optional_info[7] |= OMX_BUFFERFLAG_ODD_FIRST_FRAME;
			if(dec_disp_info_tmp.m_iPicStructure == 3 || dec_disp_info_tmp.m_iPicStructure == 1)
			{
				if(omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iTopFieldFirst == 0)
                	dec_disp_info_tmp.m_iTopFieldFirst = 0;
			}
#else
			dec_disp_info_tmp.m_iTopFieldFirst = omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iTopFieldFirst;
#endif
			if (dec_disp_info_tmp.m_iPicStructure != 3)
			{
				dec_disp_info_tmp.m_iFrameDuration = 1;
			}
			else if (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iInterlace == 0)
			{
				if (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iRepeatFirstField == 0)
					dec_disp_info_tmp.m_iFrameDuration = 2;
				else
					dec_disp_info_tmp.m_iFrameDuration =
						(omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iTopFieldFirst == 0) ? 4 : 6;
			}
			else
			{
				/* interlaced sequence */
				if (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iInterlacedFrame == 0)
					dec_disp_info_tmp.m_iFrameDuration = 2;
				else
					dec_disp_info_tmp.m_iFrameDuration =
						(omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iRepeatFirstField == 0) ? 2 : 3;
			}
			if( omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iInterlace
				|| ( ( omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iM2vProgressiveFrame == 0
				&& omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iPictureStructure == 0x3 )
				|| omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iInterlacedFrame ) )
			{
				is_progressive = 0;
			}
			else
			{
				is_progressive = 1;
			}
			dec_disp_info_tmp.m_iIsProgressive = is_progressive;
			dec_disp_info_tmp.m_iM2vFieldSequence = omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iM2vFieldSequence;
			omx_private->pVideoDecodInstance[uiDecoderID].dec_disp_info_input.m_iFrameIdx = omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodedIdx;
			omx_private->pVideoDecodInstance[uiDecoderID].dec_disp_info_input.m_iFrameRate = omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iM2vFrameRate;
			disp_pic_info (CVDEC_DISP_INFO_UPDATE,
							(void *) &(omx_private->pVideoDecodInstance[uiDecoderID].dec_disp_info_ctrl),
							(void *) &dec_disp_info_tmp,
							(void *) &(omx_private->pVideoDecodInstance[uiDecoderID].dec_disp_info_input),
							omx_private,
							uiDecoderID);
			#ifdef VIDEO_USER_DATA_PROCESS
			{
				video_userdata_list_t stUserDataList;
				GetUserDataListFromCDK((unsigned char *)omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_UserDataAddress[VA], &stUserDataList);
				//DBUG_MSG("[userdata] put %2d, %10d\n", omx_private->gsVDecOutput[uiDecoderID].m_DecOutInfo.m_iDecodedIdx,(int)(curr_timestamp/1000) );
				UserDataCtrl_Put(omx_private->pVideoDecodInstance[uiDecoderID].pUserData_List_Array,
								omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodedIdx,
								pInputBuffer->nTimeStamp, 0, &stUserDataList);
			}
			#endif
			break;

		case STD_AVC:
		default:
			if((omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iM2vProgressiveFrame == 0
				&& omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iPictureStructure == 0x3)
				|| omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iInterlacedFrame
				|| ((omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iPictureStructure  ==1)
				&& (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iInterlace ==0))
			)
			{
				if(omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iPictureStructure == 1)
					omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iTopFieldFirst = 1;

				is_progressive = 0;
			}
			else
			{
				is_progressive = 1;
			}

			dec_disp_info_tmp.m_iIsProgressive = is_progressive;
			dec_disp_info_tmp.m_iM2vFieldSequence = 0;
			dec_disp_info_tmp.m_iTopFieldFirst = omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iTopFieldFirst;
			omx_private->pVideoDecodInstance[uiDecoderID].dec_disp_info_input.m_iFrameIdx = omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodedIdx;
			disp_pic_info (CVDEC_DISP_INFO_UPDATE, (void *) &(omx_private->pVideoDecodInstance[uiDecoderID].dec_disp_info_ctrl), (void *) &dec_disp_info_tmp,
						   (void *) &(omx_private->pVideoDecodInstance[uiDecoderID].dec_disp_info_input), omx_private, uiDecoderID);
			break;
		}
		DPRINTF_DEC ("IN-Buffer :: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x",
					 pInputBufer[0], pInputBufer[1], pInputBufer[2],
					 pInputBufer[3], pInputBufer[4], pInputBufer[5],
					 pInputBufer[6], pInputBufer[7], pInputBufer[8],
					 pInputBufer[9]);
		//current decoded frame info
		DPRINTF_DEC ("[In - %s][N:%4d][LEN:%6d][RT:%8d] [DecoIdx:%2d][DecStat:%d][FieldSeq:%d][TR:%8d] ",
					 print_pic_type (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iBitstreamFormat, dec_disp_info_tmp.m_iFrameType,
									 dec_disp_info_tmp.m_iPicStructure), omx_private->pVideoDecodInstance[uiDecoderID].video_dec_idx, pInputBuffer->nFilledLen,
					 (int) (pInputBuffer->nTimeStamp / 1000), omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodedIdx,
					 omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodingStatus,
					 omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iM2vFieldSequence,
					 omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iextTimeStamp);

		#ifdef  SUPPORT_PVR
		omx_private->pVideoDecodInstance[uiDecoderID].nDelayedOutputBufferCount++;
		#endif//SUPPORT_PVR
	}
	else
	{
		DPRINTF_DEC ("IN-Buffer :: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x",
			  pInputBufer[0], pInputBufer[1], pInputBufer[2],
			  pInputBufer[3], pInputBufer[4], pInputBufer[5],
			  pInputBufer[6], pInputBufer[7], pInputBufer[8],
			  pInputBufer[9]);
		DPRINTF_DEC ("[Err In - %s][N:%4d][LEN:%6d][RT:%8d] [DecoIdx:%2d][DecStat:%d][OutStat:%d][FieldSeq:%d][TR:%8d] ",
			  print_pic_type (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iBitstreamFormat, dec_disp_info_tmp.m_iFrameType,
							  dec_disp_info_tmp.m_iPicStructure), omx_private->pVideoDecodInstance[uiDecoderID].video_dec_idx, pInputBuffer->nFilledLen,
			  (int) (pInputBuffer->nTimeStamp / 1000), omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodedIdx,
			  omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodingStatus,
			  omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iOutputStatus,
			  omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iM2vFieldSequence,
			  omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iextTimeStamp);

		omx_private->pVideoDecodInstance[uiDecoderID].guiDisplayBufFullCount++;
		DPRINTF_DEC("%s:%d:[%d]",__func__, __LINE__, omx_private->pVideoDecodInstance[uiDecoderID].guiDisplayBufFullCount);
		if(omx_private->pVideoDecodInstance[uiDecoderID].guiDisplayBufFullCount > 100 )
		{
			omx_private->pVideoDecodInstance[uiDecoderID].guiDisplayBufFullCount = 0;
			ALOGE("[VDEC_DECODE:%d]Too many VPU_DEC_BUF_FULL !!! try to re-initialize", uiDecoderID);
			if(vdec_buffer_full_error_flag == 0){
				SendVideoErrorEvent(openmaxStandComp, VDEC_BUFFER_FULL_ERROR);
				vdec_buffer_full_error_flag = 1;
			}
			goto ERR_PROCESS;
		}
	}

#if 1
#if 0
	ALOGE("%s %d m_DecOutInfo.m_iDecodingStatus =%d \n", __func__, __LINE__, omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodingStatus);
	ALOGE("%s %d m_DecOutInfo.m_iOutputStatus =%d \n", __func__, __LINE__, omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iOutputStatus);
	ALOGE("%s %d decode_result = %d  \n", __func__, __LINE__, decode_result);
	ALOGE("%s %d m_DecOutInfo.m_iDecodedIdx =%d \n", __func__, __LINE__, omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodedIdx);
	ALOGE("%s %d m_DecOutInfo.m_iDispOutIdx =%d \n", __func__, __LINE__, omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDispOutIdx);
	ALOGE("%s %d m_iPicType =%x \n", __func__, __LINE__, omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iPicType);
	ALOGE("%s %d m_iNumOfErrMBs = %d \n", __func__, __LINE__, omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iNumOfErrMBs);
#endif
	if(omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iNumOfErrMBs>0)
	{
		ALOGE("[VID:%d] m_iNumOfErrMBs (%d) = %d \n", __LINE__, *((int*)omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance), omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iNumOfErrMBs);
	}

	Decoding_fieldInfo = ((omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iPicType& (3 << 16)) >> 16);
	iPicType =(unsigned char)(omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iPicType &VIDEO_PICTYPE_MASK);

	if(omx_private->stVideoSubFunFlag.SupportFieldDecoding)
	{
		ret = field_decoding_process(omx_private, uiDecoderID, Decoding_fieldInfo);
		if(ret !=0)
			decode_result =3;

		if(omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodingStatus == VPU_DEC_BUF_FULL
			&& omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iOutputStatus == VPU_DEC_OUTPUT_SUCCESS	)
			decode_result =0;
		else if(omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodingStatus == VPU_DEC_BUF_FULL
			&& omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iOutputStatus == VPU_DEC_OUTPUT_FAIL)
			decode_result =3;
	}

	if(omx_private->stIPTVActiveModeInfo.IPTV_PLAYMode)
	{
		ret = for_iptv_trickModeEnd_process(omx_private, uiDecoderID);
		if(ret == -1)
			decode_result =3;

		ret  = for_iptv_brokenframe_check(omx_private, uiDecoderID);
		ret  = for_iptv_brokenframe_process(omx_private, uiDecoderID, outPort->sPortParam.format.video.xFramerate);
		if(ret == -1)
			decode_result =3;
	}
#endif

	//////////////////////////////////////////////////////////////////////////////////////////
	if((omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodingStatus !=VPU_DEC_SUCCESS_FIELD_PICTURE) &&
		((omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iPicWidth != omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iWidth) ||
		(omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pInitialInfo->m_iPicHeight != omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iHeight)))
	{
		ALOGE("%s %d \n", __func__, __LINE__);
		ret = isPortChange (openmaxStandComp, uiDecoderID);
		if (ret < 0)	//max-resolution over!!
			goto ERR_PROCESS;
	}

#ifdef  SUPPORT_PVR
	if (omx_private->pVideoDecodInstance[uiDecoderID].nPVRFlags & PVR_FLAG_TRICK)
	{
		if (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodingStatus == VPU_DEC_SUCCESS &&
			omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodedIdx >= 0)
		{
			nDeocodedBufferOut = 1;
		}
	}
#endif//SUPPORT_PVR

	if (nDeocodedBufferOut || omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iOutputStatus == VPU_DEC_OUTPUT_SUCCESS/* && uiDisplayID == uiDecoderID*/)
	{
		ret = isPortChange (openmaxStandComp, uiDecoderID);
		if (ret < 0)	//max-resolution over!!
			goto ERR_PROCESS;

		int dispOutIdx = omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDispOutIdx;

		#ifdef  SUPPORT_PVR
		if (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iOutputStatus == VPU_DEC_OUTPUT_SUCCESS)
		{
			omx_private->pVideoDecodInstance[uiDecoderID].nDelayedOutputBufferCount--;
		}
		else
		{
			decode_result = 2;
			dispOutIdx = -1;
		}
		#endif//SUPPORT_PVR

		/* physical address */
		for (i = 0; i < 3; i++)
			memcpy (pOutputBuffer->pBuffer + i * 4, &omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pDispOut[PA][i], 4);

		/* logical address */
		for (i = 3; i < 6; i++)
			memcpy (pOutputBuffer->pBuffer + i * 4, &omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pDispOut[VA][i - 3], 4);

		if(omx_private->stVideoSubFunFlag.SupportFieldDecoding)
		{
			if(omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodingStatus == VPU_DEC_BUF_FULL)
				pOutputBuffer ->nFlags |= OMX_BUFFERFLAG_INTERLACED_ONLY_ONEFIELD_FRAME;
			else
				pOutputBuffer ->nFlags |= OMX_BUFFERFLAG_INTERLACED_FRAME;
		}

		/*Save Display out index in order to clear it at sendbufferfunction
		 */
		memcpy (pOutputBuffer->pBuffer + 24, &omx_private->pVideoDecodInstance[uiDecoderID].buffer_unique_id, 4);
		*(unsigned int *)(pOutputBuffer->pBuffer + 48) = isVsyncEnabled(omx_private, uiDecoderID); // VSync Enable
		// "+1" is meaningless. (main_unique_addr:1, sub_unique_addr:2)
		//*(unsigned int *)(pOutputBuffer->pBuffer + 52) = uiDecoderID + 1;
		*(unsigned int *)(pOutputBuffer->pBuffer + 52) = omx_private->pVideoDecodInstance[uiDecoderID].iStreamID;

		omx_private->pVideoDecodInstance[uiDecoderID].Send_Buff_ID[omx_private->pVideoDecodInstance[uiDecoderID].buffer_unique_id%VPU_REQUEST_BUFF_COUNT]
				= (uiDecoderID<<16) + dispOutIdx + 1;

		nDisplayIndex = dispOutIdx;
		//Get TimeStamp!!
		{
			dec_disp_info_t *pdec_disp_info = NULL;
			if (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iOutputStatus == VPU_DEC_OUTPUT_SUCCESS)
			{
				omx_private->pVideoDecodInstance[uiDecoderID].dec_disp_info_input.m_iFrameIdx = dispOutIdx;
				disp_pic_info (CVDEC_DISP_INFO_GET,
							(void *) &(omx_private->pVideoDecodInstance[uiDecoderID].dec_disp_info_ctrl),
							(void *) &pdec_disp_info,
							(void *) &(omx_private->pVideoDecodInstance[uiDecoderID].dec_disp_info_input),
							omx_private,
							uiDecoderID);
			}

			#ifdef  SUPPORT_PVR
			if (nDeocodedBufferOut)
			{
				if (omx_private->pVideoDecodInstance[uiDecoderID].nDelayedOutputBufferCount != 0)
				{
					omx_private->pVideoDecodInstance[uiDecoderID].nSkipOutputBufferCount = omx_private->pVideoDecodInstance[uiDecoderID].nDelayedOutputBufferCount;
				}
				else
				{
					omx_private->pVideoDecodInstance[uiDecoderID].nSkipOutputBufferCount = 0xffff; // b-frame skip
				}
				pdec_disp_info = &dec_disp_info_tmp;
			}
			else if ((omx_private->pVideoDecodInstance[uiDecoderID].nPVRFlags & PVR_FLAG_TRICK) == 0)
			{
				if (omx_private->pVideoDecodInstance[uiDecoderID].nSkipOutputBufferCount == 0xffff)
				{
					omx_private->pVideoDecodInstance[uiDecoderID].nSkipOutputBufferCount = 0;
					SET_FLAG(omx_private->pVideoDecodInstance[uiDecoderID], STATE_SKIP_OUTPUT_B_FRAME);
				}
				else if (omx_private->pVideoDecodInstance[uiDecoderID].nSkipOutputBufferCount > 0)
				{
					ALOGD("[PVR] Skip Output Buffer after TRICK MODE(%d), idx %d", (int)omx_private->pVideoDecodInstance[uiDecoderID].nSkipOutputBufferCount, dispOutIdx);
					omx_private->pVideoDecodInstance[uiDecoderID].nSkipOutputBufferCount--;
					if (omx_private->pVideoDecodInstance[uiDecoderID].nSkipOutputBufferCount == 0)
					{
						SET_FLAG(omx_private->pVideoDecodInstance[uiDecoderID], STATE_SKIP_OUTPUT_B_FRAME);
					}
					if( ( ret = omx_private->pVideoDecodInstance[uiDecoderID].gspfVDec( VDEC_BUF_FLAG_CLEAR, NULL, &dispOutIdx, NULL, omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance) ) < 0 )
					{
						ALOGE( "[VDEC-%d] [VDEC_BUF_FLAG_CLEAR] Idx = %d, ret = %ld", (int)omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance, dispOutIdx, ret );
						if(vdec_buffer_clear_error_flag == 0){
							SendVideoErrorEvent(openmaxStandComp, VDEC_BUFFER_CLEAR_ERROR);
							vdec_buffer_clear_error_flag = 1;
						}
						goto ERR_PROCESS;
					}
					decode_result = 3;
				}
			}
			#endif//SUPPORT_PVR

			if (pdec_disp_info != (dec_disp_info_t *) 0)
			{
				if (CHECK_FLAG(omx_private->pVideoDecodInstance[uiDecoderID], STATE_SKIP_OUTPUT_B_FRAME))
				{
					OMX_S32 pic_type = GetFrameType(omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iBitstreamFormat,
													pdec_disp_info->m_iFrameType, pdec_disp_info->m_iPicStructure);
					if (pic_type == PIC_TYPE_I || pic_type == PIC_TYPE_P)
					{
						CLEAR_FLAG(omx_private->pVideoDecodInstance[uiDecoderID], STATE_SKIP_OUTPUT_B_FRAME);
					}
					else
					{
						if( ( ret = omx_private->pVideoDecodInstance[uiDecoderID].gspfVDec( VDEC_BUF_FLAG_CLEAR, NULL, &dispOutIdx, NULL, omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance) ) < 0 )
						{
							ALOGE( "[VDEC-%d] [VDEC_BUF_FLAG_CLEAR] Idx = %d, ret = %ld",(int) omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance, dispOutIdx, ret );
							if(vdec_buffer_clear_error_flag == 0){
								SendVideoErrorEvent(openmaxStandComp, VDEC_BUFFER_CLEAR_ERROR);
								vdec_buffer_clear_error_flag = 1;
							}
							goto ERR_PROCESS;
						}
						decode_result = 3;

						ALOGI("[VDEC-%d] drop frame after seek (idx: %d / type: %d)", (int)omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance, dispOutIdx, (int)pic_type);
					}
				}

				pOutputBuffer->nTimeStamp = (OMX_TICKS) pdec_disp_info->m_iTimeStamp * 1000;	//pdec_disp_info->m_iM2vFieldSequence * 1000;

				memcpy (pOutputBuffer->pBuffer + 28, &pdec_disp_info->m_iTopFieldFirst, 4);
				memcpy (pOutputBuffer->pBuffer + 32, &pdec_disp_info->m_iIsProgressive, 4);
				memcpy (pOutputBuffer->pBuffer + 36, &pdec_disp_info->m_iFrameType, 4);
#ifdef SET_FRAMEBUFFER_INTO_MAX
				if(omx_private->pVideoDecodInstance[uiDecoderID].gsVDecUserInfo.bMaxfbMode)
				{
					memcpy (pOutputBuffer->pBuffer + 40, &omx_private->pVideoDecodInstance[uiDecoderID].gsVDecUserInfo.m_iFrameWidth, 4);
					memcpy (pOutputBuffer->pBuffer + 44, &omx_private->pVideoDecodInstance[uiDecoderID].gsVDecUserInfo.m_iFrameHeight, 4);
				}
				else
#endif
				{
					memcpy (pOutputBuffer->pBuffer + 40, &pdec_disp_info->m_iWidth, 4);
					memcpy (pOutputBuffer->pBuffer + 44, &pdec_disp_info->m_iHeight, 4);
				}
				memcpy (pOutputBuffer->pBuffer + 56, &dispOutIdx, 4);
				memcpy (pOutputBuffer->pBuffer + 60, &uiDecoderID, 4);
				nOutputFrameWidth = pdec_disp_info->m_iWidth;
				nOutputFrameHeight = pdec_disp_info->m_iHeight;

				#ifdef VIDEO_USER_DATA_PROCESS
				if (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_bEnableUserData)
				{
					video_userdata_list_t *pUserDataList;
					if( UserDataCtrl_Get( omx_private->pVideoDecodInstance[uiDecoderID].pUserData_List_Array, dispOutIdx, &pUserDataList ) >= 0 )
					{
						DBUG_MSG("[userdata]\t\tget %2d, %10d\n", dispOutIdx ,(int)(pUserDataList->ullPTS/1000) );
						print_user_data (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_UserDataAddress[VA]);
						process_user_data( openmaxStandComp, pUserDataList);
						UserDataCtrl_Clear( pUserDataList );
					}
				}
				#endif

				DPRINTF_DEC ("[Out - %s][N:%4d][LEN:%6d][RT:%8d] [DispIdx:%2d][OutStat:%d][FieldSeq:%d][TR:%8d] ",
							 print_pic_type (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_iBitstreamFormat,
											 pdec_disp_info->m_iFrameType, pdec_disp_info->m_iPicStructure),
							 omx_private->pVideoDecodInstance[uiDecoderID].video_dec_idx, pdec_disp_info->m_iFrameSize, pdec_disp_info->m_iTimeStamp,
							 omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDispOutIdx,
							 omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iOutputStatus,
							 pdec_disp_info->m_iM2vFieldSequence, pdec_disp_info->m_iextTimeStamp);
			}
			else
			{
				//exception process!! temp!!
				pOutputBuffer->nTimeStamp = pInputBuffer->nTimeStamp;
			}
		}

		if(decode_result != 3)
		{
			omx_private->pVideoDecodInstance[uiDecoderID].video_dec_idx++;
			omx_private->pVideoDecodInstance[uiDecoderID].buffer_unique_id++;
		}
	}
	else
	{
		if(omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iOutputStatus == VPU_DEC_OUTPUT_SUCCESS)
		{
			dec_disp_info_t *pdec_disp_info = NULL;
			//decode_result = 3;
			nDisplayIndex = omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDispOutIdx;
			omx_private->pVideoDecodInstance[uiDecoderID].dec_disp_info_input.m_iFrameIdx = nDisplayIndex;
			disp_pic_info (CVDEC_DISP_INFO_GET, (void *) &(omx_private->pVideoDecodInstance[uiDecoderID].dec_disp_info_ctrl), (void *) &pdec_disp_info,
						   (void *) &(omx_private->pVideoDecodInstance[uiDecoderID].dec_disp_info_input), omx_private, uiDecoderID);
		}
		else if( (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodingStatus != VPU_DEC_SUCCESS) && (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodingStatus != VPU_DEC_SUCCESS_FIELD_PICTURE) ){
			ALOGE ("[VDEC_DECODE:%d] NO-OUTPUT!! m_iDispOutIdx = %d, m_iDecodedIdx = %d, m_iOutputStatus = %d, m_iDecodingStatus = %d, m_iNumOfErrMBs = %d", uiDecoderID, omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDispOutIdx, omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodedIdx, omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iOutputStatus, omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iDecodingStatus, omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iNumOfErrMBs);
			if(vdec_no_input_error_flag == 0){
				SendVideoErrorEvent(openmaxStandComp, VDEC_NO_OUTPUT_ERROR);
				vdec_no_input_error_flag = 1;
			}
		}
	}

	// To process input stream retry or next frame
	switch (decode_result)
	{
	case 0:	// Display Output Success, Decode Failed Due to Buffer Full
		pOutputBuffer->nFilledLen =
					nOutputFrameWidth * nOutputFrameHeight * 3 / 2;
		break;
	case 1:	// Display Output Failed, Decode Failed Due to Buffer Full
		break;
	case 2:	// Display Output Success, Decode Success
		pOutputBuffer->nFilledLen =
					nOutputFrameWidth * nOutputFrameHeight * 3 / 2;
		pInputBuffer->nFilledLen = 0;
		break;
	case 3:	// Display Output Failed, Decode Success
		pInputBuffer->nFilledLen = 0;
		break;
	default:
		break;
	}

	if(omx_private->pVideoDecodInstance[uiDecoderID].bVideoPaused == OMX_TRUE /*|| uiDisplayID != uiDecoderID*/)
	{
		pOutputBuffer->nFilledLen = 0;
 	}

	if( pOutputBuffer->nFilledLen == 0 )
	{
		if( nDisplayIndex != -1 )
		{
		#ifdef VIDEO_USER_DATA_PROCESS
			if (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecInit.m_bEnableUserData)
			{
				video_userdata_list_t *pUserDataList;
				if( UserDataCtrl_Get( omx_private->pVideoDecodInstance[uiDecoderID].pUserData_List_Array, nDisplayIndex, &pUserDataList ) >= 0 )
				{
					DBUG_MSG("[userdata]\t\tget %2d, %10d\n", nDisplayIndex ,(int)(pUserDataList->ullPTS/1000) );
					print_user_data (omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_UserDataAddress[VA]);
					//process_user_data( openmaxStandComp, pUserDataList);
					UserDataCtrl_Clear( pUserDataList );
				}
			}
		#endif

			if( nDisplayIndex !=	0 )
			{
				if ((ret =omx_private->pVideoDecodInstance[uiDecoderID].gspfVDec (VDEC_BUF_FLAG_CLEAR, NULL, &nDisplayIndex, NULL, omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance)) < 0)
				{
					ALOGE ("%s VDEC_BUF_FLAG_CLEAR Err : [%d] index : %d",__func__, (int)ret, (int) nDisplayIndex);
				}
			}
		}
	}

#ifdef	SUPPORT_SAVE_OUTPUTBUFFER
	if( uiDecoderID == 0 )
	{
		FILE     *fp;
		char      file_name[32];
		int       iVideoWidth, iVideoHeight;
		if (giDumpVideoFileIndex < 20)
		{
			sprintf (file_name, "/sdcard/out_video%d.raw", ++giDumpVideoFileIndex);
			fp = fopen (file_name, "wb");
			if (fp)
			{
				iVideoWidth = outPort->sPortParam.format.video.nFrameWidth;
				iVideoHeight = outPort->sPortParam.format.video.nFrameHeight;
				ALOGE ("dump file created : %s (%dx%d)", file_name, iVideoWidth, iVideoHeight);
				fwrite ((char *) omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pDispOut[VA][0], 1, iVideoWidth * iVideoHeight, fp);
				fwrite ((char *) omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pDispOut[VA][1], 1, iVideoWidth * iVideoHeight / 4, fp);
				fwrite ((char *) omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_pDispOut[VA][2], 1, iVideoWidth * iVideoHeight / 4, fp);
				fclose (fp);
				sync ();
			}
		}
	}
#endif

#ifdef SUPPORT_PVR
	if (pOutputBuffer->nFilledLen > 0)
	{
		SetPVRFlags(omx_private, uiDecoderID, pOutputBuffer);
	}
#endif//SUPPORT_PVR

	//ALOGE("PTS = %lld", pOutputBuffer->nTimeStamp);
	pthread_mutex_unlock(&omx_private->pVideoDecodInstance[uiDecoderID].stVideoStart.mutex);
	return;

  ERR_PROCESS:
#ifdef SET_FRAMEBUFFER_INTO_MAX
	if(omx_private->pVideoDecodInstance[uiDecoderID].gsVDecUserInfo.bMaxfbMode && ret == -2)
	{
		OMX_S32 left, top;
		OMX_U32 width, height;
		left	= 0;
		top		= 0;
		width	= omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iWidth;
		height	= omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput.m_DecOutInfo.m_iHeight;
		if(CheckCrop_DecodedOut(openmaxStandComp, uiDecoderID, left, top, width, height) > 0)
		{
			(*(omx_private->callbacks->EventHandler))(
							   openmaxStandComp,
							   omx_private->callbackData,
							   OMX_EventPortSettingsChanged,
							   OMX_DirOutput,
							   OMX_IndexConfigCommonOutputCrop,
							   (OMX_PTR)&omx_private->pVideoDecodInstance[uiDecoderID].rectCropParm);
		}
		pthread_mutex_unlock(&omx_private->pVideoDecodInstance[uiDecoderID].stVideoStart.mutex);
		return;
	}
#endif
	ALOGE ("%s:%d: ERROR [%d]!!!", __func__, *((int*)omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance), (int)ret);
	pOutputBuffer->nFilledLen = 0;
	pInputBuffer->nFilledLen = 0;

#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
	if(pPrivateData->bRollback == OMX_TRUE)
	{
		(*(omx_private->callbacks->EventHandler)) (openmaxStandComp, omx_private->callbackData,OMX_EventVendorStreamRollBack, 0, 0, NULL);
	}
#endif

	if (omx_private->pVideoDecodInstance[0].gsVDecOutput.m_DecOutInfo.m_iDecodingStatus == VPU_DEC_SUCCESS_FIELD_PICTURE ||
		omx_private->pVideoDecodInstance[1].gsVDecOutput.m_DecOutInfo.m_iDecodingStatus == VPU_DEC_SUCCESS_FIELD_PICTURE)
	{
		if(omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_SUB].isVPUClosed  == OMX_FALSE)
		{
			omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_SUB].avcodecInited = 0;
			omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_SUB].isVPUClosed = OMX_TRUE;
			omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_SUB].gspfVDec (VDEC_SW_RESET, NULL, NULL, NULL, omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_SUB].pVdec_Instance);
			omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_SUB].gspfVDec (VDEC_CLOSE, TCC_DXBVIDEO_DISPLAY_SUB, NULL, &omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_SUB].gsVDecOutput, omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_SUB].pVdec_Instance);

#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
			pPrivateData->bRollback = OMX_FALSE;
			omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_SUB].in_index = 0;
			omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_SUB].out_index = 0;
			omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_SUB].used_fifo_count = 0;
#endif
		}

		if(omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_MAIN].isVPUClosed == OMX_FALSE)
		{
			omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_MAIN].avcodecInited = 0;
			omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_MAIN].isVPUClosed = OMX_TRUE;
			omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_MAIN].gspfVDec (VDEC_SW_RESET, NULL, NULL, NULL, omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_MAIN].pVdec_Instance);
//			omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_MAIN].gspfVDec (VDEC_HW_RESET, TCC_DXBVIDEO_DISPLAY_MAIN, NULL, &omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_MAIN].gsVDecOutput, omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_MAIN].pVdec_Instance);
			omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_MAIN].gspfVDec (VDEC_CLOSE, TCC_DXBVIDEO_DISPLAY_MAIN, NULL, &omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_MAIN].gsVDecOutput, omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_MAIN].pVdec_Instance);

#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
			pPrivateData->bRollback = OMX_FALSE;
			omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_MAIN].in_index = 0;
			omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_MAIN].out_index = 0;
			omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_MAIN].used_fifo_count = 0;
#endif
			omx_private->stPreVideoInfo.iDecod_Instance = -1;
			omx_private->stPreVideoInfo.iDecod_status = -1;
			omx_private->stPreVideoInfo.uDecod_time = 0;
		}
		ALOGE ("%s:%d: Multi instance error END\n", __func__, __LINE__);
	}
	else
	{
		if(omx_private->pVideoDecodInstance[uiDecoderID].isVPUClosed == OMX_FALSE)
		{
			omx_private->pVideoDecodInstance[uiDecoderID].avcodecInited = 0;
			omx_private->pVideoDecodInstance[uiDecoderID].isVPUClosed = OMX_TRUE;

			//if(omx_private->Resolution_Change == 0 && get_total_handle_cnt(openmaxStandComp)<1)
			//	omx_private->pVideoDecodInstance[uiDecoderID].gspfVDec (VDEC_HW_RESET, uiDecoderID, NULL, &omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput, omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance);
			omx_private->pVideoDecodInstance[uiDecoderID].gspfVDec (VDEC_SW_RESET, NULL, NULL, NULL, omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance);
			omx_private->pVideoDecodInstance[uiDecoderID].gspfVDec (VDEC_CLOSE, uiDecoderID, NULL, &omx_private->pVideoDecodInstance[uiDecoderID].gsVDecOutput, omx_private->pVideoDecodInstance[uiDecoderID].pVdec_Instance);

			//(*(omx_private->callbacks->EventHandler)) (openmaxStandComp, omx_private->callbackData,OMX_EventError, OMX_ErrorHardware, 0, NULL);
#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
			pPrivateData->bRollback = OMX_FALSE;
			//tcc_vsync_command(omx_private, TCC_LCDC_VIDEO_END_VSYNC, NULL);
			omx_private->pVideoDecodInstance[uiDecoderID].in_index = 0;
			omx_private->pVideoDecodInstance[uiDecoderID].out_index = 0;
			omx_private->pVideoDecodInstance[uiDecoderID].used_fifo_count = 0;
			//tcc_vsync_command(omx_private, TCC_LCDC_VIDEO_START_VSYNC, VPU_BUFF_COUNT);
#endif
		}
	}
	dxb_omx_displayClearqueue_init(omx_private, uiDecoderID);
	pthread_mutex_unlock(&omx_private->pVideoDecodInstance[uiDecoderID].stVideoStart.mutex);
	return;
}

OMX_ERRORTYPE dxb_omx_videodec_component_SetParameter (OMX_IN OMX_HANDLETYPE hComponent,
												   OMX_IN OMX_INDEXTYPE nParamIndex,
												   OMX_IN OMX_PTR ComponentParameterStructure)
{
	OMX_S32   *piArg;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_U32   portIndex;

	/* Check which structure we are being fed and make control its header */
	OMX_COMPONENTTYPE *openmaxStandComp = hComponent;
	omx_videodec_component_PrivateType *omx_private = openmaxStandComp->pComponentPrivate;
	omx_base_video_PortType *port;
	TCC_VDEC_PRIVATE *pPrivateData = (TCC_VDEC_PRIVATE*) omx_private->pPrivateData;

	if (ComponentParameterStructure == NULL)
	{
		return OMX_ErrorBadParameter;
	}

	DBUG_MSG ("   Setting parameter %i\n", nParamIndex);
	switch (nParamIndex)
	{
	case OMX_IndexParamPortDefinition:
		break;

	case OMX_IndexParamStandardComponentRole:
		{
			OMX_PARAM_COMPONENTROLETYPE *pComponentRole;
			pComponentRole = ComponentParameterStructure;
			if (!strcmp ((char *) pComponentRole->cRole, VIDEO_DEC_TCC_H264_ROLE))
			{
				omx_private->pVideoDecodInstance[omx_private->iDisplayID].video_coding_type = OMX_VIDEO_CodingAVC;
			}
			else if (!strcmp ((char *) pComponentRole->cRole, VIDEO_DEC_MPEG2_ROLE))
			{
				omx_private->pVideoDecodInstance[omx_private->iDisplayID].video_coding_type = OMX_VIDEO_CodingMPEG2;
			}
			else
			{
				return OMX_ErrorBadParameter;
			}
			break;
		}

	case OMX_IndexParamVideoSkipFrames:
		omx_private->guiSkipBframeEnable = 1;//enable
		omx_private->guiSkipBFrameNumber = (unsigned int)ComponentParameterStructure;
		break;

	case OMX_IndexParamVideoSetStart:
		{
			OMX_VIDEO_PARAM_STARTTYPE *pStartInfo;
			pStartInfo = (OMX_VIDEO_PARAM_STARTTYPE*) ComponentParameterStructure;
//	if (pStartInfo->nDevID >= iTotalHandle) return OMX_ErrorBadParameter;

            if(pStartInfo->bStartFlag == TRUE) {
                //in case of starting, if the state is starting, it is error
                if( omx_private->pVideoDecodInstance[pStartInfo->nDevID].bVideoStarted == OMX_TRUE )
                    return OMX_ErrorBadParameter;
            } else {	//if(pStartInfo->bStartFlag == FALSE)
                //in case of stopping, if the state is not starting, it is error
                if( omx_private->pVideoDecodInstance[pStartInfo->nDevID].bVideoStarted != OMX_TRUE )
                    return OMX_ErrorBadParameter;
            }

			omx_private->pVideoDecodInstance[pStartInfo->nDevID].stVideoStart.nDevID = pStartInfo->nDevID;
			omx_private->pVideoDecodInstance[pStartInfo->nDevID].stVideoStart.nFormat = pStartInfo->nVideoFormat;
			if(pStartInfo->bStartFlag) {
				omx_private->pVideoDecodInstance[pStartInfo->nDevID].stVideoStart.nState = TCC_DXBVIDEO_OMX_Dec_Start;

				omx_private->stPreVideoInfo.iDecod_Instance = -1;
				omx_private->stPreVideoInfo.iDecod_status = -1;
				omx_private->stPreVideoInfo.uDecod_time = 0;
				if (pStartInfo->nDevID==0) {
					omx_private->stVideoDefinition_dual.nFrameWidth = 0;
					omx_private->stVideoDefinition_dual.nFrameHeight = 0;
					omx_private->stVideoDefinition_dual.MainDecoderID = -1;
					omx_private->stVideoDefinition_dual.nAspectRatio= 0;
					omx_private->stVideoDefinition_dual.nFrameRate = 0;
				} else if (pStartInfo->nDevID==1) {
					omx_private->stVideoDefinition_dual.nSubFrameWidth = 0;
					omx_private->stVideoDefinition_dual.nSubFrameHeight = 0;
					omx_private->stVideoDefinition_dual.SubDecoderID = -1;
					omx_private->stVideoDefinition_dual.nSubAspectRatio= 0;
				}
			} else {
				omx_private->pVideoDecodInstance[pStartInfo->nDevID].stVideoStart.nState = TCC_DXBVIDEO_OMX_Dec_Stop;
			}
			CheckVideoStart(openmaxStandComp, pStartInfo->nDevID);
			if(pStartInfo->bStartFlag == TRUE)
    			for_iptv_info_init(openmaxStandComp);
			dxb_omx_displayClearqueue_init(omx_private, pStartInfo->nDevID);
		}
		break;

	case OMX_IndexParamVideoSetPause:
		{
			OMX_VIDEO_PARAM_PAUSETYPE *pPauseInfo;
			pPauseInfo = (OMX_VIDEO_PARAM_PAUSETYPE*) ComponentParameterStructure;
			omx_private->pVideoDecodInstance[omx_private->iDisplayID].bVideoPaused = pPauseInfo->bPauseFlag;
		}
		break;

	case OMX_IndexVendorParamSetVideoPause:
		{
			OMX_U32	ulDemuxId;
			piArg = (OMX_S32 *) ComponentParameterStructure;
			ulDemuxId = (OMX_U32) piArg[0];
#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
			if (isVsyncEnabled(omx_private, ulDemuxId) == 1)
			{
				if (piArg[1] == 1) {
					tcc_vsync_command(omx_private, (int)TCC_LCDC_VIDEO_SKIP_FRAME_START, 0);
				} else {
					tcc_vsync_command(omx_private, (int)TCC_LCDC_VIDEO_SKIP_FRAME_END, 0);
				}
			}
#endif
			omx_private->pVideoDecodInstance[ulDemuxId].bVideoPaused = (OMX_BOOL) piArg[1];
		}
		break;

	case OMX_IndexVendorParamIframeSearchEnable:
		{
//#ifdef	SUPPORT_SEARCH_IFRAME_ATSTARTING
			pthread_mutex_lock(&omx_private->pVideoDecodInstance[omx_private->iDisplayID].stVideoStart.mutex);
			SetIFrameSearch(omx_private, omx_private->iDisplayID, OMX_TRUE);
			pthread_mutex_unlock(&omx_private->pVideoDecodInstance[omx_private->iDisplayID].stVideoStart.mutex);
//#endif
		}
		break;

	case OMX_IndexVendorParamVideoDisplayOutputCh:
		{
			OMX_S8 *ulDemuxId = (OMX_S8 *)ComponentParameterStructure;
			int	iDisplayID = omx_private->iDisplayID;

			if (iDisplayID < 2) pthread_mutex_lock(&omx_private->pVideoDecodInstance[iDisplayID].stVideoStart.mutex);

			if(omx_private->iDisplayID != *ulDemuxId) {
				pPrivateData->bChangeDisplayID = OMX_TRUE;
			}
			omx_private->iDisplayID = *ulDemuxId;

			if (iDisplayID < 2) pthread_mutex_unlock(&omx_private->pVideoDecodInstance[iDisplayID].stVideoStart.mutex);
		}
		break;

	case OMX_IndexVendorParamVideoSurfaceState:
		{
			piArg = (OMX_S32 *) ComponentParameterStructure;
			pPrivateData->isRenderer = (OMX_BOOL) (*piArg);
#ifdef SET_FRAMEBUFFER_INTO_MAX
			if(	pPrivateData->isRenderer
			//&&	omx_private->iDisplayID == TCC_DXBVIDEO_DISPLAY_MAIN
			&&	omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_MAIN].gsVDecUserInfo.bMaxfbMode
			&&	omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_MAIN].rectCropParm.nWidth
			&&	omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_MAIN].rectCropParm.nHeight)
			{
				(*(omx_private->callbacks->EventHandler))(
						   openmaxStandComp,
						   omx_private->callbackData,
						   OMX_EventPortSettingsChanged,
						   OMX_DirOutput,
						   OMX_IndexConfigCommonOutputCrop,
						   (OMX_PTR)&omx_private->pVideoDecodInstance[TCC_DXBVIDEO_DISPLAY_MAIN].rectCropParm);
			}
#endif
		}
		break;

	case OMX_IndexVendorParamISDBTProprietaryData:
		{
			piArg = (OMX_S32 *) ComponentParameterStructure;
			pPrivateData->stIsdbtProprietaryData.channel_index = *piArg;
		}
		break;

#ifdef  SUPPORT_PVR
	case OMX_IndexVendorParamPlayStartRequest:
		{
			OMX_S32 ulPvrId;
			OMX_S8 *pucFileName;

			piArg = (OMX_S32 *) ComponentParameterStructure;
			ulPvrId = (OMX_S32) piArg[0];
			pucFileName = (OMX_S8 *) piArg[1];

			pthread_mutex_lock(&omx_private->pVideoDecodInstance[ulPvrId].stVideoStart.mutex);
			if ((omx_private->pVideoDecodInstance[ulPvrId].nPVRFlags & PVR_FLAG_START) == 0)
			{
				omx_private->pVideoDecodInstance[ulPvrId].nPVRFlags = PVR_FLAG_START | PVR_FLAG_CHANGED;
			}
			pthread_mutex_unlock(&omx_private->pVideoDecodInstance[ulPvrId].stVideoStart.mutex);
		}
		break;

	case OMX_IndexVendorParamPlayStopRequest:
		{
			OMX_S32 ulPvrId;

			piArg = (OMX_S32 *) ComponentParameterStructure;
			ulPvrId = (OMX_S32) piArg[0];

			pthread_mutex_lock(&omx_private->pVideoDecodInstance[ulPvrId].stVideoStart.mutex);
			if (omx_private->pVideoDecodInstance[ulPvrId].nPVRFlags & PVR_FLAG_START)
			{
				omx_private->pVideoDecodInstance[ulPvrId].nPVRFlags = PVR_FLAG_CHANGED;
			}
			pthread_mutex_unlock(&omx_private->pVideoDecodInstance[ulPvrId].stVideoStart.mutex);
		}
		break;

	case OMX_IndexVendorParamSeekToRequest:
		{
			OMX_S32 ulPvrId, nSeekTime;

			piArg = (OMX_S32 *) ComponentParameterStructure;
			ulPvrId = (OMX_S32) piArg[0];
			nSeekTime = (OMX_S32) piArg[1];

			pthread_mutex_lock(&omx_private->pVideoDecodInstance[ulPvrId].stVideoStart.mutex);
			if (omx_private->pVideoDecodInstance[ulPvrId].nPVRFlags & PVR_FLAG_START)
			{
				if (nSeekTime < 0)
				{
					if (omx_private->pVideoDecodInstance[ulPvrId].nPVRFlags & PVR_FLAG_TRICK)
					{
						omx_private->pVideoDecodInstance[ulPvrId].nPVRFlags &= ~PVR_FLAG_TRICK;
						//omx_private->pVideoDecodInstance[ulPvrId].nPVRFlags |= PVR_FLAG_CHANGED;
					}
				}
				else
				{
					if ((omx_private->pVideoDecodInstance[ulPvrId].nPVRFlags & PVR_FLAG_TRICK) == 0)
					{
						omx_private->pVideoDecodInstance[ulPvrId].nPVRFlags |= (PVR_FLAG_TRICK | PVR_FLAG_CHANGED);
					}
				}
			}
			pthread_mutex_unlock(&omx_private->pVideoDecodInstance[ulPvrId].stVideoStart.mutex);
		}
		break;

	case OMX_IndexVendorParamPlaySpeed:
		{
			OMX_S32 ulPvrId, nPlaySpeed;

			piArg = (OMX_S32 *) ComponentParameterStructure;
			ulPvrId = (OMX_S32) piArg[0];
			nPlaySpeed = (OMX_S32) piArg[1];

			pthread_mutex_lock(&omx_private->pVideoDecodInstance[ulPvrId].stVideoStart.mutex);
			if (omx_private->pVideoDecodInstance[ulPvrId].nPVRFlags & PVR_FLAG_START)
			{
				ALOGD("[PVR] Set Speed = %d", (int)nPlaySpeed);
				if (nPlaySpeed == 1)
				{
					if (omx_private->pVideoDecodInstance[ulPvrId].nPVRFlags & PVR_FLAG_TRICK)
					{
						omx_private->pVideoDecodInstance[ulPvrId].nPVRFlags &= ~PVR_FLAG_TRICK;
						if (omx_private->pVideoDecodInstance[ulPvrId].nPlaySpeed < 0)
						{
							omx_private->pVideoDecodInstance[ulPvrId].nPVRFlags |= PVR_FLAG_CHANGED;
						}
					}
				}
				else
				{
					if ((omx_private->pVideoDecodInstance[ulPvrId].nPVRFlags & PVR_FLAG_TRICK) == 0)
					{
						omx_private->pVideoDecodInstance[ulPvrId].nPVRFlags |= (PVR_FLAG_TRICK | PVR_FLAG_CHANGED);
					}
				}
				omx_private->pVideoDecodInstance[ulPvrId].nPlaySpeed = nPlaySpeed;
			}
			pthread_mutex_unlock(&omx_private->pVideoDecodInstance[ulPvrId].stVideoStart.mutex);
		}
		break;

	case OMX_IndexVendorParamPlayPause:
		{
			OMX_S32 ulPvrId, nPlayPause;

			piArg = (OMX_S32 *) ComponentParameterStructure;
			ulPvrId = (OMX_S32) piArg[0];
			nPlayPause = (OMX_S32) piArg[1];

			pthread_mutex_lock(&omx_private->pVideoDecodInstance[ulPvrId].stVideoStart.mutex);
			if (omx_private->pVideoDecodInstance[ulPvrId].nPVRFlags & PVR_FLAG_START)
			{
				if (nPlayPause == 0)
				{
					omx_private->pVideoDecodInstance[ulPvrId].nPVRFlags |= PVR_FLAG_PAUSE;
				}
				else
				{
					omx_private->pVideoDecodInstance[ulPvrId].nPVRFlags &= ~PVR_FLAG_PAUSE;
				}
			}
			pthread_mutex_unlock(&omx_private->pVideoDecodInstance[ulPvrId].stVideoStart.mutex);
		}
		break;
#endif//SUPPORT_PVR

	case OMX_IndexVendorParamVideoIsSupportCountry:
		{
			OMX_U32 ulDevId, ulCountry;
			OMX_S32	 *piArg;
			piArg = (OMX_S32 *) ComponentParameterStructure;
			ulDevId = (OMX_U32) piArg[0];
			omx_private->stPreVideoInfo.iDecod_Support_Country = (OMX_U32) piArg[1];

			ALOGE("%s %d iDecod_Support_Country = %d  \n", __func__, __LINE__, omx_private->stPreVideoInfo.iDecod_Support_Country);

		}
		break;

	case OMX_IndexVendorParamDxBActiveModeSet:
		{
			OMX_S32	 *ActiveMode;
			OMX_S32	 *piArg;
			OMX_VIDEO_PARAM_STARTTYPE *pStartInfo;
			pStartInfo = (OMX_VIDEO_PARAM_STARTTYPE*) ComponentParameterStructure;

			piArg = (OMX_S32 *) ComponentParameterStructure;
			ActiveMode = (OMX_S32*) piArg[1];

			pthread_mutex_lock(&omx_private->pVideoDecodInstance[pStartInfo->nDevID].stVideoStart.mutex);

			omx_private->stIPTVActiveModeInfo.IPTV_PLAYMode =1;
			omx_private->stIPTVActiveModeInfo.IPTV_Old_Activemode = omx_private->stIPTVActiveModeInfo.IPTV_Activemode;
			omx_private->stIPTVActiveModeInfo.IPTV_Activemode = (OMX_U32)ActiveMode;
			omx_private->stIPTVActiveModeInfo.IPTV_Mode_change_cnt =0;

			omx_private->stbframe_skipinfo.bframe_skipcnt = 0;
			{
				int i;
				for (i=0; i<32; i++)
					omx_private->stbframe_skipinfo.bframe_skipindex[i] = 0x100;
			}
			pthread_mutex_unlock(&omx_private->pVideoDecodInstance[pStartInfo->nDevID].stVideoStart.mutex);
		}
		break;

	case OMX_IndexVendorParamVideoSupportFieldDecoding:
		{
			OMX_S32	 *piArg = (OMX_S32 *) ComponentParameterStructure;
			omx_private->stVideoSubFunFlag.SupportFieldDecoding = (OMX_BOOL) (*piArg);
		}
		break;

	case OMX_IndexVendorParamVideoSupportIFrameSearch:
		{
			OMX_S32	 *piArg = (OMX_S32 *) ComponentParameterStructure;
			omx_private->stVideoSubFunFlag.SupportIFrameSearchMode = (OMX_BOOL) (*piArg);
		}
		break;

	case OMX_IndexVendorParamVideoSupportUsingErrorMB:
		{
			OMX_S32	 *piArg = (OMX_S32 *) ComponentParameterStructure;
			omx_private->stVideoSubFunFlag.SupprotUsingErrorMB = (OMX_BOOL) (*piArg);
		}
		break;

	case OMX_IndexVendorParamVideoSupportDirectDisplay:
		{
			OMX_S32	 *piArg = (OMX_S32 *) ComponentParameterStructure;
			omx_private->stVideoSubFunFlag.SupprotDirectDsiplay = (OMX_BOOL) (*piArg);
		}
		break;

	case OMX_IndexVendorParamDxBGetSTCFunction:
		{
			OMX_S32	 *piArg = (OMX_S32 *) ComponentParameterStructure;
			pPrivateData->gfnDemuxGetSTCValue = (pfnDemuxGetSTC) piArg[0];
			pPrivateData->pvDemuxApp = (void*) piArg[1];
		}
		break;

	case OMX_IndexVendorParamTunerDeviceSet:
		{
			piArg = (OMX_S32*)ComponentParameterStructure;
			pPrivateData->iDeviceIndex = *piArg;
		}
		break;

	default:	/*Call the base component function */
		return dxb_omx_base_component_SetParameter (hComponent, nParamIndex, ComponentParameterStructure);
	}
	return eError;
}

OMX_ERRORTYPE dxb_omx_videodec_component_GetParameter (OMX_IN OMX_HANDLETYPE hComponent,
												   OMX_IN OMX_INDEXTYPE nParamIndex,
												   OMX_INOUT OMX_PTR ComponentParameterStructure)
{

	omx_base_video_PortType *port;
	OMX_ERRORTYPE eError = OMX_ErrorNone;

	OMX_COMPONENTTYPE *openmaxStandComp = hComponent;
	omx_videodec_component_PrivateType *omx_private = openmaxStandComp->pComponentPrivate;
	if (ComponentParameterStructure == NULL)
	{
		return OMX_ErrorBadParameter;
	}
	DBUG_MSG ("   Getting parameter %i\n", nParamIndex);
	/* Check which structure we are being fed and fill its header */
	switch (nParamIndex)
	{
	case OMX_IndexParamStandardComponentRole:
		{
			OMX_PARAM_COMPONENTROLETYPE *pComponentRole;
			pComponentRole = ComponentParameterStructure;
			if ((eError =
				 checkHeader (ComponentParameterStructure, sizeof (OMX_PARAM_COMPONENTROLETYPE))) != OMX_ErrorNone)
			{
				break;
			}

			if (omx_private->pVideoDecodInstance[omx_private->iDisplayID].video_coding_type == OMX_VIDEO_CodingAVC)
			{
				strcpy ((char *) pComponentRole->cRole, VIDEO_DEC_TCC_H264_ROLE);
			}
			else if (omx_private->pVideoDecodInstance[omx_private->iDisplayID].video_coding_type == OMX_VIDEO_CodingMPEG2)
			{
				strcpy ((char *) pComponentRole->cRole, VIDEO_DEC_MPEG2_ROLE);
			}
			else
			{
				strcpy ((char *) pComponentRole->cRole, "\0");
			}
			break;
		}

	case OMX_IndexParamInterlaceInfo:
		{
			int *p_iInterlaceInfo = (int *)ComponentParameterStructure;

			*p_iInterlaceInfo = omx_private->iInterlaceInfo;
		}
		break;

	case OMX_IndexVendorParamDxBGetVideoInfo:
		{
			int 	ulHandle;
			int	*piArg;
			int	*width, *height;

			piArg = (int *) ComponentParameterStructure;
			ulHandle = (int) piArg[0];
			width = (int *)piArg[1];
			height = (int *)piArg[2];

			if(omx_private->pVideoDecodInstance[omx_private->iDisplayID].gsVDecOutput.m_pInitialInfo != NULL)
			{
				if(width != NULL && omx_private->pVideoDecodInstance[omx_private->iDisplayID].gsVDecOutput.m_pInitialInfo->m_iPicWidth)
					*width = omx_private->pVideoDecodInstance[omx_private->iDisplayID].gsVDecOutput.m_pInitialInfo->m_iPicWidth;
				if(height != NULL && omx_private->pVideoDecodInstance[omx_private->iDisplayID].gsVDecOutput.m_pInitialInfo->m_iPicHeight)
					*height = omx_private->pVideoDecodInstance[omx_private->iDisplayID].gsVDecOutput.m_pInitialInfo->m_iPicHeight;
			}
			else
			{
				if(width != NULL)
					*width = 0;
				if(height != NULL)
					*height = 0;
			}
		}
		break;

	case OMX_IndexVendorParamDxBGetDisplayedFirstFrame:
		{
			int *piArg;
			int iDevID;
			int *pDisplayedFirstFrame;
			piArg = (int *) ComponentParameterStructure;
			iDevID = (int) piArg[0];
			pDisplayedFirstFrame = (int *)piArg[1];
			if(omx_private != NULL && omx_private->pVideoDecodInstance != NULL)
				*pDisplayedFirstFrame = omx_private->pVideoDecodInstance[iDevID].nDisplayedFirstFrame;
			else
				*pDisplayedFirstFrame = 0;
		}
		break;

	default:	/*Call the base component function */
		return dxb_omx_base_component_GetParameter (hComponent, nParamIndex, ComponentParameterStructure);
	}
	return OMX_ErrorNone;
}

OMX_ERRORTYPE dxb_omx_videodec_component_MessageHandler (OMX_COMPONENTTYPE * openmaxStandComp,
													 internalRequestMessageType * message)
{
	omx_videodec_component_PrivateType *omx_private =
		(omx_videodec_component_PrivateType *) openmaxStandComp->pComponentPrivate;
	TCC_VDEC_PRIVATE *pPrivateData = (TCC_VDEC_PRIVATE*) omx_private->pPrivateData;
	OMX_ERRORTYPE err = OMX_ErrorNone;
	OMX_STATETYPE eCurrentState = omx_private->state;
	int i, protect = 0;

	DBUG_MSG ("In %s\n", __func__);

	if (message->messageType == OMX_CommandStateSet)
	{
		if ((message->messageParam == OMX_StateExecuting) && (omx_private->state == OMX_StateIdle))
		{
			for(i=0; i<MAX_INSTANCE_CNT; i++)
			{
				if (PORT_IS_ENABLED(omx_private->ports[i]))
				{
					t_Vpu_ClearThreadArg* Arg = &pPrivateData->Vpu_ClearThreadArg[i];
					char tname[33];
					sprintf(tname, "Omx_VidoeDec_VpuBufClearThread_%x", i);

					Arg->iDecoderID = i;
					Arg->openmaxStandComp = openmaxStandComp;
					pPrivateData->Vpu_BufferClearInfo[i].VpuBufferClearThreadRunning =1;
					err = TCCTHREAD_Create(&pPrivateData->Vpu_BufferClearInfo[i].VpuBufferClearhandle, (pThreadFunc)dxb_omx_video_dec_component_vpu_deleteindex, tname, HIGH_PRIORITY_7, Arg);
					ALOGE("%s %d TCCTHREAD_Create err = %d \n", __func__, __LINE__, err);

//IM692A-7, the order of VPU instance allocation was chagned. prepare() => video start
#if 0
					omx_private->pVideoDecodInstance[i].pVdec_Instance = dxb_vdec_alloc_instance();
#endif
					if (pPrivateData->mFBdevice < 0)
					{
						pPrivateData->mFBdevice = open(VSYNC_DEVICE, O_RDWR);
						if (pPrivateData->mFBdevice < 0)
						{
							ALOGE("can't open vsync driver[%d]", pPrivateData->iDeviceIndex);
						}
					}

					if (!omx_private->pVideoDecodInstance[i].avcodecReady)
					{
						memset(&omx_private->pVideoDecodInstance[i].stVideoStart,0x0, sizeof(VideoStartInfo));
						if (omx_private->pVideoDecodInstance[i].pVdec_Instance)
						{
							err = dxb_omx_videodec_component_LibInit (omx_private, i);
							omx_private->pVideoDecodInstance[i].avcodecReady = OMX_TRUE;
						}
					}
				}
			}
		}
		else if ((message->messageParam == OMX_StateIdle) && (omx_private->state == OMX_StateLoaded))
		{
			err = dxb_omx_videodec_component_Initialize (openmaxStandComp);
		}
		else if ((message->messageParam == OMX_StateLoaded) && (omx_private->state == OMX_StateIdle))
		{
			err = dxb_omx_videodec_component_Deinit (openmaxStandComp);
		}
		else if ((message->messageParam == OMX_StateIdle) && (omx_private->state == OMX_StateExecuting))
		{
			for(i=0; i<MAX_INSTANCE_CNT; i++)
				pPrivateData->Vpu_BufferClearInfo[i].VpuBufferClearThreadRunning =0;
			ALOGE("%s %d TCCTHREAD_Create \n", __func__, __LINE__);
		}
	}
	// Execute the base message handling
	err = dxb_omx_base_component_MessageHandler (openmaxStandComp, message);

	if (message->messageType == OMX_CommandStateSet)
	{
		if ((message->messageParam == OMX_StateIdle) && (eCurrentState == OMX_StateExecuting || eCurrentState == OMX_StatePause))
		{
			for(i=0; i<MAX_INSTANCE_CNT; i++)
			{
				if (omx_private->pVideoDecodInstance[i].avcodecReady)
				{
					omx_private->pVideoDecodInstance[i].avcodecReady = OMX_FALSE;
					dxb_omx_videodec_component_LibDeinit (omx_private, i);
				}

				if (PORT_IS_ENABLED(omx_private->ports[i]))
				{
					while (pPrivateData->Vpu_BufferClearInfo[i].VpuBufferClearThreadRunning != -1)
					{
						usleep(5000);
					}
#if 1 //Leak modification for videodecode component
					while(1){
						t_Vpu_BufferCearFrameInfo *ClearFrameInfo;
						ClearFrameInfo = (t_Vpu_BufferCearFrameInfo *)dxb_dequeue(omx_private->pVideoDecodInstance[i].pVPUDisplayedClearQueue);
						if(ClearFrameInfo){
							TCC_fo_free(__func__,__LINE__, ClearFrameInfo);
						}

						protect++;
						if(protect == 10)  //max 10ms
							break;

						if(ClearFrameInfo == NULL)
							break;
						usleep(1000);
					}
#endif

					TCCTHREAD_Join((void *)pPrivateData->Vpu_BufferClearInfo[i].VpuBufferClearhandle,NULL);
					ALOGE("%s %d TCCTHREAD_Create VpuBufferClearhandle End \n", __func__, __LINE__);

					if(pPrivateData->mFBdevice >= 0)
					{
						close(pPrivateData->mFBdevice);
						pPrivateData->mFBdevice = -1;
					}

//IM692A-7, the order of VPU instance release was chagned. release() => video stop
#if 0
					dxb_vdec_release_instance((void*)omx_private->pVideoDecodInstance[i].pVdec_Instance);
					omx_private->pVideoDecodInstance[i].pVdec_Instance = NULL;
#endif
				}
			}
		}
	}
	return err;
}

OMX_ERRORTYPE dxb_omx_videodec_component_ComponentRoleEnum (OMX_IN OMX_HANDLETYPE hComponent,
														OMX_OUT OMX_U8 * cRole, OMX_IN OMX_U32 nIndex)
{

	OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *) hComponent;
	omx_videodec_component_PrivateType *omx_private =
		(omx_videodec_component_PrivateType *) openmaxStandComp->pComponentPrivate;

	if (nIndex == 0)
	{
		if (omx_private->pVideoDecodInstance[omx_private->iDisplayID].video_coding_type == OMX_VIDEO_CodingAVC)
		{
			strcpy ((char *) cRole, VIDEO_DEC_TCC_H264_ROLE);
		}
		else if (omx_private->pVideoDecodInstance[omx_private->iDisplayID].video_coding_type == OMX_VIDEO_CodingMPEG2)
		{
			strcpy ((char *) cRole, VIDEO_DEC_MPEG2_ROLE);
		}
	}
	else
	{
		return OMX_ErrorUnsupportedIndex;
	}

	return OMX_ErrorNone;
}

OMX_ERRORTYPE dxb_omx_videodec_component_SetConfig (OMX_HANDLETYPE hComponent,
												OMX_INDEXTYPE nIndex, OMX_PTR pComponentConfigStructure)
{

	OMX_ERRORTYPE err = OMX_ErrorNone;
	OMX_VENDOR_EXTRADATATYPE *pExtradata;

	OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *) hComponent;
	omx_videodec_component_PrivateType *omx_private =
		(omx_videodec_component_PrivateType *) openmaxStandComp->pComponentPrivate;

	if (pComponentConfigStructure == NULL)
	{
		return OMX_ErrorBadParameter;
	}
	DBUG_MSG ("   Getting configuration %i\n", nIndex);
	/* Check which structure we are being fed and fill its header */
	switch (nIndex)
	{
	default:	// delegate to superclass
		return dxb_omx_base_component_SetConfig (hComponent, nIndex, pComponentConfigStructure);
	}
	return err;
}

OMX_ERRORTYPE dxb_omx_videodec_component_GetExtensionIndex (OMX_IN OMX_HANDLETYPE hComponent,
														OMX_IN OMX_STRING cParameterName,
														OMX_OUT OMX_INDEXTYPE * pIndexType)
{

	DBUG_MSG ("In  %s \n", __func__);
	return OMX_ErrorNone;
}

extern OMX_ERRORTYPE dxb_omx_ring_videodec_default_component_Init(OMX_COMPONENTTYPE *openmaxStandComp);

OMX_ERRORTYPE dxb_omx_videodec_default_component_Init(OMX_COMPONENTTYPE *openmaxStandComp)
{
	OMX_ERRORTYPE err = OMX_ErrorNone;

#if defined(RING_BUFFER_MODULE_ENABLE)
	char value[PROPERTY_VALUE_MAX];
	property_get("tcc.dxb.ringbuffer.enable", value, "1");

	if( atoi(value) )
	{
		err = dxb_omx_ring_videodec_default_component_Init(openmaxStandComp);
	}
	else
	{
		err = dxb_omx_videodec_component_Constructor(openmaxStandComp,VIDEO_DEC_BASE_NAME);
	}
#else
	err = dxb_omx_videodec_component_Constructor(openmaxStandComp,VIDEO_DEC_BASE_NAME);
#endif

	return err;
}


void* dxb_omx_video_twoport_filter_component_BufferMgmtFunction (void* param)
{
  OMX_COMPONENTTYPE* openmaxStandComp = (OMX_COMPONENTTYPE*)param;
  omx_videodec_component_PrivateType* omx_base_component_Private=(omx_videodec_component_PrivateType*)openmaxStandComp->pComponentPrivate;
  omx_base_filter_PrivateType* omx_base_filter_Private = (omx_base_filter_PrivateType*)omx_base_component_Private;
  omx_base_PortType *pInPort[2];
  omx_base_PortType *pOutPort[2];
  tsem_t* pInputSem[2];
  tsem_t* pOutputSem[2];
  queue_t* pInputQueue[2];
  queue_t* pOutputQueue[2];
  OMX_BUFFERHEADERTYPE* pOutputBuffer[2];
  OMX_BUFFERHEADERTYPE* pInputBuffer[2];
  OMX_BOOL isInputBufferNeeded[2],isOutputBufferNeeded[2];
  int inBufExchanged[2],outBufExchanged[2];
  int i, reqCallback[2];

  DEBUG(DEB_LEV_FULL_SEQ, "In %s\n", __func__);

  for (i=0; i < 2; i++) {
	pInPort[i] = (omx_base_PortType *)omx_base_filter_Private->ports[i];
	pInputSem[i] = pInPort[i]->pBufferSem;
	pInputQueue[i]= pInPort[i]->pBufferQueue;
	isInputBufferNeeded[i] = OMX_TRUE;
	inBufExchanged[i] = 0;
	pInputBuffer[i] = NULL;
  }
  for (i=0; i < 2; i++) {
	pOutPort[i] = (omx_base_PortType *)omx_base_filter_Private->ports[2+i];
	pOutputSem[i] = pOutPort[i]->pBufferSem;
	pOutputQueue[i]= pOutPort[i]->pBufferQueue;
	isOutputBufferNeeded[i] = OMX_TRUE;
	outBufExchanged[i] = 0;
	pOutputBuffer[i] = NULL;
  }

  reqCallback[0] = reqCallback[1] = OMX_FALSE;

  while(omx_base_filter_Private->state == OMX_StateIdle || omx_base_filter_Private->state == OMX_StateExecuting ||  omx_base_filter_Private->state == OMX_StatePause ||
    omx_base_filter_Private->transientState == OMX_TransStateLoadedToIdle) {

    /*Wait till the ports are being flushed*/
    pthread_mutex_lock(&omx_base_filter_Private->flush_mutex);
    while( PORT_IS_BEING_FLUSHED(pInPort[0]) || PORT_IS_BEING_FLUSHED(pInPort[1]) || PORT_IS_BEING_FLUSHED(pOutPort[0]) || PORT_IS_BEING_FLUSHED(pOutPort[1]) ) {
      pthread_mutex_unlock(&omx_base_filter_Private->flush_mutex);

      DEBUG(DEB_LEV_FULL_SEQ, "In %s 1 signalling flush all cond iE=%d,%d,iF=%d,%d,oE=%d,%d,oF=%d,%d iSemVal=%d,%d,oSemval=%d,%d\n",
        __func__,inBufExchanged[0],inBufExchanged[1],isInputBufferNeeded[0],isInputBufferNeeded[1],outBufExchanged[0],outBufExchanged[1],
        isOutputBufferNeeded[0],isOutputBufferNeeded[1],pInputSem[0]->semval,pInputSem[1]->semval,pOutputSem[0]->semval, pOutputSem[1]->semval);

      for (i=0; i < 2; i++) {
        if(isOutputBufferNeeded[i]==OMX_FALSE && PORT_IS_BEING_FLUSHED(pOutPort[i])) {
          pOutPort[i]->ReturnBufferFunction(pOutPort[i],pOutputBuffer[i]);
          outBufExchanged[i]--;
          pOutputBuffer[i]=NULL;
          isOutputBufferNeeded[i]=OMX_TRUE;
          DEBUG(DEB_LEV_FULL_SEQ, "OutPorts are flushing,so returning output buffer\n");
        }
      }

      for (i=0; i < 2; i++) {
        if(isInputBufferNeeded[i]==OMX_FALSE && PORT_IS_BEING_FLUSHED(pInPort[i])) {
          pInPort[i]->ReturnBufferFunction(pInPort[i],pInputBuffer[i]);
          inBufExchanged[i]--;
          pInputBuffer[i]=NULL;
          isInputBufferNeeded[i]=OMX_TRUE;
          DEBUG(DEB_LEV_FULL_SEQ, "InPorts[%d] are flushing,so returning input buffer\n", i);
        }
      }

      DEBUG(DEB_LEV_FULL_SEQ, "In %s 2 signalling flush all cond iE=%d,%d,iF=%d,%d,oE=%d,%d,oF=%d,%d iSemVal=%d,%d,oSemval=%d,%d\n",
        __func__,inBufExchanged[0],inBufExchanged[1],isInputBufferNeeded[0],isInputBufferNeeded[1],outBufExchanged[0],outBufExchanged[1],
        isOutputBufferNeeded[0],isOutputBufferNeeded[1],pInputSem[0]->semval,pInputSem[1]->semval,pOutputSem[0]->semval, pOutputSem[1]->semval);

      tsem_up(omx_base_filter_Private->flush_all_condition);
      tsem_down(omx_base_filter_Private->flush_condition);
      pthread_mutex_lock(&omx_base_filter_Private->flush_mutex);
    }
    pthread_mutex_unlock(&omx_base_filter_Private->flush_mutex);

    /*No buffer to process. So wait here*/
    if((isInputBufferNeeded[0]==OMX_TRUE && pInputSem[0]->semval==0) && (isInputBufferNeeded[1]==OMX_TRUE && pInputSem[1]->semval==0) &&
      (omx_base_filter_Private->state != OMX_StateLoaded && omx_base_filter_Private->state != OMX_StateInvalid)) {
      //Signalled from EmptyThisBuffer or FillThisBuffer or some thing else
      DEBUG(DEB_LEV_FULL_SEQ, "Waiting for next input/output buffer\n");
      tsem_down(omx_base_filter_Private->bMgmtSem);
    }
    if(omx_base_filter_Private->state == OMX_StateLoaded || omx_base_filter_Private->state == OMX_StateInvalid) {
      DEBUG(DEB_LEV_FULL_SEQ, "In %s Buffer Management Thread is exiting\n",__func__);
      break;
    }

	if((isOutputBufferNeeded[0]==OMX_TRUE && pOutputSem[0]->semval==0) && (isOutputBufferNeeded[1]==OMX_TRUE && pOutputSem[1]->semval==0) &&
		(omx_base_filter_Private->state != OMX_StateLoaded && omx_base_filter_Private->state != OMX_StateInvalid) &&
		!(PORT_IS_BEING_FLUSHED(pInPort[0]) || PORT_IS_BEING_FLUSHED(pInPort[1]) || PORT_IS_BEING_FLUSHED(pOutPort[0]) || PORT_IS_BEING_FLUSHED(pOutPort[1]))) {
			//Signalled from EmptyThisBuffer or FillThisBuffer or some thing else
			DEBUG(DEB_LEV_FULL_SEQ, "Waiting for next input/output buffer\n");
			tsem_down(omx_base_filter_Private->bMgmtSem);
	}
	if(omx_base_filter_Private->state == OMX_StateLoaded || omx_base_filter_Private->state == OMX_StateInvalid) {
		DEBUG(DEB_LEV_FULL_SEQ, "In %s Buffer Management Thread is exiting\n",__func__);
		break;
	}

	DEBUG(DEB_LEV_FULL_SEQ, "Waiting for input buffer semval=%d,%d \n",pInputSem[0]->semval,pInputSem[1]->semval);
	for (i=0; i < 2; i++) {
		if ((pInputSem[i]->semval > 0 && isInputBufferNeeded[i]==OMX_TRUE) && pOutputSem[i]->semval >0 && isOutputBufferNeeded[i]==OMX_TRUE) {
			reqCallback[i] = OMX_TRUE;
		}
	}

	if (reqCallback[0]==OMX_TRUE || reqCallback[1]==OMX_TRUE) {
		for (i=0; i < 2; i++) {
			if (reqCallback[i]==OMX_TRUE) {
				if (pInputSem[i]->semval > 0 && isInputBufferNeeded[i]==OMX_TRUE) {
					tsem_down(pInputSem[i]);
					if(pInputQueue[i]->nelem > 0) {
						inBufExchanged[i]++;
						isInputBufferNeeded[i]=OMX_FALSE;
						pInputBuffer[i] = dxb_dequeue(pInputQueue[i]);
						if (pInputBuffer[i]==NULL) {
							DEBUG(DEB_LEV_FULL_SEQ, "Had NULL input buffer!!\n");
							goto	omx_error_exit;
						}
						if(checkHeader(pInputBuffer[i], sizeof(OMX_BUFFERHEADERTYPE)))
						{
							DEBUG(DEB_LEV_FULL_SEQ, "In %s, crashed input buffer!!\n", __func__);
							isInputBufferNeeded[i] = OMX_TRUE;
							inBufExchanged[i]--;
							pInputBuffer[i] = NULL;
						}
					}
				}

			    /*When we have input buffer to process then get one output buffer*/
				if (pOutputSem[i]->semval > 0 && isOutputBufferNeeded[i]==OMX_TRUE) {
					tsem_down (pOutputSem[i]);
					if (pOutputQueue[i]->nelem > 0) {
						outBufExchanged[i]++;
						isOutputBufferNeeded[i]=OMX_FALSE;
						pOutputBuffer[i] = dxb_dequeue(pOutputQueue[i]);
						if(pOutputBuffer[i] == NULL) {
							DEBUG(DEB_LEV_FULL_SEQ, "Had NULL output buffer!! op is=%d,iq=%d\n",pOutputSem[i]->semval,pOutputQueue[i]->nelem);
							goto omx_error_exit;
						}
						if(checkHeader(pOutputBuffer[i], sizeof(OMX_BUFFERHEADERTYPE)))
						{
							DEBUG(DEB_LEV_FULL_SEQ, "crashed output buffer!!\n");
							isOutputBufferNeeded[i]=OMX_TRUE;
							outBufExchanged[i]--;
							pOutputBuffer[i] = NULL;
						}
					}
				}

				if(isInputBufferNeeded[i]==OMX_FALSE) {
					if(pInputBuffer[i]->hMarkTargetComponent != NULL){
						if((OMX_COMPONENTTYPE*)pInputBuffer[i]->hMarkTargetComponent ==(OMX_COMPONENTTYPE *)openmaxStandComp) {
							/*Clear the mark and generate an event*/
							(*(omx_base_filter_Private->callbacks->EventHandler))
								(openmaxStandComp,
								omx_base_filter_Private->callbackData,
								OMX_EventMark, /* The command was completed */
								1, /* The commands was a OMX_CommandStateSet */
								0, /* The state has been changed in message->messageParam2 */
								pInputBuffer[i]->pMarkData);
						} else {
							/*If this is not the target component then pass the mark*/
							omx_base_filter_Private->pMark.hMarkTargetComponent = pInputBuffer[i]->hMarkTargetComponent;
							omx_base_filter_Private->pMark.pMarkData			  = pInputBuffer[i]->pMarkData;
						}
						pInputBuffer[i]->hMarkTargetComponent = NULL;
					}
				}

				if(isInputBufferNeeded[i]==OMX_FALSE && isOutputBufferNeeded[i]==OMX_FALSE) {
					if(omx_base_filter_Private->pMark.hMarkTargetComponent != NULL){
						pOutputBuffer[i]->hMarkTargetComponent = omx_base_filter_Private->pMark.hMarkTargetComponent;
						pOutputBuffer[i]->pMarkData			= omx_base_filter_Private->pMark.pMarkData;
						omx_base_filter_Private->pMark.hMarkTargetComponent = NULL;
						omx_base_filter_Private->pMark.pMarkData			= NULL;
					}

					pOutputBuffer[i]->nTimeStamp = pInputBuffer[i]->nTimeStamp;

					if (omx_base_filter_Private->BufferMgmtCallback && pInputBuffer[i]->nFilledLen > 0) {
						if(omx_base_filter_Private->state == OMX_StateExecuting) {
							(*(omx_base_filter_Private->BufferMgmtCallback))(openmaxStandComp, pInputBuffer[i], pOutputBuffer[i]);
						}
						if(pInputBuffer[i]->nFlags == OMX_BUFFERFLAG_SEEKONLY) {
							pOutputBuffer[i]->nFlags = pInputBuffer[i]->nFlags;
							pInputBuffer[i]->nFlags = 0;
						}

						if(pInputBuffer[i]->nFlags == OMX_BUFFERFLAG_SYNCFRAME) {
							pOutputBuffer[i]->nFlags = pInputBuffer[i]->nFlags;
							pInputBuffer[i]->nFlags = 0;
						}
					} else {
					/*It no buffer management call back the explicitly consume input buffer*/
						pInputBuffer[i]->nFilledLen = 0;
					}

					if(pInputBuffer[i]->nFlags == OMX_BUFFERFLAG_STARTTIME) {
						DEBUG(DEB_LEV_FULL_SEQ, "Detected	START TIME flag in the input[%d] buffer filled len=%d\n", i, (int)pInputBuffer[i]->nFilledLen);
						pOutputBuffer[i]->nFlags = pInputBuffer[i]->nFlags;
						pInputBuffer[i]->nFlags = 0;
					}

					if(((pInputBuffer[i]->nFlags & OMX_BUFFERFLAG_EOS) == 1) && pInputBuffer[i]->nFilledLen==0 ) {
						pOutputBuffer[i]->nFlags=pInputBuffer[i]->nFlags;
						pInputBuffer[i]->nFlags=0;
						(*(omx_base_filter_Private->callbacks->EventHandler))
							(openmaxStandComp,
							omx_base_filter_Private->callbackData,
							OMX_EventBufferFlag, /* The command was completed */
							1, /* The commands was a OMX_CommandStateSet */
							pOutputBuffer[i]->nFlags, /* The state has been changed in message->messageParam2 */
							NULL);
						omx_base_filter_Private->bIsEOSReached = OMX_TRUE;
					}

					if(PORT_IS_BEING_CHANGED(pOutPort[i])) {
						ALOGD("PORT_IS_BEING_CHANGED before !! ");
						tsem_wait(omx_base_component_Private->bPortChangeSem);
						ALOGD("PORT_IS_BEING_CHANGED after !!  ");
					}

					if(omx_base_filter_Private->state==OMX_StatePause &&
						!(PORT_IS_BEING_FLUSHED(pInPort[0]) || PORT_IS_BEING_FLUSHED(pInPort[1]) || PORT_IS_BEING_FLUSHED(pOutPort[0]) || PORT_IS_BEING_FLUSHED(pOutPort[1]))
					) {
						/*Waiting at paused state*/
						ALOGE("In %s, component state is paused\n", __func__);
						tsem_wait(omx_base_component_Private->bStateSem);
					}

					/*If EOS and Input buffer Filled Len Zero then Return output buffer immediately*/
					if((pOutputBuffer[i]->nFilledLen != 0) || ((pOutputBuffer[i]->nFlags & OMX_BUFFERFLAG_EOS) == 1) || (omx_base_filter_Private->bIsEOSReached == OMX_TRUE)) {
						pOutPort[i]->ReturnBufferFunction(pOutPort[i],pOutputBuffer[i]);
						outBufExchanged[i]--;
						pOutputBuffer[i]=NULL;
						isOutputBufferNeeded[i]=OMX_TRUE;
					}

					if(omx_base_filter_Private->state==OMX_StatePause &&
						!(PORT_IS_BEING_FLUSHED(pInPort[0]) || PORT_IS_BEING_FLUSHED(pInPort[1]) || PORT_IS_BEING_FLUSHED(pOutPort[0]) || PORT_IS_BEING_FLUSHED(pOutPort[1]))
					) {
						/*Waiting at paused state*/
						ALOGE("In %s, component state is paused\n", __func__);
						tsem_wait(omx_base_component_Private->bStateSem);
					}

					/*Input Buffer has been completely consumed. So, return input buffer*/
					if((isInputBufferNeeded[i]== OMX_FALSE) && (pInputBuffer[i]->nFilledLen==0)) {
						pInPort[i]->ReturnBufferFunction(pInPort[i],pInputBuffer[i]);
						inBufExchanged[i]--;
						pInputBuffer[i]=NULL;
						isInputBufferNeeded[i]=OMX_TRUE;
					}

					if (isInputBufferNeeded[i]==OMX_TRUE && isOutputBufferNeeded[i]==OMX_TRUE)
						reqCallback[i]=OMX_FALSE;
				}
			}
			else {	//if(reqCallback[i]==OMX_TRUE)
				continue;
			}
		} //for (i=0; i < 2; i++)

		// Noah, 20170614, IM478A-58, The CPU load of 1seg is very high(80%[Irix off])
		usleep(1000);
	}
	else {
		//tsem_down(omx_base_filter_Private->bMgmtSem);
		usleep(5000);
	}
  }

omx_error_exit:
  DEBUG(DEB_LEV_SIMPLE_SEQ,"Exiting Buffer Management Thread\n");
  return NULL;
}

