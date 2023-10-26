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
#ifndef _TSSERVICE_DB_CONTROLER_STR_H_
#define _TSSERVICE_DB_CONTROLER_STR_H_

#include <TsParse_ISDBT.h>

/*****************************************************************************
*                                                                            *
*  define declaration                                                        *
*                                                                            *
*****************************************************************************/
#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif

#ifndef min
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif

/*****************************************************************************
*  define declaration                                                        *
*****************************************************************************/
#define _DEF_COUNT_SECTION	(0x08)
#define _DEF_MASK_DEFAULT	(0x80)
#define _DEF_MASK_ALLPARSE	(0xFF)

#define _DEF_RESET_LASTTABLE	(0xFF)
/*****************************************************************************
*****************************************************************************/




/*****************************************************************************
*                                                                            *
*  enum declaration                                                          *
*                                                                            *
*****************************************************************************/
/*
	LinkedList	100
	TS_DB		200
	SDT_DB		300
	Service_DB	400
	Service_APP	500
	EPG_DB		600
	EPG_APP		700
	TTX_DB		800
	TTX_App		900
*/

/*****************************************************************************
*
*  Item : LIST_TYPE
*
*****************************************************************************/
typedef enum
{
  SAT = 310,
  DTP,
  ANALOG, 
  DSVC,
  CHANNEL
   
} LIST_TYPE;


/*****************************************************************************
*
*  Item : DBTABLE_TYPE
*
*****************************************************************************/
typedef enum
{
	PAT_DB,
	PMT_DB,
	SDT_DB,
	NIT_DB,
	EIT_PF_DB,
	EIT_SCH_DB,
	ECM_DB,
	EMM_DB,
	MAX_NUM_DB
} DBTABLE_TYPE;


/*****************************************************************************
*                                                                            *
*  struct declaration                                                          *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Item : SERVICE_FLAG_STRUCT
*
* Usage : Contains the definitions for the service list array
*
* Where :
*
*****************************************************************************/
typedef struct
{
	BOOL	fEIT_Schedule;
	BOOL	fEIT_PresentFollowing;
	BOOL	fCA_Mode_free;
	BOOL	fAllowCountry;

} SERVICE_FLAG_STRUCT;

#define     MAX_PMT_ES_TABLE_SUPPORT		32

#define		MAX_VIDEOPID_SUPPORT			4
/*****************************************************************************
*
*  Item : VIDEOPID_INFO
*
* Usage : Contains the all audio pids and pid informations of a service
*
* Where :
*
*****************************************************************************/
typedef struct _video_pid_info_struct
{
	U16				uiVideo_PID;
	unsigned char		ucVideo_IsScrambling;
	unsigned char	ucVideo_StreamType;
	unsigned char    ucVideoType;
	unsigned char	ucComponent_Tag;
	char			acLangCode[3];  /*iso639 language code */
}VIDEOPID_INFO;

#define		MAX_AUDIOPID_SUPPORT			4
/*****************************************************************************
*
*  Item : AUDIOPID_INFO
*
* Usage : Contains the all audio pids and pid informations of a service
*
* Where :
*
*****************************************************************************/
typedef struct _audio_pid_info_struct
{
	U16				uiAudio_PID;
	unsigned char		ucAudio_IsScrambling;
	unsigned char	ucAudio_StreamType;
	unsigned char    ucAudioType;
	unsigned char	ucComponent_Tag;
	char			acLangCode[3];  /*iso639 language code */
}AUDIOPID_INFO;


#define		MAX_SUBTITLE_SUPPORT			10
/*****************************************************************************
*
*	Item : SUBTITLE_INFO
*	Usage : Contains the subtitle informations of a service
*	Where :
*
*****************************************************************************/
typedef struct _subtitle_info_struct
{
	U16			ucSubtitle_PID;
	char			acSubtLangCode[3];		//ISO_LANG_CODE_LENGTH : 3
	U8 			ucSubtType;
	unsigned char	ucComponent_Tag;
   	U16			usCompositionPageId;
	U16			usAncillaryPageId;   
}SUBTITLE_INFO;

#define COUNTRY_CODE_MAX   3
#define COUNTRY_REGION_ID_MAX		60

typedef struct
{
	unsigned char		aucCountryCode[COUNTRY_CODE_MAX];
	unsigned char		ucCountryRegionID;
	unsigned char		ucLocalTimeOffsetPolarity;
	unsigned char		ucLocalTimeOffset_BCD[2];	/* 4 BCD characters */
	unsigned char		ucTimeOfChange_BCD[5];		/* 4 BCD characters */
	unsigned char		ucNextTimeOffset_BCD[2];	/* 4 BCD characters */
	
}  LOCAL_TIME_OFFSET_INFO;

typedef struct
{
	unsigned char				ucCount;
	LOCAL_TIME_OFFSET_INFO	astLocalTimeOffsetData[COUNTRY_REGION_ID_MAX];
   
}  LOCAL_TIME_OFFSET_STRUCT, *P_LOCAL_TIME_OFFSET_STRUCT;


/*****************************************************************************
*
*	Item : T_DVB_EIT_EVENT_DATA
*
*****************************************************************************/
typedef struct dvb_eit_event_data_struct
{
	unsigned short			uiEventID;
	DATE_TIME_STRUCT		stStartTime;
	TIME_STRUCT				stDuration;
} T_ISDBT_EIT_EVENT_DATA, *P_ISDBT_EIT_EVENT_DATA;

/*****************************************************************************
*
*	Item : T_DVB_EIT_STRUCT
*
*****************************************************************************/
typedef struct dvb_eit_struct
{
	unsigned short	uiTableID; 
	unsigned short	uiTStreamID;
	unsigned short	uiOrgNetWorkID;
	unsigned short	uiService_ID;         /* is important as it is used as   */
	unsigned char	ucVersionNumber;      /* a mask for all table structs    */
	unsigned char	ucSection;
	unsigned char	ucLastSection;
	unsigned char	ucSegmentLastSection; 
} T_ISDBT_EIT_STRUCT, *P_ISDBT_EIT_STRUCT;

typedef struct 
{
	unsigned short	eventID;
	unsigned char	group_type;
	unsigned char	event_count;
	unsigned short	service_id;
	unsigned short	event_id;
	unsigned short	original_network_id;
	unsigned short	transport_stream_id;
} T_ISDBT_EIT_EVENT_GROUP;

/*****************************************************************************
*
*  Item : SERVICE_STRUCT
*
* Usage : Contains the definitions for the service list array
*
* Where :
*           uiServiceID     = the Service ID, if unused = 0xFFFF
*           uiPMT_PID   = last PID where PMT was found, 0xFFFF = unknown
*           enType         = type of service, e.g. digital radio or TV
*           bTransIndex    = index into transport array, 0xFF = unused
*           bBouquetIndex  = index into bouquet array, 0xFF = unused
*           cName          = name, (maximum 9 chars or zero delimited)
*
*****************************************************************************/

typedef struct dvb_service_struct
{
	U16						uiCurrentChannelNumber;
	U16						uiCurrentCountryCode;

	U8						ucVersionNum;  //PAT Version
	
	U16						usNetworkID;
	U16						uiTStreamID;
	U16						usOrig_Net_ID;
	
	U16						usServiceID;
	
	U16						PMT_PID;
	U16						remocon_ID;
	int			enType;
	SERVICE_FLAG_STRUCT		stFlags;
	U8						ucNameLen;
	char					*strServiceName;

	U16						uiCA_ECM_PID;
	U16						uiAC_ECM_PID;

	U16						uiPCR_PID;

	//video management
	U16						uiTotalVideo_PID;
	VIDEOPID_INFO			stVideo_PIDInfo[MAX_VIDEOPID_SUPPORT];
	U8						ucActiveVideoIndex; 
	
	//audio management
	U16						uiTotalAudio_PID;
	AUDIOPID_INFO			stAudio_PIDInfo[MAX_AUDIOPID_SUPPORT];
	U8						ucActiveAudioIndex; 

	unsigned char				ucLastTableID;

	/*Logical Channel assignment	
	**0 Shall not be used
	**1 - 799 Broadcaster assigned logical_channel_number
	**800 - 999 Shall not be used
	**1000 - 1023 Reserved for future use
	**/
	unsigned int			uiLogicalChannel;

	//bya&. 2007.11.26.
	U16						usTotalCntSubtLang;
	SUBTITLE_INFO			stSubtitleInfo[MAX_SUBTITLE_SUPPORT];

	U16						uiTeletext_PID;
//	unsigned char				ucCount_Teletext_Info_Max;
	unsigned char				ucCount_Teletext_Info;
	U8						ucPMTVersionNum;  //PMT Version
	U8						ucSDTVersionNum;  //SDT Version
	U8 						ucEITPFSecVersion[2];
	U8 						ucEITSCHSecVersion[256];

	unsigned short			uiDSMCC_PID;
	unsigned short			ucAutoStartFlag;

	unsigned char			ucPMTEntryTotalCount;
	unsigned char			ucComponent_Tags[MAX_PMT_ES_TABLE_SUPPORT];

	/*--- logo ---*/
	int						logo_id;		/* -1: no info, else: logo_id */
	unsigned char			*logo_char;
	unsigned short			download_data_id;
	unsigned char			logo_transmission_type;

	int						iTSNameLen;
	char 					*strTSName;		/* to save ts_name_char of ts_information_descriptor */

	int						iPartialReceptionDescNum; /* to save a number of partitial reception descriptor(service) */
	int						iTotalServiceNum;
} T_ISDBT_SERVICE_STRUCT, *P_ISDBT_SERVICE_STRUCT;

typedef struct t_isdbt_logo_struct {
	unsigned short			download_data_id;
	unsigned char				ucSection;
	unsigned char				ucLastSection;
	unsigned short			original_network_id;
	unsigned short			logo_type;
	unsigned short			logo_id;
	unsigned short			logo_version;
	unsigned short			data_size_length;
	unsigned char 			*data_byte;
} T_ISDBT_LOGO_INFO, *P_ISDBT_LOGO_INFO;
#endif	//_TSSERVICE_DB_CONTROLER_STR_H_
