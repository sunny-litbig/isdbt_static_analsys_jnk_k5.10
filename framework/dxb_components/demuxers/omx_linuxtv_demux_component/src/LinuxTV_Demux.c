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
#define LOG_TAG	"LINUXTV_DEMUX"
#include <utils/Log.h>
#include <sys/time.h> // for gettimeofday()
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>

#include <OMX_Core.h>
#include <omx_tcc_thread.h>
#include <TCCMemory.h>

#include <tcc_pthread_cond.h>
#include <tcc_dxb_interface_type.h>

#include "LinuxTV_Demux.h"

//#define DBG_MSG
#ifdef DBG_MSG
#define DEBUG_MSG(msg...)	ALOGD(msg)
#else
#define DEBUG_MSG(msg...)
#endif

#define CHECK_SECTION_SIZE_INVALID
#define DEBUG_EMM_SIZE_INVALID

//#define DEBUG_SET_FILTER_TIME
#ifdef  DEBUG_SET_FILTER_TIME
struct timespec tspec1, tspec2;
#endif

#define DMX_0      "/dev/dvb%d.demux0"
#define DMX_1      "/dev/dvb%d.demux1"

#define DVR_BUFFER_SIZE			(1024*1024*10)			// 10MB
#define DVR_READ_SIZE			(PACKETSIZE*20)

#define SECTION_BUFFER_SIZE		(4096 + PACKETSIZE)
#define PES_BUFFER_SIZE			(1024*1024)

#define DMX_SECTION_BUFFER_SIZE (1024*128)
#define DMX_VIDEO_BUFFER_SIZE	(PACKETSIZE*50*1024)
#define DMX_AUDIO_BUFFER_SIZE	(PACKETSIZE*10*1024)
#define DMX_DEFAULT_BUFFER_SIZE	(PACKETSIZE*50)

#define DMX_READ_SIZE			(PACKETSIZE*1024)

#define DMX_MAX_FILTER          32 // HardWare Limitation
#define MAX_FILTER_NUM	        64 // SoftWare Limitation

typedef enum {
	THREAD_STATE_STOP  = 0x00,
	THREAD_STATE_RUN   = 0x01,
	THREAD_STATE_SKIP  = 0x02,
	THREAD_STATE_PAUSE = 0x04,
	THREAD_STATE_PVR   = 0x08,
} thread_state;

typedef struct ltv_dmx_thread {
	HandleDemux       hDMX;
	STREAM_TYPE       eStreamType;

	pthread_t         tThread;
	pthread_mutex_t   tLock;
	pthread_cond_t    tCondition;

	thread_state      iDestState;
	thread_state      iCurrState;

	unsigned int      iUpdatedDemuxList;
	unsigned int      iUpdatedInputBuffer;

	unsigned int      filternum;
	unsigned int      running;         // the number of started filter
	ltv_dmx_filter_t* filters[MAX_FILTER_NUM];
	struct pollfd     pfd[MAX_FILTER_NUM];
} ltv_dmx_thread_t;

typedef struct {
	pthread_t         tThread;
	thread_state      state;
	int               iRequestStop;
	int               iDevDemux;
	int               iDevFile;
	int               bFirst;
	long long         iOffsetPTS;
	long long         iCurrPTS;
	long long         iPrevPTS;
	int               iVideoPID;
	int               iVideoType;
	int               iAudioPID;
	int               iAudioType;
	int               iPCRPID;
} ltv_dmx_record_t;

typedef struct {
	int               iDevIdx;
	int               iDvrInput;
	int               iDummyFilter;

	ltv_dmx_filter_t  tDemuxFilterList[MAX_FILTER_NUM];
	ltv_dmx_thread_t  tDemuxThreadList[MAX_THREAD_NUM];
	ltv_dmx_record_t  tDemuxRecord;
} ltv_dmx_t;

extern int dxb_omx_dxb_GetBuffer(HandleDemuxFilter slot, OMX_BUFFERHEADERTYPE **portbuf, void *appData);
extern int dxb_omx_dxb_ScrambleStateCallback(HandleDemuxFilter slot, int fScrambled, void *appData);

#ifdef PES_TIMER_CALLBACK
#include <sys/time.h> // for gettimeofday()

#if 0
static long long cc_time_get(void)
{
	struct timespec tspec;

	clock_gettime(CLOCK_MONOTONIC , &tspec);

	return (long long)tspec.tv_sec * 1000000 + (long long)tspec.tv_nsec / 1000;
}
#endif

static int cbtimer_reset(ltv_dmx_filter_t *pDemuxFilter)
{
	struct timespec tspec;
	long long time_curr;

	if( pDemuxFilter->fUseCBTimer == 0 )
	{
		// timer is not used.
		return 0;
	}

	clock_gettime(CLOCK_MONOTONIC , &tspec);
	time_curr = tspec.tv_sec * 1000000 + (long long)tspec.tv_nsec / 1000;
	pDemuxFilter->timer_expire = time_curr + pDemuxFilter->CBTimerValue_usec;
	//ALOGE("[%s] C %d, N %d",__func__, (int)(time_curr/1000), (int)(pDemuxFilter->timer_expire/1000));

	return 0;
}

static int cbtimer_expire_check(ltv_dmx_filter_t *pDemuxFilter)
{
	struct timespec tspec;
	long long time_curr;

	if( pDemuxFilter->fUseCBTimer == 0 )
	{
		// timer is not used.
		return 0;
	}

	if( pDemuxFilter->timer_expire == -1 || pDemuxFilter->nWriteSize == 0)
	{
		// timer is not set.
		return 0;
	}

	clock_gettime(CLOCK_MONOTONIC , &tspec);
	time_curr = tspec.tv_sec * 1000000 + (long long)tspec.tv_nsec / 1000;
	if( (time_curr-pDemuxFilter->timer_expire) > 0 )
	{
		// expired
		//ALOGE("[%s] C %d",__func__, (int)(time_curr/1000));
		return 1;
	}

	return 0;
}
#endif //ifdef PES_TIMER_CALLBACK

static void FlushData(ltv_dmx_filter_t *pDemuxFilter)
{
    /* callback forcely */
    if(pDemuxFilter->pTsParser->nPTS_flag) {
		if( pDemuxFilter->nWriteSize > 0 )
		{
	        pDemuxFilter->pushCallback(pDemuxFilter,
	                                   pDemuxFilter->pWriteBuffer,
	                                   pDemuxFilter->nWriteSize,
	                                   pDemuxFilter->pTsParser->nPTS,
	                                   pDemuxFilter->isFromPVR,
	                                   pDemuxFilter->appData);
			pDemuxFilter->fDiscontinuity = FALSE;
		}
        pDemuxFilter->pTsParser->nPTS_flag = 0;
    }

    pDemuxFilter->pWriteBuffer = pDemuxFilter->pBaseBuffer;
    pDemuxFilter->nWriteSize = 0;
    cbtimer_reset(pDemuxFilter);
    return;
}

static void processData(ltv_dmx_filter_t *pDemuxFilter, unsigned char *p, int len)
{
#if 0
	if(pDemuxFilter->pTsParser->nPayLoadStart == 1 && pDemuxFilter->nWriteSize) {
		if (pDemuxFilter->state == FILTER_STATE_GO)
		{
			pDemuxFilter->pushCallback(pDemuxFilter, pDemuxFilter->pWriteBuffer, pDemuxFilter->nWriteSize, pDemuxFilter->pTsParser->nPrevPTS, pDemuxFilter->isFromPVR, pDemuxFilter->appData);
		}
		pDemuxFilter->nWriteSize = 0;
	}
#else
	unsigned int nWriteBufferSize;
    if(	pDemuxFilter->pTsParser->nPTS_flag == 0
    ||	pDemuxFilter->pTsParser->nTSScrambled != 0
    ||	pDemuxFilter->pTsParser->nPESScrambled != 0
    ||	pDemuxFilter->pTsParser->nContentSize == 0)
        return;

	if(pDemuxFilter->pTsParser->nPayLoadStart == 1){
		if(pDemuxFilter->nWriteSize){
			/* flush remain data of previous PES */
            if (pDemuxFilter->state == FILTER_STATE_GO)
            {
				#ifdef DEBUG_TS_PACKET
				if(pDemuxFilter==g_pAudio1FilterHandle && pDemuxFilter->pTsParser->iPacketCount <= 100) {
					ALOGD("[%s:%d][1] Push callback %9d, 0x%08X, %d, %d",  __func__, __LINE__,
						(int)(pDemuxFilter->pTsParser->nPrevPTS/90), (int)(pDemuxFilter->pTsParser->nPrevPTS/90), pDemuxFilter->nWriteSize, pDemuxFilter->pTsParser->nContentSize);
                }
				#endif
                pDemuxFilter->pushCallback(pDemuxFilter,
                                           pDemuxFilter->pWriteBuffer,
                                           pDemuxFilter->nWriteSize,
                                           pDemuxFilter->pTsParser->nPrevPTS,
                                           pDemuxFilter->isFromPVR,
                                           pDemuxFilter->appData);
            }
			pDemuxFilter->fDiscontinuity = FALSE;
			pDemuxFilter->pWriteBuffer = pDemuxFilter->pBaseBuffer;
			pDemuxFilter->nWriteSize = 0;
		}
        pDemuxFilter->pTsParser->nPrevPTS = 0;
#ifdef PES_TIMER_CALLBACK
        cbtimer_reset(pDemuxFilter);
#endif
	}
#endif

	if (pDemuxFilter->pWriteBuffer == NULL) {
		if (NO_ERROR != pDemuxFilter->pStreamParser->getBuffer(pDemuxFilter,
																&pDemuxFilter->pWriteBuffer,
																(unsigned int *)&pDemuxFilter->nBufferSize))
		{
			return;
		}
		pDemuxFilter->nWriteSize = 0;
	}

	if (pDemuxFilter->bUseStaticBuffer == 1)
	{
		while(pDemuxFilter->pWriteBuffer < pDemuxFilter->pReadBuffer
			  && pDemuxFilter->pReadBuffer <= pDemuxFilter->pWriteBuffer + pDemuxFilter->nWriteSize + len) {
			usleep(10);
		}
	}

	memcpy(pDemuxFilter->pWriteBuffer + pDemuxFilter->nWriteSize, p, len);
	pDemuxFilter->nWriteSize += len;

#ifdef PES_TIMER_CALLBACK
    if( cbtimer_expire_check(pDemuxFilter) == TRUE ) {
        /* callback forcely */
        if(pDemuxFilter->pTsParser->nPTS_flag) {
            pDemuxFilter->pushCallback(pDemuxFilter,
                                       pDemuxFilter->pWriteBuffer,
                                       pDemuxFilter->nWriteSize,
                                       pDemuxFilter->pTsParser->nPTS,
                                       pDemuxFilter->isFromPVR,
                                       pDemuxFilter->appData);
			pDemuxFilter->fDiscontinuity = FALSE;
            //pDemuxFilter->pTsParser->nPTS_flag = 0;
        }
        if (pDemuxFilter->pTsParser->nContentSize >= pDemuxFilter->nWriteSize) {
            pDemuxFilter->pTsParser->nContentSize -= pDemuxFilter->nWriteSize;
        }
        pDemuxFilter->pWriteBuffer = pDemuxFilter->pBaseBuffer;
        pDemuxFilter->nWriteSize = 0;
		pDemuxFilter->pTsParser->nPTS = 0;
        cbtimer_reset(pDemuxFilter);
        return;
    }
#endif // PES_TIMER_CALLBACK

	if(pDemuxFilter->pTsParser->nContentSize && pDemuxFilter->nWriteSize &&
		(pDemuxFilter->nWriteSize == pDemuxFilter->pTsParser->nContentSize))
	{
	    if(pDemuxFilter->pTsParser->nPTS_flag) {
            if (pDemuxFilter->state == FILTER_STATE_GO)
            {
                pDemuxFilter->pushCallback(pDemuxFilter,
                                           pDemuxFilter->pWriteBuffer,
                                           pDemuxFilter->nWriteSize,
                                           pDemuxFilter->pTsParser->nPTS,
                                           pDemuxFilter->isFromPVR,
                                           pDemuxFilter->appData);
				pDemuxFilter->fDiscontinuity = FALSE;
            }
            pDemuxFilter->pTsParser->nPTS_flag = 0;
        }
	    pDemuxFilter->pWriteBuffer = pDemuxFilter->pBaseBuffer;
        pDemuxFilter->nWriteSize = 0;
#ifdef PES_TIMER_CALLBACK
        cbtimer_reset(pDemuxFilter);
#endif // PES_TIMER_CALLBACK
    }
}

static void processPacket(ltv_dmx_filter_t *pDemuxFilter, unsigned char *buf, int size)
{
	int bStreamParser = (TSDEMUXSTParser_IsAvailable(pDemuxFilter->pStreamParser) == 1) ? 1 : 0;
	int ret;
	ltv_dmx_thread_t *pDemuxThread = (ltv_dmx_thread_t*) pDemuxFilter->pvParent;

	while(size >= PACKETSIZE)
	{
		if (pDemuxFilter->request_state != DEMUX_STATE_START)
			return;
		if (pDemuxThread->iDestState & THREAD_STATE_SKIP)
			return;
		if (pDemuxThread->iDestState & THREAD_STATE_PAUSE)
		{
			usleep(10);
			continue;
		}

		ret = parseTs(pDemuxFilter->pTsParser, buf, PACKETSIZE, (pDemuxFilter->eFilterType == PCR_FILTER) ? 1 : 0);

		if(ret == 0) {
		//In ISDB-T, this routine was not necessary. this rouine is onlu for ATSC for notification of scrambled information.
#if 0
			if (pDemuxFilter->eFilterType == PES_FILTER) {
				//if(pDemuxFilter->pTsParser->nTSScrambled != 0 || pDemuxFilter->pTsParser->nPESScrambled != 0)
				{
					int nScrambled = (pDemuxFilter->pTsParser->nTSScrambled != 0) ?
						pDemuxFilter->pTsParser->nTSScrambled : pDemuxFilter->pTsParser->nPESScrambled;

					pDemuxFilter->pTsParser->nScrambleDetectPattern <<= 1;
					pDemuxFilter->pTsParser->nScrambleDetectPattern |= nScrambled;
					if(pDemuxFilter->pTsParser->nPrevScrambled != nScrambled) {
						if(pDemuxFilter->pTsParser->nScrambleDetectPattern == 0 || (~pDemuxFilter->pTsParser->nScrambleDetectPattern) == 0) {
							pDemuxFilter->pTsParser->nPrevScrambled = nScrambled;
							dxb_omx_dxb_ScrambleStateCallback(pDemuxFilter, nScrambled, pDemuxFilter->appData);
						}
					}
				}
			}
#endif
			if(pDemuxFilter->eFilterType == PCR_FILTER) {
				if(pDemuxFilter->pTsParser->nAdaptation.length && pDemuxFilter->pTsParser->nAdaptation.PCR) {
					if (pDemuxFilter->state == FILTER_STATE_GO)
					{
						pDemuxFilter->pushCallback(pDemuxFilter, NULL, 0,
												   pDemuxFilter->pTsParser->nAdaptation.PCR / 90, pDemuxFilter->isFromPVR,
												   pDemuxFilter->appData);
					}
				}
			} else if(pDemuxFilter->pTsParser->nLen > 0) {
				if(bStreamParser) {
					if (pDemuxFilter->pTsParser->nPTS == 0) {
						pDemuxFilter->pTsParser->nPTS = pDemuxFilter->pTsParser->nPrevPTS;
					}
					TSDEMUXSTParser_ProcessData(pDemuxFilter->pStreamParser,
												pDemuxFilter->pTsParser->pBuf,
												pDemuxFilter->pTsParser->nLen,
												pDemuxFilter->pTsParser->nPTS,
												pDemuxFilter->pTsParser->nPayLoadStart);
				} else {
					processData(pDemuxFilter,
								pDemuxFilter->pTsParser->pBuf,
								pDemuxFilter->pTsParser->nLen);
				}
			}
		} else if(ret == 1) {
			if (pDemuxFilter->eFilterType == PES_FILTER)
			{
				if(bStreamParser==0)
				{
					FlushData(pDemuxFilter);
					pDemuxFilter->fDiscontinuity = TRUE;
				}
			}
		} else if(ret == 4) {
			if (pDemuxFilter->eFilterType == PES_FILTER)
			{
				if(bStreamParser==0)
				{
					FlushData(pDemuxFilter);
					pDemuxFilter->fDiscontinuity = TRUE;
				}
			}
			continue;
		}
		size -= PACKETSIZE;
		buf += PACKETSIZE;
	}
}

static int InitDemuxFilter(ltv_dmx_filter_t *pDemuxFilter, unsigned int uiDevId)
{
	char dev[1024];
	ltv_dmx_t *pLtvDemux = (ltv_dmx_t*) pDemuxFilter->hDMX;

	if (pDemuxFilter->eFilterType == SEC_FILTER)
	{
		if (pLtvDemux->iDvrInput == 0)
		{
			sprintf(dev, DMX_1, uiDevId);
			pDemuxFilter->iDevDemux = open(dev, O_RDWR | O_NONBLOCK);
		}
		else // file input uses only DMX_0
		{
			sprintf(dev, DMX_0, uiDevId);
			pDemuxFilter->iDevDemux = open(dev, O_RDWR | O_NONBLOCK);
		}
		pDemuxFilter->nBufferSize = SECTION_BUFFER_SIZE;
	}
	else // PES or PCR filter
	{
		sprintf(dev, DMX_0, uiDevId);
		pDemuxFilter->iDevDemux = open(dev, O_RDWR | O_NONBLOCK);
		pDemuxFilter->nBufferSize = PES_BUFFER_SIZE;
	}

	if (pDemuxFilter->iDevDemux  < 0)
	{
		ALOGE("[%s] DMX OPEN fail(iRequestID:%d).\n", __func__, pDemuxFilter->iRequestId);
		return UNKNOWN_ERROR;
	}

	pDemuxFilter->isFromPVR = 0;
	pDemuxFilter->bUseStaticBuffer = 0;
	pDemuxFilter->pBaseBuffer = (unsigned char *)TCC_fo_malloc(__func__, __LINE__,pDemuxFilter->nBufferSize);

#ifdef PES_TIMER_CALLBACK
	pDemuxFilter->fUseCBTimer = 0;
	pDemuxFilter->CBTimerValue_usec = 0;
#endif

	pDemuxFilter->pTsParser = (TS_PARSER *) TCC_fo_calloc (__func__, __LINE__,1, sizeof(TS_PARSER));
	if(pDemuxFilter->pTsParser ==  NULL){
		ALOGE("[%s] pDemuxFilter->pTsParser calloc failed\n", __func__);
		return UNKNOWN_ERROR;
	}

	pDemuxFilter->pTsParser->nPID = 0xffff;

	pDemuxFilter->pStreamParser = NULL;

	pDemuxFilter->pPortBuffer = NULL;

	pthread_mutex_init(&pDemuxFilter->tLock, NULL);

	return NO_ERROR;
}

static int DeinitDemuxFilter(ltv_dmx_filter_t *pDemuxFilter)
{

	while (pDemuxFilter->pvParent != NULL)
		usleep(10);

	if(pDemuxFilter->iDevDemux >= 0) {
		close(pDemuxFilter->iDevDemux);
	}

	if(!pDemuxFilter->bUseStaticBuffer) {
		if(pDemuxFilter->pBaseBuffer) {
			TCC_fo_free (__func__, __LINE__,pDemuxFilter->pBaseBuffer);
		}
	}

	if(pDemuxFilter->pTsParser) {
		TCC_fo_free (__func__, __LINE__,pDemuxFilter->pTsParser);
	}

	if(pDemuxFilter->pStreamParser){
		TCC_fo_free (__func__, __LINE__,pDemuxFilter->pStreamParser);
	}

	if(pDemuxFilter->pPortBuffer){
		pDemuxFilter->pushCallback(pDemuxFilter, NULL, 0, 0, pDemuxFilter->isFromPVR, pDemuxFilter->appData);
	}

	pthread_mutex_destroy(&pDemuxFilter->tLock);

	if(pDemuxFilter->tInputBuffer.size > 0) {
		TCC_fo_free (__func__, __LINE__,pDemuxFilter->tInputBuffer.pBuffer);
	}

	pDemuxFilter->state = FILTER_STATE_FREE;

	return NO_ERROR;
}

static int LinuxTV_InitBuffer(ltv_dmx_filter_t *pDemuxFilter)
{
	DEBUG_MSG("[%s] \n", __func__);

	if(TSDEMUXSTParser_IsAvailable(pDemuxFilter->pStreamParser) == 1)
	{
		TSDEMUXSTParser_InitBuffer(pDemuxFilter->pStreamParser);
	}
	else
	{
		pDemuxFilter->pWriteBuffer = pDemuxFilter->pBaseBuffer;
		pDemuxFilter->pReadBuffer = pDemuxFilter->pBaseBuffer;
		pDemuxFilter->nWriteSize = 0;
	}
	pDemuxFilter->pTsParser->nContentSize = 0;
	pDemuxFilter->pTsParser->nPTS = 0;
	pDemuxFilter->pTsParser->nPrevPTS = 0;
	pDemuxFilter->pTsParser->nPTS_flag = 0;
	pDemuxFilter->pTsParser->nNextCC = -1;
	pDemuxFilter->tInputBuffer.pop = 0;
	pDemuxFilter->tInputBuffer.push = 0;

	return NO_ERROR;
}

static void UpdateDemuxList(ltv_dmx_thread_t * pDemuxThread)
{
	ltv_dmx_t *pLtvDemux = (ltv_dmx_t*) pDemuxThread->hDMX;
	ltv_dmx_filter_t *pDemuxFilter;
	int i, ret;

	pDemuxThread->filternum = 0;

	for (i = 0; i < MAX_FILTER_NUM; i++)
	{
		pDemuxFilter = &pLtvDemux->tDemuxFilterList[i];

		if (pDemuxFilter->state == FILTER_STATE_GO)
		{
			if (pDemuxThread->eStreamType != pDemuxFilter->eStreamType)
				continue;

			pDemuxThread->filters[pDemuxThread->filternum] = pDemuxFilter;

			if (pDemuxFilter->pvParent == NULL && pDemuxThread->running < DMX_MAX_FILTER)
			{
				pDemuxThread->running++;

				pDemuxFilter->pvParent = (void*) pDemuxThread;

				if (pDemuxThread->eStreamType == STREAM_SECTION)
				{
					pDemuxFilter->pTsParser->nPID = pDemuxFilter->sctFilterParams.pid;
					ret = ioctl(pDemuxFilter->iDevDemux, DMX_SET_FILTER, &pDemuxFilter->sctFilterParams);
				}
				else
				{
					pDemuxFilter->pTsParser->nPID = pDemuxFilter->pesFilterParams.pid;
					ret = ioctl(pDemuxFilter->iDevDemux, DMX_SET_PES_FILTER, &pDemuxFilter->pesFilterParams);
				}

				if (ret < 0)
				{
					ALOGE("[%s] DMX START fail(iRequestID:%d, PID:0x%04X)\n", __func__, pDemuxFilter->iRequestId, pDemuxFilter->pTsParser->nPID);
				}
				else
				{
					ALOGD("[%s] DMX START(iRequestID:%d, PID:0x%04X)\n", __func__, pDemuxFilter->iRequestId, pDemuxFilter->pTsParser->nPID);
				}

				pDemuxFilter->nSendCount = 0;
				pDemuxFilter->nSTCOffset = 0;
				pDemuxFilter->pTsParser->nPrevScrambled = 0;
				pDemuxFilter->pTsParser->nScrambleDetectPattern = 1;

				LinuxTV_InitBuffer(pDemuxFilter);

				#ifdef DEBUG_SET_FILTER_TIME
				if (pDemuxFilter->iRequestId < 0)
				{
					unsigned long long time;
					clock_gettime(CLOCK_MONOTONIC , &tspec2);
					time = (tspec2.tv_sec - tspec1.tv_sec)*1000 + (tspec2.tv_nsec - tspec1.tv_nsec)/1000000;
					ALOGD("Set Filter Time: %lld ms(iRequestId: %d)"
							, time
							, pDemuxFilter->iRequestId);
				}
				#endif
			}
			pDemuxThread->pfd[pDemuxThread->filternum].fd = pDemuxFilter->iDevDemux;
			pDemuxThread->pfd[pDemuxThread->filternum].events = POLLIN;

			pDemuxThread->filternum++;

			if (pDemuxFilter->eStreamType < STREAM_PES) // video, audio, pcr thread has only one filter.
			{
				break;
			}
		}
		else if (pDemuxFilter->state == FILTER_STATE_SET)
		{
			if (pDemuxFilter->pvParent != NULL && pDemuxThread->eStreamType == pDemuxFilter->eStreamType)
			{
				if (pDemuxFilter->request_state == DEMUX_STATE_CLOSE)
				{
					ALOGD("[%s] DMX CLOSE(iRequestID:%d, PID:0x%04X)\n", __func__, pDemuxFilter->iRequestId, pDemuxFilter->pTsParser->nPID);
					pDemuxFilter->pvParent = NULL;
					DeinitDemuxFilter(pDemuxFilter);
				}
				else
				{
					ret = ioctl(pDemuxFilter->iDevDemux, DMX_STOP, 0);

					if (ret < 0)
					{
						ALOGE("[%s] DMX STOP fail(iRequestID:%d, PID:0x%04X)\n", __func__, pDemuxFilter->iRequestId, pDemuxFilter->pTsParser->nPID);
					}
					else
					{
						ALOGD("[%s] DMX STOP(iRequestID:%d, PID:0x%04X)\n", __func__, pDemuxFilter->iRequestId, pDemuxFilter->pTsParser->nPID);
					}

					if (pDemuxFilter->pPortBuffer)
					{
						pDemuxFilter->pushCallback(pDemuxFilter, NULL, 0, 0, pDemuxFilter->isFromPVR, pDemuxFilter->appData);
					}
					pDemuxFilter->pvParent = NULL;
				}

				pDemuxThread->running--;
			}
		}
	}
}

static int UpdatePVR(ltv_dmx_thread_t * pDemuxThread)
{
	ltv_dmx_filter_t *pDemuxFilter;
	int i;

	if ((pDemuxThread->iCurrState & THREAD_STATE_PVR) == 0)
	{
		for (i = 0; i < (int)pDemuxThread->filternum; i++)
		{
			pDemuxFilter = pDemuxThread->filters[i];

			ioctl(pDemuxFilter->iDevDemux, DMX_STOP, 0);

			LinuxTV_InitBuffer(pDemuxFilter);
		}
		pDemuxThread->iUpdatedInputBuffer = 0;
		pDemuxThread->iCurrState |= THREAD_STATE_PVR;
	}
	else
	{
		pDemuxThread->iCurrState &= ~THREAD_STATE_PVR;

		for (i = 0; i < (int)pDemuxThread->filternum; i++)
		{
			pDemuxFilter = pDemuxThread->filters[i];

			LinuxTV_InitBuffer(pDemuxFilter);

			ioctl(pDemuxFilter->iDevDemux, DMX_START, 0);
		}
	}

	return NO_ERROR;
}

static int WaitEvent(ltv_dmx_thread_t *pDemuxThread)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC , &ts);
	ts.tv_sec += 1;

	pthread_mutex_lock(&pDemuxThread->tLock);
	if (pDemuxThread->iDestState & THREAD_STATE_RUN)
	{
		if (pDemuxThread->iUpdatedDemuxList)
			goto EXIT;

		if (pDemuxThread->iDestState & THREAD_STATE_PVR)
		{
			if ((pDemuxThread->iCurrState & THREAD_STATE_PVR) == 0)
				goto EXIT;

			if (pDemuxThread->iUpdatedInputBuffer)
				goto EXIT;
		}

		tcc_pthread_cond_timedwait(&pDemuxThread->tCondition, &pDemuxThread->tLock, &ts);
	}

EXIT:
	pthread_mutex_unlock(&pDemuxThread->tLock);

	return NO_ERROR;
}


#define LINUXTV_DEMUX_THREAD_POLLTIME 20
static void *LinuxTV_Demux_Thread(ltv_dmx_thread_t * pDemuxThread)
{
	ltv_dmx_t *pLtvDemux = (ltv_dmx_t*) pDemuxThread->hDMX;
	ltv_dmx_filter_t *pDemuxFilter;
	int i, j, received_j = 0;
	unsigned char *p = (unsigned char *)TCC_fo_malloc(__func__, __LINE__,DMX_READ_SIZE);
	unsigned char *received_p = (unsigned char *)TCC_fo_malloc(__func__, __LINE__,DMX_READ_SIZE);
	unsigned short usSection_bytes_read;

	#ifdef DEBUG_TS_TRANSPORT_ERROR
	TS_PARSER *pParser;
	struct timespec tspec_curr, tspec_prev;
	int iPrevPacketCount;
	int iPrevCountTSError;
	int iPrevCountDiscontinuityError;
	#endif

	ALOGD("LinuxTV_Demux_Thread(%d) start", pDemuxThread->eStreamType);

	#ifdef DEBUG_TS_TRANSPORT_ERROR
	memset(&tspec_prev, 0, sizeof(struct timespec));
	#endif

	while ((pDemuxThread->iDestState & THREAD_STATE_RUN) || pDemuxThread->filternum > 0)
	{
		// Check Demux List
		if (pDemuxThread->iUpdatedDemuxList)
		{
			while (1)
			{
				pthread_mutex_lock(&pDemuxThread->tLock);
				pDemuxThread->iUpdatedDemuxList = 0;
				pthread_mutex_unlock(&pDemuxThread->tLock);

				UpdateDemuxList(pDemuxThread);
				if (pDemuxThread->filternum == pDemuxThread->running)
					break;

				ALOGD("Pending Filter(%d)", pDemuxThread->filternum - pDemuxThread->running);

				if (pDemuxThread->running >= DMX_MAX_FILTER)
				{
					ALOGE("The number of filters exceeds %d (%d)", DMX_MAX_FILTER, pDemuxThread->filternum);
					break;
				}
			}
			continue;
		}

		if (pDemuxThread->filternum == 0)
		{
			WaitEvent(pDemuxThread);
			continue;
		}

		if (pDemuxThread->eStreamType == STREAM_PCR_0 || pDemuxThread->eStreamType == STREAM_PCR_1) // Do nothing because parsing the pcr at the driver.
		{
			if (pLtvDemux->iDvrInput == 0)                        // but file input(not pvr) does not use the driver.
			{
				WaitEvent(pDemuxThread);
				continue;
			}
		}

		if(poll(pDemuxThread->pfd, pDemuxThread->filternum, LINUXTV_DEMUX_THREAD_POLLTIME))
		{
			#ifdef DEBUG_TS_TRANSPORT_ERROR
			clock_gettime(CLOCK_MONOTONIC , &tspec_curr);
			#endif

			for (i = 0; i < (int)pDemuxThread->filternum; i++)
			{
				if ((pDemuxThread->iDestState & THREAD_STATE_RUN) == 0)
					break;
				if ((pDemuxThread->iDestState & THREAD_STATE_PVR) != 0)
					break;

				pDemuxFilter = pDemuxThread->filters[i];

				if(pDemuxThread->pfd[i].revents & POLLIN)
				{
					if(pDemuxFilter->pushCallback)
					{						
						pDemuxFilter->isFromPVR = 0;
						if(pDemuxFilter->eFilterType == SEC_FILTER){
							j = read(pDemuxFilter->iDevDemux, received_p, DMX_READ_SIZE);
							if(j > 0 && j < DMX_READ_SIZE){
								memcpy(p, received_p, j);//copy current data
								usSection_bytes_read=((received_p[1] & 0x0f) << 8) + received_p[2] + 3;//get section size
								while(j < usSection_bytes_read){
									received_j = read(pDemuxFilter->iDevDemux, received_p, DMX_READ_SIZE); //read again
									if(received_j > 0){
										if(j+received_j > DMX_READ_SIZE){
											//discard data
											memset(p, 0x0, DMX_READ_SIZE);
											j = 0;
											break;
										}else{
//											unsigned char tab = p[0];
//											ALOGD("[recover] tableID[%02X] parsing[%d] read[%d]\n", (tab&0xFE), usSection_bytes_read, j, received_j);
											memcpy(p+j, received_p, received_j);//copy current data next previous write position
											j += received_j;//current size + previous size									
										}
									}else{
										//discard data
										j = 0;
										break;
									}
								}
							}else{
							   //discard data
								j = 0;
							}
						}else{
							j = read(pDemuxFilter->iDevDemux, p, DMX_READ_SIZE);
						}

						if (pDemuxThread->iDestState & (THREAD_STATE_SKIP | THREAD_STATE_PAUSE))
							break;

						if(j > 0)
						{
							if(pDemuxFilter->eFilterType == SEC_FILTER)
							{
								if (pDemuxFilter->state == FILTER_STATE_GO)
								{
#ifdef  DEBUG_EMM_SIZE_INVALID
									unsigned char ucTableTID=p[0];
									if( (ucTableTID&0xFE) == 0x84 ) /* EMM_0_ID=0x84, EMM_1_ID=0x85 */
									{
										unsigned short usEMMSectionSize=((p[1] & 0x0f) << 8) + p[2] + 3;
										if( usEMMSectionSize != j )
										{
											ALOGD("[DMX_Thread] EMM size invalid. (%02X, %d, %d)\n", ucTableTID, usEMMSectionSize, j);
										}
									}
#endif

#ifdef CHECK_SECTION_SIZE_INVALID
									unsigned short usSectionSize=((p[1] & 0x0f) << 8) + p[2] + 3;
									if( usSectionSize == j ){
										pDemuxFilter->pushCallback(pDemuxFilter, p, j, 0, pDemuxFilter->isFromPVR, pDemuxFilter->appData);
									}
#else
									pDemuxFilter->pushCallback(pDemuxFilter, p, j, 0, pDemuxFilter->isFromPVR, pDemuxFilter->appData);
#endif									
								}
							}
							else
							{
								processPacket(pDemuxFilter, p, j);

								#ifdef DEBUG_TS_TRANSPORT_ERROR
								if( tspec_curr.tv_sec != tspec_prev.tv_sec )
								{
									int iDiffPacketCount;
									int iDiffTSErrorCount;
									int iDiffDiscontErrorCount;
									int iRateTSErrorCount;
									
									pParser = pDemuxFilter->pTsParser;
									if( pParser )
									{
										iDiffPacketCount = pParser->iPacketCount - iPrevPacketCount;
										iDiffTSErrorCount = pParser->iCountTSError - iPrevCountTSError; 
										iDiffDiscontErrorCount = pParser->iCountDiscontinuityError - iPrevCountDiscontinuityError;

										if( iDiffPacketCount>0 && iDiffTSErrorCount>0 )
										{
											iRateTSErrorCount = iDiffTSErrorCount*100/iDiffPacketCount;
											if( iRateTSErrorCount > 0 )
											{
												ALOGD("Trans Error[PID=%d] TSErrRate %2d%%(%d/%d)", pParser->nPID, iRateTSErrorCount, iDiffPacketCount, iDiffTSErrorCount );
											}
										}
										iPrevPacketCount = pParser->iPacketCount;
										iPrevCountTSError = pParser->iCountTSError; 
										iPrevCountDiscontinuityError = pParser->iCountDiscontinuityError;
										memcpy( &tspec_prev, &tspec_curr, sizeof(struct timespec));
									}
								}
								#endif	
							}
						}
						else if(j == -1)
						{
							ALOGE("buffer overflows! [PID:0x%X]", pDemuxFilter->pTsParser->nPID);
						}
					}
				}
			}
		}
	}

	if(p){
		TCC_fo_free(__func__,__LINE__,p);
	}
	
	if(received_p){
		TCC_fo_free(__func__,__LINE__,received_p);
	}

	pDemuxThread->iCurrState = THREAD_STATE_STOP;

	pthread_detach(pDemuxThread->tThread);

	ALOGD("LinuxTV_Demux_Thread(%d) end", pDemuxThread->eStreamType);
	return (void*)NULL;
}

static int LinuxTV_CreateThread(ltv_dmx_t *pLtvDemux, STREAM_TYPE eStreamType)
{
	ltv_dmx_thread_t *pDemuxThread = &pLtvDemux->tDemuxThreadList[eStreamType];
	char tname[512];

	if (pDemuxThread->iCurrState != THREAD_STATE_STOP)
	{
		ALOGE("Thread(%d) was already created", eStreamType);
		return ERROR_THREAD;
	}

	memset(pDemuxThread, 0x0, sizeof(ltv_dmx_thread_t));

	tcc_pthread_cond_init(&pDemuxThread->tCondition, NULL);
	pthread_mutex_init(&pDemuxThread->tLock, NULL);

	pDemuxThread->hDMX = pLtvDemux;
	pDemuxThread->eStreamType = eStreamType;
	pDemuxThread->iCurrState = THREAD_STATE_RUN;
	pDemuxThread->iDestState = THREAD_STATE_RUN;
	pDemuxThread->running = 0;
	pDemuxThread->iUpdatedDemuxList = 0;

	sprintf(tname, "LnxTVDmx_%x", eStreamType);

	if(TCCTHREAD_Create(&pDemuxThread->tThread, (pThreadFunc)LinuxTV_Demux_Thread,\
			tname, HIGH_PRIORITY_7, pDemuxThread))
	{
		ALOGE("LinuxTV_CreateThread : Failed to create thread(%d)", eStreamType);
		pthread_cond_destroy(&pDemuxThread->tCondition);
		pthread_mutex_destroy(&pDemuxThread->tLock);
		pDemuxThread->iDestState = THREAD_STATE_STOP;
		pDemuxThread->iCurrState = THREAD_STATE_STOP;
		return ERROR_THREAD;
	}

	return NO_ERROR;
}

static void LinuxTV_DestroyThread(ltv_dmx_t *pLtvDemux, STREAM_TYPE eStreamType)
{
	ltv_dmx_thread_t *pDemuxThread = &pLtvDemux->tDemuxThreadList[eStreamType];

	if (pDemuxThread->iDestState != THREAD_STATE_STOP)
	{
		pthread_mutex_lock(&pDemuxThread->tLock);
		pDemuxThread->iDestState = THREAD_STATE_STOP;
		pthread_cond_signal(&pDemuxThread->tCondition);
		pthread_mutex_unlock(&pDemuxThread->tLock);
	}
}

static int StopDummyDemux(ltv_dmx_t *pLtvDemux)
{
	if (pLtvDemux->iDummyFilter >= 0)
	{
		ALOGD("LinuxTV#%d Demux Stop", pLtvDemux->iDevIdx);
		close(pLtvDemux->iDummyFilter);
		pLtvDemux->iDummyFilter = -1;
	}
	return 0;
}

static int StartDummyDemux(ltv_dmx_t *pLtvDemux)
{
	struct dmx_pes_filter_params pesFilterParams;
	char dev[1024];

	sprintf(dev, DMX_0, pLtvDemux->iDevIdx);

	pLtvDemux->iDummyFilter = open(dev, O_RDWR);
	if (pLtvDemux->iDummyFilter < 0)
	{
		ALOGE("LinuxTV#%d Demux Ready Fail", pLtvDemux->iDevIdx);
		return UNKNOWN_ERROR;
	}

/*
	pesFilterParams.pid = 0x1fff;
	pesFilterParams.pes_type = DMX_PES_OTHER;
	pesFilterParams.input = DMX_IN_FRONTEND;
	pesFilterParams.output = DMX_OUT_TSDEMUX_TAP; //DMX_OUT_DECODER, DMX_OUT_TAP
	pesFilterParams.flags = DMX_IMMEDIATE_START;

	if (ioctl(pLtvDemux->iDummyFilter, DMX_SET_PES_FILTER, &pesFilterParams) != 0)
	{
		close(pLtvDemux->iDummyFilter);
		pLtvDemux->iDummyFilter = -1;
		ALOGE("LinuxTV#%d Demux Ready Fail", pLtvDemux->iDevIdx);
		return UNKNOWN_ERROR;
	}
*/
	ALOGD("LinuxTV#%d Demux Ready", pLtvDemux->iDevIdx);
	return 0;
}

static int RegisterDemux(ltv_dmx_filter_t *pDemuxFilter, ltv_dmx_thread_t *pDemuxThread)
{
	if (pDemuxThread != NULL)
	{
		pDemuxFilter->state = FILTER_STATE_GO;

		pthread_mutex_lock(&pDemuxThread->tLock);
		pDemuxThread->iUpdatedDemuxList = 1;
		pthread_cond_signal(&pDemuxThread->tCondition);
		pthread_mutex_unlock(&pDemuxThread->tLock);
	}

	return 0;
}

static int UnregisterDemux(ltv_dmx_filter_t *pDemuxFilter)
{
	ltv_dmx_thread_t *pDemuxThread = (ltv_dmx_thread_t*) pDemuxFilter->pvParent;

	pDemuxFilter->state = FILTER_STATE_SET;

	if (pDemuxThread != NULL)
	{
		pthread_mutex_lock(&pDemuxThread->tLock);
		pDemuxThread->iUpdatedDemuxList = 1;
		pthread_cond_signal(&pDemuxThread->tCondition);
		pthread_mutex_unlock(&pDemuxThread->tLock);
	}

	return 0;
}

static void Kernel_BufferSize(ltv_dmx_filter_t *pDemuxFilter)
{
	int buffer_size = DMX_DEFAULT_BUFFER_SIZE;

	switch(pDemuxFilter->eStreamType) {
		case STREAM_PCR_0:
		case STREAM_PCR_1:
		case STREAM_VIDEO_0:
		case STREAM_VIDEO_1:
			pDemuxFilter->nEsMaxSize = VIDEO_SLOT_ES_MIN_SIZE;
			pDemuxFilter->tInputBuffer.size = DMX_VIDEO_BUFFER_SIZE;
			buffer_size = DMX_VIDEO_BUFFER_SIZE;
			break;
		case STREAM_AUDIO_0:
		case STREAM_AUDIO_1:
			pDemuxFilter->nEsMaxSize = AUDIO_SLOT_ES_MIN_SIZE;
			pDemuxFilter->tInputBuffer.size = DMX_AUDIO_BUFFER_SIZE;
			buffer_size = DMX_AUDIO_BUFFER_SIZE;
			break;
		default:
			pDemuxFilter->nEsMaxSize = 0;
			if (pDemuxFilter->eFilterType == SEC_FILTER)
			{
				pDemuxFilter->tInputBuffer.size = 0;
				buffer_size = DMX_SECTION_BUFFER_SIZE;
			}
			else
			{
				pDemuxFilter->tInputBuffer.size = DMX_DEFAULT_BUFFER_SIZE;
				buffer_size = DMX_DEFAULT_BUFFER_SIZE;
			}
			break;
	}

	if(ioctl(pDemuxFilter->iDevDemux, DMX_SET_BUFFER_SIZE, buffer_size) < 0)
	{
		ALOGE("Failed to set buffer size!![%d]", pDemuxFilter->iDevDemux);
	}

	if (pDemuxFilter->tInputBuffer.size > 0)
	{
		pDemuxFilter->tInputBuffer.pBuffer = TCC_fo_malloc(__func__, __LINE__,pDemuxFilter->tInputBuffer.size);
	}
	pDemuxFilter->tInputBuffer.pop = 0;
	pDemuxFilter->tInputBuffer.push = 0;
}

static ltv_dmx_filter_t* linuxtv_dmx_createfilter
(
	HandleDemux hDMX,
	STREAM_TYPE eStreamType,
	int iRequestId,
	void *arg,
	FILTERPUSHCALLBACK callback
)
{
	ltv_dmx_t *pLtvDemux = (ltv_dmx_t*) hDMX;
	ltv_dmx_filter_t *pDemuxFilter;
	int i;

	if (pLtvDemux->tDemuxThreadList[eStreamType].iDestState == THREAD_STATE_STOP)
	{
		return NULL;
	}

	for (i = 0; i < MAX_FILTER_NUM; i++)
	{
		pDemuxFilter = &pLtvDemux->tDemuxFilterList[i];
		if(pDemuxFilter->state == FILTER_STATE_FREE)
		{
			pDemuxFilter->state = FILTER_STATE_ALLOCATED;
			break;
		}
	}

	if(i == MAX_FILTER_NUM)
	{
		ALOGE("[%s] Filter is FULL", __func__);
		return NULL;
	}

	switch(eStreamType) {
		case STREAM_PCR_0:
		case STREAM_PCR_1:
			pDemuxFilter->eFilterType = PCR_FILTER;
			break;
		case STREAM_SECTION:
			pDemuxFilter->eFilterType = SEC_FILTER;
			break;
		case STREAM_VIDEO_0:
		case STREAM_VIDEO_1:
		case STREAM_AUDIO_0:
		case STREAM_AUDIO_1:
		case STREAM_PES:
		default:
			pDemuxFilter->eFilterType = PES_FILTER;
			break;
	}

	pDemuxFilter->hDMX = pLtvDemux;
	pDemuxFilter->eStreamType = eStreamType;
	pDemuxFilter->nFreeCheck = 0;
	pDemuxFilter->iRequestId = iRequestId;
	pDemuxFilter->appData = arg;
	pDemuxFilter->pushCallback = callback;

	pDemuxFilter->pvParent = NULL;

	if (InitDemuxFilter(pDemuxFilter, pLtvDemux->iDevIdx) != NO_ERROR)
	{
		pDemuxFilter->state = FILTER_STATE_FREE;
		return NULL;
	}

	Kernel_BufferSize(pDemuxFilter);

	ALOGD("[%s] DMX OPEN(iRequestID:%d, Type:%d)\n", __func__, iRequestId, pDemuxFilter->eFilterType);

	return pDemuxFilter;
}

static void linuxtv_dmx_destroyfilter(ltv_dmx_filter_t *pDemuxFilter)
{
	if (pDemuxFilter->state == FILTER_STATE_GO) // Enabled filter is destroyed in the thread
	{
		pDemuxFilter->nFreeCheck = 1;

		while (pDemuxFilter->pvParent == NULL)
			usleep(10);

		pDemuxFilter->request_state = DEMUX_STATE_CLOSE;

		UnregisterDemux(pDemuxFilter);
	}
	else
	{
		DeinitDemuxFilter(pDemuxFilter);
	}
}

static int linuxtv_dmx_enablefilter(ltv_dmx_filter_t *pDemuxFilter)
{
	ltv_dmx_t *pLtvDemux = (ltv_dmx_t*) pDemuxFilter->hDMX;
	ltv_dmx_thread_t *pDemuxThread = &pLtvDemux->tDemuxThreadList[pDemuxFilter->eStreamType];

	while (pDemuxFilter->state == FILTER_STATE_SET && pDemuxFilter->pvParent != NULL) { // wait until disable filter
		usleep(1000);
	}

	if (pDemuxFilter->state != FILTER_STATE_SET)
	{
		ALOGE("[%s] Filter State is not READY(iRequestId:%d)", __func__, pDemuxFilter->iRequestId);
		return UNKNOWN_ERROR;
	}

#ifdef PES_TIMER_CALLBACK
	pDemuxFilter->timer_expire = -1;
#endif

	RegisterDemux(pDemuxFilter, pDemuxThread);

	pDemuxFilter->fDiscontinuity = TRUE;
	pDemuxFilter->request_state = DEMUX_STATE_START;

	return NO_ERROR;
}

static int linuxtv_dmx_isenabledfilter(ltv_dmx_filter_t *pDemuxFilter)
{
	if (pDemuxFilter == NULL || pDemuxFilter->request_state != DEMUX_STATE_START)
	{
		return 0;
	}
	return 1;
}

static int linuxtv_dmx_disablefilter(ltv_dmx_filter_t *pDemuxFilter)
{
	while (pDemuxFilter->state == FILTER_STATE_GO && pDemuxFilter->pvParent == NULL) { // wait until enable filter
		usleep(1000);
	}

	if (pDemuxFilter->state != FILTER_STATE_GO)
	{
		ALOGE("[%s] Filter State is not GO(iRequestId:%d)", __func__, pDemuxFilter->iRequestId);
		return UNKNOWN_ERROR;
	}

	pDemuxFilter->fDiscontinuity = TRUE;
	pDemuxFilter->request_state = DEMUX_STATE_STOP;  // call demux stop by the thread.

	UnregisterDemux(pDemuxFilter);

	return NO_ERROR;
}

static int linuxtv_dmx_setpesfilter(ltv_dmx_filter_t *pDemuxFilter, unsigned int uiPid, dmx_pes_type_t iType)
{
	struct dmx_pes_filter_params *pesFilterParams = &pDemuxFilter->pesFilterParams;

	#ifdef DEBUG_SET_FILTER_TIME
	if (iType == DMX_PES_PCR0)
	{
		clock_gettime(CLOCK_MONOTONIC , &tspec1);
	}
	#endif

	if (pDemuxFilter->state == FILTER_STATE_GO)
	{
		linuxtv_dmx_disablefilter(pDemuxFilter);
	}

	while (pDemuxFilter->pvParent != NULL) { // wait until disable filter
		ALOGE("[%s] Wait Filter State is STOP(iRequestId:%d)", __func__, pDemuxFilter->iRequestId);
		usleep(1000);
	}

	pesFilterParams->pid = uiPid;
	pesFilterParams->pes_type = iType;
	pesFilterParams->input = DMX_IN_FRONTEND;
	pesFilterParams->output = DMX_OUT_TSDEMUX_TAP; //DMX_OUT_DECODER, DMX_OUT_TAP
	pesFilterParams->flags = DMX_IMMEDIATE_START;

	pDemuxFilter->state = FILTER_STATE_SET;

	return NO_ERROR;
}

static int linuxtv_dmx_setsectionfilter(ltv_dmx_filter_t *pDemuxFilter, unsigned int uiPid, int iRepeat, int iCheckCRC,
									char *pucValue, char *pucMask, char *pucExclusion, unsigned int ulFilterLen)
{
	struct dmx_sct_filter_params *sctFilterParams = &pDemuxFilter->sctFilterParams;
	int i;

	if (pDemuxFilter->state == FILTER_STATE_GO)
	{
		linuxtv_dmx_disablefilter(pDemuxFilter);
		while (pDemuxFilter->pvParent != NULL) { // wait until disable filter
			usleep(1000);
		}
	}
	else if (pDemuxFilter->pvParent != NULL)
	{
		ALOGE("[%s] Filter State is not STOP(iRequestId:%d)", __func__, pDemuxFilter->iRequestId);
		return UNKNOWN_ERROR;
	}

	sctFilterParams->pid = uiPid;
	sctFilterParams->timeout = 0;
	sctFilterParams->flags = DMX_IMMEDIATE_START;

	if(iCheckCRC)
		sctFilterParams->flags |= DMX_CHECK_CRC;
	if(iRepeat == 0)
		sctFilterParams->flags |= DMX_ONESHOT;

	memset(&sctFilterParams->filter, 0x0, sizeof(dmx_filter_t));

	for(i = 0 ; i <(int) ulFilterLen; i++)
	{
		sctFilterParams->filter.filter[i] = pucValue[i];
		sctFilterParams->filter.mask[i] = pucMask[i];
		sctFilterParams->filter.mode[i] = pucExclusion[i];
	}

	pDemuxFilter->state = FILTER_STATE_SET;

	return 0;
}

static int linuxtv_dmx_setstaticbuffer(ltv_dmx_filter_t *pDemuxFilter, unsigned char *pBuffer, int size)
{
	if (pDemuxFilter->state != FILTER_STATE_ALLOCATED)
	{
		ALOGE("[%s] Filter State is not ALLOCATED(iRequestId:%d)", __func__, pDemuxFilter->iRequestId);
		return UNKNOWN_ERROR;
	}

	if (pDemuxFilter->pBaseBuffer)
		TCC_fo_free (__func__, __LINE__,pDemuxFilter->pBaseBuffer);
	pDemuxFilter->bUseStaticBuffer = 1;
	pDemuxFilter->pBaseBuffer = pBuffer;
	pDemuxFilter->nBufferSize = size;

	return NO_ERROR;
}

static int linuxtv_dmx_setcallbacktimer(ltv_dmx_filter_t *pDemuxFilter, int fEnable, int time_usec)
{
	if(pDemuxFilter == NULL) return NO_HANDLER;

	pDemuxFilter->fUseCBTimer = fEnable;
	pDemuxFilter->CBTimerValue_usec = time_usec;

	return NO_ERROR;
}

static int linuxtv_dmx_getcallbacktimer(ltv_dmx_filter_t *pDemuxFilter, int *pfEnable, int *p_time_usec)
{
	if(pDemuxFilter == NULL) return NO_HANDLER;

	if( pfEnable )
	{
		*pfEnable = pDemuxFilter->fUseCBTimer;
	}
	if( p_time_usec )
	{
		*p_time_usec = pDemuxFilter->CBTimerValue_usec;
	}
	return NO_ERROR;
}

ltv_dmx_filter_t* linuxtv_dmx_getfilter(ltv_dmx_t *pLtvDemux, int iRequestId)
{
	ltv_dmx_filter_t *pDemuxFilter;
	int i;

	for(i = 0; i < MAX_FILTER_NUM; i++)
	{
		pDemuxFilter = &pLtvDemux->tDemuxFilterList[i];
		if(pDemuxFilter->state > FILTER_STATE_FREE && pDemuxFilter->iRequestId == iRequestId)
		{
			if (pDemuxFilter->nFreeCheck == 0)
				return pDemuxFilter;
		}
	}

	ALOGE("[%s] Can't find RequestId(%d)\n", __func__, iRequestId);
	return NULL;
}

static int linuxtv_dmx_getrequestid(ltv_dmx_filter_t *pDemuxFilter, int *iRequestId)
{
	*iRequestId = pDemuxFilter->iRequestId;

	return NO_ERROR;
}

static int LinuxTV_Demux_GetBuffer(ltv_dmx_filter_t *pDemuxFilter, unsigned char **ppucBuffer, unsigned int *puiBufferSize)
{
	OMX_BUFFERHEADERTYPE* pOutputBuffer;

	pthread_mutex_lock(&pDemuxFilter->tLock);

	if(pDemuxFilter->pPortBuffer)
		pOutputBuffer = pDemuxFilter->pPortBuffer;
	else {
		if(dxb_omx_dxb_GetBuffer(pDemuxFilter, &pOutputBuffer, pDemuxFilter->appData) != 0) {
			pthread_mutex_unlock(&pDemuxFilter->tLock);
			return ERROR_HANDLER;
		}

		if(pOutputBuffer == NULL){
			//dequeue failed
			pthread_mutex_unlock(&pDemuxFilter->tLock);
			return ERROR_HANDLER;
		}

		pDemuxFilter->pPortBuffer = (void *)pOutputBuffer;
		//ALOGD("%s %p:%d", __func__, pOutputBuffer->pBuffer, pOutputBuffer->nAllocLen);
	}

	*ppucBuffer = pOutputBuffer->pBuffer;
	*puiBufferSize = pOutputBuffer->nAllocLen;

	pthread_mutex_unlock(&pDemuxFilter->tLock);
	return NO_ERROR;
}

static int LinuxTV_Demux_StreamCallback(ltv_dmx_filter_t *pDemuxFilter, unsigned char *buf, int size, unsigned long long pts, int flags)
{
	if (flags == 0) // flag(0: normal play other: trick play)
	{
		flags = pDemuxFilter->isFromPVR;
	}
	if (pDemuxFilter->state == FILTER_STATE_GO)
	{
		pDemuxFilter->pushCallback(pDemuxFilter, buf, size, pts, flags, pDemuxFilter->appData);
	}
	return NO_ERROR;
}

static int linuxtv_dmx_setstreamtype(ltv_dmx_filter_t *pDemuxFilter, INT32 iType, INT32 ParserOnOff)
{
	int ret = 0;
	if (pDemuxFilter->state == FILTER_STATE_GO)
	{
		linuxtv_dmx_disablefilter(pDemuxFilter);
		while (pDemuxFilter->pvParent != NULL) { // wait until disable filter
			usleep(10);
		}
	}
	else if (pDemuxFilter->pvParent != NULL)
	{
		ALOGE("[%s] Filter State is not STOP(iRequestId:%d)", __func__, pDemuxFilter->iRequestId);
		return UNKNOWN_ERROR;
	}

	if (pDemuxFilter->pBaseBuffer) {
		TCC_fo_free (__func__, __LINE__,pDemuxFilter->pBaseBuffer);
		pDemuxFilter->pBaseBuffer = NULL;
	}

	ret = TSDEMUXSTParser_EnableStreamParser(&pDemuxFilter->pStreamParser);
	if(ret == MPEGDEMUX_ERR_OUTOFMEMORY){
		return UNKNOWN_ERROR;
	}
	else // jini 14th : Redundant Condition => if(pDemuxFilter->pStreamParser)
	{
		pDemuxFilter->pStreamParser->m_uiStreamType = iType;
		if(ParserOnOff == FALSE){
			pDemuxFilter->pStreamParser->m_Enable = 0;//disable stream parser
		}
	}

	pDemuxFilter->pStreamParser->appData = pDemuxFilter;
	pDemuxFilter->pStreamParser->pushCallback =(STREAM_CALLBACK_FUCTION) LinuxTV_Demux_StreamCallback;
	pDemuxFilter->pStreamParser->getBuffer =(GET_BUFFER_FUCTION) LinuxTV_Demux_GetBuffer;
	pDemuxFilter->pStreamParser->m_uiEsMaxSize = pDemuxFilter->nEsMaxSize;

	return 0;
}

static ltv_dmx_t* linuxtv_dmx_init(int iDevIdx, int iFlags)
{
	DEBUG_MSG("[%s] \n", __func__);

	int i;
	ltv_dmx_t *pLtvDemux = (ltv_dmx_t*) TCC_fo_malloc(__func__, __LINE__,sizeof(ltv_dmx_t));
	if(pLtvDemux == NULL){
		return pLtvDemux;
	}

	// initialize ltv_dmx_t structure
	memset(pLtvDemux->tDemuxFilterList, 0x0, sizeof(ltv_dmx_filter_t) * MAX_FILTER_NUM);
	memset(pLtvDemux->tDemuxThreadList, 0x0, sizeof(ltv_dmx_thread_t) * MAX_THREAD_NUM);

	pLtvDemux->iDevIdx = iDevIdx;

	pLtvDemux->iDummyFilter = -1;
	pLtvDemux->iDvrInput = 0;

	StartDummyDemux(pLtvDemux);

	if (iFlags & FLAGS_DVR_INPUT)
	{
		pLtvDemux->iDvrInput = 1;
	}

	for (i = 0; i < MAX_THREAD_NUM; i++)
	{
		if (iFlags & 0x1)
		{
			LinuxTV_CreateThread(pLtvDemux, i);
		}
		iFlags >>= 1;
	}
	return pLtvDemux;
}

static void linuxtv_dmx_deinit(ltv_dmx_t *pLtvDemux)
{
	DEBUG_MSG("[%s] \n", __func__);

	ltv_dmx_thread_t *pDemuxThread;
	int i;

	for (i = 0; i < MAX_THREAD_NUM; i++)
	{
		LinuxTV_DestroyThread(pLtvDemux, i);
	}

	for (i = 0; i < MAX_THREAD_NUM; i++)
	{
		pDemuxThread = &pLtvDemux->tDemuxThreadList[i];
		while(pDemuxThread->iCurrState != THREAD_STATE_STOP)
		{
			usleep(10);
		}

		// Noah, IM692A-29
		pthread_cond_destroy(&pDemuxThread->tCondition);
		pthread_mutex_destroy(&pDemuxThread->tLock);
	}
	StopDummyDemux(pLtvDemux);
	TCC_fo_free(__func__, __LINE__, pLtvDemux);
}

static int linuxtv_dmx_updatewritebuffer(ltv_dmx_filter_t *pDemuxFilter, unsigned char *pucReadPtr, unsigned char *pucWritePtr, unsigned int uiWriteSize)
{
	//DEBUG_MSG("%s", __func__);

	if(pucReadPtr) pDemuxFilter->pReadBuffer = pucReadPtr;
	if(pucWritePtr) {
		pDemuxFilter->pWriteBuffer = pucWritePtr + uiWriteSize;
		if(pDemuxFilter->nEsMaxSize > pDemuxFilter->pBaseBuffer + pDemuxFilter->nBufferSize - pDemuxFilter->pWriteBuffer) {
			pDemuxFilter->pWriteBuffer = pDemuxFilter->pBaseBuffer;
		}
	}

	return NO_ERROR;
}

static int linuxtv_dmx_flush(ltv_dmx_filter_t *pDemuxFilter)
{
	return NO_ERROR;
}

static int linuxtv_dmx_getstc(ltv_dmx_t *pLtvDemux, int index, long long *lRealSTC)
{
	int ret;
	struct dmx_stc arg;

	memset(&arg, 0x00, sizeof(struct dmx_stc));

	arg.num = index;

	ret = ioctl(pLtvDemux->iDummyFilter, DMX_GET_STC, &arg);

	*lRealSTC = arg.stc;

	return ret;
}

static int linuxtv_dmx_pause(ltv_dmx_t *pLtvDemux)
{
	ltv_dmx_thread_t *pDemuxThread;
	int i, j;

	for(i = 0; i < MAX_THREAD_NUM; i++)
	{
		pDemuxThread = &pLtvDemux->tDemuxThreadList[i];
		if(pDemuxThread->iCurrState & THREAD_STATE_RUN)
		{
			pthread_mutex_lock(&pDemuxThread->tLock);
			pDemuxThread->iDestState |= THREAD_STATE_PAUSE;
			pthread_cond_signal(&pDemuxThread->tCondition);
			pthread_mutex_unlock(&pDemuxThread->tLock);
		}
	}
	return NO_ERROR;
}

static int linuxtv_dmx_stop(ltv_dmx_t *pLtvDemux)
{
	ltv_dmx_thread_t *pDemuxThread;
	int i, j;

	for(i = 0; i < MAX_THREAD_NUM; i++)
	{
		pDemuxThread = &pLtvDemux->tDemuxThreadList[i];
		if(pDemuxThread->iCurrState & THREAD_STATE_RUN)
		{
			pthread_mutex_lock(&pDemuxThread->tLock);
			pDemuxThread->iDestState |= THREAD_STATE_SKIP;
			pthread_cond_signal(&pDemuxThread->tCondition);
			pthread_mutex_unlock(&pDemuxThread->tLock);
		}
	}
	return NO_ERROR;
}

static int linuxtv_dmx_resume(ltv_dmx_t *pLtvDemux)
{
	ltv_dmx_thread_t *pDemuxThread;
	int i;

	for(i = 0; i < MAX_THREAD_NUM; i++)
	{
		pDemuxThread = &pLtvDemux->tDemuxThreadList[i];
		if(pDemuxThread->iCurrState & THREAD_STATE_RUN)
		{
			pthread_mutex_lock(&pDemuxThread->tLock);
			pDemuxThread->iDestState &= ~(THREAD_STATE_SKIP | THREAD_STATE_PAUSE);
			pthread_cond_signal(&pDemuxThread->tCondition);
			pthread_mutex_unlock(&pDemuxThread->tLock);
		}
	}

	return NO_ERROR;
}

static int linuxtv_dmx_setpvr(ltv_dmx_t *pLtvDemux, int on)
{
	ltv_dmx_thread_t *pDemuxThread;
	int i;

	for(i = 0; i < MAX_THREAD_NUM; i++)
	{
		pDemuxThread = &pLtvDemux->tDemuxThreadList[i];
		if(pDemuxThread->iCurrState & THREAD_STATE_RUN && pDemuxThread->eStreamType != STREAM_SECTION)
		{
			if (on)
			{
				if (pDemuxThread->iDestState & THREAD_STATE_PVR)
					return NO_ERROR;

				while (pDemuxThread->iCurrState & THREAD_STATE_PVR)
					usleep(10);

				pthread_mutex_lock(&pDemuxThread->tLock);
				pDemuxThread->iDestState |= THREAD_STATE_PVR;
				pthread_cond_signal(&pDemuxThread->tCondition);
				pthread_mutex_unlock(&pDemuxThread->tLock);
			}
			else
			{
				if ((pDemuxThread->iDestState & THREAD_STATE_PVR) == 0)
					return NO_ERROR;

				while ((pDemuxThread->iCurrState & THREAD_STATE_PVR) == 0)
					usleep(10);

				pthread_mutex_lock(&pDemuxThread->tLock);
				pDemuxThread->iDestState &= ~THREAD_STATE_PVR;
				pthread_cond_signal(&pDemuxThread->tCondition);
				pthread_mutex_unlock(&pDemuxThread->tLock);
			}
		}
	}
	return NO_ERROR;
}

static int linuxtv_dmx_inputbuffer(ltv_dmx_t *pLtvDemux, unsigned char *pucBuff, int iSize)
{
	ltv_dmx_thread_t *pDemuxThread;
	ltv_dmx_filter_t *pDemuxFilter;
	unsigned char *p;
	int nPID, i, j, curr, next;

	for (i = 0; i < iSize; i += PACKETSIZE)
	{
		p = pucBuff + i;

		nPID = ((int)(p[1] & 0x1F)) << 8;
		nPID |= ((int)p[2]);

		for (j = 0; j < MAX_FILTER_NUM; j++)
		{
			pDemuxFilter = &pLtvDemux->tDemuxFilterList[j];
			if (pDemuxFilter->state == FILTER_STATE_GO && pDemuxFilter->eFilterType == PES_FILTER && pDemuxFilter->pTsParser->nPID == nPID)
			{
				pDemuxThread = (ltv_dmx_thread_t*) pDemuxFilter->pvParent;
				if (pDemuxThread == NULL || (pDemuxThread->iCurrState & THREAD_STATE_PVR) == 0)
					continue;

				if (pDemuxFilter->tInputBuffer.size > 0)
				{
					curr = pDemuxFilter->tInputBuffer.push;
					next = (curr + PACKETSIZE) % pDemuxFilter->tInputBuffer.size;
					if (next != pDemuxFilter->tInputBuffer.pop)
					{
						memcpy(pDemuxFilter->tInputBuffer.pBuffer + curr, p, PACKETSIZE);
						pDemuxFilter->tInputBuffer.push = next;

						pthread_mutex_lock(&pDemuxThread->tLock);
						if (pDemuxThread->iCurrState & THREAD_STATE_PVR)
						{
							pDemuxThread->iUpdatedInputBuffer = 1;
							pthread_cond_signal(&pDemuxThread->tCondition);
						}
						pthread_mutex_unlock(&pDemuxThread->tLock);
					}
					else
					{
						ALOGE("[%s] Buffer is full(RequestID = %d)", __func__, pDemuxFilter->iRequestId);
					}
				}
				break;
			}
		}
	}
	return iSize;
}

static const UINT32 crc32_table[256] = {
	0x00000000, 0xb71dc104, 0x6e3b8209, 0xd926430d,
	0xdc760413, 0x6b6bc517, 0xb24d861a, 0x0550471e,
	0xb8ed0826, 0x0ff0c922, 0xd6d68a2f, 0x61cb4b2b,
	0x649b0c35, 0xd386cd31, 0x0aa08e3c, 0xbdbd4f38,
	0x70db114c, 0xc7c6d048, 0x1ee09345, 0xa9fd5241,
	0xacad155f, 0x1bb0d45b, 0xc2969756, 0x758b5652,
	0xc836196a, 0x7f2bd86e, 0xa60d9b63, 0x11105a67,
	0x14401d79, 0xa35ddc7d, 0x7a7b9f70, 0xcd665e74,
	0xe0b62398, 0x57abe29c, 0x8e8da191, 0x39906095,
	0x3cc0278b, 0x8bdde68f, 0x52fba582, 0xe5e66486,
	0x585b2bbe, 0xef46eaba, 0x3660a9b7, 0x817d68b3,
	0x842d2fad, 0x3330eea9, 0xea16ada4, 0x5d0b6ca0,
	0x906d32d4, 0x2770f3d0, 0xfe56b0dd, 0x494b71d9,
	0x4c1b36c7, 0xfb06f7c3, 0x2220b4ce, 0x953d75ca,
	0x28803af2, 0x9f9dfbf6, 0x46bbb8fb, 0xf1a679ff,
	0xf4f63ee1, 0x43ebffe5, 0x9acdbce8, 0x2dd07dec,
	0x77708634, 0xc06d4730, 0x194b043d, 0xae56c539,
	0xab068227, 0x1c1b4323, 0xc53d002e, 0x7220c12a,
	0xcf9d8e12, 0x78804f16, 0xa1a60c1b, 0x16bbcd1f,
	0x13eb8a01, 0xa4f64b05, 0x7dd00808, 0xcacdc90c,
	0x07ab9778, 0xb0b6567c, 0x69901571, 0xde8dd475,
	0xdbdd936b, 0x6cc0526f, 0xb5e61162, 0x02fbd066,
	0xbf469f5e, 0x085b5e5a, 0xd17d1d57, 0x6660dc53,
	0x63309b4d, 0xd42d5a49, 0x0d0b1944, 0xba16d840,
	0x97c6a5ac, 0x20db64a8, 0xf9fd27a5, 0x4ee0e6a1,
	0x4bb0a1bf, 0xfcad60bb, 0x258b23b6, 0x9296e2b2,
	0x2f2bad8a, 0x98366c8e, 0x41102f83, 0xf60dee87,
	0xf35da999, 0x4440689d, 0x9d662b90, 0x2a7bea94,
	0xe71db4e0, 0x500075e4, 0x892636e9, 0x3e3bf7ed,
	0x3b6bb0f3, 0x8c7671f7, 0x555032fa, 0xe24df3fe,
	0x5ff0bcc6, 0xe8ed7dc2, 0x31cb3ecf, 0x86d6ffcb,
	0x8386b8d5, 0x349b79d1, 0xedbd3adc, 0x5aa0fbd8,
	0xeee00c69, 0x59fdcd6d, 0x80db8e60, 0x37c64f64,
	0x3296087a, 0x858bc97e, 0x5cad8a73, 0xebb04b77,
	0x560d044f, 0xe110c54b, 0x38368646, 0x8f2b4742,
	0x8a7b005c, 0x3d66c158, 0xe4408255, 0x535d4351,
	0x9e3b1d25, 0x2926dc21, 0xf0009f2c, 0x471d5e28,
	0x424d1936, 0xf550d832, 0x2c769b3f, 0x9b6b5a3b,
	0x26d61503, 0x91cbd407, 0x48ed970a, 0xfff0560e,
	0xfaa01110, 0x4dbdd014, 0x949b9319, 0x2386521d,
	0x0e562ff1, 0xb94beef5, 0x606dadf8, 0xd7706cfc,
	0xd2202be2, 0x653deae6, 0xbc1ba9eb, 0x0b0668ef,
	0xb6bb27d7, 0x01a6e6d3, 0xd880a5de, 0x6f9d64da,
	0x6acd23c4, 0xddd0e2c0, 0x04f6a1cd, 0xb3eb60c9,
	0x7e8d3ebd, 0xc990ffb9, 0x10b6bcb4, 0xa7ab7db0,
	0xa2fb3aae, 0x15e6fbaa, 0xccc0b8a7, 0x7bdd79a3,
	0xc660369b, 0x717df79f, 0xa85bb492, 0x1f467596,
	0x1a163288, 0xad0bf38c, 0x742db081, 0xc3307185,
	0x99908a5d, 0x2e8d4b59, 0xf7ab0854, 0x40b6c950,
	0x45e68e4e, 0xf2fb4f4a, 0x2bdd0c47, 0x9cc0cd43,
	0x217d827b, 0x9660437f, 0x4f460072, 0xf85bc176,
	0xfd0b8668, 0x4a16476c, 0x93300461, 0x242dc565,
	0xe94b9b11, 0x5e565a15, 0x87701918, 0x306dd81c,
	0x353d9f02, 0x82205e06, 0x5b061d0b, 0xec1bdc0f,
	0x51a69337, 0xe6bb5233, 0x3f9d113e, 0x8880d03a,
	0x8dd09724, 0x3acd5620, 0xe3eb152d, 0x54f6d429,
	0x7926a9c5, 0xce3b68c1, 0x171d2bcc, 0xa000eac8,
	0xa550add6, 0x124d6cd2, 0xcb6b2fdf, 0x7c76eedb,
	0xc1cba1e3, 0x76d660e7, 0xaff023ea, 0x18ede2ee,
	0x1dbda5f0, 0xaaa064f4, 0x738627f9, 0xc49be6fd,
	0x09fdb889, 0xbee0798d, 0x67c63a80, 0xd0dbfb84,
	0xd58bbc9a, 0x62967d9e, 0xbbb03e93, 0x0cadff97,
	0xb110b0af, 0x060d71ab, 0xdf2b32a6, 0x6836f3a2,
	0x6d66b4bc, 0xda7b75b8, 0x035d36b5, 0xb440f7b1
};

static unsigned char *skip_pes_header(unsigned char **bufp, unsigned int *len)
{
	unsigned char *inbuf = *bufp;
	unsigned char *buf = inbuf;
	unsigned char *pts = NULL;
	int skip = 0;

	static const int mpeg1_skip_table[16] = {
		1, 0xffff,      5,     10, 0xffff, 0xffff, 0xffff, 0xffff,
		0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff
	};

	if(1) { //(inbuf[6] & 0xc0) == 0x80){ /* mpeg2 */
		if(buf[7] & 0x80)
			pts = buf + 9;
		else pts = NULL;
		buf = inbuf + 9 + inbuf[8];
	} else {        /* mpeg1 */
		for(buf = inbuf + 6; *buf == 0xff; buf++)
			if(buf == inbuf + 6 + 16) {
				break;
			}
		if((*buf & 0xc0) == 0x40)
			buf += 2;
		skip = mpeg1_skip_table [*buf >> 4];
		if(skip == 5 || skip == 10) pts = buf;
		else pts = NULL;

		buf += mpeg1_skip_table [*buf >> 4];
	}
	*len -= (buf - *bufp);
	*bufp = buf;
	return pts;
}

static int pts_conversion(unsigned char *p, int len, unsigned long long offset, unsigned long long curr)
{
	unsigned long long org_pts;
	unsigned char *pts;

	if(p[0] != 0x00 || p[1] != 0x00 || p[2] != 0x01)
		return NO_ERROR;

	pts = skip_pes_header(&p,(unsigned int *) &len);
	org_pts = (unsigned long long)(pts[0] & 0x0e) << 29;
	org_pts |= pts[1] << 22;
	org_pts |= (pts[2] & 0xfe) << 14;
	org_pts |= pts[3] << 7;
	org_pts |= ((pts[4] & 0xfe) >> 1);

	org_pts = (org_pts - offset) + curr;

	pts[0] &= 0xF0;
	pts[0] |= ((org_pts >> 29) & 0x0e);
	pts[1] = (org_pts >> 22) & 0xff;
	pts[2] = (org_pts >> 14) & 0xfe;
	pts[3] = (org_pts >> 7) & 0xff;
	pts[4] = (org_pts << 1) & 0xfe;
	return NO_ERROR;
}

static int pcr_conversion(ltv_dmx_record_t *pDemuxRecord, unsigned char *p)
{
	long long pcr;

	pcr = (long long)p[6] << 25;
	pcr |= (p[7] << 17);
	pcr |= (p[8] << 9);
	pcr |= (p[9] << 1);
	pcr |= ((p[10] >> 7) & 0x01);

	if(pDemuxRecord->bFirst) {
		pDemuxRecord->bFirst = 0;
		pDemuxRecord->iCurrPTS = 0;
		pDemuxRecord->iOffsetPTS = pcr;
	} else if(pDemuxRecord->iPrevPTS > pcr + 10000) {
		pDemuxRecord->iCurrPTS += (pDemuxRecord->iPrevPTS - pDemuxRecord->iOffsetPTS + 30);
		pDemuxRecord->iOffsetPTS = pcr;
	}
	pDemuxRecord->iPrevPTS = pcr;
	pcr = (pcr - pDemuxRecord->iOffsetPTS) + pDemuxRecord->iCurrPTS;

	p[10] &= 0x7f;
	p[10] |= (pcr << 7) & 0x80;
	p[9] = (pcr >> 1) & 0xff;
	p[8] = (pcr >> 9) & 0xff;
	p[7] = (pcr >> 17) & 0xff;
	p[6] = (pcr >> 25) & 0xff;

	return NO_ERROR;
}

static int parseTSforDVR(ltv_dmx_record_t *pDemuxRecord, unsigned char *p, int len)
{
	int ret = NO_ERROR;

	int pid = ((p[1] & 0x1F) << 8) | p[2];
	int header_size = 4;

	if(p[1] & TRANS_ERROR) return NO_ERROR;

	if(p[3] & ADAPT_FIELD) {
		if((pid == pDemuxRecord->iPCRPID) && (p[5] & 0x10)) {
			pcr_conversion(pDemuxRecord, p);
		}
		header_size += p[4] + 1;
		if(len < header_size) return NO_ERROR;
	}

	if(pDemuxRecord->bFirst) return UNKNOWN_ERROR;

	if((p[3] & PAYLOAD) && (p[1] & PAY_START)) {
		p += header_size;
		len -= header_size;
		if(pid == pDemuxRecord->iVideoPID || pid == pDemuxRecord->iAudioPID) {
			ret = pts_conversion(p, len, pDemuxRecord->iOffsetPTS, pDemuxRecord->iCurrPTS);
		}
	}

	return ret;
}

static void MakeCRC32(unsigned char *p, unsigned int n_bytes)
{
	unsigned int crc = 0xffffffff;
	while(n_bytes--) {
		crc = crc32_table[(crc ^ *p++) & 0xff] ^ (crc >> 8);
	}

	*p++ = crc & 0xFF;
	*p++ = (crc >> 8) & 0xFF;
	*p++ = (crc >> 16) & 0xFF;
	*p++ = (crc >> 24) & 0xFF;

	return;
}

static int MakePATPMT(unsigned char *packet, int v_pid, int v_type, int a_pid, int a_type, int pcr_pid)
{
	memset(packet, 0xff, 188*2);

	packet[0] = 0x47; //sync byte
	packet[1] = 0x40; //low bit : pid
	packet[2] = 0x00; //pid = 0x000
	packet[3] = 0x10; //transport scrabling control, adaptation field control, continuity counter
	packet[4] = 0x00; //??
	packet[5] = 0x00; //table id
	packet[6] = 0xB0; //low 4 bit : section length
	packet[7] = 0x0D; //section length
	packet[8] = 0x02; //ts id
	packet[9] = 0x75; //ts id
	packet[10] = 0xC3; //version number, current next indicator
	packet[11] = 0x00; //section number
	packet[12] = 0x00; //last section number
	packet[13] = 0x00; //program number
	packet[14] = 0x01; //program number
	packet[15] = 0xE1; //low 5 bit : pmt id
	packet[16] = 0x00; //pmt id = 0x100

	MakeCRC32(&packet[5], 12);

	packet += 188;

	packet[0] = 0x47; //sync byte
	packet[1] = 0x41; //low bit : pid
	packet[2] = 0x00; //pid = 0x100
	packet[3] = 0x10; //transport scrabling control, adaptation field control, continuity counter
	packet[4] = 0x00; //??
	packet[5] = 0x02; //table id
	packet[6] = 0xB0; //low bit : section length
	packet[7] = 0x17; //section length
	packet[8] = 0x00; //program number
	packet[9] = 0x01; //program number
	packet[10] = 0xC3; //version number, current next indicator,
	packet[11] = 0x00; //section number
	packet[12] = 0x00; //last section number
	packet[13] = 0xE0 | ((pcr_pid >> 8) & 0x1F); // prc pid
	packet[14] = pcr_pid & 0xFF;
	packet[15] = 0xE0; //low bit : program length
	packet[16] = 0x00; //program length
	packet[17] = v_type; //stream type
	packet[18] = 0xE0 | ((v_pid >> 8) & 0x1F);	 // video pid
	packet[19] = v_pid & 0xFF;
	packet[20] = 0xF0; //low bit : es info length
	packet[21] = 0x00; //es info length
	packet[22] = a_type; //stream type
	packet[23] = 0xE0 | ((a_pid >> 8) & 0x1F); // audio pid
	packet[24] = a_pid & 0xFF;
	packet[25] = 0xF0; //low bit : es info length
	packet[26] = 0x00; //es info length

	MakeCRC32(&packet[5], 22);

	return 188*2;
}

static int LinuxTV_Demux_RecInit(ltv_dmx_record_t *pDemuxRecord)
{
	struct dmx_pes_filter_params pesFilterParams;
	pesFilterParams.input = DMX_IN_FRONTEND;
	pesFilterParams.output = DMX_OUT_TSDEMUX_TAP; //DMX_OUT_TS_TAP, DMX_OUT_TSDEMUX_TAP
	pesFilterParams.flags = 0; //DMX_IMMEDIATE_START
	pesFilterParams.pes_type = DMX_PES_OTHER;
	pesFilterParams.pid = pDemuxRecord->iPCRPID;

	pDemuxRecord->iDevDemux = open(DMX_1, O_RDONLY | O_NONBLOCK);

	if (ioctl(pDemuxRecord->iDevDemux, DMX_SET_PES_FILTER, &pesFilterParams))
		return 1;

	if (pDemuxRecord->iPCRPID != pDemuxRecord->iAudioPID
		&& ioctl(pDemuxRecord->iDevDemux, DMX_ADD_PID, &pDemuxRecord->iAudioPID))
		return 1;

	if (pDemuxRecord->iPCRPID != pDemuxRecord->iVideoPID
		&& ioctl(pDemuxRecord->iDevDemux, DMX_ADD_PID, &pDemuxRecord->iVideoPID))
		return 1;

	if (ioctl(pDemuxRecord->iDevDemux, DMX_SET_BUFFER_SIZE, DVR_BUFFER_SIZE))
		return 1;

	if (ioctl(pDemuxRecord->iDevDemux, DMX_START, 0))
		return 1;

	return 0;
}

static void *LinuxTV_Demux_Record_Thread(void* param)
{
	DEBUG_MSG("%s[START]", __func__);

	ltv_dmx_record_t *pDemuxRecord = param;
	struct pollfd pfd;
	int len;
	int written;
	char dvr_buf[DVR_READ_SIZE];
	unsigned char *p;

	if (LinuxTV_Demux_RecInit(pDemuxRecord))
	{
		pDemuxRecord->iRequestStop = 1;
	}
	else
	{
		len = MakePATPMT(dvr_buf, pDemuxRecord->iVideoPID, pDemuxRecord->iVideoType, pDemuxRecord->iAudioPID, pDemuxRecord->iAudioType, pDemuxRecord->iPCRPID);
		write(pDemuxRecord->iDevFile, dvr_buf, len);

		pfd.fd = pDemuxRecord->iDevDemux;
		pfd.events = POLLIN;
	}

	while(pDemuxRecord->iRequestStop == 0) {
		/* poll for dvr data and write to file */
		if(poll(&pfd, 1, 100)) {
			if(pfd.revents & POLLIN) {
				len = read(pDemuxRecord->iDevDemux, dvr_buf, DVR_READ_SIZE);
				if(len < 0) {
					ALOGE("recording");
				}
				if(len > 0) {
#if (1) // PTS_CONVERSION
					p = dvr_buf;
					written = 0;
					while(written < len) {
						if(parseTSforDVR(pDemuxRecord, p, PACKETSIZE) == NO_ERROR) {
							written += write(pDemuxRecord->iDevFile, p, PACKETSIZE);
						} else {
							written += PACKETSIZE;
						}
						p += PACKETSIZE;
					}
#else
					written = write(pDemuxRecord->iDevFile, dvr_buf, len);
#endif
				}
			}
		}
	}
	close(pDemuxRecord->iDevFile);
	close(pDemuxRecord->iDevDemux);
	sync();

	pDemuxRecord->state = THREAD_STATE_STOP;

	DEBUG_MSG("%s[STOP]", __func__);
	return (void*)NULL;
}

static int linuxtv_dmx_record_start(ltv_dmx_t *pLtvDemux, const char *pucFileName, int v_pid, int v_type, int a_pid, int a_type, int pcr_pid)
{
	DEBUG_MSG("[%s] filename:%s\n", __func__, pucFileName);

	ltv_dmx_record_t *pDemuxRecord = &pLtvDemux->tDemuxRecord;

	if (pDemuxRecord->state != THREAD_STATE_STOP)
		return ERROR_THREAD;

	/* open output file */
	if((pDemuxRecord->iDevFile = open(pucFileName, O_WRONLY | O_CREAT
										| O_TRUNC, S_IRUSR | S_IWUSR
										| S_IRGRP | S_IWGRP | S_IROTH |
										S_IWOTH)) < 0) {
		ALOGE("Can't open file for dvr");
		return ERROR_HANDLER;
	}

	pDemuxRecord->iVideoPID = v_pid;
	pDemuxRecord->iVideoType = v_type;
	pDemuxRecord->iAudioPID = a_pid;
	pDemuxRecord->iAudioType = a_type;
	pDemuxRecord->iPCRPID = pcr_pid;
	pDemuxRecord->bFirst = 1;
	pDemuxRecord->iRequestStop = 0;

	pDemuxRecord->state = THREAD_STATE_RUN;

	if(TCCTHREAD_Create(&pDemuxRecord->tThread, (pThreadFunc)LinuxTV_Demux_Record_Thread, "LinuxTV_Demux_Record_Thread", HIGH_PRIORITY_7, pDemuxRecord)) {
		close(pDemuxRecord->iDevFile);
		pDemuxRecord->state = THREAD_STATE_STOP;
		return ERROR_THREAD;
	}

	return NO_ERROR;
}

static void linuxtv_dmx_record_stop(ltv_dmx_t *pLtvDemux)
{
	DEBUG_MSG("[%s] \n", __func__);

	ltv_dmx_record_t *pDemuxRecord = &pLtvDemux->tDemuxRecord;

	pDemuxRecord->iRequestStop = 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HandleDemuxFilter LinuxTV_Demux_CreateFilter(HandleDemux hDMX, STREAM_TYPE eStreamType, int iRequestId, void *arg, FILTERPUSHCALLBACK callback)
{
	if (hDMX)
	{
		return (HandleDemuxFilter) linuxtv_dmx_createfilter((ltv_dmx_t*) hDMX, eStreamType, iRequestId, arg, callback);
	}
	return NULL;
}

void LinuxTV_Demux_DestroyFilter(HandleDemuxFilter hDemuxFilter)
{
	if (hDemuxFilter)
	{
		linuxtv_dmx_destroyfilter((ltv_dmx_filter_t*) hDemuxFilter);
	}
}

int LinuxTV_Demux_EnableFilter(HandleDemuxFilter hDemuxFilter)
{
	if (hDemuxFilter)
	{
		return linuxtv_dmx_enablefilter((ltv_dmx_filter_t*) hDemuxFilter);
	}
	return NO_HANDLER;
}

int LinuxTV_Demux_IsEnabledFilter(HandleDemuxFilter hDemuxFilter)
{
	if (hDemuxFilter)
	{
		return linuxtv_dmx_isenabledfilter((ltv_dmx_filter_t*) hDemuxFilter);
	}
	return NO_HANDLER;
}

int LinuxTV_Demux_DisableFilter(HandleDemuxFilter hDemuxFilter)
{
	if (hDemuxFilter)
	{
		return linuxtv_dmx_disablefilter((ltv_dmx_filter_t*) hDemuxFilter);
	}
	return NO_HANDLER;
}

int LinuxTV_Demux_SetPESFilter(HandleDemuxFilter hDemuxFilter, unsigned int uiPid, dmx_pes_type_t iType)
{
	if (hDemuxFilter)
	{
		return linuxtv_dmx_setpesfilter((ltv_dmx_filter_t*) hDemuxFilter, uiPid, iType);
	}
	return NO_HANDLER;
}

int LinuxTV_Demux_SetSectionFilter(HandleDemuxFilter hDemuxFilter, unsigned int uiPid, int iRepeat, int iCheckCRC,
                             char *pucValue, char *pucMask, char *pucExclusion, unsigned int ulFilterLen)
{
	if (hDemuxFilter)
	{
		return linuxtv_dmx_setsectionfilter((ltv_dmx_filter_t*) hDemuxFilter, uiPid, iRepeat, iCheckCRC, pucValue, pucMask, pucExclusion, ulFilterLen);
	}
	return NO_HANDLER;
}

int LinuxTV_Demux_SetStaticBuffer(HandleDemuxFilter hDemuxFilter, unsigned char *pBuffer, int size)
{
	if (hDemuxFilter)
	{
		return linuxtv_dmx_setstaticbuffer((ltv_dmx_filter_t*) hDemuxFilter, pBuffer, size);
	}
	return NO_HANDLER;
}

#ifdef PES_TIMER_CALLBACK
int LinuxTV_Demux_SetCallBackTimer(HandleDemuxFilter hDemuxFilter, int fEnable, int time_usec)
{
	if (hDemuxFilter)
	{
		return linuxtv_dmx_setcallbacktimer((ltv_dmx_filter_t*) hDemuxFilter, fEnable, time_usec);
	}
	return NO_HANDLER;
}

int LinuxTV_Demux_GetCallBackTimer(HandleDemuxFilter hDemuxFilter, int *pfEnable, int *p_time_usec)
{
	if (hDemuxFilter)
	{
		return linuxtv_dmx_getcallbacktimer((ltv_dmx_filter_t*) hDemuxFilter, pfEnable, p_time_usec);
	}
	return NO_HANDLER;
}
#endif

int LinuxTV_Demux_GetID(HandleDemuxFilter hDemuxFilter, int *iRequestId)
{
	if (hDemuxFilter)
	{
		return linuxtv_dmx_getrequestid((ltv_dmx_filter_t*) hDemuxFilter, iRequestId);
	}
	return NO_HANDLER;
}

int LinuxTV_Demux_SetStreamType(HandleDemuxFilter hDemuxFilter, INT32 iType, INT32 ParserOnOff)
{
	if (hDemuxFilter)
	{
		return linuxtv_dmx_setstreamtype((ltv_dmx_filter_t*) hDemuxFilter, iType, ParserOnOff);
	}
	return NO_HANDLER;
}

int LinuxTV_Demux_UpdateWriteBuffer(HandleDemuxFilter hDemuxFilter, unsigned char *pucReadPtr, unsigned char *pucWritePtr, unsigned int uiWriteSize)
{
	if (hDemuxFilter)
	{
		return linuxtv_dmx_updatewritebuffer((ltv_dmx_filter_t*) hDemuxFilter, pucReadPtr, pucWritePtr, uiWriteSize);
	}
	return NO_HANDLER;
}

int LinuxTV_Demux_Flush(HandleDemuxFilter hDemuxFilter)
{
	if (hDemuxFilter)
	{
		return linuxtv_dmx_flush((ltv_dmx_filter_t*) hDemuxFilter);
	}
	return NO_HANDLER;
}

HandleDemux LinuxTV_Demux_Init(int iDevIdx, int iFlags)
{
	return (HandleDemux) linuxtv_dmx_init(iDevIdx, iFlags);
}

void LinuxTV_Demux_DeInit(HandleDemux hDMX)
{
	if (hDMX)
	{
		linuxtv_dmx_deinit((ltv_dmx_t*)hDMX);
	}
}

HandleDemuxFilter LinuxTV_Demux_GetFilter(HandleDemux hDMX, int iRequestId)
{
	if (hDMX)
	{
		return (HandleDemuxFilter) linuxtv_dmx_getfilter((ltv_dmx_t*)hDMX, iRequestId);
	}
	return NULL;
}

int LinuxTV_Demux_GetSTC(HandleDemux hDMX, int index, long long *lRealSTC)
{
	if (hDMX)
	{
		return linuxtv_dmx_getstc((ltv_dmx_t*)hDMX, index, lRealSTC);
	}
	return NO_HANDLER;
}

int LinuxTV_Demux_Pause(HandleDemux hDMX)
{
	if (hDMX)
	{
		return linuxtv_dmx_pause((ltv_dmx_t*)hDMX);
	}
	return NO_HANDLER;
}

int LinuxTV_Demux_Stop(HandleDemux hDMX)
{
	if (hDMX)
	{
		return linuxtv_dmx_stop((ltv_dmx_t*)hDMX);
	}
	return NO_HANDLER;
}

int LinuxTV_Demux_Resume(HandleDemux hDMX)
{
	if (hDMX)
	{
		return linuxtv_dmx_resume((ltv_dmx_t*)hDMX);
	}
	return NO_HANDLER;
}

int LinuxTV_Demux_SetPVR(HandleDemux hDMX, int on)
{
	if (hDMX)
	{
		return linuxtv_dmx_setpvr((ltv_dmx_t*)hDMX, on);
	}
	return NO_HANDLER;
}

int LinuxTV_Demux_InputBuffer(HandleDemux hDMX, unsigned char *pucBuff, int iSize)
{
	if (hDMX)
	{
		return linuxtv_dmx_inputbuffer((ltv_dmx_t*)hDMX, pucBuff, iSize);
	}
	return NO_HANDLER;
}

int LinuxTV_Demux_Record_Start(HandleDemux hDMX, const char *pucFileName, int v_pid, int v_type, int a_pid, int a_type, int pcr_pid)
{
	if (hDMX)
	{
		return linuxtv_dmx_record_start((ltv_dmx_t*)hDMX, pucFileName, v_pid, v_type, a_pid, a_type, pcr_pid);
	}
	return NO_HANDLER;
}

void LinuxTV_Demux_Record_Stop(HandleDemux hDMX)
{
	if (hDMX)
	{
		linuxtv_dmx_record_stop((ltv_dmx_t*)hDMX);
	}
}

