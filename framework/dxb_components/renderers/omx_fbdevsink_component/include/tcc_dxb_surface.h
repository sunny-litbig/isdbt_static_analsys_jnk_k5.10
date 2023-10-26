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
#ifndef _TCC_DXB_SURFACE_H_
#define _TCC_DXB_SURFACE_H_

typedef struct
{
	int frame_addr[3]; // yuv
	int frame_width;
	int frame_height;
	int frame_rate;
	int field_info; // 0: top, 1: bottom
	int unique_id;
	int pts;
	int stc;
	int interlace; // 0: interlace, 1: non-interlace
	int vsync_enable; // 0: disable, 1:enable
	int unique_addr;
	int display_index;
	int vpu_index;
	int format;
	int first_frame_after_seek;
	int bypass_clear_vsync;
} DISP_INFO, *PDISP_INFO;

class TCCDxBSurface
{
public:
	TCCDxBSurface(int flag);
	~TCCDxBSurface();

	int SetVideoSurface(void *native);
	int UseSurface(void);
	int ReleaseSurface(void);
	int SetSurfaceParm(int width, int height, int format, int flags, int outmode);
	int WriteFrameBuf(PDISP_INFO pDispInfo = NULL);
//	int CaptureVideoFrame(char *strFilePath);
	void initNativeWindowCrop(int left, int top, int width, int height);
	int IsVSyncEnabled();

private:
	int SetSurfaceParm_l(int width, int height, int format, int flags, int outmode);
	int WriteFrameBuf_l();

	int mWidth;
	int mHeight;
	int mFormat;
	int mFlags;
	int mOutMode;
	DISP_INFO mDispInfo;

	int            mValidSurface;
	int mCropLeft;
	int mCropRight;
	int mCropTop;
	int mCropBottom;

	pthread_mutex_t m_mutex;
};

#endif//_TCC_DXB_SURFACE_H_

