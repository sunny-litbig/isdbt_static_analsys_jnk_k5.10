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
#include <tcc_ui_thread.h>

#if defined(HAVE_LINUX_PLATFORM)
#include <malloc.h>
#endif

struct tcc_ui_thread_handle {
	void (*thread_func)(void*);
	pthread_t pThread;
	void *arg;
};

void *tcc_ui_thread_func(void *arg)
{
	struct tcc_ui_thread_handle *handle = (struct tcc_ui_thread_handle*)arg;

	handle->thread_func(handle->arg);
	free(handle);

	return NULL;
}

void tcc_uithread_create(const char *name, void (*thread_func)(void*), void *arg, bool bJavaCall)
{
	struct tcc_ui_thread_handle *handle = (struct tcc_ui_thread_handle*)malloc(sizeof(struct tcc_ui_thread_handle));
	if(handle == NULL){
		return;
	}

	handle->arg = arg;
	handle->thread_func = thread_func;

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	pthread_create(&handle->pThread, &attr, tcc_ui_thread_func, handle);
	pthread_attr_destroy(&attr);
}
