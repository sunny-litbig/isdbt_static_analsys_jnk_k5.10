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


*   Description : omx_audiodec_component.c


*******************************************************************************/
#include <omxcore.h>
#include <omx_base_audio_port.h>

#include <omx_audiodec_component.h>
#include "omx_aacdec_component.h"
#include <string.h>
#include <tccaudio.h>
#include <OMX_TCC_Index.h>
#include "TCCFile.h"
#include "TCCMemory.h"
#include "cdk_sys.h"
#include "tcc_dxb_interface_type.h"

#if defined(HAVE_LINUX_PLATFORM)
#define USE_EXTERNAL_BUFFER 0
#define LOG_TAG	"OMX_TCC_AUDIODEC"
#include <utils/Log.h>

//#define DEBUG_ON
#ifdef DEBUG_ON
#define DBUG_MSG(x...)	ALOGD(x)
#else
#define DBUG_MSG(x...)
#endif

//#define MEM_DEBUG
#ifdef MEM_DEBUG
#define LOGMSG(x...) ALOGD(x)
#else
#define LOGMSG(x...)
#endif

// for APE -------------------------------------------
#define CODEC_SPECIFIC_MARKER 0x12345678
#define CODEC_SPECIFIC_MARKER_OFFSET 8
#define CODEC_SPECIFIC_INFO_OFFSET 4

typedef struct ape_dmx_exinput_t
{
	unsigned int m_uiInputBufferByteOffset;
	unsigned int m_uiCurrentFrames;
} ape_dmx_exinput_t;
// for APE -------------------------------------------

#endif

#include <cutils/properties.h>
#define		BUFFER_INDICATOR	   "TDXB"
#define		BUFFER_INDICATOR_SIZE  4

typedef struct _tagSignalQuality{
	unsigned int	SNR;
	unsigned int	PCBER;
	float			RSBER;
	double 		RSSI;
}tSignalQuality,*PSIGNALQUALITY;

OMX_U32 iTotalHandle = TOTAL_AUDIO_TRACK;

typedef long long (*pfnDemuxGetSTC)(int itype, long long lpts, unsigned int index, int log, void *pvApp);

//#define	USE_PCM_GAIN
#ifdef USE_PCM_GAIN
float gPCM_GAIN = 1.0;
#endif
// Error PCM Mute & Fade In  =====================
#define ERROR_PCM_MUTE		//Error PCM Mute Function
//#define ERROR_PCM_FADE_IN		//Error PCM Fade-In Function

#ifdef ERROR_PCM_MUTE
	#define MUTE_TIME_MSEC_SEL		1000	// Mute time [msec]
	#define MUTE_TIME_MSEC_ERR		0		// Mute time [msec]
	typedef struct
	{
		int MUTE_BYTE_SEL;
		int MUTE_BYTE_ERR;
		int mute_target_byte;
		int mute_flag_after_SetCh;			// For forced mute after the channel selection
	}stPcmMute_t;
	stPcmMute_t tPcmMute[2];
#endif	//ERROR_PCM_MUTE

#ifdef ERROR_PCM_FADE_IN
	#define FADE_IN_TIME_MSEC		100 	// Fade-In time [msec]
	#define FADE_IN_NUMBER			16 		//Fade-In Number, 5 or 16
	typedef struct
	{
		int FADE_IN_BYTE;
		int number_of_fade_in_frame;
		int fade_in_frame_size;
		int size_to_fade_in;
		int fade_in_tab_index;
		int fade_in_taget_byte;
		int frame_to_fade_in;
	}stPcmFade_in_t;
	stPcmFade_in_t tPcmFade_in[2];
#endif	//ERROR_PCM_FADE_IN

#if defined( ERROR_PCM_MUTE) || defined( ERROR_PCM_FADE_IN)
	//#define TASACHK
#endif
//=======================================

enum {
	TCCDXB_JAPAN = 0,
	TCCDXB_BRAZIL
};

int g_iDebugDecoderID=1;
#define	AUDIOMONITOR_TIME_DISCONTINUITY	1000	// unit: ms
struct _tagAudioMonitor {
	long long lastTimestamp;	// unit: ms
	int lastDuration;			// unit: ms
	int flag;					// 1: on-going, 0-stop
};

typedef struct {
	int guiSamplingRate;
	int guiChannels;
	pfnDemuxGetSTC gfnDemuxGetSTCValue;
	void* pvDemuxApp;
	struct _tagAudioMonitor stAudioMonitor[2];

#ifdef USE_PCM_GAIN
	float gPCM_GAIN = 1.0;
#endif
} TCC_ADEC_PRIVATE;

/*Log for seamless debug*/
//#define DEBUG_SEAMLESS
#ifdef DEBUG_SEAMLESS
#undef SEAMLESS_DBG_PRINTF
#define SEAMLESS_DBG_PRINTF printf
#else
#undef SEAMLESS_DBG_PRINTF
#define SEAMLESS_DBG_PRINTF ALOGD
#endif

//for seamless switching compensation
static int giDFSUseOnOff = 0;
static int giDFSInterval = 0;
static int giDFSStrength = 0;
static int giDFSNtimes = 0;
static int giDFSRange = 0;
static int giDFSgapadjust = 0;
static int giDFSgapadjust2 = 0;
static int giDFSmultiplier = 0;
static int giChannelIndex = 0;
static int giServiceId = 0;
static int giSubServiceId = 0;
static int giServiceLog = 0;
static int giSwitching = -1;
static int giSwitchingStart = 0;
static int giPrevIndex = -1;
static int giPrimaryService = -1;
static int giSubPrimaryService = -1;
static int giPrimaryServiceCheck = 0;
static int giSupportPrimary = 0;
static int giPartial_service_cnt = 0;
static int giDfs_start_flag = 0;
#if defined (SEAMLESS_SWITCHING_COMPENSATION)
#include <time.h>
static int giTime_delay = 0;
static int giTime_delay_monitor = 0;
static int dfs_state_monitor = 0;
static int giDFSQueue = 0;
static int giDFSinit = 0;
static int time_init = 0, interval_time_init = 0;
static int dfs_ret = -1;
static int dfs_found_cnt = 0;
static int giStrengthIndex = 0;
static int st_ret = 0;
static long t, dt, interval_st, interval_dt, saved_interval_dt = 0;
static int g_audio_buffer_init = 0;
static int g_dfs_on_check = 0;
static int g_dfs_result = 0;
static int g_dfs_db_broadcast_result = 0;
static int g_dfs_broadcast_result = 0;
static audio_ctx_t * g_audio_ctx = NULL;
/*saved dfs gap of each channel*/
typedef struct
{
	int saved_avg_dfs_gap;
	int saved_dfs_total;
	int saved_before_avg_dfs_gap;
}SAVED_DFS_GAP;
SAVED_DFS_GAP pstSaved_dfs_gap[100];

typedef struct
{
	short f_ldata[MAXIMUM_FRAME_SIZE];
	short f_rdata[MAXIMUM_FRAME_SIZE];
	short p_ldata[MAXIMUM_FRAME_SIZE];
	short p_rdata[MAXIMUM_FRAME_SIZE];
	int f_data_len;
	int p_data_len;
} SAVED_PCM_BUFFER;
SAVED_PCM_BUFFER pstAudio_buffer;

//#define DUMP_DFS_RESULT

static pthread_mutex_t g_DfsMutex;
static void tcc_dfs_lock_init(void)
{
	pthread_mutex_init(&g_DfsMutex, NULL);
}

static void tcc_dfs_lock_deinit(void)
{
 	pthread_mutex_destroy(&g_DfsMutex);
}

static void tcc_dfs_lock_on(void)
{
	pthread_mutex_lock(&g_DfsMutex);
}

static void tcc_dfs_lock_off(void)
{
	pthread_mutex_unlock(&g_DfsMutex);
}

static long long dfsclock()
{
	long long systime;
	struct timespec tspec;
	clock_gettime(CLOCK_MONOTONIC , &tspec);
	systime = (long long) tspec.tv_sec * 1000 + tspec.tv_nsec / 1000000;

	return systime;
}
#endif

void AudioMonInit (TCC_ADEC_PRIVATE *pPrivateData)
{
	ALOGD("In %s", __func__);

	int i;
	for (i=0; i < 2; i++) {
		pPrivateData->stAudioMonitor[i].lastTimestamp = 0;
		pPrivateData->stAudioMonitor[i].lastDuration = 0;
		pPrivateData->stAudioMonitor[i].flag = 0;
	}
}
void AudioMonRun(TCC_ADEC_PRIVATE *pPrivateData, unsigned int uiDecoderID, int start_stop)
{
	ALOGD("In %s(%d), flag=%d", __func__, uiDecoderID, start_stop);

	struct _tagAudioMonitor *pAudioMonitor;

	if (uiDecoderID < 2) {
		pAudioMonitor = &pPrivateData->stAudioMonitor[uiDecoderID];
		if (start_stop)
			pAudioMonitor->flag = 1;
		else
			pAudioMonitor->flag = 0;
	}
}
int AudioMonGetState (TCC_ADEC_PRIVATE *pPrivateData, unsigned int uiDecoderID)
{
	struct _tagAudioMonitor *pAudioMonitor;

	if (uiDecoderID < 2) {
		pAudioMonitor = &pPrivateData->stAudioMonitor[uiDecoderID];
		return pAudioMonitor->flag;
	} else
		return 0;
}
void AudioMonSet (TCC_ADEC_PRIVATE *pPrivateData, unsigned int uiDecoderID, long long pts, int duration)
{
	struct _tagAudioMonitor *pAudioMonitor;

	if (uiDecoderID < 2) {
		pAudioMonitor = &pPrivateData->stAudioMonitor[uiDecoderID];

		pAudioMonitor->lastTimestamp = pts;
		pAudioMonitor->lastDuration = duration;
	}
}
int AudioMonCheckContinuity (TCC_ADEC_PRIVATE *pPrivateData, unsigned int uiDecoderID, long long cur_pts)
{
	struct _tagAudioMonitor *pAudioMonitor;
	int	rtn = 0;
	long long expected_timestamp;

	if (uiDecoderID < 2) {
		pAudioMonitor = &pPrivateData->stAudioMonitor[uiDecoderID];
		if (pAudioMonitor->flag) {
			if(pAudioMonitor->lastTimestamp < cur_pts) {
				expected_timestamp = pAudioMonitor->lastTimestamp + (long long)pAudioMonitor->lastDuration;
				if ((expected_timestamp + AUDIOMONITOR_TIME_DISCONTINUITY) < cur_pts)
					rtn = 1;
			}
		}
	}
	return rtn;
}

// --------------------------------------------------
// memory management functions
// --------------------------------------------------
void * WRAPPER_malloc(unsigned int size)
{
	LOGMSG("malloc size %d", size);
	void * ptr = malloc(size);
	LOGMSG("malloc ptr %x", ptr);
	return ptr;
}

void * WRAPPER_calloc(unsigned int size, unsigned int count)
{
	LOGMSG("calloc size %d, count %d", size, count);
	void * ptr = calloc(size, count);
	LOGMSG("calloc ptr %x", ptr);
	return ptr;
}
void WRAPPER_free(void * ptr)
{
	LOGMSG("free %x", ptr);
	free(ptr);
	LOGMSG("free out");
}

void * WRAPPER_realloc(void * ptr, unsigned int size)
{
	LOGMSG("realloc ptr %x, size %d", ptr, size);
	return realloc(ptr, size);
}

void * WRAPPER_memcpy(void* dest, const void* src, unsigned int size)
{
	LOGMSG("memcpy dest ptr %x, src ptr %x, size %d", dest, src, size);
	return memcpy(dest, src, size);
}

void WRAPPER_memset(void* ptr, int val, unsigned int size)
{
	LOGMSG("memset ptr %x, val %d, size %d", ptr, val, size);
	memset(ptr, val, size);
}

void * WRAPPER_memmove(void* dest, const void* src, unsigned int size)
{
	LOGMSG("memmove dest %x, src %x, size %d", dest, src, size);
	return memmove(dest, src, size);
}

// ---------------------------------------------------------
// ---------------------------------------------------------

void AudioInfo_print(cdmx_info_t *info)
{
	DBUG_MSG("Audio Info Start=====================================>");

	DBUG_MSG("common!!");
	DBUG_MSG("m_iTotalNumber = %d", info->m_sAudioInfo.m_iTotalNumber);
	DBUG_MSG("m_iSamplePerSec = %d", info->m_sAudioInfo.m_iSamplePerSec);
	DBUG_MSG("m_iBitsPerSample = %d", info->m_sAudioInfo.m_iBitsPerSample);
	DBUG_MSG("m_iChannels = %d", info->m_sAudioInfo.m_iChannels);
	DBUG_MSG("m_iFormatId = %d", info->m_sAudioInfo.m_iFormatId);
	DBUG_MSG("--------------------------------------------------------------------");

	DBUG_MSG("extra info (common)!!");
	DBUG_MSG("m_pExtraData = 0x%x", info->m_sAudioInfo.m_pExtraData);
	DBUG_MSG("m_iExtraDataLen = %d", info->m_sAudioInfo.m_iExtraDataLen);
	DBUG_MSG("--------------------------------------------------------------------");

	DBUG_MSG("mp4 info!!");
	DBUG_MSG("m_iFramesPerSample = %d", info->m_sAudioInfo.m_iFramesPerSample);
	DBUG_MSG("m_iTrackTimeScale = %d", info->m_sAudioInfo.m_iTrackTimeScale);
	DBUG_MSG("m_iSamplesPerPacket = 0x%x", info->m_sAudioInfo.m_iSamplesPerPacket);
	DBUG_MSG("--------------------------------------------------------------------");

	DBUG_MSG("AVI info!!");
	DBUG_MSG("m_iNumAudioStream = %d", info->m_sAudioInfo.m_iNumAudioStream);
	DBUG_MSG("m_iCurrAudioIdx = %d", info->m_sAudioInfo.m_iCurrAudioIdx);
	DBUG_MSG("m_iBlockAlign = %d", info->m_sAudioInfo.m_iBlockAlign);
	DBUG_MSG("--------------------------------------------------------------------");

	DBUG_MSG("ASF info skip!!");
	DBUG_MSG("--------------------------------------------------------------------");

	DBUG_MSG("MPG info!!");
	DBUG_MSG("m_iBitRate = %d", info->m_sAudioInfo.m_iBitRate);
	DBUG_MSG("--------------------------------------------------------------------");

	DBUG_MSG("RM info!!");
	DBUG_MSG("ulActualRate = %d", info->m_sAudioInfo.m_iActualRate);
	DBUG_MSG("usAudioQuality = %d", info->m_sAudioInfo.m_sAudioQuality);
	DBUG_MSG("usFlavorIndex = %d", info->m_sAudioInfo.m_sFlavorIndex);
	DBUG_MSG("ulBitsPerFrame = %d", info->m_sAudioInfo.m_iBitsPerFrame);
	DBUG_MSG("ulGranularity = %d", info->m_sAudioInfo.m_iGranularity);
	DBUG_MSG("ulOpaqueDataSize = %d", info->m_sAudioInfo.m_iOpaqueDataSize);
	DBUG_MSG("pOpaqueData = 0x%x", info->m_sAudioInfo.m_pOpaqueData);
	DBUG_MSG("ulSamplesPerFrame = %d", info->m_sAudioInfo.m_iSamplesPerFrame);
	DBUG_MSG("frameSizeInBits = %d", info->m_sAudioInfo.m_iframeSizeInBits);
	DBUG_MSG("ulSampleRate = %d", info->m_sAudioInfo.m_iSampleRate);
	DBUG_MSG("cplStart = %d", info->m_sAudioInfo.m_scplStart);
	DBUG_MSG("cplQBits = %d", info->m_sAudioInfo.m_scplQBits);
	DBUG_MSG("nRegions = %d", info->m_sAudioInfo.m_snRegions);
	DBUG_MSG("dummy = %d", info->m_sAudioInfo.m_sdummy);
	DBUG_MSG("m_iFormatId = %d", info->m_sAudioInfo.m_iFormatId);
	DBUG_MSG("--------------------------------------------------------------------");

	DBUG_MSG("for Audio Only Path!!");
	DBUG_MSG("m_iMinDataSize = %d", info->m_sAudioInfo.m_iMinDataSize);
	DBUG_MSG("Audio Info End =====================================>");
}

void cdk_callback_func_init( void)
{
	cdk_malloc		= malloc;
	cdk_nc_malloc	= malloc;
	cdk_free		= free;
	cdk_nc_free		= free;
	cdk_memcpy		= memcpy;
	cdk_memset		= (memset_func*)memset;
	cdk_realloc		= realloc;
	cdk_memmove		= memmove;

#if 0
	cdk_sys_malloc_physical_addr = NULL;
	cdk_sys_free_physical_addr	 = NULL;
	cdk_sys_malloc_virtual_addr	 = NULL;
	cdk_sys_free_virtual_addr	 = NULL;
#endif

	cdk_fopen		= (fopen_func*)fopen;
	cdk_fread		= (fread_func*)fread;
	cdk_fseek		= (fseek_func*)fseek;
	cdk_ftell		= (ftell_func*)ftell;
	cdk_fwrite		= (fwrite_func*)fwrite;
	cdk_fclose		= (fclose_func*)fclose;
	cdk_unlink		= NULL;

	cdk_fseek64		= (fseek64_func*)fseek;
	cdk_ftell64		= (ftell64_func*)ftell;
#ifdef ERROR_PCM_MUTE
	tPcmMute[0].mute_flag_after_SetCh	= 1;		// Because this API is called when the channel is changed, mute_flag_after_SetCh is set ON(1).	for Fullseg
	tPcmMute[1].mute_flag_after_SetCh	= 1;		// Because this API is called when the channel is changed, mute_flag_after_SetCh is set ON(1).	for 1seg
	#ifdef TASACHK
	ALOGE("[TASACHK][AUDIO_SHOCK_NOISE] cdk_callback_func_init tPcmMute[0].mute_flag_after_SetCh=%d tPcmMute[1].mute_flag_after_SetCh=%d \n"
		,tPcmMute[0].mute_flag_after_SetCh
		,tPcmMute[1].mute_flag_after_SetCh
		);
	#endif //TASACHK
#endif //ERROR_PCM_MUTE
	return;
}

OMX_U32 CheckAudioStart(OMX_S16 nDevID, OMX_COMPONENTTYPE * openmaxStandComp)
{
	OMX_U8 ucCount, i;
	OMX_U32 ulAudioStart;
	OMX_U32 ulAudioFormat;
	OMX_U32 retry_count = 50;
	OMX_S32 comp_stc_delay[1];

	omx_audiodec_component_PrivateType *omx_private = openmaxStandComp->pComponentPrivate;
	TCC_ADEC_PRIVATE *pPrivateData = (TCC_ADEC_PRIVATE*) omx_private->pPrivateData;

	if(omx_private->pAudioDecPrivate[nDevID].stAudioStart.nState != TCC_DXBAUDIO_OMX_Dec_None)
	{
		ulAudioStart = omx_private->pAudioDecPrivate[nDevID].stAudioStart.nState;
		ulAudioFormat = omx_private->pAudioDecPrivate[nDevID].stAudioStart.nFormat;
		omx_private->pAudioDecPrivate[nDevID].stAudioStart.nState = TCC_DXBAUDIO_OMX_Dec_None;
		omx_private->pAudioDecPrivate[nDevID].stAudioStart.nFormat = 0;

		pthread_mutex_lock(&omx_private->pAudioDecPrivate[nDevID].stAudioStart.mutex);
		if(ulAudioStart == TCC_DXBAUDIO_OMX_Dec_Stop)
		{
			omx_private->pAudioDecPrivate[nDevID].bAudioStarted = OMX_FALSE;

			while(retry_count > 0){
				if(omx_private->pAudioDecPrivate[nDevID].bAudioInDec == OMX_FALSE){
					break;
				}
				usleep(1000);
				retry_count--;
			}

#ifdef SEAMLESS_SWITCHING_COMPENSATION
		if(nDevID == 0){
			comp_stc_delay[0] = 0;
			// broadcast a event for time delay.
			(*(omx_private->callbacks->EventHandler))
			(openmaxStandComp,
			omx_private->callbackData,
			OMX_EventVendorCompenSTCDelay,
			0,
			0,
			(OMX_PTR)comp_stc_delay);

			tcc_dfs_lock_on();
			tcc_dfs_buffer_clear();
			tcc_dfs_stop_process();
			tcc_dfs_deinit();
			tcc_dfs_saved_info_clear();
			giSwitching = -1;
			giServiceLog = 0;
			giDfs_start_flag = 0;
			giSwitchingStart = 0;
			giPrimaryServiceCheck = 0;
			tcc_dfs_lock_off();
		}
#endif
			DBUG_MSG("%s - TCC_DXBAUDIO_OMX_Dec_Stop\n", __func__);

			AudioMonRun(pPrivateData, nDevID, 0);
		}
		else if(ulAudioStart == TCC_DXBAUDIO_OMX_Dec_Start)
		{
			OMX_ERRORTYPE err = OMX_ErrorNone;
			dxb_omx_audiodec_component_LibDeinit (nDevID, omx_private);

			if(ulAudioFormat == TCC_DXBAUDIO_OMX_Codec_AAC_ADTS)
			{
				if(omx_private->pAudioDecPrivate[nDevID].audio_coding_type != OMX_AUDIO_CodingAAC)
				{
					err = dxb_omx_audiodec_component_Change_AACdec(nDevID, openmaxStandComp, AUDIO_DEC_AAC_NAME, AUDIO_SUBTYPE_AAC_ADTS);
				}
				else
				{
					if(omx_private->pAudioDecPrivate[nDevID].cdmx_info.m_sAudioInfo.m_ulSubType != AUDIO_SUBTYPE_AAC_ADTS)
						err = dxb_omx_audiodec_component_Change_AACdec(nDevID, openmaxStandComp, AUDIO_DEC_AAC_NAME, AUDIO_SUBTYPE_AAC_ADTS);
				}
			}
			else if(ulAudioFormat == TCC_DXBAUDIO_OMX_Codec_AAC_LATM)
			{
				if(omx_private->pAudioDecPrivate[nDevID].audio_coding_type != OMX_AUDIO_CodingAAC)
				{
					err = dxb_omx_audiodec_component_Change_AACdec(nDevID, openmaxStandComp, AUDIO_DEC_AAC_NAME, AUDIO_SUBTYPE_AAC_LATM);
				}
				else
				{
					if(omx_private->pAudioDecPrivate[nDevID].cdmx_info.m_sAudioInfo.m_ulSubType != AUDIO_SUBTYPE_AAC_LATM)
						err = dxb_omx_audiodec_component_Change_AACdec(nDevID, openmaxStandComp, AUDIO_DEC_AAC_NAME, AUDIO_SUBTYPE_AAC_LATM);
				}
			}
			else if(ulAudioFormat == TCC_DXBAUDIO_OMX_Codec_AAC_PLUS)
			{
				if(omx_private->pAudioDecPrivate[nDevID].audio_coding_type != OMX_AUDIO_CodingAAC)
				{
					err = dxb_omx_audiodec_component_Change_AACdec(nDevID, openmaxStandComp, AUDIO_DEC_AAC_NAME, AUDIO_SUBTYPE_NONE);
				}
				else
				{
					if(omx_private->pAudioDecPrivate[nDevID].cdmx_info.m_sAudioInfo.m_ulSubType != AUDIO_SUBTYPE_NONE)
						err = dxb_omx_audiodec_component_Change_AACdec(nDevID, openmaxStandComp, AUDIO_DEC_AAC_NAME, AUDIO_SUBTYPE_NONE);
				}
			}

			dxb_omx_audiodec_component_LibInit (nDevID, omx_private);
			if(err == OMX_ErrorNone)
				omx_private->pAudioDecPrivate[nDevID].bAudioStarted = OMX_TRUE;
			else
				omx_private->pAudioDecPrivate[nDevID].bAudioStarted = OMX_FALSE;

			omx_private->pAudioDecPrivate[nDevID].bAudioPaused = OMX_FALSE;

			DBUG_MSG("%s - TCC_DXBAUDIO_OMX_Dec_Start, Format=0x%x\n", __func__, ulAudioFormat);

			AudioMonInit(pPrivateData);
		}
		pthread_mutex_unlock(&omx_private->pAudioDecPrivate[nDevID].stAudioStart.mutex);
	}
	return 0;
}

OMX_ERRORTYPE dxb_omx_audiodec_component_LibInit(OMX_S16 nDevID, omx_audiodec_component_PrivateType* omx_audiodec_component_Private)
{
	cdk_callback_func_init();
	omx_audiodec_component_Private->isNewBuffer = 1;
	omx_audiodec_component_Private->pAudioDecPrivate[nDevID].bPrevDecFail = OMX_FALSE;
	omx_audiodec_component_Private->pAudioDecPrivate[nDevID].iNumOfSeek = 0;
	omx_audiodec_component_Private->pAudioDecPrivate[nDevID].iStartTS = 0;
	omx_audiodec_component_Private->pAudioDecPrivate[nDevID].iSamples = 0;

	memset(omx_audiodec_component_Private->pAudioDecPrivate[nDevID].gucAudioInitBuffer, 0x0, AUDIO_INIT_BUFFER_SIZE);

	omx_audiodec_component_Private->pAudioDecPrivate[nDevID].guiAudioInitBufferIndex = 0;
	return OMX_ErrorNone;
}

OMX_ERRORTYPE dxb_omx_audiodec_component_LibDeinit(OMX_S16 nDevID, omx_audiodec_component_PrivateType* omx_audiodec_component_Private)
{
	if( omx_audiodec_component_Private->pAudioDecPrivate[nDevID].decode_ready == OMX_TRUE )
	{
		if( cdk_adec_close(&omx_audiodec_component_Private->pAudioDecPrivate[nDevID].cdk_core, &omx_audiodec_component_Private->pAudioDecPrivate[nDevID].gsADec) < 0 )
		{
			DBUG_MSG("Audio DEC close error\n");
			return OMX_ErrorHardware;
		}
	}
	omx_audiodec_component_Private->pAudioDecPrivate[nDevID].decode_ready = OMX_FALSE;
	return OMX_ErrorNone;
}

OMX_ERRORTYPE dxb_omx_audiodec_component_MessageHandler(OMX_COMPONENTTYPE* openmaxStandComp, internalRequestMessageType *message)
{
	OMX_U32 nDevID;
	omx_audiodec_component_PrivateType* omx_audiodec_component_Private = (omx_audiodec_component_PrivateType*)openmaxStandComp->pComponentPrivate;
	OMX_ERRORTYPE err;
	OMX_STATETYPE eCurrentState = omx_audiodec_component_Private->state;
	DBUG_MSG("In %s\n", __func__);

	/** Execute the base message handling */
	err = dxb_omx_base_component_MessageHandler(openmaxStandComp, message);

	if (message->messageType == OMX_CommandStateSet){
		if ((message->messageParam == OMX_StateExecuting) && (eCurrentState == OMX_StateIdle)) {
			for(nDevID=0 ; nDevID<iTotalHandle ; nDevID++) {
				err = dxb_omx_audiodec_component_LibInit(nDevID, omx_audiodec_component_Private);
				memset(&omx_audiodec_component_Private->pAudioDecPrivate[nDevID].stAudioStart,0x0, sizeof(AudioStartInfo));
				pthread_mutex_init(&omx_audiodec_component_Private->pAudioDecPrivate[nDevID].stAudioStart.mutex, NULL);
			}
			if(err!=OMX_ErrorNone) {
				ALOGE("In %s Audio decoder library Init Failed Error=%x\n",__func__,err);
				return err;
			}
		} else if ((message->messageParam == OMX_StateIdle) && (eCurrentState == OMX_StateExecuting || eCurrentState == OMX_StatePause)) {
			for(nDevID=0 ; nDevID<iTotalHandle ; nDevID++) {
				err = dxb_omx_audiodec_component_LibDeinit(nDevID, omx_audiodec_component_Private);
				pthread_mutex_destroy(&omx_audiodec_component_Private->pAudioDecPrivate[nDevID].stAudioStart.mutex);
			}
			if(err!=OMX_ErrorNone) {
				ALOGE("In %s Audio decoder library Deinit Failed Error=%x\n",__func__,err);
				return err;
			}
		}
	}
	return err;

}

OMX_ERRORTYPE dxb_omx_audiodec_component_GetExtensionIndex(
  OMX_IN  OMX_HANDLETYPE hComponent,
  OMX_IN  OMX_STRING cParameterName,
  OMX_OUT OMX_INDEXTYPE* pIndexType)
{

	DBUG_MSG("In  %s \n",__func__);
	if(strcmp(cParameterName,TCC_AUDIO_FILE_OPEN_STRING) == 0) {
		*pIndexType = OMX_IndexVendorParamFileOpen;
	} else if(strcmp(cParameterName,TCC_AUDIO_POWERSPECTUM_STRING) == 0){
		*pIndexType = OMX_IndexVendorConfigPowerSpectrum;
	} else if(strcmp(cParameterName,TCC_AUDIO_MEDIA_INFO_STRING) == 0){
		*pIndexType = OMX_IndexVendorParamMediaInfo;
	}else if(strcmp(cParameterName,TCC_AUDIO_ENERGYVOLUME_STRING) == 0){
		*pIndexType = OMX_IndexVendorConfigEnergyVolume;
	}else{
		return OMX_ErrorBadParameter;
	}

  return OMX_ErrorNone;
}

/** The destructor */
OMX_ERRORTYPE dxb_omx_audiodec_component_Destructor(OMX_COMPONENTTYPE *openmaxStandComp)
{

	omx_audiodec_component_PrivateType* omx_audiodec_component_Private = openmaxStandComp->pComponentPrivate;
	TCC_ADEC_PRIVATE *pPrivateData = (TCC_ADEC_PRIVATE*) omx_audiodec_component_Private->pPrivateData;
	OMX_U32 i;
	char value[PROPERTY_VALUE_MAX];

#ifdef SEAMLESS_SWITCHING_COMPENSATION
	tcc_dfs_lock_on();
	tcc_dfs_stop_process();
	giSwitching = -1;
	giSwitchingStart = 0;
	tcc_dfs_lock_off();
	CloseDfsDB_Table();
#endif

	if(omx_audiodec_component_Private->pAudioDecPrivate) {
		//AudioSettingChangeRequest(openmaxStandComp, omx_audiodec_component_Private->iSPDIFMode);
		TCC_fo_free (__func__, __LINE__,omx_audiodec_component_Private->pAudioDecPrivate);
	}
	omx_audiodec_component_Private->pAudioDecPrivate = NULL;

	/* frees port/s */
	if (omx_audiodec_component_Private->ports)
	{
		for (i=0; i < omx_audiodec_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts; i++)
		{
			if(omx_audiodec_component_Private->ports[i])
				omx_audiodec_component_Private->ports[i]->PortDestructor(omx_audiodec_component_Private->ports[i]);
		}

		TCC_fo_free (__func__, __LINE__,omx_audiodec_component_Private->ports);
		omx_audiodec_component_Private->ports=NULL;
	}

	DBUG_MSG("Destructor of the Audio decoder component is called\n");

	omx_audiodec_component_Private->audio_decode_state = AUDIO_MAX_EVENT;

	//omx_mp3dec_component_Private->audio_valid_info = 0;

	TCC_fo_free(__func__, __LINE__, omx_audiodec_component_Private->pPrivateData);

	dxb_omx_base_filter_Destructor(openmaxStandComp);
#ifdef SEAMLESS_SWITCHING_COMPENSATION
	tcc_dfs_lock_deinit();
#endif
	return OMX_ErrorNone;

}

/** this function sets the parameter values regarding audio format & index */
OMX_ERRORTYPE dxb_omx_audiodec_component_SetParameter(
  OMX_IN  OMX_HANDLETYPE hComponent,
  OMX_IN  OMX_INDEXTYPE nParamIndex,
  OMX_IN  OMX_PTR ComponentParameterStructure)
{
	OMX_S32   *piArg;
	OMX_ERRORTYPE err = OMX_ErrorNone;
	OMX_AUDIO_PARAM_PORTFORMATTYPE *pAudioPortFormat;
	OMX_AUDIO_PARAM_PCMMODETYPE* pAudioPcmMode;
	OMX_PARAM_COMPONENTROLETYPE * pComponentRole;
	OMX_AUDIO_CONFIG_SEEKTYPE* gettime;
	OMX_AUDIO_CONFIG_INFOTYPE *info;

	omx_base_audio_PortType *port;
	OMX_U32 portIndex;

	/* Check which structure we are being fed and make control its header */
	OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
	omx_audiodec_component_PrivateType* omx_audiodec_component_Private = openmaxStandComp->pComponentPrivate;
	omx_aacdec_component_PrivateType* AACPrivate;
	omx_ac3dec_component_PrivateType* AC3Private;
	omx_apedec_component_PrivateType* APEPrivate;
	omx_dtsdec_component_PrivateType* DTSPrivate;
	omx_flacdec_component_PrivateType* FLACPrivate;
	omx_mp2dec_component_PrivateType* MP2Private;
	omx_mp3dec_component_PrivateType* MP3Private;
	omx_radec_component_PrivateType* RAPrivate;
	omx_vorbisdec_component_PrivateType* VORBISPrivate;
	omx_wmadec_component_PrivateType* WMAPrivate;
	omx_wavdec_component_PrivateType* WAVPrivate;

	OMX_AUDIO_PARAM_AACPROFILETYPE* pAudioAac;
	OMX_AUDIO_PARAM_AC3TYPE * pAudioAc3;
	OMX_AUDIO_PARAM_APETYPE * pAudioApe;
	OMX_AUDIO_PARAM_DTSTYPE * pAudioDts;
	OMX_AUDIO_PARAM_FLACTYPE * pAudioFlac;
	OMX_AUDIO_PARAM_MP2TYPE * pAudioMp2;
	OMX_AUDIO_PARAM_MP3TYPE * pAudioMp3;
	OMX_AUDIO_PARAM_RATYPE * pAudioRa;
	OMX_AUDIO_PARAM_VORBISTYPE * pAudioVorbis;
	OMX_AUDIO_PARAM_WMATYPE * pAudioWma;

	TCC_ADEC_PRIVATE *pPrivateData = (TCC_ADEC_PRIVATE*) omx_audiodec_component_Private->pPrivateData;
	OMX_U32 nDevID =0;

	DBUG_MSG("   Setting parameter 0x%x\n", nParamIndex);

	switch(nParamIndex)
	{
		case OMX_IndexParamAudioPortFormat:
			pAudioPortFormat = (OMX_AUDIO_PARAM_PORTFORMATTYPE*)ComponentParameterStructure;
			portIndex = pAudioPortFormat->nPortIndex;
			/*Check Structure Header and verify component state*/
			err = dxb_omx_base_component_ParameterSanityCheck(hComponent, portIndex, pAudioPortFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
			if(err!=OMX_ErrorNone)
			{
				DBUG_MSG("In %s Parameter Check Error=%x\n",__func__,err);
				break;
			}
			if (portIndex <= 2)
			{
				port = (omx_base_audio_PortType *) omx_audiodec_component_Private->ports[portIndex];
				memcpy(&port->sAudioParam, pAudioPortFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
			}
			else
			{
				err = OMX_ErrorBadPortIndex;
			}
		break;

		case OMX_IndexParamStandardComponentRole:
			pComponentRole = (OMX_PARAM_COMPONENTROLETYPE*)ComponentParameterStructure;
			if (!strcmp( (char*) pComponentRole->cRole, AUDIO_DEC_TCC_AAC_ROLE))
			{
				for(nDevID=0 ; nDevID<iTotalHandle ; nDevID++)
					omx_audiodec_component_Private->pAudioDecPrivate[nDevID].audio_coding_type = OMX_AUDIO_CodingAAC;
			}
			else if (!strcmp( (char*) pComponentRole->cRole, AUDIO_DEC_AC3_ROLE))
			{
				for(nDevID=0 ; nDevID<iTotalHandle ; nDevID++)
					omx_audiodec_component_Private->pAudioDecPrivate[nDevID].audio_coding_type = OMX_AUDIO_CodingAC3;
			}
			else if (!strcmp( (char*) pComponentRole->cRole, AUDIO_DEC_APE_ROLE))
			{
				for(nDevID=0 ; nDevID<iTotalHandle ; nDevID++)
					omx_audiodec_component_Private->pAudioDecPrivate[nDevID].audio_coding_type = OMX_AUDIO_CodingAPE;
			}
			else if (!strcmp( (char*) pComponentRole->cRole, AUDIO_DEC_DTS_ROLE))
			{
				for(nDevID=0 ; nDevID<iTotalHandle ; nDevID++)
					omx_audiodec_component_Private->pAudioDecPrivate[nDevID].audio_coding_type = OMX_AUDIO_CodingDTS;
			}
			else if (!strcmp( (char*) pComponentRole->cRole, AUDIO_DEC_FLAC_ROLE))
			{
				for(nDevID=0 ; nDevID<iTotalHandle ; nDevID++)
					omx_audiodec_component_Private->pAudioDecPrivate[nDevID].audio_coding_type = OMX_AUDIO_CodingFLAC;
			}
			else if (!strcmp( (char*) pComponentRole->cRole, AUDIO_DEC_MP2_ROLE))
			{
				for(nDevID=0 ; nDevID<iTotalHandle ; nDevID++)
					omx_audiodec_component_Private->pAudioDecPrivate[nDevID].audio_coding_type = OMX_AUDIO_CodingMP2;
			}
			else if (!strcmp( (char*) pComponentRole->cRole, AUDIO_DEC_MP3_ROLE))
			{
				for(nDevID=0 ; nDevID<iTotalHandle ; nDevID++)
					omx_audiodec_component_Private->pAudioDecPrivate[nDevID].audio_coding_type = OMX_AUDIO_CodingMP3;
			}
			else if (!strcmp( (char*) pComponentRole->cRole, AUDIO_DEC_RA_ROLE))
			{
				for(nDevID=0 ; nDevID<iTotalHandle ; nDevID++)
					omx_audiodec_component_Private->pAudioDecPrivate[nDevID].audio_coding_type = OMX_AUDIO_CodingRA;
			}
			else if (!strcmp( (char*) pComponentRole->cRole, AUDIO_DEC_VORBIS_ROLE))
			{
				for(nDevID=0 ; nDevID<iTotalHandle ; nDevID++)
					omx_audiodec_component_Private->pAudioDecPrivate[nDevID].audio_coding_type = OMX_AUDIO_CodingVORBIS;
			}
			else if (!strcmp( (char*) pComponentRole->cRole, AUDIO_DEC_PCM_ROLE))
			{
				for(nDevID=0 ; nDevID<iTotalHandle ; nDevID++)
					omx_audiodec_component_Private->pAudioDecPrivate[nDevID].audio_coding_type = OMX_AUDIO_CodingPCM;
			}
			else if (!strcmp( (char*) pComponentRole->cRole, AUDIO_DEC_TCC_WMA_ROLE))
			{
				for(nDevID=0 ; nDevID<iTotalHandle ; nDevID++)
					omx_audiodec_component_Private->pAudioDecPrivate[nDevID].audio_coding_type = OMX_AUDIO_CodingWMA;
			}
			else
			{
				err = OMX_ErrorBadParameter;
			}
		break;

		// --------------------------------------------------------------------------------
		// codec specific parameters -------------------------------------------------------

		case OMX_IndexParamAudioAac:
			pAudioAac = (OMX_AUDIO_PARAM_AACPROFILETYPE*) ComponentParameterStructure;
			portIndex = pAudioAac->nPortIndex;
			err = dxb_omx_base_component_ParameterSanityCheck(hComponent,portIndex,pAudioAac,sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));

			AACPrivate = (omx_aacdec_component_PrivateType *)omx_audiodec_component_Private;
			if(err!=OMX_ErrorNone)
			{
				DBUG_MSG("In %s Parameter Check Error=%x\n",__func__,err);
				break;
			}
			if (pAudioAac->nPortIndex == 0)
			{
				memcpy(&AACPrivate->pAudioAac, pAudioAac, sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));
			}
			else
			{
				err = OMX_ErrorBadPortIndex;
			}
		break;

		case OMX_IndexParamAudioAC3:
			pAudioAc3 = (OMX_AUDIO_PARAM_AC3TYPE*) ComponentParameterStructure;
			portIndex = pAudioAc3->nPortIndex;
			err = dxb_omx_base_component_ParameterSanityCheck(hComponent,portIndex,pAudioAc3,sizeof(OMX_AUDIO_PARAM_AC3TYPE));

			AC3Private = (omx_ac3dec_component_PrivateType *)omx_audiodec_component_Private;
			if(err!=OMX_ErrorNone)
			{
				DBUG_MSG("[DBG_AC3]  ==> In %s Parameter Check Error=%x\n",__func__,err);
				break;
			}
			if (pAudioAc3->nPortIndex == 0)
			{
				memcpy(&AC3Private->pAudioAc3, pAudioAc3, sizeof(OMX_AUDIO_PARAM_AC3TYPE));
			}
			else
			{
				err = OMX_ErrorBadPortIndex;
			}
		break;

		case OMX_IndexParamAudioDTS:
			pAudioDts = (OMX_AUDIO_PARAM_DTSTYPE*) ComponentParameterStructure;
			portIndex = pAudioDts->nPortIndex;
			err = dxb_omx_base_component_ParameterSanityCheck(hComponent,portIndex,pAudioDts,sizeof(OMX_AUDIO_PARAM_DTSTYPE));

			DTSPrivate = (omx_dtsdec_component_PrivateType *)omx_audiodec_component_Private;
			if(err!=OMX_ErrorNone)
			{
				DBUG_MSG("In %s Parameter Check Error=%x\n",__func__,err);
				break;
			}
			if (pAudioDts->nPortIndex == 0)
			{
				memcpy(&DTSPrivate->pAudioDts, pAudioDts, sizeof(OMX_AUDIO_PARAM_DTSTYPE));
			}
			else
			{
				err = OMX_ErrorBadPortIndex;
			}
		break;

		case OMX_IndexParamAudioMp2:
			pAudioMp2 = (OMX_AUDIO_PARAM_MP2TYPE*) ComponentParameterStructure;
			portIndex = pAudioMp2->nPortIndex;
			err = dxb_omx_base_component_ParameterSanityCheck(hComponent,portIndex,pAudioMp2,sizeof(OMX_AUDIO_PARAM_MP2TYPE));

			MP2Private = (omx_mp2dec_component_PrivateType *)omx_audiodec_component_Private;
			if(err!=OMX_ErrorNone)
			{
				DBUG_MSG("In %s Parameter Check Error=%x\n",__func__,err);
				break;
			}
			if (pAudioMp2->nPortIndex == 0)
			{
				memcpy(&MP2Private->pAudioMp2, pAudioMp2, sizeof(OMX_AUDIO_PARAM_MP2TYPE));
			}
			else
			{
				err = OMX_ErrorBadPortIndex;
			}
		break;

		case OMX_IndexParamAudioMp3:
			pAudioMp3 = (OMX_AUDIO_PARAM_MP3TYPE*) ComponentParameterStructure;
			portIndex = pAudioMp3->nPortIndex;
			err = dxb_omx_base_component_ParameterSanityCheck(hComponent,portIndex,pAudioMp3,sizeof(OMX_AUDIO_PARAM_MP3TYPE));

			MP3Private = (omx_mp3dec_component_PrivateType *)omx_audiodec_component_Private;
			if(err!=OMX_ErrorNone)
			{
				DBUG_MSG("In %s Parameter Check Error=%x\n",__func__,err);
				break;
			}
			if (pAudioMp3->nPortIndex == 0)
			{
				memcpy(&MP3Private->pAudioMp3, pAudioMp3, sizeof(OMX_AUDIO_PARAM_MP3TYPE));
			}
			else
			{
				err = OMX_ErrorBadPortIndex;
			}
		break;


		// codec specific ---------------------------------------------------------------
		// ------------------------------------------------------------------------------

		case OMX_IndexVendorAudioExtraData:
		{
			OMX_VENDOR_EXTRADATATYPE* pExtradata;
			pExtradata = (OMX_VENDOR_EXTRADATATYPE*)ComponentParameterStructure;

			if (pExtradata->nPortIndex <= 2) {
				/** copy the extradata in the codec context private structure */
////				memcpy(&omx_mp3dec_component_Private->audioinfo, pExtradata->pData, sizeof(rm_audio_info));
			} else {
				err = OMX_ErrorBadPortIndex;
			}
		}
		break;

		case OMX_IndexVendorParamMediaInfo:
			info = (OMX_AUDIO_CONFIG_INFOTYPE*) ComponentParameterStructure;

			pPrivateData->guiSamplingRate = info->nSamplingRate;
			pPrivateData->guiChannels = info->nChannels;
			//omx_mp3dec_component_Private->pAudioMp3.nChannels = info->nChannels;
			//omx_mp3dec_component_Private->pAudioMp3.nBitRate = info->nBitPerSample;
			//omx_mp3dec_component_Private->pAudioMp3.nSampleRate = info->nSamplingRate;
			break;


		case OMX_IndexVendorParamAudioSetStart:
			{
				OMX_AUDIO_PARAM_STARTTYPE *pStartInfo;

				pStartInfo = (OMX_AUDIO_PARAM_STARTTYPE*) ComponentParameterStructure;
				if(pStartInfo->nDevID >=(OMX_U32) iTotalHandle)
					break;

				omx_audiodec_component_Private->pAudioDecPrivate[pStartInfo->nDevID].stAudioStart.nFormat = pStartInfo->nAudioFormat;
				if(pStartInfo->bStartFlag)
				{
					omx_audiodec_component_Private->pAudioDecPrivate[pStartInfo->nDevID].stAudioStart.nState = TCC_DXBAUDIO_OMX_Dec_Start;
				}
				else
				{
					omx_audiodec_component_Private->pAudioDecPrivate[pStartInfo->nDevID].stAudioStart.nState = TCC_DXBAUDIO_OMX_Dec_Stop;
				}
				CheckAudioStart(pStartInfo->nDevID, openmaxStandComp);
			}
			break;

		case OMX_IndexVendorParamAudioSetISDBTMode:
			{
				omx_audiodec_component_Private->dxb_standard = DxB_STANDARD_ISDBT;
			}
			break;
		case OMX_IndexVendorParamAudioSetGetSignalStrength:
			{
				omx_audiodec_component_Private->callbackfunction = (void *)ComponentParameterStructure;
			}
			break;

		case OMX_IndexVendorParamDxBSetBitrate:
			{
				OMX_S16 nDevID;
				for(nDevID=0 ; nDevID<(OMX_S16)iTotalHandle ; nDevID++) {
					omx_audiodec_component_Private->pAudioDecPrivate[nDevID].cdmx_info.m_sAudioInfo.m_iBitRate = ComponentParameterStructure;
				}
			}
			break;

		case OMX_IndexVendorParamAudioOutputCh:
			{
				OMX_U32 *ulDemuxId = (OMX_U32 *)ComponentParameterStructure;
				omx_audiodec_component_Private->outDevID = *ulDemuxId;
			}
			break;

		case OMX_IndexVendorParamAudioAacDualMono:
			{
				TCC_DXBAUDIO_DUALMONO_TYPE	*paudio_sel;

				paudio_sel = (TCC_DXBAUDIO_DUALMONO_TYPE *)ComponentParameterStructure;
				AACPrivate = (omx_aacdec_component_PrivateType *)omx_audiodec_component_Private;
				AACPrivate->eAudioDualMonoSel = *paudio_sel;
				if ((AACPrivate->eAudioDualMonoSel != TCC_DXBAUDIO_DUALMONO_Left)
					&& (AACPrivate->eAudioDualMonoSel != TCC_DXBAUDIO_DUALMONO_Right)
					&& (AACPrivate->eAudioDualMonoSel != TCC_DXBAUDIO_DUALMONO_LeftNRight)
				)
					AACPrivate->eAudioDualMonoSel = TCC_DXBAUDIO_DUALMONO_LeftNRight;
			}
			break;

		case OMX_IndexVendorParamStereoMode:
			{
				TCC_DXBAUDIO_DUALMONO_TYPE *peStereoMode = (TCC_DXBAUDIO_DUALMONO_TYPE *)ComponentParameterStructure;
				omx_audiodec_component_Private->eStereoMode = *peStereoMode;
			}
			break;

		case OMX_IndexVendorParamSetAudioPause:
			{
				OMX_U32 ulDemuxId;
				piArg = (OMX_S32 *) ComponentParameterStructure;
				ulDemuxId = (OMX_U32) piArg[0];
				omx_audiodec_component_Private->pAudioDecPrivate[ulDemuxId].bAudioPaused = (OMX_BOOL) piArg[1];
			}
			break;

#ifdef	SUPPORT_PVR
		case OMX_IndexVendorParamPlayStartRequest:
			{
				OMX_S32 ulPvrId;
				OMX_S8 *pucFileName;

				piArg = (OMX_S32 *) ComponentParameterStructure;
				ulPvrId = (OMX_S32) piArg[0];
				pucFileName = (OMX_S8 *) piArg[1];

				pthread_mutex_lock(&omx_audiodec_component_Private->pAudioDecPrivate[nDevID].stAudioStart.mutex);
				if ((omx_audiodec_component_Private->pAudioDecPrivate[ulPvrId].nPVRFlags & PVR_FLAG_START) == 0)
				{
					omx_audiodec_component_Private->pAudioDecPrivate[ulPvrId].nPVRFlags = PVR_FLAG_START | PVR_FLAG_CHANGED;
				}
				pthread_mutex_unlock(&omx_audiodec_component_Private->pAudioDecPrivate[nDevID].stAudioStart.mutex);
			}
			break;

		case OMX_IndexVendorParamPlayStopRequest:
			{
				OMX_S32 ulPvrId;

				piArg = (OMX_S32 *) ComponentParameterStructure;
				ulPvrId = (OMX_S32) piArg[0];

				pthread_mutex_lock(&omx_audiodec_component_Private->pAudioDecPrivate[nDevID].stAudioStart.mutex);
				if (omx_audiodec_component_Private->pAudioDecPrivate[ulPvrId].nPVRFlags & PVR_FLAG_START)
				{
					omx_audiodec_component_Private->pAudioDecPrivate[ulPvrId].nPVRFlags = PVR_FLAG_CHANGED;
				}
				pthread_mutex_unlock(&omx_audiodec_component_Private->pAudioDecPrivate[nDevID].stAudioStart.mutex);
			}
			break;

		case OMX_IndexVendorParamSeekToRequest:
			{
				OMX_S32 ulPvrId, nSeekTime;

				piArg = (OMX_S32 *) ComponentParameterStructure;
				ulPvrId = (OMX_S32) piArg[0];
				nSeekTime = (OMX_S32) piArg[1];

				pthread_mutex_lock(&omx_audiodec_component_Private->pAudioDecPrivate[nDevID].stAudioStart.mutex);
				if (omx_audiodec_component_Private->pAudioDecPrivate[ulPvrId].nPVRFlags & PVR_FLAG_START)
				{
					if (nSeekTime < 0)
					{
						if (omx_audiodec_component_Private->pAudioDecPrivate[ulPvrId].nPVRFlags & PVR_FLAG_TRICK)
						{
							omx_audiodec_component_Private->pAudioDecPrivate[ulPvrId].nPVRFlags &= ~PVR_FLAG_TRICK;
							omx_audiodec_component_Private->pAudioDecPrivate[ulPvrId].nPVRFlags |= PVR_FLAG_CHANGED;
						}
					}
					else
					{
						if ((omx_audiodec_component_Private->pAudioDecPrivate[ulPvrId].nPVRFlags & PVR_FLAG_TRICK) == 0)
						{
							omx_audiodec_component_Private->pAudioDecPrivate[ulPvrId].nPVRFlags |= (PVR_FLAG_TRICK | PVR_FLAG_CHANGED);
						}
					}
				}
				pthread_mutex_unlock(&omx_audiodec_component_Private->pAudioDecPrivate[nDevID].stAudioStart.mutex);
			}
			break;

		case OMX_IndexVendorParamPlaySpeed:
			{
				OMX_S32 ulPvrId, nPlaySpeed;

				piArg = (OMX_S32 *) ComponentParameterStructure;
				ulPvrId = (OMX_S32) piArg[0];
				nPlaySpeed = (OMX_S32) piArg[1];

				pthread_mutex_lock(&omx_audiodec_component_Private->pAudioDecPrivate[nDevID].stAudioStart.mutex);
				if (omx_audiodec_component_Private->pAudioDecPrivate[ulPvrId].nPVRFlags & PVR_FLAG_START)
				{
					if (nPlaySpeed == 1)
					{
						if (omx_audiodec_component_Private->pAudioDecPrivate[ulPvrId].nPVRFlags & PVR_FLAG_TRICK)
						{
							omx_audiodec_component_Private->pAudioDecPrivate[ulPvrId].nPVRFlags &= ~PVR_FLAG_TRICK;
							omx_audiodec_component_Private->pAudioDecPrivate[ulPvrId].nPVRFlags |= PVR_FLAG_CHANGED;
						}
					}
					else
					{
						if ((omx_audiodec_component_Private->pAudioDecPrivate[ulPvrId].nPVRFlags & PVR_FLAG_TRICK) == 0)
						{
							omx_audiodec_component_Private->pAudioDecPrivate[ulPvrId].nPVRFlags |= (PVR_FLAG_TRICK | PVR_FLAG_CHANGED);
						}
					}
				}
				pthread_mutex_unlock(&omx_audiodec_component_Private->pAudioDecPrivate[nDevID].stAudioStart.mutex);
			}
			break;

		case OMX_IndexVendorParamPlayPause:
			{
				OMX_S32 ulPvrId, nPlayPause;

				piArg = (OMX_S32 *) ComponentParameterStructure;
				ulPvrId = (OMX_S32) piArg[0];
				nPlayPause = (OMX_S32) piArg[1];

				pthread_mutex_lock(&omx_audiodec_component_Private->pAudioDecPrivate[nDevID].stAudioStart.mutex);
				if (omx_audiodec_component_Private->pAudioDecPrivate[ulPvrId].nPVRFlags & PVR_FLAG_START)
				{
					if (nPlayPause == 0)
					{
						omx_audiodec_component_Private->pAudioDecPrivate[ulPvrId].nPVRFlags |= PVR_FLAG_PAUSE;
					}
					else
					{
						omx_audiodec_component_Private->pAudioDecPrivate[ulPvrId].nPVRFlags &= ~PVR_FLAG_PAUSE;
					}
				}
				pthread_mutex_unlock(&omx_audiodec_component_Private->pAudioDecPrivate[nDevID].stAudioStart.mutex);
			}
			break;
#endif//SUPPORT_PVR

		case OMX_IndexVendorParamDxBGetSTCFunction:
			{
				OMX_S32 *piArg = (OMX_S32*) ComponentParameterStructure;
				pPrivateData->gfnDemuxGetSTCValue = (pfnDemuxGetSTC) piArg[0];
				pPrivateData->pvDemuxApp = (void*) piArg[1];
			}
			break;
		case OMX_IndexVendorParamAudioIsSupportCountry:
			{
				OMX_U32 uiDevId, uiCountry;
				OMX_U32 *piArg;
				piArg = (OMX_U32*) ComponentParameterStructure;
				uiDevId = piArg[0];
				uiCountry = piArg[1];
				omx_audiodec_component_Private->uiCountry = uiCountry;
			}
			break;
		case OMX_IndexVendorParamSetSeamlessSwitchCompensation:
			{
				OMX_U32 *piArg;
				piArg = (OMX_U32 *)ComponentParameterStructure;
				giDFSUseOnOff = piArg[0];
				giDFSInterval = piArg[1];
				giDFSStrength = piArg[2];
				giDFSNtimes = piArg[3];
				giDFSRange = piArg[4];
				giDFSgapadjust = piArg[5];
				giDFSgapadjust2 = piArg[6];
				giDFSmultiplier = piArg[7];

				SEAMLESS_DBG_PRINTF("[AUDIODEC] SeamlessSwitch compensation function [%d:%d:%d:%d:%d:%d:%d:%d] \n", giDFSUseOnOff, giDFSInterval, giDFSStrength, giDFSNtimes, giDFSRange, giDFSgapadjust, giDFSgapadjust2, giDFSmultiplier);
			}
			break;
		case OMX_IndexVendorParamISDBTProprietaryData:
		{
			OMX_U32 *piArg;
			piArg = (OMX_U32 *)ComponentParameterStructure;
			giChannelIndex = piArg[0];
			giServiceId = piArg[1];
			giSubServiceId = piArg[2];
			giPrevIndex = piArg[3];
			giSupportPrimary = piArg[4];
			ALOGD("[AUDIODEC] giChannelIndex[%d] giServiceId[%d] giSubServiceId[%d] giPrevIndex[%d] giSupportPrimary[%d]\n", giChannelIndex, giServiceId, giSubServiceId, giPrevIndex, giSupportPrimary);
		}
		break;
		case OMX_IndexVendorParamDxBSetDualchannel:
		{
			OMX_U32 *piArg;
			piArg = (OMX_U32 *)ComponentParameterStructure;
			/*
				case	giPrevIndex	curr	piArg[0]	giSwitching giSwitchingStart	dfs restart
				1	1	12	12	-1	0	Y
				2	12	1	1	1	1	N
				3	x	1	1	-1	0	Y
				4	x	12	12	-1	0	Y
				5	1	1	1	-1	0	Y
				6	12	12	12	-1	0	Y
			*/

			if(piArg[0] == 0){
				giSwitching = -1;
			}else{
				if(giPrevIndex == 0){
					giSwitching = 1;
				}else{
					giSwitching = -1;
				}
			}
			giPrevIndex = piArg[0];
		}
		break;

		default: /*Call the base component function*/
			return dxb_omx_base_component_SetParameter(hComponent, nParamIndex, ComponentParameterStructure);

	}

	if(err != OMX_ErrorNone)
		ALOGE("ERROR %s :: nParamIndex = 0x%x, error(0x%x)", __func__, nParamIndex, err);

	return err;
}

/** this function gets the parameters regarding audio formats and index */
OMX_ERRORTYPE dxb_omx_audiodec_component_GetParameter(
  OMX_IN  OMX_HANDLETYPE hComponent,
  OMX_IN  OMX_INDEXTYPE nParamIndex,
  OMX_INOUT OMX_PTR ComponentParameterStructure)
{

	OMX_AUDIO_PARAM_PORTFORMATTYPE *pAudioPortFormat;
	OMX_AUDIO_PARAM_PCMMODETYPE *pAudioPcmMode;
	OMX_PARAM_COMPONENTROLETYPE * pComponentRole;
	OMX_AUDIO_CONFIG_GETTIMETYPE *gettime;
	OMX_AUDIO_CONFIG_INFOTYPE *info;
	omx_base_audio_PortType *port;
	OMX_ERRORTYPE err = OMX_ErrorNone;
	OMX_AUDIO_SPECTRUM_INFOTYPE *power;
	OMX_AUDIO_ENERGY_VOLUME_INFOTYPE *energyvolume;
	int i;

	OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
	omx_audiodec_component_PrivateType* omx_audiodec_component_Private = openmaxStandComp->pComponentPrivate;
	omx_aacdec_component_PrivateType* AACPrivate;
	omx_ac3dec_component_PrivateType* AC3Private;
	omx_apedec_component_PrivateType* APEPrivate;
	omx_dtsdec_component_PrivateType* DTSPrivate;
	omx_flacdec_component_PrivateType* FLACPrivate;
	omx_mp2dec_component_PrivateType* MP2Private;
	omx_mp3dec_component_PrivateType* MP3Private;
	omx_radec_component_PrivateType* RAPrivate;
	omx_vorbisdec_component_PrivateType* VORBISPrivate;
	omx_wmadec_component_PrivateType* WMAPrivate;
	omx_wavdec_component_PrivateType* WAVPrivate;

	OMX_AUDIO_PARAM_AACPROFILETYPE* pAudioAac;
	OMX_AUDIO_PARAM_AC3TYPE * pAudioAc3;
	OMX_AUDIO_PARAM_APETYPE * pAudioApe;
	OMX_AUDIO_PARAM_DTSTYPE * pAudioDts;
	OMX_AUDIO_PARAM_FLACTYPE * pAudioFlac;
	OMX_AUDIO_PARAM_MP2TYPE * pAudioMp2;
	OMX_AUDIO_PARAM_MP3TYPE * pAudioMp3;
	OMX_AUDIO_PARAM_RATYPE * pAudioRa;
	OMX_AUDIO_PARAM_VORBISTYPE * pAudioVorbis;
	OMX_AUDIO_PARAM_WMATYPE * pAudioWma;

	OMX_S16 nDevID = 0;

	if (ComponentParameterStructure == NULL)
	{
		return OMX_ErrorBadParameter;
	}
	DBUG_MSG("   Getting parameter 0x%x\n", nParamIndex);
	/* Check which structure we are being fed and fill its header */

	switch(nParamIndex)
	{
		case OMX_IndexParamAudioInit:
			if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PORT_PARAM_TYPE))) != OMX_ErrorNone)
			{
			  break;
			}
			memcpy(ComponentParameterStructure, &omx_audiodec_component_Private->sPortTypesParam, sizeof(OMX_PORT_PARAM_TYPE));
			break;

		case OMX_IndexParamAudioPortFormat:
			pAudioPortFormat = (OMX_AUDIO_PARAM_PORTFORMATTYPE*)ComponentParameterStructure;
			if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE))) != OMX_ErrorNone)
			{
				break;
			}
			if (pAudioPortFormat->nPortIndex <= 2)
			{
				port = (omx_base_audio_PortType *)omx_audiodec_component_Private->ports[pAudioPortFormat->nPortIndex];
				memcpy(pAudioPortFormat, &port->sAudioParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
			}
			else
			{
				return OMX_ErrorBadPortIndex;
			}
			break;

		case OMX_IndexParamAudioPcm:
			pAudioPcmMode = (OMX_AUDIO_PARAM_PCMMODETYPE*)ComponentParameterStructure;
			if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE))) != OMX_ErrorNone)
			{
				break;
			}
			if (pAudioPcmMode->nPortIndex > 2)
			{
				return OMX_ErrorBadPortIndex;
			}

			if (pAudioPcmMode->nPortIndex < 2) // input is PCM
			{
				WAVPrivate = (omx_wavdec_component_PrivateType *)omx_audiodec_component_Private;
				memcpy(pAudioPcmMode, &WAVPrivate->pAudioPcm, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
			}
			else // output is PCM
			{
				memcpy(pAudioPcmMode, &omx_audiodec_component_Private->pAudioPcmMode, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
			}
			break;

		// --------------------------------------------------------------------------------
		// codec specific parameters -------------------------------------------------------

		case OMX_IndexParamAudioAac:
			pAudioAac = (OMX_AUDIO_PARAM_AACPROFILETYPE*)ComponentParameterStructure;
			AACPrivate = (omx_aacdec_component_PrivateType *)omx_audiodec_component_Private;
			if (pAudioAac->nPortIndex > 1)
			{
				return OMX_ErrorBadPortIndex;
			}
			if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE))) != OMX_ErrorNone)
			{
				break;
			}
			memcpy(pAudioAac, &AACPrivate->pAudioAac, sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));
			break;

		case OMX_IndexParamAudioAC3: // parameter read(channel,samplerate etc...)
			pAudioAc3 = (OMX_AUDIO_PARAM_AC3TYPE*)ComponentParameterStructure;
			AC3Private = (omx_ac3dec_component_PrivateType *)omx_audiodec_component_Private;
			if (pAudioAc3->nPortIndex > 1)
			{
				return OMX_ErrorBadPortIndex;
			}
			if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_AUDIO_PARAM_AC3TYPE))) != OMX_ErrorNone)
			{
				break;
			}
			memcpy(pAudioAc3, &AC3Private->pAudioAc3, sizeof(OMX_AUDIO_PARAM_AC3TYPE));
			break;

		case OMX_IndexParamAudioDTS:
			pAudioDts = (OMX_AUDIO_PARAM_DTSTYPE*)ComponentParameterStructure;
			DTSPrivate = (omx_dtsdec_component_PrivateType *)omx_audiodec_component_Private;
			if (pAudioDts->nPortIndex > 1)
			{
				return OMX_ErrorBadPortIndex;
			}
			if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_AUDIO_PARAM_DTSTYPE))) != OMX_ErrorNone)
			{
				break;
			}
			memcpy(pAudioDts, &DTSPrivate->pAudioDts, sizeof(OMX_AUDIO_PARAM_DTSTYPE));
			break;

		case OMX_IndexParamAudioMp2:
			pAudioMp2 = (OMX_AUDIO_PARAM_MP2TYPE*)ComponentParameterStructure;
			MP2Private = (omx_mp2dec_component_PrivateType *)omx_audiodec_component_Private;
			if (pAudioMp2->nPortIndex > 1)
			{
				return OMX_ErrorBadPortIndex;
			}
			if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_AUDIO_PARAM_MP2TYPE))) != OMX_ErrorNone)
			{
				break;
			}
			memcpy(pAudioMp2, &MP2Private->pAudioMp2, sizeof(OMX_AUDIO_PARAM_MP2TYPE));
			break;

		case OMX_IndexParamAudioMp3:
			pAudioMp3 = (OMX_AUDIO_PARAM_MP3TYPE*)ComponentParameterStructure;
			MP3Private = (omx_mp3dec_component_PrivateType *)omx_audiodec_component_Private;
			if (pAudioMp3->nPortIndex > 1)
			{
				return OMX_ErrorBadPortIndex;
			}
			if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_AUDIO_PARAM_MP3TYPE))) != OMX_ErrorNone)
			{
				break;
			}
			memcpy(pAudioMp3, &MP3Private->pAudioMp3, sizeof(OMX_AUDIO_PARAM_MP3TYPE));
			break;


		// codec specific ---------------------------------------------------------------
		// ------------------------------------------------------------------------------
		case OMX_IndexParamStandardComponentRole:
			pComponentRole = (OMX_PARAM_COMPONENTROLETYPE*)ComponentParameterStructure;
			if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PARAM_COMPONENTROLETYPE))) != OMX_ErrorNone)
			{
				break;
			}

			if (omx_audiodec_component_Private->pAudioDecPrivate[nDevID].audio_coding_type == OMX_AUDIO_CodingAAC)
			{
				strcpy( (char*) pComponentRole->cRole, AUDIO_DEC_TCC_AAC_ROLE);
			}
			else if (omx_audiodec_component_Private->pAudioDecPrivate[nDevID].audio_coding_type == OMX_AUDIO_CodingAC3)
			{
				strcpy( (char*) pComponentRole->cRole, AUDIO_DEC_AC3_ROLE);
			}
			else if (omx_audiodec_component_Private->pAudioDecPrivate[nDevID].audio_coding_type == OMX_AUDIO_CodingAPE)
			{
				strcpy( (char*) pComponentRole->cRole, AUDIO_DEC_APE_ROLE);
			}
			else if (omx_audiodec_component_Private->pAudioDecPrivate[nDevID].audio_coding_type == OMX_AUDIO_CodingDTS)
			{
				strcpy( (char*) pComponentRole->cRole, AUDIO_DEC_DTS_ROLE);
			}
			else if (omx_audiodec_component_Private->pAudioDecPrivate[nDevID].audio_coding_type == OMX_AUDIO_CodingFLAC)
			{
				strcpy( (char*) pComponentRole->cRole, AUDIO_DEC_FLAC_ROLE);
			}
			else if (omx_audiodec_component_Private->pAudioDecPrivate[nDevID].audio_coding_type == OMX_AUDIO_CodingMP2)
			{
				strcpy( (char*) pComponentRole->cRole, AUDIO_DEC_MP2_ROLE);
			}
			else if (omx_audiodec_component_Private->pAudioDecPrivate[nDevID].audio_coding_type == OMX_AUDIO_CodingMP3)
			{
				strcpy( (char*) pComponentRole->cRole, AUDIO_DEC_MP3_ROLE);
			}
			else if (omx_audiodec_component_Private->pAudioDecPrivate[nDevID].audio_coding_type == OMX_AUDIO_CodingRA)
			{
				strcpy( (char*) pComponentRole->cRole, AUDIO_DEC_RA_ROLE);
			}
			else if (omx_audiodec_component_Private->pAudioDecPrivate[nDevID].audio_coding_type == OMX_AUDIO_CodingVORBIS)
			{
				strcpy( (char*) pComponentRole->cRole, AUDIO_DEC_VORBIS_ROLE);
			}
			else if (omx_audiodec_component_Private->pAudioDecPrivate[nDevID].audio_coding_type == OMX_AUDIO_CodingPCM)
			{
				strcpy( (char*) pComponentRole->cRole, AUDIO_DEC_PCM_ROLE);
			}
			else if (omx_audiodec_component_Private->pAudioDecPrivate[nDevID].audio_coding_type == OMX_AUDIO_CodingWMA)
			{
				strcpy( (char*) pComponentRole->cRole, AUDIO_DEC_TCC_WMA_ROLE);
			}
			else
			{
				strcpy( (char*) pComponentRole->cRole,"\0");;
			}
			break;

		case OMX_IndexVendorParamDxbGetAudioType:
		{
			int *piParamDxbGetAudioType;
			piParamDxbGetAudioType = (int *)ComponentParameterStructure;
			// [0] = decoder id
			// [1] = return for channel no
			// [2] = return for audio mode
			int iDecoderID = *piParamDxbGetAudioType;

			//default value - undefined
			*(piParamDxbGetAudioType+1) = 0;
			*(piParamDxbGetAudioType+2) = 0;

			if (iDecoderID < (int)iTotalHandle) {
				if (omx_audiodec_component_Private->pAudioDecPrivate[iDecoderID].decode_ready == OMX_TRUE) {
					if (omx_audiodec_component_Private->pAudioDecPrivate[iDecoderID].gsADec.gsADecOutput.m_eChannelType > 0) {
						*(piParamDxbGetAudioType+1) = omx_audiodec_component_Private->pAudioDecPrivate[iDecoderID].gsADec.gsADecInput.m_uiNumberOfChannel;
						*(piParamDxbGetAudioType+2) = omx_audiodec_component_Private->pAudioDecPrivate[iDecoderID].gsADec.gsADecOutput.m_eChannelType;
					}
				}
			}
			break;
		}

		case OMX_IndexVendorParamDxbGetSeamlessValue:
		{
			int *piParamDxbGetSeamlessValue;
			piParamDxbGetSeamlessValue = (int *)ComponentParameterStructure;
			// [0] = return for dfs state
			// [1] = return for dfs value
			// [2] = return for dfs value from past
			*(piParamDxbGetSeamlessValue) = 0;
			*(piParamDxbGetSeamlessValue+1) = 0;
			*(piParamDxbGetSeamlessValue+2) = 0;
			#if defined (SEAMLESS_SWITCHING_COMPENSATION)
			tcc_dfs_lock_on();
			if(giDFSinit == 1 && g_audio_ctx != NULL) {
				*(piParamDxbGetSeamlessValue) = dfs_state_monitor;
				*(piParamDxbGetSeamlessValue+1) = giTime_delay_monitor;
				*(piParamDxbGetSeamlessValue+2) = pstSaved_dfs_gap[giChannelIndex].saved_avg_dfs_gap;
			}
			tcc_dfs_lock_off();
			#endif
		}
		break;

		default: /*Call the base component function*/
			return dxb_omx_base_component_GetParameter(hComponent, nParamIndex, ComponentParameterStructure);

	}

	return OMX_ErrorNone;

}

//#define	DUMP_INPUT_TO_FILE
//#define	DUMP_OUTPUT_TO_FILE
#if defined(DUMP_INPUT_TO_FILE) || defined(DUMP_OUTPUT_TO_FILE)
#define INTPUT_FILE_PATH "/sdcard/audioinput1.mp2"
#if 0
#define OUTPUT_FILE1_PATH "/run/media/mmcblk0p10/fullseg.pcm"
#define OUTPUT_FILE2_PATH "/run/media/mmcblk0p10/1seg.pcm"
#else
#define OUTPUT_FILE1_PATH "/run/media/sda1/fullseg.pcm"
#define OUTPUT_FILE2_PATH "/run/media/sda1/1seg.pcm"
#endif

int	gDumpHandle = 0;
FILE *gFileInput;
FILE *gFileOutput1=NULL;
FILE *gFileOutput2=NULL;
int gInputWriteSize = 0;
int gOutputWriteSize1=0;
int gOutputWriteSize2=0;

void IODumpFileInit(void)
{
#ifdef 	DUMP_INPUT_TO_FILE
	gFileInput = fopen(INTPUT_FILE_PATH, "wb");
	gInputWriteSize = 0;
	ALOGD("File Handle %d", gFileInput);
	if(gFileInput == NULL)
	{
		ALOGE("audio input file dump error %d",gFileInput);
	}
#endif

#ifdef 	DUMP_OUTPUT_TO_FILE
	gFileOutput1 = fopen(OUTPUT_FILE1_PATH, "wb");
	gOutputWriteSize1 = 0;
	ALOGD("File Handle %d", gFileOutput1);
	if(gFileOutput1 == NULL)
	{
		ALOGE("audio output file dump error %d",gFileOutput1);
	}
	gFileOutput2 = fopen(OUTPUT_FILE2_PATH, "wb");
	gOutputWriteSize2 = 0;
	ALOGD("File Handle %d", gFileOutput2);
	if(gFileOutput2 == NULL)
	{
		ALOGE("audio output file dump error %d",gFileOutput2);
	}
#endif
}

int IODumpFileWriteInputData(char *data, int size)
{
	if( gFileInput )
	{
		fwrite(data, 1, size, gFileInput);
		ALOGD("INPUT : write size %d - handle %d\n", size, gFileInput);
		gInputWriteSize += size;
		if(gInputWriteSize > 300*1024)
		{
			fclose(gFileInput);
			sync();
			gFileInput = NULL;
			ALOGD("INPUT : File Saved size = %d", gInputWriteSize);
		}
		return size;
	}
	else
		return 0;
}

int iDebugPrevCnt1=0;
int iDebugPrevCnt2=0;
int IODumpFileWriteOutputData(int dec_id, char *data, int size)
{
	if( dec_id == 0 && gFileOutput1!=NULL )
	{
		fwrite(data, 1, size, gFileOutput1);
		gOutputWriteSize1+= size;
		ALOGD("OUTPUT : dec%d write size %d - handle %d\n", dec_id, gOutputWriteSize1, gFileOutput1);
		if(gOutputWriteSize1> PCM_BUFFER_SIZE)
		{
			fclose(gFileOutput1);
			gFileOutput1= NULL;
			ALOGD("OUTPUT : dec%d File Saved size = %d", dec_id, gOutputWriteSize1);
		}
		return size;
	}
	else if( dec_id == 1 && gFileOutput2!=NULL	)
	{
		fwrite(data, 1, size, gFileOutput2);
		gOutputWriteSize2 += size;
		ALOGD("OUTPUT : dec%d write size %d - handle %d\n", dec_id, gOutputWriteSize2, gFileOutput2);
		if(gOutputWriteSize2 > PCM_BUFFER_SIZE)
		{
			fclose(gFileOutput2);
			gFileOutput2 = NULL;
			ALOGD("OUTPUT : dec%d File Saved size = %d", dec_id, gOutputWriteSize2);
		}
		return size;
	}
	else
	{
		return 0;
	}

	if( gFileOutput1 == NULL && gFileOutput2 == NULL )
	{
		sync();
	}
}
#endif

/*
void AudioSettingChangeRequest(OMX_COMPONENTTYPE *openmaxStandComp, int value)
{
	omx_audiodec_component_PrivateType* pPrivate = openmaxStandComp->pComponentPrivate;
	(*(pPrivate->callbacks->EventHandler)) (openmaxStandComp,
		pPrivate->callbackData,
		OMX_EventDynamicResourcesAvailable,
		value, NULL, NULL);
}
*/

#ifdef	SUPPORT_PVR
static void SetPVRFlags(omx_audiodec_component_PrivateType *omx_private, OMX_U8 uiDecoderID, OMX_BUFFERHEADERTYPE * pOutputBuffer)
{
	if (omx_private->pAudioDecPrivate[uiDecoderID].nPVRFlags & PVR_FLAG_START)
	{
		pOutputBuffer->nFlags |= OMX_BUFFERFLAG_FILE_PLAY;

		if (omx_private->pAudioDecPrivate[uiDecoderID].nPVRFlags & PVR_FLAG_TRICK)
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

static int CheckPVR(omx_audiodec_component_PrivateType *omx_private, OMX_U8 uiDecoderID, OMX_U32 ulInputBufferFlags)
{
	OMX_U32 iPVRState, iBufferState;

	iPVRState = (omx_private->pAudioDecPrivate[uiDecoderID].nPVRFlags & PVR_FLAG_START) ? 1 : 0;
	iBufferState = (ulInputBufferFlags & OMX_BUFFERFLAG_FILE_PLAY) ? 1 : 0;
	if (iPVRState != iBufferState)
	{
		return 1; // Skip
	}

	iPVRState = (omx_private->pAudioDecPrivate[uiDecoderID].nPVRFlags & PVR_FLAG_TRICK) ? 1 : 0;
	iBufferState = (ulInputBufferFlags & OMX_BUFFERFLAG_TRICK_PLAY) ? 1 : 0;
	if (iPVRState != iBufferState)
	{
		return 1; // Skip
	}

	if (omx_private->pAudioDecPrivate[uiDecoderID].nPVRFlags & PVR_FLAG_CHANGED)
	{
		omx_private->pAudioDecPrivate[uiDecoderID].nPVRFlags &= ~PVR_FLAG_CHANGED;

		omx_private->pAudioDecPrivate[uiDecoderID].iNumOfSeek = 0;
		omx_private->pAudioDecPrivate[uiDecoderID].iStartTS = 0;
		omx_private->pAudioDecPrivate[uiDecoderID].iSamples = 0;
	}

	return 0; // Decoding
}
#endif//SUPPORT_PVR

void dxb_omx_audiodec_component_BufferMgmtCallback(OMX_COMPONENTTYPE *openmaxStandComp, OMX_BUFFERHEADERTYPE* inputbuffer, OMX_BUFFERHEADERTYPE* outputbuffer)
{

	omx_audiodec_component_PrivateType* pPrivate = openmaxStandComp->pComponentPrivate;
	omx_aacdec_component_PrivateType* pAACPrivate;
	TCC_ADEC_PRIVATE *pPrivateData = (TCC_ADEC_PRIVATE*) pPrivate->pPrivateData;

	OMX_S32 ret	= 0;
	OMX_S32 comp_stc_delay[1];
	OMX_U8* input_ptr;
	ape_dmx_exinput_t *codecSpecific = 0;
	OMX_S16 iSeekFlag = 0;
	OMX_U32 uiDemuxID = 0, uiDecoderID = 0;
	TCC_DXBAUDIO_DUALMONO_TYPE eStereoMode;
	OMX_S8 value[PROPERTY_VALUE_MAX];

	if( memcmp(inputbuffer->pBuffer,BUFFER_INDICATOR,BUFFER_INDICATOR_SIZE)==0)
	{
		memcpy(&input_ptr, inputbuffer->pBuffer + BUFFER_INDICATOR_SIZE, 4);
		if(inputbuffer->pBuffer[BUFFER_INDICATOR_SIZE+4] == 0xAA)
		{
			uiDemuxID = inputbuffer->pBuffer[BUFFER_INDICATOR_SIZE+5];
			uiDecoderID = inputbuffer->pBuffer[BUFFER_INDICATOR_SIZE+6];
			//ALOGD("%s:Demuxer[%d]Decoder[%d]",__func__, uiDemuxID, uiDecoderID);
		}
	}else{
		input_ptr = inputbuffer->pBuffer;
		uiDecoderID= inputbuffer->pBuffer[inputbuffer->nFilledLen];
	}
	pthread_mutex_lock(&pPrivate->pAudioDecPrivate[uiDecoderID].stAudioStart.mutex);
#ifdef	SUPPORT_PVR
	if (CheckPVR(pPrivate, uiDecoderID, inputbuffer->nFlags))
	{
		inputbuffer->nFilledLen = 0;
		outputbuffer->nFilledLen = 0;
		pthread_mutex_unlock(&pPrivate->pAudioDecPrivate[uiDecoderID].stAudioStart.mutex);
		return;
	}
#endif//SUPPORT_PVR

	if(pPrivate->pAudioDecPrivate[uiDecoderID].bAudioStarted == OMX_FALSE
	|| pPrivate->pAudioDecPrivate[uiDecoderID].stAudioStart.nState == TCC_DXBAUDIO_OMX_Dec_Stop)
	{
		inputbuffer->nFilledLen = 0;
		outputbuffer->nFilledLen = 0;
		pthread_mutex_unlock(&pPrivate->pAudioDecPrivate[uiDecoderID].stAudioStart.mutex);
		return;
	}

	outputbuffer->nFilledLen = 0;
	outputbuffer->nOffset = 0;
	outputbuffer->nDecodeID = uiDecoderID;

	DBUG_MSG("BufferMgmtCallback IN inLen = %ld, Flags = 0x%x", inputbuffer->nFilledLen, inputbuffer->nFlags);
	//if((inputbuffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG) && pPrivate->pAudioDecPrivate[uiDecoderID].decode_ready == OMX_FALSE)
	if( pPrivate->pAudioDecPrivate[uiDecoderID].decode_ready == OMX_FALSE )
	{
		memset(&pPrivate->pAudioDecPrivate[uiDecoderID].gsADec, 0, sizeof(ADEC_VARS));
		pPrivate->pAudioPcmMode[uiDecoderID].nSamplingRate = pPrivateData->guiSamplingRate;//48000; //default 48000
		pPrivate->pAudioPcmMode[uiDecoderID].nChannels = pPrivateData->guiChannels; //default stereo

		if(pPrivate->dxb_standard == DxB_STANDARD_ISDBT)
		{
			//pPrivate->pAudioDecPrivate[uiDecoderID].gsADec.gsADecInit.m_unAudioCodecParams.m_unMP2.m_iDABMode = 0;
			pPrivate->pAudioDecPrivate[uiDecoderID].cdk_core.m_iAudioProcessMode = AUDIO_BROADCAST_MODE;
		}

		pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_info.m_sAudioInfo.m_iBitsPerSample = pPrivate->pAudioPcmMode[uiDecoderID].nBitPerSample;
		pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_info.m_sAudioInfo.m_iSamplePerSec = pPrivate->pAudioPcmMode[uiDecoderID].nSamplingRate;
		pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_info.m_sAudioInfo.m_iChannels = pPrivate->pAudioPcmMode[uiDecoderID].nChannels;
		pPrivate->pAudioDecPrivate[uiDecoderID].iCtype = CONTAINER_TYPE_AUDIO;//CONTAINER_TYPE_TS;//CONTAINER_TYPE_AUDIO;
		pPrivate->iGuardSamples = 0;


		if( pPrivate->pAudioDecPrivate[uiDecoderID].guiAudioInitBufferIndex+inputbuffer->nFilledLen > AUDIO_INIT_BUFFER_SIZE)
		{
			memset(pPrivate->pAudioDecPrivate[uiDecoderID].gucAudioInitBuffer, 0x0, AUDIO_INIT_BUFFER_SIZE);
			pPrivate->pAudioDecPrivate[uiDecoderID].guiAudioInitBufferIndex = 0;
		}

		memcpy(pPrivate->pAudioDecPrivate[uiDecoderID].gucAudioInitBuffer+pPrivate->pAudioDecPrivate[uiDecoderID].guiAudioInitBufferIndex, input_ptr, inputbuffer->nFilledLen);
		pPrivate->pAudioDecPrivate[uiDecoderID].guiAudioInitBufferIndex += inputbuffer->nFilledLen;

		pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_info.m_sAudioInfo.m_pExtraData = pPrivate->pAudioDecPrivate[uiDecoderID].gucAudioInitBuffer;
		pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_info.m_sAudioInfo.m_iExtraDataLen = pPrivate->pAudioDecPrivate[uiDecoderID].guiAudioInitBufferIndex;
		if( pPrivate->pAudioDecPrivate[uiDecoderID].audio_coding_type == OMX_AUDIO_CodingMP2 )
			pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_info.m_sAudioInfo.m_iFormatId = AV_AUDIO_MP2;
		else if( pPrivate->pAudioDecPrivate[uiDecoderID].audio_coding_type == OMX_AUDIO_CodingAC3 )
			pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_info.m_sAudioInfo.m_iFormatId = AV_AUDIO_AC3;
		else if( pPrivate->pAudioDecPrivate[uiDecoderID].audio_coding_type == OMX_AUDIO_CodingAAC )
		{
			pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_info.m_sAudioInfo.m_iFormatId = AV_AUDIO_AAC;
			if( pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_info.m_sAudioInfo.m_ulSubType == AUDIO_SUBTYPE_NONE )
			{
				pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_info.m_sAudioInfo.m_pExtraData = NULL;
				pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_info.m_sAudioInfo.m_iExtraDataLen = 0;
			}
		}

		pPrivate->pAudioDecPrivate[uiDecoderID].cdk_core.m_pOutWav = NULL;
	#ifdef _TCC8920_
		if (pPrivate->dxb_standard == DxB_STANDARD_ISDBT) {
			if (uiDecoderID == 0 && pPrivate->uiCountry == TCCDXB_BRAZIL && pPrivate->pAudioDecPrivate[uiDecoderID].iAdecType == AUDIO_ID_AAC)
				pPrivate->pAudioDecPrivate[uiDecoderID].gsADec.gsADecInit.m_unAudioCodecParams.m_unAAC.m_uiChannelMasking = 1;
		}
	#endif

		if (pPrivate->dxb_standard == DxB_STANDARD_ISDBT) {
			if (uiDecoderID == 0 && pPrivate->uiCountry == TCCDXB_JAPAN && pPrivate->pAudioDecPrivate[uiDecoderID].iAdecType == AUDIO_ID_AAC)
				pPrivate->pAudioDecPrivate[uiDecoderID].gsADec.gsADecInit.m_unAudioCodecParams.m_unAAC.m_uiMatrixMixDownMode = 2;
		}

		#if 0// debug initial parameter
		ALOGD("[ADEC] Init PTS %d, size %d", (int)(inputbuffer->nTimeStamp/1000), (int)inputbuffer->nFilledLen);
		{
			OMX_AUDIO_DEC_PRIVATE_DATA *pAudioDecPrivate = &pPrivate->pAudioDecPrivate[uiDecoderID];

			ALOGD("[ADEC] Init Param. cdk_core.m_iAudioProcessMode[%d] iCtype[%d] audio_coding_type[%d]"
				, pAudioDecPrivate->cdk_core.m_iAudioProcessMode
				, pAudioDecPrivate->iCtype
				, pAudioDecPrivate->audio_coding_type
			);
			ALOGD("[ADEC] Init Param. cdmx_info.m_sAudioInfo m_iBitsPerSample[%d] m_iSamplePerSec[%d] m_iChannels[%d]"
				, pAudioDecPrivate->cdmx_info.m_sAudioInfo.m_iBitsPerSample
				, pAudioDecPrivate->cdmx_info.m_sAudioInfo.m_iSamplePerSec
				, pAudioDecPrivate->cdmx_info.m_sAudioInfo.m_iChannels
			);
			ALOGD("[ADEC] Init Param. cdmx_info.m_sAudioInfo m_iFormatId[%d] m_ulSubType[%d] m_iExtraDataLen[%d]"
				, pAudioDecPrivate->cdmx_info.m_sAudioInfo.m_iFormatId
				, pAudioDecPrivate->cdmx_info.m_sAudioInfo.m_ulSubType
				, pAudioDecPrivate->cdmx_info.m_sAudioInfo.m_iExtraDataLen
			);

			ALOGD("[ADEC] Init Param. guiAudioInitBufferIndex[%d]"
				, pAudioDecPrivate->guiAudioInitBufferIndex
			);
		}
		#endif

		// for debug at cdk
		pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_info.m_sAudioInfo.m_iAudioStreamID = uiDecoderID;
		pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_info.m_sFileInfo.m_iRunningtime = (int)(inputbuffer->nTimeStamp/1000);

		if( cdk_adec_init(&pPrivate->pAudioDecPrivate[uiDecoderID].cdk_core,
							&pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_info,
							pPrivate->pAudioDecPrivate[uiDecoderID].iAdecType,		// AUDIO_ID
							pPrivate->pAudioDecPrivate[uiDecoderID].iCtype,			// CONTAINER_TYPE
							pPrivate->pAudioDecPrivate[uiDecoderID].cb_function,
							&pPrivate->pAudioDecPrivate[uiDecoderID].gsADec) < 0 )	// Audio decoder function
		{
			ALOGE("Audio[%d] DEC init error.", (int)uiDecoderID);
			cdk_adec_close(&pPrivate->pAudioDecPrivate[uiDecoderID].cdk_core, &pPrivate->pAudioDecPrivate[uiDecoderID].gsADec);
			inputbuffer->nFlags &= ~OMX_BUFFERFLAG_CODECCONFIG;
			// to skip all audio data
			inputbuffer->nFilledLen = 0;
			pthread_mutex_unlock(&pPrivate->pAudioDecPrivate[uiDecoderID].stAudioStart.mutex);
			return;
		}

//=================================================================
#ifdef ERROR_PCM_MUTE
		tPcmMute[uiDecoderID].MUTE_BYTE_SEL			= (MUTE_TIME_MSEC_SEL*pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_info.m_sAudioInfo.m_iSamplePerSec/1000)*pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_info.m_sAudioInfo.m_iChannels*sizeof(short);
		tPcmMute[uiDecoderID].MUTE_BYTE_ERR			= (MUTE_TIME_MSEC_ERR*pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_info.m_sAudioInfo.m_iSamplePerSec/1000)*pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_info.m_sAudioInfo.m_iChannels*sizeof(short);
		tPcmMute[uiDecoderID].mute_target_byte		= 0;
#endif //ERROR_PCM_MUTE
#ifdef ERROR_PCM_FADE_IN
		tPcmFade_in[uiDecoderID].FADE_IN_BYTE			= (FADE_IN_TIME_MSEC*pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_info.m_sAudioInfo.m_iSamplePerSec/1000)*pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_info.m_sAudioInfo.m_iChannels*sizeof(short);
		tPcmFade_in[uiDecoderID].number_of_fade_in_frame= FADE_IN_NUMBER;
		tPcmFade_in[uiDecoderID].fade_in_frame_size		= tPcmFade_in[uiDecoderID].FADE_IN_BYTE / tPcmFade_in[uiDecoderID].number_of_fade_in_frame;
		tPcmFade_in[uiDecoderID].size_to_fade_in		= tPcmFade_in[uiDecoderID].fade_in_frame_size;
		tPcmFade_in[uiDecoderID].fade_in_tab_index		= 0;
		tPcmFade_in[uiDecoderID].fade_in_taget_byte		= 0;
		tPcmFade_in[uiDecoderID].frame_to_fade_in		= 0;
#endif //ERROR_PCM_FADE_IN
//=================================================================
		pPrivate->pAudioDecPrivate[uiDecoderID].decode_ready  = OMX_TRUE;
		pPrivate->isNewBuffer = 1;
		pPrivate->pAudioPcmMode[uiDecoderID].nSamplingRate = 0;
		pPrivate->pAudioPcmMode[uiDecoderID].nChannels = 0;
		DBUG_MSG("Audio[%d] DEC initialized.", uiDecoderID);

#if defined(DUMP_INPUT_TO_FILE) || defined(DUMP_OUTPUT_TO_FILE)
		#if 0
		if (gDumpHandle == uiDecoderID)
			IODumpFileInit();
		#else
		if(gFileOutput1 == NULL || gFileOutput2 == NULL)
		{
			IODumpFileInit();
		}
		#endif
#endif
	}

	pPrivate->pAudioDecPrivate[uiDecoderID].bAudioInDec = OMX_TRUE;

	if(pPrivate->pAudioDecPrivate[uiDecoderID].iCtype != CONTAINER_TYPE_AUDIO)
	{
		// if previous decoding failed, silence should be inserted
		if(pPrivate->pAudioDecPrivate[uiDecoderID].bPrevDecFail == OMX_TRUE)
		{
			if(/*pPrivate->outDevID==uiDecoderID*/OMX_TRUE) {
				OMX_TICKS samples;
				samples = (outputbuffer->nTimeStamp - pPrivate->pAudioDecPrivate[uiDecoderID].iPrevTS) * pPrivate->pAudioPcmMode[uiDecoderID].nSamplingRate / 1000000;
				outputbuffer->nFilledLen = pPrivate->pAudioPcmMode[uiDecoderID].nChannels * samples * sizeof(short);
				memset(outputbuffer->pBuffer, 0, outputbuffer->nFilledLen);
			}
			else {
				outputbuffer->nFilledLen = 0;
			}

			pPrivate->pAudioDecPrivate[uiDecoderID].bPrevDecFail = OMX_FALSE;
			inputbuffer->nFilledLen = 0;
#ifdef PCM_INFO_SIZE
			if(outputbuffer->nFilledLen > 0) {
				memcpy(outputbuffer->pBuffer+outputbuffer->nFilledLen, &pPrivate->pAudioPcmMode[uiDecoderID].nSamplingRate, 4);
				outputbuffer->nFilledLen += 4;
				memcpy(outputbuffer->pBuffer+outputbuffer->nFilledLen, &pPrivate->pAudioPcmMode[uiDecoderID].nChannels, 4);
				outputbuffer->nFilledLen += 4;
				*(int *)(outputbuffer->pBuffer+outputbuffer->nFilledLen) = 0;
				outputbuffer->nFilledLen += 4;
			}
#endif
			pPrivate->pAudioDecPrivate[uiDecoderID].bAudioInDec = OMX_FALSE;
			pthread_mutex_unlock(&pPrivate->pAudioDecPrivate[uiDecoderID].stAudioStart.mutex);
			return;
		}
	}

	if(pPrivate->pAudioDecPrivate[uiDecoderID].bPrevDecFail == OMX_TRUE)
	{
		if( inputbuffer->nTimeStamp == 0 )
		{
			/* if decode error happened at previous [first part of PES], this buffer is [remain splited part of PES], then discard because can't interpolate timestamp */
			inputbuffer->nFilledLen = 0;
			ALOGE("Audio[%d] discard condition 1.", uiDecoderID);
			pthread_mutex_unlock(&pPrivate->pAudioDecPrivate[uiDecoderID].stAudioStart.mutex);
			return ;
		}

		/* flush previous data */
		ALOGE("Audio[%d] process after bPrevDecFail.", uiDecoderID);
		iSeekFlag = 1;
		pPrivate->pAudioDecPrivate[uiDecoderID].iStartTS = inputbuffer->nTimeStamp;
		pPrivate->pAudioDecPrivate[uiDecoderID].iSamples = 0;
		pPrivate->pAudioDecPrivate[uiDecoderID].bPrevDecFail = OMX_FALSE;
	}

	if(inputbuffer->nFlags & OMX_BUFFERFLAG_SYNCFRAME)
	{
		iSeekFlag = 1;
		pPrivate->pAudioDecPrivate[uiDecoderID].iStartTS = inputbuffer->nTimeStamp;
		pPrivate->pAudioDecPrivate[uiDecoderID].iSamples = 0;
		pPrivate->pAudioDecPrivate[uiDecoderID].iNumOfSeek++;

		if (pPrivate->pAudioDecPrivate[uiDecoderID].iAdecType == AUDIO_ID_APE)
		{
			OMX_U8* p = input_ptr;

			if ( *((OMX_U32*)(p+inputbuffer->nFilledLen-CODEC_SPECIFIC_MARKER_OFFSET)) == CODEC_SPECIFIC_MARKER)
			{
				codecSpecific = (ape_dmx_exinput_t *)(*((OMX_U32*)(p+inputbuffer->nFilledLen-CODEC_SPECIFIC_INFO_OFFSET)));
				inputbuffer->nFilledLen -= CODEC_SPECIFIC_MARKER_OFFSET;
			}
			else
			{
				ALOGE("No codec specific data");
			}
		}
	}
	else
	{
		if( inputbuffer->nTimeStamp==0 && pPrivate->pAudioDecPrivate[uiDecoderID].iStartTS == 0 )
		{
			/* when initializing, Init fail with [first part of PES], then Init success with [remain splited part of PES], then discard because can't interpolate timestamp */
			inputbuffer->nFilledLen = 0;
			ALOGE("[ADEC][%d] discard condition 2.", uiDecoderID);
			pthread_mutex_unlock(&pPrivate->pAudioDecPrivate[uiDecoderID].stAudioStart.mutex);
			return ;
		}
	}

#if defined(DUMP_INPUT_TO_FILE)
	if (gDumpHandle == uiDecoderID)
		IODumpFileWriteInputData(input_ptr, inputbuffer->nFilledLen);
#endif

	pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_out.m_uiUseCodecSpecific = (unsigned int)codecSpecific;

	/* Decode the block */
	pPrivate->pAudioDecPrivate[uiDecoderID].cdk_core.m_pOutWav = outputbuffer->pBuffer;

	pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_out.m_pPacketData = input_ptr;
	pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_out.m_iPacketSize = inputbuffer->nFilledLen;
	pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_out.m_iTimeStamp = (int)(inputbuffer->nTimeStamp/1000); // for debug at cdk

	if (0)
	{
		OMX_U8* p = input_ptr;
		ALOGD("------------------------------");
		ALOGD("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
		ALOGD("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", p[0+16], p[1+16], p[2+16], p[3+16], p[4+16], p[5+16], p[6+16], p[7+16], p[8+16], p[9+16], p[10+16], p[11+16], p[12+16], p[13+16], p[14+16], p[15+16]);
		ALOGD("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", p[0+32], p[1+32], p[2+32], p[3+32], p[4+32], p[5+32], p[6+32], p[7+32], p[8+32], p[9+32], p[10+32], p[11+32], p[12+32], p[13+32], p[14+32], p[15+32]);
		ALOGD("------------------------------");
	}
	ret = cdk_adec_decode(&pPrivate->pAudioDecPrivate[uiDecoderID].cdk_core,
								&pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_info,
								&pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_out,
								pPrivate->pAudioDecPrivate[uiDecoderID].iAdecType,
								iSeekFlag,
								0, // alsa_status
								&pPrivate->pAudioDecPrivate[uiDecoderID].gsADec);


	if (ret >= 0)
	{
		omx_aacdec_component_PrivateType* pAACPrivate;
		OMX_U32 *pbuf = (OMX_U32 *)outputbuffer->pBuffer;
		OMX_U32  pcm;
		OMX_U16  lpcm, rpcm;
		int count;

   		if(((pPrivate->pAudioPcmMode[uiDecoderID].nSamplingRate != pPrivate->pAudioDecPrivate[uiDecoderID].gsADec.gsADecOutput.m_eSampleRate) ||
			 ( pPrivate->pAudioPcmMode[uiDecoderID].nChannels!=pPrivate->pAudioDecPrivate[uiDecoderID].gsADec.gsADecOutput.m_uiNumberOfChannel)) && /*pPrivate->outDevID==uiDecoderID*/OMX_TRUE)
		{
			ALOGE("SamplingRate[%d]Channel[%d] \n",pPrivate->pAudioDecPrivate[uiDecoderID].gsADec.gsADecOutput.m_eSampleRate, pPrivate->pAudioDecPrivate[uiDecoderID].gsADec.gsADecOutput.m_uiNumberOfChannel);
			pPrivate->pAudioPcmMode[uiDecoderID].nSamplingRate = pPrivate->pAudioDecPrivate[uiDecoderID].gsADec.gsADecOutput.m_eSampleRate;
			pPrivate->pAudioPcmMode[uiDecoderID].nChannels = pPrivate->pAudioDecPrivate[uiDecoderID].gsADec.gsADecOutput.m_uiNumberOfChannel;
			/*Send Port Settings changed call back*/
			(*(pPrivate->callbacks->EventHandler))
			(openmaxStandComp,
			pPrivate->callbackData,
			OMX_EventPortSettingsChanged, /* The command was completed */
			0,
			2, /* This is the output port index */
			NULL);
		}

		outputbuffer->nFilledLen = pPrivate->pAudioPcmMode[uiDecoderID].nChannels * pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_out.m_iDecodedSamples * sizeof(short);
		if(outputbuffer->nFilledLen == 0)
		{
			outputbuffer->nFilledLen = 0;
			inputbuffer->nFilledLen = 0;

			if( pPrivate->pAudioDecPrivate[uiDecoderID].iStartTS == 0 )
			{
				/* after interpolation reset(error or discontinuity, then no pcm output at 1st PES decoding, but Start should be set for next interpolation */
				pPrivate->pAudioDecPrivate[uiDecoderID].iStartTS = inputbuffer->nTimeStamp;
				pPrivate->pAudioDecPrivate[uiDecoderID].iSamples = 0;
			}
			pPrivate->pAudioDecPrivate[uiDecoderID].bAudioInDec = OMX_FALSE;

			pthread_mutex_unlock(&pPrivate->pAudioDecPrivate[uiDecoderID].stAudioStart.mutex);
			return;
		}

		eStereoMode = pPrivate->eStereoMode;

		if (pPrivate->pAudioDecPrivate[uiDecoderID].gsADec.gsADecOutput.m_eChannelType == TCAS_CH_DUAL)
		{
			pAACPrivate = openmaxStandComp->pComponentPrivate;
			eStereoMode = pAACPrivate->eAudioDualMonoSel;

			switch (eStereoMode)
			{
				case TCC_DXBAUDIO_DUALMONO_Left:
					for(count=0; count < pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_out.m_iDecodedSamples; count++)
					{
						pcm = (OMX_U32) pbuf[count];
						lpcm = (OMX_U16)(pcm & 0x0000FFFF);
						rpcm = (OMX_U16)((pcm & 0xFFFF0000) >> 16);
						pcm = ((OMX_U32)lpcm << 16) | ((OMX_U32)lpcm);
						pbuf[count] = pcm;
					}
					break;
				case TCC_DXBAUDIO_DUALMONO_Right:
					for(count=0; count < pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_out.m_iDecodedSamples; count++)
					{
						pcm = (OMX_U32) pbuf[count];
						lpcm = (OMX_U16)(pcm & 0x0000FFFF);
						rpcm = (OMX_U16)((pcm & 0xFFFF0000) >> 16);
						pcm = ((OMX_U32)rpcm << 16) | ((OMX_U32)rpcm);
						pbuf[count] = pcm;
					}
					break;
				case TCC_DXBAUDIO_DUALMONO_Mix:
					for(count=0; count < pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_out.m_iDecodedSamples; count++)
					{
						pcm = (OMX_U32) pbuf[count];
						lpcm = (OMX_U16)(pcm & 0x0000FFFF) >> 1;
						rpcm = (OMX_U16)((pcm & 0xFFFF0000) >> 17);
						lpcm += rpcm;
						pcm = ((OMX_U32)lpcm << 16) | ((OMX_U32)lpcm);
						pbuf[count] = pcm;
					}
					break;
				case TCC_DXBAUDIO_DUALMONO_LeftNRight:
				default:
					/* empty intentionally*/
					break;
			}
		}

	#ifdef	USE_PCM_GAIN
	{
		signed short *pl, *pr;

		#if 0
		/*--- test code start ---*/
		unsigned int uLength;
		float newgain;
		uLength = property_get("dxb.pcm.gain", value, "");
		if (uLength) {
			uLength = atoi(value);
			if (uLength != 0) {
				newgain = (float) ((float)uLength / 10);
				if (newgain != gPCM_GAIN) {
					LOGE("*** pcm gain: %f -> %f ***", gPCM_GAIN, newgain);
					gPCM_GAIN = newgain;
				}
			}
		}
		/*--- test code end ---*/
		#endif

		for (count=0; count < pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_out.m_iDecodedSamples; count++)
		{
			pl = (signed short *)&pbuf[count];
			pr = pl+1;
			*pl = (signed short int)((float)*pl * gPCM_GAIN);
			*pr = (signed short int)((float)*pr * gPCM_GAIN);
		}
	}
	#endif

		if (pPrivate->pAudioDecPrivate[uiDecoderID].iCtype != CONTAINER_TYPE_AUDIO)
		{
			if (pPrivate->pAudioDecPrivate[uiDecoderID].iAdecType == AUDIO_ID_WMA)
			{
//				outputbuffer->nTimeStamp = pPrivate->pAudioDecPrivate[uiDecoderID].iStartTS + ((OMX_TICKS)pPrivate->pAudioDecPrivate[uiDecoderID].iSamples * 1000000 + (pPrivate->pAudioPcmMode[uiDecoderID].nSamplingRate >> 1)) / pPrivate->pAudioPcmMode[uiDecoderID].nSamplingRate;
				;
			}

			// to reduce peak noise, decoded samples which are corresponding to iGuardSamples are set to 0 after seek
			// 'iNumOfSeek > 1' means that the first trial of seek is done actually
			if (pPrivate->pAudioDecPrivate[uiDecoderID].iSamples < pPrivate->iGuardSamples && pPrivate->pAudioDecPrivate[uiDecoderID].iNumOfSeek > 1)
			{
				memset(outputbuffer->pBuffer, 0, outputbuffer->nFilledLen);
			}
			pPrivate->pAudioDecPrivate[uiDecoderID].iSamples += (OMX_TICKS)pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_out.m_iDecodedSamples;
		}
		else
		{
			if (pPrivate->pAudioDecPrivate[uiDecoderID].iAdecType == AUDIO_ID_APE)
			{
				outputbuffer->nTimeStamp = pPrivate->pAudioDecPrivate[uiDecoderID].iStartTS + ((OMX_TICKS)pPrivate->pAudioDecPrivate[uiDecoderID].iSamples * 1000000 + (pPrivate->pAudioPcmMode[uiDecoderID].nSamplingRate >> 1)) / pPrivate->pAudioPcmMode[uiDecoderID].nSamplingRate;
				pPrivate->pAudioDecPrivate[uiDecoderID].iSamples += (OMX_TICKS)pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_out.m_iDecodedSamples;
			}
			else
			{
				if( pPrivate->pAudioDecPrivate[uiDecoderID].iStartTS == 0 )
				{
					/* start at first time */
					pPrivate->pAudioDecPrivate[uiDecoderID].iStartTS = inputbuffer->nTimeStamp;
					pPrivate->pAudioDecPrivate[uiDecoderID].iSamples = 0;
				}
				else
				{
					OMX_TICKS nPeriodOfFrame = (OMX_TICKS)pPrivate->pAudioDecPrivate[uiDecoderID].gsADec.gsADecOutput.m_uiSamplesPerChannel*1000000ll / pPrivate->pAudioPcmMode[uiDecoderID].nSamplingRate;

					/* Interpolation Timestamp */
					outputbuffer->nTimeStamp = pPrivate->pAudioDecPrivate[uiDecoderID].iStartTS + (pPrivate->pAudioDecPrivate[uiDecoderID].iSamples * 1000000ll / pPrivate->pAudioPcmMode[uiDecoderID].nSamplingRate);

					/* check validation of timestamp */
					/* if output timestamp is invalid, set as input timestamp,
						output timestamp can be less than 1 frame period because previously remain data,
						or output timestamp is equal input. */
					if( inputbuffer->nTimeStamp )
					{
						if( outputbuffer->nTimeStamp < (inputbuffer->nTimeStamp-(nPeriodOfFrame+(nPeriodOfFrame>>3))) || outputbuffer->nTimeStamp > inputbuffer->nTimeStamp )
						{
							/* add 10% for consider round_down, and use(1/8) for simple calculation, so nPeriodOfFrame*(1+1/8)	*/
							outputbuffer->nTimeStamp = inputbuffer->nTimeStamp;
							pPrivate->pAudioDecPrivate[uiDecoderID].iStartTS = outputbuffer->nTimeStamp;
							pPrivate->pAudioDecPrivate[uiDecoderID].iSamples = 0;
						}
					}
				}

				pPrivate->pAudioDecPrivate[uiDecoderID].iSamples += (OMX_TICKS)pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_out.m_iDecodedSamples;

				/* make as expected next Timestamp */
				pPrivate->pAudioDecPrivate[uiDecoderID].iStartTS += (pPrivate->pAudioDecPrivate[uiDecoderID].iSamples * 1000000ll / pPrivate->pAudioPcmMode[uiDecoderID].nSamplingRate);
				pPrivate->pAudioDecPrivate[uiDecoderID].iSamples = 0;

#ifdef SEAMLESS_SWITCHING_COMPENSATION
				tcc_dfs_lock_on();
				if(giPrimaryServiceCheck == 0){
					if(giSupportPrimary == 1){
						/*
							0 = Not Primary Service
							1 = Primary Service of 12 seg
							2 = Primary Service of 1 seg
						*/
						giPrimaryService = tcc_db_get_channel_primary(giServiceId);
						giSubPrimaryService = tcc_db_get_channel_primary(giSubServiceId);
						giPartial_service_cnt = tcc_db_get_one_service_num(giChannelIndex);
						if(giPartial_service_cnt >= 2 && (giPrimaryService == 1 && giSubPrimaryService == 2)){
							/*
							 If the channel has two or more the service of 1seg,
							 the gap calc will work only when the services both of 1seg/12seg are default service.
							 So giSupportPrimary is set to '1'.
							*/
							giDfs_start_flag = 1;
						}else if(giPartial_service_cnt < 2){
							/*
								If the channel has one of 1seg, the gap calc will work.
								So giSupportPrimary is set to '1'.
							*/
							giDfs_start_flag = 1;
						}else{
							/*
								Otherwise, the gap calc will not work. So giSupportPrimary is set to '0'.
							*/
							giDfs_start_flag = 0;
						}

						SEAMLESS_DBG_PRINTF("\033[32m [Seamless] <START> Primary Service Check ServiceID(%d:%d) ch_idx=[%d] 1seg_num=[%d] giPrimaryService[%d:%d]\033[m\n", giServiceId, giSubServiceId, giChannelIndex, giPartial_service_cnt, giPrimaryService, giSubPrimaryService);
					}else{
						giDfs_start_flag = 1;
					}
					giPrimaryServiceCheck = 1;
				}

				if(outputbuffer->nFilledLen > 0 && giDFSUseOnOff == 1
				&& pPrivate->pAudioDecPrivate[uiDecoderID].decode_ready == OMX_TRUE
				&& pPrivate->pAudioDecPrivate[uiDecoderID].bPrevDecFail == OMX_FALSE
				&& giDfs_start_flag == 1){
					if(giDFSinit == 0){
						if(giServiceLog == 0){
							SEAMLESS_DBG_PRINTF("\033[32m [Seamless] <START> 1st Calc after switched to ServiceID(%d) ch_idx=[%d]\033[m\n", giServiceId, giChannelIndex);
							giServiceLog = 1;
						}
						tcc_dfs_init();
						giDFSinit = 1;
						g_audio_buffer_init = 1;

						if(g_audio_ctx != NULL){
							g_audio_ctx->f_q_buf_size = F_PCM_Q_SIZE<<1;
							g_audio_ctx->f_pcm_buff = (u8 *)TCC_fo_calloc (__func__, __LINE__,1, g_audio_ctx->f_q_buf_size);
							g_audio_ctx->p_q_buf_size = P_PCM_Q_SIZE;
							g_audio_ctx->p_pcm_buff = (u8 *)TCC_fo_calloc (__func__, __LINE__,1, g_audio_ctx->p_q_buf_size);
						}
					}

					if(giDFSQueue == 0 && g_audio_ctx != NULL){
						g_audio_ctx->f_q_buf_size = F_PCM_Q_SIZE<<1;
						g_audio_ctx->f_pcm_q = cir_q_init(g_audio_ctx->f_pcm_buff, g_audio_ctx->f_q_buf_size);
						g_audio_ctx->p_q_buf_size = P_PCM_Q_SIZE;
						g_audio_ctx->p_pcm_q = cir_q_init(g_audio_ctx->p_pcm_buff, g_audio_ctx->p_q_buf_size);
						giDFSQueue = 1;
					}
					g_audio_ctx->dfs_compensation = inputbuffer->nDFScompensation;

#if defined(DUMP_OUTPUT_TO_FILE)

			IODumpFileWriteOutputData(uiDecoderID, outputbuffer->pBuffer, outputbuffer->nFilledLen);

#endif
					if(giDFSinit == 1){
						if(g_audio_ctx != NULL && g_dfs_on_check == 0){
							//dfs parameter init
							g_audio_ctx->dfs = tcc_dfs_on(g_audio_ctx, 1); //0:full search, 1:half search
							g_dfs_on_check = 1;
						}
					}

					if(g_audio_ctx != NULL && g_dfs_result == 1 && giTime_delay >= 0
					&& g_dfs_broadcast_result == 0 && giDFSinit == 1 && giDFSQueue == 1){
						comp_stc_delay[0] = giTime_delay;
						SEAMLESS_DBG_PRINTF("\033[32m [Seamless] After dfs calc is completed, gap[%d]\033[m\n", giTime_delay);

						// broadcast a event for time delay.
						(*(pPrivate->callbacks->EventHandler))
						(openmaxStandComp,
						pPrivate->callbackData,
						OMX_EventVendorCompenSTCDelay,
						0,
						0,
						(OMX_PTR)comp_stc_delay);
						g_dfs_broadcast_result = 1;
					}

					if(g_audio_ctx != NULL && g_dfs_result == 0 && giTime_delay >= 0
					&& g_dfs_db_broadcast_result == 0 && giDFSinit == 1 && giDFSQueue == 1){
						GetDFSValue_DB_Table(giChannelIndex, &giTime_delay);
						if(giTime_delay > 0){
							comp_stc_delay[0] = giTime_delay;
							SEAMLESS_DBG_PRINTF("\033[32m [Seamless] Before dfs calc is completed, gap exist[%d]\033[m\n", giTime_delay);
							// broadcast a event for time delay.
							(*(pPrivate->callbacks->EventHandler))
							(openmaxStandComp,
							pPrivate->callbackData,
							OMX_EventVendorCompenSTCDelay,
							0,
							0,
							(OMX_PTR)comp_stc_delay);
							g_dfs_db_broadcast_result = 1;
						}
					}

					/*To prevent seamless switching's compenstation, adopted only 1 dfs success sample*/
					if(giSwitching != -1){
						if(pstSaved_dfs_gap[giChannelIndex].saved_dfs_total == 1){
							giSwitchingStart = 0;
						}else{
							giSwitchingStart = 1;
						}
					}

					/*Interval routine*/
					if(dfs_ret == TC_DFS_FOUND/* && giSwitchingStart == 0*/){
						if(interval_time_init == 0 && giDFSInterval > 0 && giDFSInterval <= 300){
							interval_st = dfsclock();
							interval_time_init = 1;
							SEAMLESS_DBG_PRINTF("\033[32m [Seamless] <START> waiting interval=[%d sec] \033[m\n", giDFSInterval);
						}

						if(interval_time_init == 1){
							interval_dt = dfsclock() - interval_st;
							if(interval_dt > 0){
								interval_dt = interval_dt/1000;
								if(saved_interval_dt != interval_dt && interval_dt <= giDFSInterval){
									SEAMLESS_DBG_PRINTF("\033[32m [Seamless] [%d] sec has passed \033[m\n", interval_dt);
									saved_interval_dt = interval_dt;
								}

#if defined (CUSTOMER_DFS_OPERATION)
								st_ret = tcc_tuner_get_strength_index(&giStrengthIndex);
								if(interval_dt == giDFSInterval){
									interval_time_init = 0;
									time_init = 0;
									if(giStrengthIndex >= giDFSStrength && st_ret == 0){
										SEAMLESS_DBG_PRINTF("\033[32m [Seamless] <START> Cycle process (retry calc) \033[m\n");
										tcc_dfs_stop_process();
										giDFSUseOnOff = 1;
										dfs_ret = TC_DFS_INTERVAL_PROCESS;
									}else{
										SEAMLESS_DBG_PRINTF("\033[32m [Seamless] Result --> NG (Signal strength[%d] is lower than [%d]) \033[m\n", giStrengthIndex, giDFSStrength);
									}
								}else if(interval_dt > giDFSInterval){
									interval_time_init = 0;
									time_init = 0;
								}
#else
								if(interval_dt == giDFSInterval){
									interval_time_init = 0;
									time_init = 0;
									SEAMLESS_DBG_PRINTF("\033[32m [Seamless] <START> Cycle process (retry calc) \033[m\n");
									tcc_dfs_stop_process();
									giDFSUseOnOff = 1;
									dfs_ret = TC_DFS_INTERVAL_PROCESS;
								}else if(interval_dt > giDFSInterval){
									interval_time_init = 0;
									time_init = 0;
								}
#endif
							}
						}
					}

					if(g_audio_ctx != NULL && dfs_ret != TC_DFS_FOUND && giDFSinit == 1 && giDFSQueue == 1){
						if(g_audio_buffer_init){
							int i;
							if(uiDecoderID == 0){
								//Fullseg
								short *f_inBuff = (short *)outputbuffer->pBuffer;

								pstAudio_buffer.f_data_len = outputbuffer->nFilledLen/2;
								for(i=0; i<pstAudio_buffer.f_data_len/2; i++){
									pstAudio_buffer.f_ldata[i] = f_inBuff[i*2];
									pstAudio_buffer.f_rdata[i] = f_inBuff[i*2+1];
								}
#if defined(DUMP_OUTPUT_TO_FILE)

								IODumpFileWriteOutputData(2, pstAudio_buffer.f_ldata, pstAudio_buffer.f_data_len);
#endif

								cir_q_push(g_audio_ctx->f_pcm_q, (u8 *)pstAudio_buffer.f_ldata, pstAudio_buffer.f_data_len);
								g_audio_ctx->f_samples = pstAudio_buffer.f_data_len;
								g_audio_ctx->f_samplerate = pPrivate->pAudioPcmMode[uiDecoderID].nSamplingRate;
							}else if(uiDecoderID == 1){
								//Oneseg
								short *p_inBuff = (short *)outputbuffer->pBuffer;

								pstAudio_buffer.p_data_len = outputbuffer->nFilledLen/2;
								for(i=0; i<pstAudio_buffer.p_data_len/2; i++){
									pstAudio_buffer.p_ldata[i] = p_inBuff[i*2];
									pstAudio_buffer.p_rdata[i] = p_inBuff[i*2+1];
								}

#if defined(DUMP_OUTPUT_TO_FILE)
								IODumpFileWriteOutputData(3, pstAudio_buffer.p_ldata, pstAudio_buffer.p_data_len);
#endif
								cir_q_push(g_audio_ctx->p_pcm_q, (u8 *)pstAudio_buffer.p_ldata, pstAudio_buffer.p_data_len);
								g_audio_ctx->p_samples = pstAudio_buffer.p_data_len;
								g_audio_ctx->p_samplerate = pPrivate->pAudioPcmMode[uiDecoderID].nSamplingRate;
							}
						}

						if(time_init == 0){
							t = dfsclock();
							time_init = 1;
						}

						if(g_audio_ctx != NULL){
							while(cir_q_get_stored_size(g_audio_ctx->f_pcm_q) > (MIN_DFS_PCM>>2)
							&& cir_q_get_stored_size(g_audio_ctx->p_pcm_q) > (MIN_DFS_PCM>>2)){
								dfs_ret = tcc_proc_pcm(g_audio_ctx);
								if(dfs_ret == TC_DFS_FOUND || dfs_ret == TC_DFS_FAIL){
									break;
								}
							}
						}
					}
				}
				tcc_dfs_lock_off();
#endif
			}
		}
{
#if defined(ERROR_PCM_MUTE) || defined(ERROR_PCM_FADE_IN)
	if (1)
#else
	if (0)
#endif //#if defined(ERROR_PCM_MUTE) || defined(ERROR_PCM_FADE_IN)
	{
		//check if a pts of audio is continuous or not
		int duration;
		int continuity;
		if(pPrivate->dxb_standard != DxB_STANDARD_DAB && ret >= 0) {
			duration = (pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_out.m_iDecodedSamples * 1000 ) / pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_info.m_sAudioInfo.m_iSamplePerSec;
			if (AudioMonGetState(pPrivateData, uiDecoderID) == 0) {
				//not yet run
				AudioMonSet(pPrivateData, uiDecoderID, outputbuffer->nTimeStamp / 1000, duration);
				AudioMonRun(pPrivateData, uiDecoderID, 1);
			} else {
				continuity = AudioMonCheckContinuity(pPrivateData, uiDecoderID, outputbuffer->nTimeStamp / 1000);
				if (continuity)
				{
					#ifdef TASACHK
					ALOGE("[TASACHK][AUDIO_SHOCK_NOISE] Audio uiDecoderID=%d found discontinuity !!!!!!!!!!!!!!!! continuity=[%d]", (int)uiDecoderID, continuity);
					#endif //TASACHK
//=============================================================
#if defined( ERROR_PCM_MUTE) || defined( ERROR_PCM_FADE_IN)
					if( pPrivate->pAudioDecPrivate[uiDecoderID].iAdecType == AUDIO_ID_AAC)
					{
						pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_out.m_iPacketSize = 0;
						pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_out.m_iDecodedSamples = 0;
#ifdef ERROR_PCM_MUTE
						#ifdef TASACHK
						ALOGE("[TASACHK][AUDIO_SHOCK_NOISE]<audio found discontinuity>  cdk_adec_decode -> ret=%d uiDecoderID=%d mute_target_byte=%d MUTE_BYTE_ERR=%d\n"
							,ret
							,uiDecoderID
							,tPcmMute[uiDecoderID].mute_target_byte
							,tPcmMute[uiDecoderID].MUTE_BYTE_ERR
						);
						#endif //TASACHK

						if ( tPcmMute[uiDecoderID].mute_target_byte < tPcmMute[uiDecoderID].MUTE_BYTE_ERR ){
							ALOGE("[AUDIO_SHOCK_NOISE] apply mute_target_byte[%d](%d->%d)", (int)uiDecoderID, tPcmMute[uiDecoderID].mute_target_byte, tPcmMute[uiDecoderID].MUTE_BYTE_ERR);
							tPcmMute[uiDecoderID].mute_target_byte	= tPcmMute[uiDecoderID].MUTE_BYTE_ERR;
							#ifdef TASACHK
							ALOGE("[TASACHK][AUDIO_SHOCK_NOISE]<audio found discontinuity>  tPcmMute[%d].mute_target_byte = %d <-- (MUTE_BYTE_ERR set again) \n",uiDecoderID,tPcmMute[uiDecoderID].mute_target_byte);
							#endif //TASACHK
						}
#endif //ERROR_PCM_MUTE

#ifdef ERROR_PCM_FADE_IN
						tPcmFade_in[uiDecoderID].fade_in_taget_byte	= 0;
						tPcmFade_in[uiDecoderID].frame_to_fade_in	= 0;
						tPcmFade_in[uiDecoderID].fade_in_tab_index	= 0;
						tPcmFade_in[uiDecoderID].size_to_fade_in	= tPcmFade_in[uiDecoderID].fade_in_frame_size;
#ifndef ERROR_PCM_MUTE
						tPcmFade_in[uiDecoderID].fade_in_taget_byte = tPcmFade_in[uiDecoderID].FADE_IN_BYTE;
#endif //ERROR_PCM_MUTE
#endif //ERROR_PCM_FADE_IN
					}
#endif //defined( ERROR_PCM_MUTE) || defined( ERROR_PCM_FADE_IN)
//=============================================================
				}
				AudioMonSet(pPrivateData, uiDecoderID, outputbuffer->nTimeStamp / 1000, duration);
			}
		}
	}
//=============================================================
#if defined(ERROR_PCM_MUTE) || defined(ERROR_PCM_FADE_IN)
	if( pPrivate->pAudioDecPrivate[uiDecoderID].iAdecType == AUDIO_ID_AAC)
	{
#if 0
		if(ret > 1)
		{
			pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_out.m_iPacketSize		= 0;
			pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_out.m_iDecodedSamples	= 0;

#ifdef ERROR_PCM_MUTE
			#ifdef TASACHK
			ALOGE("[TASACHK][AUDIO_SHOCK_NOISE] cdk_adec_decode -> ret=%d (ERR) uiDecoderID=%d mute_target_byte=%d MUTE_BYTE_ERR=%d \n"
				,ret
				,uiDecoderID
				,tPcmMute[uiDecoderID].mute_target_byte
				,tPcmMute[uiDecoderID].MUTE_BYTE_ERR
			);
			#endif //TASACHK

			if ( tPcmMute[uiDecoderID].mute_target_byte < tPcmMute[uiDecoderID].MUTE_BYTE_ERR ){
				tPcmMute[uiDecoderID].mute_target_byte	= tPcmMute[uiDecoderID].MUTE_BYTE_ERR;
				#ifdef TASACHK
				ALOGE("[TASACHK][AUDIO_SHOCK_NOISE] tPcmMute[%d].mute_target_byte = %d <-- (MUTE_BYTE_ERR set again) \n",uiDecoderID,tPcmMute[uiDecoderID].mute_target_byte);
				#endif //TASACHK
			}
#endif //ERROR_PCM_MUTE

#ifdef ERROR_PCM_FADE_IN
			tPcmFade_in[uiDecoderID].fade_in_taget_byte	= 0;
			tPcmFade_in[uiDecoderID].frame_to_fade_in	= 0;
			tPcmFade_in[uiDecoderID].fade_in_tab_index	= 0;
			tPcmFade_in[uiDecoderID].size_to_fade_in	= tPcmFade_in[uiDecoderID].fade_in_frame_size;
#ifndef ERROR_PCM_MUTE
			tPcmFade_in[uiDecoderID].fade_in_taget_byte	= tPcmFade_in[uiDecoderID].FADE_IN_BYTE;
#endif //ERROR_PCM_MUTE
#endif //ERROR_PCM_FADE_IN
		}
		else
		{
#endif //If 0
			int curr_pcm_size
				= pPrivate->pAudioDecPrivate[uiDecoderID].gsADec.gsADecOutput.m_uiNumberOfChannel * pPrivate->pAudioDecPrivate[uiDecoderID].cdmx_out.m_iDecodedSamples * sizeof(unsigned short);
			short *pInterleaved = (short*)outputbuffer->pBuffer;

#ifdef ERROR_PCM_MUTE
			if(tPcmMute[uiDecoderID].mute_flag_after_SetCh == 1)
			{
				tPcmMute[uiDecoderID].mute_target_byte	= tPcmMute[uiDecoderID].MUTE_BYTE_SEL;
				#ifdef TASACHK
				ALOGE("[TASACHK][AUDIO_SHOCK_NOISE] uiDecoderID=%d mute_flag_after_SetCh=%d --> mute_target_byte=%d(MUTE_BYTE_SEL:%d) set \n"
					,uiDecoderID
					,tPcmMute[uiDecoderID].mute_flag_after_SetCh
					,tPcmMute[uiDecoderID].mute_target_byte
					,tPcmMute[uiDecoderID].MUTE_BYTE_SEL);
				#endif //TASACHK
				tPcmMute[uiDecoderID].mute_flag_after_SetCh = 0;
			}
#endif //ERROR_PCM_MUTE

#ifdef ERROR_PCM_MUTE
			if( ( tPcmMute[uiDecoderID].mute_target_byte > 0 ) && ( curr_pcm_size > 0 ) )
			{
				int curr_mute = ( tPcmMute[uiDecoderID].mute_target_byte > curr_pcm_size ) ? curr_pcm_size : tPcmMute[uiDecoderID].mute_target_byte;

				tPcmMute[uiDecoderID].mute_target_byte -= curr_mute;

				if( curr_mute > 0)
				{
					memset( pInterleaved, 0, curr_mute );
				}

				curr_pcm_size -= curr_mute;

				if( tPcmMute[uiDecoderID].mute_target_byte <= 0 )
				{
					tPcmMute[uiDecoderID].mute_target_byte = 0;
#ifdef ERROR_PCM_FADE_IN
					tPcmFade_in[uiDecoderID].fade_in_taget_byte = tPcmFade_in[uiDecoderID].FADE_IN_BYTE;
					pInterleaved += curr_mute/sizeof(short);
#endif //ERROR_PCM_FADE_IN
				}

				#ifdef TASACHK
				ALOGE("[TASACHK][AUDIO_SHOCK_NOISE] tPcmMute[%d].mute_target_byte :%d \n",uiDecoderID, tPcmMute[uiDecoderID].mute_target_byte);
				#endif //TASACHK

			}
#endif //ERROR_PCM_MUTE
#ifdef ERROR_PCM_FADE_IN
			if( ( tPcmFade_in[uiDecoderID].fade_in_taget_byte > 0 ) && ( curr_pcm_size > 0 ) )
			{
				static const short SeekFadeFacTable[FADE_IN_NUMBER] =
				{
					#if (FADE_IN_NUMBER == 5)
						//Q15
						0x16a1,  //0.17677668
						0x2000,	 //0.24999997
						0x2d41,	 //0.35355335
						0x4000,  //0.49999997
						0x5a82	 //0.70710677
					#elif (FADE_IN_NUMBER == 16)
						327,	983,	1966,	3276,
						4915,	6881,	9175,	11796,
						14745,	18022,	21626,	24903,
						27525,	29818,	31457,	32112
					#endif
				};

				int sample, curr_fade_in_size, curr_frame;

				#ifdef TASACHK
				ALOGE("[TASACHK][AUDIO_SHOCK_NOISE] tPcmFade_in[%d].fade-in byte: %d\n",uiDecoderID, tPcmFade_in[uiDecoderID].fade_in_taget_byte);
				#endif //TASACHK

				do
				{
					short faded;

					if( tPcmFade_in[uiDecoderID].frame_to_fade_in >= FADE_IN_NUMBER )
					{
						tPcmFade_in[uiDecoderID].fade_in_taget_byte = 0;
						tPcmFade_in[uiDecoderID].frame_to_fade_in = 0;
						break;
					}

					curr_fade_in_size = ( tPcmFade_in[uiDecoderID].fade_in_taget_byte > curr_pcm_size ) ? curr_pcm_size : tPcmFade_in[uiDecoderID].fade_in_taget_byte;
					curr_frame =  ( tPcmFade_in[uiDecoderID].size_to_fade_in > curr_fade_in_size ) ? curr_fade_in_size : tPcmFade_in[uiDecoderID].size_to_fade_in;

					for( sample = 0; sample < curr_frame; sample++ )
					{
						faded = ( (int)(*pInterleaved) * (int)(SeekFadeFacTable[tPcmFade_in[uiDecoderID].fade_in_tab_index]) ) >> 15;
						*pInterleaved++ = faded;
					}

					tPcmFade_in[uiDecoderID].size_to_fade_in -= curr_frame;
					curr_pcm_size -= curr_frame;
					tPcmFade_in[uiDecoderID].fade_in_taget_byte -= curr_frame;
					if( tPcmFade_in[uiDecoderID].size_to_fade_in <= 0 )
					{
						tPcmFade_in[uiDecoderID].size_to_fade_in = tPcmFade_in[uiDecoderID].fade_in_frame_size;
						tPcmFade_in[uiDecoderID].frame_to_fade_in ++;
						tPcmFade_in[uiDecoderID].fade_in_tab_index ++;

						#ifdef TASACHK
						ALOGE("[TASACHK][AUDIO_SHOCK_NOISE] tPcmFade_in[%d].fade_in_tab_index: %d \n",uiDecoderID, tPcmFade_in[uiDecoderID].fade_in_tab_index);
						#endif //TASACHK

						if( tPcmFade_in[uiDecoderID].fade_in_tab_index >= FADE_IN_NUMBER )
						{
							tPcmFade_in[uiDecoderID].fade_in_tab_index = 0;
							#ifdef TASACHK
							ALOGE("[TASACHK][AUDIO_SHOCK_NOISE] tPcmFade_in[%d].fade-in end %d \n",uiDecoderID, tPcmFade_in[uiDecoderID].fade_in_taget_byte);
							#endif //TASACHK
						}
					}
				} while( curr_pcm_size > 0 );
			}
#endif //ERROR_PCM_FADE_IN
#if 0
		}
#endif //#if 0
	}
#endif //defined(ERROR_PCM_MUTE) || defined(ERROR_PCM_FADE_IN)
//=============================================================
}
		if (0)
		{
			FILE* fp;
			fp = fopen("/nand/decout.txt", "ab");
			fwrite(outputbuffer->pBuffer, 1, outputbuffer->nFilledLen, fp);
			fclose(fp);
		}
		DBUG_MSG("Audio DEC Success. nTimeStamp = %lld", outputbuffer->nTimeStamp);
		pPrivate->pAudioDecPrivate[uiDecoderID].bPrevDecFail = OMX_FALSE;
	}
	else
	{
		ALOGE( "cdk_audio_dec_run error[%d]: %ld", (int)uiDecoderID, ret );
		pPrivate->pAudioDecPrivate[uiDecoderID].iPrevTS = inputbuffer->nTimeStamp/*same as outputbuffer->nTimeStamp*/;
		pPrivate->pAudioDecPrivate[uiDecoderID].bPrevDecFail = OMX_TRUE;
		pPrivate->pAudioDecPrivate[uiDecoderID].iStartTS = 0;
		pPrivate->pAudioDecPrivate[uiDecoderID].iSamples = 0;
	}

#if defined(DUMP_OUTPUT_TO_FILE)
//IODumpFileWriteOutputData(uiDecoderID, outputbuffer->pBuffer, outputbuffer->nFilledLen);
#endif

	pPrivate->isNewBuffer = 1;
	inputbuffer->nFilledLen = 0;

	if(pPrivate->pAudioDecPrivate[uiDecoderID].bAudioPaused == OMX_TRUE)
	{
		outputbuffer->nFilledLen = 0;
	}

#ifdef PCM_INFO_SIZE
	if(outputbuffer->nFilledLen > 0) {
		memcpy(outputbuffer->pBuffer+outputbuffer->nFilledLen, &pPrivate->pAudioPcmMode[uiDecoderID].nSamplingRate, 4);
		outputbuffer->nFilledLen += 4;
		memcpy(outputbuffer->pBuffer+outputbuffer->nFilledLen, &pPrivate->pAudioPcmMode[uiDecoderID].nChannels, 4);
		outputbuffer->nFilledLen += 4;
		*(int *)(outputbuffer->pBuffer+outputbuffer->nFilledLen) = 0;
		outputbuffer->nFilledLen += 4;
	}
#endif

#ifdef SUPPORT_PVR
	if (outputbuffer->nFilledLen > 0)
	{
		SetPVRFlags(pPrivate, uiDecoderID, outputbuffer);
	}
#endif//SUPPORT_PVR

	pPrivate->pAudioDecPrivate[uiDecoderID].bAudioInDec = OMX_FALSE;

	pthread_mutex_unlock(&pPrivate->pAudioDecPrivate[uiDecoderID].stAudioStart.mutex);
	return;
}

OMX_ERRORTYPE dxb_omx_dxb_audiodec_default_component_Constructor(OMX_COMPONENTTYPE *openmaxStandComp, OMX_STRING cComponentName)
{

	OMX_ERRORTYPE err = OMX_ErrorNone;
	omx_dxb_audio_default_component_PrivateType* omx_audiodec_component_Private;
	omx_base_audio_PortType *inPort,*outPort;
	TCC_ADEC_PRIVATE *pPrivateData;
	OMX_U32 i;
	char value[PROPERTY_VALUE_MAX];

#ifdef SEAMLESS_SWITCHING_COMPENSATION
	tcc_dfs_lock_init();
	OpenDfsDB_Table();
#endif

	if (!openmaxStandComp->pComponentPrivate)
	{
		openmaxStandComp->pComponentPrivate = TCC_fo_calloc (__func__, __LINE__,1, sizeof(omx_dxb_audio_default_component_PrivateType));

		if(openmaxStandComp->pComponentPrivate==NULL)
		{
			return OMX_ErrorInsufficientResources;
		}
	}
	else
	{
		DBUG_MSG("In %s, Error Component %x Already Allocated\n",
				  __func__, (int)openmaxStandComp->pComponentPrivate);
	}

	omx_audiodec_component_Private = openmaxStandComp->pComponentPrivate;
	omx_audiodec_component_Private->ports = NULL;

	property_get("persist.sys.spdif_setting", value, "0");
	omx_audiodec_component_Private->iSPDIFMode = atoi(value);

	/** we could create our own port structures here
	* fixme maybe the base class could use a "port factory" function pointer?
	*/
	err = dxb_omx_base_filter_Constructor(openmaxStandComp, cComponentName);

	DBUG_MSG("constructor of MP2 decoder component is called\n");

	/* Domain specific section for the ports. */
	/* first we set the parameter common to both formats */
	/* parameters related to input port which does not depend upon input audio format	 */
	/* Allocate Ports and call port constructor. */

	omx_audiodec_component_Private->sPortTypesParam[OMX_PortDomainAudio].nStartPortNumber = 0;
	omx_audiodec_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts = 4;

	if (omx_audiodec_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts && !omx_audiodec_component_Private->ports)
	{
	    omx_audiodec_component_Private->ports = TCC_fo_calloc (__func__, __LINE__,omx_audiodec_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts, sizeof(omx_base_PortType *));
		if (!omx_audiodec_component_Private->ports)
		{
	  		return OMX_ErrorInsufficientResources;
		}
		for (i=0; i < omx_audiodec_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts; i++)
		{
		      omx_audiodec_component_Private->ports[i] = TCC_fo_calloc (__func__, __LINE__,1, sizeof(omx_base_audio_PortType));
			  if (!omx_audiodec_component_Private->ports[i])
			  {
			  		if(omx_audiodec_component_Private->ports){
			  			TCC_fo_free(__func__,__LINE__, omx_audiodec_component_Private->ports);
						omx_audiodec_component_Private->ports=NULL;
			  		}
					return OMX_ErrorInsufficientResources;
			  }
		}
	}

	dxb_base_audio_port_Constructor(openmaxStandComp, &omx_audiodec_component_Private->ports[0], 0, OMX_TRUE); // input
	dxb_base_audio_port_Constructor(openmaxStandComp, &omx_audiodec_component_Private->ports[1], 1, OMX_TRUE); // input
	dxb_base_audio_port_Constructor(openmaxStandComp, &omx_audiodec_component_Private->ports[2], 2, OMX_FALSE); // output
	dxb_base_audio_port_Constructor(openmaxStandComp, &omx_audiodec_component_Private->ports[3], 3, OMX_FALSE); // output
		/** parameters related to input port */
	inPort = (omx_base_audio_PortType *) omx_audiodec_component_Private->ports[0];
	inPort->sPortParam.nBufferSize = AUDIO_DXB_IN_BUFFER_SIZE;
	inPort->sPortParam.nBufferCountMin = 4;
	inPort->sPortParam.nBufferCountActual = 4;

	inPort = (omx_base_audio_PortType *) omx_audiodec_component_Private->ports[1];
	inPort->sPortParam.nBufferSize = AUDIO_DXB_IN_BUFFER_SIZE;
	inPort->sPortParam.nBufferCountMin = 4;
	inPort->sPortParam.nBufferCountActual = 4;

	outPort = (omx_base_audio_PortType *) omx_audiodec_component_Private->ports[2];
	outPort->sPortParam.nBufferSize = AUDIO_DXB_OUT_BUFFER_SIZE;
	outPort->sPortParam.format.audio.eEncoding = OMX_AUDIO_CodingPCM;
	outPort->sAudioParam.eEncoding = OMX_AUDIO_CodingPCM;
	outPort->sPortParam.nBufferCountMin = 8;
	outPort->sPortParam.nBufferCountActual = 8;

	outPort = (omx_base_audio_PortType *) omx_audiodec_component_Private->ports[3];
	outPort->sPortParam.nBufferSize = AUDIO_DXB_OUT_BUFFER_SIZE;
	outPort->sPortParam.format.audio.eEncoding = OMX_AUDIO_CodingPCM;
	outPort->sAudioParam.eEncoding = OMX_AUDIO_CodingPCM;
	outPort->sPortParam.nBufferCountMin = 8;
	outPort->sPortParam.nBufferCountActual = 8;

	/** general configuration irrespective of any audio formats */
	/**  setting values of other fields of omx_maddec_component_Private structure */
	omx_audiodec_component_Private->BufferMgmtFunction = dxb_omx_audiodec_twoport_filter_component_BufferMgmtFunction;
	omx_audiodec_component_Private->BufferMgmtCallback = dxb_omx_audiodec_component_BufferMgmtCallback;
	omx_audiodec_component_Private->messageHandler = dxb_omx_audiodec_component_MessageHandler;
	omx_audiodec_component_Private->destructor = dxb_omx_audiodec_component_Destructor;
	openmaxStandComp->SetParameter = dxb_omx_audiodec_component_SetParameter;
	openmaxStandComp->GetParameter = dxb_omx_audiodec_component_GetParameter;
	openmaxStandComp->GetExtensionIndex = dxb_omx_audiodec_component_GetExtensionIndex;

	OMX_S16 nDevID;
	omx_audiodec_component_Private->pAudioDecPrivate = TCC_fo_calloc (__func__, __LINE__,iTotalHandle, sizeof(OMX_AUDIO_DEC_PRIVATE_DATA));
	if(omx_audiodec_component_Private->pAudioDecPrivate == NULL)
	{
		ALOGE("[%s] pAudioDecPrivate TCC_calloc fail / Error !!!!!", __func__);
		return OMX_ErrorInsufficientResources;
	}

	for(nDevID=0 ; nDevID<iTotalHandle ; nDevID++) {
		omx_audiodec_component_Private->pAudioDecPrivate[nDevID].decode_ready = OMX_FALSE;

		omx_audiodec_component_Private->pAudioDecPrivate[nDevID].cdk_core.m_iAudioProcessMode = AUDIO_BROADCAST_MODE; /* decoded pcm mode */

		omx_audiodec_component_Private->pAudioDecPrivate[nDevID].cdk_core.m_psCallback = &(omx_audiodec_component_Private->callback_func);
		omx_audiodec_component_Private->pAudioDecPrivate[nDevID].cdk_core.m_psCallback->m_pfMalloc	 = (void * (*)(unsigned int))malloc;
		omx_audiodec_component_Private->pAudioDecPrivate[nDevID].cdk_core.m_psCallback->m_pfRealloc  = (void * (*)(void*, unsigned int))realloc;
		omx_audiodec_component_Private->pAudioDecPrivate[nDevID].cdk_core.m_psCallback->m_pfFree	  = (void (*)(void*))free;
		omx_audiodec_component_Private->pAudioDecPrivate[nDevID].cdk_core.m_psCallback->m_pfMemcpy	 = (void * (*)(void*, const void*, unsigned int))memcpy;
		omx_audiodec_component_Private->pAudioDecPrivate[nDevID].cdk_core.m_psCallback->m_pfMemmove  = (void * (*)(void*, const void*, unsigned int))memmove;
		omx_audiodec_component_Private->pAudioDecPrivate[nDevID].cdk_core.m_psCallback->m_pfMemset	 = (void * (*)(void*, int, unsigned int))memset;

		omx_audiodec_component_Private->pAudioDecPrivate[nDevID].bAudioStarted = OMX_TRUE;
		omx_audiodec_component_Private->pAudioDecPrivate[nDevID].bAudioPaused = OMX_FALSE;
		omx_audiodec_component_Private->pAudioDecPrivate[nDevID].bAudioInDec = OMX_FALSE;

#ifdef	SUPPORT_PVR
		omx_audiodec_component_Private->pAudioDecPrivate[nDevID].nPVRFlags = 0;
#endif//SUPPORT_PVR
	}

	omx_audiodec_component_Private->eStereoMode = TCC_DXBAUDIO_DUALMONO_LeftNRight;
	omx_audiodec_component_Private->callbackfunction = NULL;

	omx_audiodec_component_Private->pPrivateData = TCC_fo_calloc(__func__,__LINE__, 1, sizeof(TCC_ADEC_PRIVATE));
	if(omx_audiodec_component_Private->pPrivateData == NULL)
	{
		TCC_fo_free(__func__, __LINE__, omx_audiodec_component_Private->pAudioDecPrivate);
		omx_audiodec_component_Private->pAudioDecPrivate = NULL;

		ALOGE("[%s] pPrivateData TCC_calloc fail / Error !!!!!", __func__);
		return OMX_ErrorInsufficientResources;
	}

	pPrivateData = (TCC_ADEC_PRIVATE*) omx_audiodec_component_Private->pPrivateData;

	pPrivateData->guiSamplingRate = 48000; //default 48000
	pPrivateData->guiChannels = 2; //default stereo
	pPrivateData->gfnDemuxGetSTCValue = NULL;

	return err;
}

OMX_ERRORTYPE dxb_OMX_DXB_AudioDec_Default_ComponentInit(OMX_COMPONENTTYPE *openmaxStandComp)
{
	OMX_ERRORTYPE err = OMX_ErrorNone;
	err = dxb_omx_dxb_audiodec_default_component_Constructor(openmaxStandComp,AUDIO_DEC_BASE_NAME);
	return err;
}

void* dxb_omx_audiodec_twoport_filter_component_BufferMgmtFunction (void* param)
{
  OMX_COMPONENTTYPE* openmaxStandComp = (OMX_COMPONENTTYPE*)param;
  omx_base_component_PrivateType* omx_base_component_Private=(omx_base_component_PrivateType*)openmaxStandComp->pComponentPrivate;
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

  while(omx_base_filter_Private->state == OMX_StateIdle || omx_base_filter_Private->state == OMX_StateExecuting ||	omx_base_filter_Private->state == OMX_StatePause ||
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

#if defined(SEAMLESS_SWITCHING_COMPENSATION)
int tcc_dfs_found(audio_ctx_t * _ctx)
{
	int ret = TC_DFS_ERROR;
	int dfs_state = 0;
	int dfs_db_val = 0, dfs_db_val_ret = 0;
	int dfs_db_cnt = 0, dfs_db_cnt_ret = 0;
	int dfs_diff = 0;
	int dfs_origin_gap = 0;
	int dfs_origin_gap2 = 0;
	float dfs_avg = 0.0;
	dfs_out_t * out = NULL;

	out = tc_dfs_get(_ctx->dfs, giChannelIndex, _ctx->dfs_compensation);
	if(out == NULL)
	{
		SEAMLESS_DBG_PRINTF("[%s] out is null\n", __func__);
		return ret;
	}

	dt = dfsclock() - t;
	//check the process time
	if((dt>0) && (dt/1000 > 20)){ //~20sec
		SEAMLESS_DBG_PRINTF("\033[32m [Seamless] Result --> NG (Process Over-time[%d.%d] is larger than [20]) \033[m\n", dt>0?dt / 1000:0, dt>0?dt % 1000:0);
		out->result = 0;
	}

	SEAMLESS_DBG_PRINTF("\033[32m [Seamless] calc timegap[%d] dfs_compensation[%d] \033[m\n", (int)out->timegap, _ctx->dfs_compensation);
	/*1st step*/
	if(_ctx->dfs_compensation == 0 && ((int)out->timegap > 2000  || (int)out->timegap < 101)){
		/*gap check*/
		SEAMLESS_DBG_PRINTF("\033[32m [Seamless] 1st step Result --> NG (gap[%d] is larger than [2000] or less than [101] \033[m\n", out->timegap);
		out->result = 0;
	}

	if(_ctx->dfs_compensation == 1 && ((int)out->timegap > 200  || (int)out->timegap < -200)){
		/*2nd gap check*/
		SEAMLESS_DBG_PRINTF("\033[32m [Seamless] 2nd step Result --> NG (gap[%d] is larger than [200] or less than [-200] \033[m\n", out->timegap);
		out->result = 0;
	}

	/*2nd step*/
	if(_ctx->dfs_compensation == 1 && (int)out->timegap > 2000){
		/*gap check*/
		SEAMLESS_DBG_PRINTF("\033[32m [Seamless] 2nd step Result --> NG (gap[%d] is larger than [2000] \033[m\n", out->timegap);
		out->result = 0;
	}

	if (out->peak_ratio < out->peak_ratio_threshold){
		/*peak ratio check*/
		SEAMLESS_DBG_PRINTF("\033[32m [Seamless] Result --> NG (Peak[%d] is lower than [%d]) \033[m\n", out->peak_ratio, out->peak_ratio_threshold);
		out->result = 0;
	}

	if(out->max_mag_ratio < 0.5 || out->max_mag_ratio >= 4.0){
		/*max_mag_ratio check*/
		SEAMLESS_DBG_PRINTF("\033[32m [Seamless] Result --> NG (Mag[%f] is lower than [0.5] or is larger than [4.0] \033[m\n", out->max_mag_ratio);
		out->result = 0;
	}

	if(giServiceLog == 1){
		out->result = 0;
		giServiceLog = 2;
		SEAMLESS_DBG_PRINTF("\033[32m [Seamless] DFS result is skipped\033[m\n");
	}

	if(out->result == 1){
		if(pstSaved_dfs_gap[giChannelIndex].saved_dfs_total == 0){
			/*check DB cnt value*/
			dfs_db_cnt_ret = GetDFSCnt_DB_Table(giChannelIndex, &dfs_db_cnt);
			if(dfs_db_cnt > 0 && dfs_db_cnt_ret == 0){
				pstSaved_dfs_gap[giChannelIndex].saved_dfs_total = dfs_db_cnt;
			}
		}

		if(_ctx->dfs_compensation == 0){
			pstSaved_dfs_gap[giChannelIndex].saved_dfs_total++;
		}else{
			pstSaved_dfs_gap[giChannelIndex].saved_dfs_total = pstSaved_dfs_gap[giChannelIndex].saved_dfs_total + (1*giDFSmultiplier);
		}


		SEAMLESS_DBG_PRINTF("\033[32m [Seamless] giChannelIndex[%d] saved_avg_dfs_gap[%d]\033[m\n", giChannelIndex, pstSaved_dfs_gap[giChannelIndex].saved_avg_dfs_gap);

		if(giChannelIndex != 0 && (pstSaved_dfs_gap[giChannelIndex].saved_avg_dfs_gap == 0))
		{
			/*check DB gap value*/
			dfs_db_val_ret = GetDFSValue_DB_Table(giChannelIndex, &dfs_db_val);
			if(dfs_db_val > 0 && dfs_db_val_ret == 0){
				/*DB store data existing*/
				pstSaved_dfs_gap[giChannelIndex].saved_avg_dfs_gap = dfs_db_val;
			}else{
				/*DB store data nothing*/
				pstSaved_dfs_gap[giChannelIndex].saved_avg_dfs_gap = (int)(out->timegap);
			}
			pstSaved_dfs_gap[giChannelIndex].saved_before_avg_dfs_gap = pstSaved_dfs_gap[giChannelIndex].saved_avg_dfs_gap;
				
			SEAMLESS_DBG_PRINTF("\033[32m [Seamless] DFS avg value[%d] from DB\033[m\n", pstSaved_dfs_gap[giChannelIndex].saved_before_avg_dfs_gap);
		}

		if(pstSaved_dfs_gap[giChannelIndex].saved_avg_dfs_gap != 0)
		{
			/*dfs success, store data has exist*/
			dfs_state = 0;
			dfs_avg = (float)pstSaved_dfs_gap[giChannelIndex].saved_avg_dfs_gap;

			if(_ctx->dfs_compensation == 0){
				//before switching
				dfs_origin_gap = out->timegap;
				out->timegap = dfs_origin_gap + giDFSgapadjust;
				SEAMLESS_DBG_PRINTF("\033[32m [Seamless] 1st step DFS adjust[%d] original value[%d] adjust value[%d]\033[m\n", giDFSgapadjust, dfs_origin_gap, out->timegap);

				dfs_diff = (int)out->timegap - pstSaved_dfs_gap[giChannelIndex].saved_avg_dfs_gap;
				dfs_diff = ABS(dfs_diff);
				if(giDFSNtimes > pstSaved_dfs_gap[giChannelIndex].saved_dfs_total){
					/*Until giDFSNtimes is over Total, forcibly saved the dfs value*/
					SEAMLESS_DBG_PRINTF("\033[32m [Seamless] <START> Ntimes process (Ntimes[%d] > Total[%d])\033[m\n", giDFSNtimes, pstSaved_dfs_gap[giChannelIndex].saved_dfs_total);
					SEAMLESS_DBG_PRINTF("\033[32m  [Seamless] 1st step dfs_avg calc...\033[m\n");
					dfs_avg = ((dfs_avg * (pstSaved_dfs_gap[giChannelIndex].saved_dfs_total - 1)) + (int)(out->timegap)) / (pstSaved_dfs_gap[giChannelIndex].saved_dfs_total );
					 pstSaved_dfs_gap[giChannelIndex].saved_avg_dfs_gap = ROUND(dfs_avg);
					pstSaved_dfs_gap[giChannelIndex].saved_before_avg_dfs_gap = pstSaved_dfs_gap[giChannelIndex].saved_avg_dfs_gap;
					SEAMLESS_DBG_PRINTF("\033[32m [Seamless] DFS avg value for 2nd step[%d]\033[m\n", pstSaved_dfs_gap[giChannelIndex].saved_before_avg_dfs_gap);
				}else{
					if(dfs_diff <= giDFSRange){
						/*check dfs range value*/
						SEAMLESS_DBG_PRINTF("\033[32m [Seamless] result --> OK (gap[%d]) --> OK (abs(gap[%d]-avr[%d])=[%d] <= range[%d]) Total[%d]\033[m\n",
						(int)(out->timegap), (int)(out->timegap), (int)pstSaved_dfs_gap[giChannelIndex].saved_avg_dfs_gap, dfs_diff, giDFSRange, pstSaved_dfs_gap[giChannelIndex].saved_dfs_total);
						SEAMLESS_DBG_PRINTF("\033[32m  [Seamless] 1st step dfs_avg calc... \033[m\n");
						dfs_avg = ((dfs_avg * (pstSaved_dfs_gap[giChannelIndex].saved_dfs_total - 1)) + (int)(out->timegap)) / (pstSaved_dfs_gap[giChannelIndex].saved_dfs_total );
						pstSaved_dfs_gap[giChannelIndex].saved_avg_dfs_gap = ROUND(dfs_avg);
						pstSaved_dfs_gap[giChannelIndex].saved_before_avg_dfs_gap = pstSaved_dfs_gap[giChannelIndex].saved_avg_dfs_gap;
						SEAMLESS_DBG_PRINTF("\033[32m [Seamless] DFS avg value for 2nd step[%d]\033[m\n", pstSaved_dfs_gap[giChannelIndex].saved_before_avg_dfs_gap);
					}else{
						SEAMLESS_DBG_PRINTF("\033[32m [Seamless] result --> OK (gap[%d]) --> Ingore (abs(gap[%d]-avr[%d])=[%d] > range[%d]) Total[%d]\033[m\n",
						(int)(out->timegap), (int)(out->timegap), pstSaved_dfs_gap[giChannelIndex].saved_avg_dfs_gap, dfs_diff, giDFSRange, pstSaved_dfs_gap[giChannelIndex].saved_dfs_total);
						pstSaved_dfs_gap[giChannelIndex].saved_dfs_total--;
						out->result = 0;
						dfs_state = 2;
						tcc_dfs_stop_process();
						giDFSUseOnOff = 1;
						ret = TC_DFS_FOUND;
					}
				}
			}else{
				SEAMLESS_DBG_PRINTF("\033[32m  [Seamless] 2nd step dfs_avg calc... saved_dfs_avg_gap[%d] multiplier[%d] \033[m\n", pstSaved_dfs_gap[giChannelIndex].saved_before_avg_dfs_gap, giDFSmultiplier);

				dfs_origin_gap2 = out->timegap;
				out->timegap = dfs_origin_gap2 + giDFSgapadjust2;
				SEAMLESS_DBG_PRINTF("\033[32m [Seamless] 2nd step DFS adjust[%d] original value[%d] adjust value[%d]\033[m\n", giDFSgapadjust2, dfs_origin_gap2, out->timegap);

				dfs_avg = ((dfs_avg * (pstSaved_dfs_gap[giChannelIndex].saved_dfs_total - (1*giDFSmultiplier))) + ((int)(out->timegap) + pstSaved_dfs_gap[giChannelIndex].saved_before_avg_dfs_gap)*giDFSmultiplier) / (pstSaved_dfs_gap[giChannelIndex].saved_dfs_total);
				if(ROUND(dfs_avg) > 2000){
					SEAMLESS_DBG_PRINTF("\033[32m [Seamless]  Gap fail-safe [%d]\033[m\n", ROUND(dfs_avg));
					pstSaved_dfs_gap[giChannelIndex].saved_dfs_total--;
					out->result = 0;
					dfs_state = 2;
					tcc_dfs_stop_process();
					giDFSUseOnOff = 1;
					ret = TC_DFS_FOUND;
				}else{
				 	pstSaved_dfs_gap[giChannelIndex].saved_avg_dfs_gap = ROUND(dfs_avg);
				}
			}

			giTime_delay = pstSaved_dfs_gap[giChannelIndex].saved_avg_dfs_gap;
		}

		if(out->result == 1){
			giTime_delay_monitor = (int)(out->timegap);
			g_dfs_result = 1;
			_ctx->dfs_distance = out->distance<<1;
			cir_q_discard(_ctx->f_pcm_q, _ctx->dfs_distance);
			cir_q_discard(_ctx->p_pcm_q, _ctx->dfs_distance);
			ret = TC_DFS_FOUND;
		}
	}else{
		/*dfs fail, store data nothing, retry routine*/
		dfs_state = 2;
		tcc_dfs_stop_process();
		giDFSUseOnOff = 1;
		ret = TC_DFS_FOUND;
	}

	dfs_state_monitor = dfs_state;

	if(out->result == 1){
		UpdateDB_DfsDB_Table(giChannelIndex, pstSaved_dfs_gap[giChannelIndex].saved_dfs_total, (int)pstSaved_dfs_gap[giChannelIndex].saved_avg_dfs_gap);
		SEAMLESS_DBG_PRINTF("\033[32m [Seamless] result --> OK (gap[%d]) --> Save to DB(avr[%d] total[%d]) \033[m\n",
		(int)(out->timegap), (int)pstSaved_dfs_gap[giChannelIndex].saved_avg_dfs_gap, pstSaved_dfs_gap[giChannelIndex].saved_dfs_total);
	}
#if defined(DUMP_DFS_RESULT)
		FILE *fp;
//		fp = fopen("/run/media/mmcblk0p10/dfs_result.txt","a");
		fp = fopen("/mnt/sdcard/dfs_result.txt","a");
		if(fp){
			if(dfs_found_cnt == 0){
				fprintf(fp,"cnt     total    channel	  result     timegap(dfs)      timegap(comp)      timegap(saved)        peak    	  distance	      proc.time\n");
			}
			dfs_found_cnt = dfs_found_cnt + 1;
			    fprintf(fp,"%d	    %d       %d		  %d 	     %d		           %d 	              %d		            %d            %d              %d.%d		\n", \
			dfs_found_cnt, pstSaved_dfs_gap[giChannelIndex].saved_dfs_total, giChannelIndex, out->result, (int)(out->timegap), giTime_delay, pstSaved_dfs_gap[giChannelIndex].saved_avg_dfs_gap, \
			out->peak_ratio, out->distance, dt>0?dt / 1000:0, dt>0?dt % 1000:0);
			fclose(fp);
		}else{
			SEAMLESS_DBG_PRINTF("[%s] DFS DUMP Fail\n", __func__);
		}
#endif

	return ret;
}

int tcc_proc_pcm(audio_ctx_t * _ctx)
{
	OMX_S32 dfs_res = TC_DFS_ERROR;
	OMX_S32 fullseg_samples = 0, oneseg_samples = 0;
	OMX_S16 temp_f_buff[1024] = {0,};
	OMX_S16 temp_p_buff[1024] = {0,};

	if(!f_buf_check(_ctx)){
		if(!cir_q_pop(_ctx->f_pcm_q, (u8 *)temp_f_buff, (MIN_DFS_PCM>>1))){
			fullseg_samples = (MIN_DFS_PCM >>2);
		}
	}

	if(!p_buf_check(_ctx)){
		if(!cir_q_pop(_ctx->p_pcm_q, (u8 *)temp_p_buff, (MIN_DFS_PCM>>1))){
			oneseg_samples = (MIN_DFS_PCM >>2);
		}
	}

#if defined(DUMP_OUTPUT_TO_FILE)
		IODumpFileWriteOutputData(0, temp_f_buff, (MIN_DFS_PCM >>1));
		IODumpFileWriteOutputData(1, temp_p_buff, (MIN_DFS_PCM >>1));
#endif

//	SEAMLESS_DBG_PRINTF("\033[32m [Seamless] [%s] start\n  \033[m\n", __func__);
	dfs_res = tc_dfs_proc(_ctx->dfs, temp_f_buff, fullseg_samples, temp_p_buff, oneseg_samples);
//	SEAMLESS_DBG_PRINTF("\033[32m [Seamless] [%s] dfs_res[%d] end\n  \033[m\n", __func__, dfs_res);
	if(dfs_res == TC_DFS_FOUND) {
		dfs_res = tcc_dfs_found(_ctx);
	}

	return dfs_res;
}

void tcc_dfs_init(void)
{
	SEAMLESS_DBG_PRINTF("\033[32m [Seamless] [%s] start\n  \033[m\n", __func__);

	//initialize
	if(g_audio_ctx == NULL){
		g_audio_ctx = (audio_ctx_t *)TCC_fo_calloc (__func__, __LINE__,1, sizeof(audio_ctx_t));
		SEAMLESS_DBG_PRINTF("\033[32m [Seamless] [%s] size[%d] g_audio_ctx[%p]\n  \033[m\n", __func__, sizeof(audio_ctx_t), g_audio_ctx);
		if(g_audio_ctx == NULL){
			return;
		}
	}

	memset(g_audio_ctx, 0, sizeof(audio_ctx_t));
	g_audio_ctx->p_samples = g_audio_ctx->f_samples = 1024;
	g_audio_ctx->p_samplerate = g_audio_ctx->f_samplerate = 48000;
	SEAMLESS_DBG_PRINTF("\033[32m [Seamless] [%s] end\n  \033[m\n", __func__);
}

void tcc_dfs_deinit(void){
	SEAMLESS_DBG_PRINTF("\033[32m [Seamless] [%s] start\n  \033[m\n", __func__);
	if(g_audio_ctx == NULL){
		SEAMLESS_DBG_PRINTF("\033[31m [Seamless] [%s]  g_audio_ctx is NULL\033[m\n", __func__);
	}

	if(g_audio_ctx != NULL && giDFSinit == 1 && g_dfs_on_check == 1){
		tcc_dfs_off(g_audio_ctx);

		//pcm buffer free
		if(g_audio_ctx->f_pcm_buff){
			TCC_fo_free (__func__, __LINE__,g_audio_ctx->f_pcm_buff);
			g_audio_ctx->f_pcm_buff = NULL;
		}

		if(g_audio_ctx->p_pcm_buff){
			TCC_fo_free (__func__, __LINE__,g_audio_ctx->p_pcm_buff);
			g_audio_ctx->p_pcm_buff = NULL;
		}
		SEAMLESS_DBG_PRINTF("\033[32m [Seamless] [%s] g_audio_ctx free[%p]\n  \033[m\n", __func__, g_audio_ctx);
		TCC_fo_free (__func__, __LINE__,g_audio_ctx);
		g_audio_ctx = NULL;

		giDFSinit = 0;
        g_dfs_on_check = 0;
        g_audio_buffer_init = 0;
	}
	SEAMLESS_DBG_PRINTF("\033[32m [Seamless] [%s] end\n  \033[m\n", __func__);
}

void tcc_dfs_stop_process(void){
	SEAMLESS_DBG_PRINTF("\033[32m [Seamless] [%s] start\n  \033[m\n", __func__);
	if(g_audio_ctx != NULL && giDFSinit == 1){

		//queue free
		if(g_audio_ctx->f_pcm_q){
			cir_q_deinit(g_audio_ctx->f_pcm_q);
			g_audio_ctx->f_pcm_q = 0;
		}

		if(g_audio_ctx->p_pcm_q){
			cir_q_deinit(g_audio_ctx->p_pcm_q);
			g_audio_ctx->p_pcm_q = 0;
		}

		tc_dfs_reinit(g_audio_ctx->dfs);

        giDFSQueue = 0;
        g_dfs_result = 0;
		giTime_delay = 0;
		g_dfs_broadcast_result = 0;
		g_dfs_db_broadcast_result = 0;
		time_init = giDFSUseOnOff = 0;
		giTime_delay_monitor = dfs_state_monitor = 0;
		dfs_ret = TC_DFS_ERROR;
		interval_time_init = 0;
	}
	SEAMLESS_DBG_PRINTF("\033[32m [Seamless] [%s] end\n  \033[m\n", __func__);
}

int tcc_dfs_on(audio_ctx_t *_ctx, unsigned int _fast_flag)
{
	SEAMLESS_DBG_PRINTF("\033[32m [Seamless] [%s] start\n  \033[m\n", __func__);

	int error = -1;

	if (_ctx->dfs) {
		tc_dfs_term(_ctx->dfs);
		_ctx->dfs = 0;
	}

	if(_ctx->f_samples == 0)
		_ctx->f_samples = 1024;
	if(_ctx->p_samples == 0)
		_ctx->p_samples = 1024;

	if(_ctx->f_samplerate == 0)
		_ctx->f_samplerate= 48000;
	if(_ctx->p_samplerate == 0)
		_ctx->p_samplerate= 48000;

	_ctx->dfs_conf.samples = _ctx->f_samples;
	_ctx->dfs_conf.p_samples = _ctx->p_samples;
	_ctx->dfs_conf.fs = _ctx->f_samplerate;

	if(_ctx->dfs_conf.fs > 0){
		_ctx->dfs = tc_dfs_init(_ctx->dfs_conf, 3, 1); //0: 0~search range, 1:-search range ~ + search range
	}

	if (!_ctx->dfs) {
		SEAMLESS_DBG_PRINTF("[%s] can't initialize dfs!!!\n", __func__);
		goto EXIT;
	}
//	SEAMLESS_DBG_PRINTF("[%s] fast(%d) fs=%d dist=%d\n", __func__, _fast_flag, _ctx->dfs_conf.fs, _ctx->dfs_conf.distance);
	error = 0;
EXIT:
	if (error<0) {
		tcc_dfs_off(_ctx);
	}

	SEAMLESS_DBG_PRINTF("\033[32m [Seamless] [%s] end\n  \033[m\n", __func__);

	return _ctx->dfs;
}

void tcc_dfs_off(audio_ctx_t *_ctx)
{
	SEAMLESS_DBG_PRINTF("\033[32m [Seamless] [%s] start\n  \033[m\n", __func__);
	if (_ctx->dfs) {
		tc_dfs_term(_ctx->dfs);
		_ctx->dfs = 0;
	}
	SEAMLESS_DBG_PRINTF("\033[32m [Seamless] [%s] end\n  \033[m\n", __func__);
}

/**********************************************/
//circular queue
u32 cir_q_init(u8 * _buffer, u32 _size)
{
//	SEAMLESS_DBG_PRINTF("cir_q_init\n");
	u32 handle = 0;
	cir_q_t * cir_q = NULL;
	if (_buffer==NULL || _size==0) {
		goto EXIT;
	}
	cir_q = (cir_q_t*)TCC_fo_calloc (__func__, __LINE__,1, sizeof(cir_q_t));
	if (cir_q != NULL) {
		memset(cir_q, 0, sizeof(cir_q_t));
		cir_q->buffer = _buffer;
		cir_q->size = _size;
		handle = (u32)cir_q;
	}
EXIT:
	return handle;
}

void cir_q_deinit(u32 _handle)
{
	cir_q_t * cir_q = NULL;
	if (_handle == 0) {
		goto EXIT;
	}
	cir_q = (cir_q_t*)_handle;
	TCC_fo_free (__func__, __LINE__,cir_q);
EXIT:
	return;
}

u32 cir_q_get_stored_size(u32 _handle)
{
//	SEAMLESS_DBG_PRINTF("cir_q_get_stored_size [%d]\n", _handle);
	s32 size = 0;
	cir_q_t * cir_q = NULL;
	if(_handle == 0) {
		goto EXIT;
	}
	cir_q = (cir_q_t*)_handle;
	size = (s32)(cir_q->wp - cir_q->rp);
	if (size<0) {
		size += cir_q->size;
	}
EXIT:
	return size;
}

u32 cir_q_get_free_size(u32 _handle)
{
//	SEAMLESS_DBG_PRINTF("cir_q_get_free_size [%d]\n", _handle);

	s32 size = 0;
	cir_q_t * cir_q = NULL;
	if (_handle == 0) {
		goto EXIT;
	}
	cir_q = (cir_q_t*)_handle;
	size = (s32)(cir_q->rp - cir_q->wp - 1);
//	SEAMLESS_DBG_PRINTF("cir_q_get_free_size rp[%d] wp[%d]\n", _handle, cir_q->rp, cir_q->wp);
	if (size<0) {
		size += cir_q->size;
	}
EXIT:
	return size;
}

s32 cir_q_discard(u32 _handle, u32 _size)
{
	s32 error = -1;
	cir_q_t * cir_q = NULL;
	if (_handle == 0) {
		goto EXIT;
	}
	if (cir_q_get_stored_size(_handle) < _size) {
		goto EXIT;
	}
	cir_q = (cir_q_t*)_handle;
	cir_q->rp += _size;
	if (cir_q->rp >= cir_q->size) {
		cir_q->rp -= cir_q->size;
	}
	error = 0;
EXIT:
	return error;
}

s32 cir_q_push(u32 _handle, u8 * _data, u32 _size)
{
	s32 error = -1;
	s32 temp = 0, remain = 0;
	cir_q_t * cir_q = NULL;
	u32 available_size = 0;

	if (_data == NULL) {
		SEAMLESS_DBG_PRINTF("[%s] _data NULL\n", __func__);
		goto EXIT;
	}

	if (_handle == 0) {
		SEAMLESS_DBG_PRINTF("[%s] _handle zero\n", __func__);
		goto EXIT;
	}

	if ((available_size=cir_q_get_free_size(_handle)) < _size) {
		//SEAMLESS_DBG_PRINTF("[%s] not enough buffer for %d bytes avail %d bytes\n", __func__, _size, available_size);
		goto EXIT;
	}
	cir_q = (cir_q_t*)_handle;
	temp = MIN(_size, (cir_q->size-cir_q->wp));
	memcpy(&cir_q->buffer[cir_q->wp], _data, temp);
	remain = _size - temp;
	if (remain > 0) {
		memcpy(cir_q->buffer, &_data[temp], remain);
	}
	cir_q->wp += _size;
	if (cir_q->wp >= cir_q->size) {
		cir_q->wp -= cir_q->size;
	}
	error = 0;
//	SEAMLESS_DBG_PRINTF("cir_q_push [%d] [%d]\n", _handle, cir_q->buffer[cir_q->wp]);
//	SEAMLESS_DBG_PRINTF("[%s] handle[%d], temp size=%d cir size %d\n", __func__, _handle, temp, (cir_q->size-cir_q->wp));
EXIT:
	return error;
}

s32 cir_q_pop(u32 _handle, u8 * _buffer, u32 _size)
{
	s32 error = -1;
	s32 temp = 0, remain = 0;
	cir_q_t * cir_q = NULL;
	if (_handle == 0) {
		goto EXIT;
	}
	if (cir_q_get_stored_size(_handle) < _size) {
		goto EXIT;
	}
	cir_q = (cir_q_t*)_handle;
	temp = MIN(_size, (cir_q->size-cir_q->rp));
	memcpy(_buffer, &cir_q->buffer[cir_q->rp], temp);

	remain = _size - temp;
	if (remain > 0) {
		memcpy(&_buffer[temp], cir_q->buffer, remain);
	}
	cir_q->rp += _size;
	if (cir_q->rp >= cir_q->size) {
		cir_q->rp -= cir_q->size;
	}
	error = 0;
//	SEAMLESS_DBG_PRINTF("cir_q_pop [%d]\n", _handle);
//	SEAMLESS_DBG_PRINTF("[%s] rp=%d wp=%d\n", __func__, cir_q->rp, cir_q->wp);
EXIT:
	return error;
}

s32 p_buf_check(audio_ctx_t * _ctx)
{
	s32 error = -1;
	u32 stored_pcm = 0;
	u32 buf_thres = 0;
	u32 play_thres = 0; // no reason
	u32 status = -1;
	if (!_ctx->p_pcm_q) {
		goto EXIT;
	}

	buf_thres = _ctx->p_q_buf_size>>1; // half of max size, no reason
	play_thres = (1024*1);
	stored_pcm = cir_q_get_stored_size(_ctx->p_pcm_q);

	if (stored_pcm > play_thres) {
		status = 1;
	} else if (stored_pcm < buf_thres) {
		status = 0;
	}
	if (status == 1) {
		error = 0;
	}
EXIT:
	return error;
}

s32 f_buf_check(audio_ctx_t * _ctx)
{
	s32 error = -1;
	u32 stored_pcm = 0;
	u32 buf_thres = 0;
	u32 play_thres = 0;
	u32 status = -1;
	if (!_ctx->f_pcm_q) {
		goto EXIT;
	}

	buf_thres = (_ctx->f_q_buf_size<<1)+(_ctx->f_q_buf_size<<2);
	play_thres = (1024*1);
	stored_pcm = cir_q_get_stored_size(_ctx->f_pcm_q);

	if (stored_pcm > play_thres) {
		status = 1;
	} else if (stored_pcm < buf_thres) {
		status = 0;
	}
	if (status == 1) {
		error = 0;
	}
EXIT:
	return error;
}

void tcc_dfs_buffer_clear()
{
	memset(&pstAudio_buffer, 0, sizeof(SAVED_PCM_BUFFER));
}

void tcc_dfs_saved_info_clear()
{
	SEAMLESS_DBG_PRINTF("\033[32m [Seamless] [%s] start giChannelIndex[%d] [%d:%d]\n  \033[m\n", __func__, giChannelIndex, 
		pstSaved_dfs_gap[giChannelIndex].saved_before_avg_dfs_gap, pstSaved_dfs_gap[giChannelIndex].saved_avg_dfs_gap);

	pstSaved_dfs_gap[giChannelIndex].saved_before_avg_dfs_gap = 0;
	pstSaved_dfs_gap[giChannelIndex].saved_avg_dfs_gap = 0;
	
	SEAMLESS_DBG_PRINTF("\033[32m [Seamless] [%s] end giChannelIndex[%d]  [%d:%d]\n  \033[m\n", __func__, giChannelIndex, 
		pstSaved_dfs_gap[giChannelIndex].saved_before_avg_dfs_gap, pstSaved_dfs_gap[giChannelIndex].saved_avg_dfs_gap);
}
#endif
