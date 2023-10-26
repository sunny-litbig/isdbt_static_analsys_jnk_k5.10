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
#define LOG_TAG	"ISDBT_Proc"

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <sys/ioctl.h>
#include <utils/Log.h>
#include <cutils/properties.h>

#ifndef uint32_t
typedef unsigned int uint32_t;
#endif

#include <tccfb_ioctrl.h>
#include <tcc_dxb_memory.h>
#include <tcc_dxb_thread.h>
#include "tcc_isdbt_manager_tuner.h"
#include "tcc_isdbt_manager_demux.h"
#include "tcc_isdbt_manager_demux_subtitle.h"
#include "tcc_isdbt_manager_video.h"
#include "tcc_isdbt_manager_audio.h"
#include "tcc_isdbt_manager_cas.h"
#include "tcc_dxb_manager_sc.h"
#include "tcc_isdbt_proc.h"
#include "tcc_isdbt_event.h"
#include "tcc_msg.h"
#include "TsParse_ISDBT.h"
#include "TsParse_ISDBT_DBLayer.h"
#include "TsParse_ISDBT_PngDec.h"
#include "subtitle_system.h"
#include <subtitle_display.h>
#include "log_signal.h"
#include "tcc_dxb_timecheck.h"

#define   DEBUG_MSG(msg...)  ALOGD(msg)

extern "C" void ResetSIVersionNumber(void);
extern "C" void ISDBT_Get_ServiceInfo(unsigned int *pPcrPid, unsigned int *pServiceId);
extern "C" void ISDBT_Get_ComponentTagInfo(unsigned int *TotalCount, unsigned int *pCompnentTags);
extern "C" void ISDBT_Init_DescriptorInfo(void);

typedef enum {
	DXBPROC_MSG_SET,
	DXBPROC_MSG_SCAN,
	DXBPROC_MSG_SCAN_AND_SET,
	DXBPROC_MSG_HANDOVER,
	DXBPROC_MSG_FIELDLOG,
	DXBPROC_MSG_SETPRESETMODE,
	DXBPROC_MSG_MAX
}E_DXBPROC_MSG;

typedef struct {
	E_DXBPROC_MSG cmd;
	void *arg; //argument
}ST_DXBPROC_MSG;

typedef struct {
	unsigned int uiAVSyncMode; // 0 : first frame display with ignore PTS, 1 : Normal(check PTS and STC) , 2 : Audio start sync with video, else : NOT DEIFNED
	unsigned int uiCountrycode;
	unsigned int uiChannel;
	unsigned int uiAudioTotalCount;
	unsigned int puiAudioPID[TCCDXBPROC_AUDIO_MAX_NUM];
	unsigned int puiAudioStreamType[TCCDXBPROC_AUDIO_MAX_NUM];
	unsigned int uiVideoTotalCount;
	unsigned int puiVideoPID[TCCDXBPROC_VIDEO_MAX_NUM];
	unsigned int puiVideoStreamType[TCCDXBPROC_VIDEO_MAX_NUM];
	unsigned int uiSubAudioTotalCount;
	unsigned int puiAudioSubPID[TCCDXBPROC_AUDIO_MAX_NUM];
	unsigned int puiAudioSubStreamType[TCCDXBPROC_AUDIO_MAX_NUM];
	unsigned int uiSubVideoTotalCount;
	unsigned int puiVideoSubPID[TCCDXBPROC_VIDEO_MAX_NUM];
	unsigned int puiVideoSubStreamType[TCCDXBPROC_VIDEO_MAX_NUM];
	unsigned int uiPcrPID, uiPcrSubPID;
	unsigned int uiStPID, uiSiPID;
	unsigned int uiStSubPID, uiSiSubPID;
	unsigned int uiServiceID, uiServiceSubID;
	unsigned int uiPmtPID, uiPmtSubPID;
	unsigned int uiCAECMPID, uiACECMPID;
	unsigned int uiNetworkID;
	unsigned int uiAudioIndex;
	unsigned int uiAudioMainIndex;
	unsigned int uiAudioSubIndex;
	unsigned int uiVideoIndex;
	unsigned int uiVideoMainIndex;
	unsigned int uiVideoSubIndex;
	unsigned int uiAudioMode;
	unsigned int uiServiceType, uiServiceSubType;

	unsigned int uiState;
	pthread_mutex_t lockDxBProc;
	TccMessage *pDxBProcMsg;
	pthread_t DxBProcMsgHandler;
	unsigned int uiMsgHandlerRunning; // 0 : Not running, 1 : running

	unsigned int uiStopHandover;
	unsigned int uiRaw_w;
	unsigned int uiRaw_h;
	unsigned int uiView_w;
	unsigned int uiView_h;

	unsigned int uiCurrDualDecodeMode;

	int fLogSignalEnabled;		//It's used only while field test. If field log is enabled, SQ information will be saved to storage periodically.

	TCCDXBSCAN_TYPE scanType;    // Noah / 2017.04.07 / IM478A-13, Improvement of the Search API(Auto Search) / For only AUTOSEARCH now.
}ST_TCCDXB_PROC;


typedef struct {
	unsigned int uiComponentTagCount, uiSubComponentTagCount;
	unsigned int puiComponentTag[TCCDXBPROC_COMPONENT_TAG_MAX_NUM], puiSubComponentTag[TCCDXBPROC_COMPONENT_TAG_MAX_NUM];
}ST_TCCDXB_PROC2;

typedef struct {
	int iOnoff;
	int iInterval;
	int iStrength;
	int iNtimes;
	int iRange;
	int iGapadjust;
	int iGapadjust2;
	int iMultiplier;
}SEAMLESS_TCCDXB;

static ST_TCCDXB_PROC gTCCDxBProc = {0, 0, 0,
										0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
										0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, {0, 0, 0, 0}, {0, 0, 0, 0},
										0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
										0,
										0, 0, 0, 0, 0, 0, 0,
										0, 0,
										TCCDXBPROC_STATE_INIT, PTHREAD_MUTEX_INITIALIZER, 0, 0, 0,
										0,
										0, 0, 0, 0,
										0,
										0};
static ST_TCCDXB_PROC2 gTCCDxBProc2;

static unsigned int	iDual_mode = TCCDXBPROC_MAIN;

static int gScanRequestCount;
static int gStopRequest;
static SEAMLESS_TCCDXB gTCCDxBSeamless = {1, 20, 40, 20, 100, 0, 0, 1}; /*onoff, interval, strength, ntimes, range, gapadjust, gapadjust2, muliplier*/
static int setVideoOnffValue = 1;
static pthread_mutex_t gMutex_setVideoOnffValue;    // Noah, 20161005

static unsigned int g_bSearchAndSetChannel_InProgress = 0;    // Noah, 20190219, IM692A-37, 0 == NOT Progress Ing, 1 == In Progress


//#define STOP_TIME_CHECK
#if defined(STOP_TIME_CHECK)
#include <sys/time.h>
static long long StopTimeCheck(void)
{
	long long systime;
	struct timeval tv;	
	gettimeofday (&tv, NULL);
	systime = (long long) tv.tv_sec * 1000 + tv.tv_usec / 1000;
	
	return systime;
}
#endif

/****************************************************************************
DEFINITION OF MUTEX
****************************************************************************/
void tcc_dxb_VideoOnffValue_lock_init(void)
{
	pthread_mutex_init(&gMutex_setVideoOnffValue, NULL);
}

void tcc_dxb_VideoOnffValue_lock_deinit(void)
{
	pthread_mutex_destroy(&gMutex_setVideoOnffValue);
}

void tcc_dxb_VideoOnffValue_lock_on(void)
{
	pthread_mutex_lock(&gMutex_setVideoOnffValue);
}

void tcc_dxb_VideoOnffValue_lock_off(void)
{
	pthread_mutex_unlock(&gMutex_setVideoOnffValue);
}

void tcc_dxb_proc_lock_init(void)
{
	pthread_mutex_init(&gTCCDxBProc.lockDxBProc, NULL);
}

void tcc_dxb_proc_lock_deinit(void)
{
	pthread_mutex_destroy(&gTCCDxBProc.lockDxBProc);
}

void tcc_dxb_proc_lock_on(void)
{
	pthread_mutex_lock(&gTCCDxBProc.lockDxBProc);
}

void tcc_dxb_proc_lock_off(void)
{
	pthread_mutex_unlock(&gTCCDxBProc.lockDxBProc);
}

/****************************************************************************
DEFINITION OF MESSAGE
****************************************************************************/
void tcc_dxb_proc_msg_init(void)
{
	gTCCDxBProc.pDxBProcMsg = new TccMessage(100);
}

void tcc_dxb_proc_msg_deinit(void)
{
	delete gTCCDxBProc.pDxBProcMsg;
}

int tcc_dxb_proc_msg_send(void *pmsg)
{
	return gTCCDxBProc.pDxBProcMsg->TccMessagePut((void *)pmsg);
}

void *tcc_dxb_proc_msg_get(void)
{
	return (void *)gTCCDxBProc.pDxBProcMsg->TccMessageGet();
}

int tcc_dxb_proc_msg_get_count(void)
{
	return gTCCDxBProc.pDxBProcMsg->TccMessageGetCount();
}

int tcc_dxb_proc_msg_clear(void)
{
	unsigned int i, uielement;
	ST_DXBPROC_MSG *p;
	uielement = tcc_dxb_proc_msg_get_count();
	for (i = 0;i < uielement;i++) {
		p = (ST_DXBPROC_MSG *)tcc_dxb_proc_msg_get();
		if (p)
		{
			if (p->arg) {
				if(p->cmd == DXBPROC_MSG_SET) {
					unsigned int *arg = (unsigned int*)p->arg;
					tcc_mw_free(__FUNCTION__, __LINE__, (void*)(arg[2]));
					tcc_mw_free(__FUNCTION__, __LINE__, (void*)(arg[3]));
					tcc_mw_free(__FUNCTION__, __LINE__, (void*)(arg[5]));
					tcc_mw_free(__FUNCTION__, __LINE__, (void*)(arg[6]));
					tcc_mw_free(__FUNCTION__, __LINE__, (void*)(arg[8]));
					tcc_mw_free(__FUNCTION__, __LINE__, (void*)(arg[9]));
					tcc_mw_free(__FUNCTION__, __LINE__, (void*)(arg[11]));
					tcc_mw_free(__FUNCTION__, __LINE__, (void*)(arg[12]));
				}

				tcc_mw_free(__FUNCTION__, __LINE__, p->arg);
			}

			tcc_mw_free(__FUNCTION__, __LINE__, p);
		}
	}

	return 0;
}

/****************************************************************************
DEFINITION OF LOCAL FUNCTION
****************************************************************************/
void tcc_dxb_proc_scanrequest_init (void)
{
	gScanRequestCount = 0;
}
/* this function should be accessed in atomic way */
int tcc_dxb_proc_scanrequest_add(void)
{
	gScanRequestCount++;
	return gScanRequestCount;
}
/* this function should be accessed in atomic way */
int tcc_dxb_proc_scanrequest_remove(void)
{
	if (gScanRequestCount > 0)
		gScanRequestCount--;
	return gScanRequestCount;
}
int tcc_dxb_proc_scanrequest_get(void)
{
	return gScanRequestCount;
}

/**
 * @brief
 *
 * @param
 *
 * @return
 *     0 : TCCDXBPROC_SUCCESS
 *    -1 : TCCDXBPROC_ERROR
 *    -2 : TCCDXBPROC_ERROR_INVALID_STATE
 *    -3 : TCCDXBPROC_ERROR_INVALID_PARAM
 *    -4 : tcc_manager_audio_stop(1) error
 *    -5 : tcc_manager_audio_stop(0) error
 *    -6 : tcc_manager_video_stop(1) error
 *    -7 : tcc_manager_video_stop(0) error
 *    -8 : tcc_manager_demux_av_stop(1, 1, 1, 1) error
 *    -9 : tcc_manager_video_set_proprietarydata error
 *    -10 : tcc_manager_demux_subtitle_stop error
 *    -11 : tcc_manager_video_deinitSurface error
 */
static int tcc_dxb_proc_state_change_to_load(int arg)
{
	int	timeout;
	int err = TCCDXBPROC_SUCCESS;

	if(TCCDxBProc_GetMainState() == TCCDXBPROC_STATE_LOAD)
	{
		return TCCDXBPROC_ERROR_INVALID_STATE;
	}
    
	if(TCCDxBProc_GetMainState() == TCCDXBPROC_STATE_SCAN)
	{
		if (tcc_dxb_proc_scanrequest_get())
		{
			tcc_manager_tuner_scan_cancel();
			tcc_manager_demux_scan_cancel();
		}
		TCCDxBProc_SetState(TCCDXBPROC_STATE_LOAD);
	}
	else if((TCCDxBProc_GetMainState() == TCCDXBPROC_STATE_AIRPLAY) && (TCCDxBProc_GetSubState() != TCCDXBPROC_AIRPLAY_MSG))
	{

		if(TCCDxBProc_GetSubState() & TCCDXBPROC_AIRPLAY_INFORMAL_SCAN)
		{
			tcc_manager_tuner_scan_cancel();
			tcc_manager_demux_scan_cancel();
		}

		//All components should be reset
		//TCCDxBProc_SetState(TCCDXBPROC_STATE_LOAD);    // Noah, 20180906, Commented out for IM478A-85, 'stop' API does NOT execute due to STATE_LOAD.


#if !defined(STOP_TIME_CHECK)
		err = tcc_manager_audio_stop(1);
		if (err != 0 /*DxB_ERR_OK*/)
		{
			ALOGE("[%s] tcc_manager_audio_stop(1) Error(%d)", __func__, err);
			return -4;
		}
		
		err = tcc_manager_audio_stop(0);
		if (err != 0 /*DxB_ERR_OK*/)
		{
			ALOGE("[%s] tcc_manager_audio_stop(0) Error(%d)", __func__, err);
			return -5;
		}

#if 1 //move early,m-1
		err = tcc_manager_video_deinitSurface();
		if (err != 0 /*DxB_ERR_OK*/)
		{
			ALOGE("[%s] tcc_manager_video_deinitSurface Error(%d)", __func__, err);
			return -11;
		}
#endif	

		err = tcc_manager_video_stop(1);
		if (err != 0 /*DxB_ERR_OK*/)
		{
			ALOGE("[%s] tcc_manager_video_stop(1) Error(%d)", __func__, err);
			return -6;
		}

		err = tcc_manager_video_stop(0);
		if (err != 0 /*DxB_ERR_OK*/)
		{
			ALOGE("[%s] tcc_manager_video_stop(0) Error(%d)", __func__, err);
			return -7;
		}

		err = tcc_manager_demux_av_stop(1, 1, 1, 1);
		if (err != 0)
		{
			ALOGE("[%s] tcc_manager_demux_av_stop Error(%d)", __func__, err);
			return -8;
		}

		if(arg == -1)
		{
			err = tcc_manager_video_set_proprietarydata(arg);
			if (err != 0 /*DxB_ERR_OK*/)
			{
				ALOGE("[%s] tcc_manager_video_set_proprietarydata Error(%d)", __func__, err);
				return -9;
			}
		}

		err = tcc_manager_demux_subtitle_stop(1);
		if (err != 0)
		{
			ALOGE("[%s] tcc_manager_demux_subtitle_stop(1) Error(%d)", __func__, err);
			return -10;
		}

		if(!TCC_Isdbt_IsAudioOnlyMode())
		{
			subtitle_display_wmixer_close();
		}
#if 0 //m-2
		err = tcc_manager_video_deinitSurface();
		if (err != 0 /*DxB_ERR_OK*/)
		{
			ALOGE("[%s] tcc_manager_video_deinitSurface Error(%d)", __func__, err);
			return -11;
		}
#endif		
#else
		long long stop_start = 0;
		long long stop_end = 0;

		stop_start = StopTimeCheck();
		tcc_manager_audio_stop(1);
		stop_end = StopTimeCheck();
		printf("\033[32m Noah / [%s] tcc_manager_audio_stop(1)(%04d)\033[m\n", __func__, (int)(stop_end-stop_start));

		stop_start = stop_end;
		tcc_manager_audio_stop(0);
		stop_end = StopTimeCheck();
		printf("\033[32m Noah / [%s] tcc_manager_audio_stop(0)(%04d)\033[m\n", __func__, (int)(stop_end-stop_start));

		stop_start = stop_end;
		tcc_manager_video_stop(1);
		stop_end = StopTimeCheck();
		printf("\033[32m Noah / [%s] tcc_manager_video_stop(1)(%04d)\033[m\n", __func__, (int)(stop_end-stop_start));

		stop_start = stop_end;
		tcc_manager_video_stop(0);
		stop_end = StopTimeCheck();
		printf("\033[32m Noah / [%s] tcc_manager_video_stop(0)(%04d)\033[m\n", __func__, (int)(stop_end-stop_start));

		stop_start = stop_end;
		tcc_manager_demux_av_stop(1, 1, 1, 1);
		stop_end = StopTimeCheck();
		printf("\033[32m Noah / [%s] tcc_manager_demux_av_stop(1, 1, 1, 1)(%04d)\033[m\n", __func__, (int)(stop_end-stop_start));

		stop_start = stop_end;
		if(arg == -1)
		{
			tcc_manager_video_set_proprietarydata(arg);
		}
		tcc_manager_demux_subtitle_stop(1);
		stop_end = StopTimeCheck();
		printf("\033[32m Noah / [%s] tcc_manager_demux_subtitle_stop(1)(%04d)\033[m\n", __func__, (int)(stop_end-stop_start));

		stop_start = stop_end;
		if(!TCC_Isdbt_IsAudioOnlyMode())
		{
			subtitle_display_wmixer_close();
		}
		stop_end = StopTimeCheck();
		printf("\033[32m Noah / [%s] subtitle_display_wmixer_close(%04d)\033[m\n", __func__, (int)(stop_end-stop_start));

		stop_start = stop_end;
		tcc_manager_video_deinitSurface();
		stop_end = StopTimeCheck();
		printf("\033[32m Noah / [%s] tcc_manager_video_deinitSurface(%04d)\033[m\n", __func__, (int)(stop_end-stop_start));
		stop_start = stop_end;
#endif
		
	}

	return TCCDXBPROC_SUCCESS;
}

static int tcc_dxb_proc_set(unsigned int *arg)
{
	unsigned int uiChannel, uiAudioTotalCount, uiVideoTotalCount, *puiAudioPID, *puiVideoPID;
	unsigned int *puiAudioStreamType, *puiVideoStreamType;
	unsigned int uiSubAudioTotalCount, uiSubVideoTotalCount, *puiAudioSubPID, *puiVideoSubPID;
	unsigned int *puiAudioSubStreamType, *puiVideoSubStreamType;
	unsigned int uiPcrPID, uiPcrSubPID;
	unsigned int uiCAECMPID, uiACECMPID;
	unsigned int uiServiceID, uiNetworkID, uiPmtPID, uiStPID, uiSiPID;
	unsigned int uiServiceSubID, uiPmtSubPID, uiStSubPID, uiSiSubPID;
	unsigned int subtitle_pid, superimpose_pid;
	unsigned int uiAudioIndex, uiAudioMainIndex, uiAudioSubIndex, uiAudioMode, uiServiceType;
	unsigned int uiVideoIndex, uiVideoMainIndex, uiVideoSubIndex;
	unsigned int uiSegType, uiRaw_w, uiRaw_h, uiView_w, uiView_h;
	int	ch_index;
	int	dual_mode;
	int i;

	uiChannel = arg[0];
	uiAudioTotalCount = arg[1];
	puiAudioPID = (unsigned int*)arg[2];
	puiAudioStreamType = (unsigned int*)arg[3];
	uiVideoTotalCount = arg[4];
	puiVideoPID = (unsigned int*)arg[5];
	puiVideoStreamType = (unsigned int*)arg[6];
	uiSubAudioTotalCount = arg[7];
	puiAudioSubPID = (unsigned int*)arg[8];
	puiAudioSubStreamType = (unsigned int*)arg[9];
	uiSubVideoTotalCount = arg[10];
	puiVideoSubPID = (unsigned int*)arg[11];
	puiVideoSubStreamType =(unsigned int*)arg[12];
	uiPcrPID = arg[13];
	uiPcrSubPID = arg[14];
	uiStPID = arg[15];
	uiStSubPID = arg[16];
	uiSiPID = arg[17];
	uiSiSubPID = arg[18];
	uiServiceID = arg[19];
	uiServiceSubID = arg[20];
	uiPmtPID = arg[21];
	uiPmtSubPID = arg[22];
	uiCAECMPID = arg[23];
	uiACECMPID = arg[24];
	uiAudioIndex = arg[25];
	uiVideoIndex = arg[26];
	uiAudioMode = arg[27];
	uiServiceType = arg[28];
	uiNetworkID = arg[29];
	uiRaw_w = arg[30];
	uiRaw_h = arg[31];
	uiView_w = arg[32];
	uiView_h = arg[33];
	ch_index = arg[34];
	uiAudioMainIndex = arg[35];
	uiAudioSubIndex = arg[36];
	uiVideoMainIndex = arg[37];
	uiVideoSubIndex = arg[38];

	ALOGE("%s %d uiPmtPID = %d,  uiPmtSubPID = %d \n", __func__, __LINE__, uiPmtPID, uiPmtSubPID);

	ALOGD("%s:uiChannel[%d]Audio[0x%X]Video[0x%X]AudioType[0x%X]VideoType[0x%X]PCR[0x%X]",
	    __func__, uiChannel,
	    puiAudioPID[uiAudioMainIndex], puiVideoPID[uiVideoMainIndex],
	    puiAudioStreamType[uiAudioMainIndex], puiVideoStreamType[uiVideoMainIndex],
	    uiPcrPID);

	gStopRequest = false;

// IM69A-30, Noah, 20190124
//	tcc_dxb_proc_lock_on();

    /*
        Noah, 2018-10-12, IM692A-15 / TVK does not recieve at all after 632 -> 631 of TVK.

            Precondition
                ISDBT_SINFO_CHECK_SERVICEID is enabled.

            Issue
                1. tcc_manager_audio_serviceID_disable_output(0) and tcc_manager_video_serviceID_disable_display(0) are called.
                    -> Because 632 is broadcasting in suspension, so, this is no problem.
                2. setChannel(_withChNum) or searchAndSetChannel is called to change to weak signal(1seg OK, Fseg No) channel.
                3. A/V is NOT output.
                    -> NG

            Root Cause
                The omx_alsasink_component_Private->audioskipOutput and pPrivateData->bfSkipRendering of previous channel are used in current channel.
                Refer to the following steps.

                1. In step 1 of the Issue above, omx_alsasink_component_Private->audioskipOutput and pPrivateData->bfSkipRendering are set to TRUE
                    because tcc_manager_audio_serviceID_disable_output(0) and tcc_manager_video_serviceID_disable_display(0) are called.
                2. tcc_manager_audio_serviceID_disable_output(1) and tcc_manager_video_serviceID_disable_display(1) have to be called for playing A/V.
                    To do so, DxB has to get PAT and then tcc_manager_demux_serviceInfo_comparison has to be called.
                    But, DxB can NOT get APT due to weak signal.

            Solution
                tcc_manager_audio_serviceID_disable_output(1) & tcc_manager_video_serviceID_disable_display(1) are called forcedly once as below.

                I think the following conditons should be default values. And '1' value of previous channel should not be used in current channel.
                    - omx_alsasink_component_Private->audioskipOutput == 0
                    - pPrivateData->bfSkipRendering == 0
                After that -> PAT -> If service ID is the same, A/V will be shown continually. Otherwise A/V will be blocked.

    */
    tcc_manager_demux_init_av_skip_flag();

	if (TCCDxBProc_GetMainState() != TCCDXBPROC_STATE_AIRPLAY)
	{
		// Noah / 20170918 / Added the following if statement for IM478A-71 (Not received TS at all)
		if(TCCDxBProc_GetMainState() == TCCDXBPROC_STATE_LOAD)
		{
			TCCDxBProc_SetState(TCCDXBPROC_STATE_AIRPLAY);
		}
		else
		{
// IM69A-30, Noah, 20190124
//			tcc_dxb_proc_lock_off();
			return -1;
		}
	}

	if (ch_index == 0)
	{
		 dual_mode = TCCDXBPROC_MAIN;
	}
	else
	{
		dual_mode = TCCDXBPROC_SUB;
	}

	iDual_mode = dual_mode;

	//for supporting the service which has only data(no Audio/Video).
	if(!TCC_Isdbt_IsSupportDataService())
	{
		if (((dual_mode == TCCDXBPROC_MAIN) && (puiAudioPID[uiAudioMainIndex] > 0x1FFF) && (puiVideoPID[uiVideoMainIndex] > 0x1FFF))
			|| ((dual_mode == TCCDXBPROC_SUB) && (puiAudioSubPID[uiAudioSubIndex] > 0x1FFF) && (puiVideoSubPID[uiVideoSubIndex] > 0x1FFF)))
		{
			if(TCCDxBProc_GetSubState() == TCCDXBPROC_AIRPLAY_MSG)
			{
				TCCDxBProc_SetState(TCCDXBPROC_STATE_LOAD);
			}

// IM69A-30, Noah, 20190124
//			tcc_dxb_proc_lock_off();
			return -1;
		}
	}

	tcc_dxb_proc_state_change_to_load(-1);

/*
	if(gStopRequest == true)
	{
		tcc_dxb_proc_lock_off();
		return -1;
	}
*/

	gTCCDxBProc.uiChannel = uiChannel;
	gTCCDxBProc.uiAudioTotalCount = uiAudioTotalCount;
	gTCCDxBProc.uiSubAudioTotalCount = uiSubAudioTotalCount;
	for(i=0; i<TCCDXBPROC_AUDIO_MAX_NUM; i++)
	{
		gTCCDxBProc.puiAudioPID[i] = puiAudioPID[i];
		gTCCDxBProc.puiAudioStreamType[i] = puiAudioStreamType[i];
		gTCCDxBProc.puiAudioSubPID[i] = puiAudioSubPID[i];
		gTCCDxBProc.puiAudioSubStreamType[i] = puiAudioSubStreamType[i];
	}
	gTCCDxBProc.uiVideoTotalCount = uiVideoTotalCount;
	gTCCDxBProc.uiSubVideoTotalCount = uiSubVideoTotalCount;
	for(i=0; i<TCCDXBPROC_VIDEO_MAX_NUM; i++)
	{
		gTCCDxBProc.puiVideoPID[i] = puiVideoPID[i];
		gTCCDxBProc.puiVideoStreamType[i] = puiVideoStreamType[i];
		gTCCDxBProc.puiVideoSubPID[i] = puiVideoSubPID[i];
		gTCCDxBProc.puiVideoSubStreamType[i] = puiVideoSubStreamType[i];
	}
	gTCCDxBProc.uiPcrPID = uiPcrPID;
	gTCCDxBProc.uiPcrSubPID = uiPcrSubPID;
	gTCCDxBProc.uiStPID = uiStPID;
	gTCCDxBProc.uiStSubPID = uiStSubPID;
	gTCCDxBProc.uiSiPID = uiSiPID;
	gTCCDxBProc.uiSiSubPID = uiSiSubPID;
	gTCCDxBProc.uiServiceID = uiServiceID;
	gTCCDxBProc.uiServiceSubID = uiServiceSubID;
	gTCCDxBProc.uiPmtPID = uiPmtPID;
	gTCCDxBProc.uiPmtSubPID = uiPmtSubPID;
	gTCCDxBProc.uiCAECMPID = uiCAECMPID;
	gTCCDxBProc.uiACECMPID = uiACECMPID;
	gTCCDxBProc.uiAudioIndex = uiAudioIndex;
	gTCCDxBProc.uiAudioMainIndex = uiAudioMainIndex;
	gTCCDxBProc.uiAudioSubIndex = uiAudioSubIndex;
	gTCCDxBProc.uiVideoIndex = uiVideoIndex;
	gTCCDxBProc.uiVideoMainIndex = uiVideoMainIndex;
	gTCCDxBProc.uiVideoSubIndex = uiVideoSubIndex;
	gTCCDxBProc.uiAudioMode = uiAudioMode;
	gTCCDxBProc.uiServiceType = uiServiceType;
	gTCCDxBProc.uiNetworkID = uiNetworkID;
	gTCCDxBProc.uiRaw_w = uiRaw_w;
	gTCCDxBProc.uiRaw_h = uiRaw_h;
	gTCCDxBProc.uiView_w = uiView_w;
	gTCCDxBProc.uiView_h = uiView_h;

	if (TCC_Isdbt_IsSupportSpecialService())
	{
		if (uiServiceType != SERVICE_TYPE_TEMP_VIDEO)
		{
			/* delete all information of special service if the selected service is regular */
			tcc_manager_demux_specialservice_DeleteDB();
		}
	}

	tcc_manager_demux_set_state(E_DEMUX_STATE_AIRPLAY);

	if (dual_mode == TCCDXBPROC_MAIN)
	{
		tcc_manager_demux_set_serviceID(uiServiceID);
		tcc_manager_demux_set_playing_serviceID(uiServiceID);
		tcc_manager_demux_set_first_playing_serviceID(uiServiceID);
	}
	else
	{
		tcc_manager_demux_set_serviceID(uiServiceSubID);
		tcc_manager_demux_set_playing_serviceID(uiServiceSubID);
		tcc_manager_demux_set_first_playing_serviceID(uiServiceSubID);
	}
	tcc_manager_demux_set_tunerinfo(uiChannel, 0, gTCCDxBProc.uiCountrycode);
	tcc_db_create_epg(uiChannel, uiNetworkID);
	tcc_dxb_timecheck_set("switch_channel", "tuner_lock", TIMECHECK_START);
	tcc_manager_tuner_set_channel(uiChannel);
	tcc_dxb_timecheck_set("switch_channel", "tuner_lock", TIMECHECK_STOP);
	tcc_dxb_timecheck_set("switch_channel", "NIT_set", TIMECHECK_START);
	tcc_dxb_timecheck_set("switch_channel", "PMT_set", TIMECHECK_START);
	tcc_dxb_timecheck_set("switch_channel", "FVideoCB", TIMECHECK_START);
	tcc_dxb_timecheck_set("switch_channel", "AudioCB", TIMECHECK_START);

/*
	if(gStopRequest == true)
	{
		tcc_dxb_proc_lock_off();
		return -1;
	}
*/

	if (TCC_Isdbt_IsSupportBrazil()){
		tcc_manager_demux_set_parentlock(1);
	}

	ALOGE("%s %d brazil= %d , japan= %d \n", __func__, __LINE__, TCC_Isdbt_IsSupportBrazil(), TCC_Isdbt_IsSupportJapan());

	if (TCC_Isdbt_IsSupportBrazil())
	{
		tcc_manager_video_issupport_country(0, TCCDXBPROC_BRAZIL);
		tcc_manager_audio_issupport_country(0, TCCDXBPROC_BRAZIL);
	}
	else if (TCC_Isdbt_IsSupportJapan())
	{
		tcc_manager_video_issupport_country(0, TCCDXBPROC_JAPAN);
		tcc_manager_audio_issupport_country(0, TCCDXBPROC_JAPAN);
	}

	ResetSIVersionNumber();

	tcc_manager_demux_cas_init();		//This should be called after changing RF frequency.

	tcc_manager_video_set_proprietarydata(gTCCDxBProc.uiChannel);

	TCCDxBProc_SetState(TCCDXBPROC_STATE_AIRPLAY);
	if (dual_mode == TCCDXBPROC_MAIN)
	{
		 TCCDxBProc_SetSubState(TCCDXBPROC_AIRPLAY_MAIN);
 	}
	else
	{
		 TCCDxBProc_SetSubState(TCCDXBPROC_AIRPLAY_SUB);
 	}

	/*----- logo -----*/
	tcc_manager_demux_logo_init();
	tcc_manager_demux_logo_get_infoDB(uiChannel, gTCCDxBProc.uiCountrycode, uiNetworkID, uiServiceID, uiServiceSubID);

	/*----- channel name -----*/
#if 0    // Original

	if (tcc_manager_demux_is_skip_sdt_in_scan()) {
		tcc_manager_demux_channelname_init();
		tcc_manager_demux_channelname_get_infoDB(uiChannel, gTCCDxBProc.uiCountrycode, uiNetworkID, uiServiceID, uiServiceSubID);
	}

#else    // Noah / 20180131 / IM478A-77 (relay station search in the background using 2TS) / Update 'channelName' field of DB forcibly

	tcc_manager_demux_channelname_init();
	tcc_manager_demux_channelname_get_infoDB(uiChannel, gTCCDxBProc.uiCountrycode, uiNetworkID, uiServiceID, uiServiceSubID);

#endif

	tcc_manager_video_initSurface(0);

	tcc_manager_demux_av_play(puiAudioPID[uiAudioMainIndex], puiVideoPID[uiVideoMainIndex], \
							 puiAudioStreamType[uiAudioMainIndex], puiVideoStreamType[uiVideoMainIndex], \
							 puiAudioSubPID[uiAudioSubIndex], puiVideoSubPID[uiVideoSubIndex], \
							 puiAudioSubStreamType[uiAudioSubIndex], puiVideoSubStreamType[uiVideoSubIndex], \
							 uiPcrPID, uiPcrSubPID, uiPmtPID, uiPmtSubPID, uiCAECMPID, uiACECMPID, dual_mode);

/*
	if(gStopRequest == true)
	{
		tcc_dxb_proc_lock_off();
		return -1;
	}
*/

    if (tcc_manager_audio_start(0, puiAudioStreamType[uiAudioMainIndex]) < 0)
    {
            puiAudioStreamType[uiAudioMainIndex] = 0xFFFF;
            puiAudioPID[uiAudioMainIndex] = 0xFFFF;
    }

    if (tcc_manager_audio_start(1, puiAudioSubStreamType[uiAudioSubIndex]) < 0)
    {
            puiAudioSubStreamType[uiAudioSubIndex] = 0xFFFF;
            puiAudioSubPID[uiAudioSubIndex] = 0xFFFF;
    }

	if(TCC_Isdbt_IsSupportAlsaClose())
	{
		/*ALSA close*/
		tcc_manager_audio_alsa_close(1, 1);
		tcc_manager_audio_alsa_close(0, 1);
		tcc_manager_audio_alsa_close_flag(1);
	}
	
	if(!TCC_Isdbt_IsAudioOnlyMode())
	{
		if(tcc_dxb_proc_get_videoonoff_value() == 1)
		{
		    if (tcc_manager_video_start(0, puiVideoStreamType[uiVideoMainIndex]) < 0)
		    {
		            puiVideoStreamType[uiVideoMainIndex] = 0xFFFF;
		            puiVideoPID[uiVideoMainIndex] = 0xFFFF;
		    }
		    if (tcc_manager_video_start(1, puiVideoSubStreamType[uiVideoSubIndex]) < 0)
		    {
		            puiVideoSubStreamType[uiVideoSubIndex] = 0xFFFF;
		            puiVideoSubPID[uiVideoSubIndex] = 0xFFFF;
		    }
		}
	}

	tcc_manager_audio_set_proprietarydata(gTCCDxBProc.uiChannel, gTCCDxBProc.uiServiceID, gTCCDxBProc.uiServiceSubID, dual_mode, TCC_Isdbt_IsSupportPrimaryService());

	if(puiAudioPID[uiAudioMainIndex] != 0xFFFF 
		&& puiAudioSubPID[uiAudioSubIndex] != 0xFFFF
		&& puiAudioStreamType[uiAudioMainIndex] != 0xFFFF
		&& puiAudioSubStreamType[uiAudioSubIndex] != 0xFFFF
		&& gTCCDxBSeamless.iOnoff== 1)
	{
		tcc_manager_audio_setSeamlessSwitchCompensation(gTCCDxBSeamless.iOnoff, gTCCDxBSeamless.iInterval, gTCCDxBSeamless.iStrength,
			gTCCDxBSeamless.iNtimes, gTCCDxBSeamless.iRange, gTCCDxBSeamless.iGapadjust, gTCCDxBSeamless.iGapadjust2, gTCCDxBSeamless.iMultiplier);
	}

	if (dual_mode == TCCDXBPROC_MAIN)
	{
		subtitle_pid = uiStPID;
		superimpose_pid = uiSiPID;
		uiSegType = 13;
	}
	else
	{
		subtitle_pid = uiStSubPID;
		superimpose_pid = uiSiSubPID;
		uiSegType = 1;
	}

#if defined(HAVE_LINUX_PLATFORM)
	if(!TCC_Isdbt_IsAudioOnlyMode())
	{
		subtitle_display_wmixer_open();
	}
#endif

	if (uiSegType == 13)
	{
	    subtitle_system_set_stc_index(0);
	}
	else
	{
	    subtitle_system_set_stc_index(1);
	}

 	tcc_manager_demux_subtitle_play(subtitle_pid, superimpose_pid, uiSegType, gTCCDxBProc.uiCountrycode,
									uiRaw_w, uiRaw_h, uiView_w, uiView_h, 1);

	tcc_manager_audio_set_dualmono(0, uiAudioMode);

	tcc_manager_video_select_display_output(dual_mode);
	tcc_manager_audio_select_output(dual_mode, true);
	tcc_manager_audio_select_output(dual_mode?0:1, false);
	gTCCDxBProc.uiCurrDualDecodeMode = dual_mode;

	TCCDxBEvent_AVIndexUpdate(uiChannel, uiNetworkID, uiServiceID, uiServiceSubID, uiAudioIndex, uiVideoIndex);

// IM69A-30, Noah, 20190124
//	tcc_dxb_proc_lock_off();

	return 0;
}

static int tcc_dxb_proc_scanandset(unsigned int *arg)
{
	int iCountryCode, iChannel, iTSID, iOptions;
	int iAudioIndex, iAudioMainIndex, iAudioSubIndex;
	int iVideoIndex, iVideoMainIndex, iVideoSubIndex;
	int iAudioMode;
	int iRaw_w, iRaw_h, iView_w, iView_h;
	int ch_index;
	int	dual_mode;

	int iMainRowID=0, iSubRowID=0;
	int iAudioTotalCount=0, iVideoTotalCount=0, iStPID=0xFFFF, iSiPID=0xFFFF, iServiceType=0, iServiceID=-1;
	int iSubAudioTotalCount=0, iSubVideoTotalCount=0, iStSubPID=-1, iSiSubPID=-1, iServiceSubType=0, iServiceSubID=-1;
	int iPCRPID=0, iPMTPID=0, iCAECMPID=0, iACECMPID=0;
	int iPCRSubPID=-1, iPMTSubPID=0, iCAECMSubPID=0, iACECMSubPID=0;
	int iNetworkID=-1, iThreeDigitNumber=-1;

	int piAudioPID[TCCDXBPROC_AUDIO_MAX_NUM] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
	int piAudioStreamType[TCCDXBPROC_AUDIO_MAX_NUM] = {0, 0, 0, 0};
	int piAudioComponentTag[TCCDXBPROC_AUDIO_MAX_NUM];
	int piAudioSubPID[TCCDXBPROC_AUDIO_MAX_NUM] = {-1, -1, -1, -1};
	int piAudioSubStreamType[TCCDXBPROC_AUDIO_MAX_NUM] = {-1, -1, -1, -1};
	int piAudioSubComponentTag[TCCDXBPROC_AUDIO_MAX_NUM];
	int piVideoPID[TCCDXBPROC_VIDEO_MAX_NUM] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
	int piVideoStreamType[TCCDXBPROC_VIDEO_MAX_NUM] = {0, 0, 0, 0};
	int piVideoComponentTag[TCCDXBPROC_VIDEO_MAX_NUM];
	int piVideoSubPID[TCCDXBPROC_VIDEO_MAX_NUM] = {-1, -1, -1, -1};
	int piVideoSubStreamType[TCCDXBPROC_VIDEO_MAX_NUM] = {-1, -1, -1, -1};
	int piVideoSubComponentTag[TCCDXBPROC_VIDEO_MAX_NUM];
	unsigned int subtitle_pid, superimpose_pid, seg_type;

	int i;
	int err;

	iCountryCode = arg[0];
	iChannel = arg[1];
	iTSID = arg[2];
	iOptions = arg[3];
	iAudioIndex = iAudioMainIndex = iAudioSubIndex = arg[4];
	iVideoIndex =  iVideoMainIndex = iVideoSubIndex = arg[5];
	iAudioMode = arg[6];
	iRaw_w = arg[7];
	iRaw_h = arg[8];
	iView_w = arg[9];
	iView_h = arg[10];
	ch_index = arg[11];

    g_bSearchAndSetChannel_InProgress = 1;
	
	tcc_dxb_proc_lock_on();

    /*
        Noah, 2018-10-12, IM692A-15 / TVK does not recieve at all after 632 -> 631 of TVK.

            Precondition
                ISDBT_SINFO_CHECK_SERVICEID is enabled.

            Issue
                1. tcc_manager_audio_serviceID_disable_output(0) and tcc_manager_video_serviceID_disable_display(0) are called.
                    -> Because 632 is broadcasting in suspension, so, this is no problem.
                2. setChannel(_withChNum) or searchAndSetChannel is called to change to weak signal(1seg OK, Fseg No) channel.
                3. A/V is NOT output.
                    -> NG

            Root Cause
                The omx_alsasink_component_Private->audioskipOutput and pPrivateData->bfSkipRendering of previous channel are used in current channel.
                Refer to the following steps.

                1. In step 1 of the Issue above, omx_alsasink_component_Private->audioskipOutput and pPrivateData->bfSkipRendering are set to TRUE
                    because tcc_manager_audio_serviceID_disable_output(0) and tcc_manager_video_serviceID_disable_display(0) are called.
                2. tcc_manager_audio_serviceID_disable_output(1) and tcc_manager_video_serviceID_disable_display(1) have to be called for playing A/V.
                    To do so, DxB has to get PAT and then tcc_manager_demux_serviceInfo_comparison has to be called.
                    But, DxB can NOT get APT due to weak signal.

            Solution
                tcc_manager_audio_serviceID_disable_output(1) & tcc_manager_video_serviceID_disable_display(1) are called forcedly once as below.

                I think the following conditons should be default values. And '1' value of previous channel should not be used in current channel.
                    - omx_alsasink_component_Private->audioskipOutput == 0
                    - pPrivateData->bfSkipRendering == 0
                After that -> PAT -> If service ID is the same, A/V will be shown continually. Otherwise A/V will be blocked.

    */
    tcc_manager_demux_init_av_skip_flag();

// Noah, 20190219, IM692A-37
//	gStopRequest = false;

   	if (TCCDxBProc_GetMainState() != TCCDXBPROC_STATE_AIRPLAY)
	{
		tcc_dxb_proc_lock_off();
	    g_bSearchAndSetChannel_InProgress = 0;
		return -1;
	}

	tcc_dxb_proc_state_change_to_load(-1);

/* Noah, 20190219, IM692A-37
	if(gStopRequest == true)
	{
		tcc_dxb_proc_lock_off();
		return -1;
	}
*/

	// scan
	ALOGD("%s:Start Scan iCountryCode[%d] iChannel[%d]", __func__, iCountryCode, iChannel);

	TCCDxBProc_SetSubState(TCCDXBPROC_AIRPLAY_INFORMAL_SCAN);
	tcc_manager_demux_set_state(E_DEMUX_STATE_INFORMAL_SCAN);

	if (TCC_Isdbt_IsSupportBrazil())
	{
		tcc_manager_demux_set_parentlock(0);
	}

	ResetSIVersionNumber();

	tcc_dxb_proc_lock_off();

	err = tcc_manager_tuner_scan(INFORMAL_SCAN, iCountryCode, iTSID, iChannel, iOptions);

	tcc_dxb_proc_lock_on();

	// Noah / 20171017 / Added the following if statement for IM478A-74 (There is a case that the output of a/v is very slow)
	// The if statement comes from tcc_dxb_proc_scan function.
	if (err == 1)
	{
		//when tcc_manager_tuner_scan() returned scan-cancel
		//there can be a case that giTunerScanStop is false but giDemuxScanStop is true.
		tcc_manager_tuner_scanflag_init();
		tcc_manager_demux_scanflag_init();
	}

/* Noah, 20190219, IM692A-37	
	if(gStopRequest == true)
	{
		tcc_dxb_proc_lock_off();
		return -1;
	}
*/

	TCCDxBProc_ClearSubState(TCCDXBPROC_AIRPLAY_INFORMAL_SCAN);

	//get channel
	if (err == 0)
	{
		if(TCC_Isdbt_IsSupportPrimaryService())
		{
			if(TCC_Isdbt_IsSupportProfileA())
			{
				iAudioTotalCount = TCCDXBPROC_AUDIO_MAX_NUM;
				iVideoTotalCount = TCCDXBPROC_VIDEO_MAX_NUM;
				//int piAudioPID[TCCDXBPROC_AUDIO_MAX_NUM] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};

				// Noah / 20170522 / Added for IM478A-31 (Primary Service)
				tcc_db_get_primaryService(iChannel, TCCDXBPROC_SERVICE_TYPE_12SEG, \
										&iMainRowID, &iAudioTotalCount, piAudioPID, &iVideoTotalCount, piVideoPID, &iStPID, &iSiPID, \
										&iServiceType, &iServiceID, &iPMTPID, &iCAECMPID, &iACECMPID, &iNetworkID, \
										piAudioStreamType, piAudioComponentTag, piVideoStreamType, piVideoComponentTag, &iPCRPID, &iThreeDigitNumber, \
										&iAudioIndex, &iAudioMainIndex, &iVideoIndex, &iVideoMainIndex);
				if(iMainRowID == 0)    // If fail to get Primary Service, existing method is used.
				{
					ALOGD("[%s] fail to get Primary Service of 12 seg, so existing method is used.\n", __func__);

					tcc_db_get_representative_channel(iChannel, TCCDXBPROC_SERVICE_TYPE_12SEG, \
													&iMainRowID, &iAudioTotalCount, piAudioPID, &iVideoTotalCount, piVideoPID, &iStPID, &iSiPID, \
													&iServiceType, &iServiceID, &iPMTPID, &iCAECMPID, &iACECMPID, &iNetworkID, \
													piAudioStreamType, piAudioComponentTag, piVideoStreamType, piVideoComponentTag, &iPCRPID, &iThreeDigitNumber, \
													&iAudioIndex, &iAudioMainIndex, &iVideoIndex, &iVideoMainIndex);
				}
			}

			if(TCC_Isdbt_IsSupportProfileC())
			{
				iSubAudioTotalCount = TCCDXBPROC_AUDIO_MAX_NUM;
				iSubVideoTotalCount = TCCDXBPROC_VIDEO_MAX_NUM;

				// Noah / 20170522 / Added for IM478A-31 (Primary Service)
				tcc_db_get_primaryService(iChannel, TCCDXBPROC_SERVICE_TYPE_1SEG, \
										&iSubRowID, &iSubAudioTotalCount, piAudioSubPID, &iSubVideoTotalCount, piVideoSubPID, &iStSubPID, &iSiSubPID, \
										&iServiceSubType, &iServiceSubID, &iPMTSubPID, &iCAECMSubPID, &iACECMSubPID, &iNetworkID, \
										piAudioSubStreamType, piAudioSubComponentTag, piVideoSubStreamType, piVideoSubComponentTag, &iPCRSubPID, &iThreeDigitNumber, \
										&iAudioIndex, &iAudioSubIndex, &iVideoIndex, &iVideoSubIndex);
				if(iSubRowID == 0)    // If fail to get Primary Service, existing method is used.
				{
					ALOGD("[%s] fail to get Primary Service of 1 seg, so existing method is used.\n", __func__);

					tcc_db_get_representative_channel(iChannel, TCCDXBPROC_SERVICE_TYPE_1SEG, \
													&iSubRowID, &iSubAudioTotalCount, piAudioSubPID, &iSubVideoTotalCount, piVideoSubPID, &iStSubPID, &iSiSubPID, \
													&iServiceSubType, &iServiceSubID, &iPMTSubPID, &iCAECMSubPID, &iACECMSubPID, &iNetworkID, \
													piAudioSubStreamType, piAudioSubComponentTag, piVideoSubStreamType, piVideoSubComponentTag, &iPCRSubPID, &iThreeDigitNumber, \
													&iAudioIndex, &iAudioSubIndex, &iVideoIndex, &iVideoSubIndex);
				}
			}
		}
		else
		{
			if(TCC_Isdbt_IsSupportProfileA())
			{
				iAudioTotalCount = TCCDXBPROC_AUDIO_MAX_NUM;
				iVideoTotalCount = TCCDXBPROC_VIDEO_MAX_NUM;
			//	int piAudioPID[TCCDXBPROC_AUDIO_MAX_NUM] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};

				tcc_db_get_representative_channel(iChannel, TCCDXBPROC_SERVICE_TYPE_12SEG, \
													&iMainRowID, &iAudioTotalCount, piAudioPID, &iVideoTotalCount, piVideoPID, &iStPID, &iSiPID, \
													&iServiceType, &iServiceID, &iPMTPID, &iCAECMPID, &iACECMPID, &iNetworkID, \
													piAudioStreamType, piAudioComponentTag, piVideoStreamType, piVideoComponentTag, &iPCRPID, &iThreeDigitNumber, \
													&iAudioIndex, &iAudioMainIndex, &iVideoIndex, &iVideoMainIndex);
			}

			if(TCC_Isdbt_IsSupportProfileC())
			{
				iSubAudioTotalCount = TCCDXBPROC_AUDIO_MAX_NUM;
				iSubVideoTotalCount = TCCDXBPROC_VIDEO_MAX_NUM;
				tcc_db_get_representative_channel(iChannel, TCCDXBPROC_SERVICE_TYPE_1SEG, \
													&iSubRowID, &iSubAudioTotalCount, piAudioSubPID, &iSubVideoTotalCount, piVideoSubPID, &iStSubPID, &iSiSubPID, \
													&iServiceSubType, &iServiceSubID, &iPMTSubPID, &iCAECMSubPID, &iACECMSubPID, &iNetworkID, \
													piAudioSubStreamType, piAudioSubComponentTag, piVideoSubStreamType, piVideoSubComponentTag, &iPCRSubPID, &iThreeDigitNumber, \
													&iAudioIndex, &iAudioSubIndex, &iVideoIndex, &iVideoSubIndex);
			}
		}
	}

	TCCDxBEvent_SearchAndSetChannel(err, iCountryCode, iChannel, iMainRowID, iSubRowID);

	// set
	if(iMainRowID > 0 || iSubRowID > 0)
	{
		ALOGD("%s:Set channel(%d, %d)", __func__, iMainRowID, iSubRowID);

/* Noah, 20190219, IM692A-37
		if(gStopRequest == true)
		{
			tcc_dxb_proc_lock_off();
			return -1;
		}
*/

		if (ch_index == 0)
		{
			 dual_mode = TCCDXBPROC_MAIN;
		}
		else
		{
			dual_mode = TCCDXBPROC_SUB;
		}

		iDual_mode = dual_mode;

		gTCCDxBProc.uiChannel = iChannel;
		gTCCDxBProc.uiAudioTotalCount = iAudioTotalCount;
		gTCCDxBProc.uiSubAudioTotalCount = iSubAudioTotalCount;
		for(i=0; i<TCCDXBPROC_AUDIO_MAX_NUM; i++)
		{
			gTCCDxBProc.puiAudioPID[i] = piAudioPID[i];
			gTCCDxBProc.puiAudioStreamType[i] = piAudioStreamType[i];
			gTCCDxBProc.puiAudioSubPID[i] = piAudioSubPID[i];
			gTCCDxBProc.puiAudioSubStreamType[i] = piAudioSubStreamType[i];
		}
		gTCCDxBProc.uiVideoTotalCount = iVideoTotalCount;
		gTCCDxBProc.uiSubVideoTotalCount = iSubVideoTotalCount;
		for(i=0; i<TCCDXBPROC_VIDEO_MAX_NUM; i++)
		{
			gTCCDxBProc.puiVideoPID[i] = piVideoPID[i];
			gTCCDxBProc.puiVideoStreamType[i] = piVideoStreamType[i];
			gTCCDxBProc.puiVideoSubPID[i] = piVideoSubPID[i];
			gTCCDxBProc.puiVideoSubStreamType[i] = piVideoSubStreamType[i];
		}
		gTCCDxBProc.uiPcrPID = iPCRPID;
		gTCCDxBProc.uiPcrSubPID = iPCRSubPID;
		gTCCDxBProc.uiStPID = iStPID;
		gTCCDxBProc.uiStSubPID = iStSubPID;
		gTCCDxBProc.uiSiPID = iSiPID;
		gTCCDxBProc.uiSiSubPID = iSiSubPID;
		gTCCDxBProc.uiServiceID = iServiceID;
		gTCCDxBProc.uiServiceSubID = iServiceSubID;
		gTCCDxBProc.uiPmtPID = iPMTPID;
		gTCCDxBProc.uiPmtSubPID = iPMTSubPID;
		gTCCDxBProc.uiCAECMPID = iCAECMPID;
		gTCCDxBProc.uiACECMPID = iACECMPID;
		gTCCDxBProc.uiAudioIndex = iAudioIndex;
		gTCCDxBProc.uiAudioMainIndex = iAudioMainIndex;
		gTCCDxBProc.uiAudioSubIndex = iAudioSubIndex;
		gTCCDxBProc.uiVideoIndex = iVideoIndex;
		gTCCDxBProc.uiVideoMainIndex = iVideoMainIndex;
		gTCCDxBProc.uiVideoSubIndex = iVideoSubIndex;
		gTCCDxBProc.uiAudioMode = iAudioMode;
		gTCCDxBProc.uiServiceType = iServiceType;
		gTCCDxBProc.uiServiceSubType = iServiceSubType;
		gTCCDxBProc.uiNetworkID = iNetworkID;
		gTCCDxBProc.uiRaw_w = iRaw_w;
		gTCCDxBProc.uiRaw_h = iRaw_h;
		gTCCDxBProc.uiView_w = iView_w;
		gTCCDxBProc.uiView_h = iView_h;

		if (TCC_Isdbt_IsSupportSpecialService())
		{
			if (iServiceType != SERVICE_TYPE_TEMP_VIDEO)
			{
				/* delete all information of special service if the selected service is regular */
				tcc_manager_demux_specialservice_DeleteDB();
			}
		}

		tcc_manager_demux_set_state(E_DEMUX_STATE_AIRPLAY);

		if (dual_mode == TCCDXBPROC_MAIN)
		{
			tcc_manager_demux_set_serviceID(iServiceID);
			tcc_manager_demux_set_playing_serviceID(iServiceID);
			tcc_manager_demux_set_first_playing_serviceID(iServiceID);
		}
		else
		{
			tcc_manager_demux_set_serviceID(iServiceSubID);
			tcc_manager_demux_set_playing_serviceID(iServiceSubID);
			tcc_manager_demux_set_first_playing_serviceID(iServiceSubID);
		}

		tcc_db_create_epg(iChannel, iNetworkID);

		tcc_manager_video_set_proprietarydata(gTCCDxBProc.uiChannel);

		TCCDxBProc_SetState(TCCDXBPROC_STATE_AIRPLAY);

		if (dual_mode == TCCDXBPROC_MAIN)
		{
			 TCCDxBProc_SetSubState(TCCDXBPROC_AIRPLAY_MAIN);
	 	}
		else
		{
			 TCCDxBProc_SetSubState(TCCDXBPROC_AIRPLAY_SUB);
 		}

		/*----- logo -----*/
		tcc_manager_demux_logo_init();
		tcc_manager_demux_logo_get_infoDB(iChannel, iCountryCode, iNetworkID, iServiceID, iServiceSubID);

		/*----- channel name -----*/
		if (tcc_manager_demux_is_skip_sdt_in_scan())
		{
			tcc_manager_demux_channelname_init();
			tcc_manager_demux_channelname_get_infoDB(iChannel, iCountryCode, iNetworkID, iServiceID, iServiceSubID);
		}

    	if(!TCC_Isdbt_IsAudioOnlyMode())
		{
			tcc_manager_video_initSurface(0);
		}

		tcc_manager_demux_scan_and_avplay(piAudioPID[iAudioMainIndex], piVideoPID[iVideoMainIndex], \
										 piAudioStreamType[iAudioMainIndex], piVideoStreamType[iVideoMainIndex], \
										 piAudioSubPID[iAudioSubIndex], piVideoSubPID[iVideoSubIndex], \
										 piAudioSubStreamType[iAudioSubIndex], piVideoSubStreamType[iVideoSubIndex], \
										 iPCRPID, iPCRSubPID, iPMTPID, iPMTSubPID, iCAECMPID, iACECMPID, dual_mode);

/* Noah, 20190219, IM692A-37	
		if(gStopRequest == true)
		{
			tcc_dxb_proc_lock_off();
			return -1;
		}
*/

		if (tcc_manager_audio_start(0, piAudioStreamType[iAudioMainIndex]) < 0)
		{
			piAudioStreamType[iAudioMainIndex] = 0xFFFF;
			piAudioPID[iAudioMainIndex] = 0xFFFF;
	    }

	    if (tcc_manager_audio_start(1, piAudioSubStreamType[iAudioSubIndex]) < 0)
		{
			piAudioSubStreamType[iAudioSubIndex] = 0xFFFF;
			piAudioSubPID[iAudioSubIndex] = 0xFFFF;
	    }

		if(TCC_Isdbt_IsSupportAlsaClose())
		{
			/*ALSA close*/
			tcc_manager_audio_alsa_close(1, 1);
			tcc_manager_audio_alsa_close(0, 1);
			tcc_manager_audio_alsa_close_flag(1);
		}

		if(!TCC_Isdbt_IsAudioOnlyMode())
		{
			if(tcc_dxb_proc_get_videoonoff_value() == 1)
			{
				if (tcc_manager_video_start(0, piVideoStreamType[iVideoMainIndex]) < 0)
				{
					piVideoStreamType[iVideoMainIndex] = 0xFFFF;
					piVideoPID[iVideoMainIndex] = 0xFFFF;
				}

				if (tcc_manager_video_start(1, piVideoSubStreamType[iVideoSubIndex]) < 0)
				{
					piVideoSubStreamType[iVideoSubIndex] = 0xFFFF;
					piVideoSubPID[iVideoSubIndex] = 0xFFFF;
				}
			}
		}

		tcc_manager_audio_set_proprietarydata(gTCCDxBProc.uiChannel, gTCCDxBProc.uiServiceID, gTCCDxBProc.uiServiceSubID, dual_mode, TCC_Isdbt_IsSupportPrimaryService());

		if(piAudioPID[iAudioMainIndex] != 0xFFFF
			&& piAudioSubPID[iAudioSubIndex] != 0xFFFF
			&& piAudioStreamType[iAudioMainIndex] != 0xFFFF
			&& piAudioSubStreamType[iAudioSubIndex] != 0xFFFF
			&& gTCCDxBSeamless.iOnoff == 1){
			tcc_manager_audio_setSeamlessSwitchCompensation(gTCCDxBSeamless.iOnoff, gTCCDxBSeamless.iInterval, gTCCDxBSeamless.iStrength,
				gTCCDxBSeamless.iNtimes, gTCCDxBSeamless.iRange, gTCCDxBSeamless.iGapadjust, gTCCDxBSeamless.iGapadjust2, gTCCDxBSeamless.iMultiplier);
		}

		if (dual_mode == TCCDXBPROC_MAIN)
		{
			subtitle_pid = iStPID;
			superimpose_pid = iSiPID;
			seg_type = 13;
		}
		else
		{
			subtitle_pid = iStSubPID;
			superimpose_pid = iSiSubPID;
			seg_type = 1;
		}

#if defined(HAVE_LINUX_PLATFORM)
		if(!TCC_Isdbt_IsAudioOnlyMode())
		{
			subtitle_display_wmixer_open();
		}
#endif

		if (seg_type == 13)
			subtitle_system_set_stc_index(0);
		else
			subtitle_system_set_stc_index(1);

 		tcc_manager_demux_subtitle_play(subtitle_pid, superimpose_pid, seg_type, gTCCDxBProc.uiCountrycode,
										iRaw_w, iRaw_h, iView_w, iView_h, 1);

		tcc_manager_audio_set_dualmono(0, iAudioMode);

		tcc_manager_video_select_display_output(dual_mode);
		tcc_manager_audio_select_output(dual_mode, true);
		tcc_manager_audio_select_output(dual_mode?0:1, false);
		gTCCDxBProc.uiCurrDualDecodeMode = dual_mode;

		TCCDxBEvent_AVIndexUpdate(iChannel, iNetworkID, iServiceID, iServiceSubID, iAudioIndex, iVideoIndex);
	}
	else
	{
		tcc_manager_demux_set_state(E_DEMUX_STATE_IDLE);
		tcc_manager_demux_release_searchfilter();

		tcc_manager_demux_set_skip_sdt_in_scan(FALSE);

   		TCCDxBProc_SetState(TCCDXBPROC_STATE_LOAD);
	}

	tcc_dxb_proc_lock_off();

    g_bSearchAndSetChannel_InProgress = 0;

	return 0;
}

/*
  * scan_type	0= initial scan, 1=rescan, 2=area_scan, 4=manual scan, 5=autosearch, 6=custom relay scan for JVC Kenwood
  * parameters
  * [initial scan]
  *		scan_type = 0
  *		country_code = 0 (Japan) or 1 (Brazil)
  *		area_code, channel_num, options - not used
  * [rescan]
  *		scan_type =1
  *		country_code = 0 (Japan) or 1 (Brazil)
  *		area_code, channel_num, options - not used
  * [area_scan]
  *		scan_type=2
  *		country_code = 0 (Japan) or 1 (Brazil)
  *		area_code = code for selected area
  *		channel_num, options - not used
  * [manual scan]
  *		scan_type = 4
  *		country_code = 0 (Japan) or 1 (Brazil)
  *		area_code = 0 - don't care
  *				     > 0 - a value of this parameter is network_id of service.
  *						If a new service is found, compare a network_id of found service with this value. If same, return true else false.
  *		channel_num = no. of select channel to scan
  *		options = bit0 1 - delete channel DB before to scan channels.
  *		                     0 - preserve channel DB
  *  [autosearch]
  *		search any channels from the next of current channel in specified direction.
  *		scan_type =5
  *		country_code = 0 (Japan) or 1 (Brazil)
  *		area_code - not used
  *		channel_num - current channel number. -1 means no channel was selected.
  *		options - direction. 0=down (forwarding to lower UHF channel no.), 1=up (increasing UHF channel no)
  * [custom scan]
  *		scan_type = 6
  *		country_code = search kind
  *				1 = relay station, 2 = affiliation station
  *		area_code = pointer to list of channels. 0 means UHF13.
  *		channel_num = pointer to list of transport_stream_id. A value of transport_stream_id is same with network_id in terrestrial broadcasting system.
  *		option = repeatibility
  *				0 = infinite
  *				n = n times
  */
static int tcc_dxb_proc_scan(int scan_type, int country_code, int area_code, int channel_num, int options)
{
	int request_cancel_notify = 0;
	int err;

	tcc_dxb_proc_lock_on();

   	if (TCCDxBProc_GetMainState() != TCCDXBPROC_STATE_SCAN)
   	{
		/* Noah / 20170829 / Add if statement for IM478A-30( (Improvement No3) At a weak signal, improvement of "searchAndSetChannel" )
			J&K's request
				1. searchAndSetChannel is called
				2. searchChannel is not called
				3. customRelayStationSearchRequest is called while searchAndSetChannel
				4. customRelayStationSearchRequest should work well without any problem
		*/
		if (CUSTOM_SCAN == scan_type)
		{
			TCCDxBProc_SetState(TCCDXBPROC_STATE_SCAN);
		}
		else
		{
			tcc_dxb_proc_lock_off();
			return -1;
		}
	}

	// Noah / 2017.04.07 / IM478A-13, Improvement of the Search API(Auto Search)
	TCCDxBProc_SetScanType((TCCDXBSCAN_TYPE)scan_type);

	if (scan_type != EPG_SCAN)
	{
		tcc_db_delete_epg(0);
	}

	tcc_manager_demux_set_state(E_DEMUX_STATE_SCAN);

	if (TCC_Isdbt_IsSupportBrazil())
	{
		tcc_manager_demux_set_parentlock(0);
	}

	ISDBT_Init_DescriptorInfo();
	ResetSIVersionNumber();

	tcc_dxb_proc_lock_off();

	err = tcc_manager_tuner_scan(scan_type, country_code, area_code, channel_num, options);

	tcc_dxb_proc_lock_on();

	if (err == 1)
	{
		//when tcc_manager_tuner_scan() returned scan-cancel
		//there can be a case that giTunerScanStop is false but giDemuxScanStop is true.
		tcc_manager_tuner_scanflag_init();
		tcc_manager_demux_scanflag_init();
	}

	tcc_dxb_proc_scanrequest_remove();

	if (tcc_manager_tuner_scanflag_get() || tcc_manager_demux_scanflag_get())
	{
		//in a case that scan is canceled just after tcc_manager_tuner_scan() return a success.
		request_cancel_notify = 1;
	}

	tcc_manager_demux_set_state(E_DEMUX_STATE_IDLE);
	tcc_manager_demux_release_searchfilter();

   	if (TCCDxBProc_GetMainState() == TCCDXBPROC_STATE_SCAN)
   	{
   		TCCDxBProc_SetState(TCCDXBPROC_STATE_LOAD);
   	}

	tcc_dxb_proc_lock_off();

	if(request_cancel_notify && scan_type != AUTOSEARCH && scan_type != EPG_SCAN)
		tcc_manager_tuner_scan_cancel_notify();

	return 0;
}

static int tcc_dxb_proc_handover(unsigned int *arg)
{
	int iCountryCode, iNetworkID;
	int error;

	int iChannelNumber=0, iMainRowID=0, iSubRowID=0;
	int iAudioTotalCount=0, iVideoTotalCount=0, iStPID=0xFFFF, iSiPID=0xFFFF, iServiceType=0, iServiceID=-1;
	int iSubAudioTotalCount=0, iSubVideoTotalCount=0, iStSubPID=-1, iSiSubPID=-1, iServiceSubType=0, iServiceSubID=-1;
	int iPCRPID=0, iPMTPID=0, iCAECMPID=0, iACECMPID=0;
	int iPCRSubPID=-1, iPMTSubPID=0, iCAECMSubPID=0, iACECMSubPID=0;
	int iThreeDigitNumber=-1;

	int piAudioPID[TCCDXBPROC_AUDIO_MAX_NUM] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
	int piAudioStreamType[TCCDXBPROC_AUDIO_MAX_NUM] = {0, 0, 0, 0};
	int piAudioComponentTag[TCCDXBPROC_AUDIO_MAX_NUM];
	int piAudioSubPID[TCCDXBPROC_AUDIO_MAX_NUM] = {-1, -1, -1, -1};
	int piAudioSubStreamType[TCCDXBPROC_AUDIO_MAX_NUM] = {-1, -1, -1, -1};
	int piAudioSubComponentTag[TCCDXBPROC_AUDIO_MAX_NUM];
	int piVideoPID[TCCDXBPROC_VIDEO_MAX_NUM] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
	int piVideoStreamType[TCCDXBPROC_VIDEO_MAX_NUM] = {0, 0, 0, 0};
	int piVideoComponentTag[TCCDXBPROC_VIDEO_MAX_NUM];
	int piVideoSubPID[TCCDXBPROC_VIDEO_MAX_NUM] = {-1, -1, -1, -1};
	int piVideoSubStreamType[TCCDXBPROC_VIDEO_MAX_NUM] = {-1, -1, -1, -1};
	int piVideoSubComponentTag[TCCDXBPROC_VIDEO_MAX_NUM];
	int iAudioIndex=0, iAudioMainIndex=0, iAudioSubIndex=0;
	int iVideoIndex=0, iVideoMainIndex=0, iVideoSubIndex=0;
	clock_t StartTime=0 , EndTime=0;
	int iSearchAffiliation=0;
	int iSameChannel=0;

	tcc_dxb_proc_lock_on();

   	if (TCCDxBProc_GetMainState() != TCCDXBPROC_STATE_HANDOVER)
	{
		tcc_dxb_proc_lock_off();
		return -1;
	}

	iCountryCode = (int)arg[0];

	if (TCC_Isdbt_IsSupportBrazil())
	{
		tcc_manager_demux_set_parentlock(0);
	}

	ISDBT_Init_DescriptorInfo();
	ResetSIVersionNumber();

	gTCCDxBProc.uiStopHandover = false;

	tcc_dxb_proc_lock_off();

	// search handover
	StartTime = clock();
	while (gTCCDxBProc.uiMsgHandlerRunning == 1)
	{
		if (gTCCDxBProc.uiStopHandover == true)
		{
			break;
		}

		if(iSearchAffiliation == 1)
		{
			StartTime = clock();
		}
		EndTime = clock();

		if ((((double)(EndTime-StartTime))/CLOCKS_PER_SEC) > 10)
		{
			iSearchAffiliation = 1;
		}
		else
		{
			iSearchAffiliation = 0;
		}

		iChannelNumber = tcc_manager_tuner_handover_scan(gTCCDxBProc.uiChannel, iCountryCode, iSearchAffiliation, &iSameChannel);
		if (iChannelNumber >= 0)
		{
			ALOGD("[%s] find channel(%d)!!!\n", __func__, iChannelNumber);

			tcc_manager_tuner_handover_clear();

			// set channel
			if (gTCCDxBProc.uiStopHandover == false)
			{
				iAudioIndex = gTCCDxBProc.uiAudioIndex;
				iVideoIndex = gTCCDxBProc.uiVideoIndex;

				if(TCC_Isdbt_IsSupportPrimaryService())
				{
					if(TCC_Isdbt_IsSupportProfileA())
					{
						iAudioTotalCount = TCCDXBPROC_AUDIO_MAX_NUM;
						iVideoTotalCount = TCCDXBPROC_VIDEO_MAX_NUM;

						tcc_db_get_primaryService(iChannelNumber, TCCDXBPROC_SERVICE_TYPE_12SEG, \
												&iMainRowID, &iAudioTotalCount, piAudioPID, &iVideoTotalCount, piVideoPID, \
												&iStPID, &iSiPID, &iServiceType, &iServiceID, &iPMTPID, &iCAECMPID, &iACECMPID, &iNetworkID, \
												piAudioStreamType, piAudioComponentTag, piVideoStreamType, piVideoComponentTag, &iPCRPID, &iThreeDigitNumber, \
												&iAudioIndex, &iAudioMainIndex, &iVideoIndex, &iVideoMainIndex);
						if(iMainRowID == 0)    // If fail to get Primary Service, existing method is used.
						{
							ALOGD("[%s] fail to get Primary Service of 12 seg, so existing method is used.\n", __func__);

							error = tcc_db_get_representative_channel(iChannelNumber, TCCDXBPROC_SERVICE_TYPE_12SEG, \
																	&iMainRowID, &iAudioTotalCount, piAudioPID, &iVideoTotalCount, piVideoPID, \
																	&iStPID, &iSiPID, &iServiceType, &iServiceID, &iPMTPID, &iCAECMPID, &iACECMPID, &iNetworkID, \
																	piAudioStreamType, piAudioComponentTag, piVideoStreamType, piVideoComponentTag, &iPCRPID, &iThreeDigitNumber, \
																	&iAudioIndex, &iAudioMainIndex, &iVideoIndex, &iVideoMainIndex);
						}
					}

					if(TCC_Isdbt_IsSupportProfileC())
					{
						iSubAudioTotalCount = TCCDXBPROC_AUDIO_MAX_NUM;
						iSubVideoTotalCount = TCCDXBPROC_VIDEO_MAX_NUM;

						tcc_db_get_primaryService(iChannelNumber, TCCDXBPROC_SERVICE_TYPE_1SEG, \
												&iSubRowID, &iSubAudioTotalCount, piAudioSubPID, &iSubVideoTotalCount, piVideoSubPID, \
												&iStSubPID, &iSiSubPID, &iServiceSubType, &iServiceSubID, &iPMTSubPID, &iCAECMSubPID, &iACECMSubPID, &iNetworkID, \
												piAudioSubStreamType, piAudioSubComponentTag, piVideoSubStreamType, piVideoSubComponentTag, &iPCRSubPID, &iThreeDigitNumber, \
												&iAudioIndex, &iAudioSubIndex, &iVideoIndex, &iVideoSubIndex);
						if(iSubRowID == 0)	  // If fail to get Primary Service, existing method is used.
						{
							ALOGD("[%s] fail to get Primary Service of 1 seg, so existing method is used.\n", __func__);

							error = tcc_db_get_representative_channel(iChannelNumber, TCCDXBPROC_SERVICE_TYPE_1SEG, \
																	&iSubRowID, &iSubAudioTotalCount, piAudioSubPID, &iSubVideoTotalCount, piVideoSubPID, \
																	&iStSubPID, &iSiSubPID, &iServiceSubType, &iServiceSubID, &iPMTSubPID, &iCAECMSubPID, &iACECMSubPID, &iNetworkID, \
																	piAudioSubStreamType, piAudioSubComponentTag, piVideoSubStreamType, piVideoSubComponentTag, &iPCRSubPID, &iThreeDigitNumber, \
																	&iAudioIndex, &iAudioSubIndex, &iVideoIndex, &iVideoSubIndex);
						}
					}
				}
				else
				{
					if(TCC_Isdbt_IsSupportProfileA())
					{
						iAudioTotalCount = TCCDXBPROC_AUDIO_MAX_NUM;
						iVideoTotalCount = TCCDXBPROC_VIDEO_MAX_NUM;
						error = tcc_db_get_representative_channel(iChannelNumber, TCCDXBPROC_SERVICE_TYPE_12SEG, \
															&iMainRowID, &iAudioTotalCount, piAudioPID, &iVideoTotalCount, piVideoPID, \
															&iStPID, &iSiPID, &iServiceType, &iServiceID, &iPMTPID, &iCAECMPID, &iACECMPID, &iNetworkID, \
															piAudioStreamType, piAudioComponentTag, piVideoStreamType, piVideoComponentTag, &iPCRPID, &iThreeDigitNumber, \
															&iAudioIndex, &iAudioMainIndex, &iVideoIndex, &iVideoMainIndex);
					}

					if(TCC_Isdbt_IsSupportProfileC())
					{
						iSubAudioTotalCount = TCCDXBPROC_AUDIO_MAX_NUM;
						iSubVideoTotalCount = TCCDXBPROC_VIDEO_MAX_NUM;
						error = tcc_db_get_representative_channel(iChannelNumber, TCCDXBPROC_SERVICE_TYPE_1SEG, \
															&iSubRowID, &iSubAudioTotalCount, piAudioSubPID, &iSubVideoTotalCount, piVideoSubPID, \
															&iStSubPID, &iSiSubPID, &iServiceSubType, &iServiceSubID, &iPMTSubPID, &iCAECMSubPID, &iACECMSubPID, &iNetworkID, \
															piAudioSubStreamType, piAudioSubComponentTag, piVideoSubStreamType, piVideoSubComponentTag, &iPCRSubPID, &iThreeDigitNumber, \
															&iAudioIndex, &iAudioSubIndex, &iVideoIndex, &iVideoSubIndex);
					}
				}

				if(iMainRowID > 0 || iSubRowID > 0)
				{
					if (!iSameChannel)
					{
						tcc_db_delete_channel(gTCCDxBProc.uiChannel, gTCCDxBProc.uiNetworkID);
					}

					TCCDxBProc_SetState(TCCDXBPROC_STATE_LOAD);

					TCCDxBProc_Set(iChannelNumber,
									iAudioTotalCount, (unsigned int*)piAudioPID, iVideoTotalCount, (unsigned int*)piVideoPID, (unsigned int*)piAudioStreamType, (unsigned int*)piVideoStreamType, \
									iSubAudioTotalCount, (unsigned int*)piAudioSubPID, iSubVideoTotalCount, (unsigned int*)piVideoSubPID, (unsigned int*)piAudioSubStreamType, (unsigned int*)piVideoSubStreamType, \
									iPCRPID, iPCRSubPID, iStPID, iSiPID, iServiceID, iPMTPID, iCAECMPID, iACECMPID, iNetworkID, \
									iStSubPID, iSiSubPID, iServiceSubID, iPMTSubPID, \
									iAudioIndex, iAudioMainIndex, iAudioSubIndex, iVideoIndex, iVideoMainIndex, iVideoSubIndex, gTCCDxBProc.uiAudioMode, \
									iServiceType, gTCCDxBProc.uiRaw_w, gTCCDxBProc.uiRaw_h,
									gTCCDxBProc.uiView_w, gTCCDxBProc.uiView_h,
									gTCCDxBProc.uiCurrDualDecodeMode);

					if (!iSameChannel)
					{
						TCCDxBEvent_ChannelUpdate(NULL);
					}

					return 0;
				}
				else
				{
					ALOGE("[%s] tcc_db_get_representative_channel failed!!\n", __func__);
				}
			}
		}

		usleep(50000);
	}

	tcc_manager_tuner_handover_clear();

	TCCDxBProc_SetState(TCCDXBPROC_STATE_LOAD);

	return 0;
}

int tcc_dxb_proc_setFieldLog(int fOn_Off, char *sPath)
{
	int ret = -1;

	//independent function about state

	if(fOn_Off) {
		ret = log_signal_open(sPath);
		if( ret == 0 ) {
			gTCCDxBProc.fLogSignalEnabled = TRUE;
		}
	} else {
		if( gTCCDxBProc.fLogSignalEnabled ) {
			gTCCDxBProc.fLogSignalEnabled = FALSE;
			ret = log_signal_close();
		}
	}
	return ret;
}

void* tcc_dxb_proc_thread(void *arg)
{
	ST_DXBPROC_MSG *pProcMsg;
	int scan_type, country_code, area_code, channel_num, options;

	char psPath[128];
	int fOn_Off = 0, cntFieldLog;
	int	*sqinfo = (int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(int) * 256);

	gTCCDxBProc.uiMsgHandlerRunning = 1;

	while (gTCCDxBProc.uiMsgHandlerRunning == 1)
	{
		if (tcc_dxb_proc_msg_get_count()) {
			pProcMsg = (ST_DXBPROC_MSG *)tcc_dxb_proc_msg_get();
			if (pProcMsg)
			{
				switch (pProcMsg->cmd) {
					case DXBPROC_MSG_SET:
						tcc_dxb_proc_set((unsigned int*)pProcMsg->arg);
						if (pProcMsg->arg) {
							unsigned int *arg = (unsigned int*)pProcMsg->arg;
							tcc_mw_free(__FUNCTION__, __LINE__, (void*)(arg[2]));
							tcc_mw_free(__FUNCTION__, __LINE__, (void*)(arg[3]));
							tcc_mw_free(__FUNCTION__, __LINE__, (void*)(arg[5]));
							tcc_mw_free(__FUNCTION__, __LINE__, (void*)(arg[6]));
							tcc_mw_free(__FUNCTION__, __LINE__, (void*)(arg[8]));
							tcc_mw_free(__FUNCTION__, __LINE__, (void*)(arg[9]));
							tcc_mw_free(__FUNCTION__, __LINE__, (void*)(arg[11]));
							tcc_mw_free(__FUNCTION__, __LINE__, (void*)(arg[12]));
						}
						break;

					case DXBPROC_MSG_SCAN:
						scan_type = ((int *)pProcMsg->arg)[0];
						country_code = ((int *)pProcMsg->arg)[1];
						area_code = ((int *)pProcMsg->arg)[2];
						channel_num = ((int *)pProcMsg->arg)[3];
						options = ((int *)pProcMsg->arg)[4];
						tcc_dxb_proc_scan(scan_type, country_code, area_code, channel_num, options);
						break;

					case DXBPROC_MSG_SCAN_AND_SET:
						tcc_dxb_proc_scanandset((unsigned int*)pProcMsg->arg);
						break;

					case DXBPROC_MSG_HANDOVER:
						tcc_dxb_proc_handover((unsigned int*)pProcMsg->arg);
						break;

					case DXBPROC_MSG_FIELDLOG:
						fOn_Off = (int)((char*)(pProcMsg->arg))[0];
						if(fOn_Off) {
							strncpy (psPath, &(((char *)(pProcMsg->arg))[1]), 128);
							cntFieldLog = 0;
						}
						tcc_dxb_proc_setFieldLog(fOn_Off, psPath);
						break;

					case DXBPROC_MSG_SETPRESETMODE:
						options = ((int *)pProcMsg->arg)[0];
						tcc_manager_demux_set_presetMode (options);
						break;

					default:
						ALOGE("[tcc_dxb_proc_thread] unknown command received(%d)\n", pProcMsg->cmd);
						break;
				}

				if (pProcMsg->arg) {
					tcc_mw_free(__FUNCTION__, __LINE__, pProcMsg->arg);
				}

				tcc_mw_free(__FUNCTION__, __LINE__, pProcMsg);
			}
		}
		else
		{
			if (fOn_Off) {	//field log
				cntFieldLog++;
				if (cntFieldLog >= 200) {	//per every 1sec
					tcc_manager_tuner_get_strength(sqinfo);
					if (sqinfo[0] > 4) {
						log_signal_write(sqinfo);
					}
					cntFieldLog = 0;
				}
			}
			usleep(5000);
		}
	}

	tcc_mw_free(__FUNCTION__, __LINE__, sqinfo);

	return (void*)NULL;
}

int tcc_dxb_proc_thread_init(void)
{
	int error = 0;
	error = tcc_dxb_thread_create((void *) & gTCCDxBProc.DxBProcMsgHandler,
								tcc_dxb_proc_thread,
								(unsigned char*)"tcc_dxb_proc_thread",
								LOW_PRIORITY_10, NULL);
	return error;
}

void tcc_dxb_proc_thread_deinit(void)
{
	gTCCDxBProc.uiMsgHandlerRunning = 0;
	tcc_dxb_thread_join((void *)gTCCDxBProc.DxBProcMsgHandler, NULL);

}

void tcc_dxb_proc_set_videoonoff_value(int onoff){
	tcc_dxb_VideoOnffValue_lock_on();

	setVideoOnffValue = onoff;
	ALOGE("[%s][%d]\n", __func__, onoff);

	tcc_dxb_VideoOnffValue_lock_off();
}

int tcc_dxb_proc_get_videoonoff_value(void){
	tcc_dxb_VideoOnffValue_lock_on();

	ALOGE("[%s][%d]\n", __func__, setVideoOnffValue);

	tcc_dxb_VideoOnffValue_lock_off();

	return setVideoOnffValue;
}
/****************************************************************************
DEFINITION OF FUNCTION
****************************************************************************/
void TCCDxBProc_SetState(unsigned int uiState)
{
	gTCCDxBProc.uiState = uiState;

	return;
}

void TCCDxBProc_SetSubState(unsigned int uiSubState)
{
	gTCCDxBProc.uiState |= uiSubState;

	return;
}

void TCCDxBProc_ClearSubState(unsigned int uiSubState)
{
	gTCCDxBProc.uiState &= (~uiSubState);

	return;
}

unsigned int TCCDxBProc_GetState(void)
{
	return gTCCDxBProc.uiState;
}

unsigned int TCCDxBProc_GetMainState(void)
{
	return (gTCCDxBProc.uiState & 0xFFFF0000);
}

unsigned int TCCDxBProc_GetSubState(void)
{
	return (gTCCDxBProc.uiState & 0x0000FFFF);
}

/* Noah / 2017.04.07 / IM478A-13, Improvement of the Search API(Auto Search)
	Request : While auto searching, 'setChannel' can be called without calling 'searchCancel'.
	Previous Sequence
		1. search( 5, , , , );							 -> Start 'Auto Search'
		2. Searching in progress . . .
		3. searchCancel before finishing the searching.
		4. setChannel( X, Y, , ... );					 -> Called by Application
		5. Playing the X service.
	New Sequence
		Remove 3rd step above.
	Implement
		1. Add if statement.
		1. In case of only AUTOSEARCH, according to the new sequence, DxB works.
		2. To save scan type, I made TCCDxBProc_Set( Get )ScanType functions.
		3. Call 'searchCancel()'
		4. Change the DxBProc state to TCCDXBPROC_STATE_LOAD because TCCDxBProc_Set returns Error.
*/
void TCCDxBProc_SetScanType(TCCDXBSCAN_TYPE scanType)
{
	if(MAX_SCAN <= scanType)
	{
		ALOGE("[%s] Invalid scanType : %d\n", __func__, scanType);
		return;
	}

	gTCCDxBProc.scanType = scanType;
}

TCCDXBSCAN_TYPE TCCDxBProc_GetScanType(void)
{
	return gTCCDxBProc.scanType;
}

/**
 * @brief
 *
 * @param
 *
 * @return
 *     0 : TCCDXBPROC_SUCCESS
 *    -1 : TCCDXBPROC_ERROR
 *    -2 : TCCDXBPROC_ERROR_INVALID_STATE
 *    -3 : TCCDXBPROC_ERROR_INVALID_PARAM
 *    -4 : tcc_manager_tuner_init error
 *    -5 : tcc_manager_demux_init error
 *    -6 : tcc_manager_audio_init error
 *    -7 : tcc_manager_video_init error
 *    -8 : tcc_manager_tuner_open error
 */
int TCCDxBProc_Init(unsigned int uiCountryCode)
{
	int err = TCCDXBPROC_SUCCESS;
	unsigned int   uiLen = 0, uiData = 0, uiBaseBandType = 0;
	char value[PROPERTY_VALUE_MAX];

	if(TCCDxBProc_GetMainState() != TCCDXBPROC_STATE_INIT) {
		ALOGE("[%s] Error invalid state(0x%X)",__func__, TCCDxBProc_GetState());

		return TCCDXBPROC_ERROR_INVALID_STATE;
	}

	memset(&gTCCDxBProc, 0x0, sizeof(ST_TCCDXB_PROC));
	memset(&gTCCDxBProc2, 0x0, sizeof(ST_TCCDXB_PROC2));
	TCCDxBProc_SetState(TCCDXBPROC_STATE_INIT);

	tcc_dxb_VideoOnffValue_lock_init();
	tcc_dxb_proc_msg_init();
	tcc_dxb_proc_lock_init();
	tcc_dxb_proc_lock_on();
	gTCCDxBProc.uiCountrycode = uiCountryCode;
	tcc_dxb_proc_thread_init();

	uiBaseBandType = TCC_Isdbt_GetBaseBand();

	tcc_manager_demux_set_state(E_DEMUX_STATE_IDLE);

	err = tcc_manager_tuner_init(uiBaseBandType);
	if (err != 0)
	{
		ALOGE("[%s] tcc_manager_tuner_init Error(%d)", __func__, err);
		tcc_dxb_proc_lock_off();
		return -4;
	}

	err = tcc_manager_demux_init();
	if (err != 0)
	{
		ALOGE("[%s] tcc_manager_demux_init Error(%d)", __func__, err);
		tcc_dxb_proc_lock_off();
		return -5;
	}

	err = tcc_manager_audio_init();
	if (err != 0)
	{
		ALOGE("[%s] tcc_manager_audio_init Error(%d)", __func__, err);
		tcc_dxb_proc_lock_off();
		return -6;
	}

	err = tcc_manager_video_init();
	if (err != 0)
	{
		ALOGE("[%s] tcc_manager_video_init Error(%d)", __func__, err);
		tcc_dxb_proc_lock_off();
		return -7;
	}

	err = tcc_manager_tuner_open(uiCountryCode);
	if (err != 0)
	{
		ALOGE("[%s] tcc_manager_tuner_open Error(%d)", __func__, err);
		tcc_dxb_proc_lock_off();
		return -8;
	}

	if (TCC_Isdbt_IsSupportBrazil()) {
		tcc_manager_demux_set_parentlock(0);
	}

	ResetSIVersionNumber();

	log_signal_init();
	tcc_dxb_proc_scanrequest_init();

	TCCDxBProc_SetAudioVideoSyncMode(1); //default : Normal mode(check PTS and STC)

	TCCDxBProc_SetState(TCCDXBPROC_STATE_LOAD);

	tcc_dxb_proc_lock_off();

	return TCCDXBPROC_SUCCESS;
}

/**
 * @brief
 *
 * @param
 *
 * @return
 *     0 : TCCDXBPROC_SUCCESS
 *    -1 : TCCDXBPROC_ERROR
 *    -2 : TCCDXBPROC_ERROR_INVALID_STATE
 *    -3 : TCCDXBPROC_ERROR_INVALID_PARAM
 *    -4 : tcc_manager_tuner_close error
 *    -5 : tcc_manager_tuner_deinit error
 *    -6 : tcc_manager_demux_deinit error
 *    -7 : tcc_manager_audio_deinit error
 *    -8 : tcc_manager_video_deinit error
 */
int TCCDxBProc_DeInit(void)
{
	int err = TCCDXBPROC_SUCCESS;

	if(TCCDxBProc_GetMainState() != TCCDXBPROC_STATE_LOAD)
	{
		ALOGE("[%s] Error invalid state(0x%X)",__func__, TCCDxBProc_GetState());
		return TCCDXBPROC_ERROR_INVALID_STATE;
	}

	tcc_dxb_proc_set_videoonoff_value(1);

	tcc_dxb_proc_thread_deinit();

	tcc_dxb_proc_lock_on();

	log_signal_deinit();

	tcc_dxb_proc_state_change_to_load(0);

	if (TCC_Isdbt_IsSupportSpecialService())
	{
		tcc_manager_demux_specialservice_DeleteDB();
	}

	err = tcc_manager_tuner_close();
	if (err != 0)
	{
		ALOGE("[%s] tcc_manager_tuner_init Error(%d)", __func__, err);
	    tcc_dxb_proc_lock_off();
		return -4;
	}

	err = tcc_manager_tuner_deinit();
	if (err != 0)
	{
		ALOGE("[%s] tcc_manager_tuner_init Error(%d)", __func__, err);
	    tcc_dxb_proc_lock_off();
		return -5;
	}

	err = tcc_manager_demux_deinit();
	if (err != 0)
	{
		ALOGE("[%s] tcc_manager_tuner_init Error(%d)", __func__, err);
	    tcc_dxb_proc_lock_off();
		return -6;
	}

	err = tcc_manager_audio_deinit();
	if (err != 0)
	{
		ALOGE("[%s] tcc_manager_tuner_init Error(%d)", __func__, err);
	    tcc_dxb_proc_lock_off();
		return -7;
	}

	err = tcc_manager_video_deinit();
	if (err != 0)
	{
		ALOGE("[%s] tcc_manager_tuner_init Error(%d)", __func__, err);
	    tcc_dxb_proc_lock_off();
		return -8;
	}

	TCCDxBProc_SetState(TCCDXBPROC_STATE_INIT);

	tcc_dxb_proc_lock_off();
	tcc_dxb_proc_lock_deinit();
	tcc_dxb_proc_msg_deinit();
	tcc_dxb_VideoOnffValue_lock_deinit();

	return TCCDXBPROC_SUCCESS;
}

void TCCDxBProc_Get_CASPIDInfo (unsigned int *pEsPids)
{
	//independent function about state

	*(pEsPids) = gTCCDxBProc.puiAudioPID[gTCCDxBProc.uiAudioMainIndex];
	*(pEsPids+1) = gTCCDxBProc.puiVideoPID[gTCCDxBProc.uiVideoMainIndex];
	*(pEsPids+2) = gTCCDxBProc.uiStPID;
	*(pEsPids+3) = gTCCDxBProc.uiSiPID;
}

void TCCDxbProc_Get_NetworkID (unsigned short *pusNetworkID)
{
	*pusNetworkID = (unsigned short)gTCCDxBProc.uiNetworkID;
}

int TCCDxBProc_Set(unsigned int uiChannel,
						unsigned int uiAudioTotalCount, unsigned int *puiAudioPID,
						unsigned int uiVideoTotalCount, unsigned int *puiVideoPID,
						unsigned int *puiAudioStreamType, unsigned int *puiVideoStreamType,
						unsigned int uiSubAudioTotalCount, unsigned int *puiAudioSubPID,
						unsigned int uiSubVideoTotalCount, unsigned int *puiVideoSubPID,
						unsigned int *puiAudioSubStreamType, unsigned int *puiVideoSubStreamType,
						unsigned int uiPcrPID, unsigned int uiPcrSubPID,
						unsigned int uiStPID, unsigned int uiSiPID, unsigned int uiServiceID, unsigned int uiPmtPID,
						unsigned int uiCAECMPID, unsigned int uiACECMPID, unsigned int uiNetworkID,
						unsigned int uiStSubPID, unsigned int uiSiSubPID, unsigned int uiServiceSubID, unsigned int uiPmtSubPID,
						unsigned int uiAudioIndex, unsigned int uiAudioMainIndex, unsigned int uiAudioSubIndex,
						unsigned int uiVideoIndex, unsigned int uiVideoMainIndex, unsigned int uiVideoSubIndex,
						unsigned int uiAudioMode, unsigned int service_type,
						unsigned int uiRaw_w, unsigned int uiRaw_h, unsigned int uiView_w, unsigned int uiView_h,
						int ch_index)
{
	ST_DXBPROC_MSG *pProcMsg;
	unsigned int *puiArg;
	unsigned int *puiArg_AudioPID, *puiArg_AudioStreamType;
	unsigned int *puiArg_AudioSubPID, *puiArg_AudioSubStreamType;
	unsigned int *puiArg_VideoPID, *puiArg_VideoStreamType;
	unsigned int *puiArg_VideoSubPID, *puiArg_VideoSubStreamType;
	int i;

	tcc_dxb_proc_lock_on();

	if((TCCDxBProc_GetMainState() != TCCDXBPROC_STATE_LOAD) && (TCCDxBProc_GetMainState() != TCCDXBPROC_STATE_AIRPLAY))
	{
		ALOGE("[%s] Error invalid state(0x%X)",__func__, TCCDxBProc_GetState());
		tcc_dxb_proc_lock_off();

		return TCCDXBPROC_ERROR_INVALID_STATE;
	}

	tcc_dxb_proc_msg_clear();

	pProcMsg = (ST_DXBPROC_MSG*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_MSG));
	puiArg = (unsigned int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(unsigned int) * 39);
	puiArg_AudioPID = (unsigned int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(unsigned int) * TCCDXBPROC_AUDIO_MAX_NUM);
	puiArg_AudioStreamType = (unsigned int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(unsigned int) * TCCDXBPROC_AUDIO_MAX_NUM);
	puiArg_AudioSubPID = (unsigned int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(unsigned int) * TCCDXBPROC_AUDIO_MAX_NUM);
	puiArg_AudioSubStreamType = (unsigned int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(unsigned int) * TCCDXBPROC_AUDIO_MAX_NUM);
	puiArg_VideoPID = (unsigned int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(unsigned int) * TCCDXBPROC_VIDEO_MAX_NUM);
	puiArg_VideoStreamType = (unsigned int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(unsigned int) * TCCDXBPROC_VIDEO_MAX_NUM);
	puiArg_VideoSubPID = (unsigned int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(unsigned int) * TCCDXBPROC_VIDEO_MAX_NUM);
	puiArg_VideoSubStreamType = (unsigned int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(unsigned int) * TCCDXBPROC_VIDEO_MAX_NUM);

	for(i=0; i<TCCDXBPROC_VIDEO_MAX_NUM; i++)
	{
		puiArg_VideoPID[i] = puiVideoPID[i];
		puiArg_VideoStreamType[i] = puiVideoStreamType[i];
		puiArg_VideoSubPID[i] = puiVideoSubPID[i];
		puiArg_VideoSubStreamType[i] = puiVideoSubStreamType[i];
	}

	for(i=0; i<TCCDXBPROC_AUDIO_MAX_NUM; i++)
	{
		puiArg_AudioPID[i] = puiAudioPID[i];
		puiArg_AudioStreamType[i] = puiAudioStreamType[i];
		puiArg_AudioSubPID[i] = puiAudioSubPID[i];
		puiArg_AudioSubStreamType[i] = puiAudioSubStreamType[i];
	}

	puiArg[0] = uiChannel;
	puiArg[1] = uiAudioTotalCount;
	puiArg[2] = (unsigned int)puiArg_AudioPID;
	puiArg[3] = (unsigned int)puiArg_AudioStreamType;
	puiArg[4] = uiVideoTotalCount;
	puiArg[5] = (unsigned int)puiArg_VideoPID;
	puiArg[6] = (unsigned int)puiArg_VideoStreamType;
	puiArg[7] = uiSubAudioTotalCount;
	puiArg[8] = (unsigned int)puiArg_AudioSubPID;
	puiArg[9] = (unsigned int)puiArg_AudioSubStreamType;
	puiArg[10] = uiSubVideoTotalCount;
	puiArg[11] = (unsigned int)puiArg_VideoSubPID;
	puiArg[12] = (unsigned int)puiArg_VideoSubStreamType;
	puiArg[13] = uiPcrPID;
	puiArg[14] = uiPcrSubPID;
	puiArg[15] = uiStPID;
	puiArg[16] = uiStSubPID;
	puiArg[17] = uiSiPID;
	puiArg[18] = uiSiSubPID;
	puiArg[19] = uiServiceID;
	puiArg[20] = uiServiceSubID;
	puiArg[21] = uiPmtPID;
	puiArg[22] = uiPmtSubPID;
	puiArg[23] = uiCAECMPID;
	puiArg[24] = uiACECMPID;
	puiArg[25] = uiAudioIndex;
	puiArg[26] = uiVideoIndex;
	puiArg[27] = uiAudioMode;
	puiArg[28] = service_type;
	puiArg[29] = uiNetworkID;
	puiArg[30] = uiRaw_w;
	puiArg[31] = uiRaw_h;
	puiArg[32] = uiView_w;
	puiArg[33] = uiView_h;
	puiArg[34] = ch_index;
	puiArg[35] = uiAudioMainIndex;
	puiArg[36] = uiAudioSubIndex;
	puiArg[37] = uiVideoMainIndex;
	puiArg[38] = uiVideoSubIndex;

// IM69A-30, Noah, 20190124
#if 0
	pProcMsg->cmd = DXBPROC_MSG_SET;
	pProcMsg->arg = (void *)puiArg;
	if(tcc_dxb_proc_msg_send((void *)pProcMsg)) {
		if(pProcMsg->arg) {
			unsigned int *arg = (unsigned int*)pProcMsg->arg;
			tcc_mw_free(__FUNCTION__, __LINE__, (void*)(arg[2]));
			tcc_mw_free(__FUNCTION__, __LINE__, (void*)(arg[3]));
			tcc_mw_free(__FUNCTION__, __LINE__, (void*)(arg[5]));
			tcc_mw_free(__FUNCTION__, __LINE__, (void*)(arg[6]));
			tcc_mw_free(__FUNCTION__, __LINE__, (void*)(arg[8]));
			tcc_mw_free(__FUNCTION__, __LINE__, (void*)(arg[9]));
			tcc_mw_free(__FUNCTION__, __LINE__, (void*)(arg[11]));
			tcc_mw_free(__FUNCTION__, __LINE__, (void*)(arg[12]));
			tcc_mw_free(__FUNCTION__, __LINE__, (void*)(arg));
		}
		tcc_mw_free(__FUNCTION__, __LINE__, pProcMsg);

	tcc_dxb_proc_lock_off();

        	return TCCDXBPROC_ERROR;
	}
#endif

	if(TCCDxBProc_GetMainState() == TCCDXBPROC_STATE_LOAD) {
		TCCDxBProc_SetState(TCCDXBPROC_STATE_AIRPLAY);
	}

// IM69A-30, Noah, 20190124
#if 1
    tcc_dxb_proc_set(puiArg);

	tcc_mw_free(__FUNCTION__, __LINE__, (void*)(puiArg[2]));
	tcc_mw_free(__FUNCTION__, __LINE__, (void*)(puiArg[3]));
	tcc_mw_free(__FUNCTION__, __LINE__, (void*)(puiArg[5]));
	tcc_mw_free(__FUNCTION__, __LINE__, (void*)(puiArg[6]));
	tcc_mw_free(__FUNCTION__, __LINE__, (void*)(puiArg[8]));
	tcc_mw_free(__FUNCTION__, __LINE__, (void*)(puiArg[9]));
	tcc_mw_free(__FUNCTION__, __LINE__, (void*)(puiArg[11]));
	tcc_mw_free(__FUNCTION__, __LINE__, (void*)(puiArg[12]));

	tcc_mw_free(__FUNCTION__, __LINE__, puiArg);

	tcc_mw_free(__FUNCTION__, __LINE__, pProcMsg);
#endif

	tcc_dxb_proc_lock_off();

	return TCCDXBPROC_SUCCESS;
}

/**
 * @brief
 *
 * @param
 *
 * @return The return value of this function is the return value of the tcc_dxb_proc_state_change_to_load.
 *     0 : TCCDXBPROC_SUCCESS
 *    -1 : TCCDXBPROC_ERROR
 *    -2 : TCCDXBPROC_ERROR_INVALID_STATE
 *    -3 : TCCDXBPROC_ERROR_INVALID_PARAM
 *    -4 : tcc_manager_audio_stop(1) error
 *    -5 : tcc_manager_audio_stop(0) error
 *    -6 : tcc_manager_video_stop(1) error
 *    -7 : tcc_manager_video_stop(0) error
 *    -8 : tcc_manager_demux_av_stop(1, 1, 1, 1) error
 *    -9 : tcc_manager_video_set_proprietarydata error
 *    -10 : tcc_manager_demux_subtitle_stop error
 *    -11 : tcc_manager_video_deinitSurface error
 */
int TCCDxBProc_Stop(void)
{
	int err = TCCDXBPROC_SUCCESS;
    unsigned int waitCountForSectionDecoder = 0;
    unsigned int waitCountForSearchAndSetChannel = 0;

//	gStopRequest = true;

#if defined(STOP_TIME_CHECK)
	long long stop_start = 0;
	long long stop_end = 0;
	stop_start = StopTimeCheck();
#endif

    // Noah, IM692A-29
	tcc_manager_demux_set_section_decoder_state(2);
    while (1)
    {
        if (0 == tcc_manager_demux_get_section_decoder_state())
        {
            break;
        }

        waitCountForSectionDecoder++;
        if (300 < waitCountForSectionDecoder)    // if waiting 3000 ms over, then break
        {
            ALOGD("[%s] It breaks waiting loop for section_decoder due to 3000 ms over. Please check this log !!!\n", __func__);
            break;
        }

        usleep(10 * 1000);
    }

#if defined(STOP_TIME_CHECK)
	stop_end = StopTimeCheck();
	printf("Noah / [%s] section_decoder thread(%04d)\n", __func__, (int)(stop_end-stop_start));
#endif

    // Noah, IM692A-37, searchAndSetChannel 중이라면, 처리가 끝나기리를 기다린다.
    {
        if (1 == g_bSearchAndSetChannel_InProgress)
        {
            ALOGD("[%s] searchAndSetChannel is in progress. proc state(0x%x, 0x%x)\n", __func__, TCCDxBProc_GetMainState(), TCCDxBProc_GetSubState());
        
    		tcc_manager_tuner_scan_cancel();
    		tcc_manager_demux_scan_cancel();

            while (1)
            {
                if (0 == g_bSearchAndSetChannel_InProgress)
                {
                    break;
                }

                waitCountForSearchAndSetChannel++;
                if (300 < waitCountForSearchAndSetChannel)    // if waiting 3000 ms over, then break
                {
                    ALOGD("[%s] It breaks waiting loop for waitCountForSearchAndSetChannel due to 3000 ms over. Please check this log !!!\n", __func__);
                    break;
                }

                usleep(10 * 1000);
            }
        }

    	tcc_manager_tuner_scanflag_init();
    	tcc_manager_demux_scanflag_init();
    }

	tcc_manager_tuner_UserLoopStopCmd(0);

	tcc_dxb_proc_lock_on();

	if (TCCDxBProc_GetMainState() != TCCDXBPROC_STATE_AIRPLAY)
	{
		if (TCCDxBProc_GetMainState() != TCCDXBPROC_STATE_EWSMODE)
		{
			ALOGE("[%s] Error invalid state(0x%X)",__func__, TCCDxBProc_GetState());
			tcc_dxb_proc_lock_off();
			return TCCDXBPROC_ERROR_INVALID_STATE;
		}
		else
		{
			TCCDxBProc_SetState(TCCDXBPROC_STATE_AIRPLAY);
			TCCDxBProc_SetSubState(iDual_mode?TCCDXBPROC_AIRPLAY_SUB:TCCDXBPROC_AIRPLAY_MAIN);
		}
	}
    
	if (TCCDxBProc_GetSubState() != TCCDXBPROC_AIRPLAY_MSG)
	{
		tcc_manager_demux_set_state(E_DEMUX_STATE_IDLE);
	}

	err = tcc_dxb_proc_state_change_to_load(0);
	if (err != 0)
	{
		ALOGE("[%s] tcc_dxb_proc_state_change_to_load Error(%d)", __func__, err);
		return err;
	}

	if (TCC_Isdbt_IsSupportBrazil())
	{
		tcc_manager_demux_set_parentlock(0);
	}

	ResetSIVersionNumber();

	TCCDxBProc_SetState(TCCDXBPROC_STATE_LOAD);
        
	tcc_dxb_proc_lock_off();

#if defined(STOP_TIME_CHECK)
	stop_end = StopTimeCheck();
	printf("Noah / [%s] stop total(%04d)\n", __func__, (int)(stop_end-stop_start));
#endif

	return TCCDXBPROC_SUCCESS;
}

int TCCDxBProc_SetHandover(int country_code)
{
	unsigned int *puiArg;
	ST_DXBPROC_MSG *pProcMsg;
	int error;

	tcc_dxb_proc_lock_on();

	if((TCCDxBProc_GetMainState() != TCCDXBPROC_STATE_LOAD)
	    && (TCCDxBProc_GetMainState() != TCCDXBPROC_STATE_AIRPLAY)) {
		ALOGE("[%s] Error invalid state(0x%X)",__func__, TCCDxBProc_GetState());
		tcc_dxb_proc_lock_off();

		return TCCDXBPROC_ERROR_INVALID_STATE;
	}

	// load handover info
	error = tcc_manager_tuner_handover_load(gTCCDxBProc.uiChannel, gTCCDxBProc.uiServiceID);
	if (error != 0) {
		ALOGD("[%s] can't load handover infomation(%d)!\n",__func__, error);
		tcc_dxb_proc_lock_off();

		return TCCDXBPROC_ERROR;
	}

	tcc_manager_tuner_handover_update(gTCCDxBProc.uiNetworkID);

	tcc_dxb_proc_state_change_to_load(0);

	pProcMsg = (ST_DXBPROC_MSG*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_MSG));
#if 0
	puiArg = (unsigned int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(unsigned int) * 2);
#else
	puiArg = (unsigned int*)tcc_mw_calloc(__FUNCTION__, __LINE__, 1, sizeof(unsigned int) * 2);
#endif

	puiArg[0] = (unsigned int)country_code;

	pProcMsg->cmd = DXBPROC_MSG_HANDOVER;
	pProcMsg->arg = (void *)puiArg;
	if(tcc_dxb_proc_msg_send((void *)pProcMsg)) {
		if(pProcMsg->arg) {
			tcc_mw_free(__FUNCTION__, __LINE__, puiArg);
		}
		tcc_mw_free(__FUNCTION__, __LINE__, pProcMsg);

	    tcc_dxb_proc_lock_off();

        return TCCDXBPROC_ERROR;
	}

   	TCCDxBProc_SetState(TCCDXBPROC_STATE_HANDOVER);

	tcc_dxb_proc_lock_off();

	return TCCDXBPROC_SUCCESS;
}

int TCCDxBProc_StopHandover(void)
{
	int error = 0;

	while (TCCDxBProc_GetState() == TCCDXBPROC_STATE_HANDOVER) {
		gTCCDxBProc.uiStopHandover = true;

		if ((tcc_manager_demux_get_state() == E_DEMUX_STATE_HANDOVER)
			|| (tcc_manager_demux_get_state() == E_DEMUX_STATE_SCAN)
			|| (tcc_manager_demux_get_state() == E_DEMUX_STATE_INFORMAL_SCAN)) {
			error |= tcc_manager_tuner_scan_cancel();
			error |= tcc_manager_demux_scan_cancel();
		}

		usleep(50000);
	}

	TCCDxBProc_Stop();

	return error;
}

int TCCDxBProc_Update(unsigned int uiAudioTotalCount, unsigned int puiAudioPID[], unsigned int puiAudioStreamType[], \
						unsigned int uiVideoTotalCount, unsigned int puiVideoPID[], unsigned int puiVideoStreamType[], unsigned int uiStPID, unsigned int uiSiPID)
{
	int i;
	int error = false;
	int uiSegType;
	int bChange_Audio = 0, bChange_Video = 0, bChange_Caption = 0;
	int bChange_SubAudio = 0, bChange_SubVideo = 0, bChange_SubCaption = 0;
	int bChange_PID = 0;
	unsigned int iPcrPid=0, uiServiceId=0;
	unsigned int *puiArg;
	ST_DXBPROC_MSG *pProcMsg;
	ST_CHANNEL_DATA	ch_data;

	tcc_dxb_proc_lock_on();

	if (TCCDxBProc_GetMainState() != TCCDXBPROC_STATE_AIRPLAY && TCCDxBProc_GetMainState() != TCCDXBPROC_STATE_EWSMODE) {
		ALOGE("[%s] Error invalid state(0x%X)",__func__, TCCDxBProc_GetState());
		tcc_dxb_proc_lock_off();

		return false;
	}

	TCCDxBProc_SetSubState(TCCDXBPROC_AIRPLAY_PMTUPDATE);

	ISDBT_Get_ServiceInfo(&iPcrPid, &uiServiceId);

	ALOGD("[PMT-Updated] ID(%d) M(A(%d)(%d,%d)V(%d)(%d,%d)S(%d,%d)) S(A(%d)(%d,%d)V(%d)(%d,%d)S(%d,%d)\n => A(%d)(%d,%d)V(%d)(%d,%d)S(%d,%d)", uiServiceId,
			gTCCDxBProc.uiAudioTotalCount, gTCCDxBProc.puiAudioPID[0], gTCCDxBProc.puiAudioPID[1], \
			gTCCDxBProc.uiVideoTotalCount, gTCCDxBProc.puiVideoPID[0], gTCCDxBProc.puiVideoPID[1], gTCCDxBProc.uiStPID, gTCCDxBProc.uiSiPID,
			gTCCDxBProc.uiSubAudioTotalCount, gTCCDxBProc.puiAudioSubPID[0], gTCCDxBProc.puiAudioSubPID[1],
			gTCCDxBProc.uiSubVideoTotalCount, gTCCDxBProc.puiVideoSubPID[0], gTCCDxBProc.puiVideoSubPID[1], gTCCDxBProc.uiStSubPID, gTCCDxBProc.uiSiSubPID,
			uiAudioTotalCount, puiAudioPID[0], puiAudioPID[1], uiVideoTotalCount, puiVideoPID[0], puiVideoPID[1], uiStPID, uiSiPID);

	if (uiAudioTotalCount > TCCDXBPROC_AUDIO_MAX_NUM)
	{
		uiAudioTotalCount = TCCDXBPROC_AUDIO_MAX_NUM;
	}

	if (uiVideoTotalCount > TCCDXBPROC_VIDEO_MAX_NUM)
	{
		uiVideoTotalCount = TCCDXBPROC_VIDEO_MAX_NUM;
	}

	/*========================================================================*/
	//	Check if any changes
	/*========================================================================*/
	if ((gTCCDxBProc.uiServiceID > 0 && gTCCDxBProc.uiServiceID < 0xFFFF) && (gTCCDxBProc.uiServiceID == uiServiceId))
	{
		if ((uiAudioTotalCount > 0 && uiAudioTotalCount != gTCCDxBProc.uiAudioTotalCount)
			|| (uiAudioTotalCount > 0 && gTCCDxBProc.uiAudioTotalCount == 1 && (gTCCDxBProc.puiAudioPID[0] != puiAudioPID[0]))
			|| (uiAudioTotalCount > 0 && gTCCDxBProc.uiAudioTotalCount == 2 && (gTCCDxBProc.puiAudioPID[0] != puiAudioPID[0] || gTCCDxBProc.puiAudioPID[1] != puiAudioPID[1]))
			|| (uiAudioTotalCount > 0 && gTCCDxBProc.uiAudioTotalCount == 3 && (gTCCDxBProc.puiAudioPID[0] != puiAudioPID[0] || gTCCDxBProc.puiAudioPID[1] != puiAudioPID[1] || gTCCDxBProc.puiAudioPID[2] != puiAudioPID[2])))
		{
			ALOGD("[PMT-Changed] MainAudio ID(%d) A(%d)(%d,%d,%d) => A(%d)(%d,%d,%d)",
					uiServiceId, gTCCDxBProc.uiAudioTotalCount, gTCCDxBProc.puiAudioPID[0], gTCCDxBProc.puiAudioPID[1], gTCCDxBProc.puiAudioPID[2], uiAudioTotalCount, puiAudioPID[0], puiAudioPID[1], puiAudioPID[2]);

			int bChange_AudioPID = 0;

			if (gTCCDxBProc.uiAudioIndex == gTCCDxBProc.uiAudioMainIndex && puiAudioPID[gTCCDxBProc.uiAudioIndex] >= 0x1FFF) //if the audio is disappeared
			{
				gTCCDxBProc.uiAudioMainIndex = !gTCCDxBProc.uiAudioIndex;
				bChange_AudioPID =1;
			}
			else if (gTCCDxBProc.uiAudioIndex != gTCCDxBProc.uiAudioMainIndex && puiAudioPID[gTCCDxBProc.uiAudioIndex] < 0x1FFF) //if the audio is detected
			{
				gTCCDxBProc.uiAudioMainIndex = gTCCDxBProc.uiAudioIndex;
				bChange_AudioPID =1;
			}
			else if(gTCCDxBProc.puiAudioPID[gTCCDxBProc.uiAudioMainIndex] != puiAudioPID[gTCCDxBProc.uiAudioMainIndex] && puiAudioPID[gTCCDxBProc.uiAudioMainIndex] < 0x1FFF) //if the audio is changed
			{
				bChange_AudioPID =1;
			}

			if (bChange_AudioPID == 1)
			{
				tcc_manager_audio_stop(0);
				tcc_manager_demux_change_audioPID(1, puiAudioPID[gTCCDxBProc.uiAudioMainIndex], 0, gTCCDxBProc.puiAudioSubPID[gTCCDxBProc.uiAudioSubIndex]);
				if(TCCDxBProc_GetMainState() == TCCDXBPROC_STATE_AIRPLAY)
					tcc_manager_audio_start(0, puiAudioStreamType[gTCCDxBProc.uiAudioMainIndex]);

				if(gTCCDxBProc.puiAudioPID[gTCCDxBProc.uiAudioMainIndex] != 0xFFFF
					&& gTCCDxBProc.puiAudioSubPID[gTCCDxBProc.uiAudioSubIndex] != 0xFFFF
					&& puiAudioStreamType[gTCCDxBProc.uiAudioMainIndex] != 0xFFFF
					&& puiAudioStreamType[gTCCDxBProc.uiAudioSubIndex] != 0xFFFF
					&& gTCCDxBSeamless.iOnoff== 1){
					ALOGE("[DFS] AudioMainPID changed start");
					tcc_manager_audio_setSeamlessSwitchCompensation(gTCCDxBSeamless.iOnoff, gTCCDxBSeamless.iInterval, gTCCDxBSeamless.iStrength,
						gTCCDxBSeamless.iNtimes, gTCCDxBSeamless.iRange, gTCCDxBSeamless.iGapadjust, gTCCDxBSeamless.iGapadjust2, gTCCDxBSeamless.iMultiplier);
				}
			}

			gTCCDxBProc.uiAudioTotalCount = uiAudioTotalCount;
			for (i = 0; i < TCCDXBPROC_AUDIO_MAX_NUM; ++i)
			{
				gTCCDxBProc.puiAudioPID[i]			= puiAudioPID[i];
				gTCCDxBProc.puiAudioStreamType[i]	= puiAudioStreamType[i];
			}

			bChange_Audio = 1;
		}

		if ((uiVideoTotalCount > 0 && uiVideoTotalCount != gTCCDxBProc.uiVideoTotalCount)
			|| (uiVideoTotalCount > 0 && gTCCDxBProc.uiVideoTotalCount == 1 && (gTCCDxBProc.puiVideoPID[0] != puiVideoPID[0]))
			|| (uiVideoTotalCount > 0 && gTCCDxBProc.uiVideoTotalCount == 2 && (gTCCDxBProc.puiVideoPID[0] != puiVideoPID[0] || gTCCDxBProc.puiVideoPID[1] != puiVideoPID[1]))
			|| (uiVideoTotalCount > 0 && gTCCDxBProc.uiVideoTotalCount == 3 && (gTCCDxBProc.puiVideoPID[0] != puiVideoPID[0] || gTCCDxBProc.puiVideoPID[1] != puiVideoPID[1] || gTCCDxBProc.puiVideoPID[2] != puiVideoPID[2])))
		{
			ALOGD("[PMT-Changed] MainVideo ID(%d) V(%d)(%d,%d,%d) => V(%d)(%d,%d,%d)\n",
					uiServiceId, gTCCDxBProc.uiVideoTotalCount, gTCCDxBProc.puiVideoPID[0], gTCCDxBProc.puiVideoPID[1], gTCCDxBProc.puiVideoPID[2], uiVideoTotalCount, puiVideoPID[0], puiVideoPID[1], puiVideoPID[2]);

			int bChange_VideoPID = 0;

			if (gTCCDxBProc.uiVideoIndex == gTCCDxBProc.uiVideoMainIndex && puiVideoPID[gTCCDxBProc.uiVideoIndex] >= 0x1FFF) //if the audio is disappeared
			{
				gTCCDxBProc.uiVideoMainIndex = !gTCCDxBProc.uiVideoIndex;
				bChange_VideoPID =1;
			}
			else if (gTCCDxBProc.uiVideoIndex != gTCCDxBProc.uiVideoMainIndex && puiVideoPID[gTCCDxBProc.uiVideoIndex] < 0x1FFF) //if the audio is detected
			{
				gTCCDxBProc.uiVideoMainIndex = gTCCDxBProc.uiVideoIndex;
				bChange_VideoPID =1;
			}
			else if(gTCCDxBProc.puiVideoPID[gTCCDxBProc.uiVideoMainIndex] != puiVideoPID[gTCCDxBProc.uiVideoMainIndex] && puiVideoPID[gTCCDxBProc.uiVideoMainIndex] < 0x1FFF) //if the audio is changed
			{
				bChange_VideoPID =1;
			}

			if (bChange_VideoPID == 1)
			{
				tcc_manager_video_stop(0);
				tcc_manager_demux_change_videoPID(1, puiVideoPID[gTCCDxBProc.uiVideoMainIndex], puiVideoStreamType[gTCCDxBProc.uiVideoMainIndex], \
													0, gTCCDxBProc.puiVideoSubPID[gTCCDxBProc.uiVideoSubIndex], gTCCDxBProc.puiVideoSubStreamType[gTCCDxBProc.uiVideoSubIndex]);
				if(TCCDxBProc_GetMainState() == TCCDXBPROC_STATE_AIRPLAY){
					if(!TCC_Isdbt_IsAudioOnlyMode()){
						if(tcc_dxb_proc_get_videoonoff_value() == 1){
							tcc_manager_video_start(0, puiVideoStreamType[gTCCDxBProc.uiVideoMainIndex]);
						}
					}
				}
			}

			gTCCDxBProc.uiVideoTotalCount = uiVideoTotalCount;
			for (i = 0; i < TCCDXBPROC_VIDEO_MAX_NUM; ++i)
			{
				gTCCDxBProc.puiVideoPID[i]			= puiVideoPID[i];
				gTCCDxBProc.puiVideoStreamType[i]	= puiVideoStreamType[i];
			}

			bChange_Video = 1;
		}

		if ((uiStPID > 2 && uiStPID != gTCCDxBProc.uiStPID)
			|| (uiSiPID > 2 && uiSiPID != gTCCDxBProc.uiSiPID))
		{
			ALOGD("[PMT-Changed] MainCaption ID(%d) S(%d,%d) => St(%d,%d)\n", uiServiceId, gTCCDxBProc.uiStPID, gTCCDxBProc.uiSiPID, uiStPID, uiSiPID);

			uiSegType = (gTCCDxBProc.uiPmtPID >= 0x1FC8) ? 1:13;

			if(tcc_manager_demux_get_playing_serviceID() == uiServiceId){
				tcc_manager_demux_set_eit_subtitleLangInfo_clear();
			    if (uiSegType == 13) subtitle_system_set_stc_index(0);
			    else subtitle_system_set_stc_index(1);
				if(TCCDxBProc_GetMainState() == TCCDXBPROC_STATE_AIRPLAY)
					tcc_manager_demux_subtitle_play(uiStPID, uiSiPID, uiSegType, gTCCDxBProc.uiCountrycode, gTCCDxBProc.uiRaw_w, gTCCDxBProc.uiRaw_h, gTCCDxBProc.uiView_w, gTCCDxBProc.uiView_h, 0);
			}

			gTCCDxBProc.uiStPID = uiStPID;
			gTCCDxBProc.uiSiPID = uiSiPID;

			bChange_Caption = 1;
		}

		{
			ISDBT_Get_ComponentTagInfo(&gTCCDxBProc2.uiComponentTagCount, &gTCCDxBProc2.puiComponentTag[0]);

			#if 0
			int k;
			printf("[COMPTAG][M](%d) - ", gTCCDxBProc2.uiComponentTagCount);

			for (k = 0; k < gTCCDxBProc2.uiComponentTagCount; ++k)
				printf("%02X ", gTCCDxBProc2.puiComponentTag[k]);

			printf("\n\n");
			#endif
		}
	}
	else if ((gTCCDxBProc.uiServiceSubID > 0 && gTCCDxBProc.uiServiceSubID < 0xFFFF) && (gTCCDxBProc.uiServiceSubID == uiServiceId))
	{
		if ((uiAudioTotalCount > 0 && uiAudioTotalCount != gTCCDxBProc.uiSubAudioTotalCount)
			|| (uiAudioTotalCount > 0 && gTCCDxBProc.uiSubAudioTotalCount == 1 && (gTCCDxBProc.puiAudioSubPID[0] != puiAudioPID[0]))
			|| (uiAudioTotalCount > 0 && gTCCDxBProc.uiSubAudioTotalCount == 2 && (gTCCDxBProc.puiAudioSubPID[0] != puiAudioPID[0] || gTCCDxBProc.puiAudioSubPID[1] != puiAudioPID[1]))
			|| (uiAudioTotalCount > 0 && gTCCDxBProc.uiSubAudioTotalCount == 3 && (gTCCDxBProc.puiAudioSubPID[0] != puiAudioPID[0] || gTCCDxBProc.puiAudioSubPID[1] != puiAudioPID[1] || gTCCDxBProc.puiAudioSubPID[2] != puiAudioPID[2])))
		{
			ALOGD("[PMT-Changed] SubAudio ID(%d) A(%d)(%d,%d,%d) => A(%d)(%d,%d,%d)\n",
					uiServiceId, gTCCDxBProc.uiSubAudioTotalCount, gTCCDxBProc.puiAudioSubPID[0], gTCCDxBProc.puiAudioSubPID[1], gTCCDxBProc.puiAudioSubPID[2], uiAudioTotalCount, puiAudioPID[0], puiAudioPID[1], puiAudioPID[2]);

			int bChange_SubAudioPID = 0;

			if (gTCCDxBProc.uiAudioIndex == gTCCDxBProc.uiAudioSubIndex && puiAudioPID[gTCCDxBProc.uiAudioIndex] >= 0x1FFF) //if the audio is disappeared
			{
				gTCCDxBProc.uiAudioSubIndex = !gTCCDxBProc.uiAudioIndex;
				bChange_SubAudioPID = 1;

			}
			else if (gTCCDxBProc.uiAudioIndex != gTCCDxBProc.uiAudioSubIndex && puiAudioPID[gTCCDxBProc.uiAudioIndex] < 0x1FFF) //if the audio is detected
			{
				gTCCDxBProc.uiAudioSubIndex = gTCCDxBProc.uiAudioIndex;
				bChange_SubAudioPID = 1;
			}
			else if(gTCCDxBProc.puiAudioSubPID[gTCCDxBProc.uiAudioSubIndex] != puiAudioPID[gTCCDxBProc.uiAudioSubIndex] && puiAudioPID[gTCCDxBProc.uiAudioSubIndex] < 0x1FFF) //if the audio is changed
			{
				bChange_SubAudioPID = 1;
			}

			if (bChange_SubAudioPID == 1)
			{
				tcc_manager_audio_stop(1);
				tcc_manager_demux_change_audioPID(0, gTCCDxBProc.puiAudioPID[gTCCDxBProc.uiAudioMainIndex], 1, puiAudioPID[gTCCDxBProc.uiAudioSubIndex]);
				if(TCCDxBProc_GetMainState() == TCCDXBPROC_STATE_AIRPLAY)
					tcc_manager_audio_start(1, puiAudioStreamType[gTCCDxBProc.uiAudioSubIndex]);

				if(gTCCDxBProc.puiAudioPID[gTCCDxBProc.uiAudioMainIndex] != 0xFFFF
					&& gTCCDxBProc.puiAudioSubPID[gTCCDxBProc.uiAudioSubIndex] != 0xFFFF
					&& puiAudioStreamType[gTCCDxBProc.uiAudioMainIndex] != 0xFFFF
					&& puiAudioStreamType[gTCCDxBProc.uiAudioSubIndex] != 0xFFFF
					&& gTCCDxBSeamless.iOnoff== 1){
					ALOGE("[DFS] AudioSubPID changed start");
					tcc_manager_audio_setSeamlessSwitchCompensation(gTCCDxBSeamless.iOnoff, gTCCDxBSeamless.iInterval, gTCCDxBSeamless.iStrength,
						gTCCDxBSeamless.iNtimes, gTCCDxBSeamless.iRange, gTCCDxBSeamless.iGapadjust, gTCCDxBSeamless.iGapadjust2, gTCCDxBSeamless.iMultiplier);
				}
			}

			gTCCDxBProc.uiSubAudioTotalCount = uiAudioTotalCount;
			for (i = 0; i < TCCDXBPROC_AUDIO_MAX_NUM; ++i)
			{
				gTCCDxBProc.puiAudioSubPID[i]			= puiAudioPID[i];
				gTCCDxBProc.puiAudioSubStreamType[i]	= puiAudioStreamType[i];
			}

			bChange_SubAudio = 1;
		}

		if ((uiVideoTotalCount > 0 && uiVideoTotalCount != gTCCDxBProc.uiSubVideoTotalCount)
			|| (uiVideoTotalCount > 0 && gTCCDxBProc.uiSubVideoTotalCount == 1 && (gTCCDxBProc.puiVideoSubPID[0] != puiVideoPID[0]))
			|| (uiVideoTotalCount > 0 && gTCCDxBProc.uiSubVideoTotalCount == 2 && (gTCCDxBProc.puiVideoSubPID[0] != puiVideoPID[0] || gTCCDxBProc.puiVideoSubPID[1] != puiVideoPID[1]))
			|| (uiVideoTotalCount > 0 && gTCCDxBProc.uiSubVideoTotalCount == 3 && (gTCCDxBProc.puiVideoSubPID[0] != puiVideoPID[0] || gTCCDxBProc.puiVideoSubPID[1] != puiVideoPID[1] || gTCCDxBProc.puiVideoSubPID[2] != puiVideoPID[2])))
		{
			ALOGD("[PMT-Changed] SubVideo ID(%d) V(%d)(%d,%d,%d) => V(%d)(%d,%d,%d)\n",
					uiServiceId, gTCCDxBProc.uiSubVideoTotalCount, gTCCDxBProc.puiVideoSubPID[0], gTCCDxBProc.puiVideoSubPID[1], gTCCDxBProc.puiVideoSubPID[2], uiVideoTotalCount, puiVideoPID[0], puiVideoPID[1], puiVideoPID[2]);

			int bChange_SubVideoPID = 0;

			if (gTCCDxBProc.uiVideoIndex == gTCCDxBProc.uiVideoSubIndex && puiVideoPID[gTCCDxBProc.uiVideoIndex] >= 0x1FFF) //if the audio is disappeared
			{
				gTCCDxBProc.uiVideoSubIndex = !gTCCDxBProc.uiVideoIndex;
				bChange_SubVideoPID = 1;

			}
			else if (gTCCDxBProc.uiVideoIndex != gTCCDxBProc.uiVideoSubIndex && puiVideoPID[gTCCDxBProc.uiVideoIndex] < 0x1FFF) //if the audio is detected
			{
				gTCCDxBProc.uiVideoSubIndex = gTCCDxBProc.uiVideoIndex;
				bChange_SubVideoPID = 1;
			}
			else if(gTCCDxBProc.puiVideoSubPID[gTCCDxBProc.uiVideoSubIndex] != puiVideoPID[gTCCDxBProc.uiVideoSubIndex] && puiVideoPID[gTCCDxBProc.uiVideoSubIndex] < 0x1FFF) //if the audio is changed
			{
				bChange_SubVideoPID = 1;
			}

			if (bChange_SubVideoPID == 1)
			{
				tcc_manager_video_stop(1);
				tcc_manager_demux_change_videoPID(0, gTCCDxBProc.puiVideoPID[gTCCDxBProc.uiVideoMainIndex], gTCCDxBProc.puiVideoStreamType[gTCCDxBProc.uiVideoMainIndex], \
													1, puiVideoPID[gTCCDxBProc.uiVideoSubIndex], puiVideoStreamType[gTCCDxBProc.uiVideoSubIndex]);
				if(TCCDxBProc_GetMainState() == TCCDXBPROC_STATE_AIRPLAY){
					if(!TCC_Isdbt_IsAudioOnlyMode()){
						if(tcc_dxb_proc_get_videoonoff_value() == 1){
							tcc_manager_video_start(1, puiVideoStreamType[gTCCDxBProc.uiVideoSubIndex]);
						}
					}
				}
			}

			gTCCDxBProc.uiSubVideoTotalCount = uiVideoTotalCount;
			for (i = 0; i < TCCDXBPROC_VIDEO_MAX_NUM; ++i)
			{
				gTCCDxBProc.puiVideoSubPID[i]			= puiVideoPID[i];
				gTCCDxBProc.puiVideoSubStreamType[i]	= puiVideoStreamType[i];
			}

			bChange_SubVideo = 1;
		}

		if ((uiStPID > 2 && uiStPID != gTCCDxBProc.uiStSubPID)
			|| (uiSiPID > 2 && uiSiPID != gTCCDxBProc.uiSiSubPID))
		{
			ALOGD("[PMT-Changed] SubCaption ID(%d) S(%d,%d) => St(%d,%d)\n", uiServiceId, gTCCDxBProc.uiStSubPID, gTCCDxBProc.uiSiSubPID, uiStPID, uiSiPID);

			uiSegType = (gTCCDxBProc.uiPmtSubPID >= 0x1FC8) ? 1:13;

			if(tcc_manager_demux_get_playing_serviceID() == uiServiceId){
			    tcc_manager_demux_set_eit_subtitleLangInfo_clear();
			    if (uiSegType == 13) subtitle_system_set_stc_index(0);
			    else subtitle_system_set_stc_index(1);
				if(TCCDxBProc_GetMainState() == TCCDXBPROC_STATE_AIRPLAY)
					tcc_manager_demux_subtitle_play(uiStPID, uiSiPID, uiSegType, gTCCDxBProc.uiCountrycode, gTCCDxBProc.uiRaw_w, gTCCDxBProc.uiRaw_h, gTCCDxBProc.uiView_w, gTCCDxBProc.uiView_h, 0);
			}

			gTCCDxBProc.uiStSubPID = uiStPID;
			gTCCDxBProc.uiSiSubPID = uiSiPID;

			bChange_SubCaption = 1;
		}

		{
			ISDBT_Get_ComponentTagInfo(&gTCCDxBProc2.uiSubComponentTagCount, &gTCCDxBProc2.puiSubComponentTag[0]);

			#if 0
			int k;

			printf("[COMPTAG][S](%d) - ", gTCCDxBProc2.uiSubComponentTagCount);

			for (k = 0; k < gTCCDxBProc2.uiSubComponentTagCount; ++k)
				printf("%02X ", gTCCDxBProc2.puiSubComponentTag[k]);

			printf("\n\n");
			#endif
		}
	}
	else
	{
		if (tcc_db_get_channel_data_ForChannel(&ch_data, gTCCDxBProc.uiChannel, uiServiceId) >= 0)
		{
			if ((uiAudioTotalCount > 0 && puiAudioPID[0] != ch_data.uiAudioPID)
				|| (uiVideoTotalCount > 0  && puiVideoPID[0] != ch_data.uiVideoPID)
				|| (uiStPID > 2 && uiStPID != ch_data.uiStPID)
				|| (uiSiPID > 2 && uiSiPID != ch_data.uiSiPID))
			{
				ALOGD("[PMT-Change] Others ID(%d) A(%d)V(%d)S(%d,%d) => A(%d)V(%d)S(%d,%d)\n",
						uiServiceId, ch_data.uiAudioPID, ch_data.uiVideoPID, ch_data.uiStPID, ch_data.uiSiPID, puiAudioPID[0], puiVideoPID[0], uiStPID, uiSiPID);

				bChange_PID = 1;
			}

			{
				ISDBT_Get_ComponentTagInfo(&gTCCDxBProc2.uiComponentTagCount, &gTCCDxBProc2.puiComponentTag[0]);

				#if 0
				int k;
				printf("[COMPTAG][M](%d) - ", gTCCDxBProc2.uiComponentTagCount);

				for (k = 0; k < gTCCDxBProc2.uiComponentTagCount); ++k)
					printf("%02X ", gTCCDxBProc2.puiComponentTag[k]);

				printf("\n\n");
				#endif
			}
		}
	}

	/*========================================================================*/
	//	Update if any changes
	/*========================================================================*/
	if (bChange_Audio || bChange_Video || bChange_Caption
		||bChange_SubAudio || bChange_SubVideo || bChange_SubCaption
		|| bChange_PID)
	{
		UpdateDB_Channel_SQL_Partially(gTCCDxBProc.uiChannel, gTCCDxBProc.uiCountrycode, uiServiceId);
		error = true;
	}

	TCCDxBProc_ClearSubState(TCCDXBPROC_AIRPLAY_PMTUPDATE);

	TCCDxBEvent_ChannelUpdate (NULL);

	tcc_dxb_proc_lock_off();

	return error;
}

int TCCDxBProc_SetDualChannel(unsigned int uiChannelIndex)
{
	int uiSegType;
	unsigned int uiPlayingServiceID = 0;

	if (uiChannelIndex > TCCDXBPROC_SUB) {
		ALOGE("In %s, invalid parameter\n", __func__);

		return TCCDXBPROC_ERROR_INVALID_PARAM;
	}

	ALOGE("TCCDxBProc_SetDualChannel(%d)\n", uiChannelIndex);

	tcc_dxb_proc_lock_on();

	if (TCCDxBProc_GetMainState() != TCCDXBPROC_STATE_AIRPLAY) {
		ALOGE("[%s] Error invalid state(0x%X)",__func__, TCCDxBProc_GetState());
		tcc_dxb_proc_lock_off();

		return TCCDXBPROC_ERROR_INVALID_STATE;
	}

	if((TCCDxBProc_GetSubState() & TCCDXBPROC_AIRPLAY_INFORMAL_SCAN)) {
		ALOGE("[%s] Error invalid state(0x%X)",__func__, TCCDxBProc_GetSubState());
		tcc_dxb_proc_lock_off();

		return TCCDXBPROC_ERROR_INVALID_STATE;
	}

	if (gTCCDxBProc.uiServiceType == TCCDXBPROC_SERVICE_TYPE_1SEG) {
		ALOGE("In %s, only 1seg stream\n", __func__);
		tcc_dxb_proc_lock_off();

		return TCCDXBPROC_ERROR_INVALID_STATE;
	}

	tcc_manager_demux_set_dualchannel(uiChannelIndex);
	if (uiChannelIndex == TCCDXBPROC_MAIN) {
		tcc_manager_demux_set_serviceID(gTCCDxBProc.uiServiceID);
		tcc_manager_demux_set_playing_serviceID(gTCCDxBProc.uiServiceID);
		//tcc_manager_demux_avplay(-1, -1, -1, -1, -1, -1, -1, -1, gTCCDxBProc.uiPcrPID, gTCCDxBProc.uiPcrSubPID,-1, -1, 0);

		uiSegType = (gTCCDxBProc.uiPmtPID >= 0x1fc8)? 1 : 13;
		//IM238A-53, When switching of 1seg and 12-segment, the H/W layer of lower layer becomes visible only for a moment.
		subtitle_system_set_stc_index(0);
		tcc_manager_demux_subtitle_play (gTCCDxBProc.uiStPID, gTCCDxBProc.uiSiPID, uiSegType, gTCCDxBProc.uiCountrycode,\
				gTCCDxBProc.uiRaw_w, gTCCDxBProc.uiRaw_h, gTCCDxBProc.uiView_w, gTCCDxBProc.uiView_h, 0);
		gTCCDxBProc.uiCurrDualDecodeMode = TCCDXBPROC_MAIN;

		tcc_manager_audio_select_output(TCCDXBPROC_MAIN, true);
		tcc_manager_audio_select_output(TCCDXBPROC_SUB, false);
	} else {
		tcc_manager_demux_set_serviceID(gTCCDxBProc.uiServiceSubID);
		tcc_manager_demux_set_playing_serviceID(gTCCDxBProc.uiServiceSubID);
		//tcc_manager_demux_avplay(-1, -1, -1, -1, -1, -1, -1, -1, gTCCDxBProc.uiPcrPID, gTCCDxBProc.uiPcrSubPID, -1, -1, 1);

		uiSegType = (gTCCDxBProc.uiPmtSubPID >= 0x1fc8)? 1 : 13;
		//IM238A-53, When switching of 1seg and 12-segment, the H/W layer of lower layer becomes visible only for a moment.
		subtitle_system_set_stc_index(1);
		tcc_manager_demux_subtitle_play (gTCCDxBProc.uiStSubPID, gTCCDxBProc.uiSiSubPID, uiSegType, gTCCDxBProc.uiCountrycode, \
				gTCCDxBProc.uiRaw_w, gTCCDxBProc.uiRaw_h, gTCCDxBProc.uiView_w, gTCCDxBProc.uiView_h, 0);
		gTCCDxBProc.uiCurrDualDecodeMode = TCCDXBPROC_SUB;

		tcc_manager_audio_select_output(TCCDXBPROC_SUB, true);
		tcc_manager_audio_select_output(TCCDXBPROC_MAIN, false);
	}

	iDual_mode = uiChannelIndex;

	tcc_manager_video_select_display_output(uiChannelIndex);
	if(TCC_Isdbt_IsSupportCheckServiceID()){
		uiPlayingServiceID = tcc_manager_demux_get_playing_serviceID();
		tcc_manager_demux_serviceInfo_comparison(uiPlayingServiceID);
	}
//	tcc_manager_audio_select_output(uiChannelIndex, true);

	tcc_dxb_proc_lock_off();

	return TCCDXBPROC_SUCCESS;
}

int TCCDxBProc_ChannelBackUp(void)
{
	int	err = 0;

	//independent function about state

	ALOGD("[TCCDxBProc_ChannelBackUp] \n");
	err =  tcc_manager_demux_channeldb_backup();

	return err;
}

int TCCDxBProc_ChannelDBRestoration(void)
{
	int	err = 0;

	//independent function about state

	ALOGD("[TCCDxBProc_ChannelDBRestoration] \n");
	err =  tcc_manager_demux_channeldb_restoration();

	return err;
}

/**
 * @brief
 *
 * @param
 *
 * @return
 *     0 : TCCDXBPROC_SUCCESS
 *    -1 : TCCDXBPROC_ERROR. Fail to put 'SCAN' command into proc queue.
 *    -2 : TCCDXBPROC_ERROR_INVALID_STATE
 */
int TCCDxBProc_Scan(int scan_type, int country_code, int area_code, int channel_num, int options)
{
	unsigned int *puiArg;
	ST_DXBPROC_MSG *pProcMsg;

	ALOGD("[TCCDxBProc_Scan] %d, %d, %d, %d, %d\n", scan_type, country_code, area_code, channel_num, options);

	tcc_dxb_proc_lock_on();

	if(TCCDxBProc_GetMainState() != TCCDXBPROC_STATE_LOAD)
	{
		/* Noah / 20170829 / Add if statement for IM478A-30( (Improvement No3) At a weak signal, improvement of "searchAndSetChannel" )
			J&K's request
				1. searchAndSetChannel is called
				2. searchChannel is not called
				3. customRelayStationSearchRequest is called while searchAndSetChannel
				4. customRelayStationSearchRequest should work well without any problem
		*/
		if (TCCDxBProc_GetMainState() == TCCDXBPROC_STATE_AIRPLAY &&
			TCCDxBProc_GetSubState() == TCCDXBPROC_AIRPLAY_INFORMAL_SCAN &&
			scan_type == CUSTOM_SCAN)
		{

		}
		else
		{
			ALOGE("[%s] Error invalid state(0x%X)",__func__, TCCDxBProc_GetState());
			tcc_dxb_proc_lock_off();

			return TCCDXBPROC_ERROR_INVALID_STATE;
		}
	}

	pProcMsg = (ST_DXBPROC_MSG*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_MSG));
	puiArg = (unsigned int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(unsigned int) * 5);

	puiArg[0] = scan_type;
	puiArg[1] = country_code;
	puiArg[2] = area_code;
	puiArg[3] = channel_num;
	puiArg[4] = options;

	pProcMsg->cmd = DXBPROC_MSG_SCAN;
	pProcMsg->arg = (void *)puiArg;

	if(tcc_dxb_proc_scanrequest_get() == 0)
	{
		/* Noah / 20170829 / Add if statement for IM478A-30( (Improvement No3) At a weak signal, improvement of "searchAndSetChannel" )
			J&K's request
				1. searchAndSetChannel is called
				2. searchChannel is not called
				3. customRelayStationSearchRequest is called while searchAndSetChannel
				4. customRelayStationSearchRequest should work well without any problem
		*/
		if (TCCDxBProc_GetMainState() == TCCDXBPROC_STATE_AIRPLAY &&
			TCCDxBProc_GetSubState() == TCCDXBPROC_AIRPLAY_INFORMAL_SCAN)
		{

		}
		else
		{
			tcc_manager_tuner_scanflag_init();
			tcc_manager_demux_scanflag_init();
		}
	}

	if(tcc_dxb_proc_msg_send((void *)pProcMsg))
	{
		if(pProcMsg->arg)
		{
			tcc_mw_free(__FUNCTION__, __LINE__, pProcMsg->arg);
		}

		tcc_mw_free(__FUNCTION__, __LINE__, pProcMsg);

		tcc_dxb_proc_lock_off();

		return TCCDXBPROC_ERROR;
	}

	tcc_dxb_proc_scanrequest_add();

	TCCDxBProc_SetState(TCCDXBPROC_STATE_SCAN);

	tcc_dxb_proc_lock_off();

	return TCCDXBPROC_SUCCESS;
}

int TCCDxBProc_ScanAndSet(int country_code, int channel_num, int tsid, int options, int audioIndex, int videoIndex, int audioMode, int raw_w, int raw_h, int view_w, int view_h, int ch_index)
{
	unsigned int *puiArg;
	ST_DXBPROC_MSG *pProcMsg;

	ALOGD("[TCCDxBProc_ScanAndSet] %d, %d, %d, %d\n", country_code, channel_num, tsid, options);

	tcc_dxb_proc_lock_on();

	if((TCCDxBProc_GetMainState() != TCCDXBPROC_STATE_LOAD)
	    && (TCCDxBProc_GetMainState() != TCCDXBPROC_STATE_AIRPLAY)) {
		ALOGE("[%s] Error invalid state(0x%X)",__func__, TCCDxBProc_GetState());
		tcc_dxb_proc_lock_off();

		return TCCDXBPROC_ERROR_INVALID_STATE;
	}

	tcc_dxb_proc_msg_clear();

	ISDBT_Init_DescriptorInfo();

	pProcMsg = (ST_DXBPROC_MSG*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_MSG));
	puiArg = (unsigned int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(unsigned int) * 12);
	puiArg[0] = country_code;
	puiArg[1] = channel_num;
	puiArg[2] = tsid;
	puiArg[3] = options;
	puiArg[4] = audioIndex;
	puiArg[5] = videoIndex;
	puiArg[6] = audioMode;
	puiArg[7] = raw_w;
	puiArg[8] = raw_h;
	puiArg[9] = view_w;
	puiArg[10] = view_h;
	puiArg[11] = ch_index;

	pProcMsg->cmd = DXBPROC_MSG_SCAN_AND_SET;
	pProcMsg->arg = (void *)puiArg;
	if(tcc_dxb_proc_msg_send((void *)pProcMsg)) {
		if(pProcMsg->arg) {
			tcc_mw_free(__FUNCTION__, __LINE__, pProcMsg->arg);
		}
		tcc_mw_free(__FUNCTION__, __LINE__, pProcMsg);

		tcc_dxb_proc_lock_off();

		return TCCDXBPROC_ERROR;
	}

	if(TCCDxBProc_GetMainState() == TCCDXBPROC_STATE_LOAD) {
		TCCDxBProc_SetState(TCCDXBPROC_STATE_AIRPLAY);
	}

	tcc_dxb_proc_lock_off();

	return TCCDXBPROC_SUCCESS;
}

int TCCDxBProc_ScanOneChannel(int channel_num)
{
	ALOGD("In %s\n", __func__);

	//tcc_dxb_proc_lock_on();

	if(TCCDxBProc_GetMainState() != TCCDXBPROC_STATE_LOAD) {
		ALOGE("[%s] Error invalid state(0x%X)",__func__, TCCDxBProc_GetState());
		//tcc_dxb_proc_lock_off();

		return TCCDXBPROC_ERROR_INVALID_STATE;
	}

	TCCDxBProc_SetState(TCCDXBPROC_STATE_SCAN);

	tcc_manager_demux_set_state(E_DEMUX_STATE_SCAN);

	if (TCC_Isdbt_IsSupportBrazil()) {
		tcc_manager_demux_set_parentlock(0);
	}

	ResetSIVersionNumber();

	tcc_manager_tuner_scan_manual(channel_num, gTCCDxBProc.uiCountrycode, 1);

	TCCDxBProc_SetState(TCCDXBPROC_STATE_LOAD);

	//tcc_dxb_proc_lock_off();

	return TCCDXBPROC_SUCCESS;
}

#if 0   // sunny : not use
int TCCDxBProc_StartCapture(unsigned char *pucFile)
{
	int error = 0;

	tcc_dxb_proc_lock_on();

	if(TCCDxBProc_GetMainState() != TCCDXBPROC_STATE_AIRPLAY) {
		ALOGE("[%s] Error invalid state(0x%X)",__func__, TCCDxBProc_GetState());
		tcc_dxb_proc_lock_off();

		return TCCDXBPROC_ERROR_INVALID_STATE;
	}

	error = tcc_manager_video_capture(0, pucFile);

	tcc_dxb_proc_lock_off();

	return error;
}
#endif

int TCCDxBProc_GetTunerStrength(int *sqinfo)
{
	int error = 0;

	tcc_dxb_proc_lock_on();

	if(TCCDxBProc_GetMainState() == TCCDXBPROC_STATE_HANDOVER) {
		int	*tmp = (int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(int) * 256);
		error = tcc_manager_tuner_get_strength(tmp);
		tcc_mw_free(__FUNCTION__, __LINE__, tmp);
	}
	else
	{
		error = tcc_manager_tuner_get_strength(sqinfo);
	}

	if(error) {
		ALOGE("[%s] Error tcc_manager_tuner_get_strength(%d)",__func__, error);

		error = tcc_manager_tuner_reset(gTCCDxBProc.uiCountrycode, gTCCDxBProc.uiChannel);
		if(error) {
			ALOGE("[%s] Error tcc_manager_tuner_reset(%d)",__func__, error);
		}
	}

	tcc_dxb_proc_lock_off();

	return error;
}

/**
 * @brief
 *
 * @param
 *
 * @return
 *     0 : TCCDXBPROC_SUCCESS
 *    -2 : TCCDXBPROC_ERROR_INVALID_STATE
 */
int TCCDxBProc_StopScan(void)
{
	int err = TCCDXBPROC_SUCCESS;
	int scan_request;

	tcc_dxb_proc_lock_on();

	if(TCCDxBProc_GetMainState() != TCCDXBPROC_STATE_SCAN)
	{
		/* Noah / 20170829 / Add if statement for IM478A-30( (Improvement No3) At a weak signal, improvement of "searchAndSetChannel" )
			J&K's request
				1. searchAndSetChannel is called
				2. searchChannel is not called
				3. customRelayStationSearchRequest is called while searchAndSetChannel
				4. customRelayStationSearchRequest should work well without any problem
		*/
		if (TCCDxBProc_GetMainState() == TCCDXBPROC_STATE_AIRPLAY &&
			TCCDxBProc_GetSubState() == TCCDXBPROC_AIRPLAY_INFORMAL_SCAN)
		{
			tcc_manager_tuner_scan_cancel();
			tcc_manager_demux_scan_cancel();
		}
		else
		{
			ALOGE("[%s] Error invalid state(0x%X)",__func__, TCCDxBProc_GetState());
			tcc_dxb_proc_lock_off();

			return TCCDXBPROC_ERROR_INVALID_STATE;
		}
	}

	scan_request = tcc_dxb_proc_scanrequest_get();
	if (scan_request)
	{
		err |= tcc_manager_tuner_scan_cancel();
		err |= tcc_manager_demux_scan_cancel();
	}

	tcc_dxb_proc_lock_off();

	if (!scan_request)
	{
		tcc_manager_tuner_scan_cancel_notify();
	}

	return err;
}

int TCCDxBProc_SetArea(unsigned int uiArea)
{
	if(TCCDxBProc_GetMainState() != TCCDXBPROC_STATE_LOAD) {
		ALOGE("[%s] Error invalid state(0x%X)",__func__, TCCDxBProc_GetState());
		return TCCDXBPROC_ERROR_INVALID_STATE;
	}

	tcc_manager_demux_set_area(uiArea);

	return TCCDXBPROC_SUCCESS;
}

int TCCDxBProc_SetPreset (int area_code)
{
	tcc_dxb_proc_lock_on();

	if(TCCDxBProc_GetMainState() != TCCDXBPROC_STATE_LOAD) {
		ALOGE("[%s] Error invalid state(0x%X)",__func__, TCCDxBProc_GetState());
		tcc_dxb_proc_lock_off();

		return TCCDXBPROC_ERROR_INVALID_STATE;
	}

	tcc_manager_demux_set_preset(area_code);

	tcc_dxb_proc_lock_off();

	return TCCDXBPROC_SUCCESS;
}

int TCCDxBProc_ChangeAudio(int iAudioIndex, int iAudioMainIndex, unsigned int uiAudioMainPID, int iAudioSubIndex, unsigned int uiAudioSubPID)
{
	int error = 0;
	int	fChangeAudio[2] = { 0, 0};

	tcc_dxb_proc_lock_on();

	//independent function about state

	gTCCDxBProc.uiAudioIndex = iAudioIndex;

	if (iAudioMainIndex == -1 && iAudioSubIndex == -1) {
		ALOGD("In %s, any audio is not valid", __func__);
		tcc_dxb_proc_lock_off();
		return -1;
	}

	if (iAudioMainIndex != -1 && (uiAudioMainPID != 0x1FFF)) {
		gTCCDxBProc.uiAudioMainIndex = iAudioMainIndex;
		gTCCDxBProc.puiAudioPID[iAudioMainIndex] = uiAudioMainPID;
		fChangeAudio[0] = 1;
	} else
		fChangeAudio[0] = 0;

	if (iAudioSubIndex != -1 && (uiAudioSubPID != 0x1FFF)) {
		gTCCDxBProc.uiAudioSubIndex = iAudioSubIndex;
		gTCCDxBProc.puiAudioSubPID[iAudioSubIndex] = uiAudioSubPID;
		fChangeAudio[1] = 1;
	} else
		fChangeAudio[1] = 0;

	if (fChangeAudio[0])	tcc_manager_audio_stop(0);
	if (fChangeAudio[1])	tcc_manager_audio_stop(1);
	tcc_manager_demux_change_audioPID(fChangeAudio[0], uiAudioMainPID, fChangeAudio[1], uiAudioSubPID);
	if (fChangeAudio[0])	tcc_manager_audio_start(0, gTCCDxBProc.puiAudioStreamType[gTCCDxBProc.uiAudioMainIndex]);
	if (fChangeAudio[1])	tcc_manager_audio_start(1, gTCCDxBProc.puiAudioSubStreamType[gTCCDxBProc.uiAudioSubIndex]);
	if(gTCCDxBSeamless.iOnoff == 1){
		ALOGE("[DFS] RE-START\n");
		tcc_manager_audio_setSeamlessSwitchCompensation(gTCCDxBSeamless.iOnoff, gTCCDxBSeamless.iInterval, gTCCDxBSeamless.iStrength,
			gTCCDxBSeamless.iNtimes, gTCCDxBSeamless.iRange, gTCCDxBSeamless.iGapadjust, gTCCDxBSeamless.iGapadjust2, gTCCDxBSeamless.iMultiplier);
	}

	tcc_dxb_proc_lock_off();

	return error;
}


int TCCDxBProc_ChangeVideo(int iVideoIndex, int iVideoMainIndex, unsigned int uiVideoMainPID, int iVideoSubIndex, unsigned int uiVideoSubPID)
{
	int error = 0;
	int	fChangeVideo[2] = { 0, 0};

	tcc_dxb_proc_lock_on();

	//independent function about state

	gTCCDxBProc.uiVideoIndex = iVideoIndex;

	if (iVideoMainIndex == -1 && iVideoSubIndex == -1) {
		ALOGD("In %s, any video is not valid", __func__);
		tcc_dxb_proc_lock_off();
		return error;
	}

	if (iVideoMainIndex != -1 && (uiVideoMainPID != 0x1FFF)) {
		gTCCDxBProc.uiVideoMainIndex = iVideoMainIndex;
		gTCCDxBProc.puiVideoPID[iVideoMainIndex] = uiVideoMainPID;
		fChangeVideo[0] = 1;
	} else
		fChangeVideo[0] = 0;

	if (iVideoSubIndex != -1 && (uiVideoSubPID != 0x1FFF)) {
		gTCCDxBProc.uiVideoSubIndex = iVideoSubIndex;
		gTCCDxBProc.puiVideoSubPID[iVideoSubIndex] = uiVideoSubPID;
		fChangeVideo[1] = 1;
	} else
		fChangeVideo[1] = 0;

	if (fChangeVideo[0]) tcc_manager_video_stop(0);
	if (fChangeVideo[1]) tcc_manager_video_stop(1);
	tcc_manager_demux_change_videoPID(fChangeVideo[0], gTCCDxBProc.puiVideoPID[gTCCDxBProc.uiVideoMainIndex], gTCCDxBProc.puiVideoStreamType[gTCCDxBProc.uiVideoMainIndex], \
										fChangeVideo[1], gTCCDxBProc.puiVideoSubPID[gTCCDxBProc.uiVideoSubIndex], gTCCDxBProc.puiVideoSubStreamType[gTCCDxBProc.uiVideoSubIndex]);
	if(!TCC_Isdbt_IsAudioOnlyMode()){
		if(tcc_dxb_proc_get_videoonoff_value() == 1){
			if (fChangeVideo[0]) tcc_manager_video_start(0, gTCCDxBProc.puiVideoStreamType[gTCCDxBProc.uiVideoMainIndex]);
			if (fChangeVideo[1]) tcc_manager_video_start(1, gTCCDxBProc.puiVideoSubStreamType[gTCCDxBProc.uiVideoSubIndex]);
		}
	}

	tcc_dxb_proc_lock_off();

	return error;
}


int TCCDxBProc_SetAudioDualMono(unsigned int uiAudioMode)
{
	int error = -1;

	tcc_dxb_proc_lock_on();

	//independent function about state

	error = tcc_manager_audio_set_dualmono(0, uiAudioMode);

	tcc_dxb_proc_lock_off();

	return error;
}

int TCCDxBProc_RequestEPGUpdate(int iDayOffset)
{
	int error = 0;

	tcc_dxb_proc_lock_on();

	//independent function about state

	//tcc_db_commit_epg(iDayOffset, gTCCDxBProc.uiChannel, gTCCDxBProc.uiNetworkID);

	tcc_dxb_proc_lock_off();

	return error;
}

/* This function have to call after only PMT has upated. */
int TCCDxBProc_CheckPCRChg(void)
{
	unsigned int iPcrPid=0, iServiceId=0;

	//independent function about state

	ISDBT_Get_ServiceInfo(&iPcrPid, &iServiceId);

	tcc_dxb_proc_lock_on();

	if((gTCCDxBProc.uiServiceID == iServiceId) && (gTCCDxBProc.uiPcrPID != iPcrPid))
	{
		ALOGW("PCR changed(0x%X -> 0x%X)\n", gTCCDxBProc.uiPcrPID, iPcrPid);
		tcc_manager_demux_stop_pcr_pid(0);
		tcc_manager_demux_start_pcr_pid(iPcrPid, 0);
		gTCCDxBProc.uiPcrPID = iPcrPid;
	}
	else if((gTCCDxBProc.uiServiceSubID == iServiceId) && (gTCCDxBProc.uiPcrSubPID != iPcrPid))
	{
		ALOGW("PCR changed(0x%X -> 0x%X)\n", gTCCDxBProc.uiPcrSubPID, iPcrPid);
		tcc_manager_demux_stop_pcr_pid(1);
		tcc_manager_demux_start_pcr_pid(iPcrPid, 1);
		gTCCDxBProc.uiPcrSubPID = iPcrPid;
	}

	tcc_dxb_proc_lock_off();

	return TCCDXBPROC_SUCCESS;
}

void TCCDxBProc_ReqTRMPInfo(unsigned char **ppInfo, int *piInfoSize)
{
	tcc_dxb_proc_lock_on();

	//independent function about state

	tcc_manager_cas_getInfo((void**)ppInfo, piInfoSize);

	tcc_dxb_proc_lock_off();

   	return;
}

void TCCDxBProc_ResetTRMPInfo(void)
{
	tcc_dxb_proc_lock_on();

	//independent function about state

	tcc_manager_cas_resetInfo();

	tcc_dxb_proc_lock_off();

   	return;
}

int TCCDxBProc_InitVideoSurface(int arg)
{
    int error=0;
    error = tcc_manager_video_initSurface(arg);
    return error;
}

int TCCDxBProc_DeinitVideoSurface(void)
{
    int error=0;
    error = tcc_manager_video_deinitSurface();
    return error;
}


int TCCDxBProc_SetVideoSurface(void *nativeWidow)
{
    int error=0;
    error = tcc_manager_video_setSurface(nativeWidow);
    return error;
}

int TCCDxBProc_VideoUseSurface(void)
{
    int error=0;
    error = tcc_manager_video_useSurface();
    return error;
}

int TCCDxBProc_VideoReleaseSurface(void)
{
    int error=0;
    error = tcc_manager_video_releaseSurface();
    return error;
}

void TCCDxBProc_GetCurrentChannel(int *piChannelNumber, int *piServiceID, int *piServiceSubID)
{
	//independent function about state

	*piChannelNumber = gTCCDxBProc.uiChannel;
	*piServiceID = gTCCDxBProc.uiServiceID;
	*piServiceSubID = gTCCDxBProc.uiServiceSubID;

	return;
}

int TCCDxbProc_DecodeLogoImage
(
	unsigned char *pucLogoStream,
	int iLogoStreamSize,
	unsigned int *pusLogoImage,
	int *piLogoImageWidth,
	int *piLogoImageHeight
)
{
   	int error=0;

	//independent function about state

	*piLogoImageWidth = 0;
	*piLogoImageHeight = 0;

	if (iLogoStreamSize == 0) {
		//there is no logo
		return error;
	}

	else if (iLogoStreamSize < 0) {
		ALOGE("[%s] Error invalid logo data",__func__);
		return TCCDXBPROC_ERROR_INVALID_PARAM;
	}

	error = tcc_isdbt_png_init(pucLogoStream, iLogoStreamSize,
							(unsigned int*)piLogoImageWidth,
							(unsigned int*)piLogoImageHeight);
	if (error) {
		ALOGE("[%s] Error tcc_isdbt_png_init(%d)",__func__, error);
		return error;
	}

	error = tcc_isdbt_png_dec(pucLogoStream, iLogoStreamSize,
							pusLogoImage, *piLogoImageWidth, 0, 0,
							*piLogoImageWidth, *piLogoImageHeight,
							ISDBT_PNGDEC_ARGB8888, gTCCDxBProc.uiCountrycode, 1);
	if (error) {
		ALOGE("[%s] Error tcc_isdbt_png_dec(%d)",__func__, error);
		return error;
	}

	return error;
}

int TCCDxBProc_SetFreqBand (int freq_band)
{
	int error = 0;

	tcc_dxb_proc_lock_on();

	//independent function about state

	error = tcc_manager_tuner_set_FreqBand(freq_band);

	tcc_dxb_proc_lock_off();

	return error;
}

int TCCDxBProc_SetFieldLog (char *sPath, int fOn_Off)
{
	char *psArg;
	ST_DXBPROC_MSG *pProcMsg;

	tcc_dxb_proc_lock_on();

	//independent function about state

	pProcMsg = (ST_DXBPROC_MSG*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_MSG));
	psArg = (char*)tcc_mw_malloc(__FUNCTION__, __LINE__, 130);
	psArg[0] = (char)(fOn_Off ? 1 : 0);
	strncpy (&psArg[1], sPath, 127);
	pProcMsg->cmd = DXBPROC_MSG_FIELDLOG;
	pProcMsg->arg = (void *)psArg;

	if(tcc_dxb_proc_msg_send ((void*)pProcMsg)) {
		tcc_mw_free(__FUNCTION__, __LINE__, pProcMsg->arg);
		tcc_mw_free(__FUNCTION__, __LINE__, pProcMsg);
	}

	tcc_dxb_proc_lock_off();

	return TCCDXBPROC_SUCCESS;
}

int TCCDxBProc_SetPresetMode (int preset_mode)
{
	unsigned int *puiArg;
	ST_DXBPROC_MSG *pProcMsg;
       int error = TCCDXBPROC_SUCCESS;

	if((TCCDxBProc_GetMainState() != TCCDXBPROC_STATE_INIT)
	    && (TCCDxBProc_GetMainState() != TCCDXBPROC_STATE_LOAD)) {
	    ALOGE("[%s] Error invalid state(0x%X)",__func__, TCCDxBProc_GetState());

		return TCCDXBPROC_ERROR_INVALID_STATE;
	}

	if (gTCCDxBProc.uiMsgHandlerRunning == 1)
	{
		ALOGD("TCCDxBProc_SetPresetMode %d", preset_mode);

		tcc_dxb_proc_lock_on();

		pProcMsg = (ST_DXBPROC_MSG*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_MSG));
		puiArg = (unsigned int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(unsigned int));
		puiArg[0] = preset_mode;

		pProcMsg->cmd = DXBPROC_MSG_SETPRESETMODE;
		pProcMsg->arg = (void*)puiArg;

		if(tcc_dxb_proc_msg_send((void*)pProcMsg)) {
			if(pProcMsg->arg) {
				tcc_mw_free(__FUNCTION__, __LINE__, pProcMsg->arg);
			}
			tcc_mw_free(__FUNCTION__, __LINE__, pProcMsg);
		}

		tcc_dxb_proc_lock_off();
	}
	else {
		if(tcc_manager_demux_set_presetMode (preset_mode) == 0) {
			ALOGE("[%s] Error tcc_manager_demux_set_presetMode",__func__);
			error = TCCDXBPROC_ERROR;
		}
	 }

	return error;
}
int TCCDxBProc_SetCustomTuner(int size, void *arg)
{
	//independent function about state

	return tcc_manager_tuner_set_CustomTuner(size, arg);
}

int TCCDxBProc_GetCustomTuner(int *size, void *arg)
{
	//independent function about state

	return tcc_manager_tuner_get_CustomTuner(size, arg);
}

int TCCDxBProc_SetSectionNotificationIDs(int sectionIDs)
{
	return tcc_manager_demux_set_section_notification_ids(sectionIDs);
}

int TCCDxBProc_GetCur_DCCDescriptorInfo(unsigned short usServiceID, void **pDCCInfo)
{
	//independent function about state

	return tcc_manager_demux_get_curDCCDescriptorInfo(usServiceID, pDCCInfo);
}

int TCCDxBProc_GetCur_CADescriptorInfo(unsigned short usServiceID, unsigned char ucDescID, void *arg)
{
	//independent function about state

	return tcc_manager_demux_get_curCADescriptorInfo(usServiceID, ucDescID, arg);
}

int TCCDxBProc_GetComponentTag(int iServiceID, int index, unsigned char *pucComponentTag, int *piComponentCount)
{
	//independent function about state

	return tcc_manager_demux_get_componentTag(iServiceID, index, pucComponentTag, piComponentCount);
}

int TCCDxBProc_SetAudioMuteOnOff(unsigned int uiOnOff)
{
	if(uiOnOff == 1)
		return tcc_manager_audio_setframedropflag(0);
	else if(uiOnOff == 0)
		return tcc_manager_audio_setframedropflag(1);
	else
		return -1;
}

int TCCDxBProc_SetAudioOnOff(unsigned int uiOnOff)
{
	int state = 0;
	ALOGE("TCCDxBProc_SetAudioOnOff[%d]", uiOnOff);
	state = TCCDxBProc_GetMainState();

	if (state == TCCDXBPROC_STATE_AIRPLAY){
		if(uiOnOff == 0){
			//Alsa device close
			tcc_manager_audio_alsa_close(1, 1);
			tcc_manager_audio_alsa_close(0, 1);
			tcc_manager_audio_alsa_close_flag(1);
		}else{
			//Alsa device open
			tcc_manager_audio_alsa_close(0, 0);
			tcc_manager_audio_alsa_close(1, 0);
			tcc_manager_audio_alsa_close_flag(0);
		}
		TCCDxBEvent_AudioStartStop(uiOnOff);
	}else{
		ALOGE("TCCDxBProc_SetAudioOnOff error, state[%d]", state);
		TCCDxBEvent_AudioStartStop(0);
		return -1;
	}

	return 0;
}

int TCCDxBProc_SetVideoOnOff(unsigned int uiOnOff)
{
    int state = 0;
    ALOGE("TCCDxBProc_SetVideoOnOff[%d]", uiOnOff);
    state = TCCDxBProc_GetMainState();

    if (state == TCCDXBPROC_STATE_AIRPLAY){
        if(uiOnOff == 0){
            //stop video decode/relese & fb release
            tcc_manager_video_stop(1);
            tcc_manager_video_stop(0);
#if defined(HAVE_LINUX_PLATFORM)
            subtitle_display_wmixer_close();
#endif
            tcc_manager_video_deinitSurface();
        }else{
            //clear Audio only mode
            TCC_Isdbt_Clear_AudioOnlyMode();

			//re-start decode & fb init
            tcc_manager_video_initSurface(0);
            tcc_manager_video_start(0, gTCCDxBProc.puiVideoStreamType[gTCCDxBProc.uiVideoMainIndex]);
            tcc_manager_video_start(1, gTCCDxBProc.puiVideoSubStreamType[gTCCDxBProc.uiVideoSubIndex]);
#if defined(HAVE_LINUX_PLATFORM)
            subtitle_display_wmixer_open();
#endif
        }
        tcc_dxb_proc_set_videoonoff_value(uiOnOff);
        TCCDxBEvent_VideoStartStop(uiOnOff);
    }else{
        ALOGE("TCCDxBProc_SetVideoOutput error, state[%d]", state);
        TCCDxBEvent_VideoStartStop(0);
        return -1;
    }

    return 0;
}

int TCCDxBProc_SetVideoOutput(unsigned int uiOnOff)
{
	if(uiOnOff == 1)
		return tcc_manager_video_setframedropflag(0);
	else if(uiOnOff == 0)
		return tcc_manager_video_setframedropflag(1);
	else
		return -1;
}

int TCCDxBProc_SetAudioVideoSyncMode(unsigned int uiMode)
{
	ALOGE("%s:%d mode %d", __func__, __LINE__, uiMode);

	switch(uiMode) {
		case 0:
			{
				tcc_manager_video_setfirstframebypass(1);
				tcc_manager_video_setfirstframeafterseek(0);
				tcc_manager_audio_set_AudioStartSyncWithVideo(0);
				if(TCCDxBProc_GetMainState() == TCCDXBPROC_STATE_AIRPLAY) {
					tcc_manager_audio_mute(0, 0);
					tcc_manager_audio_mute(1, 0);
				}
			}
			break;
		case 1:
			{
				tcc_manager_video_setfirstframebypass(0);
				tcc_manager_video_setfirstframeafterseek(1);
				tcc_manager_audio_set_AudioStartSyncWithVideo(0);
				if(TCCDxBProc_GetMainState() == TCCDXBPROC_STATE_AIRPLAY) {
					tcc_manager_audio_mute(0, 0);
					tcc_manager_audio_mute(1, 0);
				}
			}
			break;
		case 2:
			{
				tcc_manager_video_setfirstframebypass(0);
				tcc_manager_video_setfirstframeafterseek(1);
				tcc_manager_audio_set_AudioStartSyncWithVideo(1);
			}
			break;
		default :
			ALOGE("%s:%d invalid mode(%d)", __func__, __LINE__, uiMode);
			break;
	}
	gTCCDxBProc.uiAVSyncMode = (unsigned int)uiMode;
	return 0;
}

int TCCDxBProc_GetVideoState()
{
	return tcc_manager_video_getdisplayedfirstframe();
}

int TCCDxBProc_GetSearchState()
{
	int iScanReqCnt = tcc_dxb_proc_scanrequest_get();
	if(iScanReqCnt >= 1)
		return 1;
	else
		return iScanReqCnt;
}

int TCCDxBProc_DataServiceStart(void)
{
	return tcc_manager_demux_dataservice_start();
}

int TCCDxBProc_CustomFilterStart(int iPID, int iTableID)
{
	return tcc_manager_demux_custom_filter_start(iPID, iTableID);
}

int TCCDxBProc_CustomFilterStop(int iPID, int iTableID)
{
	return tcc_manager_demux_custom_filter_stop(iPID, iTableID);
}

int TCCDxBProc_EWSStart(unsigned int uiChannel,\
						unsigned int uiAudioTotalCount, unsigned int *puiAudioPID,\
						unsigned int uiVideoTotalCount, unsigned int *puiVideoPID,\
						unsigned int *puiAudioStreamType, unsigned int *puiVideoStreamType,\
						unsigned int uiSubAudioTotalCount, unsigned int *puiAudioSubPID,\
						unsigned int uiSubVideoTotalCount, unsigned int *puiVideoSubPID,\
						unsigned int *puiAudioSubStreamType, unsigned int *puiVideoSubStreamType,\
					unsigned int uiPcrPID, unsigned int uiPcrSubPID,\
					unsigned int uiStPID, unsigned int uiSiPID, unsigned int uiServiceID, unsigned int uiPmtPID, \
					unsigned int uiCAECMPID, unsigned int uiACECMPID, unsigned int uiNetworkID, \
				   unsigned int uiStSubPID, unsigned int uiSiSubPID, unsigned int uiServiceSubID, unsigned int uiPmtSubPID, \
						unsigned int uiAudioIndex, unsigned int uiAudioMainIndex, unsigned int uiAudioSubIndex,\
						unsigned int uiVideoIndex, unsigned int uiVideoMainIndex, unsigned int uiVideoSubIndex,\
						unsigned int uiAudioMode, unsigned int service_type, \
					unsigned int uiRaw_w, unsigned int uiRaw_h, unsigned int uiView_w, unsigned int uiView_h, \
				   int ch_index)
{
	int i;
	unsigned int state;
	int	dual_mode;
	int	iLastChannel;

	tcc_dxb_proc_lock_on();

	state = TCCDxBProc_GetMainState();

	if(	state == TCCDXBPROC_STATE_LOAD
	||	state == TCCDXBPROC_STATE_AIRPLAY) {
		if (ch_index == 0)	{
			 dual_mode = TCCDXBPROC_MAIN;
		}
		else {
			dual_mode = TCCDXBPROC_SUB;
		}

		iDual_mode = dual_mode;

		if(TCCDxBProc_GetMainState() == TCCDXBPROC_STATE_AIRPLAY) {
			tcc_manager_audio_stop(1);
			tcc_manager_audio_stop(0);
			if(!TCC_Isdbt_IsAudioOnlyMode()){
				tcc_manager_video_stop(1);
				tcc_manager_video_stop(0);
			}
			tcc_manager_demux_av_stop(1, 1, 1, 1);
			tcc_manager_video_set_proprietarydata(-1);
			tcc_manager_demux_subtitle_stop(1);

#if defined(HAVE_LINUX_PLATFORM)
			if(!TCC_Isdbt_IsAudioOnlyMode()){
				subtitle_display_wmixer_close();
			}
#endif
			if(!TCC_Isdbt_IsAudioOnlyMode()){
				tcc_manager_video_deinitSurface();
			}
		}

		gTCCDxBProc.uiChannel = uiChannel;
		gTCCDxBProc.uiAudioTotalCount = uiAudioTotalCount;
		gTCCDxBProc.uiSubAudioTotalCount = uiSubAudioTotalCount;
		for(i=0; i<TCCDXBPROC_AUDIO_MAX_NUM; i++)
		{
			gTCCDxBProc.puiAudioPID[i] = puiAudioPID[i];
			gTCCDxBProc.puiAudioStreamType[i] = puiAudioStreamType[i];
			gTCCDxBProc.puiAudioSubPID[i] = puiAudioSubPID[i];
			gTCCDxBProc.puiAudioSubStreamType[i] = puiAudioSubStreamType[i];
		}
		gTCCDxBProc.uiVideoTotalCount = uiVideoTotalCount;
		gTCCDxBProc.uiSubVideoTotalCount = uiSubVideoTotalCount;
		for(i=0; i<TCCDXBPROC_VIDEO_MAX_NUM; i++)
		{
			gTCCDxBProc.puiVideoPID[i] = puiVideoPID[i];
			gTCCDxBProc.puiVideoStreamType[i] = puiVideoStreamType[i];
			gTCCDxBProc.puiVideoSubPID[i] = puiVideoSubPID[i];
			gTCCDxBProc.puiVideoSubStreamType[i] = puiVideoSubStreamType[i];
		}
		gTCCDxBProc.uiPcrPID = uiPcrPID;
		gTCCDxBProc.uiPcrSubPID = uiPcrSubPID;
		gTCCDxBProc.uiStPID = uiStPID;
		gTCCDxBProc.uiStSubPID = uiStSubPID;
		gTCCDxBProc.uiSiPID = uiSiPID;
		gTCCDxBProc.uiSiSubPID = uiSiSubPID;
		gTCCDxBProc.uiServiceID = uiServiceID;
		gTCCDxBProc.uiServiceSubID = uiServiceSubID;
		gTCCDxBProc.uiPmtPID = uiPmtPID;
		gTCCDxBProc.uiPmtSubPID = uiPmtSubPID;
		gTCCDxBProc.uiCAECMPID = uiCAECMPID;
		gTCCDxBProc.uiACECMPID = uiACECMPID;
		gTCCDxBProc.uiAudioIndex = uiAudioIndex;
		gTCCDxBProc.uiAudioMainIndex = uiAudioMainIndex;
		gTCCDxBProc.uiAudioSubIndex = uiAudioSubIndex;
		gTCCDxBProc.uiVideoIndex = uiVideoIndex;
		gTCCDxBProc.uiVideoMainIndex = uiVideoMainIndex;
		gTCCDxBProc.uiVideoSubIndex = uiVideoSubIndex;
		gTCCDxBProc.uiAudioMode = uiAudioMode;
		gTCCDxBProc.uiServiceType = service_type;
		gTCCDxBProc.uiNetworkID = uiNetworkID;
		gTCCDxBProc.uiRaw_w = uiRaw_w;
		gTCCDxBProc.uiRaw_h = uiRaw_h;
		gTCCDxBProc.uiView_w = uiView_w;
		gTCCDxBProc.uiView_h = uiView_h;

		if (TCC_Isdbt_IsSupportSpecialService())
		{
			if (service_type != SERVICE_TYPE_TEMP_VIDEO) {
				/* delete all information of special service if the selected service is regular */
				tcc_manager_demux_specialservice_DeleteDB();
			}
		}

		tcc_manager_demux_set_state(E_DEMUX_STATE_AIRPLAY);

		if (dual_mode == TCCDXBPROC_MAIN) {
			tcc_manager_demux_set_serviceID(uiServiceID);
			tcc_manager_demux_set_playing_serviceID(uiServiceID);
			tcc_manager_demux_set_first_playing_serviceID(uiServiceID);
		}
		else {
			tcc_manager_demux_set_serviceID(uiServiceSubID);
			tcc_manager_demux_set_playing_serviceID(uiServiceSubID);
			tcc_manager_demux_set_first_playing_serviceID(uiServiceSubID);
		}

		iLastChannel = tcc_manager_tuner_get_lastchannel();
		tcc_manager_demux_set_tunerinfo(uiChannel, 0, gTCCDxBProc.uiCountrycode);
		tcc_db_create_epg(uiChannel, uiNetworkID);
		tcc_manager_tuner_set_channel(uiChannel);
		if (TCC_Isdbt_IsSupportBrazil()){
			tcc_manager_demux_set_parentlock(1);
		}

#if 0
		ALOGE("%s %d brazil= %d , japan= %d \n", __func__, __LINE__, TCC_Isdbt_IsSupportBrazil(), TCC_Isdbt_IsSupportJapan());
#endif

		if (TCC_Isdbt_IsSupportBrazil()) {
			tcc_manager_video_issupport_country(0, TCCDXBPROC_BRAZIL);
			tcc_manager_audio_issupport_country(0, TCCDXBPROC_BRAZIL);
		}
		else if (TCC_Isdbt_IsSupportJapan()) {
			tcc_manager_video_issupport_country(0, TCCDXBPROC_JAPAN);
			tcc_manager_audio_issupport_country(0, TCCDXBPROC_JAPAN);
		}

		ResetSIVersionNumber();
		if(iLastChannel != uiChannel) {
			tcc_manager_demux_cas_init();		//This should be called after changing RF frequency.
		}

		TCCDxBProc_SetState(TCCDXBPROC_STATE_EWSMODE);
		TCCDxBProc_SetSubState(iDual_mode?TCCDXBPROC_EWSMODE_SUB:TCCDXBPROC_EWSMODE_MAIN);

		tcc_manager_demux_ews_start(gTCCDxBProc.uiPmtPID, gTCCDxBProc.uiPmtSubPID);

		tcc_dxb_proc_lock_off();
		return TCCDXBPROC_SUCCESS;
	}
	else {
		ALOGE("[%s] Error invalid state(0x%X)",__func__, state);
		tcc_dxb_proc_lock_off();

		return TCCDXBPROC_ERROR_INVALID_STATE;
	}
}

int TCCDxBProc_EWSClear(void)
{
	unsigned int subtitle_pid, superimpose_pid, uiSegType;
	unsigned int substate = TCCDxBProc_GetSubState();

	tcc_dxb_proc_lock_on();

    /*
        Noah, 2018-10-12, IM692A-15 / TVK does not recieve at all after 632 -> 631 of TVK.

            Precondition
                ISDBT_SINFO_CHECK_SERVICEID is enabled.

            Issue
                1. tcc_manager_audio_serviceID_disable_output(0) and tcc_manager_video_serviceID_disable_display(0) are called.
                    -> Because 632 is broadcasting in suspension, so, this is no problem.
                2. setChannel(_withChNum) or searchAndSetChannel is called to change to weak signal(1seg OK, Fseg No) channel.
                3. A/V is NOT output.
                    -> NG

            Root Cause
                The omx_alsasink_component_Private->audioskipOutput and pPrivateData->bfSkipRendering of previous channel are used in current channel.
                Refer to the following steps.

                1. In step 1 of the Issue above, omx_alsasink_component_Private->audioskipOutput and pPrivateData->bfSkipRendering are set to TRUE
                    because tcc_manager_audio_serviceID_disable_output(0) and tcc_manager_video_serviceID_disable_display(0) are called.
                2. tcc_manager_audio_serviceID_disable_output(1) and tcc_manager_video_serviceID_disable_display(1) have to be called for playing A/V.
                    To do so, DxB has to get PAT and then tcc_manager_demux_serviceInfo_comparison has to be called.
                    But, DxB can NOT get APT due to weak signal.

            Solution
                tcc_manager_audio_serviceID_disable_output(1) & tcc_manager_video_serviceID_disable_display(1) are called forcedly once as below.

                I think the following conditons should be default values. And '1' value of previous channel should not be used in current channel.
                    - omx_alsasink_component_Private->audioskipOutput == 0
                    - pPrivateData->bfSkipRendering == 0
                After that -> PAT -> If service ID is the same, A/V will be shown continually. Otherwise A/V will be blocked.

    */
    tcc_manager_demux_init_av_skip_flag();

	if(TCCDxBProc_GetMainState() != TCCDXBPROC_STATE_EWSMODE) {
		ALOGE("[%s] Error invalid state(0x%X)",__func__, TCCDxBProc_GetMainState());
		tcc_dxb_proc_lock_off();
		return TCCDXBPROC_ERROR_INVALID_STATE;
	}

	tcc_manager_demux_ews_clear();

	ResetSIVersionNumber();

	if(!TCC_Isdbt_IsAudioOnlyMode()){
		tcc_manager_video_initSurface(0);
	}

	tcc_manager_video_set_proprietarydata(gTCCDxBProc.uiChannel);

	TCCDxBProc_SetState(TCCDXBPROC_STATE_AIRPLAY);
	if (iDual_mode == TCCDXBPROC_MAIN)	{
		 TCCDxBProc_SetSubState(TCCDXBPROC_AIRPLAY_MAIN);
	}
	else {
		 TCCDxBProc_SetSubState(TCCDXBPROC_AIRPLAY_SUB);
	}

	/*----- logo -----*/
	tcc_manager_demux_logo_init();
	tcc_manager_demux_logo_get_infoDB(gTCCDxBProc.uiChannel, gTCCDxBProc.uiCountrycode, gTCCDxBProc.uiNetworkID, gTCCDxBProc.uiServiceID, gTCCDxBProc.uiServiceSubID);

	/*----- channel name -----*/
	if (tcc_manager_demux_is_skip_sdt_in_scan()) {
		tcc_manager_demux_channelname_init();
		tcc_manager_demux_channelname_get_infoDB(gTCCDxBProc.uiChannel, gTCCDxBProc.uiCountrycode, gTCCDxBProc.uiNetworkID, gTCCDxBProc.uiServiceID, gTCCDxBProc.uiServiceSubID);
	}

	tcc_manager_demux_av_play(gTCCDxBProc.puiAudioPID[gTCCDxBProc.uiAudioMainIndex], gTCCDxBProc.puiVideoPID[gTCCDxBProc.uiVideoMainIndex], \
							 gTCCDxBProc.puiAudioStreamType[gTCCDxBProc.uiAudioMainIndex], gTCCDxBProc.puiVideoStreamType[gTCCDxBProc.uiVideoMainIndex], \
							 gTCCDxBProc.puiAudioSubPID[gTCCDxBProc.uiAudioSubIndex], gTCCDxBProc.puiVideoSubPID[gTCCDxBProc.uiVideoSubIndex], \
							 gTCCDxBProc.puiAudioSubStreamType[gTCCDxBProc.uiAudioSubIndex], gTCCDxBProc.puiVideoSubStreamType[gTCCDxBProc.uiVideoSubIndex], \
							 gTCCDxBProc.uiPcrPID, gTCCDxBProc.uiPcrSubPID, gTCCDxBProc.uiPmtPID, gTCCDxBProc.uiPmtSubPID, gTCCDxBProc.uiCAECMPID, gTCCDxBProc.uiACECMPID, iDual_mode);

	tcc_manager_audio_start(0, gTCCDxBProc.puiAudioStreamType[gTCCDxBProc.uiAudioMainIndex]);
	tcc_manager_audio_start(1, gTCCDxBProc.puiAudioSubStreamType[gTCCDxBProc.uiAudioSubIndex]);

	if(!TCC_Isdbt_IsAudioOnlyMode()){
		tcc_manager_video_start(0, gTCCDxBProc.puiVideoStreamType[gTCCDxBProc.uiVideoMainIndex]);
		tcc_manager_video_start(1, gTCCDxBProc.puiVideoSubStreamType[gTCCDxBProc.uiVideoSubIndex]);
	}

	tcc_manager_audio_set_proprietarydata(gTCCDxBProc.uiChannel, gTCCDxBProc.uiServiceID, gTCCDxBProc.uiServiceSubID, iDual_mode, TCC_Isdbt_IsSupportPrimaryService());
	if(gTCCDxBProc.puiAudioPID[gTCCDxBProc.uiAudioMainIndex] != 0xFFFF
		&& gTCCDxBProc.puiAudioSubPID[gTCCDxBProc.uiAudioSubIndex] != 0xFFFF
		&& gTCCDxBProc.puiAudioStreamType[gTCCDxBProc.uiAudioMainIndex] != 0xFFFF
		&& gTCCDxBProc.puiAudioSubStreamType[gTCCDxBProc.uiAudioSubIndex] != 0xFFFF
		&& gTCCDxBSeamless.iOnoff== 1){
		tcc_manager_audio_setSeamlessSwitchCompensation(gTCCDxBSeamless.iOnoff, gTCCDxBSeamless.iInterval, gTCCDxBSeamless.iStrength,
			gTCCDxBSeamless.iNtimes, gTCCDxBSeamless.iRange, gTCCDxBSeamless.iGapadjust, gTCCDxBSeamless.iGapadjust2, gTCCDxBSeamless.iMultiplier);
	}

	if (iDual_mode == TCCDXBPROC_MAIN){
		subtitle_pid = gTCCDxBProc.uiStPID;
		superimpose_pid = gTCCDxBProc.uiSiPID;
		uiSegType = 13;
	}else{
		subtitle_pid = gTCCDxBProc.uiStSubPID;
		superimpose_pid = gTCCDxBProc.uiSiSubPID;
		uiSegType = 1;
	}

#if defined(HAVE_LINUX_PLATFORM)
	if(!TCC_Isdbt_IsAudioOnlyMode()){
		subtitle_display_wmixer_open();
	}
#endif

	if (uiSegType == 13) subtitle_system_set_stc_index(0);
	else subtitle_system_set_stc_index(1);
	tcc_manager_demux_subtitle_play(subtitle_pid, superimpose_pid, uiSegType, gTCCDxBProc.uiCountrycode,
									gTCCDxBProc.uiRaw_w, gTCCDxBProc.uiRaw_h, gTCCDxBProc.uiView_w, gTCCDxBProc.uiView_h, 1);

	tcc_manager_audio_set_dualmono(0, gTCCDxBProc.uiAudioMode);

	tcc_manager_video_select_display_output(iDual_mode);
	tcc_manager_audio_select_output(iDual_mode, true);
	tcc_manager_audio_select_output(iDual_mode?0:1, false);
	gTCCDxBProc.uiCurrDualDecodeMode = iDual_mode;

	TCCDxBEvent_AVIndexUpdate(gTCCDxBProc.uiChannel, gTCCDxBProc.uiNetworkID, gTCCDxBProc.uiServiceID, gTCCDxBProc.uiServiceSubID, gTCCDxBProc.uiAudioIndex, gTCCDxBProc.uiVideoIndex);

	tcc_dxb_proc_lock_off();
	return TCCDXBPROC_SUCCESS;
}

int TCCDxBProc_SetSeamlessSwitchCompensation(int iOnOff, int iInterval, int iStrength, int iNtimes, int iRange, int iGapadjust, int iGapadjust2, int iMultiplier)
{
	int ret = -1;		
#if 0
	if(gTCCDxBSeamless.iOnoff == 0)
		tcc_manager_audio_setSeamlessSwitchCompensation(0);
#else
	gTCCDxBSeamless.iOnoff = iOnOff;
	if(iInterval >= 10 && iInterval <=300){
		/*10~300*/
		gTCCDxBSeamless.iInterval = iInterval;
	}else{
	    /*default: 20*/
		gTCCDxBSeamless.iInterval = 20;
	}

	if(iStrength >= 0 && iStrength <=100){
		/*0~100*/
		gTCCDxBSeamless.iStrength = iStrength;
	}else{
		/*default: 40*/
		gTCCDxBSeamless.iStrength = 40;
	}

	if(iNtimes >= 10 && iNtimes <=1000){
		/*10~1000*/
		gTCCDxBSeamless.iNtimes = iNtimes;
	}else{
		/*default: 20*/
		gTCCDxBSeamless.iNtimes = 20;
	}

	if(iRange >= 10 && iRange <=300){
		 /*10~300*/
		gTCCDxBSeamless.iRange = iRange;
	}else{
		 /*default: 100*/
		gTCCDxBSeamless.iRange = 100;
	}

	if(iGapadjust >= -300 && iGapadjust <=300){
		 /*-300~+300*/
		gTCCDxBSeamless.iGapadjust = iGapadjust;
	}else{
		 /*default: 0*/
		gTCCDxBSeamless.iGapadjust = 0;
	}

	if(iGapadjust2 >= -150 && iGapadjust2 <=150){
		 /*-150~+150*/
		gTCCDxBSeamless.iGapadjust2 = iGapadjust2;
	}else{
		 /*default: 0*/
		gTCCDxBSeamless.iGapadjust2 = 0;
	}


	if(iMultiplier >= 1 && iMultiplier <=10){
		 /*1~10*/
		gTCCDxBSeamless.iMultiplier = iMultiplier;
	}else{
		 /*default: 0*/
		gTCCDxBSeamless.iMultiplier = 1;
	}

	ALOGD("\033[32m [%s] [%d:%d:%d:%d:%d:%d:%d:%d]\033[m\n", __func__,
		gTCCDxBSeamless.iOnoff, gTCCDxBSeamless.iInterval, gTCCDxBSeamless.iStrength, gTCCDxBSeamless.iNtimes,
		gTCCDxBSeamless.iRange, gTCCDxBSeamless.iGapadjust, gTCCDxBSeamless.iGapadjust2, gTCCDxBSeamless.iMultiplier);

	ret = tcc_manager_audio_setSeamlessSwitchCompensation(gTCCDxBSeamless.iOnoff, gTCCDxBSeamless.iInterval, gTCCDxBSeamless.iStrength,
		gTCCDxBSeamless.iNtimes, gTCCDxBSeamless.iRange, gTCCDxBSeamless.iGapadjust, gTCCDxBSeamless.iGapadjust2, gTCCDxBSeamless.iMultiplier);
#endif
	return ret;
}

int TCCDxBProc_DS_GetSTC(unsigned int *hi_data, unsigned int *lo_data)
{
	int result;
	unsigned int state;

	tcc_dxb_proc_lock_on();

	state = TCCDxBProc_GetMainState();

	if(	state == TCCDXBPROC_STATE_AIRPLAY)
	{
		tcc_manager_demux_get_stc(0, hi_data, lo_data);
		result = 0;
	}
	else
	{
		*hi_data = 0;
		*lo_data = 0;
		result = -1;
	}

	tcc_dxb_proc_lock_off();
	return result;
}

int TCCDxBProc_DS_GetServiceTime(unsigned int *year, unsigned int *month, unsigned int *day, unsigned int *hour, unsigned int *min, unsigned int *sec)
{
	int result;
	unsigned int state;
	int MJD, Polarity, HourOffset, MinOffset;
	int y_dash, m_dash, k;

	tcc_dxb_proc_lock_on();

	state = TCCDxBProc_GetMainState();

	if(	state == TCCDXBPROC_STATE_AIRPLAY)
	{
		tcc_db_get_current_time(&MJD, (int*)hour, (int*)min, (int*)sec, &Polarity, &HourOffset, &MinOffset);

		y_dash = ( MJD * 100 - 1507820) / 36525;
		m_dash = ( MJD * 10000 - 149561000 - (((y_dash * 3652500)/10000)*10000)) / 306001;
		*day = (MJD * 10000 - 149560000 - (((y_dash * 3652500)/10000)*10000) - (((m_dash * 306001)/10000)*10000)) / 10000;

		if(m_dash == 14 || m_dash == 15)
			k = 1;
		else
			k = 0;

		*year = y_dash + k + 1900;
		*month = m_dash - 1 - k * 12;

		result = 0;
	}
	else
	{
		*year	= 0;
		*month	= 0;
		*day	= 0;
		*hour	= 0;
		*min	= 0;
		*sec	= 0;

		result = -1;
	}

	tcc_dxb_proc_lock_off();
	return result;
}

int TCCDxBProc_DS_CheckExistComponentTagInPMT(int target_component_tag)
{
	int i,ret;
	int result;
	unsigned int state;
	int iChannelNumber, iServiceID, iServiceSubID;

	result = 0;

	state = TCCDxBProc_GetMainState();

	if (state == TCCDXBPROC_STATE_AIRPLAY)
	{
		TCCDxBProc_GetCurrentChannel(&iChannelNumber, &iServiceID, &iServiceSubID);

		if (gTCCDxBProc.uiCurrDualDecodeMode == TCCDXBPROC_MAIN)
		{
			LLOGE("[CheckExistComponentTagInPMT] ChannelNumber(%d), ServiceID(%d) \n", iChannelNumber, iServiceID);

			printf("[COMPTAG][M](%d) - ", gTCCDxBProc2.uiComponentTagCount);
			for (i = 0; i < gTCCDxBProc2.uiComponentTagCount; ++i)
				printf("%03d ", gTCCDxBProc2.puiComponentTag[i]);
			printf("\n");

			for (i = 0; i < gTCCDxBProc2.uiComponentTagCount; i++)
			{
				if (gTCCDxBProc2.puiComponentTag[i] == target_component_tag)
					break;
			}

			if (i < gTCCDxBProc2.uiComponentTagCount)
				result = (i+1);
		}
		else
		{
			LLOGE("[CheckExistComponentTagInPMT] ChannelNumber(%d), ServiceSubID(%d) \n", iChannelNumber, iServiceSubID);

			printf("[COMPTAG][S](%d) - ", gTCCDxBProc2.uiSubComponentTagCount);
			for (i = 0; i < gTCCDxBProc2.uiSubComponentTagCount; ++i)
				printf("%03d ", gTCCDxBProc2.puiSubComponentTag[i]);
			printf("\n");

			for (i = 0; i < gTCCDxBProc2.uiSubComponentTagCount; i++)
			{
				if (gTCCDxBProc2.puiSubComponentTag[i] == target_component_tag)
					break;
			}

			if (i < gTCCDxBProc2.uiSubComponentTagCount)
				result = (i+1);
		}
	}

	return result;
}

int TCCDxBProc_GetSeamlessValue(int *state, int *cval, int *pval)
{
	int ret;
	ret = tcc_manager_audio_get_SeamlessValue(state, cval, pval);
	return ret;
}
