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
//#define LOG_NDEBUG 0
#define LOG_TAG "ISDBTPlayer"
#include <utils/Log.h>

#include <string.h>
#include <stdlib.h> // for strtoul()

#include <cutils/properties.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#ifndef uint32_t
typedef unsigned int uint32_t;
#endif
#include <tccfb_ioctrl.h>
#include "tcc_isdbt_proc.h"
#include "tcc_isdbt_event.h"
#include "player/ISDBTPlayer.h"
#include "tcc_dxb_interface_type.h"
#include "tcc_isdbt_manager_tuner.h"
#include "tcc_isdbt_manager_demux.h"
#include "tcc_isdbt_manager_audio.h"
#include "tcc_isdbt_manager_cas.h"
#include "TsParse_ISDBT_DBLayer.h"
#include <subtitle_display.h>
#include <subtitle_main.h>
#include "tcc_dxb_timecheck.h"
#include "tcc_ui_thread.h"
#include "tcc_power.h"
#include "LB_debug_log.h"
#include "tcc_dxb_memory.h"
#include "TCCMemory.h"
/****************************************************************************
DEFINITION
****************************************************************************/
#define DEBUG_MSG(msg...)	ALOGD(msg)
#define ISDBT_PRESET_MODE_MAX   3
#define	PID_PMT_1SEG		0x1FC8

// for ctrl Last Frame
#define DEV_FB0 "/dev/graphics/fb0"

//using namespace linux;

/****************************************************************************
DEFINITION OF TYPE
****************************************************************************/

/****************************************************************************
DEFINITION OF GLOBAL VARIABLES
****************************************************************************/
CHANNEL_HEADER_INFORMATION stChannelHeaderInfo;
ISDBTPlayer *gpisdbtInstance = NULL;	// XXX

static int subtitle_LangCount = 0;
static int fd = -1;

/****************************************************************************
DEFINITION OF EXTERNAL FUNCTIONS
****************************************************************************/
extern int gCurrentChannel;
extern unsigned int gPlayingServiceID;

/****************************************************************************
DEFINITION OF LOCAL FUNCTIONS
****************************************************************************/
void notify_db_information_update(void *pDBInfo);


/****************************************************************************
DEFINITION OF LOCAL FUNCTIONS
****************************************************************************/
enum {
    STOP = 0,
    REQUEST_RUN,
    RUN,
    REQUEST_EXIT,
};

/****************************************************************************
DEBUG FOR SEGMENTATION ERROR (Default: disable)
 - Only for internal debug
****************************************************************************/
//#define	CATCH_SEGMENTATION
#ifdef CATCH_SEGMENTATION
#include <stdio.h>
#include <execinfo.h>
#include <signal.h>

void sighandler(int sig)
{
	void *frame_addrs[16];
	char **frame_strings;
	size_t backtrace_size;
	char cmd[1024];
	int i;

	backtrace_size = backtrace(frame_addrs, 16);
	frame_strings = backtrace_symbols(frame_addrs, backtrace_size);

	for(i=2;i<backtrace_size;i++)
	{
		printf("%d : [0x%x]%s\n", i, frame_addrs[i], frame_strings[i]);
	}

	sprintf(cmd, "cat /proc/%d/maps", getpid());
	system(cmd);

	free(frame_strings);
	exit(1);
}
#endif
/****************************************************************************
DEBUG FOR SEGMENTATION ERROR (Default: disable)
****************************************************************************/

void ISDBTPlayer::UI_ThreadFunc(void *arg)
{
	ISDBTPlayer *player = (ISDBTPlayer *) arg;
	if (player == NULL)
    {
		ALOGE("In %s, player is not valid", __func__);
		return;
	}
	DEBUG_MSG("%s Start !!", __func__);

	TCCDxBEvent_Init();
	player->mThreadState = RUN;
	#ifdef CATCH_SEGMENTATION
	ALOGE("\033[31m CATCH SEGMENTATION \033[m\n");
	signal(SIGSEGV, sighandler);
	#endif

	while (player->mThreadState == RUN)
    {
		if( TCCDxBEvent_Process() == 0 )
        {
			usleep(5000);
		}
	}

	TCCDxBEvent_DeInit();

	DEBUG_MSG("%s End !!", __func__);

    player->mThreadState = STOP;
}

ISDBTPlayer::ISDBTPlayer()
{
	int	timeout;

	pthread_mutex_init (&mLock, NULL);
	pthread_mutex_init (&mNotifyLock, NULL);

	if (gpisdbtInstance)
	{
		timeout = 0;
		while (timeout++ < 2000)
		{
			if (gpisdbtInstance == 0)
				break;
			usleep(5000);
		}
	}
	gpisdbtInstance = this;
	mThreadState = STOP;
	mClientListener = NULL;
	mListener = NULL;
}

ISDBTPlayer::~ISDBTPlayer()
{
	gpisdbtInstance = NULL;
	tcc_dxb_timecheck_printfile();

	pthread_mutex_destroy (&mNotifyLock);
	pthread_mutex_destroy (&mLock);
}

#define Patch_version "K5.10_SDK_v0.10.0-isdbt_v1.0.1"
#define Branch_name "master"

int ISDBTPlayer::prepare(unsigned int specific_info)
{
	char value[PROPERTY_VALUE_MAX] = { 0, };
	unsigned int uiLen = 0, uiData = 0;
	int fOverride = 0;
	int	timeout = 0;
	int err = 0;

	// DXB patch version
	printf("[DXB] Start : Patch version(%s) Branch(%s)\n", Patch_version, Branch_name);

	tcc_mw_memoryleakcheck_init();    // Noah, 20180611, Memory Leak
	tcc_dxb_fo_MemoryLeakCheck_init();

	LB_Debug_InitLog();
	// b'8~9, 0 - Turn on all log
	// b'8~9, 1 - Turn on info, warn, error logs
	// b'8~9, 2 - Turn on error log
	// b'8~9, 3 - Turn off all log
	LB_Debug_SetLogInfo((specific_info & 0x00000300) >> 8);

	ALOGE("[%s] start / specific_info : 0x%x\n", __func__, specific_info);

	while(mThreadState != STOP)
		usleep(1000);
	mThreadState = REQUEST_RUN;

	tcc_uithread_create("ISDBTPlayerUIThread", UI_ThreadFunc, (void *) this, getuid());

	timeout = 0;
	while(1)
	{
		if (timeout > 5 * 1000)    //timeout 10sec
			break;

		if (mThreadState != REQUEST_RUN)
			break;

		usleep(1000);
		timeout++;
	}

	if (mThreadState != RUN)
	{
		ALOGE("[%s] createJavaThread fail\n", __func__);
		return -1;
	}

	uiLen = property_get("tcc.dxb.isdbt.sinfo.override", value, "");
	if (uiLen)
	{
		uiData = strtoul(value,0,16);
		TCC_Isdbt_Specific_Info_Init(uiData);	//set ISDB-T configuration.
		DEBUG_MSG("[%s] specific_info=0x%x overriden\n", __func__, uiData);
		sprintf(value, "%x", uiData);
		property_set("tcc.dxb.isdbt.sinfo", value);
		fOverride = 1;
	}
	else
	{
		TCC_Isdbt_Specific_Info_Init(specific_info);	//set ISDB-T configuration.
		DEBUG_MSG("[%s:%d] specific_info = 0x%x\n",__func__, __LINE__, specific_info);
		sprintf(value, "%x", specific_info);
		property_set("tcc.dxb.isdbt.sinfo", value);
		fOverride = 0;
	}

	if (fOverride)
		specific_info = uiData;
	else
		specific_info = 0;

	err = OpenDB_Table();
	if (err != 0 /*SQLITE_OK*/)
	{
		ALOGE("[%s] OpenDB_Table Error(%d)\n", __func__, err);
		return -2;
	}

	err = OpenDB_EPG_Table();
	if (err != 0 /*SQLITE_OK*/)
	{
		ALOGE("[%s] OpenDB_Table Error(%d)\n", __func__, err);
		return -3;
	}

	err = TCCDxBProc_Init(TCC_Isdbt_IsSupportJapan() ? 0 : 1);
	if (err != 0 /*TCCDXBPROC_SUCCESS*/)
	{
		ALOGE("[%s] TCCDxBProc_Init Error(%d)\n", __func__, err);
		return -4;
	}

	// Noah / 20170621 / Added for IM478A-60 (The request additinal API to start/stop EPG processing)
	// Default value of the g_uiEpgOnOff is ON.
	setEPGOnOff(1);

	notify(EVENT_PREPARED, specific_info, 0, 0);

	tcc_manager_demux_check_service_num_Init();

	return err;
}

int ISDBTPlayer::release()
{
	int err = 0;

	ALOGE("[%s] start\n", __func__);

	err = TCCDxBProc_DeInit();
	if (err != 0 /*TCCDXBPROC_SUCCESS*/)
	{
		ALOGE("[%s] TCCDxBProc_DeInit Error(%d)\n", __func__, err);
		return -4;
	}
	
	err = CloseDB_Table();
	if (err != 0 /*SQLITE_OK*/)
	{
		ALOGE("[%s] OpenDB_Table Error(%d)\n", __func__, err);
		return -2;
	}

	err = CloseDB_EPG_Table();
	if (err != 0 /*SQLITE_OK*/)
	{
		ALOGE("[%s] OpenDB_Table Error(%d)\n", __func__, err);
		return -3;
	}

	mThreadState = REQUEST_EXIT;
	while(mThreadState != STOP)
	{
	    usleep(1000);
	}

	LB_Debug_DeinitLog();

	tcc_dxb_fo_MemoryLeakCheck_term();
	tcc_mw_memoryleakcheck_term();    // Noah, 20180611, Memory Leak

	return 0;
}

int ISDBTPlayer::stop()
{
	int err = 0;
	unsigned int uiState;

	ALOGE("[%s] start\n", __func__);

	uiState = TCCDxBProc_GetMainState();

	if((uiState == TCCDXBPROC_STATE_LOAD) || (uiState == TCCDXBPROC_STATE_SCAN))
	{
		ALOGE("%s:%d Error invalid state(%d)", __func__, __LINE__, uiState);
		return -2;
	}

	if(uiState == TCCDXBPROC_STATE_HANDOVER)
	{
		TCCDxBProc_StopHandover();
	}
	else
	{
		err = TCCDxBProc_Stop();
		if (err != 0)
		{
			ALOGE("[%s] TCCDxBProc_Stop Error(%d)", __func__, err);
			return err;
		}
    }

	return 0;
}

/* Noah / 20180131 / IM478A-77 (relay station search in the background using 2TS)
	Description
		If J&K App find "Relay Station Channel" by using 2TS, J&K App calls this function to save some data of NIT to DB.
		And then, J&K App should call "setChannel" function.

	Parameter
		int total : In
			Total numberr of RelayStationChInfo
		RelayStationChData *pChData : In
			This is the struct to regist the relay station channel information.
		int *mainRowID_List : Out
			The list of '_id' of ChannelDB' for Main channel(Fullseg)
		int *subRowID_List : Out
			The list of '_id' of ChannelDB' for Sub channel(1seg)

	Return
		0  : success
		-1 : error, invalid parameter
		-2 : error, invalid state
		1  : error, related to g_pChannelMsg queue or SQLite
*/
int ISDBTPlayer::setRelayStationChDataIntoDB(int total, RelayStationChData *pChData, int *mainRowID_List, int *subRowID_List)
{
	int i = 0;
	unsigned int uiState;
	RelayStationChData* pData = NULL;
	int mainRowID_ListIndex = 0;
	int subRowID_ListIndex = 0;
	int returnValue = 0;
	CUSTOMSEARCH_FOUND_INFO_T stFoundInfo = { 0, };

	ALOGE("[%s] start / total : %d\n", __func__, total);

	if(NULL == pChData || NULL == mainRowID_List || NULL == subRowID_List)
	{
		ALOGE("[%s] invalid parameter, NULL == pChData || NULL == mainRowID_List || == subRowID_List, Error !!!!!\n", __func__);
		return -1;
	}

	uiState = TCCDxBProc_GetMainState();

	if(uiState == TCCDXBPROC_STATE_SCAN)
	{
		ALOGE("[%s] invalid state(%d), Error !!!!!", __func__, uiState);
		return -2;
	}

	if(uiState == TCCDXBPROC_STATE_AIRPLAY)
	{
		TCCDxBProc_Stop();
	}
	else if(uiState == TCCDXBPROC_STATE_HANDOVER)
	{
	    TCCDxBProc_StopHandover();
	}

	pData = pChData;

	for(i = 0; i < total; i++)
	{
		returnValue = InsertDB_ChannelTable_2(pData->channelNumber,
											pData->countryCode,
											pData->PMT_PID,
											pData->remoconID,
											pData->serviceType,
											pData->serviceID,
											pData->regionID,
											pData->threedigitNumber,
											pData->TStreamID,
											pData->berAVG,
											pData->channelName,
											pData->TSName,
											pData->networkID,
											pData->areaCode,
											pData->frequency);
		if(returnValue)
		{
			ALOGE("[%s] InsertDB_ChannelTable_2 returns Error(%d), Error !!!!!\n", __func__, returnValue);
			return 1;
		}

		pData = pData->pNext;
		if(NULL == pData)
		{
			break;
		}
	}

	pData = pChData;

	returnValue = UpdateDB_CustomSearch_2(pData->channelNumber, pData->countryCode, pData->preset, pData->TStreamID, &stFoundInfo);
	if(returnValue)
	{
		ALOGE("[%s] UpdateDB_CustomSearch_2 returns Error(%d), Error !!!!!\n", __func__, returnValue);
		return 1;
	}

	*mainRowID_List = stFoundInfo.fullseg_id;
	*subRowID_List = stFoundInfo.oneseg_id;

	ALOGD("[%s] end / fullseg_id( %d ), oneseg_id( %d )\n", __func__, stFoundInfo.fullseg_id, stFoundInfo.oneseg_id);

	return 0;
}

int ISDBTPlayer::search(int scan_type, int country_code, int area_code, int channel_num, int options)
{
	unsigned int uiState;
	int err = 0;

	ALOGE("[%s] start / scan_type : %d, country_code : %d, area_code : 0x%x, channel_num : %d, options : %d\n",
		__func__, scan_type, country_code, area_code, channel_num, options);

	uiState = TCCDxBProc_GetMainState();

	if(uiState == TCCDXBPROC_STATE_SCAN)
	{
		ALOGE("%s:%d Error invalid state(%d)", __func__, __LINE__, uiState);
		return -2;
	}

	if(uiState == TCCDXBPROC_STATE_AIRPLAY)
	{
		err = TCCDxBProc_Stop();
		if (err != TCCDXBPROC_SUCCESS)
		{
			ALOGE("[%s] TCCDxBProc_Stop returns Error(%d), Error !!!!!\n", __func__, err);
			return -4;
		}
	}
	else if(uiState == TCCDXBPROC_STATE_HANDOVER)
	{
	    TCCDxBProc_StopHandover();
	}

	err = TCCDxBProc_Scan(scan_type, country_code, area_code, channel_num, options);

	return err;
}

int ISDBTPlayer::searchAndSetChannel(int country_code, int channel_num, int tsid, int options, int audioIndex, int videoIndex, int audioMode,
											int raw_w, int raw_h, int view_w, int view_h, int ch_index)
{
	int err = 0;
	unsigned int uiState;

	ALOGE("[%s] start / country_code : %d, channel_num : %d, tsid : 0x%x, options : %d, audioIndex : %d, videoIndex : %d, audioMode : %d\n",
		__func__, country_code, channel_num, tsid, options, audioIndex, videoIndex, audioMode);
	ALOGE("[%s] start / raw_w : %d, raw_h : %d, view_w : %d, view_h : %d, ch_index : %d\n",
		__func__, raw_w, raw_h, view_w, view_h, ch_index);

	uiState = TCCDxBProc_GetMainState();

	if(uiState == TCCDXBPROC_STATE_SCAN)
	{
		ALOGE("%s:%d Error invalid state(%d)", __func__, __LINE__, uiState);
		return -2;
	}

	if(uiState == TCCDXBPROC_STATE_AIRPLAY)
	{
		err = TCCDxBProc_Stop();
		if (err != TCCDXBPROC_SUCCESS)
		{
			ALOGE("[%s] TCCDxBProc_Stop returns Error(%d), Error !!!!!\n", __func__, err);
			return -4;
		}
	}
	else if(uiState == TCCDXBPROC_STATE_HANDOVER)
	{
		TCCDxBProc_StopHandover();
	}

	err = TCCDxBProc_ScanAndSet(country_code, channel_num, tsid, options, audioIndex, videoIndex, audioMode, raw_w, raw_h, view_w, view_h, ch_index);

	return err;
}

int ISDBTPlayer::customRelayStationSearchRequest (int search_kind, int *list_ch, int *list_tsid, int repeat)
{
	int err = 0;
	unsigned int uiState;

	ALOGE("[%s] start / search_kind : %d, repeat : %d\n", __func__, search_kind, repeat);

	uiState = TCCDxBProc_GetMainState();

	if(uiState == TCCDXBPROC_STATE_SCAN)
	{
		ALOGE("%s:%d Error invalid state(%d)", __func__, __LINE__, uiState);
		return -2;
	}

	if(uiState == TCCDXBPROC_STATE_AIRPLAY)
	{
		err = TCCDxBProc_Stop();
		if (err != TCCDXBPROC_SUCCESS)
		{
			ALOGE("[%s] TCCDxBProc_Stop returns Error(%d), Error !!!!!\n", __func__, err);
			return -4;
		}
	}       
	else if(uiState == TCCDXBPROC_STATE_HANDOVER)
	{
	    TCCDxBProc_StopHandover();
	}

	err = TCCDxBProc_Scan(CUSTOM_SCAN, search_kind, (int)list_ch, (int)list_tsid, repeat);	/* casting (int*) to (int) */

	return err;
}

int ISDBTPlayer::searchCancel(void)
{
	int err = 0;

	ALOGE("[%s] start\n", __func__);

	err = TCCDxBProc_StopScan();

	return err;
}

/* Noah / 20170621 / Added for IM478A-60 (The request additinal API to start/stop EPG processing)
	uiOnOff
		0 : EPG( EIT ) Off
		1 : EPG( EIT ) On
*/
int ISDBTPlayer::setEPGOnOff(unsigned int uiOnOff)
{
	int err = 0;

	ALOGE("[%s] start / uiOnOff : %d\n", __func__, uiOnOff);

	if (uiOnOff < 0 || 1 < uiOnOff)
	{
		return -3;
	}

	tcc_manager_demux_set_EpgOnOff(uiOnOff);

	return 0;
}

int ISDBTPlayer::setAudio(int index)
{
	int iChannelNumber = 0, iServiceID = 0, iServiceSubID = 0;
	int iAudioTotalCount = TCCDXBPROC_AUDIO_MAX_NUM;
	int iAudioTotalCountSub = TCCDXBPROC_AUDIO_MAX_NUM;
	int Audio_PID[TCCDXBPROC_AUDIO_MAX_NUM] = { 0, }, Audio_StreamType[TCCDXBPROC_AUDIO_MAX_NUM] = { 0, }, ComponentTag[TCCDXBPROC_AUDIO_MAX_NUM] = { 0, };
	int Audio_SubPID[TCCDXBPROC_AUDIO_MAX_NUM] = { 0, }, Audio_SubStreamType[TCCDXBPROC_AUDIO_MAX_NUM] = { 0, }, SubComponentTag[TCCDXBPROC_AUDIO_MAX_NUM] = { 0, };
	int index_main = 0, index_sub = 0;
	int ret = 0;

	TCCDxBProc_GetCurrentChannel(&iChannelNumber, &iServiceID, &iServiceSubID);

	if (iServiceID != -1) {
		ret = tcc_db_get_audio(iServiceID, iChannelNumber, &iAudioTotalCount, Audio_PID, Audio_StreamType, ComponentTag);
		if(ret != 0)
			return -1;
	} else {
		iAudioTotalCount = 0;
	}
	if (iServiceSubID != -1) {
		ret = tcc_db_get_audio(iServiceSubID, iChannelNumber, &iAudioTotalCountSub, Audio_SubPID, Audio_SubStreamType, SubComponentTag);
		if(ret != 0)
			return -2;
	} else {
		iAudioTotalCountSub = 0;
	}

	ALOGE("[%s] start / index : %d, iAudioTotalCount : %d, iAudioTotalCountSub : %d",
		__func__, index, iAudioTotalCount, iAudioTotalCountSub);

	if(iAudioTotalCount > 1 && iAudioTotalCount > index){
		ALOGI("[%s] index_main = %d, audioPID = %d, streamtype = %d", __func__, index, Audio_PID[index], Audio_StreamType[index]);
		index_main = index;
	} else {
		index_main = -1;
	}

	if (iAudioTotalCountSub > 1 && iAudioTotalCountSub > index) {
		ALOGI("[%s] index_sub = %d, audioPID = %d, streamtype = %d", __func__, index, Audio_SubPID[index], Audio_SubStreamType[index]);
		index_sub = index;
	} else {
		index_sub = -1;
	}

	ret = TCCDxBProc_ChangeAudio(index, index_main, (index_main == -1) ? 0x1FFF : Audio_PID[index_main],
			index_sub, (index_sub == -1) ? 0x1FFF : Audio_SubPID[index_sub]);
	if(ret != 0)
		return -3;

	return ret;
}

int ISDBTPlayer::setAudioByComponentTag(int component_tag)
{
	int i = 0;
	int iChannelNumber = 0, iServiceID = 0, iServiceSubID = 0;
	int iAudioTotalCount = TCCDXBPROC_AUDIO_MAX_NUM;
	int iAudioTotalCountSub = TCCDXBPROC_AUDIO_MAX_NUM;
	int Audio_PID[TCCDXBPROC_AUDIO_MAX_NUM] = { 0, }, Audio_StreamType[TCCDXBPROC_AUDIO_MAX_NUM] = { 0, }, ComponentTag[TCCDXBPROC_AUDIO_MAX_NUM] = { 0, };
	int Audio_SubPID[TCCDXBPROC_AUDIO_MAX_NUM] = { 0, }, Audio_SubStreamType[TCCDXBPROC_AUDIO_MAX_NUM] = { 0, }, SubComponentTag[TCCDXBPROC_AUDIO_MAX_NUM] = { 0, };
	int index = -1, index_main = -1, index_sub = -1;
	int ret = 0;

	TCCDxBProc_GetCurrentChannel(&iChannelNumber, &iServiceID, &iServiceSubID);

	if (iServiceID != -1) {
		ret = tcc_db_get_audio(iServiceID, iChannelNumber, &iAudioTotalCount, Audio_PID, Audio_StreamType, ComponentTag);
		if(ret != 0)
			return -1;
	} else {
		iAudioTotalCount = 0;
	}
	if (iServiceSubID != -1) {
		ret = tcc_db_get_audio(iServiceSubID, iChannelNumber, &iAudioTotalCountSub, Audio_SubPID, Audio_SubStreamType, SubComponentTag);
		if(ret != 0)
			return -2;
	} else {
		iAudioTotalCountSub = 0;
	}

	ALOGE("[%s] start / component_tag : 0x%x, iAudioTotalCount : %d, iAudioTotalCountSub : %d",
		__func__, component_tag, iAudioTotalCount, iAudioTotalCountSub);

	if(iAudioTotalCount > 1){

		for (i = 0; i < iAudioTotalCount; ++i)
		{
			if (ComponentTag[i]	== component_tag)
				break;
		}

		if ( i < iAudioTotalCount )
		{
			index = i;
			ALOGI("[%s] index_main = %d, audioPID = %d, streamtype = %d", __func__, index, Audio_PID[index], Audio_StreamType[index]);
			index_main = index;
		}
	}

	if (iAudioTotalCountSub > 1) {

		for (i = 0; i < iAudioTotalCountSub; ++i)
		{
			if (SubComponentTag[i] == component_tag)
				break;
		}

		if ( i < iAudioTotalCountSub )
		{
			index = i;
			ALOGI("[%s] index_sub = %d, audioPID = %d, streamtype = %d", __func__, index, Audio_SubPID[index], Audio_SubStreamType[index]);
			index_sub = index;
		}
	}

	ret = TCCDxBProc_ChangeAudio(index, index_main, (index_main == -1) ? 0x1FFF : Audio_PID[index_main],
			index_sub, (index_sub == -1) ? 0x1FFF : Audio_SubPID[index_sub]);
	if(ret != 0)
		return -3;

	return index;
}

int ISDBTPlayer::setVideo(int index)
{
	int iChannelNumber = 0, iServiceID = 0, iServiceSubID = 0;
	int iVideoTotalCount = TCCDXBPROC_VIDEO_MAX_NUM;
	int iVideoTotalCountSub = TCCDXBPROC_VIDEO_MAX_NUM;
	int Video_PID[TCCDXBPROC_VIDEO_MAX_NUM] = { 0, }, Video_StreamType[TCCDXBPROC_VIDEO_MAX_NUM] = { 0, }, ComponentTag[TCCDXBPROC_VIDEO_MAX_NUM] = { 0, };
	int Video_SubPID[TCCDXBPROC_VIDEO_MAX_NUM] = { 0, }, Video_SubStreamType[TCCDXBPROC_VIDEO_MAX_NUM] = { 0, }, SubComponentTag[TCCDXBPROC_VIDEO_MAX_NUM] = { 0, };
	int index_main = 0, index_sub = 0;
	int ret = 0;

	TCCDxBProc_GetCurrentChannel(&iChannelNumber, &iServiceID, &iServiceSubID);

	if (iServiceID != -1) {
		ret = tcc_db_get_video(iServiceID, iChannelNumber, &iVideoTotalCount, Video_PID, Video_StreamType, ComponentTag);
		if(ret != 0)
			return -1;
	} else {
		iVideoTotalCount = 0;
	}
	if (iServiceSubID != -1) {
		ret = tcc_db_get_video(iServiceSubID, iChannelNumber, &iVideoTotalCountSub, Video_SubPID, Video_SubStreamType, SubComponentTag);
		if(ret != 0)
			return -2;
	} else {
		iVideoTotalCountSub = 0;
	}

	ALOGE("[%s] start / index : %d, iVideoTotalCount : %d, iVideoTotalCountSub : %d",
		__func__, index, iVideoTotalCount, iVideoTotalCountSub);

	if(iVideoTotalCount > 1 && iVideoTotalCount > index){
		ALOGI("%s index_main = %d, videoPID = %d, streamtype = %d", __func__, index, Video_PID[index], Video_StreamType[index]);
		index_main = index;
	} else {
		index_main = -1;
	}

	if (iVideoTotalCountSub > 1 && iVideoTotalCountSub > index) {
		ALOGI("%s index_sub = %d, videoPID = %d, streamtype = %d", __func__, index, Video_SubPID[index], Video_SubStreamType[index]);
		index_sub = index;
	} else {
		index_sub = -1;
	}

	ret = TCCDxBProc_ChangeVideo(index, index_main, (index_main == -1) ? 0x1FFF : Video_PID[index_main],
			index_sub, (index_sub == -1) ? 0x1FFF : Video_SubPID[index_sub]);
	if(ret != 0)
		return -3;

	return 0;
}

int ISDBTPlayer::setVideoByComponentTag(int component_tag)
{
	int i = 0;
	int iChannelNumber = 0, iServiceID = 0, iServiceSubID = 0;
	int iVideoTotalCount = TCCDXBPROC_VIDEO_MAX_NUM;
	int iVideoTotalCountSub = TCCDXBPROC_VIDEO_MAX_NUM;
	int Video_PID[TCCDXBPROC_VIDEO_MAX_NUM] = { 0, }, Video_StreamType[TCCDXBPROC_VIDEO_MAX_NUM] = { 0, }, ComponentTag[TCCDXBPROC_VIDEO_MAX_NUM] = { 0, };
	int Video_SubPID[TCCDXBPROC_VIDEO_MAX_NUM] = { 0, }, Video_SubStreamType[TCCDXBPROC_VIDEO_MAX_NUM] = { 0, }, SubComponentTag[TCCDXBPROC_VIDEO_MAX_NUM] = { 0, };
	int index = -1, index_main = -1, index_sub = -1;
	int ret = 0;

	TCCDxBProc_GetCurrentChannel(&iChannelNumber, &iServiceID, &iServiceSubID);

	if (iServiceID != -1) {
		ret = tcc_db_get_video(iServiceID, iChannelNumber, &iVideoTotalCount, Video_PID, Video_StreamType, ComponentTag);
		if(ret != 0)
			return -1;
	} else {
		iVideoTotalCount = 0;
	}
	if (iServiceSubID != -1) {
		ret = tcc_db_get_video(iServiceSubID, iChannelNumber, &iVideoTotalCountSub, Video_SubPID, Video_SubStreamType, SubComponentTag);
		if(ret != 0)
			return -2;
	} else {
		iVideoTotalCountSub = 0;
	}

	ALOGE("[%s] start / component_tag : 0x%x, iVideoTotalCount : %d, iVideoTotalCountSub : %d",
		__func__, component_tag, iVideoTotalCount, iVideoTotalCountSub);

	if(iVideoTotalCount > 1){

		for (i = 0; i < iVideoTotalCount; ++i)
		{
			if (ComponentTag[i]	== component_tag)
				break;
		}

		if ( i < iVideoTotalCount )
		{
			index = i;
			ALOGI("[%s] index_main = %d, videoPID = %d, streamtype = %d", __func__, index, Video_PID[index], Video_StreamType[index]);
			index_main = index;
		}
	}

	if (iVideoTotalCountSub) {

		for (i = 0; i < iVideoTotalCountSub; ++i)
		{
			if (SubComponentTag[i] == component_tag)
				break;
		}

		if ( i < iVideoTotalCountSub )
		{
			index = i;
			ALOGI("[%s] index_sub = %d, videoPID = %d, streamtype = %d", __func__, index, Video_SubPID[index], Video_SubStreamType[index]);
			index_sub = index;
		}
	}

	ret = TCCDxBProc_ChangeVideo(index, index_main, (index_main == -1) ? 0x1FFF : Video_PID[index_main],
			index_sub, (index_sub == -1) ? 0x1FFF : Video_SubPID[index_sub]);
	if(ret != 0)
		return -3;

	return index;
}

int ISDBTPlayer::setAudioMuteOnOff(unsigned int uiOnOff)
{
	ALOGE("[%s] start / uiOnOff : %d\n", __func__, uiOnOff);

	return TCCDxBProc_SetAudioMuteOnOff(uiOnOff);
}

int ISDBTPlayer::setAudioOnOff(unsigned int uiOnOff)
{
	ALOGE("[%s] start / uiOnOff : %d\n", __func__, uiOnOff);

	return TCCDxBProc_SetAudioOnOff(uiOnOff);
}

int ISDBTPlayer::setVideoOnOff(unsigned int uiOnOff)
{
	ALOGE("[%s] start / uiOnOff : %d\n", __func__, uiOnOff);

	return TCCDxBProc_SetVideoOnOff(uiOnOff);
}

int ISDBTPlayer::setVideoOutput(unsigned int uiOnOff)
{
	ALOGE("[%s] start / uiOnOff : %d\n", __func__, uiOnOff);

	return TCCDxBProc_SetVideoOutput(uiOnOff);
}

/*
  * mainRowID	= _id of fullseg service
  * subRowID 		= _id of 1seg service
  * audioIndex		= index of audio ES. '-1' means a default audio ES
  * videoIndex		= index of video ES. '-1' means a default video ES
  * raw_w, raw_h, vidw_w, view_h = size of Android View
  * ch_index		= designate a service which will be rendered.
  */
int ISDBTPlayer::setChannel(int mainRowID, int subRowID, int audioIndex, int videoIndex, int audioMode, int raw_w, int raw_h, int view_w, int view_h, int ch_index)
{
	int err = 0;
	int iChannelNumber=0, iStPID=0xFFFF, iSiPID=0xFFFF, iServiceID=-1;
	int iChannelSubNumber=-1, iStSubPID=-1, iSiSubPID=-1, iServiceSubID=-1;

	int iServiceType=0xFFFF, iPCRPID=0, iPMTPID= 0, iCAECMPID=0, iACECMPID=0, iNetworkID=0, iAudioTotalCount=0, iVideoTotalCount=0;
	int iServiceSubType=-1, iPCRSubPID=-1, iPMTSubPID= 0, iCAECMSubPID=0, iACECMSubPID=0, iSubAudioTotalCount=-1, iSubVideoTotalCount=-1;
	int iPreset=0;

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

	int	row_id;
	int	audioMainIndex = audioIndex;
	int	audioSubIndex = audioIndex;
	int	videoMainIndex = videoIndex;
	int	videoSubIndex = videoIndex;

	unsigned int uiState, uiSubState;

	ALOGE("[%s] start / mainRowID : %d, subRowID : %d, audioIndex : %d, videoIndex : %d, audioMode : %d\n",
		__func__, mainRowID, subRowID, audioIndex, videoIndex, audioMode);
	ALOGE("[%s] start / raw_w : %d, raw_h : %d, view_w : %d, view_h : %d, ch_index : %d\n",
		__func__, raw_w, raw_h, view_w, view_h, ch_index);

	uiState = TCCDxBProc_GetMainState();

	if(uiState == TCCDXBPROC_STATE_SCAN)
	{
		/* Noah / 2017.04.07 / IM478A-13, Improvement of the Search API(Auto Search)
			Request : While auto searching, 'setChannel' can be called without calling 'searchCancel'.
			Previous Sequence
				1. search( 5, , , , );                           -> Start 'Auto Search'
				2. Searching in progress . . .
				3. searchCancel before finishing the searching.
				4. setChannel( X, Y, , ... );                    -> Called by Application
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
		if(AUTOSEARCH == TCCDxBProc_GetScanType())
		{
			searchCancel();

			if(TCCDxBProc_GetMainState() == TCCDXBPROC_STATE_SCAN)
				TCCDxBProc_SetState(TCCDXBPROC_STATE_LOAD);
		}
		else
		{
			ALOGE("%s:%d Error invalid state(%d)", __func__, __LINE__, uiState);
			return -2;
		}
	}

	if(uiState == TCCDXBPROC_STATE_AIRPLAY)
	{
		err = TCCDxBProc_Stop();
		if (err != TCCDXBPROC_SUCCESS)
		{
			ALOGE("[%s] TCCDxBProc_Stop returns Error(%d), Error !!!!!\n", __func__, err);
			return -4;
		}
	}
    else if(uiState == TCCDXBPROC_STATE_HANDOVER)
    {
		TCCDxBProc_StopHandover();
	}

	tcc_dxb_timecheck_set("switch_channel", "application", TIMECHECK_START);

	if ((ch_index == 0 && mainRowID < 0) || (ch_index == 1 && subRowID < 0))
	{
		ALOGE("In %s, invalid channel information(%d, %d)", __func__, mainRowID, subRowID);
	}

	if (ch_index == 0)	//fullseg
		row_id = mainRowID;
	else
		row_id = subRowID;

	// check whether a channel info is made from regional preset
	err = tcc_db_get_preset(row_id, &iChannelNumber, &iPreset);
	if (err < 0)
	{
		ALOGE("In %s, no row(%d) in channel", __func__, mainRowID);
		return -1;
	}

	if (iPreset == 1) //if channel info is made from regional preset.
	{
		ALOGD("In %s (%d), this channel was preset\n", __func__, __LINE__);

		//try manual scan for iChannelNumber
		TCCDxBProc_Stop();
		if (TCCDxBProc_ScanOneChannel (iChannelNumber) != 0)
		{
			//if can't find a channel on specified channel frequency
			ALOGE("In %s(%d), return because fail scan one channel(%d)\n",__func__, __LINE__, iChannelNumber);
			return -1;
		}
		else
		{
			tcc_db_get_preset(row_id, &iChannelNumber, &iPreset);
			if (iPreset == 1) //In case that the channel informaiton was made from regional preset and it can't find the channel on air
			{
				ALOGE("In %s, can't tune the preset channel\n", __func__);
				return -1;
			}
		}
	}

	if (mainRowID > 0)
	{
		// get main channel info
		err = tcc_db_get_channel(mainRowID,
								&iChannelNumber, piAudioPID, piVideoPID,
								&iStPID, &iSiPID,
								&iServiceType, &iServiceID,
								&iPMTPID, &iCAECMPID, &iACECMPID, &iNetworkID,
								piAudioStreamType, piVideoStreamType,
								piAudioComponentTag, piVideoComponentTag,
								&iPCRPID, &iAudioTotalCount, &iVideoTotalCount,
								&audioIndex, &audioMainIndex, &videoIndex, &videoMainIndex);
		if (err < 0)
		{
			ALOGE("In %s(%d), can't find main channel info(%d)\n", __func__, __LINE__, mainRowID);
			return -1;
		}
	}

	// get sub channel info
	if(subRowID > 0)
	{
		err = tcc_db_get_channel(subRowID,
								&iChannelSubNumber, piAudioSubPID, piVideoSubPID,
								&iStSubPID, &iSiSubPID,
								&iServiceSubType, &iServiceSubID,
								&iPMTSubPID, &iCAECMSubPID, &iACECMSubPID, &iNetworkID,
								piAudioSubStreamType, piVideoSubStreamType,
								piAudioSubComponentTag, piVideoSubComponentTag,
								&iPCRSubPID, &iSubAudioTotalCount, &iSubVideoTotalCount,
								&audioIndex, &audioSubIndex, &videoIndex, &videoSubIndex);
		if (err < 0)
		{
			ALOGE("In %s(%d), can't find sub channel info(%d)\n", __func__, __LINE__, subRowID);
		}
	}

	// set channel info
	TCCDxBProc_Set(iChannelNumber,
				 	iAudioTotalCount, (unsigned int*)piAudioPID, iVideoTotalCount, (unsigned int*)piVideoPID, (unsigned int*)piAudioStreamType, (unsigned int*)piVideoStreamType,
				 	iSubAudioTotalCount, (unsigned int*)piAudioSubPID, iSubVideoTotalCount, (unsigned int*)piVideoSubPID, (unsigned int*)piAudioSubStreamType, (unsigned int*)piVideoSubStreamType,
				 	iPCRPID, iPCRSubPID, iStPID, iSiPID, iServiceID, iPMTPID, iCAECMPID, iACECMPID, iNetworkID,
					iStSubPID, iSiSubPID, iServiceSubID, iPMTSubPID,
					audioIndex, audioMainIndex, audioSubIndex,
					videoIndex, videoMainIndex, videoSubIndex,
					audioMode, iServiceType, raw_w, raw_h, view_w, view_h,
					ch_index);

	return 0;
}

/* Noah / 20171026 / Added setChannel_withChNum function for IM478A-52 (database less tuning)
  * channelNumber                = index of frequency table. Ex) 0 : 473.143MHz, 1 : 479.143MHz ...
  * serviceID_Fseg               = service_id of Fseg
  * serviceID_1seg               = service_id of 1seg
  * audioMode                    = 0 : DUALMONO_LEFT, 1 : DUALMONO_RIGHT, 2 : DUALMONO_LEFTNRIGHT
  * raw_w, raw_h, vidw_w, view_h = size of Android View
  * ch_index                     = Main/Sub channel to be displayed. 0 : Fseg, 1 : 1seg
  */
int ISDBTPlayer::setChannel_withChNum(int channelNumber, int serviceID_Fseg, int serviceID_1seg, int audioMode, int raw_w, int raw_h, int view_w, int view_h, int ch_index)
{
	int err = 0;
	int iChannelNumber=0, iStPID=0xFFFF, iSiPID=0xFFFF, iServiceID=-1;
	int iChannelSubNumber=-1, iStSubPID=-1, iSiSubPID=-1, iServiceSubID=-1;

	int iServiceType=0xFFFF, iPCRPID=0, iPMTPID= 0, iCAECMPID=0, iACECMPID=0, iNetworkID=0, iAudioTotalCount=0, iVideoTotalCount=0;
	int iServiceSubType=-1, iPCRSubPID=-1, iPMTSubPID= 0, iCAECMSubPID=0, iACECMSubPID=0, iSubAudioTotalCount=-1, iSubVideoTotalCount=-1;
	int iPreset=0;

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

	// Noah / IM478A-52
	int audioIndex = -1;    // -1 : default Audio index
	int videoIndex = -1;    // -1 : default Video index

	int	row_id;
	int	audioMainIndex = audioIndex;
	int	audioSubIndex = audioIndex;
	int	videoMainIndex = videoIndex;
	int	videoSubIndex = videoIndex;

	unsigned int uiState, uiSubState;

	// Noah / IM478A-52
	int mainRowID = 0;
	int subRowID = 0;

	ALOGE("[%s] start / channelNumber : %d, serviceID_Fseg : 0x%x, serviceID_1seg : 0x%x, audioMode : %d\n",
		__func__, channelNumber, serviceID_Fseg, serviceID_1seg, audioMode);
	ALOGE("[%s] start / raw_w : %d, raw_h : %d, view_w : %d, view_h : %d, ch_index : %d\n",
		__func__, raw_w, raw_h, view_w, view_h, ch_index);

	uiState = TCCDxBProc_GetMainState();

	if(uiState == TCCDXBPROC_STATE_SCAN)
	{
		/* Noah / 2017.04.07 / IM478A-13, Improvement of the Search API(Auto Search)
			Request : While auto searching, 'setChannel' can be called without calling 'searchCancel'.
			Previous Sequence
				1. search( 5, , , , );                           -> Start 'Auto Search'
				2. Searching in progress . . .
				3. searchCancel before finishing the searching.
				4. setChannel( X, Y, , ... );                    -> Called by Application
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
		if(AUTOSEARCH == TCCDxBProc_GetScanType())
		{
			searchCancel();

			if(TCCDxBProc_GetMainState() == TCCDXBPROC_STATE_SCAN)
				TCCDxBProc_SetState(TCCDXBPROC_STATE_LOAD);
		}
		else
		{
			ALOGE("%s:%d Error invalid state(%d)", __func__, __LINE__, uiState);
			return -2;
		}
	}

	if(uiState == TCCDXBPROC_STATE_AIRPLAY)
	{
		err = TCCDxBProc_Stop();
		if (err != TCCDXBPROC_SUCCESS)
		{
			ALOGE("[%s] TCCDxBProc_Stop returns Error(%d), Error !!!!!\n", __func__, err);
			return -4;
		}
	}
	else if(uiState == TCCDXBPROC_STATE_HANDOVER)
	{
		TCCDxBProc_StopHandover();
	}

	// Noah / IM478A-52
	{
		if (serviceID_Fseg)
			mainRowID = tcc_db_get_channel_rowid_new(channelNumber, serviceID_Fseg);

		if (serviceID_1seg)
			subRowID = tcc_db_get_channel_rowid_new(channelNumber, serviceID_1seg);

		ALOGE("[%s] mainRowID : %d, subRowID : %d\n", __func__, mainRowID, subRowID);
	}

	tcc_dxb_timecheck_set("switch_channel", "application", TIMECHECK_START);

	if ((ch_index == 0 && mainRowID < 0) || (ch_index == 1 && subRowID < 0))
	{
		ALOGE("In %s, invalid channel information(%d, %d)", __func__, mainRowID, subRowID);
	}

	if (ch_index == 0)	//fullseg
		row_id = mainRowID;
	else
		row_id = subRowID;

	// check whether a channel info is made from regional preset
	err = tcc_db_get_preset(row_id, &iChannelNumber, &iPreset);
	if (err < 0)
	{
		ALOGE("In %s, no row(%d) in channel", __func__, mainRowID);
		return -1;
	}

	if (iPreset == 1)	//if channel info is made from regional preset.
	{
		ALOGD("In %s (%d), this channel was preset\n", __func__, __LINE__);

		//try manual scan for iChannelNumber
		TCCDxBProc_Stop();
		if (TCCDxBProc_ScanOneChannel (iChannelNumber) != 0)
		{
			//if can't find a channel on specified channel frequency
			ALOGE("In %s(%d), return because fail scan one channel(%d)\n",__func__, __LINE__, iChannelNumber);
			return -1;
		}
		else
		{
			tcc_db_get_preset(row_id, &iChannelNumber, &iPreset);
			if (iPreset == 1) //In case that the channel informaiton was made from regional preset and it can't find the channel on air
			{
				ALOGE("In %s, can't tune the preset channel\n", __func__);
				return -1;
			}
		}
	}

	if (mainRowID > 0)
	{
		// get main channel info
		err = tcc_db_get_channel(mainRowID,
								&iChannelNumber, piAudioPID, piVideoPID,
								&iStPID, &iSiPID,
								&iServiceType, &iServiceID,
								&iPMTPID, &iCAECMPID, &iACECMPID, &iNetworkID,
								piAudioStreamType, piVideoStreamType,
								piAudioComponentTag, piVideoComponentTag,
								&iPCRPID, &iAudioTotalCount, &iVideoTotalCount,
								&audioIndex, &audioMainIndex, &videoIndex, &videoMainIndex);
		if (err < 0)
		{
			ALOGE("In %s(%d), can't find main channel info(%d)\n", __func__, __LINE__, mainRowID);
			return -1;
		}
	}

	// get sub channel info
	if(subRowID > 0)
	{
		err = tcc_db_get_channel(subRowID,
								&iChannelSubNumber, piAudioSubPID, piVideoSubPID,
								&iStSubPID, &iSiSubPID,
								&iServiceSubType, &iServiceSubID,
								&iPMTSubPID, &iCAECMSubPID, &iACECMSubPID, &iNetworkID,
								piAudioSubStreamType, piVideoSubStreamType,
								piAudioSubComponentTag, piVideoSubComponentTag,
								&iPCRSubPID, &iSubAudioTotalCount, &iSubVideoTotalCount,
								&audioIndex, &audioSubIndex, &videoIndex, &videoSubIndex);
		if (err < 0)
		{
			ALOGD("In %s(%d), can't find sub channel info(%d)\n", __func__, __LINE__, subRowID);
		}
	}

	// set channel info
	TCCDxBProc_Set(iChannelNumber,
				 	iAudioTotalCount, (unsigned int*)piAudioPID, iVideoTotalCount, (unsigned int*)piVideoPID, (unsigned int*)piAudioStreamType, (unsigned int*)piVideoStreamType,
				 	iSubAudioTotalCount, (unsigned int*)piAudioSubPID, iSubVideoTotalCount, (unsigned int*)piVideoSubPID, (unsigned int*)piAudioSubStreamType, (unsigned int*)piVideoSubStreamType,
				 	iPCRPID, iPCRSubPID, iStPID, iSiPID, iServiceID, iPMTPID, iCAECMPID, iACECMPID, iNetworkID,
					iStSubPID, iSiSubPID, iServiceSubID, iPMTSubPID,
					audioIndex, audioMainIndex, audioSubIndex,
					videoIndex, videoMainIndex, videoSubIndex,
					audioMode, iServiceType, raw_w, raw_h, view_w, view_h,
					ch_index);

	return 0;
}

int ISDBTPlayer::setDualChannel(int channelIndex)
{
	int ret = -1;
	ALOGE("[%s] start / channelIndex : %d\n", __func__, channelIndex);

	ret = TCCDxBProc_SetDualChannel(channelIndex);
	return ret;
}

int ISDBTPlayer::getChannelInfo(int iChannelNumber, int iServiceID,
									 int *piRemoconID, int *piThreeDigitNumber, int *piServiceType, unsigned short *pusChannelName, int *piChannelNameLen,
									 int *piTotalAudioPID, int *piTotalVideoPID, int *piTotalSubtitleLang,
									 int *piAudioMode, int *piVideoMode, int *piAudioLang1, int *piAudioLang2,
									 int *piAudioComponentTag, int *piVideoComponentTag, int *piSubtitleComponentTag,
									 int *piStartMJD, int *piStartHH, int *piStartMM, int *piStartSS, int *piDurationHH, int *piDurationMM, int *piDurationSS, unsigned short *pusEvtName, int *piEvtNameLen,
									 int *piLogoImageWidth, int *piLogoImageHeight, unsigned int *pusDecLogoImage, unsigned short *pusSimpleLogo, int *piSimpleLogoLength, int *piMVTVGroupType)
{
	int err = 0;
	unsigned char *pucEncLogoStream;
	int iLogoStreamSize, svc_type;
	iLogoStreamSize = 2048;
	int iNumCh, iAudioMode;

	pucEncLogoStream = (unsigned char*)tcc_mw_malloc(__FUNCTION__, __LINE__, 2048);

	ALOGE("[%s] start / iChannelNumber : %d, iServiceID : 0x%x\n", __func__, iChannelNumber, iServiceID);

	tcc_db_get_channel_info(iChannelNumber, iServiceID,
							piRemoconID, piThreeDigitNumber, piServiceType, pusChannelName, piChannelNameLen,
							piTotalAudioPID, piTotalVideoPID, piTotalSubtitleLang,
							piAudioMode, piVideoMode, piAudioLang1, piAudioLang2,
							piAudioComponentTag, piVideoComponentTag, piSubtitleComponentTag,
							piStartMJD, piStartHH, piStartMM, piStartSS,
							piDurationHH, piDurationMM, piDurationSS, pusEvtName, piEvtNameLen,
							pucEncLogoStream, &iLogoStreamSize, pusSimpleLogo, piSimpleLogoLength);

	*piMVTVGroupType = TCCDxBProc_GetComponentTag(iServiceID, 0, NULL, NULL);

	if(TCCDxbProc_DecodeLogoImage(pucEncLogoStream, iLogoStreamSize, pusDecLogoImage, piLogoImageWidth, piLogoImageHeight))
	{
		*piLogoImageWidth = 0;
		*piLogoImageHeight = 0;
		tcc_db_delete_logo(iChannelNumber, iServiceID);
	}

	subtitle_LangCount = *piTotalSubtitleLang;

	/* according to customer's request, audio_mode comes fromAAC codec */
	*piAudioMode = 0;	//unknown
	if (*piServiceType != -1)	//if the service was found,
	{
		if (*piServiceType == 1) //fullseg
		{
			err = tcc_manager_audio_get_audiotype(0, &iNumCh, &iAudioMode);	//fullseg
		}
		else
		{
			err = tcc_manager_audio_get_audiotype(1, &iNumCh, &iAudioMode);	//1seg
		}

		if (!err)
		{
			if (iNumCh==0)	*piAudioMode = 0;	//unknown
			else if (iNumCh==1) *piAudioMode = 1;	//1/0 - mono
			else if (iNumCh ==2)
			{
				if (iAudioMode==2) *piAudioMode = 3;	//2/0 - stereo
				else if (iAudioMode==16)	*piAudioMode = 2;	//1/0 + 1/0 - dual mono
			}
			else if (iNumCh==4) *piAudioMode = 7; //3/1 mode
			else if (iNumCh==5) *piAudioMode = 8;	//3/2 mode
			else if (iNumCh==6) *piAudioMode = 9; //3/2+LFE mode
		}
	}

	DEBUG_MSG("iChannelNumber:%d, iServiceID:%d", iChannelNumber, iServiceID);
	DEBUG_MSG("RemoconID:%d, iThreeDigitNumber:%d, iServiceType:%d", *piRemoconID, *piThreeDigitNumber, *piServiceType);
	DEBUG_MSG("iTotalAudioPID:%d, iTotalVideoPID:%d, iTotalSubtitle:%d", *piTotalAudioPID, *piTotalVideoPID, *piTotalSubtitleLang);
	DEBUG_MSG("iAudioMode:%d, iVideoMode:%d", *piAudioMode, *piVideoMode);
	DEBUG_MSG("iStartMJD:%d, iStartHH:%d, iStartMM:%d, iStartSS:%d, iDurationHH:%d, iDurationMM:%d, iDurationSS:%d", *piStartMJD, *piStartHH, *piStartMM, *piStartSS, *piDurationHH, *piDurationMM, *piDurationSS);
	DEBUG_MSG("iLogoWidth:%d, iLogoHeight:%d", *piLogoImageWidth, *piLogoImageHeight);
	DEBUG_MSG("iSimpleLogoLength:%d", *piSimpleLogoLength);

	tcc_mw_free(__FUNCTION__, __LINE__, pucEncLogoStream);

	return 0;
}

int ISDBTPlayer::getSignalStrength(int *sqinfo)
{
	ALOGE("[%s] start\n", __func__);

	TCCDxBProc_GetTunerStrength(sqinfo);

	return 0;
}

extern "C" int subtitle_data_enable(int enable);
extern "C" int superimpose_data_enable(int enable);

extern "C" int subtitle_demux_select_subtitle_lang(int index, int sel_cnt);
extern "C" int subtitle_demux_select_superimpose_lang(int index, int sel_cnt);

int  ISDBTPlayer::playSubtitle(int onoff)
{
	int ret = -1;
	ALOGE("[%s] start / onoff : %d\n", __func__, onoff);

	tcc_manager_demux_set_subtitle_option(0, onoff);
	ret = subtitle_data_enable(onoff);

	return ret;
}

int  ISDBTPlayer::playSuperimpose(int onoff)
{
	int ret = -1;
	ALOGE("[%s] start / onoff : %d\n", __func__, onoff);

	tcc_manager_demux_set_subtitle_option(1, onoff);
	ret = superimpose_data_enable(onoff);

	return ret;
}

int ISDBTPlayer::setSubtitle (int index)
{
	int ret = -1;
	ALOGE("[%s] start / index : %d, subtitle_LangCount : %d\n", __func__, index, subtitle_LangCount);

	ret = subtitle_demux_select_subtitle_lang(index, subtitle_LangCount);

	return ret;
}

int ISDBTPlayer::setSuperImpose (int index)
{
	int ret = -1;
	ALOGE("[%s] start / index : %d, subtitle_LangCount : %d\n", __func__, index, subtitle_LangCount);

	ret = subtitle_demux_select_superimpose_lang(index, subtitle_LangCount);

	return ret;
}

int ISDBTPlayer::setAudioDualMono (int audio_sel)
{
	int ret = -1;
	ALOGE("[%s] start / audio_sel : %d\n", __func__, audio_sel);

	ret = TCCDxBProc_SetAudioDualMono(audio_sel);

	return ret;
}

extern "C" void ISDBT_AccessCtrl_SetAgeRate(unsigned char age);
extern "C" void ResetSIVersionNumber(void);

int ISDBTPlayer::reqTRMPInfo (unsigned char **ppInfo, int *piInfoSize)
{
	ALOGE("[%s] start\n", __func__);

	TCCDxBProc_ReqTRMPInfo(ppInfo, piInfoSize);

	return 0;
}

int ISDBTPlayer::resetTRMPInfo(void)
{
	ALOGE("[%s] start\n", __func__);

	TCCDxBProc_ResetTRMPInfo();

	return 0;
}

int ISDBTPlayer::getSubtitleLangInfo(int *iSubtitleLangNum, int *iSuperimposeLangNum, unsigned int *iSubtitleLang1, unsigned int *iSubtitleLang2, unsigned int *iSuperimposeLang1, unsigned int *iSuperimposeLang2)
{
	int manage_ret = 0, eit_ret = 0;
	
	ALOGE("[%s] start\n", __func__);

	*iSubtitleLangNum = 0;
	*iSuperimposeLangNum = 0;
	*iSubtitleLang1 = 0;
	*iSubtitleLang2 = 0;
	*iSuperimposeLang1 = 0;
	*iSuperimposeLang2 = 0;

	if(tcc_manager_demux_get_validOutput() == 0)
	{
		//detect different serviceID.
		DEBUG_MSG("[getSubtitleLangInfo] CaptionLangNum=%d, SuperimposeLangNum=%d\n", *iSubtitleLangNum, *iSuperimposeLangNum);
		DEBUG_MSG("[getSubtitleLangInfo] CaptionLang1=0x%X, CaptionLang2=0x%X\n", *iSubtitleLang1, *iSubtitleLang2);
		DEBUG_MSG("[getSubtitleLangInfo] SuperimposeLang1=0x%X, SuperimposeLang2=0x%X\n", *iSuperimposeLang1, *iSuperimposeLang2);
		DEBUG_MSG("[getSubtitleLangInfo] Because detect to different serviceID, no informaion.\n");
		return -1;
	}

	manage_ret = subtitle_get_lang_info(0, iSubtitleLangNum, iSubtitleLang1, iSubtitleLang2);
//	DEBUG_MSG("[getSubtitleLangInfo] MGM NUM:%d, MGM valid:%d (0:valid, -1:invalid)\n", *iSubtitleLangNum, manage_ret);
	eit_ret = tcc_manager_demux_get_eit_subtitleLangInfo(iSubtitleLangNum, iSubtitleLang1, iSubtitleLang2);
//	DEBUG_MSG("[getSubtitleLangInfo] EIT NUM:%d, EIT valid:%d (0:valid, -1:invalid)\n", *iSubtitleLangNum, eit_ret);
	if(manage_ret !=0 && eit_ret != 0){
		if(subtitle_demux_get_statement_in() == 1 && subtitle_demux_get_handle_info() == 1){
			*iSubtitleLangNum = 1;
			*iSubtitleLang1 = 0x6A706E;
			*iSubtitleLang2 = 0;
		}
	}
	subtitle_get_lang_info(1, iSuperimposeLangNum, iSuperimposeLang1, iSuperimposeLang2);

	DEBUG_MSG("[getSubtitleLangInfo] CaptionLangNum=%d, SuperimposeLangNum=%d\n", *iSubtitleLangNum, *iSuperimposeLangNum);
	DEBUG_MSG("[getSubtitleLangInfo] CaptionLang1=0x%X, CaptionLang2=0x%X\n", *iSubtitleLang1, *iSubtitleLang2);
	DEBUG_MSG("[getSubtitleLangInfo] SuperimposeLang1=0x%X, SuperimposeLang2=0x%X\n", *iSuperimposeLang1, *iSuperimposeLang2);

	return 0;
}

int ISDBTPlayer::setListener(int (*listener)(int, int, int, void*))
{
	ALOGE("[%s] start\n", __func__);

	pthread_mutex_lock (&mLock);
	mListener = listener;
	pthread_mutex_unlock (&mLock);

	return 0;
}

/* err 0 = success, else = fail */
/* preset_mode = index of preset mode (0~2) */
/* preset_mode means to maintain several (3 in current implementation) channel list */
/* it's required from japanese customer */
int ISDBTPlayer::setPresetMode(int preset_mode)
{
	int err = 0;

	ALOGE("[%s] start / preset_mode : %d\n", __func__, preset_mode);

	if (preset_mode < 0 || preset_mode >= ISDBT_PRESET_MODE_MAX)
	{
		return -3;
	}

	err = TCCDxBProc_SetPresetMode(preset_mode);
	if(err != TCCDXBPROC_SUCCESS)
	{
		ALOGE("[%s] TCCDxBProc_SetPresetMode returns Error(%d), Error !!!!!", __func__, err);
	}

	return err;
}

int ISDBTPlayer::setCustom_Tuner(int size, void *arg)
{
	ALOGE("[%s] start / size : %d\n", __func__, size);

	if(arg != NULL && size > 0)
		return TCCDxBProc_SetCustomTuner(size, arg);
	return -1;
}

int ISDBTPlayer::getCustom_Tuner(int *size, void *arg)
{
	ALOGE("[%s] start\n", __func__);

	if(arg != NULL)
		return TCCDxBProc_GetCustomTuner(size, arg);
	return -1;
}

int ISDBTPlayer::setNotifyDetectSection (int sectionIDs)
{
	ALOGE("[%s] start / sectionIDs : 0x%x\n", __func__, sectionIDs);

	TCCDxBProc_SetSectionNotificationIDs(sectionIDs);

	return 0;
}

int ISDBTPlayer::getDigitalCopyControlDescriptor(unsigned short usServiceID, DigitalCopyControlDescriptor** pDCCDInfo)
{
	T_DESC_DCC* pDCCInfo = 0;

	ALOGE("[%s] start / usServiceID : 0x%x\n", __func__, usServiceID);

	// Important !!!!! : pDCCInfo is a point of &(gCurrentDescInfo.services[i].stDescDCC).
	TCCDxBProc_GetCur_DCCDescriptorInfo(usServiceID, (void**)&pDCCInfo);
	if(!pDCCInfo)
	{
		printf("[%s][%d] pDCCInfo is null Error !!!!!\n", __func__, __LINE__);
		return -1;
	}

	DigitalCopyControlDescriptor* clsDCCD = (DigitalCopyControlDescriptor*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(DigitalCopyControlDescriptor));
	if(0 == clsDCCD)
	{
		ALOGE("[%s][%d] : DigitalCopyControlDescriptor malloc fail !!!!!\n", __func__, __LINE__);
		return -4;
	}
	memset(clsDCCD, 0, sizeof(DigitalCopyControlDescriptor));

	clsDCCD->digital_recording_control_data = pDCCInfo->digital_recording_control_data;
	clsDCCD->maximum_bitrate_flag			= pDCCInfo->maximum_bitrate_flag;
	clsDCCD->component_control_flag 		= pDCCInfo->component_control_flag;
	clsDCCD->copy_control_type				= pDCCInfo->copy_control_type;
	clsDCCD->APS_control_data				= pDCCInfo->APS_control_data;
	clsDCCD->maximum_bitrate				= pDCCInfo->maximum_bitrate;
	clsDCCD->sub_info = 0;

	if( pDCCInfo->sub_info )
	{
		SubDigitalCopyControlDescriptor* subClsDCCD;
		SubDigitalCopyControlDescriptor* backup_subClsDCCD;
		SUB_T_DESC_DCC* sub_info = pDCCInfo->sub_info;

		while(sub_info)
		{
			subClsDCCD = (SubDigitalCopyControlDescriptor*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(SubDigitalCopyControlDescriptor));
			if(0 == subClsDCCD)
			{
				ALOGE("[%s][%d] : SubDigitalCopyControlDescriptor malloc fail !!!!!\n", __func__, __LINE__);
				return -4;
			}
			memset(subClsDCCD, 0, sizeof(SubDigitalCopyControlDescriptor));

			subClsDCCD->component_tag				   = sub_info->component_tag;
			subClsDCCD->digital_recording_control_data = sub_info->digital_recording_control_data;
			subClsDCCD->maximum_bitrate_flag		   = sub_info->maximum_bitrate_flag;
			subClsDCCD->copy_control_type			   = sub_info->copy_control_type;
			subClsDCCD->APS_control_data			   = sub_info->APS_control_data;
			subClsDCCD->maximum_bitrate 			   = sub_info->maximum_bitrate;

			if(0 == clsDCCD->sub_info)
			{
				clsDCCD->sub_info = subClsDCCD;
				backup_subClsDCCD = subClsDCCD;
			}
			else
			{
				backup_subClsDCCD->pNext = subClsDCCD;
				backup_subClsDCCD = subClsDCCD;
			}

			sub_info = sub_info->pNext;
		}
	}

	*pDCCDInfo = clsDCCD;

	return 0;
}

int ISDBTPlayer::getContentAvailabilityDescriptor(unsigned short usServiceID,
												unsigned char *copy_restriction_mode,
												unsigned char *image_constraint_token,
												unsigned char *retention_mode,
												unsigned char *retention_state,
												unsigned char *encryption_mode)
{
	unsigned char ucDescID = 0xde;	// 0xde = Content_Availability_Descriptor table id
	unsigned char cArg[5];

	ALOGE("[%s] start / usServiceID : 0x%x\n", __func__, usServiceID);

	if(TCCDxBProc_GetCur_CADescriptorInfo(usServiceID, ucDescID, (void *)cArg))
	{
		ALOGE("%s[%d] can't find serviceID[0x%x]", __func__, __LINE__, usServiceID);
		return -1;
	}

	*copy_restriction_mode  = cArg[0];
	*image_constraint_token	= cArg[1];
	*retention_mode			= cArg[2];
	*retention_state		= cArg[3];
	*encryption_mode		= cArg[4];

	return 0;
}

int ISDBTPlayer::getVideoState(void)
{
	ALOGE("[%s] start\n", __func__);

	return TCCDxBProc_GetVideoState();
}

int ISDBTPlayer::getSearchState(void)
{
	ALOGE("[%s] start\n", __func__);

	return TCCDxBProc_GetSearchState();
}

int ISDBTPlayer::setDataServiceStart(void)
{
	ALOGE("[%s] start\n", __func__);

	return TCCDxBProc_DataServiceStart();
}

int ISDBTPlayer::customFilterStart(int iPID, int iTableID)
{
	ALOGE("[%s] start / iPID : 0x%x, iTableID : 0x%x\n", __func__, iPID, iTableID);

	return TCCDxBProc_CustomFilterStart(iPID, iTableID);
}

int ISDBTPlayer::customFilterStop(int iPID, int iTableID)
{
	ALOGE("[%s] start / iPID : 0x%x, iTableID : 0x%x\n", __func__, iPID, iTableID);

	return TCCDxBProc_CustomFilterStop(iPID, iTableID);
}

int ISDBTPlayer::EWS_start(int mainRowID, int subRowID, int audioIndex, int videoIndex, int audioMode, int raw_w, int raw_h, int view_w, int view_h, int ch_index)
{
	int err = 0;
	int iChannelNumber=0, iStPID=0xFFFF, iSiPID=0xFFFF, iServiceID=-1;
	int iChannelSubNumber=-1, iStSubPID=-1, iSiSubPID=-1, iServiceSubID=-1;

	int iServiceType=0xFFFF, iPCRPID=0, iPMTPID= 0, iCAECMPID=0, iACECMPID=0, iNetworkID=0, iAudioTotalCount=0, iVideoTotalCount=0;
	int iServiceSubType=-1, iPCRSubPID=-1, iPMTSubPID= 0, iCAECMSubPID=0, iACECMSubPID=0, iSubAudioTotalCount=-1, iSubVideoTotalCount=-1;
	int iPreset=0;

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

	int row_id;
	int audioMainIndex = audioIndex;
	int audioSubIndex = audioIndex;
	int videoMainIndex = videoIndex;
	int videoSubIndex = videoIndex;

	unsigned int uiState, uiSubState;

	ALOGE("[%s] start / mainRowID : %d, subRowID : %d, audioIndex : %d, videoIndex : %d, audioMode : %d\n",
		__func__, mainRowID, subRowID, audioIndex, videoIndex, audioMode);
	ALOGE("[%s] start / raw_w : %d, raw_h : %d, view_w : %d, view_h : %d, ch_index : %d\n",
		__func__, raw_w, raw_h, view_w, view_h, ch_index);

	uiState = TCCDxBProc_GetMainState();

	if(uiState == TCCDXBPROC_STATE_SCAN)
	{
		ALOGE("%s:%d Error invalid state(%d)", __func__, __LINE__, uiState);
		return -2;
	}

	if(uiState == TCCDXBPROC_STATE_AIRPLAY)
	{
		err = TCCDxBProc_Stop();
		if (err != TCCDXBPROC_SUCCESS)
		{
			ALOGE("[%s] TCCDxBProc_Stop returns Error(%d), Error !!!!!\n", __func__, err);
			return -4;
		}
	}
	else if(uiState == TCCDXBPROC_STATE_HANDOVER)
	{
		TCCDxBProc_StopHandover();
	}

	if ((ch_index == 0 && mainRowID < 0) || (ch_index == 1 && subRowID < 0))
	{
		ALOGE("In %s, invalid channel information(%d, %d)\n", __func__, mainRowID, subRowID);
	}

	if (ch_index == 0)	//fullseg
		row_id = mainRowID;
	else
		row_id = subRowID;

	// check whether a channel info is made from regional preset
	err = tcc_db_get_preset(row_id, &iChannelNumber, &iPreset);
	if (err < 0)
	{
		ALOGE("In %s, no row(%d) in channel\n", __func__, mainRowID);
		return -1;
	}

	if (iPreset == 1) //if channel info is made from regional preset.
	{
		ALOGD("In %s (%d), this channel was preset\n", __func__, __LINE__);

		//try manual scan for iChannelNumber
		TCCDxBProc_Stop();
		if (TCCDxBProc_ScanOneChannel (iChannelNumber) != 0)
		{
			//if can't find a channel on specified channel frequency
			ALOGE("In %s(%d), return because fail scan one channel(%d)\n",__func__, __LINE__, iChannelNumber);
			return -1;
		}
		else
		{
			err = tcc_db_get_preset(row_id, &iChannelNumber, &iPreset);
			if (iPreset == 1) //In case that the channel informaiton was made from regional preset and it can't find the channel on air
			{
				ALOGE("In %s, can't tune the preset channel\n", __func__);
				return -1;
			}
		}
	}

	if (mainRowID > 0)
	{
		// get main channel info
		err = tcc_db_get_channel(mainRowID,
								&iChannelNumber, piAudioPID, piVideoPID,
								&iStPID, &iSiPID,
								&iServiceType, &iServiceID,
								&iPMTPID, &iCAECMPID, &iACECMPID, &iNetworkID,
								piAudioStreamType, piVideoStreamType,
								piAudioComponentTag, piVideoComponentTag,
								&iPCRPID, &iAudioTotalCount, &iVideoTotalCount,
								&audioIndex, &audioMainIndex, &videoIndex, &videoMainIndex);
		if (err < 0)
		{
			ALOGE("In %s(%d), can't find main channel info(%d)\n", __func__, __LINE__, mainRowID);
			return -1;
		}
	}

	// get sub channel info
	if(subRowID > 0)
	{
		err = tcc_db_get_channel(subRowID,
								&iChannelSubNumber, piAudioSubPID, piVideoSubPID,
								&iStSubPID, &iSiSubPID,
								&iServiceSubType, &iServiceSubID,
								&iPMTSubPID, &iCAECMSubPID, &iACECMSubPID, &iNetworkID,
								piAudioSubStreamType, piVideoSubStreamType,
								piAudioSubComponentTag, piVideoSubComponentTag,
								&iPCRSubPID, &iSubAudioTotalCount, &iSubVideoTotalCount,
								&audioIndex, &audioSubIndex, &videoIndex, &videoSubIndex);
		if (err < 0)
		{
			ALOGE("In %s(%d), can't find sub channel info(%d)\n", __func__, __LINE__, subRowID);
		}
	}

	// set channel whith ews only mode
	TCCDxBProc_EWSStart(iChannelNumber,
					iAudioTotalCount, (unsigned int*)piAudioPID, iVideoTotalCount, (unsigned int*)piVideoPID, (unsigned int*)piAudioStreamType, (unsigned int*)piVideoStreamType,
					iSubAudioTotalCount, (unsigned int*)piAudioSubPID, iSubVideoTotalCount, (unsigned int*)piVideoSubPID, (unsigned int*)piAudioSubStreamType, (unsigned int*)piVideoSubStreamType,
					iPCRPID, iPCRSubPID, iStPID, iSiPID, iServiceID, iPMTPID, iCAECMPID, iACECMPID, iNetworkID,
					iStSubPID, iSiSubPID, iServiceSubID, iPMTSubPID,
					audioIndex, audioMainIndex, audioSubIndex,
					videoIndex, videoMainIndex, videoSubIndex,
					audioMode, iServiceType, raw_w, raw_h, view_w, view_h,
					ch_index);
	return 0;
}

/* Noah / 20171026 / Added EWS_start_withChNum function for IM478A-52 (database less tuning)
  * channelNumber                = index of frequency table. Ex) 0 : 473.143MHz, 1 : 479.143MHz ...
  * serviceID_Fseg               = service_id of Fseg
  * serviceID_1seg               = service_id of 1seg
  * audioMode                    = 0 : DUALMONO_LEFT, 1 : DUALMONO_RIGHT, 2 : DUALMONO_LEFTNRIGHT
  * raw_w, raw_h, vidw_w, view_h = size of Android View
  * ch_index                     = Main/Sub channel to be displayed. 0 : Fseg, 1 : 1seg
  */
int ISDBTPlayer::EWS_start_withChNum(int channelNumber, int serviceID_Fseg, int serviceID_1seg, int audioMode, int raw_w, int raw_h, int view_w, int view_h, int ch_index)
{
	int err = 0;
	int iChannelNumber=0, iStPID=0xFFFF, iSiPID=0xFFFF, iServiceID=-1;
	int iChannelSubNumber=-1, iStSubPID=-1, iSiSubPID=-1, iServiceSubID=-1;

	int iServiceType=0xFFFF, iPCRPID=0, iPMTPID= 0, iCAECMPID=0, iACECMPID=0, iNetworkID=0, iAudioTotalCount=0, iVideoTotalCount=0;
	int iServiceSubType=-1, iPCRSubPID=-1, iPMTSubPID= 0, iCAECMSubPID=0, iACECMSubPID=0, iSubAudioTotalCount=-1, iSubVideoTotalCount=-1;
	int iPreset=0;

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

	// Noah / IM478A-52
	int audioIndex = -1;    // -1 : default Audio index
	int videoIndex = -1;    // -1 : default Video index

	int row_id;
	int audioMainIndex = audioIndex;
	int audioSubIndex = audioIndex;
	int videoMainIndex = videoIndex;
	int videoSubIndex = videoIndex;

	unsigned int uiState, uiSubState;

	// Noah / IM478A-52
	int mainRowID = 0;
	int subRowID = 0;

	ALOGE("[%s] start / channelNumber : %d, serviceID_Fseg : 0x%x, serviceID_1seg : 0x%x, audioMode : %d\n",
		__func__, channelNumber, serviceID_Fseg, serviceID_1seg, audioMode);
	ALOGE("[%s] start / raw_w : %d, raw_h : %d, view_w : %d, view_h : %d, ch_index : %d\n",
		__func__, raw_w, raw_h, view_w, view_h, ch_index);

	uiState = TCCDxBProc_GetMainState();

	if(uiState == TCCDXBPROC_STATE_SCAN)
	{
		ALOGE("%s:%d Error invalid state(%d)", __func__, __LINE__, uiState);
		return -2;
	}

	if(uiState == TCCDXBPROC_STATE_AIRPLAY)
	{
		err = TCCDxBProc_Stop();
		if (err != TCCDXBPROC_SUCCESS)
		{
			ALOGE("[%s] TCCDxBProc_Stop returns Error(%d), Error !!!!!\n", __func__, err);
			return -4;
		}
	}
	else if(uiState == TCCDXBPROC_STATE_HANDOVER)
	{
		TCCDxBProc_StopHandover();
	}

	// Noah / IM478A-52
	{
		if (serviceID_Fseg)
			mainRowID = tcc_db_get_channel_rowid_new(channelNumber, serviceID_Fseg);

		if (serviceID_1seg)
			subRowID = tcc_db_get_channel_rowid_new(channelNumber, serviceID_1seg);

		ALOGE("[%s] mainRowID : %d, subRowID : %d\n", __func__, mainRowID, subRowID);
	}

	if ((ch_index == 0 && mainRowID < 0) || (ch_index == 1 && subRowID < 0))
	{
		ALOGE("In %s, invalid channel information(%d, %d)\n", __func__, mainRowID, subRowID);
	}

	if (ch_index == 0)	//fullseg
		row_id = mainRowID;
	else
		row_id = subRowID;

	// check whether a channel info is made from regional preset
	err = tcc_db_get_preset(row_id, &iChannelNumber, &iPreset);
	if (err < 0)
	{
		ALOGE("In %s, no row(%d) in channel\n", __func__, mainRowID);
		return -1;
	}

	if (iPreset == 1) //if channel info is made from regional preset.
	{
		ALOGD("In %s (%d), this channel was preset\n", __func__, __LINE__);

		//try manual scan for iChannelNumber
		TCCDxBProc_Stop();
		if (TCCDxBProc_ScanOneChannel (iChannelNumber) != 0)
		{
			//if can't find a channel on specified channel frequency
			ALOGE("In %s(%d), return because fail scan one channel(%d)\n",__func__, __LINE__, iChannelNumber);
			return -1;
		}
		else
		{
			err = tcc_db_get_preset(row_id, &iChannelNumber, &iPreset);
			if (iPreset == 1) //In case that the channel informaiton was made from regional preset and it can't find the channel on air
			{
				ALOGE("In %s, can't tune the preset channel\n", __func__);
				return -1;
			}
		}
	}

	if (mainRowID > 0)
	{
		// get main channel info
		err = tcc_db_get_channel(mainRowID,
								&iChannelNumber, piAudioPID, piVideoPID,
								&iStPID, &iSiPID,
								&iServiceType, &iServiceID,
								&iPMTPID, &iCAECMPID, &iACECMPID, &iNetworkID,
								piAudioStreamType, piVideoStreamType,
								piAudioComponentTag, piVideoComponentTag,
								&iPCRPID, &iAudioTotalCount, &iVideoTotalCount,
								&audioIndex, &audioMainIndex, &videoIndex, &videoMainIndex);
		if (err < 0)
		{
			ALOGE("In %s(%d), can't find main channel info(%d)\n", __func__, __LINE__, mainRowID);
			return -1;
		}
	}

	// get sub channel info
	if(subRowID > 0)
	{
		err = tcc_db_get_channel(subRowID,
								&iChannelSubNumber, piAudioSubPID, piVideoSubPID,
								&iStSubPID, &iSiSubPID,
								&iServiceSubType, &iServiceSubID,
								&iPMTSubPID, &iCAECMSubPID, &iACECMSubPID, &iNetworkID,
								piAudioSubStreamType, piVideoSubStreamType,
								piAudioSubComponentTag, piVideoSubComponentTag,
								&iPCRSubPID, &iSubAudioTotalCount, &iSubVideoTotalCount,
								&audioIndex, &audioSubIndex, &videoIndex, &videoSubIndex);
		if (err < 0)
		{
			ALOGD("In %s(%d), can't find sub channel info(%d)\n", __func__, __LINE__, subRowID);
		}
	}

	// set channel whith ews only mode
	TCCDxBProc_EWSStart(iChannelNumber,
					iAudioTotalCount, (unsigned int*)piAudioPID, iVideoTotalCount, (unsigned int*)piVideoPID, (unsigned int*)piAudioStreamType, (unsigned int*)piVideoStreamType,
					iSubAudioTotalCount, (unsigned int*)piAudioSubPID, iSubVideoTotalCount, (unsigned int*)piVideoSubPID, (unsigned int*)piAudioSubStreamType, (unsigned int*)piVideoSubStreamType,
					iPCRPID, iPCRSubPID, iStPID, iSiPID, iServiceID, iPMTPID, iCAECMPID, iACECMPID, iNetworkID,
					iStSubPID, iSiSubPID, iServiceSubID, iPMTSubPID,
					audioIndex, audioMainIndex, audioSubIndex,
					videoIndex, videoMainIndex, videoSubIndex,
					audioMode, iServiceType, raw_w, raw_h, view_w, view_h,
					ch_index);

	return 0;
}

int ISDBTPlayer::EWS_clear(void)
{
	ALOGE("[%s] start\n", __func__);

	return TCCDxBProc_EWSClear();
}

int ISDBTPlayer::Set_Seamless_Switch_Compensation(int onoff, int interval, int strength, int ntimes, int range, int gapadjust, int gapadjust2, int multiplier)
{
	ALOGE("[%s] start / onoff:%d, interval:%d, strength:%d, ntimes:%d, range:%d, gapadjust:%d, gapadjust2:%d, multiplier:%d\n", __func__, onoff, interval, strength, ntimes, range, gapadjust, gapadjust2, multiplier);
	return TCCDxBProc_SetSeamlessSwitchCompensation(onoff, interval, strength, ntimes, range, gapadjust, gapadjust2, multiplier);
}

int ISDBTPlayer::getSTC(unsigned int *hi_data, unsigned int *lo_data)
{
	int result;

	ALOGE("[%s] start\n", __func__);

	result = TCCDxBProc_DS_GetSTC(hi_data, lo_data);

	return result;
}

int ISDBTPlayer::getServiceTime(unsigned int *year, unsigned int *month, unsigned int *day, unsigned int *hour, unsigned int *min, unsigned int *sec)
{
	int err = 0;

	ALOGE("[%s] start\n", __func__);

	err = TCCDxBProc_DS_GetServiceTime(year, month, day, hour, min, sec);
	return err;
}

int ISDBTPlayer::getContentID(unsigned int *contentID, unsigned int *associated_contents_flag)
{
	int err = 0;
	int iChannelNumber, iServiceID, iServiceSubID;
	unsigned int state;

	ALOGE("[%s] start\n", __func__);

	*contentID = 0;

	state = TCCDxBProc_GetMainState();

	if(	state == TCCDXBPROC_STATE_AIRPLAY)
	{
		TCCDxBProc_GetCurrentChannel(&iChannelNumber, &iServiceID, &iServiceSubID);
		tcc_manager_demux_get_contentid(contentID, associated_contents_flag);
		err = 0;
	}
	else
	{
		err = -2;
	}

	return err;
}

int ISDBTPlayer::checkExistComponentTagInPMT(int target_component_tag)
{
	int err = 0;

	ALOGE("[%s] start / target_component_tag : 0x%x\n", __func__, target_component_tag);

	err = TCCDxBProc_DS_CheckExistComponentTagInPMT(target_component_tag);

	return err;
}

int ISDBTPlayer::getSeamlessValue(int *state, int *cval, int *pval)
{
	int ret;
	ret = TCCDxBProc_GetSeamlessValue(state, cval, pval);
	return ret;
}

void ISDBTPlayer::notify(int msg, int ext1, int ext2, void *obj)
{
	ISDBTPlayerListener *listener = mClientListener;

	ALOGE("[%s] start / msg : %d, ext1 : %d, ext2 : %d\n", __func__, msg, ext1, ext2);

	if(TCCDxBProc_GetMainState() == TCCDXBPROC_STATE_EWSMODE) {
		if(msg != ISDBTPlayer::EVENT_EMERGENCY_INFO
		&& msg != ISDBTPlayer::EVENT_UPDATE_CUSTOM_TUNER) {
			return;
		}
	}

	if (listener != 0)
	{
		pthread_mutex_lock(&mNotifyLock);
		listener->notify(msg, ext1, ext2, obj);
		pthread_mutex_unlock(&mNotifyLock);
	}
	else if (mListener != 0)
	{
		pthread_mutex_lock(&mNotifyLock);
		mListener(msg, ext1, ext2, obj);
		pthread_mutex_unlock(&mNotifyLock);
	}
}

void notify_search_percent_update(void *pResult)
{
	int *pData = NULL, iPercent = 0, iChannel = 0;
	ISDBTPlayer *player = NULL;

	player = gpisdbtInstance;
	if (player == NULL) {
		return;
	}

	pData = (int*)pResult;
	iPercent = pData[0];
	iChannel = pData[1];

	ALOGE("[%s] start / iPercent : %d\n", __func__, iPercent);

	player->notify(ISDBTPlayer::EVENT_SEARCH_PERCENT, iPercent, iChannel, 0);
}

void notify_search_complete(void *pResult)
{
	int *pData = (int*)pResult;
	int msg = pData[0];
	ISDBTPlayer *player = NULL;

	ALOGE("[%s] start / msg : %d\n", __func__, msg);

	player = gpisdbtInstance;
	if (player == NULL)	{
		return;
	}

	player->notify(ISDBTPlayer::EVENT_SEARCH_COMPLETE, msg, 0, 0);
}

void notify_video_frame_output_start(void *pResult)
{
	int msg = (int)pResult;
	ISDBTPlayer *player = NULL;

	ALOGE("[%s] start\n", __func__);

	player = gpisdbtInstance;
	if (player == NULL) {
		return;
	}

	player->notify(ISDBTPlayer::EVENT_VIDEO_OUTPUT, msg, gPlayingServiceID, 0);

	tcc_dxb_timecheck_set("switch_channel", "application", TIMECHECK_STOP);
	tcc_dxb_timecheck_printlogAVG();
}

void notify_video_error(void *pResult)
{
	int msg = (int)pResult;
	ISDBTPlayer *player = NULL;

	ALOGE("[%s] start\n", __func__);

	player = gpisdbtInstance;
	if (player == NULL) {
		return;
	}

	player->notify(ISDBTPlayer::EVENT_VIDEO_ERROR, msg, gPlayingServiceID, 0);
}


void notify_channel_update(void *pResult)
{
	int request;
	ISDBTPlayer *player = NULL;

	ALOGE("[%s] start\n", __func__);

	player = gpisdbtInstance;
	if (player == NULL) {
		return;
	}

	request = *((int *)pResult);
	player->notify(ISDBTPlayer::EVENT_CHANNEL_UPDATE, request, gPlayingServiceID, 0);
}

void notify_emergency_info_update(void *pResult)
{
	EWSData ewsdata;
	int *pArg, i;
	ISDBTPlayer *player = NULL;

	ALOGE("[%s] start\n", __func__);

	player = gpisdbtInstance;
	if (player == NULL) {
		return;
	}

	pArg = (int*)pResult;

	ewsdata.serviceID = pArg[1];
	ewsdata.startendflag = pArg[2];
	ewsdata.signaltype = pArg[3];
	ewsdata.area_code_length = pArg[4];
	for (i=0; i < ewsdata.area_code_length; i++)
		ewsdata.localcode[i] = pArg[5+i];
	if (i < 256) {
		for (; i < 256; i++)
			ewsdata.localcode[i] = 0;
	}

	if (ewsdata.area_code_length > 0)
	{
		ALOGE("[notify_emergency_info_update] status=0x%X, service_id=%d, start_end=%d, signal_type=0x%X, length=%d, area_code=0x%x\n", pArg[0], pArg[1], pArg[2], pArg[3], pArg[4], pArg[5]);
	}
	else
	{
		ALOGE("[notify_emergency_info_update] status=0x%X, service_id=%d, start_end=%d, signal_type=0x%X, length=%d\n", pArg[0], pArg[1], pArg[2], pArg[3], pArg[4]);
	}

	if(pArg[0] == 1) {
		TCCDxBProc_SetSubState(TCCDXBPROC_AIRPLAY_EWS);
	}
	else {
		TCCDxBProc_ClearSubState(TCCDXBPROC_AIRPLAY_EWS);
	}

	player->notify(ISDBTPlayer::EVENT_EMERGENCY_INFO, pArg[0], gCurrentChannel, (void *)&ewsdata);
}

void notify_subtitle_linux_update(void *pResult)
{
	SubtitleDataLinux subtData;
	int *pArg;
	ISDBTPlayer *player = NULL;

	ALOGE("[%s] start\n", __func__);

	player = gpisdbtInstance;
	if (player == NULL){
		return;
	}

	pArg = (int*)pResult;
	subtData.internal_sound_subtitle_index = pArg[0]; //internal sound index
	subtData.mixed_subtitle_phy_addr = pArg[1]; //mixed subtitle for phy addr
	subtData.mixed_subtitle_size = pArg[2]; //mixed subtitle phy memory size about one handle
	if(subtData.mixed_subtitle_size == 0 || subtData.mixed_subtitle_phy_addr == NULL){
		return;
	}

    ALOGE("[notify_subtitle_linux_update] internal_sound_index[%d] mixed_subtitle_phy_addr[%p] mixed_subtitle_size[%d]\n", \
		subtData.internal_sound_subtitle_index, subtData.mixed_subtitle_phy_addr, subtData.mixed_subtitle_size);

	player->notify(ISDBTPlayer::EVENT_SUBTITLE_UPDATE, gCurrentChannel, gPlayingServiceID, (void *)&subtData);
}

void notify_video_definition_update(void *pResult)
{
	TCC_DVB_VIDEO_DEFINITIONTYPE *pVideoData_Cmd = (TCC_DVB_VIDEO_DEFINITIONTYPE *)pResult;
	VideoDefinitionData videoData;
	ISDBTPlayer *player = NULL;

	ALOGE("[%s] start\n", __func__);

	player = gpisdbtInstance;
	if (player == NULL){
		return;
	}

	/* initialize local variable */
	memset(&videoData, 0, sizeof(VideoDefinitionData));
	videoData.main_DecoderID = -1;
	videoData.sub_DecoderID = -1;

	videoData.width        = pVideoData_Cmd->nFrameWidth;
	videoData.height       = pVideoData_Cmd->nFrameHeight;
	videoData.aspect_ratio = pVideoData_Cmd->nAspectRatio;
	videoData.frame_rate = pVideoData_Cmd->nFrameRate;

	//solution for screen mode change bug
	if(pVideoData_Cmd->MainDecoderID == 0){
		videoData.main_DecoderID       =  pVideoData_Cmd->MainDecoderID;
		videoData.frame_rate = pVideoData_Cmd->nFrameRate;
	}

	if(pVideoData_Cmd->SubDecoderID == 1){
		videoData.sub_width   = pVideoData_Cmd->nSubFrameWidth;
		videoData.sub_height  = pVideoData_Cmd->nSubFrameHeight;
		videoData.sub_DecoderID       =  pVideoData_Cmd->SubDecoderID;
		videoData.sub_aspect_ratio = pVideoData_Cmd->nSubAspectRatio;
	}

	videoData.DisplayID = pVideoData_Cmd->DisplayID;

	player->notify(ISDBTPlayer::EVENT_VIDEODEFINITION_UPDATE, gCurrentChannel, gPlayingServiceID, (void *)&videoData);
}

void notify_epg_update(void *pResult)
{
	int *pData, iServiceID, iTableID;
	ISDBTPlayer *player = NULL;

	ALOGE("[%s] start\n", __func__);

	player = gpisdbtInstance;
	if (player == NULL) {
		return;
	}

	pData = (int*)pResult;
	iServiceID = pData[0];
	iTableID = pData[1];

	player->notify(ISDBTPlayer::EVENT_EPG_UPDATE, iServiceID, iTableID, 0);
}

void notify_db_information_update(void *pDBInfo)
{
	int i;
	DBInfoData dbinfoData;
	CHANNEL_HEADER_INFORMATION *pChHeaderInfo = (CHANNEL_HEADER_INFORMATION *)pDBInfo;
	ISDBTPlayer *player = gpisdbtInstance;

	ALOGE("[%s] start\n", __func__);

	if (player == NULL) {
		return;
	}

	//Channel
	{
		dbinfoData.dbCh.channelNumber = pChHeaderInfo->dbCh.channelNumber;
		dbinfoData.dbCh.countryCode = pChHeaderInfo->dbCh.countryCode;
		dbinfoData.dbCh.audioPID = pChHeaderInfo->dbCh.audioPID;
		dbinfoData.dbCh.videoPID = pChHeaderInfo->dbCh.videoPID;
		dbinfoData.dbCh.stPID = pChHeaderInfo->dbCh.stPID;
		dbinfoData.dbCh.siPID = pChHeaderInfo->dbCh.siPID;
		dbinfoData.dbCh.PMT_PID = pChHeaderInfo->dbCh.PMT_PID;
		dbinfoData.dbCh.remoconID = pChHeaderInfo->dbCh.remoconID;
		dbinfoData.dbCh.serviceType = pChHeaderInfo->dbCh.serviceType;
		dbinfoData.dbCh.serviceID = pChHeaderInfo->dbCh.serviceID;
		dbinfoData.dbCh.regionID = pChHeaderInfo->dbCh.regionID;
		dbinfoData.dbCh.threedigitNumber = pChHeaderInfo->dbCh.threedigitNumber;
		dbinfoData.dbCh.TStreamID = pChHeaderInfo->dbCh.TStreamID;
		dbinfoData.dbCh.berAVG = pChHeaderInfo->dbCh.berAVG;
		dbinfoData.dbCh.channelName = (short*)pChHeaderInfo->dbCh.channelName;
		dbinfoData.dbCh.preset = pChHeaderInfo->dbCh.preset;
		dbinfoData.dbCh.networkID = pChHeaderInfo->dbCh.networkID;
		dbinfoData.dbCh.areaCode = pChHeaderInfo->dbCh.areaCode;
	}

	//Service
	{
		dbinfoData.dbService.uiPCRPID = pChHeaderInfo->dbService.uiPCRPID;
	}

	// Audio DB
	for(i=0; i<MAX_AUDIOTRACK_SUPPORT; i++)
	{
		dbinfoData.dbAudio[i].uiServiceID = pChHeaderInfo->dbAudio[i].uiServiceID;
		dbinfoData.dbAudio[i].uiCurrentChannelNumber = pChHeaderInfo->dbAudio[i].uiCurrentChannelNumber;
		dbinfoData.dbAudio[i].uiCurrentCountryCode = pChHeaderInfo->dbAudio[i].uiCurrentCountryCode;
		dbinfoData.dbAudio[i].uiAudioPID = pChHeaderInfo->dbAudio[i].uiAudio_PID;
		dbinfoData.dbAudio[i].uiAudioIsScrambling = pChHeaderInfo->dbAudio[i].ucAudio_IsScrambling;
		dbinfoData.dbAudio[i].uiAudioStreamType = pChHeaderInfo->dbAudio[i].ucAudio_StreamType;
		dbinfoData.dbAudio[i].uiAudioType = pChHeaderInfo->dbAudio[i].ucAudioType;
		dbinfoData.dbAudio[i].pucacLangCode = (char *)pChHeaderInfo->dbAudio[i].acLangCode; // address
	}

	player->notify(ISDBTPlayer::EVENT_DBINFO_UPDATE, 0, 0, (void *)&dbinfoData);
}

void notify_autosearch_update (void *ptr)
{
	ISDBTPlayer *player = gpisdbtInstance;
	int	status, percent, *param;

	ALOGE("[%s] start\n", __func__);

	if (player != NULL) {
		param = (int*)ptr;
		status = *param;
		percent = *(param+1);
		player->notify(ISDBTPlayer::EVENT_AUTOSEARCH_UPDATE, status, percent, ptr);
	}
}

void notify_autosearch_done (void *ptr)
{
	ISDBTPlayer *player = gpisdbtInstance;
	int	status;

	ALOGE("[%s] start\n", __func__);

	if (player != NULL) {
		status = *(int*)ptr;
		player->notify(ISDBTPlayer::EVENT_AUTOSEARCH_DONE, status, 0, ptr);
	}

	/* Noah / 2017.04.07 / IM478A-13, Improvement of the Search API(Auto Search)
		Request : If it finds ISDB-T service while auto searching, DxB automatically plays the service.
		Previous Sequence
			1. search( 5, , , , );	  -> Start 'Auto Search'
			2. Searching in progress . . .
			3. Finding service and then finishing searching and then sending EVENT_AUTOSEARCH_DONE to application.
			4. setChannel( X, Y, , , , ... ); is called by application.
			5. Playing the X service.
		New Sequence
			Remove 4th step above.
			DxB plays the X service automatically.
		Implement
			1. Add if statement as below.
	*/
	if(status == 1)    // if autosearch finds some channel, DxB plays the channel automatically.
	{
		int iArraySvcRowID[32 + 4] = { 0, };
		int mainRowId = 0;
		int subRowId = 0;

		memcpy(iArraySvcRowID, ptr, 32+4);    // About 'ptr', Refer to 'EVENT_AUTOSEARCH_DONE' of 'Linux_ISDBT_TCC897x_API_specification.xls'.

		mainRowId = iArraySvcRowID[1];    // row_id(_id of ChannelDB) of fullseg service which can be selected as Main channel
		subRowId = iArraySvcRowID[2];     // row_id of oneseg service which can be selected as Sub channel

		ALOGE("[%s] setChannel is called automatically / mainRowId : %d, subRowId : %d\n", __func__, mainRowId, subRowId);

		player->setChannel(mainRowId, subRowId, 0, 0, 0, 0, 0, 0, 0, 0);
	}
}

void notify_desc_update(void *arg)
{
	ISDBTPlayer *player = gpisdbtInstance;

	ALOGE("[%s] start\n", __func__);

	if (player != NULL) {
		int *pArg = (int *)arg;
		switch(pArg[1])
		{
			case 0xc1:	//DigitalCopyControlDescriptor Table ID
			{
				T_DESC_DCC* descDcc = (T_DESC_DCC*)pArg[2];
				if(!descDcc)
				{
					printf("[%s][%d] pArg[2] is null Error !!!!!\n", __func__, __LINE__);
					return ;
				}

				DigitalCopyControlDescriptor* clsDCCD = (DigitalCopyControlDescriptor*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(DigitalCopyControlDescriptor));
				if(0 == clsDCCD)
				{
					ALOGE("[%s][%d] : DigitalCopyControlDescriptor malloc fail !!!!!\n", __func__, __LINE__);
					return ;
				}
				memset(clsDCCD, 0, sizeof(DigitalCopyControlDescriptor));

				clsDCCD->digital_recording_control_data = descDcc->digital_recording_control_data;
				clsDCCD->maximum_bitrate_flag           = descDcc->maximum_bitrate_flag;
				clsDCCD->component_control_flag         = descDcc->component_control_flag;
				clsDCCD->copy_control_type              = descDcc->copy_control_type;
				clsDCCD->APS_control_data               = descDcc->APS_control_data;
				clsDCCD->maximum_bitrate                = descDcc->maximum_bitrate;
				clsDCCD->sub_info = 0;

				if( descDcc->sub_info )
				{
					SubDigitalCopyControlDescriptor* subClsDCCD;
					SubDigitalCopyControlDescriptor* backup_subClsDCCD;
					SUB_T_DESC_DCC* sub_info = descDcc->sub_info;

					while(sub_info)
					{
						subClsDCCD = (SubDigitalCopyControlDescriptor*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(SubDigitalCopyControlDescriptor));
						if(0 == subClsDCCD)
						{
							ALOGE("[%s][%d] : SubDigitalCopyControlDescriptor malloc fail !!!!!\n", __func__, __LINE__);
							return ;
						}
						memset(subClsDCCD, 0, sizeof(SubDigitalCopyControlDescriptor));

						subClsDCCD->component_tag                  = sub_info->component_tag;
						subClsDCCD->digital_recording_control_data = sub_info->digital_recording_control_data;
						subClsDCCD->maximum_bitrate_flag           = sub_info->maximum_bitrate_flag;
						subClsDCCD->copy_control_type              = sub_info->copy_control_type;
						subClsDCCD->APS_control_data               = sub_info->APS_control_data;
						subClsDCCD->maximum_bitrate                = sub_info->maximum_bitrate;


						if(0 == clsDCCD->sub_info)
						{
							clsDCCD->sub_info = subClsDCCD;
							backup_subClsDCCD = subClsDCCD;
						}
						else
						{
							backup_subClsDCCD->pNext = subClsDCCD;
							backup_subClsDCCD = subClsDCCD;
						}

						sub_info = sub_info->pNext;
					}
				}

				//
				//////////////////////////////////////////////////////////////////////////////////////
				// Free the momery that is allocated in TCCDxBEvent_DESCUpdate/case 0xC1.

				if(descDcc->sub_info)
				{
					SUB_T_DESC_DCC* sub_info = descDcc->sub_info;
					while(sub_info)
					{
						SUB_T_DESC_DCC* temp_sub_info = sub_info->pNext;
						tcc_mw_free(__FUNCTION__, __LINE__, sub_info);
						sub_info = temp_sub_info;
					}
				}

				tcc_mw_free(__FUNCTION__, __LINE__, descDcc);

				// Free the momery that is allocated in TCCDxBEvent_DESCUpdate/case 0xC1.
				//////////////////////////////////////////////////////////////////////////////////////

				player->notify(ISDBTPlayer::EVENT_DESC_UPDATE, pArg[0], pArg[1], (void *)clsDCCD);

				//////////////////////////////////////////////////////////////////////////////////////
				// clsDCCD memory free
				/* Following codes about free clsDCCD memory are moved to CUI application.
				if(clsDCCD->sub_info)
				{
					SubDigitalCopyControlDescriptor* sub_info = clsDCCD->sub_info;
					while(sub_info)
					{
						SubDigitalCopyControlDescriptor* temp_sub_info = sub_info->pNext;
						tcc_mw_free(__FUNCTION__, __LINE__, sub_info);
						sub_info = temp_sub_info;
					}
				}

				tcc_mw_free(__FUNCTION__, __LINE__, clsDCCD);
				*/
				// clsDCCD memory free
				//////////////////////////////////////////////////////////////////////////////////////
			}
			break;
			case 0xde:	//ContentAvailabilityDescriptor Table ID
			{
				ContentAvailabilityDescriptor clsCONTA;
				clsCONTA.copy_restriction_mode  = (unsigned char)pArg[2];
				clsCONTA.image_constraint_token	= (unsigned char)pArg[3];
				clsCONTA.retention_mode			= (unsigned char)pArg[4];
				clsCONTA.retention_state		= (unsigned char)pArg[5];
				clsCONTA.encryption_mode		= (unsigned char)pArg[6];
				player->notify(ISDBTPlayer::EVENT_DESC_UPDATE, pArg[0], pArg[1], (void *)&clsCONTA);
			}
			break;
			default:
			break;
		}
	}
}

void notify_event_relay(void *arg)
{
	ISDBTPlayer *player = gpisdbtInstance;

	ALOGE("[%s] start\n", __func__);

	if (player != NULL) {
		player->notify(ISDBTPlayer::EVENT_EVENT_RELAY, gCurrentChannel, 0, arg);
	}
}

void notify_mvtv_update(void *arg)
{
	ISDBTPlayer *player = gpisdbtInstance;

	ALOGE("[%s] start\n", __func__);

	if (player != NULL) {
		int *pArg= (int *)arg;

		if(pArg[0] == 0) {
			TCCDxBProc_SetSubState(TCCDXBPROC_AIRPLAY_MULTVIEW);
		}
		else {
			TCCDxBProc_ClearSubState(TCCDXBPROC_AIRPLAY_MULTVIEW);
		}

		player->notify(ISDBTPlayer::EVENT_MVTV_UPDATE, gCurrentChannel, gPlayingServiceID, arg);
	}
}

void notify_serviceID_check(void *pResult)
{
	int *pArg;
	int valid_flag;
	ISDBTPlayer *player = gpisdbtInstance;

	ALOGE("[%s] start\n", __func__);

	if (player != NULL) {
		pArg = (int*)pResult;
		valid_flag = pArg[0];
		player->notify(ISDBTPlayer::EVENT_SERVICEID_CHECK, valid_flag, gCurrentChannel, 0);
	}
}

void notify_trmperror_update(void *pResult)
{
	int *pArg;
	int iStatus = 0;
	int iChannel = 0;
	ISDBTPlayer *player = NULL;

	ALOGE("[%s] start\n", __func__);

	player = gpisdbtInstance;
	if (player == NULL) {
		return;
	}

	if (pResult != NULL) {
		pArg = (int *)pResult;
		iStatus = pArg[0];
		iChannel = pArg[1];
	}

	player->notify(ISDBTPlayer::EVENT_TRMP_ERROR, iStatus, iChannel, 0);
}

void notify_customsearch_status (void *pResult)
{
	ISDBTPlayer *player = gpisdbtInstance;
	int *pArg;
	CustomSearchInfoData clsCSInfo;
	int	status;
	int *p, i, cnt;

	ALOGE("[%s] start\n", __func__);

	if (player != NULL) {
		if(pResult != NULL) {
			pArg = (int*)pResult;
			status = pArg[0];
			p = &pArg[1];
			clsCSInfo.status = *p++;
			clsCSInfo.ch = *p++;
			clsCSInfo.strength = *p++;
			clsCSInfo.tsid = *p++;
			clsCSInfo.fullseg_id = *p++;
			clsCSInfo.oneseg_id = *p++;
			clsCSInfo.total_cnt = *p++;
			for (i=0; i < clsCSInfo.total_cnt; i++)
				clsCSInfo.all_id[i] = *p++;
			if (i < 32) {
				for (;i<32;i++) clsCSInfo.all_id[i] = 0;
			}
			player->notify(ISDBTPlayer::EVENT_CUSTOMSEARCH_STATUS, status, 0, (void*)&clsCSInfo);

			/* --- debug log --- */
		#if 0
			ALOGE("[custom_search] arg=%d", status);
			ALOGD("[custom_search] status=%d", clsCSInfo.status);
			ALOGD("[custom_search] ch=%d", clsCSInfo.ch);
			ALOGD("[custom_search] strength=%d", clsCSInfo.strength);
			ALOGD("[custom_search] tsid=%d", clsCSInfo.tsid);
			ALOGD("[custom_search] fullseg_id=%d", clsCSInfo.fullseg_id);
			ALOGD("[custom_search] oneseg_id=%d", clsCSInfo.oneseg_id);
			ALOGD("[custom_search] total_svc=%d", clsCSInfo.total_cnt);
			for (i=0; i < clsCSInfo.total_cnt; i++)
				ALOGD("[custom_search] id[%d]=%d", i, clsCSInfo.all_id[i]);;
		#endif
		} else {
			DEBUG_MSG("In %s, parameter is null pointer", __func__);
		}
	}
}

void notify_specialservice_update (void *pResult)
{
	int status, _id, *pArg;;
	ISDBTPlayer *player = gpisdbtInstance;

	ALOGE("[%s] start\n", __func__);

	if (player != NULL && pResult != NULL) {
		pArg = (int*)pResult;
		status = pArg[0];
		_id = pArg[1];

		if(pArg[0] == 1) {
			TCCDxBProc_SetSubState(TCCDXBPROC_AIRPLAY_SPECIAL);
		}
		else {
			TCCDxBProc_ClearSubState(TCCDXBPROC_AIRPLAY_SPECIAL);
		}
		player->notify (ISDBTPlayer::EVENT_SPECIALSERVICE_UPDATE, status, _id, 0);
	}
}

void notify_av_index_update(void *pResult)
{
	AVIndexData avindexdata;
	int *pArg;
	ISDBTPlayer *player = NULL;

	player = gpisdbtInstance;
	if (player == NULL){
		return;
	}

	pArg = (int*)pResult;
	avindexdata.channelNumber	= pArg[0];
	avindexdata.networkID 		= pArg[1];
	avindexdata.serviceID		= pArg[2];
	avindexdata.serviceSubID	= pArg[3];
	avindexdata.audioIndex		= pArg[4];
	avindexdata.videoIndex		= pArg[5];

	ALOGE("[%s] start / channelNumber : %d, networkID : 0x%x, serviceID : 0x%x, serviceSubID : 0x%x, audioIndex : 0x%x, videoIndex : 0x%x\n",
		__func__,
		avindexdata.channelNumber,
		avindexdata.networkID,
		avindexdata.serviceID,
		avindexdata.serviceSubID,
		avindexdata.audioIndex,
		avindexdata.videoIndex);

	player->notify(ISDBTPlayer::EVENT_AV_INDEX_UPDATE, 0, 0, (void *)&avindexdata);
}

void notify_section_update(void *pResult)
{
	int *pArg;
	ISDBTPlayer *player = NULL;

	ALOGE("[%s] start\n", __func__);

	player = gpisdbtInstance;
	if (player == NULL){
		return;
	}

	pArg = (int*)pResult;

	player->notify(ISDBTPlayer::EVENT_SECTION_UPDATE, pArg[0], pArg[1], 0);
}

void notify_search_and_setchannel(void *pResult)
{
	SetChannelData setchanneldata;
	int status;
	int *pArg;
	ISDBTPlayer *player = NULL;

	ALOGE("[%s] start\n", __func__);

	player = gpisdbtInstance;
	if (player == NULL){
		return;
	}

	pArg = (int*)pResult;
	status = pArg[0];
	setchanneldata.countryCode = pArg[1];
	setchanneldata.channelNumber = pArg[2];
	setchanneldata.mainRowID = pArg[3];
	setchanneldata.subRowID = pArg[4];

	player->notify(ISDBTPlayer::EVENT_SEARCH_AND_SETCHANNEL, status, 0, (void *)&setchanneldata);
}

/* Original
void notify_detect_service_num_change(void)
*/
//8th Try
//David, To avoid Codesonar's warning, Dangerous Function Cast
//This function's prototype is different with pEventFunc
//So, change this function's prototype (arg : from void to void*)
void notify_detect_service_num_change(void *arg)
{
	ISDBTPlayer *player = NULL;

	ALOGE("[%s] start\n", __func__);

	player = gpisdbtInstance;
	if (player == NULL) {
		return;
	}

	player->notify(ISDBTPlayer::EVENT_DETECT_SERVICE_NUM_CHANGE, 0, 0, 0);
}

void notify_tuner_custom_event (void *pResult)
{
	ISDBTPlayer *player = NULL;

	ALOGE("[%s] start\n", __func__);

	player = gpisdbtInstance;
	if (player != NULL) {
		player->notify (ISDBTPlayer::EVENT_UPDATE_CUSTOM_TUNER, 0, 0, pResult);
	}
}

void notify_section_data_update(void *pResult)
{
	int *pArg;
	int iServiceID;
	int iTableID;
	SectionDataInfo sectiondatainfo;
	ISDBTPlayer *player = NULL;

	player = gpisdbtInstance;
	if (player == NULL) {
		return;
	}

	pArg = (int *)pResult;
	iServiceID = pArg[0];
	iTableID = pArg[1];
	sectiondatainfo.network_id = pArg[2];
	sectiondatainfo.transport_stream_id = pArg[3];
	sectiondatainfo.DataServicePID = pArg[4];
	sectiondatainfo.auto_start_flag = pArg[5];
	sectiondatainfo.rawdatalen = pArg[6];
	sectiondatainfo.rawdata = (unsigned char *)&pArg[7];

	ALOGE("[%s] start / iServiceID : 0x%x, iTableID : 0x%x\n", __func__, iServiceID, iTableID);

	player->notify (ISDBTPlayer::EVENT_SECTION_DATA_UPDATE, iServiceID, iTableID, (void *)&sectiondatainfo);
}

void notify_custom_filter_data_update(void *pResult)
{
	int *pArg;
	int iPID;
	unsigned int uiBufSize;
	unsigned char *pucBuf;
	ISDBTPlayer *player = NULL;

	player = gpisdbtInstance;
	if (player == NULL) {
		return;
	}

	pArg = (int *)pResult;
	iPID = pArg[0];
	uiBufSize = pArg[1];
	pucBuf = (unsigned char *)&pArg[2];

	ALOGE("[%s] start / iPID : 0x%x, uiBufSize : %d\n", __func__, iPID, uiBufSize);

	player->notify (ISDBTPlayer::EVENT_CUSTOM_FILTER_DATA_UPDATE, iPID, uiBufSize, pucBuf);
}

void notify_video_output_start_stop(void *pResult)
{
	int *pArg;
	int iStart;
	ISDBTPlayer *player = NULL;

	player = gpisdbtInstance;
	if (player == NULL) {
		return;
	}

	pArg = (int *)pResult;
	iStart = pArg[0];

	ALOGE("[%s] start / iStart : %d\n", __func__, iStart);

	player->notify(ISDBTPlayer::EVENT_SET_VIDEO_ONOFF, iStart, gPlayingServiceID, 0);
}

void notify_audio_output_start_stop(void *pResult)
{
	int *pArg;
	int iStart;
	ISDBTPlayer *player = NULL;

	player = gpisdbtInstance;
	if (player == NULL) {
		return;
	}

	pArg = (int *)pResult;
	iStart = pArg[0];

	ALOGE("[%s] start / iStart : %d\n", __func__, iStart);

	player->notify(ISDBTPlayer::EVENT_SET_AUDIO_ONOFF, iStart, gPlayingServiceID, 0);
}
