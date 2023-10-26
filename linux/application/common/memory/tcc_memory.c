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
#include "tcc_memory.h"

#ifdef	TCC_MEMORY_DEBUG
#include "tcc_memory_debug.h"
unsigned int gMemallocCnt = 0;
unsigned int gMemRemain = 0;
#endif

#ifdef DEBUG_TCC_MALLOC_HOOKING_ENABLE
#include "tcc_memory_hooking.h"
#endif

/******************************************************************************
*	FUNCTIONS			: TCC_malloc
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void* TCC_malloc(unsigned int iSize)
{
#ifdef TCC_LINUX_MEMORY_SYTEM
	void *ptr;
	void *caller;
	
	ptr = malloc(iSize);
	
	#ifdef TCC_MEMORY_DEBUG
	TCC_Debug_Memory_Info_Insert( ptr, iSize);
	#endif

	#ifdef DEBUG_TCC_MALLOC_HOOKING_ENABLE
	caller = __builtin_return_address(0);
	tcc_malloc_hookinfo_update_malloc(ptr, iSize, caller);
	#endif	

	return ptr;
#else
#endif
}
/******************************************************************************
*	FUNCTIONS			: TCC_calloc
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void* TCC_calloc(unsigned int isize_t, unsigned int iSize)
{
#ifdef TCC_LINUX_MEMORY_SYTEM
	void *ptr;
	void *caller;
	
	ptr = calloc(isize_t, iSize);
	
	#ifdef TCC_MEMORY_DEBUG
	TCC_Debug_Memory_Info_Insert( ptr, iSize);
	#endif

	#ifdef DEBUG_TCC_MALLOC_HOOKING_ENABLE
	caller = __builtin_return_address(0);
	tcc_malloc_hookinfo_update_malloc(ptr, isize_t*iSize, caller);
	#endif	
	
	return ptr;
#else
#endif
}
/******************************************************************************
*	FUNCTIONS			: TCC_realloc
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void* TCC_realloc(void *p,unsigned int iSize)
{
#ifdef TCC_LINUX_MEMORY_SYTEM
	void *ptr;
	void *caller;
	
	#ifdef TCC_MEMORY_DEBUG
	if( p !=NULL )
	TCC_Debug_Memory_Info_Remove( p);
	#endif
	
	ptr = realloc(p, iSize);
	
	#ifdef TCC_MEMORY_DEBUG
	TCC_Debug_Memory_Info_Insert( ptr, iSize);
	#endif

	#ifdef DEBUG_TCC_MALLOC_HOOKING_ENABLE
	caller = __builtin_return_address(0);
	tcc_malloc_hookinfo_update_realloc(ptr, p, iSize, caller);
	#endif	
	
	return ptr;	
#else  
#endif
}
/******************************************************************************
*	FUNCTIONS			: TCC_free
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/

int TCC_free(void *pvPtr)
{
#ifdef TCC_LINUX_MEMORY_SYTEM
	int ret;
	void *caller;
	
	#ifdef TCC_MEMORY_DEBUG
	TCC_Debug_Memory_Info_Remove( pvPtr);
	#endif
	
	if( pvPtr != NULL )
	{
		free(pvPtr);
		ret = 1;
	}	
	else
	{
		printf("--------------------------------------------------\n");		
		printf("In %s, This 0x%08x Address is wrong \n", __func__, pvPtr);		
		printf("--------------------------------------------------\n");		
		ret = -1;
	}

	#ifdef DEBUG_TCC_MALLOC_HOOKING_ENABLE
	caller = __builtin_return_address(0);
	tcc_malloc_hookinfo_update_free(pvPtr, caller);
	#endif	

	return ret;
#else
#endif
}


