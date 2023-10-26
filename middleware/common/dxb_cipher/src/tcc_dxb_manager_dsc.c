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
#include <utils/Log.h>
#include "tcc_dxb_manager_dsc.h"
#include "tcc_dxb_manager_cas.h"
#include "tcc_dxb_multi2_sw.h"

#ifndef NULL
	#define NULL 0
#endif

/*****************************************************************************************
define
*****************************************************************************************/
#define MAX_DEMUX_NUM			1
#define MAX_DESCRIPTION_NUM 	12	/* description id (audio/video/etc) */

#define DSC_PRINTF				//ALOGE

#define	MAX_CIPHER_PACKET_SIZE	184
#define	MAX_BLOCK_SIZE	16
#define 	MULTI2_INIT_VECTOR_SIZE		8

unsigned int	Algorithm_Mode=0;
/*****************************************************************************************
Interface API
*****************************************************************************************/
typedef struct{
	MULTI2							*m2;
	unsigned int						used;
	unsigned int						pid;
	TCCMANAGER_DESC_TYPE			descType;
	TCCMANAGER_DESC_Multi2_Cbc		cbc;
	TCCMANAGER_DESC_EncryptionSettings	envEncryp;
}TCC_DSC_GRP_TYPE;

enum OPERATION_MODE
{
	TCC_OPMODE_ECB = 0,
	TCC_OPMODE_CBC,
	TCC_OPMODE_CFB,
	TCC_OPMODE_OFB,
	TCC_OPMODE_CTR,
	TCC_OPMODE_CTR_1,
	TCC_OPMODE_CTR_2,
	TCC_OPMODE_CTR_3,
	TCC_OPMODE_MAX
};

typedef struct{
	unsigned int				used;
	TCC_DSC_GRP_TYPE		dsc[MAX_DESCRIPTION_NUM];
}TCCMANAGER_DSC_DEMUX_TYPE;

static TCCMANAGER_DSC_DEMUX_TYPE g_DscDemux[MAX_DEMUX_NUM];
static unsigned int g_demux_id = 0;
static TCC_CIPHER_ModeSettings CipherSetting;
static unsigned char	Init_Vector[MULTI2_INIT_VECTOR_SIZE];

unsigned int		TSDSC_Mode;

static unsigned int	uiOld_ScValue=0;

int	TSDEMUXDSCApi_ModeSet(TCCMANAGER_DSC_MODE_TYPE dsc_mode)
{
	TSDSC_Mode = dsc_mode;
	return TCCMANAGER_DSC_ERR_NOERROR;
}

int TSDEMUXDSCApi_Init(void)
{
	//memset(&g_pDescManager[0], 0x0, sizeof(unsigned int*)*MAX_DESCRIPTION_NUM);
	
	return TCCMANAGER_DSC_ERR_NOERROR;
}

int TSDEMUXDSCApi_Exit(void)
{
	return TCCMANAGER_DSC_ERR_NOERROR;
}


/* DESCRIPTION : descId별로 관리가 되어야 하며, 마지막 파라미터는 app로 부터 넘어 온다. */
int TSDEMUXDSCApi_Open(unsigned int *pDescId, 
								TCCMANAGER_DESC_TYPE type, 
								TCCMANAGER_DESC_EncryptionSettings *encSetting,
								TCC_CIPHER_ModeSettings *CipherModeSetting)
{
	if(CipherModeSetting == NULL)
	{
		DSC_PRINTF("[%s] CipherModeSetting is NULL Error !!!!!\n", __func__);
		return TCCMANAGER_DSC_ERR_ERROR;
	}
	
	DSC_PRINTF("TSDSC_Mode = %d,  [%s][%d] \n",TSDSC_Mode, __func__, __LINE__);

	if(TSDSC_Mode == DSC_MODE_SW_MULTI2)
	{
		TCC_DSC_GRP_TYPE *p;
		int i;
		
		if (    (type != TCCMANAGER_DESC_MULTI2)
		     || (NULL == pDescId)
		     || (NULL == encSetting) )
		{
			DSC_PRINTF("[%s] MPEGDEMUX_ERR_BADPARAMETER(%d, %p, %p)\n", __func__, type, pDescId, encSetting);
			return TCCMANAGER_DSC_ERR_BADPARAMETER;
		}

		g_DscDemux[g_demux_id].used = 1;

		for(i=0 ; i<MAX_DESCRIPTION_NUM ; i++)
		{
			if(g_DscDemux[g_demux_id].dsc[i].used == 0)
			{
				DSC_PRINTF("[%s] free desc space found(%d)\n", __func__, i);
				break;
			}
		}

		if(i>=MAX_DESCRIPTION_NUM)
		{
			DSC_PRINTF("[%s] Not enough desc space\n",__func__);
			return TCCMANAGER_DSC_ERR_BADPARAMETER;
		}

		p = &g_DscDemux[g_demux_id].dsc[i];

		p->m2 = create_multi2();
		if (NULL == p->m2)
		{
			DSC_PRINTF("[%s] Not enough free memory\n",__func__);
			return TCCMANAGER_DSC_ERR_OUTOFMEMORY;
		}
		p->used = 1;
		p->descType = type;
		memcpy(&p->envEncryp, (const void *)encSetting, sizeof(TCCMANAGER_DESC_EncryptionSettings));

		DSC_PRINTF("[%s] algorithmVar:%d, bIsUsed:%d, multi2KeySelect:%d, multi2Rounds:%d\n", __func__, 
			p->envEncryp.algorithmVar,
			p->envEncryp.bIsUsed,	/* 무시해도 됨(사용안함) */
			p->envEncryp.multi2KeySelect,
			p->envEncryp.multi2Rounds);

		DSC_PRINTF("[%s] multi2SysKey : ",__func__);
		for(i=0 ; i<32 ; i++){
			DSC_PRINTF("%02X:",p->envEncryp.multi2SysKey[i]);
		}
		DSC_PRINTF("\n");

		p->m2->set_system_key(p->m2, p->envEncryp.multi2SysKey);	// 32bytes
		p->m2->set_round(p->m2, p->envEncryp.multi2Rounds);

		DSC_PRINTF("p = %p\n", p);
		
		*pDescId = (unsigned int)p;
	}
	else if(TSDSC_Mode == DSC_MODE_HW)
	{
		Cipher_Open(CipherModeSetting);

		CipherSetting.algorithm = CipherModeSetting->algorithm;
		CipherSetting.operation_mode= CipherModeSetting->operation_mode;
		CipherSetting.algorithm_mode = CipherModeSetting->algorithm_mode;
		CipherSetting.multi2Rounds = CipherModeSetting->multi2Rounds;

		Cipher_SetMode(&CipherSetting);

		Algorithm_Mode = Cipher_Get_AlgorithmType();
	}
	else if(TSDSC_Mode == DSC_MODE_HWDEMUX)
	{
		if(CipherModeSetting->algorithm == TCC_CHIPHER_ALGORITM_AES)
			CipherSetting.algorithm = HWDEMUX_ALGORITHM_AES;
		else if(CipherModeSetting->algorithm ==TCC_CHIPHER_ALGORITM_DES)
			CipherSetting.algorithm = HWDEMUX_ALGORITHM_DES;
		else if(CipherModeSetting->algorithm ==TCC_CHIPHER_ALGORITM_MULTI2 || CipherModeSetting->algorithm ==TCC_CHIPHER_ALGORITM_MULTI2_1)
			CipherSetting.algorithm = HWDEMUX_ALGORITHM_MULTI2;
		else if(CipherModeSetting->algorithm == TCC_CHIPHER_ALGORITM_CSA)
			CipherSetting.algorithm = HWDEMUX_ALGORITHM_CSA;

		if(CipherModeSetting->operation_mode ==TCC_CHIPHER_OPMODE_CBC)
			CipherSetting.operation_mode= HWDEMUX_CIPHER_OPMODE_CBC;
		else if(CipherModeSetting->operation_mode ==TCC_CHIPHER_OPMODE_ECB)
			CipherSetting.operation_mode= HWDEMUX_CIPHER_OPMODE_ECB;
		else
			CipherSetting.operation_mode= CipherModeSetting->operation_mode;

		CipherSetting.algorithm_mode = CipherModeSetting->algorithm_mode;
		CipherSetting.multi2Rounds = CipherModeSetting->multi2Rounds;

		HWDEMUX_Cipher_Open(&CipherSetting);

		Algorithm_Mode = Cipher_Get_AlgorithmType();
	}

	return TCCMANAGER_DSC_ERR_NOERROR;
}

int TSDEMUXDSCApi_Close(unsigned int DescId)
{
	unsigned int isFound = 0;
	
	DSC_PRINTF("TSDSC_Mode = %d,  [%s][%d] \n",TSDSC_Mode, __func__, __LINE__);

	if(TSDSC_Mode == DSC_MODE_SW_MULTI2)
	{
		int i, j;
		TCC_DSC_GRP_TYPE *p = NULL;

		for(i=0 ; i<MAX_DEMUX_NUM ; i++)
		{
			if(g_DscDemux[i].used)
			{
				for(j=0 ; j<MAX_DESCRIPTION_NUM ; j++)
				{
					p = &g_DscDemux[i].dsc[j];
					if(p == (TCC_DSC_GRP_TYPE*)DescId){
						DSC_PRINTF("[%s] DescId found(%d - 0x%X)\n", __func__, j, DescId);
						//goto FOUND;	 Noah, To avoid CodeSonar warning, Redundant Condition
						isFound = 1;
						break;
					}
				}

				if(isFound == 1)
					break;
			}
		}

/* Noah, To avoid CodeSonar warning, Redundant Condition
		if(i>=MAX_DEMUX_NUM){
			DSC_PRINTF("[%s] Description ID not found(0x%X)\n",__func__, DescId);
			return TCCMANAGER_DSC_ERR_ERROR;
		}

	FOUND:
*/
		if(isFound == 0){
			DSC_PRINTF("[%s] Description ID not found(0x%X)\n",__func__, DescId);
			return TCCMANAGER_DSC_ERR_ERROR;
		}

		if((p != NULL) && (p->m2 != NULL))
		{
			release_multi2(p->m2);
			p->used = 0;
			p->m2 = NULL;
		}
	}
	else if(TSDSC_Mode == DSC_MODE_HW)
	{
		Cipher_Close();
	}
	else if(TSDSC_Mode == DSC_MODE_HWDEMUX)
	{
		HWDEMUX_Cipher_Close();
	}
	
	return TCCMANAGER_DSC_ERR_NOERROR;
}

/* DESCRIPTION : demux의 startPid로 filtering된것 중에 descramble될 packet의 pid를 설정한다. */
int TSDEMUXDSCApi_SetPid(unsigned int DescId, unsigned short pid )
{
	int error = TCCMANAGER_DSC_ERR_NOERROR;
	unsigned int i, j;
	TCC_DSC_GRP_TYPE *p = NULL;
	unsigned int isFound = 0;
	
	DSC_PRINTF("[%s] DescId:0x%x, pid:0x%X(%d)\n", __func__, DescId, pid, pid);

	for(i=0 ; i<MAX_DEMUX_NUM ; i++)
	{
		if(g_DscDemux[i].used)
		{
			for(j=0 ; j<MAX_DESCRIPTION_NUM ; j++)
			{
				p = &g_DscDemux[i].dsc[j];
				if(p == (TCC_DSC_GRP_TYPE*)DescId){
					DSC_PRINTF("[%s] DescId found(%d - 0x%X)\n", __func__, j, DescId);
					//goto FOUND;    Noah, To avoid CodeSonar warning, Redundant Condition
					isFound = 1;
					break;
				}
			}

			if(isFound == 1)
				break;	
		}
	}

/* Noah, To avoid CodeSonar warning, Redundant Condition
	if(i>=MAX_DEMUX_NUM){
		DSC_PRINTF("[%s] Description ID not found(0x%X)\n",__func__, DescId);
		return TCCMANAGER_DSC_ERR_ERROR;
	}

FOUND:
*/

	if(isFound == 0){
		DSC_PRINTF("[%s] Description ID not found(0x%X)\n",__func__, DescId);
		return TCCMANAGER_DSC_ERR_ERROR;
	}

	p->pid = pid;

	/* 여기에 Demux한테 descramble해야 될 pid를 알려 주는 루틴이 필요하다. */
	
	return error;
}

/* DESCRIPTION : descramble pid로 설정된 pid를 해제한다. */
int TSDEMUXDSCApi_ClearPid(unsigned int DescId, unsigned short pid)
{
	unsigned int i, j;
	TCC_DSC_GRP_TYPE *p = NULL;
	unsigned int isFound = 0;

	DSC_PRINTF("[%s] DescId:0x%x, pid:0x%X(%d)\n", __func__, DescId, pid, pid);

	for(i=0 ; i<MAX_DEMUX_NUM ; i++)
	{
		if(g_DscDemux[i].used)
		{
			for(j=0 ; j<MAX_DESCRIPTION_NUM ; j++)
			{
				p = &g_DscDemux[i].dsc[j];
				if(p == (TCC_DSC_GRP_TYPE*)DescId){
					DSC_PRINTF("[%s] DescId found(%d - 0x%X)\n", __func__, j, DescId);
					//goto FOUND;    Noah, To avoid CodeSonar warning, Redundant Condition
					isFound = 1;
					break;
				}
			}

			if(isFound == 1)
				break;
		}
	}

/* Noah, To avoid CodeSonar warning, Redundant Condition
	if(i>=MAX_DEMUX_NUM){
		DSC_PRINTF("[%s] Description ID not found(0x%X)\n",__func__, DescId);
		return TCCMANAGER_DSC_ERR_ERROR;
	}

FOUND:
*/

	if(isFound == 0){
		DSC_PRINTF("[%s] Description ID not found(0x%X)\n",__func__, DescId);
		return TCCMANAGER_DSC_ERR_ERROR;
	}

	p->pid = 0xFFFFFFFF;

	/* 여기에 Demux한테 descramble하지 않아도 될 pid를 알려 주는 루틴이 필요하다. */

	
	return TCCMANAGER_DSC_ERR_NOERROR;
}

int TSDEMUXDSCApi_ChkPid(unsigned int pid)
{
	int i, j;
	TCC_DSC_GRP_TYPE *p = NULL;
	
	for(i=0 ; i<MAX_DEMUX_NUM ; i++)
	{
		if(g_DscDemux[i].used)
		{
			for(j=0 ; j<MAX_DESCRIPTION_NUM ; j++)
			{
				p = &g_DscDemux[i].dsc[j];
				if(p->pid == pid){
					DSC_PRINTF("[%s] DescId found(%d - 0x%X)\n", __func__, j, pid);
					return 1;
				}
			}
		}
	}

	DSC_PRINTF("[%s] pid(0x%X) found\n", __func__, pid);

	return 0;
}

/* DESCRIPTION : 앞의 파라미터 p_key_data는 데이터키가 넘어오고, 뒤의 파라미터 p_init_vector는 cbc가 넘어 온다. */
int TSDEMUXDSCApi_SetKey
(
	unsigned int DescId,
	TCCMANAGER_DESC_ODD_EVEN parity,
	unsigned char *p_key_data,
	unsigned int key_length,
	unsigned char *p_init_vector,				// 사용하지 않는 경우 NULL
	unsigned int vector_length,				// 사용하지 않는 경우 '0'
	TCC_CIPHER_KeySettings *Keysetting
)
{
	if(Keysetting == NULL)
	{
		DSC_PRINTF("[%s] Keysetting is NULL Error !!!!!\n", __func__);		
		return TCCMANAGER_DSC_ERR_ERROR;
	}

	if(TSDSC_Mode == DSC_MODE_SW_MULTI2)
	{
		unsigned int i = 0, j = 0;
		TCC_DSC_GRP_TYPE *p = NULL;
		unsigned int isFound = 0;
		
		DSC_PRINTF("[%s] DescId:0x%x, key_length:%d, vector_length:%d\n", __func__, DescId, key_length, vector_length);
		
		for(i=0 ; i<MAX_DEMUX_NUM ; i++)
		{
			if(g_DscDemux[i].used)
			{
				for(j=0 ; j<MAX_DESCRIPTION_NUM ; j++)
				{
					p = &g_DscDemux[i].dsc[j];
					if(p == (TCC_DSC_GRP_TYPE*)DescId){
						DSC_PRINTF("[%s] DescId found(%d - 0x%X)\n", __func__, j, DescId);
						//goto FOUND;    Noah, To avoid CodeSonar warning, Redundant Condition
						isFound = 1;
						break;
					}
				}

				if( isFound == 1 )
					break;
			}
		}

/* Noah, To avoid CodeSonar warning, Redundant Condition
		if(i>=MAX_DEMUX_NUM){
			DSC_PRINTF("[%s] Description ID not found(0x%X)\n",__func__, DescId);
			return TCCMANAGER_DSC_ERR_ERROR;
		}

	FOUND:
*/

		if(isFound == 0)
		{
			DSC_PRINTF("[%s] Description ID not found(0x%X)\n",__func__, DescId);
			return TCCMANAGER_DSC_ERR_ERROR;
		}

		if((p_key_data != NULL)&&(key_length))
		{
			char ch[16];
			char *pTmp = (char*)p_key_data;

		#if 1
			DSC_PRINTF("[%s] set_scramble_key ->",__func__);
			for(i=0 ; i<key_length ; i++)
			{
				DSC_PRINTF("%02X:", pTmp[i]);
			}		
			DSC_PRINTF("\n");	

			p->m2->set_scramble_key(p->m2, parity, p_key_data);
		#else
			DSC_PRINTF("[%s] set_scramble_key ->",__func__);
			for(i=0 ; i<key_length ; i++)
			{
				DSC_PRINTF("%02X:", pTmp[i]);
			}		
			DSC_PRINTF("\n");	
			
			for(i=0 ; i<key_length ; i++)
			{
				DSC_PRINTF("%02X:", pTmp[key_length-1-i]);
				ch[i] = pTmp[key_length-1-i];
			}	

			DSC_PRINTF("\n[%s] set_scramble_key(rev) ->",__func__);
			for(i=0 ; i<key_length ; i++)
			{
				DSC_PRINTF("%02X:", ch[i]);
			}		
			DSC_PRINTF("\n");

			p->m2->set_scramble_key(p->m2, parity, (uint8_t*)&ch[0]);
		#endif /* 0 */
		}

		if((p_init_vector != NULL)&&(vector_length))
		{
			char ch[16];
			char *pTmp = (char*)p_init_vector;

		#if 1
			DSC_PRINTF("[%s] set_init_cbc ->",__func__);
			for(i=0 ; i<vector_length ; i++)
			{
				DSC_PRINTF("%02X:", pTmp[i]);
			}
			DSC_PRINTF("\n");

			p->m2->set_init_cbc(p->m2, p_init_vector);
		#else
			DSC_PRINTF("[%s] set_init_cbc ->",__func__);
			for(i=0 ; i<vector_length ; i++)
			{
				DSC_PRINTF("%02X:", pTmp[i]);
			}		
			DSC_PRINTF("\n");	
			
			for(i=0 ; i<vector_length ; i++)
			{
				DSC_PRINTF("%02X:", pTmp[vector_length-1-i]);
				ch[i] = pTmp[vector_length-1-i];
			}	

			DSC_PRINTF("\n[%s] set_init_cbc(rev) ->",__func__);
			for(i=0 ; i<vector_length ; i++)
			{
				DSC_PRINTF("%02X:", ch[i]);
			}		
			DSC_PRINTF("\n");

			p->m2->set_init_cbc(p->m2, (uint8_t*)&ch[0]);
		#endif /* 0 */
		}
		
	}
#if 0
	else if(TSDSC_Mode == DSC_MODE_HW)
#else
	else if(TSDSC_Mode == DSC_MODE_HW || TSDSC_Mode == DSC_MODE_HWDEMUX)
#endif
	{
		Cipher_SetKey(Keysetting, TSDSC_Mode);

		if(TSDSC_Mode == DSC_MODE_HWDEMUX)
			HWDEMUX_Cipher_SetBlock(Keysetting->Key_flag);

		if(TSDSC_Mode == DSC_MODE_HW)
		{
			if(Algorithm_Mode == TCC_CHIPHER_ALGORITM_MULTI2 || Algorithm_Mode == TCC_CHIPHER_ALGORITM_MULTI2_1)
			{
				//10th Try, David, To avoid Code Sonar's warning, NULL Pointer Dereference
				// add exception-handling-code for (p_init_vector) 			
				if(Keysetting->p_init_vector == NULL)
				{
					DSC_PRINTF("[%s:%d] p_init_vector is null\n", __func__, __LINE__);
				}
				else
				{
					memcpy(Init_Vector, Keysetting->p_init_vector, MULTI2_INIT_VECTOR_SIZE);
				}
			}
		}
	}
/* Original Code
	{
		Cipher_SetKey(Keysetting, TSDSC_Mode);
		if(TSDSC_Mode == DSC_MODE_HWDEMUX)
			HWDEMUX_Cipher_SetBlock(Keysetting->Key_flag);

		if(TSDSC_Mode == DSC_MODE_HW)
		{
			if(Algorithm_Mode == TCC_CHIPHER_ALGORITM_MULTI2 || Algorithm_Mode == TCC_CHIPHER_ALGORITM_MULTI2_1)
			{
				memcpy(Init_Vector, Keysetting->p_init_vector, MULTI2_INIT_VECTOR_SIZE);
			}
		}
	}
*/
	return TCCMANAGER_DSC_ERR_NOERROR;
}



/* Main Descramble 함수 */
int TSDEMUXDSCApi_Decrypt(unsigned int type, unsigned char *pBuf, unsigned int uiSize, void *pUserData)
{
	int err = TCCMANAGER_DSC_ERR_NOERROR;

//	LOGE("%s %d TSDSC_Mode = %d, uiSize = %d  \n", __func__, __LINE__, TSDSC_Mode, uiSize);

	if(TSDSC_Mode == DSC_MODE_SW_MULTI2)
	{
		TCC_DSC_GRP_TYPE *p;

		p = (TCC_DSC_GRP_TYPE*)&g_DscDemux[g_demux_id].dsc[0];

		if(p->m2 == NULL)
		{
			DSC_PRINTF("[%s] Invalid handle\n",__func__);
			return TCCMANAGER_DSC_ERR_ERROR;
		}

		DSC_PRINTF("[%s] type:%d, ts:0x%x, size:%d\n",__func__, type, (char)pBuf[0], uiSize);
		
		err = p->m2->decrypt(p->m2, type, pBuf, uiSize);

		DSC_PRINTF("[%s] type:%d, ts:0x%x, size:%d\n",__func__, type, (char)pBuf[0], uiSize);
	}
	else if(TSDSC_Mode == DSC_MODE_HW)
	{
		unsigned int		i, blocksize, algorithm_mode, cipher_size, remain=0;
		unsigned char		temp_buf[MULTI2_INIT_VECTOR_SIZE];

		if(Cipher_Get_Handle()<0)
			return err;

//		DSC_PRINTF("encryption data size = %d type = %d, Algorithm_Mode = %d  \n", uiSize, type, Algorithm_Mode);
//		Cipher_test();

		blocksize = Cipher_Get_Blocksize();
		cipher_size = uiSize;
#if 0
		remain = cipher_size%blocksize;
		if(remain>0)
		{
			int i;
			ALOGE("encryption data size = %d \n", uiSize);
			for(i=0; i<uiSize; i+=8)
			{
				ALOGE("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", 
					pBuf[i+0], pBuf[i+1], pBuf[i+2], pBuf[i+3],pBuf[i+4], pBuf[i+5], pBuf[i+6], pBuf[i+7]);
			}
		}
#endif

		if(cipher_size >= blocksize && 0 < blocksize)
		{
			remain = cipher_size%blocksize;
			if(remain>0)
			{
				cipher_size-= remain;
	 			if(Algorithm_Mode == TCC_CHIPHER_ALGORITM_MULTI2 || Algorithm_Mode == TCC_CHIPHER_ALGORITM_MULTI2_1)
				{
					if(blocksize <= MULTI2_INIT_VECTOR_SIZE)    // Noah, To avoid Codesonar's warning, Buffer Overrun
					{
						memcpy(temp_buf, &pBuf[cipher_size-blocksize], blocksize);
					}
 				}
	 		}
//			Cipher_SetMode(&CipherSetting);
			err = Cipher_SetBlock(type);
			if( err != TCCMANAGER_DSC_ERR_NOERROR)
				return err;
			Cipher_Run(pBuf, pBuf, cipher_size, TCC_CHIPHER_IOCTL_DECRYPT);
		}
		else
		{
			remain = cipher_size;
			cipher_size =0;
			
 			if(Algorithm_Mode == TCC_CHIPHER_ALGORITM_MULTI2 || Algorithm_Mode == TCC_CHIPHER_ALGORITM_MULTI2_1)
			{
				for(i=0;i<MULTI2_INIT_VECTOR_SIZE; i+=4)
				{
					temp_buf[i] = Init_Vector[i+3];
					temp_buf[i+1] = Init_Vector[i+2];
					temp_buf[i+2] = Init_Vector[i+1];
					temp_buf[i+3] = Init_Vector[i];
				}
#if 0
			{
				int i;
				ALOGE("For Residual IV data \n");
				for(i=0; i<MULTI2_INIT_VECTOR_SIZE; i+=8)
				{
					ALOGE("1. 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", 
						temp_buf[i+0], temp_buf[i+1], temp_buf[i+2], temp_buf[i+3],temp_buf[i+4], temp_buf[i+5], temp_buf[i+6], temp_buf[i+7]);
				}
			}
#endif
			}
		}
#if 1
 		if(remain>0)
		{
//			if(cipher_size ==0)
//	 			DSC_PRINTF("remain = %d, input size = %d \n", remain, uiSize);
			
			if(Algorithm_Mode == TCC_CHIPHER_ALGORITM_MULTI2 || Algorithm_Mode == TCC_CHIPHER_ALGORITM_MULTI2_1)
			{
				CipherSetting.operation_mode = TCC_OPMODE_ECB;
				Cipher_SetMode(&CipherSetting);

				err = Cipher_SetBlock(type);
				if( err != TCCMANAGER_DSC_ERR_NOERROR)
					return err;

				Cipher_Run(temp_buf, temp_buf, blocksize, TCC_CHIPHER_IOCTL_ENCRYPT);

#if 0
			if(cipher_size ==0)
			{
				int i;
				ALOGE("encryption IV ECB mode\n");
				for(i=0; i<MULTI2_INIT_VECTOR_SIZE; i+=8)
				{
					ALOGE("2. 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", 
						temp_buf[i+0], temp_buf[i+1], temp_buf[i+2], temp_buf[i+3],temp_buf[i+4], temp_buf[i+5], temp_buf[i+6], temp_buf[i+7]);
				}

				ALOGE("Residual Input data size  = %d \n", remain);
				for(i=0; i<remain; i+=8)
				{
					ALOGE("3. 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", 
						pBuf[cipher_size +i+0], pBuf[cipher_size +i+1], pBuf[cipher_size +i+2], pBuf[cipher_size +i+3],pBuf[cipher_size +i+4], pBuf[cipher_size +i+5], pBuf[cipher_size +i+6], pBuf[cipher_size +i+7]);
				}

			}
#endif
				for(i=0; i<remain; i++)
				{
					pBuf[cipher_size +i] = (unsigned char)(pBuf[cipher_size +i] ^temp_buf[i]);
				}
				CipherSetting.operation_mode = TCC_OPMODE_CBC;
				Cipher_SetMode(&CipherSetting);

#if 0
			if(cipher_size ==0)
			{
				int i;
				ALOGE("decryption remain data size = %d \n", remain);
				for(i=0; i<remain; i+=8)
				{
					ALOGE("4. 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", 
						pBuf[cipher_size +i+0], pBuf[cipher_size +i+1], pBuf[cipher_size +i+2], pBuf[cipher_size +i+3],pBuf[cipher_size +i+4], pBuf[cipher_size +i+5], pBuf[cipher_size +i+6], pBuf[cipher_size +i+7]);
				}
			}
#endif


			}
		}
#endif


#if 0
 		if(remain>0)
		{
			int i;

			ALOGE("decryption data uiSize = %d\n", uiSize);
			for(i=0; i<uiSize; i+=8)
			{
				ALOGE("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", 
					pBuf[i+0], pBuf[i+1], pBuf[i+2], pBuf[i+3],pBuf[i+4], pBuf[i+5], pBuf[i+6], pBuf[i+7]);
			}
		}
#endif

	}

	return err;
}

 /* 사용안함 */
int TSDEMUXDSCApi_SetProtectionKey
(
	unsigned int DescId,
	unsigned char *pCWPK,
	unsigned int key_length
)
{
	DSC_PRINTF("[%s] Not supported.\n",__func__);
	
	return TCCMANAGER_DSC_ERR_NOERROR;
}




 
