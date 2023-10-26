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
#define LOGE_DEBUG	0
#define LOG_TAG	"OMX_DXB_PTSCORRECTION"
#include <utils/Log.h>
#include <string.h>
#include "TCCMemory.h"
#include "tcc_msg.h"
#include "tcc_demux_ptscorrection.h"


/****************************************************************************
DEFINE
****************************************************************************/
#if LOGE_DEBUG
#define   DEBUG_MSG(msg...)	ALOGD(msg)
#else
#define   DEBUG_MSG(msg...)  
#endif


/****************************************************************************
STATIC VARIABLES
****************************************************************************/
typedef struct
{
	TccMessage *gpDEMUXMsg[ES_VIDEO_MAX];
	unsigned long long lLastInputPts[ES_VIDEO_MAX];
	TCC_PTS_ADJUSTVIDEOPTSINFO g_stAdjustVideoPTSInfo[ES_VIDEO_MAX];
	int		glVideoLastPTS[ES_VIDEO_MAX];
	unsigned char guVideoESBuffer[SIZEOF_VIDEO_ES_BUFFER];
	TCC_DEMUX_ESBUFFER	g_stESBufferInfo[ES_VIDEO_MAX];
} DemuxMessage;


/****************************************************************************
EXTERNAL FUNCTIONS
****************************************************************************/

HandleDemuxMsg TCCDEMUXMsg_Init(void)
{
	int i=0;
	DEBUG_MSG("%s::%d",__func__, __LINE__);

	DemuxMessage *pDemuxMessage = (DemuxMessage*) TCC_fo_malloc(__func__, __LINE__, sizeof(DemuxMessage));
	if(pDemuxMessage == NULL){
		return (HandleDemuxMsg) pDemuxMessage;
	}

	memset(pDemuxMessage->g_stAdjustVideoPTSInfo, 0x0, sizeof(TCC_PTS_ADJUSTVIDEOPTSINFO)*ES_VIDEO_MAX);
	memset(pDemuxMessage->g_stESBufferInfo, 0x0, sizeof(TCC_DEMUX_ESBUFFER)*ES_VIDEO_MAX);
	memset(pDemuxMessage->guVideoESBuffer, 0x0, SIZEOF_VIDEO_ES_BUFFER);

	for(i=0; i<ES_VIDEO_MAX; i++)
	{
		pDemuxMessage->gpDEMUXMsg[i] = new TccMessage(100);
		pDemuxMessage->lLastInputPts[i] =0;
		pDemuxMessage->glVideoLastPTS[i] = 0;
		pDemuxMessage->g_stESBufferInfo[i].iBufSize = SIZEOF_VIDEO_ES_BUFFER;
	}

	return (HandleDemuxMsg) pDemuxMessage;
}

int TCCDEMUXMsg_DeInit(HandleDemuxMsg pHandle)
{
	DemuxMessage *pDemuxMessage = (DemuxMessage*) pHandle;
	unsigned int i, j, uielement;
	TCC_DEMUX_ESDATA *p;

	for(j=0; j<ES_VIDEO_MAX; j++)
	{
		uielement = pDemuxMessage->gpDEMUXMsg[j]->TccMessageGetCount();
		for(i=0;i<uielement;i++)
		{
			p =(TCC_DEMUX_ESDATA *)pDemuxMessage->gpDEMUXMsg[j]->TccMessageGet();
			if(p!= NULL)
		        	delete p;			
		}
		delete pDemuxMessage->gpDEMUXMsg[j];

		pDemuxMessage->lLastInputPts[j] =0;
		pDemuxMessage->glVideoLastPTS[j] = 0;
	}
//	delete pDemuxMessage;
	TCC_fo_free(__func__, __LINE__, pDemuxMessage);
	DEBUG_MSG("%s::%d",__func__, __LINE__);

	return PTSCORRECTION_STATUS_OK;
}

int TCCDEMUXMsg_ReInit(HandleDemuxMsg pHandle)
{
	DemuxMessage *pDemuxMessage = (DemuxMessage*) pHandle;
	unsigned int i, j, uielement;
	TCC_DEMUX_ESDATA *p;

	memset(pDemuxMessage->g_stAdjustVideoPTSInfo, 0x0, sizeof(TCC_PTS_ADJUSTVIDEOPTSINFO)*2);
	memset(pDemuxMessage->g_stESBufferInfo, 0x0, sizeof(TCC_DEMUX_ESBUFFER)*2);
	memset(pDemuxMessage->guVideoESBuffer, 0x0, SIZEOF_VIDEO_ES_BUFFER);

	for(j=0; j<ES_VIDEO_MAX; j++)
	{
		uielement = pDemuxMessage->gpDEMUXMsg[j]->TccMessageGetCount();
		for(i=0;i<uielement;i++)
		{
			p =(TCC_DEMUX_ESDATA *)pDemuxMessage->gpDEMUXMsg[j]->TccMessageGet();
			if(p!= NULL)
		        	delete p;			
		}

		pDemuxMessage->lLastInputPts[j] =0;
		pDemuxMessage->glVideoLastPTS[j] = 0;
		pDemuxMessage->g_stESBufferInfo[j].iBufSize = SIZEOF_VIDEO_ES_BUFFER;
	}

	DEBUG_MSG("%s::%d",__func__, __LINE__);

	return PTSCORRECTION_STATUS_OK;
}


int TCCDEMUXMsg_GetCount(HandleDemuxMsg pHandle, int iRequestID)
{
	DemuxMessage *pDemuxMessage = (DemuxMessage*) pHandle;
	return pDemuxMessage->gpDEMUXMsg[iRequestID]->TccMessageGetCount();
}

void TCCDEMUXMsg_UpdatelastPts(HandleDemuxMsg pHandle, int iRequestID, unsigned long long pts)
{
	DemuxMessage *pDemuxMessage = (DemuxMessage*) pHandle;
	pDemuxMessage->lLastInputPts[iRequestID] = pts;
}

int TCCDEMUXMsg_CorrectionReadyCheck(HandleDemuxMsg pHandle, int iRequestID, unsigned long long pts)
{
	DemuxMessage *pDemuxMessage = (DemuxMessage*) pHandle;
	if(pDemuxMessage->lLastInputPts[iRequestID] != 0 && pDemuxMessage->lLastInputPts[iRequestID] != pts)
		return PTSCORRECTION_STATUS_CORRECTION_READY;
	else	
	{
		TCCDEMUXMsg_UpdatelastPts(pHandle, iRequestID, pts);
		return PTSCORRECTION_STATUS_OK;
	}
}

unsigned int TCCDEMUXMsg_PutEs(HandleDemuxMsg pHandle, int iRequestID, unsigned char *buf, int size, unsigned long long pts)
{
	DemuxMessage *pDemuxMessage = (DemuxMessage*) pHandle;
	TCC_DEMUX_ESDATA *pESData = new TCC_DEMUX_ESDATA;
	unsigned long long org_pts;
	unsigned int	err=0;

	org_pts = pts;
	pts = pts / 90;

	//ALOGE("%s::%d  size = %d \n",__func__, __LINE__, size);
	if(size == 0)
	{
		delete pESData;
		return PTSCORRECTION_STATUS_PUT_FAIL;
	}

	err = TCCDEMUX_UpdateESBuffer(pHandle, iRequestID, buf, size);

	if(err != PTSCORRECTION_STATUS_OK)
	{
		delete pESData;
		return PTSCORRECTION_STATUS_PUT_FAIL;
	}
		
	pESData->pFrameData 	= &pDemuxMessage->guVideoESBuffer[(pDemuxMessage->g_stESBufferInfo[iRequestID].iWritePtr- size)];
	pESData->iPTS 		= (int)pts;
	pESData->uiFrameSize 	= size;
	pESData->llOrgPTS 	= org_pts;
	pESData->iRequestID 	= iRequestID;

	err = pDemuxMessage->gpDEMUXMsg[iRequestID]->TccMessagePut((void *)pESData);
	if(err)
	{
		delete pESData;
		return PTSCORRECTION_STATUS_PUT_FAIL;
	}

	return PTSCORRECTION_STATUS_OK;
}

unsigned int TCCDEMUXMsg_GetEs(HandleDemuxMsg pHandle, int iRequestID, OMX_BUFFERHEADERTYPE* pOutputBuffer, unsigned long long pts)
{
	DemuxMessage *pDemuxMessage = (DemuxMessage*) pHandle;
	int iTCCMsgCount, size =0;
	TCC_DEMUX_ESDATA *pESData;
	int lAdjustVideoPTS;
	long long llAdjustVideoOrgPTS, lOrgPts;
	int ilastPTS, err = 0;

	iTCCMsgCount = pDemuxMessage->gpDEMUXMsg[iRequestID]->TccMessageGetCount();

	if(iTCCMsgCount>1)
	{
		pESData = (TCC_DEMUX_ESDATA *)pDemuxMessage->gpDEMUXMsg[iRequestID]->TccMessageGet();

		if(pESData == NULL)
			return PTSCORRECTION_STATUS_DATAGETFAIL;
		else
		{
			size = pESData->uiFrameSize;
			ilastPTS = pESData->iPTS;
			lOrgPts = pESData->llOrgPTS;

			err = TCCDEMUX_GetAdjustVideoPTS(pHandle, iRequestID, pESData, &lAdjustVideoPTS, &llAdjustVideoOrgPTS);
	//		ALOGE("%s %d  err = %d \n", __func__, __LINE__, err);
			if(err == PTSCORRECTION_STATUS_OK)
			{
				unsigned int	iBasePtr=0;
				iBasePtr = (iRequestID *  SIZEOF_VIDEO_ES_BUFFER/2);

	//			ALOGE("iReadPtr = %d, size = %d \n", g_stESBufferInfo[iRequestID].iReadPtr, size);

				if((pDemuxMessage->g_stESBufferInfo[iRequestID].iReadPtr + size) < (pDemuxMessage->g_stESBufferInfo[iRequestID].iBufSize/2 + iBasePtr))
				{
					memcpy(pOutputBuffer->pBuffer, &pDemuxMessage->guVideoESBuffer[pDemuxMessage->g_stESBufferInfo[iRequestID].iReadPtr], size);
					pDemuxMessage->g_stESBufferInfo[iRequestID].iReadPtr += size;	
				}
				else
				{
					memcpy(pOutputBuffer->pBuffer, &pDemuxMessage->guVideoESBuffer[iBasePtr], size);
					pDemuxMessage->g_stESBufferInfo[iRequestID].iReadPtr = iBasePtr +size;	
				}

				pOutputBuffer->pBuffer[size] = iRequestID;
				pOutputBuffer->nTimeStamp = (long long)lAdjustVideoPTS * 1000;
				pOutputBuffer->nFilledLen = size;
				pDemuxMessage->glVideoLastPTS[iRequestID] = ilastPTS;

				TCCDEMUXMsg_UpdatelastPts(pHandle, iRequestID, lOrgPts);
				delete pESData;
				return PTSCORRECTION_STATUS_OK;
			}
			else
			{
				unsigned int	iBasePtr=0;
				iBasePtr = (iRequestID *  SIZEOF_VIDEO_ES_BUFFER/2);

	//			ALOGE("[%s][%d]iReadPtr = %d, size = %d \n", __func__, __LINE__, g_stESBufferInfo[iRequestID].iReadPtr, size);
	//			ALOGE("[%s][%d] pts= %lld,  lLastInputPts[%d] = %lld \n", __func__, __LINE__, pts/90, iRequestID, lLastInputPts[iRequestID]/90);

				if((pDemuxMessage->g_stESBufferInfo[iRequestID].iReadPtr + size) < (pDemuxMessage->g_stESBufferInfo[iRequestID].iBufSize/2 + iBasePtr))
				{
					pDemuxMessage->g_stESBufferInfo[iRequestID].iReadPtr += size;
				}
				else
				{
					pDemuxMessage->g_stESBufferInfo[iRequestID].iReadPtr = iBasePtr +size;
				}
				delete pESData;
				return PTSCORRECTION_STATUS_DATAGETFAIL;
			}
		}
	}
	return PTSCORRECTION_STATUS_DATAGETFAIL;
}

int TCCDEMUX_UpdateESBuffer(HandleDemuxMsg pHandle, int iRequestID, unsigned char *buf, int size)
{
	DemuxMessage *pDemuxMessage = (DemuxMessage*) pHandle;
	unsigned int	iBasePtr=0;
	iBasePtr = (iRequestID *  SIZEOF_VIDEO_ES_BUFFER/2);

	if(pDemuxMessage->g_stESBufferInfo[iRequestID].iReadPtr == 0)
		pDemuxMessage->g_stESBufferInfo[iRequestID].iReadPtr = iBasePtr;
	if(pDemuxMessage->g_stESBufferInfo[iRequestID].iWritePtr == 0)
		pDemuxMessage->g_stESBufferInfo[iRequestID].iWritePtr = iBasePtr;

//	ALOGE("iWritePtr = %d , size = %d , iBasePtr = %d,  iBufSize = %d \n", pDemuxMessage->g_stESBufferInfo[iRequestID].iWritePtr, size, iBasePtr, pDemuxMessage->g_stESBufferInfo[iRequestID].iBufSize);

	if((pDemuxMessage->g_stESBufferInfo[iRequestID].iWritePtr + size) < (pDemuxMessage->g_stESBufferInfo[iRequestID].iBufSize/2 + iBasePtr))
	{
		memcpy(&pDemuxMessage->guVideoESBuffer[pDemuxMessage->g_stESBufferInfo[iRequestID].iWritePtr], buf, size);
		pDemuxMessage->g_stESBufferInfo[iRequestID].iWritePtr += size;
	}
	else if((iBasePtr + size) < pDemuxMessage->g_stESBufferInfo[iRequestID].iReadPtr)
	{
		memcpy(&pDemuxMessage->guVideoESBuffer[iBasePtr], buf, size);
		pDemuxMessage->g_stESBufferInfo[iRequestID].iWritePtr = (iBasePtr + size);
	}
	else
	{
		DEBUG_MSG("%s Buffer Update Error iWritePtr = %d , iReadPtr = %d \n", __func__, pDemuxMessage->g_stESBufferInfo[iRequestID].iWritePtr, pDemuxMessage->g_stESBufferInfo[iRequestID].iReadPtr);
		return PTSCORRECTION_STATUS_ESBUFUPDATEFAIL;
	}
	
	return PTSCORRECTION_STATUS_OK;
}

int TCCDEMUX_GetAdjustVideoPTS(HandleDemuxMsg pHandle, int iRequestID, TCC_DEMUX_ESDATA *pESData, int *plAdjustVideoPTS, long long *pllAdjustVideoOrgPTS)
{
	DemuxMessage *pDemuxMessage = (DemuxMessage*) pHandle;
	TCC_DEMUX_ESDATA *pNextESData;
	int iQueueNum, iFrameNum, i;
	long lVideoPTSInterval;
	long long llVideoOrgPTSInterval;
	void *hTCCESMsg;

	*plAdjustVideoPTS = pESData->iPTS;
	*pllAdjustVideoOrgPTS = pESData->llOrgPTS;

//	ALOGE("iPTS = %d,  glVideoLastPTS = %d \n", pESData->iPTS, glVideoLastPTS[iRequestID]);

	if( pESData->iPTS != pDemuxMessage->glVideoLastPTS[iRequestID] ) {
		pDemuxMessage->g_stAdjustVideoPTSInfo[iRequestID].lPTSInterval = 0;
		pDemuxMessage->g_stAdjustVideoPTSInfo[iRequestID].llOrgPTSInterval = 0;
		pDemuxMessage->g_stAdjustVideoPTSInfo[iRequestID].iCount = 0;
		pDemuxMessage->g_stAdjustVideoPTSInfo[iRequestID].iTotalCount = 0;

		DEBUG_MSG("%s LastPTS(%d) and CurPTS(%d) is different\n", __func__, pDemuxMessage->glVideoLastPTS[iRequestID], pESData->iPTS);
		return PTSCORRECTION_STATUS_OK;
	}

	if( (pDemuxMessage->g_stAdjustVideoPTSInfo[iRequestID].iTotalCount > 0) && (pDemuxMessage->g_stAdjustVideoPTSInfo[iRequestID].iCount <= pDemuxMessage->g_stAdjustVideoPTSInfo[iRequestID].iTotalCount) ) {
		if( pDemuxMessage->g_stAdjustVideoPTSInfo[iRequestID].lPTSInterval ) {
			*plAdjustVideoPTS = pESData->iPTS + (pDemuxMessage->g_stAdjustVideoPTSInfo[iRequestID].lPTSInterval * pDemuxMessage->g_stAdjustVideoPTSInfo[iRequestID].iCount);
		}

		if( pDemuxMessage->g_stAdjustVideoPTSInfo[iRequestID].llOrgPTSInterval ) {
			*pllAdjustVideoOrgPTS = pESData->llOrgPTS + (pDemuxMessage->g_stAdjustVideoPTSInfo[iRequestID].llOrgPTSInterval * pDemuxMessage->g_stAdjustVideoPTSInfo[iRequestID].iCount );
		}
		pDemuxMessage->g_stAdjustVideoPTSInfo[iRequestID].iCount++;

		DEBUG_MSG("%s CurPTS is modified(%d->%d, %lld->%lld)\n", __func__, pESData->iPTS, *plAdjustVideoPTS, pESData->llOrgPTS, *pllAdjustVideoOrgPTS);
		return PTSCORRECTION_STATUS_OK;
	} else {
		hTCCESMsg = pDemuxMessage->gpDEMUXMsg[iRequestID]->TccMessageHandleFirst();	
		iQueueNum = TCCDEMUXMsg_GetCount(pDemuxMessage, iRequestID);

		if( iQueueNum > 1 ) {
			for(i=1, iFrameNum=1; i<=iQueueNum; i++)
			{
				pNextESData = (TCC_DEMUX_ESDATA *)pDemuxMessage->gpDEMUXMsg[iRequestID]->TccMessageHandleMsg(hTCCESMsg);

				if( pNextESData ) {
					if( pNextESData->iRequestID == pESData->iRequestID ) {
						if( pESData->iPTS < pNextESData->iPTS ) {
							lVideoPTSInterval = (pNextESData->iPTS - pESData->iPTS) / (iFrameNum + 1);
							llVideoOrgPTSInterval = (pNextESData->llOrgPTS - pESData->llOrgPTS) / (iFrameNum + 1);
            				if( lVideoPTSInterval > 32 && lVideoPTSInterval < 251 )	{
								pDemuxMessage->g_stAdjustVideoPTSInfo[iRequestID].lPTSInterval = lVideoPTSInterval;
								pDemuxMessage->g_stAdjustVideoPTSInfo[iRequestID].llOrgPTSInterval = llVideoOrgPTSInterval;
								pDemuxMessage->g_stAdjustVideoPTSInfo[iRequestID].iCount = 1;
								pDemuxMessage->g_stAdjustVideoPTSInfo[iRequestID].iTotalCount = iFrameNum;

								*plAdjustVideoPTS = pESData->iPTS + (pDemuxMessage->g_stAdjustVideoPTSInfo[iRequestID].lPTSInterval * pDemuxMessage->g_stAdjustVideoPTSInfo[iRequestID].iCount);
								*pllAdjustVideoOrgPTS = pESData->llOrgPTS + (pDemuxMessage->g_stAdjustVideoPTSInfo[iRequestID].llOrgPTSInterval * pDemuxMessage->g_stAdjustVideoPTSInfo[iRequestID].iCount);
								pDemuxMessage->g_stAdjustVideoPTSInfo[iRequestID].iCount++;
							}
							return PTSCORRECTION_STATUS_OK;	
						} else if( pESData->iPTS > pNextESData->iPTS ) {
							pDemuxMessage->g_stAdjustVideoPTSInfo[iRequestID].lPTSInterval = 0;
							pDemuxMessage->g_stAdjustVideoPTSInfo[iRequestID].llOrgPTSInterval = 0;
							pDemuxMessage->g_stAdjustVideoPTSInfo[iRequestID].iCount = 0;
							pDemuxMessage->g_stAdjustVideoPTSInfo[iRequestID].iTotalCount = 0;

							DEBUG_MSG("%s CurPTS(%d) is faster than NextPTS(%d)\n",__func__, pESData->iPTS, pNextESData->iPTS);
							return PTSCORRECTION_STATUS_OK;
						}

						iFrameNum++;
					}
				}
				hTCCESMsg = pDemuxMessage->gpDEMUXMsg[iRequestID]->TccMessageHandleNext(hTCCESMsg);
			}
		}
	}

	return PTSCORRECTION_STATUS_PTSADJUSTFAIL;
}

