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
#define LOG_TAG "TSPARSE_EPG_DB"
#include <utils/Log.h>
#endif

#ifdef HAVE_LINUX_PLATFORM
#include <string.h> /* for memset(), memcpy(), strlen()*/
#include <unistd.h> /* for usleep()*/
#endif

#include "TsParser_Subtitle.h"
#include "TsParse_ISDBT_DBLayer.h"
#include "TsParse_ISDBT_EPGDb.h"
#include "tcc_isdbt_manager_demux.h"
#include "ISDBT_Common.h"
#include "subtitle_draw.h"
#include "tcc_msg.h"
#include "tcc_dxb_memory.h"

#define MAX_EPG_CONV_TXT_BYTE 512
#define MAX_EPG_CONV_EXT_EVT_TXT_BYTE (4096 + MAX_EPG_CONV_TXT_BYTE)

#define DEBUG_MSG ALOGD

typedef struct _ST_EPG_EXTEVTDESCR
{
	EXT_EVT_DESCR stEED;
	_ST_EPG_EXTEVTDESCR *pNext;
} ST_EPG_EXTEVTDESCR;

typedef struct
{
	unsigned short usUpdateFlag;
	unsigned short usItemLen;
	unsigned char *pucItem;
	unsigned short usTextLen;
	unsigned char *pucText;
	ST_EPG_EXTEVTDESCR *pstList;
} ST_EPG_EXTEVT;

typedef enum
{
	EPG_UPDATE_EVENT		= 0x80,
	EPG_UPDATE_EVTGROUP		= 0x40,
	EPG_UPDATE_CONTENT		= 0x20,
	EPG_UPDATE_EXTEVENT		= 0x10,
	EPG_UPDATE_AUDIOINFO	= 0x08,
	EPG_UPDATE_VIDEOINFO	= 0x04,
	EPG_UPDATE_RATING		= 0x02,
	EPG_UPDATE_NONE			= 0x0
}E_EPG_UPDATE;

typedef struct
{
	E_EPG_UPDATE eUpdateFlag;
	unsigned int uiUpdatedFlag;

	unsigned int uiTableId;
	unsigned int uiCurrentChannelNumber;
	unsigned int uiCurrentCountryCode;
	unsigned int ucVersionNumber;
	unsigned int ucSection;
	unsigned int ucLastSection;
	unsigned int ucSegmentLastSection;
	unsigned int OrgNetworkID;
	unsigned int TStreamID;
	unsigned int usServiceID;
	unsigned int EventID;
	unsigned int Start_MJD;
	unsigned int Start_HH;
	unsigned int Start_MM;
	unsigned int Start_SS;
	unsigned int Duration_HH;
	unsigned int Duration_MM;
	unsigned int Duration_SS;
	unsigned int iLen_EvtName;
	unsigned char *EvtName;
	unsigned int iLen_EvtText;
	unsigned char *EvtText;
	ST_EPG_EXTEVT stExtEvt;
	unsigned int iGenre;
	unsigned int iRating;
	unsigned int iAudioType;
	unsigned int iAudioLang1;
	unsigned int iAudioLang2;
	unsigned int iAudioSr;
	unsigned int iVideoInfo;
	int iReferenceServiceID;
	int iReferenceEventID;
	unsigned int MultiOrCommonEvent;

	// Noah, 20180611, Memory Leak / DUMP_13SEG551143_0828_120518.ts, 19:40 ~ End
	unsigned int is_EvtName_MustBeFreed;    // 0 : NO free, 1 : Must be freed
	// Noah, 20180611, Memory Leak / 521143_20180529_0942.ts
	unsigned int is_EvtText_MustBeFreed;    // 0 : NO free, 1 : Must be freed
}ST_EPG_DATA;

/*   <Area_code: State Specification>
	[Remark 1] If you want to know each name matched by state ID,
  		Refer to the table 1 in Annex F of SBTVD Standard N03.
  	[Remark 2] All of the string codes below are completed by following the ISO/IEC 8859-15. */
#define SBTVD_STATE_NUM_MAX		27

/*   <Area_code: Micro-region Specification>
	[Remark 1] If you want to know each name matched by micro-region ID (per each state ID),
  		Refer to the document downloadable from :
  		ftp://geoftp.ibge.gov.br/Organizacao/Divisao_Territorial/2007/
  	[Remark 2] All of the string codes below are completed by following the ISO/IEC 8859-15. */
#define SBTVD_STATE13_REGION_NUM_MAX	19

const unsigned short SBTVD_TimeZone_RegionID_Pool[SBTVD_STATE_NUM_MAX] =
{
	4, 6, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 0, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 5, 5, 3, 3
};

/* State ID #13: Pernambuco */
const unsigned short SBTVD_TimeZone_State13_RegionID_Pool[SBTVD_STATE13_REGION_NUM_MAX] =
{
//	         4           8       11       14       17       		// Region ID : 2
	1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1
};

static TccMessage *gpEPGDB_PF = NULL, *gpEPGDB_Schedule = NULL;
static unsigned char *gpEPGDB_ExtEvtItemDescr = NULL, *gpEPGDB_ExtEvtItem = NULL, *gpEPGDB_ExtEvtText = NULL, *gpEPGDB_ExtEvtConv = NULL;

static TccMessage* TCCEPGDB_GetEPGDB(int type)
{
	if (type == EPG_PF)
		return gpEPGDB_PF;
	else
		return gpEPGDB_Schedule;
}

static int TCCEPGDB_MergeExtEvt(ST_EPG_EXTEVT *pstExtEvt, EXT_EVT_DESCR *pstEED)
{
	ST_EPG_EXTEVTDESCR *pstExtEvtDescrList;
	EXT_EVT_STRUCT *pstEESList;
	unsigned short usItemDescrLen = 0;
	unsigned short usItemLen = 0;
	unsigned short usTextLen = 0;
	unsigned short usConvLen = 0;
	int i;

	// check
	for(i=0; i<=pstEED->ucLastDescrNumber; i++)
	{
		if(((1<<i) & (pstExtEvt->usUpdateFlag)) == 0) {
			//ALOGD("%s::%d Wait more descriptor(%d, %d)!!", __func__, __LINE__, pstExtEVT->usUpdateFlag, pstEED->ucLastDescrNumber);
			return EPG_UPDATE_NONE;
		}
	}

	// merge
	memset(gpEPGDB_ExtEvtItemDescr, 0, MAX_EPG_CONV_TXT_BYTE);
	memset(gpEPGDB_ExtEvtItem, 0, MAX_EPG_CONV_EXT_EVT_TXT_BYTE);
	memset(gpEPGDB_ExtEvtText, 0, MAX_EPG_CONV_EXT_EVT_TXT_BYTE);
	memset(gpEPGDB_ExtEvtConv, 0, MAX_EPG_CONV_EXT_EVT_TXT_BYTE);

	pstExtEvtDescrList = pstExtEvt->pstList;
	while(pstExtEvtDescrList)
	{
		pstEESList = pstExtEvtDescrList->stEED.pstExtEvtList;
		while(pstEESList)
		{
			if (pstEESList->ucExtEvtItemDescrLen > 0) {
				if(usItemDescrLen > 0) {
					gpEPGDB_ExtEvtItemDescr[usItemDescrLen++] = 0x0D; //APR
					usConvLen += ISDBT_EPG_DecodeCharString(gpEPGDB_ExtEvtItemDescr, &gpEPGDB_ExtEvtConv[usConvLen], usItemDescrLen, Get_Language_Code());

					memset(gpEPGDB_ExtEvtItemDescr, 0, MAX_EPG_CONV_TXT_BYTE);
					usItemDescrLen = 0;
				}

				if (usItemLen > 0) {
					gpEPGDB_ExtEvtItem[usItemLen++] = 0x0D; //APR
					usConvLen += ISDBT_EPG_DecodeCharString(gpEPGDB_ExtEvtItem, &gpEPGDB_ExtEvtConv[usConvLen], usItemLen, Get_Language_Code());

					memset(gpEPGDB_ExtEvtItem, 0, MAX_EPG_CONV_EXT_EVT_TXT_BYTE);
					usItemLen = 0;
			}

				memcpy(&gpEPGDB_ExtEvtItemDescr[usItemDescrLen], pstEESList->acExtEvtItemDescrName, pstEESList->ucExtEvtItemDescrLen);
				usItemDescrLen = pstEESList->ucExtEvtItemDescrLen;
			}

			if (pstEESList->ucExtEvtItemLen > 0) {
				memcpy(&gpEPGDB_ExtEvtItem[usItemLen], pstEESList->acExtEvtItemName, pstEESList->ucExtEvtItemLen);
				usItemLen += pstEESList->ucExtEvtItemLen;
			}

			pstEESList = pstEESList->pstExtEvtStruct;
		}

		if (pstExtEvtDescrList->stEED.ucExtEvtTextLen > 0) {
			memcpy(&gpEPGDB_ExtEvtText[usTextLen], pstExtEvtDescrList->stEED.acExtEvtTextName, pstExtEvtDescrList->stEED.ucExtEvtTextLen);
			usTextLen += pstExtEvtDescrList->stEED.ucExtEvtTextLen;;
		}

		pstExtEvtDescrList = pstExtEvtDescrList->pNext;
	}

	if(usItemDescrLen > 0) {
		gpEPGDB_ExtEvtItemDescr[usItemDescrLen++] = 0x0D; //APR
		usConvLen += ISDBT_EPG_DecodeCharString(gpEPGDB_ExtEvtItemDescr, &gpEPGDB_ExtEvtConv[usConvLen], usItemDescrLen, Get_Language_Code());
	}

	if (usItemLen > 0) {
		gpEPGDB_ExtEvtItem[usItemLen++] = 0x0D; //APR
		usConvLen += ISDBT_EPG_DecodeCharString(gpEPGDB_ExtEvtItem, &gpEPGDB_ExtEvtConv[usConvLen], usItemLen, Get_Language_Code());
	}

	if (usConvLen > 0) {
		if (((usConvLen+1)*sizeof(short)) < MAX_EPG_CONV_TXT_BYTE) {
			pstExtEvt->pucItem = (unsigned char*)tcc_mw_malloc(__FUNCTION__, __LINE__, MAX_EPG_CONV_TXT_BYTE);
		} else {
			pstExtEvt->pucItem = (unsigned char*)tcc_mw_malloc(__FUNCTION__, __LINE__, (usConvLen+1)*sizeof(short));
		}

		if (pstExtEvt->pucItem) {
			memset(pstExtEvt->pucItem, 0, (usConvLen+1)* sizeof(short));
			pstExtEvt->usItemLen = ISDBT_EPG_ConvertCharString(gpEPGDB_ExtEvtConv, usConvLen, (unsigned short*)pstExtEvt->pucItem);
		}
	}

	if (usTextLen > 0) {
		memset(gpEPGDB_ExtEvtConv, 0, MAX_EPG_CONV_EXT_EVT_TXT_BYTE);
		usConvLen = ISDBT_EPG_DecodeCharString(gpEPGDB_ExtEvtText, gpEPGDB_ExtEvtConv, usTextLen, Get_Language_Code());

		if (((usConvLen+1)*sizeof(short)) < MAX_EPG_CONV_TXT_BYTE) {
			pstExtEvt->pucText = (unsigned char*)tcc_mw_malloc(__FUNCTION__, __LINE__, MAX_EPG_CONV_TXT_BYTE);
		} else {
			pstExtEvt->pucText = (unsigned char*)tcc_mw_malloc(__FUNCTION__, __LINE__, (usConvLen+1)*sizeof(short));
		}

		if (pstExtEvt->pucText) {
			memset(pstExtEvt->pucText, 0, (usConvLen+1)* sizeof(short));
			pstExtEvt->usTextLen = ISDBT_EPG_ConvertCharString(gpEPGDB_ExtEvtConv, usConvLen, (unsigned short*)pstExtEvt->pucText);
		}
	}

	return EPG_UPDATE_EXTEVENT;
}

static int TCCEPGDB_AddExtEvtDescr(ST_EPG_EXTEVT *pstExtEvt, EXT_EVT_DESCR *pstEED)
{
	EXT_EVT_STRUCT *pstEESList = NULL, *pstEESPrev = NULL, *pstEES = NULL;
	ST_EPG_EXTEVTDESCR *pstExtEvtDescrList = NULL, *pstExtEvtDescrListPrev = NULL, *pstExtEvtDescr = NULL;

	if ((1<<pstEED->ucDescrNumber) & (pstExtEvt->usUpdateFlag)) {
		//LOGE("%s::%d Error ExtEvtDescr already updated(%d, %d)!", __func__, __LINE__, pstEED->ucDescrNumber, pstExtEvt->usUpdateFlag);
		return EPG_UPDATE_NONE;
	}

	// make ST_EPG_EXTEVTDESCR
	pstExtEvtDescr = (ST_EPG_EXTEVTDESCR*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_EPG_EXTEVTDESCR));
	memcpy(&pstExtEvtDescr->stEED, pstEED, sizeof(EXT_EVT_DESCR));
	pstExtEvtDescr->stEED.pstExtEvtList = NULL;
	pstExtEvtDescr->pNext = NULL;

	pstEESList = pstEED->pstExtEvtList;
	while(pstEESList)
	{
		pstEES = (EXT_EVT_STRUCT*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(EXT_EVT_STRUCT));
		memcpy(pstEES, pstEESList, sizeof(EXT_EVT_STRUCT));
		pstEES->pstExtEvtStruct = NULL;

		if (pstExtEvtDescr->stEED.pstExtEvtList == NULL) {
			pstExtEvtDescr->stEED.pstExtEvtList = pstEES;
		} else {
			pstEESPrev->pstExtEvtStruct = pstEES;
		}

		pstEESPrev = pstEES;
		pstEESList = pstEESList->pstExtEvtStruct;
	}

	// insert ST_EPG_EXTEVTDESCR
	if (pstExtEvt->pstList == NULL)	{
		pstExtEvt->pstList = pstExtEvtDescr;
	} else {
		pstExtEvtDescrList = pstExtEvt->pstList;
		while(pstExtEvtDescrList)
		{
			if (pstExtEvtDescrList->stEED.ucDescrNumber > pstExtEvtDescr->stEED.ucDescrNumber) {
				if (pstExtEvtDescrList == pstExtEvt->pstList) {
					pstExtEvt->pstList = pstExtEvtDescr;
					pstExtEvtDescr->pNext = pstExtEvtDescrList;
				} else {
					pstExtEvtDescrListPrev->pNext = pstExtEvtDescr;
					pstExtEvtDescr->pNext = pstExtEvtDescrList;
				}
				break;
			} else if (pstExtEvtDescrList->pNext == NULL) {
				pstExtEvtDescrList->pNext = pstExtEvtDescr;
				break;
			}

			pstExtEvtDescrListPrev = pstExtEvtDescrList;
			pstExtEvtDescrList = pstExtEvtDescrList->pNext;
		}
	}

	pstExtEvt->usUpdateFlag |= (1<<pstExtEvtDescr->stEED.ucDescrNumber);

	return EPG_UPDATE_EXTEVENT;
}

static void TCCDEPGDB_DeleteExtEvt(ST_EPG_EXTEVT *pstExtEvt)
{
	ST_EPG_EXTEVTDESCR *pstExtEvtDescrList, *pstExtEvtDescrTemp;
	EXT_EVT_STRUCT *pstEESList, *pstEESTemp;

	if (pstExtEvt == NULL) {
		ALOGE("%s::%d Error Invalid param", __func__, __LINE__);
		return;
	}

	pstExtEvtDescrList = pstExtEvt->pstList;
	while(pstExtEvtDescrList)
	{
		pstEESList = pstExtEvtDescrList->stEED.pstExtEvtList;
		while(pstEESList)
		{
			pstEESTemp = pstEESList;
			pstEESList = pstEESList->pstExtEvtStruct;
			tcc_mw_free(__FUNCTION__, __LINE__, pstEESTemp);
		}

		pstExtEvtDescrTemp = pstExtEvtDescrList;
		pstExtEvtDescrList = pstExtEvtDescrList->pNext;
		tcc_mw_free(__FUNCTION__, __LINE__, pstExtEvtDescrTemp);
	}
	pstExtEvt->pstList = NULL;

	if (pstExtEvt->pucItem) {
		tcc_mw_free(__FUNCTION__, __LINE__, pstExtEvt->pucItem);
		pstExtEvt->pucItem = NULL;
	}
	pstExtEvt->usItemLen = 0;

	if (pstExtEvt->pucText) {
		tcc_mw_free(__FUNCTION__, __LINE__, pstExtEvt->pucText);
		pstExtEvt->pucText = NULL;
	}
	pstExtEvt->usTextLen = 0;

	pstExtEvt->usUpdateFlag = 0;

	return;
}

static void TCCEPGDB_Delete(int type, int serviceId, int tableId)
{
	TccMessage *pEPGDB;
	ST_EPG_DATA *p;
	unsigned int i, uielement;

	pEPGDB = TCCEPGDB_GetEPGDB(type);
	if (pEPGDB == NULL)
	{
		return;
	}

	uielement = pEPGDB->TccMessageGetCount();
	for (i=0; i<uielement; i++)
	{
		p = (ST_EPG_DATA *)pEPGDB->TccMessageGet();
		if (p==NULL)
		{
			continue;
		}

		if (((serviceId == -1) || ((unsigned int)serviceId == p->usServiceID))
			&& ((tableId == -1) || ((unsigned int)tableId == p->uiTableId)))
		{
			if (p->iReferenceServiceID < 0)
			{
				if (p->EvtName)
				{
					tcc_mw_free(__FUNCTION__, __LINE__, p->EvtName);
				}

				if (p->EvtText)
				{
					tcc_mw_free(__FUNCTION__, __LINE__, p->EvtText);
				}

 				TCCDEPGDB_DeleteExtEvt(&(p->stExtEvt));
			}
			else    // Noah, 20180611
			{
				if(p->uiTableId == 0x4E)
				{
					if(p->EvtName)    // Noah, 20180611, Memory Leak / DUMP_13SEG551143_0828_120518.ts, 19:40 ~ End
					{
						if(p->usServiceID == (unsigned int)p->iReferenceServiceID && p->EventID == (unsigned int)p->iReferenceEventID)
						{
							tcc_mw_free(__FUNCTION__, __LINE__, p->EvtName);
						}
						else if(p->is_EvtName_MustBeFreed)
						{
							tcc_mw_free(__FUNCTION__, __LINE__, p->EvtName);
						}
					}

					if(p->EvtText)    // Noah, 20180611, Memory Leak / 521143_20180529_0942.ts
					{
						if(p->usServiceID == (unsigned int)p->iReferenceServiceID && p->EventID == (unsigned int)p->iReferenceEventID)
						{
							tcc_mw_free(__FUNCTION__, __LINE__, p->EvtText);
						}
						else if(p->is_EvtText_MustBeFreed)
						{
							tcc_mw_free(__FUNCTION__, __LINE__, p->EvtText);
						}
					}
				}
			}

			tcc_mw_free(__FUNCTION__, __LINE__, p);
		}
		else
		{
			if(pEPGDB->TccMessagePut((void *)p))
			{
				if (p->iReferenceServiceID < 0)
				{
					if (p->EvtName)
					{
						tcc_mw_free(__FUNCTION__, __LINE__, p->EvtName);
					}

					if (p->EvtText)
					{
						tcc_mw_free(__FUNCTION__, __LINE__, p->EvtText);
					}

 					TCCDEPGDB_DeleteExtEvt(&(p->stExtEvt));
				}

				tcc_mw_free(__FUNCTION__, __LINE__, p);
			}
		}
	}

	return;
}

static void TCCEPGDB_Insert(int type, ST_EPG_DATA *pdata)
{
	TccMessage *pEPGDB;

	pEPGDB = TCCEPGDB_GetEPGDB(type);
	if (pEPGDB == NULL) {
		if(pdata != NULL) {
			if (pdata->iReferenceServiceID < 0) {
				if (pdata->EvtName) {
					tcc_mw_free(__FUNCTION__, __LINE__, pdata->EvtName);
				}
				if (pdata->EvtText) {
					tcc_mw_free(__FUNCTION__, __LINE__, pdata->EvtText);
				}

				TCCDEPGDB_DeleteExtEvt(&(pdata->stExtEvt));
			}
			tcc_mw_free(__FUNCTION__, __LINE__, pdata);
		}
		return;
	}

	if(pEPGDB->TccMessagePut((void *)pdata)){
		if (pdata->iReferenceServiceID < 0) {
			if (pdata->EvtName) {
				tcc_mw_free(__FUNCTION__, __LINE__, pdata->EvtName);
			}
			if (pdata->EvtText) {
				tcc_mw_free(__FUNCTION__, __LINE__, pdata->EvtText);
			}

			TCCDEPGDB_DeleteExtEvt(&(pdata->stExtEvt));
		}
		tcc_mw_free(__FUNCTION__, __LINE__, pdata);
	}

	return;
}

static int TCCEPGDB_Update(int type, ST_EPG_DATA *pdata, EXT_EVT_DESCR *pstEED)
{
	TccMessage *pEPGDB;
	ST_EPG_DATA *p;
	void *epgdb_handle;
	unsigned int i, uielement;
	int ret = -1;

	pEPGDB = TCCEPGDB_GetEPGDB(type);
	if (pEPGDB == NULL) {
		return ret;
	}

	uielement = pEPGDB->TccMessageGetCount();
	epgdb_handle = pEPGDB->TccMessageHandleFirst();
	for (i=0; i<uielement; i++)
	{
		if (epgdb_handle == NULL) {
			break;
		}

		p = (ST_EPG_DATA *)pEPGDB->TccMessageHandleMsg(epgdb_handle);
		if ((p->uiCurrentChannelNumber == pdata->uiCurrentChannelNumber) &&
			(p->uiCurrentCountryCode  == pdata->uiCurrentCountryCode) &&
			(p->OrgNetworkID == pdata->OrgNetworkID) &&
			(p->TStreamID == pdata->TStreamID) &&
			(p->usServiceID == pdata->usServiceID) &&
			(p->uiTableId == pdata->uiTableId) &&
			(p->EventID == pdata->EventID))
		{
			if (pdata->eUpdateFlag == EPG_UPDATE_EVENT)
			{
				p->ucVersionNumber = pdata->ucVersionNumber;
				if (p->iReferenceServiceID < 0)
				{
					if (p->EvtName) {
						tcc_mw_free(__FUNCTION__, __LINE__, p->EvtName);
					}
					if (p->EvtText) {
						tcc_mw_free(__FUNCTION__, __LINE__, p->EvtText);
					}
				}

				p->EvtName = pdata->EvtName;
				p->iLen_EvtName = pdata->iLen_EvtName;
				p->EvtText = pdata->EvtText;
				p->iLen_EvtText = pdata->iLen_EvtText;
				p->uiUpdatedFlag |= EPG_UPDATE_EVENT;

				p->is_EvtName_MustBeFreed = pdata->is_EvtName_MustBeFreed;	  // Noah, 20180611, Memory Leak / DUMP_13SEG551143_0828_120518.ts, 19:40 ~ End
				p->is_EvtText_MustBeFreed = pdata->is_EvtText_MustBeFreed;	  // Noah, 20180611, Memory Leak / 521143_20180529_0942.ts
			}
			else if (pdata->eUpdateFlag == EPG_UPDATE_EXTEVENT)
			{
				ret = TCCEPGDB_AddExtEvtDescr(&p->stExtEvt, pstEED);
				if (ret == EPG_UPDATE_EXTEVENT)
				{
					p->uiUpdatedFlag |= TCCEPGDB_MergeExtEvt(&p->stExtEvt, pstEED);
				}
			}
			else if (pdata->eUpdateFlag == EPG_UPDATE_EVTGROUP)
			{
				p->iReferenceServiceID = pdata->iReferenceServiceID;
				p->uiUpdatedFlag |= EPG_UPDATE_EVTGROUP;
				p->iReferenceEventID = pdata->iReferenceEventID;
			}
			else if (pdata->eUpdateFlag == EPG_UPDATE_CONTENT)
			{
				p->iGenre = pdata->iGenre;
				p->uiUpdatedFlag |= EPG_UPDATE_CONTENT;
			}
			else if (pdata->eUpdateFlag == EPG_UPDATE_RATING)
			{
				p->iRating = pdata->iRating;
				p->uiUpdatedFlag |= EPG_UPDATE_RATING;
			}
			else if (pdata->eUpdateFlag == EPG_UPDATE_AUDIOINFO)
			{
				p->iAudioType = pdata->iAudioType;
				p->iAudioSr = pdata->iAudioSr;
				p->iAudioLang1 = pdata->iAudioLang1;
				p->iAudioLang2 = pdata->iAudioLang2;
				p->uiUpdatedFlag |= EPG_UPDATE_AUDIOINFO;
			}
			else if (pdata->eUpdateFlag == EPG_UPDATE_VIDEOINFO)
			{
				p->iVideoInfo = pdata->iVideoInfo;
				p->uiUpdatedFlag |= EPG_UPDATE_VIDEOINFO;
			}

			ret = 0;
			break;
		}
		epgdb_handle = pEPGDB->TccMessageHandleNext(epgdb_handle);
	}

	return ret;
}

#if 0    // Noah, 20180611, NOT support now
static int TCCEPGDB_UpdateEventCommon(int type, ST_EPG_DATA *pdata)
{
	TccMessage *pEPGDB;
	ST_EPG_DATA *p;
	void *epgdb_handle;
	unsigned int i, uielement;
	int ret = -1;

	pEPGDB = TCCEPGDB_GetEPGDB(type);
	if (pEPGDB == NULL) {
		return ret;
	}

	if(pdata->iReferenceServiceID > 0) {
		if(pdata->usServiceID == (unsigned int)pdata->iReferenceServiceID) {
			return ret;
		}

		pdata->EvtName = NULL;
		pdata->iLen_EvtName = 0;

		pdata->EvtText = NULL;
		pdata->iLen_EvtText = 0;

		pdata->stExtEvt.usUpdateFlag = 0;
		pdata->stExtEvt.usItemLen = 0;
		pdata->stExtEvt.pucItem = NULL;
		pdata->stExtEvt.usTextLen = 0;
		pdata->stExtEvt.pucText = NULL;

		pdata->iGenre = -1;
		pdata->iRating = 0;
		pdata->iAudioType = 0;
		pdata->iAudioLang1 = 0;
		pdata->iAudioLang2 = 0;
		pdata->iAudioSr = 0;
		pdata->iVideoInfo = 0;
	}

	uielement = pEPGDB->TccMessageGetCount();
	epgdb_handle = pEPGDB->TccMessageHandleFirst();
	for (i=0; i<uielement; i++)
	{
		if (epgdb_handle == NULL) {
			break;
		}

		p = (ST_EPG_DATA *)pEPGDB->TccMessageHandleMsg(epgdb_handle);
		if ((p->uiCurrentChannelNumber == pdata->uiCurrentChannelNumber) &&
			(p->uiCurrentCountryCode  == pdata->uiCurrentCountryCode) &&
			(p->OrgNetworkID == pdata->OrgNetworkID) &&
			(p->TStreamID == pdata->TStreamID) &&
			(p->usServiceID == (unsigned int)pdata->iReferenceServiceID) &&
			(p->EventID == (unsigned int)pdata->iReferenceEventID))
		{
			if (p->iLen_EvtName) {
				pdata->EvtName = p->EvtName;
				pdata->iLen_EvtName = p->iLen_EvtName;
			}
			if (p->iLen_EvtText) {
				pdata->EvtText = p->EvtText;
				pdata->iLen_EvtText = p->iLen_EvtText;
			}

			pdata->stExtEvt.usUpdateFlag = p->stExtEvt.usUpdateFlag;
			pdata->stExtEvt.usItemLen = p->stExtEvt.usItemLen;
			pdata->stExtEvt.pucItem = p->stExtEvt.pucItem;
			pdata->stExtEvt.usTextLen = p->stExtEvt.usTextLen;
			pdata->stExtEvt.pucText = p->stExtEvt.pucText;

			pdata->iGenre = p->iGenre;
			pdata->iRating = p->iRating;
			pdata->iAudioType = p->iAudioType;
			pdata->iAudioLang1 = p->iAudioLang1;
			pdata->iAudioLang2 = p->iAudioLang2;
			pdata->iAudioSr = p->iAudioSr;
			pdata->iVideoInfo = p->iVideoInfo;

			pdata->uiUpdatedFlag |= p->uiUpdatedFlag;

			ret = 0;
			break;
		}

		epgdb_handle = pEPGDB->TccMessageHandleNext(epgdb_handle);
	}

	return ret;
}
#endif    // Noah, 20180611, NOT support now

static int TCCEPGDB_GetCount(int type, ST_EPG_DATA *pdata)
{
	TccMessage *pEPGDB;
	ST_EPG_DATA *p;
	void *epgdb_handle;
	unsigned int i, uielement, count = 0;

	pEPGDB = TCCEPGDB_GetEPGDB(type);
	if (pEPGDB == NULL) {
		return count;
	}

	uielement = pEPGDB->TccMessageGetCount();
	epgdb_handle = pEPGDB->TccMessageHandleFirst();
	for (i=0; i<uielement; i++)
	{
		if (epgdb_handle == NULL) {
			break;
		}

		p = (ST_EPG_DATA *)pEPGDB->TccMessageHandleMsg(epgdb_handle);
		if ((p->uiCurrentChannelNumber == pdata->uiCurrentChannelNumber) &&
			(p->uiCurrentCountryCode  == pdata->uiCurrentCountryCode) &&
			(p->OrgNetworkID == pdata->OrgNetworkID) &&
			(p->TStreamID == pdata->TStreamID) &&
			(p->usServiceID == pdata->usServiceID) &&
			(p->EventID == pdata->EventID))
		{
			count++;
		}
		epgdb_handle = pEPGDB->TccMessageHandleNext(epgdb_handle);
	}

	return count;
}

static int TCCEPGDB_GetVersion(int type, ST_EPG_DATA *pdata)
{
	TccMessage *pEPGDB;
	ST_EPG_DATA *p;
	void *epgdb_handle;
	int version = -1;
	unsigned int i, uielement;

	pEPGDB = TCCEPGDB_GetEPGDB(type);
	if (pEPGDB == NULL) {
		return version;
	}

	uielement = pEPGDB->TccMessageGetCount();
	epgdb_handle = pEPGDB->TccMessageHandleFirst();
	for (i=0; i<uielement; i++)
	{
		if (epgdb_handle == NULL) {
			break;
		}

		p = (ST_EPG_DATA *)pEPGDB->TccMessageHandleMsg(epgdb_handle);
		if ((p->uiCurrentChannelNumber == pdata->uiCurrentChannelNumber) &&
			(p->usServiceID  == pdata->usServiceID) &&
			(p->uiTableId  == pdata->uiTableId) &&
			(p->ucSection == pdata->ucSection))
		{
			version = p->ucVersionNumber;
			break;
		}
		epgdb_handle = pEPGDB->TccMessageHandleNext(epgdb_handle);
	}

	return version;
}

static ST_EPG_DATA* TCCEPGDB_GetEPGData(int type, ST_EPG_DATA *pdata)
{
	TccMessage *pEPGDB;
	ST_EPG_DATA *p;
	void *epgdb_handle;
	int version = -1;
	unsigned int i, uielement;

	pEPGDB = TCCEPGDB_GetEPGDB(type);
	if (pEPGDB == NULL) {
		return NULL;
	}

	uielement = pEPGDB->TccMessageGetCount();
	epgdb_handle = pEPGDB->TccMessageHandleFirst();
	for (i=0; i<uielement; i++)
	{
		if (epgdb_handle == NULL) {
			break;
		}

		p = (ST_EPG_DATA *)pEPGDB->TccMessageHandleMsg(epgdb_handle);
		if ((p->uiCurrentChannelNumber == pdata->uiCurrentChannelNumber) &&
			(p->usServiceID  == pdata->usServiceID) &&
			(p->ucSection == 0))
		{
			return p;
		}
		epgdb_handle = pEPGDB->TccMessageHandleNext(epgdb_handle);
	}

	return NULL;
}

static ST_EPG_DATA* TCCEPGDB_GetEventData(int type, ST_EPG_DATA *pdata)
{
	TccMessage *pEPGDB;
	ST_EPG_DATA *p;
	void *epgdb_handle;
	int version = -1;
	unsigned int i, uielement;

	pEPGDB = TCCEPGDB_GetEPGDB(type);
	if (pEPGDB == NULL) {
		return NULL;
	}

	uielement = pEPGDB->TccMessageGetCount();
	epgdb_handle = pEPGDB->TccMessageHandleFirst();
	for (i=0; i<uielement; i++)
	{
		if (epgdb_handle == NULL) {
			break;
		}

		p = (ST_EPG_DATA *)pEPGDB->TccMessageHandleMsg(epgdb_handle);
		if ((p->uiCurrentChannelNumber == pdata->uiCurrentChannelNumber) &&
			(p->uiCurrentCountryCode  == pdata->uiCurrentCountryCode) &&
			(p->OrgNetworkID == pdata->OrgNetworkID) &&
			(p->TStreamID == pdata->TStreamID) &&
			(p->usServiceID == pdata->usServiceID) &&
			(p->uiTableId == pdata->uiTableId) &&
			(p->EventID == pdata->EventID))
		{
			return p;
		}
		epgdb_handle = pEPGDB->TccMessageHandleNext(epgdb_handle);
	}

	return NULL;
}

int TCCISDBT_EPGDB_GetRegionTableID(unsigned short usAreaCode)
{
	unsigned short usStateID, usMicroRegionID;
	int iRegionID = 0;

	usStateID = (usAreaCode & 0xF80) >> 7;
	usMicroRegionID = (usAreaCode & 0x7F);

	/* Sanity check of State ID */
	if(usStateID > 0 && usStateID <= SBTVD_STATE_NUM_MAX) {
		/* In advance to check Micro-Region ID, Find Region ID by State ID. */
		iRegionID = SBTVD_TimeZone_RegionID_Pool[usStateID - 1];

		/* Check if current state has multiple time zone.(Pernambuco) */
		if(usStateID == 13)	{
			/* Sanity check of Micro-Region ID */
			if(usMicroRegionID && usMicroRegionID <= SBTVD_STATE13_REGION_NUM_MAX) {
				iRegionID = SBTVD_TimeZone_State13_RegionID_Pool[usMicroRegionID - 1];
			} else {
				iRegionID = 1;
			}
		}
	}

	return iRegionID;
}

static void TCCEPGDB_GetLocalTime(int iAreaCode, LOCAL_TIME_OFFSET_STRUCT *pstLocalTimeOffset, int *piLocalOffset, int *piHourOffset, int *piMinOffset)
{
	int iLTORegionID;
	int i;

	*piLocalOffset = 0;
	*piHourOffset = 0;
	*piMinOffset = 0;

	if (TCC_Isdbt_IsSupportBrazil()) {
		iLTORegionID = TCCISDBT_EPGDB_GetRegionTableID(iAreaCode);
		if (iLTORegionID > 7) {
			iLTORegionID = 0;
		}

		for(i=0; i<pstLocalTimeOffset->ucCount; i++)
		{
			if((pstLocalTimeOffset->ucCount == 1) || (pstLocalTimeOffset->astLocalTimeOffsetData[i].ucCountryRegionID == iLTORegionID)) {
				*piLocalOffset = i;

				*piHourOffset = ((pstLocalTimeOffset->astLocalTimeOffsetData[i].ucLocalTimeOffset_BCD[0] & 0xF0) >> 4) * 10;
				*piHourOffset += pstLocalTimeOffset->astLocalTimeOffsetData[i].ucLocalTimeOffset_BCD[0] & 0x0F;

				*piMinOffset = ((pstLocalTimeOffset->astLocalTimeOffsetData[i].ucLocalTimeOffset_BCD[1] & 0xF0) >> 4) * 10;
				*piMinOffset += pstLocalTimeOffset->astLocalTimeOffsetData[i].ucLocalTimeOffset_BCD[1] & 0x0F;

				break;
			}
		}
	}

	return;
}

static void TCCEPGDB_ApplyLocalTime(LOCAL_TIME_OFFSET_STRUCT *pstLocalTimeOffset, int iLocalOffset, int iHourOffset, int iMinOffset,
									 int *piStart_MJD, int *piStart_HH, int *piStart_MM, int *piStart_SS, ST_EPG_DATA *p)
{
	int iStart_MJD, iStart_HH, iStart_MM;

	iStart_MJD = p->Start_MJD;
	iStart_HH = p->Start_HH;
	iStart_MM = p->Start_MM;

	if (iStart_HH < 24)	{
		if (pstLocalTimeOffset->astLocalTimeOffsetData[iLocalOffset].ucLocalTimeOffsetPolarity == 0) {
			iStart_HH += iHourOffset;
			if (( iStart_HH / 24) > 0) {
				iStart_HH %= 24;
				iStart_MJD += 1;
			}
			iStart_MM += iMinOffset;
		} else {
			if( iStart_HH >= iHourOffset) {
				iStart_HH -= iHourOffset;
			} else {
				iStart_HH = (iStart_HH  + 24) - iHourOffset;
				iStart_MJD -= 1;
			}
			iStart_MM -= iMinOffset;
		}
	}

	*piStart_MJD = iStart_MJD;
	*piStart_HH = iStart_HH;
	*piStart_MM = iStart_MM;
	*piStart_SS = p->Start_SS;

	return;
}
int TCCISDBT_EPGDB_Insert(int iType, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_EIT_STRUCT pstEIT, P_ISDBT_EIT_EVENT_DATA pstEITEvent)
{
#if 1
	ST_EPG_DATA *pepg_data;

	pepg_data = (ST_EPG_DATA*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_EPG_DATA));
	memset(pepg_data, 0x0, sizeof(ST_EPG_DATA));

	pepg_data->uiCurrentChannelNumber = uiCurrentChannelNumber;
	pepg_data->uiCurrentCountryCode = uiCurrentCountryCode;
	pepg_data->uiTableId = (pstEIT->uiTableID < EIT_SA_7_ID)? pstEIT->uiTableID : pstEIT->uiTableID - 0x08;
	pepg_data->ucVersionNumber = 0,
	pepg_data->ucSection =  pstEIT->ucSection;
	pepg_data->ucLastSection = pstEIT->ucLastSection;
	pepg_data->ucSegmentLastSection = pstEIT->ucSegmentLastSection;
	pepg_data->OrgNetworkID = pstEIT->uiOrgNetWorkID;
	pepg_data->TStreamID = pstEIT->uiTStreamID;
	pepg_data->usServiceID = pstEIT->uiService_ID;
	pepg_data->EventID = pstEITEvent->uiEventID;
	pepg_data->Start_MJD = pstEITEvent->stStartTime.uiMJD;
	pepg_data->Start_HH = pstEITEvent->stStartTime.stTime.ucHour;
	pepg_data->Start_MM = pstEITEvent->stStartTime.stTime.ucMinute;
	pepg_data->Start_SS = pstEITEvent->stStartTime.stTime.ucSecond;
	pepg_data->Duration_HH = pstEITEvent->stDuration.ucHour;
	pepg_data->Duration_MM = pstEITEvent->stDuration.ucMinute;
	pepg_data->Duration_SS = pstEITEvent->stDuration.ucSecond;
	pepg_data->iGenre = -1;
	pepg_data->iRating = 0;
	pepg_data->iAudioType = 0;
	pepg_data->iAudioSr = 0;
	pepg_data->iAudioLang1 = 0;
	pepg_data->iAudioLang2 = 0;
	pepg_data->iVideoInfo = 0;
	pepg_data->iReferenceServiceID = -1;
	pepg_data->iReferenceEventID = -1;

	TCCEPGDB_Insert(iType, pepg_data);
#endif

	return SQLITE_OK;
}

int TCCISDBT_EPGDB_UpdateEvent(int iType, int uiEvtNameLen, char *pEvtName, int uiEvtTextLen, char *pEvtText, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_EIT_STRUCT pstEIT, P_ISDBT_EIT_EVENT_DATA pstEITEvent)
{
#if 1
	ST_EPG_DATA epg_arg;
	int ret;

	//Check parameters
	memset(&epg_arg, 0, sizeof(ST_EPG_DATA));

	epg_arg.uiCurrentChannelNumber = uiCurrentChannelNumber;
	epg_arg.uiCurrentCountryCode = uiCurrentCountryCode;
	epg_arg.OrgNetworkID = pstEIT->uiOrgNetWorkID;
	epg_arg.TStreamID = pstEIT->uiTStreamID;
	epg_arg.usServiceID = pstEIT->uiService_ID;
	epg_arg.uiTableId = (pstEIT->uiTableID < EIT_SA_7_ID)? pstEIT->uiTableID : pstEIT->uiTableID - 0x08;
	epg_arg.EventID = pstEITEvent->uiEventID;

	//Update parameters
	epg_arg.eUpdateFlag = EPG_UPDATE_EVENT;
	epg_arg.ucVersionNumber = pstEIT->ucVersionNumber,

	epg_arg.iLen_EvtName = uiEvtNameLen;
	if (epg_arg.iLen_EvtName) {
		epg_arg.EvtName = (unsigned char *)tcc_mw_malloc(__FUNCTION__, __LINE__, MAX_EPG_CONV_TXT_BYTE);
		// jini 11th (Null Pointer Dereference => memset(epg_arg.EvtText, 0x0, MAX_EPG_CONV_TXT_BYTE);)
		if (epg_arg.EvtName == NULL)
		{
			return SQLITE_ERROR;
		}
		memset(epg_arg.EvtName, 0x0, MAX_EPG_CONV_TXT_BYTE);
		epg_arg.iLen_EvtName = ISDBTString_MakeUnicodeString(pEvtName, (short *)epg_arg.EvtName, Get_Language_Code());

		epg_arg.is_EvtName_MustBeFreed = 1;    // Noah, 20180611, Memory Leak / DUMP_13SEG551143_0828_120518.ts, 19:40 ~ End
	}

	epg_arg.iLen_EvtText = uiEvtTextLen;
	if (epg_arg.iLen_EvtText) {
		epg_arg.EvtText = (unsigned char *)tcc_mw_malloc(__FUNCTION__, __LINE__, MAX_EPG_CONV_TXT_BYTE);
		// jini 12th (Null Pointer Dereference => memset(epg_arg.EvtText, 0x0, MAX_EPG_CONV_TXT_BYTE);)
		if (epg_arg.EvtText == NULL)
		{
			if (epg_arg.EvtName)
				tcc_mw_free(__FUNCTION__, __LINE__, epg_arg.EvtName);

			return SQLITE_ERROR;
		}
		memset(epg_arg.EvtText, 0x0, MAX_EPG_CONV_TXT_BYTE);
		epg_arg.iLen_EvtText = ISDBTString_MakeUnicodeString(pEvtText, (short *)epg_arg.EvtText, Get_Language_Code());

		epg_arg.is_EvtText_MustBeFreed = 1;    // Noah, 20180611, Memory Leak / 521143_20180529_0942.ts
	}

	ret = TCCEPGDB_Update(iType, &epg_arg, NULL);
	if (ret != 0) {
		if (epg_arg.EvtName) {
			tcc_mw_free(__FUNCTION__, __LINE__, epg_arg.EvtName);
		}
		if (epg_arg.EvtText) {
			tcc_mw_free(__FUNCTION__, __LINE__, epg_arg.EvtText);
		}
	}
#endif

	return SQLITE_OK;
}

int TCCISDBT_EPGDB_UpdateExtEvent(int iType, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_EIT_STRUCT pstEIT, P_ISDBT_EIT_EVENT_DATA pstEITEvent, EXT_EVT_DESCR *pstEED)
{
#if 1
	ST_EPG_DATA epg_arg;

	//Check parameters
	memset(&epg_arg, 0, sizeof(ST_EPG_DATA));
	epg_arg.uiCurrentChannelNumber = uiCurrentChannelNumber;
	epg_arg.uiCurrentCountryCode = uiCurrentCountryCode;
	epg_arg.OrgNetworkID = pstEIT->uiOrgNetWorkID;
	epg_arg.TStreamID = pstEIT->uiTStreamID;
	epg_arg.usServiceID = pstEIT->uiService_ID;
	epg_arg.uiTableId = (pstEIT->uiTableID < EIT_SA_7_ID)? pstEIT->uiTableID : pstEIT->uiTableID - 0x08;
	epg_arg.EventID = pstEITEvent->uiEventID;

	//Update parameters
	epg_arg.eUpdateFlag = EPG_UPDATE_EXTEVENT;

	TCCEPGDB_Update(iType, &epg_arg, pstEED);
#endif

	return SQLITE_OK;
}

int TCCISDBT_EPGDB_UpdateEvtGroup(int iType, int uiReferenceServiceID, int uiReferenceEventID, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_EIT_STRUCT pstEIT, P_ISDBT_EIT_EVENT_DATA pstEITEvent)
{
#if 1
	ST_EPG_DATA epg_arg;

	//Check parameters
	memset(&epg_arg, 0, sizeof(ST_EPG_DATA));
	epg_arg.uiCurrentChannelNumber = uiCurrentChannelNumber;
	epg_arg.uiCurrentCountryCode = uiCurrentCountryCode;
	epg_arg.OrgNetworkID = pstEIT->uiOrgNetWorkID;
	epg_arg.TStreamID = pstEIT->uiTStreamID;
	epg_arg.usServiceID = pstEIT->uiService_ID;
	epg_arg.uiTableId = (pstEIT->uiTableID < EIT_SA_7_ID)? pstEIT->uiTableID : pstEIT->uiTableID - 0x08;
	epg_arg.EventID = pstEITEvent->uiEventID;

	//Update parameters
	epg_arg.eUpdateFlag = EPG_UPDATE_EVTGROUP;
	epg_arg.iReferenceServiceID = uiReferenceServiceID;
	epg_arg.iReferenceEventID = uiReferenceEventID;

	TCCEPGDB_Update(iType, &epg_arg, NULL);
#endif

	return SQLITE_OK;
}
int TCCISDBT_EPGDB_UpdateContent(int iType, unsigned int uiContentGenre, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_EIT_STRUCT pstEIT, P_ISDBT_EIT_EVENT_DATA pstEITEvent)
{
#if 1
	ST_EPG_DATA epg_arg;

	//Check parameters
	memset(&epg_arg, 0, sizeof(ST_EPG_DATA));
	epg_arg.uiCurrentChannelNumber = uiCurrentChannelNumber;
	epg_arg.uiCurrentCountryCode = uiCurrentCountryCode;
	epg_arg.OrgNetworkID = pstEIT->uiOrgNetWorkID;
	epg_arg.TStreamID = pstEIT->uiTStreamID;
	epg_arg.usServiceID = pstEIT->uiService_ID;
	epg_arg.uiTableId = (pstEIT->uiTableID < EIT_SA_7_ID)? pstEIT->uiTableID : pstEIT->uiTableID - 0x08;
	epg_arg.EventID = pstEITEvent->uiEventID;

	//Update parameters
	epg_arg.eUpdateFlag = EPG_UPDATE_CONTENT;
	epg_arg.iGenre = uiContentGenre;

	TCCEPGDB_Update(iType, &epg_arg, NULL);
#endif

	return SQLITE_OK;
}

int TCCISDBT_EPGDB_UpdateRating(int iType, unsigned int uiRating, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_EIT_STRUCT pstEIT, P_ISDBT_EIT_EVENT_DATA pstEITEvent)
{
#if 1
	ST_EPG_DATA epg_arg;
	int ret = -1;

	//Check parameters
	memset(&epg_arg, 0, sizeof(ST_EPG_DATA));
	epg_arg.uiCurrentChannelNumber = uiCurrentChannelNumber;
	epg_arg.uiCurrentCountryCode = uiCurrentCountryCode;
	epg_arg.OrgNetworkID = pstEIT->uiOrgNetWorkID;
	epg_arg.TStreamID = pstEIT->uiTStreamID;
	epg_arg.usServiceID = pstEIT->uiService_ID;
	epg_arg.uiTableId = (pstEIT->uiTableID < EIT_SA_7_ID)? pstEIT->uiTableID : pstEIT->uiTableID - 0x08;
	epg_arg.EventID = pstEITEvent->uiEventID;

	//Update parameters
	epg_arg.eUpdateFlag = EPG_UPDATE_RATING;
	epg_arg.iRating = uiRating;

	ret = TCCEPGDB_Update(iType, &epg_arg, NULL);
#endif

	return ret;
}

int TCCISDBT_EPGDB_UpdateAudioInfo (
		int iType,
		unsigned int component_tag,
		int component_type,
		int sampling_rate,
		unsigned int uiLangCode1,
		unsigned int uiLangCode2,
		int uiCurrentChannelNumber,
		int uiCurrentCountryCode,
		P_ISDBT_EIT_STRUCT pstEIT,
		P_ISDBT_EIT_EVENT_DATA pstEITEvent)
{
	int	ret = -1;
	ST_EPG_DATA epg_arg;

	//Check parameters
	memset(&epg_arg, 0, sizeof(ST_EPG_DATA));
	epg_arg.uiCurrentChannelNumber = uiCurrentChannelNumber;
	epg_arg.uiCurrentCountryCode = uiCurrentCountryCode;
	epg_arg.OrgNetworkID = pstEIT->uiOrgNetWorkID;
	epg_arg.TStreamID = pstEIT->uiTStreamID;
	epg_arg.usServiceID = pstEIT->uiService_ID;
	epg_arg.uiTableId = (pstEIT->uiTableID < EIT_SA_7_ID)? pstEIT->uiTableID : pstEIT->uiTableID - 0x08;
	epg_arg.EventID = pstEITEvent->uiEventID;

	//Update parameters
	epg_arg.eUpdateFlag = EPG_UPDATE_AUDIOINFO;
	epg_arg.iAudioType = component_type;
	epg_arg.iAudioLang1 = uiLangCode1;
	epg_arg.iAudioLang2 = uiLangCode2;
	epg_arg.iAudioSr = sampling_rate;
	ret = TCCEPGDB_Update (iType, &epg_arg, NULL);

	return ret;
}

int TCCISDBT_EPGDB_UpdateVideoInfo (int iType, unsigned int component_tag, unsigned int component_type, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_EIT_STRUCT pstEIT, P_ISDBT_EIT_EVENT_DATA pstEITEvent)
{
	int	ret = -1;
	ST_EPG_DATA epg_arg;

	//Check parameters
	memset(&epg_arg, 0, sizeof(ST_EPG_DATA));
	epg_arg.uiCurrentChannelNumber = uiCurrentChannelNumber;
	epg_arg.uiCurrentCountryCode = uiCurrentCountryCode;
	epg_arg.OrgNetworkID = pstEIT->uiOrgNetWorkID;
	epg_arg.TStreamID = pstEIT->uiTStreamID;
	epg_arg.usServiceID = pstEIT->uiService_ID;
	epg_arg.uiTableId = (pstEIT->uiTableID < EIT_SA_7_ID)? pstEIT->uiTableID : pstEIT->uiTableID - 0x08;
	epg_arg.EventID = pstEITEvent->uiEventID;

	//Update parameters
	epg_arg.eUpdateFlag = EPG_UPDATE_VIDEOINFO;
	epg_arg.iVideoInfo = component_type;
	ret = TCCEPGDB_Update (iType, &epg_arg, NULL);

	return ret;
}

int TCCISDBT_EPGDB_GetCount(int iType, int uiCurrentChannelNumber, int uiCurrentCountryCode, int uiEventID, P_ISDBT_EIT_STRUCT pstEIT)
{
#if 1
	ST_EPG_DATA epg_arg;
	int count = 0;

	//Check parameters
	memset(&epg_arg, 0, sizeof(ST_EPG_DATA));
	epg_arg.uiCurrentChannelNumber = uiCurrentChannelNumber;
	epg_arg.uiCurrentCountryCode = uiCurrentCountryCode;
	epg_arg.OrgNetworkID = pstEIT->uiOrgNetWorkID;
	epg_arg.TStreamID = pstEIT->uiTStreamID;
	epg_arg.usServiceID = pstEIT->uiService_ID;
	epg_arg.EventID = uiEventID;

	count = TCCEPGDB_GetCount(iType, &epg_arg);
#endif

	return count;
}

int TCCISDBT_EPGDB_GetVersion(int iType, int usServiceID, int uiCurrentChannelNumber, int uiTableId, int ucSection)
{
	ST_EPG_DATA epg_arg;
	int version;

	//Check parameters
	memset(&epg_arg, 0, sizeof(ST_EPG_DATA));
	epg_arg.uiCurrentChannelNumber = uiCurrentChannelNumber;
	epg_arg.usServiceID = usServiceID;
	epg_arg.uiTableId = uiTableId;
	epg_arg.ucSection = ucSection;

	//Update parameters
	version = TCCEPGDB_GetVersion(iType, &epg_arg);

	return version;
}

void TCCISDBT_EPGDB_GetChannelInfo(int iType, int uiCurrentChannelNumber, int iServiceID,
								  int *piAudioMode, int *piVideoMode, int *piAudioLang1, int *piAudioLang2,
								  int *piStartMJD, int *piStartHH, int *piStartMM, int *piStartSS,
								  int *piDurationHH, int *piDurationMM, int *piDurationSS,
								  unsigned short *pusEvtName, int *piEvtNameLen)
{
	ST_EPG_DATA epg_arg;
	ST_EPG_DATA *p;
	unsigned short *pusTemp;
 	int i;

	memset(&epg_arg, 0, sizeof(ST_EPG_DATA));
	epg_arg.uiCurrentChannelNumber = uiCurrentChannelNumber;
	epg_arg.usServiceID = iServiceID;

	p = TCCEPGDB_GetEPGData(iType, &epg_arg);
	if (p != NULL) {
		*piAudioMode = p->iAudioType;
		*piVideoMode = p->iVideoInfo;
		*piAudioLang1 = p->iAudioLang1;
		*piAudioLang2 = p->iAudioLang2;
		*piStartMJD = p->Start_MJD;
		*piStartHH = p->Start_HH;
		*piStartMM = p->Start_MM;
		*piStartSS = p->Start_SS;
		*piDurationHH = p->Duration_HH;
		*piDurationMM = p->Duration_MM;
		*piDurationSS = p->Duration_SS;
		*piEvtNameLen = p->iLen_EvtName * sizeof(unsigned short);
		memcpy(pusEvtName, p->EvtName, *piEvtNameLen);
	}

	return;
}

void TCCISDBT_EPGDB_GetChannelInfo2(int iType, int uiCurrentChannelNumber, int iServiceID, int *piGenre)
{
	ST_EPG_DATA epg_arg;
	ST_EPG_DATA *p;
	unsigned short *pusTemp;
 	int i;

	memset(&epg_arg, 0, sizeof(ST_EPG_DATA));
	epg_arg.uiCurrentChannelNumber = uiCurrentChannelNumber;
	epg_arg.usServiceID = iServiceID;

	p = TCCEPGDB_GetEPGData(iType, &epg_arg);
	if (p != NULL) {
		*piGenre = p->iGenre;
	}

	return;
}

void TCCISDBT_EPGDB_GetDateTime(int iAreaCode, DATE_TIME_STRUCT *pstDateTime, LOCAL_TIME_OFFSET_STRUCT *pstLocalTimeOffset,
								int *piMJD, int *piHour, int *piMin, int *piSec, int *piPolarity, int *piHourOffset, int *piMinOffset)
{
	int iLocalOffset, iHourOffset, iMinOffset;

	TCCEPGDB_GetLocalTime(iAreaCode, pstLocalTimeOffset, &iLocalOffset, &iHourOffset, &iMinOffset);

	*piMJD = pstDateTime->uiMJD;
	*piHour = pstDateTime->stTime.ucHour;
	*piMin = pstDateTime->stTime.ucMinute;
	*piSec = pstDateTime->stTime.ucSecond;
	*piPolarity = pstLocalTimeOffset->astLocalTimeOffsetData[iLocalOffset].ucLocalTimeOffsetPolarity;
	*piHourOffset = iHourOffset;
	*piMinOffset = iMinOffset;

	return;
}

void TCCISDBT_EPGDB_Init(void)
{
	DEBUG_MSG("%s::%d", __func__, __LINE__);
	if(gpEPGDB_PF == NULL)
		gpEPGDB_PF = new TccMessage(256);
	if(gpEPGDB_Schedule == NULL)
		gpEPGDB_Schedule = new TccMessage(2048);

	if(gpEPGDB_ExtEvtItemDescr == NULL)
		gpEPGDB_ExtEvtItemDescr = (unsigned char*)tcc_mw_malloc(__FUNCTION__, __LINE__, MAX_EPG_CONV_TXT_BYTE);
	if(gpEPGDB_ExtEvtItem == NULL)
		gpEPGDB_ExtEvtItem = (unsigned char*)tcc_mw_malloc(__FUNCTION__, __LINE__, MAX_EPG_CONV_EXT_EVT_TXT_BYTE);
	if(gpEPGDB_ExtEvtText == NULL)
		gpEPGDB_ExtEvtText = (unsigned char*)tcc_mw_malloc(__FUNCTION__, __LINE__, MAX_EPG_CONV_EXT_EVT_TXT_BYTE);
	if(gpEPGDB_ExtEvtConv == NULL)
		gpEPGDB_ExtEvtConv = (unsigned char*)tcc_mw_malloc(__FUNCTION__, __LINE__, MAX_EPG_CONV_EXT_EVT_TXT_BYTE);

	return;
}

void TCCISDBT_EPGDB_DeInit(void)
{
	DEBUG_MSG("%s::%d", __func__, __LINE__);

	TCCEPGDB_Delete(EPG_PF, -1, -1);
	TCCEPGDB_Delete(EPG_SCHEDULE, -1, -1);

	if (gpEPGDB_ExtEvtItemDescr) {
		tcc_mw_free(__FUNCTION__, __LINE__, gpEPGDB_ExtEvtItemDescr);
		gpEPGDB_ExtEvtItemDescr = NULL;
	}

	if (gpEPGDB_ExtEvtItem) {
		tcc_mw_free(__FUNCTION__, __LINE__, gpEPGDB_ExtEvtItem);
		gpEPGDB_ExtEvtItem = NULL;
	}

	if (gpEPGDB_ExtEvtText) {
		tcc_mw_free(__FUNCTION__, __LINE__, gpEPGDB_ExtEvtText);
		gpEPGDB_ExtEvtText = NULL;
	}

	if (gpEPGDB_ExtEvtConv) {
		tcc_mw_free(__FUNCTION__, __LINE__, gpEPGDB_ExtEvtConv);
		gpEPGDB_ExtEvtConv = NULL;
	}

	if (gpEPGDB_PF) {
		delete gpEPGDB_PF;
		gpEPGDB_PF = NULL;
	}

	if (gpEPGDB_Schedule) {
		delete gpEPGDB_Schedule;
		gpEPGDB_Schedule = NULL;
	}

	return;
}

void TCCISDBT_EPGDB_Delete(int iType, int iServiceId, int iTableId)
{
	TCCEPGDB_Delete(iType, iServiceId, iTableId);

	return;
}

void TCCISDBT_EPGDB_DeleteAll(void)
{
	TCCEPGDB_Delete(EPG_PF, -1, -1);
	TCCEPGDB_Delete(EPG_SCHEDULE, -1, -1);

	return;
}

static void TCCEPGDB_UpdateDB(sqlite3 *hSQL, char *szSQL, ST_EPG_DATA *p)
{
	sqlite3_stmt *stmt = NULL;
	int err;
	unsigned char nullChar[2] = {'\0', };

	if((err = sqlite3_prepare_v2(hSQL, szSQL, -1, &stmt, NULL)) == SQLITE_OK)
	{
//		err = sqlite3_reset(stmt);
//		if (err != SQLITE_OK) ALOGE("[%s:%d] ERROR : %d\n", __func__, __LINE__, err);

		err = sqlite3_bind_text16(stmt, 1, (p->EvtName!=NULL)?p->EvtName:nullChar, -1, SQLITE_STATIC);
		if (err != SQLITE_OK) ALOGE("[%s:%d] ERROR : %d\n", __func__, __LINE__, err);

		err = sqlite3_bind_text16(stmt, 2, (p->EvtText!=NULL)?p->EvtText:nullChar, -1, SQLITE_STATIC);
		if (err != SQLITE_OK) ALOGE("[%s:%d] ERROR : %d\n", __func__, __LINE__, err);

		err = sqlite3_bind_text16(stmt, 3, (p->stExtEvt.pucItem!=NULL)?p->stExtEvt.pucItem:nullChar, -1, SQLITE_STATIC);
		if (err != SQLITE_OK) ALOGE("[%s:%d] ERROR : %d\n", __func__, __LINE__, err);

		err = sqlite3_bind_text16(stmt, 4, (p->stExtEvt.pucText!=NULL)?p->stExtEvt.pucText:nullChar, -1, SQLITE_STATIC);
		if (err != SQLITE_OK) ALOGE("[%s:%d] ERROR : %d\n", __func__, __LINE__, err);

		err = sqlite3_step(stmt);
		if ((err != SQLITE_ROW) && (err != SQLITE_DONE)){
			ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);
		}
	}
	if(stmt) sqlite3_finalize(stmt);
	return ;
}

extern unsigned int g_nHowMany_FsegServices;    // IM478A-21, Multi or Common Event
extern unsigned int g_nServiceID_FsegServices[16];    // IM478A-21, Multi or Common Event, 16 is temp.

static void TCCEPGDB_UpdateSQL(sqlite3 *hSQL, char *szTableName, int iStart_MJD, int iStart_HH, int iStart_MM, int iStart_SS, ST_EPG_DATA *p)
{
	// Noah, To avoid Codesonar's warning, Buffer Overrun
	// It said, "sprintf() writes up to 1034 bytes starting at the beginning of the buffer".
	// So, I changed the buffer size from 1024 to 1040.
	// It said , "sprintf() writes up to 1042 bytes starting at the beginning of the buffer".
	// So, I changed the buffer size from 1040 to 1050
	char szSQL[1050] = { 0, };
	sqlite3_stmt *stmt = NULL;
	int iUpdatedFlag;
	int err;

	// update EPGDB to database
	if (p->uiUpdatedFlag)
	{
		sprintf(szSQL,
			"SELECT uiUpdatedFlag \
			 FROM %s_%d_0x%x \
			 WHERE uiCurrentCountryCode=%d AND EventID=%d AND Start_MJD=%d",
			szTableName, p->uiCurrentChannelNumber, p->usServiceID,
			p->uiCurrentCountryCode, p->EventID, p->Start_MJD);

		if (sqlite3_prepare_v2(hSQL, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK)
		{
			if ((err = sqlite3_step(stmt)) == SQLITE_ROW)
			{
				iUpdatedFlag = sqlite3_column_int(stmt, 0);
			}
			else
			{
				iUpdatedFlag = -1;
			}
		}
		else
		{
			iUpdatedFlag = -1;
		}

		if(stmt)
			sqlite3_finalize(stmt);

		if (iUpdatedFlag < p->uiUpdatedFlag)
		{
			//DEBUG_MSG("Update SQL(0x%08X)\n", p->uiUpdatedFlag);

			sprintf(szSQL, 
					"UPDATE %s_%d_0x%x \
						SET uiUpdatedFlag=%d, uiTableId=%d, \
							uiCurrentChannelNumber=%d, uiCurrentCountryCode=%d, ucVersionNumber=%d, \
							ucSection=%d, ucLastSection=%d, ucSegmentLastSection=%d, \
							OrgNetworkID=%d, TStreamID=%d, usServiceID=%d, EventID=%d, \
							Start_MJD=%d, Start_HH=%d, Start_MM=%d, Start_SS=%d, \
							Duration_HH=%d, Duration_MM=%d, Duration_SS=%d, \
							iLen_EvtName=%d, EvtName=?, iLen_EvtText=%d, EvtText=?, iLen_ExtEvtItem=%d, ExtEvtItem=?, iLen_ExtEvtText=%d, ExtEvtText=?, \
					 		Genre=%d, AudioMode=%d, AudioSr=%d, AudioLang1=%d, AudioLang2=%d, VideoMode=%d, \
							iRating=0, RefServiceID=%d, RefEventID=%d, \
							MultiOrCommonEvent=%d \
						WHERE EventID=%d AND Start_MJD=%d",
						szTableName, p->uiCurrentChannelNumber, p->usServiceID,
						p->uiUpdatedFlag, p->uiTableId,
						p->uiCurrentChannelNumber, p->uiCurrentCountryCode, p->ucVersionNumber,
						p->ucSection, p->ucLastSection, p->ucSegmentLastSection,
						p->OrgNetworkID, p->TStreamID, p->usServiceID, p->EventID,
						iStart_MJD, iStart_HH, iStart_MM, iStart_SS,
						p->Duration_HH, p->Duration_MM, p->Duration_SS,
						p->iLen_EvtName, p->iLen_EvtText, p->stExtEvt.usItemLen, p->stExtEvt.usTextLen,
						p->iGenre, p->iAudioType, p->iAudioSr, p->iAudioLang1, p->iAudioLang2, p->iVideoInfo,
						p->iReferenceServiceID, p->iReferenceEventID,
						p->MultiOrCommonEvent,
						p->EventID, p->Start_MJD);

			TCCEPGDB_UpdateDB(hSQL, szSQL, p);
		}
		else if (iUpdatedFlag < 0)
		{
			//DEBUG_MSG("Insert SQL(0x%08X)\n", p->uiUpdatedFlag);

///////////////////////////////////////////////////////////////////////////////////////////////////////
// IM478A-21, Multi or Common Event, Start S S S S S S S S S S S S S S S S S S S S S S S S S S S S S S
			unsigned int i = 0;
			unsigned int isFsegService = 0;

			isFsegService = 0;
			for (i = 0; i < g_nHowMany_FsegServices; i++)
			{
				if (g_nServiceID_FsegServices[i] == p->usServiceID)
				{
					isFsegService = 1;
					break;
				}
			}

			if (isFsegService)
			{
				TccMessage *pEPGDB = NULL;
				ST_EPG_DATA *pPreviousDataInQueue = NULL;
				void *epgdb_handle = NULL;
				unsigned int uielement = 0;

				if (EIT_PF_A_ID == p->uiTableId)
				{
					pEPGDB = TCCEPGDB_GetEPGDB(EPG_PF);
				}
				else    // EIT_SA_0_ID ~ EIT_SA_F_ID
				{
					pEPGDB = TCCEPGDB_GetEPGDB(EPG_SCHEDULE);
				}

				if (pEPGDB != NULL)
				{
					unsigned int nHowMany_SameEventId_InOtherService = 0;

					uielement = pEPGDB->TccMessageGetCount();
					epgdb_handle = pEPGDB->TccMessageHandleFirst();

					for (i = 0; i < uielement; i++)
					{
						if (epgdb_handle == NULL)
						{
							break;
						}

						pPreviousDataInQueue = (ST_EPG_DATA *)pEPGDB->TccMessageHandleMsg(epgdb_handle);

						isFsegService = 0;
						for (i = 0; i < g_nHowMany_FsegServices; i++)
						{
							if (g_nServiceID_FsegServices[i] == pPreviousDataInQueue->usServiceID)
							{
								isFsegService = 1;
								break;
							}
						}

						if (isFsegService)
						{
							if ((p->uiCurrentChannelNumber == pPreviousDataInQueue->uiCurrentChannelNumber) &&
								(p->TStreamID == pPreviousDataInQueue->TStreamID) &&
								(p->usServiceID != pPreviousDataInQueue->usServiceID) &&
								(p->EventID == pPreviousDataInQueue->EventID))
							{
								nHowMany_SameEventId_InOtherService++;

								/*
								ALOGD("[%s] / g_nHowMany_FsegServices(%d) / Found Same EventID(%d, 0x%x, 0x%x, %d, %d) in other service(0x%x, %d) / nHowMany_SameEventId_InOtherService(%d)\n",
									__func__,
									g_nHowMany_FsegServices,
									p->uiCurrentChannelNumber, p->TStreamID, p->usServiceID, p->EventID, p->MultiOrCommonEvent,
									pPreviousDataInQueue->usServiceID, pPreviousDataInQueue->MultiOrCommonEvent,
									nHowMany_SameEventId_InOtherService);
								*/
							}
						}

						epgdb_handle = pEPGDB->TccMessageHandleNext(epgdb_handle);
					}

					////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					// Update of current data in Queue
					if (0 == nHowMany_SameEventId_InOtherService)    // NOT found the same EventID
					{
						p->MultiOrCommonEvent = g_nHowMany_FsegServices;
					}
					else if (nHowMany_SameEventId_InOtherService < g_nHowMany_FsegServices)
					{
						p->MultiOrCommonEvent = g_nHowMany_FsegServices - nHowMany_SameEventId_InOtherService;
					}
					else
					{
						// Error
						ALOGD("[%s] Found the same EventID, but the number of the EventID Error !!!!! / g_nHowMany_FsegServices(%d), nHowMany_SameEventId_InOtherService(%d)\n",
							__func__, g_nHowMany_FsegServices, nHowMany_SameEventId_InOtherService);
						printf("[%s] Found the same EventID, but the number of the EventID Error !!!!! / g_nHowMany_FsegServices(%d), nHowMany_SameEventId_InOtherService(%d)\n",
							__func__, g_nHowMany_FsegServices, nHowMany_SameEventId_InOtherService);

						p->MultiOrCommonEvent = 1;
					}
					// Update of current data in Queue
					////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

					if (0 != nHowMany_SameEventId_InOtherService)
					{
						epgdb_handle = pEPGDB->TccMessageHandleFirst();

						for (i = 0; i < uielement; i++)
						{
							if (epgdb_handle == NULL)
							{
								break;
							}

							pPreviousDataInQueue = (ST_EPG_DATA *)pEPGDB->TccMessageHandleMsg(epgdb_handle);

							isFsegService = 0;
							for (i = 0; i < g_nHowMany_FsegServices; i++)
							{
								if (g_nServiceID_FsegServices[i] == pPreviousDataInQueue->usServiceID)
								{
									isFsegService = 1;
									break;
								}
							}

							if (isFsegService)
							{
								if ((p->uiCurrentChannelNumber == pPreviousDataInQueue->uiCurrentChannelNumber) &&
									(p->TStreamID == pPreviousDataInQueue->TStreamID) &&
									(p->usServiceID != pPreviousDataInQueue->usServiceID) &&
									(p->EventID == pPreviousDataInQueue->EventID))
								{
									if (p->MultiOrCommonEvent != pPreviousDataInQueue->MultiOrCommonEvent)
									{
										//ALOGD("[%s] / pPreviousDataInQueue->usServiceID(0x%x) / pPreviousDataInQueue->MultiOrCommonEvent(%d) -> -> -> p->MultiOrCommonEvent(%d)\n",
										//	__func__, pPreviousDataInQueue->usServiceID, pPreviousDataInQueue->MultiOrCommonEvent, p->MultiOrCommonEvent);

										////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
										// Update of previous data in Queue
										pPreviousDataInQueue->MultiOrCommonEvent = p->MultiOrCommonEvent;
										// Update of previous data in Queue
										////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

										////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
										// Real DB Update of previous data in Queue
										int err = SQLITE_OK;
										char szSQL[1024] = { 0 };
										sqlite3_stmt *stmt = NULL;

										if (EIT_PF_A_ID == p->uiTableId)
										{
											sprintf(szSQL, "UPDATE %s_%d_0x%x SET MultiOrCommonEvent=%d WHERE uiCurrentChannelNumber=%d AND usServiceID=%d AND EventID=%d",
														"EPG_PF", pPreviousDataInQueue->uiCurrentChannelNumber, pPreviousDataInQueue->usServiceID,
														pPreviousDataInQueue->MultiOrCommonEvent,
														pPreviousDataInQueue->uiCurrentChannelNumber, pPreviousDataInQueue->usServiceID, pPreviousDataInQueue->EventID);

										}
										else    // EIT_SA_0_ID ~ EIT_SA_F_ID
										{
											sprintf(szSQL, "UPDATE %s_%d_0x%x SET MultiOrCommonEvent=%d WHERE uiCurrentChannelNumber=%d AND usServiceID=%d AND EventID=%d",
														"EPG_Sche", pPreviousDataInQueue->uiCurrentChannelNumber, pPreviousDataInQueue->usServiceID,
														pPreviousDataInQueue->MultiOrCommonEvent,
														pPreviousDataInQueue->uiCurrentChannelNumber, pPreviousDataInQueue->usServiceID, pPreviousDataInQueue->EventID);
										}

										err = sqlite3_prepare_v2(hSQL, szSQL, strlen(szSQL), &stmt, NULL);
										if (err == SQLITE_OK)
										{
											sqlite3_step(stmt);
											sqlite3_finalize(stmt);	
										}
										else
										{
											ALOGE("[%s] sqlite3_prepare_v2 returned Error(%d) !!!!!\n", __func__, err);
										}
										// Real DB Update of previous data
										////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
									}
								}
							}

							epgdb_handle = pEPGDB->TccMessageHandleNext(epgdb_handle);
						}
					}
				}
				else
				{
					// Error
					ALOGE("[%s:%d] Please check this log !!!!!\n", __func__, __LINE__);
				}
			}
// IM478A-21, Multi or Common Event, End E E E E E E E E E E E E E E E E E E E E E E E E E E E E E E E
///////////////////////////////////////////////////////////////////////////////////////////////////////

			sprintf(szSQL,
					"INSERT \
						INTO %s_%d_0x%x \
							(uiUpdatedFlag, uiTableId, \
							uiCurrentChannelNumber, uiCurrentCountryCode, ucVersionNumber, \
							ucSection, ucLastSection, ucSegmentLastSection, \
							OrgNetworkID, TStreamID, usServiceID, EventID, \
							Start_MJD, Start_HH, Start_MM, Start_SS, \
							Duration_HH, Duration_MM, Duration_SS, \
							iLen_EvtName, EvtName, iLen_EvtText, EvtText, iLen_ExtEvtItem, ExtEvtItem, iLen_ExtEvtText, ExtEvtText, \
							Genre, AudioMode, AudioSr, AudioLang1, AudioLang2, VideoMode, \
							iRating, RefServiceID, RefEventID, \
							MultiOrCommonEvent) \
						VALUES (%d, %d, \
								%d, %d, %d, \
								%d, %d, %d, \
								%d, %d, %d, %d, \
								%d, %d, %d, %d, \
								%d, %d, %d, \
								%d, ?, %d, ?, %d, ?, %d, ?, \
								%d, %d, %d, %d, %d, %d, \
								0, %d, %d, \
								%d)",
						szTableName, p->uiCurrentChannelNumber, p->usServiceID,
						p->uiUpdatedFlag, p->uiTableId,
						p->uiCurrentChannelNumber, p->uiCurrentCountryCode, p->ucVersionNumber,
						p->ucSection, p->ucLastSection, p->ucSegmentLastSection,
						p->OrgNetworkID, p->TStreamID, p->usServiceID, p->EventID,
						iStart_MJD, iStart_HH, iStart_MM, iStart_SS,
						p->Duration_HH, p->Duration_MM, p->Duration_SS,
						p->iLen_EvtName, p->iLen_EvtText, p->stExtEvt.usItemLen, p->stExtEvt.usTextLen,
						p->iGenre, p->iAudioType, p->iAudioSr, p->iAudioLang1, p->iAudioLang2, p->iVideoInfo,
						p->iReferenceServiceID,	p->iReferenceEventID,
						p->MultiOrCommonEvent);

			TCCEPGDB_UpdateDB(hSQL, szSQL, p);
		}
		else
		{
			//DEBUG_MSG("Skip SQL(0x%08X)\n", p->uiUpdatedFlag);
		}
	}

	return;
}

static void TCCEPGDB_UpdateEventCommonAndSQL(sqlite3 *sqlhandle, char *szTableName, int type, ST_EPG_DATA *pdata)
{
	TccMessage *pEPGDB;
	ST_EPG_DATA *p;
	void *epgdb_handle;
	unsigned int i, uielement;

	pEPGDB = TCCEPGDB_GetEPGDB(type);
	if (pEPGDB == NULL)
	{
		return;
	}

	if( pdata->iReferenceServiceID == -1 ||
		(pdata->usServiceID == (unsigned int)pdata->iReferenceServiceID && pdata->EventID == (unsigned int)pdata->iReferenceEventID))
	{
		TCCEPGDB_UpdateSQL(sqlhandle, szTableName, pdata->Start_MJD, pdata->Start_HH, pdata->Start_MM, pdata->Start_SS, pdata);

		uielement = pEPGDB->TccMessageGetCount();
		epgdb_handle = pEPGDB->TccMessageHandleFirst();
		for (i=0; i<uielement; i++)
		{
			if (epgdb_handle == NULL)
			{
				break;
			}

			p = (ST_EPG_DATA *)pEPGDB->TccMessageHandleMsg(epgdb_handle);
			if ((pdata->uiCurrentChannelNumber == p->uiCurrentChannelNumber) &&
				(pdata->uiCurrentCountryCode  == p->uiCurrentCountryCode) &&
				(pdata->OrgNetworkID == p->OrgNetworkID) &&
				(pdata->TStreamID == p->TStreamID) &&
				(pdata->usServiceID == p->iReferenceServiceID) &&
				(pdata->EventID == (unsigned int)p->iReferenceEventID) &&
				(pdata->uiUpdatedFlag != p->uiUpdatedFlag))
			{
				if (pdata->iLen_EvtName)
				{
					if(type == EPG_PF && p->EvtName)    // Noah, 20180611, Memory Leak / 521143_20180529_0942.ts
					{
						if(p->is_EvtName_MustBeFreed == 1)
						{
							tcc_mw_free(__FUNCTION__, __LINE__, p->EvtName);

							p->is_EvtName_MustBeFreed = 0;
						}
					}

					p->EvtName = pdata->EvtName;
					p->iLen_EvtName = pdata->iLen_EvtName;
				}

				if (pdata->iLen_EvtText)
				{
					if(type == EPG_PF && p->EvtText)    // Noah, 20180611, Memory Leak / 521143_20180529_0942.ts
					{
						if(p->is_EvtText_MustBeFreed == 1)
						{
							tcc_mw_free(__FUNCTION__, __LINE__, p->EvtText);

							p->is_EvtText_MustBeFreed = 0;
						}
					}

					p->EvtText = pdata->EvtText;
					p->iLen_EvtText = pdata->iLen_EvtText;
				}

				p->stExtEvt.usUpdateFlag = pdata->stExtEvt.usUpdateFlag;
				p->stExtEvt.usItemLen = pdata->stExtEvt.usItemLen;
				p->stExtEvt.pucItem = pdata->stExtEvt.pucItem;
				p->stExtEvt.usTextLen = pdata->stExtEvt.usTextLen;
				p->stExtEvt.pucText = pdata->stExtEvt.pucText;

				p->iGenre = pdata->iGenre;
				p->iRating = pdata->iRating;
				p->iAudioType = pdata->iAudioType;
				p->iAudioLang1 = pdata->iAudioLang1;
				p->iAudioLang2 = pdata->iAudioLang2;
				p->iAudioSr = pdata->iAudioSr;
				p->iVideoInfo = pdata->iVideoInfo;

				p->uiUpdatedFlag |= pdata->uiUpdatedFlag;

				TCCEPGDB_UpdateSQL(sqlhandle, szTableName, p->Start_MJD, p->Start_HH, p->Start_MM, p->Start_SS, p);
			}

			epgdb_handle = pEPGDB->TccMessageHandleNext(epgdb_handle);
		}
	}
	else
	{
		uielement = pEPGDB->TccMessageGetCount();
		epgdb_handle = pEPGDB->TccMessageHandleFirst();
		for (i=0; i<uielement; i++)
		{
			if (epgdb_handle == NULL) {
				break;
			}

			p = (ST_EPG_DATA *)pEPGDB->TccMessageHandleMsg(epgdb_handle);
			if ((p->uiCurrentChannelNumber == pdata->uiCurrentChannelNumber) &&
				(p->uiCurrentCountryCode  == pdata->uiCurrentCountryCode) &&
				(p->OrgNetworkID == pdata->OrgNetworkID) &&
				(p->TStreamID == pdata->TStreamID) &&
				(p->usServiceID == (unsigned int)pdata->iReferenceServiceID) &&
				(p->EventID == (unsigned int)pdata->iReferenceEventID))
			{
				if (p->iLen_EvtName)
				{
					if(type == EPG_PF && pdata->EvtName)    // Noah, 20180611, Memory Leak / DUMP_13SEG551143_0828_120518.ts, 19:40 ~ End
					{
						if(pdata->is_EvtName_MustBeFreed == 1)
						{
							tcc_mw_free(__FUNCTION__, __LINE__, pdata->EvtName);

							pdata->is_EvtName_MustBeFreed = 0;
						}
					}

					pdata->EvtName = p->EvtName;
					pdata->iLen_EvtName = p->iLen_EvtName;
				}

				if (p->iLen_EvtText)
				{
					if(type == EPG_PF && pdata->EvtText)    // Noah, 20180611, Memory Leak / 521143_20180529_0942.ts
					{
						if(pdata->is_EvtText_MustBeFreed == 1)
						{
							tcc_mw_free(__FUNCTION__, __LINE__, pdata->EvtText);

							pdata->is_EvtText_MustBeFreed = 0;
						}
					}

					pdata->EvtText = p->EvtText;
					pdata->iLen_EvtText = p->iLen_EvtText;
				}

				pdata->stExtEvt.usUpdateFlag = p->stExtEvt.usUpdateFlag;
				pdata->stExtEvt.usItemLen = p->stExtEvt.usItemLen;
				pdata->stExtEvt.pucItem = p->stExtEvt.pucItem;
				pdata->stExtEvt.usTextLen = p->stExtEvt.usTextLen;
				pdata->stExtEvt.pucText = p->stExtEvt.pucText;

				pdata->iGenre = p->iGenre;
				pdata->iRating = p->iRating;
				pdata->iAudioType = p->iAudioType;
				pdata->iAudioLang1 = p->iAudioLang1;
				pdata->iAudioLang2 = p->iAudioLang2;
				pdata->iAudioSr = p->iAudioSr;
				pdata->iVideoInfo = p->iVideoInfo;

				pdata->uiUpdatedFlag |= p->uiUpdatedFlag;

				TCCEPGDB_UpdateSQL(sqlhandle, szTableName, pdata->Start_MJD, pdata->Start_HH, pdata->Start_MM, pdata->Start_SS, pdata);

				return;
			}

			epgdb_handle = pEPGDB->TccMessageHandleNext(epgdb_handle);
		}

		if (type == EPG_SCHEDULE && pdata->iReferenceServiceID > 0 && pdata->iReferenceEventID > 0) {
			pdata->EvtName = NULL;
			pdata->iLen_EvtName = 0;

			pdata->EvtText = NULL;
			pdata->iLen_EvtText = 0;

			pdata->stExtEvt.usUpdateFlag = 0;
			pdata->stExtEvt.usItemLen = 0;
			pdata->stExtEvt.pucItem = NULL;
			pdata->stExtEvt.usTextLen = 0;
			pdata->stExtEvt.pucText = NULL;

			pdata->iGenre = -1;
			pdata->iRating = 0;
			pdata->iAudioType = 0;
			pdata->iAudioLang1 = 0;
			pdata->iAudioLang2 = 0;
			pdata->iAudioSr = 0;
			pdata->iVideoInfo = 0;
		}

		TCCEPGDB_UpdateSQL(sqlhandle, szTableName, pdata->Start_MJD, pdata->Start_HH, pdata->Start_MM, pdata->Start_SS, pdata);
	}

	return;
}

/*
 * Commit one of EPG data
 */
int TCCISDBT_EPGDB_CommitEvent(
	sqlite3 *sqlhandle,
	int uiCurrentChannelNumber,
	int uiCurrentCountryCode,
	P_ISDBT_EIT_STRUCT pstEIT,
	int iEventID
)
{
	TccMessage *pEPGDB;
	ST_EPG_DATA *p;
	void *epgdb_handle;
	unsigned int i, uielement;
	int iType;
	char szTableName[32];
	unsigned int uiTableID;

	if (pstEIT->uiTableID == 0x4E) {
		iType = EPG_PF;
		pEPGDB = gpEPGDB_PF;
		sprintf(szTableName, "EPG_PF");
	} else {
		iType = EPG_SCHEDULE;
		pEPGDB = gpEPGDB_Schedule;
		sprintf(szTableName, "EPG_Sche");
	}

	if (pEPGDB == NULL) {
		return -1;
	}

	uiTableID  = (pstEIT->uiTableID < EIT_SA_7_ID)? pstEIT->uiTableID : pstEIT->uiTableID - 0x08;

	uielement = pEPGDB->TccMessageGetCount();
	epgdb_handle = pEPGDB->TccMessageHandleFirst();
	for (i=0; i<uielement; i++)
	{
		if (epgdb_handle == NULL) {
			break;
		}

		p = (ST_EPG_DATA *)pEPGDB->TccMessageHandleMsg(epgdb_handle);
		if ((p->uiCurrentChannelNumber == (unsigned int)uiCurrentChannelNumber) &&
			(p->uiCurrentCountryCode  == (unsigned int)uiCurrentCountryCode) &&
			(p->OrgNetworkID == pstEIT->uiOrgNetWorkID) &&
			(p->TStreamID == pstEIT->uiTStreamID) &&
			(p->usServiceID == pstEIT->uiService_ID) &&
			(p->uiTableId == uiTableID) &&
			(p->EventID == (unsigned int)iEventID))
		{
			// get information to use an event common & update
			TCCEPGDB_UpdateEventCommonAndSQL(sqlhandle, szTableName, iType, p);

			return 0;
		}
		epgdb_handle = pEPGDB->TccMessageHandleNext(epgdb_handle);
	}

	return -1;
}

#if 0    // Noah, 20180611, NOT support now
/*
 * Commit 24hour EPG data from current time+iDayOffset
 * iDayOffset : -1:Whole Update, 0~:Day Offset, It is added to current time
 */
int TCCISDBT_EPGDB_Commit(
	sqlite3 *sqlhandle,
	int iDayOffset,
	int iChannel,
	int iAreaCode,
	DATE_TIME_STRUCT *pstDateTime,
	LOCAL_TIME_OFFSET_STRUCT *pstLocalTimeOffset
)
{
	TccMessage *pEPGDB;
	ST_EPG_DATA *p, *q;
	void *epgdb_handle, *ref_handle, *pf_handle;
	unsigned int uielement, uiref_element, uipf_element, uiupdated_elements;
	char szTableName[32];
	DATE_TIME_STRUCT current_time = {0, {0, 0, 0}};
	int iLocalOffset, iHourOffset, iMinOffset;
	int iStart_MJD, iStart_HH, iStart_MM, iStart_SS, iRequest_MJD = 0;
	int conflict;
	unsigned int i, j, k;
	int err;

	TCCEPGDB_GetLocalTime(iAreaCode, pstLocalTimeOffset, &iLocalOffset, &iHourOffset, &iMinOffset);

	if (iDayOffset >= 0) {
		current_time.uiMJD = pstDateTime->uiMJD;
		current_time.stTime.ucHour = pstDateTime->stTime.ucHour;
		current_time.stTime.ucMinute = pstDateTime->stTime.ucMinute;
		current_time.stTime.ucSecond = pstDateTime->stTime.ucSecond;

		pEPGDB = gpEPGDB_PF;
		uielement = pEPGDB->TccMessageGetCount();
		epgdb_handle = pEPGDB->TccMessageHandleFirst();
		for (i = 0;i < uielement;i++)
		{
			if (epgdb_handle == NULL) {
				break;
			}

			p = (ST_EPG_DATA *)pEPGDB->TccMessageHandleMsg(epgdb_handle);
			if (p == NULL) {
				epgdb_handle = pEPGDB->TccMessageHandleNext(epgdb_handle);
				continue;
			}

			// check channel number
			if ((unsigned int)iChannel != p->uiCurrentChannelNumber) {
				epgdb_handle = pEPGDB->TccMessageHandleNext(epgdb_handle);
				continue;
			}

			if (p->ucSection == 0) {
				if (p->Start_MJD != current_time.uiMJD) {
					current_time.uiMJD = p->Start_MJD;
					current_time.stTime.ucHour = (unsigned char)p->Start_HH;
					current_time.stTime.ucMinute = (unsigned char)p->Start_MM;
					current_time.stTime.ucSecond = (unsigned char)p->Start_SS;
					ALOGD("%s::%d current_time is changed to EPG_PF's MJD", __func__, __LINE__);
				}
				break;
			}

			epgdb_handle = pEPGDB->TccMessageHandleNext(epgdb_handle);
		}

		iRequest_MJD = current_time.uiMJD + iDayOffset;

		if ((current_time.uiMJD == 0) && (current_time.stTime.ucHour == 0) && (current_time.stTime.ucMinute == 0) && (current_time.stTime.ucSecond == 0)) {
			ALOGE("%s::%d SQL isn't able to update because current_time isn't updated!", __func__, __LINE__);
		}
	}

	DEBUG_MSG("%s::%d::Channel[%d]DayOffset[%d]MJD[%d]H[%d]M[%d]", __func__, __LINE__,
				iChannel, iDayOffset, current_time.uiMJD, current_time.stTime.ucHour, current_time.stTime.ucMinute);

	for (j = 0;j < 2;j++)
	{
		if (j == EPG_PF) {
			pEPGDB = gpEPGDB_PF;
			sprintf(szTableName, "EPG_PF");
		} else {
			pEPGDB = gpEPGDB_Schedule;
			sprintf(szTableName, "EPG_Sche");
		}

		if (pEPGDB == NULL) {
			continue;
		}

		uielement = pEPGDB->TccMessageGetCount();
		epgdb_handle = pEPGDB->TccMessageHandleFirst();
		for (i = 0;i < uielement;i++)
		{
			if (epgdb_handle == NULL) {
				break;
			}

			p = (ST_EPG_DATA *)pEPGDB->TccMessageHandleMsg(epgdb_handle);
			if (p == NULL) {
				epgdb_handle = pEPGDB->TccMessageHandleNext(epgdb_handle);
				continue;
			}

			// check channel number
			if ((unsigned int)iChannel != p->uiCurrentChannelNumber) {
				epgdb_handle = pEPGDB->TccMessageHandleNext(epgdb_handle);
				continue;
			}

			// apply time & date
			TCCEPGDB_ApplyLocalTime(pstLocalTimeOffset, iLocalOffset, iHourOffset, iMinOffset, &iStart_MJD, &iStart_HH, &iStart_MM, &iStart_SS, p);

			// check time & date
			if ((j == EPG_PF && iDayOffset > 0) || (j == EPG_SCHEDULE && iDayOffset >= 0)) {
				if (current_time.uiMJD) {
					if ((unsigned int)iStart_MJD != (unsigned int)iRequest_MJD) {
						epgdb_handle = pEPGDB->TccMessageHandleNext(epgdb_handle);
						continue;
					}

					if ((unsigned int)iStart_MJD == current_time.uiMJD) {
						unsigned int check_time_val, current_time_val;
						current_time_val = (current_time.stTime.ucHour << 16) | (current_time.stTime.ucMinute << 8) | (current_time.stTime.ucSecond);
						check_time_val = ((iStart_HH + p->Duration_HH) << 16) | ((iStart_MM + p->Duration_MM) << 8) | (iStart_SS + p->Duration_SS);
						if (check_time_val < current_time_val) {
							epgdb_handle = pEPGDB->TccMessageHandleNext(epgdb_handle);
							continue;
						}
					}
				}
			}

			// get information to use an event common
			TCCEPGDB_UpdateEventCommon(j, p);

			// update
			TCCEPGDB_UpdateSQL(sqlhandle, szTableName, iStart_MJD, iStart_HH, iStart_MM, iStart_SS, p);

			epgdb_handle = pEPGDB->TccMessageHandleNext(epgdb_handle);
		}
	}

	return 0;
}
#endif    // Noah, 20180611, NOT support now

