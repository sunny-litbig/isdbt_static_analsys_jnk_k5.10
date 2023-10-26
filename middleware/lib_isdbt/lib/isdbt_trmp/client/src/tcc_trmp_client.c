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
#define LOG_TAG "TCC_TRMP_CLIENT"
#include <utils/Log.h>
#include <pthread.h>
#include <fcntl.h>

#include "tcc_trmp_client.h"

/****************************************************************************
DEFINITION
****************************************************************************/
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
#define INFO_PRINTF
#undef DBG_PRINTF
#define DBG_PRINTF
#undef ERR_PRINTF
#define ERR_PRINTF ALOGE
#endif

/****************************************************************************
DEFINITION OF LOCAL VARIABLES
****************************************************************************/

/****************************************************************************
DEFINITION OF FUNCTIONS
****************************************************************************/
#if defined(__SECURITEE__)
#include "ta_protocol.h"

int tcc_trmp_client_init(void)
{
	int iRet;

	iRet = CmdInitTRMP();
	if (iRet != TCC_TRMP_SUCCESS) {
		ERR_PRINTF("[%s] Failed CmdInitTRMP(%d)\n", __func__, iRet);

		return TCC_TRMP_ERROR;
	}

	return iRet;
}

void tcc_trmp_client_deinit(void)
{
	int iRet;

	iRet = CmdDeinitTRMP();
	if (iRet != TCC_TRMP_SUCCESS) {
		ERR_PRINTF("[%s] Failed CmdDeinitTRMP(%d)\n", __func__, iRet);
	}

	return;
}

int tcc_trmp_client_reset(void)
{
	int iRet;

	iRet = CmdResetTRMP();
	if (iRet != TCC_TRMP_SUCCESS) {
		ERR_PRINTF("[%s] Failed CmdResetTRMP(%d)\n", __func__, iRet);
	}

	return iRet;
}

#if 0   // sunny : not use
void tcc_trmp_client_setDeviceKeyUpdateFunction(updateDeviceKey func)
{
	ERR_PRINTF("[%s] Failed tcc_trmp_client_setDeviceKeyUpdateFunction\n", __func__);

	return;
}
#endif

int tcc_trmp_client_setEMM(
	unsigned short usNetworkID, unsigned char *pSectionData, int nSectionDataSize)
{
	int iRet;

	iRet = CmdSetEMM(usNetworkID, pSectionData, nSectionDataSize);
	if (iRet != TCC_TRMP_SUCCESS) {
		ERR_PRINTF("[%s] Failed CmdSetEMM(%d)\n", __func__, iRet);
	}

	return TCC_TRMP_SUCCESS;
}

int tcc_trmp_client_setECM(
	unsigned short usNetworkID, unsigned char *pSectionData, int nSectionDataSize,
	unsigned char *pOddKey, unsigned char *pEvenKey)
{
	int iRet;

	iRet = CmdSetECM(usNetworkID, pSectionData, nSectionDataSize, pOddKey, pEvenKey);
	if (iRet != TCC_TRMP_SUCCESS) {
		ERR_PRINTF("[%s] Failed CmdSetECM(%d)\n", __func__, iRet);
	}

	return iRet;
}

void tcc_trmp_client_getMulti2SystemKey(unsigned char *pSystemKey, unsigned char *pCBCDefaultValue)
{
	int iRet;

	iRet = CmdGetMulti2SystemKey(pSystemKey, pCBCDefaultValue);
	if (iRet != TCC_TRMP_SUCCESS) {
		ERR_PRINTF("[%s] Failed CmdGetMulti2SystemKey(%d)\n", __func__, iRet);
	}

	return;
}

int tcc_trmp_client_getIDInfoSize(void)
{
	int iSize = 0;
	int iRet;

	iRet = CmdGetIDInfoSize(&iSize);
	if (iRet != TCC_TRMP_SUCCESS) {
		ERR_PRINTF("[%s] Failed CmdGetIDInfoSize(%d)\n", __func__, iRet);

		return 0;
	}

	return iSize;
}

int tcc_trmp_client_getIDInfo(unsigned char *pIDInfo, int iIDInfoSize)
{
	int iRet;

	iRet = CmdGetIDInfo(pIDInfo, iIDInfoSize);
	if (iRet != TCC_TRMP_SUCCESS) {
		ERR_PRINTF("[%s] Failed CmdGetIDInfo(%d)\n", __func__, iRet);
	}

	return iRet;
}
#else 
int tcc_trmp_client_init(void)
{
	return TCC_TRMP_ERROR_UNSUPPORTED;
}

void tcc_trmp_client_deinit(void)
{
	return;
}

int tcc_trmp_client_reset(void)
{
	return TCC_TRMP_ERROR_UNSUPPORTED;
}

#if 0   // sunny : not use
void tcc_trmp_client_setDeviceKeyUpdateFunction(updateDeviceKey func)
{
	return;
}
#endif

int tcc_trmp_client_setEMM(unsigned short usNetworkID, unsigned char *pSectionData, int nSectionDataSize)
{
	return TCC_TRMP_ERROR_UNSUPPORTED;
}

int tcc_trmp_client_setECM(unsigned short usNetworkID, unsigned char *pSectionData, int nSectionDataSize, unsigned char *pOddKey, unsigned char *pEvenKey)
{
	return TCC_TRMP_ERROR_UNSUPPORTED;
}

void  tcc_trmp_client_getMulti2SystemKey(unsigned char *pSystemKey, unsigned char *pCBCDefaultValue)
{
	return;
}

int tcc_trmp_client_getIDInfoSize(void)
{
	return 0;
}
	
int tcc_trmp_client_getIDInfo(unsigned char *pIDInfo, int iIDInfoSize)
{
	return TCC_TRMP_ERROR_UNSUPPORTED;
}	
#endif