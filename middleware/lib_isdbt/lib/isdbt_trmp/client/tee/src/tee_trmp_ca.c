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
#ifndef __MODULE__
#define __MODULE__ "tee_trmp_ca.c"
#endif

#include <utils/Log.h>
#include <stdio.h>
#include <stdlib.h>

#include "tcc_trmp_type.h"
#include "calibInterface.h"
#include "ta_protocol.h"

#define DEBUG_ISDBT_CAS
#ifdef DEBUG_ISDBT_CAS
#undef INFO_PRINTF
#define INFO_PRINTF // ALOGI
#undef DBG_PRINTF
#define DBG_PRINTF ALOGD
#undef ERR_PRINTF
#define ERR_PRINTF ALOGE
#else
#undef INFO_PRINTF
#deifne INFO_PRINTF
#undef DBG_PRINTF
#define DBG_PRINTF
#undef ERR_PRINTF
#define ERR_PRINTF ALOGE
#endif

/****************************************************************************
DEFINITION OF LOCAL VARIABLES
****************************************************************************/
static void *m_pContext;
static TEEC_UUID g_sUUID = TA_UUID;
/****************************************************************************
DEFINITION OF FUNCTIONS
****************************************************************************/
#define LOGMSG_TEE_TIME
#if defined(LOGMSG_TEE_TIME)
#include <time.h>

static uint64_t _u64_timeget(void)
{
	struct timespec tspec;
	uint64_t retval;

	clock_gettime(CLOCK_MONOTONIC, &tspec);

	retval = (uint64_t)(tspec.tv_sec * 1000 * 1000);
	retval += tspec.tv_nsec / 1000;

	return retval;
}

static uint32_t _diff_ms(uint32_t StartStamp, uint32_t StopStamp)
{
	uint64_t Delta;

	// time stamps are in microseconds
	Delta = StopStamp - StartStamp;

	// round to milliseconds
	if (Delta < UINT64_MAX - 500)
		Delta += 500;

	Delta = Delta / 1000;

	// cap at.. 49 days
	if (Delta > UINT32_MAX)
		return UINT32_MAX;

	return (uint32_t)Delta;
}
#endif

/*----------------------------------------------------------------------------
 * CmdInitTRMP
 */
int CmdInitTRMP(void)
{
	struct CALibParams sParams;
	TEEC_Result uRes;
	uint32_t res;
#if defined(LOGMSG_TEE_TIME)
	uint64_t _ms_start;
	uint64_t _ms_end;
	uint64_t _ms_diff;
#endif

	INFO_PRINTF("%s()", __func__);
#if defined(LOGMSG_TEE_TIME)
	_ms_start = _u64_timeget();
#endif
	CALib_InitializeParam(&sParams);

	uRes = CALib_OpenTA(&g_sUUID, NULL, &m_pContext);
	if (uRes != TEEC_SUCCESS) {
		ERR_PRINTF("Failed CALib_OpenTA 0x%x", uRes);
		return TCC_TRMP_ERROR_TEE_OPEN_SESSION;
	}
#if defined(LOGMSG_TEE_TIME)
	_ms_end = _u64_timeget();
	_ms_diff = _diff_ms(_ms_start, _ms_end);
	DBG_PRINTF("CALib_OpenTA took %llu ms\n", _ms_diff);
#endif

	sParams.m_apParams[0].CALibValue.m_uA = res;
	sParams.m_apParams[0].m_uType = CALIB_PARAM_VALUE_OUT;

	uRes = CALib_ExecuteCommand(m_pContext, &sParams, CMD_INIT_TRMP);

	if (uRes != TEEC_SUCCESS) {
		ERR_PRINTF("Failed CALib_ExecuteCommand(%d)\n", uRes);
		return TCC_TRMP_ERROR_TEE;
	}

	return sParams.m_apParams[0].CALibValue.m_uA;
}

/*----------------------------------------------------------------------------
 * CmdDeinitTRMP
 */
int CmdDeinitTRMP(void)
{
	struct CALibParams sParams;
	TEEC_Result uRes;
#if defined(LOGMSG_TEE_TIME)
	uint64_t _ms_start;
	uint64_t _ms_end;
	uint64_t _ms_diff;
#endif

	INFO_PRINTF("%s()", __func__);

#if defined(LOGMSG_TEE_TIME)
	_ms_start = _u64_timeget();
#endif
	CALib_InitializeParam(&sParams);

	uRes = CALib_ExecuteCommand(m_pContext, &sParams, CMD_DEINIT_TRMP);

	if (uRes != TEEC_SUCCESS) {
		ERR_PRINTF("Failed CALib_ExecuteCommand(%d)\n", uRes);
	}

	CALib_CloseTA(m_pContext);

#if defined(LOGMSG_TEE_TIME)
	_ms_end = _u64_timeget();
	_ms_diff = _diff_ms(_ms_start, _ms_end);
	DBG_PRINTF("CALib_CloseTA took %llu ms", _ms_diff);
#endif

	return TCC_TRMP_SUCCESS;
}

/*----------------------------------------------------------------------------
 * CmdResetTRMP
 */
int CmdResetTRMP(void)
{
	struct CALibParams sParams;
	TEEC_Result uRes;

	INFO_PRINTF("%s()", __func__);

	CALib_InitializeParam(&sParams);

	uRes = CALib_ExecuteCommand(m_pContext, &sParams, CMD_RESET_TRMP);

	if (uRes != TEEC_SUCCESS) {
		ERR_PRINTF("Failed CALib_ExecuteCommand(%d)\n", uRes);
	}

	return TCC_TRMP_SUCCESS;
}

/*----------------------------------------------------------------------------
 * CmdSetEMM
 */
int CmdSetEMM(unsigned short usNetworkID, unsigned char *pSectionData, int nSectionDataSize)
{
	struct CALibParams sParams;
	TEEC_Result uRes;

	INFO_PRINTF("%s()", __func__);

	CALib_InitializeParam(&sParams);

	sParams.m_apParams[0].CALibValue.m_uA = usNetworkID;
	sParams.m_apParams[0].CALibValue.m_uB = sizeof(unsigned short);
	sParams.m_apParams[0].m_uType = CALIB_PARAM_VALUE_INOUT;

	sParams.m_apParams[1].CALibMemRef.m_pBuffer = (void *)pSectionData;
	sParams.m_apParams[1].CALibMemRef.m_uSize = nSectionDataSize;
	sParams.m_apParams[1].m_uType = CALIB_PARAM_BUF_INOUT;

	uRes = CALib_ExecuteCommand(m_pContext, &sParams, CMD_SET_EMM);

	if (uRes != TEEC_SUCCESS) {
		ERR_PRINTF("Failed CALib_ExecuteCommand(%d)\n", uRes);
		return TCC_TRMP_ERROR_TEE;
	}

	return sParams.m_apParams[0].CALibValue.m_uA;
}

/*----------------------------------------------------------------------------
 * CmdSetECM
 */
int CmdSetECM(
	unsigned short usNetworkID, unsigned char *pSectionData, int nSectionDataSize,
	unsigned char *pOddKey, unsigned char *pEvenKey)
{
	struct CALibParams sParams;
	TEEC_Result uRes;

	INFO_PRINTF("%s()", __func__);

	if (m_pContext == NULL) {
		ERR_PRINTF("CmdSetECM is NULL\n");
		return TCC_TRMP_ERROR_TEE;
	}

	CALib_InitializeParam(&sParams);

	sParams.m_apParams[0].CALibValue.m_uA = usNetworkID;
	sParams.m_apParams[0].CALibValue.m_uB = sizeof(unsigned short);
	sParams.m_apParams[0].m_uType = CALIB_PARAM_VALUE_INOUT;

	sParams.m_apParams[1].CALibMemRef.m_pBuffer = (void *)pSectionData;
	sParams.m_apParams[1].CALibMemRef.m_uSize = nSectionDataSize;
	sParams.m_apParams[1].m_uType = CALIB_PARAM_BUF_INOUT;

	sParams.m_apParams[2].CALibMemRef.m_pBuffer = (void *)pOddKey;
	sParams.m_apParams[2].CALibMemRef.m_uSize = TCC_TRMP_SCRAMBLE_KEY_LENGTH;
	sParams.m_apParams[2].m_uType = CALIB_PARAM_BUF_OUT;

	sParams.m_apParams[3].CALibMemRef.m_pBuffer = (void *)pEvenKey;
	sParams.m_apParams[3].CALibMemRef.m_uSize = TCC_TRMP_SCRAMBLE_KEY_LENGTH;
	sParams.m_apParams[3].m_uType = CALIB_PARAM_BUF_OUT;

	uRes = CALib_ExecuteCommand(m_pContext, &sParams, CMD_SET_ECM);

	if (uRes != TEEC_SUCCESS) {
		ERR_PRINTF("Failed CALib_ExecuteCommand(%d)\n", uRes);
		return TCC_TRMP_ERROR_TEE;
	}

	return sParams.m_apParams[0].CALibValue.m_uA;
}

/*----------------------------------------------------------------------------
 * CmdGetMulti2SystemKey
 */

int CmdGetMulti2SystemKey(unsigned char *pMulti2SystemKey, unsigned char *pMulti2CBCDefaultValue)
{
	struct CALibParams sParams;
	TEEC_Result uRes;

	INFO_PRINTF("%s()", __func__);

	CALib_InitializeParam(&sParams);

	sParams.m_apParams[0].CALibMemRef.m_pBuffer = (void *)pMulti2SystemKey;
	sParams.m_apParams[0].CALibMemRef.m_uSize = TCC_TRMP_MULTI2_SYSTEM_KEY_LENGTH;
	sParams.m_apParams[0].m_uType = CALIB_PARAM_BUF_OUT;

	sParams.m_apParams[1].CALibMemRef.m_pBuffer = (void *)pMulti2CBCDefaultValue;
	sParams.m_apParams[1].CALibMemRef.m_uSize = TCC_TRMP_MULTI2_CBC_DEFAULT_VALUE_LENGTH;
	sParams.m_apParams[1].m_uType = CALIB_PARAM_BUF_OUT;

	uRes = CALib_ExecuteCommand(m_pContext, &sParams, CMD_GET_MULTI2_SYSTEM_KEY);
	if (uRes != TEEC_SUCCESS) {
		ERR_PRINTF("Failed CALib_ExecuteCommand(%d)\n", uRes);
		return TCC_TRMP_ERROR_TEE;
	}

	return TCC_TRMP_SUCCESS;
}

/*----------------------------------------------------------------------------
 * CmdGetIDInfoSize
 */
int CmdGetIDInfoSize(int *pIDInfoSize)
{
	struct CALibParams sParams;
	TEEC_Result uRes;

	INFO_PRINTF("%s()", __func__);

	CALib_InitializeParam(&sParams);

	sParams.m_apParams[0].CALibMemRef.m_pBuffer = (void *)pIDInfoSize;
	sParams.m_apParams[0].CALibMemRef.m_uSize = sizeof(int);
	sParams.m_apParams[0].m_uType = CALIB_PARAM_BUF_OUT;

	uRes = CALib_ExecuteCommand(m_pContext, &sParams, CMD_GET_ID_INFO_SIZE);
	if (uRes != TEEC_SUCCESS) {
		ERR_PRINTF("Failed CALib_ExecuteCommand(%d)\n", uRes);
		return TCC_TRMP_ERROR_TEE;
	}

	return TCC_TRMP_SUCCESS;
}

/*----------------------------------------------------------------------------
 * CmdGetIDInfo
 */
int CmdGetIDInfo(unsigned char *pIDInfo, int iIDInfoSize)
{
	struct CALibParams sParams;
	TEEC_Result uRes;

	INFO_PRINTF("%s()", __func__);

	CALib_InitializeParam(&sParams);

	sParams.m_apParams[0].CALibMemRef.m_pBuffer = (void *)pIDInfo;
	sParams.m_apParams[0].CALibMemRef.m_uSize = iIDInfoSize;
	sParams.m_apParams[0].m_uType = CALIB_PARAM_BUF_OUT;

	uRes = CALib_ExecuteCommand(m_pContext, &sParams, CMD_GET_ID_INFO);
	if (uRes != TEEC_SUCCESS) {
		ERR_PRINTF("Failed CALib_ExecuteCommand(%d)\n", uRes);
		return TCC_TRMP_ERROR_TEE;
	}

	return TCC_TRMP_SUCCESS;
}

/* end of file tee_trmp_ca.c */
