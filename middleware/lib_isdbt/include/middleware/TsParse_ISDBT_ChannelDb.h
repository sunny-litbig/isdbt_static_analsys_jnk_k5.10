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
#ifndef _TCC_CHANNEL_DB_H_
#define	_TCC_CHANNEL_DB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ISDBT_Common.h"
#include "sqlite3.h"

/******************************************************************************
* defines 
******************************************************************************/
#define MAX_CHANNEL_NAME	 256


/******************************************************************************
* typedefs & structure
******************************************************************************/
typedef struct
{
	unsigned int uiCurrentChannelNumber;
	unsigned int uiCurrentCountryCode;
	unsigned int uiAudioPID;
	unsigned int uiVideoPID;
	unsigned int uiStPID;
	unsigned int uiSiPID;
	unsigned int uiPMTPID;
	unsigned int uiCAECMPID;
	unsigned int uiACECMPID;
	unsigned int uiRemoconID;
	unsigned int uiServiceType;
	unsigned int uiServiceID;
	unsigned int uiRegionID;
	unsigned int uiThreeDigitNumber;
	unsigned int uiTStreamID;
	int uiBerAvg;				// save RSSI. The weaker is a signal, the lower
	unsigned short* pusChannelName;
	unsigned int uiNetworkID;
	unsigned int uiAreaCode;
	T_DESC_EBD stDescEBD;
	T_DESC_TDSD stDescTDSD;
	unsigned int uiArrange;		// 1 - discard this info
	unsigned short *pusTSName;
	unsigned int uiTotalAudio_PID;
	unsigned int uiTotalVideo_PID;
	unsigned int 	usTotalCntSubtLang;
	unsigned int 	DWNL_ID;
	unsigned int 	LOGO_ID;
	unsigned short*	strLogo_Char;
	unsigned int 	uiPCR_PID;
	unsigned int 	ucAudio_StreamType;
	unsigned int 	ucVideo_StreamType;
	unsigned int usOrig_Net_ID;
	// Noah / 20170522 / Added for IM478A-31 (Primary Service)
	unsigned int uiPrimaryServiceFlag;
}ST_CHANNEL_DATA;

/******************************************************************************
* globals
******************************************************************************/

/******************************************************************************
* locals
******************************************************************************/

/******************************************************************************
* declarations
******************************************************************************/
void TCCISDBT_ChannelDB_Init(void);
void TCCISDBT_ChannelDB_Deinit(void);

int TCCISDBT_ChannelDB_Clear(void);
int TCCISDBT_ChannelDB_Insert(P_ISDBT_SERVICE_STRUCT pServiceList, int fUpdateType);
int TCCISDBT_ChannelDB_UpdateStreamID(int iCurrentChannelNumber, int iCurrentCountryCode, int iServiceID, int iStreamID);
int TCCISDBT_ChannelDB_Update(int iCurrentChannelNumber, int iCurrentCountryCode, int iServiceID,  P_ISDBT_SERVICE_STRUCT pServiceList);
int TCCISDBT_ChannelDB_UpdatePMTPID(int iCurrentChannelNumber, int iCurrentCountryCode, int iServiceID, int iPMTPID);
int TCCISDBT_ChannelDB_UpdateRemoconID(int iCurrentChannelNumber, int iCurrentCountryCode, int iServiceID, int iRemoconID, int iRegionID, int iThreeDigitNumber);
int TCCISDBT_ChannelDB_UpdateChannelName(int iCurrentChannelNumber, int iCurrentCountryCode, int iServiceID, unsigned short *pusChannelName);
int TCCISDBT_ChannelDB_UpdateTSName(int iCurrentChannelNumber, int iCurrentCountryCode, int iServiceID, unsigned short *pusTSName);
int TCCISDBT_ChannelDB_UpdateNetworkID(int iCurrentChannelNumber, int iCurrentCountryCode, int iServiceID, int iNetworkID);
int TCCISDBT_ChannelDB_UpdateAreaCode(int iCurrentChannelNumber, int iCurrentCountryCode, int iAreaCode);
int TCCISDBT_ChannelDB_UpdateFrequency(int iCurrentChannelNumber, int iCurrentCountryCode, void *pDescTDSD);
int TCCISDBT_ChannelDB_UpdateAffiliationID(int iCurrentChannelNumber, int iCurrentCountryCode, void *pDescEBD);
int TCCISDBT_ChannelDB_UpdateSQLfile(sqlite3 *sqlhandle, int channel, int country_code, int service_id, int del_option);
int TCCISDBT_ChannelDB_UpdateSQLfile_Partially(sqlite3 *sqlhandle, unsigned int channel, unsigned int country_code, unsigned int service_id);
int TCCISDBT_ChannelDB_UpdateSQLfile_AutoSearch(sqlite3 *sqlhandle, int channel, int country_code, int service_id);
int TCCISDBT_ChannelDB_CheckDoneUpdateInfo(int iCurrentChannelNumber, int iCurrentCountryCode, int iServiceID);
int TCCISDBT_ChannelDB_UpdateStrength (int strength_index, int service_id);
int TCCISDBT_ChannelDB_GetPMTCount(int iCurrentChannelNumber);
int TCCISDBT_ChannelDB_GetPMTPID(int iCurrentChannelNumber, unsigned short* pPMT_PID, int iPMT_PID_Size);
int TCCISDBT_ChannelDB_GetServiceID(int iCurrentChannelNumber, unsigned short* pSERVICE_ID, int iSERVICE_ID_Size);
int TCCISDBT_ChannelDB_ArrangeTable (sqlite3 *sqlhandle);
int TCCISDBT_ChannelDB_GetChannelInfo (SERVICE_FOUND_INFO_T *p);
int TCCISDBT_ChannelDB_GetChannelSvcCnt (void);
int TCCISDBT_ChannelDB_DelAutoSearch (sqlite3 *sqlhandle);
int TCCISDBT_ChannelDB_SetAutosearchInfo (sqlite3 *sqlhandle, SERVICE_FOUND_INFO_T *p);
int TCCISDBT_ChannelDB_ArrangeAutoSearch (sqlite3 *sqlhandle, SERVICE_FOUND_INFO_T *p);
int TCCISDBT_ChannelDB_SetArrangementType(int iType);
int TCCISDBT_ChannelDB_Is1SegService(unsigned int channel, unsigned int country_code, unsigned int service_id);
int TCCISDBT_ChannelDB_Get1SegServiceCount(unsigned int channel, unsigned int country_code);
int TCCISDBT_ChannelDB_GetDataServiceCount(unsigned int channel, unsigned int country_code);
int TCCISDBT_ChannelDB_GetSpecialServiceCount(unsigned int channel, unsigned int country_code);
int TCCISDBT_ChannelDB_Get1SegServiceID(unsigned int channel, unsigned int country_code, unsigned int *service_id);
int TCCISDBT_ChannelDB_GetDataServiceID(unsigned int channel, unsigned int country_code, unsigned int *service_id);
unsigned int TCCISDBT_ChannelDB_GetNetworkID (int channel, int country_code, int service_id);
int TCCISDBT_ChannelDB_CustomSearch_DelInfo(sqlite3 *sqlhandle, int preset, int network_id);
void TCCISDBT_ChannelDB_CustomSearch_UpdateInfo (sqlite3 *sqlhandle, int preset, CUSTOMSEARCH_FOUND_INFO_T *pFoundInfo);
int TCCISDBT_ChannelDB_CustomSearch_SearchSameService(sqlite3 *sqlhandle, int channel, int preset, int network_id, int *pfullseg_id, int *poneseg_id, int foverwrite);
int TCCISDBT_ChannelDB_CustomSearch_GetChannelInfoReplace(SERVICE_FOUND_INFO_T *pSvc);
int TCCISDBT_ChannelDB_CustomSearch_ChannelInfoReplace(sqlite3 *sqlhandle, SERVICE_FOUND_INFO_T *pSvcInfo, int preset, int tsid);
int TCCISDBT_ChannelDB_UpdateLogoInfo(int iServiceID, int iCurrentChannelNumber, int iCurrentCountryCode, P_ISDBT_SERVICE_STRUCT pServiceList, unsigned short *pusSimpleLogo);
int TCCISDBT_ChannelDB_UpdateOriginalNetworkID(int ch_no, int country_code, unsigned int service_id, unsigned int original_network_id);
int TCCISDBT_ChannelDB_DelSpecialServiceInfo (sqlite3 *sqlhandle);
int TCCISDBT_ChannelDB_UpdateSQLFfile_SpecialService (sqlite3 *sqlhandle, int channel_number, int country_code, unsigned short service_id, int *row_id);
int TCCISDBT_ChannelDB_ArrangeSpecialService (void);
int TCCISDBT_ChannelDB_CheckNetworkID (int network_id);
int TCCISDBT_ChannelDB_ArrangeDataService (void);
int TCCISDBT_ChannelDB_GetInfoLogoDB(sqlite3 *sqlhandle, unsigned int channel, unsigned int country_code, unsigned int network_id, unsigned int service_id, unsigned int *dwnl_id, unsigned int *logo_id, unsigned short *sSimpLogo, unsigned int *simplogo_len);
int TCCISDBT_ChannelDB_UpdateInfoLogoDB(sqlite3 *sqlhandle, unsigned int channel, unsigned int country_code, unsigned int network_id, unsigned int dwnl_id, unsigned int logo_id, unsigned short *sSimpleLogo, unsigned int simplelogo_len);
int TCCISDBT_ChannelDB_GetInfoChannelNameDB(sqlite3 *sqlhandle, unsigned int channel, unsigned int country_code, unsigned int network_id, unsigned int service_id, unsigned short *sServiceName, unsigned int *service_name_len);
int TCCISDBT_ChannelDB_UpdateChannelNameDB (sqlite3 *sqlhandle, unsigned int channel, unsigned int country_code, unsigned int network_id);
// Noah / 20170522 / Added for IM478A-31 (Primary Service)
int TCCISDBT_ChannelDB_UpdatePrimaryServiceFlag(int iCurrentChannelNumber, int iCurrentCountryCode, int iServiceID, int iPrimaryServiceFlagValue);
// Noah / 20180131 / IM478A-77 (relay station search in the background using 2TS)
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
								unsigned char *frequency);
					

#ifdef __cplusplus
}
#endif

#endif

