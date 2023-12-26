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
#include "tcc_dxb_memory.h"
#define LOG_TAG	"tcc_dxb_memory"
#include <utils/Log.h>


//#define TCC_MW_MEMORYLEAKCHECK


#ifdef TCC_MW_MEMORYLEAKCHECK

#include <pthread.h>
#include <tcc_dxb_thread.h>

#define	TCC_MW_MEMORYLEAKCHECK_STRUCT_MAX       256*1024
#define TCC_MW_MEMORYLEAKCHECK_FILE_NAME_MAX    48

typedef struct _TccMw_MemoryLeakCheck_Struct{
	char     functionName[TCC_MW_MEMORYLEAKCHECK_FILE_NAME_MAX];
	int      line;
	int      size;
	int      usage;
	void*    ptr;
}TccMw_MemoryLeakCheck_Struct;
static TccMw_MemoryLeakCheck_Struct g_TccMw_MemoryLeakCheck_Struct[TCC_MW_MEMORYLEAKCHECK_STRUCT_MAX];
static unsigned int g_TccMw_MemoryLeakCheck_CurrentTotalMallocSize = 0;
static unsigned int g_TccMw_MemoryLeakCheck_CurrentTotalMallocCount = 0;
static pthread_mutex_t g_TccMw_MemoryLeakCheck_Mutex;

static unsigned int g_TccMw_MemoryLeakCheck_HighestIndex = 0;    // Highest index of the TccMw_MemoryLeakCheck_Struct while checking memory leak


////////////////////////////////////////////////////////////////////
// Thread, to check current memory usage every 10 seconds

static pthread_t g_TccMw_MemoryLeakCheck_ThreadId;
static int g_TccMw_MemoryLeakCheck_ThreadRunning = 0;

void* tcc_mw_memoryleakcheck_thread(void *arg)
{
	int i = 0;

	while(g_TccMw_MemoryLeakCheck_ThreadRunning)
	{
		printf("[%s] CurrentTotalMallocSize(%d), CurrentTotalMallocCount(%d)\n",
			__FUNCTION__, g_TccMw_MemoryLeakCheck_CurrentTotalMallocSize, g_TccMw_MemoryLeakCheck_CurrentTotalMallocCount);

		for(i = 0; i < 1000; i++)    // 10 seconds ( 10 ms * 1000)
//		for(i = 0; i < 100; i++)    // 1 second ( 10 ms * 100)		
		{
			if(0 == g_TccMw_MemoryLeakCheck_ThreadRunning)
				break;

			usleep(10 * 1000);
		}
	}

	g_TccMw_MemoryLeakCheck_ThreadRunning = -1;

	return (void*)NULL; 
}

// Thread
////////////////////////////////////////////////////////////////////

#endif

void tcc_mw_memoryleakcheck_init(void)
{
#ifdef TCC_MW_MEMORYLEAKCHECK

	int i = 0;

	printf("[%s] start\n", __FUNCTION__);

	pthread_mutex_init(&g_TccMw_MemoryLeakCheck_Mutex, 0);

	for(i = 0; i < TCC_MW_MEMORYLEAKCHECK_STRUCT_MAX; i++)
	{
		g_TccMw_MemoryLeakCheck_Struct[i].usage = 0;
	}

	g_TccMw_MemoryLeakCheck_CurrentTotalMallocSize = 0;
	g_TccMw_MemoryLeakCheck_CurrentTotalMallocCount = 0;

	g_TccMw_MemoryLeakCheck_ThreadRunning = 1;
	tcc_dxb_thread_create((void *)&g_TccMw_MemoryLeakCheck_ThreadId, 
							tcc_mw_memoryleakcheck_thread, 
							(unsigned char*)"TccMw_MemoryLeakCheck_thread", 
							HIGH_PRIORITY_2, //LOW_PRIORITY_11, 
							NULL);

#endif
}

void tcc_mw_memoryleakcheck_term(void)
{
#ifdef TCC_MW_MEMORYLEAKCHECK

	int i = 0;
	int memoryLeakCount = 0;
	int totalMemoryLeak = 0;

	g_TccMw_MemoryLeakCheck_ThreadRunning = 0;
	while(1)
	{
		if(g_TccMw_MemoryLeakCheck_ThreadRunning == -1)
			break;

		usleep(5 * 1000);
	}

	pthread_mutex_lock(&g_TccMw_MemoryLeakCheck_Mutex);
	
	for(i = 0; i < TCC_MW_MEMORYLEAKCHECK_STRUCT_MAX; i++)
	{
		if (g_TccMw_MemoryLeakCheck_Struct[i].usage == 1)
		{
			memoryLeakCount++;
			printf("[%s] Memory Leak %d / ptr(0x%x), function(%s), line(%d), size(%d)\n", 
				__FUNCTION__, memoryLeakCount,
				g_TccMw_MemoryLeakCheck_Struct[i].ptr, g_TccMw_MemoryLeakCheck_Struct[i].functionName,
				g_TccMw_MemoryLeakCheck_Struct[i].line, g_TccMw_MemoryLeakCheck_Struct[i].size);

			totalMemoryLeak += g_TccMw_MemoryLeakCheck_Struct[i].size;
		}
	}

	printf("[%s] totalMemoryLeak(%d), Max(%d), g_TccMw_MemoryLeakCheck_HighestIndex(%d)\n",
		__FUNCTION__, totalMemoryLeak, TCC_MW_MEMORYLEAKCHECK_STRUCT_MAX, g_TccMw_MemoryLeakCheck_HighestIndex);

	pthread_mutex_unlock(&g_TccMw_MemoryLeakCheck_Mutex);
	pthread_mutex_destroy(&g_TccMw_MemoryLeakCheck_Mutex);

#endif
}

void* tcc_mw_malloc(const char* functionName, unsigned int line, unsigned int size)
{
	int i = 0;
	void *ptr = NULL;

	ptr = malloc(size);

#ifdef TCC_MW_MEMORYLEAKCHECK

	pthread_mutex_lock(&g_TccMw_MemoryLeakCheck_Mutex);

	for(i = 0; i < TCC_MW_MEMORYLEAKCHECK_STRUCT_MAX; i++)
	{
		if(g_TccMw_MemoryLeakCheck_Struct[i].usage == 0)
		{
			strncpy(g_TccMw_MemoryLeakCheck_Struct[i].functionName, functionName, TCC_MW_MEMORYLEAKCHECK_FILE_NAME_MAX);
			g_TccMw_MemoryLeakCheck_Struct[i].functionName[TCC_MW_MEMORYLEAKCHECK_FILE_NAME_MAX - 1] = 0; /* null terminated */
			g_TccMw_MemoryLeakCheck_Struct[i].line = line;
			g_TccMw_MemoryLeakCheck_Struct[i].size = size;
			g_TccMw_MemoryLeakCheck_Struct[i].ptr = ptr;
			g_TccMw_MemoryLeakCheck_Struct[i].usage = 1;

			g_TccMw_MemoryLeakCheck_CurrentTotalMallocSize += size;
			g_TccMw_MemoryLeakCheck_CurrentTotalMallocCount++;

			if(g_TccMw_MemoryLeakCheck_HighestIndex < i)
				g_TccMw_MemoryLeakCheck_HighestIndex = i;

			break;
		}
	}

	if (i == TCC_MW_MEMORYLEAKCHECK_STRUCT_MAX)
		printf("[%s] malloc / Critical Error Happened / Please check this log !!!!!\n", __FUNCTION__);

	pthread_mutex_unlock(&g_TccMw_MemoryLeakCheck_Mutex);

#endif

	return ptr;
}

void* tcc_mw_calloc(const char* functionName, unsigned int line, unsigned int isize_t, unsigned int size)
{
	int i = 0;
	void *ptr = NULL;

#ifdef TCC_DXB_MEMORY_CORRUPTION_CHECK
	ptr = malloc((isize_t * size) + 16);
	memset(ptr, 0, (isize_t * size) + 16);
		
	unsigned char* tempPtr = (unsigned char*)ptr;
	unsigned int temp = (isize_t * size);

	tempPtr[0] = 49;
	tempPtr[1] = 49;
	tempPtr[2] = 49;
	tempPtr[3] = 49;
	tempPtr[4] = (unsigned char)((temp & 0xFF000000) >> 24);
	tempPtr[5] = (unsigned char)((temp & 0x00FF0000) >> 16);
	tempPtr[6] = (unsigned char)((temp & 0x0000FF00) >> 8);
	tempPtr[7] = (unsigned char)((temp & 0x000000FF));
			
	tempPtr[8 + temp + 0] = tempPtr[0];
	tempPtr[8 + temp + 1] = tempPtr[1];
	tempPtr[8 + temp + 2] = tempPtr[2];
	tempPtr[8 + temp + 3] = tempPtr[3];
	tempPtr[8 + temp + 4] = tempPtr[4];
	tempPtr[8 + temp + 5] = tempPtr[5];
	tempPtr[8 + temp + 6] = tempPtr[6];
	tempPtr[8 + temp + 7] = tempPtr[7];

	ptr = ptr + 8;
		
	/*
	printf("[%s] / %s, %d, %d / head %d, %d, %d, %d / tail %d, %d, %d, %d\n", 
		__func__, functionName, line, size,
		tempPtr[0], tempPtr[1], tempPtr[2], tempPtr[3],
		tempPtr[size + 4 + 0], tempPtr[size + 4 + 1], tempPtr[size + 4 + 2], tempPtr[size + 4 + 3]);
	*/
#else
	ptr = calloc(isize_t, size);
#endif

#ifdef TCC_MW_MEMORYLEAKCHECK
	(void)pthread_mutex_lock(&g_TccMw_MemoryLeakCheck_Mutex);

	for (i = 0; i < TCC_MW_MEMORYLEAKCHECK_STRUCT_MAX; i++)
	{
		if (g_TccMw_MemoryLeakCheck_Struct[i].usage == 0)
		{
			strncpy(g_TccMw_MemoryLeakCheck_Struct[i].functionName, functionName, TCC_MW_MEMORYLEAKCHECK_FILE_NAME_MAX);
			g_TccMw_MemoryLeakCheck_Struct[i].functionName[TCC_MW_MEMORYLEAKCHECK_FILE_NAME_MAX - 1] = 0; /* null terminated */
			g_TccMw_MemoryLeakCheck_Struct[i].line = line;
			g_TccMw_MemoryLeakCheck_Struct[i].size = isize_t * size;
			g_TccMw_MemoryLeakCheck_Struct[i].ptr = ptr;
			g_TccMw_MemoryLeakCheck_Struct[i].usage = 1;

			g_TccMw_MemoryLeakCheck_CurrentTotalMallocSize += (isize_t * size);
			g_TccMw_MemoryLeakCheck_CurrentTotalMallocCount++;

			if (g_TccMw_MemoryLeakCheck_HighestTotalMallocSize < g_TccMw_MemoryLeakCheck_CurrentTotalMallocSize)
				g_TccMw_MemoryLeakCheck_HighestTotalMallocSize = g_TccMw_MemoryLeakCheck_CurrentTotalMallocSize;

			if (g_TccMw_MemoryLeakCheck_HighestTotalMallocCount < g_TccMw_MemoryLeakCheck_CurrentTotalMallocCount)
				g_TccMw_MemoryLeakCheck_HighestTotalMallocCount = g_TccMw_MemoryLeakCheck_CurrentTotalMallocCount;

			if (g_TccMw_MemoryLeakCheck_HighestIndex < i)
				g_TccMw_MemoryLeakCheck_HighestIndex = i;

			break;
		}
	}

	if (i == TCC_MW_MEMORYLEAKCHECK_STRUCT_MAX)
		printf("[%s] calloc / Critical Error Happened / Please check this log !!!!!\n", __FUNCTION__);

	pthread_mutex_unlock(&g_TccMw_MemoryLeakCheck_Mutex);
#endif

	return ptr;
}

void tcc_mw_free(const char* functionName, unsigned int line, void* ptr)
{
	int i = 0;

#ifdef TCC_MW_MEMORYLEAKCHECK

	pthread_mutex_lock(&g_TccMw_MemoryLeakCheck_Mutex);

	if(NULL == ptr)
	{
		printf("[%s] free 1 / Critical Error Happened / Please check this log !!!!!\n", __FUNCTION__);
		return ;
	}
	
	for(i = 0; i < TCC_MW_MEMORYLEAKCHECK_STRUCT_MAX; i++)
	{
		if(g_TccMw_MemoryLeakCheck_Struct[i].usage == 1 && g_TccMw_MemoryLeakCheck_Struct[i].ptr == ptr)
		{
			g_TccMw_MemoryLeakCheck_Struct[i].usage = 0;

			g_TccMw_MemoryLeakCheck_CurrentTotalMallocSize -= g_TccMw_MemoryLeakCheck_Struct[i].size;
			g_TccMw_MemoryLeakCheck_CurrentTotalMallocCount--;

			break;
		}
	}

	if(i == TCC_MW_MEMORYLEAKCHECK_STRUCT_MAX)
	{
		printf("[%s] free 2 / Critical Error Happened / Please check this log !!!!! / function %s, line %d\n",
			__FUNCTION__, functionName, line);
	}

	pthread_mutex_unlock(&g_TccMw_MemoryLeakCheck_Mutex);

#endif

	free(ptr);
}

