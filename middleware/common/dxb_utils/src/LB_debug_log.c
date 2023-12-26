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
#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include "LB_debug_log.h"


#define LB_MAX_LOG_MSG_SIZE 256
static char g_LB_logMsg[ LB_MAX_LOG_MSG_SIZE ] = {0, };
static pthread_mutex_t g_LB_logMutex;
static LB_LOG_TYPE g_LB_logType = LB_LOG_TYPE_ALL_ON;


void LB_Debug_InitLog(void)
{
	pthread_mutex_init(&g_LB_logMutex, 0);
}

void LB_Debug_DeinitLog(void)
{
	pthread_mutex_destroy(&g_LB_logMutex);
}

void LB_Debug_SetLogInfo(unsigned int _logType)
{
	g_LB_logType = _logType;
}

unsigned int LB_Debug_GetLogInfo(void)
{
	return g_LB_logType;
}

void LB_Debug_Message(LB_LOG_LEVEL _logLevel, const char* _format, ...)
{
	pthread_mutex_lock(&g_LB_logMutex);
		
	va_list vaList;

	if( !(LB_LOG_LEVEL_VERBOSE <= _logLevel && _logLevel <= LB_LOG_LEVEL_ERROR) )
	{
		pthread_mutex_unlock(&g_LB_logMutex);
		return;
	}

	switch(g_LB_logType)
	{
		case LB_LOG_TYPE_ALL_ON:
			break;
			
		case LB_LOG_TYPE_IWE_ON:
			if(_logLevel == LB_LOG_LEVEL_VERBOSE || _logLevel == LB_LOG_LEVEL_DEBUG)
			{
				pthread_mutex_unlock(&g_LB_logMutex);
				return;
			}
			break;
			
		case LB_LOG_TYPE_E_ON:
			if( !(_logLevel == LB_LOG_LEVEL_ERROR) )
			{
				pthread_mutex_unlock(&g_LB_logMutex);
				return;
			}
			break;
			
		case LB_LOG_TYPE_ALL_OFF:
		default:
			pthread_mutex_unlock(&g_LB_logMutex);
			return;
	}
	
	va_start(vaList, _format);
	vsnprintf(g_LB_logMsg, LB_MAX_LOG_MSG_SIZE, _format, vaList);

	printf("%s\n", g_LB_logMsg);
	
	va_end(vaList);

	pthread_mutex_unlock(&g_LB_logMutex);
}

