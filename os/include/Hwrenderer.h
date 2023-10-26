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
#ifdef HAVE_LINUX_PLATFORM
#ifndef uint32_t
typedef unsigned int uint32_t;
#endif
#endif

//#define USE_OUTPUT_COMPONENT_COMPOSITE 
#ifndef ADDRESS_ALIGNED
#define ADDRESS_ALIGNED
#define ALIGN_BIT (0x8-1)
#define BIT_0 3
#define GET_ADDR_YUV42X_spY(Base_addr)          (((((unsigned int)Base_addr) + ALIGN_BIT)>> BIT_0)<<BIT_0)
#define GET_ADDR_YUV42X_spU(Yaddr, x, y)                (((((unsigned int)Yaddr+(x*y)) + ALIGN_BIT)>> BIT_0)<<BIT_0)
#define GET_ADDR_YUV422_spV(Uaddr, x, y)                (((((unsigned int)Uaddr+(x*y/2)) + ALIGN_BIT) >> BIT_0)<<BIT_0)
#define GET_ADDR_YUV420_spV(Uaddr, x, y)                (((((unsigned int)Uaddr+(x*y/4)) + ALIGN_BIT) >> BIT_0)<<BIT_0)
#endif

typedef enum{
    TCC_LCDC_IMG_FMT_1BPP,
    TCC_LCDC_IMG_FMT_2BPP,
    TCC_LCDC_IMG_FMT_4BPP,
    TCC_LCDC_IMG_FMT_8BPP,
    TCC_LCDC_IMG_FMT_RGB332 = 8,
    TCC_LCDC_IMG_FMT_RGB444 = 9,
    TCC_LCDC_IMG_FMT_RGB565 = 10,
    TCC_LCDC_IMG_FMT_RGB555 = 11,
    TCC_LCDC_IMG_FMT_RGB888 = 12,
    TCC_LCDC_IMG_FMT_RGB666 = 13,
    TCC_LCDC_IMG_FMT_RGB888_3   = 14,       /* RGB888 - 3bytes aligned - B1[31:24],R0[23:16],G0[15:8],B0[7:0] newly supported : 3 bytes format*/
    TCC_LCDC_IMG_FMT_ARGB6666_3 = 15,       /* ARGB6666 - 3bytes aligned - A[23:18],R[17:12],G[11:6],B[5:0]newly supported : 3 bytes format */
    TCC_LCDC_IMG_FMT_COMP = 16,             // 4bytes
    TCC_LCDC_IMG_FMT_DECOMP = (TCC_LCDC_IMG_FMT_COMP),
    TCC_LCDC_IMG_FMT_444SEP = 21,
    TCC_LCDC_IMG_FMT_UYVY = 22,     /* 2bytes   : YCbCr 4:2:2 Sequential format LSB [Y/U/Y/V] MSB : newly supported : 2 bytes format*/
    TCC_LCDC_IMG_FMT_VYUY = 23,     /* 2bytes   : YCbCr 4:2:2 Sequential format LSB [Y/V/Y/U] MSB : newly supported : 2 bytes format*/

    TCC_LCDC_IMG_FMT_YUV420SP = 24,
    TCC_LCDC_IMG_FMT_YUV422SP = 25,
    TCC_LCDC_IMG_FMT_YUYV = 26,
    TCC_LCDC_IMG_FMT_YVYU = 27,

    TCC_LCDC_IMG_FMT_YUV420ITL0 = 28,
    TCC_LCDC_IMG_FMT_YUV420ITL1 = 29,
    TCC_LCDC_IMG_FMT_YUV422ITL0 = 30,
    TCC_LCDC_IMG_FMT_YUV422ITL1 = 31,
    TCC_LCDC_IMG_FMT_MAX
}TCC_LCDC_IMG_FMT_TYPE;

#include <tcc_grp_ioctrl.h>
#include <tcc_scaler_ioctrl.h>
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE 
#include <tcc_composite_ioctl.h>
#include <tcc_component_ioctl.h>
#endif
#include <tcc_overlay_ioctl.h>
#include <tccfb_ioctrl.h>
#include <tcc_viqe_ioctl.h>
#include <tcc_mem_ioctl.h>

#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
#include <tcc_vsync_ioctl.h>
#endif

#include <libpmap/pmap.h>
#include <stdlib.h>
#include <linux/fb.h>
#include <tcc_video_private.h>

#if defined(HAVE_LINUX_PLATFORM)
#ifndef uint8_t
typedef unsigned char uint8_t;
#endif
#endif

#define OVERLAY_0_DEVICE	"/dev/overlay"
#define OVERLAY_1_DEVICE	"/dev/overlay1"
#define GRAPHIC_DEVICE		"/dev/graphic"
#define SCALER0_DEVICE		"/dev/scaler1"
#define SCALER1_DEVICE		"/dev/scaler3"
#define FB0_DEVICE			"/dev/tcc_vsync0"
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE 
#define COMPOSITE_DEVICE 	"/dev/composite"
#define COMPONENT_DEVICE 	"/dev/component"
#endif
#define VIQE_DEVICE			"/dev/viqe"
#define TMEM_DEVICE			"/dev/tmem"
#define CLK_CTRL_DEVICE	"/dev/clockctrl"
#define HDMI_DRVICE			"/dev/hdmi"
#define HDMI_CEC_DEVICE		"/dev/cec"
#define HDMI_HPD_DEVICE		"/dev/hpd"

#define VIDEO_PLAY_SCALER			SCALER0_DEVICE
#define VIDEO_CAPTURE_SCALER 		SCALER1_DEVICE
#define VSYNC_OVERLAY_COUNT 6
#define HDMI_DEC_MAX_WIDTH	1024
#define HDMI_DEC_MAX_HEIGHT	600
#define LCDC_VIDEO_CHANNEL		3
#define RET_RETRY_THIS_FRAME 10
#define TCC_CLOCKCTRL_HWC_DDI_CLOCK_DISABLE 116
#define TCC_CLOCKCTRL_HWC_DDI_CLOCK_ENABLE 117

typedef struct
{
	unsigned int YaddrPhy;
	unsigned int UaddrPhy;
	unsigned int VaddrPhy;
#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
	unsigned int timeStamp;
	unsigned int curTime;
	unsigned int buffer_id;
#endif
	unsigned int flags;
	unsigned int frameView;

	unsigned int MVC_Base_YaddrPhy;
	unsigned int MVC_Base_UaddrPhy;
	unsigned int MVC_Base_VaddrPhy;

}output_overlay_param_t;

typedef enum{
	OUTPUT_LCDC_0,
	OUTPUT_LCDC_1,
	OUTPUT_LCDC_MAX
}TCC_OUTPUT_LCDC_TYPE;

typedef struct
{
	unsigned char lcdc;
	unsigned char mode;
} TCC_OUTPUT_START_TYPE;

typedef struct
{
	unsigned int sx;
	unsigned int sy;
	unsigned int width;
	unsigned int height;
	unsigned int format;
	unsigned int transform;
}pos_info_t;

typedef struct
{
	unsigned int sx;
	unsigned int sy;
	unsigned int srcW;
	unsigned int srcH;
	unsigned int dstW;
	unsigned int dstH;
}crop_info_t;


typedef struct
{
	unsigned int left;
	unsigned int top;
	unsigned int right;
	unsigned int bottom;
}region_info_t;

typedef struct
{
	region_info_t source;
	region_info_t display;
}region_info_detail_t;

typedef struct
{
	unsigned int	mScreenModeIndex;
	bool			mScreenMode;
	int				mOutputMode; //0 : lcd , 1 : external display , 2 : dual(lcd  + external display)
	bool			mHDMIOutput;
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE 
	bool			mCompositeOutput;
	bool			mComponentOutput;
	unsigned int	mCompositeMode;
	unsigned int	mComponentMode;
#endif
	//video display mode related with vsync
	int 			mVsyncMode;
	bool			mVSyncSelected;
	unsigned int	mUnique_address;
	// video display mode check onthefly
	int				mOntheflymode;
	//video display deinterlace mode
	int 			mVideoDeinterlaceFlag;
	int             mVideoDeinterlaceScaleUpOneFieldFlag;
	int 			mVideoDeinterlaceSupport;
	int 			mVideoDeinterlaceForce;
	int 			mVideoDeinterlaceToMemory;

	//Variable
	int				mOverlay_fd;
	int				mOverlay_ch;
	int 			mRt_fd;
	int 			mM2m_fd;
	int 			mFb_fd;
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE 
	int 			mComposite_fd;
	int 			mComponent_fd;
#endif
//	int 			mJpegEnc_fd;
//	int 			mJpegDec_fd;
	int 			mViqe_fd;
	bool			bUseMemInfo;
	int				mClkctrl_fd;
	int				mTmem_fd;
	void* 			mTMapInfo;
	pmap_t			mUmpReservedPmap;

	unsigned int 	mLcd_width;
	unsigned int 	mLcd_height;

	unsigned int 	mOutput_width;
	unsigned int 	mOutput_height;

    /*for stream original aspect ratio
     *if -1(not define), aspect ratio is ratio for image wdith and height.
     */
	int 			mStreamAspectRatio; //-1:not define 0:16:9, 1:4:3

	//Overlay
	bool			mOverlay_recheck;
	int 			mOverlayBuffer_Cnt; // Total Count!!
	int 			mOverlayBuffer_Current;
	unsigned int			mOverlayBuffer_PhyAddr[6];
	//Cropped region info.
	bool			mCropRgn_changed;
	bool			mCropChanged;
	bool			mSkipCropChanged;
	bool			mAdditionalCropSource;
	region_info_detail_t		mOrigin_RegionInfo;
	crop_info_t 	mTarget_CropInfo;

	unsigned int	mDispWidth; // Scaler OUT!!
	unsigned int	mDispHeight;
	unsigned int	mFinal_DPWidth; //Crop OUT!!
	unsigned int	mFinal_DPHeight;
	unsigned int	mFinal_stride;
	unsigned int	mRight_truncation;

	//Overlay :: Calculated Real_target to display!!
	pos_info_t		mOrigin_DispInfo;
	pos_info_t		mTarget_DispInfo;

	pos_info_t		mScreenMode_DispInfo;

	bool			mFirst_Frame;
	bool			mParallel;
	bool			mOrder_Parallel;
	SCALER_TYPE 	mScaler_arg;
	G2D_COMMON_TYPE mGrp_arg;
	bool			mNeed_scaler;
	bool			mNeed_transform;
	bool			mNeed_rotate;
	bool			mNeed_crop;
	unsigned int 	mTransform;
	unsigned int	mFrame_cnt;

	pmap_t			mOverlayPmap;
	pmap_t			mOverlayRotation_1st;
	pmap_t			mOverlayRotation_2nd;
	pmap_t			mDualDisplayPmap;

	bool 			mExclusive_Enable;
	bool 			mExclusive_Display;
	char			*mHDMIUiSize;
	int				mDisplayOutputSTB;
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE	
	char			*mComponentChip;
#endif
	unsigned int 	mBackupOutputModeIndex;
	unsigned int 	mOutputModeIndex;

	VIQE_DI_TYPE	mViqe_arg;

	bool			mVIQE_onoff;
	bool			mVIQE_onthefly;
	bool			mVIQE_DI_use;
	bool			mVIQE_OddFirst;
	int 			mVIQE_firstfrm;
	int 			mVIQE_m2mDICondition;
	unsigned int	mPrevY;
	unsigned int	mPrevU;
	unsigned int	mPrevV;
	unsigned int	mFrame_rate;
	unsigned int	mOriginalHDMIResolution;
	bool			mChangedHDMIResolution;
	bool			mMVCmode;

	unsigned int	mMode_3Dui;

	struct tcc_lcdc_image_update extend_display;

	bool mIgnore_ratio;
	bool bShowFps;
	bool bSupport_lastframe;

	int mIdx_codecInstance;

	TCC_PLATFORM_PRIVATE_PMEM_INFO private_data;
	bool bHWaddr;
	bool bExtract_data;
	bool bCheckCrop;
	bool bCropChecked;
}render_param_t;


class HwRenderer
{
public:
	HwRenderer(
				int overlay_ch,
				int inWidth, int inHeight, int inFormat,
				int outLeft, int outTop, int outRight, int outBottom, unsigned int transform,
				int creation_from,
				int numOfExternalVideo);
	~HwRenderer();
	bool initCheck() const { return mInitCheck; }

	int setCropRegion(int fd_0, char* ghandle, int inWidth, int inHeight, int srcLeft, int srcTop, int srcRight, int srcBottom, int outLeft, int outTop, int outRight, int outBottom, unsigned int transform);
	int render(int fd_0, int inWidth, int inHeight, int outLeft, int outTop, int outRight, int outBottom, unsigned int transform);
	void setNumberOfExternalVideo( int numOfExternalVideo, bool isPrepare);
	void setFBChannel( int numOLayers, bool isReset);

protected:
	int _openOverlay(void);
	int _closeOverlay(unsigned int onoff);
	int _ioctlOverlay(int cmd, overlay_video_buffer_t* arg);
	int initDevice(int inWidth, int inHeight);
	void closeDevice();
	int getDisplaySize(OUTPUT_SELECT Output, tcc_display_size *display_size);
	int getfromsysfs(const char *sysfspath, char *val);
	int VIQEProcess(unsigned int PA_Y, unsigned int PA_U, unsigned int PA_V, unsigned int DestAddr);

	bool (HwRenderer::*OverlayCheckDisplayFunction)(bool, int, int, int, int, unsigned int);
	bool (HwRenderer::*OverlayCheckExternalDisplayFunction)(bool, int, int, int, int, unsigned int);

	int overlayInit(SCALER_TYPE *scaler_arg, G2D_COMMON_TYPE *grp_arg, uint8_t isOutRGB);
	bool overlayCheck(bool reCheck, int outSx, int outSy, int outWidth, int outHeight, unsigned int rotation);
	int overlayResizeExternalDisplay(OUTPUT_SELECT type, int flag);
	int overlayResizeGetPosition(OUTPUT_SELECT type, int flag);
	bool overlayCheckExternalDisplay(bool reCheck, int outSx, int outSy, int outWidth, int outHeight, unsigned int rotation);
	bool overlayCheckDisplayScreenMode(bool reCheck, int outSx, int outSy, int outWidth, int outHeight, unsigned int rotation);
	bool overlayCheckExternalDisplayScreenMode(bool reCheck, int outSx, int outSy, int outWidth, int outHeight, unsigned int rotation);
	void overlayScreenModeCalculate(float fCurrentCheckDisp, bool bCut);

	int overlayProcess(SCALER_TYPE *scaler_arg, G2D_COMMON_TYPE *grp_arg, unsigned int srcY, unsigned int srcU, unsigned int srcV, unsigned int dest);
	int overlayProcess_Parallel(SCALER_TYPE *scaler_arg, G2D_COMMON_TYPE *grp_arg, unsigned int srcY, unsigned int srcU, unsigned int srcV, uint8_t *dest);
	bool get_private_info(TCC_PLATFORM_PRIVATE_PMEM_INFO *info, char* grhandle, bool isHWaddr, int fd_val, unsigned int width, unsigned int height);
	bool output_change_check(int outSx, int outSy, int outWidth, int outHeight, unsigned int rotation, bool *overlayCheckRet, bool interlaced);
	bool output_Overlay(output_overlay_param_t * pOverlayInfo, bool *overlayCheckRet);

	void setVsync();
	void clearVsync(bool able_use_vsync, bool prev_onthefly_mode, bool use_lastframe);
	void setting_mode();

	void Set_MVC_Mode(TCC_PLATFORM_PRIVATE_PMEM_INFO* pPrivate_data, int inWidth, int inHeight, int outWidth, int outHeight);
	void Native_FramerateMode(TCC_PLATFORM_PRIVATE_PMEM_INFO* pPrivate_data, int inWidth, int inHeight, int outWidth, int outHeight);
	int digits10(unsigned int num);
	int intToStr(int num, int sign, char *buffer);
	bool check_valid_hw_availble(int fd_val);

private:
	bool mInitCheck;
	render_param_t *pMParm;
	unsigned int mDecodedWidth, mDecodedHeight;
	unsigned int mOutputWidth, mOutputHeight;
	unsigned int mInputWidth, mInputHeight;
	unsigned int mBox;
	unsigned int mCheckSync;
	int mNumberOfExternalVideo;
	int mCheckEnabledTCCSurface;
	int mVideoCount;
	bool mIsFBChannelOn;

	int mColorFormat;
	int mGColorFormat;
	int mBufferId;
	int mNoSkipBufferId;

	HwRenderer(const HwRenderer &);
	HwRenderer &operator=(const HwRenderer &);
};
