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
#include "tcc_message.h"
#ifdef TCC_MESSAGE_DEBUG
#include "tcc_common_cmd.h"
#endif


/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_Lock_Message_Queue(TCC_MESSAGE_QUEUE_t *message_queue)
{
	pthread_mutex_lock(&message_queue->mutex);
}

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_UnLock_Message_Queue(TCC_MESSAGE_QUEUE_t *message_queue)
{
	pthread_mutex_unlock(&message_queue->mutex);
}

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
TCC_MESSAGE_t *TCC_Creat_Message()
{
	TCC_MESSAGE_t *Message;
	
	Message = TCC_malloc(sizeof(TCC_MESSAGE_t));
	
	return (Message);
}

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_Delete_Message(TCC_MESSAGE_t *Message)
{
	if ((Message->Free_Flag == TRUE ) && (Message->Data_Size > 0))
	{
		TCC_free(Message->Data);
	}	
	TCC_free(Message);
}

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
int TCC_Put_Message(TCC_MESSAGE_QUEUE_t *message_queue, TCC_MESSAGE_t *Message)
{
	TCC_MESSAGE_Data_t 	*new_message_data;
	int 				ret = -1;
	
#ifdef TCC_GUI_INCLUDE
	int 				reply = 0;
	
	if (Message->DonFunction != NULL)
	{
		reply = 1;
	}
	tcc_ipc_ui_cmd_send(Message, Message->Data_Size, reply);

	if (Message->Data != NULL)
		TCC_free(Message->Data);
	
	if (Message != NULL)
		TCC_free(Message);
#else // #ifdef TCC_GUI_INCLUDE
	
	TCC_Lock_Message_Queue(message_queue);
	
	if (message_queue->message_count == 0)
	{
		new_message_data				= TCC_malloc(sizeof(TCC_MESSAGE_Data_t));
		if(new_message_data == NULL)
		{
			printf("[%s:%d] malloc fail : 0x%x\n", __func__, __LINE__, new_message_data);
		}
		else
		{
			new_message_data->Message 		= Message;
			new_message_data->next 			= NULL;
			message_queue->message_data 	= new_message_data;
			message_queue->tail			= new_message_data;
			message_queue->header 			= new_message_data;
			
			ret = message_queue->message_count++;
			
#ifdef TCC_MESSAGE_DEBUG
			TCC_Display_Debug_Message(message_queue, Message);
#endif
		}
	}
	else if (message_queue->message_count < TCC_MAX_MESSAGE_NUM)
	{
		new_message_data			= TCC_malloc(sizeof(TCC_MESSAGE_Data_t));
		if(new_message_data == NULL)
		{
			printf("[%s:%d] malloc fail : 0x%x\n", __func__, __LINE__, new_message_data);
		}
		else
		{
			new_message_data->Message 	= Message;
			new_message_data->next 		= NULL;
			message_queue->tail->next	= new_message_data;
			message_queue->tail 		= new_message_data;
			
			ret = message_queue->message_count++;
			
#ifdef TCC_MESSAGE_DEBUG
			TCC_Display_Debug_Message(message_queue, Message);
#endif
		}
	}
	else
	{
		printf(" -------------Message Over---------[0x%x - operand %d]--------- \n", message_queue, Message->Operand);
		
#ifdef TCC_MESSAGE_DEBUG
		TCC_Display_Debug_Message(message_queue, Message);
#endif
		// TCC_MAX_MESSAGE_NUM 보다 클 경우
		// message를 free 해준다.
		
		if ((Message->Data_Size > 0) && (Message->Data != NULL))
		{
			TCC_free(Message->Data);
		}
		
		TCC_free(Message);
	}
	TCC_UnLock_Message_Queue(message_queue);
#endif //ifdef TCC_GUI_INCLUDE

	return (ret);
}

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
#ifdef TCC_USE_PUT_MESSAGE2_INCLUDE
int TCC_Put_Message2(Message_Type MsgType, TCC_MESSAGE_QUEUE_t *message_queue, TCC_MESSAGE_t *Message)
{
	TCC_MESSAGE_Data_t 	*new_message_data;
	int 				ret = -1;

	if(message_queue == NULL || Message == NULL)
	{
		printf("[%s] message_queue == NULL || Message == NULL Error !!!!!\n", __func__);
		return ret;
	}

	if ((MsgType == MESSAGE_TYPE_UI_TO_PROCESS) || (MsgType == MESSAGE_TYPE_PROCESS_TO_UI))
	{
		int reply = 0;
		#if 0	
		TCC_Lock_Message_Queue(message_queue);
	
		if (Message->DonFunction != NULL)
		{
			reply = 1;
		}
		tcc_ipc_ui_cmd_send(Message, Message->Data_Size, reply);

		if (Message->Data != NULL)
			TCC_free(Message->Data);
	
		if (Message != NULL)
			TCC_free(Message);
			
		TCC_UnLock_Message_Queue(message_queue);
		#else
		printf("[%s] ignore MsgType=%d\n", __func__, MsgType);
		#endif
	}
	else if ( MsgType == MESSAGE_TYPE_CUI_TO_PROCESS )
	{
		TCC_Lock_Message_Queue(message_queue);
	
/* Original
		if (message_queue->message_count == 0)
		{
			new_message_data				= TCC_malloc(sizeof(TCC_MESSAGE_Data_t));
			new_message_data->Message 		= Message;
			new_message_data->next 			= NULL;
			message_queue->message_data 	= new_message_data;
			message_queue->tail				= new_message_data;
			message_queue->header 			= new_message_data;
		
			ret = message_queue->message_count++;
		
			#ifdef TCC_MESSAGE_DEBUG
			TCC_Display_Debug_Message(message_queue, Message);
			#endif
		}
*/
		if (message_queue->message_count == 0)
		{
			new_message_data				= TCC_malloc(sizeof(TCC_MESSAGE_Data_t));
//11th Try, David, To avoid Codesonar's warning, NULL Pointer Dereference
//add exception-handling-code for new_message_data ( Check malloc function )
			if(new_message_data == NULL)
			{
				printf("[%s:%d] malloc fail : 0x%x !!!!\n", __func__, __LINE__, new_message_data);
			}
			else
			{
				new_message_data->Message		= Message;
				new_message_data->next			= NULL;
				message_queue->message_data 	= new_message_data;
				message_queue->tail 			= new_message_data;
				message_queue->header			= new_message_data;
			
				ret = message_queue->message_count++;
			
			#ifdef TCC_MESSAGE_DEBUG
				TCC_Display_Debug_Message(message_queue, Message);
			#endif
			}
		}
		else if (message_queue->message_count < TCC_MAX_MESSAGE_NUM)
		{
			new_message_data			= TCC_malloc(sizeof(TCC_MESSAGE_Data_t));
//11th Try, David, To avoid Codesonar's warning, NULL Pointer Dereference
//add exception-handling-code for new_message_data (Check malloc function)
			if(new_message_data == NULL)
			{
				printf("[%s:%d] malloc fail : 0x%x !!!!\n", __func__, __LINE__, new_message_data);
			}
			else
			{
				new_message_data->Message 	= Message;
				new_message_data->next 		= NULL;
				message_queue->tail->next	= new_message_data;
				message_queue->tail 		= new_message_data;
				
				ret = message_queue->message_count++;
				
				#ifdef TCC_MESSAGE_DEBUG
				TCC_Display_Debug_Message(message_queue, Message);
				#endif
			}
		}
		else
		{
			printf(" -------------Message Over---------[0x%x - operand %d]--------- \n", message_queue, Message->Operand);
		
			#ifdef TCC_MESSAGE_DEBUG
			TCC_Display_Debug_Message(message_queue, Message);
			#endif
			// TCC_MAX_MESSAGE_NUM 보다 클 경우
			// message를 free 해준다.
		
			if ((Message->Data_Size > 0) && (Message->Data != NULL))
			{
				TCC_free(Message->Data);
			}
			
			TCC_free(Message);
		}
		TCC_UnLock_Message_Queue(message_queue);
	}			

	return (ret);
}
#endif //TCC_USE_PUT_MESSAGE2_INCLUDE

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
int TCC_Put_Priority_Message(TCC_MESSAGE_QUEUE_t *message_queue, TCC_MESSAGE_t *Message)
{
	TCC_MESSAGE_Data_t 	*new_message_data = NULL;
	int 				ret = -1;
	
	TCC_Lock_Message_Queue(message_queue);
/* Original	
	if (message_queue->message_count < TCC_MAX_MESSAGE_NUM)
	{
		new_message_data			= TCC_malloc(sizeof(TCC_MESSAGE_Data_t));
		new_message_data->Message 	= Message;
		new_message_data->next 		= message_queue->header;
		message_queue->header		= new_message_data;
		
		ret = message_queue->message_count++;
	}
*/	

	if (message_queue->message_count < TCC_MAX_MESSAGE_NUM)
	{
		new_message_data			= TCC_malloc(sizeof(TCC_MESSAGE_Data_t));
//11th Try, David, To avoid Codesonar's warning, NULL Pointer Dereference
//add exception-handling-code for new_message_data ( check malloc function )
		if(new_message_data == NULL)
		{
			printf("[%s:%d] malloc fail - new_message_data is null : 0x%x!!!!\n", __func__, __LINE__, new_message_data);
		}
		else
		{
			new_message_data->Message 	= Message;
			new_message_data->next 		= message_queue->header;
			message_queue->header		= new_message_data;
		}
		ret = message_queue->message_count++;
	}
//11th Try, David, To avoid Codesonar's warning, NULL Pointer Dereference
//add exception-handling-code for new_message_data ( check malloc function )
	if(new_message_data == NULL)
	{
		printf("[%s:%d] malloc fail - new_message_data is null : 0x%x !!!!\n", __func__, __LINE__, new_message_data);
	}
	else
	{
		TCC_UnLock_Message_Queue(message_queue);
	}
	
	return (ret);
}

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
int TCC_Put_Checked_Duplication_Message(TCC_MESSAGE_QUEUE_t *message_queue, TCC_MESSAGE_t *Message)
{
	TCC_MESSAGE_Data_t 	*Current;
	int 				ret = -1;
	int 				i;
	
	TCC_Lock_Message_Queue(message_queue);
	
	if (message_queue->message_count > 0)
	{
		Current = message_queue->header;
	
		for (i = 0; i < message_queue->message_count; i++)
		{
			if ((Current->Message->Operand == Message->Operand) && (Current->Message->Opcode == Message->Opcode))
			{
				// Delete Message 
				if (( Message->Data_Size > 0) && (Message->Data != NULL))
				{
					TCC_free(Message->Data);
				}
				
				TCC_free(Message);
				
				TCC_UnLock_Message_Queue(message_queue);
				return -1;
			}
			Current = Current->next;
		}
		
	}
	
	TCC_UnLock_Message_Queue(message_queue);
	ret = TCC_Put_Message(message_queue, Message);
	
	return (ret);
}

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
TCC_MESSAGE_t *TCC_Get_Message(TCC_MESSAGE_QUEUE_t *message_queue)
{
	TCC_MESSAGE_t 		*new_packet;
	TCC_MESSAGE_Data_t 	*temp_message;
	
	TCC_Lock_Message_Queue(message_queue);
	
	if (message_queue->message_count > 0)
	{
		//new_packet=TCC_malloc(sizeof(TCC_MESSAGE_t));
		new_packet		= message_queue->header->Message;
		temp_message	= message_queue->header;
		if( temp_message->next != NULL )
		{
			message_queue->header = temp_message->next;
			message_queue->message_count--;
		}
		else
		{
			message_queue->message_data 	= NULL;
			message_queue->header 			= NULL;
			message_queue->tail 			= NULL;
			message_queue->message_count	= 0;
		}
		TCC_free(temp_message);
		
	}
	else
	{
		new_packet = NULL;
	}
	TCC_UnLock_Message_Queue(message_queue);
	return (new_packet);
}

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
int TCC_Get_Message_Count(TCC_MESSAGE_QUEUE_t *message_queue)
{
	return (message_queue->message_count);
}

#ifdef TCC_MESSAGE_DEBUG
/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
int	TCC_Display_Debug_Message(TCC_MESSAGE_QUEUE_t *message_queue, TCC_MESSAGE_t *Message)
{
		if (message_queue->message_type == MESSAGE_TYPE_UI_TO_PROCESS)
			printf(" -------------UI====>MULTIMEDIA------------------ \n");
		else if (message_queue->message_type == MESSAGE_TYPE_PROCESS_TO_UI)
			printf(" -------------MULTIMEIAD======>UI------------------ \n");
			
		printf(" message_queue->message_count=%d\n", message_queue->message_count);
#if 0
		switch(Message->Opcode)
		{
			case TCC_OPCODE_AUDIO_DEC:
				printf(" Message->Opcode=%d : TCC_OPCODE_AUDIO_DEC\n",Message->Opcode);
				break;
				
			case TCC_OPCODE_AUDIO_ENC:
				printf(" Message->Opcode=%d : TCC_OPCODE_AUDIO_ENC\n",Message->Opcode);
				break;
				
			case TCC_OPCODE_VIDEO_DEC:
				printf(" Message->Opcode=%d : TCC_OPCODE_VIDEO_DEC\n",Message->Opcode);
				break;
				
			case TCC_OPCODE_IMAGE_DEC:
				printf(" Message->Opcode=%d : TCC_OPCODE_IMAGE_DEC\n",Message->Opcode);
				break;
				
			case TCC_OPCODE_ISDBT_DEC:
				printf(" Message->Opcode=%d : TCC_OPCODE_ISDBT_DEC\n",Message->Opcode);
				break;
				
			case TCC_OPCODE_DVBT_DEC:
				printf(" Message->Opcode=%d : TCC_OPCODE_DVBT_DEC\n",Message->Opcode);
				break;
				
			case TCC_OPCODE_TDMB_DEC:
				printf(" Message->Opcode=%d : TCC_OPCODE_TDMB_DEC\n",Message->Opcode);
				break;
				
			case TCC_OPCODE_CAMERA:
				printf(" Message->Opcode=%d : TCC_OPCODE_CAMERA\n",Message->Opcode);
				break;
				
#ifdef TCC_SUBTITLE_INCLUDE
			case TCC_OPCODE_SUBTITLE:
				printf(" Message->Opcode=%d : TCC_OPCODE_SUBTITLE\n",Message->Opcode);
				break;
#endif
			
			default:
			break;
		}
		
		if ((Message->Opcode ==TCC_OPCODE_AUDIO_DEC ) ||
			(Message->Opcode ==TCC_OPCODE_AUDIO_ENC ) ||
			(Message->Opcode ==TCC_OPCODE_VIDEO_DEC ) ||
			(Message->Opcode ==TCC_OPCODE_IMAGE_DEC ) ||
			(Message->Opcode ==TCC_OPCODE_ISDBT_DEC ) ||
			(Message->Opcode ==TCC_OPCODE_TDMB_DEC ) ||
			(Message->Opcode ==TCC_OPCODE_DVBT_DEC ) ||
			(Message->Opcode ==TCC_OPCODE_DVBT_DEC ) ||
			(Message->Opcode ==TCC_OPCODE_CAMERA ))
		{
		switch(Message->Operand)
		{
			case TCC_OPERAND_PLAY:
				printf(" Message->Operand=%d : TCC_OPERAND_PLAY\n",Message->Operand);
				break;
				
			case TCC_OPERAND_RECODE:
				printf(" Message->Operand=%d : TCC_OPERAND_RECODE\n",Message->Operand);
				break;
				
			case TCC_OPERAND_PAUSE:
				printf(" Message->Operand=%d : TCC_OPERAND_PAUSE\n",Message->Operand);
				break;
				
			case TCC_OPERAND_RESUME:
				printf(" Message->Operand=%d : TCC_OPERAND_RESUME\n",Message->Operand);
				break;
				
			case TCC_OPERAND_STOP:
				printf(" Message->Operand=%d : TCC_OPERAND_STOP\n",Message->Operand);
				break;
				
			case TCC_OPERAND_NEXT:
				printf(" Message->Operand=%d : TCC_OPERAND_NEXT\n",Message->Operand);
				break;
				
			case TCC_OPERAND_PREVIOUS:
				printf(" Message->Operand=%d : TCC_OPERAND_PREVIOUS\n",Message->Operand);
				break;
				
			case TCC_OPERAND_EOF:
				printf(" Message->Operand=%d : TCC_OPERAND_EOF\n",Message->Operand);
				break;
				
			case TCC_OPERAND_FF_SEEK:
				printf(" Message->Operand=%d : TCC_OPERAND_FF_SEEK\n",Message->Operand);
				break;
				
			case TCC_OPERAND_REW_SEEK:
				printf(" Message->Operand=%d : TCC_OPERAND_REW_SEEK\n",Message->Operand);
				break;
				
			case TCC_OPERAND_SEEK:
				printf(" Message->Operand=%d : TCC_OPERAND_SEEK\n",Message->Operand);
				break;
				
			case TCC_OPERAND_VOLUME:
				printf(" Message->Operand=%d : TCC_OPERAND_VOLUME\n",Message->Operand);
				break;
				
			case TCC_OPERAND_INFO:
				printf(" Message->Operand=%d : TCC_OPERAND_INFO\n",Message->Operand);
				break;
				
			case TCC_OPERAND_CURRENT:
				printf(" Message->Operand=%d : TCC_OPERAND_CURRENT\n",Message->Operand);
				break;
				
			case TCC_OPERAND_EQ:
				printf(" Message->Operand=%d : TCC_OPERAND_EQ\n",Message->Operand);
				break;
				
			case TCC_OPERAND_PLAYLIST:
				printf(" Message->Operand=%d : TCC_OPERAND_PLAYLIST\n",Message->Operand);
				break;
				
			case TCC_OPERAND_PLAYLIST_MODE:
				printf(" Message->Operand=%d : TCC_OPERAND_PLAYLIST_MODE\n",Message->Operand);
				break;
				
			case TCC_OPERAND_THUMB:
				printf(" Message->Operand=%d : TCC_OPERAND_THUMB\n",Message->Operand);
				break;
				
			case TCC_OPERAND_ROTATION:
				printf(" Message->Operand=%d : TCC_OPERAND_ROTATION\n",Message->Operand);
				break;
				
			case TCC_OPERAND_ZOOM:
				printf(" Message->Operand=%d : TCC_OPERAND_ZOOM\n",Message->Operand);
				break;
				
			case TCC_OPERAND_PAN:
				printf(" Message->Operand=%d : TCC_OPERAND_PAN\n",Message->Operand);
				break;
				
			case TCC_OPERAND_SEARCH:
				printf(" Message->Operand=%d : TCC_OPERAND_SEARCH\n",Message->Operand);
				break;
				
			case TCC_OPERAND_SEARCHED:
				printf(" Message->Operand=%d : TCC_OPERAND_SEARCHED\n",Message->Operand);
				break;
				
			case TCC_OPERAND_SET:
				printf(" Message->Operand=%d : TCC_OPERAND_SET\n",Message->Operand);
				break;
				
			case TCC_OPERAND_SET_MUTE:
				printf(" Message->Operand=%d : TCC_OPERAND_SET_MUTE\n",Message->Operand);
				break;
				
			case TCC_OPERAND_EXIT:
				printf(" Message->Operand=%d : TCC_OPERAND_EXIT\n",Message->Operand);
				break;
				
			case TCC_OPERAND_ERROR:
				printf(" Message->Operand=%d : TCC_OPERAND_ERROR\n",Message->Operand);
				break;
				
			case TCC_OPERAND_NEXT_FRAME:
				printf(" Message->Operand=%d : TCC_OPERAND_NEXT_FRAME\n",Message->Operand);
				break;
				
			case TCC_OPERAND_STOP_IND:
				printf(" Message->Operand=%d : TCC_OPERAND_STOP_IND\n",Message->Operand);
				break;
				
			case TCC_OPERAND_SLIDE:
				printf(" Message->Operand=%d : TCC_OPERAND_SLIDE\n",Message->Operand);
				break;
				
			case TCC_OPERAND_HDMI_SET:
				printf(" Message->Operand=%d : TCC_OPERAND_HDMI_SET\n",Message->Operand);
				break;
				
			case TCC_OPERAND_FRAME_SET:
				printf(" Message->Operand=%d : TCC_OPERAND_FRAME_SET\n",Message->Operand);
				break;
				
			case TCC_OPERAND_CAMREC_PREVIEW:
				printf(" Message->Operand=%d : TCC_OPERAND_CAMREC_PREVIEW\n",Message->Operand);
				break;
				
			default:
				printf(" Message->Operand=%d ",Message->Operand);
				break;
		}
		}
#ifdef TCC_SUBTITLE_INCLUDE
		else if (Message->Opcode ==TCC_OPCODE_SUBTITLE)
		{
			switch(Message->Operand)
			{
				case TCC_OPERAND_SUBTITLE_PLAY:
					printf(" Message->Operand=%d : TCC_OPERAND_SUBTITLE_PLAY\n",Message->Operand);
					break;
					
				default:
					printf(" Message->Operand=%d : subtitle default\n",Message->Operand);
					break;
			}
		}
#endif

#else
	printf(" Message->Opcode=%d : \n", Message->Opcode);
	printf(" Message->Operand=%d : \n", Message->Operand);
#endif
		switch (Message->Cmd_Type)
		{
			case TCC_CMD_SEND:
				printf(" Message->Cmd_Type=%d : TCC_CMD_SEND\n", Message->Cmd_Type);
				break;
				
			case TCC_CMD_REPLY:
				printf(" Message->Cmd_Type=%d : TCC_CMD_REPLY\n", Message->Cmd_Type);
				break;
				
			case TCC_CMD_IND:
				printf(" Message->Cmd_Type=%d : TCC_CMD_IND\n", Message->Cmd_Type);
				break;
				
			default:
				break;
		}
		
		printf(" Message->Free_Flag=%d\n", Message->Free_Flag);
		printf(" Message->Data_Size=%d\n", Message->Data_Size);
}
#endif
