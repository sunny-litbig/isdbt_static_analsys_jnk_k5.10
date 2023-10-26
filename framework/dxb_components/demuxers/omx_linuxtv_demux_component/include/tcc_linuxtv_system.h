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
#ifndef _TCC_LINUXTV_SYSTEM_H_
#define _TCC_LINUXTV_SYSTEM_H_

#define DEMUXCLOCK_MAX	2

struct DemuxClock
{
	INT64	  msPCR;									   /* millisecond */
	INT64	  msSTC;									   /* millisecond */
	INT64	  msPreStc; 								   /* millisecond */
	INT32	  msOffset; 								   /* difference between PCR and STC */
	INT32	  pause;									   /* 1: pause, 0: running */
	pthread_mutex_t    lockPcr;
};

typedef struct {
	pfnDxB_DEMUX_Notify	gpfnSectionNotify;
	pfnDxB_DEMUX_AllocBuffer gpfnSectionAllocBuffer;

	pfnDxB_DEMUX_PES_Notify gpfnSubtitleNotify;
	pfnDxB_DEMUX_AllocBuffer gpfnSubtitleAllocBuffer;
	pfnDxB_DEMUX_FreeBufferForError gpfnSubtitleFreeBuffer;

	pfnDxB_DEMUX_PES_Notify gpfnTeletextNotify;
	pfnDxB_DEMUX_AllocBuffer gpfnTeletextAllocBuffer;
	pfnDxB_DEMUX_FreeBufferForError gpfnTeletextFreeBuffer;

	pfnDxB_DEMUX_PES_Notify gpfnUserDefinedNotify;
	pfnDxB_DEMUX_AllocBuffer gpfnUserDefinedAllocBuffer;
	pfnDxB_DEMUX_FreeBufferForError gpfnUserDefinedFreeBuffer;

	unsigned int guiPCRPid[2];

	unsigned int g_ISDBT_Feature;
	unsigned int g_Parent_Lock;
	int g_LastInputtedPTS;
	pthread_mutex_t    TCCSTCLock;
	int giSTCDelayOffset[2];
	struct DemuxClock demux_clock[DEMUXCLOCK_MAX];
} DemuxSystem, *HandleDemuxSystem;

int TCCDEMUX_SetISDBTFeature(HandleDemuxSystem pHandle, unsigned int feature);
int TCCDEMUX_GetISDBTFeature(HandleDemuxSystem pHandle, unsigned int *feature);
int TCCDEMUX_SetParentLock(HandleDemuxSystem pHandle, unsigned int lock);
int TCCDemux_GetBrazilParentLock(HandleDemuxSystem pHandle);
MPEGDEMUX_ERROR TCCDemux_ResetSystemTime(HandleDemuxSystem pHandle);
INT64 TCCDemux_GetSystemTime(HandleDemuxSystem pHandle, int index);
MPEGDEMUX_ERROR TCCDemux_UpdatePCR (HandleDemuxSystem pHandle, INT32 itype, int index, INT64 pcr);
long long TCCDEMUX_GetSTC(HandleDemuxSystem pHandle, HandleDemux hDMX, int itype, long long lpts, int index, int bPlayMode, int log);
int TCCDEMUX_CBSection(HandleDemuxFilter slot, unsigned char *buf, int size, unsigned long long pts, int done, void *appData);
int TCCDEMUX_CBSubtitlePES(HandleDemuxFilter slot, unsigned char *buf, int size, unsigned long long pts, int done, void *appData);
int TCCDEMUX_CBTeletextPES (HandleDemuxFilter slot, unsigned char *buf, int size, unsigned long long pts, int done, void *appData);
int TCCDEMUX_CBUserDefinedPES(HandleDemuxFilter slot, unsigned char *buf, int size, unsigned long long pts, int done, void *appData);
int TCCDEMUX_Init(HandleDemuxSystem pHandle);
int TCCDEMUX_DeInit(HandleDemuxSystem pHandle);
int TCCDEMUX_ReInit(HandleDemuxSystem pHandle);
void TCCDEMUX_SetSTCOffset(HandleDemuxSystem pHandle, int iType, int iIndex, int iOffset, int iOption);

//For seamless switching
void TCCDEMUX_Compensation_STC(int delay);
int TCCDEMUX_Compensation_get_Excute(void);
int TCCDEMUX_Compensation_Enable(void);
unsigned long long TCCDEMUX_Compensation_get_STC_Delay_Time(void);
#endif
