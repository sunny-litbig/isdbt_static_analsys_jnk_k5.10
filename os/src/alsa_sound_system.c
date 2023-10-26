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
#if defined(HAVE_LINUX_PLATFORM)
#define LOG_NDEBUG 0
#define LOG_TAG	"ALSA_SOUND_SYSTEM"
#include <utils/Log.h>
#else
#include <stdio.h>
#define	ALOGD		printf
#define	ALOGE		printf
#define	ALOGI		printf
#endif

#define DEBUG_MSG(msg...)		//ALOGD(msg)

#include <alsa/asoundlib.h>
#include <alsa_sound_system.h>
#include <sys/time.h>
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

#ifndef TRUE
#define  TRUE                 1
#endif

#ifndef FALSE
#define  FALSE                0
#endif

#if defined(ALSA_PULSE_AUDIO)
#define AUDIO_ALSA_PERIOD_FRAME_SIZE    512
#else
#define AUDIO_ALSA_PERIOD_FRAME_SIZE    4096
#endif

#define AUDIO_ALSA_BUFFER_FRAME_SIZE    16384
#define AUDIO_ALSA_THRESHOLD_FRAME_SIZE ((AUDIO_ALSA_BUFFER_FRAME_SIZE/4)*3)
extern long long dxb_omx_alsasink_getsystemtime(void);

static char szZeroBuffer[AUDIO_ALSA_BUFFER_FRAME_SIZE*4];

long long dxb_omx_alsasink_getsystemtime(void)
{
	long long systime;
	struct timespec tspec;
	clock_gettime(CLOCK_MONOTONIC , &tspec);
	systime = (long long) tspec.tv_sec * 1000 + tspec.tv_nsec / 1000000;

	return systime;
}

static void TCCALSA_InitALSAOutDelay(ALSA_SOUND_PRIVATE *pPrivate)
{
	pPrivate->glALSAFirstWriteDone = 0;
	pPrivate->glALSAOutDelay_StartTime = 0;
	pPrivate->glALSAOutDelay_EndTime = 0;
	//AVSync_Display_log_count = 0;
	pPrivate->fullthreshold = 0;
	pPrivate->output_delay_ms = 0;
	pPrivate->dropDataForAVsync = 0;
	pPrivate->llPTSLast = -1;
}

int TCCALSA_SoundSetting (ALSA_SOUND_PRIVATE *pPrivate)
{
	int       err;
	snd_pcm_t *playback_handle;
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_sw_params_t *sw_params;
	char     *pcm_name;
	int       MinFrameSize;

	pPrivate->period_size	= (AUDIO_ALSA_PERIOD_FRAME_SIZE);
	pPrivate->buffer_size	= (AUDIO_ALSA_BUFFER_FRAME_SIZE);
	pPrivate->threshold_size = (AUDIO_ALSA_PERIOD_FRAME_SIZE);

	pthread_mutex_lock(&pPrivate->gsAlsa_snd_pcm_lock);

	pPrivate->alsa_ready = 0;
	pPrivate->gfnDemuxGetSTCValue = (pfnDemuxGetSTC)pPrivate->pfGetSTC;

	// alsa reopen start ------->
	if (pPrivate->alsa_reopen_request || pPrivate->alsa_close_request)
	{
		if(pPrivate->alsa_reopen_request == TRUE){
			if (pPrivate->hw_params)
			{
				snd_pcm_hw_params_free (pPrivate->hw_params);
				pPrivate->hw_params = NULL;
			}
			if (pPrivate->sw_params)
			{
				snd_pcm_sw_params_free (pPrivate->sw_params);
				pPrivate->sw_params = NULL;
			}
			if (pPrivate->playback_handle)
			{
				snd_pcm_close (pPrivate->playback_handle);
				pPrivate->playback_handle = NULL;
			}
		}

#if defined(ALSA_PULSE_AUDIO)
		pcm_name = "pulsemedia";
#else
		pcm_name = strdup ("plug:tcc");
#endif
		/* Allocate the playback handle and the hardware parameter structure */
		if ((err = snd_pcm_open (&pPrivate->playback_handle, pcm_name, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
		{
			ALOGE ("cannot open audio device %s (%s)\n", pcm_name, snd_strerror (err));
			goto ErrorALSA;
		}
		else
		{
			ALOGE ("Got playback handle at %p %p in %i\n",
				   pPrivate->playback_handle, &pPrivate->playback_handle,
				   getpid ());
			ALOGD("\033[32m Open ALSA success \033[m\n");
		}

		if (snd_pcm_hw_params_malloc (&pPrivate->hw_params) < 0)
		{
			ALOGE ("%s: failed allocating input pPort hw parameters\n", __func__);
			goto ErrorALSA;
		}
		else
			ALOGE ("Got hw parameters at %p\n", pPrivate->hw_params);

		if (snd_pcm_sw_params_malloc (&pPrivate->sw_params) < 0)
		{
			ALOGE ("%s: failed allocating input pPort sw parameters\n", __func__);
			goto ErrorALSA;
		}
		else
			ALOGE ("Got sw parameters at %p\n", pPrivate->sw_params);

		pPrivate->alsa_reopen_request = FALSE;
	}
	// alsa reopen end  <-------

	playback_handle = pPrivate->playback_handle;
	hw_params = pPrivate->hw_params;
	sw_params = pPrivate->sw_params;

	if (pPrivate->alsa_configure_request)
	{
		ALOGD("[%s:%d] Configure Alsa \n", __func__, __LINE__);

		ALOGE ("alsa configure setting!!!! %d %d\n", pPrivate->nSamplingRate,
				pPrivate->nChannels);

		if ((err =
			 snd_pcm_hw_params_any (pPrivate->playback_handle,
									pPrivate->hw_params)) < 0)
		{
			ALOGE ("cannot initialize hardware parameter structure (%s)\n", snd_strerror (err));
			goto ErrorALSA;
		}

		if ((err = snd_pcm_hw_params_set_access (playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
		{
			ALOGE ("cannot set access type intrleaved (%s)\n", snd_strerror (err));
			goto ErrorALSA;
		}

		pPrivate->eEndian = 1;
		if (pPrivate->eEndian == 1) // OMX_EndianLittle
		{
			switch (pPrivate->nBitPerSample)
			{
				case 24:
					if (snd_pcm_hw_params_set_format (playback_handle, hw_params, SND_PCM_FORMAT_S24_3LE) < 0)
					{
						ALOGE ("snd_pcm_hw_params_set_format error\n");
						goto ErrorALSA;
					}
					break;

				case 16:
				default:
					if (snd_pcm_hw_params_set_format (playback_handle, hw_params, SND_PCM_FORMAT_S16_LE) < 0)
					{
						ALOGE ("snd_pcm_hw_params_set_format error\n");
						goto ErrorALSA;
					}
					break;
			}
		}
		else
		{
			if (snd_pcm_hw_params_set_format (playback_handle, hw_params, SND_PCM_FORMAT_S16_BE) < 0)
			{
				ALOGE ("snd_pcm_hw_params_set_format error\n");
				goto ErrorALSA;
			}
		}

		// sample
		if (snd_pcm_hw_params_set_rate_near
			(playback_handle, hw_params, (unsigned int *)&pPrivate->nSamplingRate, 0) < 0)
		{
			ALOGE ("snd_pcm_hw_params_set_rate_near error\n");
			goto ErrorALSA;
		}
		// channel
		if (snd_pcm_hw_params_set_channels
			(playback_handle, hw_params, pPrivate->nChannels) < 0)
		{
			ALOGE ("snd_pcm_hw_params_set_channels error\n");
			goto ErrorALSA;
		}

		if(snd_pcm_hw_params_set_period_size_near(playback_handle, hw_params, &pPrivate->period_size,0) < 0)
		{
			ALOGE ("snd_pcm_hw_params_set_period_size_near error\n");
			goto ErrorALSA;
		}

		if(pPrivate->period_size != AUDIO_ALSA_PERIOD_FRAME_SIZE)
		{
			ALOGE("error incorrect period_size[%d][%d]\n", pPrivate->period_size, AUDIO_ALSA_PERIOD_FRAME_SIZE);
		}

		pPrivate->period_size = AUDIO_ALSA_PERIOD_FRAME_SIZE;
		if (snd_pcm_hw_params_set_buffer_size_near(playback_handle, hw_params, &pPrivate->buffer_size) < 0)
		{
			ALOGE ("snd_pcm_hw_params_set_buffer_size_near error \n");
			goto ErrorALSA;
		}

		if(pPrivate->buffer_size != AUDIO_ALSA_BUFFER_FRAME_SIZE)
		{
			ALOGE("error incorrect buffer_size[%d][%d]\n", pPrivate->buffer_size, AUDIO_ALSA_BUFFER_FRAME_SIZE);
		}

		pPrivate->buffer_size = AUDIO_ALSA_BUFFER_FRAME_SIZE;

		if ((err = snd_pcm_hw_params (playback_handle, hw_params)) < 0)
		{
			ALOGE ("cannot set parameters (%s)\n", snd_strerror (err));
			goto ErrorALSA;
		}
#if 0 //jktest
		{
			unsigned int trate = 0;
			unsigned int tchannel = 0;
			unsigned int tperiod_size = 0;
			unsigned int tperiod_size_min = 0;
			unsigned int tperiod_size_max = 0;
			unsigned int tperiod_time = 0;
			unsigned int tbuffer_size = 0;
			unsigned int tbuffer_size_min = 0;
			unsigned int tbuffer_size_max = 0;
			unsigned int tbuffer_time = 0;
			unsigned int tfifo_size = 0;
			snd_pcm_format_t tformat = 0;
			snd_pcm_hw_params_get_format(hw_params, &tformat);
			snd_pcm_hw_params_get_channels(hw_params, &tchannel);
			snd_pcm_hw_params_get_rate(hw_params, &trate, 0);
			snd_pcm_hw_params_get_period_size(hw_params, &tperiod_size,0);
			snd_pcm_hw_params_get_period_size_min(hw_params, &tperiod_size_min,0);
			snd_pcm_hw_params_get_period_size_max(hw_params, &tperiod_size_max,0);
			snd_pcm_hw_params_get_period_time(hw_params, &tperiod_time,0);
			snd_pcm_hw_params_get_buffer_size(hw_params, &tbuffer_size);
			snd_pcm_hw_params_get_buffer_size_min(hw_params, &tbuffer_size_min);
			snd_pcm_hw_params_get_buffer_size_max(hw_params, &tbuffer_size_max);
			snd_pcm_hw_params_get_buffer_time(hw_params, &tbuffer_time, 0);
			tfifo_size = snd_pcm_hw_params_get_fifo_size(hw_params);
			ALOGE("\033[101m###jktest %s format\t[0x%X]\033[0m", __func__, tformat);
			ALOGE("\033[101m###jktest %s channel\t[0x%X]\033[0m", __func__, tchannel);
			ALOGE("\033[101m###jktest %s rate\t[0x%X]\033[0m", __func__, trate);
			ALOGE("\033[101m###jktest %s period\t[0x%X]\033[0m", __func__, tperiod_size);
			ALOGE("\033[101m###jktest %s peri_min\t[0x%X]\033[0m", __func__, tperiod_size_min);
			ALOGE("\033[101m###jktest %s peri_max\t[0x%X]\033[0m", __func__, tperiod_size_max);
			ALOGE("\033[101m###jktest %s peri_time\t[0x%X]\033[0m", __func__, tperiod_time);
			ALOGE("\033[101m###jktest %s buffer\t[0x%X]\033[0m", __func__, tbuffer_size);
			ALOGE("\033[101m###jktest %s buf_min\t[0x%X]\033[0m", __func__, tbuffer_size_min);
			ALOGE("\033[101m###jktest %s buf_max\t[0x%X]\033[0m", __func__, tbuffer_size_max);
			ALOGE("\033[101m###jktest %s buf_time\t[0x%X]\033[0m", __func__, tbuffer_time);
			ALOGE("\033[101m###jktest %s fifo\t[0x%X]\033[0m", __func__, tfifo_size);
		}
#endif

		snd_pcm_sw_params_current(playback_handle, sw_params);

		#if 0
		if ((err = snd_pcm_sw_params_set_sleep_min(playback_handle, sw_params, 0)) < 0)
		{
			ALOGE ("snd_pcm_sw_params_set_sleep_min error (%s)\n", snd_strerror(err));
			return -1;
		}

		if ((err = snd_pcm_sw_params_set_avail_min(playback_handle, sw_params, period_size)) < 0)
		{
			ALOGE ("snd_pcm_sw_params_set_avail_min error (%s)\n", snd_strerror(err));
			return -1;
		}
		#endif

		if ((err = snd_pcm_sw_params_set_start_threshold(playback_handle, sw_params, pPrivate->threshold_size)) < 0)
		{
			ALOGE ("snd_pcm_sw_params_set_start_threshold error (%s)\n", snd_strerror(err));
			goto ErrorALSA;
		}

		if ((err = snd_pcm_sw_params(playback_handle, sw_params)) < 0)
		{
			ALOGE ("snd_pcm_sw_params error (%s)\n", snd_strerror(err ));
			goto ErrorALSA;
		}

		if ((err = snd_pcm_prepare (playback_handle)) < 0)
		{
			ALOGE ("cannot prepare audio interface for use (%s)\n", snd_strerror (err));
			goto ErrorALSA;
		}

		pPrivate->alsa_configure_request = FALSE;
		pPrivate->alsa_close_request = FALSE;
		TCCALSA_InitALSAOutDelay(pPrivate);
		pPrivate->alsa_ready = 1;
		memset (szZeroBuffer,0, sizeof(szZeroBuffer));
	}

	pthread_mutex_unlock(&pPrivate->gsAlsa_snd_pcm_lock);
	return 0;

ErrorALSA:
	ALOGE("%s failed\n", __func__);
	pthread_mutex_unlock(&pPrivate->gsAlsa_snd_pcm_lock);
	return -1;
}

int TCCALSA_SoundClose (ALSA_SOUND_PRIVATE *pPrivate)
{
	pthread_mutex_lock(&pPrivate->gsAlsa_snd_pcm_lock);

	ALOGV("[%s] pPrivate->alsa_close_request[%d]\n", __func__, pPrivate->alsa_close_request);
	if(pPrivate->alsa_close_request == FALSE){
	if (pPrivate->hw_params)
	{
		snd_pcm_hw_params_free (pPrivate->hw_params);
		pPrivate->hw_params = NULL;
	}

	if (pPrivate->sw_params)
	{
		snd_pcm_sw_params_free (pPrivate->sw_params);
		pPrivate->sw_params = NULL;
	}

	if (pPrivate->playback_handle)
	{
		ALOGD("\033[32m close ALSA success \033[m\n");
		snd_pcm_close (pPrivate->playback_handle);
		pPrivate->playback_handle = NULL;
		}
	}

	pPrivate->alsa_close_request = TRUE;
	ALOGV("[%s]\n", __func__);
	pthread_mutex_unlock(&pPrivate->gsAlsa_snd_pcm_lock);
	return 0;
}

long long OmxAlsaGetSTC(ALSA_SOUND_PRIVATE *pPrivate, int dev_id)
{
	long long llSTCValue;

	if( pPrivate->gfnDemuxGetSTCValue != NULL )
	{
		llSTCValue = pPrivate->gfnDemuxGetSTCValue(-1, 0, dev_id, 0, pPrivate->pvDemuxApp);
	}
	else
	{
		llSTCValue=0;
	}

	return llSTCValue;
}

static int tcc_snd_pcm_avail_update(snd_pcm_t *pcm)
{
	snd_pcm_sframes_t avail;
	if(!pcm)
		return 0;
	avail = snd_pcm_avail_update(pcm);
	if(avail < (long)0) {
		ALOGE("%s failed[%ld][%s]", __func__, avail, snd_strerror(avail));
		avail = (long)0;
	}
	else if(avail > (long)AUDIO_ALSA_BUFFER_FRAME_SIZE) {
		ALOGE("%s size error[%ld]", __func__, avail);
		avail = (long)AUDIO_ALSA_BUFFER_FRAME_SIZE;
	}
	return (int)avail;
}

//#define _DEBUG_AUDIO_SYNC_
//#define _DEBUG_AUDIO_SYNC_DETAIL_
int TCCALSA_SoundWrite (ALSA_SOUND_PRIVATE *pPrivate, unsigned char *pucBuffer, unsigned int uiBufferLength, long long llPTS, int dev_id)
{
	int frameSize, totalBuffer, offsetBuffer, written;
	int allDataSent;
	int loop_count;
	int write_elapsed;
	int avail;
	snd_pcm_state_t state;
	int frames, writeframes;
	int err = 0;
	int fulldatatoready = 0;
	long long llTimeSTC;

	loop_count = ((AUDIO_ALSA_BUFFER_FRAME_SIZE * 1000)/pPrivate->nSamplingRate)/10;

	if (pPrivate->playback_handle == NULL)
	{
		ALOGE ("%s Alsa didn't open !!!!\n", __func__);
		return -1;
	}

	frameSize = (pPrivate->nChannels * pPrivate->nBitPerSample) >> 3;

	allDataSent		= FALSE;
	totalBuffer		= uiBufferLength / frameSize;
	offsetBuffer	= 0;

	while (!allDataSent && loop_count)
	{
		if (pPrivate->alsa_ready == 0)
			break;

		llTimeSTC = OmxAlsaGetSTC(pPrivate, dev_id);

		if(pPrivate->glALSAFirstWriteDone == 0)
		{
			int iSizeOfZeroFrames;
			long long llTimePTS;
			int iDiff;
			llTimePTS = llPTS;
			iDiff = (int)(llTimePTS-llTimeSTC);

			if( iDiff < 50 )
			{
				/* too late. discard */
				ALOGD("[ALSASINK] Too late. Discard. STC %lld, PTS %lld, diff %d", llTimeSTC, llTimePTS, (int)(llTimePTS-llTimeSTC));
				return 0;
			}
			else if ( iDiff > 200 )
			{
				/* too early. abnoraml status. */
				ALOGD("[ALSASINK] Too early. Discard. STC %lld, PTS %lld, diff %d", llTimeSTC, llTimePTS, (int)(llTimePTS-llTimeSTC));
			}

			iSizeOfZeroFrames = (int)(llTimePTS-llTimeSTC)*((pPrivate->nSamplingRate)/1000);
			if( (iSizeOfZeroFrames>AUDIO_ALSA_BUFFER_FRAME_SIZE) || (iSizeOfZeroFrames<0))
			{
				ALOGD("[ALSASINK] ZeroFill=%d size error. discard.", iSizeOfZeroFrames);
				return 0;
			}
			pthread_mutex_lock(&pPrivate->gsAlsa_snd_pcm_lock);
			written = snd_pcm_writei(pPrivate->playback_handle, szZeroBuffer, iSizeOfZeroFrames);
			pthread_mutex_unlock(&pPrivate->gsAlsa_snd_pcm_lock);

			#ifdef _DEBUG_AUDIO_SYNC_
			ALOGD("[ALSASINK] FillZero STC %lld, PTS %lld, diff %d, frame %d", llTimeSTC, llTimePTS, (int)(llTimePTS-llTimeSTC), iSizeOfZeroFrames);
			#endif
			pPrivate->glALSAFirstWriteDone = 1;
			pPrivate->llPTSLast = llPTS;
			pPrivate->dev_id = dev_id;
		}

		if( dev_id != pPrivate->dev_id )
		{
			int iDuration = (uiBufferLength/frameSize)*1000/(pPrivate->nSamplingRate);
			int iPTSgap;
			int latency_ms;

			state = snd_pcm_state(pPrivate->playback_handle);
			avail = tcc_snd_pcm_avail_update(pPrivate->playback_handle);
			if (state == SND_PCM_STATE_RUNNING)
			{
				if (avail <= AUDIO_ALSA_BUFFER_FRAME_SIZE)
					latency_ms = ((AUDIO_ALSA_BUFFER_FRAME_SIZE - avail)*1000)/pPrivate->nSamplingRate;
				else
					latency_ms = AUDIO_ALSA_BUFFER_FRAME_SIZE*1000/pPrivate->nSamplingRate;

				llTimeSTC = (OmxAlsaGetSTC(pPrivate, dev_id));
				pPrivate->llPTSLast = llTimeSTC+latency_ms;

				#ifdef _DEBUG_AUDIO_SYNC_
				ALOGD("[ALSASINK] switch dec[%d->%d], STC %lld, lastPTS %lld, PTS %lld, diff %d"
					, pPrivate->dev_id, dev_id
					, llTimeSTC
					, pPrivate->llPTSLast
					, llPTS
					, (int)(pPrivate->llPTSLast - llPTS)
				);
				#endif

				pPrivate->dev_id = dev_id;

				iPTSgap = (int)(pPrivate->llPTSLast-llPTS);

				if( iPTSgap >= iDuration )
				{
					/* discard frame */
					#ifdef _DEBUG_AUDIO_SYNC_
					ALOGD("[ALSASINK] discard frame diff %d > iDuration %d", iPTSgap, iDuration);
					#endif
					return 0;
				}
				else if( iPTSgap > 0 )
				{
					/* skip pcm */
					int iSkipFrames =  iPTSgap*((pPrivate->nSamplingRate)/1000);
					llPTS += iPTSgap;
					offsetBuffer += iSkipFrames;
					totalBuffer -= iSkipFrames;
					#ifdef _DEBUG_AUDIO_SYNC_
					ALOGD("[ALSASINK] skip %dms, %d frames", iPTSgap, iSkipFrames);
					#endif
				}
				#if 0
				else if( iPTSgap <= 0 )
				{
					/* go */
				}
				#endif
			}
			else
			{
				pPrivate->alsa_ready = 0;
				snd_pcm_prepare(pPrivate->playback_handle);
				TCCALSA_InitALSAOutDelay(pPrivate);
				pPrivate->alsa_ready = 1;
				continue;
			}
		}

		int iPTSdiff = (int)(llPTS - pPrivate->llPTSLast);
		if( iPTSdiff<-50 || iPTSdiff>50 )
		{
			#ifdef _DEBUG_AUDIO_SYNC_
			ALOGD("[ALSASINK] DropPCM, STC %lld, PTS %lld, LastPTS %lld, lastdiff %d", llTimeSTC, llPTS, pPrivate->llPTSLast, iPTSdiff );
			#endif
			snd_pcm_drop(pPrivate->playback_handle);
			pPrivate->alsa_ready = 0;
			snd_pcm_prepare(pPrivate->playback_handle);
			TCCALSA_InitALSAOutDelay(pPrivate);
			pPrivate->alsa_ready = 1;
			continue;
		}

		long long iPTSOutputEstimated;
		llTimeSTC = OmxAlsaGetSTC(pPrivate, dev_id);
		state = snd_pcm_state(pPrivate->playback_handle);
		avail = tcc_snd_pcm_avail_update(pPrivate->playback_handle);

		if (state==SND_PCM_STATE_RUNNING)
		{
			iPTSOutputEstimated = pPrivate->llPTSLast - ( (AUDIO_ALSA_BUFFER_FRAME_SIZE-avail)*1000/(pPrivate->nSamplingRate) );
			iPTSdiff = (int)(iPTSOutputEstimated - llTimeSTC);
			if( iPTSdiff<-50 || iPTSdiff>50 )
			{
				#ifdef _DEBUG_AUDIO_SYNC_
				ALOGD("[ALSASINK] DropPCM2, STC %lld, PTS %lld, LastPTS %lld, OutPTS %lld, diff %d", llTimeSTC, llPTS, pPrivate->llPTSLast, iPTSOutputEstimated, iPTSdiff );
				#endif
				snd_pcm_drop(pPrivate->playback_handle);
				pPrivate->alsa_ready = 0;
				snd_pcm_prepare(pPrivate->playback_handle);
				TCCALSA_InitALSAOutDelay(pPrivate);
				pPrivate->alsa_ready = 1;
				continue;
			}
		}

		pthread_mutex_lock(&pPrivate->gsAlsa_snd_pcm_lock);
#ifdef _DEBUG_AUDIO_SYNC_DETAIL_/* debug */
		unsigned int uiTimeWriteStart = (unsigned int)(dxb_omx_alsasink_getsystemtime());
#endif

		written = snd_pcm_writei(pPrivate->playback_handle, pucBuffer + (offsetBuffer * frameSize), totalBuffer);

#ifdef _DEBUG_AUDIO_SYNC_DETAIL_/* debug */
		unsigned int uiTimeWriteDone = (unsigned int)(dxb_omx_alsasink_getsystemtime());
		llTimeSTC = OmxAlsaGetSTC(pPrivate, dev_id);
		state = snd_pcm_state(pPrivate->playback_handle);
		avail = tcc_snd_pcm_avail_update(pPrivate->playback_handle);
#endif
		pthread_mutex_unlock(&pPrivate->gsAlsa_snd_pcm_lock);
		DEBUG_MSG("[TCCALSA_SoundWrite][%lld] snd_pcm_writei(%d)(%d) ", dxb_omx_alsasink_getsystemtime(), written, totalBuffer);

		if (written < 0)
		{
			if (written == -EPIPE)
			{
				state = snd_pcm_state(pPrivate->playback_handle);
				avail = tcc_snd_pcm_avail_update(pPrivate->playback_handle);

				ALOGE ("ALSA Underrun.. state(%d) avail(%d), written(%d)  \n", state, avail, written);
				written = 0;
				pPrivate->alsa_ready = 0;
				snd_pcm_prepare(pPrivate->playback_handle);
				TCCALSA_InitALSAOutDelay(pPrivate);
				pPrivate->alsa_ready = 1;
				err = -2;
				break;
			}
			else
			{
				state = snd_pcm_state(pPrivate->playback_handle);
				ALOGI("%s error: %s. state(%d) Trying reopen, configure", __func__, snd_strerror(written), state);
				pPrivate->alsa_reopen_request = pPrivate->alsa_configure_request = TRUE;
				if(TCCALSA_SoundSetting(pPrivate) != 0) {
					ALOGE("%s reopen, configure error !!!", __func__);
				}
				err = -2;
				break;
			}
		}
#ifdef _DEBUG_AUDIO_SYNC_DETAIL_/* debug */
		else
		{
			int uiTimePCMWritten		= (offsetBuffer+written)*1000/(pPrivate->nSamplingRate);
			long long uiTimePTSTail	= llPTS+uiTimePCMWritten;
			long long uiTimePTSHead	= uiTimePTSTail-( (AUDIO_ALSA_BUFFER_FRAME_SIZE-avail)*1000/(pPrivate->nSamplingRate) );
			ALOGD( "[TCCALSA_SoundWrite][%d] %010u, %019lld, %019lld, %019lld, %3d, %3u, %d, %d"//",/, %d, %d, %d"
				, dev_id
				, uiTimeWriteDone                  /* system time */
				, llTimeSTC                       /* STC */
				, uiTimePTSTail                    /* PTS of tail of alsa_buffer */
				, uiTimePTSHead                    /* PTS of head of alsa_buffer */
				, (int)(uiTimePTSHead-llTimeSTC)  /* GAP bwt STC and PTS(current output) */
				, uiTimeWriteDone-uiTimeWriteStart /* spent time in snd_pcm_writei() */
				, state                            /* current status of alsa */
				, avail                            /* available size of alsa_buffer */
				//, offsetBuffer
				//, written
				//, uiTimePCMWritten
			);
		}
#endif

		if (written != totalBuffer)
		{
			totalBuffer = totalBuffer - written;
			offsetBuffer = written;
		}
		else
		{
			//ALOGD("Buffer successfully sent to ALSA. Length was %i\n", (int) uiBufferLength);
			allDataSent = TRUE;

			if (pPrivate->glALSAFirstWriteDone == 0)
			{
				pPrivate->glALSAOutDelay_StartTime = dxb_omx_alsasink_getsystemtime();
				pPrivate->glALSAFirstWriteDone = 1;
				//ALOGD("[TCCALSA_SoundWrite] Start(%lld) \n", pPrivate->glALSAOutDelay_StartTime);
			}
		}

		--loop_count;
	}

	if (!loop_count)
	{
		ALOGE ("[ALSAOUT] Write Hang Up !!!!!! \n");
		pPrivate->alsa_ready = 0;
		snd_pcm_prepare(pPrivate->playback_handle);
		TCCALSA_InitALSAOutDelay(pPrivate);
		pPrivate->alsa_ready = 1;
		err = -3;
	}

	pPrivate->llPTSLast = llPTS + (uiBufferLength/frameSize)*1000/(pPrivate->nSamplingRate);
	pPrivate->dev_id = dev_id;
	if (pPrivate->glALSAOutDelay_EndTime == 0)
	{
		//ALOGD("[ALSAOUT] (%lld) write pcm(%d)  \n", dxb_omx_alsasink_getsystemtime(), uiBufferLeangth);
	}

	return err;
}

int TCCALSA_SoundReset(ALSA_SOUND_PRIVATE *pPrivate)
{
	if( pPrivate->playback_handle == NULL)
	{
		ALOGE ("[%s] Alsa didn't open !!!!\n", __func__);
		return -1;
	}

	pthread_mutex_lock(&pPrivate->gsAlsa_snd_pcm_lock);

	pPrivate->alsa_ready = 0;
	snd_pcm_prepare(pPrivate->playback_handle);
	TCCALSA_InitALSAOutDelay(pPrivate);
	pPrivate->alsa_ready = 1;

	pthread_mutex_unlock(&pPrivate->gsAlsa_snd_pcm_lock);

	return 0;
}

int TCCALSA_SoundGetLatency(ALSA_SOUND_PRIVATE *pPrivate, int* alsa_state)
{
	int	latency_ms = 0;
	int thresholdsize, threshold_ms, gap_ms;
	int avail;
	int default_latency = (AUDIO_ALSA_THRESHOLD_FRAME_SIZE*1000)/pPrivate->nSamplingRate;
	snd_pcm_state_t		state;
	snd_pcm_sframes_t	delay = 0;

	if (pPrivate->playback_handle == NULL)
	{
		return latency_ms;
	}

	snd_pcm_status(pPrivate->playback_handle, pPrivate->playback_status);
	state = snd_pcm_status_get_state(pPrivate->playback_status);
	avail = snd_pcm_avail_update(pPrivate->playback_handle);
	delay = snd_pcm_status_get_delay(pPrivate->playback_status);
	*alsa_state = (int)state;

	//latency_ms = 0;

	if (state == SND_PCM_STATE_RUNNING)
	{
		if (avail <= AUDIO_ALSA_BUFFER_FRAME_SIZE)
			latency_ms = ((AUDIO_ALSA_BUFFER_FRAME_SIZE - avail)*1000)/pPrivate->nSamplingRate;
		else
			latency_ms = AUDIO_ALSA_BUFFER_FRAME_SIZE*1000/pPrivate->nSamplingRate;
	}
	else
	{
		latency_ms = default_latency;
	}

#if 0
	if (state == SND_PCM_STATE_RUNNING)
	{
		if (pPrivate->glALSAFirstWriteDone == 1 && pPrivate->glALSAOutDelay_StartTime > 0 && pPrivate->glALSAOutDelay_EndTime == 0)
		{
			pPrivate->glALSAOutDelay_EndTime = dxb_omx_alsasink_getsystemtime();

			if (pPrivate->glALSAOutDelay_EndTime >= pPrivate->glALSAOutDelay_StartTime)
				gap_ms = (int)(pPrivate->glALSAOutDelay_EndTime - pPrivate->glALSAOutDelay_StartTime);
			else
				gap_ms = 0;

			thresholdsize = pPrivate->threshold_size;
			threshold_ms = (thresholdsize*1000)/pPrivate->nSamplingRate;

			if (gap_ms > threshold_ms)
			{
				pPrivate->alsa_ready = 0;
				snd_pcm_prepare(pPrivate->playback_handle);
				TCCALSA_InitALSAOutDelay(pPrivate);
				pPrivate->alsa_ready = 1;
			}
			else
			{
				ALOGD("[ALSASINK] Threshold(%d-%d-%d) Output Delay(%dmS) %d \n",
					thresholdsize,
					pPrivate->nSamplingRate,
					threshold_ms,
					gap_ms, delay);

				pPrivate->glALSAOutDelay_StartTime = 0;
				pPrivate->fullthreshold = 1;
				pPrivate->output_delay_ms = gap_ms;
			}
		}
	}
	else
	{
		pPrivate->glALSAOutDelay_EndTime = 0;
	}

	if (pPrivate->glALSAOutDelay_EndTime == 0)
	{
		//ALOGD("[ALSAOUT] Get Latency l(%dmS) d(%d) s(%d) \n", state, latency_ms, delay);
	}

	if(latency_ms < default_latency)
		latency_ms = default_latency;
#endif
	return latency_ms;
}

int TCCALSA_SoundGetMaxLatency(ALSA_SOUND_PRIVATE *pPrivate)
{
	return (int)((AUDIO_ALSA_BUFFER_FRAME_SIZE*1000)/pPrivate->nSamplingRate);
}
