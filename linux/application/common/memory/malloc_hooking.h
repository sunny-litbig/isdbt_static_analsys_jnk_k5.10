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
#ifndef	_MALLOC_HOOKING__H__
#define	_MALLOC_HOOKING__H__

/******************************************************************************
* include 
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

/******************************************************************************
* defines 
******************************************************************************/
#define MAX_MALLOC_HOOK_INFO_SLOTS			32768
#define MAX_MALLOC_HOOK_SUMMARY_CALLER		8192

#define FLAG_MALLOC_HOOK_STATUS_FREE		0x00000000
#define FLAG_MALLOC_HOOK_STATUS_USED		0x00000001

#define DELAY_US_MALLOC_HOOK_NO_PRINT		10

/******************************************************************************
* typedefs & structure
******************************************************************************/
typedef struct tag_MALLOC_HOOK_INFO
{
	int		flag;
	void*	ptr;
	int		size;
	void*	caller;
} MALLOC_HOOK_INFO;

typedef struct tag_MALLOC_HOOK_SUMMARY_CALLER
{
	int		flag;
	void*	caller;
	int		total_used_slots;
	int		total_used_size;
} MALLOC_HOOK_SUMMARY_CALLER;

/******************************************************************************
* globals
******************************************************************************/
extern void my_malloc_hookinfo_proc_command(int mode, int param1, int param2, int param3, int param4);


/******************************************************************************
* declarations
******************************************************************************/


#endif //_TCC_UTIL_H___
