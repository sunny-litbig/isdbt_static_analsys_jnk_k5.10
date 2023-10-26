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
#ifndef __ISDBT_EPG_H__
#define __ISDBT_EPG_H__

#ifndef FALSE
#define FALSE	0
#endif

#ifndef TRUE
#define TRUE		1
#endif

#define TC_SUCCESS 0 

/* language type */
typedef enum{
	UI_LANG_ENGLISH,
	UI_LANG_GERMAN,
	UI_LANG_SPANICH,
	UI_LANG_FRENCH,
	UI_LANG_ITALIAN,
	UI_LANG_RUSSIAN,
	UI_LANG_CHINESE_TRADITION,
	UI_LANG_CHINESE_SIMPLE,
	UI_LANG_JAPANESE,
	UI_LANG_PORTUGUESE,
	UI_LANG_DANISH,
	UI_LANG_NORWEGIAN,
	UI_LANG_SWEDISH,
	UI_LANG_FINNISH,
	UI_LANG_KOREAN,
	UI_LANG_CENTRAL_EUROPE,
	UI_LANG_LATIN_I,
	UI_NUM_OF_LANGUAGE
}LANG_TYPE;


/* [Local time reflection Test] Display local time of country region id #3 (Sao Paulo) */
#ifdef DxB_SBTVD_INCLUDE
//#define	SBTVD_TEST_TOT_DETECT_SAOPAULO
#endif

/* If you want to change the value of this symbolic constant, 
    you should change the constant of same name in "mpeg_def.h" */
#ifndef AFFILIATION_ID_MAX
#define AFFILIATION_ID_MAX              (255)
#endif

#ifndef COUNTRY_CODE_MAX
#define COUNTRY_CODE_MAX                (3)
#endif

#ifndef COUNTRY_REGION_ID_MAX
#define	COUNTRY_REGION_ID_MAX           (10)
#endif

#define	ISDBT_EPG_STR_OFFSET            (5)

#define	ISDBT_EPG_DISPBUF_WIDTH         (320)
#define	ISDBT_EPG_DISPBUF_HEIGHT        (240)

#define	ISDBT_EPG_DISPTXT_WIDTH		ISDBT_EPG_DISPBUF_WIDTH - 10

#define	ISDBT_EPG_WIDTH	                (320)
#define	ISDBT_EPG_HEIGHT                (240)

#define 	EPG_BAR_BOX_START_X 			ISDBT_EPG_DISPBUF_WIDTH - 8
#define EPG_BAR_BOX_START_Y             (28)
#define 	EPG_BAR_BOX_END_X			ISDBT_EPG_DISPBUF_WIDTH - 1
#define 	EPG_BAR_BOX_END_Y			ISDBT_EPG_DISPBUF_HEIGHT - 11

#ifdef DxB_SBTVD_INCLUDE
#define	ISDBT_MAX_REGION_ID_NUM 		(7)	// The num. of Time Zone Brazil use is 7
#else
#define	ISDBT_MAX_REGION_ID_NUM 		(1)	// The num. of Time Zone Japan use is only 1
#endif

#define	ISDBT_TIME_MAX_DISPLAY_LINE     ISDBT_MAX_REGION_ID_NUM

#define ISDBT_EPG_MAX_TXT_BYTE          (256)
#define ISDBT_EPG_MAXLINE_PER_ONEPAGE   (5)
#define ISDBT_EPG_MAXCHAR_PER_ONELINE   (17)
#define ISDBT_EPG_LINEFEED_SIZE         (18)
// (*) NSZ: Normal SiZe, MSZ: Middle SiZe
#define	ISDBT_EPG_NSZ_FONTWD            (16)
#define	ISDBT_EPG_MSZ_FONTWD			(ISDBT_EPG_NSZ_FONTWD / 2)

/* EPG Display mode */
#define	SCROLL_EVENTNAME                (0)
#define	NOSCROLL_EVENTNAME              (1)

/* present(mandatory) + following(mandatory) + p/f after(optional, max. 7) */
#define	MAX_L_EIT_NUM                   (10)	// Max. EIT(Event) num for 1-Seg device
#define	MAX_TS_SECTION_NUM              (10)	// Max. section num. in one TS

/* Max. Num. of large/middle genre */
#define ISDBT_EPG_GENRE_NUM             (16) 	

#define	EVENT_RATING_DONE               (1000)

/* Choose Default EPG Channel name to display */
//#define	ISDBT_EPG_TSNAME_USE
#define	ISDBT_EPG_SERVICENAME_USE

typedef enum
{
	DISPLAY_EVENT_TIMEOFFSET,
	DISPLAY_EVENT_TEXT,
	DISPLAY_EVENT_NEXT,
	DISPLAY_EVENT_PREV	
} ISDBT_EPG_DISPLAY_MODE;


typedef struct
{
	unsigned short	year;
	unsigned char		month;
	unsigned char		day;
	unsigned char		weekday;
} DATE_TIME_T;

typedef struct 
{
	unsigned char 	TsTableId;
	unsigned char 	TsSectionNum;
	unsigned short	TsSectionLen;
	unsigned short	TsReadIdx;
} TS_SECTION_INFO;

/* L-EIT Linked list for 1-SEG EPG in ISDB-T */
typedef struct _L_EIT_EPG_NODE_
{
	unsigned short 	eventID;	// used for sorting events
	unsigned char		nodeIndex;
	unsigned char		rating;
	
	unsigned char        staHour;
	unsigned char        staMinute;
	unsigned char        staSecond;
	unsigned short	staMJD;

	unsigned char        durHour;
	unsigned char        durMinute;
	unsigned char        durSecond;

	unsigned char        endHour;
	unsigned char        endMinute;
	unsigned char        endSecond;

	/* short_event_descriptor */
	unsigned char		ucLangCode[3];
	unsigned char		eventNamelen;
	unsigned char  	*pEventName;
	unsigned char		eventTextLen;
	unsigned char 	*pEventText;

	/* content_descriptor */
	unsigned char		largeGenre;
	unsigned char 	middleGenre;
	
	/* digital_copy_control_descriptor */
	unsigned char		digitalRecCtrl;
	unsigned char		componentCtrlFlag;

	struct _L_EIT_EPG_NODE_ * pNext;
	struct _L_EIT_EPG_NODE_ * pPrev;
} L_EIT_EPG_NODE;


typedef struct 
{
	unsigned short	totalEventCnt;
	unsigned char		fDetectTmOffset;
	unsigned char		hourOffset;
	unsigned char		minOffset;
	
	unsigned char 	version;
	unsigned short	networkId;	
	unsigned char		TSNameLength;
	unsigned char	*	pTSName;
	unsigned short 	areaCode;
	unsigned char		guardInterval;
	unsigned char		transmissionMode;
	unsigned char		fRemoconKeyId; // remote_control_key_id is acquired or not
	unsigned char 	remoconKeyId;

	unsigned char		serviceType;
	unsigned char		serviceNumber;

	unsigned short		numServiceId;
	unsigned char 		bServiceIdDetected;
	
	unsigned short 	serviceId;
	unsigned short      otherServiceId[8];
	unsigned char		serviceNameLength;
	unsigned char *	pServiceName;
	unsigned char		affiliationIdNum;
	unsigned char 	affiliationIdArray[AFFILIATION_ID_MAX+1];
	
	L_EIT_EPG_NODE * pHead;
} L_EIT_EPG_LIST_h;

typedef struct _ChannelList_
{
	unsigned char		TSNameLength;
	unsigned char	*	pTSName;
	unsigned char		fRemoconKeyId; // remote_control_key_id is acquired or not
	unsigned char 	remoconKeyId;
	
	struct _ChannelList_ * pNext;
} ISDBT_ChannelList;

typedef struct _TS_VERSION_INFO_
{
	unsigned char	version;
	unsigned char	prevSectionNum;
	unsigned char	lastSectionNum;
	unsigned char	fLastSectionParsed;
	unsigned char	ucSectionNumberCheck;
} TS_VERSION_INFO;


typedef struct _TS_EIT_VERSION_INFO_
{
	unsigned char		Version;
	unsigned char		TableID;	
	unsigned short 	PID;
} TS_EIT_VERSION_INFO;


typedef struct _L_TOT_LOCAL_INFO_
{
	unsigned char		CountryCode[COUNTRY_CODE_MAX];
	unsigned char		CountryRegionID;
	unsigned char		OffsetPolarity;
	unsigned char		LocalTimeOffset_BCD[2];		/* 4 BCD characters */
	unsigned char		TimeOfChange_BCD[5];		/* 4 BCD characters */
	unsigned char		NextTimeOffset_BCD[2];		/* 4 BCD characters */
} L_TOT_LOCAL_INFO;


typedef struct _ISDBT_TOT_INFO_
{
	unsigned short		currentMJD;
	unsigned char			currentHour;
	unsigned char			currentMin;
	unsigned char			currentSec;
	unsigned char			localCount;
	L_TOT_LOCAL_INFO 	LocalInfoArray[COUNTRY_REGION_ID_MAX]; 
}  ISDBT_TOT_INFO;


typedef struct
{
	ISDBT_EPG_DISPLAY_MODE	eDisplayStatus;
	unsigned char		highlightDisplayMode;
	unsigned char		fHoldScroll;
	unsigned int		fgColor;
	unsigned int		bgColor;
	
	unsigned char 	epgDbInx;
	unsigned char 	targetEventInx;
	unsigned char		currentLastEventInx;
	unsigned char		currentFirstEventInx;
	unsigned int		bitmapWD;
	unsigned short	scrStartY;
	unsigned short	uniStringSize;
	unsigned short 	remainCharNum;
} ISDBT_EPGUI_HIGHLIGHT;

typedef struct
{
	const char *pLargeGenre;
	const char **ppMiddleGenre;
} Genre;

#ifndef INT
#define INT(_x, _y) ((_x) /  (_y)) * (_y)
#endif

/*------------------------------------------------------------------------*/
/* 1Seg EPG DB Ctrl/Mgmt. functions */
/*------------------------------------------------------------------------*/
extern L_EIT_EPG_LIST_h * ISDBT_EPG_CreateEventList_h(void);
extern void	ISDBT_EPG_FreeEventList_h(L_EIT_EPG_LIST_h *);	
extern int		ISDBT_EPG_AddLastEvent(L_EIT_EPG_LIST_h *, L_EIT_EPG_NODE *);
extern void	ISDBT_EPG_DeleteLastEvent(L_EIT_EPG_LIST_h *);
extern void	ISDBT_EPG_DeleteAllEvent(L_EIT_EPG_LIST_h *);
extern void	ISDBT_EPG_RemoveDB(void);

/*------------------------------------------------------------------------*/
/* 1Seg EPG DB Access functions */
/*------------------------------------------------------------------------*/
extern void * ISDBT_EPG_GetEventDBRoot(void);
extern void * ISDBT_EPG_GetFirstEvent(void *);
extern void * ISDBT_EPG_MoveNextEvent(void *);
extern void * ISDBT_EPG_FindEventNode(void *, unsigned char);
extern unsigned char ISDBT_EPG_GetCurEventIndex(void *);
extern short ISDBT_EPG_GetTotEventNum(void *);
extern short 	ISDBT_EPG_GetCurNetworkID(void *);
extern unsigned short ISDBT_EPG_GetCurServiceID(void);
extern short	ISDBT_EPG_GetAreaCode(void *);
extern unsigned char ISDBT_EPG_GetGuardInterval(void *);
extern unsigned char ISDBT_EPG_GetTransmissionMode(void *);
extern char	ISDBT_EPG_GetRemoconKeyID(void *);
extern short 	ISDBT_EPG_CalculateChannelNum(void *);

/*  [SBTVD] Channel number extraction related */
#ifdef DxB_SBTVD_INCLUDE
extern unsigned char SBTVD_EPG_GetServiceType(void *pstEpgList);
extern unsigned char SBTVD_EPG_GetServiceNumber(void *pstEpgList);
#endif	// DxB_SBTVD_INCLUDE

/*  [ISDBT] Channel number extraction related */
extern unsigned char * 	ISDBT_EPG_GetServiceChannelName(void *, unsigned char * );
extern unsigned char * 	ISDBT_EPG_GetTSName(void *, unsigned char * );
extern unsigned char * 	ISDBT_EPG_GetAffiliationIdArrayPtr(void *, unsigned char * );
extern unsigned char ISDBT_EPG_GetTimeOffsetState(void);

extern unsigned short ISDBT_EPG_GetStartTime_MJD(void *);
extern unsigned char ISDBT_EPG_GetStartTime_Hour(void *);
extern unsigned char ISDBT_EPG_GetStartTime_Minute(void *);
extern unsigned char ISDBT_EPG_GetStartTime_Second(void *);
extern unsigned short ISDBT_EPG_AdjustStartTime_MJD(void *pstEpg, unsigned char polarity);
extern unsigned char ISDBT_EPG_GetDurTime_Hour(void *);
extern unsigned char ISDBT_EPG_GetDurTime_Minute(void *);
extern unsigned char ISDBT_EPG_GetDurTime_Second(void *);
extern unsigned char ISDBT_EPG_GetEndTime_Hour(void *);
extern unsigned char ISDBT_EPG_GetEndTime_Minute(void *);
extern unsigned char ISDBT_EPG_GetEndTime_Second(void *);
extern unsigned char ISDBT_EPG_GetEventLangCode(void *);
extern unsigned char * ISDBT_EPG_GetEventName(void *, unsigned char *);
extern unsigned char * ISDBT_EPG_GetEventText(void *, unsigned char *);

/*------------------------------------------------------------------------*/
/* 1Seg EPG DB Utility functions */
/*------------------------------------------------------------------------*/
extern unsigned char * ISDBT_EPG_GetGenreString(void *);
extern unsigned char * ISDBT_EPG_GetGenreDetailString(void *);
extern void ISDBT_EPG_MakeDisplayTimeString(void *, DATE_TIME_T, unsigned char *);
extern void ISDBT_TIME_GetRealDate(DATE_TIME_T *pRealDate, unsigned int  MJD);
extern void ISDBT_EPG_ChangeUni_Alpha2AsciiChar(unsigned char *, unsigned int);
extern void ISDBT_EPG_ChangeUni_Alpha2AsciiChar_HalfWD(unsigned char *, unsigned int);
extern void ISDBT_EPG_ChangeUni_LatinExt2AsciiChar_HalfWD(unsigned char *, unsigned int);

/*------------------------------------------------------------------------*/
/* 1Seg EPG display (buffer) access functions                                                                  */
/*------------------------------------------------------------------------*/
extern unsigned char * ISDBT_EPG_GetDisplayBuf(void);
extern void ISDBT_EPG_CreateDisplayBuf(void);
extern void ISDBT_EPG_RemoveDisplayBuf(void);
#ifdef ISDBT_1SEG_EPG_DISPLAY
extern void	ISDBT_EscapeEPGServiceMode(void);
#endif	// ISDBT_1SEG_EPG_DISPLAY

/*------------------------------------------------------------------------*/
/* ISDB-T TOT DB Access functions */
/*------------------------------------------------------------------------*/
extern void * ISDBT_GetTotDB(void);
extern void ISDBT_TOT_RemoveDB(void);
extern unsigned short ISDBT_TOT_GetMJD(void * pstTot);
extern unsigned char ISDBT_TOT_GetHour(void * pstTot);
extern unsigned char ISDBT_TOT_GetMinute(void * pstTot);
extern unsigned char ISDBT_TOT_GetSecond(void * pstTot);
extern unsigned short ISDBT_TOT_AdjustLocalTime_MJD(void *pstTot, unsigned char polarity);

extern unsigned char ISDBT_TOT_GetLocalTimeOffsetClock(
	void * 			pstTot, 
	unsigned short	MJDVal,
	void *			pReal_date,
	unsigned char * 	pRegionHour, 
	unsigned char * 	pRegionMinute, 
	unsigned char *	pTmPolarity,
	unsigned char *	pHourOffset,
	unsigned char *	pMinuteOffset,
	unsigned char 	LTO_RegionID	/* 0x00: determine region automatically (Not yet implemented !!) */
									/* 0x01 ~ 0x3C (1 ~ 60): Region ID available */
									/* Over 0x3C : Do not apply a local time offset to time */
);

/*  [ISDBT] Local Time Offset Related */
extern unsigned char ISDBT_TOT_GetLocalRegionCnt(void * pstTot);
extern void ISDBT_TOT_GetCountrycode(void * pstTot, unsigned char * paucCountryCode);
extern unsigned char ISDBT_TOT_GetCountryRegionId(void * pstTot);
extern unsigned char ISDBT_TOT_GetLocalTimeOffsetPolarity(void * pstTot);
extern void ISDBT_TOT_GetLocalTimeOffset(void * pstTot, unsigned char * paucLTOffset);
extern void ISDBT_TOT_GetTimeOfChange(void * pstTot, unsigned char * paucTOC);
extern void ISDBT_TOT_GetNextTimeOffset(void * pstTot, unsigned char * paucNTO);

/*  [SBTVD] Local Time Offset Related */
#if (1) //def DxB_SBTVD_INCLUDE
extern void SBTVD_TOT_GetCountrycode(void * pstTot, unsigned char * paucCountryCode, unsigned char idx);
extern unsigned char SBTVD_TOT_GetCountryRegionId(void * pstTot, unsigned char idx);
extern unsigned char SBTVD_TOT_GetLocalTimeOffsetPolarity(void * pstTot, unsigned char idx);
extern void SBTVD_TOT_GetLocalTimeOffset(void * pstTot, unsigned char * paucLTOffset, unsigned char idx);
extern void SBTVD_TOT_GetTimeOfChange(void * pstTot, unsigned char * paucTOC, unsigned char idx);
extern void SBTVD_TOT_GetNextTimeOffset(void * pstTot, unsigned char * paucNTO, unsigned char idx);
#endif	// DxB_SBTVD_INCLUDE

/*------------------------------------------------------------------------*/
/* EPG Display related functions */
/*------------------------------------------------------------------------*/
extern void  ISDBT_EPG_SetUIHighlightEventMgr(unsigned int uiBitmapWD, unsigned short y, unsigned short strLen, 
											unsigned short remainCharNum, unsigned char * string, unsigned int fclr, 
											unsigned int bclr, unsigned char highlightDisplayMode);
#endif // __ISDBT_EPG_H__

