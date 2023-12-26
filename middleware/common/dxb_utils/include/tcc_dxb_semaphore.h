/*******************************************************************************

*   FileName : tcc_dxb_semaphore.h

*   Copyright (c) Telechips Inc.

*   Description : tcc_dxb_semaphore.h

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


#ifndef __TCC_DXB_SEMAPHORE_H__
#define __TCC_DXB_SEMAPHORE_H__

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/** The structure contains the semaphore value, mutex and green light flag
 */ 
typedef struct tcc_dxb_sem_t{
  pthread_cond_t condition;
  pthread_mutex_t mutex;
  unsigned int semval;
}tcc_dxb_sem_t;

/** Initializes the semaphore at a given value
 * 
 * @param tsem the semaphore to initialize
 * 
 * @param val the initial value of the semaphore
 */
void tcc_dxb_sem_init(tcc_dxb_sem_t* tsem, unsigned int val);

/** Destroy the semaphore
 *  
 * @param tsem the semaphore to destroy
 */
void tcc_dxb_sem_deinit(tcc_dxb_sem_t* tsem);

/** Decreases the value of the semaphore. Blocks if the semaphore
 * value is zero.
 * 
 * @param tsem the semaphore to decrease
 */
void tcc_dxb_sem_down(tcc_dxb_sem_t* tsem);

/** Increases the value of the semaphore
 * 
 * @param tsem the semaphore to increase
 */
void tcc_dxb_sem_up(tcc_dxb_sem_t* tsem);

/** Reset the value of the semaphore
 * 
 * @param tsem the semaphore to reset
 */
void tcc_dxb_sem_reset(tcc_dxb_sem_t* tsem);

/** Wait on the condition.
 * 
 * @param tsem the semaphore to wait
 */
void tcc_dxb_sem_wait(tcc_dxb_sem_t* tsem);

/** Signal the condition,if waiting
 * 
 * @param tsem the semaphore to signal
 */
void tcc_dxb_sem_signal(tcc_dxb_sem_t* tsem);

int tcc_dxb_sem_down_timewait(tcc_dxb_sem_t* tsem,int expire_time);

#ifdef __cplusplus
}
#endif

#endif
