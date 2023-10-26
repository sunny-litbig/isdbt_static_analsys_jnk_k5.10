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
#ifndef	_TCC_DXB_INTERFACE_CIPHER_H_
#define	_TCC_DXB_INTERFACE_CIPHER_H_

/***************************************************************
Macro definition
***************************************************************/


/***************************************************************
Type definition
***************************************************************/
/* CIPHER IOCTL Command */
#ifdef CONFIG_ARCH_TCC897X
enum
{
	TCC_CHIPHER_IOCTL_SET_ALGORITHM = 0x100,
	TCC_CHIPHER_IOCTL_SET_KEY,
	TCC_CHIPHER_IOCTL_SET_VECTOR,
	TCC_CHIPHER_IOCTL_ENCRYPT,
	TCC_CHIPHER_IOCTL_DECRYPT,
	TCC_CHIPHER_IOCTL_CLOSE,
	TCC_CHIPHER_IOCTL_MAX,
};

/* The Algorithm of the cipher block */
enum TCC_CHIPHER_ALGORITHM_TYPE
{
	TCC_CHIPHER_ALGORITM_AES = 0,
	TCC_CHIPHER_ALGORITM_DES,
	TCC_CHIPHER_ALGORITM_MULTI2,
	TCC_CHIPHER_ALGORITM_MULTI2_1,
	TCC_CHIPHER_ALGORITM_CSA,
	TCC_CHIPHER_ALGORITM_MAX
};

/* HWDEMUX Cipher algorithm*/
enum HWDEMUX_CHIPHER_ALGORITHM_TYPE
{
	HWDEMUX_ALGORITHM_DES = 0,
	HWDEMUX_ALGORITHM_CSA,
	HWDEMUX_ALGORITHM_MULTI2,
	HWDEMUX_ALGORITHM_AES,
	HWDEMUX_ALGORITHM_MAX
};

/* The Algorithm of the operation mode */
enum TCC_CHIPHER_OPERATION_MODE_TYPE
{
	TCC_CHIPHER_OPMODE_ECB = 0,
	TCC_CHIPHER_OPMODE_CBC,
	TCC_CHIPHER_OPMODE_CFB,
	TCC_CHIPHER_OPMODE_OFB,
	TCC_CHIPHER_OPMODE_CTR,
	TCC_CHIPHER_OPMODE_CTR_1,
	TCC_CHIPHER_OPMODE_CTR_2,
	TCC_CHIPHER_OPMODE_CTR_3,
	TCC_CHIPHER_OPMODE_MAX
};

/* The Operation Mode */ 
enum HWDEMUX_CHIPHER_OPERATION_MODE_TYPE
{
	HWDEMUX_CIPHER_OPMODE_CBC = 0,
	HWDEMUX_CIPHER_OPMODE_ECB,
	HWDEMUX_CIPHER_OPMODE_CFB,
	HWDEMUX_CIPHER_OPMODE_OFB,
	HWDEMUX_CIPHER_OPMODE_CTR,
	HWDEMUX_CIPHER_OPMODE_CTR_1,
	HWDEMUX_CIPHER_OPMODE_CTR_2,
	HWDEMUX_CIPHER_OPMODE_CTR_3,
	HWDEMUX_CIPHER_OPMODE_MAX
};


/* The Key Option for AES */ 
enum TCC_CHIPHER_AES_MODE_TYPE
{
	TCC_CHIPHER_AES_128KEY =0,
	TCC_CHIPHER_AES_192KEY,
	TCC_CHIPHER_AES_256KEY_1,
	TCC_CHIPHER_AES_256KEY_2,
	TCC_CHIPHER_AES_KEYMAX,
};

/* The Key Option for DES */ 
enum TCC_CHIPHER_DES_MODE_TYPE
{
	TCC_CHIPHER_DES_SINGLE =0,
	TCC_CHIPHER_DES_DOUBLE,
	TCC_CHIPHER_DES_TRIPLE_2KEY,
	TCC_CHIPHER_DES_TRIPLE_3KEY,
	TCC_CHIPHER_DES_KEYMAX,
};

/* The Parity bit Option for DES */ 
enum TCC_CHIPHER_DES_PARITY_TYPE
{
	TCC_CHIPHER_DES_LSB_PRT =0,
	TCC_CHIPHER_DES_MSB_PRT,
	TCC_CHIPHER_DES_PARITYMAX,
};

/* The Key Option for Multi2 */ 
enum TCC_CHIPHER_MILTI2_KEY_TYPE
{
	TCC_CHIPHER_KEY_MULTI2_DATA = 0,
	TCC_CHIPHER_KEY_MULTI2_SYSTEM,
	TCC_CHIPHER_KEY_MULTI2_MAX
};

/* HWDEMUX The Key Option */ 
enum
{
	HWDEMUX_KEY_DATA = 0,
	HWDEMUX_KEY_MULTI2_SYSTEM,
	HWDEMUX_KEY_EVEN,
	HWDEMUX_KEY_ODD,
	HWDEMUX_KEY_MAX
};

/* DMA TYPE */ 
enum
{
	TCC_CIPHER_DMA_INTERNAL = 0,
	TCC_CIPHER_DMA_DMAX,
	TCC_CIPHER_DMA_MAX
};


#elif defined(CONFIG_ARCH_TCC803X)

enum
{
	TCC_CHIPHER_IOCTL_SET_ALGORITHM = 0x100,
	TCC_CHIPHER_IOCTL_SET_KEY,
	TCC_CHIPHER_IOCTL_SET_VECTOR,
	TCC_CHIPHER_IOCTL_ENCRYPT,
	TCC_CHIPHER_IOCTL_DECRYPT,
	TCC_CHIPHER_IOCTL_CLOSE,
	TCC_CHIPHER_IOCTL_MAX,
};

/* The Algorithm of the cipher block */
enum TCC_CHIPHER_ALGORITHM_TYPE
{
	TCC_CHIPHER_ALGORITM_MULTI2 = 6,
	TCC_CHIPHER_ALGORITM_AES,
	TCC_CHIPHER_ALGORITM_DES,
	TCC_CHIPHER_ALGORITM_MULTI2_1,
	TCC_CHIPHER_ALGORITM_CSA,
	TCC_CHIPHER_ALGORITM_MAX
};

/* HWDEMUX Cipher algorithm*/
enum HWDEMUX_CHIPHER_ALGORITHM_TYPE
{
	HWDEMUX_ALGORITHM_MULTI2 = 6,
	HWDEMUX_ALGORITHM_DES,
	HWDEMUX_ALGORITHM_CSA,
	HWDEMUX_ALGORITHM_AES,
	HWDEMUX_ALGORITHM_MAX
};

/* The Algorithm of the operation mode */
enum TCC_CHIPHER_OPERATION_MODE_TYPE
{
	TCC_CHIPHER_OPMODE_ECB = 0,
	TCC_CHIPHER_OPMODE_CBC,
	TCC_CHIPHER_OPMODE_CFB,
	TCC_CHIPHER_OPMODE_OFB,
	TCC_CHIPHER_OPMODE_CTR,
	TCC_CHIPHER_OPMODE_CTR_1,
	TCC_CHIPHER_OPMODE_CTR_2,
	TCC_CHIPHER_OPMODE_CTR_3,
	TCC_CHIPHER_OPMODE_MAX
};

/* The Operation Mode */
enum HWDEMUX_CHIPHER_OPERATION_MODE_TYPE
{
	HWDEMUX_CIPHER_OPMODE_ECB = 0,
	HWDEMUX_CIPHER_OPMODE_CBC,
	HWDEMUX_CIPHER_OPMODE_CFB,
	HWDEMUX_CIPHER_OPMODE_OFB,
	HWDEMUX_CIPHER_OPMODE_CTR,
	HWDEMUX_CIPHER_OPMODE_CTR_1,
	HWDEMUX_CIPHER_OPMODE_CTR_2,
	HWDEMUX_CIPHER_OPMODE_CTR_3,
	HWDEMUX_CIPHER_OPMODE_MAX
};


/* The Key Option for AES */
enum TCC_CHIPHER_AES_MODE_TYPE
{
	TCC_CHIPHER_AES_128KEY =0,
	TCC_CHIPHER_AES_192KEY,
	TCC_CHIPHER_AES_256KEY_1,
	TCC_CHIPHER_AES_256KEY_2,
	TCC_CHIPHER_AES_KEYMAX,
};

/* The Key Option for DES */
enum TCC_CHIPHER_DES_MODE_TYPE
{
	TCC_CHIPHER_DES_SINGLE =0,
	TCC_CHIPHER_DES_DOUBLE,
	TCC_CHIPHER_DES_TRIPLE_2KEY,
	TCC_CHIPHER_DES_TRIPLE_3KEY,
	TCC_CHIPHER_DES_KEYMAX,
};

/* The Parity bit Option for DES */
enum TCC_CHIPHER_DES_PARITY_TYPE
{
	TCC_CHIPHER_DES_LSB_PRT =0,
	TCC_CHIPHER_DES_MSB_PRT,
	TCC_CHIPHER_DES_PARITYMAX,
};

/* The Key Option for Multi2 */
enum TCC_CHIPHER_MILTI2_KEY_TYPE
{
	TCC_CHIPHER_KEY_MULTI2_DATA = 0,
	TCC_CHIPHER_KEY_MULTI2_SYSTEM,
	TCC_CHIPHER_KEY_MULTI2_MAX
};

/* HWDEMUX The Key Option */
enum
{
	HWDEMUX_KEY_DATA = 0,
	HWDEMUX_KEY_MULTI2_SYSTEM,
	HWDEMUX_KEY_EVEN,
	HWDEMUX_KEY_ODD,
	HWDEMUX_KEY_MAX
};

/* DMA TYPE */
enum
{
	TCC_CIPHER_DMA_INTERNAL = 0,
	TCC_CIPHER_DMA_DMAX,
	TCC_CIPHER_DMA_MAX
};

#endif
typedef struct
{
	unsigned int  uDmaMode;
	unsigned int	uOperationMode;
	unsigned int	uAlgorithm;
	unsigned int	uArgument1;
	unsigned int	uArgument2;
} stCIPHER_ALGORITHM;

typedef struct
{
	unsigned char	 *pucData;
	unsigned int		 uLength;
	unsigned int		 uOption;
} stCIPHER_KEY;


typedef struct
{
	unsigned char	 *pucData;
	unsigned int		 uLength;
	unsigned int		 uOption;
} stCIPHER_SYSTEMKEY;


typedef struct
{
	unsigned char	 *pucData;
	unsigned int		 uLength;
} stCIPHER_VECTOR;


typedef struct
{
	unsigned char 	*pucSrcAddr;
	unsigned char 	*pucDstAddr;
	unsigned int		uLength;
} stCIPHER_ENCRYPTION;

#if 1		//for HWDEMUX Cipher
typedef struct
{
	unsigned int	uDemuxId;
	unsigned int	uOperationMode;			//op Mode (ex: ecb,cbc,...)
	unsigned int	uAlgorithm;				//algorithm (ex: AES,DES,..)
	unsigned int	uResidual;			// for "AES = Key length"& for "DES = des mode" & for "Multi2 = round Cnt"
	unsigned int	uSmsg;			// PRT (for DES : Select the parity bit location of the key)
} stHWDEMUXCIPHER_ALGORITHM;

typedef struct
{
	unsigned int	uDemuxId;
	unsigned char	 *pucData;
	unsigned int	uLength;
	unsigned int	uKeyType;
	unsigned int	uKeyMode;
} stHWDEMUXCIPHER_KEY;


typedef struct
{
	unsigned int	uDemuxId;
	unsigned char	 *pucData;
	unsigned int	 uLength;
	unsigned int	uKeyType;
	unsigned int	uKeyMode;
} stHWDEMUXCIPHER_SYSTEMKEY;


typedef struct
{
	unsigned int	uDemuxId;
	unsigned char	 *pucData;
	unsigned int		 uLength;
	unsigned 	int	uOption;			//(“0:Init Vector” or for “1:Residual Init Vector”)
} stHWDEMUXCIPHER_VECTOR;

typedef struct
{
	unsigned 	int	uDemuxId;
	unsigned 	int	uExecuteOption;		//("0:decryption", "1:encryption")
	unsigned int	uCWsel;
	unsigned int	uKLIdx;
	unsigned 	int	uKeyMode;		//("0”: HWDEMUX Cipher  not Used, "1”: HWDEMUX Cipher used.)
} stHWDEMUXCIPHER_EXECUTE;

typedef struct
{
	unsigned int		uDemuxId;
	unsigned char 	*pucSrcAddr;
	unsigned char 	*pucDstAddr;
	unsigned int		uLength;
} stHWDEMUXCIPHER_SENDDATA;
#endif

/**
algorithm			: AES, DES, Multi2 	refer to ALGORITHM_TYPE
operation_mode		: ECB, CBC, ... 		refer to OPERATION_MODE_TYPE
algorithm_mode		: AES		-> refer to AES_MODE_TYPE
						DES	-> refer to DES_MODE_TYPE
						Multi2 	-> not used
multi2Rounds		: Multi2		-> round_count (Variable)
						AES, DES  -> not  used
**/
typedef struct TCC_CIPHER_ModeSettings
{
	unsigned int		algorithm;		
	unsigned int		operation_mode;
	unsigned int		algorithm_mode;
	unsigned int		multi2Rounds;
} TCC_CIPHER_ModeSettings;
 
/**
p_key_data			: data key
key_length			: data key_length
p_init_vector		: initial vector 
vector_length		: vector length
p_Systemkey_data	: system key data (used to only multi2)
Systemkey_length	: system key_length(used to only multi2)
**/
 typedef struct TCC_CIPHER_KeySettings
 {
	 unsigned char *p_key_data;
	 unsigned int	 key_length;
	 unsigned char *p_init_vector;				 // 사용하지 않는 경우 NULL
	  unsigned int vector_length;				 // 사용하지 않는 경우 '0'
	 unsigned char *p_Systemkey_data;				 // 사용하지 않는 경우 NULL
	 unsigned int	 Systemkey_length;			 // 사용하지 않는 경우 '0'
	 unsigned int Key_flag;			 		// 10: even key, 11:odd key
} TCC_CIPHER_KeySettings;

/***************************************************************
Function definition
***************************************************************/
int Cipher_Open(TCC_CIPHER_ModeSettings *CipherModeSetting);
int Cipher_Close();
int Cipher_SetKey(TCC_CIPHER_KeySettings *Keysetting, unsigned int DSC_MODE);
int Cipher_SetMode(TCC_CIPHER_ModeSettings *CipherModeSetting);
int Cipher_Get_AlgorithmType(void);
int Cipher_Run(unsigned char* ucSrcData, unsigned char * ucDstData1, unsigned int Src_length, unsigned int mode);
int HWDEMUX_Cipher_Run(unsigned int execute);
int HWDEMUX_Cipher_Open(TCC_CIPHER_ModeSettings *CipherModeSetting);
int HWDEMUX_Cipher_Close();
int Cipher_Get_Handle(void);
int Cipher_Get_Blocksize(void);
int Cipher_SetBlock(int kye_type);
int HWDEMUX_Cipher_SetBlock(int kye_type);

#endif
