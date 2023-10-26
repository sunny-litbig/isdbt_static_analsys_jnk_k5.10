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
#define LOG_NDEBUG 1
#define LOG_TAG "TCCUTILS"
#include <utils/Log.h>
#include <cutils/properties.h>

#include <libpmap/pmap.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/mman.h>

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <linux/fb.h>
#include <tccfb_ioctrl.h>
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE 
#include <tcc_composite_ioctl.h>
#include <tcc_component_ioctl.h>
#endif
#include <TCC_VPU_CODEC.h>
#include <tcc_jpeg_ioctl.h>

#define USE_WMIXER_FOR_COPY
#ifdef USE_WMIXER_FOR_COPY
#include <tcc_wmixer_ioctrl.h>
#include <vioc_global.h>
#define COPY_DEVICE             "/dev/wmixer1"
#else
#include <tcc_scaler_ioctrl.h>
#define SCALE_DEVICE             "/dev/scaler1"
#endif

#if defined(TCC_VIDEO_DEINTERLACE_SUPPORT)
#include <tcc_viqe_ioctl.h>
#define VIQE_DEVICE              "/dev/viqe"
#endif

#include "../../../dxb_components/decoders/dxb_cdk_library/cdk/jpu_jpeg.h"

#include "TCCMemory.h"
#include "tcc_utils.h"

#define JPEGENC_DEVICE          "/dev/jpegenc"
#define FB0_DEVICE              "/dev/graphics/fb0"

#define ALIGNED_BUFF(buf, mul) ( ( (unsigned int)buf + (mul-1) ) & ~(mul-1) )

//#define YUV_VIDEO_DUMP

typedef enum OMX_BOOL {
    OMX_FALSE = 0,
    OMX_TRUE = !OMX_FALSE,
    OMX_BOOL_MAX = 0x7FFFFFFF
} OMX_BOOL;

typedef enum DxB_EVENT_ERR_CODE {
    DxB_EVENT_ERR_OK= 0,
    DxB_EVENT_ERR_ERR,
    DxB_EVENT_ERR_NOFREESPACE,
    DxB_EVENT_ERR_FILEOPENERROR,
    DxB_EVENT_ERR_INVALIDMEMORY,
    DxB_EVENT_ERR_INVALIDOPERATION,
    DxB_EVENT_FAIL_NOFREESPACE,
    DxB_EVENT_FAIL_INVALIDSTORAGE,
    DxB_EVENT_ERR_MAX_SIZE,
    DxB_EVENT_ERR_MAX,
}DxB_EVENT_ERR_CODE;

typedef struct {
	int m_FD_COPY;

	OMX_BOOL bVIQEInit;
	int m_FD_VIQE;
	int iVIQEMode;

	int	jpeg_enc_fd;

	pmap_t pmap_capture;

	#if defined(TCC_VIDEO_DEINTERLACE_SUPPORT)
	VIQE_DI_TYPE viqeDIInfo;
	#endif
} tcc_video_utils_t;

static int getDisplaySize(tcc_video_utils_t *pVideoUtils, OUTPUT_SELECT Output, tcc_display_size *display_size, char *componentChip)
{
#ifdef USE_OUTPUT_COMPONENT_COMPOSITE 
	char value[PROPERTY_VALUE_MAX];
	property_get("persist.sys.composite_mode", value, "");
	int compositeMode = atoi(value);

	property_get("persist.sys.component_mode", value, "");
	int componentMode = atoi(value);
#endif

	switch (Output) {
	case OUTPUT_NONE: {	// LCD size
		/* open framebuffer device */
		int check_device = open(FB0_DEVICE, O_RDWR);
		if (check_device >= 0) {
			struct fb_var_screeninfo info;
			ioctl(check_device, FBIOGET_VSCREENINFO, &info);
			ALOGE("%d, %d", info.xres, info.yres);
			close(check_device);
			display_size->width = info.xres;
			display_size->height = info.yres;
		} else {
			ALOGE("can't open '%s'", FB0_DEVICE);
		}
	}
	break;

	case OUTPUT_HDMI: {	// HDMI
		tcc_display_size HdmiSize;
		// check external HDMI size
		int check_device = open(FB0_DEVICE, O_RDWR);
		if (ioctl(check_device, TCC_LCDC_HDMI_GET_SIZE, &HdmiSize) < 0)	{
			ALOGE(" TCC_LCDC_HDMI_GET_SIZE  ERROR");
		}
		close(check_device);
		*display_size = HdmiSize;
	}
	break;

#ifdef USE_OUTPUT_COMPONENT_COMPOSITE 
	case OUTPUT_COMPOSITE: {	//composite
		if (compositeMode == OUTPUT_COMPOSITE_NTSC) {
			display_size->width	= TCC_COMPOSITE_NTSC_WIDTH;
			display_size->height = TCC_COMPOSITE_NTSC_HEIGHT;
		} else {
			display_size->width	= TCC_COMPOSITE_PAL_WIDTH;
			display_size->height = TCC_COMPOSITE_PAL_HEIGHT;
		}
	}
	break;

	case OUTPUT_COMPONENT: {	//component
		if (!strcasecmp(componentChip, "cs4954")) {
			display_size->width	= TCC_COMPONENT_NTSC_WIDTH;
			display_size->height = TCC_COMPONENT_NTSC_HEIGHT;
		} else {
			if (componentMode == OUTPUT_COMPONENT_1080I) {
				display_size->width	= TCC_COMPONENT_1080I_WIDTH;
				display_size->height = TCC_COMPONENT_1080I_HEIGHT;
			} else {
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

#if defined(TCC_VIDEO_DEINTERLACE_SUPPORT)
static int deinterlace_process(tcc_video_utils_t *pVideoUtils, int fieldInfomation, int interlaceMode, int useWMIXER, unsigned int width, unsigned int height, unsigned char *YSrc, unsigned char *USrc, unsigned char *VSrc, char bSrcYUVInter, unsigned char *addrDst)
{
    unsigned int stride;
    unsigned int framesize_Y, framesize_C;
    struct pollfd poll_event[1];
    char value[PROPERTY_VALUE_MAX];

    stride = ALIGNED_BUFF(width, 16);
    framesize_Y = stride * height;
    framesize_C = ALIGNED_BUFF(stride/2, 16) * height/2;

    if(pVideoUtils->m_FD_VIQE >= 0) {

        if (stride != pVideoUtils->viqeDIInfo.srcWidth || height != pVideoUtils->viqeDIInfo.srcHeight) {
	        property_get("tcc.video.deinterlace.support", value, "0");
	        if((pVideoUtils->viqeDIInfo.srcWidth != stride && pVideoUtils->viqeDIInfo.srcHeight != height) || atoi(value) == 0) {
	            property_set("tcc.video.deinterlace.support", "1");
	            if(pVideoUtils->bVIQEInit) {
	                ioctl(pVideoUtils->m_FD_VIQE, IOCTL_VIQE_DEINITIALIZE);
	                pVideoUtils->bVIQEInit = OMX_FALSE;
	            }
	        }
	        memset(&pVideoUtils->viqeDIInfo, 0x00, sizeof(pVideoUtils->viqeDIInfo));
	}

        if(pVideoUtils->bVIQEInit == OMX_FALSE) {
        pVideoUtils->viqeDIInfo.lcdCtrlNo = 0;
        pVideoUtils->viqeDIInfo.scalerCh = 0;
        pVideoUtils->viqeDIInfo.onTheFly = 0;
//        pVideoUtils->viqeDIInfo.useWMIXER = 1;//useWMIXER;
        pVideoUtils->viqeDIInfo.srcWidth =  stride;
        pVideoUtils->viqeDIInfo.srcHeight = height;
        pVideoUtils->viqeDIInfo.crop_top = 0;
        pVideoUtils->viqeDIInfo.crop_bottom = 0;
        pVideoUtils->viqeDIInfo.crop_left = 0;
        pVideoUtils->viqeDIInfo.crop_right = 0;
        pVideoUtils->viqeDIInfo.deinterlaceM2M = 1;
        pVideoUtils->viqeDIInfo.renderCnt = 0;
        pVideoUtils->viqeDIInfo.OddFirst = fieldInfomation;

        }

	if(interlaceMode)
	        pVideoUtils->viqeDIInfo.DI_use = 0;
	else
	        pVideoUtils->viqeDIInfo.DI_use = 1;

        pVideoUtils->viqeDIInfo.address[0] = (unsigned int)YSrc;
        pVideoUtils->viqeDIInfo.address[1] = (unsigned int)USrc;
        pVideoUtils->viqeDIInfo.address[2] = (unsigned int)VSrc;
        pVideoUtils->viqeDIInfo.dstAddr = (unsigned int)addrDst;

        if(pVideoUtils->bVIQEInit == OMX_FALSE) {
            if (ioctl(pVideoUtils->m_FD_VIQE, IOCTL_VIQE_INITIALIZE, &pVideoUtils->viqeDIInfo) < 0) {
                ALOGE("VIQE DI Init Error!" );
                return -1;
            }
            pVideoUtils->bVIQEInit = OMX_TRUE;
        }
        if (ioctl(pVideoUtils->m_FD_VIQE, IOCTL_VIQE_EXCUTE, &pVideoUtils->viqeDIInfo) < 0) {
            ALOGE("VIQE DI Run Error!");
            return -1;
        }
    }

    return 0;
}
#endif

TCC_VIDEO_UTILS TCCDxB_ScalerOpen(void)
{
	tcc_video_utils_t *pVideoUtils = (tcc_video_utils_t*) TCC_fo_malloc(__func__, __LINE__, sizeof(tcc_video_utils_t));
	if(pVideoUtils == NULL){
		return (TCC_VIDEO_UTILS) pVideoUtils;
	}
	pVideoUtils->bVIQEInit = OMX_FALSE;
	pVideoUtils->m_FD_VIQE = -1;
	pVideoUtils->iVIQEMode = 0;
	pVideoUtils->jpeg_enc_fd = -1;

#ifdef USE_WMIXER_FOR_COPY
	pVideoUtils->m_FD_COPY = open(COPY_DEVICE, O_RDWR);
	if (pVideoUtils->m_FD_COPY < 0) {
		ALOGE("can't open '%s'", COPY_DEVICE);
	}
#else
	pVideoUtils->m_FD_COPY = open(SCALE_DEVICE, O_RDWR);
	if (pVideoUtils->m_FD_COPY < 0) {
		ALOGE("can't open '%d'", pVideoUtils->m_FD_COPY);
	}
#endif

#if 0//defined(TCC_VIDEO_DEINTERLACE_SUPPORT)
    {
        char value[PROPERTY_VALUE_MAX];
        property_get("tcc.video.deinterlace.support", value, "0");
        pVideoUtils->iVIQEMode = atoi(value);
        ALOGI("tcc.video.deinterlace.support : %d", pVideoUtils->iVIQEMode);
        if(pVideoUtils->iVIQEMode)
        {
            pVideoUtils->m_FD_VIQE = open(VIQE_DEVICE, O_RDWR);
            if (pVideoUtils->m_FD_VIQE < 0) {
                ALOGE("can't open %s", VIQE_DEVICE);
            }
            pVideoUtils->bVIQEInit = OMX_FALSE;
        }
    }
#endif

    return (TCC_VIDEO_UTILS) pVideoUtils;
}

void TCCDxB_ScalerClose(TCC_VIDEO_UTILS hUtils)
{
	tcc_video_utils_t *pVideoUtils = (tcc_video_utils_t*) hUtils;

#ifdef USE_WMIXER_FOR_COPY
	if (pVideoUtils->m_FD_COPY >= 0)
		close(pVideoUtils->m_FD_COPY);
	pVideoUtils->m_FD_COPY = -1;
#else
	if (pVideoUtils->m_FD_COPY >= 0)
		close(pVideoUtils->m_FD_COPY);
	pVideoUtils->m_FD_COPY = -1;
#endif

#if defined(TCC_VIDEO_DEINTERLACE_SUPPORT)
	if (pVideoUtils->iVIQEMode)	{
		if (pVideoUtils->m_FD_VIQE >= 0) {
			if (pVideoUtils->bVIQEInit) {
				//ioctl(pVideoUtils->m_FD_VIQE, IOCTL_VIQE_DEINITIALIZE);
				pVideoUtils->bVIQEInit = OMX_FALSE;
			}
			close(pVideoUtils->m_FD_VIQE);
			pVideoUtils->m_FD_VIQE = -1;
		}
		pVideoUtils->iVIQEMode = 0;
	}
#endif
	TCC_fo_free(__func__, __LINE__, pVideoUtils);
}

int TCCDxB_ScalerCopyData(TCC_VIDEO_UTILS hUtils, unsigned int width, unsigned int height, unsigned char *YSrc, unsigned char *USrc, unsigned char *VSrc,
								char bSrcYUVInter, unsigned char *addrDst, unsigned char ignoreAligne, int fieldInfomation, int interlaceMode)
{
	tcc_video_utils_t *pVideoUtils = (tcc_video_utils_t*) hUtils;

     //ignoreAligne : true : frame copy for mali draw, false : mem copy for capturing

#ifdef USE_WMIXER_FOR_COPY
    WMIXER_INFO_TYPE WmixerInfo;
#else
    SCALER_TYPE ScaleInfo;
#endif
    int use_device = -1, ret = -1;

    unsigned int stride;
    unsigned int framesize_Y, framesize_C;
    struct pollfd poll_event[1];

    if(ignoreAligne == true)
    {
        stride = ALIGNED_BUFF(width, 16);
        framesize_Y = stride * height;
        framesize_C = ALIGNED_BUFF(stride/2, 16) * height/2;
    }
    else
    {
        stride = width;
        framesize_Y = stride * height;
        framesize_C = stride/2 * height/2;
    }

    {
#ifdef USE_WMIXER_FOR_COPY
	#if defined(TCC_VIDEO_DEINTERLACE_SUPPORT)
        if(pVideoUtils->m_FD_VIQE >= 0)
            deinterlace_process(pVideoUtils, fieldInfomation, interlaceMode, 1, width, height, YSrc, USrc, VSrc, bSrcYUVInter, addrDst);
    #endif
        use_device = pVideoUtils->m_FD_COPY;
        memset(&WmixerInfo, 0x00, sizeof(WmixerInfo));
        WmixerInfo.rsp_type         = WMIXER_INTERRUPT;

        WmixerInfo.src_y_addr       = (unsigned int)YSrc;
        WmixerInfo.src_u_addr       = (unsigned int)USrc;
        WmixerInfo.src_v_addr       = (unsigned int)VSrc;

        if(bSrcYUVInter)
            WmixerInfo.src_fmt  = VIOC_IMG_FMT_YUV420IL0;
        else
            WmixerInfo.src_fmt      = VIOC_IMG_FMT_YUV420SEP;

        WmixerInfo.dst_y_addr       = (unsigned int)addrDst;
        WmixerInfo.dst_u_addr       = (unsigned int)((char*)WmixerInfo.dst_y_addr + framesize_Y);
        WmixerInfo.dst_v_addr       = (unsigned int)((char*)WmixerInfo.dst_u_addr + framesize_C);

        if(ignoreAligne == true)
            WmixerInfo.dst_fmt          = VIOC_IMG_FMT_YUV420IL0;
        else
            WmixerInfo.dst_fmt          = VIOC_IMG_FMT_YUV420SEP;

		WmixerInfo.src_img_width 		= stride;
		WmixerInfo.src_img_height		= height;
		WmixerInfo.src_win_left			= 0;
		WmixerInfo.src_win_top			= 0;
		WmixerInfo.src_win_right		= stride;
		WmixerInfo.src_win_bottom		= height;

		WmixerInfo.dst_img_width 		= stride;
		WmixerInfo.dst_img_height		= height;
		WmixerInfo.dst_win_left			= 0;
		WmixerInfo.dst_win_top			= 0;
		WmixerInfo.dst_win_right		= stride;
		WmixerInfo.dst_win_bottom		= height;

        //ALOGW("%s copy :: 0x%x-0x%x-0x%x -> 0x%x(0x%x-0x%x-0x%x)", COPY_DEVICE, WmixerInfo.src_y_addr, WmixerInfo.src_u_addr, WmixerInfo.src_v_addr, addrDst, WmixerInfo.dst_y_addr, WmixerInfo.dst_u_addr, WmixerInfo.dst_v_addr);

        if (ioctl(use_device, TCC_WMIXER_IOCTRL, &WmixerInfo) < 0)
        {
            ALOGE("%s Out Error 1 !!![%d]", COPY_DEVICE, errno);
            return -1;
        }

#else
        use_device = pVideoUtils->m_FD_COPY;

        memset(&ScaleInfo, 0x00, sizeof(ScaleInfo));
        ScaleInfo.src_Yaddr         = (char*)YSrc;
        ScaleInfo.src_Uaddr         = (char*)USrc;
        ScaleInfo.src_Vaddr         = (char*)VSrc;
        ScaleInfo.responsetype      = SCALER_INTERRUPT;
        if(bSrcYUVInter)
            ScaleInfo.src_fmt       = SCALER_YUV420_inter;
        else
            ScaleInfo.src_fmt       = SCALER_YUV420_sp;
        ScaleInfo.src_ImgWidth      = width;
        ScaleInfo.src_ImgHeight     = height;
        ScaleInfo.src_winLeft       = 0;
        ScaleInfo.src_winTop        = 0;
        ScaleInfo.src_winRight      = ScaleInfo.src_winLeft + ScaleInfo.src_ImgWidth;
        ScaleInfo.src_winBottom     = ScaleInfo.src_winTop + ScaleInfo.src_ImgHeight;

        ScaleInfo.dest_Yaddr        = (char*)addrDst;
        if(ignoreAligne == true)
        {
            ScaleInfo.dest_Vaddr    = (char*)ScaleInfo.dest_Yaddr + framesize_Y;
            ScaleInfo.dest_Uaddr    = (char*)ScaleInfo.dest_Vaddr + framesize_C;
        }
        else
        {
            ScaleInfo.dest_Uaddr    = (char*)ScaleInfo.dest_Yaddr + framesize_Y;
            ScaleInfo.dest_Vaddr    = (char*)ScaleInfo.dest_Uaddr + framesize_C;
        }
        //ALOGW("Scaler copy :: 0x%p-0x%p-0x%p -> 0x%p(0x%p-0x%p-0x%p)", ScaleInfo.src_Yaddr, ScaleInfo.src_Uaddr, ScaleInfo.src_Vaddr, addrDst, ScaleInfo.dest_Yaddr, ScaleInfo.dest_Uaddr, ScaleInfo.dest_Vaddr);

        ScaleInfo.dest_fmt          = SCALER_YUV420_sp;
        ScaleInfo.dest_ImgWidth     = width;
        ScaleInfo.dest_ImgHeight    = height;
        ScaleInfo.dest_winLeft      = 0;
        ScaleInfo.dest_winTop       = 0;
        ScaleInfo.dest_winRight     = ScaleInfo.dest_winLeft + ScaleInfo.dest_ImgWidth;
        ScaleInfo.dest_winBottom    = ScaleInfo.dest_winTop + ScaleInfo.dest_ImgHeight;

        if (ioctl(use_device, TCC_SCALER_IOCTRL, &ScaleInfo) < 0)
        {
            ALOGE("%s Out Error 2 !!![%d]", SCALE_DEVICE, errno);
            return -1;
        }
#endif
    }
#if 0
    {
        FILE *fp;
    	char file_name[32];
    	static int giDumpFileIndex = 0;
		sprintf(file_name, "/sdcard/in_display%d.raw", ++giDumpFileIndex);
		if( giDumpFileIndex < 5 )
        {
            fp = fopen(file_name, "wb");
            if(fp)
            {
                char *buffer = (char*)mmap(NULL, 0x200000, PROT_READ|PROT_WRITE, MAP_SHARED, pVideoUtils->m_FD_COPY, (unsigned int)addrDst);
                if (MAP_FAILED == buffer)
                {
                    ALOGE(" $$$$$$$	ERROR [%d] :: MMAP failed!", -1);
                }

                ALOGE("dump file created !!!: %s", file_name);
                fwrite((char *)buffer, 1, width*height, fp);
                fwrite((char *)buffer+framesize_Y, 1, width*height/4, fp);
                fwrite((char *)buffer+framesize_Y+framesize_C, 1, width*height/4, fp);
                fclose(fp);
                sync();
                munmap(buffer, 0x200000);
            }
            else
                ALOGE("dump file open failed : %s", file_name);
        }
    }
#endif
    {
        int ret;
        memset(poll_event, 0, sizeof(poll_event));
        poll_event[0].fd = use_device;
        poll_event[0].events = POLLIN;
        ret = poll((struct pollfd*)poll_event, 1, 400);

        if (ret < 0) {
            ALOGE("m2m poll error\n");
            return -1;
        }else if (ret == 0) {
            ALOGE("m2m poll timeout\n");
            return -1;
        }else if (ret > 0) {
            if (poll_event[0].revents & POLLERR) {
                ALOGE("m2m poll POLLERR\n");
                return -1;
            }
        }
    }
    return 0;
}

void TCCDxB_GetDispSize(TCC_VIDEO_UTILS hUtils, unsigned int *p_disp_w, unsigned int *p_disp_h)
{
	tcc_video_utils_t *pVideoUtils = (tcc_video_utils_t*) hUtils;

	tcc_display_size display_size;
	char value[PROPERTY_VALUE_MAX];
	char componentChip[PROPERTY_VALUE_MAX];
	unsigned int uiOutputMode = 0;

	if ((p_disp_w == NULL) || (p_disp_h == NULL)) {
		ALOGE("[TCCDxB_GetDispSize] Null pointer exception\n");
		return;
	}

	*p_disp_w = *p_disp_h = 0;

	property_get("persist.sys.output_mode", value, "");
	uiOutputMode = atoi(value);

	property_get("tcc.component.chip", componentChip, "0");

	getDisplaySize(pVideoUtils, (OUTPUT_SELECT)uiOutputMode, &display_size, componentChip);

	*p_disp_w = display_size.width;
	*p_disp_h = display_size.height;
}

static void Convert_YUV420_INTER_TO_YUV420_SP(tcc_video_utils_t *pVideoUtils, void *data, void *dst, int width, int height)
{
        const uint8_t *src_y = (const uint8_t *)data;
        const uint8_t *src_uv = (const uint8_t *)data + width * height;
        uint8_t *dst_y = (uint8_t *)dst;

        size_t dst_y_size = width * height;
        size_t dst_c_stride = ALIGNED_BUFF(width / 2, 16);
        size_t dst_c_size = dst_c_stride * height / 2;
        uint8_t *dst_u = dst_y + dst_y_size;
        uint8_t *dst_v = dst_u + dst_c_size;

        for (int y = 0; y < height; ++y) {
            memcpy(dst_y, src_y, width);

            src_y += width;
            dst_y += width;
        }

        for (int y = 0; y < (height + 1) / 2; ++y) {
            size_t tmp = (width + 1) / 2;
            for (size_t x = 0; x < tmp; ++x) {
                dst_u[x] = src_uv[2 * x];
                dst_v[x] = src_uv[2 * x + 1];
            }

            src_uv += width;
            dst_u += dst_c_stride;
            dst_v += dst_c_stride;
        }
}
#if 0
int TCCDxB_CaptureImage(TCC_VIDEO_UTILS hUtils, unsigned int width, unsigned int height, int *yuvbuffer, int format, int interlaceMode, int fieldInfomation, unsigned char *strFilePath, int useMali)
{
	tcc_video_utils_t *pVideoUtils = (tcc_video_utils_t*) hUtils;

    TCCXXX_JPEG_ENC_DATA VideoCaptureInfo;

    char *data = NULL;
    char *jpeg_buffer, *temp_buffer;
    unsigned int dAddress, dSize;
    int ret;
    int jpeg_size;
    char value[PROPERTY_VALUE_MAX];
    char componentChip[PROPERTY_VALUE_MAX];
    unsigned int	uiOutputMode = 0;
    tcc_display_size display_size;

    pmap_get_info("jpg_raw_dxb", &pVideoUtils->pmap_capture);

    unsigned char *yuvData = NULL;
    //Capture!!
    pVideoUtils->jpeg_enc_fd = open(JPEGENC_DEVICE,O_RDWR);
    if (pVideoUtils->jpeg_enc_fd <= 0) {
        ALOGE("can't open jpeg device '%s'", JPEGENC_DEVICE);
        return DxB_EVENT_ERR_ERR;
    }

    property_get("persist.sys.output_mode", value, "");
    uiOutputMode = atoi(value);
    property_get("tcc.component.chip", componentChip, "0");

    getDisplaySize(pVideoUtils, (OUTPUT_SELECT)uiOutputMode, &display_size, componentChip);

    //ALOGD("%d, %d", display_size.width, display_size.height);
    if((width * height / 4) >=(unsigned int) (display_size.width * display_size.height))
        interlaceMode = 1;

    unsigned char *virt_jpeg = (unsigned char *)mmap(0, pVideoUtils->pmap_capture.size, (PROT_READ|PROT_WRITE), MAP_SHARED, pVideoUtils->jpeg_enc_fd, pVideoUtils->pmap_capture.base);
    memset(virt_jpeg, 0xff, pVideoUtils->pmap_capture.size);
#if defined(_TCC8960_) || defined(_TCC8970_)
    if(useMali)
    {
        unsigned int stride = width;
        unsigned int framesize_Y = stride * height;
        unsigned int framesize_C = stride/2 * height/2;

        yuvData = (unsigned char*)malloc(width*height*3/2);
        memcpy((void*)yuvData, (void*)yuvbuffer[0], framesize_Y);
        memcpy((void*)(yuvData + framesize_Y), (void*)yuvbuffer[1], framesize_C);
        memcpy((void*)(yuvData + framesize_Y + framesize_C), (void*)yuvbuffer[2], framesize_C);

        Convert_YUV420_INTER_TO_YUV420_SP(pVideoUtils, yuvData, virt_jpeg, width, height);
        free(yuvData);
    }
    else
#endif
    {
        TCCDxB_ScalerCopyData(hUtils, width, height, (unsigned char*)yuvbuffer[0], (unsigned char*)yuvbuffer[1], (unsigned char*)yuvbuffer[2], format, (unsigned char*)pVideoUtils->pmap_capture.base, false, fieldInfomation, interlaceMode);
    }
    //unsigned char *virt_jpeg;
    unsigned char *virt_encoded_jpeg;
    uint32_t src_buffer_size = pVideoUtils->pmap_capture.size;
	jpu_conf_enc_t gsJPUEncodeInfo;

    memset(&VideoCaptureInfo, 0x00, sizeof(VideoCaptureInfo));
    VideoCaptureInfo.source_addr    = (unsigned int)pVideoUtils->pmap_capture.base;
    VideoCaptureInfo.width          = width;
    VideoCaptureInfo.height         = height;
    VideoCaptureInfo.q_value        = JPEG_SUPERFINE;
    VideoCaptureInfo.src_format     = ENC_INPUT_420;
    VideoCaptureInfo.target_addr    = (unsigned int )pVideoUtils->pmap_capture.base;
    VideoCaptureInfo.target_size    = pVideoUtils->pmap_capture.size;

    dAddress = VideoCaptureInfo.target_addr;
    dSize    = VideoCaptureInfo.target_size;

    //ALOGE("addr=0x%x, size=0x%x", yuvbuffer, src_buffer_size);
    //ALOGE("capture=%d x %d", width, height);
    //virt_jpeg = (unsigned char *)mmap(0, src_buffer_size, (PROT_READ|PROT_WRITE), MAP_SHARED, pVideoUtils->jpeg_enc_fd, VideoCaptureInfo.source_addr);

#ifdef YUV_VIDEO_DUMP
    {
        FILE *fp;
        char file_name[32];
        static int giDumpFileIndex = 0;
        sprintf(file_name, "storage/sdcard0/in_display%d.raw", ++giDumpFileIndex);
        if( giDumpFileIndex < 5 )
        {
            fp = fopen(file_name, "wb");
            if(fp)
            {
                ALOGE("dump file created !!!: %s", file_name);
                fwrite((char *)virt_jpeg, 1, width*height*3/2, fp);
                //fwrite((char *)buffer+framesize_Y, 1, width*height/4, fp);
                //fwrite((char *)buffer+framesize_Y+framesize_C, 1, width*height/4, fp);
                fclose(fp);
            }
            else
                ALOGE("dump file open failed : %s", file_name);
        }
    }
#endif

    virt_encoded_jpeg = (unsigned char *)mmap(0, dSize, (PROT_READ|PROT_WRITE), MAP_SHARED, pVideoUtils->jpeg_enc_fd, VideoCaptureInfo.target_addr);
    ALOGI("virt_jpeg[%p], virt_encoded_jpeg[%p], jpeg_enc_fd[0x%x]",virt_jpeg, virt_encoded_jpeg, pVideoUtils->jpeg_enc_fd);

    // Initialize JPU
    memset(&gsJPUEncodeInfo, 0x00, sizeof(jpu_conf_enc_t));
    gsJPUEncodeInfo.m_InpPhyYUVBuffAddr         = (codec_addr_t)VideoCaptureInfo.source_addr;
    gsJPUEncodeInfo.m_InpVirtYUVBuffAddr        = (codec_addr_t)virt_jpeg;
    gsJPUEncodeInfo.m_outVirtHeaderBuffAddr     = (codec_addr_t)virt_encoded_jpeg;
    gsJPUEncodeInfo.m_iTotalFrames              = 1;
    gsJPUEncodeInfo.m_iWidth                    = width;
    gsJPUEncodeInfo.m_iHeight                   = height;
    gsJPUEncodeInfo.m_iFramesPerSecond          = 30;
    gsJPUEncodeInfo.m_iEncQuality               = 2; //JPEG_SUPERFINE;
    gsJPUEncodeInfo.m_iYuvFormat                = YUV_FORMAT_420;
	gsJPUEncodeInfo.m_iYUVWidth                 = width;
	gsJPUEncodeInfo.m_iYUVHeight                = height;

    ALOGI("[%d x %d], [%d x %d]", gsJPUEncodeInfo.m_iWidth, gsJPUEncodeInfo.m_iHeight, gsJPUEncodeInfo.m_iYUVWidth, gsJPUEncodeInfo.m_iYUVHeight);
    ALOGI("JPU ENC Start!!");
    ret = jpu_enc(pVideoUtils->jpeg_enc_fd, &gsJPUEncodeInfo);
    ALOGI("JPU ENC End!!");
    ALOGI("JPU ENC INFO:  jpeg_start_addr=0x%08x, jpeg_header_size=0x%x, jpeg_stream_size=0x%x, jpeg_total_size=0x%x.",	\
            gsJPUEncodeInfo.m_outVirtHeaderBuffAddr, gsJPUEncodeInfo.m_outSizeOfHeaderBuff, 							\
            gsJPUEncodeInfo.m_outSizeOfEncodedBuff, gsJPUEncodeInfo.m_outSizeOfTotalBuff);

    VideoCaptureInfo.header_size                = gsJPUEncodeInfo.m_outSizeOfHeaderBuff;
    VideoCaptureInfo.header_offset              = 0;
    VideoCaptureInfo.bitstream_size             = gsJPUEncodeInfo.m_outSizeOfEncodedBuff;
    VideoCaptureInfo.bitstream_offset           = 64 * 1024;

    if(munmap((void *)virt_jpeg, src_buffer_size) < 0) {
        ALOGE("err: UNMAP(virt_jpeg) failed. addr=0x%x, size=0x%x", (unsigned int)virt_jpeg, src_buffer_size);
    }

    if(munmap((void *)virt_encoded_jpeg, dSize) < 0) {
        ALOGE("err: UNMAP(virt_encoded_jpeg) failed. addr=0x%x, size=0x%x", (unsigned int)virt_encoded_jpeg, dSize);
    }

    jpeg_size	= VideoCaptureInfo.bitstream_size + VideoCaptureInfo.header_size;
    ALOGD("Capture Done = (%d)%d KB (%d / %d)!!\n", jpeg_size, jpeg_size/1024, VideoCaptureInfo.header_size, VideoCaptureInfo.bitstream_size);

    jpeg_buffer = (char*)mmap(NULL, dSize, PROT_READ|PROT_WRITE, MAP_SHARED, pVideoUtils->jpeg_enc_fd, dAddress);
    if (MAP_FAILED == jpeg_buffer)
    {
        ALOGE(" $$$$$$$	ERROR [%d] :: MMAP failed!", ret);

        close(pVideoUtils->jpeg_enc_fd);
        return DxB_EVENT_ERR_INVALIDMEMORY;
    }

    ALOGI("%s", strFilePath);
    FILE *fp = fopen((char *)strFilePath, "wb");
    if( fp == NULL )
    {
        ALOGE("Can NOT make file(%s)\n", strFilePath);
        munmap(jpeg_buffer, dSize);
        close(pVideoUtils->jpeg_enc_fd);
        return DxB_EVENT_ERR_FILEOPENERROR;
    }

    if(jpeg_buffer)
    {
		int write_size = 0;
#if 0
		int i=0, sof_marker=0;
		#define _baseline_ 0xFFC0
		#define _extened_sequential_ 0xFFC1
		#define _progressive_ 0xFFC2
		while(i<gsJPUEncodeInfo.m_outSizeOfTotalBuff)
		{
			sof_marker <<= 8;
			sof_marker |= jpeg_buffer[i];
			switch(sof_marker & 0x0000FFFF)
			{
				case _baseline_:
				case _extened_sequential_:
				case _progressive_:
					{
						jpeg_buffer[i+1+2+1] = (height & 0xFF00)>> 8;
						jpeg_buffer[i+1+2+1+1] = (height & 0x00FF);
					}
					i = gsJPUEncodeInfo.m_outSizeOfTotalBuff;
					break;
			}
			i++;
		}
#endif
        write_size = fwrite(jpeg_buffer, gsJPUEncodeInfo.m_outSizeOfTotalBuff, 1, fp);
        if (write_size != 1) {
            munmap(jpeg_buffer, dSize);
            close(pVideoUtils->jpeg_enc_fd);
            fclose(fp);
            remove((char *)strFilePath);
            return DxB_EVENT_ERR_NOFREESPACE;
        }
        munmap(jpeg_buffer, dSize);
        close(pVideoUtils->jpeg_enc_fd);
        fclose(fp);
    }else{
    	fclose(fp);
    }

    return DxB_EVENT_ERR_OK;
}
#endif


void BackupLayerOrder(void)
{
#if 0
	int fb_fd = -1;
	int lcd_num = -1, rtn = -1, stb_mode =  0;
	char value[PROPERTY_VALUE_MAX]={0,};
	tccfb_set_wmix_order_type	wmix_order_type;

	if ((fb_fd = open(FB0_DEVICE, O_RDWR)) >= 0){
		rtn = ioctl(fb_fd, TCC_LCDC_GET_ACT_DISP_NUM, &wmix_order_type.num);
		if (rtn < 0) {
			ALOGE("[%s] can't get lcd num", __func__);
		} else {
			property_get("tcc.display.output.stb", value, "");
			stb_mode = atoi(value);

			wmix_order_type.ovp = (stb_mode)?1:0;
			rtn = ioctl(fb_fd, TCC_LCDC_SET_LAYER_ORDER, &wmix_order_type);
			g_old_layer_num = wmix_order_type.num;
			g_old_layer_order = wmix_order_type.ovp;

			ALOGD("[%s] wmix_num:%d, order:%d\n", __func__, g_old_layer_num, g_old_layer_order);
		}
		close(fb_fd);
	}
#endif
}

void RestoreLayerOrder(void)
{
#if 0
	int fb_fd = -1;
	int lcd_num = -1, rtn = -1;
	tccfb_set_wmix_order_type	wmix_order_type;

	if ((fb_fd = open(FB0_DEVICE, O_RDWR)) >= 0){
		wmix_order_type.num = g_old_layer_num;
		wmix_order_type.ovp = g_old_layer_order;

		ALOGD("[%s] wmix_num:%d, order:%d\n", __func__, g_old_layer_num, g_old_layer_order);

		rtn = ioctl(fb_fd, TCC_LCDC_SET_LAYER_ORDER, &wmix_order_type);

		close(fb_fd);
	} else {
		ALOGE("[%s] can't open '%s'", __func__, FB0_DEVICE);
	}
#endif
}
