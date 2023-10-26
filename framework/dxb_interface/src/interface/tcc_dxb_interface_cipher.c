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
#include <stdio.h>
#include <utils/Log.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "tcc_dxb_interface_cipher.h"


/*******************************************************************************/
/*  ForHWDEMUX Cipher ****************************************************************/
#ifndef     _TCC_DXB_CONTROL_H_
#define     _TCC_DXB_CONTROL_H_
#include    <linux/types.h>

#define DXB_CTRL_DEV_FILE		"/dev/tcc_isdbt_ctrl"
#define DXB_CTRL_DEV_NAME		"tcc_isdbt_ctrl"
#define DXB_CTRL_DEV_MAJOR		250
#define DXB_CTRL_DEV_MINOR		0

#define IOCTL_DXB_CTRL_OFF		    _IO(DXB_CTRL_DEV_MAJOR, 1)
#define IOCTL_DXB_CTRL_ON			_IO(DXB_CTRL_DEV_MAJOR, 2)
#define IOCTL_DXB_CTRL_RESET    	_IO(DXB_CTRL_DEV_MAJOR, 3)
#define IOCTL_DXB_CTRL_SET_BOARD    _IO(DXB_CTRL_DEV_MAJOR, 4)
#define IOCTL_DXB_CTRL_GET_CTLINFO  _IO(DXB_CTRL_DEV_MAJOR, 5)
#define IOCTL_DXB_CTRL_RF_PATH      _IO(DXB_CTRL_DEV_MAJOR, 6)
#define IOCTL_DXB_CTRL_SET_CTRLMODE _IO(DXB_CTRL_DEV_MAJOR, 7)
/* add for HwDemux cipher */
#define IOCTL_DXB_CTRL_HWDEMUX_CIPHER_SET_ALGORITHM	    _IO(DXB_CTRL_DEV_MAJOR, 12)
#define IOCTL_DXB_CTRL_HWDEMUX_CIPHER_SET_KEY		    _IO(DXB_CTRL_DEV_MAJOR, 13)
#define IOCTL_DXB_CTRL_HWDEMUX_CIPHER_SET_VECTOR	    _IO(DXB_CTRL_DEV_MAJOR, 14)
#define IOCTL_DXB_CTRL_HWDEMUX_CIPHER_SET_DATA	    _IO(DXB_CTRL_DEV_MAJOR, 15)
#define IOCTL_DXB_CTRL_HWDEMUX_CIPHER_EXECUTE	    _IO(DXB_CTRL_DEV_MAJOR, 16)
#define IOCTL_DXB_CTRL_UNLOCK                       _IO(DXB_CTRL_DEV_MAJOR, 17)
#define IOCTL_DXB_CTRL_LOCK                         _IO(DXB_CTRL_DEV_MAJOR, 18)
#endif

/*******************************************************************************/
#define CIPHER_DEVICE "/dev/cipher"
#define DEBUG_PRT	//ALOGE

#define	BLOCK_SIZE_AES	16	//16*8 = 128, AES
#define	BLOCK_SIZE_OTHERS	8	//8*8 = 64, DES, Multi2

#define EVEN_KEY_TYPE	0x02
#define ODD_KEY_TYPE	0x03

#define MAX_CIPHER_IV_SIZE		16		//128bit
#define ISDBT_SYSTEM_KEY_SIZE	32
#define ISDBT_ECM_KEY_LEN		8

static int	Cipher_Handle=-1;
static TCC_CIPHER_ModeSettings CipherSettingValue;

static stCIPHER_KEY 	stEvenKey;
static stCIPHER_KEY 	stOddKey;

static stCIPHER_KEY 	stOldEvenKey;
static stCIPHER_KEY 	stOldOddKey;

static stCIPHER_VECTOR stVector;
static stCIPHER_SYSTEMKEY	stSystemkey;

static int 	Cipher_Keytype=-1;
static int	Key_SefFlag=-1;

#define USE_MUTEX	
#ifdef USE_MUTEX
pthread_mutex_t	CipherMutex;
#endif

int		HWDEMUXCIPHER_EXECUTE_STATUS = -1;

unsigned char ucIV_SwapData[MAX_CIPHER_IV_SIZE];
unsigned char ucEvenKEY_SwapData[ISDBT_SYSTEM_KEY_SIZE];
unsigned char ucOddKEY_SwapData[ISDBT_SYSTEM_KEY_SIZE];
unsigned char ucSystem_SwapData[ISDBT_SYSTEM_KEY_SIZE];

unsigned char	ucOldEvenKey_Data[ISDBT_ECM_KEY_LEN];
unsigned char	ucOldOddKey_Data[ISDBT_ECM_KEY_LEN];

static unsigned int	uiEvenKeyChange = -1;
static unsigned int	uiOddKeyChange = -1;

int Cipher_Open(TCC_CIPHER_ModeSettings *CipherModeSetting)
{
#ifndef WIN32
	stCIPHER_ALGORITHM stAlgorithm;
	int	err = 0;

	if(Cipher_Handle > 0)
	{
		DEBUG_PRT("%d Cipher Block Open ciipher_Handle = %d \n", __LINE__, Cipher_Handle);
		return -1;
	}
	
    if (CipherModeSetting == NULL)
	{
        DEBUG_PRT("[%s] CipherModeSetting is NULL.\n", __func__);
		return -1;
	}

	Cipher_Handle = open(CIPHER_DEVICE, O_RDWR);
	if(Cipher_Handle < 0)
	{
		DEBUG_PRT("%d Cipher Block Open Fail Cipher_Handle = %d \n", __LINE__, Cipher_Handle);
		return -1;
	}
	DEBUG_PRT("Cipher_Handle = %d,  [%s][%d] \n",Cipher_Handle, __func__, __LINE__);

	stAlgorithm.uDmaMode = 1;//TCC_CIPHER_DMA_INTERNAL;
	stAlgorithm.uAlgorithm = CipherSettingValue.algorithm = CipherModeSetting->algorithm;
	stAlgorithm.uOperationMode = CipherSettingValue.operation_mode= CipherModeSetting->operation_mode;

	if(stAlgorithm.uAlgorithm == TCC_CHIPHER_ALGORITM_AES ||stAlgorithm.uAlgorithm == TCC_CHIPHER_ALGORITM_DES)
		stAlgorithm.uArgument1 = CipherSettingValue.algorithm_mode = CipherModeSetting->algorithm_mode;
	else
		stAlgorithm.uArgument1 = CipherSettingValue.multi2Rounds = CipherModeSetting->multi2Rounds;

	DEBUG_PRT("%s %d uAlgorithm = %d uOperationMode = %d algorithm_mode= %d \n",
		__func__, __LINE__, stAlgorithm.uAlgorithm , stAlgorithm.uOperationMode, stAlgorithm.uArgument1);

	stAlgorithm.uArgument2 = 0;

#ifdef USE_MUTEX
	pthread_mutex_init(&CipherMutex, NULL);
#endif

	if(ioctl(Cipher_Handle, TCC_CHIPHER_IOCTL_SET_ALGORITHM, &stAlgorithm) != 0)
	{
		DEBUG_PRT("Cipher Error: TCC_CHIPHER_IOCTL_SET_ALGORITHM\n");
		return -1;
	}

	memset(&stEvenKey, 0, sizeof(stCIPHER_KEY));
	memset(&stOddKey, 0, sizeof(stCIPHER_KEY));
	memset(&stOldEvenKey, 0, sizeof(stCIPHER_KEY));
	memset(&stOldOddKey, 0, sizeof(stCIPHER_KEY));
	memset(ucOldEvenKey_Data, 0, ISDBT_ECM_KEY_LEN);
	memset(ucOldOddKey_Data, 0, ISDBT_ECM_KEY_LEN);

#endif
	return 0;
}


int HWDEMUX_Cipher_Open(TCC_CIPHER_ModeSettings *CipherModeSetting)
{
#ifndef WIN32
	stHWDEMUXCIPHER_ALGORITHM stAlgorithm;
	int	err = 0;
	if(Cipher_Handle > 0)
	{
		ALOGE("%d HWDEMUX Cipher Block Open Error Cipher_Handle = %d \n", __LINE__, Cipher_Handle);
		return -1;
	}
	
    if (CipherModeSetting == NULL)
	{
        DEBUG_PRT("[%s] CipherModeSetting is NULL.\n", __func__);
		return -1;
	}

	Cipher_Handle = open(DXB_CTRL_DEV_FILE, O_RDWR | O_NDELAY);
	if(Cipher_Handle < 0)
	{
		ALOGE("%d HWDEMUX Cipher Block Open Fail Cipher_Handle = %d \n", __LINE__, Cipher_Handle);
		return -1;
	}
	DEBUG_PRT("Cipher_Handle = %d,  [%s][%d] \n",Cipher_Handle, __func__, __LINE__);

	stAlgorithm.uAlgorithm = CipherSettingValue.algorithm = CipherModeSetting->algorithm;
	stAlgorithm.uOperationMode = CipherSettingValue.operation_mode= CipherModeSetting->operation_mode;
	stAlgorithm.uResidual = 4;
	stAlgorithm.uSmsg = 1;//2;
	stAlgorithm.uDemuxId = 0;

	DEBUG_PRT("%s %d uAlgorithm = %d uOperationMode = %d\n",
		__func__, __LINE__, stAlgorithm.uAlgorithm , stAlgorithm.uOperationMode);

#ifdef USE_MUTEX
	pthread_mutex_init(&CipherMutex, NULL);
#endif

	if(ioctl(Cipher_Handle, IOCTL_DXB_CTRL_HWDEMUX_CIPHER_SET_ALGORITHM, &stAlgorithm) != 0)
	{
		DEBUG_PRT("Cipher Error: TCC_CHIPHER_IOCTL_SET_ALGORITHM\n");
		return -1;
	}

	memset(&stEvenKey, 0, sizeof(stCIPHER_KEY));
	memset(&stOddKey, 0, sizeof(stCIPHER_KEY));
	memset(&stOldEvenKey, 0, sizeof(stCIPHER_KEY));
	memset(&stOldOddKey, 0, sizeof(stCIPHER_KEY));
	memset(ucOldEvenKey_Data, 0, ISDBT_ECM_KEY_LEN);
	memset(ucOldOddKey_Data, 0, ISDBT_ECM_KEY_LEN);
#endif
	return 0;
}

int Cipher_Close()
{
#ifndef WIN32
	if(Cipher_Handle > 0)
	{
		close(Cipher_Handle);
		Cipher_Handle = -1;

#ifdef USE_MUTEX
		pthread_mutex_destroy(&CipherMutex);
#endif
	}
	else
	{
		DEBUG_PRT("Cipher_Handle close error\n");
		return -1;
	}
#endif
	return 0;
}

int HWDEMUX_Cipher_Close()
{
#ifndef WIN32
	DEBUG_PRT("[%s][%d] Cipher_Handle = %d\n", __func__, __LINE__, Cipher_Handle);
	if(Cipher_Handle > 0)
	{
		close(Cipher_Handle);
		Cipher_Handle = -1;
		HWDEMUXCIPHER_EXECUTE_STATUS = -1;

#ifdef USE_MUTEX
		pthread_mutex_destroy(&CipherMutex);
#endif
	}
	else
	{
		DEBUG_PRT("HWDEMUX Cipher_Handle close error\n");
		return -1;
	}
#endif
	return 0;
}

int Cipher_SetMode(TCC_CIPHER_ModeSettings *CipherModeSetting)
{
    if (CipherModeSetting == NULL)
	{
		DEBUG_PRT("[%s] CipherModeSetting is NULL.\n", __func__);
		return -1;
	}

#ifndef WIN32
	stHWDEMUXCIPHER_ALGORITHM stAlgorithm;
#ifdef USE_MUTEX
	pthread_mutex_lock(&CipherMutex);
#endif

	stAlgorithm.uAlgorithm = CipherSettingValue.algorithm = CipherModeSetting->algorithm;
	stAlgorithm.uOperationMode = CipherSettingValue.operation_mode= CipherModeSetting->operation_mode;
#if 0
	if(stAlgorithm.uAlgorithm == TCC_CHIPHER_ALGORITM_AES ||stAlgorithm.uAlgorithm == TCC_CHIPHER_ALGORITM_DES)
		stAlgorithm.uArgument1 = CipherSettingValue.algorithm_mode = CipherModeSetting->algorithm_mode;
	else
		stAlgorithm.uArgument1 = CipherSettingValue.multi2Rounds = CipherModeSetting->multi2Rounds;

	DEBUG_PRT("%s %d uAlgorithm = %d uOperationMode = %d algorithm_mode= %d \n",
		__func__, __LINE__, stAlgorithm.uAlgorithm , stAlgorithm.uOperationMode, stAlgorithm.uArgument1);
#else
	stAlgorithm.uResidual = 0;
	stAlgorithm.uSmsg = 1;//0;

	DEBUG_PRT("%s %d uAlgorithm = %d uOperationMode = %d\n",
		__func__, __LINE__, stAlgorithm.uAlgorithm , stAlgorithm.uOperationMode);
#endif
	stAlgorithm.uDemuxId =0;

	if(ioctl(Cipher_Handle, TCC_CHIPHER_IOCTL_SET_ALGORITHM, &stAlgorithm) != 0)
	{
		DEBUG_PRT("Cipher Error: TCC_CHIPHER_IOCTL_SET_ALGORITHM\n");
#ifdef USE_MUTEX
		pthread_mutex_unlock(&CipherMutex);
#endif
		return -1;
	}
#ifdef USE_MUTEX
	pthread_mutex_unlock(&CipherMutex);
#endif

#endif
	return 0;
}

#if 0   // sunny : not use.
int HWDEMUX_Cipher_SetMode(TCC_CIPHER_ModeSettings *CipherModeSetting)
{
#ifndef WIN32
	stHWDEMUXCIPHER_ALGORITHM stAlgorithm;
	DEBUG_PRT("[%s][%d] \n", Cipher_Handle, __func__, __LINE__);

    if (CipherModeSetting == NULL)
	{
		DEBUG_PRT("[%s] CipherModeSetting is NULL.\n", __func__);
		return -1;
	}

	stAlgorithm.uAlgorithm = CipherSettingValue.algorithm = CipherModeSetting->algorithm;
	stAlgorithm.uOperationMode = CipherSettingValue.operation_mode= CipherModeSetting->operation_mode;

	stAlgorithm.uResidual = 4;
	stAlgorithm.uSmsg = 1;//2;
	stAlgorithm.uDemuxId = 0;

	if(ioctl(Cipher_Handle, IOCTL_DXB_CTRL_HWDEMUX_CIPHER_SET_ALGORITHM, &stAlgorithm) != 0)
	{
		DEBUG_PRT("Cipher Error: TCC_CHIPHER_IOCTL_SET_ALGORITHM\n");
		return -1;
	}
#endif
	return 0;
}
#endif

void Cipher_Swap(unsigned char *srcdata, unsigned char *destdata, int size)
{
	int		i;
	for(i=0;i<size; i+=4)
	{
		destdata[i] = srcdata[i+3];
		destdata[i+1] = srcdata[i+2];
		destdata[i+2] = srcdata[i+1];
		destdata[i+3] = srcdata[i];
	}
}

int Cipher_SetKey(TCC_CIPHER_KeySettings *Keysetting, unsigned int DSC_MODE)
{
#ifndef WIN32
	if(Cipher_Handle<0)
	{
		DEBUG_PRT("%s %d Cipher Error: Cipher handel  = %d error\n", __func__, __LINE__, Cipher_Handle);
		return -1;
	}

	DEBUG_PRT("%s %d \n", __func__, __LINE__);

    if (Keysetting == NULL)
	{
		DEBUG_PRT("[%s] Keysetting is NULL.\n", __func__);
		return -1;
	}

#ifdef USE_MUTEX
	pthread_mutex_lock(&CipherMutex);
#endif

	if(Keysetting->Key_flag == EVEN_KEY_TYPE)
	{
		if(stOldEvenKey.uLength != 0)
			memcpy(ucOldEvenKey_Data, &stEvenKey.pucData[0], ISDBT_ECM_KEY_LEN);

		if(DSC_MODE == 0)
		{	
			Cipher_Swap(Keysetting->p_key_data, ucEvenKEY_SwapData, Keysetting->key_length);
			stEvenKey.pucData = ucEvenKEY_SwapData;
		}
		else
			stEvenKey.pucData = Keysetting->p_key_data;
			
		stEvenKey.uLength = Keysetting->key_length;
		stEvenKey.uOption = 0;

		if(stOldEvenKey.uLength == 0)
		{	
			stOldEvenKey.uLength = stEvenKey.uLength ;
			memcpy(ucOldEvenKey_Data, &stEvenKey.pucData[0], ISDBT_ECM_KEY_LEN);
		}
		uiEvenKeyChange =0;
		
		DEBUG_PRT("Even_Key: %x:%x:%x:%x:%x:%x:%x:%x\n", stEvenKey.pucData[0],stEvenKey.pucData[1]
						,stEvenKey.pucData[2],stEvenKey.pucData[3]
						,stEvenKey.pucData[4],stEvenKey.pucData[5]
						,stEvenKey.pucData[6],stEvenKey.pucData[7]);
		
		DEBUG_PRT("OldEven_Key: %x:%x:%x:%x:%x:%x:%x:%x\n", ucOldEvenKey_Data[0],ucOldEvenKey_Data[1]
						,ucOldEvenKey_Data[2],ucOldEvenKey_Data[3]
						,ucOldEvenKey_Data[4],ucOldEvenKey_Data[5]
						,ucOldEvenKey_Data[6],ucOldEvenKey_Data[7]);
	}
	else if(Keysetting->Key_flag == ODD_KEY_TYPE)
	{
		if(stOldOddKey.uLength != 0)
			memcpy(ucOldOddKey_Data,&stOddKey.pucData[0], ISDBT_ECM_KEY_LEN);

		if(DSC_MODE == 0)
		{	
			Cipher_Swap(Keysetting->p_key_data, ucOddKEY_SwapData, Keysetting->key_length);
			stOddKey.pucData = ucOddKEY_SwapData;
		}
		else
			stOddKey.pucData = Keysetting->p_key_data;
			
		stOddKey.uLength = Keysetting->key_length;
		stOddKey.uOption = 0;

		if(stOldOddKey.uLength == 0)
		{	
			stOldOddKey.uLength = stOddKey.uLength;
			memcpy(ucOldOddKey_Data,&stOddKey.pucData[0], ISDBT_ECM_KEY_LEN);
		}
		uiOddKeyChange =0;
		
		DEBUG_PRT("Odd_Key: %x:%x:%x:%x:%x:%x:%x:%x\n", stOddKey.pucData[0],stOddKey.pucData[1]
						,stOddKey.pucData[2],stOddKey.pucData[3]
						,stOddKey.pucData[4],stOddKey.pucData[5]
						,stOddKey.pucData[6],stOddKey.pucData[7]);
		
		DEBUG_PRT("OldOdd_Key: %x:%x:%x:%x:%x:%x:%x:%x\n", ucOldOddKey_Data[0],ucOldOddKey_Data[1]
						,ucOldOddKey_Data[2],ucOldOddKey_Data[3]
						,ucOldOddKey_Data[4],ucOldOddKey_Data[5]
						,ucOldOddKey_Data[6],ucOldOddKey_Data[7]);

	}

	if(Keysetting->vector_length>0)
	{	
		if(DSC_MODE == 0)
		{	
			Cipher_Swap(Keysetting->p_init_vector, ucIV_SwapData, Keysetting->vector_length);
			stVector.pucData = ucIV_SwapData;
		}
		else
			stVector.pucData = Keysetting->p_init_vector;
			
		stVector.uLength = Keysetting->vector_length;
	}

	if(CipherSettingValue.algorithm == TCC_CHIPHER_ALGORITM_MULTI2 ||CipherSettingValue.algorithm == TCC_CHIPHER_ALGORITM_MULTI2_1)
	{
		if(DSC_MODE == 0)
		{	
			Cipher_Swap(Keysetting->p_Systemkey_data, ucSystem_SwapData, Keysetting->Systemkey_length);
			stSystemkey.pucData = ucSystem_SwapData;
		}
		else
			stSystemkey.pucData = Keysetting->p_Systemkey_data;

		stSystemkey.uLength = Keysetting->Systemkey_length;
		stSystemkey.uOption = TCC_CHIPHER_KEY_MULTI2_SYSTEM;
		stEvenKey.uOption = TCC_CHIPHER_KEY_MULTI2_DATA;
		stOddKey.uOption = TCC_CHIPHER_KEY_MULTI2_DATA;		
	}
	if(stEvenKey.uLength != 0 && stOddKey.uLength != 0)
		Key_SefFlag =1;
	
#ifdef USE_MUTEX
	pthread_mutex_unlock(&CipherMutex);
#endif

#endif
	return 0;
}


int Cipher_SetBlock(int kye_type)
{
#ifndef WIN32
	stCIPHER_KEY 	stUsed_EvenKey;
	stCIPHER_KEY 	stUsed_OddKey;

	if(Cipher_Handle<0)
	{
		DEBUG_PRT("%s %d Cipher Error: Cipher handel error\n", __func__, __LINE__);
		return -1;
	}

	if(Key_SefFlag != 1)
	{
		DEBUG_PRT("%s %d Cipher Error: Key Set Error, Not Yet Set\n", __func__, __LINE__);
		return -1;
	}
#ifdef USE_MUTEX
	pthread_mutex_lock(&CipherMutex);
#endif

	if(Cipher_Keytype != kye_type || uiEvenKeyChange == 1 || uiOddKeyChange == 1)
	{
		if(uiEvenKeyChange == 1)
			uiEvenKeyChange = 0;
		if(uiOddKeyChange == 1)
			uiOddKeyChange =0;

		DEBUG_PRT("kye_type  = 0x%x\n", kye_type);

		if(kye_type == EVEN_KEY_TYPE)
		{
			stUsed_EvenKey = stEvenKey;
				
			if(ioctl(Cipher_Handle, TCC_CHIPHER_IOCTL_SET_KEY, &stUsed_EvenKey) != 0)
			{
				DEBUG_PRT("Cipher Error: TCC_CHIPHER_IOCTL_SET_KEY, option=%d\n", stEvenKey.uOption);
#ifdef USE_MUTEX
				pthread_mutex_unlock(&CipherMutex);
#endif
				return -1;
			}
		}
		else if(kye_type == ODD_KEY_TYPE)
		{
			stUsed_OddKey = stOddKey;

			if(ioctl(Cipher_Handle, TCC_CHIPHER_IOCTL_SET_KEY, &stUsed_OddKey) != 0)
			{
				DEBUG_PRT("Cipher Error: TCC_CHIPHER_IOCTL_SET_KEY, option=%d\n", stOddKey.uOption);
#ifdef USE_MUTEX
				pthread_mutex_unlock(&CipherMutex);
#endif
				return -1;
			}
		}		

		if(stVector.uLength !=0)
		{
			if(ioctl(Cipher_Handle, TCC_CHIPHER_IOCTL_SET_VECTOR, &stVector))
			{
				DEBUG_PRT("Cipher Error: TCC_CHIPHER_IOCTL_SET_VECTOR\n");
#ifdef USE_MUTEX
				pthread_mutex_unlock(&CipherMutex);
#endif
				return -1;
			}
		}
		if(CipherSettingValue.algorithm ==TCC_CHIPHER_ALGORITM_MULTI2 ||CipherSettingValue.algorithm == TCC_CHIPHER_ALGORITM_MULTI2_1)
		{
			if(ioctl(Cipher_Handle, TCC_CHIPHER_IOCTL_SET_KEY, &stSystemkey) != 0)
			{
				DEBUG_PRT("Cipher Error: TCC_CHIPHER_IOCTL_SET_KEY, option=%d\n", stSystemkey.uOption);
#ifdef USE_MUTEX
				pthread_mutex_unlock(&CipherMutex);
#endif
				return -1;
			}
		}
		Cipher_Keytype = kye_type;

		
	}
#ifdef USE_MUTEX
	pthread_mutex_unlock(&CipherMutex);
#endif

#endif
	return 0;
}

int HWDEMUX_Cipher_SetBlock(int kye_type)
{
#ifndef WIN32
	stHWDEMUXCIPHER_KEY 		stHWDEMUXEvenKey;
	stHWDEMUXCIPHER_KEY 		stHWDEMUXOddKey;
	stHWDEMUXCIPHER_VECTOR 	stHWDEMUXVector;
	stHWDEMUXCIPHER_SYSTEMKEY	stHWDEMUXSystemkey;

	if(Cipher_Handle<0)
	{
		DEBUG_PRT("%s %d Cipher Error: Cipher handel error\n", __func__, __LINE__);
		return -1;
	}

	if(Key_SefFlag != 1)
	{
		DEBUG_PRT("%s %d Cipher Error: Key Set Error, Not Yet Set\n", __func__, __LINE__);
		return -1;
	}

	DEBUG_PRT("Cipher_Keytype = %d,  kye_type = %d, [%s][%d] \n",Cipher_Keytype, kye_type, __func__, __LINE__);
#ifdef USE_MUTEX
	pthread_mutex_lock(&CipherMutex);
#endif
	if(Cipher_Keytype != kye_type)
	{
		if(stVector.uLength !=0)
		{
			stHWDEMUXVector.uDemuxId = 0;
			stHWDEMUXVector.uLength = stVector.uLength;
			stHWDEMUXVector.pucData =  stVector.pucData;
			stHWDEMUXVector.uOption =  1;

			if(ioctl(Cipher_Handle, IOCTL_DXB_CTRL_HWDEMUX_CIPHER_SET_VECTOR, &stHWDEMUXVector) != 0)
			{
				DEBUG_PRT("Cipher Error: TCC_CHIPHER_IOCTL_SET_VECTOR\n");
	#ifdef USE_MUTEX
			pthread_mutex_unlock(&CipherMutex);
	#endif
				return -1;
			}
		}
		DEBUG_PRT("kye_type = %d,  [%s][%d] \n", kye_type, __func__, __LINE__);

		if(kye_type == EVEN_KEY_TYPE)
		{
			stHWDEMUXEvenKey.uDemuxId = 0;
			stHWDEMUXEvenKey.uKeyType = EVEN_KEY_TYPE;
			stHWDEMUXEvenKey.uLength = stEvenKey.uLength;
			stHWDEMUXEvenKey.uKeyMode = 0;
			stHWDEMUXEvenKey.pucData = stEvenKey.pucData;

			if(ioctl(Cipher_Handle, IOCTL_DXB_CTRL_HWDEMUX_CIPHER_SET_KEY, &stHWDEMUXEvenKey) != 0)
			{
				DEBUG_PRT("Cipher Error: TCC_CHIPHER_IOCTL_SET_KEY, option=%d\n", stHWDEMUXEvenKey.uKeyType);
		#ifdef USE_MUTEX
				pthread_mutex_unlock(&CipherMutex);
		#endif
				return -1;
			}
		}
		else if(kye_type == ODD_KEY_TYPE)
		{
			stHWDEMUXOddKey.uDemuxId = 0;
			stHWDEMUXOddKey.uKeyType = ODD_KEY_TYPE;
			stHWDEMUXOddKey.uLength = stOddKey.uLength;
			stHWDEMUXOddKey.uKeyMode = 0;
			stHWDEMUXOddKey.pucData = stOddKey.pucData;

			if(ioctl(Cipher_Handle, IOCTL_DXB_CTRL_HWDEMUX_CIPHER_SET_KEY, &stHWDEMUXOddKey) != 0)
			{
				DEBUG_PRT("Cipher Error: TCC_CHIPHER_IOCTL_SET_KEY, option=%d\n", stHWDEMUXOddKey.uKeyType);
		#ifdef USE_MUTEX
				pthread_mutex_unlock(&CipherMutex);
		#endif
				return -1;
			}
		}

		if(CipherSettingValue.algorithm ==TCC_CHIPHER_ALGORITM_MULTI2 ||CipherSettingValue.algorithm == TCC_CHIPHER_ALGORITM_MULTI2_1)
		{
			if(stSystemkey.uLength>0)
			{	
				stHWDEMUXSystemkey.uDemuxId = 0;
				stHWDEMUXSystemkey.uKeyType = stSystemkey.uOption;
				stHWDEMUXSystemkey.uLength = stSystemkey.uLength;
				stHWDEMUXSystemkey.uKeyMode = 1;
				stHWDEMUXSystemkey.pucData = stSystemkey.pucData;

				if(ioctl(Cipher_Handle, IOCTL_DXB_CTRL_HWDEMUX_CIPHER_SET_KEY, &stHWDEMUXSystemkey) != 0)
				{
					DEBUG_PRT("Cipher Error: TCC_CHIPHER_IOCTL_SET_KEY, option=%d\n", stHWDEMUXSystemkey.uKeyType);
			#ifdef USE_MUTEX
					pthread_mutex_unlock(&CipherMutex);
			#endif
					return -1;
				}
			}
		}
		Cipher_Keytype = kye_type;
	}
#ifdef USE_MUTEX
	pthread_mutex_unlock(&CipherMutex);
#endif

#endif
	return 0;
}

int Cipher_Get_Blocksize(void)
{
#ifndef WIN32
	if(CipherSettingValue.algorithm == TCC_CHIPHER_ALGORITM_DES ||CipherSettingValue.algorithm == TCC_CHIPHER_ALGORITM_MULTI2
		||CipherSettingValue.algorithm == TCC_CHIPHER_ALGORITM_MULTI2_1)
	{
		return  BLOCK_SIZE_OTHERS;
	}
	else if(CipherSettingValue.algorithm == TCC_CHIPHER_ALGORITM_AES)
	{
		return BLOCK_SIZE_AES;
	}
#endif
	return 0;
}

int Cipher_Get_Handle(void)
{
	return Cipher_Handle;
}


int Cipher_Get_AlgorithmType(void)
{
#ifndef WIN32
	if(Cipher_Handle > 0)
	{
		return CipherSettingValue.algorithm;
	}	
#endif
	return 0;
}

int Cipher_Run(unsigned char* ucSrcData, unsigned char * ucDstData1, unsigned int Src_length, unsigned int mode)
{
#ifndef WIN32
	stCIPHER_ENCRYPTION stEncryption;

	if(Cipher_Handle < 0)
	{
		DEBUG_PRT("%s %d Cipher Error: Cipher handel error\n", __func__, __LINE__);
		return -1;
	}
#ifdef USE_MUTEX
	pthread_mutex_lock(&CipherMutex);
#endif

	stEncryption.pucSrcAddr = ucSrcData;
	stEncryption.pucDstAddr = ucDstData1;
	stEncryption.uLength = Src_length;
		
	if(ioctl(Cipher_Handle, mode, &stEncryption) != 0)
	{
		DEBUG_PRT("Cipher Error: TCCDEMUX_CHIPHER_IOCTL_ENCRYPT\n");
#ifdef USE_MUTEX
		pthread_mutex_unlock(&CipherMutex);
#endif
		return -1;
	}
#ifdef USE_MUTEX
	pthread_mutex_unlock(&CipherMutex);
#endif

#endif

	return 0;
}
