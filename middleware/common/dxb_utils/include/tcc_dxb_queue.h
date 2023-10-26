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
#ifndef __TCC_DXB_QUEUE_H__
#define __TCC_DXB_QUEUE_H__

#include <pthread.h>

#ifdef __cplusplus
extern    "C"
{
#endif

/** Maximum number of elements in a queue 
 */
#define MAX_QUEUE_ELEMENTS 160 //ZzaU:: increase count because of encode!!
#define TDMB_MAX_QUEUE_ELEMENTS 40

/** Output port queue element. Contains an OMX buffer header type
 */
	typedef struct tcc_qelem_t tcc_qelem_t;
	struct tcc_qelem_t
	{
		tcc_qelem_t  *q_forw;
		void     *data;
	};

/** This structure contains the queue
 */
	typedef struct tcc_dxb_queue_t
	{
		tcc_qelem_t  *first;
				  /**< Output buffer queue head */
		tcc_qelem_t  *last;
				 /**< Output buffer queue tail */
		int       nelem;
			 /**< Number of elements in the queue */
		int       nmax_elem;
				 /**maximum number of elements**/
		pthread_mutex_t mutex;
	} tcc_dxb_queue_t;

/** Initialize a queue descriptor
 * 
 * @param queue The queue descriptor to initialize. 
 * The user needs to allocate the queue
 */
	void      tcc_dxb_queue_init (tcc_dxb_queue_t * tcc_dxb_queue);
	void      tcc_dxb_queue_init_ex (tcc_dxb_queue_t * tcc_dxb_queue, int nmax_elem);

/** Deinitialize a queue descriptor
 * flushing all of its internal data
 * 
 * @param queue the queue descriptor to dump
 */
	void      tcc_dxb_queue_deinit (tcc_dxb_queue_t * queue);

/** Enqueue an element to the given queue descriptor
 * 
 * @param queue the queue descritpor where to queue data
 * 
 * @param data the data to be enqueued
 */
	void      tcc_dxb_queue (tcc_dxb_queue_t * queue, void *data);
	int 	  tcc_dxb_queue_ex (tcc_dxb_queue_t * queue, void *data);
	int		  tcc_dxb_queue_first (tcc_dxb_queue_t * queue, void *data);


/** Dequeue an element from the given queue descriptor
 * 
 * @param queue the queue descriptor from which to dequeue the element
 * 
 * @return the element that has bee dequeued. If the queue is empty
 *  a NULL value is returned
 */
	void     *tcc_dxb_dequeue (tcc_dxb_queue_t * queue);
	void     *tcc_dxb_queue_getdata (tcc_dxb_queue_t * queue);

/** Returns the number of elements hold in the queue
 * 
 * @param queue the requested queue
 * 
 * @return the number of elements in the queue
 */
	int       tcc_dxb_getquenelem (tcc_dxb_queue_t * queue);


	int tcc_dxb_get_maxqueuelem (tcc_dxb_queue_t * queue);
	void *tcc_dxb_peek_nth(tcc_dxb_queue_t *queue, int nth);


#ifdef __cplusplus
}
#endif

#endif
