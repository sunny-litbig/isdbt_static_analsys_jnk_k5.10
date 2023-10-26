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
#ifndef _TCC_EPG_DB_H_
#define	_TCC_EPG_DB_H_

#ifdef __cplusplus
extern "C" {
#endif


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
void TCCISDBT_EPGDB_Init(void);
void TCCISDBT_EPGDB_DeInit(void);
void TCCISDBT_EPGDB_Delete(int iType, int iServiceId, int iTableId);
void TCCISDBT_EPGDB_DeleteAll(void);
int TCCISDBT_EPGDB_GetCount(int iType, int uiCurrentChannelNumber, int uiCurrentCountryCode, int uiEventID, P_ISDBT_EIT_STRUCT pstEIT);
int TCCISDBT_EPGDB_Insert(int iType, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_EIT_STRUCT pstEIT, P_ISDBT_EIT_EVENT_DATA pstEITEvent);
int TCCISDBT_EPGDB_UpdateEvent(int iType, int uiEvtNameLen, char *pEvtName, int uiEvtTextLen, char *pEvtText, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_EIT_STRUCT pstEIT, P_ISDBT_EIT_EVENT_DATA pstEITEvent);
int TCCISDBT_EPGDB_UpdateExtEvent(int iType, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_EIT_STRUCT pstEIT, P_ISDBT_EIT_EVENT_DATA pstEITEvent, EXT_EVT_DESCR *pstEED);
int TCCISDBT_EPGDB_UpdateEvtGroup(int iType, int uiReferenceServiceID, int uiReferenceEventID, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_EIT_STRUCT pstEIT, P_ISDBT_EIT_EVENT_DATA pstEITEvent);
int TCCISDBT_EPGDB_UpdateContent(int iType, unsigned int uiContentGenre, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_EIT_STRUCT pstEIT, P_ISDBT_EIT_EVENT_DATA pstEITEvent);
int TCCISDBT_EPGDB_UpdateRating(int iType, unsigned int uiRating, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_EIT_STRUCT pstEIT, P_ISDBT_EIT_EVENT_DATA pstEITEvent);
int TCCISDBT_EPGDB_GetVersion(int iType, int usServiceID, int uiCurrentChannelNumber, int uiTableId, int ucSection);
void TCCISDBT_EPGDB_GetChannelInfo(int iType, int uiCurrentChannelNumber, int iServiceID, int *piAudioMode, int *piVideoMode, int *piAudioLang1, int *piAudioLang2, int *piStartMJD, int *piStartHH, int *piStartMM, int *piStartSS, int *piDurationHH, int *piDurationMM, int *piDurationSS, unsigned short *pusEvtName, int *piEvtNameLen);
void TCCISDBT_EPGDB_GetChannelInfo2(int iType, int uiCurrentChannelNumber, int iServiceID, int *piGenre);
void TCCISDBT_EPGDB_GetDateTime(int iAreaCode, DATE_TIME_STRUCT *pstDateTime, LOCAL_TIME_OFFSET_STRUCT *pstLocalTimeOffset, int *piMJD, int *piHour, int *piMin, int *piSec, int *piPolarity, int *piHourOffset, int *piMinOffset);
int TCCISDBT_EPGDB_UpdateVideoInfo (int iType, unsigned int component_tag, unsigned int component_type, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_EIT_STRUCT pstEIT, P_ISDBT_EIT_EVENT_DATA pstEITEvent);
int TCCISDBT_EPGDB_UpdateAudioInfo (int iType, unsigned int component_tag, int component_type, int sampling_rate, unsigned int uiLangCode1, unsigned int uiLangCode2, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_EIT_STRUCT pstEIT, P_ISDBT_EIT_EVENT_DATA pstEITEvent);

int TCCISDBT_EPGDB_CommitEvent(sqlite3 *sqlhandle, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_EIT_STRUCT pstEIT, int iEventID);
// Noah, 20180611, NOT support now
//int TCCISDBT_EPGDB_Commit(sqlite3 *sqlhandle, int iDayOffset, int iChannel, int iAreaCode, DATE_TIME_STRUCT *pstDateTime, LOCAL_TIME_OFFSET_STRUCT *pstLocalTimeOffset);

#ifdef __cplusplus
}
#endif

#endif

