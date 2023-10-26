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
#include "tcc_common_cmd.h"
#include "tcc_dxb_isdbtfseg_cmd_list.h"


/******************************************************************************
* Funtions
******************************************************************************/
void TCC_DXB_ISDBTFSEG_Exit_CMD(Message_Type MsgType)
{
	TCC_MESSAGE_t* pProcessMessage;
	TCC_DXB_ISDBTFSEG_CMD_LIST_Exit_T *ptCmd;

	ptCmd = TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_CMD_LIST_Exit_T));
	if (ptCmd == NULL)
		return;

	pProcessMessage = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_EXIT, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_CMD_LIST_Exit_T),ptCmd, NULL);
	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcessMessage);
}

void TCC_DXB_ISDBTFSEG_Search_CMD(Message_Type MsgType, int iScanType, int iCountryCode, int iAreaCode, int iChannelNum, int iOptions)
{
	TCC_MESSAGE_t *pProcessMessage;
	TCC_DXB_ISDBTFSEG_CMD_LIST_Scan_T *ptCmd;

	printf("[%s:%d] Scan\n", __func__, __LINE__);
	ptCmd = TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_CMD_LIST_Scan_T));
	if (ptCmd == NULL)
		return;

	ptCmd->iScanType 		= iScanType;
	ptCmd->iCountryCode 	= iCountryCode;
	ptCmd->iAreaCode 		= iAreaCode;
	ptCmd->iChannelNum  	= iChannelNum;
	ptCmd->iOptions			= iOptions;

	pProcessMessage = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SCAN, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_CMD_LIST_Scan_T), ptCmd,	NULL);
	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcessMessage);
}

void TCC_DXB_ISDBTFSEG_SearchCancel_CMD(Message_Type MsgType)
{
	printf("[%s:%d] ScanStop\n", __func__, __LINE__);
	TCC_MESSAGE_t *pProcessMessage;
	TCC_DXB_ISDBTFSEG_CMD_LIST_ScanStop_T *ptCmd;

	ptCmd = TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_CMD_LIST_ScanStop_T));
	if (ptCmd == NULL)
		return;

	ptCmd->iResult	= 1;

	pProcessMessage = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SCANSTOP, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_CMD_LIST_ScanStop_T), ptCmd,	NULL);
	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcessMessage);
}

void TCC_DXB_ISDBTFSEG_SetChannel_CMD(Message_Type MsgType, int mainRowID, int subRowID, int audioIndex, int videoIndex, int audioMode, int raw_w, int raw_h, int view_w, int view_h, int ch_index)
{
	TCC_MESSAGE_t *pProcessMessage;
	TCC_DXB_ISDBTFSEG_CMD_LIST_Set_T *ptCmd;

	ptCmd = TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_CMD_LIST_Set_T));
	if (ptCmd == NULL)
		return;

	ptCmd->mainRowID	= mainRowID;// jini Null Pointer Dereference
	ptCmd->subRowID		= subRowID;
	ptCmd->audioIndex	= audioIndex;
	ptCmd->videoIndex	= videoIndex;
	ptCmd->audioMode	= audioMode;
	ptCmd->raw_w 		= raw_w;
	ptCmd->raw_h 		= raw_h;
	ptCmd->view_w 		= view_w;
	ptCmd->view_h 		= view_h;
	ptCmd->ch_index		= ch_index;

	pProcessMessage = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_CMD_LIST_Set_T), ptCmd, NULL);
	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcessMessage);
}

void TCC_DXB_ISDBTFSEG_SetChannel_CMD_withChNum(Message_Type MsgType, int channelNumber, int serviceID_Fseg, int serviceID_1seg, int audioMode, int raw_w, int raw_h, int view_w, int view_h, int ch_index)
{
	TCC_MESSAGE_t *pProcessMessage;
	TCC_DXB_ISDBTFSEG_CMD_LIST_Set_T_WITHCHNUM *ptCmd;

	ptCmd = TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_CMD_LIST_Set_T_WITHCHNUM));
	if (ptCmd == NULL)
		return;

	ptCmd->channelNumber  = channelNumber;
	ptCmd->serviceID_Fseg = serviceID_Fseg;
	ptCmd->serviceID_1seg = serviceID_1seg;
	ptCmd->audioMode      = audioMode;
	ptCmd->raw_w          = raw_w;
	ptCmd->raw_h          = raw_h;
	ptCmd->view_w         = view_w;
	ptCmd->view_h         = view_h;
	ptCmd->ch_index       = ch_index;

	pProcessMessage = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_WITHCHNUM, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_CMD_LIST_Set_T_WITHCHNUM), ptCmd, NULL);
	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcessMessage);
}

void TCC_DXB_ISDBTFSEG_Stop_CMD(Message_Type MsgType)
{
	TCC_MESSAGE_t* pProcessMessage;
	TCC_DXB_ISDBTFSEG_CMD_LIST_Stop_T *ptCmd;

	ptCmd = TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_CMD_LIST_Stop_T));
	if (ptCmd == NULL)
		return;

	pProcessMessage = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_STOP, TCC_CMD_SEND,	TRUE, sizeof(TCC_DXB_ISDBTFSEG_CMD_LIST_Stop_T), ptCmd, NULL);
	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcessMessage);
}

void TCC_DXB_ISDBTFSEG_SetDualChannel_CMD(Message_Type MsgType, int index)
{
	TCC_MESSAGE_t *pProcessMessage;
	TCC_DXB_ISDBTFSEG_CMD_LIST_SetDualChannel_T *ptCmd;

	ptCmd = TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_CMD_LIST_SetDualChannel_T));
	if (ptCmd == NULL)
		return;

	ptCmd->iChannelIndex = index;

	pProcessMessage = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_DUAL_CHANNEL, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_CMD_LIST_SetDualChannel_T), ptCmd, NULL);
	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcessMessage);
}

void TCC_DXB_ISDBTFSEG_CMD_PROCESS_Show(Message_Type MsgType)
{
	TCC_MESSAGE_t* pProcessMessage;

	pProcessMessage = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SHOW_CHANNEL_LIST, TCC_CMD_SEND, TRUE, 0, NULL, NULL);
	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcessMessage);
}

void TCC_DXB_ISDBTFSEG_GetCurrentDateTime_CMD(Message_Type MsgType, TCC_DXB_ISDBTFSEG_GET_CURRENT_DATETIME_PARAM_t *pGetCurrentDateTime_Param)
{
	TCC_DXB_ISDBTFSEG_GET_CURRENT_DATETIME_CMD_t* pGetCurrentDateTime_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pGetCurrentDateTime_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_GET_CURRENT_DATETIME_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pGetCurrentDateTime_Cmd->pGetCurrentDateTime_Param = pGetCurrentDateTime_Param;)
	if (pGetCurrentDateTime_Cmd == NULL)
		return;
	pGetCurrentDateTime_Cmd->pGetCurrentDateTime_Param = pGetCurrentDateTime_Param;

	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_GET_DATETIME, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_GET_CURRENT_DATETIME_CMD_t), pGetCurrentDateTime_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);
}

void TCC_DXB_ISDBTFSEG_Prepare_CMD(Message_Type MsgType, int iSpecificInfo)
{
	TCC_DXB_ISDBTFSEG_PREPARE_CMD_t* pPrepare_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pPrepare_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_PREPARE_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pPrepare_Cmd->iSpecificInfo = iSpecificInfo;)
	if (pPrepare_Cmd == NULL)
		return;
	pPrepare_Cmd->iSpecificInfo = iSpecificInfo;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_PREPARE, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_PREPARE_CMD_t), pPrepare_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_Start_CMD(Message_Type MsgType, int iCountryCode)
{
	TCC_DXB_ISDBTFSEG_START_CMD_t* pStart_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pStart_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_START_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pStart_Cmd->iCountryCode = iCountryCode;)
	if (pStart_Cmd == NULL)
		return;
	pStart_Cmd->iCountryCode = iCountryCode;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_START, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_START_CMD_t), pStart_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_StartTestMode_CMD(Message_Type MsgType, int iIndex)
{
	TCC_DXB_ISDBTFSEG_START_TEST_MODE_CMD_t* pStartTestMode_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pStartTestMode_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_START_TEST_MODE_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pStartTestMode_Cmd->iIndex = iIndex;)
	if (pStartTestMode_Cmd == NULL)
		return;
	pStartTestMode_Cmd->iIndex = iIndex;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_START_TEST_MODE, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_START_TEST_MODE_CMD_t), pStartTestMode_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_Release_CMD(Message_Type MsgType)
{
	TCC_DXB_ISDBTFSEG_RELEASE_CMD_t* pRelease_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pRelease_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_RELEASE_CMD_t), 1);
	if (pRelease_Cmd == NULL)
		return;

	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_RELEASE, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_RELEASE_CMD_t), pRelease_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_ChannelBackUp_CMD(Message_Type MsgType)
{
	TCC_DXB_ISDBTFSEG_CHANNEL_BACKUP_CMD_t* pChannelBackUp_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pChannelBackUp_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_CHANNEL_BACKUP_CMD_t), 1);
	if (pChannelBackUp_Cmd == NULL)
		return;

	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_CHANNEL_BACKUP, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_CHANNEL_BACKUP_CMD_t), pChannelBackUp_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_ChannelDBRestoration_CMD(Message_Type MsgType)
{
	TCC_DXB_ISDBTFSEG_CHANNEL_DB_RESTORATION_CMD_t* pChannelDBRestoration_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pChannelDBRestoration_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_CHANNEL_DB_RESTORATION_CMD_t), 1);
	if (pChannelDBRestoration_Cmd == NULL)
		return;

	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_CHANNEL_DB_RESTORATION, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_CHANNEL_DB_RESTORATION_CMD_t), pChannelDBRestoration_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_SetEPG_CMD(Message_Type MsgType, int iOption)
{
	TCC_DXB_ISDBTFSEG_SET_EPG_CMD_t* pSetEPG_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetEPG_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_EPG_CMD_t), 1);
	if (pSetEPG_Cmd == NULL)
		return;

	pSetEPG_Cmd->iOption = iOption;

	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_EPG, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_EPG_CMD_t), pSetEPG_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);
}

void TCC_DXB_ISDBTFSEG_EPGSearch_CMD(Message_Type MsgType, int iChannelNum, int iOption)
{
	TCC_DXB_ISDBTFSEG_EPG_SEARCH_CMD_t* pEPGSearch_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pEPGSearch_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_EPG_SEARCH_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pEPGSearch_Cmd->iChannelNum = iChannelNum;)
	if (pEPGSearch_Cmd == NULL)
		return;
	pEPGSearch_Cmd->iChannelNum = iChannelNum;
	pEPGSearch_Cmd->iOption = iOption;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_EPG_SEARCH, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_EPG_SEARCH_CMD_t), pEPGSearch_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_EPGSearchCancel_CMD(Message_Type MsgType)
{
	TCC_DXB_ISDBTFSEG_EPG_SEARCH_CANCEL_CMD_t* pEPGSearchCancel_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pEPGSearchCancel_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_EPG_SEARCH_CANCEL_CMD_t), 1);
	if (pEPGSearchCancel_Cmd == NULL)
		return;

	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_EPG_SEARCH_CANCEL, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_EPG_SEARCH_CANCEL_CMD_t), pEPGSearchCancel_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_ReleaseSurface_CMD(Message_Type MsgType)
{
	TCC_DXB_ISDBTFSEG_RELEASE_SURFACE_CMD_t* pReleaseSurface_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pReleaseSurface_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_RELEASE_SURFACE_CMD_t), 1);
	if (pReleaseSurface_Cmd == NULL)
		return;

	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_RELEASE_SURFACE, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_RELEASE_SURFACE_CMD_t), pReleaseSurface_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_UseSurface_CMD(Message_Type MsgType, int iArg)
{
	TCC_DXB_ISDBTFSEG_USE_SURFACE_CMD_t* pUseSurface_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pUseSurface_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_USE_SURFACE_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pUseSurface_Cmd->iArg = iArg;)
	if (pUseSurface_Cmd == NULL)
		return;
	pUseSurface_Cmd->iArg = iArg;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_USE_SURFACE, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_USE_SURFACE_CMD_t), pUseSurface_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_SetCountryCode_CMD(Message_Type MsgType, int iCountryCode)
{
	TCC_DXB_ISDBTFSEG_SET_COUNTRYCODE_CMD_t* pSetCountryCode_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetCountryCode_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_COUNTRYCODE_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pSetCountryCode_Cmd->iCountryCode = iCountryCode;)
	if (pSetCountryCode_Cmd == NULL)
		return;
	pSetCountryCode_Cmd->iCountryCode = iCountryCode;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_COUNTRYCODE, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_COUNTRYCODE_CMD_t), pSetCountryCode_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_SetGroup_CMD(Message_Type MsgType, int iIndex)
{
	TCC_DXB_ISDBTFSEG_SET_GROUP_CMD_t* pSetGroup_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetGroup_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_GROUP_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pSetGroup_Cmd->iIndex = iIndex;)
	if (pSetGroup_Cmd == NULL)
		return;
	pSetGroup_Cmd->iIndex = iIndex;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_GROUP, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_GROUP_CMD_t), pSetGroup_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_SetAudio_CMD(Message_Type MsgType, int iIndex)
{
	TCC_DXB_ISDBTFSEG_SET_AUDIO_CMD_t* pSetAudio_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetAudio_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_AUDIO_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pSetAudio_Cmd->iIndex = iIndex;)
	if (pSetAudio_Cmd == NULL)
		return;
	pSetAudio_Cmd->iIndex = iIndex;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_AUDIO, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_AUDIO_CMD_t), pSetAudio_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_SetVideo_CMD(Message_Type MsgType, int iIndex)
{
	TCC_DXB_ISDBTFSEG_SET_VIDEO_CMD_t* pSetVideo_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetVideo_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_VIDEO_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pSetVideo_Cmd->iIndex = iIndex;)
	if (pSetVideo_Cmd == NULL)
		return;
	pSetVideo_Cmd->iIndex = iIndex;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_VIDEO, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_VIDEO_CMD_t), pSetVideo_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_SetAudioOnOff_CMD(Message_Type MsgType, int iOnOff)
{
	TCC_DXB_ISDBTFSEG_SET_AUDIO_ONOFF_CMD_t* pSetAudioOnOff_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetAudioOnOff_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_AUDIO_ONOFF_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pSetAudioOnOff_Cmd->iOnOff = iOnOff;)
	if (pSetAudioOnOff_Cmd == NULL)
		return;
	pSetAudioOnOff_Cmd->iOnOff = iOnOff;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_AUDIO_ONOFF, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_AUDIO_ONOFF_CMD_t), pSetAudioOnOff_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);
}

void TCC_DXB_ISDBTFSEG_SetAudioMuteOnOff_CMD(Message_Type MsgType, int iOnOff)
{
	TCC_DXB_ISDBTFSEG_SET_AUDIO_MUTE_ONOFF_CMD_t* pSetAudioMuteOnOff_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetAudioMuteOnOff_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_AUDIO_MUTE_ONOFF_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pSetAudioMuteOnOff_Cmd->iOnOff = iOnOff;)
	if (pSetAudioMuteOnOff_Cmd == NULL)
		return;
	pSetAudioMuteOnOff_Cmd->iOnOff = iOnOff;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_AUDIO_MUTE_ONOFF, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_AUDIO_MUTE_ONOFF_CMD_t), pSetAudioMuteOnOff_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);
}

void TCC_DXB_ISDBTFSEG_SetVideoOnOff_CMD(Message_Type MsgType, int iOnOff)
{
	TCC_DXB_ISDBTFSEG_SET_VIDEO_ONOFF_CMD_t* pSetVideoOnOff_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetVideoOnOff_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_VIDEO_ONOFF_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pSetVideoOnOff_Cmd->iOnOff = iOnOff;)
	if (pSetVideoOnOff_Cmd == NULL)
		return;
	pSetVideoOnOff_Cmd->iOnOff = iOnOff;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_VIDEO_ONOFF, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_VIDEO_ONOFF_CMD_t), pSetVideoOnOff_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);
}

void TCC_DXB_ISDBTFSEG_SetVideoOutput_CMD(Message_Type MsgType, int iOnOff)
{
	TCC_DXB_ISDBTFSEG_SET_VIDEO_OUTPUT_CMD_t* pSetVideoOutPut_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetVideoOutPut_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_VIDEO_OUTPUT_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pSetVideoOutPut_Cmd->iOnOff = iOnOff;)
	if (pSetVideoOutPut_Cmd == NULL)
		return;
	pSetVideoOutPut_Cmd->iOnOff = iOnOff;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_VIDEO_VIS_ONOFF, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_VIDEO_OUTPUT_CMD_t), pSetVideoOutPut_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_SetAudioVideoSyncMode_CMD(Message_Type MsgType, int iMode)
{
	TCC_DXB_ISDBTFSEG_SET_AV_SYNC_MODE_CMD_t* pSetAudioVideoSyncMode_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetAudioVideoSyncMode_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_AV_SYNC_MODE_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pSetAudioVideoSyncMode_Cmd->iMode = iMode;)
	if (pSetAudioVideoSyncMode_Cmd == NULL)
		return;
	pSetAudioVideoSyncMode_Cmd->iMode = iMode;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_AV_SYNC_MODE, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_AV_SYNC_MODE_CMD_t), pSetAudioVideoSyncMode_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_SetVolume_CMD(Message_Type MsgType, int iVolume)
{
	TCC_DXB_ISDBTFSEG_SET_VOLUME_CMD_t* pSetVolume_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetVolume_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_VOLUME_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pSetVolume_Cmd->iVolume = iVolume;)
	if (pSetVolume_Cmd == NULL)
		return;
	pSetVolume_Cmd->iVolume = iVolume;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_VOLUME, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_VOLUME_CMD_t), pSetVolume_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_SetVolumeLR_CMD(Message_Type MsgType, int iLeftVolume, int iRightVolume)
{
	TCC_DXB_ISDBTFSEG_SET_VOLUME_LR_CMD_t* pSetVolumeLR_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetVolumeLR_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_VOLUME_LR_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pSetVolumeLR_Cmd->iLeftVolume = iLeftVolume;)
	if (pSetVolumeLR_Cmd == NULL)
		return;
	pSetVolumeLR_Cmd->iLeftVolume = iLeftVolume;
	pSetVolumeLR_Cmd->iRightVolume = iRightVolume;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_VOLUME_LR, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_VOLUME_LR_CMD_t), pSetVolumeLR_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_SetDisplayPosition_CMD(Message_Type MsgType, int iX, int iY, int iWidth, int iHeight, int iRotate)
{
	TCC_DXB_ISDBTFSEG_SET_DISPLAY_POSITION_CMD_t* pSetDisplayPosition_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetDisplayPosition_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_DISPLAY_POSITION_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pSetDisplayPosition_Cmd->iX = iX;)
	if (pSetDisplayPosition_Cmd == NULL)
		return;
	pSetDisplayPosition_Cmd->iX = iX;
	pSetDisplayPosition_Cmd->iY = iY;
	pSetDisplayPosition_Cmd->iWidth = iWidth;
	pSetDisplayPosition_Cmd->iHeight = iHeight;
	pSetDisplayPosition_Cmd->iRotate = iRotate;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_DISPLAY_POSITION, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_DISPLAY_POSITION_CMD_t), pSetDisplayPosition_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_SetCapture_CMD(Message_Type MsgType, char *pcFilePath)
{
	TCC_DXB_ISDBTFSEG_SET_CAPTURE_CMD_t* pSetCapture_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetCapture_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_CAPTURE_CMD_t), 1);
	if (pSetCapture_Cmd == NULL)
		return;

	memcpy(pSetCapture_Cmd->acFilePath, pcFilePath, strlen(pcFilePath) + 1);
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_CAPTURE, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_CAPTURE_CMD_t), pSetCapture_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_PlaySubtitle_CMD(Message_Type MsgType, int iOnOff)
{
	TCC_DXB_ISDBTFSEG_PLAY_SUBTITLE_CMD_t* pPlaySubtitle_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pPlaySubtitle_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_PLAY_SUBTITLE_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pPlaySubtitle_Cmd->iOnOff = iOnOff;)
	if (pPlaySubtitle_Cmd == NULL)
		return;
	pPlaySubtitle_Cmd->iOnOff = iOnOff;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_PLAY_SUBTITLE, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_PLAY_SUBTITLE_CMD_t), pPlaySubtitle_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_PlaySuperimpose_CMD(Message_Type MsgType, int iOnOff)
{
	TCC_DXB_ISDBTFSEG_PLAY_SUPERIMPOSE_CMD_t* pPlaySuperimpose_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pPlaySuperimpose_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_PLAY_SUPERIMPOSE_CMD_t), 1);
	if (pPlaySuperimpose_Cmd == NULL)
		return;
	pPlaySuperimpose_Cmd->iOnOff = iOnOff;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_PLAY_SUPERIMPOSE, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_PLAY_SUPERIMPOSE_CMD_t), pPlaySuperimpose_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_PlayPng_CMD(Message_Type MsgType, int iOnOff)
{
	TCC_DXB_ISDBTFSEG_PLAY_PNG_CMD_t* pPlayPng_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pPlayPng_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_PLAY_PNG_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pPlayPng_Cmd->iOnOff = iOnOff;)
	if (pPlayPng_Cmd == NULL)
		return;
	pPlayPng_Cmd->iOnOff = iOnOff;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_PLAY_PNG, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_PLAY_PNG_CMD_t), pPlayPng_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_SetSubtitle_CMD(Message_Type MsgType, int iIndex)
{
	TCC_DXB_ISDBTFSEG_SET_SUBTITLE_CMD_t* pSetSubtitle_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetSubtitle_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_SUBTITLE_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pSetSubtitle_Cmd->iIndex = iIndex;)
	if (pSetSubtitle_Cmd == NULL)
		return;
	pSetSubtitle_Cmd->iIndex = iIndex;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_SUBTITLE, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_SUBTITLE_CMD_t), pSetSubtitle_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_SetSuperimpose_CMD(Message_Type MsgType, int iIndex)
{
	TCC_DXB_ISDBTFSEG_SET_SUPERIMPOSE_CMD_t* pSetSuperimpose_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetSuperimpose_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_SUPERIMPOSE_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pSetSuperimpose_Cmd->iIndex = iIndex;)
	if (pSetSuperimpose_Cmd == NULL)
		return;
	pSetSuperimpose_Cmd->iIndex = iIndex;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_SUPERIMPOSE, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_SUPERIMPOSE_CMD_t), pSetSuperimpose_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_SetAudioDualMono_CMD(Message_Type MsgType, int iAudioSelect)
{
	TCC_DXB_ISDBTFSEG_SET_AUDIO_DUALMONO_CMD_t* pSetAudioDualMono_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetAudioDualMono_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_AUDIO_DUALMONO_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pSetAudioDualMono_Cmd->iAudioSelect = iAudioSelect;)
	if (pSetAudioDualMono_Cmd == NULL)
		return;
	pSetAudioDualMono_Cmd->iAudioSelect = iAudioSelect;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_AUDIO_DUALMONO, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_AUDIO_DUALMONO_CMD_t), pSetAudioDualMono_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_SetParentalRate_CMD(Message_Type MsgType, int iAge)
{
	TCC_DXB_ISDBTFSEG_SET_PARENTALRATE_CMD_t* pSetParentalRate_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetParentalRate_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_PARENTALRATE_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pSetParentalRate_Cmd->iAge = iAge;)
	if (pSetParentalRate_Cmd == NULL)
		return;
	pSetParentalRate_Cmd->iAge = iAge;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_PARENTALRATE, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_PARENTALRATE_CMD_t), pSetParentalRate_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_SetArea_CMD(Message_Type MsgType, int iAreaCode)
{
	TCC_DXB_ISDBTFSEG_SET_AREA_CMD_t* pSetArea_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetArea_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_AREA_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pSetArea_Cmd->iAreaCode = iAreaCode;)
	if (pSetArea_Cmd == NULL)
		return;
	pSetArea_Cmd->iAreaCode = iAreaCode;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_AREA, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_AREA_CMD_t), pSetArea_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_SetPreset_CMD(Message_Type MsgType, int iAreaCode)
{
	TCC_DXB_ISDBTFSEG_SET_PRESET_CMD_t* pSetPreset_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetPreset_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_PRESET_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pSetPreset_Cmd->iAreaCode = iAreaCode;)
	if (pSetPreset_Cmd == NULL)
		return;
	pSetPreset_Cmd->iAreaCode = iAreaCode;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_PRESET, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_PRESET_CMD_t), pSetPreset_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_SetHandover_CMD(Message_Type MsgType, int iCountryCode)
{
	TCC_DXB_ISDBTFSEG_SET_HANDOVER_CMD_t* pSetHandover_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetHandover_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_HANDOVER_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pSetHandover_Cmd->iCountryCode = iCountryCode;)
	if (pSetHandover_Cmd == NULL)
		return;
	pSetHandover_Cmd->iCountryCode = iCountryCode;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_HANDOVER, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_HANDOVER_CMD_t), pSetHandover_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_RequestEPGUpdate_CMD(Message_Type MsgType, int iDayOffset)
{
	TCC_DXB_ISDBTFSEG_REQ_EPG_UPDATE_CMD_t* pRequestEPGUpdate_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pRequestEPGUpdate_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_REQ_EPG_UPDATE_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pRequestEPGUpdate_Cmd->iDayOffset= iDayOffset;)
	if (pRequestEPGUpdate_Cmd == NULL)
		return;
	pRequestEPGUpdate_Cmd->iDayOffset= iDayOffset;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_REQ_EPG_UPDATE, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_REQ_EPG_UPDATE_CMD_t), pRequestEPGUpdate_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_ReqSCInfo_CMD(Message_Type MsgType, int iArg)
{
	TCC_DXB_ISDBTFSEG_REQ_SC_INFO_CMD_t* pReqSCInfo_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pReqSCInfo_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_REQ_SC_INFO_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pReqSCInfo_Cmd->iArg = iArg;)
	if (pReqSCInfo_Cmd == NULL)
		return;
	pReqSCInfo_Cmd->iArg = iArg;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_REQ_SC_INFO, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_REQ_SC_INFO_CMD_t), pReqSCInfo_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_SetFreqBand_CMD(Message_Type MsgType, int iFreqBand)
{
	TCC_DXB_ISDBTFSEG_SET_FREQ_BAND_CMD_t* pSetFreqBand_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetFreqBand_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_FREQ_BAND_CMD_t), 1);
	if (pSetFreqBand_Cmd == NULL)
		return;
	pSetFreqBand_Cmd->iFreqBand = iFreqBand;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_FREQ_BAND, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_FREQ_BAND_CMD_t), pSetFreqBand_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_SetFieldLog_CMD(Message_Type MsgType, char *pcPath, int iOnOff)
{
	TCC_DXB_ISDBTFSEG_SET_FIELD_LOG_CMD_t* pSetFieldLog_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetFieldLog_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_FIELD_LOG_CMD_t), 1);
	if (pSetFieldLog_Cmd == NULL)
		return;
	memcpy(pSetFieldLog_Cmd->acPath, pcPath, strlen(pcPath) + 1);
	pSetFieldLog_Cmd->iOnOff = iOnOff;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_FIELD_LOG, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_FIELD_LOG_CMD_t), pSetFieldLog_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_SetPresetMode_CMD(Message_Type MsgType, int iPresetMode)
{
	TCC_DXB_ISDBTFSEG_SET_PRESET_MODE_CMD_t* pSetPresetMode_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetPresetMode_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_PRESET_MODE_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pSetPresetMode_Cmd->iPresetMode = iPresetMode;)
	if (pSetPresetMode_Cmd == NULL)
		return;
	pSetPresetMode_Cmd->iPresetMode = iPresetMode;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_PRESET_MODE, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_PRESET_MODE_CMD_t), pSetPresetMode_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_SetUserDataRegionID_CMD(Message_Type MsgType, int iRegionID)
{
	TCC_DXB_ISDBTFSEG_SET_USERDATA_REGIONID_CMD_t* pSetUserDataRegionID_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetUserDataRegionID_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_USERDATA_REGIONID_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pSetUserDataRegionID_Cmd->iRegionID = iRegionID;)
	if (pSetUserDataRegionID_Cmd == NULL)
		return;
	pSetUserDataRegionID_Cmd->iRegionID = iRegionID;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_USERDATA_REGIONID, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_USERDATA_REGIONID_CMD_t), pSetUserDataRegionID_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_SetUserDataPrefecturalCode_CMD(Message_Type MsgType, int iPrefecturalBitOrder)
{
	TCC_DXB_ISDBTFSEG_SET_USERDATA_PREFECTURALCODE_CMD_t* pSetUserDataPrefecturalCode_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetUserDataPrefecturalCode_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_USERDATA_PREFECTURALCODE_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pSetUserDataPrefecturalCode_Cmd->iPrefecturalBitOrder = iPrefecturalBitOrder;)
	if (pSetUserDataPrefecturalCode_Cmd == NULL)
		return;
	pSetUserDataPrefecturalCode_Cmd->iPrefecturalBitOrder = iPrefecturalBitOrder;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_USERDATA_PREFECTURALCODE, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_USERDATA_PREFECTURALCODE_CMD_t), pSetUserDataPrefecturalCode_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_SetUserDataAreaCode_CMD(Message_Type MsgType, int iAreaCode)
{
	TCC_DXB_ISDBTFSEG_SET_USERDATA_AREACODE_CMD_t* pSetUserDataAreaCode_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetUserDataAreaCode_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_USERDATA_AREACODE_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pSetUserDataAreaCode_Cmd->iAreaCode = iAreaCode;)
	if (pSetUserDataAreaCode_Cmd == NULL)
		return;
	pSetUserDataAreaCode_Cmd->iAreaCode = iAreaCode;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_USERDATA_AREACODE, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_USERDATA_AREACODE_CMD_t), pSetUserDataAreaCode_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_SetUserDataPostalCode_CMD(Message_Type MsgType, char *pcPostalCode)
{
	TCC_DXB_ISDBTFSEG_SET_USERDATA_POSTALCODE_CMD_t* pSetUserDataPostalCode_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	if(pcPostalCode == NULL)
	{
		printf("[%s] pcPostalCode == NULL Error !!!!!\n", pcPostalCode);
		return;
	}

	pSetUserDataPostalCode_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_USERDATA_POSTALCODE_CMD_t), 1);
	if (pSetUserDataPostalCode_Cmd == NULL)
		return;
	memcpy(pSetUserDataPostalCode_Cmd->acPostalCode, pcPostalCode, strlen(pcPostalCode) + 1);
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_USERDATA_POSTALCODE, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_USERDATA_POSTALCODE_CMD_t), pSetUserDataPostalCode_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_SetListener_CMD(Message_Type MsgType, int (*pfListener)(int, int, int, void *))
{
	TCC_DXB_ISDBTFSEG_SET_LISTENER_CMD_t* pSetListener_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetListener_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_LISTENER_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pIsPlaying_Cmd->pIsPlaying_Param = pIsPlaying_Param;)
	if (pSetListener_Cmd == NULL)
		return;
	pSetListener_Cmd->pfListener = pfListener;// jini Null Pointer Dereference
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_LISTENER, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_LISTENER_CMD_t), pSetListener_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_IsPlaying_CMD(Message_Type MsgType, TCC_DXB_ISDBTFSEG_RETURN_PARAM_t *pIsPlaying_Param)
{
	TCC_DXB_ISDBTFSEG_IS_PLAYING_CMD_t* pIsPlaying_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pIsPlaying_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_IS_PLAYING_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pIsPlaying_Cmd->pIsPlaying_Param = pIsPlaying_Param;)
	if (pIsPlaying_Cmd == NULL)
		return;
	pIsPlaying_Cmd->pIsPlaying_Param = pIsPlaying_Param;

	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_IS_PLAYING, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_IS_PLAYING_CMD_t), pIsPlaying_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_GetVideoWidth_CMD(Message_Type MsgType, TCC_DXB_ISDBTFSEG_GET_VIDEO_WIDTH_PARAM_t *pGetVideoWidth_Param)
{
	TCC_DXB_ISDBTFSEG_GET_VIDEO_WIDTH_CMD_t* pGetVideoWidth_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pGetVideoWidth_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_GET_VIDEO_WIDTH_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pGetVideoWidth_Cmd->pGetVideoWidth_Param = pGetVideoWidth_Param;)
	if (pGetVideoWidth_Cmd == NULL)
		return;
	pGetVideoWidth_Cmd->pGetVideoWidth_Param = pGetVideoWidth_Param;

	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_GET_VIDEO_WIDTH, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_GET_VIDEO_WIDTH_CMD_t), pGetVideoWidth_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_GetVideoHeight_CMD(Message_Type MsgType, TCC_DXB_ISDBTFSEG_GET_VIDEO_HEIGHT_PARAM_t *pGetVideoHeight_Param)
{
	TCC_DXB_ISDBTFSEG_GET_VIDEO_HEIGHT_CMD_t* pGetVideoHeight_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pGetVideoHeight_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_GET_VIDEO_HEIGHT_CMD_t), 1);
	if (pGetVideoHeight_Cmd == NULL)
		return;
	pGetVideoHeight_Cmd->pGetVideoHeight_Param = pGetVideoHeight_Param;

	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_GET_VIDEO_HEIGHT, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_GET_VIDEO_HEIGHT_CMD_t), pGetVideoHeight_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_GetSignalStrength_CMD(Message_Type MsgType, TCC_DXB_ISDBTFSEG_GET_SIGNAL_STRENGTH_PARAM_t *pGetSignalStrength_Param)
{
	TCC_DXB_ISDBTFSEG_GET_SIGNAL_STRENGTH_CMD_t* pGetSignalStrength_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pGetSignalStrength_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_GET_SIGNAL_STRENGTH_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pGetSignalStrength_Cmd->pGetSignalStrength_Param = pGetSignalStrength_Param;)
	if (pGetSignalStrength_Cmd == NULL)
		return;
	pGetSignalStrength_Cmd->pGetSignalStrength_Param = pGetSignalStrength_Param;

	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_GET_SIGNAL_STRENGTH, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_GET_SIGNAL_STRENGTH_CMD_t), pGetSignalStrength_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_CustomRelayStationSearchRequest_CMD(Message_Type MsgType, int iSearchKind, int *piListChannel, int *piListTSID, int iRepeat)
{
	TCC_DXB_ISDBTFSEG_CUSTOM_RELAY_STATION_SEARCH_REQUEST_CMD_t* pCustomRelayStationSearchRequest_Cmd;
	TCC_MESSAGE_t *pProcess_Message;
	int iListChannel_Cnt = 0;
	int iListTSID_Cnt = 0;

	pCustomRelayStationSearchRequest_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_CUSTOM_RELAY_STATION_SEARCH_REQUEST_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pCustomRelayStationSearchRequest_Cmd->iSearchKind = iSearchKind;)
	if (pCustomRelayStationSearchRequest_Cmd == NULL)
		return;
	pCustomRelayStationSearchRequest_Cmd->iSearchKind = iSearchKind;
	while(piListChannel[iListChannel_Cnt++] != -1);
	memcpy(pCustomRelayStationSearchRequest_Cmd->aiListChannel, piListChannel, iListChannel_Cnt*sizeof(int));
	while(piListTSID[iListTSID_Cnt++] != -1);
	memcpy(pCustomRelayStationSearchRequest_Cmd->aiListTSID, piListTSID, iListTSID_Cnt*sizeof(int));
	pCustomRelayStationSearchRequest_Cmd->iRepeat = iRepeat;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_CUSTOM_RELAY_STATION_SEARCH_REQUEST, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_CUSTOM_RELAY_STATION_SEARCH_REQUEST_CMD_t), pCustomRelayStationSearchRequest_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_GetCountryCode_CMD(Message_Type MsgType, TCC_DXB_ISDBTFSEG_RETURN_PARAM_t *pGetCountryCode_Param)
{
	TCC_DXB_ISDBTFSEG_GET_GOUNTRYCODE_CMD_t* pGetCountryCode_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pGetCountryCode_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_GET_GOUNTRYCODE_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pGetCountryCode_Cmd->pGetCountryCode_Param = pGetCountryCode_Param;)
	if (pGetCountryCode_Cmd == NULL)
		return;
	pGetCountryCode_Cmd->pGetCountryCode_Param = pGetCountryCode_Param;

	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_GET_GOUNTRYCODE, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_GET_GOUNTRYCODE_CMD_t), pGetCountryCode_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_GetAudioCnt_CMD(Message_Type MsgType, TCC_DXB_ISDBTFSEG_RETURN_PARAM_t *pGetAudioCnt_Param)
{
	TCC_DXB_ISDBTFSEG_GET_AUDIO_CNT_CMD_t* pGetAudioCnt_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pGetAudioCnt_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_GET_AUDIO_CNT_CMD_t), 1);
	if (pGetAudioCnt_Cmd == NULL)
		return;
	pGetAudioCnt_Cmd->pGetAudioCnt_Param = pGetAudioCnt_Param;

	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_GET_AUDIO_CNT, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_GET_AUDIO_CNT_CMD_t), pGetAudioCnt_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_GetChannelInfo_CMD(Message_Type MsgType, int iChannelNumber, int iServiceID, TCC_DXB_ISDBTFSEG_GET_CHANNEL_INFO_PARAM_t *pGetChannelInfo_Param)
{
	TCC_DXB_ISDBTFSEG_GET_CHANNEL_INFO_CMD_t* pGetChannelInfo_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pGetChannelInfo_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_GET_CHANNEL_INFO_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pGetChannelInfo_Cmd->iChannelNumber = iChannelNumber;)
	if (pGetChannelInfo_Cmd == NULL)
		return;
	pGetChannelInfo_Cmd->iChannelNumber = iChannelNumber;
	pGetChannelInfo_Cmd->iServiceID = iServiceID;
	pGetChannelInfo_Cmd->pGetChannelInfo_Param = pGetChannelInfo_Param;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_GET_CHANNEL_INFO, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_GET_CHANNEL_INFO_CMD_t), pGetChannelInfo_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_ReqTRMPInfo_CMD(Message_Type MsgType, TCC_DXB_ISDBTFSEG_REQ_TRMP_INFO_PARAM_t *pReqTRMPInfo_Param)
{
	TCC_DXB_ISDBTFSEG_REQ_TRMP_INFO_CMD_t* pReqTRMPInfo_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pReqTRMPInfo_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_REQ_TRMP_INFO_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pReqTRMPInfo_Cmd->pReqTRMPInfo_Param = pReqTRMPInfo_Param;)
	if (pReqTRMPInfo_Cmd == NULL)
		return;
	pReqTRMPInfo_Cmd->pReqTRMPInfo_Param = pReqTRMPInfo_Param;

	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_REQ_TRMP_INFO, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_REQ_TRMP_INFO_CMD_t), pReqTRMPInfo_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_GetSubtitleLangInfo_CMD(Message_Type MsgType, TCC_DXB_ISDBTFSEG_GET_SUBTITLE_LANG_INFO_PARAM_t *pGetSubtitleLangInfo_Param)
{
	TCC_DXB_ISDBTFSEG_GET_SUBTITLE_LANG_INFO_CMD_t* pGetSubtitleLangInfo_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pGetSubtitleLangInfo_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_GET_SUBTITLE_LANG_INFO_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pGetSubtitleLangInfo_Cmd->pGetSubtitleLangInfo_Param = pGetSubtitleLangInfo_Param;)
	if (pGetSubtitleLangInfo_Cmd == NULL)
		return;
	pGetSubtitleLangInfo_Cmd->pGetSubtitleLangInfo_Param = pGetSubtitleLangInfo_Param;

	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_GET_SUBTITLE_LANG_INFO, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_GET_SUBTITLE_LANG_INFO_CMD_t), pGetSubtitleLangInfo_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_GetUserData_CMD(Message_Type MsgType, TCC_DXB_ISDBTFSEG_GET_USERDATA_PARAM_t *pGetUserData_Param)
{
	TCC_DXB_ISDBTFSEG_GET_USERDATA_CMD_t* pGetUserData_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pGetUserData_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_GET_USERDATA_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pGetUserData_Cmd->pGetUserData_Param = pGetUserData_Param;)
	if (pGetUserData_Cmd == NULL)
		return;
	pGetUserData_Cmd->pGetUserData_Param = pGetUserData_Param;

	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_GET_USERDATA, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_GET_USERDATA_CMD_t), pGetUserData_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_SetCustomTuner_CMD(Message_Type MsgType, int iSize, void *pvArg)
{
	TCC_DXB_ISDBTFSEG_SET_CUSTOM_TUNER_CMD_t* pSetCustomTuner_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetCustomTuner_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_CUSTOM_TUNER_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pSetCustomTuner_Cmd->iSize = iSize;)
	if (pSetCustomTuner_Cmd == NULL)
		return;
	pSetCustomTuner_Cmd->iSize = iSize;
	memcpy(pSetCustomTuner_Cmd->aiArg, pvArg, iSize);
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_CUSTOM_TUNER, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_CUSTOM_TUNER_CMD_t), pSetCustomTuner_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_GetCustomTuner_CMD(Message_Type MsgType, TCC_DXB_ISDBTFSEG_GET_CUSTOM_TUNER_PARAM_t *pGetCustomTuner_Param)
{
	TCC_DXB_ISDBTFSEG_GET_CUSTOM_TUNER_CMD_t* pGetCustomTuner_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pGetCustomTuner_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_GET_CUSTOM_TUNER_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pGetCustomTuner_Cmd->pGetCustomTuner_Param = pGetCustomTuner_Param;)
	if (pGetCustomTuner_Cmd == NULL)
		return;
	pGetCustomTuner_Cmd->pGetCustomTuner_Param = pGetCustomTuner_Param;

	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_GET_CUSTOM_TUNER, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_GET_CUSTOM_TUNER_CMD_t), pGetCustomTuner_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_GetDigitalCopyControlDescriptor_CMD(Message_Type MsgType, unsigned short usServiceID)
{
	TCC_DXB_ISDBTFSEG_GET_DCCD_CMD_t* pGetDigitalCopyControlDescriptor_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pGetDigitalCopyControlDescriptor_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_GET_DCCD_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pGetDigitalCopyControlDescriptor_Cmd->usServiceID = usServiceID;)
	if (pGetDigitalCopyControlDescriptor_Cmd == NULL)
		return;
	pGetDigitalCopyControlDescriptor_Cmd->usServiceID = usServiceID;
	pGetDigitalCopyControlDescriptor_Cmd->pGetDCCD_Param = 0;

	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_GET_DCCD, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_GET_DCCD_CMD_t), pGetDigitalCopyControlDescriptor_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_GetContentAvailabilityDescriptor_CMD(Message_Type MsgType, unsigned short usServiceID, TCC_DXB_ISDBTFSEG_GET_CAD_PARAM_t *pGetContentAvailabilityDescriptor_Param)
{
	TCC_DXB_ISDBTFSEG_GET_CAD_CMD_t* pGetContentAvailabilityDescriptor_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pGetContentAvailabilityDescriptor_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_GET_CAD_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pGetContentAvailabilityDescriptor_Cmd->usServiceID = usServiceID;)
	if (pGetContentAvailabilityDescriptor_Cmd == NULL)
		return;
	pGetContentAvailabilityDescriptor_Cmd->usServiceID = usServiceID;
	pGetContentAvailabilityDescriptor_Cmd->pGetCAD_Param = pGetContentAvailabilityDescriptor_Param;

	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_GET_CAD, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_GET_CAD_CMD_t), pGetContentAvailabilityDescriptor_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);

}

void TCC_DXB_ISDBTFSEG_SetNotifyDetectSection_CMD(Message_Type MsgType, int iSectionIDs)
{
	TCC_DXB_ISDBTFSEG_SET_NOTIFY_DETECT_SECTION_CMD_t* pSetNotifyDetectSection_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetNotifyDetectSection_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_NOTIFY_DETECT_SECTION_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pSetNotifyDetectSection_Cmd->iSectionIDs = iSectionIDs;)
	if (pSetNotifyDetectSection_Cmd == NULL)
		return;
	pSetNotifyDetectSection_Cmd->iSectionIDs = iSectionIDs;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_NOTIFY_DETECT_SECTION, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_NOTIFY_DETECT_SECTION_CMD_t), pSetNotifyDetectSection_Cmd, NULL);
	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);
}

void TCC_DXB_ISDBTFSEG_SearchAndSetChannel_CMD(Message_Type MsgType, int iCountryCode, int iChannelNum, int iTsid, int iOptions, int iAudioIndex, int iVideoIndex, int iAudioMode, int iRaw_w, int iRaw_h, int iView_w, int iView_h, int iCh_index)
{
	TCC_DXB_ISDBTFSEG_SEARCH_AND_SET_CMD_t* pSearchAndSetChannel_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSearchAndSetChannel_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SEARCH_AND_SET_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pSearchAndSetChannel_Cmd->iCountryCode = iCountryCode;)
	if (pSearchAndSetChannel_Cmd == NULL)
		return;
	pSearchAndSetChannel_Cmd->iCountryCode = iCountryCode;
	pSearchAndSetChannel_Cmd->iChannelNum = iChannelNum;
	pSearchAndSetChannel_Cmd->iTsid = iTsid;
	pSearchAndSetChannel_Cmd->iOptions = iOptions;
	pSearchAndSetChannel_Cmd->iAudioIndex = iAudioIndex;
	pSearchAndSetChannel_Cmd->iVideoIndex = iVideoIndex;
	pSearchAndSetChannel_Cmd->iAudioMode = iAudioMode;
	pSearchAndSetChannel_Cmd->iRaw_w = iRaw_w;
	pSearchAndSetChannel_Cmd->iRaw_h = iRaw_h;
	pSearchAndSetChannel_Cmd->iView_w = iView_w;
	pSearchAndSetChannel_Cmd->iView_h = iView_h;
	pSearchAndSetChannel_Cmd->iCh_index = iCh_index;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SEARCH_AND_SET, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SEARCH_AND_SET_CMD_t), pSearchAndSetChannel_Cmd, NULL);
	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);
}

void TCC_DXB_ISDBTFSEG_SetDeviceKeyUpdateFunction_CMD(Message_Type MsgType, unsigned char *(*pfUpdateDeviceKey)(unsigned char , unsigned int))
{
	TCC_DXB_ISDBTFSEG_SET_DEVICE_KEY_UPDATE_FUNCTION_CMD_t* pSetDeviceKeyUpdateFunction_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetDeviceKeyUpdateFunction_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_DEVICE_KEY_UPDATE_FUNCTION_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pSetDeviceKeyUpdateFunction_Cmd->pfUpdateDeviceKey = pfUpdateDeviceKey;)
	if (pSetDeviceKeyUpdateFunction_Cmd == NULL)
		return;
	pSetDeviceKeyUpdateFunction_Cmd->pfUpdateDeviceKey = pfUpdateDeviceKey;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_DEVICE_KEY_UPDATE_FUNCTION, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_DEVICE_KEY_UPDATE_FUNCTION_CMD_t), pSetDeviceKeyUpdateFunction_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);
}

void TCC_DXB_ISDBTFSEG_GetVideoState_CMD(Message_Type MsgType, TCC_DXB_ISDBTFSEG_RETURN_PARAM_t *pGetVideoState_Param)
{
	TCC_DXB_ISDBTFSEG_GET_VIDEO_STATE_CMD_t* pGetVideoState_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pGetVideoState_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_GET_VIDEO_STATE_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pGetVideoState_Cmd->pGetVideoState_Param = pGetVideoState_Param;)
	if (pGetVideoState_Cmd == NULL)
		return;
	pGetVideoState_Cmd->pGetVideoState_Param = pGetVideoState_Param;

	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_GET_VIDEO_STATE, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_GET_VIDEO_STATE_CMD_t), pGetVideoState_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);
}

void TCC_DXB_ISDBTFSEG_GetSearchState_CMD(Message_Type MsgType, TCC_DXB_ISDBTFSEG_RETURN_PARAM_t *pGetSearchState_Param)
{
	TCC_DXB_ISDBTFSEG_GET_SEARCH_STATE_CMD_t* pGetSearchState_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pGetSearchState_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_GET_SEARCH_STATE_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pGetSearchState_Cmd->pGetSearchState_Param = pGetSearchState_Param;)
	if (pGetSearchState_Cmd == NULL)
		return;
	pGetSearchState_Cmd->pGetSearchState_Param = pGetSearchState_Param;

	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_GET_SEARCH_STATE, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_GET_SEARCH_STATE_CMD_t), pGetSearchState_Cmd, NULL);

	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);
}

void TCC_DXB_ISDBTFSEG_SetDataServiceStart_CMD(Message_Type MsgType)
{
	TCC_DXB_ISDBTFSEG_SET_DATASERVICE_START_CMD_t* pSetDataServiceStart_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pSetDataServiceStart_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_SET_DATASERVICE_START_CMD_t), 1);
	if (pSetDataServiceStart_Cmd == NULL)
		return;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_DATASERVICE_START, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_SET_DATASERVICE_START_CMD_t), pSetDataServiceStart_Cmd, NULL);
	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);
}

void TCC_DXB_ISDBTFSEG_CustomFilterStart_CMD(Message_Type MsgType, int iPID, int iTableID)
{
	TCC_DXB_ISDBTFSEG_CUSTOM_FILTER_START_CMD_t* pCustomFilterStart_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pCustomFilterStart_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_CUSTOM_FILTER_START_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pCustomFilterStart_Cmd->iPID = iPID;)
	if (pCustomFilterStart_Cmd == NULL)
		return;
	pCustomFilterStart_Cmd->iPID = iPID;
	pCustomFilterStart_Cmd->iTableID = iTableID;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_CUSTOM_FILTER_START, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_CUSTOM_FILTER_START_CMD_t), pCustomFilterStart_Cmd, NULL);
	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);
}

void TCC_DXB_ISDBTFSEG_CustomFilterStop_CMD(Message_Type MsgType, int iPID, int iTableID)
{
	TCC_DXB_ISDBTFSEG_CUSTOM_FILTER_STOP_CMD_t* pCustomFilterStop_Cmd;
	TCC_MESSAGE_t *pProcess_Message;

	pCustomFilterStop_Cmd = TCC_calloc(sizeof(TCC_DXB_ISDBTFSEG_CUSTOM_FILTER_STOP_CMD_t), 1);
	// jini 12th (Null Pointer Dereference => pCustomFilterStop_Cmd->iPID = iPID;)
	if (pCustomFilterStop_Cmd == NULL)
		return;
	pCustomFilterStop_Cmd->iPID = iPID;
	pCustomFilterStop_Cmd->iTableID = iTableID;
	pProcess_Message = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_CUSTOM_FILTER_STOP, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_CUSTOM_FILTER_STOP_CMD_t), pCustomFilterStop_Cmd, NULL);
	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcess_Message);
}

void TCC_DXB_ISDBTFSEG_EWS_start_CMD(Message_Type MsgType, int mainRowID, int subRowID, int audioIndex, int videoIndex, int audioMode, int raw_w, int raw_h, int view_w, int view_h, int ch_index)
{
	TCC_MESSAGE_t *pProcessMessage;
	TCC_DXB_ISDBTFSEG_EWS_START_CMD_t *ptCmd;

	ptCmd = TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_EWS_START_CMD_t));
	if (ptCmd == NULL)
		return;
	ptCmd->mainRowID	= mainRowID;
	ptCmd->subRowID 	= subRowID;
	ptCmd->audioIndex	= audioIndex;
	ptCmd->videoIndex	= videoIndex;
	ptCmd->audioMode	= audioMode;
	ptCmd->raw_w		= raw_w;
	ptCmd->raw_h		= raw_h;
	ptCmd->view_w		= view_w;
	ptCmd->view_h		= view_h;
	ptCmd->ch_index 	= ch_index;

	pProcessMessage = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_EWS_START, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_EWS_START_CMD_t), ptCmd, NULL);
	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcessMessage);
}

void TCC_DXB_ISDBTFSEG_EWS_start_CMD_withChNum(Message_Type MsgType, int channelNumber, int serviceID_Fseg, int serviceID_1seg, int audioMode, int raw_w, int raw_h, int view_w, int view_h, int ch_index)
{
	TCC_MESSAGE_t *pProcessMessage;
	TCC_DXB_ISDBTFSEG_EWS_START_CMD_t_WITHCHNUM *ptCmd;

	ptCmd = TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_EWS_START_CMD_t_WITHCHNUM));
	if (ptCmd == NULL)
		return;
	ptCmd->channelNumber  = channelNumber;
	ptCmd->serviceID_Fseg = serviceID_Fseg;
	ptCmd->serviceID_1seg = serviceID_1seg;
	ptCmd->audioMode	= audioMode;
	ptCmd->raw_w		= raw_w;
	ptCmd->raw_h		= raw_h;
	ptCmd->view_w		= view_w;
	ptCmd->view_h		= view_h;
	ptCmd->ch_index 	= ch_index;

	pProcessMessage = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_EWS_START_WITHCHNUM, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_EWS_START_CMD_t_WITHCHNUM), ptCmd, NULL);
	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcessMessage);
}

void TCC_DXB_ISDBTFSEG_EWS_clear_CMD(Message_Type MsgType)
{
	TCC_MESSAGE_t *pProcessMessage;
	TCC_DXB_ISDBTFSEG_EWS_CLEAR_CMD_t *ptCmd;

	ptCmd = TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_EWS_CLEAR_CMD_t));

	pProcessMessage = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_EWS_CLEAR, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_EWS_CLEAR_CMD_t), ptCmd, NULL);
	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcessMessage);
}

void TCC_DXB_ISDBTFSEG_Set_Seamless_Switch_Compensation_CMD(Message_Type MsgType, int onoff, int interval, int strength, int ntimes, int range, int gapadjust, int gapadjust2, int multiplier)
{
	TCC_MESSAGE_t *pProcessMessage;
	TCC_DXB_ISDBTFSEG_Seamless_Switch_Compensation_CMD_t *ptCmd;

	ptCmd = TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_Seamless_Switch_Compensation_CMD_t));
	// jini 12th (Null Pointer Dereference => ptCmd->iOnOff = onoff;)
	if (ptCmd == NULL)
		return;
	ptCmd->iOnOff = onoff;
	ptCmd->iInterval = interval;
	ptCmd->iStrength = strength;
	ptCmd->iNtimes = ntimes;
	ptCmd->iRange = range;
	ptCmd->iGapadjust = gapadjust;
	ptCmd->iGapadjust2 = gapadjust2;
	ptCmd->iMultiplier = multiplier;

	pProcessMessage = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_SET_SEAMLESS_SWITCH_COMPENSATION, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_Seamless_Switch_Compensation_CMD_t), ptCmd, NULL);
	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcessMessage);
}

void TCC_DXB_ISDBTFSEG_GetSTC_CMD(Message_Type MsgType, unsigned int *hi_data, unsigned *lo_data)
{
	TCC_MESSAGE_t *pProcessMessage;
	TCC_DXB_ISDBTFSEG_DS_GET_STC_CMD_t *ptCmd;

	ptCmd = TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_DS_GET_STC_CMD_t));
	// jini 12th (Null Pointer Dereference => ptCmd->hi_data = hi_data;)
	if (ptCmd == NULL)
		return;
	ptCmd->hi_data = hi_data;
	ptCmd->lo_data = lo_data;
	pProcessMessage = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_DS_GET_STC, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_DS_GET_STC_CMD_t), ptCmd, NULL);
	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcessMessage);
}

void TCC_DXB_ISDBTFSEG_GetServiceTime_CMD(Message_Type MsgType, unsigned int *year, unsigned int *month, unsigned int *day, unsigned int *hour, unsigned int *min, unsigned int *sec)
{
	TCC_MESSAGE_t *pProcessMessage;
	TCC_DXB_ISDBTFSEG_DS_GET_SERVICE_TIME_CMD_t *ptCmd;

	ptCmd = TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_DS_GET_SERVICE_TIME_CMD_t));
	// jini 12th (Null Pointer Dereference => ptCmd->year		= year;)
	if (ptCmd == NULL)
		return;
	ptCmd->year		= year;
	ptCmd->month	= month;
	ptCmd->day		= day;
	ptCmd->hour		= hour;
	ptCmd->min		= min;
	ptCmd->sec		= sec;
	pProcessMessage = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_DS_GET_SERVICETIME, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_DS_GET_SERVICE_TIME_CMD_t), ptCmd, NULL);
	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcessMessage);
}

void TCC_DXB_ISDBTFSEG_GetContentID_CMD(Message_Type MsgType, int *content_id, int *associated_contents_flag)
{
	TCC_MESSAGE_t *pProcessMessage;
	TCC_DXB_ISDBTFSEG_DS_GET_CONTENTID_CMD_t *ptCmd;

	ptCmd = TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_DS_GET_CONTENTID_CMD_t));
	// jini 12th (Null Pointer Dereference => ptCmd->contentID = content_id;)
	if (ptCmd == NULL)
		return;
	ptCmd->contentID = content_id;
	ptCmd->associated_contents_flag = associated_contents_flag;
	pProcessMessage = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_DS_GET_CONTENTID, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_DS_GET_CONTENTID_CMD_t), ptCmd, NULL);
	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcessMessage);
}

void TCC_DXB_ISDBTFSEG_GetSeamlessValue_CMD(Message_Type MsgType, int *state, int *cval, int *pval)
{
	TCC_MESSAGE_t *pProcessMessage;
	TCC_DXB_ISDBTFSEG_GET_SEAMLESSVALUE_CMD_t *ptCmd;

	ptCmd = TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_GET_SEAMLESSVALUE_CMD_t));
	if (ptCmd == NULL)
		return;
	ptCmd->state = state;
	ptCmd->cval = cval;
	ptCmd->pval = pval;
	pProcessMessage = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_GET_SEAMLESSVALUE, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_GET_SEAMLESSVALUE_CMD_t), ptCmd, NULL);
	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcessMessage);

}

void TCC_DXB_ISDBTFSEG_CheckExistComponentTagInPMT_CMD(Message_Type MsgType, int target_component_tag)
{
	TCC_MESSAGE_t *pProcessMessage;
	TCC_DXB_ISDBTFSEG_DS_CHECK_EXIST_COMPONENTTAG_IN_PMT_CMD_t *ptCmd;

	ptCmd = TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_DS_CHECK_EXIST_COMPONENTTAG_IN_PMT_CMD_t));
	// jini 12th (Null Pointer Dereference => ptCmd->target_component_tag = target_component_tag;)
	if (ptCmd == NULL)
		return;
	ptCmd->target_component_tag = target_component_tag;
	pProcessMessage = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_DS_CHECK_EXISTCOMPONENTTAGINPMT, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_DS_CHECK_EXIST_COMPONENTTAG_IN_PMT_CMD_t), ptCmd, NULL);
	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcessMessage);
}

void TCC_DXB_ISDBTFSEG_SetVideoByComponentTag_CMD(Message_Type MsgType, int component_tag)
{
	TCC_MESSAGE_t *pProcessMessage;
	TCC_DXB_ISDBTFSEG_DS_SET_VIDEO_BY_COMPONENTTAG_CMD_t *ptCmd;

	ptCmd = TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_DS_SET_VIDEO_BY_COMPONENTTAG_CMD_t));
	// jini 12th (Null Pointer Dereference => ptCmd->component_tag = component_tag;)
	if (ptCmd == NULL)
		return;
	ptCmd->component_tag = component_tag;
	pProcessMessage = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_DS_SET_VIDEOBYCOMPONENTTAG, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_DS_SET_VIDEO_BY_COMPONENTTAG_CMD_t), ptCmd, NULL);
	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcessMessage);
}

void TCC_DXB_ISDBTFSEG_SetAudioByComponentTag_CMD(Message_Type MsgType, int component_tag)
{
	TCC_MESSAGE_t *pProcessMessage;
	TCC_DXB_ISDBTFSEG_DS_SET_AUDIO_BY_COMPONENTTAG_CMD_t *ptCmd;

	ptCmd = TCC_malloc(sizeof(TCC_DXB_ISDBTFSEG_DS_SET_AUDIO_BY_COMPONENTTAG_CMD_t));
	// jini 12th (Null Pointer Dereference => ptCmd->component_tag = component_tag;)
	if (ptCmd == NULL)
		return;
	ptCmd->component_tag = component_tag;
	pProcessMessage = TCC_DXB_ISDBTFSEG_Set_Process_Message(TCC_OPERAND_DS_SET_AUDIOBYCOMPONENTTAG, TCC_CMD_SEND, TRUE, sizeof(TCC_DXB_ISDBTFSEG_DS_SET_AUDIO_BY_COMPONENTTAG_CMD_t), ptCmd, NULL);
	TCC_Put_Message2(MsgType, TCC_DXB_ISDBTFSEG_CMD_Cmd.pProcessMessageQueue, pProcessMessage);
}
