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
#ifndef	_TCC_ISDBT_MANAGER_DEMUX_H_
#define	_TCC_ISDBT_MANAGER_DEMUX_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ------ backscan -----*/
#define MAX_SUPPORT_PLAYBACKSCAN_PMTS	32

#define DEMUX_BACKSCAN_NIT	0x00000001
#define DEMUX_BACKSCAN_PAT	0x00000002
#define DEMUX_BACKSCAN_PMT	0x00000004
#define DEMUX_BACKSCAN_BIT	0x00000008
#define DEMUX_BACKSCAN_SDT	0x00000010

#define	DEMUX_BACKSCAN_REQUEST_NONE		(0)
#define	DEMUX_BACKSCAN_REQUEST_SEARCH_FULLSEG	(1<<0)

typedef enum{
	E_DEMUX_STATE_SCAN,
	E_DEMUX_STATE_INFORMAL_SCAN,	// for only searchAndSetChannel
	E_DEMUX_STATE_AIRPLAY,
	E_DEMUX_STATE_HANDOVER,
	E_DEMUX_STATE_IDLE,
	E_DEMUX_STATE_MAX
}E_DEMUX_STATE;

#define PID_PAT				0x0000
#define PID_CAT				0x0001
#define PID_PMT				0x0002
#define	PID_PMT_1SEG		0x1FC8
#define PID_NIT 				0x0010
#define PID_SDT				0x0011
#define PID_EIT				0x0012
#define PID_BIT				0x0024
#define PID_L_EIT			0x0027
#define PID_M_EIT			0x0026
#define PID_H_EIT			(PID_EIT)
#define PID_TDT_TOT			0x0014
#define PID_CDT				0x0029


#define ISDBT_PARTIAL_RECEPTION_SERVICE_NUM 8
typedef struct
{
	unsigned int				eit_type;
	unsigned int				numServiceId;
	unsigned short			servicd_id[ISDBT_PARTIAL_RECEPTION_SERVICE_NUM];
} ISDBT_PARTIAL_RECEPTION_ATTRIBUTE;

typedef enum
{
	DEMUX_REQUEST_PAT = 1000,
	DEMUX_REQUEST_PMT = 2000,
	DEMUX_REQUEST_SDT = 3000,
	DEMUX_REQUEST_EIT = 4000,
	DEMUX_REQUEST_NIT = 5000,
	DEMUX_REQUEST_SUBT = 6000,
	DEMUX_REQUEST_1SEG_SUBT = 6001,
	DEMUX_REQUEST_TTX = 6002,
	DEMUX_REQUEST_1SEG_TTX = 6003,
	DEMUX_REQUEST_ECM = 7000,
	DEMUX_REQUEST_TIME = 8000,
	DEMUX_REQUEST_CAT = 9000,
	DEMUX_REQUEST_EMM = 10000,
	DEMUX_REQUEST_BIT = 11000,
	DEMUX_REQUEST_CDT = 12000,
	DEMUX_REQUEST_1SEG_PMT = 13000,
	DEMUX_REQUEST_FSEG_PMT_BACKSCAN = 14000,
	DEMUX_REQUEST_CUSTOM = 15000
}E_REQUEST_IDS;

typedef enum
{
	DEC_CMDTYPE_DATA = 0,
	DEC_CMDTYPE_CONTROL
}E_DECTHREAD_COMMANDTYPE;

typedef enum
{
	DEC_CTRLCMD_START = 0,
	DEC_CTRLCMD_STOP,
	DEC_CTRLCMD_CLEAR,
	DEC_CTRLCMD_SETCFG,
	DEC_CTRLCMD_RESTART,
	DEC_CTRLCMD_RESTOP
}E_DECTHREAD_CTRLCOMMAND;

typedef enum
{
	DEC_THREAD_RUNNING = 0,
	DEC_THREAD_STOP,
	DEC_THREAD_STOP_REQUEST    // Noah, IM692A-29
}E_DECTHREAD_STATUS;

typedef enum
{
	EWS_NONE = 0,
	EWS_START,
	EWS_ONGOING,
	EWS_STOPWAIT,
	EWS_STOP
} E_EWS_STATE;

typedef enum {
	SPECIAL_SERVICE_STOP = 0,
	SPECIAL_SERVICE_NONE,
	SPECIAL_SERVICE_START_WAITING,
	SPECIAL_SERVICE_RUNNING
} E_SPECIAL_SERVICE_STATE;

typedef enum {
	SCAN_STATE_NONE = 0,
	SCAN_STATE_NIT,	/* wait NIT */
	SCAN_STATE_PAT,	/* wait PAT */
	SCAN_STATE_PMTs, /* wait PMTs of PAT */
	SCAN_STATE_COMPLETE /* wait All */
} E_SCAN_STATE;

typedef enum {
	NOTIFY_SECTION_NIT = 1,
	NOTIFY_SECTION_PAT = 2,
	NOTIFY_SECTION_SDT = 4,
	NOTIFY_SECTION_PMT_MAIN = 8,
	NOTIFY_SECTION_PMT_SUB = 16
} E_NOTIFY_SECTION_IDS;

typedef struct
{
	E_DECTHREAD_COMMANDTYPE eCommandType;
	E_DECTHREAD_CTRLCOMMAND	eCtrlCmd;
	int iCommandSync;
	void *pArg;
	int iRequestID;
	unsigned int uiDataSize;
	unsigned int uiPTS;
	unsigned char *pData;
	unsigned int uiType;
}DEMUX_DEC_COMMAND_TYPE;

#define MAX_CUSTOM_FILTER 32
typedef struct
{
	int iPID;
	int iTableID;
	int iRequestID;
} CUSTOM_FILTER;
typedef struct
{
	CUSTOM_FILTER customFilter[MAX_CUSTOM_FILTER];
} CUSTOM_FILTER_LIST;

typedef enum {
	DS_STATE_NONE,
	DS_STATE_REQUEST
} E_DS_STATE;

#define MAX_DS_ELE 32
typedef struct demux_mgr_ds_ele{
	unsigned short	usServiceID;
	int iTableID;
	unsigned short	uiDSMCC_PID;
	unsigned short	usDS_SID;
	unsigned short	ucAutoStartFlag;
	unsigned short	usNetworkID;
	unsigned short	usTransportStreamID;
	unsigned int	uiRawDataLen;
	unsigned char	*pucRawData;
} DEMUX_MGR_DS_ELE;

typedef struct {
	E_DS_STATE state;
	DEMUX_MGR_DS_ELE list[MAX_DS_ELE];
} DEMUX_MGR_DS;

/*--------- definition for scan ------------*/
#define	ISDBT_SCANOPTION_NONE		0
#define	ISDBT_SCANOPTION_ARRANGE_RSSI	(1<<0)
#define	ISDBT_SCANOPTION_AUTOSEARCH		(1<<1)
#define	ISDBT_SCANOPTION_RESCAN			(1<<2)
#define ISDBT_SCANOPTION_HANDOVER		(1<<3)
#define ISDBT_SCANOPTION_CUSTOM_RELAY	(1<<4)
#define	ISDBT_SCANOPTION_CUSTOM_AFFILIATION (1<<5)
#define ISDBT_SCANOPTION_MANUAL			(1<<6)
#define ISDBT_SCANOPTION_INFORMAL		(1<<7)
#define ISDBT_SCANOPTION_INFINITE_TIMER	(1<<8)    // Noah / 20170706 / IM478A-30 / In case of the searchAndSetChannel, NIT & BIT have infinite timer.

// Noah, IM692A-29
extern void tcc_manager_demux_set_section_decoder_state(int state);
extern int tcc_manager_demux_get_section_decoder_state(void);

extern void tcc_manager_demux_set_state(E_DEMUX_STATE state);
extern E_DEMUX_STATE tcc_manager_demux_get_state(void);
extern void tcc_manager_set_updatechannel_flag (int arg);
extern int tcc_manager_get_updatechannel_flag(void);

extern int tcc_manager_demux_init(void);
extern int tcc_manager_demux_deinit(void);
extern int tcc_manager_demux_set_serviceID(int iServiceID);
extern int tcc_manager_demux_get_serviceID(void);
extern int tcc_manager_demux_set_playing_serviceID(unsigned int iServiceID);
extern unsigned int tcc_manager_demux_get_playing_serviceID(void);
extern int tcc_manager_demux_set_first_playing_serviceID(unsigned int iServiceID);
extern unsigned int tcc_manager_demux_first_get_playing_serviceID(void);
extern int tcc_manager_demux_set_tunerinfo(int iChannel, int iFrequency, int iCountrycode);
extern int tcc_manager_demux_delete_db(void);
extern void tcc_manager_demux_set_eittype(int type);
extern int tcc_manager_demux_get_eittype(void);
extern int tcc_manager_demux_set_validOutput(int valid);
extern void tcc_manager_demux_set_1seg_service(int service_id);
extern int tcc_manager_demux_set_sectionfilter(int table_id, int pid, int request_id);
extern int tcc_manager_demux_scan(int channel, int country_code, int option, void *ptr);
extern int tcc_manager_demux_scan_customize(int channel, int country_code, int option, int *tsid, void *ptr);
extern int	tcc_manager_demux_channeldb_backup(void);
extern int tcc_manager_demux_channeldb_restoration(void);
extern int tcc_manager_demux_scan_cancel(void);
extern int tcc_manager_demux_handover(void);
extern int tcc_manager_demux_teletext_play(int iTeletextPID);

extern int tcc_manager_demux_reset_av(int Audio, int Video);


extern int tcc_manager_demux_av_play(int AudioPID, int VideoPID, int AudioType, int VideoType,\
                             int AudioSubPID, int VideoSubPID, int AudioSubType, int VideoSubType,\
                             int PcrPID, int PcrSubPid, int PmtPID, int PmtSubPID, int CAECMPID, int ACECMPID, int dual_mode);
extern int tcc_manager_demux_scan_and_avplay(int AudioPID, int VideoPID, int AudioType, int VideoType,\
                             int AudioSubPID, int VideoSubPID, int AudioSubType, int VideoSubType,\
                             int PcrPID, int PcrSubPid, int PmtPID, int PmtSubPID, int CAECMPID, int ACECMPID, int dual_mode);
extern int tcc_manager_demux_av_stop(int StopAudio, int StopVideo, int StopAudioSub, int StopVideoSub);

extern int tcc_manager_demux_set_area(int area_code);
extern int tcc_manager_demux_set_preset(int area_code);
extern int tcc_manager_demux_get_area (void);
extern int tcc_manager_demux_set_parentlock(unsigned int lock);
extern int tcc_manager_demux_change_videoPID(int fVideoMain, int VideoPID, int VideoType, int fVideoSub, int VideoSubPID, int VideoSubType);
extern int tcc_manager_demux_change_audioPID(int fAudioMain, unsigned int uiAudioMainPID, int fAudioSub, unsigned int uiAudioSubPID);

extern unsigned int TCC_Isdbt_Get_Specific_Info(void);
extern void TCC_Isdbt_Specific_Info_Init(unsigned int specific_info);
extern void TCC_Isdbt_SetProfileA(void);
extern void TCC_Isdbt_SetProfileC(void);
extern int TCC_Isdbt_IsSupportProfileA(void);
extern int TCC_Isdbt_IsSupportProfileC(void);
extern int TCC_Isdbt_IsSupportJapan(void);
extern int TCC_Isdbt_IsSupportBrazil(void);
extern int TCC_Isdbt_IsSupportBCAS(void);
extern int TCC_Isdbt_IsSupportTRMP(void);
extern int TCC_Isdbt_IsSupportCAS(void);
extern int TCC_Isdbt_IsSupportPVR(void);
extern int TCC_Isdbt_GetBaseBand (void);
extern int TCC_Isdbt_IsSupportTunerTCC351x_CSPI_STS(void);
extern int TCC_Isdbt_IsSupportTunerTCC351x_I2C_STS(void);
extern int TCC_Isdbt_IsSupportTunerTCC351x_I2C_SPIMS(void);
extern int TCC_Isdbt_IsSuppportTunerTCC353x_CSPI_STS(void);
extern int TCC_Isdbt_IsSuppportTunerTCC353x_I2C_STS(void);
extern int TCC_Isdbt_GetDecryptInfo(void);
extern int TCC_Isdbt_IsSupportDecryptTCC353x(void);
extern int TCC_Isdbt_IsSupportDecryptHWDEMUX(void);
extern int TCC_Isdbt_IsSupportDecryptACPU(void);
extern int TCC_Isdbt_IsSupportEventRelay(void);
extern int TCC_Isdbt_IsSupportMVTV(void);
extern int TCC_Isdbt_IsSupportCheckServiceID(void);
extern int TCC_Isdbt_IsSupportChannelDbBackUp(void);
extern int TCC_Isdbt_IsSupportSpecialService(void);
extern int TCC_Isdbt_IsSupportDataService(void);
extern int TCC_Isdbt_IsSkipSDTInScan(void);
extern int TCC_Isdbt_IsAudioOnlyMode(void);
extern int TCC_Isdbt_IsSupportAlsaClose(void);

extern int TCC_Isdbt_IsSupportPrimaryService(void);
extern void TCC_Isdbt_Clear_AudioOnlyMode(void);

extern int tcc_manager_demux_cas_init(void);

extern int tcc_manager_demux_start_pcr_pid(int pcr_pid, int index);
extern int tcc_manager_demux_stop_pcr_pid(int index);

extern void tcc_manager_demux_set_callback_getBER (void *arg);
extern void tcc_manager_demux_set_strengthIndexTime (int tuner_str_idx_time);
extern int tcc_manager_demux_set_presetMode (int preset_mode);

extern int tcc_manager_demux_set_eit_subtitleLangInfo(unsigned int NumofLang, unsigned int firstLangcode, unsigned int secondLangcode, unsigned short ServiceID, int cnt);
extern int tcc_manager_demux_get_eit_subtitleLangInfo(int *LangNum, unsigned int *SubtitleLang1, unsigned int *SubtitleLang2);
extern int tcc_demux_section_decoder_eit_update(int service_id, int table_id);
extern void tcc_manager_demux_set_eit_subtitleLangInfo_clear(void);
extern int tcc_manager_demux_get_curDCCDescriptorInfo(unsigned short usServiceID, void **pDCCInfo);
extern int tcc_manager_demux_get_curCADescriptorInfo(unsigned short usServiceID, unsigned char ucDescID, void *arg);
extern int tcc_manager_demux_get_componentTag(int iServiceID, int index, unsigned char *pucComponentTag, int *piComponentCount);
extern int tcc_manager_demux_desc_update(unsigned short usServiceID, unsigned char ucDescID, void *arg);
extern int tcc_manager_demux_event_relay(unsigned short usServiceID, unsigned short usEventID, unsigned short usOrigianlNetworkID, unsigned short usTransportStreamID);
extern int tcc_manager_demux_mvtv_update(unsigned char *pucArr);

extern int tcc_manager_demux_scanflag_init(void);
extern int tcc_manager_demux_scanflag_get(void);
extern void tcc_manager_demux_release_searchfilter(void);
extern void tcc_manager_demux_set_channel_service_info_clear(void);
extern void tcc_manager_demux_specialservice_DeleteDB(void);
extern int tcc_manager_demux_set_section_notification_ids(int iSectionIDs);
extern void tcc_manager_demux_reset_section_notification(void);

extern int tcc_demux_parse_servicetype(unsigned int service_id);
extern int tcc_demux_parse_servicenumber(unsigned int service_id);

extern void tcc_manager_demux_logo_init (void);
extern void tcc_manager_demux_logo_get_infoDB (unsigned int channel_no, unsigned int country_code, unsigned int network_id, unsigned int svc_id_f, unsigned int svc_id_1);
extern void tcc_manager_demux_channelname_init(void);
extern void tcc_manager_demux_channelname_get_infoDB(unsigned int channel_no, unsigned int country_code, unsigned int network_id, unsigned int svc_id_f, unsigned int svc_id_1);

extern void tcc_manager_demux_set_skip_sdt_in_scan(int enable);
extern int tcc_manager_demux_is_skip_sdt_in_scan(void);
extern int tcc_manager_demux_scan_customize_read_strength(void);

//Check the service ID of broadcast
extern void tcc_manager_demux_serviceInfo_Init(void);
extern int tcc_manager_demux_serviceInfo_set(unsigned int recServiceId);
extern void tcc_manager_demux_init_av_skip_flag(void);
extern int tcc_manager_demux_serviceInfo_comparison(unsigned int curServiceId);
extern int tcc_manager_demux_get_validOutput(void);

//PAT check
extern int tcc_demux_section_decoder_update_pat_in_broadcast(void);

//Detect difference the number of service
extern void tcc_manager_demux_detect_service_num_Init(void);
extern void tcc_manager_demux_check_service_num_Init(void);
extern int tcc_manager_demux_detect_service_num(int SvcNum, int NetworkID);
extern void tcc_manager_demux_set_service_num(int SvcNum);
extern int tcc_manager_demux_get_service_num(void);

extern int tcc_manager_demux_dataservice_start(void);
extern int tcc_manager_demux_custom_filter_start(int iPID, int iTableID);
extern int tcc_manager_demux_custom_filter_stop(int iPID, int iTableID);

extern int tcc_manager_demux_ews_start(int PmtPID, int PmtSubPID);
extern int tcc_manager_demux_ews_clear();

extern int tcc_manager_demux_get_stc(int index, unsigned int *hi_data, unsigned int *lo_data);
extern int tcc_manager_demux_set_dualchannel(unsigned int index);
extern void tcc_manager_demux_set_subtitle_option(int type, int onoff);
extern int tcc_manager_demux_get_contentid(unsigned int *contentID, unsigned int *associated_contents_flag);

// Noah / 20170621 / Added for IM478A-60 (The request additinal API to start/stop EPG processing)
extern void tcc_manager_demux_set_EpgOnOff(unsigned int uiEpgOnOff);
extern unsigned int tcc_manager_demux_get_EpgOnOff(void);


#ifdef __cplusplus
}
#endif

#endif


