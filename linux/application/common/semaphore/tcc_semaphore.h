/*******************************************************************************

*   FileName : tcc_semaphore.h

*   Copyright (c) Telechips Inc.

*   Description : tcc_semaphore.h

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

#ifndef	_TCC_SEMAPHORE_H__
#define	_TCC_SEMAPHORE_H__

/******************************************************************************
* include 
******************************************************************************/
#include <pthread.h>


/******************************************************************************
* defines 
******************************************************************************/


/******************************************************************************
* typedefs & structure
******************************************************************************/
#ifndef 	TRUE
#define	TRUE 	1
#define	FALSE	0
#endif


typedef struct TCC_SEMAPHORE_t
{
  pthread_cond_t condition;
  pthread_mutex_t mutex;
  unsigned int value;
}TCC_Sema_t,*pTCC_Sema_t;



/******************************************************************************
* globals
******************************************************************************/



/******************************************************************************
* locals
******************************************************************************/


/******************************************************************************
* declarations
******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

void TCC_Sema_Init(TCC_Sema_t* Sema, unsigned int val) ;
void TCC_Sema_Deinit(TCC_Sema_t* Sema) ;
void TCC_Sema_Down(TCC_Sema_t* Sema) ;
void TCC_Sema_Up(TCC_Sema_t* Sema) ;
void TCC_Sema_Reset(TCC_Sema_t* Sema) ;
void TCC_Sema_Wait(TCC_Sema_t* Sema) ;
void TCC_Sema_Signal(TCC_Sema_t* Sema); 
void TCC_Sema_Lock(TCC_Sema_t* Sema);
void TCC_Sema_Unlock(TCC_Sema_t* Sema);
int TCC_Sema_Down_TimeWait(TCC_Sema_t* Sema,int expire_time);


#ifdef __cplusplus
}
#endif



#endif //_TCC_SEMAPHORE_H__




