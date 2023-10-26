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
#if defined(HAVE_LINUX_PLATFORM)
#define LOG_TAG "TSPARSE_ISDBT"
#include <utils/Log.h>
#endif

#define SIMGMT_DEBUG	//ALOGE

#include <cutils/properties.h>
#include <stdlib.h>

#include "TsParse_ISDBT.h"

#include "ISDBT.h"
#include "ISDBT_EPG.h"
#include "ISDBT_Common.h"
#include "TsParse_ISDBT_DBLayer.h"
#include "tcc_dxb_timecheck.h"
#include "tcc_isdbt_manager_cas.h"
#include "tcc_isdbt_manager_demux.h"
#include "TsParse_ISDBT_SI.h"

/****************************************************************************
DEFINITION
****************************************************************************/
#define PARSE_DBG	//ALOGE

#define PID_EIT				0x0012
#define PID_L_EIT				0x0027
#define PID_M_EIT				0x0026
#define PID_H_EIT				(PID_EIT)

#define CAS_EFFECTIVE_ID	0x0005
#define RMP_EFFECTIVE_ID	0x000E

#define SI_TBL_MAX			0xFF
#define EIT_TBL_MAX			0x20
#define EIT_SEC_MAX			0xFF

#define TSMIN(a,b)  (((a) < (b)) ? (a) : (b))

typedef struct
{
	unsigned short 	ServiceID;
	unsigned short	AreaCode;
} AREA_UNIT_INFO;

typedef struct
{
	AREA_UNIT_INFO 	PrevAreaInfo;
	AREA_UNIT_INFO	RegisteredAreaInfo;
	AREA_UNIT_INFO	CurrAreaInfo;
	unsigned char		fAreaChanged;
	unsigned char		fInitialized;
} TS_RECEIPTION_AREA_INFO;

typedef struct
{
	unsigned char   Version;
	unsigned char	TableID;
	int	RequestID;
	unsigned int    SectionLength;
	unsigned int	TSID;
} SI_INFO;

typedef struct
{
	unsigned char   Hour;
	unsigned char   Min;
	unsigned char   Sec;
} TOT_INFO;

typedef struct
{
	unsigned short 	ServiceID;
	unsigned char	Version[EIT_TBL_MAX];
	unsigned char	ReceivingVersion[EIT_TBL_MAX];
	unsigned char   Received[EIT_TBL_MAX][EIT_SEC_MAX+1];
	void *          pNext;
} SEC_INFO;

typedef struct
{
	SEC_INFO *      pSecInfo;
} EIT_INFO;

/* CDT info */
#define	CDT_LOGO_SECTION_MAX	5	/* logo_type 0~5 */
typedef struct {
	unsigned short	download_data_id;
	unsigned char	section_version[CDT_LOGO_SECTION_MAX+1];
	unsigned char	last_section;
	void *pNext;
} CDT_SEC_INFO;
typedef struct {
	CDT_SEC_INFO *pSec;
} CDT_INFO;

/****************************************************************************
DEFINITION OF EXTERNAL VARIABLES
****************************************************************************/
extern int gCurrentChannel, gCurrentFrequency, gCurrentCountryCode;
extern unsigned int gPlayingServiceID, gSetServiceID;
/****************************************************************************
DEFINITION OF EXTERNAL FUNCTIONS
****************************************************************************/
int free_eid(EMERGENCY_INFO_DATA **ppData);

/****************************************************************************
DEFINITION OF GLOVAL VARIABLES
****************************************************************************/
T_ISDBT_SERVICE_STRUCT g_stISDBT_ServiceList;
static unsigned char fCurrEventRate = FALSE;
static void *g_pSI_Table;

static TS_RECEIPTION_AREA_INFO	stTSAreaInfo = { {0,0}, {0,0}, {0,0}, 0, 0};

static T_DESC_SERVICE_LIST g_stDescSVCLIST = { E_TABLE_ID_MAX, 0, {0, 0}, NULL };
static T_DESC_DTCP g_stDescDTCP = { E_TABLE_ID_MAX, 0, 0, 0, 0, 0, 0, 0};
static T_DESC_PR g_stDescPR[2] = {{E_TABLE_ID_MAX, 0xff}, {E_TABLE_ID_MAX, 0xff}};
static T_DESC_CA g_stDescPMTCA = { E_TABLE_ID_MAX, 0, 0, 0};
static T_DESC_CA g_stDescCATCA = { E_TABLE_ID_MAX, 0, 0, 0};
static T_DESC_CA_SERVICE g_stDescCATCAService = { E_TABLE_ID_MAX, 0, 0, 0, 0, 0, 0 };
static T_DESC_AC g_stDescPMTAC = { E_TABLE_ID_MAX, 0, 0, 0};
static T_DESC_AC g_stDescCATAC = { E_TABLE_ID_MAX, 0, 0, 0};
static T_DESC_EBD g_stDescEBD = {0, 0, 0, {0, }, 0, {0, }, {0, }};
static T_DESC_TDSD g_stDescTDSD = {0, 0, 0, 0, {0, }};
static T_DESC_LTD g_stDescLTD = { 0, 0, 0, 0, 0, NULL};
static T_DESC_BXML g_stDescBXML = { 0, 0, 0, 0 };

static SI_INFO g_si_info[SI_TBL_MAX];
static TOT_INFO g_tot_info;
static EIT_INFO g_eit_info;
static CDT_INFO	g_cdt_info;	/* to check CDT version */
static T_EID_TYPE g_eid_type = {NULL};
T_ISDBT_LOGO_INFO	stISDBTLogoInfo;
static unsigned char g_ts_name_char[MAX_TS_NAME_LEN+1];
static unsigned short uiTOTMJD = -1;

static int eitcapcnt;
static struct
{
	unsigned short usNetworkID;
	struct
	{
		unsigned short	usServiceID;
		unsigned char	ucServiceType;

		unsigned char	ucPMTDescBitMap;
		/*PMTDescBitMap
		* bit 	 Descriptor
		*  1		: Digital Copy Control
		*  2		: Content Availability
		*  else		: reserve
		*/
		T_DESC_DCC		stDescDCC;
		T_DESC_CONTA	stDescCONTA;

		unsigned char	ucSDTDescBitMap;
		/*SDTDescBitMap
		* bit 	 Descriptor
		*  1		: Digital Copy Control
		*  2		: Content Availability
		*  else		: reserve
		*/
		unsigned char	ucEITDescBitMap;
		/*EITDescBitMap
		* bit 	 Descriptor
		*  1		: Digital Copy Control
		*  2		: Content Availability
		*  4		: Event Group
		*  8		: Component Group
		*  else		: reserve
		*/
		T_ISDBT_EIT_EVENT_GROUP	stEGD;
		T_DESC_CGD				stDescCGD;
	}services[MAX_SUPPORT_CURRENT_SERVICE];
}gCurrentDescInfo;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)

// for g_stDescSVCLIST, g_stDescDTCP, g_stDescPR, g_stDescPMTCA, g_stDescCATCA, g_stDescCATCAService,
//     g_stDescPMTAC, g_stDescCATAC, g_stDescEBD, g_stDescTDSD, g_stDescLTD, g_stDescBXML
static pthread_mutex_t g_DescriptorMutex;

// for g_eit_info
static pthread_mutex_t g_EitInfoMutex;

// for g_cdt_info
static pthread_mutex_t g_CdtInfoMutex;

// Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/****************************************************************************
DEFINITION OF LOCAL FUNCTIONS
****************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
  * made by Noah / 20180705 / from 'ISDBT_TSPARSE_DescriptorLock_Init' to 'ISDBT_TSPARSE_CdtInfoLock_Off'
  *
  * description
  *   Refer to J&K / IM478A-51 (Memory Access Timing)
*/

static void ISDBT_TSPARSE_DescriptorLock_Init(void)
{
	pthread_mutex_init(&g_DescriptorMutex, NULL);
}
static void ISDBT_TSPARSE_DescriptorLock_Deinit(void)
{
	pthread_mutex_destroy(&g_DescriptorMutex);
}
static void ISDBT_TSPARSE_DescriptorLock_On(void)
{
	pthread_mutex_lock(&g_DescriptorMutex);
}
static void ISDBT_TSPARSE_DescriptorLock_Off(void)
{
	pthread_mutex_unlock(&g_DescriptorMutex);
}

static void ISDBT_TSPARSE_EitInfoLock_Init(void)
{
	pthread_mutex_init(&g_EitInfoMutex, NULL);
}
static void ISDBT_TSPARSE_EitInfoLock_Deinit(void)
{
	pthread_mutex_destroy(&g_EitInfoMutex);
}
static void ISDBT_TSPARSE_EitInfoLock_On(void)
{
	pthread_mutex_lock(&g_EitInfoMutex);
}
static void ISDBT_TSPARSE_EitInfoLock_Off(void)
{
	pthread_mutex_unlock(&g_EitInfoMutex);
}

static void ISDBT_TSPARSE_CdtInfoLock_Init(void)
{
	pthread_mutex_init(&g_CdtInfoMutex, NULL);
}
static void ISDBT_TSPARSE_CdtInfoLock_Deinit(void)
{
	pthread_mutex_destroy(&g_CdtInfoMutex);
}
static void ISDBT_TSPARSE_CdtInfoLock_On(void)
{
	pthread_mutex_lock(&g_CdtInfoMutex);
}
static void ISDBT_TSPARSE_CdtInfoLock_Off(void)
{
	pthread_mutex_unlock(&g_CdtInfoMutex);
}

/*
  * made by Noah / 20180705 / from 'ISDBT_TSPARSE_DescriptorLock_Init' to 'ISDBT_TSPARSE_DescriptorLock_Off'
  *
  * description
  *   Refer to J&K / IM478A-51 (Memory Access Timing)
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////

static void SetTOTTime(unsigned char ucHour, unsigned char ucMin, unsigned char ucSec)
{
	g_tot_info.Hour = ucHour;
	g_tot_info.Min = ucMin;
	g_tot_info.Sec = ucSec;
}

static SEC_INFO* GetEITSection(unsigned short service_id)
{
	SEC_INFO *prev, *cur;

    ISDBT_TSPARSE_EitInfoLock_On();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)

	cur = g_eit_info.pSecInfo;
	while(cur != NULL)
	{
		if (cur->ServiceID == service_id)
		{
            ISDBT_TSPARSE_EitInfoLock_Off();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)
			return cur;
		}

		prev = cur;
		cur = (SEC_INFO*)cur->pNext;
	}

	cur = tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(SEC_INFO));
	if (cur != NULL)
	{
		if (g_eit_info.pSecInfo == NULL)
		{
			g_eit_info.pSecInfo = cur;
		}
		else
		{
			prev->pNext = cur;
		}

		cur->ServiceID = service_id;
		memset(cur->Version , 0xFF, EIT_TBL_MAX);
		memset(cur->ReceivingVersion , 0xFF, EIT_TBL_MAX);
		memset(cur->Received, 0, EIT_TBL_MAX*EIT_SEC_MAX);
		cur->pNext = NULL;

        ISDBT_TSPARSE_EitInfoLock_Off();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)
		return cur;
	}

	ALOGE("[ISDBT_PARSER][%d] malloc error!\n", __LINE__);

    ISDBT_TSPARSE_EitInfoLock_Off();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)
	return NULL;
}

static void ClearEITSection(void)
{
	SEC_INFO *cur, *next;

    ISDBT_TSPARSE_EitInfoLock_On();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)

	cur = g_eit_info.pSecInfo;
	while(cur != NULL)
	{
		next = cur->pNext;
		tcc_mw_free(__FUNCTION__, __LINE__, cur);
		cur = next;
	}

	g_eit_info.pSecInfo = NULL;

    ISDBT_TSPARSE_EitInfoLock_Off();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)
	return;
}

static int isEITUpdateDone()
{
	SEC_INFO *pstSecInfo;
	int i, j;
	int iEpgScanCount = tcc_manager_demux_is_epgscan();

    ISDBT_TSPARSE_EitInfoLock_On();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)

	if(g_eit_info.pSecInfo == NULL)
	{
	    ISDBT_TSPARSE_EitInfoLock_Off();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)
		return 0;
	}

	pstSecInfo = g_eit_info.pSecInfo;
	while(pstSecInfo != NULL)
	{
		for(i=0;i<EIT_TBL_MAX;i++)
		{
			for(j=0;j<EIT_SEC_MAX+1;j++)
			{
				if(pstSecInfo->Received[i][j] == 1)
				{
					if(pstSecInfo->Version[i] != 0xFF)
					{
						iEpgScanCount--;
						break;
					}
					else
					{
					    ISDBT_TSPARSE_EitInfoLock_Off();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)
						return 0;
					}
				}
			}
		}
		pstSecInfo = pstSecInfo->pNext;
	}

    ISDBT_TSPARSE_EitInfoLock_Off();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)

	if(iEpgScanCount > 1)
		return 0;
	else
		return 1;
}

static void SetEITVersionNumber(int channel_num, unsigned short service_id, unsigned char table_id, int sec_num, int last_sec_num, int seg_last_sec_num, unsigned char ver)
{
	SEC_INFO *sec_info;
	unsigned char eit_table_id=0;
	int seg_start_sec_num=0;
	int i=0, j=0;

	if((sec_num < 0) || ( sec_num > EIT_SEC_MAX)){
		ALOGE("[%s] Invalid sec_num(%d)\n", __func__, sec_num);
		return;
	}

	if((last_sec_num < 0) || ( last_sec_num > EIT_SEC_MAX)){
		ALOGE("[%s] Invalid last_sec_num(%d)\n", __func__, last_sec_num);
		return;
	}

	sec_info = GetEITSection(service_id);
	if (sec_info == NULL) {
		ALOGE("[ISDBT_PARSER][%d] SEC_INFO is NULL!\n", __LINE__);
		return;
	}

	eit_table_id = table_id - EIT_PF_A_ID;

	if (sec_info->Version[eit_table_id] == ver) {
		ALOGE("[ISDBT_PARSER][%d] SEC_INFO is already updated!!\n", __LINE__);
		return;
	}

	for(i=0, j=0; i<=last_sec_num; i++)
	{
		if (sec_info->Received[eit_table_id][i]) {
			j++;
			break;
		}
	}

	if ((j == 0) || (sec_info->ReceivingVersion[eit_table_id] != ver)) {
		if(j != 0) {
			memset(&sec_info->Received[eit_table_id][0], 0, EIT_SEC_MAX);
		}
		sec_info->ReceivingVersion[eit_table_id] = ver;

		if (table_id == EIT_PF_A_ID) {
			DeleteDB_EPG_PF_Table(channel_num, service_id, table_id);
		} else if ((table_id >= EIT_SA_0_ID) && (table_id <= EIT_SA_7_ID)) {
			DeleteDB_EPG_Schedule_Table(channel_num, service_id, table_id, sec_num, ver);
		}
	}

	//set the section flag
	sec_info->Received[eit_table_id][sec_num] = 1;


	//Check for schedule.
	if ((table_id >= EIT_SA_0_ID) && (table_id <= EIT_SA_F_ID)) {
		if (sec_num != 0) {
			seg_start_sec_num = (sec_num / 8) * 8;
		}

		for(i=seg_start_sec_num, j=0; i<=seg_last_sec_num; i++)
		{
			if (sec_info->Received[eit_table_id][i])	{
				j++;
			}
		}

		if (j > (seg_last_sec_num - seg_start_sec_num))	{
			memset(&(sec_info->Received[eit_table_id][seg_start_sec_num]), 1, 8);
		}
	}

	for(i=0, j=0; i<=last_sec_num; i++)
	{
		if (sec_info->Received[eit_table_id][i]) {
			j++;
		}
	}

	//If this is today information, EPG section of previous time is not transmitted.
	if ((table_id == EIT_SA_0_ID) || (table_id == EIT_SA_8_ID)) {
		j += (g_tot_info.Hour / 3) * 8;
	}

	if (j > last_sec_num) {
		sec_info->Version[eit_table_id] = ver;
		sec_info->ReceivingVersion[eit_table_id] = ver;
		if(tcc_manager_demux_is_epgscan() && isEITUpdateDone()) {
			tcc_manager_demux_epgscan_done();
		}
		memset(&sec_info->Received[eit_table_id][0], 0, EIT_SEC_MAX);
	}

	return;
}

static char GetEITVersionNumber(unsigned short service_id, unsigned char table_id)
{
	SEC_INFO *sec_info;
	unsigned char eit_table_id=0;

	sec_info = GetEITSection(service_id);
	if (sec_info == NULL) {
		ALOGE("[ISDBT_PARSER][%d] SEC_INFO is NULL!\n", __LINE__);
		return 0xFF;
	}

	eit_table_id = table_id - EIT_PF_A_ID;

	return sec_info->Version[eit_table_id];
}

static void SendEITUpdateEvent(unsigned short service_id, unsigned char table_id, unsigned char ver)
{
	if(GetEITVersionNumber(service_id, table_id) == ver)
	{
		tcc_demux_section_decoder_eit_update(service_id, table_id);
	}
}

static void SendSubtitleLangInfo(unsigned int NumofLang, unsigned int firstLangcode, unsigned int secondLangcode, unsigned int ServiceID, int index)
{
	tcc_manager_demux_set_eit_subtitleLangInfo(NumofLang, firstLangcode, secondLangcode, ServiceID, index);
}

/* ----- CDT start ----- */
static CDT_SEC_INFO* GetCDTSection (unsigned short download_data_id)
{
	CDT_SEC_INFO *prev, *cur;

    ISDBT_TSPARSE_CdtInfoLock_On();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)

	cur = g_cdt_info.pSec;
	while (cur != NULL)
	{
		if (cur->download_data_id == download_data_id)
		{
            ISDBT_TSPARSE_CdtInfoLock_Off();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)
			return cur;
		}

		prev = cur;
		cur = (CDT_SEC_INFO*)cur->pNext;
	}
	cur = tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(CDT_SEC_INFO));
	if (cur != NULL)
    {
		if (g_cdt_info.pSec == NULL)
            g_cdt_info.pSec = cur;
		else
            prev->pNext = cur;

		cur->download_data_id = download_data_id;
		memset(cur->section_version, 0xFF, CDT_LOGO_SECTION_MAX+1);
		cur->last_section = 0xFF;
		cur->pNext = NULL;
	}

    ISDBT_TSPARSE_CdtInfoLock_Off();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)
	return cur;
}
static unsigned char GetCDTVersion (unsigned short download_data_id, unsigned char section)
{
	CDT_SEC_INFO *sec;
	if (section > CDT_LOGO_SECTION_MAX)
        return 0xFF;

	sec = GetCDTSection (download_data_id);
	if (sec != NULL)
	{
		return sec->section_version[section];
	}
	return 0xFF;
}
static int SetCDTVersion (unsigned short download_data_id, unsigned char section, unsigned char last_section, unsigned char version)
{
	CDT_SEC_INFO *sec;

	if (section > CDT_LOGO_SECTION_MAX || last_section > CDT_LOGO_SECTION_MAX)
	    return -1;

	if ((version != 0xFF) && (version > 0x1F))
	    return -1;

	sec = GetCDTSection (download_data_id);
	if (sec != NULL)
	{
		if (sec->last_section == 0xFF)
		    sec->last_section = last_section;

		if (sec->section_version[section] != version)
		{
			sec->section_version[section] = version;
			return 0;
		}

		return -3;	/* already received */
	}
	else
		return -2;
}
static void ClearCDTVersion (void)
{
	CDT_SEC_INFO *cur, *next;

    ISDBT_TSPARSE_CdtInfoLock_On();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)

	cur = g_cdt_info.pSec;
	while (cur != NULL)
	{
		next = cur->pNext;
		tcc_mw_free(__FUNCTION__, __LINE__, cur);
		cur = next;
	}
	g_cdt_info.pSec = NULL;

    ISDBT_TSPARSE_CdtInfoLock_Off();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)
}
/* ----- CDT end -----*/

static void SetSIVersionNumber(int request_id, unsigned char table_id, unsigned char ver)
{
	int	i;

	if((table_id == PAT_ID) || (table_id == PMT_ID) || (table_id == SDT_A_ID) || (table_id == NIT_A_ID) || \
		(table_id == ECM_0_ID) || (table_id == ECM_1_ID) || (table_id == EMM_0_ID) || (table_id == EMM_1_ID) || (table_id == CDT_ID)) {
		if(ver <= 0x1f) {
			for (i=0;i < SI_TBL_MAX;i++)
			{
				if ((g_si_info[i].RequestID == -1) || (g_si_info[i].RequestID == request_id)) {
					g_si_info[i].RequestID	= request_id;
					g_si_info[i].TableID	= table_id;
					g_si_info[i].Version	= ver;
					break;
				}
			}
		}
	} else if ((table_id == EIT_PF_A_ID) || ((table_id >= EIT_SA_0_ID) && (table_id <= EIT_SA_F_ID))) {
		//nothing
	}
}

static unsigned char GetSIVersionNumber(int request_id, unsigned char table_id)
{
	unsigned char ver=0xff;
	int	i;

	if((table_id == PAT_ID) || (table_id == PMT_ID) || (table_id == SDT_A_ID) || (table_id == NIT_A_ID) || \
		(table_id == ECM_0_ID) || (table_id == ECM_1_ID) || (table_id == EMM_0_ID) || (table_id == EMM_1_ID) || (table_id == CDT_ID)) {
		for (i = 0; i < SI_TBL_MAX; i++)
		{
			if (g_si_info[i].RequestID == request_id) {
				ver	= g_si_info[i].Version;
				break;
			}
		}
	} else if ((table_id == EIT_PF_A_ID) || ((table_id >= EIT_SA_0_ID) && (table_id <= EIT_SA_F_ID))) {
		//nothing
	}

	return ver;
}

static void SetSISectionLength(int request_id, unsigned char table_id, unsigned int section_length)
{
	int i;

	if((table_id == PAT_ID || table_id == NIT_A_ID || table_id == PMT_ID)) {
		if(section_length <= 0xfff) {
			for (i=0;i < SI_TBL_MAX;i++)
			{
				if ((g_si_info[i].RequestID == -1) || (g_si_info[i].RequestID == request_id)) {
					g_si_info[i].RequestID	= request_id;
					g_si_info[i].TableID	= table_id;
					g_si_info[i].SectionLength = section_length;
					break;
				}
			}
		}
	}
}

static unsigned int GetSISectionLength(int request_id, unsigned char table_id)
{
	unsigned int section_length=0xfff;
	int i;

	if(table_id == PAT_ID || table_id == NIT_A_ID || table_id == PMT_ID) {
		for (i = 0; i < SI_TBL_MAX; i++)
		{
			if (g_si_info[i].RequestID == request_id) {
				section_length = g_si_info[i].SectionLength;
				break;
			}
		}
	}

	return section_length;
}

static void SetSITSID(int request_id, unsigned char table_id, unsigned int ts_id)
{
	int i;

	if((table_id == PAT_ID) || (table_id == NIT_A_ID) || (table_id == PMT_ID)) {
		if(ts_id <= 0xffff) {
			for (i=0;i < SI_TBL_MAX;i++)
			{
				if ((g_si_info[i].RequestID == -1) || (g_si_info[i].RequestID == request_id)) {
					g_si_info[i].RequestID	= request_id;
					g_si_info[i].TableID	= table_id;
					g_si_info[i].TSID = ts_id;
					break;
				}
			}
		}
	}
}

static unsigned int GetSITSID(int request_id, unsigned char table_id)
{
	unsigned int ts_id=0xffff;
	int i;

	if(table_id == PAT_ID || table_id == NIT_A_ID || table_id == PMT_ID) {
		for (i = 0; i < SI_TBL_MAX; i++)
		{
			if (g_si_info[i].RequestID == request_id) {
				ts_id = g_si_info[i].TSID;
				break;
			}
		}
	}

	return ts_id;
}

void ResetSIVersionNumber(void)
{
	int i=0;

	for(i=0; i<SI_TBL_MAX; i++)
	{
		g_si_info[i].RequestID	= -1;
		g_si_info[i].TableID	= 0xFF;
		g_si_info[i].Version	= 0xFF;
		g_si_info[i].SectionLength = 0xFFF;
		g_si_info[i].TSID = 0xFFFF;
	}

	ClearEITSection();
	ClearCDTVersion();
}

void ISDBT_SetTOTMJD(unsigned short uiMJD)
{
	uiTOTMJD = uiMJD;
}

int ISDBT_GetTOTMJD(void)
{
	return uiTOTMJD;
}

/*****************************************************************************
*	ROUTINE : ISDBT_TSPARSE_GetRegisteredAreaCode
*	DESCRIPTION :  Get the area code being used currently.
*	COMMENTS :
*	INPUT : void
* 	OUTPUT : unsigned short - return type
* 	RETURNS :
*****************************************************************************/
unsigned short ISDBT_TSPARSE_GetRegisteredAreaCode(void)
{
	return stTSAreaInfo.RegisteredAreaInfo.AreaCode;
}

/*****************************************************************************
*	ROUTINE : ISDBT_TSPARSE_SetRegisteredAreaCode
*	DESCRIPTION :  Set the area code being used currently.
*	COMMENTS :
*	INPUT : unsigned short - areaCode
* 	OUTPUT : void
* 	RETURNS :
*****************************************************************************/
void ISDBT_TSPARSE_SetRegisteredAreaCode(unsigned short areaCode)
{
	stTSAreaInfo.RegisteredAreaInfo.AreaCode = areaCode;
}

/*****************************************************************************
*	ROUTINE : ISDBT_TSPARSE_GetRegisteredServiceID
*	DESCRIPTION :  Get the service ID being used currently.
*	COMMENTS :
*	INPUT : void
* 	OUTPUT : unsigned short - return type
* 	RETURNS :
*****************************************************************************/
unsigned short ISDBT_TSPARSE_GetRegisteredServiceID(void)
{
	return stTSAreaInfo.RegisteredAreaInfo.ServiceID;
}

/*****************************************************************************
*	ROUTINE : ISDBT_TSPARSE_SetRegisteredServiceID
*	DESCRIPTION :  Set the service ID being used currently.
*	COMMENTS :
*	INPUT : unsigned short - serviceID
* 	OUTPUT : void
* 	RETURNS :
*****************************************************************************/
void ISDBT_TSPARSE_SetRegisteredServiceID(unsigned short serviceID)
{
	stTSAreaInfo.RegisteredAreaInfo.ServiceID = serviceID;
}

/*****************************************************************************
*	ROUTINE : ISDBT_TSPARSE_GetCurrAreaCode
*	DESCRIPTION :  Get the area code being changed.
*	COMMENTS :
*	INPUT : void
* 	OUTPUT : unsigned short - return type
* 	RETURNS :
*****************************************************************************/
unsigned short ISDBT_TSPARSE_GetCurrAreaCode(void)
{
	return stTSAreaInfo.CurrAreaInfo.AreaCode;
}

/*****************************************************************************
*	ROUTINE : ISDBT_TSPARSE_SetCurrAreaCode
*	DESCRIPTION :  Set the area code being changed.
*	COMMENTS :
*	INPUT : unsigned short - areaCode
* 	OUTPUT : void - return type
* 	RETURNS :
*****************************************************************************/
void ISDBT_TSPARSE_SetCurrAreaCode(unsigned short areaCode)
{
	stTSAreaInfo.CurrAreaInfo.AreaCode = areaCode;
}

/*****************************************************************************
*	ROUTINE : ISDBT_TSPARSE_GetCurrServiceID
*	DESCRIPTION :  Get the service ID being changed.
*	COMMENTS :
*	INPUT : void
* 	OUTPUT : unsigned short - return type
* 	RETURNS :
*****************************************************************************/
unsigned short ISDBT_TSPARSE_GetCurrServiceID(void)
{
	return stTSAreaInfo.CurrAreaInfo.ServiceID;
}

/*****************************************************************************
*	ROUTINE : ISDBT_TSPARSE_SetCurrServiceID
*	DESCRIPTION :  Set the service ID being changed.
*	COMMENTS :
*	INPUT : unsigned short - serviceID
* 	OUTPUT : void - return type
* 	RETURNS :
*****************************************************************************/
void ISDBT_TSPARSE_SetCurrServiceID(unsigned short serviceID)
{
	stTSAreaInfo.CurrAreaInfo.ServiceID = serviceID;
}

/*****************************************************************************
*	ROUTINE : ISDBT_TSPARSE_IsAreaCodeRegistered
*	DESCRIPTION :  Check if the area code is registered.
*	COMMENTS : Area code is 0 right only after the channel change
*	INPUT : void
* 	OUTPUT : unsigned char - return type
* 	RETURNS :
*****************************************************************************/
unsigned char ISDBT_TSPARSE_IsAreaCodeRegistered(void)
{
	return (stTSAreaInfo.RegisteredAreaInfo.AreaCode == 0) ? FALSE : TRUE;
}

/*****************************************************************************
*	ROUTINE : ISDBT_TSPARSE_SetAreaChangeStatus
*	DESCRIPTION :  Set the status of area-change from the current TS
*	COMMENTS :
*	INPUT : unsigned char - flag
* 	OUTPUT : void- return type
* 	RETURNS :
*****************************************************************************/
void ISDBT_TSPARSE_SetAreaChangeStatus(unsigned char flag)
{
	stTSAreaInfo.fAreaChanged = flag;
}

/*****************************************************************************
*	ROUTINE : ISDBT_TSPARSE_GetPrevAreaCode
*	DESCRIPTION :  Get the area code being used previously.
*	COMMENTS :
*	INPUT : void
* 	OUTPUT : unsigned short - return type
* 	RETURNS :
*****************************************************************************/
unsigned short ISDBT_TSPARSE_GetPrevAreaCode(void)
{
	return stTSAreaInfo.PrevAreaInfo.AreaCode;
}

/*****************************************************************************
*	ROUTINE : ISDBT_TSPARSE_SetPrevAreaCode
*	DESCRIPTION :  Set the area code being used previously.
*	COMMENTS :
*	INPUT : unsigned short - areaCode
* 	OUTPUT : void
* 	RETURNS :
*****************************************************************************/
void ISDBT_TSPARSE_SetPrevAreaCode(unsigned short areaCode)
{
	stTSAreaInfo.PrevAreaInfo.AreaCode = areaCode;
}

/*****************************************************************************
*	ROUTINE : ISDBT_TSPARSE_IsServiceIDRegistered
*	DESCRIPTION :  Check if the service ID is registered.
*	COMMENTS : Service ID is 0 right only after the channel change
*			This value(Service ID for 1-Seg) is extracted from partial
*           receiption desc. in NIT
*	INPUT : void
* 	OUTPUT : unsigned char - return type
* 	RETURNS :
*****************************************************************************/
unsigned char ISDBT_TSPARSE_IsServiceIDRegistered(void)
{
	return (stTSAreaInfo.RegisteredAreaInfo.ServiceID == 0) ? FALSE : TRUE;
}

void ISDBT_Init_CA_DescriptorInfo(void)
{
    ISDBT_TSPARSE_DescriptorLock_On();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)

	if(g_stDescPMTCA.ucTableID == E_TABLE_ID_MAX)
	{
		g_stDescPMTCA.ucTableID = E_TABLE_ID_MAX;
		g_stDescPMTCA.uiCASystemID = -1;
		g_stDescPMTCA.uiCA_PID = -1;

		g_stDescCATCA.ucTableID = E_TABLE_ID_MAX;
		g_stDescCATCA.uiCASystemID = -1;
		g_stDescCATCA.uiCA_PID = -1;
	}

	ISDBT_TSPARSE_DescriptorLock_Off();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)
}

void ISDBT_Reset_AC_DescriptorInfo(void)
{
    ISDBT_TSPARSE_DescriptorLock_On();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)

	if(g_stDescPMTAC.ucTableID != E_TABLE_ID_MAX)
	{
		g_stDescPMTAC.ucTableID = E_TABLE_ID_MAX;
		g_stDescPMTAC.uiCASystemID = 0;
		g_stDescPMTAC.uiTransmissionType = 0;
		g_stDescPMTAC.uiPID = 0;
	}

	if(g_stDescCATAC.ucTableID != E_TABLE_ID_MAX)
	{
		g_stDescCATAC.ucTableID = E_TABLE_ID_MAX;
		g_stDescCATAC.uiCASystemID = 0;
		g_stDescCATAC.uiTransmissionType = 0;
		g_stDescCATAC.uiPID = 0;
	}

	ISDBT_TSPARSE_DescriptorLock_Off();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)
}

/**************************************************************************
*
 * EMERGENCY INFO DESCRIPTOR (0xFC)
 *  Using functions about EID should be done in single thread, otherwise it's required to prevent reentrance from several thread.
 *
 ***************************************************************************/
void isdbt_emergency_info_clear(void)
{
	T_DESC_EID	*p_eid_cur=NULL, *p_eid_next=NULL;

	p_eid_cur = g_eid_type.pEID;
	while(p_eid_cur){
		p_eid_next = p_eid_cur->pNext;
		tcc_mw_free(__FUNCTION__, __LINE__, p_eid_cur);
		p_eid_cur = p_eid_next;
	}

	g_eid_type.pEID = NULL;
}

int isdbt_emergency_info_set_data(T_DESC_EID **pp_eid, int service_id, int start_end_flag, int signal_type)
{
	T_DESC_EID	*p_desc_eid;

	p_desc_eid = (T_DESC_EID*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(T_DESC_EID));
	if(p_desc_eid == NULL){
		return -1;
	}

	p_desc_eid->pNext = NULL;
	p_desc_eid->service_id = service_id;
	p_desc_eid->start_end_flag = start_end_flag;
	p_desc_eid->signal_type = signal_type;
	p_desc_eid->area_code_length = 0;

	*pp_eid = p_desc_eid;
	return 0;
}

int isdbt_emergency_info_set_area(T_DESC_EID *p_set_eid, int area_code)
{
	if (p_set_eid == NULL) return -1;
	if (area_code < 0 || area_code > 0xFFF) {
		ALOGD("In %s, area_code is not valid:0x%x", __func__, area_code);
		return -1;
	}
	if (p_set_eid->area_code_length < 256) {
		p_set_eid->area_code[p_set_eid->area_code_length] = area_code;
		p_set_eid->area_code_length++;
	}
	return 0;
}

int isdbt_emergency_info_get_data(int *service_id, int *area_code, int *signal_type, int *start_end_flag, int *area_code_length)
{
	int	status = 0;
	T_DESC_EID	*p_eid_cur, *p_eid_next;
	int count;

	p_eid_cur = g_eid_type.pEID;
	while(p_eid_cur){
		p_eid_next = p_eid_cur->pNext;
		*service_id = p_eid_cur->service_id;
		*signal_type = p_eid_cur->signal_type;
		*start_end_flag = p_eid_cur->start_end_flag;
		*area_code_length = p_eid_cur->area_code_length;
		for(count=0; count < p_eid_cur->area_code_length;)
		{
			*area_code++ = p_eid_cur->area_code[count++];
			if (count >=256)
				break;
		}
		status = 1;
		/* get the first information intentionally.
		    If it's required to get all EWS info for different service_id, this function shoud be improved */
		break;
		p_eid_cur = p_eid_next;
	}
	return status;
}

/**************************************************************************
*
*  FUNCTION NAME
*      int ISDBT_Get_DescriptorInfo(unsigned char ucDescType, void *pDescData);
*
*  DESCRIPTION
*      This function returns the descriptor data according to input descriptor type.
*
*  INPUT
*      ucDescType = Descriptor Type.
*                   (Refer to E_DESC_TYPE.)
*      pDescData  = Pointer of Descriptor Structure.
*                   - E_DESC_SERVICE_LIST -> T_DESC_SERVICE_LIST
*                     ** After you needs pstSvcList, MUST free its memory
*                        by calling TC_Deallocate_Memory().
*                   - E_DESC_DTCP -> T_DESC_DTCP
*                   - E_DESC_PARENT_RATING -> T_DESC_PR
*
*  OUTPUT
*      int			= Return type.
*      pDescData	= Descriptor Data.
*
*  REMARK
*      - If ucTableID is E_TABLE_ID_MAX, it is the garbage information.
*      - MUST free the memory of pstSvcList which is the member of T_DESC_SERVICE_LIST.
*
**************************************************************************/
int ISDBT_Get_DescriptorInfo(unsigned char ucDescType, void *pDescData)
{
	T_DESC_DCC			*pDescDCC = NULL;
	T_DESC_CONTA		*pDescCONTA = NULL;
	T_DESC_SERVICE_LIST	*pDescSVCLIST = NULL;
	T_DESC_DTCP			*pDescDTCP = NULL;
	T_DESC_PR			*pDescPR = NULL;
	T_DESC_CA			*pDescCA = NULL;
	T_DESC_CA_SERVICE	*pDescCAService = NULL;
	T_DESC_AC			*pDescAC = NULL;
	T_DESC_EBD			*pDescEBD = NULL;
	T_DESC_TDSD			*pDescTDSD = NULL;
	T_DESC_LTD			*pDescLTD = NULL;
	T_DESC_BXML			*pDescBXML = NULL;

	int	rtn = ISDBT_ERROR;

	if (pDescData == NULL)
	{
		ALOGE(" ISDBT_PARSER][%d] Pointer of Desc Data is NULL!\n", __LINE__);
		return ISDBT_ERROR;
	}

    ISDBT_TSPARSE_DescriptorLock_On();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)

	/* Service List Descriptor */
	if (ucDescType == E_DESC_SERVICE_LIST)
	{
		int i;
		unsigned int uiAllocMemSize;

		pDescSVCLIST = (T_DESC_SERVICE_LIST *)pDescData;

		/* Copy the data of service list descriptor. */
		if (g_stDescSVCLIST.ucTableID != E_TABLE_ID_MAX)
		{
			pDescSVCLIST->ucTableID          = g_stDescSVCLIST.ucTableID;
			pDescSVCLIST->service_list_count = g_stDescSVCLIST.service_list_count;
			pDescSVCLIST->pstSvcList         = NULL;

			/* Set the array of service list. */
			if (pDescSVCLIST->service_list_count > 0)
			{
				uiAllocMemSize = pDescSVCLIST->service_list_count * sizeof(T_SERVICE_LIST);
				pDescSVCLIST->pstSvcList = (T_SERVICE_LIST *)tcc_mw_malloc(__FUNCTION__, __LINE__, uiAllocMemSize);
				if (pDescSVCLIST->pstSvcList == NULL)
				{
					ALOGE(" ISDBT_PARSER][%d] Failed in alloc the memory of service list!\n", __LINE__);
					ISDBT_TSPARSE_DescriptorLock_Off();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)
					return ISDBT_ERROR;
				}

				for (i = 0; i < pDescSVCLIST->service_list_count; i++)
				{
					pDescSVCLIST->pstSvcList[i].service_id   = g_stDescSVCLIST.pstSvcList[i].service_id;
					pDescSVCLIST->pstSvcList[i].service_type = g_stDescSVCLIST.pstSvcList[i].service_type;
				}
			}
			rtn = ISDBT_SUCCESS;
		}
	}
	/* DTCP(Digital Transmission Content Protection) Descriptor */
	else if (ucDescType == E_DESC_DTCP)
	{
		pDescDTCP = (T_DESC_DTCP *)pDescData;

		/* Copy the data of DTCP descriptor. */
		if (g_stDescDTCP.ucTableID != E_TABLE_ID_MAX)
		{
			pDescDTCP->ucTableID              	= g_stDescDTCP.ucTableID;
			pDescDTCP->CA_System_ID           	= g_stDescDTCP.CA_System_ID;
			pDescDTCP->Retention_Move_mode   = g_stDescDTCP.Retention_Move_mode;
			pDescDTCP->Retention_State        	= g_stDescDTCP.Retention_State;
			pDescDTCP->EPN                    		= g_stDescDTCP.EPN;
			pDescDTCP->DTCP_CCI               	= g_stDescDTCP.DTCP_CCI;
			pDescDTCP->Image_Constraint_Token = g_stDescDTCP.Image_Constraint_Token;
			pDescDTCP->APS                    		= g_stDescDTCP.APS;
			rtn = ISDBT_SUCCESS;
		}
	}
	/* Parental Rating Descriptor */
	else if (ucDescType == E_DESC_PARENT_RATING)
	{
		pDescPR = (T_DESC_PR *)pDescData;

		/* Copy the data of parental rating descriptor. */
		/* Priority Order : PMT > EIT */
		if(g_stDescPR[0].ucTableID != E_TABLE_ID_MAX && g_stDescPR[0].Rating != 0xff)
		{
			pDescPR->ucTableID	= g_stDescPR[0].ucTableID;
			pDescPR->Rating		= g_stDescPR[0].Rating;

			PARSE_DBG("[parental rate update] Get PMT rating:0x%X\n", g_stDescPR[0].Rating);
		}
		else if(g_stDescPR[1].ucTableID != E_TABLE_ID_MAX && g_stDescPR[1].Rating != 0xff)
		{
			pDescPR->ucTableID	= g_stDescPR[1].ucTableID;
			pDescPR->Rating		= g_stDescPR[1].Rating;

			PARSE_DBG("[parental rate update] Get EIT rating:0x%X\n", g_stDescPR[1].Rating);
		}
		else
		{
			pDescPR->ucTableID	= E_TABLE_ID_MAX;
			pDescPR->Rating		= 0xff;
		}

		if (pDescPR->ucTableID != E_TABLE_ID_MAX)
			rtn = ISDBT_SUCCESS;
	}
	else if(ucDescType == E_DESC_PMT_CA)
	{
		pDescCA = (T_DESC_CA *)pDescData;
		if (g_stDescPMTCA.ucTableID != E_TABLE_ID_MAX)
		{
			pDescCA->ucTableID = g_stDescPMTCA.ucTableID;
			pDescCA->uiCASystemID = g_stDescPMTCA.uiCASystemID;
			pDescCA->uiCA_PID = g_stDescPMTCA.uiCA_PID;
			rtn = ISDBT_SUCCESS;
		}
	}
	else if(ucDescType == E_DESC_CAT_CA)
	{
		pDescCA = (T_DESC_CA *)pDescData;
		if (g_stDescCATCA.ucTableID != E_TABLE_ID_MAX)
		{
			pDescCA->ucTableID = g_stDescCATCA.ucTableID;
			pDescCA->uiCASystemID = g_stDescCATCA.uiCASystemID;
			pDescCA->uiCA_PID = g_stDescCATCA.uiCA_PID;
			pDescCA->ucEMM_TransmissionFormat = g_stDescCATCA.ucEMM_TransmissionFormat;
			rtn = ISDBT_SUCCESS;
		}
	}
	else if(ucDescType == E_DESC_CAT_SERVICE)
	{
		pDescCAService = (T_DESC_CA_SERVICE *)pDescData;
		if(g_stDescCATCAService.ucTableID != E_TABLE_ID_MAX)
		{
			pDescCAService->ucTableID = g_stDescCATCAService.ucTableID ;
			pDescCAService->uiCA_Service_SystemID = g_stDescCATCAService.uiCA_Service_SystemID;
			pDescCAService->ucCA_Service_Broadcast_GroutID = g_stDescCATCAService.ucCA_Service_Broadcast_GroutID;
			pDescCAService->ucCA_Service_MessageCtrl = g_stDescCATCAService.ucCA_Service_MessageCtrl;
			pDescCAService->ucCA_Service_DescLen = g_stDescCATCAService.ucCA_Service_DescLen;
			rtn = ISDBT_SUCCESS;
		}
	}
	else if(ucDescType == E_DESC_PMT_AC)
	{
		pDescAC = (T_DESC_AC *)pDescData;
		if (g_stDescPMTAC.ucTableID != E_TABLE_ID_MAX)
		{
			pDescAC->ucTableID = g_stDescPMTAC.ucTableID;
			pDescAC->uiCASystemID = g_stDescPMTAC.uiCASystemID;
			pDescAC->uiTransmissionType = g_stDescPMTAC.uiTransmissionType;
			pDescAC->uiPID = g_stDescPMTAC.uiPID;
			rtn = ISDBT_SUCCESS;
		}
	}
	else if(ucDescType == E_DESC_CAT_AC)
	{
		pDescAC = (T_DESC_AC *)pDescData;
		if (g_stDescCATAC.ucTableID != E_TABLE_ID_MAX)
		{
			pDescAC->ucTableID = g_stDescCATAC.ucTableID;
			pDescAC->uiCASystemID = g_stDescCATAC.uiCASystemID;
			pDescAC->uiTransmissionType = g_stDescCATAC.uiTransmissionType;
			pDescAC->uiPID = g_stDescCATAC.uiPID;
			rtn = ISDBT_SUCCESS;
		}
	}
	else if(ucDescType == E_DESC_TERRESTRIAL_INFO)
	{
		pDescTDSD = (T_DESC_TDSD*)pDescData;
		if (g_stDescTDSD.freq_ch_num != 0xFF)
		{
			pDescTDSD->area_code = g_stDescTDSD.area_code;
			pDescTDSD->guard_interval = g_stDescTDSD.guard_interval;
			pDescTDSD->transmission_mode = g_stDescTDSD.transmission_mode;
			pDescTDSD->freq_ch_num = g_stDescTDSD.freq_ch_num;
			if (pDescTDSD->freq_ch_num > MAX_FREQ_TBL_NUM)
			{
				pDescTDSD->freq_ch_num = MAX_FREQ_TBL_NUM;
			}

			if (pDescTDSD->freq_ch_num > 0)
			{
				memset(&pDescTDSD->freq_ch_table[0], 0, MAX_FREQ_TBL_NUM);
				memcpy(&pDescTDSD->freq_ch_table[0], &g_stDescTDSD.freq_ch_table[0], pDescTDSD->freq_ch_num);
			}
			rtn = ISDBT_SUCCESS;
		}
		else
		{
			rtn = ISDBT_ERROR;
		}
		//ALOGD("[%s] TDSD freq_ch_num:%d\n", __func__, pDescTDSD->freq_ch_num);
	}
	else if(ucDescType == E_DESC_EXT_BROADCAST_INFO)
	{
		pDescEBD = (T_DESC_EBD*)pDescData;

		if (g_stDescEBD.affiliation_id_num > 0)
		{
			pDescEBD->affiliation_id_num = g_stDescEBD.affiliation_id_num;
			memcpy(&pDescEBD->affiliation_id_array[0], &g_stDescEBD.affiliation_id_array[0], pDescEBD->affiliation_id_num);

			rtn = ISDBT_SUCCESS;
		}
		else
		{
			rtn = ISDBT_ERROR;
		}
		//ALOGD("[%s] EBD affiliation_id_num:%d\n", __func__, pDescEBD->affiliation_id_num);
	}
	else if (ucDescType == E_DESC_LOGO_TRANSMISSION_INFO)
	{
		pDescLTD = (T_DESC_LTD *)pDescData;

		if (g_stDescLTD.ucTableID != E_TABLE_ID_MAX)
		{
			pDescLTD->ucTableID = g_stDescLTD.ucTableID;
			pDescLTD->download_data_id = g_stDescLTD.download_data_id;
			pDescLTD->logo_id = g_stDescLTD.logo_id;
			pDescLTD->logo_transmission_type = g_stDescLTD.logo_transmission_type;
			pDescLTD->logo_version = g_stDescLTD.logo_version;
			rtn = ISDBT_SUCCESS;
		}
		else
		{
			rtn = ISDBT_ERROR;
		}
	}
	else if (ucDescType == E_DESC_ARIB_BXML_INFO)
	{
		pDescBXML = (T_DESC_BXML *)pDescData;

		if (g_stDescBXML.ucTableID != E_TABLE_ID_MAX)
		{
			pDescBXML->ucTableID				= g_stDescBXML.ucTableID;
			pDescBXML->content_id_flag			= g_stDescBXML.content_id_flag;
			pDescBXML->associated_content_flag	= g_stDescBXML.associated_content_flag;
			pDescBXML->content_id				= g_stDescBXML.content_id;
			rtn = ISDBT_SUCCESS;
		}
		else
		{
			rtn = ISDBT_ERROR;
		}
	}
	/* Undefined Descriptor */
	else
	{
        ISDBT_TSPARSE_DescriptorLock_Off();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)
		return ISDBT_ERROR;
	}

    ISDBT_TSPARSE_DescriptorLock_Off();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)
	return rtn;
}

/**************************************************************************
*
*  FUNCTION NAME
*      int ISDBT_Set_DescriptorInfo(unsigned char ucDescType, void *pDescData);
*
*  DESCRIPTION
*      This function sets the descriptor data according to input descriptor type.
*
*  INPUT
*      ucDescType = Descriptor Type.
*                   (Refer to E_DESC_TYPE.)
*      pDescData  = Pointer of Descriptor Structure.
*
*  OUTPUT
*      int			= Return type.
*
*  REMARK
*      None
*
**************************************************************************/
int ISDBT_Set_DescriptorInfo(unsigned char ucDescType, void *pDescData)
{
	T_DESC_DCC			*pDescDCC = NULL;
	T_DESC_CONTA		*pDescCONTA = NULL;
	T_DESC_SERVICE_LIST	*pDescSVCLIST = NULL;
	T_DESC_DTCP			*pDescDTCP = NULL;
	T_DESC_PR			*pDescPR = NULL;
	T_DESC_CA			*pDescCA= NULL;
	T_DESC_CA_SERVICE	*pDescCAService= NULL;
	T_DESC_AC			*pDescAC= NULL;
	T_DESC_EBD			*pDescEBD = NULL;
	T_DESC_TDSD			*pDescTDSD = NULL;
	T_DESC_LTD			*pDescLTD = NULL;
	T_DESC_BXML			*pDescBXML = NULL;

	int iRetStatus;

	if (pDescData == NULL)
	{
		PARSE_DBG(" ISDBT_PARSER][%d] Pointer of Desc Data is NULL!\n", __LINE__);
		return ISDBT_ERROR;
	}

    ISDBT_TSPARSE_DescriptorLock_On();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)

	if (ucDescType == E_DESC_SERVICE_LIST)
	{
		pDescSVCLIST = (T_DESC_SERVICE_LIST *)pDescData;

		/* Free the memory of pstSvcList. */
		if (g_stDescSVCLIST.pstSvcList != NULL)
		{
			tcc_mw_free(__FUNCTION__, __LINE__, (void *)g_stDescSVCLIST.pstSvcList);
			g_stDescSVCLIST.pstSvcList = NULL;
		}

		/* Set the data of service list descriptor. - Only NIT */
		g_stDescSVCLIST.ucTableID          = pDescSVCLIST->ucTableID;
		g_stDescSVCLIST.service_list_count = pDescSVCLIST->service_list_count;
		g_stDescSVCLIST.pstSvcList         = pDescSVCLIST->pstSvcList;
	}
	/* DTCP(Digital Transmission Content Protection) Descriptor */
	else if (ucDescType == E_DESC_DTCP)
	{
		pDescDTCP = (T_DESC_DTCP *)pDescData;

		/* Copy the data of DTCP descriptor. */
		g_stDescDTCP.ucTableID              = pDescDTCP->ucTableID;
		g_stDescDTCP.CA_System_ID           = pDescDTCP->CA_System_ID;
		g_stDescDTCP.Retention_Move_mode    = pDescDTCP->Retention_Move_mode;
		g_stDescDTCP.Retention_State        = pDescDTCP->Retention_State;
		g_stDescDTCP.EPN                    = pDescDTCP->EPN;
		g_stDescDTCP.DTCP_CCI               = pDescDTCP->DTCP_CCI;
		g_stDescDTCP.Image_Constraint_Token = pDescDTCP->Image_Constraint_Token;
		g_stDescDTCP.APS                    = pDescDTCP->APS;
	}
	/* Parental Rating Descritpor */
	else if (ucDescType == E_DESC_PARENT_RATING)
	{
		pDescPR = (T_DESC_PR *)pDescData;

		if (pDescPR->ucTableID == E_TABLE_ID_PMT)
		{
			/* Copy the data of parental rating descriptor */
			g_stDescPR[0].ucTableID	= pDescPR->ucTableID;
			g_stDescPR[0].Rating	= pDescPR->Rating;
		}
		else if (pDescPR->ucTableID == E_TABLE_ID_EIT)
		{
			/* Copy the data of parental rating descriptor */
			g_stDescPR[1].ucTableID	= pDescPR->ucTableID;
			g_stDescPR[1].Rating	= pDescPR->Rating;
		}
		else
		{
			PRINTF(" ISDBT_PARSER][%d] Invalid setting of Parental Rating Descriptor (Table ID error)\n", __LINE__);
			ISDBT_TSPARSE_DescriptorLock_Off();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)
			return ISDBT_ERROR;
		}
	}
	else if(ucDescType == E_DESC_PMT_CA)
	{
		pDescCA = (T_DESC_CA *)pDescData;
		if(pDescCA->uiCASystemID == CAS_EFFECTIVE_ID)
		{
			g_stDescPMTCA.ucTableID = pDescCA->ucTableID;
			g_stDescPMTCA.uiCASystemID = pDescCA->uiCASystemID;
			g_stDescPMTCA.uiCA_PID = pDescCA->uiCA_PID;
		}
	}
	else if(ucDescType == E_DESC_CAT_CA)
	{
		pDescCA = (T_DESC_CA *)pDescData;
		if(pDescCA->uiCASystemID == CAS_EFFECTIVE_ID)
		{
			g_stDescCATCA.ucTableID = pDescCA->ucTableID;
			g_stDescCATCA.uiCASystemID = pDescCA->uiCASystemID;
			g_stDescCATCA.uiCA_PID = pDescCA->uiCA_PID;
			g_stDescCATCA.ucEMM_TransmissionFormat = pDescCA->ucEMM_TransmissionFormat;
		}
	}
	else if(ucDescType == E_DESC_CAT_SERVICE)
	{
		pDescCAService = (T_DESC_CA_SERVICE *)pDescData;
		if(pDescCAService->uiCA_Service_SystemID == CAS_EFFECTIVE_ID)
		{
			g_stDescCATCAService.ucTableID = pDescCAService->ucTableID;
			g_stDescCATCAService.uiCA_Service_SystemID = pDescCAService->uiCA_Service_SystemID;
			g_stDescCATCAService.ucCA_Service_Broadcast_GroutID =  pDescCAService->ucCA_Service_Broadcast_GroutID;
			g_stDescCATCAService.ucCA_Service_MessageCtrl =  pDescCAService->ucCA_Service_MessageCtrl;
			g_stDescCATCAService.ucCA_Service_DescLen =  pDescCAService->ucCA_Service_DescLen;
		}
	}
	else if(ucDescType == E_DESC_PMT_AC)
	{
		pDescAC = (T_DESC_AC *)pDescData;
		if(pDescAC->uiCASystemID == RMP_EFFECTIVE_ID)
		{
			g_stDescPMTAC.ucTableID = pDescAC->ucTableID;
			g_stDescPMTAC.uiCASystemID = pDescAC->uiCASystemID;
			g_stDescPMTAC.uiTransmissionType = pDescAC->uiTransmissionType;
			g_stDescPMTAC.uiPID = pDescAC->uiPID;
		}
	}
	else if(ucDescType == E_DESC_CAT_AC)
	{
		pDescAC = (T_DESC_AC *)pDescData;
		if(pDescAC->uiCASystemID == RMP_EFFECTIVE_ID)
		{
			g_stDescCATAC.ucTableID = pDescAC->ucTableID;
			g_stDescCATAC.uiCASystemID = pDescAC->uiCASystemID;
			g_stDescCATAC.uiTransmissionType = pDescAC->uiTransmissionType;
			g_stDescCATAC.uiPID = pDescAC->uiPID;
		}
	}
	else if(ucDescType == E_DESC_EMERGENCY_INFO)
	{
		EMERGENCY_INFO_DESCR	*p_eid_desc;
		EMERGENCY_INFO_DATA	*p_eid_cur, *p_eid_next;
		EMERGENCY_AREA_CODE	*p_area_cur, *p_area_next;
		T_DESC_EID				*p_eid, **pp_eid;
		T_DESC_EID		**pp_set_eid;
		int err = 0;

		isdbt_emergency_info_clear();

		pp_set_eid = &g_eid_type.pEID;

		p_eid_desc = (EMERGENCY_INFO_DESCR*)pDescData;
		p_eid_cur = p_eid_desc->pEID;
		while(p_eid_cur)
		{
			p_eid_next = p_eid_cur->pNext;
			p_area_cur = p_eid_cur->pAreaCode;

			if(p_area_cur)
			{
				err = isdbt_emergency_info_set_data (pp_set_eid,
						p_eid_cur->service_id, p_eid_cur->start_end_flag,
						p_eid_cur->signal_level);
				if (err)
				    goto EID_END;

				while(p_area_cur)
				{
					p_area_next = p_area_cur->pNext;
					isdbt_emergency_info_set_area(*pp_set_eid, p_area_cur->area_code);
					p_area_cur = p_area_next;
				}
			}
			p_eid_cur = p_eid_next;
			pp_set_eid = &(((T_DESC_EID*)*pp_set_eid)->pNext);
		}
	EID_END:
		if(err)
		{
			isdbt_emergency_info_clear();
		}
		free_eid(&(p_eid_desc->pEID));
	}
	else if(ucDescType == E_DESC_TERRESTRIAL_INFO)
	{
		pDescTDSD = (T_DESC_TDSD*)pDescData;
		memcpy(&g_stDescTDSD, pDescTDSD, sizeof(T_DESC_TDSD));
	}
	else if(ucDescType == E_DESC_EXT_BROADCAST_INFO)
	{
		pDescEBD = (T_DESC_EBD*)pDescData;
		memcpy(&g_stDescEBD, pDescEBD, sizeof(T_DESC_EBD));
	}
	else if (ucDescType == E_DESC_LOGO_TRANSMISSION_INFO)
	{
		pDescLTD = (T_DESC_LTD *)pDescData;
		memcpy (&g_stDescLTD, pDescLTD, sizeof(T_DESC_LTD));
	}
	else if (ucDescType == E_DESC_ARIB_BXML_INFO)
	{
		pDescBXML = (T_DESC_BXML *)pDescData;
		memcpy (&g_stDescBXML, pDescBXML, sizeof(T_DESC_BXML));
	}
	/* Undefined Descriptor */
	else
	{
        ISDBT_TSPARSE_DescriptorLock_Off();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)
		return ISDBT_ERROR;
	}

    ISDBT_TSPARSE_DescriptorLock_Off();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)
	return ISDBT_SUCCESS;
}

int ISDBT_TSPARSE_CURDESCINFO_init(int iServiceID, int iDescTableID)
{
	PARSE_DBG("%s ServiceID[0x%x] DescID[0x%x]", __func__, iServiceID, iDescTableID);
	int i;

	if(iServiceID != 0x0) {
		for(i=0;i<MAX_SUPPORT_CURRENT_SERVICE;i++) {
			if(gCurrentDescInfo.services[i].usServiceID == iServiceID) {
				if(iDescTableID == 0x0 || iDescTableID == DIGITALCOPY_CONTROL_ID) {
					if(gCurrentDescInfo.services[i].stDescDCC.sub_info)
					{
						SUB_T_DESC_DCC* sub_info = gCurrentDescInfo.services[i].stDescDCC.sub_info;
						while(sub_info)
						{
							SUB_T_DESC_DCC* temp_sub_info = sub_info->pNext;
							tcc_mw_free(__FUNCTION__, __LINE__, sub_info);
							sub_info = temp_sub_info;
						}
					}
					gCurrentDescInfo.services[i].stDescDCC.sub_info = 0;

					memset(&gCurrentDescInfo.services[i].stDescDCC, 0x0, sizeof(T_DESC_DCC));
				}
				if(iDescTableID == 0x0 || iDescTableID == CONTENT_AVAILABILITY_ID) {
					memset(&gCurrentDescInfo.services[i].stDescCONTA, 0x0, sizeof(T_DESC_CONTA));
				}
				if(iDescTableID == 0x0 || iDescTableID == EVT_GROUP_DESCR_ID) {
					memset(&gCurrentDescInfo.services[i].stEGD, 0x0, sizeof(T_ISDBT_EIT_EVENT_GROUP));
				}
				if(iDescTableID == 0x0 || iDescTableID == COMPONENT_GROUP_DESCR_ID) {
					memset(&gCurrentDescInfo.services[i].stDescCGD, 0x0, sizeof(T_DESC_CGD));
					gCurrentDescInfo.services[i].stDescCGD.component_group_type = 0xFF;
				}
				return 1;
			}
		}
	}
	else {
		memset(&gCurrentDescInfo, 0x0, sizeof(gCurrentDescInfo));
		gCurrentDescInfo.usNetworkID = 0xFFFF;
		for(i=0;i<MAX_SUPPORT_CURRENT_SERVICE;i++) {
			gCurrentDescInfo.services[i].stDescCGD.component_group_type = 0xFF;
		}
	}
	return 1;
}
/**************************************************************************
*
*  FUNCTION NAME
*      void ISDBT_Init_DescriptorInfo(void);
*
*  DESCRIPTION
*      This function initializes the descriptor information.
*
**************************************************************************/
void ISDBT_Init_DescriptorInfo(void)
{
	int iRetStatus;

    ISDBT_TSPARSE_DescriptorLock_On();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)

	/* Service List Descriptor */
	g_stDescSVCLIST.ucTableID = E_TABLE_ID_MAX;
	//g_stDescSVCLIST.service_list_count = 0;
	if (g_stDescSVCLIST.pstSvcList != NULL)
	{
		tcc_mw_free(__FUNCTION__, __LINE__, (void *)g_stDescSVCLIST.pstSvcList);
		g_stDescSVCLIST.pstSvcList = NULL;
	}

	/* DTCP Descriptor */
	g_stDescDTCP.ucTableID = E_TABLE_ID_MAX;

	/* Parental Rating Descriptor */
	g_stDescPR[0].ucTableID = E_TABLE_ID_MAX;
	g_stDescPR[0].Rating = 0xff;
	g_stDescPR[1].ucTableID = E_TABLE_ID_MAX;
	g_stDescPR[1].Rating = 0xff;

	g_stDescPMTCA.ucTableID = E_TABLE_ID_MAX;
	g_stDescCATCA.ucTableID = E_TABLE_ID_MAX;

	g_stDescPMTAC.ucTableID = E_TABLE_ID_MAX;
	g_stDescCATAC.ucTableID = E_TABLE_ID_MAX;

	g_stDescLTD.ucTableID = E_TABLE_ID_MAX;

	memset(&g_stDescEBD, 0x0, sizeof(T_DESC_EBD));
	memset(&g_stDescBXML, 0x0, sizeof(T_DESC_BXML));

	memset(&g_stDescTDSD, 0x0, sizeof(T_DESC_TDSD));
	g_stDescTDSD.freq_ch_num = 0xFF;

	ISDBT_TSPARSE_DescriptorLock_Off();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)

	ISDBT_TSPARSE_CURDESCINFO_init(0, 0);
	if(TCC_Isdbt_IsSupportCheckServiceID())
	{
		tcc_manager_demux_serviceInfo_Init();
	}
	tcc_manager_demux_detect_service_num_Init();
	tcc_manager_demux_check_service_num_Init();
}

/**************************************************************************
*
*  FUNCTION NAME
*      int ISDBT_Get_PIDInfo(int *piTotalAudioPID, int piAudioPID[], int piAudioStreamType[], int *piVideoPID, int *piVideoStreamType, int *piStPID, int *piSiPID);
*
*  DESCRIPTION
*      This function gets the PID value of audio, video and subtitle.
*
*  INPUT
*      piTotalAudioPID  = Pointer of count of audio PID.
*      ppiAudioPID  = Pointer array of audio PID.
*      piVideoPID  = Pointer of video PID.
*      piSubtitlePID  = Pointer of subtitle PID.
*
*  OUTPUT
*      int			= Return type.
*
*  REMARK
*      None
*
**************************************************************************/
int ISDBT_Get_PIDInfo(int *piTotalAudioPID, int piAudioPID[], int piAudioStreamType[], int *piTotalVideoPID, int piVideoPID[], int piVideoStreamType[], int *piStPID, int *piSiPID)
{
	int i;

	for(i = 0; i < g_stISDBT_ServiceList.uiTotalAudio_PID && i < MAX_AUDIOPID_SUPPORT; i++)
	{
		piAudioPID[i] = g_stISDBT_ServiceList.stAudio_PIDInfo[i].uiAudio_PID;
		piAudioStreamType[i] = g_stISDBT_ServiceList.stAudio_PIDInfo[i].ucAudio_StreamType;
	}
	*piTotalAudioPID = g_stISDBT_ServiceList.uiTotalAudio_PID;

	for(i = 0; i < g_stISDBT_ServiceList.uiTotalVideo_PID && i < MAX_VIDEOPID_SUPPORT; i++)
	{
		piVideoPID[i] = g_stISDBT_ServiceList.stVideo_PIDInfo[i].uiVideo_PID;
		piVideoStreamType[i] = g_stISDBT_ServiceList.stVideo_PIDInfo[i].ucVideo_StreamType;
	}
	*piTotalVideoPID = g_stISDBT_ServiceList.uiTotalVideo_PID;

	*piStPID = g_stISDBT_ServiceList.stSubtitleInfo[0].ucSubtitle_PID;
	*piSiPID = g_stISDBT_ServiceList.stSubtitleInfo[1].ucSubtitle_PID;

	if ((piAudioPID[0] == NULL_PID_F) || (*piVideoPID == NULL_PID_F))
	{
		return ISDBT_ERROR;
	}

	return ISDBT_SUCCESS;
}

void ISDBT_TSPARSE_Init(void)
{
	g_pSI_Table = NULL;

	ISDBT_TSPARSE_DescriptorLock_Init();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)
	ISDBT_TSPARSE_EitInfoLock_Init();
	ISDBT_TSPARSE_CdtInfoLock_Init();
}

// Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)
void ISDBT_TSPARSE_Deinit(void)
{
	ISDBT_TSPARSE_DescriptorLock_Deinit();
	ISDBT_TSPARSE_EitInfoLock_Deinit();
	ISDBT_TSPARSE_CdtInfoLock_Deinit();
}

void ISDBT_Get_ServiceInfo(unsigned int *pPcrPid, unsigned int *pServiceId)
{
	*pPcrPid = g_stISDBT_ServiceList.uiPCR_PID;
	*pServiceId = g_stISDBT_ServiceList.usServiceID;
}

void ISDBT_Get_ComponentTagInfo(unsigned int *TotalCount, unsigned int *pCompnentTags)
{
	int i;

	*TotalCount = g_stISDBT_ServiceList.ucPMTEntryTotalCount;

	for (i = 0; i < MAX_PMT_ES_TABLE_SUPPORT; ++i )
	{
		pCompnentTags[i] = g_stISDBT_ServiceList.ucComponent_Tags[i];
	}
}

int ISDBT_Get_1SegServiceCount(void)
{
	return g_stISDBT_ServiceList.iPartialReceptionDescNum;
}

int ISDBT_Get_TotalServiceCount(void)
{
	return g_stISDBT_ServiceList.iTotalServiceNum;
}

int ISDBT_Get_NetworkIDInfo(int *piNetworkID)
{
	*piNetworkID = g_stISDBT_ServiceList.usNetworkID;

	return ISDBT_SUCCESS;
}

void ISDBT_Clear_NetworkIDInfo(void)
{
	g_stISDBT_ServiceList.usNetworkID = 0;
}

/*
  *  ISDBT_TSPARSE_ProcessInsertService
  *  Insert a service information to DB.
  *  This routine is used only when test a special stream which doesn't have a NIT
  */
void ISDBT_TSPARSE_ProcessInsertService(unsigned int ch_no, unsigned int country_code, int service_type, unsigned short service_id)
{
	char sName[16];

	g_stISDBT_ServiceList.uiCurrentChannelNumber = ch_no;
	g_stISDBT_ServiceList.uiCurrentCountryCode = country_code;
	g_stISDBT_ServiceList.usServiceID = service_id;
	g_stISDBT_ServiceList.enType = service_type;
	InsertDB_ChannelTable(&g_stISDBT_ServiceList, 1);

	memset(&sName[0], 0, 16);
	sName[0] = 0x0e;	//LS1 - alphanumeric
	snprintf(&sName[1], 14, "%d", service_id);

	if (g_stISDBT_ServiceList.ucNameLen == 0) {
		g_stISDBT_ServiceList.ucNameLen = strlen(sName);
		g_stISDBT_ServiceList.strServiceName = (char*)sName;
	}

	g_stISDBT_ServiceList.iTSNameLen = strlen(sName);
	g_stISDBT_ServiceList.strTSName = (char *)sName;

	UpdateDB_ChannelName_ChannelTable(service_id, ch_no, country_code, &g_stISDBT_ServiceList);
	UpdateDB_TSName_ChannelTable(service_id, ch_no, country_code, &g_stISDBT_ServiceList);
}

/**************************************************************************
*  FUNCTION NAME :
*
*      int ISDBT_TSPARSE_PreProcess (void *handle, unsigned char *pucRawData, unsigned int uiRawDataLen, int RequestID);
*
*  DESCRIPTION : Before decode stream, check version or other information for each tables.
*                if data of each table is invalid(old version), we skip decode
*  INPUT:
*			handle	= Bit Parser Handle
*			pucRawData	= data pointer, it may start with table ID
*			uiRawDataLen = size of pucRawData
*			RequestID = RequestID of Input SI Data
*
*  OUTPUT:	int - Return Type
*  			= error type, if less than 0, it is error( -1:old version, -2:unkown error)
*			  if success, return Table ID
*
*  REMARK:
**************************************************************************/
int ISDBT_TSPARSE_PreProcess(void *handle, unsigned char *pucRawData, unsigned int uiRawDataLen, int RequestID)
{
	unsigned char ucTableID, ucSection, ucLastSection, ucSegmentLastSection = 0;
	t_BITPARSER_DRIVER_INFO_ *stDriverInfo;
	unsigned int curServiceId;
	unsigned char enCurrNext, ucVersionNumber, ucPreviousVersion= 0xFF;
	unsigned int uiSectionLength=0xFFF, uiPrevSectionLength = 0xFFF;
	unsigned int uiTSID, uiPrevTSID = 0xFFFF;
	unsigned int uiCurrentChannelNumber = gCurrentChannel;
	unsigned int uiCurrentCountryCode = gCurrentCountryCode;

	if (handle == NULL)
		return -1;

	ucSection = ucLastSection = 0xff;
	curServiceId= 0xffff;
	ucTableID = *pucRawData;

	if(tcc_manager_tuner_is_suspending()) {
		//ALOGD("Drop section data of previous channel[%d]!!!\n", ucTableID);
		return -1;
	}

	if( (ucTableID == NIT_A_ID) || (ucTableID == PAT_ID) || (ucTableID == PMT_ID) || (ucTableID == SDT_A_ID) ||
		(ucTableID == EIT_PF_A_ID) || ((ucTableID >= EIT_SA_0_ID) && (ucTableID <= EIT_SA_F_ID)) ||
		(ucTableID == ECM_0_ID) ||(ucTableID == ECM_1_ID) ||(ucTableID == EMM_0_ID) ||(ucTableID == EMM_1_ID) ||
		(ucTableID == CDT_ID) )
	{
		if(uiRawDataLen < 20) {
			return -2;
		}

		stDriverInfo = (t_BITPARSER_DRIVER_INFO_ *) handle;
		BITPARS_InitBitParser(stDriverInfo, pucRawData, 20);
#if 0
		BITPARS_GetBits(stDriverInfo, 24);	//table ID, section_syntax_indicator, section_length etc
#else
		if(ucTableID == PAT_ID || ucTableID == NIT_A_ID || ucTableID == PMT_ID){
			BITPARS_GetBits(stDriverInfo, 12);	//table ID, section_syntax_indicator, section_length etc
			BITPARS_GetBits(stDriverInfo, 12);	// section_length
			uiSectionLength = stDriverInfo->uiCurrent;
		}else{
			BITPARS_GetBits(stDriverInfo, 24);	//table ID, section_syntax_indicator, section_length etc
		}
#endif

		BITPARS_GetBits(stDriverInfo, 16);	/* get service_id. It's a download_data_id in case of CDT */
		uiTSID = stDriverInfo->uiCurrent;

		BITPARS_GetBits(stDriverInfo, 2);

		BITPARS_GetBits(stDriverInfo, 5);	/* get version_number */
		ucVersionNumber = stDriverInfo->ucCurrent;

		BITPARS_GetBits(stDriverInfo, 1);	/* get current_next_indicator */
		enCurrNext = (CURR_NEXT) stDriverInfo->ucCurrent;

		BITPARS_GetBits(stDriverInfo, 8);	/* section_number */
		ucSection = stDriverInfo->ucCurrent;

		BITPARS_GetBits(stDriverInfo, 8);	/* last_section_number */
		ucLastSection = stDriverInfo->ucCurrent;

		if((ucTableID == PAT_ID) || (ucTableID == PMT_ID) || (ucTableID == NIT_A_ID) || (ucTableID == SDT_A_ID) ||
			(ucTableID == ECM_0_ID) || (ucTableID == ECM_1_ID) || (ucTableID == EMM_0_ID) || (ucTableID == EMM_1_ID))
		{
			ucPreviousVersion = GetSIVersionNumber(RequestID, ucTableID);

			if(ucTableID == PAT_ID || ucTableID == NIT_A_ID || ucTableID == PMT_ID) {
				uiPrevSectionLength = GetSISectionLength(RequestID, ucTableID);
				uiPrevTSID = GetSITSID(RequestID, ucTableID);
			}
		}
		else if((ucTableID == EIT_PF_A_ID) || ((ucTableID >= EIT_SA_0_ID) && (ucTableID <= EIT_SA_F_ID)))
		{
			if (enCurrNext == 0)	//This is not yet applicable
			{
				PARSE_DBG("[%s] Unavailable next data\n", __func__);
				return ucTableID;
			}

			/*Below is for EIT, In case of Other descriptors, It may be wrong.
			 * Do NOT use at othter case not EIT.
			 */
			BITPARS_GetBits(stDriverInfo, 16);
			BITPARS_GetBits(stDriverInfo, 16);
			BITPARS_GetBits(stDriverInfo, 8);
			ucSegmentLastSection = stDriverInfo->ucCurrent;;

			ucPreviousVersion = GetEITVersionNumber(uiTSID, ucTableID);
		}
		else if (ucTableID == CDT_ID)
		{
			ucPreviousVersion = GetCDTVersion (uiTSID, ucSection);
		}
		else
		{
			return -2;
		}

		/*Check the PAT Version*/
		if((ucTableID == PAT_ID) && (ucVersionNumber != ucPreviousVersion) && (ucVersionNumber != 0xFF) && (ucPreviousVersion != 0xFF)){
			ALOGD("PAT version num different!!![%d:%d]\n", ucVersionNumber, ucPreviousVersion);
			if(TCC_Isdbt_IsSupportCheckServiceID()){
				tcc_manager_demux_serviceInfo_Init();
			}
			tcc_demux_section_decoder_update_pat_in_broadcast();
		}

		if (ucVersionNumber == ucPreviousVersion)
		{
#if 1
			if((ucTableID == PAT_ID || ucTableID == NIT_A_ID || ucTableID == PMT_ID) && (uiPrevSectionLength != uiSectionLength)){
				ALOGD("PAT/NIT/PMT section info(length) different!!![%d:%d] ver[%d] ucTableID[%d]\n", uiPrevSectionLength, uiSectionLength, ucVersionNumber, ucTableID);
			}else if((ucTableID == PAT_ID || ucTableID == NIT_A_ID) && (uiPrevTSID != uiTSID)){
				ALOGD("PAT/NIT section info(tsid/networkid) different!!![%d:%d] ver[%d] ucTableID[%d]\n", uiPrevTSID, uiTSID, ucVersionNumber, ucTableID);
			}else{
				return -1;
			}
#else
			return -1;
#endif
		}
	}

	return ucTableID;
}

void ISDBT_TSPARSE_PostProcess(void *handle, unsigned char *pucRawData, unsigned int uiRawDataLen, int RequestID)
{
	unsigned char ucTableID, ucSection, ucLastSection, ucSegmentLastSection = 0;
	t_BITPARSER_DRIVER_INFO_ *stDriverInfo;
	unsigned int curServiceId;
	unsigned char enCurrNext, ucVersionNumber, ucPreviousVersion = 0xFF;
	unsigned int uiSectionLength = 0xFFF, uiPrevSectionLength = 0xFFF;
	unsigned int uiTSID, uiPrevTSID = 0xFFFF;
	unsigned int uiCurrentChannelNumber = gCurrentChannel;
	unsigned int uiCurrentCountryCode = gCurrentCountryCode;
	int state;

	if (handle == NULL)
		return;

	ucSection = ucLastSection = 0xff;
	curServiceId= 0xffff;
	ucTableID = *pucRawData;

	if ((ucTableID == NIT_A_ID) || (ucTableID == PAT_ID) || (ucTableID == PMT_ID) || (ucTableID == SDT_A_ID) ||
		(ucTableID == EIT_PF_A_ID) || ((ucTableID >= EIT_SA_0_ID) && (ucTableID <= EIT_SA_F_ID)) ||
		(ucTableID == ECM_0_ID) ||(ucTableID == ECM_1_ID) ||(ucTableID == EMM_0_ID) ||(ucTableID == EMM_1_ID) ||
		(ucTableID == CDT_ID)
	)
	{
		stDriverInfo = (t_BITPARSER_DRIVER_INFO_ *) handle;
		BITPARS_InitBitParser(stDriverInfo, pucRawData, 20);

#if 0
		BITPARS_GetBits(stDriverInfo, 24);	//table ID, section_syntax_indicator, section_length etc
#else
		if(ucTableID == PAT_ID || ucTableID == NIT_A_ID || ucTableID == PMT_ID){
			BITPARS_GetBits(stDriverInfo, 12);	//table ID, section_syntax_indicator, section_length etc
			BITPARS_GetBits(stDriverInfo, 12);	// section_length
			uiSectionLength = stDriverInfo->uiCurrent;
		}else{
			BITPARS_GetBits(stDriverInfo, 24);	//table ID, section_syntax_indicator, section_length etc
		}
#endif

		BITPARS_GetBits(stDriverInfo, 16);	/* get service_id. It's a download_data_id in case of CDT */
		uiTSID = stDriverInfo->uiCurrent;

		BITPARS_GetBits(stDriverInfo, 2);

		BITPARS_GetBits(stDriverInfo, 5);	/* get version_number */
		ucVersionNumber = stDriverInfo->ucCurrent;

		BITPARS_GetBits(stDriverInfo, 1);	/* get current_next_indicator */
		enCurrNext = (CURR_NEXT) stDriverInfo->ucCurrent;

		BITPARS_GetBits(stDriverInfo, 8);	/* section_number */
		ucSection = stDriverInfo->ucCurrent;

		BITPARS_GetBits(stDriverInfo, 8);	/* last_section_number */
		ucLastSection = stDriverInfo->ucCurrent;

		if((ucTableID == EIT_PF_A_ID) || ((ucTableID >= EIT_SA_0_ID) && (ucTableID <= EIT_SA_F_ID)))
		{
			if (enCurrNext == 0)	//This is not yet applicable
		{
				PARSE_DBG("[%s] Unavailable next data\n", __func__);
				return;
			}

			/*Below is for EIT, In case of Other descriptors, It may be wrong.
			 * Do NOT use at othter case not EIT.
			 */
			BITPARS_GetBits(stDriverInfo, 16);
			BITPARS_GetBits(stDriverInfo, 16);
			BITPARS_GetBits(stDriverInfo, 8);
			ucSegmentLastSection = stDriverInfo->ucCurrent;;
		}

		state = tcc_manager_demux_get_state();
		if((state == E_DEMUX_STATE_INFORMAL_SCAN) || (state == E_DEMUX_STATE_AIRPLAY))
		{
			if((ucTableID == PAT_ID) || (ucTableID == PMT_ID) || (ucTableID == NIT_A_ID) || (ucTableID == SDT_A_ID) ||
				(ucTableID == ECM_0_ID) || (ucTableID == ECM_1_ID) || (ucTableID == EMM_0_ID) || (ucTableID == EMM_1_ID))
			{
				SetSIVersionNumber(RequestID, ucTableID, ucVersionNumber);
					if(ucTableID == PAT_ID || ucTableID == NIT_A_ID || ucTableID == PMT_ID){
						SetSISectionLength(RequestID, ucTableID, uiSectionLength);
						SetSITSID(RequestID, ucTableID, uiTSID);
					}
			} else if (ucTableID == CDT_ID) {
				SetCDTVersion (uiTSID, ucSection, ucLastSection, ucVersionNumber);
			} else if((ucTableID == EIT_PF_A_ID) || ((ucTableID >= EIT_SA_0_ID) && (ucTableID <= EIT_SA_F_ID)))
			{
				SetEITVersionNumber(uiCurrentChannelNumber, uiTSID, ucTableID, ucSection, ucLastSection, ucSegmentLastSection, ucVersionNumber);
				//LOGE("0x%02X %d %d %d\n", ucTableID, ucSection, ucSegmentLastSection, ucLastSection);
			}
		}
		if(tcc_manager_demux_is_epgscan()) {
			if((ucTableID == EIT_PF_A_ID) || ((ucTableID >= EIT_SA_0_ID) && (ucTableID <= EIT_SA_F_ID)))
			{
				SetEITVersionNumber(uiCurrentChannelNumber, uiTSID, ucTableID, ucSection, ucLastSection, ucSegmentLastSection, ucVersionNumber);
			}
		}
	}

	return;
}

void ISDBT_TSPARSE_ServiceList_Init(void)
{
	int i;

	unsigned int uiCurrentChannelNumber = gCurrentChannel;
	unsigned int uiCurrentCountryCode = gCurrentCountryCode;

	memset(&g_stISDBT_ServiceList, 0x00, sizeof(T_ISDBT_SERVICE_STRUCT));

	g_stISDBT_ServiceList.ucPMTVersionNum = 0xFF;
	g_stISDBT_ServiceList.ucSDTVersionNum = 0xFF;

	g_stISDBT_ServiceList.enType = SERVICE_RES_00;
	g_stISDBT_ServiceList.stFlags.fEIT_Schedule = FALSE;
	g_stISDBT_ServiceList.stFlags.fEIT_PresentFollowing = FALSE;
	g_stISDBT_ServiceList.stFlags.fCA_Mode_free = FALSE;
	g_stISDBT_ServiceList.stFlags.fAllowCountry = TRUE;
	g_stISDBT_ServiceList.ucNameLen = 0;
	g_stISDBT_ServiceList.strServiceName = NULL;
	g_stISDBT_ServiceList.ucActiveAudioIndex = 1;
	g_stISDBT_ServiceList.uiTeletext_PID = NULL_PID_F;
	g_stISDBT_ServiceList.uiPCR_PID = NULL_PID_F;
	g_stISDBT_ServiceList.uiTotalAudio_PID = 0;
	g_stISDBT_ServiceList.uiTotalVideo_PID = 0;


	g_stISDBT_ServiceList.uiCurrentChannelNumber = uiCurrentChannelNumber;
	g_stISDBT_ServiceList.uiCurrentCountryCode = uiCurrentCountryCode;

	g_stISDBT_ServiceList.iTSNameLen = 0;
	g_stISDBT_ServiceList.strTSName = NULL;

	g_stISDBT_ServiceList.iPartialReceptionDescNum = 0;
	g_stISDBT_ServiceList.iTotalServiceNum = 0;

	g_stISDBT_ServiceList.ucPMTEntryTotalCount = 0;
	for (i = 0; i < MAX_PMT_ES_TABLE_SUPPORT; ++i)
		g_stISDBT_ServiceList.ucComponent_Tags[i] = 0xFF;
}

int ISDBT_TSPARSE_ProcessPAT(int fFirstSection, int fDone, PAT_STRUCT * pstPAT, SI_MGMT_SECTION *pSection)
{
	unsigned int fInitService = 0;

	S16       ssService_Count = 0;
	unsigned int return_size = 0;
	int       iSize_ChannelInfo = 0;

	unsigned int uiCurrentChannelNumber = gCurrentChannel;
	unsigned int uiCurrentCountryCode = gCurrentCountryCode;

	char value[PROPERTY_VALUE_MAX];
	unsigned int uiLen = 0, invalid_service = 0;
	int skip_nit_flag=0;
	unsigned int usFsegServiceNum = 0;

	/* ---- management for section informatin start ----*/
	SI_MGMT_SECTIONLOOP *pSecLoop = (SI_MGMT_SECTIONLOOP*)NULL;
	SI_MGMT_SECTIONLOOP_DATA unSectionLoopData;
	int status;

	if (CHK_VALIDPTR(pSection) && CHK_VALIDPTR(pstPAT)) {
		unsigned short cnt;
		/* check section loop */
		for(cnt=0; cnt < pstPAT->uiEntries; cnt++) {
			unSectionLoopData.pat_loop.program_map_pid = pstPAT->astPatEntries[cnt].uiPID;
			unSectionLoopData.pat_loop.program_number = pstPAT->astPatEntries[cnt].uiServiceID;
			if ((pstPAT->astPatEntries[cnt].uiServiceID==0x00)
				|| (pstPAT->astPatEntries[cnt].uiServiceID==0xff)
				|| (pstPAT->astPatEntries[cnt].uiPID>=0x1FFF))
				continue;
			pSecLoop = (SI_MGMT_SECTIONLOOP*)NULL;
			status = TSPARSE_ISDBT_SiMgmt_Section_FindLoop (&pSecLoop, &unSectionLoopData, pSection);
			if (status != TSPARSE_SIMGMT_OK && !CHK_VALIDPTR(pSecLoop)) {
				status = TSPARSE_ISDBT_SiMgmt_Section_AddLoop(&pSecLoop, &unSectionLoopData, pSection);
				if (status != TSPARSE_SIMGMT_OK && !CHK_VALIDPTR(pSecLoop))
					SIMGMT_DEBUG("[ProcessPAT]Section_AddLoop (service_id=0x%x,PID=0x%x) fail (%d)",
							pstPAT->astPatEntries[ssService_Count].uiPID, pstPAT->astPatEntries[cnt].uiServiceID, status);
				else
					SIMGMT_DEBUG("[ProcessPAT]Section_AddLoop (service_id=0x%x,PID=0x%x) success",
							pstPAT->astPatEntries[ssService_Count].uiPID, pstPAT->astPatEntries[cnt].uiServiceID);
			}
		}
	}
	/* ---- management for section informatin end ----*/

	for (ssService_Count = 0; ssService_Count < (S16) pstPAT->uiEntries; ssService_Count++)
	{
		if ((pstPAT->astPatEntries[ssService_Count].uiServiceID == 0x00) || (pstPAT->astPatEntries[ssService_Count].uiServiceID == 0xFF))	/* If service_id is reserved */
		{
			continue;
		}
		//by see21 2007-8-2 15:30:16
		//some PAT has invalid PMT pids like 0x1fff so we must skip those pids
		if (pstPAT->astPatEntries[ssService_Count].uiPID >= 0x1FFF)
		{
			continue;
		}

		uiLen = property_get("tcc.dxb.isdbt.skipnit", value, "");
		if(uiLen) skip_nit_flag = atoi(value);
		if (skip_nit_flag != 0) {
			/*
			 * tcc.dxb.isdbt.skipnit should not be used for real product.
			 * this property is used only for testing special streams which doesn't have NIT in itself.
			 */
			int service_type;

			if (pstPAT->astPatEntries[ssService_Count].uiPID >= 0x1FC8 && pstPAT->astPatEntries[ssService_Count].uiPID <= (0x1FC8+8))
				service_type = 0xC0;
			else
				service_type = 0x01;
			ISDBT_TSPARSE_ProcessInsertService (uiCurrentChannelNumber, uiCurrentCountryCode, service_type, pstPAT->astPatEntries[ssService_Count].uiServiceID);
		}

		g_stISDBT_ServiceList.usServiceID = pstPAT->astPatEntries[ssService_Count].uiServiceID;
		g_stISDBT_ServiceList.uiTStreamID = pstPAT->uiTransportID;
		g_stISDBT_ServiceList.ucVersionNum = pstPAT->ucVersionNumber;
		g_stISDBT_ServiceList.PMT_PID = pstPAT->astPatEntries[ssService_Count].uiPID;

		/* (Add) data - End */
		/********************/
		tcc_manager_demux_set_serviceID(g_stISDBT_ServiceList.usServiceID);
		/*Check the service ID of broadcast*/
		if (TCC_Isdbt_IsSupportProfileA()) {
			if(TCC_Isdbt_IsSupportCheckServiceID()){
				tcc_manager_demux_serviceInfo_set(g_stISDBT_ServiceList.usServiceID);
			}
		}

		if(g_stISDBT_ServiceList.usServiceID > 0xF9BF){
			invalid_service++;
		}

		LLOGE("[%s] ServiceID = %x, PMT_PID= %x \n", __func__, g_stISDBT_ServiceList.usServiceID, g_stISDBT_ServiceList.PMT_PID);

		if (g_stISDBT_ServiceList.PMT_PID > 0 && g_stISDBT_ServiceList.PMT_PID < 0x1FFF)
		{
 			UpdateDB_StreamID_ChannelTable(g_stISDBT_ServiceList.usServiceID, g_stISDBT_ServiceList.uiTStreamID);
			UpdateDB_PMTPID_ChannelTable(g_stISDBT_ServiceList.usServiceID, g_stISDBT_ServiceList.PMT_PID);
		}

		usFsegServiceNum++;
	}
	/* (Add) Service List - End */
	/****************************/
	if (TCC_Isdbt_IsSupportProfileA()){
		g_stISDBT_ServiceList.iTotalServiceNum = usFsegServiceNum;
		if(TCC_Isdbt_IsSupportCheckServiceID()){
			tcc_manager_demux_serviceInfo_comparison(gSetServiceID);
		}

		if(invalid_service > 0 && usFsegServiceNum > 0){
			usFsegServiceNum  = usFsegServiceNum - invalid_service;
			invalid_service = 0;
		}
	}
	return 1;
}

int ISDBT_TSPARSE_AddVideoPID(PMT_STREAM_DESCR *pstEStreamDescr)
{
	DESCRIPTOR_STRUCT *pstDescriptor = NULL;
	unsigned int i, j;
	unsigned int uiTotalLangCount = 0;
	unsigned int index = 0, start_index = 0;
	unsigned char ucScramblingInfo = 0;

	if (g_stISDBT_ServiceList.uiTotalVideo_PID < MAX_VIDEOPID_SUPPORT)
	{
		start_index = g_stISDBT_ServiceList.uiTotalVideo_PID++;
		index = start_index;

		g_stISDBT_ServiceList.stVideo_PIDInfo[index].uiVideo_PID = pstEStreamDescr->uiStreamPID;
		//g_stISDBT_ServiceList.stVideo_PIDInfo[index].ucVideo_IsScrambling   = pstEStreamDescr->ucTransport_Scrambling_Control;
		g_stISDBT_ServiceList.stVideo_PIDInfo[index].ucVideo_StreamType = pstEStreamDescr->enStreamType;
		pstDescriptor = pstEStreamDescr->pstLocalDescriptor;
		while (pstDescriptor != NULL)
		{
			if (pstDescriptor->enDescriptorID == (DESCRIPTOR_IDS) STREAM_IDENT_DESCR_ID)
			{
				g_stISDBT_ServiceList.stVideo_PIDInfo[index].ucComponent_Tag = pstDescriptor->unDesc.stSID.ucComponentTag;
			}
			else if (pstDescriptor->enDescriptorID == (DESCRIPTOR_IDS) LANG_DESCR_ID)
			{
				uiTotalLangCount = pstDescriptor->unDesc.stISO_LD.ucLangCount;
				if (uiTotalLangCount > ISO_LANG_MAX)
					uiTotalLangCount = ISO_LANG_MAX;

				for (j = 0; j < uiTotalLangCount; j++)
				{
					if (j > 0)
					{
						index = g_stISDBT_ServiceList.uiTotalVideo_PID++;
						g_stISDBT_ServiceList.stVideo_PIDInfo[index].uiVideo_PID = pstEStreamDescr->uiStreamPID;
						//g_stISDBT_ServiceList.stVideo_PIDInfo[index].ucVideo_IsScrambling   = pstEStreamDescr->ucTransport_Scrambling_Control;
						g_stISDBT_ServiceList.stVideo_PIDInfo[index].ucVideo_IsScrambling = ucScramblingInfo;
						g_stISDBT_ServiceList.stVideo_PIDInfo[index].ucVideo_StreamType = pstEStreamDescr->enStreamType;
					}
				}
				//break;
			}
			else if ((pstDescriptor->enDescriptorID == (DESCRIPTOR_IDS) CA_DESCR_ID)
						|| (pstDescriptor->enDescriptorID == (DESCRIPTOR_IDS) ACCESS_CONTROL_DESCR_ID))
			{
				//g_stISDBT_ServiceList.stVideo_PIDInfo[index].ucVideo_IsScrambling = 1; //Scrambled
				if (ucScramblingInfo == 0)
				{
					ucScramblingInfo = 1;	//Scrambled
					for (i = 0; i < uiTotalLangCount; i++)
						g_stISDBT_ServiceList.stVideo_PIDInfo[i + start_index].ucVideo_IsScrambling = ucScramblingInfo;
				}
			}
			pstDescriptor = pstDescriptor->pstLinkedDescriptor;
		}
	}
	return 0;
}

int ISDBT_TSPARSE_AddAudioPID(PMT_STREAM_DESCR *pstEStreamDescr)
{
	DESCRIPTOR_STRUCT *pstDescriptor = NULL;
	unsigned int i, j;
	unsigned int uiTotalLangCount = 0;
	unsigned int index = 0, start_index = 0;
	unsigned char ucScramblingInfo = 0;

	if (g_stISDBT_ServiceList.uiTotalAudio_PID < MAX_AUDIOPID_SUPPORT)
	{
		start_index = g_stISDBT_ServiceList.uiTotalAudio_PID++;
		index = start_index;

		g_stISDBT_ServiceList.stAudio_PIDInfo[index].uiAudio_PID = pstEStreamDescr->uiStreamPID;
		//g_stISDBT_ServiceList.stAudio_PIDInfo[index].ucAudio_IsScrambling   = pstEStreamDescr->ucTransport_Scrambling_Control;
		g_stISDBT_ServiceList.stAudio_PIDInfo[index].ucAudio_StreamType = pstEStreamDescr->enStreamType;
		pstDescriptor = pstEStreamDescr->pstLocalDescriptor;
		while (pstDescriptor != NULL)
		{
			if (pstDescriptor->enDescriptorID == (DESCRIPTOR_IDS) STREAM_IDENT_DESCR_ID)
			{
				g_stISDBT_ServiceList.stAudio_PIDInfo[index].ucComponent_Tag = pstDescriptor->unDesc.stSID.ucComponentTag;
			}
			else if (pstDescriptor->enDescriptorID == (DESCRIPTOR_IDS) LANG_DESCR_ID)
			{
				uiTotalLangCount = pstDescriptor->unDesc.stISO_LD.ucLangCount;
				if (uiTotalLangCount > ISO_LANG_MAX)
					uiTotalLangCount = ISO_LANG_MAX;

				for (j = 0; j < uiTotalLangCount; j++)
				{
					if (j > 0)
					{
						index = g_stISDBT_ServiceList.uiTotalAudio_PID++;
						g_stISDBT_ServiceList.stAudio_PIDInfo[index].uiAudio_PID = pstEStreamDescr->uiStreamPID;
						//g_stISDBT_ServiceList.stAudio_PIDInfo[index].ucAudio_IsScrambling   = pstEStreamDescr->ucTransport_Scrambling_Control;
						g_stISDBT_ServiceList.stAudio_PIDInfo[index].ucAudio_IsScrambling = ucScramblingInfo;
						g_stISDBT_ServiceList.stAudio_PIDInfo[index].ucAudio_StreamType = pstEStreamDescr->enStreamType;
					}

					g_stISDBT_ServiceList.stAudio_PIDInfo[index].ucAudioType =
						pstDescriptor->unDesc.stISO_LD.ucAudioType[j];

					g_stISDBT_ServiceList.stAudio_PIDInfo[index].acLangCode[0] =
						pstDescriptor->unDesc.stISO_LD.acLangCode[j][0];
					g_stISDBT_ServiceList.stAudio_PIDInfo[index].acLangCode[1] =
						pstDescriptor->unDesc.stISO_LD.acLangCode[j][1];
					g_stISDBT_ServiceList.stAudio_PIDInfo[index].acLangCode[2] =
						pstDescriptor->unDesc.stISO_LD.acLangCode[j][2];
				}
				//break;
			}
			else if ((pstDescriptor->enDescriptorID == (DESCRIPTOR_IDS) CA_DESCR_ID)
						|| (pstDescriptor->enDescriptorID == (DESCRIPTOR_IDS) ACCESS_CONTROL_DESCR_ID))
			{
				//g_stISDBT_ServiceList.stAudio_PIDInfo[index].ucAudio_IsScrambling = 1; //Scrambled
				if (ucScramblingInfo == 0)
				{
					ucScramblingInfo = 1;	//Scrambled
					for (i = 0; i < uiTotalLangCount; i++)
						g_stISDBT_ServiceList.stAudio_PIDInfo[i + start_index].ucAudio_IsScrambling = ucScramblingInfo;
				}
			}
			pstDescriptor = pstDescriptor->pstLinkedDescriptor;
		}
	}
	return 0;
}

int ISDBT_TSPARSE_ProcessCAT(int fFirstSection, int fDone, CAT_STRUCT *pstCAT, SI_MGMT_SECTION *pSection)
{
	CAT_STRUCT 	*pCATSection;
	unsigned short			usDesCnt;
	DESCRIPTOR_STRUCT *pstDescriptor = NULL;

	/* ---- management for section informatin start ----*/
	DESCRIPTOR_STRUCT *pDesc = (DESCRIPTOR_STRUCT*)NULL;
	int status,cnt;

	if (CHK_VALIDPTR(pSection) && CHK_VALIDPTR(pstCAT)) {
		if (pstCAT->uiObjDescrCount > 0) {
			pstDescriptor = pstCAT->pstObjectDescr;
			for (cnt=0; cnt < pstCAT->uiObjDescrCount && CHK_VALIDPTR(pstDescriptor); cnt++) {
				pDesc = (DESCRIPTOR_STRUCT*)NULL;
				status = TSPARSE_ISDBT_SiMgmt_Section_FindDescriptor_Global(&pDesc, pstDescriptor->enDescriptorID, NULL, pSection);
				if (status != TSPARSE_SIMGMT_OK && !CHK_VALIDPTR(pDesc)) {
					status = TSPARSE_ISDBT_SiMgmt_Section_AddDescriptor_Global(&pDesc, pstDescriptor, pSection);
					if (status != TSPARSE_SIMGMT_OK && !CHK_VALIDPTR(pDesc))
						SIMGMT_DEBUG("[ProcessCAT]AddDescriptor (0x%x) fail", pstDescriptor->enDescriptorID);
				} else {
					/* do nothing in case that there is already same descriptor */
				}
				pstDescriptor = pstDescriptor->pstLinkedDescriptor;
			}
		}
	}
	/* ---- management for section informatin end ----*/

	pCATSection = (CAT_STRUCT *)pstCAT;
	usDesCnt = pCATSection->uiObjDescrCount;
	pstDescriptor = pCATSection->pstObjectDescr;

	while( usDesCnt > 0 )
	{
		while (pstDescriptor != NULL)
		{
			if(pstDescriptor->enDescriptorID == CA_DESCR_ID ) {
				T_DESC_CA stDescCA;

				stDescCA.ucTableID = E_TABLE_ID_CAT;
				stDescCA.uiCASystemID = pstDescriptor->unDesc.stCAD.uiCASystemID;
				stDescCA.uiCA_PID = pstDescriptor->unDesc.stCAD.uiCA_PID;

				if( pstDescriptor->unDesc.stCAD.ucDescLen >4)
					stDescCA.ucEMM_TransmissionFormat = pstDescriptor->unDesc.stCAD.aucPrivateData[0];
				if (ISDBT_Set_DescriptorInfo(E_DESC_CAT_CA, (void *)&stDescCA) != ISDBT_SUCCESS)
					ALOGE("---> [E_TABLE_ID_PMT] Failed in setting a ca descriptor info!\n");
			}
			else if(pstDescriptor->enDescriptorID ==   CA_SERVICE_DESCR_ID)
			{
				T_DESC_CA_SERVICE stDescCAService;

				stDescCAService.ucTableID = E_TABLE_ID_CAT;
				stDescCAService.uiCA_Service_SystemID = pstDescriptor->unDesc.stCASD.uiCA_Service_SystemID;
				stDescCAService.ucCA_Service_Broadcast_GroutID = pstDescriptor->unDesc.stCASD.ucCA_Service_Broadcast_GroutID;
				stDescCAService.ucCA_Service_MessageCtrl = pstDescriptor->unDesc.stCASD.ucCA_Service_MessageCtrl;
				stDescCAService.ucCA_Service_DescLen = pstDescriptor->unDesc.stCASD.ucCA_Service_DescLen;

				if (ISDBT_Set_DescriptorInfo(E_DESC_CAT_SERVICE, (void *)&stDescCAService) != ISDBT_SUCCESS)
				{
					ALOGE("---> [E_TABLE_ID_CAT] Failed in setting a ca descriptor info!\n");
				}

				ALOGE("CA Service -EMM: stDescCAService.uiCA_Service_SystemID =0x%x\n" , stDescCAService.uiCA_Service_SystemID);
				ALOGE("CA Service -EMM: stDescCAService.ucCA_Service_Broadcast_GroutID =0x%x\n" , stDescCAService.ucCA_Service_Broadcast_GroutID);
				ALOGE("CA Service -EMM: stDescCAService.ucCA_Service_MessageCtrl =0x%x\n" , stDescCAService.ucCA_Service_MessageCtrl);
				ALOGE("CA Service -EMM: stDescCAService.ucCA_Service_DescLen =0x%x\n" , stDescCAService.ucCA_Service_DescLen);

				if(stDescCAService.ucCA_Service_DescLen>4)
				{
					stDescCAService.uiCA_SerivceID = pstDescriptor->unDesc.stCASD.aucServiceID[0];
					ALOGE("CA Service -EMM: stDescCAService.uiCA_SerivceID =0x%x\n" , stDescCAService.uiCA_SerivceID);
				}
			}
			else if(pstDescriptor->enDescriptorID == ACCESS_CONTROL_DESCR_ID)
			{
				T_DESC_AC stDescAC;

				stDescAC.ucTableID = E_TABLE_ID_CAT;
				stDescAC.uiCASystemID = pstDescriptor->unDesc.stACCD.uiCASystemID;
				stDescAC.uiTransmissionType = pstDescriptor->unDesc.stACCD.uiTransmissionType;
				stDescAC.uiPID = pstDescriptor->unDesc.stACCD.uiPID;

				if (ISDBT_Set_DescriptorInfo(E_DESC_CAT_AC, (void *)&stDescAC) != ISDBT_SUCCESS)
				{
					ALOGE("---> [E_TABLE_ID_PMT] Failed in setting a access control descriptor info!\n");
				}
			}

			pstDescriptor = pstDescriptor->pstLinkedDescriptor;
		}
		usDesCnt--;
	}

	return 0;
}

int ISDBT_TSPARSE_ProcessPMT(int fFirstSection, int fDone, PMT_STRUCT *pstPMT, SI_MGMT_SECTION *pSection)
{
	U16       i, j;
	BOOL      fFoundVideo = FALSE;
	BOOL      fFoundAudio = FALSE;
	BOOL      fFoundSubtitle = FALSE;
	BOOL      fFoundTeletext = FALSE;

	int iAudioTotal, iVideoTotal, iSubtitleTotal, fCaptionMain, fSuperimposeMain;

	PMT_STREAM_DESCR *pstEStreamDescr = NULL;
	DESCRIPTOR_STRUCT *pstDescriptor = NULL;

	unsigned int uiCurrentChannelNumber = gCurrentChannel;
	unsigned int uiCurrentCountryCode = gCurrentCountryCode;

	unsigned char ucPMTDescBitMap = 0x0;

	char value[PROPERTY_VALUE_MAX];
	unsigned int uiLen = 0;
	int skip_nit_flag=0;

	/* ---- management for section informatin start ----*/
	SI_MGMT_SECTIONLOOP	*pSecLoop= (SI_MGMT_SECTIONLOOP*)NULL;
	SI_MGMT_SECTIONLOOP_DATA unSecLoopData;
	DESCRIPTOR_STRUCT *pDesc = (DESCRIPTOR_STRUCT*)NULL;
	int status;

	if (CHK_VALIDPTR(pSection) && CHK_VALIDPTR(pstPMT)) {
		/* check global descriptor */
		if (pstPMT->uiGDescrCount) {
			pstDescriptor = pstPMT->pstGlobDescr;
			while (CHK_VALIDPTR(pstDescriptor)) {
				pDesc = (DESCRIPTOR_STRUCT*)NULL;
				status = TSPARSE_ISDBT_SiMgmt_Section_FindDescriptor_Global(&pDesc, pstDescriptor->enDescriptorID, NULL, pSection);
				if (!(status==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pDesc))) {
					status = TSPARSE_ISDBT_SiMgmt_Section_AddDescriptor_Global(&pDesc, pstDescriptor, pSection);
					if (status!=TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pDesc))
						SIMGMT_DEBUG("[ProcessPMT]Section_AddDescriptor_Global(0x%x) failed(%d)", pstDescriptor->enDescriptorID, status);
					else
						SIMGMT_DEBUG("[ProcessPMT]Section_AddDescriptor_Global(0x%x) success", pstDescriptor->enDescriptorID);
				}
				pstDescriptor = pstDescriptor->pstLinkedDescriptor;
			}
		}

		/* check section loop */
		pstEStreamDescr = pstPMT->pstEStreamDescr;
		for(i=0; i < pstPMT->uiEStreamCount; i++) {
			if (!CHK_VALIDPTR(pstEStreamDescr)) break;

			pSecLoop = (SI_MGMT_SECTIONLOOP*)NULL;
			unSecLoopData.pmt_loop.stream_type = pstEStreamDescr->enStreamType;
			unSecLoopData.pmt_loop.elementary_PID = pstEStreamDescr->uiStreamPID;
			status = TSPARSE_ISDBT_SiMgmt_Section_FindLoop (&pSecLoop, &unSecLoopData, pSection);
			if (!(status==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pSecLoop))) {
				status = TSPARSE_ISDBT_SiMgmt_Section_AddLoop(&pSecLoop, &unSecLoopData, pSection);
				if (status != TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pSecLoop)) {
					pSecLoop = (SI_MGMT_SECTIONLOOP*)NULL;
					SIMGMT_DEBUG("[ProcessPMT]Section_AddLoop (type=0x%x,PID=0x%x) fail (%d)", pstEStreamDescr->enStreamType, pstEStreamDescr->uiStreamPID, status);
				} else
					SIMGMT_DEBUG("[ProcessPMT]Section_AddLoop (type=0x%x,PID=0x%x) success", pstEStreamDescr->enStreamType, pstEStreamDescr->uiStreamPID);
			} else {
				/* there is already a loop. do nothing */
			}
			/* add local descriptor to section loop */
			if (CHK_VALIDPTR(pSecLoop)) {
				pstDescriptor = pstEStreamDescr->pstLocalDescriptor;
				while (CHK_VALIDPTR(pstDescriptor))
				{
					pDesc = (DESCRIPTOR_STRUCT*)NULL;
					if (pstDescriptor->enDescriptorID == DATA_CONTENT_DESCR_ID)
						status = TSPARSE_ISDBT_SiMgmt_SectionLoop_FindDescriptor(&pDesc, pstDescriptor->enDescriptorID, (void*)&pstDescriptor->unDesc.stDCD.data_component_id, pSecLoop);
					else
						status = TSPARSE_ISDBT_SiMgmt_SectionLoop_FindDescriptor(&pDesc, pstDescriptor->enDescriptorID, NULL, pSecLoop);
					if (!(status==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pDesc))) {
						status = TSPARSE_ISDBT_SiMgmt_SectionLoop_AddDescriptor(&pDesc, pstDescriptor, pSecLoop);
						if (status!=TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pDesc)) {
							SIMGMT_DEBUG("[ProcessPMT]SectionLoop_AddDescriptor(0x%x) failed(%d)", pstDescriptor->enDescriptorID, status);
						}
						else
							SIMGMT_DEBUG("[ProcessPMT]SectionLoop_AddDescriptor(0x%x) success)", pstDescriptor->enDescriptorID);
					}
					pstDescriptor = pstDescriptor->pstLinkedDescriptor;
				}
			}
			pstEStreamDescr = pstEStreamDescr->pstStreamDescriptor;
		}
	}
	/* ---- management for section informatin end ----*/

	tcc_dxb_timecheck_set("switch_channel", "PMT_get", TIMECHECK_STOP);

	iAudioTotal = iVideoTotal = iSubtitleTotal = 0;
	fCaptionMain = fSuperimposeMain = 0;

	/***************************************************************************
	* PMT SCAN START... This will be removed if the home channel carries fully *
	*                   DVB compliant stream.                                  *
	***************************************************************************/
	g_stISDBT_ServiceList.usServiceID 				= pstPMT->uiServiceID;
	g_stISDBT_ServiceList.uiPCR_PID 				= pstPMT->uiPCR_PID;
	g_stISDBT_ServiceList.uiTeletext_PID 			= NULL_PID_F;
	g_stISDBT_ServiceList.ucPMTVersionNum 			= pstPMT->ucVersionNumber;
	g_stISDBT_ServiceList.ucAutoStartFlag			= 0;

	PARSE_DBG("[%s][%d] pstPMT->uiServiceID= %x  \n", __func__, __LINE__, pstPMT->uiServiceID);

	uiLen = property_get("tcc.dxb.isdbt.skipnit", value, "");
	if(uiLen) skip_nit_flag = atoi(value);
	if (skip_nit_flag != 0) {
		/*
		 * tcc.dxb.isdbt.skipnit should not be used for real product.
		 * this property is used only for testing special streams which doesn't have NIT in itself.
		 */
		if (TCC_Isdbt_IsSupportProfileA()) {
			/* no code by intention. in this case, service_id was already inserted when PAT was found */
		} else {
			ISDBT_TSPARSE_ProcessInsertService(uiCurrentChannelNumber, uiCurrentCountryCode, 0xC0, pstPMT->uiServiceID);
		}
	}

	g_stISDBT_ServiceList.uiTotalAudio_PID = 0;
	memset(g_stISDBT_ServiceList.stAudio_PIDInfo, 0x00, sizeof(AUDIOPID_INFO) * MAX_AUDIOPID_SUPPORT);
	for (i = 0; i < MAX_AUDIOPID_SUPPORT; i++)
	{
		g_stISDBT_ServiceList.stAudio_PIDInfo[i].uiAudio_PID = NULL_PID_F;
		g_stISDBT_ServiceList.stAudio_PIDInfo[i].ucAudio_IsScrambling = 0;
	}

	g_stISDBT_ServiceList.uiTotalVideo_PID = 0;
	memset(g_stISDBT_ServiceList.stVideo_PIDInfo, 0x00, sizeof(VIDEOPID_INFO) * MAX_VIDEOPID_SUPPORT);
	for (i = 0; i < MAX_VIDEOPID_SUPPORT; i++)
	{
		g_stISDBT_ServiceList.stVideo_PIDInfo[i].uiVideo_PID = NULL_PID_F;
		g_stISDBT_ServiceList.stVideo_PIDInfo[i].ucVideo_IsScrambling = 0;
	}

	/* initialize for subtitle list */
	g_stISDBT_ServiceList.usTotalCntSubtLang = 0;
	memset(g_stISDBT_ServiceList.stSubtitleInfo, 0x00, sizeof(SUBTITLE_INFO) * MAX_SUBTITLE_SUPPORT);
	for (i = 0; i < MAX_SUBTITLE_SUPPORT; i++)
	{
		g_stISDBT_ServiceList.stSubtitleInfo[i].ucSubtitle_PID = NULL_PID_F;
	}

	/* initialize for component_tags */
	g_stISDBT_ServiceList.ucPMTEntryTotalCount = 0;
	for (i = 0; i < MAX_PMT_ES_TABLE_SUPPORT; ++i)
		g_stISDBT_ServiceList.ucComponent_Tags[i] = 0xFF;

	i = 0;
	pstEStreamDescr = pstPMT->pstEStreamDescr;
	g_stISDBT_ServiceList.ucPMTEntryTotalCount = pstPMT->uiEStreamCount;
	while ((i < pstPMT->uiEStreamCount) && (pstEStreamDescr != NULL))
	{
		/* Gathering ComponentTags */
		{
			pstDescriptor = pstEStreamDescr->pstLocalDescriptor;

			if (pstDescriptor != NULL)
			{
				if (pstDescriptor->enDescriptorID == (DESCRIPTOR_IDS) STREAM_IDENT_DESCR_ID)
				{
					if ( i < MAX_PMT_ES_TABLE_SUPPORT )
					g_stISDBT_ServiceList.ucComponent_Tags[i] = pstDescriptor->unDesc.stSID.ucComponentTag;
				}
			}
		}

		if ((pstEStreamDescr->enStreamType == ES_MPEG1_VIDEO) || \
			(pstEStreamDescr->enStreamType == ES_MPEG2_VIDEO) || \
			(pstEStreamDescr->enStreamType == ES_SDMB_VIDEO))
		{
			fFoundVideo = TRUE;

			ISDBT_TSPARSE_AddVideoPID(pstEStreamDescr);
		}
		else if ( (pstEStreamDescr->enStreamType == ES_SDMB_AAC)  || \
				(pstEStreamDescr->enStreamType == ES_SDTBD_AAC) || \
				(pstEStreamDescr->enStreamType == ES_MPEG2_AUDIO))
		{
			fFoundAudio = TRUE;

			ISDBT_TSPARSE_AddAudioPID(pstEStreamDescr);
		}
		else if (pstEStreamDescr->enStreamType == ES_PRIV_PES)
		{
			STREAM_IDENT_DESCR		*p_stream_id_descr = NULL;

			pstDescriptor = pstEStreamDescr->pstLocalDescriptor;
			while (pstDescriptor != NULL)
			{
				if (pstDescriptor->enDescriptorID == (DESCRIPTOR_IDS) STREAM_IDENT_DESCR_ID)
				{
					p_stream_id_descr = &pstDescriptor->unDesc.stSID;

					/*
					  * 0x30 : subtitle(main)
					  * 0x31 ~ 0x37 : subtitle(sub)
					  * 0x38 : superimpose(main)
					  * 0x39 ~ 0x3f : superimpose(sub)
					  * 0x87 : 1seg subtitle
					  * 0x88 : 1seg superimpose
					  */

					if((p_stream_id_descr->ucComponentTag == 0x30)||(p_stream_id_descr->ucComponentTag == 0x87)){
						g_stISDBT_ServiceList.stSubtitleInfo[0].ucSubtitle_PID = pstEStreamDescr->uiStreamPID;
						g_stISDBT_ServiceList.stSubtitleInfo[0].ucComponent_Tag = p_stream_id_descr->ucComponentTag;
						fCaptionMain = 1;
					}else if((p_stream_id_descr->ucComponentTag == 0x38)||(p_stream_id_descr->ucComponentTag == 0x88)){
						g_stISDBT_ServiceList.stSubtitleInfo[1].ucSubtitle_PID = pstEStreamDescr->uiStreamPID;
						g_stISDBT_ServiceList.stSubtitleInfo[1].ucComponent_Tag = p_stream_id_descr->ucComponentTag;
						fSuperimposeMain = 1;
					}else{
						if (iSubtitleTotal < (MAX_SUBTITLE_SUPPORT-2)) {
							g_stISDBT_ServiceList.stSubtitleInfo[2+iSubtitleTotal].ucSubtitle_PID = pstEStreamDescr->uiStreamPID;
							g_stISDBT_ServiceList.stSubtitleInfo[2+iSubtitleTotal].ucComponent_Tag = p_stream_id_descr->ucComponentTag;
							iSubtitleTotal++;
						}
					}
					g_stISDBT_ServiceList.usTotalCntSubtLang = iSubtitleTotal + fCaptionMain + fSuperimposeMain;
				}
				pstDescriptor = pstDescriptor->pstLinkedDescriptor;
			}

			pstDescriptor = pstEStreamDescr->pstLocalDescriptor;
		}
		else if (pstEStreamDescr->enStreamType == ES_TC_DSM_CC)
		{
			unsigned char ComponentTag = 0;

			pstDescriptor = pstEStreamDescr->pstLocalDescriptor;
			while (pstDescriptor != NULL)
			{
				if (pstDescriptor->enDescriptorID == (DESCRIPTOR_IDS) STREAM_IDENT_DESCR_ID)
				{
					if (pstDescriptor->unDesc.stSID.ucComponentTag == 0x40)
					{
						ComponentTag = 0x40;
						g_stISDBT_ServiceList.uiDSMCC_PID = pstEStreamDescr->uiStreamPID;
					}
				}

				if (pstDescriptor->enDescriptorID == (DESCRIPTOR_IDS) DATA_COMPONENT_DESCR_ID)
				{
					DATA_COMPONENT_DESCR *p_data_component_descr = &pstDescriptor->unDesc.stDCOMP_D;

					/* additional_arib_bxml_info */
					if (p_data_component_descr->data_component_id == 0x000C && ComponentTag == 0x40)
					{
						char* temp;

						temp = (char*)&p_data_component_descr->a_data_comp_info;
						LLOGE("[DATA_COMPONENT_DESCR_ID] DSMCC_PID(%d) len(%d) 0x%02X \n", g_stISDBT_ServiceList.uiDSMCC_PID, p_data_component_descr->a_data_comp_info_length, temp[0] );

						if ((temp[0] & 0x30) == 0x30)
						{
							g_stISDBT_ServiceList.ucAutoStartFlag = 1;
							LLOGE("[DATA_COMPONENT_DESCR_ID] ucAutoStartFlag = 1 \n");
						}
					}
				}

				pstDescriptor = pstDescriptor->pstLinkedDescriptor;
			}
		}

		pstEStreamDescr = pstEStreamDescr->pstStreamDescriptor;
		i++;
	}

	if (pstPMT->uiGDescrCount)
	{
		DESCRIPTOR_STRUCT *pstDescriptor;
		pstDescriptor = pstPMT->pstGlobDescr;
		while (pstDescriptor != NULL)
		{
			if (pstDescriptor->enDescriptorID == CA_DESCR_ID)
			{
				T_DESC_CA stDescCA;

				stDescCA.ucTableID = E_TABLE_ID_PMT;
				stDescCA.uiCASystemID = pstDescriptor->unDesc.stCAD.uiCASystemID;
				stDescCA.uiCA_PID = pstDescriptor->unDesc.stCAD.uiCA_PID;

				if (ISDBT_Set_DescriptorInfo(E_DESC_PMT_CA, (void *)&stDescCA) != ISDBT_SUCCESS)
				{
					ALOGE("---> [E_TABLE_ID_PMT] Failed in setting a ca descriptor info!\n");
				}

				for (i = 0; i < g_stISDBT_ServiceList.uiTotalVideo_PID; i++)
					g_stISDBT_ServiceList.stVideo_PIDInfo[i].ucVideo_IsScrambling = 1;
				for (i = 0; i < g_stISDBT_ServiceList.uiTotalAudio_PID; i++)
					g_stISDBT_ServiceList.stAudio_PIDInfo[i].ucAudio_IsScrambling = 1;

				g_stISDBT_ServiceList.uiCA_ECM_PID = pstDescriptor->unDesc.stCAD.uiCA_PID;
			}
			else if (pstDescriptor->enDescriptorID == ACCESS_CONTROL_DESCR_ID)
			{
				T_DESC_AC stDescAC;
				char IsScrambling;

				stDescAC.ucTableID = E_TABLE_ID_PMT;
				stDescAC.uiCASystemID = pstDescriptor->unDesc.stACCD.uiCASystemID;
				stDescAC.uiTransmissionType = pstDescriptor->unDesc.stACCD.uiTransmissionType;
				stDescAC.uiPID = pstDescriptor->unDesc.stACCD.uiPID;

				if (ISDBT_Set_DescriptorInfo(E_DESC_PMT_AC, (void *)&stDescAC) != ISDBT_SUCCESS)
				{
					ALOGE("---> [E_TABLE_ID_PMT] Failed in setting a access control  descriptor info!\n");
				}

				IsScrambling = (stDescAC.uiPID == 0x1FFF)?0:1;
				for (i = 0; i < g_stISDBT_ServiceList.uiTotalVideo_PID; i++)
					g_stISDBT_ServiceList.stVideo_PIDInfo[i].ucVideo_IsScrambling = IsScrambling;
				for (i = 0; i < g_stISDBT_ServiceList.uiTotalAudio_PID; i++)
					g_stISDBT_ServiceList.stAudio_PIDInfo[i].ucAudio_IsScrambling = IsScrambling;

				if(pstDescriptor->unDesc.stACCD.uiTransmissionType == ISDBT_TRANSMISSION_TYPE_RMP) {
					g_stISDBT_ServiceList.uiAC_ECM_PID = pstDescriptor->unDesc.stACCD.uiPID;
				}
			}
			else if (pstDescriptor->enDescriptorID == PARENT_DESCR_ID)
			{
				T_DESC_PR	stDescPR_Local;
				unsigned char ucPRCount;

				ucPRCount = pstDescriptor->unDesc.stPRD.ucDataCount;
				if(ucPRCount > 0)
				{
					stDescPR_Local.ucTableID = E_TABLE_ID_PMT;
					stDescPR_Local.Rating = pstDescriptor->unDesc.stPRD.pastParentData->ucRating;
					if (ISDBT_Set_DescriptorInfo(E_DESC_PARENT_RATING, (void *)&stDescPR_Local) != ISDBT_SUCCESS)
					{
						ALOGE("---> [E_TABLE_ID_PMT] Failed in setting a descriptor info!\n");
					}
				}
			}
			else if(pstDescriptor->enDescriptorID == EMERGENCY_INFORMATION_DESCR_ID)
			{
				ISDBT_Set_DescriptorInfo(E_DESC_EMERGENCY_INFO, &pstDescriptor->unDesc.stEID);
			}
			else if (pstDescriptor->enDescriptorID == DIGITALCOPY_CONTROL_ID) {
				T_DESC_DCC	stDescDCC;

				/* Set the descriptor information. */
				stDescDCC.ucTableID                      = E_TABLE_ID_PMT;
				stDescDCC.digital_recording_control_data = pstDescriptor->unDesc.stDCCD.digital_recording_control_data;
				stDescDCC.maximum_bitrate_flag			 = pstDescriptor->unDesc.stDCCD.maximum_bitrate_flag;
				stDescDCC.component_control_flag         = pstDescriptor->unDesc.stDCCD.component_control_flag;
				stDescDCC.copy_control_type              = pstDescriptor->unDesc.stDCCD.copy_control_type;
				stDescDCC.APS_control_data               = pstDescriptor->unDesc.stDCCD.APS_control_data;
				stDescDCC.maximum_bitrate				 = pstDescriptor->unDesc.stDCCD.maximum_bitrate;

				stDescDCC.sub_info = 0;
				if( pstDescriptor->unDesc.stDCCD.sub_info )
				{
					SUB_T_DESC_DCC* stSubDescDCC;
					SUB_T_DESC_DCC* backup_sub_info;
					SUB_DIGITALCOPY_CONTROL_DESC* sub_info = pstDescriptor->unDesc.stDCCD.sub_info;

					while(sub_info)
					{
						stSubDescDCC = (SUB_T_DESC_DCC*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(SUB_T_DESC_DCC));
						if(0 == stSubDescDCC)
						{
							ALOGE("[%s][%d] : SUB_T_DESC_DCC malloc fail !!!!!\n", __func__, __LINE__);
							return -1;
						}
						memset(stSubDescDCC, 0, sizeof(SUB_T_DESC_DCC));

						stSubDescDCC->component_tag                  = sub_info->component_tag;
						stSubDescDCC->digital_recording_control_data = sub_info->digital_recording_control_data;
						stSubDescDCC->maximum_bitrate_flag           = sub_info->maximum_bitrate_flag;
						stSubDescDCC->copy_control_type              = sub_info->copy_control_type;
						stSubDescDCC->APS_control_data               = sub_info->APS_control_data;
						stSubDescDCC->maximum_bitrate                = sub_info->maximum_bitrate;

						if(0 == stDescDCC.sub_info)
						{
							stDescDCC.sub_info = stSubDescDCC;
							backup_sub_info = stSubDescDCC;
						}
						else
						{
							backup_sub_info->pNext = stSubDescDCC;
							backup_sub_info = stSubDescDCC;
						}

						sub_info = sub_info->pNext;
					}
				}

				if (ISDBT_TSPARSE_CURDESCINFO_set(pstPMT->uiServiceID, DIGITALCOPY_CONTROL_ID, (void *)&stDescDCC)) {
					ALOGE("---> [E_TABLE_ID_PMT] Failed in setting a current descriptor(0x%x) info!\n", pstDescriptor->enDescriptorID);
				}

				if(stDescDCC.sub_info)
				{
					SUB_T_DESC_DCC* sub_info = stDescDCC.sub_info;
					while(sub_info)
					{
						SUB_T_DESC_DCC* temp_sub_info = sub_info->pNext;
						tcc_mw_free(__FUNCTION__, __LINE__, sub_info);
						sub_info = temp_sub_info;
					}
				}
				stDescDCC.sub_info = 0;

				ucPMTDescBitMap |= 0x1;
			}
			else if (pstDescriptor->enDescriptorID == CONTENT_AVAILABILITY_ID) {
				T_DESC_CONTA	stDescCONTA;

				/* Set the descriptor information. */
				stDescCONTA.ucTableID				= E_TABLE_ID_PMT;
				stDescCONTA.copy_restriction_mode   = pstDescriptor->unDesc.stCONTAD.copy_restriction_mode;
				stDescCONTA.image_constraint_token	= pstDescriptor->unDesc.stCONTAD.image_constraint_token;
				stDescCONTA.retention_mode			= pstDescriptor->unDesc.stCONTAD.retention_mode;
				stDescCONTA.retention_state			= pstDescriptor->unDesc.stCONTAD.retention_state;
				stDescCONTA.encryption_mode			= pstDescriptor->unDesc.stCONTAD.encryption_mode;

				if (ISDBT_TSPARSE_CURDESCINFO_set(pstPMT->uiServiceID, CONTENT_AVAILABILITY_ID, (void *)&stDescCONTA)) {
					ALOGE("---> [E_TABLE_ID_PMT] Failed in setting a current descriptor(0x%x) info!\n", pstDescriptor->enDescriptorID);
				}
				ucPMTDescBitMap |= 0x2;
			}
			else if(pstDescriptor->enDescriptorID == EXT_BROADCASTER_DESCR_ID)
			{
				ISDBT_Set_DescriptorInfo(E_DESC_EXT_BROADCAST_INFO, &pstDescriptor->unDesc.stEBD);
				UpdateDB_AffiliationID_ChannelTable(uiCurrentChannelNumber, uiCurrentCountryCode, &g_stISDBT_ServiceList);
			}
			pstDescriptor = pstDescriptor->pstLinkedDescriptor;
		}
		ISDBT_TSPARSE_CURDESCINFO_setBitMap(pstPMT->uiServiceID, E_TABLE_ID_PMT, ucPMTDescBitMap);
		ucPMTDescBitMap = 0x0;
	}

	/*  (Service Type) default setting  */
	if (g_stISDBT_ServiceList.enType == SERVICE_RES_00)
	{
		if (g_stISDBT_ServiceList.uiTotalVideo_PID > 0)
		{
			g_stISDBT_ServiceList.enType = SERVICE_DIG_TV;
		}
		else if (g_stISDBT_ServiceList.uiTotalAudio_PID > 0)
		{
			g_stISDBT_ServiceList.enType = SERVICE_DIG_RADIO;
		}
	}

	tcc_manager_demux_set_serviceID(g_stISDBT_ServiceList.usServiceID);

	UpdateDB_ChannelTable(pstPMT->uiServiceID, uiCurrentChannelNumber, uiCurrentCountryCode, &g_stISDBT_ServiceList);

	InsertDB_VideoPIDTable(uiCurrentChannelNumber, uiCurrentCountryCode, &g_stISDBT_ServiceList);

	InsertDB_AudioPIDTable(uiCurrentChannelNumber, uiCurrentCountryCode, &g_stISDBT_ServiceList);

	InsertDB_SubTitlePIDTable(uiCurrentChannelNumber, uiCurrentCountryCode, &g_stISDBT_ServiceList);

	return 1;
}

int   ISDBT_TSPARSE_ProcessTDT(int fFirstSection, int fDone, TDT_STRUCT *pstTDT)
{
	DATE_TIME_STRUCT	DateTime;

	DateTime.uiMJD			= pstTDT->stDateTime.uiMJD;
	DateTime.stTime.ucHour	= pstTDT->stDateTime.stTime.ucHour;
	DateTime.stTime.ucMinute	= pstTDT->stDateTime.stTime.ucMinute;
	DateTime.stTime.ucSecond	= pstTDT->stDateTime.stTime.ucSecond;
	UpdateDB_TDTTime(&DateTime);

	return 0;
}

int   ISDBT_TSPARSE_ProcessTOT(int fFirstSection, int fDone, TOT_STRUCT *pstTOT, SI_MGMT_SECTION *pSection)
{
	DESCRIPTOR_STRUCT	*pstDescriptor;
	unsigned char		fLocalTimeOffsetDescParsed = FALSE;
	DATE_TIME_STRUCT	DateTime;
	LOCAL_TIME_OFFSET_STRUCT LocalTimeOffset;

	/* ---- management for section informatin start ----*/
	SI_MGMT_SECTIONLOOP *pSecLoop= (SI_MGMT_SECTIONLOOP*)NULL;
	SI_MGMT_SECTIONLOOP_DATA unSecLoopData;
	DESCRIPTOR_STRUCT *pDesc = (DESCRIPTOR_STRUCT*)NULL;
	int status;

	if (CHK_VALIDPTR(pSection) && CHK_VALIDPTR(pstTOT)) {
		/* check global descriptor */
		pstDescriptor = pstTOT->pstTotDescr;
		while (CHK_VALIDPTR(pstDescriptor)) {
			pDesc = (DESCRIPTOR_STRUCT*)NULL;
			status = TSPARSE_ISDBT_SiMgmt_Section_FindDescriptor_Global(&pDesc, pstDescriptor->enDescriptorID, NULL, pSection);
			if (!(status==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pDesc))) {
				status = TSPARSE_ISDBT_SiMgmt_Section_AddDescriptor_Global(&pDesc, pstDescriptor, pSection);
				if (status!=TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pDesc))
					SIMGMT_DEBUG("[ProcessTOT]Section_AddDescriptor_Global(0x%x) failed(%d)", pstDescriptor->enDescriptorID, status);
				else
					SIMGMT_DEBUG("[ProcessTOT]Section_AddDescriptor_Global(0x%x) success", pstDescriptor->enDescriptorID);
			}
			pstDescriptor = pstDescriptor->pstLinkedDescriptor;
		}
	}
	/* ---- management for section informatin end ----*/

	DateTime.uiMJD			= pstTOT->stDateTime.uiMJD;
	DateTime.stTime.ucHour	= pstTOT->stDateTime.stTime.ucHour;
	DateTime.stTime.ucMinute	= pstTOT->stDateTime.stTime.ucMinute;
	DateTime.stTime.ucSecond	= pstTOT->stDateTime.stTime.ucSecond;
	SetTOTTime(DateTime.stTime.ucHour, DateTime.stTime.ucMinute, DateTime.stTime.ucSecond);
	ISDBT_SetTOTMJD(DateTime.uiMJD);

	memset(&LocalTimeOffset, 0, sizeof(LOCAL_TIME_OFFSET_STRUCT));

	pstDescriptor = pstTOT->pstTotDescr;
	while(pstDescriptor != NULL)
	{
		switch(pstDescriptor->enDescriptorID)
		{
			case LOCAL_TIME_OFFSET_DESCR_ID:
			{
				int i, regionNum;
				unsigned char ucRegionID;
				unsigned char ucPolarity;
				unsigned char *pucLocalTimeOffset_BCD;		/* 4 BCD characters */
				unsigned char *pucTimeOfChange_BCD = NULL;	/* 4 BCD characters */
				unsigned char *pucNextTimeOffset_BCD;			/* 4 BCD characters */
			#ifdef TRACE_ISDBT_TOT
				unsigned short      usChangeMJD;
				DATE_TIME_T		real_date;
			#endif

				if(fLocalTimeOffsetDescParsed == TRUE)
					break;

				if(pstDescriptor->unDesc.stLTOD.ucCount > COUNTRY_REGION_ID_MAX){
					regionNum = COUNTRY_REGION_ID_MAX;
				}else{
					regionNum = pstDescriptor->unDesc.stLTOD.ucCount;
				}

				//PRINTF("[TOT parse] local area num : %d\n", regionNum);
				LocalTimeOffset.ucCount = regionNum;
				memset(LocalTimeOffset.astLocalTimeOffsetData, 0, sizeof(LOCAL_TIME_OFFSET_INFO) * COUNTRY_REGION_ID_MAX);

				for(i=0; i<regionNum; i++)
				{
					//usChangeMJD = 0;

					/* Country code */
					memcpy((void *)  &LocalTimeOffset.astLocalTimeOffsetData[i].aucCountryCode[0],
							(void *)(pstDescriptor->unDesc.stLTOD.astLocalTimeOffsetData[i].aucCountryCode), 3);

					/* Country region ID */
					ucRegionID = pstDescriptor->unDesc.stLTOD.astLocalTimeOffsetData[i].aucCountryRegionID;\
					LocalTimeOffset.astLocalTimeOffsetData[i].ucCountryRegionID = ucRegionID;

					/* Local time offset polarity */
					ucPolarity = pstDescriptor->unDesc.stLTOD.astLocalTimeOffsetData[i].ucLocalTimeOffsetPolarity;
					LocalTimeOffset.astLocalTimeOffsetData[i].ucLocalTimeOffsetPolarity = ucPolarity;

					/* Local time offset */
					memcpy(LocalTimeOffset.astLocalTimeOffsetData[i].ucLocalTimeOffset_BCD,
							pstDescriptor->unDesc.stLTOD.astLocalTimeOffsetData[i].ucLocalTimeOffset_BCD, 2);
					pucLocalTimeOffset_BCD = LocalTimeOffset.astLocalTimeOffsetData[i].ucLocalTimeOffset_BCD;

					/* Time of change */
					memcpy(LocalTimeOffset.astLocalTimeOffsetData[i].ucTimeOfChange_BCD/*ucTimeOfChange_BCD*/,
							pstDescriptor->unDesc.stLTOD.astLocalTimeOffsetData[i].ucTimeOfChange_BCD, 5);
					pucTimeOfChange_BCD = LocalTimeOffset.astLocalTimeOffsetData[i].ucTimeOfChange_BCD;

					/* Next time offset */
					memcpy(LocalTimeOffset.astLocalTimeOffsetData[i].ucNextTimeOffset_BCD/*ucNextTimeOffset_BCD*/,
							pstDescriptor->unDesc.stLTOD.astLocalTimeOffsetData[i].ucNextTimeOffset_BCD, 2);
					pucNextTimeOffset_BCD = LocalTimeOffset.astLocalTimeOffsetData[i].ucNextTimeOffset_BCD;

				#ifdef TRACE_ISDBT_TOT
					PARSE_DBG("[region #%d] Polarity : 0x%x\n", ucRegionID, ucPolarity);
					PARSE_DBG("\t Local Time Offset : 0x%02x 0x%02x\n", pucLocalTimeOffset_BCD[0], pucLocalTimeOffset_BCD[1]);
					PARSE_DBG("\t Time Of Change : 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
									pucTimeOfChange_BCD[0], pucTimeOfChange_BCD[1],
									pucTimeOfChange_BCD[2], pucTimeOfChange_BCD[3],
									pucTimeOfChange_BCD[4]);
					PARSE_DBG("\t Next Time Offset : 0x%02x 0x%02x\n", pucNextTimeOffset_BCD[0], pucNextTimeOffset_BCD[1]);

					usChangeMJD = pucTimeOfChange_BCD[0];
					usChangeMJD = ((usChangeMJD & 0xff) << 8);
					usChangeMJD += pucTimeOfChange_BCD[1];

					PARSE_DBG("\t Change Time : 0x%4x\n", usChangeMJD);

					memset(&real_date, 0, sizeof(DATE_TIME_T));
					ISDBT_TIME_GetRealDate((DATE_TIME_T *)&real_date, usChangeMJD);
					PARSE_DBG("\t year: %d, month: %d, day: %d\n\n", real_date.year, real_date.month, real_date.day);
				#endif
				}

				#if 1
				{
				/* Original Null Pointer Dereference
					unsigned short      usChangeMJD;
					usChangeMJD = pucTimeOfChange_BCD[0];
					usChangeMJD = ((usChangeMJD & 0xff) << 8);
					usChangeMJD += pucTimeOfChange_BCD[1];
					ISDBT_SetTOTMJD(usChangeMJD);
				*/
				// jini 9th : Ok
					if (pucTimeOfChange_BCD != NULL)
					{
						unsigned short      usChangeMJD;
						usChangeMJD = pucTimeOfChange_BCD[0];
						usChangeMJD = ((usChangeMJD & 0xff) << 8);
						usChangeMJD += pucTimeOfChange_BCD[1];
						ISDBT_SetTOTMJD(usChangeMJD);
					}
					//ALOGE("%s %d usChangeMJD = %x \n", __func__, __LINE__, usChangeMJD);
				}
				#endif

				/* [Remark] Only one local time offset descriptor is valid */
				fLocalTimeOffsetDescParsed = TRUE;
			}
			break;

			default:
				PARSE_DBG(" [TOT_PARSING] Detect unexpected descriptor : 0x%02x\n", pstDescriptor->enDescriptorID);
			break;
		}
		pstDescriptor = pstDescriptor->pstLinkedDescriptor;
	}

	UpdateDB_TOTTime(&DateTime, &LocalTimeOffset);

	return 0;
}

int ISDBT_TSPARSE_ProcessNIT(int fFirstSection, int fDone, NIT_STRUCT *pstNIT, SI_MGMT_SECTION *pSection)
{
	TRANS_STREAM_DESCR *	pTSDescr_t = NULL;
	DESCRIPTOR_STRUCT * pstTSDescriptor_t = NULL;
	DESCRIPTOR_STRUCT * pstPRDescriptor_t = NULL;
	unsigned short usNetworkId = 0;
	unsigned int uiCurrentChannelNumber = gCurrentChannel;
	unsigned int uiCurrentCountryCode = gCurrentCountryCode;
	unsigned short usServiceID = 0xff;
	unsigned short usServiceNum = 0xff;
	int i, j, fReqSvcNameUpdate = 0, total_service_num, invalid_service = 0;
	unsigned int ts_name_len = 0;

	SI_MGMT_TABLE *pTable;

	/* ---- management for section informatin start ----*/
	SI_MGMT_SECTIONLOOP *pSecLoop= (SI_MGMT_SECTIONLOOP*)NULL;
	SI_MGMT_SECTIONLOOP_DATA unSecLoopData;
	DESCRIPTOR_STRUCT *pDesc = (DESCRIPTOR_STRUCT*)NULL;
	int status;

	if (CHK_VALIDPTR(pSection) && CHK_VALIDPTR(pstNIT))
	{
		/* check global descriptor */
		if (pstNIT->uiNDescrCount)
		{
			pstTSDescriptor_t = pstNIT->pstNDescr;
			while (CHK_VALIDPTR(pstTSDescriptor_t))
			{
				pDesc = (DESCRIPTOR_STRUCT*)NULL;
				status = TSPARSE_ISDBT_SiMgmt_Section_FindDescriptor_Global(&pDesc, pstTSDescriptor_t->enDescriptorID, NULL, pSection);
				if (!(status==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pDesc)))
				{
					status = TSPARSE_ISDBT_SiMgmt_Section_AddDescriptor_Global(&pDesc, pstTSDescriptor_t, pSection);
					if (status!=TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pDesc))
						SIMGMT_DEBUG("[ProcessNIT]Section_AddDescriptor_Global(0x%x) failed(%d)", pstTSDescriptor_t->enDescriptorID, status);
					else
						SIMGMT_DEBUG("[ProcessNIT]Section_AddDescriptor_Global(0x%x) success", pstTSDescriptor_t->enDescriptorID);
				}
				pstTSDescriptor_t = pstTSDescriptor_t->pstLinkedDescriptor;
			}
		}

		/* check section loop */
		pTSDescr_t = pstNIT->pstTSDescriptor;
		for(i=0; i < pstNIT->uiTSDescrCount; i++)
		{
			if (!CHK_VALIDPTR(pTSDescr_t))
				break;

			pSecLoop = (SI_MGMT_SECTIONLOOP*)NULL;
			unSecLoopData.nit_loop.transport_stream_id = pTSDescr_t->uiTStreamID;
			unSecLoopData.nit_loop.original_network_id = pTSDescr_t->uiOrgTStreamID;

			status = TSPARSE_ISDBT_SiMgmt_Section_FindLoop (&pSecLoop, &unSecLoopData, pSection);
			if (!(status==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pSecLoop)))
			{
				status = TSPARSE_ISDBT_SiMgmt_Section_AddLoop(&pSecLoop, &unSecLoopData, pSection);
				if (status != TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pSecLoop))
				{
					pSecLoop = (SI_MGMT_SECTIONLOOP*)NULL;
					SIMGMT_DEBUG("[ProcessNIT]Section_AddLoop (ts_stream_id=0x%x) fail(%d)", pTSDescr_t->uiTStreamID, status);
				}
				else
					SIMGMT_DEBUG("[ProcessNIT]Section_AddLoop (ts_stream_id=0x%x) success", pTSDescr_t->uiTStreamID);
			}
			else
			{
				/* there is already a loop. do nothing */
			}

			/* add local descriptor to section loop */
			if (CHK_VALIDPTR(pSecLoop))
			{
				pstTSDescriptor_t = pTSDescr_t->pstTSDescriptor;
				while (CHK_VALIDPTR(pstTSDescriptor_t))
				{
					pDesc = (DESCRIPTOR_STRUCT*)NULL;

					status = TSPARSE_ISDBT_SiMgmt_SectionLoop_FindDescriptor(&pDesc, pstTSDescriptor_t->enDescriptorID, NULL, pSecLoop);
					if (!(status==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pDesc)))
					{
						status = TSPARSE_ISDBT_SiMgmt_SectionLoop_AddDescriptor(&pDesc, pstTSDescriptor_t, pSecLoop);
						if (status!=TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pDesc))
						{
							SIMGMT_DEBUG("[ProcessNIT]SectionLoop_AddDescriptor(0x%x) fail(%d), size=%d", pstTSDescriptor_t->enDescriptorID, status, pstTSDescriptor_t->struct_length);
						}
						else
							SIMGMT_DEBUG("[ProcessNIT]SectionLoop_AddDescriptor(0x%x) success", pstTSDescriptor_t->enDescriptorID);
					}
					pstTSDescriptor_t = pstTSDescriptor_t->pstLinkedDescriptor;
				}
			}
			pTSDescr_t = pTSDescr_t->pstTStreamDescriptor;
		}
	}
	/* ---- management for section informatin end ----*/

	tcc_dxb_timecheck_set("switch_channel", "NIT_get", TIMECHECK_STOP);

	usNetworkId = pstNIT->uiNetworkID;
	pTSDescr_t = pstNIT->pstTSDescriptor;
	if(pTSDescr_t != NULL)
	{
		pstTSDescriptor_t = pTSDescr_t->pstTSDescriptor;
		while (pstTSDescriptor_t != NULL)
		{
			//[2011.06.01] In case of 1seg, partial_reception_descriptr should be processed at first becasue tabel of DB would be maden on finding this descriptor.
			if (pstTSDescriptor_t->enDescriptorID == PARTIAL_RECEPTION_DESCR_ID)
			{
				pstPRDescriptor_t = pstTSDescriptor_t;

				usServiceNum = pstTSDescriptor_t->unDesc.stPRECEP_D.numServiceId;
				g_stISDBT_ServiceList.iPartialReceptionDescNum = usServiceNum;

				if (!TCC_Isdbt_IsSupportProfileA())
				{
					for (i = 0; i < usServiceNum; i++)
					{
						LLOGD("[NIT] (#%d) 1 Seg Service ID : 0x%x NetworkID = %d \n", i, pstTSDescriptor_t->unDesc.stPRECEP_D.servicd_id[i], usNetworkId);

						usServiceID = pstTSDescriptor_t->unDesc.stPRECEP_D.servicd_id[i];

						g_stISDBT_ServiceList.usNetworkID = usNetworkId;
						g_stISDBT_ServiceList.uiTStreamID = pTSDescr_t->uiTStreamID;
						g_stISDBT_ServiceList.usOrig_Net_ID = pTSDescr_t->uiOrgTStreamID;
						g_stISDBT_ServiceList.enType = 0xC0;
						g_stISDBT_ServiceList.usServiceID = pstTSDescriptor_t->unDesc.stPRECEP_D.servicd_id[i];

						if(g_stISDBT_ServiceList.usServiceID > 0xF9BF)
						{
							invalid_service++;
						}

						InsertDB_ChannelTable(&g_stISDBT_ServiceList, 1);
						UpdateDB_StreamID_ChannelTable(pstTSDescriptor_t->unDesc.stPRECEP_D.servicd_id[i], g_stISDBT_ServiceList.uiTStreamID);
						UpdateDB_OriginalNetworID_ChannelTable(pstTSDescriptor_t->unDesc.stPRECEP_D.servicd_id[i], uiCurrentChannelNumber, uiCurrentCountryCode, &g_stISDBT_ServiceList);
						if(TCC_Isdbt_IsSupportCheckServiceID())
						{
							tcc_manager_demux_serviceInfo_set(g_stISDBT_ServiceList.usServiceID);
						}
					}

					if(TCC_Isdbt_IsSupportCheckServiceID())
					{
						tcc_manager_demux_serviceInfo_comparison(gSetServiceID);
					}

					if(invalid_service > 0 && usServiceNum > 0)
					{
						usServiceNum  = usServiceNum - invalid_service;
						invalid_service = 0;
					}

					tcc_manager_demux_set_service_num(usServiceNum);
					tcc_manager_demux_detect_service_num(tcc_manager_demux_get_service_num(), g_stISDBT_ServiceList.usNetworkID);

					/* To check the change status of partial reception service ID */
					if (ISDBT_TSPARSE_IsServiceIDRegistered())
					{
						/* In case that Registered service ID already exists */
						if (ISDBT_TSPARSE_GetRegisteredServiceID() != usServiceID)
						{
							PARSE_DBG("[NIT Info] 1-Seg Service ID is Changed !!: 0x%x -> 0x%x\n", ISDBT_TSPARSE_GetRegisteredServiceID(), usServiceID);

							ISDBT_TSPARSE_SetCurrServiceID(usServiceID);
						}
					}
				}
			}
			pstTSDescriptor_t = pstTSDescriptor_t->pstLinkedDescriptor;
		}
	}

	while (pTSDescr_t != NULL)
	{
		g_stISDBT_ServiceList.uiTStreamID = pTSDescr_t->uiTStreamID;
		g_stISDBT_ServiceList.usOrig_Net_ID = pTSDescr_t->uiOrgTStreamID;

		pstTSDescriptor_t = pTSDescr_t->pstTSDescriptor;
		while (pstTSDescriptor_t != NULL)
		{
			/* extract/save remote_control_key_id to calculate the 1-seg channel number */
			if (pstTSDescriptor_t->enDescriptorID == TS_INFO_DESCR_ID)
			{
				g_stISDBT_ServiceList.usNetworkID = usNetworkId;
				g_stISDBT_ServiceList.remocon_ID = pstTSDescriptor_t->unDesc.stTSID.remote_control_key_id;

				ts_name_len = TSMIN(pstTSDescriptor_t->unDesc.stTSID.length_of_ts_name, MAX_TS_NAME_LEN);
				if(ts_name_len != 0)
				{
					memset(&g_ts_name_char[0], 0, MAX_TS_NAME_LEN+1);
					memcpy(&g_ts_name_char[0], pstTSDescriptor_t->unDesc.stTSID.ts_name_char, ts_name_len);

					if (!tcc_manager_demux_is_skip_sdt_in_scan())
					{
						if (g_stISDBT_ServiceList.ucNameLen==0)
						{
							/* if SDT is not yet received ? */
							g_stISDBT_ServiceList.ucNameLen = ts_name_len;
							g_stISDBT_ServiceList.strServiceName = (char*)g_ts_name_char;
							fReqSvcNameUpdate = 1;
						}
					}

					g_stISDBT_ServiceList.iTSNameLen = ts_name_len;
					g_stISDBT_ServiceList.strTSName = (char *)g_ts_name_char;
				}
			}
/*
			else if(pstTSDescriptor_t->enDescriptorID ==CA_EMM_TS_DESCR_ID)
			{
				CA_EMM_TS_DESCR stDescCAEmmTs;
				stDescCAEmmTs.uiCAEMM_CASystemID = pstTSDescriptor_t->unDesc.stCAEMMTSD.uiCAEMM_CASystemID;
				stDescCAEmmTs.uiCAEMM_TsStreamID = pstTSDescriptor_t->unDesc.stCAEMMTSD.uiCAEMM_TsStreamID;
				stDescCAEmmTs.uiCAEMM_OrgNetworkID = pstTSDescriptor_t->unDesc.stCAEMMTSD.uiCAEMM_OrgNetworkID;
				stDescCAEmmTs.ucCAEMM_PowerSuplyPeriod = pstTSDescriptor_t->unDesc.stCAEMMTSD.ucCAEMM_PowerSuplyPeriod;

				ALOGE("CA EmmTS -EMM: stDescCAService.uiCAEMM_CASystemID =0x%x\n" , stDescCAEmmTs.uiCAEMM_CASystemID);
				ALOGE("CA EmmTS -EMM: stDescCAService.uiCAEMM_TsStreamID =0x%x\n" , stDescCAEmmTs.uiCAEMM_TsStreamID);
				ALOGE("CA EmmTS -EMM: stDescCAService.uiCAEMM_OrgNetworkID =0x%x\n" , stDescCAEmmTs.uiCAEMM_OrgNetworkID);
				ALOGE("CA EmmTS -EMM: stDescCAService.ucCAEMM_PowerSuplyPeriod =0x%x\n" , stDescCAEmmTs.ucCAEMM_PowerSuplyPeriod);

			}
*/
			pstTSDescriptor_t = pstTSDescriptor_t->pstLinkedDescriptor;
		}

		pstTSDescriptor_t = pTSDescr_t->pstTSDescriptor;
		while (pstTSDescriptor_t != NULL)
		{
			/* Descriptors Inserted into the Second Loop(TS loop) of NIT is as follows(4 type) : */
			switch (pstTSDescriptor_t->enDescriptorID)
			{
				case SERVICE_LIST_DESCR_ID:
				{
					T_DESC_SERVICE_LIST stDescSVCLIST;
					int iAllocMemSize;
					char serviceName[10], ts_name[10];

					/* Set the descriptor information. */
					stDescSVCLIST.ucTableID = E_TABLE_ID_NIT;
					stDescSVCLIST.service_list_count = pstTSDescriptor_t->unDesc.stSLD.ucSvcListCount;
					stDescSVCLIST.pstSvcList = NULL;

					/* Set the array of service list. */
					if (stDescSVCLIST.service_list_count > 0)
					{
						iAllocMemSize = stDescSVCLIST.service_list_count * sizeof(T_SERVICE_LIST);
						stDescSVCLIST.pstSvcList = (T_SERVICE_LIST *)tcc_mw_malloc(__FUNCTION__, __LINE__, iAllocMemSize);
						if (stDescSVCLIST.pstSvcList == NULL)
						{
							PARSE_DBG(" NIT_PARSING][%d] Failed in alloc the memory of service list!\n", __LINE__);
							break;
						}

						total_service_num = stDescSVCLIST.service_list_count;
						invalid_service = 0;

						for (i = 0; i < stDescSVCLIST.service_list_count; i++)
						{
							stDescSVCLIST.pstSvcList[i].service_id	 = pstTSDescriptor_t->unDesc.stSLD.pastSvcList[i].uiServiceID;
							stDescSVCLIST.pstSvcList[i].service_type = pstTSDescriptor_t->unDesc.stSLD.pastSvcList[i].enSvcType;

							if (!tcc_manager_demux_is_skip_sdt_in_scan())
							{
								if (g_stISDBT_ServiceList.ucNameLen == 0)
								{
									memset(serviceName, 0x0, 10);

									// Insert control code(0x0e) forcedly for ALPHANUMERIC conversion.
									serviceName[0] = 0x0e;		//LS1
									snprintf(&serviceName[1], 9, "%d", stDescSVCLIST.pstSvcList[i].service_id);
									g_stISDBT_ServiceList.strServiceName = serviceName;
									fReqSvcNameUpdate = 1;
								}
							}

							if (g_stISDBT_ServiceList.iTSNameLen == 0)
							{
								memset(ts_name, 0x0, 10);
								ts_name[0] = 0x0e;
								snprintf(&ts_name[1], 9, "%d", stDescSVCLIST.pstSvcList[i].service_id);
								g_stISDBT_ServiceList.strTSName = ts_name;
							}

							g_stISDBT_ServiceList.enType = stDescSVCLIST.pstSvcList[i].service_type;

							g_stISDBT_ServiceList.usServiceID = stDescSVCLIST.pstSvcList[i].service_id;
							g_stISDBT_ServiceList.uiCurrentChannelNumber = uiCurrentChannelNumber;
							g_stISDBT_ServiceList.uiCurrentCountryCode = uiCurrentCountryCode;

							//service_type of 1seg stream of Brazil "TV Gazeta" is set 0x1 (Digital TV), not 0xC0 (1seg).
							//so check once more if the service is 1seg or full-seg.
							if (pstPRDescriptor_t != NULL)
							{
								usServiceNum = pstPRDescriptor_t->unDesc.stPRECEP_D.numServiceId;
								for (j = 0; j < usServiceNum; j++)
								{
									if (stDescSVCLIST.pstSvcList[i].service_id == pstPRDescriptor_t->unDesc.stPRECEP_D.servicd_id[j])
									{
										g_stISDBT_ServiceList.enType = SERVICE_DATA_1SEG;
									}
								}
							}

							LLOGD("[%s] usServiceID = %x, enType = %x NetworkID = %d \n", __func__, g_stISDBT_ServiceList.usServiceID, g_stISDBT_ServiceList.enType, g_stISDBT_ServiceList.usNetworkID);

							if (TCC_Isdbt_IsSupportProfileA())
							{
								InsertDB_ChannelTable(&g_stISDBT_ServiceList, 1);
							}

							UpdateDB_OriginalNetworID_ChannelTable(stDescSVCLIST.pstSvcList[i].service_id, uiCurrentChannelNumber, uiCurrentCountryCode, &g_stISDBT_ServiceList);

							UpdateDB_RemoconID_ChannelTable(stDescSVCLIST.pstSvcList[i].service_id,
															  uiCurrentChannelNumber,
															  uiCurrentCountryCode,
															  &g_stISDBT_ServiceList);

							UpdateDB_NetworkID_ChannelTable(stDescSVCLIST.pstSvcList[i].service_id,
															  uiCurrentChannelNumber,
															  uiCurrentCountryCode,
															  &g_stISDBT_ServiceList);

							if (fReqSvcNameUpdate)
							{
								UpdateDB_ChannelName_ChannelTable(stDescSVCLIST.pstSvcList[i].service_id,
															  uiCurrentChannelNumber,
															  uiCurrentCountryCode,
															  &g_stISDBT_ServiceList);
							}

							UpdateDB_TSName_ChannelTable(stDescSVCLIST.pstSvcList[i].service_id,
															  uiCurrentChannelNumber,
															  uiCurrentCountryCode,
															  &g_stISDBT_ServiceList);

							if (g_stISDBT_ServiceList.usServiceID > 0xF9BF)
							{
								invalid_service++;
							}
							else if (g_stISDBT_ServiceList.enType != SERVICE_TYPE_FSEG &&
								     g_stISDBT_ServiceList.enType != SERVICE_TYPE_TEMP_VIDEO &&
								     g_stISDBT_ServiceList.enType != SERVICE_TYPE_1SEG)
							{
								invalid_service++;
							}
						}

						if (TCC_Isdbt_IsSupportProfileA())
						{
							if (invalid_service > 0 && total_service_num > 0)
							{
								total_service_num  = total_service_num - invalid_service;
								invalid_service = 0;
							}

							tcc_manager_demux_set_service_num(total_service_num);
							tcc_manager_demux_detect_service_num(tcc_manager_demux_get_service_num(), g_stISDBT_ServiceList.usNetworkID);
						}

						if(stDescSVCLIST.pstSvcList)
						{
							tcc_mw_free(__FUNCTION__, __LINE__, stDescSVCLIST.pstSvcList);    // Noah, 20180611, Memory Leak
							stDescSVCLIST.pstSvcList = NULL;
						}
					}

					if (ISDBT_Set_DescriptorInfo(E_DESC_SERVICE_LIST, (void *)&stDescSVCLIST) != ISDBT_SUCCESS)
					{
						PARSE_DBG(" NIT_PARSING][%d] Failed in setting a descriptor info!\n", __LINE__);
					}
				}
				break;

				case ISDBT_TERRESTRIAL_DS_DESCR_ID:
				{
					unsigned int areaCode, guardInterval, transmissionMode;

					areaCode = pstTSDescriptor_t->unDesc.stITDSD.area_code;
					guardInterval = pstTSDescriptor_t->unDesc.stITDSD.guard_interval;
					transmissionMode = pstTSDescriptor_t->unDesc.stITDSD.transmission_mode;

					PARSE_DBG("[NIT Info] areaCode:%d, guardInterval:%d, mode:%d\n", areaCode, guardInterval, transmissionMode);

					if (ISDBT_TSPARSE_GetRegisteredAreaCode() == 0 && areaCode != 0)
					{
						ISDBT_TSPARSE_SetRegisteredAreaCode((unsigned short)areaCode);
					}

					PARSE_DBG("[Area] Registered : %d\n", ISDBT_TSPARSE_GetRegisteredAreaCode());

					/* To check the change status of area */
					if (ISDBT_TSPARSE_IsAreaCodeRegistered())
					{
						/* In case that Registered area code already exists */
						if (ISDBT_TSPARSE_GetRegisteredAreaCode() != areaCode)
						{
							PARSE_DBG("[NIT Info] Area Code is Changed !!: %d -> %d\n", ISDBT_TSPARSE_GetRegisteredAreaCode(), areaCode);

							ISDBT_TSPARSE_SetCurrAreaCode(areaCode);
							ISDBT_TSPARSE_SetAreaChangeStatus(TRUE);
						}
						else
						{
							ISDBT_TSPARSE_SetAreaChangeStatus(FALSE);
						}
					}
					else
					{
						/* In case of the first NIT parsing of the current channel */
						ISDBT_TSPARSE_SetRegisteredAreaCode(areaCode);
						ISDBT_TSPARSE_SetAreaChangeStatus(FALSE);
					}

					if(ISDBT_Set_DescriptorInfo(E_DESC_TERRESTRIAL_INFO, (void *)&pstTSDescriptor_t->unDesc.stITDSD) == ISDBT_SUCCESS)
					{
						UpdateDB_Frequency_ChannelTable(uiCurrentChannelNumber, uiCurrentCountryCode, &g_stISDBT_ServiceList);
					}

					UpdateDB_AreaCode_ChannelTable(uiCurrentChannelNumber, uiCurrentCountryCode, areaCode);
				}
				break;

				case PARTIAL_RECEPTION_DESCR_ID:
					break;

				case TS_INFO_DESCR_ID:
					break;

				default:
					PARSE_DBG(" NIT_PARSING] Detect unexpected descriptor : 0x%02x\n", pstTSDescriptor_t->enDescriptorID);
					break;
			}
			pstTSDescriptor_t = pstTSDescriptor_t->pstLinkedDescriptor;
		}

		if(TCC_Isdbt_IsSupportPrimaryService())
		{
			//////////////////////////////////////////////////////////////////////////////////////////
			// Noah / 20170522 / Added for IM478A-31 (Primary Service)
			// Refer to  "ARIB TR-B14"
			//     "Table 30-4-3-16 Rules for Reception Processing for TS Information Descriptor"
			//     "Also judge that the first service is the primary service of the relative layer."
			// If it processes at above TS_INFO_DESCR_ID, 'UpdateDB_PrimaryServiceFlag_ChannelTable' returns fail.

			pstTSDescriptor_t = pTSDescr_t->pstTSDescriptor;
			while (pstTSDescriptor_t != NULL)
			{
				if (pstTSDescriptor_t->enDescriptorID == TS_INFO_DESCR_ID)
				{
					int j = 0, k = 0;

					TRANSMISSION_TYPE_LOOP* pTransmissionTypeLoop_temp = NULL;
					SERVICE_ID_LOOP* pServiceIdLoop_temp = NULL;

					pTransmissionTypeLoop_temp = pstTSDescriptor_t->unDesc.stTSID.pTransmissionTypeLoop;

					for(j = 0; j < pstTSDescriptor_t->unDesc.stTSID.transmission_type_count; j++)
					{
						unsigned char transmission_type_info = 0;
						unsigned char num_of_service = 0;
						unsigned short service_id = 0;

						transmission_type_info = pTransmissionTypeLoop_temp->transmission_type_info;
						num_of_service = pTransmissionTypeLoop_temp->num_of_service;

						pServiceIdLoop_temp = pTransmissionTypeLoop_temp->pServiceIdLoop;

						if(num_of_service)
						{
							service_id = pServiceIdLoop_temp->service_id;	 // 1st service_id is a primary service.

							if((transmission_type_info & 0xC0) == 0x00)    // Type a (12 seg)
							{
								UpdateDB_PrimaryServiceFlag_ChannelTable(service_id,
																		uiCurrentChannelNumber,
																		uiCurrentCountryCode,
																		1);
							}
							else if((transmission_type_info & 0xC0) == 0x80)	// Type c (1 seg)
							{
								UpdateDB_PrimaryServiceFlag_ChannelTable(service_id,
																		uiCurrentChannelNumber,
																		uiCurrentCountryCode,
																		2);
							}
							else
							{
								/*
									(transmission_type_info & 0xC0) == 0x00    // Type a (12 seg)
									(transmission_type_info & 0xC0) == 0x40    // Type b
									(transmission_type_info & 0xC0) == 0x80    // Type c (1 seg)
									(transmission_type_info & 0xC0) == 0xC0    // Reserved
								*/
								PARSE_DBG("[%s] transmission_type_info(0x%x) has strange value \n", __func__, transmission_type_info);
							}
						}

						pTransmissionTypeLoop_temp = pTransmissionTypeLoop_temp->pNext;
					}
				}

				pstTSDescriptor_t = pstTSDescriptor_t->pstLinkedDescriptor;
			}

			// Noah / 20170522 / Added for IM478A-31 (Primary Service)
			//////////////////////////////////////////////////////////////////////////////////////////
		}

		pTSDescr_t = pTSDescr_t->pstTStreamDescriptor;
	}

	return 1;
}

int ISDBT_TSPARSE_ProcessSDT(int fFirstSection, int fDone, SDT_STRUCT *pstSDT, SI_MGMT_SECTION *pSection)
{
	DESCRIPTOR_STRUCT *pstDS = NULL;
	unsigned int return_size = 0;
	unsigned int uiCurrentChannelNumber = gCurrentChannel;
	unsigned int uiCurrentCountryCode = gCurrentCountryCode;
	int fInsert;

	/*  SDT - Service Value */
	SDT_SERVICE_DATA *pstSDTService = NULL;

	unsigned char ucSDTDescBitMap = 0x0;

	/***************************************************************************
	* SDT SCAN START... This will be removed if the home channel carries fully *
	*                   DVB compliant stream.                                  *
	***************************************************************************/
	/* ---- management for section informatin start ----*/
	SI_MGMT_SECTIONLOOP *pSecLoop= (SI_MGMT_SECTIONLOOP*)NULL;
	SI_MGMT_SECTIONLOOP_DATA unSecLoopData;
	DESCRIPTOR_STRUCT *pDesc = (DESCRIPTOR_STRUCT*)NULL;
	int status;

	if (CHK_VALIDPTR(pSection) && CHK_VALIDPTR(pstSDT)) {
		/* SDT have no global descriptor */
		/* check section loop */
		pstSDTService = pstSDT->pstServiceData;
		while (CHK_VALIDPTR(pstSDTService)) {
			pSecLoop = (SI_MGMT_SECTIONLOOP*)NULL;
			unSecLoopData.sdt_loop.service_id = pstSDTService->uiServiceID;
			unSecLoopData.sdt_loop.H_EIT_flag = pstSDTService->H_EIT_flag;
			unSecLoopData.sdt_loop.M_EIT_flag = pstSDTService->M_EIT_flag;
			unSecLoopData.sdt_loop.L_EIT_flag = pstSDTService->L_EIT_flag;
			unSecLoopData.sdt_loop.EIT_schedule_flag = pstSDTService->fEIT_Schedule;
			unSecLoopData.sdt_loop.EIT_present_following_flag = pstSDTService->fEIT_PresentFollowing;
			unSecLoopData.sdt_loop.running_status = pstSDTService->enRunningStatus;
			unSecLoopData.sdt_loop.free_CA_mode = pstSDTService->fCA_Mode_free;
			status = TSPARSE_ISDBT_SiMgmt_Section_FindLoop (&pSecLoop, &unSecLoopData, pSection);
			if (!(status==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pSecLoop))) {
				status = TSPARSE_ISDBT_SiMgmt_Section_AddLoop(&pSecLoop, &unSecLoopData, pSection);
				if (status != TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pSecLoop)) {
					pSecLoop = (SI_MGMT_SECTIONLOOP*)NULL;
					SIMGMT_DEBUG("[ProcessSDT]Section_AddLoop (service_id=0x%x) fail(%d)", pstSDTService->uiServiceID, status);
				} else
					SIMGMT_DEBUG("[ProcessSDT]Section_AddLoop (service_id=0x%x) success", pstSDTService->uiServiceID);
			} else {
				/* there is already a loop. do nothing */
			}
			/* add local descriptor to section loop */
			if (CHK_VALIDPTR(pSecLoop)) {
				if (pstSDTService->uiSvcDescrCount > 0) {
					pstDS = pstSDTService->pstSvcDescr;
					while (CHK_VALIDPTR(pstDS))
					{
						pDesc = (DESCRIPTOR_STRUCT*)NULL;
						status = TSPARSE_ISDBT_SiMgmt_SectionLoop_FindDescriptor(&pDesc, pstDS->enDescriptorID, NULL, pSecLoop);
						if (!(status==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pDesc))) {
							status = TSPARSE_ISDBT_SiMgmt_SectionLoop_AddDescriptor(&pDesc, pstDS, pSecLoop);
							if (status!=TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pDesc))
								SIMGMT_DEBUG("[ProcessSDT]SectionLoop_AddDescriptor(0x%x) fail(%d), size=%d", pstDS->enDescriptorID, status, pstDS->struct_length);
							else
								SIMGMT_DEBUG("[ProcessSDT]SectionLoop_AddDescriptor(0x%x) success", pstDS->enDescriptorID);
						}
						pstDS = pstDS->pstLinkedDescriptor;
					}
				}
			}
			pstSDTService = pstSDTService->pstServiceData;
		}
	}
	/* ---- management for section informatin end ----*/

	pstSDTService = pstSDT->pstServiceData;
	while (pstSDTService != NULL)
	{
		if ((pstSDTService->uiServiceID == 0x00) || (pstSDTService->uiServiceID == 0xFF))	/* If service_id is reserved */
		{
			pstSDTService = pstSDTService->pstServiceData;
			continue;
		}

		/***********************************************/
		/* Searching (availability Descriptor) - Start */
		pstDS = pstSDTService->pstSvcDescr;
		while ((pstDS != NULL) && (pstDS->enDescriptorID != SERVICE_DESCR_ID))
		{
			pstDS = pstDS->pstLinkedDescriptor;
		}

		if (pstDS == NULL)
		{
			pstSDTService = pstSDTService->pstServiceData;
			continue;
		}
		/* (Add) Struct - End */
		/********************/

		/* Check the descriptor is present in the SDT or not */
	#if 0
		pstDS = pstSDTService->pstSvcDescr;
		{
			g_stISDBT_ServiceList.uiTStreamID = pstSDT->uiTS_ID;
			g_stISDBT_ServiceList.usOrig_Net_ID = pstSDT->uiOrigNetID;
			g_stISDBT_ServiceList.ucSDTVersionNum = pstSDT->ucVersionNumber;
			g_stISDBT_ServiceList.stFlags.fCA_Mode_free = pstSDTService->fCA_Mode_free;
			g_stISDBT_ServiceList.stFlags.fEIT_Schedule = pstSDTService->fEIT_Schedule;
			g_stISDBT_ServiceList.stFlags.fEIT_PresentFollowing = pstSDTService->fEIT_PresentFollowing;

			UpdateDB_OriginalNetworID_ChannelTable(pstSDTService->uiServiceID, uiCurrentChannelNumber, uiCurrentCountryCode, &g_stISDBT_ServiceList);
		}
	#else
		g_stISDBT_ServiceList.usServiceID = pstSDTService->uiServiceID;

		g_stISDBT_ServiceList.uiTStreamID = pstSDT->uiTS_ID;
		g_stISDBT_ServiceList.usOrig_Net_ID = pstSDT->uiOrigNetID;
		g_stISDBT_ServiceList.ucSDTVersionNum = pstSDT->ucVersionNumber;
		g_stISDBT_ServiceList.stFlags.fCA_Mode_free = pstSDTService->fCA_Mode_free;
		g_stISDBT_ServiceList.stFlags.fEIT_Schedule = pstSDTService->fEIT_Schedule;
		g_stISDBT_ServiceList.stFlags.fEIT_PresentFollowing = pstSDTService->fEIT_PresentFollowing;

		g_stISDBT_ServiceList.uiCurrentChannelNumber = uiCurrentChannelNumber;
		g_stISDBT_ServiceList.uiCurrentCountryCode = uiCurrentCountryCode;

		pstDS = pstSDTService->pstSvcDescr;
		while (pstDS != NULL) {
			if (pstDS->enDescriptorID == SERVICE_DESCR_ID) {
				/* service_descriptor is mandatory.*/
				g_stISDBT_ServiceList.enType = pstDS->unDesc.stSD.enSvcType;
				g_stISDBT_ServiceList.logo_id = -1;
				fInsert = 0;
				if (TCC_Isdbt_IsSupportProfileA()) fInsert = 1;
				else {
					/* insert a service into db only when it is a partial reception service */
					if (tcc_demux_parse_servicetype(g_stISDBT_ServiceList.usServiceID)==3)
						fInsert = 1;
				}
				if (fInsert) {
					InsertDB_ChannelTable(&g_stISDBT_ServiceList, 0);
					UpdateDB_OriginalNetworID_ChannelTable(pstSDTService->uiServiceID, uiCurrentChannelNumber, uiCurrentCountryCode, &g_stISDBT_ServiceList);

					if (pstDS->unDesc.stSD.ucSvcNameLen > 0) {
						g_stISDBT_ServiceList.ucNameLen = pstDS->unDesc.stSD.ucSvcNameLen;
						g_stISDBT_ServiceList.strServiceName = pstDS->unDesc.stSD.pszSvcName;
						UpdateDB_ChannelName_ChannelTable(pstSDTService->uiServiceID, uiCurrentChannelNumber, uiCurrentCountryCode, &g_stISDBT_ServiceList);
					}
				}
				break;
			}
			pstDS = pstDS->pstLinkedDescriptor;
		}
	#endif

		pstDS = pstSDTService->pstSvcDescr;
		while (pstDS != NULL)
		{
			switch (pstDS->enDescriptorID)
			{
			#if 0
				case SERVICE_DESCR_ID:
				{
					if (pstDS->unDesc.stSD.ucSvcNameLen > 0)
					{
						g_stISDBT_ServiceList.ucNameLen = pstDS->unDesc.stSD.ucSvcNameLen;
						g_stISDBT_ServiceList.strServiceName = pstDS->unDesc.stSD.pszSvcName;
						g_stISDBT_ServiceList.enType = pstDS->unDesc.stSD.enSvcType;
						g_stISDBT_ServiceList.logo_id = -1;

						//service_type of 1seg stream of Brazil "TV Gazeta" is set 0x1 (Digital TV), not 0xC0 (1seg).
						//so check once more if the service is 1seg or full-seg.
						if(UpdateDB_Channel_Is1SegService(uiCurrentChannelNumber, uiCurrentCountryCode, (unsigned int)(pstSDTService->uiServiceID)))
							g_stISDBT_ServiceList.enType = 0xC0;
						UpdateDB_ChannelName_ChannelTable(pstSDTService->uiServiceID, uiCurrentChannelNumber, uiCurrentCountryCode, &g_stISDBT_ServiceList);
					}
					break;
				}
			#endif

				case CA_IDENTIFIER_DESCR_ID:
					break;

				/* Note: service_descriptor should be parsed before processing this descriptor. 				*/
				/*          Broadcasting station will locate the service_descriptor at first into service loop of SDT. 	*/
				/*		However, if ther is a case of any other descriptors being located before service_descriptor, */
				/*			routine of parsing descriptors of SDT should be modified such that service_descriptor will be parsed at first. */
				case LOGO_TRANSMISSION_DESCR_ID:
				{
					T_DESC_LTD lDescLTD = {0, 0, 0, 0, 0, NULL};
					T_DESC_LTD lDescLTD_T;
					int LTD_T_Status;

					g_stISDBT_ServiceList.logo_id = -1;
					lDescLTD.ucTableID = E_TABLE_ID_SDT;
					switch (pstDS->unDesc.stLTD.logo_transmission_type)
					{
						case 1:
							lDescLTD.logo_transmission_type = pstDS->unDesc.stLTD.logo_transmission_type;
							lDescLTD.logo_id = pstDS->unDesc.stLTD.logo_id;
							lDescLTD.logo_version = pstDS->unDesc.stLTD.logo_version;
							lDescLTD.download_data_id = pstDS->unDesc.stLTD.download_data_id;
							lDescLTD.logo_char = NULL;
							ISDBT_Set_DescriptorInfo (E_DESC_LOGO_TRANSMISSION_INFO, &lDescLTD);
							break;
						case 2:
							lDescLTD.logo_transmission_type = pstDS->unDesc.stLTD.logo_transmission_type;
							lDescLTD.logo_id = pstDS->unDesc.stLTD.logo_id;
							lDescLTD.logo_version = 0;
							lDescLTD.download_data_id = 0;
							lDescLTD.logo_char = NULL;
							LTD_T_Status = ISDBT_Get_DescriptorInfo (E_DESC_LOGO_TRANSMISSION_INFO, &lDescLTD_T);
							if (LTD_T_Status == ISDBT_SUCCESS) {
								lDescLTD.download_data_id = lDescLTD_T.download_data_id;
								lDescLTD.logo_version = lDescLTD_T.logo_version;
							}
							break;
						case 3:
							lDescLTD.logo_transmission_type = pstDS->unDesc.stLTD.logo_transmission_type;
							lDescLTD.logo_id = 0;
							lDescLTD.logo_version = 0;
							lDescLTD.download_data_id = 0;
							lDescLTD.logo_char = pstDS->unDesc.stLTD.simple_logo;
							break;
						default:
							break;
					}
					g_stISDBT_ServiceList.logo_id 			= lDescLTD.logo_id;
					g_stISDBT_ServiceList.download_data_id	= lDescLTD.download_data_id;
					g_stISDBT_ServiceList.logo_char 		= lDescLTD.logo_char;
					g_stISDBT_ServiceList.logo_transmission_type = lDescLTD.logo_transmission_type;
					if (g_stISDBT_ServiceList.logo_id != -1) {
						UpdateDB_Channel_LogoInfo (pstSDTService->uiServiceID, uiCurrentChannelNumber,
												uiCurrentCountryCode, &g_stISDBT_ServiceList);
					}
					break;
				}
				case DIGITALCOPY_CONTROL_ID:
				{
					T_DESC_DCC	stDescDCC;

					/* Set the descriptor information. */
					stDescDCC.ucTableID                      = E_TABLE_ID_SDT;
					stDescDCC.digital_recording_control_data = pstDS->unDesc.stDCCD.digital_recording_control_data;
					stDescDCC.maximum_bitrate_flag			 = pstDS->unDesc.stDCCD.maximum_bitrate_flag;
					stDescDCC.component_control_flag         = pstDS->unDesc.stDCCD.component_control_flag;
					stDescDCC.copy_control_type              = pstDS->unDesc.stDCCD.copy_control_type;
					stDescDCC.APS_control_data               = pstDS->unDesc.stDCCD.APS_control_data;
					stDescDCC.maximum_bitrate				 = pstDS->unDesc.stDCCD.maximum_bitrate;

					stDescDCC.sub_info = 0;
					if( pstDS->unDesc.stDCCD.sub_info )
					{
						SUB_T_DESC_DCC* stSubDescDCC;
						SUB_T_DESC_DCC* backup_sub_info;
						SUB_DIGITALCOPY_CONTROL_DESC* sub_info = pstDS->unDesc.stDCCD.sub_info;

						while(sub_info)
						{
							stSubDescDCC = (SUB_T_DESC_DCC*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(SUB_T_DESC_DCC));
							if(0 == stSubDescDCC)
							{
								ALOGE("[%s][%d] : SUB_T_DESC_DCC malloc fail !!!!!\n", __func__, __LINE__);
								return -1;
							}
							memset(stSubDescDCC, 0, sizeof(SUB_T_DESC_DCC));

							stSubDescDCC->component_tag 				 = sub_info->component_tag;
							stSubDescDCC->digital_recording_control_data = sub_info->digital_recording_control_data;
							stSubDescDCC->maximum_bitrate_flag			 = sub_info->maximum_bitrate_flag;
							stSubDescDCC->copy_control_type 			 = sub_info->copy_control_type;
							stSubDescDCC->APS_control_data				 = sub_info->APS_control_data;
							stSubDescDCC->maximum_bitrate				 = sub_info->maximum_bitrate;

							if(0 == stDescDCC.sub_info)
							{
								stDescDCC.sub_info = stSubDescDCC;
								backup_sub_info = stSubDescDCC;
							}
							else
							{
								backup_sub_info->pNext = stSubDescDCC;
								backup_sub_info = stSubDescDCC;
							}

							sub_info = sub_info->pNext;
						}
					}

					if (ISDBT_TSPARSE_CURDESCINFO_set(pstSDTService->uiServiceID, DIGITALCOPY_CONTROL_ID, (void *)&stDescDCC)) {
						ALOGE("---> [E_TABLE_ID_SDT] Failed in setting a current descriptor(0x%x) info!\n", pstDS->enDescriptorID);
					}

					if(stDescDCC.sub_info)
					{
						SUB_T_DESC_DCC* sub_info = stDescDCC.sub_info;
						while(sub_info)
						{
							SUB_T_DESC_DCC* temp_sub_info = sub_info->pNext;
							tcc_mw_free(__FUNCTION__, __LINE__, sub_info);
							sub_info = temp_sub_info;
						}
					}
					stDescDCC.sub_info = 0;

					ucSDTDescBitMap |= 0x1;
					break;
				}
				case CONTENT_AVAILABILITY_ID:
				{
					T_DESC_CONTA	stDescCONTA;

					/* Set the descriptor information. */
					stDescCONTA.ucTableID				= E_TABLE_ID_SDT;
					stDescCONTA.copy_restriction_mode   = pstDS->unDesc.stCONTAD.copy_restriction_mode;
					stDescCONTA.image_constraint_token	= pstDS->unDesc.stCONTAD.image_constraint_token;
					stDescCONTA.retention_mode			= pstDS->unDesc.stCONTAD.retention_mode;
					stDescCONTA.retention_state			= pstDS->unDesc.stCONTAD.retention_state;
					stDescCONTA.encryption_mode			= pstDS->unDesc.stCONTAD.encryption_mode;

					if (ISDBT_TSPARSE_CURDESCINFO_set(pstSDTService->uiServiceID, CONTENT_AVAILABILITY_ID, (void *)&stDescCONTA)) {
						ALOGE("---> [E_TABLE_ID_SDT] Failed in setting a current descriptor(0x%x) info!\n", pstDS->enDescriptorID);
					}
					ucSDTDescBitMap |= 0x2;
					break;
				}
				default:
					break;
			}	/*switch (pstDS->enDescriptorID) */

			/* Next descriptor */
			pstDS = pstDS->pstLinkedDescriptor;

		}	/*while(pstDS != NULL) */
		/* (Add) Data - End */
		/********************/
		ISDBT_TSPARSE_CURDESCINFO_setBitMap(pstSDTService->uiServiceID, E_TABLE_ID_SDT, ucSDTDescBitMap);
		ucSDTDescBitMap = 0x0;
		pstSDTService = pstSDTService->pstServiceData;
	}	/*while(pstDS != NULL) */
	/* Service Linked List Make - End */
	/**********************************/
	return TRUE;
}

int  ISDBT_TSPARSE_ProcessEIT(int fFirstSection, int fDone, P_EIT_STRUCT pstEIT, SI_MGMT_SECTION *pSection)
{
	P_EIT_EVENT_DATA pstEventData_t = { 0, };
	DESCRIPTOR_STRUCT *pstDS = NULL;

	unsigned int uiCurrentChannelNumber = gCurrentChannel;
	unsigned int uiCurrentCountryCode = gCurrentCountryCode;
	unsigned char bNewData = FALSE;
	int iTableType = 0;
	int totalEvtCnt = 0;
	int totalDescCnt = 0;
	T_ISDBT_EIT_STRUCT stDxBEIT = { 0, };
	T_ISDBT_EIT_EVENT_DATA stDxBEITEvent = { 0, };
	SUBTITLE_LANGUAGE_INFO stDxBSubtLangInfo = { 0, };

	unsigned char ucEITDescBitMap = 0x0;

	/* ---- management for section informatin start ----*/
	SI_MGMT_SECTIONLOOP *pSecLoop= (SI_MGMT_SECTIONLOOP*)NULL;
	SI_MGMT_SECTIONLOOP_DATA unSecLoopData;
	DESCRIPTOR_STRUCT *pDesc = (DESCRIPTOR_STRUCT*)NULL;
	int status;

	if (CHK_VALIDPTR(pSection) && CHK_VALIDPTR(pstEIT)
	&&	pstEIT->enTableType == EIT_PF_A_ID) {
		pstEventData_t = pstEIT->pstEventData;
		while (CHK_VALIDPTR(pstEventData_t)) {
			pSecLoop = (SI_MGMT_SECTIONLOOP*)NULL;
			unSecLoopData.eit_loop.event_id = pstEventData_t->uiEventID;
			unSecLoopData.eit_loop.stStartTime = pstEventData_t->stStartTime;
			unSecLoopData.eit_loop.stDuration = pstEventData_t->stDuration;
			unSecLoopData.eit_loop.running_status = pstEventData_t->enRunningStatus;
			unSecLoopData.eit_loop.free_CA_mode = pstEventData_t->fCA_Mode_free;
			status = TSPARSE_ISDBT_SiMgmt_Section_FindLoop (&pSecLoop, &unSecLoopData, pSection);
			if (!(status==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pSecLoop))) {
				status = TSPARSE_ISDBT_SiMgmt_Section_AddLoop(&pSecLoop, &unSecLoopData, pSection);
				if (status != TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pSecLoop)) {
					pSecLoop = (SI_MGMT_SECTIONLOOP*)NULL;
					SIMGMT_DEBUG("[ProcessEIT]Section_AddLoop (event_id=0x%x) fail(%d)", pstEventData_t->uiEventID, status);
				} else
					SIMGMT_DEBUG("[ProcessEIT]Section_AddLoop (event_id=0x%x) success", pstEventData_t->uiEventID);
			} else {
				/* there is already a loop. do nothing */
			}
			/* add local descriptor to section loop */
			if (CHK_VALIDPTR(pSecLoop)) {
				if (pstEventData_t->wEvtDescrCount > 0) {
					pstDS = pstEventData_t->pstEvtDescr;
					while (CHK_VALIDPTR(pstDS))
					{
						pDesc = (DESCRIPTOR_STRUCT*)NULL;
						if (pstDS->enDescriptorID == EXTN_EVT_DESCR_ID)
							status = TSPARSE_ISDBT_SiMgmt_SectionLoop_FindDescriptor(&pDesc, pstDS->enDescriptorID, (void*)&pstDS->unDesc.stEED.ucDescrNumber, pSecLoop);
						else if (pstDS->enDescriptorID == DATA_CONTENT_DESCR_ID)
							status = TSPARSE_ISDBT_SiMgmt_SectionLoop_FindDescriptor(&pDesc, pstDS->enDescriptorID, (void*)&pstDS->unDesc.stDCD.data_component_id, pSecLoop);
						else
							status = TSPARSE_ISDBT_SiMgmt_SectionLoop_FindDescriptor(&pDesc, pstDS->enDescriptorID, NULL, pSecLoop);
						if (!(status==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pDesc))) {
							status = TSPARSE_ISDBT_SiMgmt_SectionLoop_AddDescriptor(&pDesc, pstDS, pSecLoop);
							if (status!=TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pDesc))
								SIMGMT_DEBUG("[ProcessEIT]SectionLoop_AddDescriptor(0x%x) fail(%d), size=%d", pstDS->enDescriptorID, status, pstDS->struct_length);
							else
								SIMGMT_DEBUG("[ProcessEIT]SectionLoop_AddDescriptor(0x%x) success", pstDS->enDescriptorID);
						}
						pstDS = pstDS->pstLinkedDescriptor;
					}
				}
			}
			pstEventData_t = pstEventData_t->pstEventData;
		}
	}
	/* ---- management for section informatin end ----*/

	if (pstEIT == NULL)
	{
		/* Impossible */
		PARSE_DBG("\n[ProcessEIT] Error - Data Fail");
		return -1;
	}

	stDxBEIT.uiTableID = pstEIT->enTableType;
	stDxBEIT.ucLastSection = pstEIT->ucLastSection;
	stDxBEIT.ucSection = pstEIT->ucSection;
	stDxBEIT.ucSegmentLastSection = pstEIT->ucSegmentLastSection;
	stDxBEIT.ucVersionNumber = pstEIT->ucVersionNumber;
	stDxBEIT.uiOrgNetWorkID = pstEIT->uiOrgNetWorkID;
	stDxBEIT.uiService_ID = pstEIT->uiService_ID;
	stDxBEIT.uiTStreamID = pstEIT->uiTStreamID;

	pstEventData_t = pstEIT->pstEventData;

	if (pstEventData_t == NULL)
	{
		SendEITUpdateEvent(stDxBEIT.uiService_ID, stDxBEIT.uiTableID, stDxBEIT.ucVersionNumber);

		/* There are no events for this service */
		PARSE_DBG("\n[ProcessEIT] Error - No event data !!!");
		return -1;
	}

	while (pstEventData_t != NULL)
	{
		stDxBEITEvent.uiEventID = pstEventData_t->uiEventID;
		if(	TCC_Isdbt_IsSupportEventRelay()
		&&  (pstEIT->enTableType == EIT_PF_A_ID)
		&& 	(pstEIT->ucSection == 0x0)
		&&	(stDxBEIT.uiService_ID == gPlayingServiceID))
		{
			T_ISDBT_EIT_EVENT_GROUP stEGD = { 0, };
			ISDBT_TSPARSE_CURDESCINFO_get(stDxBEIT.uiService_ID, EVT_GROUP_DESCR_ID, (void *)&stEGD);
			if((stDxBEITEvent.uiEventID != stEGD.eventID)
			&& (stEGD.eventID != 0x0)
			&& (	stEGD.group_type == 0x2
				||	stEGD.group_type == 0x3
				||	stEGD.group_type == 0x4
				||	stEGD.group_type == 0x5)) {
				tcc_manager_demux_event_relay(stEGD.service_id, stEGD.event_id, stEGD.original_network_id, stEGD.transport_stream_id);
			}
		}

		memcpy(&(stDxBEITEvent.stDuration), &(pstEventData_t->stDuration), sizeof(pstEventData_t->stDuration));
		memcpy(&(stDxBEITEvent.stStartTime), &(pstEventData_t->stStartTime), sizeof(pstEventData_t->stStartTime));

		/* in case of EWS Event, set all field of start time to 0xff */
		if((stDxBEITEvent.stStartTime.stTime.ucHour == 0xa5) &&
			(stDxBEITEvent.stStartTime.stTime.ucMinute== 0xa5) &&
			(stDxBEITEvent.stStartTime.stTime.ucSecond== 0xa5))
		{
			//PRINTF("[EIT] EWS detected !!\n");
			stDxBEITEvent.stStartTime.stTime.ucHour = 0xff;
			stDxBEITEvent.stStartTime.stTime.ucMinute = 0xff;
		}
		else
		{
			/* Check Validation of Time Information Value */
			if((stDxBEITEvent.stStartTime.stTime.ucHour > 24) ||
				(stDxBEITEvent.stStartTime.stTime.ucMinute > 60))
			{
				PARSE_DBG(" EIT_PARSING] Invalid Time schedule Information - Start Time is %02d:%02d\n",
											stDxBEITEvent.stStartTime.stTime.ucHour,
											stDxBEITEvent.stStartTime.stTime.ucMinute);
				pstEventData_t = pstEventData_t->pstEventData;
				continue;
			}
		}

		/* in case of EWS Event, set all field of end time to 0xff */
		if((stDxBEITEvent.stDuration.ucHour == 0xa5) &&
			(stDxBEITEvent.stDuration.ucMinute== 0xa5) &&
			(stDxBEITEvent.stDuration.ucSecond== 0xa5))
		{
			//PRINTF("[EIT] EWS detected !!\n");
			stDxBEITEvent.stDuration.ucHour = 0xff;
			stDxBEITEvent.stDuration.ucMinute = 0xff;
		}

		if (pstEIT->enTableType == EIT_PF_A_ID)
		{
			iTableType = EPG_PF;
		}
		else
		{
			iTableType = EPG_SCHEDULE;
		}

		if (GetCountDB_EPG_Table(iTableType, uiCurrentChannelNumber, uiCurrentCountryCode, stDxBEITEvent.uiEventID, &stDxBEIT) == 0)
		{
			if (InsertDB_EPG_Table(iTableType, uiCurrentChannelNumber, uiCurrentCountryCode, &stDxBEIT, &stDxBEITEvent) == TRUE)
			{
				bNewData = TRUE;
			}
		}
		else
		{
			bNewData = TRUE;
		}

		if (bNewData)
		{
			pstDS = pstEventData_t->pstEvtDescr;
			totalDescCnt = 0;
			while (pstDS != NULL)
			{
				switch(pstDS->enDescriptorID)
				{
				case SHRT_EVT_DESCR_ID:
				case CONTENT_DESCR_ID:
					if (pstEIT->enTableType != EIT_PF_A_ID)
					{
						totalDescCnt++;
					}
					break;

				case EXTN_EVT_DESCR_ID:
				case DATA_CONTENT_DESCR_ID:
					totalDescCnt++;
					break;

				default:
					break;
				}
				pstDS = pstDS->pstLinkedDescriptor;
			}

			pstDS = pstEventData_t->pstEvtDescr;
			while (pstDS != NULL)
			{
				switch (pstDS->enDescriptorID)
				{
				case SHRT_EVT_DESCR_ID:
					UpdateDB_EPG_XXXTable(iTableType, (int) pstDS->unDesc.stSED.ucShortEventNameLen,
										  (char *) &pstDS->unDesc.stSED.pszShortEventName[0],
										  (int) pstDS->unDesc.stSED.ucShortEventTextLen,
										  (char *) &pstDS->unDesc.stSED.pszShortEventText[0],
										  uiCurrentChannelNumber,
										  uiCurrentCountryCode,
										  &stDxBEIT, &stDxBEITEvent);
					break;

				case EXTN_EVT_DESCR_ID:
					/* Store extended event info. for current event of current actual service */

					UpdateDB_EPG_XXXTable_ExtEvent(iTableType,
													 uiCurrentChannelNumber, uiCurrentCountryCode,
												 &stDxBEIT, &stDxBEITEvent, &pstDS->unDesc.stEED);
					break;

				case CONTENT_DESCR_ID:
					if( pstDS->unDesc.stCOND.ucDataCount )
					{
						UpdateDB_EPG_Content(iTableType,
												pstDS->unDesc.stCOND.pastContentData[0].enNib_1,
												uiCurrentChannelNumber,
												uiCurrentCountryCode,
												&stDxBEIT, &stDxBEITEvent);
					}
					break;

				case PARENT_DESCR_ID:
					if( pstDS->unDesc.stPRD.ucDataCount )
					{
						T_DESC_PR	stDescPR_Local;
						unsigned char ucPRCount;

						ucPRCount = pstDS->unDesc.stPRD.ucDataCount;

						if(ucPRCount > 0)
						{
							int country_code = 0, i;

							stDescPR_Local.ucTableID	= E_TABLE_ID_EIT;
							stDescPR_Local.Rating = pstDS->unDesc.stPRD.pastParentData->ucRating;

							for(i=0 ; i<3 ; i++)
							{
								country_code = (country_code<<8)|(pstDS->unDesc.stPRD.pastParentData->acCountryCode[i]&0xFF);
							}

							if ((totalEvtCnt == 0) && (country_code == 0x425241 /* BRA */))
							{
								/*PARSE_DBG("~~~~~~ [%s] PARENT_DESCR_ID(%d - rate:0x%X, CC:0x%X)\n", \
									__func__, ucPRCount, stDescPR_Local.Rating, country_code);*/

								if (stDxBEIT.ucSection == 0)	//update parental_rating_descriptor only if it's a present section.
								{
									if (ISDBT_Set_DescriptorInfo(E_DESC_PARENT_RATING, (void *)&stDescPR_Local) != ISDBT_SUCCESS)
									{
										ALOGE("---> [E_TABLE_ID_EIT] Failed in setting a descriptor info!\n");
									}
								}
							}
						}
					}
					break;

				case EVT_GROUP_DESCR_ID:
					if (TCC_Isdbt_IsSupportEventRelay()
					&&	pstEIT->enTableType == EIT_PF_A_ID
					&& 	pstEIT->ucSection == 0x0
					&& ( pstDS->unDesc.stEGD.group_type == 0x2
					|| pstDS->unDesc.stEGD.group_type == 0x3
					|| pstDS->unDesc.stEGD.group_type == 0x4
					|| pstDS->unDesc.stEGD.group_type == 0x5 ))
					{
						T_ISDBT_EIT_EVENT_GROUP stEGD;
						stEGD.eventID					= stDxBEITEvent.uiEventID;
						stEGD.group_type				= pstDS->unDesc.stEGD.group_type;
						stEGD.event_count				= pstDS->unDesc.stEGD.event_count;
						stEGD.service_id				= pstDS->unDesc.stEGD.service_id[0];
						stEGD.event_id					= pstDS->unDesc.stEGD.event_id[0];
						if(pstDS->unDesc.stEGD.group_type == 0x4 || pstDS->unDesc.stEGD.group_type == 0x5) {
							stEGD.original_network_id	= pstDS->unDesc.stEGD.original_network_id;
							stEGD.transport_stream_id	= pstDS->unDesc.stEGD.transport_stream_id;
						}
						else {
							stEGD.original_network_id	= 0;
							stEGD.transport_stream_id	= 0;
						}
						if (ISDBT_TSPARSE_CURDESCINFO_set(stDxBEIT.uiService_ID, EVT_GROUP_DESCR_ID, (void *)&stEGD)) {
							ALOGE("---> [E_TABLE_ID_EIT] Failed in setting a current descriptor(0x%x) info!\n", pstDS->enDescriptorID);
						}
						ucEITDescBitMap |= 0x4;
					}
					else if((totalDescCnt == 0) && (pstDS->unDesc.stEGD.group_type == 0x1))
					{
						UpdateDB_EPG_XXXTable_EvtGroup(iTableType, \
														(int)pstDS->unDesc.stEGD.service_id[0], \
														(int)pstDS->unDesc.stEGD.event_id[0], \
														uiCurrentChannelNumber, \
														uiCurrentCountryCode, \
														&stDxBEIT, &stDxBEITEvent);
					}
					break;

				case AUDIO_COMPONENT_DESCR_ID:
					{
						if (pstDS->unDesc.stACD.main_component_flag) {	//if it's main component ?
							UpdateDB_EPG_AudioInfo (\
									iTableType,
									pstDS->unDesc.stACD.component_tag,
									pstDS->unDesc.stACD.stream_type,
									pstDS->unDesc.stACD.component_type,
									pstDS->unDesc.stACD.ES_multi_lingual_flag,
									pstDS->unDesc.stACD.sampling_rate,
									(unsigned char *)pstDS->unDesc.stACD.acLangCode[0],
									(unsigned char *)pstDS->unDesc.stACD.acLangCode[1],
									uiCurrentChannelNumber, uiCurrentCountryCode,
									(P_ISDBT_EIT_STRUCT)&stDxBEIT, (P_ISDBT_EIT_EVENT_DATA)&stDxBEITEvent);
						}
					}
					break;

				case COMPONENT_DESCR_ID:
					if (pstDS->unDesc.stCOMD.enStreamContent == COMP_VIDEO) {
						UpdateDB_EPG_VideoInfo (\
									iTableType,
									pstDS->unDesc.stCOMD.ucComponentTag,
									pstDS->unDesc.stCOMD.ucComponentType,
									uiCurrentChannelNumber, uiCurrentCountryCode,
									&stDxBEIT, &stDxBEITEvent);
					}
					break;

				case PRIVATE_DATA_SPECIFIER_ID:
				case TIME_S_EVT_DESCR_ID:
				case COUNTRY_AVAIL_DESCR_ID:
				case LINKAGE_DESCR_ID:
					break;
				case DATA_CONTENT_DESCR_ID:
				{
					if(pstDS->unDesc.stDCD.data_component_id == 0x0008){
						if(	pstEIT->enTableType == EIT_PF_A_ID
						&& 	pstEIT->ucSection == 0x0
						&&  stDxBEIT.uiService_ID == gPlayingServiceID){
							stDxBSubtLangInfo[eitcapcnt].ServiceID = stDxBEIT.uiService_ID;
							stDxBSubtLangInfo[eitcapcnt].NumofLang = pstDS->unDesc.stDCD.num_of_language;
							if(stDxBSubtLangInfo[eitcapcnt].NumofLang == 2){
								stDxBSubtLangInfo[eitcapcnt].SubtLangCode[0] = pstDS->unDesc.stDCD.ISO_639_language_code[0];
								stDxBSubtLangInfo[eitcapcnt].SubtLangCode[1] = pstDS->unDesc.stDCD.ISO_639_language_code[1];
							}else if(stDxBSubtLangInfo[eitcapcnt].NumofLang == 1){
								stDxBSubtLangInfo[eitcapcnt].SubtLangCode[0] = pstDS->unDesc.stDCD.ISO_639_language_code[0];
								stDxBSubtLangInfo[eitcapcnt].SubtLangCode[1] = 0x0;
							}else{
								stDxBSubtLangInfo[eitcapcnt].SubtLangCode[0] = 0x0;
								stDxBSubtLangInfo[eitcapcnt].SubtLangCode[1] = 0x0;
							}
							SendSubtitleLangInfo(stDxBSubtLangInfo[eitcapcnt].NumofLang, stDxBSubtLangInfo[eitcapcnt].SubtLangCode[0], \
												stDxBSubtLangInfo[eitcapcnt].SubtLangCode[1], stDxBSubtLangInfo[eitcapcnt].ServiceID, eitcapcnt);
							eitcapcnt++;
							if(eitcapcnt == MAX_SUPPORT)
								eitcapcnt =0;
						}
					}
					else if (pstDS->unDesc.stDCD.data_component_id == 0x000C)
					{
						if (pstEIT->enTableType == EIT_PF_A_ID && stDxBEIT.uiService_ID == gPlayingServiceID)
						{
							if (pstDS->unDesc.stDCD.selector_length)
							{
								T_DESC_BXML	stDescBXML;
								unsigned char content_id_flag = 0;
								unsigned int  content_id = 0;
								unsigned char associated_contents_flag = 0;

								/* arib_bxml_info() content_id_flag */
								if ((char)pstDS->unDesc.stDCD.selector_byte[1] & 0x10)
								{
									content_id_flag = 1;
									content_id		= (int)(pstDS->unDesc.stDCD.selector_byte[5]*0x1000000) +
													  (int)(pstDS->unDesc.stDCD.selector_byte[6]*0x10000) +
													  (int)(pstDS->unDesc.stDCD.selector_byte[7]*0x100) +
													  (int)(pstDS->unDesc.stDCD.selector_byte[8]);
								}

								/* arib_bxml_info() associated_contents_flag */
								if ((char)pstDS->unDesc.stDCD.selector_byte[1] & 0x08)
								{
									associated_contents_flag = 1;
								}

								/* Set the descriptor information. */
								stDescBXML.ucTableID				= E_TABLE_ID_EIT;
								stDescBXML.content_id_flag			= content_id_flag;
								stDescBXML.associated_content_flag	= associated_contents_flag;
								stDescBXML.content_id				= content_id;

								if (ISDBT_Set_DescriptorInfo(E_DESC_ARIB_BXML_INFO, (void*)&stDescBXML)  != ISDBT_SUCCESS) {
									ALOGE("---> [E_TABLE_ID_EIT] Failed in setting a descriptor(0x%x) info!\n", pstDS->enDescriptorID);
								}

								LLOGD("[ARIB_XML] TableType(%d), ServiceID(%d) length(%d) content_id_flag(%d) associated_contents_flag(%d), content_id(%d) \n",
										pstEIT->enTableType, stDxBEIT.uiService_ID, pstDS->unDesc.stDCD.selector_length,
										content_id_flag, associated_contents_flag, content_id);
							}
						}
					}

					break;
				}
				case DIGITALCOPY_CONTROL_ID:
				{
					if(	pstEIT->enTableType == EIT_PF_A_ID && pstEIT->ucSection == 0x0)
					{
						T_DESC_DCC	stDescDCC;

						/* Set the descriptor information. */
						stDescDCC.ucTableID                      = E_TABLE_ID_EIT;
						stDescDCC.digital_recording_control_data = pstDS->unDesc.stDCCD.digital_recording_control_data;
						stDescDCC.maximum_bitrate_flag			 = pstDS->unDesc.stDCCD.maximum_bitrate_flag;
						stDescDCC.component_control_flag         = pstDS->unDesc.stDCCD.component_control_flag;
						stDescDCC.copy_control_type              = pstDS->unDesc.stDCCD.copy_control_type;
						stDescDCC.APS_control_data               = pstDS->unDesc.stDCCD.APS_control_data;
						stDescDCC.maximum_bitrate				 = pstDS->unDesc.stDCCD.maximum_bitrate;

						stDescDCC.sub_info = 0;
						if( pstDS->unDesc.stDCCD.sub_info )
						{
							SUB_T_DESC_DCC* stSubDescDCC;
							SUB_T_DESC_DCC* backup_sub_info;
							SUB_DIGITALCOPY_CONTROL_DESC* sub_info = pstDS->unDesc.stDCCD.sub_info;

							while(sub_info)
							{
								stSubDescDCC = (SUB_T_DESC_DCC*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(SUB_T_DESC_DCC));
								if(0 == stSubDescDCC)
								{
									ALOGE("[%s][%d] : SUB_T_DESC_DCC malloc fail !!!!!\n", __func__, __LINE__);
									return -1;
								}
								memset(stSubDescDCC, 0, sizeof(SUB_T_DESC_DCC));

								stSubDescDCC->component_tag 				 = sub_info->component_tag;
								stSubDescDCC->digital_recording_control_data = sub_info->digital_recording_control_data;
								stSubDescDCC->maximum_bitrate_flag			 = sub_info->maximum_bitrate_flag;
								stSubDescDCC->copy_control_type 			 = sub_info->copy_control_type;
								stSubDescDCC->APS_control_data				 = sub_info->APS_control_data;
								stSubDescDCC->maximum_bitrate				 = sub_info->maximum_bitrate;

								if(0 == stDescDCC.sub_info)
								{
									stDescDCC.sub_info = stSubDescDCC;
									backup_sub_info = stSubDescDCC;
								}
								else
								{
									backup_sub_info->pNext = stSubDescDCC;
									backup_sub_info = stSubDescDCC;
								}

								sub_info = sub_info->pNext;
							}
						}

						if (ISDBT_TSPARSE_CURDESCINFO_set(stDxBEIT.uiService_ID, DIGITALCOPY_CONTROL_ID, (void *)&stDescDCC)) {
							ALOGE("---> [E_TABLE_ID_EIT] Failed in setting a current descriptor(0x%x) info!\n", pstDS->enDescriptorID);
						}

						if(stDescDCC.sub_info)
						{
							SUB_T_DESC_DCC* sub_info = stDescDCC.sub_info;
							while(sub_info)
							{
								SUB_T_DESC_DCC* temp_sub_info = sub_info->pNext;
								tcc_mw_free(__FUNCTION__, __LINE__, sub_info);
								sub_info = temp_sub_info;
							}
						}
						stDescDCC.sub_info = 0;

						ucEITDescBitMap |= 0x1;
					}
					break;
				}
				case CONTENT_AVAILABILITY_ID:
				{
					if(	pstEIT->enTableType == EIT_PF_A_ID
					&& 	pstEIT->ucSection == 0x0) {
						T_DESC_CONTA	stDescCONTA;

						/* Set the descriptor information. */
						stDescCONTA.ucTableID				= E_TABLE_ID_EIT;
						stDescCONTA.copy_restriction_mode   = pstDS->unDesc.stCONTAD.copy_restriction_mode;
						stDescCONTA.image_constraint_token	= pstDS->unDesc.stCONTAD.image_constraint_token;
						stDescCONTA.retention_mode			= pstDS->unDesc.stCONTAD.retention_mode;
						stDescCONTA.retention_state			= pstDS->unDesc.stCONTAD.retention_state;
						stDescCONTA.encryption_mode			= pstDS->unDesc.stCONTAD.encryption_mode;

						if (ISDBT_TSPARSE_CURDESCINFO_set(stDxBEIT.uiService_ID, CONTENT_AVAILABILITY_ID, (void *)&stDescCONTA)) {
							ALOGE("---> [E_TABLE_ID_EIT] Failed in setting a current descriptor(0x%x) info!\n", pstDS->enDescriptorID);
						}
						ucEITDescBitMap |= 0x2;
					}
					break;
				}
				case COMPONENT_GROUP_DESCR_ID:
				{
					if(	pstEIT->enTableType == EIT_PF_A_ID
					&&	pstEIT->ucSection == 0x0
					&&	gPlayingServiceID == pstEIT->uiService_ID
					&&	pstDS->unDesc.stCGD.component_group_type == 0x0) {
						int group_index, ca_unit_index, component_index, text_index;
						T_DESC_CGD stDescCGD = {0, };

						stDescCGD.component_group_type = pstDS->unDesc.stCGD.component_group_type;
						stDescCGD.total_bit_rate_flag = pstDS->unDesc.stCGD.total_bit_rate_flag;
						stDescCGD.num_of_group = pstDS->unDesc.stCGD.num_of_group;
						for(group_index=0;group_index<pstDS->unDesc.stCGD.num_of_group;group_index++) {
							stDescCGD.component_group_id[group_index] = pstDS->unDesc.stCGD.component_group_id[group_index];
							stDescCGD.num_of_CA_unit[group_index] = pstDS->unDesc.stCGD.num_of_CA_unit[group_index];
							for(ca_unit_index=0;ca_unit_index<pstDS->unDesc.stCGD.num_of_CA_unit[group_index];ca_unit_index++) {
								stDescCGD.CA_unit_id[group_index][ca_unit_index] = pstDS->unDesc.stCGD.CA_unit_id[group_index][ca_unit_index];
								stDescCGD.num_of_component[group_index][ca_unit_index] = pstDS->unDesc.stCGD.num_of_component[group_index][ca_unit_index];
								for(component_index=0;component_index<pstDS->unDesc.stCGD.num_of_component[group_index][ca_unit_index];component_index++) {
									stDescCGD.component_tag[group_index][ca_unit_index][component_index] = pstDS->unDesc.stCGD.component_tag[group_index][ca_unit_index][component_index];
								}
							}
							if(pstDS->unDesc.stCGD.total_bit_rate_flag == 0x1) {
								stDescCGD.total_bit_rate[group_index] = pstDS->unDesc.stCGD.total_bit_rate[group_index];
							}
							else {
								stDescCGD.total_bit_rate[group_index] = 0;
							}
							stDescCGD.text_length[group_index] = pstDS->unDesc.stCGD.text_length[group_index];
							for(text_index=0;text_index<pstDS->unDesc.stCGD.text_length[group_index]+2;text_index++) {
								stDescCGD.text_char[group_index][text_index] = pstDS->unDesc.stCGD.text_char[group_index][text_index];
							}
						}

						if (ISDBT_TSPARSE_CURDESCINFO_set(stDxBEIT.uiService_ID, COMPONENT_GROUP_DESCR_ID, (void *)&stDescCGD)) {
							ALOGE("---> [E_TABLE_ID_EIT] Failed in setting a current descriptor(0x%x) info!\n", pstDS->enDescriptorID);
						}
						ucEITDescBitMap |= 0x8;
					}
					break;
				}
				default:
					break;
				}	/* End "switch(pstDS->enDescriptorID)" */

				pstDS = pstDS->pstLinkedDescriptor;
			}	/* End "while (pstDS != NULL)" */

			if(pstEIT->enTableType == EIT_PF_A_ID && pstEIT->ucSection == 0x0)
			{
				ISDBT_TSPARSE_CURDESCINFO_setBitMap(stDxBEIT.uiService_ID, E_TABLE_ID_EIT, ucEITDescBitMap);
				ucEITDescBitMap = 0x0;
			}
		}

		totalEvtCnt++;

		/* Designate next Event (outer for() in the event_information_section()) */
		pstEventData_t = pstEventData_t->pstEventData;
	}

	// update events of this table
	int i;
	int *piEvtIDList = tcc_mw_malloc(__FUNCTION__, __LINE__, totalEvtCnt * sizeof(int));
	// jini 9th : Ok : Null Pointer Dereference => piEvtIDList[i] = (int)pstEventData_t->uiEventID;
	if (piEvtIDList == NULL)
	{
		ALOGE("[ProcessEIT] Error - piEvtIDList is NULL !!!");
		return -1;
	}
	memset(piEvtIDList, 0, totalEvtCnt * sizeof(int));

	pstEventData_t = pstEIT->pstEventData;
	for(i=0; i<totalEvtCnt && pstEventData_t; i++)
	{
		piEvtIDList[i] = (int)pstEventData_t->uiEventID;
		pstEventData_t = pstEventData_t->pstEventData;
	}

	UpdateDB_EPG_Commit(totalEvtCnt, piEvtIDList, uiCurrentChannelNumber, uiCurrentCountryCode, &stDxBEIT);

	SendEITUpdateEvent(stDxBEIT.uiService_ID, stDxBEIT.uiTableID, stDxBEIT.ucVersionNumber);

	// Redundant Condition : if(piEvtIDList)
	tcc_mw_free(__FUNCTION__, __LINE__, piEvtIDList);

	return 1;
}

int ISDBT_TSPARSE_ProcessBIT(int fFirstSection, int fDone, BIT_STRUCT *pstBIT, SI_MGMT_SECTION *pSection)
{
	BIT_BROADCASTER_DATA *pBCData_t = NULL;
	DESCRIPTOR_STRUCT *pstBCDescriptor_t = NULL;
	unsigned int uiCurrentChannelNumber = gCurrentChannel;
	unsigned int uiCurrentCountryCode = gCurrentCountryCode;
	unsigned short usAffiliationCnt=0;

	/* ---- management for section informatin start ----*/
	SI_MGMT_SECTIONLOOP *pSecLoop= (SI_MGMT_SECTIONLOOP*)NULL;
	SI_MGMT_SECTIONLOOP_DATA unSecLoopData;
	DESCRIPTOR_STRUCT *pDesc = (DESCRIPTOR_STRUCT*)NULL;
	int status;

	if (CHK_VALIDPTR(pSection) && CHK_VALIDPTR(pstBIT)) {
		/* check global descriptor */
		if (pstBIT->uiFDescrCount) {
			pstBCDescriptor_t = pstBIT->pstFDescr;
			while (CHK_VALIDPTR(pstBCDescriptor_t)) {
				pDesc = (DESCRIPTOR_STRUCT*)NULL;
				status = TSPARSE_ISDBT_SiMgmt_Section_FindDescriptor_Global(&pDesc, pstBCDescriptor_t->enDescriptorID, NULL, pSection);
				if (!(status==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pDesc))) {
					status = TSPARSE_ISDBT_SiMgmt_Section_AddDescriptor_Global(&pDesc, pstBCDescriptor_t, pSection);
					if (status!=TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pDesc))
						SIMGMT_DEBUG("[ProcessBIT]Section_AddDescriptor_Global(0x%x) failed(%d)", pstBCDescriptor_t->enDescriptorID, status);
					else
						SIMGMT_DEBUG("[ProcessBIT]Section_AddDescriptor_Global(0x%x) success(%d)", pstBCDescriptor_t->enDescriptorID, status);
				}
				pstBCDescriptor_t = pstBCDescriptor_t->pstLinkedDescriptor;
			}
		}

		/* check section loop */
		pBCData_t = pstBIT->pstBroadcasterData;
		while (CHK_VALIDPTR(pBCData_t)) {
			pSecLoop = (SI_MGMT_SECTIONLOOP*)NULL;
			unSecLoopData.bit_loop.broadcaster_id = pBCData_t->broadcaster_id;
			status = TSPARSE_ISDBT_SiMgmt_Section_FindLoop (&pSecLoop, &unSecLoopData, pSection);
			if (!(status==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pSecLoop))) {
				status = TSPARSE_ISDBT_SiMgmt_Section_AddLoop(&pSecLoop, &unSecLoopData, pSection);
				if (status != TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pSecLoop)) {
					pSecLoop = (SI_MGMT_SECTIONLOOP*)NULL;
					SIMGMT_DEBUG("[ProcessBIT]Section_AddLoop (broadcaster_id=0x%x) fail (%d)", pBCData_t->broadcaster_id, status);
				} else
					SIMGMT_DEBUG("[ProcessBIT]Section_AddLoop (broadcaster_id=0x%x) success (%d)", pBCData_t->broadcaster_id, status);
			} else {
				/* there is already a loop. do nothing */
			}
			/* add local descriptor to sectio loop */
			if (CHK_VALIDPTR(pSecLoop)) {
				if (pBCData_t->uiBCDescrCount > 0) {
					pstBCDescriptor_t = pBCData_t->pstBCDescr;
					while (CHK_VALIDPTR(pstBCDescriptor_t))
					{
						pDesc = (DESCRIPTOR_STRUCT*)NULL;
						status = TSPARSE_ISDBT_SiMgmt_SectionLoop_FindDescriptor(&pDesc, pstBCDescriptor_t->enDescriptorID, NULL, pSecLoop);
						if (!(status==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pDesc))) {
							status = TSPARSE_ISDBT_SiMgmt_SectionLoop_AddDescriptor(&pDesc, pstBCDescriptor_t, pSecLoop);
							if (status!=TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pDesc))
								SIMGMT_DEBUG("[ProcessBIT]SectionLoop_AddDescriptor(0x%x) failed(%d)", pstBCDescriptor_t->enDescriptorID, status);
							else
								SIMGMT_DEBUG("[ProcessBIT]SectionLoop_AddDescriptor(0x%x) success(%d)", pstBCDescriptor_t->enDescriptorID, status);
						}
						pstBCDescriptor_t = pstBCDescriptor_t->pstLinkedDescriptor;
					}
				}
			}
			pBCData_t = pBCData_t->pstBroadcasterData;
		}
	}
	/* ---- management for section informatin end ----*/

	pBCData_t = pstBIT->pstBroadcasterData;
	while(pBCData_t != NULL)
	{
		pstBCDescriptor_t = pBCData_t->pstBCDescr;
		while(pstBCDescriptor_t != NULL)
		{
			switch(pstBCDescriptor_t->enDescriptorID)
			{
				case EXT_BROADCASTER_DESCR_ID:
					if (ISDBT_Set_DescriptorInfo(E_DESC_EXT_BROADCAST_INFO, (void *)&(pstBCDescriptor_t->unDesc.stEBD)) == ISDBT_SUCCESS)
					{
						//empty
					}
					break;

				default:
					break;
			}
			pstBCDescriptor_t = pstBCDescriptor_t->pstLinkedDescriptor;
		}
		pBCData_t = pBCData_t->pstBroadcasterData;
	}

	return 1;
}

int ISDBT_TSPARSE_ProcessCDT(int fFirstSection, int fDone, CDT_STRUCT *pstCDT)
{
	if (pstCDT->data_type != 1 || pstCDT->ucSection > CDT_LOGO_SECTION_MAX || pstCDT->stLogoData.data_byte == NULL || pstCDT->stLogoData.data_byte[0] != 0x89) {
		//invalid logo
#if 1
		ALOGE("In ISDBT_TSPARSE_ProcessCDT, table error !");
		ALOGE(" CDT.download_data_id=%d", pstCDT->download_data_id);
		ALOGE(" CDT.version_number=%d", pstCDT->ucVersionNumber);
		ALOGE(" CDT.section=%d, last_section=%d", pstCDT->ucSection, pstCDT->ucLastSection);
		ALOGE(" CDT.original_network_id=0x%x", pstCDT->original_network_id);
		ALOGE(" CDT.data_type=%d", pstCDT->data_type);
#endif
		return -1;
	}

	stISDBTLogoInfo.download_data_id = pstCDT->download_data_id;
	stISDBTLogoInfo.ucSection = pstCDT->ucSection;
	stISDBTLogoInfo.ucLastSection = pstCDT->ucLastSection;
	stISDBTLogoInfo.original_network_id = pstCDT->original_network_id;
	stISDBTLogoInfo.logo_type = pstCDT->stLogoData.logo_type;
	stISDBTLogoInfo.logo_id = pstCDT->stLogoData.logo_id;
	stISDBTLogoInfo.logo_version = pstCDT->stLogoData.logo_version;
	stISDBTLogoInfo.data_size_length = pstCDT->stLogoData.data_size_length;
	stISDBTLogoInfo.data_byte = pstCDT->stLogoData.data_byte;
	UpdateDB_Logo_Info (&stISDBTLogoInfo);

	return 1;
}

int ISDBT_TSPARSE_ProcessTable_CopySectionExt (SI_MGMT_SECTION_EXTENSION* punSecExt, void *pstTable, int iTableID)
{
	int fDefault=0;
	if (!CHK_VALIDPTR(punSecExt) || !CHK_VALIDPTR(pstTable)) return 0;

	if (iTableID==PAT_ID) {
		fDefault = 1;
	} else if (iTableID==CAT_ID) {
		fDefault = 1;
	} else if (iTableID==PMT_ID) {
		SI_MGMT_SECTION_EXTENSION_PMT *pmt_sec_ext = &(punSecExt->pmt_sec_ext);
		PMT_STRUCT *pstPMT = (PMT_STRUCT*)pstTable;
		pmt_sec_ext->PCR_PID = pstPMT->uiPCR_PID;
	} else if (iTableID==NIT_A_ID) {
		fDefault = 1;
	} else if (iTableID==SDT_A_ID) {
		fDefault = 1;
	} else if (iTableID==SDT_O_ID) {
		fDefault = 1;
	} else if (iTableID==BAT_ID) {
		fDefault = 1;
	} else if (iTableID==EIT_PF_A_ID || (iTableID>=EIT_SA_0_ID && iTableID<=EIT_SA_F_ID)) {
		SI_MGMT_SECTION_EXTENSION_EIT *eit_sec_ext = &(punSecExt->eit_sec_ext);;
		EIT_STRUCT *pstEIT = (EIT_STRUCT*)pstTable;
		eit_sec_ext->segment_last_section_number = pstEIT->ucSegmentLastSection;
	} else if (iTableID==EIT_PF_O_ID || (iTableID>=EIT_SO_0_ID && iTableID<=EIT_SO_0_ID)) {
		fDefault = 1;
	} else if (iTableID==TDT_ID) {
		fDefault = 1;
	} else if (iTableID==TOT_ID) {
		SI_MGMT_SECTION_EXTENSION_TOT *tot_sec_ext = &(punSecExt->tot_sec_ext);
		TOT_STRUCT *pstTOT = (TOT_STRUCT*)pstTable;
		memcpy (&(tot_sec_ext->stDateTime), &(pstTOT->stDateTime), sizeof(DATE_TIME_STRUCT));
	} else if (iTableID==ECM_0_ID || iTableID==ECM_1_ID || iTableID==EMM_0_ID || iTableID==EMM_1_ID) {
		fDefault = 1;
	} else if (iTableID==BIT_ID) {
		fDefault = 1;
	} else if (iTableID==CDT_ID) {
		SI_MGMT_SECTION_EXTENSION_CDT *cdt_sec_ext = &(punSecExt->cdt_sec_ext);
		CDT_STRUCT *pstCDT = (CDT_STRUCT*)pstTable;
		cdt_sec_ext->data_type = pstCDT->data_type;
	}
	if (fDefault) memset(punSecExt, 0, sizeof(SI_MGMT_SECTION_EXTENSION));
	return 1;
}
int ISDBT_TSPARSE_ProcessTable_CopyTableExt (SI_MGMT_TABLE_EXTENSION* punTableExt, void *pstTable, int iTableID)
{
	int fDefault=0;
	if (!CHK_VALIDPTR(punTableExt) || !CHK_VALIDPTR(pstTable)) return 0;

	if (iTableID==PAT_ID) {
		SI_MGMT_TABLE_EXTENSION_PAT *pat_ext = &(punTableExt->pat_ext);
		PAT_STRUCT *pstPAT = (PAT_STRUCT*)pstTable;
		pat_ext->transport_stream_id = pstPAT->uiTransportID;
	} else if (iTableID==CAT_ID) {
		fDefault = 1;
	} else if (iTableID==PMT_ID) {
		SI_MGMT_TABLE_EXTENSION_PMT *pmt_ext = &(punTableExt->pmt_ext);
		PMT_STRUCT *pstPMT = (PMT_STRUCT*)pstTable;
		pmt_ext->program_number = pstPMT->uiServiceID;
	} else if (iTableID==NIT_A_ID) {
		SI_MGMT_TABLE_EXTENSION_NIT *nit_ext = &(punTableExt->nit_ext);
		NIT_STRUCT *pstNIT = (NIT_STRUCT*)pstTable;
		nit_ext->network_id = pstNIT->uiNetworkID;
	} else if (iTableID==SDT_A_ID) {
		SI_MGMT_TABLE_EXTENSION_SDT *sdt_ext = &(punTableExt->sdt_ext);
		SDT_STRUCT *pstSDT = (SDT_STRUCT*)pstTable;
		sdt_ext->original_network_id = pstSDT->uiOrigNetID;
		sdt_ext->transport_stream_id = pstSDT->uiTS_ID;
	} else if (iTableID==SDT_O_ID) {
		fDefault = 1;
	} else if (iTableID==BAT_ID) {
		fDefault = 1;
	} else if (iTableID==EIT_PF_A_ID || (iTableID>=EIT_SA_0_ID && iTableID<=EIT_SA_F_ID)) {
		SI_MGMT_TABLE_EXTENSION_EIT *eit_ext = &(punTableExt->eit_ext);;
		EIT_STRUCT *pstEIT = (EIT_STRUCT*)pstTable;
		eit_ext->service_id = pstEIT->uiService_ID;
		eit_ext->transport_stream_id = pstEIT->uiTStreamID;
		eit_ext->original_network_id = pstEIT->uiOrgNetWorkID;
		eit_ext->last_table_id = pstEIT->enLastTableType;
	} else if (iTableID==EIT_PF_O_ID || (iTableID>=EIT_SO_0_ID && iTableID<=EIT_SO_0_ID)) {
		fDefault = 1;
	} else if (iTableID==TDT_ID) {
		fDefault = 1;
	} else if (iTableID==TOT_ID) {
		fDefault = 1;
	} else if (iTableID==ECM_0_ID || iTableID==ECM_1_ID || iTableID==EMM_0_ID || iTableID==EMM_1_ID) {
		fDefault = 1;
	} else if (iTableID==BIT_ID) {
		SI_MGMT_TABLE_EXTENSION_BIT *bit_ext = &(punTableExt->bit_ext);
		BIT_STRUCT *pstBIT = (BIT_STRUCT*)pstTable;
		bit_ext->original_network_id = pstBIT->uiOrigNetID;
	} else if (iTableID==CDT_ID) {
		SI_MGMT_TABLE_EXTENSION_CDT *cdt_ext = &(punTableExt->cdt_ext);
		CDT_STRUCT *pstCDT = (CDT_STRUCT*)pstTable;
		cdt_ext->download_data_id = pstCDT->download_data_id;
		cdt_ext->original_network_id = pstCDT->original_network_id;
	}
	if (fDefault) memset(punTableExt, 0, sizeof(SI_MGMT_TABLE_EXTENSION));
	return 1;
}

/**************************************************************************
*  FUNCTION NAME :
*
*      int ISDBT_TSPARSE_ProcessTable (void *handle, unsigned char *pucRawData, unsigned int uiRawDataLen, unsigned short uiPID);
*
*  DESCRIPTION : Decoding DVB SI Informations(PAT, PMT, NIT, SDT, EIT ... )
*
*  INPUT:
*			handle	= Bit Parser Handle
*			pucRawData	= Table Data
*			uiRawDataLen = Size of Table Data.
*			RequestID	= RequestID of Input SI Data
*
*  OUTPUT:	int - Return Type
*  			= -1 : decoding error, -2 : Not support SI, -3 : Version is old(Skip Decoding)
*             if decoding is success, return Table ID. ( In SDT, if remain section to decode, return Status)
*
*  REMARK:
**************************************************************************/

int ISDBT_TSPARSE_ProcessTable(void *handle, unsigned char *pucRawData, unsigned int uiRawDataLen, int RequestID)
{
	/* bFilter must be initialized first */
	int       fDone = TRUE;
	int       ucTableID;
	int       ucSectionT;
	int       ucLastSectionT;
	int       fDiscard = TRUE;
	int       fRslt = 0;
	int       fFirstSection = TRUE;

	SI_MGMT_TABLE_ROOT *pTableRoot = (SI_MGMT_TABLE_ROOT*)NULL;
	SI_MGMT_TABLE *pTable = (SI_MGMT_TABLE *)NULL;
	SI_MGMT_TABLE_EXTENSION unTableExt = { 0, };
	SI_MGMT_SECTION *pSection = (SI_MGMT_SECTION*)NULL;
	SI_MGMT_SECTION_EXTENSION unSectionExt = { 0, };
	unsigned char section_version = 0, version_number = 0;
	int status = 0;

	ucTableID = *pucRawData;
	fRslt = ISDBT_TSPARSE_PreProcess(handle, pucRawData, uiRawDataLen, RequestID);
	if (fRslt == -1)
	{
		//Old versin SI Information, Skip Decoding
		return -3;
	}
	else if (fRslt == -2)
	{
		//error
		return -1;
	}

	fRslt = MPEGPARS_ParseTable(handle, pucRawData, uiRawDataLen, &g_pSI_Table);
	if (fRslt == FALSE)
	{
		/* Clean up heap */
		MPEGPARS_FreeTable((VOID *) & g_pSI_Table);
		g_pSI_Table = NULL;
		return -1;
	}	/* End "if (fRslt == FALSE)" */
	else
	{
		ISDBT_TSPARSE_PostProcess(handle, pucRawData, uiRawDataLen, RequestID);

		/* Update section */
		switch (ucTableID)
		{
		case TOT_ID:
		case TDT_ID:
		case RST_ID:
			/* These tables have only one section and no ExtIDs */
			ucSectionT = 0;
			ucLastSectionT = 0;
			version_number = 0;
			break;

		default:
		#if 0
			/* All others have potential sections which are all in the same */
			/* place, so use NIT as a generic pointer for them all          */
			ucSectionT = ((NIT_STRUCT *) g_pSI_Table)->ucSection;
			ucLastSectionT = ((NIT_STRUCT *) g_pSI_Table)->ucLastSection;
		#else
			ucSectionT = *(pucRawData+6);
			ucLastSectionT = *(pucRawData+7);
			version_number = (*(pucRawData+5) & 0x3E)>>1;
		#endif
			break;
		}

		if (ucSectionT == 0) fFirstSection = TRUE;
		else fFirstSection = FALSE;

		/* There are more sections */
		fDone = (ucSectionT == ucLastSectionT);
		fDiscard = TRUE;	/* Assume that the raw table must be discarded */

		//PARSE_DBG("TableID(0x%X)Section done %d-current(%d)last(%d)\n",ucTableID, fDone, ucSectionT, ucLastSectionT);

		/* ---- management for section informatin start ----*/
		status = TSPARSE_ISDBT_SiMgmt_GetTableRoot (&pTableRoot, ucTableID);
		if (status==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pTableRoot)
		&&	(ucTableID < EIT_SA_0_ID || ucTableID > EIT_SO_F_ID)) {
			/* find subtable */
			ISDBT_TSPARSE_ProcessTable_CopyTableExt(&unTableExt, g_pSI_Table, ucTableID);
			ISDBT_TSPARSE_ProcessTable_CopySectionExt(&unSectionExt, g_pSI_Table, ucTableID);
			status = TSPARSE_ISDBT_SiMgmt_FindTable (&pTable, ucTableID, &unTableExt, pTableRoot);
			if (status==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pTable)) {
				/* if there is already same subtable */
				status = TSPARSE_ISDBT_SiMgmt_Table_FindSection (&pSection, eSECTION_CURR, ucSectionT, pTable);
				if (status==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pSection)) {
					/* if there is already same sectin */
					TSPARSE_ISDBT_SiMgmt_Section_GetVersioNumber (&section_version, pSection);
					if ((ucTableID==TOT_ID) /* TOT has no version. it should always be updated */
						|| (section_version != version_number)) {
						/* if a version of section is different */
						TSPARSE_ISDBT_SiMgmt_Table_SectionTransit (pTable);
						status = TSPARSE_ISDBT_SiMgmt_Table_AddSection (&pSection, eSECTION_CURR, version_number, ucSectionT, ucLastSectionT, &unSectionExt, pTable);
						if (status != TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pSection)) SIMGMT_DEBUG("[ISDBT_TSPARSE_ProcessPAT AddSectoin fail (%d)", status);
					} else {
						SIMGMT_DEBUG("[ProcessTable]this table(table_id=0x%x, section_number=0x%x, version_number=0x%x) already exist", ucTableID, ucSectionT, version_number);
					}
				} else { /* there is no same section */
					status = TSPARSE_ISDBT_SiMgmt_Table_AddSection (&pSection, eSECTION_CURR, version_number, ucSectionT, ucLastSectionT, &unSectionExt, pTable);
					if (status != TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pSection)) SIMGMT_DEBUG("[ISDBT_TSPARSE_ProcessPAT AddSectoin fail (%d)", status);
				}
			} else { /* there is no same subtable */
				SIMGMT_DEBUG("[ProcessTable]No same subtable (table_id=0x%x). Add a new subtable", ucTableID);
				status = TSPARSE_ISDBT_SiMgmt_AddTable (&pTable, ucTableID, &unTableExt, pTableRoot);
				if (status != TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pTable)) SIMGMT_DEBUG("[ISDBT_TSPARSE_ProcessPAT] AddTable fail (%d)", status);
				status = TSPARSE_ISDBT_SiMgmt_Table_AddSection (&pSection, eSECTION_CURR, version_number, ucSectionT, ucLastSectionT, &unSectionExt, pTable);
				if (status != TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pSection)) SIMGMT_DEBUG("[ISDBT_TSPARSE_ProcessPAT AddSectoin fail (%d)", status);
			}
		} else {
			SIMGMT_DEBUG("[ProcessTable]this table is not handled by simgmt(%d)", ucTableID);
		}
		/* ---- management for section informatin start ----*/

		switch (ucTableID)
		{
		case PAT_ID:
			// clear all. table
			if (ISDBT_TSPARSE_ProcessPAT(fFirstSection, fDone, (PAT_STRUCT *)g_pSI_Table, pSection) < 0) fRslt = -1;
			else fRslt = PAT_ID;
			break;

		case CAT_ID:
			if (ISDBT_TSPARSE_ProcessCAT(fFirstSection, fDone, (CAT_STRUCT *)g_pSI_Table, pSection) < 0) fRslt = -1;
			else fRslt = ucTableID;
			break;

		case PMT_ID:
			if (ISDBT_TSPARSE_ProcessPMT(fFirstSection, fDone, (PMT_STRUCT *)g_pSI_Table, pSection) < 0) fRslt = -1;
			else fRslt = PMT_ID;
			if(g_stISDBT_ServiceList.uiDSMCC_PID != 0) {
				tcc_manager_demux_dataservice_set(((PMT_STRUCT *)g_pSI_Table)->uiServiceID, ucTableID, g_stISDBT_ServiceList.uiDSMCC_PID, g_stISDBT_ServiceList.ucAutoStartFlag, g_stISDBT_ServiceList.usNetworkID, g_stISDBT_ServiceList.uiTStreamID, uiRawDataLen, pucRawData);
				g_stISDBT_ServiceList.uiDSMCC_PID = 0;
			}
			break;

		case ODT_ID:
			break;

		case NIT_A_ID:
			if (ISDBT_TSPARSE_ProcessNIT(fFirstSection, fDone, (NIT_STRUCT *)g_pSI_Table, pSection) < 0) fRslt = -1;
			else fRslt = NIT_A_ID;
			break;

		case SDT_A_ID:
			ISDBT_TSPARSE_ProcessSDT(fFirstSection, fDone, (SDT_STRUCT *)g_pSI_Table, pSection);
			if (fDone) fRslt = SDT_A_ID;	//SDT Parsing done
			else fRslt = -1;
			break;

		case SDT_O_ID:
			fRslt = SDT_O_ID;
			break;

		case BAT_ID:
			fRslt = ucTableID;
			break;

		case EIT_PF_A_ID:
			if (ISDBT_TSPARSE_ProcessEIT(fFirstSection, fDone, (EIT_STRUCT *)g_pSI_Table, pSection) < 0) fRslt = -1;
			else fRslt = EIT_PF_A_ID;
			break;

		case EIT_PF_O_ID:
			fRslt = ucTableID;
			break;

		case EIT_SA_0_ID:
		case EIT_SA_1_ID:
		case EIT_SA_2_ID:
		case EIT_SA_3_ID:
		case EIT_SA_4_ID:
		case EIT_SA_5_ID:
		case EIT_SA_6_ID:
		case EIT_SA_7_ID:
		case EIT_SA_8_ID:
		case EIT_SA_9_ID:
		case EIT_SA_A_ID:
		case EIT_SA_B_ID:
		case EIT_SA_C_ID:
		case EIT_SA_D_ID:
		case EIT_SA_E_ID:
		case EIT_SA_F_ID:
			if (ISDBT_TSPARSE_ProcessEIT(fFirstSection, fDone, (EIT_STRUCT *)g_pSI_Table, pSection) < 0) fRslt = -1;
			else fRslt = ucTableID;
			break;

			//added by see21
			//2007-4-4 13:27:55
		case EIT_SO_0_ID:
		case EIT_SO_1_ID:
		case EIT_SO_2_ID:
		case EIT_SO_3_ID:
		case EIT_SO_4_ID:
		case EIT_SO_5_ID:
		case EIT_SO_6_ID:
		case EIT_SO_7_ID:
		case EIT_SO_8_ID:
		case EIT_SO_9_ID:
		case EIT_SO_A_ID:
		case EIT_SO_B_ID:
		case EIT_SO_C_ID:
		case EIT_SO_D_ID:
		case EIT_SO_E_ID:
		case EIT_SO_F_ID:
			fRslt = ucTableID;
			break;

		case TDT_ID:
			if (ISDBT_TSPARSE_ProcessTDT(fFirstSection, fDone, (TDT_STRUCT *)g_pSI_Table) < 0) fRslt = -1;
			else fRslt = TDT_ID;
			break;

		case TOT_ID:
			if (ISDBT_TSPARSE_ProcessTOT(fFirstSection, fDone, (TOT_STRUCT *)g_pSI_Table, pSection) < 0) fRslt = -1;
			else fRslt = TOT_ID;
			break;

		case ECM_0_ID:
		case ECM_1_ID:
//		case ECM_2_ID:
		case EMM_0_ID:
		case EMM_1_ID:
//			ALOGE("==========> ECM_?_ ucTableID = %d\n", ucTableID);
			fRslt = ucTableID;
			break;

		case BIT_ID:
			if (ISDBT_TSPARSE_ProcessBIT(fFirstSection, fDone, (BIT_STRUCT *)g_pSI_Table, pSection) < 0) fRslt = -1;
			else fRslt = BIT_ID;
			break;

		case CDT_ID:
		{
			CDT_STRUCT *cdt = (CDT_STRUCT *)g_pSI_Table;
			if (ISDBT_TSPARSE_ProcessCDT(fFirstSection, fDone, (CDT_STRUCT *)g_pSI_Table) < 0) {
				SetCDTVersion (cdt->download_data_id, cdt->ucSection, cdt->ucLastSection, 0xFF);
				fRslt = -1;
			}
			else
				fRslt = CDT_ID;
			break;
		}

		default:
			ALOGE("########## ucTableID:0x%X\n", ucTableID);
			fRslt = -2;
			break;

		}	/* End "switch (ucTableID)" */

#if 0 /* test code for section management */
//		if (!(ucTableID ==EIT_PF_A_ID) && !(ucTableID >= EIT_SA_0_ID && ucTableID <= EIT_SA_F_ID))
//		if (ucTableID==PAT_ID)
//		if (ucTableID==CAT_ID)
		if (ucTableID==PMT_ID)
//		if (ucTableID==BIT_ID)
//		if (ucTableID==NIT_A_ID)
//		if (ucTableID==SDT_A_ID)
//		if (TSPARSE_ISDBT_SiMgmt_Table_IsEIT(ucTableID))
//		if (ucTableID==0x4E)
//		if(ucTableID==0x50)
//		if (ucTableID==TOT_ID)
			TSPARSE_ISDBT_SiMgmt_TEST_DisplayTable(ucTableID, &unTableExt);
#endif

		MPEGPARS_FreeTable((VOID *) & g_pSI_Table);
		g_pSI_Table = NULL;
	}	/* End " else  // if (fRslt == TRUE)" */

	return fRslt;
}	/* End"SI_TABLE_ProcessTable ()" */

int ISDBT_TSPARSE_CURDESCINFO_set(unsigned short usServiceID, unsigned char ucDescID, void *pvDesc)
{
	PARSE_DBG("%s ServiceID[0x%x] DescID[0x%x]\n", __func__, usServiceID, ucDescID);

	int i;
	int updateFlg = 0;
	int iArg[20] = {0, };
	int state = tcc_manager_demux_get_state();

	if(state != E_DEMUX_STATE_AIRPLAY)
	{
		PARSE_DBG("%s demux state = %d\n", __func__, state);
		return 0;
	}

	if(gCurrentDescInfo.services[0].usServiceID == 0x0)
	{
		int serviceID[MAX_SUPPORT_CURRENT_SERVICE] = {0, };
		int serviceType[MAX_SUPPORT_CURRENT_SERVICE] = {0, };
		int serviceCount = 0;
		serviceCount = tcc_db_get_service_info(gCurrentChannel, gCurrentCountryCode, serviceID, serviceType);
		for(i=0;i<serviceCount&&i<MAX_SUPPORT_CURRENT_SERVICE;i++)
		{
			gCurrentDescInfo.services[i].usServiceID = serviceID[i];
			gCurrentDescInfo.services[i].ucServiceType = serviceType[i];
		}
	}

	for(i=0;i<MAX_SUPPORT_CURRENT_SERVICE;i++)
		if(gCurrentDescInfo.services[i].usServiceID == usServiceID)
			break;

	if(i == MAX_SUPPORT_CURRENT_SERVICE)
	{
		PARSE_DBG("%s MAX_SUPPORT_CURRENT_SERVICE over\n", __func__);
		return 1;
	}

	switch(ucDescID)
	{
		case 	DIGITALCOPY_CONTROL_ID:
		{
			T_DESC_DCC	*pstDescDCC = (T_DESC_DCC *)pvDesc;

			if( (gCurrentDescInfo.services[i].stDescDCC.digital_recording_control_data	!= pstDescDCC->digital_recording_control_data) ||
				(gCurrentDescInfo.services[i].stDescDCC.maximum_bitrate_flag			!= pstDescDCC->maximum_bitrate_flag) ||
				(gCurrentDescInfo.services[i].stDescDCC.component_control_flag			!= pstDescDCC->component_control_flag) ||
				(gCurrentDescInfo.services[i].stDescDCC.copy_control_type				!= pstDescDCC->copy_control_type) ||
				(gCurrentDescInfo.services[i].stDescDCC.APS_control_data 				!= pstDescDCC->APS_control_data))
			{
				if ((pstDescDCC->ucTableID == E_TABLE_ID_PMT) ||
					((pstDescDCC->ucTableID == E_TABLE_ID_EIT) && ((gCurrentDescInfo.services[i].stDescDCC.ucTableID == E_TABLE_ID_EIT) || (gCurrentDescInfo.services[i].stDescDCC.ucTableID == E_TABLE_ID_SDT))) ||
					((pstDescDCC->ucTableID == E_TABLE_ID_SDT) && (gCurrentDescInfo.services[i].stDescDCC.ucTableID == E_TABLE_ID_SDT)) ||
					gCurrentDescInfo.services[i].stDescDCC.ucTableID == E_TABLE_ID_NONE)
				{
					gCurrentDescInfo.services[i].stDescDCC.ucTableID								= pstDescDCC->ucTableID;
					gCurrentDescInfo.services[i].stDescDCC.digital_recording_control_data	= pstDescDCC->digital_recording_control_data;
					gCurrentDescInfo.services[i].stDescDCC.maximum_bitrate_flag				= pstDescDCC->maximum_bitrate_flag;
					gCurrentDescInfo.services[i].stDescDCC.component_control_flag			= pstDescDCC->component_control_flag;
					gCurrentDescInfo.services[i].stDescDCC.copy_control_type				= pstDescDCC->copy_control_type;
					gCurrentDescInfo.services[i].stDescDCC.APS_control_data 				= pstDescDCC->APS_control_data;
					if(gCurrentDescInfo.services[i].stDescDCC.maximum_bitrate_flag)
						gCurrentDescInfo.services[i].stDescDCC.maximum_bitrate				= pstDescDCC->maximum_bitrate;
					else
						gCurrentDescInfo.services[i].stDescDCC.maximum_bitrate				= 0;

					// If sub_info exist, it is freed.
					if(gCurrentDescInfo.services[i].stDescDCC.sub_info)
					{
						SUB_T_DESC_DCC* sub_info = gCurrentDescInfo.services[i].stDescDCC.sub_info;
						while(sub_info)
						{
							SUB_T_DESC_DCC* temp_sub_info = sub_info->pNext;
							tcc_mw_free(__FUNCTION__, __LINE__, sub_info);
							sub_info = temp_sub_info;
						}
					}
					gCurrentDescInfo.services[i].stDescDCC.sub_info = 0;

					// Set sub_info with new information.
					if(pstDescDCC->sub_info)
					{
						SUB_T_DESC_DCC* stSubDescDCC;
						SUB_T_DESC_DCC* backup_sub_info;
						SUB_T_DESC_DCC* sub_info = pstDescDCC->sub_info;

						while(sub_info)
						{
							stSubDescDCC = (SUB_T_DESC_DCC*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(SUB_T_DESC_DCC));
							if(0 == stSubDescDCC)
							{
								ALOGE("[%s][%d] : SUB_T_DESC_DCC malloc fail !!!!!\n", __func__, __LINE__);
								return 1;
							}
							memset(stSubDescDCC, 0, sizeof(SUB_T_DESC_DCC));

							stSubDescDCC->component_tag 				 = sub_info->component_tag;
							stSubDescDCC->digital_recording_control_data = sub_info->digital_recording_control_data;
							stSubDescDCC->maximum_bitrate_flag			 = sub_info->maximum_bitrate_flag;
							stSubDescDCC->copy_control_type 			 = sub_info->copy_control_type;
							stSubDescDCC->APS_control_data				 = sub_info->APS_control_data;
							stSubDescDCC->maximum_bitrate				 = sub_info->maximum_bitrate;

							if(0 == gCurrentDescInfo.services[i].stDescDCC.sub_info)
							{
								gCurrentDescInfo.services[i].stDescDCC.sub_info = stSubDescDCC;
								backup_sub_info = stSubDescDCC;
							}
							else
							{
								backup_sub_info->pNext = stSubDescDCC;
								backup_sub_info = stSubDescDCC;
							}

							sub_info = sub_info->pNext;
						}
					}

					iArg[0] = &(gCurrentDescInfo.services[i].stDescDCC);

					updateFlg = 1;
				}
			}
		}
		break;
		case 	CONTENT_AVAILABILITY_ID:
		{
			T_DESC_CONTA	*pstDescCONTA = (T_DESC_CONTA *)pvDesc;
			if(	(gCurrentDescInfo.services[i].stDescCONTA.copy_restriction_mode     != pstDescCONTA->copy_restriction_mode) ||
				(gCurrentDescInfo.services[i].stDescCONTA.image_constraint_token	!= pstDescCONTA->image_constraint_token) ||
				(gCurrentDescInfo.services[i].stDescCONTA.retention_mode 			!= pstDescCONTA->retention_mode) ||
				(gCurrentDescInfo.services[i].stDescCONTA.retention_state			!= pstDescCONTA->retention_state) ||
				(gCurrentDescInfo.services[i].stDescCONTA.encryption_mode			!= pstDescCONTA->encryption_mode))
			{
				if ((pstDescCONTA->ucTableID == E_TABLE_ID_PMT) ||
					((pstDescCONTA->ucTableID == E_TABLE_ID_EIT) && ((gCurrentDescInfo.services[i].stDescCONTA.ucTableID == E_TABLE_ID_EIT) || (gCurrentDescInfo.services[i].stDescCONTA.ucTableID == E_TABLE_ID_SDT))) ||
					((pstDescCONTA->ucTableID == E_TABLE_ID_SDT) && (gCurrentDescInfo.services[i].stDescCONTA.ucTableID == E_TABLE_ID_SDT)) ||
					gCurrentDescInfo.services[i].stDescDCC.ucTableID == E_TABLE_ID_NONE)
				{
					gCurrentDescInfo.services[i].stDescCONTA.ucTableID							= pstDescCONTA->ucTableID;
					iArg[0] = gCurrentDescInfo.services[i].stDescCONTA.copy_restriction_mode    = pstDescCONTA->copy_restriction_mode;
					iArg[1] = gCurrentDescInfo.services[i].stDescCONTA.image_constraint_token	= pstDescCONTA->image_constraint_token;
					iArg[2] = gCurrentDescInfo.services[i].stDescCONTA.retention_mode 			= pstDescCONTA->retention_mode;
					iArg[3] = gCurrentDescInfo.services[i].stDescCONTA.retention_state			= pstDescCONTA->retention_state;
					iArg[4] = gCurrentDescInfo.services[i].stDescCONTA.encryption_mode			= pstDescCONTA->encryption_mode;

					updateFlg = 1;
				}
			}

		}
		break;
		case EVT_GROUP_DESCR_ID:
		{
			T_ISDBT_EIT_EVENT_GROUP *pstDescEGD = (T_ISDBT_EIT_EVENT_GROUP *)pvDesc;
			memcpy(&gCurrentDescInfo.services[i].stEGD, pstDescEGD, sizeof(T_ISDBT_EIT_EVENT_GROUP));
		}
		break;
		case COMPONENT_GROUP_DESCR_ID:
		{
			T_DESC_CGD *pstDescCGD = (T_DESC_CGD *)pvDesc;
			memcpy(&gCurrentDescInfo.services[i].stDescCGD, pstDescCGD, sizeof(T_DESC_CGD));
		}
		break;
		default:
			PARSE_DBG("%s 0x%x is not support\n", __func__, ucDescID);
			return 1;
	}

	if(updateFlg)
	{
		tcc_manager_demux_desc_update(usServiceID, ucDescID, (void *)iArg);
	}

	return 0;
}

/*
  * made by Noah / 201609XX / return current Digital Copy Control descriptor info.
  *
  * description
  *   This function is only for Digital Copy Control and is based on ISDBT_TSPARSE_CURDESCINFO_get.
  *
  * parameter
  *   usServiceID - To search the service of the usServiceID.
  *   pDCCInfo - It is output parameter and is pointing DCC info struct.
  *
  * return
  *   0 - Success
  *   else - Fail
*/
int ISDBT_TSPARSE_CURDCCDESCINFO_get(unsigned short usServiceID, void **pDCCInfo)
{
	PARSE_DBG("%s ServiceID[0x%x]\n", __func__, usServiceID);

	int i;
	int state = tcc_manager_demux_get_state();

	if(state != E_DEMUX_STATE_AIRPLAY) {
		PARSE_DBG("%s demux state = %d\n", __func__, state);
		return 0;
	}

	if(usServiceID == 0) {
		if(gPlayingServiceID != 0) {
			usServiceID = gPlayingServiceID;
		}
		else {
			PARSE_DBG("%s there is no curent service info curServiceID[0x%x]\n", __func__, gPlayingServiceID);
			return 1;
		}
	}

	for(i=0;i<MAX_SUPPORT_CURRENT_SERVICE;i++)
		if(gCurrentDescInfo.services[i].usServiceID == usServiceID)
			break;

	if(i == MAX_SUPPORT_CURRENT_SERVICE) {
		PARSE_DBG("%s there is no service[0x%x] info\n", __func__, usServiceID);
		return 1;
	}


	// DCC info in PMT, EIT, and SDT doesn't exist
	if( !(gCurrentDescInfo.services[i].ucPMTDescBitMap & 0x01) &&
		!(gCurrentDescInfo.services[i].ucSDTDescBitMap & 0x01) &&
		!(gCurrentDescInfo.services[i].ucEITDescBitMap & 0x01) )
	{
		ISDBT_TSPARSE_CURDESCINFO_init(usServiceID, DIGITALCOPY_CONTROL_ID);

		gCurrentDescInfo.services[i].stDescDCC.digital_recording_control_data;
		gCurrentDescInfo.services[i].stDescDCC.maximum_bitrate_flag;
		gCurrentDescInfo.services[i].stDescDCC.component_control_flag;
		if(gCurrentDescInfo.services[i].ucServiceType == 0x01)         // 0x01 is Digital TV Service. It means 12seg.
			gCurrentDescInfo.services[i].stDescDCC.copy_control_type = 0x1;
		else if(gCurrentDescInfo.services[i].ucServiceType == 0xC0)    // 0xC0 is Data Service. It means 1seg.
			gCurrentDescInfo.services[i].stDescDCC.copy_control_type = 0x2;
		else
			gCurrentDescInfo.services[i].stDescDCC.copy_control_type = 0x3;
		gCurrentDescInfo.services[i].stDescDCC.APS_control_data;
		gCurrentDescInfo.services[i].stDescDCC.maximum_bitrate;

		gCurrentDescInfo.services[i].stDescDCC.sub_info = 0;
	}

	// Important !!!!! : pDCCInfo is sent to ISDBTPlayer::getDigitalCopyControlDescriptor function.
	*pDCCInfo = &(gCurrentDescInfo.services[i].stDescDCC);

	return 0;
}

int ISDBT_TSPARSE_CURDESCINFO_get(unsigned short usServiceID, DESCRIPTOR_IDS ucDescID, void *pvDesc)
{
	PARSE_DBG("%s ServiceID[0x%x] DescID[0x%x]\n", __func__, usServiceID, ucDescID);

	int i;
	int state = tcc_manager_demux_get_state();

	if(state != E_DEMUX_STATE_AIRPLAY) {
		PARSE_DBG("%s demux state = %d\n", __func__, state);
		return 0;
	}

	if(usServiceID == 0) {
		if(gPlayingServiceID != 0) {
			usServiceID = gPlayingServiceID;
		}
		else {
			PARSE_DBG("%s there is no curent service info curServiceID[0x%x]\n", __func__, gPlayingServiceID);
			return 1;
		}
	}

	for(i=0;i<MAX_SUPPORT_CURRENT_SERVICE;i++)
		if(gCurrentDescInfo.services[i].usServiceID == usServiceID)
		break;

	if(i == MAX_SUPPORT_CURRENT_SERVICE) {
		PARSE_DBG("%s there is no service[0x%x] info\n", __func__, usServiceID);
		return 1;
	}

	switch(ucDescID)
	{
		case 	CONTENT_AVAILABILITY_ID:
		{
			T_DESC_CONTA *pstDescCONTA = (T_DESC_CONTA *)pvDesc;
			pstDescCONTA->ucTableID					= gCurrentDescInfo.services[i].stDescCONTA.ucTableID;
			pstDescCONTA->copy_restriction_mode     = gCurrentDescInfo.services[i].stDescCONTA.copy_restriction_mode;
			pstDescCONTA->image_constraint_token	= gCurrentDescInfo.services[i].stDescCONTA.image_constraint_token;
			pstDescCONTA->retention_mode			= gCurrentDescInfo.services[i].stDescCONTA.retention_mode;
			pstDescCONTA->retention_state			= gCurrentDescInfo.services[i].stDescCONTA.retention_state;
			pstDescCONTA->encryption_mode			= gCurrentDescInfo.services[i].stDescCONTA.encryption_mode;
		}
		break;
		case 	EVT_GROUP_DESCR_ID:
		{
			T_ISDBT_EIT_EVENT_GROUP *pstDescEGD = (T_ISDBT_EIT_EVENT_GROUP *)pvDesc;
			pstDescEGD->eventID				= gCurrentDescInfo.services[i].stEGD.eventID;
			pstDescEGD->group_type			= gCurrentDescInfo.services[i].stEGD.group_type;
			pstDescEGD->event_count			= gCurrentDescInfo.services[i].stEGD.event_count;
			pstDescEGD->service_id			= gCurrentDescInfo.services[i].stEGD.service_id;
			pstDescEGD->event_id			= gCurrentDescInfo.services[i].stEGD.event_id;
			pstDescEGD->original_network_id	= gCurrentDescInfo.services[i].stEGD.original_network_id;
			pstDescEGD->transport_stream_id	= gCurrentDescInfo.services[i].stEGD.transport_stream_id;
		}
		break;
		default:
			PARSE_DBG("%s 0x%d is not support\n", __func__, ucDescID);
			return 1;
	}
	return 0;
}

int ISDBT_TSPARSE_CURDESCINFO_setBitMap(unsigned short usServiceID, unsigned char ucTableID, unsigned char ucDescBitMap)
{
	PARSE_DBG("%s ServiceID[0x%x] TableID[0x%x]\n", __func__, usServiceID, ucTableID);

	int i;
	int piArg[10] = {0, };
	int state = tcc_manager_demux_get_state();
	unsigned char flgDCCupdate = 0;
	unsigned char flgCONTAupdate = 0;

	if(state != E_DEMUX_STATE_AIRPLAY) {
		PARSE_DBG("%s demux state = %d\n", __func__, state);
		return 0;
	}

	if(gCurrentDescInfo.services[0].usServiceID == 0x0) {
		int serviceID[MAX_SUPPORT_CURRENT_SERVICE] = {0, };
		int serviceType[MAX_SUPPORT_CURRENT_SERVICE] = {0, };
		int serviceCount = 0;
		serviceCount = tcc_db_get_service_info(gCurrentChannel, gCurrentCountryCode, serviceID, serviceType);
		for(i=0;i<serviceCount&&i<MAX_SUPPORT_CURRENT_SERVICE;i++) {
			gCurrentDescInfo.services[i].usServiceID = serviceID[i];
			gCurrentDescInfo.services[i].ucServiceType = serviceType[i];
		}
	}

	for(i=0;i<MAX_SUPPORT_CURRENT_SERVICE;i++)
		if(gCurrentDescInfo.services[i].usServiceID == usServiceID)
			break;

	if(i == MAX_SUPPORT_CURRENT_SERVICE) {
		PARSE_DBG("%s there is no service[0x%x] info\n", __func__, usServiceID);
		return 1;
	}

	switch(ucTableID)
	{
		case E_TABLE_ID_PMT:
			if(gCurrentDescInfo.services[i].ucPMTDescBitMap != 0x0)
			{
				// DCC info in PMT exists -> doesn't exist
				if((gCurrentDescInfo.services[i].ucPMTDescBitMap & 0x1) && !(ucDescBitMap & 0x1))
				{
					flgDCCupdate = 1;
				}

				if((gCurrentDescInfo.services[i].ucPMTDescBitMap & 0x2) && !(ucDescBitMap & 0x2))
				{
					flgCONTAupdate = 1;
				}
			}

			gCurrentDescInfo.services[i].ucPMTDescBitMap = ucDescBitMap;
			break;

		case E_TABLE_ID_SDT:
			if(gCurrentDescInfo.services[i].ucSDTDescBitMap != 0x0)
			{
				// DCC info in PMT and EIT doesn't exist, but DCC info in SDT exists -> doesn't exist
				if(	!(gCurrentDescInfo.services[i].ucPMTDescBitMap & 0x1) &&
					!(gCurrentDescInfo.services[i].ucEITDescBitMap & 0x1) &&
					(gCurrentDescInfo.services[i].ucSDTDescBitMap & 0x1) && !(ucDescBitMap & 0x1))
				{
					flgDCCupdate = 1;
				}

				if(	!(gCurrentDescInfo.services[i].ucPMTDescBitMap & 0x2) &&
					!(gCurrentDescInfo.services[i].ucEITDescBitMap & 0x2) &&
					(gCurrentDescInfo.services[i].ucSDTDescBitMap & 0X2) && !(ucDescBitMap & 0x2))
				{
					flgCONTAupdate = 1;
				}
			}

			gCurrentDescInfo.services[i].ucSDTDescBitMap = ucDescBitMap;

			// DCC info in PMT, EIT, and SDT doesn't exist
			if(	!(gCurrentDescInfo.services[i].ucPMTDescBitMap & 0x1) &&
				!(gCurrentDescInfo.services[i].ucEITDescBitMap & 0x1) &&
				!(gCurrentDescInfo.services[i].ucSDTDescBitMap & 0x1))
			{
				flgDCCupdate = 1;
			}

			if(	!(gCurrentDescInfo.services[i].ucPMTDescBitMap & 0x2) &&
				!(gCurrentDescInfo.services[i].ucEITDescBitMap & 0x2) &&
				!(gCurrentDescInfo.services[i].ucSDTDescBitMap & 0X2))
			{
				flgCONTAupdate = 1;
			}
			break;

		case E_TABLE_ID_EIT:
			if(gCurrentDescInfo.services[i].ucEITDescBitMap != 0x0)
			{
				// DCC info in PMT doesn't exist, but DCC info in EIT exists -> doesn't exist
				if(	!(gCurrentDescInfo.services[i].ucPMTDescBitMap & 0x1) &&
					(gCurrentDescInfo.services[i].ucEITDescBitMap & 0x1) && !(ucDescBitMap & 0x1))
				{
					flgDCCupdate = 1;
				}

				if(	!(gCurrentDescInfo.services[i].ucPMTDescBitMap & 0x2) &&
					(gCurrentDescInfo.services[i].ucEITDescBitMap & 0x2) && !(ucDescBitMap & 0x2))
				{
					flgCONTAupdate = 1;
				}

				if((gCurrentDescInfo.services[i].ucEITDescBitMap & 0x4) && !(ucDescBitMap & 0x4))
				{
					ISDBT_TSPARSE_CURDESCINFO_init(usServiceID, EVT_GROUP_DESCR_ID);
				}
			}

			if(usServiceID == gPlayingServiceID)
			{
				unsigned char arr[5] = {0, };
				if(!(ucDescBitMap & 0x8))
				{
					ISDBT_TSPARSE_CURDESCINFO_init(usServiceID, COMPONENT_GROUP_DESCR_ID);
				}
				arr[0] = gCurrentDescInfo.services[i].stDescCGD.component_group_type;
				arr[1] = gCurrentDescInfo.services[i].stDescCGD.num_of_group;
				arr[2] = gCurrentDescInfo.services[i].stDescCGD.component_group_id[0];
				arr[3] = gCurrentDescInfo.services[i].stDescCGD.component_group_id[1];
				arr[4] = gCurrentDescInfo.services[i].stDescCGD.component_group_id[2];
				tcc_manager_demux_mvtv_update(arr);
			}
			gCurrentDescInfo.services[i].ucEITDescBitMap = ucDescBitMap;
			break;

		default:
			ALOGE("%s 0x%x is not support section", __func__, ucTableID);
			return 1;
	}

	if(flgDCCupdate == 1)
	{
		ISDBT_TSPARSE_CURDESCINFO_init(usServiceID, DIGITALCOPY_CONTROL_ID);

		gCurrentDescInfo.services[i].stDescDCC.digital_recording_control_data;
		gCurrentDescInfo.services[i].stDescDCC.maximum_bitrate_flag;
		gCurrentDescInfo.services[i].stDescDCC.component_control_flag;
		if(gCurrentDescInfo.services[i].ucServiceType == 0x01)         // 0x01 is Digital TV Service. It means 12seg.
			gCurrentDescInfo.services[i].stDescDCC.copy_control_type = 0x1;
		else if(gCurrentDescInfo.services[i].ucServiceType == 0xC0)    // 0xC0 is Data Service. It means 1seg.
			gCurrentDescInfo.services[i].stDescDCC.copy_control_type = 0x2;
		else
			gCurrentDescInfo.services[i].stDescDCC.copy_control_type = 0x3;
		gCurrentDescInfo.services[i].stDescDCC.APS_control_data;
		gCurrentDescInfo.services[i].stDescDCC.maximum_bitrate;

		gCurrentDescInfo.services[i].stDescDCC.sub_info = 0;

		piArg[0] = &(gCurrentDescInfo.services[i].stDescDCC);

		tcc_manager_demux_desc_update(usServiceID, DIGITALCOPY_CONTROL_ID, (void *)piArg);
	}

	if(flgCONTAupdate == 1)
	{
		ISDBT_TSPARSE_CURDESCINFO_init(usServiceID, CONTENT_AVAILABILITY_ID);

		piArg[0] = gCurrentDescInfo.services[i].stDescCONTA.copy_restriction_mode;
		piArg[1] = gCurrentDescInfo.services[i].stDescCONTA.image_constraint_token;
		piArg[2] = gCurrentDescInfo.services[i].stDescCONTA.retention_mode;
		piArg[3] = gCurrentDescInfo.services[i].stDescCONTA.retention_state;
		piArg[4] = gCurrentDescInfo.services[i].stDescCONTA.encryption_mode = 0x1;

		tcc_manager_demux_desc_update(usServiceID, CONTENT_AVAILABILITY_ID, (void *)piArg);
	}

	return 0;
}

int isdbt_mvtv_info_get_data(int iServiceID, int index, unsigned char *pucComponentTag, int *piComponentCount)
{
	int i, j;
	for(i=0;i<MAX_SUPPORT_CURRENT_SERVICE;i++)
		if(gCurrentDescInfo.services[i].usServiceID == (unsigned short)iServiceID) break;
	if(i == MAX_SUPPORT_CURRENT_SERVICE) {
		PARSE_DBG("%s there is no service[0x%x] info", __func__, iServiceID);
		return -1;
	}
	if(gCurrentDescInfo.services[i].stDescCGD.component_group_type != 0x0 || pucComponentTag == NULL || piComponentCount == NULL) {
		return (int)gCurrentDescInfo.services[i].stDescCGD.component_group_type;
	}
	*piComponentCount = gCurrentDescInfo.services[i].stDescCGD.num_of_component[index][0];
	for(j=0;j<gCurrentDescInfo.services[i].stDescCGD.num_of_component[index][0];j++) {
		pucComponentTag[j] = gCurrentDescInfo.services[i].stDescCGD.component_tag[index][0][j];
	}

	return (int)gCurrentDescInfo.services[i].stDescCGD.component_group_type;
}

