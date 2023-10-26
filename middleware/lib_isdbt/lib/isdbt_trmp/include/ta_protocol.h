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
#ifndef TA_PROTOCOL_H
#define TA_PROTOCOL_H

#define __SECURITEE_DEBUG__
#if defined(__SECURITEE_DEBUG__)
#define L_DEBUG TEES_Print
#else
#define L_DEBUG(...)
#endif

//#define __SECURITEE_TRACE__
#if defined(__SECURITEE_TRACE__)
#define L_TRACE TEES_Print
#else
#define L_TRACE(...)
#endif

//#define __SECURITEE_INVTIME__
#if defined(__SECURITEE_INVTIME__)
#define L_INVTIME TEES_Print
#else
#define L_INVTIME(...)
#endif

#define INTERNAL static

/* Service Identifier:
 * {12345678-0000-0000-0123-456789ABCDEF} */
#define TA_UUID                                            \
	{                                                      \
		0xc6255201, 0x4db7, 0xa5e8,                        \
		{                                                  \
			0xb3, 0xeb, 0x68, 0x9f, 0x96, 0x7a, 0x60, 0x0b \
		}                                                  \
	}

/* Command: Initialize TRMP manager
 * Parameters:
 */
#define CMD_INIT_TRMP 0x00001000
int CmdInitTRMP(void);

/* Command: Deinitialize TRMP manager
 * Parameters:
 */
#define CMD_DEINIT_TRMP 0x00001001
int CmdDeinitTRMP(void);

/* Command: Encryption
 * Parameters:
 */
#define CMD_RESET_TRMP 0x00001002
int CmdResetTRMP(void);

/* Command: Set EMM section data
 * Parameters:
 */
#define CMD_SET_EMM 0x00001010
int CmdSetEMM(unsigned short usNetworkID, unsigned char *pSectionData, int nSectionDataSize);

/* Command: Set ECM section data
 * Parameters:
 */
#define CMD_SET_ECM 0x00001011
int CmdSetECM(
	unsigned short usNetworkID, unsigned char *pSectionData, int nSectionDataSize,
	unsigned char *pOddKey, unsigned char *pEvenKey);

/* Command: Get system key for multi2
 * Parameters:
 */
#define CMD_GET_MULTI2_SYSTEM_KEY 0x00001020
int CmdGetMulti2SystemKey(unsigned char *pMulti2SystemKey, unsigned char *pMulti2CBCDefaultValue);

/* Command: Get ID Size
 * Parameters:
 */
#define CMD_GET_ID_INFO_SIZE 0x00001021
int CmdGetIDInfoSize(int *pIDInfoSize);

/* Command: Get ID information
 * Parameters:
 */
#define CMD_GET_ID_INFO 0x00001022
int CmdGetIDInfo(unsigned char *pIDInfo, int iIDInfoSize);

#endif /* Include Guard */

/* end of file ta_protocol.h */
