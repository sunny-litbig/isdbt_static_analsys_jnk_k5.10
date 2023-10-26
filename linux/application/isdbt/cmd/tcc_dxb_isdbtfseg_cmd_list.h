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
#ifndef	_TCC_DXB_ISDBTFSEG_CMD_LIST_H__
#define	_TCC_DXB_ISDBTFSEG_CMD_LIST_H__

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
* include
******************************************************************************/
//#include "main.h"
//#include "globals.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include "tcc_dxb_isdbtfseg_cmd.h"
#include "tcc_dxb_isdbtfseg_ui.h"

/******************************************************************************
* defines
******************************************************************************/
#ifndef TRUE
#define	TRUE	1
#define	FALSE	0
#endif

/******************************************************************************
* typedefs & structure
******************************************************************************/
typedef struct
{
	int iRet;
} TCC_DXB_ISDBTFSEG_RETURN_PARAM_t;

/* UI -----> PROCESS */

typedef struct tag_TCC_DXB_ISDBTFSEG_CMD_LIST_Exit_T
{
	int iResult;
} TCC_DXB_ISDBTFSEG_CMD_LIST_Exit_T;

typedef struct tag_TCC_DXB_ISDBTFSEG_CMD_LIST_Scan_T
{
	int iScanType;
	int iCountryCode;
	int iAreaCode;
	int iChannelNum;
	int iOptions;
	int	iResult;
} TCC_DXB_ISDBTFSEG_CMD_LIST_Scan_T;

typedef struct tag_TCC_DXB_ISDBTFSEG_CMD_LIST_ScanStop_T
{
	int	iResult;
} TCC_DXB_ISDBTFSEG_CMD_LIST_ScanStop_T;

typedef struct tag_TCC_DXB_ISDBTFSEG_CMD_LIST_Set_T
{
	int mainRowID;
	int subRowID;
	int audioIndex;
	int videoIndex;
	int audioMode;
	int ch_index;
	int raw_w;
	int raw_h;
	int view_w;
	int view_h;
	int	iResult;
} TCC_DXB_ISDBTFSEG_CMD_LIST_Set_T;

typedef struct tag_TCC_DXB_ISDBTFSEG_CMD_LIST_Set_T_WITHCHNUM
{
	int channelNumber;
	int serviceID_Fseg;
	int serviceID_1seg;
	int audioMode;
	int ch_index;
	int raw_w;
	int raw_h;
	int view_w;
	int view_h;
	int	iResult;
} TCC_DXB_ISDBTFSEG_CMD_LIST_Set_T_WITHCHNUM;

typedef struct tag_TCC_DXB_ISDBTFSEG_CMD_LIST_Stop_T
{
	int iResult;
} TCC_DXB_ISDBTFSEG_CMD_LIST_Stop_T;

typedef struct tag_TCC_DXB_ISDBTFSEG_CMD_LIST_SetDualChannel_T
{
	int iChannelIndex;
	int	iResult;
} TCC_DXB_ISDBTFSEG_CMD_LIST_SetDualChannel_T;

typedef struct
{
	int iRet;
	int *piMJD;
	int *piHour;
	int *piMin;
	int *piSec;
	int *piPolarity;
	int *piHourOffset;
	int *piMinOffset;
} TCC_DXB_ISDBTFSEG_GET_CURRENT_DATETIME_PARAM_t;

typedef struct
{
	int iRet;
	int iMJD;
	int iHour;
	int iMin;
	int iSec;
	int iPolarity;
	int iHourOffset;
	int iMinOffset;
	int iResult;
	TCC_DXB_ISDBTFSEG_GET_CURRENT_DATETIME_PARAM_t *pGetCurrentDateTime_Param;
} TCC_DXB_ISDBTFSEG_GET_CURRENT_DATETIME_CMD_t;

typedef struct
{
	int iRet;
	int iSpecificInfo;
	int iResult;
} TCC_DXB_ISDBTFSEG_PREPARE_CMD_t;

typedef struct
{
	int iRet;
	int iCountryCode;
	int iResult;
} TCC_DXB_ISDBTFSEG_START_CMD_t;

typedef struct
{
	int iRet;
	int iIndex;
	int iResult;
} TCC_DXB_ISDBTFSEG_START_TEST_MODE_CMD_t;

typedef struct
{
	int iRet;
	int iResult;
} TCC_DXB_ISDBTFSEG_RELEASE_CMD_t;

typedef struct
{
	int iRet;
	int iResult;
} TCC_DXB_ISDBTFSEG_CHANNEL_BACKUP_CMD_t;

typedef struct
{
	int iRet;
	int iResult;
} TCC_DXB_ISDBTFSEG_CHANNEL_DB_RESTORATION_CMD_t;

typedef struct
{
	int iRet;
	int iOption;
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_EPG_CMD_t;

typedef struct
{
	int iRet;
	int iChannelNum;
	int iOption;
	int iResult;
} TCC_DXB_ISDBTFSEG_EPG_SEARCH_CMD_t;

typedef struct
{
	int iRet;
	int iResult;
} TCC_DXB_ISDBTFSEG_EPG_SEARCH_CANCEL_CMD_t;

typedef struct
{
	int iRet;
	int iResult;
} TCC_DXB_ISDBTFSEG_RELEASE_SURFACE_CMD_t;

typedef struct
{
	int iRet;
	int iArg;
	int iResult;
} TCC_DXB_ISDBTFSEG_USE_SURFACE_CMD_t;

typedef struct
{
	int iRet;
	int iCountryCode;
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_COUNTRYCODE_CMD_t;

typedef struct
{
	int iRet;
	int iIndex;
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_GROUP_CMD_t;

typedef struct
{
	int iRet;
	int iIndex;
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_AUDIO_CMD_t;

typedef struct
{
	int iRet;
	int iIndex;
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_VIDEO_CMD_t;

typedef struct
{
	int iRet;
	int iOnOff;
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_AUDIO_ONOFF_CMD_t;

typedef struct
{
	int iRet;
	int iOnOff;
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_AUDIO_MUTE_ONOFF_CMD_t;

typedef struct
{
	int iRet;
	int iOnOff;
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_VIDEO_ONOFF_CMD_t;

typedef struct
{
	int iRet;
	int iOnOff;
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_VIDEO_OUTPUT_CMD_t;

typedef struct
{
	int iRet;
	int iMode;
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_AV_SYNC_MODE_CMD_t;

typedef struct
{
	int iRet;
	int iVolume;
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_VOLUME_CMD_t;

typedef struct
{
	int iRet;
	int iLeftVolume;
	int iRightVolume;
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_VOLUME_LR_CMD_t;

typedef struct
{
	int iRet;
	int iX;
	int iY;
	int iWidth;
	int iHeight;
	int iRotate;
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_DISPLAY_POSITION_CMD_t;

typedef struct
{
	int iRet;
	char acFilePath[128];
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_CAPTURE_CMD_t;

typedef struct
{
	int iRet;
	int iOnOff;
	int iResult;
} TCC_DXB_ISDBTFSEG_PLAY_SUBTITLE_CMD_t;

typedef struct
{
	int iRet;
	int iOnOff;
	int iResult;
} TCC_DXB_ISDBTFSEG_PLAY_SUPERIMPOSE_CMD_t;

typedef struct
{
	int iRet;
	int iOnOff;
	int iResult;
} TCC_DXB_ISDBTFSEG_PLAY_PNG_CMD_t;

typedef struct
{
	int iRet;
	int iIndex;
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_SUBTITLE_CMD_t;

typedef struct
{
	int iRet;
	int iIndex;
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_SUPERIMPOSE_CMD_t;

typedef struct
{
	int iRet;
	int iAudioSelect;
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_AUDIO_DUALMONO_CMD_t;

typedef struct
{
	int iRet;
	int iAge;
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_PARENTALRATE_CMD_t;

typedef struct
{
	int iRet;
	int iAreaCode;
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_AREA_CMD_t;

typedef struct
{
	int iRet;
	int iAreaCode;
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_PRESET_CMD_t;

typedef struct
{
	int iRet;
	int iCountryCode;
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_HANDOVER_CMD_t;

typedef struct
{
	int iRet;
	int iDayOffset;
	int iResult;
} TCC_DXB_ISDBTFSEG_REQ_EPG_UPDATE_CMD_t;

typedef struct
{
	int iRet;
	int iArg;
	int iResult;
} TCC_DXB_ISDBTFSEG_REQ_SC_INFO_CMD_t;

typedef struct
{
	int iRet;
	int iFreqBand;
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_FREQ_BAND_CMD_t;

typedef struct
{
	int iRet;
	char acPath[128];
	int iOnOff;
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_FIELD_LOG_CMD_t;

typedef struct
{
	int iRet;
	int iPresetMode;
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_PRESET_MODE_CMD_t;

typedef struct
{
	int iRet;
	int iRegionID;
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_USERDATA_REGIONID_CMD_t;

typedef struct
{
	int iRet;
	int iPrefecturalBitOrder;
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_USERDATA_PREFECTURALCODE_CMD_t;

typedef struct
{
	int iRet;
	int iAreaCode;
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_USERDATA_AREACODE_CMD_t;

typedef struct
{
	int iRet;
	char acPostalCode[16];
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_USERDATA_POSTALCODE_CMD_t;

typedef struct
{
	int iRet;
	int (*pfListener)(int, int, int, void *);
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_LISTENER_CMD_t;

typedef struct
{
	int iRet;
	int iResult;
	TCC_DXB_ISDBTFSEG_RETURN_PARAM_t *pIsPlaying_Param;
} TCC_DXB_ISDBTFSEG_IS_PLAYING_CMD_t;

typedef struct
{
	int iRet;
	int *piWidth;
} TCC_DXB_ISDBTFSEG_GET_VIDEO_WIDTH_PARAM_t;

typedef struct
{
	int iRet;
	int iWidth;
	int iResult;
	TCC_DXB_ISDBTFSEG_GET_VIDEO_WIDTH_PARAM_t *pGetVideoWidth_Param;
} TCC_DXB_ISDBTFSEG_GET_VIDEO_WIDTH_CMD_t;

typedef struct
{
	int iRet;
	int *piHeight;
} TCC_DXB_ISDBTFSEG_GET_VIDEO_HEIGHT_PARAM_t;

typedef struct
{
	int iRet;
	int iHeight;
	int iResult;
	TCC_DXB_ISDBTFSEG_GET_VIDEO_HEIGHT_PARAM_t *pGetVideoHeight_Param;
} TCC_DXB_ISDBTFSEG_GET_VIDEO_HEIGHT_CMD_t;

typedef struct
{
	int iRet;
	int *piSQInfo;
} TCC_DXB_ISDBTFSEG_GET_SIGNAL_STRENGTH_PARAM_t;

typedef struct
{
	int iRet;
	int aiSQInfo[256];
	int iResult;
	TCC_DXB_ISDBTFSEG_GET_SIGNAL_STRENGTH_PARAM_t *pGetSignalStrength_Param;
} TCC_DXB_ISDBTFSEG_GET_SIGNAL_STRENGTH_CMD_t;

typedef struct
{
	int iRet;
	int iSearchKind;
	int aiListChannel[128];
	int aiListTSID[128];
	int iRepeat;
	int iResult;
} TCC_DXB_ISDBTFSEG_CUSTOM_RELAY_STATION_SEARCH_REQUEST_CMD_t;

typedef struct
{
	int iRet;
	int iResult;
	TCC_DXB_ISDBTFSEG_RETURN_PARAM_t *pGetCountryCode_Param;
} TCC_DXB_ISDBTFSEG_GET_GOUNTRYCODE_CMD_t;

typedef struct
{
	int iRet;
	int iResult;
	TCC_DXB_ISDBTFSEG_RETURN_PARAM_t *pGetAudioCnt_Param;
} TCC_DXB_ISDBTFSEG_GET_AUDIO_CNT_CMD_t;

typedef struct
{
	int iRet;
	int *piRemoconID;
	int *piThreeDigitNumber;
	int *piServiceType;
	unsigned short *pusChannelName;
	int *piChannelNameLen;
	int *piTotalAudioPID;
	int *piTotalVideoPID;
	int *piTotalSubtitleLang;
	int *piAudioMode;
	int *piVideoMode;
	int *piAudioLang1;
	int *piAudioLang2;
	int *piAudioComponentTag;
	int *piVideoComponentTag;
	int *piStartMJD;
	int *piStartHH;
	int *piStartMM;
	int *piStartSS;
	int *piDurationHH;
	int *piDurationMM;
	int *piDurationSS;
	unsigned short *pusEvtName;
	int *piEvtNameLen;
	int *piLogoImageWidth;
	int *piLogoImageHeight;
	unsigned int *pusLogoImage;
	unsigned short *pusSimpleLogo;
	int *piSimpleLogoLength;
	int *piMVTVGroupType;
} TCC_DXB_ISDBTFSEG_GET_CHANNEL_INFO_PARAM_t;

typedef struct
{
	int iRet;
	int iChannelNumber;
	int iServiceID;
	int iRemoconID;
	int iThreeDigitNumber;
	int iServiceType;
	unsigned short ausChannelName[256];
	int iChannelNameLen;
	int iTotalAudioPID;
	int iTotalVideoPID;
	int iTotalSubtitleLang;
	int iAudioMode;
	int iVideoMode;
	int iAudioLang1;
	int iAudioLang2;
	int iAudioComponentTag;
	int iVideoComponentTag;
	int iSubtitleComponentTag;
	int iStartMJD;
	int iStartHH;
	int iStartMM;
	int iStartSS;
	int iDurationHH;
	int iDurationMM;
	int iDurationSS;
	unsigned short ausEvtName[256];
	int iEvtNameLen;
	int iLogoImageWidth;
	int iLogoImageHeight;
	unsigned int ausLogoImage[5184];
	unsigned short ausSimpleLogo[256];
	int iSimpleLogoLength;
	int iMVTVGroupType;
	int iResult;
	TCC_DXB_ISDBTFSEG_GET_CHANNEL_INFO_PARAM_t *pGetChannelInfo_Param;
} TCC_DXB_ISDBTFSEG_GET_CHANNEL_INFO_CMD_t;

typedef struct
{
	int iRet;
	unsigned char **ppucInfo;
	int *piInfoSize;
} TCC_DXB_ISDBTFSEG_REQ_TRMP_INFO_PARAM_t;

typedef struct
{
	int iRet;
	unsigned char aucInfo[2048];
	int iInfoSize;
	int iResult;
	TCC_DXB_ISDBTFSEG_REQ_TRMP_INFO_PARAM_t *pReqTRMPInfo_Param;
} TCC_DXB_ISDBTFSEG_REQ_TRMP_INFO_CMD_t;

typedef struct
{
	int iRet;
	int *piSubtitleLangNum;
	int *piSuperimposeLangNum;
	unsigned int *puiSubtitleLang1;
	unsigned int *puiSubtitleLang2;
	unsigned int *puiSuperimposeLang1;
	unsigned int *puiSuperimposeLang2;
} TCC_DXB_ISDBTFSEG_GET_SUBTITLE_LANG_INFO_PARAM_t;

typedef struct
{
	int iRet;
	int iSubtitleLangNum;
	int iSuperimposeLangNum;
	unsigned int uiSubtitleLang1;
	unsigned int uiSubtitleLang2;
	unsigned int uiSuperimposeLang1;
	unsigned int uiSuperimposeLang2;
	int iResult;
	TCC_DXB_ISDBTFSEG_GET_SUBTITLE_LANG_INFO_PARAM_t *pGetSubtitleLangInfo_Param;
} TCC_DXB_ISDBTFSEG_GET_SUBTITLE_LANG_INFO_CMD_t;

typedef struct
{
	int iRet;
	unsigned int *puiRegionID;
	unsigned long long *pullPrefecturalCode;
	unsigned int *puiAreaCode;
	char *pcPostalCode;
} TCC_DXB_ISDBTFSEG_GET_USERDATA_PARAM_t;

typedef struct
{
	int iRet;
	unsigned int uiRegionID;
	unsigned long long ullPrefecturalCode;
	unsigned int uiAreaCode;
	char acPostalCode[16];
	int iResult;
	TCC_DXB_ISDBTFSEG_GET_USERDATA_PARAM_t *pGetUserData_Param;
} TCC_DXB_ISDBTFSEG_GET_USERDATA_CMD_t;

typedef struct
{
	int iRet;
	int iSize;
	int aiArg[512];
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_CUSTOM_TUNER_CMD_t;

typedef struct
{
	int iRet;
	int *piSize;
	void *pvArg;
} TCC_DXB_ISDBTFSEG_GET_CUSTOM_TUNER_PARAM_t;

typedef struct
{
	int iRet;
	int iSize;
	int aiArg[512];
	int iResult;
	TCC_DXB_ISDBTFSEG_GET_CUSTOM_TUNER_PARAM_t *pGetCustomTuner_Param;
} TCC_DXB_ISDBTFSEG_GET_CUSTOM_TUNER_CMD_t;

typedef struct
{
	int iRet;
	unsigned char *pucDigital_recording_control_data;
	unsigned char *pucMaximum_bitrate_flag;
	unsigned char *pucComponent_control_flag;
	unsigned char *pucCopy_control_type;
	unsigned char *pucAPS_control_data;
	unsigned char *pucMaximum_bitrate;
} TCC_DXB_ISDBTFSEG_GET_DCCD_PARAM_t;

typedef struct
{
	int iRet;
	unsigned short usServiceID;
	unsigned char ucDigital_recording_control_data;
	unsigned char ucMaximum_bitrate_flag;
	unsigned char ucComponent_control_flag;
	unsigned char ucCopy_control_type;
	unsigned char ucAPS_control_data;
	unsigned char ucMaximum_bitrate;
	int iResult;
	TCC_DXB_ISDBTFSEG_GET_DCCD_PARAM_t *pGetDCCD_Param;
} TCC_DXB_ISDBTFSEG_GET_DCCD_CMD_t;

typedef struct
{
	int iRet;
	unsigned char *pucCopy_restriction_mode;
	unsigned char *pucImage_constraint_token;
	unsigned char *pucRetention_mode;
	unsigned char *pucRetention_state;
	unsigned char *pucEncryption_mode;
} TCC_DXB_ISDBTFSEG_GET_CAD_PARAM_t;

typedef struct
{
	int iRet;
	unsigned short usServiceID;
	unsigned char ucCopy_restriction_mode;
	unsigned char ucImage_constraint_token;
	unsigned char ucRetention_mode;
	unsigned char ucRetention_state;
	unsigned char ucEncryption_mode;
	int iResult;
	TCC_DXB_ISDBTFSEG_GET_CAD_PARAM_t *pGetCAD_Param;
} TCC_DXB_ISDBTFSEG_GET_CAD_CMD_t;

typedef struct
{
	int iRet;
	int iSectionIDs;
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_NOTIFY_DETECT_SECTION_CMD_t;

typedef struct
{
	int iRet;
	int iCountryCode;
	int iChannelNum;
	int iTsid;
	int iOptions;
	int iAudioIndex;
	int iVideoIndex;
	int iAudioMode;
	int iRaw_w;
	int iRaw_h;
	int iView_w;
	int iView_h;
	int iCh_index;
	int iResult;
} TCC_DXB_ISDBTFSEG_SEARCH_AND_SET_CMD_t;

typedef struct
{
	int iRet;
	unsigned char *(*pfUpdateDeviceKey)(unsigned char , unsigned int);
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_DEVICE_KEY_UPDATE_FUNCTION_CMD_t;

typedef struct
{
	int iRet;
	int iResult;
	TCC_DXB_ISDBTFSEG_RETURN_PARAM_t *pGetVideoState_Param;
} TCC_DXB_ISDBTFSEG_GET_VIDEO_STATE_CMD_t;

typedef struct
{
	int iRet;
	int iResult;
	TCC_DXB_ISDBTFSEG_RETURN_PARAM_t *pGetSearchState_Param;
} TCC_DXB_ISDBTFSEG_GET_SEARCH_STATE_CMD_t;

typedef struct
{
	int iRet;
	int iResult;
} TCC_DXB_ISDBTFSEG_SET_DATASERVICE_START_CMD_t;

typedef struct
{
	int iRet;
	int iPID;
	int iTableID;
	int iResult;
} TCC_DXB_ISDBTFSEG_CUSTOM_FILTER_START_CMD_t;

typedef struct
{
	int iRet;
	int iPID;
	int iTableID;
	int iResult;
} TCC_DXB_ISDBTFSEG_CUSTOM_FILTER_STOP_CMD_t;

typedef struct
{
	int mainRowID;
	int subRowID;
	int audioIndex;
	int videoIndex;
	int audioMode;
	int ch_index;
	int raw_w;
	int raw_h;
	int view_w;
	int view_h;
	int iResult;
} TCC_DXB_ISDBTFSEG_EWS_START_CMD_t;

typedef struct
{
	int channelNumber;
	int serviceID_Fseg;
	int serviceID_1seg;
	int audioMode;
	int ch_index;
	int raw_w;
	int raw_h;
	int view_w;
	int view_h;
	int iResult;
} TCC_DXB_ISDBTFSEG_EWS_START_CMD_t_WITHCHNUM;

typedef struct
{
	int iRet;
	int iResult;
} TCC_DXB_ISDBTFSEG_EWS_CLEAR_CMD_t;

typedef struct
{
	int iRet;
	int iOnOff;
	int iStrength;
	int iInterval;
	int iNtimes;
	int iRange;
	int iGapadjust;
	int iGapadjust2;
	int iMultiplier;
	int iResult;
} TCC_DXB_ISDBTFSEG_Seamless_Switch_Compensation_CMD_t;

typedef struct
{
	int iRet;
	unsigned int *hi_data;
	unsigned int *lo_data;
} TCC_DXB_ISDBTFSEG_DS_GET_STC_CMD_t;

typedef struct
{
	int iRet;
	unsigned int *year;
	unsigned int *month;
	unsigned int *day;
	unsigned int *hour;
	unsigned int *min;
	unsigned int *sec;
} TCC_DXB_ISDBTFSEG_DS_GET_SERVICE_TIME_CMD_t;

typedef struct
{
	int iRet;
	unsigned int *contentID;
	unsigned int *associated_contents_flag;
} TCC_DXB_ISDBTFSEG_DS_GET_CONTENTID_CMD_t;

typedef struct
{
	int iRet;
	int target_component_tag;
} TCC_DXB_ISDBTFSEG_DS_CHECK_EXIST_COMPONENTTAG_IN_PMT_CMD_t;

typedef struct
{
	int iRet;
	int component_tag;
} TCC_DXB_ISDBTFSEG_DS_SET_VIDEO_BY_COMPONENTTAG_CMD_t;

typedef struct
{
	int iRet;
	int component_tag;
} TCC_DXB_ISDBTFSEG_DS_SET_AUDIO_BY_COMPONENTTAG_CMD_t;

typedef struct
{
	int iRet;
	int *state;
	int *cval;
	int *pval;
} TCC_DXB_ISDBTFSEG_GET_SEAMLESSVALUE_CMD_t;
/******************************************************************************
* struct of EVENT data
******************************************************************************/
typedef struct
{
	int channelNumber;
	int countryCode;
	int uiVersionNum;
	int audioPID;
	int videoPID;
	int stPID;
	int siPID;
	int PMT_PID;
	int remoconID;
	int serviceType;
	int serviceID;
	int regionID;
	int threedigitNumber;
	int TStreamID;
	int berAVG;
	int preset;
	int networkID;
	int areaCode;
	short *channelName;
} DB_CHANNEL_DATA_t;


typedef struct
{
	int uiPCRPID;
} DB_SERVICE_DATA_t;

typedef struct
{
	int uiServiceID;
	int uiCurrentChannelNumber;
	int uiCurrentCountryCode;
	int uiSubtitlePID;
	char *pucacSubtLangCode;
	int uiSubtType;
	int uiCompositionPageID;
	int uiAncillaryPageID;
} DB_SUBTITLE_DATA_t;

typedef struct
{
	int uiServiceID;
	int uiCurrentChannelNumber;
	int uiCurrentCountryCode;
	int usNetworkID;
	int uiAudioPID;
	int uiAudioIsScrambling;
	int uiAudioStreamType;
	int uiAudioType;
	char *pucacLangCode;
} DB_AUDIO_DATA_t;

typedef struct
{
	DB_CHANNEL_DATA_t dbCh;
	DB_SERVICE_DATA_t dbService;
	DB_AUDIO_DATA_t dbAudio[5];
	DB_SUBTITLE_DATA_t dbSubtitle[10];
} DB_INFO_DATA_t;

typedef struct
{
	char SCDataSize;
	char SCData[196];
} SC_INFO_t;

typedef struct
{
	int MessageID;
	int Broadcaster_GroupID;
	int Deletion_Status;
	int Disp_Duration1;
	int Disp_Duration2;
	int Disp_Duration3;
	int Disp_Cycle;
	int Format_Version;
	int MessageLen;
	int Disp_Horizontal_PositionInfo;
	int Disp_Vertical_PositionInfo;
	char MessagePayload[800];
} AUTO_MESSAGE_INFO_t;

typedef struct
{
	int internal_sound_subtitle_index;
	int mixed_subtitle_size;
	int mixed_subtitle_phy_addr;
} SUBTITLE_DATA_t;

typedef struct
{
		int width;
		int height;
		int aspect_ratio;
		int frame_rate;
		int main_DecoderID;
		int sub_width;
		int sub_height;
		int sub_aspect_ratio;
		int sub_DecoderID;
		int DisplayID;
} VIDEO_DEFINITION_DATA_t;

typedef struct
{
		int serviceID;
		int startendflag;
		int signaltype;
		int area_code_length;
		int localcode[256];
} EWS_DATA_t;

typedef struct _SUB_DIGITAL_COPY_CONTROL_DESCR_t
{
	struct _SUB_DIGITAL_COPY_CONTROL_DESCR_t* pNext;

	unsigned char	component_tag;
	unsigned char	digital_recording_control_data;
	unsigned char	maximum_bitrate_flag;
	unsigned char	copy_control_type;
	unsigned char	APS_control_data;
	unsigned char	maximum_bitrate;
} SUB_DIGITAL_COPY_CONTROL_DESCR_t;

typedef struct
{
	unsigned char digital_recording_control_data;
	unsigned char maximum_bitrate_flag;
	unsigned char component_control_flag;
	unsigned char copy_control_type;
	unsigned char APS_control_data;
	unsigned char maximum_bitrate;

	SUB_DIGITAL_COPY_CONTROL_DESCR_t* sub_info;
} DIGITAL_COPY_CONTROL_DESCR_t;

typedef struct
{
	unsigned char copy_restriction_mode;
	unsigned char image_constraint_token;
	unsigned char retention_mode;
	unsigned char retention_state;
	unsigned char encryption_mode;
} CONTENT_AVAILABILITY_DESCR_t;

typedef struct
{
	int status;
	int ch;
	int strength;
	int fullseg_id;
	int oneseg_id;
	int total_cnt;
	int all_id[32];
} CUSTOM_SEARCH_INFO_DATA_t;

typedef struct
{
    int channelNumber;
    int networkID;
    int serviceID;
    int serviceSubID;
    int audioIndex;
    int videoIndex;
} AV_INDEX_DATA_t;

typedef struct
{
	int countryCode;
	int channelNumber;
	int mainRowID;
	int subRowID;
} SET_CHANNEL_DATA_t;

typedef struct
{
	int status;
	int percent;
} AUTO_SEARCH_UPDATE_INFO_t;

typedef struct
{
	int iArraySvcRowID[36];
} AUTO_SEARCH_DONE_INFO_t;

typedef struct
{
	int usServiceID;
	int usEventID;
	int usOriginalNetworkID;
	int usTransportStreamID;
} RELAY_EVENT_INFO_t;

typedef struct
{
	int component_group_type;
	int num_of_group;
	int main_component_group_id;
	int sub1_component_group_id;
	int sub2_component_group_id;
} MVTV_UPDATE_INFO_t;

typedef enum event_type
{
	EVENT_NOP = 0,
	EVENT_PREPARED = 1,
	EVENT_SEARCH_COMPLETE = 2,
	EVENT_CHANNEL_UPDATE = 3,
	EVENT_TUNNING_COMPLETE = 4,
	EVENT_SEARCH_PERCENT = 5,
	EVENT_VIDEO_OUTPUT = 6,
	EVENT_RECORDING_COMPLETE		= 8,
	EVENT_SUBTITLE_UPDATE			= 9,
	EVENT_VIDEODEFINITION_UPDATE	= 10,
	EVENT_DEBUGMODE_UPDATE			= 11,
	EVENT_PLAYING_COMPLETE			= 12,
	EVENT_PLAYING_CURRENT_TIME		= 13,
	EVENT_DBINFO_UPDATE				= 14,
	EVENT_DBINFORMATION_UPDATE		= 15,
	EVENT_ERROR						= 16,

	// ISDBT
	EVENT_EPG_UPDATE				= 30,
	EVENT_PARENTLOCK_COMPLETE		= 31,
	EVENT_HANDOVER_CHANNEL			= 32,
	EVENT_EMERGENCY_INFO			= 33,
	EVENT_SMARTCARD_ERROR			= 34,
	EVENT_SMARTCARDINFO_UPDATE		= 35,

	EVENT_AUTOSEARCH_UPDATE			= 36,
	EVENT_AUTOSEARCH_DONE			= 37,

	EVENT_DESC_UPDATE				= 38,
	EVENT_EVENT_RELAY				= 39,
	EVENT_MVTV_UPDATE				= 40,
	EVENT_SERVICEID_CHECK			= 41,
	EVENT_AUTOMESSAGE_UPDATE		= 42,
	EVENT_TRMP_ERROR				= 43,
	EVENT_CUSTOMSEARCH_STATUS		= 44,
	EVENT_SPECIALSERVICE_UPDATE		= 45,
	EVENT_UPDATE_CUSTOM_TUNER		= 46,
	EVENT_EPGSEARCH_COMPLETE		= 47,
	EVENT_AV_INDEX_UPDATE			= 48,
	EVENT_SECTION_UPDATE			= 49,
	EVENT_SEARCH_AND_SETCHANNEL		= 50,
	EVENT_DETECT_SERVICE_NUM_CHANGE = 51,
	EVENT_SECTION_DATA_UPDATE		= 52,
	EVENT_CUSTOM_FILTER_DATA_UPDATE	= 53,
	EVENT_SET_VIDEO_ONOFF           = 54,
	EVENT_SET_AUDIO_ONOFF			= 55,
	EVENT_VIDEO_ERROR				= 56
};

typedef struct
{
	int iMsg;
	int iExt1;
	int iExt2;
	int iObjSize;
	char *pObj;
	int iResult;
} TCC_DXB_ISDBTFSEG_EVENT_NOTIFY_CMD_t;


/******************************************************************************
* globals
******************************************************************************/


/******************************************************************************
* locals
******************************************************************************/


/******************************************************************************
* declarations
******************************************************************************/
void TCC_DXB_ISDBTFSEG_CMD_PROCESS_Exit(Message_Type MsgType);
void TCC_DXB_ISDBTFSEG_CMD_PROCESS_Scan(Message_Type MsgType, int iScanType, int iCountryCode, int iAreaCode, int iChannelNum, int iOptions);
void TCC_DXB_ISDBTFSEG_CMD_PROCESS_ScanStop(Message_Type MsgType);
void TCC_DXB_ISDBTFSEG_CMD_PROCESS_Set(Message_Type MsgType, int mainRowID, int subRowID, int audioIndex, int videoIndex, int audioMode, int ch_index);
void TCC_DXB_ISDBTFSEG_CMD_PROCESS_SetDualChannel(Message_Type MsgType, int index);
void TCC_DXB_ISDBTFSEG_CMD_PROCESS_Show(Message_Type MsgType);


#ifdef __cplusplus
}
#endif

#endif //_TCC_DXB_ISDBTFSEG_CMD_LIST_H__
