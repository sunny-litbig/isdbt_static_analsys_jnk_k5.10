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
#include <pthread.h>
#include <linux/unistd.h>
#include "tcc_message.h"
#include "tcc_common_cmd.h"
#include "tcc_dxb_isdbtfseg_cui.h"
#include "tcc_dxb_isdbtfseg_ui.h"
#include "tcc_dxb_isdbtfseg_ui_process.h"

/******************************************************************************
* defines 
******************************************************************************/
#define	TCC_DXB_ISDBTFSEG_UI_PROCESS_SLEEP_TIME			10000	//10ms


/******************************************************************************
* locals
******************************************************************************/
static TCC_DXB_ISDBTFSEG_UI_PROCESS_Instance_T	G_t_TCC_DXB_ISDBTFSEG_UI_PROCESS_Instance;

/******************************************************************************
* extern
******************************************************************************/
extern int Init_Support_UART_Console;

/******************************************************************************
* Core Functions
******************************************************************************/
static int TCC_DXB_ISDBTFSEG_UI_PROCESS_ProcMessage(void)
{
	TCC_MESSAGE_t *ptMessage = NULL;
	
	if(TCC_DXB_ISDBTFSEG_CMD_GetUIMessageCount() > 0)
	{
		if((ptMessage = TCC_DXB_ISDBTFSEG_CMD_GetUIMessage()) != NULL)
		{
			if(ptMessage->Cmd_Type == TCC_CMD_REPLY)
			{
				switch(ptMessage->Operand)
				{
					case TCC_OPERAND_SCAN:
						printf("[%s:%d] %d \n", __func__, __LINE__, ptMessage->Operand);
						break;

					default :
						printf("[%s:%d] %d \n", __func__, __LINE__, ptMessage->Operand);
						break;
				}
			}
			else if(ptMessage->Cmd_Type == TCC_CMD_IND)
			{
				switch(ptMessage->Operand)
				{
					case TCC_OPERAND_ERROR:
						printf("[%s:%d] <TCC_CMD_IND> TCC_OPERAND_ERROR\n", __func__, __LINE__);
						break;
					default : 
						break;
				}
			}

			if(ptMessage->DonFunction != NULL)
			{
				if((ptMessage->Free_Flag == 0) && (ptMessage->Data_Size > 0))
					ptMessage->DonFunction(ptMessage->Data);
				else
					ptMessage->DonFunction(NULL);
			}
			
			TCC_Delete_Message(ptMessage);
		}
	}
	else
	{
		usleep(TCC_DXB_ISDBTFSEG_UI_PROCESS_SLEEP_TIME);
	}

	return 0;
}

static int TCC_DXB_ISDBTFSEG_UI_PROCESS_Thread(void *pvPrivate)
{
	G_t_TCC_DXB_ISDBTFSEG_UI_PROCESS_Instance.iRunFlag = 1;
	
	printf("[%s] TID(%d) \n", __func__, syscall(__NR_gettid));
	if (Init_Support_UART_Console > 0)
	{
		printf(">> [CUI] Ready!!\n");
	}	

	while(G_t_TCC_DXB_ISDBTFSEG_UI_PROCESS_Instance.iRunFlag == 1)
	{
		if (Init_Support_UART_Console > 0)
			TCC_DXB_ISDBTFSEG_CUI_ProcMessage();

		TCC_DXB_ISDBTFSEG_UI_PROCESS_ProcMessage();
	}
}

int TCC_DXB_ISDBTFSEG_UI_PROCESS_Init(void)
{
	int iRetValue;

	pthread_attr_t tAttr;
	struct sched_param tParam;

	if (Init_Support_UART_Console > 0)
		TCC_DXB_ISDBTFSEG_CUI_Init();

	/* initialized with default attributes */
	pthread_attr_init(&tAttr);
	pthread_attr_setinheritsched(&tAttr, PTHREAD_EXPLICIT_SCHED);  
	pthread_attr_setscope(&tAttr, PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setschedpolicy(&tAttr, SCHED_OTHER);
	/* safe to get existing scheduling param */
	pthread_attr_getschedparam(&tAttr, &tParam);
	/* set the priority; others are unchanged */
	tParam.__sched_priority = 0;
	/* setting the new scheduling param */
	pthread_attr_setschedparam(&tAttr, &tParam);
	pthread_attr_getschedparam(&tAttr, &tParam);

	if((iRetValue = pthread_create(&G_t_TCC_DXB_ISDBTFSEG_UI_PROCESS_Instance.tThread, &tAttr, TCC_DXB_ISDBTFSEG_UI_PROCESS_Thread, NULL)) < 0)
	{
		printf("[%s:%d] TCC_DXB_ISDBTFSEG_UI_PROCESS_Thread create failed !!\n", __func__, __LINE__);
		
		pthread_attr_destroy(&tAttr);
		
		return iRetValue;
	}

	pthread_attr_destroy(&tAttr);

	return 0;

}

int TCC_DXB_ISDBTFSEG_UI_PROCESS_DeInit(void)
{
	if (Init_Support_UART_Console > 0)
		TCC_DXB_ISDBTFSEG_CUI_DeInit();
	
	G_t_TCC_DXB_ISDBTFSEG_UI_PROCESS_Instance.iRunFlag = 0;
	pthread_join(G_t_TCC_DXB_ISDBTFSEG_UI_PROCESS_Instance.tThread, NULL);

	return 0;
}

