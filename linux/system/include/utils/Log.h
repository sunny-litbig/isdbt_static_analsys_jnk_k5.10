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
#ifndef __LOG_H__
#define __LOG_H__

#ifndef LOG_NDEBUG
#ifdef NDEBUG
#define LOG_NDEBUG 1
#else
#define LOG_NDEBUG 0
#endif
#endif

#ifndef printf
#include <stdio.h>  	/* for printf() */
#endif

#include "LB_debug_log.h"


#ifndef ALOGV
#define ALOGV(msg...) LB_Debug_Message(LB_LOG_LEVEL_VERBOSE, msg)
#endif

#ifndef ALOGD
#define ALOGD(msg...) LB_Debug_Message(LB_LOG_LEVEL_DEBUG, msg)
#endif

#ifndef ALOGI
#define ALOGI(msg...) LB_Debug_Message(LB_LOG_LEVEL_INFO, msg)
#endif

#ifndef ALOGW
#define ALOGW(msg...) LB_Debug_Message(LB_LOG_LEVEL_WARN, msg)
#endif

#ifndef ALOGE
#define ALOGE(msg...) LB_Debug_Message(LB_LOG_LEVEL_ERROR, msg)
#endif


#ifndef LLOGV
#define LLOGV(msg...) LB_Debug_Message(LB_LOG_LEVEL_VERBOSE, msg)
#endif

#ifndef LLOGD
#define LLOGD(msg...) LB_Debug_Message(LB_LOG_LEVEL_DEBUG, msg)
#endif

#ifndef LLOGI
#define LLOGI(msg...) LB_Debug_Message(LB_LOG_LEVEL_INFO, msg)
#endif

#ifndef LLOGW
#define LLOGW(msg...) LB_Debug_Message(LB_LOG_LEVEL_WARN, msg)
#endif

#ifndef LLOGE
#define LLOGE(msg...) LB_Debug_Message(LB_LOG_LEVEL_ERROR, msg)
#endif

#ifndef LOG_TAG
#define LOG_TAG NULL
#endif

#endif // __LOG_H__