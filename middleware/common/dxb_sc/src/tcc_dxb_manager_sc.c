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

/****************************************************************************

Revision History

****************************************************************************

****************************************************************************/

#define LOG_NDEBUG 1
#define LOG_TAG	"TCC_DXB_MANAGER_SC"
#include <utils/Log.h>
#include <pthread.h>
#include <fcntl.h>
#include <tcc_sc_ioctl.h>

#include "tcc_dxb_interface_sc.h"
#include "tcc_dxb_manager_sc.h"

#if LOG_NDEBUG
#undef	DEBUG
#define DEBUG(msg...)
#else
#undef	DEBUG
#define DEBUG(n, msg...)	ALOGE(msg)
#endif

#define MAX_CARD_DETECT_ERROR_CNT	3

static sTCC_SC_ATR_CHARS stATRChar;
static SC_Cmd_APDU_Header_Architecture_t SC_CMD_Header_ARCH; 
BCAS_InitSet_Architecture_t BCAS_InitSet_ARCH;
BCAS_Ecm_Architecture_t	BCAS_Ecm_ARCH;
BCAS_Default_Architecture_t BCAS_Emm_Arch;
BCAS_CardID_Architecture_t BCAS_CardID_Arch;	
BCAS_EMM_Indi_Message_Architecture_t BCAS_EMM_Indi_MessageArch;
BCAS_Auto_Disp_Message_Architecture_t BCAS_Auto_Disp_MessageArch;
	
EMM_Message_Section_Header_t 		EMM_MessSection_Header;
EMM_Common_Message_t			EMM_ComMessage[BCAS_MAX_EMM_COM_MESSAGE_CNT];
EMM_Individual_Message_t			EMM_IndiMessage[BCAS_MAX_EMM_COM_MESSAGE_CNT];
EMM_MailMessage_Code_Region_t		EMM_MailMessage_CodeResion;
EMM_AutoMessage_Code_Region_t		EMM_AutoMessage_CodeResion;
EMM_Message_Disp_Info_t			EMM_Message_DispInfo;
EMG_ResCMD_MessageCode_LengthCheck_t	EMG_ResCMD_MessageCode_LengthCheck;
EMM_Message_Receive_Status_t		EMM_Message_Receive_status;

pthread_mutex_t tcc_sc_mutex;

static unsigned int	MAX_DATA_FIELD_LENGTH=0;
static unsigned int	SC_PROTOCOL_TYPE = 0;
static unsigned int	EDC_TYPE = 0;
static unsigned int INITFIELD_EDC_POS = 0;
static unsigned int	APDU_RES_LENGTH;
static unsigned int	Seq_Num = SC_BLOCK_DEF_PCB;
static unsigned int	SC_CLOSE_FLAG = 1;
static unsigned int	TCC_SC_DESCIRPTION_MODE = MULTI2_MODE_HW;
static unsigned int	AutoDisp_Message_GetStatus = 0;
static unsigned int	EMM_ComMessage_Cnt = 0;
static unsigned int	EMM_IndiMessage_Cnt = 0;

static unsigned int	ISDBT_MESSAGE_TYPE = EMM_BASIC;
static unsigned int	Broadcaster_GroupID = 0;
static unsigned int	Message_ID = 0;
static unsigned int	ISDB_Mail_NoPreset_Message_check =0;
static unsigned int	BCAS_Multi_Partion_Position = 0;	//for EMG Multi partition 

#define	EMM_MALLOC_DEBUGING
#ifdef EMM_MALLOC_DEBUGING		//EMM malloc size debuging
static unsigned long malloc_size = 0;
static unsigned long free_size = 0;
#endif

/**************************************************************************
 FUNCTION NAME : TCC_DxB_CAS_KeySwap
  DESCRIPTION : 
  INPUT	 :  encryptionSetting - Parameter
 OUTPUT  : int - Return Type
 REMARK  : 
**************************************************************************/
int TCC_DxB_CAS_KeySwap (unsigned char *srckey, int size)
{
	unsigned char *destkey;
	int		i;

	destkey = tcc_mw_malloc(__FUNCTION__, __LINE__, size);
	if(destkey == NULL)
	{
		ALOGE("%s %d / destkey malloc fail / Error !!!!!\n", __func__, __LINE__);
		return 0;
	}

	for(i=0;i<size; i+=4)
	{
		destkey[i] = srckey[i+3];
		destkey[i+1] = srckey[i+2];
		destkey[i+2] = srckey[i+1];
		destkey[i+3] = srckey[i];
	}
#if 0
	for(i=0; i<size; i+=4)
	{
		ALOGE("0x%02x 0x%02x 0x%02x 0x%02x\n", srckey[i+0], srckey[i+1], srckey[i+2], srckey[i+3]);
		ALOGE("0x%02x 0x%02x 0x%02x 0x%02x\n", destkey[i+0], destkey[i+1], destkey[i+2], destkey[i+3]);
	}
#endif
	memcpy(srckey, destkey, size);

	tcc_mw_free(__FUNCTION__, __LINE__, destkey);

	return 0;
}


void TCC_DxB_SC_Manager_SetDescriptionMode(int desc_mode)
{
	TCC_SC_DESCIRPTION_MODE = desc_mode;	
}

int TCC_DxB_SC_Manager_Init(void)
{
	unsigned char ucAtrBuf[ISDBT_MAX_BLOCK_SIZE];
	unsigned int	uiAtrLength;
	int	err = SC_ERR_OK;

	DEBUG(DEFAULT_MESSAGES, "%s %d \n", __func__, __LINE__);
		
	TCC_Dxb_SC_Manager_InitParam();
	SC_CLOSE_FLAG = FALSE;
	err = TCC_Dxb_SC_Manager_GetATR(0, ucAtrBuf, &uiAtrLength );
	if(err != SC_ERR_OK)
	{
		DEBUG(DEFAULT_MESSAGES, "%s %d err = %x \n", __func__, __LINE__, err);
		return err;
	}
	DEBUG(DEFAULT_MESSAGES, "%s %d  err = %d \n", __func__, __LINE__, err);
	DEBUG(DEFAULT_MESSAGES, "%s %d  uiAtrLength = %d \n", __func__, __LINE__, uiAtrLength);

	TCC_Dxb_SC_Manager_ChangeIFSD();

	TCC_Dxb_SC_Manager_CmdHeaderSet(BCAS_CMD_HEADER_DEF_CLA, BCAS_CMD_HEADER_DEF_P1, BCAS_CMD_HEADER_DEF_P2);
	err = TCC_DxB_SC_Manager_SendT1(SC_INS_PARAM_INT, 0, NULL);

	return err;
}

/************************************************************************************************************
* FUNCTION NAME : TCC_Dxb_SC_Manager_ChangeIFSD
* DESCRIPTION :  IFSD change
* INPUT	 : 
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
int TCC_Dxb_SC_Manager_ChangeIFSD(void)
{
	unsigned char	ucWriteBuf[ISDBT_MAX_BLOCK_SIZE] = { 0, };
	unsigned char	edc_lrc = 0;
	unsigned int	uiWriteLength = 0;
	unsigned char ucReadBuf[ISDBT_MAX_BLOCK_SIZE] = { 0, };
	unsigned int	uiReadLength = 0;
	int	err = SC_ERR_OK;
	unsigned char	ucReceiveBlock = 0;
	unsigned char	ucReceiveIFSD = 0;

	ucWriteBuf[0] = SC_BLOCK_DEF_NAD;
	ucWriteBuf[INITFIELD_PCB_POS] = SC_BLOCK_DEF_SPCB | SC_SBLOKC_BLOKC_TYPE_REQUEST|SC_SBLOCK_CON_IFS;
	ucWriteBuf[INITFIELD_LEN_POS] = 1;
	ucWriteBuf[INITFIELD_LEN_POS +1] = ISDBT_SC_MAX_IFSD;
	TCC_Dxb_SC_Manager_CalcEDC (ucWriteBuf, INITFIELD_LEN_POS+2, &edc_lrc);
	ucWriteBuf[INITFIELD_LEN_POS+2] = edc_lrc;
	uiWriteLength = 5;
#if 0
	{
		int i;
		ALOGE("%s %d ucWriteBuf  start \n", __func__, __LINE__);
		for (i=0; i<uiWriteLength; i++)
		{
			ALOGE("  i =%d, ucWriteBuf = %x ", i,  ucWriteBuf[i]);
		}
		ALOGE("%s %d uiWriteLength = %d \n", __func__, __LINE__, uiWriteLength);
	}	
#endif

	err = TCC_DxB_SC_ReadData(0, ucWriteBuf, uiWriteLength, ucReadBuf, &uiReadLength);

#if 0
	{
		int i;
		ALOGE("%s %d ucReadBuf  start \n", __func__, __LINE__);
		for (i=0; i<uiReadLength; i++)
		{
			ALOGE("  i =%d, ucReadBuf = %x ", i,  ucReadBuf[i]);
		}
		ALOGE("%s %d uiReadLength = %d \n", __func__, __LINE__, uiReadLength);
	}
#endif

	ucReceiveBlock= ucReadBuf[INITFIELD_PCB_POS];
	ucReceiveIFSD = ucReadBuf[INITFIELD_LEN_POS +1];
		
	if(ucReceiveBlock == (SC_BLOCK_DEF_SPCB|SC_SBLOKC_BLOKC_TYPE_RESPONSE | SC_SBLOCK_CON_IFS))
	{
		if(ucReceiveIFSD == ISDBT_SC_MAX_IFSD)
			err =  SC_CHANGE_IFSD_OK;
	}
	else
		err =  SC_CHANGE_IFSD_ERROR;

	ALOGE("%s %d err = %d \n", __func__, __LINE__, err);
	
	return err;
}

/************************************************************************************************************
* FUNCTION NAME : TCC_DxB_SC_Manager_CardDetect
* DESCRIPTION :  init parameter
* INPUT	 : 
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
int TCC_DxB_SC_Manager_CardDetect(void)
{
	DEBUG(DEFAULT_MESSAGES, "%s %d \n", __func__, __LINE__);

	if(TCC_DxB_SC_CardDetect() !=DxB_ERR_OK)
		return SC_ERR_CARD_DETECT;

	return DxB_ERR_OK;
}

void TCC_DxB_SC_Manager_ClearCardInfo (void)
{
	memset (&BCAS_CardID_Arch, 0, sizeof(BCAS_CardID_Architecture_t));
}

int TCC_DxB_SC_Manager_GetCardInfo (void  **pCardInfo, int *pSize)
{
	*pCardInfo = (void *)&BCAS_CardID_Arch;
	*pSize = sizeof(BCAS_CardID_Architecture_t);
	return 0;
}

/************************************************************************************************************
* FUNCTION NAME : TCC_Dxb_SC_Manager_InitParam
* DESCRIPTION :  init parameter
* INPUT	 : 
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
void TCC_Dxb_SC_Manager_InitParam(void)
{
	DEBUG(DEFAULT_MESSAGES, "%s %d \n", __func__, __LINE__);

//	MAX_DATA_FIELD_LENGTH=0;
	SC_PROTOCOL_TYPE = 0;
	INITFIELD_EDC_POS = 0;
	APDU_RES_LENGTH =0;
	Seq_Num = SC_BLOCK_DEF_PCB;
	AutoDisp_Message_GetStatus =0;

	memset(&BCAS_InitSet_ARCH, 0, sizeof(BCAS_InitSet_ARCH));
	memset(&EMG_ResCMD_MessageCode_LengthCheck, 0, sizeof(EMG_ResCMD_MessageCode_LengthCheck));
}


/************************************************************************************************************
* FUNCTION NAME : TCC_Dxb_SC_Manager_AutoDisp_Message_Get
* DESCRIPTION :  Automatic Display Message Dsiplay Information Acquire Command
* INPUT	 : 
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
int TCC_Dxb_SC_Manager_AutoDisp_Message_Get(unsigned short uiCurrDate, unsigned char ucCA_Gr_ID, unsigned char ucDelay_Interval)
{
	int	err =SC_ERR_MAX;
	unsigned char	SC_EMD_Data[ISDBT_EMM_AUTODISP_MESSAGE_COM_DEF_LEN];
	
	if(uiCurrDate != 0xffff)
	{
		SC_EMD_Data[AUOT_DISP_CURRENT_DATA] = uiCurrDate>>8;
		SC_EMD_Data[AUOT_DISP_CURRENT_DATA +1] = 0xff & uiCurrDate;
		SC_EMD_Data[AUOT_DISP_BROADCAST_GR_ID]= ucCA_Gr_ID;
		SC_EMD_Data[AUTO_DISP_DELAYINTERVAL]= ucDelay_Interval;

//		ALOGE("SC_EMD_Data[%x, %x,%x, %x]\n", SC_EMD_Data[0], SC_EMD_Data[1], SC_EMD_Data[2], SC_EMD_Data[3]);
		err = TCC_DxB_SC_Manager_SendT1_Glue((char)SC_INS_PARAM_EMD, ISDBT_EMM_AUTODISP_MESSAGE_COM_DEF_LEN, (char*)SC_EMD_Data);
		if(err == SC_ERR_OK)
		{	
			EMM_Message_Receive_status.uiMessageType =  EMM_AUTODISPLAY_MESSAGE;
			EMM_Message_Receive_status.uiEncrypted_status = EMM_INDIMESSAGE_ENCRYPTED;
			ISDBT_MESSAGE_TYPE = EMM_AUTODISPLAY_MESSAGE;
		}
		AutoDisp_Message_GetStatus =1;
	}
	return err;
}

int TCC_Dxb_SC_Manager_AutoDisp_Message_GetStatus(void)
{
	return AutoDisp_Message_GetStatus;
}

/************************************************************************************************************
* FUNCTION NAME : TCC_Dxb_SC_Manager_EMM_ComMessageInit
* DESCRIPTION :  EMM Common Message Init
* INPUT	 : 
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
void TCC_Dxb_SC_Manager_EMM_ComMessageInit(void)
{
	memset(&BCAS_Auto_Disp_MessageArch, 0, sizeof(BCAS_Auto_Disp_MessageArch));
	BCAS_Auto_Disp_MessageArch.Differential_information = NULL;

	memset(&EMM_MailMessage_CodeResion, 0, sizeof(EMM_MailMessage_CodeResion));
	TCC_Dxb_SC_Manager_EMM_DispMessageInit();
}

void TCC_Dxb_SC_Manager_EMM_DispMessageInit(void)
{
	ALOGE("%s %d \n", __func__, __LINE__);
	memset(&EMM_Message_DispInfo, 0, sizeof(EMM_Message_DispInfo));
}

/************************************************************************************************************
* FUNCTION NAME : TCC_Dxb_SC_Manager_EMM_ComMessageDeInit
* DESCRIPTION :  EMM Common Message Deinit
* INPUT	 : 
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
void TCC_Dxb_SC_Manager_EMM_ComMessageDeInit(void)
{
	ALOGE("%s %d \n", __func__, __LINE__);

	memset(EMM_ComMessage, 0, sizeof(EMM_Common_Message_t)*BCAS_MAX_EMM_COM_MESSAGE_CNT);
	if(BCAS_Auto_Disp_MessageArch.Differential_information != NULL)
		tcc_mw_free(__FUNCTION__, __LINE__, BCAS_Auto_Disp_MessageArch.Differential_information);
}

/************************************************************************************************************
* FUNCTION NAME : TCC_Dxb_SC_Manager_EMM_IndiMessageInit
* DESCRIPTION :  EMM Individual Message Init
* INPUT	 : 
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
void TCC_Dxb_SC_Manager_EMM_IndiMessageInit(void)
{
	int i =0;
	ALOGE("%s %d \n", __func__, __LINE__);

	memset(&BCAS_EMM_Indi_MessageArch, 0, sizeof(BCAS_EMM_Indi_MessageArch));
	memset(&EMM_AutoMessage_CodeResion, 0, sizeof(EMM_AutoMessage_CodeResion));
}

/************************************************************************************************************
* FUNCTION NAME : TCC_Dxb_SC_Manager_EMM_IndiMessageDeInit
* DESCRIPTION :  EMM Individual Message Deinit
* INPUT	 : 
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
void TCC_Dxb_SC_Manager_EMM_IndiMessageDeInit(int cnt)
{
	EMM_Individual_Message_Field_t		*temp_ptr1 = NULL, *temp_ptr2 = NULL;
	EMM_Individual_Message_Field_t		*src_ptr = NULL;
	unsigned int	i=0, clear_cnt = 0;

	if(cnt == 0)
		clear_cnt = EMM_IndiMessage_Cnt;
	else
		clear_cnt = cnt;

	for(i=0; i<clear_cnt; i++)
	{	
		ALOGE("%s %d uiEMM_IndividualMessage_PayloadCnt = %d \n",
			__func__, __LINE__, EMM_IndiMessage[i].uiEMM_IndividualMessage_PayloadCnt);

		if(EMM_IndiMessage[i].EMM_IndividualMessage_Field != NULL)
		{
			if(EMM_IndiMessage[i].uiEMM_IndividualMessage_PayloadCnt > 1)
			{	
				src_ptr = EMM_IndiMessage[i].EMM_IndividualMessage_Field;

				ALOGE("%s %d EMM_IndiMessage.uiEMM_IndividualMessage_PayloadCnt = %d \n",
					__func__, __LINE__, EMM_IndiMessage[i].uiEMM_IndividualMessage_PayloadCnt);

				temp_ptr1 = EMM_IndiMessage[i].EMM_IndividualMessage_Field;
				if(temp_ptr1 == NULL)
					return;

				while(1)
				{	
					if(temp_ptr1->Prev_EMM_IndiMessage != NULL)
					{	
						temp_ptr2 = temp_ptr1;
						temp_ptr1  = temp_ptr1->Prev_EMM_IndiMessage;
					}
					else 
					{
						if(temp_ptr2)
						{
							ALOGE("%s %d free  temp_ptr= %p \n", __func__, __LINE__, temp_ptr1);
							tcc_mw_free(__FUNCTION__, __LINE__, temp_ptr1);
							temp_ptr2->Prev_EMM_IndiMessage = NULL;
#ifdef EMM_MALLOC_DEBUGING		//EMM malloc size debuging
							free_size += (sizeof(EMM_IndiMessage[i].EMM_IndividualMessage_Field));
#endif
							temp_ptr1 = src_ptr;
							EMM_IndiMessage[i].uiEMM_IndividualMessage_PayloadCnt --;
						}
					}

					if(EMM_IndiMessage[i].uiEMM_IndividualMessage_PayloadCnt == 1)
						break;
				}
			}
#ifdef EMM_MALLOC_DEBUGING		//EMM malloc size debuging
			free_size += (sizeof(EMM_Individual_Message_Field_t));
#endif
		ALOGE("%s %d \n", __func__, __LINE__);
			if(EMM_IndiMessage[i].EMM_IndividualMessage_Field != NULL)
				tcc_mw_free(__FUNCTION__, __LINE__, EMM_IndiMessage[i].EMM_IndividualMessage_Field);
		}

		EMM_IndiMessage[i].EMM_IndividualMessage_Field = NULL;
	}	
}

/************************************************************************************************************
* FUNCTION NAME : TCC_Dxb_SC_Manager_EMM_IndiMessageReciveCmd
* DESCRIPTION :  EMM Individual Message Receive Command
* INPUT	 : 
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
int TCC_Dxb_SC_Manager_EMM_IndiMessageReciveCmd(EMM_Individual_Message_Field_t *IndiMessage, unsigned int uidata_len)
{
	int	err =SC_ERR_MAX;
	unsigned int	i=0, j=0;
	unsigned char	SC_EMG_Data[ISDBT_MAX_BLOCK_SIZE] ;
	unsigned int	copy_code_size = 0;	

	if(uidata_len>0)
	{
		if(uidata_len < BCAS_InitSet_ARCH.Message_Partition_len)
		{
			for(i=0; i<BCAS_CARDID_LEGTH; i++)
				 SC_EMG_Data[i] = IndiMessage->ucCard_ID[i];
			SC_EMG_Data[EMM_MESSAGE_PROTOCOL_NUM] = IndiMessage->ucProtocol_Num;
			SC_EMG_Data[EMM_MESSAGE_BR_GR_ID] = IndiMessage->ucCa_Broadcaster_Gr_ID;
			SC_EMG_Data[EMM_MESSAGE_CONTROL] = IndiMessage->ucMessage_Control;
			
			for(i=0; i<uidata_len; i++)
				SC_EMG_Data[EMM_MESSAGE_CODE + i] = IndiMessage->ucMessage_Area[i];
#if 0
			ALOGE("%s %d uidata_len = %d, total_len = %d \n", __func__, __LINE__, uidata_len, (uidata_len + ISDBT_EMM_INDI_MESSSAGE_COM_DEF_LEN));
			ALOGE("%s %d ucCard_ID =[%x, %x, %x, %x, %x, %x]  \n", __func__, __LINE__, SC_EMG_Data[0],SC_EMG_Data[1],SC_EMG_Data[2],SC_EMG_Data[3],SC_EMG_Data[4],SC_EMG_Data[5]);
			ALOGE("%s %d ucMessage_Control = %d\n", __func__, __LINE__, SC_EMG_Data[EMM_MESSAGE_CONTROL]);
#endif
			EMG_ResCMD_MessageCode_LengthCheck.uiTotal_Cnt =1;
			
			err = TCC_DxB_SC_Manager_SendT1_Glue((char)SC_INS_PARAM_EMG, (uidata_len + ISDBT_EMM_INDI_MESSSAGE_COM_DEF_LEN), (char*)SC_EMG_Data);
		}
		else
		{
			copy_code_size = BCAS_InitSet_ARCH.Message_Partition_len;
			j =0; 
			EMG_ResCMD_MessageCode_LengthCheck.uiTotal_Cnt = (uidata_len/BCAS_InitSet_ARCH.Message_Partition_len);
			if((uidata_len/(int)BCAS_InitSet_ARCH.Message_Partition_len) != 0)
				EMG_ResCMD_MessageCode_LengthCheck.uiTotal_Cnt ++;

			EMG_ResCMD_MessageCode_LengthCheck.uiSend_Cnt = 0;
			EMG_ResCMD_MessageCode_LengthCheck.uiReceive_Cnt = 0;

//			ALOGE("%s %d uidata_len = %d, Message_Partition_len = %d, EMG_ResCMD_MessageCode_LengthCheck.uiTotal_Cnt = %d  \n", __func__, __LINE__, uidata_len, BCAS_InitSet_ARCH.Message_Partition_len, EMG_ResCMD_MessageCode_LengthCheck.uiTotal_Cnt);

			for(i=0; i<BCAS_CARDID_LEGTH; i++)
				 SC_EMG_Data[i] = IndiMessage->ucCard_ID[i];
			SC_EMG_Data[EMM_MESSAGE_PROTOCOL_NUM] = IndiMessage->ucProtocol_Num;
			SC_EMG_Data[EMM_MESSAGE_BR_GR_ID] = IndiMessage->ucCa_Broadcaster_Gr_ID;
			SC_EMG_Data[EMM_MESSAGE_CONTROL] = IndiMessage->ucMessage_Control;

			while(1)
			{	
				if((j + copy_code_size)>uidata_len)
					copy_code_size = (uidata_len -j);
	
				for(i=0; i<copy_code_size; i++)
					SC_EMG_Data[EMM_MESSAGE_CODE + i] = IndiMessage->ucMessage_Area[i + j];
#if 0
				ALOGE("%s %d MAX_DATA_FIELD_LENGTH = %d, uidata_len = %d , j = %d, copy_code_size = %d  \n", __func__, __LINE__, MAX_DATA_FIELD_LENGTH, uidata_len, j, copy_code_size);
				ALOGE("%s %d ucCard_ID =[%x, %x, %x, %x, %x, %x]  \n", __func__, __LINE__, SC_EMG_Data[0],SC_EMG_Data[1],SC_EMG_Data[2],SC_EMG_Data[3],SC_EMG_Data[4],SC_EMG_Data[5]);
				ALOGE("%s %d ucMessage_Control = %d\n", __func__, __LINE__, SC_EMG_Data[EMM_MESSAGE_CONTROL]);
#endif
				EMG_ResCMD_MessageCode_LengthCheck.uiSend_Cnt ++;
				err = TCC_DxB_SC_Manager_SendT1_Glue((char)SC_INS_PARAM_EMG, (copy_code_size +ISDBT_EMM_INDI_MESSSAGE_COM_DEF_LEN) , (char*)SC_EMG_Data);

				if((j+BCAS_InitSet_ARCH.Message_Partition_len)>uidata_len)
					break;

				j+=BCAS_InitSet_ARCH.Message_Partition_len ;
			}
		}
	}
	return err;
}

/************************************************************************************************************
* FUNCTION NAME : TCC_Dxb_SC_Manager_EMM_MessageInfoSet
* DESCRIPTION :  EMM Message Info  set
* INPUT	 : 
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
int TCC_Dxb_SC_Manager_EMM_MessageInfoSet(void)
{
	unsigned int	ret = EMM_Message_Err_MAX;
	unsigned int	i = 0, j = 0;
	unsigned char	Message_data[ISDBT_EMM_MAX_MAIL_SIZE] = { 0, };
	unsigned int	uiMessage_Cnt = 0, uiTableid_Chk = 0;

	if(EMM_Message_Receive_status.uiMessageType== EMM_BASIC)
	{
//		ALOGE("%s %d uiMessageType = %d\n", __func__, __LINE__, EMM_Message_Receive_status.uiMessageType);
		return ret;
	}

	if(EMM_Message_DispInfo.ucMessage_Cnt>0)
	{
		for(i=0; i<EMM_Message_DispInfo.ucMessage_Cnt; i++)
		{
			if(! ((EMM_Message_Receive_status.uiMessageType == EMM_MAIL_MESSAGE) && (EMM_MailMessage_CodeResion.uiFixed_Mmessage_ID == ISDBT_MAIL_NO_PRESET_MESSAGE)) 
			&&(EMM_Message_DispInfo.EMM_MessageInfo[i].uiMessageID  == EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.uiTable_Id_Extension)
			&&(EMM_Message_DispInfo.EMM_MessageInfo[i].ucDeletion_Status == EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucDeletion_Status)
			)
			{	
//				ALOGE("%s %d This Message has.. \n", __func__, __LINE__);
				return EMM_Message_Exist;
			}	
		}
	}
//	ALOGE("%s %d uiMessageType = %d, uiFixed_Mmessage_ID=%x \n", __func__, __LINE__, EMM_Message_Receive_status.uiMessageType, EMM_MailMessage_CodeResion.uiFixed_Mmessage_ID);
	if((EMM_Message_Receive_status.uiMessageType == EMM_MAIL_MESSAGE) 
		&& (EMM_MailMessage_CodeResion.uiFixed_Mmessage_ID == ISDBT_MAIL_NO_PRESET_MESSAGE)
	)
	{
		if(EMM_Message_DispInfo.ucMessage_Cnt == ISDBT_EMM_MAX_MESSAGE_CNT)
			EMM_Message_DispInfo.ucMessage_Cnt=0;;	

		EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].uiMessageID = Message_ID;
		EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucBroadcaster_GroupID = Broadcaster_GroupID;
		EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDeletion_Status = 0;
		EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Duration1 = 0;
		EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Duration2 = 0;
		EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Duration3 = 0;
		EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Cycle = 0;
		EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucFormat_Version = EMM_MailMessage_CodeResion.ucExtraMessage_FormatVersion;
		EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Horizontal_PositionInfo = 0;
		EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Vertical_PositionInfo = 0;

		EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].uiMessageLen = EMM_MailMessage_CodeResion.uiExtra_Message_Len;

		ALOGE("%s %d ucMessage_Cnt = %d, uiMessageLen=%d \n", __func__, __LINE__, EMM_Message_DispInfo.ucMessage_Cnt, EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].uiMessageLen );
																									
		memcpy(EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucMessagePayload, 
			EMM_MailMessage_CodeResion.ucExtra_MessageCode, 
			EMM_MailMessage_CodeResion.uiExtra_Message_Len);

		memset(&EMM_MailMessage_CodeResion, 0, sizeof(EMM_MailMessage_CodeResion));

		ISDB_Mail_NoPreset_Message_check = TRUE;

		ret = EMM_Message_Get_Success;
	}
	else if(EMM_Message_Receive_status.uiMessageType == EMM_MAIL_MESSAGE)
	{
#if 1
		if(EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.uiTable_Id_Extension != EMM_MailMessage_CodeResion.uiFixed_Mmessage_ID)
		{	
			ALOGE("%s %d uiFixed_Mmessage_ID=%d, uiTable_Id_Extension = %d \n", __func__, __LINE__, EMM_MailMessage_CodeResion.uiFixed_Mmessage_ID, EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.uiTable_Id_Extension);
			return EMM_Message_Get_Fail;
		}
		else
			uiMessage_Cnt = EMM_ComMessage_Cnt;
#else
		for(i=0; i<BCAS_MAX_EMM_COM_MESSAGE_CNT; i++)
		{	
			if(EMM_ComMessage[i].EMM_MessSection_Header.uiTable_Id_Extension == EMM_MailMessage_CodeResion.uiFixed_Mmessage_ID)
			{
				uiTableid_Chk =1;
				uiMessage_Cnt =i;
				break;
			}	
		}
		if(!uiTableid_Chk)
			return EMM_Message_Get_Fail;
#endif

		if(EMM_Message_DispInfo.ucMessage_Cnt == ISDBT_EMM_MAX_MESSAGE_CNT)
			EMM_Message_DispInfo.ucMessage_Cnt=0;;	

		EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].uiMessageID = Message_ID;
		EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucBroadcaster_GroupID = EMM_ComMessage[uiMessage_Cnt].EMM_ComMessage_Field.ucBroadcaster_GroupID;
		EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDeletion_Status = 0;
		EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Duration1 = 0;
		EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Duration2 = 0;
		EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Duration3 = 0;
		EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Cycle = 0;
		EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucFormat_Version = EMM_ComMessage[uiMessage_Cnt].EMM_ComMessage_Field.ucFormat_Version;

		if(EMM_ComMessage[uiMessage_Cnt].EMM_ComMessage_Field.uiMessageLen>0)
		{
			EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Horizontal_PositionInfo = 0;
			EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Vertical_PositionInfo = 0;

			ALOGE("%s %d uiMessageLen =%d  \n", __func__, __LINE__, EMM_ComMessage[uiMessage_Cnt].EMM_ComMessage_Field.uiMessageLen);

			for(i=0; i<(unsigned int)(EMM_ComMessage[uiMessage_Cnt].EMM_ComMessage_Field.uiMessageLen-1); i++)
			{
				if(EMM_ComMessage[uiMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i+1] != ISDBT_EMMMESSAGE_DIFFDATA_INSERT_VALUE && EMM_ComMessage[uiMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i+1] != 0)
					Message_data[j] = EMM_ComMessage[uiMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i+1];
				else if(EMM_ComMessage[uiMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i+1] == ISDBT_EMMMESSAGE_DIFFDATA_INSERT_VALUE)
				{
#if 0
					if((EMM_Message_Receive_status.uiMessageType == EMM_AUTODISPLAY_MESSAGE) && (BCAS_Auto_Disp_MessageArch.ucMessageInfoGetStatus == 1))
					{
							memcpy(&Message_data[j], BCAS_Auto_Disp_MessageArch.Differential_information, BCAS_Auto_Disp_MessageArch.Differential_info_length);
							j+= (BCAS_Auto_Disp_MessageArch.Differential_info_length -1);
							ALOGE("%s %d Error \n", __func__, __LINE__);
					}
					else /*if(EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].uiMessageID==EMM_MailMessage_CodeResion.uiFixed_Mmessage_ID )*/
#endif
					{
						ALOGE("EMM_Message_CodeResion.uiExtra_Message_Len  = %d, ucExtra_MessageCode= 0x%x  \n", EMM_MailMessage_CodeResion.uiExtra_Message_Len, EMM_MailMessage_CodeResion.ucExtra_MessageCode[i]);
						if(EMM_MailMessage_CodeResion.uiExtra_Message_Len>0 && EMM_MailMessage_CodeResion.ucExtra_MessageCode != NULL)
						{	
							int k=0;
							ALOGE("EMM ComMessage Differential data insert \n");
							ALOGE("EMM_Message_CodeResion.uiExtra_Message_Len  = %d, j= %d  \n", EMM_MailMessage_CodeResion.uiExtra_Message_Len, j);
				#if 1
							for(k=0;  k<EMM_MailMessage_CodeResion.uiExtra_Message_Len;k+=4)
							{
								ALOGE("[%d][0x%x, 0x%x, 0x%x, 0x%x]\n", __LINE__, EMM_MailMessage_CodeResion.ucExtra_MessageCode[k], EMM_MailMessage_CodeResion.ucExtra_MessageCode[k +1],
									EMM_MailMessage_CodeResion.ucExtra_MessageCode[k +2], EMM_MailMessage_CodeResion.ucExtra_MessageCode[k+3]);
							}
				#endif	
							memcpy(&Message_data[j], EMM_MailMessage_CodeResion.ucExtra_MessageCode, EMM_MailMessage_CodeResion.uiExtra_Message_Len);
							j+= (EMM_MailMessage_CodeResion.uiExtra_Message_Len -1);
						}
					}
					ALOGE("i  = %d, j= %d, EMM_ComMessage.EMM_ComMessage_Field.uiMessageLen = %d  \n", i, j, EMM_ComMessage[uiMessage_Cnt].EMM_ComMessage_Field.uiMessageLen);
				}
				else if(EMM_ComMessage[uiMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i+1] == 0)
					break;
				j++;
			}

			EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].uiMessageLen = j;
			memcpy(EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucMessagePayload, Message_data, EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].uiMessageLen);
			memset(&EMM_MailMessage_CodeResion, 0, sizeof(EMM_MailMessage_CodeResion));
			ret = EMM_Message_Get_Success;
		}
		else
			ret = EMM_Message_Get_Fail;

#if 1
		ALOGE("EMM MESSAGE Parsing \n");
		ALOGE("EMM ComMessage: uiMessageID  = %x \n", EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].uiMessageID );
		ALOGE("EMM ComMessage: ucBroadcaster_GroupID = %x \n", EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucBroadcaster_GroupID );
		ALOGE("EMM ComMessage: ucDeletion_Status = %x \n", EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDeletion_Status );
		ALOGE("EMM ComMessage: ucDisp_Duration1 = %x \n", EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Duration1);
		ALOGE("EMM ComMessage: ucDisp_Duration2 = %x \n", EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Duration2 );
		ALOGE("EMM ComMessage: ucDisp_Duration3 = %x \n", EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Duration3);
		ALOGE("EMM ComMessage: ucDisp_Cycle  = %x \n", EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Cycle );
		ALOGE("EMM ComMessage: ucFormat_Version  = %x \n", EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucFormat_Version );
		ALOGE("EMM ComMessage: ucDisp_Horizontal_PositionInfo  = %x \n", EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Horizontal_PositionInfo);
		ALOGE("EMM ComMessage: ucDisp_Vertical_PositionInfo  = %x \n", EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Vertical_PositionInfo);
		ALOGE("EMM ComMessage: uiMessageLen = %x \n", EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].uiMessageLen);
		ALOGE("EMM ComMessage: ucMessage_Cnt = %x \n", EMM_Message_DispInfo.ucMessage_Cnt );

#if 0
		if(EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.uiMessageLen>0)
		{
			ALOGE("EMM_ComMessage.EMM_ComMessage_Field.ucMessage_Area [uiMessageLen = %d] \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.uiMessageLen);
			for(i=0;  i<(EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.uiMessageLen-7);i+=4)
			{
				ALOGE("EMM ComMessage[0x%x, 0x%x, 0x%x, 0x%x]\n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i], EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i +1],
					EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i +2], EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i+3],
					EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i +4], EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i+5],
					EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i +6], EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i+7]
					);
			}
			ALOGE("\n");
		}

		if(EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].uiMessageLen>0)
		{
			ALOGE("EMM ComMessage Payload [uiMessageLen = %d] \n", EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].uiMessageLen);
			for(i=0;  i<(EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].uiMessageLen-7);i+=4)
			{
				ALOGE("[0x%x, 0x%x, 0x%x, 0x%x]\n", EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucMessagePayload[i], EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucMessagePayload[i +1],
					EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucMessagePayload[i +2], EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucMessagePayload[i+3],
					EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucMessagePayload[i +4], EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucMessagePayload[i+5],
					EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucMessagePayload[i +6], EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucMessagePayload[i+7]
					);
			}
			ALOGE("\n");
		}
#endif	
#endif		
	}
	else if(EMM_Message_Receive_status.uiMessageType == EMM_AUTODISPLAY_MESSAGE)
	{	
	//	ALOGE("EMM_AutoMessage_CodeResion.uiFixed_Mmessage_ID  = %x \n", EMM_AutoMessage_CodeResion.uiFixed_Mmessage_ID );
		if(EMM_ComMessage_Cnt, EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucDeletion_Status != EMM_DSTATUS_ERASE)
		{		
			if(EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.uiTable_Id_Extension != EMM_AutoMessage_CodeResion.uiFixed_Mmessage_ID )
				return EMM_Message_Get_Fail;
			if(EMM_AutoMessage_CodeResion.uiFixed_Mmessage_ID == 0)
				return EMM_Message_Get_Fail;
		
			for(i=0; i<BCAS_MAX_EMM_COM_MESSAGE_CNT;i++)
			{	
				if(EMM_ComMessage[i].EMM_MessSection_Header.uiTable_Id_Extension == EMM_AutoMessage_CodeResion.uiFixed_Mmessage_ID )
				{
					uiTableid_Chk =1;
					uiMessage_Cnt =i;
					break;
				}	
			}

			if(EMM_AutoMessage_CodeResion.uiFixed_Mmessage_ID != EMM_ComMessage[uiMessage_Cnt].EMM_MessSection_Header.uiTable_Id_Extension)
			{
				return EMM_Message_Get_Fail;
			}

			if(!uiTableid_Chk)
				return EMM_Message_Get_Fail;

		}
		else
		{
			uiMessage_Cnt = EMM_ComMessage_Cnt;
		}

		if(EMM_Message_DispInfo.ucMessage_Cnt == ISDBT_EMM_MAX_MESSAGE_CNT)
			EMM_Message_DispInfo.ucMessage_Cnt=0;;	

		EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].uiMessageID = EMM_ComMessage[uiMessage_Cnt].EMM_MessSection_Header.uiTable_Id_Extension;
		EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucBroadcaster_GroupID = EMM_ComMessage[uiMessage_Cnt].EMM_ComMessage_Field.ucBroadcaster_GroupID;
		EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDeletion_Status = EMM_ComMessage[uiMessage_Cnt].EMM_ComMessage_Field.ucDeletion_Status;
		EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Duration1 = EMM_ComMessage[uiMessage_Cnt].EMM_ComMessage_Field.ucDisp_Duration1;
		EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Duration2 = EMM_ComMessage[uiMessage_Cnt].EMM_ComMessage_Field.ucDisp_Duration2;
		EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Duration3 = EMM_ComMessage[uiMessage_Cnt].EMM_ComMessage_Field.ucDisp_Duration3;
		EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Cycle = EMM_ComMessage[uiMessage_Cnt].EMM_ComMessage_Field.ucDisp_Cycle;
		EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucFormat_Version = EMM_ComMessage[uiMessage_Cnt].EMM_ComMessage_Field.ucFormat_Version;

		if(EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDeletion_Status == EMM_DSTATUS_ERASE)
		{
			ret = EMM_Message_Get_Success;
		}
		else
		{	
			if(EMM_ComMessage[uiMessage_Cnt].EMM_ComMessage_Field.uiMessageLen>0)
			{
				EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Horizontal_PositionInfo = EMM_ComMessage[uiMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[0]>>4;
				EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Vertical_PositionInfo = (0x0f&EMM_ComMessage[uiMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[0]);

				for(i=0; i<(unsigned int)(EMM_ComMessage[uiMessage_Cnt].EMM_ComMessage_Field.uiMessageLen-1); i++)
				{
					if(EMM_ComMessage[uiMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i+1] != ISDBT_EMMMESSAGE_DIFFDATA_INSERT_VALUE && EMM_ComMessage[uiMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i+1] != '\0')
						Message_data[j] = EMM_ComMessage[uiMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i+1];
					else if(EMM_ComMessage[uiMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i+1] == ISDBT_EMMMESSAGE_DIFFDATA_INSERT_VALUE)
					{
						if((EMM_Message_Receive_status.uiMessageType == EMM_AUTODISPLAY_MESSAGE) && (BCAS_Auto_Disp_MessageArch.ucMessageInfoGetStatus == 1))
						{
								memcpy(&Message_data[j], BCAS_Auto_Disp_MessageArch.Differential_information, BCAS_Auto_Disp_MessageArch.Differential_info_length);
								j+= (BCAS_Auto_Disp_MessageArch.Differential_info_length -1);
								ALOGE("%s %d Error \n", __func__, __LINE__);
						}
						else if(EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].uiMessageID==EMM_AutoMessage_CodeResion.uiFixed_Mmessage_ID )
						{
							ALOGE("EMM_Message_CodeResion.uiExtra_Message_Len  = %d, ucExtra_MessageCode= 0x%x  \n", EMM_AutoMessage_CodeResion.uiExtra_Message_Len, EMM_AutoMessage_CodeResion.ucExtra_MessageCode[i]);
							if(EMM_AutoMessage_CodeResion.uiExtra_Message_Len>0 && EMM_AutoMessage_CodeResion.ucExtra_MessageCode != NULL)
							{	
								int k=0;
								ALOGE("EMM ComMessage Differential data insert \n");
								ALOGE("EMM_Message_CodeResion.uiExtra_Message_Len  = %d, j= %d  \n", EMM_AutoMessage_CodeResion.uiExtra_Message_Len, j);
						#if 0
								for(k=0;  k<EMM_AutoMessage_CodeResion.uiExtra_Message_Len;k+=4)
								{
									ALOGE("[0x%x, 0x%x, 0x%x, 0x%x]\n", EMM_AutoMessage_CodeResion.ucExtra_MessageCode[k], EMM_AutoMessage_CodeResion.ucExtra_MessageCode[k +1],
										EMM_AutoMessage_CodeResion.ucExtra_MessageCode[k +2], EMM_AutoMessage_CodeResion.ucExtra_MessageCode[k+3]);
								}
						#endif
								memcpy(&Message_data[j], EMM_AutoMessage_CodeResion.ucExtra_MessageCode, EMM_AutoMessage_CodeResion.uiExtra_Message_Len);
								j+= (EMM_AutoMessage_CodeResion.uiExtra_Message_Len -1);
							}
						}
						ALOGE("i  = %d, j= %d, EMM_ComMessage.EMM_ComMessage_Field.uiMessageLen = %d  \n", i, j, EMM_ComMessage[uiMessage_Cnt].EMM_ComMessage_Field.uiMessageLen);
					}
					else if(EMM_ComMessage[uiMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i+1] == 0)
						break;
					j++;
				}

				EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].uiMessageLen = j;
				memcpy(EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucMessagePayload, Message_data, EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].uiMessageLen);
				memset(&EMM_AutoMessage_CodeResion, 0, sizeof(EMM_AutoMessage_CodeResion));
				ret = EMM_Message_Get_Success;
			}
			else
			{	
				if(AutoDisp_Message_GetStatus)
					ret = EMM_Message_Get_Success;
				else
					ret = EMM_Message_Get_Fail;
			}	
		}	

#if 1
		ALOGE("EMM MESSAGE Parsing \n");
		ALOGE("EMM ComMessage: uiMessageID  = %x \n", EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].uiMessageID );
		ALOGE("EMM ComMessage: ucBroadcaster_GroupID = %x \n", EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucBroadcaster_GroupID );
		ALOGE("EMM ComMessage: ucDeletion_Status = %x \n", EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDeletion_Status );
		ALOGE("EMM ComMessage: ucDisp_Duration1 = %x \n", EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Duration1);
		ALOGE("EMM ComMessage: ucDisp_Duration2 = %x \n", EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Duration2 );
		ALOGE("EMM ComMessage: ucDisp_Duration3 = %x \n", EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Duration3);
		ALOGE("EMM ComMessage: ucDisp_Cycle  = %x \n", EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Cycle );
		ALOGE("EMM ComMessage: ucFormat_Version  = %x \n", EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucFormat_Version );
		ALOGE("EMM ComMessage: ucDisp_Horizontal_PositionInfo  = %x \n", EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Horizontal_PositionInfo);
		ALOGE("EMM ComMessage: ucDisp_Vertical_PositionInfo  = %x \n", EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucDisp_Vertical_PositionInfo);
		ALOGE("EMM ComMessage: uiMessageLen = %x \n", EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].uiMessageLen);
		ALOGE("EMM ComMessage: ucMessage_Cnt = %x \n", EMM_Message_DispInfo.ucMessage_Cnt );

#if 0
		if(EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.uiMessageLen>0)
		{
			ALOGE("EMM_ComMessage.EMM_ComMessage_Field.ucMessage_Area [uiMessageLen = %d] \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.uiMessageLen);
			for(i=0;  i<(EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.uiMessageLen-7);i+=4)
			{
				ALOGE("EMM ComMessage[0x%x, 0x%x, 0x%x, 0x%x]\n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i], EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i +1],
					EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i +2], EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i+3],
					EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i +4], EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i+5],
					EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i +6], EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i+7]
					);
			}
			ALOGE("\n");
		}

		if(EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].uiMessageLen>0)
		{
			ALOGE("EMM ComMessage Payload [uiMessageLen = %d] \n", EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].uiMessageLen);
			for(i=0;  i<(EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].uiMessageLen-7);i+=4)
			{
				ALOGE("[0x%x, 0x%x, 0x%x, 0x%x]\n", EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucMessagePayload[i], EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucMessagePayload[i +1],
					EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucMessagePayload[i +2], EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucMessagePayload[i+3],
					EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucMessagePayload[i +4], EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucMessagePayload[i+5],
					EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucMessagePayload[i +6], EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt].ucMessagePayload[i+7]
					);
			}
			ALOGE("\n");
		}
#endif	
#endif		
	}

	if(ret == EMM_Message_Get_Success)
	{	
		if(EMM_Message_DispInfo.ucMessage_Cnt<ISDBT_EMM_MAX_MESSAGE_CNT)
			EMM_Message_DispInfo.ucMessage_Cnt ++;	
	}	

	ALOGE("EMM MESSAGE Parsing End \n");

	return ret;
}

int TCC_Dxb_SC_Manager_EMM_MessageInfoGet(EMM_Message_Info_t *DispMessage)
{
	if(EMM_Message_DispInfo.ucMessage_Cnt >0)
	{
		ALOGE("%s %d ucMessage_Cnt = %d \n", __func__, __LINE__,EMM_Message_DispInfo.ucMessage_Cnt);
		memcpy(DispMessage, &EMM_Message_DispInfo.EMM_MessageInfo[EMM_Message_DispInfo.ucMessage_Cnt-1], sizeof(EMM_Message_Info_t));
		return 1;	
	}
	else
		return 0;
}

/************************************************************************************************************
* FUNCTION NAME : TCC_Dxb_SC_Manager_EMM_MessageCardIDCheck
* DESCRIPTION :  Card Id Check
* INPUT	 : 
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
int TCC_Dxb_SC_Manager_EMM_MessageCardIDCheck(EMM_Individual_Message_Field_t *IndiMessage )
{
	unsigned int	ret =0;
	unsigned int	i=0;

	for(i=0; i<BCAS_CardID_Arch.Number_of_CardIDs; i++)
	{	
		if(!memcmp(&BCAS_CardID_Arch.Display_Card_ID[i].Card_Id.ID[0], &IndiMessage->ucCard_ID[0], BCAS_CARDID_LEGTH))
			ret =1;
	}

	return ret;
}

/************************************************************************************************************
* FUNCTION NAME : TCC_Dxb_SC_Manager_EMM_MessageVersionCheck
* DESCRIPTION :  EMM version check
* INPUT	 : 
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
int TCC_Dxb_SC_Manager_EMM_MessageVersionCheck(unsigned short uiTable_Id_Extension, unsigned char ucVersionNum)
{
	unsigned int	ret =0;
	unsigned int	i=0;

	if(uiTable_Id_Extension == ISDBT_EMM_INDIVIDUAL_MESSAGE_TABLEID_EXTENSION)
	{

#if 0
		for(i=0; i<EMM_IndiMessage_Cnt; i++)
		{
			if(EMM_IndiMessage[i].EMM_MessSection_Header.uiTable_Id_Extension == uiTable_Id_Extension &&
				EMM_IndiMessage[i].EMM_MessSection_Header.ucVersionNum == ucVersionNum)
			{
				ret = 1;
				break;
			}
		}
#endif
		;
	}
	else
	{
		for(i=0; i<EMM_ComMessage_Cnt; i++)
		{
			if(EMM_ComMessage[i].EMM_MessSection_Header.uiTable_Id_Extension == uiTable_Id_Extension &&
				EMM_ComMessage[i].EMM_MessSection_Header.ucVersionNum == ucVersionNum)
			{
				ret = 1;
				break;
			}
		}
	}
	
	return ret;
}


/************************************************************************************************************
* FUNCTION NAME : TCC_Dxb_SC_Manager_Open
* DESCRIPTION :  Smart Card Open
* INPUT	 : 
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
int TCC_Dxb_SC_Manager_Open(void)
{
	int err= SC_ERR_OK;
	DEBUG(DEFAULT_MESSAGES, "%s %d \n", __func__, __LINE__);

	pthread_mutex_init(&tcc_sc_mutex, NULL);
	
	err = TCC_DxB_SC_Open();

	if(err)
	{
		DEBUG(DEFAULT_MESSAGES,"%s %d open Error\n", __func__, __LINE__);
		return SC_ERR_OPEN;
	}
	TCC_Dxb_SC_Manager_InitParam();

	SC_CLOSE_FLAG = FALSE;
	memset(&EMM_ComMessage, 0, sizeof(EMM_ComMessage));
	EMM_ComMessage_Cnt =0;
	EMM_IndiMessage_Cnt =0;
	
	return err;
}

/************************************************************************************************************
* FUNCTION NAME : TCC_Dxb_SC_Manager_Close
* DESCRIPTION :  Smart Card Close
* INPUT	 : 
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
int TCC_Dxb_SC_Manager_Close(void)
{
	int err = SC_ERR_OK;
	DEBUG(DEFAULT_MESSAGES, "%s %d \n", __func__, __LINE__);

	pthread_mutex_destroy(&tcc_sc_mutex);

	SC_CLOSE_FLAG = TRUE;

#if 1
	// Noah, To avoid CodeSonar warning, Redundant Condition
	// TCC_DxB_SC_Close in tcc_dxb_interface_sc.c returns always DxB_ERR_OK.

	TCC_DxB_SC_Close();
#else
	err= TCC_DxB_SC_Close();
	if(err)
	{
		DEBUG(DEFAULT_MESSAGES, "%s %d Close Error\n", __func__, __LINE__);
		return SC_ERR_CLOSE;
	}
#endif

	TCC_Dxb_SC_Manager_InitParam();

	TCC_Dxb_SC_Manager_EMM_ComMessageDeInit();
	TCC_Dxb_SC_Manager_EMM_IndiMessageDeInit(0);
#ifdef EMM_MALLOC_DEBUGING		//EMM malloc size debuging
	ALOGE("%s %d malloc_size = %ld , free_size = %ld \n", __func__, __LINE__, malloc_size, free_size);
#endif

	return err;
}

/************************************************************************************************************
* FUNCTION NAME : TCC_Dxb_SC_Manager_InitParam
* DESCRIPTION :  init parameter
* INPUT	 : 
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
int TCC_Dxb_SC_Manager_GetATR(unsigned int dev_id, unsigned char*ucAtrBuf,  unsigned int *uiAtrLength)
{
	int err =SC_ERR_OK;
	DEBUG(DEFAULT_MESSAGES, "%s %d \n", __func__, __LINE__);

	err = TCC_DxB_SC_Reset(dev_id, ucAtrBuf, uiAtrLength );
	if(err)
	{
		DEBUG(DEFAULT_MESSAGES, "%s %d err = %x \n", __func__, __LINE__, err);
		return SC_ERR_GET_ATR;
	}
	err = TCC_Dxb_SC_Manager_Parse_Atr(ucAtrBuf, uiAtrLength, &stATRChar);
	return err;
}

/************************************************************************************************************
* FUNCTION NAME : TCC_Dxb_SC_Manager_Parse_Atr
* DESCRIPTION :  ATR Parisngr
* INPUT	 : 
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
int TCC_Dxb_SC_Manager_Parse_Atr(unsigned char *pATRData, unsigned int*uATRLength, sTCC_SC_ATR_CHARS *pstATRParse)
{
	int i;
	int iProtocol = 0;
	unsigned char uCount = 0;
	unsigned char uHCharNum = 0;

	DEBUG(DEFAULT_MESSAGES, "%s %d \n", __func__, __LINE__);
	
	memset(pstATRParse, 0xFF, sizeof(sTCC_SC_ATR_CHARS));

	pstATRParse->ucTS = *(pATRData + uCount++);
	pstATRParse->ucT0 = *(pATRData + uCount++);

	uHCharNum = pstATRParse->ucT0 & 0x0F;

	//Get TA1, TB1, TC1, TD1 Characters
	if(pstATRParse->ucT0 & TA_CHECK_BIT)
		pstATRParse->usTA[0] = *(pATRData + uCount++);
	if(pstATRParse->ucT0 & TB_CHECK_BIT)
		pstATRParse->usTB[0] = *(pATRData + uCount++);
	if(pstATRParse->ucT0 & TC_CHECK_BIT)
		pstATRParse->usTC[0] = *(pATRData + uCount++);
	if(pstATRParse->ucT0 & TD_CHECK_BIT)
		pstATRParse->usTD[0] = *(pATRData + uCount++);

	//Get TA2, TB2, TC2, TD2 Characters
	if(pstATRParse->usTD[0] != 0xFFFF)
	{
		if(pstATRParse->usTD[0] & TA_CHECK_BIT)
			pstATRParse->usTA[1] = *(pATRData + uCount++);
		if(pstATRParse->usTD[0] & TB_CHECK_BIT)
			pstATRParse->usTB[1] = *(pATRData + uCount++);
		if(pstATRParse->usTD[0] & TC_CHECK_BIT)
			pstATRParse->usTC[1] = *(pATRData + uCount++);
		if(pstATRParse->usTD[0] & TD_CHECK_BIT)
			pstATRParse->usTD[1] = *(pATRData + uCount++);

		iProtocol = pstATRParse->usTD[0] & 0x0F;
	}
		
	//Get TA3, TB3, TC3, TD3 Characters
	if(pstATRParse->usTD[1] != 0xFFFF)
	{
		if(pstATRParse->usTD[1] & TA_CHECK_BIT)
			pstATRParse->usTA[2] = *(pATRData + uCount++);
		if(pstATRParse->usTD[1] & TB_CHECK_BIT)
			pstATRParse->usTB[2] = *(pATRData + uCount++);
		if(pstATRParse->usTD[1] & TC_CHECK_BIT)
			pstATRParse->usTC[2] = *(pATRData + uCount++);
		if(pstATRParse->usTD[1] & TD_CHECK_BIT)
			pstATRParse->usTD[2] = *(pATRData + uCount++);

		iProtocol = pstATRParse->usTD[1] & 0x0F;
		MAX_DATA_FIELD_LENGTH = pstATRParse->usTA[2];
	}
		
	//Get TA4, TB4, TC4, TD4 Characters
	if(pstATRParse->usTD[2] != 0xFFFF)
	{
		if(pstATRParse->usTD[2] & TA_CHECK_BIT)
			pstATRParse->usTA[3] = *(pATRData + uCount++);
		if(pstATRParse->usTD[2] & TB_CHECK_BIT)
			pstATRParse->usTB[3] = *(pATRData + uCount++);
		if(pstATRParse->usTD[2] & TC_CHECK_BIT)
			pstATRParse->usTC[3] = *(pATRData + uCount++);
		if(pstATRParse->usTD[2] & TD_CHECK_BIT)
			pstATRParse->usTD[3] = *(pATRData + uCount++);
	}

	//Get Historical Characters
	for(i=0; i<uHCharNum; i++)
	{
		pstATRParse->ucHC[i] = *(pATRData + uCount++);
	}

	//Get TCK Characters
	if(iProtocol != 0)
	{
		pstATRParse->ucTCK = *(pATRData + uCount++);
	}

	#if 1 // Test Log
	ALOGE("TS:0x%02x\n", pstATRParse->ucTS);
	ALOGE("TO:0x%02x\n", pstATRParse->ucT0);
	ALOGE("TA1:0x%04x, TB1:0x%04x, TC1:0x%04x, TD1:0x%04x\n", pstATRParse->usTA[0], pstATRParse->usTB[0], pstATRParse->usTC[0], pstATRParse->usTD[0]);
	ALOGE("TA2:0x%04x, TB2:0x%04x, TC2:0x%04x, TD2:0x%04x\n", pstATRParse->usTA[1], pstATRParse->usTB[1], pstATRParse->usTC[1], pstATRParse->usTD[1]);
	ALOGE("TA3:0x%04x, TB3:0x%04x, TC3:0x%04x, TD3:0x%04x\n", pstATRParse->usTA[2], pstATRParse->usTB[2], pstATRParse->usTC[2], pstATRParse->usTD[2]);
	ALOGE("TA4:0x%04x, TB4:0x%04x, TC4:0x%04x, TD4:0x%04x\n", pstATRParse->usTA[3], pstATRParse->usTB[3], pstATRParse->usTC[3], pstATRParse->usTD[3]);
	ALOGE("Historical Charaters : %s\n", pstATRParse->ucHC);
	ALOGE("TCK:0x%02x\n", pstATRParse->ucTCK);
	ALOGE("ATRLenth=%d, uCount=%d\n", *uATRLength, uCount);
	ALOGE("MAX_DATA_FIELD_LENGTH=%d\n", MAX_DATA_FIELD_LENGTH);
	#endif

	DEBUG(DEFAULT_MESSAGES, "MAX_DATA_FIELD_LENGTH=%d\n", MAX_DATA_FIELD_LENGTH);

	//Check the ATR length
	if((*uATRLength != uCount)  || MAX_DATA_FIELD_LENGTH==0)
		return SC_ERR_GET_ATR;
		
	SC_PROTOCOL_TYPE = iProtocol;
	
	return SC_ERR_OK;
}

/************************************************************************************************************
* FUNCTION NAME : TCC_Dxb_SC_Manager_Get_CASystemID
* DESCRIPTION :  return CA System Id 
* INPUT	 : 
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
int TCC_Dxb_SC_Manager_GetCASystemID(void)
{
	return BCAS_InitSet_ARCH.CA_System_ID;
}


/************************************************************************************************************
* FUNCTION NAME : TCC_Dxb_SC_Manager_Cmd_Header_Set
* DESCRIPTION :  Command APDU default setting (BCAS fix at "cla=0x90, p1=0x00, p2=0x00")
* INPUT	 : 
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
void TCC_Dxb_SC_Manager_CmdHeaderSet(char cla, char p1, char p2)
{
	SC_CMD_Header_ARCH.CLA = cla;
	SC_CMD_Header_ARCH.P1 = p1;
	SC_CMD_Header_ARCH.P2 = p2;
}

/************************************************************************************************************
* FUNCTION NAME : TCC_Dxb_SC_Manager_GenEDC
* DESCRIPTION :  Generate EDC (BCAS Fefault use LRC, CRC is not ready yet)
* INPUT	 : 
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
int TCC_Dxb_SC_Manager_GenEDC(char edc, char data)
{
	if(EDC_TYPE == EDC_TYPE_LRC)
	{
		return (edc^data);
	}
	else if(EDC_TYPE == EDC_TYPE_CRC)
	{
		/* Not ready Yet*/
		return -1;
	}
	return 0;
}

/*
  * TCC_Dxb_SC_Manager_CalcEDC
  *
  *	This function is only useful for LRC, not yet ready for CRC.
  */
int TCC_Dxb_SC_Manager_CalcEDC (unsigned char *buf, int len, unsigned char *edc)
{
	int	i;
	unsigned char	lrc;
	if (EDC_TYPE == EDC_TYPE_LRC)
	{
		lrc = 0;
		for (i=0; i < len; i++)
		{
			lrc ^= *buf++;
		}
		*edc = lrc;
	}
	else
	{
		return -1;
	}
	return 0;
}

/************************************************************************************************************
* FUNCTION NAME : TCC_Dxb_SC_Manager_DefSetEDC
* DESCRIPTION :  Default  EDC setting (LRC is Null, CRC is 0xffff)
* INPUT	 : 
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
int TCC_Dxb_SC_Manager_DefSetEDC(int type)
{
	if(type == EDC_TYPE_LRC)
	{
		EDC_TYPE = EDC_TYPE_LRC;
	}
	else if(type ==  EDC_TYPE_CRC)
	{
		EDC_TYPE = EDC_TYPE_CRC;
	}
	return 0;
}

/************************************************************************************************************
* FUNCTION NAME : TCC_Dxb_SC_Manager_ProcessResINT
* DESCRIPTION :  BCas Initial setting conditions response processing.
* INPUT	 :  Response APDU. (only APDU Data)	
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
int TCC_Dxb_SC_Manager_ProcessResINT(unsigned char *data)
{
	int	err = SC_ERR_OK;

	DEBUG(DEFAULT_MESSAGES, "%s %d \n", __func__, __LINE__);
	
	BCAS_InitSet_ARCH.Return_Code = (data[RES_DATAFIELD_RETURN_CODE]<<8|data[RES_DATAFIELD_RETURN_CODE +1]);
	if(BCAS_InitSet_ARCH.Return_Code != BCASC_INT_RETURN_CODE_NORMAL)
	{
		DEBUG(DEFAULT_MESSAGES,"%s %d Initial Response Return_Code = %x \n", __func__, __LINE__, BCAS_InitSet_ARCH.Return_Code );
		return SC_ERR_RETURN_CODE ;
	}

	BCAS_InitSet_ARCH.Protocol_Unit_Num = data[RES_DATAFIELD_PRO_UNITNUM];
	BCAS_InitSet_ARCH.Unit_Len = data[RES_DATAFIELD_UNIT_LEN];
	BCAS_InitSet_ARCH.IC_Card_Instruction = (data[RES_DATAFIELD_CARD_INS]<<8|data[RES_DATAFIELD_CARD_INS+1]);
	BCAS_InitSet_ARCH.CA_System_ID = (data[INIT_SET_CA_SYSTEM_ID_POS]<<8|data[INIT_SET_CA_SYSTEM_ID_POS+1]);
	memcpy(BCAS_InitSet_ARCH.Card_ID, &data[INIT_SET_CARD_ID_POS], BCAS_CARDID_LEGTH);
	BCAS_InitSet_ARCH.Card_Type = data[INIT_SET_CARD_TYPE_POS];
	BCAS_InitSet_ARCH.Message_Partition_len = data[INIT_SET_MESS_PARTI_LNG_POS];
	memcpy(BCAS_InitSet_ARCH.Desc_System_Key, &data[INIT_SET_SYSTEM_KEY_POS], BCAS_SYSTEM_KEY_LEN);
	memcpy(BCAS_InitSet_ARCH.Desc_CBC_InitValue, &data[INIT_SET_CBC_INITVAL_POS], BCAS_CBC_INIT_LEN);
	BCAS_InitSet_ARCH.SW1_SW2 = (data[BCAS_InitSet_ARCH.Unit_Len+2]<<8|data[BCAS_InitSet_ARCH.Unit_Len+3]);

	if(TCC_SC_DESCIRPTION_MODE == MULTI2_MODE_HW)
	{
		TCC_DxB_CAS_KeySwap(BCAS_InitSet_ARCH.Desc_CBC_InitValue, BCAS_CBC_INIT_LEN);
		TCC_DxB_CAS_KeySwap(BCAS_InitSet_ARCH.Desc_System_Key, BCAS_SYSTEM_KEY_LEN);
	}
#if 1
	ALOGE("%s %d BCAS_InitSet_ARCH.Protocol_Unit_Num = %x \n", __func__, __LINE__, BCAS_InitSet_ARCH.Protocol_Unit_Num);
	ALOGE("%s %d BCAS_InitSet_ARCH.Unit_Len = %d \n", __func__, __LINE__, BCAS_InitSet_ARCH.Unit_Len);
	ALOGE("%s %d BCAS_InitSet_ARCH.IC_Card_Instruction = %x \n", __func__, __LINE__, BCAS_InitSet_ARCH.IC_Card_Instruction);
	ALOGE("%s %d BCAS_InitSet_ARCH.CA_System_ID = %x \n", __func__, __LINE__, BCAS_InitSet_ARCH.CA_System_ID);
#if 0
	{
		int i;
		for(i=0; i<BCAS_CARD_ID_LEN; i++)
			ALOGE("BCAS_InitSet_ARCH.Card_ID[%d] = %x \n", i, BCAS_InitSet_ARCH.Card_ID[i]);
	}
#endif
	ALOGE("%s %d BCAS_InitSet_ARCH.Card_Type = %x \n", __func__, __LINE__, BCAS_InitSet_ARCH.Card_Type);
	ALOGE("%s %d BCAS_InitSet_ARCH.Message_Partition_len = %d \n", __func__, __LINE__, BCAS_InitSet_ARCH.Message_Partition_len);
#if 0
	{
		int i;
		for(i=0; i<BCAS_SYSTEM_KEY_LEN; i++)
			ALOGE("BCAS_InitSet_ARCH.Desc_System_Key[%d] = %x \n", i, BCAS_InitSet_ARCH.Desc_System_Key[i]);
	}
	{
		int i;
		for(i=0; i<BCAS_CBC_INIT_LEN; i++)
			ALOGE("BCAS_InitSet_ARCH.Desc_CBC_InitValue[%d] = %x \n", i, BCAS_InitSet_ARCH.Desc_CBC_InitValue[i]);
	}
#endif
	ALOGE("%s %d BCAS_InitSet_ARCH.SW1_SW2 = %x \n", __func__, __LINE__, BCAS_InitSet_ARCH.SW1_SW2);
#endif

	if(BCAS_InitSet_ARCH.SW1_SW2 != SC_SW1SW2_CMD_NORMAL)
		return SC_ERR_RETURN_CODE;
	
	return err;
	
}

/************************************************************************************************************
* FUNCTION NAME : TCC_Dxb_SC_Manager_ProcessResECM
* DESCRIPTION :  BCas ECM response processing.
* INPUT	 :  Response APDU. (only APDU Data)	
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
int TCC_Dxb_SC_Manager_ProcessResECM(unsigned char *data)
{
	int	err = SC_ERR_OK;

	DEBUG(DEFAULT_MESSAGES, "%s %d \n", __func__, __LINE__);
	
	BCAS_Ecm_ARCH.Return_Code = (data[RES_DATAFIELD_RETURN_CODE]<<8|data[RES_DATAFIELD_RETURN_CODE +1]);
	if(BCAS_Ecm_ARCH.Return_Code != BCASC_ECM_RETURN_CODE_PURCHASED_TIER)
	{
		DEBUG(DEFAULT_MESSAGES,"%s %d ECM Response Return_Code = %x \n", __func__, __LINE__, BCAS_Ecm_ARCH.Return_Code);
		ALOGE("%s %d ECM Response Return_Code = %x \n", __func__, __LINE__, BCAS_Ecm_ARCH.Return_Code);
		return SC_ERR_RETURN_CODE;
	}

	BCAS_Ecm_ARCH.Protocol_Unit_Num = data[RES_DATAFIELD_PRO_UNITNUM];
	BCAS_Ecm_ARCH.Unit_Len = data[RES_DATAFIELD_UNIT_LEN];
	BCAS_Ecm_ARCH.IC_Card_Instruction = (data[RES_DATAFIELD_CARD_INS]<<8|data[RES_DATAFIELD_CARD_INS+1]);
	memcpy(BCAS_Ecm_ARCH.Ks_Odd, &data[ECM_KS_ODD_POS], BCAS_ECM_KS_LEN);
	memcpy(BCAS_Ecm_ARCH.Ks_Even, &data[ECM_KS_EVEN_POS], BCAS_ECM_KS_LEN);
	BCAS_Ecm_ARCH.Recording_Control = data[ECM_RECORDING_CON_POS];
	BCAS_Ecm_ARCH.SW1_SW2 = (data[BCAS_Ecm_ARCH.Unit_Len+2]<<8|data[BCAS_Ecm_ARCH.Unit_Len+3]);

	if(TCC_SC_DESCIRPTION_MODE == MULTI2_MODE_HW)
	{
		TCC_DxB_CAS_KeySwap(BCAS_Ecm_ARCH.Ks_Even, BCAS_ECM_KS_LEN);
		TCC_DxB_CAS_KeySwap(BCAS_Ecm_ARCH.Ks_Odd, BCAS_ECM_KS_LEN);
	}

#if 0
	ALOGE("%s %d BCAS_Ecm_ARCH.Protocol_Unit_Num = %x \n", __func__, __LINE__, BCAS_Ecm_ARCH.Protocol_Unit_Num);
	ALOGE("%s %d BCAS_Ecm_ARCH.Unit_Len = %d \n", __func__, __LINE__, BCAS_Ecm_ARCH.Unit_Len);
	ALOGE("%s %d BCAS_Ecm_ARCH.IC_Card_Instruction = %x \n", __func__, __LINE__, BCAS_Ecm_ARCH.IC_Card_Instruction);
	ALOGE("%s %d BCAS_Ecm_ARCH.Return_Code = %x \n", __func__, __LINE__, BCAS_Ecm_ARCH.Return_Code);

	{
		int i;
		for(i=0; i<BCAS_ECM_KS_LEN; i++)
		{
			ALOGE("BCAS_Ecm_ARCH.Ks_Odd[%d] = %x \n", i, BCAS_Ecm_ARCH.Ks_Odd[i]);
		}
		for(i=0; i<BCAS_ECM_KS_LEN; i++)
		{
			ALOGE("BCAS_Ecm_ARCH.Ks_Even[%d] = %x \n", i, BCAS_Ecm_ARCH.Ks_Even[i]);
		}
	}

	ALOGE("%s %d BCAS_Ecm_ARCH.SW1_SW2 = %x \n", __func__, __LINE__, BCAS_Ecm_ARCH.SW1_SW2);
#endif
	return err;
	
}

/************************************************************************************************************
* FUNCTION NAME : TCC_Dxb_SC_Manager_ProcessResEMM
* DESCRIPTION :  BCas EMM response processing.
* INPUT	 :  Response APDU. (only APDU Data)	
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
int TCC_Dxb_SC_Manager_ProcessResEMM(unsigned char *data)
{
	int	err = SC_ERR_OK;

	DEBUG(DEFAULT_MESSAGES, "%s %d \n", __func__, __LINE__);
	
	BCAS_Emm_Arch.Return_Code = (data[RES_DATAFIELD_RETURN_CODE]<<8|data[RES_DATAFIELD_RETURN_CODE +1]);
	if(BCAS_Emm_Arch.Return_Code != BCASC_EMM_RETURN_CODE_NORMAL)
	{
		DEBUG(DEFAULT_MESSAGES,"%s %d EMM Response Return_Code = %x \n", __func__, __LINE__, BCAS_Emm_Arch.Return_Code);
		ALOGE("%s %d EMM Response Return_Code = %x \n", __func__, __LINE__, BCAS_Emm_Arch.Return_Code);
		return SC_ERR_RETURN_CODE;
	}

	BCAS_Emm_Arch.Protocol_Unit_Num = data[RES_DATAFIELD_PRO_UNITNUM];
	BCAS_Emm_Arch.Unit_Len = data[RES_DATAFIELD_UNIT_LEN];
	BCAS_Emm_Arch.IC_Card_Instruction = (data[RES_DATAFIELD_CARD_INS]<<8|data[RES_DATAFIELD_CARD_INS+1]);
	BCAS_Emm_Arch.SW1_SW2 = (data[BCAS_Emm_Arch.Unit_Len+2]<<8|data[BCAS_Emm_Arch.Unit_Len+3]);

#if 0
	ALOGE("%s %d BCAS_Emm_Arch.Protocol_Unit_Num = %x \n", __func__, __LINE__, BCAS_Emm_Arch.Protocol_Unit_Num);
	ALOGE("%s %d BCAS_Emm_Arch.Unit_Len = %d \n", __func__, __LINE__, BCAS_Emm_Arch.Unit_Len);
	ALOGE("%s %d BCAS_Emm_Arch.IC_Card_Instruction = %x \n", __func__, __LINE__, BCAS_Emm_Arch.IC_Card_Instruction);
	ALOGE("%s %d BCAS_Emm_Arch.Return_Code = %x \n", __func__, __LINE__, BCAS_Emm_Arch.Return_Code);
	ALOGE("%s %d BCAS_Emm_Arch.SW1_SW2 = %x \n", __func__, __LINE__, BCAS_Emm_Arch.SW1_SW2);

	if(BCAS_Emm_Arch.Unit_Len>4)
	{
		int i;
		for(i=0; i<BCAS_Emm_Arch.Unit_Len; i++)
		{
			ALOGE("BCAS_Emm_Arch_Data[%d] = %x \n", i, data[RES_DATAFIELD_CARD_INS + i]);
		}
	}

#endif
	return err;
	
}

/************************************************************************************************************
* FUNCTION NAME : TCC_Dxb_SC_Manager_ProcessResEMG
* DESCRIPTION :  BCas receive EMM Individual Message  processing.
* INPUT	 :  Response APDU. (only APDU Data)	
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
int TCC_Dxb_SC_Manager_ProcessResEMG(unsigned char *data)
{
	int	err = SC_ERR_OK;
	unsigned int	sw1_sw2_offset =0;

	DEBUG(DEFAULT_MESSAGES, "%s %d \n", __func__, __LINE__);

 	BCAS_EMM_Indi_MessageArch.Return_Code = (data[RES_DATAFIELD_RETURN_CODE]<<8|data[RES_DATAFIELD_RETURN_CODE +1]);
	if(BCAS_EMM_Indi_MessageArch.Return_Code != BCASC_EMM_RETURN_CODE_NORMAL)
	{
		DEBUG(DEFAULT_MESSAGES,"%s %d EMM Response Return_Code = %x \n", __func__, __LINE__, BCAS_EMM_Indi_MessageArch.Return_Code);
		ALOGE("%s %d EMM Response Return_Code = %x \n", __func__, __LINE__, BCAS_EMM_Indi_MessageArch.Return_Code);
		return SC_ERR_RETURN_CODE;
	}

	BCAS_EMM_Indi_MessageArch.Protocol_Unit_Num = data[RES_DATAFIELD_PRO_UNITNUM];
	BCAS_EMM_Indi_MessageArch.Unit_Len = data[RES_DATAFIELD_UNIT_LEN];
	BCAS_EMM_Indi_MessageArch.IC_Card_Instruction = (data[RES_DATAFIELD_CARD_INS]<<8|data[RES_DATAFIELD_CARD_INS+1]);

//	ALOGE("%s %d, [Unit_Len=%d][uiTotal_Cnt =%d]\n", __func__, __LINE__, BCAS_EMM_Indi_MessageArch.Unit_Len, EMG_ResCMD_MessageCode_LengthCheck.uiTotal_Cnt);
//	ALOGE("%s %d, [uiMessageType =%d]\n", __func__, __LINE__, EMM_Message_Receive_status.uiMessageType);

	if(BCAS_EMM_Indi_MessageArch.Unit_Len <4)
	{
		ALOGE("%s %d, EMM Message Code None [Unit_Len=%d]\n", __func__, __LINE__, BCAS_EMM_Indi_MessageArch.Unit_Len);
		return SC_ERR_MASSAGECODE_NONE;
	}
	
	memcpy(BCAS_EMM_Indi_MessageArch.Response_message_code, &data[RES_DATAFIELD_RETURN_CODE +2], (BCAS_EMM_Indi_MessageArch.Unit_Len-4));

	if(EMG_ResCMD_MessageCode_LengthCheck.uiTotal_Cnt == 1)
	{
		sw1_sw2_offset = RES_DATAFIELD_RETURN_CODE + (BCAS_EMM_Indi_MessageArch.Unit_Len-4);
		BCAS_EMM_Indi_MessageArch.SW1_SW2 = (data[sw1_sw2_offset]<<8|data[sw1_sw2_offset+1]);

		if(EMM_Message_Receive_status.uiMessageType == EMM_AUTODISPLAY_MESSAGE)
		{	
			EMM_AutoMessage_CodeResion.uiAlternation_Detector = (BCAS_EMM_Indi_MessageArch.Response_message_code[0]<<8) |BCAS_EMM_Indi_MessageArch.Response_message_code[1];
			EMM_AutoMessage_CodeResion.uiLimmit_Date = (BCAS_EMM_Indi_MessageArch.Response_message_code[2]<<8) |BCAS_EMM_Indi_MessageArch.Response_message_code[3];
			EMM_AutoMessage_CodeResion.uiFixed_Mmessage_ID = (BCAS_EMM_Indi_MessageArch.Response_message_code[4]<<8) |BCAS_EMM_Indi_MessageArch.Response_message_code[5];
			EMM_AutoMessage_CodeResion.ucExtraMessage_FormatVersion = BCAS_EMM_Indi_MessageArch.Response_message_code[6];
			EMM_AutoMessage_CodeResion.uiExtra_Message_Len = (BCAS_EMM_Indi_MessageArch.Response_message_code[7]<<8) |BCAS_EMM_Indi_MessageArch.Response_message_code[8];

			ALOGE("[%d][Encrypted Auto Message] EMM_Message_CodeResion.uiExtra_Message_Len = %x \n",  __LINE__, EMM_AutoMessage_CodeResion.uiExtra_Message_Len);

			if(EMM_AutoMessage_CodeResion.uiExtra_Message_Len>0)
			{	
				memset(EMM_AutoMessage_CodeResion.ucExtra_MessageCode, 0, sizeof(EMM_AutoMessage_CodeResion.ucExtra_MessageCode));
				memcpy(EMM_AutoMessage_CodeResion.ucExtra_MessageCode , 
					&BCAS_EMM_Indi_MessageArch.Response_message_code[ISDBT_EMM_MESSAGECODE_EXTRA_MESSAGE_CODE_POSITION], 
					EMM_AutoMessage_CodeResion.uiExtra_Message_Len);
			}

#if 1
			ALOGE("[Encrypted Auto Message] BCAS_EMM_Indi_MessageArch.Protocol_Unit_Num = %x \n",  BCAS_EMM_Indi_MessageArch.Protocol_Unit_Num);
			ALOGE("[Encrypted Auto Message] BCAS_EMM_Indi_MessageArch.Unit_Len = %d \n",  BCAS_EMM_Indi_MessageArch.Unit_Len);
			ALOGE("[Encrypted Auto Message] BCAS_EMM_Indi_MessageArch.IC_Card_Instruction = %x \n",  BCAS_EMM_Indi_MessageArch.IC_Card_Instruction);
			ALOGE("[Encrypted Auto Message] BCAS_EMM_Indi_MessageArch.Return_Code = %x \n",  BCAS_EMM_Indi_MessageArch.Return_Code);
			ALOGE("[Encrypted Auto Message] BCAS_EMM_Indi_MessageArch.SW1_SW2 = %x \n",  BCAS_EMM_Indi_MessageArch.SW1_SW2);
			ALOGE("[Encrypted Auto Message] EMM_AutoMessage_CodeResion.uiAlternation_Detector = %x \n",  EMM_AutoMessage_CodeResion.uiAlternation_Detector);
			ALOGE("[Encrypted Auto Message] EMM_AutoMessage_CodeResion.uiLimmit_Date = %x \n",  EMM_AutoMessage_CodeResion.uiLimmit_Date);
			ALOGE("[Encrypted Auto Message] EMM_AutoMessage_CodeResion.uiFixed_Mmessage_ID = %x \n",  EMM_AutoMessage_CodeResion.uiFixed_Mmessage_ID);
			ALOGE("[Encrypted Auto Message] EMM_AutoMessage_CodeResion.ucExtraMessage_FormatVersion = %x \n",  EMM_AutoMessage_CodeResion.ucExtraMessage_FormatVersion);
			ALOGE("[Encrypted Auto Message] EMM_AutoMessage_CodeResion.uiExtra_Message_Len = %x \n",  EMM_AutoMessage_CodeResion.uiExtra_Message_Len);
	#if 1
			if(EMM_AutoMessage_CodeResion.uiExtra_Message_Len>0)
			{
				int i;
				for(i=0; i<(EMM_AutoMessage_CodeResion.uiExtra_Message_Len-3); i+=4)
				{
					ALOGE("[Encrypted Auto Message] ucExtra_MessageCode[%d] = [%x, %x, %x, %x] \n", i, EMM_AutoMessage_CodeResion.ucExtra_MessageCode[i],
						EMM_AutoMessage_CodeResion.ucExtra_MessageCode[i+1],EMM_AutoMessage_CodeResion.ucExtra_MessageCode[i+2],
						EMM_AutoMessage_CodeResion.ucExtra_MessageCode[i+3]);
				}
			}
	#endif		
#endif
		}
		else			//Mail lMessage
		{
			EMM_MailMessage_CodeResion.uiReserved1= (BCAS_EMM_Indi_MessageArch.Response_message_code[0]<<8) |BCAS_EMM_Indi_MessageArch.Response_message_code[1];
			EMM_MailMessage_CodeResion.uiReserved2 = (BCAS_EMM_Indi_MessageArch.Response_message_code[2]<<8) |BCAS_EMM_Indi_MessageArch.Response_message_code[3];
			EMM_MailMessage_CodeResion.uiFixed_Mmessage_ID = (BCAS_EMM_Indi_MessageArch.Response_message_code[4]<<8) |BCAS_EMM_Indi_MessageArch.Response_message_code[5];
			EMM_MailMessage_CodeResion.ucExtraMessage_FormatVersion = BCAS_EMM_Indi_MessageArch.Response_message_code[6];
			EMM_MailMessage_CodeResion.uiExtra_Message_Len = (BCAS_EMM_Indi_MessageArch.Response_message_code[7]<<8) |BCAS_EMM_Indi_MessageArch.Response_message_code[8];

			ALOGE("[%d][Encrypted Mail] EMM_MailMessage_CodeResion.uiExtra_Message_Len = %x \n", __LINE__,   EMM_MailMessage_CodeResion.uiExtra_Message_Len);

			if(EMM_MailMessage_CodeResion.uiExtra_Message_Len>0)
			{	
				memset(EMM_MailMessage_CodeResion.ucExtra_MessageCode, 0, sizeof(EMM_MailMessage_CodeResion.ucExtra_MessageCode));
				memcpy(EMM_MailMessage_CodeResion.ucExtra_MessageCode , 
					&BCAS_EMM_Indi_MessageArch.Response_message_code[ISDBT_EMM_MESSAGECODE_EXTRA_MESSAGE_CODE_POSITION], 
					EMM_MailMessage_CodeResion.uiExtra_Message_Len);
#if 1
				ALOGE("[%d][Encrypted Mail] BCAS_EMM_Indi_MessageArch.Protocol_Unit_Num = %x \n",  __LINE__, BCAS_EMM_Indi_MessageArch.Protocol_Unit_Num);
				ALOGE("[%d][Encrypted Mail] BCAS_EMM_Indi_MessageArch.Unit_Len = %d \n", __LINE__,   BCAS_EMM_Indi_MessageArch.Unit_Len);
				ALOGE("[%d][Encrypted Mail] BCAS_EMM_Indi_MessageArch.IC_Card_Instruction = %x \n", __LINE__,   BCAS_EMM_Indi_MessageArch.IC_Card_Instruction);
				ALOGE("[%d][Encrypted Mail] BCAS_EMM_Indi_MessageArch.Return_Code = %x \n", __LINE__,   BCAS_EMM_Indi_MessageArch.Return_Code);
				ALOGE("[%d][Encrypted Mail] BCAS_EMM_Indi_MessageArch.SW1_SW2 = %x \n", __LINE__,   BCAS_EMM_Indi_MessageArch.SW1_SW2);
#if 0
				if(BCAS_EMM_Indi_MessageArch.Unit_Len>4)
				{
					int i;
					ALOGE("BCAS_EMM_Indi_MessageArch.Response_message_code[%d] = %x \n", i, BCAS_EMM_Indi_MessageArch.Response_message_code[i]);
					for(i=0; i<(BCAS_EMM_Indi_MessageArch.Unit_Len -4); i++)
					{
						ALOGE("0x%x \n", BCAS_EMM_Indi_MessageArch.Response_message_code[i]);
					}
				}
#endif
				ALOGE("[%d][Encrypted Mail] EMM_Message_CodeResion.uiReserved1 = %x \n", __LINE__,   EMM_MailMessage_CodeResion.uiReserved1);
				ALOGE("[%d][Encrypted Mail] EMM_Message_CodeResion.uiReserved2 = %x \n",  __LINE__,  EMM_MailMessage_CodeResion.uiReserved2);
				ALOGE("[%d][Encrypted Mail] EMM_Message_CodeResion.uiFixed_Mmessage_ID = %x \n", __LINE__,   EMM_MailMessage_CodeResion.uiFixed_Mmessage_ID);
				ALOGE("[%d][Encrypted Mail] EMM_Message_CodeResion.ucExtraMessage_FormatVersion = %x \n",  __LINE__,  EMM_MailMessage_CodeResion.ucExtraMessage_FormatVersion);
				ALOGE("[%d][Encrypted Mail] EMM_Message_CodeResion.uiExtra_Message_Len = %x \n",  __LINE__,  EMM_MailMessage_CodeResion.uiExtra_Message_Len);
		#if 0
				if(EMM_MailMessage_CodeResion.uiExtra_Message_Len>0)
				{
					int i;
					for(i=0; i<(EMM_MailMessage_CodeResion.uiExtra_Message_Len-3); i+=4)
					{
						ALOGE("EMM_MailMessage_CodeResion.ucExtra_MessageCode[%d] = [%x, %x, %x, %x] \n", i, EMM_MailMessage_CodeResion.ucExtra_MessageCode[i],
							EMM_MailMessage_CodeResion.ucExtra_MessageCode[i+1],EMM_MailMessage_CodeResion.ucExtra_MessageCode[i+2],
							EMM_MailMessage_CodeResion.ucExtra_MessageCode[i+3]);
					}
				}
		#endif		
#endif
			}

			if(EMM_MailMessage_CodeResion.uiFixed_Mmessage_ID == ISDBT_MAIL_NO_PRESET_MESSAGE )
			{
				TCC_Dxb_SC_Manager_EMM_MessageInfoSet();
			}
		}
	}
	else
	{
//		ALOGE("%s %d uiReceive_Cnt = %d ,uiTotal_Cnt = %d \n", __func__, __LINE__, EMG_ResCMD_MessageCode_LengthCheck.uiReceive_Cnt, EMG_ResCMD_MessageCode_LengthCheck.uiTotal_Cnt);
		if(EMG_ResCMD_MessageCode_LengthCheck.uiReceive_Cnt == EMG_ResCMD_MessageCode_LengthCheck.uiTotal_Cnt)
		{
			sw1_sw2_offset = RES_DATAFIELD_RETURN_CODE + (BCAS_EMM_Indi_MessageArch.Unit_Len-4);
			BCAS_EMM_Indi_MessageArch.SW1_SW2 = (data[sw1_sw2_offset]<<8|data[sw1_sw2_offset+1]);
		}

		if(EMM_Message_Receive_status.uiMessageType == EMM_AUTODISPLAY_MESSAGE)
		{	
			if(EMG_ResCMD_MessageCode_LengthCheck.uiReceive_Cnt == 0)
			{	
				EMM_AutoMessage_CodeResion.uiAlternation_Detector= (BCAS_EMM_Indi_MessageArch.Response_message_code[0]<<8) |BCAS_EMM_Indi_MessageArch.Response_message_code[1];
				EMM_AutoMessage_CodeResion.uiLimmit_Date = (BCAS_EMM_Indi_MessageArch.Response_message_code[2]<<8) |BCAS_EMM_Indi_MessageArch.Response_message_code[3];
				EMM_AutoMessage_CodeResion.uiFixed_Mmessage_ID = (BCAS_EMM_Indi_MessageArch.Response_message_code[4]<<8) |BCAS_EMM_Indi_MessageArch.Response_message_code[5];
				EMM_AutoMessage_CodeResion.ucExtraMessage_FormatVersion = BCAS_EMM_Indi_MessageArch.Response_message_code[6];
				EMM_AutoMessage_CodeResion.uiExtra_Message_Len = (BCAS_EMM_Indi_MessageArch.Response_message_code[7]<<8) |BCAS_EMM_Indi_MessageArch.Response_message_code[8];

				ALOGE("[Encrypted Auto Message] uiExtra_Message_Len = %x \n",  EMM_AutoMessage_CodeResion.uiExtra_Message_Len);
				BCAS_Multi_Partion_Position =0;
				
				if(EMM_AutoMessage_CodeResion.uiExtra_Message_Len>0)
				{	
//					memset(EMM_MailMessage_CodeResion.ucExtra_MessageCode, 0, sizeof(EMM_MailMessage_CodeResion.ucExtra_MessageCode));
					memcpy(EMM_AutoMessage_CodeResion.ucExtra_MessageCode , 
						&BCAS_EMM_Indi_MessageArch.Response_message_code[ISDBT_EMM_MESSAGECODE_EXTRA_MESSAGE_CODE_POSITION], 
						(BCAS_EMM_Indi_MessageArch.Unit_Len -ISDBT_EMM_INDI_MESSAGE_RECEIVECMD_DEFAULT_LEN -ISDBT_EMM_MESSAGECODE_EXTRA_MESSAGE_CODE_POSITION + ISDBT_EMM_SW1SW2_LEN) );
					BCAS_Multi_Partion_Position += (BCAS_EMM_Indi_MessageArch.Unit_Len -ISDBT_EMM_INDI_MESSAGE_RECEIVECMD_DEFAULT_LEN -ISDBT_EMM_MESSAGECODE_EXTRA_MESSAGE_CODE_POSITION + ISDBT_EMM_SW1SW2_LEN);
				}
			}
			else
			{
				memcpy(&EMM_AutoMessage_CodeResion.ucExtra_MessageCode[BCAS_Multi_Partion_Position] , 
					&BCAS_EMM_Indi_MessageArch.Response_message_code[0], (BCAS_EMM_Indi_MessageArch.Unit_Len -ISDBT_EMM_INDI_MESSAGE_RECEIVECMD_DEFAULT_LEN + ISDBT_EMM_SW1SW2_LEN) );
				BCAS_Multi_Partion_Position += (BCAS_EMM_Indi_MessageArch.Unit_Len -ISDBT_EMM_INDI_MESSAGE_RECEIVECMD_DEFAULT_LEN + ISDBT_EMM_SW1SW2_LEN);
			}

		}
		else			//Mail lMessage
		{
			if(EMG_ResCMD_MessageCode_LengthCheck.uiReceive_Cnt == 0)
			{	
				EMM_MailMessage_CodeResion.uiReserved1= (BCAS_EMM_Indi_MessageArch.Response_message_code[0]<<8) |BCAS_EMM_Indi_MessageArch.Response_message_code[1];
				EMM_MailMessage_CodeResion.uiReserved2 = (BCAS_EMM_Indi_MessageArch.Response_message_code[2]<<8) |BCAS_EMM_Indi_MessageArch.Response_message_code[3];
				EMM_MailMessage_CodeResion.uiFixed_Mmessage_ID = (BCAS_EMM_Indi_MessageArch.Response_message_code[4]<<8) |BCAS_EMM_Indi_MessageArch.Response_message_code[5];
				EMM_MailMessage_CodeResion.ucExtraMessage_FormatVersion = BCAS_EMM_Indi_MessageArch.Response_message_code[6];
				EMM_MailMessage_CodeResion.uiExtra_Message_Len = (BCAS_EMM_Indi_MessageArch.Response_message_code[7]<<8) |BCAS_EMM_Indi_MessageArch.Response_message_code[8];

				ALOGE("[Encrypted Mail][%d] EMM_MailMessage_CodeResion.uiExtra_Message_Len = %d \n",  __LINE__, EMM_MailMessage_CodeResion.uiExtra_Message_Len);
				ALOGE("[Encrypted Mail][%d] BCAS_EMM_Indi_MessageArch.Unit_Len = %d \n",   __LINE__, BCAS_EMM_Indi_MessageArch.Unit_Len);
				BCAS_Multi_Partion_Position =0;
			
				if(EMM_MailMessage_CodeResion.uiExtra_Message_Len>0 && \
					BCAS_EMM_Indi_MessageArch.Unit_Len>(ISDBT_EMM_INDI_MESSAGE_RECEIVECMD_DEFAULT_LEN + ISDBT_EMM_MESSAGECODE_EXTRA_MESSAGE_CODE_POSITION) )
				{	
					memset(EMM_MailMessage_CodeResion.ucExtra_MessageCode, 0, sizeof(EMM_MailMessage_CodeResion.ucExtra_MessageCode));
					memcpy(EMM_MailMessage_CodeResion.ucExtra_MessageCode , 
						&BCAS_EMM_Indi_MessageArch.Response_message_code[ISDBT_EMM_MESSAGECODE_EXTRA_MESSAGE_CODE_POSITION], 
						(BCAS_EMM_Indi_MessageArch.Unit_Len -ISDBT_EMM_INDI_MESSAGE_RECEIVECMD_DEFAULT_LEN -ISDBT_EMM_MESSAGECODE_EXTRA_MESSAGE_CODE_POSITION + ISDBT_EMM_SW1SW2_LEN) );
					BCAS_Multi_Partion_Position += (BCAS_EMM_Indi_MessageArch.Unit_Len -ISDBT_EMM_INDI_MESSAGE_RECEIVECMD_DEFAULT_LEN -ISDBT_EMM_MESSAGECODE_EXTRA_MESSAGE_CODE_POSITION + ISDBT_EMM_SW1SW2_LEN);
				}
			}
			else
			{
				memcpy(&EMM_MailMessage_CodeResion.ucExtra_MessageCode[BCAS_Multi_Partion_Position] , 
					&BCAS_EMM_Indi_MessageArch.Response_message_code[0], 
					(BCAS_EMM_Indi_MessageArch.Unit_Len -ISDBT_EMM_INDI_MESSAGE_RECEIVECMD_DEFAULT_LEN + ISDBT_EMM_SW1SW2_LEN) );
				BCAS_Multi_Partion_Position += BCAS_EMM_Indi_MessageArch.Unit_Len -ISDBT_EMM_INDI_MESSAGE_RECEIVECMD_DEFAULT_LEN + ISDBT_EMM_SW1SW2_LEN;
			}
		}

		EMG_ResCMD_MessageCode_LengthCheck.uiReceive_Cnt ++;

		if(EMG_ResCMD_MessageCode_LengthCheck.uiReceive_Cnt == EMG_ResCMD_MessageCode_LengthCheck.uiTotal_Cnt && 
			EMM_Message_Receive_status.uiMessageType == EMM_MAIL_MESSAGE)
		{
#if 1
			ALOGE("[Encrypted Mail] BCAS_EMM_Indi_MessageArch.Protocol_Unit_Num = %x \n",  BCAS_EMM_Indi_MessageArch.Protocol_Unit_Num);
			ALOGE("[Encrypted Mail] BCAS_EMM_Indi_MessageArch.Unit_Len = %d \n",  BCAS_EMM_Indi_MessageArch.Unit_Len);
			ALOGE("[Encrypted Mail] BCAS_EMM_Indi_MessageArch.IC_Card_Instruction = %x \n",  BCAS_EMM_Indi_MessageArch.IC_Card_Instruction);
			ALOGE("[Encrypted Mail] BCAS_EMM_Indi_MessageArch.Return_Code = %x \n",  BCAS_EMM_Indi_MessageArch.Return_Code);
			ALOGE("[Encrypted Mail] BCAS_EMM_Indi_MessageArch.SW1_SW2 = %x \n",  BCAS_EMM_Indi_MessageArch.SW1_SW2);
			ALOGE("[Encrypted Mail] EMM_Message_CodeResion.uiReserved1 = %x \n",  EMM_MailMessage_CodeResion.uiReserved1);
			ALOGE("[Encrypted Mail] EMM_Message_CodeResion.uiReserved2 = %x \n",  EMM_MailMessage_CodeResion.uiReserved2);
			ALOGE("[Encrypted Mail] EMM_Message_CodeResion.uiFixed_Mmessage_ID = %x \n",  EMM_MailMessage_CodeResion.uiFixed_Mmessage_ID);
			ALOGE("[Encrypted Mail] EMM_Message_CodeResion.ucExtraMessage_FormatVersion = %x \n",  EMM_MailMessage_CodeResion.ucExtraMessage_FormatVersion);
			ALOGE("[Encrypted Mail] EMM_Message_CodeResion.uiExtra_Message_Len = %d \n",  EMM_MailMessage_CodeResion.uiExtra_Message_Len);
			ALOGE("[Encrypted Mail] BCAS_Multi_Partion_Position= %d \n",  BCAS_Multi_Partion_Position);
		#if 0
			if(EMM_MailMessage_CodeResion.uiExtra_Message_Len>0)
			{
				int i;
				for(i=0; i<(EMM_MailMessage_CodeResion.uiExtra_Message_Len-3); i+=4)
				{
					ALOGE("[Encrypted Mail] EMM_MailMessage_CodeResion.ucExtra_MessageCode[%d] = [%x, %x, %x, %x] \n", i, EMM_MailMessage_CodeResion.ucExtra_MessageCode[i],
						EMM_MailMessage_CodeResion.ucExtra_MessageCode[i+1],EMM_MailMessage_CodeResion.ucExtra_MessageCode[i+2],
						EMM_MailMessage_CodeResion.ucExtra_MessageCode[i+3]);
				}
			}
		#endif	
#endif
			if(EMM_MailMessage_CodeResion.uiFixed_Mmessage_ID == ISDBT_MAIL_NO_PRESET_MESSAGE )
			{
				TCC_Dxb_SC_Manager_EMM_MessageInfoSet();
			}
		}
	}

 	return err;
	
}

/************************************************************************************************************
* FUNCTION NAME : TCC_Dxb_SC_Manager_ProcessResEMD
* DESCRIPTION :  Automatic Display Message Display Information Acquire response
* INPUT	 :  Response APDU. (only APDU Data)	
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
int TCC_Dxb_SC_Manager_ProcessResEMD(unsigned char *data)
{
	int	err = SC_ERR_OK;
	unsigned int	sw1_sw2_offset =0;

	DEBUG(DEFAULT_MESSAGES, "%s %d \n", __func__, __LINE__);

	BCAS_Auto_Disp_MessageArch.Return_Code = (data[RES_DATAFIELD_RETURN_CODE]<<8|data[RES_DATAFIELD_RETURN_CODE +1]);
	if(BCAS_Auto_Disp_MessageArch.Return_Code != BCASC_EMD_RETURN_CODE_NORMAL)
	{
		DEBUG(DEFAULT_MESSAGES,"%s %d EMD Response Return_Code = %x \n", __func__, __LINE__, BCAS_Auto_Disp_MessageArch.Return_Code);
		ALOGE("%s %d EMD Response Return_Code = %x \n", __func__, __LINE__, BCAS_Auto_Disp_MessageArch.Return_Code);
		return SC_ERR_RETURN_CODE;
	}

	BCAS_Auto_Disp_MessageArch.Protocol_Unit_Num = data[RES_DATAFIELD_PRO_UNITNUM];
	BCAS_Auto_Disp_MessageArch.Unit_Len = data[RES_DATAFIELD_UNIT_LEN];
	BCAS_Auto_Disp_MessageArch.IC_Card_Instruction = (data[RES_DATAFIELD_CARD_INS]<<8|data[RES_DATAFIELD_CARD_INS+1]);
	BCAS_Auto_Disp_MessageArch.Expiration_date = (data[AUOT_DISP_EXPIRATION_DATA]<<8|data[AUOT_DISP_EXPIRATION_DATA+1]);
	BCAS_Auto_Disp_MessageArch.Message_preset_text_no = (data[AUOT_DISP_MESSAGE_PRESET_TEXT_NO]<<8|data[AUOT_DISP_MESSAGE_PRESET_TEXT_NO+1]);
	BCAS_Auto_Disp_MessageArch.Differential_format_number = data[AUTO_DISP_DIFF_FORMAT_NUM];
	BCAS_Auto_Disp_MessageArch.Differential_info_length = (data[AUTO_DISP_DIFF_INFO_LEN]<<8|data[AUTO_DISP_DIFF_INFO_LEN+1]);

	if(BCAS_Auto_Disp_MessageArch.Differential_info_length>0)
	{	
/* Original
		BCAS_Auto_Disp_MessageArch.Differential_information = (unsigned char *)tcc_mw_malloc(__FUNCTION__, __LINE__, BCAS_Auto_Disp_MessageArch.Differential_info_length);
		memcpy(BCAS_Auto_Disp_MessageArch.Differential_information, &data[AUTO_DISP_DIFF_INFO], BCAS_Auto_Disp_MessageArch.Differential_info_length);
*/
//10th Try
//David, To avoid Codesonar's warning, NULL Pointer Dereference
//add code for handling exception
		BCAS_Auto_Disp_MessageArch.Differential_information = (unsigned char *)tcc_mw_malloc(__FUNCTION__, __LINE__, BCAS_Auto_Disp_MessageArch.Differential_info_length);
		if(BCAS_Auto_Disp_MessageArch.Differential_information == NULL)
		{
			ALOGE("[%s:%d] Differential_information malloc-fail\n", __func__, __LINE__);
			return SC_ERR_RETURN_CODE;
		}
		else
		{
			memcpy(BCAS_Auto_Disp_MessageArch.Differential_information, &data[AUTO_DISP_DIFF_INFO], BCAS_Auto_Disp_MessageArch.Differential_info_length);
		}
	}	

	sw1_sw2_offset = AUTO_DISP_DIFF_INFO_LEN + BCAS_Auto_Disp_MessageArch.Differential_info_length;

	BCAS_Auto_Disp_MessageArch.SW1_SW2 = (data[sw1_sw2_offset]<<8|data[sw1_sw2_offset+1]);
	BCAS_Auto_Disp_MessageArch.ucMessageInfoGetStatus = 1;

#if 1
	ALOGE(" BCAS_Auto_Disp_MessageArch.Protocol_Unit_Num = %x \n", BCAS_Auto_Disp_MessageArch.Protocol_Unit_Num);
	ALOGE(" BCAS_Auto_Disp_MessageArch.Unit_Len = %d \n",  BCAS_Auto_Disp_MessageArch.Unit_Len);
	ALOGE(" BCAS_Auto_Disp_MessageArch.IC_Card_Instruction = %x \n",  BCAS_Auto_Disp_MessageArch.IC_Card_Instruction);
	ALOGE(" BCAS_Auto_Disp_MessageArch.Return_Code = %x \n",  BCAS_Auto_Disp_MessageArch.Return_Code);
	ALOGE(" BCAS_Auto_Disp_MessageArch.Expiration_date = %x \n",  BCAS_Auto_Disp_MessageArch.Expiration_date);
	ALOGE(" BCAS_Auto_Disp_MessageArch.Message_preset_text_no = %d \n",  BCAS_Auto_Disp_MessageArch.Message_preset_text_no);
	ALOGE(" BCAS_Auto_Disp_MessageArch.Differential_format_number = %x \n",  BCAS_Auto_Disp_MessageArch.Differential_format_number);
	ALOGE(" BCAS_Auto_Disp_MessageArch.Differential_info_length = %x \n",  BCAS_Auto_Disp_MessageArch.Differential_info_length);
	ALOGE(" BCAS_Auto_Disp_MessageArch.SW1_SW2 = %x \n",  BCAS_Auto_Disp_MessageArch.SW1_SW2);

	if(BCAS_Auto_Disp_MessageArch.Unit_Len>4)
	{
		int i=0;
		ALOGE("BCAS_Auto_Disp_MessageArch.Differential_information[%d] = %x \n", i, BCAS_Auto_Disp_MessageArch.Differential_information[i]);

		for(i=0; i<BCAS_Auto_Disp_MessageArch.Differential_info_length; i++)
		{
			ALOGE("0x%x ", BCAS_Auto_Disp_MessageArch.Differential_information[i]);
		}
		ALOGE("\n");
	}

#endif

	return err;
	
}



/************************************************************************************************************
* FUNCTION NAME : TCC_Dxb_SC_Manager_ProcessResIDI
* DESCRIPTION :  BCas Acquire CARD ID Information response processing.
* INPUT	 :  Response APDU. (only APDU Data)	
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
int TCC_Dxb_SC_Manager_ProcessResIDI(unsigned char *data)
{
	int	err = SC_ERR_OK;
	int	i=0, j=0;

	DEBUG(DEFAULT_MESSAGES, "%s %d \n", __func__, __LINE__);

	BCAS_CardID_Arch.Return_Code = (data[RES_DATAFIELD_RETURN_CODE]<<8|data[RES_DATAFIELD_RETURN_CODE +1]);
	if(BCAS_CardID_Arch.Return_Code != BCASC_EMM_RETURN_CODE_NORMAL)
	{
		DEBUG(DEFAULT_MESSAGES,"%s %d Card ID Response Return_Code = %x \n", __func__, __LINE__, BCAS_CardID_Arch.Return_Code);
		return SC_ERR_RETURN_CODE;
	}

	BCAS_CardID_Arch.Protocol_Unit_Num = data[RES_DATAFIELD_PRO_UNITNUM];
	BCAS_CardID_Arch.Unit_Len = data[RES_DATAFIELD_UNIT_LEN];
	BCAS_CardID_Arch.IC_Card_Instruction = (data[RES_DATAFIELD_CARD_INS]<<8|data[RES_DATAFIELD_CARD_INS+1]);
	BCAS_CardID_Arch.Number_of_CardIDs = data[CARDID_NUMOFIDS_POS];

	if (BCAS_CardID_Arch.Number_of_CardIDs >= BCAS_CARDID_MAX_COUNT)
		BCAS_CardID_Arch.Number_of_CardIDs = BCAS_CARDID_MAX_COUNT;

	for(i=0; i<BCAS_CardID_Arch.Number_of_CardIDs; i++)
	{
		BCAS_CardID_Arch.Display_Card_ID[i].Card_Type.Manufacturer_identifier = data[CARDID_DISPLAY_CARDID_POS*(i+1)];
		BCAS_CardID_Arch.Display_Card_ID[i].Card_Type.Version = data[(CARDID_DISPLAY_CARDID_POS+1)*(i+1)];
		BCAS_CardID_Arch.Display_Card_ID[i].Card_Id.ID_Identifier = (data[(CARDID_DISPLAY_CARDID_POS+2)*(i+1)])>>5;
		memcpy(BCAS_CardID_Arch.Display_Card_ID[i].Card_Id.ID, &data[(CARDID_DISPLAY_CARDID_POS+2)*(i+1)], BCAS_CARDID_LEGTH);
		BCAS_CardID_Arch.Display_Card_ID[i].Card_Id.ID[0] = (BCAS_CardID_Arch.Display_Card_ID[i].Card_Id.ID[0] & 0x1f);
		BCAS_CardID_Arch.Display_Card_ID[i].Check_Code =(data[(CARDID_DISPLAY_CARDID_POS+8)*(i+1)]<<8|data[(CARDID_DISPLAY_CARDID_POS+9)*(i+1)]);
	}

	BCAS_CardID_Arch.SW1_SW2 = (data[BCAS_CardID_Arch.Unit_Len+2]<<8|data[BCAS_CardID_Arch.Unit_Len+3]);

#if 0
	ALOGE("%s %d BCAS_CardID_Arch.Protocol_Unit_Num = %x \n", __func__, __LINE__, BCAS_CardID_Arch.Protocol_Unit_Num);
	ALOGE("%s %d BCAS_CardID_Arch.Unit_Len = %d \n", __func__, __LINE__, BCAS_CardID_Arch.Unit_Len);
	ALOGE("%s %d BCAS_CardID_Arch.IC_Card_Instruction = %x \n", __func__, __LINE__, BCAS_CardID_Arch.IC_Card_Instruction);
	ALOGE("%s %d BCAS_CardID_Arch.Number_of_CardIDs = %x \n", __func__, __LINE__, BCAS_CardID_Arch.Number_of_CardIDs);
	ALOGE("%s %d BCAS_CardID_Arch.Return_Code = %x \n", __func__, __LINE__, BCAS_CardID_Arch.Return_Code);

	for(j=0; j<BCAS_CardID_Arch.Number_of_CardIDs; j++)
	{
		ALOGE("%s %d BCAS_CardID_Arch.Display_Card_ID[%d].Card_Type.Manufacturer_identifier = %x \n", __func__, __LINE__, j, BCAS_CardID_Arch.Display_Card_ID[j].Card_Type.Manufacturer_identifier);
		ALOGE("%s %d BCAS_CardID_Arch.Display_Card_ID[%d].Card_Type.Version = %x \n", __func__, __LINE__, j, BCAS_CardID_Arch.Display_Card_ID[j].Card_Type.Version);
		ALOGE("%s %d BCAS_CardID_Arch.Display_Card_ID[%d].Card_Id.ID_Identifier = %x \n", __func__, __LINE__, j, BCAS_CardID_Arch.Display_Card_ID[j].Card_Id.ID_Identifier);
		ALOGE("%s %d BCAS_CardID_Arch.Display_Card_ID[%d].Check_Code = %x \n", __func__, __LINE__, j, BCAS_CardID_Arch.Display_Card_ID[j].Check_Code);

		for(i=0; i<BCAS_CARDID_LEGTH; i++)
		{
			ALOGE("BCAS_CardID_Arch.Display_Card_ID[%d].Card_Id.ID[%d] = %x \n", j, i, BCAS_CardID_Arch.Display_Card_ID[j].Card_Id.ID[i]);
		}
	}
	
	ALOGE("%s %d BCAS_CardID_Arch.SW1_SW2 = %x \n", __func__, __LINE__, BCAS_CardID_Arch.SW1_SW2);
#endif
	
	return err;
}

/************************************************************************************************************
* FUNCTION NAME : TCC_DxB_SC_Manager_ProcessResponse
* DESCRIPTION :  BCas INS processing.
* INPUT	 :  INS: Command INS Code,  data : APDU Data(not Block Data)
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
int TCC_DxB_SC_Manager_ProcessResponse(char ins, unsigned char *data)
{
	int	err = SC_ERR_OK;
	
	DEBUG(DEFAULT_MESSAGES, "%s %d \n", __func__, __LINE__);

	switch(ins)
	{
		case SC_INS_PARAM_INT:
				err= TCC_Dxb_SC_Manager_ProcessResINT(data);
			break;

		case SC_INS_PARAM_ECM:
				err= TCC_Dxb_SC_Manager_ProcessResECM(data);
			break;

		case SC_INS_PARAM_EMM:
				err = TCC_Dxb_SC_Manager_ProcessResEMM(data);
			break;

		case SC_INS_PARAM_CHK:
			break;

		case SC_INS_PARAM_EMG:
				err = TCC_Dxb_SC_Manager_ProcessResEMG(data);
			break;

		case SC_INS_PARAM_EMD:
				err = TCC_Dxb_SC_Manager_ProcessResEMD(data);
			break;

		case SC_INS_PARAM_PVS:
			break;

		case SC_INS_PARAM_PPV:
			break;

		case SC_INS_PARAM_PRP:
			break;

		case SC_INS_PARAM_CRQ:
			break;

		case SC_INS_PARAM_TLS:
			break;

		case SC_INS_PARAM_RQD:
			break;

		case SC_INS_PARAM_CRD:
			break;

		case SC_INS_PARAM_UDT:
			break;

		case SC_INS_PARAM_UTN:
			break;

		case SC_INS_PARAM_UUR:
			break;

		case SC_INS_PARAM_IRS:
			break;

		case SC_INS_PARAM_CRY:
			break;

		case SC_INS_PARAM_UNC:
			break;

		case SC_INS_PARAM_IRR:
			break;

		case SC_INS_PARAM_WUI:
			break;

		case SC_INS_PARAM_IDI:
			TCC_Dxb_SC_Manager_ProcessResIDI(data);
			break;

		default:
			break;				
	}

	return err;
}

/************************************************************************************************************
* FUNCTION NAME : TCC_DxB_SC_Manager_ReceiveErrCheck
* DESCRIPTION :  Smart Card response Block Error Check
* INPUT	 :  data: Block Data.
* OUTPUT  :  refer to "Smart Card Error Type define"
* REMARK  : 
*************************************************************************************************************/
int TCC_DxB_SC_Manager_ReceiveErrCheck(unsigned char *data)
{
	unsigned int RBlock_SEQNo=0;
	DEBUG(DEFAULT_MESSAGES, "%s %d \n", __func__, __LINE__);

	if(data[INITFIELD_NAD_POS] != SC_BLOCK_DEF_NAD)
	{
		DEBUG(DEFAULT_MESSAGES,"%s %d SC_ERR_INVALID_CMD  err NAD= %x \n", __func__, __LINE__, data[INITFIELD_NAD_POS]);
		ALOGE("%s %d SC_ERR_INVALID_CMD  err NAD= %x \n", __func__, __LINE__, data[INITFIELD_NAD_POS]);
		return SC_ERR_INVALID_NAD;
	}
	else if(data[INITFIELD_PCB_POS] & (SC_RBLOCK_ERR_EDC_MASK| SC_RBLOCK_ERR_OTHER_MASK))
	{
		DEBUG(DEFAULT_MESSAGES,"%s %d PCB  err PCB= %x \n", __func__, __LINE__, data[INITFIELD_PCB_POS]);
		ALOGE("%s %d PCB  err PCB= %x \n", __func__, __LINE__, data[INITFIELD_PCB_POS]);
		if(data[INITFIELD_PCB_POS] &SC_RBLOCK_ERR_EDC_MASK)
			return SC_ERR_RECEIVE_EDC;
		else
		{
			RBlock_SEQNo = (data[INITFIELD_PCB_POS] & SC_RBLOCK_SEQNO_MASK);
			if(RBlock_SEQNo == Seq_Num)
				return SC_ERR_RECEIVE_OTHER;
		}
	}
	return SC_ERR_OK;
}

/************************************************************************************************************
* FUNCTION NAME : TCC_DxB_SC_Manager_ReceiveBlockCheck
* DESCRIPTION :  Smart Card response Block Type Check.
* INPUT	 :  data: Block Data.
* OUTPUT  :  refer to enum "BCAS_BLOCKTYPE".
* REMARK  : 
*************************************************************************************************************/
int TCC_DxB_SC_Manager_ReceiveBlockCheck(unsigned char *data)
{
	unsigned int	ReceiveBlockInfo = 0;
	unsigned char	ReBlockType;

	DEBUG(DEFAULT_MESSAGES, "%s %d \n", __func__, __LINE__);

	ReBlockType = data[INITFIELD_PCB_POS]& SC_SBLOCK_MASK;
	if(ReBlockType == SC_BLOCK_DEF_SPCB)
	{
		if((data[INITFIELD_PCB_POS]& SC_SBLOCK_BLOCK_TYPE_MASK) == SC_SBLOCK_BLOCK_TYPE_REPS_MASK)
			ReceiveBlockInfo = BCAS_SBLCOK_RESPONSE;				
		else
		{
			if((data[INITFIELD_PCB_POS]& SC_SBLOCK_CON_MASK) ==SC_SBLOCK_CON_IFS)
				ReceiveBlockInfo = BCAS_SBLCOK_IFS_REQ;
			else if((data[INITFIELD_PCB_POS]& SC_SBLOCK_CON_MASK) ==SC_SBLOCK_CON_WTX)
				ReceiveBlockInfo = BCAS_SBLCOK_WTX_REQ;
			else if((data[INITFIELD_PCB_POS]& SC_SBLOCK_CON_MASK) ==SC_SBLOCK_CON_RES)
				ReceiveBlockInfo = BCAS_SBLCOK_RESYNC_REQ;
		}			
	}
	else if(ReBlockType == SC_BLOCK_DEF_RPCB)
	{
		ReceiveBlockInfo = BCAS_RBLCOK;
	}
	else
	{
		if((data[INITFIELD_PCB_POS]& SC_IBLOCK_MOREDATA_MASK) == SC_IBLOCK_MOREDATA_MASK)
			ReceiveBlockInfo = BCAS_IBLCOK_MODREDATA;
		else
			ReceiveBlockInfo = BCAS_IBLCOK;
	}

	return ReceiveBlockInfo;
}

/************************************************************************************************************
* FUNCTION NAME : TCC_DxB_SC_Manager_SendT1_Glue
* DESCRIPTION :  Smart Card T=1 Protocol
* INPUT	 :  ins:ins code,  data_len: command data length(LC)  , data : command data(Data  "ex: ecm, emm, etc..)
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
int TCC_DxB_SC_Manager_SendT1_Glue(char ins, int data_len, char *data)
{
	int	err = SC_ERR_OK;
	unsigned int	i=0, j=0;
	unsigned char	SC_Input_Data[ISDBT_MAX_BLOCK_SIZE] ;
	unsigned int	copy_code_size = 0;	

	if(SC_CLOSE_FLAG)
		return SC_ERR_CLOSE;
	
	pthread_mutex_lock(&tcc_sc_mutex);

	if((unsigned int)data_len > MAX_DATA_FIELD_LENGTH)
	{
		copy_code_size = MAX_DATA_FIELD_LENGTH;
		j =0; 
		ALOGE("%s %d data_len error[ins = %x]  [data_len = %d, Message_Partition_len = %d]\n",  __func__, __LINE__, ins, data_len,  MAX_DATA_FIELD_LENGTH);

		while(1)
		{	
			if((j + copy_code_size) > (unsigned int)data_len)
				copy_code_size = (data_len -j);

			for(i=0; i<copy_code_size; i++)
				SC_Input_Data[i] = data[i + j];

			err = TCC_DxB_SC_Manager_SendT1(ins, copy_code_size , (char*)SC_Input_Data);

			if(err !=SC_ERR_OK)
				break;

			if((j+MAX_DATA_FIELD_LENGTH) > (unsigned int)data_len)
				break;

			j+=MAX_DATA_FIELD_LENGTH ;
		}
	}
	else
	{	
		if (SC_PROTOCOL_TYPE  != BCAS_SC_PROTOCOL_TYPE)
		{
			if (TCC_DxB_SC_CardDetect() == DxB_ERR_OK)
			{
				err = TCC_DxB_SC_Manager_Init();
			}
			else
				err = SC_ERR_CARD_DETECT;

			if (err !=  DxB_ERR_OK)
			{
				pthread_mutex_unlock(&tcc_sc_mutex);
				return err;
			}
		}
		err = TCC_DxB_SC_Manager_SendT1(ins, data_len, data);

	}
	pthread_mutex_unlock(&tcc_sc_mutex);
	return err;
}

/************************************************************************************************************
* FUNCTION NAME : TCC_DxB_SC_Manager_SendT1
* DESCRIPTION :  Smart Card T=1 Protocol
* INPUT	 :  ins:ins code,  data_len: command data length(LC)  , data : command data(Data  "ex: ecm, emm, etc..)
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
int TCC_DxB_SC_Manager_SendT1(char ins, int data_len, char *data)
{
	unsigned int	i = 0;
	int	err = SC_ERR_OK;
	unsigned int	err_count = 0;
	unsigned char	ucReadBuf[ISDBT_MAX_BLOCK_SIZE] = { 0, };
	unsigned char	ucReadDataBuf[ISDBT_MAX_BLOCK_SIZE] = { 0, };
	unsigned int	uiReadLength = 0;
	unsigned char	ucWriteBuf[MAX_DATA_FIELD_LENGTH+DEFAULT_BLCOK_SIZE];
	unsigned int	uiWriteLength = 0;
	unsigned int	uiInfoFieldLength = 0;
	unsigned int	uiBlockPCBValue = SC_BLOCK_DEF_PCB;
	unsigned int	uiSeqNumber = Seq_Num;
	unsigned int	uiLeLengthPos = 0;
	unsigned char	SBlock_Block_Type = SC_SBLOCK_BLOCK_TYPE_REQ_MASK;
	unsigned char	SBlock_Control_Type = SC_SBLOCK_CON_IFS;
	unsigned int	SendBlock = 0;
	unsigned int	ReceiveBlock = 0;
	unsigned int	temp_data_len = 0;
	unsigned char	edc_lrc = 0;

	if(SC_CLOSE_FLAG)
		return SC_ERR_CLOSE;
	
	DEBUG(DEFAULT_MESSAGES, "%s %d data_len = %d\n", __func__, __LINE__, data_len);

	if(SC_PROTOCOL_TYPE  != BCAS_SC_PROTOCOL_TYPE)
	{
		if(TCC_DxB_SC_CardDetect() == DxB_ERR_OK)
		{
			err = TCC_DxB_SC_Manager_Init();
		}
		else
			err = SC_ERR_CARD_DETECT;
		
		return err;
	}

	//DEFAULT_CMD_APDU_LENGTH = CLA + INS + P1 + P2 + Lc
	if(data_len == 0)
		uiInfoFieldLength = DEFAULT_CMD_APDU_LENGTH;
	else
		uiInfoFieldLength = DEFAULT_CMD_APDU_LENGTH + data_len + 1;	//including DATA + Le

	INITFIELD_EDC_POS = INFOFIELD_INF_ST_POS+ uiInfoFieldLength;
	uiLeLengthPos = INITFIELD_EDC_POS-1;
	uiWriteLength = INITFIELD_EDC_POS +1;
	edc_lrc = TCC_Dxb_SC_Manager_DefSetEDC(EDC_TYPE_LRC);

	memset(ucReadDataBuf, 0, sizeof(ucReadDataBuf));
	APDU_RES_LENGTH = 0;
	while (1)
	{
		ucWriteBuf[INITFIELD_NAD_POS] = SC_BLOCK_DEF_NAD;
		if(uiBlockPCBValue == SC_BLOCK_DEF_IPCB)		ucWriteBuf[INITFIELD_PCB_POS] = (uiBlockPCBValue|uiSeqNumber);
		else if(uiBlockPCBValue == SC_BLOCK_DEF_RPCB)	ucWriteBuf[INITFIELD_PCB_POS] = (uiBlockPCBValue|(uiSeqNumber>>2));
		else if(uiBlockPCBValue == SC_BLOCK_DEF_SPCB)	ucWriteBuf[INITFIELD_PCB_POS] = (uiBlockPCBValue|SBlock_Block_Type|SBlock_Control_Type);	

		if(uiBlockPCBValue == SC_BLOCK_DEF_IPCB)
		{
			temp_data_len = data_len;
			if (data_len != 0)
			{
				uiInfoFieldLength = data_len + DEFAULT_CMD_APDU_LENGTH +1;
				if (uiInfoFieldLength > MAX_DATA_FIELD_LENGTH)
				{
					ucWriteBuf[INITFIELD_PCB_POS] |= SC_IBLOCK_MOREDATA_MASK;
					temp_data_len = MAX_DATA_FIELD_LENGTH - DEFAULT_CMD_APDU_LENGTH -1;
				}
			}
			
			if (temp_data_len == 0)
				ucWriteBuf[INITFIELD_LEN_POS] = DEFAULT_CMD_APDU_LENGTH;
			else
				ucWriteBuf[INITFIELD_LEN_POS] = temp_data_len + DEFAULT_CMD_APDU_LENGTH + 1;
			ucWriteBuf[INFOFIELD_CLA_POS] = SC_CMD_Header_ARCH.CLA ;
			ucWriteBuf[INFOFIELD_INS_POS] = ins ;
			ucWriteBuf[INFOFIELD_P1_POS] = SC_CMD_Header_ARCH.P1 ;
			ucWriteBuf[INFOFIELD_P2_POS] = SC_CMD_Header_ARCH.P2 ;

			if(temp_data_len != 0)
			{
				ucWriteBuf[INFOFIELD_CMDDATA_LEN_POS] = temp_data_len ;
				for(i=0; i<temp_data_len; i++)
				{
					ucWriteBuf[INFOFIELD_CMDDATA_POS+i] = *(data+i);
				}
				uiLeLengthPos = INFOFIELD_CMDDATA_POS + temp_data_len;
			}
			else
				uiLeLengthPos = INFOFIELD_P2_POS + 1; //Lc position iff there is no Le/DATA.
			ucWriteBuf[uiLeLengthPos] = BCAS_CMD_BODY_DEF_Le;
			uiLeLengthPos++;
			TCC_Dxb_SC_Manager_CalcEDC (ucWriteBuf, uiLeLengthPos, &edc_lrc);
			ucWriteBuf[uiLeLengthPos] = edc_lrc;
			uiWriteLength = uiLeLengthPos+1;
		}
		else //if(uiBlockPCBValue == SC_BLOCK_DEF_RPCB) R-Block & S-Block
		{
			ucWriteBuf[INITFIELD_LEN_POS] = 0;
			TCC_Dxb_SC_Manager_CalcEDC (ucWriteBuf, INITFIELD_LEN_POS+1, &edc_lrc);
			ucWriteBuf[INITFIELD_LEN_POS+1] = edc_lrc;
			uiWriteLength = 4;
		}
#if 0
//		if(ins == SC_INS_PARAM_EMG)
		{
			int i;
			ALOGE("%s %d ucWriteBuf  start \n", __func__, __LINE__);

			for (i=0; i<uiWriteLength; i++)
			{
				ALOGE("  i =%d, ucWriteBuf = %x ", i,  ucWriteBuf[i]);
			}
			ALOGE("%s %d uiWriteLength = %d \n", __func__, __LINE__, uiWriteLength);
		}
#endif	
		memset(ucReadBuf, 0, sizeof(ucReadBuf));
		err = TCC_DxB_SC_ReadData(0, ucWriteBuf, uiWriteLength, ucReadBuf, &uiReadLength);
//		usleep(1*1000);

		if(uiReadLength == 0)
		{
			for(i=0; i<BCAS_SEND_RETRY_CNT; i++)
			{
				err = TCC_DxB_SC_ReadData(0, ucWriteBuf, uiWriteLength, ucReadBuf, &uiReadLength);
				if(uiReadLength != 0)
					break;
			}
		}

#if 1
//		if(ins == SC_INS_PARAM_EMG)
		if(uiReadLength  == 4)
		{
			unsigned int i;
			ALOGE("%s %d ucReadBuf  start \n", __func__, __LINE__);

			for (i=0; i<uiReadLength; i++)
			{
				ALOGE("  i =%d, ucReadBuf = %x ", i,  ucReadBuf[i]);
			}
			ALOGE("%s %d uiReadLength = %d \n", __func__, __LINE__, uiReadLength);
		}
#endif	

		SendBlock = uiBlockPCBValue;

		if(uiReadLength != 0)
			err = TCC_DxB_SC_Manager_ReceiveErrCheck(ucReadBuf);
		edc_lrc = TCC_Dxb_SC_Manager_DefSetEDC(EDC_TYPE_LRC);

		if(uiReadLength == 0 || err != SC_ERR_OK)
		{

			ALOGE("%s %d ins=%x, err = %d, err_count = %d,  uiReadLength = %d, SendBlock = %x \n", __func__, __LINE__, ins,  err, err_count,uiReadLength, SendBlock);

			if(err_count==BCAS_MAX_ERROR_COUNT)
			{
				uiBlockPCBValue = SC_BLOCK_DEF_SPCB;
				SBlock_Control_Type = SC_SBLOCK_CON_RES;
				SBlock_Block_Type = SC_SBLOCK_BLOCK_TYPE_REQ_MASK;
			}
			else if(err_count > BCAS_MAX_ERROR_COUNT)
			{
				TCC_Dxb_SC_Manager_InitParam();
				return err;
			}
			else 
				uiBlockPCBValue = SC_BLOCK_DEF_RPCB;

			err_count ++;
		}	
		else
		{
			ReceiveBlock = TCC_DxB_SC_Manager_ReceiveBlockCheck(ucReadBuf);
	//		ALOGE("%s %d ReceiveBlock = %d \n", __func__, __LINE__, ReceiveBlock);

			if (ReceiveBlock == BCAS_IBLCOK || ReceiveBlock == BCAS_IBLCOK_MODREDATA)
			{
				if(APDU_RES_LENGTH == 0 && ReceiveBlock == BCAS_IBLCOK_MODREDATA )
				{
					APDU_RES_LENGTH = ucReadBuf[INITFIELD_LEN_POS];
					memcpy(ucReadDataBuf, &ucReadBuf[INFOFIELD_INF_ST_POS], APDU_RES_LENGTH);
		//			ALOGE("%s %d APDU_RES_LENGTH = %d \n", __func__, __LINE__, APDU_RES_LENGTH);
				}
				else
				{
					if(ucReadBuf[INFOFIELD_INF_ST_POS] != 0)		//protocol num check
					{
						memcpy(&ucReadDataBuf[APDU_RES_LENGTH], &ucReadBuf[INFOFIELD_INF_ST_POS], ucReadBuf[INITFIELD_LEN_POS]);
						APDU_RES_LENGTH += ucReadBuf[INITFIELD_LEN_POS];
		//				ALOGE("%s %d APDU_RES_LENGTH = %d \n", __func__, __LINE__, APDU_RES_LENGTH);
					}
					else
					{	//In case of red B-CAS, send a part of response at first and send all resonse (including data which already sent) at second time
						APDU_RES_LENGTH = ucReadBuf[INFOFIELD_INF_ST_POS+1] + 4;	//unit_length + 4 byte (data field header + SW1,2)
						memcpy(ucReadDataBuf, &ucReadBuf[INFOFIELD_INF_ST_POS], APDU_RES_LENGTH);
		//				ALOGE("%s %d APDU_RES_LENGTH = %d \n", __func__, __LINE__, APDU_RES_LENGTH);
					}

				}
			}

			if(ReceiveBlock != BCAS_IBLCOK)
			{
		//		ALOGE("%s %d ReceiveBlock = %d \n", __func__, __LINE__, ReceiveBlock);

				switch(ReceiveBlock)
				{
					case BCAS_IBLCOK_MODREDATA:
						uiBlockPCBValue = SC_BLOCK_DEF_RPCB;
						uiSeqNumber ^= 0x40;
						Seq_Num = uiSeqNumber;
						break;	
					
					case BCAS_RBLCOK:
						if(SendBlock == SC_BLOCK_DEF_IPCB)
						{
							uiSeqNumber ^= 0x40;
							Seq_Num = uiSeqNumber;
						}
						uiBlockPCBValue = SC_BLOCK_DEF_IPCB;

						if(err_count==BCAS_MAX_ERROR_COUNT)
						{
							uiBlockPCBValue = SC_BLOCK_DEF_SPCB;
							SBlock_Control_Type = SC_SBLOCK_CON_RES;
							SBlock_Block_Type = SC_SBLOCK_BLOCK_TYPE_REQ_MASK;
						}
						else if(err_count > BCAS_MAX_ERROR_COUNT)
						{
							TCC_Dxb_SC_Manager_InitParam();
							return err;
						}
						ALOGE("%s %d ins = 0x%x err_count=%d , uiBlockPCBValue = %x \n", __func__, __LINE__, ins, err_count,uiBlockPCBValue);

						err_count ++;
						
						break;
					
					case BCAS_SBLCOK_RESPONSE:
						uiBlockPCBValue = SC_BLOCK_DEF_IPCB;
						break;

					case BCAS_SBLCOK_IFS_REQ:
						uiBlockPCBValue = SC_BLOCK_DEF_SPCB;
						SBlock_Control_Type = SC_SBLOCK_CON_IFS;
						SBlock_Block_Type = SC_SBLOCK_BLOCK_TYPE_REPS_MASK;
							break;
				
					case BCAS_SBLCOK_WTX_REQ:
						uiBlockPCBValue = SC_BLOCK_DEF_SPCB;
						SBlock_Control_Type = SC_SBLOCK_CON_WTX;
						SBlock_Block_Type = SC_SBLOCK_BLOCK_TYPE_REPS_MASK;
						break;

					case BCAS_SBLCOK_RESYNC_REQ:
						uiBlockPCBValue = SC_BLOCK_DEF_SPCB;
						SBlock_Control_Type = SC_SBLOCK_CON_RES;
						SBlock_Block_Type = SC_SBLOCK_BLOCK_TYPE_REPS_MASK;
						break;

				}
			}
			else
			{
				data += temp_data_len;
				data_len -=temp_data_len;
					
				if(SendBlock != SC_BLOCK_DEF_RPCB)
				{
					uiSeqNumber ^= 0x40;
					Seq_Num = uiSeqNumber;
				}
	//			ALOGE("%s %d data_len = %d \n", __func__, __LINE__, data_len);

				if (data_len <= 0)
				{
					err = TCC_DxB_SC_Manager_ProcessResponse(ins, ucReadDataBuf);
					if(err != SC_ERR_OK)
					{	
						ALOGE("%s %d ins= %x, err = %d \n", __func__, __LINE__, ins, err);
						return err;
					}	
					break;
				}
			}
		}
	}		

	return err;	
}

/************************************************************************************************************
* FUNCTION NAME : TCC_DxB_SC_Manager_EMM_Section_Parsing
* DESCRIPTION :  EMM section header parsing
* INPUT	 :  data_len:data length  , data : command data(EMM data)
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
int	TCC_DxB_SC_Manager_EMM_SectionParsing(int data_len, char *data)
{
	int err = SC_ERR_OK;
	unsigned int	uiPayload_len =0;
	unsigned int	uisend_Cnt=0;

	uiPayload_len = *(data + BCAS_CARDID_LEGTH);
	if(uiPayload_len<((unsigned int)data_len - BCAS_CARDID_LEGTH - BCAS_INFO_BYTE_LEN))
	{
	//	ALOGE("%s %d Multi Payload EMM data : [data_len = %d] \n", __func__, __LINE__, data_len);
#if 1
		return SC_ERR_OK;
#else
		while(1)
		{
			err = TCC_DxB_SC_Manager_SendT1_Glue(SC_INS_PARAM_EMM, (uiPayload_len + BCAS_CARDID_LEGTH + BCAS_INFO_BYTE_LEN ), (char*)(data + uisend_Cnt));
			uisend_Cnt += (uiPayload_len + BCAS_CARDID_LEGTH + BCAS_INFO_BYTE_LEN );
			if(uisend_Cnt>=data_len)
				break;
			uiPayload_len = *(data + BCAS_CARDID_LEGTH + uisend_Cnt);
		}
#endif
	}
	else
	{
		err = TCC_DxB_SC_Manager_SendT1_Glue(SC_INS_PARAM_EMM, data_len,(char*)(data));
	}

	return err;
}

/************************************************************************************************************
* FUNCTION NAME : TCC_DxB_SC_Manager_EMMmessage_Section_Parsing
* DESCRIPTION :  EMM Message section header parsing(identify EMM type(ex: EMM common messge, EMM individual mmessage, .)
* INPUT	 :  data_len:data length  , data : command data(EMM data)
* OUTPUT  :  
* REMARK  : 
*************************************************************************************************************/
int	TCC_DxB_SC_Manager_EMM_MessageSectionParsing(int data_len, char *data)
{
	int	ret = EMM_Message_Err_MAX, err = SC_ERR_OK;
	unsigned int	i=0, j, field_offset =0;
	EMM_Individual_Message_Field_t		*IndiMessage = NULL;
	EMM_Individual_Message_Field_t		*Prev_IndiMessage = NULL;
	unsigned int	indiMessage_cnt =0;
	unsigned int	playload_cnt = 0;

 	EMM_MessSection_Header.ucTable_Id = *(data);
	EMM_MessSection_Header.ucSection_Syntax_Indicator = 0x01&(*(data+1)>>7);
	EMM_MessSection_Header.ucPrivate_Indicator = 0x01&(*(data+1)>>6);
	EMM_MessSection_Header.ucReserved1 = 0x03&(*(data+1)>>4);
	EMM_MessSection_Header.uiSection_len = ((0x0f&(*(data+1)))<<8) |*(data+2) ;
	EMM_MessSection_Header.uiTable_Id_Extension = ((*(data+3))<<8) |*(data+4) ;
	EMM_MessSection_Header.ucReserved2 = 0x03&(*(data+5)>>6) ;
	EMM_MessSection_Header.ucVersionNum = 0x1f&(*(data+5)>>1) ;
	EMM_MessSection_Header.ucCurr_Next_Indicator = 0x01&(*(data+5));
	EMM_MessSection_Header.ucSectionNum = *(data+6);
	EMM_MessSection_Header.ucLastSectionNum = *(data+7);

	if(EMM_MessSection_Header.uiTable_Id_Extension == ISDBT_EMM_INDIVIDUAL_MESSAGE_TABLEID_EXTENSION)
	{	
		if(EMM_IndiMessage_Cnt>BCAS_MAX_EMM_COM_MESSAGE_CNT)
		{	
			TCC_Dxb_SC_Manager_EMM_IndiMessageDeInit(1);
			indiMessage_cnt = EMM_IndiMessage_Cnt =0;
		}
		else
			indiMessage_cnt = EMM_IndiMessage_Cnt;

		EMM_IndiMessage[indiMessage_cnt].EMM_MessSection_Header.ucTable_Id = EMM_MessSection_Header.ucTable_Id;
		EMM_IndiMessage[indiMessage_cnt].EMM_MessSection_Header.ucSection_Syntax_Indicator = EMM_MessSection_Header.ucSection_Syntax_Indicator;
		EMM_IndiMessage[indiMessage_cnt].EMM_MessSection_Header.ucPrivate_Indicator = EMM_MessSection_Header.ucPrivate_Indicator;
		EMM_IndiMessage[indiMessage_cnt].EMM_MessSection_Header.ucReserved1 = EMM_MessSection_Header.ucReserved1;
		EMM_IndiMessage[indiMessage_cnt].EMM_MessSection_Header.uiSection_len = EMM_MessSection_Header.uiSection_len;
		EMM_IndiMessage[indiMessage_cnt].EMM_MessSection_Header.uiTable_Id_Extension = EMM_MessSection_Header.uiTable_Id_Extension;
		EMM_IndiMessage[indiMessage_cnt].EMM_MessSection_Header.ucReserved2 = EMM_MessSection_Header.ucReserved2;
		EMM_IndiMessage[indiMessage_cnt].EMM_MessSection_Header.ucVersionNum = EMM_MessSection_Header.ucVersionNum;
		EMM_IndiMessage[indiMessage_cnt].EMM_MessSection_Header.ucCurr_Next_Indicator = EMM_MessSection_Header.ucCurr_Next_Indicator;
		EMM_IndiMessage[indiMessage_cnt].EMM_MessSection_Header.ucSectionNum = EMM_MessSection_Header.ucSectionNum;
		EMM_IndiMessage[indiMessage_cnt].EMM_MessSection_Header.ucLastSectionNum = EMM_MessSection_Header.ucLastSectionNum;

		EMM_IndiMessage[indiMessage_cnt].uiEMM_IndividualMessage_PayloadCnt = 1;
#if 0
		ALOGE("EMM INDIVIDUAL MESSAGE Parsing \n");
		ALOGE("EMM_IndiMessage.EMM_MessSection_Header.ucTable_Id  = %x \n", EMM_IndiMessage[indiMessage_cnt].EMM_MessSection_Header.ucTable_Id );
		ALOGE("EMM_IndiMessage.EMM_MessSection_Header.ucSection_Syntax_Indicator  = %x \n", EMM_IndiMessage[indiMessage_cnt].EMM_MessSection_Header.ucSection_Syntax_Indicator );
		ALOGE("EMM_IndiMessage.EMM_MessSection_Header.ucPrivate_Indicator  = %x \n", EMM_IndiMessage[indiMessage_cnt].EMM_MessSection_Header.ucPrivate_Indicator );
		ALOGE("EMM_IndiMessage.EMM_MessSection_Header.uiSection_len  = %x \n", EMM_IndiMessage[indiMessage_cnt].EMM_MessSection_Header.uiSection_len );
		ALOGE("EMM_IndiMessage.EMM_MessSection_Header.uiTable_Id_Extension  = %x \n", EMM_IndiMessage[indiMessage_cnt].EMM_MessSection_Header.uiTable_Id_Extension );
		ALOGE("EMM_IndiMessage.EMM_MessSection_Header.ucVersionNum  = %x \n", EMM_IndiMessage[indiMessage_cnt].EMM_MessSection_Header.ucVersionNum );
		ALOGE("EMM_IndiMessage.EMM_MessSection_Header.ucCurr_Next_Indicator  = %x \n", EMM_IndiMessage[indiMessage_cnt].EMM_MessSection_Header.ucCurr_Next_Indicator );
		ALOGE("EMM_IndiMessage.EMM_MessSection_Header.ucSectionNum  = %x \n", EMM_IndiMessage[indiMessage_cnt].EMM_MessSection_Header.ucSectionNum );
		ALOGE("EMM_IndiMessage.EMM_MessSection_Header.ucLastSectionNum  = %x \n", EMM_IndiMessage[indiMessage_cnt].EMM_MessSection_Header.ucLastSectionNum );
#endif
		field_offset += ISDBT_EMM_SECTON_HEAER_LENGTH;		//8

		for(i=1; i<EMM_IndiMessage[indiMessage_cnt].EMM_MessSection_Header.uiSection_len; )
		{
/* Original		
			IndiMessage = (EMM_Individual_Message_Field_t *)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(EMM_Individual_Message_Field_t));
			IndiMessage->Prev_EMM_IndiMessage =  NULL;
*/
//10th Try
//David, To avoid Codesonar's warning, NULL Pointer Dereference
//add code for handling exception
			IndiMessage = (EMM_Individual_Message_Field_t *)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(EMM_Individual_Message_Field_t));
			if(IndiMessage == NULL)
			{
				ALOGE("[%s:%d] EMM_Individual_Message_Field - malloc fail : 0x%x\n", __func__, __LINE__, IndiMessage);
				return EMM_Message_Parsing_Err;
			}
			else
			{
				IndiMessage->Prev_EMM_IndiMessage =  NULL;
			}
#ifdef EMM_MALLOC_DEBUGING		//EMM malloc size debuging
			malloc_size += (sizeof(EMM_IndiMessage[indiMessage_cnt].EMM_IndividualMessage_Field));
#endif
			if(Prev_IndiMessage == NULL)
			{
				EMM_IndiMessage[indiMessage_cnt].EMM_IndividualMessage_Field = IndiMessage;
			}
			else
			{
				Prev_IndiMessage->Prev_EMM_IndiMessage = IndiMessage;
			}

			for(j=0; j<BCAS_CARDID_LEGTH; j++)
				IndiMessage->ucCard_ID[j] = *(data+field_offset + j);
			field_offset += BCAS_CARDID_LEGTH;	//14
			IndiMessage->uiMessage_Len = (*(data+field_offset)<< 8) | *(data+field_offset +1);
			field_offset += 2;	//16
			IndiMessage->ucProtocol_Num = *(data+field_offset);
			field_offset ++;		//17			
			IndiMessage->ucCa_Broadcaster_Gr_ID = *(data+field_offset);
			field_offset ++;		//18
			IndiMessage->ucMessage_ID = *(data+field_offset);
			field_offset ++;		//19
			IndiMessage->ucMessage_Control = *(data+field_offset);
			field_offset ++;		//20

			if(IndiMessage->uiMessage_Len>EMM_IndiMessage[indiMessage_cnt].EMM_MessSection_Header.uiSection_len)
			{
				ALOGE("%s %d  EMM Individual Message Parsing Error [uiSection_len = %d][uiMessage_Len = %d]\n", __func__, __LINE__, EMM_IndiMessage[indiMessage_cnt].EMM_MessSection_Header.uiSection_len, IndiMessage->uiMessage_Len);
				{
					int k=0;
					for(k=0; k<EMM_IndiMessage[indiMessage_cnt].EMM_MessSection_Header.uiSection_len; k+=8)
					{	
						ALOGE("[%d]EMM_IndiMessage[0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x] \n", __LINE__, 
							*(data + k), *(data + k+1), *(data + k +2), *(data + k+3), *(data + k+4), *(data + k+5), *(data + k+6), *(data + k+7));
					}
				}

				IndiMessage->uiMessage_Len =0;
				break;
			}
			
			if(IndiMessage->uiMessage_Len>ISDBT_EMM_INDI_MESSAGE_DEFAULT_FIELD_LEN)
			{	
				memset(IndiMessage->ucMessage_Area, 0, ISDBT_EMM_MAX_MAIL_SIZE);
				for(j=0; j<(unsigned int)(IndiMessage->uiMessage_Len - ISDBT_EMM_INDI_MESSAGE_DEFAULT_FIELD_LEN); j++)
				{
					IndiMessage->ucMessage_Area[j] = *(data + field_offset + j);
				}	
				field_offset +=(IndiMessage->uiMessage_Len - ISDBT_EMM_INDI_MESSAGE_DEFAULT_FIELD_LEN);
			}
			i= field_offset +1;

//			ALOGE("%s %d IndiMessage = %x , Prev_IndiMessage = %x, Prev_EMM_IndiMessage = %x,  \n", __func__, __LINE__, IndiMessage, Prev_IndiMessage,IndiMessage->Prev_EMM_IndiMessage);
//			ALOGE("%s %d uiSection_len = %d, uiMessage_Len = %d , i = %d, field_offset = %d \n", __func__, __LINE__, EMM_IndiMessage[indiMessage_cnt].EMM_MessSection_Header.uiSection_len,IndiMessage->uiMessage_Len, i, field_offset);
			
			if(i<EMM_IndiMessage[indiMessage_cnt].EMM_MessSection_Header.uiSection_len)
			{
				Prev_IndiMessage = IndiMessage;
				EMM_IndiMessage[indiMessage_cnt].uiEMM_IndividualMessage_PayloadCnt ++;
			}
			else
				EMM_IndiMessage[indiMessage_cnt].uiEMM_Message_Section_CRC  = (*(data+field_offset)<<24) | (*(data+field_offset + 1)<<16)
									|(*(data+ field_offset + 2)<<8) |(*(data+field_offset + 3));
		}

//		ALOGE("[%d] indiMessage_cnt = %d, EMM_IndiMessage.uiEMM_IndividualMessage_PayloadCnt  = %d \n",__LINE__, indiMessage_cnt, EMM_IndiMessage[indiMessage_cnt].uiEMM_IndividualMessage_PayloadCnt);
		IndiMessage = EMM_IndiMessage[indiMessage_cnt].EMM_IndividualMessage_Field;
		playload_cnt = EMM_IndiMessage[indiMessage_cnt].uiEMM_IndividualMessage_PayloadCnt;
		
		for(i=0;i<playload_cnt; i++)
		{
#if 1		
			if(TCC_Dxb_SC_Manager_EMM_MessageCardIDCheck(IndiMessage))
			{
				ALOGE("EMM_IndiMessage.EMM_IndividualMessage_Field.ucCard_ID  = [%x,%x,%x,%x,%x,%x ]\n", IndiMessage->ucCard_ID[0]
					, IndiMessage->ucCard_ID[1], IndiMessage->ucCard_ID[2]
					, IndiMessage->ucCard_ID[3], IndiMessage->ucCard_ID[4]
					, IndiMessage->ucCard_ID[5]);
				
				ALOGE("EMM_IndiMessage.EMM_IndividualMessage_Field.uiMessage_Len  = %d \n", IndiMessage->uiMessage_Len );
				ALOGE("EMM_IndiMessage.EMM_IndividualMessage_Field.ucProtocol_Num  = %d \n", IndiMessage->ucProtocol_Num );
				ALOGE("EMM_IndiMessage.EMM_IndividualMessage_Field.ucCa_Broadcaster_Gr_ID  = %d \n", IndiMessage->ucCa_Broadcaster_Gr_ID );
				ALOGE("EMM_IndiMessage.EMM_IndividualMessage_Field.ucMessage_ID  = %d \n", IndiMessage->ucMessage_ID );
				ALOGE("EMM_IndiMessage.EMM_IndividualMessage_Field.ucMessage_Control  = %d \n", IndiMessage->ucMessage_Control );

				if((IndiMessage->uiMessage_Len - ISDBT_EMM_INDI_MESSAGE_DEFAULT_FIELD_LEN)>0)
				{
#if 0
					ALOGE("EMM_IndiMessage.EMM_IndividualMessage_Field.ucMessage_Area\n");
					for(j=0;  j<(IndiMessage->uiMessage_Len - ISDBT_EMM_INDI_MESSAGE_DEFAULT_FIELD_LEN -7);j+=8)
					{
						ALOGE("[%d]EMM_IndividualMessage_Field.ucMessage_Area  = [0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x ]\n", __LINE__, IndiMessage->ucMessage_Area[j]
							, IndiMessage->ucMessage_Area[j+1], IndiMessage->ucMessage_Area[j+2], IndiMessage->ucMessage_Area[j+3], IndiMessage->ucMessage_Area[j+4]
							, IndiMessage->ucMessage_Area[j+5], IndiMessage->ucMessage_Area[j+6], IndiMessage->ucMessage_Area[j+7]);
					}
#endif
				}
				ALOGE("EMM_IndiMessage.EMM_IndividualMessage_Field.Next_EMM_IndiMessage  = %p \n", IndiMessage->Prev_EMM_IndiMessage );
			}
#endif

			if(IndiMessage->ucProtocol_Num == ISDBT_EMM_INDIIDAUL_MESSAGE_ENCRYPT_CHECK) //no encryption
			{
				if(IndiMessage->ucMessage_Control == EMM_COMMESSAGE_STORED_DIRD)
				{
					if(TCC_Dxb_SC_Manager_EMM_MessageCardIDCheck(IndiMessage))
					{
		//				TCC_Dxb_SC_Manager_EMM_DispMessageInit();
						EMM_Message_Receive_status.uiMessageType = EMM_MAIL_MESSAGE;
						EMM_Message_Receive_status.uiEncrypted_status = EMM_INDIMESSAGE_UNENCRYPTED;
						Message_ID = IndiMessage->ucMessage_ID;
						ret = EMM_Message_Individual_Message;
						ALOGE("1. EMM Received Unencrypted Mail Message \n");

						EMM_MailMessage_CodeResion.uiReserved1= (IndiMessage->ucMessage_Area[0]<<8) |IndiMessage->ucMessage_Area[1];
						EMM_MailMessage_CodeResion.uiReserved2 = (IndiMessage->ucMessage_Area[2]<<8) |IndiMessage->ucMessage_Area[3];
						EMM_MailMessage_CodeResion.uiFixed_Mmessage_ID = (IndiMessage->ucMessage_Area[4]<<8) |IndiMessage->ucMessage_Area[5];
						EMM_MailMessage_CodeResion.ucExtraMessage_FormatVersion = IndiMessage->ucMessage_Area[6];
						EMM_MailMessage_CodeResion.uiExtra_Message_Len = (IndiMessage->ucMessage_Area[7]<<8) |IndiMessage->ucMessage_Area[8];

						ALOGE("[Unencrypted Mai] EMM_MailMessage_CodeResion.uiExtra_Message_Len = %x \n",  EMM_MailMessage_CodeResion.uiExtra_Message_Len);

						if(EMM_MailMessage_CodeResion.uiExtra_Message_Len>0)
						{	
							memset(EMM_MailMessage_CodeResion.ucExtra_MessageCode, 0, sizeof(EMM_MailMessage_CodeResion.ucExtra_MessageCode));
							memcpy(EMM_MailMessage_CodeResion.ucExtra_MessageCode , 
								&IndiMessage->ucMessage_Area[ISDBT_EMM_MESSAGECODE_EXTRA_MESSAGE_CODE_POSITION], 
								EMM_MailMessage_CodeResion.uiExtra_Message_Len);
#if 1
							ALOGE("[Unencrypted Mai] EMM_Message_CodeResion.uiReserved1 = %x \n",  EMM_MailMessage_CodeResion.uiReserved1);
							ALOGE("[Unencrypted Mai] EMM_Message_CodeResion.uiReserved2 = %x \n",  EMM_MailMessage_CodeResion.uiReserved2);
							ALOGE("[Unencrypted Mai] EMM_Message_CodeResion.uiFixed_Mmessage_ID = %x \n",  EMM_MailMessage_CodeResion.uiFixed_Mmessage_ID);
							ALOGE("[Unencrypted Mai] EMM_Message_CodeResion.ucExtraMessage_FormatVersion = %x \n",  EMM_MailMessage_CodeResion.ucExtraMessage_FormatVersion);
							ALOGE("[Unencrypted Mai] EMM_Message_CodeResion.uiExtra_Message_Len = %x \n",  EMM_MailMessage_CodeResion.uiExtra_Message_Len);
		#if 0
							if(EMM_MailMessage_CodeResion.uiExtra_Message_Len>0)
							{
								for(j=0; j<(EMM_MailMessage_CodeResion.uiExtra_Message_Len-3); j+=4)
								{
									ALOGE("EMM_MailMessage_CodeResion.ucExtra_MessageCode[%d] = [%x, %x, %x, %x] \n", j, EMM_MailMessage_CodeResion.ucExtra_MessageCode[j],
										EMM_MailMessage_CodeResion.ucExtra_MessageCode[j+1],EMM_MailMessage_CodeResion.ucExtra_MessageCode[j+2],
										EMM_MailMessage_CodeResion.ucExtra_MessageCode[j+3]);
								}
							}
		#endif					
#endif
						}
						ISDBT_MESSAGE_TYPE = EMM_MAIL_MESSAGE;

						if(EMM_MailMessage_CodeResion.uiFixed_Mmessage_ID == ISDBT_MAIL_NO_PRESET_MESSAGE)
							ret = TCC_Dxb_SC_Manager_EMM_MessageInfoSet();
						
					}	
				}	
				else
					ret =  EMM_Message_Parsing_Err;
			}
			else	// encrypted
			{
				if(TCC_Dxb_SC_Manager_EMM_MessageCardIDCheck(IndiMessage))
				{
		//			TCC_Dxb_SC_Manager_EMM_DispMessageInit();
					if(IndiMessage->ucMessage_Control == EMM_COMMESSAGE_STORED_ICCARD)
					{	
						EMM_Message_Receive_status.uiMessageType =  EMM_AUTODISPLAY_MESSAGE;
						EMM_Message_Receive_status.uiEncrypted_status = EMM_INDIMESSAGE_ENCRYPTED;
						ret = EMM_Message_Individual_Message;
						ALOGE("EMM Received AUTODISPLAY MESSAGE \n");
					}	
					else if(IndiMessage->ucMessage_Control == EMM_COMMESSAGE_STORED_DIRD)
					{
						EMM_Message_Receive_status.uiMessageType =  EMM_MAIL_MESSAGE;
						EMM_Message_Receive_status.uiEncrypted_status = EMM_INDIMESSAGE_ENCRYPTED;
						ret = EMM_Message_Individual_Message;
						ALOGE("2. EMM Received Encrypted Mail Message \n");
					}	
					else
					{	
						ret =  EMM_Message_Parsing_Err;
						EMM_Message_Receive_status.uiMessageType  = EMM_BASIC;
						EMM_Message_Receive_status.uiEncrypted_status = -1;
						ISDBT_MESSAGE_TYPE = EMM_BASIC;
					}
					
					if((IndiMessage->uiMessage_Len - ISDBT_EMM_INDI_MESSAGE_DEFAULT_FIELD_LEN)>0)
					{	
						Broadcaster_GroupID = IndiMessage->ucCa_Broadcaster_Gr_ID;
						Message_ID = IndiMessage->ucMessage_ID;
						err = TCC_Dxb_SC_Manager_EMM_IndiMessageReciveCmd(IndiMessage, (IndiMessage->uiMessage_Len - ISDBT_EMM_INDI_MESSAGE_DEFAULT_FIELD_LEN));
					}

					if(err == SC_ERR_OK)
					{	
						if(EMM_Message_Receive_status.uiMessageType ==  EMM_AUTODISPLAY_MESSAGE)
						{	
							ISDBT_MESSAGE_TYPE = EMM_AUTODISPLAY_MESSAGE;
						}	
						else if(EMM_Message_Receive_status.uiMessageType == EMM_MAIL_MESSAGE)
						{
							ISDBT_MESSAGE_TYPE = EMM_MAIL_MESSAGE;
						}	
					}

				}
			}
			if(IndiMessage->Prev_EMM_IndiMessage != NULL)
			{	
				IndiMessage = IndiMessage->Prev_EMM_IndiMessage;
		//		ALOGE("EMM Individual Message Payload Change: IndiMessage = %x, i= %d,  indiMessage_cnt = %d, PayloadCnt= %d, uiEMM_IndividualMessage_PayloadCnt = %d \n", IndiMessage, i, indiMessage_cnt, playload_cnt,EMM_IndiMessage[indiMessage_cnt].uiEMM_IndividualMessage_PayloadCnt);
			}
			else
				break;
		}
		EMM_IndiMessage[indiMessage_cnt].uiEMM_IndividualMessage_PayloadCnt = playload_cnt;

		ALOGE("[%d] err = %x, ret = %d \n", __LINE__, err, ret );
//		ALOGE("[%d] EMM_IndiMessage.uiEMM_Message_Section_CRC  = %x \n", __LINE__, EMM_IndiMessage[indiMessage_cnt].uiEMM_Message_Section_CRC);
//		ALOGE("[%d] EMM indiMessage_cnt = %d, PayloadCnt= %d, uiEMM_IndividualMessage_PayloadCnt = %d \n",__LINE__,  indiMessage_cnt, playload_cnt,EMM_IndiMessage[indiMessage_cnt].uiEMM_IndividualMessage_PayloadCnt);
		ALOGE("[%d] EMM EMM_Message_Receive_status.uiMessageType= %d \n", __LINE__, EMM_Message_Receive_status.uiMessageType);

		if(ISDBT_MESSAGE_TYPE != EMM_BASIC)
		{	
			if(EMM_IndiMessage_Cnt<BCAS_MAX_EMM_COM_MESSAGE_CNT)
				EMM_IndiMessage_Cnt ++;
		}
	}
	else		//EMM Common Mmessage
	{
//		ALOGE("Received EMM Common Message\n");
		if(EMM_ComMessage_Cnt>BCAS_MAX_EMM_COM_MESSAGE_CNT)
		{	
			EMM_ComMessage_Cnt =0;
		}

		EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.ucTable_Id = EMM_MessSection_Header.ucTable_Id;
		EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.ucSection_Syntax_Indicator = EMM_MessSection_Header.ucSection_Syntax_Indicator;
		EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.ucPrivate_Indicator = EMM_MessSection_Header.ucPrivate_Indicator;
		EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.ucReserved1 = EMM_MessSection_Header.ucReserved1;
		EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.uiSection_len = EMM_MessSection_Header.uiSection_len;
		EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.uiTable_Id_Extension = EMM_MessSection_Header.uiTable_Id_Extension;
		EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.ucReserved2 = EMM_MessSection_Header.ucReserved2;
		EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.ucVersionNum = EMM_MessSection_Header.ucVersionNum;
		EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.ucCurr_Next_Indicator = EMM_MessSection_Header.ucCurr_Next_Indicator;
		EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.ucSectionNum = EMM_MessSection_Header.ucSectionNum;
		EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.ucLastSectionNum = EMM_MessSection_Header.ucLastSectionNum;

		field_offset = 0;
		if(EMM_MessSection_Header.uiSection_len>ISDBT_EMM_SECTON_DEFAULT_FIELD_LEN)
		{	
			field_offset += ISDBT_EMM_SECTON_HEAER_LENGTH;
			EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucBroadcaster_GroupID = *(data+field_offset);
			field_offset ++;
			EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucDeletion_Status = *(data+field_offset);
			field_offset ++;
			EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucDisp_Duration1 = *(data+field_offset);
			field_offset ++;
			EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucDisp_Duration2 = *(data+field_offset);
			field_offset ++;
			EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucDisp_Duration3 = *(data+field_offset);
			field_offset ++;
			EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucDisp_Cycle = *(data+field_offset);
			field_offset ++;
			EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucFormat_Version = *(data+field_offset);
			field_offset ++;
			EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.uiMessageLen = (*(data+field_offset)<< 8) | *(data+field_offset +1);
			field_offset += 2;

			if(BCAS_Auto_Disp_MessageArch.Differential_information != NULL)
				TCC_Dxb_SC_Manager_EMM_ComMessageDeInit();

			memset(EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area, 0, ISDBT_EMM_MAX_MAIL_SIZE);

			if(EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.uiMessageLen >0)
			{	
				for(i=0; i<EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.uiMessageLen; i++)
				{
					EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i] = *(data+field_offset+ i);
				}	
				field_offset += EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.uiMessageLen;
				EMM_ComMessage[EMM_ComMessage_Cnt].uiEMM_Message_Section_CRC  = (*(data+field_offset)<<24) | (*(data+field_offset + 1)<<16)
													|(*(data+ field_offset + 2)<<8) |(*(data+field_offset + 3));
			}
			else
				EMM_ComMessage[EMM_ComMessage_Cnt].uiEMM_Message_Section_CRC  = (*(data+ISDBT_EMM_SECTON_HEAER_LENGTH + 9)<<24) | (*(data+ISDBT_EMM_SECTON_HEAER_LENGTH + 10)<<16)
													|(*(data+ISDBT_EMM_SECTON_HEAER_LENGTH + 11)<<8) |(*(data+ISDBT_EMM_SECTON_HEAER_LENGTH + 12));

//		ALOGE("[%d][EMM_ComMessage_Cnt=%d ],  ucDeletion_Status  = %x \n", __LINE__, EMM_ComMessage_Cnt, EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucDeletion_Status );
//		ALOGE("EMM_ComMessage.EMM_MessSection_Header.uiTable_Id_Extension  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.uiTable_Id_Extension );

#if 0
		ALOGE("EMM COMMON MESSAGE Parsing \n");
		ALOGE("EMM_ComMessage.EMM_MessSection_Header.ucTable_Id  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.ucTable_Id );
		ALOGE("EMM_ComMessage.EMM_MessSection_Header.ucSection_Syntax_Indicator  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.ucSection_Syntax_Indicator );
		ALOGE("EMM_ComMessage.EMM_MessSection_Header.ucPrivate_Indicator  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.ucPrivate_Indicator );
		ALOGE("EMM_ComMessage.EMM_MessSection_Header.uiSection_len  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.uiSection_len );
		ALOGE("EMM_ComMessage.EMM_MessSection_Header.uiTable_Id_Extension  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.uiTable_Id_Extension );
		ALOGE("EMM_ComMessage.EMM_MessSection_Header.ucVersionNum  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.ucVersionNum );
		ALOGE("EMM_ComMessage.EMM_MessSection_Header.ucCurr_Next_Indicator  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.ucCurr_Next_Indicator );
		ALOGE("EMM_ComMessage.EMM_MessSection_Header.ucSectionNum  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.ucSectionNum );
		ALOGE("EMM_ComMessage.EMM_MessSection_Header.ucLastSectionNum  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.ucLastSectionNum );

		ALOGE("EMM_ComMessage.EMM_ComMessage_Field.ucBroadcaster_GroupID  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucBroadcaster_GroupID );
		ALOGE("EMM_ComMessage.EMM_ComMessage_Field.ucDeletion_Status  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucDeletion_Status );
		ALOGE("EMM_ComMessage.EMM_ComMessage_Field.ucDisp_Duration1  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucDisp_Duration1 );
		ALOGE("EMM_ComMessage.EMM_ComMessage_Field.ucDisp_Duration2 = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucDisp_Duration2 );
		ALOGE("EMM_ComMessage.EMM_ComMessage_Field.ucDisp_Duration3  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucDisp_Duration3 );
		ALOGE("EMM_ComMessage.EMM_ComMessage_Field.ucDisp_Cycle  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucDisp_Cycle );
		ALOGE("EMM_ComMessage.EMM_ComMessage_Field.ucFormat_Version  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucFormat_Version );
		ALOGE("EMM_ComMessage.EMM_ComMessage_Field.uiMessageLen  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.uiMessageLen );
//		ALOGE("EMM_ComMessage.uiEMM_Message_Section_CRC  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].uiEMM_Message_Section_CRC);
#endif
			ret = TCC_Dxb_SC_Manager_EMM_MessageInfoSet();

//			ALOGE("%s %d ret = %d\n", __func__, __LINE__, ret) ;
			if(ret != EMM_Message_Get_Success)
			{	
				return ret;
			}
			
			if(EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucDeletion_Status == EMM_DSTATUS_ERASE)
				ret =  EMM_AUTODISPLAY_MESSAGE;
		}
		else
		{	
			EMM_ComMessage[EMM_ComMessage_Cnt].uiEMM_Message_Section_CRC  = (*(data+ISDBT_EMM_SECTON_HEAER_LENGTH)<<24) | (*(data+ISDBT_EMM_SECTON_HEAER_LENGTH + 1)<<16)
													|(*(data+ISDBT_EMM_SECTON_HEAER_LENGTH + 2)<<8) |(*(data+ISDBT_EMM_SECTON_HEAER_LENGTH+ 3));
			ret = EMM_Message_Get_Fail;
		}

		ALOGE("EMM_ComMessage.EMM_MessSection_Header.uiTable_Id_Extension  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.uiTable_Id_Extension );

#if 0
		ALOGE("EMM COMMON MESSAGE Parsing \n");
		ALOGE("EMM_ComMessage.EMM_MessSection_Header.ucTable_Id  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.ucTable_Id );
		ALOGE("EMM_ComMessage.EMM_MessSection_Header.ucSection_Syntax_Indicator  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.ucSection_Syntax_Indicator );
		ALOGE("EMM_ComMessage.EMM_MessSection_Header.ucPrivate_Indicator  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.ucPrivate_Indicator );
		ALOGE("EMM_ComMessage.EMM_MessSection_Header.uiSection_len  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.uiSection_len );
		ALOGE("EMM_ComMessage.EMM_MessSection_Header.uiTable_Id_Extension  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.uiTable_Id_Extension );
		ALOGE("EMM_ComMessage.EMM_MessSection_Header.ucVersionNum  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.ucVersionNum );
		ALOGE("EMM_ComMessage.EMM_MessSection_Header.ucCurr_Next_Indicator  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.ucCurr_Next_Indicator );
		ALOGE("EMM_ComMessage.EMM_MessSection_Header.ucSectionNum  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.ucSectionNum );
		ALOGE("EMM_ComMessage.EMM_MessSection_Header.ucLastSectionNum  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_MessSection_Header.ucLastSectionNum );

		ALOGE("EMM_ComMessage.EMM_ComMessage_Field.ucBroadcaster_GroupID  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucBroadcaster_GroupID );
		ALOGE("EMM_ComMessage.EMM_ComMessage_Field.ucDeletion_Status  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucDeletion_Status );
		ALOGE("EMM_ComMessage.EMM_ComMessage_Field.ucDisp_Duration1  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucDisp_Duration1 );
		ALOGE("EMM_ComMessage.EMM_ComMessage_Field.ucDisp_Duration2 = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucDisp_Duration2 );
		ALOGE("EMM_ComMessage.EMM_ComMessage_Field.ucDisp_Duration3  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucDisp_Duration3 );
		ALOGE("EMM_ComMessage.EMM_ComMessage_Field.ucDisp_Cycle  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucDisp_Cycle );
		ALOGE("EMM_ComMessage.EMM_ComMessage_Field.ucFormat_Version  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucFormat_Version );
		ALOGE("EMM_ComMessage.EMM_ComMessage_Field.uiMessageLen  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.uiMessageLen );
		ALOGE("EMM_ComMessage.uiEMM_Message_Section_CRC  = %x \n", EMM_ComMessage[EMM_ComMessage_Cnt].uiEMM_Message_Section_CRC);
#if 0
		if(EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.uiMessageLen>0)
		{
			ALOGE("EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area\n");
//			for(i=0;  i<EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.uiMessageLen;i+=4)
			for(i=0;  i<4;i+=8)
			{
				ALOGE("0x%x, 0x%x, 0x%x, 0x%x,\n", EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i], EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i +1],
					EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i +2], EMM_ComMessage[EMM_ComMessage_Cnt].EMM_ComMessage_Field.ucMessage_Area[i+4]);
			}
			ALOGE("\n");
		}
#endif		
#endif		
		if(BCAS_Auto_Disp_MessageArch.ucMessageInfoGetStatus)
			EMM_Message_Receive_status.uiMessageType =  EMM_AUTODISPLAY_MESSAGE;

		if(EMM_ComMessage_Cnt<BCAS_MAX_EMM_COM_MESSAGE_CNT)
			EMM_ComMessage_Cnt ++;

		ret =  ISDBT_MESSAGE_TYPE;
	}
	if(ISDB_Mail_NoPreset_Message_check == TRUE)
	{	
		ISDB_Mail_NoPreset_Message_check = 0;
		EMM_Message_Receive_status.uiMessageType  = EMM_BASIC;
		EMM_Message_Receive_status.uiEncrypted_status = -1;
		ISDBT_MESSAGE_TYPE = EMM_BASIC;
		ret  = EMM_MAIL_MESSAGE;
	}
	else if(ret == EMM_MAIL_MESSAGE)
	{
		ISDB_Mail_NoPreset_Message_check = 0;
		EMM_Message_Receive_status.uiMessageType  = EMM_BASIC;
		EMM_Message_Receive_status.uiEncrypted_status = -1;
		ISDBT_MESSAGE_TYPE = EMM_BASIC;
	}

	return ret;
}

