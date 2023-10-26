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
#define LOG_NDEBUG 0
#define LOG_TAG	"ISDBT_MANAGER_DEMUX"
#include <stdlib.h>
#include <utils/Log.h>
#include <fcntl.h>
#include <pthread.h>
#include <cutils/properties.h>
#include "tcc_isdbt_event.h"
#include "tcc_dxb_interface_video.h"

#include <tcc_dxb_memory.h>
#include <tcc_dxb_queue.h>
#include <tcc_dxb_semaphore.h>
#include <tcc_dxb_thread.h>
#include "tcc_isdbt_manager_demux.h"
#include "tcc_isdbt_manager_demux_subtitle.h"
#include "tcc_isdbt_manager_tuner.h"
#include "tcc_isdbt_manager_cas.h"
#include "tcc_isdbt_manager_audio.h"
#include "tcc_isdbt_manager_video.h"
#include "tcc_isdbt_event.h"
#include "tcc_isdbt_proc.h"

#include "BitParser.h"
#include "ISDBT_Caption.h"
#include "ISDBT_Common.h"
#include "TsParse_ISDBT.h"
#include "TsParser_Subtitle.h"
#include "TsParse_ISDBT_DBLayer.h"
#include "TsParse_ISDBT_ParentalRate.h"
#include "TsParse_ISDBT_SI.h"

#include "tcc_dxb_interface_cipher.h"
#include "tcc_dxb_manager_dsc.h"
#include "tcc_dxb_manager_sc.h"
#include "tcc_dxb_manager_cas.h"
#include "tcc_dxb_timecheck.h"
#include "subtitle_draw.h"

/****************************************************************************
DEFINITION
****************************************************************************/
#define MNG_DBG(msg...) ALOGD(msg)
#define	 _SUPPORT_ISDBT_EIT_

#define	SECTION_QUEUESIZE		1000

/*---------- scan ----------*/
#define	ISDBT_DEMUX_SCAN_TIMEOUT	(7)			//unit: sec
#define ISDBT_DEMUX_EPG_SCAN_TIMEOUT    (60)    //unit: sec
#define	ISDBT_DEMUX_SCAN_TIMEOUT_NIT	(2000)	//unit: msec
#define ISDBT_DEMUX_SCAN_TIMEOUT_PAT	(500)	//unit: msec
#define ISDBT_DEMUX_SCAN_TIMEOUT_PMTs	(1000)	//unit: msec. only pmts of fullseg
#define TCC_MANAGER_DEMUX_SCAN_CHECKTIME (20)	//unit: msec. check state every this time

#define 	MAX_1SEG_SERVICE_ID_CNT		8
#define	MAXSEARCH_LIST	32//64

#define	LOOP_LIMIT_256	256

/****************************************************************************
DEFINITION OF EXTERNAL VARIABLES
****************************************************************************/
ISDBT_EIT_CAPTION_INFO gEITCaptionInfo;

/****************************************************************************
DEFINITION OF EXTERNAL FUNCTIONS
****************************************************************************/

/****************************************************************************
DEFINITION OF GLOBAL VARIABLES
****************************************************************************/
static pthread_t SectionDecoderThreadID;
static tcc_dxb_queue_t* SectionDecoderQueue;
static int giSectionThreadRunning = 0;
static int giSectionDecoderState = 0;
static int giSectionDecoderFull = 0;

static CUSTOM_FILTER_LIST gstCustomFilterlist;
static DEMUX_MGR_DS gstDataService;
static pthread_mutex_t g_DataServiceMutex;    // 20161005 : Noah
static int giSearchList[MAXSEARCH_LIST];
static int giInformalSearchList[MAXSEARCH_LIST];
static ISDBT_PARTIAL_RECEPTION_ATTRIBUTE	 stPartial_attribute;
static int	gi1SEG_scan_FoundablePMTCount ;
static int gi1SEG_scan_FoundPMTCount;
static int giScanSkipSDT = 0;
static int giScanDoneFlag = 0;
static int g_ISDBT_UpdateChannelFlag;
static E_DEMUX_STATE g_demux_state = E_DEMUX_STATE_IDLE;
static unsigned int usCheckServiceID[MAX_SUPPORT_CURRENT_SERVICE];
static int iPrevParseSvcNum = 0, iCheckSvcNum = 0, iParseSvcNum = 0, iCheckServiceInfo = 0;
int gCurrentServiceID =-1, gCurrentChannel = -1, gCurrentFrequency = -1, gCurrentCountryCode = -1, gServiceValid = -1;
unsigned int gPlayingServiceID =0, gSetServiceID = 0;
static int g_caption_onoff = 0, g_super_onoff = 0;

int gCurrentPMTPID=-1, gCurrentPMTSubPID=-1;
tcc_dxb_sem_t	*gpDemuxScanWait;

static short gECM_PID;
static short gEMM_PID;

int (*fnGetBer)(int *);
static int gTunerGetStrIdxTime = 0;
static int gLangIndex = -1, gEITSubtitleLangInfo = 0;

static int giDemuxScanStop;

static pthread_mutex_t mutex_demux_filter;
/*----- EWS -----*/
struct _ews_info {
	E_EWS_STATE	ews_state;
	int old_signal;
	int cur_signal;
	int reset_request;
};
static struct _ews_info ews_info;

/*----- back scan -----*/
struct _backscan_info {
	int request;
	int pmt_count;
	unsigned int f_section_found;
	unsigned short int service_id[MAX_SUPPORT_PLAYBACKSCAN_PMTS];
};
static struct _backscan_info backscan_info;

/*----- special service -----*/
#define SPECIALSERVICE_TABLE_PAT (1<<0)
#define SPECIALSERVICE_TABLE_PMT (1<<1)
#define SPECIALSERVICE_TABLE_SDT (1<<2)
#define SPECIALSERVICE_TABLE_MASK (SPECIALSERVICE_TABLE_PAT | SPECIALSERVICE_TABLE_PMT | SPECIALSERVICE_TABLE_SDT)
struct _special_service_info {
	E_SPECIAL_SERVICE_STATE state;
	unsigned short transport_stream_id;
	unsigned short original_network_id;
	unsigned short service_id;	// service_id of special service
	unsigned char service_type;	// service_type of special service
	int _id_channels;			// _id of channel table of SQL DB
	unsigned int ftable;		// bit0=PAT, bit1=PMT, bit2=SDT
};
struct _special_service_info special_service_info;

/*----- section notification -----*/
struct _section_notification_info {
	int request_ids;
	int response_ids;
};
static struct _section_notification_info section_notification_info = {0, 0};

/*----- scan ------*/
struct _scan_info {
	int state;
	int count_complete;
	int count_nit;
	int count_pat;
	int count_pmts;
	int fNit;
	int fPat;
	int isInfiniteTimer;    // Noah / 20170706 / IM478A-30 / In case of the searchAndSetChannel, NIT & BIT have infinite timer.
	pthread_mutex_t scan_info_lock;
};
static struct _scan_info scan_info;

/*----- logo -----*/
struct _logo_info  {
	unsigned int dwnl_id;
	unsigned int logo_id;
	unsigned short sSimpleLogo[256];
	int simple_logo_len;
	unsigned int service_id_fseg;
	unsigned int service_id_1seg;
	unsigned int network_id;
};
static struct _logo_info logo_info;

/*----- service name -----*/
struct _service_name_info {
	unsigned short service_name_fseg[256];
	int svc_name_len_fseg;
	unsigned int service_id_fseg;

	unsigned short service_name_1seg[256];
	int svc_name_len_1seg;
	unsigned int service_id_1seg;

	unsigned int network_id;
};
static struct _service_name_info service_name_info;


static int giEPGScanFlag = 0;

extern DxBInterface *hInterface;

/* Noah / 20170621 / Added for IM478A-60 (The request additinal API to start/stop EPG processing)
	g_uiEpgOnOff
		0 : EPG( EIT ) Off
		1 : EPG( EIT ) On
*/
static unsigned int g_uiEpgOnOff = 0;

// Noah / 20170706 / IM478A-30 / In case of the searchAndSetChannel, NIT & BIT have infinite timer.
static int g_isAddBit_ForSearchAndSetChannel = 0;	// 0 - BIT is added, 1 - BIT is not added

/****************************************************************************
DEFINITION OF LOCAL FUNCTIONS
****************************************************************************/

/*
  * made by Noah / 20161005 / from 'tcc_manager_demux_dataservice_lock_init' to 'dataservice_lock_off'
  *
  * description
  *   Refer to J&K / IM478A-26 - [ISDB-T] Segmentation fault in libdxbutils.so.
*/
static void tcc_manager_demux_dataservice_lock_init(void)
{
	pthread_mutex_init(&g_DataServiceMutex, NULL);
}

static void tcc_manager_demux_dataservice_lock_deinit(void)
{
	pthread_mutex_destroy(&g_DataServiceMutex);
}

static void tcc_manager_demux_dataservice_lock_on(void)
{
	pthread_mutex_lock(&g_DataServiceMutex);
}

static void tcc_manager_demux_dataservice_lock_off(void)
{
	pthread_mutex_unlock(&g_DataServiceMutex);
}

void tcc_manager_demux_filter_lock_init(void)
{
	pthread_mutex_init(&mutex_demux_filter, NULL);
}

void tcc_manager_demux_filter_lock_deinit(void)
{
	pthread_mutex_destroy(&mutex_demux_filter);
}

void tcc_manager_demux_filter_lock_on(void)
{
	pthread_mutex_lock(&mutex_demux_filter);
}

void tcc_manager_demux_filter_lock_off(void)
{
	pthread_mutex_unlock(&mutex_demux_filter);
}

void tcc_manager_demux_set_section_decoder_state(int state)
{
	giSectionDecoderState = state;
}

int tcc_manager_demux_get_section_decoder_state(void)
{
	return giSectionDecoderState;
}

/* ---- cancel scanning channels -----*/
int tcc_manager_demux_scanflag_init(void)
{
	giDemuxScanStop = 0;
	return 0;
}
int tcc_manager_demux_scanflag_get(void)
{
	return giDemuxScanStop;
}

/* ------- ews ----------*/
void tcc_manager_demux_ews_reset (void)
{
	ews_info.old_signal = 0;
	ews_info.cur_signal = 0;
	ews_info.ews_state = EWS_STOP;
	ews_info.reset_request = 0;
}
void tcc_manager_demux_ews_setrequest(int arg)
{
	ews_info.reset_request = arg;
}
int tcc_manager_demux_ews_getrequest(void)
{
	return ews_info.reset_request;
}
void tcc_manager_demux_ews_setstate(E_EWS_STATE new_state)
{
	if (new_state <= EWS_STOP)
		ews_info.ews_state = new_state;
}
E_EWS_STATE tcc_manager_demux_ews_getstate(void)
{
	return ews_info.ews_state;
}

/* --------- backscan  ----------*/
void tcc_manager_demux_backscan_reset(void)
{
	backscan_info.request = 0;
	backscan_info.pmt_count = 0;
	backscan_info.f_section_found = 0;
	memset(backscan_info.service_id, 0, sizeof(unsigned short int)*MAX_SUPPORT_PLAYBACKSCAN_PMTS);
}
void tcc_manager_demux_backscan_start_scanfullseg(void)
{
	backscan_info.request |= DEMUX_BACKSCAN_REQUEST_SEARCH_FULLSEG;
	backscan_info.pmt_count = 0;
	backscan_info.f_section_found = (DEMUX_BACKSCAN_NIT | DEMUX_BACKSCAN_PAT \
				| DEMUX_BACKSCAN_PMT | DEMUX_BACKSCAN_BIT | DEMUX_BACKSCAN_SDT);
}
int	tcc_manager_demux_backscan_isbeing_scanfullseg(void)
{
	if (backscan_info.request & DEMUX_BACKSCAN_REQUEST_SEARCH_FULLSEG) return 1;
	else return 0;
}
void tcc_manager_demux_backscan_stop_scanfullseg(void)
{
	backscan_info.request	&= ~DEMUX_BACKSCAN_REQUEST_SEARCH_FULLSEG;
}
void tcc_manager_demux_backscan_reset_pmtcount(void)
{
	backscan_info.pmt_count = 0;
}
int tcc_manager_demux_backscan_get_pmtcount(void)
{
	return backscan_info.pmt_count;
}
int tcc_manager_demux_backscan_add_service(unsigned short int service_id)
{
	backscan_info.service_id[backscan_info.pmt_count++] = service_id;
	return backscan_info.pmt_count;
}
void tcc_manager_demux_backscan_update_sectioninfo(int mode, unsigned int flag)
{
	if (mode==1) {
		backscan_info.f_section_found |= flag;
	} else if (mode==0) {
		backscan_info.f_section_found &= ~flag;
	} else if (mode==-1) {
		backscan_info.f_section_found = 0;
	}
}
unsigned int tcc_manager_demux_backscan_get_sectioninfo(void)
{
	return backscan_info.f_section_found;
}
unsigned short int tcc_manager_demux_backscan_get_serviceid(int index)
{
	return backscan_info.service_id[index];
}

/* ----- special service -----*/
void tcc_manager_demux_specialservice_init(void)
{
	memset (&special_service_info, 0, sizeof(struct _special_service_info));
	special_service_info.state = SPECIAL_SERVICE_STOP;
}
E_SPECIAL_SERVICE_STATE tcc_manager_demux_specialservice_get_state(void)
{
	return special_service_info.state;
}
void tcc_manager_demux_specialservice_set_state(E_SPECIAL_SERVICE_STATE new_state)
{
	special_service_info.state = new_state;
}
void tcc_manager_demux_specialservice_set_serviceinfo (unsigned short service_id, unsigned char service_type, unsigned short transport_stream_id, unsigned short original_network_id)
{
	special_service_info.service_id = service_id;
	special_service_info.service_type = service_type;
	special_service_info.transport_stream_id = transport_stream_id;
	special_service_info.original_network_id = original_network_id;
}
void tcc_manager_demux_specialservice_set_dbinfo(int db_table_id)
{
	special_service_info._id_channels= db_table_id;
}
void tcc_manager_demux_specialservice_set_tableinfo(unsigned int ftable)
{
	special_service_info.ftable |= ftable;
}
void tcc_manager_demux_specialservice_clear_tableinfo(void)
{
	special_service_info.ftable = 0;
}
unsigned int tcc_manager_demux_specialservice_get_tableinfo(void)
{
	return special_service_info.ftable;
}
void tcc_manager_demux_specialservice_DeleteDB(void)
{
	UpdateDB_SpecialService_DelInfo();
}

/*----- notify detect section -----*/
int tcc_manager_demux_set_section_notification_ids(int iSectionIDs)
{
	section_notification_info.request_ids = iSectionIDs;
	section_notification_info.response_ids = iSectionIDs;

	return 0;
}

void tcc_manager_demux_reset_section_notification(void)
{
	section_notification_info.response_ids = section_notification_info.request_ids;
}

int tcc_manager_demux_specialservice_find_specialservice(unsigned short *service_id, unsigned char *service_type, unsigned short *transport_stream_id, unsigned short *original_network_id)
{
	SI_MGMT_SECTION *pSection = (SI_MGMT_SECTION*)NULL;
	SI_MGMT_SECTIONLOOP *pSecLoop = (SI_MGMT_SECTIONLOOP *)NULL;
	DESCRIPTOR_STRUCT *pDesc;
	int status, cnt, svc_count;

	status = TSPARSE_ISDBT_SiMgmt_Util_GetNITSection (&pSection);
	if (status != TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pSection)) return -1;
	status = TSPARSE_ISDBT_SiMgmt_Section_GetLoop(&pSecLoop, pSection);
	if (status != TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pSecLoop)) return -1;

	for (cnt=0; cnt < LOOP_LIMIT_256 && CHK_VALIDPTR(pSecLoop); cnt++) {
		pDesc = (DESCRIPTOR_STRUCT *)NULL;
		status = TSPARSE_ISDBT_SiMgmt_SectionLoop_FindDescriptor (&pDesc, SERVICE_LIST_DESCR_ID, NULL, pSecLoop);
		if (status==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pDesc)) {
			for (svc_count=0; svc_count < pDesc->unDesc.stSLD.ucSvcListCount; svc_count++) {
				if (pDesc->unDesc.stSLD.pastSvcList[svc_count].enSvcType == 0xA1) {
					*service_id = pDesc->unDesc.stSLD.pastSvcList[svc_count].uiServiceID;
					*service_type = pDesc->unDesc.stSLD.pastSvcList[svc_count].enSvcType;
					*transport_stream_id = pSecLoop->unSectionLoopData.nit_loop.transport_stream_id;
					*original_network_id = pSecLoop->unSectionLoopData.nit_loop.original_network_id;
					return 0;
				}
			}
		}
		pSecLoop = pSecLoop->pNext;
	}
	return -1;
}
int tcc_manager_demux_specialservice_wait_pat (unsigned short *pid_of_pmt, unsigned short service_id)
{
	SI_MGMT_SECTION *pSection = (SI_MGMT_SECTION*)NULL;
	SI_MGMT_SECTIONLOOP *pSecLoop = (SI_MGMT_SECTIONLOOP *)NULL;
	int status, cnt;

	status = TSPARSE_ISDBT_SiMgmt_Util_GetPATSection (&pSection);
	if (status != TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pSection)) return -1;
	status = TSPARSE_ISDBT_SiMgmt_Section_GetLoop(&pSecLoop, pSection);
	if (status != TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pSecLoop)) return -1;

	for (cnt=0; cnt < LOOP_LIMIT_256 && CHK_VALIDPTR(pSecLoop); cnt++) {
		if (pSecLoop->unSectionLoopData.pat_loop.program_number == service_id) {
			*pid_of_pmt = pSecLoop->unSectionLoopData.pat_loop.program_map_pid;
			return 0;
		}
		pSecLoop = pSecLoop->pNext;
	}
	return -1;
}
int tcc_manager_demux_specialservice_wait_pmt (unsigned short service_id)
{
	SI_MGMT_TABLE_ROOT *pTableRoot = (SI_MGMT_TABLE_ROOT*)NULL;
	SI_MGMT_TABLE *pTable = (SI_MGMT_TABLE*)NULL;
	SI_MGMT_TABLE_EXTENSION unTableExt;
	MPEG_TABLE_IDS table_id = PMT_ID;
	int status, cnt;

	status = TSPARSE_ISDBT_SiMgmt_GetTableRoot(&pTableRoot, table_id);
	if(!CHK_VALIDPTR(pTableRoot))
		return -1;

	unTableExt.pmt_ext.program_number = service_id;
	status = TSPARSE_ISDBT_SiMgmt_FindTable(&pTable, table_id, &unTableExt, pTableRoot);
	if(status!=TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pTable)) return -1;
	return 0;
}
int tcc_manager_demux_specialservice_wait_sdt (unsigned short service_id, unsigned short transport_stream_id, unsigned short original_network_id)
{
	SI_MGMT_TABLE_ROOT *pTableRoot = (SI_MGMT_TABLE_ROOT*)NULL;
	SI_MGMT_TABLE *pTable = (SI_MGMT_TABLE*)NULL;
	SI_MGMT_TABLE_EXTENSION unTableExt;
	SI_MGMT_SECTION *pSection = (SI_MGMT_SECTION*)NULL;
	SI_MGMT_SECTIONLOOP *pSecLoop;
	SI_MGMT_SECTIONLOOP_DATA unSectionLoopData;
	MPEG_TABLE_IDS table_id = SDT_A_ID;
	int status, cnt;

	status = TSPARSE_ISDBT_SiMgmt_GetTableRoot(&pTableRoot, table_id);
	if(!CHK_VALIDPTR(pTableRoot))
		return -1;

	unTableExt.sdt_ext.transport_stream_id = transport_stream_id;
	unTableExt.sdt_ext.original_network_id = original_network_id;
	status = TSPARSE_ISDBT_SiMgmt_FindTable(&pTable, table_id, &unTableExt, pTableRoot);
	if(status!=TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pTable)) return -1;
	status = TSPARSE_ISDBT_SiMgmt_Table_GetSection(&pSection, eSECTION_CURR, pTable);
	if(status!=TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pSection)) return -1;

	unSectionLoopData.sdt_loop.service_id = service_id;
	for(cnt=0; cnt < LOOP_LIMIT_256 && CHK_VALIDPTR(pSection); cnt++) {
		pSecLoop = (SI_MGMT_SECTIONLOOP *)NULL;
		status = TSPARSE_ISDBT_SiMgmt_Section_FindLoop (&pSecLoop,  &unSectionLoopData, pSection);
		if (status==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pSecLoop))
			return 0;
		pSection = pSection->pNext;
	}
	return -1;
}
/* [state:none] try to find special service from NIT. */
int tcc_manager_demux_specialservice_search(void)
{
	unsigned short service_id, transport_stream_id, original_network_id;
	unsigned char service_type;
	int find_result;

	find_result = tcc_manager_demux_specialservice_find_specialservice(&service_id, &service_type, &transport_stream_id, &original_network_id);
	if (find_result==0) {
		tcc_manager_demux_specialservice_clear_tableinfo();
		tcc_manager_demux_specialservice_set_serviceinfo (service_id, service_type, transport_stream_id, original_network_id);
		tcc_manager_demux_specialservice_set_state (SPECIAL_SERVICE_START_WAITING);
	}
	return find_result;
}
/* [state:start_waiting] search all information required to play a service. or check for sepcial service to disappear */
int tcc_manager_demux_specialservice_waiting(void)
{
	unsigned short service_id, pmt_pid, playing_service_id, transport_stream_id, original_network_id;
	unsigned char service_type;
	int status, row_id;

	status = tcc_manager_demux_specialservice_find_specialservice(&service_id, &service_type, &transport_stream_id, &original_network_id);
	if (status!=0) {	/* if a special service is finished */
		tcc_manager_demux_specialservice_init();
		tcc_manager_demux_specialservice_set_state (SPECIAL_SERVICE_NONE);
	} else {
		if (!(tcc_manager_demux_specialservice_get_tableinfo() & SPECIALSERVICE_TABLE_PAT)) {	/* if pat is not yet received */
			status = tcc_manager_demux_specialservice_wait_pat (&pmt_pid, service_id);
			if (status==0) tcc_manager_demux_specialservice_set_tableinfo(SPECIALSERVICE_TABLE_PAT);
		}

		if (!(tcc_manager_demux_specialservice_get_tableinfo() & SPECIALSERVICE_TABLE_PMT)) {	/* if pmt is not yet received */
			status = tcc_manager_demux_specialservice_wait_pmt (service_id);
			if (status==0) tcc_manager_demux_specialservice_set_tableinfo(SPECIALSERVICE_TABLE_PMT);
		}

		if (!(tcc_manager_demux_specialservice_get_tableinfo() & SPECIALSERVICE_TABLE_SDT)) {	/* if sdt is not yet received */
			status = tcc_manager_demux_specialservice_wait_sdt (service_id, transport_stream_id, original_network_id);
			if (status==0) tcc_manager_demux_specialservice_set_tableinfo(SPECIALSERVICE_TABLE_SDT);
		}

		if ((tcc_manager_demux_specialservice_get_tableinfo() & SPECIALSERVICE_TABLE_MASK) == SPECIALSERVICE_TABLE_MASK) {
			playing_service_id = (unsigned short) tcc_manager_demux_get_playing_serviceID();
			if (playing_service_id != service_id) {
				/* save a channel informatoin of special service and issue an event only when in playing regular service */
				status = UpdateDB_SpecialService_UpdateInfo (gCurrentChannel, gCurrentCountryCode, service_id, &row_id );
				if (status != 0) {
					tcc_manager_demux_specialservice_init();
					tcc_manager_demux_specialservice_set_state(SPECIAL_SERVICE_NONE);
					return -1;
				}
				tcc_manager_demux_specialservice_set_dbinfo(row_id);

				TCCDxBEvent_SpecialServiceUpdate (1, row_id);	/* special service started */
			}
			/* state change */
			tcc_manager_demux_specialservice_set_state (SPECIAL_SERVICE_RUNNING);
		}
	}

	return 0;
}
/* [state:running]check for special serice to disappear */
int tcc_manager_demux_specialservice_running(void)
{
	unsigned short service_id, transport_stream_id, original_network_id;
	unsigned char service_type;
	int status;

	status = tcc_manager_demux_specialservice_find_specialservice(&service_id, &service_type, &transport_stream_id, &original_network_id);
	if (status!=0) {	/* if a special service is finished */
		TCCDxBEvent_SpecialServiceUpdate (2, 0); /* special service finished */
		tcc_manager_demux_specialservice_init();
		tcc_manager_demux_specialservice_set_state(SPECIAL_SERVICE_NONE);
		tcc_manager_demux_specialservice_DeleteDB();
	}
	return 0;
}
void tcc_manager_demux_specialservice_process (void)
{
	E_SPECIAL_SERVICE_STATE state;
	int special_service_support;

	special_service_support = TCC_Isdbt_IsSupportSpecialService();
	if (special_service_support) {
		state = tcc_manager_demux_specialservice_get_state();
		switch (state) {
			case SPECIAL_SERVICE_STOP:
				/* do nothing */
				break;
			case SPECIAL_SERVICE_NONE:
				tcc_manager_demux_specialservice_search();
				break;
			case SPECIAL_SERVICE_START_WAITING:
				tcc_manager_demux_specialservice_waiting();
				break;
			case SPECIAL_SERVICE_RUNNING:
				tcc_manager_demux_specialservice_running();
				break;
			default:
				ALOGE("In %s, state is invalid(%d)", __func__, state);
				break;
		}
	}
}

/*------------- scan -------------*/
/* these routine will be called only within scan logic.			*/
/* if other thread than tcc_dxb_proc_thread, these routine should be protected by using lock/unlock */
void tcc_manager_demux_scan_init (void)
{
	scan_info.state = SCAN_STATE_NONE;
	scan_info.count_complete = (ISDBT_DEMUX_SCAN_TIMEOUT * 1000 / TCC_MANAGER_DEMUX_SCAN_CHECKTIME);
	scan_info.count_nit = (ISDBT_DEMUX_SCAN_TIMEOUT_NIT / TCC_MANAGER_DEMUX_SCAN_CHECKTIME);
	scan_info.count_pat = (ISDBT_DEMUX_SCAN_TIMEOUT_PAT / TCC_MANAGER_DEMUX_SCAN_CHECKTIME);
	scan_info.count_pmts = (ISDBT_DEMUX_SCAN_TIMEOUT_PMTs / TCC_MANAGER_DEMUX_SCAN_CHECKTIME);
	scan_info.fNit = 0;
	scan_info.fPat = 0;
	scan_info.isInfiniteTimer = 0;	// Noah / 20170706 / IM478A-30 / In case of the searchAndSetChannel, NIT & BIT have infinite timer.
}

/*
  * made by Noah / 2016081X / reset count_pmts.
*/
void tcc_manager_demux_scan_init_only_pmts (void)
{
	scan_info.count_pmts = (ISDBT_DEMUX_SCAN_TIMEOUT_PMTs / TCC_MANAGER_DEMUX_SCAN_CHECKTIME);
}

void tcc_manager_demux_scan_set_state(E_SCAN_STATE state)
{
	scan_info.state = state;
}

E_SCAN_STATE tcc_manager_demux_scan_get_state(void)
{
	E_SCAN_STATE state;
	state = scan_info.state;
	return state;
}

/*
	return value
		0 : is processing
		1 : timeout
*/
int tcc_manager_demux_scan_check_timeout(void)
{
	int rtn = 0;

	// Noah / 20170706 / IM478A-30 / In case of the searchAndSetChannel, NIT & BIT have infinite timer.
	if(scan_info.isInfiniteTimer == 1)
	{
		if (!tcc_manager_demux_scan_get_nit())		// If NIT is not processed
			return 0;

		if (1 == g_isAddBit_ForSearchAndSetChannel)	// If BIT is not processed
			return 0;

        /*
            Noah, 20181011, IM692A-13 (TVK does not recieve at all)
                Precondition
                    TVK signal is enough for 1seg, but 12seg can not be received.
                Problem
                    App calls searchAndSetChannel for TVK, but no A/V is output at all.
                Root Cause
                    1. TVK has 2nd 1seg service in 'servcie list descriptor' of NIT, but the actual 2nd 1seg service does not exist.
                    2. Side effect of IM478A-30 ([ISDB-T] (Improvement No3) At a weak signal, improvement of "searchAndSetChannel").
                        Modification of IM478A-30 : Infinite timer works for NIT, BIT and 1seg PMTs when searchAndSetChannel is called.
                        DxB waits 2 kind of 1seg PMTs of TVK forever, but 2nd 1seg PMT does not exist.
                        Therefore, the scan routine of searchAndSetChannel does not finish.
                Patch
                    It removes 1seg PMT from infinite timer while scanning of searchAndSetChannel.
                    It means the following 2 lines are commented out.
        */
		//if (0 != gi1SEG_scan_FoundablePMTCount)		// If PMTs of the 1seg are not processed
		//	return 0;
	}

	if (scan_info.count_complete > 0)
		scan_info.count_complete--;

	// Noah / 2016081X /
	if (scan_info.count_complete <= 0)
	{
		ALOGE("%s / Whole Timeout( 7s ) is expired in %d state\n", __func__, scan_info.state);
		return 1;
	}

	switch(scan_info.state)
	{
		case SCAN_STATE_NONE:
			rtn=1;
			break;

		case SCAN_STATE_NIT:
			if (scan_info.count_nit > 0)
				scan_info.count_nit--;

			if (scan_info.count_nit <= 0)
				rtn=1;
			break;

		case SCAN_STATE_PAT:
			if (scan_info.count_pat > 0)
				scan_info.count_pat--;

			if (scan_info.count_pat <= 0)
				rtn=1;
			break;

		// Noah / 2016081X /
		case SCAN_STATE_PMTs:
			if (scan_info.count_pmts > 0)
				scan_info.count_pmts--;

			if (scan_info.count_pmts <= 0)
				rtn=1;
			break;

		case SCAN_STATE_COMPLETE:
			// Noah, To avoid Codesonar's warning, Redundant Condition
			// The following if statement is already checked above.
			//if (scan_info.count_complete <= 0) rtn = 1;
			break;

		default:
			rtn=1;
			break;
	}

	return rtn;
}
int tcc_manager_demux_scan_get_nit(void)
{
	int fNit;
	fNit = scan_info.fNit;
	return fNit;
}
void tcc_manager_demux_scan_set_nit(int fNit)
{
	scan_info.fNit = fNit;
}
int tcc_manager_demux_scan_get_pat(void)
{
	int fPat;
	fPat = scan_info.fPat;
	return fPat;
}
void tcc_manager_demux_scan_set_pat(int fPat)
{
	scan_info.fPat = fPat;
}

/* // Noah / 20170706 / IM478A-30 / In case of the searchAndSetChannel, NIT & BIT have infinite timer.
	infiniteTimer
		0 : Not infinite timer
		1 : Infinite timer
*/
void tcc_manager_demux_scan_set_infinite_timer(int infiniteTimer)
{
	scan_info.isInfiniteTimer = infiniteTimer;
}

void tcc_manager_demux_set_skip_sdt_in_scan(int enable)
{
	giScanSkipSDT = enable;
}

int tcc_manager_demux_is_skip_sdt_in_scan(void)
{
	if(TCC_Isdbt_IsSkipSDTInScan() || (giScanSkipSDT != 0)) {
		return TRUE;
	}

	return FALSE;
}

/*---------- logo ----------*/
void tcc_manager_demux_logo_init (void)
{
	logo_info.dwnl_id = -1;
	logo_info.logo_id  = -1;
	logo_info.simple_logo_len = 0;
	logo_info.service_id_fseg = 0;
	logo_info.service_id_1seg = 0;
}
void tcc_manager_demux_logo_get_infoDB (unsigned int channel_no, unsigned int country_code, unsigned int network_id, unsigned int svc_id_f, unsigned int svc_id_1)
{
	int err;
	unsigned int dwnl_id, logo_id;
	unsigned short simple_logo_str[256];
	unsigned int simple_logo_len;

	if (svc_id_f)
	{
		simple_logo_len = 256 * sizeof(unsigned short);
		err = UpdateDB_GetInfoLogoDB (channel_no, country_code, network_id, svc_id_f, &dwnl_id, &logo_id, simple_logo_str, &simple_logo_len);
		if (err == 0)
		{
			logo_info.dwnl_id = dwnl_id;
			logo_info.logo_id = logo_id;
			logo_info.service_id_fseg = svc_id_f;
		}
	}

	if (svc_id_1)
	{
		simple_logo_len = 256 * sizeof(unsigned short);
		err = UpdateDB_GetInfoLogoDB (channel_no, country_code, network_id, svc_id_1, &dwnl_id, &logo_id, simple_logo_str, &simple_logo_len);
		if (err == 0)
		{
			if(simple_logo_len != 0)
			{
				memset(logo_info.sSimpleLogo, 0, 256*sizeof(unsigned short));
				memcpy(logo_info.sSimpleLogo, simple_logo_str, simple_logo_len);
				logo_info.simple_logo_len = simple_logo_len;
				logo_info.service_id_1seg = svc_id_1;
			}
		}
	}
	logo_info.network_id = network_id;
}

void tcc_manager_demux_logo_update_infoDB(void)
{
	int err;
	err = UpdateDB_UpdateInfoLogoDB (gCurrentChannel, gCurrentCountryCode, logo_info.network_id, logo_info.dwnl_id, logo_info.logo_id, logo_info.sSimpleLogo, logo_info.simple_logo_len);
	if (!err) /* update logo_info only when new information is written to DB */
		tcc_manager_demux_logo_get_infoDB (gCurrentChannel, gCurrentCountryCode, logo_info.network_id, logo_info.service_id_fseg, logo_info.service_id_1seg);
}

/*----------- channel name ----------*/
void tcc_manager_demux_channelname_init(void)
{
	service_name_info.network_id = 0;
	memset(service_name_info.service_name_fseg, 0, 256*sizeof(unsigned short));
	service_name_info.svc_name_len_fseg = 0;
	memset(service_name_info.service_name_1seg, 0, 256*sizeof(unsigned short));
	service_name_info.svc_name_len_1seg = 0;

	service_name_info.service_id_fseg = 0;
	service_name_info.service_id_1seg = 0;
}
void tcc_manager_demux_channelname_get_infoDB(unsigned int channel_no, unsigned int country_code, unsigned int network_id, unsigned int svc_id_f, unsigned int svc_id_1)
{
	unsigned int svc_name_len;
	unsigned short svc_name[256];
	int err;

	service_name_info.network_id = network_id;
	if (svc_id_f)
	{
		svc_name_len = 256 * sizeof(unsigned short);
		err = UpdateDB_GetInfoChannelNameDB (channel_no, country_code, network_id, svc_id_f, svc_name, &svc_name_len);
		if(err == 0)
		{
			if(svc_name_len != 0)
			{
				memset (service_name_info.service_name_fseg, 0, 256*sizeof(unsigned short));
				memcpy (service_name_info.service_name_fseg, svc_name, svc_name_len);
				service_name_info.svc_name_len_fseg = svc_name_len;
			}
		}

		service_name_info.service_id_fseg = svc_id_f;
	}

	if(svc_id_1)
	{
		svc_name_len = 256 * sizeof(unsigned short);
		err = UpdateDB_GetInfoChannelNameDB (channel_no, country_code, network_id, svc_id_1, svc_name, &svc_name_len);
		if(err == 0)
		{
			if(svc_name_len != 0)
			{
				memset (service_name_info.service_name_1seg, 0, 256*sizeof(unsigned short));
				memcpy (service_name_info.service_name_1seg, svc_name, svc_name_len);
				service_name_info.svc_name_len_1seg = svc_name_len;
			}
		}

		service_name_info.service_id_1seg = svc_id_1;
	}
}
void tcc_manager_demux_channelname_update_infoDB (void)
{
#if 0    // Original

	int err;
	int fUpdate = 0;
	if ((service_name_info.service_id_fseg != 0) && (service_name_info.svc_name_len_fseg == 0)) fUpdate = 1;
	if ((service_name_info.service_id_1seg != 0) && (service_name_info.svc_name_len_1seg == 0)) fUpdate = 1;
	if (fUpdate) {
		err = UpdateDB_UpdateChannelNameDB (gCurrentChannel, gCurrentCountryCode, service_name_info.network_id);
		if (err == 0)
			tcc_manager_demux_channelname_get_infoDB(gCurrentChannel, gCurrentCountryCode, service_name_info.network_id, service_name_info.service_id_fseg, service_name_info.service_id_1seg);
	}

#else    // Noah / 20180131 / IM478A-77 (relay station search in the background using 2TS) / Update 'channelName' field of DB forcibly

	int err;

	err = UpdateDB_UpdateChannelNameDB(gCurrentChannel, gCurrentCountryCode, service_name_info.network_id);
	if(err == 0)
	{
		tcc_manager_demux_channelname_get_infoDB(gCurrentChannel,
												gCurrentCountryCode,
												service_name_info.network_id,
												service_name_info.service_id_fseg,
												service_name_info.service_id_1seg);
	}

#endif
}

/*--------- demux -------*/
void tcc_manager_demux_set_state(E_DEMUX_STATE state)
{
	if(state >= E_DEMUX_STATE_MAX){
		ALOGE("[%s] Invalid demux_state\n", __func__);
		return;
	}

	g_demux_state = state;
}

E_DEMUX_STATE tcc_manager_demux_get_state(void)
{
	return g_demux_state;
}

int tcc_manager_demux_is_epgscan(void)
{
	return giEPGScanFlag;
}

void tcc_manager_set_updatechannel_flag (int arg)
{
	g_ISDBT_UpdateChannelFlag = arg;
}

int tcc_manager_get_updatechannel_flag(void)
{
	return g_ISDBT_UpdateChannelFlag;
}

static int tcc_manager_check_PmtProfile (int PmtPID)
{
	if((PmtPID <= 0) || (PmtPID > 0x1FFF))
		return -1;

	if (PmtPID >= PID_PMT_1SEG && PmtPID < (PID_PMT_1SEG+8))
		return	0;
	else
		return	1;
}

void tcc_demux_init_searchlist(void)
{
	int i;
	for(i=0; i<MAXSEARCH_LIST; i++)
	{
		giSearchList[i] = -1;
		giInformalSearchList[i] = -1;
	}
}

int tcc_demux_add_searchlist(int iReqID)
{
	int i;
	for(i=0; i<MAXSEARCH_LIST; i++)
	{
		if( giSearchList[i] == -1)
		{
			giSearchList[i] = iReqID;
			giInformalSearchList[i] = iReqID;
			break;
		}
	}
	if( i==MAXSEARCH_LIST )
	{
		ALOGE("tcc_demux_add_searchlist failed(%d)", iReqID);
		return 1;
	}

	return 0;
}

int tcc_demux_delete_searchlist(int iReqID)
{
	int i;
	for(i=0; i<MAXSEARCH_LIST; i++)
	{
		if( giSearchList[i] == iReqID )
		{
			giSearchList[i] = -1;
			giInformalSearchList[i] = -1;
			break;
		}
	}
	if( i == MAXSEARCH_LIST )
		return 1;

	return 0;
}

int tcc_demux_delete_informal_searchlist(int iReqID)
{
	int i;
	for(i=0; i<MAXSEARCH_LIST; i++)
	{
		if( giSearchList[i] == iReqID )
		{
			giInformalSearchList[i] = -1;
			break;
		}
	}
	if( i == MAXSEARCH_LIST )
		return 1;

	return 0;
}

int tcc_demux_check_searchlist(int iReqID)
{
	int i;
	for(i=0; i<MAXSEARCH_LIST; i++)
	{
		if( giSearchList[i] == iReqID )
			break;
	}

	if( i == MAXSEARCH_LIST )
		return 1;

	return 0;
}

int tcc_demux_get_valid_searchlist(void)
{
	int ivalidnum = 0, i;
	for(i=0; i<MAXSEARCH_LIST; i++)
	{
		if( giSearchList[i] != -1 )
		{
			ivalidnum++;
		}
	}
	return ivalidnum;
}

int tcc_demux_get_valid_informal_searchlist(void)
{
	int ivalidnum = 0, i;
	for(i=0; i<MAXSEARCH_LIST; i++)
	{
		if( giInformalSearchList[i] != -1 && giInformalSearchList[i] != DEMUX_REQUEST_ECM )
		{
			ivalidnum++;
		}
	}
	return ivalidnum;
}

int tcc_demux_unload_all_searchlist_filters(void)
{
    int i;
	tcc_manager_demux_filter_lock_on();
	for(i=0; i<MAXSEARCH_LIST; i++)
	{
		if( giSearchList[i] != -1 )
        {
            ALOGD("%s:%d:[%d]", __func__, __LINE__, giSearchList[i]);
    		TCC_DxB_DEMUX_StopSectionFilter(hInterface, giSearchList[i]);
			tcc_demux_delete_searchlist(giSearchList[i]);
    		giSearchList[i] = -1;
			giInformalSearchList[i] = -1;
        }
	}
	tcc_manager_demux_filter_lock_off();
    return 0;
}

static void tcc_demux_customfilterlist_init(void)
{
	int i;
	for(i=0; i<MAX_CUSTOM_FILTER; i++)
	{
		gstCustomFilterlist.customFilter[i].iPID = -1;
		gstCustomFilterlist.customFilter[i].iTableID = -1;
		gstCustomFilterlist.customFilter[i].iRequestID = -1;
	}
}

static int tcc_demux_customfilterlist_add(int iPID, int iTableID, int iReqID)
{
	int i;
	for(i=0; i<MAX_CUSTOM_FILTER; i++) {
		if(gstCustomFilterlist.customFilter[i].iPID == -1) {
			gstCustomFilterlist.customFilter[i].iPID = iPID;
			gstCustomFilterlist.customFilter[i].iTableID = iTableID;
			gstCustomFilterlist.customFilter[i].iRequestID = iReqID;
			return 1;
		}
	}
	ALOGE("%s failed(0x%X, 0x%X)(%d)", __func__, iPID, iTableID, iReqID);
	return 0;
}

static int tcc_demux_customfilterlist_delete(int iPID, int iTableID)
{
	int i;
	for(i=0; i<MAX_CUSTOM_FILTER; i++) {
		if(gstCustomFilterlist.customFilter[i].iPID == iPID
		&& gstCustomFilterlist.customFilter[i].iTableID == iTableID) {
			gstCustomFilterlist.customFilter[i].iPID = -1;
			gstCustomFilterlist.customFilter[i].iTableID = -1;
			gstCustomFilterlist.customFilter[i].iRequestID = -1;
			return 0;
		}
	}
	ALOGE("%s failed(0x%X, 0x%X)", __func__, iPID, iTableID);
	return 1;
}

static int tcc_demux_customfilterlist_getPID(int iReqID)
{
	int i;
	for(i=0; i<MAX_CUSTOM_FILTER; i++)
	{
		if(gstCustomFilterlist.customFilter[i].iRequestID == iReqID)
		{
			return gstCustomFilterlist.customFilter[i].iPID;
		}
	}
	ALOGE("%s failed(%d)", __func__, iReqID);
	return 0;
}

static int tcc_demux_customfilterlist_getRequestID(int iPID, int iTableID)
{
	int i;
	for(i=0; i<MAX_CUSTOM_FILTER; i++)
	{
		if(gstCustomFilterlist.customFilter[i].iPID == iPID
		&& gstCustomFilterlist.customFilter[i].iTableID == iTableID)
		{
			return gstCustomFilterlist.customFilter[i].iRequestID;
		}
	}
	ALOGE("%s failed(0x%X, 0x%X)", __func__, iPID, iTableID);
	return 0;
}

static int tcc_demux_customfilterlist_getIndex(int iPID)
{
	int i;
	for(i=0; i<MAX_CUSTOM_FILTER; i++) {
		if(gstCustomFilterlist.customFilter[i].iPID == -1) {
			return i;
		}
	}
	ALOGE("%s failed(0x%X)", __func__, iPID);
	return 0;
}

static void tcc_demux_customfilterlist_unload(void)
{
	int i;
	for(i=0; i<MAX_CUSTOM_FILTER; i++) {
		if(gstCustomFilterlist.customFilter[i].iPID != -1) {
			ALOGE("%s remain custom filter (0x%X)", __func__, gstCustomFilterlist.customFilter[i].iPID);
			TCC_DxB_DEMUX_StopSectionFilter(hInterface, gstCustomFilterlist.customFilter[i].iRequestID);
			tcc_demux_customfilterlist_delete(gstCustomFilterlist.customFilter[i].iPID, gstCustomFilterlist.customFilter[i].iTableID);
		}
	}
}

int tcc_demux_load_pmts(void)
{
	unsigned char ucValue[16], ucMask[16], ucExclusion[16];
	int ret = 0, count = 0, iPMT = 0;
	unsigned short *pPMT_PID = NULL, *pTMP_PID = NULL;
	unsigned short *pTMPSERVICE_ID = NULL;
	int iRequestID = DEMUX_REQUEST_PMT;

	MNG_DBG("In %s\n", __func__);

	ucValue[0] = PMT_ID;
	ucMask[0] = 0xff;
	ucExclusion[0] = 0x00;

	iPMT = GetCountDB_PMT_PID_ChannelTable();
	MNG_DBG("In %s -Total %d\n", __func__, iPMT);

	if( iPMT > 0 )
	{
#if 0
		pPMT_PID = (unsigned short *)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(short) * iPMT);
#else
		pPMT_PID = (unsigned short *)tcc_mw_calloc(__FUNCTION__, __LINE__, 1, sizeof(short) * iPMT);
#endif

		GetPMT_PID_ChannelTable(pPMT_PID, iPMT);
		pTMP_PID = pPMT_PID;
	}

	if(pTMP_PID == NULL)
		return ret;

	while( count < iPMT )
	{
		if(*pTMP_PID!=0 && (*pTMP_PID <PID_PMT_1SEG || *pTMP_PID > (PID_PMT_1SEG+8)))
		//if(*pTMP_PID!=0 && (*pTMP_PID <0x1FFF))
		{
			MNG_DBG("0x%x PMT Loading\n", *pTMP_PID);
			tcc_manager_demux_filter_lock_on();
			if(tcc_demux_add_searchlist(iRequestID) == 0)
			{
				TCC_DxB_DEMUX_StartSectionFilter(hInterface, *pTMP_PID, iRequestID++, TRUE, 1, ucValue, ucMask, ucExclusion, TRUE, NULL);
			}
			tcc_manager_demux_filter_lock_off();
		}
		pTMP_PID++;
		count++;
	}

	if( pPMT_PID != NULL )
	{
		tcc_mw_free(__FUNCTION__, __LINE__, pPMT_PID);
		pPMT_PID = NULL;
		pTMP_PID = NULL;
	}

	return ret;
}

int tcc_demux_load_pmts_backscan(void)
{
	unsigned char ucValue[16], ucMask[16], ucExclusion[16];
	int ret = 0, count = 0, iPMT = 0;
	unsigned short *pPMT_PID = NULL, *pTMP_PID = NULL;
	unsigned short *pSERVICE_ID = NULL, *pTMPSERVICE_ID = NULL;
	int iRequestID = DEMUX_REQUEST_FSEG_PMT_BACKSCAN;

	MNG_DBG("In %s\n", __func__);

	ucValue[0] = PMT_ID;
	ucMask[0] = 0xff;
	ucExclusion[0] = 0x00;

	iPMT = GetCountDB_PMT_PID_ChannelTable();
	ALOGD("[%s] -Total %d\n", __func__, iPMT);

	if( iPMT > 0 )
	{
		if (iPMT > MAX_SUPPORT_PLAYBACKSCAN_PMTS)
		{
			iPMT = MAX_SUPPORT_PLAYBACKSCAN_PMTS;
		}

#if 0
		pPMT_PID = (unsigned short *)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(short) * iPMT);
#else
		pPMT_PID = (unsigned short *)tcc_mw_calloc(__FUNCTION__, __LINE__, 1, sizeof(short) * iPMT);
#endif
		// jini 9th
		if (pPMT_PID == NULL)
			return ret;

#if 0
		pSERVICE_ID	= (unsigned short *)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(short) * iPMT);
#else
		pSERVICE_ID	= (unsigned short *)tcc_mw_calloc(__FUNCTION__, __LINE__, 1, sizeof(short) * iPMT);
#endif
		// jini 9th
		if (pSERVICE_ID == NULL)
		{
			tcc_mw_free(__FUNCTION__, __LINE__, pPMT_PID);
			return ret;
		}

		GetPMT_PID_ChannelTable(pPMT_PID, iPMT);
		GetServiceID_ChannelTable(pSERVICE_ID, iPMT);

		pTMP_PID		= pPMT_PID;
		pTMPSERVICE_ID	= pSERVICE_ID;

		if(gCurrentPMTPID == -1){
			gCurrentPMTPID = pTMP_PID[0];
		}
	}

	tcc_manager_demux_backscan_reset_pmtcount();
	while (count < iPMT)
	{
		if (*pTMP_PID !=0 && (*pTMP_PID < PID_PMT_1SEG || *pTMP_PID > (PID_PMT_1SEG+8)))
		{
			tcc_manager_demux_filter_lock_on();
			if(tcc_demux_add_searchlist(iRequestID) == 0)
			{
				TCC_DxB_DEMUX_StartSectionFilter(hInterface, *pTMP_PID, iRequestID++, FALSE, 1, ucValue, ucMask, ucExclusion, TRUE, NULL);
			}
			tcc_manager_demux_filter_lock_off();
			tcc_manager_demux_backscan_add_service(*pTMPSERVICE_ID);
		}
		pTMP_PID++;
		pTMPSERVICE_ID++;
		count++;
	}

	ALOGD("[%s] Total PMTs (%d) need to scan \n", __func__, tcc_manager_demux_backscan_get_pmtcount());

	if (pPMT_PID != NULL)
	{
		tcc_mw_free(__FUNCTION__, __LINE__, pPMT_PID);
		pPMT_PID = NULL;
		pTMP_PID = NULL;
	}

	if (pSERVICE_ID != NULL)
	{
		tcc_mw_free(__FUNCTION__, __LINE__, pSERVICE_ID);
		pSERVICE_ID = NULL;
		pTMPSERVICE_ID = NULL;
	}

	return ret;
}

int tcc_demux_check_pmts_backscan(void)
{
	int ret, iRequestID = DEMUX_REQUEST_FSEG_PMT_BACKSCAN;
	tcc_manager_demux_filter_lock_on();
	ret = tcc_demux_check_searchlist(iRequestID);
	tcc_manager_demux_filter_lock_off();
	return ret;
}

/*
  * check service_id to know service number and PMT_PID of corresponding 1seg service.
  * refer to ARIB TR-B14 Vol7. 5.2.9 Operation of Partial Reception
  */
int tcc_demux_parse_servicetype(unsigned int service_id)
{
	int svc_info = -1;
	if (service_id != 0) {
		if (TCC_Isdbt_IsSupportJapan()) svc_info = (int)((service_id & (3<<7))>>7);
		else svc_info = (int)((service_id & (3<<3))>>3);
	}
	return svc_info;
}
int tcc_demux_parse_servicenumber(unsigned int service_id)
{
	int svc_info = -1;
	if (service_id != 0) {
		svc_info = (int)(service_id & 3);
	}
	return svc_info;
}
int tcc_demux_load_1seg_pmts_japan (void)
{
	unsigned char      ucValue[16];
	unsigned char      ucMask[16];
	unsigned char      ucExclusion[16];
	UINT16	pid = 0;
	int	search_cnt, i;
	int iRequestID = DEMUX_REQUEST_1SEG_PMT;
	unsigned int service_ids[MAX_1SEG_SERVICE_ID_CNT];
	int svc_type, svc_num;

	MNG_DBG("In %s\n", __func__);

	/* get 1seg service_id from channel information - check service_number & service_type from service_id of each service */
	memset(&service_ids[0], 0, MAX_1SEG_SERVICE_ID_CNT * sizeof(unsigned int));
	search_cnt = UpdateDB_Channel_Get1SegServiceID(gCurrentChannel, gCurrentCountryCode, service_ids);
	gi1SEG_scan_FoundablePMTCount = 0;
	gi1SEG_scan_FoundPMTCount = 0;
	for (i=0; i < search_cnt && i < MAX_1SEG_SERVICE_ID_CNT; i++) {
		svc_type = tcc_demux_parse_servicetype(service_ids[i]);
		if (svc_type == 3) {	/* 0=TV type, 1,2=Data type (excluding partial reception), 3=Data type (partial reception */
			svc_num = tcc_demux_parse_servicenumber(service_ids[i]);

			pid = PID_PMT_1SEG + svc_num;
			ucValue[0] = PMT_ID;
			ucMask[0] = 0xff;
			ucExclusion[0] = 0x00;
			tcc_manager_demux_filter_lock_on();
			if(tcc_demux_add_searchlist(iRequestID) == 0) {
				TCC_DxB_DEMUX_StartSectionFilter(hInterface, pid, iRequestID++, TRUE, 1, ucValue, ucMask, ucExclusion, TRUE, NULL);
			}
			tcc_manager_demux_filter_lock_off();

			gi1SEG_scan_FoundablePMTCount++;
		}
	}
	return 0;
}

int tcc_demux_load_1seg_pmts(void)
{
	unsigned char      ucValue[16];
	unsigned char      ucMask[16];
	unsigned char      ucExclusion[16];
	UINT16	pid = 0;
	int	search_cnt=0;
	int iRequestID = DEMUX_REQUEST_1SEG_PMT;

	MNG_DBG("In %s\n", __func__);

	gi1SEG_scan_FoundablePMTCount = ISDBT_Get_1SegServiceCount();
	if (gi1SEG_scan_FoundablePMTCount == 0 || gi1SEG_scan_FoundablePMTCount > 8) {
		ALOGE ("In %s, gi1SEG_scan_FoundablePMTCount = %d, Error\n", __func__, gi1SEG_scan_FoundablePMTCount);
		gi1SEG_scan_FoundablePMTCount = 0;
		return 0;
	}

	gi1SEG_scan_FoundPMTCount = 0;
	for(search_cnt = 0; search_cnt < MAX_1SEG_SERVICE_ID_CNT; search_cnt++)
	{
		pid = PID_PMT_1SEG + search_cnt;

		ucValue[0] = PMT_ID;
		ucMask[0] = 0xff;
		ucExclusion[0] = 0x00;

		tcc_manager_demux_filter_lock_on();
		if(tcc_demux_add_searchlist(iRequestID) == 0)
		{
			TCC_DxB_DEMUX_StartSectionFilter(hInterface, pid, iRequestID++, TRUE, 1, ucValue, ucMask, ucExclusion, TRUE, NULL);
		}
		tcc_manager_demux_filter_lock_off();
	}
	return 0;
}

/*
  * tcc_demux_load_1seg_pmts_all
  *  make filter for all 1seg PMTS.
  *  this routine is useful only when tcc.dxb.isdbt.skipnit was set. This property is only to test a special stream which has not a NIT.
  */
int tcc_demux_load_1seg_pmts_all (void)
{
	unsigned char      ucValue[16];
	unsigned char      ucMask[16];
	unsigned char      ucExclusion[16];
	UINT16	pid = 0;
	int	cnt;
	int iRequestID = DEMUX_REQUEST_1SEG_PMT;

	gi1SEG_scan_FoundablePMTCount = 8;
	gi1SEG_scan_FoundPMTCount = 0;
	for(cnt = 0; cnt < MAX_1SEG_SERVICE_ID_CNT; cnt++)
	{
		pid = PID_PMT_1SEG + cnt;
		ucValue[0] = PMT_ID;
		ucMask[0] = 0xff;
		ucExclusion[0] = 0x00;

		tcc_manager_demux_filter_lock_on();
		if(tcc_demux_add_searchlist(iRequestID) == 0)
			TCC_DxB_DEMUX_StartSectionFilter(hInterface, pid, iRequestID++, TRUE, 1, ucValue, ucMask, ucExclusion, TRUE, NULL);
		tcc_manager_demux_filter_lock_off();
	}
	return 0;
}

int tcc_demux_get_1seg_pmtpid(int service_id)
{
	UINT16	pid = 0;

	service_id = service_id & 0x00000007;

	pid = PID_PMT_1SEG + service_id;

	return pid;
}

int tcc_demux_load_pat(void)
{
	int ret = 0;
	int iRequestID = DEMUX_REQUEST_PAT;
	unsigned char ucValue[16],  ucMask[16], ucExclusion[16];

	MNG_DBG("In %s\n", __func__);

	ucValue[0] = PAT_ID;
	ucMask[0] = 0xff;
	ucExclusion[0] = 0x00;

	tcc_manager_demux_filter_lock_on();
	if(tcc_demux_add_searchlist(iRequestID) == 0)
	{
		TCC_DxB_DEMUX_StartSectionFilter(hInterface, PID_PAT, iRequestID, TRUE, 1, ucValue, ucMask, ucExclusion, TRUE, NULL);
	}
	tcc_manager_demux_filter_lock_off();
	return 0;
}

int tcc_demux_load_broadcast_pat(void)
{
	int ret = 0;
	int iRequestID = DEMUX_REQUEST_PAT;
	unsigned char ucValue[16],  ucMask[16], ucExclusion[16];

	MNG_DBG("In %s\n", __func__);

	ucValue[0] = PAT_ID;
	ucMask[0] = 0xff;
	ucExclusion[0] = 0x00;

	tcc_manager_demux_filter_lock_on();
	if(tcc_demux_add_searchlist(iRequestID) == 0)
	{
		TCC_DxB_DEMUX_StartSectionFilter(hInterface, PID_PAT, iRequestID, FALSE, 1, ucValue, ucMask, ucExclusion, TRUE, NULL);
	}
	tcc_manager_demux_filter_lock_off();
	return 0;
}

int tcc_demux_check_pat_filtering(void)
{
	int ret = 0;
	int iRequestID = DEMUX_REQUEST_PAT;

	tcc_manager_demux_filter_lock_on();
	ret = tcc_demux_check_searchlist (iRequestID);
	tcc_manager_demux_filter_lock_off();
	return ret;
}

int tcc_demux_load_sdt(void)
{
	int ret = 0;
	int iRequestID = DEMUX_REQUEST_SDT;
	unsigned char ucValue[16],  ucMask[16], ucExclusion[16];

	MNG_DBG("In %s\n", __func__);

	ucValue[0] = SDT_A_ID;
	ucMask[0] = 0xff;
	ucExclusion[0] = 0x00;

	tcc_manager_demux_filter_lock_on();
	if(tcc_demux_add_searchlist(iRequestID) == 0)
	{
		TCC_DxB_DEMUX_StartSectionFilter(hInterface, PID_SDT, iRequestID, TRUE, 1, ucValue, ucMask, ucExclusion, TRUE, NULL);
	}
	tcc_manager_demux_filter_lock_off();
	return 0;
}

int tcc_demux_check_sdt_filtering(void)
{
	int ret = 0;
	int iRequestID = DEMUX_REQUEST_SDT;

	tcc_manager_demux_filter_lock_on();
	ret = tcc_demux_check_searchlist (iRequestID);
	tcc_manager_demux_filter_lock_off();
	return ret;
}

int tcc_demux_load_nit(void)
{
	int ret = 0;
	int iRequestID = DEMUX_REQUEST_NIT;
	unsigned char ucValue[16],  ucMask[16], ucExclusion[16];

	MNG_DBG("In %s\n", __func__);

	ucValue[0] = NIT_A_ID;
	ucMask[0] = 0xff;
	ucExclusion[0] = 0x00;

	tcc_manager_demux_filter_lock_on();
	if(tcc_demux_add_searchlist(iRequestID) == 0)
	{
		TCC_DxB_DEMUX_StartSectionFilter(hInterface, PID_NIT, iRequestID, TRUE, 1, ucValue, ucMask, ucExclusion, TRUE, NULL);
	}
	tcc_manager_demux_filter_lock_off();
	return 0;
}

int tcc_demux_load_bit(void)
{
	int ret = 0;
	int iRequestID = DEMUX_REQUEST_BIT;
	unsigned char ucValue[16],  ucMask[16], ucExclusion[16];

	MNG_DBG("In %s\n", __func__);

	ucValue[0] = BIT_ID;
	ucMask[0] = 0xff;
	ucExclusion[0] = 0x00;

	tcc_manager_demux_filter_lock_on();
	if(tcc_demux_add_searchlist(iRequestID) == 0)
	{
		TCC_DxB_DEMUX_StartSectionFilter(hInterface, PID_BIT, iRequestID, TRUE, 1, ucValue, ucMask, ucExclusion, TRUE, NULL);
	}
	tcc_manager_demux_filter_lock_off();
	return 0;
}

int tcc_demux_check_bit_filtering(void)
{
	int iRequestID=DEMUX_REQUEST_BIT, ret;
	tcc_manager_demux_filter_lock_on();
	ret = tcc_demux_check_searchlist (iRequestID);
	tcc_manager_demux_filter_lock_off();
	return ret;
}

int tcc_demux_start_leit_filtering(void)
{
	unsigned char ucValue[16],  ucMask[16], ucExclusion[16];

	if(TCC_Isdbt_IsSupportProfileC() != 1)
		return -1;

		ucValue[0] = EIT_PF_A_ID;
		ucMask[0] = 0xff;
		ucExclusion[0] = 0x00;
		tcc_manager_demux_filter_lock_on();
		if(tcc_demux_add_searchlist(DEMUX_REQUEST_EIT) == 0)
		{
			TCC_DxB_DEMUX_StartSectionFilter(hInterface, PID_L_EIT /*PID_EIT*/, DEMUX_REQUEST_EIT, \
											FALSE, 1, ucValue, ucMask, ucExclusion, TRUE, NULL);
		}
		tcc_manager_demux_filter_lock_off();

	return 0;
	}

int tcc_demux_start_heit_filtering(void)
{
	unsigned char ucValue[16],  ucMask[16], ucExclusion[16];

	if(TCC_Isdbt_IsSupportProfileA() != 1)
		return -1;

	ucValue[0] = EIT_PF_A_ID;
	ucMask[0] = 0xff;
	ucExclusion[0] = 0x00;
	tcc_manager_demux_filter_lock_on();
	if(tcc_demux_add_searchlist(DEMUX_REQUEST_EIT+1) == 0)
	{
		TCC_DxB_DEMUX_StartSectionFilter(hInterface, PID_H_EIT /*PID_EIT*/, DEMUX_REQUEST_EIT+1, \
										FALSE, 1, ucValue, ucMask, ucExclusion, TRUE, NULL);
	}
	tcc_manager_demux_filter_lock_off();

	ucValue[0] = EIT_SA_0_ID;
	ucMask[0] = 0xf0;
	ucExclusion[0] = 0x00;
	tcc_manager_demux_filter_lock_on();
	if(tcc_demux_add_searchlist(DEMUX_REQUEST_EIT+2) == 0)
	{
		TCC_DxB_DEMUX_StartSectionFilter(hInterface, PID_H_EIT /*PID_EIT*/, DEMUX_REQUEST_EIT+2, \
										FALSE, 1, ucValue, ucMask, ucExclusion, TRUE, NULL);
	}
	tcc_manager_demux_filter_lock_off();

	return 0;
}

int tcc_demux_start_eit_filtering(void)
{
	int ret = 0;

	int fProfile = -1;

	MNG_DBG("In %s\n", __func__);

	tcc_demux_start_leit_filtering();

	tcc_demux_start_heit_filtering();

	return 0;
}

/**
 * @brief
 *
 * @param
 *
 * @return
 *     0 : Success
 *    -1 : StopSectionFilter error
 */
int tcc_demux_stop_eit_filtering(void)
{
	int err = 0;

	MNG_DBG("In %s\n", __func__);

	tcc_manager_demux_filter_lock_on();
	tcc_demux_delete_searchlist(DEMUX_REQUEST_EIT);
	err = TCC_DxB_DEMUX_StopSectionFilter (hInterface,DEMUX_REQUEST_EIT);
	if (err != 0 /*DxB_ERR_OK*/)
	{
		ALOGE("[%s] TCC_DxB_DEMUX_StopSectionFilter Error(%d)", __func__, err);
		err = -1;
	}

	if (TCC_Isdbt_IsSupportProfileA())
	{
		tcc_demux_delete_searchlist(DEMUX_REQUEST_EIT+1);
		err = TCC_DxB_DEMUX_StopSectionFilter (hInterface,DEMUX_REQUEST_EIT+1);
		if (err != 0 /*DxB_ERR_OK*/)
		{
			ALOGE("[%s] TCC_DxB_DEMUX_StopSectionFilter Error(%d)", __func__, err);
			err = -1;
		}

		tcc_demux_delete_searchlist(DEMUX_REQUEST_EIT+2);
		err = TCC_DxB_DEMUX_StopSectionFilter (hInterface,DEMUX_REQUEST_EIT+2);
		if (err != 0 /*DxB_ERR_OK*/)
		{
			ALOGE("[%s] TCC_DxB_DEMUX_StopSectionFilter Error(%d)", __func__, err);
			err = -1;
		}
	}
	tcc_manager_demux_filter_lock_off();

	return err;
}

int tcc_demux_start_nit_filtering(void)
{
	unsigned char ucValue[16],  ucMask[16], ucExclusion[16];

	MNG_DBG( "In %s\n", __func__);

	ucValue[0] = NIT_A_ID;
	ucMask[0] = 0xff;
	ucExclusion[0] = 0x00;

	tcc_manager_demux_filter_lock_on();
	if(tcc_demux_add_searchlist(DEMUX_REQUEST_NIT) == 0)
	{
		TCC_DxB_DEMUX_StartSectionFilter(hInterface, PID_NIT, DEMUX_REQUEST_NIT, \
										FALSE, 1, ucValue, ucMask, ucExclusion, TRUE, NULL);
	}
	tcc_manager_demux_filter_lock_off();

	tcc_dxb_timecheck_set("switch_channel", "NIT_set", TIMECHECK_STOP);
	tcc_dxb_timecheck_set("switch_channel", "NIT_get", TIMECHECK_START);

	return 0;
}

/**
 * @brief
 *
 * @param
 *
 * @return
 *     0 : Success
 *    -1 : StopSectionFilter error
 */
int tcc_demux_stop_nit_filtering(void)
{
	int err = 0;

	MNG_DBG( "In %s\n", __func__);

	tcc_manager_demux_filter_lock_on();
	err = TCC_DxB_DEMUX_StopSectionFilter(hInterface, DEMUX_REQUEST_NIT);
	if (err != 0 /*DxB_ERR_OK*/)
	{
		ALOGE("[%s] TCC_DxB_DEMUX_StopSectionFilter Error(%d)", __func__, err);
		err = -1;
	}
	tcc_demux_delete_searchlist(DEMUX_REQUEST_NIT);
	tcc_manager_demux_filter_lock_off();

	return err;
}

/**
 * @brief
 *
 * @param
 *
 * @return
 *     0 : Success
 *    -1 : StopSectionFilter error
 */
int tcc_demux_stop_sdt_filtering(void)
{
	int err = 0;

	MNG_DBG( "In %s\n", __func__);

	tcc_manager_demux_filter_lock_on();
	err = TCC_DxB_DEMUX_StopSectionFilter(hInterface, DEMUX_REQUEST_SDT);
	if (err != 0 /*DxB_ERR_OK*/)
	{
		ALOGE("[%s] TCC_DxB_DEMUX_StopSectionFilter Error(%d)", __func__, err);
		err = -1;
	}
	tcc_demux_delete_searchlist(DEMUX_REQUEST_SDT);
	tcc_manager_demux_filter_lock_off();

	return err;
}

/**
 * @brief
 *
 * @param
 *
 * @return
 *     0 : Success
 *    -1 : StopSectionFilter error
 */
int tcc_demux_stop_pat_filtering(void)
{
	int err = 0;

	MNG_DBG( "In %s\n", __func__);

	tcc_manager_demux_filter_lock_on();
	err = TCC_DxB_DEMUX_StopSectionFilter(hInterface, DEMUX_REQUEST_PAT);
	if (err != 0 /*DxB_ERR_OK*/)
	{
		ALOGE("[%s] TCC_DxB_DEMUX_StopSectionFilter Error(%d)", __func__, err);
		err = -1;
	}
	tcc_demux_delete_searchlist(DEMUX_REQUEST_PAT);
	tcc_manager_demux_filter_lock_off();

	return err;
}

int tcc_demux_start_pmt_filtering(int iPmtPID, int iProfile)
{
	unsigned char ucValue[16],  ucMask[16], ucExclusion[16];

	MNG_DBG( "In %s\n", __func__);

	if((iPmtPID <= 0) || (iPmtPID > 0x1FFF)) {
		ALOGE("[%s] Invalid PMT PID(0x%X)\n", __func__, iPmtPID);
		return -1;
	}

	ucValue[0] 		= PMT_ID;
	ucMask[0] 		= 0xff;
	ucExclusion[0] 	= 0x00;

	if(iProfile)
	{
		tcc_manager_demux_filter_lock_on();
		if(tcc_demux_add_searchlist(DEMUX_REQUEST_PMT) == 0)
		{
			TCC_DxB_DEMUX_StartSectionFilter(hInterface, iPmtPID, DEMUX_REQUEST_PMT, \
										FALSE, 1, ucValue, ucMask, ucExclusion, TRUE, NULL);
		}
		tcc_manager_demux_filter_lock_off();
	}
	else
	{
		tcc_manager_demux_filter_lock_on();
		if(tcc_demux_add_searchlist(DEMUX_REQUEST_PMT+1) == 0)
		{
			TCC_DxB_DEMUX_StartSectionFilter(hInterface, iPmtPID, DEMUX_REQUEST_PMT+1, \
										FALSE, 1, ucValue, ucMask, ucExclusion, TRUE, NULL);
		}
		tcc_manager_demux_filter_lock_off();
	}
	tcc_dxb_timecheck_set("switch_channel", "PMT_set", TIMECHECK_STOP);
	tcc_dxb_timecheck_set("switch_channel", "PMT_get", TIMECHECK_START);

	return 0;
}
int tcc_demux_check_pmt_filtering (int iProfile)
{
	int iRequestID, ret;
	if (iProfile) iRequestID = DEMUX_REQUEST_PMT;
	else iRequestID = DEMUX_REQUEST_PMT+1;
	tcc_manager_demux_filter_lock_on();
	ret = tcc_demux_check_searchlist(iRequestID);
	tcc_manager_demux_filter_lock_off();
	return ret;
}

/**
 * @brief
 *
 * @param
 *
 * @return
 *     0 : Success
 *    -1 : StopSectionFilter error
 */
int tcc_demux_stop_pmt_filtering(void)
{
	int err = 0;

	MNG_DBG( "In %s\n", __func__);

	tcc_manager_demux_filter_lock_on();
	err = TCC_DxB_DEMUX_StopSectionFilter(hInterface, DEMUX_REQUEST_PMT);
	if (err != 0 /*DxB_ERR_OK*/)
	{
		ALOGE("[%s] TCC_DxB_DEMUX_StopSectionFilter Error(%d)", __func__, err);
		err = -1;
	}

	tcc_demux_delete_searchlist(DEMUX_REQUEST_PMT);

	err = TCC_DxB_DEMUX_StopSectionFilter(hInterface, DEMUX_REQUEST_PMT+1);
	if (err != 0 /*DxB_ERR_OK*/)
	{
		ALOGE("[%s] TCC_DxB_DEMUX_StopSectionFilter Error(%d)", __func__, err);
		err = -1;
	}	
	tcc_demux_delete_searchlist(DEMUX_REQUEST_PMT+1);
	tcc_manager_demux_filter_lock_off();

	return err;
}

/**
 * @brief
 *
 * @param
 *
 * @return
 *     0 : Success
 *    -1 : StopSectionFilter error
 */
int tcc_demux_stop_pmt_backscan_filtering(void)
{
	int err = 0;
	int i;
	int pmt_count;

	MNG_DBG( "In %s\n", __func__);

	tcc_manager_demux_filter_lock_on();
	pmt_count = tcc_manager_demux_backscan_get_pmtcount();
	for (i = 0; i < pmt_count; ++i)
	{
		tcc_demux_delete_searchlist(DEMUX_REQUEST_FSEG_PMT_BACKSCAN+i);
		err = TCC_DxB_DEMUX_StopSectionFilter(hInterface, DEMUX_REQUEST_FSEG_PMT_BACKSCAN+i);
		if (err != 0 /*DxB_ERR_OK*/)
		{
			ALOGE("[%s] TCC_DxB_DEMUX_StopSectionFilter Error(%d)", __func__, err);
			err = -1;
		}	
	}
	tcc_manager_demux_filter_lock_off();
	tcc_manager_demux_backscan_reset_pmtcount();

	return err;
}

int tcc_demux_start_tot_filtering(void)
{
	int ret = 0;
	unsigned char ucValue[16],  ucMask[16], ucExclusion[16];

	MNG_DBG("In %s\n", __func__);

	ucValue[0] = TOT_ID;
	ucMask[0] = 0xff;
	ucExclusion[0] = 0x00;

	tcc_manager_demux_filter_lock_on();
	if(tcc_demux_add_searchlist(DEMUX_REQUEST_TIME) == 0)
	{
		TCC_DxB_DEMUX_StartSectionFilter(hInterface, PID_TDT_TOT, DEMUX_REQUEST_TIME, \
										FALSE, 1, ucValue, ucMask, ucExclusion, TRUE, NULL);
	}
	tcc_manager_demux_filter_lock_off();

	return 0;
}

/**
 * @brief
 *
 * @param
 *
 * @return
 *     0 : Success
 *    -1 : StopSectionFilter error
 */
int tcc_demux_stop_tot_filtering(void)
{
	int err = 0;

	MNG_DBG("In %s\n", __func__);

	tcc_manager_demux_filter_lock_on();
	tcc_demux_delete_searchlist(DEMUX_REQUEST_TIME);
	err = TCC_DxB_DEMUX_StopSectionFilter (hInterface,DEMUX_REQUEST_TIME);
	if (err != 0 /*DxB_ERR_OK*/)
	{
		ALOGE("[%s] TCC_DxB_DEMUX_StopSectionFilter Error(%d)", __func__, err);
		err = -1;
	}
	tcc_manager_demux_filter_lock_off();

	return err;
}

int tcc_demux_start_bit_filtering(void)
{
	int ret = 0;
	unsigned char ucValue[16],  ucMask[16], ucExclusion[16];

	MNG_DBG("In %s\n", __func__);

	ucValue[0] = BIT_ID;
	ucMask[0] = 0xff;
	ucExclusion[0] = 0x00;

	tcc_manager_demux_filter_lock_on();
	if(tcc_demux_add_searchlist(DEMUX_REQUEST_BIT) == 0)
	{
		TCC_DxB_DEMUX_StartSectionFilter(hInterface, PID_BIT, DEMUX_REQUEST_BIT, \
										FALSE, 1, ucValue, ucMask, ucExclusion, TRUE, NULL);
	}
	tcc_manager_demux_filter_lock_off();

	return 0;
}


/**
 * @brief
 *
 * @param
 *
 * @return
 *     0 : Success
 *    -1 : StopSectionFilter error
 */
int tcc_demux_stop_bit_filtering(void)
{
	int err = 0;

	MNG_DBG("In %s\n", __func__);

	tcc_manager_demux_filter_lock_on();
	tcc_demux_delete_searchlist(DEMUX_REQUEST_BIT);
	err = TCC_DxB_DEMUX_StopSectionFilter(hInterface,DEMUX_REQUEST_BIT);
	if (err != 0 /*DxB_ERR_OK*/)
	{
		ALOGE("[%s] TCC_DxB_DEMUX_StopSectionFilter Error(%d)", __func__, err);
		err = -1;
	}
	tcc_manager_demux_filter_lock_off();

	return err;
}

int tcc_demux_start_cdt_filtering (void)
{
	int	ret = 0;
	unsigned char ucValue[16],  ucMask[16], ucExclusion[16];

	MNG_DBG("In %s\n", __func__);

	ucValue[0] = CDT_ID;
	ucMask[0] = 0xff;
	ucExclusion[0] = 0x00;

	tcc_manager_demux_filter_lock_on();
	if(tcc_demux_add_searchlist (DEMUX_REQUEST_CDT) == 0)
	{
		ret = (int)TCC_DxB_DEMUX_StartSectionFilter(hInterface, PID_CDT, DEMUX_REQUEST_CDT, \
										FALSE, 1, ucValue, ucMask, ucExclusion, TRUE, NULL);
	}
	tcc_manager_demux_filter_lock_off();
	return ret;
}

/**
 * @brief
 *
 * @param
 *
 * @return
 *     0 : Success
 *    -1 : StopSectionFilter error
 */
int tcc_demux_stop_cdt_filtering(void)
{
	int err = 0;

	MNG_DBG("In %s\n", __func__);

	tcc_manager_demux_filter_lock_on();
	tcc_demux_delete_searchlist(DEMUX_REQUEST_CDT);
	err = TCC_DxB_DEMUX_StopSectionFilter(hInterface,DEMUX_REQUEST_CDT);
	if (err != 0 /*DxB_ERR_OK*/)
	{
		ALOGE("[%s] TCC_DxB_DEMUX_StopSectionFilter Error(%d)", __func__, err);
		err = -1;
	}
	tcc_manager_demux_filter_lock_off();

	return err;
}

int tcc_demux_start_cat_filtering(void)
{
	int ret = 0;
	int iRequestID = DEMUX_REQUEST_CAT;
	unsigned char ucValue[16],  ucMask[16], ucExclusion[16];

	//MNG_DBG("%s %d \n", __func__, __LINE__);

	ucValue[0] = CAT_ID;
	ucMask[0] = 0xff;
	ucExclusion[0] = 0x00;

	tcc_manager_demux_filter_lock_on();
	if(tcc_demux_add_searchlist(iRequestID) == 0)
	{
		TCC_DxB_DEMUX_StartSectionFilter(hInterface, PID_CAT, iRequestID, TRUE, 1, ucValue, ucMask, ucExclusion, TRUE, NULL);
	}
	tcc_manager_demux_filter_lock_off();
	return 0;
}

/**
 * @brief
 *
 * @param
 *
 * @return
 *     0 : Success
 *    -1 : StopSectionFilter error
 */
int tcc_demux_stop_cat_filtering(void)
{
	int err = 0;

	tcc_manager_demux_filter_lock_on();
	tcc_demux_delete_searchlist(DEMUX_REQUEST_CAT);
	err = TCC_DxB_DEMUX_StopSectionFilter(hInterface, DEMUX_REQUEST_CAT);
	if (err != 0 /*DxB_ERR_OK*/)
	{
		ALOGE("[%s] TCC_DxB_DEMUX_StopSectionFilter Error(%d)", __func__, err);
		err = -1;
	}
	tcc_manager_demux_filter_lock_off();

	return err;
}

int tcc_demux_start_emm_filtering(short emm_pid)
{
	int ret = 0;
	unsigned int 		pid, search_cnt=0;
	int iRequestID = DEMUX_REQUEST_EMM;
	unsigned char ucValue[16],  ucMask[16], ucExclusion[16];

	MNG_DBG("%s %d \n", __func__, __LINE__);

	if (emm_pid <= 2 || emm_pid >= 0x1FFF) {
		ALOGE("%s EMMPID(%d) is invalid\n", __func__, emm_pid);
		return -1;
	}

	if (gEMM_PID == emm_pid)
	{
		ALOGE("%s EMMPID(%d) is already set\n", __func__, emm_pid);
		return -1;
	}

	ucValue[0] = EMM_0_ID;
	ucMask[0] = 0xff;
	ucExclusion[0] = 0x00;

	tcc_manager_demux_filter_lock_on();
	if(tcc_demux_add_searchlist(iRequestID) == 0)
	{
		TCC_DxB_DEMUX_StartSectionFilter(hInterface, emm_pid, iRequestID, FALSE, 1, ucValue, ucMask, ucExclusion, TRUE, NULL);
	}

	if(TCC_Isdbt_IsSupportBCAS())
	{
		ucValue[0] = EMM_1_ID;
		ucMask[0] = 0xff;
		ucExclusion[0] = 0x00;
		iRequestID = iRequestID+1;

		if(tcc_demux_add_searchlist(iRequestID) == 0)
		{
			TCC_DxB_DEMUX_StartSectionFilter(hInterface, emm_pid, iRequestID, FALSE, 1, ucValue, ucMask, ucExclusion, TRUE, NULL);
		}

		TCC_Dxb_SC_Manager_EMM_ComMessageInit();
		TCC_Dxb_SC_Manager_EMM_IndiMessageInit();
	}

	tcc_manager_demux_filter_lock_off();

	gEMM_PID = emm_pid;

	return 0;
}

int tcc_demux_check_emm_filtering(void)
{
	int ret, iRequestID = DEMUX_REQUEST_EMM;
	tcc_manager_demux_filter_lock_on();
	ret = tcc_demux_check_searchlist(iRequestID);
	tcc_manager_demux_filter_lock_off();
	return ret;
}

/**
 * @brief
 *
 * @param
 *
 * @return
 *     0 : Success
 *    -1 : StopSectionFilter error
 */
int tcc_demux_stop_emm_filtering(void)
{
	int err = 0;

	tcc_manager_demux_filter_lock_on();

	tcc_demux_delete_searchlist(DEMUX_REQUEST_EMM);
	err = TCC_DxB_DEMUX_StopSectionFilter(hInterface, DEMUX_REQUEST_EMM);
	if (err != 0 /*DxB_ERR_OK*/)
	{
		ALOGE("[%s] TCC_DxB_DEMUX_StopSectionFilter Error(%d)", __func__, err);
		err = -1;
	}

	gEMM_PID = 0x1FFF;

	if(TCC_Isdbt_IsSupportBCAS())
	{
		tcc_demux_delete_searchlist(DEMUX_REQUEST_EMM+1);
		err = TCC_DxB_DEMUX_StopSectionFilter(hInterface, DEMUX_REQUEST_EMM+1);
		if (err != 0 /*DxB_ERR_OK*/)
		{
			ALOGE("[%s] TCC_DxB_DEMUX_StopSectionFilter Error(%d)", __func__, err);
			err = -1;
		}

		TCC_Dxb_SC_Manager_EMM_ComMessageDeInit();
		TCC_Dxb_SC_Manager_EMM_IndiMessageDeInit(0);
		TCC_Dxb_SC_Manager_EMM_ComMessageInit();
		TCC_Dxb_SC_Manager_EMM_IndiMessageInit();
	}

	tcc_manager_demux_filter_lock_off();

	return err;
}

int tcc_demux_start_ecm_filtering(short ecm_pid)
{
	int ret = 0;
	unsigned int 		pid, search_cnt=0;
	int iRequestID = DEMUX_REQUEST_ECM;
	unsigned char ucValue[16],  ucMask[16], ucExclusion[16];

	if (ecm_pid <= 2 || ecm_pid >= 0x1FFF) {
		ALOGE("%s ECMPID(%d) is invalid\n", __func__, ecm_pid);
		return -1;
	}

	if (gECM_PID == ecm_pid)
	{
		ALOGE("%s ECMPID(%d) is already set\n", __func__, ecm_pid);
		return -1;
	}

	ucValue[0] = ECM_0_ID;
	ucMask[0] = 0xff;
	ucExclusion[0] = 0x00;

	tcc_manager_demux_filter_lock_on();
	if(tcc_demux_add_searchlist(iRequestID) == 0)
	{
		TCC_DxB_DEMUX_StartSectionFilter(hInterface, ecm_pid, iRequestID, FALSE, 1, ucValue, ucMask, ucExclusion, TRUE, NULL);
	}
	tcc_manager_demux_filter_lock_off();

	gECM_PID = ecm_pid;

	ALOGE("%s ECMPID(%d) is set\n", __func__, ecm_pid);

	return 0;
}

void tcc_demux_stop_ecm_filtering(void)
{
	int	i;

	tcc_manager_demux_filter_lock_on();
	tcc_demux_delete_searchlist(DEMUX_REQUEST_ECM);
	TCC_DxB_DEMUX_StopSectionFilter(hInterface, DEMUX_REQUEST_ECM);
	tcc_manager_demux_filter_lock_off();
}

void tcc_demux_load_emm(void)
{
	T_DESC_CA	pDescCA;
	T_DESC_AC	pDescAC;
	short sSystemID = -1;
	short sTransmissionType = -1;
	short sEMM_PID = -1;
	int iRet;

	if(TCC_Isdbt_IsSupportBCAS() && ISDBT_Get_DescriptorInfo(E_DESC_CAT_CA, (void *)&pDescCA))
	{
		sSystemID = pDescCA.uiCASystemID;
		sEMM_PID = pDescCA.uiCA_PID;

		iRet = tcc_demux_start_emm_filtering(sEMM_PID);
		if(iRet == 0) {
			ALOGE("%s %d BCAS uiCASystemID = %d,  uiCA_EMM_PID =%d,  ucTableID = 0x%x, ucEMM_TransmissionFormat = 0x%x\n", \
					 __func__, __LINE__, sSystemID, sEMM_PID, pDescCA.ucTableID, pDescCA.ucEMM_TransmissionFormat );
		}
	}
	else if(TCC_Isdbt_IsSupportTRMP() && ISDBT_Get_DescriptorInfo(E_DESC_CAT_AC, (void *)&pDescAC))
	{
		sSystemID = pDescAC.uiCASystemID;
		sTransmissionType = pDescAC.uiTransmissionType;
		sEMM_PID = pDescAC.uiPID;

		if(sSystemID == ISDBT_CA_SYSTEM_ID_RMP && sTransmissionType != ISDBT_TRANSMISSION_TYPE_RMP) {
			ALOGE("%s %d Error Transmission Type\n", __func__, __LINE__);
			TCCDxBEvent_TRMPErrorUpdate(TCC_TRMP_EVENT_ERROR_TRANSMISSION_TYPE, gCurrentChannel);
			return;
		}

		iRet = tcc_demux_start_emm_filtering(sEMM_PID);
		if(iRet == 0) {
			ALOGE("%s %d TRMP sSystemID = %d,  sTransmissionType = %d, sEMM_PID =%d \n", __func__, __LINE__, sSystemID, sTransmissionType, sEMM_PID);
		}
	}
}

void tcc_demux_load_ecm(void)
{
	T_DESC_CA	pDescCA;
	T_DESC_AC	pDescAC;
	short sSystemID = -1;
	short sTransmissionType = -1;
	short sECM_PID = -1;
	int iRet;

	if(TCC_Isdbt_IsSupportBCAS() && ISDBT_Get_DescriptorInfo(E_DESC_PMT_CA, (void *)&pDescCA))
	{
		sSystemID = pDescCA.uiCASystemID;
		sECM_PID = pDescCA.uiCA_PID;

		iRet = tcc_demux_start_ecm_filtering(sECM_PID);
		if(iRet == 0) {
			ALOGE("%s %d  BCAS SystemID = %d, ECM_PID =%d \n", __func__, __LINE__, sSystemID, sECM_PID);
		}
	}
	else if(TCC_Isdbt_IsSupportTRMP() && ISDBT_Get_DescriptorInfo(E_DESC_PMT_AC, (void *)&pDescAC))
	{
		sSystemID = pDescAC.uiCASystemID;
		sTransmissionType = pDescAC.uiTransmissionType;
		sECM_PID = pDescAC.uiPID;

		if(sSystemID != ISDBT_CA_SYSTEM_ID_RMP) {
			TCCDxBEvent_TRMPErrorUpdate(TCC_TRMP_EVENT_ERROR_EC23, gCurrentChannel);
			return;
		}

		if(sTransmissionType != ISDBT_TRANSMISSION_TYPE_RMP) {
			ALOGE("%s %d Error Transmission Type\n", __func__, __LINE__);
			TCCDxBEvent_TRMPErrorUpdate(TCC_TRMP_EVENT_ERROR_TRANSMISSION_TYPE, gCurrentChannel);
			return;
		}

		iRet = tcc_demux_start_ecm_filtering(sECM_PID);
		if(iRet == 0) {
			ALOGE("%s %d TRMP sSystemID = %d,  sTransmissionType = %d, sECM_PID =%d \n", __func__, __LINE__, sSystemID, sTransmissionType, sECM_PID);
		}
	}
}

unsigned int tcc_demux_get_ecm_pid(void)
{
	return gECM_PID;
}

int tcc_manager_demux_dataservice_serviceid(unsigned int channel, unsigned int country_code, unsigned int *service_id)
{
	UpdateDB_Channel_GetDataServiceID(channel, country_code, service_id);
	return 0;
}

int tcc_manager_demux_dataservice_init(void)
{
	tcc_manager_demux_dataservice_lock_on();

	int i;
	for(i=0;i<MAX_DS_ELE;i++) {
		if(gstDataService.list[i].pucRawData != NULL)
			tcc_mw_free(__FUNCTION__, __LINE__, gstDataService.list[i].pucRawData);
		memset(&gstDataService.list[i], 0x0, sizeof(DEMUX_MGR_DS_ELE));
	}
	gstDataService.state = DS_STATE_NONE;

	tcc_manager_demux_dataservice_lock_off();
	return 0;
}

int tcc_manager_demux_dataservice_event(void)
{
	int i;
	for(i=0;i<MAX_DS_ELE;i++) {
		if(gstDataService.list[i].usServiceID == gPlayingServiceID) {
			TCCDxBEvent_SectionDataUpdate(gstDataService.list[i].usServiceID, gstDataService.list[i].iTableID, gstDataService.list[i].uiRawDataLen,	gstDataService.list[i].pucRawData,
				gstDataService.list[i].usNetworkID, gstDataService.list[i].usTransportStreamID, gstDataService.list[i].usDS_SID, gstDataService.list[i].ucAutoStartFlag);
			return 0;
		}
	}
	return 1;
}

int tcc_manager_demux_dataservice_start(void)
{
	tcc_manager_demux_dataservice_lock_on();

	gstDataService.state = DS_STATE_REQUEST;
	tcc_manager_demux_dataservice_event();

	tcc_manager_demux_dataservice_lock_off();
	return 0;
}

int tcc_manager_demux_dataservice_set(unsigned short usServiceID, int iTableID, unsigned short uiDSMCC_PID,
													unsigned short ucAutoStartFlag, unsigned short usNetworkID,
													unsigned short usTransportStreamID, unsigned int uiRawDataLen,
													unsigned char *pucRawData)
{
	tcc_manager_demux_dataservice_lock_on();

	int i = 0;
	int ds_serviceid = 0;

	if( pucRawData == NULL )
	{
		ALOGE("[%s] pucRawData is NULL Error !!!!!\n", __func__);
		tcc_manager_demux_dataservice_lock_off();
		return -1;
	}

	for(i=0;i<MAX_DS_ELE;i++) {
		if(gstDataService.list[i].usServiceID == usServiceID || gstDataService.list[i].usServiceID == 0) {
			gstDataService.list[i].usServiceID 			= usServiceID;
			gstDataService.list[i].iTableID				= iTableID;
			gstDataService.list[i].uiDSMCC_PID 			= uiDSMCC_PID;
			gstDataService.list[i].ucAutoStartFlag		= ucAutoStartFlag;
			gstDataService.list[i].usNetworkID 			= usNetworkID;
			gstDataService.list[i].usTransportStreamID	= usTransportStreamID;
			gstDataService.list[i].uiRawDataLen			= uiRawDataLen;
			if(gstDataService.list[i].pucRawData != NULL) {
				tcc_mw_free(__FUNCTION__, __LINE__, gstDataService.list[i].pucRawData);
			}
			gstDataService.list[i].pucRawData			= tcc_mw_malloc(__FUNCTION__, __LINE__, uiRawDataLen);
			if( gstDataService.list[i].pucRawData == NULL )
			{
				ALOGE("[%s] gstDataService.list[i].pucRawData is NULL Error !!!!!\n", __func__);
				tcc_manager_demux_dataservice_lock_off();
				return -1;
			}
			memcpy(gstDataService.list[i].pucRawData, pucRawData, uiRawDataLen);

			tcc_manager_demux_dataservice_serviceid(gCurrentChannel, gCurrentCountryCode, &ds_serviceid);
			gstDataService.list[i].usDS_SID = ds_serviceid;

			break;
		}
	}
	if(gstDataService.state == DS_STATE_REQUEST) {
		if (usServiceID == gPlayingServiceID)
			tcc_manager_demux_dataservice_event();
	}

	tcc_manager_demux_dataservice_lock_off();
	return 0;
}

int tcc_manager_demux_custom_filter_start(int iPID, int iTableID)
{
	int ret = 0;
	int iRequestID = DEMUX_REQUEST_CUSTOM + tcc_demux_customfilterlist_getIndex(iPID);
	unsigned char ucValue[16],  ucMask[16], ucExclusion[16];

	ucValue[0] = iTableID;
	ucMask[0] = 0xff;
	ucExclusion[0] = 0x00;

	tcc_manager_demux_filter_lock_on();
	if(tcc_demux_customfilterlist_add(iPID, iTableID, iRequestID)) {
		ret = TCC_DxB_DEMUX_StartSectionFilter(hInterface, iPID, iRequestID, FALSE, 1, ucValue, ucMask, ucExclusion, TRUE, NULL);
	}
	tcc_manager_demux_filter_lock_off();
	return ret;
}

int tcc_manager_demux_custom_filter_stop(int iPID, int iTableID)
{
	int ret = 0;
	int iRequestID = tcc_demux_customfilterlist_getRequestID(iPID, iTableID);
	if(iRequestID == 0) {
		ALOGE("%s failed(0x%X, 0x%X)", __func__, iPID, iTableID);
		return -1;
	}

	tcc_manager_demux_filter_lock_on();
	ret = TCC_DxB_DEMUX_StopSectionFilter(hInterface, iRequestID);
	tcc_demux_customfilterlist_delete(iPID, iTableID);
	tcc_manager_demux_filter_lock_off();

	return ret;
}

int tcc_manager_demux_cas_init(void)
{
	int 	err =0;
	int	service_id=0;
	int ret;

	gECM_PID = 0x1FFF;
	gEMM_PID = 0x1FFF;

	ISDBT_Clear_NetworkIDInfo();

	ret = tcc_manager_cas_reset();
	if(ret == ISDBT_CAS_SUCCESS)
	{
		ALOGD("%s Success", __func__);
	}

	return 0;
}

int tcc_demux_section_decoder_update_pmt_in_scan(int req_id)
{
	int Service_cnt = 0;

	if(TCC_Isdbt_IsSupportProfileC())
	{
		if((req_id - DEMUX_REQUEST_1SEG_PMT)<0  || (req_id - DEMUX_REQUEST_1SEG_PMT)>MAX_1SEG_SERVICE_ID_CNT) {
			return 0;
		}

		if(gi1SEG_scan_FoundablePMTCount > 0) {
			gi1SEG_scan_FoundablePMTCount--;
			gi1SEG_scan_FoundPMTCount++;
		}

		ALOGE("%s %d PMT_PID = %x\n  ServiceID = %x ", __func__, __LINE__, (((req_id) - DEMUX_REQUEST_1SEG_PMT) + PID_PMT_1SEG), gCurrentServiceID);

		UpdateDB_PMTPID_ChannelTable(gCurrentServiceID, ((req_id) - DEMUX_REQUEST_1SEG_PMT) + PID_PMT_1SEG);
	}

	return 0;
}

int tcc_demux_section_decoder_update_pmt_in_broadcast(void)
{
	int iTotalAudioPID=0, iTotalVideoPID=0, iStPID=0xFFFF, iSiPID=0xFFFF;
	int piAudioPID[4]={0x1FFF, 0x1FFF, 0x1FFF, 0x1FFF};
	int piAudioStreamType[4]={0, 0, 0, 0};
	int piVideoPID[4]={0x1FFF, 0x1FFF, 0x1FFF, 0x1FFF};
	int piVideoStreamType[4]={0, 0, 0, 0};

	TCCDxBProc_CheckPCRChg();

	if(ISDBT_Get_PIDInfo(&iTotalAudioPID, piAudioPID, piAudioStreamType, &iTotalVideoPID, piVideoPID, piVideoStreamType,  &iStPID, &iSiPID))
	{
		if(TCCDxBProc_Update((unsigned int)iTotalAudioPID, (unsigned int *)piAudioPID, (unsigned int *)piAudioStreamType, \
							(unsigned int)iTotalVideoPID, (unsigned int *)piVideoPID, (unsigned int *)piVideoStreamType, (unsigned int)iStPID, (unsigned int)iSiPID))
		{
			MNG_DBG("In %s PMT updated!!!\n", __func__);
			//TCCDxBEvent_ChannelUpdate(NULL);
		}
	}

	return 0;
}

int tcc_demux_section_decoder_ews_update(int ews_proc)
{
	int	service_id, area_code[256], area_code_length;
	int	signal_type, start_end_flag;
	int	arg[6];
	int	status;

	switch (ews_proc)
	{
		case 1:		//start
			status = isdbt_emergency_info_get_data(&service_id, area_code, &signal_type, &start_end_flag, &area_code_length);
			if (status == 1)	//found service_id and area_code from emergency_information_descriptor
			{
				arg[0] = 1;	// 1 means start.
				arg[1] = service_id;
				arg[2] = start_end_flag;
				arg[3] = signal_type;
				arg[4] = area_code_length;
				arg[5] = (int)&area_code[0];
				TCCDxBEvent_Emergency_Info_Update((void*)arg);
			}

			break;
		case 0:		//stop
			arg[0] = 0;
			arg[1] = arg[2] = arg[3] = arg[4] = arg[5] = 0;
			TCCDxBEvent_Emergency_Info_Update ((void*)arg);
			status = 1;
			break;
		default:
			status = 0;
			break;
	}
	return status;
}

int tcc_demux_section_decoder_eit_update(int service_id, int table_id)
{
	TCCDxBEvent_EPGUpdate(service_id, table_id);
	return 0;
}

void tcc_manager_demux_set_eit_subtitleLangInfo_clear(void)
{
	//ALOGE("[%s]\n", __func__);
	memset(&gEITCaptionInfo, 0, sizeof(ISDBT_EIT_CAPTION_INFO));
	gEITSubtitleLangInfo = 0;
}

int tcc_manager_demux_set_eit_subtitleLangInfo(unsigned int NumofLang, unsigned int firstLangcode, unsigned int secondLangcode, unsigned short ServiceID, int cnt)
{
	//ALOGE("[%s]\n", __func__);
	gEITCaptionInfo[cnt].ServiceID = ServiceID;
	gEITCaptionInfo[cnt].NumofLang = NumofLang;
	gEITCaptionInfo[cnt].SubtLangCode[0] = firstLangcode;
	gEITCaptionInfo[cnt].SubtLangCode[1] = secondLangcode;
	gEITSubtitleLangInfo = 1;

	return 0;
}

int tcc_manager_demux_get_eit_subtitleLangInfo(int *LangNum, unsigned int *SubtitleLang1, unsigned int *SubtitleLang2)
{
//	ALOGE("[%s]\n", __func__);
	int i=0;

	if(gEITSubtitleLangInfo == 0){
		//not set the subtitlelanginfo from eit
		return -1;
	}

	for(i=0; i< MAX_SUPPORT;i++){
		if(gEITCaptionInfo[i].ServiceID == gPlayingServiceID){
			*LangNum = gEITCaptionInfo[i].NumofLang;
			*SubtitleLang1 = gEITCaptionInfo[i].SubtLangCode[0];
			*SubtitleLang2 = gEITCaptionInfo[i].SubtLangCode[1];
			break;
		}

		if(i==(MAX_SUPPORT-1)){
			//ALOGE("[subtitle][%s] eit get fail\n", __func__);
			return -1;
		}
	}

	return 0;
}

void tcc_manager_demux_need_more_scan(void)
{
	tcc_manager_demux_filter_lock_on();
	if( tcc_demux_get_valid_searchlist() == 0 ){
		tcc_dxb_sem_up(gpDemuxScanWait);
	}
	tcc_manager_demux_filter_lock_off();
}

int tcc_manager_demux_need_more_informal_scan(void)
{
	int rtn = 1;

	tcc_manager_demux_filter_lock_on();
	if( tcc_demux_get_valid_informal_searchlist() == 0 ){
		tcc_dxb_sem_up(gpDemuxScanWait);
		rtn = 0;
	}
	tcc_manager_demux_filter_lock_off();

	return rtn;
}

void tcc_manager_demux_epgscan_done(void)
{
	tcc_dxb_sem_up(gpDemuxScanWait);
}

int tcc_demux_section_decoder_in_scan(int table_id, int req_id)
{
	int 	loop, iRequestID;
	E_SCAN_STATE eScanState;

	switch(table_id)
	{
		case PAT_ID:
			MNG_DBG("[TCCDEMUX] Found PAT\n");
			tcc_manager_demux_filter_lock_on();
			TCC_DxB_DEMUX_StopSectionFilter (hInterface,req_id);
			tcc_demux_delete_searchlist(req_id);
			tcc_manager_demux_filter_lock_off();

			tcc_demux_load_pmts();
			tcc_manager_demux_scan_set_pat(1);

			tcc_manager_demux_need_more_scan();
			break;

		case PMT_ID:
		{
			MNG_DBG("[TCCDEMUX] Found PMT (%d)\n", req_id );
			tcc_demux_section_decoder_update_pmt_in_scan(req_id);

			tcc_manager_demux_filter_lock_on();

			TCC_DxB_DEMUX_StopSectionFilter (hInterface,req_id);

			tcc_demux_delete_searchlist(req_id);

			tcc_manager_demux_filter_lock_off();

			// Noah / 2016081X / Only Fullseg PMTs
			if( !(DEMUX_REQUEST_1SEG_PMT <= req_id &&  req_id <= DEMUX_REQUEST_1SEG_PMT + MAX_1SEG_SERVICE_ID_CNT) )
			{
				// Whenever one of Fullseg PMTs comes, PMTs timeout is reset.
				tcc_manager_demux_scan_init_only_pmts();
			}

			if(TCC_Isdbt_IsSupportProfileC() && (gi1SEG_scan_FoundablePMTCount == 0)){
				iRequestID = DEMUX_REQUEST_1SEG_PMT;
				for (loop=0; loop < MAX_1SEG_SERVICE_ID_CNT; loop++){
					tcc_manager_demux_filter_lock_on();

					TCC_DxB_DEMUX_StopSectionFilter (hInterface,iRequestID);

					tcc_demux_delete_searchlist(iRequestID);

					tcc_manager_demux_filter_lock_off();

					iRequestID++;
				}
			}
			tcc_manager_demux_need_more_scan();
			break;
		}
		case SDT_A_ID:
		{
			MNG_DBG("[TCCDEMUX] Found SDT\n");

			tcc_manager_demux_filter_lock_on();
			TCC_DxB_DEMUX_StopSectionFilter (hInterface,req_id);
			tcc_demux_delete_searchlist(req_id);
			tcc_manager_demux_filter_lock_off();

			tcc_manager_demux_need_more_scan();
			break;
		}

		case NIT_A_ID:
		{
			MNG_DBG("[TCCDEMUX] Found NIT\n");

			tcc_manager_demux_filter_lock_on();
			TCC_DxB_DEMUX_StopSectionFilter (hInterface,req_id);
			tcc_demux_delete_searchlist(req_id);
			tcc_manager_demux_filter_lock_off();

			if(TCC_Isdbt_IsSupportProfileA()){	//if support Fullseg
				tcc_demux_load_pat();
			}
			if(TCC_Isdbt_IsSupportProfileC()){	//if support 1seg ?
				if (TCC_Isdbt_IsSupportJapan())
					tcc_demux_load_1seg_pmts_japan();
				else
					tcc_demux_load_1seg_pmts();
			}

			tcc_manager_demux_scan_set_nit(1);

			tcc_manager_demux_need_more_scan();
			break;
		}

		case BIT_ID:
			MNG_DBG("[TCCDEMUX] Found BIT\n");

			tcc_manager_demux_filter_lock_on();
			TCC_DxB_DEMUX_StopSectionFilter (hInterface,req_id);
			tcc_demux_delete_searchlist(req_id);
			tcc_manager_demux_filter_lock_off();

			tcc_manager_demux_need_more_scan();
			break;

		default:
			break;
	}
	return 0;
}

int tcc_demux_section_decoder_in_informal_scan(int table_id, int req_id)
{
	int	loop, iRequestID;
	E_SCAN_STATE eScanState;
	int section_id=0;

	switch(table_id)
	{
		case PAT_ID:
			MNG_DBG("[TCCDEMUX_INFORMAL] Found PAT\n");
			tcc_manager_demux_filter_lock_on();
			tcc_demux_delete_informal_searchlist(req_id);
			tcc_manager_demux_filter_lock_off();

			if (tcc_demux_check_pmts_backscan()!=0) {
				tcc_demux_load_pmts_backscan(); //pmts filer is installed for both case of searching fullseg and checking update of es pid
			}

			tcc_manager_demux_scan_set_pat(1);

			section_id = NOTIFY_SECTION_PAT;

			if (tcc_manager_demux_backscan_isbeing_scanfullseg())
				tcc_manager_demux_backscan_update_sectioninfo(0,DEMUX_BACKSCAN_PAT);
			break;

		case PMT_ID:
			MNG_DBG("[TCCDEMUX_INFORMAL] Found PMT (%d)\n", req_id );
			tcc_demux_section_decoder_update_pmt_in_scan(req_id);

			tcc_manager_demux_filter_lock_on();
			tcc_demux_delete_informal_searchlist(req_id);
			tcc_manager_demux_filter_lock_off();

			if(TCC_Isdbt_IsSupportProfileC() && (gi1SEG_scan_FoundablePMTCount == 0)){
				iRequestID = DEMUX_REQUEST_1SEG_PMT;
				for (loop=0; loop < MAX_1SEG_SERVICE_ID_CNT; loop++){
					tcc_manager_demux_filter_lock_on();
					TCC_DxB_DEMUX_StopSectionFilter (hInterface,iRequestID);
					tcc_demux_delete_searchlist(iRequestID);
					tcc_manager_demux_filter_lock_off();
					iRequestID++;
				}
			}

			if (req_id == DEMUX_REQUEST_PMT) {
				section_id = NOTIFY_SECTION_PMT_MAIN;
			}
			else if (req_id == (DEMUX_REQUEST_PMT+1)) {
				section_id = NOTIFY_SECTION_PMT_SUB;
			}

			if(TCC_Isdbt_IsSupportCAS()){
				tcc_demux_load_ecm();
			}

			if (tcc_manager_demux_backscan_isbeing_scanfullseg())
				tcc_manager_demux_backscan_update_sectioninfo (0, DEMUX_BACKSCAN_PMT);
			break;

		case NIT_A_ID:
			MNG_DBG("[TCCDEMUX_INFORMAL] Found NIT\n");

			tcc_manager_demux_filter_lock_on();
			tcc_demux_delete_informal_searchlist(req_id);
			tcc_manager_demux_filter_lock_off();

			if (TCC_Isdbt_IsSupportProfileA()){	//if support Fullseg
				tcc_demux_load_broadcast_pat();//tcc_demux_load_pat();
			}

			if (TCC_Isdbt_IsSupportProfileC()) {	//if support 1seg ?
				if (TCC_Isdbt_IsSupportJapan()) {
					tcc_demux_load_1seg_pmts_japan();
				} else {
					tcc_demux_load_1seg_pmts();
				}
			}

			tcc_manager_demux_scan_set_nit(1);

			section_id = NOTIFY_SECTION_NIT;

			if (tcc_manager_demux_backscan_isbeing_scanfullseg())
				tcc_manager_demux_backscan_update_sectioninfo (0, DEMUX_BACKSCAN_NIT);
			break;

		case BIT_ID:
			MNG_DBG("[TCCDEMUX_INFORMAL] Found BIT\n");

			// Noah / 20170706 / IM478A-30 / In case of the searchAndSetChannel, NIT & BIT & PMT_1 have infinite timer.
			g_isAddBit_ForSearchAndSetChannel = 0;

			tcc_manager_demux_filter_lock_on();
			tcc_demux_delete_informal_searchlist(req_id);
			tcc_manager_demux_filter_lock_off();

			if (tcc_manager_demux_backscan_isbeing_scanfullseg())
				tcc_manager_demux_backscan_update_sectioninfo(0, DEMUX_BACKSCAN_BIT);
			break;

		default:
			break;
	}

	if (section_id > 0) {
		if (section_notification_info.response_ids & section_id) {
			section_notification_info.response_ids &= (~section_id);
			TCCDxBEvent_SectionUpdate(gCurrentChannel, section_id);
		}
	}

	if(!tcc_manager_demux_need_more_informal_scan()) {
		tcc_manager_demux_backscan_stop_scanfullseg();
	}

	return 0;
}

int tcc_demux_section_decoder_in_broadcast(int table_id, int req_id)
{
	int	status, request_ch_update=1;
	int section_id=0;
	int i;
	unsigned int proc_state = TCCDxBProc_GetMainState();

	if(proc_state == TCCDXBPROC_STATE_AIRPLAY) {
		switch(table_id)
		{
			case PAT_ID:
				ALOGD("[TCCDEMUX_PLAY] Found PAT(%d) \n", req_id);

				if (tcc_demux_check_pmts_backscan()!=0)
					tcc_demux_load_pmts_backscan(); //pmts filer is installed for both case of searching fullseg and checking update of es pid
				if (tcc_manager_demux_backscan_isbeing_scanfullseg())
					tcc_manager_demux_backscan_update_sectioninfo(0,DEMUX_BACKSCAN_PAT);

				section_id = NOTIFY_SECTION_PAT;
				break;

			case CAT_ID:
				ALOGD("[TCCDEMUX_PLAY] Found CAT(%d) \n", req_id);

				if(TCC_Isdbt_IsSupportCAS()){
					if (tcc_demux_check_emm_filtering()!=0)
					tcc_demux_load_emm();
				}
				break;

			case PMT_ID:
				if ((req_id >= DEMUX_REQUEST_FSEG_PMT_BACKSCAN) && (req_id < DEMUX_REQUEST_FSEG_PMT_BACKSCAN+1000)) {
					if (tcc_manager_demux_backscan_isbeing_scanfullseg())
						tcc_manager_demux_backscan_update_sectioninfo (0, DEMUX_BACKSCAN_PMT);
					tcc_demux_section_decoder_update_pmt_in_broadcast();
				} else {
					if (tcc_manager_demux_ews_getstate() == EWS_START) {
						//if EWS signal was found, but processing EWS descritpor is not yet
						status = tcc_demux_section_decoder_ews_update(1);
						if (status){
							tcc_manager_demux_ews_setstate(EWS_ONGOING);
						}
					}

					tcc_demux_section_decoder_update_pmt_in_broadcast();

					if(TCC_Isdbt_IsSupportBrazil()){
						ISDBT_AccessCtrl_ProcessParentalRating();
					}

					if (req_id == DEMUX_REQUEST_PMT) {
						section_id = NOTIFY_SECTION_PMT_MAIN;
					}
					else if (req_id == (DEMUX_REQUEST_PMT+1)) {
						section_id = NOTIFY_SECTION_PMT_SUB;
					}
				}

				if(TCC_Isdbt_IsSupportCAS()){
					tcc_demux_load_ecm();
				}
				break;

			case SDT_A_ID:
				ALOGD("[TCCDEMUX_PLAY] Found SDT(%d) \n", req_id);

				if (tcc_manager_demux_backscan_isbeing_scanfullseg())
					tcc_manager_demux_backscan_update_sectioninfo (0, DEMUX_BACKSCAN_SDT);

				section_id = NOTIFY_SECTION_SDT;
				tcc_manager_demux_logo_update_infoDB();

#if 0    // Original
				if (tcc_manager_demux_is_skip_sdt_in_scan())
					tcc_manager_demux_channelname_update_infoDB();
#else    // Noah / 20180131 / IM478A-77 (relay station search in the background using 2TS) / Update 'channelName' field of DB forcibly
				tcc_manager_demux_channelname_update_infoDB();
#endif

				break;

			case NIT_A_ID:
				ALOGD("[TCCDEMUX_PLAY] Found NIT(%d) \n", req_id);

				if (TCC_Isdbt_IsSupportProfileA())	//if support Fullseg
					if (tcc_demux_check_pat_filtering()!=0) tcc_demux_load_broadcast_pat();//tcc_demux_load_pat();

				if (tcc_demux_check_sdt_filtering()!=0) tcc_demux_load_sdt();

				if (tcc_demux_check_pmt_filtering(1) != 0) {
				    if (gCurrentPMTPID > 2 && gCurrentPMTPID < 0x1FFF) {
				        tcc_demux_start_pmt_filtering(gCurrentPMTPID, 1);
				    }
				}

				if (tcc_demux_check_pmt_filtering(0) != 0) {
				    if (gCurrentPMTSubPID > 2 && gCurrentPMTSubPID < 0x1FFF) {
					    tcc_demux_start_pmt_filtering(gCurrentPMTSubPID, 0);
				    }
				}

				if (tcc_demux_check_bit_filtering()!=0) tcc_demux_load_bit();

				if (tcc_manager_demux_backscan_isbeing_scanfullseg())
					tcc_manager_demux_backscan_update_sectioninfo (0, DEMUX_BACKSCAN_NIT);

				section_id = NOTIFY_SECTION_NIT;
				break;

			case EIT_PF_A_ID:
				if(TCC_Isdbt_IsSupportBrazil()){
					ISDBT_AccessCtrl_ProcessParentalRating();
				}
				break;

			case BIT_ID:
				ALOGD("[TCCDEMUX_PLAY] Found BIT(%d) \n", req_id);

				tcc_manager_demux_filter_lock_on();
				TCC_DxB_DEMUX_StopSectionFilter(hInterface,req_id);
				tcc_demux_delete_searchlist(req_id);
				tcc_manager_demux_filter_lock_off();

				if (tcc_manager_demux_backscan_isbeing_scanfullseg())
					tcc_manager_demux_backscan_update_sectioninfo(0, DEMUX_BACKSCAN_BIT);
				break;

			default:
				break;
		}
	}
	else if(proc_state == TCCDXBPROC_STATE_EWSMODE) {
		switch(table_id) {
			case PAT_ID:
				if (tcc_demux_check_pmts_backscan()!=0) {
					tcc_demux_load_pmts_backscan(); //pmts filer is installed for both case of searching fullseg and checking update of es pid
				}
				if (tcc_manager_demux_backscan_isbeing_scanfullseg()) {
					tcc_manager_demux_backscan_update_sectioninfo(0,DEMUX_BACKSCAN_PAT);
				}
				break;
			case PMT_ID:
				if ((req_id >= DEMUX_REQUEST_FSEG_PMT_BACKSCAN) && (req_id < DEMUX_REQUEST_FSEG_PMT_BACKSCAN+1000)) {
					if (tcc_manager_demux_backscan_isbeing_scanfullseg())
						tcc_manager_demux_backscan_update_sectioninfo (0, DEMUX_BACKSCAN_PMT);
					tcc_demux_section_decoder_update_pmt_in_broadcast();
				} else {
					if (tcc_manager_demux_ews_getstate() == EWS_START) {
						//if EWS signal was found, but processing EWS descritpor is not yet
						status = tcc_demux_section_decoder_ews_update(1);
						if (status){
							tcc_manager_demux_ews_setstate(EWS_ONGOING);
						}
					}

					tcc_demux_section_decoder_update_pmt_in_broadcast();
				}
				break;
			case NIT_A_ID:
				if (TCC_Isdbt_IsSupportProfileA()) {
					if (tcc_demux_check_pat_filtering()!=0) {
						tcc_demux_load_broadcast_pat();
					}
				}
				if (tcc_demux_check_pmt_filtering(1) != 0) {
				    if (gCurrentPMTPID > 2 && gCurrentPMTPID < 0x1FFF) {
				        tcc_demux_start_pmt_filtering(gCurrentPMTPID, 1);
				    }
				}
				if (tcc_demux_check_pmt_filtering(0) != 0) {
				    if (gCurrentPMTSubPID > 2 && gCurrentPMTSubPID < 0x1FFF) {
					    tcc_demux_start_pmt_filtering(gCurrentPMTSubPID, 0);
				    }
				}
				if (tcc_manager_demux_backscan_isbeing_scanfullseg()) {
					tcc_manager_demux_backscan_update_sectioninfo (0, DEMUX_BACKSCAN_NIT);
				}
				break;
			default:
				break;
		}
	}

	if (section_id > 0) {
		if (section_notification_info.response_ids & section_id) {
			section_notification_info.response_ids &= (~section_id);
			TCCDxBEvent_SectionUpdate(gCurrentChannel, section_id);
		}
	}

	if (table_id == PAT_ID || table_id == PMT_ID || table_id == SDT_A_ID || table_id == NIT_A_ID || table_id == BIT_ID)
	{
		if (tcc_manager_demux_backscan_isbeing_scanfullseg() && tcc_manager_demux_backscan_get_pmtcount() > 0 && tcc_manager_demux_backscan_get_sectioninfo() == 0)
		{
			int error = 0;
			int tuner_strength_index;
			int pmt_count;

			pmt_count = tcc_manager_demux_backscan_get_pmtcount();
			for (i = 0; i < pmt_count; ++i)
			{
				error |= UpdateDB_CheckDoneUpdateInfo(gCurrentChannel, gCurrentCountryCode, tcc_manager_demux_backscan_get_serviceid(i));
				if (error)	break;
			}

			if (error == 0)
			{
				ALOGD("[TCCDEMUX_PLAY] !!!!! Done PLAY BACK SCAN !!!!! \n");

				if(fnGetBer)
					fnGetBer(&tuner_strength_index);
				else
					tuner_strength_index = 0;

				if (TCC_Isdbt_IsSupportSpecialService()) {
					UpdateDB_ArrangeSpecialService();
				}

				if (!TCC_Isdbt_IsSupportDataService()) {
					UpdateDB_ArrangeDataService();
				}

				for (i = 0; i < pmt_count; ++i)
				{
					UpdateDB_Strength_ChannelTable (tuner_strength_index, (int)tcc_manager_demux_backscan_get_serviceid(i));
					UpdateDB_Channel_SQL(gCurrentChannel, gCurrentCountryCode, (int)tcc_manager_demux_backscan_get_serviceid(i), 1);
				}
				tcc_manager_demux_backscan_stop_scanfullseg();
				TCCDxBEvent_ChannelUpdate(&request_ch_update);
			}
		}
	}

	return 0;
}

int tcc_demux_section_decoder_in_handover(int table_id, int req_id)
{
	int	status;

	switch(table_id)
	{
		case NIT_A_ID:
			tcc_manager_demux_filter_lock_on();
			TCC_DxB_DEMUX_StopSectionFilter (hInterface,req_id);
			tcc_demux_delete_searchlist(req_id);
			tcc_manager_demux_filter_lock_off();

			if (tcc_demux_check_bit_filtering()!=0) tcc_demux_load_bit();

			if( tcc_demux_get_valid_searchlist() == 0 ){
				tcc_dxb_sem_up(gpDemuxScanWait);
			}
			break;

		case BIT_ID:
			tcc_manager_demux_filter_lock_on();
			TCC_DxB_DEMUX_StopSectionFilter (hInterface,req_id);
			tcc_demux_delete_searchlist(req_id);
			tcc_manager_demux_filter_lock_off();

			if( tcc_demux_get_valid_searchlist() == 0 ){
				tcc_dxb_sem_up(gpDemuxScanWait);
			}
			break;

		default:
			break;
	}
	return 0;
}

int tcc_demux_section_decoder_in_idle(int table_id, int req_id)
{
	switch(table_id)
	{
		case PAT_ID:
		case CAT_ID:
		case PMT_ID:
		case SDT_A_ID:
		case NIT_A_ID:
		case EIT_PF_A_ID:
		case BIT_ID:
			/* no code intentionally */
		default:
			tcc_manager_demux_filter_lock_on();
			TCC_DxB_DEMUX_StopSectionFilter (hInterface,req_id);
			tcc_demux_delete_searchlist(req_id);
			tcc_manager_demux_filter_lock_off();
			break;
	}
	return 0;
}

void* tcc_demux_section_decoder(void *arg)
{
	int i;
	int queue_num = 0, max_queue_num = 0, result = 0;
	void 	*pSectionBitPars = NULL;
	unsigned int queue_state_change = 0;
	DEMUX_DEC_COMMAND_TYPE *pSectionCmd = NULL;
	E_EWS_STATE	ews_state = EWS_NONE;
	E_DEMUX_STATE state = E_DEMUX_STATE_MAX;

	struct timespec tspec;
	long long llCurrTime;
	long long llEwsChkT, llSpecialServiceChkT;
	int ews_wait_time=0;

	MNG_DBG("In %s\n", __func__);

	OpenDB_Table();
	OpenDB_EPG_Table();

	BITPARS_OpenBitParser( &pSectionBitPars );

	clock_gettime(CLOCK_MONOTONIC , &tspec);
	llCurrTime = (long long)tspec.tv_sec * 1000000 + (long long)tspec.tv_nsec / 1000;
	llEwsChkT = llCurrTime;
	llSpecialServiceChkT = llCurrTime;

	tcc_manager_demux_ews_reset();

	while(giSectionThreadRunning)
	{
		queue_num = tcc_dxb_getquenelem(SectionDecoderQueue);
		max_queue_num = tcc_dxb_get_maxqueuelem(SectionDecoderQueue);

		if(queue_num == max_queue_num)
		{
			ALOGI("[%s] Q full(%d/%d)\n", __func__, queue_num, max_queue_num);
			queue_state_change = 1;
		}

		if(queue_state_change && (queue_num != max_queue_num))
		{
			ALOGI("[%s] Q consumed(%d->%d)\n", __func__, max_queue_num, queue_num);
			queue_state_change = 0;
		}

		if (tcc_manager_demux_ews_getrequest())
		{
			/* it's to reset ews information before finishing 'channle play' */
			tcc_manager_demux_ews_reset();
			isdbt_emergency_info_clear();
		}

		state = tcc_manager_demux_get_state();
		if (state == E_DEMUX_STATE_AIRPLAY)
		{
			clock_gettime(CLOCK_MONOTONIC , &tspec);
			llCurrTime = (long long)tspec.tv_sec * 1000000 + (long long)tspec.tv_nsec / 1000;

			/* process EWS only in airplay mode */
			if((llCurrTime - llEwsChkT) > 200000LL)    //per every 200ms
			{
				llEwsChkT = llCurrTime;
				tcc_manager_tuner_get_ews_flag(&ews_info.cur_signal);
				ews_wait_time++;

				ews_state = tcc_manager_demux_ews_getstate();
			    switch (ews_state)
			    {
				    case EWS_STOP:
					    if (ews_info.old_signal==0 && ews_info.cur_signal==1)
					    {
						    tcc_manager_demux_ews_setstate(EWS_START);

						    result = tcc_demux_section_decoder_ews_update(1);
						    if (result)
						    {
							    tcc_manager_demux_ews_setstate(EWS_ONGOING);
						    }
					    }
					    break;
				    case EWS_START:
					    if (ews_info.old_signal==1 && ews_info.cur_signal==0)
					    {
						    tcc_manager_demux_ews_setstate(EWS_STOP);
					    }
					    //if there is emergency_information_descriptor in PMT first loop,
					    //the state will be changed EWS_ONGOING from tcc_demux_section_decoder_in_broadcast() after issuing event of starting EWS.
					    break;
				    case EWS_ONGOING:
						if (ews_info.old_signal==1 && ews_info.cur_signal==0)
						{
						    ews_wait_time = 0;
							tcc_manager_demux_ews_setstate(EWS_STOPWAIT);
						    isdbt_emergency_info_clear();
					    }
					    break;
				    case EWS_STOPWAIT:
						if (ews_info.old_signal==0 && ews_info.cur_signal==1)
						{
							tcc_manager_demux_ews_setstate(EWS_START);

							result = tcc_demux_section_decoder_ews_update(1);
							if (result)
							{
								tcc_manager_demux_ews_setstate(EWS_ONGOING);
							}
					    }
					    else
					    {
						    if (ews_wait_time > 450)    //200ms * 450 = 90s
						    {
								tcc_manager_demux_ews_setstate(EWS_STOP);
							    tcc_demux_section_decoder_ews_update(0);
						    }
					    }
					    break;
				    default:
					    break;
				}
				ews_info.old_signal = ews_info.cur_signal;
			}

			/* process special service in airplay mode for every 100ms */
			if ((llCurrTime - llSpecialServiceChkT) > 100000LL)
			{
				llSpecialServiceChkT = llCurrTime;
				tcc_manager_demux_specialservice_process();
			}
		}

        // Noah, IM692A-29
	    if (2 == tcc_manager_demux_get_section_decoder_state())
	    {
            tcc_manager_demux_set_section_decoder_state(0);
	    }

		if (queue_num)
		{
            // Noah, IM692A-29
			if ((1 == tcc_manager_demux_get_section_decoder_state()) && (!giSectionDecoderFull))
			{
				pSectionCmd = tcc_dxb_dequeue(SectionDecoderQueue);
				if (pSectionCmd)
				{
					result = ISDBT_TSPARSE_ProcessTable(pSectionBitPars, pSectionCmd->pData, pSectionCmd->uiDataSize, pSectionCmd->iRequestID);
					if( result >= 0 )
					{
						if(state == E_DEMUX_STATE_SCAN)
						{
							tcc_demux_section_decoder_in_scan(result, pSectionCmd->iRequestID);
						}
						else if(state == E_DEMUX_STATE_INFORMAL_SCAN)
						{
							tcc_demux_section_decoder_in_informal_scan(result, pSectionCmd->iRequestID);
						}
						else if(state == E_DEMUX_STATE_HANDOVER)
						{
							tcc_demux_section_decoder_in_handover(result, pSectionCmd->iRequestID);
						}
						else if(state == E_DEMUX_STATE_AIRPLAY)
						{
							tcc_demux_section_decoder_in_broadcast(result, pSectionCmd->iRequestID);
						}
						else if(state == E_DEMUX_STATE_IDLE)
						{
							tcc_demux_section_decoder_in_idle(result, pSectionCmd->iRequestID);
						}
					}

					tcc_mw_free(__FUNCTION__, __LINE__, pSectionCmd->pData);
					tcc_mw_free(__FUNCTION__, __LINE__, pSectionCmd);
				}
			}
			else
			{
				for (i = 0; i < queue_num; ++i)
				{
					pSectionCmd = tcc_dxb_dequeue(SectionDecoderQueue);
					if (pSectionCmd)
					{
						tcc_mw_free(__FUNCTION__, __LINE__, pSectionCmd->pData);
						tcc_mw_free(__FUNCTION__, __LINE__, pSectionCmd);
					}
				}
				giSectionDecoderFull = 0;
			}

		}
		else
		{
			usleep(5000);
		}
	}

	BITPARS_CloseBitParser( pSectionBitPars );

	CloseDB_EPG_Table();
	CloseDB_Table();

	giSectionThreadRunning = -1;

	return (void*)NULL;
}

DxB_ERR_CODE tcc_demux_section_notify(UINT8 *pucBuf,  UINT32 ulBufSize, UINT32 ulRequestId, void *pUserData)
{
	DEMUX_DEC_COMMAND_TYPE *pSectionCmd;
	int iNetworkID;
	int *pArg;

	// Noah / 20170621 / Added for IM478A-60 (The request additinal API to start/stop EPG processing)
	if (ulRequestId == DEMUX_REQUEST_EIT || ulRequestId == DEMUX_REQUEST_EIT+1 || ulRequestId == DEMUX_REQUEST_EIT+2)
	{
		if (0 == tcc_manager_demux_get_EpgOnOff())	// if EPG is off
		{
			if (pucBuf)
			{
				tcc_mw_free(__FUNCTION__, __LINE__, pucBuf);
			}
			return DxB_ERR_OK;
		}
	}

	if(ulRequestId >= DEMUX_REQUEST_CUSTOM && ulRequestId < (DEMUX_REQUEST_CUSTOM + MAX_CUSTOM_FILTER))
	{
		TCCDxBEvent_CustomFilterDataUpdate(tcc_demux_customfilterlist_getPID(ulRequestId), pucBuf, ulBufSize);
		if (pucBuf)
		{
			tcc_mw_free(__FUNCTION__, __LINE__, pucBuf);
		}
		return DxB_ERR_OK;
	}

    // Noah, IM692A-29
	if (1 != tcc_manager_demux_get_section_decoder_state())
	{
		if (pucBuf)
		{
			tcc_mw_free(__FUNCTION__, __LINE__, pucBuf);    // Noah, 20180611, Memory Leak
		}
		return DxB_ERR_OK;
	}

#if 0
	pSectionCmd = tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(DEMUX_DEC_COMMAND_TYPE));
#else
	pSectionCmd = tcc_mw_calloc(__FUNCTION__, __LINE__, 1, sizeof(DEMUX_DEC_COMMAND_TYPE));
#endif

	if(pSectionCmd == NULL){
		ALOGE("[%s] DEMUX_DEC_COMMAND_TYPE allocation fail\n", __func__);
		return DxB_ERR_NO_ALLOC;
	}

	pSectionCmd->pData = pucBuf;
	pSectionCmd->uiDataSize = ulBufSize;
	pSectionCmd->iRequestID = ulRequestId;
	pSectionCmd->pArg = NULL;

	if((ulRequestId == DEMUX_REQUEST_EMM ||ulRequestId == (DEMUX_REQUEST_EMM +1)) || ulRequestId ==DEMUX_REQUEST_ECM)
	{
		pArg = tcc_mw_malloc(__FUNCTION__, __LINE__, 2 * sizeof(int));
		memset(pArg, 0, 2 * sizeof(int));

		if(pArg != NULL) {
			ISDBT_Get_NetworkIDInfo(&iNetworkID);
			pArg[0] = iNetworkID;
			pArg[1] = gCurrentChannel;
			pSectionCmd->pArg = (void*)pArg;
		}

		if (tcc_manager_cas_pushSectionData((void*)pSectionCmd) != ISDBT_CAS_SUCCESS)
		{
			if(pSectionCmd->pArg != NULL) {
				tcc_mw_free(__FUNCTION__, __LINE__, pSectionCmd->pArg);
			}
			tcc_mw_free(__FUNCTION__, __LINE__, pSectionCmd->pData);
			tcc_mw_free(__FUNCTION__, __LINE__, pSectionCmd);
		}
	}
	else
	{
		if (!giSectionDecoderFull)
		{
			if( tcc_dxb_queue_ex(SectionDecoderQueue, pSectionCmd) == 0 )
			{
				int elem_num = tcc_dxb_getquenelem(SectionDecoderQueue);

				ALOGD("[%s] SQ full (reqId:0x%x, TableId:0x%x, queue:%d->%d\n",__func__, \
						pSectionCmd->iRequestID,  *pSectionCmd->pData,
						elem_num, tcc_dxb_getquenelem(SectionDecoderQueue));

				giSectionDecoderFull = 1;

				tcc_mw_free(__FUNCTION__, __LINE__, pSectionCmd->pData);
				tcc_mw_free(__FUNCTION__, __LINE__, pSectionCmd);
			}
		}
		else
		{
			tcc_mw_free(__FUNCTION__, __LINE__, pSectionCmd->pData);
			tcc_mw_free(__FUNCTION__, __LINE__, pSectionCmd);
		}
	}

	return DxB_ERR_OK;
}

int tcc_demux_send_dec_ctrl_cmd(tcc_dxb_queue_t* pDecoderQueue, E_DECTHREAD_CTRLCOMMAND eESCmd, int iSync, void *pArg)
{
	int ret = 0;
	DEMUX_DEC_COMMAND_TYPE *pDecCmd;
	tcc_dxb_sem_t *pCmdSync = NULL;
	int err;

#if 0
	pDecCmd = tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(DEMUX_DEC_COMMAND_TYPE));
#else
	pDecCmd = tcc_mw_calloc(__FUNCTION__, __LINE__, 1, sizeof(DEMUX_DEC_COMMAND_TYPE));
#endif

	if( pDecCmd )
	{
		pDecCmd->eCommandType = DEC_CMDTYPE_CONTROL;
		pDecCmd->eCtrlCmd = eESCmd;
		pDecCmd->iCommandSync = iSync;
		pDecCmd->pArg = pArg;
		if( pDecCmd->iCommandSync )
		{
			pCmdSync = (tcc_dxb_sem_t *)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(tcc_dxb_sem_t));
			memset(pCmdSync, 0, sizeof(tcc_dxb_sem_t));

			tcc_dxb_sem_init(pCmdSync, 0);
			pDecCmd->pData = (unsigned char *)pCmdSync;
		}

		if( tcc_dxb_queue_ex(pDecoderQueue, pDecCmd) == 0 )
		{
			ALOGE("%s send cmd fail !!!", __func__);
			if(pDecCmd->iCommandSync)
			{
				tcc_dxb_sem_deinit(pCmdSync);
				tcc_mw_free(__FUNCTION__, __LINE__, pCmdSync);
			}

			if(pDecCmd->pArg){
				tcc_mw_free(__FUNCTION__, __LINE__, pDecCmd->pArg);
			}

			tcc_mw_free(__FUNCTION__, __LINE__, pDecCmd);
		}
	}
	else
		ret = 1;

	return ret;
}

DxB_ERR_CODE tcc_demux_alloc_buffer(UINT32 usBufLen, UINT8 **ppucBuf)
{
	*ppucBuf = tcc_mw_malloc(__FUNCTION__, __LINE__, usBufLen);
	return DxB_ERR_OK;
}

DxB_ERR_CODE tcc_demux_free_buffer( UINT8 *pucBuf)
{
	MNG_DBG("In %s\n", __func__);

	tcc_mw_free(__FUNCTION__, __LINE__, pucBuf);
	return DxB_ERR_OK;
}

/**
 * @brief
 *
 * @param
 *
 * @return
 *     0 : Success
 *    -1 : TCC_DxB_DEMUX_Init error
 *    -2 : tcc_manager_cas_init error
 */
int tcc_manager_demux_init(void)
{
	int status;
	int err=0;

	tcc_manager_demux_dataservice_lock_init();
	tcc_manager_demux_filter_lock_init();

	err = TCC_DxB_DEMUX_Init(hInterface, DxB_STANDARD_ISDBT, 0, 0);
	if (err != 0 /*DxB_ERR_OK*/)
	{
		ALOGE("[%s] tcc_manager_tuner_init Error(%d)", __func__, err);
		return -1;
	}

	ISDBT_TSPARSE_Init();
	ISDBT_Init_DescriptorInfo();
	TSPARSE_ISDBT_SiMgmt_Init();
	giSectionThreadRunning = 1;

	giDemuxScanStop = 0;

	SectionDecoderQueue = tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(tcc_dxb_queue_t));
	memset(SectionDecoderQueue, 0, sizeof(tcc_dxb_queue_t));

	tcc_dxb_queue_init_ex(SectionDecoderQueue, SECTION_QUEUESIZE);

	status = tcc_dxb_thread_create((void *)&SectionDecoderThreadID,
								tcc_demux_section_decoder,
								(unsigned char*)"tcc_demux_section_decoder",
								HIGH_PRIORITY_2,
								NULL);
	if(status){
		ALOGE("[%s] CAN NOT make Section decoder !!!! Error(%d)\n", __func__, status);
	}

	TCC_DxB_DEMUX_RegisterSectionCallback(hInterface, tcc_demux_section_notify, tcc_demux_alloc_buffer);

	tcc_manager_demux_subtitle_init();
	tcc_manager_demux_set_eit_subtitleLangInfo_clear();

	gpDemuxScanWait = (tcc_dxb_sem_t *)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(tcc_dxb_sem_t));

	if (gpDemuxScanWait == NULL)
	{
		ALOGE("[%s] gpDemuxScanWait allocate fail\n", __func__);
	}

	memset(gpDemuxScanWait, 0, sizeof(tcc_dxb_sem_t));

	err = tcc_manager_cas_init();
	if (err != 0 /*ISDBT_CAS_SUCCESS*/)
	{
		ALOGE("[%s] tcc_manager_tuner_init Error(%d)", __func__, err);
		return -2;
	}

	tcc_demux_init_searchlist();
	tcc_manager_demux_dataservice_init();
	tcc_demux_customfilterlist_init();

	return 0;
}

int tcc_manager_demux_deinit(void)
{
	int err;

	giSectionThreadRunning = 0;

	while(1)
	{
		if( giSectionThreadRunning == -1)
			break;
		usleep(5000);
	}
	tcc_demux_customfilterlist_unload();
	tcc_manager_demux_dataservice_init();
	tcc_manager_demux_set_eit_subtitleLangInfo_clear();
	tcc_manager_demux_subtitle_deinit();

	if(TCC_Isdbt_IsSupportCAS())
	{
		tcc_demux_stop_cat_filtering();
		tcc_demux_stop_ecm_filtering();
		tcc_demux_stop_emm_filtering();
	}

	tcc_manager_cas_deinit();

	err = tcc_dxb_thread_join((void *)SectionDecoderThreadID,NULL);

	tcc_mw_free(__FUNCTION__, __LINE__, gpDemuxScanWait);
	TCC_DxB_DEMUX_Deinit(hInterface);
	TSPARSE_ISDBT_SiMgmt_Deinit();

	tcc_dxb_queue_deinit(SectionDecoderQueue);
	tcc_mw_free(__FUNCTION__, __LINE__, SectionDecoderQueue);
	tcc_manager_demux_filter_lock_deinit();
	tcc_manager_demux_dataservice_lock_deinit();

    ISDBT_TSPARSE_Deinit();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)

	return 0;
}

int tcc_manager_demux_set_serviceID(int iServiceID)
{
	MNG_DBG("In %s serviceid(%d)\n", __func__, iServiceID);

	gCurrentServiceID = iServiceID;
	return 0;
}

int tcc_manager_demux_get_serviceID(void)
{
	return gCurrentServiceID;
}

int tcc_manager_demux_set_validOutput(int valid)
{
	MNG_DBG("In %s serviceid valid?(%d)\n", __func__, valid);
	gServiceValid = valid;
	TCCDxBEvent_ServiceID_Check(valid);
	return 0;
}

int tcc_manager_demux_get_validOutput(void)
{
	return gServiceValid;
}

int tcc_manager_demux_set_playing_serviceID(unsigned int iServiceID)
{
	MNG_DBG("In %s serviceid(%d)\n", __func__, iServiceID);

	gPlayingServiceID = iServiceID;
	return 0;
}

unsigned int tcc_manager_demux_get_playing_serviceID(void)
{
	return gPlayingServiceID;
}

int tcc_manager_demux_set_first_playing_serviceID(unsigned int iServiceID)
{
	MNG_DBG("In %s serviceid(%d)\n", __func__, iServiceID);

	gSetServiceID = iServiceID;
	return 0;
}

unsigned int tcc_manager_demux_first_get_playing_serviceID(void)
{
	return gSetServiceID;
}

int tcc_manager_demux_set_tunerinfo(int iChannel, int iFrequency, int iCountrycode)
{
	MNG_DBG("In %s ch(%d)freq(%d)contry(%d)\n", __func__, iChannel, iFrequency, iCountrycode);

	gCurrentChannel = iChannel;
	gCurrentFrequency = iFrequency;
	gCurrentCountryCode = iCountrycode;
	return 0;
}

int tcc_manager_demux_delete_db(void)
{
	DeleteDB_Table(); //Delete DB table
	return 0;
}

void tcc_manager_demux_set_channel_service_info_clear(void)
{
	UpdateDB_ChannelDB_Clear();
}

int	tcc_manager_demux_set_sectionfilter(int table_id, int pid, int request_id)
{
	int		err = 0;
	unsigned char ucValue[16], ucMask[16], ucExclusion[16];

	ucValue[0] = table_id;
	ucMask[0] = 0xff;
	ucExclusion[0] = 0x00;

	tcc_manager_demux_filter_lock_on();
	tcc_demux_init_searchlist();

	if(tcc_demux_add_searchlist(request_id) == 0)
	{
		err = TCC_DxB_DEMUX_StartSectionFilter(hInterface, pid, request_id, TRUE, 1, ucValue, ucMask, ucExclusion, TRUE, NULL);
	}
	tcc_manager_demux_filter_lock_off();
	return err;
}

int tcc_manager_demux_scan_getStrength(long long llTimeEnd)
{
	int tuner_strength_index;
	struct timespec tspec;
	long long llTime;

	if(fnGetBer) {
		clock_gettime(CLOCK_MONOTONIC , &tspec);
		llTime = (long long)tspec.tv_sec * 1000000 + (long long)tspec.tv_nsec / 1000;
		if (llTimeEnd > llTime) {
			unsigned long useconds;
			useconds = (unsigned long)(llTimeEnd - llTime);
			MNG_DBG("[%s] usleep(%d) \n", __func__, useconds);
			usleep(useconds);
		}
		fnGetBer(&tuner_strength_index);
	}
	else
		tuner_strength_index = 0;
	return tuner_strength_index;
}

/*
  * made by Noah / 2016081X / delete all fullseg PMTs remained.
*/
int tcc_demux_delete_all_fullseg_pmts_remained(void)
{
	int ret = 0, count = 0, iPMT = 0;
	unsigned short *pPMT_PID = NULL, *pTMP_PID = NULL;
	int iRequestID = DEMUX_REQUEST_PMT;

	iPMT = GetCountDB_PMT_PID_ChannelTable();
	MNG_DBG("In %s -Total %d\n", __func__, iPMT);

	if( iPMT > 0 )
	{
#if 0
		pPMT_PID = (unsigned short *)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(short) * iPMT);
#else
		pPMT_PID = (unsigned short *)tcc_mw_calloc(__FUNCTION__, __LINE__, 1, sizeof(short) * iPMT);
#endif

		GetPMT_PID_ChannelTable(pPMT_PID, iPMT);
		pTMP_PID = pPMT_PID;
	}

	if(pTMP_PID == NULL)
		return ret;

	while( count < iPMT )
	{
		if(*pTMP_PID != 0 && (*pTMP_PID < PID_PMT_1SEG || *pTMP_PID > (PID_PMT_1SEG+8)))
		{
			tcc_manager_demux_filter_lock_on();
			if(tcc_demux_delete_searchlist(iRequestID) == 0)
			{
				TCC_DxB_DEMUX_StopSectionFilter(hInterface, iRequestID);
			}
			iRequestID++;
			tcc_manager_demux_filter_lock_off();
		}
		pTMP_PID++;
		count++;
	}

	if( pPMT_PID != NULL )
	{
		tcc_mw_free(__FUNCTION__, __LINE__, pPMT_PID);
		pPMT_PID = NULL;
		pTMP_PID = NULL;
	}

	return ret;
}

/*
  * made by Noah / 2016081X / return the number of fullseg PMTs reminaed.
  *
  * return
  *   0 - there is no fullseg pmt remained
  *   else - the number of fullseg pmt remained
*/
int tcc_demux_is_fullseg_pmts_remained(void)
{
	int ret = 0, count = 0, iPMT = 0;
	unsigned short *pPMT_PID = NULL, *pTMP_PID = NULL;
	int iRequestID = DEMUX_REQUEST_PMT;

	iPMT = GetCountDB_PMT_PID_ChannelTable();
	//MNG_DBG("In %s -Total %d\n", __func__, iPMT);

	if( iPMT > 0 )
	{
#if 0
		pPMT_PID = (unsigned short *)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(short) * iPMT);
#else
		pPMT_PID = (unsigned short *)tcc_mw_calloc(__FUNCTION__, __LINE__, 1, sizeof(short) * iPMT);
#endif

		GetPMT_PID_ChannelTable(pPMT_PID, iPMT);
		pTMP_PID = pPMT_PID;
	}

	if(pTMP_PID == NULL)
		return ret;

	while( count < iPMT )
	{
		if(*pTMP_PID != 0 && (*pTMP_PID < PID_PMT_1SEG || *pTMP_PID > (PID_PMT_1SEG+8)))
		{
			tcc_manager_demux_filter_lock_on();
			if(tcc_demux_check_searchlist(iRequestID) == 0)
			{
				ret++;
			}
			iRequestID++;
			tcc_manager_demux_filter_lock_off();
		}
		pTMP_PID++;
		count++;
	}

	if( pPMT_PID != NULL )
	{
		tcc_mw_free(__FUNCTION__, __LINE__, pPMT_PID);
		pPMT_PID = NULL;
		pTMP_PID = NULL;
	}

	return ret;
}

/*
  * option
  *  b0 - if set, check a signal strength of found channel and replace old channel info with new one when there was already a channel info of same netowrk_id and found channel is stronger than old one.
  *  b1 - if set, it means autosearch. it's required to maintain only one channel info which was found by autosearch.
  *  b2 - if set, it means rescan. If there is same service in DB, discard the scanned service to maintain DB gathered by initial scan.
  * return
  *  0 = success
  *  -1 = failure
  *  1 = canceled
  */
int tcc_manager_demux_scan(int channel, int country_code, int option, void *ptr)
{
	char value[PROPERTY_VALUE_MAX];
	unsigned int uiLen = 0;
	int	rtn = 0;
	int skip_nit_flag=0;
	int	tuner_strength_index;
	SERVICE_FOUND_INFO_T *pSvcFound = (SERVICE_FOUND_INFO_T*)ptr;
	int svc;
	int special_service_support, data_service_support;
	int tsid_arg;
	int del_option = 1;
	E_SCAN_STATE eScanState;

	struct timespec tspec;
	long long llTime, llTimeEnd;

	if(0 < gTunerGetStrIdxTime && gTunerGetStrIdxTime < 10000)
	{
		clock_gettime(CLOCK_MONOTONIC , &tspec);
		llTime = (long long)tspec.tv_sec * 1000000 + (long long)tspec.tv_nsec / 1000;
		llTimeEnd = llTime + (long long)gTunerGetStrIdxTime * 1000;	//gTunerGetStrIdxTime is in ms.
	}
	else
	{
		llTime = llTimeEnd = 0;
	}

	MNG_DBG("In %s(%d)\n", __func__, gTunerGetStrIdxTime);

	if (giDemuxScanStop)
	{
		giDemuxScanStop = 0;
		return 1;
	}

	if(option & ISDBT_SCANOPTION_INFORMAL)
	{
		ISDBT_Init_DescriptorInfo();

		ISDBT_TSPARSE_ServiceList_Init();

		tcc_manager_demux_cas_init();

		/* ---- backscan ----*/
		tcc_manager_demux_backscan_reset();

		if(TCC_Isdbt_IsSupportProfileA())
		{
			tcc_manager_demux_backscan_start_scanfullseg();
		}

		/* ---- special service ----*/
		tcc_manager_demux_specialservice_init();
		tcc_manager_demux_specialservice_set_state (SPECIAL_SERVICE_NONE);

		/*----- section notification -----*/
		tcc_manager_demux_reset_section_notification();

		tcc_manager_demux_set_skip_sdt_in_scan(TRUE);

		del_option = 0;
	}
	else
	{
		ISDBT_TSPARSE_ServiceList_Init();
	}

	special_service_support = TCC_Isdbt_IsSupportSpecialService();
	data_service_support = TCC_Isdbt_IsSupportDataService();

	/*
	 * tcc.dxb.isdbt.skipnit should not be used for real product.
	 * this property is used only for testing special streams which doesn't have NIT in itself.
	 */
	uiLen = property_get("tcc.dxb.isdbt.skipnit", value, "");
	if( uiLen ) skip_nit_flag = atoi(value);

	if(skip_nit_flag == 0)
	{
		if(option & ISDBT_SCANOPTION_INFORMAL)
		{
			tcc_demux_start_nit_filtering();
		}
		else
		{
			if(tcc_manager_demux_set_sectionfilter(NIT_A_ID, PID_NIT, DEMUX_REQUEST_NIT))
			{
				MNG_DBG("In %s - Fileter ID Set Fail !!!\n", __func__);
				rtn = -1;
			}
		}
	}
	else
	{
		if (TCC_Isdbt_IsSupportProfileA())
		{
			tcc_demux_load_pat();
		}
		else
		{
			tcc_demux_load_1seg_pmts_all();
		}
	}

	g_isAddBit_ForSearchAndSetChannel = 1;	// Noah / 20170706 / IM478A-30 / In case of the searchAndSetChannel, NIT & BIT have infinite timer.

	tcc_demux_load_bit();

	if (!tcc_manager_demux_is_skip_sdt_in_scan())
	{
		tcc_demux_load_sdt();
	}

	if (rtn==0)
	{
		tcc_manager_demux_set_section_decoder_state(1);
		tcc_dxb_sem_init(gpDemuxScanWait, 0);
		tcc_manager_demux_scan_init();
		tcc_manager_demux_scan_set_state(SCAN_STATE_NIT);

		// Noah / 20170706 / IM478A-30 / In case of the searchAndSetChannel, NIT & BIT have infinite timer.
		if(option & ISDBT_SCANOPTION_INFINITE_TIMER)
		{
			tcc_manager_demux_scan_set_infinite_timer(1);	 // Enable infinite timeout
		}
		else
		{
			tcc_manager_demux_scan_set_infinite_timer(0);	 // disable infinite timeout
		}

		while (1)
		{
			usleep(TCC_MANAGER_DEMUX_SCAN_CHECKTIME*1000);

			if (gpDemuxScanWait->semval > 0)	/* all sections are received */
				break;

			if (giDemuxScanStop)	/* scan is canceled by application */
				break;

			eScanState = tcc_manager_demux_scan_get_state();

			if (eScanState == SCAN_STATE_NIT)
			{
				if (tcc_manager_demux_scan_check_timeout())
				{
					tcc_manager_demux_scan_set_state(SCAN_STATE_NONE);
					rtn = -1;
					break;
				}
				else if (tcc_manager_demux_scan_get_nit())
				{
					if (TCC_Isdbt_IsSupportProfileA())
						tcc_manager_demux_scan_set_state(SCAN_STATE_PAT);
					else
						tcc_manager_demux_scan_set_state(SCAN_STATE_COMPLETE);
				}
			}
			else if (eScanState == SCAN_STATE_PAT)
			{
				if (tcc_manager_demux_scan_check_timeout())
				{
					tcc_demux_stop_pat_filtering();
					if(option & ISDBT_SCANOPTION_INFORMAL)
					{
						tcc_manager_demux_need_more_informal_scan();
					}
					else
					{
						tcc_manager_demux_need_more_scan();
					}

					tcc_manager_demux_scan_set_state(SCAN_STATE_COMPLETE);
				}
				else if (tcc_manager_demux_scan_get_pat())
				{
					tcc_manager_demux_scan_set_state(SCAN_STATE_PMTs);
				}
			}
			/////////////////////////////////////////////////////////////////////
			// Noah / 2016081X /
			// About following routine - To improve scan time within weak signal, I added PMTs timeout.
			// The timeout value is 1 second, but whenever one of fullseg PMTs comes, the value is reset.
			//     Before
			//         BIT, SDT, NIT -> NIT complete -> PAT -> PAT complete -> PMTs -> waiting PMTs until ISDBT_DEMUX_SCAN_TIMEOUT.
			//     After
			//         BIT, SDT, NIT -> NIT complete -> PAT -> PAT complete -> PMTs with ISDBT_DEMUX_SCAN_TIMEOUT_PMTs
			//         -> If PMTs is expired, scan is terminated.
			else if (eScanState == SCAN_STATE_PMTs)
			{
				if (tcc_manager_demux_scan_check_timeout())
				{
					tcc_demux_delete_all_fullseg_pmts_remained();
					tcc_manager_demux_need_more_scan();
					tcc_manager_demux_scan_set_state(SCAN_STATE_COMPLETE);
				}
				else if (!tcc_demux_is_fullseg_pmts_remained())
				{
					tcc_manager_demux_scan_set_state(SCAN_STATE_COMPLETE);
				}
			}
			// Noah / 2016081X
			/////////////////////////////////////////////////////////////////////
			else if (eScanState == SCAN_STATE_COMPLETE)
			{
				if (tcc_manager_demux_scan_check_timeout())
				{
					tcc_manager_demux_scan_set_state(SCAN_STATE_NONE);
					break;
				}
			}
			else
			{
				rtn = -1;
				break;
			}
		}

		if (gpDemuxScanWait->semval > 0)
		{
			tcc_dxb_sem_down(gpDemuxScanWait);

			if ((tcc_manager_demux_scan_get_pat() == 0) && (gi1SEG_scan_FoundPMTCount == 0))	//if found only NIT(no f-seg and 1-seg)
			{
				rtn = -1;
			}
		}
		else
		{
			if (TCC_Isdbt_IsSupportProfileA())
			{
				rtn = -1;
			}
			else
			{
				if (gi1SEG_scan_FoundPMTCount)	//if found at least 1 service
					rtn = 0;
				else
					rtn = -1;
			}
			if (rtn == -1)
			{
				svc = UpdateDB_Get_ChannelSvcCnt();
				if (svc > 0)
					rtn = 0; // when some table is not transmitted
			}
		}

		if (giDemuxScanStop || rtn ==-1)
		{
			if (giDemuxScanStop)
			{
				giDemuxScanStop = 0;
				MNG_DBG("In %s - Scan Canceled !!!\n", __func__);
				rtn = 1;
			}
			else
			{
				MNG_DBG("In %s - Scan Failed !!!\n", __func__);
				rtn = -1;
			}

			if ((option & ISDBT_SCANOPTION_AUTOSEARCH && (pSvcFound != NULL)))
			{
				pSvcFound->found_no = 0;
				pSvcFound->pServiceFound = NULL;
			}

			tcc_demux_unload_all_searchlist_filters();
			tcc_manager_demux_set_channel_service_info_clear();
			TSPARSE_ISDBT_SiMgmt_Clear();
			tcc_manager_demux_set_section_decoder_state(0);

			return rtn;
		}

		if ((option & (ISDBT_SCANOPTION_MANUAL|ISDBT_SCANOPTION_INFORMAL)) && (ptr !=NULL))
		{
			/* check if network_id is same with tsid specified via *ptr */
			tsid_arg = *(int*)ptr;
			if (tsid_arg > 0)
			{
				if (UpdateDB_CheckNetworkID(tsid_arg) != 0)
				{
					/* network_id is different */
					MNG_DBG("In %s, network_id is different with argument(0x%x)", __func__, tsid_arg);

					tcc_demux_unload_all_searchlist_filters();
					tcc_manager_demux_set_channel_service_info_clear();
					TSPARSE_ISDBT_SiMgmt_Clear();
					tcc_manager_demux_set_section_decoder_state(0);

					rtn = -2;
					return rtn;
				}
			}
		}

		/* it's moved to decrease scan time from ISDBT_TSPARSE_ProcessBIT(). The 3rd parameter is not used in current implementation.  */
		UpdateDB_AffiliationID_ChannelTable(gCurrentChannel, gCurrentCountryCode, NULL);

		if(fnGetBer && rtn==0)
			tuner_strength_index = tcc_manager_demux_scan_getStrength(llTimeEnd);
		else
			tuner_strength_index = 0;

		UpdateDB_Strength_ChannelTable (tuner_strength_index, -1);

		if (special_service_support)
		{
			//when special service is supported, special service is discarded on scanning channels.
			//special service will be found and notified only while playing regular service
			UpdateDB_ArrangeSpecialService();
		}

		if (!data_service_support)
		{
			/* delete data service from channel information when data_service_support is not set from specific_information */
			UpdateDB_ArrangeDataService();
		}

		if (option & ISDBT_SCANOPTION_ARRANGE_RSSI)
			UpdateDB_Arrange_ChannelTable();	//if there is already same netowrk_id in DB, select stronger channel.

		svc = 0;

		if ((option & ISDBT_SCANOPTION_AUTOSEARCH) && (pSvcFound != NULL))
		{
			svc = UpdateDB_Get_ChannelInfo(pSvcFound);
			if (svc > 0)
			{
				//delete channel info which was found by autosearch. it's to maintain only 1 channel info for autosearch.
				//and check if there is already a channel info which is same with one found by autosearch.
				//if there is same channel info in DB with one found by autosearch, the channel info found by autosearch should be skipped.
				UpdateDB_Del_AutoSearch();
				UpdateDB_Del_CustomSearch(RECORD_PRESET_CUSTOM_AFFILIATION);
				UpdateDB_ArrangeAutoSearch(pSvcFound);

				//update DB and set a preset field to 2. It's to distinguish a channel info found by autosearch from others (initial scan, rescan, manual scan).
				UpdateDB_Channel_SQL_AutoSearch(channel, country_code, (int)-1);
			}
		}

		if (option & ISDBT_SCANOPTION_RESCAN)
		{
			UpdateDB_Del_AutoSearch();	//according to request of kenwood, delete autosearch info
			UpdateDB_Del_CustomSearch(RECORD_PRESET_CUSTOM_AFFILIATION);
			UpdateDB_Set_ArrangementType(SERVICE_ARRANGE_TYPE_RESCAN);
		}

		if (option & ISDBT_SCANOPTION_HANDOVER)
		{
			UpdateDB_Set_ArrangementType(SERVICE_ARRANGE_TYPE_HANDOVER);
		}

		if (!(option & ISDBT_SCANOPTION_AUTOSEARCH))
		{
			//if it's not AutoSearch
			UpdateDB_Channel_SQL(channel, country_code, (int)-1, del_option);
		}
	}

	if(!(option & ISDBT_SCANOPTION_INFORMAL))
	{
		//if it's not informal scan
		tcc_demux_unload_all_searchlist_filters();
		tcc_manager_demux_set_channel_service_info_clear();
		TSPARSE_ISDBT_SiMgmt_Clear();
		tcc_manager_demux_set_section_decoder_state(0);
	}

//	tcc_manager_demux_set_section_decoder_state(0);

	return rtn;
}

/* option
  *  b4 - if set, it means customized relay station search for JK
  *  b5 - if set, it means customized affiliation station search for JK
  * return
  *  0 = success
  * -1 = failure
  * -2 = used only for customized search. It menas some services are found but not matched with relay station.
  *  1 = canceled
  */
int tcc_manager_demux_scan_customize(int channel, int country_code, int option, int *tsid, void *ptr)
{
	int rtn = 0;
	int tuner_strength_index;
	int svc;
	int special_service_support, data_service_support;
	E_SCAN_STATE eScanState;

	struct timespec tspec;
	long long llTime, llTimeEnd;

	if(0 < gTunerGetStrIdxTime && gTunerGetStrIdxTime < 10000) {
		clock_gettime(CLOCK_MONOTONIC , &tspec);
		llTime = (long long)tspec.tv_sec * 1000000 + (long long)tspec.tv_nsec / 1000;
		llTimeEnd = llTime + (long long)gTunerGetStrIdxTime * 1000; //gTunerGetStrIdxTime is in ms.
	} else {
		llTime = llTimeEnd = 0;
	}

	MNG_DBG("In %s(%d)\n", __func__, gTunerGetStrIdxTime);

	if (giDemuxScanStop) {
		giDemuxScanStop = 0;
		return 1;
	}

	ISDBT_TSPARSE_ServiceList_Init();

	if(tcc_manager_demux_set_sectionfilter(NIT_A_ID, PID_NIT, DEMUX_REQUEST_NIT)){
		MNG_DBG("In %s - Fileter ID Set Fail !!!\n", __func__);
		rtn = -1;
	}
	tcc_demux_load_bit();
	if (!tcc_manager_demux_is_skip_sdt_in_scan()) {
		tcc_demux_load_sdt();
	}

	if (rtn==0){
		tcc_manager_demux_set_section_decoder_state(1);
		tcc_dxb_sem_init(gpDemuxScanWait, 0);
		tcc_manager_demux_scan_init();
		tcc_manager_demux_scan_set_state(SCAN_STATE_NIT);
		while (1) {
			usleep(TCC_MANAGER_DEMUX_SCAN_CHECKTIME*1000);
			if (gpDemuxScanWait->semval > 0) break;	/* all sections are received */
			if (giDemuxScanStop) break;	/* scan is canceled by application */
			eScanState = tcc_manager_demux_scan_get_state();
			if (eScanState == SCAN_STATE_NIT) {
				if (tcc_manager_demux_scan_check_timeout()) {
					tcc_manager_demux_scan_set_state(SCAN_STATE_NONE);
					rtn = -1;
					break;
				} else if (tcc_manager_demux_scan_get_nit()) {
					if (TCC_Isdbt_IsSupportProfileA())
						tcc_manager_demux_scan_set_state(SCAN_STATE_PAT);
					else
						tcc_manager_demux_scan_set_state(SCAN_STATE_COMPLETE);
				}
			} else if (eScanState == SCAN_STATE_PAT) {
				if (tcc_manager_demux_scan_check_timeout()) {
					tcc_demux_stop_pat_filtering();
					tcc_manager_demux_need_more_scan();
					tcc_manager_demux_scan_set_state(SCAN_STATE_COMPLETE);
				} else if (tcc_manager_demux_scan_get_pat()) {
					tcc_manager_demux_scan_set_state(SCAN_STATE_COMPLETE);
				}
			} else if (eScanState == SCAN_STATE_COMPLETE) {
				if (tcc_manager_demux_scan_check_timeout()) {
					tcc_manager_demux_scan_set_state(SCAN_STATE_NONE);
					break;
				}
			} else {
				rtn = -1;
				break;
			}
		}
		if (gpDemuxScanWait->semval > 0) {
			tcc_dxb_sem_down(gpDemuxScanWait);
			if ((tcc_manager_demux_scan_get_pat() == 0) && (gi1SEG_scan_FoundPMTCount == 0)) //if found only NIT(no f-seg and 1-seg)
				rtn = -1;
		} else if (!giDemuxScanStop){
			if (TCC_Isdbt_IsSupportProfileA()) {
				rtn = -1;
			} else {
				if (gi1SEG_scan_FoundPMTCount) rtn = 0; //if found at least 1 service
				else rtn = -1;
			}
			if (rtn == -1) {
				svc = UpdateDB_Get_ChannelSvcCnt();
				if (svc > 0) rtn = 0; // when some table is not transmitted
			}
		}
		if (giDemuxScanStop || rtn ==-1) {
			if (giDemuxScanStop) {
				giDemuxScanStop = 0;
				MNG_DBG("In %s - Scan Canceled !!!\n", __func__);
				rtn = 1;
			} else {
				MNG_DBG("In %s - Scan Failed !!!\n", __func__);
				rtn = -1;
				if (fnGetBer) tuner_strength_index = tcc_manager_demux_scan_getStrength (llTimeEnd);
				else tuner_strength_index = 0;
				UpdateDB_CustomSearch_AddReport_SignalStrength (tuner_strength_index, ptr);
			}
			tcc_demux_unload_all_searchlist_filters();
			tcc_manager_demux_set_channel_service_info_clear();
			TSPARSE_ISDBT_SiMgmt_Clear();
			tcc_manager_demux_set_section_decoder_state(0);
			return rtn;
		}

		/* it's moved to decrease scan time from ISDBT_TSPARSE_ProcessBIT(). The 3rd parameter is not used in current implementation.  */
		UpdateDB_AffiliationID_ChannelTable(gCurrentChannel, gCurrentCountryCode, NULL);

		if(fnGetBer && rtn==0)
			tuner_strength_index = tcc_manager_demux_scan_getStrength (llTimeEnd);
		else
			tuner_strength_index = 0;
		UpdateDB_Strength_ChannelTable (tuner_strength_index, -1);

		special_service_support = TCC_Isdbt_IsSupportSpecialService();
		if (special_service_support) {
			//when special service is supported, special service is discarded on scanning channels.
			//special service will be found and notified only while playing regular service
			UpdateDB_ArrangeSpecialService();
		}

		data_service_support = TCC_Isdbt_IsSupportDataService();
		if (!data_service_support) {
			/* delete data service from channel information when data_service_support is not set from specific_information */
			UpdateDB_ArrangeDataService();
		}

		rtn = UpdateDB_CustomSearch(channel, country_code, option, tsid, ptr);
		UpdateDB_CustomSearch_AddReport_SignalStrength (tuner_strength_index, ptr);
	}

	tcc_demux_unload_all_searchlist_filters();
	tcc_manager_demux_set_channel_service_info_clear();
	TSPARSE_ISDBT_SiMgmt_Clear();
	tcc_manager_demux_set_section_decoder_state(0);

	return rtn;
}
/*
  * it's only for customizing request from customer
  *   to return a signal strength even when channel is not detected.
  */
int tcc_manager_demux_scan_customize_read_strength(void)
{
	int tuner_strength_index;
	struct timespec tspec;
	long long llTime, llTimeEnd;

	if(0 < gTunerGetStrIdxTime && gTunerGetStrIdxTime < 10000) {
		clock_gettime(CLOCK_MONOTONIC , &tspec);
		llTime = (long long)tspec.tv_sec * 1000000 + (long long)tspec.tv_nsec / 1000;
		llTimeEnd = llTime + (long long)gTunerGetStrIdxTime * 1000; //gTunerGetStrIdxTime is in ms.
	} else {
		llTime = llTimeEnd = 0;
	}

	if (fnGetBer) tuner_strength_index = tcc_manager_demux_scan_getStrength (llTimeEnd);
	else tuner_strength_index = 0;

	return tuner_strength_index;
}

int tcc_manager_demux_scan_epg(int channel_num, int networkID, int options)
{
	int err = 0;
	int scan_wait_count = (ISDBT_DEMUX_EPG_SCAN_TIMEOUT * 1000 / 100); /* unit 100ms */

	giEPGScanFlag = 1;	//EPGSCAN START

	tcc_db_create_epg(channel_num, networkID);

	if(giDemuxScanStop) {
		giDemuxScanStop = 0;
		err = 1;
	}

	if(options & 0x1) {
		tcc_demux_start_leit_filtering();
		giEPGScanFlag += 1;
	}
	if(options & 0x2) {
		tcc_demux_start_heit_filtering();
		giEPGScanFlag += 1;
	}

	tcc_dxb_sem_init(gpDemuxScanWait, 0);
	if (scan_wait_count <= 0) scan_wait_count = 1;
	while (scan_wait_count) {
		usleep(100*1000);
		if (gpDemuxScanWait->semval > 0) break;
		if (giDemuxScanStop) break;
		if (scan_wait_count > 0) scan_wait_count--;
	}

	if (gpDemuxScanWait->semval > 0) {
		tcc_dxb_sem_down(gpDemuxScanWait);
	}

	if(giDemuxScanStop) {
		giDemuxScanStop = 0;
		err = 1;
	}

	tcc_demux_unload_all_searchlist_filters();
	tcc_manager_demux_set_channel_service_info_clear();
	TSPARSE_ISDBT_SiMgmt_Clear();
//	TCCDxBEvent_EPGSearchingDone((void *)&err);

	giEPGScanFlag = 0;	//EPGSCAN END

	return err;
}

int	tcc_manager_demux_channeldb_backup(void)
{
	int	err =0;
	if(TCC_Isdbt_IsSupportChannelDbBackUp())
		err = ChannelDB_BackUp();
	return err;
}

int tcc_manager_demux_channeldb_restoration(void)
{
	int	err =0;
	if(TCC_Isdbt_IsSupportChannelDbBackUp())
		err = ChannelDB_Recover();
	return err;
}

int tcc_manager_demux_scan_cancel(void)
{
	giDemuxScanStop = 1;
	return 0;
}

int tcc_manager_demux_handover(void)
{
	unsigned int uiLen = 0;
	int	rtn = 0;
	int scan_wait_count = (ISDBT_DEMUX_SCAN_TIMEOUT * 1000 / 100); /* unit 100ms */

	MNG_DBG("In %s\n", __func__);

	if(tcc_manager_demux_set_sectionfilter(NIT_A_ID, PID_NIT, DEMUX_REQUEST_NIT)){
		MNG_DBG("In %s - Fileter ID Set Fail !!!\n", __func__);
		rtn = -1;
	}

	if (rtn == 0){
		tcc_manager_demux_set_section_decoder_state(1);
		tcc_dxb_sem_init(gpDemuxScanWait, 0);
		if (scan_wait_count <= 0) scan_wait_count = 1;
		while (scan_wait_count) {
			usleep(100*1000);
			if (gpDemuxScanWait->semval > 0) break;
			if (giDemuxScanStop) {
				break;
			}
			if (scan_wait_count > 0) scan_wait_count--;
		}
		if (gpDemuxScanWait->semval > 0) {
			tcc_dxb_sem_down(gpDemuxScanWait);
		} else {
			MNG_DBG("In %s - Scan Failed !!!\n", __func__);
			if (giDemuxScanStop) {
				giDemuxScanStop = 0;
				rtn = 1;
			} else
				rtn = -1;
		}
	}

	tcc_demux_unload_all_searchlist_filters();
	tcc_manager_demux_set_channel_service_info_clear();
	TSPARSE_ISDBT_SiMgmt_Clear();
	tcc_manager_demux_set_section_decoder_state(0);
	return rtn;
}

void tcc_manager_demux_start_av_pid
(
	int AudioPID, int VideoPID,
	int AudioType, int VideoType,
       int AudioSubPID, int VideoSubPID,
       int AudioSubType, int VideoSubType,
	int PcrPID, int PcrSubPID
)
{
	DxB_Pid_Info AVPidInfo;

	MNG_DBG("In %s\n", __func__);

	memset(&AVPidInfo, 0x0, sizeof(DxB_Pid_Info));

	if (PcrPID > 2 && PcrPID < 0x1FFF)
	{
		AVPidInfo.pidBitmask |= PID_BITMASK_PCR;
		AVPidInfo.usPCRPid = PcrPID;
	}
	if (PcrSubPID > 2 && PcrSubPID < 0x1FFF)
	{
		AVPidInfo.pidBitmask |= PID_BITMASK_PCR_SUB;
		AVPidInfo.usPCRSubPid = PcrSubPID;
	}
	if (VideoPID > 2 && VideoPID < 0x1FFF)
	{
		ALOGD("VideoMain=0x%x(%d), 0x%x\n", VideoPID, VideoPID, VideoType);
		AVPidInfo.pidBitmask |= PID_BITMASK_VIDEO_MAIN;
		AVPidInfo.usVideoMainPid = VideoPID;
		AVPidInfo.usVideoMainType = VideoType;
	}
	if (AudioPID > 2 && AudioPID < 0x1FFF)
	{
		ALOGD("AudioMain=0x%x(%d), 0x%x\n", AudioPID, AudioPID, AudioType);
		AVPidInfo.pidBitmask |= PID_BITMASK_AUDIO_MAIN;
		AVPidInfo.usAudioMainPid = AudioPID;
		AVPidInfo.usAudioMainType = AudioType;
	}

	if (VideoSubPID > 2 && VideoSubPID < 0x1FFF)
	{
		ALOGD("VideoSub=0x%x(%d), 0x%x\n", VideoSubPID, VideoSubPID, VideoSubType);
		AVPidInfo.pidBitmask |= PID_BITMASK_VIDEO_SUB;
		AVPidInfo.usVideoSubPid = VideoSubPID;
		AVPidInfo.usVideoSubType = VideoSubType;
	}
	if (AudioSubPID > 2 && AudioSubPID < 0x1FFF)
	{
		ALOGD("AudioSub=0x%x(%d), 0x%x\n", AudioSubPID, AudioSubPID,  AudioSubType);
		AVPidInfo.pidBitmask |= PID_BITMASK_AUDIO_SUB;
		AVPidInfo.usAudioSubPid = AudioSubPID;
		AVPidInfo.usAudioSubType = AudioSubType;
	}

	//AVPidInfo.pidBitmask = (PID_BITMASK_VIDEO_MAIN | PID_BITMASK_AUDIO_MAIN);
	TCC_DxB_DEMUX_StartAVPID(hInterface, &AVPidInfo);
}

int tcc_manager_demux_av_play
(
	int AudioPID, int VideoPID,
	int AudioType, int VideoType,
    int AudioSubPID, int VideoSubPID,
    int AudioSubType, int VideoSubType,
	int PcrPID, int PcrSubPID,
	int PmtPID, int PmtSubPID,
	int CAECMPID, int ACECMPID,
	int dual_mode
)
{
	MNG_DBG("In %s\n", __func__);

	ISDBT_Init_DescriptorInfo();
	tcc_manager_demux_set_eit_subtitleLangInfo_clear();

	gCurrentPMTPID = PmtPID;
	gCurrentPMTSubPID = PmtSubPID;

	ISDBT_TSPARSE_ServiceList_Init();

	/* ---- backscan ----*/
	tcc_manager_demux_backscan_reset();
	if (PmtPID == 0 && TCC_Isdbt_IsSupportProfileA())
		tcc_manager_demux_backscan_start_scanfullseg();

	/* ---- special service ----*/
	tcc_manager_demux_specialservice_init();
	tcc_manager_demux_specialservice_set_state (SPECIAL_SERVICE_NONE);

	/*----- section notification -----*/
	tcc_manager_demux_reset_section_notification();

	//if((PmtPID > 0) ||(PmtSubPID > 0))
	//IM112A-77 no output at Broadcasting college
	{
		tcc_demux_start_nit_filtering();
		tcc_demux_start_tot_filtering();
		tcc_demux_start_cdt_filtering();
		tcc_demux_start_eit_filtering();

		if(TCC_Isdbt_IsSupportProfileA() && TCC_Isdbt_IsSupportCAS())
		{
			tcc_demux_start_cat_filtering();

			if(TCC_Isdbt_IsSupportBCAS())
			{
				tcc_demux_start_ecm_filtering(CAECMPID);
			}
			else if(TCC_Isdbt_IsSupportTRMP())
			{
				tcc_demux_start_ecm_filtering(ACECMPID);
			}
		}
	}

	tcc_manager_demux_set_section_decoder_state(1);
	tcc_manager_demux_start_av_pid(AudioPID, VideoPID, AudioType, VideoType, AudioSubPID, VideoSubPID, AudioSubType, VideoSubType, PcrPID, PcrSubPID);

	return 0;
}

int tcc_manager_demux_scan_and_avplay
(
	int AudioPID, int VideoPID,
	int AudioType, int VideoType,
	int AudioSubPID, int VideoSubPID,
	int AudioSubType, int VideoSubType,
	int PcrPID, int PcrSubPID,
	int PmtPID, int PmtSubPID,
	int CAECMPID, int ACECMPID,
	int dual_mode
)
{
	MNG_DBG("In %s\n", __func__);

	if((PmtPID > 0) ||(PmtSubPID > 0)) {
		if (PmtPID > 2 && PmtPID < 0x1FFF) {
			gCurrentPMTPID = PmtPID;
			if (tcc_demux_check_pmt_filtering(1) != 0) {
				tcc_demux_start_pmt_filtering(gCurrentPMTPID, 1);
			}
		}

		if (PmtSubPID > 2 && PmtSubPID < 0x1FFF) {
			gCurrentPMTSubPID = PmtSubPID;
			if (tcc_demux_check_pmt_filtering(0) != 0) {
				tcc_demux_start_pmt_filtering(gCurrentPMTSubPID, 0);
			}
		}

		tcc_demux_start_tot_filtering();
		tcc_demux_start_cdt_filtering();
		tcc_demux_start_eit_filtering();

		if (tcc_demux_check_pat_filtering()!=0) tcc_demux_load_broadcast_pat();//tcc_demux_load_pat();
		if (tcc_demux_check_sdt_filtering()!=0) tcc_demux_load_sdt();

		if(TCC_Isdbt_IsSupportProfileA() && TCC_Isdbt_IsSupportCAS())
		{
			tcc_demux_start_cat_filtering();
		}
	}

	tcc_manager_demux_start_av_pid(AudioPID, VideoPID, AudioType, VideoType, AudioSubPID, VideoSubPID, AudioSubType, VideoSubType, PcrPID, PcrSubPID);

	return 0;
}

int tcc_manager_demux_change_audioPID(int fAudioMain, unsigned int uiAudioMainPID, int fAudioSub, unsigned int uiAudioSubPID)
{
	DxB_Pid_Info AVPidInfo;

	MNG_DBG( "In %s AudioMainPID=%d, AudioSubPID=%d \n", __func__, uiAudioMainPID, uiAudioSubPID);

	AVPidInfo.pidBitmask = 0;
	if (fAudioMain) {
		AVPidInfo.usAudioMainPid = uiAudioMainPID;
		AVPidInfo.pidBitmask |= PID_BITMASK_AUDIO_MAIN;
	}
	if (fAudioSub) {
		AVPidInfo.usAudioSubPid = uiAudioSubPID;
		AVPidInfo.pidBitmask |= PID_BITMASK_AUDIO_SUB;
	}
	if (fAudioMain || fAudioSub)
		TCC_DxB_DEMUX_StartAVPID(hInterface, &AVPidInfo);
	return 0;
}

int tcc_manager_demux_change_videoPID(int fVideoMain, int VideoPID, int VideoType, int fVideoSub, int VideoSubPID, int VideoSubType)
{
	DxB_Pid_Info AVPidInfo;

	ALOGD("[%s] VideoPID = %d VideoType = %d \n", __func__, VideoPID, VideoType);

	AVPidInfo.pidBitmask = 0;
	if (fVideoMain) {
		AVPidInfo.usVideoMainPid	= VideoPID;
		AVPidInfo.usVideoMainType	= VideoType;
		AVPidInfo.pidBitmask 		= PID_BITMASK_VIDEO_MAIN;
	}
	if (fVideoSub) {
		AVPidInfo.usVideoSubPid 	= VideoSubPID;
		AVPidInfo.usVideoSubType	= VideoSubType;
		AVPidInfo.pidBitmask 		= PID_BITMASK_VIDEO_SUB;
	}

	TCC_DxB_DEMUX_StartAVPID(hInterface, &AVPidInfo);
	TCC_DxB_DEMUX_ResetVideo(hInterface, 0);

	return 0;
}

/**
 * @brief
 *
 * @param
 *
 * @return
 *     0 : Success
 *    -1 : StopSectionFilter error
 *    -2 : TCC_DxB_DEMUX_StopAVPID error
 */
int tcc_manager_demux_av_stop(int StopAudio, int StopVideo, int StopAudioSub, int StopVideoSub)
{
	int err = 0;
	unsigned int uiControlBitMask = 0;

	tcc_manager_demux_set_section_decoder_state(0);

	err |= tcc_demux_stop_cdt_filtering();
	err |= tcc_demux_stop_nit_filtering();
	err |= tcc_demux_stop_eit_filtering();
	err |= tcc_demux_stop_tot_filtering();
	err |= tcc_demux_stop_bit_filtering();
	err |= tcc_demux_stop_pmt_filtering();
	err |= tcc_demux_stop_sdt_filtering();
	err |= tcc_demux_stop_pat_filtering();

	if (TCC_Isdbt_IsSupportCAS())
	{
		err |= tcc_demux_stop_cat_filtering();
		err |= tcc_demux_stop_emm_filtering();
	}


	tcc_demux_customfilterlist_unload();
	tcc_manager_demux_dataservice_init();
	err |= tcc_demux_stop_pmt_backscan_filtering();

	if (err != 0)
	{
		ALOGE("[%s] StopSectionFilter Error(%d)", __func__, err);
		return -1;
	}

	tcc_manager_demux_backscan_reset();

	gCurrentPMTPID = -1;
	gCurrentPMTSubPID = -1;

	if(StopAudio)
		uiControlBitMask |= PID_BITMASK_AUDIO_MAIN;
	if(StopVideo)
		uiControlBitMask |= PID_BITMASK_VIDEO_MAIN;
	if(StopAudioSub)
		uiControlBitMask |= PID_BITMASK_AUDIO_SUB;
	if(StopVideoSub)
		uiControlBitMask |= PID_BITMASK_VIDEO_SUB;
	if(StopAudio && StopVideo)
		uiControlBitMask |= PID_BITMASK_PCR;
	if(StopAudioSub && StopVideoSub)
		uiControlBitMask |= PID_BITMASK_PCR_SUB;

	err = TCC_DxB_DEMUX_StopAVPID(hInterface, uiControlBitMask);
	if (err != 0 /*DxB_ERR_OK*/)
	{
		ALOGE("[%s] TCC_DxB_DEMUX_StopAVPID Error(%d)", __func__, err);
		return -2;
	}

	if (StopAudio && StopVideo && StopAudioSub && StopVideoSub)
		tcc_demux_unload_all_searchlist_filters();

	/* if a state of special service is not STOP but RUNNING,
	an event of showing a finish of special service will be issued on calling TSPARSE_ISDBT_SiMgmt_Clear() because of disappearing section data */
	tcc_manager_demux_specialservice_init();
	tcc_manager_demux_set_channel_service_info_clear ();
	tcc_manager_demux_set_skip_sdt_in_scan(FALSE);

	TSPARSE_ISDBT_SiMgmt_Clear();
	tcc_manager_demux_ews_setrequest(1);

	return 0;
}

int tcc_manager_demux_start_pcr_pid(int pcr_pid, int index)
{
	DxB_Pid_Info AVPidInfo;

	MNG_DBG("In %s\n", __func__);

	memset(&AVPidInfo, 0x0, sizeof(DxB_Pid_Info));

	if (pcr_pid > 2 && pcr_pid < 0x1FFF)
	{
		if (index ==0) {
			AVPidInfo.pidBitmask |= PID_BITMASK_PCR;
			AVPidInfo.usPCRPid = pcr_pid;
		} else {
			AVPidInfo.pidBitmask |= PID_BITMASK_PCR_SUB;
			AVPidInfo.usPCRSubPid = pcr_pid;
		}
	}
	TCC_DxB_DEMUX_StartAVPID(hInterface, &AVPidInfo);

	return 0;
}

int tcc_manager_demux_stop_pcr_pid(int index)
{
	unsigned int uiControlBitMask = 0;

	if (index==0) uiControlBitMask |= PID_BITMASK_PCR;
	else uiControlBitMask |= PID_BITMASK_PCR_SUB;

	TCC_DxB_DEMUX_StopAVPID(hInterface, uiControlBitMask);

	return 0;
}

int tcc_manager_demux_reset_av(int Audio, int Video)
{
	if(Audio)
		TCC_DxB_DEMUX_ResetAudio(hInterface, 0);

	if(Video)
		TCC_DxB_DEMUX_ResetVideo(hInterface, 0);

	return 0;
}

int tcc_manager_demux_rec_start(unsigned char *pucFileName)
{
	TCC_DxB_DEMUX_RecordStart (hInterface, 0, pucFileName);
	return 0;
}

int tcc_manager_demux_rec_stop(void)
{
	TCC_DxB_DEMUX_RecordStop(hInterface, 0);
	return 0;
}

int tcc_manager_demux_set_parentlock(unsigned int lock)
{
	TCC_DxB_DEMUX_SetParentLock(hInterface, 0, lock);
	return 0;
}

int tcc_manager_demux_set_area (int area_code)
{
	Set_User_Area_Code(area_code);
	return 0;
}

int tcc_manager_demux_set_preset (int area_code)
{
	int err = 0;

	err = UpdateDB_ChannelTablePreset (area_code);
	if(err) ALOGE("[%s] err : %d\n", __func__, err);

	if (err != 0) err = -1;
	err = TCCDxBEvent_SearchingDone(&err);
	if(err) ALOGE("[%s] err : %d\n", __func__, err);

	return err;
}

int    tcc_manager_demux_get_area (void)
{
	return Get_User_Area_Code();
}

void tcc_manager_demux_set_callback_getBER (void *arg)
{
	if (arg != NULL)
		fnGetBer =(int(*)(int*))arg;
	else
		fnGetBer = NULL;
}

//tuner_str_idx_time in ms
void tcc_manager_demux_set_strengthIndexTime (int tuner_str_idx_time)
{
	if (tuner_str_idx_time < 0) tuner_str_idx_time = 0;
	if (tuner_str_idx_time > 10000) tuner_str_idx_time = 10000;
	gTunerGetStrIdxTime = tuner_str_idx_time;
}

/* return 0 - fail, 1-success */
int tcc_manager_demux_set_presetMode (int preset_mode)
{
	int rtn, prev_handle, prev_epg_handle;

	MNG_DBG("In %s, %d", __func__, preset_mode);

	if(GetDB_Handle() == NULL) {
		ALOGE ("In %s, Error channel DB is not opened\n", __func__);
		return 0;
	}

	rtn = ChangeDB_Handle(preset_mode, &prev_handle);
	if (rtn != 0) return 0;
	rtn = ChangeDB_EPG_Handle(preset_mode, &prev_epg_handle);
	if (rtn != 0) {
		preset_mode = prev_handle;
		ChangeDB_Handle(preset_mode, &prev_handle);
		return 0;
	}
	return 1;
}

int tcc_manager_demux_get_curDCCDescriptorInfo(unsigned short usServiceID, void **pDCCInfo)
{
	void* stDescDCC = 0;

	if(ISDBT_TSPARSE_CURDCCDESCINFO_get(usServiceID, (void *)&stDescDCC))
		return 1;

	*pDCCInfo = stDescDCC;

	return 0;
}

int tcc_manager_demux_get_curCADescriptorInfo(unsigned short usServiceID, unsigned char ucDescID, void *arg)
{
	T_DESC_CONTA stDescCONTA = { 0, };
	unsigned char *piArg = (unsigned char *)arg;
	if(ISDBT_TSPARSE_CURDESCINFO_get(usServiceID, CONTENT_AVAILABILITY_ID, (void *)&stDescCONTA))
		return 1;

	piArg[0] = stDescCONTA.copy_restriction_mode;
	piArg[1] = stDescCONTA.image_constraint_token;
	piArg[2] = stDescCONTA.retention_mode;
	piArg[3] = stDescCONTA.retention_state;
	piArg[4] = stDescCONTA.encryption_mode;

	return 0;
}

int tcc_manager_demux_get_componentTag(int iServiceID, int index, unsigned char *pucComponentTag, int *piComponentCount)
{
	return isdbt_mvtv_info_get_data(iServiceID, index, pucComponentTag, piComponentCount);
}

int tcc_manager_demux_desc_update(unsigned short usServiceID, unsigned char ucDescID, void *arg) {
	return TCCDxBEvent_DESCUpdate(usServiceID, ucDescID, arg);
}

int tcc_manager_demux_event_relay(unsigned short usServiceID, unsigned short usEventID, unsigned short usOrigianlNetworkID, unsigned short usTransportStreamID) {
	return TCCDxBEvent_EventRelay(usServiceID, usEventID, usOrigianlNetworkID, usTransportStreamID);
}

int tcc_manager_demux_mvtv_update(unsigned char *pucArr) {
	return TCCDxBEvent_MVTVUpdate(pucArr);
}

void tcc_manager_demux_release_searchfilter(void)
{
	if (tcc_demux_get_valid_searchlist())
		tcc_demux_unload_all_searchlist_filters();
}

/*Detect different the number of service*/
void tcc_manager_demux_detect_service_num_Init(void)
{
//	ALOGD("[%s]\n", __func__);
	iPrevParseSvcNum = 0;
	iParseSvcNum = 0;
}

void tcc_manager_demux_check_service_num_Init(void)
{
//	ALOGD("[%s]\n", __func__);
	iCheckSvcNum = 0;
}

void tcc_manager_demux_set_service_num(int SvcNum)
{
	ALOGD("[%s][%d]\n", __func__, SvcNum);
	iParseSvcNum = SvcNum;
}

int tcc_manager_demux_get_service_num(void)
{
	return iParseSvcNum;
}

int tcc_manager_demux_detect_service_num(int SvcNum, int NetworkID)
{
	int data_service_num=0;
	int special_service_num=0;
	int state = tcc_manager_demux_get_state();

	if (state == E_DEMUX_STATE_AIRPLAY)
	{
		iPrevParseSvcNum = tcc_db_get_service_num(gCurrentChannel, gCurrentCountryCode, NetworkID);

		if (TCC_Isdbt_IsSupportSpecialService())
		{
			special_service_num = UpdateDB_Channel_GetSpecialServiceCount(gCurrentChannel, gCurrentCountryCode);
			if ((special_service_num > 0) && (SvcNum > special_service_num))
			{
				SvcNum = SvcNum - special_service_num;
			}
		}

		if (!TCC_Isdbt_IsSupportDataService())
		{
			data_service_num = UpdateDB_Channel_GetDataServiceCount(gCurrentChannel, gCurrentCountryCode);
			if ((data_service_num > 0) && (SvcNum > data_service_num))
			{
				SvcNum = SvcNum - data_service_num;
			}
		}

		LLOGD("[%s] state[%d] data[%d] special[%d]\n", __func__, state, data_service_num, special_service_num);

		if ((iPrevParseSvcNum != 0) && (iPrevParseSvcNum != SvcNum) && (tcc_manager_demux_get_validOutput() != 0))
		{
			LLOGD("[%s] the service num changed and event notify [%d:%d] valid[%d] data[%d] special[%d] \n", __func__, iPrevParseSvcNum, SvcNum, tcc_manager_demux_get_validOutput(), data_service_num, special_service_num);
			TCCDxBEvent_DetectServiceNumChange();
		}
		else
		{
			LLOGD("[%s] state[%d] prev_parse[%d] parse[%d] first[%d] valid[%d] data[%d] special[%d] \n", __func__, state, iPrevParseSvcNum, SvcNum, iCheckSvcNum, tcc_manager_demux_get_validOutput(), data_service_num, special_service_num);
		}

		iPrevParseSvcNum = SvcNum;
	}
	else
	{
		LLOGD("[%s] state[%d] prev_parse[%d] parse[%d] first[%d] valid[%d] \n", __func__, state, iPrevParseSvcNum, SvcNum, iCheckSvcNum, tcc_manager_demux_get_validOutput());
	}

	return 0;
}

/*PAT version check routine start*/
int tcc_demux_section_decoder_update_pat_in_broadcast(void)
{
	ALOGD("[%s]\n", __func__);
 	tcc_demux_stop_pmt_filtering();
	tcc_demux_stop_pmt_backscan_filtering();
	tcc_manager_demux_backscan_reset();
	gCurrentPMTPID = -1;

	return 0;
}
/*PAT version check routine end*/

/*service_id check routine start*/
void tcc_manager_demux_serviceInfo_Init(void)
{
	ALOGD("[%s]\n", __func__);
	int i;
	iCheckServiceInfo = 0;
	for(i=0;i<MAX_SUPPORT_CURRENT_SERVICE;i++) {
		usCheckServiceID[i] = 0x0;
	}
}

int tcc_manager_demux_serviceInfo_set(unsigned int recServiceId)
{
	ALOGD("[%s] recServiceId=%d\n", __func__, recServiceId);
	int i;
	int state = tcc_manager_demux_get_state();

	if(state != E_DEMUX_STATE_AIRPLAY){
		//ALOGE("[%s] Invalid state[%d]\n", __func__, state);
		return -1;
	}

	/*check the valid serviceID*/
	if(recServiceId > 0xF9BF){
		ALOGE("[%s] Invalid serviceID[%d]\n", __func__, recServiceId);
		return -1;
	}

	for(i=0;i<MAX_SUPPORT_CURRENT_SERVICE;i++){
		//ALOGD("[%s] usCheckServiceID[i]=%d\n", __func__, usCheckServiceID[i]);
    	if(usCheckServiceID[i] == 0x0)
    		break;
    	if(usCheckServiceID[i] == recServiceId){
    		return -1;
    	}
	}

	if(i == MAX_SUPPORT_CURRENT_SERVICE) {
		ALOGE("[%s] MAX_SUPPORT_CURRENT_SERVICE over", __func__);
    }else{
    	/*save air's serviceID*/
		usCheckServiceID[i] = recServiceId;
	}

	return 0;
}

/*
    Noah, 2018-10-11, IM692A-13 / TVK does not recieve at all.

        Precondition
            ISDBT_SINFO_CHECK_SERVICEID is enabled.

        Issue
            1. tcc_manager_audio_serviceID_disable_output(0) and tcc_manager_video_serviceID_disable_display(0) are called.
                -> Because 632 is broadcasting in suspension, so, this is no problem.
            2. setChannel(_withChNum) or searchAndSetChannel is called to change to weak signal(1seg OK, Fseg No) channel.
            3. A/V is NOT output.
                -> NG

        Root Cause
            The omx_alsasink_component_Private->audioskipOutput and pPrivateData->bfSkipRendering of previous channel are used in current channel.
            Refer to the following steps.

            1. In step 1 of the Issue above, omx_alsasink_component_Private->audioskipOutput and pPrivateData->bfSkipRendering are set to TRUE
                because tcc_manager_audio_serviceID_disable_output(0) and tcc_manager_video_serviceID_disable_display(0) are called.
            2. tcc_manager_audio_serviceID_disable_output(1) and tcc_manager_video_serviceID_disable_display(1) have to be called for playing A/V.
                To do so, DxB has to get PAT and then tcc_manager_demux_serviceInfo_comparison has to be called.
                But, DxB can NOT get APT due to weak signal.

        Solution
            tcc_manager_audio_serviceID_disable_output(1) & tcc_manager_video_serviceID_disable_display(1) are called forcedly once as below.

            I think the following conditons should be default values. And '1' value of previous channel should not be used in current channel.
                - omx_alsasink_component_Private->audioskipOutput == 0
                - pPrivateData->bfSkipRendering == 0
            After that -> PAT -> If service ID is the same, A/V will be shown continually. Otherwise A/V will be blocked.

*/
void tcc_manager_demux_init_av_skip_flag(void)
{
    // A/V play
    tcc_manager_demux_set_validOutput(1);
    tcc_manager_audio_serviceID_disable_output(1);
    tcc_manager_video_serviceID_disable_display(1);
}

int tcc_manager_demux_serviceInfo_comparison(unsigned int curServiceId)
{
	int i, find_flag=0 ;
	int state = tcc_manager_demux_get_state();

	if(state != E_DEMUX_STATE_AIRPLAY){
		//ALOGE("[%s] Invalid state[%d]\n", __func__, state);
		return -1;
	}

	if(iCheckServiceInfo == 1)
		return -1;

	/*check the valid serviceID*/
	if(curServiceId > 0xF9BF){
		ALOGE("[%s] Invalid serviceID[%d]\n", __func__, curServiceId);
		return -1;
	}

	/*check the valid air's saved serviceID*/
	if(usCheckServiceID[0] != 0x0){
		for(i=0;i<MAX_SUPPORT_CURRENT_SERVICE;i++){
			//comparison between air's saved serviceID and current playing serviceID.
			if(usCheckServiceID[i] == curServiceId){
				ALOGD("[%s] found service[%d]\n", __func__, usCheckServiceID[i]);
				find_flag = 1;
				break;
			}
		}

		if(find_flag == 0){
			//skip A/V/S play
			tcc_manager_demux_set_validOutput(0);
			tcc_manager_audio_serviceID_disable_output(0);
			tcc_manager_video_serviceID_disable_display(0);
			if(subtitle_fb_type() == 0){
				subtitle_data_enable(0);
				superimpose_data_enable(0);
			}
		}else if(find_flag == 1){
			// A/V play
			tcc_manager_demux_set_validOutput(1);
			tcc_manager_audio_serviceID_disable_output(1);
			tcc_manager_video_serviceID_disable_display(1);
		}

		iCheckServiceInfo = 1;
		tcc_manager_demux_serviceInfo_Init();
	}
	return 0;
}
/*service_id check routine end*/

int tcc_manager_demux_ews_start(int PmtPID, int PmtSubPID)
{
	ISDBT_Init_DescriptorInfo();
	tcc_manager_demux_set_eit_subtitleLangInfo_clear();

	gCurrentPMTPID = PmtPID;
	gCurrentPMTSubPID = PmtSubPID;

	ISDBT_TSPARSE_ServiceList_Init();

	/* ---- backscan ----*/
	tcc_manager_demux_backscan_reset();
	if (PmtPID == 0 && TCC_Isdbt_IsSupportProfileA())
		tcc_manager_demux_backscan_start_scanfullseg();

	tcc_demux_start_nit_filtering();
	tcc_manager_demux_set_section_decoder_state(1);
	return 0;
}

int tcc_manager_demux_ews_clear()
{
	tcc_manager_demux_set_section_decoder_state(0);
	tcc_demux_stop_nit_filtering();
	tcc_demux_stop_pmt_filtering();
	tcc_demux_stop_pat_filtering();

	tcc_demux_stop_pmt_backscan_filtering();
	tcc_manager_demux_backscan_reset();

	gCurrentPMTPID = -1;
	gCurrentPMTSubPID = -1;

	return 0;
}

int tcc_manager_demux_get_stc(int index, unsigned int *hi_data, unsigned int *lo_data)
{
	TCC_DxB_DEMUX_GetSTC(hInterface, hi_data, lo_data, index);
	return 0;
}

int tcc_manager_demux_set_dualchannel(unsigned int index)
{
	TCC_DxB_DEMUX_SetDualchannel(hInterface, 0, index);
	return 0;
}

void tcc_manager_demux_set_subtitle_option(int type, int onoff)
{
	ALOGE("tcc_manager_demux_set_subtitle_option type[%d] onoff[%d]\n", type, onoff);
	if(type == 0){
		g_caption_onoff = onoff;
	}else if(type == 1){
		g_super_onoff = onoff;
	}
}

extern int tcc_manager_demux_get_contentid(unsigned int *contentID, unsigned int *associated_contents_flag)
{
	T_DESC_BXML	DescBXML = { 0, };

	ISDBT_Get_DescriptorInfo(E_DESC_ARIB_BXML_INFO, (void *)&DescBXML);

	*contentID = (unsigned int)(DescBXML.content_id);
	*associated_contents_flag = (unsigned int)(DescBXML.associated_content_flag);
	return 0;
}

/* Noah / 20170621 / Added for IM478A-60 (The request additinal API to start/stop EPG processing)
	uiEpgOnOff
		0 : EPG( EIT ) Off
		1 : EPG( EIT ) On
*/
void tcc_manager_demux_set_EpgOnOff(unsigned int uiEpgOnOff)
{
	g_uiEpgOnOff = uiEpgOnOff;
}

unsigned int tcc_manager_demux_get_EpgOnOff(void)
{
	return g_uiEpgOnOff;
}

/************************************************************************************************
ISDBT INFO SPECIFICATION (refer to globals.h)
*************************************************************************************************
MSB																							  LSB
|31|30|29|28|27|26|25|24|23|22|21|20|19|18|17|16|15|14|13|12|11|10|09|08|07|06|05|04|03|02|01|00|
|  |  |  |  |           |                       |                       |                       |

Bit[31]		: 13Seg
Bit[30]		: 1Seg
Bit[29]		: JAPAN
Bit[28]		: BRAZIL
Bit[27]		: BCAS
Bit[26]		: TRMP
Bit[25:23]	: reserved for ISDBT feature
Bit[22]		: record support
Bit[21]		: book support
Bit[20]		: timeshfit support
Bit[19]		: Dual-decoding support
Bit[18]		: Event Relay support
Bit[17]		: MVTV support
Bit[16]		: check of the service ID of broadcast
Bit[15]		: Channel db Backup ( Cancel a scan in the scanning. channel db backup )
Bit[14]		: special service support
Bit[13]		: data service support
Bit[12]		: skip SDT while scanning channels
Bit[11]		: primary service ON/OFF
Bit[10]		: audio only mode
Bit[09:08]	: Information about log
{
	ISDBT_SINFO_ALL_ON_LOG		= 0,
	ISDBT_SINFO_IWE_LOG			= 1,
	ISDBT_SINFO_E_LOG			= 2,
	ISDBT_SINFO_ALL_OFF_LOG		= 3
}
Bit[07:06]	: Decrypt Info
{
	Decrypt_None				= 0,
	Decrypt_TCC353x				= 1,
	Decrypt_HWDEMUX				= 2,
	Decrypt_ACPU				= 3,
}
Bit[05:00]	: Demod Band Info
{
	Baseband_None				= 0,
	Baseband_TCC351x_CSPI_STS	= 1,
	Baseband_reserved			= 2,
	Baseband_TCC351x_I2C_STS	= 3,
	Baesband_reserved6			= 4,
	Baseband_TCC351x_I2C_SPIMS	= 5, //reserved
	Baseband_reserved			= 6,
	Baseband_reserved			= 7,
	Baseband_TCC353X_CSPI_STS	= 8,
	Baseband_TCC353X_I2C_STS	= 9,
	Baseband_Max				= 63,
}
*/

/* NOTICE
- Below variables are also used in tcc_isdbt_system.c (around line 84).
*/

//information about profile
#define ISDBT_SINFO_PROFILE_A			0x80000000		//b'31 - support full seg
#define ISDBT_SINFO_PROFILE_C			0x40000000		//b'30 -support 1seg

//information about nation
#define ISDBT_SINFO_JAPAN				0x20000000		//b'29 - japan
#define ISDBT_SINFO_BRAZIL				0x10000000		//b'28 - brazil

//information about nation
#define ISDBT_SINFO_BCAS				0x08000000		//b'27 - BCAS
#define ISDBT_SINFO_TRMP				0x04000000		//b'26 - TRMP

//ALSA open
#define ISDBT_SINFO_ALSA_CLOSE			0x00800000		//b'23 - ALSA close

//PVR Function ON/OFF
#define ISDBT_SINFO_Enabling_PVR		0x00400000		//b'22 - Enable PVR

//Special function
#define ISDBT_SINFO_EVENTRELAY			0x00040000		//b'18 - Event Relay
#define ISDBT_SINFO_MVTV				0x00020000		//b'17 - MVTV
#define ISDBT_SINFO_CHECK_SERVICEID		0x00010000		//b'16 - check of the service ID of broadcast
#define ISDBT_SINFO_CHANNEL_DBBACKUP	0x00008000		//b'15 - Channel db Backupt
#define ISDBT_SINFO_SPECIALSERVICE		0x00004000		//b'14 - special service
#define ISDBT_SINFO_DATASERVICE			0x00002000		//b'13 - data service
#define ISDBT_SINFO_SKIP_SDT_INSCAN		0x00001000		//b'12 - skip SDT while scanning channels

#define ISDBT_SINFO_PRIMARYSERVICE		0x00000800		//b'11 - primary service ON/OFF

#define ISDBT_SINFO_AUDIO_ONLY_MODE		0x00000400		//b'10 - DTV Audio only mode

#define ISDBT_DECRYPT_TCC353x			0x00000040
#define ISDBT_DECRYPT_HWDEMUX			0x00000080
#define ISDBT_DECRYPT_ACPU				0x000000C0
#define ISDBT_DECRYPT_MASK				0x000000C0

#define ISDBT_BASEBAND_MASK				0x0000001f

typedef enum
{
	BASEBAND_NONE				= 0,
	BASEBAND_TCC351x_CSPI_STS	= 1,
	BASEBAND_DIBCOM				= 2,
	BASEBAND_TCC351x_I2C_STS	= 3,
	BASEBAND_NMI32x				= 4,
	BASEBAND_TCC351x_I2C_SPIMS	= 5, //reserved
	BASEBAND_MTV818				= 6,
	BASEBAND_TOSHIBA			= 7,
	BASEBAND_TCC353x_CSPI_STS	= 8,
	BASEBAND_TCC353x_I2C_STS	= 9,
	BASEBAND_MAX				= 63,
}ISDBT_BASEBAND_TYPE;

//macros
#define IsdbtSInfo_IsSupportProfileA(X)				(((X) & ISDBT_SINFO_PROFILE_A) ? 1 : 0)
#define IsdbtSInfo_IsSupportProfileC(X)				(((X) & ISDBT_SINFO_PROFILE_C) ? 1 : 0)

#define IsdbtSInfo_IsForJapan(X)					(((X) & ISDBT_SINFO_JAPAN) ? 1 : 0)
#define IsdbtSInfo_IsForBrazil(X)					(((X) & ISDBT_SINFO_BRAZIL) ? 1 : 0)

#define IsdbtSInfo_IsForBCAS(X)						(((X) & ISDBT_SINFO_BCAS) ? 1 : 0)
#define IsdbtSInfo_IsForTRMP(X)						(((X) & ISDBT_SINFO_TRMP) ? 1 : 0)

#define IsdbtSInfo_IsSupportPVR(X)					(((X) & ISDBT_SINFO_Enabling_PVR) ? 1 : 0)

#define IsdbtSInfo_IsSupportEventRelay(X)			((((X) & ISDBT_SINFO_EVENTRELAY) == ISDBT_SINFO_EVENTRELAY) ? 1 : 0)
#define IsdbtSInfo_IsSupportMVTV(X)					((((X) & ISDBT_SINFO_MVTV) == ISDBT_SINFO_MVTV) ? 1 : 0)
#define IsdbtSInfo_IsSupportCheckServiceID(X)		((((X) & ISDBT_SINFO_CHECK_SERVICEID) == ISDBT_SINFO_CHECK_SERVICEID)? 1 : 0)
#define IsdbtSInfo_IsSupportChannelDbBackUp(X)		((((X) & ISDBT_SINFO_CHANNEL_DBBACKUP) == ISDBT_SINFO_CHANNEL_DBBACKUP)? 1 : 0)
#define IsdbtSInfo_IsSupportSpecialService(X)		((((X) & ISDBT_SINFO_SPECIALSERVICE) == ISDBT_SINFO_SPECIALSERVICE) ? 1 : 0)
#define IsdbtSInfo_IsSupportDataService(X)			((((X) & ISDBT_SINFO_DATASERVICE) == ISDBT_SINFO_DATASERVICE) ? 1 : 0)
#define IsdbtSInfo_IsSkipSDTInScan(X)				((((X) & ISDBT_SINFO_SKIP_SDT_INSCAN) == ISDBT_SINFO_SKIP_SDT_INSCAN) ? 1 : 0)

#define IsdbtSInfo_IsSupportPrimaryService(X)		(((X) & ISDBT_SINFO_PRIMARYSERVICE) ? 1 : 0)

#define IsdbtSInfo_IsAudioOnlyMode(X)				((((X) & ISDBT_SINFO_AUDIO_ONLY_MODE) == ISDBT_SINFO_AUDIO_ONLY_MODE) ? 1 : 0)
#define IsdbtSInfo_IsSupportAlsaClose(X)				((((X) & ISDBT_SINFO_ALSA_CLOSE) == ISDBT_SINFO_ALSA_CLOSE) ? 1 : 0)

#define IsdbtSInfo_GetDecryptInfo(X)				((X) & ISDBT_DECRYPT_MASK)
#define IsdbtSInfo_IsDecryptTCC353x(X)				((((X) & ISDBT_DECRYPT_MASK) == ISDBT_DECRYPT_TCC353x) ? 1 : 0)
#define IsdbtSInfo_IsDecryptHWDEMUX(X)				((((X) & ISDBT_DECRYPT_MASK) == ISDBT_DECRYPT_HWDEMUX) ? 1 : 0)
#define IsdbtSInfo_IsDecryptACPU(X)					((((X) & ISDBT_DECRYPT_MASK) == ISDBT_DECRYPT_ACPU) ? 1 : 0)

#define IsdbtSInfo_IsBaseBandTCC351x_CSPI_STS(X)	((((X) & ISDBT_BASEBAND_MASK) == BASEBAND_TCC351x_CSPI_STS) ? 1 : 0)
#define IsdbtSInfo_IsBaseBandTCC351x_I2C_STS(X)		((((X) & ISDBT_BASEBAND_MASK) == BASEBAND_TCC351x_I2C_STS) ? 1 : 0)
#define IsdbtSInfo_IsBaseBandTCC351x_I2C_SPIMS(X)	((((X) & ISDBT_BASEBAND_MASK) == BASEBAND_TCC351x_I2C_SPIMS) ? 1 : 0)
#define IsdbtSInfo_IsBaseBandTCC353x_CSPI_STS(X)	((((X) & ISDBT_BASEBAND_MASK) == BASEBAND_TCC353x_CSPI_STS) ? 1 : 0)
#define IsdbtSInfo_IsBaseBandTCC353x_I2C_STS(X)		((((X) & ISDBT_BASEBAND_MASK) == BASEBAND_TCC353x_I2C_STS) ? 1 : 0)
#define IsdbtSInfo_GetBaseBand(X)					((X) & ISDBT_BASEBAND_MASK)


static unsigned int g_Isdbt_Info_Specification = 0x00000000;

unsigned int TCC_Isdbt_Get_Specific_Info(void)
{
	return g_Isdbt_Info_Specification;
}

void TCC_Isdbt_Specific_Info_Init(unsigned int specific_info)
{
	g_Isdbt_Info_Specification = specific_info;
}

void TCC_Isdbt_SetProfileA(void)
{
	unsigned int info = g_Isdbt_Info_Specification;

	info &= ~(ISDBT_SINFO_PROFILE_A|ISDBT_SINFO_PROFILE_C);
	info |= ISDBT_SINFO_PROFILE_A;

	/* we assumed that country code is never changed after first execution. */
	TCC_Isdbt_Specific_Info_Init(info);
}

void TCC_Isdbt_SetProfileC(void)
{
	unsigned int info = g_Isdbt_Info_Specification;

	info &= ~(ISDBT_SINFO_PROFILE_A|ISDBT_SINFO_PROFILE_C);
	info |= ISDBT_SINFO_PROFILE_C;

	/* we assumed that country code is never changed after first execution. */
	TCC_Isdbt_Specific_Info_Init(info);
}

int TCC_Isdbt_IsSupportProfileA(void)
{
	return IsdbtSInfo_IsSupportProfileA(g_Isdbt_Info_Specification);
}

int TCC_Isdbt_IsSupportProfileC(void)
{
	return IsdbtSInfo_IsSupportProfileC(g_Isdbt_Info_Specification);
}

int TCC_Isdbt_IsSupportJapan(void)
{
	return IsdbtSInfo_IsForJapan(g_Isdbt_Info_Specification);
}

int TCC_Isdbt_IsSupportBrazil(void)
{
	return IsdbtSInfo_IsForBrazil(g_Isdbt_Info_Specification);
}

int TCC_Isdbt_IsSupportBCAS(void)
{
	return (TCC_Isdbt_IsSupportJapan() && TCC_Isdbt_IsSupportProfileA() && IsdbtSInfo_IsForBCAS(g_Isdbt_Info_Specification));
}

int TCC_Isdbt_IsSupportTRMP(void)
{
	return (TCC_Isdbt_IsSupportJapan() && TCC_Isdbt_IsSupportProfileA() && IsdbtSInfo_IsForTRMP(g_Isdbt_Info_Specification));
}

int TCC_Isdbt_IsSupportCAS(void)
{
	return (TCC_Isdbt_IsSupportBCAS() || TCC_Isdbt_IsSupportTRMP());
}

int TCC_Isdbt_IsSupportPVR(void)
{
	return IsdbtSInfo_IsSupportPVR(g_Isdbt_Info_Specification);
}

int TCC_Isdbt_GetBaseBand (void)
{
	return IsdbtSInfo_GetBaseBand (g_Isdbt_Info_Specification);
}

int TCC_Isdbt_IsSupportTunerTCC351x_CSPI_STS(void)
{
	return IsdbtSInfo_IsBaseBandTCC351x_CSPI_STS(g_Isdbt_Info_Specification);
}

int TCC_Isdbt_IsSupportTunerTCC351x_I2C_STS(void)
{
	return IsdbtSInfo_IsBaseBandTCC351x_I2C_STS(g_Isdbt_Info_Specification);
}

int TCC_Isdbt_IsSupportTunerTCC351x_I2C_SPIMS(void)
{
	return IsdbtSInfo_IsBaseBandTCC351x_I2C_SPIMS(g_Isdbt_Info_Specification);
}

int TCC_Isdbt_IsSuppportTunerTCC353x_CSPI_STS(void)
{
	return IsdbtSInfo_IsBaseBandTCC353x_CSPI_STS(g_Isdbt_Info_Specification);
}

int TCC_Isdbt_IsSuppportTunerTCC353x_I2C_STS(void)
{
	return IsdbtSInfo_IsBaseBandTCC353x_I2C_STS(g_Isdbt_Info_Specification);
}

int TCC_Isdbt_GetDecryptInfo(void)
{
	return IsdbtSInfo_GetDecryptInfo(g_Isdbt_Info_Specification);
}

int TCC_Isdbt_IsSupportDecryptTCC353x(void)
{
	return IsdbtSInfo_IsDecryptTCC353x(g_Isdbt_Info_Specification);
}

int TCC_Isdbt_IsSupportDecryptHWDEMUX(void)
{
	return IsdbtSInfo_IsDecryptHWDEMUX(g_Isdbt_Info_Specification);
}

int TCC_Isdbt_IsSupportDecryptACPU(void)
{
	return IsdbtSInfo_IsDecryptACPU(g_Isdbt_Info_Specification);
}

int TCC_Isdbt_IsSupportEventRelay(void)
{
	return IsdbtSInfo_IsSupportEventRelay(g_Isdbt_Info_Specification);
}

int TCC_Isdbt_IsSupportMVTV(void)
{
	return IsdbtSInfo_IsSupportMVTV(g_Isdbt_Info_Specification);
}

int TCC_Isdbt_IsSupportCheckServiceID(void)
{
	return IsdbtSInfo_IsSupportCheckServiceID(g_Isdbt_Info_Specification);
}

int TCC_Isdbt_IsSupportChannelDbBackUp(void)
{
	return IsdbtSInfo_IsSupportChannelDbBackUp(g_Isdbt_Info_Specification);
}

int TCC_Isdbt_IsSupportSpecialService(void)
{
	return IsdbtSInfo_IsSupportSpecialService(g_Isdbt_Info_Specification);
}

int TCC_Isdbt_IsSupportDataService(void)
{
	return IsdbtSInfo_IsSupportDataService(g_Isdbt_Info_Specification);
}

int TCC_Isdbt_IsSkipSDTInScan(void)
{
	return IsdbtSInfo_IsSkipSDTInScan(g_Isdbt_Info_Specification);
}

int TCC_Isdbt_IsAudioOnlyMode(void)
{
	return IsdbtSInfo_IsAudioOnlyMode(g_Isdbt_Info_Specification);
}

int TCC_Isdbt_IsSupportAlsaClose(void)
{
	return IsdbtSInfo_IsSupportAlsaClose(g_Isdbt_Info_Specification);
}

/* Noah / 20170522 / Added for IM478A-31 (Primary Service)
*/
int TCC_Isdbt_IsSupportPrimaryService(void)
{
	return IsdbtSInfo_IsSupportPrimaryService(g_Isdbt_Info_Specification);
}

void TCC_Isdbt_Clear_AudioOnlyMode(void)
{
	g_Isdbt_Info_Specification = g_Isdbt_Info_Specification & 0xFFFFFBFF;
}
