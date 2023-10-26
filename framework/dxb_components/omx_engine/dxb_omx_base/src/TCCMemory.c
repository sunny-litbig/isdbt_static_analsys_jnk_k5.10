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
#include "TCCMemory.h"
#define LOG_TAG	"TCCMemory"
#include <utils/Log.h>


//#define TCC_DXB_MEMORYLEAKCHECK


#ifdef TCC_DXB_MEMORYLEAKCHECK

#include <pthread.h>
#include <tcc_dxb_thread.h>

#define	DXB_MEMORYLEAKCHECK_STRUCT_MAX       256*1024
#define DXB_MEMORYLEAKCHECK_FILE_NAME_MAX    48

typedef struct _DxB_MemoryLeakCheck_Struct{
	char     functionName[DXB_MEMORYLEAKCHECK_FILE_NAME_MAX];
	int      line;
	int      size;
	int      usage;
	void*    ptr;
}DxB_MemoryLeakCheck_Struct;
static DxB_MemoryLeakCheck_Struct g_DxB_MemoryLeakCheck_Struct[DXB_MEMORYLEAKCHECK_STRUCT_MAX];
static unsigned int g_DxB_MemoryLeakCheck_CurrentTotalMallocSize = 0;
static pthread_mutex_t g_DxB_MemoryLeakCheck_Mutex;

static unsigned int g_DxB_MemoryLeakCheck_HighestIndex = 0;    // Highest index of the DxB_MemoryLeakCheck_Struct while checking memory leak



////////////////////////////////////////////////////////////////////
// Thread, to check current memory usage every 10 seconds

static pthread_t g_DxB_MemoryLeakCheck_ThreadId;
static int g_DxB_MemoryLeakCheck_ThreadRunning = 0;

void* tcc_dxb_fo_MemoryLeakCheck_thread(void *arg)
{
	int i = 0;

	while(g_DxB_MemoryLeakCheck_ThreadRunning)
	{
		printf("BDS / [%s] g_DxB_MemoryLeakCheck_CurrentTotalMallocSize(%d)\n", __FUNCTION__, g_DxB_MemoryLeakCheck_CurrentTotalMallocSize
		);

		for(i = 0; i < 1000; i++)    // 10 seconds ( 10 ms * 1000)
//		for(i = 0; i < 100; i++)    // 1 seconds ( 10 ms * 100)
		{
			if(0 == g_DxB_MemoryLeakCheck_ThreadRunning)
				break;

			usleep(10 * 1000);
		}
	}

	g_DxB_MemoryLeakCheck_ThreadRunning = -1;

	return (void*)NULL;
}

// Thread
////////////////////////////////////////////////////////////////////

#endif




void tcc_dxb_fo_MemoryLeakCheck_init(void)
{
#ifdef TCC_DXB_MEMORYLEAKCHECK	
	int i = 0;

	pthread_mutex_init(&g_DxB_MemoryLeakCheck_Mutex, 0);

	for(i = 0; i < DXB_MEMORYLEAKCHECK_STRUCT_MAX; i++)
	{
		g_DxB_MemoryLeakCheck_Struct[i].usage = 0;
	}

	g_DxB_MemoryLeakCheck_CurrentTotalMallocSize = 0;

	g_DxB_MemoryLeakCheck_ThreadRunning = 1;
	tcc_dxb_thread_create((void *)&g_DxB_MemoryLeakCheck_ThreadId,
							tcc_dxb_fo_MemoryLeakCheck_thread,
							(unsigned char*)"MemoryLeakCheck_fo_thread",
							HIGH_PRIORITY_2, //LOW_PRIORITY_11,
							NULL);

#endif
}


void tcc_dxb_fo_MemoryLeakCheck_term(void)
{
#ifdef TCC_DXB_MEMORYLEAKCHECK	
	int i = 0;
	int memoryLeakCount = 0;
	int totalMemoryLeak = 0;

	g_DxB_MemoryLeakCheck_ThreadRunning = 0;
	while(1)
	{
		if(g_DxB_MemoryLeakCheck_ThreadRunning == -1)
			break;

		usleep(5 * 1000);
	}

	pthread_mutex_lock(&g_DxB_MemoryLeakCheck_Mutex);

	for(i = 0; i < DXB_MEMORYLEAKCHECK_STRUCT_MAX; i++)
	{
		if (g_DxB_MemoryLeakCheck_Struct[i].usage == 1)
		{
			memoryLeakCount++;
			printf("\033[32m BDS / [%s]  / Memory Leak %d / ptr(0x%x), function(%s), line(%d), size(%d)  \033[m\n",
				__FUNCTION__, memoryLeakCount,
				g_DxB_MemoryLeakCheck_Struct[i].ptr, g_DxB_MemoryLeakCheck_Struct[i].functionName,
				g_DxB_MemoryLeakCheck_Struct[i].line, g_DxB_MemoryLeakCheck_Struct[i].size);

			totalMemoryLeak += g_DxB_MemoryLeakCheck_Struct[i].size;
		}
	}

	printf("\033[32m BDS / [%s] EEEEEEEEEEEEEEEEEEEEE / totalMemoryLeak(%d), Max(%d), g_DxB_MemoryLeakCheck_HighestIndex(%d)  \033[m\n",
	__FUNCTION__, totalMemoryLeak, DXB_MEMORYLEAKCHECK_STRUCT_MAX, g_DxB_MemoryLeakCheck_HighestIndex);

	pthread_mutex_unlock(&g_DxB_MemoryLeakCheck_Mutex);
	pthread_mutex_destroy(&g_DxB_MemoryLeakCheck_Mutex);
#endif
}

void* tcc_dxb_calloc(const char* functionName, unsigned int line, unsigned int isize_t, unsigned int iSize)
{
	int i = 0;
	void *ptr = NULL;

	ptr = calloc(isize_t, iSize);

#ifdef TCC_DXB_MEMORYLEAKCHECK

	pthread_mutex_lock(&g_DxB_MemoryLeakCheck_Mutex);

	for(i = 0; i < DXB_MEMORYLEAKCHECK_STRUCT_MAX; i++)
	{
		if(g_DxB_MemoryLeakCheck_Struct[i].usage == 0)
		{

			strncpy(g_DxB_MemoryLeakCheck_Struct[i].functionName, functionName, DXB_MEMORYLEAKCHECK_FILE_NAME_MAX);
			g_DxB_MemoryLeakCheck_Struct[i].functionName[DXB_MEMORYLEAKCHECK_FILE_NAME_MAX - 1] = 0; /* null terminated */
			g_DxB_MemoryLeakCheck_Struct[i].line = line;
			g_DxB_MemoryLeakCheck_Struct[i].size = iSize;
			g_DxB_MemoryLeakCheck_Struct[i].ptr = ptr;
			g_DxB_MemoryLeakCheck_Struct[i].usage = 1;

			g_DxB_MemoryLeakCheck_CurrentTotalMallocSize += iSize;

			if(g_DxB_MemoryLeakCheck_HighestIndex < i)
				g_DxB_MemoryLeakCheck_HighestIndex = i;

			break;
		}
	}

	if (i == DXB_MEMORYLEAKCHECK_STRUCT_MAX)
		printf("BDS / [%s] calloc / XXXXXXXXXXXXXXXXXXXXXX \n", __FUNCTION__);

	pthread_mutex_unlock(&g_DxB_MemoryLeakCheck_Mutex);

#endif

	return ptr;
}

void* tcc_dxb_malloc(const char* functionName, unsigned int line, unsigned int size)
{
	int i = 0;
	void *ptr = NULL;

	ptr = malloc(size);

#ifdef TCC_DXB_MEMORYLEAKCHECK

	pthread_mutex_lock(&g_DxB_MemoryLeakCheck_Mutex);

	for(i = 0; i < DXB_MEMORYLEAKCHECK_STRUCT_MAX; i++)
	{
		if(g_DxB_MemoryLeakCheck_Struct[i].usage == 0)
		{

			strncpy(g_DxB_MemoryLeakCheck_Struct[i].functionName, functionName, DXB_MEMORYLEAKCHECK_FILE_NAME_MAX);
			g_DxB_MemoryLeakCheck_Struct[i].functionName[DXB_MEMORYLEAKCHECK_FILE_NAME_MAX - 1] = 0; /* null terminated */
			g_DxB_MemoryLeakCheck_Struct[i].line = line;
			g_DxB_MemoryLeakCheck_Struct[i].size = size;
			g_DxB_MemoryLeakCheck_Struct[i].ptr = ptr;
			g_DxB_MemoryLeakCheck_Struct[i].usage = 1;

			g_DxB_MemoryLeakCheck_CurrentTotalMallocSize += size;

			if(g_DxB_MemoryLeakCheck_HighestIndex < i)
				g_DxB_MemoryLeakCheck_HighestIndex = i;

			break;
		}
	}

	if (i == DXB_MEMORYLEAKCHECK_STRUCT_MAX)
		printf("BDS / [%s] malloc / XXXXXXXXXXXXXXXXXXXXXX \n", __FUNCTION__);

	pthread_mutex_unlock(&g_DxB_MemoryLeakCheck_Mutex);

#endif

	return ptr;
}

int tcc_dxb_free(const char* functionName, unsigned int line, void* ptr)
{
	int i = 0;

#ifdef TCC_DXB_MEMORYLEAKCHECK

	pthread_mutex_lock(&g_DxB_MemoryLeakCheck_Mutex);

	if(NULL == ptr)
	{
		printf("BDS / [%s] free / XXXXXXXXXXXXXXXXXXXXXX \n", __FUNCTION__);
		return 0;
	}

	for(i = 0; i < DXB_MEMORYLEAKCHECK_STRUCT_MAX; i++)
	{
		if(g_DxB_MemoryLeakCheck_Struct[i].usage == 1 && g_DxB_MemoryLeakCheck_Struct[i].ptr == ptr)
		{

			g_DxB_MemoryLeakCheck_Struct[i].usage = 0;

			g_DxB_MemoryLeakCheck_CurrentTotalMallocSize -= g_DxB_MemoryLeakCheck_Struct[i].size;
			break;
		}
	}

	if(i == DXB_MEMORYLEAKCHECK_STRUCT_MAX)
	{
		printf("BDS / [%s] free / XXXXXXXXXXXXXXXXXXXXXX / function %s, line %d\n",
			__FUNCTION__, functionName, line);

	}

	pthread_mutex_unlock(&g_DxB_MemoryLeakCheck_Mutex);

#endif

	free(ptr);

	return 0;
}

void* TCC_fo_calloc(const char* functionName, unsigned int line, unsigned int isize_t, unsigned int iSize)
{
    return tcc_dxb_calloc(functionName, line, isize_t, iSize);
}

void* TCC_fo_malloc(const char* functionName, unsigned int line, unsigned int iSize)
{
    return tcc_dxb_malloc(functionName, line, iSize);
}

void TCC_fo_free(const char* functionName, unsigned int line, void *pvPtr)
{
    return tcc_dxb_free(functionName, line, pvPtr);
}



