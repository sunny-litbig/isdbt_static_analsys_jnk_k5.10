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
#define LOG_TAG	"TCC_TRMP_MANAGER"
#include <utils/Log.h>
#include <pthread.h>
#include <fcntl.h>

#include "tcc_trmp_manager.h"
#include "tcc_trmp_client.h"


/****************************************************************************
DEFINITION
****************************************************************************/
#define DEBUG_ISDBT_CAS
#ifdef DEBUG_ISDBT_CAS
#undef INFO_PRINTF
#define INFO_PRINTF					//ALOGI
#undef DBG_PRINTF
#define DBG_PRINTF					ALOGD
#undef ERR_PRINTF
#define ERR_PRINTF					ALOGE
#else
#undef INFO_PRINTF
#define INFO_PRINTF					
#undef DBG_PRINTF
#define DBG_PRINTF			
#undef ERR_PRINTF
#define ERR_PRINTF					ALOGE
#endif

/****************************************************************************
DEFINITION OF LOCAL VARIABLES
****************************************************************************/
pthread_mutex_t g_hMutex;

static int g_iState = TCC_TRMP_STATE_NONE;
static int g_iEnableSwapKey = FALSE;

static unsigned char g_aSystemKey[TCC_TRMP_MULTI2_SYSTEM_KEY_LENGTH] = {
	0,
};
static unsigned char g_aCBCDefaultValue[TCC_TRMP_MULTI2_CBC_DEFAULT_VALUE_LENGTH] = {
	0,
};

static unsigned char *g_pIDInfo = NULL;
static int g_iIDInfoSize = 0;

/****************************************************************************
DEFINITION OF LOCAL Function
****************************************************************************/
static void SWAP(unsigned char *pSrc, unsigned char *pDst, int nSize)
{
	int i = 0;

	if( pSrc == NULL || pDst == NULL )
		return ;

	for (i = 0; i < nSize; i += 4) {
		pDst[i] = pSrc[i + 3];
		pDst[i + 1] = pSrc[i + 2];
		pDst[i + 2] = pSrc[i + 1];
		pDst[i + 3] = pSrc[i];
	}
}

/****************************************************************************
DEFINITION OF FUNCTIONS
****************************************************************************/
int tcc_trmp_manager_init(void)
{
	int iRet;
	
	pthread_mutex_init(&g_hMutex, NULL);

	pthread_mutex_lock(&g_hMutex);

	iRet = tcc_trmp_client_init();
	if(iRet != TCC_TRMP_SUCCESS) {
		ERR_PRINTF("[%s] Failed tcc_trmp_client_init(%d)\n", __func__, iRet);
		pthread_mutex_unlock(&g_hMutex);	
	
		return iRet;
	}

	tcc_trmp_client_getMulti2SystemKey(g_aSystemKey, g_aCBCDefaultValue);

	g_iState = TCC_TRMP_STATE_INIT;
	g_iEnableSwapKey = FALSE;

	g_pIDInfo = NULL;
	g_iIDInfoSize = 0;
	
	pthread_mutex_unlock(&g_hMutex);	

	return iRet;
}

void tcc_trmp_manager_deinit(void)
{
	pthread_mutex_lock(&g_hMutex);

	tcc_trmp_client_deinit();

	g_iState = TCC_TRMP_STATE_NONE;
	g_iEnableSwapKey = FALSE;

	if(g_pIDInfo != NULL) {
		tcc_mw_free(__FUNCTION__, __LINE__, g_pIDInfo);
	}
	g_pIDInfo = NULL;
	g_iIDInfoSize = 0;
	
	pthread_mutex_unlock(&g_hMutex);

	pthread_mutex_destroy(&g_hMutex);

	return;
}

int tcc_trmp_manager_reset(void)
{
	int iRet;
	
	pthread_mutex_lock(&g_hMutex);

	iRet = tcc_trmp_client_reset();
	if(iRet != TCC_TRMP_SUCCESS) {
		ERR_PRINTF("[%s] Failed tcc_trmp_client_reset(%d)\n", __func__, iRet);
	}

	pthread_mutex_unlock(&g_hMutex);	
	
	return iRet;
}

void tcc_trmp_manager_enableKeySwap(int nEnable)
{
	unsigned char aSystemKey[TCC_TRMP_MULTI2_SYSTEM_KEY_LENGTH] = {
		0,
	};
	unsigned char aCBCDefaultValue[TCC_TRMP_MULTI2_CBC_DEFAULT_VALUE_LENGTH] = {
		0,
	};

	if (g_iEnableSwapKey != nEnable) {
		if (nEnable == TRUE) {
			tcc_trmp_client_getMulti2SystemKey(aSystemKey, aCBCDefaultValue);

			SWAP(aSystemKey, g_aSystemKey, TCC_TRMP_MULTI2_SYSTEM_KEY_LENGTH);
			SWAP(aCBCDefaultValue, g_aCBCDefaultValue, TCC_TRMP_MULTI2_CBC_DEFAULT_VALUE_LENGTH);
		} else {
			tcc_trmp_client_getMulti2SystemKey(g_aSystemKey, g_aCBCDefaultValue);
		}

		g_iEnableSwapKey = nEnable;
	}

	return;
}

#if 0   // sunny : not use
void tcc_trmp_manager_setDeviceKeyUpdateFunction(updateDeviceKey func)
{
	tcc_trmp_client_setDeviceKeyUpdateFunction(func);

	return;
}
#endif

int tcc_trmp_manager_setEMM(
	unsigned short usNetworkID, unsigned char *pSectionData, int nSectionDataSize)
{
	int iRet;

	pthread_mutex_lock(&g_hMutex);

	if(g_iState == TCC_TRMP_STATE_NONE) {
		ERR_PRINTF("[%s] EMM is dropped due to invalid state(%d)\n", __func__, g_iState);
		pthread_mutex_unlock(&g_hMutex);	
		
		return TCC_TRMP_ERROR_INVALID_STATE;
	}

	if ((nSectionDataSize < TCC_TRMP_EMM_SECTION_MIN_LENGTH)
		|| (nSectionDataSize > TCC_TRMP_EMM_SECTION_MAX_LENGTH)) {
		ERR_PRINTF("[%s] Size of EMM is invalid(%d)\n", __func__, nSectionDataSize);
		pthread_mutex_unlock(&g_hMutex);	
		
		return TCC_TRMP_ERROR_INVALID_PARAM;
	}

	iRet = tcc_trmp_client_setEMM(usNetworkID, pSectionData, nSectionDataSize);
	if(iRet != TCC_TRMP_SUCCESS) {
		ERR_PRINTF("[%s] Failed tcc_trmp_client_setEMM(%d)\n", __func__, iRet);
		pthread_mutex_unlock(&g_hMutex);	

		return iRet;
	}

	pthread_mutex_unlock(&g_hMutex);	
	
	return TCC_TRMP_SUCCESS;
}

int tcc_trmp_manager_setECM(
	unsigned short usNetworkID, unsigned char *pSectionData, int nSectionDataSize,
	unsigned char *pOddKey, unsigned char *pEvenKey)
{
	unsigned char aOrgOddKey[TCC_TRMP_SCRAMBLE_KEY_LENGTH] = {
		0,
	};
	unsigned char aOrgEvenKey[TCC_TRMP_SCRAMBLE_KEY_LENGTH] = {
		0,
	};
	int iRet = 0;

	pthread_mutex_lock(&g_hMutex);

	if(g_iState == TCC_TRMP_STATE_NONE) {
		ERR_PRINTF("[%s] ECM is dropped due to invalid state(%d)\n", __func__, g_iState);
		pthread_mutex_unlock(&g_hMutex);	
		
		return TCC_TRMP_ERROR_INVALID_STATE;
	}

	if(nSectionDataSize != TCC_TRMP_ECMF1_SECTION_LENGTH) {
		ERR_PRINTF("[%s] Size of ECMF1 is not matched(%d)\n", __func__, nSectionDataSize);
		pthread_mutex_unlock(&g_hMutex);	
		
		return TCC_TRMP_ERROR_INVALID_PARAM;
	}

	if (g_iEnableSwapKey == TRUE) {
		iRet = tcc_trmp_client_setECM(
			usNetworkID, pSectionData, nSectionDataSize, aOrgOddKey, aOrgEvenKey);
		if (iRet != TCC_TRMP_SUCCESS) {
			ERR_PRINTF("[%s] Failed tcc_trmp_client_setECM(%d)\n", __func__, iRet);
			pthread_mutex_unlock(&g_hMutex);

			return iRet;
		}

		SWAP(aOrgOddKey, pOddKey, TCC_TRMP_SCRAMBLE_KEY_LENGTH);
		SWAP(aOrgEvenKey, pEvenKey, TCC_TRMP_SCRAMBLE_KEY_LENGTH);
	} else {
		iRet =
			tcc_trmp_client_setECM(usNetworkID, pSectionData, nSectionDataSize, pOddKey, pEvenKey);
		if (iRet != TCC_TRMP_SUCCESS) {
			ERR_PRINTF("[%s] Failed tcc_trmp_client_setECM(%d)\n", __func__, iRet);
			pthread_mutex_unlock(&g_hMutex);

			return iRet;
		}
	}

	pthread_mutex_unlock(&g_hMutex);	

	return TCC_TRMP_SUCCESS;
}

int tcc_trmp_manager_getMulti2Info(unsigned char **ppSystemKey, unsigned char **ppCBCDefaultValue)
{
	int iRet;

	pthread_mutex_lock(&g_hMutex);

	if(g_iState == TCC_TRMP_STATE_NONE) {
		ERR_PRINTF("[%s] State is invalid(%d)\n", __func__, g_iState);
		pthread_mutex_unlock(&g_hMutex);	
		
		return TCC_TRMP_ERROR_INVALID_STATE;
	}

	*ppSystemKey = g_aSystemKey;
	*ppCBCDefaultValue = g_aCBCDefaultValue;
	
	pthread_mutex_unlock(&g_hMutex);	

	return TCC_TRMP_SUCCESS;
}

int tcc_trmp_manager_getIDInfo(unsigned char **ppInfo, int *piInfoSize)
{
	int iSize;
	int iRet;

	pthread_mutex_lock(&g_hMutex);

	if(g_iState == TCC_TRMP_STATE_NONE) {
		ERR_PRINTF("[%s] State is invalid(%d)\n", __func__, g_iState);
		pthread_mutex_unlock(&g_hMutex);	
		
		return TCC_TRMP_ERROR_INVALID_STATE;
	}

	iSize = tcc_trmp_client_getIDInfoSize();
	if(iSize <= 0) {
		ERR_PRINTF("[%s] Failed tcc_trmp_client_getIDInfoSize(%d)\n", __func__, iSize);
		pthread_mutex_unlock(&g_hMutex);	

		return TCC_TRMP_ERROR;	
	}

	if(iSize > g_iIDInfoSize) {
		if(g_pIDInfo != NULL) {
			tcc_mw_free(__FUNCTION__, __LINE__, g_pIDInfo);
		}

		g_pIDInfo = (unsigned char *)tcc_mw_malloc(__FUNCTION__, __LINE__, iSize);
		if(g_pIDInfo == NULL) {
			ERR_PRINTF("[%s] Failed malloc(%d)\n", __func__, iSize);
			pthread_mutex_unlock(&g_hMutex);	

			return TCC_TRMP_ERROR_INVALID_MEMORY;
		}
		
		g_iIDInfoSize = iSize;
	}
		
	iRet = tcc_trmp_client_getIDInfo(g_pIDInfo, g_iIDInfoSize);
	if(iRet != TCC_TRMP_SUCCESS) {
		ERR_PRINTF("[%s] Failed tcc_trmp_client_getIDInfo(%d)\n", __func__, iRet);
		pthread_mutex_unlock(&g_hMutex);	

		return iRet;
	}

	*ppInfo = g_pIDInfo;
	*piInfoSize = g_iIDInfoSize;

	pthread_mutex_unlock(&g_hMutex);	

	return TCC_TRMP_SUCCESS;
}
