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
/******************************************************************************
* include 
******************************************************************************/
#include <tcc_memory_hooking.h>

/******************************************************************************
* locals
******************************************************************************/
static TCC_MALLOC_HOOK_INFO				g_tcc_malloc_info[MAX_TCC_MALLOC_HOOK_INFO_SLOTS];
static TCC_MALLOC_HOOK_SUMMARY_CALLER	g_tcc_malloc_summary_caller[MAX_TCC_MALLOC_HOOK_SUMMARY_CALLER];

static int tcc_malloc_enable_print_trace_hooking_slots = 0;
static void* tcc_malloc_trace_hooking_slots_caller = (void*)0;
static int tcc_malloc_enable_hooking_info = 0;

static void tcc_malloc_hookinfo_clear()
{
	int i;

	for (i = 0; i < MAX_TCC_MALLOC_HOOK_INFO_SLOTS; ++i)
	{
		g_tcc_malloc_info[i].flag		= FLAG_TCC_MALLOC_HOOK_STATUS_FREE;
		g_tcc_malloc_info[i].ptr		= NULL;
		g_tcc_malloc_info[i].size		= 0;
		g_tcc_malloc_info[i].caller		= NULL;
	}

	for (i = 0; i < MAX_TCC_MALLOC_HOOK_SUMMARY_CALLER; ++i)
	{
		g_tcc_malloc_summary_caller[i].flag = 0;
		g_tcc_malloc_summary_caller[i].caller = NULL;
		g_tcc_malloc_summary_caller[i].total_used_slots = 0;
		g_tcc_malloc_summary_caller[i].total_used_size = 0;
	}
}

void tcc_malloc_hookinfo_init()
{
	tcc_malloc_hookinfo_clear();
}

void tcc_malloc_hookinfo_update_malloc(void *ptr, int size, void* caller)
{
	int i;
	int result = -1;

	if ((ptr == NULL) || (!size) || (!tcc_malloc_enable_hooking_info))
		return result;

	#if 0
	// Scan Dulicated info & Remove
	for (i = 0; i < MAX_TCC_MALLOC_HOOK_INFO_SLOTS; ++i)
	{
		if ((g_tcc_malloc_info[i].flag == FLAG_TCC_MALLOC_HOOK_STATUS_USED) && (g_tcc_malloc_info[i].ptr == ptr))
		{
			g_tcc_malloc_info[i].flag		= FLAG_TCC_MALLOC_HOOK_STATUS_FREE;
			g_tcc_malloc_info[i].ptr		= NULL;
			g_tcc_malloc_info[i].size		= 0;
			g_tcc_malloc_info[i].caller		= NULL;
		}
	}
	#endif
	
	// Insert New
	for (i = 0; i < MAX_TCC_MALLOC_HOOK_INFO_SLOTS; ++i)
	{
		if (g_tcc_malloc_info[i].flag == FLAG_TCC_MALLOC_HOOK_STATUS_FREE)
		{
			g_tcc_malloc_info[i].flag		= FLAG_TCC_MALLOC_HOOK_STATUS_USED;
			g_tcc_malloc_info[i].ptr		= ptr;
			g_tcc_malloc_info[i].size		= size;
			g_tcc_malloc_info[i].caller		= caller;
			result = i;
			break;
		}
	}

	if (i < MAX_TCC_MALLOC_HOOK_INFO_SLOTS)
	{
		if (tcc_malloc_enable_print_trace_hooking_slots)
		{
			if (!tcc_malloc_trace_hooking_slots_caller)
				printf("[TCCMEMHOOK] %p = malloc(%u) by %p => S[%d] \n", ptr, (unsigned int)size, caller, i);
			else if (tcc_malloc_trace_hooking_slots_caller == caller)
				printf("[TCCMEMHOOK] %p = malloc(%u) by %p => S[%d] \n", ptr, (unsigned int)size, caller, i);
		}			
	}
	
	return result;
}

void tcc_malloc_hookinfo_update_realloc(void *ptr, void *oldptr, int size, void* caller)
{
	int i;
	int result = -1;

	if ((ptr == NULL) || (oldptr == NULL) || (!size) || (!tcc_malloc_enable_hooking_info))
		return result;

	for (i = 0; i < MAX_TCC_MALLOC_HOOK_INFO_SLOTS; ++i)
	{
		if ((g_tcc_malloc_info[i].flag == FLAG_TCC_MALLOC_HOOK_STATUS_USED) && (g_tcc_malloc_info[i].ptr == oldptr))
		{
			g_tcc_malloc_info[i].flag		= FLAG_TCC_MALLOC_HOOK_STATUS_USED;
			g_tcc_malloc_info[i].ptr		= ptr;
			g_tcc_malloc_info[i].size		= size;
			g_tcc_malloc_info[i].caller		= caller;
			result = i;
			break;
		}
	}

	if (i < MAX_TCC_MALLOC_HOOK_INFO_SLOTS)
	{
		if (tcc_malloc_enable_print_trace_hooking_slots)
		{
			if (!tcc_malloc_trace_hooking_slots_caller)
				printf("[TCCMEMHOOK] %p = realloc(%p,%u) by %p => S[%d] \n", ptr, oldptr, (unsigned int)size, caller, i);
			else if (tcc_malloc_trace_hooking_slots_caller == caller)
				printf("[TCCMEMHOOK] %p = realloc(%p,%u) by %p => S[%d] \n", ptr, oldptr, (unsigned int)size, caller, i);
		}			
	}

	return result;
}

void tcc_malloc_hookinfo_update_free(void *ptr, void* caller)
{
	int i;
	int result = -1;

	if ((ptr == NULL) || (!tcc_malloc_enable_hooking_info))
		return result;

	for (i = 0; i < MAX_TCC_MALLOC_HOOK_INFO_SLOTS; ++i)
	{
		if ((g_tcc_malloc_info[i].flag == FLAG_TCC_MALLOC_HOOK_STATUS_USED) && (g_tcc_malloc_info[i].ptr == ptr))
		{
			g_tcc_malloc_info[i].flag		= FLAG_TCC_MALLOC_HOOK_STATUS_FREE;
			g_tcc_malloc_info[i].ptr		= NULL;
			g_tcc_malloc_info[i].size		= 0;
			g_tcc_malloc_info[i].caller		= NULL;
			result = i;
			break;
		}
	}

	if (i < MAX_TCC_MALLOC_HOOK_INFO_SLOTS)
	{
		if (tcc_malloc_enable_print_trace_hooking_slots)
		{
			if (!tcc_malloc_trace_hooking_slots_caller)
				printf("[TCCMEMHOOK] free(%p) by %p <= S[%d] \n", ptr, caller, i);
			else if (tcc_malloc_trace_hooking_slots_caller == caller)
				printf("[TCCMEMHOOK] free(%p) by %p <= S[%d] \n", ptr, caller, i);
		}			
	}

	return result;
}

//===============================================================================================
//	tcc_malloc_hookinfo_proc_command
//
//	[ MODE 0 ]
//		Only Print Total used count & Size
//	[ MODE 1 ]
//		Print All used slots
//	[ MODE 2 ]
//		Print used slots matched for "size > param1"
//	[ MODE 3 ]
//		Print used slots matched for "caller == param1"
//	[ MODE 4 ]
//		Print slots matched for "slot no == param1"
//	[ MODE 5 ]
//		Print summary by caller
//
//	[ MODE 100 ]
//		Stop  to gathering hook information
//	[ MODE 101 ]
//		Start to gathering hook information
//	[ MODE 255 ]
//		On & Off to print hooking process "on,off = param1"
//
//===============================================================================================
void tcc_malloc_hookinfo_proc_command(int mode, int param1, int param2, int param3, int param4)
{
	int		i,k;
	int		not_print_match_slots = 0;
	int		total_used_size = 0;
	int		total_used_count = 0;
	int		total_filter_used_size = 0;
	int		total_filter_used_count = 0;
	int		last_used_slots = 0;
	int 	filter_slotnumber = 0;
	int 	filter_size = 0;
	void	*filter_caller = NULL;

	//================================================
	// Manuplation relevant commands
	//================================================
	if (mode == 100)
	{
		tcc_malloc_enable_hooking_info = 0;
	}
	else if (mode == 101)
	{
		tcc_malloc_hookinfo_clear();
		tcc_malloc_enable_hooking_info = 1;
	}
	else if (mode == 255)
	{
		tcc_malloc_enable_print_trace_hooking_slots = (int)param1;
		tcc_malloc_trace_hooking_slots_caller = (void*)param2;
	}		
	//================================================
	// Print Slot information relevant commands
	//================================================
	else
	{
		//------------------------------------------------
		// Pre-do
		//------------------------------------------------
		if (mode == 2)
		{
			filter_size = (int)param1;
			not_print_match_slots = (int)param2;
		}
		else if (mode == 3)
		{
			filter_caller = (void*)param1;
			not_print_match_slots = (int)param2;
		}
		else if (mode == 4)
		{
			filter_slotnumber = (void*)param1;
		}

		//------------------------------------------------
		// Search slots
		//------------------------------------------------
		if ( mode == 4 )
		{
			i = filter_slotnumber;
			if (filter_slotnumber < MAX_TCC_MALLOC_HOOK_INFO_SLOTS)
				printf("[TCCMEMHOOK] [%04d] P(%8p)C(%8p)S(%d) \n", i, g_tcc_malloc_info[i].ptr,  g_tcc_malloc_info[i].caller, g_tcc_malloc_info[i].size);
		}
		else if ( mode == 5 )
		{
			for (k = 0; k < MAX_TCC_MALLOC_HOOK_SUMMARY_CALLER; ++k)
				g_tcc_malloc_summary_caller[k].flag = 0;
			
			for (i = 0; i < MAX_TCC_MALLOC_HOOK_INFO_SLOTS; ++i)
			{		
				if ((g_tcc_malloc_info[i].flag == FLAG_TCC_MALLOC_HOOK_STATUS_USED))
				{
					// search already exist
					for (k = 0; k < MAX_TCC_MALLOC_HOOK_SUMMARY_CALLER; ++k)
					{
						if ((g_tcc_malloc_summary_caller[k].flag == 1) && (g_tcc_malloc_info[i].caller == g_tcc_malloc_summary_caller[k].caller))
						{
							g_tcc_malloc_summary_caller[k].total_used_slots++;
							g_tcc_malloc_summary_caller[k].total_used_size += g_tcc_malloc_info[i].size;
							break;
						}	
					}

					// Not Found & Insert New
					if ( k >= MAX_TCC_MALLOC_HOOK_SUMMARY_CALLER )
					{
						for (k = 0; k < MAX_TCC_MALLOC_HOOK_SUMMARY_CALLER; ++k)
						{
							if (g_tcc_malloc_summary_caller[k].flag == 0)
							{
								g_tcc_malloc_summary_caller[k].flag = 1;
								g_tcc_malloc_summary_caller[k].caller = g_tcc_malloc_info[i].caller;
								g_tcc_malloc_summary_caller[k].total_used_slots = 1;
								g_tcc_malloc_summary_caller[k].total_used_size = g_tcc_malloc_info[i].size;
								break;
							}	
						}
					}
				}
			}
		}
		else
		{
			for (i = 0; i < MAX_TCC_MALLOC_HOOK_INFO_SLOTS; ++i)
			{
				if ((g_tcc_malloc_info[i].flag == FLAG_TCC_MALLOC_HOOK_STATUS_USED))
				{
					last_used_slots = i;
					total_used_size += g_tcc_malloc_info[i].size;
					total_used_count++;

					if (mode == 1)
					{
						printf("[TCCMEMHOOK] [%04d] P(%8p)C(%8p)S(%d) \n", i, g_tcc_malloc_info[i].ptr,  g_tcc_malloc_info[i].caller, g_tcc_malloc_info[i].size);
					}	
					else if (mode == 2)
					{
						if (g_tcc_malloc_info[i].size > filter_size)
						{
							if (!not_print_match_slots)
								printf("[TCCMEMHOOK] [%04d] P(%8p)C(%8p)S(%d) \n", i, g_tcc_malloc_info[i].ptr,  g_tcc_malloc_info[i].caller, g_tcc_malloc_info[i].size);
						}	
					}
					else if (mode == 3)
					{
						if (g_tcc_malloc_info[i].caller == filter_caller)
						{
							total_filter_used_size += g_tcc_malloc_info[i].size;
							++total_filter_used_count;

							if (!not_print_match_slots)
								printf("[TCCMEMHOOK] [%04d] P(%8p)C(%8p)S(%d) \n", i, g_tcc_malloc_info[i].ptr,  g_tcc_malloc_info[i].caller, g_tcc_malloc_info[i].size);
						}	
					}
				}
			}
		}
		
		//------------------------------------------------
		// Printf result
		//------------------------------------------------
		if (mode == 2)
		{
			//
		}
		else if (mode == 3)
		{
			printf("[TCCMEMHOOK] ----- FILTER Total USE COUNT(%d) & SIZE(%d) \n", total_filter_used_count, total_filter_used_size);
		}
		else if (mode == 5)
		{
			for (k = 0; k < MAX_TCC_MALLOC_HOOK_SUMMARY_CALLER; ++k)
			{
				if (g_tcc_malloc_summary_caller[k].flag == 1)
				{
					printf("[TCCMEMHOOK] Summary by Caller(%08p) total use slots(%05d) & size(%d) \n",
						g_tcc_malloc_summary_caller[k].caller,
						g_tcc_malloc_summary_caller[k].total_used_slots,
						g_tcc_malloc_summary_caller[k].total_used_size);
				}	
			}
			printf("\n");
		}

		if ((mode == 0)||(mode == 1)||(mode == 2)||(mode == 3))
		{
			printf("[TCCMEMHOOK] ===== TOTAL USE SLOTS(%d) & SIZE(%d) \n", total_used_count, total_used_size);
			printf("[TCCMEMHOOK] ===== TOTAL SLOT(%d) LAST_USED_SLOT(%d) \n\n", MAX_TCC_MALLOC_HOOK_INFO_SLOTS, last_used_slots);
		}
	}			
}


