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
/******************************************************************************
* include 
******************************************************************************/
#include "tcc_dxb_isdbtfseg_message.h"


/******************************************************************************
* globals
******************************************************************************/
TCC_MESSAGE_QUEUE_t	TCC_DXB_ISDBTFSEG_MESSAGE_ProcessMessageQueue;
TCC_MESSAGE_QUEUE_t	TCC_DXB_ISDBTFSEG_MESSAGE_UIMessageQueue;

/******************************************************************************
* Funtions
******************************************************************************/
void TCC_DXB_ISDBTFSEG_MESSAGE_InitQueue(void)
{
	// Init Process message	
	pthread_mutex_init(&TCC_DXB_ISDBTFSEG_MESSAGE_ProcessMessageQueue.mutex, NULL);
	TCC_DXB_ISDBTFSEG_MESSAGE_ProcessMessageQueue.message_count	= 0;
	TCC_DXB_ISDBTFSEG_MESSAGE_ProcessMessageQueue.message_type	= MESSAGE_TYPE_UI_TO_PROCESS;
	TCC_DXB_ISDBTFSEG_MESSAGE_ProcessMessageQueue.message_data	= NULL;
	TCC_DXB_ISDBTFSEG_MESSAGE_ProcessMessageQueue.header		= TCC_DXB_ISDBTFSEG_MESSAGE_ProcessMessageQueue.message_data;
	TCC_DXB_ISDBTFSEG_MESSAGE_ProcessMessageQueue.tail			= TCC_DXB_ISDBTFSEG_MESSAGE_ProcessMessageQueue.message_data;	

	// Init UI message
	pthread_mutex_init(&TCC_DXB_ISDBTFSEG_MESSAGE_UIMessageQueue.mutex, NULL);	
	TCC_DXB_ISDBTFSEG_MESSAGE_UIMessageQueue.message_count		= 0;
	TCC_DXB_ISDBTFSEG_MESSAGE_UIMessageQueue.message_type		= MESSAGE_TYPE_PROCESS_TO_UI;
	TCC_DXB_ISDBTFSEG_MESSAGE_UIMessageQueue.message_data		= NULL;
	TCC_DXB_ISDBTFSEG_MESSAGE_UIMessageQueue.header				= TCC_DXB_ISDBTFSEG_MESSAGE_UIMessageQueue.message_data;
	TCC_DXB_ISDBTFSEG_MESSAGE_UIMessageQueue.tail				= TCC_DXB_ISDBTFSEG_MESSAGE_UIMessageQueue.message_data;
}

TCC_MESSAGE_QUEUE_t* TCC_DXB_ISDBTFSEG_MESSAGE_GetUIMessageQueue(void)
{
	return(&TCC_DXB_ISDBTFSEG_MESSAGE_UIMessageQueue);
}

TCC_MESSAGE_QUEUE_t* TCC_DXB_ISDBTFSEG_MESSAGE_GetProcessMessageQueue(void)
{
	return(&TCC_DXB_ISDBTFSEG_MESSAGE_ProcessMessageQueue);
}
