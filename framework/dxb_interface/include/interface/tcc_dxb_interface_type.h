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
#ifndef _TCC_DXB_INTERFACE_TYPE_H_
#define _TCC_DXB_INTERFACE_TYPE_H_

/* 8 bit integer types */
typedef	signed char				INT8;
typedef	unsigned char			UINT8;

/* 16 bit integer types */
typedef	signed short			INT16;
typedef	unsigned short			UINT16;

/* 32 bit integer types */
typedef	int						INT32;
typedef	unsigned int			UINT32;


typedef float					REAL32;

/* 64 bit integer types */
typedef signed long long		INT64;
typedef unsigned long long		UINT64;

/* boolean type */
typedef	int			BOOLEAN;

#ifndef TRUE
#define TRUE 1
#endif


#ifndef FALSE
#define	FALSE 0
#endif

#ifndef NULL
#define  NULL 0
#endif

typedef enum DxB_ERR_CODE
{
	DxB_ERR_OK= 0,
	DxB_ERR_ERROR,
	DxB_ERR_INVALID_PARAM,
	DxB_ERR_NO_RESOURCE,
	DxB_ERR_NO_ALLOC,
	DxB_ERR_NOT_SUPPORTED,
	DxB_ERR_INIT,
	DxB_ERR_SEND,
	DxB_ERR_RECV,
	DxB_ERR_TIMEOUT,
	DxB_ERR_CRC
}DxB_ERR_CODE;

typedef enum DxB_EVENT_ERR_CODE
{
    DxB_EVENT_ERR_OK= 0,
    DxB_EVENT_ERR_ERR,
    DxB_EVENT_ERR_NOFREESPACE,
    DxB_EVENT_ERR_FILEOPENERROR,
    DxB_EVENT_ERR_INVALIDMEMORY,
    DxB_EVENT_ERR_INVALIDOPERATION,
    DxB_EVENT_FAIL_NOFREESPACE,
    DxB_EVENT_FAIL_INVALIDSTORAGE,
    DxB_EVENT_ERR_MAX_SIZE,
    DxB_EVENT_ERR_MAX,
}DxB_EVENT_ERR_CODE;

typedef	enum DxB_STANDARDS
{
   DxB_STANDARD_IPTV = 0,
   DxB_STANDARD_ISDBT,
   DxB_STANDARD_TDMB,
   DxB_STANDARD_DVBT,
   DxB_STARDARD_DVBS,
   DxB_STANDARD_ATSC,
   DxB_STANDARD_DAB,
   DxB_STANDARD_MAX
}DxB_STANDARDS;

typedef struct
{
	int iTunerType;
	void *pOpenMaxIL;
	void *pTunerPrivate;
	void *pDemuxPrivate;
	void *pVideoPrivate;
	void *pAudioPrivate;
	void *pPVRPrivate;
} DxBInterface;

#endif


