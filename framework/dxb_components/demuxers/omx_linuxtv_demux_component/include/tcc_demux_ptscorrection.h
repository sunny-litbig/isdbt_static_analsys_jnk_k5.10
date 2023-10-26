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
#ifndef _TCC_DEMUX_EVENT_H_
#define	_TCC_DEMUX_EVENT_H_

#include "OMX_Core.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
* defines 
******************************************************************************/
#define 	SIZEOF_VIDEO_ES_BUFFER	(1024*1024*2)


/******************************************************************************
* typedefs & structure
******************************************************************************/

enum {
	ES_VIDEO_MAIN = 0,
	ES_VIDEO_SUB,
	ES_VIDEO_MAX
};

typedef enum
{
	PTSCORRECTION_STATUS_OK		= 0,
	PTSCORRECTION_STATUS_CORRECTION_READY,
	PTSCORRECTION_STATUS_PUT_FAIL,
	PTSCORRECTION_STATUS_NOTREADYYET,
	PTSCORRECTION_STATUS_DATAGETFAIL,
	PTSCORRECTION_STATUS_PTSADJUSTFAIL,
	PTSCORRECTION_STATUS_ESBUFUPDATEFAIL,
	PTSCORRECTION_STATUS_MAX,
}DEMUX_PTSCORRECTION_STATUS_TYPE;

typedef struct
{
	unsigned int	iWritePtr;
	unsigned int	iReadPtr;
	unsigned int	iBasePtr;
	unsigned int	iBufSize;
}TCC_DEMUX_ESBUFFER;

typedef struct
{
	long		lPTSInterval;
	long long	llOrgPTSInterval;
	int			iCount;
	int	 		iTotalCount;

}TCC_PTS_ADJUSTVIDEOPTSINFO;

typedef struct
{
	int 		 	iPTS;
	unsigned int 	uiFrameSize;
	unsigned char *pFrameData;
	long long 	llOrgPTS;
	int 			iRequestID;
}TCC_DEMUX_ESDATA;

typedef void* HandleDemuxMsg;


/******************************************************************************
* globals
******************************************************************************/

/******************************************************************************
* locals
******************************************************************************/

/******************************************************************************
* declarations
******************************************************************************/

HandleDemuxMsg TCCDEMUXMsg_Init(void);
int TCCDEMUXMsg_DeInit(HandleDemuxMsg pHandle);
int TCCDEMUXMsg_ReInit(HandleDemuxMsg pHandle);
int TCCDEMUXMsg_GetCount(HandleDemuxMsg pHandle, int iRequestID);
void TCCDEMUXMsg_UpdatelastPts(HandleDemuxMsg pHandle, int iRequestID, unsigned long long pts);
int TCCDEMUXMsg_CorrectionReadyCheck(HandleDemuxMsg pHandle, int iRequestID, unsigned long long pts);
unsigned int TCCDEMUXMsg_PutEs(HandleDemuxMsg pHandle, int iRequestID, unsigned char *buf, int size, unsigned long long pts);
unsigned int TCCDEMUXMsg_GetEs(HandleDemuxMsg pHandle, int iRequestID, OMX_BUFFERHEADERTYPE* pOutputBuffer, unsigned long long pts);
int TCCDEMUX_UpdateESBuffer(HandleDemuxMsg pHandle, int iRequestID, unsigned char *buf, int size);
int TCCDEMUX_GetAdjustVideoPTS(HandleDemuxMsg pHandle, int iRequestID, TCC_DEMUX_ESDATA *pESData, int *plAdjustVideoPTS, long long *pllAdjustVideoOrgPTS);

#ifdef __cplusplus
}
#endif

#endif
