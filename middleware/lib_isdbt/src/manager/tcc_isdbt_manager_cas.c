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
#define LOG_TAG	"ISDBT_MANAGER_CAS"

#include <stdlib.h>
#include <utils/Log.h>
#include <fcntl.h>
#include <pthread.h>

#include <tcc_dxb_memory.h>
#include <tcc_dxb_queue.h>
#include <tcc_dxb_thread.h>
#include "tcc_isdbt_event.h"
#include "tcc_dxb_interface_demux.h"

#include "tcc_dxb_interface_cipher.h"
#include "BitParser.h"
#include "ISDBT.h"
#include "ISDBT_Caption.h"
#include "ISDBT_Common.h"
#include "TsParse_ISDBT.h"
#include "TsParse_ISDBT_DBLayer.h"
#include "TsParser_Subtitle.h"

#include "subtitle_draw.h"

#include "tcc_dxb_manager_dsc.h"
#include "tcc_dxb_manager_sc.h"
#include "tcc_dxb_manager_cas.h"
#include "tcc_trmp_manager.h"

#include "tcc_isdbt_manager_tuner.h"
#include "tcc_isdbt_manager_demux.h"
#include "tcc_isdbt_manager_cas.h"

#include "tcc_isdbt_proc.h"

/****************************************************************************
DEFINITION
****************************************************************************/
#define DEBUG_ISDBT_CAS
#ifdef DEBUG_ISDBT_CAS
#undef DBG_PRINTF
#define DBG_PRINTF					ALOGD
#undef ERR_PRINTF
#define ERR_PRINTF					ALOGE
#else
#undef DBG_PRINTF
#define DBG_PRINTF			
#undef ERR_PRINTF
#define ERR_PRINTF					ALOGE
#endif

/****************************************************************************
DEFINITION OF EXTERNAL VARIABLES
****************************************************************************/
extern BCAS_InitSet_Architecture_t BCAS_InitSet_ARCH;
extern BCAS_Ecm_Architecture_t	BCAS_Ecm_ARCH;
extern BCAS_CardID_Architecture_t BCAS_CardID_Arch;

/****************************************************************************
DEFINITION OF EXTERNAL FUNCTIONS
****************************************************************************/
extern int tcc_demux_stop_cat_filtering(void);

/****************************************************************************
DEFINITION OF GLOBAL VARIABLES
****************************************************************************/

/****************************************************************************
DEFINITION OF LOCAL VARIABLES
****************************************************************************/

/****************************************************************************
DEFINITION OF LOCAL FUNCTIONS
****************************************************************************/

/****************************************************************************
DEFINITION OF BCAS 
****************************************************************************/
static int giBCasThreadRunning = 0;
static pthread_t BCasDecoderThreadID;
static tcc_dxb_queue_t* BCasDecoderQueue;
EMM_Message_Info_t		EMM_DispMessage;

static unsigned int	ECM_PARSE_STOP_FLAG=1;

static unsigned int	SC_Old_Error= SC_ERR_MAX;

unsigned char	Old_Ks_Odd[BCAS_ECM_KS_LEN];
unsigned char	Old_Ks_Even[BCAS_ECM_KS_LEN];

//this array stores PIDs of ES, which will be transfered to tcc353x to decrypt TS.
static unsigned int gTCCBB_CAS_PID[4];

int (*fnGetBer)(int *);
static int gTunerGetStrIdxTime = 0;

static int giBCASPMTScanFlag = 0;

static unsigned int CAS_Open_Flag=0;
static unsigned int	CARD_DETECT_FLAG=0;

static int 	OldEcmVersion_Num = -1;

extern DxBInterface *hInterface;

int tcc_demux_get_message(int Message_type)
{
	unsigned short ucMessageData[ISDBT_EMM_MAX_MAIL_SIZE];
	unsigned int i=0, ret, len;;
	void*	ptr= NULL;

	TCC_Dxb_SC_Manager_EMM_MessageInfoGet(&EMM_DispMessage);
	ret = ISDBTString_MakeUnicodeString_For_Message((char *)EMM_DispMessage.ucMessagePayload, (short *)ucMessageData, Get_Language_Code());

	ALOGE("EMM_DispMessage: Message_type = %d, uiMessageLen =%d , ret  = %d \n",Message_type, EMM_DispMessage.uiMessageLen, ret );
	memcpy(EMM_DispMessage.ucMessagePayload, ucMessageData, ret*2);

	EMM_DispMessage.uiMessageLen = ret *2;

	if(Message_type == EMM_AUTODISPLAY_MESSAGE)
	{
//		TCCDxBEvent_AutoMessageUpdate((void *)&EMM_DispMessage, EMM_DispMessage.uiMessageLen);
	}

	ALOGE("EMM_DispMessage: uiMessageID  = %x \n", EMM_DispMessage.uiMessageID );
	ALOGE("EMM_DispMessage: ucBroadcaster_GroupID = %x \n", EMM_DispMessage.ucBroadcaster_GroupID );
	ALOGE("EMM_DispMessage: ucDeletion_Status = %x \n", EMM_DispMessage.ucDeletion_Status );
	ALOGE("EMM_DispMessage: ucDisp_Duration1 = %x \n", EMM_DispMessage.ucDisp_Duration1);
	ALOGE("EMM_DispMessage: ucDisp_Duration2 = %x \n", EMM_DispMessage.ucDisp_Duration2 );
	ALOGE("EMM_DispMessage: ucDisp_Duration3 = %x \n", EMM_DispMessage.ucDisp_Duration3);
	ALOGE("EMM_DispMessage: ucDisp_Cycle  = %x \n", EMM_DispMessage.ucDisp_Cycle );
	ALOGE("EMM_DispMessage: ucFormat_Version  = %x \n", EMM_DispMessage.ucFormat_Version );
	ALOGE("EMM_DispMessage: ucDisp_Horizontal_PositionInfo  = %x \n", EMM_DispMessage.ucDisp_Horizontal_PositionInfo);
	ALOGE("EMM_DispMessage: ucDisp_Horizontal_PositionInfo  = %x \n", EMM_DispMessage.ucDisp_Vertical_PositionInfo);
	ALOGE("EMM_DispMessage: uiMessageLen = %d \n", EMM_DispMessage.uiMessageLen);

	ALOGE("%s %d [ret= %d ]\n", __func__, __LINE__, ret);
#if 0
	if(ret>0)
	{
		for(i=0;  i<(EMM_DispMessage.uiMessageLen-3);i+=4)
		{
			ALOGE("[i = %d][0x%x, 0x%x, 0x%x, 0x%x]\n", i, EMM_DispMessage.ucMessagePayload[i], EMM_DispMessage.ucMessagePayload[i +1],
				EMM_DispMessage.ucMessagePayload[i +2], EMM_DispMessage.ucMessagePayload[i+3]);
		}
		ALOGE("\n");
	}
#endif

	return 0;
}

int tcc_demux_mail_DeleteStatus_update(int uiMessageID , int uiDelete_status)
{
	int err=0;
	err = UpdateDB_MailTable_DeleteStatus(uiMessageID, uiDelete_status);
	return err;
}

int tcc_demux_mail_UserViewStatus_update(int uiMessageID , int uiUserView_status)
{
	int err=0;
	err = UpdateDB_MailTable_UserViewStatus(uiMessageID,uiUserView_status);
	return err;
}

int tcc_demux_mail_update(int uiCASystemID)
{
	unsigned int	uiCurrentData=0;
	unsigned int	uiYear, uiMonth, uiDay;

	int YEAR = 1900;
	int y, m, k;

	uiCurrentData=(int)ISDBT_GetTOTMJD();

	y = (int)((uiCurrentData - 15078.2f) / 365.25f);
	m = (int)((uiCurrentData - 14956.1f - (y * 365.25f)) / 30.6001f);
	uiDay =(int)(uiCurrentData - 14956 - (y * 365.25f) -(m * 30.6001f));
	if (m == 14 || m == 15)
		k = 1;
	else
		k = 0;
	uiYear = YEAR + y + k;
	uiMonth = m - 1 - k * 12;

	ALOGE("%s %d uiYear=%d , uiMonth=%d, uiDay=%d\n", __func__, __LINE__, uiYear, uiMonth, uiDay);

#if 1	
	if(EMM_DispMessage.uiMessageLen>0)
	{	
		UpdateDB_MailTable(uiCASystemID, 
			EMM_DispMessage.uiMessageID,
			uiYear,
			uiMonth,
			uiDay,
			EMM_DispMessage.uiMessageLen,
			(char *)EMM_DispMessage.ucMessagePayload,
			0,
			0
			);
	}
	else
		ALOGE("%s %d Mail uiMessageLen error \n", __func__, __LINE__);
#endif
	return 0;
}

void tcc_demux_get_autodisp_info(void)
{
	int err;
	T_DESC_CA_SERVICE pDescCAService;

	if(!TCC_Dxb_SC_Manager_AutoDisp_Message_GetStatus())
	{	
		if(ISDBT_Get_DescriptorInfo(E_DESC_CAT_SERVICE, (void *)&pDescCAService))
		{
			unsigned short	uiCASystemID = -1;
			unsigned char		ucCABroadcate_GR_ID = -1;
			unsigned char		ucCAMessage_Ctrl = -1;
			unsigned short	uiCurrentDate =-1;
			
			uiCASystemID = pDescCAService.uiCA_Service_SystemID;
			ucCABroadcate_GR_ID = pDescCAService.ucCA_Service_Broadcast_GroutID;
			ucCAMessage_Ctrl = pDescCAService.ucCA_Service_MessageCtrl;

			uiCurrentDate = ISDBT_GetTOTMJD();
			
			ALOGE("%s %d uiCurrentDate = %x, uiCASystemID = %d,  ucCABroadcate_GR_ID =%d,  ucCAMessage_Ctrl = 0x%x\n", __func__, __LINE__, uiCurrentDate, uiCASystemID, ucCABroadcate_GR_ID, ucCAMessage_Ctrl);	
			err = TCC_Dxb_SC_Manager_AutoDisp_Message_Get(uiCurrentDate, ucCABroadcate_GR_ID, ucCAMessage_Ctrl);
			ALOGE("%s %d err = 0x%x\n", __func__, __LINE__, err);	

		}
	}
}

void tcc_manager_bcas_smartcard_error_update(void *arg)
{
	int err=0;

	 //DBG_PRINTF("%s %d arg = %d  \n", __func__, __LINE__, (int)arg);

	if(SC_Old_Error != (unsigned int)arg)
	{
		err = TCC_DxB_SC_Manager_SendT1_Glue((char)SC_INS_PARAM_IDI, (int)NULL, (char *)NULL);
		if(err != SC_ERR_OK)
		{
			TCC_DxB_SC_Manager_ClearCardInfo();
		}
//		TCCDxBEvent_SmartCardErrorUpdate(arg);
		SC_Old_Error = (unsigned int)arg;
	}	
}

void tcc_manager_bcas_get_smartcard_info(void)
{
	int err=0;
#if 0	
	err = TCC_DxB_SC_Manager_SendT1((char)SC_INS_PARAM_IDI, (int)NULL, (char *)NULL);
#else
	err = TCC_DxB_SC_Manager_SendT1_Glue((char)SC_INS_PARAM_IDI, (int)NULL, (char *)NULL);
#endif
	tcc_manager_bcas_smartcard_error_update((void*)err);

	if(err != SC_ERR_OK)
	{
		TCC_DxB_SC_Manager_ClearCardInfo();
	}
	//TCCDxBEvent_SmartCardInfoUpdate(&BCAS_CardID_Arch, sizeof(BCAS_CardID_Architecture_t));
}

int tcc_manager_bcas_parser_emm(void *handle, char *data, unsigned int data_size)
{
	void *gpSimpleSITable = NULL;
	unsigned short usDesCnt = 0;
	int Result = 0;
	int data_len = 0;
	int err = 0, ret = 0;
	unsigned char ucEMM_TableID = 0;
	unsigned int Version_Num = 0;
	T_DESC_CA pDescCA = { 0, };
	unsigned int uiCASystemID = -1;
	
	Version_Num = 0x3f&(*(data+5)>>1);

	ucEMM_TableID = data[0];
	data_len = ((data[1] & 0x0f) << 8) + data[2] + 3;
	if((unsigned int)data_len > data_size)
	{
		ALOGE("%s %d data_len=%d data_size=%d \n", __func__, __LINE__, data_len, data_size);
		return -1;
	}

	if(ucEMM_TableID == EMM_1_ID)
	{	
/*
Noah, To avoid Codesonar's warning, Redundant Condition
'if statement' is always false.
		if(ret == EMM_AUTODISPLAY_MESSAGE)
			tcc_demux_get_autodisp_info();
*/

		ret = TCC_DxB_SC_Manager_EMM_MessageSectionParsing(data_len, data);

//	ALOGE("%s %d ret = %d, data_len=%d \n", __func__, __LINE__, ret, data_len);

		if(ret == EMM_AUTODISPLAY_MESSAGE || ret == EMM_MAIL_MESSAGE)
			tcc_demux_get_message(ret);

		if(ret  == EMM_MAIL_MESSAGE)
		{
			ISDBT_Get_DescriptorInfo(E_DESC_CAT_CA, (void *)&pDescCA);
			uiCASystemID = (unsigned int)pDescCA.uiCASystemID;
			tcc_demux_mail_update(uiCASystemID);
		}
	}
	else
	{	
		data_len -= (ISDBT_CAS_SECTION_HEADER_LENGTH+ISDBT_CAS_SECTION_CRC_LENGTH);
		if(data_len <= 0)
		{
			ALOGE("%s %d data_len=%d \n", __func__, __LINE__, data_len);
			return -1;
		}

//		ALOGE("%s %d data_len = %d \n", __func__, __LINE__, data_len);
		err = TCC_DxB_SC_Manager_EMM_SectionParsing(data_len,(char*)(data+ISDBT_CAS_SECTION_HEADER_LENGTH));
	}

	tcc_manager_bcas_smartcard_error_update((void*)err);
	
	return 0;
}

int tcc_manager_bcas_parser_ecm(void *handle, char *data, unsigned int data_size)
{
	int 	Result=0;
	int	err=0, ret =0;
	int data_len=0;
	unsigned int	Version_Num;
	unsigned int	bcas_cipher_usage = 0;

	Version_Num = (0x3f&data[5])>>1;
	if(OldEcmVersion_Num == (signed int)Version_Num)
		return 0;
	OldEcmVersion_Num = Version_Num;

	//DBG_PRINTF("%s %d \n", __func__, __LINE__);
	
	data_len = ((data[1] & 0x0f) << 8) + data[2] + 3;
	if(((unsigned int)data_len > data_size) || (data_len <= (ISDBT_CAS_SECTION_HEADER_LENGTH+ISDBT_CAS_SECTION_CRC_LENGTH)))
	{
		ALOGE("%s %d data_len=%d data_size=%d \n", __func__, __LINE__, data_len, data_size);
		return -1;
	}
	data_len -= (ISDBT_CAS_SECTION_HEADER_LENGTH+ISDBT_CAS_SECTION_CRC_LENGTH);
#if 0	
	err = TCC_DxB_SC_Manager_SendT1((char)SC_INS_PARAM_ECM, data_len, (char*)(data+ISDBT_CAS_SECTION_HEADER_LENGTH));
#else
	err = TCC_DxB_SC_Manager_SendT1_Glue((char)SC_INS_PARAM_ECM, data_len, (char*)(data+ISDBT_CAS_SECTION_HEADER_LENGTH));
#endif
	tcc_manager_bcas_smartcard_error_update((void*)err);

	//DBG_PRINTF("In %s, err=%d\n", __func__, err);

	if(!err)
	{
#if 0
		ERR_PRINTF("Even_Key: %x:%x:%x:%x:%x:%x:%x:%x\n", BCAS_Ecm_ARCH.Ks_Even[0],BCAS_Ecm_ARCH.Ks_Even[1]
						,BCAS_Ecm_ARCH.Ks_Even[2],BCAS_Ecm_ARCH.Ks_Even[3]
						,BCAS_Ecm_ARCH.Ks_Even[4],BCAS_Ecm_ARCH.Ks_Even[5]
						,BCAS_Ecm_ARCH.Ks_Even[6],BCAS_Ecm_ARCH.Ks_Even[7]);
		
		ERR_PRINTF("Odd_Key: %x:%x:%x:%x:%x:%x:%x:%x\n", BCAS_Ecm_ARCH.Ks_Odd[0],BCAS_Ecm_ARCH.Ks_Odd[1]
						,BCAS_Ecm_ARCH.Ks_Odd[2],BCAS_Ecm_ARCH.Ks_Odd[3]
						,BCAS_Ecm_ARCH.Ks_Odd[4],BCAS_Ecm_ARCH.Ks_Odd[5]
						,BCAS_Ecm_ARCH.Ks_Odd[6],BCAS_Ecm_ARCH.Ks_Odd[7]);

		ERR_PRINTF("CBC_init_value: %x:%x:%x:%x:%x:%x:%x:%x\n", BCAS_InitSet_ARCH.Desc_CBC_InitValue[0],BCAS_InitSet_ARCH.Desc_CBC_InitValue[1]
						,BCAS_InitSet_ARCH.Desc_CBC_InitValue[2],BCAS_InitSet_ARCH.Desc_CBC_InitValue[3]
						,BCAS_InitSet_ARCH.Desc_CBC_InitValue[4],BCAS_InitSet_ARCH.Desc_CBC_InitValue[5]
						,BCAS_InitSet_ARCH.Desc_CBC_InitValue[6],BCAS_InitSet_ARCH.Desc_CBC_InitValue[7]);

		ERR_PRINTF("System_Key: %x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x\n", 
				BCAS_InitSet_ARCH.Desc_System_Key[0],BCAS_InitSet_ARCH.Desc_System_Key[1],BCAS_InitSet_ARCH.Desc_System_Key[2]
			,BCAS_InitSet_ARCH.Desc_System_Key[3],BCAS_InitSet_ARCH.Desc_System_Key[4],BCAS_InitSet_ARCH.Desc_System_Key[5]
			,BCAS_InitSet_ARCH.Desc_System_Key[6],BCAS_InitSet_ARCH.Desc_System_Key[7],BCAS_InitSet_ARCH.Desc_System_Key[8]
			,BCAS_InitSet_ARCH.Desc_System_Key[9],BCAS_InitSet_ARCH.Desc_System_Key[10],BCAS_InitSet_ARCH.Desc_System_Key[11],
			BCAS_InitSet_ARCH.Desc_System_Key[12],BCAS_InitSet_ARCH.Desc_System_Key[13],BCAS_InitSet_ARCH.Desc_System_Key[14],
			BCAS_InitSet_ARCH.Desc_System_Key[15],BCAS_InitSet_ARCH.Desc_System_Key[16],BCAS_InitSet_ARCH.Desc_System_Key[17],
			BCAS_InitSet_ARCH.Desc_System_Key[18],BCAS_InitSet_ARCH.Desc_System_Key[19],BCAS_InitSet_ARCH.Desc_System_Key[20],
			BCAS_InitSet_ARCH.Desc_System_Key[21],BCAS_InitSet_ARCH.Desc_System_Key[22],BCAS_InitSet_ARCH.Desc_System_Key[23],
			BCAS_InitSet_ARCH.Desc_System_Key[24],BCAS_InitSet_ARCH.Desc_System_Key[25],BCAS_InitSet_ARCH.Desc_System_Key[26],
			BCAS_InitSet_ARCH.Desc_System_Key[27],BCAS_InitSet_ARCH.Desc_System_Key[28],BCAS_InitSet_ARCH.Desc_System_Key[29],
			BCAS_InitSet_ARCH.Desc_System_Key[30],BCAS_InitSet_ARCH.Desc_System_Key[31]);
#endif

		bcas_cipher_usage = tcc_manager_cas_getCipherType();
		if (bcas_cipher_usage == ISDBT_CAS_CIPHER_SW) {
			if(CAS_Open_Flag == 0)
			{
				err = TCC_DxB_CAS_OpenSWMulti2(0, 0, 0, 4, BCAS_InitSet_ARCH.Desc_System_Key);
				if(err !=0)
				{
					ERR_PRINTF("error [%s][%d] \n", __func__, __LINE__);
					return -1;
				}
				CAS_Open_Flag = 1;
				TCC_DxB_CAS_SetKeySWMulti2(0, BCAS_Ecm_ARCH.Ks_Odd, BCAS_ECM_KS_LEN,
											BCAS_InitSet_ARCH.Desc_CBC_InitValue, BCAS_CBC_INIT_LEN);
				TCC_DxB_CAS_SetKeySWMulti2(1, BCAS_Ecm_ARCH.Ks_Even, BCAS_ECM_KS_LEN,
											BCAS_InitSet_ARCH.Desc_CBC_InitValue, BCAS_CBC_INIT_LEN);
			}
			else
			{
				TCC_DxB_CAS_SetKeySWMulti2(0, BCAS_Ecm_ARCH.Ks_Odd, BCAS_ECM_KS_LEN, 0, 0);
				TCC_DxB_CAS_SetKeySWMulti2(1, BCAS_Ecm_ARCH.Ks_Even, BCAS_ECM_KS_LEN, 0, 0);
			}
		} else if (bcas_cipher_usage == ISDBT_CAS_CIPHER_HW || bcas_cipher_usage == ISDBT_CAS_CIPHER_HWDEMUX) {
			if(CAS_Open_Flag == 0)
			{
				err = TCC_DxB_CAS_Open(TCC_CHIPHER_ALGORITM_MULTI2, TCC_CHIPHER_OPMODE_CBC, 0, 4);
				if(err !=0)
				{
					ERR_PRINTF("error [%s][%d] \n", __func__, __LINE__);
					return -1;
				}
				CAS_Open_Flag = 1;
			}

			err = TCC_DxB_CAS_SetKey(BCAS_Ecm_ARCH.Ks_Even, BCAS_ECM_KS_LEN,
									BCAS_InitSet_ARCH.Desc_CBC_InitValue, BCAS_CBC_INIT_LEN,
									BCAS_InitSet_ARCH.Desc_System_Key, BCAS_SYSTEM_KEY_LEN, 0x02);
			if(err !=0)
			{
				ERR_PRINTF("error [%s][%d] \n", __func__, __LINE__);
				return -1;
			}

			err = TCC_DxB_CAS_SetKey(BCAS_Ecm_ARCH.Ks_Odd, BCAS_ECM_KS_LEN,
									BCAS_InitSet_ARCH.Desc_CBC_InitValue, BCAS_CBC_INIT_LEN,
									BCAS_InitSet_ARCH.Desc_System_Key, BCAS_SYSTEM_KEY_LEN, 0x03);
			if(err !=0)
			{
				ERR_PRINTF("error [%s][%d] \n", __func__, __LINE__);
				return -1;
			}
		} else if (bcas_cipher_usage == ISDBT_CAS_CIPHER_TCC353X) {
			if(CAS_Open_Flag == 0)
			{
				unsigned int EsPids[4];
				int	i, count;

				tcc_manager_tuner_cas_open (4, &BCAS_InitSet_ARCH.Desc_System_Key[0]);

				TCCDxBProc_Get_CASPIDInfo ((unsigned int *)&EsPids[0]);
				count = 0;
				for(i=0; i < 4; i++) {
					if (EsPids[i] < 2 || EsPids[i] >= 0x1FFF)
						EsPids[i] = 0xFFFF;
					if (gTCCBB_CAS_PID[i] != EsPids[i]) {
						count++;
					}
				}
				if (count) {
					//back-up PIDs
					for(i=0; i < 4; i++) {
						gTCCBB_CAS_PID[i] = EsPids[i];
					}

					count = 0; //count valid pid
					for(i=0; i < 4; i++) {
						if (gTCCBB_CAS_PID[i] != 0xFFFF) {
							EsPids[count++] = gTCCBB_CAS_PID[i];
						}
				}
					tcc_manager_tuner_cas_set_pid (&EsPids[0], count);
				}

				CAS_Open_Flag = 1;

				tcc_manager_tuner_cas_key_multi2 (0, BCAS_Ecm_ARCH.Ks_Odd, BCAS_ECM_KS_LEN, BCAS_InitSet_ARCH.Desc_CBC_InitValue, BCAS_CBC_INIT_LEN);
				tcc_manager_tuner_cas_key_multi2 (1, BCAS_Ecm_ARCH.Ks_Even, BCAS_ECM_KS_LEN, BCAS_InitSet_ARCH.Desc_CBC_InitValue, BCAS_CBC_INIT_LEN);

				memcpy(Old_Ks_Odd, BCAS_Ecm_ARCH.Ks_Odd, BCAS_ECM_KS_LEN);
				memcpy(Old_Ks_Even, BCAS_Ecm_ARCH.Ks_Even, BCAS_ECM_KS_LEN);
			}
			else
			{
				unsigned int evenchanged = 0;
				unsigned int oddchanged = 0;
				int i;

				for(i=0; i<BCAS_ECM_KS_LEN; i++)
				{
					if(!evenchanged)
					{
						if(BCAS_Ecm_ARCH.Ks_Even[i]!=Old_Ks_Even[i])
							evenchanged = 1;
					}
					if(!oddchanged)
					{
						if(BCAS_Ecm_ARCH.Ks_Odd[i]!=Old_Ks_Odd[i])
							oddchanged = 1;
					}
				}

				tcc_manager_tuner_cas_open (4, &BCAS_InitSet_ARCH.Desc_System_Key[0]);
				tcc_manager_tuner_cas_key_multi2 (0, BCAS_Ecm_ARCH.Ks_Odd, BCAS_ECM_KS_LEN, BCAS_InitSet_ARCH.Desc_CBC_InitValue, BCAS_CBC_INIT_LEN);
				tcc_manager_tuner_cas_key_multi2 (1, BCAS_Ecm_ARCH.Ks_Even, BCAS_ECM_KS_LEN, BCAS_InitSet_ARCH.Desc_CBC_InitValue, BCAS_CBC_INIT_LEN);

				memcpy(Old_Ks_Odd, BCAS_Ecm_ARCH.Ks_Odd, BCAS_ECM_KS_LEN);
				memcpy(Old_Ks_Even, BCAS_Ecm_ARCH.Ks_Even, BCAS_ECM_KS_LEN);
				/*
				tcc_manager_tuner_cas_key_multi2 (0, BCAS_Ecm_ARCH.Ks_Odd, BCAS_ECM_KS_LEN, 0, 0);
				tcc_manager_tuner_cas_key_multi2 (1, BCAS_Ecm_ARCH.Ks_Even, BCAS_ECM_KS_LEN, 0, 0);
				*/
			}
		}
	}
	return 0;
}

void* tcc_manager_bcas_decoder(void *arg)
{
	void 	*pSectionBitPars = NULL;
	int Result = 0, ucTableID;
	DEMUX_DEC_COMMAND_TYPE *pSectionCmd;

	DBG_PRINTF("In %s\n", __func__);

	BITPARS_OpenBitParser( &pSectionBitPars );

	while(giBCasThreadRunning)
	{
		if(tcc_dxb_getquenelem(BCasDecoderQueue)){
			pSectionCmd = tcc_dxb_dequeue(BCasDecoderQueue);
			if (pSectionCmd){
				if(ECM_PARSE_STOP_FLAG){
					usleep(5000);			
				}else{
					if(pSectionCmd->uiDataSize > (ISDBT_CAS_SECTION_HEADER_LENGTH + ISDBT_CAS_SECTION_CRC_LENGTH)) {
						ucTableID = *pSectionCmd->pData;
						switch(ucTableID)
						{
							case ECM_0_ID:
							case ECM_1_ID:
								//DBG_PRINTF("%s %d Searched ECM\n", __func__, __LINE__);
								if(CARD_DETECT_FLAG == 1){
									tcc_manager_bcas_parser_ecm(pSectionBitPars, (char*)pSectionCmd->pData, pSectionCmd->uiDataSize);
								}
								break;
	
							case EMM_0_ID:
							case EMM_1_ID:
								//DBG_PRINTF("%s %d Searched EMM\n", __func__, __LINE__);
								if(CARD_DETECT_FLAG == 1){
									tcc_manager_bcas_parser_emm(pSectionBitPars, (char*)pSectionCmd->pData, pSectionCmd->uiDataSize);
								}
								break;
							
							default:
								break;
						}
					}
				}

				if(pSectionCmd->pArg != NULL) {
					tcc_mw_free(__FUNCTION__, __LINE__, pSectionCmd->pArg);
				}
				tcc_mw_free(__FUNCTION__, __LINE__, pSectionCmd->pData);
				tcc_mw_free(__FUNCTION__, __LINE__, pSectionCmd);
			}
		}else{
			if(ECM_PARSE_STOP_FLAG == 0 && CARD_DETECT_FLAG == 0){
				if(TCC_DxB_SC_Manager_CardDetect() == SC_ERR_OK){
					tcc_demux_stop_cat_filtering();
				#if 1
					giBCASPMTScanFlag = 0;
				#endif
					Result= tcc_manager_demux_cas_init();
					if(Result == 0)
						CARD_DETECT_FLAG = 1;
				}
				else
					tcc_manager_bcas_smartcard_error_update((void*)SC_ERR_CARD_DETECT);
			}
			if (CARD_DETECT_FLAG==0)	usleep(50000);
			else usleep(5000);
		}
	}

	BITPARS_CloseBitParser( pSectionBitPars );
#if 0
	giBCASPMTScanFlag = 0;
#endif
	giBCasThreadRunning = -1;

	return (void*)NULL;
}

int  tcc_manager_bcas_init(void)
{
	unsigned int bcas_cipher_usage = 0;
	int i;
	int status;
	int err;

	for(i=0; i < 4; i++) {
		gTCCBB_CAS_PID[i] = 0xFFFF;
	}

	giBCasThreadRunning = 1;

	BCasDecoderQueue = tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(tcc_dxb_queue_t));
	memset(BCasDecoderQueue, 0, sizeof(tcc_dxb_queue_t));

	tcc_dxb_queue_init_ex(BCasDecoderQueue, ISDBT_CAS_SECTION_QUEUESIZE);

	status = tcc_dxb_thread_create((void *)&BCasDecoderThreadID,
								tcc_manager_bcas_decoder,
								(unsigned char*)"tcc_manager_bcas_decoder",
								LOW_PRIORITY_10,
								NULL);
	if(status)
	{
		ERR_PRINTF("In %s CAN NOT make bcas decoder !!!!\n", __func__);
	}

	SC_Old_Error= SC_ERR_MAX;
	err = TCC_Dxb_SC_Manager_Open();
	tcc_manager_bcas_smartcard_error_update((void*)err);

	bcas_cipher_usage = tcc_manager_cas_getCipherType();
	if (bcas_cipher_usage == ISDBT_CAS_CIPHER_SW) {
		TCC_DxB_SC_Manager_SetDescriptionMode(MULTI2_MODE_SW);
		TSDEMUXDSCApi_ModeSet(DSC_MODE_SW_MULTI2);
	} else if (bcas_cipher_usage == ISDBT_CAS_CIPHER_HW) {
		TCC_DxB_SC_Manager_SetDescriptionMode(MULTI2_MODE_HW);
		TSDEMUXDSCApi_ModeSet(DSC_MODE_HW);
	} else if (bcas_cipher_usage == ISDBT_CAS_CIPHER_TCC353X) {
		//blank in intention.
	} else if (bcas_cipher_usage == ISDBT_CAS_CIPHER_HWDEMUX) {
		TCC_DxB_SC_Manager_SetDescriptionMode(MULTI2_MODE_HWDEMUX);
		TSDEMUXDSCApi_ModeSet(DSC_MODE_HWDEMUX);
	}

	TCC_DxB_DEMUX_RegisterCasDecryptCallback(hInterface, 0, TSDEMUXDSCApi_Decrypt);

	return ISDBT_CAS_SUCCESS;
}

void tcc_manager_bcas_deinit(void)
{
	unsigned int bcas_cipher_usage = 0;
	int i;
	int err;

	for (i=0; i <4; i++)
	{
		gTCCBB_CAS_PID[i] = 0xFFFF;
	}

	giBCasThreadRunning =0;
	while(1)
	{
		if( giBCasThreadRunning == -1)
			break;
		usleep(5000);
	}

	bcas_cipher_usage = tcc_manager_cas_getCipherType();
	if (bcas_cipher_usage == ISDBT_CAS_CIPHER_SW) {
		TCC_DxB_CAS_CloseSWMulti2();
	} else if (bcas_cipher_usage == ISDBT_CAS_CIPHER_HW || bcas_cipher_usage == ISDBT_CAS_CIPHER_HWDEMUX) {
		TCC_DxB_CAS_Close();
	} else if (bcas_cipher_usage == ISDBT_CAS_CIPHER_TCC353X) {
		//blank in intention
	}

	err = TCC_Dxb_SC_Manager_Close();
	tcc_manager_bcas_smartcard_error_update((void*)err);

	err = tcc_dxb_thread_join((void *)BCasDecoderThreadID,NULL);
	tcc_dxb_queue_deinit(BCasDecoderQueue);
	tcc_mw_free(__FUNCTION__, __LINE__, BCasDecoderQueue);

	return;
}

int tcc_manager_bcas_reset(void)
{
	int err = ISDBT_CAS_SUCCESS;

	if(giBCASPMTScanFlag == TRUE)
	{
		ERR_PRINTF("%s %d Already B-CAS Open\n", __func__, __LINE__);
		CARD_DETECT_FLAG = 1;
		return ISDBT_CAS_ERROR;
	}
	else
		CARD_DETECT_FLAG = 0;

	if(TCC_DxB_SC_Manager_CardDetect() == SC_ERR_OK)
	{
		ISDBT_Init_CA_DescriptorInfo();
		err = TCC_DxB_SC_Manager_Init();
		tcc_manager_bcas_smartcard_error_update((void*)err);

		if(err != 0)
		{
			ECM_PARSE_STOP_FLAG = 0;
			giBCASPMTScanFlag = 0;
			return ISDBT_CAS_ERROR_SMART_CARD;
		}
			
		CAS_Open_Flag = 0;
		OldEcmVersion_Num = 0;
		CARD_DETECT_FLAG = 1;
		giBCASPMTScanFlag = 1;

		OpenMailDB_Table();
		tcc_manager_bcas_get_smartcard_info();
		DBG_PRINTF("%s %d B-CAS Init OK\n", __func__, __LINE__);

		err = ISDBT_CAS_SUCCESS;
	}
	else
	{
		TCC_DxB_SC_Manager_ClearCardInfo ();
		tcc_manager_bcas_smartcard_error_update((void*)SC_ERR_CARD_DETECT);

		err = ISDBT_CAS_ERROR_SMART_CARD;
	}

	ECM_PARSE_STOP_FLAG = 0;

	return err;
}

int tcc_manager_bcas_getSCInfo(void **ppInfo, int *pInfoSize)
{
	TCC_DxB_SC_Manager_GetCardInfo(ppInfo, pInfoSize);

	return ISDBT_CAS_SUCCESS;
}

/****************************************************************************
DEFINITION OF TRMP
****************************************************************************/
static tcc_dxb_queue_t* TRMPDecoderQueue;

static int g_iTRMPThreadRunning = 0;
static pthread_t TRMPDecoderThreadID;

static int g_iMulti2Running = 0;

static int g_iECMVersionNumber = -1;
static int g_iPrevECMRetValue = TCC_TRMP_SUCCESS;
static int g_iPrevEMMRetValue = TCC_TRMP_SUCCESS;

int tcc_manager_trmp_parseEMM(unsigned short usNetworkID, char *pEMMSectionData, unsigned int uiEMMSectionDataLen)
{
	int iEMMSectionBodyLen=0;
	int iProtocolNumber = 0;
	int iRet;
	
	//DBG_PRINTF("[%s]\n", __func__);

	if(usNetworkID == 0) {
		//ERR_PRINTF("[%s] NetworkID is 0\n", __func__);

		return TCC_TRMP_SUCCESS;
	}

	iEMMSectionBodyLen = ((pEMMSectionData[1] & 0x0f) << 8) + pEMMSectionData[2] + 3;
	if(((unsigned int)iEMMSectionBodyLen > uiEMMSectionDataLen)
		|| (iEMMSectionBodyLen <= (ISDBT_CAS_SECTION_HEADER_LENGTH + ISDBT_CAS_SECTION_CRC_LENGTH))) {
		ERR_PRINTF("[%s] Size of EMM section is invalid(%d, %d)\n", __func__, uiEMMSectionDataLen, iEMMSectionBodyLen);

		return TCC_TRMP_SUCCESS;
	}
	iEMMSectionBodyLen -= (ISDBT_CAS_SECTION_HEADER_LENGTH + ISDBT_CAS_SECTION_CRC_LENGTH);

	iProtocolNumber = pEMMSectionData[ISDBT_CAS_SECTION_HEADER_LENGTH+9] & 0x3f;
	if(iProtocolNumber != 0x00) {
		//ERR_PRINTF("[%s] EMM protocol number is invalid(%d)\n", __func__, iProtocolNumber);

		return TCC_TRMP_ERROR_INVALID_PROTOCOL;
	}

	iRet = tcc_trmp_manager_setEMM(usNetworkID, (unsigned char*)&(pEMMSectionData[ISDBT_CAS_SECTION_HEADER_LENGTH]), iEMMSectionBodyLen);
	if(iRet != TCC_TRMP_SUCCESS) {
		//ERR_PRINTF("[%s] Parsing EMM section is failed(%d)\n", __func__, iRet);
		
		return TCC_TRMP_SUCCESS;
	}

	return TCC_TRMP_SUCCESS;
}

int tcc_manager_trmp_parseECM(unsigned short usNetworkID, char *pECMSectionData, unsigned int uiECMSectionDataLen)
{
	unsigned char *pSystemKey, *pCBCDefaultValue;
	unsigned char aOddKey[TCC_TRMP_SCRAMBLE_KEY_LENGTH];
	unsigned char aEvenKey[TCC_TRMP_SCRAMBLE_KEY_LENGTH];
	int iECMSectionBodyLen=0;
	int iVersionNumber = 0;
	int iProtocolNumber = 0;
	int cipherType;
	int iRet;

	//DBG_PRINTF("[%s]\n", __func__);
	
	iVersionNumber = (0x3f&pECMSectionData[5])>>1;
	if(g_iECMVersionNumber == iVersionNumber) {
		//DBG_PRINTF("[%s] ECM secion already was parsed(%d)\n", __func__, g_iECMVersionNumber);

		return g_iPrevECMRetValue;
	}	
	
	if(usNetworkID == 0) {
		TCCDxbProc_Get_NetworkID(&usNetworkID);
	}

	iECMSectionBodyLen = ((pECMSectionData[1] & 0x0f) << 8) + pECMSectionData[2] + 3;
	if(((unsigned int)iECMSectionBodyLen > uiECMSectionDataLen)
		|| (iECMSectionBodyLen <= (ISDBT_CAS_SECTION_HEADER_LENGTH + ISDBT_CAS_SECTION_CRC_LENGTH))) {
		ERR_PRINTF("[%s] Size of ECM section is invalid(%d, %d)\n", __func__, uiECMSectionDataLen, iECMSectionBodyLen);

		return TCC_TRMP_ERROR_INVALID_PARAM;
	}
	iECMSectionBodyLen -= (ISDBT_CAS_SECTION_HEADER_LENGTH + ISDBT_CAS_SECTION_CRC_LENGTH);

	iProtocolNumber = pECMSectionData[ISDBT_CAS_SECTION_HEADER_LENGTH] & 0x3f;
	if(iProtocolNumber != 0x01) {
		//ERR_PRINTF("[%s] ECM protocol number is invalid(%d)\n", __func__, iProtocolNumber);

		return TCC_TRMP_ERROR_INVALID_PROTOCOL;
	}

	iRet = tcc_trmp_manager_setECM(usNetworkID, (unsigned char*)&(pECMSectionData[ISDBT_CAS_SECTION_HEADER_LENGTH]), iECMSectionBodyLen, aOddKey, aEvenKey);
	if(iRet != TCC_TRMP_SUCCESS) {
		//ERR_PRINTF("[%s] Parsing ECM section is failed(%d)\n", __func__, iRet);
		
		return iRet;
	}
	
	g_iECMVersionNumber = iVersionNumber;
	
	cipherType = tcc_manager_cas_getCipherType();
	if (cipherType == ISDBT_CAS_CIPHER_SW) {
		if(g_iMulti2Running == 0) {
			iRet = tcc_trmp_manager_getMulti2Info(&pSystemKey, &pCBCDefaultValue);
			if(iRet != TCC_TRMP_SUCCESS) {
				ERR_PRINTF("[%s] Failed to get multi2 information(%d)\n", __func__, iRet);
		
				return iRet;
			}
			
			iRet = TCC_DxB_CAS_OpenSWMulti2(0, 0, 0, 4, pSystemKey);
			if(iRet != 0) {
				ERR_PRINTF("[%s] Failed TCC_DxB_CAS_OpenSWMulti2(%d)\n", __func__, iRet);

				return TCC_TRMP_ERROR;
			}
		
			TCC_DxB_CAS_SetKeySWMulti2(0, aOddKey, BCAS_ECM_KS_LEN, pCBCDefaultValue, BCAS_CBC_INIT_LEN);
			TCC_DxB_CAS_SetKeySWMulti2(1, aEvenKey, BCAS_ECM_KS_LEN, pCBCDefaultValue, BCAS_CBC_INIT_LEN);

			g_iMulti2Running = 1;
		}
		else {
			TCC_DxB_CAS_SetKeySWMulti2(0, aOddKey, BCAS_ECM_KS_LEN, 0, 0);
			TCC_DxB_CAS_SetKeySWMulti2(1, aEvenKey, BCAS_ECM_KS_LEN, 0, 0);
		}
	}
	else if (cipherType == ISDBT_CAS_CIPHER_HW || cipherType == ISDBT_CAS_CIPHER_HWDEMUX) {
		if(g_iMulti2Running == 0) {
			
			iRet = TCC_DxB_CAS_Open(TCC_CHIPHER_ALGORITM_MULTI2, TCC_CHIPHER_OPMODE_CBC, 0, 4);
			if(iRet !=0) {
				ERR_PRINTF("error [%s][%d] \n", __func__, __LINE__);

				return TCC_TRMP_ERROR;
			}

			g_iMulti2Running = 1;
		}

		iRet = tcc_trmp_manager_getMulti2Info(&pSystemKey, &pCBCDefaultValue);
		if(iRet != TCC_TRMP_SUCCESS) {
			ERR_PRINTF("[%s] Failed to get multi2 information(%d)\n", __func__, iRet);
		
			return iRet;
		}

		iRet = TCC_DxB_CAS_SetKey(aEvenKey, BCAS_ECM_KS_LEN, pCBCDefaultValue, BCAS_CBC_INIT_LEN, pSystemKey, BCAS_SYSTEM_KEY_LEN, 0x02);
		if(iRet !=0) {
			ERR_PRINTF("error [%s][%d] \n", __func__, __LINE__);

			return TCC_TRMP_ERROR;
		}

		iRet = TCC_DxB_CAS_SetKey(aOddKey, BCAS_ECM_KS_LEN, pCBCDefaultValue, BCAS_CBC_INIT_LEN, pSystemKey, BCAS_SYSTEM_KEY_LEN, 0x03);
		if(iRet !=0) {
			ERR_PRINTF("error [%s][%d] \n", __func__, __LINE__);

			return TCC_TRMP_ERROR;
		}
	}
	else if (cipherType == ISDBT_CAS_CIPHER_TCC353X) {
		iRet = tcc_trmp_manager_getMulti2Info(&pSystemKey, &pCBCDefaultValue);
		if(iRet != TCC_TRMP_SUCCESS) {
			ERR_PRINTF("[%s] Failed to get multi2 information(%d)\n", __func__, iRet);
		
			return iRet;
		}

		if(g_iMulti2Running == 0) {
			unsigned int EsPids[4];
			int	i, count;

			tcc_manager_tuner_cas_open (4, pSystemKey);

			TCCDxBProc_Get_CASPIDInfo ((unsigned int *)&EsPids[0]);
			count = 0;
			for(i=0; i < 4; i++) 
			{
				if (EsPids[i] < 2 || EsPids[i] >= 0x1FFF)
					EsPids[i] = 0xFFFF;
				if (gTCCBB_CAS_PID[i] != EsPids[i]) {
					count++;
				}
			}
			if (count) {
				//back-up PIDs
				for(i=0; i < 4; i++) 
				{
					gTCCBB_CAS_PID[i] = EsPids[i];
				}

				count = 0; //count valid pid
				for(i=0; i < 4; i++) 
				{
					if (gTCCBB_CAS_PID[i] != 0xFFFF) {
						EsPids[count++] = gTCCBB_CAS_PID[i];
					}
				}

				tcc_manager_tuner_cas_set_pid (&EsPids[0], count);
			}

			tcc_manager_tuner_cas_key_multi2 (0, aOddKey, BCAS_ECM_KS_LEN, pCBCDefaultValue, BCAS_CBC_INIT_LEN);
			tcc_manager_tuner_cas_key_multi2 (1, aEvenKey, BCAS_ECM_KS_LEN, pCBCDefaultValue, BCAS_CBC_INIT_LEN);

			g_iMulti2Running = 1;
		}
		else {
			tcc_manager_tuner_cas_open (4, pSystemKey);
			tcc_manager_tuner_cas_key_multi2 (0, aOddKey, BCAS_ECM_KS_LEN, pCBCDefaultValue, BCAS_CBC_INIT_LEN);
			tcc_manager_tuner_cas_key_multi2 (1, aEvenKey, BCAS_ECM_KS_LEN, pCBCDefaultValue, BCAS_CBC_INIT_LEN);
		}
	}

	return TCC_TRMP_SUCCESS;
}

void* tcc_manager_trmp_decode(void *arg)
{
	void 	*pSectionBitPars = NULL;
	DEMUX_DEC_COMMAND_TYPE *pSectionCmd = NULL;
	struct timespec prevtimespec = { 0, }, curtimespec = { 0, };
	int iTableID = 0;
	int iChannelNumber = 0;
	int iNetworkID = 0;
	int *pArg = NULL;
	int iRet = 0;

	DBG_PRINTF("In %s\n", __func__);

	BITPARS_OpenBitParser( &pSectionBitPars );
	while(g_iTRMPThreadRunning)
	{
		if(tcc_dxb_getquenelem(TRMPDecoderQueue)){
			pSectionCmd = tcc_dxb_dequeue(TRMPDecoderQueue);
			if (pSectionCmd){
				if(pSectionCmd->uiDataSize > (ISDBT_CAS_SECTION_HEADER_LENGTH + ISDBT_CAS_SECTION_CRC_LENGTH)) {
					iTableID = (int)*pSectionCmd->pData;
					if(pSectionCmd->pArg != NULL) {
						pArg = (int*)pSectionCmd->pArg;
						iNetworkID = pArg[0];
						iChannelNumber = pArg[1] + 13;
					}
					else {
						iNetworkID = 0;
						iChannelNumber = 0;
					}
			
					switch(iTableID)
					{
						case ECM_0_ID:
						case ECM_1_ID:
							iRet = tcc_manager_trmp_parseECM(iNetworkID, (char*)pSectionCmd->pData, pSectionCmd->uiDataSize);
							if(g_iPrevECMRetValue != iRet) {
								if(iRet == TCC_TRMP_SUCCESS) {
									if(g_iPrevEMMRetValue == TCC_TRMP_SUCCESS) {
										TCCDxBEvent_TRMPErrorUpdate(TCC_TRMP_EVENT_OK, iChannelNumber);
									}
									prevtimespec.tv_sec = 0;
									g_iPrevECMRetValue = iRet;
								}
								else if(iRet == TCC_TRMP_ERROR_UPDATE_KEY) {
									TCCDxBEvent_TRMPErrorUpdate(TCC_TRMP_EVENT_ERROR_EC21, iChannelNumber);
									clock_gettime(CLOCK_MONOTONIC , &prevtimespec);
									if(prevtimespec.tv_sec == 0) {
										 prevtimespec.tv_sec = 1;
									}
									g_iPrevECMRetValue = iRet;
								}
								else if((iRet == TCC_TRMP_ERROR_INVALID_MEMORY) || (iRet == TCC_TRMP_ERROR_STORAGE)) {
									TCCDxBEvent_TRMPErrorUpdate(TCC_TRMP_EVENT_ERROR_UNEXPECTED, iChannelNumber);
									prevtimespec.tv_sec = 0;
									g_iPrevECMRetValue = iRet;
								}
								else if(iRet == TCC_TRMP_ERROR_CRC) {
									TCCDxBEvent_TRMPErrorUpdate(TCC_TRMP_EVENT_ERROR_CRC, iChannelNumber);
									g_iPrevECMRetValue = iRet;
								}
								else if(iRet == TCC_TRMP_ERROR_INVALID_PROTOCOL) {
									TCCDxBEvent_TRMPErrorUpdate(TCC_TRMP_EVENT_ERROR_PROTOCAL_NUMBER, iChannelNumber);
									g_iPrevECMRetValue = iRet;
								}
							}
							break;

						case EMM_0_ID:
						case EMM_1_ID:
							iRet = tcc_manager_trmp_parseEMM(iNetworkID, (char*)pSectionCmd->pData, pSectionCmd->uiDataSize);
							if(g_iPrevEMMRetValue != iRet) {
								if(iRet == TCC_TRMP_SUCCESS) {
									if(g_iPrevECMRetValue == TCC_TRMP_SUCCESS) {
										TCCDxBEvent_TRMPErrorUpdate(TCC_TRMP_EVENT_OK, iChannelNumber);
									}
									g_iPrevEMMRetValue = iRet;
								}
								else if(iRet == TCC_TRMP_ERROR_INVALID_PROTOCOL) {
									TCCDxBEvent_TRMPErrorUpdate(TCC_TRMP_EVENT_ERROR_PROTOCAL_NUMBER, iChannelNumber);
									g_iPrevEMMRetValue = iRet;
								}
							}
							break;
						
						default:
							break;
					}
				}
			
				if(pSectionCmd->pArg != NULL) {
					tcc_mw_free(__FUNCTION__, __LINE__, pSectionCmd->pArg);
				}
			    tcc_mw_free(__FUNCTION__, __LINE__, pSectionCmd->pData);
			    tcc_mw_free(__FUNCTION__, __LINE__, pSectionCmd);
			}
		}
		else{
			usleep(5000);
		}

		if(g_iPrevECMRetValue == TCC_TRMP_ERROR_UPDATE_KEY) {
			if(prevtimespec.tv_sec != 0) {
				clock_gettime(CLOCK_MONOTONIC , &curtimespec);
				if(curtimespec.tv_sec >= prevtimespec.tv_sec + ISDBT_CAS_EMM_TIME_LIMITE) {
					TCCDxBEvent_TRMPErrorUpdate(TCC_TRMP_EVENT_ERROR_EC22, iChannelNumber);
					prevtimespec.tv_sec = 0;
				}
			}
		}
	}

	BITPARS_CloseBitParser( pSectionBitPars );
	
	g_iTRMPThreadRunning = -1;

	return (void*)NULL;
}

int tcc_manager_trmp_init(void)
{
	int cipherType;
	int status;
	int i;
	int ret;

	for(i=0; i < 4; i++) {
		gTCCBB_CAS_PID[i] = 0xFFFF;
	}
	
	g_iECMVersionNumber = -1;

	ret = tcc_trmp_manager_init();
	if(ret != TCC_TRMP_SUCCESS) {
		ERR_PRINTF("[%s] Failed  tcc_trmp_manager_init\n", __func__);
		if(ret == TCC_TRMP_ERROR_TEE_OPEN_SESSION) {
			TCCDxBEvent_TRMPErrorUpdate(TCC_TRMP_EVENT_ERROR_LIBRARY_FALSIFICATION, 0);
		}

		return ISDBT_CAS_ERROR;
	}

	TRMPDecoderQueue = tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(tcc_dxb_queue_t));
	memset(TRMPDecoderQueue, 0, sizeof(tcc_dxb_queue_t));

	tcc_dxb_queue_init_ex(TRMPDecoderQueue, ISDBT_CAS_SECTION_QUEUESIZE);

	g_iTRMPThreadRunning = 1;
	status = tcc_dxb_thread_create((void *)&TRMPDecoderThreadID,
								tcc_manager_trmp_decode,
								(unsigned char*)"tcc_manager_trmp_decode",
								LOW_PRIORITY_10,
								NULL);
	if(status)	{
		ERR_PRINTF("[%s] Can't make tcc_manager_trmp_decode\n", __func__);
		tcc_dxb_queue_deinit(TRMPDecoderQueue);
		tcc_mw_free(__FUNCTION__, __LINE__, TRMPDecoderQueue);
		TRMPDecoderQueue = NULL;
		
		return ISDBT_CAS_ERROR;
	}

	cipherType = tcc_manager_cas_getCipherType();
	if (cipherType == ISDBT_CAS_CIPHER_SW) {
		TSDEMUXDSCApi_ModeSet(DSC_MODE_SW_MULTI2);
	} 
	else if (cipherType == ISDBT_CAS_CIPHER_HW) {
		tcc_trmp_manager_enableKeySwap(TRUE);
		TSDEMUXDSCApi_ModeSet(DSC_MODE_HW);
	}
	else if (cipherType == ISDBT_CAS_CIPHER_TCC353X) {
		tcc_trmp_manager_enableKeySwap(TRUE);
	}
	else if (cipherType == ISDBT_CAS_CIPHER_HWDEMUX) {
		TCC_DxB_SC_Manager_SetDescriptionMode(MULTI2_MODE_HWDEMUX);
		TSDEMUXDSCApi_ModeSet(DSC_MODE_HWDEMUX);
	}

	TCC_DxB_DEMUX_RegisterCasDecryptCallback(hInterface, 0, TSDEMUXDSCApi_Decrypt);
	
	g_iMulti2Running = 0;
	
	return ISDBT_CAS_SUCCESS;
}

int  tcc_manager_trmp_deinit(void)
{
	int cipherType;
	int status;
	int i;
	int ret;

	for(i=0; i < 4; i++) {
		gTCCBB_CAS_PID[i] = 0xFFFF;
	}

	if(g_iTRMPThreadRunning == 1) {
		g_iTRMPThreadRunning = 0;
		while(1)
		{
			if( g_iTRMPThreadRunning == -1) {
				break;
			}
			
			usleep(5000);
		}
		status = tcc_dxb_thread_join((void *)TRMPDecoderThreadID,NULL);
		if(status != 0) {
			ERR_PRINTF("[%s] Failed tcc_dxb_thread_join(%d)\n", __func__, status);	
		}
	}

	cipherType = tcc_manager_cas_getCipherType();

#if 1
	// Noah, To avoid Codesonar's warning, Redundant Condition
	// ISDBT_CAS_CIPHER_SW is not one of return value of tcc_manager_cas_getCipherType().
	if (cipherType == ISDBT_CAS_CIPHER_HW || cipherType == ISDBT_CAS_CIPHER_HWDEMUX) {
		TCC_DxB_CAS_Close();
	} else if (cipherType == ISDBT_CAS_CIPHER_TCC353X) {
		//blank in intention
	}
#else
	if (cipherType == ISDBT_CAS_CIPHER_SW) {
		TCC_DxB_CAS_CloseSWMulti2();
	} else if (cipherType == ISDBT_CAS_CIPHER_HW || cipherType == ISDBT_CAS_CIPHER_HWDEMUX) {
		TCC_DxB_CAS_Close();
	} else if (cipherType == ISDBT_CAS_CIPHER_TCC353X) {
		//blank in intention
	}
#endif

	g_iMulti2Running = 0;
	
	if(TRMPDecoderQueue != NULL) {
		tcc_dxb_queue_deinit(TRMPDecoderQueue);
		tcc_mw_free(__FUNCTION__, __LINE__, TRMPDecoderQueue);
		TRMPDecoderQueue = NULL;
	}

	tcc_trmp_manager_deinit();

	return ISDBT_CAS_SUCCESS;
}

int tcc_manager_trmp_reset(void)
{
	g_iECMVersionNumber = -1;
	g_iPrevECMRetValue = TCC_TRMP_SUCCESS;
	g_iPrevEMMRetValue = TCC_TRMP_SUCCESS;
	
	ISDBT_Reset_AC_DescriptorInfo();
	
	return ISDBT_CAS_SUCCESS;
}

int tcc_manager_trmp_resetInfo(void)
{
	int ret;
	
	ret = tcc_trmp_manager_reset();
	if(ret != TCC_TRMP_SUCCESS) {
		ERR_PRINTF("[%s] Failed  tcc_manager_trmp_resetInfo!!!\n", __func__);
		return ISDBT_CAS_ERROR;
	}
	
	return ISDBT_CAS_SUCCESS;
}

#if 0   // sunny : not use
int tcc_manager_trmp_setDeviceKeyUpdateFunction(updateDeviceKey func)
{
	tcc_trmp_manager_setDeviceKeyUpdateFunction(func);

	return ISDBT_CAS_SUCCESS;
}
#endif

int tcc_manager_trmp_getInfo(void **ppInfo, int *pInfoSize)
{
	tcc_trmp_manager_getIDInfo((unsigned char **)ppInfo, pInfoSize);

	return ISDBT_CAS_SUCCESS;
}

/****************************************************************************
DEFINITION OF CAS
****************************************************************************/

int tcc_manager_cas_pushSectionData(void *data)
{
	if (TCC_Isdbt_IsSupportBCAS()) {
		if((BCasDecoderQueue != NULL) && (tcc_dxb_queue_ex(BCasDecoderQueue, data) == 0))
		{
			DEMUX_DEC_COMMAND_TYPE *pSectionCmd = (DEMUX_DEC_COMMAND_TYPE *)data;
			ERR_PRINTF("[%s] BQ full (reqId:0x%x, TableId:0x%x\n",__func__, pSectionCmd->iRequestID, *pSectionCmd->pData);
			
			return ISDBT_CAS_ERROR_QUEUE_FULL;
		}
	}
	else if (TCC_Isdbt_IsSupportTRMP()) {
		if((TRMPDecoderQueue != NULL) && (tcc_dxb_queue_ex(TRMPDecoderQueue, data) == 0))
		{
			DEMUX_DEC_COMMAND_TYPE *pSectionCmd = (DEMUX_DEC_COMMAND_TYPE *)data;
			ERR_PRINTF("[%s] BQ full (reqId:0x%x, TableId:0x%x\n",__func__, pSectionCmd->iRequestID, *pSectionCmd->pData);
			
			return ISDBT_CAS_ERROR_QUEUE_FULL;
		}
	}

	return ISDBT_CAS_SUCCESS;
}

int tcc_manager_cas_init(void)
{
	int ret = ISDBT_CAS_ERROR_UNSUPPORTED;
	
	if (TCC_Isdbt_IsSupportBCAS()) {
		ret= tcc_manager_bcas_init();
	}
	else if (TCC_Isdbt_IsSupportTRMP()) {
		ret = tcc_manager_trmp_init();
	}
	else {
		ERR_PRINTF("[%s] CAS is not supported\n", __func__);
	}
	
	return ret;
}

int tcc_manager_cas_deinit(void)
{
	int ret = ISDBT_CAS_ERROR_UNSUPPORTED;

	if (TCC_Isdbt_IsSupportBCAS()) {
		tcc_manager_bcas_deinit();
	}
	else if (TCC_Isdbt_IsSupportTRMP()) {
		tcc_manager_trmp_deinit();
	}
	else {
		ERR_PRINTF("[%s] CAS is not supported\n", __func__);
	}
	
	return ret;
}

int tcc_manager_cas_reset(void)
{
	int ret = ISDBT_CAS_ERROR_UNSUPPORTED;

	if (TCC_Isdbt_IsSupportBCAS()) {
		ret = tcc_manager_bcas_reset();
	}
	else if (TCC_Isdbt_IsSupportTRMP()) {
		ret = tcc_manager_trmp_reset();
	}

	return ret;;
}

int tcc_manager_cas_resetInfo(void)
{
	int ret = ISDBT_CAS_ERROR_UNSUPPORTED;

	if (TCC_Isdbt_IsSupportTRMP()) {
		ret = tcc_manager_trmp_resetInfo();
	}
	
	return ret;
}

#if 0   // sunny : not use
int tcc_manager_cas_setDeviceKeyUpdateFunction(updateDeviceKey func)
{
	if (TCC_Isdbt_IsSupportTRMP()) {
		tcc_manager_trmp_setDeviceKeyUpdateFunction(func);

		return ISDBT_CAS_SUCCESS;
	}
	
	return ISDBT_CAS_ERROR;
}
#endif

int tcc_manager_cas_getInfo(void **ppInfo, int *pInfoSize)
{
	int ret = ISDBT_CAS_ERROR_UNSUPPORTED;

	if (TCC_Isdbt_IsSupportBCAS()) {
		ret = tcc_manager_bcas_getSCInfo(ppInfo, pInfoSize);
	}
	else if (TCC_Isdbt_IsSupportTRMP()) {
		ret = tcc_manager_trmp_getInfo(ppInfo, pInfoSize);
	}
	
	return ret;
}

int tcc_manager_cas_getCipherType (void)
{
	if (TCC_Isdbt_IsSupportCAS()) {
		if (TCC_Isdbt_IsSupportDecryptTCC353x()) {
			return ISDBT_CAS_CIPHER_TCC353X;
		}
		else if	(TCC_Isdbt_IsSupportDecryptHWDEMUX()) {
			return ISDBT_CAS_CIPHER_HWDEMUX;
		}
		else if (TCC_Isdbt_IsSupportDecryptACPU()) {
			return ISDBT_CAS_CIPHER_HW;
		}
	} 

	return ISDBT_CAS_ERROR_UNSUPPORTED;
}

