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
#ifndef __TCC_DXB_ISDBT_CUI_DISPLAY_INFO_H__
#define __TCC_DXB_ISDBT_CUI_DISPLAY_INFO_H__


/******************************************************************************
* include
******************************************************************************/


/******************************************************************************
* defines
******************************************************************************/


/******************************************************************************
* typedefs & structure
******************************************************************************/


/******************************************************************************
* globals
******************************************************************************/


/******************************************************************************
* locals
******************************************************************************/


/******************************************************************************
* declarations
******************************************************************************/
void TCC_CUI_Display_Show_Channel_DB(void);
void TCC_CUI_Display_GetCurrentDateTime(int *piMJD, int *piHour, int *piMin, int *piSec, int *piPolarity, int *piHourOffset, int *piMinOffset);
void TCC_CUI_Display_IsPlaying(int iRet);
void TCC_CUI_Display_GetVideoWidth(int *piWidth);
void TCC_CUI_Display_GetVideoHeight(int *piHeight);
void TCC_CUI_Display_GetSignalStrength(int *piSQInfo);
void TCC_CUI_Display_GetCountryCode(int iRet);
void TCC_CUI_Display_GetAudioCnt(int iRet);
void TCC_CUI_Display_GetChannelInfo(int iChannelNumber, int iServiceID,
									int *piRemoconID, int *piThreeDigitNumber, int *piServiceType,
									unsigned short *pusChannelName, int *piChannelNameLen,
									int *piTotalAudioPID, int *piTotalVideoPID, int *piTotalSubtitleLang,
									int *piAudioMode, int *piVideoMode,
									int *piAudioLang1, int *piAudioLang2,
									int *piStartMJD, int *piStartHH, int *piStartMM, int *piStartSS,
									int *piDurationHH, int *piDurationMM, int *piDurationSS,
									unsigned short *pusEvtName, int *piEvtNameLen,
									int *piLogoImageWidth, int *piLogoImageHeight, unsigned int *pusLogoImage,
									unsigned short *pusSimpleLogo, int *piSimpleLogoLength,
									int *piMVTVGroupType);
void TCC_CUI_Display_ReqTRMPInfo(unsigned char **ppucInfo, int *piInfoSize);
void TCC_CUI_Display_GetDuration(int iRet);
void TCC_CUI_Display_GetCurrentPosition(int iRet);
void TCC_CUI_Display_GetCurrentReadPosition(int iRet);
void TCC_CUI_Display_GetCurrentWritePosition(int iRet);
void TCC_CUI_Display_GetTotalSize(int iRet);
void TCC_CUI_Display_GetMaxSize(int iRet);
void TCC_CUI_Display_GetFreeSize(int iRet);
void TCC_CUI_Display_GetBlockSize(int iRet);
void TCC_CUI_Display_GetFSType(char *pcFSType);
void TCC_CUI_Display_GetSubtitleLangInfo(int *piSubtitleLangNum, int *piSuperimposeLangNum, unsigned int *puiSubtitleLang1, unsigned int *puiSubtitleLang2, unsigned int *puiSuperimposeLang1, unsigned int *puiSuperimposeLang2);
void TCC_CUI_Display_GetUserData(unsigned int *puiRegionID, unsigned long long *pullPrefecturalCode, unsigned int *puiAreaCode, char *pcPostalCode);
void TCC_CUI_Display_GetDigitalCopyControlDescriptor(unsigned short usServiceID, unsigned char *pucDigital_recording_control_data, unsigned char *pucMaximum_bitrate_flag, unsigned char *pucComponent_control_flag, unsigned char *pucCopy_control_type, unsigned char *pucAPS_control_data, unsigned char *maximum_bitrate, unsigned char *pucIsAvailable);
void TCC_CUI_Display_GetContentAvailabilityDescriptor(unsigned short usServiceID, unsigned char *pucImage_constraint_token, unsigned char *pucRetention_mode, unsigned char *pucRetention_state, unsigned char *pucEncryption_mode, unsigned char *pucIsAvailable);

#endif // __TCC_DXB_ISDBT_CUI_DISPLAY_INFO_H__
