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
#define LOG_TAG "TsParser_Subtitle"
#include <utils/Log.h>

#define LOGE 	ALOGE
#endif

#include <stdio.h>
#include <stdarg.h>
#include "TsParser_Subtitle_Debug.h"

static unsigned int debug_level = DBG_ERR;

void subtitle_lib_set_debug_level(unsigned int level)
{
	debug_level = (level == 0)?DBG_ERR:level;
}

void LIB_DBG(const int level, const char *format, ...)
{
	va_list arg;
	char buf[256];

	if(debug_level & level){
		va_start(arg, format);
		vsprintf(buf, format, arg);
		va_end(arg);

		LOGE("%s", buf);
	}
}
