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
#ifndef	_ALSA_SOUND_SYSTEM_H_
#define	_ALSA_SOUND_SYSTEM_H_
#ifdef __cplusplus
extern "C" {
#endif

#define MAJOR_NUM 247
#define DEV_NAME "/dev/tcc-ioctl-ckc"

/* ioctl for audio */
#define IOCTL_ADMA_START            _IO(MAJOR_NUM, 150)
#define IOCTL_ADMA_STOP             _IO(MAJOR_NUM, 151)

#define IOCTL_I2S_START             _IO(MAJOR_NUM, 152)
#define IOCTL_I2S_STOP              _IO(MAJOR_NUM, 153)

#define IOCTL_GET_DAI_CKC           _IO(MAJOR_NUM, 154)
#define IOCTL_INC_DAI_CKC           _IO(MAJOR_NUM, 155)
#define IOCTL_DEC_DAI_CKC           _IO(MAJOR_NUM, 156)


#define AUDIO_PLAYBACK_CTRL_ID      	0x0001
#define AUDIO_CAPTURE_CTRL_ID       	0x0002
#define AUDIO_SPDIF_CTRL_ID         	0x0003

struct tcc_audio_ioctl {
    int audio_num;
    unsigned int sample_rate;
};

typedef long long (*pfnDemuxGetSTC)(int itype, long long lpts, unsigned int index, int log, void *pvApp);


typedef struct ALSA_SOUND_PRIVATE {
	snd_pcm_t*                  playback_handle;
	snd_pcm_hw_params_t*        hw_params;
	snd_pcm_sw_params_t*        sw_params;
	snd_pcm_status_t*			playback_status;
	int						   	alsa_ready;
	int                         fullthreshold;
	int                         output_delay_ms;
	int                         dropDataForAVsync; 
	int							nSamplingRate;
	int							nChannels;
	int							nBitPerSample;
	int							eEndian; // 0: OMX_EndianBig, 1: OMX_EndianLittle
	int						   	alsa_reopen_request;
	int						   	alsa_close_request;
	int							alsa_configure_request;
	long long                   llPTSLast;
	int							dev_id;
	void *						pfGetSTC;
	pfnDemuxGetSTC  			gfnDemuxGetSTCValue;
	void *						pvDemuxApp;
	int                         glALSAFirstWriteDone;
	long long                   glALSAOutDelay_StartTime;
	long long                   glALSAOutDelay_EndTime;
	pthread_mutex_t	            gsAlsa_snd_pcm_lock;
	snd_pcm_uframes_t           period_size;
	snd_pcm_uframes_t           buffer_size;
	snd_pcm_uframes_t           threshold_size;
}ALSA_SOUND_PRIVATE;

extern int TCCALSA_SoundSetting (ALSA_SOUND_PRIVATE * pPrivate);
extern int TCCALSA_SoundClose (ALSA_SOUND_PRIVATE * pPrivate);
extern int TCCALSA_SoundWrite (ALSA_SOUND_PRIVATE * pPrivate, unsigned char *pucBuffer, unsigned int uiBufferLength, long long llPTS, int dev_id);
extern int TCCALSA_SoundReset(ALSA_SOUND_PRIVATE * pPrivate);
extern int TCCALSA_SoundGetLatency(ALSA_SOUND_PRIVATE * pPrivate, int *alsa_state);
extern int TCCALSA_SoundGetMaxLatency(ALSA_SOUND_PRIVATE * pPrivate);

#ifdef __cplusplus
}
#endif
#endif


