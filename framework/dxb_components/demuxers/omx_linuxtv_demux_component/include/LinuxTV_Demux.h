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
#ifndef __LINUXTV_DEMUX_H__
#define __LINUXTV_DEMUX_H__

/* modify for linux */
#ifndef __user
#define __user
#endif

#include <dmx.h>
#include <sys/poll.h>

#include "tsemaphore.h"
#include "tcc_ts_parser.h"
#include "tcc_stream_parser.h"

//#define USE_CALLBACK_BUFFER_FROM_PVR
#define PES_TIMER_CALLBACK

typedef enum {
	NO_ERROR = 0,
	NO_HANDLER = -1,
	ERROR_HANDLER = -2,
	ERROR_THREAD = -3,
	NOT_SUPPORT = -4,
	UNKNOWN_ERROR = -5,
} ERROR_TYPE;

typedef enum {
	DEMUX_STATE_STOP = 0,
	DEMUX_STATE_START,
	DEMUX_STATE_CLOSE,
} demux_state;

typedef enum {
	FILTER_STATE_FREE = 0,
	FILTER_STATE_ALLOCATED,
	FILTER_STATE_SET,
	FILTER_STATE_GO,
} filter_state;

typedef enum {
	STREAM_VIDEO_0 = 0,
	STREAM_VIDEO_1,
	STREAM_AUDIO_0,
	STREAM_AUDIO_1,
	STREAM_PCR_0,
	STREAM_PCR_1,
	STREAM_PES,
	STREAM_SECTION,
	MAX_THREAD_NUM,
} STREAM_TYPE;

typedef enum {
	PES_FILTER =0,
	SEC_FILTER,
	PCR_FILTER,
} FILTER_TYPE;

#define FLAGS_VIDEO_0    (1 << STREAM_VIDEO_0)
#define FLAGS_VIDEO_1    (1 << STREAM_VIDEO_1)
#define FLAGS_AUDIO_0    (1 << STREAM_AUDIO_0)
#define FLAGS_AUDIO_1    (1 << STREAM_AUDIO_1)
#define FLAGS_PCR_0      (1 << STREAM_PCR_0)
#define FLAGS_PCR_1      (1 << STREAM_PCR_1)
#define FLAGS_PES        (1 << STREAM_PES)
#define FLAGS_SECTION    (1 << STREAM_SECTION)
#define FLAGS_DVR_INPUT  (0x00010000)

typedef void* HandleDemux;
typedef struct ltv_dmx_filter* HandleDemuxFilter;

typedef int (*FILTERPUSHCALLBACK) (HandleDemuxFilter hDemuxFilter, unsigned char *buf, int size, unsigned long long pts, int flags, void *appData);

typedef struct ltv_dmx_filter {
	HandleDemux        hDMX;
	void*              pvParent;

	int                iDevDemux;
	struct dmx_sct_filter_params sctFilterParams;
	struct dmx_pes_filter_params pesFilterParams;

	FILTER_TYPE        eFilterType;
	STREAM_TYPE        eStreamType;
	filter_state       state;
	demux_state        request_state;
	int                isFromPVR;

	int                nFreeCheck;
	TS_PARSER*         pTsParser;
	ST_STREAM_PARSER*  pStreamParser;	//for Stream parser for video only
	FILTERPUSHCALLBACK pushCallback;
	void*              appData;
	void*              pUserData;
	int                iRequestId;

    int                nSendCount;
    int                nSTCOffset; //STC offset for this handle
	unsigned char  fDiscontinuity;
    long long          nPrevPTS; //Previous PTS (ms)
	int                nBufferSize;
	int                nWriteSize;
	int                nEsMaxSize;
	int                bUseStaticBuffer;
	unsigned char*     pWriteBuffer;
	unsigned char*     pReadBuffer;
	unsigned char*     pBaseBuffer;
	void*              pPortBuffer;   //OMX Port buffer

	pthread_mutex_t    tLock;

	int                fUseCBTimer;
	int                CBTimerValue_usec;
	long long          timer_expire;

	struct {
		unsigned char *pBuffer;
		int size;
		int push;
		int pop;
	} tInputBuffer;
} ltv_dmx_filter_t;

HandleDemuxFilter LinuxTV_Demux_CreateFilter(HandleDemux hDMX, STREAM_TYPE eStreamType, int iRequestId, void *arg, FILTERPUSHCALLBACK callback);
void               LinuxTV_Demux_DestroyFilter(HandleDemuxFilter hDemuxFilter);
int                LinuxTV_Demux_EnableFilter(HandleDemuxFilter hDemuxFilter);
int                LinuxTV_Demux_IsEnabledFilter(HandleDemuxFilter hDemuxFilter);
int                LinuxTV_Demux_DisableFilter(HandleDemuxFilter hDemuxFilter);
int                LinuxTV_Demux_SetPESFilter(HandleDemuxFilter hDemuxFilter, unsigned int uiPid, dmx_pes_type_t iType);
int                LinuxTV_Demux_SetSectionFilter(HandleDemuxFilter hDemuxFilter, unsigned int uiPid, int iRepeat, int iCheckCRC,
                             char *pucValue, char *pucMask, char *pucExclusion, unsigned int ulFilterLen);
int                LinuxTV_Demux_SetStaticBuffer(HandleDemuxFilter hDemuxFilter, unsigned char *pBuffer, int size);
int                LinuxTV_Demux_SetCallBackTimer(HandleDemuxFilter hDemuxFilter, int fEnable, int time_usec);
int                LinuxTV_Demux_GetCallBackTimer(HandleDemuxFilter hDemuxFilter, int *pfEnable, int *p_time_usec);
int                LinuxTV_Demux_GetID(HandleDemuxFilter hDemuxFilter, int *iRequestID);
int                LinuxTV_Demux_SetStreamType(HandleDemuxFilter hDemuxFilter, INT32 iType, INT32 ParserOnOff);
int                LinuxTV_Demux_UpdateWriteBuffer(HandleDemuxFilter hDemuxFilter, unsigned char *pucReadPtr, unsigned char *pucWritePtr, unsigned int uiWriteSize);
int                LinuxTV_Demux_Flush(HandleDemuxFilter hDemuxFilter);

HandleDemux        LinuxTV_Demux_Init(int iDevIdx, int iFlags);
void               LinuxTV_Demux_DeInit(HandleDemux hDMX);
HandleDemuxFilter  LinuxTV_Demux_GetFilter(HandleDemux hDMX, int iRequestId);
int                LinuxTV_Demux_GetSTC(HandleDemux hDMX, int index, long long *lRealSTC);
int                LinuxTV_Demux_Pause(HandleDemux hDMX);
int                LinuxTV_Demux_Stop(HandleDemux hDMX);
int                LinuxTV_Demux_Resume(HandleDemux hDMX);
int                LinuxTV_Demux_SetPVR(HandleDemux hDMX, int on);
int                LinuxTV_Demux_InputBuffer(HandleDemux hDMX, unsigned char *pucBuff, int iSize);
int                LinuxTV_Demux_Record_Start(HandleDemux hDMX, const char *pucFileName, int v_pid, int v_type, int a_pid, int a_type, int pcr_pid);
void               LinuxTV_Demux_Record_Stop(HandleDemux hDMX);

#endif //__LINUXTV_DEMUX_H__
