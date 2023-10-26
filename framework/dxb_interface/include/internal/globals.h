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
#ifndef	_GLOBALS_H__
#define	_GLOBALS_H__

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
* include 
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/******************************************************************************
* DXB defines 
******************************************************************************/
//#define   SUPPORT_TSFILE_PLAY
#ifdef	SUPPORT_TSFILE_PLAY
#define	TSFILE_NAME "/nand/2010-05-20-CADTV103-1301.tp"
#endif

#define SUPPORT_ALWAYS_IN_OMX_EXECUTING

/******************************************************************************
* typedefs & structure
******************************************************************************/

/******************************************************************************
* defines 
******************************************************************************/
#define	PRINTF_DEBUG
#ifdef PRINTF_DEBUG
#define	PRINTF ALOGD
#else
#define	PRINTF 
#endif


#ifndef TRUE
#define TRUE 1
#endif


#ifndef FALSE
#define	FALSE 0
#endif

#ifndef NULL
#define  NULL 0
#endif


/******************************************************************************
* globals
******************************************************************************/

/******************************************************************************
* locals
******************************************************************************/

/******************************************************************************
* declarations
******************************************************************************/

/************************************************************************************************
*										 MACRO												   *
************************************************************************************************/
#ifndef BITSET
#define BITSET(X, MASK) 			( (X) |= (unsigned int)(MASK) )
#endif
#ifndef BITSCLR
#define BITSCLR(X, SMASK, CMASK)	( (X) = ((((unsigned int)(X)) | ((unsigned int)(SMASK))) & ~((unsigned int)(CMASK))) )
#endif
#ifndef BITCSET
#define BITCSET(X, CMASK, SMASK)	( (X) = ((((unsigned int)(X)) & ~((unsigned int)(CMASK))) | ((unsigned int)(SMASK))) )
#endif
#ifndef BITCLR
#define BITCLR(X, MASK) 			( (X) &= ~((unsigned int)(MASK)) )
#endif
#ifndef BITXOR
#define BITXOR(X, MASK) 			( (X) ^= (unsigned int)(MASK) )
#endif
#ifndef ISZERO
#define ISZERO(X, MASK) 			(  ! (((unsigned int)(X)) & ((unsigned int)(MASK))) )
#endif
#ifndef	ISSET
#define	ISSET(X, MASK)			( (U32)(X) & ((U32)(MASK)) )
#endif

/************************************************************************************************
*										 ENUM												   *
************************************************************************************************/

/************************************************************************************************
ISDBT INFO SPECIFICATION
*************************************************************************************************
MSB																							  LSB
|31|30|29|28|27|26|25|24|23|22|21|20|19|18|17|16|15|14|13|12|11|10|09|08|07|06|05|04|03|02|01|00|
|  |  |  |  |           |                       |                       |                       |

Bit[31] : 13Seg
Bit[30] : 1Seg
Bit[29] : JAPAN
Bit[28] : BRAZIL
Bit[27:24] : reserved for ISDBT feature
Bit[23:16] : reserved for UI
Bit[15:05] : reserved
Bit[05:00] : Demod Band Info
	{
		Baseband_None =0,
		Baseband_TCC351x_CSPI_STS 	=1,
		Baseband_Dibcom =2,
		Baseband_TCC351x_I2C_STS 	=3,
		Baseband_NMI32x				=4,
		Baseband_TCC351x_I2C_SPIMS 	=5, //reserved
		Baseband_MTV818			= 6,
		Baseband_TOSHIBA		= 7,
		Baseband_Max =63,
	}		
*/

/* NOTICE
- Below variables are also used in tcc_isdbt_system.c (around line 84). 
*/

//information about profile
#define ISDBT_SINFO_PROFILE_A		0x80000000		//b'31 - support full seg
#define ISDBT_SINFO_PROFILE_C		0x40000000		//b'30 -support 1seg

//information about nation
#define ISDBT_SINFO_JAPAN			0x20000000		//b'29 - japan
#define ISDBT_SINFO_BRAZIL			0x10000000		//b'28 - brazil

#define ISDBT_BASEBAND_MASK		0x0000001f

typedef enum
{
	BASEBAND_NONE=0,
	BASEBAND_TCC351x_CSPI_STS	=1,
	BASEBAND_DIBCOM				=2,
	BASEBAND_TCC351x_I2C_STS	=3,
	BASEBAND_NMI32x				=4,
	BASEBAND_TCC351x_I2C_SPIMS	=5, //reserved
	BASEBAND_MTV818				=6,
	BASEBAND_TOSHIBA			=7,
	BASEBAND_TCC353x_CSPI_STS	= 8,
	BASEBAND_TCC353x_I2C_STS	= 9,
	BASEBAND_MAX=63,
}ISDBT_BASEBAND_TYPE;


//macros
#define IsdbtSInfo_IsSupportProfileA(X)		(((X) & ISDBT_SINFO_PROFILE_A) ? 1 : 0)
#define IsdbtSInfo_IsSupportProfileC(X)		(((X) & ISDBT_SINFO_PROFILE_C) ? 1 : 0)
#define IsdbtSInfo_IsForJapan(X)				(((X) & ISDBT_SINFO_JAPAN) ? 1 : 0)
#define IsdbtSInfo_IsForBrazil(X)				(((X) & ISDBT_SINFO_BRAZIL) ? 1 : 0)
#define IsdbtSInfo_IsBaseBandTCC351x_CSPI_STS(X)	((((X) & ISDBT_BASEBAND_MASK) == BASEBAND_TCC351x_CSPI_STS) ? 1 : 0)
#define IsdbtSInfo_IsBaseBandTCC351x_I2C_STS(X)	((((X) & ISDBT_BASEBAND_MASK) == BASEBAND_TCC351x_I2C_STS) ? 1 : 0)
#define IsdbtSInfo_IsBaseBandTCC351x_I2C_SPIMS(X)	((((X) & ISDBT_BASEBAND_MASK) == BASEBAND_TCC351x_I2C_SPIMS) ? 1 : 0)
#define IsdbtSInfo_IsBaseBandTCC353x_CSPI_STS(X)	((((X) & ISDBT_BASEBAND_MASK) == BASEBAND_TCC353x_CSPI_STS) ? 1 : 0)
#define IsdbtSInfo_IsBaseBandTCC353x_I2C_STS(X)	((((X) & ISDBT_BASEBAND_MASK) == BASEBAND_TCC353x_I2C_STS) ? 1 : 0)
#define IsdbtSInfo_GetBaseBand(X)			((X) & ISDBT_BASEBAND_MASK)

#ifdef __cplusplus
}
#endif

#endif // __GLOBALS_H__

