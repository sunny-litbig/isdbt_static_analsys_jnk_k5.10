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
#define LOG_NDEBUG 0
#define LOG_TAG	"ISDBT_Event"


#include <utils/Log.h>
#include <cutils/properties.h>

#ifdef HAVE_LINUX_PLATFORM
#include <string.h> /* for strlen()*/
#endif

#include "tcc_msg.h"
#include "tcc_isdbt_proc.h"
#include "tcc_isdbt_event.h"
#include "tcc_dxb_memory.h"

#define   DEBUG_MSG(msg...)  ALOGD(msg)

typedef    void (*pEventFunc)(void *arg);
typedef struct {
    pEventFunc pNotifyFunc;
    unsigned int uiFlgFreeArg; //0:don't free *arg, 1:free *arg
    void *arg; //argument
}ST_DXBPROC_EVENT;
static TccMessage *gpDxBProcEvent;

void notify_search_complete(void *pResult);
void notify_search_percent_update(void *pResult);
void notify_video_frame_output_start(void *pResult);
void notify_video_error(void *pResult);
void notify_channel_update (void *pResult);
void notify_emergency_info_update(void *pResult);
void notify_subtitle_linux_update(void *pResult);
void notify_video_definition_update(void *pResult);
void notify_epg_update(void *pResult);
void notify_autosearch_update (void *ptr);
void notify_autosearch_done (void *ptr);
void notify_desc_update(void *arg);
void notify_event_relay(void *arg);
void notify_mvtv_update(void *arg);
void notify_serviceID_check(void *pResult);
void notify_trmperror_update(void *pResult);
void notify_customsearch_status (void *pResult);
void notify_specialservice_update (void *pResult);
void notify_av_index_update(void *pResult);
void notify_section_update(void *pResult);
void notify_search_and_setchannel(void *pResult);
void notify_detect_service_num_change(void *arg);
void notify_tuner_custom_event (void *pResult);
void notify_section_data_update(void *pResult);
void notify_custom_filter_data_update(void *pResult);
void notify_video_output_start_stop(void *pResult);
void notify_audio_output_start_stop(void *pResult);

int TCCDxBEvent_Init(void)
{
    DEBUG_MSG("%s::%d",__func__, __LINE__);
    gpDxBProcEvent = new TccMessage(100);
    return 0;
}

int TCCDxBEvent_DeInit(void)
{
	unsigned int i, uielement;
	ST_DXBPROC_EVENT *pEvent;

	uielement = gpDxBProcEvent->TccMessageGetCount();
	for (i = 0;i < uielement;i++)
	{
		pEvent =(ST_DXBPROC_EVENT *)gpDxBProcEvent->TccMessageGet();
		if(pEvent->arg && pEvent->uiFlgFreeArg)
		{
			tcc_mw_free(__FUNCTION__, __LINE__, pEvent->arg);
		}

		tcc_mw_free(__FUNCTION__, __LINE__, pEvent);
	}

	delete gpDxBProcEvent;

	DEBUG_MSG("%s::%d",__func__, __LINE__);
	return 0;
}

unsigned int TCCDxBEvent_SearchingProgress(void *arg, int ch)
{
    int *pArg;
    ST_DXBPROC_EVENT *pEvent;
    DEBUG_MSG("%s::%d::[%d]",__func__, __LINE__, (int)arg);
    pEvent = (ST_DXBPROC_EVENT*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_EVENT));
    pArg = (int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(int) * 2);
    pArg[0] = (int)arg;
    pArg[1] = ch;
    pEvent->pNotifyFunc = (pEventFunc)notify_search_percent_update;
    pEvent->arg = pArg;
    pEvent->uiFlgFreeArg = 1;
    if(gpDxBProcEvent->TccMessagePut((void *)pEvent)) {
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent->arg);
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent);
    }
    return 0;
}

unsigned int TCCDxBEvent_SearchingDone(void *arg)
{
	int *pArg;
    ST_DXBPROC_EVENT *pEvent;
    DEBUG_MSG("%s::%d::[%d]",__func__, __LINE__, *(int*)arg);
    pEvent = (ST_DXBPROC_EVENT*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_EVENT));
	pArg = (int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(int));
	pArg[0] = *(int*)arg;
    pEvent->pNotifyFunc = (pEventFunc)notify_search_complete;
    pEvent->arg = pArg;
    pEvent->uiFlgFreeArg = 1;
    if(gpDxBProcEvent->TccMessagePut((void *)pEvent)) {
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent->arg);
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent);
    }
    return 0;
}

unsigned int TCCDxBEvent_FirstFrameDisplayed(void *arg)
{
    ST_DXBPROC_EVENT *pEvent;
    DEBUG_MSG("%s::%d::[%d]",__func__, __LINE__, (int)arg);
    pEvent = (ST_DXBPROC_EVENT*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_EVENT));
    pEvent->pNotifyFunc = (pEventFunc)notify_video_frame_output_start;
    pEvent->arg = arg;
    pEvent->uiFlgFreeArg = 0;
    if(gpDxBProcEvent->TccMessagePut((void *)pEvent)) {
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent);
    }
    return 0;
}

unsigned int TCCDxBEvent_VideoError(void *arg)
{
    ST_DXBPROC_EVENT *pEvent;
    DEBUG_MSG("%s::%d::[%d]",__func__, __LINE__, (int)arg);
    pEvent = (ST_DXBPROC_EVENT*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_EVENT));
    pEvent->pNotifyFunc = (pEventFunc)notify_video_error;
    pEvent->arg = arg;
    pEvent->uiFlgFreeArg = 0;
    if(gpDxBProcEvent->TccMessagePut((void *)pEvent)) {
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent);
    }
    return 0;
}

unsigned int TCCDxBEvent_ChannelUpdate(void *arg)
{
	int	*pArg;
    ST_DXBPROC_EVENT *pEvent;
    DEBUG_MSG("%s::%d::[%d]",__func__, __LINE__, (int)arg);
    pEvent = (ST_DXBPROC_EVENT*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_EVENT));
    pEvent->pNotifyFunc = (pEventFunc)notify_channel_update;

	pArg = (int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(int));
	if (arg == NULL)	pArg[0] = 0;
	else pArg[0] = *(int*)arg; /* 0=success, 1=canceled, -1=error */
	pEvent->arg = pArg;
	pEvent->uiFlgFreeArg = 1;

	DEBUG_MSG("%s::%d::[%d]",__func__, __LINE__, (int)pArg[0]);

    if(gpDxBProcEvent->TccMessagePut((void *)pEvent)) {
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent->arg);
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent);
    }
    return 0;
}

unsigned int TCCDxBEvent_Emergency_Info_Update(void *arg)
{
	int *pData, *pArg;
	ST_DXBPROC_EVENT *pEvent;
	DEBUG_MSG("%s::%d::[0x%p]",__func__, __LINE__, (int *)arg);
	pEvent = (ST_DXBPROC_EVENT*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_EVENT));
#if 0
	pData = (int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(int) * (256+5));
#else
	pData = (int*)tcc_mw_calloc(__FUNCTION__, __LINE__, 1, sizeof(int) * (256+5));
#endif

	int *pAreaCode, area_code_length, i;

	pArg = (int*)arg;

	pData[0] = (int)pArg[0];	// 1: start, 0:stop
	pData[1] = (int)pArg[1];	// service_id
	pData[2] = (int)pArg[2];	// start_end_flag
	pData[3] = (int)pArg[3];	// signal_type
	pData[4] = (int)pArg[4];	// area_code_length
	pAreaCode = (int *)pArg[5];	// pointer to area_code
	area_code_length = (int)pArg[4];
	if (pAreaCode != NULL) {
		for(i=0; i < 256 && i < area_code_length; i++)
		{
			pData[5+i] = *pAreaCode++;
		}
	} else
		pData[4] = 0;

	pEvent->pNotifyFunc = (pEventFunc)notify_emergency_info_update;
	pEvent->arg = pData;
	pEvent->uiFlgFreeArg = 1;
	if(gpDxBProcEvent->TccMessagePut((void *)pEvent)) {
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent->arg);
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent);
    }

	return 0;
}

unsigned int TCCDxBEvent_SubtitleUpdateLinux(void *arg)
{
	int *pData, *pArg, *phy_addr;
	ST_DXBPROC_EVENT *pEvent;
	pEvent = (ST_DXBPROC_EVENT*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_EVENT));
#if 0
	pData = (int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(int) * 3);
#else
	pData = (int*)tcc_mw_calloc(__FUNCTION__, __LINE__, 1, sizeof(int) * 3);
#endif

	pArg = (int*)arg;
	pData[0] = (int)pArg[0]; //internal_sound_index
	phy_addr = (int*)pArg[1]; //mixed subtitle for phy addr
	if (phy_addr != NULL) {
		pData[1] = *phy_addr;
		ALOGE("[%s]physical addr:%p", __func__, pData[1]);
	}
	pData[2] = (int)pArg[2]; //mixed subtitle phy memory size about one handle
	pEvent->pNotifyFunc = (pEventFunc)notify_subtitle_linux_update;
	pEvent->arg = pData;
	pEvent->uiFlgFreeArg = 1;
	if(gpDxBProcEvent->TccMessagePut((void *)pEvent)) {
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent->arg);
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent);
	}
	return 0;
}

unsigned int TCCDxBEvent_VideoDefinitionUpdate(void *arg)
{
    ST_DXBPROC_EVENT *pEvent;
    //DEBUG_MSG("%s::%d::[%d]",__func__, __LINE__, (int)arg);
    pEvent = (ST_DXBPROC_EVENT*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_EVENT));
    pEvent->pNotifyFunc = (pEventFunc)notify_video_definition_update;
    pEvent->arg = arg;
    pEvent->uiFlgFreeArg = 0;
    if(gpDxBProcEvent->TccMessagePut((void *)pEvent)) {
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent);
    }
	return 0;
}

unsigned int TCCDxBEvent_EPGUpdate(int service_id, int tableID)
{
	int *pArg;
    ST_DXBPROC_EVENT *pEvent;
    DEBUG_MSG("%s::%d::[%d]",__func__, __LINE__, service_id);
    pEvent = (ST_DXBPROC_EVENT*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_EVENT));
    pArg = (int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(int) * 2);
    pArg[0] = service_id;
    pArg[1] = tableID;
    pEvent->pNotifyFunc = (pEventFunc)notify_epg_update;
    pEvent->arg = pArg;
    pEvent->uiFlgFreeArg = 1;
    if(gpDxBProcEvent->TccMessagePut((void *)pEvent)) {
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent->arg);
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent);
    }
    return 0;
}

unsigned int TCCDxBEvent_AutoSearchProgress(void *arg)
{
	ST_DXBPROC_EVENT *pEvent;

	pEvent = (ST_DXBPROC_EVENT*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_EVENT));
	pEvent->pNotifyFunc = (pEventFunc)notify_autosearch_update;
	pEvent->arg = arg;
	pEvent->uiFlgFreeArg = 0;
	if(gpDxBProcEvent->TccMessagePut((void *)pEvent)) {
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent);
    }
	return 0;
}

unsigned int TCCDxBEvent_AutoSearchDone(void *arg)
{
	ST_DXBPROC_EVENT *pEvent;

	pEvent = (ST_DXBPROC_EVENT*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_EVENT));
	pEvent->pNotifyFunc = (pEventFunc)notify_autosearch_done;
	pEvent->arg = arg;
	pEvent->uiFlgFreeArg = 0;
	if(gpDxBProcEvent->TccMessagePut((void *)pEvent)) {
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent);
    }
	return 0;
}

unsigned int TCCDxBEvent_DESCUpdate(unsigned short usServiceID, unsigned char ucDescID, void *arg)
{
	ST_DXBPROC_EVENT *pEvent;
	int *pArg;
	int *pArgTemp = (int *)arg;

	pEvent = (ST_DXBPROC_EVENT*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_EVENT));
	pEvent->pNotifyFunc = (pEventFunc)notify_desc_update;
	pArg = (int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(int) * 9);

	switch(ucDescID)
	{
		case 0xC1://DIGITALCOPY_CONTROL_ID:
		{
			// Important !!!!! : stDescDCC(stSubDescDCC) memory is freed in notify_desc_update/case 0xc1.

			T_DESC_DCC* descDcc = (T_DESC_DCC*)pArgTemp[0];

			T_DESC_DCC* stDescDCC = (T_DESC_DCC*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(T_DESC_DCC));
			if(0 == stDescDCC)
			{
				ALOGE("[%s][%d] : T_DESC_DCC malloc fail !!!!!\n", __func__, __LINE__);
				return -1;
			}
			memset(stDescDCC, 0, sizeof(T_DESC_DCC));

			/* Set the descriptor information. */
			//stDescDCC.ucTableID 					 = E_TABLE_ID_XXX;
			stDescDCC->digital_recording_control_data = descDcc->digital_recording_control_data;
			stDescDCC->maximum_bitrate_flag			 = descDcc->maximum_bitrate_flag;
			stDescDCC->component_control_flag		 = descDcc->component_control_flag;
			stDescDCC->copy_control_type 			 = descDcc->copy_control_type;
			stDescDCC->APS_control_data				 = descDcc->APS_control_data;
			stDescDCC->maximum_bitrate				 = descDcc->maximum_bitrate;
			stDescDCC->sub_info = 0;

			if( descDcc->sub_info )
			{
				SUB_T_DESC_DCC* stSubDescDCC;
				SUB_T_DESC_DCC* backup_sub_info;
				SUB_T_DESC_DCC* sub_info = descDcc->sub_info;

				while(sub_info)
				{
					stSubDescDCC = (SUB_T_DESC_DCC*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(SUB_T_DESC_DCC));
					if(0 == stSubDescDCC)
					{
						ALOGE("[%s][%d] : SUB_T_DESC_DCC malloc fail !!!!!\n", __func__, __LINE__);
						return -1;
					}
					memset(stSubDescDCC, 0, sizeof(SUB_T_DESC_DCC));

					stSubDescDCC->component_tag 				 = sub_info->component_tag;
					stSubDescDCC->digital_recording_control_data = sub_info->digital_recording_control_data;
					stSubDescDCC->maximum_bitrate_flag			 = sub_info->maximum_bitrate_flag;
					stSubDescDCC->copy_control_type 			 = sub_info->copy_control_type;
					stSubDescDCC->APS_control_data				 = sub_info->APS_control_data;
					stSubDescDCC->maximum_bitrate				 = sub_info->maximum_bitrate;

					if(0 == stDescDCC->sub_info)
					{
						stDescDCC->sub_info = stSubDescDCC;
						backup_sub_info = stSubDescDCC;
					}
					else
					{
						backup_sub_info->pNext = stSubDescDCC;
						backup_sub_info = stSubDescDCC;
					}

					sub_info = sub_info->pNext;
				}
			}

			pArg[0] = (int)usServiceID;
			pArg[1] = (int)ucDescID;

			pArg[2] = (int)stDescDCC;
		}
		break;

		case 0xDE://CONTENT_AVAILABILITY_ID:
			pArg[0] = (int)usServiceID;
			pArg[1] = (int)ucDescID;

			pArg[2] = (int)pArgTemp[0];
			pArg[3] = (int)pArgTemp[1];
			pArg[4] = (int)pArgTemp[2];
			pArg[5] = (int)pArgTemp[3];
			pArg[6] = (int)pArgTemp[4];
		break;

		default:
		break;
	}

	pEvent->arg = pArg;
	pEvent->uiFlgFreeArg = 1;
	if(gpDxBProcEvent->TccMessagePut((void *)pEvent)) {
		if(ucDescID == 0xC1)    //DIGITALCOPY_CONTROL_ID
		{
			T_DESC_DCC* descDcc = (T_DESC_DCC*)pArg[2];

			if(descDcc->sub_info)
			{
				SUB_T_DESC_DCC* sub_info = descDcc->sub_info;
				while(sub_info)
				{
					SUB_T_DESC_DCC* temp_sub_info = sub_info->pNext;
					tcc_mw_free(__FUNCTION__, __LINE__, sub_info);
					sub_info = temp_sub_info;
				}
			}

			tcc_mw_free(__FUNCTION__, __LINE__, descDcc);
		}

		tcc_mw_free(__FUNCTION__, __LINE__, pEvent->arg);
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent);
    }

	return 0;
}

unsigned int TCCDxBEvent_EventRelay(unsigned short usServiceID, unsigned short usEventID, unsigned short usOriginalNetworkID, unsigned short usTransportStreamID)
{
	ST_DXBPROC_EVENT *pEvent;
	int *pArg;

	pEvent = (ST_DXBPROC_EVENT*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_EVENT));
	pEvent->pNotifyFunc = (pEventFunc)notify_event_relay;
	pArg = (int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(int) * 4);
	pArg[0] = (int)usServiceID;
	pArg[1] = (int)usEventID;
	pArg[2] = (int)usOriginalNetworkID;
	pArg[3] = (int)usTransportStreamID;
	pEvent->arg = pArg;
	pEvent->uiFlgFreeArg = 1;
	if(gpDxBProcEvent->TccMessagePut((void *)pEvent)) {
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent->arg);
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent);
    }
	return 0;
}

unsigned int TCCDxBEvent_MVTVUpdate(unsigned char *pucArr)
{
	ST_DXBPROC_EVENT *pEvent;
	int *pArg;
	pEvent = (ST_DXBPROC_EVENT*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_EVENT));
	pEvent->pNotifyFunc = (pEventFunc)notify_mvtv_update;
	pArg = (int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(int) * 5);
	pArg[0] = (int)pucArr[0];	//component_group_type
	pArg[1] = (int)pucArr[1];	//num_of_group
	pArg[2] = (int)pucArr[2];	//main_component_group_id
	pArg[3] = (int)pucArr[3];	//sub1_component_group_id
	pArg[4] = (int)pucArr[4];	//sub2_component_group_id
	pEvent->arg = pArg;
	pEvent->uiFlgFreeArg = 1;
	if(gpDxBProcEvent->TccMessagePut((void *)pEvent)) {
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent->arg);
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent);
    }
	return 0;
}

int TCCDxBEvent_ServiceID_Check(int valid)
{
	//ALOGE("[#][%s] [%d]\n", __func__, valid);
	ST_DXBPROC_EVENT *pEvent;
	int *pArg;
	pEvent = (ST_DXBPROC_EVENT*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_EVENT));
	pEvent->pNotifyFunc = (pEventFunc)notify_serviceID_check;
	pArg = (int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(int));
	pArg[0] = valid;
	pEvent->arg = pArg;
	pEvent->uiFlgFreeArg = 1;
	if(gpDxBProcEvent->TccMessagePut((void *)pEvent)) {
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent->arg);
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent);
    }
	return 0;
}

unsigned int TCCDxBEvent_TRMPErrorUpdate(int status, int ch)
{
    int *pArg;
    ST_DXBPROC_EVENT *pEvent;
    //DEBUG_MSG("%s::%d::[%d]",__func__, __LINE__, (int)arg);
    pEvent = (ST_DXBPROC_EVENT*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_EVENT));
    pArg = (int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(int) * 2);
    pArg[0] = status;
    pArg[1] = ch;
    pEvent->pNotifyFunc = (pEventFunc)notify_trmperror_update;
    pEvent->arg = pArg;
    pEvent->uiFlgFreeArg = 1;
    if(gpDxBProcEvent->TccMessagePut((void *)pEvent)) {
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent->arg);
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent);
    }
    return 0;
}

/*
  * status -1=failure, 1=success, 2=cancel, 3=on-going
  */
int TCCDxBEvent_CustomSearchStatus(int status, int *arg)
{
	ST_DXBPROC_EVENT *pEvent;
	int *pArg, *p = arg;
	int i;

	if (arg == NULL) {
		DEBUG_MSG("TCCDxBEvent_CustomSearchStatus, result=%d, arg error", status);
		return -1;
	} else  {
		DEBUG_MSG("TCCDxBEvent_CustomSearchStatus, result=%d, status=%d", status, *arg);
	}

	pArg = (int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(int) * (32+6+1)); //last 1=status, 32+6=ISDBTPlayer::CustomSearchInfoData
	pArg[0] = status;
	for (i=0; i < (32+6); i++)
		pArg[1+i] = *p++;

	pEvent = (ST_DXBPROC_EVENT*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_EVENT));
	pEvent->pNotifyFunc = (pEventFunc)notify_customsearch_status;
	pEvent->arg = (void *)pArg;
	pEvent->uiFlgFreeArg = 1;
	if(gpDxBProcEvent->TccMessagePut((void *)pEvent)) {
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent->arg);
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent);
    }
	return 0;
}

/*
  *  status = 1 : special service start, 2 : special service finished
  * _id = value of _id of   special service in channel DB when a special service start
  */
int TCCDxBEvent_SpecialServiceUpdate (int status, int _id)
{
	ST_DXBPROC_EVENT *pEvent;
	int *pArg;

	pArg = (int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(int) * 2);
	pArg[0] = status;
	pArg[1] = _id;

	pEvent = (ST_DXBPROC_EVENT*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_EVENT));
	pEvent->pNotifyFunc = (pEventFunc)notify_specialservice_update;
	pEvent->arg = (void*)pArg;
	pEvent->uiFlgFreeArg = 1;
	if(gpDxBProcEvent->TccMessagePut((void *)pEvent)) {
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent->arg);
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent);
	}
	return 0;
}

unsigned int TCCDxBEvent_AVIndexUpdate(unsigned int channelNumber, unsigned int networkID, \
											unsigned int serviceID, unsigned int serviceSubID, \
											unsigned int audioIndex, unsigned int videoIndex)
{
    int *pArg;
    ST_DXBPROC_EVENT *pEvent;

    //DEBUG_MSG("%s::%d::[%d]",__func__, __LINE__, index);
    pEvent = (ST_DXBPROC_EVENT*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_EVENT));
    pArg = (int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(int) * 6);

    pArg[0] = (int)channelNumber;
    pArg[1] = (int)networkID;
    pArg[2] = (int)serviceID;
    pArg[3] = (int)serviceSubID;
    pArg[4] = (int)audioIndex;
    pArg[5] = (int)videoIndex;

    pEvent->pNotifyFunc = (pEventFunc)notify_av_index_update;
    pEvent->arg = pArg;
    pEvent->uiFlgFreeArg = 1;
    if(gpDxBProcEvent->TccMessagePut((void *)pEvent)) {
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent->arg);
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent);
    }
	return 0;
}

unsigned int TCCDxBEvent_SectionUpdate(unsigned int channelNumber, unsigned int sectionType)
{
    int *pArg;
    ST_DXBPROC_EVENT *pEvent;

    //DEBUG_MSG("%s::%d::[%d]",__func__, __LINE__, index);
    pEvent = (ST_DXBPROC_EVENT*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_EVENT));
    pArg = (int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(int) * 2);

    pArg[0] = (int)channelNumber;
    pArg[1] = (int)sectionType;

    pEvent->pNotifyFunc = (pEventFunc)notify_section_update;
    pEvent->arg = pArg;
    pEvent->uiFlgFreeArg = 1;
    if(gpDxBProcEvent->TccMessagePut((void *)pEvent)) {
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent->arg);
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent);
    }
    return 0;
}

unsigned int TCCDxBEvent_TunerCustomEvent (void *pEventData)
{
	ST_DXBPROC_EVENT *pEvent;

	pEvent = (ST_DXBPROC_EVENT*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_EVENT));
	pEvent->pNotifyFunc = (pEventFunc)notify_tuner_custom_event;
	pEvent->arg = pEventData;
	pEvent->uiFlgFreeArg = 0;
    if(gpDxBProcEvent->TccMessagePut((void *)pEvent)) {
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent);
    }

	return 0;
}

unsigned int TCCDxBEvent_SearchAndSetChannel(unsigned int status, unsigned int countryCode, unsigned int channelNumber, unsigned int mainRowID, unsigned int subRowID)
{
    int *pArg;
    ST_DXBPROC_EVENT *pEvent;

    //DEBUG_MSG("%s::%d::[%d]",__func__, __LINE__, index);
    pEvent = (ST_DXBPROC_EVENT*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_EVENT));
    pArg = (int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(int) * 5);

    pArg[0] = (int)status;
    pArg[1] = (int)countryCode;
    pArg[2] = (int)channelNumber;
    pArg[3] = (int)mainRowID;
    pArg[4] = (int)subRowID;

    pEvent->pNotifyFunc = (pEventFunc)notify_search_and_setchannel;
    pEvent->arg = pArg;
    pEvent->uiFlgFreeArg = 1;
    if(gpDxBProcEvent->TccMessagePut((void *)pEvent)) {
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent->arg);
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent);
    }
    return 0;
}

unsigned int TCCDxBEvent_DetectServiceNumChange(void)
{
	ST_DXBPROC_EVENT *pEvent;
	DEBUG_MSG("%s::%d::",__func__, __LINE__);
	pEvent = (ST_DXBPROC_EVENT*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_EVENT));
	pEvent->pNotifyFunc = (pEventFunc)notify_detect_service_num_change;
	pEvent->uiFlgFreeArg = 0;
	if(gpDxBProcEvent->TccMessagePut((void *)pEvent)) {
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent);
	}
	return 0;
}

unsigned int TCCDxBEvent_SectionDataUpdate(unsigned short usServiceID, unsigned short usTableID, unsigned int uiRawDataLen, unsigned char *pucRawData, unsigned short usNetworkID, unsigned short usTransportStreamID, unsigned short usDataServicePID, unsigned char auto_start_flag)
{
	int *pArg;
	ST_DXBPROC_EVENT *pEvent;
	DEBUG_MSG("%s::%d::",__func__, __LINE__);
	pEvent = (ST_DXBPROC_EVENT*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_EVENT));

	pArg = (int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(int) * (8 + (uiRawDataLen/sizeof(int))));
	pArg[0] = (int)usServiceID;
	pArg[1] = (int)usTableID;
	pArg[2] = (int)usNetworkID;
	pArg[3] = (int)usTransportStreamID;
	pArg[4] = (int)usDataServicePID;
	pArg[5] = (int)auto_start_flag;
	pArg[6] = (int)uiRawDataLen;
	memcpy(pArg+7, pucRawData, uiRawDataLen);
	pEvent->pNotifyFunc = (pEventFunc)notify_section_data_update;
	pEvent->arg = (void *)pArg;
    pEvent->uiFlgFreeArg = 1;
	LLOGD("[TCCDxBEvent_SectionDataUpdate] SID(%d) TID(%d) NID(%d) TSID(%d) DS_SID(%d) asFlag(%d) RDLen(%d) \n", pArg[0], pArg[1], pArg[2], pArg[3], pArg[4], pArg[5], pArg[6]);

	if(gpDxBProcEvent->TccMessagePut((void *)pEvent)) {
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent->arg);
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent);
	}
	return 0;
}

unsigned int TCCDxBEvent_CustomFilterDataUpdate(int iPID, unsigned char *pucBuf, unsigned int uiBufSize)
{
	int *pArg;
	ST_DXBPROC_EVENT *pEvent;
	//DEBUG_MSG("%s::%d::",__func__, __LINE__);
	pEvent = (ST_DXBPROC_EVENT*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_EVENT));

	pArg = (int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(int) * (3 + (uiBufSize/sizeof(int)))); // 2 = (1)iPID + (1)uiBufSize + (1)remain byte of uiBufSize/sizeof(int)
	pArg[0] = iPID;
	pArg[1] = uiBufSize;
	memcpy(pArg+2, pucBuf, uiBufSize); // 2 = (1)iPID + (1)uiBufSize
	pEvent->pNotifyFunc = (pEventFunc)notify_custom_filter_data_update;
	pEvent->arg = (void *)pArg;
	pEvent->uiFlgFreeArg = 1;
	if(gpDxBProcEvent->TccMessagePut((void *)pEvent)) {
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent->arg);
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent);
	}
	return 0;
}

unsigned int TCCDxBEvent_VideoStartStop(int iStart)
{
	int *pArg;
	ST_DXBPROC_EVENT *pEvent;

	ALOGE("%s::%d::[%d]",__func__, __LINE__, (int)iStart);
	pEvent = (ST_DXBPROC_EVENT*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_EVENT));
	pArg = (int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(int));

	pArg[0] = (int)iStart;

	pEvent->pNotifyFunc = (pEventFunc)notify_video_output_start_stop;
	pEvent->arg = pArg;
	pEvent->uiFlgFreeArg = 1;
	if(gpDxBProcEvent->TccMessagePut((void *)pEvent)) {
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent->arg);
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent);
	}
	return 0;
}

unsigned int TCCDxBEvent_AudioStartStop(int iStart)
{
	int *pArg;
	ST_DXBPROC_EVENT *pEvent;

	ALOGE("%s::%d::[%d]",__func__, __LINE__, (int)iStart);
	pEvent = (ST_DXBPROC_EVENT*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ST_DXBPROC_EVENT));
	pArg = (int*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(int));

	pArg[0] = (int)iStart;

	pEvent->pNotifyFunc = (pEventFunc)notify_audio_output_start_stop;
	pEvent->arg = pArg;
	pEvent->uiFlgFreeArg = 1;
	if(gpDxBProcEvent->TccMessagePut((void *)pEvent)) {
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent->arg);
		tcc_mw_free(__FUNCTION__, __LINE__, pEvent);
	}
	return 0;
}

unsigned int TCCDxBEvent_Process(void)
{
	int ret;
	ST_DXBPROC_EVENT *pEvent;

	ret = gpDxBProcEvent->TccMessageGetCount();
	if (ret)
	{
		pEvent =(ST_DXBPROC_EVENT *)gpDxBProcEvent->TccMessageGet();
		if (pEvent)
		{
			pEvent->pNotifyFunc(pEvent->arg);
			if(pEvent->arg && pEvent->uiFlgFreeArg)
			{
				tcc_mw_free(__FUNCTION__, __LINE__, pEvent->arg);
			}

			tcc_mw_free(__FUNCTION__, __LINE__, pEvent);
		}
	}

	return ret;
}

