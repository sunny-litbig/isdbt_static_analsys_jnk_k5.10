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
#ifndef _AENC_VARS_H_
#define _AENC_VARS_H_

#include "../audio_codec/aenc.h"

typedef struct _AENC_VARS
{
	cdk_audio_func_t* gspfAEnc;
	aenc_input_t gsAEncInput;
	aenc_output_t gsAEncOutput;
	aenc_init_t gsAEncInit;
	unsigned char *gpucStreamBuffer;
	unsigned char *gpucPcmBuffer;
	unsigned int guiRecTime;
	unsigned int guiTimePos;
	TCAS_CDSyncInfo	gCDSync;
} AENC_VARS;

#endif // _AENC_VARS_H_
