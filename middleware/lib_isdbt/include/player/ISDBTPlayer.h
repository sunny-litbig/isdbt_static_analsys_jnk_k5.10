/**
 * @file ISDBTPlayer.h
 * @brief 
 * @author Telechips & LitBig
 * @version 1.0
 * @date 2019-07-12
 */

/*******************************************************************************

*   FileName : ISDBTPlayer.h

*   Copyright (c) Telechips Inc.

*   Description : ISDBTPlayer.h

********************************************************************************
*
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

#ifndef LINUX_ISDBT_BPLAYER_H
#define LINUX_ISDBT_BPLAYER_H

#include <pthread.h>

//namespace linux
//{
/**
 * @brief 
 */
typedef struct
{
	unsigned char ucHour;
	unsigned char ucMinute;
	unsigned char ucSecond;
} TIME_STRUCT_TYPE;

typedef struct
{
	unsigned short uiMJD;
	TIME_STRUCT_TYPE stTime;
}  DATE_TIME_STRUCT_TYPE;

class DB_CHANNEL_Data
{
	public:
		int channelNumber;
		int countryCode;
		int uiVersionNum;
		int audioPID;
		int videoPID;
		int stPID;
		int siPID;
		int PMT_PID;
		int remoconID;
		int serviceType;
		int serviceID;
		int regionID;
		int threedigitNumber;
		int TStreamID;
		int berAVG;
		int preset;
		int networkID;
		int areaCode;
		short *channelName;
};

class DB_SERVICE_Data
{
	public:
		int uiPCRPID;
};

class DB_AUDIO_Data
{
	public:
		int uiServiceID;
		int uiCurrentChannelNumber;
		int uiCurrentCountryCode;
		int usNetworkID;
		int uiAudioPID;
		int uiAudioIsScrambling;
		int uiAudioStreamType;
		int uiAudioType;
		char *pucacLangCode;
};

class DB_SUBTITLE_Data
{
	public:
		int uiServiceID;
		int uiCurrentChannelNumber;
		int uiCurrentCountryCode;
		int uiSubtitlePID;
		char *pucacSubtLangCode;
		int uiSubtType;
		int uiCompositionPageID;
		int uiAncillaryPageID;
};

class DBInfoData
{
	public:
		DB_CHANNEL_Data dbCh;
		DB_SERVICE_Data dbService;
		DB_AUDIO_Data dbAudio[5];
		DB_SUBTITLE_Data dbSubtitle[10];
};

class RelayStationChData
{
	public:
		class RelayStationChData* pNext;

		int channelNumber;
		int countryCode;
		int PMT_PID;
		int remoconID;
		int serviceType;
		int serviceID;
		int regionID;
		int threedigitNumber;
		int TStreamID;
		int berAVG;
		char *channelName;
		char *TSName;
		int preset;
		int networkID;
		int areaCode;
		unsigned char *frequency;
};

class Channel
{
	public:
		int channelNumber;
		int countryCode;
		int audioPID;
		int videoPID;
		int serviceID;
		int channelFreq;
		char channelName[20];
};

class SubtitleDataLinux
{
	public:
		int internal_sound_subtitle_index;
		int mixed_subtitle_size;
		int mixed_subtitle_phy_addr;
};

class VideoDefinitionData
{
	public:
		int width;
		int height;
		int aspect_ratio;
		int frame_rate;
		int main_DecoderID;
		int sub_width;
		int sub_height;
		int sub_aspect_ratio;
		int sub_DecoderID;
		int DisplayID;
};


class EWSData
{
	public:
		int serviceID;
		int startendflag;
		int signaltype;
		int area_code_length;
		int localcode[256];
};

class SubDigitalCopyControlDescriptor
{
	public:
		class SubDigitalCopyControlDescriptor* pNext;

		unsigned char	component_tag;
		unsigned char	digital_recording_control_data;
		unsigned char	maximum_bitrate_flag;
		unsigned char	copy_control_type;
		unsigned char	APS_control_data;
		unsigned char	maximum_bitrate;
};

class DigitalCopyControlDescriptor
{
	public:
		unsigned char digital_recording_control_data;
		unsigned char maximum_bitrate_flag;
		unsigned char component_control_flag;
		unsigned char copy_control_type;
		unsigned char APS_control_data;
		unsigned char maximum_bitrate;

		SubDigitalCopyControlDescriptor* sub_info;
};

class ContentAvailabilityDescriptor
{
	public:
		unsigned char copy_restriction_mode;
		unsigned char image_constraint_token;
		unsigned char retention_mode;
		unsigned char retention_state;
		unsigned char encryption_mode;
};

class CustomSearchInfoData {
	public:
		int status;
		int ch;
		int strength;
		int tsid;
		int fullseg_id;
		int oneseg_id;
		int total_cnt;
		int all_id[32];
};

class AVIndexData {
	public:
		int channelNumber;
		int networkID;
		int serviceID;
		int serviceSubID;
		int audioIndex;
		int videoIndex;
};

class SetChannelData {
	public:
		int countryCode;
		int channelNumber;
		int mainRowID;
		int subRowID;
};

class SectionDataInfo {
	public:
		int network_id;
		int transport_stream_id;
		int DataServicePID;
		int auto_start_flag;
		int rawdatalen;
		unsigned char *rawdata;
};

class ISDBTPlayerListener
{
	public:
		virtual ~ISDBTPlayerListener() {};
		virtual void notify(int msg, int ext1, int ext2, void *obj) = 0;
};

class ISDBTPlayer
{
	public:
		enum event_type
		{
			/**
			 * @brief Do nothing
			 * @param msg 0
			 * @param ext1 not used
			 * @param ext2 not used
			 * @param obj not used
			 */
			EVENT_NOP = 0,

			/**
			 * @brief Called when at the end of execution of ISDBTPlayer::prepare.\n
			 *    It means the ISDBTPlayer is ready for a playback.
			 * @param msg 1
			 * @param ext1 not used
			 * @param ext2 not used
			 * @param obj not used
			 */
			EVENT_PREPARED = 1,

			/**
			 * @brief Called when channel search is completed.
			 * @param msg 2
			 * @param ext1 search_result\n
			 *    0      = success\n
			 *    1      = canceled\n
			 *    else   = failure. It depends on search type.\n
			 * @param ext2 not used
			 * @param obj not used
			 */
			EVENT_SEARCH_COMPLETE = 2,

			/**
			 * @brief Called when the channel information is updated. ISDBTx.db file should be updated.
			 * @param msg 3
			 * @param ext1 request_ch_update\n
			 *    0 = no need to update\n
			 *    1 = need to update.
			 * @param ext2 serviceID updated.
			 * @param obj not used
			 */
			EVENT_CHANNEL_UPDATE = 3,

			/**
			 * @brief Called to notify the current process percentage while searching.\n
			 *    Note that this event is issued only for Init scan, rescan, and region scan.\n
			 *    In case of manual scan, this event won't happen.
			 * @param msg 5
			 * @param ext1 search process percent(0~100)
			 * @param ext2 channel index.\n
			 *    In case of Japan, 0 means 473.143Mhz and 1 means 479.143Mhz ... 39 means 707.143Mhz.
			 * @param obj not used
			 */
			EVENT_SEARCH_PERCENT = 5,

			/**
			 * @brief Called when the start of video output.
			 * @param msg 6
			 * @param ext1 channel index.
			 * @param ext2 serviceID
			 * @param obj not used
			 */			
			EVENT_VIDEO_OUTPUT = 6,

			/**
			 * @brief Called when the subtitle updated.
			 * @param msg 9
			 * @param ext1 channel index.
			 * @param ext2 serviceID
			 * @param obj class SubtitleDataLinux.\n			 
     		 *	 internal_sound_subtitle_index.\n
			 *		 -1 = no built-in sound.\n
			 *		 0 ~ 15 = built-in sound designation.\n
			 *	 mixed_subtitle_size.\n
			 *		 - physical memory size for mixed subtitle raw data.\n
			 *	 mixed_subtitle_phy_addr.\n
			 *		 - physical memory address for mixed subtitle raw data.\n
			 * @param objsize sizeof(SubtitleDataLinux)
			 */
			EVENT_SUBTITLE_UPDATE			= 9,

			/**
			 * @brief Called when the video data information updated.
			 * @param msg 10
			 * @param ext1 channel index.
			 * @param ext2 serviceID
			 * @param obj class VideoDefinitionData
			 * @param objsize sizeof(VideoDefinitionData)
			 */	
			EVENT_VIDEODEFINITION_UPDATE	= 10,

			/**
			 * @brief Called when the db information updated for file play.
			 * @param msg 14
			 * @param ext1 not used
			 * @param ext2 not used
			 * @param obj class DBInfoData
			 * @param objsize sizeof(DBInfoData)
			 */	
			EVENT_DBINFO_UPDATE				= 14,

			/**
			 * @brief Called when the EPG information is updated. ISDBTEPGx.db file should be updated.
			 * @param msg 30
			 * @param ext1 serviceID updated.
			 * @param ext2 tableID updated.
			 * @param obj not used
			 */
			EVENT_EPG_UPDATE				= 30,			

			/**
			 * @brief Called when EWS information is found.
			 * @param msg 33
			 * @param ext1 StatOrStop.\n
			 *    0 : EWS stops.
			 *    1 : EWS starts.
			 * @param ext2 channel index.\n
			 *    In case of Japan, 0 means 473.143Mhz and 1 means 479.143Mhz ... 39 means 707.143Mhz.
			 * @param obj class EWSData
			 */
			EVENT_EMERGENCY_INFO			= 33,

			/**
			 * @brief Called while auto_scan.
			 * @param msg 36
			 * @param ext1 not used
			 * @param ext2 percent of progress(0~100)
			 * @param obj int[3]\n
			 *    pArg[0] not used\n
			 *    pArg[1] percent, same as ext2\n
			 *    pArg[2] channel index.\n
			 *        In case of Japan, 0 means 473.143Mhz and 1 means 479.143Mhz ... 39 means 707.143Mhz.
			 */
			EVENT_AUTOSEARCH_UPDATE			= 36,

			/**
			 * @brief Called when auto_scan is completed.
			 * @param msg 37
			 * @param ext1 result of auto_scan\n
			 *    1  = success\n
			 *    -1 = fail to find a channel.
			 * @param ext2 not used
			 * @param obj int[32+4]\n
			 *    pArg[0]    : result of auto_scan, same as ext1.\n
			 *    pArg[1]    : row_id(_id of ChannelDB) of fullseg service which can be selected as Main channel.\n
			 *    pArg[2]    : row_id of oneseg service which can be selected as Sub channel.\n
			 *    pArg[3]    : number of found service(max=32).\n
			 *    pArg[4~35] : array of row_id's which has found
			 */
			EVENT_AUTOSEARCH_DONE			= 37,

			/**
			 * @brief Called when descriptor is updated.\n
			 *    (usable only for digital_copy_control_descriptor and content_availability_descriptor)
			 * @param msg 38
			 * @param ext1 serviceID
			 * @param ext2 descriptor_tag of each descriptor
			 * @param obj class DigitalCopyControlDescriptor for digital_copy_control_descriptor\n
			 *    class ContentAvailabilityDescriptor for content_availability_descriptor
			 *    Caution : In case of digital_copy_control_descriptor, App must free obj(DigitalCopyControlDescriptor and SubDigitalCopyControlDescriptor).
			 */
			EVENT_DESC_UPDATE				= 38,

			/**
			 * @brief Called when Event Relay information is updated.
			 * @param msg 39
			 * @param ext1 channel index.\n
			 *    In case of Japan, 0 means 473.143Mhz and 1 means 479.143Mhz ... 39 means 707.143Mhz.
			 * @param ext2 not used
			 * @param obj int[4]\n
			 *    pArg[0] = serviceID\n
			 *    pArg[1] = eventID\n
			 *    pArg[2] = originalNetworkID\n
			 *    pArg[3] = transportStreamID\n
			 */
			EVENT_EVENT_RELAY				= 39,

			/**
			 * @brief Called when MVTV information is updated.\n
			 *    Update the current status about multiview whenever there is new event(EIT[present])
			 * @param msg 40
			 * @param ext1 channel index.\n
			 *    In case of Japan, 0 means 473.143Mhz and 1 means 479.143Mhz ... 39 means 707.143Mhz.
			 * @param ext2 serviceID
			 * @param obj int[5]\n
			 *    [0] = component_group_type\n
			 *    {\n
			 *        0    - means the current event is a multiview service.\n
			 *        else - not multiview. (default: 0xFF)\n
			 *    }\n
			 *    [1] = num_of_group\n
			 *    [2] = main_component_group_id\n
			 *    [3] = sub1_component_group_id\n
			 *    [4] = sub2_component_group_id
			 */
			EVENT_MVTV_UPDATE				= 40,

			/**
			 * @brief Called when start to play service and service information is updated.
			 * @param msg 41
			 * @param ext1 valid_serviceID_flag\n
			 *    0 - serviceId of broadcast isn't matched with information of DB. No play audio and video.\n
			 *    1 - serviceId of broadcast is same with one of DB\n
			 * @param ext2 channel index.\n
			 *    In case of Japan, 0 means 473.143Mhz and 1 means 479.143Mhz ... 39 means 707.143Mhz.
			 * @param obj not used
			 */
			EVENT_SERVICEID_CHECK			= 41,

			/**
			 * @brief Called when TRMP error has occurred.
			 * @param msg 43
			 * @param ext1 TRMP error code
			 * @param ext2 channel number
			 * @param obj not used
			 */
			EVENT_TRMP_ERROR			= 43,

			/**
			 * @brief Called when custom(relay/affiliation) search is being processed.
			 * @param msg 44
			 * @param ext1 status
			 *    3  : on going
			 *    2  : cancel
			 *    1  : success
			 *    -1 : fail
			 * @param ext2 not used
			 * @param obj class CustomSearchInfoData\n
			 *    status of CustomSearchInfoData in case of on-going\n
			 *    {\n
			 *        1 = channel lock\n
			 *        2 = skip for not detect station\n
			 *        3 = skip for not receive\n
			 *    }\n
			 *    status of CustomSearchInfoData in others\n
			 *    {\n
			 *        -1 if ext1 = -1\n
			 *        1  if ext1 = 1\n
			 *        2  if ext1 = 2\n
			 *    }\n
			 */
			EVENT_CUSTOMSEARCH_STATUS		= 44,

			/**
			 * @brief Called when Special Service information is updated.
			 * @param msg 45
			 * @param ext1 status\n
			 *    1 : Special Service Started\n
			 *    2 : Special Service Finished
			 * @param ext2 Special Service RowID in DB
			 * @param obj not used
			 */
			EVENT_SPECIALSERVICE_UPDATE		= 45,

			/**
			 * @brief A custom-tuner component can send custom type data to Application \n
			 *        by calling TCC_DxB_TUNER_SendCustomEventToApp API that is defined in tcc_dxb_interface_tuner.h.
			 * @param msg 46
			 * @param ext1 not used
			 * @param ext2 not used
			 * @param obj void* custom type
			 */
			EVENT_UPDATE_CUSTOM_TUNER		= 46,

			/**
			 * @brief Called when user select a service by ISDBTPlayer::setChannel() and ISDBTPlayer::TCCDxBProc_EWSClear().\n
			 *    or Called when success ISDBTPlayer::searchAndSetChannel().
			 * @param msg 48
			 * @param ext1 not used
			 * @param ext2 not used
			 * @param obj class AVIndexData
			 */
			EVENT_AV_INDEX_UPDATE			= 48,

			/**
			 * @brief Called when the section specified by ISDBTPlayer::setNotifyDetectSection() is received.
			 * @param msg 49
			 * @param ext1 channel index\n
			 *    In case of Japan, 0 means 473.143Mhz and 1 means 479.143Mhz ... 39 means 707.143Mhz.
			 * @param ext2 bit pattern of received sections.\n
			 *    bit0 = NIT\n
			 *    bit1 = PAT\n
			 *    bit2 = SDT\n
			 *    bit3 = PMT (main)\n
			 *    bit4 = PMT (sub)
			 * @param obj not used
			 */
			EVENT_SECTION_UPDATE			= 49,

			/**
			 * @brief Called to notify a result of ISDBTPlayer::searchAndSetChannel().
			 * @param msg 50
			 * @param ext1 result\n
			 *    0  : success\n
			 *    1  : scan cancel\n
			 *    -1 : scan fail\n
			 *    -2 : tsid mismatched\n
			 * @param ext2 not used
			 * @param obj class SetChannelData
			 */
			EVENT_SEARCH_AND_SETCHANNEL		= 50,

			/**
			 * @brief Called when current service number is changed.
			 * @param msg 51
			 * @param ext1 not used
			 * @param ext2 not used
			 * @param obj not used
			 */
			EVENT_DETECT_SERVICE_NUM_CHANGE	= 51,

			/**
			 * @brief Called to notify a result of ISDBTPlayer::setDataServiceStart().
			 * @param msg 52
			 * @param ext1 serviceID
			 * @param ext2 tableID
			 * @param obj class SectionDataInfo
			 */
			EVENT_SECTION_DATA_UPDATE		= 52,

			/**
			 * @brief Called to notify a result of ISDBTPlayer::customFilterStart().
			 * @param msg 53
			 * @param ext1 PID
			 * @param ext2 size of section data
			 * @param obj pointer to section data from table_id
			 */
			EVENT_CUSTOM_FILTER_DATA_UPDATE	= 53,

			/**
			 * @brief Called when the release/prepare of video decoder/renderer.
			 * @param msg 54
			 * @param ext1 OnOff\n
			 *        0 : off (video decode stop/release, video renderer stop/release)\n
			 *        1 : on (video decode start/prepare, video renderer start/prepare)
			 * @param ext2 channel index			 
			 * @param obj not used
			 */
			EVENT_SET_VIDEO_ONOFF           = 54,

			/**
			 * @brief Called when the Alsa close/open.
			 * @param msg 55
			 * @param ext1 OnOff\n
			 *        0 : off (Alsa Close)\n
			 *        1 : on (Alsa Open)
			 * @param ext2 channel index
			 * @param obj not used
			 */
			EVENT_SET_AUDIO_ONOFF			= 55,

			/**
			 * @brief Called when the video decode component has error.
			 * @param msg 56
			 * @param ext1 video error type\n
			 *        1 : VDEC_INIT_ERROR (include "VPU memory allocation error")\n
			 *        2 : VDEC_SEQ_HEADER_ERROR\n
			 *        3 : VDEC_DECODE_ERROR\n
			 *        4 : VDEC_BUFFER_FULL_ERROR\n
			 *        5 : VDEC_BUFFER_CLEAR_ERROR\n
			 *        6 : VDEC_NO_OUTPUT_ERROR
			 * @param ext2 channel index
			 * @param obj not used
			 */
			EVENT_VIDEO_ERROR				= 56
		};

		ISDBTPlayer();
		~ISDBTPlayer();

		/**
		 * @brief
		 *    The setListener should be called at first in order to get all events of DxB.
		 *
		 * @param [in] listener
		 *        Function pointer to get events of DxB.
		 *
		 * @return
		 *    0 : Success.
		 */
		int setListener(int (*listener)(int, int, int, void*));


		/**
		 * @brief
		 *    The prepare is used to initialize DxB core and OMX components.\n
		 *    Creating DxB process threads, initializing tuner, audio, video and demux components and so on.\n
		 *    EVENT_PREPARED event is issued just before the prepare returns.
		 *
		 * @param [in] specific_info
		 *        According to the PrepareBitMap below, App should set specific_info.\n
		 *        The specific_info consist of 32 bits and each bit has different meaning respectively.\n
		 *        \n
		 *        PrepareBitMap\n
		 *        Bit[31]    : 13Seg\n
		 *        Bit[30]    : 1Seg\n
		 *        Bit[29]    : JAPAN\n
		 *        Bit[28]    : BRAZIL\n
		 *        Bit[27]    : BCAS\n
		 *        Bit[26]    : TRMP\n
		 *        Bit[25:23] : reserved for ISDBT feature\n
		 *        Bit[22]    : record support\n
		 *        Bit[21]    : book support\n
		 *        Bit[20]    : timeshfit support\n
		 *        Bit[19]    : Dual-decoding support\n
		 *        Bit[18]    : Event Relay support\n
		 *        Bit[17]    : MVTV support\n
		 *        Bit[16]    : check of the service ID of broadcast\n
		 *        Bit[15]    : channel db Backup (Cancel a scan in the scanning. channel db backup)\n
		 *        Bit[14]    : special service support\n
		 *        Bit[13]    : data service support\n
		 *        Bit[12]    : skip SDT while scanning channels\n
		 *        Bit[11]    : primary service support\n
		 *        Bit[10]    : audio only mode\n
		 *        Bit[09:08] : Information about log\n
		 *                     {\n
		 *                         ISDBT_SINFO_ALL_ON_LOG  = 0,\n
		 *                         ISDBT_SINFO_IWE_LOG     = 1,\n
		 *                         ISDBT_SINFO_E_LOG       = 2,\n
		 *                         ISDBT_SINFO_ALL_OFF_LOG = 3\n
		 *                     }\n
		 *        Bit[07:06] : Decrypt Info\n
		 *                     {\n
		 *                         Decrypt_None    = 0,\n
		 *                         Decrypt_TCC353x = 1,\n
		 *                         Decrypt_HWDEMUX = 2,\n
		 *                         Decrypt_ACPU    = 3,\n
		 *                     }\n
		 *        Bit[05:00] : Demod Band Info\n
		 *                     {\n
		 *                         Baseband_None              = 0,\n
		 *                         Baseband_TCC351x_CSPI_STS  = 1,\n
		 *                         Baseband_reserved          = 2,\n
		 *                         Baseband_TCC351x_I2C_STS   = 3,\n
		 *                         Baesband_reserved6         = 4,\n
		 *                         Baseband_TCC351x_I2C_SPIMS = 5, //reserved\n
		 *                         Baseband_reserved          = 6,\n
		 *                         Baseband_reserved          = 7,\n
		 *                         Baseband_TCC353X_CSPI_STS  = 8,\n
		 *                         Baseband_TCC353X_I2C_STS   = 9,\n
		 *                         Baseband_Max               = 63\n
		 *                     }\n
		 *
		 * @return
		 *     0 : Success.\n
		 *    -1 : Fail to create event thread.\n
		 *    -2 : Fail to open ISDBTx.db.\n
		 *    -3 : Fail to open ISDBTEPGx.db.\n
		 *    -4 : Fail to init proc(tuner init, demux init, audio init, video init).
		 */
		int	prepare(unsigned int specific_info);

		/**
		 * @brief
		 *    The stop is used to stop A/V playback, so it can be called after calling setChannel.
		 *
		 * @return
		 *     0 : Success.\n
		 *    -1 : Not used.\n
		 *    -2 : Invalid state error.\n
		 *    -3 : Invalid param error. Not used.\n
		 *    -4 : 1seg audio stop error.\n
		 *    -5 : Fseg audio stop error.\n
		 *    -6 : 1seg video stop error.\n
		 *    -7 : Fseg video stop error.\n
		 *    -8 : Demux AV stop error.\n
		 *    -9 : Video set proprietarydata error.\n
		 *    -10 : Subtitle stop error.\n
		 *    -11 : Video deinitSurface error.
		 */
		int stop(void);

		/**
		 * @brief
		 *    The getSignalStrength is used to get signal strength that depends on tuner component.
		 *
		 * @param [out] sqinfo
		 *        It is a pointer allocated that has maximum size of int[256].\n
		 *        \n
		 *        In case of TCC353x, refer to information as below.\n
		 *        sqinfo[0]  : size of sqinfo in bytes.\n
		 *        sqinfo[1]  : percentage of 1seg signal quality.\n
		 *        sqinfo[2]  : percentage of fullseg signal quality.\n
		 *        sqinfo[3~] : informations about signal quality.\n
		 *
		 * @return
		 *    0 : Success.\n
		 *    1 : OMX error
		 */
		int getSignalStrength(int *sqinfo);


		/**
		 * @brief
		 *    The release is used to terminate DxB threads, deinitialize components and release resources.
		 *    
		 * @return
		 *     0 : Success.\n
		 *    -1 : Not used.\n
		 *    -2 : Fail to close ISDBTx.db.\n
		 *    -3 : Fail to close ISDBTEPGx.db.\n
		 *    -4 : Fail to deinit proc(tuner close & deinit, demux deinit, audio deinit, video deinit).
		 */
		int release(void);

		/**
		 * @brief
		 *    The setRelayStationChDataIntoDB is only for JVCKENWOOD.\n
		 *    If JVCKENWOOD App finds relay station by using 2TS, JVCKENWOOD App calls this function to save some data of NIT to DB.\n
		 *    And then, JVCKENWOOD App will call setChannel with mainRowID_List and subRowID_List.
		 *
		 * @param [in] total
		 *        Total number of RelayStationChInfo
		 * @param [in] pChData
		 *        This is the struct to regist the relay station channel information.
		 * @param [out] mainRowID_List
		 *        The list of '_id' of ChannelDB' for Main channel(Fullseg)
		 * @param [out] subRowID_List
		 *        The list of '_id' of ChannelDB' for Sub channel(1seg)
		 *
		 * @return 
		 *     1 : SQLite error.\n
		 *     0 : Success.\n
		 *    -1 : Parameter error. The pChData, mainRowID_List and subRowID_List should not be NULL.\n
		 *    -2 : Invalid state error.
		 */
		int setRelayStationChDataIntoDB(int total, RelayStationChData *pChData, int *mainRowID_List, int *subRowID_List);


		/**
		 * @brief
		 *    The search is used to search channels and according to scan_type, it supports several kinds of searching.
		 *
		 * @param [in] scan_type
		 *        0 : init_scan\n
		 *        1 : re_scan\n
		 *        2 : region_scan\n
		 *        3 : preset_scan\n
		 *        4 : manual_scan\n
		 *        5 : auto_search\n
		 *        6 : custom_scan\n
		 *        7 : epg_scan
		 * @param [in] country_code
		 *        0 : Japan\n
		 *        1 : Brazil\n
		 * @param [in] area_code
		 *        init_scan   : not used\n
		 *        re_scan     : not used\n
		 *        region_scan : refer region_id of Sheet(AreaInfo)\n
		 *        preset_scan : not used\n
		 *        manual_scan : fix to 0(zero)\n
		 *        auto_search : not used\n
		 *        custom_scan : pointer of channel list\n
		 *        epg_scan    : Network ID\n
		 * @param [in] channel_num
		 *        init_scan   : not used\n
		 *        re_scan     : not used\n
		 *        region_scan : not used\n
		 *        preset_scan : not used\n
		 *        manual_scan : channel index\n
		 *            In case of Japan, 0 means 473.143Mhz and 1 means 479.143Mhz ... 39 means 707.143Mhz.\n
		 *        auto_search : channel index\n
		 *        custom_scan : pointer of TSID list\n
		 *        epg_scan    : channel index\n
		 * @param [in] options
		 *        init_scan   : not used\n
		 *        re_scan     : not used\n
		 *        region_scan : not used\n
		 *        preset_scan : not used\n
		 *        manual_scan : 1 = clear DB, then scan, 0 = keep DB, additional scan\n
		 *        auto_search : 0 = downward, 1 = upward\n
		 *        custom_scan : number of repeat\n
		 *        epg_scan    : 1(1segEPG), 2(FsegEPG), 3(1seg,FsegEPG)\n
		 *
		 * @return
		 *     0 : Success.\n
		 *    -1 : Fail to put 'SCAN' command into proc queue.\n
		 *    -2 : Invalid state error.\n
		 *    -3 : Not used.\n
		 *    -4 : Stop error. If DxB state is AirPlay when 'search' is called, 'stop' processes first, but error happenes.
		 */
		int	search(int scan_type, int country_code, int area_code, int channel_num, int options);


		/**
		 * @brief 
		 *    The customRelayStationSearchRequest is used to execute relay & affiliation search in JVCKENWOOD's own way.
		 *
		 * @param [in] search_kind
		 *        1 : relay station search\n
		 *        2 : affiliation station search
		 * @param [in] list_ch
		 *        List of target channels to be searched.\n
		 *        The last element of array must be -1 and maximum size of the array is int[128].
		 * @param [in] list_tsid
		 *        List of target tsid.\n
		 *        The last element of array must be -1 and maximum size of the array is int[128].
		 * @param [in] repeat
		 *        repeat : 0 = infinity, N(>0) = times repeat
		 *
		 * @return
		 *     0 : Success.\n
		 *    -1 : Fail to put 'SCAN' command into proc queue.\n
		 *    -2 : Invalid state error.\n
		 *    -3 : Not used.\n
		 *    -4 : Stop error. If DxB state is AirPlay when 'customRelayStationSearchRequest' is called, 'stop' processes first, but error happenes.
		 */
		int	customRelayStationSearchRequest(int search_kind, int *list_ch, int *list_tsid, int repeat);


		/**
		 * @brief 
		 *    The searchCancel is used to stop channel searching if while searching.
		 *
		 * @return 
		 *     0 : Success.\n
		 *    -2 : Invalid state error.
		 */
		int searchCancel(void);


		/**
		 * @brief 
		 *    The searchAndSetchannel is a integration function of manual-scan and setchannel.\n
		 *    Process of the searchAndsetChannel is divided like below 2 step.\n
		 *    First, start manual-scan by country_code, channel_num. \n
		 *    After that, If found some services, start playing channel.(But if not, skip the setChannel process)
		 *
		 * @param [in] country_code
		 *        Japan(0) or Brazil(1). It is the same as parameter of search function.
		 * @param [in] channel_num
		 *        channel  index number for manual scan. It is the same as parameter of search function.
		 * @param [in] tsid
		 *        If tsid > 0, a tsid of found service will be comapred with this value and if it is different, return event of failure.\n
		 *        But if the tsid is 0, it's not used. It is the same as area_code parameter of search function.
		 * @param [in] options
		 *        0 = keep DB, additional scan\n
		 *        1 = clear DB, then scan\n
		 *        It the same as parameter of search function.
		 * @param [in] audioIndex
		 *        -1  : default Audio index\n
		 *        0~2 : Audio index. It is the same as parameter of setChannel function.
		 * @param [in] videoIndex
		 *        -1  : default Video index\n
		 *        0~2 : Video index. It is the same as parameter of setChannel function.
		 * @param [in] audioMode
		 *        0 : DUALMONO_LEFT\n
		 *        1 : DUALMONO_RIGHT\n
		 *        2 : DUALMONO_LEFTNRIGHT\n
		 *        It is the same as parameter of setChannel function.
		 * @param [in] raw_w
		 *        Whole screen width. It is the same as parameter of setChannel function.
		 * @param [in] raw_h
		 *        Whole screen height. It is the same as parameter of setChannel function.
		 * @param [in] view_w
		 *        Active screen width. It is the same as parameter of setChannel function.
		 * @param [in] view_h
		 *        Active screen height. It is same as parameter of setChannel function.
		 * @param [in] ch_index
		 *        Main/Sub channel to be displayed. It is the same as parameter of setChannel function.\n
		 *        0 : Fseg\n
		 *        1 : 1seg
		 *
		 * @return
		 *     0 : Success.\n
		 *    -1 : Fail to put SCAN_AND_SET command into proc queue.\n
		 *    -2 : Invalid state error.\n
		 *    -3 : Not used.\n
		 *    -4 : Stop error. If DxB state is AirPlay when 'searchAndSetChannel' is called, 'stop' processes first, but error happenes.		 
		 */
		int	searchAndSetChannel(int country_code, int channel_num, int tsid, int options, int audioIndex, int videoIndex, int audioMode, int raw_w, int raw_h, int view_w, int view_h, int ch_index);


		/**
		 * @brief 
		 *    The setEPGOnOff is used to decide whether EPG process on or off.
		 *
		 * @param [in] uiOnOff
		 *        0 : EPG(EIT) off\n
		 *        1 : EPG(EIT) on
		 *
		 * @return
		 *     0 : Success.\n
		 *    -3 : Invalid parameter error. The uiOnOff must be 0 or 1.
		 */
		int	setEPGOnOff(unsigned int uiOnOff);

		/**
		 * @brief 
		 *    Set audio track index.
		 *
		 * @param [in] index
		 *        0 : audio track 1\n 
		 *        1 : audio track 2\n
		 *        2 : audio track 3
		 *
		 * @return 
		 *     0 : Success.\n
		 *    -1 : DB error of main.\n
		 *    -2 : DB error of sub.\n
		 *    -3 : Invalid state.
		 */
		int setAudio(int index);

		/**
		 * @brief 
		 *    Set audio by component_tag.
		 *
		 * @param [in] component_tag
		 *        0 : audio track 1\n 
		 *        1 : audio track 2\n
		 *        2 : audio track 3
		 *
		 * @return 
		 *     0 : Success.\n
		 *    -1 : DB error of main.\n
		 *    -2 : DB error of sub.\n
		 *    -3 : Invalid state.
		 */
		int  setAudioByComponentTag(int component_tag);

		/**
		 * @brief 
		 *    Set video track index.
		 *
		 * @param [in] index
		 *        0 : video track 1\n
		 *        1 : video track 2\n
		 *        2 : video track 3		 
		 *
		 * @return 
		 *     0 : Success.\n
		 *    -1 : DB error of main.\n
		 *    -2 : DB error of sub.\n
		 *    -3 : Invalid state.
		 */
		int setVideo(int index);

		/**
		 * @brief 
		 *    Set video by component_tag.
		 *
		 * @param [in] index
		 *        0 : video track 1\n
		 *        1 : video track 2\n
		 *        2 : video track 3		 
		 *
		 * @return 
		 *     0 : Success.\n
		 *    -1 : DB error of main.\n
		 *    -2 : DB error of sub.\n
		 *    -3 : Invalid state.
		 */
		int  setVideoByComponentTag(int component_tag);
		
		/**
		 * @brief 
		 *    Set Audio render pause/restart.
		 *
		 * @param [in] uiOnOff
		 *        0 : Audio render pause (PCM data drop)\n
		 *        1 : Audio render restart
		 *
		 * @return 
		 *    0  : Success.\n
		 *    1  : OMX Parameter error.\n
		 *    -1 : Index Parameter error. It has only 0 or 1.
		 */
		int setAudioMuteOnOff(unsigned int uiOnOff);


		/**
		 * @brief 
		 *    Set Alsa close/open.
		 *
		 * @param [in] uiOnOff
		 *        0 : Alsa Close\n
		 *        1 : Alsa Open
		 *
		 * @return 
		 *    0  : Success.\n 
		 *    -1 : Error state. It is not AirPlay state.
		 */
		int setAudioOnOff(unsigned int uiOnOff);


		/**
		 * @brief 
		 *    Set Video render pause/restart.
		 *
		 * @param [in] uiOnOff
		 *        0 : Video render pause (video data drop)\n
		 *        1 : Video render restart
		 *
		 * @return 
		 *    0  : Success.\n
		 *    1  : OMX Parameter error.\n
		 *    -1 : Index Parameter error. It has only 0 or 1.
		 */
		int setVideoOutput(unsigned int uiOnOff);


		/**
		 * @brief 
		 *    Set Video decode & renderer release/create.
		 *
		 * @param [in] uiOnOff
		 *        0 : Video decode & render stop/release.\n
		 *        1 : Video decode & render start/create
		 *
		 * @return 
		 *    0  : Success.\n 
		 *    -1 : Error state. It is not AirPlay state.		 
		 */
		int setVideoOnOff(unsigned int uiOnOff);

		/**
		 * @brief 
		 *    The setChannel is used to set the new channel(Service, Audio, Video) and start playing.
		 *
		 * @param [in] mainRowID
		 *        It is _id of ChannelDB for Main channel(Fullseg).
		 * @param [in] subRowID
		 *        It is _id of ChannelDB for Sub channel(1seg).
		 * @param [in] audioIndex
		 *        -1  : Default Audio index.\n
		 *        0~2 : Audio index.
		 * @param [in] videoIndex
		 *        -1  : Default Video index.\n
		 *        0~2 : Video index.
		 * @param [in] audioMode
		 *        0 : DUALMONO_LEFT\n
		 *        1 : DUALMONO_RIGHT\n
		 *        2 : DUALMONO_LEFTNRIGHT
		 * @param [in] raw_w
		 *        Whole screen width of Android view. In case of Linux, set 0.
		 * @param [in] raw_h
		 *        Whole screen height of Android view. In case of Linux, set 0.
		 * @param [in] view_w
		 *        Active screen width of Android view. In case of Linux, set 0.
		 * @param [in] view_h
		 *        Active screen height of Android view. In case of Linux, set 0.
		 * @param [in] ch_index
		 *        Main/Sub channel to be displayed.\n
		 *        0 : Fseg\n
		 *        1 : 1seg
		 *
		 * @return
		 *     0 : Success.\n
		 *    -1 : Fail to get channel information from DB matched mainRowID and subRowID.\n
		 *    -2 : Invalid state error.\n
		 *    -3 : Not used.\n
		 *    -4 : Stop error. If DxB state is AirPlay when 'setChannel' is called, 'stop' processes first, but error happenes.
		 */
		int setChannel(int mainRowID, int subRowID, int audioIndex, int videoIndex, int audioMode, int raw_w, int raw_h, int view_w, int view_h, int ch_index);


		/**
		 * @brief 
		 *    The setChannel_withChNum is completely same with setChannel except 1st, 2nd and 3rd parmaeter.\n
		 *    1st, 2nd, 3rd and 4th parameter of the setChannel are replaced by 1st, 2nd and 3rd parmaeter of the setChannel_withChNum.
		 *
		 * @param [in] channelNumber
		 *        It is the channelNumber to play.\n
		 *        In case of Japan, 0 means 473.143Mhz and 1 means 479.143Mhz ... 39 means 707.143Mhz.
		 * @param [in] serviceID_Fseg
		 *        It is the servcie ID of Fseg to play.
		 * @param [in] serviceID_1seg
		 *        It is the service ID of 1seg to play.
		 * @param [in] audioMode
		 *        0 : DUALMONO_LEFT\n
		 *        1 : DUALMONO_RIGHT\n
		 *        2 : DUALMONO_LEFTNRIGHT
		 * @param [in] raw_w
		 *        Whole screen width of Android view. In case of Linux, set 0.
		 * @param [in] raw_h
		 *        Whole screen height of Android view. In case of Linux, set 0.
		 * @param [in] view_w
		 *        Active screen width of Android view. In case of Linux, set 0.
		 * @param [in] view_h
		 *        Active screen height of Android view. In case of Linux, set 0.
		 * @param [in] ch_index
		 *        Main/Sub channel to be displayed.\n
		 *        0 : Fseg\n
		 *        1 : 1seg
		 *
		 * @return 
		 *     0 : Success.\n
		 *    -1 : Fail to get channel information from DB matched channelNumber, serviceID_Fseg and serviceID_1seg.\n
		 *    -2 : Invalid state error.\n
		 *    -3 : Not used.\n
		 *    -4 : Stop error. If DxB state is AirPlay when 'setChannel_withChNum' is called, 'stop' processes first, but error happenes.
		 */
		int setChannel_withChNum(int channelNumber, int serviceID_Fseg, int serviceID_1seg, int audioMode, int raw_w, int raw_h, int view_w, int view_h, int ch_index);


		/**
		 * @brief 
		 *    Select 12seg mode or 1seg mode.
		 *
		 * @param [in] channelIndex
		 *        0 : 12seg mode\n
		 *        1 : 1seg mode
		 *
		 * @return 
		 *     0 : Success.\n
		 *	  -1 : proc error.\n
		 *	  -2 : parameter error.\n
		 *    -3 : state error.
		 */
		int	setDualChannel(int channelIndex);


		/**
		 * @brief 
		 *    The getChannelInfo is used to get information of current playing channel.
		 *
		 * @param [in] iChannelNumber
		 *        channel Number of current playing channel.
		 * @param [in] iServiceID
		 *        Service ID of current playing channel.
		 * @param [out] piRemoconID
		 *        Remocon ID
		 * @param [out] piThreeDigitNumber
		 *        Three Digit Number
		 * @param [out] piServiceType
		 *        Service Type
		 * @param [out] pusChannelName
		 *        channel Name. It is a pointer that is allocated with unsigned short[512].
		 * @param [out] piChannelNameLen
		 *        channel Name Length
		 * @param [out] piTotalAudioPID
		 *        Total Audio PID Number
		 * @param [out] piTotalVideoPID
		 *        Total Video PID Number
		 * @param [out] piTotalSubtitleLang
		 *        Total Subtitle Language Number
		 * @param [out] piAudioMode
		 *        Audio Mode
		 * @param [out] piVideoMode
		 *        Video Mode
		 * @param [out] piAudioLang1
		 *        Audio Language 1 
		 * @param [out] piAudioLang2
		 *        Audio Language 2
		 * @param [out] piAudioComponentTag
		 *        Audio Component Tag
		 * @param [out] piVideoComponentTag
		 *        Video Component Tag
		 * @param [out] piSubtitleComponentTag
		 *        Subtitle Component Tag
		 * @param [out] piStartMJD
		 *        Start MJD
		 * @param [out] piStartHH
		 *        Start Hour
		 * @param [out] piStartMM
		 *        Start Minute
		 * @param [out] piStartSS
		 *        Start Second
		 * @param [out] piDurationHH
		 *        Duration Hour
		 * @param [out] piDurationMM
		 *        Duration Minute
		 * @param [out] piDurationSS
		 *        Duration Second
		 * @param [out] pusEvtName
		 *        Event Name. It is a pointer that is allocated with unsigned short[512].
		 * @param [out] piEvtNameLen
		 *        Event Name Length
		 * @param [out] piLogoImageWidth
		 *        Logo Image Width
		 * @param [out] piLogoImageHeight
		 *        Logo Image Height
		 * @param [out] pusLogoImage
		 *        Logo Image Pointer. It is a pointer that is allocated with unsigned int[5184].
		 * @param [out] pusSimpleLogo
		 *        Simple Logo Pointer. It is a pointer that is allocated with unsigned short[512].
		 * @param [out] piSimpleLogoLength
		 *        Simple Logo Length
		 * @param
		 *    [out] piMVTVGroupType
		 *        0x0 : MVTV\n
		 *        0xFF : not MVTV\n
		 *        0xFFFFFFF : error
		 *
		 * @return
		 *    0 : Success.
		 */
		int	getChannelInfo(int iChannelNumber, int iServiceID,
								 int *piRemoconID, int *piThreeDigitNumber, int *piServiceType,
								 unsigned short *pusChannelName, int *piChannelNameLen,
								 int *piTotalAudioPID, int *piTotalVideoPID, int *piTotalSubtitleLang,
								 int *piAudioMode, int *piVideoMode, int *piAudioLang1, int *piAudioLang2,
								 int *piAudioComponentTag, int *piVideoComponentTag, int *piSubtitleComponentTag,
								 int *piStartMJD, int *piStartHH, int *piStartMM, int *piStartSS,
								 int *piDurationHH, int *piDurationMM, int *piDurationSS,
								 unsigned short *pusEvtName, int *piEvtNameLen,
								 int *piLogoImageWidth, int *piLogoImageHeight, unsigned int *pusLogoImage,
								 unsigned short *pusSimpleLogo, int *piSimpleLogoLength,
								 int *piMVTVGroupType);

		/**
		 * @brief 
		 *    Caption On/Off.
		 *
		 * @param [in] onoff
		 *        0 : On.\n
		 *        1 : Off	 
		 *
		 * @return 
		 *     0 : Success.\n
		 *    -1 : Error of getting a memory.\n
		 *    -2 : Size error.\n
		 *    -3 : Display thread stop state.\n
		 *    -4 : Memory not prepared.
		 */
		int playSubtitle(int onoff);


		/**
		 * @brief 
		 *    Superimpose On/Off.
         *
		 * @param [in] onoff
		 *        0 : On.\n
		 *        1 : Off	 
		 *
		 * @return 
		 *     0 : Success.\n
		 *    -1 : Error of getting a memory.\n
		 *    -2 : Size error.\n
		 *    -3 : Display thread stop state.\n
		 *    -4 : Memory not prepared.
		 */
		int playSuperimpose(int onoff);


		/**
		 * @brief 
		 *    set index of Caption.
		 *
		 * @param [in] index
		 *        0 : Off\n
		 *        1 : Caption-1 (ex: Japaness)\n
		 *        2 : Caption-2 (ex: Portugues)
		 *
		 * @return 
		 *     0 : Success.\n
		 *    -1 : Invalid language index (max: 8)
		 */
		int	setSubtitle (int index);


		/**
		 * @brief 
		 *    set index of Superimpose.
		 *
		 * @param [in] index
		 *        0 : Off\n
		 *        1 : Superimpose-1\n
		 *        2 : Superimpose-2
		 *
		 * @return 
		 *    0 : Success.\n
		 *    -1 : Invalid language index (max: 8)		 
		 */
		int	setSuperImpose (int index);


		/**
		 * @brief 
		 *    set mode of Audio DualMono.
		 *
		 * @param [in] audio_sel
		 *        0 : DualMono Left\n
		 *        1 : DualMono Right\n
		 *        2 : DualMono Left&Right
		 *
		 * @return 
		 *    0 : Success.\n
		 *    1 : OMX error.
		 */
		int	setAudioDualMono(int audio_sel);

		/**
		 * @brief 
		 *    get TRMP Information.
		 *
		 * @param [out] ppInfo
		 *        pointer of pointer to be returned data TRMP Information
		 * @param [out] piInfoSize
		 *        pointer to be returned size of TRMP Information
		 *
		 * @return 
		 *    0 : Success.
		 */
		int reqTRMPInfo(unsigned char **ppInfo, int *piInfoSize);

		/**
		 * @brief 
		 *    reset TRMP Information.
		 *
		 * @return 
		 *    0 : Success.
		 */
		int  resetTRMPInfo(void);

		/**
		 * @brief 
		 *    get Subtitle Language Information.
		 *
		 * @param [out] iSubtitleLangNum
		 *        Caption Language Number
		 * @param [out] iSuperimposeLangNum
		 *        Superimpose Language Number
		 * @param [out] iSubtitleLang1
		 *        Caption Language 1 
		 * @param [out] iSubtitleLang2
		 *        Caption Language 2 		 
		 * @param [out] iSuperimposeLang1
		 *        Superimpose Language 2 		 
		 * @param [out] iSuperimposeLang2
		 *        Superimpose Language 2 		 
		 *
		 * @return 
		 *      0 : Success.\n
		 *     -1 : Because detect to different serviceID, no informaion.
		 */
		int  getSubtitleLangInfo(int *iSubtitleLangNum, int *iSuperimposeLangNum, unsigned int *iSubtitleLang1, unsigned int *iSubtitleLang2, unsigned int *iSuperimposeLang1, unsigned int *iSuperimposeLang2);


		/**
		 * @brief 
		 *    The setPresetMode is used to set the current preset channel DB among three preset channel DB.\n
		 *    This API must be called idle(stopped) status.\n
		 *    Note that if ISDB-T is playing, you have to stop A/V playing and caption(superimpose) displaying together before this API is called.\n
		 *    Refer to stop and playSubtitle(playSuperimpose).
		 *
		 * @param [in] preset_mode
		 *        0~2 : Preset Mode
		 *
		 * @return
		 *     0 : Success.\n
		 *    -1 : Message handler is not running error.\n
		 *    -2 : Invalid state error.\n
		 *    -3 : Parameter error.
		 */
		int setPresetMode(int preset_mode);

		/**
		 * @brief 
		 *    customizing path to send any information to tuner.
		 *
		 * @param [in] size
		 *        Size of *arg\n
		 *        This value will be transferred to tuner component.
		 * @param [in] arg
		 *        This pointer will be transferred to tuner component through [SetParam (OMX_IndexVendorParamTunerCustom)] without any modification.\n
		 *        This pointer should not be NULL.
		 *
		 * @return 
		 *    0     : Success.\n
		 *    -1    : Parameter error. arg has not null. size has not zero (not minus).\n
		 *    ohter : OMX error
		 */
		int setCustom_Tuner(int size, void *arg);


		/**
		 * @brief 
		 *    customizing path to get any information from tuner.
		 *
		 * @param [out] size
		 *        Size of int
		 * @param [out] arg
         *        It has value of 2. Telechips&Litbig don't care this API.
		 *
		 * @return 
		 *     0     : Success.\n 
		 *     -1    : Parameter error. arg has not null.\n
		 *     other : OMX error
		 */
		int getCustom_Tuner(int *size, void *arg);


		/**
		 * @brief 
		 *    The setNotifyDetectSection is used to specify sections which will issue an event when the section is received.
		 *
		 * @param [in] sectionIDs
		 *        bit0 = NIT\n
		 *        bit1 = PAT\n
		 *        bit2 = SDT\n
		 *        bit3 = PMT(main)\n
		 *        bit4 = PMT(sub)\n
		 *
		 * @return
		 *    0 : Success.
		 */
		int setNotifyDetectSection(int sectionIDs);


		/**
		 * @brief 
		 *    The getDigitalCopyControlDescriptor is used to get an information of digital_copy_control_descriptor.
		 *
		 * @param [in] usServiceID
		 *        The service ID of information that you want to get.
		 * @param [out] pDCCDInfo
		 *        This is the struct that Digital Copy Control Descriptor information is saved into.
		 *        Caution : App must free the pDCCDinfo and sub_info memories.
		 *
		 * @return
		 *     0 : Success.\n
		 *    -1 : Invalid state error or there is no service matched usServiceID.\n
		 *    -2 : Not used.\n
		 *    -3 : Not used.\n
		 *    -4 : Malloc fail error.
		 */
		int getDigitalCopyControlDescriptor(unsigned short usServiceID, DigitalCopyControlDescriptor** pDCCDInfo);


		/**
		 * @brief 
		 *    The getContentAvailabilityDescriptor is used to get an information of content availability descriptor.
		 *
		 * @param [in] usServiceID
		 *        The service ID of information that you want to get.
		 * @param [out] copy_restriction_mode
		 *        copy_restriction_mode of content availability descriptor
		 * @param [out] image_constraint_token
		 *        image_constraint_token of content availability descriptor
		 * @param [out] retention_mode
		 *        retention_mode of content availability descriptor
		 * @param [out] retention_state
		 *        retention_state of content availability descriptor
		 * @param [out] encryption_mode
		 *        encryption_mode of content availability descriptor
		 *
		 * @return
		 *     0 : Success.\n
		 *    -1 : Invalid state error or there is no service matched usServiceID.
		 */
		int getContentAvailabilityDescriptor(unsigned short usServiceID,
											unsigned char *copy_restriction_mode,
											unsigned char *image_constraint_token,
											unsigned char *retention_mode,
											unsigned char *retention_state,
											unsigned char *encryption_mode);

		/**
		 * @brief 
		 *    get a state of 1st video frame output.
		 *
		 * @return 
		 *    0  : Waiting for video data.\n
		 *    1  : Completed to output of 1st video frame.\n
		 *    -1 : Get Parameter Error.		 
		 */
		int getVideoState(void);
		

		/**
		 * @brief 
		 *    The getSearchState is used to get a state of search.\n
		 *    Caution : If you call it before search() or customRelaystationSearchRequest(), it will always return 0.
		 *
		 * @return
		 *    1 : During the search.\n
		 *    0 : Search completed.
		 */
		int getSearchState(void);


		/**
		 * @brief 
		 *    The setDataServiceStart is used to request dataservice information.\n
		 *    If DxB has or finds the information, EVENT_SECTION_DATA_UPDATE will be issued.
		 *
		 * @return 
		 *    0 : Success.
		 */
		int setDataServiceStart(void);


		/**
		 * @brief 
		 *    The customFilterStart is used to install custom section filter.\n
		 *    The section data will return with EVENT_CUSTOM_FILTER_DATA_UPDATE.
		 *
		 * @param [in] iPID
		 *        Section PID you want to install.
		 * @param [in] iTableID
		 *        Section table ID you want to install.
		 *
		 * @return
		 *    1 : Demux component handle error.\n
		 *    0 : Success.
		 */
		int customFilterStart(int iPID, int iTableID);


		/**
		 * @brief 
		 *    The customFilterStop is used to remove custom section filter.
		 *
		 * @param [in] iPID
		 *        Section PID you want to remove.
		 * @param [in] iTableID
		 *        Section table ID you want to remove.
		 *
		 * @return 
		 *     1 : Demux component handle error.\n
		 *     0 : Success.\n
		 *    -1 : There is no custom filter matched iPID and iTableID.
		 */
		int customFilterStop(int iPID, int iTableID);


		/**
		 * @brief 
		 *    The EWS_start is used to start EWS detection only mode.
		 *
		 * @param [in] mainRowID
		 *        It is used by EWS_clear.\n
		 *        _id of ChannelDB for Main channel(Fullseg)
		 * @param [in] subRowID
		 *        It is used by EWS_clear.\n
		 *        _id of ChannelDB' for Sub channel(1seg)
		 * @param [in] audioIndex
		 *        It is used by EWS_clear.\n
		 *        -1  : default Audio index\n
		 *        0~2 : Audio index
		 * @param [in] videoIndex
		 *        It is used by EWS_clear.\n
		 *        -1  : default Video index\n
		 *        0~2 : Video index
		 * @param [in] audioMode
		 *        It is used by EWS_clear.\n
		 *        0 : DUALMONO_LEFT\n
		 *        1 : DUALMONO_RIGHT\n
		 *        2 : DUALMONO_LEFTNRIGHT
		 * @param [in] raw_w
		 *        Whole screen width of Android view.\n
		 *        In case of Linux, set 0.
		 * @param [in] raw_h
		 *        Whole screen height of Android view.\n
		 *        In case of Linux, set 0.
		 * @param [in] view_w
		 *        Active screen width of Android view.\n
		 *        In case of Linux, set 0.
		 * @param [in] view_h
		 *        Active screen height of Android view.\n
		 *        In case of Linux, set 0.
		 * @param [in] ch_index
		 *        Main/Sub channel to be displayed\n
		 *        0 : Fseg\n
		 *        1 : 1seg
		 *
		 * @return
		 *     0 : Success.\n
		 *    -1 : Fail to get channel information from DB matched mainRowID and subRowID.\n
		 *    -2 : Invalid state error.\n
		 *    -3 : Not used.\n
		 *    -4 : Stop error. If DxB state is AirPlay when 'EWS_start' is called, 'stop' processes first, but error happenes.
		 */
		int EWS_start(int mainRowID, int subRowID, int audioIndex, int videoIndex, int audioMode, int raw_w, int raw_h, int view_w, int view_h, int ch_index);


		/**
		 * @brief 
		 *    The EWS_start_withChNum is completely same with EWS_start except 1st, 2nd and 3rd parmaeter.\n
		 *    1st, 2nd, 3rd and 4th parameters of the EWS_start are replaced by 1st, 2nd and 3rd parmaeters of the EWS_start_withChNum.
		 *
		 * @param [in] channelNumber
		 *        It is used by EWS_clear.\n
		 *        It is the channelNumber to play.\n
		 *        In case of Japan, 0 means 473.143Mhz and 1 means 479.143Mhz ... 39 means 707.143Mhz.
		 * @param [in] serviceID_Fseg
		 *        It is used by EWS_clear.\n
		 *        It is the servcie ID of Fseg to play.
		 * @param [in] serviceID_1seg
		 *        It is used by EWS_clear.\n
		 *        It is the service ID of 1seg to play.
		 * @param [in] audioMode
		 *        It is used by EWS_clear.\n
		 *        0 : DUALMONO_LEFT\n
		 *        1 : DUALMONO_RIGHT\n
		 *        2 : DUALMONO_LEFTNRIGHT
		 * @param [in] raw_w
		 *        Whole screen width of Android view. In case of Linux, set 0.
		 * @param [in] raw_h
		 *        Whole screen height of Android view. In case of Linux, set 0.
		 * @param [in] view_w
		 *        Active screen width of Android view. In case of Linux, set 0.
		 * @param [in] view_h
		 *        Active screen height of Android view. In case of Linux, set 0.
		 * @param [in] ch_index
		 *        Main/Sub channel to be displayed\n
		 *        0 : Fseg\n
		 *        1 : 1seg
		 *
		 * @return
		 *     0 : Success.\n
		 *    -1 : Fail to get channel information from DB matched channelNumber, serviceID_Fseg and serviceID_1seg.\n
		 *    -2 : Invalid state error.\n
		 *    -3 : Not used.\n
		 *    -4 : Stop error. If DxB state is AirPlay when 'EWS_start_withChNum' is called, 'stop' processes first, but error happenes.
		 */
		int EWS_start_withChNum(int channelNumber, int serviceID_Fseg, int serviceID_1seg, int audioMode, int raw_w, int raw_h, int view_w, int view_h, int ch_index);


		/**
		 * @brief 
		 *    The EWS_clear is used to stop EWS detection only mode and start A/V playback with channel information that EWS_start(__withChNum) set.
		 *
		 * @return 
		 *     0 : Success.\n
		 *    -1 : Not used.\n
		 *    -2 : Invalid state error.
		 */
		int EWS_clear(void);


		/**
		 * @brief 
		 *    Make settings related to seamless function.
		 *
		 * @param [in] onoff
		 *        0 : off\n
         *        1 : on (default value) , compensation seamless delay
		 * @param [in] interval
		 *        This value is the time period of seamless retry. (unit: seconds)\n
         *        Valid value   : 10~300\n
         *        Default value : 20
		 * @param [in] strength
         *        This value is the signal level condition of seamless retry. (Unit: dbm)\n
         *        Valid value   : 0~100\n
         *        Default value : 40 
		 * @param [in] ntimes
         *        This value is the number of times the range function starts.\n
         *        Valid value   : 10~1000\n
         *        Default value : 20		 
		 * @param [in] range
         *        This value represents the range of valid ranges of the dfs gap. (unit: mili seconds)\n
         *        Valid value   : 10~300\n
         *        Default value : 100		 
		 * @param [in] gapadjust
         *        This value is the tuning value for the correction of 1st dfs gap.\n
         *        Valid value   : -300~300\n
         *        Default value : 0		 
		 * @param [in] gapadjust2
         *        This value is the tuning value for the compensation of the 2nd dfs gap.\n
         *        Valid value   : -150~150\n
         *        Default value : 0		 
		 * @param [in] multiplier
		 *        This value is the tuning value for multiples of 2nd dfs gap.\n
         *        Valid value   : 1~10\n
         *        Default value : 1
         *
		 * @return
		 *     0 : Success.\n
		 *    -1 : OMX error.
		 */
		int Set_Seamless_Switch_Compensation(int onoff, int interval, int strength, int ntimes, int range, int gapadjust, int gapadjust2, int multiplier);


		/**
		 * @brief 
		 *    Monitoring of seamless value.
		 *
		 * @param [out] state
		 *        DFS state information (0~2):\n 
         *        0 : dfs success, store data has exist\n
         *        1 : dfs fail, store data has exist\n
         *        2 : dfs fail, store data nothing, retry routine starts
		 * @param [out] cval
		 *        Current DFS value
		 * @param [out] pval
		 *        DFS value (Avr in the past) / Number of save
		 * @return 
		 *     0 : Success.\n
		 *     1 : OMX Error
		 */
		int getSeamlessValue(int *state, int *cval, int *pval);


		/**
		 * @brief 
		 *    Get STC time.
		 *
		 * @param [out] hi_data
		 *        b32 of stc data (33bits)
		 * @param [out] lo_data
		 *        b0-b31 of stc data(33bits)
		 *
		 * @return 
		 *    0  : Success.\n
		 *    -1 : Invalid stc data (0)
		 */
		int getSTC(unsigned int *hi_data, unsigned int *lo_data);


		/**
		 * @brief 
		 *    The getServiceTime is used to get current service time(TOT)
		 *
		 * @param [out] year
		 *        year of current service time
		 * @param [out] month
		 *        month of current service time
		 * @param [out] day
		 *        day of current service time
		 * @param [out] hour
		 *        hour of current service time
		 * @param [out] min
		 *        minute of current service time
		 * @param [out] sec
		 *        second of current service time
		 *
		 * @return 
		 *     0 : Success.\n
		 *    -1 : Invalid state error.
		 */
		int getServiceTime(unsigned int *year, unsigned int *month, unsigned int *day, unsigned int *hour, unsigned int *min, unsigned int *sec);


		/**
		 * @brief 
		 *    The getContentID is used to get content ID and associated contents flag of data content descriptor.
		 *
		 * @param [out] contentID
		 *        content ID
		 * @param [out] associated_contents_flag
		 *        associated contents flag
		 *
		 * @return 
		 *     0 : Success.\n
		 *    -1 : Not used.\n
		 *    -2 : Invalid state error.
		 */
		int getContentID(unsigned int *contentID, unsigned int *associated_contents_flag);


		/**
		 * @brief 
		 *    The checkExistComponentTagInPMT is used to check the existence of the given component tag in PMT.
		 *
		 * @param [in] target_component_tag
		 *        component tag to check
		 *
		 * @return 
		 *    0    : Not Exist\n
		 *    else : Number of PMT 2nd Loop (1 means 1th in 2nd Loop / 2 means 2nd in 2nd Loop / ...)
		 */
		int checkExistComponentTagInPMT(int target_component_tag);


		/**
		 * @brief 
		 *    Event noficiation.
		 *
		 * @param [out] msg
		 *        event message number
		 * @param [out] ext1
		 *        event message-1
		 * @param [out] ext2
		 *        event message-2
		 * @param [out] obj
		 *        event message type of struct
		 */
		void notify(int msg, int ext1, int ext2, void *obj);

		static void UI_ThreadFunc(void *arg);

	private:
		pthread_mutex_t	mLock; // related with ISDBTPlayerListener
		pthread_mutex_t	mNotifyLock; // related with notify
		ISDBTPlayerListener *mClientListener;
		int (*mListener)(int, int, int, void*);
		int	mThreadState;
};

//}; // namespace linux

#endif // LINUX_ISDBT_BPLAYER_H
