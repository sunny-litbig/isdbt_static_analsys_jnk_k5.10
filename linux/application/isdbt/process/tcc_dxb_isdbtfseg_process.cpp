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
/******************************************************************************
* include
******************************************************************************/
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h> /* for usleep()*/
#include <tcc_message.h>
#include <tcc_dxb_isdbtfseg_cmd_list.h>
#include "tcc_dxb_isdbtfseg_process.h"
#include "tcc_dxb_isdbtfseg_main.h"

#ifdef DEBUG_TCC_MALLOC_HOOKING_ENABLE
#include "tcc_memory_hooking.h"
#endif

#include <ISDBTPlayer.h>

#include <sqlite3.h>
#include <process_sub.h>

/******************************************************************************
* defines
******************************************************************************/
#define DEBUG_MSG_ON 1
#if DEBUG_MSG_ON
#define DEBUG_MSG(msg...)	printf(msg)
#else
#define DEBUG_MSG(msg...)
#endif

//#define FBDEV_FILENAME  "/dev/fb0"
#define	TCC_DXB_ISDBTFSEG_PROCESS_SLEEP_TIME		100000	//100ms

/******************************************************************************
* typedefs & structure
******************************************************************************/
typedef enum tag_TCC_DXB_ISDBTFSEG_PROCESS_ProcState_E
{
	TCC_DXB_ISDBTFSEG_PROC_STATE_NONE,
	TCC_DXB_ISDBTFSEG_PROC_STATE_IDLE,
	TCC_DXB_ISDBTFSEG_PROC_STATE_SCAN,
	TCC_DXB_ISDBTFSEG_PROC_STATE_PLAY,
	TCC_DXB_ISDBTFSEG_PROC_STATE_MAX
} TCC_DXB_ISDBTFSEG_PROCESS_ProcState_E;

typedef enum {
	NON_PLAY,
	RECORD_PLAY,
	AIR_PLAY,
	FILE_PLAY,
	CONTINUE_FILE_PLAY
} TCC_DXB_ISDBTFSEG_PROCESS_PlayState_E;

typedef enum {
	PLAY_CONTINUE,
	PLAY_THUMBNAIL,
	PLAY_SPEED,
} TCC_DXB_ISDBTFSEG_PROCESS_PlayMode_E;

typedef struct tag_TCC_DXB_ISDBTFSEG_PROCESS_Instance_T
{
	pthread_mutex_t	lockProcess;

	TCC_DXB_ISDBTFSEG_PROCESS_State_E			eState;
	TCC_DXB_ISDBTFSEG_PROCESS_ProcState_E		eProcState;
	TCC_DXB_ISDBTFSEG_PROCESS_PlayState_E		ePlayState;
	TCC_DXB_ISDBTFSEG_PROCESS_PlayMode_E		ePlayMode;

} TCC_DXB_ISDBTFSEG_PROCESS_Instance_T;


/******************************************************************************
* For subtitle dump
******************************************************************************/
//#define SUBTITLE_DUMP_INPUT_TO_FILE
#if defined(SUBTITLE_DUMP_INPUT_TO_FILE)
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <libpmap/pmap.h>
#include <sys/mman.h>

static FILE *gp_file_handle = NULL;
pmap_t pmem_info;
static int fd = -1;
unsigned int subtitle_buf_addr;

void subtitle_app_dump_file_open()
{
	static unsigned int index = 0;
	char szName[1024] = {0,};

	if(gp_file_handle == NULL){
		sprintf(szName, "/run/media/sda1/subtitle_app_dump_%03d.raw", index++);
		gp_file_handle = fopen(szName, "wb");
		if(gp_file_handle == NULL){
			printf("===>> subtitle dump file open error %d\n", gp_file_handle);
		}else{
			printf("===>> subtitle dump file open success(%s)\n", szName);
		}
	}
}

int subtitle_app_dump_file_write(int *data, int size)
{
	if(gp_file_handle != NULL){
		printf("===>> write start size [%d]\n", size);
		fwrite(data, 1, size, gp_file_handle);
		printf("===>> write end size [%d]\n", size);
		fclose(gp_file_handle);
		gp_file_handle = NULL;
		return 0;
	}

	if(gp_file_handle == NULL){
		sync();
	}
}

void subtitle_app_memory_mapping_init(unsigned int addr, int size)
{
	if(addr == NULL)
		return;

	if(fd == -1){
#if 0
		fd = open("/dev/ccfb", O_RDWR);
		pmap_get_info("overlay1", &pmem_info);
#else
		fd = open("/dev/wmixer1", O_RDWR);
		pmap_get_info("subtitle", &pmem_info);
#endif

		printf("<===== size[%d], buf_phy[%p]=====>\n", size, addr);
		subtitle_buf_addr = (unsigned int)mmap(0, pmem_info.size, (PROT_READ|PROT_WRITE), MAP_SHARED, fd, pmem_info.base);
		if(subtitle_buf_addr == (unsigned int)MAP_FAILED){
			close(fd);
			fd = -1;
			return;
		}else{
			printf("<===== memory mapping success!!!=====>\n");
		}
	}
}

void subtitle_app_phy_to_vir_memory(unsigned int addr, int size)
{
	unsigned int *vir_subtitle_buf;

	if(addr == NULL)
		return;

	if(fd != -1){
		vir_subtitle_buf = (unsigned int *)(subtitle_buf_addr + (addr - pmem_info.base));
		printf("<===== size[%d], buf_phy[%p] vir[%p]=====>\n", size, subtitle_buf_addr, vir_subtitle_buf);
		subtitle_app_dump_file_open();
		subtitle_app_dump_file_write((int*)vir_subtitle_buf, size);
	}
}
#endif /* DUMP_INPUT_TO_FILE */

long long tcc_dxb_isdbtfseg_process_getsystemtime(void)
{
	long long systime;
	struct timeval tv;
	gettimeofday (&tv, NULL);
	systime = (long long) tv.tv_sec * 1000 + tv.tv_usec / 1000;

	return systime;
}

/******************************************************************************
* globals
******************************************************************************/
TCC_DXB_ISDBTFSEG_PROCESS_Instance_T G_t_TCC_DXB_ISDBTFSEG_PROCESS_Instance;

/******************************************************************************
* locals
******************************************************************************/
//using namespace linux;
static ISDBTPlayer *pPlayer = NULL;

/******************************************************************************
* extern Functions
******************************************************************************/

/******************************************************************************
* local Functions
******************************************************************************/
int TCC_DXB_ISDBTFSEG_PROCESS_Notifier(int msg, int ext1, int ext2, void *obj);

void TCC_DXB_ISDBTFSEG_PROCESS_lock_init(void)
{
	pthread_mutex_init(&G_t_TCC_DXB_ISDBTFSEG_PROCESS_Instance.lockProcess, NULL);
}

void TCC_DXB_ISDBTFSEG_PROCESS_lock_deinit(void)
{
	pthread_mutex_destroy(&G_t_TCC_DXB_ISDBTFSEG_PROCESS_Instance.lockProcess);
}

void TCC_DXB_ISDBTFSEG_PROCESS_lock_on(void)
{
	pthread_mutex_lock(&G_t_TCC_DXB_ISDBTFSEG_PROCESS_Instance.lockProcess);
}

void TCC_DXB_ISDBTFSEG_PROCESS_lock_off(void)
{
	pthread_mutex_unlock(&G_t_TCC_DXB_ISDBTFSEG_PROCESS_Instance.lockProcess);
}

static void TCC_DXB_ISDBTFSEG_PROCESS_SetProcState(TCC_DXB_ISDBTFSEG_PROCESS_ProcState_E eProcState)
{
	G_t_TCC_DXB_ISDBTFSEG_PROCESS_Instance.eProcState = eProcState;
}

static TCC_DXB_ISDBTFSEG_PROCESS_ProcState_E TCC_DXB_ISDBTFSEG_PROCESS_GetProcState(void)
{
	return G_t_TCC_DXB_ISDBTFSEG_PROCESS_Instance.eProcState;
}

static void TCC_DXB_ISDBTFSEG_PROCESS_SetState(TCC_DXB_ISDBTFSEG_PROCESS_State_E eState)
{
	G_t_TCC_DXB_ISDBTFSEG_PROCESS_Instance.eState = eState;
}

static TCC_DXB_ISDBTFSEG_PROCESS_State_E TCC_DXB_ISDBTFSEG_PROCESS_GetState(void)
{
	return G_t_TCC_DXB_ISDBTFSEG_PROCESS_Instance.eState;
}
/******************************************************************************
* Process Functions
******************************************************************************/

/******************************************************************************
* Command Functions
******************************************************************************/
void TCC_DXB_ISDBTFSEG_PROCESS_Exit(void)
{
	long long stop_start,stop_end,release_start,release_end;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	//TCCDxBProc_Stop();
	DEBUG_MSG("[%s:%d]\n", __func__, __LINE__);
	TCC_DXB_ISDBTFSEG_PROCESS_SetState(TCC_DXB_ISDBTFSEG_STATE_EXIT);

	stop_start = tcc_dxb_isdbtfseg_process_getsystemtime();

	pPlayer->stop();

	stop_end = release_start = tcc_dxb_isdbtfseg_process_getsystemtime();
	DEBUG_MSG("[%s:%d] stop(%d)\n", __func__, __LINE__, (int)(stop_end-stop_start));

	pPlayer->release();

	release_end = tcc_dxb_isdbtfseg_process_getsystemtime();

	delete pPlayer;
	pPlayer = NULL;

	DEBUG_MSG("[%s:%d] Elapsed(mS) Total(%d) Stop(%d) Release(%d)\n", __func__, __LINE__,
		(int)(release_end - stop_start),(int)(stop_end-stop_start),(int)(release_end-release_start));
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

void TCC_DXB_ISDBTFSEG_PROCESS_Search(void *Data)
{
	TCC_DXB_ISDBTFSEG_CMD_LIST_Scan_T *ptCmd = (TCC_DXB_ISDBTFSEG_CMD_LIST_Scan_T *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();
	DEBUG_MSG("[%s:%d] Scan Type[%d] CountryCode[%d] AreaCode[%d] ChannelNum[%d] Options[%d]\n"
		, __func__, __LINE__
		, ptCmd->iScanType, ptCmd->iCountryCode, ptCmd->iAreaCode,  ptCmd->iChannelNum, ptCmd->iOptions );

	pPlayer->search(ptCmd->iScanType, ptCmd->iCountryCode, ptCmd->iAreaCode,  ptCmd->iChannelNum, ptCmd->iOptions);

	ptCmd->iResult = 1;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

void TCC_DXB_ISDBTFSEG_PROCESS_SearchCancel(void)
{
	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();
	DEBUG_MSG("[%s:%d] \n", __func__, __LINE__);

	pPlayer->searchCancel();

	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}


void TCC_DXB_ISDBTFSEG_PROCESS_SetChannel(void *Data)
{
#if 0
/*
  * mainRowID	= _id of fullseg service
  * subRowID 	= _id of 1seg service
  * audioIndex	= index of audio ES. '-1' means a default audio ES
  * videoIndex	= index of video ES. '-1' means a default video ES
  * audioMode   = dual mono mode
  * raw_w, raw_h, vidw_w, view_h = size of Android View
  * ch_index	= designate a service which will be rendered.
  */
status_t ISDBTPlayer::setChannel(int mainRowID, int subRowID, int audioIndex, int videoIndex, int audioMode, int raw_w, int raw_h, int view_w, int view_h, int ch_index)
#endif
	TCC_DXB_ISDBTFSEG_CMD_LIST_Set_T *ptCmd = (TCC_DXB_ISDBTFSEG_CMD_LIST_Set_T *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	DEBUG_MSG("[%s:%d] mainRowID[%d] subRowID[%d] audioIndex[%d] videoIndex[%d] audioMode[%d] ch_index[%d]\n",
		__func__, __LINE__,
		ptCmd->mainRowID, ptCmd->subRowID, ptCmd->audioIndex, ptCmd->videoIndex, ptCmd->audioMode, ptCmd->ch_index );

	pPlayer->setChannel( ptCmd->mainRowID, ptCmd->subRowID, ptCmd->audioIndex, ptCmd->videoIndex, ptCmd->audioMode
						, 0/*raw_w*/, 0/*raw_h*/, 0/*view_w*/, 0/*view_h*/, ptCmd->ch_index);
	ptCmd->iResult = 1;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();

}

void TCC_DXB_ISDBTFSEG_PROCESS_SetChannel_withChNum(void *Data)
{
	TCC_DXB_ISDBTFSEG_CMD_LIST_Set_T_WITHCHNUM *ptCmd = (TCC_DXB_ISDBTFSEG_CMD_LIST_Set_T_WITHCHNUM *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	DEBUG_MSG("[%s:%d] channelNumber[%d] serviceID_Fseg[%d] serviceID_1seg[%d] audioMode[%d] ch_index[%d]\n",
		__func__, __LINE__,
		ptCmd->channelNumber, ptCmd->serviceID_Fseg, ptCmd->serviceID_1seg, ptCmd->audioMode, ptCmd->ch_index );

	pPlayer->setChannel_withChNum( ptCmd->channelNumber, ptCmd->serviceID_Fseg, ptCmd->serviceID_1seg, ptCmd->audioMode
						, 0/*raw_w*/, 0/*raw_h*/, 0/*view_w*/, 0/*view_h*/, ptCmd->ch_index);
	ptCmd->iResult = 1;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();

}

void TCC_DXB_ISDBTFSEG_PROCESS_Stop(void)
{
	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();
	DEBUG_MSG("[%s:%d]\n", __func__, __LINE__);

	pPlayer->playSubtitle(0);
	pPlayer->stop();

	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

void TCC_DXB_ISDBTFSEG_PROCESS_SetDualChannel(void *Data)
{
	TCC_DXB_ISDBTFSEG_CMD_LIST_SetDualChannel_T *ptCmd = (TCC_DXB_ISDBTFSEG_CMD_LIST_SetDualChannel_T *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	DEBUG_MSG("[%s:%d] index[%d]\n", __func__, __LINE__, ptCmd->iChannelIndex);

	pPlayer->setDualChannel(ptCmd->iChannelIndex);

	ptCmd->iResult = 1;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

void TCC_DXB_ISDBTFSEG_PROCESS_Cmd_Show(void)
{
	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();
//	process_show_channelDB();
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

void TCC_DXB_ISDBTFSEG_PROCESS_Prepare(void *Data)
{
	TCC_DXB_ISDBTFSEG_PREPARE_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_PREPARE_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("iSpecificInfo=%d\n\n", pCmd->iSpecificInfo);

	pCmd->iRet = pPlayer->prepare(pCmd->iSpecificInfo);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

void TCC_DXB_ISDBTFSEG_PROCESS_Release(void *Data)
{
	TCC_DXB_ISDBTFSEG_RELEASE_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_RELEASE_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("Player release\n\n");

	pCmd->iRet = pPlayer->release();

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

void TCC_DXB_ISDBTFSEG_PROCESS_SetEPG(void *Data)
{
	TCC_DXB_ISDBTFSEG_SET_EPG_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_SET_EPG_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("Player setEPGOnOff, iOption=%d\n\n",
		pCmd->iOption);

	pCmd->iRet = pPlayer->setEPGOnOff(pCmd->iOption);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

void TCC_DXB_ISDBTFSEG_PROCESS_SetAudio(void *Data)
{
	TCC_DXB_ISDBTFSEG_SET_AUDIO_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_SET_AUDIO_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("iIndex=%d\n\n",
		pCmd->iIndex);

	pCmd->iRet = pPlayer->setAudio(pCmd->iIndex);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

void TCC_DXB_ISDBTFSEG_PROCESS_SetVideo(void *Data)
{
	TCC_DXB_ISDBTFSEG_SET_VIDEO_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_SET_VIDEO_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("iIndex=%d\n\n",
		pCmd->iIndex);

	pCmd->iRet = pPlayer->setVideo(pCmd->iIndex);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

void TCC_DXB_ISDBTFSEG_PROCESS_SetAudioOnOff(void *Data)
{
	TCC_DXB_ISDBTFSEG_SET_AUDIO_ONOFF_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_SET_AUDIO_ONOFF_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("iOnOff=%d\n\n",
		pCmd->iOnOff);

	pCmd->iRet = pPlayer->setAudioOnOff(pCmd->iOnOff);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

void TCC_DXB_ISDBTFSEG_PROCESS_SetAudioMuteOnOff(void *Data)
{
	TCC_DXB_ISDBTFSEG_SET_AUDIO_MUTE_ONOFF_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_SET_AUDIO_MUTE_ONOFF_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("iOnOff=%d\n\n",
		pCmd->iOnOff);

	pCmd->iRet = pPlayer->setAudioMuteOnOff(pCmd->iOnOff);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

void TCC_DXB_ISDBTFSEG_PROCESS_SetVideoOnOff(void *Data)
{
	TCC_DXB_ISDBTFSEG_SET_VIDEO_ONOFF_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_SET_VIDEO_ONOFF_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("iOnOff=%d\n\n",
		pCmd->iOnOff);

	pCmd->iRet = pPlayer->setVideoOnOff(pCmd->iOnOff);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

void TCC_DXB_ISDBTFSEG_PROCESS_SetVideoOutput(void *Data)
{
	TCC_DXB_ISDBTFSEG_SET_VIDEO_OUTPUT_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_SET_VIDEO_OUTPUT_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("iOnOff=%d\n\n",	pCmd->iOnOff);

	pCmd->iRet = pPlayer->setVideoOutput(pCmd->iOnOff);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

void TCC_DXB_ISDBTFSEG_PROCESS_PlaySubtitle(void *Data)
{
	TCC_DXB_ISDBTFSEG_PLAY_SUBTITLE_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_PLAY_SUBTITLE_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("iOnOff=%d\n\n",
		pCmd->iOnOff);

	pCmd->iRet = pPlayer->playSubtitle(pCmd->iOnOff);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

void TCC_DXB_ISDBTFSEG_PROCESS_PlaySuperimpose(void *Data)
{
	TCC_DXB_ISDBTFSEG_PLAY_SUPERIMPOSE_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_PLAY_SUPERIMPOSE_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("iOnOff=%d\n\n",
		pCmd->iOnOff);

	pCmd->iRet = pPlayer->playSuperimpose(pCmd->iOnOff);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

void TCC_DXB_ISDBTFSEG_PROCESS_SetSubtitle(void *Data)
{
	TCC_DXB_ISDBTFSEG_SET_SUBTITLE_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_SET_SUBTITLE_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("iIndex=%d\n\n",
		pCmd->iIndex);

	pCmd->iRet = pPlayer->setSubtitle(pCmd->iIndex);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

void TCC_DXB_ISDBTFSEG_PROCESS_SetSuperimpose(void *Data)
{
	TCC_DXB_ISDBTFSEG_SET_SUPERIMPOSE_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_SET_SUPERIMPOSE_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("iIndex=%d\n\n",
		pCmd->iIndex);

	pCmd->iRet = pPlayer->setSuperImpose(pCmd->iIndex);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

void TCC_DXB_ISDBTFSEG_PROCESS_SetAudioDualMono(void *Data)
{
	TCC_DXB_ISDBTFSEG_SET_AUDIO_DUALMONO_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_SET_AUDIO_DUALMONO_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("iAudioSelect=%d\n\n",
		pCmd->iAudioSelect);

	pCmd->iRet = pPlayer->setAudioDualMono(pCmd->iAudioSelect);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

void TCC_DXB_ISDBTFSEG_PROCESS_SetPresetMode(void *Data)
{
	TCC_DXB_ISDBTFSEG_SET_PRESET_MODE_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_SET_PRESET_MODE_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("iPresetMode=%d\n\n",
		pCmd->iPresetMode);

	pCmd->iRet = pPlayer->setPresetMode(pCmd->iPresetMode);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

void TCC_DXB_ISDBTFSEG_PROCESS_SetListener(void *Data)
{
	TCC_DXB_ISDBTFSEG_SET_LISTENER_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_SET_LISTENER_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("Player SetListener\n\n");

	pCmd->iRet = pPlayer->setListener(pCmd->pfListener);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();

}

void TCC_DXB_ISDBTFSEG_PROCESS_GetSignalStrength(void *Data)
{
	TCC_DXB_ISDBTFSEG_GET_SIGNAL_STRENGTH_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_GET_SIGNAL_STRENGTH_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("Player GetSignalStrength\n\n");

	pCmd->iRet = pPlayer->getSignalStrength(pCmd->aiSQInfo);

	printf("sinal quality size  :%d\n1seg signal quality :%d\nFseg signal quality :%d\n",
			pCmd->aiSQInfo[0], pCmd->aiSQInfo[1], pCmd->aiSQInfo[2]);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();

}


void TCC_DXB_ISDBTFSEG_PROCESS_CustomRelayStationSearchRequest(void *Data)
{
	TCC_DXB_ISDBTFSEG_CUSTOM_RELAY_STATION_SEARCH_REQUEST_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_CUSTOM_RELAY_STATION_SEARCH_REQUEST_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("Player CustomRelayStationSearchRequest\n\n");

	pCmd->iRet = pPlayer->customRelayStationSearchRequest(pCmd->iSearchKind, pCmd->aiListChannel, pCmd->aiListTSID, pCmd->iRepeat);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();

}

void TCC_DXB_ISDBTFSEG_PROCESS_GetChannelInfo(void *Data)
{
	TCC_DXB_ISDBTFSEG_GET_CHANNEL_INFO_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_GET_CHANNEL_INFO_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("Player GetChannelInfo\n\n");

	pCmd->iRet = pPlayer->getChannelInfo(pCmd->iChannelNumber,
							pCmd->iServiceID,
							&pCmd->iRemoconID,
							&pCmd->iThreeDigitNumber,
							&pCmd->iServiceType,
							pCmd->ausChannelName,
							&pCmd->iChannelNameLen,
							&pCmd->iTotalAudioPID,
							&pCmd->iTotalVideoPID,
							&pCmd->iTotalSubtitleLang,
							&pCmd->iAudioMode,
							&pCmd->iVideoMode,
							&pCmd->iAudioLang1,
							&pCmd->iAudioLang2,
							&pCmd->iAudioComponentTag,
							&pCmd->iVideoComponentTag,
							&pCmd->iSubtitleComponentTag,
							&pCmd->iStartMJD,
							&pCmd->iStartHH,
							&pCmd->iStartMM,
							&pCmd->iStartSS,
							&pCmd->iDurationHH,
							&pCmd->iDurationMM,
							&pCmd->iDurationSS,
							pCmd->ausEvtName,
							&pCmd->iEvtNameLen,
							&pCmd->iLogoImageWidth,
							&pCmd->iLogoImageHeight,
							pCmd->ausLogoImage,
							pCmd->ausSimpleLogo,
							&pCmd->iSimpleLogoLength,
							&pCmd->iMVTVGroupType);

	printf("ChannelNumber     :%d\nServiceID         :%d\nRemoconID         :%d\nThreeDigitNumber  :%d\nServiceType       :%d\nChannelName       :%s\nChannelNameLen    :%d\nTotalAudioPID     :%d\nTotalVideoPID     :%d\nTotalSubtitleLang :%d\nAudioMode         :%d\nVideoMode         :%d\nAudioLang1        :%d\nAudioLang2        :%d\nAudioComponentTag :%d\nVideoComponentTag :%d\nSubTiComponentTag :%d\nStartMJD          :%d\nStartHH           :%d\nStartMM           :%d\nStartSS           :%d\nDurationHH        :%d\nDurationMM        :%d\nDurationSS        :%d\nEvtName           :%s\nEvtNameLen        :%d\nLogoImageWidth    :%d\nLogoImageHeight   :%d\nLogoImage         :0x%X\nSimpleLogo        :%s\nSimpleLogoLength  :%d\nMVTVGroupType     :0x%X\n",
			pCmd->iChannelNumber,
			pCmd->iServiceID,
			pCmd->iRemoconID,
			pCmd->iThreeDigitNumber,
			pCmd->iServiceType,
			pCmd->ausChannelName,
			pCmd->iChannelNameLen,
			pCmd->iTotalAudioPID,
			pCmd->iTotalVideoPID,
			pCmd->iTotalSubtitleLang,
			pCmd->iAudioMode,
			pCmd->iVideoMode,
			pCmd->iAudioLang1,
			pCmd->iAudioLang2,
			pCmd->iAudioComponentTag,
			pCmd->iVideoComponentTag,
			pCmd->iSubtitleComponentTag,
			pCmd->iStartMJD,
			pCmd->iStartHH,
			pCmd->iStartMM,
			pCmd->iStartSS,
			pCmd->iDurationHH,
			pCmd->iDurationMM,
			pCmd->iDurationSS,
			pCmd->ausEvtName,
			pCmd->iEvtNameLen,
			pCmd->iLogoImageWidth,
			pCmd->iLogoImageHeight,
			pCmd->ausLogoImage,
			pCmd->ausSimpleLogo,
			pCmd->iSimpleLogoLength,
			pCmd->iMVTVGroupType);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();

}


void TCC_DXB_ISDBTFSEG_PROCESS_ReqTRMPInfo(void *Data)
{
	TCC_DXB_ISDBTFSEG_REQ_TRMP_INFO_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_REQ_TRMP_INFO_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("Player ReqTRMPInfo\n\n");

	pCmd->iRet = pPlayer->reqTRMPInfo((unsigned char **)&pCmd->aucInfo, &pCmd->iInfoSize);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();

}

void TCC_DXB_ISDBTFSEG_PROCESS_GetSubtitleLangInfo(void *Data)
{
	TCC_DXB_ISDBTFSEG_GET_SUBTITLE_LANG_INFO_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_GET_SUBTITLE_LANG_INFO_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("Player GetSubtitleLangInfo\n\n");

	pCmd->iRet = pPlayer->getSubtitleLangInfo(	&pCmd->iSubtitleLangNum,
												&pCmd->iSuperimposeLangNum,
												&pCmd->uiSubtitleLang1,
												&pCmd->uiSubtitleLang2,
												&pCmd->uiSuperimposeLang1,
												&pCmd->uiSuperimposeLang2);
	printf("SubtitleLangNum    :%d\nSuperimposeLangNum :%d\nSubtitleLang1      :%u\nSubtitleLang2      :%u\nSuperimposeLang1   :%u\nSuperimposeLang2   :%u\n",
			pCmd->iSubtitleLangNum,
			pCmd->iSuperimposeLangNum,
			pCmd->uiSubtitleLang1,
			pCmd->uiSubtitleLang2,
			pCmd->uiSuperimposeLang1,
			pCmd->uiSuperimposeLang2);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();

}

void TCC_DXB_ISDBTFSEG_PROCESS_SetCustomTuner(void *Data)
{
	TCC_DXB_ISDBTFSEG_SET_CUSTOM_TUNER_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_SET_CUSTOM_TUNER_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("Player SetCustomTuner\n\n");

	pCmd->iRet = pPlayer->setCustom_Tuner(pCmd->iSize, pCmd->aiArg);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();

}


void TCC_DXB_ISDBTFSEG_PROCESS_GetCustomTuner(void *Data)
{
	TCC_DXB_ISDBTFSEG_GET_CUSTOM_TUNER_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_GET_CUSTOM_TUNER_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("Player GetCustomTuner\n\n");

	pCmd->iRet = pPlayer->getCustom_Tuner(&pCmd->iSize, pCmd->aiArg);

	printf("size :%d\n", pCmd->iSize);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();

}


void TCC_DXB_ISDBTFSEG_PROCESS_GetDigitalCopyControlDescriptor(void *Data)
{
	TCC_DXB_ISDBTFSEG_GET_DCCD_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_GET_DCCD_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("Player GetDigitalCopyControlDescriptor\n\n");

	/* Important !!!!!
		1. Do free the memory of the DCC_Descriptor & the DCC_Descriptor->sub_info.
	*/
	DIGITAL_COPY_CONTROL_DESCR_t* pDCCDInfo = 0;
	pCmd->iRet = pPlayer->getDigitalCopyControlDescriptor(pCmd->usServiceID, (DigitalCopyControlDescriptor**)&pDCCDInfo);
	if(pDCCDInfo)
	{
		DIGITAL_COPY_CONTROL_DESCR_t* DCC_Descriptor = pDCCDInfo;
		DIGITAL_COPY_CONTROL_DESCR_t* DCC_Descriptor_ToFree = pDCCDInfo;

		printf("========== Get / DIGITAL_COPY_CONTROL ==========\n");
		printf("ServiceID 0x%x\n", pCmd->usServiceID);
		printf("digital_recording_control_data 0x%x, maximum_bitrate_flag 0x%x\n",
			DCC_Descriptor->digital_recording_control_data, DCC_Descriptor->maximum_bitrate_flag);
		printf("component_control_flag 0x%x, copy_control_type 0x%x\n",
			DCC_Descriptor->component_control_flag, DCC_Descriptor->copy_control_type);
		printf("APS_control_data 0x%x, maximum_bitrate 0x%x\n",
			DCC_Descriptor->APS_control_data, DCC_Descriptor->maximum_bitrate);

		if(DCC_Descriptor->sub_info)
		{
			SUB_DIGITAL_COPY_CONTROL_DESCR_t* sub_DCC_Descriptor = DCC_Descriptor->sub_info;
			while(sub_DCC_Descriptor)
			{
				printf("\t----- SUB / DIGITAL_COPY_CONTROL -----\n");

				printf("\tcomponent_tag 0x%x, digital_recording_control_data 0x%x\n",
					sub_DCC_Descriptor->component_tag, sub_DCC_Descriptor->digital_recording_control_data);
				printf("\tmaximum_bitrate_flag 0x%x, copy_control_type 0x%x\n",
					sub_DCC_Descriptor->maximum_bitrate_flag, sub_DCC_Descriptor->copy_control_type);
				printf("\tAPS_control_data 0x%x, maximum_bitrate 0x%x\n",
					sub_DCC_Descriptor->APS_control_data, sub_DCC_Descriptor->maximum_bitrate);

				sub_DCC_Descriptor = sub_DCC_Descriptor->pNext;
			}
		}
		printf("================================================\n");

		//////////////////////////////////////////////////////////
		// Free DCC Info received
		if(DCC_Descriptor_ToFree->sub_info)
		{
			SUB_DIGITAL_COPY_CONTROL_DESCR_t* sub_info = DCC_Descriptor_ToFree->sub_info;
			while(sub_info)
			{
				SUB_DIGITAL_COPY_CONTROL_DESCR_t* temp_sub_info = sub_info->pNext;
				free(sub_info);
				sub_info = temp_sub_info;
			}
		}
		free(DCC_Descriptor_ToFree);
		// Free DCC Info received
		//////////////////////////////////////////////////////////
	}

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();

}


void TCC_DXB_ISDBTFSEG_PROCESS_GetContentAvailabilityDescriptor(void *Data)
{
	TCC_DXB_ISDBTFSEG_GET_CAD_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_GET_CAD_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("Player GetContentAvailabilityDescriptor\n\n");

	pCmd->iRet = pPlayer->getContentAvailabilityDescriptor(pCmd->usServiceID,
											&pCmd->ucCopy_restriction_mode,
											&pCmd->ucImage_constraint_token,
											&pCmd->ucRetention_mode,
											&pCmd->ucRetention_state,
											&pCmd->ucEncryption_mode);

	printf("ServiceID              :0x%X\nCopy_restriction_mode  :0x%x\nImage_constraint_token :0x%X\nRetention_mode         :0x%X\nRetention_state        :0x%X\nEncryption_mode        :0x%X\n",
			pCmd->usServiceID,
			pCmd->ucCopy_restriction_mode,
			pCmd->ucImage_constraint_token,
			pCmd->ucRetention_mode,
			pCmd->ucRetention_state,
			pCmd->ucEncryption_mode);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();

}

/******************************************************************************
*	FUNCTIONS			:
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS 			:
*	ERRNO				:
******************************************************************************/
void TCC_DXB_ISDBTFSEG_PROCESS_SetNotifyDetectSection(void *Data)
{
	TCC_DXB_ISDBTFSEG_SET_NOTIFY_DETECT_SECTION_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_SET_NOTIFY_DETECT_SECTION_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("iSectionIDs=%d\n\n",
		pCmd->iSectionIDs);

	pCmd->iRet = pPlayer->setNotifyDetectSection(pCmd->iSectionIDs);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();

}

/******************************************************************************
*	FUNCTIONS			:
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS 			:
*	ERRNO				:
******************************************************************************/
void TCC_DXB_ISDBTFSEG_PROCESS_SearchAndSetChannel(void *Data)
{
	TCC_DXB_ISDBTFSEG_SEARCH_AND_SET_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_SEARCH_AND_SET_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("iCountryCode=%d iChannelNum=%d iTsid=%d iOptions=%d iAudioIndex=%d iVideoIndex=%d iAudioMode=%d iRaw_w=%d iRaw_h=%d iView_w=%d iView_h=%d iCh_index=%d\n\n",
		pCmd->iCountryCode,
		pCmd->iChannelNum,
		pCmd->iTsid,
		pCmd->iOptions,
		pCmd->iAudioIndex,
		pCmd->iVideoIndex,
		pCmd->iAudioMode,
		pCmd->iRaw_w,
		pCmd->iRaw_h,
		pCmd->iView_w,
		pCmd->iView_h,
		pCmd->iCh_index);

	pCmd->iRet = pPlayer->searchAndSetChannel(pCmd->iCountryCode, pCmd->iChannelNum, pCmd->iTsid, pCmd->iOptions, pCmd->iAudioIndex, pCmd->iVideoIndex, pCmd->iAudioMode, pCmd->iRaw_w, pCmd->iRaw_h, pCmd->iView_w, pCmd->iView_h, pCmd->iCh_index);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

/******************************************************************************
*	FUNCTIONS			:
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS 			:
*	ERRNO				:
******************************************************************************/
void TCC_DXB_ISDBTFSEG_PROCESS_GetVideoState(void *Data)
{
	TCC_DXB_ISDBTFSEG_GET_VIDEO_STATE_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_GET_VIDEO_STATE_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("Player GetVideoState\n\n");
	pCmd->iRet = pPlayer->getVideoState();
	printf("state = %d\n", pCmd->iRet);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

/******************************************************************************
*	FUNCTIONS			:
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS 			:
*	ERRNO				:
******************************************************************************/
void TCC_DXB_ISDBTFSEG_PROCESS_GetSearchState(void *Data)
{
	TCC_DXB_ISDBTFSEG_GET_SEARCH_STATE_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_GET_SEARCH_STATE_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("Player GetSearchState\n\n");
	pCmd->iRet = pPlayer->getSearchState();
	printf("state = %d\n", pCmd->iRet);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

/******************************************************************************
*	FUNCTIONS			:
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS 			:
*	ERRNO				:
******************************************************************************/
void TCC_DXB_ISDBTFSEG_PROCESS_SetDataServiceStart(void *Data)
{
	TCC_DXB_ISDBTFSEG_SET_DATASERVICE_START_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_SET_DATASERVICE_START_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("Player setDataServiceStart\n\n");

	pCmd->iRet = pPlayer->setDataServiceStart();

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

/******************************************************************************
*	FUNCTIONS			:
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS 			:
*	ERRNO				:
******************************************************************************/
void TCC_DXB_ISDBTFSEG_PROCESS_CustomFilterStart(void *Data)
{
	TCC_DXB_ISDBTFSEG_CUSTOM_FILTER_START_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_CUSTOM_FILTER_START_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("iPID=0x%X iTableID=0x%X\n\n", pCmd->iPID, pCmd->iTableID);

	pCmd->iRet = pPlayer->customFilterStart(pCmd->iPID, pCmd->iTableID);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

/******************************************************************************
*	FUNCTIONS			:
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS 			:
*	ERRNO				:
******************************************************************************/
void TCC_DXB_ISDBTFSEG_PROCESS_CustomFilterStop(void *Data)
{
	TCC_DXB_ISDBTFSEG_CUSTOM_FILTER_STOP_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_CUSTOM_FILTER_STOP_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("iPID=0x%X iTableID=0x%X\n\n", pCmd->iPID, pCmd->iTableID);

	pCmd->iRet = pPlayer->customFilterStop(pCmd->iPID, pCmd->iTableID);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

/******************************************************************************
*	FUNCTIONS			:
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS 			:
*	ERRNO				:
******************************************************************************/
void TCC_DXB_ISDBTFSEG_PROCESS_EWS_start(void *Data)
{
	TCC_DXB_ISDBTFSEG_EWS_START_CMD_t *ptCmd = (TCC_DXB_ISDBTFSEG_EWS_START_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	DEBUG_MSG("[%s:%d] mainRowID[%d] subRowID[%d] audioIndex[%d] videoIndex[%d] audioMode[%d] ch_index[%d]\n",
		__func__, __LINE__,
		ptCmd->mainRowID, ptCmd->subRowID, ptCmd->audioIndex, ptCmd->videoIndex, ptCmd->audioMode, ptCmd->ch_index );

	pPlayer->EWS_start( ptCmd->mainRowID, ptCmd->subRowID, ptCmd->audioIndex, ptCmd->videoIndex, ptCmd->audioMode
						, 0/*raw_w*/, 0/*raw_h*/, 0/*view_w*/, 0/*view_h*/, ptCmd->ch_index);
	ptCmd->iResult = 1;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

/******************************************************************************
*	FUNCTIONS			:
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS 			:
*	ERRNO				:
******************************************************************************/
void TCC_DXB_ISDBTFSEG_PROCESS_EWS_start_withChNum(void *Data)
{
	TCC_DXB_ISDBTFSEG_EWS_START_CMD_t_WITHCHNUM *ptCmd = (TCC_DXB_ISDBTFSEG_EWS_START_CMD_t_WITHCHNUM *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	DEBUG_MSG("[%s:%d] channelNumber[%d] serviceID_Fseg[%d] serviceID_1seg[%d] audioMode[%d] ch_index[%d]\n",
		__func__, __LINE__,
		ptCmd->channelNumber, ptCmd->serviceID_Fseg, ptCmd->serviceID_1seg, ptCmd->audioMode, ptCmd->ch_index );

	pPlayer->EWS_start_withChNum( ptCmd->channelNumber, ptCmd->serviceID_Fseg, ptCmd->serviceID_1seg, ptCmd->audioMode
						, 0/*raw_w*/, 0/*raw_h*/, 0/*view_w*/, 0/*view_h*/, ptCmd->ch_index);
	ptCmd->iResult = 1;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

/******************************************************************************
*	FUNCTIONS			:
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS 			:
*	ERRNO				:
******************************************************************************/
void TCC_DXB_ISDBTFSEG_PROCESS_EWS_clear(void *Data)
{
	TCC_DXB_ISDBTFSEG_EWS_CLEAR_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_EWS_CLEAR_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("Player EWS_clear\n\n");

	pCmd->iRet = pPlayer->EWS_clear();

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

/******************************************************************************
*	FUNCTIONS			:
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS 			:
*	ERRNO				:
******************************************************************************/
void TCC_DXB_ISDBTFSEG_PROCESS_Set_Seamless_Switch_Compensation(void *Data)
{
	TCC_DXB_ISDBTFSEG_Seamless_Switch_Compensation_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_Seamless_Switch_Compensation_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	printf("Player Set_Seamless_Switch_Compensation\n\n");

	pCmd->iRet = pPlayer->Set_Seamless_Switch_Compensation(pCmd->iOnOff, pCmd->iInterval, pCmd->iStrength, pCmd->iNtimes, pCmd->iRange, pCmd->iGapadjust, pCmd->iGapadjust2, pCmd->iMultiplier);

	pCmd->iResult = TRUE;
	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

/******************************************************************************
*	FUNCTIONS			:
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS 			:
*	ERRNO				:
******************************************************************************/
void TCC_DXB_ISDBTFSEG_PROCESS_DS_GetSTC(void *Data)
{
	TCC_DXB_ISDBTFSEG_DS_GET_STC_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_DS_GET_STC_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	pCmd->iRet = pPlayer->getSTC(pCmd->hi_data, pCmd->lo_data);

	printf("[Player] DS Get_STC Ret(%d) (HI)%X (LO)%X \n", pCmd->iRet, *(pCmd->hi_data), *(pCmd->lo_data));

	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

/******************************************************************************
*	FUNCTIONS			:
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS 			:
*	ERRNO				:
******************************************************************************/
void TCC_DXB_ISDBTFSEG_PROCESS_DS_GetServiceTime(void *Data)
{
	TCC_DXB_ISDBTFSEG_DS_GET_SERVICE_TIME_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_DS_GET_SERVICE_TIME_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	pCmd->iRet = pPlayer->getServiceTime(pCmd->year, pCmd->month, pCmd->day, pCmd->hour, pCmd->min, pCmd->sec);

	printf("[Player] DS Get_ServiceTime Ret(%d) %4d/%2d/%2d %02d:%02d:%02d \n", pCmd->iRet,
		*(pCmd->year), *(pCmd->month), *(pCmd->day), *(pCmd->hour), *(pCmd->min), *(pCmd->sec));

	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

/******************************************************************************
*	FUNCTIONS			:
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS 			:
*	ERRNO				:
******************************************************************************/
void TCC_DXB_ISDBTFSEG_PROCESS_DS_GetContenID(void *Data)
{
	TCC_DXB_ISDBTFSEG_DS_GET_CONTENTID_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_DS_GET_CONTENTID_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	pCmd->iRet = pPlayer->getContentID(pCmd->contentID, pCmd->associated_contents_flag);

	printf("[Player] DS Get_ContentID Ret(%d) contendID(%d) associated_contents_flag(%d) \n",
		pCmd->iRet, *(pCmd->contentID), *(pCmd->associated_contents_flag));

	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

/******************************************************************************
*	FUNCTIONS			:
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS 			:
*	ERRNO				:
******************************************************************************/
void TCC_DXB_ISDBTFSEG_PROCESS_DS_CheckExistComponentTagInPMT(void *Data)
{
	TCC_DXB_ISDBTFSEG_DS_CHECK_EXIST_COMPONENTTAG_IN_PMT_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_DS_CHECK_EXIST_COMPONENTTAG_IN_PMT_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	pCmd->iRet = pPlayer->checkExistComponentTagInPMT(pCmd->target_component_tag);

	printf("[Player] DS Check_ExistComponentTagInPMT Tag(%d) Ret(%d) \n", pCmd->target_component_tag, pCmd->iRet);

	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

/******************************************************************************
*	FUNCTIONS			:
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS 			:
*	ERRNO				:
******************************************************************************/
void TCC_DXB_ISDBTFSEG_PROCESS_DS_SetVideoByComponentTag(void *Data)
{
	TCC_DXB_ISDBTFSEG_DS_SET_VIDEO_BY_COMPONENTTAG_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_DS_SET_VIDEO_BY_COMPONENTTAG_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	pCmd->iRet = pPlayer->setVideoByComponentTag(pCmd->component_tag);

	printf("[Player] DS Set_VideoByComponentTag(%d) Ret(%d) \n", pCmd->component_tag, pCmd->iRet);

	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}

/******************************************************************************
*	FUNCTIONS			:
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS 			:
*	ERRNO				:
******************************************************************************/
void TCC_DXB_ISDBTFSEG_PROCESS_GetSeamlessValue(void *Data)
{
	TCC_DXB_ISDBTFSEG_GET_SEAMLESSVALUE_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_GET_SEAMLESSVALUE_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	pCmd->iRet = pPlayer->getSeamlessValue(pCmd->state, pCmd->cval, pCmd->pval);

	printf("[Player] GetSeamlessValue Ret(%d) state(%d) cval(%d) pval(%d)\n",
		pCmd->iRet, *(pCmd->state), *(pCmd->cval), *(pCmd->pval));

	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}
/******************************************************************************
*	FUNCTIONS			:
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS 			:
*	ERRNO				:
******************************************************************************/
void TCC_DXB_ISDBTFSEG_PROCESS_DS_SetAudioByComponentTag(void *Data)
{
	TCC_DXB_ISDBTFSEG_DS_SET_AUDIO_BY_COMPONENTTAG_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_DS_SET_AUDIO_BY_COMPONENTTAG_CMD_t *)Data;

	TCC_DXB_ISDBTFSEG_PROCESS_lock_on();

	pCmd->iRet = pPlayer->setAudioByComponentTag(pCmd->component_tag);

	printf("[Player] DS Set_AudioByComponentTag(%d) Ret(%d) \n", pCmd->component_tag, pCmd->iRet);

	TCC_DXB_ISDBTFSEG_PROCESS_lock_off();
}


int TCC_DXB_ISDBTFSEG_PROCESS_Notifier(int msg, int ext1, int ext2, void *obj)
{
	int objsize = 0;

	switch(msg)
	{
		case ISDBTPlayer::EVENT_NOP :
			printf("EVENT_NOP\n");
			break;

		case ISDBTPlayer::EVENT_PREPARED :
			printf("EVENT_PREPARED, specific_info=%08X\n", ext1);
			break;

		case ISDBTPlayer::EVENT_SEARCH_COMPLETE :
			printf("EVENT_SEARCH_COMPLETE, search_result=%d\n", ext1);
			break;

		case ISDBTPlayer::EVENT_CHANNEL_UPDATE :
			printf("EVENT_CHANNEL_UPDATE, request_ch_update=%d\n", ext1);
			break;

		case ISDBTPlayer::EVENT_SEARCH_PERCENT :
			printf("EVENT_SEARCH_PERCENT, %d%, channel=%d\n", ext1, ext2 );
			break;

		case ISDBTPlayer::EVENT_VIDEO_OUTPUT :
			printf("EVENT_VIDEO_OUTPUT, channel_index=%d\n", ext1);
			break;

#if defined(SUBTITLE_DUMP_INPUT_TO_FILE)
		case ISDBTPlayer::EVENT_SUBTITLE_UPDATE :
			printf("<====== subtitle update start!!!======>\n");
			SUBTITLE_DATA_t *subt;
			subt = (SUBTITLE_DATA_t *)obj;
			objsize = sizeof(SUBTITLE_DATA_t);
			printf("EVENT_SUBTITLE_UPDATE, objsize=%d, internal_sound_index[%d] size[%d] phy_addr[%p]\n", \
				objsize, subt->internal_sound_subtitle_index, subt->mixed_subtitle_size, subt->mixed_subtitle_phy_addr);
			if(subt->mixed_subtitle_size > 0){
				subtitle_app_memory_mapping_init(subt->mixed_subtitle_phy_addr, subt->mixed_subtitle_size);
				subtitle_app_phy_to_vir_memory(subt->mixed_subtitle_phy_addr, subt->mixed_subtitle_size);
			}else{
				printf("<====== subtitle no-image!!!======>\n");
			}
			printf("<====== subtitle update end!!!======>\n");

			break;
#else
		case ISDBTPlayer::EVENT_SUBTITLE_UPDATE :
			printf("<====== subtitle update (no dump)!!!======>\n");
			SUBTITLE_DATA_t *subt;
			subt = (SUBTITLE_DATA_t *)obj;
			objsize = sizeof(SUBTITLE_DATA_t);
//			printf("EVENT_SUBTITLE_UPDATE, objsize=%d, internal_sound_index[%d] size[%d] phy_addr[%p]\n", objsize, \
//				subt->internal_sound_subtitle_index, subt->mixed_subtitle_size, subt->mixed_subtitle_phy_addr);
			break;
#endif
		case ISDBTPlayer::EVENT_VIDEODEFINITION_UPDATE :
			objsize = sizeof(VIDEO_DEFINITION_DATA_t);
			printf("EVENT_VIDEODEFINITION_UPDATE, objsize=%d\n", objsize);
			break;

		case ISDBTPlayer::EVENT_DBINFO_UPDATE :
			/* check later : class has pointer member, so can't send ipc.
                               This is for playback recorded file.
			objsize = sizeof(DB_INFO_DATA_t);
			printf("EVENT_DBINFO_UPDATE, objsize=%d\n", objsize);

			*/
			break;

		case ISDBTPlayer::EVENT_EPG_UPDATE:
			printf("EVENT_EPG_UPDATE, ServiceID=%d, TableID=%d\n", ext1, ext2 );
			break;

		case ISDBTPlayer::EVENT_EMERGENCY_INFO :
			objsize = sizeof(EWS_DATA_t);
			printf("EVENT_EMERGENCY_INFO, start/end=%d, objsize=%d\n", ext1, objsize);
			break;

		case ISDBTPlayer::EVENT_AUTOSEARCH_UPDATE :
			objsize = sizeof(int)*2;
			printf("EVENT_AUTOSEARCH_UPDATE, status=%d, percent=%d, objsize=%d\n", ext1, ext2, objsize);
			break;

		case ISDBTPlayer::EVENT_AUTOSEARCH_DONE :
			objsize = sizeof(int)*(32+4);
			printf("EVENT_AUTOSEARCH_DONE, ArraySvcRowID=%d, objsize=%d\n", ext1, objsize);
			break;

		case ISDBTPlayer::EVENT_DESC_UPDATE :
			if( ext2 == 0xc1 )
			{
				/* Important !!!!!
					1. Do free the memory of the DCC_Descriptor & the DCC_Descriptor->sub_info.
				*/

				DIGITAL_COPY_CONTROL_DESCR_t* DCC_Descriptor = (DIGITAL_COPY_CONTROL_DESCR_t *)obj;
				DIGITAL_COPY_CONTROL_DESCR_t* DCC_Descriptor_ToFree = (DIGITAL_COPY_CONTROL_DESCR_t *)obj;

				printf("========== EVENT_DESC_UPDATE / DIGITAL_COPY_CONTROL ==========\n");
				printf("ServiceID 0x%x\n", ext1);
				printf("digital_recording_control_data 0x%x, maximum_bitrate_flag 0x%x\n",
					DCC_Descriptor->digital_recording_control_data, DCC_Descriptor->maximum_bitrate_flag);
				printf("component_control_flag 0x%x, copy_control_type 0x%x\n",
					DCC_Descriptor->component_control_flag, DCC_Descriptor->copy_control_type);
				printf("APS_control_data 0x%x, maximum_bitrate 0x%x\n",
					DCC_Descriptor->APS_control_data, DCC_Descriptor->maximum_bitrate);

				if(DCC_Descriptor->sub_info)
				{
					SUB_DIGITAL_COPY_CONTROL_DESCR_t* sub_DCC_Descriptor = DCC_Descriptor->sub_info;
					while(sub_DCC_Descriptor)
					{
						printf("\t----- SUB / DIGITAL_COPY_CONTROL -----\n");

						printf("\tcomponent_tag 0x%x, digital_recording_control_data 0x%x\n",
							sub_DCC_Descriptor->component_tag, sub_DCC_Descriptor->digital_recording_control_data);
						printf("\tmaximum_bitrate_flag 0x%x, copy_control_type 0x%x\n",
							sub_DCC_Descriptor->maximum_bitrate_flag, sub_DCC_Descriptor->copy_control_type);
						printf("\tAPS_control_data 0x%x, maximum_bitrate 0x%x\n",
							sub_DCC_Descriptor->APS_control_data, sub_DCC_Descriptor->maximum_bitrate);

						sub_DCC_Descriptor = sub_DCC_Descriptor->pNext;
					}
				}
				printf("==============================================================\n");

				//////////////////////////////////////////////////////////
				// Free DCC Info received
				if(DCC_Descriptor_ToFree->sub_info)
				{
					SUB_DIGITAL_COPY_CONTROL_DESCR_t* sub_info = DCC_Descriptor_ToFree->sub_info;
					while(sub_info)
					{
						SUB_DIGITAL_COPY_CONTROL_DESCR_t* temp_sub_info = sub_info->pNext;
						free(sub_info);
						sub_info = temp_sub_info;
					}
				}
				free(DCC_Descriptor_ToFree);
				// Free DCC Info received
				//////////////////////////////////////////////////////////
			}
			else if ( ext2 == 0xde )
			{
				objsize = sizeof(CONTENT_AVAILABILITY_DESCR_t);
				CONTENT_AVAILABILITY_DESCR_t *CA_Descriptor = (CONTENT_AVAILABILITY_DESCR_t *)obj;
				printf("========== EVENT_DESC_UPDATE / CONTENT_AVAILABILITY ==========\n");
				printf("ServiceID 0x%x\n", ext1);
				printf("copy_restriction_mode 0x%x, image_constraint_token 0x%x\n",
					CA_Descriptor->copy_restriction_mode, CA_Descriptor->image_constraint_token);
				printf("retention_mode 0x%x, retention_state 0x%x\n",
					CA_Descriptor->retention_mode, CA_Descriptor->retention_state);
				printf("encryption_mode 0x%x\n",
					CA_Descriptor->encryption_mode);
				printf("==============================================================\n");
			}
			break;

		case ISDBTPlayer::EVENT_EVENT_RELAY :
			objsize = sizeof(int)*4;
			printf("EVENT_EVENT_RELAY, objsize=%d\n", objsize);
			break;

		case ISDBTPlayer::EVENT_MVTV_UPDATE :
			objsize = sizeof(int)*5;
			printf("EVENT_MVTV_UPDATE, objsize=%d\n", objsize);
			break;

		case ISDBTPlayer::EVENT_SERVICEID_CHECK :
			printf("EVENT_SERVICEID_CHECK, valid_serviceID_flag=%d\n", ext1);
			break;

		case ISDBTPlayer::EVENT_TRMP_ERROR :
			printf("EVENT_TRMP_ERROR, TRMPErrorCode=%d\n", ext1);
			break;

		case ISDBTPlayer::EVENT_CUSTOMSEARCH_STATUS :
			objsize = sizeof(CUSTOM_SEARCH_INFO_DATA_t);
			printf("EVENT_CUSTOMSEARCH_STATUS, status=%d, objsize=%d\n", ext1, objsize);
			break;

		case ISDBTPlayer::EVENT_SPECIALSERVICE_UPDATE :
			printf("EVENT_SPECIALSERVICE_UPDATE, status=%d, RowID=%d\n", ext1, ext2);
			break;

		case ISDBTPlayer::EVENT_UPDATE_CUSTOM_TUNER :
			printf("EVENT_UPDATE_CUSTOM_TUNER\n");
			break;

		case ISDBTPlayer::EVENT_AV_INDEX_UPDATE :
			objsize = sizeof(AV_INDEX_DATA_t);
			printf("EVENT_AV_INDEX_UPDATE, objsize=%d\n", objsize);
			break;

		case ISDBTPlayer::EVENT_SECTION_UPDATE :
			printf("EVENT_SECTION_UPDATE, channelNumber=%d, sectionType=%d\n", ext1, ext2);
			break;

		case ISDBTPlayer::EVENT_SEARCH_AND_SETCHANNEL :
			objsize = sizeof(SET_CHANNEL_DATA_t);
			printf("EVENT_SEARCH_AND_SETCHANNEL, status=%d, objsize=%d\n", ext1, objsize);
			break;

		case ISDBTPlayer::EVENT_DETECT_SERVICE_NUM_CHANGE :
			printf("EVENT_DETECT_SERVICE_NUM_CHANGE\n");
			break;
		case ISDBTPlayer::EVENT_SECTION_DATA_UPDATE :
			printf("EVENT_SECTION_DATA_UPDATE service_id=0x%X, table_id=0x%X\n", ext1, ext2);
			break;

		case ISDBTPlayer::EVENT_CUSTOM_FILTER_DATA_UPDATE :
			printf("EVENT_CUSTOM_FILTER_DATA_UPDATE PID=0x%X, size=%d\n", ext1, ext2);
			break;

		default:
			printf("[PROCESS_Notifier] message received msg=%d, ext1=%d, ext2=%d, obj=%p\n", msg, ext1, ext2, obj);
			objsize = 0;
			obj = NULL;
			break;
	}
	return 0;
}

/******************************************************************************
* Core Functions
******************************************************************************/
int TCC_DXB_ISDBTFSEG_PROCESS_ProcMessage(void)
{
	TCC_MESSAGE_t *tMessage;

	if(TCC_DXB_ISDBTFSEG_CMD_GetProcessMessageCount() > 0)
	{
		if((tMessage = TCC_DXB_ISDBTFSEG_CMD_GetProcessMessage()) != NULL )
		{
			switch(tMessage->Operand)
			{
				case TCC_OPERAND_EXIT:
					TCC_DXB_ISDBTFSEG_PROCESS_Exit();
					break;

				case TCC_OPERAND_SCAN:
					TCC_DXB_ISDBTFSEG_PROCESS_Search(tMessage->Data);
					break;

				case TCC_OPERAND_SCANSTOP:
					TCC_DXB_ISDBTFSEG_PROCESS_SearchCancel();
					break;

				case TCC_OPERAND_SET:
					TCC_DXB_ISDBTFSEG_PROCESS_SetChannel(tMessage->Data);
					break;

				case TCC_OPERAND_SET_WITHCHNUM:
					TCC_DXB_ISDBTFSEG_PROCESS_SetChannel_withChNum(tMessage->Data);
					break;

				case TCC_OPERAND_STOP:
					TCC_DXB_ISDBTFSEG_PROCESS_Stop();
					break;

				case TCC_OPERAND_SHOW_CHANNEL_LIST:
					TCC_DXB_ISDBTFSEG_PROCESS_Cmd_Show();
					break;
				case TCC_OPERAND_SET_DUAL_CHANNEL:
					TCC_DXB_ISDBTFSEG_PROCESS_SetDualChannel(tMessage->Data);
					break;

				case TCC_OPERAND_PREPARE:
					TCC_DXB_ISDBTFSEG_PROCESS_Prepare(tMessage->Data);
					break;

				case TCC_OPERAND_RELEASE:
					TCC_DXB_ISDBTFSEG_PROCESS_Release(tMessage->Data);
					break;

				case TCC_OPERAND_SET_EPG:
					TCC_DXB_ISDBTFSEG_PROCESS_SetEPG(tMessage->Data);
					break;

				case TCC_OPERAND_SET_AUDIO:
					TCC_DXB_ISDBTFSEG_PROCESS_SetAudio(tMessage->Data);
					break;

				case TCC_OPERAND_SET_VIDEO:
					TCC_DXB_ISDBTFSEG_PROCESS_SetVideo(tMessage->Data);
					break;

				case TCC_OPERAND_SET_AUDIO_ONOFF:
					TCC_DXB_ISDBTFSEG_PROCESS_SetAudioOnOff(tMessage->Data);
					break;

				case TCC_OPERAND_SET_AUDIO_MUTE_ONOFF:
					TCC_DXB_ISDBTFSEG_PROCESS_SetAudioMuteOnOff(tMessage->Data);
					break;

				case TCC_OPERAND_SET_VIDEO_ONOFF:
					TCC_DXB_ISDBTFSEG_PROCESS_SetVideoOnOff(tMessage->Data);
					break;

				case TCC_OPERAND_SET_VIDEO_VIS_ONOFF:
					TCC_DXB_ISDBTFSEG_PROCESS_SetVideoOutput(tMessage->Data);
					break;

				case TCC_OPERAND_PLAY_SUBTITLE:
					TCC_DXB_ISDBTFSEG_PROCESS_PlaySubtitle(tMessage->Data);
					break;

				case TCC_OPERAND_PLAY_SUPERIMPOSE:
					TCC_DXB_ISDBTFSEG_PROCESS_PlaySuperimpose(tMessage->Data);
					break;

				case TCC_OPERAND_SET_SUBTITLE:
					TCC_DXB_ISDBTFSEG_PROCESS_SetSubtitle(tMessage->Data);
					break;

				case TCC_OPERAND_SET_SUPERIMPOSE:
					TCC_DXB_ISDBTFSEG_PROCESS_SetSuperimpose(tMessage->Data);
					break;

				case TCC_OPERAND_SET_AUDIO_DUALMONO:
					TCC_DXB_ISDBTFSEG_PROCESS_SetAudioDualMono(tMessage->Data);
					break;

				case TCC_OPERAND_SET_PRESET_MODE:
					TCC_DXB_ISDBTFSEG_PROCESS_SetPresetMode(tMessage->Data);
					break;

				case TCC_OPERAND_SET_LISTENER:
					TCC_DXB_ISDBTFSEG_PROCESS_SetListener(tMessage->Data);
					break;

				case TCC_OPERAND_GET_SIGNAL_STRENGTH:
					TCC_DXB_ISDBTFSEG_PROCESS_GetSignalStrength(tMessage->Data);
					break;

				case TCC_OPERAND_CUSTOM_RELAY_STATION_SEARCH_REQUEST:
					TCC_DXB_ISDBTFSEG_PROCESS_CustomRelayStationSearchRequest(tMessage->Data);
					break;

				case TCC_OPERAND_GET_CHANNEL_INFO:
					TCC_DXB_ISDBTFSEG_PROCESS_GetChannelInfo(tMessage->Data);
					break;

				case TCC_OPERAND_REQ_TRMP_INFO:
					TCC_DXB_ISDBTFSEG_PROCESS_ReqTRMPInfo(tMessage->Data);
					break;

				case TCC_OPERAND_GET_SUBTITLE_LANG_INFO:
					TCC_DXB_ISDBTFSEG_PROCESS_GetSubtitleLangInfo(tMessage->Data);
					break;

				case TCC_OPERAND_SET_CUSTOM_TUNER:
					TCC_DXB_ISDBTFSEG_PROCESS_SetCustomTuner(tMessage->Data);
					break;

				case TCC_OPERAND_GET_CUSTOM_TUNER:
					TCC_DXB_ISDBTFSEG_PROCESS_GetCustomTuner(tMessage->Data);
					break;

				case TCC_OPERAND_GET_DCCD:
					TCC_DXB_ISDBTFSEG_PROCESS_GetDigitalCopyControlDescriptor(tMessage->Data);
					break;

				case TCC_OPERAND_GET_CAD:
					TCC_DXB_ISDBTFSEG_PROCESS_GetContentAvailabilityDescriptor(tMessage->Data);
					break;

				case TCC_OPERAND_SET_NOTIFY_DETECT_SECTION:
					TCC_DXB_ISDBTFSEG_PROCESS_SetNotifyDetectSection(tMessage->Data);
					break;

				case TCC_OPERAND_SEARCH_AND_SET:
					TCC_DXB_ISDBTFSEG_PROCESS_SearchAndSetChannel(tMessage->Data);
					break;

				case TCC_OPERAND_GET_VIDEO_STATE:
					TCC_DXB_ISDBTFSEG_PROCESS_GetVideoState(tMessage->Data);
					break;

				case TCC_OPERAND_GET_SEARCH_STATE:
					TCC_DXB_ISDBTFSEG_PROCESS_GetSearchState(tMessage->Data);
					break;

				case TCC_OPERAND_SET_DATASERVICE_START:
					TCC_DXB_ISDBTFSEG_PROCESS_SetDataServiceStart(tMessage->Data);
					break;

				case TCC_OPERAND_CUSTOM_FILTER_START:
					TCC_DXB_ISDBTFSEG_PROCESS_CustomFilterStart(tMessage->Data);
					break;

				case TCC_OPERAND_CUSTOM_FILTER_STOP:
					TCC_DXB_ISDBTFSEG_PROCESS_CustomFilterStop(tMessage->Data);
					break;

				case TCC_OPERAND_EWS_START:
					TCC_DXB_ISDBTFSEG_PROCESS_EWS_start(tMessage->Data);
					break;

				case TCC_OPERAND_EWS_START_WITHCHNUM:
					TCC_DXB_ISDBTFSEG_PROCESS_EWS_start_withChNum(tMessage->Data);
					break;

				case TCC_OPERAND_EWS_CLEAR:
					TCC_DXB_ISDBTFSEG_PROCESS_EWS_clear(tMessage->Data);
					break;
				case TCC_OPERAND_DS_GET_STC:
					TCC_DXB_ISDBTFSEG_PROCESS_DS_GetSTC(tMessage->Data);
					break;
				case TCC_OPERAND_DS_GET_SERVICETIME:
					TCC_DXB_ISDBTFSEG_PROCESS_DS_GetServiceTime(tMessage->Data);
					break;
				case TCC_OPERAND_DS_GET_CONTENTID:
					TCC_DXB_ISDBTFSEG_PROCESS_DS_GetContenID(tMessage->Data);
					break;
				case TCC_OPERAND_DS_CHECK_EXISTCOMPONENTTAGINPMT:
					TCC_DXB_ISDBTFSEG_PROCESS_DS_CheckExistComponentTagInPMT(tMessage->Data);
					break;
				case TCC_OPERAND_DS_SET_VIDEOBYCOMPONENTTAG:
					TCC_DXB_ISDBTFSEG_PROCESS_DS_SetVideoByComponentTag(tMessage->Data);
					break;
				case TCC_OPERAND_DS_SET_AUDIOBYCOMPONENTTAG:
					TCC_DXB_ISDBTFSEG_PROCESS_DS_SetAudioByComponentTag(tMessage->Data);
					break;
				case TCC_OPERAND_SET_SEAMLESS_SWITCH_COMPENSATION:
					TCC_DXB_ISDBTFSEG_PROCESS_Set_Seamless_Switch_Compensation(tMessage->Data);
					break;
				case TCC_OPERAND_GET_SEAMLESSVALUE:
					TCC_DXB_ISDBTFSEG_PROCESS_GetSeamlessValue(tMessage->Data);
					break;
			 	default:
					break;
			}

			if(tMessage->DonFunction != NULL)
			{
				if((tMessage->Free_Flag == 0) && (tMessage->Data_Size > 0))
					tMessage->DonFunction(tMessage->Data);
				else
					tMessage->DonFunction(NULL);
			}

			TCC_Delete_Message(tMessage);
		}
	}
	else
	{
		usleep(TCC_DXB_ISDBTFSEG_PROCESS_SLEEP_TIME);
	}

	return TCC_DXB_ISDBTFSEG_PROCESS_GetState();
}

int TCC_DXB_ISDBTFSEG_PROCESS_Init(unsigned int uiCountryCode)
{

	TCC_DXB_ISDBTFSEG_PROCESS_lock_init();

	#ifdef DEBUG_TCC_MALLOC_HOOKING_ENABLE
	tcc_malloc_hookinfo_init();
	#endif

	//=======================================================
	//	Init CMD & Process
	//=======================================================
	TCC_DXB_ISDBTFSEG_CMD_InitCmd();
	TCC_DXB_ISDBTFSEG_CMD_SetOpcode();
	TCC_DXB_ISDBTFSEG_PROCESS_SetProcState(TCC_DXB_ISDBTFSEG_PROC_STATE_IDLE);
	TCC_DXB_ISDBTFSEG_PROCESS_SetState(TCC_DXB_ISDBTFSEG_STATE_RUNNING);

	#ifdef DEBUG_TCC_MALLOC_HOOKING_ENABLE
	tcc_malloc_hookinfo_proc_command(101, 0, 0, 0, 0);
	#endif

	/* init player */
	pPlayer = new ISDBTPlayer();
	pPlayer->setListener(TCC_DXB_ISDBTFSEG_PROCESS_Notifier);

	return 0;
}

void TCC_DXB_ISDBTFSEG_PROCESS_Deinit(void)
{
	TCC_DXB_ISDBTFSEG_CMD_DeinitCmd();

	TCC_DXB_ISDBTFSEG_PROCESS_lock_deinit();

	TCC_DXB_ISDBTFSEG_PROCESS_SetProcState(TCC_DXB_ISDBTFSEG_PROC_STATE_NONE);
	TCC_DXB_ISDBTFSEG_PROCESS_SetState(TCC_DXB_ISDBTFSEG_STATE_EXIT);
}
