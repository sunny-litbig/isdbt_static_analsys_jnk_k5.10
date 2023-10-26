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
#ifndef	_TCC_MEMORY_DEBUG_H_
#define	_TCC_MEMORY_DEBUG_H_

/*****************************************************************************
*
* Include
*
******************************************************************************/
#include "Tc_linkedlist.h"

/*****************************************************************************
*
* Type Defines
*
******************************************************************************/

/*****************************************************************************
*
* Structures
*
******************************************************************************/
typedef struct TCC_MEMORY_structure
{
	unsigned int 	m_uiMem_Size;
	void 			*m_pStartAdd;
}TCC_MEMORY_INFO_t;

typedef struct TCC_DEBUG_MEMORY_structure
{
	DLinked_list 		m_DLList;
	pthread_mutex_t mutex;
	unsigned int 		m_uiMem_Counter;
	unsigned int 		m_uiMem_CurSize;
	unsigned int 		m_uiMem_MaxPitch;

}TCC_DEBUG_MEM_INFO_t;

void TCC_Debug_Display_Memory_Info( void);
void TCC_Debug_Display_Memory_Leak( void);
void TCC_Debug_Memory_Info_Insert( void *pStartAdd, unsigned int uiSize);
void TCC_Debug_Memory_Info_Remove( void *pStartAdd);
void TCC_Debug_Display_Memory_Info( void);

#endif /* _TCC_MEMORY_DEBUG_H_ */
