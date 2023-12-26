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
#define LOG_TAG	"TCC_THREAD"

#include <pthread.h>
#include <stdlib.h>
#include <utils/Log.h>
#include "tcc_dxb_thread.h"

//#define	 NOPRIORITY_SUPPORT
#define	 DBG_PRINTF		ALOGD	

typedef struct TCC_DXB_THREAD_COUSTOM_DEF
{
	unsigned char *pucname;
	int sched_type;
	E_TCC_DXB_THREAD_PRIORITY epriority;
} ST_TCC_DXB_THREAD_COUSTOM_DEF;


ST_TCC_DXB_THREAD_COUSTOM_DEF stTCCThreadTable[] = 
{
	{	"default"      ,					SCHED_RR,	LOW_PRIORITY_11		},		
	{	"OMX.tcc.broadcast.tuner.atsc",		SCHED_RR,	LOW_PRIORITY_11		},
	{	"OMX.tcc.broadcast.dvbt",			SCHED_RR,	LOW_PRIORITY_11		},
	{	"OMX.tcc.broadcast.isdbt",			SCHED_RR,	LOW_PRIORITY_11		},	
	{	"OMX.tcc.broadcast.demux.atsc",		SCHED_FIFO,	HIGH_PRIORITY_0		},
	{	"OMX.tcc.broadcast.dxb_demux",		SCHED_FIFO,	HIGH_PRIORITY_0		},
	{	"OMX.tcc.broadcast.iptv_demux",		SCHED_FIFO,	HIGH_PRIORITY_0		},
	{	"OMX.tcc.broadcast.section",		SCHED_RR,	HIGH_PRIORITY_7		},
	{	"OMX.tcc.video_decoder",			SCHED_RR,	HIGH_PRIORITY_7		},
	{	"OMX.tcc.audio_decoder",			SCHED_RR,	HIGH_PRIORITY_7		},
	{	"OMX.TCC.mpeg2dec",					SCHED_RR,	HIGH_PRIORITY_7		},
	{	"OMX.TCC.avcdec",					SCHED_RR,	HIGH_PRIORITY_7		},
	{	"OMX.TCC.mp2dec",					SCHED_RR,	HIGH_PRIORITY_7		},
	{	"OMX.TCC.aacdec",					SCHED_RR,	HIGH_PRIORITY_7		},
	{	"OMX.TCC.ac3dec",					SCHED_RR,	HIGH_PRIORITY_7		},
	{	"OMX.tcc.fbdev.fbdev_sink",			SCHED_RR,	HIGH_PRIORITY_7		},
	{	"OMX.tcc.alsa.alsasink",			SCHED_RR,	HIGH_PRIORITY_7		},
	{	NULL,								0,			0 					},
};


int tcc_dxb_thread_create(void *pHandle, pThreadFunc Func, unsigned char *pucName, E_TCC_DXB_THREAD_PRIORITY ePriority, void *arg)
{
	int i, ret = 0;
	int iSchedType, iPriorityOffset, iMaxPriority;
	struct sched_param param;
	pthread_attr_t p_tattr;
	
#ifdef		NOPRIORITY_SUPPORT
	return pthread_create(pHandle, NULL, Func,  arg);
#else

	if(pucName == NULL)
	{
		DBG_PRINTF("[%s] pucName is NULL Error !!!!!\n", __func__);
		return -1;
	}

	iSchedType = -1;
	if( ePriority >= PREDEF_PRIORITY )
		ePriority = HIGH_PRIORITY_9;
	
	for( i=0; stTCCThreadTable[i].pucname != NULL; i++ )
	{
		if( !strcmp(stTCCThreadTable[i].pucname, pucName) )
		{				
			iPriorityOffset = stTCCThreadTable[i].epriority;	
			iSchedType = stTCCThreadTable[i].sched_type;	
			DBG_PRINTF("%s %d %d", pucName, ePriority, iSchedType);
			break;
		}
	}
	
	if(iSchedType == -1)
	{
		if(ePriority >= LOW_PRIORITY_0 )
		{
			#if 1
			iSchedType = SCHED_OTHER;		
			//iPriorityOffset = ePriority-LOW_PRIORITY_0;		
			iPriorityOffset = 0;
			#else
			iSchedType = SCHED_RR;
			iPriorityOffset = ePriority;
			#endif
		}
		else
		{
			iSchedType = SCHED_RR;
			iPriorityOffset = ePriority;
		}	
	}	
	/* initialized with default attributes */
	ret |= pthread_attr_init (&p_tattr);
	
	//ret |= pthread_attr_setinheritsched(p_tattr, PTHREAD_EXPLICIT_SCHED);  
	ret |= pthread_attr_setscope(&p_tattr, PTHREAD_SCOPE_SYSTEM);	
	ret |= pthread_attr_setschedpolicy(&p_tattr, iSchedType);
	/* safe to get existing scheduling param */
	ret |= pthread_attr_getschedparam (&p_tattr, &param);
	
	/* set the priority; others are unchanged */
	iMaxPriority = sched_get_priority_max(iSchedType);
	if( iMaxPriority > iPriorityOffset )
		param.sched_priority = iMaxPriority - iPriorityOffset;
	else
		param.sched_priority = iMaxPriority;
	//ALOGE("MAX %d,Offset %d, Type %d", sched_get_priority_max(iSchedType), iPriorityOffset, iSchedType);
	/* setting the new scheduling param */
	ret |= pthread_attr_setschedparam (&p_tattr, &param);
	if(ret != 0)
	{
		DBG_PRINTF("%s::%d thread(%s) error %d", __func__,__LINE__, pucName, ret);
		return ret;
	}
	
	/* Original 
		ret = pthread_create(pHandle, &p_tattr, Func,  arg);	
	*/
	//8th Try
	//David, To avoid Codesonar's warning, Dangerous Function Cast
	//Add implicit-casting
	ret = pthread_create(pHandle, &p_tattr, (pThreadFunc)Func,	arg);	
	if(ret != 0)
	{
		DBG_PRINTF("%s::%d thread(%s) error %d", __func__,__LINE__, pucName, ret);
		return ret;
	}
	i = strlen (pucName);
	if (i > 0) {
		char *pName = pucName;
		if (i > 15) 
			pName += (i-15);
		pthread_setname_np (*(pthread_t *)pHandle, pName);
	}
	
	DBG_PRINTF("%s::%s Success!! Priority(%d)Type(%d)", __func__, pucName, param.sched_priority, iSchedType);
	return ret;
#endif	
}

int tcc_dxb_thread_join(void *pHandle, void **pThreadRet)
{
	return pthread_join(pHandle, pThreadRet);
}
