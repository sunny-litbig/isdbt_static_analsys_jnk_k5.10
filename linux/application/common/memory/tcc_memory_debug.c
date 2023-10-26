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
#include "tcc_memory_debug.h"

static TCC_DEBUG_MEM_INFO_t	*TCC_MemInfo;
/******************************************************************************
*	FUNCTIONS			: TCC_Debug_Memory_Info_Init
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_Debug_Memory_Info_Init(void)
{
	TCC_MemInfo = (TCC_DEBUG_MEM_INFO_t *)malloc(sizeof(TCC_DEBUG_MEM_INFO_t));

	dlist_init(&TCC_MemInfo->m_DLList, free);

	TCC_MemInfo->m_uiMem_Counter = 0;
	TCC_MemInfo->m_uiMem_CurSize = 0;
	TCC_MemInfo->m_uiMem_MaxPitch= 0;
}

/******************************************************************************
*	FUNCTIONS			: TCC_Debug_Memory_Info_Deinit
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_Debug_Memory_Info_Deinit( void)
{
	TCC_Debug_Display_Memory_Info();

	TCC_Debug_Display_Memory_Leak();

	dlist_destroy( &TCC_MemInfo->m_DLList);
	free(TCC_MemInfo);
}


/******************************************************************************
*	FUNCTIONS			: TCC_Debug_Memory_Info_Insert
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_Debug_Memory_Info_Insert( void *pStartAdd, unsigned int uiSize)
{
	TCC_MEMORY_INFO_t	*list;
	
	TCC_DEBUG_MEM_INFO_t *pTCC_MemInfo = TCC_MemInfo;
	
	printf("In %s, 0x%08x, size[%d], mem cnt [%d]\n", __func__, pStartAdd, uiSize, pTCC_MemInfo->m_uiMem_Counter);	
	
	pthread_mutex_lock(&pTCC_MemInfo->mutex);
	
	list = (( TCC_MEMORY_INFO_t *)malloc( sizeof( TCC_MEMORY_INFO_t)));
	
	list->m_pStartAdd 		= pStartAdd;
	list->m_uiMem_Size		= uiSize;

	pTCC_MemInfo->m_uiMem_Counter++;
	pTCC_MemInfo->m_uiMem_CurSize += uiSize;

	if( pTCC_MemInfo->m_uiMem_CurSize > pTCC_MemInfo->m_uiMem_MaxPitch)
		pTCC_MemInfo->m_uiMem_MaxPitch = pTCC_MemInfo->m_uiMem_CurSize;

	if( dlist_insert_nextelmt( &pTCC_MemInfo->m_DLList, dlist_tail( &pTCC_MemInfo->m_DLList), list) != NULL)
	{
		free( list);
	}

	pthread_mutex_unlock(&pTCC_MemInfo->mutex);
}

/******************************************************************************
*	FUNCTIONS			: TCC_Debug_Memory_Info_Remove
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_Debug_Memory_Info_Remove( void *pStartAdd)
{
	DListElmt 			*element;
	TCC_MEMORY_INFO_t	*list;

	TCC_DEBUG_MEM_INFO_t *pTCC_MemInfo = TCC_MemInfo;
	pthread_mutex_lock(&pTCC_MemInfo->mutex);
	element = dlist_head( &pTCC_MemInfo->m_DLList);

	if( element == NULL)
	{
		printf("--------------------------------------------------\n");	
		printf("Memory structure NULL\n");
		printf("--------------------------------------------------\n");	
		return;
	}

	if( dlist_elmtcnt( &pTCC_MemInfo->m_DLList) > 0)
	{
		while( 1)
		{
			list = dlist_data( element);

			if( list->m_pStartAdd == pStartAdd)
			{
				pTCC_MemInfo->m_uiMem_Counter--;
				pTCC_MemInfo->m_uiMem_CurSize -= list->m_uiMem_Size;
				
				if( dlist_remove_elmt( &pTCC_MemInfo->m_DLList, element, (void **)&list) != NULL)
				{
					return;	
				}

				break;
			}

			if( dlist_is_tail( element))
			{

				printf("--------------------------------------------------\n");		
				printf("In %s, This 0x%08x Address is wrong \n", __func__, pStartAdd);		
				printf("--------------------------------------------------\n");		
				break;
			}	
			else
				element = dlist_next(element);
			
		}
	}
	printf("In %s, 0x%08x, mem cnt [%d]\n", __func__, pStartAdd, pTCC_MemInfo->m_uiMem_Counter);	
	pthread_mutex_unlock(&pTCC_MemInfo->mutex);
}
/******************************************************************************
*	FUNCTIONS			: TCC_Debug_Display_Memory_Info
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_Debug_Display_Memory_Info( void)
{
	TCC_DEBUG_MEM_INFO_t *pTCC_MemInfo = TCC_MemInfo;
	
	printf("\n#############  Player MemoryInformation ####################  \n");
	printf("#Memory Information		Malloc Memory Current Size %d\n", pTCC_MemInfo->m_uiMem_CurSize);
	printf("#Memory Information		Malloc Memory Counter %d\n", pTCC_MemInfo->m_uiMem_Counter);
	printf("#Memory Information		Malloc Memory Pitch %d\n", pTCC_MemInfo->m_uiMem_MaxPitch);
	printf("############################################################\n");
}

/******************************************************************************
*	FUNCTIONS			: TCC_Debug_Display_Memory_Leak
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_Debug_Display_Memory_Leak( void)
{

	DListElmt 			*element;
	TCC_MEMORY_INFO_t	*list;

	TCC_DEBUG_MEM_INFO_t *pTCC_MemInfo = TCC_MemInfo;

	if( pTCC_MemInfo != NULL)
	{
		element = dlist_head( &pTCC_MemInfo->m_DLList);
		
		if( dlist_elmtcnt( &pTCC_MemInfo->m_DLList) > 0)
		{
			printf("\n#############  Player Leack Information ####################  \n");
			printf("#Memory Information		Malloc Memory Current Size %d\n", pTCC_MemInfo->m_uiMem_CurSize);
			printf("#Memory Information		Malloc Memory Counter %d\n", pTCC_MemInfo->m_uiMem_Counter);
		
			while( 1)
			{
				list = dlist_data( element);

				if( list->m_pStartAdd )
				{
				
					pTCC_MemInfo->m_uiMem_Counter--;
					pTCC_MemInfo->m_uiMem_CurSize -= list->m_uiMem_Size;
					printf("---In %s, 0x%08x, size[%d], mem cnt [%d]\n", __func__, list->m_pStartAdd, list->m_uiMem_Size, pTCC_MemInfo->m_uiMem_Counter);	
				}

				if( dlist_is_tail( element))
				{
					break;
				}	
				else
					element = dlist_next(element);
				
			}

			printf("###########################################################\n");
		}
		else
			return;
		
	}	
	
}

