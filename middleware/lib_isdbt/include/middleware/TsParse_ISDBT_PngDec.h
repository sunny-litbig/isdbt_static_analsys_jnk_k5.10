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
#ifndef __TSPARSE_ISDBT_PNGDEC_H__
#define __TSPARSE_ISDBT_PNGDEC_H__

#include <TCCXXX_PNG_DEC.h>
#include <TCCXXX_IMAGE_CUSTOM_OUTPUT_SET_ISDBT.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ConvertARGB4444toRGBA4444(a) (((a&0xf000)>>12)|((a&0x0fff)<<4))
#define ConvertARGB8888toARGB4444(a) (((a&0xf0000000)>>16)|((a&0xf00000)>>12)|((a&0xf000)>>8)|((a&0xf0)>>4))
#define ConvertARGB8888toRGBA4444(a) (((a&0xf00000)>>8)|((a&0xf000)>>4)|((a&0xf0)>>0)|((a&0xf0000000)>>28))

typedef enum{
	ISDBT_PNGDEC_ARGB4444,
	ISDBT_PNGDEC_RGBA4444,
	ISDBT_PNGDEC_ARGB8888,
	ISDBT_PNGDEC_MAX
}ISDBT_PNGDEC_FORMAT;
	
typedef struct{
	char *pBuf;
	int total_size;
	int cur_pos;
}FILE_CTX;

extern int tcc_isdbt_png_init(void *pSrc, int SrcBufSize, unsigned int *pWidth, unsigned int *pHeight);
extern int tcc_isdbt_png_dec(void *pSrc, int SrcBufSize, void *pDst, int dst_w_pitch, int png_x, int png_y, int png_w, int png_h, int format, int cc, int logo);

#ifdef __cplusplus
}
#endif

#endif	/* __TSPARSE_ISDBT_PNGDEC_H__ */	