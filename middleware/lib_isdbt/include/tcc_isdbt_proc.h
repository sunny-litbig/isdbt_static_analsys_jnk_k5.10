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
#ifndef _TCC_ISDBT_PROC_H_
#define	_TCC_ISDBT_PROC_H_

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************************************
* defines
******************************************************************************/
#define TCCDXBPROC_SUCCESS					(0)
#define TCCDXBPROC_ERROR					(-1)
#define TCCDXBPROC_ERROR_INVALID_STATE		(-2)
#define TCCDXBPROC_ERROR_INVALID_PARAM		(-3)

#define TCCDXBPROC_VIDEO_MAX_NUM			(4)
#define TCCDXBPROC_AUDIO_MAX_NUM			(4)
#define TCCDXBPROC_COMPONENT_TAG_MAX_NUM	(32)

#define TCCDXBPROC_SERVICE_TYPE_12SEG	(0x1)
#define TCCDXBPROC_SERVICE_TYPE_1SEG	(0xC0)

/******************************************************************************
* typedefs & structure
******************************************************************************/
typedef enum
{
	INIT_SCAN,
	RE_SCAN,
	REGION_SCAN,
	PRESET_SCAN,
	MANUAL_SCAN,
	AUTOSEARCH,
	CUSTOM_SCAN,
	INFORMAL_SCAN,	// For only searchAndSetChannel
	EPG_SCAN,
	MAX_SCAN
}TCCDXBSCAN_TYPE;

typedef enum
{
	TCCDXBPROC_JAPAN =0,
	TCCDXBPROC_BRAZIL

}TCCDXBPROC_COUNTRYCODE;

typedef enum
{
	TCCDXBPROC_MAIN,
	TCCDXBPROC_SUB

}TCCDXBPROC_DUALDECODE;

typedef enum
{
	TCCDXBPROC_AIRPLAY_MSG				= 0x00000000,
	TCCDXBPROC_AIRPLAY_MAIN				= 0x00000001,
	TCCDXBPROC_AIRPLAY_SUB				= 0x00000002,
	TCCDXBPROC_AIRPLAY_MULTVIEW			= 0x00000010,
	TCCDXBPROC_AIRPLAY_SPECIAL			= 0x00000020,
	TCCDXBPROC_AIRPLAY_EWS				= 0x00000040,
	TCCDXBPROC_AIRPLAY_PMTUPDATE		= 0x00000080,
	TCCDXBPROC_AIRPLAY_INFORMAL_SCAN	= 0x00001000,
	TCCDXBPROC_EWSMODE_MAIN				= 0x00002000,
	TCCDXBPROC_EWSMODE_SUB				= 0x00004000

} TCCDXBPROC_AIRPLAY_STATE;

typedef enum
{
	TCCDXBPROC_STATE_INIT			= 0x00000000,
	TCCDXBPROC_STATE_LOAD			= 0x00010000,
	TCCDXBPROC_STATE_SCAN			= 0x00020000,
	TCCDXBPROC_STATE_AIRPLAY		= 0x00040000,
	TCCDXBPROC_STATE_HANDOVER		= 0x00080000,
	TCCDXBPROC_STATE_EWSMODE		= 0x00200000

} TCCDXBPROC_STATE;

/******************************************************************************
* globals
******************************************************************************/

/******************************************************************************
* locals
******************************************************************************/

/******************************************************************************
* declarations
******************************************************************************/

int TCCDxBProc_Init(unsigned int uiCountryCode);
int TCCDxBProc_DeInit(void);
void TCCDxBProc_SetState(unsigned int uiState);
void TCCDxBProc_SetSubState(unsigned int uiSubState);
void TCCDxBProc_ClearSubState(unsigned int uiSubState);
unsigned int TCCDxBProc_GetState(void);
unsigned int TCCDxBProc_GetMainState(void);
unsigned int TCCDxBProc_GetSubState(void);
int TCCDxBProc_Set(unsigned int uiChannel,\
						unsigned int uiAudioTotalCount, unsigned int *puiAudioPID,\
						unsigned int uiVideoTotalCount, unsigned int *puiVideoPID,\
						unsigned int *puiAudioStreamType, unsigned int *puiVideoStreamType,\
						unsigned int uiSubAudioTotalCount, unsigned int *puiAudioSubPID,\
						unsigned int uiSubVideoTotalCount, unsigned int *puiVideoSubPID,\
						unsigned int *puiAudioSubStreamType, unsigned int *puiVideoSubStreamType,\
						unsigned int uiPcrPID, unsigned int uiPcrSubPID,\
						unsigned int uiStPID, unsigned int uiSiPID, unsigned int uiServiceID, unsigned int uiPmtPID, \
						unsigned int uiCAECMPID, unsigned int uiACECMPID, unsigned int uiNetworkID, \
						unsigned int uiStSubPID, unsigned int uiSiSubPID, unsigned int uiServiceSubID, unsigned int uiPmtSubPID, \
						unsigned int uiAudioIndex, unsigned int uiAudioMainIndex, unsigned int uiAudioSubIndex,\
						unsigned int uiVideoIndex, unsigned int uiVideoMainIndex, unsigned int uiVideoSubIndex,\
						unsigned int uiAudioMode, unsigned int service_type, \
						unsigned int uiRaw_w, unsigned int uiRaw_h, unsigned int uiView_w, unsigned int uiView_h, \
						int ch_index);
int TCCDxBProc_Update(unsigned int uiTotalAudioPID, unsigned int puiAudioPID[], unsigned int puiAudioStreamType[], \
						unsigned int uiVideoTotalCount, unsigned int puiVideoPID[], unsigned int puiVideoStreamType[], unsigned int uiStPID, unsigned int uiSiPID);
int TCCDxBProc_SetDualChannel(unsigned int uiChannelIndex);
int TCCDxBProc_Stop(void);
int TCCDxBProc_ScanOneChannel(int channel_num);
int TCCDxBProc_ChannelBackUp(void);
int TCCDxBProc_ChannelDBRestoration(void);
int TCCDxBProc_Scan(int scan_type, int country_code, int area_code, int channel_num, int options);
int TCCDxBProc_ScanAndSet(int country_code, int channel_num, int tsid, int options, int audioIndex, int videoIndex, int audioMode, int raw_w, int raw_h, int view_w, int view_h, int ch_index);
int TCCDxBProc_StartSubtitle(unsigned int uiPID, unsigned int uiCompositionID, unsigned int uiAncillaryID);
int TCCDxBProc_StopSubtitle(void);
int TCCDxBProc_StartTeletext(unsigned int uiPID);
int TCCDxBProc_StopTeletext(void);
int TCCDxBProc_GetSkipAVinfo(void);

//int TCCDxBProc_StartCapture(unsigned char *pucFile);
int TCCDxBProc_GetTunerStrength(int *sqinfo);
int TCCDxBProc_StopScan(void);
int TCCDxBProc_SetArea(unsigned int uiArea);
int TCCDxBProc_SetPreset(int area_code);
int TCCDxBProc_SetHandover(int country_code);
int TCCDxBProc_StopHandover(void);
int TCCDxBProc_ChangeAudio(int iAudioIndex, int iAudioMainIndex, unsigned int uiAudioMainPID, int iAudioSubIndex, unsigned int uiAudioSubPID);
int TCCDxBProc_ChangeVideo(int iVideoIndex, int iVideoMainIndex, unsigned int uiVideoMainPID, int iVideoSubIndex, unsigned int uiVideoSubPID);

int TCCDxBProc_GetCurrentIndex(int iVideoIndex, int iVideoMainIndex, unsigned int uiVideoMainPID, int iVideoSubIndex, unsigned int uiVideoSubPID);

int TCCDxBProc_SetAudioDualMono(unsigned int uiAudioMode);
int TCCDxBProc_RequestEPGUpdate(int iDayOffset);
int TCCDxBProc_CheckPCRChg(void);
void TCCDxBProc_ReqTRMPInfo(unsigned char **ppInfo, int *piInfoSize);
void TCCDxBProc_ResetTRMPInfo(void);
int TCCDxBProc_InitVideoSurface(int arg);
int TCCDxBProc_DeinitVideoSurface();
int TCCDxBProc_SetVideoSurface(void *nativeWidow);
int TCCDxBProc_VideoUseSurface(void);
int TCCDxBProc_VideoReleaseSurface(void);
void TCCDxBProc_GetCurrentChannel(int *piChannelNumber, int *piServiceID, int *piServiceSubID);
int TCCDxbProc_DecodeLogoImage(unsigned char *pucLogoStream, int iLogoStreamSize, unsigned int *pusLogoImage, int *piLogoImageWidth, int *piLogoImageHeight);
void TCCDxBProc_Get_CASPIDInfo (unsigned int *pEsPids);
void TCCDxbProc_Get_NetworkID (unsigned short *pusNetworkID);
int TCCDxBProc_SetFreqBand (int freq_band);
int TCCDxBProc_SetFieldLog (char *sPath, int fOn_Off);
int TCCDxBProc_SetPresetMode (int preset_mode);
int TCCDxBProc_SetCustomTuner(int size, void *arg);
int TCCDxBProc_GetCustomTuner(int *size, void *arg);
int TCCDxBProc_SetSectionNotificationIDs(int sectionIDs);
int TCCDxBProc_GetCur_DCCDescriptorInfo(unsigned short usServiceID, void **pDCCInfo);
int TCCDxBProc_GetCur_CADescriptorInfo(unsigned short usServiceID, unsigned char ucDescID, void *arg);
int TCCDxBProc_GetComponentTag(int iServiceID, int index, unsigned char *pucComponentTag, int *piComponentCount);

int TCCDxBProc_SetAudioMuteOnOff(unsigned int uiOnOff);
int TCCDxBProc_SetAudioOnOff(unsigned int uiOnOff);
int TCCDxBProc_SetVideoOutput(unsigned int uiOnOff);
int TCCDxBProc_SetVideoOnOff(unsigned int uiOnOff);
int TCCDxBProc_SetAudioVideoSyncMode(unsigned int uiMode);

int TCCDxBProc_GetVideoState();
int TCCDxBProc_GetSearchState();

int TCCDxBProc_DataServiceStart(void);
int TCCDxBProc_CustomFilterStart(int iPID, int iTableID);
int TCCDxBProc_CustomFilterStop(int iPID, int iTableID);

int TCCDxBProc_EWSStart(unsigned int uiChannel,\
						unsigned int uiAudioTotalCount, unsigned int *puiAudioPID,\
						unsigned int uiVideoTotalCount, unsigned int *puiVideoPID,\
						unsigned int *puiAudioStreamType, unsigned int *puiVideoStreamType,\
						unsigned int uiSubAudioTotalCount, unsigned int *puiAudioSubPID,\
						unsigned int uiSubVideoTotalCount, unsigned int *puiVideoSubPID,\
						unsigned int *puiAudioSubStreamType, unsigned int *puiVideoSubStreamType,\
						unsigned int uiPcrPID, unsigned int uiPcrSubPID,\
						unsigned int uiStPID, unsigned int uiSiPID, unsigned int uiServiceID, unsigned int uiPmtPID, \
						unsigned int uiCAECMPID, unsigned int uiACECMPID, unsigned int uiNetworkID, \
						unsigned int uiStSubPID, unsigned int uiSiSubPID, unsigned int uiServiceSubID, unsigned int uiPmtSubPID, \
						unsigned int uiAudioIndex, unsigned int uiAudioMainIndex, unsigned int uiAudioSubIndex,\
						unsigned int uiVideoIndex, unsigned int uiVideoMainIndex, unsigned int uiVideoSubIndex,\
						unsigned int uiAudioMode, unsigned int service_type, \
						unsigned int uiRaw_w, unsigned int uiRaw_h, unsigned int uiView_w, unsigned int uiView_h, \
						int ch_index);
int TCCDxBProc_EWSClear(void);
int TCCDxBProc_SetSeamlessSwitchCompensation(int iOnOff, int iInterval, int iStrength, int iNtimes, int iRange, int iGapadjust, int iGapadjust2, int iMultiplier);
int TCCDxBProc_DS_GetSTC(unsigned int *hi_data, unsigned *lo_data);
int TCCDxBProc_DS_GetServiceTime(unsigned int *year, unsigned int *month, unsigned int *day, unsigned int *hour, unsigned int *min, unsigned int *sec);
int TCCDxBProc_DS_CheckExistComponentTagInPMT(int target_component_tag);
extern void tcc_dxb_proc_set_videoonoff_value(int onoff);
extern int tcc_dxb_proc_get_videoonoff_value(void);
int TCCDxBProc_GetSeamlessValue(int *state, int *cval, int *pval);

/* Noah / 2017.04.07 / IM478A-13, Improvement of the Search API(Auto Search)
	Request : While auto searching, 'setChannel' can be called without calling 'searchCancel'.
	Previous Sequence
		1. search( 5, , , , );							 -> Start 'Auto Search'
		2. Searching in progress . . .
		3. searchCancel before finishing the searching.
		4. setChannel( X, Y, , ... );					 -> Called by Application
		5. Playing the X service.
	New Sequence
		Remove 3rd step above.
	Implement
		1. Add if statement.
		1. In case of only AUTOSEARCH, according to the new sequence, DxB works.
		2. To save scan type, I made TCCDxBProc_Set( Get )ScanType functions.
		3. Call 'searchCancel()'
		4. Change the DxBProc state to TCCDXBPROC_STATE_LOAD because TCCDxBProc_Set returns Error.
*/
void TCCDxBProc_SetScanType(TCCDXBSCAN_TYPE scanType);
TCCDXBSCAN_TYPE TCCDxBProc_GetScanType(void);


#ifdef __cplusplus
}
#endif

#endif

