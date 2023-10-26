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
#ifndef _ISDBT_DB_LAYER_H_
#define _ISDBT_DB_LAYER_H_

#include "TsParse_ISDBT_DBInterface.h"
#include "sqlite3.h"

#define	SERVICE_ARRANGE_TYPE_NOTHING	0
#define	SERVICE_ARRANGE_TYPE_DISCARD	1
#define	SERVICE_ARRANGE_TYPE_REPLACE	2
#define	SERVICE_ARRANGE_TYPE_RESCAN		4
#define	SERVICE_ARRANGE_TYPE_HANDOVER	5

#define	SERVICE_ARRANGE_MAX_COUNT	64
struct _tag_arrange_service_info_ {
	unsigned int uiCurrentChannelNumber;
	unsigned int uiCurrentCountryCode;
	unsigned int uiNetworkID;
	unsigned int uiServiceID;
	unsigned int uiArrange;
};
typedef struct {
	int	limit_count;
	int arrange_count;
	struct _tag_arrange_service_info_ service_info[SERVICE_ARRANGE_MAX_COUNT];
} SERVICE_ARRANGE_INFO_T;

struct _tag_service_found_info {
	unsigned int rowID;
	unsigned int uiCurrentChannelNumber;
	unsigned int uiCurrentCountryCode;
	unsigned int uiNetworkID;
	unsigned int uiServiceID;
	unsigned int uiServiceType;
	unsigned int uiOption;
};
typedef struct {
	int	found_no;
	struct _tag_service_found_info *pServiceFound;
} SERVICE_FOUND_INFO_T;

/* order of each member should be match with ISDBTPlayer::CustomSearchInfoData */
typedef struct _tag_customsearch_found_info {
	int status;			/* -1=failure, 1=success, 2=cancel, 3=on-going */
	int ch;
	int strength;
	int tsid;
	int fullseg_id;		/* _id filed of channel db */
	int oneseg_id;
	int total_svc;		/* count of total service */
	int all_id[32];
} CUSTOMSEARCH_FOUND_INFO_T;

#define	RECORD_PRESET_INITSEARCH	0
#define	RECORD_PRESET_REGIONCHANNEL	1
#define	RECORD_PRESET_AUTOSEARCH	2
#define	RECORD_PRESET_CUSTOM_RELAY	3
#define	RECORD_PRESET_CUSTOM_AFFILIATION	4

#include "TsParse_ISDBT_ChannelDb.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
* defines 
******************************************************************************/

/******************************************************************************
* typedefs & structure
******************************************************************************/
typedef enum
{
	EPG_PF,
	EPG_SCHEDULE,
	EPG_ERR
}E_EPG_TYPE;

#define MAX_SUPPORT 32
typedef struct 
{
	unsigned short ServiceID;
	unsigned int NumofLang;
	unsigned int SubtLangCode[2];
}SUBTITLE_LANGUAGE_INFO[MAX_SUPPORT];

/******************************************************************************
* declarations
******************************************************************************/
extern void Set_User_Area_Code(int area);
extern int Get_User_Area_Code(void);
extern void Set_NIT_Area_Code(int area);
extern int Get_NIT_Area_Code(void);
extern void Set_Language_Code(int lang);
extern int Get_Language_Code(void);

extern int TCC_SQLITE3_EXEC(sqlite3* h,  const char *sql,  int (*callback)(void*,int,char**,char**), void *arg, char **errmsg);

extern int	ChannelDB_BackUp(void);
extern int	ChannelDB_Recover(void);

extern int UpdateDB_AreaCode(int AreaCode);
extern int UpdateDB_TDTTime(DATE_TIME_STRUCT *pstDateTime);
extern int UpdateDB_TOTTime(DATE_TIME_STRUCT *pstDateTime, LOCAL_TIME_OFFSET_STRUCT *pstLocalTimeOffset);

extern sqlite3 *GetDB_Handle(void);
extern int OpenDB_Table(void);
extern int CloseDB_Table(void);
extern int DeleteDB_Table(void);

extern int GetCountDB_PMT_PID_ChannelTable(void);
extern BOOL GetPMT_PID_ChannelTable(unsigned short* pPMT_PID, int iPMT_PID);
extern BOOL GetServiceID_ChannelTable(unsigned short *pSERVICE_ID, int iSERVICE_ID_SIZE);
extern BOOL InsertDB_ChannelTable(P_ISDBT_SERVICE_STRUCT pServiceList, int fUpdateType);
extern BOOL UpdateDB_ChannelTable(int uiServiceID, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_SERVICE_STRUCT pServiceList);
extern int UpdateDB_ChannelTablePreset (int area_code);
extern BOOL UpdateDB_PMTPID_ChannelTable(int uiServiceID, int iPMT);
extern BOOL UpdateDB_StreamID_ChannelTable(int iServiceID, int iStreamID);
extern BOOL UpdateDB_RemoconID_ChannelTable(int uiServiceID, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_SERVICE_STRUCT pServiceList);
extern BOOL UpdateDB_ChannelName_ChannelTable(int uiServiceID, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_SERVICE_STRUCT pServiceList);
extern BOOL UpdateDB_TSName_ChannelTable (int uiServiceID, int uiCurrentChannelNumber,int uiCurrentCountryCode, P_ISDBT_SERVICE_STRUCT pSvcList);
extern BOOL UpdateDB_NetworkID_ChannelTable(int uiServiceID, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_SERVICE_STRUCT pServiceList);
extern BOOL UpdateDB_Frequency_ChannelTable(int uiCurrentChannelNumber,	int uiCurrentCountryCode, P_ISDBT_SERVICE_STRUCT pServiceList);
extern BOOL UpdateDB_AffiliationID_ChannelTable(int uiCurrentChannelNumber,	int uiCurrentCountryCode, P_ISDBT_SERVICE_STRUCT pServiceList);
extern BOOL UpdateDB_Channel_LogoInfo(int uiServiceID, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_SERVICE_STRUCT pServiceList);
extern int UpdateDB_OriginalNetworID_ChannelTable (int uiServiceID, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_SERVICE_STRUCT pServiceList);

extern BOOL InsertDB_VideoPIDTable(int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_SERVICE_STRUCT pServiceList);
extern BOOL InsertDB_AudioPIDTable(int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_SERVICE_STRUCT pServiceList);
extern BOOL InsertDB_SubTitlePIDTable(int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_SERVICE_STRUCT pServiceList);
extern BOOL UpdateDB_Logo_Info (T_ISDBT_LOGO_INFO *pstLogoInfo);
extern int UpdateDB_AreaCode_ChannelTable(int uiCurrentChannelNumber, int uiCurrentCountryCode,	int AreaCode);
extern int UpdateDB_Channel_SQL(int channel, int country_code, int service_id, int del_option);
extern int UpdateDB_Channel_SQL_Partially(unsigned int channel, unsigned int country_code, unsigned int service_id);
extern int UpdateDB_Channel_SQL_AutoSearch(int channel, int country_code, int service_id);
extern int UpdateDB_CheckDoneUpdateInfo(int iCurrentChannelNumber, int iCurrentCountryCode, int iServiceID);
extern int UpdateDB_Channel_Is1SegService(unsigned int channel, unsigned int country_code, unsigned int service_id);
extern int UpdateDB_Channel_Get1SegServiceCount(unsigned int channel, unsigned int country_code);
extern int UpdateDB_Channel_Get1SegServiceID (unsigned int channel, unsigned int country_code, unsigned int *service_id);
extern int UpdateDB_Channel_GetDataServiceID (unsigned int channel, unsigned int country_code, unsigned int *service_id);
extern int UpdateDB_Channel_GetDataServiceCount(unsigned int channel, unsigned int country_code);
extern int UpdateDB_Channel_GetSpecialServiceCount(unsigned int channel, unsigned int country_code);

extern BOOL InsertDB_TeleText_HeaderTable(unsigned char ucMagazine_Number, unsigned char ucPage_Number, unsigned char ucField_Parity, unsigned char ucLine_Offset, unsigned char* pSubCode, unsigned short usControl_Bit, unsigned char *pData_block);
extern int GetCountDB_TeleText_HeaderTable(unsigned char ucMagazine_Number, unsigned char ucPage_Number);

extern sqlite3 *GetDB_EPG_Handle(void);
extern int OpenDB_EPG_Table(void);
extern int CloseDB_EPG_Table(void);
extern int DeleteDB_EPG_PF_Table(int iChannelNumber, int iServiceID, int iTableID);
extern int DeleteDB_EPG_Schedule_Table(int iChannelNumber, int iServiceID, int iTableID, int iSectionNumber, int iVersionNumber);

extern int GetCountDB_EPG_Table(int iTableType, int uiCurrentChannelNumber, int uiCurrentCountryCode, int uiEventID, P_ISDBT_EIT_STRUCT pstEIT);
extern BOOL InsertDB_EPG_Table(int iTableType, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_EIT_STRUCT pstEIT, P_ISDBT_EIT_EVENT_DATA pstEITEvent);
extern BOOL UpdateDB_EPG_XXXTable(int iTableType, int uiEvtNameLen, char *pEvtName, int uiEvtTextLen, char *pEvtText, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_EIT_STRUCT pstEIT, P_ISDBT_EIT_EVENT_DATA pstEITEvent);
extern BOOL UpdateDB_EPG_XXXTable_ExtEvent(int iTableType, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_EIT_STRUCT pstEIT, P_ISDBT_EIT_EVENT_DATA pstEITEvent, EXT_EVT_DESCR *pstEED);
extern BOOL UpdateDB_EPG_XXXTable_EvtGroup(int iTableType, int uiRefServiceID, int uiRefEventID, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_EIT_STRUCT pstEIT, P_ISDBT_EIT_EVENT_DATA pstEITEvent);

extern BOOL UpdateDB_EPG_Content(int iTableType, unsigned int uiContentGenre, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_EIT_STRUCT pstEIT, P_ISDBT_EIT_EVENT_DATA pstEITEvent);
extern BOOL UpdateDB_EPG_Rating(int iTableType, unsigned int uiRating, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_EIT_STRUCT pstEIT, P_ISDBT_EIT_EVENT_DATA pstEITEvent);
extern BOOL UpdateDB_EPG_VideoInfo (int iTableType, unsigned int component_tag, unsigned int component_type, int uiCurrentChannelNumber,  int uiCurrentCountryCode,  P_ISDBT_EIT_STRUCT pstEIT, P_ISDBT_EIT_EVENT_DATA pstEITEvent);
extern BOOL UpdateDB_EPG_AudioInfo (int iTableType, unsigned int component_tag, int stream_type, int component_type, int fMultiLingual, int sampling_rate, unsigned char *psLangCode1, unsigned char *psLangCode2, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_EIT_STRUCT pstEIT, P_ISDBT_EIT_EVENT_DATA pstEITEvent);
extern void UpdateDB_EPG_Commit(int iTotalEventCount, int *piEventIDList, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_EIT_STRUCT pstEIT);
extern int GetDBVersion_TableType(DBTABLE_TYPE sType, int *piParam, int iServiceID, int iChannelNumber, int iCountryCode);
extern int UpdateDB_Strength_ChannelTable(int strength_index,int service_id);
extern int UpdateDB_Arrange_ChannelTable(void);
extern int UpdateDB_Get_ChannelInfo (SERVICE_FOUND_INFO_T *p);
extern int UpdateDB_Get_ChannelSvcCnt (void);
extern int UpdateDB_Del_AutoSearch (void);
extern int UpdateDB_Set_AutosearchInfo (SERVICE_FOUND_INFO_T *P);
extern int UpdateDB_ArrangeAutoSearch (SERVICE_FOUND_INFO_T *p);
extern int UpdateDB_Set_ArrangementType(int iType);
extern int ChangeDB_Handle (int index, int *prev_handle);
extern int UpdateDB_ChannelDB_Clear(void);
extern int ChangeDB_EPG_Handle (int index, int *prev_handle);

extern int tcc_db_get_current_time(int *piMJD, int *piHour, int *piMin, int *piSec, int *piPolarity, int *piHourOffset, int *piMinOffset);
extern int tcc_db_get_channel_count(void);
extern int tcc_db_get_channel_data(int *pData, int max_count);
extern int tcc_db_get_audio(int uiServiceID, int uiCurrentChannelNumber, int *piAudioMaxCount, int *piAudioPID, int *piAudioStreamType, int *piComponentTag);
extern int tcc_db_get_audio_byTag(int uiServiceID, int uiCurrentChannelNumber, int iComponentTag, int *piAudioPID);
extern int tcc_db_get_video(int uiServiceID, int uiCurrentChannelNumber, int* piVideoMaxCount, int *piVideoPID, int *piVideoStreamType, int *piComponentTag);
extern int tcc_db_get_video_byTag(int uiServiceID, int uiCurrentChannelNumber, int iComponentTag, int *piVideoPID);

extern int tcc_db_get_preset(int iRowID, int *piChannelNumber, int *piPreset);
extern int tcc_db_get_channel_byChannelNumber(int iChannelNumber, int *piCountryCode, int *piNetworkID);
extern int tcc_db_get_channel(int iRowID, int *piChannelNumber, int piAudioPID[], int piVideoPID[], int *piStPID, int *piSiPID, int *piServiceType, int *piServiceID, int *piPMTPID, int *piCAECMPID, int *piACECMPID, int *piNetworkID, int piAudioStreamType[], int piVideoStreamType[], int piAudioComponentTag[], int piVideoComponentTag[], int *piPCRPID, int *piTotalAudioCount, int *piTotalVideoCount, int *piAudioIndex, int *piAudioRealIndex, int *piVideoIndex, int *piVideoRealIndex);
extern int tcc_db_get_channel_info(int iChannelNumber, int iServiceID, 	int *piRemoconID, int *piThreeDigitNumber, int *piServiceType, unsigned short *pusChannelName, int *piChannelNameLen, int *piTotalAudioPID, int *piTotalVideoPID, int *piTotalSubtitleLang, int *piAudioMode, int *piVideoMode, int *piAudioLang1, int *piAudioLang2, int *piAudioComponentTag, int *piVideoComponentTag, int *piSubTitleComponentTag, int *piStartMJD, int *piStartHH, int *piStartMM, int *piStartSS, int *piDurationHH, int *piDurationMM, int *piDurationSS, unsigned short *pusEvtName, int *piEvtNameLen, unsigned char *pucLogoStream, int *piLogoStreamSize, unsigned short *pusSimpleLogo, int *uiSimpleLogoLength);
extern int tcc_db_get_channel_info2(int iChannelNumber, int iServiceID, int *piGenre);
extern int tcc_db_get_handover_info(int iChannelNumber, int iServiceID, void *pDescTDSD, void *pDescEBD);
extern int tcc_db_get_representative_channel(int iChannelNumber, int iServiceType, int *piRowID, int *piTotalAudioCount, int piAudioPID[], int *piTotalVideoCount, int piVideoPID[], int *piStPID, int *piSiPID, int *piServiceType, int *piServiceID, int *piPMTPID, int *piCAECMPID, int *piACECMPID, int *piNetworkID, int piAudioStreamType[],  int piAudioComponentTag[], int piVideoStreamType[], int piVideoComponentTag[], int *piPCRPID, int *piThreeDigitNumber, int *piAudioIndex, int *piAudioRealIndex, int *piVideoIndex, int *piVideoRealIndex);
// Noah / 20170522 / Added for IM478A-31 (Primary Service)
extern int tcc_db_get_primaryService(int iChannelNumber, int iServiceType, int *piRowID, int *piTotalAudioCount, int piAudioPID[], int *piTotalVideoCount, int piVideoPID[], int *piStPID, int *piSiPID, int *piServiceType, int *piServiceID, int *piPMTPID, int *piCAECMPID, int *piACECMPID, int *piNetworkID, int piAudioStreamType[], int piAudioComponentTag[], int piVideoStreamType[], int piVideoComponentTag[], int *piPCRPID, int *piThreeDigitNumber, int *piAudioIndex, int *piAudioRealIndex, int *piVideoIndex, int *piVideoRealIndex);
extern void tcc_db_delete_channel(int iChannelNumber, int iNetworkID);
extern void tcc_db_create_epg(int iChannelNumber, int iNetworkID);
extern void tcc_db_delete_epg(int iCheckMJD);
// Noah, 20180611, NOT support now
//extern void tcc_db_commit_epg(int iDayOffset, int iChannel, int iNetworkID);
extern void tcc_db_delete_logo(int iChannelNumber, int iServiceID);
extern void tcc_db_get_channel_for_pvr(void *p, unsigned int ch_num, unsigned int service_id);
extern void tcc_db_get_service_for_pvr(void *p, unsigned int ch_num, unsigned int service_id);
extern void tcc_db_get_audio_for_pvr(void *p, unsigned int ch_num, unsigned int service_id);
extern int tcc_db_get_channel_data_ForChannel(ST_CHANNEL_DATA *p, unsigned int uiChannelNo, unsigned int uiChannelServiceID);
extern int tcc_db_get_channel_rowid (unsigned int uiChannelNumber, unsigned int  uiCountryCode, unsigned int uiNetworkID, unsigned int uiServiceID, unsigned int uiServiceType);
// Noah / 20171026 / Added for IM478A-52 (database less tuing)
extern int tcc_db_get_channel_rowid_new(unsigned int uiChannelNumber, unsigned int uiServiceID);
extern int tcc_db_get_channel_current_date_precision(int *mjd, int *year, int *month, int *day, int *hour, int *minute, int *second, int *msec);
extern int tcc_db_get_service_info(int iChannelNumber, int iCountryCode, int *piServiceID, int *piServiceType);
extern int tcc_db_get_service_num(int iChannelNumber, int iCountryCode, int NetworkID);
extern int UpdateDB_UserData_RegionID(unsigned int uiRegionID);
extern int UpdateDB_UserData_PrefecturalCode(unsigned int uiPrefecturalBitOrder);
extern int UpdateDB_UserData_AreaCode(unsigned int uiAreaCode);
extern int UpdateDB_UserData_PostalCode(char *pucPostalCode);
extern void tcc_db_get_userDatas(unsigned int *puiRegionID, unsigned long long *pullPrefecturalCode, unsigned int *puiAreaCode, char *pcPostalCode);
extern BOOL UpdateDB_MailTable_DeleteStatus(int uiMessageID, int uiDelete_status);
extern BOOL UpdateDB_MailTable_UserViewStatus(int uiMessageID, int uiUserView_status);
extern BOOL UpdateDB_MailTable(int uiCA_system_id, int uiMessage_ID, int uiYear, int uiMonth, int uiDay, int uiMail_length, char *ucMailpayload, int uiUserView_status,	int uiDelete_status);
extern int OpenMailDB_Table(void);
extern int UpdateDB_CustomSearch(int channel, int country_code, int option, int *tsid, void *ptr);
extern int UpdateDB_Del_CustomSearch (int preset);
extern int UpdateDB_CustomSearch_AddReport_SignalStrength(int signal_strength, void *ptr);
extern int UpdateDB_ArrangeSpecialService (void);
extern int UpdateDB_CheckNetworkID (int network_id);
extern int UpdateDB_ArrangeDataService(void);
extern int UpdateDB_GetInfoLogoDB (unsigned int channel, unsigned int country_code, unsigned int network_id, unsigned int service_id, unsigned int *dwnl_id, unsigned int *logo_id, unsigned short *sSimpleLogo, unsigned int *simplelogo_len);
extern int UpdateDB_UpdateInfoLogoDB (unsigned int channel, unsigned int country_code, unsigned int network_id, unsigned int dwnl_id, unsigned int logo_id, unsigned short *sSimpleLogo, unsigned int simplelogo_len);
extern int UpdateDB_GetInfoChannelNameDB (unsigned int channel, unsigned int country_code, unsigned network_id, unsigned service_id, unsigned short *sServiceName, unsigned int *service_name_len);
extern int UpdateDB_UpdateChannelNameDB (unsigned int channel, unsigned int country_code, unsigned int network_id);
// Noah / 20170522 / Added for IM478A-31 (Primary Service)
extern BOOL UpdateDB_PrimaryServiceFlag_ChannelTable(int uiServiceID, int uiCurrentChannelNumber, int uiCurrentCountryCode, int uiPrimaryServiceFlagValue);
// Noah / 20180131 / IM478A-77 (relay station search in the background using 2TS)
extern int UpdateDB_CustomSearch_2(int channel, int country_code, int preset, int tsid, void *ptr);
// Noah / 20171226 / IM478A-77 (relay station search in the background using 2TS)
extern int InsertDB_ChannelTable_2(int channelNumber,
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

//Seamless
extern int OpenDfsDB_Table(void);
extern int CloseDfsDB_Table(void);
extern int tcc_db_get_channel_tstreamid (unsigned int uiChannelNumber);
extern BOOL UpdateDB_DfsDB_Table(unsigned int uiChannelNumber, int uiTotal, int uiAverage);
extern int GetDFSValue_DB_Table(int uiChannelNumber, int *piDFSval);
extern int GetDFSCnt_DB_Table(int uiChannelNumber, int *piDFScnt);
extern int tcc_db_get_channel_primary (int uiServiceID);
extern int tcc_db_get_one_service_num(int iChannelNumber);

#ifdef __cplusplus
}
#endif

#endif
