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
#ifndef __TCCXXX_IMAGE_CUSTOM_OUTPUT_SET_H__
#define __TCCXXX_IMAGE_CUSTOM_OUTPUT_SET_H__

typedef struct {
	int Comp_1;
	int Comp_2;
	int Comp_3;
	int Comp_4;		//V.1.52~
	int Offset;
	int x;
	int y;
	int Src_Fmt;
}IM_PIX_INFO;

#define IM_SRC_YUV		0
#define IM_SRC_RGB		1
#define IM_SRC_OTHER	2

extern unsigned int	IM_2nd_Offset;
extern unsigned int	IM_3rd_Offset;
extern unsigned int	IM_LCD_Half_Stride;
extern unsigned int	IM_LCD_Addr;

void TCC_CustomOutput_RGB565(IM_PIX_INFO out_info);
void TCC_CustomOutput_RGB888_With_Alpha(IM_PIX_INFO out_info);
void TCC_CustomOutput_RGB888(IM_PIX_INFO out_info);
void TCC_CustomOutput_YUV420(IM_PIX_INFO out_info);
void TCC_CustomOutput_YUV444(IM_PIX_INFO out_info);

void TCC_CustomOutput_RGB888_With_Alpha_ISDBT(IM_PIX_INFO out_info); //PNGDEC_SUPPORT_ISDBT_PNG

#endif //__TCCXXX_IMAGE_CUSTOM_OUTPUT_SET_H__
