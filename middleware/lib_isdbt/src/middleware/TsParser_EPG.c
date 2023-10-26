/*******************************************************************************

*   FileName : TsParser_EPG.c

*   Copyright (c) Telechips Inc.

*   Description : TsParser_EPG.c

********************************************************************************
*
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
#define LOG_NDEBUG 0
#define LOG_TAG	"TsParser_EPG"
#include <utils/Log.h>
#endif

#include <stdio.h>
#include <string.h>
#include "ISDBT_EPG.h"

#define PRINTF		//ALOGE
#define LOGE		ALOGE

L_EIT_EPG_LIST_h *	g_pEventList = NULL;

int TC_Deallocate_Memory(void *p)
{
	tcc_mw_free(__FUNCTION__, __LINE__, p);
	return 1;
}

void ISDBT_EPG_FreeEventList_h(L_EIT_EPG_LIST_h * pEpgList_h)
{
	L_EIT_EPG_NODE* pEpgNode = NULL;
	unsigned short usCurEventID = 0;
	unsigned char ucCurVersion = 0;
	short sLEITEventCnt = MAX_L_EIT_NUM;

	if (pEpgList_h == NULL) {
		LOGE("[%s] Cannot find a pointer of EPG LIST Root !!\n", __func__);
		return;
	}

	ucCurVersion = pEpgList_h->version;

	while (pEpgList_h->pHead != NULL)
	{
		pEpgNode = pEpgList_h->pHead;
		usCurEventID = pEpgNode->eventID;

		pEpgList_h->pHead = pEpgList_h->pHead->pNext;

		if (pEpgNode->pEventName)
		{
			TC_Deallocate_Memory(pEpgNode->pEventName);
			pEpgNode->pEventName = NULL;
		}

		if (pEpgNode->pEventText)
		{
			TC_Deallocate_Memory(pEpgNode->pEventText);
			pEpgNode->pEventText = NULL;
		}

		TC_Deallocate_Memory(pEpgNode);
		pEpgNode = NULL;

		/* If loop is executed over the 10 times, escape the loop */
		if (sLEITEventCnt-- == 0)
			break;
	}

	/* mem free to EPG service name */
	TC_Deallocate_Memory(pEpgList_h->pServiceName);
	pEpgList_h->pServiceName = NULL;

	/* mem free to EPG TS name */
	TC_Deallocate_Memory(pEpgList_h->pTSName);
	pEpgList_h->pTSName = NULL;

	/* mem free to EPG root pointer */
	TC_Deallocate_Memory(pEpgList_h);

	PRINTF("[EPG version:0x%02x] mem free pEpgList_h\n", ucCurVersion);
}

void * ISDBT_EPG_GetEventDBRoot(void)
{
	return (void *)g_pEventList;
}

void ISDBT_TIME_GetRealDate(DATE_TIME_T *pRealDate, unsigned int  MJD)
{
	int y_dash, m_dash, k;

	y_dash = (MJD * 100 - 1507820) / 36525;
	m_dash = (MJD * 10000 - 149561000 - INT(y_dash * 3652500, 10000)) / 306001;
	pRealDate->day = (MJD * 10000 - 149560000 - INT(y_dash * 3652500, 10000) - INT(m_dash * 306001, 10000)) / 10000;

	if (m_dash == 14 || m_dash == 15)
		k = 1;
	else
		k = 0;

	pRealDate->year = y_dash + k + 1900;
	pRealDate->month = m_dash - 1 - k * 12;
	pRealDate->weekday = ((MJD + 2) % 7) + 1;
}

/* End of "ISDBT_EPG.c" File */

