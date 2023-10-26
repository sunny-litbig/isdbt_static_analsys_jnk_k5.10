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
#define LOG_TAG	"TCC_DXB_SURFACE"

#include <utils/Log.h>
#include <cutils/properties.h>

#if defined(HAVE_LINUX_PLATFORM)
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#endif

#include <sys/mman.h>
#include <libpmap/pmap.h>
#include <fcntl.h>
#include <dirent.h>

#include "tcc_video_render.h"

#define TMEM_DEVICE "/dev/tmem"

#if defined(HAVE_LINUX_PLATFORM)
#include "Hwrenderer.h"
#include <mach/tcc_video_private.h>
#define NUM_OF_PMEM_INFO  3
#endif
#include "TCCMemory.h"

class TCCVideoOut {
public:
	TCCVideoOut() {};
	~TCCVideoOut() {};

#if defined(HAVE_LINUX_PLATFORM)
	int mfd_id;
	void* mTMapInfo;
	HwRenderer *mHwRenderer;
#endif
};

TCCVideoOut *mOut;

TCCVideoRender::TCCVideoRender(int flag)
{
	mOut = new TCCVideoOut();

#if defined(HAVE_LINUX_PLATFORM)
	mOut->mHwRenderer = NULL;
	mOut->mfd_id = 0;
	mOut->mTMapInfo = TCC_fo_calloc (__func__, __LINE__,sizeof(TCC_PLATFORM_PRIVATE_PMEM_INFO)*NUM_OF_PMEM_INFO, 1);
#endif
}

TCCVideoRender::~TCCVideoRender(void)
{
#if defined(HAVE_LINUX_PLATFORM)
	if (mOut->mHwRenderer) {
		delete mOut->mHwRenderer;
	}
	mOut->mHwRenderer = NULL;
	if( mOut->mTMapInfo != NULL )
	{
		TCC_fo_free (__func__, __LINE__, mOut->mTMapInfo );
	}
#endif
	delete mOut;
}

void* TCCVideoRender::GetPrivateAddr(int fd_val, int width, int height)
{
	//ALOGV("%s", __func__);
	return (void*)fd_val;
}

int TCCVideoRender::SetVideoSurface(void *native)
{
	return 0;
}

int TCCVideoRender::ReleaseVideoSurface(void)
{
	return 0;
}

int TCCVideoRender::SetNativeWindow(int width, int height, int format, int outmode)
{
#if defined(HAVE_LINUX_PLATFORM)
	if(mOut->mHwRenderer == NULL)
	{
		mOut->mHwRenderer = new HwRenderer( 0,			// int overlay_ch
												width,		// in inWidth
												height,		// int inHeight
												format,		// int inFormat
												0,			// int outLeft
												0,			// int outTop
												1024,		// int outRigth
												600,		// int outBottom
												0,			// unsigned int transform
												0,			// int creation_form_external
												1			// int numOfExternalVideo
												);

		if(mOut->mHwRenderer->initCheck() == false) {
			ALOGE("mHwRenderer->initCheck failed");
			return 1;
		}

		SetDisplaySize();
		mOut->mHwRenderer->setCropRegion((int)mOut->mTMapInfo,
											0,
											width,
											height,
											0,
											0,
											width,
											height,
											0,
											0,
											m_clDisplayPos.m_iWidth,
											m_clDisplayPos.m_iHeight,
											0
											);
	}
#endif
	return 0;
}

int TCCVideoRender::dequeueBuffer(unsigned char **buf, int width, int height, int pts)
{
#if defined(HAVE_LINUX_PLATFORM)
	*buf = (unsigned char*)(mOut->mTMapInfo + mOut->mfd_id*sizeof(TCC_PLATFORM_PRIVATE_PMEM_INFO));
#endif
	return 0;
}

void TCCVideoRender::enqueueBuffer(unsigned char *buf, int width, int height, int pts)
{
#if defined(HAVE_LINUX_PLATFORM)
	mOut->mHwRenderer->setCropRegion((int)buf,
										0,
										width,
										height,
										0,
										0,
										width,
										height,
										0,
										0,
										m_clDisplayPos.m_iWidth,
										m_clDisplayPos.m_iHeight,
										0
										);

	mOut->mHwRenderer->render((int)buf,
									width,
									height,
									0,
									0,
									m_clDisplayPos.m_iWidth,
									m_clDisplayPos.m_iHeight,
									0);

	mOut->mfd_id++;

	if(mOut->mfd_id >= NUM_OF_PMEM_INFO)
		mOut->mfd_id = 0;
#endif
}

int TCCVideoRender::IsVSyncEnabled()
{
	return 1;
}

void TCCVideoRender::setCrop(int left, int top, int width, int height)
{

}

int TCCVideoRender::getfromsysfs(const char *sysfspath, char *val)
{
  int sysfs_fd;
  int read_count;

  sysfs_fd = open(sysfspath, O_RDONLY);
  read_count = read(sysfs_fd, val, PROPERTY_VALUE_MAX);
  val[read_count] = '\0';
  close(sysfs_fd);
  return read_count;
}

int TCCVideoRender::SetDisplaySize(void)
{
#if 1

// Noah, To avoid CodeSonar warning, Redundant Condition

	unsigned int uiOutputMode = 0;

#if defined(HAVE_LINUX_PLATFORM)
	////////////////////////////  TASACHK <START> /////////////////////////////
	ALOGE("[TASACHK](TCCVideoRender::SetDisplaySize) uiOutputMode=%d m_clDisplayPos.m_iWidth=%d m_clDisplayPos.m_iHeight=%d"
		,uiOutputMode
		,m_clDisplayPos.m_iWidth
		,m_clDisplayPos.m_iHeight
	);
	m_clDisplayPos.m_iWidth  = 1024;
	m_clDisplayPos.m_iHeight = 600;
	ALOGE("[TASACHK](TCCVideoRender::SetDisplaySize) uiOutputMode=%d m_clDisplayPos.m_iWidth=%d m_clDisplayPos.m_iHeight=%d  (FORCE SET 800x480)"
		,uiOutputMode
		,m_clDisplayPos.m_iWidth
		,m_clDisplayPos.m_iHeight
	);
	///////////////////////////// TASACHK <END> //////////////////////////////
#endif
	return 0;

#else

#if defined(HAVE_LINUX_PLATFORM)
	char value[PROPERTY_VALUE_MAX];
	unsigned int uiOutputMode = 0;

//	getfromsysfs("/sys/class/tcc_dispman/tcc_dispman/persist_output_mode", value);
//	uiOutputMode = atoi(value);

	if(uiOutputMode == 0){ //lcd only
//		getfromsysfs("/sys/class/tcc_dispman/tcc_dispman/tcc_output_panel_width", value);
//		m_clDisplayPos.m_iWidth = atoi(value);
//		getfromsysfs("/sys/class/tcc_dispman/tcc_dispman/tcc_output_panel_height", value);
//		m_clDisplayPos.m_iHeight = atoi(value);

		////////////////////////////  TASACHK <START> /////////////////////////////
		ALOGE("[TASACHK](TCCVideoRender::SetDisplaySize) uiOutputMode=%d m_clDisplayPos.m_iWidth=%d m_clDisplayPos.m_iHeight=%d"
			,uiOutputMode
			,m_clDisplayPos.m_iWidth
			,m_clDisplayPos.m_iHeight
		);
		m_clDisplayPos.m_iWidth  = 1024;
		m_clDisplayPos.m_iHeight = 600;
		ALOGE("[TASACHK](TCCVideoRender::SetDisplaySize) uiOutputMode=%d m_clDisplayPos.m_iWidth=%d m_clDisplayPos.m_iHeight=%d  (FORCE SET 800x480)"
			,uiOutputMode
			,m_clDisplayPos.m_iWidth
			,m_clDisplayPos.m_iHeight
		);
		///////////////////////////// TASACHK <END> //////////////////////////////

	}else if(uiOutputMode == 1){ //lcd +hdmi, default:lcd
//		getfromsysfs("/sys/class/tcc_dispman/tcc_dispman/tcc_output_panel_width", value);
//		m_clDisplayPos.m_iWidth = atoi(value);
//		getfromsysfs("/sys/class/tcc_dispman/tcc_dispman/tcc_output_panel_height", value);
//		m_clDisplayPos.m_iHeight = atoi(value);
		m_clDisplayPos.m_iWidth  = 0;
		m_clDisplayPos.m_iHeight = 0;
	}else{ //hdmi/component/composite
//		getfromsysfs("/sys/class/tcc_dispman/tcc_dispman/tcc_output_dispdev_width", value);
//		m_clDisplayPos.m_iWidth = atoi(value);
//		getfromsysfs("/sys/class/tcc_dispman/tcc_dispman/tcc_output_dispdev_height", value);
//		m_clDisplayPos.m_iHeight = atoi(value);
		m_clDisplayPos.m_iWidth  = 0;
		m_clDisplayPos.m_iHeight = 0;
	}
#endif
	return 0;

#endif
}
