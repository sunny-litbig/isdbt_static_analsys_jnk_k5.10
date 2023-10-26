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
#ifndef	_TCC_MESSAGE_H__
#define	_TCC_MESSAGE_H__



#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
* include 
******************************************************************************/
//#include "main.h"
//#include "globals.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

/******************************************************************************
* defines 
******************************************************************************/
#ifndef	TRUE
#define	TRUE	1
#define	FALSE	0
#endif

#ifndef NULL
#define	NULL	0
#endif

//#define	TCC_MESSAGE_DEBUG 

#define	TCC_MAX_MESSAGE_NUM	1000

typedef enum Message_Type
{
	MESSAGE_TYPE_NULL= 0,
	MESSAGE_TYPE_CUI_TO_PROCESS,
	MESSAGE_TYPE_UI_TO_PROCESS,
	MESSAGE_TYPE_PROCESS_TO_UI,
	MESSAGE_TYPE_TO_GSENSOR,
	MESSAGE_TYPE_TO_GPS,
	MESSAGE_TYPE_TO_ETC,
#ifdef TCC_SUBTITLE_INCLUDE
	MESSAGE_TYPE_PROCESS_TO_SUBTITLE,
#endif
	MESSAGE_TYPE_NUM,
}Message_Type;


/******************************************************************************
* globals
******************************************************************************/


/******************************************************************************
* locals
******************************************************************************/


/******************************************************************************
* typedefs & structure
******************************************************************************/
typedef	struct TCC_MESSAGE_structure
{
	int	Opcode;
	int	Operand;
	int	Cmd_Type;
	int	Free_Flag;
	int	Data_Size;
	void * Data;	
	void	(*DonFunction)(void *);
}TCC_MESSAGE_t ;


typedef	struct TCC_MESSAGE_Data_structure
{
	TCC_MESSAGE_t *Message;
	struct TCC_MESSAGE_Data_structure* next;
	
}TCC_MESSAGE_Data_t ;

typedef	struct TCC_MESSAGE_QUEUE_structure
{
	pthread_mutex_t mutex;
	int	message_count;
	int	message_type;
	TCC_MESSAGE_Data_t *message_data;
	TCC_MESSAGE_Data_t *header;
	TCC_MESSAGE_Data_t *tail;
}TCC_MESSAGE_QUEUE_t ;


extern TCC_MESSAGE_t *TCC_Creat_Message();
extern void TCC_Delete_Message(TCC_MESSAGE_t *Message);
extern int TCC_Put_Message(TCC_MESSAGE_QUEUE_t *message_queue, TCC_MESSAGE_t *Message);
#ifdef TCC_USE_PUT_MESSAGE2_INCLUDE
extern int TCC_Put_Message2(Message_Type MsgType, TCC_MESSAGE_QUEUE_t *message_queue, TCC_MESSAGE_t *Message);
#endif
extern int TCC_Put_Priority_Message(TCC_MESSAGE_QUEUE_t *message_queue, TCC_MESSAGE_t *Message);
extern int TCC_Put_Checked_Duplication_Message(TCC_MESSAGE_QUEUE_t *message_queue, TCC_MESSAGE_t *Message);
extern TCC_MESSAGE_t *TCC_Get_Message(TCC_MESSAGE_QUEUE_t *message_queue);
extern int TCC_Get_Message_Count(TCC_MESSAGE_QUEUE_t *message_queue);
#ifdef TCC_MESSAGE_DEBUG
extern int	TCC_Display_Debug_Message(TCC_MESSAGE_QUEUE_t *message_queue, TCC_MESSAGE_t *Message);
#endif

#ifdef __cplusplus
}
#endif

#endif //_TCC_MESSAGE_H__
