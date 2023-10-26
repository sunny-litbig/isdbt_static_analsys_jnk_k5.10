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

#include <pthread.h>
#include <string.h>

#include <mach/tcc_video_private.h>

#include <OMX_IVCommon.h>

#include "tcc_dxb_surface.h"
#include "tcc_video_render.h"

#ifdef SUPPORT_SCALER_COPY
extern int TCCDxB_ScalerOpen(void);
extern void TCCDxB_ScalerClose(void);
extern int TCCDxB_ScalerCopyData(unsigned int width, unsigned int height, unsigned char *YSrc, unsigned char *USrc, unsigned char *VSrc, 
								char bSrcYUVInter, unsigned char *addrDst, unsigned char ignoreAligne, int fieldInfomation, int interlaceMode);
extern int TCCDxB_CaptureImage(unsigned int width, unsigned int height, int *yuvbuffer, int format, int interlaceMode, int fieldInfomation, unsigned char *strFilePath, int iCropLeft, int iCropTop, int iCropWidth, int iCropHeight);
#endif

TCCVideoRender *mRender;

TCCDxBSurface::TCCDxBSurface(int flag)
{
	mRender = new TCCVideoRender(flag);

	mWidth = mHeight = mFormat = mFlags = mOutMode = 0;
	mValidSurface = 1; //0:invalid, 1:valid

	pthread_mutex_init(&m_mutex, NULL);

#ifdef SUPPORT_SCALER_COPY
//	TCCDxB_ScalerOpen();
#endif
}

TCCDxBSurface::~TCCDxBSurface(void)
{
	ALOGV("%s", __func__);

	pthread_mutex_lock(&m_mutex);
	{
#ifdef SUPPORT_SCALER_COPY
//		TCCDxB_ScalerClose();
#endif

		delete mRender;

		mValidSurface = 0;
		mWidth = mHeight = mFormat = mFlags = mOutMode = 0;
	}
	pthread_mutex_unlock(&m_mutex);

	pthread_mutex_destroy(&m_mutex);
}

int TCCDxBSurface::SetVideoSurface(void *native)
{
	ALOGV("%s", __func__);

	pthread_mutex_lock(&m_mutex);
	{
		mRender->SetVideoSurface(native);
		if(mRender->SetNativeWindow(mWidth, mHeight, mFormat, mOutMode) != 0)
		{
			//mRender->ReleaseVideoSurface();
			mValidSurface = 0;
		}
		else
		{
			mValidSurface = 1;
		}
	}
	pthread_mutex_unlock(&m_mutex);

	return 0;
}

int TCCDxBSurface::UseSurface(void)
{
	ALOGV("%s", __func__);

	pthread_mutex_lock(&m_mutex);
	{
		mValidSurface = 1;
	}
	pthread_mutex_unlock(&m_mutex);

	return 0;
}

int TCCDxBSurface::ReleaseSurface(void)
{
	ALOGV("%s", __func__);

	pthread_mutex_lock(&m_mutex);
	{
		mRender->ReleaseVideoSurface();
		mValidSurface = 0;
	}
	pthread_mutex_unlock(&m_mutex);

	return 0;
}

int TCCDxBSurface::SetSurfaceParm(int width, int height, int format, int flags, int outmode)
{
	int ret;

	pthread_mutex_lock(&m_mutex);
	{
		ret = SetSurfaceParm_l(width, height, format, flags, outmode);
	}
	pthread_mutex_unlock(&m_mutex);

	return ret;
}

int TCCDxBSurface::SetSurfaceParm_l(int width, int height, int format, int flags, int outmode)
{
	ALOGV("%s - width=%d, height=%d, format=%d, flags=%d, outmode=%d", __func__, width, height, format, flags, outmode);

	mWidth = width;
	mHeight = height;
	mFormat = format;
	mFlags = flags; //0:interlaced , 1:progressive
	mOutMode = outmode; //0:YUV420 Interleave , 1:YUV420 sp
	mRender->SetNativeWindow(mWidth, mHeight, mFormat, mOutMode);

	return 0;
}

int TCCDxBSurface::WriteFrameBuf(PDISP_INFO pDispInfo)
{
	//ALOGV("%s", __func__);

	int ret;

	pthread_mutex_lock(&m_mutex);
	{
		if (pDispInfo != NULL)
		{
			memcpy(&mDispInfo, pDispInfo, sizeof(DISP_INFO));
		}
		ret = WriteFrameBuf_l();
	}
	pthread_mutex_unlock(&m_mutex);

	return ret;
}

int TCCDxBSurface::WriteFrameBuf_l()
{
	//ALOGV("%s", __func__);

	TCC_PLATFORM_PRIVATE_PMEM_INFO *frame_pmem_info = NULL;

	unsigned char* buf;

	if(mValidSurface == 0)
	{
		return -1;
	}

	if (mDispInfo.frame_width != mWidth || mDispInfo.frame_height != mHeight)
	{
		SetSurfaceParm_l(mDispInfo.frame_width, mDispInfo.frame_height, mFormat, mFlags, mOutMode);
	}

	if (mRender->dequeueBuffer(&buf, mDispInfo.frame_width, mDispInfo.frame_height, mDispInfo.pts) != 0)
	{
		return -2;
	}

	if (buf == NULL)
	{
		return -1;
	}

	if(mRender->IsVSyncEnabled() != 2)
	{
		frame_pmem_info = (TCC_PLATFORM_PRIVATE_PMEM_INFO *)mRender->GetPrivateAddr((int)buf, mDispInfo.frame_width, mDispInfo.frame_height);

		if(frame_pmem_info)
		{
			frame_pmem_info->copied = 1;

			/* mDispInfo.interlace is interlaced information 0:interlaced frame 1:Non-interlaced*/
			frame_pmem_info->optional_info[7] = (mDispInfo.interlace==0) ? 0x40000000 : 0x0;//OMX_BUFFERFLAG_INTERLACED_FRAME;
			frame_pmem_info->optional_info[4] = mDispInfo.unique_id; //Unique ID
			frame_pmem_info->optional_info[5] = mDispInfo.pts; //PTS
			frame_pmem_info->optional_info[6] = mDispInfo.stc; //STC
			frame_pmem_info->optional_info[8] = mDispInfo.frame_rate; //frame_rate
			if(mDispInfo.field_info == 0) // 0: top, 1: bottom
				frame_pmem_info->optional_info[7] |= 0x10000000;
			if(mDispInfo.pts == 0)
				frame_pmem_info->optional_info[7] |= 0x00000002; // Ignore pts
			if(mDispInfo.vsync_enable)
			{
				if(mDispInfo.frame_rate <= 60)
					frame_pmem_info->optional_info[7] |= 0x00000004; // Turn on deinterlace forcedly
			}
/*
			if(mDispInfo.unique_id == 1 && mDispInfo.first_frame_after_seek) //first frame only
				frame_pmem_info->optional_info[7] |= 0x00000001; // Turn on first frame after seek
*/
			if(mDispInfo.bypass_clear_vsync == 1)
				frame_pmem_info->optional_info[7] |= 0x80000000;	//for seamless change. for not working clear vsync.

			frame_pmem_info->unique_addr = mDispInfo.unique_addr;

			memcpy(frame_pmem_info->offset, mDispInfo.frame_addr, sizeof(int)*3);

			frame_pmem_info->width = mDispInfo.frame_width;
			frame_pmem_info->height = mDispInfo.frame_height;
			if(!mFormat)
				frame_pmem_info->format = OMX_COLOR_FormatYUV420SemiPlanar;
			else
				frame_pmem_info->format = OMX_COLOR_FormatYUV420Planar;
			sprintf((char*)frame_pmem_info->name, "video");
			frame_pmem_info->name[5] = 0;
			frame_pmem_info->optional_info[0] = mDispInfo.frame_width;
			frame_pmem_info->optional_info[1] = mDispInfo.frame_height;
			frame_pmem_info->optional_info[2] = mDispInfo.frame_width;
			frame_pmem_info->optional_info[3] = mDispInfo.frame_height;
			frame_pmem_info->optional_info[9] = 0;
			frame_pmem_info->optional_info[10] = 0;
			frame_pmem_info->optional_info[11] = 0;
			frame_pmem_info->optional_info[12] = 0;
			frame_pmem_info->optional_info[13] = mDispInfo.vsync_enable; // vsync enable
			frame_pmem_info->optional_info[14] = mDispInfo.display_index;
			frame_pmem_info->optional_info[15] = mDispInfo.vpu_index;
		}
	}
	else
	{
		ALOGV("Display Usi mali");
		mDispInfo.vsync_enable = 0;
	}

	if(mDispInfo.vsync_enable)
	{
		frame_pmem_info->copied = 0;
	}
	else
	{
#ifdef SUPPORT_SCALER_COPY
/*
		TCCDxB_ScalerCopyData(	mDispInfo.frame_width,
								mDispInfo.frame_height,
								(unsigned char*)mDispInfo.frame_addr[0],  // YSrc
								(unsigned char*)mDispInfo.frame_addr[1],  // USrc
								(unsigned char*)mDispInfo.frame_addr[2],  // VSrc
								mDispInfo.format,                 // bSrcYUVInter
								buf,                        // addrDst
								(unsigned char)1,           // ignoreAligne
								mDispInfo.field_info,                  // fieldInfomation
								1);                         // interlaceMode(0: deinterlacing, 1: bypass )
*/
#endif								
	}

	mRender->enqueueBuffer(buf, mDispInfo.frame_width, mDispInfo.frame_height, mDispInfo.pts);

	return 0;
}

#if 0   // sunny : not use
int TCCDxBSurface::CaptureVideoFrame(char *strFilePath)
{
	ALOGV("%s", __func__);

#ifdef SUPPORT_SCALER_COPY
	int ret;

	pthread_mutex_lock(&m_mutex);
	{
		ret = TCCDxB_CaptureImage(mDispInfo.frame_width, mDispInfo.frame_height, mDispInfo.frame_addr, 
									mDispInfo.format, mDispInfo.interlace, mDispInfo.field_info, (unsigned char*)strFilePath,
									mCropLeft, mCropTop, mCropRight, mCropBottom);
	}
	pthread_mutex_unlock(&m_mutex);
#endif
	return 0;
}
#endif

void TCCDxBSurface::initNativeWindowCrop(int left, int top, int width, int height)
{
	ALOGV("%s %d X %d(%d, %d)", __func__, width, height, left, top);
	mCropLeft = left;
	mCropTop = top;
	mCropRight = width;
	mCropBottom = height;
	mRender->setCrop(left, top, width, height);
}

int TCCDxBSurface::IsVSyncEnabled()
{
	return mRender->IsVSyncEnabled();
}

