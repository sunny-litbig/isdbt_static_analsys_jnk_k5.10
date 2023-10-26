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
#if defined(HAVE_LINUX_PLATFORM)
#define LOG_NDEBUG 0
#define LOG_TAG	"ISDBT_MANAGER_DEMUX_SUBTITLE"
#include <utils/Log.h>
#endif

#include <cutils/properties.h>
#include <tcc_dxb_memory.h>
#include <tcc_dxb_queue.h>
#include <tcc_dxb_semaphore.h>
#include <tcc_dxb_thread.h>
#include "tcc_isdbt_manager_demux.h"
#include "tcc_isdbt_manager_demux_subtitle.h"
#include "TsParse_ISDBT_DBLayer.h"
#include "subtitle_main.h"
#include "subtitle_system.h"
#include "subtitle_display.h"
#include "ISDBT_Caption.h"


/****************************************************************************
DEFINITION
****************************************************************************/
#define SUBT_QUEUESIZE 100

/****************************************************************************
DEFINITION OF EXTERNAL FUNCTIONS
****************************************************************************/
extern DxBInterface *hInterface;
extern int TCC_Isdbt_IsSupportJapan(void);
extern DxB_ERR_CODE tcc_demux_alloc_buffer(UINT32 usBufLen, UINT8 **ppucBuf);
extern DxB_ERR_CODE tcc_demux_free_buffer( UINT8 *pucBuf);
extern  int tcc_demux_send_dec_ctrl_cmd(tcc_dxb_queue_t* pDecoderQueue, E_DECTHREAD_CTRLCOMMAND eESCmd, int iSync, void *pArg);
extern ISDBT_CAPTION_INFO gIsdbtCapInfo[2];

/****************************************************************************
DEFINITION OF GLOBAL VARIABLES
****************************************************************************/
static pthread_t SUBTDecoderThreadID;
static tcc_dxb_queue_t* SUBTDecoderQueue;
static int giSUBTThreadRunning = 0;
static int giIsplayingSubtitle = 0;
static int giIsPesFullCaption = 0, giIsPesOneCaption = 0;
static int giIsPesFullSuper = 0, giIsPesOneSuper = 0;

// Noah, IM692A-29
E_DECTHREAD_STATUS guiDecoderStatus = DEC_THREAD_STOP;
/****************************************************************************
DEFINITION OF LOCAL FUNCTIONS
****************************************************************************/
int tcc_manager_demux_subtitle_init(void)
{
	//ALOGE("[%s]\n", __func__);
	int status = 0, lang = 0;

	lang = (TCC_Isdbt_IsSupportJapan())?0x08:0x0; // 0x08: ISDBT for JAPAN, other for SDTV
	Set_Language_Code(lang);

	SUBTDecoderQueue = tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(tcc_dxb_queue_t));

	if(SUBTDecoderQueue == NULL){
		ALOGE("[%s] allocation error", __func__);
		return -1;
	}

	memset(SUBTDecoderQueue, 0, sizeof(tcc_dxb_queue_t));

	giSUBTThreadRunning = 1;

	tcc_dxb_queue_init_ex(SUBTDecoderQueue, SUBT_QUEUESIZE);

	status = tcc_dxb_thread_create((void *)&SUBTDecoderThreadID,
								tcc_demux_subtitle_decoder,
								(unsigned char*)"tcc_demux_subtitle_decoder",
								LOW_PRIORITY_10,
								NULL);
	if(status){
		ALOGE("[%s] subtitle_decoder thread creation fail.", __func__);
		if(SUBTDecoderQueue){
			tcc_mw_free(__FUNCTION__, __LINE__, SUBTDecoderQueue);
		}
		giSUBTThreadRunning = 0;
		return -1;
	}

	TCC_DxB_DEMUX_RegisterPESCallback(hInterface, DxB_DEMUX_PESTYPE_SUBTITLE_0, \
										tcc_demux_subtitle_notify,
										tcc_demux_alloc_buffer,
										tcc_demux_free_buffer);

	TCC_DxB_DEMUX_RegisterPESCallback(hInterface, DxB_DEMUX_PESTYPE_SUBTITLE_1, \
										tcc_demux_subtitle_notify,
										tcc_demux_alloc_buffer,
										tcc_demux_free_buffer);

	TCC_DxB_DEMUX_RegisterPESCallback(hInterface, DxB_DEMUX_PESTYPE_TELETEXT_0, \
										tcc_demux_superimpose_notify,
										tcc_demux_alloc_buffer,
										tcc_demux_free_buffer);

	TCC_DxB_DEMUX_RegisterPESCallback(hInterface, DxB_DEMUX_PESTYPE_TELETEXT_1, \
										tcc_demux_superimpose_notify,
										tcc_demux_alloc_buffer,
										tcc_demux_free_buffer);
	giIsplayingSubtitle = 0;

	return 0;
}

int tcc_manager_demux_subtitle_deinit(void)
{
	//ALOGE("[%s]\n", __func__);
	int err = -1;

	giSUBTThreadRunning = 0;

	while(1)
	{
		if( giSUBTThreadRunning == -1)
			break;
		usleep(5000);
	}

	err = tcc_dxb_thread_join((void *)SUBTDecoderThreadID,NULL);
	tcc_dxb_queue_deinit(SUBTDecoderQueue);
	tcc_mw_free(__FUNCTION__, __LINE__, SUBTDecoderQueue);

//	ALOGD("[%s] subtitle_decoder thread exit", __func__);
	return 0;
}


int tcc_manager_demux_subtitle_stop(int init)
{
    unsigned int waitCountForSubtitleDecoder = 0;

	if(giIsplayingSubtitle == 1){
        // During the app exit or set the other channel, this code will operate.
    	guiDecoderStatus = DEC_THREAD_STOP_REQUEST;
        while (1)
        {
            if (DEC_THREAD_STOP == guiDecoderStatus)
            {
                break;
            }

            waitCountForSubtitleDecoder++;
            if (300 < waitCountForSubtitleDecoder)    // if waiting 3000 ms over, then break
            {
                //waitCountForSubtitleDecoder = 0;
                ALOGD("[%s] It takes about 3000 ms over for waiting subtitle_decoder. Please check this log !!!\n", __func__);
                guiDecoderStatus = DEC_THREAD_STOP;
                break;
            }

            usleep(10 * 1000);
        }

        subtitle_demux_exit(init);

		if(giIsPesFullCaption == 1){
			TCC_DxB_DEMUX_StopPESFilter(hInterface, DEMUX_REQUEST_SUBT); //STOP for fullseg subtitle
			giIsPesFullCaption = 0;
		}
		if(giIsPesOneCaption == 1){
			TCC_DxB_DEMUX_StopPESFilter(hInterface, DEMUX_REQUEST_1SEG_SUBT); //STOP for 1seg subtitle
			giIsPesOneCaption = 0;
		}
		if(giIsPesFullSuper == 1){
			TCC_DxB_DEMUX_StopPESFilter(hInterface, DEMUX_REQUEST_TTX); //STOP for fullseg superimpose
			giIsPesFullSuper = 0;
		}
		if(giIsPesOneSuper == 1){
			TCC_DxB_DEMUX_StopPESFilter(hInterface, DEMUX_REQUEST_1SEG_TTX); //STOP for 1seg superimpose
			giIsPesOneSuper = 0;
		}
	}

	giIsplayingSubtitle = 0;

	return 0;
}

int tcc_manager_demux_subtitle_play(int uiStPID, int uiSiPID, int iSegType, int iCountryCode, int iRaw_w, int iRaw_h, int iView_w, int iView_h, int init)
{
	ALOGE("[%s] init[%d]\n", __func__, init);

	if(giIsplayingSubtitle == 1){
		tcc_manager_demux_subtitle_stop(init);
	}

	if(guiDecoderStatus == DEC_THREAD_STOP){
		// During 1st setchannel, this code will operate.
        if(subtitle_demux_init(iSegType, iCountryCode, iRaw_w, iRaw_h, iView_w, iView_h, init) == 0){
            guiDecoderStatus = DEC_THREAD_RUNNING;
        }
        else{
            ALOGE("[%s] ERR - subtitle_demux_init fail", __func__);
        }
    }else{
        ALOGE("[%s] Invalid state to initialize subtitle decoder", __func__);
    }

	//Fullseg Subtitle
	if ((uiStPID != 0xffff) && (uiStPID != 0) && iSegType==13){
		TCC_DxB_DEMUX_StartPESFilter(hInterface, uiStPID, DxB_DEMUX_PESTYPE_SUBTITLE_0, DEMUX_REQUEST_SUBT, NULL);
		giIsPesFullCaption = 1;
	}

	//1seg Subtitle
	if ((uiStPID != 0xffff) && (uiStPID != 0) && iSegType==1){
		TCC_DxB_DEMUX_StartPESFilter(hInterface, uiStPID, DxB_DEMUX_PESTYPE_SUBTITLE_1, DEMUX_REQUEST_1SEG_SUBT, NULL);
		giIsPesOneCaption = 1;
	}

	//Fullseg Superimpose
	if ((uiSiPID != 0xffff) && (uiSiPID != 0)&& iSegType==13){
		TCC_DxB_DEMUX_StartPESFilter(hInterface, uiSiPID, DxB_DEMUX_PESTYPE_TELETEXT_0, DEMUX_REQUEST_TTX, NULL);
		giIsPesFullSuper = 1;
	}

	//1seg Superimpose
	if ((uiSiPID != 0xffff) && (uiSiPID != 0)&& iSegType==1){
		TCC_DxB_DEMUX_StartPESFilter(hInterface, uiSiPID, DxB_DEMUX_PESTYPE_TELETEXT_1, DEMUX_REQUEST_1SEG_TTX, NULL);
		giIsPesOneSuper = 1;
	}

	giIsplayingSubtitle = 1;

	return 0;
}

DxB_ERR_CODE tcc_demux_subtitle_notify(UINT8 *pucBuf, UINT32 uiBufSize, UINT64 ullPTS, UINT32 ulRequestId, void *pUserData)
{
	DEMUX_DEC_COMMAND_TYPE *pSubtCmd;
	int i, queue_num;

	pSubtCmd = tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(DEMUX_DEC_COMMAND_TYPE));
	if( pSubtCmd == NULL )
	{
		ALOGE("[%s] pSubtCmd is NULL Error !!!!!\n", __func__);
		return DxB_ERR_NO_ALLOC;
	}

	pSubtCmd->eCommandType = DEC_CMDTYPE_DATA;
	pSubtCmd->pData = pucBuf;
	pSubtCmd->uiDataSize = uiBufSize;
	pSubtCmd->uiPTS = ullPTS/90;	//ms unit
	pSubtCmd->iRequestID = ulRequestId;
	pSubtCmd->uiType = 0;

	if( tcc_dxb_queue_ex(SUBTDecoderQueue, pSubtCmd) == 0 )
	{
		queue_num = tcc_dxb_getquenelem(SUBTDecoderQueue);
		if(queue_num > 0){
			//subtitle queue flush
			for(i =0; i<queue_num; i++){
				pSubtCmd = tcc_dxb_dequeue(SUBTDecoderQueue);
			}

			if(pSubtCmd && pSubtCmd->pData){
				ALOGE("QUEUE CAPTION FULL FREE 1");
				tcc_mw_free(__FUNCTION__, __LINE__, pSubtCmd->pData);
			}

			if(pSubtCmd){
				ALOGE("QUEUE CAPTION FULL FREE 2");
				tcc_mw_free(__FUNCTION__, __LINE__, pSubtCmd);
			}
		}

		ALOGE("After caption queue full, num[%d]", tcc_dxb_getquenelem(SUBTDecoderQueue));
		ALOGE("[%s] Can not push subtile queue", __func__);
	}

	return DxB_ERR_OK;
}


DxB_ERR_CODE tcc_demux_superimpose_notify(UINT8 *pucBuf, UINT32 uiBufSize, UINT64 ullPTS, UINT32 ulRequestId, void *pUserData)
{
	DEMUX_DEC_COMMAND_TYPE *pSubtCmd;
	int i, queue_num;

	pSubtCmd = tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(DEMUX_DEC_COMMAND_TYPE));
	if( pSubtCmd == NULL )
	{
		ALOGE("[%s] pSubtCmd is NULL Error !!!!!\n", __func__);
		return DxB_ERR_NO_ALLOC;
	}

	pSubtCmd->eCommandType = DEC_CMDTYPE_DATA;
	pSubtCmd->pData = pucBuf;
	pSubtCmd->uiDataSize = uiBufSize;
	pSubtCmd->uiPTS = ullPTS/90;
	pSubtCmd->iRequestID = ulRequestId;
	pSubtCmd->uiType = 1;

	if( tcc_dxb_queue_ex(SUBTDecoderQueue, pSubtCmd) == 0 )
	{
		queue_num = tcc_dxb_getquenelem(SUBTDecoderQueue);
		if(queue_num > 0){
			//subtitle queue flush
			for(i =0; i<queue_num; i++){
				pSubtCmd = tcc_dxb_dequeue(SUBTDecoderQueue);
			}

			if(pSubtCmd && pSubtCmd->pData){
				ALOGE("QUEUE SUPER FULL FREE 1");
				tcc_mw_free(__FUNCTION__, __LINE__, pSubtCmd->pData);
			}

			if(pSubtCmd){
				ALOGE("QUEUE SUPER FULL FREE 2");
				tcc_mw_free(__FUNCTION__, __LINE__, pSubtCmd);
			}
		}


		ALOGE("After super queue full, num[%d]", tcc_dxb_getquenelem(SUBTDecoderQueue));
		ALOGE("[%s] Can not push subtile queue", __func__);
	}

	return DxB_ERR_OK;
}

void* tcc_demux_subtitle_decoder(void *arg)
{
	DEMUX_DEC_COMMAND_TYPE *pSubtCmd;
	int iQueueNum, i;
	unsigned long long last_update_time[2] = {0, 0};

	subtitle_display_set_cls(-1, 0);

	while(giSUBTThreadRunning == 1)
	{
        // Noah, IM692A-29
	    if (guiDecoderStatus == DEC_THREAD_STOP_REQUEST)
	    {
            guiDecoderStatus = DEC_THREAD_STOP;
	    }

		if(guiDecoderStatus == DEC_THREAD_RUNNING){
			for(i=0 ; i<2 ; i++){
				if(subtitle_system_check_noupdate(last_update_time[i])){
					subtitle_display_set_cls(i, 1);
					last_update_time[i] = 0;
				}
			}
 		}

		iQueueNum = tcc_dxb_getquenelem(SUBTDecoderQueue);
		if(iQueueNum){
			pSubtCmd = tcc_dxb_dequeue(SUBTDecoderQueue);
			if (pSubtCmd){
				switch(pSubtCmd->eCommandType)
				{
					case DEC_CMDTYPE_DATA:
						if(guiDecoderStatus == DEC_THREAD_RUNNING){
							subtitle_display_set_cls(pSubtCmd->uiType, 0);
							subtitle_demux_dec(pSubtCmd->uiType,
												pSubtCmd->pData,
												pSubtCmd->uiDataSize,
												pSubtCmd->uiPTS);
							last_update_time[pSubtCmd->uiType] = subtitle_system_get_systime();
						}

						if(pSubtCmd->pData != NULL){
							tcc_mw_free(__FUNCTION__, __LINE__, pSubtCmd->pData);
						}

						if(pSubtCmd != NULL){
							tcc_mw_free(__FUNCTION__, __LINE__, pSubtCmd);
						}
						break;
				}
			}
		}else{
			usleep(5000);
		}
	}

	while(1)
	{
		iQueueNum = tcc_dxb_getquenelem(SUBTDecoderQueue);
		if(iQueueNum)
		{
			pSubtCmd = tcc_dxb_dequeue(SUBTDecoderQueue);
			if (pSubtCmd)
			{
				switch(pSubtCmd->eCommandType)
				{
					case DEC_CMDTYPE_DATA:
						if(pSubtCmd->pData != NULL){
							tcc_mw_free(__FUNCTION__, __LINE__, pSubtCmd->pData);
						}

						if(pSubtCmd != NULL){
							tcc_mw_free(__FUNCTION__, __LINE__, pSubtCmd);
						}
					break;
				}
			}
		}
		else
		{
			break;
		}
	}

	subtitle_display_set_cls(-1, 0);

	giSUBTThreadRunning = -1;

	return (void*)NULL;
}
