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
#if defined(HAVE_LINUX_PLATFORM)
#define LOG_NDEBUG 0
#define LOG_TAG	"subtitle_memory"
#include <utils/Log.h>
#endif

#include <pthread.h>
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <subtitle_memory.h>
#include <subtitle_display.h>
#include <subtitle_debug.h>
#include <libpmap/pmap.h>
/****************************************************************************
DEFINITION
****************************************************************************/
#define MEMDBG		//LOGE

#define MEM_GET_MUTEX_CREATE	pthread_mutex_init (&g_get_mem_mutex, NULL);
#define MEM_GET_MUTEX_DESTROY	pthread_mutex_destroy (&g_get_mem_mutex);
#define MEM_GET_MUTEX_LOCK	pthread_mutex_lock (&g_get_mem_mutex);
#define MEM_GET_MUTEX_UNLOCK	pthread_mutex_unlock (&g_get_mem_mutex);

#define MEM_PUT_MUTEX_CREATE	pthread_mutex_init (&g_put_mem_mutex, NULL);
#define MEM_PUT_MUTEX_DESTROY	pthread_mutex_destroy (&g_put_mem_mutex);
#define MEM_PUT_MUTEX_LOCK	pthread_mutex_lock (&g_put_mem_mutex);
#define MEM_PUT_MUTEX_UNLOCK	pthread_mutex_unlock (&g_put_mem_mutex);

#define MEM_MUTEX_CREATE	pthread_mutex_init (&g_mem_mutex, NULL);
#define MEM_MUTEX_DESTROY	pthread_mutex_destroy (&g_mem_mutex);
#define MEM_MUTEX_LOCK	pthread_mutex_lock (&g_mem_mutex);
#define MEM_MUTEX_UNLOCK	pthread_mutex_unlock (&g_mem_mutex);

/****************************************************************************
DEFINITION OF EXTERNAL VARIABLES
****************************************************************************/

/****************************************************************************
DEFINITION OF GLOBAL VARIABLES
****************************************************************************/
static SUB_MEM_MGR_TYPE g_mem_mgr_type;
static pthread_mutex_t g_get_mem_mutex;
static pthread_mutex_t g_put_mem_mutex;
static pthread_mutex_t g_mem_mutex;
/****************************************************************************
DEFINITION OF LOCAL FUNCTIONS
****************************************************************************/
static SUB_MEM_MGR_TYPE* subtitle_memory_getctx(void)
{
	return &g_mem_mgr_type;
}

#if defined(HAVE_ANDROID_OS)
int subtitle_memory_create(int width, int height)
{
	SUB_MEM_MGR_TYPE *pMem;
	int i;
	int max_queue_num = 0, sub_plane_size;

	pMem = subtitle_memory_getctx();
	if(pMem->mem != NULL){
		LOGE("[%s] subtitle memory is already created.\n", __func__);
		return -1;
	}

	sub_plane_size = (width*height*sizeof(int));
	max_queue_num = 10;

	pMem->mem = (SUB_MEM_TYPE *)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(SUB_MEM_TYPE) * max_queue_num);
	if(pMem->mem == NULL){
		ALOGE("[%s] Not enough memory for context.", __func__);
		return -1;
	}

	pMem->vmem_addr = (unsigned int *)tcc_mw_malloc(__FUNCTION__, __LINE__, sub_plane_size*max_queue_num);
	if(pMem->vmem_addr == NULL){
		ALOGE("[%s] Not enough memory for subtitle memory.", __func__);
		return -1;
	}

	pMem->ccfb_w = width;
	pMem->ccfb_h = height;
	pMem->total = max_queue_num;
	pMem->cur_index = 0;

	for(i=0 ; i<max_queue_num ; i++)
	{
		pMem->mem[i].use = 0;
		pMem->mem[i].vir = (unsigned int *)(pMem->vmem_addr + (width*height*i));
		memset(pMem->mem[i].vir, 0x0, sub_plane_size);
		MEMDBG("[%s] vir:%p\n", __func__, pMem->mem[i].vir);
	}

	MEM_GET_MUTEX_CREATE
	MEM_PUT_MUTEX_CREATE

	return max_queue_num;
}
#endif

int subtitle_memory_linux_create(int width, int height)
{
    SUB_MEM_MGR_TYPE *pMem;
    int ccfb_fd = -1, i;
    int max_queue_num = 0, sub_plane_size;
    int page_size, n_page;
    unsigned int pmem_addr, vmem_addr, pmem_size;
    pmap_t pmem_info;

    LOGD("In [%s]\n", __func__);
    MEM_MUTEX_CREATE
    MEM_MUTEX_LOCK

#if defined(SUBTITLE_CCFB_DISPLAY_ENABLE)    
    ccfb_fd = subtitle_display_get_ccfb_fd();
    if(ccfb_fd == -1){
        LOGE("[%s] ccfb_fd is not initialized yet!\n", __func__);
        subtitle_display_set_memory_sync_flag(0);
        MEM_MUTEX_UNLOCK
        MEM_MUTEX_DESTROY
        return -1;
    }
#else
    ccfb_fd = subtitle_display_get_wmixer_fd();
    if(ccfb_fd == -1){
        LOGE("[%s] fd is not initialized yet!\n", __func__);
        subtitle_display_set_memory_sync_flag(0);
        MEM_MUTEX_UNLOCK
        MEM_MUTEX_DESTROY
        return -1;
    }	
#endif	

	pmem_info.base = 0;
	pmem_info.size = 0;
	
    //pmap_get_info("overlay1", &pmem_info);
    pmap_get_info("subtitle", &pmem_info);
    pmem_addr = pmem_info.base;
    pmem_size = pmem_info.size;

    page_size = getpagesize();

    if(pmem_size != 0 && page_size != 0){
    	n_page = (pmem_size / page_size);
    	if(pmem_size % page_size) n_page++;
    }

    pMem = subtitle_memory_getctx();

	if(pMem->mem == NULL){
		LOGE("[%s] subtitle memory destroy success\n", __func__);
	}

    sub_plane_size = (width*height*sizeof(unsigned int));
    if(sub_plane_size != 0 && pmem_size != 0){
    	max_queue_num = (pmem_size /sub_plane_size);
    }

    if(max_queue_num == 0) {
        LOGE("[%s] Not enough memory for subtitle plane.(0x%X,%d,%d,%d)\n", __func__,\
            pmem_addr, pmem_size, width, height);
        subtitle_display_set_memory_sync_flag(0);
        MEM_MUTEX_UNLOCK
        MEM_MUTEX_DESTROY
        return -1;
    }

	if(pmem_size != 0){
    	LOGD("[%s] addr:0x%08X, size:%d(KB), w:%d, h:%d, max_queue_num:%d\n", __func__, \
            pmem_addr, (pmem_size>>10), width, height, max_queue_num);
	}
    pMem->mem = (SUB_MEM_TYPE *)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(SUB_MEM_TYPE) * max_queue_num);
    if(pMem->mem  == NULL){
        LOGE("[%s] Not enough memory for context.\n", __func__);
        subtitle_display_set_memory_sync_flag(0);
        MEM_MUTEX_UNLOCK
        MEM_MUTEX_DESTROY
        return -1;
    }

    vmem_addr = (unsigned int)mmap(NULL, n_page*page_size, PROT_READ|PROT_WRITE,    MAP_SHARED,
                        ccfb_fd,  (off_t)pmem_addr);
    if (vmem_addr ==  (unsigned int)MAP_FAILED)
    {
        LOGE("mmap failed.(%p)\n", (unsigned int*)vmem_addr);
        if(pMem->mem ){
            tcc_mw_free(__FUNCTION__, __LINE__, pMem->mem);
            pMem->mem  = NULL;
        }
        subtitle_display_set_memory_sync_flag(0);
        MEM_MUTEX_UNLOCK
        MEM_MUTEX_DESTROY
        return -1;
    }

	pMem->vmap_addr = vmem_addr;
    pMem->vmap_size = pmem_size;    //vmem size is the same with pmem size
    pMem->ccfb_w = width;
    pMem->ccfb_h = height;
    pMem->total = max_queue_num-1;
    pMem->cur_index = 0;

    for(i=0 ; i<(max_queue_num-1) ; i++)
    {
        pMem->mem[i].use = 0;
        pMem->mem[i].phy = (unsigned int*)(pmem_addr + (sub_plane_size * i));
        pMem->mem[i].vir = (unsigned int*)(vmem_addr + (sub_plane_size * i));
    }

	//For clear
	pMem->mem[pMem->total].phy = (unsigned int*)(pmem_addr + (sub_plane_size * (pMem->total)));
	pMem->mem[pMem->total].vir = (unsigned int*)(vmem_addr + (sub_plane_size * (pMem->total)));
    memset(pMem->mem[pMem->total].vir, 0x0, sub_plane_size);
    ALOGE("[%s] clear_addr[%p]\n", __func__, pMem->mem[pMem->total].phy);

    subtitle_display_set_memory_sync_flag(1);

    MEM_GET_MUTEX_CREATE
    MEM_PUT_MUTEX_CREATE

	MEM_MUTEX_UNLOCK
    LOGD("Out [%s]\n", __func__);
    return max_queue_num;
}

int subtitle_memory_destroy(void)
{
	LOGD("In [%s]\n", __func__);

    SUB_MEM_MGR_TYPE *pMem = NULL;

    MEM_MUTEX_LOCK

    pMem = subtitle_memory_getctx();

#if 1
// Noah, To avoid Codesonar's warning, Redundant Condition
// subtitle_memory_getctx returns &g_mem_mgr_type and g_mem_mgr_type is not pointer.
	if(pMem->mem == NULL || subtitle_display_get_memory_sync_flag() == 0){
#else
	if(pMem == NULL || pMem->mem == NULL || subtitle_display_get_memory_sync_flag() == 0){
#endif
        LOGD("[%s] subtitle memory is not initialized yet!\n", __func__);
        subtitle_display_set_memory_sync_flag(0);
        MEM_MUTEX_UNLOCK
        MEM_MUTEX_DESTROY
        return -1;
    }

	subtitle_display_set_memory_sync_flag(0);

	tcc_mw_free(__FUNCTION__, __LINE__, pMem->mem);
	pMem->mem = NULL;

    if(subtitle_fb_type() == 0){
    	if(pMem->vmap_addr) munmap((void*)pMem->vmap_addr, pMem->vmap_size);
	}else{
	    if(pMem->vmem_addr) tcc_mw_free(__FUNCTION__, __LINE__, pMem->vmem_addr);
	}

	if(pMem){
		memset((void*)pMem, 0x0, sizeof(SUB_MEM_MGR_TYPE));
	}

    MEM_MUTEX_UNLOCK

	LOGD("Out [%s]\n", __func__);

    MEM_GET_MUTEX_DESTROY
    MEM_PUT_MUTEX_DESTROY
    MEM_MUTEX_DESTROY
    return 0;
}

int subtitle_memory_get_handle(void)
{
    SUB_MEM_MGR_TYPE *pMem;
    unsigned int handle, last_handle;

    MEM_GET_MUTEX_LOCK

    pMem = subtitle_memory_getctx();
    if(pMem->mem == NULL || subtitle_display_get_memory_sync_flag() == 0){
        MEM_GET_MUTEX_UNLOCK
        ALOGE("[%s] subtitle memory is not initialized yet!\n", __func__);
        return -1;
    }

    if(pMem->cur_index == pMem->total){
        pMem->cur_index = 0;
    }

    last_handle = pMem->cur_index;

RETRY:
    if(pMem->mem[pMem->cur_index].use == 0){
        pMem->mem[pMem->cur_index].use = 1;
        pMem->cur_index++;

        MEMDBG("[%s] get handle:%d\n", __func__, pMem->cur_index-1);
        MEM_GET_MUTEX_UNLOCK
        return pMem->cur_index-1;
    }else{
        pMem->cur_index++;
        if(pMem->cur_index == pMem->total){
            pMem->cur_index = 0;
    }

        if(last_handle == pMem->cur_index){
            goto END;
        }else{
            goto RETRY;
        }
    }

END:
    MEM_GET_MUTEX_UNLOCK

    MEMDBG("[%s] No free memory found.(max:%d)\n", __func__, pMem->total);
    return -1;
}

int subtitle_memory_put_handle(int handle)
{
    SUB_MEM_MGR_TYPE *pMem = subtitle_memory_getctx();
    if(pMem->mem == NULL || subtitle_display_get_memory_sync_flag() == 0){
        ALOGE("[%s] subtitle memory is not initialized yet!\n", __func__);
        return -1;
    }

    if(handle < 0){
        MEMDBG("[%s] Invalid handle : %d\n", __func__, handle);
        return -1;
    }

    MEMDBG("[%s] put handle : %d\n", __func__, handle);

    MEM_PUT_MUTEX_LOCK
    pMem->mem[handle].use = 0;
    MEM_PUT_MUTEX_UNLOCK

    return 0;
}

int subtitle_memory_get_clear_handle(void)
{
 	SUB_MEM_MGR_TYPE *pMem;
	unsigned int handle, last_handle;

	MEM_GET_MUTEX_LOCK

	pMem = subtitle_memory_getctx();
	if(pMem->mem == NULL || subtitle_display_get_memory_sync_flag() == 0){
		MEM_GET_MUTEX_UNLOCK
		ALOGE("[%s] subtitle memory is not initialized yet!\n", __func__);
		return -1;
	}

	MEM_GET_MUTEX_UNLOCK
	return pMem->total;
}

unsigned int* subtitle_memory_get_paddr(int handle)
{
    SUB_MEM_MGR_TYPE *pMem = subtitle_memory_getctx();
    if(pMem->mem == NULL || subtitle_display_get_memory_sync_flag() == 0){
        ALOGE("[%s] subtitle memory is not initialized yet!\n", __func__);
        return NULL;
    }

    if(handle < 0){
        MEMDBG("[%s] Invalid handle : %d\n", __func__, handle);
        return NULL;
    }

    //MEMDBG("[%s] get padd : %d\n", __func__, handle);

    return pMem->mem[handle].phy;
}

unsigned int* subtitle_memory_get_vaddr(int handle)
{
    SUB_MEM_MGR_TYPE *pMem = subtitle_memory_getctx();
    if(pMem->mem == NULL || subtitle_display_get_memory_sync_flag() == 0){
        ALOGE("[%s] subtitle memory is not initialized yet!\n", __func__);
        return NULL;
    }

    if(handle < 0){
        MEMDBG("[%s] Invalid handle : %d\n", __func__, handle);
        return NULL;
    }

	//MEMDBG("[%s] get vaddr : %d\n", __func__, handle);
    return pMem->mem[handle].vir;
}

unsigned int subtitle_memory_sub_width(void)
{
    SUB_MEM_MGR_TYPE *pMem = subtitle_memory_getctx();
    if(pMem->mem == NULL || subtitle_display_get_memory_sync_flag() == 0){
        ALOGE("[%s] subtitle memory is not initialized yet!\n", __func__);
        return 0;
    }

    return pMem->ccfb_w;
}

unsigned int subtitle_memory_sub_height(void)
{
    SUB_MEM_MGR_TYPE *pMem = subtitle_memory_getctx();
    if(pMem->mem == NULL || subtitle_display_get_memory_sync_flag() == 0){
        ALOGE("[%s] subtitle memory is not initialized yet!\n", __func__);
        return 0;
    }

    return pMem->ccfb_h;
}

unsigned int subtitle_memory_sub_clear(int handle)
{
    SUB_MEM_MGR_TYPE *pMem = subtitle_memory_getctx();
    unsigned int size = 0;
    unsigned int *p = NULL;

    if(handle < 0){
        MEMDBG("[%s] Invalid handle : %d\n", __func__, handle);
        return -1;
    }

    size = (pMem->ccfb_w * pMem->ccfb_h * sizeof(unsigned int));

    if(size != 0){
        p = subtitle_memory_get_vaddr(handle);
        if(p){
            memset(p, 0x0, size);
            return 0;
        }
    }
    return -1;
}

void subtitle_memory_get_used_count(const char *file, int line)
{
    SUB_MEM_MGR_TYPE *pMem;
    unsigned int handle, i, used_count;

    pMem = subtitle_memory_getctx();
    if(pMem->mem == NULL){
        MEMDBG("[%s] subtitle memory is not initialized yet!\n", __func__);
        return;
    }

    used_count = 0;

    for(i=0 ; i < pMem->total ; i++)
    {
        if(pMem->mem[i].use == 1){
            used_count++;
        }
    }

    MEMDBG("[%s:%d] used_count(%d/%d)\n", file, line, used_count, pMem->total);
}

int subtitle_memory_memset(int handle, int size){
    SUB_MEM_MGR_TYPE *pMem;
	unsigned int *vir = NULL;

	MEM_MUTEX_LOCK
    pMem = subtitle_memory_getctx();
    if(pMem->mem == NULL){
        ALOGE("subtitle memory not prepared\n", __func__);
        MEM_MUTEX_UNLOCK
        return -1;
    }

    vir = subtitle_memory_get_vaddr(handle);
	if(vir != NULL && size != 0){
		ALOGE("In [%s] [%p] [%d] [%d]\n", __func__, vir, size, handle);
		memset((void*)vir, 0x0, (sizeof(int)*size));
		ALOGE("Out [%s]\n", __func__);
	}

	MEM_MUTEX_UNLOCK
	return 0;
}
