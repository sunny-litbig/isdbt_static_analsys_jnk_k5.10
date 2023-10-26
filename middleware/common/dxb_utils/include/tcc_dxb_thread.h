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
#ifndef		_TCC_DXB_THREAD_H_
#define		_TCC_DXB_THREAD_H_

#ifdef __cplusplus
extern    "C"
{
#endif

typedef enum
{
	HIGH_PRIORITY_0 = 20,	//highest
	HIGH_PRIORITY_1,
	HIGH_PRIORITY_2,
	HIGH_PRIORITY_3,
	HIGH_PRIORITY_4,
	HIGH_PRIORITY_5,
	HIGH_PRIORITY_6,
	HIGH_PRIORITY_7,
	HIGH_PRIORITY_8,
	HIGH_PRIORITY_9,
	HIGH_PRIORITY_10,
	LOW_PRIORITY_0,
	LOW_PRIORITY_1,
	LOW_PRIORITY_2,
	LOW_PRIORITY_3,
	LOW_PRIORITY_4,
	LOW_PRIORITY_5,
	LOW_PRIORITY_6,
	LOW_PRIORITY_7,
	LOW_PRIORITY_8,
	LOW_PRIORITY_9,
	LOW_PRIORITY_10,
	LOW_PRIORITY_11,		//lowest
	PREDEF_PRIORITY			//determine by name
}E_TCC_DXB_THREAD_PRIORITY;


typedef	void* (*pThreadFunc)(void *);
int tcc_dxb_thread_create(void *pHandle, pThreadFunc Func, unsigned char *pucName, E_TCC_DXB_THREAD_PRIORITY ePriority, void *arg);
int tcc_dxb_thread_join(void *pHandle, void **pThreadRet);

#ifdef __cplusplus
}
#endif

#endif
