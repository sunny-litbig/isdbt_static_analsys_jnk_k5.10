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
#ifndef	_TCC_ISDBT_MANAGER_CAS_H_
#define	_TCC_ISDBT_MANAGER_CAS_H_

#include "tcc_trmp_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ISDBT_CAS_SUCCESS					0
#define ISDBT_CAS_ERROR						(-1)
#define ISDBT_CAS_ERROR_UNSUPPORTED			(-2)
#define ISDBT_CAS_ERROR_INVALID_PARAM		(-3)
#define ISDBT_CAS_ERROR_QUEUE_FULL			(-4)
#define ISDBT_CAS_ERROR_CIPHER				(-5)
#define ISDBT_CAS_ERROR_SMART_CARD			(-6)
#define ISDBT_CAS_ERROR_TRMP_MODULE			(-7)

#define ISDBT_CAS_CIPHER_TYPE_MASK 			(7<<1)		/* 1=built-in cipher block, 2=s/w, 4=tcc353x, 0=not defined */
#define ISDBT_CAS_CIPHER_HW					(1<<1)
#define ISDBT_CAS_CIPHER_SW					(2<<1)
#define ISDBT_CAS_CIPHER_TCC353X			(4<<1)
#define ISDBT_CAS_CIPHER_HWDEMUX			(8<<1)

#define ISDBT_CAS_SECTION_HEADER_LENGTH		8
#define ISDBT_CAS_SECTION_CRC_LENGTH		4
#define	ISDBT_CAS_SECTION_QUEUESIZE			1000

#define ISDBT_CA_SYSTEM_ID_CA5				0x0005
#define ISDBT_CA_SYSTEM_ID_RMP						0x000E

#define ISDBT_TRANSMISSION_TYPE_RMP					0x7

#define ISDBT_CAS_EMM_TIME_LIMITE			30

extern int tcc_manager_cas_pushSectionData(void *data);
extern int tcc_manager_cas_init(void);
extern int tcc_manager_cas_deinit(void);
extern int tcc_manager_cas_reset(void);
extern int tcc_manager_cas_resetInfo(void);
//extern int tcc_manager_cas_setDeviceKeyUpdateFunction(updateDeviceKey func);
extern int tcc_manager_cas_getInfo(void **ppInfo, int *pInfoSize);
extern int tcc_manager_cas_getCipherType (void);

#ifdef __cplusplus
}
#endif

#endif
