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
#define LOG_TAG	"subtitle_queue_pos"
#include <utils/Log.h>
#endif
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <subtitle_queue_pos.h>
#include <subtitle_debug.h>


/****************************************************************************
DEFINITION
****************************************************************************/
typedef struct _dpos
{
	int x;
	int y;
	int w;
	int h;
	struct _dpos *prev;
	struct _dpos *next;
} dpos;

/****************************************************************************
DEFINITION OF EXTERNAL VARIABLES
****************************************************************************/


/****************************************************************************
DEFINITION OF GLOBAL VARIABLES
****************************************************************************/
static dpos *st_head, *st_tail;
static dpos *si_head, *si_tail;
static int g_st_pos_total_cnt;
static int g_si_pos_total_cnt;
static pthread_mutex_t st_mutex;
static pthread_mutex_t si_mutex;


/****************************************************************************
DEFINITION OF LOCAL FUNCTIONS
****************************************************************************/
static int subtitle_st_queue_pos_init(void)
{
	st_head = (dpos*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(dpos));
	if(st_head == NULL){
		LOGE("[%s] Not enough memory for qHead.\n", __func__);
		return -1;
	}
	st_tail = (dpos*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(dpos));
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

static int subtitle_si_queue_pos_init(void)
{
	si_head = (dpos*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(dpos));
	if(si_head == NULL){
		LOGE("[%s] Not enough memory for qHead.\n", __func__);
		return -1;
	}
	si_tail = (dpos*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(dpos));
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

static void subtitle_st_queue_pos_exit(void)
{
	dpos *t = NULL;
	dpos *s = NULL;

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

static void subtitle_si_queue_pos_exit(void)
{
	dpos *t = NULL;
	dpos *s = NULL;

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

int subtitle_st_queue_pos_put(int x, int y, int w, int h)
{
	dpos *t;

	if((st_head == NULL) ||(st_tail == NULL)){
		LOGE("[%s] subtitle queue is not initialized yet!\n", __func__);
		return -1;
	}

	if ((t = (dpos*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(dpos))) == NULL)
	{
		LOGE("[%s] Not enough memory.\n", __func__);
		return -1;
	}

	t->x = x;
	t->y = y;
	t->w = w;
	t->h = h;

	pthread_mutex_lock (&st_mutex);
	st_tail->prev->next = t;
	t->prev = st_tail->prev;
	st_tail->prev = t;
	t->next = st_tail;

	g_st_pos_total_cnt++;
	pthread_mutex_unlock (&st_mutex);

	return 0;
}

int subtitle_si_queue_pos_put(int x, int y, int w, int h)
{
	dpos *t;

	if((si_head == NULL) ||(si_tail == NULL)){
		LOGE("[%s] subtitle queue is not initialized yet!\n", __func__);
		return -1;
	}

	if ((t = (dpos*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(dpos))) == NULL)
	{
		LOGE("[%s] Not enough memory.\n", __func__);
		return -1;
	}

	t->x = x;
	t->y = y;
	t->w = w;
	t->h = h;

	pthread_mutex_lock (&si_mutex);
	si_tail->prev->next = t;
	t->prev = si_tail->prev;
	si_tail->prev = t;
	t->next = si_tail;

	g_si_pos_total_cnt++;
	pthread_mutex_unlock (&si_mutex);

	return 0;
}

int subtitle_st_queue_pos_get(int *x, int *y, int *w, int *h)
{
	dpos *t;
	int i;

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
		return -1;
	}

	*x = t->x;
	*y = t->y;
	*w = t->w;
	*h = t->h;

	st_head->next = t->next;
	t->next->prev = st_head;
	tcc_mw_free(__FUNCTION__, __LINE__, t);

	g_st_pos_total_cnt--;
	pthread_mutex_unlock (&st_mutex);

	return 0;
}

int subtitle_si_queue_pos_get(int *x, int *y, int *w, int *h)
{
	dpos *t;
	int i;

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
		return -1;
	}

	*x = t->x;
	*y = t->y;
	*w = t->w;
	*h = t->h;

	si_head->next = t->next;
	t->next->prev = si_head;
	tcc_mw_free(__FUNCTION__, __LINE__, t);

	g_si_pos_total_cnt--;
	pthread_mutex_unlock (&si_mutex);

	return 0;
}

int subtitle_st_queue_pos_peek_nth(int index, int *x, int *y, int *w, int *h)
{
	dpos *t;
	int i;

	if((st_head == NULL) ||(st_tail == NULL)){
		//LOGE("[%s] subtitle queue is not initialized yet!\n", __func__);
		return -1;
	}

	if((g_st_pos_total_cnt == 0) || (index >= g_st_pos_total_cnt) || (index < 0)) {
		return -2;
	}

	pthread_mutex_lock (&st_mutex);
	for(i=0, t = st_head->next ; (i<g_st_pos_total_cnt && t != st_tail) ; i++, t = t->next)
	{
		if(i == index){
			*x = t->x;
			*y = t->y;
			*w = t->w;
			*h = t->h;
			break;
		}
	}
	pthread_mutex_unlock (&st_mutex);

	return 0;
}

int subtitle_si_queue_pos_peek_nth(int index, int *x, int *y, int *w, int *h)
{
	dpos *t;
	int i;

	if((si_head == NULL) ||(si_tail == NULL)){
		//LOGE("[%s] subtitle queue is not initialized yet!\n", __func__);
		return -1;
	}

	if((g_si_pos_total_cnt == 0) || (index >= g_si_pos_total_cnt) || (index < 0)) {
		return -2;
	}

	pthread_mutex_lock (&si_mutex);
	for(i=0, t = si_head->next ; (i<g_si_pos_total_cnt && t != si_tail) ; i++, t = t->next)
	{
		if(i == index){
			*x = t->x;
			*y = t->y;
			*w = t->w;
			*h = t->h;
			break;
		}
	}
	pthread_mutex_unlock (&si_mutex);

	return 0;
}

int subtitle_st_queue_pos_remove_all(void)
{
	dpos *t, *s;

	if((st_head == NULL) ||(st_tail == NULL)){
		LOGE("[%s] subtitle queue is not initialized yet!\n", __func__);
		return -1;
	}

	pthread_mutex_lock (&st_mutex);
	t = st_head->next;
	if (t == st_tail)
	{
		//LOGE("[%s] There is no data.\n", __func__);
		pthread_mutex_unlock (&st_mutex);
		return -1;
	}

	while (t != st_tail)
	{
		s = t;
		t = t->next;
		tcc_mw_free(__FUNCTION__, __LINE__, s);
	}
	st_head->next = st_tail;
	st_tail->prev = st_head;

	g_st_pos_total_cnt = 0;
	pthread_mutex_unlock (&st_mutex);

	return 0;
}

int subtitle_si_queue_pos_remove_all(void)
{
	dpos *t, *s;

	if((si_head == NULL) ||(si_tail == NULL)){
		LOGE("[%s] subtitle queue is not initialized yet!\n", __func__);
		return -1;
	}

	pthread_mutex_lock (&si_mutex);
	t = si_head->next;
	if (t == si_tail)
	{
		//LOGE("[%s] There is no data.\n", __func__);
		pthread_mutex_unlock (&si_mutex);
		return -1;
	}

	while (t != si_tail)
	{
		s = t;
		t = t->next;
		tcc_mw_free(__FUNCTION__, __LINE__, s);
	}
	si_head->next = si_tail;
	si_tail->prev = si_head;

	g_si_pos_total_cnt = 0;
	pthread_mutex_unlock (&si_mutex);

	return 0;
}

/************************************************************************************************/
int subtitle_queue_pos_init(void)
{
	if(subtitle_st_queue_pos_init()){
		LOGE("[%s] st_queue init fail\n", __func__);
		return -1;
	}
	if(subtitle_si_queue_pos_init()){
		LOGE("[%s] st_queue init fail\n", __func__);
		return -1;
	}
	return 0;
}

void subtitle_queue_pos_exit(void)
{
	subtitle_st_queue_pos_exit();
	subtitle_si_queue_pos_exit();
}

int subtitle_queue_pos_getcount(int type)
{
	return (type == 0)?g_st_pos_total_cnt:g_si_pos_total_cnt;
}

int subtitle_queue_pos_put(int type, int x, int y, int w, int h)
{
	int err = -1;

	if(type == 0){
		err = subtitle_st_queue_pos_put(x, y, w, h);
	}else{
		err = subtitle_si_queue_pos_put(x, y, w, h);
	}

	return err;
}

int subtitle_queue_pos_get(int type, int *x, int *y, int *w, int *h)
{
	int err;

	if(type == 0){
		err = subtitle_st_queue_pos_get(x, y, w, h);
	}else{
		err = subtitle_si_queue_pos_get(x, y, w, h);
	}

	return err;
}

int subtitle_queue_pos_peek_nth(int type, int index, int *x, int *y, int *w, int *h)
{
	int err = -1;

	if(type == 0){
		err = subtitle_st_queue_pos_peek_nth(index, x, y, w, h);
	}else{
		err = subtitle_si_queue_pos_peek_nth(index, x, y, w, h);
	}

	return err;
}

int subtitle_queue_pos_remove_all(int type)
{
	int err = -1;

	if(type == 0){
		err = subtitle_st_queue_pos_remove_all();
	}else{
		err = subtitle_si_queue_pos_remove_all();
	}

	return err;
}

void subtitle_queue_pos_print(void)
{
	dpos *t;

	if((st_head == NULL) ||(st_tail == NULL)){
		LOGE("[%s] subtitle queue is not initialized yet!\n", __func__);
		return;
	}

	t = st_head->next;
	LOGE("[%s] Queue contents : Front ----> Rear\n", __func__);
	while (t != st_tail)
	{
		LOGE("%d, %d, %d, %d\n", t->x, t->y, t->w, t->h);
		t = t->next;
	}
}
