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
#define LOG_TAG "SiMgmt"
#include <utils/Log.h>
#endif

#define ERR_MSG	ALOGE
#define DBG_MSG //ALOGD

#include <pthread.h>
#include "TsParse_ISDBT_SI.h"

#define	LOOP_LIMIT	256 /* to avoid an endless-loop */

SI_MGMT_INFORMATION stSiMgmtInfo;

static pthread_mutex_t lock_simgmt;
#define LOCK_START	tsparse_isdbt_simgmt_lock_on
#define LOCK_STOP	tsparse_isdbt_simgmt_lock_off

/* ----- local functions -----*/
#ifdef __cplusplus
extern "C" {
#endif
int tsparse_isdbt_simgmt_GetTableRoot(SI_MGMT_TABLE_ROOT **ppTableRoot, MPEG_TABLE_IDS table_id);
int tsparse_isdbt_simgmt_GetTable (SI_MGMT_TABLE **ppTable, SI_MGMT_TABLE_ROOT *pTableRoot);
int tsparse_isdbt_simgmt_DistroyTable(SI_MGMT_TABLE_ROOT *pTableRoot);
int tsparse_isdbt_simgmt_FindTable(SI_MGMT_TABLE **ppTable, MPEG_TABLE_IDS table_id, SI_MGMT_TABLE_EXTENSION *punTableExt, SI_MGMT_TABLE_ROOT *pTableRoot);
int tsparse_isdbt_simgmt_AddTable (SI_MGMT_TABLE **ppTable, MPEG_TABLE_IDS table_id, SI_MGMT_TABLE_EXTENSION *punTableExt, SI_MGMT_TABLE_ROOT *pTableRoot);
int tsparse_isdbt_simgmt_DeleteTable(SI_MGMT_TABLE *pTable, SI_MGMT_TABLE_ROOT *pTableRoot);
int tsparse_isdbt_simgmt_DeleteTableExt (MPEG_TABLE_IDS table_id, SI_MGMT_TABLE_EXTENSION *punTableExt, SI_MGMT_TABLE_ROOT *pTableRoot);
int tsparse_isdbt_simgmt_Table_SectionTransit(SI_MGMT_TABLE *pTable);
int tsparse_isdbt_simgmt_Table_AddSection (SI_MGMT_SECTION **ppSection, int fPrevCurr, unsigned char version_number, unsigned char section_number,	unsigned char last_section_number, SI_MGMT_SECTION_EXTENSION *punSectionExt, SI_MGMT_TABLE *pTable);
int tsparse_isdbt_simgmt_Table_DeleteSection (int fPrevCurr, SI_MGMT_SECTION *pSection, SI_MGMT_TABLE *pTable);
int tsparse_isdbt_simgmt_Table_DeleteSectionExt(int fPrevCurr, unsigned char section_number, SI_MGMT_TABLE *pTable);
int tsparse_isdbt_simgmt_Table_DestroySection(int fPrevCurr, SI_MGMT_TABLE *pTable);
int tsparse_isdbt_simgmt_Table_GetTableID(MPEG_TABLE_IDS *p_table_id, SI_MGMT_TABLE *pTable);
int tsparse_isdbt_simgmt_Table_GetTableExt(SI_MGMT_TABLE_EXTENSION **punTableExt, SI_MGMT_TABLE *pTable);
int tsparse_isdbt_simgmt_Table_GetParentRoot (SI_MGMT_TABLE_ROOT **ppTableRoot, SI_MGMT_TABLE *pTable);
int tsparse_isdbt_simgmt_Table_GetNextTable(SI_MGMT_TABLE **ppTableNext, SI_MGMT_TABLE *pTable);
int tsparse_isdbt_simgmt_Table_SetTableExt (SI_MGMT_TABLE_EXTENSION *punTableExt, SI_MGMT_TABLE *pTable);
int tsparse_isdbt_simgmt_Table_FindSection (SI_MGMT_SECTION **ppSection, int fPrevCurr, unsigned char section_number, SI_MGMT_TABLE *pTable);
int tsparse_isdbt_simgmt_Table_GetSection(SI_MGMT_SECTION **ppSection, int fPrevCurr, SI_MGMT_TABLE *pTable);
int tsparse_isdbt_simgmt_Table_GetSection(SI_MGMT_SECTION **ppSection, int fPrevCurr, SI_MGMT_TABLE *pTable);
int tsparse_isdbt_simgmt_Section_AddLoop(SI_MGMT_SECTIONLOOP **ppSectionLoop, SI_MGMT_SECTIONLOOP_DATA *punSectionLoopData, SI_MGMT_SECTION *pSection);
int tsparse_isdbt_simgmt_Section_FindLoop (SI_MGMT_SECTIONLOOP **ppSectionLoop, SI_MGMT_SECTIONLOOP_DATA *pSectionLoopData, SI_MGMT_SECTION *pSection);
int tsparse_isdbt_simgmt_Section_DestroyLoop (SI_MGMT_SECTION *pSection);
int tsparse_isdbt_simgmt_Section_DeleteLoop(SI_MGMT_SECTIONLOOP* pSectionLoop, SI_MGMT_SECTION *pSection);
int tsparse_isdbt_simgmt_Section_AddDescriptor_Global(DESCRIPTOR_STRUCT **ppDescRtn, DESCRIPTOR_STRUCT *pDescriptor, SI_MGMT_SECTION *pSection);
int tsparse_isdbt_simgmt_Section_SetSectionExt (SI_MGMT_SECTION_EXTENSION *punSectionExt, SI_MGMT_SECTION *pSection);
int tsparse_isdbt_simgmt_Section_DeleteLoopExt(SI_MGMT_SECTIONLOOP_DATA* punSectionLoopData, SI_MGMT_SECTION *pSection);
int tsparse_isdbt_simgmt_Section_GetVersioNumber(unsigned char *p_version_number, SI_MGMT_SECTION *pSection);
int tsparse_isdbt_simgmt_Section_GetSecNo(unsigned char *p_section_number, SI_MGMT_SECTION *pSection);
int tsparse_isdbt_simgmt_Section_GetLastSecNo(unsigned char *p_last_section_number, SI_MGMT_SECTION *pSection);
int tsparse_isdbt_simgmt_Section_GetSectionExt(SI_MGMT_SECTION_EXTENSION **ppSectionExt, SI_MGMT_SECTION *pSection);
int tsparse_isdbt_simgmt_Section_GetDescriptor_Global(DESCRIPTOR_STRUCT **ppDescriptor, SI_MGMT_SECTION *pSection);
int tsparse_isdbt_simgmt_Section_GetLoop(SI_MGMT_SECTIONLOOP **ppSectionLoop, SI_MGMT_SECTION *pSection);
int tsparse_isdbt_simgmt_Section_GetParentTable(SI_MGMT_TABLE **ppTable, SI_MGMT_SECTION *pSection);
int tsparse_isdbt_simgmt_Section_GetNextSection(SI_MGMT_SECTION **ppSection, SI_MGMT_SECTION *pSection);
int tsparse_isdbt_simgmt_Section_FindDescriptor_Global (DESCRIPTOR_STRUCT **ppDescriptor, DESCRIPTOR_IDS descriptor_id, void *arg, SI_MGMT_SECTION *pSection);
int tsparse_isdbt_sigment_Section_DeleteDescriptor_Global (DESCRIPTOR_STRUCT *pDescriptor, SI_MGMT_SECTION *pSection);
int tsparse_isdbt_simgmt_Section_DeleteDescriptorExt_Global(DESCRIPTOR_IDS descriptor_id, void *arg, SI_MGMT_SECTION *pSection);
int tsparse_isdbt_simgmt_Section_DestroyDescriptor_Global(SI_MGMT_SECTION *pSection);
int tsparse_isdbt_simgmt_SectionLoop_SetLoopData (SI_MGMT_SECTIONLOOP_DATA *pSectionLoopData, SI_MGMT_SECTIONLOOP *pSectionLoop);
int tsparse_isdbt_simgmt_SectionLoop_GetLoopNext(SI_MGMT_SECTIONLOOP **ppSectionLoop, SI_MGMT_SECTIONLOOP *pSectionLoop);
int tsparse_isdbt_simgmt_SectionLoop_GetLoopData(SI_MGMT_SECTIONLOOP_DATA **ppSectionLoopData, SI_MGMT_SECTIONLOOP *pSectionLoop);
int tsparse_isdbt_simgmt_SectionLoop_GetDescriptor(DESCRIPTOR_STRUCT **ppDescriptor, SI_MGMT_SECTIONLOOP *pSectionLoop);
int tsparse_isdbt_simgmt_SectionLoop_GetParentSection (SI_MGMT_SECTION **ppSection, SI_MGMT_SECTIONLOOP *pSectionLoop);
int tsparse_isdbt_simgmt_SectionLoop_GetNextLoop(SI_MGMT_SECTIONLOOP **ppSectionLoop, SI_MGMT_SECTIONLOOP *pSectionLoop);
int tsparse_isdbt_simgmt_SectionLoop_AddDescriptor(DESCRIPTOR_STRUCT **ppDescRtn, DESCRIPTOR_STRUCT *pDescriptor, SI_MGMT_SECTIONLOOP *pSectionLoop);
int tsparse_isdbt_simgmt_SectionLoop_FindDescriptor(DESCRIPTOR_STRUCT **ppDescriptor, DESCRIPTOR_IDS descriptor_id, void *arg, SI_MGMT_SECTIONLOOP *pSectionLoop);
int tsparse_isdbt_simgmt_SectionLoop_DeleteDescriptor (DESCRIPTOR_STRUCT *pDescriptor, SI_MGMT_SECTIONLOOP *pSectionLoop);
int tsparse_isdbt_simgmt_SectionLoop_DeleteDescriptorExt(DESCRIPTOR_IDS descriptor_id, void *arg, SI_MGMT_SECTIONLOOP *pSectionLoop);
int tsparse_isdbt_simgmt_SectionLoop_DestroyDescriptor (SI_MGMT_SECTIONLOOP *pSectionLoop);
int tsparse_isdbt_simgmt_Descriptor_Add (DESCRIPTOR_STRUCT **ppDescRtn, DESCRIPTOR_STRUCT *pDescriptor, DESCRIPTOR_STRUCT *pSectionDescriptorList);
int tsparse_isdbt_simgmt_Descriptor_Find(DESCRIPTOR_STRUCT **ppDescriptor, DESCRIPTOR_IDS descriptor_id, void *arg,DESCRIPTOR_STRUCT *pSectionDescripotrList);
#ifdef __cplusplus
}
#endif

/* ------ lock  --------*/
void tsparse_isdbt_simgmt_lock_init(void)
{
	pthread_mutex_init(&lock_simgmt, NULL);
}

void tsparse_isdbt_simgmt_lock_deinit(void)
{
	pthread_mutex_destroy(&lock_simgmt);
}

void tsparse_isdbt_simgmt_lock_on(void)
{
	pthread_mutex_lock(&lock_simgmt);
}

void tsparse_isdbt_simgmt_lock_off(void)
{
	pthread_mutex_unlock(&lock_simgmt);
}

/* ---------- state management ----------*/

static void tsparse_isdbt_simgmt_state_set(E_SI_MGMT_STATE eState)
{
	stSiMgmtInfo.eState = eState;
}

static E_SI_MGMT_STATE tsparse_isdbt_simgmt_state_get()
{
	return stSiMgmtInfo.eState;
}

/* ---------- utility ----------*/
int tsparse_isdbt_simgmt_Util_GetSection(SI_MGMT_SECTION **ppSec, int fPrevCurr, MPEG_TABLE_IDS table_id)
{
	SI_MGMT_TABLE_ROOT *pTableRoot = (SI_MGMT_TABLE_ROOT*)NULL;
	SI_MGMT_TABLE *pTable = (SI_MGMT_TABLE *)NULL;
	SI_MGMT_SECTION *pSection = (SI_MGMT_SECTION*)NULL;
	int status;

	if (!CHK_VALIDPTR(ppSec))
		return TSPARSE_SIMGMT_INVALIDPARAM;

	if (fPrevCurr != eSECTION_CURR && fPrevCurr != eSECTION_PREV)
		return TSPARSE_SIMGMT_INVALIDPARAM;

	*ppSec = (SI_MGMT_SECTION *)NULL;
	status = tsparse_isdbt_simgmt_GetTableRoot (&pTableRoot, table_id);

	if (status != TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pTableRoot))
		return TSPARSE_SIMGMT_INVALIDDATA;

	status = tsparse_isdbt_simgmt_GetTable(&pTable, pTableRoot);
#if 1
// Noah, To avoid Codesonar's warning, Redundant Condition
// '&pTable' and 'pTableRoot' are not NULL, so status( return value ) is always TSPARSE_SIMGMT_OK.
	if (!CHK_VALIDPTR(pTable))
#else
	if (status != TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pTable))
#endif
		return TSPARSE_SIMGMT_INVALIDDATA;

	status = tsparse_isdbt_simgmt_Table_GetSection	(&pSection, fPrevCurr, pTable);
	if (status != TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pSection))
		return TSPARSE_SIMGMT_INVALIDDATA;

	*ppSec = pSection;

	return status;
}
int TSPARSE_ISDBT_SiMgmt_Util_GetNITSection(SI_MGMT_SECTION **ppSec)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Util_GetSection (ppSec, eSECTION_CURR, NIT_A_ID);
	LOCK_STOP();
	return err;
}
int TSPARSE_ISDBT_SiMgmt_Util_GetPATSection(SI_MGMT_SECTION **ppSec)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Util_GetSection (ppSec, eSECTION_CURR, PAT_ID);
	LOCK_STOP();
	return err;
}
int TSPARSE_ISDBT_SiMgmt_Table_IsPAT(MPEG_TABLE_IDS table_id)
{
	return ((table_id == PAT_ID) ?  1 : 0);
}
int TSPARSE_ISDBT_SiMgmt_Table_IsPMT(MPEG_TABLE_IDS table_id)
{
	return ((table_id == PMT_ID) ?  1 : 0);
}
int TSPARSE_ISDBT_SiMgmt_Table_IsBIT(MPEG_TABLE_IDS table_id)
{
	return ((table_id == BIT_ID) ?  1 : 0);
}
int TSPARSE_ISDBT_SiMgmt_Table_IsNIT(MPEG_TABLE_IDS table_id)
{
	return ((table_id == NIT_A_ID) ?  1 : 0);
}
int TSPARSE_ISDBT_SiMgmt_Table_IsSDT(MPEG_TABLE_IDS table_id)
{
	return ((table_id == SDT_A_ID) ?  1 : 0);
}
int TSPARSE_ISDBT_SiMgmt_Table_IsEIT(MPEG_TABLE_IDS table_id)
{
	if ((table_id == EIT_PF_A_ID) || (table_id >= EIT_SA_0_ID && table_id <=EIT_SA_F_ID)) return 1;
	else return 0;
}
int TSPARSE_ISDBT_SiMgmt_Table_IsTOT(MPEG_TABLE_IDS table_id)
{
	return ((table_id == TOT_ID) ? 1 : 0);
}
int TSPARSE_ISDBT_SiMgmt_Table_IsCDT(MPEG_TABLE_IDS table_id)
{
	return ((table_id == CDT_ID) ? 1 : 0);
}
int TSPARSE_ISDBT_SiMgmt_Table_IsCAT(MPEG_TABLE_IDS table_id)
{
	return ((table_id == CAT_ID) ? 1 : 0);
}
int TSPARSE_ISDBT_SiMgmt_GetTableRootPtrByTableID(MPEG_TABLE_IDS table_id, SI_MGMT_TABLE_ROOT **ppTableRoot)
{
	int err = TSPARSE_SIMGMT_OK;

	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN)
		return TSPARSE_SIMGMT_INVALIDSTATE;

	if (!CHK_VALIDPTR(ppTableRoot))
		return TSPARSE_SIMGMT_INVALIDPARAM;

	if(TSPARSE_ISDBT_SiMgmt_Table_IsPAT(table_id)) *ppTableRoot = &stSiMgmtInfo.PAT;
	else if (TSPARSE_ISDBT_SiMgmt_Table_IsPMT(table_id)) *ppTableRoot = &stSiMgmtInfo.PMT;
	else if (TSPARSE_ISDBT_SiMgmt_Table_IsBIT(table_id)) *ppTableRoot = &stSiMgmtInfo.BIT;
	else if (TSPARSE_ISDBT_SiMgmt_Table_IsNIT(table_id)) *ppTableRoot = &stSiMgmtInfo.NIT;
	else if (TSPARSE_ISDBT_SiMgmt_Table_IsSDT(table_id)) *ppTableRoot = &stSiMgmtInfo.SDT;
	else if (TSPARSE_ISDBT_SiMgmt_Table_IsEIT(table_id)) *ppTableRoot = &stSiMgmtInfo.EIT;
	else if (TSPARSE_ISDBT_SiMgmt_Table_IsTOT(table_id)) *ppTableRoot = &stSiMgmtInfo.TOT;
	else if (TSPARSE_ISDBT_SiMgmt_Table_IsCDT(table_id)) *ppTableRoot = &stSiMgmtInfo.CDT;
	else if (TSPARSE_ISDBT_SiMgmt_Table_IsCAT(table_id)) *ppTableRoot = &stSiMgmtInfo.CAT;
	else {
		*ppTableRoot = (SI_MGMT_TABLE_ROOT *)NULL;
		err = TSPARSE_SIMGMT_INVALIDDATA;
	}

	return err;
}
int TSPARSE_ISDBT_SiMgmt_CmpTableExtPAT (SI_MGMT_TABLE_EXTENSION *punTableExtSrc, SI_MGMT_TABLE_EXTENSION *punTableExtDst)
{
	SI_MGMT_TABLE_EXTENSION_PAT *pPatExtSrc = NULL, *pPatExtDst = NULL;
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) return TSPARSE_SIMGMT_INVALIDSTATE;
	if(!CHK_VALIDPTR(punTableExtSrc) || !CHK_VALIDPTR(punTableExtDst)) return TSPARSE_SIMGMT_INVALIDPARAM;
	pPatExtSrc = &(punTableExtSrc->pat_ext);
	pPatExtDst = &(punTableExtDst->pat_ext);

	if (pPatExtSrc->transport_stream_id == pPatExtDst->transport_stream_id) return TSPARSE_SIMGMT_OK;
	else return TSPARSE_SIMGMT_INVALIDDATA;
}
int TSPARSE_ISDBT_SiMgmt_CmpTableExtPMT (SI_MGMT_TABLE_EXTENSION *punTableExtSrc, SI_MGMT_TABLE_EXTENSION *punTableExtDst)
{
	SI_MGMT_TABLE_EXTENSION_PMT *pPmtExtSrc = NULL, *pPmtExtDst = NULL;
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) return TSPARSE_SIMGMT_INVALIDSTATE;
	if(!CHK_VALIDPTR(punTableExtSrc) || !CHK_VALIDPTR(punTableExtDst)) return TSPARSE_SIMGMT_INVALIDPARAM;
	pPmtExtSrc = &(punTableExtSrc->pmt_ext);
	pPmtExtDst = &(punTableExtDst->pmt_ext);

	if (pPmtExtSrc->program_number == pPmtExtDst->program_number) return TSPARSE_SIMGMT_OK;
	else return TSPARSE_SIMGMT_INVALIDDATA;
}
int TSPARSE_ISDBT_SiMgmt_CmpTableExtBIT (SI_MGMT_TABLE_EXTENSION *punTableExtSrc, SI_MGMT_TABLE_EXTENSION *punTableExtDst)
{
	SI_MGMT_TABLE_EXTENSION_BIT *pBitExtSrc = NULL, *pBitExtDst = NULL;
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) return TSPARSE_SIMGMT_INVALIDSTATE;
	if(!CHK_VALIDPTR(punTableExtSrc) || !CHK_VALIDPTR(punTableExtDst)) return TSPARSE_SIMGMT_INVALIDPARAM;
	pBitExtSrc = &(punTableExtSrc->bit_ext);
	pBitExtDst = &(punTableExtDst->bit_ext);

	if (pBitExtSrc->original_network_id == pBitExtDst->original_network_id) return TSPARSE_SIMGMT_OK;
	else return TSPARSE_SIMGMT_INVALIDDATA;
}
int TSPARSE_ISDBT_SiMgmt_CmpTableExtNIT (SI_MGMT_TABLE_EXTENSION *punTableExtSrc, SI_MGMT_TABLE_EXTENSION *punTableExtDst)
{
	SI_MGMT_TABLE_EXTENSION_NIT *pNitExtSrc = NULL, *pNitExtDst = NULL;
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) return TSPARSE_SIMGMT_INVALIDSTATE;
	if(!CHK_VALIDPTR(punTableExtSrc) || !CHK_VALIDPTR(punTableExtDst)) return TSPARSE_SIMGMT_INVALIDPARAM;
	pNitExtSrc = &(punTableExtSrc->nit_ext);
	pNitExtDst = &(punTableExtDst->nit_ext);

	if (pNitExtSrc->network_id == pNitExtDst->network_id) return TSPARSE_SIMGMT_OK;
	else return TSPARSE_SIMGMT_INVALIDDATA;
}
int TSPARSE_ISDBT_SiMgmt_CmpTableExtSDT (SI_MGMT_TABLE_EXTENSION *punTableExtSrc, SI_MGMT_TABLE_EXTENSION *punTableExtDst)
{
	SI_MGMT_TABLE_EXTENSION_SDT *pSdtExtSrc = NULL, *pSdtExtDst = NULL;
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) return TSPARSE_SIMGMT_INVALIDSTATE;
	if(!CHK_VALIDPTR(punTableExtSrc) || !CHK_VALIDPTR(punTableExtDst)) return TSPARSE_SIMGMT_INVALIDPARAM;
	pSdtExtSrc = &(punTableExtSrc->sdt_ext);
	pSdtExtDst = &(punTableExtDst->sdt_ext);

	if ((pSdtExtSrc->transport_stream_id == pSdtExtDst->transport_stream_id)
		&& (pSdtExtSrc->original_network_id == pSdtExtDst->original_network_id)) return TSPARSE_SIMGMT_OK;
	else return TSPARSE_SIMGMT_INVALIDDATA;
}
int TSPARSE_ISDBT_SiMgmt_CmpTableExtEIT(SI_MGMT_TABLE_EXTENSION *punTableExtSrc, SI_MGMT_TABLE_EXTENSION *punTableExtDst)
{
	SI_MGMT_TABLE_EXTENSION_EIT *pEitExtSrc = NULL, *pEitExtDst = NULL;
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) return TSPARSE_SIMGMT_INVALIDSTATE;
	if(!CHK_VALIDPTR(punTableExtSrc) || !CHK_VALIDPTR(punTableExtDst)) return TSPARSE_SIMGMT_INVALIDPARAM;
	pEitExtSrc = &(punTableExtSrc->eit_ext);
	pEitExtDst = &(punTableExtDst->eit_ext);

	if ((pEitExtSrc->service_id == pEitExtDst->service_id)
		&& (pEitExtSrc->original_network_id == pEitExtDst->original_network_id)
		&& (pEitExtSrc->transport_stream_id == pEitExtDst->transport_stream_id)) return TSPARSE_SIMGMT_OK;
	else return TSPARSE_SIMGMT_INVALIDDATA;
}
int TSPARSE_ISDBT_SiMgmt_CmpTableExtCDT(SI_MGMT_TABLE_EXTENSION *punTableExtSrc, SI_MGMT_TABLE_EXTENSION *punTableExtDst)
{
	SI_MGMT_TABLE_EXTENSION_CDT *pCdtExtSrc = NULL, *pCdtExtDst = NULL;
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) return TSPARSE_SIMGMT_INVALIDSTATE;
	if(!CHK_VALIDPTR(punTableExtSrc) || !CHK_VALIDPTR(punTableExtDst)) return TSPARSE_SIMGMT_INVALIDPARAM;
	pCdtExtSrc = &(punTableExtSrc->cdt_ext);
	pCdtExtDst = &(punTableExtDst->cdt_ext);

	if((pCdtExtSrc->original_network_id == pCdtExtDst->original_network_id)
		&& (pCdtExtSrc->download_data_id == pCdtExtDst->download_data_id))
		return TSPARSE_SIMGMT_OK;
	else return TSPARSE_SIMGMT_INVALIDDATA;
}
int TSPARSE_ISDBT_SiMgmt_CmpTableExt(MPEG_TABLE_IDS table_id, SI_MGMT_TABLE_EXTENSION *punTableExtSrc, SI_MGMT_TABLE_EXTENSION *punTableExtDst)
{
	int err;
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) return TSPARSE_SIMGMT_INVALIDSTATE;
	if(TSPARSE_ISDBT_SiMgmt_Table_IsPAT(table_id)) err = TSPARSE_ISDBT_SiMgmt_CmpTableExtPAT(punTableExtSrc, punTableExtDst);
	else if (TSPARSE_ISDBT_SiMgmt_Table_IsPMT(table_id)) err = TSPARSE_ISDBT_SiMgmt_CmpTableExtPMT(punTableExtSrc, punTableExtDst);
	else if (TSPARSE_ISDBT_SiMgmt_Table_IsBIT(table_id)) err = TSPARSE_ISDBT_SiMgmt_CmpTableExtBIT(punTableExtSrc, punTableExtDst);
	else if (TSPARSE_ISDBT_SiMgmt_Table_IsNIT(table_id)) err = TSPARSE_ISDBT_SiMgmt_CmpTableExtNIT(punTableExtSrc, punTableExtDst);
	else if (TSPARSE_ISDBT_SiMgmt_Table_IsSDT(table_id)) err = TSPARSE_ISDBT_SiMgmt_CmpTableExtSDT(punTableExtSrc, punTableExtDst);
	else if (TSPARSE_ISDBT_SiMgmt_Table_IsEIT(table_id)) err = TSPARSE_ISDBT_SiMgmt_CmpTableExtEIT(punTableExtSrc, punTableExtDst);
	else if (TSPARSE_ISDBT_SiMgmt_Table_IsTOT(table_id)) err = TSPARSE_SIMGMT_OK;
	else if (TSPARSE_ISDBT_SiMgmt_Table_IsCDT(table_id)) err = TSPARSE_ISDBT_SiMgmt_CmpTableExtCDT(punTableExtSrc, punTableExtDst);
	else if (TSPARSE_ISDBT_SiMgmt_Table_IsCAT(table_id)) err = TSPARSE_SIMGMT_OK;
	else err = TSPARSE_SIMGMT_INVALIDDATA;
	return err;
}

int TSPARSE_ISDBT_SiMgmt_CopyDescriptor (DESCRIPTOR_STRUCT **ppDescDest, DESCRIPTOR_STRUCT *pDescSrc)
{
	int size, cnt, err = TSPARSE_SIMGMT_OK;
	DESCRIPTOR_STRUCT *pDesc;

	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN)
		return TSPARSE_SIMGMT_INVALIDSTATE;

	if (!CHK_VALIDPTR(ppDescDest) || !CHK_VALIDPTR(pDescSrc))
		return TSPARSE_SIMGMT_INVALIDPARAM;

	*ppDescDest = (DESCRIPTOR_STRUCT*)NULL;
	size = pDescSrc->struct_length;

	switch (pDescSrc->enDescriptorID)
	{
		case SERVICE_LIST_DESCR_ID:
			pDesc = (DESCRIPTOR_STRUCT*)tcc_mw_malloc(__FUNCTION__, __LINE__, size);
			//memset(pDesc, 0, size);
			if (CHK_VALIDPTR(pDesc))
			{
				memcpy (pDesc, pDescSrc, size);
				cnt = (char*)pDescSrc->unDesc.stSLD.pastSvcList - (char*)pDescSrc;
				pDesc->unDesc.stSLD.pastSvcList = (SERVICE_LIST_STRUCT*)((char*)pDesc + cnt);
				*ppDescDest = pDesc;
			}
			else
				err = TSPARSE_SIMGMT_MEMORYALLOCFAIL;
			break;
		case NETWORK_NAME_DESCR_ID:
			pDesc = (DESCRIPTOR_STRUCT*)tcc_mw_malloc(__FUNCTION__, __LINE__, size);
			//memset(pDesc, 0, size);
			if (CHK_VALIDPTR(pDesc))
			{
				memcpy (pDesc, pDescSrc, size);
				cnt = (char *)pDescSrc->unDesc.stNND.pcNetName - (char*)pDescSrc;
				pDesc->unDesc.stNND.pcNetName = (char*)((char*)pDesc + cnt);
				*ppDescDest = pDesc;
			}
			else
				err = TSPARSE_SIMGMT_MEMORYALLOCFAIL;
			break;
		case SERVICE_DESCR_ID:
			pDesc = (DESCRIPTOR_STRUCT*)tcc_mw_malloc(__FUNCTION__, __LINE__, size);
			//memset(pDesc, 0, size);
			if (CHK_VALIDPTR(pDesc))
			{
				memcpy (pDesc, pDescSrc, size);
				cnt = (char*)pDescSrc->unDesc.stSD.pszSvcProvider - (char *)pDescSrc;
				pDesc->unDesc.stSD.pszSvcProvider = (char *)((char*)pDesc + cnt);
				cnt = (char*)pDescSrc->unDesc.stSD.pszSvcName - (char *)pDescSrc;
				pDesc->unDesc.stSD.pszSvcName = (char *)((char*)pDesc + cnt);
				*ppDescDest = pDesc;
			}
			else
				err = TSPARSE_SIMGMT_MEMORYALLOCFAIL;
			break;
		case LINKAGE_DESCR_ID:
			pDesc = (DESCRIPTOR_STRUCT*)tcc_mw_malloc(__FUNCTION__, __LINE__, size);
			//memset(pDesc, 0, size);
			if (CHK_VALIDPTR(pDesc))
			{
				memcpy (pDesc, pDescSrc, size);
				cnt = (char *)(pDescSrc->unDesc.stLD.paucPrivData) - (char *)pDescSrc;
				pDesc->unDesc.stLD.paucPrivData = (unsigned char *)((char*)pDesc + cnt);
				*ppDescDest = pDesc;
			}
			else
				err = TSPARSE_SIMGMT_MEMORYALLOCFAIL;
			break;
		case SHRT_EVT_DESCR_ID:
			pDesc = (DESCRIPTOR_STRUCT*)tcc_mw_malloc(__FUNCTION__, __LINE__, size);
			//memset(pDesc, 0, size);
			if (CHK_VALIDPTR(pDesc))
			{
				memcpy (pDesc, pDescSrc, size);
				cnt = (char *)pDescSrc->unDesc.stSED.pszShortEventName - (char*)pDescSrc;
				pDesc->unDesc.stSED.pszShortEventName = (unsigned char*)((char*)pDesc + cnt);
				cnt = (char *)pDescSrc->unDesc.stSED.pszShortEventText - (char*)pDescSrc;
				pDesc->unDesc.stSED.pszShortEventText = (unsigned char*)((char*)pDesc + cnt);
				*ppDescDest = pDesc;
			}
			else
				err = TSPARSE_SIMGMT_MEMORYALLOCFAIL;
			break;
		case EXTN_EVT_DESCR_ID:
			{
				EXT_EVT_STRUCT *pExtEvtSrc;
				EXT_EVT_STRUCT *pExtEvt, *pExtEvtPrev;

				pDesc = (DESCRIPTOR_STRUCT*)tcc_mw_malloc(__FUNCTION__, __LINE__, size);
				//memset(pDesc, 0, size);
				if (CHK_VALIDPTR(pDesc))
				{
					memcpy (pDesc, pDescSrc, size);
					pDesc->unDesc.stEED.pstExtEvtList = (EXT_EVT_STRUCT*)NULL;

					pExtEvtPrev = (EXT_EVT_STRUCT *)NULL;
					pExtEvtSrc = pDescSrc->unDesc.stEED.pstExtEvtList;

					if (CHK_VALIDPTR(pExtEvtSrc) && pDescSrc->unDesc.stEED.ucExtEvtCount > 0)
					{
						for (cnt=0; cnt < LOOP_LIMIT && CHK_VALIDPTR(pExtEvtSrc); cnt++)
						{
							pExtEvt = (EXT_EVT_STRUCT*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(EXT_EVT_STRUCT));
							//memset(pExtEvt, 0, sizeof(EXT_EVT_STRUCT));
							if (CHK_VALIDPTR(pExtEvt))
							{
								memcpy (pExtEvt, pExtEvtSrc, sizeof(EXT_EVT_STRUCT));
								pExtEvt->pstExtEvtStruct = (EXT_EVT_STRUCT*)NULL;

								if (CHK_VALIDPTR(pExtEvtPrev))
									pExtEvtPrev->pstExtEvtStruct = pExtEvt;
								else
									pDesc->unDesc.stEED.pstExtEvtList = pExtEvt;

								pExtEvtPrev = pExtEvt;

								pExtEvtSrc = pExtEvtSrc->pstExtEvtStruct;
							}
							else
							{
								err = TSPARSE_SIMGMT_MEMORYALLOCFAIL;
							}
						}
					}
					*ppDescDest = pDesc;
				}
				else
					err = TSPARSE_SIMGMT_MEMORYALLOCFAIL;
			}
			break;
		case COMPONENT_DESCR_ID:
			pDesc = (DESCRIPTOR_STRUCT*)tcc_mw_malloc(__FUNCTION__, __LINE__, size);
			//memset(pDesc, 0, size);
			if (CHK_VALIDPTR(pDesc))
			{
				memcpy (pDesc, pDescSrc, size);
				cnt = (char *)pDescSrc->unDesc.stCOMD.pszComponentText - (char*)pDescSrc;
				pDesc->unDesc.stCOMD.pszComponentText = (char*)((char*)pDesc + cnt);
				*ppDescDest = pDesc;
			}
			else
				err = TSPARSE_SIMGMT_MEMORYALLOCFAIL;
			break;
		case CONTENT_DESCR_ID:
			pDesc = (DESCRIPTOR_STRUCT*)tcc_mw_malloc(__FUNCTION__, __LINE__, size);
			//memset(pDesc, 0, size);
			if (CHK_VALIDPTR(pDesc))
			{
				memcpy (pDesc, pDescSrc, size);
				cnt = (char *)pDescSrc->unDesc.stCOND.pastContentData - (char*)pDescSrc;
				pDesc->unDesc.stCOND.pastContentData = (CONTENT_DATA*)((char*)pDesc + cnt);
				*ppDescDest = pDesc;
			}
			else
				err = TSPARSE_SIMGMT_MEMORYALLOCFAIL;
			break;
		case EMERGENCY_INFORMATION_DESCR_ID:
			{
				EMERGENCY_INFO_DATA **ppEid, *pEidSrc;
				EMERGENCY_AREA_CODE **ppArea, *pAreaSrc;
				int cnt_eid, cnt_area;
				pDesc = (DESCRIPTOR_STRUCT*)tcc_mw_malloc(__FUNCTION__, __LINE__, size);
				//memset(pDesc, 0, size);
				if (CHK_VALIDPTR(pDesc))
				{
					memcpy (pDesc, pDescSrc, size);
					pDesc->unDesc.stEID.pEID = (EMERGENCY_INFO_DATA*)NULL;

					if(CHK_VALIDPTR(pDescSrc->unDesc.stEID.pEID))
					{
						pEidSrc = pDescSrc->unDesc.stEID.pEID;
						ppEid = &pDesc->unDesc.stEID.pEID;

						for(cnt_eid=0; cnt_eid < LOOP_LIMIT && CHK_VALIDPTR(pEidSrc); cnt_eid++)
						{
							*ppEid = (EMERGENCY_INFO_DATA*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(EMERGENCY_INFO_DATA));
							//memset(*ppEid, 0, sizeof(EMERGENCY_INFO_DATA));
							if (!CHK_VALIDPTR(*ppEid))
							{
								err = TSPARSE_SIMGMT_MEMORYALLOCFAIL;
								break;
							}
							memcpy (*ppEid, pEidSrc, sizeof(EMERGENCY_INFO_DATA));

							(*ppEid)->pAreaCode = (EMERGENCY_AREA_CODE*)NULL;
							(*ppEid)->pNext = NULL;

							if (CHK_VALIDPTR(pEidSrc->pAreaCode))
							{
								ppArea = &((*ppEid)->pAreaCode);
								pAreaSrc = pEidSrc->pAreaCode;

								for(cnt_area=0; (cnt_area < LOOP_LIMIT) && CHK_VALIDPTR(pAreaSrc); cnt_area++)
								{
									*ppArea = (EMERGENCY_AREA_CODE*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(EMERGENCY_AREA_CODE));
									if (!CHK_VALIDPTR(*ppArea))
									{
										err = TSPARSE_SIMGMT_MEMORYALLOCFAIL;
										break;
									}
									memset(*ppArea, 0, sizeof(EMERGENCY_AREA_CODE));

									(*ppArea)->area_code = pAreaSrc->area_code;
									(*ppArea)->pNext = NULL;
									ppArea = &(*ppArea)->pNext;
									pAreaSrc = pAreaSrc->pNext;
								}
							}

							pEidSrc = pEidSrc->pNext;
							ppEid = &(*ppEid)->pNext;
						}
					}

					*ppDescDest = pDesc;
				}
				else
				{
					err = TSPARSE_SIMGMT_MEMORYALLOCFAIL;
					break;
				}
			}
			break;
		default:
			pDesc = (DESCRIPTOR_STRUCT*)tcc_mw_malloc(__FUNCTION__, __LINE__, size);
			if (CHK_VALIDPTR(pDesc))
			{
				memcpy (pDesc, pDescSrc, size);
				*ppDescDest = pDesc;
			}
			else
				err = TSPARSE_SIMGMT_MEMORYALLOCFAIL;
			break;
	}
	return TSPARSE_SIMGMT_OK;
}

void TSPARSE_ISDBT_SiMgmt_DelDescriptor (DESCRIPTOR_STRUCT *pDesc)
{
	if (CHK_VALIDPTR(pDesc)) {
		switch (pDesc->enDescriptorID) {
			case EXTN_EVT_DESCR_ID:
				{
					int count;
					EXT_EVT_STRUCT *pExtEvt, *pExtEvtP;

					pExtEvt = pDesc->unDesc.stEED.pstExtEvtList;

					for (count=0; count < LOOP_LIMIT && CHK_VALIDPTR(pExtEvt); count++)
					{
						pExtEvtP = pExtEvt->pstExtEvtStruct;
						tcc_mw_free(__FUNCTION__, __LINE__, pExtEvt);
						pExtEvt = pExtEvtP;
					}

					tcc_mw_free(__FUNCTION__, __LINE__, pDesc);
				}
				break;
			case EMERGENCY_INFORMATION_DESCR_ID:
				{
					EMERGENCY_INFO_DATA *pEid,*pEidN;
					EMERGENCY_AREA_CODE *pArea, *pAreaN;
					int cnt_eid, cnt_area;

					pEid = pDesc->unDesc.stEID.pEID;

					for(cnt_eid=0; cnt_eid < LOOP_LIMIT && CHK_VALIDPTR(pEid); cnt_eid++)
					{
						pEidN = pEid->pNext;

						pArea = pEid->pAreaCode;
						for (cnt_area=0; cnt_area < LOOP_LIMIT && CHK_VALIDPTR(pArea); cnt_area++)
						{
							pAreaN = pArea->pNext;
							tcc_mw_free(__FUNCTION__, __LINE__, pArea);
							pArea = pAreaN;
						}

						tcc_mw_free(__FUNCTION__, __LINE__, pEid);
						pEid = pEidN;
					}

					tcc_mw_free(__FUNCTION__, __LINE__, pDesc);
				}
				break;
			default:
				tcc_mw_free(__FUNCTION__, __LINE__, pDesc);
				break;
		}
	}
}
static void TSPARSE_ISDBT_SiMgmt_InitAllInfo(void)
{
	TSPARSE_ISDBT_SiMgmt_DistroyTable(&(stSiMgmtInfo.PAT));
	TSPARSE_ISDBT_SiMgmt_DistroyTable(&(stSiMgmtInfo.CAT));
	TSPARSE_ISDBT_SiMgmt_DistroyTable(&(stSiMgmtInfo.PMT));
	TSPARSE_ISDBT_SiMgmt_DistroyTable(&(stSiMgmtInfo.BIT));
	TSPARSE_ISDBT_SiMgmt_DistroyTable(&(stSiMgmtInfo.NIT));
	TSPARSE_ISDBT_SiMgmt_DistroyTable(&(stSiMgmtInfo.SDT));
	TSPARSE_ISDBT_SiMgmt_DistroyTable(&(stSiMgmtInfo.EIT));
	TSPARSE_ISDBT_SiMgmt_DistroyTable(&(stSiMgmtInfo.TOT));
	TSPARSE_ISDBT_SiMgmt_DistroyTable(&(stSiMgmtInfo.CDT));
}

/* ------------- top ---------------*/
/**
  @brief initialize a management for PSI/SI information
  @param void
  @return 0=success, else=failure
  */
int TSPARSE_ISDBT_SiMgmt_Init(void)
{
	/**<This routine doesn't do nothing in current implementation*/
	tsparse_isdbt_simgmt_lock_init();
	TSPARSE_ISDBT_SiMgmt_InitAllInfo();
	return 0;
}

/**
  @brief destroy a management for PSI/SI information
  @param void
  @return 0=success, else=failure
  */
int TSPARSE_ISDBT_SiMgmt_Deinit(void)
{
	/**<This routine doesn't do nothing in current implementation*/
	TSPARSE_ISDBT_SiMgmt_InitAllInfo();
	tsparse_isdbt_simgmt_lock_deinit();
	return 0;
}

/**
 @brief clear all PSI/SI information

 It's used to clear old information when start to process a new channel
 @param none
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_Clear (void)
{
	TSPARSE_ISDBT_SiMgmt_InitAllInfo();
	return 0;
}

/**
 @brief get a root pointer to each PSI/SI information.
 @param table_id id of table. it's used to indicate which table the caller want to get
 @param ppTableRoot a refernce of pointer to return a found root pointer to caller
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_GetTableRoot (SI_MGMT_TABLE_ROOT **ppTableRoot, MPEG_TABLE_IDS table_id)
{
	int err;
	LOCK_START();
	tsparse_isdbt_simgmt_state_set(TSPARSE_SIMGMT_STATE_RUN);
	err = tsparse_isdbt_simgmt_GetTableRoot (ppTableRoot, table_id);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_GetTableRoot(SI_MGMT_TABLE_ROOT **ppTableRoot, MPEG_TABLE_IDS table_id)
{
	SI_MGMT_TABLE_ROOT *pTableRootPtr = NULL;
	int err = TSPARSE_SIMGMT_OK, status = 0;

	if (!CHK_VALIDPTR(ppTableRoot))
		return TSPARSE_SIMGMT_INVALIDPARAM;

	status = TSPARSE_ISDBT_SiMgmt_GetTableRootPtrByTableID(table_id, &pTableRootPtr);
	if (CHK_VALIDPTR(pTableRootPtr))
	{
		*ppTableRoot = pTableRootPtr;
	}
	else
	{
		*ppTableRoot = (SI_MGMT_TABLE_ROOT *)NULL;
		err = TSPARSE_SIMGMT_INVALIDDATA;
	}

	return err;
}

/**
 @brief get a pointer to each table from a root pointer
 @param ppTable a reference of pointer to return a pointer of each table to caller
 @param pTableRoot  a root pointer to each PSI/SI information
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_GetTable (SI_MGMT_TABLE **ppTable, SI_MGMT_TABLE_ROOT *pTableRoot)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_GetTable(ppTable, pTableRoot);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_GetTable (SI_MGMT_TABLE **ppTable, SI_MGMT_TABLE_ROOT *pTableRoot)
{
	if (!CHK_VALIDPTR(ppTable) || !CHK_VALIDPTR(pTableRoot)) return TSPARSE_SIMGMT_INVALIDPARAM;

	*ppTable = pTableRoot->pTable;
	return TSPARSE_SIMGMT_OK;
}

/**
 @brief find a subtable which is matched with table data from a root pointer
 @param ppTable a reference of pointer to return a found table to caller
 @param table_id id of table
 @param punTableExt table data which comprise subtable
 @prarm pRootTble a root pointer to each PSI/SI information
 @return 0=success, else-failure
 */
int TSPARSE_ISDBT_SiMgmt_FindTable(SI_MGMT_TABLE **ppTable, MPEG_TABLE_IDS table_id, SI_MGMT_TABLE_EXTENSION *punTableExt, SI_MGMT_TABLE_ROOT *pTableRoot)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_FindTable(ppTable, table_id, punTableExt, pTableRoot);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_FindTable(SI_MGMT_TABLE **ppTable, MPEG_TABLE_IDS table_id, SI_MGMT_TABLE_EXTENSION *punTableExt, SI_MGMT_TABLE_ROOT *pTableRoot)
{
	SI_MGMT_TABLE_ROOT *pTableRootPtr = NULL;
	SI_MGMT_TABLE *pTable = NULL;
	int cnt = 0, err = 0;

	if (!CHK_VALIDPTR(ppTable)) return TSPARSE_SIMGMT_INVALIDPARAM;	/* skip to check punTableExt intentionally */

	if (!CHK_VALIDPTR(pTableRoot)) {
		err = TSPARSE_ISDBT_SiMgmt_GetTableRootPtrByTableID(table_id, &pTableRootPtr);
		if (err != TSPARSE_SIMGMT_OK) pTableRoot = (SI_MGMT_TABLE_ROOT*)NULL;
	} else {
		pTableRootPtr = pTableRoot;
	}

	*ppTable = (SI_MGMT_TABLE *)NULL;
	err = TSPARSE_SIMGMT_INVALIDDATA;
	if (CHK_VALIDPTR(pTableRootPtr)) {
		pTable = pTableRootPtr->pTable;
		for (cnt=0; cnt < LOOP_LIMIT && CHK_VALIDPTR(pTable); cnt++) {
			if (pTable->table_id == table_id) {
				if (TSPARSE_ISDBT_SiMgmt_CmpTableExt(table_id, punTableExt, &(pTable->unTableExt)) == TSPARSE_SIMGMT_OK) {
					*ppTable = pTable;
					err = TSPARSE_SIMGMT_OK;
					break;
				}
			}
			pTable = pTable->pNext;
		}
	}
	return err;
}

/**
 @brief add a tabel to a root pointer of PSI/SI information

 if there is already a table which has a same table_id and table data, it'll return a pointer to this table without updating any information.
 @param ppTable a reference of pointer to return an added table to caller. If it's invalid pointer, doens't return an added table to caller.
 @param table_id id of table
 @param punTableExt a pointer to table data
 @param pTableRoot a root pointer to each PSI/SI information
 */
int TSPARSE_ISDBT_SiMgmt_AddTable (SI_MGMT_TABLE **ppTable, MPEG_TABLE_IDS table_id, SI_MGMT_TABLE_EXTENSION *punTableExt, SI_MGMT_TABLE_ROOT *pTableRoot)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_AddTable(ppTable, table_id, punTableExt, pTableRoot);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_AddTable (SI_MGMT_TABLE **ppTable, MPEG_TABLE_IDS table_id, SI_MGMT_TABLE_EXTENSION *punTableExt, SI_MGMT_TABLE_ROOT *pTableRoot)
{
	SI_MGMT_TABLE_ROOT *pTableRootPtr = NULL;
	SI_MGMT_TABLE *pTable = NULL, *pTableTemp = NULL;
	int status = 0, err = 0;
	/* skip to check a validity of punTableExt intentionally */

	if (!CHK_VALIDPTR(pTableRoot))
	{
		err = TSPARSE_ISDBT_SiMgmt_GetTableRootPtrByTableID(table_id, &pTableRootPtr);
		if (err != TSPARSE_SIMGMT_OK)
			pTableRoot = (SI_MGMT_TABLE_ROOT*)NULL;
	}
	else
	{
		pTableRootPtr = pTableRoot;
	}

	if (CHK_VALIDPTR(ppTable))
		*ppTable = (SI_MGMT_TABLE *)NULL;

	err = TSPARSE_SIMGMT_INVALIDDATA;

	if (CHK_VALIDPTR(pTableRootPtr))
	{
		pTable = (SI_MGMT_TABLE *)NULL;
		status = tsparse_isdbt_simgmt_FindTable (&pTable, table_id, punTableExt, pTableRootPtr);
		if (status==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pTable))
		{
			if (CHK_VALIDPTR(ppTable))
				*ppTable = pTable;

			ERR_MSG("[AddTable] there is already table(%d)", table_id);
			err = TSPARSE_SIMGMT_ADDDUPLICATE;
		}
		else
		{
			pTable = tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(SI_MGMT_TABLE));
			if (CHK_VALIDPTR(pTable))
			{
				memset(pTable, 0, sizeof(SI_MGMT_TABLE));

				pTable->table_id = table_id;
				if (CHK_VALIDPTR(punTableExt))
					memcpy (&(pTable->unTableExt), punTableExt, sizeof(SI_MGMT_TABLE_EXTENSION));
				pTable->pSecPrev = (SI_MGMT_SECTION *)NULL;
				pTable->pSecCurr = (SI_MGMT_SECTION *)NULL;
				pTable->pParentTableRoot = pTableRootPtr;
				pTable->pNext = pTableRootPtr->pTable;
				pTable->pPrev = (SI_MGMT_TABLE *)NULL;
				if (CHK_VALIDPTR(pTableRootPtr->pTable))
					pTableRootPtr->pTable->pPrev = pTable;
				pTableRootPtr->pTable = pTable;
				if (CHK_VALIDPTR(ppTable))
					*ppTable = pTable;	/* return a pointer to added table only if ppTable is valid. This argument can be null by intention */

				err = TSPARSE_SIMGMT_OK;
			}
			else
			{
				err = TSPARSE_SIMGMT_MEMORYALLOCFAIL;
			}
		}
	}

	return err;
}

/**
 @brief delete a table from a root pointer of PSI/SI information
 @param pTable a pointer to table which will be deleted from
 @param pTableRoot a root pointer to each PSI/SI information
 */
int TSPARSE_ISDBT_SiMgmt_DeleteTable(SI_MGMT_TABLE *pTable, SI_MGMT_TABLE_ROOT *pTableRoot)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_DeleteTable(pTable, pTableRoot);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_DeleteTable(SI_MGMT_TABLE *pTable, SI_MGMT_TABLE_ROOT *pTableRoot)
{
	SI_MGMT_TABLE_ROOT *pTableRootPtr;
	if (!CHK_VALIDPTR(pTable)) return TSPARSE_SIMGMT_INVALIDPARAM;

	if (!CHK_VALIDPTR(pTableRoot)) pTableRootPtr = pTable->pParentTableRoot;
	else pTableRootPtr = pTableRoot;

	if(!CHK_VALIDPTR(pTable->pPrev)) {
		if (CHK_VALIDPTR(pTable->pNext)) pTable->pNext->pPrev = (SI_MGMT_TABLE *)NULL;
		pTableRootPtr->pTable = pTable->pNext;
	} else {
		pTable->pPrev->pNext = pTable->pNext;
		if(CHK_VALIDPTR(pTable->pNext)) pTable->pNext->pPrev = pTable->pPrev;
	}
	if (CHK_VALIDPTR(pTable->pSecCurr)) tsparse_isdbt_simgmt_Table_DestroySection(eSECTION_CURR, pTable);
	if (CHK_VALIDPTR(pTable->pSecPrev)) tsparse_isdbt_simgmt_Table_DestroySection(eSECTION_PREV, pTable);

	tcc_mw_free(__FUNCTION__, __LINE__, pTable);

	return TSPARSE_SIMGMT_OK;
}

/**
 @brief delete a table by using table data from a root pointer of PSI/SI information
 @param table_id id of table
 @punTableExt a pointer to table data which comprise subtable
 @pTableRoot a root pointer to each PSI/SI information
 */
int TSPARSE_ISDBT_SiMgmt_DeleteTableExt (MPEG_TABLE_IDS table_id, SI_MGMT_TABLE_EXTENSION *punTableExt, SI_MGMT_TABLE_ROOT *pTableRoot)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_DeleteTableExt(table_id, punTableExt, pTableRoot);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_DeleteTableExt (MPEG_TABLE_IDS table_id, SI_MGMT_TABLE_EXTENSION *punTableExt, SI_MGMT_TABLE_ROOT *pTableRoot)
{
	int err, status;
	SI_MGMT_TABLE_ROOT *pTableRootPtr;
	SI_MGMT_TABLE *pTable = (SI_MGMT_TABLE*)NULL;

	if (!CHK_VALIDPTR(pTableRoot)) {
		err = TSPARSE_ISDBT_SiMgmt_GetTableRootPtrByTableID(table_id, &pTableRootPtr);
		if (err != TSPARSE_SIMGMT_OK) pTableRootPtr = (SI_MGMT_TABLE_ROOT*)NULL;
	} else
		pTableRootPtr = pTableRoot;

	err = TSPARSE_SIMGMT_OK;
	if (CHK_VALIDPTR (pTableRootPtr)) {
		pTable = (SI_MGMT_TABLE*)NULL;
		status = tsparse_isdbt_simgmt_FindTable (&pTable, table_id, punTableExt, pTableRootPtr);
		if (status == TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pTable)) {
			status = tsparse_isdbt_simgmt_DeleteTable (pTable, pTableRoot);
			if (status != TSPARSE_SIMGMT_OK) err = TSPARSE_SIMGMT_INVALIDDATA;
		} else
			err = TSPARSE_SIMGMT_INVALIDDATA;
	}
	return err;
}

/**
 @brief destroy all information of table from a root pointer
 @param pTableRoot a root pointer to each PSI/SI information
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_DistroyTable(SI_MGMT_TABLE_ROOT *pTableRoot)
{
	int err;
	LOCK_START();
	tsparse_isdbt_simgmt_state_set(TSPARSE_SIMGMT_STATE_INIT);
	err = tsparse_isdbt_simgmt_DistroyTable(pTableRoot);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_DistroyTable(SI_MGMT_TABLE_ROOT *pTableRoot)
{
	SI_MGMT_TABLE *pTable, *pTableNext;
	int cnt;
	if (!CHK_VALIDPTR(pTableRoot)) return TSPARSE_SIMGMT_INVALIDPARAM;

	pTable = pTableRoot->pTable;
	for (cnt=0; cnt < LOOP_LIMIT && CHK_VALIDPTR(pTable); cnt++) {
		pTableNext = pTable->pNext;
		tsparse_isdbt_simgmt_DeleteTable(pTable, pTableRoot);
		pTable = pTableNext;
	}
	pTableRoot->pTable = (SI_MGMT_TABLE*)NULL;
	return TSPARSE_SIMGMT_OK;
}


/*--------- table ------------*/
/**
 @brief transit a current table pointer to a previous one

 destroy all information of previous section at first, and copy a current section pointer to a previous one
 @param pTable pointer to table from which section transition will be done
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_Table_SectionTransit(SI_MGMT_TABLE *pTable)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Table_SectionTransit(pTable);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Table_SectionTransit(SI_MGMT_TABLE *pTable)
{
	if (!CHK_VALIDPTR(pTable)) return TSPARSE_SIMGMT_INVALIDPARAM;

	if (CHK_VALIDPTR(pTable->pSecPrev)) tsparse_isdbt_simgmt_Table_DestroySection(eSECTION_PREV, pTable);
	if (CHK_VALIDPTR(pTable->pSecCurr)) pTable->pSecPrev = pTable->pSecCurr;
	pTable->pSecCurr = (SI_MGMT_SECTION *)NULL;
	return TSPARSE_SIMGMT_OK;
}

/**
 @brief search a section withc is matched with a section_number from specified table
 @param ppSection reference of pointer to return a found section to caller
 @param fPrevCurr specify from which list  a section will be searched. 0 means a list of section of old version number, 1 means a list of section of current version number
 @param section_number specify a number of section
 @param pTable specify a table from which section will be got
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_Table_FindSection (SI_MGMT_SECTION **ppSection, int fPrevCurr, unsigned char section_number, SI_MGMT_TABLE *pTable)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Table_FindSection(ppSection, fPrevCurr, section_number, pTable);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Table_FindSection (SI_MGMT_SECTION **ppSection, int fPrevCurr, unsigned char section_number, SI_MGMT_TABLE *pTable)
{
	int count, err = TSPARSE_SIMGMT_OK;
	SI_MGMT_SECTION *pSec;
	if (!CHK_VALIDPTR(ppSection) || !CHK_VALIDPTR(pTable)) return TSPARSE_SIMGMT_INVALIDPARAM;

	*ppSection = (SI_MGMT_SECTION*)NULL;
	if(fPrevCurr == eSECTION_PREV) pSec = pTable->pSecPrev;
	else if (fPrevCurr == eSECTION_CURR) pSec = pTable->pSecCurr;
	else return TSPARSE_SIMGMT_INVALIDDATA;

	for (count=0; count < LOOP_LIMIT && CHK_VALIDPTR(pSec); count++) {
		if (pSec->section_number == section_number) break;
		pSec = pSec->pNext;
	}
	if (count==LOOP_LIMIT || !CHK_VALIDPTR(pSec)) err = TSPARSE_SIMGMT_INVALIDDATA;
	else *ppSection = pSec;
	return err;
}

/**
 @brief add a section to the table.

 if there is already a section which has a same number, it'll return a pointer to this section without updating any information.
 @param ppSection a reference of poiner to return an added section to caller
 @param fPrevCurr specify to which list of section a new section will be added. 0 means a list of section of old version number, 1 means a list of section of current version number
 @param version_number version number of section
 @param section_number section number
 @param last_section_number last sectin number
 @param punSectionExt pointer to section data
 @param pTable pointer to table to which a section will be added
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_Table_AddSection (SI_MGMT_SECTION **ppSection, int fPrevCurr, unsigned char version_number,
															unsigned char section_number,	unsigned char last_section_number,
															SI_MGMT_SECTION_EXTENSION *punSectionExt, SI_MGMT_TABLE *pTable)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Table_AddSection(ppSection, fPrevCurr, version_number, section_number, last_section_number, punSectionExt, pTable);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Table_AddSection (SI_MGMT_SECTION **ppSection, int fPrevCurr, unsigned char version_number,
															unsigned char section_number,	unsigned char last_section_number,
															SI_MGMT_SECTION_EXTENSION *punSectionExt, SI_MGMT_TABLE *pTable)
{
	int err, status;
	SI_MGMT_SECTION **ppSecRoot, *pSec;

	if (!CHK_VALIDPTR(pTable))
		return TSPARSE_SIMGMT_INVALIDPARAM;

	if(fPrevCurr == eSECTION_PREV)
		ppSecRoot = &(pTable->pSecPrev);
	else if (fPrevCurr == eSECTION_CURR)
		ppSecRoot = &(pTable->pSecCurr);
	else
		return TSPARSE_SIMGMT_INVALIDDATA;

	pSec = (SI_MGMT_SECTION*)NULL;
	status = tsparse_isdbt_simgmt_Table_FindSection(&pSec, fPrevCurr, section_number, pTable);
	if (status == TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pSec))   /* if there is a same section */
	{
		if (CHK_VALIDPTR(ppSection)) *ppSection = pSec;
		ERR_MSG("[Table_AddSection] there is already a same sectin (fPrevCurr=%d, ver=%d, sec_no=%d)", fPrevCurr, version_number, section_number);
		err = TSPARSE_SIMGMT_ADDDUPLICATE;
	}
	else
	{
		if (CHK_VALIDPTR(ppSection))
			*ppSection = (SI_MGMT_SECTION*)NULL;

		pSec = tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(SI_MGMT_SECTION));
		if (CHK_VALIDPTR(pSec))
		{
			memset(pSec, 0, sizeof(SI_MGMT_SECTION));

			pSec->version_number = version_number;
			pSec->section_number = section_number;
			pSec->last_section_number = last_section_number;
			if (CHK_VALIDPTR(punSectionExt))
				memcpy (&(pSec->unSectionExt), punSectionExt, sizeof(SI_MGMT_SECTION_EXTENSION));
			else
				memset(&(pSec->unSectionExt), 0, sizeof(SI_MGMT_SECTION_EXTENSION));
			pSec->pGlobalDesc = (DESCRIPTOR_STRUCT*)NULL;
			pSec->pParentTable = pTable;
			pSec->pNext = *ppSecRoot;
			pSec->pPrev = (SI_MGMT_SECTION*)NULL;

			if (CHK_VALIDPTR(*ppSecRoot))
				(*ppSecRoot)->pPrev = pSec;
			*ppSecRoot = pSec;

			if (CHK_VALIDPTR(ppSection))
				*ppSection = pSec;

			err = TSPARSE_SIMGMT_OK;
		}
		else
			err = TSPARSE_SIMGMT_MEMORYALLOCFAIL;
	}

	return err;
}

/**
 @brief delete a section from table
 @param fPrevCurr specify from which list  a section will be deleted. 0 means a list of section of old version number, 1 means a list of section of current version number
 @param pSection pointer to section whch will be deleted
 @param pTable pointer to table from which a section will be deleted
 @return 0=success, esle=failure
 */
int TSPARSE_ISDBT_SiMgmt_Table_DeleteSection (int fPrevCurr, SI_MGMT_SECTION *pSection, SI_MGMT_TABLE *pTable)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Table_DeleteSection(fPrevCurr, pSection, pTable);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Table_DeleteSection (int fPrevCurr, SI_MGMT_SECTION *pSection, SI_MGMT_TABLE *pTable)
{
	SI_MGMT_SECTION **ppSecRoot;
	if (!CHK_VALIDPTR(pSection) || !CHK_VALIDPTR(pTable)) return TSPARSE_SIMGMT_INVALIDPARAM;

	if(fPrevCurr == eSECTION_PREV) ppSecRoot = &(pTable->pSecPrev);
	else if (fPrevCurr == eSECTION_CURR) ppSecRoot = &(pTable->pSecCurr);
	else return TSPARSE_SIMGMT_INVALIDDATA;
	if (!CHK_VALIDPTR(ppSecRoot)) return TSPARSE_SIMGMT_INVALIDDATA;

	if (!CHK_VALIDPTR(pSection->pPrev)) {
		if (CHK_VALIDPTR(pSection->pNext)) pSection->pNext->pPrev = (SI_MGMT_SECTION*)NULL;
		*ppSecRoot = pSection->pNext;
	} else {
		pSection->pPrev->pNext = pSection->pNext;
		if (CHK_VALIDPTR(pSection->pNext)) pSection->pNext->pPrev = pSection->pPrev;
	}
	if (CHK_VALIDPTR(pSection->pGlobalDesc)) tsparse_isdbt_simgmt_Section_DestroyDescriptor_Global(pSection);
	if (CHK_VALIDPTR(pSection->pSectionLoop)) tsparse_isdbt_simgmt_Section_DestroyLoop(pSection);

	tcc_mw_free(__FUNCTION__, __LINE__, pSection);

	return TSPARSE_SIMGMT_OK;
}

/**
 @brief delete section from a table
 @param fPrevCurr specify from which list  a section will be deleted. 0 means a list of section of old version number, 1 means a list of section of current version number
 @param section_number specify a section_number of which a section will be deleted
 @param pTable pointer to table from which a section will be deleted
 @return 0=success, esle=failure
 */
int TSPARSE_ISDBT_SiMgmt_Table_DeleteSectionExt(int fPrevCurr, unsigned char section_number, SI_MGMT_TABLE *pTable)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Table_DeleteSectionExt(fPrevCurr, section_number, pTable);
 	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Table_DeleteSectionExt(int fPrevCurr, unsigned char section_number, SI_MGMT_TABLE *pTable)
{
	SI_MGMT_SECTION **ppSecRoot, *pSection;
	int status, err = TSPARSE_SIMGMT_OK;
	if (!CHK_VALIDPTR(pTable)) return TSPARSE_SIMGMT_INVALIDPARAM;

	if(fPrevCurr == eSECTION_PREV) ppSecRoot = &(pTable->pSecPrev);
	else if (fPrevCurr == eSECTION_CURR) ppSecRoot = &(pTable->pSecCurr);
	else return TSPARSE_SIMGMT_INVALIDDATA;

	/* try to find a section from a list */
	pSection = (SI_MGMT_SECTION *)NULL;
	status = tsparse_isdbt_simgmt_Table_FindSection(&pSection, fPrevCurr, section_number, pTable);
	if (status == TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pSection)) {
		status = tsparse_isdbt_simgmt_Table_DeleteSection (fPrevCurr, pSection, pTable);
		if (status != TSPARSE_SIMGMT_OK) err = TSPARSE_SIMGMT_INVALIDDATA;
	} else
		err = TSPARSE_SIMGMT_INVALIDDATA;
	return err;
}

/**
 @brief delete all section from table
 @param fPrevCurr specify from which list  a section will be deleted. 0 means a list of section of old version number, 1 means a list of section of current version number
 @param pTable pointer to table from which a section will be deleted
 @return 0=success, esle=failure
 */
int TSPARSE_ISDBT_SiMgmt_Table_DestroySection (int fPrevCurr, SI_MGMT_TABLE *pTable)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Table_DestroySection(fPrevCurr, pTable);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Table_DestroySection(int fPrevCurr, SI_MGMT_TABLE *pTable)
{
	SI_MGMT_SECTION *pSection, *pSectionNext;
	int cnt,status;
	if (!CHK_VALIDPTR(pTable)) return TSPARSE_SIMGMT_INVALIDPARAM;

	if (fPrevCurr == eSECTION_PREV) pSection = pTable->pSecPrev;
	else if (fPrevCurr == eSECTION_CURR) pSection = pTable->pSecCurr;
	else return TSPARSE_SIMGMT_INVALIDDATA;

	for (cnt=0; cnt < LOOP_LIMIT && CHK_VALIDPTR(pSection); cnt++) {
		pSectionNext = pSection->pNext;
		status = tsparse_isdbt_simgmt_Table_DeleteSection(fPrevCurr, pSection, pTable);
		pSection = pSectionNext;
	}
	if (fPrevCurr == eSECTION_PREV) pTable->pSecPrev = (SI_MGMT_SECTION *)NULL;
	else if (fPrevCurr == eSECTION_CURR) pTable->pSecCurr = (SI_MGMT_SECTION *)NULL;
	return TSPARSE_SIMGMT_OK;
}

/**
 @brief get a pointer to section from table
 @param ppSection reference of pointer to return a pointer to section of table
 @param fPrevCurr specify from which list  a section will be searched. 0 means a list of section of old version number, 1 means a list of section of current version number
 @param pTable specify a table from which section will be got
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_Table_GetSection(SI_MGMT_SECTION **ppSection, int fPrevCurr, SI_MGMT_TABLE *pTable)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Table_GetSection(ppSection, fPrevCurr, pTable);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Table_GetSection(SI_MGMT_SECTION **ppSection, int fPrevCurr, SI_MGMT_TABLE *pTable)
{
	int err = TSPARSE_SIMGMT_OK;
	SI_MGMT_SECTION *pSec;
	if (!CHK_VALIDPTR(ppSection) || !CHK_VALIDPTR(pTable)) return TSPARSE_SIMGMT_INVALIDPARAM;

	*ppSection = (SI_MGMT_SECTION *)NULL;

	if(fPrevCurr == eSECTION_PREV) pSec = pTable->pSecPrev;
	else if (fPrevCurr == eSECTION_CURR) pSec = pTable->pSecCurr;
	else return TSPARSE_SIMGMT_INVALIDDATA;

	if (CHK_VALIDPTR(pSec)) *ppSection = pSec;
	else err = TSPARSE_SIMGMT_INVALIDDATA;
	return err;
}

/**
 @brief get a table_id of specified table
 @param p_table_id reference to return a table_id to caller
 @param pTable specify a table from witch a table_id will be got
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_Table_GetTableID(MPEG_TABLE_IDS *p_table_id, SI_MGMT_TABLE *pTable)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Table_GetTableID(p_table_id, pTable);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Table_GetTableID(MPEG_TABLE_IDS *p_table_id, SI_MGMT_TABLE *pTable)
{
	if (!CHK_VALIDPTR(p_table_id) || !CHK_VALIDPTR(pTable)) return TSPARSE_SIMGMT_INVALIDPARAM;

	*p_table_id = pTable->table_id;
	return TSPARSE_SIMGMT_OK;
}

/**
 @brief get a data of specified table
 @param pTableExt reference to return a pointer pointing table data to caller
 @param pTable specify a table from witch table_id_extension will be got
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_Table_GetTableExt(SI_MGMT_TABLE_EXTENSION **punTableExt, SI_MGMT_TABLE *pTable)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Table_GetTableExt (punTableExt, pTable);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Table_GetTableExt(SI_MGMT_TABLE_EXTENSION **punTableExt, SI_MGMT_TABLE *pTable)
{
	if (!CHK_VALIDPTR(punTableExt) || !CHK_VALIDPTR(pTable)) return TSPARSE_SIMGMT_INVALIDPARAM;

	*punTableExt = &(pTable->unTableExt);
	return TSPARSE_SIMGMT_OK;
}

/**
 @brief get a root pointer of table
 @param ppTableRoot reference to return a root pointer to caller
 @param pTable specify a table from which a root pointer will be got
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_Table_GetParentRoot (SI_MGMT_TABLE_ROOT **ppTableRoot, SI_MGMT_TABLE *pTable)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Table_GetParentRoot (ppTableRoot, pTable);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Table_GetParentRoot (SI_MGMT_TABLE_ROOT **ppTableRoot, SI_MGMT_TABLE *pTable)
{
	if (!CHK_VALIDPTR(ppTableRoot) || !CHK_VALIDPTR(pTable)) return TSPARSE_SIMGMT_INVALIDPARAM;

	*ppTableRoot = pTable->pParentTableRoot;
	return TSPARSE_SIMGMT_OK;
}

/**
 @brief get a pointer to next table
 @param ppTableNext reference to return a pointer pointing next table to caller
 @param pTable specify a table from which a next table will be got
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_Table_GetNextTable(SI_MGMT_TABLE **ppTableNext, SI_MGMT_TABLE *pTable)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Table_GetNextTable (ppTableNext, pTable);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Table_GetNextTable(SI_MGMT_TABLE **ppTableNext, SI_MGMT_TABLE *pTable)
{
	if (!CHK_VALIDPTR(ppTableNext) || !CHK_VALIDPTR(pTable)) return TSPARSE_SIMGMT_INVALIDPARAM;

	*ppTableNext = pTable->pNext;
	if (CHK_VALIDPTR(pTable->pNext)) return TSPARSE_SIMGMT_OK;
	else return TSPARSE_SIMGMT_INVALIDDATA;
}

/**
 @brief set a data of specified table
 @param punTableExt pointer to data of table
 @param pTable pointer to table to which data will be set
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_Table_SetTableExt (SI_MGMT_TABLE_EXTENSION *punTableExt, SI_MGMT_TABLE *pTable)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Table_SetTableExt(punTableExt, pTable);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Table_SetTableExt (SI_MGMT_TABLE_EXTENSION *punTableExt, SI_MGMT_TABLE *pTable)
{
	if (!CHK_VALIDPTR(punTableExt) || !CHK_VALIDPTR(pTable)) return TSPARSE_SIMGMT_INVALIDPARAM;

	memcpy (&(pTable->unTableExt), punTableExt, sizeof(SI_MGMT_TABLE_EXTENSION));
	return 	TSPARSE_SIMGMT_OK;
}


/*---------- section ----------*/

/**
 @brief find a section loop from a section
 @param ppSectionLoop reference of pointer to return a found a section loop witch is matched with pSectionLoopData
 @param pSectionLoopData pointer to a data of section loop. It's used as parameter for finding a section loop.
 @param pSection pointer to a section of table from which a sectin loop will be got
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_Section_FindLoop (SI_MGMT_SECTIONLOOP **ppSectionLoop, SI_MGMT_SECTIONLOOP_DATA *pSectionLoopData, SI_MGMT_SECTION *pSection)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Section_FindLoop(ppSectionLoop, pSectionLoopData, pSection);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Section_FindLoop (SI_MGMT_SECTIONLOOP **ppSectionLoop, SI_MGMT_SECTIONLOOP_DATA *pSectionLoopData, SI_MGMT_SECTION *pSection)

{
	MPEG_TABLE_IDS table_id;
	SI_MGMT_SECTIONLOOP *pSecLoop;
	SI_MGMT_SECTIONLOOP_DATA *pSecLoopD;
	int count, err = TSPARSE_SIMGMT_OK;

	if(!CHK_VALIDPTR(pSectionLoopData) || !CHK_VALIDPTR(pSection)) return TSPARSE_SIMGMT_INVALIDPARAM;

	if (!CHK_VALIDPTR(pSection->pParentTable)) {
		*ppSectionLoop = (SI_MGMT_SECTIONLOOP *)NULL;
		return TSPARSE_SIMGMT_INVALIDDATA;
	}
	table_id = pSection->pParentTable->table_id;
	pSecLoop = pSection->pSectionLoop;

	*ppSectionLoop = (SI_MGMT_SECTIONLOOP*)NULL;
	for (count=0; count < LOOP_LIMIT && CHK_VALIDPTR(pSecLoop); count++) {
		pSecLoopD = &(pSecLoop->unSectionLoopData);
		if(TSPARSE_ISDBT_SiMgmt_Table_IsPAT(table_id)) {
			if ((pSectionLoopData->pat_loop.program_map_pid == pSecLoopD->pat_loop.program_map_pid)
				&& (pSectionLoopData->pat_loop.program_number == pSecLoopD->pat_loop.program_number))
				break;
		} else if (TSPARSE_ISDBT_SiMgmt_Table_IsPMT(table_id)) {
			if (pSectionLoopData->pmt_loop.elementary_PID == pSecLoopD->pmt_loop.elementary_PID)
				break;
		} else if (TSPARSE_ISDBT_SiMgmt_Table_IsBIT(table_id)) {
			if (pSectionLoopData->bit_loop.broadcaster_id == pSecLoopD->bit_loop.broadcaster_id)
				break;
		} else if (TSPARSE_ISDBT_SiMgmt_Table_IsNIT(table_id)) {
			if (pSectionLoopData->nit_loop.transport_stream_id == pSecLoopD->nit_loop.transport_stream_id)
				break;
		} else if (TSPARSE_ISDBT_SiMgmt_Table_IsSDT(table_id)) {
			if (pSectionLoopData->sdt_loop.service_id == pSecLoopD->sdt_loop.service_id)
				break;
		} else if (TSPARSE_ISDBT_SiMgmt_Table_IsEIT(table_id)) {
			if (pSectionLoopData->eit_loop.event_id == pSecLoopD->eit_loop.event_id)
				break;
		}
		/**< TOT, CDT, CAT : doesn't make section loop in current implementation */
		else if (TSPARSE_ISDBT_SiMgmt_Table_IsTOT(table_id)) {
			count = LOOP_LIMIT;
			break;
		} else if (TSPARSE_ISDBT_SiMgmt_Table_IsCDT(table_id)) {
			count = LOOP_LIMIT;
			break;
		} else if (TSPARSE_ISDBT_SiMgmt_Table_IsCAT(table_id)) {
			count = LOOP_LIMIT;
			break;
		} else {
			ERR_MSG("[SiMgmt]table_id=0x%x not applicable", table_id);
			count = LOOP_LIMIT;
			break;
		}
		pSecLoop = pSecLoop->pNext;
	}
	if (count == LOOP_LIMIT || !CHK_VALIDPTR(pSecLoop)) err = TSPARSE_SIMGMT_INVALIDDATA;
	else *ppSectionLoop = pSecLoop;
	return err;
}

/**
 @brief add a loop to a section

 if there is already a loop which has smae data, it'll return a pointer to this loop without updating any information
 @param ppSectionLoop reference of pointer to return an added loop to caller
 @param punSectionLoopData pointer to section data
 @param pSection pointer to section to which a loop will be added
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_Section_AddLoop(SI_MGMT_SECTIONLOOP **ppSectionLoop, SI_MGMT_SECTIONLOOP_DATA *punSectionLoopData, SI_MGMT_SECTION *pSection)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Section_AddLoop(ppSectionLoop, punSectionLoopData, pSection);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Section_AddLoop(SI_MGMT_SECTIONLOOP **ppSectionLoop, SI_MGMT_SECTIONLOOP_DATA *punSectionLoopData, SI_MGMT_SECTION *pSection)
{
	int err, status;
	SI_MGMT_SECTIONLOOP *pSecLoop;

	if (!CHK_VALIDPTR(pSection))
		return TSPARSE_SIMGMT_INVALIDPARAM;

	pSecLoop = (SI_MGMT_SECTIONLOOP*)NULL;
	status = tsparse_isdbt_simgmt_Section_FindLoop(&pSecLoop, punSectionLoopData, pSection);
	if (status==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pSecLoop))
	{
		if (CHK_VALIDPTR(ppSectionLoop))
			*ppSectionLoop = pSecLoop;

		ERR_MSG("[Section_AddLoop]there is already a same loop (table_id=%d)",  CHK_VALIDPTR(pSection->pParentTable) ? (int)(pSection->pParentTable->table_id) : -1);
		err = TSPARSE_SIMGMT_ADDDUPLICATE;
	}
	else
	{
		pSecLoop = tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(SI_MGMT_SECTIONLOOP));
		if (CHK_VALIDPTR(pSecLoop))
		{
			memset(pSecLoop, 0, sizeof(SI_MGMT_SECTIONLOOP));

			pSecLoop->pDesc = (DESCRIPTOR_STRUCT*)NULL;
			pSecLoop->pParentSection = pSection;
			if (CHK_VALIDPTR(punSectionLoopData))
				memcpy (&(pSecLoop->unSectionLoopData), punSectionLoopData, sizeof(SI_MGMT_SECTIONLOOP_DATA));
			else
				memset(&(pSecLoop->unSectionLoopData), 0, sizeof(SI_MGMT_SECTIONLOOP_DATA));
			pSecLoop->pPrev = (SI_MGMT_SECTIONLOOP*)NULL;
			pSecLoop->pNext = pSection->pSectionLoop;

			if (CHK_VALIDPTR(pSection->pSectionLoop))
				pSection->pSectionLoop->pPrev = pSecLoop;
			pSection->pSectionLoop = pSecLoop;
			if (CHK_VALIDPTR(ppSectionLoop))
				*ppSectionLoop = pSecLoop;

			err = TSPARSE_SIMGMT_OK;
		}
		else
		{
			if (CHK_VALIDPTR(ppSectionLoop))
				*ppSectionLoop = (SI_MGMT_SECTIONLOOP*)NULL;

			err = TSPARSE_SIMGMT_MEMORYALLOCFAIL;
		}
	}

	return err;
}

/**
 @brief delete section loop from a section
 @param pSectionLoop pointer to loop which will be deleted
 @param pSection pointer to  section of table from which a loop will be deleted
 @return 0==success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_Section_DeleteLoop(SI_MGMT_SECTIONLOOP* pSectionLoop, SI_MGMT_SECTION *pSection)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Section_DeleteLoop(pSectionLoop, pSection);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Section_DeleteLoop(SI_MGMT_SECTIONLOOP* pSectionLoop, SI_MGMT_SECTION *pSection)
{
	if (!CHK_VALIDPTR(pSectionLoop) || !CHK_VALIDPTR(pSection)) return TSPARSE_SIMGMT_INVALIDPARAM;

	if (!CHK_VALIDPTR(pSectionLoop->pPrev)) {
		if (CHK_VALIDPTR(pSectionLoop->pNext)) pSectionLoop->pNext->pPrev = (SI_MGMT_SECTIONLOOP*)NULL;
		pSection->pSectionLoop = pSectionLoop->pNext;
	} else {
		pSectionLoop->pPrev->pNext = pSectionLoop->pNext;
		if (CHK_VALIDPTR(pSectionLoop->pNext)) pSectionLoop->pNext->pPrev = pSectionLoop->pPrev;
	}
	if (CHK_VALIDPTR(pSectionLoop->pDesc)) tsparse_isdbt_simgmt_SectionLoop_DestroyDescriptor(pSectionLoop);

	tcc_mw_free(__FUNCTION__, __LINE__, pSectionLoop);

	return TSPARSE_SIMGMT_OK;
}

/**
 @brief delete a section loop from a section
 @param punSectionLoopData pointer to section data
 @param pSection pointer to section from which a loop will be deleted
 @return 0==success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_Section_DeleteLoopExt(SI_MGMT_SECTIONLOOP_DATA* punSectionLoopData, SI_MGMT_SECTION *pSection)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Section_DeleteLoopExt (punSectionLoopData, pSection);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Section_DeleteLoopExt(SI_MGMT_SECTIONLOOP_DATA* punSectionLoopData, SI_MGMT_SECTION *pSection)
{
	SI_MGMT_SECTIONLOOP *pSectionLoop = (SI_MGMT_SECTIONLOOP*)NULL;
	int err=TSPARSE_SIMGMT_OK, status;
	if (!CHK_VALIDPTR(punSectionLoopData) || !CHK_VALIDPTR(pSection)) return TSPARSE_SIMGMT_INVALIDPARAM;
	status = tsparse_isdbt_simgmt_Section_FindLoop (&pSectionLoop, punSectionLoopData, pSection);
	if (status==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pSectionLoop)) {
		status = tsparse_isdbt_simgmt_Section_DeleteLoop(pSectionLoop, pSection);
		if (status != TSPARSE_SIMGMT_OK) err = TSPARSE_SIMGMT_INVALIDDATA;
	} else
		err = TSPARSE_SIMGMT_INVALIDDATA;
	return err;
}

/**
 @brief delete all loop from a section
 @param pSection pointer to section from which loop will be deleted
 @return 0=success, esle=failure
 */
int TSPARSE_ISDBT_SiMgmt_Section_DestroyLoop (SI_MGMT_SECTION *pSection)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Section_DestroyLoop (pSection);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Section_DestroyLoop (SI_MGMT_SECTION *pSection)
{
	SI_MGMT_SECTIONLOOP *pSecLoop, *pSecLoopNext;
	int cnt;
	if (!CHK_VALIDPTR(pSection)) return TSPARSE_SIMGMT_INVALIDPARAM;

	pSecLoop = pSection->pSectionLoop;
	for (cnt=0; cnt < LOOP_LIMIT && CHK_VALIDPTR(pSecLoop); cnt++) {
		pSecLoopNext = pSecLoop->pNext;
		tsparse_isdbt_simgmt_Section_DeleteLoop (pSecLoop, pSection);
		pSecLoop = pSecLoopNext;
	}
	pSection->pSectionLoop = (SI_MGMT_SECTIONLOOP*)NULL;
	return TSPARSE_SIMGMT_OK;
}

/**
 @brief find a descriptor from global descriptor of a section
 @param ppDescriptor reference of poiner to return a found descriptor
 @param descriptor_id id of descriptor
 @param pSection pointer to a section of table from which a descriptor will be got
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_Section_FindDescriptor_Global (DESCRIPTOR_STRUCT **ppDescriptor, DESCRIPTOR_IDS descriptor_id, void *arg, SI_MGMT_SECTION *pSection)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Section_FindDescriptor_Global (ppDescriptor, descriptor_id, arg, pSection);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Section_FindDescriptor_Global (DESCRIPTOR_STRUCT **ppDescriptor, DESCRIPTOR_IDS descriptor_id, void *arg, SI_MGMT_SECTION *pSection)
{
	DESCRIPTOR_STRUCT *pDesc;
	int count, err = TSPARSE_SIMGMT_INVALIDDATA;
	unsigned char descriptor_number=0, fTrue;
	unsigned short data_component_id=0;
	if (!CHK_VALIDPTR(ppDescriptor) || !CHK_VALIDPTR(pSection)) return TSPARSE_SIMGMT_INVALIDPARAM;

	*ppDescriptor = (DESCRIPTOR_STRUCT*)NULL;
	pDesc = pSection->pGlobalDesc;
	for (count=0; count < LOOP_LIMIT && CHK_VALIDPTR(pDesc); count++) {
		if (pDesc->enDescriptorID == descriptor_id) {
			switch (descriptor_id) {
				case EXTN_EVT_DESCR_ID:
					if (!CHK_VALIDPTR(arg)) fTrue=1;
					else {
						fTrue=0;
						descriptor_number = *((unsigned char*)arg);
					}
					if (pDesc->unDesc.stEED.ucDescrNumber == descriptor_number || fTrue) {
						*ppDescriptor = pDesc;
						err = TSPARSE_SIMGMT_OK;
					}
					break;
				case DATA_CONTENT_DESCR_ID:
					if (!CHK_VALIDPTR(arg)) fTrue = 1;
					else {
						fTrue=0;
						data_component_id = *((unsigned short*)arg);
					}
					if (pDesc->unDesc.stDCD.data_component_id == data_component_id || fTrue) {
						*ppDescriptor = pDesc;
						err = TSPARSE_SIMGMT_OK;
					}
					break;
				default:
					*ppDescriptor = pDesc;
					err = TSPARSE_SIMGMT_OK;
					break;
			}
			if (err == TSPARSE_SIMGMT_OK) break;
		}
		pDesc = pDesc->pstLinkedDescriptor;
	}
	return err;
}

/**
 @brief add a descriptor to a list of global descriptor of section
 @param ppDescRtn reference of pointer to return a pointer to added descriptor
 @param pDescr pointer to descriptor to be added
 @param pSection specify a section of table to which a descriptor will be added
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_Section_AddDescriptor_Global(DESCRIPTOR_STRUCT **ppDescRtn, DESCRIPTOR_STRUCT *pDescriptor, SI_MGMT_SECTION *pSection)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Section_AddDescriptor_Global (ppDescRtn, pDescriptor, pSection);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Section_AddDescriptor_Global(DESCRIPTOR_STRUCT **ppDescRtn, DESCRIPTOR_STRUCT *pDescriptor, SI_MGMT_SECTION *pSection)
{
	DESCRIPTOR_STRUCT *pDesc;
	int err, size;
	if (!CHK_VALIDPTR(pSection)) return TSPARSE_SIMGMT_INVALIDPARAM;

	if (pDescriptor->enDescriptorID == EXTN_EVT_DESCR_ID)
		err = tsparse_isdbt_simgmt_Section_FindDescriptor_Global (&pDesc, pDescriptor->enDescriptorID, (void*)&pDescriptor->unDesc.stEED.ucDescrNumber, pSection);
	else if (pDescriptor->enDescriptorID == DATA_CONTENT_DESCR_ID)
		err = tsparse_isdbt_simgmt_Section_FindDescriptor_Global (&pDesc, pDescriptor->enDescriptorID, (void*)&pDescriptor->unDesc.stDCD.data_component_id, pSection);
	else
		err = tsparse_isdbt_simgmt_Section_FindDescriptor_Global (&pDesc, pDescriptor->enDescriptorID, NULL, pSection);
	if (err==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pDesc)) { /* if there is  already a descriptor */
		ERR_MSG("[Section_AddDescriptor_Global]there is already a descriptor (table_id=%d, descriptor_tag=%d)",
			CHK_VALIDPTR(pSection->pParentTable) ? (int)(pSection->pParentTable->table_id) : -1, pDescriptor->enDescriptorID);
		if (CHK_VALIDPTR(ppDescRtn)) *ppDescRtn = pDesc;
		err = TSPARSE_SIMGMT_ADDDUPLICATE;
	} else {
		err = TSPARSE_SIMGMT_OK;
		if (CHK_VALIDPTR(ppDescRtn)) *ppDescRtn = (DESCRIPTOR_STRUCT *)NULL;
		size = pDescriptor->struct_length;
		err = TSPARSE_ISDBT_SiMgmt_CopyDescriptor (&pDesc, pDescriptor);
		if (err==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pDesc)) {
			pDesc->pstLinkedDescriptor = pSection->pGlobalDesc;
			pSection->pGlobalDesc = pDesc;
			if (CHK_VALIDPTR(ppDescRtn)) *ppDescRtn = pDesc;
		} else {
			if (pDescriptor->enDescriptorID==EXTN_EVT_DESCR_ID || pDescriptor->enDescriptorID==EMERGENCY_INFORMATION_DESCR_ID)
				TSPARSE_ISDBT_SiMgmt_DelDescriptor(pDesc);
			err = TSPARSE_SIMGMT_MEMORYALLOCFAIL;
		}
	}
	return err;
}

/**
 @brief delete a descriptor from a list of global descriptro of section
 @param pDescriptor pointer to descriptor to be deleted
 @param pSection pointer to a section of table from which a descriptor will be deleted
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_Section_DeleteDescriptor_Global (DESCRIPTOR_STRUCT *pDescriptor, SI_MGMT_SECTION *pSection)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_sigment_Section_DeleteDescriptor_Global(pDescriptor, pSection);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_sigment_Section_DeleteDescriptor_Global (DESCRIPTOR_STRUCT *pDescriptor, SI_MGMT_SECTION *pSection)
{
	DESCRIPTOR_STRUCT *pDesc, *pDescTemp;
	int cnt;
	if (!CHK_VALIDPTR(pDescriptor) || !CHK_VALIDPTR(pSection)) return TSPARSE_SIMGMT_INVALIDPARAM;
	pDesc = pSection->pGlobalDesc;
	pDescTemp = (DESCRIPTOR_STRUCT*)NULL;
	for(cnt=0; cnt < LOOP_LIMIT && CHK_VALIDPTR(pDesc); cnt++) {
		if (pDesc->enDescriptorID == pDescriptor->enDescriptorID) {
			if (pDescriptor->enDescriptorID == EXTN_EVT_DESCR_ID) {
				if (pDesc->unDesc.stEED.ucDescrNumber == pDescriptor->unDesc.stEED.ucDescrNumber)
					break;
			} else if (pDescriptor->enDescriptorID == DATA_CONTENT_DESCR_ID) {
				if (pDesc->unDesc.stDCD.data_component_id == pDescriptor->unDesc.stDCD.data_component_id)
					break;
			} else
				break;
		}
		pDescTemp = pDesc;
		pDesc = pDesc->pstLinkedDescriptor;
	}
	if (cnt == LOOP_LIMIT || !CHK_VALIDPTR(pDesc)) return TSPARSE_SIMGMT_INVALIDDATA;

	if (!CHK_VALIDPTR(pDescTemp)) pSection->pGlobalDesc = pDesc->pstLinkedDescriptor;
	else pDescTemp->pstLinkedDescriptor = pDesc->pstLinkedDescriptor;
	TSPARSE_ISDBT_SiMgmt_DelDescriptor(pDesc);
	return TSPARSE_SIMGMT_OK;
}

/**
 @brief delete a descriptor from a list of global descriptro of section
 @param descriptor_id id of descriptor (descriptor_tag)
 @param pSection pointer to a section of table from which a descriptor will be deleted
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_Section_DeleteDescriptorExt_Global(DESCRIPTOR_IDS descriptor_id, void *arg, SI_MGMT_SECTION *pSection)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Section_DeleteDescriptorExt_Global (descriptor_id, arg, pSection);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Section_DeleteDescriptorExt_Global(DESCRIPTOR_IDS descriptor_id, void *arg, SI_MGMT_SECTION *pSection)
{
	DESCRIPTOR_STRUCT *pDescriptor;
	int err = TSPARSE_SIMGMT_OK, status;
	/* Original : Null Pointer Dereference
	unsigned char *p_arg = NULL;

	if (!CHK_VALIDPTR(pSection)) return TSPARSE_SIMGMT_INVALIDPARAM;
	if (descriptor_id == EXTN_EVT_DESCR_ID) {
		if (CHK_VALIDPTR(arg)) *p_arg = *((unsigned char*)arg);
	}
	if (descriptor_id == DATA_CONTENT_DESCR_ID) {
		if (CHK_VALIDPTR(arg)) *p_arg = *((unsigned short*)arg);
	}
	status = tsparse_isdbt_simgmt_Section_FindDescriptor_Global (&pDescriptor, descriptor_id, (void*)p_arg, pSection);
	*/
	// jini 9th : Ok
	if (!CHK_VALIDPTR(pSection))
		return TSPARSE_SIMGMT_INVALIDPARAM;

	status = tsparse_isdbt_simgmt_Section_FindDescriptor_Global (&pDescriptor, descriptor_id, arg, pSection);

	if (status==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pDescriptor)) {
		status = tsparse_isdbt_sigment_Section_DeleteDescriptor_Global (pDescriptor, pSection);
		if (status != TSPARSE_SIMGMT_OK) err = TSPARSE_SIMGMT_INVALIDDATA;
	} else
		err = TSPARSE_SIMGMT_INVALIDDATA;
	return err;
}

/**
 @brief delete all descriptor from a list of global descriptor of section
 @param pSection pointer to a section from which descirptor will be deleted
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_Section_DestroyDescriptor_Global(SI_MGMT_SECTION *pSection)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Section_DestroyDescriptor_Global(pSection);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Section_DestroyDescriptor_Global(SI_MGMT_SECTION *pSection)
{
	DESCRIPTOR_STRUCT *pDesc, *pDescNext;
	int cnt;
	if (!CHK_VALIDPTR(pSection)) return TSPARSE_SIMGMT_INVALIDPARAM;

	pDesc = pSection->pGlobalDesc;
	for(cnt=0; cnt < LOOP_LIMIT && CHK_VALIDPTR(pDesc); cnt++) {
		pDescNext = pDesc->pstLinkedDescriptor;
		tsparse_isdbt_sigment_Section_DeleteDescriptor_Global(pDesc, pSection);
		pDesc = pDescNext;
	}
	pSection->pGlobalDesc = (DESCRIPTOR_STRUCT*)NULL;
	return TSPARSE_SIMGMT_OK;
}

/**
 @brief get a version number from section
 @param p_version_number pointer to return a version number
 @pSection pointer to section from which a version number will be got
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_Section_GetVersioNumber(unsigned char *p_version_number, SI_MGMT_SECTION *pSection)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Section_GetVersioNumber(p_version_number, pSection);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Section_GetVersioNumber(unsigned char *p_version_number, SI_MGMT_SECTION *pSection)
{
	if (!CHK_VALIDPTR(p_version_number) || !CHK_VALIDPTR(pSection)) return TSPARSE_SIMGMT_INVALIDPARAM;
	*p_version_number = pSection->version_number;
	return TSPARSE_SIMGMT_OK;
}


/**
 @brief get a section_number from a section
 @param p_section_number pointer to return a section_number
 @param pSection pointer to a section of table from which a section_number will be got
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_Section_GetSecNo(unsigned char *p_section_number, SI_MGMT_SECTION *pSection)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Section_GetSecNo(p_section_number, pSection);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Section_GetSecNo(unsigned char *p_section_number, SI_MGMT_SECTION *pSection)
{
	if (!CHK_VALIDPTR(p_section_number) || !CHK_VALIDPTR(pSection)) return TSPARSE_SIMGMT_INVALIDPARAM;
	*p_section_number = pSection->section_number;
	return TSPARSE_SIMGMT_OK;
}

/**
 @brief get a last_section_number from a section
 @param p_last_section_number pointer to return a last_section_number
 @param pSection pointer to a section of table from which a last_section_number will be got
@return 0=success, else=failure
*/
int TSPARSE_ISDBT_SiMgmt_Section_GetLastSecNo(unsigned char *p_last_section_number, SI_MGMT_SECTION *pSection)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Section_GetLastSecNo(p_last_section_number, pSection);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Section_GetLastSecNo(unsigned char *p_last_section_number, SI_MGMT_SECTION *pSection)
{
	if (!CHK_VALIDPTR(p_last_section_number) || !CHK_VALIDPTR(pSection)) return TSPARSE_SIMGMT_INVALIDPARAM;
	*p_last_section_number = pSection->last_section_number;
	return TSPARSE_SIMGMT_OK;
}

/**
 @brief get a pointer to section data
 @param ppSectionExt reference of pointer to return a pointer to section data to caller
 @param pSection pointer to a section of table from which a section data will be got
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_Section_GetSectionExt(SI_MGMT_SECTION_EXTENSION **ppSectionExt, SI_MGMT_SECTION *pSection)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Section_GetSectionExt (ppSectionExt, pSection);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Section_GetSectionExt(SI_MGMT_SECTION_EXTENSION **ppSectionExt, SI_MGMT_SECTION *pSection)
{
	if (!CHK_VALIDPTR(ppSectionExt) || !CHK_VALIDPTR(pSection)) return TSPARSE_SIMGMT_INVALIDPARAM;
	*ppSectionExt = &(pSection->unSectionExt);
	return TSPARSE_SIMGMT_OK;
}

/**
 @brief get a pinter to global descriptor from a section
 @param ppDescriptor reference of pointer to return a pointer to global descriptor
 @param pSection pointer to a section of table from which a global descriptor will be got
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_Section_GetDescriptor_Global(DESCRIPTOR_STRUCT **ppDescriptor, SI_MGMT_SECTION *pSection)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = TSPARSE_ISDBT_SiMgmt_Section_GetDescriptor_Global (ppDescriptor, pSection);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Section_GetDescriptor_Global(DESCRIPTOR_STRUCT **ppDescriptor, SI_MGMT_SECTION *pSection)
{
	int err = TSPARSE_SIMGMT_OK;
	if (!CHK_VALIDPTR(ppDescriptor) || !CHK_VALIDPTR(pSection)) return TSPARSE_SIMGMT_INVALIDPARAM;

	if (CHK_VALIDPTR(pSection->pGlobalDesc)) *ppDescriptor = pSection->pGlobalDesc;
	else {
		*ppDescriptor = (DESCRIPTOR_STRUCT*)NULL;
		err = TSPARSE_SIMGMT_INVALIDDATA;
	}
	return err;
}

/**
 @brief get a pointer to section loop from a section
 @param ppSectionLoop reference of pointer to return a pointer to section loop
 @param pSection pointer to a section of table from which a section loop will be got
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_Section_GetLoop(SI_MGMT_SECTIONLOOP **ppSectionLoop, SI_MGMT_SECTION *pSection)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Section_GetLoop (ppSectionLoop, pSection);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Section_GetLoop(SI_MGMT_SECTIONLOOP **ppSectionLoop, SI_MGMT_SECTION *pSection)
{
	int err = TSPARSE_SIMGMT_OK;
	if (!CHK_VALIDPTR(ppSectionLoop) || !CHK_VALIDPTR(pSection)) return TSPARSE_SIMGMT_INVALIDPARAM;

	if (CHK_VALIDPTR(pSection->pSectionLoop)) {
		*ppSectionLoop = pSection->pSectionLoop;
	} else {
		*ppSectionLoop = (SI_MGMT_SECTIONLOOP*)NULL;
		err = TSPARSE_SIMGMT_INVALIDDATA;
	}
	return err;
}

/**
 @brief get a pointer to table to which a section belong
 @param ppTable reference of pointer to return a pointer to table
 @param pSection pointer to a section
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_Section_GetParentTable(SI_MGMT_TABLE **ppTable, SI_MGMT_SECTION *pSection)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Section_GetParentTable(ppTable, pSection);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Section_GetParentTable(SI_MGMT_TABLE **ppTable, SI_MGMT_SECTION *pSection)
{
	int err = TSPARSE_SIMGMT_OK;
	if (!CHK_VALIDPTR(ppTable) || !CHK_VALIDPTR(pSection)) return TSPARSE_SIMGMT_INVALIDPARAM;

	if (CHK_VALIDPTR(pSection->pParentTable)) *ppTable = pSection->pParentTable;
	else {
		*ppTable = (SI_MGMT_TABLE*)NULL;
		err = TSPARSE_SIMGMT_INVALIDDATA;
	}
	return err;
}

/**
 @brief get a pointer to next section
 @param ppSection reference of pointer to return a next section
 @param pSection pointer to section from which a pointer to next section will be got
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_Section_GetNextSection(SI_MGMT_SECTION **ppSection, SI_MGMT_SECTION *pSection)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Section_GetNextSection(ppSection, pSection);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Section_GetNextSection(SI_MGMT_SECTION **ppSection, SI_MGMT_SECTION *pSection)
{
	int err = TSPARSE_SIMGMT_OK;
	if (!CHK_VALIDPTR(ppSection) || !CHK_VALIDPTR(pSection)) return TSPARSE_SIMGMT_INVALIDPARAM;

	if (CHK_VALIDPTR(pSection->pNext)) *ppSection = pSection->pNext;
	else {
		*ppSection = (SI_MGMT_SECTION*)NULL;
		err = TSPARSE_SIMGMT_INVALIDDATA;
	}
	return err;
}

/**
  @brief set section data
  @param punSectionExt pointer to section data which will be updated to section
  @param pSection pointer to a section of table to which data will be set
  @return 0=success, else=failure
  */
int TSPARSE_ISDBT_SiMgmt_Section_SetSectionExt (SI_MGMT_SECTION_EXTENSION *punSectionExt, SI_MGMT_SECTION *pSection)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Section_SetSectionExt (punSectionExt, pSection);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Section_SetSectionExt (SI_MGMT_SECTION_EXTENSION *punSectionExt, SI_MGMT_SECTION *pSection)
{
	if (!CHK_VALIDPTR(punSectionExt) || !CHK_VALIDPTR(pSection)) return TSPARSE_SIMGMT_INVALIDPARAM;
	memcpy(&(pSection->unSectionExt), punSectionExt, sizeof(SI_MGMT_SECTION_EXTENSION));
	return TSPARSE_SIMGMT_OK;
}

/*---------- section loop -------*/

/**
 @brief find a descriptor from a section loop
 @param ppDescriptor reference of pointer to return a pointer to found descriptor
 @param descriptor_id id of descriptor
 @param pSectionLoop pointer to a section loop from which a descriptor will be got
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_SectionLoop_FindDescriptor(DESCRIPTOR_STRUCT **ppDescriptor, DESCRIPTOR_IDS descriptor_id, void *arg, SI_MGMT_SECTIONLOOP *pSectionLoop)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_SectionLoop_FindDescriptor (ppDescriptor, descriptor_id, arg, pSectionLoop);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_SectionLoop_FindDescriptor(DESCRIPTOR_STRUCT **ppDescriptor, DESCRIPTOR_IDS descriptor_id, void *arg, SI_MGMT_SECTIONLOOP *pSectionLoop)
{
	DESCRIPTOR_STRUCT *pDesc;
	int count, err = TSPARSE_SIMGMT_INVALIDDATA;
	unsigned char descriptor_number = 0, fTrue;
	unsigned short data_component_id=0;
	if (!CHK_VALIDPTR(ppDescriptor) || !CHK_VALIDPTR(pSectionLoop)) return TSPARSE_SIMGMT_INVALIDPARAM;

	*ppDescriptor = (DESCRIPTOR_STRUCT*)NULL;
	pDesc = pSectionLoop->pDesc;
	for (count=0; count < LOOP_LIMIT && CHK_VALIDPTR(pDesc); count++) {
		if (pDesc->enDescriptorID == descriptor_id) {
			switch (descriptor_id) {
				case EXTN_EVT_DESCR_ID:
					if (!CHK_VALIDPTR(arg)) fTrue=1;
					else {
						fTrue = 0;
						descriptor_number = *((unsigned char*)arg);
					}
					if (pDesc->unDesc.stEED.ucDescrNumber == descriptor_number || fTrue) {
						*ppDescriptor = pDesc;
						err = TSPARSE_SIMGMT_OK;
					}
					break;
				case DATA_CONTENT_DESCR_ID:
					if (!CHK_VALIDPTR(arg)) fTrue=1;
					else {
						fTrue = 0;
						data_component_id = *((unsigned short*)arg);
					}
					if (pDesc->unDesc.stDCD.data_component_id == data_component_id || fTrue) {
						*ppDescriptor = pDesc;
						err = TSPARSE_SIMGMT_OK;
					}
					break;
				default:
					*ppDescriptor = pDesc;
					err = TSPARSE_SIMGMT_OK;
					break;
			}
			if (err == TSPARSE_SIMGMT_OK) break;
		}
		pDesc = pDesc->pstLinkedDescriptor;
	}
	return err;
}

/**
 @brief add a descriptor to a section loop
 @param ppDescRtn reference of pointer to return a pointer to added descriptor
 @param pDescriptor pointer to descriptor which will be added to a section loop
 @param pSectinLoop pointer to a section loop to which a descriptor will be added
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_SectionLoop_AddDescriptor(DESCRIPTOR_STRUCT **ppDescRtn, DESCRIPTOR_STRUCT *pDescriptor, SI_MGMT_SECTIONLOOP *pSectionLoop)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_SectionLoop_AddDescriptor (ppDescRtn, pDescriptor, pSectionLoop);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_SectionLoop_AddDescriptor(DESCRIPTOR_STRUCT **ppDescRtn, DESCRIPTOR_STRUCT *pDescriptor, SI_MGMT_SECTIONLOOP *pSectionLoop)
{
	DESCRIPTOR_STRUCT *pDesc;
	int size, err;
	if (!CHK_VALIDPTR(pDescriptor) || !CHK_VALIDPTR(pSectionLoop)) return TSPARSE_SIMGMT_INVALIDPARAM;

	if (pDescriptor->enDescriptorID == EXTN_EVT_DESCR_ID)
		err = tsparse_isdbt_simgmt_SectionLoop_FindDescriptor (&pDesc, pDescriptor->enDescriptorID, (void*)&pDescriptor->unDesc.stEED.ucDescrNumber, pSectionLoop);
	else if (pDescriptor->enDescriptorID == DATA_CONTENT_DESCR_ID)
		err = tsparse_isdbt_simgmt_SectionLoop_FindDescriptor (&pDesc, pDescriptor->enDescriptorID, (void*)&pDescriptor->unDesc.stDCD.data_component_id, pSectionLoop);
	else
		err = tsparse_isdbt_simgmt_SectionLoop_FindDescriptor (&pDesc, pDescriptor->enDescriptorID, NULL, pSectionLoop);
	if (err==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pDesc)) { /* if there is  already a descriptor */
		ERR_MSG("[SectionLoop_AddDescriptor]there is already a descriptor (descriptor_tag=%d)", pDescriptor->enDescriptorID);
		if (CHK_VALIDPTR(ppDescRtn)) *ppDescRtn = pDesc;
		err = TSPARSE_SIMGMT_ADDDUPLICATE;
	} else {
		err = TSPARSE_SIMGMT_OK;
		if (CHK_VALIDPTR(ppDescRtn)) *ppDescRtn = (DESCRIPTOR_STRUCT*)NULL;
		size = pDescriptor->struct_length;
		err = TSPARSE_ISDBT_SiMgmt_CopyDescriptor (&pDesc, pDescriptor);
		if (err==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pDesc)) {
			pDesc->pstLinkedDescriptor = pSectionLoop->pDesc;
			pSectionLoop->pDesc = pDesc;
			if (CHK_VALIDPTR(ppDescRtn)) *ppDescRtn = pDesc;
		} else {
			if (pDescriptor->enDescriptorID==EXTN_EVT_DESCR_ID || pDescriptor->enDescriptorID==EMERGENCY_INFORMATION_DESCR_ID)
				TSPARSE_ISDBT_SiMgmt_DelDescriptor(pDesc);
			err = TSPARSE_SIMGMT_MEMORYALLOCFAIL;
		}
	}
	return err;;
}

/**
 @brief delete a descriptor from a section loop
 @param pDescriptor pointer to descriptor which will be deleted
 @param pSectionLoop pointer to a section loop from which a descriptor will be deleted
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_SectionLoop_DeleteDescriptor (DESCRIPTOR_STRUCT *pDescriptor, SI_MGMT_SECTIONLOOP *pSectionLoop)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_SectionLoop_DeleteDescriptor(pDescriptor, pSectionLoop);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_SectionLoop_DeleteDescriptor (DESCRIPTOR_STRUCT *pDescriptor, SI_MGMT_SECTIONLOOP *pSectionLoop)
{
	DESCRIPTOR_STRUCT *pDesc, *pDescTemp;
	int cnt, err=TSPARSE_SIMGMT_OK;
	if (!CHK_VALIDPTR(pDescriptor) || !CHK_VALIDPTR(pSectionLoop)) return TSPARSE_SIMGMT_INVALIDPARAM;
	pDesc = pSectionLoop->pDesc;
	pDescTemp = (DESCRIPTOR_STRUCT*)NULL;
	for(cnt=0; cnt < LOOP_LIMIT && CHK_VALIDPTR(pDesc); cnt++) {
		if (pDesc->enDescriptorID == pDescriptor->enDescriptorID) {
			if (pDescriptor->enDescriptorID == EXTN_EVT_DESCR_ID) {
				if (pDesc->unDesc.stEED.ucDescrNumber == pDescriptor->unDesc.stEED.ucDescrNumber)
					break;
			} else if (pDescriptor->enDescriptorID == DATA_CONTENT_DESCR_ID) {
				if (pDesc->unDesc.stDCD.data_component_id == pDescriptor->unDesc.stDCD.data_component_id)
					break;
			} else
				break;
		}

		if (pDesc->enDescriptorID == pDescriptor->enDescriptorID) break;
		pDescTemp = pDesc;
		pDesc = pDesc->pstLinkedDescriptor;
	}
	if (cnt == LOOP_LIMIT || !CHK_VALIDPTR(pDesc)) err = TSPARSE_SIMGMT_INVALIDDATA;
	else {
		if (!CHK_VALIDPTR(pDescTemp)) pSectionLoop->pDesc = pDesc->pstLinkedDescriptor;
		else pDescTemp->pstLinkedDescriptor = pDesc->pstLinkedDescriptor;
		TSPARSE_ISDBT_SiMgmt_DelDescriptor(pDesc);
	}
	return err;;
}

/**
 @brief delete a descriptor from a section loop
 @param descriptor_id id of descriptor
 @param pSectionLoop pointer to a section loop from which a descriptor will be deleted
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_SectionLoop_DeleteDescriptorExt(DESCRIPTOR_IDS descriptor_id, void *arg, SI_MGMT_SECTIONLOOP *pSectionLoop)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_SectionLoop_DeleteDescriptorExt (descriptor_id, arg, pSectionLoop);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_SectionLoop_DeleteDescriptorExt(DESCRIPTOR_IDS descriptor_id, void *arg, SI_MGMT_SECTIONLOOP *pSectionLoop)
{
	DESCRIPTOR_STRUCT *pDesc;
	int err;
	/* Original Null Pointer Dereference
	unsigned char *p_arg = NULL;

	if (!CHK_VALIDPTR(pSectionLoop)) return TSPARSE_SIMGMT_INVALIDPARAM;
	if (descriptor_id == EXTN_EVT_DESCR_ID) {
		if (CHK_VALIDPTR(arg)) *p_arg = *((unsigned char*)arg);
	}
	if (descriptor_id == DATA_CONTENT_DESCR_ID) {
		if (CHK_VALIDPTR(arg)) *p_arg = *((unsigned short*)arg);
	}

	pDesc = (DESCRIPTOR_STRUCT*)NULL;
	err = tsparse_isdbt_simgmt_SectionLoop_FindDescriptor(&pDesc, descriptor_id, (void*)p_arg, pSectionLoop);
	*/
	// jini 9th : Ok
	if (!CHK_VALIDPTR(pSectionLoop))
		return TSPARSE_SIMGMT_INVALIDPARAM;

	err = tsparse_isdbt_simgmt_SectionLoop_FindDescriptor(&pDesc, descriptor_id, arg, pSectionLoop);
	if(err==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pDesc)) {
		err = tsparse_isdbt_simgmt_SectionLoop_DeleteDescriptor (pDesc, pSectionLoop);
		if(err != TSPARSE_SIMGMT_OK) err = TSPARSE_SIMGMT_INVALIDDATA;
	} else err = TSPARSE_SIMGMT_INVALIDDATA;
	return err;
}

/**
 @brief delete all descriptor from a section loop
 @param pSectinLoop pointer to a section loop from which a descriptor will be deleted
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_SectionLoop_DestroyDescriptor (SI_MGMT_SECTIONLOOP *pSectionLoop)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_SectionLoop_DestroyDescriptor(pSectionLoop);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_SectionLoop_DestroyDescriptor (SI_MGMT_SECTIONLOOP *pSectionLoop)
{
	DESCRIPTOR_STRUCT *pDesc, *pDescNext;
	int cnt;
	if (!CHK_VALIDPTR(pSectionLoop)) return TSPARSE_SIMGMT_INVALIDPARAM;

	pDesc = pSectionLoop->pDesc;
	for(cnt=0; cnt < LOOP_LIMIT && CHK_VALIDPTR(pDesc); cnt++) {
		pDescNext = pDesc->pstLinkedDescriptor;
		tsparse_isdbt_simgmt_SectionLoop_DeleteDescriptor(pDesc, pSectionLoop);
		pDesc = pDescNext;
	}
	pSectionLoop->pDesc = (DESCRIPTOR_STRUCT*)NULL;
	return TSPARSE_SIMGMT_OK;
}

/**
 @brief get a pointer to data of section loop
 @param pSectionLoopData reference of pointer to return a  pointer to data of section loop
 @param pSectionLoop pointer to a section loop from which a data of section loop will be got
 @reutn 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_SectionLoop_GetLoopData(SI_MGMT_SECTIONLOOP_DATA **ppSectionLoopData, SI_MGMT_SECTIONLOOP *pSectionLoop)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_SectionLoop_GetLoopData (ppSectionLoopData, pSectionLoop);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_SectionLoop_GetLoopData(SI_MGMT_SECTIONLOOP_DATA **ppSectionLoopData, SI_MGMT_SECTIONLOOP *pSectionLoop)
{
	if(!CHK_VALIDPTR(ppSectionLoopData) || !CHK_VALIDPTR(pSectionLoop)) return TSPARSE_SIMGMT_INVALIDPARAM;
	*ppSectionLoopData = &(pSectionLoop->unSectionLoopData);
	return TSPARSE_SIMGMT_OK;
}

/**
 @brief get a pointer to descriptor from a seciton loop
 @param ppDescriptor reference of pointer to return a pointer to descriptor
 @param pSectionLoop pointer to a section loop from which a pointer to descriptor will be got
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_SectionLoop_GetDescriptor(DESCRIPTOR_STRUCT **ppDescriptor, SI_MGMT_SECTIONLOOP *pSectionLoop)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_SectionLoop_GetDescriptor (ppDescriptor, pSectionLoop);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_SectionLoop_GetDescriptor(DESCRIPTOR_STRUCT **ppDescriptor, SI_MGMT_SECTIONLOOP *pSectionLoop)
{
	int err = TSPARSE_SIMGMT_INVALIDDATA;
	if (!CHK_VALIDPTR(ppDescriptor) || !CHK_VALIDPTR(pSectionLoop)) return TSPARSE_SIMGMT_INVALIDPARAM;

	if (CHK_VALIDPTR(pSectionLoop->pDesc)) {
		*ppDescriptor = pSectionLoop->pDesc;
		err = TSPARSE_SIMGMT_OK;
	} else
		*ppDescriptor = (DESCRIPTOR_STRUCT*)NULL;
	return err;
}

/**
 @brief get a pointer to section to which a sectin loop is belong
 @param ppSection reference of pointer to return a pointer to section
 @param pSectionLoop pointer to a section loop from which a pointer to section will be got
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_SectionLoop_GetParentSection (SI_MGMT_SECTION **ppSection, SI_MGMT_SECTIONLOOP *pSectionLoop)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_SectionLoop_GetParentSection (ppSection, pSectionLoop);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_SectionLoop_GetParentSection (SI_MGMT_SECTION **ppSection, SI_MGMT_SECTIONLOOP *pSectionLoop)
{
	int err = TSPARSE_SIMGMT_INVALIDDATA;
	if (!CHK_VALIDPTR(ppSection) || !CHK_VALIDPTR(pSectionLoop)) return TSPARSE_SIMGMT_INVALIDPARAM;
	if (CHK_VALIDPTR(pSectionLoop->pParentSection)) {
		*ppSection = pSectionLoop->pParentSection;
		err = TSPARSE_SIMGMT_OK;
	} else
		*ppSection = (SI_MGMT_SECTION*)NULL;
	return err;
}

/**
 @brief get a pointer to next loop of section from a section loop
 @param ppSectionLoop reference of pointer to return a pointer to next loop
 @param pSectionLoop pointer to a section loop from which a pointer to next loop will be got
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_SectionLoop_GetNextLoop(SI_MGMT_SECTIONLOOP **ppSectionLoop, SI_MGMT_SECTIONLOOP *pSectionLoop)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_SectionLoop_GetNextLoop(ppSectionLoop, pSectionLoop);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_SectionLoop_GetNextLoop(SI_MGMT_SECTIONLOOP **ppSectionLoop, SI_MGMT_SECTIONLOOP *pSectionLoop)
{
	int err = TSPARSE_SIMGMT_INVALIDDATA;
	if (!CHK_VALIDPTR(ppSectionLoop) || !CHK_VALIDPTR(pSectionLoop)) return TSPARSE_SIMGMT_INVALIDPARAM;
	if (CHK_VALIDPTR(pSectionLoop->pNext)) {
		*ppSectionLoop = pSectionLoop->pNext;
		err = TSPARSE_SIMGMT_OK;
	} else
		*ppSectionLoop = (SI_MGMT_SECTIONLOOP*)NULL;
	return err;
}

/**
 @brief set a loop data of section
 @partam pSectionLoopData pointer to a loop data of section
 @param pSectionLoop pointer to a section loop to which a loop data will be set
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_SectionLoop_SetLoopData (SI_MGMT_SECTIONLOOP_DATA *pSectionLoopData, SI_MGMT_SECTIONLOOP *pSectionLoop)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_SectionLoop_SetLoopData(pSectionLoopData, pSectionLoop);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_SectionLoop_SetLoopData (SI_MGMT_SECTIONLOOP_DATA *pSectionLoopData, SI_MGMT_SECTIONLOOP *pSectionLoop)
{
	if (!CHK_VALIDPTR(pSectionLoopData) || !CHK_VALIDPTR(pSectionLoop)) return TSPARSE_SIMGMT_INVALIDPARAM;

	memcpy(&(pSectionLoop->unSectionLoopData), pSectionLoopData, sizeof(SI_MGMT_SECTIONLOOP_DATA));
	return TSPARSE_SIMGMT_OK;
}

/*--------descriptor ---------*/
/**
 @brief add a descriptor to any existing descriptor
 @param ppDescRtn reference of pointer to an added descriptor
 @param pDescriptor pointer to descriptor to be added
 @param pSectionDescriptorList pointer to a list of descriptor to which a descriptor will be added
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_Descriptor_Add (DESCRIPTOR_STRUCT **ppDescRtn, DESCRIPTOR_STRUCT *pDescriptor, DESCRIPTOR_STRUCT *pSectionDescriptorList)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Descriptor_Add (ppDescRtn, pDescriptor, pSectionDescriptorList);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Descriptor_Add (DESCRIPTOR_STRUCT **ppDescRtn, DESCRIPTOR_STRUCT *pDescriptor, DESCRIPTOR_STRUCT *pSectionDescriptorList)
{
	DESCRIPTOR_STRUCT *pDesc;
	int err=TSPARSE_SIMGMT_OK;
	if (!CHK_VALIDPTR(pDescriptor) || !CHK_VALIDPTR(pSectionDescriptorList)) return TSPARSE_SIMGMT_INVALIDPARAM;

	if (CHK_VALIDPTR(ppDescRtn))*ppDescRtn = (DESCRIPTOR_STRUCT*)NULL;
	if (pDescriptor->enDescriptorID == EXTN_EVT_DESCR_ID)
		err = tsparse_isdbt_simgmt_Descriptor_Find (&pDesc, pDescriptor->enDescriptorID, (void*)&pDescriptor->unDesc.stEED.ucDescrNumber, pSectionDescriptorList);
	else if (pDescriptor->enDescriptorID == DATA_CONTENT_DESCR_ID)
		err = tsparse_isdbt_simgmt_Descriptor_Find (&pDesc, pDescriptor->enDescriptorID, (void*)&pDescriptor->unDesc.stDCD.data_component_id, pSectionDescriptorList);
	else
		err = tsparse_isdbt_simgmt_Descriptor_Find (&pDesc, pDescriptor->enDescriptorID, NULL, pSectionDescriptorList);
	if (err==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pDesc)) { /* if there is  already a descriptor */
		ERR_MSG("[Descriptor_Add]there is already a descriptor (descriptor_tag=%d)", pDescriptor->enDescriptorID);
		if (CHK_VALIDPTR(ppDescRtn)) *ppDescRtn = pDesc;
		err = TSPARSE_SIMGMT_ADDDUPLICATE;
	} else {
		err = TSPARSE_ISDBT_SiMgmt_CopyDescriptor (&pDesc, pDescriptor);
		if (err==TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pDesc)) {
			pDesc->pstLinkedDescriptor = pSectionDescriptorList->pstLinkedDescriptor;
			pSectionDescriptorList->pstLinkedDescriptor = pDesc;
			if (CHK_VALIDPTR(ppDescRtn)) *ppDescRtn = pDesc;
		} else {
			if (pDescriptor->enDescriptorID==EXTN_EVT_DESCR_ID || pDescriptor->enDescriptorID==EMERGENCY_INFORMATION_DESCR_ID)
				TSPARSE_ISDBT_SiMgmt_DelDescriptor(pDesc);
			err = TSPARSE_SIMGMT_MEMORYALLOCFAIL;
		}
	}
	return err;;
}

/**
 @brief find a descriptor from a list of any existing descriptor

 a list of descriptor can be either global descriptor or loop descriptor
 @param ppDescriptor reference of pointer to descriptor to return a found descriptor
 @param descriptor_id id of descriptor
 @param pSectionDescriptorList pointer to a list of descriptor from which a descriptor will be got
 @return 0=success, else=failure
 */
int TSPARSE_ISDBT_SiMgmt_Descriptor_Find(DESCRIPTOR_STRUCT **ppDescriptor, DESCRIPTOR_IDS descriptor_id, void *arg, DESCRIPTOR_STRUCT *pSectionDescripotrList)
{
	int err;
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	err = tsparse_isdbt_simgmt_Descriptor_Find (ppDescriptor, descriptor_id, arg, pSectionDescripotrList);
	LOCK_STOP();
	return err;
}
int tsparse_isdbt_simgmt_Descriptor_Find(DESCRIPTOR_STRUCT **ppDescriptor, DESCRIPTOR_IDS descriptor_id, void *arg, DESCRIPTOR_STRUCT *pSectionDescripotrList)
{
	DESCRIPTOR_STRUCT *pDesc;
	int count, err=TSPARSE_SIMGMT_INVALIDDATA;
	unsigned char descriptor_number = 0, fTrue;
	unsigned short data_component_id=0;
	if (!CHK_VALIDPTR(ppDescriptor) || !CHK_VALIDPTR(pSectionDescripotrList)) return TSPARSE_SIMGMT_INVALIDPARAM;

	*ppDescriptor = (DESCRIPTOR_STRUCT*)NULL;
	pDesc = pSectionDescripotrList;
	for (count=0; count < LOOP_LIMIT && CHK_VALIDPTR(pDesc); count++) {
		if (pDesc->enDescriptorID == descriptor_id) {
			switch (descriptor_id) {
				case EXTN_EVT_DESCR_ID:
					if (!CHK_VALIDPTR(arg)) fTrue = 1;
					else {
						fTrue = 0;
						descriptor_number = *((unsigned char*)arg);
					}
					if (pDesc->unDesc.stEED.ucDescrNumber == descriptor_number || fTrue) {
						*ppDescriptor = pDesc;
						err = TSPARSE_SIMGMT_OK;
					}
					break;
				case DATA_CONTENT_DESCR_ID:
					if (!CHK_VALIDPTR(arg)) fTrue = 1;
					else {
						fTrue = 0;
						data_component_id = *((unsigned short*)arg);
					}
					if (pDesc->unDesc.stDCD.data_component_id == data_component_id || fTrue) {
						*ppDescriptor = pDesc;
						err = TSPARSE_SIMGMT_OK;
					}
					break;
				default:
					*ppDescriptor = pDesc;
					err = TSPARSE_SIMGMT_OK;
					break;
			}
			if (err == TSPARSE_SIMGMT_OK) break;
		}
		pDesc = pDesc->pstLinkedDescriptor;
	}
	return err;
}

/*=== test ===*/
void display_conditional_access_descriptor(DESCRIPTOR_STRUCT *pDesc)
{
	char *name = "conditional_access_descriptor";
	CA_DESCR *p;
	if (CHK_VALIDPTR(pDesc)) {
		ALOGD("[SiMgmt]display %s", name);
		p = &pDesc->unDesc.stCAD;
		ALOGD("[SiMgmt]CA_system_id=0x%x, CA_PID=0x%x", p->uiCASystemID, p->uiCA_PID);
	}
}
void display_network_name_descriptor(DESCRIPTOR_STRUCT *pDesc)
{
	char *name = "network_name_descriptor";
	NETWORK_NAME_DESCR *p;
	unsigned int i, size;
	if (CHK_VALIDPTR(pDesc)) {
		ALOGD("[SiMgmt]display %s", name);
		p = &pDesc->unDesc.stNND;
		size = (unsigned int)(p->ucNameLen);
		ALOGD("[SiMgmt]network name is");
		for (i=0; i < size && i < 16; i++)
			ALOGD("[SiMgmt] 0x%x", p->pcNetName[i]);
	}
}
void display_service_list_descriptor(DESCRIPTOR_STRUCT *pDesc)
{
	char *name = "service_list_descriptor";
	SERVICE_LIST_DESCR *p;
	unsigned int i, total;
	if (CHK_VALIDPTR(pDesc)) {
		ALOGD("[SiMgmt]display %s", name);
		p = &pDesc->unDesc.stSLD;
		total = (unsigned int)(p->ucSvcListCount);
		for(i=0; i < total; i++)
			ALOGD("[SiMgmt][%d] service_id=0x%x, service_type=0x%x", i, p->pastSvcList[i].uiServiceID, p->pastSvcList[i].enSvcType);
	}
}
void display_stuffing_descriptor(DESCRIPTOR_STRUCT *pDesc)
{
	char *name = "stuffing_descriptor";
	ALOGD("[SiMgmt]display %s", name);
}
void display_service_descriptor(DESCRIPTOR_STRUCT *pDesc)
{
	char *name = "service_descriptor";
	SERVICE_DESCR *p;
	int i;
	if(CHK_VALIDPTR(pDesc)) {
		ALOGD("[SiMgmt]display %s", name);
		p = &pDesc->unDesc.stSD;
		ALOGD("[SiMgmt]service_type=0x%x", p->enSvcType);
		ALOGD("[SiMgmt] service_provider_name is");
		for(i=0; i < p->ucSvcProvLen && i < 16; i++) ALOGD("[SiMgmt]  0x%x", p->pszSvcProvider[i]);
		ALOGD("[SiMgmt] service_name is");
		for(i=0; i < p->ucSvcNameLen && i < 16; i++) ALOGD("[SiMgmt]  0x%x", p->pszSvcName[i]);
	}
}
void display_linkage_descriptor(DESCRIPTOR_STRUCT *pDesc)
{
	char *name = "linkage_descriptor";
	LINKAGE_DESCR *p;
	if (CHK_VALIDPTR(pDesc)) {
		ALOGD("[SiMgmt]display %s", name);
		p = &pDesc->unDesc.stLD;
		ALOGD("[SiMgmt]transport_stream_id=0x%x, original_network_id=0x%x, service_id=0x%x, linkage_type=0x%x",
			p->uiTStreamID, p->wOrgNetID, p->uiServiceID, p->enLinkageType);
	}
}
void display_short_event_descriptor(DESCRIPTOR_STRUCT *pDesc)
{
	char *name = "short_event_descriptor";
	SHRT_EVT_DESCR *p;
	int i;
	if (CHK_VALIDPTR(pDesc)) {
		ALOGD("[SiMgmt]display %s", name);
		p = &pDesc->unDesc.stSED;
		ALOGD("[SiMgmt]ISO_639_lang=0x%x 0x%x 0x%x", p->acLangCode[0], p->acLangCode[1], p->acLangCode[2]);
		ALOGD("[SiMgmt]event_name_length=%d", p->ucShortEventNameLen);
		for(i=0; i<p->ucShortEventNameLen && i<16;i++)
			ALOGD("[SiMgmt] 0x%x", p->pszShortEventName[i]);
	}
}
void display_exented_event_descriptor(DESCRIPTOR_STRUCT *pDesc)
{
	char *name = "exented_event_descriptor";
	EXT_EVT_DESCR *p;
	if (CHK_VALIDPTR(pDesc)) {
		ALOGD("[SiMgmt]display %s", name);
		p = &pDesc->unDesc.stEED;
		ALOGD("[SiMgmt]descriptor_number=%d, last_descriptor_number=%d, ISO_639_lang=0x%x 0x%x 0x%x, length_of_items=%d",
			p->ucDescrNumber,p->ucLastDescrNumber,p->acLangCode[0],p->acLangCode[1],p->acLangCode[2], p->ucExtEvtCount);
	}
}
void display_component_descriptor(DESCRIPTOR_STRUCT *pDesc)
{
	char *name = "component_descriptor";
	COMPONENT_DESCR *p;
	if (CHK_VALIDPTR(pDesc)) {
		ALOGD("[SiMgmt]display %s", name);
		p = &pDesc->unDesc.stCOMD;
		ALOGD("[SiMgmt]stream_content=0x%x, component_type=0x%x, component_tag=0x%x, ISO_639_lang=0x%x 0x%x 0x%x",
			p->enStreamContent, p->ucComponentType, p->ucComponentTag,
			p->acLangCode[0],p->acLangCode[1],p->acLangCode[2]);
	}
}
void display_stream_identifier_descriptor(DESCRIPTOR_STRUCT *pDesc)
{
	char *name = "stream_identifier_descriptor";
	STREAM_IDENT_DESCR *p;
	if (CHK_VALIDPTR(pDesc)) {
		p = &pDesc->unDesc.stSID;
		ALOGD("[SiMgmt]display %s", name);
		ALOGD("[SiMgmt]component_tag=0x%x", p->ucComponentTag);
	}
}
void display_content_descriptor(DESCRIPTOR_STRUCT *pDesc)
{
	char *name = "content_descriptor";
	CONTENT_DESCR *p;
	int i;
	if (CHK_VALIDPTR(pDesc)) {
		ALOGD("[SiMgmt]display %s", name);
		p = &pDesc->unDesc.stCOND;
		for (i=0; i < p->ucDataCount; i++)
			ALOGD("[SiMgmt] content_nibble=0x%x 0x%x, user_nibble=0x%x 0x%x",
				p->pastContentData[i].enNib_1, p->pastContentData[i].ucNib_2, p->pastContentData[i].ucUserNib_1, p->pastContentData[i].ucUserNib_2);
	}
}
void display_local_time_offset_descriptor(DESCRIPTOR_STRUCT *pDesc)
{
	char *name = "local_time_offset_descriptor";
	LOCAL_TIME_OFFSET_DESCR *p;
	int i;
	if (CHK_VALIDPTR(pDesc)) {
		ALOGD("[SiMgmt]display %s", name);
		p = &pDesc->unDesc.stLTOD;
		for (i=0; i < p->ucCount; i++) {
			ALOGD("[SiMgmt]country_code=0x%x 0x%x 0x%x",
				p->astLocalTimeOffsetData[i].aucCountryCode[0], p->astLocalTimeOffsetData[i].aucCountryCode[1], p->astLocalTimeOffsetData[i].aucCountryCode[2]);
			ALOGD("[SiMgmt]country_region_id=0x%x, local_time_offset_polarity=0x%x", p->astLocalTimeOffsetData[i].aucCountryRegionID, p->astLocalTimeOffsetData[i].ucLocalTimeOffsetPolarity);
			ALOGD("[SiMgmt]local_time_offset=%c%c", p->astLocalTimeOffsetData[i].ucLocalTimeOffset_BCD[0],p->astLocalTimeOffsetData[i].ucLocalTimeOffset_BCD[1]);
			ALOGD("[SiMgmt]time_of_change=%c%c%c%c%c", p->astLocalTimeOffsetData[i].ucTimeOfChange_BCD[0], p->astLocalTimeOffsetData[i].ucTimeOfChange_BCD[1],
				p->astLocalTimeOffsetData[i].ucTimeOfChange_BCD[2], p->astLocalTimeOffsetData[i].ucTimeOfChange_BCD[3], p->astLocalTimeOffsetData[i].ucTimeOfChange_BCD[4]);
			ALOGD("[SiMgmt]next_time_offset=%c%c", p->astLocalTimeOffsetData[i].ucNextTimeOffset_BCD[0],p->astLocalTimeOffsetData[i].ucNextTimeOffset_BCD[1]);
		}
	}
}
void display_digital_copy_control_descriptor(DESCRIPTOR_STRUCT *pDesc)
{
	char *name = "digital_copy_control_descriptor";
	DIGITALCOPY_CONTROL_DESC *p;
	if (CHK_VALIDPTR(pDesc)) {
		ALOGD("[SiMgmt]display %s", name);
		p = &pDesc->unDesc.stDCCD;
		ALOGD("[SiMgmt]digital_recording_control_data=0x%x, maximum_bitrate_flag=%d, component_control_flag=%d, copy_control_type=0x%x",
			p->digital_recording_control_data, p->maximum_bitrate_flag, p->component_control_flag, p->copy_control_type);
		if (p->copy_control_type != 0) ALOGD("[SiMgmt]APS_control_date=0x%x",p->APS_control_data);
		if (p->maximum_bitrate_flag) ALOGD("[SiMgmt]maximum_bitrate=0x%x", p->maximum_bitrate);
	}
}
void display_audio_component_descriptor(DESCRIPTOR_STRUCT *pDesc)
{
	char *name = "audio_component_descriptor";
	AUDIO_COMPONENT_DESCR *p;
	int i, cnt;
	if(CHK_VALIDPTR(pDesc)) {
		ALOGD("[SiMgmt]display %s", name);
		p = &pDesc->unDesc.stACD;
		ALOGD("[SiMgmt]stream_content=0x%x, component_type=0x%x, component_tag=0x%x, stream_type=0x%x, simulcast_group_tag=0x%x",
			p->stream_content, p->component_type, p->component_tag, p->stream_type, p->simulcast_group_tag);
		ALOGD("[SiMgmt]main_component_flag=0x%x, quality_indicator=0x%x, sampling_rate=0x%x", p->main_component_flag, p->quality_indicator, p->sampling_rate);
		cnt=1;
		if (p->ES_multi_lingual_flag) cnt++;
		for(i=0; i <cnt; i++) {
			ALOGD("[SiMmgt] [%d]ISO_639_Lang=0x%x 0x%x 0x%x", i, p->acLangCode[i][0], p->acLangCode[i][1], p->acLangCode[i][2]);
		}
	}
}
void display_data_contents_descriptor(DESCRIPTOR_STRUCT *pDesc)
{
	char *name = "data_contents_descriptor";
	DATA_CONTENT_DESCR *p;
	unsigned int i;
	if (CHK_VALIDPTR(pDesc)) {
		p = &pDesc->unDesc.stDCD;
		ALOGD("[SiMgmt]display %s", name);
		ALOGD("[SiMgmt]data_componnet_id=0x%x, entry_component=0x%x, num_of_language=0x%x", p->data_component_id, p->entry_component, p->num_of_language);
		for (i=0; i < p->num_of_language; i++) {
			ALOGD("[SiMgmt] [%d]language_tag=0x%x, DMF=0x%x, ISO_639_lang=0x%x", i, p->language_tag[i], p->DMF[i], p->ISO_639_language_code[i]);
		}
		ALOGD("[SiMgmt] ISO_639_language_code=0x%x", p->ISO_639_language_code_i);
	}
}
void display_CA_contract_info_descriptor(DESCRIPTOR_STRUCT *pDesc)
{
	char *name = "CA_contract_info_descriptor";
}
void display_CA_service_descriptor(DESCRIPTOR_STRUCT *pDesc)
{
	char *name = "CA_service_descriptor";
	CA_SERVICE_DESCR *p;
	int i, count;
	if (CHK_VALIDPTR(pDesc)) {
		ALOGD("[SiMgmt]display %s", name);
		p = &pDesc->unDesc.stCASD;
		ALOGD("[SiMgmt]CA_system_id=0x%x, CA_broadcaster_group_id=0x%x, message_contro=0x%x", p->uiCA_Service_SystemID, p->ucCA_Service_Broadcast_GroutID, p->ucCA_Service_MessageCtrl);
		count = (p->ucCA_Service_DescLen - 4) / 2;
		for(i=0; i < count; i++)
			ALOGD("service_id[%d]=0x%x", i, p->aucServiceID[i]);
	}
}
void display_TS_information_descriptor(DESCRIPTOR_STRUCT *pDesc)
{
	char *name = "TS_information_descriptor";
	TS_INFO_DESCR *p;
	int i;
	if (CHK_VALIDPTR(pDesc)) {
		ALOGD("[SiMgmt]display %s", name);
		p = &pDesc->unDesc.stTSID;
		ALOGD("[SiMmgt]remote_control_key_id=%d, ts_name is", p->remote_control_key_id);
		for(i=0; i < p->length_of_ts_name && i < 20; i++) {
			ALOGD("[SiMgmt]0x%x", p->ts_name_char[i]);
		}
	}
}
void display_extended_broadcaster_descriptor(DESCRIPTOR_STRUCT *pDesc)
{
	char *name = "extended_broadcaster_descriptor";
	EXT_BROADCASTER_DESCR *p;
	int i;
	if (CHK_VALIDPTR(pDesc)) {
		p = &pDesc->unDesc.stEBD;
		ALOGD("[SiMgmt]display %s", name);
		ALOGD("[SiMmgt]broadcaster_type=0x%x, terrestrial_broadcaster_id=0x%x", p->broadcaster_type, p->terrestrial_broadcaster_id);
		for(i=0; i < p->affiliation_id_num; i++) {
			ALOGD("affiliation_id[%d]=0x%x", i, p->affiliation_id_array[i]);
		}
		for(i=0; i < p->broadcaster_id_num; i++) {
			ALOGD("[SiMgmt]broadcaster_id[%d]=0x%x", i, p->broadcaster_id[i]);
		}
	}
}
void display_logo_transmission_descriptor(DESCRIPTOR_STRUCT *pDesc)
{
	char *name = "logo_transmission_descriptor";
	LOGO_TRANSMISSION_DESCR	*p;
	if (CHK_VALIDPTR(pDesc)) {
		ALOGD("[SiMgmt]display %s", name);
		p = &pDesc->unDesc.stLTD;
		ALOGD("[SiMgmt] logo_transmission_type=0x%x, logo_id=0x%x, logo_version=0x%x, donwload_data_id=0x%x",
			p->logo_transmission_type, p->logo_id, p->logo_version, p->download_data_id);
	}
}
void display_event_group_descriptor(DESCRIPTOR_STRUCT *pDesc)
{
	char *name = "event_group_descriptor";
	EVT_GROUP_DESCR *p;
	unsigned char group_type;
	int event_count, i;
	if(CHK_VALIDPTR(pDesc)) {
		ALOGD("[SiMgmt]display %s", name);
		p = &pDesc->unDesc.stEGD;
		group_type = p->group_type;
		event_count = (int)p->event_count;
		for(i=0; i < event_count; i++) {
			ALOGD("[SiMgmt]  event(#%d): service_id=0x%x, event_id=0x%x", i, p->service_id[i], p->event_id[i]);
		}
		if (group_type==4 || group_type==5) {
			ALOGD("[SiMgmt]  to/from other netowrk, original_network_id=0x%x, transport_stream_id=0x%x", p->original_network_id, p->transport_stream_id);
		}
	}
}
void display_component_group_descriptor(DESCRIPTOR_STRUCT *pDesc)
{
	char *name = "component_group_descriptor";
	COMPONENT_GROUP_DESCR *p;
	int cnt_group, cnt_ca_unit, cnt_component;
	if(CHK_VALIDPTR(pDesc)) {
		ALOGD("[SiMgmt]display %s", name);
		p = &pDesc->unDesc.stCGD;
		ALOGD("[SiMgmt]component_group_type=%d", p->component_group_type);
		for(cnt_group=0; cnt_group < p->num_of_group; cnt_group++) {
			ALOGD("[SiMgmt]%dth group, group_id=0x%x", cnt_group, (int)(p->component_group_id[cnt_group]));
			for (cnt_ca_unit=0; cnt_ca_unit < p->num_of_CA_unit[cnt_group]; cnt_ca_unit++) {
				ALOGD("[SiMgmt]  CA_unit_id=0x%x", (unsigned int)(p->CA_unit_id[cnt_group]));
				for(cnt_component=0; cnt_component < p->num_of_component[cnt_group][cnt_ca_unit]; cnt_component++) {
					ALOGD("[SiMgmt]  ->component_tag=0x%x", p->component_tag[cnt_group][cnt_ca_unit][cnt_component]);
				}
			}
			if (p->total_bit_rate_flag)
				ALOGD("[SiMgmt]  total_bit_rate=0x%x", p->total_bit_rate[cnt_group]);
		}
	}
}
void display_content_availability_descriptor(DESCRIPTOR_STRUCT *pDesc)
{
	char *name = "content_availability_descriptor";
	CONTENT_AVAILABILITY_DESC *p;
	if (CHK_VALIDPTR(pDesc)) {
		p = &pDesc->unDesc.stCONTAD;
		ALOGD("[SiMgmt]display %s", name);
		ALOGD("[SiMgmt]copy_restriction_mode=0x%x, image_constraint_token=0x%x, retention_mode=0x%x, retention_state=0x%x, encryption_mode=0x%x",
			p->copy_restriction_mode, p->image_constraint_token, p->retention_mode, p->retention_state, p->encryption_mode);
	}
}
void display_access_control_descriptor(DESCRIPTOR_STRUCT *pDesc)
{
	char *name = "access_control_descriptor";
	ACCESS_CONTROL_DESCR *p;
	if (CHK_VALIDPTR(pDesc)) {
		ALOGD("[SiMgmt]display %s", name);
		p = &pDesc->unDesc.stACCD;
		ALOGD("[SiMgmt]CA_system_id=0x%x, PID=0x%x", p->uiCASystemID, p->uiPID);
	}
}
void display_terrestrial_delivery_system_descriptor(DESCRIPTOR_STRUCT *pDesc)
{
	char *name = "terrestrial_delivery_system_descriptor";
	ISDBT_TERRESTRIAL_DS_DESCR *p;
	int cnt;
	if (CHK_VALIDPTR(pDesc)) {
		ALOGD("[SiMgmt]display %s", name);
		p = &pDesc->unDesc.stITDSD;
		ALOGD("[SiMgmt]area_code=0x%x, guard_interval=%d, transmission_mode=%d", p->area_code, p->guard_interval, p->transmission_mode);
		for (cnt=0; cnt < p->freq_ch_num; cnt++) {
			ALOGD("[SiMmgt]frequency=0x%x", p->freq_ch_table[cnt]);
		}
	}
}
void display_partial_reception_descriptor(DESCRIPTOR_STRUCT *pDesc)
{
	char *name = "partial_reception_descriptor";
	PARTIAL_RECEPTION_DESCR *p;
	int cnt;
	if (CHK_VALIDPTR(pDesc)) {
		p = &pDesc->unDesc.stPRECEP_D;
		ALOGD("[SiMgmt]display %s", name);
		for (cnt=0; cnt < p->numServiceId; cnt++) {
			ALOGD("[SiMgmt]service_id=0x%x", p->servicd_id[cnt]);
		}
	}
}
void display_emergency_information_descriptor(DESCRIPTOR_STRUCT *pDesc)
{
	char *name = "emergency_information_descriptor";
	EMERGENCY_INFO_DESCR *p;;
	EMERGENCY_INFO_DATA *pEid;
	EMERGENCY_AREA_CODE *pArea;
	int i, j;
	if (CHK_VALIDPTR(pDesc)) {
		p = &pDesc->unDesc.stEID;
		if(CHK_VALIDPTR(p->pEID)) {
			ALOGD("[SiMgmt]display %s", name);
			pEid = p->pEID;
			for (i=0; i < LOOP_LIMIT && CHK_VALIDPTR(pEid); i++) {
				ALOGD("[SiMgmt]%dth loop of descriptor", i);
				ALOGD("[SiMgmt]service_id=0x%x, start_end_flag=%d, signal_level=%d", pEid->service_id, pEid->start_end_flag, pEid->signal_level);
				pArea = pEid->pAreaCode;
				for(j=0; j < LOOP_LIMIT && CHK_VALIDPTR(pArea); j++) {
					ALOGD("[SiMgmt]area_code=0x%x", pArea->area_code);
					pArea = pArea->pNext;
				}
				pEid = pEid->pNext;
			}
		}
	}
}
void display_data_component_descriptor(DESCRIPTOR_STRUCT *pDesc)
{
	char *name = "data_component_descriptor";
	DATA_COMPONENT_DESCR *p;
	if (CHK_VALIDPTR(pDesc)) {
		p = &pDesc->unDesc.stDCOMP_D;
		ALOGD("[SiMgmt]display %s", name);
		if( p->data_component_id == 0x0008 )
		{
			ALOGD("[SiMgmt] data_component_id=0x%X, DMF=%d, Timing=%d", p->data_component_id, (int)(p->DMF), (int)(p->Timing));
		}
		else
		{
			unsigned char szString[256*4];
			unsigned char *pBuf = szString;
			int i;
			if( p->a_data_comp_info_length > 0 )
			{
				for(i=0;i<p->a_data_comp_info_length;i++)
				{
					pBuf += sprintf(pBuf,"%02X ",p->a_data_comp_info[i]);
				}
				*(pBuf-1)=0; // remove last 'space'
			}
			else
			{
				szString[0]=0;
			}
			ALOGD("[SiMgmt] data_component_id=0x%X, additional_data_component_info[%d]=[%s]", p->data_component_id, p->a_data_comp_info_length, szString);
		}
	}
}
void display_system_management_descriptor(DESCRIPTOR_STRUCT *pDesc)
{
	char *name = "system_management_descriptor";
}

void TSPARSE_ISDBT_SiMgmt_TEST_DisplayDescriptor (DESCRIPTOR_STRUCT *pDesc)
{
	int cnt;
	DESCRIPTOR_STRUCT *pDescTemp;
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) return;
	pDescTemp = pDesc;
	for (cnt=0; cnt < LOOP_LIMIT && CHK_VALIDPTR(pDescTemp); cnt++) {
		switch (pDescTemp->enDescriptorID) {
			case CA_DESCR_ID:
				display_conditional_access_descriptor(pDescTemp);
				break;
			case NETWORK_NAME_DESCR_ID:
				display_network_name_descriptor(pDescTemp);
				break;
			case SERVICE_LIST_DESCR_ID:
				display_service_list_descriptor(pDescTemp);
				break;
			case STUFFING_DESCR_ID:
				display_stuffing_descriptor(pDescTemp);
				break;
			case SERVICE_DESCR_ID:
				display_service_descriptor(pDescTemp);
				break;
			case LINKAGE_DESCR_ID:
				display_linkage_descriptor(pDescTemp);
				break;
			case SHRT_EVT_DESCR_ID:
				display_short_event_descriptor(pDescTemp);
				break;
			case EXTN_EVT_DESCR_ID:
				display_exented_event_descriptor(pDescTemp);
				break;
			case COMPONENT_DESCR_ID:
				display_component_descriptor(pDescTemp);
				break;
			case STREAM_IDENT_DESCR_ID:
				display_stream_identifier_descriptor(pDescTemp);
				break;
			case CONTENT_DESCR_ID:
				display_content_descriptor(pDescTemp);
				break;
			case LOCAL_TIME_OFFSET_DESCR_ID:
				display_local_time_offset_descriptor(pDescTemp);
				break;
			case DIGITALCOPY_CONTROL_ID:
				display_digital_copy_control_descriptor(pDescTemp);
				break;
			case AUDIO_COMPONENT_DESCR_ID:
				display_audio_component_descriptor(pDescTemp);
				break;
			case DATA_CONTENT_DESCR_ID:
				display_data_contents_descriptor(pDescTemp);
				break;
			case 0xc8:
				//display_video_decode_control_descriptor(pDescTemp);
				ALOGE("In TSPARSE_ISDBT_SiMgmt_TEST_DisplayDescriptor, video_decode_control_descriptor is not handled");
				break;
			case 0xc9:
				//display_download_content_descriptor(pDescTemp);
				ALOGE("In TSPARSE_ISDBT_SiMgmt_TEST_DisplayDescriptor, download_content_descriptor is not handled");
				break;
			case CA_CONTRACT_INFO_DESCR_ID:
				//display_CA_contract_info_descriptor(pDescTemp);
				ALOGE("In TSPARSE_ISDBT_SiMgmt_TEST_DisplayDescriptor, CA_contract_info_descriptor is not handled");
				break;
			case CA_SERVICE_DESCR_ID:
				display_CA_service_descriptor(pDescTemp);
				break;
			case TS_INFO_DESCR_ID:
				display_TS_information_descriptor(pDescTemp);
				break;
			case EXT_BROADCASTER_DESCR_ID:
				display_extended_broadcaster_descriptor(pDescTemp);
				break;
			case LOGO_TRANSMISSION_DESCR_ID:
				display_logo_transmission_descriptor(pDescTemp);
				break;
			case 0xd5:
				//display_series_descriptor(pDescTemp);
				ALOGE("In TSPARSE_ISDBT_SiMgmt_TEST_DisplayDescriptor, series_descriptor is not handled");
				break;
			case EVT_GROUP_DESCR_ID:
				display_event_group_descriptor(pDescTemp);
				break;
			case 0xd7:
				//display_SI_transmission_parameter_descriptor(pDescTemp);
				ALOGE("In TSPARSE_ISDBT_SiMgmt_TEST_DisplayDescriptor, SI_transmission_parameter_descriptor is not handled");
				break;
			case COMPONENT_GROUP_DESCR_ID:
				display_component_group_descriptor(pDescTemp);
				break;
			case CONTENT_AVAILABILITY_ID:
				display_content_availability_descriptor(pDescTemp);
				break;
			case ACCESS_CONTROL_DESCR_ID:
				display_access_control_descriptor(pDescTemp);
				break;
			case ISDBT_TERRESTRIAL_DS_DESCR_ID:
				display_terrestrial_delivery_system_descriptor(pDescTemp);
				break;
			case PARTIAL_RECEPTION_DESCR_ID:
				display_partial_reception_descriptor(pDescTemp);
				break;
			case EMERGENCY_INFORMATION_DESCR_ID:
				display_emergency_information_descriptor(pDescTemp);
				break;
			case DATA_COMPONENT_DESCR_ID:
				display_data_component_descriptor(pDescTemp);
				break;
			case SYSTEM_MANAGEMENT_DESCR_ID:
				//display_system_management_descriptor(pDescTemp);
				ALOGE("In TSPARSE_ISDBT_SiMgmt_TEST_DisplayDescriptor, system_management_descriptor is not handled");
				break;
			default:
				ALOGE("In TSPARSE_ISDBT_SiMgmt_TEST_DisplayDescriptor, this descriptor(0x%x) is not handled", pDescTemp->enDescriptorID);
				break;
		}

		pDescTemp = pDescTemp->pstLinkedDescriptor;
	}
}

void TSPARSE_ISDBT_SiMgmt_TEST_DisplaySecLoopData (SI_MGMT_SECTIONLOOP *pSecLoop)
{
	MPEG_TABLE_IDS table_id;
	unsigned char section_number;
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) return;
	if (!CHK_VALIDPTR(pSecLoop)) {
		ALOGE("[SiMgmtTest] SectionLoop is not valid");
		return;
	}
	if (!CHK_VALIDPTR(pSecLoop->pParentSection)) {
		ALOGE("[SiMgmtTest] SectionLoop doens't have a link to a parent section");
		return;
	}
	if (!CHK_VALIDPTR(pSecLoop->pParentSection->pParentTable)) {
		ALOGE("[SiMgmtTest] parent section of this section loop doesn't have a link to a parent table");
		return;
	}
	section_number = pSecLoop->pParentSection->section_number;
	table_id = pSecLoop->pParentSection->pParentTable->table_id;

	if (TSPARSE_ISDBT_SiMgmt_Table_IsPAT(table_id)) {
		ALOGE("[SiMgmtTest]PAT.section(%d).program_number=0x%x", section_number, pSecLoop->unSectionLoopData.pat_loop.program_number);
		ALOGE("[SiMgmtTest]PAT.section(%d).program_map_pid=0x%x", section_number, pSecLoop->unSectionLoopData.pat_loop.program_map_pid);
	} else if (TSPARSE_ISDBT_SiMgmt_Table_IsPMT(table_id)) {
		ALOGE("[SiMgmtTest]PMT.section(%d).stream_type=0x%x", section_number, pSecLoop->unSectionLoopData.pmt_loop.stream_type);
		ALOGE("[SiMgmtTest]PMT.section(%d).elementary_PID=0x%x", section_number, pSecLoop->unSectionLoopData.pmt_loop.elementary_PID);
	} else if (TSPARSE_ISDBT_SiMgmt_Table_IsBIT(table_id)) {
		ALOGE("[SiMgmtTest]BIT.section(%d).boradcaster_id=0x%x", section_number, pSecLoop->unSectionLoopData.bit_loop.broadcaster_id);
	} else if (TSPARSE_ISDBT_SiMgmt_Table_IsNIT(table_id)) {
		ALOGE("[SiMgmtTest]NIT.section(%d).transport_stream_id=0x%x", section_number, pSecLoop->unSectionLoopData.nit_loop.transport_stream_id);
		ALOGE("[SiMgmtTest]NIT.section(%d).original_network_id=0x%x", section_number, pSecLoop->unSectionLoopData.nit_loop.original_network_id);
	} else if (TSPARSE_ISDBT_SiMgmt_Table_IsSDT(table_id)) {
		ALOGE("[SiMgmtTest]SDT.section(%d).service_id=0x%x", section_number, pSecLoop->unSectionLoopData.sdt_loop.service_id);
		ALOGE("[SiMgmtTest]SDT.section(%d).H_EIT_flag=0x%x", section_number, pSecLoop->unSectionLoopData.sdt_loop.H_EIT_flag);
		ALOGE("[SiMgmtTest]SDT.section(%d).M_EIT_flag=0x%x", section_number, pSecLoop->unSectionLoopData.sdt_loop.M_EIT_flag);
		ALOGE("[SiMgmtTest]SDT.section(%d).L_EIT_flag=0x%x", section_number, pSecLoop->unSectionLoopData.sdt_loop.L_EIT_flag);
	} else if (TSPARSE_ISDBT_SiMgmt_Table_IsEIT(table_id)) {
		ALOGE("[SiMgmtTest]EIT.section(%d).event_id=0x%x", section_number, pSecLoop->unSectionLoopData.eit_loop.event_id);
		ALOGE("[SiMgmtTest]EIT.section(%d).stStartTime.uiMJD=0x%x", section_number, pSecLoop->unSectionLoopData.eit_loop.stStartTime.uiMJD);
		ALOGE("[SiMgmtTest]EIT.section(%d).stStartTime.stTime.ucHour=0x%x", section_number, pSecLoop->unSectionLoopData.eit_loop.stStartTime.stTime.ucHour);
		ALOGE("[SiMgmtTest]EIT.section(%d).stStartTime.stTime.ucMinute=0x%x", section_number, pSecLoop->unSectionLoopData.eit_loop.stStartTime.stTime.ucMinute);
		ALOGE("[SiMgmtTest]EIT.section(%d).stStartTime.stTime.ucSecond=0x%x", section_number, pSecLoop->unSectionLoopData.eit_loop.stStartTime.stTime.ucSecond);
		ALOGE("[SiMgmtTest]EIT.section(%d).stDuration.ucHour=0x%x", section_number, pSecLoop->unSectionLoopData.eit_loop.stDuration.ucHour);
		ALOGE("[SiMgmtTest]EIT.section(%d).stDuration.ucMinute=0x%x", section_number, pSecLoop->unSectionLoopData.eit_loop.stDuration.ucMinute);
		ALOGE("[SiMgmtTest]EIT.section(%d).stDuration.ucSecond=0x%x", section_number, pSecLoop->unSectionLoopData.eit_loop.stDuration.ucSecond);
	} else if (TSPARSE_ISDBT_SiMgmt_Table_IsTOT(table_id)) {
		ALOGE("[SiMgmtTest]TOTsection(%d) nothing", section_number);
	} else if (TSPARSE_ISDBT_SiMgmt_Table_IsCDT(table_id)) {
		ALOGE("[SiMgmtTest]CDT.section(%d) nothing", section_number);
	} else if (TSPARSE_ISDBT_SiMgmt_Table_IsCAT(table_id)) {
		ALOGE("[SiMgmtTest]CAT.section(%d) nothing", section_number);
	} else {
		ALOGE("[SiMgmtTest]table_id(0x%x) of section (section_number=%d) of this sectin loop is not useful", table_id, section_number);
	}
}

void TSPARSE_ISDBT_SiMgmt_TEST_DisplaySection(SI_MGMT_SECTION *pSection)
{
	MPEG_TABLE_IDS table_id;
	unsigned char section_number;
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) return;
	if (!CHK_VALIDPTR(pSection)) {
		ALOGE("[SiMgmtTest] Section is not valid");
		return;
	}

	ALOGE("[SiMgmtTest]section(%d)'s version=0x%x, last_sec=%d", pSection->section_number, pSection->version_number, pSection->last_section_number);

	if(CHK_VALIDPTR(pSection->pParentTable)) table_id = pSection->pParentTable->table_id;
	else {
		ALOGE("[SiMgmtTest] section doens' have a link to parent table. FAIL");
		return;
	}
	section_number = pSection->section_number;
	if (TSPARSE_ISDBT_SiMgmt_Table_IsPAT(table_id)) {
		ALOGE("[SiMgmtTest]PAT.section(%d) ext nothing", section_number);
	} else if (TSPARSE_ISDBT_SiMgmt_Table_IsPMT(table_id)) {
		ALOGE("[SiMgmtTest]PMT.section(%d).PCR_PID=0x%x", section_number, pSection->unSectionExt.pmt_sec_ext.PCR_PID);
	} else if (TSPARSE_ISDBT_SiMgmt_Table_IsBIT(table_id)) {
		ALOGE("[SiMgmtTest]BIT.section(%d) ext nothing", section_number);
	} else if (TSPARSE_ISDBT_SiMgmt_Table_IsNIT(table_id)) {
		ALOGE("[SiMgmtTest]NIT.section(%d) ext nothing", section_number);
	} else if (TSPARSE_ISDBT_SiMgmt_Table_IsSDT(table_id)) {
		ALOGE("[SiMgmtTest]SDT.section(%d) ext nothing", section_number);
	} else if (TSPARSE_ISDBT_SiMgmt_Table_IsEIT(table_id)) {
		ALOGE("[SiMgmtTest]EIT.section(%d).segment_last_section_number=0x%x", section_number, pSection->unSectionExt.eit_sec_ext.segment_last_section_number);
	} else if (TSPARSE_ISDBT_SiMgmt_Table_IsTOT(table_id)) {
		ALOGE("[SiMgmtTest]TOTsection(%d).uiMJD=0x%x ", section_number, pSection->unSectionExt.tot_sec_ext.stDateTime.uiMJD);
		ALOGE("[SiMgmtTest]TOTsection(%d).hour=0x%x ", section_number, pSection->unSectionExt.tot_sec_ext.stDateTime.stTime.ucHour);
		ALOGE("[SiMgmtTest]TOTsection(%d).min=0x%x ", section_number, pSection->unSectionExt.tot_sec_ext.stDateTime.stTime.ucMinute);
		ALOGE("[SiMgmtTest]TOTsection(%d).sec=0x%x ", section_number, pSection->unSectionExt.tot_sec_ext.stDateTime.stTime.ucSecond);
	} else if (TSPARSE_ISDBT_SiMgmt_Table_IsCDT(table_id)) {
		ALOGE("[SiMgmtTest]CDT.section(%d).data_type=0x%x", section_number, pSection->unSectionExt.cdt_sec_ext.data_type);
	} else if (TSPARSE_ISDBT_SiMgmt_Table_IsCAT(table_id)) {
		ALOGE("[SiMgmtTest]CAT.section(%d) ext nothing", section_number);
	} else {
		ALOGE("[SiMgmtTest]table_id(0x%x) of section (section_number=%d) is not useful", table_id, pSection->section_number);
	}
}

void TSPARSE_ISDBT_SiMgmt_TEST_DisplayTableExt (SI_MGMT_TABLE *pTable)
{
	MPEG_TABLE_IDS table_id;
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) return;
	if (!CHK_VALIDPTR(pTable)) {
		ALOGE("[SiMgmtTest] Table is not valid");
		return;
	}

	table_id = pTable->table_id;
	if (TSPARSE_ISDBT_SiMgmt_Table_IsPAT(table_id)) {
		ALOGE("[SiMgmtTest]PAT.transport_stream_id=0x%x", pTable->unTableExt.pat_ext.transport_stream_id);
	} else if (TSPARSE_ISDBT_SiMgmt_Table_IsPMT(table_id)) {
		ALOGE("[SiMgmtTest]PMT.program_number=0x%x", pTable->unTableExt.pmt_ext.program_number);
	} else if (TSPARSE_ISDBT_SiMgmt_Table_IsBIT(table_id)) {
		ALOGE("[SiMgmtTest]BIT.original_network_id=0x%x", pTable->unTableExt.bit_ext.original_network_id);
	} else if (TSPARSE_ISDBT_SiMgmt_Table_IsNIT(table_id)) {
		ALOGE("[SiMgmtTest]NIT.network_id=0x%x", pTable->unTableExt.nit_ext.network_id);
	} else if (TSPARSE_ISDBT_SiMgmt_Table_IsSDT(table_id)) {
		ALOGE("[SiMgmtTest]SDT.transport_stream_id=0x%x", pTable->unTableExt.sdt_ext.transport_stream_id);
		ALOGE("[SiMgmtTest]SDT.original_network_id=0x%x", pTable->unTableExt.sdt_ext.original_network_id);
	} else if (TSPARSE_ISDBT_SiMgmt_Table_IsEIT(table_id)) {
		ALOGE("[SiMgmtTest]EIT.service_id=0x%x", pTable->unTableExt.eit_ext.service_id);
		ALOGE("[SiMgmtTest]EIT.transport_stream_id=0x%x", pTable->unTableExt.eit_ext.transport_stream_id);
		ALOGE("[SiMgmtTest]EIT.original_network_id=0x%x", pTable->unTableExt.eit_ext.original_network_id);
		ALOGE("[SiMgmtTest]EIT.last_table_id=0x%x", pTable->unTableExt.eit_ext.last_table_id);
	} else if (TSPARSE_ISDBT_SiMgmt_Table_IsTOT(table_id)) {
		ALOGE("[SiMgmtTest]TOT ext. nothing");
	} else if (TSPARSE_ISDBT_SiMgmt_Table_IsCDT(table_id)) {
		ALOGE("[SiMgmtTest]CDT.original_network_id=0x%x", pTable->unTableExt.cdt_ext.original_network_id);
		ALOGE("[SiMgmtTest]CDT.download_data_id=0x%x", pTable->unTableExt.cdt_ext.download_data_id);
	} else if (TSPARSE_ISDBT_SiMgmt_Table_IsCAT(table_id)) {
		ALOGE("[SiMgmtTest]CAT ext. nothing");
	} else {
		ALOGE("[SiMgmtTest] table_id=0x%x is not useful", table_id);
	}
}

void TSPARSE_ISDBT_SiMgmt_TEST_DisplayTable(MPEG_TABLE_IDS table_id, SI_MGMT_TABLE_EXTENSION *pTableExt)
{
	SI_MGMT_TABLE_ROOT *pTableRoot = (SI_MGMT_TABLE_ROOT*)NULL;
	SI_MGMT_TABLE *pTable = (SI_MGMT_TABLE*)NULL;
	SI_MGMT_SECTION *pSection = (SI_MGMT_SECTION*)NULL;
	SI_MGMT_SECTIONLOOP *pSecLoop = (SI_MGMT_SECTIONLOOP*)NULL;
	int status;
	int table_count, section_count, loop_count;
	int section_number, last_section_number;
	int fDisplayAllSubtable;

	ALOGE("***** TSPARSE_ISDBT_SiMgmt_TEST_DisplayTable (0x%x) *****", table_id);
	LOCK_START();
	if(tsparse_isdbt_simgmt_state_get() != TSPARSE_SIMGMT_STATE_RUN) {
		LOCK_STOP();
		return TSPARSE_SIMGMT_INVALIDSTATE;
	}
	status = tsparse_isdbt_simgmt_GetTableRoot (&pTableRoot, table_id);
	if (status != TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pTableRoot)) {
		ALOGE("[SiMgmtTest]table(%d) doesn't have a root pointer to table", table_id);
		LOCK_STOP();
		return;
	}

	if (CHK_VALIDPTR(pTableExt)) fDisplayAllSubtable = 0;
	else fDisplayAllSubtable = 1;

	if (fDisplayAllSubtable) status = tsparse_isdbt_simgmt_GetTable (&pTable, pTableRoot);
	else status = tsparse_isdbt_simgmt_FindTable(&pTable, table_id, pTableExt, pTableRoot);
	if (status != TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pTable)) {
		ALOGE("[SiMgmtTest]there is no table(%d) in list", table_id);
		LOCK_STOP();
		return;
	}

	for (table_count=0; table_count < LOOP_LIMIT && CHK_VALIDPTR(pTable); table_count++) {
		ALOGE("[SiMgmtTest]display list of tables #%d -----", table_count);
		TSPARSE_ISDBT_SiMgmt_TEST_DisplayTableExt(pTable);
		status = tsparse_isdbt_simgmt_Table_GetSection(&pSection, eSECTION_CURR, pTable);
		if (status != TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pSection)) {
			ALOGE("[SiMgmtTest]this table have no section");
			continue;
		} else {
			last_section_number = pSection->last_section_number;
		}
		for (section_number=0; section_number <= last_section_number; section_number++) {
			status = tsparse_isdbt_simgmt_Table_FindSection (&pSection, eSECTION_CURR, section_number, pTable);
			if (status != TSPARSE_SIMGMT_OK || !CHK_VALIDPTR(pSection)) {
				ALOGE("[SiMgmtTest]there is no section(%d) in %dth table", section_number, table_count);
				continue;
			}
			ALOGE("[SiMgmtTest]display section(section_number=%d) of %dth table -----", section_number, table_count);
			TSPARSE_ISDBT_SiMgmt_TEST_DisplaySection(pSection);
			if (CHK_VALIDPTR(pSection->pGlobalDesc)) {
				ALOGE("[SiMgmtTest]global descriptors of this section (%d) is", section_number);
				TSPARSE_ISDBT_SiMgmt_TEST_DisplayDescriptor(pSection->pGlobalDesc);
			} else {
				ALOGE("[SiMgmtTest]no global descriptor in this section(%d)", section_number);
			}
			status = tsparse_isdbt_simgmt_Section_GetLoop(&pSecLoop, pSection);
			if (status == TSPARSE_SIMGMT_OK && CHK_VALIDPTR(pSecLoop)) {
				for (loop_count=0; loop_count < LOOP_LIMIT && CHK_VALIDPTR(pSecLoop); loop_count++) {
					ALOGE("[SiMgmtTest]display #%dth section loop of section_number=%d -----", loop_count, section_number);
					TSPARSE_ISDBT_SiMgmt_TEST_DisplaySecLoopData(pSecLoop);
					TSPARSE_ISDBT_SiMgmt_TEST_DisplayDescriptor(pSecLoop->pDesc);
					pSecLoop = pSecLoop->pNext;
				}
			} else {
				ALOGE("[SiMgmtTest]section (%d) of %dth table doesn't have a section loop", section_number, table_count);
			}
			pSection = pSection->pNext;
		}
		if (fDisplayAllSubtable) pTable = pTable->pNext;
		else break;
	}
	LOCK_STOP();
}

