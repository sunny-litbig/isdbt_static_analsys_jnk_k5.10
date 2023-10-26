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
typedef void* TCC_VIDEO_UTILS;

TCC_VIDEO_UTILS TCCDxB_ScalerOpen(void);
void TCCDxB_ScalerClose(TCC_VIDEO_UTILS hUtils);
int TCCDxB_ScalerCopyData(TCC_VIDEO_UTILS hUtils, unsigned int width, unsigned int height, unsigned char *YSrc, unsigned char *USrc, unsigned char *VSrc,
								char bSrcYUVInter, unsigned char *addrDst, unsigned char ignoreAligne, int fieldInfomation, int interlaceMode);
int TCCDxB_CaptureImage(TCC_VIDEO_UTILS hUtils, unsigned int width, unsigned int height, int *yuvbuffer, int format, int interlaceMode, int fieldInfomation, unsigned char *strFilePath, int useMali);

