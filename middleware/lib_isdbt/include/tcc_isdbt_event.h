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
#ifndef _TCC_ISDBT_EVENT_H_
#define	_TCC_ISDBT_EVENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ISDBT_Common.h>

/******************************************************************************
* defines
******************************************************************************/
#define    MAX_AUDIOTRACK_SUPPORT            5
#define    MAX_SUBTITLETRACK_SUPPORT        10



/******************************************************************************
* typedefs & structure
******************************************************************************/
typedef struct TCC_DVB_DEC_SUBTITLE_CMD_Structure
{
	int subtitle_type;
	int update_flag;
	int internal_sound_index;
	int hd_flag;
	int auto_display;
}TCC_DVB_DEC_SUBTITLE_CMD_t, *pTCC_DVB_DEC_SUBTITLE_CMD_t;

typedef struct TCC_DVB_SUBTITLE_Structure
{
	int x;
	int y;
	int width;
	int height;
	int size;
	unsigned int pts;
	char *data;
} TCC_DVB_SUBTITLE_t, *pTCC_DVB_SUBTITLE_t;

typedef struct TCC_DVB_VIDEO_DEFINITIONTYPE {
    unsigned int nFrameWidth;
    unsigned int nFrameHeight;
    unsigned int nAspectRatio;
    unsigned int nFrameRate;
    unsigned int MainDecoderID;
    unsigned int nSubFrameWidth;
    unsigned int nSubFrameHeight;
    unsigned int nSubAspectRatio;
    unsigned int SubDecoderID;
    unsigned int DisplayID;
} TCC_DVB_VIDEO_DEFINITIONTYPE;

typedef struct DB_CHANNEL_ELEMENT_Structure {
	unsigned int channelNumber;
	unsigned int countryCode;
	unsigned int audioPID;
	unsigned int videoPID;
	unsigned int stPID;
	unsigned int siPID;
	unsigned int PMT_PID;
	unsigned int remoconID;
	unsigned int serviceType;
	unsigned int serviceID;
	unsigned int regionID;
	unsigned int threedigitNumber;
	unsigned int TStreamID;
	unsigned int berAVG;
	unsigned int preset;
	unsigned int networkID;
	unsigned int areaCode;
	unsigned short channelName[256];
	//T_DESC_EBD stDescEBD;
	//T_DESC_TDSD stDescTDSD;
} DB_CHANNEL_ELEMENT;

typedef struct DB_SERVICE_ELEMENT_Structure {
	unsigned int audio_stream_type;
	unsigned int video_stream_type;
	unsigned int uiPCRPID;
	unsigned int uiECMPID;
} DB_SERVICE_ELEMENT;

typedef struct DB_AUDIO_ELEMENT_Structure {
	unsigned int uiServiceID;
	unsigned int uiCurrentChannelNumber;
	unsigned int uiCurrentCountryCode;
	unsigned int usNetworkID;
	unsigned int uiAudio_PID;
	unsigned int ucAudio_IsScrambling;
	unsigned int ucAudio_StreamType;
	unsigned int ucAudioType;
	unsigned char acLangCode[8];
} DB_AUDIO_ELEMENT;

typedef struct CHANNEL_HEADER_INFORMATION_Structure {
	DB_CHANNEL_ELEMENT dbCh;
	DB_SERVICE_ELEMENT dbService;
	DB_AUDIO_ELEMENT   dbAudio[MAX_AUDIOTRACK_SUPPORT];
} CHANNEL_HEADER_INFORMATION;

typedef struct
{
	int	iIndex;
	int	iLength;
	char*	msg;
}TCC_DVB_DEBUGMSG;


/******************************************************************************
* globals
******************************************************************************/

/******************************************************************************
* locals
******************************************************************************/

/******************************************************************************
* declarations
******************************************************************************/
int TCCDxBEvent_Init(void);
int TCCDxBEvent_DeInit(void);
unsigned int TCCDxBEvent_SearchingProgress(void *arg, int ch);
unsigned int TCCDxBEvent_SearchingDone(void *arg);
unsigned int TCCDxBEvent_FirstFrameDisplayed(void *arg);
unsigned int TCCDxBEvent_VideoError(void *arg);
unsigned int TCCDxBEvent_ChannelUpdate(void *arg);
unsigned int TCCDxBEvent_Emergency_Info_Update(void *arg);
unsigned int TCCDxBEvent_Process();
unsigned int TCCDxBEvent_SubtitleUpdateLinux(void *arg);
unsigned int TCCDxBEvent_VideoDefinitionUpdate(void *arg);
unsigned int TCCDxBEvent_EPGUpdate(int service_id, int tableID);
unsigned int TCCDxBEvent_AutoSearchProgress(void *arg);
unsigned int TCCDxBEvent_AutoSearchDone(void *arg);
unsigned int TCCDxBEvent_DESCUpdate(unsigned short usServiceID, unsigned char ucDescID, void *arg);
unsigned int TCCDxBEvent_EventRelay(unsigned short usServiceID, unsigned short usEventID, unsigned short usOriginalNetworkID, unsigned short usTransportStreamID);
unsigned int TCCDxBEvent_MVTVUpdate(unsigned char *pucArr);
unsigned int TCCDxBEvent_TRMPErrorUpdate(int status, int ch);
unsigned int TCCDxBEvent_AVIndexUpdate(unsigned int channelNumber, unsigned int networkID, \
											unsigned int serviceID, unsigned int serviceSubID, \
											unsigned int audioIndex, unsigned int videoIndex);
unsigned int TCCDxBEvent_SectionUpdate(unsigned int channelNumber , unsigned int sectionID);
unsigned int TCCDxBEvent_SearchAndSetChannel(unsigned int status, unsigned int countryCode, unsigned int channelNumber, unsigned int mainRowID, unsigned int subRowID);
unsigned int TCCDxBEvent_DetectServiceNumChange(void);
unsigned int TCCDxBEvent_TunerCustomEvent (void *pEventData);
unsigned int TCCDxBEvent_SectionDataUpdate(unsigned short usServiceID, unsigned short usTableID, unsigned int uiRawDataLen, unsigned char *pucRawData, unsigned short usNetworkID, unsigned short usTransportStreamID, unsigned short usDataServicePID, unsigned char auto_start_flag);
unsigned int TCCDxBEvent_CustomFilterDataUpdate(int iPID, unsigned char *pucBuf, unsigned int uiBufSize);
unsigned int TCCDxBEvent_VideoStartStop(int iStart);
unsigned int TCCDxBEvent_AudioStartStop(int iStart);
int TCCDxBEvent_ServiceID_Check(int valid);
int TCCDxBEvent_CustomSearchStatus(int status, int *arg);
int TCCDxBEvent_SpecialServiceUpdate (int status, int _id);

#ifdef __cplusplus
}
#endif

#endif

