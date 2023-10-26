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
#define LOG_TAG	"subtitle_scaler"
#include <utils/Log.h>
#endif

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <tcc_scaler_ioctrl.h>

#include <subtitle_debug.h>

/****************************************************************************
DEFINITION
****************************************************************************/
#define GETA(c)    		(((c) & 0xFF000000) >> 24)
#define GETR(c)    		(((c) & 0xFF0000) >> 16)
#define GETG(c)    		(((c) & 0xFF00)>>8)
#define GETB(c)    		((c) & 0xFF)

#define MAKECOL(a, r, g, b)    ( ((a)<<24)|((r)<<16) | ((g)<<8) | (b))

/****************************************************************************
DEFINITION OF EXTERNAL VARIABLES
****************************************************************************/


/****************************************************************************
DEFINITION OF GLOVAL VARIABLES
****************************************************************************/


/****************************************************************************
DEFINITION OF LOCAL FUNCTIONS
****************************************************************************/
int subtitle_sw_scaler(unsigned long* srcBuf, int src_w, int src_h, unsigned long* dstBuf, int dst_w, int dst_h)
{
	register int i, j;
	register double src_dest_w = (double)(src_w - 1) / (double)(dst_w - 1);
	register double src_dest_h = (double)(src_h - 1) / (double)(dst_h - 1);
	register double ci, cj;
	register int x1, y1, x2, y2;
	register double xoff, yoff;
	register unsigned c1_rgb[4], c2_rgb[4], c3_rgb[4], c4_rgb[4];
	register unsigned a, r, g, b;

	register long *line1, *line2;
	register long *dest_line;

	cj = 0;
	for (j = 0; j < dst_h; j++)
	{
		y1 = cj;
		y2 = y1 < src_w - 1 ? y1 + 1 : src_w - 1;
		yoff = cj - y1;

		line1 = (long *)srcBuf + (src_w * y1);
		line2 = (long *)srcBuf + (src_w * y2);
		dest_line = (long *)dstBuf + (dst_w * j);

		ci = 0;
		for (i = 0; i < dst_w; i++)
		{
			x1 = ci;
			x2 = x1 < src_w - 1 ? x1 + 1 : src_w - 1;

			c1_rgb[0] = GETA(line1[x1]);
			c1_rgb[1] = GETR(line1[x1]);
			c1_rgb[2] = GETG(line1[x1]);
			c1_rgb[3] = GETB(line1[x1]);

			c2_rgb[0] = GETA(line1[x2]);
			c2_rgb[1] = GETR(line1[x2]);
			c2_rgb[2] = GETG(line1[x2]);
			c2_rgb[3] = GETB(line1[x2]);

			c3_rgb[0] = GETA(line2[x1]);
			c3_rgb[1] = GETR(line2[x1]);
			c3_rgb[2] = GETG(line2[x1]);
			c3_rgb[3] = GETB(line2[x1]);

			c4_rgb[0] = GETA(line2[x2]);
			c4_rgb[1] = GETR(line2[x2]);
			c4_rgb[2] = GETG(line2[x2]);
			c4_rgb[3] = GETB(line2[x2]);

			xoff = ci - x1;

#if 0
			a = (c1_rgb[0] * (1 - xoff) + (c2_rgb[0] * xoff)) * (1 - yoff) + (c3_rgb[0] * (1 - xoff) + (c4_rgb[0] * xoff)) * yoff;
			r = (c1_rgb[1] * (1 - xoff) + (c2_rgb[1] * xoff)) * (1 - yoff) + (c3_rgb[1] * (1 - xoff) + (c4_rgb[1] * xoff)) * yoff;
			g = (c1_rgb[2] * (1 - xoff) + (c2_rgb[2] * xoff)) * (1 - yoff) + (c3_rgb[2] * (1 - xoff) + (c4_rgb[2] * xoff)) * yoff;
			b = (c1_rgb[3] * (1 - xoff) + (c2_rgb[3] * xoff)) * (1 - yoff) + (c3_rgb[3] * (1 - xoff) + (c4_rgb[3] * xoff)) * yoff;
#elif 0
			a = (c1_rgb[0] + c2_rgb[0] + c3_rgb[0] + c4_rgb[0])>>2;
			r = (c1_rgb[1] + c2_rgb[1] + c3_rgb[1] + c4_rgb[1])>>2;
			g = (c1_rgb[2] + c2_rgb[2] + c3_rgb[2] + c4_rgb[2])>>2;
			b = (c1_rgb[3] + c2_rgb[3] + c3_rgb[3] + c4_rgb[3])>>2;
#elif 0
			a = (c1_rgb[0] * (1 - yoff)) + c3_rgb[0] * yoff;
			r = (c1_rgb[1] * (1 - yoff)) + c3_rgb[1] * yoff;
			g = (c1_rgb[2] * (1 - yoff)) + c3_rgb[2] * yoff;
			b = (c1_rgb[3] * (1 - yoff)) + c3_rgb[3] * yoff;
#else
			a = c1_rgb[0];
			r = c1_rgb[1];
			g = c1_rgb[2];
			b = c1_rgb[3];
#endif    /* End of 0 */

			dest_line[i] = MAKECOL(a, r, g, b);

			ci += src_dest_w;
		}
		cj += src_dest_h;
	}

	return 0;
}

int subtitle_hw_scaler
(
	unsigned int* srcBuf,
	int src_x, int src_y, int src_w, int src_h, int src_pw, int src_ph,
	unsigned int* dstBuf,
	int dst_x, int dst_y, int dst_w, int dst_h, int dst_pw, int dst_ph
)
{
	ALOGE("[%s]", __func__);
	int ret = -1;
	int scaler_fd = -1;
	struct SCALER_TYPE scaler_type;
	struct pollfd poll_event[1];

	if ((scaler_fd = subtitle_display_get_scaler_fd()) < 0){
		LOGE("[%s] can't open scaler driver(%d)", __func__, scaler_fd);
		goto END;
	}

	memset(&scaler_type, 0x0, sizeof(struct SCALER_TYPE));

	scaler_type.responsetype = SCALER_INTERRUPT;

	scaler_type.src_Yaddr = (char*)srcBuf;
	scaler_type.src_Uaddr = NULL;
	scaler_type.src_Vaddr = NULL;
	scaler_type.src_fmt = SCALER_ARGB8888;
	scaler_type.src_ImgWidth = src_pw;
	scaler_type.src_ImgHeight = src_ph;
	scaler_type.src_winLeft = src_x;
	scaler_type.src_winTop = src_y;
	scaler_type.src_winRight = src_x+src_w;
	scaler_type.src_winBottom = src_y+src_h;

	scaler_type.dest_Yaddr = (char*)dstBuf;
	scaler_type.dest_Uaddr = NULL;
	scaler_type.dest_Vaddr = NULL;
	scaler_type.dest_fmt = SCALER_ARGB8888;
	scaler_type.dest_ImgWidth = dst_pw;
	scaler_type.dest_ImgHeight = dst_ph;
	scaler_type.dest_winLeft = dst_x;
	scaler_type.dest_winTop = dst_y;
	scaler_type.dest_winRight = dst_x+dst_w;
	scaler_type.dest_winBottom = dst_y+dst_h;

	scaler_type.viqe_onthefly = 0;
	scaler_type.interlaced = 0;
	scaler_type.plugin_path = 0;
	scaler_type.divide_path = 0;
//	scaler_type.wdma_r2y = 0;

	ret = ioctl(scaler_fd, TCC_SCALER_IOCTRL, &scaler_type);
	if(ret != 0){
		LOGE("[%s] SCALER ERROR(%d)\n", __func__, ret);
		goto END;
	}

	memset(&poll_event[0], 0, sizeof(poll_event));

	poll_event[0].fd = scaler_fd;
	poll_event[0].events = POLLIN;

	while(1){
		ret = poll((struct pollfd*)poll_event, 1, 500 /* ms */);
		if (ret < 0){
			LOGE("[%s] scaler poll error\n", __func__);
			break;
		}else if (ret == 0){
			LOGE("[%s] scaler poll timeout\n", __func__);
			break;
		}else if (ret > 0){
			if (poll_event[0].revents & POLLERR){
				LOGE("[%s] scaler POLLERR\n", __func__);
				break;
			}else if (poll_event[0].revents & POLLIN){
				ret = 0;
				break;
			}
		}
	}

END:
	if(scaler_fd>=0) close(scaler_fd);

	return ret;
}
