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
#ifndef	_TCC_CUI_H__
#define	_TCC_CUI_H__

/******************************************************************************
* include 
******************************************************************************/
//#include "main.h"
//#include "globals.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
//#include "tcc_memory.h"

#define	TCC_FILE_PATH_MAX	1024
#define	TCC_POLL_TIME_OUT	1000

#ifndef MAX_FILENAME_SIZE
#define MAX_FILENAME_SIZE		256
#endif

#ifndef TRUE
#define TRUE                1
#endif

#ifndef FALSE
#define FALSE               0
#endif

/******************************************************************************
* defines 
******************************************************************************/

#define	TCC_CUI_DEVICE	"/dev/console"



/******************************************************************************
* typedefs & structure
******************************************************************************/

enum	
{
	TCC_CUI_NULL= 0,
	TCC_CUI_OPEN,
	TCC_CUI_CLOSE,
	TCC_CUI_PLAY,
	TCC_CUI_PREV,
	TCC_CUI_NEXT,
	TCC_CUI_STOP,
	TCC_CUI_PAUSE,
	TCC_CUI_RESUME,
	TCC_CUI_SCAN,
	TCC_CUI_SCAN_STOP,	
	TCC_CUI_SCAN_MANUAL,
	TCC_CUI_RECORD,
	TCC_CUI_RECORDSTOP,
	TCC_CUI_FILEPLAY,
	TCC_CUI_FILEPLAYSTOP,
	TCC_CUI_FILEPLAYPAUSE,
	TCC_CUI_FILEPLAYSEEK,
	TCC_CUI_FILEPLAYMODE,
	TCC_CUI_PRESET,
	TCC_CUI_PREPARE,
	TCC_CUI_REW,
	TCC_CUI_FF,
	TCC_CUI_TIME_SEEK,
	TCC_CUI_VOLUME_UP,
	TCC_CUI_VOLUME_DOWN,
	TCC_CUI_VOLUME_SET,
	TCC_CUI_PLAY_MODE,
	TCC_CUI_PLAY_LIST,
	TCC_CUI_AP_REPEAT,
	TCC_CUI_RECODE,
	TCC_CUI_SAVEAS,	
	TCC_CUI_CODEC,
	TCC_CUI_INPUT,
	TCC_CUI_BITRATE,
	TCC_CUI_RECODE_STOP,
	TCC_CUI_INFO,
	TCC_CUI_GETCONTENTINFO,
	TCC_CUI_EXIT,
	TCC_CUI_HELP,
	TCC_CUI_DEQ_SET,
	TCC_CUI_START,
	TCC_CUI_SEARCH,
	TCC_CUI_SET,
	TCC_CUI_SET_WITHCHNUM,
	TCC_CUI_SET_BB,
	TCC_CUI_SET_BB_EX,
	TCC_CUI_SET_DMB_MODE,
	TCC_CUI_SET_DISPLAY,
	TCC_CUI_SET_MANUAL,
	TCC_CUI_SET_IP,
	TCC_CUI_SET_PID,
	TCC_CUI_SET_HANDOVER,
	TCC_CUI_SET_SUBTITLE,
	TCC_CUI_RELEASE,
	TCC_CUI_DATABASE,
	TCC_CUI_SET_EPG,
	TCC_CUI_EPG_SEARCH,
	TCC_CUI_EPG_SEARCH_CANCEL,
	TCC_CUI_SURFACE,
	TCC_CUI_SET_VOLUME,
	TCC_CUI_GET_SUBTITLE_LANG_INFO,
	TCC_CUI_SET_AUDIO_DUALMONO,
	TCC_CUI_SET_PARENTALRATE,
	TCC_CUI_SET_AREA,
	TCC_CUI_SET_PRESET,
	TCC_CUI_SET_PRESET_MODE,
	TCC_CUI_BACKGORUND,
	TCC_CUI_GET_CHANNEL_INFO,
	TCC_CUI_GET_SIGNAL_STRENGTH,
	TCC_CUI_REQ_EPG_UPDATE,
	TCC_CUI_REQ_SC_INFO,
	TCC_CUI_REQ_TRMP_INFO,
	TCC_CUI_SET_FREQ_BAND,
	TCC_CUI_SET_FIELD_LOG,
	TCC_CUI_GET_COPYCONTROL_INFO,
	TCC_CUI_SET_NOTIFY_DETECT_SECTION,
	TCC_CUI_SEARCH_AND_SET,
	TCC_CUI_GET_DATETIME,
	TCC_CUI_SET_AUDIO,
	TCC_CUI_SET_VIDEO,
	TCC_CUI_SET_AV_SYNC,
	TCC_CUI_SET_CAPTURE,
	TCC_CUI_SET_RECORD,
	TCC_CUI_PLAY_SUBTITLE,
	TCC_CUI_PLAY_SUPERIMPOSE,
	TCC_CUI_PLAY_PNG,
	TCC_CUI_SET_GROUP,
	TCC_CUI_SET_USERDATA,
	TCC_CUI_GET_USERDATA,
	TCC_CUI_GET_STATE,
	TCC_CUI_SET_DATASERVICE_START,
	TCC_CUI_CUSTOM_FILTER,
	TCC_CUI_CUSTOM_RELAYSEARCH,	
	TCC_CUI_EWS,
	TCC_CUI_EWS_WITHCHNUM,
	TCC_CUI_START_LOCALPLAY,
	TCC_CUI_STOP_LOCALPLAY,
	TCC_CUI_SET_LOCALPLAY,
	TCC_CUI_GET_LOCALPLAY,
	TCC_CUI_SET_SEAMLESS_SWITCH_COMPENSATION,
	TCC_CUI_GET_SEAMLESSVALUE,
	TCC_CUI_DS_GET_STC,
	TCC_CUI_DS_GET_SERVICE_TIME,
	TCC_CUI_DS_GET_CONTENT_ID,
	TCC_CUI_DS_CHECK_EXIST_COMPONENTTAGINPMT,	
	TCC_CUI_DS_SET_VIDEO_BY_COMPONENTTAG,
	TCC_CUI_DS_SET_AUDIO_BY_COMPONENTTAG,
#ifdef TCC_BROADCAST_TDMB_INCLUDE		
	TCC_CUI_TDMBSET,
#endif	
	TCC_CUI_ZOOM,
	TCC_CUI_ROTATION,
	TCC_CUI_PAN,	
	TCC_CUI_EFFECTOUT,	
	TCC_CUI_SHOW,
	TCC_CUI_MUTE_SET,
	TCC_CUI_CONFIG_SET,	
#ifdef VIQE_INCLUDE	
	TCC_CUI_VIQE_SHOW_STATE,
	TCC_CUI_VIQE_TURN,
	TCC_CUI_VIQE_CONF_DEINTL,
	TCC_CUI_VIQE_CONF_DNTS,
	TCC_CUI_VIQE_CONF_DNSP,
	TCC_CUI_VIQE_CONF_HPF,
	TCC_CUI_VIQE_CONF_HISEQ,
	TCC_CUI_VIQE_CONF_GAMUT,
	TCC_CUI_VIQE_CONF_RANDOM,
	TCC_CUI_VIQE_CONF_HALF_MODE,
	TCC_CUI_VIQE_CONF_HELP,
	TCC_CUI_VIQE_CONF_STEP,
	TCC_CUI_VIQE_CONF_HUFF,
#endif
#ifdef TCC_VIDEO_DECODING_INCLUDE
	TCC_CUI_VIDEO_START_MEASURE,
	TCC_CUI_VIDEO_CHANGE_SCREENMODE,
	TCC_CUI_VIDEO_SET_TVOUT_MODE,
#endif	
#ifdef TCC_SUBTITLE_INCLUDE	
	TCC_CUI_SUBTITLE_PLAY,
	TCC_CUI_SUBTITLE_PATH_PLAY,
	TCC_CUI_SUBTITLE_SINK_FF_TIME,
	TCC_CUI_SUBTITLE_SINK_REW_TIME,
	TCC_CUI_SUBTITLE_SHOW,
	TCC_CUI_SUBTITLE_HIDE,
	TCC_SUI_SUBTITLE_SELECT_LANGUAGE,
#endif	

	TCC_CUI_AUDIO_OUTPUTMODE,

#ifdef TCC_AUDIO_DECODING_INCLUDE
	TCC_CUI_AUDIO_SCAN,
#endif	
	TCC_CUI_VIDEO_SET_STEP_VALUE,
	TCC_CUI_VIDEO_SET_SLOW_VALUE,
	TCC_CUI_VIDEO_SET_SCAN,
	TCC_CUI_THUMB,
	TCC_CUI_VIDEO_SET_HDMI_VALUE,
	TCC_CUI_AUDIO_SET_DYNAMIC_RANGE,	
	TCC_CUI_SPDIFSET,
	TCC_CUI_LCDSET,
	TCC_CUI_CORECLOCKSET,
	TCC_CUI_MEMORYCLOCKSET,
	TCC_CUI_FGIOCLOCKSET,
	TCC_CUI_FVBUSCLOCKSET,
	TCC_CUI_FCODECCLOCKSET,
	TCC_CUI_FDDICLOCKSET,
	TCC_CUI_FSMUCLOCKSET,
	TCC_CUI_FGRPCLOCKSET,
	

#ifdef TCC_AUDIO_DECODING_INCLUDE
	TCC_CUI_AUDIO_START_MEASURE,
#endif

#ifdef TCC_CUI_AGING_TEST_INCLUDE
	TCC_CUI_AGING_TEST,
#endif

	TCC_CUI_AUDIO_SET_EXTRA,
	TCC_CUI_VIDEO_SET_DEST_RESOLUTION,
	TCC_CUI_VIDEO_ZOOM,

	TCC_CUI_AUDIO_LANGUAGE,
	TCC_CUI_HDMI_AUDIO_PATH_SET,
	TCC_CUI_SHOW_STREAM,
	TCC_CUI_SHOW_STREAM_LIST,
	TCC_CUI_SELECT_STREAM,
	TCC_CUI_CHANGE_AUDIO_STREAM,
	TCC_CUI_CHANGE_SUBTITLE_STREAM,
	TCC_CUI_CHANGE_SUBTITLE_PGS_STREAM,

	TCC_CUI_CAMERA_OMX_TEST,
	TCC_CUI_CAMERA_PREVIEW,
	TCC_CUI_CAMERA_RECODING,

	TCC_CUI_CAPTURE_SCREEN,
#ifdef TCC_BROADCAST_ATSC_INCLUDE
	TCC_CUI_SERVICE_STOP,
	TCC_CUI_ATSC_INDI_TEST,
	TCC_CUI_ATSC_CHANGE_INPUT_SOURCE,
	TCC_CUI_ATSC_SET_ASPECT_RATIO,
	TCC_CUI_ATSC_SET_AUDIO_LANGUAGE,
	TCC_CUI_ATSC_ENABLE_CAPTION,
	#ifdef TS_RECORD_ENABLE
	TCC_CUI_ATSC_REC_START,
	TCC_CUI_ATSC_REC_STOP,
	#endif
#ifdef TCC_BROADCAST_ATSC_CUITEST_INCLUDE
	TCC_CUI_ATSC_NEXT,
	TCC_CUI_ATSC_PREV,
#endif
#endif

#ifdef TCC_BROADCAST_DVBH_INCLUDE
	TCC_CUI_DVBH_ADD_IP, 
	TCC_CUI_DVBH_REMOVE_IP, 
#endif

#ifdef TCC_MEMORY_DEBUG	
	TCC_CUI_MEMORY_INFO,
#endif

	TCC_CUI_ENABLE_DUAL_CHANNEL,
	TCC_CUI_SET_DUAL_CHANNEL,
	TCC_CUI_GET_STRENGTH,
	TCC_CUI_SET_VIDEO_OUTPUT_PATH,
	TCC_CUI_GET_BCAS_CARD_INFO,
	TCC_CUI_SET_CHANNEL_DB_INDEX,
	TCC_CUI_SET_SCREEN_MODE,
	TCC_CUI_SET_AUDIO_TRACK,
	TCC_CUI_SET_AUDIO_MODE,
	TCC_CUI_SET_AUDIO_VOLUME,
	TCC_CUI_SET_AUDIO_MUTE,
	TCC_CUI_SET_PARENTAL_LOCK,
	TCC_CUI_SET_AGE_RATE_FOR_PARENTAL_LOCK,
	TCC_CUI_CLEAR_FB,
	TCC_CUI_HOOK_MALLOC_INFO,
#ifdef LOG_SIGNAL_STATUS
	TCC_CUI_LOG_SIGNAL,
#endif	
	TCC_CUI_VDTIME,
	TCC_CUI_PTSVIEW,
	TCC_CUI_NUM
};

enum	
{
	TCC_CUI_CONFIG_SHOW_ON= 0,
	TCC_CUI_CONFIG_SHOW_OFF,
	TCC_CUI_CONFIG_EFFECT_TRAN_NEXT,
	TCC_CUI_CONFIG_EFFECT_TRAN_PREV,
	TCC_CUI_CONFIG_EFFECT_TYPE_NEXT,
	TCC_CUI_CONFIG_EFFECT_TYPE_PREV,
	TCC_CUI_CONFIG_TRACK_RANDOM_ON,
	TCC_CUI_CONFIG_TRACK_RANDOM_OFF,
	TCC_CUI_CONFIG_TRACK_REPEAT_ON,
	TCC_CUI_CONFIG_TRACK_REPEAT_OFF,
	TCC_CUI_CONFIG_FRAME_MODE_NEXT,
	TCC_CUI_CONFIG_FRAME_MODE_PREV	
};

typedef	struct	TCC_CUI_structure
{

	int 	Fd ;
	struct pollfd Poll;
	int	Poll_Timeout;
	int	Cmd;
	char File_path[TCC_FILE_PATH_MAX];
	char Dir_path[TCC_FILE_PATH_MAX];
	char Play_Dir[TCC_FILE_PATH_MAX];
	int	Index;
	int	Time;
	int	Level;
	int Scantype;
	int	Input;
	int	Bitrate;
	int	Codec;
	int	Freq;
	int	Mode;
	int	Countrycode;
	int service_id;
	int Pmt_Id;

#ifdef TCC_BROADCAST_TDMB_INCLUDE
	int Serveic_ID;
	int Channel_ID;
	int Channel_TYPE;
#endif

#ifdef TCC_VIDEO_DECODING_INCLUDE
#ifdef TCC_SUBTITLE_INCLUDE
	char Subtitle_path[TCC_FILE_PATH_MAX];
#endif
	int scan_val;
	int	seek_mode;
#endif
#ifdef TCC_SUBTITLE_INCLUDE
	int	Subtitle_OnOff;
#endif
	int step_on_off;
	int slow;
	float scan_value;

//add by Robert,for image thumb display
	char File_Outpath[TCC_FILE_PATH_MAX];
	int wd;
	int ht;
	int rot;	
	int Thumb_Addr;

	int hdmi_width;
	int hdmi_height;
}TCC_CUI_t;

typedef	struct	TCC_CUI_CMD_structure
{
	char *Cmd;
	int	Cmd_Size;
	int  	Index;
}TCC_CUI_CMD_t;

enum	
{
	PANSCAN_UP = 0,        
	PANSCAN_DOWN,
	PANSCAN_LEFT,
	PANSCAN_RIGHT
};

/******************************************************************************
* globals
******************************************************************************/

/******************************************************************************
* locals
******************************************************************************/

int TCC_CUI_Parse_File_Path(char* Buffer,char *File_Path);
int TCC_CUI_Parse_Index(char* Buffer,int  *Index);
int TCC_CUI_Parse_Time(char* Buffer,int  *Time);
int TCC_CUI_Parse_Level(char* Buffer,int  *Level);
int TCC_CUI_Parse_Mode(char* Buffer,int  *Mode);
int TCC_CUI_Parse_Path(char* Buffer,char *Path);
int TCC_CUI_Parse_Add(char* Buffer,char *Path);
int TCC_CUI_Parse_Del(char* Buffer,char *Path);
int TCC_CUI_Parse_Name(char* Buffer,char *Name);
int TCC_CUI_Parse_Codec(char* Buffer,int  *Codec);
int TCC_CUI_Parse_Input(char* Buffer,int  *Input);
int TCC_CUI_Parse_Bitrate(char* Buffer,int  *Bitrate);
int TCC_CUI_Get_Mute_OnOff(char *buffer);
int TCC_CUI_Parse_OnOff(char* Buffer,int  *OnOff);
int TCC_CUI_Parse_SPDIF(char* Buffer,int  *spdif_mode);
int TCC_CUI_Parse_CLOCK(char* Buffer,int  *clock_index);
int TCC_CUI_Parse_DestResoltuion(char* Buffer, int *startX, int *startY, int  *width, int *height);
int TCC_CUI_Parse_Audio_Lang(char* Buffer,int  *mode);
int TCC_CUI_Parse_HDMI_Audio_Path(char* Buffer,int  *mode);
int TCC_CUI_Parse_Motion_Detection_Param(char* Buffer, char *Header, int  *ret_data);
int TCC_CUI_Parse_FrameRate(char* Buffer,int  *Bitrate);
void TCC_CUI_Set_CORECLOCK(char* buffer);
void TCC_CUI_Set_MEMORYCLOCK(char* buffer);
void TCC_CUI_Set_FGIOCLOCK(char* buffer);
void TCC_CUI_Set_FVBUSCLOCK(char* buffer);
void TCC_CUI_Set_FCODECCLOCK(char* buffer);
void TCC_CUI_Set_FDDICLOCK(char* buffer);
void TCC_CUI_Set_FSMUCLOCK(char* buffer);
void TCC_CUI_Set_FGRPCLOCK(char* buffer);
void TCC_CUI_Display_Error_Message(int Error);

#endif

