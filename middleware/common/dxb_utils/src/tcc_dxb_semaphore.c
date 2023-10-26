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
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include <pthread.h>
#include <sys/time.h>
#include <errno.h>

#include "tcc_dxb_semaphore.h"
#include <tcc_pthread_cond.h>

/** Initializes the semaphore at a given value
 * 
 * @param tsem the semaphore to initialize
 * @param val the initial value of the semaphore
 * 
 */
void tcc_dxb_sem_init(tcc_dxb_sem_t* tsem, unsigned int val) {
  if(tsem == NULL)
  {
    return;
  }
  
  tcc_pthread_cond_init(&tsem->condition, NULL);
  pthread_mutex_init(&tsem->mutex, NULL);
  tsem->semval = val;
}

/** Destroy the semaphore
 *  
 * @param tsem the semaphore to destroy
 */
void tcc_dxb_sem_deinit(tcc_dxb_sem_t* tsem) {
  pthread_cond_destroy(&tsem->condition);
  pthread_mutex_destroy(&tsem->mutex);
}

/** Decreases the value of the semaphore. Blocks if the semaphore
 * value is zero.
 * 
 * @param tsem the semaphore to decrease
 */
void tcc_dxb_sem_down(tcc_dxb_sem_t* tsem) {
  pthread_mutex_lock(&tsem->mutex);
  while (tsem->semval == 0) {
	  pthread_cond_wait(&tsem->condition, &tsem->mutex);
  }
  tsem->semval--;
  pthread_mutex_unlock(&tsem->mutex);
}

int tcc_dxb_sem_down_timewait(tcc_dxb_sem_t* tsem,int expire_time)
{
	int err = 0;
    struct timespec ts = { 0, };

    clock_gettime(CLOCK_MONOTONIC , &ts);
    ts.tv_sec += expire_time; 	// sec 단위로 입력..

	pthread_mutex_lock(&tsem->mutex);
	while (tsem->semval == 0) {
		err = tcc_pthread_cond_timedwait(&tsem->condition, &tsem->mutex , &ts);

		if(err == ETIMEDOUT)
		{
			tsem->semval++;
		}
	}
	tsem->semval--;
	pthread_mutex_unlock(&tsem->mutex);

	if(err == ETIMEDOUT)
		return 0;
	else return 1;
}

int tcc_dxb_sem_down_timewait_msec(tcc_dxb_sem_t* tsem,int expire_time)
{
    int err = 0;
    struct timespec ts = { 0, };

    clock_gettime(CLOCK_MONOTONIC , &ts);
    ts.tv_nsec = ts.tv_nsec + (expire_time%1000)*(1000*1000);
    ts.tv_sec = ts.tv_sec + expire_time/1000 + ts.tv_nsec/(1000*1000*1000);
    ts.tv_nsec = ts.tv_nsec%(1000*1000*1000);

	pthread_mutex_lock(&tsem->mutex);
	while (tsem->semval == 0) {
		err = tcc_pthread_cond_timedwait(&tsem->condition, &tsem->mutex , &ts);
		if(err == ETIMEDOUT)
		{
			tsem->semval++;
		}

		if(err == 0){ //when the thread wake-up, don't wait.
			break;
		}
	}
	tsem->semval--;
	pthread_mutex_unlock(&tsem->mutex);
	if(err == ETIMEDOUT)
		return 0;
	else return 1;
}


/** Increases the value of the semaphore
 * 
 * @param tsem the semaphore to increase
 */
void tcc_dxb_sem_up(tcc_dxb_sem_t* tsem) {
  pthread_mutex_lock(&tsem->mutex);
  tsem->semval++;
  pthread_cond_signal(&tsem->condition);
  pthread_mutex_unlock(&tsem->mutex);
}

/** Reset the value of the semaphore
 * 
 * @param tsem the semaphore to reset
 */
void tcc_dxb_sem_reset(tcc_dxb_sem_t* tsem) {
  pthread_mutex_lock(&tsem->mutex);
  tsem->semval=0;
  pthread_mutex_unlock(&tsem->mutex);
}

/** Wait on the condition.
 * 
 * @param tsem the semaphore to wait
 */
void tcc_dxb_sem_wait(tcc_dxb_sem_t* tsem) {
  pthread_mutex_lock(&tsem->mutex);
  pthread_cond_wait(&tsem->condition, &tsem->mutex);
  pthread_mutex_unlock(&tsem->mutex);
}

/** Signal the condition,if waiting
 * 
 * @param tsem the semaphore to signal
 */
void tcc_dxb_sem_signal(tcc_dxb_sem_t* tsem) {
  pthread_mutex_lock(&tsem->mutex);
  pthread_cond_signal(&tsem->condition);
  pthread_mutex_unlock(&tsem->mutex);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
