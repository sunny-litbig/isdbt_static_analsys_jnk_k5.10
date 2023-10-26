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
/******************************************************************************
* include
******************************************************************************/
#include <pthread.h>
#include "tcc_message.h"
//#include "tcc_semaphore.h"
#include "tcc_cui.h"
//#include "tcc_dxb_isdbtfseg_cui.h"
//#include "tcc_dxb_isdbtfseg_ui.h"

/******************************************************************************
* Defines
******************************************************************************/


/******************************************************************************
* locals
******************************************************************************/
static TCC_CUI_t Tcc_ISDBT_Cui;
static TCC_CUI_CMD_t TCC_Cui_Isdbt_Cmd[] =
{
	{"help", 				4, 		TCC_CUI_HELP},
	{"exit", 				4, 		TCC_CUI_EXIT},
	{"scanstop", 			8, 		TCC_CUI_SCAN_STOP},
	{"scan", 				4, 		TCC_CUI_SCAN},
	{"setchnum",			8,		TCC_CUI_SET_WITHCHNUM},
	{"set", 				3, 		TCC_CUI_SET},
	{"playstop", 			8, 		TCC_CUI_STOP},
	{"dualset", 			7, 		TCC_CUI_SET_DUAL_CHANNEL},
	{"show", 				4, 		TCC_CUI_SHOW},
	{"gettime",				7, 		TCC_CUI_GET_DATETIME},
	{"audio",  				5,  	TCC_CUI_SET_AUDIO},
	{"video",  				5,  	TCC_CUI_SET_VIDEO},
	{"capture",  			7,  	TCC_CUI_SET_CAPTURE},
	{"handover",  			8,  	TCC_CUI_SET_HANDOVER},
	{"record",  			6,  	TCC_CUI_SET_RECORD},
	{"subtitle",  			8,  	TCC_CUI_PLAY_SUBTITLE},
	{"superimpose",  		11,  	TCC_CUI_PLAY_SUPERIMPOSE},
	{"png",  				3,  	TCC_CUI_PLAY_PNG},
	{"group",  				5,  	TCC_CUI_SET_GROUP},
	{"userdataset",  		11,  	TCC_CUI_SET_USERDATA},
	{"userdataget",  		11,  	TCC_CUI_GET_USERDATA},
	{"localplaystart",  	14,  	TCC_CUI_START_LOCALPLAY},
	{"localplaystop",  		13,  	TCC_CUI_STOP_LOCALPLAY},
	{"localplaycontrol",  	16,  	TCC_CUI_SET_LOCALPLAY},
	{"localplayinfo",  		13,  	TCC_CUI_GET_LOCALPLAY},
	{"start",  				5,  	TCC_CUI_START},
	{"release",  			7,  	TCC_CUI_RELEASE},
	{"database",  			8,  	TCC_CUI_DATABASE},
	{"epg", 				3,		TCC_CUI_SET_EPG},
	{"epgscanstart",  		12,  	TCC_CUI_EPG_SEARCH},
	{"epgscanstop",  		11,  	TCC_CUI_EPG_SEARCH_CANCEL},
	{"surface",  			7,  	TCC_CUI_SURFACE},
	{"volume",  			6,  	TCC_CUI_SET_VOLUME},
	{"subtlanginfo",  		12,  	TCC_CUI_GET_SUBTITLE_LANG_INFO},
	{"dualmono",  			8,  	TCC_CUI_SET_AUDIO_DUALMONO},
	{"parentalrate",  		12,  	TCC_CUI_SET_PARENTALRATE},
	{"areaset",  			7,  	TCC_CUI_SET_AREA},
	{"presetset",  			9,  	TCC_CUI_SET_PRESET},
	{"presetmode",  		10,  	TCC_CUI_SET_PRESET_MODE},
	{"background",  		10,  	TCC_CUI_BACKGORUND},
	{"channelinfo",  		11,  	TCC_CUI_GET_CHANNEL_INFO},
	{"signalstrength",  	14,  	TCC_CUI_GET_SIGNAL_STRENGTH},
	{"epgupdate",  			9,  	TCC_CUI_REQ_EPG_UPDATE},
	{"scinfo",  			6,  	TCC_CUI_REQ_SC_INFO},
	{"trmpinfo",  			8,  	TCC_CUI_REQ_TRMP_INFO},
	{"freqband",  			9,  	TCC_CUI_SET_FREQ_BAND},
	{"fieldlog",  			9,  	TCC_CUI_SET_FIELD_LOG},
	{"copycontrolinfo",  	15,  	TCC_CUI_GET_COPYCONTROL_INFO},
	{"detectsection",  		13,  	TCC_CUI_SET_NOTIFY_DETECT_SECTION},
	{"sset",  				4,  	TCC_CUI_SEARCH_AND_SET},
	{"getstate",			8,  	TCC_CUI_GET_STATE},
	{"dataservice",  		11,  	TCC_CUI_SET_DATASERVICE_START},
	{"customfilter",		12,  	TCC_CUI_CUSTOM_FILTER},
	{"customrelaysearch",	17,  	TCC_CUI_CUSTOM_RELAYSEARCH},
	{"ewschnum",			8,		TCC_CUI_EWS_WITHCHNUM},
	{"ews",					3,  	TCC_CUI_EWS},
	{"seamlesscomp",		12,		TCC_CUI_SET_SEAMLESS_SWITCH_COMPENSATION},
	{"seamlessval",         11,     TCC_CUI_GET_SEAMLESSVALUE},
	{"dsgetstc",			8,		TCC_CUI_DS_GET_STC},
	{"dsgetservicetime",	16,		TCC_CUI_DS_GET_SERVICE_TIME},
	{"dsgetcontentid",		14,		TCC_CUI_DS_GET_CONTENT_ID},
	{"dscheckcomptag",		14,		TCC_CUI_DS_CHECK_EXIST_COMPONENTTAGINPMT},
	{"dssetvideobycomptag",	19,		TCC_CUI_DS_SET_VIDEO_BY_COMPONENTTAG},
	{"dssetaudiobycomptag",	19,		TCC_CUI_DS_SET_AUDIO_BY_COMPONENTTAG},

	{0, 0, 0}
};

int TCC_DXB_ISDBTFSEG_CUI_ParseOption(char *pszBuffer,const char *pOptKeyword, int *pRetValue);
int TCC_DXB_ISDBTFSEG_CUI_ParseOption2(char *pszBuffer,const char *pOptKeyword, char *pRetStr);

/******************************************************************************
* CUI Process Functions
******************************************************************************/
static void TCC_DXB_ISDBTFSEG_CUI_Cmd_Help(void)
{
	printf("\n\n");
	printf("#########################[ Telechips DXB ISDBT-FULLSEG Player ]###############################################\n");
	printf("#                                                                                                            #\n");
	printf("# Display menu           : help                                                                              #\n");
	printf("# Exit program           : exit                                                                              #\n");
	printf("# System Prepare & Start : start -s [specificinfo] -c [countrycode]                                          #\n");
	printf("# System Stop            : release                                                                           #\n");
	printf("# Search                 : scan -t [type:0,1,2,4,5,6] -n [country:0,1] -a [areacode] -c [ch no] -o [opt]     #\n");
	printf("# Search Cancel          : scanstop                                                                          #\n");
	printf("# Channel set            : set -m [mainRowID] -s [subRowID] -a [AudIndex] -v [VidIndex] -n [DualAudMode]     #\n");
	printf("#                            -d [raw_w] -f [raw_h] -w [view_w] -h [view_h] -i [ch_index]                     #\n");
	printf("# Channel set with ch num : setchnum -c [channelNumber] -m [serviceID_Fseg] -s [serviceID_1seg]              #\n");
	printf("#                            -n [DualAudMode] -d [raw_w] -f [raw_h] -w [view_w] -h [view_h] -i [ch_index]    #\n");
	printf("# Channel Play stop      : playstop                                                                          #\n");
	printf("# Channel set dual-ch    : dualset -i [index(0,1)]                                                           #\n");
	printf("# Channel show lists     : show                                                                              #\n");
	printf("# Get Current Date&Time  : gettime                                                                           #\n");
	printf("# Audio Set              : audio -o [on(1),off(0)] -i [index(0,1,2)] -m [mode(0,1,2)] -c [close(0),open(1)]  #\n");
	printf("# Video Set              : video -o [on(1),off(0)] -i [index(0,1,2)] -m [mode(0,1,2)] -c [close(0),open(1)]  #\n");
	printf("# Subtitle Set           : subtitle -o [on(1),off(0)] -i [index(0,1,2)]                                      #\n");
	printf("# Superimpose Set        : superimpose -o [on(1),off(0)] -i [index(0,1,2)]                                   #\n");
	printf("# PNG Set                : png -o [on(1),off(0)] -i [index(0,1,2)]                                           #\n");
	printf("# Group Set              : group -i [index:0,1,2]                                                            #\n");
	printf("# Capture Screen         : capture -f [filepath]                                                             #\n");
	printf("# Handover               : handover -c [japan(0),brazil(1)]                                                  #\n");
	printf("# Record                 : record -s [start(1),stop(0)] -f [filepath]                                        #\n");
	printf("# Set User Data          : userdataset -c [countrycode] -r [regionid] -b [prefecturalbit] -a [areacode]      #\n");
	printf("#                                      -p [postalcode]                                                       #\n");
	printf("# Get User Data          : userdataget                                                                       #\n");
	printf("# DB Backup&Restore      : database -o [backup(0),restore(1)]                                                #\n");
	printf("# EPG Set                : epg -o [on(1),off(0)]                                                             #\n");
	printf("# EPG Search             : epgscanstart -c [channelnumber] -o [option]                                       #\n");
	printf("# EPG Search Stop        : epgscanstop                                                                       #\n");
	printf("# Surface Use & Release  : surface -o [release(1),use(0)] -a [arg]                                           #\n");
	printf("# Volume                 : volume -m [mainvolume] -l [leftvolume] -r [rightvolume]                           #\n");
	printf("# Get Subtitle Lang Info : subtlanginfo                                                                      #\n");
	printf("# Set Audio Dual Mono    : dualmono -s [select:0,1,2]                                                        #\n");
	printf("# Set Parental Rate      : parentalrate -a [ageindex:0,1,2,3,4]                                              #\n");
	printf("# Set Area               : areaset -a [areacode]                                                             #\n");
	printf("# Set Preset             : presetset -a [areacode]                                                           #\n");
	printf("# Set Preset Mode        : presetmode -p [presetmode:0,1,2]                                                  #\n");
	printf("# Background Start &stop : background -s [start(1),stop(0)]                                                  #\n");
	printf("# Get Channel Info       : channelinfo -c [channelnumber] -s [serviceid]                                     #\n");
	printf("# Get Signal Strength    : signalstrength                                                                    #\n");
	printf("# Request EPG Update     : epgupdate -d [dayoffset]                                                          #\n");
	printf("# Request SC Info        : scinfo -a [arg]                                                                   #\n");
	printf("# Request TRMP Info      : reqtrmpinfo                                                                       #\n");
	printf("# Set Frequency Band     : freqband -f [frequencyband]                                                       #\n");
	printf("# Set Field Log          : fieldlog -p [path] -o [onoff]                                                     #\n");
	printf("# Get CopyControl Info   : copycontrolinfo -s [serviceid]                                                    #\n");
	printf("# Set Notify Section     : detectsection -s [sectiontype(1,NIT)(2,PAT)(4,SDT)(8,PMT_main)(16,PMT_sub)]       #\n");
	printf("# Search And Set Channel : sset -c [countrycode] -n [channelnum] -t [tsid] -o [option]                       #\n");
	printf("#                               -a [audioindex] -v [videoindex] -m [audiomode]                               #\n");
	printf("#                               -d [raw_w] -f [raw_h] -w [view_w] -h [view_h] -i [ch_index]                  #\n");
	printf("# Get State              : getstate -o [video(0), search(1)]                                                 #\n");
	printf("# Set DataService Start  : dataservice                                                                       #\n");
	printf("# CustomFilter Start/Stop: customfilter -o [start(0), stop(1)] -p [PID] -t [table_id]                        #\n");
	printf("# Custom RelaySearch     : customrelaysearch -c [kind] -r [repeat] -s [s_ch] -e [e_ch] -t [tsid]             #\n");
	printf("# EWS Start/Clear        : ews -o [start(0), clear(1)] -m [mainRowID] -s [subRowID]                          #\n");
	printf("#                              -a [AudIndex] -v [VidIndex] -n [DualAudMode]                                  #\n");
	printf("#                              -d [raw_w] -f [raw_h] -w [view_w] -h [view_h] -i [ch_index]                   #\n");
	printf("# EWS Start with ch num  : ewschnum -o [start(0), clear(1)] -c [channelNumber]                               #\n");
	printf("#                                   -m [serviceID_Fseg] -s [serviceID_1seg] -n [DualAudMode]                 #\n");
	printf("#                                   -d [raw_w] -f [raw_h] -w [view_w] -h [view_h] -i [ch_index]              #\n");
	printf("# Set Compensation Seamless : seamlesscomp -o [onoff] -i [interval] -s [strength] -n[ntimes]                 #\n");
    printf("#			                  -r[range] -a[gapadjust] -b[gapadjust2] -m[multiplier]                          #\n");
	printf("# Get Seamless Value        : seamlessval                                                                    #\n");
	printf("# [DS] Get STC                   : dsgetstc                                                                  #\n");
	printf("# [DS] Get ServiceTime           : dsgetservicetime                                                          #\n");
	printf("# [DS] Get ContentID             : dsgetcontentid                                                            #\n");
	printf("# [DS] Check ComponentTag        : dscheckcomptag -t [component_tag]                                         #\n");
	printf("# [DS] Set VideoByComponentTag   : dssetvideobycomptag -t [component_tag]                                    #\n");
	printf("# [DS] Set AudioByComponentTag   : dssetaudiobycomptag -t [component_tag]                                    #\n");
	printf("##############################################################################################################\n");
	printf("\n");
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_Exit(void)
{
	printf("Application Exit\n");

	TCC_UI_DXB_ISDBTFSEG_Exit(MESSAGE_TYPE_CUI_TO_PROCESS);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_Search(char *pacBuffer)
{
	int ScanType;
	int CountryCode;
	int AreaCode;
	int ChannelNum;
	int Options;

	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-t", &ScanType);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-n", &CountryCode);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-a", &AreaCode);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-c", &ChannelNum);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-o", &Options);

	printf("[scan] ScanType=%d, CountryCode=%d, AreaCode=%d, ChannelNum=%d, Options=%d\n",
		ScanType, CountryCode, AreaCode, ChannelNum, Options);

	TCC_UI_DXB_ISDBTFSEG_Search(MESSAGE_TYPE_CUI_TO_PROCESS, ScanType, CountryCode, AreaCode, ChannelNum, Options);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_SearchCancel(void)
{
	printf("[scanstop]\n" );

	TCC_UI_DXB_ISDBTFSEG_SearchCancel(MESSAGE_TYPE_CUI_TO_PROCESS);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_SetChannel(char *pacBuffer)
{
	int mainRowID, subRowID, audioIndex, videoIndex, audioMode;
	int raw_w, raw_h, view_w, view_h, ch_index;

	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-m", &mainRowID);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-s", &subRowID);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-a", &audioIndex);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-v", &videoIndex);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-n", &audioMode);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-f", &raw_w);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-d", &raw_h);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-w", &view_w);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-h", &view_h);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-i", &ch_index);

	printf("[set] mainRowID=%d, subRowID=%d, audioIndex=%d, videoIndex=%d, audioMode=%d, ch_index=%d raw_w=%d raw_h=%d view_w=%d view_h=%d\n",
		mainRowID, subRowID, audioIndex, videoIndex, audioMode, ch_index, raw_w, raw_h, view_w, view_h);

	TCC_UI_DXB_ISDBTFSEG_SetChannel(MESSAGE_TYPE_CUI_TO_PROCESS, mainRowID, subRowID, audioIndex, videoIndex, audioMode, raw_w, raw_h, view_w, view_h, ch_index);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_SetChannel_withChNum(char *pacBuffer)
{
	int channelNumber, serviceID_Fseg, serviceID_1seg, audioMode;
	int raw_w, raw_h, view_w, view_h, ch_index;

	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-c", &channelNumber);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-m", &serviceID_Fseg);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-s", &serviceID_1seg);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-n", &audioMode);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-f", &raw_w);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-d", &raw_h);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-w", &view_w);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-h", &view_h);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-i", &ch_index);

	printf("[set with ch num] channelNumber=%d, serviceID_Fseg=0x%x, serviceID_1seg=0x%x, audioMode=%d, ch_index=%d raw_w=%d raw_h=%d view_w=%d view_h=%d\n",
		channelNumber, serviceID_Fseg, serviceID_1seg, audioMode, ch_index, raw_w, raw_h, view_w, view_h);

	TCC_UI_DXB_ISDBTFSEG_SetChannel_withChNum(MESSAGE_TYPE_CUI_TO_PROCESS, channelNumber, serviceID_Fseg, serviceID_1seg, audioMode, raw_w, raw_h, view_w, view_h, ch_index);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_Stop(void)
{
	printf("[playstop]\n" );

	TCC_UI_DXB_ISDBTFSEG_Stop(MESSAGE_TYPE_CUI_TO_PROCESS);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_SetDualChannel(char *pacBuffer)
{
	int index;

	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-i", &index);

	printf("[dualset] dualset=%d\n",index );

	TCC_UI_DXB_ISDBTFSEG_SetDualChannel(MESSAGE_TYPE_CUI_TO_PROCESS, index);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_Show(void)
{
	printf("[show]\n" );

	TCC_CUI_Display_Show_Channel_DB();
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_GetCurrentDateTime(void)
{
	int iMJD;
	int iHour;
	int iMin;
	int iSec;
	int iPolarity;
	int iHourOffset;
	int iMinOffset;

	printf("[gettime]\n" );

	TCC_UI_DXB_ISDBTFSEG_GetCurrentDateTime(MESSAGE_TYPE_CUI_TO_PROCESS, &iMJD, &iHour, &iMin, &iSec, &iPolarity, &iHourOffset, &iMinOffset);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_SetAudio(char *pacBuffer)
{
	int onoff, index, mode, close;

	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-o", &onoff))
		TCC_UI_DXB_ISDBTFSEG_SetAudioMuteOnOff(MESSAGE_TYPE_CUI_TO_PROCESS, onoff);
	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-i", &index))
		TCC_UI_DXB_ISDBTFSEG_SetAudio(MESSAGE_TYPE_CUI_TO_PROCESS, index);
	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-m", &mode))
		TCC_UI_DXB_ISDBTFSEG_SetAudioVideoSyncMode(MESSAGE_TYPE_CUI_TO_PROCESS, mode);
	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-c", &close))
		TCC_UI_DXB_ISDBTFSEG_SetAudioOnOff(MESSAGE_TYPE_CUI_TO_PROCESS, close);

	printf("[audio] onoff=%d index=%d mode=%d close=%d\n", onoff, index, mode, close);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_SetVideo(char *pacBuffer)
{
	int onoff, index, mode, close;

	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-o", &onoff))
		TCC_UI_DXB_ISDBTFSEG_SetVideoOutput(MESSAGE_TYPE_CUI_TO_PROCESS, onoff);
	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-i", &index))
		TCC_UI_DXB_ISDBTFSEG_SetVideo(MESSAGE_TYPE_CUI_TO_PROCESS, index);
	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-m", &mode))
		TCC_UI_DXB_ISDBTFSEG_SetAudioVideoSyncMode(MESSAGE_TYPE_CUI_TO_PROCESS, mode);
	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-c", &close))
		TCC_UI_DXB_ISDBTFSEG_SetVideoOnOff(MESSAGE_TYPE_CUI_TO_PROCESS, close);

	printf("[video] onoff=%d index=%d mode=%d close=%d\n", onoff, index, mode, close);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_SetSubtitle(char *pacBuffer)
{
	int onoff, index;

	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-o", &onoff))
		TCC_UI_DXB_ISDBTFSEG_PlaySubtitle(MESSAGE_TYPE_CUI_TO_PROCESS, onoff);
	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-i", &index))
		TCC_UI_DXB_ISDBTFSEG_SetSubtitle(MESSAGE_TYPE_CUI_TO_PROCESS, index);

	printf("[subtitle] onoff=%d index=%d\n", onoff, index);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_SetSuperimpose(char *pacBuffer)
{
	int onoff, index;

	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-o", &onoff))
		TCC_UI_DXB_ISDBTFSEG_PlaySuperimpose(MESSAGE_TYPE_CUI_TO_PROCESS, onoff);
	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-i", &index))
		TCC_UI_DXB_ISDBTFSEG_SetSuperimpose(MESSAGE_TYPE_CUI_TO_PROCESS, index);

	printf("[superimpose] onoff=%d index=%d\n", onoff, index);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_SetPng(char *pacBuffer)
{
	int onoff, index;

	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-o", &onoff))
		TCC_UI_DXB_ISDBTFSEG_PlayPng(MESSAGE_TYPE_CUI_TO_PROCESS, onoff);
	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-i", &index))
		TCC_UI_DXB_ISDBTFSEG_SetSuperimpose(MESSAGE_TYPE_CUI_TO_PROCESS, index);

	printf("[png] onoff=%d index=%d\n", onoff, index);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_Capture(char *pacBuffer)
{
	char filepath[128] = {0, };

	TCC_DXB_ISDBTFSEG_CUI_ParseOption2(pacBuffer, "-f", filepath);

	printf("[capture] filepath=%s\n", filepath);

	TCC_UI_DXB_ISDBTFSEG_SetCapture(MESSAGE_TYPE_CUI_TO_PROCESS, filepath);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_Handover(char *pacBuffer)
{
	int country_code;

	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-c", &country_code);

	printf("[handover] country_code=%d\n", country_code);

	TCC_UI_DXB_ISDBTFSEG_SetHandover(MESSAGE_TYPE_CUI_TO_PROCESS, country_code);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_SetGroup(char *pacBuffer)
{
	int index;

	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-i", &index))
		TCC_UI_DXB_ISDBTFSEG_SetGroup(MESSAGE_TYPE_CUI_TO_PROCESS, index);

	printf("[group]\n index=%d", index);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_SetUserData(char *pacBuffer)
{
	int countrycode;
	int regionid;
	int prefecturalbitorder;
	int areacode;
	char postalcode[16] = {0,}; // jini 15th : Uninitialized Variable

	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-c", &countrycode))
		TCC_UI_DXB_ISDBTFSEG_SetCountryCode(MESSAGE_TYPE_CUI_TO_PROCESS, countrycode);
	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-r", &regionid))
		TCC_UI_DXB_ISDBTFSEG_SetUserDataRegionID(MESSAGE_TYPE_CUI_TO_PROCESS, regionid);
	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-b", &prefecturalbitorder))
		TCC_UI_DXB_ISDBTFSEG_SetUserDataPrefecturalCode(MESSAGE_TYPE_CUI_TO_PROCESS, prefecturalbitorder);
	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-a", &areacode))
		TCC_UI_DXB_ISDBTFSEG_SetUserDataAreaCode(MESSAGE_TYPE_CUI_TO_PROCESS, areacode);
	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption2(pacBuffer, "-p", postalcode))
		TCC_UI_DXB_ISDBTFSEG_SetUserDataPostalCode(MESSAGE_TYPE_CUI_TO_PROCESS, postalcode);

	printf("[userdataset] countrycode=%d regionid=%d prefecturalbitorder=%d areacode=%d postalcode=%s\n",
	countrycode, regionid, prefecturalbitorder, areacode, postalcode);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_GetUserData(void)
{
	unsigned int *puiRegionID = NULL;
	unsigned long long *pullPrefecturalCode = NULL;
	unsigned int *puiAreaCode = NULL;
	char *pcPostalCode = NULL;

	printf("[userdataget]\n");

	TCC_UI_DXB_ISDBTFSEG_GetCountryCode(MESSAGE_TYPE_CUI_TO_PROCESS);
	TCC_UI_DXB_ISDBTFSEG_GetUserData(MESSAGE_TYPE_CUI_TO_PROCESS, puiRegionID, pullPrefecturalCode, puiAreaCode, pcPostalCode);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_Start(char *pacBuffer)
{
	int specificinfo = 0;
	int country_code = 0;

	//information about profile
	static const int ISDBT_SINFO_PROFILE_A				= 0x80000000; //b'31 - support full seg
	static const int ISDBT_SINFO_PROFILE_C				= 0x40000000; //b'30 - support 1seg

	//information about nation
	static const int ISDBT_SINFO_JAPAN					= 0x20000000; //b'29 - japan
	static const int ISDBT_SINFO_BRAZIL					= 0x10000000; //b'28 - brazil

	//information about CAS
	static const int ISDBT_SINFO_BCAS					= 0x08000000; //b'27 - BCAS
	static const int ISDBT_SINFO_TRMP					= 0x04000000; //b'26 - TRMP

	//reserved for ISDBT feature, Bit[25:24]
	static const int ISDBT_SINFO_ALSA_CLOSE				= 0x00800000; //b'23 - ALSA close state at startup (0:open, 1:close)

	//information about UI ON/OFF
	static const int ISDBT_SINFO_RECORD					= 0x00400000; //b'22 - record
	static const int ISDBT_SINFO_BOOKRECORD				= 0x00200000; //b'21 - book-record
	static const int ISDBT_SINFO_TIMESHIFT				= 0x00100000; //b'20 - time-shift
	static const int ISDBT_SINFO_DUALDECODE				= 0x00080000; //b'19 - dual-decode

	//Special function
	static const int ISDBT_SINFO_EVENTRELAY				= 0x00040000; //b'18 - Event Relay
	static const int ISDBT_SINFO_MVTV					= 0x00020000; //b'17 - MVTV
	static const int ISDBT_SINFO_CHECK_SERVICEID		= 0x00010000; //b'16 - check of the service ID of broadcast
	static const int ISDBT_SINFO_CHANNEL_DBBACKUP		= 0x00008000; //b'15 - Channel db Backupt
	static const int ISDBT_SINFO_SPECIALSERVICE			= 0x00004000; //b'14 - special service
	static const int ISDBT_SINFO_DATASERVICE			= 0x00002000; //b'13 - data service
	static const int ISDBT_SINFO_SKIP_SDT_INSCAN		= 0x00001000; //b'12 - skip SDT While scanning channels

	//primary service ON/OFF, Bit[11]
	static const int ISDBT_SINFO_PRIMARYSERVICE       = 0x00000800;		//b'11 - primary service ON/OFF

	//audio only mode, Bit[10]

	//information about log
	static const int ISDBT_SINFO_ALL_ON_LOG				= 0x00000000; //b'8~9, 0 - Turn on all log
	static const int ISDBT_SINFO_IWE_LOG				= 0x00000100; //b'8~9, 1 - Turn on info, warn, error logs
	static const int ISDBT_SINFO_E_LOG					= 0x00000200; //b'8~9, 2 - Turn on error log
	static const int ISDBT_SINFO_ALL_OFF_LOG			= 0x00000300; //b'8~9, 3 - Turn off all log

	//Audio Only mode
	static const int ISDBT_SINFO_AUDIO_ONLY_MODE		= 0x00000400; //b'10 - DTV Audio only mode

	//information about Decrypt
	static const int ISDBT_DECRYPT_TCC353x				= 0x00000040;
	static const int ISDBT_DECRYPT_HWDEMUX				= 0x00000080;
	static const int ISDBT_DECRYPT_ACPU					= 0x000000C0;
	static const int ISDBT_DECRYPT_MASK					= 0x000000C0;

	//information about baseband
	static const int ISDBT_BASEBAND_TCC351x_CSPI_STS	= 0x00000001;
	static const int ISDBT_BASEBAND_TCC351x_I2C_STS		= 0x00000003;
	static const int ISDBT_BASEBAND_TCC351x_I2C_SPIMS	= 0x00000005; //reserved
	static const int ISDBT_BASEBAND_TCC353X_CSPI_STS	= 0x00000008;
	static const int ISDBT_BASEBAND_TCC353X_I2C_STS		= 0x00000009;
	static const int ISDBT_BASEBAND_MASK				= 0x0000003F;

	//compress expression of specification
#if defined(TRMP_ENABLE)
	static const int ISDBT_SINFO_MID_JAPAN				= (ISDBT_SINFO_JAPAN|ISDBT_SINFO_TRMP|ISDBT_SINFO_DUALDECODE|ISDBT_SINFO_SPECIALSERVICE|ISDBT_SINFO_DATASERVICE|ISDBT_SINFO_ALL_OFF_LOG);
#else
	static const int ISDBT_SINFO_MID_JAPAN              = (ISDBT_SINFO_JAPAN|ISDBT_SINFO_BCAS|ISDBT_SINFO_DUALDECODE|ISDBT_SINFO_SPECIALSERVICE|ISDBT_SINFO_DATASERVICE);
#endif
	//static const int ISDBT_SINFO_MID_JAPAN				= (ISDBT_SINFO_JAPAN|ISDBT_SINFO_TRMP|ISDBT_SINFO_DUALDECODE|ISDBT_SINFO_SPECIALSERVICE|ISDBT_SINFO_DATASERVICE);
	static const int ISDBT_SINFO_MID_BRAZIL				= (ISDBT_SINFO_BRAZIL|ISDBT_SINFO_RECORD|ISDBT_SINFO_DUALDECODE);
	static const int ISDBT_SINFO_STB_JAPAN				= (ISDBT_SINFO_JAPAN);
	static const int ISDBT_SINFO_STB_BRAZIL				= (ISDBT_SINFO_BRAZIL|ISDBT_SINFO_RECORD|ISDBT_SINFO_BOOKRECORD|ISDBT_SINFO_TIMESHIFT);

	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-s", &specificinfo)) {
		TCC_UI_DXB_ISDBTFSEG_Prepare(MESSAGE_TYPE_CUI_TO_PROCESS, specificinfo);
		if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-c", &country_code))
			TCC_UI_DXB_ISDBTFSEG_Start(MESSAGE_TYPE_CUI_TO_PROCESS, country_code);
	}
	else {
		specificinfo = (ISDBT_SINFO_PROFILE_A|ISDBT_SINFO_PROFILE_C|ISDBT_SINFO_MID_JAPAN|ISDBT_BASEBAND_TCC353X_I2C_STS|ISDBT_DECRYPT_HWDEMUX);
		TCC_UI_DXB_ISDBTFSEG_Prepare(MESSAGE_TYPE_CUI_TO_PROCESS, specificinfo);
		TCC_UI_DXB_ISDBTFSEG_Start(MESSAGE_TYPE_CUI_TO_PROCESS, specificinfo&ISDBT_SINFO_JAPAN?0:1);
	}

	printf("[start] specificinfo=%d country_code=%d\n",
		specificinfo, country_code);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_Release(void)
{
	printf("[release]\n");

	TCC_UI_DXB_ISDBTFSEG_Release(MESSAGE_TYPE_CUI_TO_PROCESS);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_Database(char *pacBuffer)
{
	int option;

	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-o", &option)) {
		if(option == 1)
			TCC_UI_DXB_ISDBTFSEG_ChannelDBRestoration(MESSAGE_TYPE_CUI_TO_PROCESS);
		else
			TCC_UI_DXB_ISDBTFSEG_ChannelBackUp(MESSAGE_TYPE_CUI_TO_PROCESS);
	}

	printf("[database] option=%d\n", option);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_SetEPG(char *pacBuffer)
{
	int option;

	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-o", &option);

	printf("[epg] option=%d", option);

	TCC_UI_DXB_ISDBTFSEG_SetEPG(MESSAGE_TYPE_CUI_TO_PROCESS, option);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_EPGSearch(char *pacBuffer)
{
	int channel_num, option;

	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-c", &channel_num);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-o", &option);

	printf("[epgscan]\n channel_num=%d option=%d", channel_num, option);

	TCC_UI_DXB_ISDBTFSEG_EPGSearch(MESSAGE_TYPE_CUI_TO_PROCESS, channel_num, option);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_EPGSearchCancel(void)
{
	printf("[epgscanstop]\n");

	TCC_UI_DXB_ISDBTFSEG_EPGSearchCancel(MESSAGE_TYPE_CUI_TO_PROCESS);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_Surface(char *pacBuffer)
{
	int option = 0;
	int arg = 0;

	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-o", &option)) {
		if(option == 0) {
			if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-a", &arg))
				TCC_UI_DXB_ISDBTFSEG_UseSurface(MESSAGE_TYPE_CUI_TO_PROCESS, arg);
		}
		else
			TCC_UI_DXB_ISDBTFSEG_ReleaseSurface(MESSAGE_TYPE_CUI_TO_PROCESS);
	}


	printf("[surface] option=%d arg=%d\n",
		option, arg);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_SetVolume(char *pacBuffer)
{
	int volume = 0;
	int leftvolume = 0;
	int rightvolume = 0;

	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-m", &volume))
		TCC_UI_DXB_ISDBTFSEG_SetVolume(MESSAGE_TYPE_CUI_TO_PROCESS, volume);
	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-l", &leftvolume)) {
		if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-r", &rightvolume)) {
			TCC_UI_DXB_ISDBTFSEG_SetVolumeLR(MESSAGE_TYPE_CUI_TO_PROCESS, leftvolume, rightvolume);
		}
	}

	printf("[volume] volume=%d leftvolume=%d rightvolume=%d\n",
		volume, leftvolume, rightvolume);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_GetSubtitleLangInfo(void)
{
	int *piSubtitleLangNum = NULL;
	int *piSuperimposeLangNum = NULL;
	unsigned int *puiSubtitleLang1 = NULL;
	unsigned int *puiSubtitleLang2 = NULL;
	unsigned int *puiSuperimposeLang1 = NULL;
	unsigned int *puiSuperimposeLang2 = NULL;

	printf("[subtitlelanginfo]\n");

	TCC_UI_DXB_ISDBTFSEG_GetSubtitleLangInfo(MESSAGE_TYPE_CUI_TO_PROCESS, piSubtitleLangNum, piSuperimposeLangNum, puiSubtitleLang1, puiSubtitleLang2, puiSuperimposeLang1, puiSuperimposeLang2);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_SetAudioDualMono(char *pacBuffer)
{
	int audio_sel;

	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-s", &audio_sel))
		TCC_UI_DXB_ISDBTFSEG_SetAudioDualMono(MESSAGE_TYPE_CUI_TO_PROCESS, audio_sel);

	printf("[audiodualmono] select=%d\n", audio_sel);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_SetParentalRate(char *pacBuffer)
{
	int age;

	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-a", &age))
		TCC_UI_DXB_ISDBTFSEG_SetParentalRate(MESSAGE_TYPE_CUI_TO_PROCESS, age);

	printf("[parentalrate] age=%d\n", age);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_SetArea(char *pacBuffer)
{
	int area_code;

	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-a", &area_code))
		TCC_UI_DXB_ISDBTFSEG_SetArea(MESSAGE_TYPE_CUI_TO_PROCESS, area_code);

	printf("[area] area_code=%d\n", area_code);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_SetPreset(char *pacBuffer)
{
	int area_code;

	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-a", &area_code))
		TCC_UI_DXB_ISDBTFSEG_SetPreset(MESSAGE_TYPE_CUI_TO_PROCESS, area_code);

	printf("[preset] area_code=%d\n", area_code);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_SetPresetMode(char *pacBuffer)
{
	int preset_mode;

	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-p", &preset_mode))
		TCC_UI_DXB_ISDBTFSEG_SetPresetMode(MESSAGE_TYPE_CUI_TO_PROCESS, preset_mode);

	printf("[presetmode] presetmode=%d\n", preset_mode);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_GetChannelInfo(char *pacBuffer)
{
	int iChannelNumber = 0;
	int iServiceID = 0;
	int *piRemoconID = NULL;
	int *piThreeDigitNumber = NULL;
	int *piServiceType = NULL;
	unsigned short *pusChannelName = NULL;
	int *piChannelNameLen = NULL;
	int *piTotalAudioPID = NULL;
	int *piTotalVideoPID = NULL;
	int *piTotalSubtitleLang = NULL;
	int *piAudioMode = NULL;
	int *piVideoMode = NULL;
	int *piAudioLang1 = NULL;
	int *piAudioLang2 = NULL;
	int *piAudioComponentTag = NULL;
	int *piVideoComponentTag = NULL;
	int *piStartMJD = NULL;
	int *piStartHH = NULL;
	int *piStartMM = NULL;
	int *piStartSS = NULL;
	int *piDurationHH = NULL;
	int *piDurationMM = NULL;
	int *piDurationSS = NULL;
	unsigned short *pusEvtName = NULL;
	int *piEvtNameLen = NULL;
	int *piLogoImageWidth = NULL;
	int *piLogoImageHeight = NULL;
	unsigned int *pusLogoImage = NULL;
	unsigned short *pusSimpleLogo = NULL;
	int *piSimpleLogoLength = NULL;
	int *piMVTVGroupType = NULL;

	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-c", &iChannelNumber);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-s", &iServiceID);

	printf("[getchannelinfo]\n");

	TCC_UI_DXB_ISDBTFSEG_GetChannelInfo(MESSAGE_TYPE_CUI_TO_PROCESS, iChannelNumber, iServiceID, piRemoconID, piThreeDigitNumber, piServiceType, pusChannelName, piChannelNameLen, piTotalAudioPID, piTotalVideoPID, piTotalSubtitleLang, piAudioMode, piVideoMode, piAudioLang1, piAudioLang2, piAudioComponentTag, piVideoComponentTag, piStartMJD, piStartHH, piStartMM, piStartSS, piDurationHH, piDurationMM, piDurationSS, pusEvtName, piEvtNameLen, piLogoImageWidth, piLogoImageHeight, pusLogoImage, pusSimpleLogo, piSimpleLogoLength, piMVTVGroupType);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_GetSignalStrength(void)
{
	printf("[getsignalstrength]\n");

	TCC_UI_DXB_ISDBTFSEG_GetSignalStrength(MESSAGE_TYPE_CUI_TO_PROCESS);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_RequestEPGUpdate(char *pacBuffer)
{
	int day_offset;

	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-d", &day_offset))
		TCC_UI_DXB_ISDBTFSEG_RequestEPGUpdate(MESSAGE_TYPE_CUI_TO_PROCESS, day_offset);

	printf("[epgupdate] day_offset=%d\n", day_offset);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_ReqSCInfo(char *pacBuffer)
{
	int arg;

	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-a", &arg))
		TCC_UI_DXB_ISDBTFSEG_ReqSCInfo(MESSAGE_TYPE_CUI_TO_PROCESS, arg);

	printf("[scinfo] arg=%d\n", arg);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_ReqTRMPInfo(void)
{
	unsigned char **ppucInfo = NULL;
	int *piInfoSize = NULL;

	printf("[reqtrmpinfo]\n");

	TCC_UI_DXB_ISDBTFSEG_ReqTRMPInfo(MESSAGE_TYPE_CUI_TO_PROCESS, ppucInfo, piInfoSize);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_SetFreqBand(char *pacBuffer)
{
	int freq_band;

	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-f", &freq_band))
		TCC_UI_DXB_ISDBTFSEG_SetFreqBand(MESSAGE_TYPE_CUI_TO_PROCESS, freq_band);

	printf("[freqband] frequencyband=%d\n", freq_band);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_SetFieldLog(char *pacBuffer)
{
	int onoff = 0;
	char filepath[128] = { 0, };

	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption2(pacBuffer, "-p", filepath)) {
		if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-o", &onoff))
			TCC_UI_DXB_ISDBTFSEG_SetFieldLog(MESSAGE_TYPE_CUI_TO_PROCESS, filepath, onoff);
	}

	printf("[fieldlog] path=%s onoff=%d\n", filepath, onoff);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_GetCopyControlInfo(char *pacBuffer)
{
	int usServiceID = 0;

	unsigned char *copy_restriction_mode = NULL;
	unsigned char *image_constraint_token = NULL;
	unsigned char *retention_mode = NULL;
	unsigned char *retention_state = NULL;
	unsigned char *encryption_mode = NULL;

	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-s", &usServiceID))
	{
		TCC_UI_DXB_ISDBTFSEG_GetDigitalCopyControlDescriptor(MESSAGE_TYPE_CUI_TO_PROCESS, usServiceID);

		TCC_UI_DXB_ISDBTFSEG_GetContentAvailabilityDescriptor(MESSAGE_TYPE_CUI_TO_PROCESS, usServiceID,
															copy_restriction_mode,
															image_constraint_token,
															retention_mode,
															retention_state,
															encryption_mode);
	}

	printf("[getcopycontrolinfo] serviceid=%d\n", usServiceID);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_SetNotifyDetectSection(char *pacBuffer)
{
	int sectionIDs;

	if(TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-s", &sectionIDs))
	{
		TCC_UI_DXB_ISDBTFSEG_SetNotifyDetectSection(MESSAGE_TYPE_CUI_TO_PROCESS, sectionIDs);
	}

	printf("[detectsection] sectionIDs=%d\n", sectionIDs);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_SearchAndSetChannel(char **pacBuffer)
{
	int country_code;
	int channel_num;
	int tsid;
	int options;
	int audioIndex;
	int videoIndex;
	int audioMode;
	int raw_w;
	int raw_h;
	int view_w;
	int view_h;
	int ch_index;

	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-c", &country_code);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-n", &channel_num);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-t", &tsid);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-o", &options);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-a", &audioIndex);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-v", &videoIndex);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-m", &audioMode);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-d", &raw_w);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-f", &raw_h);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-w", &view_w);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-h", &view_h);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-i", &ch_index);

	TCC_UI_DXB_ISDBTFSEG_SearchAndSetChannel(MESSAGE_TYPE_CUI_TO_PROCESS, country_code, channel_num, tsid, options, audioIndex, videoIndex, audioMode, raw_w, raw_h, view_w, view_h, ch_index);

	printf("[sset] country_code=%d channel_num=%d tsid=%d options=%d audioIndex=%d videoIndex=%d audioMode=%d raw_w=%d raw_h=%d view_w=%d view_h=%d ch_index=%d\n",
	country_code, channel_num, tsid, options, audioIndex, videoIndex, audioMode, raw_w, raw_h, view_w, view_h, ch_index);
}

static int TCC_DXB_ISDBTFSEG_CUI_Cmd_GetState(char *pacBuffer)
{
	int option;

	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-o", &option);

	if(option) {
		TCC_UI_DXB_ISDBTFSEG_GetSearchState(MESSAGE_TYPE_CUI_TO_PROCESS);
	} else {
		TCC_UI_DXB_ISDBTFSEG_GetVideoState(MESSAGE_TYPE_CUI_TO_PROCESS);
	}
	printf("[getstate] option=%d\n", option);

	return 0;
}

static int TCC_DXB_ISDBTFSEG_CUI_Cmd_SetDataServiceStart(void)
{
	TCC_UI_DXB_ISDBTFSEG_SetDataServiceStart(MESSAGE_TYPE_CUI_TO_PROCESS);
	printf("[dataservice]\n");

	return 0;
}

static int TCC_DXB_ISDBTFSEG_CUI_Cmd_CustomFilter(char *pacBuffer)
{
	int option;
	int pid;
	int table_id;

	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-o", &option);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-p", &pid);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-t", &table_id);

	if(option) {
		TCC_UI_DXB_ISDBTFSEG_CustomFilterStop(MESSAGE_TYPE_CUI_TO_PROCESS, pid, table_id);
	} else {
		TCC_UI_DXB_ISDBTFSEG_CustomFilterStart(MESSAGE_TYPE_CUI_TO_PROCESS, pid, table_id);
	}
	printf("[customfilter] option=%d pid=0x%X table_id=0x%X\n", option, pid, table_id);

	return 0;
}

static int TCC_DXB_ISDBTFSEG_CUI_Cmd_CustomRelaySearch(char *pacBuffer)
{
	int searchkind;
	int repeat;
	int Channel_start;
	int Channel_end;
	int tsid;

	int ChannelList[128];
	int tsidList[2];

	int i;

	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-c", &searchkind);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-r", &repeat);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-s", &Channel_start);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-e", &Channel_end);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-t", &tsid);

	if (Channel_start < 63 && Channel_end < 63 && Channel_end >= Channel_start && tsid > 0)
	{
		for (i = 0; i < 128; ++i)
			ChannelList[i] = -1;

		tsidList[0] = -1;
		tsidList[1] = -1;


		for (i = 0; i < (Channel_end-Channel_start+1); ++i)
		{
			ChannelList[i] = Channel_start+i;
		}

		tsidList[0] = tsid;

		TCC_UI_DXB_ISDBTFSEG_CustomRelayStationSearchRequest(MESSAGE_TYPE_CUI_TO_PROCESS, searchkind, ChannelList, tsidList, repeat);
	}
	else
	{
		printf("[CustomRelaySearch] wrong input (%d)(%d)(%d)(%d)(%d) \n", searchkind, repeat, Channel_start, Channel_end, tsid);
	}

	return 0;
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_EWS(char *pacBuffer)
{
	int mainRowID, subRowID, audioIndex, videoIndex, audioMode;
	int raw_w, raw_h, view_w, view_h, ch_index;
	int option;

	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-o", &option);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-m", &mainRowID);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-s", &subRowID);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-a", &audioIndex);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-v", &videoIndex);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-n", &audioMode);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-f", &raw_w);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-d", &raw_h);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-w", &view_w);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-h", &view_h);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-i", &ch_index);

	if(option) {
		TCC_UI_DXB_ISDBTFSEG_EWS_clear(MESSAGE_TYPE_CUI_TO_PROCESS);
		printf("[ews_clear]\n");
	} else {
		printf("[ews_start] mainRowID=%d, subRowID=%d, audioIndex=%d, videoIndex=%d, audioMode=%d, ch_index=%d raw_w=%d raw_h=%d view_w=%d view_h=%d\n",
		mainRowID, subRowID, audioIndex, videoIndex, audioMode, ch_index, raw_w, raw_h, view_w, view_h);
		TCC_UI_DXB_ISDBTFSEG_EWS_start(MESSAGE_TYPE_CUI_TO_PROCESS, mainRowID, subRowID, audioIndex, videoIndex, audioMode, raw_w, raw_h, view_w, view_h, ch_index);
	}
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_EWS_withChNum(char *pacBuffer)
{
	int channelNumber, serviceID_Fseg, serviceID_1seg, audioMode;
	int raw_w, raw_h, view_w, view_h, ch_index;
	int option;

	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-o", &option);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-c", &channelNumber);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-m", &serviceID_Fseg);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-s", &serviceID_1seg);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-n", &audioMode);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-f", &raw_w);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-d", &raw_h);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-w", &view_w);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-h", &view_h);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-i", &ch_index);

	if(option) {
		TCC_UI_DXB_ISDBTFSEG_EWS_clear(MESSAGE_TYPE_CUI_TO_PROCESS);
		printf("[ews_clear]\n");
	} else {
		printf("[ews_start with ch num] channelNumber=%d, serviceID_Fseg=0x%x, serviceID_1seg=0x%x, audioMode=%d, ch_index=%d raw_w=%d raw_h=%d view_w=%d view_h=%d\n",
		channelNumber, serviceID_Fseg, serviceID_1seg, audioMode, ch_index, raw_w, raw_h, view_w, view_h);
		TCC_UI_DXB_ISDBTFSEG_EWS_start_withChNum(MESSAGE_TYPE_CUI_TO_PROCESS, channelNumber, serviceID_Fseg, serviceID_1seg, audioMode, raw_w, raw_h, view_w, view_h, ch_index);
	}
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_Set_Seamless_Switch_Compensation(char *pacBuffer)
{
	int onoff;
	int interval;
	int strength;
	int ntimes;
	int range;
	int gapadjust;
	int gapadjust2;
	int muliplier;

	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-o", &onoff);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-i", &interval);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-s", &strength);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-n", &ntimes);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-r", &range);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-a", &gapadjust);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-b", &gapadjust2);
	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-m", &muliplier);
	TCC_UI_DXB_ISDBTFSEG_Set_Seamless_Switch_Compensation(MESSAGE_TYPE_CUI_TO_PROCESS, onoff, interval, strength, ntimes, range, gapadjust, gapadjust2, muliplier);
	printf("[Set_Seamless_Switch_Compensation] onoff=%d, interval=%d, strength=%d, ntimes=%d, range=%d, gapadjust=%d, gapadjust2=%d, multiplier=%d\n", onoff, interval, strength, ntimes, range, gapadjust, gapadjust2, muliplier);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_DS_Get_STC(char *pacBuffer)
{
	int hi_data;
	int lo_data;

	TCC_UI_DXB_ISDBTFSEG_GetSTC(MESSAGE_TYPE_CUI_TO_PROCESS, &hi_data, &lo_data);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_DS_Get_ServiceTime(char *pacBuffer)
{
	unsigned int year, month, day, hour, min, sec;

	TCC_UI_DXB_ISDBTFSEG_GetServiceTime(MESSAGE_TYPE_CUI_TO_PROCESS, &year, &month, &day, &hour, &min, &sec);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_DS_Get_ContentID(char *pacBuffer)
{
	int content_id;
	int associated_contents_flag;

	TCC_UI_DXB_ISDBTFSEG_GetContentID(MESSAGE_TYPE_CUI_TO_PROCESS, &content_id, &associated_contents_flag);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_DS_Check_ExistComponentTagInPMT(char *pacBuffer)
{
	int component_tag;

	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-t", &component_tag);
	TCC_UI_DXB_ISDBTFSEG_CheckExistComponentTagInPMT(MESSAGE_TYPE_CUI_TO_PROCESS, component_tag);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_DS_Set_Video_ByComponentTAG(char *pacBuffer)
{
	int component_tag;

	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-t", &component_tag);
	TCC_UI_DXB_ISDBTFSEG_SetVideoByComponentTag(MESSAGE_TYPE_CUI_TO_PROCESS, component_tag);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_DS_Set_Audio_ByComponentTAG(char *pacBuffer)
{
	int component_tag;

	TCC_DXB_ISDBTFSEG_CUI_ParseOption(pacBuffer, "-t", &component_tag);
	TCC_UI_DXB_ISDBTFSEG_SetAudioByComponentTag(MESSAGE_TYPE_CUI_TO_PROCESS, component_tag);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_Get_SeamlessValue(char *pacBuffer)
{
	int state;
	int cval;
	int pval;

	TCC_UI_DXB_ISDBTFSEG_GetSeamlessValue(MESSAGE_TYPE_CUI_TO_PROCESS, &state, &cval, &pval);
}

#if 0
static void TCC_DXB_ISDBTFSEG_CUI_Cmd_SetCustomTuner(void)
{
	void *pvArg;

	printf("[setcustomtuner]\n");

	TCC_UI_DXB_ISDBTFSEG_SetCustomTuner(MESSAGE_TYPE_CUI_TO_PROCESS, pvArg);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_GetCustomTuner(void)
{
	void *pvArg;

	printf("[getcustomtuner]\n");

	TCC_UI_DXB_ISDBTFSEG_GetCustomTuner(MESSAGE_TYPE_CUI_TO_PROCESS, pvArg);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_SetDeviceKeyUpdateFunction(void)
{
	unsigned char *(*pfUpdateDeviceKey)(unsigned char , unsigned int);

	printf("[devkeyupdatefunc]\n");

	TCC_UI_DXB_ISDBTFSEG_SetDeviceKeyUpdateFunction(MESSAGE_TYPE_CUI_TO_PROCESS, pfUpdateDeviceKey);
}

static void TCC_DXB_ISDBTFSEG_CUI_Cmd_SetListener(void)
{
	int (*pfListener)(int, int, int, void *);

	printf("[setlistener]\n");

	TCC_UI_DXB_ISDBTFSEG_SetListener(MESSAGE_TYPE_CUI_TO_PROCESS, pfListener);
}
#endif

/******************************************************************************
* Core Functions
******************************************************************************/
/******************************************************************************
*	FUNCTIONS			: TCC_DXB_ISDBTFSEG_CUI_Str2Dec
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
static int TCC_DXB_ISDBTFSEG_CUI_Str2Dec(char *szStr)
{
	if((szStr[0] == '0') && (szStr[1] == 'x' || szStr[1] == 'X'))
	{
		int iInt = 0;
		char cChar;

		szStr += 2;
		cChar = *szStr++;
		while((cChar >= '0' && cChar <= '9') || (cChar >= 'a' && cChar <= 'f') || (cChar >= 'A' && cChar <= 'F'))
		{
			iInt *= 0x10;
			if(cChar <= '9')
			{
				iInt += cChar-'0';
			}
			else if(cChar <= 'F')
			{
				iInt += cChar - ('A' - 10);
			}
			else //if(cChar <= 'f')
			{
				iInt += cChar - ('a' - 10);
			}
			//printf("[TCC_ATSC_CUI_Str2Dec] 0x%x\n", iInt);
			cChar = *szStr++;
		}
		return iInt;
	}
	else
	{
		return atoi(szStr);
	}
}

/******************************************************************************
*	FUNCTIONS			: TCC_DXB_ISDBTFSEG_CUI_ParseOption
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
int TCC_DXB_ISDBTFSEG_CUI_ParseOption(char *pszBuffer,const char *pOptKeyword, int *pRetValue)
{
	char *szToken;
	char *szPartStart;
	char szData[TCC_FILE_PATH_MAX];
	int	findoption = 0;

	if( pRetValue )
	{
		*pRetValue = 0;
	}
	memset(szData, 0, TCC_FILE_PATH_MAX);

	memcpy(szData, pszBuffer, strlen(pszBuffer));
	szToken = strtok(szData, " \n\r\n\t");

  	while(szToken)
	{
		szPartStart = strstr(szToken, pOptKeyword);

		if(szPartStart != NULL)
		{
			if( pRetValue )
			{
				*pRetValue = TCC_DXB_ISDBTFSEG_CUI_Str2Dec(szPartStart + 3);
			}
			findoption = 1;
			break;
		}
		szToken = strtok(NULL, " \n\r\n\t");
	}

	return findoption;
}

/******************************************************************************
*	FUNCTIONS			: TCC_DXB_ISDBTFSEG_CUI_ParseOption2
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
int TCC_DXB_ISDBTFSEG_CUI_ParseOption2(char *pszBuffer,const char *pOptKeyword, char *pRetStr)
{
	char *szToken;
	char *szPartStart;
	char szData[TCC_FILE_PATH_MAX];
	int	findoption = 0;

	memset(szData, 0, TCC_FILE_PATH_MAX);

	memcpy(szData, pszBuffer, strlen(pszBuffer));
	szToken = strtok(szData, " \n\r\n\t");

  	while(szToken)
	{
		szPartStart = strstr(szToken, pOptKeyword);

		if(szPartStart != NULL)
		{
			szToken = strtok(&szPartStart[3], " \n\r\n\t");
			if(pRetStr && szToken)
				strcpy(pRetStr, szToken);

			findoption = 1;
			break;
		}
		szToken = strtok(NULL, " \n\r\n\t");
	}

	return findoption;
}

/******************************************************************************
*	FUNCTIONS			: TCC_DXB_ISDBTFSEG_CUI_Parse_CMD
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
static int TCC_DXB_ISDBTFSEG_CUI_Parse_CMD(char *pacBuffer)
{
	int i;

	for(i = 0; i < TCC_CUI_NUM; i ++)
	{
		if(strncmp(pacBuffer, TCC_Cui_Isdbt_Cmd[i].Cmd, TCC_Cui_Isdbt_Cmd[i].Cmd_Size) == 0)
		{
			return TCC_Cui_Isdbt_Cmd[i].Index;
		}
	}

	return TCC_CUI_NULL;
}


/******************************************************************************
*	FUNCTIONS			: TCC_DXB_ISDBTFSEG_CUI_Message_Parser
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
int TCC_DXB_ISDBTFSEG_CUI_Message_Parser(char *buffer)
{
	Tcc_ISDBT_Cui.Cmd = TCC_DXB_ISDBTFSEG_CUI_Parse_CMD(buffer);

	switch (Tcc_ISDBT_Cui.Cmd)
	{
		case TCC_CUI_HELP:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_Help();
			break;

		case TCC_CUI_EXIT:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_Exit();
			break;

		case TCC_CUI_SCAN:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_Search(buffer);
			break;

		case TCC_CUI_SCAN_STOP:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_SearchCancel();
			break;

		case TCC_CUI_SET:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_SetChannel(buffer);
			break;

		case TCC_CUI_SET_WITHCHNUM:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_SetChannel_withChNum(buffer);
			break;

		case TCC_CUI_STOP:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_Stop();
			break;

		case TCC_CUI_SET_DUAL_CHANNEL:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_SetDualChannel(buffer);
			break;

		case TCC_CUI_SHOW:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_Show();
			break;

		case TCC_CUI_GET_DATETIME:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_GetCurrentDateTime();
			break;

		case TCC_CUI_SET_AUDIO:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_SetAudio(buffer);
			break;

		case TCC_CUI_SET_VIDEO:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_SetVideo(buffer);
			break;

		case TCC_CUI_SET_CAPTURE:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_Capture(buffer);
			break;

		case TCC_CUI_SET_HANDOVER:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_Handover(buffer);
			break;

		case TCC_CUI_PLAY_SUBTITLE:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_SetSubtitle(buffer);
			break;

		case TCC_CUI_PLAY_SUPERIMPOSE:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_SetSuperimpose(buffer);
			break;

		case TCC_CUI_PLAY_PNG:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_SetPng(buffer);
			break;

		case TCC_CUI_SET_GROUP:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_SetGroup(buffer);
			break;

		case TCC_CUI_SET_USERDATA:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_SetUserData(buffer);
			break;

		case TCC_CUI_GET_USERDATA:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_GetUserData();
			break;

		case TCC_CUI_START:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_Start(buffer);
			break;

		case TCC_CUI_RELEASE:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_Release();
			break;

		case TCC_CUI_DATABASE:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_Database(buffer);
			break;

		case TCC_CUI_SET_EPG:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_SetEPG(buffer);
			break;

		case TCC_CUI_EPG_SEARCH:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_EPGSearch(buffer);
			break;

		case TCC_CUI_EPG_SEARCH_CANCEL:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_EPGSearchCancel();
			break;

		case TCC_CUI_SURFACE:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_Surface(buffer);
			break;

		case TCC_CUI_SET_VOLUME:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_SetVolume(buffer);
			break;

		case TCC_CUI_GET_SUBTITLE_LANG_INFO:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_GetSubtitleLangInfo();
			break;

		case TCC_CUI_SET_AUDIO_DUALMONO:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_SetAudioDualMono(buffer);
			break;

		case TCC_CUI_SET_PARENTALRATE:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_SetParentalRate(buffer);
			break;

		case TCC_CUI_SET_AREA:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_SetArea(buffer);
			break;

		case TCC_CUI_SET_PRESET:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_SetPreset(buffer);
			break;

		case TCC_CUI_SET_PRESET_MODE:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_SetPresetMode(buffer);
			break;

		case TCC_CUI_GET_CHANNEL_INFO:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_GetChannelInfo(buffer);
			break;

		case TCC_CUI_GET_SIGNAL_STRENGTH:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_GetSignalStrength();
			break;

		case TCC_CUI_REQ_EPG_UPDATE:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_RequestEPGUpdate(buffer);
			break;

		case TCC_CUI_REQ_SC_INFO:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_ReqSCInfo(buffer);
			break;

		case TCC_CUI_REQ_TRMP_INFO:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_ReqTRMPInfo();
			break;

		case TCC_CUI_SET_FREQ_BAND:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_SetFreqBand(buffer);
			break;

		case TCC_CUI_SET_FIELD_LOG:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_SetFieldLog(buffer);
			break;

		case TCC_CUI_GET_COPYCONTROL_INFO:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_GetCopyControlInfo(buffer);
			break;
		case TCC_CUI_SET_NOTIFY_DETECT_SECTION:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_SetNotifyDetectSection(buffer);
			break;
		case TCC_CUI_SEARCH_AND_SET:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_SearchAndSetChannel(buffer);
			break;

		case TCC_CUI_GET_STATE:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_GetState(buffer);
			break;

		case TCC_CUI_SET_DATASERVICE_START:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_SetDataServiceStart();
			break;

		case TCC_CUI_CUSTOM_FILTER:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_CustomFilter(buffer);
			break;

		case TCC_CUI_CUSTOM_RELAYSEARCH:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_CustomRelaySearch(buffer);
			break;

		case TCC_CUI_EWS:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_EWS(buffer);
			break;
		case TCC_CUI_EWS_WITHCHNUM:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_EWS_withChNum(buffer);
			break;
		case TCC_CUI_SET_SEAMLESS_SWITCH_COMPENSATION:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_Set_Seamless_Switch_Compensation(buffer);
			break;
		case TCC_CUI_GET_SEAMLESSVALUE:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_Get_SeamlessValue(buffer);
			break;
		case TCC_CUI_DS_GET_STC:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_DS_Get_STC(buffer);
			break;
		case TCC_CUI_DS_GET_SERVICE_TIME:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_DS_Get_ServiceTime(buffer);
			break;
		case TCC_CUI_DS_GET_CONTENT_ID:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_DS_Get_ContentID(buffer);
			break;
		case TCC_CUI_DS_CHECK_EXIST_COMPONENTTAGINPMT:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_DS_Check_ExistComponentTagInPMT(buffer);
			break;
		case TCC_CUI_DS_SET_VIDEO_BY_COMPONENTTAG:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_DS_Set_Video_ByComponentTAG(buffer);
			break;
		case TCC_CUI_DS_SET_AUDIO_BY_COMPONENTTAG:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_DS_Set_Audio_ByComponentTAG(buffer);
			break;

 #if 0
		case TCC_CUI_SET_CUSTOM_TUNER:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_SetCustomTuner();
			break;

		case TCC_CUI_GET_CUSTOM_TUNER:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_GetCustomTuner();
			break;

		case TCC_CUI_GET_DCCD:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_GetDigitalCopyControlDescriptor();
			break;

		case TCC_CUI_SET_LISTENER:
			TCC_DXB_ISDBTFSEG_CUI_Cmd_SetListener();
			break;
#endif
		default:
			printf("\n\n############# Type 'help' ####################\n");
			return FALSE;
	}

	return TRUE;
}

/******************************************************************************
*	FUNCTIONS			: TCC_DXB_ISDBTFSEG_CUI_Init_Message
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
int TCC_DXB_ISDBTFSEG_CUI_Init_Message(void)
{
	Tcc_ISDBT_Cui.Cmd = TCC_CUI_NULL;
	memset(Tcc_ISDBT_Cui.File_path, 0, TCC_FILE_PATH_MAX);
	memset(Tcc_ISDBT_Cui.Dir_path, 0, TCC_FILE_PATH_MAX);
	Tcc_ISDBT_Cui.Time = 0;
	Tcc_ISDBT_Cui.Level = 90;
	Tcc_ISDBT_Cui.Input = 0;
	Tcc_ISDBT_Cui.Bitrate = 0;
	Tcc_ISDBT_Cui.Codec = 0;
	Tcc_ISDBT_Cui.Freq = 0;
	Tcc_ISDBT_Cui.Mode = 0;
	Tcc_ISDBT_Cui.Countrycode = 0;

	return 0;
}

/******************************************************************************
*	FUNCTIONS			: TCC_DXB_ISDBTFSEG_CUI_Init
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
int TCC_DXB_ISDBTFSEG_CUI_ProcMessage(void)
{
	int iRetValue;
	char acBuffer[TCC_FILE_PATH_MAX];

	iRetValue = poll(&Tcc_ISDBT_Cui.Poll, 1, Tcc_ISDBT_Cui.Poll_Timeout);
	if((iRetValue > 0) && (Tcc_ISDBT_Cui.Poll.revents & (POLLIN | POLLPRI)))
	{
		memset(acBuffer, 0, TCC_FILE_PATH_MAX);

		if((iRetValue = read(Tcc_ISDBT_Cui.Fd, &acBuffer, TCC_FILE_PATH_MAX)) > 0)
		{
			return TCC_DXB_ISDBTFSEG_CUI_Message_Parser(acBuffer);
		}
	}
	return 0;
}


void TCC_DXB_ISDBTFSEG_CUI_Init(void)
{
	TCC_DXB_ISDBTFSEG_CUI_Init_Message();

	memset(&Tcc_ISDBT_Cui, 0, sizeof(TCC_CUI_t));
	memset(&Tcc_ISDBT_Cui.Poll, 0, sizeof(struct pollfd));

	Tcc_ISDBT_Cui.Fd = open(TCC_CUI_DEVICE, O_RDWR | O_NDELAY);
	Tcc_ISDBT_Cui.Poll_Timeout = TCC_POLL_TIME_OUT;
	Tcc_ISDBT_Cui.Poll.fd = Tcc_ISDBT_Cui.Fd;
	Tcc_ISDBT_Cui.Poll.events = POLLIN | POLLPRI;
	Tcc_ISDBT_Cui.Poll.revents = 0;

	TCC_DXB_ISDBTFSEG_CUI_Cmd_Help();
	TCC_CUI_Display_Show_Channel_DB();
}

/******************************************************************************
*	FUNCTIONS			: TCC_DXB_ISDBTFSEG_CUI_DeInit
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_DXB_ISDBTFSEG_CUI_DeInit(void)
{
	close(Tcc_ISDBT_Cui.Fd);
}
