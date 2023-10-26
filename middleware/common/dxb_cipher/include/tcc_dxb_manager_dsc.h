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
#ifndef _TCC_DXB_MANAGER_DSC_H_
#define _TCC_DXB_MANAGER_DSC_H_

#include "tcc_dxb_interface_cipher.h"

typedef enum
{
	TCCMANAGER_DSC_ERR_NOERROR =0,
	TCCMANAGER_DSC_ERR_BADPARAMETER,
	TCCMANAGER_DSC_ERR_OUTOFMEMORY,
	TCCMANAGER_DSC_ERR_ERROR
}TCCMANAGER_DSC_ERROR;


typedef enum
{
	TCCMANAGER_DESC_DVB,
	TCCMANAGER_DESC_AES,
	TCCMANAGER_DESC_DES,
	TCCMANAGER_DESC_AES_CLEARMODE,	//CI+
	TCCMANAGER_DESC_DES_CLEARMODE,	//CI+
	TCCMANAGER_DESC_3DESABA,
	TCCMANAGER_DESC_MULTI2
} TCCMANAGER_DESC_TYPE;

typedef enum
{
	TCCMANAGER_DESC_ODD,
	TCCMANAGER_DESC_EVEN
} TCCMANAGER_DESC_ODD_EVEN;

typedef enum TCCMANAGER_DESC_AlgorithmVariant
{
	TCCMANAGER_DESC_AlgorithmVariant_eEcb,
	TCCMANAGER_DESC_AlgorithmVariant_eCbc,
	TCCMANAGER_DESC_AlgorithmVariant_eCounter,
	/* Add new algorithm variant definition before this line */
	TCCMANAGER_DESC_AlgorithmVariant_eMax
}   TCCMANAGER_DESC_AlgorithmVariant;

typedef struct TCCMANAGER_DESC_Multi2_Cbc
{
	unsigned int cbc_l;
	unsigned int cbc_r;	
} TCCMANAGER_DESC_Multi2_Cbc;

typedef struct TCCMANAGER_DESC_EncryptionSettings
{
	TCCMANAGER_DESC_AlgorithmVariant	algorithmVar;
	int				bIsUsed;
	unsigned short	multi2KeySelect;
	unsigned char	multi2Rounds;
	unsigned char	multi2SysKey[32];
} TCCMANAGER_DESC_EncryptionSettings;

typedef enum
{
	DSC_MODE_HW =0,
	DSC_MODE_SW_MULTI2,
	DSC_MODE_HWDEMUX,
	DSC_MODE_NONE,
}TCCMANAGER_DSC_MODE_TYPE;

int TSDEMUXDSCApi_ModeSet(TCCMANAGER_DSC_MODE_TYPE dsc_mode);
int TSDEMUXDSCApi_Open(unsigned int *pDescId, 
								TCCMANAGER_DESC_TYPE type, 
								TCCMANAGER_DESC_EncryptionSettings *encSetting,
								TCC_CIPHER_ModeSettings *CipherModeSetting);
int TSDEMUXDSCApi_SetPid(unsigned int DescId, unsigned short pid );
int TSDEMUXDSCApi_ClearPid(unsigned int DescId, unsigned short pid);
int TSDEMUXDSCApi_Close(unsigned int DescId);
int TSDEMUXDSCApi_SetKey
(
	unsigned int DescId,
	TCCMANAGER_DESC_ODD_EVEN parity,
	unsigned char *p_key_data,
	unsigned int key_length,
	unsigned char *p_init_vector,				// 사용하지 않는 경우 NULL
	unsigned int vector_length,				// 사용하지 않는 경우 '0'
	TCC_CIPHER_KeySettings *Keysetting);
int TSDEMUXDSCApi_Decrypt(unsigned int type, unsigned char *pBuf, unsigned int uiSize, void *pUserData);
int TSDEMUXDSCApi_SetProtectionKey(	unsigned int DescId,unsigned char *pCWPK,unsigned int key_length);



#endif													  
