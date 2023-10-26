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
#define LOG_TAG "TCCAudioRender"
#include <utils/Log.h>

#include <pthread.h>

#include "tcc_audio_render.h"

#if defined(HAVE_LINUX_PLATFORM)
#include <alsa/asoundlib.h>
#include <alsa_sound_system.h>
#include "TCCMemory.h"
#endif

/****************************************************************************
 * Class    - TCCAlsa
 * Platform - Linux
 ****************************************************************************/
#if defined(HAVE_LINUX_PLATFORM)
#define USE_ALSA_WRITE_THREAD
#ifdef USE_ALSA_WRITE_THREAD
#define AUDIO_PCM_QUEUESIZE 1000
#include <tcc_dxb_queue.h>
#include <tcc_dxb_thread.h>
typedef struct writeQueue_ele {
	char *data;
	int size;
	long long llPTS;
	int dev_id;
}writeQueue_ele;
#endif
class TCCAlsa : public TCCAudioOut
{
public:
	TCCAlsa();
	virtual ~TCCAlsa();

	virtual int set(int iSampleRate, int iChannels, int iBitPerSample, int iEndian, int iFrameSize, int iOutputMode, void *pParam, void *DemuxApp);
	virtual void close();
	virtual void stop();
	virtual int write(unsigned char *data, int data_size, long long llPTS, int dev_id);
	virtual int setVolume(float left, float right);
	virtual int latency();
	virtual int writable(int data_size);

private:
	ALSA_SOUND_PRIVATE	*mOut;
#ifdef USE_ALSA_WRITE_THREAD
	tcc_dxb_queue_t *getWriteQueue() {return pWriteQueue;}
	ALSA_SOUND_PRIVATE *getOut() {return mOut;}
	tcc_dxb_queue_t		*pWriteQueue;
	pthread_t			stWriteThread;
	int					getWriteThreadState() {return iWriteThreadState;}
	int					iWriteThreadState;
	static void* writeThreadFunc(void *param);
	static void flushAlsaqueue(void *queue);
#endif
};

int TCCAlsa::write(unsigned char *data, int data_size, long long llPTS, int dev_id)
{
#ifdef USE_ALSA_WRITE_THREAD
	int i;
	int elem_num;
	if(pWriteQueue != NULL) {
		writeQueue_ele *ele = (writeQueue_ele *)TCC_fo_calloc(__func__,__LINE__,1, sizeof(writeQueue_ele));
		if(ele == NULL){
			return -1;
		}
		ele->size = data_size;
		ele->data = (char *)TCC_fo_calloc(__func__,__LINE__,1, ele->size);
		if(ele->data == NULL){
//			if(ele){    Noah, To avoid CodeSonar warning, Redundant Condition
				TCC_fo_free (__func__, __LINE__,ele);
				ele = NULL;
//			}
			return -1;
		}
		ele->llPTS = llPTS;
		ele->dev_id = dev_id;
		memcpy(ele->data, data, ele->size);
		if(tcc_dxb_queue_ex(pWriteQueue, (void *)ele) == 0){
			//queue full -> queue all flush!
			flushAlsaqueue(pWriteQueue);
			if(ele) {
				if(ele->data) {
					TCC_fo_free (__func__, __LINE__,ele->data);
				}
				TCC_fo_free (__func__, __LINE__,ele);
			}
			ele = NULL;
			return -1;
		}
		return 0;
	}
	return -1;
#else
	//data_size = data_size - mOut->dropDataForAVsync;
	TCCALSA_SoundWrite(mOut, data, data_size, llPTS, dev_id);
	return data_size;
#endif
}

#ifdef USE_ALSA_WRITE_THREAD
void TCCAlsa::flushAlsaqueue(void *queue)
{
	int i;
	int elem_num;
	tcc_dxb_queue_t *mQueue = (tcc_dxb_queue_t *)queue;
	writeQueue_ele *mEle;

	ALOGI("%s\n", __func__);

	if(mQueue != NULL){
		while(tcc_dxb_getquenelem(mQueue) > 0) {
			mEle = (writeQueue_ele *)tcc_dxb_dequeue(mQueue);
			if(mEle) {
				if(mEle->data) {
					TCC_fo_free (__func__, __LINE__,mEle->data);
				}
				TCC_fo_free (__func__, __LINE__,mEle);
			}
			mEle = NULL;
		}
	}
}

void* TCCAlsa::writeThreadFunc(void *param)
{
	int ret;
	TCCAlsa *mAlsa = (TCCAlsa *)param;
	tcc_dxb_queue_t *mQueue = mAlsa->getWriteQueue();
	writeQueue_ele *ele;
	while(mAlsa->getWriteThreadState() == 1) {
		if(tcc_dxb_getquenelem(mQueue) > 0) {
			ele = (writeQueue_ele *)tcc_dxb_dequeue(mQueue);
			if(ele != NULL && ele->data != NULL && ele->size > 0) {
				ret = TCCALSA_SoundWrite(mAlsa->getOut(), (unsigned char *)ele->data, ele->size, ele->llPTS, ele->dev_id);
				if(ret < 0){
					flushAlsaqueue(mQueue);
				}
			}
			if(ele) {
				if(ele->data) {
					TCC_fo_free (__func__, __LINE__,ele->data);
				}
				TCC_fo_free (__func__, __LINE__,ele);
			}
			ele = NULL;
		}
		else {
			usleep(1000);
		}
	}

	while(1) {
		if(tcc_dxb_getquenelem(mQueue) > 0) {
			ele = (writeQueue_ele *)tcc_dxb_dequeue(mQueue);
			if(ele) {
				if(ele->data) {
					TCC_fo_free (__func__, __LINE__,ele->data);
				}
				TCC_fo_free (__func__, __LINE__,ele);
				ele = NULL;
			}
		}
		else {
			break;
		}
	}	

	mAlsa->iWriteThreadState = -1;

	return (void*)NULL;
}
#endif

TCCAlsa::TCCAlsa()
{
	int status;

	mOut = (ALSA_SOUND_PRIVATE*)TCC_fo_calloc (__func__, __LINE__,1, sizeof(ALSA_SOUND_PRIVATE));
	if(mOut == NULL){
		return;
	}

	pthread_mutex_init(&mOut->gsAlsa_snd_pcm_lock, NULL);

	mOut->fullthreshold = 0;
	mOut->output_delay_ms = 0;

#if 0
	mOut->playback_status = (snd_pcm_status_t *)TCC_fo_malloc(__func__, __LINE__,snd_pcm_status_sizeof());
#else
	mOut->playback_status = (snd_pcm_status_t *)TCC_fo_calloc(__func__, __LINE__, 1, snd_pcm_status_sizeof());
#endif

	mOut->alsa_reopen_request = 1;

	#if 0
	mInitError = set(48000, 2, 16, 1, 0, 0);
	#else
	mInitError = -1;
	#endif

#ifdef USE_ALSA_WRITE_THREAD
	pWriteQueue = (tcc_dxb_queue_t *)TCC_fo_calloc (__func__, __LINE__,1, sizeof(tcc_dxb_queue_t));
	tcc_dxb_queue_init_ex(pWriteQueue, AUDIO_PCM_QUEUESIZE);
	iWriteThreadState = 1;
	status = tcc_dxb_thread_create((void *)&stWriteThread, writeThreadFunc, (unsigned char *)"ALSA_WRITE_THREAD", LOW_PRIORITY_11, (void *)this);
	if(status)	{
		ALOGE("[%s] Can't make alsa write thread\n", __func__);
		if(pWriteQueue != NULL){
			tcc_dxb_queue_deinit(pWriteQueue);
			TCC_fo_free (__func__, __LINE__,pWriteQueue);
			pWriteQueue = NULL;
		}
	}else{
		ALOGE("[%s] alsa write thread made success\n", __func__);
	}
#endif
}

TCCAlsa::~TCCAlsa()
{
#ifdef USE_ALSA_WRITE_THREAD
	int status;
	ALOGE("[%s] alsa write thread state[%d]\n", __func__, iWriteThreadState);
	if(iWriteThreadState == 1) {
		iWriteThreadState = 0;
		while(1)
		{
			if(iWriteThreadState == -1) {
				break;
			}			
			usleep(2000);
		}

		status = tcc_dxb_thread_join((void *)stWriteThread,NULL);
		if(status != 0) {
			ALOGE("[%s] Failed tcc_dxb_thread_join(%d)\n", __func__, status);
		}

		if(pWriteQueue != NULL){
			tcc_dxb_queue_deinit(pWriteQueue);
			TCC_fo_free (__func__, __LINE__,pWriteQueue);
			pWriteQueue = NULL;
		}
		ALOGE("[%s]alsa write thread destroy success\n", __func__);
	}
#endif
	TCCALSA_SoundClose(mOut);

	TCC_fo_free (__func__, __LINE__,mOut->playback_status);
	pthread_mutex_destroy(&mOut->gsAlsa_snd_pcm_lock);

	TCC_fo_free(__func__, __LINE__, mOut);
}

int TCCAlsa::set(int iSampleRate, int iChannels, int iBitPerSample, int iEndian, int iFrameSize, int iOutputMode, void *pParam, void *DemuxApp)
{
	mOut->nSamplingRate = iSampleRate;
	mOut->nChannels = iChannels;
	mOut->nBitPerSample = iBitPerSample;
	mOut->eEndian = iEndian;
	mOut->alsa_configure_request = 1;

	mOut->pfGetSTC = pParam;
	mOut->pvDemuxApp = DemuxApp;
#ifdef USE_ALSA_WRITE_THREAD
	if(iWriteThreadState == -1) {
		int status;
		pWriteQueue = (tcc_dxb_queue_t *)TCC_fo_calloc (__func__, __LINE__,1, sizeof(tcc_dxb_queue_t));
		tcc_dxb_queue_init_ex(pWriteQueue, AUDIO_PCM_QUEUESIZE);
		iWriteThreadState = 1;
		status = tcc_dxb_thread_create((void *)&stWriteThread, writeThreadFunc, (unsigned char *)"ALSA_WRITE_THREAD", LOW_PRIORITY_11, (void *)this);
		if(status)	{
			ALOGE("[%s] Can't make alsa write thread\n", __func__);
			if(pWriteQueue != NULL){
				tcc_dxb_queue_deinit(pWriteQueue);
				TCC_fo_free (__func__, __LINE__,pWriteQueue);
				pWriteQueue = NULL;
			}
		}else{
			ALOGE("[%s] alsa write thread made success\n", __func__);
		}
	}
#endif
	mInitError = TCCALSA_SoundSetting(mOut);

	return mInitError;
}

void TCCAlsa::close()
{
	if (mInitError == 0)
	{
#ifdef USE_ALSA_WRITE_THREAD
	int status;
	if(iWriteThreadState == 1) {
		iWriteThreadState = 0;
		while(1)
		{
			if(iWriteThreadState == -1) {
				break;
			}			
			usleep(2000);
		}

		status = tcc_dxb_thread_join((void *)stWriteThread,NULL);
		if(status != 0) {
			ALOGE("[%s] Failed tcc_dxb_thread_join(%d)\n", __func__, status);
		}

		if(pWriteQueue != NULL){
			tcc_dxb_queue_deinit(pWriteQueue);
			TCC_fo_free (__func__, __LINE__, pWriteQueue);
			pWriteQueue = NULL;
		}
		ALOGE("[%s]alsa write thread destroy success\n", __func__);
	}
#endif
		TCCALSA_SoundClose(mOut);
		ALOGE("TCCAlsa::close\n");
	}
}

void TCCAlsa::stop()
{
	if (mInitError == 0)
	{
		TCCALSA_SoundReset(mOut);
	}
}

int TCCAlsa::setVolume(float left, float right)
{
	// Do nothing
	return 0;
}

int TCCAlsa::latency()
{
	int alsa_state;
	return TCCALSA_SoundGetLatency(mOut, &alsa_state);
}

int TCCAlsa::writable(int data_size)
{
#if 0
	int alsa_state;
	int frameSize = (mOut->nChannels * mOut->nBitPerSample) >> 3;

	if (mOut->fullthreshold)
	{
		int max_ms = (AUDIO_ALSA_BUFFER_FRAME_SIZE*1000)/mOut->nSamplingRate;
		int write_ms = ((data_size/frameSize)*1000)/mOut->nSamplingRate;

		if (write_ms > (max_ms - TCCALSA_SoundGetLatency(mOut, &alsa_state)))
		{
			//LOGD("[ALSASINK] WRITEOVER %d %d %d %d \n", max_ms, latency, (max_ms - latency), write_ms);
			return 0; // Wait
		}
	}
#endif
	return 1;
}
#endif


/****************************************************************************
 * class TCCAudioRender
 ****************************************************************************/
TCCAudioRender::TCCAudioRender(int iTrackCount)
{
	mCurrentIdx = -1;
	mAlsa = new TCCAlsa();
}

TCCAudioRender::~TCCAudioRender()
{
	delete mAlsa;
}

int TCCAudioRender::mute(bool bMute)
{
	return 0;
}

int TCCAudioRender::set(int iIdx, int iSampleRate, int iChannels, int iBitPerSample, int iEndian, int iFrameSize, int iOutputMode, void *pParam, void *DemuxApp)
{
	ALOGD("iIdx(%d) iSampleRate(%d) iChannels(%d) iBitPerSample(%d) iEndian(%d) iFrameSize(%d) iOutputMode(%d)",
			iIdx, iSampleRate, iChannels, iBitPerSample, iEndian, iFrameSize, iOutputMode);

#if 0
	mAlsa->stop();
#endif

	mCurrentIdx = iIdx;
	return mAlsa->set(iSampleRate, iChannels, iBitPerSample, iEndian, iFrameSize, iOutputMode, pParam, DemuxApp);

}

TCCAudioOut *TCCAudioRender::getOut(int iIdx)
{
	if (mCurrentIdx == iIdx)
	{
		return mAlsa;
	}
	return NULL;
}

void TCCAudioRender::close(int iIdx)
{
	TCCAudioOut *pOut = getOut(iIdx);

	if (pOut == NULL)
		return;

	pOut->close();
}

int TCCAudioRender::getOutputLatency(int iIdx)
{
	TCCAudioOut *pOut = getOut(iIdx);

	if (pOut == NULL)
		return 0;

	return pOut->getOutputLatency();
}

int TCCAudioRender::error_check(int iIdx)
{
	TCCAudioOut *pOut = getOut(iIdx);

	if (pOut == NULL)
		return -1;

	return pOut->initCheck();
}

void TCCAudioRender::stop(int iIdx)
{
	TCCAudioOut *pOut = getOut(iIdx);

	if (pOut == NULL)
		return;

	pOut->stop();
}

int TCCAudioRender::write(int iIdx, unsigned char *data, int data_size, long long llPTS)
{
	TCCAudioOut *pOut = getOut(iIdx);

	if (pOut == NULL)
		return 0;

	return pOut->write(data, data_size, llPTS, iIdx);
}

int TCCAudioRender::writable(int iIdx, int data_size)
{
	TCCAudioOut *pOut = getOut(iIdx);

	if (pOut == NULL)
		return 0;

	return pOut->writable(data_size);
}

int TCCAudioRender::latency(int iIdx)
{
	TCCAudioOut *pOut = getOut(iIdx);

	if (pOut == NULL)
		return 0;

	return pOut->latency();
}

int TCCAudioRender::setVolume(int iIdx, float left, float right)
{
	TCCAudioOut *pOut = getOut(iIdx);

	if (pOut == NULL)
		return -1;

	return pOut->setVolume(left, right);
}

int TCCAudioRender::latency_ex()
{
	return mAlsa->latency();
}

int TCCAudioRender::setCurrentIdx(int iIdx)
{
	mCurrentIdx = iIdx;
	return 0;
}
