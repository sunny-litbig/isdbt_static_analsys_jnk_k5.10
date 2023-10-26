/*******************************************************************************

*   FileName : malloc_hooking.c

*   Copyright (c) Telechips Inc.

*   Description : malloc_hooking.c

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


/******************************************************************************
* include 
******************************************************************************/
#include <malloc_hooking.h>

#define RESTORE_HOOK()  	do { \
                        	  __malloc_hook		= old_malloc_hook; \
                        	  __realloc_hook	= old_realloc_hook; \
                        	  __free_hook		= old_free_hook; \
                        	} while (0)

#define SAVE_UNDER_HOOK()	do { \
                         	 old_malloc_hook	= __malloc_hook; \
                         	 old_realloc_hook	= __realloc_hook; \
                         	 old_free_hook		= __free_hook; \
                        	} while (0)
                        
#define SAVE_HOOK()     	do { \
                        	  __malloc_hook		= my_malloc_hook; \
                        	  __realloc_hook	= my_realloc_hook; \
                        	  __free_hook		= my_free_hook; \
                        	} while (0)

static void	my_init_hook(void);
static void *my_malloc_hook(size_t size, const void *caller);
static void *my_realloc_hook(void *ptr, size_t size, const void *caller);
static void *my_free_hook(void *ptr, const void *caller);

static void *(*old_malloc_hook)(size_t size, const void *caller);
static void *(*old_realloc_hook)(void *ptr, size_t size, const void *caller);
static void *(*old_free_hook)(void *ptr, const void *caller);

void (*__malloc_initialize_hook)(void) = my_init_hook;

/******************************************************************************
* locals
******************************************************************************/
pthread_mutex_t	lockHookProc;

static MALLOC_HOOK_INFO g_malloc_info[MAX_MALLOC_HOOK_INFO_SLOTS];
static MALLOC_HOOK_SUMMARY_CALLER g_malloc_summary_caller[MAX_MALLOC_HOOK_SUMMARY_CALLER];

static int print_mode_process_hooking_slots = 0;
static int enable_hooking_info = 0;

static void my_malloc_hookinfo_clear();
static void my_malloc_hookinfo_init();
static int my_malloc_hookinfo_update_malloc(void *ptr, int size, void* caller);
static int my_malloc_hookinfo_update_realloc(void *ptr, void *oldptr, int size, void* caller);
static int my_malloc_hookinfo_update_free(void *ptr, void* caller);


static void my_init_hook(void)
{
	my_malloc_hookinfo_init();
	
	old_malloc_hook		= __malloc_hook;
	old_realloc_hook	= __realloc_hook;
	old_free_hook		= __free_hook;
	
    __malloc_hook		= my_malloc_hook;
	__realloc_hook		= my_realloc_hook;
	__free_hook			= my_free_hook;
}

static void *my_malloc_hook(size_t size, const void *caller)
{
	void *result;
	int	update_slot_no;

	pthread_mutex_lock(&lockHookProc);

	RESTORE_HOOK();
	result = malloc(size);
	SAVE_UNDER_HOOK();
		
	update_slot_no = my_malloc_hookinfo_update_malloc(result, size, caller);
	if (print_mode_process_hooking_slots == 1)
	{
		printf("[MEMHOOK] %p = malloc(%u) by %p ==> SLOT[%d] \n", result, (unsigned int)size, caller, update_slot_no);
	}
	else if (print_mode_process_hooking_slots == 2)
	{
		if (update_slot_no < 0)
			printf("[MEMHOOK] %p = malloc(%u) by %p ==> SLOT[%d] \n", result, (unsigned int)size, caller, update_slot_no);
	}
	else
	{
		usleep(DELAY_US_MALLOC_HOOK_NO_PRINT);
	}

	SAVE_HOOK();

	pthread_mutex_unlock(&lockHookProc);
	return result;
}

static void *my_realloc_hook(void *ptr, size_t size, const void *caller)
{
	void *result = NULL;
	int	update_slot_no;

	pthread_mutex_lock(&lockHookProc);
	
	RESTORE_HOOK();
	if (!ptr)
	{
		SAVE_UNDER_HOOK();
		if (print_mode_process_hooking_slots)
			printf("[MEMHOOK] Wrong realloc(%p,%u) by %p \n", ptr, (unsigned int)size, caller);
		RESTORE_HOOK();
	}
	result = realloc(ptr, size);
	SAVE_UNDER_HOOK();

	update_slot_no = my_malloc_hookinfo_update_realloc(result, ptr, size, caller);
	if (print_mode_process_hooking_slots == 1)
	{
		printf("[MEMHOOK] %p = realloc(%p,%u) by %p ==> SLOT[%d] \n", result, ptr, (unsigned int)size, caller, update_slot_no);
	}
	else if (print_mode_process_hooking_slots == 2)
	{
		if (update_slot_no < 0) 
			printf("[MEMHOOK] %p = realloc(%p,%u) by %p ==> SLOT[%d] \n", result, ptr, (unsigned int)size, caller, update_slot_no);
	}
	else
	{
		usleep(DELAY_US_MALLOC_HOOK_NO_PRINT);
	}

	SAVE_HOOK();

	pthread_mutex_unlock(&lockHookProc);
	return result;
}

static void *my_free_hook(void *ptr, const void *caller)
{
	int	update_slot_no;

	pthread_mutex_lock(&lockHookProc);

	RESTORE_HOOK();
	if (!ptr)
	{
		SAVE_UNDER_HOOK();
		if (print_mode_process_hooking_slots)
			printf("[MEMHOOK] Wrong free(%p) by %p \n", ptr, caller);
		RESTORE_HOOK();
	}
	free(ptr);
	SAVE_UNDER_HOOK();
		
	update_slot_no = my_malloc_hookinfo_update_free(ptr, caller);
	if (print_mode_process_hooking_slots == 1)
	{
		printf("[MEMHOOK] free(%p) by %p <== SLOT[%d] \n", ptr, caller, update_slot_no);
	}
	else if (print_mode_process_hooking_slots == 2)
	{
		if (update_slot_no < 0)
			printf("[MEMHOOK] free(%p) by %p <== SLOT[%d] \n", ptr, caller, update_slot_no);
	}
	else
	{
		usleep(DELAY_US_MALLOC_HOOK_NO_PRINT);
	}
	
	SAVE_HOOK();

	pthread_mutex_unlock(&lockHookProc);
	return NULL;
}

static void my_malloc_hookinfo_clear()
{
	int i;

	for (i = 0; i < MAX_MALLOC_HOOK_INFO_SLOTS; ++i)
	{
		g_malloc_info[i].flag		= FLAG_MALLOC_HOOK_STATUS_FREE;
		g_malloc_info[i].ptr		= NULL;
		g_malloc_info[i].size		= 0;
		g_malloc_info[i].caller		= NULL;
	}
}

static void my_malloc_hookinfo_init()
{
	my_malloc_hookinfo_clear();
	pthread_mutex_init(&lockHookProc);
}

static int my_malloc_hookinfo_update_malloc(void *ptr, int size, void* caller)
{
	int i;
	int result = -1;

	if ((ptr == NULL) || (!size) || (!enable_hooking_info))
		return result;

	// Scan Dulicated info & Remove
	for (i = 0; i < MAX_MALLOC_HOOK_INFO_SLOTS; ++i)
	{
		if ((g_malloc_info[i].flag == FLAG_MALLOC_HOOK_STATUS_USED) && (g_malloc_info[i].ptr == ptr))
		{
			g_malloc_info[i].flag		= FLAG_MALLOC_HOOK_STATUS_FREE;
			g_malloc_info[i].ptr		= NULL;
			g_malloc_info[i].size		= 0;
			g_malloc_info[i].caller		= NULL;
		}
	}

	// Insert New
	for (i = 0; i < MAX_MALLOC_HOOK_INFO_SLOTS; ++i)
	{
		if (g_malloc_info[i].flag == FLAG_MALLOC_HOOK_STATUS_FREE)
		{
			g_malloc_info[i].flag		= FLAG_MALLOC_HOOK_STATUS_USED;
			g_malloc_info[i].ptr		= ptr;
			g_malloc_info[i].size		= size;
			g_malloc_info[i].caller		= caller;
			result = i;
			break;
		}
	}

	return result;
}

static int my_malloc_hookinfo_update_realloc(void *ptr, void *oldptr, int size, void* caller)
{
	int i;
	int result = -1;

	if ((ptr == NULL) || (oldptr == NULL) || (!size) || (!enable_hooking_info))
		return result;

	for (i = 0; i < MAX_MALLOC_HOOK_INFO_SLOTS; ++i)
	{
		if ((g_malloc_info[i].flag == FLAG_MALLOC_HOOK_STATUS_USED) && (g_malloc_info[i].ptr == oldptr))
		{
			g_malloc_info[i].flag		= FLAG_MALLOC_HOOK_STATUS_USED;
			g_malloc_info[i].ptr		= ptr;
			g_malloc_info[i].size		= size;
			g_malloc_info[i].caller		= caller;
			result = i;
			break;
		}
	}

	return result;
}

static int my_malloc_hookinfo_update_free(void *ptr, void* caller)
{
	int i;
	int result = -1;

	if ((ptr == NULL) || (!enable_hooking_info))
		return result;

	for (i = 0; i < MAX_MALLOC_HOOK_INFO_SLOTS; ++i)
	{
		if ((g_malloc_info[i].flag == FLAG_MALLOC_HOOK_STATUS_USED) && (g_malloc_info[i].ptr == ptr))
		{
			g_malloc_info[i].flag		= FLAG_MALLOC_HOOK_STATUS_FREE;
			g_malloc_info[i].ptr		= NULL;
			g_malloc_info[i].size		= 0;
			g_malloc_info[i].caller		= NULL;
			result = i;
			break;
		}
	}

	return result;
}

//===============================================================================================
//	my_malloc_hookinfo_proc_command
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
void my_malloc_hookinfo_proc_command(int mode, int param1, int param2, int param3, int param4)
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
		enable_hooking_info = 0;
	}
	else if (mode == 101)
	{
		my_malloc_hookinfo_clear();
		enable_hooking_info = 1;
	}
	else if (mode == 255)
	{
		print_mode_process_hooking_slots = (int)param1;
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
			if (filter_slotnumber < MAX_MALLOC_HOOK_INFO_SLOTS)
				printf("[MEMHOOK] [%04d] P(%8p)C(%8p)S(%d) \n", i, g_malloc_info[i].ptr,  g_malloc_info[i].caller, g_malloc_info[i].size);
		}
		else if ( mode == 5 )
		{
			for (k = 0; k < MAX_MALLOC_HOOK_SUMMARY_CALLER; ++k)
				g_malloc_summary_caller[k].flag = 0;
			
			for (i = 0; i < MAX_MALLOC_HOOK_INFO_SLOTS; ++i)
			{		
				if ((g_malloc_info[i].flag == FLAG_MALLOC_HOOK_STATUS_USED))
				{
					// search already exist
					for (k = 0; k < MAX_MALLOC_HOOK_SUMMARY_CALLER; ++k)
					{
						if ((g_malloc_summary_caller[k].flag == 1) && (g_malloc_info[i].caller == g_malloc_summary_caller[k].caller))
						{
							g_malloc_summary_caller[k].total_used_slots++;
							g_malloc_summary_caller[k].total_used_size += g_malloc_info[i].size;
							break;
						}	
					}

					// Not Found & Insert New
					if ( k >= MAX_MALLOC_HOOK_SUMMARY_CALLER )
					{
						for (k = 0; k < MAX_MALLOC_HOOK_SUMMARY_CALLER; ++k)
						{
							if (g_malloc_summary_caller[k].flag == 0)
							{
								g_malloc_summary_caller[k].flag = 1;
								g_malloc_summary_caller[k].caller = g_malloc_info[i].caller;
								g_malloc_summary_caller[k].total_used_slots = 1;
								g_malloc_summary_caller[k].total_used_size = g_malloc_info[i].size;
								break;
							}	
						}
					}
				}
			}
		}
		else
		{
			for (i = 0; i < MAX_MALLOC_HOOK_INFO_SLOTS; ++i)
			{
				if ((g_malloc_info[i].flag == FLAG_MALLOC_HOOK_STATUS_USED))
				{
					last_used_slots = i;
					total_used_size += g_malloc_info[i].size;
					total_used_count++;

					if (mode == 1)
					{
						printf("[MEMHOOK] [%04d] P(%8p)C(%8p)S(%d) \n", i, g_malloc_info[i].ptr,  g_malloc_info[i].caller, g_malloc_info[i].size);
					}	
					else if (mode == 2)
					{
						if (g_malloc_info[i].size > filter_size)
						{
							if (!not_print_match_slots)
								printf("[MEMHOOK] [%04d] P(%8p)C(%8p)S(%d) \n", i, g_malloc_info[i].ptr,  g_malloc_info[i].caller, g_malloc_info[i].size);
						}	
					}
					else if (mode == 3)
					{
						if (g_malloc_info[i].caller == filter_caller)
						{
							total_filter_used_size += g_malloc_info[i].size;
							++total_filter_used_count;

							if (!not_print_match_slots)
								printf("[MEMHOOK] [%04d] P(%8p)C(%8p)S(%d) \n", i, g_malloc_info[i].ptr,  g_malloc_info[i].caller, g_malloc_info[i].size);
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
			printf("[MEMHOOK] ----- FILTER Total USE COUNT(%d) & SIZE(%d) \n", total_filter_used_count, total_filter_used_size);
		}
		else if (mode == 5)
		{
			for (k = 0; k < MAX_MALLOC_HOOK_SUMMARY_CALLER; ++k)
			{
				if (g_malloc_summary_caller[k].flag == 1)
				{
					printf("[MEMHOOK] Summary by Caller(%08p) total use slots(%05d) & size(%d) \n",
						g_malloc_summary_caller[k].caller,
						g_malloc_summary_caller[k].total_used_slots,
						g_malloc_summary_caller[k].total_used_size);
				}	
			}
			printf("\n");
		}

		if ((mode == 0)||(mode == 1)||(mode == 2)||(mode == 3))
		{
			printf("[MEMHOOK] ===== TOTAL USE SLOTS(%d) & SIZE(%d) \n", total_used_count, total_used_size);
			printf("[MEMHOOK] ===== TOTAL SLOT(%d) LAST_USED_SLOT(%d) \n\n", MAX_MALLOC_HOOK_INFO_SLOTS, last_used_slots);
		}
	}			
}


