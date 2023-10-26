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
#ifndef  _TCC_AUDIO_RENDER_H_
#define  _TCC_AUDIO_RENDER_H_

class TCCAudioOut
{
public:
	virtual ~TCCAudioOut() {};

	virtual int set(int iSampleRate, int iChannels, int iBitPerSample, int iEndian, int iFrameSize, int iOutputMode, void *pParam, void *DemuxApp) = 0;
	virtual void close() = 0;
	virtual void stop() = 0;
	virtual int write(unsigned char *data, int data_size, long long llPTS, int dev_id) = 0;
	virtual int setVolume(float left, float right) = 0;
	virtual int latency() = 0;
	virtual int writable(int data_size) = 0;

	int getOutputLatency() { return mLatency; };
	int initCheck() { return mInitError; };

protected:
	int mLatency;
	int mInitError;
	pthread_mutex_t mLock;
};

class TCCAudioRender
{
public:
	enum
	{
		OUTPUT_PCM,
		OUTPUT_HDMI_PASSTHRU_AC3,
		OUTPUT_HDMI_PASSTHRU_DDP,
		OUTPUT_SPDIF_PASSTHRU_AC3,
		OUTPUT_SPDIF_PASSTHRU_DDP,
	};

	TCCAudioRender(int iTrackCount);
	~TCCAudioRender();

	int error_check(int iIdx);
	int set(int iIdx, int iSampleRate, int iChannels, int iBitPerSample, int iEndian, int iFrameSize, int iOutputMode, void *pParam, void *DemuxApp);
	void close(int iIdx);
	void stop(int iIdx);
	int write(int iIdx, unsigned char *data, int data_size, long long llPTS);
	int setVolume(int iIdx, float left, float right);
	int mute(bool bMute);
	int latency(int iIdx);
	int getOutputLatency(int iIdx);
	int writable(int iIdx, int data_size);
	int latency_ex();
	int setCurrentIdx(int iIdx);

private:
	TCCAudioOut *getOut(int iIdx);

#if defined(HAVE_LINUX_PLATFORM)
	int mCurrentIdx;
	TCCAudioOut *mAlsa;
#endif
};

#endif//_TCC_AUDIO_RENDER_H_
