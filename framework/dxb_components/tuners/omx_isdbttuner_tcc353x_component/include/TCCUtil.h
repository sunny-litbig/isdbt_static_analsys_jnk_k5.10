/*******************************************************************************

*   FileName : TCCUtil.h

*   Copyright (c) Telechips Inc.

*   Description : TCCUtil.h

********************************************************************************
*
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

#ifndef	_TCC_UTIL_H__
#define	_TCC_UTIL_H__

/******************************************************************************
* include 
******************************************************************************/
#include "globals.h"

/******************************************************************************
* typedefs & structure
******************************************************************************/
typedef unsigned char	U8;
typedef unsigned short	U16;
typedef unsigned int	U32;

/******************************************************************************
* defines 
******************************************************************************/

#ifndef BITSET
#define	BITSET(X, MASK)				( (X) |= (U32)(MASK) )
#endif

#ifndef BITSET
#define	BITSET(X, MASK)				( (X) |= (U32)(MASK) )
#endif
#ifndef BITSCLR
#define	BITSCLR(X, SMASK, CMASK)	( (X) = ((((U32)(X)) | ((U32)(SMASK))) & ~((U32)(CMASK))) )
#endif
#ifndef BITCSET
#define	BITCSET(X, CMASK, SMASK)	( (X) = ((((U32)(X)) & ~((U32)(CMASK))) | ((U32)(SMASK))) )
#endif
#ifndef BITCLR
#define	BITCLR(X, MASK)				( (X) &= ~((U32)(MASK)) )
#endif
#ifndef BITXOR
#define	BITXOR(X, MASK)				( (X) ^= (U32)(MASK) )
#endif
#ifndef ISZERO
#define	ISZERO(X, MASK)				(  ! (((U32)(X)) & ((U32)(MASK))) )
#endif
#ifndef BYTE_OF
#define	BYTE_OF(X)					( *(volatile unsigned char *)(&(X)) )
#endif
#ifndef HWORD_OF
#define	HWORD_OF(X)					( *(volatile unsigned short *)(&(X)) )
#endif
#ifndef WORD_OF
#define	WORD_OF(X)					( *(volatile unsigned int *)(&(X)) )
#endif

#ifndef MCHw1
#define MCHw1(x)  ( (0x00000001<<(x)))
#endif

#ifndef MCHw0
#define	MCHw0(x)  (~(0x00000001<<(x)))
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



#endif //_TCC_UTIL_H___
