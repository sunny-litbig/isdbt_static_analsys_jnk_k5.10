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

#ifndef __SUBTITLE_DEBUG_H__
#define __SUBTITLE_DEBUG_H__

#define LOGE		ALOGE
#define LOGD		ALOGD
#define LOGI		ALOGI
#define LOGW		ALOGW
#define LOGV		ALOGV

#define CC_PRINTF	//LOGE
#define ERR_DBG		LOGE

//#define SUBTITLE_DATA_DUMP


#if defined(SUBTITLE_DATA_DUMP)
void dump_data_group(TS_PES_CAP_DATAGROUP	*ptrCurrDGroup, int iDGroupNum)
{
	TS_PES_CAP_DATAUNIT 	*ptrCurrDUnit, *ptrNextDUnit;
	int j, iDataUnitSize;
	char szDumpBuf[100]={0,}, szDummy[5]={0,};
	
	while (ptrCurrDGroup != NULL)
	{
		if (iDGroupNum == 0)
		{
			break;
		}
		iDGroupNum--;

		/* Check a caption satatement data */
		if (ptrCurrDGroup->DState.DUnit != NULL)
		{
			ptrCurrDUnit = (TS_PES_CAP_DATAUNIT *)ptrCurrDGroup->DState.DUnit;
			while (ptrCurrDUnit != NULL)
			{
				ptrNextDUnit = (TS_PES_CAP_DATAUNIT *)ptrCurrDUnit->ptrNextDUnit;
				iDataUnitSize = (int)ptrCurrDUnit->data_unit_size;

				LOGE("===================================================\n");
				LOGE("_DATA_DUMP_OF_DATA_UNIT(STATEMENT:%d)\n", iDataUnitSize);
				for (j = 0; j < ptrCurrDUnit->data_unit_size; j++)
				{
					if((j!=0)&&((j%16)==0)){
						LOGE("%s\n", szDumpBuf);
						memset(szDumpBuf, 0x0, 100);
					}
					sprintf(szDummy, "0x%02X ", ptrCurrDUnit->pData[j]);
					strcat(szDumpBuf, szDummy);
					
				}
				if(iDataUnitSize%16){
					LOGE("%s\n", szDumpBuf);
				}
				LOGE("\n===================================================\n");

				ptrCurrDUnit = ptrNextDUnit;
			}
		}

		/* Check a caption management data. */
		if (ptrCurrDGroup->DMnge.DUnit != NULL)
		{
			ptrCurrDUnit = (TS_PES_CAP_DATAUNIT *)ptrCurrDGroup->DMnge.DUnit;
			while (ptrCurrDUnit != NULL)
			{
				ptrNextDUnit = (TS_PES_CAP_DATAUNIT *)ptrCurrDUnit->ptrNextDUnit;
				iDataUnitSize = (int)ptrCurrDUnit->data_unit_size;
				
				LOGE("===================================================\n");
				LOGE("_DATA_DUMP_OF_DATA_UNIT(MANAGEMENT:%d)\n", iDataUnitSize);
				for (j = 0; j < ptrCurrDUnit->data_unit_size; j++)
				{
					if((j!=0)&&((j%16)==0)){
						LOGE("%s\n", szDumpBuf);
						memset(szDumpBuf, 0x0, 100);
					}
					sprintf(szDummy, "0x%02X ", ptrCurrDUnit->pData[j]);
					strcat(szDumpBuf, szDummy);
				}
				if(iDataUnitSize%16){
					LOGE("%s\n", szDumpBuf);
				}
				LOGE("\n===================================================\n");
				
				ptrCurrDUnit = ptrNextDUnit;
			}
		}

		ptrCurrDGroup = (TS_PES_CAP_DATAGROUP *)ptrCurrDGroup->ptrNextDGroup;
	}
}

void dump_data_unit(char *pType, TS_PES_CAP_DATAUNIT *ptrCurrDUnit, int iDataUnitSize)
{
	char szDumpBuf[100]={0,}, szDummy[5]={0,};
	int j, cnt=0;
	
	LOGE("===================================================\n");
	LOGE("_DATA_DUMP_OF_DATA_UNIT_____%s______(0x%02X:%d)", \
		pType, ptrCurrDUnit->data_unit_parameter, iDataUnitSize);

	cnt = (iDataUnitSize/16);
	if(iDataUnitSize%16){
		cnt++;
	}
	
	for (j = 0; j < iDataUnitSize; j++)
	{
		if((j!=0)&&((j%16)==0)){
			LOGE("%s\n", szDumpBuf);
			memset(szDumpBuf, 0x0, 100);
			cnt--;
		}
		sprintf(szDummy, "0x%02X ", ptrCurrDUnit->pData[j]);
		strcat(szDumpBuf, szDummy);
	}
	if(cnt){
		LOGE("%s\n", szDumpBuf);
	}
	LOGE("\n===================================================\n");
}
#endif /* SUBTITLE_DATA_DUMP */

#endif	/* __SUBTITLE_DEBUG_H__ */