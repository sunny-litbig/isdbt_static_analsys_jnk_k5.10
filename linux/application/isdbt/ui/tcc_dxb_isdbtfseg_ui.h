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
#ifndef __TCC_DXB_ISDBTFSEG_UI_H__
#define __TCC_DXB_ISDBTFSEG_UI_H__

#include "tcc_message.h"
//#include "tcc_dxb_isdbtfseg_db_config.h"
//#include "isdbt_tuner_space.h"
#if 0//def TCC_GUI_PVR_SUPPORTED
#include <JP_Lib_C_Std.h>
#include <JP_Lib_C_D_LinkedList.h>
#endif

#define	BCAS_CARD_IDENTIFIER_INFO_SIZE			5
#define	BCAS_CARD_INDIVIDUAL_ID_INFO_SIZE		25
#define	BCAS_CARD_GROUP_ID_INFO_SIZE			25

#define MAX_RMT_NUMBER							12
#define MAX_CHANNEL_NAME						256
#define MAX_CHANNEL_NAME_SCAN					64
#define MAX_FOUND_CHANNEL_SCAN					10
#define MAX_EWS_MESSAGE							1

/******************************************************************************
* typedefs & structure
******************************************************************************/
typedef enum DxB_INTERFACE_EVENT_ERR_CODE
{
    DxB_INTERFACE_EVENT_ERR_OK= 0,
    DxB_INTERFACE_EVENT_ERR_ERR,
    DxB_INTERFACE_EVENT_ERR_NOFREESPACE,
    DxB_INTERFACE_EVENT_ERR_FILEOPENERROR,
    DxB_INTERFACE_EVENT_ERR_INVALIDMEMORY,
    DxB_INTERFACE_EVENT_ERR_INVALIDOPERATION,
    DxB_INTERFACE_EVENT_ERR_MAX_SIZE,
    DxB_INTERFACE_EVENT_ERR_PLAY_ENDOFFILE,
    DxB_INTERFACE_EVENT_ERR_MAX,
} DxB_INTERFACE_EVENT_ERR_CODE;

typedef enum DxB_INTERFACE_EVENT_INFO_CODE
{
    DxB_INTERFACE_EVENT_INFO_RECORD_DROPDATA = 0,
    DxB_INTERFACE_EVENT_INFO_MAX,
} DxB_INTERFACE_EVENT_INFO_CODE;

typedef struct tag_dxb_parentlock_info
{
	int lock;
	int setAgeRate;
	int curAgeRate;
} dxb_parentlock_info;

typedef struct tag_dxb_service_timeinfo
{
	int mjd;
	int year;
	int month;
	int day;
	int hour;
	int minute;
	int second;
} dxb_service_timeinfo;

typedef struct tag_dxb_recording_timeinfo
{
	int hour;
	int minute;
	int second;
} dxb_recording_timeinfo;

typedef struct tag_dxb_fileplay_timeinfo
{
	int playtotaltime;
	int playcurrtime;
	int	steptime;
} dxb_fileplay_timeinfo;

typedef struct tag_dxb_signal_strength
{
	int bLock;
	int barlevel;
	int perlevel;
} dxb_signal_strength;

typedef struct tag_dxb_bcas_card_info
{
	int	bExist;

	char card_identifier[BCAS_CARD_IDENTIFIER_INFO_SIZE];		// ex) "M002"
	char card_individualID[BCAS_CARD_INDIVIDUAL_ID_INFO_SIZE];	// ex) "0000 5000 0990 7861 3827"
	char card_groupID[BCAS_CARD_GROUP_ID_INFO_SIZE];			// ex) "???? ???? ???? ???? ????"
} dxb_bcas_card_info;

typedef struct tag_dxb_scan_frequency_info
{
	int					scannedfrequencyidx;
	int					minfrequency;
	int					maxfrequency;
	unsigned char		scannedfrequencynum[32];
	unsigned char		scannedcenterfreq[32];
	unsigned char		minfrequencynum[32];
	unsigned char		mincenterfreq[32];
} dxb_scan_frequency_info;

typedef struct tag_dxb_scanned_program_info
{
	unsigned int	RmtNum;
	unsigned int	ChannelNo;
	unsigned int	ThreedigitNo;
	unsigned int	ServiceType;
	unsigned int	ServiceID;
	char			ChName[MAX_CHANNEL_NAME_SCAN];
} dxb_scanned_program_info;

typedef struct {
	unsigned int tmccLock;
	unsigned int mer[3];
	unsigned int vBer[3];
	unsigned int pcber[3];
	unsigned int tsper[3];
	int rssi;
	unsigned int bbLoopGain;
	unsigned int rfLoopGain;
	unsigned int snr;
	unsigned int frequency;
} _Tcc353xStatusToUi;

typedef struct tagb_dxb_soc_debug_info
{
	int bLock;
	int barlevel;
	int perlevel;

	_Tcc353xStatusToUi tcc353xStatUi;
} dxb_soc_debug_info;

typedef struct tagb_dxb_logo_info
{
	int found;
	int netid;
	int dwnlid;
	int logoid;
	int logotype;
	int width;
	int height;
	unsigned char* rgb888;
} dxb_logo_info;

typedef struct tag_dxb_middleware_request
{
	int change_dualchannel_state;
} dxb_middleware_request;

typedef struct tag_dxb_ews_data
{
	int curr_area;
	int ews_area;
	int start_end_flag;
	int signal_type;
} dxb_ews_data;

typedef struct tag_dxb_curevent_data
{
	int	isUpdated;
	int iserviceId;
	int	curMJD;
	int startHH;
	int startMM;
	int durationHH;
	int durationMM;
	unsigned char curEvent[256];
} dxb_curevent_data;

typedef struct tag_dxb_curr_channel_info
{
	int subtitle_type;						// 0:[Not Exist],	1:[Exist]
	int video_resolution_type;				// 0:[Not Exist],	1:[1SEG],		2:[SD],			3[HD]
	int video_aspect_type;					// 0:[Not Exist],	1:[4:3],		2:[16:9]
	int audio_channel_type;					// 0:[Not Exist],	1:[Mono], 		2:[STEREO],		3[DUALMONO]
} dxb_curr_channel_info;

typedef struct tag_TCC_DXB_ISDBTFSEG_UI_Info
{
	unsigned int 				fseg_state;
	unsigned int				dxb_service_aliveReady;
	unsigned int				countrycode;
	unsigned int				prefecture_id;
	unsigned int				widearea_id;
	unsigned int 				frequency_range;
	unsigned int				scan_search_progress_percent;
	unsigned int				scaningfullcount;
	unsigned int				scaningonecount;

	unsigned int 				fullseg_chcount;
	unsigned int 				oneseg_chcount;
	unsigned int				currentservice_type;
	unsigned int				curfullseg_index;
	unsigned int				curoneseg_index;
	unsigned int				curChannelDBIndex;
	unsigned int				oneseg_nostream;

	int							curRmtNum;
	int 						curChannelNo;
	int 						curThreedigitNo;
	int							cur2digitNo;
	int							curFourthDigit;
	int							curServiceID;
	int							curvideoPID;
	char						curChName[MAX_CHANNEL_NAME];

	unsigned int 				curEPGChannelNo;
	unsigned int 				curEPG2DigitNo;
	unsigned int 				curEPG3DigitNo;
	int							curEPG4DigitNo;
	unsigned int 				curEPGEventID;
	unsigned int 				curEPGRMTNo;
	char*						curEPGChName;

	unsigned int				curEWSTunerflag;
	unsigned int				curEWSmessage_flag;
	unsigned int				curEWSmessage_count;
	dxb_ews_data				curEWSmessage[MAX_EWS_MESSAGE];

	unsigned int				curHANDOVERflag;
	unsigned int				curHANDOVERFrequency;
	unsigned int				curHANDOVER_last_channel_threedigitnumber;
	unsigned int				curChannelDBUpdated_OnHandOver_flag;
	unsigned int				curChannelDBUpdated_OnHandOver_index;
	unsigned int				curChannelDBUpdated_OnAirPlay_flag;
	unsigned int				curChannelDBUpdated_OnAirPlay_index;

	unsigned int				needEpgRequest;
	dxb_curevent_data			curEPGEvent;

	unsigned int				IsLogoUpdated;
	dxb_logo_info				logoinfo;

	unsigned int				CurrVideoResolution;
	unsigned int				PrevVideoResolution;
	unsigned int				SavedVideoWidth;
	unsigned int				SavedVideoHeight;

	unsigned int				iFound_Rmt_Program;

	unsigned int				channel_audio_total;
	unsigned int				curr_channel_audiotrack;
	int							record_errorcode;
	int							record_infocode;
	int							record_filesize;
	int							record_freesize;
	int							record_totalesize;
	int							fileplayindex;
	int							filecountrycode;
	char						filpath[MAX_CHANNEL_NAME];
	char						filename[MAX_CHANNEL_NAME];
	DxB_INTERFACE_EVENT_ERR_CODE	file_errorcode;

	dxb_scan_frequency_info		curFrequencyInfo;
	dxb_curr_channel_info		currCHANNELInfo;
	dxb_scanned_program_info 	found_Rmt_program_info[MAX_FOUND_CHANNEL_SCAN];
	dxb_service_timeinfo		curServicetime;
	dxb_parentlock_info			curParentLockInfo;
	dxb_signal_strength			tunerSignalStrength;
	dxb_bcas_card_info			BCASCardInfo;
	dxb_middleware_request		middlewareRequest;
	dxb_soc_debug_info			SocDebugInfo;
	dxb_recording_timeinfo		recording_timeinfo;
	dxb_fileplay_timeinfo		fileplay_timeinfo;
} TCC_DXB_ISDBTFSEG_UI_InfoStruct;

#if 0//def TCC_GUI_PVR_SUPPORTED
JP_D_LinkedList_IF_T *ptFileList;

typedef struct tag_FileListDataInfo_T
{
	unsigned int fileIndex;
	unsigned int countrycode;
	unsigned int date;
	unsigned int stime;
	unsigned int etime;
	unsigned long long filesize;
	unsigned char density[8];
	unsigned char fullpath[256];
	unsigned char proname[128];
} FileListDataInfo_T;
#endif //#ifdef TCC_GUI_PVR_SUPPORTED

/******************************************************************************
* extern variables
******************************************************************************/
extern TCC_DXB_ISDBTFSEG_UI_InfoStruct G_TCC_DXB_ISDBTFSEG_UI_Info;
extern const unsigned char	TCC_DXB_ISDBTFSEG_GUI_CHDB_PathName[2/*ISDBT_CHANNEL_DB_INDEX_MAX*/][256];

/******************************************************************************
* extern functions
******************************************************************************/

/******************************************************************************
* declarations
******************************************************************************/
void TCC_UI_DXB_ISDBTFSEG_Exit(Message_Type MsgType);
void TCC_UI_DXB_ISDBTFSEG_Search(Message_Type MsgType, int Scantype, int Countrycode, int AreadCode, int ChannelNum, int Options );
void TCC_UI_DXB_ISDBTFSEG_SearchCancel(Message_Type MsgType);
void TCC_UI_DXB_ISDBTFSEG_SetChannel(Message_Type MsgType, int mainRowID, int subRowID, int audioIndex, int videoIndex, int audioMode, int raw_w, int raw_h, int view_w, int view_h, int ch_index);
void TCC_UI_DXB_ISDBTFSEG_SetChannel_withChNum(Message_Type MsgType, int channelNumber, int serviceID_Fseg, int serviceID_1seg, int audioMode, int raw_w, int raw_h, int view_w, int view_h, int ch_index);
void TCC_UI_DXB_ISDBTFSEG_Stop(Message_Type MsgType);
void TCC_UI_DXB_ISDBTFSEG_SetDualChannel(Message_Type MsgType, int index);
void TCC_DXB_ISDBTFSEG_UI_Show(Message_Type MsgType);
int TCC_UI_DXB_ISDBTFSEG_GetCurrentDateTime(Message_Type MsgType, int *piMJD, int *piHour, int *piMin, int *piSec, int *piPolarity, int *piHourOffset, int *piMinOffset);
int TCC_UI_DXB_ISDBTFSEG_Prepare(Message_Type MsgType, int iSpecificInfo);
int TCC_UI_DXB_ISDBTFSEG_Start(Message_Type MsgType, int iCountryCode);
int TCC_UI_DXB_ISDBTFSEG_StartTestMode(Message_Type MsgType, int iIndex);
int TCC_UI_DXB_ISDBTFSEG_Release(Message_Type MsgType);
int TCC_UI_DXB_ISDBTFSEG_ChannelBackUp(Message_Type MsgType);
int TCC_UI_DXB_ISDBTFSEG_ChannelDBRestoration(Message_Type MsgType);
int TCC_UI_DXB_ISDBTFSEG_SetEPG(Message_Type MsgType, int iOption);
int TCC_UI_DXB_ISDBTFSEG_EPGSearch(Message_Type MsgType, int iChannelNum, int iOption);
int TCC_UI_DXB_ISDBTFSEG_EPGSearchCancel(Message_Type MsgType);
int TCC_UI_DXB_ISDBTFSEG_ReleaseSurface(Message_Type MsgType);
int TCC_UI_DXB_ISDBTFSEG_UseSurface(Message_Type MsgType, int iArg);
int TCC_UI_DXB_ISDBTFSEG_SetCountryCode(Message_Type MsgType, int iCountryCode);
int TCC_UI_DXB_ISDBTFSEG_SetGroup(Message_Type MsgType, int iIndex);
int TCC_UI_DXB_ISDBTFSEG_SetAudio(Message_Type MsgType, int iIndex);
int TCC_UI_DXB_ISDBTFSEG_SetVideo(Message_Type MsgType, int iIndex);
int TCC_UI_DXB_ISDBTFSEG_SetAudioOnOff(Message_Type MsgType, int iOnOff);
int TCC_UI_DXB_ISDBTFSEG_SetVideoOnOff(Message_Type MsgType, int iOnOff);
int TCC_UI_DXB_ISDBTFSEG_SetAudioVideoSyncMode(Message_Type MsgType, int iMode);
int TCC_UI_DXB_ISDBTFSEG_SetVolume(Message_Type MsgType, int iVolume);
int TCC_UI_DXB_ISDBTFSEG_SetVolumeLR(Message_Type MsgType, int iLeftVolume, int iRightVolume);
int TCC_UI_DXB_ISDBTFSEG_SetDisplayPosition(Message_Type MsgType, int iX, int iY, int iWidth, int iHeight, int iRotate);
int TCC_UI_DXB_ISDBTFSEG_SetCapture(Message_Type MsgType, char *pcFilePath);
int TCC_UI_DXB_ISDBTFSEG_PlaySubtitle(Message_Type MsgType, int iOnOff);
int TCC_UI_DXB_ISDBTFSEG_PlaySuperimpose(Message_Type MsgType, int iOnOff);
int TCC_UI_DXB_ISDBTFSEG_PlayPng(Message_Type MsgType, int iOnOff);
int TCC_UI_DXB_ISDBTFSEG_SetSubtitle(Message_Type MsgType, int iIndex);
int TCC_UI_DXB_ISDBTFSEG_SetSuperimpose(Message_Type MsgType, int iIndex);
int TCC_UI_DXB_ISDBTFSEG_SetAudioDualMono(Message_Type MsgType, int iAudioSelect);
int TCC_UI_DXB_ISDBTFSEG_SetParentalRate(Message_Type MsgType, int iAge);
int TCC_UI_DXB_ISDBTFSEG_SetArea(Message_Type MsgType, int iAreaCode);
int TCC_UI_DXB_ISDBTFSEG_SetPreset(Message_Type MsgType, int iAreaCode);
int TCC_UI_DXB_ISDBTFSEG_SetHandover(Message_Type MsgType, int iCountryCode);
int TCC_UI_DXB_ISDBTFSEG_RequestEPGUpdate(Message_Type MsgType, int iDayOffset);
int TCC_UI_DXB_ISDBTFSEG_ReqSCInfo(Message_Type MsgType, int iArg);
int TCC_UI_DXB_ISDBTFSEG_SetFreqBand(Message_Type MsgType, int iFreqBand);
int TCC_UI_DXB_ISDBTFSEG_SetFieldLog(Message_Type MsgType, char *pcPath, int iOnOff);
int TCC_UI_DXB_ISDBTFSEG_SetPresetMode(Message_Type MsgType, int iPresetMode);
int TCC_UI_DXB_ISDBTFSEG_SetUserDataRegionID(Message_Type MsgType, int iRegionID);
int TCC_UI_DXB_ISDBTFSEG_SetUserDataPrefecturalCode(Message_Type MsgType, int iPrefecturalBitOrder);
int TCC_UI_DXB_ISDBTFSEG_SetUserDataAreaCode(Message_Type MsgType, int iAreaCode);
int TCC_UI_DXB_ISDBTFSEG_SetUserDataPostalCode(Message_Type MsgType, char *pcPostalCode);
int TCC_UI_DXB_ISDBTFSEG_SetListener(Message_Type MsgType, int (*pfListener)(int, int, int, void *));
int TCC_UI_DXB_ISDBTFSEG_IsPlaying(Message_Type MsgType);
int TCC_UI_DXB_ISDBTFSEG_GetVideoWidth(Message_Type MsgType, int *piWidth);
int TCC_UI_DXB_ISDBTFSEG_GetVideoHeight(Message_Type MsgType, int *piHeight);
int TCC_UI_DXB_ISDBTFSEG_GetSignalStrength(Message_Type MsgType, int *piSQInfo);
int TCC_UI_DXB_ISDBTFSEG_CustomRelayStationSearchRequest(Message_Type MsgType, int iSearchKind, int *piListChannel, int *piListTSID, int iRepeat);
int TCC_UI_DXB_ISDBTFSEG_GetCountryCode(Message_Type MsgType);
int TCC_UI_DXB_ISDBTFSEG_GetAudioCnt(Message_Type MsgType);
int TCC_UI_DXB_ISDBTFSEG_GetChannelInfo(Message_Type MsgType, int iChannelNumber, int iServiceID,
											int *piRemoconID, int *piThreeDigitNumber, int *piServiceType,
											unsigned short *pusChannelName, int *piChannelNameLen,
											int *piTotalAudioPID, int *piTotalVideoPID, int *piTotalSubtitleLang,
											int *piAudioMode, int *piVideoMode,
											int *piAudioLang1, int *piAudioLang2,
											int *piAudioComponentTag, int *piVideoComponentTag,
											int *piStartMJD, int *piStartHH, int *piStartMM, int *piStartSS,
											int *piDurationHH, int *piDurationMM, int *piDurationSS,
											unsigned short *pusEvtName, int *piEvtNameLen,
											int *piLogoImageWidth, int *piLogoImageHeight, unsigned int *pusLogoImage,
											unsigned short *pusSimpleLogo, int *piSimpleLogoLength,
											int *piMVTVGroupType);
int TCC_UI_DXB_ISDBTFSEG_ReqTRMPInfo(Message_Type MsgType, unsigned char **ppucInfo, int *piInfoSize);
int TCC_UI_DXB_ISDBTFSEG_GetSubtitleLangInfo(Message_Type MsgType, int *piSubtitleLangNum, int *piSuperimposeLangNum, unsigned int *puiSubtitleLang1, unsigned int *puiSubtitleLang2, unsigned int *puiSuperimposeLang1, unsigned int *puiSuperimposeLang2);
int TCC_UI_DXB_ISDBTFSEG_GetUserData(Message_Type MsgType, unsigned int *puiRegionID, unsigned long long *pullPrefecturalCode, unsigned int *puiAreaCode, char *pcPostalCode);
int TCC_UI_DXB_ISDBTFSEG_SetCustomTuner(Message_Type MsgType, int iSize, void *pvArg);
int TCC_UI_DXB_ISDBTFSEG_GetCustomTuner(Message_Type MsgType, int *piSize, void *pvArg);
int TCC_UI_DXB_ISDBTFSEG_GetDigitalCopyControlDescriptor(Message_Type MsgType, unsigned short usServiceID);
int TCC_UI_DXB_ISDBTFSEG_GetContentAvailabilityDescriptor(Message_Type MsgType, unsigned short usServiceID, unsigned char *pucCopy_restriction_mode, unsigned char *pucImage_constraint_token, unsigned char *pucRetention_mode, unsigned char *pucRetention_state, unsigned char *pucEncryption_mode);
int TCC_UI_DXB_ISDBTFSEG_SetNotifyDetectSection(Message_Type MsgType, int iSectionIDs);
int TCC_UI_DXB_ISDBTFSEG_SearchAndSetChannel(Message_Type MsgType, int iCountryCode, int iChannelNum, int iTsid, int iOptions, int iAudioIndex, int iVideoIndex, int iAudioMode, int iRaw_w, int iRaw_h, int iView_w, int iView_h, int iCh_index);
int TCC_UI_DXB_ISDBTFSEG_SetDeviceKeyUpdateFunction(Message_Type MsgType, unsigned char *(*pfUpdateDeviceKey)(unsigned char , unsigned int));
int TCC_UI_DXB_ISDBTFSEG_GetVideoState(Message_Type MsgType);
int TCC_UI_DXB_ISDBTFSEG_GetSearchState(Message_Type MsgType);
int TCC_UI_DXB_ISDBTFSEG_SetDataServiceStart(Message_Type MsgType);
int TCC_UI_DXB_ISDBTFSEG_CustomFilterStart(Message_Type MsgType, int iPID, int iTableID);
int TCC_UI_DXB_ISDBTFSEG_CustomFiterStop(Message_Type MsgType, int iPID);
void TCC_UI_DXB_ISDBTFSEG_GetSeamlessValue(Message_Type MsgType, int *state, int *cval, int *pval);

#endif //ifdef __TCC_DXB_ISDBTFSEG_UI_H__

