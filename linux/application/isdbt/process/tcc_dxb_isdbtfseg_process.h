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
#ifndef __TCC_DXB_ISDBTFSEG_PROCESS_H__
#define __TCC_DXB_ISDBTFSEG_PROCESS_H__

/******************************************************************************
* include
******************************************************************************/
#include <pthread.h>
//#include "sqlite3.h"
//#include "tcc_isdbt_proc.h"
//#include "tcc_isdbt_event.h"
//#include "ISDBT.h"


/******************************************************************************
* define
******************************************************************************/


/******************************************************************************
* typedefs & structure
******************************************************************************/
typedef enum tag_TCC_DXB_ISDBTFSEG_PROCESS_State_E
{
	TCC_DXB_ISDBTFSEG_STATE_EXIT,
	TCC_DXB_ISDBTFSEG_STATE_RUNNING
} TCC_DXB_ISDBTFSEG_PROCESS_State_E;


#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
* declaration variables
******************************************************************************/

/******************************************************************************
* declaration functions
******************************************************************************/
int TCC_DXB_ISDBTFSEG_PROCESS_ProcMessage(void);
int TCC_DXB_ISDBTFSEG_PROCESS_Init(unsigned int uiCountryCode);
void TCC_DXB_ISDBTFSEG_PROCESS_Deinit(void);

void TCC_DXB_ISDBTFSEG_PROCESS_Exit(void);
void TCC_DXB_ISDBTFSEG_PROCESS_Search(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SearchCancel(void);
void TCC_DXB_ISDBTFSEG_PROCESS_SetChannel(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetChannel_withChNum(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_Stop(void);
void TCC_DXB_ISDBTFSEG_PROCESS_Cmd_Show(void);
void TCC_DXB_ISDBTFSEG_PROCESS_SetDualChannel(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_GetCurrentDateTime(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_Prepare(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_Start(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_StartTestMode(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_Release(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_ChannelBackUp(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_ChannelDBRestoration(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetEPG(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_EPGSearch(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_EPGSearchCancel(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_ReleaseSurface(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_UseSurface(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetCountryCode(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetGroup(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetAudio(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetVideo(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetAudioOnOff(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetVideoOnOff(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetAudioVideoSyncMode(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetVolume(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetVolumeLR(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetDisplayPosition(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetCapture(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_PlaySubtitle(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_PlaySuperimpose(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_PlayPng(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetSubtitle(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetSuperimpose(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetAudioDualMono(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetParentalRate(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetArea(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetPreset(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetHandover(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_RequestEPGUpdate(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_ReqSCInfo(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetFreqBand(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetFieldLog(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetPresetMode(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetUserDataRegionID(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetUserDataPrefecturalCode(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetUserDataAreaCode(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetUserDataPostalCode(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetListener(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_IsPlaying(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_GetVideoWidth(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_GetVideoHeight(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_GetSignalStrength(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_CustomRelayStationSearchRequest(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_GetCountryCode(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_GetAudioCnt(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_GetChannelInfo(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_ReqTRMPInfo(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_GetSubtitleLangInfo(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_GetUserData(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetCustomTuner(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_GetCustomTuner(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_GetDigitalCopyControlDescriptor(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_GetContentAvailabilityDescriptor(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SetNotifyDetectSection(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_SearchAndSetChannel(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_Set_Seamless_Switch_Compensation(void *Data);
void TCC_DXB_ISDBTFSEG_PROCESS_GetSeamlessValue(void *Data);

#ifdef __cplusplus
}
#endif

#endif
