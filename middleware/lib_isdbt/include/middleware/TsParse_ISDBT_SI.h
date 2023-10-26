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
#ifndef	_TSPARSE_ISDBT_SI_H_
#define	_TSPARSE_ISDBT_SI_H_

/*
  table --> subtable --+-> prev -----> section --+---> global descriptor
                      |          |                            |          |
                      |         +-> curr                  |         +---> loop ---> descriptor
                      |                                        |                     |
                      |                                        |                   loop ---> descriptor
                 subtable                              section                 |
                      |                                        |                    ...
                     ...                                      ...
[<-------- table ------------>][<--- sectin ---->][<--sectin loop-->]

*********
Delete - effect to specified element
Destroy - effect to all element of list.
Get - retrieve an information of root of list
Find - retrieve an information of specified element
*********
 */

#ifdef __cplusplus
extern "C" {
#endif

/* definitoin of PSI/SI of program */

#include "TsParse.h"
#include "mpeg_def.h"
#include "mpeg_ext.h"

#define	TSPARSE_SIMGMT_OK			(0)
#define	TSPARSE_SIMGMT_INVALIDPARAM	(-1)
#define	TSPARSE_SIMGMT_MEMORYALLOCFAIL	(-2)
#define TSPARSE_SIMGMT_INVALIDDATA	(-3)
#define	TSPARSE_SIMGMT_ADDDUPLICATE (-4)
#define TSPARSE_SIMGMT_INVALIDSTATE (-5)


#define	CHK_VALIDPTR(ptr) (ptr!=NULL)

enum enumSectionPrevCurr {
	eSECTION_PREV=0,
	eSECTION_CURR,
};

typedef enum {
	TSPARSE_SIMGMT_STATE_INIT,
	TSPARSE_SIMGMT_STATE_RUN,
} E_SI_MGMT_STATE;

/*----- section loop -----*/
typedef struct _si_mgmt_section_loop_data_pat {
	unsigned short program_number;
	unsigned short program_map_pid;
} SI_MGMT_SECTIONLOOP_DATA_PAT;
typedef struct _si_mgmt_section_loop_data_pmt {
	unsigned char stream_type;
	unsigned short elementary_PID;
} SI_MGMT_SECTIONLOOP_DATA_PMT;
typedef struct _si_mgmt_section_loop_data_bit {
	unsigned char broadcaster_id;
} SI_MGMT_SECTIONLOOP_DATA_BIT;
typedef struct _si_mgmt_section_loop_data_nit {
	unsigned short transport_stream_id;
	unsigned short original_network_id;
} SI_MGMT_SECTIONLOOP_DATA_NIT;
typedef struct _si_mgmt_section_loop_data_sdt {
	unsigned short service_id;
	unsigned char H_EIT_flag;
	unsigned char M_EIT_flag;
	unsigned char L_EIT_flag;
	unsigned char EIT_schedule_flag;
	unsigned char EIT_present_following_flag;
	RUNNING_STATUS running_status;
	unsigned char free_CA_mode;
} SI_MGMT_SECTIONLOOP_DATA_SDT;
typedef struct _si_mgmt_section_loop_data_eit {
	unsigned short event_id;
	DATE_TIME_STRUCT	stStartTime;
	TIME_STRUCT			stDuration;
	RUNNING_STATUS	running_status;;
	unsigned char free_CA_mode;
} SI_MGMT_SECTIONLOOP_DATA_EIT;
typedef union _si_mgmt_section_loop_data {
	SI_MGMT_SECTIONLOOP_DATA_PAT	pat_loop;
	SI_MGMT_SECTIONLOOP_DATA_PMT	pmt_loop;
	SI_MGMT_SECTIONLOOP_DATA_BIT	bit_loop;
	SI_MGMT_SECTIONLOOP_DATA_NIT	nit_loop;
	SI_MGMT_SECTIONLOOP_DATA_SDT	sdt_loop;
	SI_MGMT_SECTIONLOOP_DATA_EIT	eit_loop;
} SI_MGMT_SECTIONLOOP_DATA;
typedef struct _si_mgmt_section_loop {
	SI_MGMT_SECTIONLOOP_DATA unSectionLoopData;
	DESCRIPTOR_STRUCT *pDesc;
	struct _si_mgmt_section *pParentSection;
	struct _si_mgmt_section_loop *pNext;
	struct _si_mgmt_section_loop *pPrev;
} SI_MGMT_SECTIONLOOP;

/* ----- section ----- */
typedef struct _si_mgmt_section_extension_pmt {
	unsigned short PCR_PID;
} SI_MGMT_SECTION_EXTENSION_PMT;
typedef struct _si_mgmt_section_extension_eit {
	unsigned char segment_last_section_number;
} SI_MGMT_SECTION_EXTENSION_EIT;
typedef struct _si_mgmt_section_extension_cdt {
	unsigned char data_type;
} SI_MGMT_SECTION_EXTENSION_CDT;
typedef struct _si_mgmt_section_extension_tot {
	DATE_TIME_STRUCT stDateTime;
} SI_MGMT_SECTION_EXTENSION_TOT;
typedef union _si_mgmt_section_sextension {
	SI_MGMT_SECTION_EXTENSION_PMT pmt_sec_ext;
	SI_MGMT_SECTION_EXTENSION_EIT eit_sec_ext;
	SI_MGMT_SECTION_EXTENSION_CDT cdt_sec_ext;
	SI_MGMT_SECTION_EXTENSION_TOT tot_sec_ext;
} SI_MGMT_SECTION_EXTENSION;
typedef struct _si_mgmt_section {
	unsigned char version_number;
	unsigned char section_number;
	unsigned char last_section_number;
	SI_MGMT_SECTION_EXTENSION unSectionExt;
	DESCRIPTOR_STRUCT *pGlobalDesc;
	SI_MGMT_SECTIONLOOP *pSectionLoop;
	struct _si_mgmt_table *pParentTable;
	struct _si_mgmt_section *pNext;
	struct _si_mgmt_section *pPrev;
} SI_MGMT_SECTION;

/* ----- table ----- */
typedef struct _si_mgmt_table_extension_pat {
	unsigned short transport_stream_id;
} SI_MGMT_TABLE_EXTENSION_PAT;
typedef struct _si_mgmt_table_extension_pmt {
	unsigned short program_number;
} SI_MGMT_TABLE_EXTENSION_PMT;
typedef struct _si_mgmt_table_extension_bit {
	unsigned short original_network_id;
} SI_MGMT_TABLE_EXTENSION_BIT;
typedef struct _si_mgmt_table_extension_nit {
	unsigned short network_id;
} SI_MGMT_TABLE_EXTENSION_NIT;
typedef struct _si_mgmt_table_extension_sdt {
	unsigned short transport_stream_id;
	unsigned short original_network_id;
} SI_MGMT_TABLE_EXTENSION_SDT;
typedef struct _si_mgmt_table_extension_eit {
	unsigned short service_id;
	unsigned short transport_stream_id;
	unsigned short original_network_id;
	unsigned char last_table_id;
} SI_MGMT_TABLE_EXTENSION_EIT;
typedef struct _si_mgmt_table_extension_cdt {
	unsigned short original_network_id;
 	unsigned short download_data_id;
} SI_MGMT_TABLE_EXTENSION_CDT;
typedef union _si_mgmt_table_extension {
	SI_MGMT_TABLE_EXTENSION_PAT pat_ext;
	SI_MGMT_TABLE_EXTENSION_PMT pmt_ext;
	SI_MGMT_TABLE_EXTENSION_BIT bit_ext;
	SI_MGMT_TABLE_EXTENSION_NIT nit_ext;
	SI_MGMT_TABLE_EXTENSION_SDT sdt_ext;
	SI_MGMT_TABLE_EXTENSION_EIT eit_ext;
	SI_MGMT_TABLE_EXTENSION_CDT cdt_ext;
} SI_MGMT_TABLE_EXTENSION;

typedef struct _si_mgmt_table {
	MPEG_TABLE_IDS	table_id;
	SI_MGMT_TABLE_EXTENSION unTableExt;
	SI_MGMT_SECTION *pSecPrev;
	SI_MGMT_SECTION *pSecCurr;
	struct _si_mgmt_table *pNext;
	struct _si_mgmt_table *pPrev;
	struct _si_mgmt_table_root *pParentTableRoot;
} SI_MGMT_TABLE;

/*---------- top ----------*/
typedef struct _si_mgmt_table_root {
	SI_MGMT_TABLE *pTable;
} SI_MGMT_TABLE_ROOT;
typedef struct _si_mgmt_information {
	SI_MGMT_TABLE_ROOT PAT;
	SI_MGMT_TABLE_ROOT CAT;
	SI_MGMT_TABLE_ROOT PMT;
	SI_MGMT_TABLE_ROOT BIT;
	SI_MGMT_TABLE_ROOT NIT;
	SI_MGMT_TABLE_ROOT SDT;
	SI_MGMT_TABLE_ROOT EIT;
	SI_MGMT_TABLE_ROOT TOT;
	SI_MGMT_TABLE_ROOT CDT;
	E_SI_MGMT_STATE    eState;
} SI_MGMT_INFORMATION;

/* ======= functions ========== */
/*---------- top ----------*/
void TSPARSE_ISDBT_SiMgmt_TEST_DisplayTable(MPEG_TABLE_IDS table_id, SI_MGMT_TABLE_EXTENSION *pTableExt);
void TSPARSE_ISDBT_SiMgmt_TEST_DisplayDescriptor (DESCRIPTOR_STRUCT *pDesc);

int TSPARSE_ISDBT_SiMgmt_Init(void);
int TSPARSE_ISDBT_SiMgmt_Deinit(void);
int TSPARSE_ISDBT_SiMgmt_Clear (void);
int TSPARSE_ISDBT_SiMgmt_GetTableRoot(SI_MGMT_TABLE_ROOT **ppTableRoot, MPEG_TABLE_IDS table_id);
int TSPARSE_ISDBT_SiMgmt_GetTable (SI_MGMT_TABLE **ppTable, SI_MGMT_TABLE_ROOT *pTableRoot);
int TSPARSE_ISDBT_SiMgmt_FindTable(SI_MGMT_TABLE **ppTable, MPEG_TABLE_IDS table_id, SI_MGMT_TABLE_EXTENSION *punTableExt, SI_MGMT_TABLE_ROOT *pTableRoot);
int TSPARSE_ISDBT_SiMgmt_AddTable (SI_MGMT_TABLE **ppTable, MPEG_TABLE_IDS table_id, SI_MGMT_TABLE_EXTENSION *punTableExt, SI_MGMT_TABLE_ROOT *pTableRoot);
int TSPARSE_ISDBT_SiMgmt_DeleteTable(SI_MGMT_TABLE *pTable, SI_MGMT_TABLE_ROOT *pTableRoot);
int TSPARSE_ISDBT_SiMgmt_DeleteTableExt (MPEG_TABLE_IDS table_id, SI_MGMT_TABLE_EXTENSION *punTableExt, SI_MGMT_TABLE_ROOT *pTableRoot);
int TSPARSE_ISDBT_SiMgmt_DistroyTable(SI_MGMT_TABLE_ROOT *pTableRoot);

/*---------- table ----------*/
int TSPARSE_ISDBT_SiMgmt_Table_SectionTransit(SI_MGMT_TABLE *pTable);
int TSPARSE_ISDBT_SiMgmt_Table_FindSection (SI_MGMT_SECTION **ppSection, int fPrevCurr, unsigned char section_number, SI_MGMT_TABLE *pTable);
int TSPARSE_ISDBT_SiMgmt_Table_AddSection (SI_MGMT_SECTION **ppSection, int fPrevCurr, unsigned char version_number, unsigned char section_number,	unsigned char last_section_number, SI_MGMT_SECTION_EXTENSION *punSectionExt, SI_MGMT_TABLE *pTable);
int TSPARSE_ISDBT_SiMgmt_Table_DeleteSection (int fPrevCurr, SI_MGMT_SECTION *pSection, SI_MGMT_TABLE *pTable);
int TSPARSE_ISDBT_SiMgmt_Table_DeleteSectionExt(int fPrevCurr, unsigned char section_number, SI_MGMT_TABLE *pTable);
int TSPARSE_ISDBT_SiMgmt_Table_DestroySection (int fPrevCurr, SI_MGMT_TABLE *pTable);
int TSPARSE_ISDBT_SiMgmt_Table_GetSection(SI_MGMT_SECTION **ppSection, int fPrevCurr, SI_MGMT_TABLE *pTable);
int TSPARSE_ISDBT_SiMgmt_Table_GetTableID(MPEG_TABLE_IDS *p_table_id, SI_MGMT_TABLE *pTable);
int TSPARSE_ISDBT_SiMgmt_Table_GetTableExt(SI_MGMT_TABLE_EXTENSION **ppunTableExt, SI_MGMT_TABLE *pTable);
int TSPARSE_ISDBT_SiMgmt_Table_GetParentRoot (SI_MGMT_TABLE_ROOT **ppTableRoot, SI_MGMT_TABLE *pTable);
int TSPARSE_ISDBT_SiMgmt_Table_GetNextTable(SI_MGMT_TABLE **ppTableNext, SI_MGMT_TABLE *pTable);
int TSPARSE_ISDBT_SiMgmt_Table_SetTableExt (SI_MGMT_TABLE_EXTENSION *punTableExt, SI_MGMT_TABLE *pTable);

/*---------- section ----------*/
int TSPARSE_ISDBT_SiMgmt_Section_FindLoop (SI_MGMT_SECTIONLOOP **ppSectionLoop, SI_MGMT_SECTIONLOOP_DATA *pSectionLoopData, SI_MGMT_SECTION *pSection);
int TSPARSE_ISDBT_SiMgmt_Section_AddLoop(SI_MGMT_SECTIONLOOP **ppSectionLoop, SI_MGMT_SECTIONLOOP_DATA *punSectionLoopData, SI_MGMT_SECTION *pSection);
int TSPARSE_ISDBT_SiMgmt_Section_DeleteLoop(SI_MGMT_SECTIONLOOP* pSectionLoop, SI_MGMT_SECTION *pSection);
int TSPARSE_ISDBT_SiMgmt_Section_DeleteLoopExt(SI_MGMT_SECTIONLOOP_DATA* punSectionLoopData, SI_MGMT_SECTION *pSection);
int TSPARSE_ISDBT_SiMgmt_Section_DestroyLoop (SI_MGMT_SECTION *pSection);
int TSPARSE_ISDBT_SiMgmt_Section_FindDescriptor_Global (DESCRIPTOR_STRUCT **ppDescriptor, DESCRIPTOR_IDS descriptor_id, void *arg, SI_MGMT_SECTION *pSection);
int TSPARSE_ISDBT_SiMgmt_Section_AddDescriptor_Global(DESCRIPTOR_STRUCT **ppDescRtn, DESCRIPTOR_STRUCT *pDescriptor, SI_MGMT_SECTION *pSection);
int TSPARSE_ISDBT_SiMgmt_Section_DeleteDescriptor_Global (DESCRIPTOR_STRUCT *pDescriptor, SI_MGMT_SECTION *pSection);
int TSPARSE_ISDBT_SiMgmt_Section_DeleteDescriptorExt_Global(DESCRIPTOR_IDS descriptor_id, void *arg, SI_MGMT_SECTION *pSection);
int TSPARSE_ISDBT_SiMgmt_Section_DestroyDescriptor_Global(SI_MGMT_SECTION *pSection);
int TSPARSE_ISDBT_SiMgmt_Section_GetVersioNumber(unsigned char *p_version_number, SI_MGMT_SECTION *pSection);
int TSPARSE_ISDBT_SiMgmt_Section_GetSecNo(unsigned char *p_section_number, SI_MGMT_SECTION *pSection);
int TSPARSE_ISDBT_SiMgmt_Section_GetLastSecNo(unsigned char *p_last_section_number, SI_MGMT_SECTION *pSection);
int TSPARSE_ISDBT_SiMgmt_Section_GetSectionExt(SI_MGMT_SECTION_EXTENSION **ppunSectionExt, SI_MGMT_SECTION *pSection);
int TSPARSE_ISDBT_SiMgmt_Section_GetDescriptor_Global(DESCRIPTOR_STRUCT **ppDescriptor, SI_MGMT_SECTION *pSection);
int TSPARSE_ISDBT_SiMgmt_Section_GetLoop(SI_MGMT_SECTIONLOOP **ppSectionLoop, SI_MGMT_SECTION *pSection);
int TSPARSE_ISDBT_SiMgmt_Section_GetParentTable(SI_MGMT_TABLE **ppTable, SI_MGMT_SECTION *pSection);
int TSPARSE_ISDBT_SiMgmt_Section_GetNextSection(SI_MGMT_SECTION **ppSection, SI_MGMT_SECTION *pSection);
int TSPARSE_ISDBT_SiMgmt_Section_SetSectionExt (SI_MGMT_SECTION_EXTENSION *punSectinExt, SI_MGMT_SECTION *pSection);

/*---------- section loop -------*/
int TSPARSE_ISDBT_SiMgmt_SectionLoop_FindDescriptor(DESCRIPTOR_STRUCT **ppDescriptor, DESCRIPTOR_IDS descriptor_id, void *arg, SI_MGMT_SECTIONLOOP *pSectionLoop);
int TSPARSE_ISDBT_SiMgmt_SectionLoop_AddDescriptor(DESCRIPTOR_STRUCT **ppDescRtn, DESCRIPTOR_STRUCT *pDescriptor, SI_MGMT_SECTIONLOOP *pSectionLoop);
int TSPARSE_ISDBT_SiMgmt_SectionLoop_DeleteDescriptor(DESCRIPTOR_STRUCT *pDescriptor, SI_MGMT_SECTIONLOOP *pSectionLoop);
int TSPARSE_ISDBT_SiMgmt_SectionLoop_DeleteDescriptorExt(DESCRIPTOR_IDS descriptor_id, void *arg, SI_MGMT_SECTIONLOOP *pSectionLoop);
int TSPARSE_ISDBT_SiMgmt_SectionLoop_DestroyDescriptor (SI_MGMT_SECTIONLOOP *pSectionLoop);
int TSPARSE_ISDBT_SiMgmt_SectionLoop_GetLoopData(SI_MGMT_SECTIONLOOP_DATA **ppunSectionLoopData, SI_MGMT_SECTIONLOOP *pSectionLoop);
int TSPARSE_ISDBT_SiMgmt_SectionLoop_GetDescriptor(DESCRIPTOR_STRUCT **ppDescriptor, SI_MGMT_SECTIONLOOP *pSectionLoop);
int TSPARSE_ISDBT_SiMgmt_SectionLoop_GetParentSection (SI_MGMT_SECTION **ppSection, SI_MGMT_SECTIONLOOP *pSectionLoop);
int TSPARSE_ISDBT_SiMgmt_SectionLoop_GetNextLoop(SI_MGMT_SECTIONLOOP **ppSectionLoop, SI_MGMT_SECTIONLOOP *pSectionLoop);
int TSPARSE_ISDBT_SiMgmt_SectionLoop_SetLoopData (SI_MGMT_SECTIONLOOP_DATA *punSectionLoopData, SI_MGMT_SECTIONLOOP *pSectionLoop);

/*--------descriptor ---------*/
int TSPARSE_ISDBT_SiMgmt_Descriptor_Add (DESCRIPTOR_STRUCT **ppDescRtn, DESCRIPTOR_STRUCT *pDescriptor, DESCRIPTOR_STRUCT *pSectionDescriptorList);
int TSPARSE_ISDBT_SiMgmt_Descriptor_Find(DESCRIPTOR_STRUCT **ppDescriptor, DESCRIPTOR_IDS descriptor_id, void *arg, DESCRIPTOR_STRUCT *pSectionDescripotrList);

/* --------- util ----------*/
int TSPARSE_ISDBT_SiMgmt_Util_GetNITSection(SI_MGMT_SECTION **ppSec);
int TSPARSE_ISDBT_SiMgmt_Util_GetPATSection(SI_MGMT_SECTION **ppSec);

#ifdef __cplusplus
}
#endif
#endif	//_TSPARSE_ISDBT_H_
