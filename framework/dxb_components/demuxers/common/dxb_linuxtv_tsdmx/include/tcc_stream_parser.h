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
#ifndef _TCC_DXB_STREAM_PARSER_H_
#define _TCC_DXB_STREAM_PARSER_H_

#define VIDEO_SLOT_ES_MIN_SIZE	(600*1024)
#define AUDIO_SLOT_ES_MIN_SIZE	(50*1024)

typedef enum
{
	MPEGDEMUX_ERR_NOERROR = 0,
	MPEGDEMUX_ERR_SLOTSIZEMISMATCH = 1000,
	MPEGDEMUX_ERR_PACKETTYPEMISMATCH,
	MPEGDEMUX_ERR_SLOTCRC,
	MPEGDEMUX_ERR_SLOTSECTIONSIZE,
	MPEGDEMUX_ERR_BADPARAMETER, // 1004
	MPEGDEMUX_ERR_OUTOFMEMORY, // 1005
	MPEGDEMUX_ERR_INVALIDHANDLE,
	MPEGDEMUX_ERR_NOTSUPORTSTEAMPARSER,	
	MPEGDEMUX_ERR_SLOTINVALIDSTATE,
	MPEGDEMUX_ERR_CONTENTTYPE,
	MPEGDEMUX_ERR_DUPLICATEDPID,
	MPEGDEMUX_ERR_DUPLICATEDSECTIONFILTER,
	MPEGDEMUX_ERR_PIDFILTERFULL,
	MPEGDEMUX_ERR_OUTOFPAYLOAD,
	MPEGDEMUX_ERR_DISCONTINUITYCOUNTER,
	MPEGDEMUX_ERR_PESPACKET,
	MPEGDEMUX_ERR_NOTFOUNDPID,
	MPEGDEMUX_ERR_SLOTNOTREADY,
	MPEGDEMUX_ERR_NOTSUPPORTSCRAMBLED,
	MPEGDEMUX_ERR_INVALIDPIDFILTER,
	MPEGDEMUX_ERR_ALREADYINITIALIZED,
	MPEGDEMUX_ERR_NOTINITIALIZEDYET,
	MPEGDEMUX_ERR_RESET,
	MPEGDEMUX_ERR_ERROR
} MPEGDEMUX_ERROR;

typedef struct
{
	INT64 m_lPTS;
	INT64 m_PrevlPTS;
	UINT32 m_BufferSize; //buffer size for data
	UINT32 m_BufferSendSize; //send size for callback
	UINT8 *m_pBuffer; //buffer for data
}ST_SEND_DATA;

typedef INT32 (*GET_BUFFER_FUCTION) (void *pDemuxFilter, UINT8 **ppucBuffer, UINT32 *puiBufferSize);
typedef INT32 (*STREAM_CALLBACK_FUCTION) (void *pDemuxFilter, UINT8 *buf, INT32 size, INT64 pts, INT32 flags);

typedef struct
{
	/* [COMMON] */
	UINT32 m_Enable;	
	UINT32 m_uiPacketFirstSend;	
	UINT32 m_uiStreamType;
	UINT32 m_uiQueue;
	UINT64 m_ulliQueue;
	UINT32 m_uiDeliveryDataLength;
	ST_SEND_DATA m_SendData;
	UINT32 m_uiEsMaxSize;
	STREAM_CALLBACK_FUCTION pushCallback;
	GET_BUFFER_FUCTION getBuffer;
	void *appData;
	INT32 m_uiPlayMode;

	/* [VIDEO] */
	UINT32 m_uiSequence;	//MPEG2 : SEQ HEADER, H264:SPS
	UINT32 m_uihasPicture;	//Only use in MPEG2

	/* [AUDIO] */
	UINT32 m_uiFindStartHeader;
	UINT32 m_uiSyncFrameOutputCounter;
	UINT32 m_uiSyncFrameSize;
	UINT32 m_uiSyncFrameTime;	// mS
	
}ST_STREAM_PARSER;


INT32 TSDEMUXSTParser_GetStreamType(ST_STREAM_PARSER *pstParser);
INT32 TSDEMUXSTParser_EnableStreamParser(ST_STREAM_PARSER **pstParser);
INT32 TSDEMUXSTParser_DisableStreamParser(ST_STREAM_PARSER *pstParser);
INT32 TSDEMUXSTParser_IsAvailable(ST_STREAM_PARSER *pstParser);
INT32 TSDEMUXSTParser_InitBuffer(ST_STREAM_PARSER *pstParser);
INT32 TSDEMUXSTParser_ResetBuffer(ST_STREAM_PARSER *pstParser);
INT32 TSDEMUXSTParser_WriteBuffer(ST_STREAM_PARSER *pStreamParser, UINT8 *p, INT32 iCount);
INT32 TSDEMUXSTParser_SendBuffer(ST_STREAM_PARSER *pstParser);
INT32 TSDEMUXSTParser_WriteBuffer2(ST_STREAM_PARSER *pstParser, UINT8 *p, INT32 iCount);
INT32 TSDEMUXSTParser_SendBuffer2(ST_STREAM_PARSER *pstParser);
INT32 TSDEMUXSTParser_H264FindStartCode(ST_STREAM_PARSER *pstParser, UINT8 *p, INT32 l, INT32 *pCount);
INT32 TSDEMUXSTParser_ProcessH264Data(ST_STREAM_PARSER *pstParser, UINT8 *pData, UINT32 uiDataSize, INT64 lPTS);
INT32 TSDEMUXSTParser_H264FindStartCode_Reverse(ST_STREAM_PARSER *pstParser, UINT8 *p, INT32 l, INT32 *pCount);
INT32 TSDEMUXSTParser_ProcessH264Data_Reverse(ST_STREAM_PARSER *pstParser, UINT8 *pData, UINT32 uiDataSize, INT64 lPTS);
INT32 TSDEMUXSTParser_MPEG2FindStartCode(ST_STREAM_PARSER *pstParser, UINT8 *p, INT32 l, INT32 *pCount);
INT32 TSDEMUXSTParser_ProcessMPEG2Data(ST_STREAM_PARSER *pstParser, UINT8 *pData, UINT32 uiDataSize, INT64 lPTS);
INT32 TSDEMUXSTParser_MPEG2FindStartCode_Reverse(ST_STREAM_PARSER *pstParser, UINT8 *p, INT32 l, INT32 *pCount);
INT32 TSDEMUXSTParser_ProcessMPEG2Data_Reverse(ST_STREAM_PARSER *pstParser, UINT8 *pData, UINT32 uiDataSize, INT64 lPTS);
INT32 TSDEMUXSTParser_AC3FindStartCode(ST_STREAM_PARSER *pstParser, UINT8 *p, INT32 l, INT32 *pCount);
INT32 TSDEMUXSTParser_ProcessAC3Data(ST_STREAM_PARSER *pstParser, UINT8 *pData, UINT32 uiDataSize, INT64 lPTS, INT32 pusi);
INT32 TSDEMUXSTParser_ProcessData(ST_STREAM_PARSER *pstParser, UINT8 *pData, UINT32 uiDataSize, INT64 lPTS, INT32 pusi);
INT32 TSDEMUXSTParser_IsKeyFrame(ST_STREAM_PARSER *pstParser, UINT8 *pbyBuffer, UINT32 lBuffLen);

#endif
