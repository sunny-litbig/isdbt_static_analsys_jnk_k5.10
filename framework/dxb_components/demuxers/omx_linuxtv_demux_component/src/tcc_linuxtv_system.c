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
#define LOG_TAG	"OMX_DXB_SYSTEM"
#include <utils/Log.h>
#include <errno.h>
#include <omx_linuxtv_demux_component.h>

#include "globals.h"
#include "tcc_dxb_interface_demux.h"
#include "LinuxTV_Demux.h"

#include "tcc_linuxtv_system.h"


/****************************************************************************
DEFINE
****************************************************************************/
#define DELAY 					500
#define __ABS__(a) 				((a) >= 0 ? (a) : (-(a)))
#define DEFAULT_STC_DELAY       -20 // the delay of devide driver
#define DEBUG_MSG(msg...)		ALOGD(msg)

#define	JVCK_STC_OFFSET	-200	//customer JVC Kenwood

#define MAX_STC_OFFSET		0
#define MIN_STC_OFFSET		-2500


/****************************************************************************
TYPE DEFINE
****************************************************************************/


/****************************************************************************
STATIC VARIABLES
****************************************************************************/

static int compensation_delay_check= 0;
static int compensation_enable = 0;
static int compensation_stc_delay = -1;
static int prev_compensation_stc_delay = -1;
static unsigned int g_Setdual_Index;

/****************************************************************************
GLOBAL VARIABLES
****************************************************************************/


/****************************************************************************
EXTERNAL VARIABLES
****************************************************************************/


/****************************************************************************
EXTERNAL FUNCTIONS
****************************************************************************/
int TCCDEMUX_SetISDBTFeature(HandleDemuxSystem pHandle, unsigned int feature)
{
	pHandle->g_ISDBT_Feature = feature;
	return 0;
}
int TCCDEMUX_GetISDBTFeature(HandleDemuxSystem pHandle, unsigned int *feature)
{
	*feature = pHandle->g_ISDBT_Feature;
	return 0;
}

int TCCDEMUX_SetParentLock(HandleDemuxSystem pHandle, unsigned int lock)
{
	if (pHandle->g_Parent_Lock != lock)
	{
		pHandle->g_Parent_Lock = lock;
		ALOGI("g_Parent_Lock : %d\n", pHandle->g_Parent_Lock);
	}
	return 0;
}

static unsigned int TCCDEMUX_GetParentLock(HandleDemuxSystem pHandle)
{
	return pHandle->g_Parent_Lock;
}

int TCCDemux_GetBrazilParentLock(HandleDemuxSystem pHandle)
{
	return (IsdbtSInfo_IsForBrazil(pHandle->g_ISDBT_Feature) && pHandle->g_Parent_Lock);
}

int TCCDEMUX_SetDualchannel(unsigned int index)
{
	g_Setdual_Index = index;
	return 0;
}
MPEGDEMUX_ERROR TCCDemux_ResetSystemTime(HandleDemuxSystem pHandle)
{
	struct DemuxClock *pDemuxClock;
	int	i;

	for (i=0; i < DEMUXCLOCK_MAX; i++) {
		pDemuxClock = &pHandle->demux_clock[i];
		pDemuxClock->msPCR = 0;
		pDemuxClock->msPreStc = 0;
		pDemuxClock->msSTC = 0;
		pDemuxClock->msOffset = 0;
	}
	return MPEGDEMUX_ERR_NOERROR;
}

INT64 TCCDemux_GetSystemTime(HandleDemuxSystem pHandle, int index)
{
	struct timespec tspec;
	INT64 curStc, stc, diff;
	struct DemuxClock *pDemuxClock;

	if (index >= DEMUXCLOCK_MAX)	return 0;
	pDemuxClock = &pHandle->demux_clock[index];
	if (pDemuxClock->msPCR == 0)
	{
		return 0;
	}

	pthread_mutex_lock (&pDemuxClock->lockPcr);
	if (pDemuxClock->pause)
	{
		pthread_mutex_unlock (&pDemuxClock->lockPcr);
		return pDemuxClock->msPreStc;
	}

	clock_gettime(CLOCK_MONOTONIC , &tspec);
	curStc = ((INT64)tspec.tv_sec) * 1000 + ((INT64)tspec.tv_nsec) / 1000000;

	if (pDemuxClock->msSTC)
	{
		diff = curStc - pDemuxClock->msSTC;
	}
	else
	{
		diff = 0;
		pDemuxClock->msSTC = curStc;
	}

	stc = diff + pDemuxClock->msPCR + pDemuxClock->msOffset - DELAY;
	if (stc < 0)
	{
		stc = 0;
	}
	pDemuxClock->msPreStc = stc;
	pthread_mutex_unlock (&pDemuxClock->lockPcr);

	return stc;
}

/**
 * pcr is 90KHz tick count
 */
MPEGDEMUX_ERROR TCCDemux_UpdatePCR (HandleDemuxSystem pHandle, INT32 itype, int index, INT64 pcr)
{
	struct timespec tspec;
	INT64 msSTC = 0;
	struct DemuxClock *pDemuxClock;

	if (index >= DEMUXCLOCK_MAX)
		return MPEGDEMUX_ERR_ERROR;

	pDemuxClock = &pHandle->demux_clock[index];
	if(itype == 1) {
		ALOGE("TCCDemux_UpdatePCR : [%d]", itype);
		//if(pcr > 0) pcr += 200;
		pcr = pDemuxClock->msPCR - pcr;
	} else if (itype == 2) {
		return TCCDemux_ResetSystemTime(pHandle);
	}

	if(pDemuxClock->msPCR) {
		INT64 istc = 0;
		istc = TCCDemux_GetSystemTime(pHandle, index) + DELAY;
		if( __ABS__((int)(pcr - istc)) < 1000 ) //1000ms
			return 0;
		ALOGE("%s PCR[%d] %lld, STC %lld, DIFF %d\n", __func__, index, pcr, istc, (int)(pcr - istc));
	}

	pthread_mutex_lock (&pDemuxClock->lockPcr);

	clock_gettime(CLOCK_MONOTONIC , &tspec);
	msSTC = (INT64) tspec.tv_sec * 1000 + tspec.tv_nsec / 1000000;

	pDemuxClock->msSTC = msSTC;
	pDemuxClock->msPCR = pcr;
	pDemuxClock->msOffset = 0;

	pthread_mutex_unlock (&pDemuxClock->lockPcr);

	return MPEGDEMUX_ERR_NOERROR;
}

long long TCCDEMUX_GetSTC(HandleDemuxSystem pHandle, HandleDemux hDMX, int itype, long long lpts, int index, int bPlayMode, int log)
{
	long long ret = DxB_SYNC_OK;
	long long cur_msec;
	long long lRealSTC = 0, lPCR = 0;
	unsigned int uiPCRKernel = 0;
	int iDiff;

	if (index > 2) index = 0;

	if(pHandle->guiPCRPid[index] >= 0x1fff)
	{
		if(itype == DxB_SYNC_VIDEO){ //video case
			return DxB_SYNC_BYPASS; //bypass in vsync mode
		}
        	return DxB_SYNC_OK; //No need to check
	}

	pthread_mutex_lock(&pHandle->TCCSTCLock);

	if(bPlayMode == 0) {
		LinuxTV_Demux_GetSTC(hDMX, index, &lRealSTC);
		TCCDemux_UpdatePCR(pHandle, 0, index, lRealSTC);
		//IM238A-11 Seamless switch between 1seg and 12seg
		if(compensation_delay_check == 0 && compensation_stc_delay > 0 && index == 0 && g_Setdual_Index == 1)
		{
			//reset value
			pHandle->giSTCDelayOffset[0] = 0;
			pHandle->giSTCDelayOffset[1] = -300;

			prev_compensation_stc_delay = compensation_stc_delay;
			if(compensation_stc_delay > 0){
				pHandle->giSTCDelayOffset[0] -= (compensation_stc_delay);
			}
			ALOGE("DFS PCR compensation [%d] offset[%d]", compensation_stc_delay, pHandle->giSTCDelayOffset[0]);
			compensation_delay_check = 1;
			compensation_enable = 1;
			g_Setdual_Index = 0;
		}
	}else if (bPlayMode == 1) {
		lRealSTC = TCCDemux_GetSystemTime(pHandle, index);
	}else {
		lRealSTC = 0;
	}

	if(lRealSTC == 0)
	{
		if(itype == DxB_SYNC_VIDEO) //video case
            ret = DxB_SYNC_BYPASS; //bypass in vsync mode
		goto __ret__;
	}

	if (bPlayMode == 0)
	{
		if(lRealSTC)
			lRealSTC += pHandle->giSTCDelayOffset[index];
	}

	iDiff = (int)(lpts - lRealSTC);
	if(itype == DxB_SYNC_VIDEO) //video case
	{
		//if(log) ALOGE("VIDEO[%d] STC(%d) PTS(%d) DIFF(%d)", index, (int)lRealSTC, (int)lpts, (int)iDiff);
		if(iDiff < -500 || iDiff > 50)
		{
			if(iDiff < -500 )
			{
				if(log) ALOGE("VIDEO[%d] FRAME DROP %d  giSTCDelayOffset[%d] = %d \n", index, iDiff, index, pHandle->giSTCDelayOffset[index]);
				ret = DxB_SYNC_DROP;  //drop frame
			}
			else if(iDiff < 800)
			{
				ret = DxB_SYNC_WAIT; //wait frame
			}
			else if(iDiff < 5000)
			{
				ret = DxB_SYNC_LONG_WAIT; //long wait frame
			}
			else
			{
				if(log) ALOGE("VIDEO[%d] FRAME DROP %d  giSTCDelayOffset[%d] = %d \n", index, iDiff, index, pHandle->giSTCDelayOffset[index]);
				ret = DxB_SYNC_DROP;  //drop frame
			}
		}
	}
	else if(itype == DxB_SYNC_AUDIO ) //audio case
	{
		//if(log) ALOGE("AUDIO[%d] STC(%d) PTS(%d) DIFF(%d)", index, (int)lRealSTC, (int)lpts, (int)iDiff);
        #define AUD_PTS_MAX 200//50
		#define AUD_PTS_MIN 50//(-50)
		if(iDiff < AUD_PTS_MIN || iDiff > AUD_PTS_MAX)
		{
			if(iDiff < AUD_PTS_MIN )
			{
				if(log) ALOGE("AUDIO[%d] FRAME DROP %d  giSTCDelayOffset[%d] = %d \n", index, iDiff, index, pHandle->giSTCDelayOffset[index]);
				ret = DxB_SYNC_DROP;  //drop frame
			}
			else if(iDiff < 5000)
			{
				ret = DxB_SYNC_WAIT; //wait frame
			}
			else
			{
				if(log) ALOGE("AUDIO[%d] FRAME DROP %d  giSTCDelayOffset[%d] = %d \n", index, iDiff, index, pHandle->giSTCDelayOffset[index]);
				ret = DxB_SYNC_DROP;  //drop frame
			}
		}
	}
	else
		ret = lRealSTC;

__ret__:
	pthread_mutex_unlock(&pHandle->TCCSTCLock);
	return ret;

}

int TCCDEMUX_CBSection(HandleDemuxFilter slot, unsigned char *buf, int size, unsigned long long pts, int done, void *appData)
{
	HandleDemuxSystem pHandle = (HandleDemuxSystem) appData;
	int iRequestID = 0;
	unsigned char *pSendBuffer;
	LinuxTV_Demux_GetID(slot, &iRequestID);
	iRequestID -= TCCSWDEMUXER_SECTIONFILTER_BEGIN;
	//DEBUG_MSG("%s - RequestID %d\n", __func__, iRequestID);
	pHandle->gpfnSectionAllocBuffer(size, &pSendBuffer);
	memcpy(pSendBuffer, buf, size );
	pHandle->gpfnSectionNotify(pSendBuffer, size, iRequestID, slot->pUserData);
	return 0;
}

int TCCDEMUX_CBSubtitlePES(HandleDemuxFilter slot, unsigned char *buf, int size, unsigned long long pts, int done, void *appData)
{
	HandleDemuxSystem pHandle = (HandleDemuxSystem) appData;
	int iRequestID = 0;
	unsigned char *pSendBuffer;

	if(TCCDemux_GetBrazilParentLock(pHandle)){
		return 1;
	}

	LinuxTV_Demux_GetID(slot, &iRequestID);
	iRequestID -= TCCSWDEMUXER_PESFILTER_BEGIN;
	pHandle->gpfnSubtitleAllocBuffer(size, &pSendBuffer);
	memcpy(pSendBuffer, buf, size );
	pHandle->gpfnSubtitleNotify(pSendBuffer, size, pts, iRequestID, slot->pUserData);
	return 0;
}

int TCCDEMUX_CBTeletextPES(HandleDemuxFilter slot, unsigned char *buf, int size, unsigned long long pts, int done, void *appData)
{
	HandleDemuxSystem pHandle = (HandleDemuxSystem) appData;
	int iRequestID = 0;
	unsigned char *pSendBuffer;

	if(TCCDemux_GetBrazilParentLock(pHandle)){
		return 1;
	}

	LinuxTV_Demux_GetID(slot, &iRequestID);
	iRequestID -= TCCSWDEMUXER_PESFILTER_BEGIN;
	pHandle->gpfnTeletextAllocBuffer(size, &pSendBuffer);
	memcpy(pSendBuffer, buf, size );
	pHandle->gpfnTeletextNotify(pSendBuffer, size, pts, iRequestID, slot->pUserData);
	return 0;
}

int TCCDEMUX_CBUserDefinedPES(HandleDemuxFilter slot, unsigned char *buf, int size, unsigned long long pts, int done, void *appData)
{
	HandleDemuxSystem pHandle = (HandleDemuxSystem) appData;
	int iRequestID = 0;
	unsigned char *pSendBuffer;

	LinuxTV_Demux_GetID(slot, &iRequestID);
	iRequestID -= TCCSWDEMUXER_PESFILTER_BEGIN;
	pHandle->gpfnUserDefinedAllocBuffer(size, &pSendBuffer);
	memcpy(pSendBuffer, buf, size );
	pHandle->gpfnUserDefinedNotify(pSendBuffer, size, pts, iRequestID, slot->pUserData);
	return 0;
}

int TCCDEMUX_Init(HandleDemuxSystem pHandle)
{
	int i;
	struct DemuxClock *pDemuxClock;

	DEBUG_MSG("in %s\n", __func__);

	pthread_mutex_init(&pHandle->TCCSTCLock, NULL);
	pHandle->giSTCDelayOffset[0] = 0;
	pHandle->giSTCDelayOffset[1] = 0;
	compensation_stc_delay = 0;
	compensation_delay_check = 0;
	prev_compensation_stc_delay = 0;
	compensation_enable = 0;

	for (i=0; i < DEMUXCLOCK_MAX; i++) {
		pDemuxClock = &pHandle->demux_clock[i];
		pDemuxClock->msPCR = 0;
		pDemuxClock->msPreStc = 0;
		pDemuxClock->msSTC = 0;
		pDemuxClock->msOffset = 0;
		pthread_mutex_init(&pDemuxClock->lockPcr, NULL);
	}

	pHandle->guiPCRPid[0] = 0xffff;
	pHandle->guiPCRPid[1] = 0xffff;

	return 0;
}

int TCCDEMUX_DeInit(HandleDemuxSystem pHandle)
{
	int	i;
	struct DemuxClock *pDemuxClock;

	DEBUG_MSG("in %s\n", __func__);

	pHandle->giSTCDelayOffset[0] = 0;
	pHandle->giSTCDelayOffset[1] = 0;
	compensation_stc_delay = 0;
	compensation_delay_check = 0;
	prev_compensation_stc_delay = 0;
	compensation_enable = 0;
	for (i=0; i < DEMUXCLOCK_MAX; i++) {
		pDemuxClock = &pHandle->demux_clock[i];
		pthread_mutex_destroy(&pDemuxClock->lockPcr);
	}
	pthread_mutex_destroy(&pHandle->TCCSTCLock);

	return 0;
}

int TCCDEMUX_ReInit(HandleDemuxSystem pHandle)
{
	DEBUG_MSG("in %s\n", __func__);

	pHandle->giSTCDelayOffset[0] = 0;
	pHandle->giSTCDelayOffset[1] = 0;
	return 0;
}

void TCCDEMUX_SetSTCOffset(HandleDemuxSystem pHandle, int iType, int iIndex, int iOffset, int iOption)
{
	if(compensation_enable == 1)
	{
		compensation_enable = 0;
	}

	if(iIndex == 0){
		pHandle->giSTCDelayOffset[0] = -200;
	}else if(iIndex == 1){
		pHandle->giSTCDelayOffset[1] = -300;
	}
	ALOGE("\033[32m [BDS] [%s]Offset to [%d] [Index = %d, Type = %d]\033[m\n", __func__, pHandle->giSTCDelayOffset[iIndex], iIndex, iType);
}

void TCCDEMUX_Compensation_STC(int delay){
	compensation_stc_delay = delay;
	if(prev_compensation_stc_delay != compensation_stc_delay){
		compensation_delay_check = 0;
	}
	ALOGE("[%s] compensation_stc_delay[%d]\n", __func__, compensation_stc_delay);
}

int TCCDEMUX_Compensation_get_Excute(void){
	return compensation_delay_check;
}

int TCCDEMUX_Compensation_Enable(void){
	return compensation_enable;
}

unsigned long long TCCDEMUX_Compensation_get_STC_Delay_Time(void){
	return (unsigned long long)compensation_stc_delay;
}
