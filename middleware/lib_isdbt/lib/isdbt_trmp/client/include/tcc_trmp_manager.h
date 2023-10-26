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
#ifndef _TCC_TRMP_MANAGER_H_
#define _TCC_TRMP_MANAGER_H_

#include "tcc_trmp_type.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int tcc_trmp_manager_init(void);
extern void tcc_trmp_manager_deinit(void);
extern int tcc_trmp_manager_reset(void);

extern void tcc_trmp_manager_enableKeySwap(int nEnable);
//extern void tcc_trmp_manager_setDeviceKeyUpdateFunction(updateDeviceKey func);

extern int tcc_trmp_manager_setEMM(
	unsigned short usNetworkID, unsigned char *pSectionData, int nSectionDataSize);
extern int tcc_trmp_manager_setECM(
	unsigned short usNetworkID, unsigned char *pSectionData, int nSectionDataSize,
	unsigned char *pOddKey, unsigned char *pEvenKey);

extern int
tcc_trmp_manager_getMulti2Info(unsigned char **ppSystemKey, unsigned char **ppCBCDefaultValue);
extern int tcc_trmp_manager_getIDInfo(unsigned char **ppInfo, int *piInfoSize);

#ifdef __cplusplus
}
#endif

#endif
