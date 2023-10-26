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
#ifndef __SUBTITLE_MEMORY_H__
#define __SUBTITLE_MEMORY_H__

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
DEFINITION OF STRUCTURE
****************************************************************************/
typedef struct{
	unsigned int use;
	unsigned int *phy;
	unsigned int *vir;
}SUB_MEM_TYPE;
	
typedef struct{
	unsigned int *vmem_addr;
	unsigned int vmap_addr;
	unsigned int vmap_size;
	unsigned int ccfb_w;
	unsigned int ccfb_h;
	unsigned int total;
	unsigned int cur_index;
	SUB_MEM_TYPE *mem;
}SUB_MEM_MGR_TYPE;

extern int subtitle_memory_create(int width, int height);
extern int subtitle_memory_linux_create(int width, int height);
extern int subtitle_memory_destroy(void);
extern int subtitle_memory_get_handle(void);
extern int subtitle_memory_put_handle(int handle);
extern unsigned int* subtitle_memory_get_vaddr(int handle);
extern unsigned int* subtitle_memory_get_paddr(int handle);
extern unsigned int subtitle_memory_sub_width(void);
extern unsigned int subtitle_memory_sub_height(void);
extern unsigned int subtitle_memory_sub_clear(int handle);
extern void subtitle_memory_get_used_count(const char *file, int line);
extern int subtitle_memory_get_clear_handle(void);
extern int subtitle_memory_memset(int handle, int size);
#ifdef __cplusplus
}
#endif

#endif	/* __SUBTITLE_MEMORY_H__ */

