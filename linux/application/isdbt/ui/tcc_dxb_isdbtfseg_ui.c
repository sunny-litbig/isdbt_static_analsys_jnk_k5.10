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
/****************************************************************************
* include
******************************************************************************/
#include <time.h>
#include "tcc_semaphore.h"
#include "tcc_message.h"
#include "tcc_dxb_isdbtfseg_cmd_list.h"
#include "tcc_dxb_isdbtfseg_ui.h"
#include "tcc_dxb_isdbtfseg_ui_process.h"

#define TCC_UI_DXB_ISDBTFSEG_TIMEOUT 1

TCC_DXB_ISDBTFSEG_UI_InfoStruct G_TCC_DXB_ISDBTFSEG_UI_Info;

long long tcc_dxb_isdbtfseg_ui_getsystemtime(Message_Type MsgType)
{
	long long systime;
	struct timespec tspec;

	clock_gettime(CLOCK_MONOTONIC , &tspec);
	systime = (long long) tspec.tv_sec * 1000 + tspec.tv_nsec / 1000000;

	return systime;
}

static TCC_Sema_t gsemaISDBTui;

/******************************************************************************
*
*	UI ------> Process Functions
*
******************************************************************************/
void TCC_UI_DXB_ISDBTFSEG_Exit(Message_Type MsgType)
{
	TCC_DXB_ISDBTFSEG_Exit_CMD(MsgType);
}

void TCC_UI_DXB_ISDBTFSEG_Search(Message_Type MsgType, int Scantype, int Countrycode, int AreadCode, int ChannelNum, int Options )
{
	TCC_DXB_ISDBTFSEG_Search_CMD(MsgType, Scantype, Countrycode, AreadCode, ChannelNum, Options);
}

void TCC_UI_DXB_ISDBTFSEG_SearchCancel(Message_Type MsgType)
{
	TCC_DXB_ISDBTFSEG_SearchCancel_CMD(MsgType);
}

void TCC_UI_DXB_ISDBTFSEG_SetChannel(Message_Type MsgType, int mainRowID, int subRowID, int audioIndex, int videoIndex, int audioMode, int raw_w, int raw_h, int view_w, int view_h, int ch_index)
{
	TCC_DXB_ISDBTFSEG_SetChannel_CMD(MsgType, mainRowID, subRowID, audioIndex, videoIndex, audioMode, raw_w, raw_h, view_w, view_h, ch_index);
}

void TCC_UI_DXB_ISDBTFSEG_SetChannel_withChNum(Message_Type MsgType, int channelNumber, int serviceID_Fseg, int serviceID_1seg, int audioMode, int raw_w, int raw_h, int view_w, int view_h, int ch_index)
{
	TCC_DXB_ISDBTFSEG_SetChannel_CMD_withChNum(MsgType, channelNumber, serviceID_Fseg, serviceID_1seg, audioMode, raw_w, raw_h, view_w, view_h, ch_index);
}

void TCC_UI_DXB_ISDBTFSEG_Stop(Message_Type MsgType)
{
	TCC_DXB_ISDBTFSEG_Stop_CMD(MsgType);
}

void TCC_UI_DXB_ISDBTFSEG_SetDualChannel(Message_Type MsgType, int index)
{
	TCC_DXB_ISDBTFSEG_SetDualChannel_CMD(MsgType, index);
}

int  TCC_UI_DXB_ISDBTFSEG_GetCurrentDateTime(Message_Type MsgType, int *piMJD, int *piHour, int *piMin, int *piSec, int *piPolarity, int *piHourOffset, int *piMinOffset)
{
	int ret;
	TCC_DXB_ISDBTFSEG_GET_CURRENT_DATETIME_PARAM_t *pGetCurrentDateTime_Param;

	pGetCurrentDateTime_Param = (TCC_DXB_ISDBTFSEG_GET_CURRENT_DATETIME_PARAM_t *)TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_GET_CURRENT_DATETIME_PARAM_t));
	if(pGetCurrentDateTime_Param == NULL)
	{
		printf("[%s] pGetCurrentDateTime_Param is NULL Error !!!!!\n", __func__);
		return -1;
	}

	pGetCurrentDateTime_Param->piMJD = piMJD;
	pGetCurrentDateTime_Param->piHour = piHour;
	pGetCurrentDateTime_Param->piMin = piMin;
	pGetCurrentDateTime_Param->piSec = piSec;
	pGetCurrentDateTime_Param->piPolarity = piPolarity;
	pGetCurrentDateTime_Param->piHourOffset = piHourOffset;
	pGetCurrentDateTime_Param->piMinOffset = piMinOffset;

	TCC_DXB_ISDBTFSEG_GetCurrentDateTime_CMD(MsgType, pGetCurrentDateTime_Param);

	if(!TCC_Sema_Down_TimeWait(&gsemaISDBTui, TCC_UI_DXB_ISDBTFSEG_TIMEOUT)){
		ret = -1;
	}
	else {
		ret = pGetCurrentDateTime_Param->iRet;
	}
	TCC_free(pGetCurrentDateTime_Param);
	return ret;
}

int TCC_UI_DXB_ISDBTFSEG_Prepare(Message_Type MsgType, int iSpecificInfo)
{
	TCC_DXB_ISDBTFSEG_Prepare_CMD(MsgType, iSpecificInfo);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_Start(Message_Type MsgType, int iCountryCode)
{
	TCC_DXB_ISDBTFSEG_Start_CMD(MsgType, iCountryCode);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_StartTestMode(Message_Type MsgType, int iIndex)
{
	TCC_DXB_ISDBTFSEG_StartTestMode_CMD(MsgType, iIndex);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_Release(Message_Type MsgType)
{
	TCC_DXB_ISDBTFSEG_Release_CMD(MsgType);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_ChannelBackUp(Message_Type MsgType)
{
	TCC_DXB_ISDBTFSEG_ChannelBackUp_CMD(MsgType);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_ChannelDBRestoration(Message_Type MsgType)
{
	TCC_DXB_ISDBTFSEG_ChannelDBRestoration_CMD(MsgType);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetEPG(Message_Type MsgType, int iOption)
{
	TCC_DXB_ISDBTFSEG_SetEPG_CMD(MsgType, iOption);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_EPGSearch(Message_Type MsgType, int iChannelNum, int iOption)
{
	TCC_DXB_ISDBTFSEG_EPGSearch_CMD(MsgType, iChannelNum, iOption);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_EPGSearchCancel(Message_Type MsgType)
{
	TCC_DXB_ISDBTFSEG_EPGSearchCancel_CMD(MsgType);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_ReleaseSurface(Message_Type MsgType)
{
	TCC_DXB_ISDBTFSEG_ReleaseSurface_CMD(MsgType);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_UseSurface(Message_Type MsgType, int iArg)
{
	TCC_DXB_ISDBTFSEG_UseSurface_CMD(MsgType, iArg);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetCountryCode(Message_Type MsgType, int iCountryCode)
{
	TCC_DXB_ISDBTFSEG_SetCountryCode_CMD(MsgType, iCountryCode);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetGroup(Message_Type MsgType, int iIndex)
{
	TCC_DXB_ISDBTFSEG_SetGroup_CMD(MsgType, iIndex);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetAudio(Message_Type MsgType, int iIndex)
{
	TCC_DXB_ISDBTFSEG_SetAudio_CMD(MsgType, iIndex);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetVideo(Message_Type MsgType, int iIndex)
{
	TCC_DXB_ISDBTFSEG_SetVideo_CMD(MsgType, iIndex);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetVolume(Message_Type MsgType, int iVolume)
{
	TCC_DXB_ISDBTFSEG_SetVolume_CMD(MsgType, iVolume);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetAudioVideoSyncMode(Message_Type MsgType, int iMode)
{
	TCC_DXB_ISDBTFSEG_SetAudioVideoSyncMode_CMD(MsgType, iMode);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetVideoOnOff(Message_Type MsgType, int iOnOff)
{
	TCC_DXB_ISDBTFSEG_SetVideoOnOff_CMD(MsgType, iOnOff);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetVideoOutput(Message_Type MsgType, int iOnOff)
{
	TCC_DXB_ISDBTFSEG_SetVideoOutput_CMD(MsgType, iOnOff);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetAudioOnOff(Message_Type MsgType, int iOnOff)
{
	TCC_DXB_ISDBTFSEG_SetAudioOnOff_CMD(MsgType, iOnOff);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetAudioMuteOnOff(Message_Type MsgType, int iOnOff)
{
	TCC_DXB_ISDBTFSEG_SetAudioMuteOnOff_CMD(MsgType, iOnOff);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetVolumeLR(Message_Type MsgType, int iLeftVolume, int iRightVolume)
{
	TCC_DXB_ISDBTFSEG_SetVolumeLR_CMD(MsgType, iLeftVolume, iRightVolume);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetDisplayPosition(Message_Type MsgType, int iX, int iY, int iWidth, int iHeight, int iRotate)
{
	TCC_DXB_ISDBTFSEG_SetDisplayPosition_CMD(MsgType, iX, iY, iWidth, iHeight, iRotate);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetCapture(Message_Type MsgType, char *pcFilePath)
{
	TCC_DXB_ISDBTFSEG_SetCapture_CMD(MsgType, pcFilePath);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_PlaySubtitle(Message_Type MsgType, int iOnOff)
{
	TCC_DXB_ISDBTFSEG_PlaySubtitle_CMD(MsgType, iOnOff);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_PlaySuperimpose(Message_Type MsgType, int iOnOff)
{
	TCC_DXB_ISDBTFSEG_PlaySuperimpose_CMD(MsgType, iOnOff);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_PlayPng(Message_Type MsgType, int iOnOff)
{
	TCC_DXB_ISDBTFSEG_PlayPng_CMD(MsgType, iOnOff);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetSubtitle(Message_Type MsgType, int iIndex)
{
	TCC_DXB_ISDBTFSEG_SetSubtitle_CMD(MsgType, iIndex);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetSuperimpose(Message_Type MsgType, int iIndex)
{
	TCC_DXB_ISDBTFSEG_SetSuperimpose_CMD(MsgType, iIndex);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetAudioDualMono(Message_Type MsgType, int iAudioSelect)
{
	TCC_DXB_ISDBTFSEG_SetAudioDualMono_CMD(MsgType, iAudioSelect);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetParentalRate(Message_Type MsgType, int iAge)
{
	TCC_DXB_ISDBTFSEG_SetParentalRate_CMD(MsgType, iAge);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetArea(Message_Type MsgType, int iAreaCode)
{
	TCC_DXB_ISDBTFSEG_SetArea_CMD(MsgType, iAreaCode);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetPreset(Message_Type MsgType, int iAreaCode)
{
	TCC_DXB_ISDBTFSEG_SetPreset_CMD(MsgType, iAreaCode);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetHandover(Message_Type MsgType, int iCountryCode)
{
	TCC_DXB_ISDBTFSEG_SetHandover_CMD(MsgType, iCountryCode);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_RequestEPGUpdate(Message_Type MsgType, int iDayOffset)
{
	TCC_DXB_ISDBTFSEG_RequestEPGUpdate_CMD(MsgType, iDayOffset);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_ReqSCInfo(Message_Type MsgType, int iArg)
{
	TCC_DXB_ISDBTFSEG_ReqSCInfo_CMD(MsgType, iArg);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetFreqBand(Message_Type MsgType, int iFreqBand)
{
	TCC_DXB_ISDBTFSEG_SetFreqBand_CMD(MsgType, iFreqBand);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetFieldLog(Message_Type MsgType, char *pcPath, int iOnOff)
{
	TCC_DXB_ISDBTFSEG_SetFieldLog_CMD(MsgType, pcPath, iOnOff);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetPresetMode(Message_Type MsgType, int iPresetMode)
{
	TCC_DXB_ISDBTFSEG_SetPresetMode_CMD(MsgType, iPresetMode);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetUserDataRegionID(Message_Type MsgType, int iRegionID)
{
	TCC_DXB_ISDBTFSEG_SetUserDataRegionID_CMD(MsgType, iRegionID);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetUserDataPrefecturalCode(Message_Type MsgType, int iPrefecturalBitOrder)
{
	TCC_DXB_ISDBTFSEG_SetUserDataPrefecturalCode_CMD(MsgType, iPrefecturalBitOrder);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetUserDataAreaCode(Message_Type MsgType, int iAreaCode)
{
	TCC_DXB_ISDBTFSEG_SetUserDataAreaCode_CMD(MsgType, iAreaCode);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetUserDataPostalCode(Message_Type MsgType, char *pcPostalCode)
{
	TCC_DXB_ISDBTFSEG_SetUserDataPostalCode_CMD(MsgType, pcPostalCode);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetListener(Message_Type MsgType, int (*pfListener)(int, int, int, void *))
{
	TCC_DXB_ISDBTFSEG_SetListener_CMD(MsgType, pfListener);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_IsPlaying(Message_Type MsgType)
{
	int ret = 0;
	TCC_DXB_ISDBTFSEG_RETURN_PARAM_t *pIsPlaying_Param = NULL;

	pIsPlaying_Param = (TCC_DXB_ISDBTFSEG_RETURN_PARAM_t *)TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_RETURN_PARAM_t));
	if(pIsPlaying_Param == NULL)
	{
		printf("[%s] pIsPlaying_Param is NULL Error !!!!!\n", __func__);
		return -1;
	}
	memset( pIsPlaying_Param, NULL, sizeof(TCC_DXB_ISDBTFSEG_RETURN_PARAM_t) );

	TCC_DXB_ISDBTFSEG_IsPlaying_CMD(MsgType, pIsPlaying_Param);

	if(!TCC_Sema_Down_TimeWait(&gsemaISDBTui, TCC_UI_DXB_ISDBTFSEG_TIMEOUT)) {

		ret = -1;
	}
	else {
		ret = pIsPlaying_Param->iRet;
	}
	TCC_free(pIsPlaying_Param);
	return ret;
}

int TCC_UI_DXB_ISDBTFSEG_GetVideoWidth(Message_Type MsgType, int *piWidth)
{
	int ret = 0;
	TCC_DXB_ISDBTFSEG_GET_VIDEO_WIDTH_PARAM_t *pGetVideoWidth_Param = NULL;

	pGetVideoWidth_Param = (TCC_DXB_ISDBTFSEG_GET_VIDEO_WIDTH_PARAM_t *)TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_GET_VIDEO_WIDTH_PARAM_t));
	if(pGetVideoWidth_Param == NULL)
	{
		printf("[%s] pGetVideoWidth_Param is NULL Error !!!!!\n", __func__);
		return -1;
	}
	memset( pGetVideoWidth_Param, NULL, sizeof(TCC_DXB_ISDBTFSEG_GET_VIDEO_WIDTH_PARAM_t) );

	pGetVideoWidth_Param->piWidth = piWidth;

	TCC_DXB_ISDBTFSEG_GetVideoWidth_CMD(MsgType, pGetVideoWidth_Param);

	if(!TCC_Sema_Down_TimeWait(&gsemaISDBTui, TCC_UI_DXB_ISDBTFSEG_TIMEOUT)) {

		ret = -1;
	}
	else {
		ret = pGetVideoWidth_Param->iRet;
	}
	TCC_free(pGetVideoWidth_Param);
	return ret;
}

int TCC_UI_DXB_ISDBTFSEG_GetVideoHeight(Message_Type MsgType, int *piHeight)
{
	int ret = 0;
	TCC_DXB_ISDBTFSEG_GET_VIDEO_HEIGHT_PARAM_t *pGetVideoHeight_Param = NULL;

	pGetVideoHeight_Param = (TCC_DXB_ISDBTFSEG_GET_VIDEO_HEIGHT_PARAM_t *)TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_GET_VIDEO_HEIGHT_PARAM_t));
	if(pGetVideoHeight_Param == NULL)
	{
		printf("[%s] pGetVideoHeight_Param is NULL Error !!!!!\n", __func__);
		return -1;
	}
	memset( pGetVideoHeight_Param, NULL, sizeof(TCC_DXB_ISDBTFSEG_GET_VIDEO_HEIGHT_PARAM_t) );

	pGetVideoHeight_Param->piHeight = piHeight;

	TCC_DXB_ISDBTFSEG_GetVideoHeight_CMD(MsgType, pGetVideoHeight_Param);

	if(!TCC_Sema_Down_TimeWait(&gsemaISDBTui, TCC_UI_DXB_ISDBTFSEG_TIMEOUT)) {

		ret = -1;
	}
	else {
		ret = pGetVideoHeight_Param->iRet;
	}
	TCC_free(pGetVideoHeight_Param);
	return ret;
}

int TCC_UI_DXB_ISDBTFSEG_GetSignalStrength(Message_Type MsgType, int *piSQInfo)
{
	int ret = 0;
	TCC_DXB_ISDBTFSEG_GET_SIGNAL_STRENGTH_PARAM_t *pGetSignalStrength_Param = NULL;

	pGetSignalStrength_Param = (TCC_DXB_ISDBTFSEG_GET_SIGNAL_STRENGTH_PARAM_t *)TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_GET_SIGNAL_STRENGTH_PARAM_t));
	if(pGetSignalStrength_Param == NULL)
	{
		printf("[%s] pGetSignalStrength_Param is NULL Error !!!!!\n", __func__);
		return -1;
	}
	memset( pGetSignalStrength_Param, NULL, sizeof(TCC_DXB_ISDBTFSEG_GET_SIGNAL_STRENGTH_PARAM_t) );

	pGetSignalStrength_Param->piSQInfo = piSQInfo;

	TCC_DXB_ISDBTFSEG_GetSignalStrength_CMD(MsgType, pGetSignalStrength_Param);

	if(!TCC_Sema_Down_TimeWait(&gsemaISDBTui, TCC_UI_DXB_ISDBTFSEG_TIMEOUT)) {

		ret = -1;
	}
	else {
		ret = pGetSignalStrength_Param->iRet;
	}
	TCC_free(pGetSignalStrength_Param);
	return ret;
}

int TCC_UI_DXB_ISDBTFSEG_CustomRelayStationSearchRequest(Message_Type MsgType, int iSearchKind, int *piListChannel, int *piListTSID, int iRepeat)
{
	TCC_DXB_ISDBTFSEG_CustomRelayStationSearchRequest_CMD(MsgType, iSearchKind, piListChannel, piListTSID, iRepeat);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_GetCountryCode(Message_Type MsgType)
{
	int ret = 0;
	TCC_DXB_ISDBTFSEG_RETURN_PARAM_t *pGetCountryCode_Param = NULL;

	pGetCountryCode_Param = (TCC_DXB_ISDBTFSEG_RETURN_PARAM_t *)TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_RETURN_PARAM_t));
	if(pGetCountryCode_Param == NULL)
	{
		printf("[%s] pGetCountryCode_Param is NULL Error !!!!!\n", __func__);
		return -1;
	}
	memset( pGetCountryCode_Param, NULL, sizeof(TCC_DXB_ISDBTFSEG_RETURN_PARAM_t) );

	TCC_DXB_ISDBTFSEG_GetCountryCode_CMD(MsgType, pGetCountryCode_Param);

	if(!TCC_Sema_Down_TimeWait(&gsemaISDBTui, TCC_UI_DXB_ISDBTFSEG_TIMEOUT)) {

		ret = -1;
	}
	else {
		ret = pGetCountryCode_Param->iRet;
	}
	TCC_free(pGetCountryCode_Param);
	return ret;
}

int TCC_UI_DXB_ISDBTFSEG_GetAudioCnt(Message_Type MsgType)
{
	int ret = 0;
	TCC_DXB_ISDBTFSEG_RETURN_PARAM_t *pGetAudioCnt_Param = NULL;

	pGetAudioCnt_Param = (TCC_DXB_ISDBTFSEG_RETURN_PARAM_t *)TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_RETURN_PARAM_t));
	if(pGetAudioCnt_Param == NULL)
	{
		printf("[%s] pGetAudioCnt_Param is NULL Error !!!!!\n", __func__);
		return -1;
	}
	memset( pGetAudioCnt_Param, NULL, sizeof(TCC_DXB_ISDBTFSEG_RETURN_PARAM_t) );

	TCC_DXB_ISDBTFSEG_GetAudioCnt_CMD(MsgType, pGetAudioCnt_Param);

	if(!TCC_Sema_Down_TimeWait(&gsemaISDBTui, TCC_UI_DXB_ISDBTFSEG_TIMEOUT)) {

		ret = -1;
	}
	else {
		ret = pGetAudioCnt_Param->iRet;
	}
	TCC_free(pGetAudioCnt_Param);
	return ret;
}

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
											int *piMVTVGroupType)
{
	int ret = 0;
	TCC_DXB_ISDBTFSEG_GET_CHANNEL_INFO_PARAM_t *pGetChannelInfo_Param = NULL;

	pGetChannelInfo_Param = (TCC_DXB_ISDBTFSEG_GET_CHANNEL_INFO_PARAM_t *)TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_GET_CHANNEL_INFO_PARAM_t));
	if(pGetChannelInfo_Param == NULL)
	{
		printf("[%s] pGetChannelInfo_Param is NULL Error !!!!!\n", __func__);
		return -1;
	}
	memset( pGetChannelInfo_Param, NULL, sizeof(TCC_DXB_ISDBTFSEG_GET_CHANNEL_INFO_PARAM_t) );

	pGetChannelInfo_Param->piRemoconID = piRemoconID;
	pGetChannelInfo_Param->piThreeDigitNumber = piThreeDigitNumber;
	pGetChannelInfo_Param->piServiceType = piServiceType;
	pGetChannelInfo_Param->pusChannelName = pusChannelName;
	pGetChannelInfo_Param->piChannelNameLen = piChannelNameLen;
	pGetChannelInfo_Param->piTotalAudioPID = piTotalAudioPID;
	pGetChannelInfo_Param->piTotalVideoPID = piTotalVideoPID;
	pGetChannelInfo_Param->piTotalSubtitleLang = piTotalSubtitleLang;
	pGetChannelInfo_Param->piAudioMode = piAudioMode;
	pGetChannelInfo_Param->piVideoMode = piVideoMode;
	pGetChannelInfo_Param->piAudioLang1 = piAudioLang1;
	pGetChannelInfo_Param->piAudioLang2 = piAudioLang2;
	pGetChannelInfo_Param->piAudioComponentTag = piAudioComponentTag;
	pGetChannelInfo_Param->piVideoComponentTag = piVideoComponentTag;
	pGetChannelInfo_Param->piStartMJD = piStartMJD;
	pGetChannelInfo_Param->piStartHH = piStartHH;
	pGetChannelInfo_Param->piStartMM = piStartMM;
	pGetChannelInfo_Param->piStartSS = piStartSS;
	pGetChannelInfo_Param->piDurationHH = piDurationHH;
	pGetChannelInfo_Param->piDurationMM = piDurationMM;
	pGetChannelInfo_Param->piDurationSS = piDurationSS;
	pGetChannelInfo_Param->pusEvtName = pusEvtName;
	pGetChannelInfo_Param->piEvtNameLen = piEvtNameLen;
	pGetChannelInfo_Param->piLogoImageWidth = piLogoImageWidth;
	pGetChannelInfo_Param->piLogoImageHeight = piLogoImageHeight;
	pGetChannelInfo_Param->pusLogoImage = pusLogoImage;
	pGetChannelInfo_Param->pusSimpleLogo = pusSimpleLogo;
	pGetChannelInfo_Param->piSimpleLogoLength = piSimpleLogoLength;
	pGetChannelInfo_Param->piMVTVGroupType = piMVTVGroupType;

	TCC_DXB_ISDBTFSEG_GetChannelInfo_CMD(MsgType, iChannelNumber, iServiceID, pGetChannelInfo_Param);

	if(!TCC_Sema_Down_TimeWait(&gsemaISDBTui, TCC_UI_DXB_ISDBTFSEG_TIMEOUT)) {

		ret = -1;
	}
	else {
		ret = pGetChannelInfo_Param->iRet;
	}
	TCC_free(pGetChannelInfo_Param);
	return ret;
}

int TCC_UI_DXB_ISDBTFSEG_ReqTRMPInfo(Message_Type MsgType, unsigned char **ppucInfo, int *piInfoSize)
{
	int ret = 0;
	TCC_DXB_ISDBTFSEG_REQ_TRMP_INFO_PARAM_t *pReqTRMPInfo_Param = NULL;

	pReqTRMPInfo_Param = (TCC_DXB_ISDBTFSEG_REQ_TRMP_INFO_PARAM_t *)TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_REQ_TRMP_INFO_PARAM_t));
	if(pReqTRMPInfo_Param == NULL)
	{
		printf("[%s] pReqTRMPInfo_Param is NULL Error !!!!!\n", __func__);
		return -1;
	}
	memset( pReqTRMPInfo_Param, NULL, sizeof(TCC_DXB_ISDBTFSEG_REQ_TRMP_INFO_PARAM_t) );

	pReqTRMPInfo_Param->ppucInfo = ppucInfo;
	pReqTRMPInfo_Param->piInfoSize = piInfoSize;

	TCC_DXB_ISDBTFSEG_ReqTRMPInfo_CMD(MsgType, pReqTRMPInfo_Param);

	if(!TCC_Sema_Down_TimeWait(&gsemaISDBTui, TCC_UI_DXB_ISDBTFSEG_TIMEOUT)) {

		ret = -1;
	}
	else {
		ret = pReqTRMPInfo_Param->iRet;
	}
	TCC_free(pReqTRMPInfo_Param);
	return ret;
}

int TCC_UI_DXB_ISDBTFSEG_GetSubtitleLangInfo(Message_Type MsgType, int *piSubtitleLangNum, int *piSuperimposeLangNum, unsigned int *puiSubtitleLang1, unsigned int *puiSubtitleLang2, unsigned int *puiSuperimposeLang1, unsigned int *puiSuperimposeLang2)
{
	int ret = 0;
	TCC_DXB_ISDBTFSEG_GET_SUBTITLE_LANG_INFO_PARAM_t *pGetSubtitleLangInfo_Param = NULL;

	pGetSubtitleLangInfo_Param = (TCC_DXB_ISDBTFSEG_GET_SUBTITLE_LANG_INFO_PARAM_t *)TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_GET_SUBTITLE_LANG_INFO_PARAM_t));
	if(pGetSubtitleLangInfo_Param == NULL)
	{
		printf("[%s] pGetSubtitleLangInfo_Param is NULL Error !!!!!\n", __func__);
		return -1;
	}
	memset( pGetSubtitleLangInfo_Param, NULL, sizeof(TCC_DXB_ISDBTFSEG_GET_SUBTITLE_LANG_INFO_PARAM_t) );

	pGetSubtitleLangInfo_Param->piSubtitleLangNum = piSubtitleLangNum;
	pGetSubtitleLangInfo_Param->piSuperimposeLangNum = piSuperimposeLangNum;
	pGetSubtitleLangInfo_Param->puiSubtitleLang1 = puiSubtitleLang1;
	pGetSubtitleLangInfo_Param->puiSubtitleLang2 = puiSubtitleLang2;
	pGetSubtitleLangInfo_Param->puiSuperimposeLang1 = puiSuperimposeLang1;
	pGetSubtitleLangInfo_Param->puiSuperimposeLang2 = puiSuperimposeLang2;

	TCC_DXB_ISDBTFSEG_GetSubtitleLangInfo_CMD(MsgType, pGetSubtitleLangInfo_Param);

	if(!TCC_Sema_Down_TimeWait(&gsemaISDBTui, TCC_UI_DXB_ISDBTFSEG_TIMEOUT)) {

		ret = -1;
	}
	else {
		ret = pGetSubtitleLangInfo_Param->iRet;
	}
	TCC_free(pGetSubtitleLangInfo_Param);
	return ret;
}

int TCC_UI_DXB_ISDBTFSEG_GetUserData(Message_Type MsgType, unsigned int *puiRegionID, unsigned long long *pullPrefecturalCode, unsigned int *puiAreaCode, char *pcPostalCode)
{
	int ret = 0;
	TCC_DXB_ISDBTFSEG_GET_USERDATA_PARAM_t *pGetUserData_Param = NULL;

	pGetUserData_Param = (TCC_DXB_ISDBTFSEG_GET_USERDATA_PARAM_t *)TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_GET_USERDATA_PARAM_t));
	if(pGetUserData_Param == NULL)
	{
		printf("[%s] pGetUserData_Param is NULL Error !!!!!\n", __func__);
		return -1;
	}
	memset( pGetUserData_Param, NULL, sizeof(TCC_DXB_ISDBTFSEG_GET_USERDATA_PARAM_t) );

	pGetUserData_Param->puiRegionID = puiRegionID;
	pGetUserData_Param->pullPrefecturalCode = pullPrefecturalCode;
	pGetUserData_Param->puiAreaCode = puiAreaCode;
	pGetUserData_Param->pcPostalCode = pcPostalCode;

	TCC_DXB_ISDBTFSEG_GetUserData_CMD(MsgType, pGetUserData_Param);

	if(!TCC_Sema_Down_TimeWait(&gsemaISDBTui, TCC_UI_DXB_ISDBTFSEG_TIMEOUT)) {

		ret = -1;
	}
	else {
		ret = pGetUserData_Param->iRet;
	}
	TCC_free(pGetUserData_Param);
	return ret;
}

int TCC_UI_DXB_ISDBTFSEG_SetCustomTuner(Message_Type MsgType, int iSize, void *pvArg)
{
	TCC_DXB_ISDBTFSEG_SetCustomTuner_CMD(MsgType, iSize, pvArg);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_GetCustomTuner(Message_Type MsgType, int *piSize, void *pvArg)
{
	int ret = 0;
	TCC_DXB_ISDBTFSEG_GET_CUSTOM_TUNER_PARAM_t *pGetCustomTuner_Param = NULL;

	pGetCustomTuner_Param = (TCC_DXB_ISDBTFSEG_GET_CUSTOM_TUNER_PARAM_t *)TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_GET_CUSTOM_TUNER_PARAM_t));
	if(pGetCustomTuner_Param == NULL)
	{
		printf("[%s] pGetCustomTuner_Param is NULL Error !!!!!\n", __func__);
		return -1;
	}
	memset( pGetCustomTuner_Param, NULL, sizeof(TCC_DXB_ISDBTFSEG_GET_CUSTOM_TUNER_PARAM_t) );

	pGetCustomTuner_Param->piSize = piSize;
	pGetCustomTuner_Param->pvArg = pvArg;

	TCC_DXB_ISDBTFSEG_GetCustomTuner_CMD(MsgType, pGetCustomTuner_Param);

	if(!TCC_Sema_Down_TimeWait(&gsemaISDBTui, TCC_UI_DXB_ISDBTFSEG_TIMEOUT)) {

		ret = -1;
	}
	else {
		ret = pGetCustomTuner_Param->iRet;
	}
	TCC_free(pGetCustomTuner_Param);
	return ret;
}

int TCC_UI_DXB_ISDBTFSEG_GetDigitalCopyControlDescriptor(Message_Type MsgType, unsigned short usServiceID)
{
	int ret = 0;
	TCC_DXB_ISDBTFSEG_GET_DCCD_PARAM_t *pGetDigitalCopyControlDescriptor_Param = NULL;

	pGetDigitalCopyControlDescriptor_Param = (TCC_DXB_ISDBTFSEG_GET_DCCD_PARAM_t *)TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_GET_DCCD_PARAM_t));
	if(pGetDigitalCopyControlDescriptor_Param == NULL)
	{
		printf("[%s] pGetDigitalCopyControlDescriptor_Param is NULL Error !!!!!\n", __func__);
		return -1;
	}
	memset( pGetDigitalCopyControlDescriptor_Param, NULL, sizeof(TCC_DXB_ISDBTFSEG_GET_DCCD_PARAM_t) );

	TCC_DXB_ISDBTFSEG_GetDigitalCopyControlDescriptor_CMD(MsgType, usServiceID);

	if(!TCC_Sema_Down_TimeWait(&gsemaISDBTui, TCC_UI_DXB_ISDBTFSEG_TIMEOUT)) {

		ret = -1;
	}
	else {
		ret = pGetDigitalCopyControlDescriptor_Param->iRet;
	}
	TCC_free(pGetDigitalCopyControlDescriptor_Param);
	return ret;
}

int TCC_UI_DXB_ISDBTFSEG_GetContentAvailabilityDescriptor(Message_Type MsgType, unsigned short usServiceID, unsigned char *pucCopy_restriction_mode, unsigned char *pucImage_constraint_token, unsigned char *pucRetention_mode, unsigned char *pucRetention_state, unsigned char *pucEncryption_mode)
{
	int ret = 0;
	TCC_DXB_ISDBTFSEG_GET_CAD_PARAM_t *pGetContentAvailabilityDescriptor_Param = NULL;

	pGetContentAvailabilityDescriptor_Param = (TCC_DXB_ISDBTFSEG_GET_CAD_PARAM_t *)TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_GET_CAD_PARAM_t));
	if(pGetContentAvailabilityDescriptor_Param == NULL)
	{
		printf("[%s] pGetContentAvailabilityDescriptor_Param is NULL Error !!!!!\n", __func__);
		return -1;
	}
	memset( pGetContentAvailabilityDescriptor_Param, NULL, sizeof(TCC_DXB_ISDBTFSEG_GET_CAD_PARAM_t) );

	pGetContentAvailabilityDescriptor_Param->pucCopy_restriction_mode = pucCopy_restriction_mode;
	pGetContentAvailabilityDescriptor_Param->pucImage_constraint_token = pucImage_constraint_token;
	pGetContentAvailabilityDescriptor_Param->pucRetention_mode = pucRetention_mode;
	pGetContentAvailabilityDescriptor_Param->pucRetention_state = pucRetention_state;
	pGetContentAvailabilityDescriptor_Param->pucEncryption_mode = pucEncryption_mode;

	TCC_DXB_ISDBTFSEG_GetContentAvailabilityDescriptor_CMD(MsgType, usServiceID, pGetContentAvailabilityDescriptor_Param);

	if(!TCC_Sema_Down_TimeWait(&gsemaISDBTui, TCC_UI_DXB_ISDBTFSEG_TIMEOUT)) {
		ret = -1;
	}
	else {
		ret = pGetContentAvailabilityDescriptor_Param->iRet;
	}
	TCC_free(pGetContentAvailabilityDescriptor_Param);
	return ret;
}

int TCC_UI_DXB_ISDBTFSEG_SetNotifyDetectSection(Message_Type MsgType, int iSectionIDs)
{
	TCC_DXB_ISDBTFSEG_SetNotifyDetectSection_CMD(MsgType, iSectionIDs);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SetDeviceKeyUpdateFunction(Message_Type MsgType, unsigned char *(*pfUpdateDeviceKey)(unsigned char , unsigned int))
{
	TCC_DXB_ISDBTFSEG_SetDeviceKeyUpdateFunction_CMD(MsgType, pfUpdateDeviceKey);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_SearchAndSetChannel(Message_Type MsgType, int iCountryCode, int iChannelNum, int iTsid, int iOptions, int iAudioIndex, int iVideoIndex, int iAudioMode, int iRaw_w, int iRaw_h, int iView_w, int iView_h, int iCh_index)
{
		TCC_DXB_ISDBTFSEG_SearchAndSetChannel_CMD(MsgType, iCountryCode, iChannelNum, iTsid, iOptions, iAudioIndex, iVideoIndex, iAudioMode, iRaw_w, iRaw_h, iView_w, iView_h, iCh_index);
		return 0;
}

int TCC_UI_DXB_ISDBTFSEG_GetVideoState(Message_Type MsgType)
{
	int ret = 0;
	TCC_DXB_ISDBTFSEG_RETURN_PARAM_t *pGetVideoState_Param = NULL;

	pGetVideoState_Param = (TCC_DXB_ISDBTFSEG_RETURN_PARAM_t *)TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_RETURN_PARAM_t));
	if(pGetVideoState_Param == NULL)
	{
		printf("[%s] pGetVideoState_Param is NULL Error !!!!!\n", __func__);
		return -1;
	}
	memset( pGetVideoState_Param, NULL, sizeof(TCC_DXB_ISDBTFSEG_RETURN_PARAM_t) );

	TCC_DXB_ISDBTFSEG_GetVideoState_CMD(MsgType, pGetVideoState_Param);

	if(!TCC_Sema_Down_TimeWait(&gsemaISDBTui, TCC_UI_DXB_ISDBTFSEG_TIMEOUT)) {
		ret = -1;
	}
	else {
		ret = pGetVideoState_Param->iRet;
	}
	TCC_free(pGetVideoState_Param);
	return ret;
}

int TCC_UI_DXB_ISDBTFSEG_GetSearchState(Message_Type MsgType)
{
	int ret = 0;
	TCC_DXB_ISDBTFSEG_RETURN_PARAM_t *pGetSearchState_Param = NULL;

	pGetSearchState_Param = (TCC_DXB_ISDBTFSEG_RETURN_PARAM_t *)TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_RETURN_PARAM_t));
	if(pGetSearchState_Param == NULL)
	{
		printf("[%s] pGetSearchState_Param is NULL Error !!!!!\n", __func__);
		return -1;
	}
	memset( pGetSearchState_Param, NULL, sizeof(TCC_DXB_ISDBTFSEG_RETURN_PARAM_t) );

	TCC_DXB_ISDBTFSEG_GetSearchState_CMD(MsgType, pGetSearchState_Param);

	if(!TCC_Sema_Down_TimeWait(&gsemaISDBTui, TCC_UI_DXB_ISDBTFSEG_TIMEOUT)) {
		ret = -1;
	}
	else {
		ret = pGetSearchState_Param->iRet;
	}
	TCC_free(pGetSearchState_Param);
	return ret;
}

int TCC_UI_DXB_ISDBTFSEG_SetDataServiceStart(Message_Type MsgType)
{
	TCC_DXB_ISDBTFSEG_SetDataServiceStart_CMD(MsgType);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_CustomFilterStart(Message_Type MsgType, int iPID, int iTableID)
{
	TCC_DXB_ISDBTFSEG_CustomFilterStart_CMD(MsgType, iPID, iTableID);
	return 0;
}

int TCC_UI_DXB_ISDBTFSEG_CustomFilterStop(Message_Type MsgType, int iPID, int iTableID)
{
	TCC_DXB_ISDBTFSEG_CustomFilterStop_CMD(MsgType, iPID, iTableID);
	return 0;
}

void TCC_UI_DXB_ISDBTFSEG_EWS_start(Message_Type MsgType, int mainRowID, int subRowID, int audioIndex, int videoIndex, int audioMode, int raw_w, int raw_h, int view_w, int view_h, int ch_index)
{
	TCC_DXB_ISDBTFSEG_EWS_start_CMD(MsgType, mainRowID, subRowID, audioIndex, videoIndex, audioMode, raw_w, raw_h, view_w, view_h, ch_index);
}

void TCC_UI_DXB_ISDBTFSEG_EWS_start_withChNum(Message_Type MsgType, int channelNumber, int serviceID_Fseg, int serviceID_1seg, int audioMode, int raw_w, int raw_h, int view_w, int view_h, int ch_index)
{
	TCC_DXB_ISDBTFSEG_EWS_start_CMD_withChNum(MsgType, channelNumber, serviceID_Fseg, serviceID_1seg, audioMode, raw_w, raw_h, view_w, view_h, ch_index);
}

void TCC_UI_DXB_ISDBTFSEG_EWS_clear(Message_Type MsgType)
{
	TCC_DXB_ISDBTFSEG_EWS_clear_CMD(MsgType);
}

void TCC_UI_DXB_ISDBTFSEG_Set_Seamless_Switch_Compensation(Message_Type MsgType, int onoff, int interval, int strength, int ntimes, int range, int gapadjust, int gapadjust2, int multiplier)
{
	TCC_DXB_ISDBTFSEG_Set_Seamless_Switch_Compensation_CMD(MsgType, onoff, interval, strength, ntimes, range, gapadjust, gapadjust2, multiplier);
}

void TCC_UI_DXB_ISDBTFSEG_GetSTC(Message_Type MsgType, unsigned int *hi_data, unsigned *lo_data)
{
	int ret = 0;

	TCC_DXB_ISDBTFSEG_GetSTC_CMD(MsgType, hi_data, lo_data);

	if(!TCC_Sema_Down_TimeWait(&gsemaISDBTui, TCC_UI_DXB_ISDBTFSEG_TIMEOUT)) {

		ret = -1;
	}
	else {
		ret = 0;
	}
}

void TCC_UI_DXB_ISDBTFSEG_GetServiceTime(Message_Type MsgType, unsigned int *year, unsigned int *month, unsigned int *day, unsigned int *hour, unsigned int *min, unsigned int *sec)
{
	int ret = 0;

	TCC_DXB_ISDBTFSEG_GetServiceTime_CMD(MsgType, year, month, day, hour, min, sec);

	if(!TCC_Sema_Down_TimeWait(&gsemaISDBTui, TCC_UI_DXB_ISDBTFSEG_TIMEOUT)) {

		ret = -1;
	}
	else {
		ret = 0;
	}
}

void TCC_UI_DXB_ISDBTFSEG_GetContentID(Message_Type MsgType, int *content_id, int *associated_contents_flag)
{
	int ret = 0;

	TCC_DXB_ISDBTFSEG_GetContentID_CMD(MsgType, content_id, associated_contents_flag);

	if(!TCC_Sema_Down_TimeWait(&gsemaISDBTui, TCC_UI_DXB_ISDBTFSEG_TIMEOUT)) {

		ret = -1;
	}
	else {
		ret = 0;
	}
}

void TCC_UI_DXB_ISDBTFSEG_GetSeamlessValue(Message_Type MsgType, int *state, int *cval, int *pval)
{
	int ret = 0;

	TCC_DXB_ISDBTFSEG_GetSeamlessValue_CMD(MsgType, state, cval, pval);

	if(!TCC_Sema_Down_TimeWait(&gsemaISDBTui, TCC_UI_DXB_ISDBTFSEG_TIMEOUT)) {

		ret = -1;
	}
	else {
		ret = 0;
	}
}

void TCC_UI_DXB_ISDBTFSEG_CheckExistComponentTagInPMT(Message_Type MsgType, int target_component_tag)
{
	int ret = 0;

	TCC_DXB_ISDBTFSEG_CheckExistComponentTagInPMT_CMD(MsgType, target_component_tag);

	if(!TCC_Sema_Down_TimeWait(&gsemaISDBTui, TCC_UI_DXB_ISDBTFSEG_TIMEOUT)) {

		ret = -1;
	}
	else {
		ret = 0;
	}
}

void TCC_UI_DXB_ISDBTFSEG_SetVideoByComponentTag(Message_Type MsgType, int component_tag)
{
	TCC_DXB_ISDBTFSEG_SetVideoByComponentTag_CMD(MsgType, component_tag);
}

void TCC_UI_DXB_ISDBTFSEG_SetAudioByComponentTag(Message_Type MsgType, int component_tag)
{
	TCC_DXB_ISDBTFSEG_SetAudioByComponentTag_CMD(MsgType, component_tag);
}

void TCC_DXB_ISDBTFSEG_UI_Show(Message_Type MsgType)
{
	TCC_DXB_ISDBTFSEG_CMD_PROCESS_Show(MsgType);
}

/*
void TCC_UI_DXB_ISDBTFSEG_EventNotify(Message_Type MsgType, void *Data)
{
	TCC_DXB_ISDBTFSEG_EVENT_NOTIFY_CMD_t *pCmd = (TCC_DXB_ISDBTFSEG_EVENT_NOTIFY_CMD_t *)Data;

	if( pCmd->iObjSize > 0)
	{
		pCmd->pObj = ((char*)Data + sizeof(TCC_DXB_ISDBTFSEG_EVENT_NOTIFY_CMD_t));
	}

	TCC_CUI_Display_EventNotify(pCmd->iMsg, pCmd->iExt1, pCmd->iExt2, pCmd->iObjSize, pCmd->pObj);
}
*/
