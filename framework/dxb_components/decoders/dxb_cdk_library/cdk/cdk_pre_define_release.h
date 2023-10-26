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
#ifndef _CDK_PRE_DEFINE_RELEASE_H_
#define _CDK_PRE_DEFINE_RELEASE_H_

#include "cdk_pre_define_origin.h"

#undef INCLUDE_EXT_F
#undef INCLUDE_RAG2_DEC
#undef INCLUDE_AC3_DEC
#undef INCLUDE_DTS_DEC
#undef INCLUDE_RV9_DEC

// if you wanna use a specific library, 
// remove comment at head of the correspoding line

// EXT media fileformat parser
//#define INCLUDE_EXT_F

// real audio codec
//#define INCLUDE_RAG2_DEC

// ac3 audio codec
//#define INCLUDE_AC3_DEC

// dts audio codec
//#define INCLUDE_DTS_DEC

// real video 9 video codec
//#define INCLUDE_RV9_DEC

#endif // _CDK_PRE_DEFINE_RELEASE_H_
