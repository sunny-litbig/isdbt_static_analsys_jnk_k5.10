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
#define LOG_TAG	"subtitle_queue"
#include <utils/Log.h>
#endif
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <subtitle_queue.h>
#include <subtitle_debug.h>


/****************************************************************************
DEFINITION
****************************************************************************/
typedef struct _dnode
{
	int handle;
	unsigned long long pts;
	int flash_handle;
	int x;
	int y;
	int w;
	int h;
	struct _dnode *prev;
	struct _dnode *next;
} dnode;

typedef struct _disp_dnode
{
	int handle;
	unsigned long long pts;
	int png_flag;
	struct _disp_dnode *prev;
	struct _disp_dnode *next;
} disp_dnode;

/****************************************************************************
DEFINITION OF EXTERNAL VARIABLES
****************************************************************************/



/****************************************************************************
DEFINITION OF GLOBAL VARIABLES
****************************************************************************/
static dnode *st_head, *st_tail; //subtitle
static dnode *si_head, *si_tail; //superimpose
static disp_dnode *disp_head, *disp_tail; // setting for bitmap and disp handle
static pthread_mutex_t st_mutex;
static pthread_mutex_t si_mutex;
static pthread_mutex_t disp_mutex;
static pthread_mutex_t mutex;


/****************************************************************************
DEFINITION OF LOCAL FUNCTIONS
****************************************************************************/
static int subtitle_st_queue_init(void)
{
	st_head = (dnode*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(dnode));
	if(st_head == NULL){
		LOGE("[%s] Not enough memory for qHead.\n", __func__);
		return -1;
	}
	st_tail = (dnode*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(dnode));
	if(st_tail == NULL){
		LOGE("[%s] Not enough memory for qTail.\n", __func__);
		tcc_mw_free(__FUNCTION__, __LINE__, st_head);
		return -1;
	}

	pthread_mutex_init (&st_mutex, NULL);

	pthread_mutex_lock (&st_mutex);
	st_head->prev = st_head;
	st_head->next = st_tail;
	st_tail->prev = st_head;
	st_tail->next = st_tail;
	pthread_mutex_unlock (&st_mutex);

	return 0;
}

static int subtitle_si_queue_init(void)
{
	si_head = (dnode*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(dnode));
	if(si_head == NULL){
		LOGE("[%s] Not enough memory for qHead.\n", __func__);
		return -1;
	}
	si_tail = (dnode*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(dnode));
	if(si_tail == NULL){
		LOGE("[%s] Not enough memory for qTail.\n", __func__);
		tcc_mw_free(__FUNCTION__, __LINE__, si_head);
		return -1;
	}

	pthread_mutex_init (&si_mutex, NULL);

	pthread_mutex_lock (&si_mutex);
	si_head->prev = si_head;
	si_head->next = si_tail;
	si_tail->prev = si_head;
	si_tail->next = si_tail;
	pthread_mutex_unlock (&si_mutex);

	return 0;
}

static int subtitle_disp_queue_init(void)
{
	disp_head = (disp_dnode*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(disp_dnode));
	if(disp_head == NULL){
		LOGE("[%s] Not enough memory for qHead.\n", __func__);
		return -1;
	}
	disp_tail = (disp_dnode*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(disp_dnode));
	if(disp_tail == NULL){
		LOGE("[%s] Not enough memory for qTail.\n", __func__);
		tcc_mw_free(__FUNCTION__, __LINE__, disp_head);
		return -1;
	}

	pthread_mutex_init (&disp_mutex, NULL);

	pthread_mutex_lock (&disp_mutex);
	disp_head->prev = disp_head;
	disp_head->next = disp_tail;
	disp_tail->prev = disp_head;
	disp_tail->next = disp_tail;
	pthread_mutex_unlock (&disp_mutex);

	return 0;
}

static void subtitle_st_queue_exit(void)
{
	dnode *t = NULL;
	dnode *s = NULL;

	if((st_head == NULL) ||(st_tail == NULL)){
		LOGE("[%s] subtitle queue is not initialized yet!\n", __func__);
		return;
	}

	pthread_mutex_lock (&st_mutex);
	t = st_head->next;
	while (t != st_tail)
	{
		s = t;
		t = t->next;
		tcc_mw_free(__FUNCTION__, __LINE__, s);
	}
	st_head->next = st_tail;
	st_tail->prev = st_head;

	tcc_mw_free(__FUNCTION__, __LINE__, st_head);
	tcc_mw_free(__FUNCTION__, __LINE__, st_tail);

	pthread_mutex_unlock (&st_mutex);

	pthread_mutex_destroy (&st_mutex);
}

static void subtitle_si_queue_exit(void)
{
	dnode *t = NULL;
	dnode *s = NULL;

	if((si_head == NULL) ||(si_tail == NULL)){
		LOGE("[%s] subtitle queue is not initialized yet!\n", __func__);
		return;
	}

	pthread_mutex_lock (&si_mutex);
	t = si_head->next;
	while (t != si_tail)
	{
		s = t;
		t = t->next;
		tcc_mw_free(__FUNCTION__, __LINE__, s);
	}
	si_head->next = si_tail;
	si_tail->prev = si_head;

	tcc_mw_free(__FUNCTION__, __LINE__, si_head);
	tcc_mw_free(__FUNCTION__, __LINE__, si_tail);

	pthread_mutex_unlock (&si_mutex);

	pthread_mutex_destroy (&si_mutex);
}

static void subtitle_disp_queue_exit(void)
{
	disp_dnode *t;
	disp_dnode *s;

	if((disp_head == NULL) ||(disp_tail == NULL)){
		LOGE("[%s] subtitle queue is not initialized yet!\n", __func__);
		return;
	}

	pthread_mutex_lock (&disp_mutex);
	t = disp_head->next;
	while (t != disp_tail)
	{
		s = t;
		t = t->next;
		tcc_mw_free(__FUNCTION__, __LINE__, s);
	}
	disp_head->next = disp_tail;
	disp_tail->prev = disp_head;

	tcc_mw_free(__FUNCTION__, __LINE__, disp_head);
	tcc_mw_free(__FUNCTION__, __LINE__, disp_tail);

	pthread_mutex_unlock (&disp_mutex);

	pthread_mutex_destroy (&disp_mutex);
}

static int subtitle_st_queue_put(int handle, int x, int y, int w, int h, unsigned long long pts, int flash_handle)
{
	dnode *t;

	if((st_head == NULL) ||(st_tail == NULL)){
		LOGE("[%s] subtitle queue is not initialized yet!\n", __func__);
		return -1;
	}

	if ((t = (dnode*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(dnode))) == NULL)
	{
		LOGE("[%s] Not enough memory.\n", __func__);
		return -1;
	}

	t->handle = handle;
	t->pts = pts;
	t->flash_handle = flash_handle;
	t->x = x;
	t->y = y;
	t->w = w;
	t->h = h;

	pthread_mutex_lock (&st_mutex);
	st_tail->prev->next = t;
	t->prev = st_tail->prev;
	st_tail->prev = t;
	t->next = st_tail;
	pthread_mutex_unlock (&st_mutex);

	return 0;
}

static int subtitle_si_queue_put(int handle, int x, int y, int w, int h, unsigned long long pts, int flash_handle)
{
	dnode *t;

	if((si_head == NULL) ||(si_tail == NULL)){
		LOGE("[%s] subtitle queue is not initialized yet!\n", __func__);
		return -1;
	}

	if ((t = (dnode*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(dnode))) == NULL)
	{
		LOGE("[%s] Not enough memory.\n", __func__);
		return -1;
	}

	t->handle = handle;
	t->pts = pts;
	t->flash_handle = flash_handle;
	t->x = x;
	t->y = y;
	t->w = w;
	t->h = h;

	pthread_mutex_lock (&si_mutex);
	si_tail->prev->next = t;
	t->prev = si_tail->prev;
	si_tail->prev = t;
	t->next = si_tail;
	pthread_mutex_unlock (&si_mutex);

	return 0;
}

static int subtitle_st_queue_put_first(int handle, int x, int y, int w, int h, unsigned long long pts, int flash_handle)
{
	dnode *t;

	if((st_head == NULL) ||(st_tail == NULL)){
		LOGE("[%s] subtitle queue is not initialized yet!\n", __func__);
		return -1;
	}

	if ((t = (dnode*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(dnode))) == NULL)
	{
		LOGE("[%s] Not enough memory.\n", __func__);
		return -1;
	}

	t->handle = handle;
	t->pts = pts;
	t->flash_handle = flash_handle;
	t->x = x;
	t->y = y;
	t->w = w;
	t->h = h;

	pthread_mutex_lock (&st_mutex);

	t->prev = st_head;
	t->next = st_head->next;
	st_head->next = t;

	pthread_mutex_unlock (&st_mutex);

	return 0;
}

static int subtitle_si_queue_put_first(int handle, int x, int y, int w, int h, unsigned long long pts, int flash_handle)
{
	dnode *t;

	if((si_head == NULL) ||(si_tail == NULL)){
		LOGE("[%s] subtitle queue is not initialized yet!\n", __func__);
		return -1;
	}

	if ((t = (dnode*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(dnode))) == NULL)
	{
		LOGE("[%s] Not enough memory.\n", __func__);
		return -1;
	}

	t->handle = handle;
	t->pts = pts;
	t->flash_handle = flash_handle;
	t->x = x;
	t->y = y;
	t->w = w;
	t->h = h;

	pthread_mutex_lock (&si_mutex);
	t->prev = si_head;
	t->next = si_head->next;
	si_head->next = t;
	pthread_mutex_unlock (&si_mutex);

	return 0;
}

static int subtitle_st_queue_get(int *p_handle, int *p_x, int *p_y, int *p_w, int *p_h, unsigned long long *p_pts, int *p_flash_handle)
{
	dnode *t = NULL;

	if((st_head == NULL) ||(st_tail == NULL)){
		//LOGE("[%s] subtitle queue is not initialized yet!\n", __func__);
		return -1;
	}

	pthread_mutex_lock (&st_mutex);
	t = st_head->next;

	if(t == NULL){
		pthread_mutex_unlock (&st_mutex);
		return -1;
	}

	if (t == st_tail)
	{
		//LOGE("[%s] There is no data.\n", __func__);
		pthread_mutex_unlock (&st_mutex);
		return -2;
	}

	*p_handle = t->handle;
	*p_pts = t->pts;
	*p_flash_handle = t->flash_handle;
	*p_x = t->x;
	*p_y = t->y;
	*p_w = t->w;
	*p_h = t->h;

	st_head->next = t->next;
	t->next->prev = st_head;

	tcc_mw_free(__FUNCTION__, __LINE__, t);

	pthread_mutex_unlock (&st_mutex);

	return 0;
}

static int subtitle_si_queue_get(int *p_handle, int *p_x, int *p_y, int *p_w, int *p_h, unsigned long long *p_pts, int *p_flash_handle)
{
	dnode *t = NULL;

	if((si_head == NULL) ||(si_tail == NULL)){
		//LOGE("[%s] subtitle queue is not initialized yet!\n", __func__);
		return -1;
	}

	pthread_mutex_lock (&si_mutex);
	t = si_head->next;
	if(t == NULL){
		pthread_mutex_unlock (&si_mutex);
		return -1;
	}

	if (t == si_tail)
	{
		//LOGE("[%s] There is no data.\n", __func__);
		pthread_mutex_unlock (&si_mutex);
		return -2;
	}

	*p_handle = t->handle;
	*p_pts = t->pts;
	*p_flash_handle = t->flash_handle;
	*p_x = t->x;
	*p_y = t->y;
	*p_w = t->w;
	*p_h = t->h;

	si_head->next = t->next;
	t->next->prev = si_head;

	tcc_mw_free(__FUNCTION__, __LINE__, t);

	pthread_mutex_unlock (&si_mutex);

	return 0;
}

static int subtitle_st_queue_peek(int *p_handle, int *p_x, int *p_y, int *p_w, int *p_h, unsigned long long *p_pts, int *p_flash_handle)
{
	dnode *t;

	if((st_head == NULL) ||(st_tail == NULL)){
		//LOGE("[%s] subtitle queue is not initialized yet!\n", __func__);
		return -1;
	}

	pthread_mutex_lock (&st_mutex);
	t = st_head->next;
	if (t == st_tail)
	{
		//LOGE("[%s] There is no data.\n", __func__);
		pthread_mutex_unlock (&st_mutex);
		return -2;
	}

	*p_handle = t->handle;
	*p_pts = t->pts;
	*p_flash_handle = t->flash_handle;
	*p_x = t->x;
	*p_y = t->y;
	*p_w = t->w;
	*p_h = t->h;
	pthread_mutex_unlock (&st_mutex);

	return 0;
}

static int subtitle_si_queue_peek(int *p_handle, int *p_x, int *p_y, int *p_w, int *p_h, unsigned long long *p_pts, int *p_flash_handle)
{
	dnode *t;

	if((si_head == NULL) ||(si_tail == NULL)){
		//LOGE("[%s] subtitle queue is not initialized yet!\n", __func__);
		return -1;
	}

	pthread_mutex_lock (&si_mutex);
	t = si_head->next;
	if (t == si_tail)
	{
		//LOGE("[%s] There is no data.\n", __func__);
		pthread_mutex_unlock (&si_mutex);
		return -2;
	}

	*p_handle = t->handle;
	*p_pts = t->pts;
	*p_flash_handle = t->flash_handle;
	*p_x = t->x;
	*p_y = t->y;
	*p_w = t->w;
	*p_h = t->h;
	pthread_mutex_unlock (&si_mutex);

	return 0;
}

static int subtitle_st_queue_remove_all(void)
{
	dnode *t;
	dnode *s;

	if((st_head == NULL) ||(st_tail == NULL)){
		LOGE("[%s] subtitle queue is not initialized yet!\n", __func__);
		return -1;
	}

	pthread_mutex_lock (&st_mutex);
	t = st_head->next;
	while (t != st_tail)
	{
		s = t;
		t = t->next;
		tcc_mw_free(__FUNCTION__, __LINE__, s);
	}
	st_head->next = st_tail;
	st_tail->prev = st_head;

	pthread_mutex_unlock (&st_mutex);

	return 0;
}

static int subtitle_si_queue_remove_all(void)
{
	dnode *t;
	dnode *s;

	if((si_head == NULL) ||(si_tail == NULL)){
		LOGE("[%s] subtitle queue is not initialized yet!\n", __func__);
		return -1;
	}

	pthread_mutex_lock (&si_mutex);
	t = si_head->next;
	while (t != si_tail)
	{
		s = t;
		t = t->next;
		tcc_mw_free(__FUNCTION__, __LINE__, s);
	}
	si_head->next = si_tail;
	si_tail->prev = si_head;

	pthread_mutex_unlock (&si_mutex);

	return 0;
}

int subtitle_queue_remove_disp(void)
{
	disp_dnode *t;
	disp_dnode *s;

	if((disp_head == NULL) ||(disp_tail == NULL)){
		LOGE("[%s] subtitle queue is not initialized yet!\n", __func__);
		return -1;
	}

	pthread_mutex_lock (&disp_mutex);
	t = disp_head->next;
	while (t != disp_tail)
	{
		s = t;
		t = t->next;
		tcc_mw_free(__FUNCTION__, __LINE__, s);
	}
	disp_head->next = disp_tail;
	disp_tail->prev = disp_head;

	pthread_mutex_unlock (&disp_mutex);

	return 0;
}

/******************************************************************************************/
int subtitle_queue_init(void)
{
	int err = -1;

	if(subtitle_st_queue_init()){
		LOGE("[%s] st_queue init fail\n", __func__);
		return -1;
	}
	if(subtitle_si_queue_init()){
		LOGE("[%s] st_queue init fail\n", __func__);
		return -1;
	}
	return 0;
}

void subtitle_queue_exit(void)
{
	subtitle_st_queue_exit();
	subtitle_si_queue_exit();
}

int subtitle_queue_put(int type, int handle, int x, int y, int w, int h, unsigned long long pts, int flash_handle)
{
	int err = -1;

	if(type == 0){
		err = subtitle_st_queue_put(handle, x, y, w, h, pts, flash_handle);
	}else if(type == 1){
		err = subtitle_si_queue_put(handle, x, y, w, h, pts, flash_handle);
	}else{
		LOGE("[%s] invalid type(%d)\n", __func__, type);
	}

	return err;
}

int subtitle_queue_put_first(int type, int handle, int x, int y, int w, int h, unsigned long long pts, int flash_handle)
{
	int err = -1;

	if(type == 0){
		err = subtitle_st_queue_put_first(handle, x, y, w, h, pts, flash_handle);
	}else if(type == 1){
		err = subtitle_si_queue_put_first(handle, x, y, w, h, pts, flash_handle);
	}else{
		LOGE("[%s] invalid type(%d)\n", __func__, type);
	}

	return err;
}

int subtitle_queue_get(int type, int *p_handle, int *p_x, int *p_y, int *p_w, int *p_h, unsigned long long *p_pts, int *p_flash_handle)
{
	int err = -1;

	if(type == 0){
		err = subtitle_st_queue_get(p_handle, p_x, p_y, p_w, p_h, p_pts, p_flash_handle);
	}else if(type == 1){
		err = subtitle_si_queue_get(p_handle, p_x, p_y, p_w, p_h, p_pts, p_flash_handle);
	}else{
		LOGE("[%s] invalid type(%d)\n", __func__, type);
	}

	return err;
}

int subtitle_queue_peek(int type, int *p_handle, int *p_x, int *p_y, int *p_w, int *p_h, unsigned long long *p_pts, int *p_flash_handle)
{
	int err = -1;

	if(type == 0){
		err = subtitle_st_queue_peek(p_handle, p_x, p_y, p_w, p_h, p_pts, p_flash_handle);
	}else if(type == 1){
		err = subtitle_si_queue_peek(p_handle, p_x, p_y, p_w, p_h, p_pts, p_flash_handle);
	}else{
		LOGE("[%s] invalid type(%d)\n", __func__, type);
	}

	return err;
}

int subtitle_queue_remove_all(int type)
{
	int err = -1;

	if(type == 0){
		err = subtitle_st_queue_remove_all();
	}else{
		err = subtitle_si_queue_remove_all();
	}
	return err;
}

void subtitle_queue_print(int type)
{
	dnode *t;
	dnode *pHead, *pTail;

	pHead = (type==0)?st_head:si_head;
	pTail = (type==0)?st_tail:si_tail;

	if((pHead == NULL) ||(pTail == NULL)){
		LOGE("[%s] subtitle queue is not initialized yet!\n", __func__);
		return;
	}

	t = pHead->next;
	LOGE("[%s] type:%d, Queue contents : Front ----> Rear\n", __func__, type);
	while (t != pTail)
	{
		LOGE("%d, %lld\n", t->handle, t->pts);
		t = t->next;
	}
}

/******************************************************************************************/
int subtitle_queue_put_disp(int handle, unsigned long long pts, int png_flag)
{
	disp_dnode *t;

	if((disp_head == NULL) ||(disp_tail == NULL)){
		LOGE("[%s] subtitle queue is not initialized yet!\n", __func__);
		return -1;
	}

	if ((t = (disp_dnode*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(disp_dnode))) == NULL)
	{
		//LOGE("[%s] Not enough memory.\n", __func__);
		return -1;
	}

	t->handle = handle;
	t->pts = pts;
	t->png_flag = png_flag;

	pthread_mutex_lock (&disp_mutex);
	disp_tail->prev->next = t;
	t->prev = disp_tail->prev;
	disp_tail->prev = t;
	t->next = disp_tail;
	pthread_mutex_unlock (&disp_mutex);

	return 0;
}

int subtitle_queue_get_disp(int *p_handle, unsigned long long *p_pts, int *p_png_flag)
{
	disp_dnode *t = NULL;

	if((disp_head == NULL) ||(disp_tail == NULL)){
		LOGE("[%s] subtitle queue is not initialized yet!\n", __func__);
		return -1;
	}

	pthread_mutex_lock (&disp_mutex);
	t = disp_head->next;
	if(t == NULL){
		pthread_mutex_unlock (&disp_mutex);
		return -1;
	}

	if (t == disp_tail)
	{
		//LOGE("[%s] There is no data.\n", __func__);
		pthread_mutex_unlock (&disp_mutex);
		return -2;
	}

	*p_handle = t->handle;
	*p_pts = t->pts;
	*p_png_flag = t->png_flag;
	disp_head->next = t->next;
	t->next->prev = disp_head;

	tcc_mw_free(__FUNCTION__, __LINE__, t);

	pthread_mutex_unlock (&disp_mutex);

	return 0;
}
