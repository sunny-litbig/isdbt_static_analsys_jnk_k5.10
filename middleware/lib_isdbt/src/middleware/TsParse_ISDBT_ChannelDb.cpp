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
#define LOG_TAG "TSPARSE_CHANNEL_DB"
#include <utils/Log.h>
#endif

#ifdef HAVE_LINUX_PLATFORM
#include <string.h> /* for memset(), memcpy()*/
#include <unistd.h> /* for usleep()*/
#endif

#include "ISDBT_Common.h"
#include "TsParse_ISDBT_DBLayer.h"
#include "TsParse_ISDBT_ChannelDb.h"
#include "tcc_msg.h"
#include "tcc_isdbt_manager_demux.h"    /* for TCC_Isdbt_IsSupportPrimaryService */
#include "tcc_dxb_memory.h"

#define ERR_MSG	ALOGE
#define DBG_MSG //ALOGD

//#define	ISDBT_RESCAN_PROTECT_OLDCHINFO

static TccMessage *g_pChannelMsg;

static ST_CHANNEL_DATA* ChannelDB_GetChannelData(unsigned int uiCurrentChannelNumber, unsigned int uiCurrentCountryCode, unsigned int uiServiceID)
{
	void *hTCCMsg;
	int iTCCMsgCount;
	int i;
	ST_CHANNEL_DATA *pChannelData;

	hTCCMsg = g_pChannelMsg->TccMessageHandleFirst();
	iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();
	for (i = 0; i < iTCCMsgCount; i++)
	{
		if (hTCCMsg == NULL)
		{
			ERR_MSG("[ChannelDB_GetChannelData] Error hTCCMsg is NULL");
			return NULL;
		}

		pChannelData = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);
		if ((pChannelData->uiCurrentChannelNumber == uiCurrentChannelNumber)
			&& (pChannelData->uiCurrentCountryCode == uiCurrentCountryCode)
			&& (pChannelData->uiServiceID == uiServiceID))
		{
			DBG_MSG("[ChannelDB_GetChannelData] Found channel data(0x%08X)\n", pChannelData);
			return pChannelData;
		}

		DBG_MSG("[ChannelDB_GetChannelData] %u, %u, 0x%04X\n", pChannelData->uiCurrentChannelNumber, pChannelData->uiCurrentCountryCode, pChannelData->uiServiceID);

		hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
	}

	DBG_MSG("[ChannelDB_GetChannelData] Error can't found channel data(%u, %u, 0x%04X)\n", uiCurrentChannelNumber, uiCurrentCountryCode, uiServiceID);
	return NULL;
}

void TCCISDBT_ChannelDB_Init(void)
{
	if(g_pChannelMsg == NULL)
		g_pChannelMsg = new TccMessage(2048);

	DBG_MSG("[%s] g_pChannelMsg(0x%08X)\n", __func__, g_pChannelMsg);
	return;
}

void TCCISDBT_ChannelDB_Deinit(void)
{
	int iTCCMsgCount;
	int i;
	ST_CHANNEL_DATA *pChannelData;

	if (g_pChannelMsg)
	{
		iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();
		for (i = 0; i < iTCCMsgCount; i++)
		{
			pChannelData = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageGet();
			if (pChannelData)
			{
				if (pChannelData->pusChannelName)
					tcc_mw_free(__FUNCTION__, __LINE__, pChannelData->pusChannelName);
				if (pChannelData->pusTSName)
					tcc_mw_free(__FUNCTION__, __LINE__, pChannelData->pusTSName);
				if (pChannelData->strLogo_Char)
					tcc_mw_free(__FUNCTION__, __LINE__, pChannelData->strLogo_Char);

				tcc_mw_free(__FUNCTION__, __LINE__, pChannelData);
			}
		}

		delete g_pChannelMsg;
		g_pChannelMsg = NULL;
	}

	DBG_MSG("[%s] g_pChannelMsg(0x%08X)\n", __func__, g_pChannelMsg);
	return;
}

int TCCISDBT_ChannelDB_Clear(void)
{
	int i;
	int iTCCMsgCount;
	ST_CHANNEL_DATA *pChannelData;

	if (g_pChannelMsg)
	{
		iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();
		for (i = 0; i < iTCCMsgCount; i++)
		{
			pChannelData = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageGet();
			if (pChannelData)
			{
				if (pChannelData->pusChannelName)
					tcc_mw_free(__FUNCTION__, __LINE__, pChannelData->pusChannelName);
				if (pChannelData->pusTSName)
					tcc_mw_free(__FUNCTION__, __LINE__, pChannelData->pusTSName);
				if (pChannelData->strLogo_Char)
					tcc_mw_free(__FUNCTION__, __LINE__, pChannelData->strLogo_Char);

				tcc_mw_free(__FUNCTION__, __LINE__, pChannelData);
			}
		}
	}

	return 0;
}

int TCCISDBT_ChannelDB_Insert(P_ISDBT_SERVICE_STRUCT pServiceList, int fUpdateType)
{
	ST_CHANNEL_DATA *pChannelData;
	int f_insert = 0;

	pChannelData = ChannelDB_GetChannelData(pServiceList->uiCurrentChannelNumber, pServiceList->uiCurrentCountryCode, pServiceList->usServiceID);
	if (pChannelData == NULL)
	{
		pChannelData = (ST_CHANNEL_DATA*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_CHANNEL_DATA));
		memset(pChannelData, 0x0, sizeof(ST_CHANNEL_DATA));
		pChannelData->uiCurrentChannelNumber = pServiceList->uiCurrentChannelNumber;
		pChannelData->uiCurrentCountryCode = pServiceList->uiCurrentCountryCode;
		pChannelData->uiServiceID = pServiceList->usServiceID;
		pChannelData->uiArrange = SERVICE_ARRANGE_TYPE_NOTHING;
		pChannelData->DWNL_ID = -1;
		pChannelData->LOGO_ID = -1;
		pChannelData->strLogo_Char = (unsigned short *)NULL;

		f_insert = 1;
	}

	if (fUpdateType != 0 || pChannelData->uiServiceType == 0)
		pChannelData->uiServiceType = pServiceList->enType;

	DBG_MSG("[%s] (%d, %d, 0x%04X)\n", __func__, pChannelData->uiCurrentChannelNumber, pChannelData->uiCurrentCountryCode, pChannelData->uiServiceID);

	if (f_insert) {
		if(g_pChannelMsg->TccMessagePut((void *)pChannelData)) {
			if(pChannelData->pusChannelName)
				tcc_mw_free(__FUNCTION__, __LINE__, pChannelData->pusChannelName);
			if(pChannelData->pusTSName)
				tcc_mw_free(__FUNCTION__, __LINE__, pChannelData->pusTSName);
			if(pChannelData->strLogo_Char)
				tcc_mw_free(__FUNCTION__, __LINE__, pChannelData->strLogo_Char);
			tcc_mw_free(__FUNCTION__, __LINE__, pChannelData);
		}
	}

	return SQLITE_OK;
}

int TCCISDBT_ChannelDB_UpdateStreamID(int iCurrentChannelNumber, int iCurrentCountryCode, int iServiceID, int iStreamID)
{
	ST_CHANNEL_DATA *pChannelData;

	DBG_MSG("[%s] (%d, %d, 0x%04X)\n", __func__, iCurrentChannelNumber, iCurrentCountryCode, iServiceID);

	pChannelData = ChannelDB_GetChannelData(iCurrentChannelNumber, iCurrentCountryCode, iServiceID);
	if (pChannelData)
	{
		pChannelData->uiTStreamID = iStreamID;

		return SQLITE_OK;
	}

	ERR_MSG("[%s] Error update StreamID\n", __func__);
	return SQLITE_ERROR;
}
int TCCISDBT_ChannelDB_Update(int iCurrentChannelNumber, int iCurrentCountryCode, int iServiceID,  P_ISDBT_SERVICE_STRUCT pServiceList)
{
	ST_CHANNEL_DATA *pChannelData;

	DBG_MSG("[%s] (%d, %d, 0x%04X)\n", __func__, iCurrentChannelNumber, iCurrentCountryCode, iServiceID);

	pChannelData = ChannelDB_GetChannelData(iCurrentChannelNumber, iCurrentCountryCode, iServiceID);
	if (pChannelData)
	{
		pChannelData->uiCAECMPID = pServiceList->uiCA_ECM_PID;
		pChannelData->uiACECMPID = pServiceList->uiAC_ECM_PID;
		pChannelData->uiAudioPID = pServiceList->stAudio_PIDInfo[0].uiAudio_PID;
		pChannelData->uiVideoPID = pServiceList->stVideo_PIDInfo[0].uiVideo_PID;
		pChannelData->uiStPID = pServiceList->stSubtitleInfo[0].ucSubtitle_PID;
		pChannelData->uiSiPID = pServiceList->stSubtitleInfo[1].ucSubtitle_PID;
		pChannelData->uiTotalAudio_PID = pServiceList->uiTotalAudio_PID;
		pChannelData->uiTotalVideo_PID = pServiceList->uiTotalVideo_PID;
		pChannelData->usTotalCntSubtLang = pServiceList->usTotalCntSubtLang;
		pChannelData->uiPCR_PID = pServiceList->uiPCR_PID;
		pChannelData->ucAudio_StreamType = pServiceList->stAudio_PIDInfo[0].ucAudio_StreamType;
		pChannelData->ucVideo_StreamType = pServiceList->stVideo_PIDInfo[0].ucVideo_StreamType;
		return SQLITE_OK;
	}

	ERR_MSG("[%s] Error update audioPID, videoPID, stPID, siPID\n", __func__);
	return SQLITE_ERROR;
}

int TCCISDBT_ChannelDB_UpdatePMTPID(int iCurrentChannelNumber, int iCurrentCountryCode, int iServiceID, int iPMTPID)
{
	ST_CHANNEL_DATA *pChannelData;

	DBG_MSG("[%s] (%d, %d, 0x%04X)\n", __func__, iCurrentChannelNumber, iCurrentCountryCode, iServiceID);

	pChannelData = ChannelDB_GetChannelData(iCurrentChannelNumber, iCurrentCountryCode, iServiceID);
	if (pChannelData)
	{
		pChannelData->uiPMTPID = iPMTPID;

		return SQLITE_OK;
	}

	ERR_MSG("[%s] Error update PMTPID\n", __func__);
	return SQLITE_ERROR;
}

int TCCISDBT_ChannelDB_UpdateRemoconID(int iCurrentChannelNumber, int iCurrentCountryCode, int iServiceID, int iRemoconID, int iRegionID, int iThreeDigitNumber)
{
	ST_CHANNEL_DATA *pChannelData;

	DBG_MSG("[%s] (%d, %d, 0x%04X)\n", __func__, iCurrentChannelNumber, iCurrentCountryCode, iServiceID);

	pChannelData = ChannelDB_GetChannelData(iCurrentChannelNumber, iCurrentCountryCode, iServiceID);
	if (pChannelData)
	{
		pChannelData->uiRemoconID = iRemoconID;
		pChannelData->uiRegionID = iRegionID;
		pChannelData->uiThreeDigitNumber = iThreeDigitNumber;

		return SQLITE_OK;
	}

	ERR_MSG("[%s] Error update remoconid\n", __func__);
	return SQLITE_OK;
}

int TCCISDBT_ChannelDB_UpdateChannelName(int iCurrentChannelNumber, int iCurrentCountryCode, int iServiceID, unsigned short *pusChannelName)
{
	ST_CHANNEL_DATA *pChannelData;
	int length = MAX_CHANNEL_NAME * sizeof(unsigned short);

	DBG_MSG("[%s] (%d, %d, 0x%04X)\n", __func__, iCurrentChannelNumber, iCurrentCountryCode, iServiceID);

	pChannelData = ChannelDB_GetChannelData(iCurrentChannelNumber, iCurrentCountryCode, iServiceID);
	if (pChannelData)
	{
		if(pChannelData->pusChannelName != NULL)
			tcc_mw_free(__FUNCTION__, __LINE__, pChannelData->pusChannelName);

		pChannelData->pusChannelName = (unsigned short*)tcc_mw_malloc(__FUNCTION__, __LINE__, length);
		if (pChannelData->pusChannelName) {
			memset(pChannelData->pusChannelName, 0, length);
			memcpy(pChannelData->pusChannelName, pusChannelName, length);
		}
		return SQLITE_OK;
	}

	ERR_MSG("[%s] Error update channelname\n", __func__);
	return SQLITE_ERROR;
}

int TCCISDBT_ChannelDB_UpdateTSName(int iCurrentChannelNumber, int iCurrentCountryCode, int iServiceID, unsigned short *pusTSName)
{
	ST_CHANNEL_DATA *pChannelData;
	int length = MAX_CHANNEL_NAME * sizeof(unsigned short);

	DBG_MSG("[%s] (%d, %d, 0x%04X)\n", __func__, iCurrentChannelNumber, iCurrentCountryCode, iServiceID);

	pChannelData = ChannelDB_GetChannelData(iCurrentChannelNumber, iCurrentCountryCode, iServiceID);
	if (pChannelData)
	{
		if(pChannelData->pusTSName != NULL)
			tcc_mw_free(__FUNCTION__, __LINE__, pChannelData->pusTSName);

		pChannelData->pusTSName = (unsigned short*)tcc_mw_malloc(__FUNCTION__, __LINE__, length);
		if (pChannelData->pusTSName) {
			memset (pChannelData->pusTSName, 0, length);
			memcpy(pChannelData->pusTSName, pusTSName, length);
		}
		return SQLITE_OK;
	}

	return SQLITE_ERROR;
}

int TCCISDBT_ChannelDB_UpdateNetworkID(int iCurrentChannelNumber, int iCurrentCountryCode, int iServiceID, int iNetworkID)
{
	ST_CHANNEL_DATA *pChannelData;

	DBG_MSG("[%s] (%d, %d, 0x%04X)\n", __func__, iCurrentChannelNumber, iCurrentCountryCode, iServiceID);

	pChannelData = ChannelDB_GetChannelData(iCurrentChannelNumber, iCurrentCountryCode, iServiceID);
	if (pChannelData)
	{
		pChannelData->uiNetworkID = iNetworkID;

		return SQLITE_OK;
	}

	ERR_MSG("[%s] Error update networkID\n", __func__);
	return SQLITE_OK;
}

int TCCISDBT_ChannelDB_UpdateAreaCode(int iCurrentChannelNumber, int iCurrentCountryCode, int iAreaCode)
{
	void *hTCCMsg;
	int iTCCMsgCount;
	int i;
	ST_CHANNEL_DATA *pChannelData;

	DBG_MSG("[%s] (%d, %d)\n", __func__, iCurrentChannelNumber, iCurrentCountryCode);

	hTCCMsg = g_pChannelMsg->TccMessageHandleFirst();
	iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();
	for (i = 0; i < iTCCMsgCount; i++)
	{
		if (hTCCMsg == NULL)
		{
			ERR_MSG("[%s] Error hTCCMsg is NULL", __func__);
			return SQLITE_ERROR;
		}

		pChannelData = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);
		if ((pChannelData->uiCurrentChannelNumber == (unsigned int)iCurrentChannelNumber)
			&& (pChannelData->uiCurrentCountryCode == (unsigned int)iCurrentCountryCode))
		{
			DBG_MSG("[%s] Found channel data(0x%08X)\n", __func__, pChannelData);

			pChannelData->uiAreaCode = iAreaCode;
		}

		DBG_MSG("[%s] %d, %d, 0x%04X\n", __func__, pChannelData->uiCurrentChannelNumber, pChannelData->uiCurrentCountryCode, pChannelData->uiServiceID);

		hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
	}

	return SQLITE_OK;
}

int TCCISDBT_ChannelDB_UpdateFrequency(int iCurrentChannelNumber, int iCurrentCountryCode, void *pDescTDSD)
{
	void *hTCCMsg;
	int iTCCMsgCount;
	int i;
	ST_CHANNEL_DATA *pChannelData;

	DBG_MSG("[%s] (%d, %d)\n", __func__, iCurrentChannelNumber, iCurrentCountryCode);

	hTCCMsg = g_pChannelMsg->TccMessageHandleFirst();
	iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();
	for (i = 0; i < iTCCMsgCount; i++)
	{
		if (hTCCMsg == NULL)
		{
			ERR_MSG("[%s] Error hTCCMsg is NULL", __func__);
			return SQLITE_ERROR;
		}

		pChannelData = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);
		if ((pChannelData->uiCurrentChannelNumber == (unsigned int)iCurrentChannelNumber)
			&& (pChannelData->uiCurrentCountryCode == (unsigned int)iCurrentCountryCode))
		{
			DBG_MSG("[%s] Found channel data(0x%08X)\n", __func__, pChannelData);

			memcpy(&(pChannelData->stDescTDSD), pDescTDSD, sizeof(T_DESC_TDSD));
		}

		DBG_MSG("[%s] %d, %d, 0x%04X\n", __func__, pChannelData->uiCurrentChannelNumber, pChannelData->uiCurrentCountryCode, pChannelData->uiServiceID);

		hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
	}

	return SQLITE_OK;
}

int TCCISDBT_ChannelDB_UpdateAffiliationID(int iCurrentChannelNumber, int iCurrentCountryCode, void *pDescEBD)
{
	void *hTCCMsg;
	int iTCCMsgCount;
	int i;
	ST_CHANNEL_DATA *pChannelData;

	DBG_MSG("[%s] (%d, %d)\n", __func__, iCurrentChannelNumber, iCurrentCountryCode);

	hTCCMsg = g_pChannelMsg->TccMessageHandleFirst();
	iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();
	for (i = 0; i < iTCCMsgCount; i++)
	{
		if (hTCCMsg == NULL)
		{
			ERR_MSG("[%s] Error hTCCMsg is NULL", __func__);
			return SQLITE_ERROR;
		}

		pChannelData = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);
		if ((pChannelData->uiCurrentChannelNumber == (unsigned int)iCurrentChannelNumber)
			&& (pChannelData->uiCurrentCountryCode == (unsigned int)iCurrentCountryCode))
		{
			DBG_MSG("[%s] Found channel data(0x%08X)\n", __func__, pChannelData);

			memcpy(&(pChannelData->stDescEBD), pDescEBD, sizeof(T_DESC_EBD));
		}

		DBG_MSG("[%s] %d, %d, 0x%04X\n", __func__, pChannelData->uiCurrentChannelNumber, pChannelData->uiCurrentCountryCode, pChannelData->uiServiceID);

		hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
	}

	return SQLITE_OK;
}

int TCCISDBT_ChannelDB_UpdateStrength (int strength_index, int service_id)
{
	ST_CHANNEL_DATA	*pChannelData;
	void *hTCCMsg;
	int	iTCCMsgCount, i;

	hTCCMsg = g_pChannelMsg->TccMessageHandleFirst();
	iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();
	for (i=0; i < iTCCMsgCount; i++)
	{
		if (hTCCMsg == NULL)
			return SQLITE_ERROR;

		pChannelData = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);
		if (service_id == -1) {	//update all channel information
			pChannelData->uiBerAvg = strength_index;
		} else {	//update an information for specific service
			if (pChannelData->uiServiceID == (unsigned int)service_id) {
				pChannelData->uiBerAvg = strength_index;
			}
		}

		hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
	}

	return SQLITE_OK;
}

int TCCISDBT_ChannelDB_UpdateLogoInfo(int iServiceID, int iCurrentChannelNumber, int iCurrentCountryCode, P_ISDBT_SERVICE_STRUCT pServiceList, unsigned short *pusSimpleLogo)
{
	ST_CHANNEL_DATA *pChannelData;

	pChannelData = ChannelDB_GetChannelData(iCurrentChannelNumber, iCurrentCountryCode, iServiceID);
	if (pChannelData) {
		pChannelData->DWNL_ID		= pServiceList->download_data_id;
		pChannelData->LOGO_ID		= pServiceList->logo_id;

		if(pChannelData->strLogo_Char != NULL)
			tcc_mw_free(__FUNCTION__, __LINE__, pChannelData->strLogo_Char);

		pChannelData->strLogo_Char	= (unsigned short*)tcc_mw_malloc(__FUNCTION__, __LINE__, 256 * sizeof(unsigned short));;
		if (pChannelData->strLogo_Char != NULL)
			memcpy(pChannelData->strLogo_Char, pusSimpleLogo, 256*sizeof(unsigned short));
	}
	return SQLITE_OK;
}

int TCCISDBT_ChannelDB_UpdateOriginalNetworkID(int ch_no, int country_code, unsigned int service_id, unsigned int original_network_id)
{
	ST_CHANNEL_DATA *pChannelData;

	pChannelData = ChannelDB_GetChannelData (ch_no, country_code, service_id);
	if (pChannelData) {
		pChannelData->usOrig_Net_ID = original_network_id;
	}
	return SQLITE_OK;
}

int TCCISDBT_ChannelDB_GetPMTCount(int iCurrentChannelNumber)
{
	void *hTCCMsg;
	int iTCCMsgCount;
	int i;
	ST_CHANNEL_DATA *pChannelData;
	int channel_cnt=0;

	hTCCMsg = g_pChannelMsg->TccMessageHandleFirst();
	iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();

	if(iTCCMsgCount ==0)
	{
		ERR_MSG("[%s] Error can't found service data(%d)\n", __func__, iCurrentChannelNumber);
		return -1;
	}

	for (i = 0; i < iTCCMsgCount; i++)
	{
		if (hTCCMsg == NULL)
		{
			ERR_MSG("[%s] Error hTCCMsg is NULL", __func__);
			return -1;
		}

		pChannelData = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);
		if ((pChannelData->uiCurrentChannelNumber == (unsigned int)iCurrentChannelNumber))
		{
			DBG_MSG("[%s] Found serivce data(0x%08X)\n", __func__, pChannelData);
			channel_cnt++;
		}

		DBG_MSG("[%s] uiCurrentChannelNumber = %d channel_cnt= %d \n", __func__, pChannelData->uiCurrentChannelNumber, channel_cnt);
		hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
	}

	return channel_cnt;
}


int TCCISDBT_ChannelDB_GetPMTPID(int iCurrentChannelNumber, unsigned short* pPMT_PID, int iPMT_PID_Size)
{
	void *hTCCMsg;
	int iTCCMsgCount;
	int i, j;
	ST_CHANNEL_DATA *pChannelData;

	// jini 9th : Ok (*pPMT_PID++ = pChannelData->uiPMTPID; => Null Pointer Dereference)
	if (pPMT_PID == NULL)
	{
		ERR_MSG("[%s] Error pPMT_PID is NULL\n", __func__);
		return -1;
	}

	hTCCMsg = g_pChannelMsg->TccMessageHandleFirst();
	iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();

	if(iTCCMsgCount ==0)
	{
		ERR_MSG("[%s] Error can't found service data(%d)\n", __func__, iCurrentChannelNumber);
		return -1;
	}

	DBG_MSG("[%s] iTCCMsgCount = %d \n", __func__, iTCCMsgCount);

	for (i=0, j=0; i < iTCCMsgCount; i++)
	{
		if (hTCCMsg == NULL)
		{
			ERR_MSG("[%s] Error hTCCMsg is NULL", __func__);
			return -1;
		}

		pChannelData = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);
		if ((pChannelData->uiCurrentChannelNumber == (unsigned int)iCurrentChannelNumber))
		{
			DBG_MSG("[%s] Found serivce data(0x%08X)\n", __func__, pChannelData);
			DBG_MSG("[%s] PMT_PID = 0x%x, ServiceID=0x%x\n", __func__, pChannelData->uiPMTPID, pChannelData->uiServiceID);

			if(j >= iPMT_PID_Size)
			{
				ERR_MSG("[%s] Error PMT_PIDs are bigger than iPMT_PID_Size", __func__);
				return j;
			}

			*pPMT_PID++ = pChannelData->uiPMTPID;
			j++;
		}

		DBG_MSG("[%s] %d   pChannelData->PMT_PID = %x \n", __func__, pChannelData->uiCurrentChannelNumber, pChannelData->uiPMTPID);
		hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
	}

	return j;
}

int TCCISDBT_ChannelDB_GetServiceID(int iCurrentChannelNumber, unsigned short* pSERVICE_ID, int iSERVICE_ID_Size)
{
	void *hTCCMsg;
	int iTCCMsgCount;
	int i, j;
	ST_CHANNEL_DATA *p;

	hTCCMsg = g_pChannelMsg->TccMessageHandleFirst();
	iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();
	if(iTCCMsgCount ==0)
	{
		ERR_MSG("[%s] Error can't found service data(%d)\n", __func__, iCurrentChannelNumber);
		return -1;
	}

	DBG_MSG("[%s] iTCCMsgCount = %d \n", __func__, iTCCMsgCount);

	for (i=0, j=0; i < iTCCMsgCount; i++)
	{
		if (hTCCMsg == NULL)
		{
			ERR_MSG("[%s] Error hTCCMsg is NULL", __func__);
			return -1;
		}

		p = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);
		if ((p->uiCurrentChannelNumber == (unsigned int)iCurrentChannelNumber))
		{
			DBG_MSG("[%s] Found serivce data(0x%08X)\n", __func__, p);
			DBG_MSG("[%s] PMT_PID=0x%x, usServiceID=0x%x\n", __func__, p->uiPMTPID, p->uiServiceID);

			if(j >= iSERVICE_ID_Size)
			{
				ERR_MSG("[%s] Error PMT_PIDs are bigger than iPMT_PID_Size", __func__);
				return j;
			}

			*pSERVICE_ID++ = p->uiServiceID;
			j++;
		}

		DBG_MSG("[%s] %d   p->SERVICE_ID=%x \n", __func__, p->uiCurrentChannelNumber, p->uiServiceID);
		hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
	}

	return j;
}

/*
  * TEST item
  * 1) In DB, ther is no row which has same network_id
  * 2)
  */
int TCCISDBT_ChannelDB_ArrangeTable (sqlite3 *sqlhandle)
{
	ST_CHANNEL_DATA *p;
	void *hTCCMsg;
	int iTCCMsgCount, i, j;
	unsigned int uiNetworkID, uiServiceType, uiServiceID;
	int iDB_BER;
	unsigned int uiDB_channelNumber;
	char szSQL[1024];
	sqlite3_stmt *stmt;
	int err=-1;

	hTCCMsg= g_pChannelMsg->TccMessageHandleFirst();
	iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();
	for (i=0; i < iTCCMsgCount; i++)
	{
		if (hTCCMsg == NULL)
			return SQLITE_ERROR;

		p = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);
		if (p->uiArrange == SERVICE_ARRANGE_TYPE_DISCARD) {
			hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
			continue;
		}

		uiNetworkID = p->uiNetworkID;
		uiServiceType = p->uiServiceType;
		uiServiceID = p->uiServiceID;
		p->uiArrange = SERVICE_ARRANGE_TYPE_NOTHING;

		sprintf(szSQL, "SELECT berAVG, channelNumber FROM channels WHERE networkID=%d AND serviceType=%d AND serviceID=%d", uiNetworkID, uiServiceType, uiServiceID);
		if (sqlite3_prepare_v2(sqlhandle, szSQL, -1, &stmt, NULL) == SQLITE_OK) {
			err = sqlite3_step(stmt);
		}
		if (err == SQLITE_OK || err == SQLITE_ROW) {
			iDB_BER = sqlite3_column_int (stmt, 0);
			uiDB_channelNumber = (unsigned int)sqlite3_column_int (stmt, 1);

			if (p->uiBerAvg < iDB_BER) {
				//if new channel is weaker than old
				p->uiArrange = SERVICE_ARRANGE_TYPE_DISCARD;
			} else if (p->uiBerAvg == iDB_BER) {
				/* if strength of channel is same, select the channel of which a channel number is small */
				if(p->uiCurrentChannelNumber < uiDB_channelNumber) {
					p->uiArrange = SERVICE_ARRANGE_TYPE_REPLACE;
				} else {
					p->uiArrange = SERVICE_ARRANGE_TYPE_DISCARD;
				}
			}
			else {
				//if new channel is stronger than old
				p->uiArrange = SERVICE_ARRANGE_TYPE_REPLACE;
				//the channel which has same network_id will be deleted while updating SQL file.
			}
		}
		if (stmt) sqlite3_finalize(stmt);

		hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
	}
	return SQLITE_OK;
}

int TCCISDBT_ChannelDB_GetChannelInfo (SERVICE_FOUND_INFO_T *pSvc)
{
	ST_CHANNEL_DATA *p;
	void *hTCCMsg;
	int iTCCMsgCount;
	int	svc_count = 0, i;

	if (pSvc==NULL) return svc_count;

	pSvc->pServiceFound = NULL;
	pSvc->found_no = 0;

	hTCCMsg = g_pChannelMsg->TccMessageHandleFirst();
	iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();

	if (iTCCMsgCount > 32)	iTCCMsgCount = 32;	//limit count of service
	if (iTCCMsgCount > 0) {
		pSvc->pServiceFound = (struct _tag_service_found_info *)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(struct _tag_service_found_info) * iTCCMsgCount);
		if(pSvc->pServiceFound != NULL) {
			for(i=0; i < iTCCMsgCount; i++)
			{
				if (hTCCMsg == NULL) break;

				p = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);
				if (p->uiArrange != SERVICE_ARRANGE_TYPE_DISCARD) {
					pSvc->pServiceFound[svc_count].uiCurrentChannelNumber = p->uiCurrentChannelNumber;
					pSvc->pServiceFound[svc_count].uiCurrentCountryCode = p->uiCurrentCountryCode;
					pSvc->pServiceFound[svc_count].uiNetworkID = p->uiNetworkID;
					pSvc->pServiceFound[svc_count].uiServiceID = p->uiServiceID;
					pSvc->pServiceFound[svc_count].uiServiceType = p->uiServiceType;
					pSvc->pServiceFound[svc_count].uiOption = 0;
				    pSvc->found_no++;
				    svc_count++;
				}

				hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
			}
		}
	}
	return svc_count;
}

int TCCISDBT_ChannelDB_GetChannelSvcCnt (void)
{
	ST_CHANNEL_DATA *p;
	void *hTCCMsg;
	int iTCCMsgCount, svc_count, i;

	svc_count = 0;
	hTCCMsg = g_pChannelMsg->TccMessageHandleFirst();
	iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();
	for(i=0; i < iTCCMsgCount; i++)
	{
		if (hTCCMsg == NULL) break;
		p = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);
		if (p->uiServiceID !=0 && p->uiPMTPID > 0 && p->uiPMTPID < 0x1FFF && p->uiServiceType != 0)
			svc_count++;

		hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
	}
	return svc_count;
}

int TCCISDBT_ChannelDB_GetChannelInfoDB (sqlite3 *sqlhandle, SERVICE_FOUND_INFO_T *pDBInfo, int country_code, int network_id)
{
	int iSvcCount = 0, i, iSvcFound;
	char szSQL[1024] = { 0 };
	sqlite3_stmt *stmt;
	int row_id, service_id, service_type;
	int err = SQLITE_ERROR;

	sprintf(szSQL, "SELECT COUNT(*) FROM channels WHERE countryCode=%d AND networkID=%d", country_code, network_id);
	if (sqlite3_prepare_v2(sqlhandle, szSQL, -1, &stmt, NULL) == SQLITE_OK) {
		err = sqlite3_step(stmt);
		if (err == SQLITE_ROW)
			iSvcCount = sqlite3_column_int(stmt, 0);
	}
	if (stmt) {
		sqlite3_finalize(stmt);
		stmt = NULL;
	}

	iSvcFound = 0;
	if (iSvcCount > 0) {
		pDBInfo->pServiceFound = (struct _tag_service_found_info *)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(struct _tag_service_found_info) * iSvcCount);
		if (pDBInfo->pServiceFound != NULL) {
			sprintf(szSQL, "SELECT _id, serviceID, serviceType, channelNumber FROM channels WHERE countryCode=%d AND networkID=%d", country_code, network_id);
			err = sqlite3_prepare_v2(sqlhandle, szSQL, -1, &stmt, NULL);
			if (err == SQLITE_OK) {
				for(i=0; i < iSvcCount; i++)
				{
					err = sqlite3_step(stmt);
					if (err == SQLITE_ROW) {
						pDBInfo->pServiceFound[i].rowID = (unsigned int)sqlite3_column_int(stmt, 0);
						pDBInfo->pServiceFound[i].uiServiceID = (unsigned int )sqlite3_column_int(stmt, 1);
						pDBInfo->pServiceFound[i].uiServiceType = (unsigned int)sqlite3_column_int(stmt, 2);
						pDBInfo->pServiceFound[i].uiCurrentChannelNumber = (unsigned int)sqlite3_column_int(stmt, 3);;
						pDBInfo->pServiceFound[i].uiCurrentCountryCode = (unsigned int)country_code;
						pDBInfo->pServiceFound[i].uiNetworkID = (unsigned int)network_id;
						pDBInfo->pServiceFound[i].uiOption = 0;
						iSvcFound++;
					}
				}
			}
			if (stmt) {
				sqlite3_finalize(stmt);
				stmt = NULL;
			}
		}
	}

	pDBInfo->found_no = iSvcFound;
	return iSvcFound;
}

int TCCISDBT_ChannelDB_ChannelInfoArrange (SERVICE_FOUND_INFO_T *pSvcInfo, SERVICE_FOUND_INFO_T *pDBInfo)
{
	int	iSvcInfoCnt, iDBInfoCnt;
	int i, j;
	unsigned int svc_ch_no, svc_country, svc_network_id, svc_service_id;
	unsigned int db_ch_no, db_country, db_network_id, db_service_id;

	if((pSvcInfo == NULL) || (pDBInfo == NULL))
		return -1;
	if (pSvcInfo->pServiceFound == NULL)
		return -1;

	iSvcInfoCnt = pSvcInfo->found_no;
	iDBInfoCnt = pDBInfo->found_no;

	if(iSvcInfoCnt <= 0)
		return -1;

	for (i=0; i < iSvcInfoCnt; i++) {
		svc_country = pSvcInfo->pServiceFound[i].uiCurrentCountryCode;
		svc_network_id = pSvcInfo->pServiceFound[i].uiNetworkID;
		svc_service_id = pSvcInfo->pServiceFound[i].uiServiceID;
		for (j=0; j < iDBInfoCnt; j++) {
			 db_country = pDBInfo->pServiceFound[j].uiCurrentCountryCode;
			 db_network_id = pDBInfo->pServiceFound[j].uiNetworkID;
			 db_service_id = pDBInfo->pServiceFound[j].uiServiceID;
			 if (
			 		(svc_country == db_country) &&
			 		(svc_network_id == db_network_id) &&
			 		(svc_service_id == db_service_id)
			 ) {
			 	pSvcInfo->pServiceFound[i].uiOption = 1;
				pDBInfo->pServiceFound[j].uiOption = 1;
			 }
		}
	}
	return 0;
}

int TCCISDBT_ChannelDB_ChannelInfoArrangeNewSvc(SERVICE_FOUND_INFO_T *pSvcInfo, unsigned int network_id, unsigned int service_id)
{
	int	iSvcCnt,i;
	int fNewSvc = 0;

	if (pSvcInfo == NULL)
		return 0;
	if (pSvcInfo->found_no <= 0)
		return 0;
	if (pSvcInfo->pServiceFound == NULL)
		return 0;

	iSvcCnt = pSvcInfo->found_no;
	for(i=0; i < iSvcCnt; i++) {
		if ((pSvcInfo->pServiceFound[i].uiNetworkID == network_id)
			&& (pSvcInfo->pServiceFound[i].uiServiceID == service_id) ) {
			if 	(pSvcInfo->pServiceFound[i].uiOption == 0)
				fNewSvc = 1;
		}
	}
	return fNewSvc;
}

int TCCISDBT_ChannelDB_DelAutoSearch (sqlite3 *sqlhandle)
{
	char szSQL[1024];
	int err = SQLITE_ERROR;

	sprintf(szSQL, "DELETE FROM channels WHERE preset=%d", RECORD_PRESET_AUTOSEARCH);
	err = TCC_SQLITE3_EXEC (sqlhandle, szSQL, NULL, NULL, NULL);
	return err;
}

int TCCISDBT_ChannelDB_SetAutosearchInfo (sqlite3 *sqlhandle, SERVICE_FOUND_INFO_T *p)
{
	int err = SQLITE_ERROR;
	int i, channel_cnt;
	char szSQL[1024];

	if (sqlhandle == NULL || p == NULL) return err;
	if (p->found_no <= 0) return err;
	if (p->pServiceFound == NULL) return err;

	channel_cnt = p->found_no;
	if (channel_cnt > 32) channel_cnt = 32;
	for (i=0; i < channel_cnt; i++)
	{
		if (p->pServiceFound[i].uiOption != SERVICE_ARRANGE_TYPE_DISCARD) {
			sprintf(szSQL, "UPDATE channels SET preset=%d WHERE channelNumber=%d AND countryCode=%d AND networkID=%d AND serviceID=%d AND serviceType=%d", RECORD_PRESET_AUTOSEARCH,
					p->pServiceFound[i].uiCurrentChannelNumber,
					p->pServiceFound[i].uiCurrentCountryCode,
					p->pServiceFound[i].uiNetworkID,
					p->pServiceFound[i].uiServiceID,
					p->pServiceFound[i].uiServiceType);
			err = TCC_SQLITE3_EXEC (sqlhandle, szSQL, NULL, NULL, NULL);
		}
	}
	return err;
}

void TCCISDBT_ChannelDB_ArrangeAutoSearch_SetDiscard(unsigned int channel_no, unsigned int country_code, unsigned int network_id, unsigned int service_id, unsigned int service_type)
{
	ST_CHANNEL_DATA *p;
	void *hTCCMsg;
	int iTCCMsgCount, i;

	hTCCMsg = g_pChannelMsg->TccMessageHandleFirst();
	iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();
	for(i=0; i < iTCCMsgCount; i++)
	{
		if (hTCCMsg == NULL) break;
		p = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);
		if(p->uiCurrentChannelNumber == channel_no &&
				p->uiCurrentCountryCode == country_code &&
				p->uiNetworkID == network_id &&
				p->uiServiceID == service_id &&
				p->uiServiceType == service_type)
		{
			p->uiArrange = SERVICE_ARRANGE_TYPE_DISCARD;
			break;
		}

		hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
	}
}

int TCCISDBT_ChannelDB_ArrangeAutoSearch (sqlite3 *sqlhandle, SERVICE_FOUND_INFO_T *p)
{
	int err = SQLITE_ERROR;
	int rowID, channel_cnt, i, j;
	char szSQL[1024];
	sqlite3_stmt *stmt;

	if (sqlhandle == NULL || p == NULL) return err;
	if (p->found_no <= 0) return err;
	if (p->pServiceFound == NULL) return err;

	channel_cnt = p->found_no;
	if (channel_cnt > 32) channel_cnt = 32;
	for (i=0; i < channel_cnt; i++)
	{
		sprintf(szSQL,"SELECT _id FROM channels WHERE channelNumber=%d AND countryCode=%d AND networkID=%d AND serviceID=%d AND serviceType=%d",
				p->pServiceFound[i].uiCurrentChannelNumber,
				p->pServiceFound[i].uiCurrentCountryCode,
				p->pServiceFound[i].uiNetworkID,
				p->pServiceFound[i].uiServiceID,
				p->pServiceFound[i].uiServiceType);
		err = sqlite3_prepare_v2(sqlhandle, szSQL, -1, &stmt, NULL);
		if (err == SQLITE_OK) {
			err = sqlite3_step(stmt);
			if(err == SQLITE_ROW) {
				rowID = sqlite3_column_int(stmt, 0);
				p->pServiceFound[i].uiOption = SERVICE_ARRANGE_TYPE_DISCARD;
				ALOGD("In %s, discard autosearch info ch_no=%d, network_id=0x%x, service_id=0x%x, service_type=0x%x [row=%d]", __func__,
						p->pServiceFound[i].uiCurrentChannelNumber,
						p->pServiceFound[i].uiNetworkID,
						p->pServiceFound[i].uiServiceID,
						p->pServiceFound[i].uiServiceType,
						rowID);
				TCCISDBT_ChannelDB_ArrangeAutoSearch_SetDiscard(
						p->pServiceFound[i].uiCurrentChannelNumber,
						p->pServiceFound[i].uiCurrentCountryCode,
						p->pServiceFound[i].uiNetworkID,
						p->pServiceFound[i].uiServiceID,
						p->pServiceFound[i].uiServiceType);
			}
			else {
				p->pServiceFound[i].uiOption = SERVICE_ARRANGE_TYPE_NOTHING;
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
			stmt = NULL;
		}
	}
	return SQLITE_OK;
}

int TCCISDBT_ChannelDB_SetArrangementType(int iType)
{
	int err = SQLITE_ERROR;
	ST_CHANNEL_DATA *p;
	void *hTCCMsg;
	int iTCCMsgCount, i;

	hTCCMsg = g_pChannelMsg->TccMessageHandleFirst();
	iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();

	if (iTCCMsgCount > 0) {
		for(i=0; i < iTCCMsgCount; i++)
		{
			if (hTCCMsg == NULL) break;

			p = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);
			if (p->uiArrange != SERVICE_ARRANGE_TYPE_DISCARD)
				p->uiArrange = iType;
			hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
		}
		err = i;
	}

	return err;
}

void TCCISDBT_ChannelDB_ClearOption (SERVICE_FOUND_INFO_T *pSvcInfo, SERVICE_FOUND_INFO_T *pDBInfo)
{
	ST_CHANNEL_DATA *p;
	void *hTCCMsg;
	int iCount, i;

	hTCCMsg = g_pChannelMsg->TccMessageHandleFirst();
	iCount = g_pChannelMsg->TccMessageGetCount();
	for(i=0; i < iCount; i++)
	{
		if (hTCCMsg == NULL) break;
		p = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);
		p->uiArrange = SERVICE_ARRANGE_TYPE_NOTHING;

		hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
	}

	iCount = pSvcInfo->found_no;
	if (iCount > 0 && pSvcInfo->pServiceFound != NULL) {
		for(i=0; i < iCount; i++)
			pSvcInfo->pServiceFound[i].uiOption = 0;
	}

	iCount = pDBInfo->found_no;
	if (iCount > 0 && pDBInfo->pServiceFound != NULL) {
		for(i=0; i < iCount; i++)
			pDBInfo->pServiceFound[i].uiOption = 1;
	}
}

int TCCISDBT_ChannelDB_Is1SegService(unsigned int channel, unsigned int country_code, unsigned int service_id)
{
	ST_CHANNEL_DATA *p;
	void *hTCCMsg;
	int iTCCMsgCount, i, rtn=0;

	hTCCMsg = g_pChannelMsg->TccMessageHandleFirst();
	iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();
	for(i=0; i < iTCCMsgCount; i++)
	{
		if (hTCCMsg == NULL) break;
		p = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);
		if(p->uiCurrentChannelNumber == channel &&
				p->uiCurrentCountryCode == country_code &&
				p->uiServiceID == service_id)
		{
			if(p->uiServiceType == 0xC0) rtn = 1;
			break;
		}

		hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
	}
	return rtn;
}

int TCCISDBT_ChannelDB_Get1SegServiceCount(unsigned int channel, unsigned int country_code)
{
	ST_CHANNEL_DATA *p;
	void *hTCCMsg;
	int iTCCMsgCount, i, rtn=0;

	hTCCMsg = g_pChannelMsg->TccMessageHandleFirst();
	iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();
	for(i=0; i < iTCCMsgCount; i++)
	{
		if (hTCCMsg == NULL) break;
		p = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);
		if(p->uiCurrentChannelNumber == channel &&
				p->uiCurrentCountryCode == country_code)
		{
			if(p->uiServiceType == 0xC0) rtn++;
		}

		hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
	}
	return rtn;
}

int TCCISDBT_ChannelDB_GetDataServiceCount(unsigned int channel, unsigned int country_code)
{
	ST_CHANNEL_DATA *p;
	void *hTCCMsg;
	int iTCCMsgCount, i, rtn=0;

	hTCCMsg = g_pChannelMsg->TccMessageHandleFirst();
	iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();
	for(i=0; i < iTCCMsgCount; i++)
	{
		if (hTCCMsg == NULL) break;
		p = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);
		if(p->uiCurrentChannelNumber == channel &&
				p->uiCurrentCountryCode == country_code)
		{
			if(p->uiServiceType == SERVICE_TYPE_1SEG) {
				if (p->uiThreeDigitNumber >= 200 && p->uiThreeDigitNumber < 600){
					 rtn++;
				}
			}
		}

		hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
	}
	return rtn;
}

int TCCISDBT_ChannelDB_GetSpecialServiceCount(unsigned int channel, unsigned int country_code)
{
	ST_CHANNEL_DATA *p;
	void *hTCCMsg;
	int iTCCMsgCount, i, rtn=0;

	hTCCMsg = g_pChannelMsg->TccMessageHandleFirst();
	iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();
	for(i=0; i < iTCCMsgCount; i++)
	{
		if (hTCCMsg == NULL) break;
		p = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);
		if(p->uiCurrentChannelNumber == channel &&
				p->uiCurrentCountryCode == country_code)
		{
			if(p->uiServiceType == SERVICE_TYPE_TEMP_VIDEO) {
				rtn++;
			}
		}

		hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
	}
	return rtn;
}

int TCCISDBT_ChannelDB_Get1SegServiceID(unsigned int channel, unsigned int country_code, unsigned int *service_id)
{
	ST_CHANNEL_DATA *p;
	void *hTCCMsg;
	int iTCCMsgCount, i, rtn=0, fPartialService;
	unsigned int *pSvcId = service_id;

	if (service_id == NULL) return rtn;

	hTCCMsg = g_pChannelMsg->TccMessageHandleFirst();
	iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();
	for(i=0; i < iTCCMsgCount; i++)
	{
		if (hTCCMsg == NULL) break;
		p = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);
		if(p->uiCurrentChannelNumber == channel &&
				p->uiCurrentCountryCode == country_code)
		{
			if(p->uiServiceType == 0xC0) {
				*pSvcId++ = p->uiServiceID;
				rtn++;
				if (rtn >= 8) break;
			}
		}
		hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
	}
	return rtn;
}

int TCCISDBT_ChannelDB_GetDataServiceID(unsigned int channel, unsigned int country_code, unsigned int *service_id)
{
	ST_CHANNEL_DATA *p;
	void *hTCCMsg;
	int iTCCMsgCount, i, rtn=0;

	*service_id = 0;

	hTCCMsg = g_pChannelMsg->TccMessageHandleFirst();
	iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();
	for(i=0; i < iTCCMsgCount; i++)
	{
		if (hTCCMsg == NULL) break;
		p = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);
		if(p->uiCurrentChannelNumber == channel &&
				p->uiCurrentCountryCode == country_code)
		{
			if(p->uiServiceType == SERVICE_TYPE_1SEG) {
				if (p->uiThreeDigitNumber >= 200 && p->uiThreeDigitNumber < 600){
					*service_id = p->uiServiceID;
					break;
				}
			}
		}

		hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
	}
	return 0;
}

unsigned int TCCISDBT_ChannelDB_GetNetworkID (int channel, int country_code, int service_id)
{
	ST_CHANNEL_DATA *p;
	unsigned int uiNetworkID;
	void *hTCCMsg;
	int iTCCMsgCount;

	if (service_id != -1) {
		p = ChannelDB_GetChannelData(channel, country_code, service_id);
		if (p)
			uiNetworkID = p->uiNetworkID;
		else
			uiNetworkID = 0;
	} else {
		hTCCMsg = g_pChannelMsg->TccMessageHandleFirst();
		if (hTCCMsg == NULL) {
			uiNetworkID = 0;
		} else {
			p = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);
			if ((p->uiCurrentChannelNumber == (unsigned int)channel)
				&& (p->uiCurrentCountryCode == (unsigned int)country_code)){
				uiNetworkID = p->uiNetworkID;
			} else {
				uiNetworkID = 0;
			}
		}
	}
	return uiNetworkID;
}

/* -------- custom search for jk : start ----------------*/
int TCCISDBT_ChannelDB_CustomSearch_DelInfo(sqlite3 *sqlhandle, int preset, int network_id)
{
	int err = SQLITE_ERROR;
	char szSQL[1024];

	if(sqlhandle==NULL || (preset==-1 && network_id==-1)) return err;

	if (network_id==-1) {
		sprintf(szSQL, "DELETE FROM channels WHERE preset=%d", preset);
	} else {
		sprintf(szSQL, "DELETE FROM channels WHERE preset=%d AND networkID=%d", preset, network_id);
	}
	err = TCC_SQLITE3_EXEC (sqlhandle, szSQL, NULL, NULL, NULL);
	return err;
}
int TCCISDBT_ChannelDB_CustomSearch_SearchSameServiceFromDB (sqlite3 *sqlhandle, int channel, int preset, int network_id, int service_id)
{
	int err = SQLITE_ERROR;
	char szSQL[1024];
	sqlite3_stmt *stmt;
	int channel_id = 0;

	if (channel != -1)
		sprintf(szSQL, "SELECT _id FROM channels WHERE channelNumber=%d AND preset=%d AND networkID=%d AND serviceID=%d", channel, preset, network_id, service_id);
	else
		sprintf(szSQL, "SELECT _id FROM channels WHERE preset=%d AND networkID=%d AND serviceID=%d", preset, network_id, service_id);
	if (sqlite3_prepare_v2(sqlhandle, szSQL, -1, &stmt, NULL) == SQLITE_OK) {
		err = sqlite3_step(stmt);
		if (err == SQLITE_ROW)
			channel_id = sqlite3_column_int(stmt, 0);
	}
	if (stmt) {
		sqlite3_finalize(stmt);
		stmt = NULL;
	}
	return channel_id;
}
int TCCISDBT_ChannelDB_CustomSearch_SearchSameService(sqlite3 *sqlhandle, int channel, int preset, int network_id, int *pfullseg_id, int *poneseg_id, int foverwrite)
{
	ST_CHANNEL_DATA *p;
	void *hTCCMsg;
	int iTCCMsgCount, i, fFullSearch, fOneSearch;
	int channel_id;

	if (sqlhandle == NULL || pfullseg_id==NULL || poneseg_id==NULL) return SQLITE_ERROR;

	fFullSearch = fOneSearch = 1;
	*pfullseg_id = 0;
	*poneseg_id = 0;

	hTCCMsg = g_pChannelMsg->TccMessageHandleFirst();
	iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();
	for(i=0; i < iTCCMsgCount; i++)
	{
		if (hTCCMsg == NULL) break;
		p = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);
		if (p->uiArrange != SERVICE_ARRANGE_TYPE_DISCARD) {
			if (fFullSearch) {
				if (p->uiServiceType == 0x01) {
					channel_id = TCCISDBT_ChannelDB_CustomSearch_SearchSameServiceFromDB (sqlhandle, channel, preset, network_id, p->uiServiceID);
					if (channel_id != 0) {
						if (*pfullseg_id == 0) *pfullseg_id = channel_id;
						if (foverwrite) p->uiArrange = SERVICE_ARRANGE_TYPE_REPLACE;
					}
				}
			}
			if (fOneSearch) {
				if (p->uiServiceType == 0xC0) {
					channel_id = TCCISDBT_ChannelDB_CustomSearch_SearchSameServiceFromDB (sqlhandle, channel, preset, network_id, p->uiServiceID);
					if (channel_id != 0) {
						if (*poneseg_id == 0) *poneseg_id = channel_id;
						if (foverwrite) p->uiArrange = SERVICE_ARRANGE_TYPE_REPLACE;
					}
				}
			}
		}
		hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
	}
	if (*pfullseg_id != 0 || *poneseg_id != 0)
	{
		printf("[CustomSearch_SearchSameService] fullseg_id(%d) oneseg_id(%d) \n", *pfullseg_id, *poneseg_id);
		return SQLITE_OK;
	}
	else
		return SQLITE_ERROR;
}

int TCCISDBT_ChannelDB_CustomSearch_GetChannelInfoReplace(SERVICE_FOUND_INFO_T *pSvc)
{
	ST_CHANNEL_DATA *p;
	void *hTCCMsg;
	int iTCCMsgCount;
	int svc_count = 0, i;

	if (pSvc==NULL) return svc_count;

	pSvc->pServiceFound = NULL;
	pSvc->found_no = 0;

	hTCCMsg = g_pChannelMsg->TccMessageHandleFirst();
	iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();

	if (iTCCMsgCount > 32)	iTCCMsgCount = 32;	//limit count of service
	if (iTCCMsgCount > 0) {
		pSvc->pServiceFound = (struct _tag_service_found_info *)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(struct _tag_service_found_info) * iTCCMsgCount);
		if(pSvc->pServiceFound != NULL) {
			for(i=0; i < iTCCMsgCount; i++)
			{
				if (hTCCMsg == NULL) break;
				p = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);


				printf("[GetChannelInfoReplace] Arrange(%d) Ch(%d) (tsid)%d (Sid)%d (stype)%d \n",
					p->uiArrange, p->uiCurrentChannelNumber, p->uiNetworkID, p->uiServiceID, p->uiServiceType);


				if (p->uiArrange == SERVICE_ARRANGE_TYPE_REPLACE) {
					pSvc->pServiceFound[svc_count].uiCurrentChannelNumber = p->uiCurrentChannelNumber;
					pSvc->pServiceFound[svc_count].uiCurrentCountryCode = p->uiCurrentCountryCode;
					pSvc->pServiceFound[svc_count].uiNetworkID = p->uiNetworkID;
					pSvc->pServiceFound[svc_count].uiServiceID = p->uiServiceID;
					pSvc->pServiceFound[svc_count].uiServiceType = p->uiServiceType;
					pSvc->pServiceFound[svc_count].uiOption = 0;
					pSvc->found_no++;
					svc_count++;
				}

				hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
			}
		}
	}
	return svc_count;
}
int TCCISDBT_ChannelDB_CustomSearch_ChannelInfoReplace(sqlite3 *sqlhandle, SERVICE_FOUND_INFO_T *pSvcInfo, int preset, int tsid)
{
	ST_CHANNEL_DATA *p;
	int cnt;
	char szSQL[1024];
	sqlite3_stmt *stmt;

	if ((pSvcInfo != NULL) && (sqlhandle != NULL) && (pSvcInfo->found_no != 0) && (pSvcInfo->pServiceFound != NULL)) {
		for (cnt=0; cnt < pSvcInfo->found_no; cnt++)
		{
			p = ChannelDB_GetChannelData(pSvcInfo->pServiceFound[cnt].uiCurrentChannelNumber, pSvcInfo->pServiceFound[cnt].uiCurrentCountryCode,
				pSvcInfo->pServiceFound[cnt].uiServiceID);
			if (p)
			{
				if(TCC_Isdbt_IsSupportPrimaryService())
				{
					sprintf(szSQL, "UPDATE channels SET channelNumber=%d, \
								audioPID=%d, \
								videoPID=%d, \
								stPID=%d, \
								siPID=%d, \
								PMT_PID=%d, \
								CA_ECM_PID=%d, \
								AC_ECM_PID=%d, \
								remoconID=%d, \
								serviceType=%d, \
								serviceID=%d, \
								regionID=%d, \
								threedigitNumber=%d, \
								TStreamID=%d, \
								berAVG=%d, \
								areaCode=%d, \
								frequency=?, \
								affiliationID=?, \
								channelName=?, \
								TSName=?, \
								uiTotalAudio_PID=%d, \
								uiTotalVideo_PID=%d, \
								DWNL_ID=%d, \
								LOGO_ID=%d, \
								strLogo_Char=?, \
								usTotalCntSubtLang=%d, \
								uiPCR_PID=%d, \
								ucAudio_StreamType=%d, \
								ucVideo_StreamType=%d, \
								usOrig_Net_ID=%d, \
								primaryServiceFlag=%d \
						WHERE preset=%d AND networkID=%d AND serviceID=%d",
						p->uiCurrentChannelNumber, p->uiAudioPID, p->uiVideoPID, p->uiStPID, p->uiSiPID, p->uiPMTPID, p->uiCAECMPID, p->uiACECMPID, p->uiRemoconID, p->uiServiceType,
						p->uiServiceID, p->uiRegionID, p->uiThreeDigitNumber, p->uiTStreamID, p->uiBerAvg, p->uiAreaCode,
						p->uiTotalAudio_PID, p->uiTotalVideo_PID, p->DWNL_ID, p->LOGO_ID, p->usTotalCntSubtLang, p->uiPCR_PID,
						p->ucAudio_StreamType, p->ucVideo_StreamType, p->usOrig_Net_ID, p->uiPrimaryServiceFlag,
						preset, tsid, p->uiServiceID);
				}
				else
				{
					sprintf(szSQL, "UPDATE channels SET channelNumber=%d, \
									audioPID=%d, \
									videoPID=%d, \
									stPID=%d, \
									siPID=%d, \
									PMT_PID=%d, \
									CA_ECM_PID=%d, \
									AC_ECM_PID=%d, \
									remoconID=%d, \
									serviceType=%d, \
									serviceID=%d, \
									regionID=%d, \
									threedigitNumber=%d, \
									TStreamID=%d, \
									berAVG=%d, \
									areaCode=%d, \
									frequency=?, \
									affiliationID=?, \
									channelName=?, \
									TSName=?, \
									uiTotalAudio_PID=%d, \
									uiTotalVideo_PID=%d, \
									DWNL_ID=%d, \
									LOGO_ID=%d, \
									strLogo_Char=?, \
									usTotalCntSubtLang=%d, \
									uiPCR_PID=%d, \
									ucAudio_StreamType=%d, \
									ucVideo_StreamType=%d, \
									usOrig_Net_ID=%d \
							WHERE preset=%d AND networkID=%d AND serviceID=%d",
						p->uiCurrentChannelNumber, p->uiAudioPID, p->uiVideoPID, p->uiStPID, p->uiSiPID, p->uiPMTPID, p->uiCAECMPID, p->uiACECMPID, p->uiRemoconID, p->uiServiceType,
						p->uiServiceID, p->uiRegionID, p->uiThreeDigitNumber, p->uiTStreamID, p->uiBerAvg, p->uiAreaCode,
						p->uiTotalAudio_PID, p->uiTotalVideo_PID, p->DWNL_ID, p->LOGO_ID, p->usTotalCntSubtLang, p->uiPCR_PID,
						p->ucAudio_StreamType, p->ucVideo_StreamType, p->usOrig_Net_ID,
						preset, tsid, p->uiServiceID);
				}

				sqlite3_prepare_v2(sqlhandle, szSQL, -1, &stmt, NULL);
				sqlite3_reset(stmt);
				sqlite3_bind_blob(stmt, 1, &(p->stDescTDSD), sizeof(T_DESC_TDSD), SQLITE_TRANSIENT);
				sqlite3_bind_blob(stmt, 2, &(p->stDescEBD), sizeof(T_DESC_EBD), SQLITE_TRANSIENT);
				sqlite3_bind_text16(stmt, 3, p->pusChannelName, -1, SQLITE_STATIC);
				sqlite3_bind_text16(stmt, 4, p->pusTSName, -1, SQLITE_STATIC);
				sqlite3_bind_text16(stmt, 5, p->strLogo_Char, -1, SQLITE_STATIC);
				sqlite3_step(stmt);
				sqlite3_finalize(stmt);
			}
		}
	}
	return SQLITE_OK;
}
void TCCISDBT_ChannelDB_CustomSearch_UpdateInfo (sqlite3 *sqlhandle, int preset, CUSTOMSEARCH_FOUND_INFO_T *pFoundInfo)
{
	ST_CHANNEL_DATA *p;
	int iTCCMsgCount;
	int svc_count = 0, i;
	char szSQL[1024];
	sqlite3_stmt *stmt;
	int fullseg_flag, oneseg_flag;
	int row_id;

	if (sqlhandle==NULL || pFoundInfo==NULL) return;

	fullseg_flag = oneseg_flag = 1;
	memset (pFoundInfo, 0, sizeof(CUSTOMSEARCH_FOUND_INFO_T));

	svc_count = 0;
	iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();
	for(i=0; i < iTCCMsgCount; i++)
	{
		p = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageGet();
		if (p == NULL) continue;
		if (p->uiServiceType != SERVICE_TYPE_FSEG && p->uiServiceType != SERVICE_TYPE_1SEG) continue;
		if (p->uiArrange == SERVICE_ARRANGE_TYPE_DISCARD) continue;

		if(TCC_Isdbt_IsSupportPrimaryService())
		{
			sprintf(szSQL,
				"INSERT INTO channels (channelNumber, countryCode, audioPID, videoPID, stPID, siPID, PMT_PID, CA_ECM_PID, AC_ECM_PID, remoconID, serviceType, \
					serviceID, regionID, threedigitNumber, TStreamID, berAVG, preset, networkID, areaCode, uiTotalAudio_PID, uiTotalVideo_PID, frequency, affiliationID, channelName, TSName, \
					DWNL_ID, LOGO_ID, strLogo_Char, usTotalCntSubtLang, uiPCR_PID, ucAudio_StreamType, ucVideo_StreamType, usOrig_Net_ID, primaryServiceFlag) \
					VALUES (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d,%d, %d, %d, %d, %d, %d, %d, %d, ?, ?, ?, ?, %d, %d, ?, %d, %d, %d, %d, %d, %d)",
				p->uiCurrentChannelNumber, p->uiCurrentCountryCode, p->uiAudioPID, p->uiVideoPID, p->uiStPID, p->uiSiPID,p->uiPMTPID, p->uiCAECMPID, p->uiACECMPID, p->uiRemoconID, p->uiServiceType,
				p->uiServiceID, p->uiRegionID, p->uiThreeDigitNumber, p->uiTStreamID, p->uiBerAvg, preset, p->uiNetworkID, p->uiAreaCode, p->uiTotalAudio_PID, p->uiTotalVideo_PID, p->uiPCR_PID,
				p->DWNL_ID, p->LOGO_ID, p->usTotalCntSubtLang, p->ucAudio_StreamType, p->ucVideo_StreamType, p->usOrig_Net_ID, p->uiPrimaryServiceFlag);
		}
		else
		{
			sprintf(szSQL,
				"INSERT INTO channels (channelNumber, countryCode, audioPID, videoPID, stPID, siPID, PMT_PID, CA_ECM_PID, AC_ECM_PID, remoconID, serviceType, \
					serviceID, regionID, threedigitNumber, TStreamID, berAVG, preset, networkID, areaCode, uiTotalAudio_PID, uiTotalVideo_PID, frequency, affiliationID, channelName, TSName, \
					DWNL_ID, LOGO_ID, strLogo_Char, usTotalCntSubtLang, uiPCR_PID, ucAudio_StreamType, ucVideo_StreamType, usOrig_Net_ID) \
					VALUES (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d,%d, %d, %d, %d, %d, %d, %d, %d, ?, ?, ?, ?, %d, %d, ?, %d, %d, %d, %d, %d)",
				p->uiCurrentChannelNumber, p->uiCurrentCountryCode, p->uiAudioPID, p->uiVideoPID, p->uiStPID, p->uiSiPID,p->uiPMTPID, p->uiCAECMPID, p->uiACECMPID, p->uiRemoconID, p->uiServiceType,
				p->uiServiceID, p->uiRegionID, p->uiThreeDigitNumber, p->uiTStreamID, p->uiBerAvg, preset, p->uiNetworkID, p->uiAreaCode, p->uiTotalAudio_PID, p->uiTotalVideo_PID, p->uiPCR_PID,
				p->DWNL_ID, p->LOGO_ID, p->usTotalCntSubtLang, p->ucAudio_StreamType, p->ucVideo_StreamType, p->usOrig_Net_ID);
		}

		sqlite3_prepare_v2(sqlhandle, szSQL, -1, &stmt, NULL);
		sqlite3_reset(stmt);
		sqlite3_bind_blob(stmt, 1, &(p->stDescTDSD), sizeof(T_DESC_TDSD), SQLITE_TRANSIENT);
		sqlite3_bind_blob(stmt, 2, &(p->stDescEBD), sizeof(T_DESC_EBD), SQLITE_TRANSIENT);
		sqlite3_bind_text16(stmt, 3, p->pusChannelName, -1, SQLITE_STATIC);
		sqlite3_bind_text16(stmt, 4, p->pusTSName,-1, SQLITE_STATIC);
		sqlite3_bind_text16(stmt, 5, p->strLogo_Char, -1, SQLITE_STATIC);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);

		row_id = sqlite3_last_insert_rowid(sqlhandle);
		if (row_id > 0) {
			pFoundInfo->all_id[svc_count++] = row_id;
			if (fullseg_flag) {
				if (p->uiServiceType==0x01) {
					pFoundInfo->fullseg_id = row_id;
					fullseg_flag = 0;
				}
			}
			if (oneseg_flag) {
				if (p->uiServiceType==0xC0) {
					pFoundInfo->oneseg_id= row_id;
					oneseg_flag = 0;
				}
			}
		}
	}
	pFoundInfo->total_svc = svc_count;
}
/* ----------- custom search for JK : end -------------*/

/* to do -make macro or sub function to handle a snippet of same code used two or more times */
int TCCISDBT_ChannelDB_UpdateSQLfile(sqlite3 *sqlhandle, int channel, int country_code, int service_id, int del_option)
{
	int iTCCMsgCount, i, cnt;
	ST_CHANNEL_DATA *p;
	char szSQL[1024];
	sqlite3_stmt *stmt;
	SERVICE_FOUND_INFO_T stSvcInfo, stDBInfo;
	void *hTCCMsg = (void *)NULL;

	int err = -1;

	if (g_pChannelMsg == NULL)
	{
		ERR_MSG("[%s] Error g_pChannelMsg is NULL\n", __func__);
		return SQLITE_ERROR;
	}

	if (service_id == (int)-1)    //update all channel information
	{
		iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();
		if(iTCCMsgCount <= 0)
			return SQLITE_ERROR;

		stSvcInfo.found_no = 0; stSvcInfo.pServiceFound = NULL;
		stDBInfo.found_no = 0; stDBInfo.pServiceFound = NULL;

		TCCISDBT_ChannelDB_GetChannelInfo(&stSvcInfo);
		if(stSvcInfo.found_no == 0)
			return SQLITE_ERROR;

		hTCCMsg = g_pChannelMsg->TccMessageHandleFirst();
		if (hTCCMsg == NULL)
			return SQLITE_ERROR;

		p = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);
		TCCISDBT_ChannelDB_GetChannelInfoDB (sqlhandle, &stDBInfo, p->uiCurrentCountryCode, p->uiNetworkID);
		TCCISDBT_ChannelDB_ChannelInfoArrange(&stSvcInfo, &stDBInfo);

		/* --- to add up all channel informatin to DB in purpose of test, you have to call TCCISDBT_ChannelDB_ClearOption()*/
		//TCCISDBT_ChannelDB_ClearOption(&stSvcInfo, &stDBInfo);

		for (i = 0; i < iTCCMsgCount; i++)
		{
			if(del_option == 0)
			{
				p = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);
			}
			else
			{
				p = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageGet();
			}

			if (p == NULL || p->uiArrange==SERVICE_ARRANGE_TYPE_DISCARD)
			{
				/* Original Null Pointer Dereference
				ALOGE("In %s, ch discarded, netowrk_id=0x%x, service_id=0x%x", __func__, p->uiNetworkID, p->uiServiceID);
				*/
				// jini 9th : Ok
				if (p != NULL)
					ALOGE("In %s, ch discarded, netowrk_id=0x%x, service_id=0x%x", __func__, p->uiNetworkID, p->uiServiceID);
				else
					ALOGE("In %s, ST_CHANNEL_DATA is NULL", __func__);

				if(del_option == 0)
				{
					hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
				}
				continue;
			}

			if((&p->uiPMTPID != NULL) && (p->uiServiceType == SERVICE_TYPE_FSEG||p->uiServiceType == SERVICE_TYPE_TEMP_VIDEO||p->uiServiceType == SERVICE_TYPE_1SEG))
			{
				if (p->uiArrange == SERVICE_ARRANGE_TYPE_REPLACE)
				{
					//if it's same service with different frequency

					if(TCC_Isdbt_IsSupportPrimaryService())
					{
						sprintf(szSQL, "UPDATE channels SET channelNumber=%d, \
										audioPID=%d, \
										videoPID=%d, \
										stPID=%d, \
										siPID=%d, \
										PMT_PID=%d, \
										CA_ECM_PID=%d, \
										AC_ECM_PID=%d, \
										remoconID=%d, \
										serviceType=%d, \
										regionID=%d, \
										threedigitNumber=%d, \
										TStreamID=%d, \
										berAVG=%d, \
										areaCode=%d, \
										uiTotalAudio_PID=%d, \
										uiTotalVideo_PID=%d, \
										frequency=?, \
										affiliationID=?, \
										channelName=?, \
										TSName=?, \
										DWNL_ID=%d, \
										LOGO_ID=%d, \
										strLogo_Char=?, \
										usTotalCntSubtLang=%d, \
										uiPCR_PID=%d, \
										ucAudio_StreamType=%d, \
										ucVideo_StreamType=%d, \
										usOrig_Net_ID=%d, \
										primaryServiceFlag=%d \
								WHERE countryCode=%d AND networkID=%d AND serviceID=%d",
							p->uiCurrentChannelNumber, p->uiAudioPID, p->uiVideoPID, p->uiStPID, p->uiSiPID, p->uiPMTPID, p->uiCAECMPID, p->uiACECMPID, p->uiRemoconID, p->uiServiceType,
							p->uiRegionID, p->uiThreeDigitNumber, p->uiTStreamID, p->uiBerAvg, p->uiAreaCode, p->uiTotalAudio_PID, p->uiTotalVideo_PID,
							p->DWNL_ID, p->LOGO_ID, p->usTotalCntSubtLang, p->uiPCR_PID, p->ucAudio_StreamType, p->ucVideo_StreamType, p->usOrig_Net_ID, p->uiPrimaryServiceFlag,
							p->uiCurrentCountryCode, p->uiNetworkID, p->uiServiceID);
					}
					else
					{
						sprintf(szSQL, "UPDATE channels SET channelNumber=%d, \
										audioPID=%d, \
										videoPID=%d, \
										stPID=%d, \
										siPID=%d, \
										PMT_PID=%d, \
										CA_ECM_PID=%d, \
										AC_ECM_PID=%d, \
										remoconID=%d, \
										serviceType=%d, \
										regionID=%d, \
										threedigitNumber=%d, \
										TStreamID=%d, \
										berAVG=%d, \
										areaCode=%d, \
										uiTotalAudio_PID=%d, \
										uiTotalVideo_PID=%d, \
										frequency=?, \
										affiliationID=?, \
										channelName=?, \
										TSName=?, \
										DWNL_ID=%d, \
										LOGO_ID=%d, \
										strLogo_Char=?, \
										usTotalCntSubtLang=%d, \
										uiPCR_PID=%d, \
										ucAudio_StreamType=%d, \
										ucVideo_StreamType=%d, \
										usOrig_Net_ID=%d \
								WHERE countryCode=%d AND networkID=%d AND serviceID=%d",
							p->uiCurrentChannelNumber, p->uiAudioPID, p->uiVideoPID, p->uiStPID, p->uiSiPID, p->uiPMTPID, p->uiCAECMPID, p->uiACECMPID, p->uiRemoconID, p->uiServiceType,
							p->uiRegionID, p->uiThreeDigitNumber, p->uiTStreamID, p->uiBerAvg, p->uiAreaCode, p->uiTotalAudio_PID, p->uiTotalVideo_PID,
							p->DWNL_ID, p->LOGO_ID, p->usTotalCntSubtLang, p->uiPCR_PID, p->ucAudio_StreamType, p->ucVideo_StreamType, p->usOrig_Net_ID,
							p->uiCurrentCountryCode, p->uiNetworkID, p->uiServiceID);
					}

					sqlite3_prepare_v2(sqlhandle, szSQL, -1, &stmt, NULL);
					sqlite3_reset(stmt);
					sqlite3_bind_blob(stmt, 1, &(p->stDescTDSD), sizeof(T_DESC_TDSD), SQLITE_TRANSIENT);
					sqlite3_bind_blob(stmt, 2, &(p->stDescEBD), sizeof(T_DESC_EBD), SQLITE_TRANSIENT);
					sqlite3_bind_text16(stmt, 3, p->pusChannelName, -1, SQLITE_STATIC);
					sqlite3_bind_text16(stmt, 4, p->pusTSName, -1, SQLITE_STATIC);
					sqlite3_bind_text16(stmt, 5, p->strLogo_Char, -1, SQLITE_STATIC);
					sqlite3_step(stmt);
					sqlite3_finalize(stmt);
				}
				else if (p->uiArrange == SERVICE_ARRANGE_TYPE_HANDOVER)
				{
					if(TCC_Isdbt_IsSupportPrimaryService())
					{
						sprintf(szSQL,
							"INSERT INTO channels (channelNumber, countryCode, audioPID, videoPID, stPID, siPID, PMT_PID, CA_ECM_PID, AC_ECM_PID, remoconID, serviceType, serviceID, regionID, threedigitNumber, TStreamID, berAVG, preset, networkID, areaCode, uiTotalAudio_PID, uiTotalVideo_PID, frequency, affiliationID, channelName, TSName, DWNL_ID, LOGO_ID, strLogo_Char, usTotalCntSubtLang, uiPCR_PID, ucAudio_StreamType, ucVideo_StreamType, usOrig_Net_ID, primaryServiceFlag) VALUES (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d,%d, %d, %d, %d, %d, %d, %d, ?, ?, ?, ?, %d, %d, ?, %d, %d, %d, %d, %d, %d)",
							p->uiCurrentChannelNumber, p->uiCurrentCountryCode, p->uiAudioPID, p->uiVideoPID, p->uiStPID, p->uiSiPID,
							p->uiPMTPID, p->uiCAECMPID, p->uiACECMPID, p->uiRemoconID, p->uiServiceType, p->uiServiceID, p->uiRegionID, p->uiThreeDigitNumber, p->uiTStreamID, p->uiBerAvg, RECORD_PRESET_INITSEARCH, p->uiNetworkID, p->uiAreaCode, p->uiTotalAudio_PID, p->uiTotalVideo_PID,
							p->DWNL_ID, p->LOGO_ID, p->usTotalCntSubtLang, p->uiPCR_PID, p->ucAudio_StreamType, p->ucVideo_StreamType, p->usOrig_Net_ID, p->uiPrimaryServiceFlag);
					}
					else
					{
						sprintf(szSQL,
							"INSERT INTO channels (channelNumber, countryCode, audioPID, videoPID, stPID, siPID, PMT_PID, CA_ECM_PID, AC_ECM_PID, remoconID, serviceType, serviceID, regionID, threedigitNumber, TStreamID, berAVG, preset, networkID, areaCode, uiTotalAudio_PID, uiTotalVideo_PID, frequency, affiliationID, channelName, TSName, DWNL_ID, LOGO_ID, strLogo_Char, usTotalCntSubtLang, uiPCR_PID, ucAudio_StreamType, ucVideo_StreamType, usOrig_Net_ID) VALUES (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d,%d, %d, %d, %d, %d, %d, %d, ?, ?, ?, ?, %d, %d, ?, %d, %d, %d, %d, %d)",
							p->uiCurrentChannelNumber, p->uiCurrentCountryCode, p->uiAudioPID, p->uiVideoPID, p->uiStPID, p->uiSiPID,
							p->uiPMTPID, p->uiCAECMPID, p->uiACECMPID, p->uiRemoconID, p->uiServiceType, p->uiServiceID, p->uiRegionID, p->uiThreeDigitNumber, p->uiTStreamID, p->uiBerAvg, RECORD_PRESET_INITSEARCH, p->uiNetworkID, p->uiAreaCode, p->uiTotalAudio_PID, p->uiTotalVideo_PID,
							p->DWNL_ID, p->LOGO_ID, p->usTotalCntSubtLang, p->uiPCR_PID, p->ucAudio_StreamType, p->ucVideo_StreamType, p->usOrig_Net_ID);
					}

					sqlite3_prepare_v2(sqlhandle, szSQL, -1, &stmt, NULL);
					sqlite3_reset(stmt);
					sqlite3_bind_blob(stmt, 1, &(p->stDescTDSD), sizeof(T_DESC_TDSD), SQLITE_TRANSIENT);
					sqlite3_bind_blob(stmt, 2, &(p->stDescEBD), sizeof(T_DESC_EBD), SQLITE_TRANSIENT);
					sqlite3_bind_text16(stmt, 3, p->pusChannelName, -1, SQLITE_STATIC);
					sqlite3_bind_text16(stmt, 4, p->pusTSName,-1, SQLITE_STATIC);
					sqlite3_bind_text16(stmt, 5, p->strLogo_Char,-1, SQLITE_STATIC);
					sqlite3_step(stmt);
					sqlite3_finalize(stmt);
				}
				else if (!TCCISDBT_ChannelDB_ChannelInfoArrangeNewSvc(&stSvcInfo, p->uiNetworkID, p->uiServiceID))
				{
					//if it's NOT a new service
				#ifdef ISDBT_RESCAN_PROTECT_OLDCHINFO
					if (p->uiArrange == SERVICE_ARRANGE_TYPE_RESCAN)
					{
						//do nothing to maintain previous information
					}
					else
					{
				#endif
						if(TCC_Isdbt_IsSupportPrimaryService())
						{
							sprintf(szSQL, "UPDATE channels SET audioPID=%d, videoPID=%d, stPID=%d, siPID=%d, PMT_PID=%d, CA_ECM_PID=%d, AC_ECM_PID=%d, remoconID=%d, serviceType=%d, regionID=%d, \
								threedigitNumber=%d, TStreamID=%d, berAVG=%d, areaCode=%d, uiTotalAudio_PID=%d, uiTotalVideo_PID=%d, frequency=?, \
								affiliationID=?, channelName=?, TSName=?, DWNL_ID=%d, LOGO_ID=%d, strLogo_Char=?, usTotalCntSubtLang=%d, uiPCR_PID=%d, ucAudio_StreamType=%d, ucVideo_StreamType=%d, usOrig_Net_ID=%d, primaryServiceFlag=%d \
								WHERE channelNumber=%d AND countryCode=%d AND networkID=%d AND serviceID=%d",
								p->uiAudioPID, p->uiVideoPID, p->uiStPID, p->uiSiPID, p->uiPMTPID, p->uiCAECMPID, p->uiACECMPID, p->uiRemoconID, p->uiServiceType,
								p->uiRegionID, p->uiThreeDigitNumber, p->uiTStreamID, p->uiBerAvg, p->uiAreaCode, p->uiTotalAudio_PID, p->uiTotalVideo_PID,
								p->DWNL_ID, p->LOGO_ID, p->usTotalCntSubtLang, p->uiPCR_PID, p->ucAudio_StreamType, p->ucVideo_StreamType, p->usOrig_Net_ID, p->uiPrimaryServiceFlag,
								p->uiCurrentChannelNumber, p->uiCurrentCountryCode, p->uiNetworkID, p->uiServiceID);
						}
						else
						{
							sprintf(szSQL, "UPDATE channels SET audioPID=%d, videoPID=%d, stPID=%d, siPID=%d, PMT_PID=%d, CA_ECM_PID=%d, AC_ECM_PID=%d, remoconID=%d, serviceType=%d, regionID=%d, \
								threedigitNumber=%d, TStreamID=%d, berAVG=%d, areaCode=%d, uiTotalAudio_PID=%d, uiTotalVideo_PID=%d, frequency=?, \
								affiliationID=?, channelName=?, TSName=?, DWNL_ID=%d, LOGO_ID=%d, strLogo_Char=?, usTotalCntSubtLang=%d, uiPCR_PID=%d, ucAudio_StreamType=%d, ucVideo_StreamType=%d, usOrig_Net_ID=%d \
								WHERE channelNumber=%d AND countryCode=%d AND networkID=%d AND serviceID=%d",
								p->uiAudioPID, p->uiVideoPID, p->uiStPID, p->uiSiPID, p->uiPMTPID, p->uiCAECMPID, p->uiACECMPID, p->uiRemoconID, p->uiServiceType,
								p->uiRegionID, p->uiThreeDigitNumber, p->uiTStreamID, p->uiBerAvg, p->uiAreaCode, p->uiTotalAudio_PID, p->uiTotalVideo_PID,
								p->DWNL_ID, p->LOGO_ID, p->usTotalCntSubtLang, p->uiPCR_PID, p->ucAudio_StreamType, p->ucVideo_StreamType, p->usOrig_Net_ID,
								p->uiCurrentChannelNumber, p->uiCurrentCountryCode, p->uiNetworkID, p->uiServiceID);
						}

						sqlite3_prepare_v2(sqlhandle, szSQL, -1, &stmt, NULL);
						sqlite3_reset(stmt);
						sqlite3_bind_blob(stmt, 1, &(p->stDescTDSD), sizeof(T_DESC_TDSD), SQLITE_TRANSIENT);
						sqlite3_bind_blob(stmt, 2, &(p->stDescEBD), sizeof(T_DESC_EBD), SQLITE_TRANSIENT);
						sqlite3_bind_text16(stmt, 3, p->pusChannelName, -1, SQLITE_STATIC);
						sqlite3_bind_text16(stmt, 4, p->pusTSName, -1, SQLITE_STATIC);
						sqlite3_bind_text16(stmt, 5, p->strLogo_Char,-1, SQLITE_STATIC);
						sqlite3_step(stmt);
						sqlite3_finalize(stmt);
				#ifdef ISDBT_RESCAN_PROTECT_OLDCHINFO
					}
				#endif
				}
				else
				{
					//if it's a new service

					if(TCC_Isdbt_IsSupportPrimaryService())
					{
						sprintf(szSQL,
							"INSERT INTO channels (channelNumber, countryCode, audioPID, videoPID, stPID, siPID, PMT_PID, CA_ECM_PID, AC_ECM_PID, remoconID, serviceType, serviceID, regionID, threedigitNumber, TStreamID, berAVG, preset, networkID, areaCode, uiTotalAudio_PID, uiTotalVideo_PID, frequency, affiliationID, channelName, TSName, DWNL_ID, LOGO_ID, strLogo_Char, usTotalCntSubtLang, uiPCR_PID, ucAudio_StreamType, ucVideo_StreamType, usOrig_Net_ID, primaryServiceFlag) VALUES (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d,%d, %d, %d, %d, %d, %d, %d, %d, ?, ?, ?, ?, %d, %d, ?, %d, %d, %d, %d, %d, %d)",
							p->uiCurrentChannelNumber, p->uiCurrentCountryCode, p->uiAudioPID, p->uiVideoPID, p->uiStPID, p->uiSiPID,
							p->uiPMTPID, p->uiCAECMPID, p->uiACECMPID, p->uiRemoconID, p->uiServiceType, p->uiServiceID, p->uiRegionID, p->uiThreeDigitNumber, p->uiTStreamID, p->uiBerAvg, RECORD_PRESET_INITSEARCH, p->uiNetworkID, p->uiAreaCode, p->uiTotalAudio_PID, p->uiTotalVideo_PID,
							p->DWNL_ID, p->LOGO_ID, p->usTotalCntSubtLang, p->uiPCR_PID, p->ucAudio_StreamType, p->ucVideo_StreamType, p->usOrig_Net_ID, p->uiPrimaryServiceFlag);
					}
					else
					{
						sprintf(szSQL,
							"INSERT INTO channels (channelNumber, countryCode, audioPID, videoPID, stPID, siPID, PMT_PID, CA_ECM_PID, AC_ECM_PID, remoconID, serviceType, serviceID, regionID, threedigitNumber, TStreamID, berAVG, preset, networkID, areaCode, uiTotalAudio_PID, uiTotalVideo_PID, frequency, affiliationID, channelName, TSName, DWNL_ID, LOGO_ID, strLogo_Char, usTotalCntSubtLang, uiPCR_PID, ucAudio_StreamType, ucVideo_StreamType, usOrig_Net_ID) VALUES (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d,%d, %d, %d, %d, %d, %d, %d, %d, ?, ?, ?, ?, %d, %d, ?, %d, %d, %d, %d, %d)",
							p->uiCurrentChannelNumber, p->uiCurrentCountryCode, p->uiAudioPID, p->uiVideoPID, p->uiStPID, p->uiSiPID,
							p->uiPMTPID, p->uiCAECMPID, p->uiACECMPID, p->uiRemoconID, p->uiServiceType, p->uiServiceID, p->uiRegionID, p->uiThreeDigitNumber, p->uiTStreamID, p->uiBerAvg, RECORD_PRESET_INITSEARCH, p->uiNetworkID, p->uiAreaCode, p->uiTotalAudio_PID, p->uiTotalVideo_PID,
							p->DWNL_ID, p->LOGO_ID, p->usTotalCntSubtLang, p->uiPCR_PID, p->ucAudio_StreamType, p->ucVideo_StreamType, p->usOrig_Net_ID);
					}

					sqlite3_prepare_v2(sqlhandle, szSQL, -1, &stmt, NULL);
					sqlite3_reset(stmt);
					sqlite3_bind_blob(stmt, 1, &(p->stDescTDSD), sizeof(T_DESC_TDSD), SQLITE_TRANSIENT);
					sqlite3_bind_blob(stmt, 2, &(p->stDescEBD), sizeof(T_DESC_EBD), SQLITE_TRANSIENT);
					sqlite3_bind_text16(stmt, 3, p->pusChannelName, -1, SQLITE_STATIC);
					sqlite3_bind_text16(stmt, 4, p->pusTSName,-1, SQLITE_STATIC);
					sqlite3_bind_text16(stmt, 5, p->strLogo_Char,-1, SQLITE_STATIC);
					sqlite3_step(stmt);
					sqlite3_finalize(stmt);
				}
			}

			if(del_option == 0)
			{
				hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
			}
			else    // Noah, 20180611, Memory Leak / ex) scanning,
			{
				if(p)
				{
					if(p->pusChannelName)
					{
						tcc_mw_free(__FUNCTION__, __LINE__, p->pusChannelName);
						p->pusChannelName = NULL;
					}

					if(p->pusTSName)
					{
						tcc_mw_free(__FUNCTION__, __LINE__, p->pusTSName);
						p->pusTSName = NULL;
					}

					if(p->strLogo_Char)
					{
						tcc_mw_free(__FUNCTION__, __LINE__, p->strLogo_Char);
						p->strLogo_Char = NULL;
					}

					tcc_mw_free(__FUNCTION__, __LINE__, p);
					p = NULL;
				}
			}
		}

	#if 0
		for(i=0; i < stDBInfo.found_no; i++) {
			if (!stDBInfo.pServiceFound[i].uiOption) {
				//delete this service from channel DB because this service is not discovered anymore.
				char *szSQLDel = "DELETE FROM channels WHERE _id=%d";
				sprintf(szSQL, szSQLDel, stDBInfo.pServiceFound[i].rowID);
				TCC_SQLITE3_EXEC(sqlhandle, szSQL, NULL, NULL, NULL);
			}
		}
	#endif

		if (stSvcInfo.pServiceFound != NULL)
			tcc_mw_free(__FUNCTION__, __LINE__, stSvcInfo.pServiceFound);

		if (stDBInfo.pServiceFound != NULL)
			tcc_mw_free(__FUNCTION__, __LINE__, stDBInfo.pServiceFound);
	}
	else    //update any information about specific service_id
	{
		p = ChannelDB_GetChannelData(channel, country_code, service_id);

		if (p != NULL && p->uiArrange != SERVICE_ARRANGE_TYPE_DISCARD &&
			(p->uiServiceType == SERVICE_TYPE_FSEG||p->uiServiceType == SERVICE_TYPE_TEMP_VIDEO||p->uiServiceType == SERVICE_TYPE_1SEG))
		{
			if (p->uiCurrentChannelNumber == (unsigned int)channel&& p->uiCurrentCountryCode == (unsigned int)country_code && p->uiServiceID == (unsigned int)service_id)
			{
				int rowID =-1;

				sprintf(szSQL, "SELECT _id FROM channels WHERE channelNumber=%d AND countryCode=%d AND serviceID=%d", p->uiCurrentChannelNumber, p->uiCurrentCountryCode, p->uiServiceID);
				err = sqlite3_prepare_v2(sqlhandle, szSQL, -1, &stmt, NULL);
				if (err == SQLITE_OK)
				{
					err = sqlite3_step(stmt);
					if (err == SQLITE_ROW)
					{
						rowID = sqlite3_column_int(stmt, 0);
					}
				}

				if(stmt)
				{
					sqlite3_finalize(stmt);
				}

				if(rowID != -1)
				{
					if(TCC_Isdbt_IsSupportPrimaryService())
					{
						sprintf(szSQL, "UPDATE channels SET audioPID=%d, videoPID=%d, stPID=%d, siPID=%d, PMT_PID=%d, CA_ECM_PID=%d, AC_ECM_PID=%d, remoconID=%d, serviceType=%d, regionID=%d, \
								threedigitNumber=%d, TStreamID=%d, berAVG=%d, areaCode=%d, uiTotalAudio_PID=%d, uiTotalVideo_PID=%d, frequency=?, affiliationID=?, channelName=?, TSName=?, DWNL_ID=%d, LOGO_ID=%d, strLogo_Char=?, usTotalCntSubtLang=%d, uiPCR_PID=%d, ucAudio_StreamType=%d, ucVideo_StreamType=%d, usOrig_Net_ID=%d, primaryServiceFlag=%d \
								WHERE channelNumber=%d AND countryCode=%d AND serviceID=%d",
							p->uiAudioPID, p->uiVideoPID, p->uiStPID, p->uiSiPID, p->uiPMTPID, p->uiCAECMPID, p->uiACECMPID, p->uiRemoconID, p->uiServiceType,
							p->uiRegionID, p->uiThreeDigitNumber, p->uiTStreamID, p->uiBerAvg, p->uiAreaCode, p->uiTotalAudio_PID, p->uiTotalVideo_PID, p->DWNL_ID, p->LOGO_ID, p->usTotalCntSubtLang, p->uiPCR_PID, p->ucAudio_StreamType, p->ucVideo_StreamType, p->usOrig_Net_ID, p->uiPrimaryServiceFlag,
							p->uiCurrentChannelNumber, p->uiCurrentCountryCode, p->uiServiceID);
					}
					else
					{
						sprintf(szSQL, "UPDATE channels SET audioPID=%d, videoPID=%d, stPID=%d, siPID=%d, PMT_PID=%d, CA_ECM_PID=%d, AC_ECM_PID=%d, remoconID=%d, serviceType=%d, regionID=%d, \
								threedigitNumber=%d, TStreamID=%d, berAVG=%d, areaCode=%d, uiTotalAudio_PID=%d, uiTotalVideo_PID=%d, frequency=?, affiliationID=?, channelName=?, TSName=?, DWNL_ID=%d, LOGO_ID=%d, strLogo_Char=?, usTotalCntSubtLang=%d, uiPCR_PID=%d, ucAudio_StreamType=%d, ucVideo_StreamType=%d, usOrig_Net_ID=%d \
								WHERE channelNumber=%d AND countryCode=%d AND serviceID=%d",
							p->uiAudioPID, p->uiVideoPID, p->uiStPID, p->uiSiPID, p->uiPMTPID, p->uiCAECMPID, p->uiACECMPID, p->uiRemoconID, p->uiServiceType,
							p->uiRegionID, p->uiThreeDigitNumber, p->uiTStreamID, p->uiBerAvg, p->uiAreaCode, p->uiTotalAudio_PID, p->uiTotalVideo_PID, p->DWNL_ID, p->LOGO_ID, p->usTotalCntSubtLang, p->uiPCR_PID, p->ucAudio_StreamType, p->ucVideo_StreamType, p->usOrig_Net_ID,
							p->uiCurrentChannelNumber, p->uiCurrentCountryCode, p->uiServiceID);
					}

					sqlite3_prepare_v2(sqlhandle, szSQL, -1, &stmt, NULL);
					sqlite3_reset(stmt);
					sqlite3_bind_blob(stmt, 1, &(p->stDescTDSD), sizeof(T_DESC_TDSD), SQLITE_TRANSIENT);
					sqlite3_bind_blob(stmt, 2, &(p->stDescEBD), sizeof(T_DESC_EBD), SQLITE_TRANSIENT);
					sqlite3_bind_text16(stmt, 3, p->pusChannelName, -1, SQLITE_STATIC);
					sqlite3_bind_text16(stmt, 4, p->pusTSName, -1, SQLITE_STATIC);
					sqlite3_bind_text16(stmt, 5, p->strLogo_Char,-1, SQLITE_STATIC);
					sqlite3_step(stmt);
					sqlite3_finalize(stmt);
				}
				else
				{
					if(TCC_Isdbt_IsSupportPrimaryService())
					{
						sprintf(szSQL,
							"INSERT INTO channels (channelNumber, countryCode, audioPID, videoPID, stPID, siPID, PMT_PID, CA_ECM_PID, AC_ECM_PID, remoconID, serviceType, serviceID, regionID, threedigitNumber, TStreamID, berAVG, preset, networkID, areaCode, uiTotalAudio_PID, uiTotalVideo_PID, frequency, affiliationID, channelName, TSName, DWNL_ID, LOGO_ID, strLogo_Char, usTotalCntSubtLang, uiPCR_PID, ucAudio_StreamType, ucVideo_StreamType, usOrig_Net_ID, primaryServiceFlag) VALUES (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d,%d, %d, %d, %d, %d, %d, %d, ?, ?, ?, ?, %d, %d, ?, %d, %d, %d, %d, %d, %d)",
							p->uiCurrentChannelNumber, p->uiCurrentCountryCode, p->uiAudioPID, p->uiVideoPID, p->uiStPID, p->uiSiPID,
							p->uiPMTPID, p->uiCAECMPID, p->uiACECMPID, p->uiRemoconID, p->uiServiceType, p->uiServiceID, p->uiRegionID, p->uiThreeDigitNumber, p->uiTStreamID, p->uiBerAvg, RECORD_PRESET_INITSEARCH, p->uiNetworkID, p->uiAreaCode, p->uiTotalAudio_PID, p->uiTotalVideo_PID, p->DWNL_ID, p->LOGO_ID, p->usTotalCntSubtLang, p->uiPCR_PID, p->ucAudio_StreamType, p->ucVideo_StreamType, p->usOrig_Net_ID, p->uiPrimaryServiceFlag);
					}
					else
					{
						sprintf(szSQL,
							"INSERT INTO channels (channelNumber, countryCode, audioPID, videoPID, stPID, siPID, PMT_PID, CA_ECM_PID, AC_ECM_PID, remoconID, serviceType, serviceID, regionID, threedigitNumber, TStreamID, berAVG, preset, networkID, areaCode, uiTotalAudio_PID, uiTotalVideo_PID, frequency, affiliationID, channelName, TSName, DWNL_ID, LOGO_ID, strLogo_Char, usTotalCntSubtLang, uiPCR_PID, ucAudio_StreamType, ucVideo_StreamType, usOrig_Net_ID) VALUES (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d,%d, %d, %d, %d, %d, %d, %d, ?, ?, ?, ?, %d, %d, ?, %d, %d, %d, %d, %d)",
							p->uiCurrentChannelNumber, p->uiCurrentCountryCode, p->uiAudioPID, p->uiVideoPID, p->uiStPID, p->uiSiPID,
							p->uiPMTPID, p->uiCAECMPID, p->uiACECMPID, p->uiRemoconID, p->uiServiceType, p->uiServiceID, p->uiRegionID, p->uiThreeDigitNumber, p->uiTStreamID, p->uiBerAvg, RECORD_PRESET_INITSEARCH, p->uiNetworkID, p->uiAreaCode, p->uiTotalAudio_PID, p->uiTotalVideo_PID, p->DWNL_ID, p->LOGO_ID, p->usTotalCntSubtLang, p->uiPCR_PID, p->ucAudio_StreamType, p->ucVideo_StreamType, p->usOrig_Net_ID);
					}

					sqlite3_prepare_v2(sqlhandle, szSQL, -1, &stmt, NULL);
					sqlite3_reset(stmt);
					sqlite3_bind_blob(stmt, 1, &(p->stDescTDSD), sizeof(T_DESC_TDSD), SQLITE_TRANSIENT);
					sqlite3_bind_blob(stmt, 2, &(p->stDescEBD), sizeof(T_DESC_EBD), SQLITE_TRANSIENT);
					sqlite3_bind_text16(stmt, 3, p->pusChannelName, -1, SQLITE_STATIC);
					sqlite3_bind_text16(stmt, 4, p->pusTSName, -1, SQLITE_STATIC);
					sqlite3_bind_text16(stmt, 5, p->strLogo_Char,-1, SQLITE_STATIC);
					sqlite3_step(stmt);
					sqlite3_finalize(stmt);
				}
			}
		}
	}

	return SQLITE_OK;
}

int TCCISDBT_ChannelDB_UpdateSQLfile_Partially(sqlite3 *sqlhandle, unsigned int channel, unsigned int country_code, unsigned int service_id)
{
	ST_CHANNEL_DATA *p;
	char szSQL[1024];
	sqlite3_stmt *stmt;

	p = ChannelDB_GetChannelData(channel, country_code, service_id);
	if (p == NULL)
	{
		ERR_MSG("[%s] Error ChannelData is NULL\n", __func__);
		return SQLITE_ERROR;
	}

	if(p->uiPMTPID != 0) {
		if(p->uiCAECMPID != 0 && p->uiACECMPID != 0) {
			sprintf(szSQL, "UPDATE channels SET CA_ECM_PID=%d, AC_ECM_PID=%d, audioPID=%d, videoPID=%d, stPID=%d, siPID=%d, PMT_PID=%d, uiTotalAudio_PID=%d, uiTotalVideo_PID=%d, usTotalCntSubtLang=%d, uiPCR_PID=%d, ucAudio_StreamType=%d, ucVideo_StreamType=%d WHERE channelNumber=%d AND countryCode=%d AND serviceID=%d",
				p->uiCAECMPID, p->uiACECMPID, p->uiAudioPID, p->uiVideoPID, p->uiStPID, p->uiSiPID, p->uiPMTPID, p->uiTotalAudio_PID, p->uiTotalVideo_PID, p->usTotalCntSubtLang, p->uiPCR_PID, p->ucAudio_StreamType, p->ucVideo_StreamType, p->uiCurrentChannelNumber, p->uiCurrentCountryCode, p->uiServiceID);
		} else if(p->uiCAECMPID != 0) {
			sprintf(szSQL, "UPDATE channels SET CA_ECM_PID=%d, audioPID=%d, videoPID=%d, stPID=%d, siPID=%d, PMT_PID=%d, uiTotalAudio_PID=%d, uiTotalVideo_PID=%d, usTotalCntSubtLang=%d, uiPCR_PID=%d, ucAudio_StreamType=%d, ucVideo_StreamType=%d WHERE channelNumber=%d AND countryCode=%d AND serviceID=%d",
				p->uiCAECMPID, p->uiAudioPID, p->uiVideoPID, p->uiStPID, p->uiSiPID, p->uiPMTPID, p->uiTotalAudio_PID, p->uiTotalVideo_PID, p->usTotalCntSubtLang, p->uiPCR_PID, p->ucAudio_StreamType, p->ucVideo_StreamType, p->uiCurrentChannelNumber, p->uiCurrentCountryCode, p->uiServiceID);
		} else if(p->uiACECMPID != 0) {
			sprintf(szSQL, "UPDATE channels SET AC_ECM_PID=%d, audioPID=%d, videoPID=%d, stPID=%d, siPID=%d, PMT_PID=%d, uiTotalAudio_PID=%d, uiTotalVideo_PID=%d, usTotalCntSubtLang=%d, uiPCR_PID=%d, ucAudio_StreamType=%d, ucVideo_StreamType=%d WHERE channelNumber=%d AND countryCode=%d AND serviceID=%d",
				p->uiACECMPID, p->uiAudioPID, p->uiVideoPID, p->uiStPID, p->uiSiPID, p->uiPMTPID, p->uiTotalAudio_PID, p->uiTotalVideo_PID, p->usTotalCntSubtLang, p->uiPCR_PID, p->ucAudio_StreamType, p->ucVideo_StreamType, p->uiCurrentChannelNumber, p->uiCurrentCountryCode, p->uiServiceID);
		} else {
			sprintf(szSQL, "UPDATE channels SET audioPID=%d, videoPID=%d, stPID=%d, siPID=%d, PMT_PID=%d, uiTotalAudio_PID=%d, uiTotalVideo_PID=%d, usTotalCntSubtLang=%d, uiPCR_PID=%d, ucAudio_StreamType=%d, ucVideo_StreamType=%d WHERE channelNumber=%d AND countryCode=%d AND serviceID=%d",
				p->uiAudioPID, p->uiVideoPID, p->uiStPID, p->uiSiPID, p->uiPMTPID, p->uiTotalAudio_PID, p->uiTotalVideo_PID, p->usTotalCntSubtLang, p->uiPCR_PID, p->ucAudio_StreamType, p->ucVideo_StreamType, p->uiCurrentChannelNumber, p->uiCurrentCountryCode, p->uiServiceID);
		}
	} else {
		if(p->uiCAECMPID != 0 && p->uiACECMPID != 0) {
			sprintf(szSQL, "UPDATE channels SET CA_ECM_PID=%d, AC_ECM_PID=%d, audioPID=%d, videoPID=%d, stPID=%d, siPID=%d, uiTotalAudio_PID=%d, uiTotalVideo_PID=%d, usTotalCntSubtLang=%d, uiPCR_PID=%d, ucAudio_StreamType=%d, ucVideo_StreamType=%d WHERE channelNumber=%d AND countryCode=%d AND serviceID=%d",
				p->uiCAECMPID, p->uiACECMPID, p->uiAudioPID, p->uiVideoPID, p->uiStPID, p->uiSiPID, p->uiTotalAudio_PID, p->uiTotalVideo_PID, p->usTotalCntSubtLang, p->uiPCR_PID, p->ucAudio_StreamType, p->ucVideo_StreamType, p->uiCurrentChannelNumber, p->uiCurrentCountryCode, p->uiServiceID);
		} else if(p->uiCAECMPID != 0) {
			sprintf(szSQL, "UPDATE channels SET CA_ECM_PID=%d, audioPID=%d, videoPID=%d, stPID=%d, siPID=%d, uiTotalAudio_PID=%d, uiTotalVideo_PID=%d, usTotalCntSubtLang=%d, uiPCR_PID=%d, ucAudio_StreamType=%d, ucVideo_StreamType=%d WHERE channelNumber=%d AND countryCode=%d AND serviceID=%d",
				p->uiCAECMPID, p->uiAudioPID, p->uiVideoPID, p->uiStPID, p->uiSiPID, p->uiTotalAudio_PID, p->uiTotalVideo_PID, p->usTotalCntSubtLang, p->uiPCR_PID, p->ucAudio_StreamType, p->ucVideo_StreamType, p->uiCurrentChannelNumber, p->uiCurrentCountryCode, p->uiServiceID);
		} else if(p->uiACECMPID != 0) {
			sprintf(szSQL, "UPDATE channels SET AC_ECM_PID=%d, audioPID=%d, videoPID=%d, stPID=%d, siPID=%d, uiTotalAudio_PID=%d, uiTotalVideo_PID=%d, usTotalCntSubtLang=%d, uiPCR_PID=%d, ucAudio_StreamType=%d, ucVideo_StreamType=%d WHERE channelNumber=%d AND countryCode=%d AND serviceID=%d",
				p->uiACECMPID, p->uiAudioPID, p->uiVideoPID, p->uiStPID, p->uiSiPID, p->uiTotalAudio_PID, p->uiTotalVideo_PID, p->usTotalCntSubtLang, p->uiPCR_PID, p->ucAudio_StreamType, p->ucVideo_StreamType, p->uiCurrentChannelNumber, p->uiCurrentCountryCode, p->uiServiceID);
		} else {
			sprintf(szSQL, "UPDATE channels SET audioPID=%d, videoPID=%d, stPID=%d, siPID=%d, uiTotalAudio_PID=%d, uiTotalVideo_PID=%d, usTotalCntSubtLang=%d, uiPCR_PID=%d, ucAudio_StreamType=%d, ucVideo_StreamType=%d WHERE channelNumber=%d AND countryCode=%d AND serviceID=%d",
				p->uiAudioPID, p->uiVideoPID, p->uiStPID, p->uiSiPID, p->uiTotalAudio_PID, p->uiTotalVideo_PID, p->usTotalCntSubtLang, p->uiPCR_PID, p->ucAudio_StreamType, p->ucVideo_StreamType, p->uiCurrentChannelNumber, p->uiCurrentCountryCode, p->uiServiceID);
		}
	}

	sqlite3_prepare_v2(sqlhandle, szSQL, -1, &stmt, NULL);
	sqlite3_reset(stmt);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	return SQLITE_OK;
}

int TCCISDBT_ChannelDB_UpdateSQLfile_AutoSearch(sqlite3 *sqlhandle, int channel, int country_code, int service_id)
{
	int iTCCMsgCount, i;
	ST_CHANNEL_DATA *p;
	char szSQL[1024];
	sqlite3_stmt *stmt;

	iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();
	if(iTCCMsgCount <= 0) return SQLITE_ERROR;

	for (i = 0; i < iTCCMsgCount; i++)
	{
		p = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageGet();
		if (p == NULL || p->uiArrange==SERVICE_ARRANGE_TYPE_DISCARD) {
			/* Original Null Pointer Dereference
			ALOGE("In %s, ch discarded, netowrk_id=0x%x, service_id=0x%x", __func__, p->uiNetworkID, p->uiServiceID);
			*/
			// jini 9th : Ok
			if (p != NULL)
				ALOGE("In %s, ch discarded, netowrk_id=0x%x, service_id=0x%x", __func__, p->uiNetworkID, p->uiServiceID);
			else
				ALOGE("In %s, ST_CHANNEL_DATA is NULL", __func__);
			continue;
		}

		if((&p->uiPMTPID != NULL) && (p->uiServiceType == SERVICE_TYPE_FSEG||p->uiServiceType == SERVICE_TYPE_TEMP_VIDEO||p->uiServiceType == SERVICE_TYPE_1SEG))
		{
			if(TCC_Isdbt_IsSupportPrimaryService())
			{
				sprintf(szSQL,
					"INSERT INTO channels (channelNumber, countryCode, audioPID, videoPID, stPID, siPID, PMT_PID, CA_ECM_PID, AC_ECM_PID, remoconID, serviceType, serviceID, regionID, threedigitNumber, TStreamID, berAVG, preset, networkID, areaCode, uiTotalAudio_PID, uiTotalVideo_PID, frequency, affiliationID, channelName, TSName, DWNL_ID, LOGO_ID, strLogo_Char, usTotalCntSubtLang, uiPCR_PID, ucAudio_StreamType, ucVideo_StreamType, usOrig_Net_ID, primaryServiceFlag) VALUES (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, ?, ?, ?, ?, %d, %d, ?, %d, %d, %d, %d, %d, %d)",
					p->uiCurrentChannelNumber, p->uiCurrentCountryCode, p->uiAudioPID, p->uiVideoPID, p->uiStPID, p->uiSiPID,
					p->uiPMTPID, p->uiCAECMPID, p->uiACECMPID, p->uiRemoconID, p->uiServiceType, p->uiServiceID, p->uiRegionID, p->uiThreeDigitNumber, p->uiTStreamID, p->uiBerAvg, RECORD_PRESET_AUTOSEARCH, p->uiNetworkID, p->uiAreaCode, p->uiTotalAudio_PID, p->uiTotalVideo_PID, p->DWNL_ID, p->LOGO_ID, p->usTotalCntSubtLang, p->uiPCR_PID, p->ucAudio_StreamType, p->ucVideo_StreamType, p->usOrig_Net_ID, p->uiPrimaryServiceFlag);
			}
			else
			{
				sprintf(szSQL,
					"INSERT INTO channels (channelNumber, countryCode, audioPID, videoPID, stPID, siPID, PMT_PID, CA_ECM_PID, AC_ECM_PID, remoconID, serviceType, serviceID, regionID, threedigitNumber, TStreamID, berAVG, preset, networkID, areaCode, uiTotalAudio_PID, uiTotalVideo_PID, frequency, affiliationID, channelName, TSName, DWNL_ID, LOGO_ID, strLogo_Char, usTotalCntSubtLang, uiPCR_PID, ucAudio_StreamType, ucVideo_StreamType, usOrig_Net_ID) VALUES (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, ?, ?, ?, ?, %d, %d, ?, %d, %d, %d, %d, %d)",
					p->uiCurrentChannelNumber, p->uiCurrentCountryCode, p->uiAudioPID, p->uiVideoPID, p->uiStPID, p->uiSiPID,
					p->uiPMTPID, p->uiCAECMPID, p->uiACECMPID, p->uiRemoconID, p->uiServiceType, p->uiServiceID, p->uiRegionID, p->uiThreeDigitNumber, p->uiTStreamID, p->uiBerAvg, RECORD_PRESET_AUTOSEARCH, p->uiNetworkID, p->uiAreaCode, p->uiTotalAudio_PID, p->uiTotalVideo_PID, p->DWNL_ID, p->LOGO_ID, p->usTotalCntSubtLang, p->uiPCR_PID, p->ucAudio_StreamType, p->ucVideo_StreamType, p->usOrig_Net_ID);
			}

			sqlite3_prepare_v2(sqlhandle, szSQL, -1, &stmt, NULL);
			sqlite3_reset(stmt);
			sqlite3_bind_blob(stmt, 1, &(p->stDescTDSD), sizeof(T_DESC_TDSD), SQLITE_TRANSIENT);
			sqlite3_bind_blob(stmt, 2, &(p->stDescEBD), sizeof(T_DESC_EBD), SQLITE_TRANSIENT);
			sqlite3_bind_text16(stmt, 3, p->pusChannelName, -1, SQLITE_STATIC);
			sqlite3_bind_text16(stmt, 4, p->pusTSName,-1, SQLITE_STATIC);
			sqlite3_bind_text16(stmt, 5, p->strLogo_Char,-1, SQLITE_STATIC);
			sqlite3_step(stmt);
			sqlite3_finalize(stmt);
		}
	}

	return SQLITE_OK;
}

int TCCISDBT_ChannelDB_CheckDoneUpdateInfo(int iCurrentChannelNumber, int iCurrentCountryCode, int iServiceID)
{
	ST_CHANNEL_DATA *pChannelData;

	DBG_MSG("[%s](%d, %d, %d)\n", __func__, iCurrentChannelNumber, iCurrentCountryCode, iServiceID);

	pChannelData = ChannelDB_GetChannelData(iCurrentChannelNumber, iCurrentCountryCode, iServiceID);
	if (pChannelData)
	{
		if (pChannelData->uiPMTPID > 0 && pChannelData->uiAudioPID > 0 && pChannelData->uiVideoPID > 0 && pChannelData->uiPCR_PID > 0 &&
			pChannelData->uiStPID > 0 && pChannelData->uiSiPID > 0 && pChannelData->uiServiceType > 0)
		{
			return SQLITE_OK;
		}
	}

	return SQLITE_ERROR;
}

int TCCISDBT_ChannelDB_ArrangeSpecialService (void)
{
	ST_CHANNEL_DATA *p;
	void *hTCCMsg;
	int iTCCMsgCount, i;

	hTCCMsg = g_pChannelMsg->TccMessageHandleFirst();
	iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();
	for(i=0; i < iTCCMsgCount; i++)
	{
		if (hTCCMsg == NULL) break;
		p = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);
		if(p->uiServiceType == SERVICE_TYPE_TEMP_VIDEO)
			p->uiArrange = SERVICE_ARRANGE_TYPE_DISCARD;
		hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
	}
	return SQLITE_OK;
}

int TCCISDBT_ChannelDB_DelSpecialServiceInfo (sqlite3 *sqlhandle)
{
	char szSQL[1024];
	int err = SQLITE_ERROR;

	sprintf(szSQL, "DELETE FROM channels WHERE serviceType=%d", SERVICE_TYPE_TEMP_VIDEO);
	err = TCC_SQLITE3_EXEC (sqlhandle, szSQL, NULL, NULL, NULL);
	return err;
}

int TCCISDBT_ChannelDB_UpdateSQLFfile_SpecialService (sqlite3 *sqlhandle, int channel_number, int country_code, unsigned short service_id, int *row_id)
{
	ST_CHANNEL_DATA *p;
	char szSQL[1024];
	sqlite3_stmt *stmt;

	if (row_id==0) return SQLITE_ERROR;

	*row_id = 0;
	p = ChannelDB_GetChannelData (channel_number, country_code, service_id);
	if (p) {
		if (p->uiServiceType != 0xA1) {
			ALOGE("In %s, service (id=0x%x) at channel=%d is not special service", __func__, service_id, channel_number);
			return SQLITE_ERROR;
		}
		TCCISDBT_ChannelDB_DelSpecialServiceInfo(sqlhandle);

		if((&p->uiPMTPID != NULL))
		{
			sprintf(szSQL,
				"INSERT INTO channels (channelNumber, countryCode, audioPID, videoPID, stPID, siPID, PMT_PID, CA_ECM_PID, AC_ECM_PID, remoconID, serviceType, serviceID, regionID, threedigitNumber, TStreamID, berAVG, preset, networkID, areaCode, uiTotalAudio_PID, uiTotalVideo_PID, frequency, affiliationID, channelName, TSName, DWNL_ID, LOGO_ID, strLogo_Char, usTotalCntSubtLang, uiPCR_PID, ucAudio_StreamType, ucVideo_StreamType, usOrig_Net_ID) VALUES (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, ?, ?, ?, ?, %d, %d, ?, %d, %d, %d, %d, %d)",
				p->uiCurrentChannelNumber, p->uiCurrentCountryCode, p->uiAudioPID, p->uiVideoPID, p->uiStPID, p->uiSiPID,
				p->uiPMTPID, p->uiCAECMPID, p->uiACECMPID, p->uiRemoconID, p->uiServiceType, p->uiServiceID, p->uiRegionID, p->uiThreeDigitNumber, p->uiTStreamID, p->uiBerAvg, RECORD_PRESET_INITSEARCH, p->uiNetworkID, p->uiAreaCode, p->uiTotalAudio_PID, p->uiTotalVideo_PID, p->DWNL_ID, p->LOGO_ID, p->usTotalCntSubtLang, p->uiPCR_PID, p->ucAudio_StreamType, p->ucVideo_StreamType, p->usOrig_Net_ID);

			sqlite3_prepare_v2(sqlhandle, szSQL, -1, &stmt, NULL);
			sqlite3_reset(stmt);
			sqlite3_bind_blob(stmt, 1, &(p->stDescTDSD), sizeof(T_DESC_TDSD), SQLITE_TRANSIENT);
			sqlite3_bind_blob(stmt, 2, &(p->stDescEBD), sizeof(T_DESC_EBD), SQLITE_TRANSIENT);
			sqlite3_bind_text16(stmt, 3, p->pusChannelName, -1, SQLITE_STATIC);
			sqlite3_bind_text16(stmt, 4, p->pusTSName,-1, SQLITE_STATIC);
			sqlite3_bind_text16(stmt, 5, p->strLogo_Char,-1, SQLITE_STATIC);
			sqlite3_step(stmt);
			sqlite3_finalize(stmt);

			*row_id = sqlite3_last_insert_rowid(sqlhandle);
		}
	} else {
		ALOGE("In %s, no information about special service (id=0x%x)", __func__, service_id);
		return SQLITE_ERROR;
	}
	return SQLITE_OK;
}

int TCCISDBT_ChannelDB_CheckNetworkID (int network_id)
{
	ST_CHANNEL_DATA *p;
	void *hTCCMsg;
	int err = SQLITE_ERROR;
	int iTCCMsgCount, i;
	unsigned int tsid = (unsigned int)network_id;

	hTCCMsg = g_pChannelMsg->TccMessageHandleFirst();
	iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();
	for(i=0; i < iTCCMsgCount; i++)
	{
		if (hTCCMsg == NULL) break;
		p = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);
		if (p!=NULL && p->uiNetworkID == tsid) {
			err = SQLITE_OK;
			break;
		}
		hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
	}
	return err;
}

int TCCISDBT_ChannelDB_ArrangeDataService (void)
{
	ST_CHANNEL_DATA *p;
	void *hTCCMsg;
	int iTCCMsgCount, i;

	hTCCMsg = g_pChannelMsg->TccMessageHandleFirst();
	iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();
	for(i=0; i < iTCCMsgCount; i++)
	{
		if (hTCCMsg == NULL) break;
		p = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);
		if(p->uiServiceType == SERVICE_TYPE_1SEG) {
			if (p->uiThreeDigitNumber >= 200 && p->uiThreeDigitNumber < 600) {
				/* service_type=0xC0 AND PID of service is not 1seg */
				p->uiArrange = SERVICE_ARRANGE_TYPE_DISCARD;
			}
		}
		hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
	}
	return SQLITE_OK;
}

/*
  * get an information about logo from DB
  * [input]
  *	channel = channel number
  *	country_code = 0:japan, 1:brazil
  *	network_id = network_id of TS
  *	service_id = service_id of service
  *	*simplogo_len = length of buffer to which simple logo string will be returned
  * [output]
  *	*dwnl_id = download_id. -1 when failure
  *	*logo_id = logo_id. -1 when failure
  *	*sSimpLogo = pointer to buffer to which simple logo string will be returned
  *	*simplogo_len = length of simple logo. 0 when failure. It doesn't include terminating null.
  */
int TCCISDBT_ChannelDB_GetInfoLogoDB(sqlite3 *sqlhandle, unsigned int channel, unsigned int country_code,
													unsigned int network_id, unsigned int service_id, unsigned int *dwnl_id,
													unsigned int *logo_id, unsigned short *sSimpLogo, unsigned int *simplogo_len)
{
	int err = -1, i;
	sqlite3_stmt *stmt;
	char szSQL[1024];

	unsigned short *pText;
	unsigned int str_size;

	unsigned int backup_simplogo_len = *simplogo_len;

	sprintf(szSQL, "SELECT DWNL_ID, LOGO_ID, strLogo_Char FROM channels WHERE channelNumber=%d AND countryCode=%d AND networkID=%d AND serviceID=%d",
			channel, country_code, network_id, service_id);
	if (sqlite3_prepare_v2 (sqlhandle, szSQL, -1, &stmt, NULL) == SQLITE_OK) {
		err = sqlite3_step(stmt);
	}

	*dwnl_id = -1;
	*logo_id = -1;
	*simplogo_len = 0;

	if (err == SQLITE_OK)
	{
		*dwnl_id = sqlite3_column_int (stmt, 0);
		*logo_id = sqlite3_column_int (stmt, 1);

		pText = (unsigned short *)sqlite3_column_text16 (stmt, 2);
		if (pText)
		{
			str_size = (unsigned int)sqlite3_column_bytes16(stmt, 2);

			if (str_size > backup_simplogo_len)
				str_size = backup_simplogo_len;

			if (str_size)
			{
				memcpy(sSimpLogo, pText, str_size);
				*simplogo_len = str_size;
			}
		}

		err = SQLITE_OK;
	}

	if (stmt)
		sqlite3_finalize(stmt);

	return err;
}
int TCCISDBT_ChannelDB_UpdateInfoLogoDB(sqlite3 *sqlhandle, unsigned int channel, unsigned int country_code, unsigned int network_id, unsigned int dwnl_id, unsigned int logo_id, unsigned short *sSimpleLogo, unsigned int simplelogo_len)
{
	ST_CHANNEL_DATA *p;
	void *hTCCMsg;
	int iTCCMsgCount, i;
	char szSQL[1024];
	sqlite3_stmt *stmt;
	int err = -1;

	hTCCMsg = g_pChannelMsg->TccMessageHandleFirst();
	iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();
	for(i=0; i < iTCCMsgCount; i++) {
		if (hTCCMsg == NULL) break;
		p = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);

		if ((p->uiCurrentChannelNumber == channel) && (p->uiCurrentCountryCode == country_code) && (p->uiNetworkID == network_id)) {
			if (p->uiServiceType == SERVICE_TYPE_FSEG || p->uiServiceType == SERVICE_TYPE_TEMP_VIDEO) {
				if ((p->DWNL_ID !=-1) && (p->LOGO_ID !=-1) && ((p->DWNL_ID != dwnl_id) || (p->LOGO_ID != logo_id))) {
					sprintf(szSQL, "UPDATE channels SET DWNL_ID=%d, LOGO_ID=%d WHERE networkID=%d AND channelNumber=%d AND countryCode=%d AND serviceID=%d",
							p->DWNL_ID, p->LOGO_ID, p->uiNetworkID, p->uiCurrentChannelNumber, p->uiCurrentCountryCode, p->uiServiceID);
					sqlite3_prepare_v2(sqlhandle, szSQL, -1, &stmt, NULL);
					sqlite3_reset(stmt);
					sqlite3_step(stmt);
					sqlite3_finalize(stmt);
					err = SQLITE_OK;
				}
			} else if (p->uiServiceType == SERVICE_TYPE_1SEG) {
				if ((p->strLogo_Char != NULL) && (strcmp((char*)(p->strLogo_Char), (char*)sSimpleLogo ))) {
					sprintf(szSQL, "UPDATE channels SET DWNL_ID=0, LOGO_ID=0, strLogo_Char=? WHERE networkID=%d AND channelNumber=%d AND countryCode=%d AND serviceID=%d",
							p->uiNetworkID, p->uiCurrentChannelNumber, p->uiCurrentCountryCode, p->uiServiceID);
					sqlite3_prepare_v2(sqlhandle, szSQL, -1, &stmt, NULL);
					sqlite3_reset(stmt);
					sqlite3_bind_text16(stmt, 1, p->strLogo_Char, -1, SQLITE_STATIC);
					sqlite3_step(stmt);
					sqlite3_finalize(stmt);
					err = SQLITE_OK;
				}
			}
		}
		hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
	}
	return err;
}

int TCCISDBT_ChannelDB_GetInfoChannelNameDB(sqlite3 *sqlhandle, unsigned int channel, unsigned int country_code,
															unsigned int network_id, unsigned int service_id,
															unsigned short *sServiceName, unsigned int *service_name_len)
{
	int err = -1, i;
	sqlite3_stmt *stmt;
	char szSQL[1024];

	unsigned short *pText;
	unsigned int str_size;
	unsigned int backup_service_name_len = *service_name_len;

	*service_name_len = 0;

	sprintf(szSQL, "SELECT channelName FROM channels WHERE channelNumber=%d AND countryCode=%d AND networkID=%d AND serviceID=%d",
			channel, country_code, network_id, service_id);
	err = sqlite3_prepare_v2(sqlhandle, szSQL, -1, &stmt, NULL);
	if (err == SQLITE_OK)
	{
		err = sqlite3_step(stmt);
		if (err == SQLITE_ROW)
		{
			pText = (unsigned short *)sqlite3_column_text16 (stmt, 0);
			if (pText)
			{
				str_size = (unsigned int)sqlite3_column_bytes16 (stmt, 0);

				if (str_size > backup_service_name_len)
				{
					str_size = backup_service_name_len;
				}

				if (str_size)
				{
					memcpy (sServiceName, pText, str_size);
					*service_name_len = str_size;
				}
			}
			err = SQLITE_OK;
		}
	}

	if (stmt)
		sqlite3_finalize (stmt);

	return err;
}

int TCCISDBT_ChannelDB_UpdateChannelNameDB (sqlite3 *sqlhandle, unsigned int channel, unsigned int country_code, unsigned int network_id)
{
	ST_CHANNEL_DATA *p;
	void *hTCCMsg;
	int iTCCMsgCount, i;
	char szSQL[1024];
	sqlite3_stmt *stmt;
	int err = SQLITE_ERROR;

	hTCCMsg = g_pChannelMsg->TccMessageHandleFirst();
	iTCCMsgCount = g_pChannelMsg->TccMessageGetCount();
	for(i=0; i < iTCCMsgCount; i++) {
		if (hTCCMsg == NULL) break;
		p = (ST_CHANNEL_DATA *)g_pChannelMsg->TccMessageHandleMsg(hTCCMsg);
		if ((p->uiCurrentChannelNumber == channel) && (p->uiCurrentCountryCode == country_code) && (p->uiNetworkID == network_id)) {
			sprintf(szSQL, "UPDATE channels SET channelName=? WHERE networkID=%d AND channelNumber=%d AND countryCode=%d AND serviceID=%d",
					p->uiNetworkID, p->uiCurrentChannelNumber, p->uiCurrentCountryCode, p->uiServiceID);
			sqlite3_prepare_v2(sqlhandle, szSQL, -1, &stmt, NULL);
			sqlite3_reset(stmt);
			sqlite3_bind_text16(stmt, 1, p->pusChannelName, -1, SQLITE_STATIC);
			sqlite3_step(stmt);
			sqlite3_finalize(stmt);
			err = SQLITE_OK;
		}

		hTCCMsg = g_pChannelMsg->TccMessageHandleNext(hTCCMsg);
	}
	return err;
}

/* Noah / 20170522 / Added for IM478A-31 (Primary Service)
	This function is based on 'TCCISDBT_ChannelDB_UpdateNetworkID'.
*/
int TCCISDBT_ChannelDB_UpdatePrimaryServiceFlag(int iCurrentChannelNumber, int iCurrentCountryCode, int iServiceID, int iPrimaryServiceFlagValue)
{
	ST_CHANNEL_DATA *pChannelData;

	DBG_MSG("[%s] (%d, %d, 0x%04X, %d)\n", __func__, iCurrentChannelNumber, iCurrentCountryCode, iServiceID, iPrimaryServiceFlagValue);

	pChannelData = ChannelDB_GetChannelData(iCurrentChannelNumber, iCurrentCountryCode, iServiceID);
	if(pChannelData)
	{
		pChannelData->uiPrimaryServiceFlag = iPrimaryServiceFlagValue;

		return SQLITE_OK;
	}

	ERR_MSG("[%s] Error update iPrimaryServiceFlagValue\n", __func__);
	return SQLITE_OK;
}

/* Noah / 20180131 / IM478A-77 (relay station search in the background using 2TS)
	Description
		This fucntion is based on 'TCCISDBT_ChannelDB_Insert' and is for 'setRelayStationChDataIntoDB' API.
		If J&K App finds "Relay Station Channel" by using 2TS,
			1. J&K App calls 'setRelayStationChDataIntoDB' API to save some data of NIT to DB
			2. 'setRelayStationChDataIntoDB' API calls the following 2 functions.
				2.1. InsertDB_ChannelTable_2
					In order to save the data to Queue (g_pChannelMsg) first
				2.2. UpdateDB_CustomSearch_2
					2.2.1. In order to fit J&K's specification (refer to Requirement_Specification_for_RelayStaion_Search_20140620_V1.06.pdf)
					2.2.2. In order to save the data to real DB.
			3. After 'setRelayStationChDataIntoDB' API is done, J&K App calls 'setChannel' to play.

	Parameter
		All of them : Input

	Return
		0 (SQLITE_OK) : success
		else          : error
*/
int TCCISDBT_ChannelDB_Insert_2(int channelNumber,
											int countryCode,
											int PMT_PID,
											int remoconID,
											int serviceType,
											int serviceID,
											int regionID,
											int threedigitNumber,
											int TStreamID,
											int berAVG,
											char *channelName,
											char *TSName,
											int networkID,
											int areaCode,
											unsigned char *frequency)
{
	int length = 0;
	ST_CHANNEL_DATA *pChannelData = NULL;

	pChannelData = (ST_CHANNEL_DATA*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_CHANNEL_DATA));
	memset(pChannelData, 0x0, sizeof(ST_CHANNEL_DATA));

	pChannelData->uiArrange = SERVICE_ARRANGE_TYPE_NOTHING;
	pChannelData->DWNL_ID = -1;
	pChannelData->LOGO_ID = -1;
	pChannelData->strLogo_Char = (unsigned short *)NULL;
	pChannelData->uiCurrentChannelNumber = channelNumber;
	pChannelData->uiCurrentCountryCode = countryCode;
	pChannelData->uiPMTPID = PMT_PID;
	pChannelData->uiRemoconID = remoconID;
	pChannelData->uiServiceType = serviceType;
	pChannelData->uiServiceID = serviceID;
	pChannelData->uiRegionID = regionID;
	pChannelData->uiThreeDigitNumber = threedigitNumber;
	pChannelData->uiTStreamID = TStreamID;
	pChannelData->uiBerAvg = berAVG;

	if(channelName)
	{
		length = 0;
		while(1)
		{
			if(channelName[length] == 0x00 && channelName[length + 1] == 0x00)
			{
				break;
			}

			length += 2;  // TSName is unicode, so I use +2.

			if(MAX_CHANNEL_NAME < length)
			{
				ALOGE("[%s] channelName length is over MAX_CHANNEL_NAME, Error !!!!!", __func__);
				return SQLITE_ERROR;
			}
		}

		if(length)
		{
			pChannelData->pusChannelName = (unsigned short*)tcc_mw_malloc(__FUNCTION__, __LINE__, length + 2);  // End code is two bytes 0x0000 due to sqlite3_bind_text16.
			if(pChannelData->pusChannelName)
			{
				memset(pChannelData->pusChannelName, 0, length + 2);
				memcpy(pChannelData->pusChannelName, channelName, length);
			}
		}
	}

	if(TSName)
	{
		length = 0;
		while(1)
		{
			if(TSName[length] == 0x00 && TSName[length + 1] == 0x00)
			{
				break;
			}

			length += 2;  // TSName is unicode, so I use +2.

			if(MAX_CHANNEL_NAME < length)
			{
				ALOGE("[%s] TSName length is over MAX_CHANNEL_NAME, Error !!!!!", __func__);
				return SQLITE_ERROR;
			}
		}

		if(length)
		{
			pChannelData->pusTSName = (unsigned short*)tcc_mw_malloc(__FUNCTION__, __LINE__, length + 2);  // End code is two bytes 0x0000 due to sqlite3_bind_text16.
			if(pChannelData->pusTSName)
			{
				memset(pChannelData->pusTSName, 0, length + 2);
				memcpy(pChannelData->pusTSName, TSName, length);
			}
		}
	}

	pChannelData->uiNetworkID = networkID;
	pChannelData->uiAreaCode = areaCode;

	if(frequency)
		memcpy(&pChannelData->stDescTDSD, frequency, sizeof(T_DESC_TDSD));

	DBG_MSG("[%s] (%d, %d, 0x%04X)\n", __func__, pChannelData->uiCurrentChannelNumber, pChannelData->uiCurrentCountryCode, pChannelData->uiServiceID);

	if(g_pChannelMsg->TccMessagePut((void *)pChannelData))
	{
		if(pChannelData->pusChannelName)
			tcc_mw_free(__FUNCTION__, __LINE__, pChannelData->pusChannelName);
		if(pChannelData->pusTSName)
			tcc_mw_free(__FUNCTION__, __LINE__, pChannelData->pusTSName);
		if(pChannelData->strLogo_Char)
			tcc_mw_free(__FUNCTION__, __LINE__, pChannelData->strLogo_Char);

		tcc_mw_free(__FUNCTION__, __LINE__, pChannelData);
	}

	return SQLITE_OK;
}

