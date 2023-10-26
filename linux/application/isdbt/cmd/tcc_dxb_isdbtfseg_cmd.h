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
#ifndef __TCC_DXB_ISDBTFSEG_CMD_H__
#define __TCC_DXB_ISDBTFSEG_CMD_H__

/******************************************************************************
* include 
******************************************************************************/
#include <tcc_message.h>
#include <tcc_common_cmd.h>
#include <tcc_semaphore.h>

/******************************************************************************
* typedefs & structure
******************************************************************************/
typedef	struct TCC_DXB_ISDBTFSEG_CMD_Structure
{
	int						Opcode;
	TCC_MESSAGE_QUEUE_t*	pUIMessageQueue;
	TCC_MESSAGE_QUEUE_t*	pProcessMessageQueue;
	pTCC_Sema_t*			pCmdSem;
}TCC_DXB_ISDBTFSEG_CMD_t;

/******************************************************************************
* globals
******************************************************************************/


/******************************************************************************
* locals
******************************************************************************/
extern TCC_DXB_ISDBTFSEG_CMD_t	TCC_DXB_ISDBTFSEG_CMD_Cmd;


/******************************************************************************
* declarations
******************************************************************************/
void			TCC_DXB_ISDBTFSEG_CMD_InitCmd(void);
void			TCC_DXB_ISDBTFSEG_CMD_DeinitCmd(void);
void			TCC_DXB_ISDBTFSEG_CMD_SetOpcode(void);
int				TCC_DXB_ISDBTFSEG_CMD_GetOpcode(void);
TCC_MESSAGE_t*	TCC_DXB_ISDBTFSEG_Set_Process_Message(int operand, int cmd_type,int free_flag, int data_size, void* data, void (*donfunction)(void *));
TCC_MESSAGE_t*	TCC_DXB_ISDBTFSEG_CMD_SetUIMessage(int operand, int cmd_type, int free_flag, int data_size, void* data, void (*donfunction)(void *));
TCC_MESSAGE_t*	TCC_DXB_ISDBTFSEG_CMD_GetProcessMessage(void);
TCC_MESSAGE_t*	TCC_DXB_ISDBTFSEG_CMD_GetUIMessage(void);
int				TCC_DXB_ISDBTFSEG_CMD_GetProcessMessageCount(void);
int				TCC_DXB_ISDBTFSEG_CMD_GetUIMessageCount(void);

#endif
