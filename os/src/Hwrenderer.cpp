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
#define LOG_TAG	"HWRENDERER"
#include <utils/Log.h>

#include "Hwrenderer.h"

#include <fcntl.h>
#include <sys/poll.h>
#include <sys/mman.h>
#include <errno.h>
#include <linux/videodev2.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "TCCMemory.h"
#ifndef TCC_LCDC_HDMI_LASTFRAME
#define TCC_LCDC_HDMI_LASTFRAME TCC_LCDC_VIDEO_KEEP_LASTFRAME
#endif

typedef enum OMX_COLOR_FORMATTYPE {
    OMX_COLOR_FormatUnused,
    OMX_COLOR_FormatMonochrome,
    OMX_COLOR_Format8bitRGB332,
    OMX_COLOR_Format12bitRGB444,
    OMX_COLOR_Format16bitARGB4444,
    OMX_COLOR_Format16bitARGB1555,
    OMX_COLOR_Format16bitRGB565,
    OMX_COLOR_Format16bitBGR565,
    OMX_COLOR_Format18bitRGB666,
    OMX_COLOR_Format18bitARGB1665,
    OMX_COLOR_Format19bitARGB1666,
    OMX_COLOR_Format24bitRGB888,
    OMX_COLOR_Format24bitBGR888,
    OMX_COLOR_Format24bitARGB1887,
    OMX_COLOR_Format25bitARGB1888,
    OMX_COLOR_Format32bitBGRA8888,
    OMX_COLOR_Format32bitARGB8888,
    OMX_COLOR_FormatYUV411Planar,
    OMX_COLOR_FormatYUV411PackedPlanar,
    OMX_COLOR_FormatYUV420Planar,
    OMX_COLOR_FormatYUV420PackedPlanar,
    OMX_COLOR_FormatYUV420SemiPlanar,
    OMX_COLOR_FormatYUV422Planar,
    OMX_COLOR_FormatYUV422PackedPlanar,
    OMX_COLOR_FormatYUV422SemiPlanar,
    OMX_COLOR_FormatYCbYCr,
    OMX_COLOR_FormatYCrYCb,
    OMX_COLOR_FormatCbYCrY,
    OMX_COLOR_FormatCrYCbY,
    OMX_COLOR_FormatYUV444Interleaved,
    OMX_COLOR_FormatRawBayer8bit,
    OMX_COLOR_FormatRawBayer10bit,
    OMX_COLOR_FormatRawBayer8bitcompressed,
    OMX_COLOR_FormatL2,
    OMX_COLOR_FormatL4,
    OMX_COLOR_FormatL8,
    OMX_COLOR_FormatL16,
    OMX_COLOR_FormatL24,
    OMX_COLOR_FormatL32,
    OMX_COLOR_FormatYUV420PackedSemiPlanar,
    OMX_COLOR_FormatYUV422PackedSemiPlanar,
    OMX_COLOR_Format18BitBGR666,
    OMX_COLOR_Format24BitARGB6666,
    OMX_COLOR_Format24BitABGR6666,
    OMX_COLOR_FormatKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */
    OMX_COLOR_FormatVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_COLOR_FormatMax = 0x7FFFFFFF
} OMX_COLOR_FORMATTYPE;

/* system/core/include/system/graphics.h */
enum {
	HAL_PIXEL_FORMAT_YV12   = 0x32315659, // YCrCb 4:2:0 Planar
	HAL_PIXEL_FORMAT_YCbCr_420_SP  = HAL_PIXEL_FORMAT_YV12 + 0x1, // Telechips, ZzaU
};

enum {
    /* flip source image horizontally (around the vertical axis) */
    HAL_TRANSFORM_FLIP_H    = 0x01,
    /* flip source image vertically (around the horizontal axis)*/
    HAL_TRANSFORM_FLIP_V    = 0x02,
    /* rotate source image 90 degrees clockwise */
    HAL_TRANSFORM_ROT_90    = 0x04,
    /* rotate source image 180 degrees */
    HAL_TRANSFORM_ROT_180   = 0x03,
    /* rotate source image 270 degrees clockwise */
    HAL_TRANSFORM_ROT_270   = 0x07,
    /* don't use. see system/window.h */
    HAL_TRANSFORM_RESERVED  = 0x08,
};

/* hardware/libhardware/include/hardware/hwcomposer_defs.h */
enum {
    /* flip source image horizontally */
    HWC_TRANSFORM_FLIP_H = HAL_TRANSFORM_FLIP_H,
    /* flip source image vertically */
    HWC_TRANSFORM_FLIP_V = HAL_TRANSFORM_FLIP_V,
    /* rotate source image 90 degrees clock-wise */
    HWC_TRANSFORM_ROT_90 = HAL_TRANSFORM_ROT_90,
    /* rotate source image 180 degrees */
    HWC_TRANSFORM_ROT_180 = HAL_TRANSFORM_ROT_180,
    /* rotate source image 270 degrees clock-wise */
    HWC_TRANSFORM_ROT_270 = HAL_TRANSFORM_ROT_270,
};

#define PROPERTY_KEY_MAX   32
#define PROPERTY_VALUE_MAX  92

typedef struct
{
	char key[PROPERTY_KEY_MAX];
	char value[PROPERTY_VALUE_MAX];
}_PROPERTY_LIST_;

_PROPERTY_LIST_ property_list[] =
{
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE
	{"persist.sys.component_mode", "0"},
	{"persist.sys.composite_mode", "0"},
#endif
	{"persist.sys.display.mode", "0"},
	{"persist.sys.hdmi_resolution", "125"},
	{"persist.sys.renderer_onthefly", "true"},
	{"ro.board.platform", "tcc893x"},
	{"tcc.check.hdmi_hpd", ""},
	{"tcc.component.chip", "0"},
	{"tcc.display.output.stb", "0"},
	{"tcc.hdmi.uisize", "0"},
	{"tcc.media.wfd.sink.run", "0"},
	{"tcc.show.video.fps", "0"},
	{"tcc.solution.mbox", "0"},
	{"tcc.solution.preview", "0"},
	{"tcc.solution.use.screenmode", "0"}, //output display: 0->1
	{"tcc.sys.output_mode_detected", "0"}, //output display: 0->1
	{"tcc.sys.output_mode_plugout", "0"},
	{"tcc.video.check.rightCrop", "0"},
	{"tcc.video.deinterlace.force", "0"},
	{"tcc.video.deinterlace.support", "0"},
	{"tcc.video.hdmi_resolution", "0"},
	{"tcc.video.hwr.id", "0"},
	{"tcc.video.surface.enable", "0"},
	{"tcc.video.surface.support", "0"},
	{"tcc.video.vsync.enable", "0"},
	{"tcc.video.vsync.support", "1"},
	{"tcc.wfd.vsync.disable", "0"},
	{"tcc.dxb.fullscreen", "0"}, //output full display: 0->1
};

int property_set(const char *key, const char *value)
{
	if(key == NULL || value == NULL)
		return -1;

	int i;
	for(i = 0; i < sizeof(property_list)/sizeof(property_list[0]); i++)
	{
		if(strcmp(key, property_list[i].key) == 0)
		{
			memset(property_list[i].value, 0, sizeof(PROPERTY_VALUE_MAX));
			memcpy(property_list[i].value, value, strlen(value));
			//printf("property_set - %s : %s\n", property_list[i].key, property_list[i].value);
			return strlen(value);
		}
	}

	return -1;
}

int property_get(const char *key, char *value, const char *default_value)
{
	int i, len;

	for(i = 0; i < sizeof(property_list)/sizeof(property_list[0]); i++)
	{
		if(strcmp(key, property_list[i].key) == 0)
		{
			len = strlen(property_list[i].value);
			memcpy(value, property_list[i].value, len + 1);
			//printf("property_get - %s : %s\n", property_list[i].key, property_list[i].value);
			return len;
		}
	}

	value[0] = 0;
	len = 0;

	if(default_value) {
		len = strlen(default_value);
		memcpy(value, default_value, len + 1);
	}

	return len;
}


/************************************************************************************************/
static int DEBUG_ON = 0;
#define DBUG(msg...)	if (DEBUG_ON) { ALOGD( "Render :: " msg); }
#define DBUG_FRM(msg...) //ALOGD( "Render :: " msg); //per every frame

#define LOGD(x...)    ALOGD(x)
#define LOGE(x...)    ALOGE(x)
#define LOGI(x...)    ALOGI(x)
#define LOGV(x...)    ALOGV(x)

//#define DEBUG_TIME_LOG
//#define DEBUG_PROCTIME_LOG
#if defined(DEBUG_TIME_LOG) || defined(DEBUG_PROCTIME_LOG)
#include "time.h"
static unsigned int dec_time[30] = {0,};
static unsigned int time_cnt = 0;
static unsigned int total_dec_time = 0;
static clock_t clk_prev;
#endif

#define ALIGNED_BUFF(buf, mul) ( ( (unsigned int)buf + (mul-1) ) & ~(mul-1) )
/************************************************************************************************/
int HwRenderer::_openOverlay(void)
{
	pos_info_t config;
	int ret;

	if(pMParm->mOverlay_fd >= 0)
		return 0;

	if(pMParm->mOverlay_ch == 0){
		if((pMParm->mOverlay_fd = open(OVERLAY_0_DEVICE, O_RDWR)) < 0){
			LOGE("can't open overlay engine device '%s'", OVERLAY_0_DEVICE);
			return -1;
		}
	}
	else{
		if((pMParm->mOverlay_fd = open(OVERLAY_1_DEVICE, O_RDWR)) < 0){
			LOGE("can't open overlay engine device '%s'", OVERLAY_1_DEVICE);
			return -1;
		}
	}

	config.sx         = pMParm->mTarget_DispInfo.sx;
	config.sy         = pMParm->mTarget_DispInfo.sy;
	config.width      = pMParm->mTarget_DispInfo.width;
	config.height     = pMParm->mTarget_DispInfo.height;
	config.format     = V4L2_PIX_FMT_YUV422P; //420sequential

	if((ret = ioctl(pMParm->mOverlay_fd, OVERLAY_SET_CONFIGURE, &config)) < 0){
		LOGE("Error: pMParm->mOverlay_fd(cmd %d)", OVERLAY_SET_CONFIGURE);
	}

	DBUG("Overlay is opened.");
	return 0;
}

int HwRenderer::_closeOverlay(unsigned int onoff)
{
	if(pMParm->mOverlay_fd != -1){
		if(close(pMParm->mOverlay_fd) < 0)
		{
			LOGE("Error: close(pMParm->mOverlay_fd)");
		}
		pMParm->mOverlay_fd = -1;
	}

	DBUG("Overlay is closed.");
	return 0;
}

int HwRenderer::_ioctlOverlay(int cmd, overlay_video_buffer_t* arg)
{
	int ret = 0;

	_openOverlay();

	if(pMParm->mOverlay_fd < 0){
		DBUG("Overlay was not opened.");
		return -1;
	}

	if((ret = ioctl(pMParm->mOverlay_fd, cmd, arg)) < 0){
		LOGE("Error: pMParm->mOverlay_fd(cmd %d)", cmd);
	}

	return ret;
}

/************************************************************************************************/
HwRenderer::HwRenderer(
	int overlay_ch,
	int inWidth, int inHeight, int inFormat,
	int outLeft, int outTop, int outRight, int outBottom, unsigned int transform,
	int creation_from_external,
	int numOfExternalVideo
)
{
	int err;
	char value[PROPERTY_VALUE_MAX];
	int outSx, outSy, outWidth, outHeight;

	mInitCheck = false;
	inHeight = (inHeight>>1)<<1;

	if(numOfExternalVideo > 1)
		property_set("tcc.video.multidecoding.check", "1");

	if(initDevice(inWidth, inHeight) < 0)
		return;

	property_get("tcc.video.check.rightCrop", value, "");
	pMParm->bCheckCrop = atoi(value);
	pMParm->bCropChecked = false;
	if( !pMParm->bCheckCrop )
		pMParm->bCropChecked = true;

	mInputWidth = mDecodedWidth   = inWidth;
	mInputHeight = mDecodedHeight  = inHeight;
	pMParm->mTransform = transform;

	if ( inFormat == HAL_PIXEL_FORMAT_YV12 )
		mGColorFormat = mColorFormat = OMX_COLOR_FormatYUV420Planar;
	else
		mGColorFormat = mColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;

	property_get("tcc.display.output.stb", value, "0");
	bool DisplayOutputSTB = atoi(value);

	if(DisplayOutputSTB)
		pMParm->mIgnore_ratio = true;
	else
		pMParm->mIgnore_ratio = false;

	if(creation_from_external)
	{
		tcc_display_size display_size;

		getDisplaySize(OUTPUT_HDMI, &display_size);

		pMParm->mLcd_width = ( display_size.width > pMParm->mLcd_width ) ? pMParm->mLcd_width : display_size.width;
		pMParm->mLcd_height = ( display_size.height > pMParm->mLcd_height ) ? pMParm->mLcd_height : display_size.height;
	}

	if(outLeft <= 0){
		outSx = 0;
		outWidth = (pMParm->mLcd_width < outRight) ? pMParm->mLcd_width : outRight;
	}
	else{
		outSx = outLeft;
		outWidth = (pMParm->mLcd_width < outRight) ? (pMParm->mLcd_width - outLeft) : (outRight - outLeft);
	}

	if(outTop <= 0){
		outSy = 0;
		outHeight= (pMParm->mLcd_height < outBottom) ? pMParm->mLcd_height : outBottom;
	}
	else{
		outSy = outTop;
		outHeight= (pMParm->mLcd_height < outBottom) ? (pMParm->mLcd_height - outTop) : (outBottom - outTop);
	}

	pMParm->mOverlay_ch= overlay_ch;
	if(pMParm->mOverlay_ch == 0)
		pmap_get_info("overlay", &pMParm->mOverlayPmap);
	else
		pmap_get_info("overlay1", &pMParm->mOverlayPmap);
/*
	pmap_get_info("overlay_rot", &pMParm->mOverlayRotation_1st);
	pMParm->mOverlayRotation_1st.size = pMParm->mOverlayRotation_1st.size/2;
	pMParm->mOverlayRotation_2nd.base = pMParm->mOverlayRotation_1st.base + pMParm->mOverlayRotation_1st.size;
	pMParm->mOverlayRotation_2nd.size = pMParm->mOverlayRotation_1st.size;
	LOGI("overlay_rot: 0x%x - 0x%x, 0x%x-0x%x", pMParm->mOverlayRotation_1st.base, pMParm->mOverlayRotation_1st.size, pMParm->mOverlayRotation_2nd.base, pMParm->mOverlayRotation_2nd.size);

	pmap_get_info("video_dual", &pMParm->mDualDisplayPmap);
*/
	unsigned int uiLen = 0, uiMode = 0;
	uiLen = property_get ("tcc.solution.use.screenmode", value, "");

	if(uiLen){
		uiMode = atoi(value);
		LOGD("property_get[tcc.solution.use.screenmode]::%d", uiMode);
		switch(uiMode)
		{
			case 1:
				pMParm->mScreenMode = true;
				OverlayCheckDisplayFunction = &HwRenderer::overlayCheckDisplayScreenMode;
				OverlayCheckExternalDisplayFunction = &HwRenderer::overlayCheckExternalDisplayScreenMode;
				break;

			default:
				pMParm->mScreenMode = false;
				OverlayCheckDisplayFunction = &HwRenderer::overlayCheck;
				OverlayCheckExternalDisplayFunction = &HwRenderer::overlayCheckExternalDisplay;
				break;
		}
	}
	else{
		LOGD("property_get[tcc.solution.use.screenmode]::%d", uiMode);
		property_set("tcc.solution.use.screenmode", "0");
		pMParm->mScreenMode = false;
		OverlayCheckDisplayFunction = &HwRenderer::overlayCheck;
		OverlayCheckExternalDisplayFunction = &HwRenderer::overlayCheckExternalDisplay;
	}

	property_get ("tcc.solution.mbox", value, "");
	mBox =  atoi(value);

	pMParm->bSupport_lastframe = true;

#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
	if( mBox && ioctl( pMParm->mFb_fd, TCC_LCDC_VIDEO_GET_LASTFRAME_STATUS, NULL) < 0 ) {
		LOGI("will not use last-frame feature.");
		pMParm->bSupport_lastframe = false;
	}
#endif

	mOutputWidth	= outWidth;
	mOutputHeight	= outHeight;

	DBUG("Decode(%dx%d), format(%d)!", mDecodedWidth, mDecodedHeight, mColorFormat);
	if(mDecodedWidth == 1920 && mDecodedHeight > 1080){ //HDMI :: to prevent scaling in case of 1920x1088 contents
		mDecodedHeight = 1080;
	}

	if(!pMParm->mCropRgn_changed){
		pMParm->mOrigin_RegionInfo.source.left  = 0;
		pMParm->mOrigin_RegionInfo.source.top   = 0;
		pMParm->mOrigin_RegionInfo.source.right = mDecodedWidth;
		pMParm->mOrigin_RegionInfo.source.bottom = mDecodedHeight;

		pMParm->mTarget_CropInfo.sx 	= 0;
		pMParm->mTarget_CropInfo.sy 	= 0;
		pMParm->mTarget_CropInfo.srcW	= ((inWidth+15)>>4)<<4;
		pMParm->mTarget_CropInfo.srcH	= inHeight;

		if(transform == HAL_TRANSFORM_ROT_90 || transform == HAL_TRANSFORM_ROT_270) {
			pMParm->mTarget_CropInfo.dstW	= outHeight;
			pMParm->mTarget_CropInfo.dstH	= ((outWidth+3) >> 2)<<2;
		}
		else {
			pMParm->mTarget_CropInfo.dstW	= ((outWidth+3) >> 2)<<2;
			pMParm->mTarget_CropInfo.dstH	= outHeight;
		}
	}
	pMParm->mOverlay_recheck = false;

#ifndef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
	if(pMParm->mOutputMode == 0 || pMParm->mOutputMode == 2) // lcd or dual display mode
	{

	}
//	else if((this->*OverlayCheckDisplayFunction)(pMParm->mOverlay_recheck, outSx, outSy, outWidth, outHeight))
//	{
//		(this->*OverlayCheckExternalDisplayFunction)(pMParm->mOverlay_recheck, outSx, outSy, outWidth, outHeight);
//	}
#endif

	/* Stream aspect ratio of default vaule is not-defined.
	*/
	pMParm->mStreamAspectRatio = -1;
	pMParm->mChangedHDMIResolution = false;
	pMParm->mOriginalHDMIResolution = 0;
	pMParm->mMVCmode = false;
	mBufferId = -1;
	mNoSkipBufferId = -1;
	mCheckSync = 0;
	mNumberOfExternalVideo = 0;
	mCheckEnabledTCCSurface = -1;
	pMParm->mOverlayBuffer_Current = 0;
	LOGI("HwRenderer creation is succeed.");
	mVideoCount=0;
	mInitCheck = true;
}

HwRenderer::~HwRenderer() {
	for (int i = 0; i < pMParm->mOverlayBuffer_Cnt; i++) {
		pMParm->mOverlayBuffer_PhyAddr[i] = 0;
	}

	clearVsync(false, false, false);
	closeDevice();
	LOGI("~HwRenderer Done");
}

int HwRenderer::initDevice(int inWidth, int inHeight)
{
	unsigned int	uiOutputMode = 0;

	pMParm = NULL;
	if((pMParm = (render_param_t*)TCC_fo_malloc(__FUNCTION__, __LINE__,sizeof(render_param_t))) == NULL){
		LOGE("memory allocation for render_param_t is failed.");
		return -1;
	}
	memset(pMParm, 0x00, sizeof(render_param_t));

	pMParm->mRt_fd = pMParm->mM2m_fd = pMParm->mFb_fd = /*pMParm->mComposite_fd = pMParm->mComponent_fd =*/ pMParm->mViqe_fd = -1;
	pMParm->mOverlay_fd = pMParm->mTmem_fd = -1;

	pMParm->mUnique_address = 0;
	pMParm->mOutputMode = false;
	pMParm->mHDMIOutput = false;
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE
	pMParm->mCompositeOutput = false;
	pMParm->mComponentOutput = false;
#endif 

	pMParm->mNeed_rotate = false;
	pMParm->mNeed_transform = false;
	pMParm->mNeed_crop = false;
	pMParm->mVideoDeinterlaceSupport = false;
	pMParm->mVideoDeinterlaceFlag = false;
	pMParm->mVideoDeinterlaceScaleUpOneFieldFlag = false;
	pMParm->mVideoDeinterlaceForce = false;
	pMParm->mVIQE_firstfrm = 1;
	pMParm->mVIQE_onoff = false;
	pMParm->mVIQE_onthefly = false;
	pMParm->mVIQE_m2mDICondition = 0;

	char value[PROPERTY_VALUE_MAX];
	pMParm->bUseMemInfo = false;

	pMParm->bShowFps = false;
	property_get("tcc.show.video.fps", value, "");
	if( atoi(value) != 0)
		pMParm->bShowFps = true;

//	getfromsysfs("/sys/class/tcc_dispman/tcc_dispman/persist_output_mode", value);
//	uiOutputMode = atoi(value);
	uiOutputMode = 0;	// LCD only

#ifdef USE_OUTPUT_COMPONENT_COMPOSITE 
	property_get("persist.sys.composite_mode", value, "");
	pMParm->mCompositeMode = atoi(value);

	property_get("persist.sys.component_mode", value, "");
	pMParm->mComponentMode = atoi(value);
#endif

	pMParm->mFb_fd = open(FB0_DEVICE, O_RDWR);
	if (pMParm->mFb_fd < 0) {
		LOGE("can't open '%s'", FB0_DEVICE);
	}

	if( pMParm->bUseMemInfo ) {
		pMParm->mTmem_fd = open(TMEM_DEVICE, O_RDWR);
		if (pMParm->mTmem_fd < 0) {
			LOGE("can't open'%s'", TMEM_DEVICE);
		}
		else {
			pmap_get_info("ump_reserved", &pMParm->mUmpReservedPmap);

			if( ( pMParm->mTMapInfo = (void*)mmap(0, pMParm->mUmpReservedPmap.size, PROT_READ | PROT_WRITE, MAP_SHARED, pMParm->mTmem_fd, pMParm->mUmpReservedPmap.base) ) == MAP_FAILED ) {
				LOGE("/dev/tmem device's mmap failed.");
				close(pMParm->mTmem_fd);
				pMParm->mTmem_fd = -1;
			}
		}
	}

	pMParm->mFrame_cnt = 0;
	pMParm->mCropChanged = pMParm->mCropRgn_changed = false;
	memset(&pMParm->mOrigin_RegionInfo, 0x00, sizeof(pMParm->mOrigin_RegionInfo));
	memset(&pMParm->mTarget_CropInfo, 0x00, sizeof(pMParm->mTarget_CropInfo));
	memset(&pMParm->mScaler_arg, 0x00, sizeof(pMParm->mScaler_arg));
	memset(&pMParm->mGrp_arg, 0x00, sizeof(pMParm->mGrp_arg));

	tcc_display_size DisplaySize;
	getDisplaySize(OUTPUT_NONE, &DisplaySize);
	pMParm->mLcd_width = DisplaySize.width;
	pMParm->mLcd_height = DisplaySize.height;
	pMParm->mVSyncSelected = false;
	pMParm->mVsyncMode = 0;

#if defined(DEBUG_TIME_LOG) || defined(DEBUG_PROCTIME_LOG)
	time_cnt = 0;
	total_dec_time = 0;
#endif

#if defined(TCC_VIDEO_DEINTERLACE_SUPPORT) && !defined(_TCC8800_)
	property_set("tcc.video.deinterlace.support", "1");
#endif
	return 0;
}

void HwRenderer::closeDevice()
{
	unsigned int overlay_forbid;
	TCC_HDMI_M hdmi_video;
	struct tcc_lcdc_image_update lcdc_display;

	if(pMParm == NULL)
	{
		LOGE("[%s] pMParm is NULL / Error !!!!!", __func__);
		return ;
	}

	if(pMParm->mDisplayOutputSTB)
	{
		char value[PROPERTY_VALUE_MAX];
		unsigned int uiPreViewState;

		property_get("tcc.solution.preview", value, "");
		uiPreViewState = atoi(value);

		if(pMParm->mMode_3Dui && (uiPreViewState == 0))
		{
			memset(value, NULL, PROPERTY_VALUE_MAX);

			value[0] = '0';
			property_set("tcc.output.hdmi.video.format", value);

			if(ioctl( pMParm->mFb_fd, TCC_LCDC_3D_UI_DISABLE, NULL) < 0) {
				LOGD("Fb0 TCC_LCDC_3D_UI_DISABLE ctrl Error!");
			}
		}
	}

	memset(&lcdc_display, 0x00, sizeof(lcdc_display));
	lcdc_display.Lcdc_layer = LCDC_VIDEO_CHANNEL;
	lcdc_display.enable = 0;

	if(pMParm->mHDMIOutput) {
		if(ioctl(pMParm->mFb_fd, TCC_LCDC_HDMI_DISPLAY, &lcdc_display) < 0) {
			LOGE("TCC_LCDC_HDMI_DISPLAY Error!");
		}
	}
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE
	else if(pMParm->mCompositeOutput) {
		if(ioctl(pMParm->mComposite_fd, TCC_COMPOSITE_IOCTL_UPDATE, &lcdc_display) < 0) {
			LOGE("TCC_COMPOSITE_IOCTL_UPDATE Error!");
		}
	}
	else if(pMParm->mComponentOutput) {
		if(ioctl(pMParm->mComponent_fd, TCC_COMPONENT_IOCTL_UPDATE, &lcdc_display) < 0) {
			LOGE("TCC_COMPONENT_IOCTL_UPDATE Error!");
		}
	}
#endif  

	else if(pMParm->mOutputMode == 0){
#ifdef USE_LCD_OTF_SCALING
		if(ioctl(pMParm->mFb_fd, TCC_LCDC_DISPLAY_UPDATE, &lcdc_display) < 0) {
			LOGE("TCC_LCDC_DISPLAY_UPDATE Error! ");
		}
#endif
	}

#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
	if(pMParm->mVsyncMode)
	{
		int result = ioctl(pMParm->mFb_fd, TCC_LCDC_VIDEO_SKIP_FRAME_START, 0);
		if(result < 0)
			LOGE("### error fb ioctl : TCC_LCDC_VIDEO_SKIP_FRAME_START") ;
		pMParm->mVsyncMode = 0;
	}
#endif

	if(pMParm->mFb_fd != -1)
	{
		if(close(pMParm->mFb_fd) < 0)
		{
			LOGE("Error: close(pMParm->mFb_fd)");
		}
		pMParm->mFb_fd = -1;
	}

#ifdef USE_OUTPUT_COMPONENT_COMPOSITE
	if(pMParm->mComposite_fd!= -1)
	{
		if(close(pMParm->mComposite_fd) < 0)
		{
			LOGE("Error: close(pMParm->mComposite_fd)");
		}
		pMParm->mComposite_fd = -1;
	}

	if(pMParm->mComponent_fd!= -1)
	{
		if(close(pMParm->mComponent_fd) < 0)
		{
			LOGE("Error: close(pMParm->mComponent_fd)");
		}
		pMParm->mComponent_fd = -1;
	}
#endif

////////////////////////////////////
	if(pMParm->mViqe_fd != -1)
	{
		int iRet;
		if(pMParm->mVIQE_firstfrm == 0)
			ioctl(pMParm->mViqe_fd, IOCTL_VIQE_DEINITIALIZE);

		if(close(pMParm->mViqe_fd) < 0)
			LOGE("Error: close(pMParm->mViqe_fd)");

		memset(&pMParm->mViqe_arg, 0x00, sizeof(VIQE_DI_TYPE));
		pMParm->mVIQE_onoff = false;
		pMParm->mVIQE_onthefly = false;
		pMParm->mVIQE_DI_use = 0;
		pMParm->mVIQE_firstfrm = 1;
		pMParm->mViqe_fd = -1;
	}
#if defined(TCC_VIDEO_DEINTERLACE_SUPPORT) && !defined(_TCC8800_)
	property_set("tcc.video.deinterlace.support", "0");
#endif
	////////////////////////////////////

	if(pMParm->mM2m_fd != -1){
		if(close(pMParm->mM2m_fd) < 0)
		{
			LOGE("Error: close(pMParm->mM2m_fd)");
		}
		pMParm->mM2m_fd = -1;
	}

	if(pMParm->mRt_fd != -1){
		if(close(pMParm->mRt_fd) < 0)
		{
			LOGE("Error: close(pMParm->mRt_fd)");
		}
		pMParm->mRt_fd = -1;
	}

	if(pMParm->mHDMIUiSize)
	{
		TCC_fo_free (__FUNCTION__, __LINE__,pMParm->mHDMIUiSize);
		pMParm->mHDMIUiSize = NULL;
	}

#ifdef USE_OUTPUT_COMPONENT_COMPOSITE
	if(pMParm->mComponentChip)
	{
		TCC_fo_free (__FUNCTION__, __LINE__,pMParm->mComponentChip);
		pMParm->mComponentChip = NULL;
	}
#endif

	{
		vbuffer_manager vBuff_Init;

		vBuff_Init.istance_index = pMParm->mIdx_codecInstance;
		vBuff_Init.index = -1;

		ioctl(pMParm->mTmem_fd, TCC_VIDEO_SET_DISPLAYED_BUFFER_ID, &vBuff_Init);
		ioctl(pMParm->mTmem_fd, TCC_VIDEO_SET_DISPLAYING_IDX, &vBuff_Init);
	}

	if( pMParm->bUseMemInfo ) {
		if(pMParm->mTmem_fd != -1) {
			munmap((void*)pMParm->mTMapInfo, pMParm->mUmpReservedPmap.size);
			if(close(pMParm->mTmem_fd) < 0)
			{
				LOGE("Error: close(pMParm->mTmem_fd)");
			}
			pMParm->mTmem_fd = -1;
		}
	}

	if(pMParm)
	{
		TCC_fo_free (__FUNCTION__, __LINE__,pMParm);
		pMParm = NULL;
	}
}

int HwRenderer::getfromsysfs(const char *sysfspath, char *val)
{
  int sysfs_fd;
  int read_count;

  sysfs_fd = open(sysfspath, O_RDONLY);
  read_count = read(sysfs_fd, val, PROPERTY_VALUE_MAX);
  val[read_count] = '\0';
  close(sysfs_fd);
  return read_count;
}

int HwRenderer::getDisplaySize(OUTPUT_SELECT Output, tcc_display_size *display_size)
{
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE
	char value[PROPERTY_VALUE_MAX];
	property_get("persist.sys.composite_mode", value, "");
	pMParm->mCompositeMode = atoi(value);

	property_get("persist.sys.component_mode", value, "");
	pMParm->mComponentMode = atoi(value);
#endif

	switch(Output)
	{
		case OUTPUT_NONE:	// LCD size
			{
//				getfromsysfs("/sys/class/tcc_dispman/tcc_dispman/tcc_output_panel_width", value);
//              display_size->width = atoi(value);
//              getfromsysfs("/sys/class/tcc_dispman/tcc_dispman/tcc_output_panel_height", value);
//              display_size->height = atoi(value);
                display_size->width = 1024;
                display_size->height = 600;
			}
			break;

		case OUTPUT_HDMI:	// HDMI
			{
//				getfromsysfs("/sys/class/tcc_dispman/tcc_dispman/tcc_output_dispdev_width", value);
//              display_size->width = atoi(value);
//              getfromsysfs("/sys/class/tcc_dispman/tcc_dispman/tcc_output_dispdev_height", value);
//              display_size->height = atoi(value);
                display_size->width = 0;
                display_size->height = 0;
			}
			break;

#ifdef USE_OUTPUT_COMPONENT_COMPOSITE
		case OUTPUT_COMPOSITE:	//composite
			{
				if(pMParm->mCompositeMode == OUTPUT_COMPOSITE_NTSC)
				{
					display_size->width	= TCC_COMPOSITE_NTSC_WIDTH;
					display_size->height = TCC_COMPOSITE_NTSC_HEIGHT;
				}
				else
				{
					display_size->width	= TCC_COMPOSITE_PAL_WIDTH;
					display_size->height = TCC_COMPOSITE_PAL_HEIGHT;
				}
			}
			break;

		case OUTPUT_COMPONENT:	//component
			{
				if(!strcasecmp(pMParm->mComponentChip, "cs4954")) {
					display_size->width	= TCC_COMPONENT_NTSC_WIDTH;
					display_size->height = TCC_COMPONENT_NTSC_HEIGHT;
				}
				else{
					if(pMParm->mComponentMode == OUTPUT_COMPONENT_1080I)
					{
						display_size->width	= TCC_COMPONENT_1080I_WIDTH;
						display_size->height = TCC_COMPONENT_1080I_HEIGHT;
					}
					else
					{
						display_size->width	= TCC_COMPONENT_720P_WIDTH;
						display_size->height = TCC_COMPONENT_720P_HEIGHT;
					}
				}
			}
			break;
#endif

		default:
			break;
	}

	return 1;
}

int HwRenderer::VIQEProcess(unsigned int PA_Y, unsigned int PA_U, unsigned int PA_V, unsigned int DestAddr)
{
	int inFrameWidth, inFrameHeight;
	int iRet;

	inFrameWidth = ((mDecodedWidth+15)>>4)<<4;
	inFrameHeight = mDecodedHeight;

	if(pMParm->mVIQE_onoff)
	{
		pMParm->mViqe_arg.address[0] = PA_Y; pMParm->mViqe_arg.address[1] = PA_U; pMParm->mViqe_arg.address[2] = PA_V;
		pMParm->mViqe_arg.address[3] = pMParm->mPrevY; pMParm->mViqe_arg.address[4] = pMParm->mPrevU; pMParm->mViqe_arg.address[5] = pMParm->mPrevV;
		pMParm->mViqe_arg.dstAddr = DestAddr;

		if(pMParm->mVIQE_firstfrm == 1)
		{
			LOGI("Process First Frame. pMParm->mVIQE_firstfrm %d", pMParm->mVIQE_firstfrm);

			pMParm->mViqe_arg.onTheFly = pMParm->mViqe_arg.deinterlaceM2M = pMParm->mVIQE_onthefly;		//// VIQE always pass to M2MScaler on-the-fly.
			pMParm->mViqe_arg.srcWidth = inFrameWidth;
			pMParm->mViqe_arg.srcHeight = inFrameHeight;
#if 0
		#if !defined(_TCC8800_)
			pMParm->mViqe_arg.useWMIXER = 0;
		#endif
#endif
			pMParm->mViqe_arg.scalerCh = 0;

			if(pMParm->mHDMIOutput == 1)
				pMParm->mViqe_arg.lcdCtrlNo = 0;
			else
				pMParm->mViqe_arg.lcdCtrlNo = 1;

			pMParm->mViqe_arg.address[3] = PA_Y; pMParm->mViqe_arg.address[4] = PA_U; pMParm->mViqe_arg.address[5] = PA_V;

			pMParm->mViqe_fd = open(VIQE_DEVICE, O_RDWR);
			if (pMParm->mViqe_fd < 0)
				LOGE("can't open '%s'", VIQE_DEVICE);

			if(pMParm->mM2m_fd > 0)
			{
				LOGI("Now pMParm->mM2m_fd is not closed normaly , close and open pMParm->mM2m_fd %d ", pMParm->mM2m_fd);
				close(pMParm->mM2m_fd);
				pMParm->mM2m_fd = -1;
			}

			if(pMParm->mM2m_fd < 0)
			{
				pMParm->mM2m_fd = open(VIDEO_PLAY_SCALER, O_RDWR);
				if (pMParm->mM2m_fd < 0) {
					LOGE("can't open '%s'", VIDEO_PLAY_SCALER);
				}
				else {
					LOGI("%d : %s is opened.", __LINE__, VIDEO_PLAY_SCALER);
				}
			}

			iRet = ioctl(pMParm->mViqe_fd, IOCTL_VIQE_DEINITIALIZE, &pMParm->mViqe_arg);
			iRet = ioctl(pMParm->mViqe_fd, IOCTL_VIQE_INITIALIZE, &pMParm->mViqe_arg);
			if(iRet < 0)
				LOGE("Viqe Initialize Fail!!\n");

			pMParm->mVIQE_firstfrm = 0;
		}

		iRet = ioctl(pMParm->mViqe_fd, IOCTL_VIQE_EXCUTE, &pMParm->mViqe_arg);
		if(iRet < 0)
		{
			pMParm->mVIQE_onoff = false;
			DBUG("VIQE OFF %d\n", pMParm->mVIQE_onoff);
		}
	}

	pMParm->mPrevY = PA_Y;
	pMParm->mPrevU = PA_U;
	pMParm->mPrevV = PA_V;

	return 0;
}

int HwRenderer::setCropRegion(int fd_0, char* ghandle, int inWidth, int inHeight, int srcLeft, int srcTop, int srcRight, int srcBottom, int outLeft, int outTop, int outRight, int outBottom, unsigned int transform)
{
	crop_info_t Temp_CropInfo;
	int outVirtWidth, outVirtHeight, outVirtCroppedSx, outVirtCroppedSy, outVirtCroppedWidth, outvirtCroppedHeight;
	int finalOutWidth, finalOutHeight; // final displayed region on target device.
	int srcOriginCropWidth, srcOriginCropHeight; // original cropped region from Surface.
	int srcCropOfCropSx, srcCropOfCropSy, srcCropOfCropWidth, srcCropOfCropHeight; // crop info corresponding displayed region in original cropped region from Surface.
	int finalSrcCropSx, finalSrcCropSy, finalSrcCropWidth, finalSrcCropHeight; // final cropped region on source contents.
	int gapBottom, gapRight;
	int end_x, end_y;
	int temp;
	char *rot_str;
	inHeight = (inHeight>>1)<<1;
	srcTop = (srcTop>>1)<<1;
	srcBottom = (srcBottom>>1)<<1;


	if((outRight - outLeft) *(outBottom - outTop) <= 16)
	{
		//prevent 1,1 video in html5 youtube
		ALOGE("video size is wrong !!! (%d, %d) ~ (%d, %d) ",outLeft,outRight,outRight, outBottom);
		return 0;
	}

	pMParm->bHWaddr = check_valid_hw_availble(fd_0);
	pMParm->bExtract_data = get_private_info(&pMParm->private_data, ghandle, pMParm->bHWaddr, fd_0, inWidth, inHeight);

	if( pMParm->bCheckCrop ){
		if( (pMParm->private_data.optional_info[0] != (srcRight-srcLeft)) || (pMParm->private_data.optional_info[1] != (srcBottom-srcTop))){
			ALOGE("video source's crop is wrong !!! (%d x %d) != (%d, %d, %d, %d) ", pMParm->private_data.optional_info[0], pMParm->private_data.optional_info[1], srcLeft, srcTop, srcRight, srcBottom);
			return 0;
		}
	}
	pMParm->bCropChecked = true;

	if((pMParm->mOrigin_RegionInfo.source.left != srcLeft) || (pMParm->mOrigin_RegionInfo.source.top != srcTop) ||
		(pMParm->mOrigin_RegionInfo.source.right != srcRight) || (pMParm->mOrigin_RegionInfo.source.bottom != srcBottom) ||
		(pMParm->mOrigin_RegionInfo.display.left != outLeft) || (pMParm->mOrigin_RegionInfo.display.top != outTop) ||
		(pMParm->mOrigin_RegionInfo.display.right != outRight) || (pMParm->mOrigin_RegionInfo.display.bottom != outBottom)
		)
	{
		pMParm->mOrigin_RegionInfo.source.left  = srcLeft;
		pMParm->mOrigin_RegionInfo.source.top   = srcTop;
		pMParm->mOrigin_RegionInfo.source.right = srcRight;
		pMParm->mOrigin_RegionInfo.source.bottom = srcBottom;

		pMParm->mOrigin_RegionInfo.display.left  = outLeft;
		pMParm->mOrigin_RegionInfo.display.top   = outTop;
		pMParm->mOrigin_RegionInfo.display.right = outRight;
		pMParm->mOrigin_RegionInfo.display.bottom = outBottom;

// exceptional process for DXB.
		if( !transform && outLeft >= 0 && outTop >= 0 )
		{
			Temp_CropInfo.sx = srcLeft;
			Temp_CropInfo.sy = srcTop;
			Temp_CropInfo.srcW = srcRight - srcLeft;
			Temp_CropInfo.srcH = srcBottom - srcTop;

			if(outRight > pMParm->mLcd_width){
				outRight = pMParm->mLcd_width;

				if(outBottom > pMParm->mLcd_height)
					outBottom = pMParm->mLcd_height;
			}

			Temp_CropInfo.dstW = outRight - outLeft;
			Temp_CropInfo.dstH = outBottom - outTop;

			pMParm->mAdditionalCropSource = false;
			if(Temp_CropInfo.srcH > 1080)
				Temp_CropInfo.srcH	= 1080;

			goto DONE;
		}

// In order to calculate exact region to display on target device.
/*
    Displayed value : -20, -20 ~ 1046, 620
    Contents's resolution: 1920 x 1080

                                    outVirtWidth (1066)
        |------------------------------------------------------------------------|
        |                                                                        |
        | (outVirtCroppedSx,                                                     |
        |  outVirtCroppedSy)      outVirtCroppedWidth  (1024)                    |
        | (20,20) -----------------------------------------------------|         |
        |         |----------------------------------------------------|         |
        |         |----------------------------------------------------|         | outVirtHeight (640)
        |         |----------------------------------------------------|         |
        |         |----------------------------------------------------|outVirtCroppedHeight
        |         |----------------------------------------------------|(600)    |
        |         |----------------------------------------------------|         |
        |         |----------------------------------------------------|         |
        |         |----------------------------------------------------|         |
        |         -----------------------------------------------------|         |
        |                                                                        |
        |                                                                        |
        |------------------------------------------------------------------------|
*/
		gapBottom = pMParm->mLcd_height - outBottom;
		gapRight  = pMParm->mLcd_width - outRight;

		if(outLeft <= 0)
			finalOutWidth = (pMParm->mLcd_width < outRight) ? pMParm->mLcd_width : outRight;
		else
			finalOutWidth = (pMParm->mLcd_width < outRight) ? (pMParm->mLcd_width - outLeft) : (outRight - outLeft);

		if(outTop <= 0)
			finalOutHeight = (pMParm->mLcd_height < outBottom) ? pMParm->mLcd_height : outBottom;
		else
			finalOutHeight = (pMParm->mLcd_height < outBottom) ? (pMParm->mLcd_height - outTop) : (outBottom - outTop);

		outVirtWidth = outRight - outLeft;
		outVirtHeight = outBottom - outTop;
		outVirtCroppedWidth = finalOutWidth;
		outvirtCroppedHeight = finalOutHeight;

		if( transform == HAL_TRANSFORM_ROT_90 || transform == HAL_TRANSFORM_ROT_270 ){
			temp = outVirtWidth;
			outVirtWidth = outVirtHeight;
			outVirtHeight = temp;
			temp = outVirtCroppedWidth;
			outVirtCroppedWidth = outvirtCroppedHeight;
			outvirtCroppedHeight = temp;
		}

		if( transform == HAL_TRANSFORM_ROT_90 )
		{
			rot_str = "90";
			outVirtCroppedSx = (gapBottom < 0) ? -gapBottom : 0;
			outVirtCroppedSy = (outLeft < 0) ? -outLeft : 0;
		}
		else if( transform == HAL_TRANSFORM_ROT_180 )
		{
			rot_str = "180";
			outVirtCroppedSx = (outLeft < 0) ? -outLeft : 0;
			outVirtCroppedSy = (gapBottom < 0) ? -gapBottom : 0;
		}
		else if( transform == HAL_TRANSFORM_ROT_270 )
		{
			rot_str = "270";
			outVirtCroppedSx = (outTop < 0) ? -outTop : 0;
			outVirtCroppedSy = (gapRight < 0) ? -gapRight : 0;
		}
		else//( transform == HAL_TRANSFORM_ROT_0 )
		{
			rot_str = "0";
			outVirtCroppedSx = (outLeft < 0) ? -outLeft : 0;
			outVirtCroppedSy = (outTop < 0) ? -outTop : 0;
		}

		DBUG("%s Rotate Virtual Display Source(%dx%d), Crop(%d,%d ~ %dx%d) => Exact Display_region for Device(%dx%d)", rot_str,
			outVirtWidth, outVirtHeight, outVirtCroppedSx, outVirtCroppedSy, outVirtCroppedWidth, outvirtCroppedHeight, finalOutWidth, finalOutHeight);

// In order to find out exact cropped source's region corresponding above measured region.

/*
    Displayed value : -20, -20 ~ 1046, 620
    Contents's resolution: 1920 x 1080  => crop(40,40 ~ 1880, 1040)

                                    1920
        |------------------------------------------------------------------------|
        |                                                                        |
        |                                                                        |
        | (srcLeft,srcTop)      srcOriginCropWidth  (1840)                       |
        | (40,40) -----------------------------------------------------|         |
        |         |             srcCropOfCropWidth                     |         |
        |         |      |-------------------------------------|       |         | 1080
        |         |      |-------------------------------------|srcCropOfCropHeight
        |         |      |-------------------------------------|       |srcOriginCropHeight
        |         |      |-------------------------------------|       |(1000)   |
        |         |      |-------------------------------------|       |         |
        |         |      |-------------------------------------|       |         |
        |         |                                                    |         |
        |         -----------------------------------------------------|         |
        |                                                                        |
        |                                                                        |
        |------------------------------------------------------------------------|
*/
		srcOriginCropWidth  = srcRight - srcLeft;
		srcOriginCropHeight = srcBottom - srcTop;

/*

                               srcOriginCropWidth  (1840)
        |------------------------------------------------------------------------|
        | (srcCropOfCropSx,                                                      |
        |   srcCropOfCropSy)         srcCropOfCropWidth                          |
        | (70,64) -----------------------------------------------------|         |
        |         |                                                    |         |
        |         |                                                    |         |
        |         |                                                    |         | srcOriginCropHeight (1000)
        |         |                                                    |         |
        |         |                                                    | srcCropOfCropHeight
        |         |                                                    |         |
        |         |                                                    |         |
        |         |                                                    |         |
        |         -----------------------------------------------------|         |
        |                                                                        |
        |                                                                        |
        |------------------------------------------------------------------------|
*/
		DBUG("Source Crop from Surface (%d,%d ~ %dx%d)", srcLeft, srcTop, srcOriginCropWidth, srcOriginCropHeight);

		srcCropOfCropSx = (srcOriginCropWidth * outVirtCroppedSx)/outVirtWidth;
		srcCropOfCropSy = (srcOriginCropHeight * outVirtCroppedSy)/outVirtHeight;
		end_x = (srcOriginCropWidth * (outVirtCroppedSx + outVirtCroppedWidth))/outVirtWidth;
		if( end_x > srcOriginCropWidth)
			srcCropOfCropWidth  = srcOriginCropWidth - srcCropOfCropSx;
		else
			srcCropOfCropWidth  = end_x - srcCropOfCropSx;
		end_y = (srcOriginCropHeight * (outVirtCroppedSy + outvirtCroppedHeight))/outVirtHeight;
		if( end_y > srcOriginCropHeight)
			srcCropOfCropHeight = srcOriginCropHeight - srcCropOfCropSy;
		else
			srcCropOfCropHeight = end_y - srcCropOfCropSy;

		DBUG("Source Crop of Crop for cropped display (%d,%d ~ %dx%d)", srcCropOfCropSx, srcCropOfCropSy, srcCropOfCropWidth, srcCropOfCropHeight);

		finalSrcCropSx = srcLeft + srcCropOfCropSx;
		finalSrcCropSy = srcTop + srcCropOfCropSy;
		finalSrcCropWidth = srcCropOfCropWidth;
		finalSrcCropHeight = srcCropOfCropHeight;

		DBUG("Final Source Crop :: %d,%d ~ %d x %d", finalSrcCropSx, finalSrcCropSy, finalSrcCropWidth, finalSrcCropHeight);

//		if(finalSrcCropWidth == 1920 && finalSrcCropHeight == 1080 && finalOutWidth == 960 && finalOutHeight > 540)
//			finalOutHeight = 540;

		Temp_CropInfo.sx	= finalSrcCropSx;
		Temp_CropInfo.sy	= finalSrcCropSy;
		Temp_CropInfo.srcW	= ((finalSrcCropWidth+3)>>2)<<2; // ((srcW+3) >> 2)<<2; //Bug fix :: Crop width have to be multiple of 16.

		if(inWidth < Temp_CropInfo.srcW)
			Temp_CropInfo.srcW	= ((inWidth+3)>>2)<<2;
		else
		{
			if(inWidth < (Temp_CropInfo.sx + Temp_CropInfo.srcW))
			{
				Temp_CropInfo.sx = inWidth - Temp_CropInfo.srcW;
			}
		}

		if(finalSrcCropHeight > 1080)
			Temp_CropInfo.srcH	= 1080;
		else
			Temp_CropInfo.srcH	= ((finalSrcCropHeight+3)>>2)<<2;

		if(transform == HAL_TRANSFORM_ROT_90 || transform == HAL_TRANSFORM_ROT_270) {
			Temp_CropInfo.dstW	= ((finalOutHeight+3)>>2)<<2;
			Temp_CropInfo.dstH	= ((finalOutWidth+3)>>2)<<2;
		}
		else {
			Temp_CropInfo.dstW	= ((finalOutWidth+3)>>2)<<2;
			Temp_CropInfo.dstH	= ((finalOutHeight+3)>>2)<<2;
		}

		Temp_CropInfo.sx		= ((Temp_CropInfo.sx+1)>>1)<<1;
		Temp_CropInfo.sy		= ((Temp_CropInfo.sy+1)>>1)<<1;

DONE:
		if((pMParm->mTarget_CropInfo.sx != Temp_CropInfo.sx) || (pMParm->mTarget_CropInfo.sy != Temp_CropInfo.sy) ||
			(pMParm->mTarget_CropInfo.srcW != Temp_CropInfo.srcW) || (pMParm->mTarget_CropInfo.srcH != Temp_CropInfo.srcH) ||
			(pMParm->mTarget_CropInfo.dstW != Temp_CropInfo.dstW) || (pMParm->mTarget_CropInfo.dstH != Temp_CropInfo.dstH)
		)
		{
			pMParm->mTarget_CropInfo.sx = Temp_CropInfo.sx;
			pMParm->mTarget_CropInfo.sy = Temp_CropInfo.sy;
			pMParm->mTarget_CropInfo.srcW = Temp_CropInfo.srcW;
			pMParm->mTarget_CropInfo.srcH = Temp_CropInfo.srcH;
			pMParm->mTarget_CropInfo.dstW = Temp_CropInfo.dstW;
			pMParm->mTarget_CropInfo.dstH = Temp_CropInfo.dstH;

			if( (pMParm->mTarget_CropInfo.sx != srcLeft) || (pMParm->mTarget_CropInfo.sy != srcTop)
				|| (pMParm->mTarget_CropInfo.srcW != (srcRight - srcLeft)) || (pMParm->mTarget_CropInfo.srcH != (srcBottom - srcTop))
				|| (inWidth != pMParm->mTarget_CropInfo.srcW) || (inHeight != pMParm->mTarget_CropInfo.srcH))
			{
				pMParm->mAdditionalCropSource = true;
				if(pMParm->mTarget_CropInfo.sx == 0 && pMParm->mTarget_CropInfo.sy == 0 &&
					((inWidth - pMParm->mTarget_CropInfo.srcW) <= 8) && ((inHeight - pMParm->mTarget_CropInfo.srcH) <= 8)){
					pMParm->mAdditionalCropSource = false;
				}
			}
			else{
				pMParm->mAdditionalCropSource = false;
			}

			pMParm->mCropRgn_changed = true;

			if(!pMParm->mCropChanged)
				pMParm->mCropChanged = true;

			LOGD("Overlay Crop AddCropSource(%d) :: Resolution(%d*%d), Origin Crop(%d,%d ~ %d,%d) and Display(%d,%d ~ %d,%d) => reCal (%d,%d ~ %d x %d => %d x %d)",
				pMParm->mAdditionalCropSource, inWidth, inHeight, srcLeft, srcTop, srcRight, srcBottom, outLeft, outTop, outRight, outBottom,
				pMParm->mTarget_CropInfo.sx, pMParm->mTarget_CropInfo.sy,
				pMParm->mTarget_CropInfo.srcW, pMParm->mTarget_CropInfo.srcH,
				pMParm->mTarget_CropInfo.dstW , pMParm->mTarget_CropInfo.dstH);
		}
		else
		{
			DBUG("Overlay Keep the previous-crop AddCropSource(%d) :: Resolution(%d*%d), Origin Crop(%d,%d ~ %d,%d) and Display(%d,%d ~ %d,%d) => reCal (%d,%d ~ %d x %d => %d x %d)",
				pMParm->mAdditionalCropSource, inWidth, inHeight, srcLeft, srcTop, srcRight, srcBottom, outLeft, outTop, outRight, outBottom,
				pMParm->mTarget_CropInfo.sx, pMParm->mTarget_CropInfo.sy,
				pMParm->mTarget_CropInfo.srcW, pMParm->mTarget_CropInfo.srcH,
				pMParm->mTarget_CropInfo.dstW , pMParm->mTarget_CropInfo.dstH);

		}
	}
	return 0;
}

int HwRenderer::overlayInit(SCALER_TYPE *scaler_arg, G2D_COMMON_TYPE *grp_arg, uint8_t isOutRGB)
{
	unsigned int SrcWidth, SrcHeight; 						//scaler_in size
	unsigned int ScropSx, ScropSy, ScropWidth, ScropHeight; //scaler_in cropped information
	unsigned int SoutWidth, SoutHeight; 					//scaler_out size

	char value[PROPERTY_VALUE_MAX];

	pMParm->mNeed_scaler = true;
	ScropSx = pMParm->mTarget_CropInfo.sx;
	ScropSy = pMParm->mTarget_CropInfo.sy;
	ScropWidth = pMParm->mTarget_CropInfo.srcW;
	ScropHeight = pMParm->mTarget_CropInfo.srcH;

	SrcWidth = ((mDecodedWidth+15)>>4)<<4;
	SrcHeight = mDecodedHeight;

	if(pMParm->mNeed_transform && pMParm->mNeed_rotate)
	{
		SoutWidth	= pMParm->mDispHeight;
		SoutHeight  = pMParm->mDispWidth;

		if(ScropWidth != pMParm->mDispHeight || ScropHeight != pMParm->mDispWidth)
			pMParm->mNeed_scaler = true;
		else if(pMParm->mCropChanged)
			pMParm->mNeed_scaler = true;
	}
	else
	{
		SoutWidth	= pMParm->mDispWidth;
		SoutHeight = pMParm->mDispHeight;

		if(SrcWidth != pMParm->mDispWidth || SrcHeight != pMParm->mDispHeight)
			pMParm->mNeed_scaler = true;
		else
		{
			//enable scaler if g2d engine was disabled.
			if(!pMParm->mNeed_crop)
				pMParm->mNeed_scaler = true;
		}
	}

	if((pMParm->mNeed_transform || pMParm->mNeed_crop))
		pMParm->mParallel = true;
	else
		pMParm->mParallel = false;

	DBUG("pMParm->mParallel(%d) => Need:: Scaler(%d), transform(%d), pMParm->mNeed_crop(%d)", pMParm->mParallel, pMParm->mNeed_scaler, pMParm->mNeed_transform, pMParm->mNeed_crop);

	scaler_arg->responsetype	= SCALER_INTERRUPT; //SCALER_INTERRUPT - SCALER_POLLING
	if(mColorFormat == OMX_COLOR_FormatYUV420SemiPlanar)
		scaler_arg->src_fmt			= SCALER_YUV420_inter;
	else if(mColorFormat == OMX_COLOR_FormatYUV422Planar)
		scaler_arg->src_fmt			= SCALER_YUV422_sp;
	else
		scaler_arg->src_fmt 		= SCALER_YUV420_sp;
	scaler_arg->src_ImgWidth   	= SrcWidth;
	scaler_arg->src_ImgHeight  	= SrcHeight;
	scaler_arg->src_winLeft		= ScropSx;
	scaler_arg->src_winTop		= ScropSy;
	scaler_arg->src_winRight    = scaler_arg->src_winLeft + ScropWidth;
	scaler_arg->src_winBottom  	= scaler_arg->src_winTop + ScropHeight;
	DBUG("@@@ Scaler-SRC(fmt:%d):: %d(0x%x) x %d(0x%x), %d(0x%x),%d(0x%x) ~ %d(0x%x) x %d(0x%x)", scaler_arg->src_fmt, scaler_arg->src_ImgWidth, scaler_arg->src_ImgWidth, scaler_arg->src_ImgHeight,
			scaler_arg->src_ImgHeight, scaler_arg->src_winLeft, scaler_arg->src_winLeft, scaler_arg->src_winTop, scaler_arg->src_winTop, ScropWidth, ScropWidth, ScropHeight, ScropHeight);

	scaler_arg->dest_Yaddr 		= pMParm->mOverlayRotation_2nd.base;
	scaler_arg->dest_Uaddr  	= 0;
	scaler_arg->dest_Vaddr 		= 0;

	if(isOutRGB)
		scaler_arg->dest_fmt = SCALER_RGB565;
	else
	{
		if(pMParm->mDisplayOutputSTB)
		{
			property_get("persist.sys.display.mode", value, "");
			if(atoi(value) != 0)
				scaler_arg->dest_fmt = SCALER_YUV420_inter;
			else
				scaler_arg->dest_fmt = SCALER_YUV422_sq0;
		}
		else
			scaler_arg->dest_fmt = SCALER_YUV422_sq0;
	}

	scaler_arg->dest_ImgWidth  	= SoutWidth;
	scaler_arg->dest_ImgHeight 	= SoutHeight;
	scaler_arg->dest_winLeft	= 0;
	scaler_arg->dest_winTop		= 0;
	scaler_arg->dest_winRight 	= scaler_arg->dest_winLeft + SoutWidth;
	scaler_arg->dest_winBottom	= scaler_arg->dest_winTop + SoutHeight;
	DBUG("@@@ Scaler-Dest(rmt:%d):: %d(0x%x) x %d(0x%x)", scaler_arg->dest_fmt, scaler_arg->dest_ImgWidth, scaler_arg->dest_ImgWidth, scaler_arg->dest_ImgHeight, scaler_arg->dest_ImgHeight);

///////////////////////////added by hyun
	scaler_arg->viqe_onthefly = 0;
///////////////////////////

	if(pMParm->mNeed_transform || pMParm->mNeed_crop)
	{
		//ZzaU:: change because of Video Hang-Problem.
		grp_arg->responsetype = G2D_INTERRUPT; //G2D_INTERRUPT, G2D_POLLING;
//		grp_arg->src0		= (unsigned int)src_addrY;
//		grp_arg->src1 		= (unsigned int)src_addrU;
//		grp_arg->src2 		= (unsigned int)src_addrV;

		if(pMParm->mNeed_scaler)	{
			if(isOutRGB)
				grp_arg->srcfm.format 	= GE_RGB565;
			else
				grp_arg->srcfm.format 	= GE_YUV422_sq;
		}
		else	{
			if(mColorFormat == OMX_COLOR_FormatYUV420SemiPlanar)
				grp_arg->srcfm.format	= GE_YUV420_in;
			else if(mColorFormat == OMX_COLOR_FormatYUV422Planar)
				grp_arg->srcfm.format	= GE_YUV422_sp;
			else
				grp_arg->srcfm.format	= GE_YUV420_sp;
		}
		grp_arg->srcfm.data_swap	= 0;

		grp_arg->src_imgx 	= SoutWidth;
		grp_arg->src_imgy 	= SoutHeight;
		grp_arg->DefaultBuffer = 0;

//		grp_arg->tgt0		= (unsigned int)dest;
//		grp_arg->tgt1 		= 0;
//		grp_arg->tgt2 		= 0;

		if(isOutRGB)
			grp_arg->tgtfm.format 	= GE_RGB565;
		else
			grp_arg->tgtfm.format 	= GE_YUV422_sq;

		grp_arg->tgtfm.data_swap 	= 0;

		if(pMParm->mNeed_transform)//Overlay Preview!!
		{
			switch(pMParm->mOrigin_DispInfo.transform)
			{
				case HWC_TRANSFORM_FLIP_H:
					grp_arg->ch_mode	= FLIP_HOR;
					break;

				case HWC_TRANSFORM_FLIP_V:
					grp_arg->ch_mode	= FLIP_VER;
					break;

				case HWC_TRANSFORM_ROT_90:
					grp_arg->ch_mode	= ROTATE_90;
					break;

				case HWC_TRANSFORM_ROT_180:
					grp_arg->ch_mode	= ROTATE_180;
					break;

				case HWC_TRANSFORM_ROT_270:
					grp_arg->ch_mode	= ROTATE_270;
					break;
			}
		}
		else
		{
			grp_arg->ch_mode 	= NOOP;
		}

		if((grp_arg->ch_mode == ROTATE_90) || (grp_arg->ch_mode == ROTATE_270))
		{
			if(SoutWidth > (pMParm->mRight_truncation + pMParm->mFinal_DPHeight))
				grp_arg->crop_offx = (SoutWidth - pMParm->mRight_truncation - pMParm->mFinal_DPHeight)/2;
			else
				grp_arg->crop_offx = 0;

			grp_arg->crop_offy = (SoutHeight - pMParm->mFinal_DPWidth)/2;
			grp_arg->crop_imgx	= pMParm->mFinal_DPHeight;
			grp_arg->crop_imgy	= pMParm->mFinal_DPWidth;

			grp_arg->dst_imgx = grp_arg->crop_imgy;
			grp_arg->dst_imgy = grp_arg->crop_imgx;
		}
		else
		{
			if(SoutWidth > (pMParm->mRight_truncation + pMParm->mFinal_DPWidth))
				grp_arg->crop_offx = (SoutWidth - pMParm->mRight_truncation - pMParm->mFinal_DPWidth)/2;
			else
				grp_arg->crop_offx = 0;

			if(SoutHeight > pMParm->mFinal_DPHeight)
				grp_arg->crop_offy = (SoutHeight - pMParm->mFinal_DPHeight)/2;
			else
				grp_arg->crop_offy = 0;
			grp_arg->crop_imgx	= pMParm->mFinal_DPWidth;
			grp_arg->crop_imgy	= pMParm->mFinal_DPHeight;

			grp_arg->dst_imgx = grp_arg->crop_imgx;
			grp_arg->dst_imgy = grp_arg->crop_imgy;
		}

		grp_arg->dst_off_x = 0;
		grp_arg->dst_off_y = 0;

		DBUG("@@@ G2D-SRC:: %d x %d, %d,%d ~ %d x %d", grp_arg->src_imgx, grp_arg->src_imgy, grp_arg->crop_offx, grp_arg->crop_offy, grp_arg->crop_imgx, grp_arg->crop_imgy);
		DBUG("@@@ G2D-Dest:: %d x %d", grp_arg->dst_imgx, grp_arg->dst_imgy);
	}
	return 0;
}

bool HwRenderer::overlayCheck(bool reCheck, int outSx, int outSy, int outWidth, int outHeight, unsigned int rotation)
{
	float fScaleWidth, fScaleHeight, fScaleTotal, fScale;
	unsigned int SrcWidth, SrcHeight;
	unsigned int stride_width, final_stride;
	// Real_target to display. set real position in lcd!!
	unsigned int target_sx, target_sy, target_width, target_height, transform;
	unsigned int gap_width, gap_height;
	unsigned int stride_check_width;
	int CommitResult = 1;

	if(outSx < 0){
		outWidth += outSx;
		outSx = 0;
	}
	if(outSy < 0){
		outHeight += outSy;
		outSy = 0;
	}

	if(reCheck)
	{
		//in case of change in playing!!
		target_sx       = outSx;
		target_sy       = outSy;
		target_width    = outWidth;
		target_height   = outHeight;
		transform       = rotation;

		if(target_width > (pMParm->mLcd_width - target_sx))
			target_width = (pMParm->mLcd_width - target_sx);

		if(target_height > (pMParm->mLcd_height - target_sy))
			target_height = (pMParm->mLcd_height - target_sy);

		if(CommitResult)
		{
			if((pMParm->mOrigin_DispInfo.sx != target_sx) || (pMParm->mOrigin_DispInfo.sy != target_sy) ||
				(pMParm->mOrigin_DispInfo.width != target_width) || (pMParm->mOrigin_DispInfo.height != target_height) || (pMParm->mOrigin_DispInfo.transform != transform)
			)
			{
				if((target_width == 0) || (target_height == 0))
				{
					LOGE("L: Operation size is abnormal. (%d x %d)", target_width, target_height);
					return false;
				}

				pMParm->mOrigin_DispInfo.sx 		= target_sx;
				pMParm->mOrigin_DispInfo.sy 		= target_sy;
				pMParm->mOrigin_DispInfo.width 		= target_width;
				pMParm->mOrigin_DispInfo.height 	= target_height;
				pMParm->mOrigin_DispInfo.transform 	= transform;

				LOGI("L: Overlay reSetting In :: %d,%d ~ %d,%d, %d => %d,%d ~ %d,%d, %d", outSx, outSy, outWidth, outHeight, rotation,
                              	pMParm->mOrigin_DispInfo.sx, pMParm->mOrigin_DispInfo.sy, pMParm->mOrigin_DispInfo.width, pMParm->mOrigin_DispInfo.height, pMParm->mOrigin_DispInfo.transform);
			}
			else
			{
				if(pMParm->mCropRgn_changed == false){
					return false;
				}
			}
		}
		else
		{
			if(pMParm->mCropRgn_changed == false){
				return false;
			}
		}
	}
	else
	{
		//first change after playback!!
		pMParm->mOrigin_DispInfo.sx         = outSx;
		pMParm->mOrigin_DispInfo.sy         = outSy;
		pMParm->mOrigin_DispInfo.width      = outWidth;
		pMParm->mOrigin_DispInfo.height     = outHeight;
		pMParm->mOrigin_DispInfo.transform  = rotation;

		if(pMParm->mOrigin_DispInfo.width > (pMParm->mLcd_width - pMParm->mOrigin_DispInfo.sx))
			pMParm->mOrigin_DispInfo.width = (pMParm->mLcd_width - pMParm->mOrigin_DispInfo.sx);

		if(pMParm->mOrigin_DispInfo.height > (pMParm->mLcd_height - pMParm->mOrigin_DispInfo.sy))
			pMParm->mOrigin_DispInfo.height = (pMParm->mLcd_height - pMParm->mOrigin_DispInfo.sy);

		if(CommitResult)
		{
			pMParm->mOverlay_recheck = true;
			LOGI("L: Overlay first Setting In :: %d,%d ~ %d,%d, %d => %d,%d ~ %d,%d, %d", outSx, outSy, outWidth, outHeight, rotation,
				pMParm->mOrigin_DispInfo.sx, pMParm->mOrigin_DispInfo.sy, pMParm->mOrigin_DispInfo.width, pMParm->mOrigin_DispInfo.height, pMParm->mOrigin_DispInfo.transform);

			if((pMParm->mOrigin_DispInfo.width == 0) || (pMParm->mOrigin_DispInfo.height == 0))
			{
				LOGE("L: Operation size is abnormal. (%d x %d)", pMParm->mOrigin_DispInfo.width, pMParm->mOrigin_DispInfo.height);
				return false;
			}
		}
		else{
			return false;
		}
	}

	DBUG("L: Overlay Check In!!, SRC::%d x %d   TAR:: %d x %d", mDecodedWidth, mDecodedHeight, pMParm->mOrigin_DispInfo.width, pMParm->mOrigin_DispInfo.height);
	pMParm->mTarget_DispInfo.sx		= pMParm->mOrigin_DispInfo.sx;
	pMParm->mTarget_DispInfo.sy		= pMParm->mOrigin_DispInfo.sy;
	pMParm->mTarget_DispInfo.width	= pMParm->mOrigin_DispInfo.width;
	pMParm->mTarget_DispInfo.height	= pMParm->mOrigin_DispInfo.height;
	pMParm->mTarget_DispInfo.transform  = pMParm->mOrigin_DispInfo.transform;

//	pMParm->mOrigin_DispInfo.transform = HAL_TRANSFORM_ROT_270; //to test transform

	pMParm->mNeed_rotate = false;
	if(pMParm->mOrigin_DispInfo.transform)
	{
		pMParm->mNeed_transform = true;
		if(pMParm->mOrigin_DispInfo.transform == HAL_TRANSFORM_ROT_90 || pMParm->mOrigin_DispInfo.transform == HAL_TRANSFORM_ROT_270)
		{
			pMParm->mNeed_rotate = true;
		}
	}
	else
	{
		pMParm->mNeed_transform = false;
	}

	if(!pMParm->mCropChanged)
	{
		SrcWidth = ((mDecodedWidth + 15)>>4)<<4;
		SrcHeight = ((mDecodedHeight + 1)>>1)<<1;
		stride_check_width = mDecodedWidth;
	}
	else
	{
		SrcWidth = pMParm->mTarget_CropInfo.srcW;
		SrcHeight = pMParm->mTarget_CropInfo.srcH;
		stride_check_width = SrcWidth;
	}

	if(pMParm->mNeed_rotate)
	{
		if(!pMParm->mCropChanged)
		{
			pMParm->mTarget_CropInfo.dstW = pMParm->mOrigin_DispInfo.height;
			pMParm->mTarget_CropInfo.dstH = pMParm->mOrigin_DispInfo.width;
		}

		pMParm->mDispWidth		= pMParm->mTarget_CropInfo.dstH;
		pMParm->mDispHeight 	= pMParm->mTarget_CropInfo.dstW;

		//Scaler limitation!!
		if(pMParm->mTarget_CropInfo.srcW < pMParm->mTarget_CropInfo.dstW/*pMParm->mDispWidth*/ && pMParm->mTarget_CropInfo.srcH < pMParm->mTarget_CropInfo.dstH/*pMParm->mDispHeight*/)
		{
			float freScale;

			fScaleWidth 	= (float)pMParm->mTarget_CropInfo.dstW/*pMParm->mDispWidth*/ / pMParm->mTarget_CropInfo.srcW;
			fScaleHeight	= (float)pMParm->mTarget_CropInfo.dstH/*pMParm->mDispHeight*/ / pMParm->mTarget_CropInfo.srcH;
			fScaleTotal 	= fScaleWidth * fScaleHeight;

	#ifdef USE_LIMITATION_SCALER_RATIO
			if(fScaleWidth >= 3.8 || fScaleHeight >= 3.8)
			{
				freScale		= 3.8;
				pMParm->mDispWidth		= (unsigned int)(pMParm->mTarget_CropInfo.srcH * freScale);
				pMParm->mDispHeight 	= (unsigned int)(pMParm->mTarget_CropInfo.srcW * freScale);
			}
	#endif
		}

		pMParm->mDispWidth	= ((pMParm->mDispWidth+3)>>2)<<2;
		pMParm->mDispHeight = ((pMParm->mDispHeight+3)>>2)<<2;
		DBUG("L: Scaler Out [r]: %d x %d", pMParm->mDispHeight, pMParm->mDispWidth);

		pMParm->mFinal_DPWidth = (pMParm->mDispWidth>>2)<<2; // for 16x multiple width on LCDC..

		pMParm->mFinal_stride = 0;
		stride_width = pMParm->mTarget_CropInfo.srcW - stride_check_width;//SrcWidth;
		DBUG("L: ##@@## s %d = %d - %d", stride_width, pMParm->mTarget_CropInfo.srcW, SrcWidth);
		if(stride_width)
		{
			pMParm->mFinal_stride = ((stride_width*pMParm->mDispHeight) / pMParm->mTarget_CropInfo.srcW);// + 1;

			DBUG("L: ##@@## f %d = %d x %d / %d", pMParm->mFinal_stride, stride_width, pMParm->mDispHeight, pMParm->mTarget_CropInfo.srcW);
			pMParm->mFinal_stride = ((pMParm->mFinal_stride+1)>>1)<<1;
		}

		pMParm->mFinal_DPHeight = pMParm->mDispHeight - pMParm->mFinal_stride;
		pMParm->mRight_truncation = pMParm->mFinal_stride;
		DBUG("L: transform/Crop Out [r]: %d x %d, height_stride: %d", pMParm->mFinal_DPWidth, pMParm->mFinal_DPHeight, pMParm->mFinal_stride);
	}
	else
	{
		if(!pMParm->mCropChanged)
		{
			pMParm->mTarget_CropInfo.dstW = pMParm->mOrigin_DispInfo.width;
			pMParm->mTarget_CropInfo.dstH = pMParm->mOrigin_DispInfo.height;
		}

		fScaleWidth 	= (float)pMParm->mTarget_CropInfo.dstW / pMParm->mTarget_CropInfo.srcW;
		fScaleHeight	= (float)pMParm->mTarget_CropInfo.dstH / pMParm->mTarget_CropInfo.srcH;
		fScaleTotal 	= fScaleWidth * fScaleHeight;

		//Scaler limitation!!
#ifdef USE_LIMITATION_SCALER_RATIO
		if(pMParm->mTarget_CropInfo.srcW < pMParm->mOrigin_DispInfo.width && pMParm->mTarget_CropInfo.srcH < pMParm->mOrigin_DispInfo.height
			&& (fScaleWidth >= 3.8 || fScaleHeight >= 3.8)
		)
		{
			fScale		= 3.8;
			pMParm->mDispWidth	= (uint32_t)(pMParm->mTarget_CropInfo.srcW* fScale);
			pMParm->mDispHeight = (uint32_t)(pMParm->mTarget_CropInfo.srcH * fScale);
		}
		else
#endif
		{
			pMParm->mDispWidth		= pMParm->mTarget_CropInfo.dstW;
			pMParm->mDispHeight 	= pMParm->mTarget_CropInfo.dstH;
		}

		pMParm->mDispWidth	= ((pMParm->mDispWidth+3)>>2)<<2;
		pMParm->mDispHeight = pMParm->mFinal_DPHeight = ((pMParm->mDispHeight+3)>>2)<<2;
		DBUG("L: Scaler Out : %d x %d", pMParm->mDispWidth, pMParm->mDispHeight);

		pMParm->mFinal_stride = 0;
		stride_width = pMParm->mTarget_CropInfo.srcW- stride_check_width;//SrcWidth;
		if(stride_width)
		{
			pMParm->mFinal_stride = ((stride_width*pMParm->mDispWidth) / pMParm->mTarget_CropInfo.srcW);// + 1;
			pMParm->mFinal_stride = ((pMParm->mFinal_stride+1)>>1)<<1;
		}

		pMParm->mFinal_DPWidth = pMParm->mDispWidth - pMParm->mFinal_stride;
		pMParm->mFinal_DPWidth = (pMParm->mFinal_DPWidth>>2)<<2; // for 16x multiple width on LCDC..
		pMParm->mRight_truncation = pMParm->mFinal_stride;
		DBUG("L: transform/Crop Out : %d x %d, width_stride: %d", pMParm->mFinal_DPWidth, pMParm->mFinal_DPHeight, pMParm->mFinal_stride);
	}

	if(pMParm->mFinal_DPWidth > pMParm->mLcd_width)
		pMParm->mFinal_DPWidth = pMParm->mLcd_width;

	if(pMParm->mFinal_DPHeight > pMParm->mLcd_height)
		pMParm->mFinal_DPHeight = pMParm->mLcd_height;

	if(pMParm->mFinal_DPWidth != pMParm->mDispWidth || pMParm->mFinal_DPHeight != pMParm->mDispHeight)
		pMParm->mNeed_crop = true;
	else
		pMParm->mNeed_crop = false;

	overlayInit(&pMParm->mScaler_arg, &pMParm->mGrp_arg, false);

	LOGI("L: Overlay :: isParallel(%d, %d/%d), src([o]%d x %d, [t]%d x %d) => S_Out(%d x %d) => G_Out(%d x %d) !!",
							pMParm->mParallel, pMParm->mNeed_scaler, (pMParm->mNeed_transform || pMParm->mNeed_crop), SrcWidth, SrcHeight, pMParm->mTarget_CropInfo.srcW, pMParm->mTarget_CropInfo.srcH, pMParm->mDispWidth, pMParm->mDispHeight, pMParm->mFinal_DPWidth, pMParm->mFinal_DPHeight);

	pMParm->mTarget_DispInfo.width = pMParm->mFinal_DPWidth;
	pMParm->mTarget_DispInfo.height = pMParm->mFinal_DPHeight;

	if(pMParm->mOrigin_DispInfo.width != pMParm->mTarget_DispInfo.width){
		if(pMParm->mOrigin_DispInfo.width > pMParm->mTarget_DispInfo.width){
			gap_width = pMParm->mOrigin_DispInfo.width - pMParm->mTarget_DispInfo.width;
			pMParm->mTarget_DispInfo.sx = pMParm->mOrigin_DispInfo.sx + (gap_width/2);
		}
		else{
			gap_width = pMParm->mTarget_DispInfo.width - pMParm->mOrigin_DispInfo.width;
			if(pMParm->mOrigin_DispInfo.sx <= (gap_width/2))
				pMParm->mTarget_DispInfo.sx = 0;
			else
				pMParm->mTarget_DispInfo.sx = pMParm->mOrigin_DispInfo.sx - (gap_width/2);
		}
	}

	if(pMParm->mOrigin_DispInfo.height != pMParm->mTarget_DispInfo.height){
		if(pMParm->mOrigin_DispInfo.height > pMParm->mTarget_DispInfo.height){
			gap_height = pMParm->mOrigin_DispInfo.height - pMParm->mTarget_DispInfo.height;
			pMParm->mTarget_DispInfo.sy = pMParm->mOrigin_DispInfo.sy + (gap_height/2);
		}
		else{
			gap_height = pMParm->mTarget_DispInfo.height - pMParm->mOrigin_DispInfo.height;
			if(pMParm->mOrigin_DispInfo.sy <= (gap_height/2))
				pMParm->mTarget_DispInfo.sy = 0;
			else
				pMParm->mTarget_DispInfo.sy = pMParm->mOrigin_DispInfo.sy - (gap_height/2);
		}
	}

	if((pMParm->mTarget_DispInfo.sx + pMParm->mTarget_DispInfo.width) > pMParm->mLcd_width)
	{
		if(pMParm->mTarget_DispInfo.width > pMParm->mLcd_width)
			return false;
		else
		{
			pMParm->mTarget_DispInfo.sx = pMParm->mLcd_width - pMParm->mTarget_DispInfo.width;
		}
	}

	if((pMParm->mTarget_DispInfo.sy + pMParm->mTarget_DispInfo.height) > pMParm->mLcd_height)
	{
		if(pMParm->mTarget_DispInfo.height > pMParm->mLcd_height)
			return false;
		else
		{
			pMParm->mTarget_DispInfo.sy = pMParm->mLcd_height - pMParm->mTarget_DispInfo.height;
		}
	}

	LOGI("L: LCD Pos :: %d,%d ~ %d,%d,%d", pMParm->mTarget_DispInfo.sx, pMParm->mTarget_DispInfo.sy, pMParm->mTarget_DispInfo.width, pMParm->mTarget_DispInfo.height, pMParm->mTarget_DispInfo.transform);

	if(!reCheck){
		pMParm->mFirst_Frame = pMParm->mOrder_Parallel = true;
	}
	else
	{
		pMParm->mFirst_Frame = true;
	}

	if(pMParm->mCropRgn_changed)
		pMParm->mCropRgn_changed = false;

	return true;
}

int HwRenderer::overlayResizeExternalDisplay(OUTPUT_SELECT type, int flag)
{
	tcc_display_size display_size;
	char value[PROPERTY_VALUE_MAX];
	unsigned int resize_setting_up, resize_setting_down, resize_setting_left, resize_setting_right, resize_value;
	unsigned int resize_result = 0;

	switch(type)
	{
		case OUTPUT_HDMI:
			property_get("persist.sys.hdmi_resize_up", value, "");
			resize_setting_up = atoi(value);
			property_get("persist.sys.hdmi_resize_dn", value, "");
			resize_setting_down = atoi(value);
			property_get("persist.sys.hdmi_resize_lt", value, "");
			resize_setting_left = atoi(value);
			property_get("persist.sys.hdmi_resize_rt", value, "");
			resize_setting_right = atoi(value);
			break;
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE 
		case OUTPUT_COMPOSITE:
			property_get("persist.sys.composite_resize_up", value, "");
			resize_setting_up = atoi(value);
			property_get("persist.sys.composite_resize_dn", value, "");
			resize_setting_down = atoi(value);
			property_get("persist.sys.composite_resize_lt", value, "");
			resize_setting_left = atoi(value);
			property_get("persist.sys.composite_resize_rt", value, "");
			resize_setting_right = atoi(value);
			break;

		case OUTPUT_COMPONENT:
			property_get("persist.sys.component_resize_up", value, "");
			resize_setting_up = atoi(value);
			property_get("persist.sys.component_resize_dn", value, "");
			resize_setting_down = atoi(value);
			property_get("persist.sys.component_resize_lt", value, "");
			resize_setting_left = atoi(value);
			property_get("persist.sys.component_resize_rt", value, "");
			resize_setting_right = atoi(value);
			break;
#endif 

		default:
			display_size.width	= pMParm->mLcd_width;
			display_size.height	= pMParm->mLcd_height;
			resize_setting_up = resize_setting_down = resize_setting_left = resize_setting_right = 0;
			break;
	}

	if(flag == 0){
		resize_result = (resize_setting_left+resize_setting_right) << 3;
	} else if(flag == 1){
		resize_result = (resize_setting_up+resize_setting_down) << 2;
	}

	return resize_result;
}

int HwRenderer::overlayResizeGetPosition(OUTPUT_SELECT type, int flag)
{
	tcc_display_size display_size;
	char value[PROPERTY_VALUE_MAX];
	unsigned int resize_setting_up, resize_setting_down, resize_setting_left, resize_setting_right, resize_setting_sum;
	unsigned int resize_unit_value, resize_position;

	switch(type)
	{
		case OUTPUT_HDMI:
			property_get("persist.sys.hdmi_resize_up", value, "");
			resize_setting_up = atoi(value);
			property_get("persist.sys.hdmi_resize_dn", value, "");
			resize_setting_down = atoi(value);
			property_get("persist.sys.hdmi_resize_lt", value, "");
			resize_setting_left = atoi(value);
			property_get("persist.sys.hdmi_resize_rt", value, "");
			resize_setting_right = atoi(value);
			break;
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE
		case OUTPUT_COMPOSITE:
			property_get("persist.sys.composite_resize_up", value, "");
			resize_setting_up = atoi(value);
			property_get("persist.sys.composite_resize_dn", value, "");
			resize_setting_down = atoi(value);
			property_get("persist.sys.composite_resize_lt", value, "");
			resize_setting_left = atoi(value);
			property_get("persist.sys.composite_resize_rt", value, "");
			resize_setting_right = atoi(value);
			break;

		case OUTPUT_COMPONENT:
			property_get("persist.sys.component_resize_up", value, "");
			resize_setting_up = atoi(value);
			property_get("persist.sys.component_resize_dn", value, "");
			resize_setting_down = atoi(value);
			property_get("persist.sys.component_resize_lt", value, "");
			resize_setting_left = atoi(value);
			property_get("persist.sys.component_resize_rt", value, "");
			resize_setting_right = atoi(value);
			break;
#endif
		default:
			display_size.width	= pMParm->mLcd_width;
			display_size.height	= pMParm->mLcd_height;
			resize_setting_up = resize_setting_down = resize_setting_left = resize_setting_right = 0;
			break;
	}

	if(pMParm->mMVCmode) {
		resize_setting_up = resize_setting_down = resize_setting_left = resize_setting_right = 0;
	}

	resize_position = 0;

	if(flag == 0){
		if(resize_setting_left)
		{
			resize_setting_sum = resize_setting_left+resize_setting_right;
			resize_unit_value = overlayResizeExternalDisplay(type, flag)/resize_setting_sum;
			resize_position = resize_unit_value * resize_setting_left;
		}
	} else if(flag == 1){
		if(resize_setting_up)
		{
			resize_setting_sum = resize_setting_up+resize_setting_down;
			resize_unit_value = overlayResizeExternalDisplay(type, flag)/resize_setting_sum;
			resize_position = resize_unit_value * resize_setting_up;
		}
	}

	return resize_position;
}

bool HwRenderer::overlayCheckExternalDisplay(bool reCheck, int outSx, int outSy, int outWidth, int outHeight, unsigned int rotation)
{
	char value[PROPERTY_VALUE_MAX];
	float fScaleWidth, fScaleHeight, fScale;
	unsigned int SrcWidth, SrcHeight; // source buffer size
	unsigned int stride_width, final_stride;
	// Real_target to display. set real position in lcd!!
	unsigned int target_sx, target_sy, target_width, target_height, transform;
	int gap_width, gap_height;
	int CommitResult = 1;
	unsigned int out_width, out_height;
	int ChangedPos = 0;
	tcc_display_size DisplaySize;
	unsigned int mscreen;
	int lcd_width_r, lcd_height_r;
	OUTPUT_SELECT output_type;

	property_get("tcc.dxb.fullscreen",value,"");
	mscreen = atoi(value);

	lcd_width_r = pMParm->mLcd_width;
	lcd_height_r = pMParm->mLcd_height;

	if(pMParm->mHDMIOutput)
		output_type = OUTPUT_HDMI;
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE 
	else if(pMParm->mCompositeOutput)
		output_type = OUTPUT_COMPOSITE;
	else if(pMParm->mComponentOutput)
		output_type = OUTPUT_COMPONENT;
#endif
	else
		output_type = OUTPUT_NONE;
	if((pMParm->mLcd_height > pMParm->mLcd_width) && (rotation != 0))
	{
		int temp = 0;

		if(rotation == HAL_TRANSFORM_ROT_270) {
			temp = outSx;
			outSx = pMParm->mLcd_height - outHeight - outSy;
			outSy = temp;
		}
		else if(rotation == HAL_TRANSFORM_ROT_90) {
			temp = outSx;
			outSx = outSy;
			outSy = pMParm->mLcd_width - outWidth - temp;
		}
		else if(rotation == HAL_TRANSFORM_ROT_180) {
			temp = outSx;
			outSx = outSy;
			outSy = pMParm->mLcd_width - outWidth - temp;
		}

		temp = outWidth;
		outWidth = outHeight;
		outHeight = temp;

		rotation = 0;
		lcd_width_r = pMParm->mLcd_height;
		lcd_height_r = pMParm->mLcd_width;
	}


	if(outSx < 0){
		outWidth += outSx;
		outSx = 0;
	}
	if(outSy < 0){
		outHeight += outSy;
		outSy = 0;
	}

	if(reCheck)
	{
		target_sx       = outSx;
		target_sy       = outSy;
		target_width    = outWidth;
		target_height   = outHeight;

		transform       = rotation;

		if(pMParm->mLcd_height < pMParm->mLcd_width)
		{
			if(target_width > (lcd_width_r - target_sx))
				target_width = (lcd_width_r - target_sx);

			if(target_height > (lcd_height_r - target_sy))
				target_height = (lcd_height_r - target_sy);
		}

		if(CommitResult)
		{
			if((pMParm->mOrigin_DispInfo.sx != target_sx) || (pMParm->mOrigin_DispInfo.sy != target_sy) ||
				(pMParm->mOrigin_DispInfo.width != target_width) || (pMParm->mOrigin_DispInfo.height != target_height) || (pMParm->mOrigin_DispInfo.transform != transform)
			)
			{
				if((target_width == 0) || (target_height == 0))
				{
					LOGE("E :: Operation size is abnormal. (%d x %d)", target_width, target_height);
					return false;
				}

				pMParm->mOrigin_DispInfo.sx 		= target_sx;
				pMParm->mOrigin_DispInfo.sy 		= target_sy;
				pMParm->mOrigin_DispInfo.width 		= target_width;
				pMParm->mOrigin_DispInfo.height 	= target_height;
				pMParm->mOrigin_DispInfo.transform 	= transform;

				ChangedPos = 1;
				LOGI("E :: Overlay reSetting In :: %d,%d ~ %d,%d, %d => %d,%d ~ %d,%d, %d", outSx, outSy, outWidth, outHeight, rotation,
					pMParm->mOrigin_DispInfo.sx, pMParm->mOrigin_DispInfo.sy, pMParm->mOrigin_DispInfo.width, pMParm->mOrigin_DispInfo.height, pMParm->mOrigin_DispInfo.transform);
			}
		}

		if(pMParm->mCropRgn_changed)
			ChangedPos = 1;
	}
	else
	{
		pMParm->mOrigin_DispInfo.sx       = outSx;
		pMParm->mOrigin_DispInfo.sy       = outSy;
		pMParm->mOrigin_DispInfo.width    = outWidth;
		pMParm->mOrigin_DispInfo.height   = outHeight;

		pMParm->mOrigin_DispInfo.transform  = rotation;

		if(pMParm->mLcd_height < pMParm->mLcd_width)
		{
			if(pMParm->mOrigin_DispInfo.width > (lcd_width_r - pMParm->mOrigin_DispInfo.sx))
				pMParm->mOrigin_DispInfo.width = (lcd_width_r - pMParm->mOrigin_DispInfo.sx);

			if(pMParm->mOrigin_DispInfo.height > (lcd_height_r - pMParm->mOrigin_DispInfo.sy))
				pMParm->mOrigin_DispInfo.height = (lcd_height_r - pMParm->mOrigin_DispInfo.sy);
		}

		if(CommitResult)
		{
			if((pMParm->mOrigin_DispInfo.width == 0) || (pMParm->mOrigin_DispInfo.height == 0))
			{
				LOGE("E :: Operation size is abnormal. (%d x %d)", pMParm->mOrigin_DispInfo.width, pMParm->mOrigin_DispInfo.height);
				return false;
			}

			pMParm->mOverlay_recheck = true;
			LOGI("E :: Overlay first Setting In :: pMParm->mOrigin_DispInfo %d,%d ~ %d,%d, %d => %d,%d ~ %d,%d, %d", outSx, outSy, outWidth, outHeight, rotation,
                            pMParm->mOrigin_DispInfo.sx, pMParm->mOrigin_DispInfo.sy, pMParm->mOrigin_DispInfo.width, pMParm->mOrigin_DispInfo.height, pMParm->mOrigin_DispInfo.transform);
		}
		else{
			return false;
		}

		pMParm->mOverlay_recheck = true;
	}

	// check external HDMI size
	if(pMParm->mHDMIOutput)
	{
		getDisplaySize(OUTPUT_HDMI, &DisplaySize);
		out_width = DisplaySize.width;
		out_height = DisplaySize.height;
	}
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE
	else if(pMParm->mCompositeOutput)
	{
		getDisplaySize(OUTPUT_COMPOSITE, &DisplaySize);
		out_width = DisplaySize.width;
		out_height = DisplaySize.height;
	}
	else if(pMParm->mComponentOutput)
	{
		getDisplaySize(OUTPUT_COMPONENT, &DisplaySize);
		out_width = DisplaySize.width;
		out_height = DisplaySize.height;
	}
#endif  
	else
	{
		out_width	= lcd_width_r;
		out_height	= lcd_height_r;
	}

	if(reCheck)
	{
		if(!ChangedPos && (pMParm->mOutput_width == out_width) && (pMParm->mOutput_height == out_height))
			return false;
	}

	pMParm->mOutput_width = out_width;
	pMParm->mOutput_height = out_height;
	SrcWidth = ((mDecodedWidth + 15)>>4)<<4;
	SrcHeight = ((mDecodedHeight + 1)>>1)<<1;

	if(mscreen == 1)
		pMParm->mCropChanged = 1;

	//LOGI("check get pMParm->mHDMIUiSize : %s pMParm->mDisplayOutputSTB : %d pMParm->mComponentChip %s", pMParm->mHDMIUiSize,pMParm->mDisplayOutputSTB,pMParm->mComponentChip);
    LOGI("check get pMParm->mHDMIUiSize : %s pMParm->mDisplayOutputSTB : %d", pMParm->mHDMIUiSize,pMParm->mDisplayOutputSTB);
    if( (strcasecmp(pMParm->mHDMIUiSize, "1280x720")!=0) && (pMParm->mOrigin_DispInfo.width == lcd_width_r && pMParm->mOrigin_DispInfo.height == lcd_height_r)
		&& !pMParm->mCropChanged
	)
	{
		fScaleWidth = (float)pMParm->mOutput_width / SrcWidth;
		fScaleHeight = (float)pMParm->mOutput_height/ SrcHeight;
		if(fScaleWidth > fScaleHeight)
			fScale = fScaleHeight;
		else
			fScale = fScaleWidth;

		pMParm->mTarget_CropInfo.sx = (SrcWidth - mDecodedWidth)/2;
		pMParm->mTarget_CropInfo.sy = (SrcHeight - mDecodedHeight)/2;
		pMParm->mTarget_CropInfo.srcW = SrcWidth;
		pMParm->mTarget_CropInfo.srcH = SrcHeight;
		pMParm->mTarget_CropInfo.dstW = mDecodedWidth * fScale;
		pMParm->mTarget_CropInfo.dstH = mDecodedHeight * fScale;

		pMParm->mTarget_DispInfo.sx = pMParm->mTarget_DispInfo.sy = 0;
		if(pMParm->mOutput_width > pMParm->mTarget_CropInfo.dstW)
			pMParm->mTarget_DispInfo.sx = (pMParm->mOutput_width - pMParm->mTarget_CropInfo.dstW)/2;
		if(pMParm->mOutput_height > pMParm->mTarget_CropInfo.dstH)
			pMParm->mTarget_DispInfo.sy = (pMParm->mOutput_height- pMParm->mTarget_CropInfo.dstH)/2;
		pMParm->mTarget_DispInfo.width	= pMParm->mTarget_CropInfo.dstW;
		pMParm->mTarget_DispInfo.height = pMParm->mTarget_CropInfo.dstH;

		LOGI("E(%dx%d -> %dx%d) :: reSized_info :1: %d,%d ~ %d,%d",
			lcd_width_r, lcd_height_r, pMParm->mOutput_width, pMParm->mOutput_height,
			pMParm->mTarget_DispInfo.sx, pMParm->mTarget_DispInfo.sy, pMParm->mTarget_DispInfo.width, pMParm->mTarget_DispInfo.height);
	}
	else
	{
		int end_x, end_y;

		pMParm->mTarget_DispInfo.sx = (pMParm->mOutput_width * pMParm->mOrigin_DispInfo.sx)/lcd_width_r;
		pMParm->mTarget_DispInfo.sy = (pMParm->mOutput_height* pMParm->mOrigin_DispInfo.sy)/lcd_height_r;
		end_x = (pMParm->mOutput_width * (pMParm->mOrigin_DispInfo.sx + pMParm->mOrigin_DispInfo.width))/lcd_width_r;
		end_y = (pMParm->mOutput_height * (pMParm->mOrigin_DispInfo.sy + pMParm->mOrigin_DispInfo.height))/lcd_height_r;
		if( end_x > pMParm->mOutput_width)
			pMParm->mTarget_DispInfo.width  = pMParm->mOutput_width - pMParm->mTarget_DispInfo.sx;
		else
			pMParm->mTarget_DispInfo.width  = end_x - pMParm->mTarget_DispInfo.sx;

		if( end_y > pMParm->mOutput_height)
			pMParm->mTarget_DispInfo.height = pMParm->mOutput_height - pMParm->mTarget_DispInfo.sy;
		else
			pMParm->mTarget_DispInfo.height = end_y - pMParm->mTarget_DispInfo.sy;

		pMParm->mTarget_DispInfo.transform	= pMParm->mOrigin_DispInfo.transform;
		LOGI("E(%dx%d -> %dx%d) :: reSized_info :2: %d,%d ~ %d,%d (pos: %d,%d)",
			lcd_width_r, lcd_height_r, pMParm->mOutput_width, pMParm->mOutput_height,
			pMParm->mTarget_DispInfo.sx, pMParm->mTarget_DispInfo.sy, pMParm->mTarget_DispInfo.width, pMParm->mTarget_DispInfo.height,
			pMParm->mTarget_DispInfo.sx+pMParm->mTarget_DispInfo.width, pMParm->mTarget_DispInfo.sy+pMParm->mTarget_DispInfo.height);

		fScaleWidth = (float)pMParm->mTarget_DispInfo.width / SrcWidth;
		fScaleHeight = (float)pMParm->mTarget_DispInfo.height / SrcHeight;
		if(fScaleWidth > fScaleHeight)
			fScale = fScaleHeight;
		else
			fScale = fScaleWidth;

		/* for crop
		pMParm->mTarget_CropInfo.sx = pMParm->mTarget_CropInfo.sy = 0;
		if(SrcWidth > mDecodedWidth)
			pMParm->mTarget_CropInfo.sx = (SrcWidth - mDecodedWidth)/2;
		if(SrcHeight > mDecodedHeight)
			pMParm->mTarget_CropInfo.sy = (SrcHeight - mDecodedHeight)/2;

		pMParm->mTarget_CropInfo.srcW = SrcWidth;
		pMParm->mTarget_CropInfo.srcH = SrcHeight;
		*/


		if(strcasecmp(pMParm->mHDMIUiSize, "1280x720")!=0){
			pMParm->mTarget_CropInfo.dstW = pMParm->mTarget_DispInfo.width;
			pMParm->mTarget_CropInfo.dstH = pMParm->mTarget_DispInfo.height;
		}
		else{
			if(pMParm->mIgnore_ratio){
				pMParm->mTarget_CropInfo.dstW = pMParm->mTarget_DispInfo.width;
				pMParm->mTarget_CropInfo.dstH = pMParm->mTarget_DispInfo.height;
			}
			else{
				pMParm->mTarget_CropInfo.dstW = mDecodedWidth * fScale;
				pMParm->mTarget_CropInfo.dstH = mDecodedHeight * fScale;
			}
		}
	}

	pMParm->mNeed_scaler = true;
	pMParm->mNeed_transform  = pMParm->mNeed_rotate = false;
	pMParm->mNeed_crop = false;

	DBUG("E :: Overlay Check In!!, SRC::%d x %d	 TAR:: %d x %d", mDecodedWidth, mDecodedHeight, pMParm->mTarget_CropInfo.dstW, pMParm->mTarget_CropInfo.dstH);

	// TCC92xx block limitation check	>>
	SrcWidth = pMParm->mTarget_CropInfo.srcW;
	SrcHeight = pMParm->mTarget_CropInfo.srcH;

	fScaleWidth 	= (float)pMParm->mTarget_CropInfo.dstW / pMParm->mTarget_CropInfo.srcW;
	fScaleHeight	= (float)pMParm->mTarget_CropInfo.dstH / pMParm->mTarget_CropInfo.srcH;

	//Scaler  limitation!!
#ifdef USE_LIMITATION_SCALER_RATIO
	if(fScaleWidth >= 3.8 || fScaleHeight >= 3.8)
	{
		if(fScaleWidth > fScaleHeight)
			fScale = fScaleHeight;
		else
			fScale = fScaleWidth;

		if(fScale > 3.8)
			fScale = 3.8;

		if(pMParm->mIgnore_ratio)
		{
			if(fScaleWidth > fScaleHeight)
			{
				pMParm->mDispWidth	= (uint32_t)(pMParm->mTarget_CropInfo.srcW * 3.8);
				pMParm->mDispHeight = (uint32_t)(pMParm->mTarget_CropInfo.srcH * fScale);
			}
			else
			{
				pMParm->mDispWidth	= (uint32_t)(pMParm->mTarget_CropInfo.srcW * fScale);
				pMParm->mDispHeight = (uint32_t)(pMParm->mTarget_CropInfo.srcH * 3.8);
			}
		}
		else
		{
			pMParm->mDispWidth	= (uint32_t)(pMParm->mTarget_CropInfo.srcW * fScale);
			pMParm->mDispHeight = (uint32_t)(pMParm->mTarget_CropInfo.srcH * fScale);
		}
	}
	else
#endif
	{
		pMParm->mDispWidth		= pMParm->mTarget_CropInfo.dstW;
		pMParm->mDispHeight 	= pMParm->mTarget_CropInfo.dstH;
		if(pMParm->mDisplayOutputSTB){
			if(pMParm->mDispWidth == 960 && pMParm->mDispHeight >540)
				pMParm->mDispHeight = 540;
		}
	}

	pMParm->mDispWidth	= ((pMParm->mDispWidth+3)>>2)<<2;
	pMParm->mDispHeight = pMParm->mFinal_DPHeight = ((pMParm->mDispHeight+3)>>2)<<2;

	/* Display Position setting!!*/
	#if 1 //DDR3 scaler limitation
	if((pMParm->mTarget_CropInfo.dstW == 1920) && (pMParm->mTarget_CropInfo.dstH == 1080) && (SrcWidth == 960) && (SrcHeight == 540))
	{
		pMParm->mDispWidth	= 1912;
	}
	#endif//

	DBUG("E :: Display Position setting:pMParm->mTarget_DispInfo %d,%d ~ %d,%d DispWidth %d Height %d ", pMParm->mTarget_DispInfo.sx, pMParm->mTarget_DispInfo.sy, pMParm->mTarget_DispInfo.width, pMParm->mTarget_DispInfo.height,pMParm->mDispWidth,pMParm->mDispHeight);
	if(pMParm->mTarget_DispInfo.width >= pMParm->mDispWidth) {
		pMParm->mTarget_DispInfo.sx += (pMParm->mTarget_DispInfo.width - pMParm->mDispWidth)/2;
	} else {
		if(pMParm->mTarget_DispInfo.sx >= ((pMParm->mDispWidth- pMParm->mTarget_DispInfo.width)/2))
			pMParm->mTarget_DispInfo.sx -= (pMParm->mDispWidth- pMParm->mTarget_DispInfo.width)/2;
		else
			pMParm->mTarget_DispInfo.sx = 0;
	}

	if(pMParm->mTarget_DispInfo.height >= pMParm->mDispHeight) {
		pMParm->mTarget_DispInfo.sy += (pMParm->mTarget_DispInfo.height- pMParm->mDispHeight)/2;
	} else {
		if(pMParm->mTarget_DispInfo.sy >= ((pMParm->mDispHeight - pMParm->mTarget_DispInfo.height)/2))
			pMParm->mTarget_DispInfo.sy -= (pMParm->mDispHeight - pMParm->mTarget_DispInfo.height)/2;
		else
			pMParm->mTarget_DispInfo.sy = 0;
	}
	pMParm->mTarget_DispInfo.width	= pMParm->mDispWidth;
	pMParm->mTarget_DispInfo.height = pMParm->mDispHeight;

	//if(pMParm->mDisplayOutputSTB)
	{
		pMParm->mDispWidth	= pMParm->mDispWidth * (out_width - overlayResizeExternalDisplay(output_type, 0)) / out_width ;
		pMParm->mDispHeight	= pMParm->mDispHeight * (out_height - overlayResizeExternalDisplay(output_type, 1)) / out_height;

		pMParm->mTarget_DispInfo.sx		= (out_width - overlayResizeExternalDisplay(output_type, 0)) * pMParm->mTarget_DispInfo.sx / out_width + overlayResizeGetPosition(output_type, 0);
		pMParm->mTarget_DispInfo.sy		= (out_height - overlayResizeExternalDisplay(output_type, 1)) * pMParm->mTarget_DispInfo.sy / out_height + overlayResizeGetPosition(output_type, 1);
		pMParm->mTarget_DispInfo.width	= pMParm->mTarget_DispInfo.width  * (out_width - overlayResizeExternalDisplay(output_type, 0)) / out_width;
		pMParm->mTarget_DispInfo.height	= pMParm->mTarget_DispInfo.height * (out_height - overlayResizeExternalDisplay(output_type, 1)) / out_height;
	}

	DBUG("E :: Display Position setting:pMParm->mTarget_DispInfo %d,%d ~ %d,%d", pMParm->mTarget_DispInfo.sx, pMParm->mTarget_DispInfo.sy, pMParm->mTarget_DispInfo.width, pMParm->mTarget_DispInfo.height);

	overlayInit(&pMParm->mScaler_arg, &pMParm->mGrp_arg, false);

	LOGE("E :: Overlay :: isParallel(%d, %d/%d), src([o]%d x %d, [t]%d x %d) => S_Out(%d x %d) !!",  pMParm->mParallel, pMParm->mNeed_scaler, (pMParm->mNeed_transform || pMParm->mNeed_crop), SrcWidth, SrcHeight, pMParm->mTarget_CropInfo.srcW, pMParm->mTarget_CropInfo.srcH, pMParm->mDispWidth, pMParm->mDispHeight);

	/* open scaler device */
	if(!reCheck){
		pMParm->mFirst_Frame = pMParm->mOrder_Parallel = true;
	}
	else{
		pMParm->mFirst_Frame = true;
	}
	if(pMParm->mCropRgn_changed)
		pMParm->mCropRgn_changed = false;

	return true;
}

void HwRenderer::overlayScreenModeCalculate(float fCurrentCheckDisp, bool bCut)
{
	int hasStreamAspectRatio = 0;
	float fCheckDecodedInfo = (float)mDecodedWidth / mDecodedHeight;
	switch(pMParm->mStreamAspectRatio)
	{
		case 0: //16:9
			fCheckDecodedInfo = 16.0/9.0;
			hasStreamAspectRatio = 1;
			break;
		case 1:  //4:3
			fCheckDecodedInfo = 4.0/3.0;
			hasStreamAspectRatio = 1;
			break;
		default: //-1 : don't have stream aspect ratio
			hasStreamAspectRatio = 0;
			break;
	}

	if(pMParm->mScreenModeIndex == 2 || fCurrentCheckDisp == fCheckDecodedInfo)
	{
		pMParm->mScreenMode_DispInfo.sx = pMParm->mOrigin_DispInfo.sx;
		pMParm->mScreenMode_DispInfo.sy = pMParm->mOrigin_DispInfo.sy;
		pMParm->mScreenMode_DispInfo.width = pMParm->mOrigin_DispInfo.width;
		pMParm->mScreenMode_DispInfo.height = pMParm->mOrigin_DispInfo.height;
		pMParm->mScreenMode_DispInfo.transform = pMParm->mOrigin_DispInfo.transform;
		//setCropRegion(mDecodedWidth, mDecodedHeight, 0, 0, ((mDecodedWidth+15)>>4)<<4, mDecodedHeight, pMParm->mScreenMode_DispInfo.width, pMParm->mScreenMode_DispInfo.height, 0);
	}
	else if(fCurrentCheckDisp < fCheckDecodedInfo)
	{
		pMParm->mScreenMode_DispInfo.sx = pMParm->mOrigin_DispInfo.sx;
		pMParm->mScreenMode_DispInfo.width = pMParm->mOrigin_DispInfo.width;

		pMParm->mScreenMode_DispInfo.height = (unsigned int)(pMParm->mOrigin_DispInfo.width/fCheckDecodedInfo);
		pMParm->mScreenMode_DispInfo.sy = pMParm->mOrigin_DispInfo.sy + ((pMParm->mOrigin_DispInfo.height-pMParm->mScreenMode_DispInfo.height)/2);

		pMParm->mScreenMode_DispInfo.transform = pMParm->mOrigin_DispInfo.transform;

		if(bCut == true)
		{
			int sW, srcW;

			pMParm->mScreenMode_DispInfo.sy = pMParm->mOrigin_DispInfo.sy;
			pMParm->mScreenMode_DispInfo.height = pMParm->mOrigin_DispInfo.height;

			if(hasStreamAspectRatio)
			{
				srcW = (int)(mDecodedWidth*fCurrentCheckDisp/fCheckDecodedInfo);
			}
			else
			{
				srcW = (int)(mDecodedHeight*fCurrentCheckDisp);
			}
			srcW = ((srcW+15)>>4)<<4;

			sW = (mDecodedWidth - srcW) / 2;
			//setCropRegion(mDecodedWidth, mDecodedHeight, sW, 0, srcW, mDecodedHeight, pMParm->mScreenMode_DispInfo.width, pMParm->mScreenMode_DispInfo.height, 0);
			LOGD("Crop srcW=%d[%f][%f]", srcW, fCurrentCheckDisp, fCheckDecodedInfo);
		}
		else
		{
			//setCropRegion(mDecodedWidth, mDecodedHeight, 0, 0, ((mDecodedWidth+15)>>4)<<4, mDecodedHeight, pMParm->mScreenMode_DispInfo.width, pMParm->mScreenMode_DispInfo.height, 0);
		}
	}
	else if(fCurrentCheckDisp > fCheckDecodedInfo)
	{
		pMParm->mScreenMode_DispInfo.sy = pMParm->mOrigin_DispInfo.sy;
		pMParm->mScreenMode_DispInfo.height = pMParm->mOrigin_DispInfo.height;

		pMParm->mScreenMode_DispInfo.width = (unsigned int)(pMParm->mOrigin_DispInfo.height*fCheckDecodedInfo);
		pMParm->mScreenMode_DispInfo.sx = pMParm->mOrigin_DispInfo.sx + ((pMParm->mOrigin_DispInfo.width-pMParm->mScreenMode_DispInfo.width)/2);

		pMParm->mScreenMode_DispInfo.transform = pMParm->mOrigin_DispInfo.transform;
		if(bCut == true)
		{
			pMParm->mScreenMode_DispInfo.sx = pMParm->mOrigin_DispInfo.sx;
			pMParm->mScreenMode_DispInfo.width = pMParm->mOrigin_DispInfo.width;

			int srcH;
			if(hasStreamAspectRatio)
				srcH = (int)(mDecodedHeight*fCheckDecodedInfo/fCurrentCheckDisp);
			else
				srcH = (int)(mDecodedWidth/fCurrentCheckDisp);
			int sH = (mDecodedHeight - srcH) / 2;
			//setCropRegion(mDecodedWidth, mDecodedHeight, 0, sH, ((mDecodedWidth+15)>>4)<<4, srcH, pMParm->mScreenMode_DispInfo.width, pMParm->mScreenMode_DispInfo.height, 0);
			LOGD("Crop srcH=%d[%f][%f]", srcH, fCurrentCheckDisp, fCheckDecodedInfo);
		}
		else
		{
			//setCropRegion(mDecodedWidth, mDecodedHeight, 0, 0, ((mDecodedWidth+15)>>4)<<4, mDecodedHeight, pMParm->mScreenMode_DispInfo.width, pMParm->mScreenMode_DispInfo.height, 0);
		}
	}
	LOGD("(%d,%d)@%dx%d[%f:%d]", pMParm->mScreenMode_DispInfo.sx, pMParm->mScreenMode_DispInfo.sy, pMParm->mScreenMode_DispInfo.width, pMParm->mScreenMode_DispInfo.height, fCheckDecodedInfo, pMParm->mStreamAspectRatio);
}

bool HwRenderer::overlayCheckDisplayScreenMode(bool reCheck, int outSx, int outSy, int outWidth, int outHeight, unsigned int rotation)
{
	float fScaleWidth, fScaleHeight, fScaleTotal, fScale;
	unsigned int SrcWidth, SrcHeight;
	unsigned int stride_width, final_stride;
	// Real_target to display. set real position in lcd!!
	unsigned int target_sx, target_sy, target_width, target_height, transform;
	unsigned int gap_width, gap_height;
	unsigned int stride_check_width;
	int CommitResult = 1;

	char value[PROPERTY_VALUE_MAX];
	unsigned int uiLen = 0, uiMode = 0, uiPreviewMode = 0;;
	uiLen = property_get ("tcc.solution.screenmode.index", value, "");

	if(uiLen)
	{
		uiMode = atoi(value);
		//LOGD("property_get[tcc.solution.screenmode.index]::%d", uiMode);
	}
	else
	{
		LOGD("property_get[tcc.solution.screenmode.index]::%d", uiMode);
		property_set("tcc.solution.screenmode.index", "0");
	}

	if(outSx < 0){
		outWidth += outSx;
		outSx = 0;
	}
	if(outSy < 0){
		outHeight += outSy;
		outSy = 0;
	}

	if(reCheck)
	{
		float fCurrentCheckDisp = (float)pMParm->mOrigin_DispInfo.width/pMParm->mOrigin_DispInfo.height;

		//in case of change in playing!!
		target_sx       = outSx;
		target_sy       = outSy;
		target_width    = outWidth;
		target_height   = outHeight;
		transform       = rotation;

		if(CommitResult)
		{
			uiLen = property_get ("tcc.solution.preview", value, "");

			if(uiLen)
			{
				uiPreviewMode = atoi(value);
			}
			else
			{
				LOGD("property_get[tcc.solution.preview]::%d", uiPreviewMode);
				property_set("tcc.solution.preview", "0");
			}

			if(uiPreviewMode == 1)
			{
				target_width = pMParm->mLcd_width / 2;
				target_height = target_width * 3 / 4;
				target_sx = pMParm->mLcd_width - target_width;
				target_sy = (pMParm->mLcd_height - target_height) / 2;
			}
			else if(uiPreviewMode == 0)
			{
				target_sx = 0;
				target_sy = 0;
				target_width = pMParm->mLcd_width;
				target_height = pMParm->mLcd_height;
			}

			if(target_width > (pMParm->mLcd_width - target_sx))
				target_width = (pMParm->mLcd_width - target_sx);

			if(target_height > (pMParm->mLcd_height - target_sy))
				target_height = (pMParm->mLcd_height - target_sy);

			if((pMParm->mOrigin_DispInfo.sx != target_sx) || (pMParm->mOrigin_DispInfo.sy != target_sy) ||
				(pMParm->mOrigin_DispInfo.width != target_width) || (pMParm->mOrigin_DispInfo.height != target_height) || (pMParm->mOrigin_DispInfo.transform != transform)
			)
			{
				pMParm->mOrigin_DispInfo.sx 		= target_sx;
				pMParm->mOrigin_DispInfo.sy 		= target_sy;
				pMParm->mOrigin_DispInfo.width 		= target_width;
				pMParm->mOrigin_DispInfo.height 	= target_height;
				pMParm->mOrigin_DispInfo.transform 	= transform;

				LOGI("Overlay reSetting In :: %d,%d ~ %d,%d, %d", pMParm->mOrigin_DispInfo.sx, pMParm->mOrigin_DispInfo.sy, pMParm->mOrigin_DispInfo.width, pMParm->mOrigin_DispInfo.height, pMParm->mOrigin_DispInfo.transform);

				pMParm->mScreenModeIndex = uiMode;
				switch(pMParm->mScreenModeIndex)
				{
					case 0:
						{
							overlayScreenModeCalculate(fCurrentCheckDisp, false);
						}
						break;

					case 1:
						{
							overlayScreenModeCalculate(fCurrentCheckDisp, true);
						}
						break;

					case 2:
						{
							overlayScreenModeCalculate(fCurrentCheckDisp, false);
						}
						break;
				}
			}
			else if(pMParm->mScreenModeIndex != uiMode)
			{
				pMParm->mScreenModeIndex = uiMode;
				switch(pMParm->mScreenModeIndex)
				{
					case 0:
						{
							overlayScreenModeCalculate(fCurrentCheckDisp, false);
						}
						break;

					case 1:
						{
							overlayScreenModeCalculate(fCurrentCheckDisp, true);
						}
						break;

					case 2:
						{
							overlayScreenModeCalculate(fCurrentCheckDisp, false);
						}
						break;
				}
			}
			else
			{
				if(pMParm->mCropRgn_changed == false){
					return false;
				}
			}
		}
		else
		{
			if(pMParm->mScreenModeIndex != uiMode)
			{
				pMParm->mScreenModeIndex = uiMode;
				switch(pMParm->mScreenModeIndex)
				{
					case 0:
						{
							overlayScreenModeCalculate(fCurrentCheckDisp, false);
						}
						break;

					case 1:
						{
							overlayScreenModeCalculate(fCurrentCheckDisp, true);
						}
						break;

					case 2:
						{
							overlayScreenModeCalculate(fCurrentCheckDisp, false);
						}
						break;
				}
			}
			else if(pMParm->mCropRgn_changed == false){
				return false;
			}
		}
	}
	else
	{
		//first change after playback!!
		pMParm->mOrigin_DispInfo.sx         = outSx;
		pMParm->mOrigin_DispInfo.sy         = outSy;
		pMParm->mOrigin_DispInfo.width      = outWidth;
		pMParm->mOrigin_DispInfo.height     = outHeight;
		pMParm->mOrigin_DispInfo.transform  = rotation;

		if(CommitResult)
		{
			uiLen = property_get ("tcc.solution.preview", value, "");

			if(uiLen)
			{
				uiPreviewMode = atoi(value);
			}
			else
			{
				LOGD("property_get[tcc.solution.preview]::%d", uiPreviewMode);
				property_set("tcc.solution.preview", "0");
			}

			if(uiPreviewMode == 1)
			{
				pMParm->mOrigin_DispInfo.width = pMParm->mLcd_width / 2;
				pMParm->mOrigin_DispInfo.height = pMParm->mOrigin_DispInfo.width * 3 / 4;
				pMParm->mOrigin_DispInfo.sx = pMParm->mLcd_width - pMParm->mOrigin_DispInfo.width;
				pMParm->mOrigin_DispInfo.sy = (pMParm->mLcd_height - pMParm->mOrigin_DispInfo.height) / 2;
			}
			else if(uiPreviewMode == 0)
			{
				pMParm->mOrigin_DispInfo.sx = 0;
				pMParm->mOrigin_DispInfo.sy = 0;
				pMParm->mOrigin_DispInfo.width = pMParm->mLcd_width;
				pMParm->mOrigin_DispInfo.height = pMParm->mLcd_height;
			}

			if(pMParm->mOrigin_DispInfo.width > pMParm->mLcd_width)
				pMParm->mOrigin_DispInfo.width = pMParm->mLcd_width;

			if(pMParm->mOrigin_DispInfo.height > pMParm->mLcd_height)
				pMParm->mOrigin_DispInfo.height = pMParm->mLcd_height;

			float fCurrentCheckDisp = (float)pMParm->mOrigin_DispInfo.width/pMParm->mOrigin_DispInfo.height;
			pMParm->mScreenModeIndex = uiMode;
			switch(pMParm->mScreenModeIndex)
			{
				case 0:
					{
						overlayScreenModeCalculate(fCurrentCheckDisp, false);
					}
					break;

				case 1:
					{
						overlayScreenModeCalculate(fCurrentCheckDisp, true);
					}
					break;

				case 2:
					{
						overlayScreenModeCalculate(fCurrentCheckDisp, false);
					}
					break;
			}

			pMParm->mOverlay_recheck = true;
			LOGI("Overlay first Setting In :: %d,%d ~ %d,%d, %d", pMParm->mOrigin_DispInfo.sx, pMParm->mOrigin_DispInfo.sy, pMParm->mOrigin_DispInfo.width, pMParm->mOrigin_DispInfo.height, pMParm->mOrigin_DispInfo.transform);
		}
		else{
			return false;
		}
	}

	DBUG("Overlay Check In!!, SRC::%d x %d   TAR:: %d x %d", mDecodedWidth, mDecodedHeight, target_width, target_height);
#if 0
	pMParm->mTarget_DispInfo.sx		= pMParm->mOrigin_DispInfo.sx;
	pMParm->mTarget_DispInfo.sy		= pMParm->mOrigin_DispInfo.sy;
	pMParm->mTarget_DispInfo.width	= pMParm->mOrigin_DispInfo.width;
	pMParm->mTarget_DispInfo.height	= pMParm->mOrigin_DispInfo.height;
	pMParm->mTarget_DispInfo.transform  = pMParm->mOrigin_DispInfo.transform;
#else
	pMParm->mTarget_DispInfo.sx		= pMParm->mScreenMode_DispInfo.sx;
	pMParm->mTarget_DispInfo.sy		= pMParm->mScreenMode_DispInfo.sy;
	pMParm->mTarget_DispInfo.width	= pMParm->mScreenMode_DispInfo.width;
	pMParm->mTarget_DispInfo.height	= pMParm->mScreenMode_DispInfo.height;
	pMParm->mTarget_DispInfo.transform  = pMParm->mScreenMode_DispInfo.transform;
#endif

//	pMParm->mOrigin_DispInfo.transform = HAL_TRANSFORM_ROT_270; //to test transform

	pMParm->mNeed_rotate = false;
	if(pMParm->mOrigin_DispInfo.transform)
	{
		pMParm->mNeed_transform = true;
		if(pMParm->mOrigin_DispInfo.transform == HAL_TRANSFORM_ROT_90 || pMParm->mOrigin_DispInfo.transform == HAL_TRANSFORM_ROT_270)
		{
			pMParm->mNeed_rotate = true;
		}
	}
	else
	{
		pMParm->mNeed_transform = false;
	}

	if(!pMParm->mCropChanged)
	{
		SrcWidth = ((mDecodedWidth + 15)>>4)<<4;
		SrcHeight = ((mDecodedHeight + 1)>>1)<<1;
		stride_check_width = mDecodedWidth;
	}
	else
	{
		SrcWidth = pMParm->mTarget_CropInfo.srcW;
		SrcHeight = pMParm->mTarget_CropInfo.srcH;
		stride_check_width = SrcWidth;
	}

	if(pMParm->mNeed_rotate)
	{
		if(!pMParm->mCropChanged)
		{
		#if 0
			pMParm->mTarget_CropInfo.dstW = pMParm->mOrigin_DispInfo.height;
			pMParm->mTarget_CropInfo.dstH = pMParm->mOrigin_DispInfo.width;
		#else
			pMParm->mTarget_CropInfo.dstW = pMParm->mScreenMode_DispInfo.height;
			pMParm->mTarget_CropInfo.dstH = pMParm->mScreenMode_DispInfo.width;
		#endif
		}

		pMParm->mDispWidth		= pMParm->mTarget_CropInfo.dstH;
		pMParm->mDispHeight 	= pMParm->mTarget_CropInfo.dstW;

		//Scaler limitation!!
		if(pMParm->mTarget_CropInfo.srcW < pMParm->mDispWidth && pMParm->mTarget_CropInfo.srcH < pMParm->mDispHeight)
		{
			float freScale;

			fScaleWidth 	= (float)pMParm->mDispWidth / pMParm->mTarget_CropInfo.srcW;
			fScaleHeight	= (float)pMParm->mDispHeight / pMParm->mTarget_CropInfo.srcH;
			fScaleTotal 	= fScaleWidth * fScaleHeight;

#ifdef USE_LIMITATION_SCALER_RATIO
			if(fScaleWidth >= 3.8 || fScaleHeight >= 3.8)
			{
				fScale = 3.8;
				if(fScaleWidth > fScaleHeight)
				{
					pMParm->mDispWidth	= (uint32_t)(pMParm->mTarget_CropInfo.srcW * fScale);
					pMParm->mDispHeight = (uint32_t)(pMParm->mTarget_CropInfo.srcH * fScale*fScaleHeight/fScaleWidth);
				}
				else
				{
					pMParm->mDispWidth	= (uint32_t)(pMParm->mTarget_CropInfo.srcW * fScale*fScaleWidth/fScaleHeight);
					pMParm->mDispHeight = (uint32_t)(pMParm->mTarget_CropInfo.srcH * fScale);
				}
			}
#endif
		}

		pMParm->mDispWidth	= ((pMParm->mDispWidth+3)>>2)<<2;
		pMParm->mDispHeight = ((pMParm->mDispHeight+3)>>2)<<2;
		DBUG("Scaler Out [r]: %d x %d", pMParm->mDispHeight, pMParm->mDispWidth);

		pMParm->mFinal_DPWidth = (pMParm->mDispWidth>>2)<<2; // for 16x multiple width on LCDC..

		pMParm->mFinal_stride = 0;
		stride_width = pMParm->mTarget_CropInfo.srcW - stride_check_width;//SrcWidth;
		DBUG("##@@## s %d = %d - %d", stride_width, pMParm->mTarget_CropInfo.srcW, SrcWidth);
		if(stride_width)
		{
			pMParm->mFinal_stride = ((stride_width*pMParm->mDispHeight) / pMParm->mTarget_CropInfo.srcW);// + 1;

			DBUG("##@@## f %d = %d x %d / %d", pMParm->mFinal_stride, stride_width, pMParm->mDispHeight, pMParm->mTarget_CropInfo.srcW);
			pMParm->mFinal_stride = ((pMParm->mFinal_stride+1)>>1)<<1;
		}

		pMParm->mFinal_DPHeight = pMParm->mDispHeight - pMParm->mFinal_stride;
		pMParm->mRight_truncation = pMParm->mFinal_stride;
		DBUG("transform/Crop Out [r]: %d x %d, height_stride: %d", pMParm->mFinal_DPWidth, pMParm->mFinal_DPHeight, pMParm->mFinal_stride);
	}
	else
	{
		if(!pMParm->mCropChanged)
		{
		#if 0
			pMParm->mTarget_CropInfo.dstW = pMParm->mOrigin_DispInfo.width;
			pMParm->mTarget_CropInfo.dstH = pMParm->mOrigin_DispInfo.height;
		#else
			pMParm->mTarget_CropInfo.dstW = pMParm->mScreenMode_DispInfo.width;
			pMParm->mTarget_CropInfo.dstH = pMParm->mScreenMode_DispInfo.height;
		#endif
		}

		fScaleWidth 	= (float)pMParm->mTarget_CropInfo.dstW / pMParm->mTarget_CropInfo.srcW;
		fScaleHeight	= (float)pMParm->mTarget_CropInfo.dstH / pMParm->mTarget_CropInfo.srcH;
		fScaleTotal 	= fScaleWidth * fScaleHeight;

		//Scaler limitation!!
#ifdef USE_LIMITATION_SCALER_RATIO
	#if 0
		if(pMParm->mTarget_CropInfo.srcW < pMParm->mOrigin_DispInfo.width && pMParm->mTarget_CropInfo.srcH < pMParm->mOrigin_DispInfo.height
			&& (fScaleWidth >= 3.8 || fScaleHeight >= 3.8))
	#else
		if(pMParm->mTarget_CropInfo.srcW < pMParm->mScreenMode_DispInfo.width && pMParm->mTarget_CropInfo.srcH < pMParm->mScreenMode_DispInfo.height
			&& (fScaleWidth >= 3.8 || fScaleHeight >= 3.8))
	#endif
		{
			fScale		= 3.8;
			pMParm->mDispWidth	= (uint32_t)(pMParm->mTarget_CropInfo.srcW * fScale);
			pMParm->mDispHeight = (uint32_t)(pMParm->mTarget_CropInfo.srcH * fScale);
		}
		else
#endif
		{
			pMParm->mDispWidth		= pMParm->mTarget_CropInfo.dstW;
			pMParm->mDispHeight 	= pMParm->mTarget_CropInfo.dstH;
		}

		pMParm->mDispWidth	= ((pMParm->mDispWidth+3)>>2)<<2;
		pMParm->mDispHeight = pMParm->mFinal_DPHeight = ((pMParm->mDispHeight+3)>>2)<<2;
		DBUG("Scaler Out : %d x %d", pMParm->mDispWidth, pMParm->mDispHeight);

		pMParm->mFinal_stride = 0;
		stride_width = pMParm->mTarget_CropInfo.srcW- stride_check_width;//SrcWidth;
		if(stride_width)
		{
			pMParm->mFinal_stride = ((stride_width*pMParm->mDispWidth) / pMParm->mTarget_CropInfo.srcW);// + 1;
			pMParm->mFinal_stride = ((pMParm->mFinal_stride+1)>>1)<<1;
		}

		pMParm->mFinal_DPWidth = pMParm->mDispWidth - pMParm->mFinal_stride;
		pMParm->mFinal_DPWidth = (pMParm->mFinal_DPWidth>>2)<<2; // for 16x multiple width on LCDC..
		pMParm->mRight_truncation = pMParm->mFinal_stride;
		DBUG("transform/Crop Out : %d x %d, width_stride: %d", pMParm->mFinal_DPWidth, pMParm->mFinal_DPHeight, pMParm->mFinal_stride);
	}

	if(pMParm->mFinal_DPWidth > pMParm->mLcd_width)
		pMParm->mFinal_DPWidth = pMParm->mLcd_width;

	if(pMParm->mFinal_DPHeight > pMParm->mLcd_height)
		pMParm->mFinal_DPHeight = pMParm->mLcd_height;

	if(pMParm->mFinal_DPWidth != pMParm->mDispWidth || pMParm->mFinal_DPHeight != pMParm->mDispHeight)
		pMParm->mNeed_crop = true;
	else
		pMParm->mNeed_crop = false;

	overlayInit(&pMParm->mScaler_arg, &pMParm->mGrp_arg, false);

	LOGI("Overlay :: isParallel(%d, %d/%d), src([o]%d x %d, [t]%d x %d) => S_Out(%d x %d) => G_Out(%d x %d) !!",
							pMParm->mParallel, pMParm->mNeed_scaler, (pMParm->mNeed_transform || pMParm->mNeed_crop), SrcWidth, SrcHeight, pMParm->mTarget_CropInfo.srcW, pMParm->mTarget_CropInfo.srcH, pMParm->mDispWidth, pMParm->mDispHeight, pMParm->mFinal_DPWidth, pMParm->mFinal_DPHeight);

	pMParm->mTarget_DispInfo.width = pMParm->mFinal_DPWidth;
	pMParm->mTarget_DispInfo.height = pMParm->mFinal_DPHeight;
#if 0
	if(pMParm->mOrigin_DispInfo.width != pMParm->mTarget_DispInfo.width){
		if(pMParm->mOrigin_DispInfo.width > pMParm->mTarget_DispInfo.width){
			gap_width = pMParm->mOrigin_DispInfo.width - pMParm->mTarget_DispInfo.width;
			pMParm->mTarget_DispInfo.sx = pMParm->mOrigin_DispInfo.sx + (gap_width/2);
		}
		else{
			gap_width = pMParm->mTarget_DispInfo.width - pMParm->mOrigin_DispInfo.width;
			if(pMParm->mOrigin_DispInfo.sx <= (gap_width/2))
				pMParm->mTarget_DispInfo.sx = 0;
			else
				pMParm->mTarget_DispInfo.sx = pMParm->mOrigin_DispInfo.sx - (gap_width/2);
		}
	}

	if(pMParm->mOrigin_DispInfo.height != pMParm->mTarget_DispInfo.height){
		if(pMParm->mOrigin_DispInfo.height > pMParm->mTarget_DispInfo.height){
			gap_height = pMParm->mOrigin_DispInfo.height - pMParm->mTarget_DispInfo.height;
			pMParm->mTarget_DispInfo.sy = pMParm->mOrigin_DispInfo.sy + (gap_height/2);
		}
		else{
			gap_height = pMParm->mTarget_DispInfo.height - pMParm->mOrigin_DispInfo.height;
			if(pMParm->mOrigin_DispInfo.sy <= (gap_height/2))
				pMParm->mTarget_DispInfo.sy = 0;
			else
				pMParm->mTarget_DispInfo.sy = pMParm->mOrigin_DispInfo.sy - (gap_height/2);
		}
	}
#else
	if(pMParm->mScreenMode_DispInfo.width != pMParm->mTarget_DispInfo.width){
		if(pMParm->mScreenMode_DispInfo.width > pMParm->mTarget_DispInfo.width){
			gap_width = pMParm->mScreenMode_DispInfo.width - pMParm->mTarget_DispInfo.width;
			pMParm->mTarget_DispInfo.sx = pMParm->mScreenMode_DispInfo.sx + (gap_width/2);
		}
		else{
			gap_width = pMParm->mTarget_DispInfo.width - pMParm->mScreenMode_DispInfo.width;
			if(pMParm->mScreenMode_DispInfo.sx <= (gap_width/2))
				pMParm->mTarget_DispInfo.sx = 0;
			else
				pMParm->mTarget_DispInfo.sx = pMParm->mScreenMode_DispInfo.sx - (gap_width/2);
		}
	}

	if(pMParm->mScreenMode_DispInfo.height != pMParm->mTarget_DispInfo.height){
		if(pMParm->mScreenMode_DispInfo.height > pMParm->mTarget_DispInfo.height){
			gap_height = pMParm->mScreenMode_DispInfo.height - pMParm->mTarget_DispInfo.height;
			pMParm->mTarget_DispInfo.sy = pMParm->mScreenMode_DispInfo.sy + (gap_height/2);
		}
		else{
			gap_height = pMParm->mTarget_DispInfo.height - pMParm->mScreenMode_DispInfo.height;
			if(pMParm->mScreenMode_DispInfo.sy <= (gap_height/2))
				pMParm->mTarget_DispInfo.sy = 0;
			else
				pMParm->mTarget_DispInfo.sy = pMParm->mScreenMode_DispInfo.sy - (gap_height/2);
		}
	}
#endif
	LOGI("LCD Pos :: %d,%d ~ %d,%d,%d", pMParm->mTarget_DispInfo.sx, pMParm->mTarget_DispInfo.sy, pMParm->mTarget_DispInfo.width, pMParm->mTarget_DispInfo.height, pMParm->mTarget_DispInfo.transform);

	if(!reCheck){
		pMParm->mFirst_Frame = pMParm->mOrder_Parallel = true;
	}
	else
	{
		pMParm->mFirst_Frame = true;
	}

	if(pMParm->mCropRgn_changed)
		pMParm->mCropRgn_changed = false;

	return true;
}

bool HwRenderer::overlayCheckExternalDisplayScreenMode(bool reCheck, int outSx, int outSy, int outWidth, int outHeight, unsigned int rotation)
{
	char value[PROPERTY_VALUE_MAX];
	float fScaleWidth, fScaleHeight, fScale;
	unsigned int SrcWidth, SrcHeight; // source buffer size
	unsigned int stride_width, final_stride;
	// Real_target to display. set real position in lcd!!
	unsigned int target_sx, target_sy, target_width, target_height, transform;
	int gap_width, gap_height;
	int CommitResult = 1;
	unsigned int out_width, out_height;
	int ChangedPos = 0;
	tcc_display_size DisplaySize;
	OUTPUT_SELECT output_type;
	unsigned int mscreen;

	property_get("tcc.dxb.fullscreen",value,"");
	mscreen = atoi(value);

	if(outSx < 0){
		outWidth += outSx;
		outSx = 0;
	}
	if(outSy < 0){
		outHeight += outSy;
		outSy = 0;
	}

	if(pMParm->mHDMIOutput)
		output_type = OUTPUT_HDMI;
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE
	else if(pMParm->mCompositeOutput)
		output_type = OUTPUT_COMPOSITE;
	else if(pMParm->mComponentOutput)
		output_type = OUTPUT_COMPONENT;
#endif
	else
		output_type = OUTPUT_NONE;

	if(reCheck)
	{
		target_sx       = outSx;
		target_sy       = outSy;
		target_width    = outWidth;
		target_height   = outHeight;

		transform       = rotation;

		if(target_width > (pMParm->mLcd_width - target_sx))
			target_width = (pMParm->mLcd_width - target_sx);

		if(target_height > (pMParm->mLcd_height - target_sy))
			target_height = (pMParm->mLcd_height - target_sy);

		if(CommitResult)
		{
			if((pMParm->mOrigin_DispInfo.sx != target_sx) || (pMParm->mOrigin_DispInfo.sy != target_sy) ||
				(pMParm->mOrigin_DispInfo.width != target_width) || (pMParm->mOrigin_DispInfo.height != target_height) || (pMParm->mOrigin_DispInfo.transform != transform)
			)
			{
				if((target_width == 0) || (target_height == 0))
				{
					LOGE("ESM :: Operation size is abnormal. (%d x %d)", target_width, target_height);
					return false;
				}

				pMParm->mOrigin_DispInfo.sx 		= target_sx;
				pMParm->mOrigin_DispInfo.sy 		= target_sy;
				pMParm->mOrigin_DispInfo.width 		= target_width;
				pMParm->mOrigin_DispInfo.height 	= target_height;
				pMParm->mOrigin_DispInfo.transform 	= transform;

				ChangedPos = 1;
				LOGI("ESM :: Overlay reSetting In :: %d,%d ~ %d,%d, %d", pMParm->mOrigin_DispInfo.sx, pMParm->mOrigin_DispInfo.sy, pMParm->mOrigin_DispInfo.width, pMParm->mOrigin_DispInfo.height, pMParm->mOrigin_DispInfo.transform);
			}
		}

		if(pMParm->mCropRgn_changed)
			ChangedPos = 1;
	}
	else
	{
		pMParm->mOrigin_DispInfo.sx       = outSx;
		pMParm->mOrigin_DispInfo.sy       = outSy;
		pMParm->mOrigin_DispInfo.width    = outWidth;
		pMParm->mOrigin_DispInfo.height   = outHeight;

		pMParm->mOrigin_DispInfo.transform  = rotation;

		if(pMParm->mOrigin_DispInfo.width > (pMParm->mLcd_width - pMParm->mOrigin_DispInfo.sx))
			pMParm->mOrigin_DispInfo.width = (pMParm->mLcd_width - pMParm->mOrigin_DispInfo.sx);

		if(pMParm->mOrigin_DispInfo.height > (pMParm->mLcd_height - pMParm->mOrigin_DispInfo.sy))
			pMParm->mOrigin_DispInfo.height = (pMParm->mLcd_height - pMParm->mOrigin_DispInfo.sy);

		if(CommitResult)
		{
			if((pMParm->mOrigin_DispInfo.width == 0) || (pMParm->mOrigin_DispInfo.height == 0))
			{
				LOGE("ESM :: Operation size is abnormal. (%d x %d)", pMParm->mOrigin_DispInfo.width, pMParm->mOrigin_DispInfo.height);
				return false;
			}

			pMParm->mOverlay_recheck = true;
			LOGI("ESM :: Overlay first Setting In :: pMParm->mOrigin_DispInfo %d,%d ~ %d,%d, %d", pMParm->mOrigin_DispInfo.sx, pMParm->mOrigin_DispInfo.sy, pMParm->mOrigin_DispInfo.width, pMParm->mOrigin_DispInfo.height, pMParm->mOrigin_DispInfo.transform);
		}
		else{
			return false;
		}

		pMParm->mOverlay_recheck = true;
	}

	// check external HDMI size
	if(pMParm->mHDMIOutput)
	{
		getDisplaySize(OUTPUT_HDMI, &DisplaySize);
		out_width = DisplaySize.width;
		out_height = DisplaySize.height;
	}
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE
	else if(pMParm->mCompositeOutput)
	{
		getDisplaySize(OUTPUT_COMPOSITE, &DisplaySize);
		out_width = DisplaySize.width;
		out_height = DisplaySize.height;
	}
	else if(pMParm->mComponentOutput)
	{
		getDisplaySize(OUTPUT_COMPONENT, &DisplaySize);
		out_width = DisplaySize.width;
		out_height = DisplaySize.height;
	}
#endif  
	else
	{
		out_width	= pMParm->mLcd_width;
		out_height	= pMParm->mLcd_height;
	}

	if(reCheck)
	{
		if(!ChangedPos && (pMParm->mOutput_width == out_width) && (pMParm->mOutput_height == out_height))
			return false;
	}

	pMParm->mOutput_width = out_width;
	pMParm->mOutput_height = out_height;
	SrcWidth = ((mDecodedWidth + 15)>>4)<<4;
	SrcHeight = ((mDecodedHeight + 1)>>1)<<1;

	if(mscreen == 1)
		pMParm->mSkipCropChanged = 1;

	LOGI("ESM :: lcd_info : %d,%d -> %d,%d", pMParm->mLcd_width, pMParm->mLcd_height, pMParm->mOutput_width, pMParm->mOutput_height);

	//LOGI("check get pMParm->mHDMIUiSize : %s pMParm->mDisplayOutputSTB : %d pMParm->mComponentChip %s", pMParm->mHDMIUiSize,pMParm->mDisplayOutputSTB,pMParm->mComponentChip);
	if( (strcasecmp(pMParm->mHDMIUiSize, "1280x720")!=0) && (pMParm->mOrigin_DispInfo.width == pMParm->mLcd_width || pMParm->mOrigin_DispInfo.height == pMParm->mLcd_height)
		&& (!pMParm->mSkipCropChanged))
	{
		fScaleWidth = (float)pMParm->mOutput_width / SrcWidth;
		fScaleHeight = (float)pMParm->mOutput_height/ SrcHeight;
		if(fScaleWidth > fScaleHeight)
			fScale = fScaleHeight;
		else
			fScale = fScaleWidth;
	#if 0
		pMParm->mTarget_CropInfo.sx = (SrcWidth - mDecodedWidth)/2;
		pMParm->mTarget_CropInfo.sy = (SrcHeight - mDecodedHeight)/2;
		pMParm->mTarget_CropInfo.srcW = SrcWidth;
		pMParm->mTarget_CropInfo.srcH = SrcHeight;
		pMParm->mTarget_CropInfo.dstW = mDecodedWidth * fScale;
		pMParm->mTarget_CropInfo.dstH = mDecodedHeight * fScale;
	#endif

		pMParm->mTarget_DispInfo.sx = pMParm->mTarget_DispInfo.sy = 0;
		if(pMParm->mOutput_width > pMParm->mTarget_CropInfo.dstW)
			pMParm->mTarget_DispInfo.sx = (pMParm->mOutput_width - pMParm->mTarget_CropInfo.dstW)/2;
		if(pMParm->mOutput_height > pMParm->mTarget_CropInfo.dstH)
			pMParm->mTarget_DispInfo.sy = (pMParm->mOutput_height- pMParm->mTarget_CropInfo.dstH)/2;
		pMParm->mTarget_DispInfo.width	= pMParm->mTarget_CropInfo.dstW;
		pMParm->mTarget_DispInfo.height = pMParm->mTarget_CropInfo.dstH;

		LOGI("ESM :: reSized_info :1: %d,%d ~ %d,%d", pMParm->mTarget_DispInfo.sx, pMParm->mTarget_DispInfo.sy, pMParm->mTarget_DispInfo.width, pMParm->mTarget_DispInfo.height);
	}else if(pMParm->mSkipCropChanged){
		pMParm->mTarget_DispInfo.sx = pMParm->mTarget_DispInfo.sy = 0;
		pMParm->mTarget_CropInfo.dstW = pMParm->mOutput_width;
		pMParm->mTarget_CropInfo.dstH = pMParm->mOutput_height;
		pMParm->mTarget_DispInfo.width	= pMParm->mTarget_CropInfo.dstW;
		pMParm->mTarget_DispInfo.height = pMParm->mTarget_CropInfo.dstH;
		LOGI("ESM :: reSized_info :only full output screen");
	}
	else
	{
		pMParm->mTarget_DispInfo.sx = (pMParm->mOutput_width * pMParm->mOrigin_DispInfo.sx)/pMParm->mLcd_width;
		pMParm->mTarget_DispInfo.sy = (pMParm->mOutput_height* pMParm->mOrigin_DispInfo.sy)/pMParm->mLcd_height;
		pMParm->mTarget_DispInfo.width	= (pMParm->mOutput_width * pMParm->mOrigin_DispInfo.width)/pMParm->mLcd_width;
		pMParm->mTarget_DispInfo.height = (pMParm->mOutput_height* pMParm->mOrigin_DispInfo.height)/pMParm->mLcd_height;
		pMParm->mTarget_DispInfo.transform	= pMParm->mOrigin_DispInfo.transform;
		LOGI("ESM :: reSized_info :2: %d,%d ~ %d,%d", pMParm->mTarget_DispInfo.sx, pMParm->mTarget_DispInfo.sy, pMParm->mTarget_DispInfo.width, pMParm->mTarget_DispInfo.height);

		fScaleWidth = (float)pMParm->mTarget_DispInfo.width / SrcWidth;
		fScaleHeight = (float)pMParm->mTarget_DispInfo.height / SrcHeight;
		if(fScaleWidth > fScaleHeight)
			fScale = fScaleHeight;
		else
			fScale = fScaleWidth;

	#if 0
		pMParm->mTarget_CropInfo.sx = pMParm->mTarget_CropInfo.sy = 0;
		if(SrcWidth > mDecodedWidth)
			pMParm->mTarget_CropInfo.sx = (SrcWidth - mDecodedWidth)/2;
		if(SrcHeight > mDecodedHeight)
			pMParm->mTarget_CropInfo.sy = (SrcHeight - mDecodedHeight)/2;
		pMParm->mTarget_CropInfo.srcW = SrcWidth;
		pMParm->mTarget_CropInfo.srcH = SrcHeight;
	#endif

		if(strcasecmp(pMParm->mHDMIUiSize, "1280x720")!=0){
			pMParm->mTarget_CropInfo.dstW = pMParm->mTarget_DispInfo.width;
			pMParm->mTarget_CropInfo.dstH = pMParm->mTarget_DispInfo.height;
		}
		else{
			if(pMParm->mIgnore_ratio){
				pMParm->mTarget_CropInfo.dstW = pMParm->mTarget_DispInfo.width;
				pMParm->mTarget_CropInfo.dstH = pMParm->mTarget_DispInfo.height;
			}
			else{
				pMParm->mTarget_CropInfo.dstW = mDecodedWidth * fScale;
				pMParm->mTarget_CropInfo.dstH = mDecodedHeight * fScale;
			}
		}
	}

	pMParm->mNeed_scaler = true;
	pMParm->mNeed_transform  = pMParm->mNeed_rotate = false;
	pMParm->mNeed_crop = false;

	DBUG("ESM :: Overlay Check In!!, SRC::%d x %d	 TAR:: %d x %d", mDecodedWidth, mDecodedHeight, pMParm->mTarget_CropInfo.dstW, pMParm->mTarget_CropInfo.dstH);

	// TCC92xx block limitation check	>>
	SrcWidth = pMParm->mTarget_CropInfo.srcW;
	SrcHeight = pMParm->mTarget_CropInfo.srcH;

	fScaleWidth 	= (float)pMParm->mTarget_CropInfo.dstW / pMParm->mTarget_CropInfo.srcW;
	fScaleHeight	= (float)pMParm->mTarget_CropInfo.dstH / pMParm->mTarget_CropInfo.srcH;

	//Scaler  limitation!!
#ifdef USE_LIMITATION_SCALER_RATIO
	if(fScaleWidth >= 3.8 || fScaleHeight >= 3.8)
	{
		if(fScaleWidth > fScaleHeight)
			fScale = fScaleHeight;
		else
			fScale = fScaleWidth;

		if(fScale > 3.8)
			fScale = 3.8;

		if(pMParm->mIgnore_ratio)
		{
			if(fScaleWidth > fScaleHeight)
			{
				pMParm->mDispWidth	= (uint32_t)(pMParm->mTarget_CropInfo.srcW * 3.8);
				pMParm->mDispHeight = (uint32_t)(pMParm->mTarget_CropInfo.srcH * fScale);
			}
			else
			{
				pMParm->mDispWidth	= (uint32_t)(pMParm->mTarget_CropInfo.srcW * fScale);
				pMParm->mDispHeight = (uint32_t)(pMParm->mTarget_CropInfo.srcH * 3.8);
			}
		}
		else
		{
			pMParm->mDispWidth	= (uint32_t)(pMParm->mTarget_CropInfo.srcW * fScale);
			pMParm->mDispHeight = (uint32_t)(pMParm->mTarget_CropInfo.srcH * fScale);
		}
	}
	else
#endif
	{
		pMParm->mDispWidth		= pMParm->mTarget_CropInfo.dstW;
		pMParm->mDispHeight 	= pMParm->mTarget_CropInfo.dstH;
	}

	pMParm->mDispWidth	= ((pMParm->mDispWidth+3)>>2)<<2;
	pMParm->mDispHeight = pMParm->mFinal_DPHeight = ((pMParm->mDispHeight+3)>>2)<<2;

	/* Display Position setting!!*/
	#if 1 //DDR3 scaler limitation
	if((pMParm->mTarget_CropInfo.dstW == 1920) && (pMParm->mTarget_CropInfo.dstH == 1080) && (SrcWidth == 960) && (SrcHeight == 540))
	{
		pMParm->mDispWidth	= 1912;
	}
	#endif//

	LOGI("ESM :: Display Position setting:pMParm->mTarget_DispInfo %d,%d ~ %d,%d DispWidth %d Height %d ", pMParm->mTarget_DispInfo.sx, pMParm->mTarget_DispInfo.sy, pMParm->mTarget_DispInfo.width, pMParm->mTarget_DispInfo.height,pMParm->mDispWidth,pMParm->mDispHeight);
	if(pMParm->mTarget_DispInfo.width >= pMParm->mDispWidth)
		pMParm->mTarget_DispInfo.sx += (pMParm->mTarget_DispInfo.width - pMParm->mDispWidth)/2;
	else
	{
		if(pMParm->mTarget_DispInfo.sx >= ((pMParm->mDispWidth- pMParm->mTarget_DispInfo.width)/2))
			pMParm->mTarget_DispInfo.sx -= (pMParm->mDispWidth- pMParm->mTarget_DispInfo.width)/2;
		else
			pMParm->mTarget_DispInfo.sx = 0;
	}

	if(pMParm->mTarget_DispInfo.height >= pMParm->mDispHeight)
		pMParm->mTarget_DispInfo.sy += (pMParm->mTarget_DispInfo.height- pMParm->mDispHeight)/2;
	else
	{
		if(pMParm->mTarget_DispInfo.sy >= ((pMParm->mDispHeight - pMParm->mTarget_DispInfo.height)/2))
			pMParm->mTarget_DispInfo.sy -= (pMParm->mDispHeight - pMParm->mTarget_DispInfo.height)/2;
		else
			pMParm->mTarget_DispInfo.sy = 0;
	}

	pMParm->mTarget_DispInfo.width	= pMParm->mDispWidth;
	pMParm->mTarget_DispInfo.height = pMParm->mDispHeight;

	//if(pMParm->mDisplayOutputSTB)
	{
		if(out_width > 0 && out_height > 0){
			pMParm->mDispWidth	= pMParm->mDispWidth * (out_width - overlayResizeExternalDisplay(output_type, 0)) / out_width ;
			pMParm->mDispHeight	= pMParm->mDispHeight * (out_height - overlayResizeExternalDisplay(output_type, 1)) / out_height;

			pMParm->mTarget_DispInfo.sx		= (out_width - overlayResizeExternalDisplay(output_type, 0)) * pMParm->mTarget_DispInfo.sx / out_width + overlayResizeGetPosition(output_type, 0);
			pMParm->mTarget_DispInfo.sy		=  (out_height - overlayResizeExternalDisplay(output_type, 1)) * pMParm->mTarget_DispInfo.sy / out_height + overlayResizeGetPosition(output_type, 1);
			pMParm->mTarget_DispInfo.width	= pMParm->mTarget_DispInfo.width  * (out_width - overlayResizeExternalDisplay(output_type, 0)) / out_width;
			pMParm->mTarget_DispInfo.height	= pMParm->mTarget_DispInfo.height * (out_height - overlayResizeExternalDisplay(output_type, 1)) / out_height;
		}
	}

	LOGI("ESM :: Display Position setting:pMParm->mTarget_DispInfo %d,%d ~ %d,%d", pMParm->mTarget_DispInfo.sx, pMParm->mTarget_DispInfo.sy, pMParm->mTarget_DispInfo.width, pMParm->mTarget_DispInfo.height);

	overlayInit(&pMParm->mScaler_arg, &pMParm->mGrp_arg, false);

	LOGE("ESM :: Overlay :: isParallel(%d, %d/%d), src([o]%d x %d, [t]%d x %d) => S_Out(%d x %d) !!",  pMParm->mParallel, pMParm->mNeed_scaler, (pMParm->mNeed_transform || pMParm->mNeed_crop), SrcWidth, SrcHeight, pMParm->mTarget_CropInfo.srcW, pMParm->mTarget_CropInfo.srcH, pMParm->mDispWidth, pMParm->mDispHeight);

	if(!reCheck){
		pMParm->mFirst_Frame = pMParm->mOrder_Parallel = true;
	}
	else{
		pMParm->mFirst_Frame = true;
	}

	if(pMParm->mCropRgn_changed)
		pMParm->mCropRgn_changed = false;

	return true;
}

int HwRenderer::overlayProcess(SCALER_TYPE *scaler_arg, G2D_COMMON_TYPE *grp_arg, unsigned int srcY, unsigned int srcU, unsigned int srcV, unsigned int dest)
{
	char value[PROPERTY_VALUE_MAX];

	unsigned int src_addrY, src_addrU, src_addrV;
	struct pollfd poll_event[1];

	src_addrY = src_addrU = src_addrV = 0;
	if(pMParm->mNeed_scaler)
	{
		///////////////////////////added by ronald.lee
			// default scaler : memory  to memroy interrupt type
		scaler_arg->viqe_onthefly = 0x0;
		scaler_arg->responsetype = SCALER_INTERRUPT;

		if(mColorFormat == OMX_COLOR_FormatYUV420SemiPlanar)
			scaler_arg->src_fmt 		= SCALER_YUV420_inter;
		else if(mColorFormat == OMX_COLOR_FormatYUV422Planar)
			scaler_arg->src_fmt 		= SCALER_YUV422_sp;
		else
			scaler_arg->src_fmt 		= SCALER_YUV420_sp;

		///////////////////////////

		scaler_arg->src_Yaddr 		= srcY;
		scaler_arg->src_Uaddr		= srcU;
		scaler_arg->src_Vaddr		= srcV;

		scaler_arg->dest_Yaddr	= dest;
		scaler_arg->dest_Uaddr  	= GET_ADDR_YUV42X_spU(scaler_arg->dest_Yaddr, scaler_arg->dest_ImgWidth, scaler_arg->dest_ImgHeight);//scaler_arg->dest_Yaddr + (scaler_arg->dest_ImgWidth * scaler_arg->dest_ImgHeight);
		scaler_arg->dest_Vaddr 	= GET_ADDR_YUV420_spV(scaler_arg->dest_Uaddr, scaler_arg->dest_ImgWidth, scaler_arg->dest_ImgHeight);

		scaler_arg->interlaced = pMParm->mVideoDeinterlaceScaleUpOneFieldFlag ? 1 : 0;

		if(pMParm->mM2m_fd < 0){
			pMParm->mM2m_fd = open(VIDEO_PLAY_SCALER, O_RDWR);
			if (pMParm->mM2m_fd < 0) {
				LOGE("can't open'%s'", VIDEO_PLAY_SCALER);
			}
			else {
				LOGI("%d : %s is opened.", __LINE__, VIDEO_PLAY_SCALER);
			}
		}

		if (ioctl(pMParm->mM2m_fd, TCC_SCALER_IOCTRL, scaler_arg) < 0)
		{
			LOGE("Scaler Out Error!");
			goto SCALER_ERROR;
		}

		DBUG_FRM("@@@ Scaler-SRC(fmt:%d):: %d x %d, %d,%d ~ %d x %d", scaler_arg->src_fmt, scaler_arg->src_ImgWidth, scaler_arg->src_ImgHeight,
				scaler_arg->src_winLeft, scaler_arg->src_winTop, scaler_arg->src_winRight-scaler_arg->src_winLeft, scaler_arg->src_winBottom-scaler_arg->src_winTop);
		DBUG_FRM("@@@ Scaler-Dest(rmt:%d):: %d x %d", scaler_arg->dest_fmt, scaler_arg->dest_ImgWidth, scaler_arg->dest_ImgHeight);
		DBUG_FRM("@@@ %p-%p-%p => %p-%p-%p", (void*)srcY, (void*)srcU, (void*)srcV, (void*)scaler_arg->dest_Yaddr, (void*)scaler_arg->dest_Uaddr, (void*)scaler_arg->dest_Vaddr);

		if(scaler_arg->responsetype == SCALER_INTERRUPT)
		{
			int ret;
			memset(poll_event, 0, sizeof(poll_event));
			poll_event[0].fd = pMParm->mM2m_fd;
			poll_event[0].events = POLLIN;
			ret = poll((struct pollfd*)poll_event, 1, 400);

			if (ret < 0) {
				LOGE("m2m poll error\n");
				return -1;
			}else if (ret == 0) {
				LOGE("m2m poll timeout\n");
				goto SCALER_ERROR;
			}else if (ret > 0) {
				if (poll_event[0].revents & POLLERR) {
					LOGE("m2m poll POLLERR\n");
					return -1;
				}
			}
		}

		src_addrY = (unsigned int)scaler_arg->dest_Yaddr;
		src_addrU = (unsigned int)scaler_arg->dest_Uaddr;
		src_addrV = (unsigned int)scaler_arg->dest_Vaddr;
	}
	else
	{
#if !defined(_TCC8900_) && !defined(_TCC9300_)
		src_addrY 	= (unsigned int)srcY;
		src_addrU	= (unsigned int)srcU;
		src_addrV 	= (unsigned int)srcV;
#else
		scaler_arg->dest_Yaddr 		= (char*)srcY;
		scaler_arg->dest_Uaddr		= (char*)srcU;
		scaler_arg->dest_Vaddr		= (char*)srcV;
		scaler_arg->dest_ImgWidth	= pMParm->mDispWidth;
		scaler_arg->dest_ImgHeight	= pMParm->mDispHeight;
#endif
	}

	if(pMParm->mNeed_transform || pMParm->mNeed_crop)
	{
		grp_arg->src0		= (unsigned int)src_addrY;
		grp_arg->src1		= (unsigned int)src_addrU;
		grp_arg->src2		= (unsigned int)src_addrV;

		grp_arg->tgt0		= (unsigned int)dest;
		grp_arg->tgt1		= GET_ADDR_YUV42X_spU(grp_arg->tgt0, grp_arg->dst_imgx, grp_arg->dst_imgy);
		grp_arg->tgt2		= GET_ADDR_YUV420_spV(grp_arg->tgt1, grp_arg->dst_imgx, grp_arg->dst_imgy);

		if( pMParm->mRt_fd < 0 ) {
			pMParm->mRt_fd = open(GRAPHIC_DEVICE, O_RDWR);
			if (pMParm->mRt_fd < 0){
				LOGE("can't open'%s'", GRAPHIC_DEVICE);
			}
		}

		if(ioctl(pMParm->mRt_fd, TCC_GRP_COMMON_IOCTRL, grp_arg) < 0)
		{
			LOGE("G2D Out Error!");
			goto G2D_ERROR;
		}

		DBUG_FRM("@@@ G2D-SRC:: %d x %d, %d,%d ~ %d x %d (rot: %d)", grp_arg->src_imgx, grp_arg->src_imgy, grp_arg->crop_offx, grp_arg->crop_offy, grp_arg->crop_imgx, grp_arg->crop_imgy, grp_arg->ch_mode);
		DBUG_FRM("@@@ G2D-Dest:: %d x %d", grp_arg->dst_imgx, grp_arg->dst_imgy);
		DBUG_FRM("@@@ %p-%p-%p => %p-%p-%p", (void*)grp_arg->src0, (void*)grp_arg->src1, (void*)grp_arg->src2, (void*)grp_arg->tgt0, (void*)grp_arg->tgt1, (void*)grp_arg->tgt2);

		if(grp_arg->responsetype == G2D_INTERRUPT)
		{
			int ret;
			memset(poll_event, 0, sizeof(poll_event));
			poll_event[0].fd = pMParm->mRt_fd;
			poll_event[0].events = POLLIN;
			ret = poll((struct pollfd*)poll_event, 1, 400);

			if (ret < 0) {
				LOGE("g2d poll error\n");
				return -1;
			}else if (ret == 0) {
				LOGE("g2d poll timeout\n");
				goto G2D_ERROR;
			}else if (ret > 0) {
				if (poll_event[0].revents & POLLERR) {
					LOGE("g2d poll POLLERR\n");
					return -1;
				}
			}
		}
	}

	return 0;

SCALER_ERROR:
	close(pMParm->mM2m_fd); pMParm->mM2m_fd = -1;

	return -1;

G2D_ERROR:
	close(pMParm->mRt_fd); pMParm->mRt_fd = -1;

	return -1;

}

int HwRenderer::overlayProcess_Parallel(SCALER_TYPE *scaler_arg, G2D_COMMON_TYPE *grp_arg,
											unsigned int srcY, unsigned int srcU, unsigned int srcV, uint8_t *dest)
{
	unsigned int src_addrY, src_addrU, src_addrV;
	struct pollfd poll_event[1];

	if(pMParm->mFirst_Frame)
	{
		//Scaler!!
		scaler_arg->src_Yaddr 		= srcY;
		scaler_arg->src_Uaddr		= srcU;
		scaler_arg->src_Vaddr		= srcV;

		if(pMParm->mOrder_Parallel)
			scaler_arg->dest_Yaddr = pMParm->mOverlayRotation_1st.base;
		else
			scaler_arg->dest_Yaddr = pMParm->mOverlayRotation_2nd.base;

		scaler_arg->dest_Uaddr = GET_ADDR_YUV42X_spU(scaler_arg->dest_Yaddr, scaler_arg->dest_ImgWidth, scaler_arg->dest_ImgHeight);
		scaler_arg->dest_Vaddr = GET_ADDR_YUV420_spV(scaler_arg->dest_Uaddr, scaler_arg->dest_ImgWidth, scaler_arg->dest_ImgHeight);

		scaler_arg->interlaced = pMParm->mVideoDeinterlaceScaleUpOneFieldFlag ? 1 : 0;

		if(pMParm->mM2m_fd < 0){
			pMParm->mM2m_fd = open(VIDEO_PLAY_SCALER, O_RDWR);
			if (pMParm->mM2m_fd < 0) {
				LOGE("can't open '%s'", VIDEO_PLAY_SCALER);
			}
			else {
				LOGI("%d : %s is opened.", __LINE__, VIDEO_PLAY_SCALER);
			}
		}

		if (ioctl(pMParm->mM2m_fd, TCC_SCALER_IOCTRL, scaler_arg) < 0)
		{
			LOGE("Scaler Out Error!");
			goto SCALER_ERROR;
		}
		DBUG_FRM("@@@ Scaler-SRC rsp_type(0x%x), (fmt:%d):: %d x %d, %d,%d ~ %d x %d", scaler_arg->responsetype, scaler_arg->src_fmt, scaler_arg->src_ImgWidth, scaler_arg->src_ImgHeight,
				scaler_arg->src_winLeft, scaler_arg->src_winTop, scaler_arg->src_winRight-scaler_arg->src_winLeft, scaler_arg->src_winBottom-scaler_arg->src_winTop);
		DBUG_FRM("@@@ Scaler-Dest(rmt:%d):: %d x %d", scaler_arg->dest_fmt, scaler_arg->dest_ImgWidth, scaler_arg->dest_ImgHeight);
		DBUG_FRM("@@@ %p-%p-%p => %p-%p-%p", (void*)srcY, (void*)srcU, (void*)srcV, (void*)scaler_arg->dest_Yaddr, (void*)scaler_arg->dest_Uaddr, (void*)scaler_arg->dest_Vaddr);

		if(scaler_arg->responsetype == SCALER_INTERRUPT)
		{
			int ret;
			memset(poll_event, 0, sizeof(poll_event));
			poll_event[0].fd = pMParm->mM2m_fd;
			poll_event[0].events = POLLIN;
			ret = poll((struct pollfd*)poll_event, 1, 400);

			if (ret < 0) {
				LOGE("m2m poll error\n");
				return -1;
			}else if (ret == 0) {
				LOGE("m2m poll timeout\n");
				goto SCALER_ERROR;
			}else if (ret > 0) {
				if (poll_event[0].revents & POLLERR) {
					LOGE("m2m poll POLLERR\n");
					return -1;
				}
			}
		}

		//G2D Rotate!!
		grp_arg->src0		= (unsigned int)scaler_arg->dest_Yaddr;
		grp_arg->src1		= GET_ADDR_YUV42X_spU(grp_arg->src0, grp_arg->src_imgx, grp_arg->src_imgy);
		grp_arg->src2		= GET_ADDR_YUV420_spV(grp_arg->src1, grp_arg->src_imgx, grp_arg->src_imgy);

		grp_arg->tgt0		= (unsigned int)dest;
		grp_arg->tgt1		= GET_ADDR_YUV42X_spU(grp_arg->tgt0, grp_arg->dst_imgx, grp_arg->dst_imgy);
		grp_arg->tgt2		= GET_ADDR_YUV420_spV(grp_arg->tgt1, grp_arg->dst_imgx, grp_arg->dst_imgy);

		if( pMParm->mRt_fd < 0 ) {
			pMParm->mRt_fd = open(GRAPHIC_DEVICE, O_RDWR);
			if (pMParm->mRt_fd < 0){
				LOGE("can't open '%s'", GRAPHIC_DEVICE);
			}
		}

		if(ioctl(pMParm->mRt_fd, TCC_GRP_COMMON_IOCTRL, grp_arg) < 0)
		{
			LOGE("G2D Out Error!");
			goto G2D_ERROR;
		}
		DBUG_FRM("@@@ G2D-SRC:: %d x %d, %d,%d ~ %d x %d (rot: %d)", grp_arg->src_imgx, grp_arg->src_imgy, grp_arg->crop_offx, grp_arg->crop_offy, grp_arg->crop_imgx, grp_arg->crop_imgy, grp_arg->ch_mode);
		DBUG_FRM("@@@ G2D-Dest:: %d x %d", grp_arg->dst_imgx, grp_arg->dst_imgy);
		DBUG_FRM("@@@ %p-%p-%p => %p-%p-%p", (void*)grp_arg->src0, (void*)grp_arg->src1, (void*)grp_arg->src2, (void*)grp_arg->tgt0, (void*)grp_arg->tgt1, (void*)grp_arg->tgt2);

		if(grp_arg->responsetype == G2D_INTERRUPT)
		{
			int ret;
			memset(poll_event, 0, sizeof(poll_event));
			poll_event[0].fd = pMParm->mRt_fd;
			poll_event[0].events = POLLIN;
			ret = poll((struct pollfd*)poll_event, 1, 400);

			if (ret < 0) {
				LOGE("g2d poll error\n");
				return -1;
			}else if (ret == 0) {
				LOGE("g2d poll timeout\n");
				goto G2D_ERROR;
			}else if (ret > 0) {
				if (poll_event[0].revents & POLLERR) {
					LOGE("g2d poll POLLERR\n");
					return -1;
				}
			}
		}
	}
	else
	{
		//G2D Rotate!!
		if(pMParm->mOrder_Parallel)
			grp_arg->src0		= (unsigned int)pMParm->mOverlayRotation_2nd.base;
		else
			grp_arg->src0		= (unsigned int)pMParm->mOverlayRotation_1st.base;

		grp_arg->src1		= GET_ADDR_YUV42X_spU(grp_arg->src0, grp_arg->src_imgx, grp_arg->src_imgy);
		grp_arg->src2		= GET_ADDR_YUV420_spV(grp_arg->src1, grp_arg->src_imgx, grp_arg->src_imgy);

		grp_arg->tgt0		= (unsigned int)dest;
		grp_arg->tgt1		= GET_ADDR_YUV42X_spU(grp_arg->tgt0, grp_arg->dst_imgx, grp_arg->dst_imgy);
		grp_arg->tgt2		= GET_ADDR_YUV420_spV(grp_arg->tgt1, grp_arg->dst_imgx, grp_arg->dst_imgy);

		if( pMParm->mRt_fd < 0 ) {
			pMParm->mRt_fd = open(GRAPHIC_DEVICE, O_RDWR);
			if (pMParm->mRt_fd < 0){
				LOGE("can't open '%s'" GRAPHIC_DEVICE);
			}
		}

		if(ioctl(pMParm->mRt_fd, TCC_GRP_COMMON_IOCTRL, grp_arg) < 0)
		{
			LOGE("G2D Out Error!");
			goto G2D_ERROR;
		}
		DBUG_FRM("### G2D-SRC:: %d x %d, %d,%d ~ %d x %d (rot: %d)", grp_arg->src_imgx, grp_arg->src_imgy, grp_arg->crop_offx, grp_arg->crop_offy, grp_arg->crop_imgx, grp_arg->crop_imgy, grp_arg->ch_mode);
		DBUG_FRM("### G2D-Dest:: %d x %d", grp_arg->dst_imgx, grp_arg->dst_imgy);
		DBUG_FRM("### %p-%p-%p => %p-%p-%p", (void*)grp_arg->src0, (void*)grp_arg->src1, (void*)grp_arg->src2, (void*)grp_arg->tgt0, (void*)grp_arg->tgt1, (void*)grp_arg->tgt2);

		//Scaler!!
		scaler_arg->src_Yaddr 		= srcY;
		scaler_arg->src_Uaddr		= srcU;
		scaler_arg->src_Vaddr		= srcV;

		if(pMParm->mOrder_Parallel)
			scaler_arg->dest_Yaddr = pMParm->mOverlayRotation_1st.base;
		else
			scaler_arg->dest_Yaddr = pMParm->mOverlayRotation_2nd.base;

		scaler_arg->dest_Uaddr = GET_ADDR_YUV42X_spU(scaler_arg->dest_Yaddr, scaler_arg->dest_ImgWidth, scaler_arg->dest_ImgHeight);
		scaler_arg->dest_Vaddr = GET_ADDR_YUV420_spV(scaler_arg->dest_Uaddr, scaler_arg->dest_ImgWidth, scaler_arg->dest_ImgHeight);

		scaler_arg->interlaced = pMParm->mVideoDeinterlaceScaleUpOneFieldFlag ? 1 : 0;

		if(pMParm->mM2m_fd < 0){
			pMParm->mM2m_fd = open(VIDEO_PLAY_SCALER, O_RDWR);
			if (pMParm->mM2m_fd < 0) {
				LOGE("can't open '%s'", VIDEO_PLAY_SCALER);
			}
			else {
				LOGI("%d : %s is opened.", __LINE__, VIDEO_PLAY_SCALER);
			}
		}

		if (ioctl(pMParm->mM2m_fd, TCC_SCALER_IOCTRL, scaler_arg) < 0)
		{
			LOGE("Scaler Out Error!");
			return -1;
		}
		DBUG_FRM("### Scaler-SRC(fmt:%d):: %d x %d, %d,%d ~ %d x %d", scaler_arg->src_fmt, scaler_arg->src_ImgWidth, scaler_arg->src_ImgHeight,
				scaler_arg->src_winLeft, scaler_arg->src_winTop, scaler_arg->src_winRight-scaler_arg->src_winLeft, scaler_arg->src_winBottom-scaler_arg->src_winTop);
		DBUG_FRM("### Scaler-Dest(rmt:%d):: %d x %d", scaler_arg->dest_fmt, scaler_arg->dest_ImgWidth, scaler_arg->dest_ImgHeight);
		DBUG_FRM("### %p-%p-%p => %p-%p-%p", (void*)srcY, (void*)srcU, (void*)srcV, (void*)scaler_arg->dest_Yaddr, (void*)scaler_arg->dest_Uaddr, (void*)scaler_arg->dest_Vaddr);

		if(grp_arg->responsetype == G2D_INTERRUPT)
		{
			int ret;
			memset(poll_event, 0, sizeof(poll_event));
			poll_event[0].fd = pMParm->mRt_fd;
			poll_event[0].events = POLLIN;
			ret = poll((struct pollfd*)poll_event, 1, 400);

			if (ret < 0) {
				LOGE("g2d poll error\n");
				return -1;
			}else if (ret == 0) {
				LOGE("g2d poll timeout\n");
				goto G2D_ERROR;
			}else if (ret > 0) {
				if (poll_event[0].revents & POLLERR) {
					LOGE("g2d poll POLLERR\n");
					return -1;
				}
			}
		}

		if(scaler_arg->responsetype == SCALER_INTERRUPT)
		{
			int ret;
			memset(poll_event, 0, sizeof(poll_event));
			poll_event[0].fd = pMParm->mM2m_fd;
			poll_event[0].events = POLLIN;
			ret = poll((struct pollfd*)poll_event, 1, 400);

			if (ret < 0) {
				LOGE("m2m poll error\n");
				return -1;
			}else if (ret == 0) {
				LOGE("m2m poll timeout\n");
				goto SCALER_ERROR;
			}else if (ret > 0) {
				if (poll_event[0].revents & POLLERR) {
					LOGE("m2m poll POLLERR\n");
					return -1;
				}
			}
		}
	}

	if(pMParm->mOrder_Parallel)
		pMParm->mOrder_Parallel = false;
	else
		pMParm->mOrder_Parallel = true;

	return 0;

SCALER_ERROR:
	close(pMParm->mM2m_fd); pMParm->mM2m_fd = -1;

	return -1;

G2D_ERROR:
	close(pMParm->mRt_fd); pMParm->mRt_fd = -1;

	return -1;
}

bool HwRenderer::output_change_check(int outSx, int outSy, int outWidth, int outHeight, unsigned int rotation, bool *overlayCheckRet, bool interlaced)
{
	if(pMParm->mOutputMode == 1) //external display device
	{
		*overlayCheckRet = (this->*OverlayCheckExternalDisplayFunction)(pMParm->mOverlay_recheck, outSx, outSy, outWidth, outHeight, rotation);
	}
	else
	{
		pos_info_t pos_info;

		*overlayCheckRet = (this->*OverlayCheckDisplayFunction)(pMParm->mOverlay_recheck, outSx, outSy, outWidth, outHeight, rotation);
	}

	if(!(*overlayCheckRet))
	{
		if(!pMParm->mOverlay_recheck)
			return 1;
	}

	char value[PROPERTY_VALUE_MAX];
	unsigned int	uiOutputMode = 0;
	unsigned int	uiHdmiResolution = 0;
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE  	
	unsigned int	uiComponentMode = 0;
#endif	
	unsigned int	uiDetectedResolution = 0;
	bool able_to_otf_mode = false;
	unsigned int base_width, base_height, src_crop_width, src_crop_height, dest_crop_width, dest_crop_height;

	src_crop_width = pMParm->mScaler_arg.src_winRight - pMParm->mScaler_arg.src_winLeft;
	src_crop_height = pMParm->mScaler_arg.src_winBottom - pMParm->mScaler_arg.src_winTop;
	dest_crop_width = pMParm->mScaler_arg.dest_winRight - pMParm->mScaler_arg.dest_winLeft;
	dest_crop_height = pMParm->mScaler_arg.dest_winBottom - pMParm->mScaler_arg.dest_winTop;

	if( pMParm->mOutputMode == 1 )//external display mode
	{
#if 0 // Enable after testing real situation!! Actually we do not have any application that can change the region of surface.
		tcc_display_size DisplaySize;

		if(pMParm->mHDMIOutput)
		{
		getDisplaySize(OUTPUT_HDMI, &DisplaySize);
		base_width = DisplaySize.width;
		base_height = DisplaySize.height;
		}
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE        
		else if(pMParm->mCompositeOutput)
		{
		getDisplaySize(OUTPUT_COMPOSITE, &DisplaySize);
		base_width = DisplaySize.width;
		base_height = DisplaySize.height;
		}
		else if(pMParm->mComponentOutput)
		{
		getDisplaySize(OUTPUT_COMPONENT, &DisplaySize);
		base_width = DisplaySize.width;
		base_height = DisplaySize.height;
		}
#endif
#else
		base_width = src_crop_width;
		base_height = src_crop_height;
#endif

		if( (dest_crop_width * dest_crop_height) <= (((((base_width/2)+15)>>4)<<4) * (((base_height/2)+1)>>1)<<1))
			able_to_otf_mode = false;
		else
			able_to_otf_mode = true;
	}
#if defined(USE_LCD_OTF_SCALING) || defined(USE_LCD_VIDEO_VSYNC)
	else if( !pMParm->mTransform && !pMParm->mOutputMode )
	{
		int base_ratio = 0;
		bool downscaling = false;
		int srcSize = src_crop_width * src_crop_height;
		int destSize = dest_crop_width * dest_crop_height;

		if(interlaced)
			base_ratio = 7; // OK: 8
		else
			base_ratio = 5;

		if( destSize < srcSize)
			downscaling = true;

		if( downscaling && (destSize <= ((pMParm->mLcd_width*pMParm->mLcd_height*base_ratio)/10)))
		{
			able_to_otf_mode = false;
		}
		else
		{
			able_to_otf_mode = true;
		}
        }
#endif

//    if(interlaced && pMParm->mAdditionalCropSource)
//       able_to_otf_mode = false;

//	getfromsysfs("/sys/class/tcc_dispman/tcc_dispman/persist_output_mode", value);
//	uiOutputMode = atoi(value);
	uiOutputMode = 0;	// LCD only
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE 
	if( !pMParm->mDisplayOutputSTB && (uiOutputMode == OUTPUT_COMPONENT || uiOutputMode == OUTPUT_COMPOSITE) )
		able_to_otf_mode = false;
#endif

	property_get("persist.sys.hdmi_resolution", value, "0");
	uiHdmiResolution = atoi(value);

#ifdef USE_OUTPUT_COMPONENT_COMPOSITE 
	property_get("persist.sys.component_mode", value, "0");
	uiComponentMode = atoi(value);
#endif

	property_get("persist.sys.hdmi_detected_res", value, "0");
	uiDetectedResolution = atoi(value);

	property_get("ro.board.platform", value, "");
	if(!strncmp((char*)value, "tcc893x", 7))
	{
		if(uiOutputMode == 1) {	//HDMI
			switch(uiHdmiResolution)
			{
				case 2:		//1920x1080i 60Hz
				case 3:		//1920x1080i 50Hz
					//check interlace bypass condition
					if(!(interlaced && (pMParm->mScaler_arg.src_ImgWidth == pMParm->mScaler_arg.dest_ImgWidth)
						&& (pMParm->mScaler_arg.src_ImgHeight ==pMParm->mScaler_arg.dest_ImgHeight)))
					able_to_otf_mode = false;
					break;
				case 125:	//AutoDetect
					if(uiDetectedResolution == 2 || uiDetectedResolution == 3){
						if(!(interlaced && (pMParm->mScaler_arg.src_ImgWidth == pMParm->mScaler_arg.dest_ImgWidth)
							&& (pMParm->mScaler_arg.src_ImgHeight ==pMParm->mScaler_arg.dest_ImgHeight)))
						able_to_otf_mode = false;
					}
				default:
					break;
			}
		} 
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE
        else if(uiOutputMode == 2) { //Composite
			able_to_otf_mode = false;
		}else if(uiOutputMode == 3) {
			if(uiComponentMode == 1)
				able_to_otf_mode = false;
		}
#endif 
	}

	if( pMParm->mOntheflymode != 0 && pMParm->mOntheflymode != 1 )
		LOGE("pMParm->mOntheflymode value is strange. %d", pMParm->mOntheflymode);

	if( able_to_otf_mode )
	{
		property_get("persist.sys.renderer_onthefly", value, NULL);
		if (strcmp(value, "true") == 0){
			if(pMParm->mOntheflymode == 0){
				LOGI("mOntheflymode is changed '%d' into '1', on-the-fly mode will be used", pMParm->mOntheflymode);
				pMParm->mOntheflymode = 1;
			}
		}
		else
			pMParm->mOntheflymode = 0;
	}
	else
	{
		if(pMParm->mOntheflymode)
		{
			LOGI("mOntheflymode is changed '%d' into '0', M-to-M mode will be used", pMParm->mOntheflymode);
			pMParm->mOntheflymode = 0;
		}
	}

	return 0;
}

bool HwRenderer::output_Overlay(output_overlay_param_t * pOverlayInfo, bool *overlayCheckRet)
{
	float rate;
	int decodedSize;
	int displaySize;
	bool bBufferchange = false;

	pMParm->mFrame_cnt++;

	if(pMParm->mOutputMode == 1
#ifdef USE_LCD_OTF_SCALING
		|| ( !pMParm->mTransform && pMParm->mOntheflymode && !pMParm->mVideoDeinterlaceFlag )
#endif
#ifdef USE_LCD_VIDEO_VSYNC
		|| ( !pMParm->mTransform && pMParm->mOntheflymode && pMParm->mOutputMode == 0 && pMParm->mVsyncMode )
#endif
	)//external display mode
	{
	#if defined(TCC_VIDEO_DISPLAY_BY_VSYNC_INT) && defined(TCC_VIDEO_DEINTERLACE_SUPPORT)
		if(pMParm->mVsyncMode && pMParm->mVideoDeinterlaceFlag)
		{
			#if defined(_TCC8800_)
			decodedSize = mDecodedWidth * mDecodedHeight;
			displaySize = pMParm->mTarget_DispInfo.width * pMParm->mTarget_DispInfo.height;
			if (decodedSize > 2000000)
				rate = 0.64;
			else if(decodedSize > 1500000)
				rate = 0.44;
			else
				rate = 0.25;

			if(decodedSize * rate > displaySize)
				pMParm->mVideoDeinterlaceToMemory = 1;		// VIQE-Scaler on-the-fly (30Hz mode) and image write to memory.
			else  //((mDecodedWidth * mDecodedHeight) <= (pMParm->mTarget_DispInfo.width * pMParm->mTarget_DispInfo.height * 8))
				pMParm->mVideoDeinterlaceToMemory = 0;		// VIQE-Scaler-LCD on-the-fly (60Hz mode) and video image write to LCDC directly.
			#endif

			if(*overlayCheckRet)	// overlay check result is changed, so viqe/scaler driver must be closed.
			{
				int result;
				result = ioctl(pMParm->mFb_fd, TCC_LCDC_VIDEO_SET_SIZE_CHANGE, 0);
				LOGI("Inform that display size is changed, %d", result);

			}
		}
		#if defined(_TCC8800_)
		if(pMParm->mVsyncMode && pMParm->mVideoDeinterlaceFlag && pMParm->mVideoDeinterlaceToMemory == 0)		// VIQE-Scaler-LCD will be use for de-interlace.
		{
			struct tcc_lcdc_image_update lcdc_display;
			memset(&lcdc_display, 0x00, sizeof(lcdc_display));

			lcdc_display.Lcdc_layer = LCDC_VIDEO_CHANNEL;
			lcdc_display.enable = 1;
			lcdc_display.addr0 = (unsigned int) pOverlayInfo->YaddrPhy;
			lcdc_display.addr1 = (unsigned int) pOverlayInfo->UaddrPhy;
			lcdc_display.addr2 = (unsigned int) pOverlayInfo->VaddrPhy;

			lcdc_display.offset_x = pMParm->mTarget_DispInfo.sx;
			lcdc_display.offset_y = pMParm->mTarget_DispInfo.sy;

			lcdc_display.Frame_width = ((mDecodedWidth+15)>>4)<<4;	// iVideoDisplayWidth -> iVideoWidth
			lcdc_display.Frame_height = mDecodedHeight;
			lcdc_display.Image_width = ((pMParm->mDispWidth+15)>>4)<<4;
			lcdc_display.Image_height = pMParm->mDispHeight;

			// +----------------+----
			// | src            |  crop_top
			// |   +------+     |----
			// |   | dest |     |
			// |   |      |     |
			// |   +------+     |-----
			// |                |  crop_bottom
			// +----------------+-----
			// |<->|      |<--->|
			// crop_left   crop_right
			//
			// If vsync will be use, cropping informations have to send to fb_driver.
			lcdc_display.crop_top = pMParm->mTarget_CropInfo.sy;
			lcdc_display.crop_bottom = lcdc_display.Frame_height - pMParm->mTarget_CropInfo.srcH - pMParm->mTarget_CropInfo.sy;
			lcdc_display.crop_left = pMParm->mTarget_CropInfo.sx;
			lcdc_display.crop_right = lcdc_display.Frame_width - pMParm->mTarget_CropInfo.srcW - pMParm->mTarget_CropInfo.sx;
			lcdc_display.on_the_fly = 0;

			if(mColorFormat == OMX_COLOR_FormatYUV420SemiPlanar)
				lcdc_display.fmt = TCC_LCDC_IMG_FMT_YUV420ITL0;
			else if(mColorFormat == OMX_COLOR_FormatYUV422Planar)
				lcdc_display.fmt = TCC_LCDC_IMG_FMT_YUV422SP;
			else
				lcdc_display.fmt = TCC_LCDC_IMG_FMT_YUV420SP;

			lcdc_display.time_stamp = pOverlayInfo->timeStamp;
			lcdc_display.sync_time = pOverlayInfo->curTime;
			lcdc_display.buffer_unique_id = pOverlayInfo->buffer_id;
			lcdc_display.overlay_used_flag = 1;
			if(pMParm->mHDMIOutput)
				lcdc_display.outputMode = OUTPUT_HDMI;
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE
			else if(pMParm->mComponentOutput)
				lcdc_display.outputMode = OUTPUT_COMPONENT;
			 else if(pMParm->mCompositeOutput)
			 	lcdc_display.outputMode = OUTPUT_COMPOSITE;
#endif  
			else
				lcdc_display.outputMode = OUTPUT_NONE;

			lcdc_display.deinterlace_mode = 1;//(pOverlayInfo->flags & 0x40000000)?1:0; //OMX_BUFFERFLAG_INTERLACED_FRAME
			lcdc_display.odd_first_flag = (pOverlayInfo->flags & 0x10000000)?1:0; //OMX_BUFFERFLAG_ODD_FIRST_FRAME

			lcdc_display.first_frame_after_seek = (pOverlayInfo->flags & 0x00000001) ?1:0;
			lcdc_display.output_path = (pOverlayInfo->flags & 0x00000002) ?1:0;
			lcdc_display.m2m_mode = 0;
			lcdc_display.output_toMemory = 0;

			pMParm->mOverlayBuffer_Current = ioctl(pMParm->mFb_fd, TCC_LCDC_VIDEO_PUSH_VSYNC, &lcdc_display);

			//LOGE("### pMParm->mOverlayBuffer_Current %d", pMParm->mOverlayBuffer_Current) ;
			//LOGE("### mio PUSH vtime(%d), ctime(%d), b_indx(%d)", pOverlayInfo->timeStamp, pOverlayInfo->curTime,  pOverlayInfo->buffer_id) ;

		}
		else
		#endif
	#endif
		{
			int ret = -1;
			char support_vsync = false, supprot_STB = false;
			memset(&pMParm->extend_display, 0x00, sizeof(pMParm->extend_display));

			if(pMParm->mHDMIOutput)	{
				support_vsync = true;
				supprot_STB = true;
			}
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE
			else if(pMParm->mCompositeOutput) {
				support_vsync = true;
				supprot_STB = true;
			}
			else if(pMParm->mComponentOutput) {
				support_vsync = true;
				supprot_STB = true;
			}
#endif
	#if defined(USE_LCD_VIDEO_VSYNC)
			else {
				support_vsync = true;
			}
	#endif

			if(pMParm->mDisplayOutputSTB)
			{
				exclusive_ui_params exclusive_ui_param;
				memset(&exclusive_ui_param, 0x00, sizeof(exclusive_ui_param));

				if(pMParm->mExclusive_Enable)
				{
					if(exclusive_ui_param.enable)
						pMParm->mExclusive_Display = true;
					else
						pMParm->mExclusive_Display = false;
				}
				else
				{
					pMParm->mExclusive_Display = false;
				}
			}

			if(pMParm->mVideoDeinterlaceFlag == true)
			{
		#if !defined(_TCC8800_)
				if((supprot_STB && pMParm->mDisplayOutputSTB && pMParm->mExclusive_Display == true)
					|| ( pMParm->mOntheflymode && (pMParm->mVsyncMode && support_vsync)))
					pMParm->mVideoDeinterlaceToMemory = 0;
				else
					pMParm->mVideoDeinterlaceToMemory = 1;
		#endif

				pMParm->mVIQE_onoff = true;
				pMParm->mVIQE_onthefly = true;

				if (pMParm->mVideoDeinterlaceForce)
					pMParm->mVIQE_DI_use = pMParm->mViqe_arg.DI_use = 1;
				else
					pMParm->mVIQE_DI_use = pMParm->mViqe_arg.DI_use = (pOverlayInfo->flags & 0x40000000)?1:0; //OMX_BUFFERFLAG_INTERLACED_FRAME
				pMParm->mViqe_arg.OddFirst = (pOverlayInfo->flags & 0x10000000)?1:0; //OMX_BUFFERFLAG_ODD_FIRST_FRAME

				pMParm->mViqe_arg.crop_top = pMParm->mTarget_CropInfo.sy;
				pMParm->mViqe_arg.crop_bottom = mDecodedHeight - pMParm->mTarget_CropInfo.srcH - pMParm->mTarget_CropInfo.sy;
				pMParm->mViqe_arg.crop_left = pMParm->mTarget_CropInfo.sx;
				pMParm->mViqe_arg.crop_right = mDecodedWidth - pMParm->mTarget_CropInfo.srcW - pMParm->mTarget_CropInfo.sx;
				// In this case, decoded output needs deinterlace but target size is too small (below source area / 8),
				// so viqe-scaler will be write to pMParm->mOverlayBuffer_PhyAddr[pMParm->mOverlayBuffer_Current].
				if(pMParm->mVideoDeinterlaceToMemory)
					VIQEProcess(pOverlayInfo->YaddrPhy, pOverlayInfo->UaddrPhy, pOverlayInfo->VaddrPhy, (unsigned int)pMParm->mOverlayBuffer_PhyAddr[pMParm->mOverlayBuffer_Current]);

				DBUG("@@VIQE@@ %d x %d -> %d x %d, Viqe path = %d : %s, on-the-fly(%d)",
					mDecodedWidth, mDecodedHeight, pMParm->mTarget_DispInfo.width, pMParm->mTarget_DispInfo.height,
					pMParm->mVideoDeinterlaceToMemory, pMParm->mVideoDeinterlaceToMemory ? "VIQE-Scaler-Memory" : "(VIQE)-Scaler-LCDC", pMParm->mOntheflymode);
			}
			else
			{
				// In this case, decoded output needs deinterlace but 89/92 platform doesn't support deinterlace,
				// so interlace image will be deinterlacing by scaler on overlayProcess function.
				pMParm->mScaler_arg.interlaced = (pOverlayInfo->flags & 0x40000000)?1:0;
				pMParm->mVideoDeinterlaceToMemory = 0;
			}

			if( !pMParm->mVideoDeinterlaceToMemory &&
				( (supprot_STB && pMParm->mDisplayOutputSTB && pMParm->mExclusive_Display == true)
	#if defined(TCC_VIDEO_DISPLAY_BY_VSYNC_INT)
				|| ( pMParm->mOntheflymode && (pMParm->mVsyncMode && support_vsync))
	#endif
				|| (pMParm->mOntheflymode))
			)
			{

				pMParm->extend_display.Lcdc_layer = LCDC_VIDEO_CHANNEL;
				pMParm->extend_display.enable = 1;
				pMParm->extend_display.addr0 = (unsigned int)pOverlayInfo->YaddrPhy;
				pMParm->extend_display.addr1 = (unsigned int)pOverlayInfo->UaddrPhy;
				pMParm->extend_display.addr2 = (unsigned int)pOverlayInfo->VaddrPhy;

				pMParm->extend_display.Frame_width 	= pMParm->mScaler_arg.src_ImgWidth;
				pMParm->extend_display.Frame_height = pMParm->mScaler_arg.src_ImgHeight;
				pMParm->extend_display.offset_x 	= pMParm->mTarget_DispInfo.sx;
				pMParm->extend_display.offset_y 	= pMParm->mTarget_DispInfo.sy;

				pMParm->extend_display.Image_width = pMParm->mScaler_arg.dest_ImgWidth;
				pMParm->extend_display.Image_height = pMParm->mScaler_arg.dest_ImgHeight;

				pMParm->extend_display.one_field_only_interlace = pMParm->mVideoDeinterlaceScaleUpOneFieldFlag ? 1 : 0;
				pMParm->extend_display.codec_id = pMParm->mUnique_address;

				if(pMParm->mHDMIOutput) {
					pMParm->extend_display.outputMode = OUTPUT_HDMI;
				}
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE
				else if(pMParm->mCompositeOutput) {
					pMParm->extend_display.outputMode = OUTPUT_COMPOSITE;
				}
				else if(pMParm->mComponentOutput) {
					pMParm->extend_display.outputMode = OUTPUT_COMPONENT;
				}
#endif
				else {
					pMParm->extend_display.outputMode = OUTPUT_NONE;
				}

				//Image crop
				pMParm->extend_display.crop_top = pMParm->mTarget_CropInfo.sy;
				if( (pMParm->extend_display.crop_top + pMParm->mTarget_CropInfo.srcH) <= mDecodedHeight)
					pMParm->extend_display.crop_bottom = pMParm->extend_display.crop_top + pMParm->mTarget_CropInfo.srcH;
				else
					pMParm->extend_display.crop_bottom = mDecodedHeight - pMParm->extend_display.crop_top;

				pMParm->extend_display.crop_left = pMParm->mTarget_CropInfo.sx;
				if( pMParm->extend_display.crop_left+pMParm->mTarget_CropInfo.srcW <= mDecodedWidth )
					pMParm->extend_display.crop_right = pMParm->extend_display.crop_left + pMParm->mTarget_CropInfo.srcW;
				else
					pMParm->extend_display.crop_right = mDecodedWidth - pMParm->extend_display.crop_left;

				if((pMParm->mScaler_arg.dest_ImgWidth == pMParm->mScaler_arg.src_ImgWidth) && (pMParm->mScaler_arg.dest_ImgHeight == pMParm->mScaler_arg.src_ImgHeight))
					pMParm->extend_display.on_the_fly = 1;  // same size Source size and Display size
				else
					pMParm->extend_display.on_the_fly = 1;

				if(mColorFormat == OMX_COLOR_FormatYUV420SemiPlanar)
					pMParm->extend_display.fmt = TCC_LCDC_IMG_FMT_YUV420ITL0;
				else if(mColorFormat == OMX_COLOR_FormatYUV422Planar)
					pMParm->extend_display.fmt = TCC_LCDC_IMG_FMT_YUV422SP;
				else
					pMParm->extend_display.fmt = TCC_LCDC_IMG_FMT_YUV420SP;

				if(pMParm->mDisplayOutputSTB && supprot_STB && pMParm->mExclusive_Display == true)
				{
					pMParm->extend_display.dst_addr0 = (unsigned int)pMParm->mOverlayBuffer_PhyAddr[pMParm->mOverlayBuffer_Current];
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE 
					if(pMParm->mCompositeOutput) {
						ioctl(pMParm->mComposite_fd, TCC_COMPOSITE_IOCTL_PROCESS, &pMParm->extend_display);
					}
					else if(pMParm->mComponentOutput) {
						ioctl(pMParm->mComponent_fd, TCC_COMPONENT_IOCTL_PROCESS, &pMParm->extend_display);
					}
#endif
				}

				//LOGV("hdmi setting buffer:0x%x Y:0x%x U:0x%x V:0x%x W:%d H:%d",
				//	pMParm->mOverlayBuffer_PhyAddr[pMParm->mOverlayBuffer_Current], hdmi_display.addr0, hdmi_display.addr1, hdmi_display.addr2, hdmi_display.Image_width, hdmi_display.Image_height);

			#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
				if(pMParm->mVsyncMode && support_vsync)
				{
					pMParm->extend_display.time_stamp = pOverlayInfo->timeStamp;
					pMParm->extend_display.sync_time = pOverlayInfo->curTime;
					pMParm->extend_display.buffer_unique_id = pOverlayInfo->buffer_id;
					pMParm->extend_display.MVCframeView = pOverlayInfo->frameView;

					if(pMParm->extend_display.MVCframeView)
					{
						pMParm->extend_display.MVC_Base_addr0 = (unsigned int)pOverlayInfo->MVC_Base_YaddrPhy;
						pMParm->extend_display.MVC_Base_addr1 = (unsigned int)pOverlayInfo->MVC_Base_UaddrPhy;
						pMParm->extend_display.MVC_Base_addr2 = (unsigned int)pOverlayInfo->MVC_Base_VaddrPhy;

						//LOGD("MVC D view with Base Y:%x  U:%x  V:%x",
						//	pMParm->extend_display.MVC_Base_addr0, pMParm->extend_display.MVC_Base_addr1, pMParm->extend_display.MVC_Base_addr2 ) ;
					}

					pMParm->extend_display.overlay_used_flag = 0;
					if(pMParm->mVideoDeinterlaceFlag == true)
					{
						pMParm->extend_display.deinterlace_mode = 1;

						if(pMParm->mVideoDeinterlaceForce)
							pMParm->extend_display.frameInfo_interlace = 1;
						else
							pMParm->extend_display.frameInfo_interlace = (pOverlayInfo->flags & 0x40000000)?1:0; //OMX_BUFFERFLAG_INTERLACED_FRAME
						pMParm->extend_display.odd_first_flag = (pOverlayInfo->flags & 0x10000000)?1:0; //OMX_BUFFERFLAG_ODD_FIRST_FRAME
					}
					else
					{
						pMParm->extend_display.deinterlace_mode = 0;
						pMParm->extend_display.frameInfo_interlace = 0;
						pMParm->extend_display.odd_first_flag = 0;
					}

					pMParm->extend_display.deinterlace_mode = 1;
					pMParm->extend_display.on_the_fly = 0;
					pMParm->extend_display.m2m_mode = 1;
					pMParm->extend_display.output_toMemory = 1;
					pMParm->extend_display.fps = pMParm->mFrame_rate;
					pMParm->extend_display.first_frame_after_seek = (pOverlayInfo->flags & 0x00000001) ?1:0;
					pMParm->extend_display.output_path = (pOverlayInfo->flags & 0x00000002) ?1:0;
#if 1
					if(pMParm->mOverlayBuffer_Current < (VSYNC_OVERLAY_COUNT/2)){
						pMParm->extend_display.dst_addr0 = (unsigned int)pMParm->mOverlayBuffer_PhyAddr[pMParm->mOverlayBuffer_Current*2];
						pMParm->extend_display.dst_addr1 = (unsigned int)pMParm->mOverlayBuffer_PhyAddr[(pMParm->mOverlayBuffer_Current*2)+1];
					}else{
						pMParm->extend_display.dst_addr0 = (unsigned int)pMParm->mOverlayBuffer_PhyAddr[(pMParm->mOverlayBuffer_Current*2) - VSYNC_OVERLAY_COUNT];
						pMParm->extend_display.dst_addr1 = (unsigned int)pMParm->mOverlayBuffer_PhyAddr[((pMParm->mOverlayBuffer_Current*2) - VSYNC_OVERLAY_COUNT)+1];
					}
#else
					if(pMParm->mOverlayBuffer_Current < (VSYNC_OVERLAY_COUNT)){
						pMParm->extend_display.dst_addr0 = (unsigned int)pMParm->mOverlayBuffer_PhyAddr[pMParm->mOverlayBuffer_Current];
						pMParm->extend_display.dst_addr1 = (unsigned int)pMParm->mOverlayBuffer_PhyAddr[(pMParm->mOverlayBuffer_Current)+1];
					}else{
						pMParm->extend_display.dst_addr0 = (unsigned int)pMParm->mOverlayBuffer_PhyAddr[(pMParm->mOverlayBuffer_Current) - VSYNC_OVERLAY_COUNT];
						pMParm->extend_display.dst_addr1 = (unsigned int)pMParm->mOverlayBuffer_PhyAddr[((pMParm->mOverlayBuffer_Current) - VSYNC_OVERLAY_COUNT)+1];
					}
#endif
					DBUG("### mio PUSH Start1");
					pMParm->mOverlayBuffer_Current = ioctl(pMParm->mFb_fd, TCC_LCDC_VIDEO_PUSH_VSYNC, &pMParm->extend_display);
					//LOGE("### mio PUSH vtime(%d), ctime(%d), b_indx(%d)", pOverlayInfo->timeStamp, pOverlayInfo->curTime,  pOverlayInfo->buffer_id) ;
				}
				else
			#endif
				{
					if(pMParm->mHDMIOutput) {
						ret = ioctl(pMParm->mFb_fd, TCC_LCDC_HDMI_DISPLAY, &pMParm->extend_display);
					}
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE
					else if(pMParm->mCompositeOutput) {
						ret = ioctl(pMParm->mComposite_fd, TCC_COMPOSITE_IOCTL_UPDATE, &pMParm->extend_display);
					}
					else if(pMParm->mComponentOutput) {
						ret = ioctl(pMParm->mComponent_fd, TCC_COMPONENT_IOCTL_UPDATE, &pMParm->extend_display);
					}
#endif
		#ifdef USE_LCD_OTF_SCALING
					else {
						ret = ioctl(pMParm->mFb_fd, TCC_LCDC_DISPLAY_UPDATE, &pMParm->extend_display);
					}
		#endif
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE
					if(ret < 0)	{
						LOGE("Extend Display update %d ERROR[%d] HDMI:%d Composite:%d Component:%d", __LINE__, ret, pMParm->mHDMIOutput, pMParm->mCompositeOutput, pMParm->mComponentOutput);
					}
#else

					if(ret < 0)	{
						LOGE("Extend Display update %d ERROR[%d] HDMI:%d", __LINE__, ret, pMParm->mHDMIOutput);
					}
#endif
				}
			}
			else if(overlayProcess(&pMParm->mScaler_arg, &pMParm->mGrp_arg, pOverlayInfo->YaddrPhy, pOverlayInfo->UaddrPhy, pOverlayInfo->VaddrPhy, pMParm->mOverlayBuffer_PhyAddr[pMParm->mOverlayBuffer_Current]) >= 0)
			{
				pMParm->extend_display.Lcdc_layer = LCDC_VIDEO_CHANNEL;
				pMParm->extend_display.enable = 1;
				pMParm->extend_display.addr0 = (unsigned int)pMParm->mScaler_arg.dest_Yaddr;
				pMParm->extend_display.addr1 = (unsigned int)pMParm->mScaler_arg.dest_Uaddr;
				pMParm->extend_display.addr2 = (unsigned int)pMParm->mScaler_arg.dest_Vaddr;

				pMParm->extend_display.offset_x = pMParm->mTarget_DispInfo.sx;
				pMParm->extend_display.offset_y = pMParm->mTarget_DispInfo.sy;
				pMParm->extend_display.Frame_width = pMParm->mScaler_arg.dest_ImgWidth;
				pMParm->extend_display.Frame_height = pMParm->mScaler_arg.dest_ImgHeight;

				pMParm->extend_display.Image_width = pMParm->mScaler_arg.dest_ImgWidth;
				pMParm->extend_display.Image_height = pMParm->mScaler_arg.dest_ImgHeight;

				pMParm->extend_display.one_field_only_interlace = pMParm->mVideoDeinterlaceScaleUpOneFieldFlag ? 1 : 0;

				pMParm->extend_display.crop_left = 0;
				pMParm->extend_display.crop_top = 0;
				pMParm->extend_display.crop_right = pMParm->mScaler_arg.dest_ImgWidth;
				pMParm->extend_display.crop_bottom = pMParm->mScaler_arg.dest_ImgHeight;
				pMParm->extend_display.codec_id = pMParm->mUnique_address;

				if(pMParm->mHDMIOutput) {
					pMParm->extend_display.outputMode = OUTPUT_HDMI;
				}
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE
				else if(pMParm->mCompositeOutput) {
					pMParm->extend_display.outputMode = OUTPUT_COMPOSITE;
				}
				else if(pMParm->mComponentOutput) {
					pMParm->extend_display.outputMode = OUTPUT_COMPONENT;
				}
#endif  
				else {
					pMParm->extend_display.outputMode = OUTPUT_NONE;
				}

				if(pMParm->mScaler_arg.dest_fmt == SCALER_YUV420_inter)
					pMParm->extend_display.fmt = TCC_LCDC_IMG_FMT_YUV420ITL0;
				else
					pMParm->extend_display.fmt = TCC_LCDC_IMG_FMT_YUYV;

				if(pMParm->mScaler_arg.viqe_onthefly == 0x2)	{
					#ifdef VIDEO_PLAY_USE_SCALER1
					pMParm->extend_display.on_the_fly = 2;
					#else
					pMParm->extend_display.on_the_fly = 1;
					#endif //
				}
				else {
					pMParm->extend_display.on_the_fly = 0;
				}


				#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
				if(pMParm->mVsyncMode && support_vsync)
				{
					pMParm->extend_display.time_stamp = pOverlayInfo->timeStamp;
					pMParm->extend_display.sync_time = pOverlayInfo->curTime;
					pMParm->extend_display.buffer_unique_id = pOverlayInfo->buffer_id;
					pMParm->extend_display.MVCframeView = pOverlayInfo->frameView;
					if(pMParm->extend_display.MVCframeView)
					{
						pMParm->extend_display.MVC_Base_addr0 = (unsigned int)pOverlayInfo->MVC_Base_YaddrPhy;
						pMParm->extend_display.MVC_Base_addr1 = (unsigned int)pOverlayInfo->MVC_Base_UaddrPhy;
						pMParm->extend_display.MVC_Base_addr2 = (unsigned int)pOverlayInfo->MVC_Base_VaddrPhy;
					}
					pMParm->extend_display.overlay_used_flag = 1;
					if(pMParm->mVideoDeinterlaceFlag == true)
					{
						pMParm->extend_display.deinterlace_mode = 1;
						if(pMParm->mVideoDeinterlaceForce)
							pMParm->extend_display.frameInfo_interlace = 1;
						else
							pMParm->extend_display.frameInfo_interlace = (pOverlayInfo->flags & 0x40000000)?1:0; //OMX_BUFFERFLAG_INTERLACED_FRAME
						pMParm->extend_display.odd_first_flag = (pOverlayInfo->flags & 0x10000000)?1:0; //OMX_BUFFERFLAG_ODD_FIRST_FRAME
					}
					else
					{
						pMParm->extend_display.deinterlace_mode = 0;
						pMParm->extend_display.frameInfo_interlace = 0;
						pMParm->extend_display.odd_first_flag = 0;
					}
					pMParm->extend_display.m2m_mode = 0;
					pMParm->extend_display.output_toMemory = 1;
					pMParm->extend_display.first_frame_after_seek = (pOverlayInfo->flags & 0x00000001) ?1:0;
					pMParm->extend_display.output_path = (pOverlayInfo->flags & 0x00000002) ?1:0;

					DBUG("$$$ mio PUSH Start2") ;
						pMParm->mOverlayBuffer_Current = ioctl(pMParm->mFb_fd, TCC_LCDC_VIDEO_PUSH_VSYNC, &pMParm->extend_display);
					//LOGE("### pMParm->mOverlayBuffer_Current %d", pMParm->mOverlayBuffer_Current) ;
					//LOGE("$$$ mio PUSH vtime(%d), ctime(%d), b_indx(%d)", pOverlayInfo->timeStamp, pOverlayInfo->curTime,  pOverlayInfo->buffer_id) ;
				}
				else
				#endif
				{
					if(pMParm->mHDMIOutput) {
						ret = ioctl(pMParm->mFb_fd, TCC_LCDC_HDMI_DISPLAY, &pMParm->extend_display);
					}
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE
					else if(pMParm->mCompositeOutput) {
						ret = ioctl(pMParm->mComposite_fd, TCC_COMPOSITE_IOCTL_UPDATE, &pMParm->extend_display);
					}
					else if(pMParm->mComponentOutput) {
						ret = ioctl(pMParm->mComponent_fd, TCC_COMPONENT_IOCTL_UPDATE, &pMParm->extend_display);
					}

					if(ret < 0)	{
						LOGE("Extend Display update %d ERROR[%d] HDMI:%d Composite:%d Component:%d", __LINE__, ret, pMParm->mHDMIOutput, pMParm->mCompositeOutput, pMParm->mComponentOutput);
					}
#else
					if(ret < 0)	{
						LOGE("Extend Display update %d ERROR[%d] HDMI:%d", __LINE__, ret, pMParm->mHDMIOutput);
					}
#endif
				}
			}
		}
	}
	else
	{
		#if !defined(_TCC8800_)
		if(pMParm->mVideoDeinterlaceFlag == true)
		{
			pMParm->mVideoDeinterlaceToMemory = 1;
			pMParm->mVIQE_onoff = true;
			pMParm->mVIQE_onthefly = true;

			if (pMParm->mVideoDeinterlaceForce){
				pMParm->mVIQE_DI_use = 1;
				pMParm->mViqe_arg.DI_use = 1;
			}else
				pMParm->mVIQE_DI_use = pMParm->mViqe_arg.DI_use = (pOverlayInfo->flags & 0x40000000)?1:0; //OMX_BUFFERFLAG_INTERLACED_FRAME
			pMParm->mViqe_arg.OddFirst = (pOverlayInfo->flags & 0x10000000)?1:0; //OMX_BUFFERFLAG_ODD_FIRST_FRAME

			pMParm->mViqe_arg.crop_top = pMParm->mTarget_CropInfo.sy;
			pMParm->mViqe_arg.crop_bottom = mDecodedHeight - pMParm->mTarget_CropInfo.srcH - pMParm->mTarget_CropInfo.sy;
			pMParm->mViqe_arg.crop_left = pMParm->mTarget_CropInfo.sx;
			pMParm->mViqe_arg.crop_right = mDecodedWidth - pMParm->mTarget_CropInfo.srcW - pMParm->mTarget_CropInfo.sx;
			// In this case, decoded output needs deinterlace but target size is too small (below source area / 8),
			// so viqe-scaler will be write to pMParm->mOverlayBuffer_PhyAddr[pMParm->mOverlayBuffer_Current].
			if(pMParm->mVideoDeinterlaceToMemory)
				VIQEProcess(pOverlayInfo->YaddrPhy, pOverlayInfo->UaddrPhy, pOverlayInfo->VaddrPhy, (unsigned int)pMParm->mOverlayBuffer_PhyAddr[pMParm->mOverlayBuffer_Current]);

			DBUG("LCD :: @@VIQE@@ %d x %d -> %d x %d, Viqe path = %d : %s, on-the-fly(%d)",
				mDecodedWidth, mDecodedHeight, pMParm->mTarget_DispInfo.width, pMParm->mTarget_DispInfo.height,
				pMParm->mVideoDeinterlaceToMemory, pMParm->mVideoDeinterlaceToMemory ? "VIQE-Scaler-Memory" : "(VIQE)-Scaler-LCDC", pMParm->mOntheflymode);

		}
		#endif

		if(pMParm->mParallel)
		{
			if(overlayProcess_Parallel(&pMParm->mScaler_arg, &pMParm->mGrp_arg, pOverlayInfo->YaddrPhy, pOverlayInfo->UaddrPhy, pOverlayInfo->VaddrPhy, (uint8_t *)pMParm->mOverlayBuffer_PhyAddr[pMParm->mOverlayBuffer_Current]) >= 0)
			{
				#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
				if(pMParm->mVsyncMode)
				{
					struct tcc_lcdc_image_update lcd_display;
					memset(&lcd_display, 0x00, sizeof(lcd_display));

					lcd_display.Lcdc_layer = LCDC_VIDEO_CHANNEL;
					lcd_display.enable = 1;
					lcd_display.addr0 = pMParm->mGrp_arg.tgt0;
					lcd_display.addr1 = pMParm->mGrp_arg.tgt1;
					lcd_display.addr2 = pMParm->mGrp_arg.tgt2;

					lcd_display.offset_x = pMParm->mTarget_DispInfo.sx;
					lcd_display.offset_y = pMParm->mTarget_DispInfo.sy;
					lcd_display.Frame_width = pMParm->mGrp_arg.dst_imgx;
					lcd_display.Frame_height = pMParm->mGrp_arg.dst_imgy;

					lcd_display.Image_width = pMParm->mGrp_arg.dst_imgx;
					lcd_display.Image_height = pMParm->mGrp_arg.dst_imgy;
					lcd_display.fmt = TCC_LCDC_IMG_FMT_YUYV;
					lcd_display.time_stamp = pOverlayInfo->timeStamp;
					lcd_display.sync_time = pOverlayInfo->curTime;
					lcd_display.buffer_unique_id = pOverlayInfo->buffer_id;
					lcd_display.overlay_used_flag = 1;

	    				if(pMParm->mHDMIOutput) {
							lcd_display.outputMode = OUTPUT_HDMI;
	    				}
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE
	    				else if(pMParm->mCompositeOutput) {
							lcd_display.outputMode = OUTPUT_COMPOSITE;
	    				}
	    				else if(pMParm->mComponentOutput) {
							lcd_display.outputMode = OUTPUT_COMPONENT;
	    				}
#endif 
	    				else {
							lcd_display.outputMode = OUTPUT_NONE;
	    				}

					lcd_display.crop_left = 0;
					lcd_display.crop_top = 0;
					lcd_display.crop_right = pMParm->mGrp_arg.dst_imgx;
					lcd_display.crop_bottom = pMParm->mGrp_arg.dst_imgy;

					lcd_display.deinterlace_mode = 0;
					lcd_display.m2m_mode = 0;
					lcd_display.output_toMemory = 1;
					lcd_display.odd_first_flag = 0;
					lcd_display.first_frame_after_seek = (pOverlayInfo->flags & 0x00000001) ?1:0;
					lcd_display.output_path = (pOverlayInfo->flags & 0x00000002) ?1:0;
					lcd_display.codec_id = pMParm->mUnique_address;

					DBUG_FRM("LCD :: Parallel - TCC_LCDC_VIDEO_PUSH_VSYNC :: vtime(%d), ctime(%d), b_indx(%d)", pOverlayInfo->timeStamp, pOverlayInfo->curTime,  pOverlayInfo->buffer_id)
					pMParm->mOverlayBuffer_Current = ioctl(pMParm->mFb_fd, TCC_LCDC_VIDEO_PUSH_VSYNC, &lcd_display);
				}
				else
				#endif
				{
					DBUG_FRM("LCD :: Paralel - OVERLAY_QUEUE_BUFFER :: goto end Video-%d %lld ms", pMParm->mFrame_cnt, systemTime()/1000000);

					overlay_video_buffer_t overlay_info;

					overlay_info.cfg.sx         = pMParm->mTarget_DispInfo.sx;
					overlay_info.cfg.sy         = pMParm->mTarget_DispInfo.sy;
					overlay_info.cfg.width      = pMParm->mTarget_DispInfo.width;
					overlay_info.cfg.height     = pMParm->mTarget_DispInfo.height;
					overlay_info.cfg.format     = V4L2_PIX_FMT_YUV422P; //420sequential
					overlay_info.addr           = (unsigned int)(pMParm->mOverlayBuffer_PhyAddr[pMParm->mOverlayBuffer_Current]);

					_ioctlOverlay(OVERLAY_PUSH_VIDEO_BUFFER, &overlay_info);
					bBufferchange = true;
				}
			}
		}
		else
		if(overlayProcess(&pMParm->mScaler_arg, &pMParm->mGrp_arg, pOverlayInfo->YaddrPhy, pOverlayInfo->UaddrPhy, pOverlayInfo->VaddrPhy, pMParm->mOverlayBuffer_PhyAddr[pMParm->mOverlayBuffer_Current]) >= 0)
		{
			#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
			if(pMParm->mVsyncMode)
			{
				struct tcc_lcdc_image_update lcd_display;
				memset(&lcd_display, 0x00, sizeof(lcd_display));

				lcd_display.Lcdc_layer = LCDC_VIDEO_CHANNEL;
				lcd_display.enable = 1;
				lcd_display.addr0 = (unsigned int)pMParm->mScaler_arg.dest_Yaddr;
				lcd_display.addr1 = (unsigned int)pMParm->mScaler_arg.dest_Uaddr;
				lcd_display.addr2 = (unsigned int)pMParm->mScaler_arg.dest_Vaddr;

				lcd_display.offset_x = pMParm->mTarget_DispInfo.sx;
				lcd_display.offset_y = pMParm->mTarget_DispInfo.sy;
				lcd_display.Frame_width = pMParm->mScaler_arg.dest_ImgWidth;
				lcd_display.Frame_height = pMParm->mScaler_arg.dest_ImgHeight;

				lcd_display.Image_width = pMParm->mScaler_arg.dest_ImgWidth;
				lcd_display.Image_height = pMParm->mScaler_arg.dest_ImgHeight;

				#if !defined(_TCC8800_)
				lcd_display.fmt = TCC_LCDC_IMG_FMT_YUYV;
				#else
				lcd_display.fmt = TCC_LCDC_IMG_FMT_YUV422SQ;
				#endif//

				lcd_display.time_stamp = pOverlayInfo->timeStamp;
				lcd_display.sync_time = pOverlayInfo->curTime;
				lcd_display.buffer_unique_id = pOverlayInfo->buffer_id;
				lcd_display.MVCframeView= pOverlayInfo->frameView;
				lcd_display.overlay_used_flag = 1;

				if(pMParm->mHDMIOutput) {
					lcd_display.outputMode = OUTPUT_HDMI;
				}
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE 
				else if(pMParm->mCompositeOutput) {
					lcd_display.outputMode = OUTPUT_COMPOSITE;
				}
				else if(pMParm->mComponentOutput) {
					lcd_display.outputMode = OUTPUT_COMPONENT;
				}
#endif
				else {
					lcd_display.outputMode = OUTPUT_NONE;
				}

				lcd_display.crop_left = 0;
				lcd_display.crop_top = 0;
				lcd_display.crop_right = pMParm->mScaler_arg.dest_ImgWidth;
				lcd_display.crop_bottom = pMParm->mScaler_arg.dest_ImgHeight;

				lcd_display.deinterlace_mode = 0;
				lcd_display.m2m_mode = 0;
				lcd_display.output_toMemory = 1;
				lcd_display.odd_first_flag = 0;
				lcd_display.first_frame_after_seek = (pOverlayInfo->flags & 0x00000001) ?1:0;
				lcd_display.output_path = (pOverlayInfo->flags & 0x00000002) ?1:0;
				lcd_display.codec_id = pMParm->mUnique_address;

				DBUG_FRM("LCD :: Single - TCC_LCDC_VIDEO_PUSH_VSYNC :: vtime(%d), ctime(%d), b_indx(%d)", pOverlayInfo->timeStamp, pOverlayInfo->curTime,  pOverlayInfo->buffer_id) ;

				pMParm->mOverlayBuffer_Current = ioctl(pMParm->mFb_fd, TCC_LCDC_VIDEO_PUSH_VSYNC, &lcd_display);

			}
			else
			#endif
			{
				DBUG_FRM("LCD :: Single - OVERLAY_QUEUE_BUFFER :: goto end Video-%d %lld ms", pMParm->mFrame_cnt, systemTime()/1000000);


				overlay_video_buffer_t overlay_info;

				overlay_info.cfg.sx         = pMParm->mTarget_DispInfo.sx;
				overlay_info.cfg.sy         = pMParm->mTarget_DispInfo.sy;
				overlay_info.cfg.width      = pMParm->mTarget_DispInfo.width;
				overlay_info.cfg.height     = pMParm->mTarget_DispInfo.height;
				overlay_info.cfg.format     = V4L2_PIX_FMT_YUV422P; //420sequential
				overlay_info.addr           = (unsigned int)(pMParm->mOverlayBuffer_PhyAddr[pMParm->mOverlayBuffer_Current]);

				_ioctlOverlay(OVERLAY_PUSH_VIDEO_BUFFER, &overlay_info);
				bBufferchange = true;
			}
		}

#ifdef USE_OUTPUT_COMPONENT_COMPOSITE
		if((pMParm->mOutputMode == 2) && (pMParm->mHDMIOutput || pMParm->mCompositeOutput || pMParm->mComponentOutput) ) //dual display
#else
		if((pMParm->mOutputMode == 2) && (pMParm->mHDMIOutput)) //dual display
#endif
		{
			tcc_display_size display_size;
			SCALER_TYPE ScaleInfo;
			int m2m_fd, ret = -1;
			unsigned int SrcWidth, SrcHeight, DestWidth, DestHeight;
			float fScaleWidth, fScaleHeight, ExLCD_fScale_w, ExLCD_fScale_h;
			OUTPUT_SELECT output_type;
			pos_info_t extendisplay_pos;

			if(pMParm->mHDMIOutput)
				output_type = OUTPUT_HDMI;
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE
			else if(pMParm->mCompositeOutput)
				output_type = OUTPUT_COMPOSITE;
			else if(pMParm->mComponentOutput)
				output_type = OUTPUT_COMPONENT;
#endif
			else
				output_type = OUTPUT_NONE;

			SrcWidth = ((mDecodedWidth+15)>>4)<<4;
			SrcHeight = mDecodedHeight;

			// get external display size
			getDisplaySize(output_type, &display_size);


			// calculate Extend display resize menu
			if(display_size.width > 0 && display_size.height > 0){
				display_size.width	= display_size.width  * (display_size.width - overlayResizeExternalDisplay(output_type, 0)) / display_size.width;
				display_size.height	= display_size.height * (display_size.height - overlayResizeExternalDisplay(output_type, 1)) / display_size.height;
			}
			//calculate LCD & Extend display size ratio
			ExLCD_fScale_w = (float)display_size.width / (float)pMParm->mLcd_width;
			ExLCD_fScale_h = (float)display_size.height / (float)pMParm->mLcd_height;

			pMParm->extend_display.Lcdc_layer = LCDC_VIDEO_CHANNEL;
			pMParm->extend_display.enable = 1;
			pMParm->extend_display.offset_x = (float)pMParm->mTarget_DispInfo.sx * ExLCD_fScale_w + overlayResizeGetPosition(output_type, 0);
			pMParm->extend_display.offset_y = (float)pMParm->mTarget_DispInfo.sy * ExLCD_fScale_h + overlayResizeGetPosition(output_type, 1);
			pMParm->extend_display.on_the_fly = 0;
			DBUG_FRM("LCD: %d %d ex size: %d %d Extend %d %d  ratio %f %f ",pMParm->mLcd_width, pMParm->mLcd_height, display_size.width, display_size.height, pMParm->extend_display.offset_x, pMParm->extend_display.offset_y, ExLCD_fScale_w, ExLCD_fScale_h);

			DBUG_FRM("%d %d %d %d", pMParm->mTarget_DispInfo.sx, pMParm->mTarget_DispInfo.sy, pMParm->mTarget_DispInfo.width, pMParm->mTarget_DispInfo.height);
			DBUG_FRM("%d %d %d %d", pMParm->mOrigin_DispInfo.sx, pMParm->mOrigin_DispInfo.sy, pMParm->mOrigin_DispInfo.width, pMParm->mOrigin_DispInfo.height);
			DBUG_FRM("%d %d %d %d", pMParm->mOrigin_RegionInfo.source.left, pMParm->mOrigin_RegionInfo.source.right, pMParm->mOrigin_RegionInfo.source.top, pMParm->mOrigin_RegionInfo.source.bottom);
			DBUG("%d %d %d %d", pMParm->mOrigin_RegionInfo.display.left, pMParm->mOrigin_RegionInfo.display.right, pMParm->mOrigin_RegionInfo.display.top, pMParm->mOrigin_RegionInfo.display.bottom);

			 if((pMParm->mNeed_scaler) && (display_size.width == pMParm->mLcd_width) && (display_size.height == pMParm->mLcd_height))
			{
				pMParm->extend_display.addr0 = (unsigned int)pMParm->mScaler_arg.dest_Yaddr;
				pMParm->extend_display.addr1 = (unsigned int)pMParm->mScaler_arg.dest_Uaddr;
				pMParm->extend_display.addr2 = (unsigned int)pMParm->mScaler_arg.dest_Vaddr;

				pMParm->extend_display.Frame_width = pMParm->mScaler_arg.dest_ImgWidth;
				pMParm->extend_display.Frame_height = pMParm->mScaler_arg.dest_ImgHeight;
				pMParm->extend_display.Image_width = pMParm->mScaler_arg.dest_ImgWidth;
				pMParm->extend_display.Image_height = pMParm->mScaler_arg.dest_ImgHeight;

				pMParm->extend_display.crop_left = 0;
				pMParm->extend_display.crop_top = 0;
				pMParm->extend_display.crop_right = pMParm->mScaler_arg.dest_ImgWidth;
				pMParm->extend_display.crop_bottom = pMParm->mScaler_arg.dest_ImgHeight;


		              pMParm->extend_display.codec_id = pMParm->mUnique_address;

               		pMParm->extend_display.one_field_only_interlace = pMParm->mVideoDeinterlaceScaleUpOneFieldFlag ? 1 : 0;
    				pMParm->extend_display.fmt = TCC_LCDC_IMG_FMT_YUYV;


				DBUG_FRM("Extenddisplay is LCD size %d %d ", pMParm->extend_display.Image_width, pMParm->extend_display.Image_height);
			}
			else
			{

				DestWidth = (float)pMParm->mDispWidth * ExLCD_fScale_w;
				DestHeight = (float)pMParm->mDispHeight * ExLCD_fScale_h;
				DestWidth = ((DestWidth + 3)>>2)<<2;
				DestHeight = ((DestHeight + 3)>>2)<<2;

 				memset(&ScaleInfo, 0x00, sizeof(ScaleInfo));
				ScaleInfo.src_Yaddr 		= pOverlayInfo->YaddrPhy;
				ScaleInfo.src_Uaddr 		= pOverlayInfo->UaddrPhy;
				ScaleInfo.src_Vaddr 		=  pOverlayInfo->VaddrPhy;
				ScaleInfo.responsetype		= SCALER_INTERRUPT;

				if(mColorFormat == OMX_COLOR_FormatYUV420SemiPlanar)
					ScaleInfo.src_fmt 		= SCALER_YUV420_inter;
				else if(mColorFormat == OMX_COLOR_FormatYUV422Planar)
					ScaleInfo.src_fmt 		= SCALER_YUV422_sp;
				else
					ScaleInfo.src_fmt 		= SCALER_YUV420_sp;

				ScaleInfo.src_ImgWidth		= SrcWidth;
				ScaleInfo.src_ImgHeight 	= SrcHeight;
				ScaleInfo.src_winLeft		= 0;
				ScaleInfo.src_winTop		= 0;
				ScaleInfo.src_winRight		= ScaleInfo.src_winLeft + ScaleInfo.src_ImgWidth;
				ScaleInfo.src_winBottom 	= ScaleInfo.src_winTop + ScaleInfo.src_ImgHeight;

				if((pMParm->mOverlayBuffer_Current % 2) == 0)
					ScaleInfo.dest_Yaddr = pMParm->mDualDisplayPmap.base;
				else
					ScaleInfo.dest_Yaddr = pMParm->mDualDisplayPmap.base + (pMParm->mDualDisplayPmap.size/2);

				ScaleInfo.dest_Uaddr		= NULL;
				ScaleInfo.dest_Vaddr		= NULL;
				ScaleInfo.dest_fmt			= SCALER_YUV422_sq0;
				ScaleInfo.dest_ImgWidth 	= DestWidth;
				ScaleInfo.dest_ImgHeight	= DestHeight;
				ScaleInfo.dest_winLeft		= 0;
				ScaleInfo.dest_winTop		= 0;
				ScaleInfo.dest_winRight 	= ScaleInfo.dest_winLeft + ScaleInfo.dest_ImgWidth;
				ScaleInfo.dest_winBottom	= ScaleInfo.dest_winTop + ScaleInfo.dest_ImgHeight;

				DBUG_FRM(" Scaler-SRC:: %d x %d, ~ %d x %d", ScaleInfo.src_ImgWidth, ScaleInfo.src_ImgHeight,
						 ScaleInfo.dest_ImgWidth, ScaleInfo.dest_ImgHeight);

				if (ioctl(pMParm->mM2m_fd, TCC_SCALER_IOCTRL, &ScaleInfo) < 0)	{
					ScaleInfo.responsetype = SCALER_POLLING;
					LOGE("Scaler Out Error!" );
				}

				if(ScaleInfo.responsetype == SCALER_INTERRUPT) {
					int ret;
					struct pollfd poll_event[1];

					memset(poll_event, 0, sizeof(poll_event));
					poll_event[0].fd = pMParm->mM2m_fd;
					poll_event[0].events = POLLIN;
					ret = poll((struct pollfd*)poll_event, 1, 400);

					if (ret < 0) {
						LOGE("m2m poll error\n");
					}else if (ret == 0) {
						LOGE("m2m poll timeout\n");
					}else if (ret > 0) {
						if (poll_event[0].revents & POLLERR) {
							LOGE("m2m poll POLLERR\n");
						}
					}
				}

				pMParm->extend_display.addr0 = (unsigned int)ScaleInfo.dest_Yaddr;
				pMParm->extend_display.addr1 = (unsigned int)ScaleInfo.dest_Uaddr;
				pMParm->extend_display.addr2 = (unsigned int)ScaleInfo.dest_Vaddr;
				pMParm->extend_display.codec_id = pMParm->mUnique_address;

				pMParm->extend_display.Frame_width = ScaleInfo.dest_ImgWidth;
				pMParm->extend_display.Frame_height = ScaleInfo.dest_ImgHeight;

				pMParm->extend_display.Image_width = ScaleInfo.dest_ImgWidth;
				pMParm->extend_display.Image_height = ScaleInfo.dest_ImgHeight;

				pMParm->extend_display.one_field_only_interlace = pMParm->mVideoDeinterlaceScaleUpOneFieldFlag ? 1 : 0;

				pMParm->extend_display.fmt = TCC_LCDC_IMG_FMT_YUYV;

				pMParm->extend_display.crop_left = 0;
				pMParm->extend_display.crop_top = 0;
				pMParm->extend_display.crop_right = ScaleInfo.dest_ImgWidth;
				pMParm->extend_display.crop_bottom = ScaleInfo.dest_ImgHeight;

				DBUG_FRM("Extenddisplay is scale size %d %d 0x%x", pMParm->extend_display.Image_width, pMParm->extend_display.Image_height, pMParm->extend_display.addr0);
			}

			if(pMParm->mHDMIOutput) {
				ret = ioctl(pMParm->mFb_fd, TCC_LCDC_HDMI_DISPLAY, &pMParm->extend_display);
			}
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE 
			else if(pMParm->mCompositeOutput) {
				ret = ioctl(pMParm->mComposite_fd, TCC_COMPOSITE_IOCTL_UPDATE, &pMParm->extend_display);
			}
			else if(pMParm->mComponentOutput) {
				ret = ioctl(pMParm->mComponent_fd, TCC_COMPONENT_IOCTL_UPDATE, &pMParm->extend_display);
			}

			if(ret < 0) {
				LOGE("Extend Display update %d ERROR[%d] HDMI:%d Composite:%d Component:%d", __LINE__, ret, pMParm->mHDMIOutput, pMParm->mCompositeOutput, pMParm->mComponentOutput);
			}
#else
            if(ret < 0) {
				LOGE("Extend Display update %d ERROR[%d] HDMI:%d", __LINE__, ret, pMParm->mHDMIOutput);
			}
#endif
		}
	}

	if(pMParm->mOntheflymode == 0 || bBufferchange)
	{
		if(pMParm->mParallel)
		{
			if(pMParm->mFirst_Frame)
				pMParm->mFirst_Frame = false;
		}

#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
		if(!pMParm->mVsyncMode)
#endif
		pMParm->mOverlayBuffer_Current = (pMParm->mOverlayBuffer_Current + 1) % pMParm->mOverlayBuffer_Cnt;

  		return true;
	}

	return false;
}

bool HwRenderer::get_private_info(TCC_PLATFORM_PRIVATE_PMEM_INFO *info, char* grhandle, bool isHWaddr, int fd_val, unsigned int width, unsigned int height)
{
	unsigned int target_addr;
	int stride, stride_c, frame_len = 0;

	//follow the source code (hardware/telechips/omx/omx_videodec_component/src/omx_videodec_component.cpp, get_private_addr() function)
	stride = (width + 15) & ~15;
	stride_c = (stride/2 + 15) & ~15;
	frame_len = height * (stride + stride_c);// + 100;
///////////////////////////////////////////////////////////////////

//    mColorFormat = OMX_COLOR_FormatYUV420Planar; //default

	if( isHWaddr && !pMParm->bUseMemInfo )
		return false;

	if( isHWaddr && pMParm->mTmem_fd < 0 ) {
		LOGE("/dev/tmem device is not opened.");
		return false;
	}

	if( isHWaddr ){
		target_addr = (unsigned int)pMParm->mTMapInfo + (fd_val - pMParm->mUmpReservedPmap.base) + frame_len;
		memcpy((void*)info, (void*)target_addr, sizeof(TCC_PLATFORM_PRIVATE_PMEM_INFO));

		if(0)
		{
			ALOGD("[get_private_info] fd_val : 0x%x, pMParm->mUmpReservedPmap.base : 0x%x, frame_len : %d", fd_val, pMParm->mUmpReservedPmap.base, frame_len);

			ALOGD("[get_private_info] info->width : %d", info->width);
			ALOGD("[get_private_info] info->height : %d", info->height);
			ALOGD("[get_private_info] info->format : %d", info->format);
			ALOGD("[get_private_info] info->offset : 0x%x, 0x%x, 0x%x", info->offset[0], info->offset[1], info->offset[2]);

			ALOGD("[get_private_info] info->optional_info(picture width) : %d", info->optional_info[0]);
			ALOGD("[get_private_info] info->optional_info(picture height) : %d", info->optional_info[1]);
			ALOGD("[get_private_info] info->optional_info(FrameWidth) : %d", info->optional_info[2]);
			ALOGD("[get_private_info] info->optional_info(FrameHeight) : %d", info->optional_info[3]);
			ALOGD("[get_private_info] info->optional_info(buffer_id) : %d", info->optional_info[4]);
			ALOGD("[get_private_info] info->optional_info(timeStamp) : %d", info->optional_info[5]);
			ALOGD("[get_private_info] info->optional_info(curTime) : %d", info->optional_info[6]);
			ALOGD("[get_private_info] info->optional_info(flags) : 0x%x", info->optional_info[7]);
			ALOGD("[get_private_info] info->optional_info(framerate) : %d", info->optional_info[8]);
			ALOGD("[get_private_info] info->optional_info(display out index for MVC) : %d", info->optional_info[9]);
			ALOGD("[get_private_info] info->optional_info(MVC Base addr0) : %d", info->optional_info[10]);
			ALOGD("[get_private_info] info->optional_info(MVC Base addr1) : %d", info->optional_info[11]);
			ALOGD("[get_private_info] info->optional_info(MVC Base addr2) : %d", info->optional_info[12]);
			ALOGD("[get_private_info] info->optional_info(Vsync enable field) : %d", info->optional_info[13]);
			ALOGD("[get_private_info] info->optional_info(display out index of VPU) : %d", info->optional_info[14]);
			ALOGD("[get_private_info] info->optional_info(vpu instance-index) : %d", info->optional_info[15]);
			ALOGD("[get_private_info] info->optional_info(not reserved) : %d", info->optional_info[16]);
			ALOGD("[get_private_info] info->optional_info(not reserved) : %d", info->optional_info[17]);
			ALOGD("[get_private_info] info->optional_info(not reserved) : %d", info->optional_info[18]);
			ALOGD("[get_private_info] info->optional_info(not reserved) : %d", info->optional_info[19]);

			ALOGD("[get_private_info] info->name : %s", info->name);
			ALOGD("[get_private_info] info->unique_addr : %d", info->unique_addr);
			ALOGD("[get_private_info] info->copied : %d", info->copied);
		}
	}
	else{
		target_addr = fd_val;
		memcpy((void*)info, (void*)target_addr, sizeof(TCC_PLATFORM_PRIVATE_PMEM_INFO));
	}

	DBUG("copy ? private data %p %s %dx%d, %d, 0x%x-0x%x-0x%x", (void*)target_addr, info->name, info->width, info->height, info->format,
								info->offset[0], info->offset[1], info->offset[2]);

	if( width == info->width && height == info->height && !strncmp((char*)info->name, "video", 5) )
	{
		mColorFormat = info->format;
		return true;
	}
	memset((void*)info, 0x00, sizeof(TCC_PLATFORM_PRIVATE_PMEM_INFO));

	return false;
}

void HwRenderer::setVsync()
{
	char value[PROPERTY_VALUE_MAX];
	int result;

	#ifdef TCC_SKIP_UI_UPDATE_DURING_VIDEO
	int isTCCSurface = false;
	int isTCCMultiDecoding = false;

	property_get("tcc.video.surface.support", value, "0");
	isTCCSurface = atoi(value);

	isTCCMultiDecoding = (mNumberOfExternalVideo > 1) ? 1:0;

	mCheckEnabledTCCSurface = 0;

	if (isTCCSurface == 1 && !isTCCMultiDecoding) {
		property_set("tcc.video.surface.enable", "1");
		mCheckEnabledTCCSurface = 1;
	}
	#endif

#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
	int isVsyncSupport;
	int isExDisplay, isExtDisplaySelected;
	bool vsync = true;

	property_get("tcc.video.vsync.support", value, "0");
	isVsyncSupport = atoi(value);

//	getfromsysfs("/sys/class/tcc_dispman/tcc_dispman/persist_output_mode", value);
//	isExDisplay = atoi(value);
	isExDisplay = 0;	// LCD only

	property_get("tcc.sys.output_mode_detected", value, "");
	isExtDisplaySelected = atoi(value);
	if (isVsyncSupport == 0) {
		vsync = false;
	}

	if (isExDisplay == 0) {
#ifdef USE_LCD_VIDEO_VSYNC
		if(!pMParm->mOntheflymode){
			vsync = false;
		}
#else
		vsync = false;
#endif
	}

	if (isExtDisplaySelected == 0) {
#ifdef USE_LCD_VIDEO_VSYNC
		if(!pMParm->mOntheflymode){
			vsync = false;
		}
#else
		vsync = false;
#endif
	}

	LOGI("mCheckSync  %d ",mCheckSync);
	if ( mCheckSync == 0) {
		vsync = false;
	}

	property_get("tcc.media.wfd.sink.run", value, "0");
	int isWFDSupport = atoi(value);

	property_get("tcc.wfd.vsync.disable", value, "1");
	int isWFDvsync = atoi(value);

	if(isWFDSupport == 1 && isWFDvsync == 1) {
		LOGE("vsync disable");
		vsync = false;
	}

	property_set("tcc.sys.output_mode_plugout", "0");

	if (vsync) {
		property_set("tcc.video.vsync.enable", "1");

		struct tcc_lcdc_image_update init_display;
		memset(&init_display, 0x00, sizeof(init_display));
		init_display.max_buffer = VSYNC_OVERLAY_COUNT;
		init_display.ex_output = isExtDisplaySelected;

#ifdef USE_LCD_VIDEO_VSYNC
		if(isExtDisplaySelected || isExDisplay==OUTPUT_HDMI)
		{
			unsigned int isValid = 0;
			isValid = property_get("tcc.check.hdmi_hpd", value, "");
			if(isValid && atoi(value)==0)
			{
				LOGI("### LCD VSYNC mHDMIOutput %d mOutputMode %d ",pMParm->mHDMIOutput, pMParm->mOutputMode);
				init_display.ex_output = 0;
			}
		}
#endif

		unsigned int lastframe_onoff = 0;

		if(pMParm->mOutputMode==0)
			lastframe_onoff = 1;

		_closeOverlay(lastframe_onoff);

		do {
			result = ioctl(pMParm->mFb_fd, TCC_LCDC_VIDEO_START_VSYNC, &init_display);
			if (result < 0) {
				LOGE("### error fb ioctl : TCC_LCDC_VIDEO_START_VSYNC") ;
				usleep(5000);
			}
		} while (result < 0);
		LOGI("### start vsync with output : %d ",init_display.ex_output);
	}
	else {
		property_set("tcc.video.vsync.enable", "0");
	}
#endif

#ifdef USE_LCD_OTF_SCALING
	if( pMParm->mFb_fd != -1 && !isExtDisplaySelected && pMParm->mOntheflymode ) {
		result = ioctl( pMParm->mFb_fd, TCC_LCDC_DISPLAY_START, 0 );
		if(result < 0)
			LOGE("TCC_LCDC_DISPLAY_START fail");
	}
#endif

}

void HwRenderer::clearVsync(bool able_use_vsync, bool prev_onthefly_mode, bool use_lastframe)
{
	int result;

	pMParm->mFrame_cnt = 0;
#ifdef USE_LCD_OTF_SCALING
	if( pMParm->mFb_fd != -1) {
		result = ioctl( pMParm->mFb_fd, TCC_LCDC_DISPLAY_END, 0 );
		if(result < 0)
			LOGE("TCC_LCDC_DISPLAY_END fail");
	}
#endif

#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT

    if( pMParm->mFb_fd != -1) {
    	result = ioctl( pMParm->mFb_fd, TCC_LCDC_VIDEO_END_VSYNC, 0 );
    	if(result < 0)
    		LOGE("TCC_LCDC_VIDEO_END_VSYNC fail");

        if( pMParm->mOutputMode == 1 && pMParm->mVsyncMode == false && prev_onthefly_mode == true )
        {
            struct tcc_lcdc_image_update lcdc_display;
            memset(&lcdc_display, 0x00, sizeof(lcdc_display));
            lcdc_display.Lcdc_layer = LCDC_VIDEO_CHANNEL;
            lcdc_display.enable = 0;

            if(pMParm->mHDMIOutput) {
                if(ioctl(pMParm->mFb_fd, TCC_LCDC_HDMI_DISPLAY, &lcdc_display) < 0) {
                    LOGE("TCC_LCDC_HDMI_DISPLAY Error!");
                }
            }
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE 
            else if(pMParm->mCompositeOutput) {
                if(ioctl(pMParm->mComposite_fd, TCC_COMPOSITE_IOCTL_UPDATE, &lcdc_display) < 0) {
                    LOGE("TCC_COMPOSITE_IOCTL_UPDATE Error!");
                }
            }
            else if(pMParm->mComponentOutput) {
                if(ioctl(pMParm->mComponent_fd, TCC_COMPONENT_IOCTL_UPDATE, &lcdc_display) < 0) {
                    LOGE("TCC_COMPONENT_IOCTL_UPDATE Error!");
                }
            }
#endif  
        }


    }

	// MtoM scaler has already been opened. Let's close
	if(pMParm->mM2m_fd > 0 ){
		close(pMParm->mM2m_fd);
        LOGE("%d Scaler is closed.", __LINE__);
		pMParm->mM2m_fd = -1;
	}

    if( pMParm->mFb_fd != -1) {
		int res_changed = 0;

		if((pMParm->mOutputMode == 0) && use_lastframe){
			if( ((prev_onthefly_mode == true) ||
				((prev_onthefly_mode == false) && (pMParm->mOntheflymode)) )

			)
			{
				ioctl( pMParm->mTmem_fd, TCC_LCDC_HDMI_LASTFRAME, &res_changed );
			}
		}
		else{
				if(use_lastframe){
					ioctl( pMParm->mTmem_fd, TCC_LCDC_HDMI_LASTFRAME, &res_changed );
				}
		}
    }

    #if !defined(_TCC8800_)
	if(pMParm->mViqe_fd != -1)
	{
		int iRet;
		if(pMParm->mVIQE_firstfrm == 0)
			ioctl(pMParm->mViqe_fd, IOCTL_VIQE_DEINITIALIZE);

		if(close(pMParm->mViqe_fd) < 0)
			LOGE("Error: close(pMParm->mViqe_fd)");

		memset(&pMParm->mViqe_arg, 0x00, sizeof(VIQE_DI_TYPE));
		pMParm->mVIQE_onoff = false;
		pMParm->mVIQE_onthefly = false;
		pMParm->mVIQE_DI_use = 0;
		pMParm->mVIQE_firstfrm = 1;
	    	pMParm->mViqe_fd = -1;
	}
    #endif

	pMParm->mVSyncSelected = false;

	if(!able_use_vsync){
		property_set("tcc.video.vsync.enable", "0");
		LOGI("### stop vsync");
	}

	if(pMParm->mChangedHDMIResolution )
	{
		char value[PROPERTY_VALUE_MAX];
		memset(value, 0x00, PROPERTY_VALUE_MAX);

		if(pMParm->mMVCmode )
		{
			ioctl(pMParm->mFb_fd, TCC_LCDC_VIDEO_SET_MVC_STATUS, 0);
			//usleep(10*1000);
			property_set("tcc.output.hdmi.structure.3d","0");
			property_set("tcc.output.hdmi.video.format","0");
			property_set("tcc.output.hdmi_3d_format","0");
			property_set("tcc.video.mvc.mode","0");

			property_set("tcc.video.mvc.playing","0");

			pMParm->mMVCmode = false;
		}

		//intToStr(pMParm->mOriginalHDMIResolution,0,value);
		//property_set("persist.sys.hdmi_resolution", value);

		//LOGI("restore hdmi resolution to %d",pMParm->mOriginalHDMIResolution );

		pMParm->mChangedHDMIResolution = false;
		property_set("tcc.video.hdmi_resolution", "999");
		/*
		int loopCount = 0;
		while(loopCount < 10){
			usleep(100000);
			loopCount++;
		}
		*/
	}

	property_set("tcc.sys.output_mode_plugout", "0");
#endif

	property_set("tcc.video.surface.enable", "0");
}

void HwRenderer::setting_mode()
{
	unsigned int	uiOutputMode = 0;
	unsigned int screen_size;
	char value[PROPERTY_VALUE_MAX];
	int result = 0;

	sprintf(value, "%u", pMParm->mUnique_address);
	property_set("tcc.video.hwr.id", value);

//	getfromsysfs("/sys/class/tcc_dispman/tcc_dispman/persist_output_mode", value);
//	uiOutputMode = atoi(value);
	uiOutputMode = OUTPUT_NONE;	// LCD only

#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
	// check vsync mode by system property.
	property_get("tcc.video.vsync.enable", value, "0");
	pMParm->mVsyncMode = atoi(value);

	LOGI("tcc.video.vsync.enable : %d", pMParm->mVsyncMode);

	if(pMParm->mVsyncMode)
	{
		result = ioctl(pMParm->mFb_fd, TCC_LCDC_VIDEO_SKIP_FRAME_END, 0);
		if(result < 0)
			LOGE("### error fb ioctl : TCC_LCDC_VIDEO_SKIP_FRAME_END") ;
	}
#endif

#ifdef TCC_VIDEO_DEINTERLACE_SUPPORT
	// check deinterlace mode by system property.
	property_get("tcc.video.deinterlace.support", value, "0");
	pMParm->mVideoDeinterlaceSupport = atoi(value);
	LOGI("tcc.video.deinterlace.support : %d", pMParm->mVideoDeinterlaceSupport);

	property_get("tcc.video.deinterlace.force", value, NULL);
	if (strcmp(value, "1") == 0)
	{
		LOGD("Enable video deinterlaced mode by force");
		pMParm->mVideoDeinterlaceForce = true;
	}
#endif

	if( pMParm->mVsyncMode )
	{
#if 0// !defined(_TCC8800_)
		screen_size = (HDMI_DEC_MAX_WIDTH/2) * (HDMI_DEC_MAX_HEIGHT/2) * 2;
#else
		screen_size = (HDMI_DEC_MAX_WIDTH) * (HDMI_DEC_MAX_HEIGHT) * 2;
#endif
		pMParm->mOverlayBuffer_Cnt = VSYNC_OVERLAY_COUNT;
	}
	else
	{
		screen_size = (HDMI_DEC_MAX_WIDTH) * (HDMI_DEC_MAX_HEIGHT) * 2;
		pMParm->mOverlayBuffer_Cnt = (pMParm->mOverlayPmap.size / screen_size);
	}

	pMParm->mOverlayBuffer_Current = 0;

	LOGD("Overlay buffer count : %d, current count : %d", pMParm->mOverlayBuffer_Cnt, pMParm->mOverlayBuffer_Current);

	for(int buffer_cnt=0; buffer_cnt < pMParm->mOverlayBuffer_Cnt; buffer_cnt++)
	{
		pMParm->mOverlayBuffer_PhyAddr[buffer_cnt] = ((unsigned int)pMParm->mOverlayPmap.base + (screen_size * buffer_cnt));
		DBUG("Overlay :: %d/%d = 0x%x  \n", buffer_cnt, pMParm->mOverlayBuffer_Cnt, (unsigned int)(pMParm->mOverlayBuffer_PhyAddr[buffer_cnt]));
	}
}

#define NATIVE_DBUG(msg...)	if (0) { LOGI( "NativeFramerate :: " msg); }
enum {
	//NATIVE_MODE_NONE = 0,
	//NATIVE_MODE_ALL,
	NATIVE_MODE_P_23 =0,
	NATIVE_MODE_P_24,
	NATIVE_MODE_P_50,
	NATIVE_MODE_P_60,
	NATIVE_MODE_I_50,
	NATIVE_MODE_I_60,
	NATIVE_MODE_MAX
};

int HwRenderer::digits10(unsigned int num)
{
	int i = 0;

	do {
		num /= 10;
		i++;
	} while (num != 0);

	return i;
}

int HwRenderer::intToStr(int num, int sign, char *buffer)
{
	int pos;
	unsigned int a;
	int neg;

	if (sign && num < 0) {
		a = -num;
		neg = 1;
	}
	else {
		a = num;
		neg = 0;
	}

	pos = digits10(a);

	if (neg)
		pos++;

	buffer[pos] = '\0';
	pos--;

	do {
		buffer[pos] = (a % 10) + '0';
		pos--;
		a /= 10;
	} while (a != 0);

	if (neg)
		buffer[pos] = '-';

	return 0;
}

void HwRenderer::Native_FramerateMode(TCC_PLATFORM_PRIVATE_PMEM_INFO* pPrivate_data, int inWidth, int inHeight, int outWidth, int outHeight)
{
	bool isResolutionChanged = false;
	char value[PROPERTY_VALUE_MAX];
	int videoDeinterlaceFlag=false;
	int selectedMenuValule = 0;

	//check if STB or not
	property_get("tcc.solution.mbox", value, "0");
	if(!atoi(value))
		return;

	//check if HDMI or not
//	getfromsysfs("/sys/class/tcc_dispman/tcc_dispman/persist_output_mode", value);
//	if(atoi(value) != OUTPUT_HDMI)
		return;

	//check if menu enabled or not
	property_get("persist.sys.native_mode_select", value, "0");
	if(!atoi(value))
		return;

	//check if local play or not
	property_get("tcc.video.lplayback.mode", value, "0");
	if(!atoi(value))
		return;

	//check if preview or not
	property_get("tcc.solution.preview", value, "0");
	if(atoi(value))
		return;

	property_get("tcc.video.mvc.mode", value, "0");
	if(atoi(value))
		return;

	if(pMParm->mFrame_cnt == 0 )
	{
		#if defined(TCC_VIDEO_DISPLAY_BY_VSYNC_INT)
		if( pPrivate_data->optional_info[7] & 0x40000000 )//OMX_BUFFERFLAG_INTERLACED_FRAME
		{
			videoDeinterlaceFlag = true;
		}

		if(pPrivate_data->optional_info[8] > 10000)
			pMParm->mFrame_rate =  pPrivate_data->optional_info[8]/1000;
		else
			pMParm->mFrame_rate =  pPrivate_data->optional_info[8];

		#endif

		property_get("persist.sys.hdmi_resolution", value, "0");
		pMParm->mOriginalHDMIResolution = atoi(value);

		property_get("persist.sys.native_mode", value, "0");
		selectedMenuValule = atoi(value);

		NATIVE_DBUG("Check resolution %d Scan type is Interlace %d and Framerate %d",pMParm->mOriginalHDMIResolution,videoDeinterlaceFlag,pMParm->mFrame_rate);

		bool selectedItem[NATIVE_MODE_MAX];

		for(int i=0 ; i < NATIVE_MODE_MAX ; i++){
			selectedItem[i] =false;
			if( (selectedMenuValule & (1<<i)) !=0){
				selectedItem[i] =true;
				NATIVE_DBUG("NATIVE_MODE selected menu %d",i);
			}
		}

		if(videoDeinterlaceFlag)
		{
			switch(pMParm->mFrame_rate)
			{
				case 25:
					if(( selectedItem[NATIVE_MODE_I_50]) && pMParm->mOriginalHDMIResolution !=3 ){
						property_set("tcc.video.hdmi_resolution", "3");
						isResolutionChanged = true;
						NATIVE_DBUG("Set 1080i 50Hz");
					}
					break;

				case 29:
				case 30:
					if(( selectedItem[NATIVE_MODE_I_60]) && pMParm->mOriginalHDMIResolution !=2 ){
						property_set("tcc.video.hdmi_resolution", "2");
						isResolutionChanged = true;
						NATIVE_DBUG("Set 1080i 60Hz");
					}
					break;

				default:
					break;
			}

		}
		else{
			switch(pMParm->mFrame_rate)
			{
				case 23:
					if(( selectedItem[NATIVE_MODE_P_23]) && pMParm->mOriginalHDMIResolution !=13 ){
						property_set("tcc.video.hdmi_resolution", "13");
						isResolutionChanged = true;
						NATIVE_DBUG("Set 1080p 23Hz");
					}
					break;

				case 24:
					if(( selectedItem[NATIVE_MODE_P_24]) && pMParm->mOriginalHDMIResolution !=14 ){
						property_set("tcc.video.hdmi_resolution", "14");
						isResolutionChanged = true;
						NATIVE_DBUG("Set 1080p 24Hz");
					}
					break;

				case 50:
					if(( selectedItem[NATIVE_MODE_P_50]) && pMParm->mOriginalHDMIResolution !=1 ){
						property_set("tcc.video.hdmi_resolution", "1");
						isResolutionChanged = true;
						NATIVE_DBUG("Set 1080p 50Hz");
					}
					break;

				case 30:
				case 59:
				case 60:
				case 119:
				case 120:
					if((selectedItem[NATIVE_MODE_P_60]) && pMParm->mOriginalHDMIResolution!=125 && pMParm->mOriginalHDMIResolution!=0 ){
						property_set("tcc.video.hdmi_resolution", "0");
						isResolutionChanged = true;
						NATIVE_DBUG("Set 1080p 60Hz");
					}
					break;
				default:
					break;
			}

		}

		pMParm->mChangedHDMIResolution = isResolutionChanged;

		if(isResolutionChanged)
		{
			int loopCount = 0;

			while(loopCount < 10){
				usleep(100000);
				loopCount++;
			}
			LOGI("set hdmi resolution to rate %d form %d ",pMParm->mFrame_rate,atoi(value));
		}
	}
}

void HwRenderer::Set_MVC_Mode(TCC_PLATFORM_PRIVATE_PMEM_INFO* pPrivate_data, int inWidth, int inHeight, int outWidth, int outHeight)
{
	bool isResolutionChanged = false;
	char value[PROPERTY_VALUE_MAX];
	int videoMVCflag=false;

	#if defined(HDMI_V1_4)
	#else
		return;
	#endif

	//check if STB or not
	property_get("tcc.solution.mbox", value, "0");
	if(!atoi(value))
		return;

	//check if HDMI or not
	property_get("persist.sys.output_mode", value, "");
	if(atoi(value) != OUTPUT_HDMI)
		return;

	//check if preview or not
	property_get("tcc.solution.preview", value, "0");
	if(atoi(value))
		return;

	property_get("tcc.video.mvc.support", value, "0");
	if(!atoi(value))
		return;

	if(pMParm->mMVCmode == false)
	{

		#if defined(TCC_VIDEO_DISPLAY_BY_VSYNC_INT)
		if(pPrivate_data->optional_info[9])
		{
			ioctl(pMParm->mFb_fd, TCC_LCDC_VIDEO_SET_MVC_STATUS, 1);
			//usleep(10*1000);
			property_set("tcc.video.mvc.mode","1");
			videoMVCflag = true;
		}
		#endif

		property_get("persist.sys.hdmi_resolution", value, "0");
		pMParm->mOriginalHDMIResolution = atoi(value);

		//LOGI("Check resolution %d videoMVCflag %d inWidth %d",pMParm->mOriginalHDMIResolution,videoMVCflag,inWidth);

		if(videoMVCflag)
		{
			property_set("tcc.video.mvc.playing","1");

			switch(inWidth)
			{
				case 1280:
					if(pMParm->mOriginalHDMIResolution != 25){
						property_set("tcc.output.hdmi.structure.3d","0");
						property_set("tcc.output.hdmi.video.format","2");
						property_set("tcc.output.hdmi_3d_format","1");
						property_set("tcc.video.hdmi_resolution", "25");
						isResolutionChanged = true;
					}
					break;

				case 1920:
					if(pMParm->mOriginalHDMIResolution != 26){
						property_set("tcc.output.hdmi.structure.3d","0");
						property_set("tcc.output.hdmi.video.format","2");
						property_set("tcc.output.hdmi_3d_format","1");
						property_set("tcc.video.hdmi_resolution", "26");
						isResolutionChanged = true;
					}
					break;

				default:
					break;
			}
		}

		pMParm->mMVCmode = videoMVCflag;
		pMParm->mChangedHDMIResolution = isResolutionChanged;

		if(isResolutionChanged)
		{
			int loopCount = 0;

			while(loopCount < 10){
				usleep(100000);
				loopCount++;
			}
			LOGI("set hdmi resolution to mDecodedWidth %d from %s",mDecodedWidth,value);
		}
	}
}

bool HwRenderer::check_valid_hw_availble(int fd_val)
{
	return false;
}

void HwRenderer::setNumberOfExternalVideo( int numOfExternalVideo, bool isPrepare)
{
	#ifdef TCC_SKIP_UI_UPDATE_DURING_VIDEO
	int isTCCSurface = false;
	int isTCCMultiDecoding = false;
	char value[PROPERTY_VALUE_MAX];

	mNumberOfExternalVideo = numOfExternalVideo;

	property_get("tcc.video.surface.support", value, "0");
	isTCCSurface = atoi(value);

	if(!isTCCSurface) return;

	isTCCMultiDecoding = (mNumberOfExternalVideo > 1) ? 1:0;

	if (!isTCCMultiDecoding) {
		if(mCheckEnabledTCCSurface==0) {//not yet enabled
			property_set("tcc.video.surface.enable", "1");
			mCheckEnabledTCCSurface = 1;

			LOGI("Single Video, Let's be enable TCC surface property ");
		}
	}
	else{
		if(mCheckEnabledTCCSurface==1){ //already enabled
			property_set("tcc.video.surface.enable", "0");
			mCheckEnabledTCCSurface = 0;
			LOGI("Multi(%d) Video, Let's be disable TCC surface property ",mNumberOfExternalVideo);
		}

	}
	#endif

}

void HwRenderer::setFBChannel( int numOfLayers, bool isReset)
{
	if(isReset || !mBox || !( pMParm->mVsyncMode ))
	{
		mVideoCount=0;
		mIsFBChannelOn = true;
		return;
	}

	if(numOfLayers ==1 && pMParm->mFb_fd!=-1)
	{
		int interval=0;

		mVideoCount++;
		if(pMParm->mFrame_rate > 19 && pMParm->mFrame_rate < 121)
		interval = 1000/pMParm->mFrame_rate;
		else
			interval = 30;

		if( (mVideoCount*interval) > 6000){
			int ret;
			unsigned int onOff = 0;

			char value[PROPERTY_VALUE_MAX];
			property_get("tcc.video.fbchannel.enable", value, "0");
			if(!atoi(value))
				return;

			if(mIsFBChannelOn){
				mIsFBChannelOn = false;
				ret = ioctl( pMParm->mFb_fd, TCC_LCDC_FBCHANNEL_ONOFF, &onOff);
				if(ret < 0){
					LOGE("Error: pMParm->mFb_fd(TCC_LCDC_FBCHANNEL_ONOFF)");
				}
			}
		}
	}

	return;
}

int HwRenderer::render(int fd_0, int inWidth, int inHeight, int outLeft, int outTop, int outRight, int outBottom, unsigned int transform)
{
	int hdmi_detect = 0;
	int LCD_display = 0;
	int outSx, outSy, outWidth, outHeight;
	int OutputModeCheck = 0;
	int OutputModeDetected = 0;
	int OutputModeChanged = 0;

	char value[PROPERTY_VALUE_MAX];

	if(0)
	{
		ALOGD("[render] fd_0 : 0x%x", fd_0);
		ALOGD("[render] inWidth : %d", inWidth);
		ALOGD("[render] inHeight : %d", inHeight);
		ALOGD("[render] outLeft : %d", outLeft);
		ALOGD("[render] outTop : %d", outTop);
		ALOGD("[render] outRight : %d", outRight);
		ALOGD("[render] outRight : %d", outBottom);
		ALOGD("[render] transform : %d", transform);
	}

	if( pMParm->bCropChecked == false )
		return 0;

// exceptional process for DXB.
	if( !transform && outLeft >= 0 && outTop >= 0 )
	{
		if(outRight > pMParm->mLcd_width){
			outRight = pMParm->mLcd_width;

			if(outBottom > pMParm->mLcd_height)
				outBottom = pMParm->mLcd_height;
		}
	}

	if(outLeft <= 0){
		outSx = 0;
		outWidth = (pMParm->mLcd_width < outRight) ? pMParm->mLcd_width : outRight;
	}
	else{
		outSx = outLeft;
		outWidth = (pMParm->mLcd_width < outRight) ? (pMParm->mLcd_width - outLeft) : (outRight - outLeft);
	}
	outSx = (outSx>>1)<<1;
	if(outTop <= 0){
		outSy = 0;
		outHeight= (pMParm->mLcd_height < outBottom) ? pMParm->mLcd_height : outBottom;
	}
	else{
		outSy = outTop;
		outHeight= (pMParm->mLcd_height < outBottom) ? (pMParm->mLcd_height - outTop) : (outBottom - outTop);
	}
	outSy = (outSy>>1)<<1;
	outHeight = (outHeight>>2)<<2;

	if (property_get("ro.sf.hwrotation", value, NULL) > 0) {
		//displayOrientation
		if(atoi(value) == 180)
		{
			int isExDisplay, isExtDisplaySelected;

//			getfromsysfs("/sys/class/tcc_dispman/tcc_dispman/persist_output_mode", value);
//			isExDisplay = atoi(value);
			isExDisplay = 0;	// LCD only
			property_get("tcc.sys.output_mode_detected", value, "");
			isExtDisplaySelected = atoi(value);

			#ifdef USE_LCD_VIDEO_VSYNC
			if(isExDisplay && isExtDisplaySelected)
			{
				unsigned int isValid = 0;
				isValid = property_get("tcc.check.hdmi_hpd", value, "");
				if(isValid && atoi(value)==0)
				{
					isExDisplay=0;
					isExtDisplaySelected=0;
				}
			}
			#endif

			if(isExDisplay && isExtDisplaySelected)
			{
				int org_end_x, org_end_y;

				org_end_x = outSx + outWidth;
				org_end_y = outSy + outHeight;

				outSx = outSy = 0;
				if(org_end_x <= pMParm->mLcd_width)
					outSx = pMParm->mLcd_width - org_end_x;
				if(org_end_y <= pMParm->mLcd_height)
					outSy = pMParm->mLcd_height - org_end_y;
			}
		}
	}

	if( !pMParm->bExtract_data && !pMParm->bHWaddr ) {
		ALOGE("We can't render this frame due to the reason(%d/%d)", pMParm->bExtract_data, pMParm->bHWaddr);
		return 0;
	}

#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
	if( pMParm->bExtract_data == true ){
		if(mBufferId == pMParm->private_data.optional_info[4])
		{
			//mBufferId == overlayInfo.buffer_id means same video frame comes again, skipping of it may be good

			//LOGI("Old frame buffer_id %d previos bufId %d mNoSkipBufferId %d :: %dx%d -> %dx%d", pMParm->private_data.optional_info[4], mBufferId, mNoSkipBufferId,
			//		mOutputWidth, mOutputHeight, outWidth, outHeight);

			//mid case : HDMI , vidoe pause -> home -> come back to gallery, although video has same buffer id video has to be displayed.
			if((mOutputWidth != outWidth && mOutputHeight != outHeight) || pMParm->mCropRgn_changed){
				if(!pMParm->bHWaddr){
					//Youtube - flash playback :: broken image is displayed whenever changing resolution.
					DBUG("Error: normal same buffer id(%d), addr 0x%x, res %d x %d , just return(skip) due to no hwaddr", pMParm->private_data.optional_info[4], pMParm->private_data.offset[0], pMParm->private_data.width, pMParm->private_data.height);
					return 0;
				}

				mNoSkipBufferId = mBufferId;

				// in case video time > auido time, it takes some time, i.e the difference  between video and audio time to output this frame.
				// That cause display delay , so this frame make display directly
				if(pMParm->private_data.optional_info[5] > pMParm->private_data.optional_info[6] + 60)
					pMParm->private_data.optional_info[7] |= 0x00000002;
			}
			else{
				//normal same buffer id , just return(skip);
				DBUG("Error: normal same buffer id(%d), addr 0x%x, res %d x %d , just return(skip)", pMParm->private_data.optional_info[4], pMParm->private_data.offset[0], pMParm->private_data.width, pMParm->private_data.height);
				return 0;
			}
		}
	}
#endif

	if(pMParm->mTransform != transform)
	{
		if(pMParm->mOverlay_fd != -1){
			unsigned int lastframe_onoff = 0;

			if(pMParm->mOutputMode==0)
			lastframe_onoff = 1;

			_closeOverlay(lastframe_onoff);

			LOGD("Let's clearVsync. The Rotation is detected");
			clearVsync(pMParm->bExtract_data, pMParm->mOntheflymode, true);
		}

		if(pMParm->mRt_fd > 0 ){
			close(pMParm->mRt_fd);
			LOGE("%d graphic is closed.", __LINE__);
			pMParm->mRt_fd = -1;
		}

		pMParm->mTransform = transform;
		return 0;
	}

	inHeight = (inHeight>>1)<<1;
	if(pMParm->bSupport_lastframe == false && mNoSkipBufferId == -1)
	{
		//This is for re-usage of surface view without destorying it.
		if((mInputWidth != inWidth || mInputHeight != inHeight)
			|| (mOutputWidth != outWidth || mOutputHeight != outHeight))
		{
			LOGD("Let's clearVsync. The size of Input or Output is changed");
			clearVsync(pMParm->bExtract_data, pMParm->mOntheflymode, true);
		}
	}

	mInputWidth = mDecodedWidth   = inWidth;
	mInputHeight = mDecodedHeight  = inHeight;

	mOutputWidth	= outWidth;
	mOutputHeight	= outHeight;

	if(mDecodedWidth == 1920 && mDecodedHeight > 1080){ //HDMI :: to prevent scaling in case of 1920x1088 contents
		mDecodedHeight = 1080;
	}

	pMParm->mScaler_arg.interlaced = 0;

#ifdef DEBUG_TIME_LOG
	clock_t clk_cur;

	clk_cur = clock();
	dec_time[time_cnt] = (clk_cur-clk_prev)*1000/CLOCKS_PER_SEC;
	clk_prev = clk_cur;
	if(time_cnt != 0 && time_cnt % 29 == 0)
	{
		LOGD("Disp	ms : %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d",
			 dec_time[0], dec_time[1], dec_time[2], dec_time[3], dec_time[4], dec_time[5], dec_time[6], dec_time[7], dec_time[8], dec_time[9],
			 dec_time[10], dec_time[11], dec_time[12], dec_time[13], dec_time[14], dec_time[15], dec_time[16], dec_time[17], dec_time[18], dec_time[19],
			 dec_time[20], dec_time[21], dec_time[22], dec_time[23], dec_time[24], dec_time[25], dec_time[26], dec_time[27], dec_time[28], dec_time[29]);
		time_cnt = 0;
	}
	else{
		time_cnt++;
	}
#endif

	if(pMParm->mDisplayOutputSTB)
	{
		unsigned int uiOutputMode = 0;
		char value[PROPERTY_VALUE_MAX];
		pMParm->mHDMIOutput = 0;
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE 
        pMParm->mCompositeOutput = pMParm->mComponentOutput = 0;
#endif

//		getfromsysfs("/sys/class/tcc_dispman/tcc_dispman/persist_output_mode", value);
//		uiOutputMode = atoi(value);
		uiOutputMode = 0;

		switch(uiOutputMode)
		{
			case OUTPUT_NONE:
			case OUTPUT_HDMI:
				pMParm->mHDMIOutput = true;
				pMParm->mOutputModeIndex = 0;
				break;
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE 
			case OUTPUT_COMPOSITE:
				pMParm->mCompositeOutput = true;
				pMParm->mOutputModeIndex = 1;
				break;
			case OUTPUT_COMPONENT:
				pMParm->mComponentOutput = true;
				pMParm->mOutputModeIndex = 2;
				break;
#endif  
		}

		/* check auto-detection and dual output mode
		 * persist.sys.display.mode :
		 * 0 - normal
		 * 1 - auto-detection(HDMI<->CVBS)
		 * 2 - dual output(HDMI/CVBS<->CVBS)
		 * 3 - dual output(HDMI/CVBS<->CVBS/Component) */
		 if(property_get("persist.sys.display.mode", value, "") > 0)
		 {
		 	if(atoi(value) != 0)
			{
				OutputModeCheck = 1;

				property_get("tcc.sys.output_mode_detected", value, "");
				OutputModeDetected = atoi(value);

				property_get("tcc.sys.output_mode_plugout", value, "");
				if(atoi(value) == 1)
				{
					OutputModeChanged = 1;
					property_set("tcc.sys.output_mode_plugout", "0");
				}
			}
		}
	}

	{
		output_overlay_param_t overlayInfo;
		bool use_Gbuffer = false;

		if(pMParm->bHWaddr){
			if( !pMParm->bExtract_data
				|| ( pMParm->bExtract_data && pMParm->private_data.offset[0] == NULL )
			)
			{
				use_Gbuffer = true;
			}

			DBUG("use_GBuffer(%d). %d/%d/%d-%d", use_Gbuffer, pMParm->bExtract_data, pMParm->private_data.copied, mColorFormat, mGColorFormat);
		}

		if( use_Gbuffer )
		{
			unsigned int stride, stride_c;
			unsigned int framesize_Y, framesize_C;
			unsigned int frame_w, frame_h;

			if(pMParm->bExtract_data)
			{
				frame_w = pMParm->private_data.width;
				frame_h = pMParm->private_data.height;
			}
			else
			{
				frame_w = mDecodedWidth;
				frame_h = mDecodedHeight;
			}

			stride = ALIGNED_BUFF(frame_w, 16);
			framesize_Y = ALIGNED_BUFF(stride * frame_h, 64);

			stride_c = ALIGNED_BUFF(stride/2, 16);
			framesize_C = ALIGNED_BUFF(stride_c * frame_h/2, 64);

			overlayInfo.YaddrPhy 	= fd_0;

			mColorFormat = mGColorFormat;
			if( mColorFormat == OMX_COLOR_FormatYUV420SemiPlanar )
			{
				overlayInfo.UaddrPhy 	= overlayInfo.YaddrPhy + framesize_Y;
				overlayInfo.VaddrPhy 	= overlayInfo.UaddrPhy + framesize_C;
			}
			else
			{
				overlayInfo.VaddrPhy 	= overlayInfo.YaddrPhy + framesize_Y;
				overlayInfo.UaddrPhy 	= overlayInfo.VaddrPhy + framesize_C;
			}
		}
		else
		{
				overlayInfo.YaddrPhy 	= pMParm->private_data.offset[0];
				overlayInfo.UaddrPhy 	= pMParm->private_data.offset[1];
				overlayInfo.VaddrPhy 	= pMParm->private_data.offset[2];
		}
#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
		property_get("tcc.video.vsync.force_enable", value, "0");
		if(atoi(value))
			mCheckSync = 1;
		else
			mCheckSync = pMParm->private_data.optional_info[13];
#endif
#ifdef DEBUG_PROCTIME_LOG
		clk_prev = clock();
#endif

		if( pMParm->private_data.optional_info[7] & 0x20000000 ) //OMX_BUFFERFLAG_INTERLACED_ONLY_ONEFIELD_FRAME
		{
			if( pMParm->mVideoDeinterlaceScaleUpOneFieldFlag == false )
			{
				pMParm->mVideoDeinterlaceScaleUpOneFieldFlag = true;
				ALOGI("ON for OneFiled only De-Interlacing");
				pMParm->mUnique_address = 0x100;
			}
			pMParm->private_data.optional_info[7] &= ~0x40000000;
		}
		else
		{
			if( pMParm->mVideoDeinterlaceScaleUpOneFieldFlag == true )
			{
				pMParm->mVideoDeinterlaceScaleUpOneFieldFlag = false;
				ALOGI("OFF for OneFiled only De-Interlacing");
				pMParm->mUnique_address = 0x100;
			}
		}

		bool OverlayCheckResult = 0;
		bool OnTheFly_prev = 0;
		bool RetOutCheck = 0;

		OnTheFly_prev = pMParm->mOntheflymode;
		RetOutCheck = output_change_check(outSx, outSy, outWidth, outHeight, transform, &OverlayCheckResult, (pMParm->private_data.optional_info[7] & 0x40000000) ? true : false);

		if( pMParm->bExtract_data == true )
		{
			if((pMParm->mUnique_address != pMParm->private_data.unique_addr) || ( mBufferId > 0 && mBufferId > pMParm->private_data.optional_info[4] )){
				pMParm->mVideoDeinterlaceFlag = false;
				pMParm->mFrame_cnt = 0;
			}

			Set_MVC_Mode(&pMParm->private_data,inWidth,inHeight,outWidth, outHeight);
			Native_FramerateMode(&pMParm->private_data,inWidth,inHeight,outWidth, outHeight);
#if 1
		    if(pMParm->mVSyncSelected && pMParm->mUnique_address)
		    {
			    if( (pMParm->mUnique_address != pMParm->private_data.unique_addr
				    || ( mBufferId > 0 && mBufferId > pMParm->private_data.optional_info[4] )
				    || ( OnTheFly_prev != pMParm->mOntheflymode )
				    || ( OutputModeChanged == 1 ))
				    && (!( pMParm->private_data.optional_info[7] & 0x80000000))
			    )
			    {
				    LOGD("Let's clearVsync. Reason(%d or %d or %d(%d!=%d) or %d)", pMParm->mUnique_address != pMParm->private_data.unique_addr,
							    mBufferId > pMParm->private_data.optional_info[4], OnTheFly_prev != pMParm->mOntheflymode, OnTheFly_prev, pMParm->mOntheflymode, OutputModeChanged);

				    LOGD("Let's clearVsync. mBufferId =%d, pMParm->private_data.optional_info[4] =%d)", mBufferId, pMParm->private_data.optional_info[4]);

				    LOGD("Let's clearVsync.  optional_info[7] =%x, option=%x \n", pMParm->private_data.optional_info[7], !(pMParm->private_data.optional_info[7] & 0x10000000));
				    clearVsync(pMParm->bExtract_data, OnTheFly_prev, true);
			    }
		    }

#else
			if(pMParm->mVSyncSelected && pMParm->mUnique_address)
			{
				if( pMParm->mUnique_address != pMParm->private_data.unique_addr
					|| ( mBufferId > 0 && mBufferId > pMParm->private_data.optional_info[4] )
					|| ( OnTheFly_prev != pMParm->mOntheflymode )
					|| ( OutputModeChanged == 1 )
				)
				{
					LOGD("Let's clearVsync. Reason(%d or %d or %d(%d!=%d) or %d)", pMParm->mUnique_address != pMParm->private_data.unique_addr,
					mBufferId > pMParm->private_data.optional_info[4], OnTheFly_prev != pMParm->mOntheflymode, OnTheFly_prev, pMParm->mOntheflymode, OutputModeChanged);

                    LOGD("Let's clearVsync. mBufferId =%d, pMParm->private_data.optional_info[4] =%d)", mBufferId, pMParm->private_data.optional_info[4]);


					clearVsync(pMParm->bExtract_data, OnTheFly_prev, true);
				}
			}
#endif
			if(OutputModeCheck)
			{
				if(!pMParm->mVSyncSelected && (OutputModeDetected == 1))
				{
					setVsync();
					pMParm->mUnique_address = pMParm->private_data.unique_addr;
					pMParm->mOverlay_recheck = false;
					LOGI("[Dual/Auto] mUnique_address 0x%x, use_GBuffer(%d).", pMParm->mUnique_address, use_Gbuffer);
				}
			}
			else
			{
				if(!pMParm->mVSyncSelected)
				{
					setVsync();
					pMParm->mUnique_address = pMParm->private_data.unique_addr;
					LOGI("mUnique_address 0x%x, use_GBuffer(%d).", pMParm->mUnique_address, use_Gbuffer);
				}
			}

			mBufferId = pMParm->private_data.optional_info[4];
		}

		if(OutputModeCheck)
		{
			if(!pMParm->mVSyncSelected && (OutputModeDetected == 1)){
				setting_mode();
				pMParm->mVSyncSelected = true;
			}
		}
		else
		{
			if(!pMParm->mVSyncSelected){
				setting_mode();
				pMParm->mVSyncSelected = true;
			}
		}

#if defined(TCC_VIDEO_DEINTERLACE_SUPPORT)
		if(pMParm->mVideoDeinterlaceSupport
			&& !pMParm->mVideoDeinterlaceScaleUpOneFieldFlag
		)
		{
			unsigned int exclusive_ui_enable;
			char value[PROPERTY_VALUE_MAX];
			unsigned int first_frame_check;

			if(pMParm->mDisplayOutputSTB){
				first_frame_check = pMParm->private_data.optional_info[4];
				if(first_frame_check){
					first_frame_check = pMParm->mFrame_cnt;
				}
			}else{
				first_frame_check = pMParm->mFrame_cnt;
			}

			//property_get("tcc.exclusive.ui.enable", value, "");
			//exclusive_ui_enable = atoi(value);

			// check first frame to enable VIQE block
			//		if(exclusive_ui_enable)
			{
				if(pMParm->mVideoDeinterlaceForce)
				{
					if(first_frame_check == 0)
					{
						LOGI("Deinterlace Mode On by force");
						pMParm->mVideoDeinterlaceFlag = true;
					}
				}
				else
				{
					if( pMParm->private_data.optional_info[7] & 0x40000000 )//OMX_BUFFERFLAG_INTERLACED_FRAME
					{
						if(first_frame_check == 0)
						{
							LOGI("Deinterlace Mode On");
							pMParm->mVideoDeinterlaceFlag = true;
						}

						if(pMParm->mVideoDeinterlaceFlag == false)
						{
							pMParm->private_data.optional_info[7] &= ~0x40000000;
						}
					} else if( pMParm->private_data.optional_info[7] & 0x00000004) { //if dxb player, force deinterlace mode on
						if(first_frame_check == 0) {
							LOGI("DXB Deinterlace Mode On");
							pMParm->mVideoDeinterlaceFlag = true;
						}
					}
				}
			}
			//else
			//{
			//pMParm->mVideoDeinterlaceFlag = true;
			//}
		}
		else
		{
			pMParm->private_data.optional_info[7] &= ~0xF0000000;
		}
#endif
		overlayInfo.flags 		= pMParm->private_data.optional_info[7];

#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
		overlayInfo.buffer_id 	= pMParm->private_data.optional_info[4];
		overlayInfo.timeStamp 	= pMParm->private_data.optional_info[5];
		overlayInfo.curTime 	= pMParm->private_data.optional_info[6];
		pMParm->mFrame_rate		= pMParm->private_data.optional_info[8];
		overlayInfo.frameView	= pMParm->private_data.optional_info[9];

		if(overlayInfo.frameView)
		{
			overlayInfo.MVC_Base_YaddrPhy = pMParm->private_data.optional_info[10];
			overlayInfo.MVC_Base_UaddrPhy = pMParm->private_data.optional_info[11];
			overlayInfo.MVC_Base_VaddrPhy = pMParm->private_data.optional_info[12];
		}

#endif

		if(OutputModeCheck)
		{
			if((RetOutCheck == 0) && (OutputModeDetected == 1))
			{
				int ret = 0;
				bool bPreProcessed = false;
				vbuffer_manager vBuff_ID_Set, vBuff_Idx_Set;

				bPreProcessed = output_Overlay(&overlayInfo, &OverlayCheckResult);

				pMParm->mIdx_codecInstance = vBuff_ID_Set.istance_index = vBuff_Idx_Set.istance_index = pMParm->private_data.optional_info[15];
				if( pMParm->mVsyncMode )
				{
					vBuff_ID_Set.index = vBuff_Idx_Set.index = -1;
				}
				else
				{
					//Check if the buffer of VPU is used directly.
					//Then if it's true, Set the previous buffer-id, otherwise Set the current buffer-id.
					vBuff_ID_Set.index = (bPreProcessed || use_Gbuffer) ? pMParm->private_data.optional_info[4] : pMParm->private_data.optional_info[4] - 1;
					//Then, Set the current buffer-index if it's true.
					vBuff_Idx_Set.index = (bPreProcessed || use_Gbuffer) ? -1 : pMParm->private_data.optional_info[14];
				}
				DBUG("(%d/%d) Buffer ID %d -> %d, Index %d -> %d", bPreProcessed, use_Gbuffer, pMParm->private_data.optional_info[4], vBuff_ID_Set.index, pMParm->private_data.optional_info[14], vBuff_Idx_Set.index);

				if(!pMParm->mVsyncMode)
				{
					/* below commands are used only when vsync is not used. */
					if( 0 > ( ret = ioctl(pMParm->mTmem_fd, TCC_VIDEO_SET_DISPLAYED_BUFFER_ID, &vBuff_ID_Set) )){
						ALOGE("Error: mTmem_fd(TCC_VIDEO_SET_DISPLAYED_BUFFER_ID)");
					}
					if( 0 > ( ret = ioctl(pMParm->mTmem_fd, TCC_VIDEO_SET_DISPLAYING_IDX, &vBuff_Idx_Set) ) ){
						LOGE("Error: pMParm->mTmem_fd(TCC_VIDEO_SET_DISPLAYING_IDX)");
					}
				}
			}
		}
		else
		{
			if( RetOutCheck == 0)
			{
				int ret = 0;
				bool bPreProcessed = false;
				vbuffer_manager vBuff_ID_Set, vBuff_Idx_Set;

				bPreProcessed = output_Overlay(&overlayInfo, &OverlayCheckResult);

				pMParm->mIdx_codecInstance = vBuff_ID_Set.istance_index = vBuff_Idx_Set.istance_index = pMParm->private_data.optional_info[15];
				if( pMParm->mVsyncMode )
				{
					vBuff_ID_Set.index = vBuff_Idx_Set.index = -1;
				}
				else
				{
					//Check if the buffer of VPU is used directly.
					//Then if it's true, Set the previous buffer-id, otherwise Set the current buffer-id.
					vBuff_ID_Set.index = (bPreProcessed || use_Gbuffer) ? pMParm->private_data.optional_info[4] : pMParm->private_data.optional_info[4] - 1;
					//Then, Set the current buffer-index if it's true.
					vBuff_Idx_Set.index = (bPreProcessed || use_Gbuffer) ? -1 : pMParm->private_data.optional_info[14];
				}
				DBUG("(%d/%d) Buffer ID %d -> %d, Index %d -> %d", bPreProcessed, use_Gbuffer, pMParm->private_data.optional_info[4], vBuff_ID_Set.index, pMParm->private_data.optional_info[14], vBuff_Idx_Set.index);

				if(!pMParm->mVsyncMode)
				{
					/* below commands are used only when vsync is not used. */
					if( 0 > ( ret = ioctl(pMParm->mTmem_fd, TCC_VIDEO_SET_DISPLAYED_BUFFER_ID, &vBuff_ID_Set) )){
						ALOGE("Error: mTmem_fd(TCC_VIDEO_SET_DISPLAYED_BUFFER_ID)");
					}
					if( 0 > ( ret = ioctl(pMParm->mTmem_fd, TCC_VIDEO_SET_DISPLAYING_IDX, &vBuff_Idx_Set) ) ){
						LOGE("Error: pMParm->mTmem_fd(TCC_VIDEO_SET_DISPLAYING_IDX)");
					}
				}
			}
		}

#ifdef DEBUG_PROCTIME_LOG
		clock_t clk_cur;

		clk_cur = clock();
		dec_time[time_cnt] = (clk_cur-clk_prev)*1000/CLOCKS_PER_SEC;
		total_dec_time += dec_time[time_cnt];
		if(time_cnt != 0 && time_cnt % 29 == 0)
		{
			LOGD("Disp %d ms = %d/%d: %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d",
				total_dec_time/pMParm->mFrame_cnt, total_dec_time, pMParm->mFrame_cnt, dec_time[0], dec_time[1], dec_time[2], dec_time[3], dec_time[4], dec_time[5], dec_time[6], dec_time[7], dec_time[8], dec_time[9],
				dec_time[10], dec_time[11], dec_time[12], dec_time[13], dec_time[14], dec_time[15], dec_time[16], dec_time[17], dec_time[18], dec_time[19],
				dec_time[20], dec_time[21], dec_time[22], dec_time[23], dec_time[24], dec_time[25], dec_time[26], dec_time[27], dec_time[28], dec_time[29]);
			time_cnt = 0;
		}
		else{
			time_cnt++;
		}
#endif
	}


    return 0;
}

