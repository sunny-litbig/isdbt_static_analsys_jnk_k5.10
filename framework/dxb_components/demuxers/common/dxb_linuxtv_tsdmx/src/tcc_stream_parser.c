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
#include <utils/Log.h>
#include <tcc_dxb_interface_demux.h>
#include <tcc_stream_parser.h>

//For Stream Parser
#define	PICTURE_START_CODE		0x00000100
#define	SLICE_START_CODE_MIN	0x00000101
#define	SLICE_START_CODE_MAX	0x000001AF
#define	USER_DATA_START_CODE	0x000001B2
#define	SEQUENCE_HEADER_CODE	0x000001B3
#define	SEQUENCE_ERROR_CODE		0x000001B4
#define	EXTENSION_START_CODE	0x000001B5
#define	SEQUENCE_END_CODE		0x000001B7
#define	GROUP_START_CODE		0x000001B8
#define	SYSTEM_START_CODE_MIN	0x000001B9
#define	SYSTEM_START_CODE_MAX	0x000001FF

#define DEBUG_NO_OUTPUT     0x00
#define DEBUG_ERROR         0x01
#define DEBUG_WARNING       0x02
#define DEBUG_SEQ		    0x04
#define DEBUG_ALL           0xff
#define DEBUG_LEVEL         DEBUG_ALL
#define DTV_DEBUG(n, args...) ALOGI(args)

#define AUDIO_AC3_HEADER_SIGNATURE		0x0B77000000000000
#define AUDIO_AC3_HEADER_SIGNATURE_MASK	0xFFFF000000000000	/* 0B 77 xx xx fscod bsid xx xx */

const UINT32 AC3_FrameSizeCodeTable[38][4] =
{
/*	bitrate		fs=48khz	fs=44.1khz	fs=32khz	*/
/*	(bps)		(words)		(words)		(words)		*/
	32000,		64,			69,			96,			
	32000,		64,			70,			96,			
	40000,		80,			87,			120,		
	40000,		80,			88,			120,		
	48000,		96,			104,		144,		
	48000,		96,			105,		144,		
	56000,		112,		121,		168,		
	56000,		112,		122,		168,		
	64000,		128,		139,		192,		
	64000,		128,		140,		192,		
	80000,		160,		174,		240,		
	80000,		160,		175,		240,		
	96000,		192,		208,		288,		
	96000,		192,		209,		288,		
	112000,		224,		243,		336,		
	112000,		224,		244,		336,		
	128000,		256,		278,		384,		
	128000,		256,		279,		384,		
	160000,		320,		348,		480,		
	160000,		320,		349,		480,		
	192000,		384,		417,		576,		
	192000,		384,		418,		576,		
	224000,		448,		487,		672,		
	224000,		448,		488,		672,		
	256000,		512,		557,		768,		
	256000,		512,		558,		768,		
	320000,		640,		696,		960,		
	320000,		640,		697,		960,		
	384000,		768,		835,		1152,		
	384000,		768,		836,		1152,		
	448000,		896,		975,		1344,		
	448000,		896,		976,		1344,		
	512000,		1024,		1114,		1536,		
	512000,		1024,		1115,		1536,		
	576000,		1152,		1253,		1728,		
	576000,		1152,		1254,		1728,		
	640000,		1280,		1393,		1920,		
	640000,		1280,		1394,		1920
};

INT32 TSDEMUXSTParser_GetStreamType(ST_STREAM_PARSER *pstParser)
{
	DTV_DEBUG (DEBUG_SEQ,"%s\n", __func__);
	
	if( pstParser )	
		return pstParser->m_uiStreamType;			

	return STREAMTYPE_NONE;
}

INT32 TSDEMUXSTParser_EnableStreamParser(ST_STREAM_PARSER **pstParser)
{
	DTV_DEBUG (DEBUG_SEQ,"%s\n", __func__);

	if( *pstParser == NULL )	
	{
		if ((*pstParser = (ST_STREAM_PARSER *) TCC_fo_calloc (__func__, __LINE__, 1, sizeof(ST_STREAM_PARSER))) == NULL)
		{
			return MPEGDEMUX_ERR_OUTOFMEMORY;
		}	
	}
	(*pstParser)->m_uiPlayMode = 0;
	(*pstParser)->m_Enable = 1;	
	return 0;
}

INT32 TSDEMUXSTParser_DisableStreamParser(ST_STREAM_PARSER *pstParser)
{	
	DTV_DEBUG (DEBUG_SEQ,"%s\n", __func__);

	if( pstParser )	
	{
		TSDEMUXSTParser_ResetBuffer(pstParser);
		pstParser->m_Enable = 0;
	}
	return 0;
}

INT32 TSDEMUXSTParser_IsAvailable(ST_STREAM_PARSER *pstParser)
{
	if( pstParser )
	{
		if( pstParser->m_Enable == 0 )
			return 0;
	}
	else
		return 0;

	return 1;
}

INT32 TSDEMUXSTParser_InitBuffer(ST_STREAM_PARSER *pstParser)
{
    if( pstParser )
    {
        pstParser->m_uiSequence = 0;
        pstParser->m_SendData.m_pBuffer = NULL;
        pstParser->m_SendData.m_BufferSize = 0;
        pstParser->m_uiDeliveryDataLength = 0;
        pstParser->m_uihasPicture=FALSE;
        pstParser->m_uiQueue = 0xffffffff;
        pstParser->m_uiPacketFirstSend = 0;
    }
    return 0;
}

INT32 TSDEMUXSTParser_ResetBuffer(ST_STREAM_PARSER *pstParser)
{
	if( pstParser )
	{
	    DTV_DEBUG (DEBUG_SEQ,"%s : seq(%d) type(%d)\n", __func__, pstParser->m_uiSequence, pstParser->m_uiStreamType);	
#if 1	    
		if( pstParser->m_uiStreamType == STREAMTYPE_VIDEO_AVCH264 )
		{
			pstParser->m_uiSequence &=~1;	
			pstParser->m_SendData.m_pBuffer = NULL;
			pstParser->m_SendData.m_BufferSize = 0;
			pstParser->m_uiDeliveryDataLength = 0;	
			pstParser->m_uiQueue = 0xffffffff;
		}
		else if (pstParser->m_uiStreamType == STREAMTYPE_AUDIO_AC3_2)
		{
			pstParser->m_SendData.m_pBuffer = NULL;
			pstParser->m_SendData.m_BufferSize = 0;
			pstParser->m_uiDeliveryDataLength = 0;
			pstParser->m_ulliQueue = (UINT64)0xFFFFFFFFFFFFFFFF;
			pstParser->m_uiFindStartHeader = 0;
			pstParser->m_uiSyncFrameTime = 0;
			pstParser->m_uiSyncFrameOutputCounter = 0;
		}
		else
		{
			pstParser->m_uiSequence &=~1;
			pstParser->m_SendData.m_pBuffer = NULL;
			pstParser->m_SendData.m_BufferSize = 0;
			pstParser->m_uihasPicture=FALSE;								
			pstParser->m_uiDeliveryDataLength = 0;				
			pstParser->m_uiQueue = 0xffffffff;
		}	
#else
        pstParser->m_uiSequence = 0;	
		pstParser->m_SendData.m_pBuffer = NULL;
		pstParser->m_SendData.m_BufferSize = 0;
		pstParser->m_uiDeliveryDataLength = 0;	
		pstParser->m_uihasPicture=FALSE;								
		pstParser->m_uiQueue = 0xffffffff;
        pstParser->m_uiPacketFirstSend = 0;
#endif        
	}
	return 0;
}


INT32 TSDEMUXSTParser_WriteBuffer(ST_STREAM_PARSER *pStreamParser, UINT8 *p, INT32 iCount)
{
	if( pStreamParser->m_SendData.m_pBuffer && pStreamParser->m_SendData.m_BufferSize)
	{
		if( (pStreamParser->m_uiDeliveryDataLength + iCount) < pStreamParser->m_SendData.m_BufferSize )
		{
			if (pStreamParser->m_uiPlayMode == 0)
				memcpy(pStreamParser->m_SendData.m_pBuffer+pStreamParser->m_uiDeliveryDataLength,p,iCount);
			else
				memcpy(pStreamParser->m_SendData.m_pBuffer+pStreamParser->m_SendData.m_BufferSize-iCount-pStreamParser->m_uiDeliveryDataLength,p,iCount);
			pStreamParser->m_uiDeliveryDataLength += iCount;
		}
		else
		{
			iCount = 0;
		}	
	}
	else
		iCount = -1;
	
	return iCount;
}

INT32 TSDEMUXSTParser_SendBuffer(ST_STREAM_PARSER *pstParser)
{	
	
	if( pstParser->m_SendData.m_pBuffer == NULL )
		return -1;
	
	pstParser->m_SendData.m_PrevlPTS = pstParser->m_SendData.m_lPTS;		
	pstParser->pushCallback (pstParser->appData, pstParser->m_SendData.m_pBuffer, pstParser->m_SendData.m_BufferSendSize, pstParser->m_SendData.m_lPTS, 0);
	return pstParser->m_SendData.m_BufferSendSize;

}

INT32 TSDEMUXSTParser_WriteBuffer2(ST_STREAM_PARSER *pstParser, UINT8 *p, INT32 iCount)
{
	if (pstParser->m_uiDeliveryDataLength+iCount > pstParser->m_uiEsMaxSize)
	{
		ALOGE("Flush Buffer [%08X] size[%d]", pstParser->m_SendData.m_pBuffer, pstParser->m_uiDeliveryDataLength+iCount);
		TSDEMUXSTParser_ResetBuffer(pstParser);
		iCount = 0;
	}
	else
	{
		memcpy(pstParser->m_SendData.m_pBuffer+pstParser->m_uiDeliveryDataLength,p,iCount);
		pstParser->m_uiDeliveryDataLength += iCount;
	}
	return iCount;
}

INT32 TSDEMUXSTParser_SendBuffer2(ST_STREAM_PARSER *pstParser)
{	
	INT32 send_data;
	INT64 iPTS;
	
	if (pstParser->m_SendData.m_pBuffer == NULL)
		return -1;

	/* PTS Interpolation */
	iPTS = pstParser->m_SendData.m_lPTS + pstParser->m_uiSyncFrameOutputCounter*(pstParser->m_uiSyncFrameTime*90); 
	pstParser->m_SendData.m_PrevlPTS = iPTS;
	
	//ALOGI("pushCallback m_pBuffer[%08X] size[%d]", pstParser->m_SendData.m_pBuffer, pstParser->m_uiDeliveryDataLength, 0);
	pstParser->pushCallback (pstParser->appData, pstParser->m_SendData.m_pBuffer, pstParser->m_uiDeliveryDataLength, iPTS, 0);
	if (pstParser->m_uiDeliveryDataLength != pstParser->m_uiSyncFrameSize)
	{
		ALOGE("Not Framesize data output [%d]", pstParser->m_uiDeliveryDataLength);
	}

	send_data = pstParser->m_uiDeliveryDataLength;
	pstParser->m_uiDeliveryDataLength = 0;
	pstParser->m_SendData.m_pBuffer = NULL;
	pstParser->m_SendData.m_BufferSize = 0;
	++pstParser->m_uiSyncFrameOutputCounter;
	
	return send_data;
}

INT32 TSDEMUXSTParser_H264FindStartCode(ST_STREAM_PARSER *pstParser, UINT8 *p, INT32 l, INT32 *pCount)
{
	INT32 n=0;

    if( 0x00000001==(0x00ffffff&pstParser->m_uiQueue) )
    {
        *pCount=0;
        return TRUE;
    }

    while( n<l )
    {
		#if 0
        pstParser->m_uiQueue=pstParser->m_uiQueue<<8|p[n++];
		#else
		pstParser->m_uiQueue=pstParser->m_uiQueue<<8|*p++;
		n++;
		#endif
        if( 0x00000001==(0x00ffffff&pstParser->m_uiQueue) )
        {
            *pCount=n;
            return TRUE;
        }
    }
	return FALSE;
}

INT32 TSDEMUXSTParser_ProcessH264Data(ST_STREAM_PARSER *pstParser, UINT8 *pData, UINT32 uiDataSize, INT64 lPTS)
{
	INT32 RemainDataLength;
	INT32 Position=0;	
	INT32 Count;
	UINT8* p;	
	UINT8 NalUnitType;

	p = pData;
	RemainDataLength = uiDataSize;	
	while( RemainDataLength )
	{	
		if(  TSDEMUXSTParser_H264FindStartCode(pstParser, &p[Position], RemainDataLength, &Count) )
		{
			if( RemainDataLength > Count ) //If not, NalUnitType is existed on next packet.
			{
				pstParser->m_uiQueue = 0xffffffff;

				NalUnitType=p[Position+Count]&0x1f;
				if( 0x09==NalUnitType )
				{
					if( TSDEMUXSTParser_WriteBuffer(pstParser, &p[Position], Count) >= 0 )	
					{
						if( pstParser->m_uiSequence )
						{
							pstParser->m_SendData.m_BufferSendSize = pstParser->m_uiDeliveryDataLength -3;							
							if(!pstParser->m_uiPacketFirstSend)
							{
								DTV_DEBUG (DEBUG_SEQ,"[SWDEMUX]video first frame send:size(%d)pts(%lld)!!\n", pstParser->m_SendData.m_BufferSendSize, pstParser->m_SendData.m_lPTS/90);
								pstParser->m_uiPacketFirstSend=1;	
							}

							//DTV_DEBUG (DEBUG_SEQ,"RemainLen%d, SendLen %d\n",RemainDataLength, slot->pstParser->m_SendData.m_BufferSendSize);						
							TSDEMUXSTParser_SendBuffer(pstParser);

							pstParser->m_uiSequence|=2;   
							pstParser->m_SendData.m_pBuffer = NULL;
							pstParser->m_SendData.m_BufferSize = 0;
							pstParser->m_uiDeliveryDataLength = 0;		
						}
						else
						{
							(*(UINT32 *)pstParser->m_SendData.m_pBuffer) = 0x00010000;									
							pstParser->m_uiDeliveryDataLength = 3;						
							pstParser->m_SendData.m_lPTS = lPTS;	
						}

						pstParser->m_uiSequence &=~1;	
					}

					if( pstParser->m_SendData.m_pBuffer == NULL)
					{
                        if( pstParser->getBuffer(pstParser->appData, &pstParser->m_SendData.m_pBuffer, &pstParser->m_SendData.m_BufferSize) )
                            return MPEGDEMUX_ERR_BADPARAMETER;
						(*(UINT32 *)pstParser->m_SendData.m_pBuffer) = 0x00010000;									
						pstParser->m_uiDeliveryDataLength = 3;											
						pstParser->m_SendData.m_lPTS = lPTS;	
					}             
				}
				else
				{
					if(	TSDEMUXSTParser_WriteBuffer(pstParser, &p[Position], Count) >= 0 )
					{
						if( 0x07==NalUnitType ) //find SPS
						{
							if(!pstParser->m_uiPacketFirstSend)
							{
								if( pstParser->m_uiDeliveryDataLength <= 12 )
									pstParser->m_uiSequence|=1;
							}
							else
								pstParser->m_uiSequence|=1;
						}
					}                
				}

				RemainDataLength-=Count;
				Position+=Count;
			}
			else
			{
				//DTV_DEBUG (DEBUG_SEQ, "Find start code, but need more data\n");
				TSDEMUXSTParser_WriteBuffer(pstParser, &p[Position], RemainDataLength);
				break;
			}
		}
		else
		{
			TSDEMUXSTParser_WriteBuffer(pstParser, &p[Position], RemainDataLength);
			break;
		}

	}	
	//DEBUG_MSG("out of %s\n", __func__);		
	return TRUE;
}

INT32 TSDEMUXSTParser_H264FindStartCode_Reverse(ST_STREAM_PARSER *pstParser, UINT8 *p, INT32 l, INT32 *pCount)
{
	INT32 n=0;

	while( n<l )
	{
		#if 0
		pstParser->m_uiQueue=pstParser->m_uiQueue<<8|p[n++];
		#else
		pstParser->m_uiQueue=pstParser->m_uiQueue>>8 | (UINT32)*(--p)<<24;
		n++;
		#endif
		if( 0x00000100==(0xffffff00&pstParser->m_uiQueue) )
		{
			*pCount=n;
			return TRUE;
		}
	}
	return FALSE;
}

INT32 TSDEMUXSTParser_ProcessH264Data_Reverse(ST_STREAM_PARSER *pstParser, UINT8 *pData, UINT32 uiDataSize, INT64 lPTS)
{
	INT32 RemainDataLength;
	INT32 Count;
	UINT8* p;
	UINT8 NalUnitType;

	p = pData;
	RemainDataLength = uiDataSize;
	while( RemainDataLength )
	{
		if(  TSDEMUXSTParser_H264FindStartCode_Reverse(pstParser, &p[RemainDataLength], RemainDataLength, &Count) )
		{
			RemainDataLength-=Count;

			NalUnitType=pstParser->m_uiQueue&0x1f;
			if( 0x09==NalUnitType )
			{
				pstParser->m_SendData.m_lPTS = lPTS;
				if( TSDEMUXSTParser_WriteBuffer(pstParser, &p[RemainDataLength], Count) >= 0 )
				{
					if( pstParser->m_uiSequence )
					{
						pstParser->m_SendData.m_BufferSendSize = pstParser->m_uiDeliveryDataLength;
						if(!pstParser->m_uiPacketFirstSend)
						{
							DTV_DEBUG (DEBUG_SEQ,"[SWDEMUX]video first frame send:size(%d)pts(%lld)!!\n", pstParser->m_SendData.m_BufferSendSize, pstParser->m_SendData.m_lPTS/90);
							pstParser->m_uiPacketFirstSend=1;
						}

						//DTV_DEBUG (DEBUG_SEQ,"RemainLen%d, SendLen %d\n",RemainDataLength, slot->pstParser->m_SendData.m_BufferSendSize);
						TSDEMUXSTParser_SendBuffer(pstParser);

						pstParser->m_uiSequence|=2;
						pstParser->m_SendData.m_pBuffer = NULL;
						pstParser->m_SendData.m_BufferSize = 0;
						pstParser->m_uiDeliveryDataLength = 0;
					}
					pstParser->m_uiSequence &=~1;
				}

				if( pstParser->m_SendData.m_pBuffer == NULL)
				{
					if( pstParser->getBuffer(pstParser->appData, &pstParser->m_SendData.m_pBuffer, &pstParser->m_SendData.m_BufferSize) )
						return MPEGDEMUX_ERR_BADPARAMETER;
				}
			}
			else
			{
				if(	TSDEMUXSTParser_WriteBuffer(pstParser, &p[RemainDataLength], Count) >= 0 )
				{
					if( 0x07==NalUnitType ) //find SPS
					{
						if(!pstParser->m_uiPacketFirstSend)
						{
							if( pstParser->m_uiDeliveryDataLength <= 12 )
								pstParser->m_uiSequence|=1;
						}
						else
							pstParser->m_uiSequence|=1;
					}
				}
			}
		}
		else
		{
			TSDEMUXSTParser_WriteBuffer(pstParser, p, RemainDataLength);
			break;
		}

	}
	//DEBUG_MSG("out of %s\n", __func__);
	return TRUE;
}

INT32 TSDEMUXSTParser_MPEG2FindStartCode(ST_STREAM_PARSER *pstParser, UINT8 *p, INT32 l, INT32 *pCount)
{
    INT32 n=0;

    while( n<l )
    {
		#if 0
        pstParser->m_uiQueue=pstParser->m_uiQueue<<8|p[n++];
		#else
		pstParser->m_uiQueue=pstParser->m_uiQueue<<8|*p++;
		n++;
		#endif

        if( PICTURE_START_CODE==pstParser->m_uiQueue || SEQUENCE_HEADER_CODE==pstParser->m_uiQueue || GROUP_START_CODE==pstParser->m_uiQueue )
        {			
            if( SEQUENCE_HEADER_CODE==pstParser->m_uiQueue )
            {				
                pstParser->m_uiSequence|=1;
            }
            *pCount=n;
            return TRUE;
        }
    }

    return FALSE;
}

INT32 TSDEMUXSTParser_ProcessMPEG2Data(ST_STREAM_PARSER *pstParser, UINT8 *pData, UINT32 uiDataSize, INT64 lPTS)
{
	INT32 RemainDataLength;
	INT32 Position=0, Count;	
	UINT8 *p;

	p = pData;
	RemainDataLength = uiDataSize;	
	while( RemainDataLength )
	{	
		if(  TSDEMUXSTParser_MPEG2FindStartCode(pstParser, &p[Position], RemainDataLength, &Count) && pstParser->m_uiSequence )
		{
			if( pstParser->m_uihasPicture )
			{	
				if( TSDEMUXSTParser_WriteBuffer(pstParser, &p[Position], Count) >= 0 )
				{	
					pstParser->m_SendData.m_BufferSendSize = pstParser->m_uiDeliveryDataLength-4;							
					TSDEMUXSTParser_SendBuffer(pstParser);
					if(!pstParser->m_uiPacketFirstSend )
					{						
						DTV_DEBUG (DEBUG_SEQ,"[SWDEMUX]video first frame send:size(%d)pts(%lld)!!\n", pstParser->m_SendData.m_BufferSendSize, pstParser->m_SendData.m_lPTS/90);
						pstParser->m_uiPacketFirstSend=1;		
					}															
					pstParser->m_uiSequence|=2;
	                pstParser->m_uiSequence&=~1;
					pstParser->m_SendData.m_pBuffer = NULL;
					pstParser->m_SendData.m_BufferSize = 0;
					pstParser->m_uihasPicture=FALSE;								
					pstParser->m_uiDeliveryDataLength = 0;		
					
				}	
				
			}

			if( TSDEMUXSTParser_WriteBuffer(pstParser, &p[Position], Count) == -1)			
			{
                if( pstParser->getBuffer(pstParser->appData, &pstParser->m_SendData.m_pBuffer, &pstParser->m_SendData.m_BufferSize) )
                    return MPEGDEMUX_ERR_BADPARAMETER;
				(*(UINT32 *)pstParser->m_SendData.m_pBuffer) = 0x00010000|(pstParser->m_uiQueue<<24);				
				pstParser->m_uiDeliveryDataLength = 4;				
			}

			if( PICTURE_START_CODE==pstParser->m_uiQueue && pstParser->m_SendData.m_pBuffer )
			{					
				pstParser->m_SendData.m_lPTS = lPTS;	
				pstParser->m_uihasPicture=TRUE;
			}

			RemainDataLength-=Count;
			Position+=Count;
		}
		else
		{	
			TSDEMUXSTParser_WriteBuffer(pstParser, &p[Position], RemainDataLength);
			break;
		}
	}	
	return TRUE;
}

INT32 TSDEMUXSTParser_MPEG2FindStartCode_Reverse(ST_STREAM_PARSER *pstParser, UINT8 *p, INT32 l, INT32 *pCount)
{
	INT32 n=0;

	while( n<l )
	{
		#if 0
		pstParser->m_uiQueue=pstParser->m_uiQueue<<8|p[n++];
		#else
		pstParser->m_uiQueue=pstParser->m_uiQueue>>8 | (UINT32)*(--p)<<24;
		n++;
		#endif

		if( PICTURE_START_CODE==pstParser->m_uiQueue || SEQUENCE_HEADER_CODE==pstParser->m_uiQueue || GROUP_START_CODE==pstParser->m_uiQueue )
		{
			if( SEQUENCE_HEADER_CODE==pstParser->m_uiQueue )
			{
				pstParser->m_uiSequence|=1;
			}
			*pCount=n;
			return TRUE;
		}
	}

	return FALSE;
}

INT32 TSDEMUXSTParser_ProcessMPEG2Data_Reverse(ST_STREAM_PARSER *pstParser, UINT8 *pData, UINT32 uiDataSize, INT64 lPTS)
{
	INT32 RemainDataLength;
	INT32 Count;
	UINT8 *p;

	p = pData;
	RemainDataLength = uiDataSize;
	while( RemainDataLength > 0)
	{
		if(  TSDEMUXSTParser_MPEG2FindStartCode_Reverse(pstParser, &p[RemainDataLength], RemainDataLength, &Count) )
		{
			RemainDataLength-=Count;

			if (pstParser->m_uiSequence == 0)
				continue;

			if( PICTURE_START_CODE==pstParser->m_uiQueue && pstParser->m_SendData.m_pBuffer )
			{
				pstParser->m_SendData.m_lPTS = lPTS;
				pstParser->m_uihasPicture=TRUE;
			}

			if( TSDEMUXSTParser_WriteBuffer(pstParser, &p[RemainDataLength], Count) >= 0)
			{
				if( pstParser->m_uihasPicture==TRUE )
				{
					pstParser->m_SendData.m_BufferSendSize = pstParser->m_uiDeliveryDataLength;
					TSDEMUXSTParser_SendBuffer(pstParser);
					if(!pstParser->m_uiPacketFirstSend )
					{
						DTV_DEBUG (DEBUG_SEQ,"[SWDEMUX]video first frame send:size(%d)pts(%lld)!!\n", pstParser->m_SendData.m_BufferSendSize, pstParser->m_SendData.m_lPTS/90);
						pstParser->m_uiPacketFirstSend=1;
					}
					pstParser->m_uiSequence|=2;
					pstParser->m_uiSequence&=~1;
					pstParser->m_SendData.m_pBuffer = NULL;
					pstParser->m_SendData.m_BufferSize = 0;
					pstParser->m_uihasPicture=FALSE;
					pstParser->m_uiDeliveryDataLength = 0;

					if( pstParser->getBuffer(pstParser->appData, &pstParser->m_SendData.m_pBuffer, &pstParser->m_SendData.m_BufferSize) )
						return MPEGDEMUX_ERR_BADPARAMETER;
				}
			}
			else
			{
				if( pstParser->getBuffer(pstParser->appData, &pstParser->m_SendData.m_pBuffer, &pstParser->m_SendData.m_BufferSize) )
					return MPEGDEMUX_ERR_BADPARAMETER;
			}
		}
		else
		{
			TSDEMUXSTParser_WriteBuffer(pstParser, p, RemainDataLength);
			break;
		}
	}
	return TRUE;
}

INT32 TSDEMUXSTParser_AC3FindStartCode(ST_STREAM_PARSER *pstParser, UINT8 *p, INT32 l, INT32 *pCount)
{
	INT32 n=0;
	UINT8 bsid;
	UINT8 fscod;
	UINT8 frmsizecod;
	UINT8 samplingratecod;
	UINT32 framesize;

	while( n<l )
	{
		pstParser->m_ulliQueue = (UINT64)(pstParser->m_ulliQueue<<8|*p++);
		n++;

		if ((UINT64)(pstParser->m_ulliQueue & AUDIO_AC3_HEADER_SIGNATURE_MASK) == (UINT64)AUDIO_AC3_HEADER_SIGNATURE)
		{
			/* 0B 77 xx xx fscod bsi xx xx */
			bsid = (UINT8)((pstParser->m_ulliQueue>>19)&0x1F);
			fscod = (UINT8)((pstParser->m_ulliQueue>>24)&0xFF);
			samplingratecod = (fscod>>6)&0x03;
			frmsizecod = fscod&0x3F;

			if (bsid <= 8 && samplingratecod < 3 && frmsizecod < 38)
			{
				/* 1words = 16bits */
				framesize = (AC3_FrameSizeCodeTable[frmsizecod][samplingratecod+1]*2);
				
				/* Check The header's SyncFrameSize = sending data size */
				if (pstParser->m_uiDeliveryDataLength)
				{
					if ( framesize == pstParser->m_uiSyncFrameSize )
					{
						if (n < sizeof(pstParser->m_ulliQueue))
						{
							if (framesize <= (pstParser->m_uiDeliveryDataLength - (sizeof(pstParser->m_ulliQueue) - n)))
							{
								pstParser->m_uiFindStartHeader = 1;
								pstParser->m_uiSyncFrameSize = framesize;
								pstParser->m_uiSyncFrameTime = pstParser->m_uiSyncFrameSize/(AC3_FrameSizeCodeTable[frmsizecod][0]/8000);
								*pCount=n;
								return TRUE;
							}
						}
						else
						{
							if (framesize <= (pstParser->m_uiDeliveryDataLength + (n - sizeof(pstParser->m_ulliQueue))))
							{
								pstParser->m_uiFindStartHeader = 1;
								pstParser->m_uiSyncFrameSize = framesize;
								pstParser->m_uiSyncFrameTime = pstParser->m_uiSyncFrameSize/(AC3_FrameSizeCodeTable[frmsizecod][0]/8000);
								*pCount=n;
								return TRUE;
							}
						}
					}
				}
				else
				{
					pstParser->m_uiFindStartHeader = 1;
					pstParser->m_uiSyncFrameSize = framesize;
					pstParser->m_uiSyncFrameTime = pstParser->m_uiSyncFrameSize/(AC3_FrameSizeCodeTable[frmsizecod][0]/8000);
					*pCount=n;
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

INT32 TSDEMUXSTParser_ProcessAC3Data(ST_STREAM_PARSER *pstParser, UINT8 *pData, UINT32 uiDataSize, INT64 lPTS, INT32 pusi)
{
	int	i;
	INT32 Count;

	if (pusi)
	{
		pstParser->m_SendData.m_lPTS = lPTS;
		pstParser->m_uiSyncFrameOutputCounter = 0;
	}

	if (TSDEMUXSTParser_AC3FindStartCode(pstParser, &pData[0], uiDataSize, &Count))
	{
		/* Send out data if previous packet is already ready */
		if (pstParser->m_uiDeliveryDataLength)
		{
			/* The header is located on middle of two TS packets */
			if (Count < sizeof(pstParser->m_ulliQueue))
			{
				pstParser->m_uiDeliveryDataLength -= sizeof(pstParser->m_ulliQueue)-Count;
				TSDEMUXSTParser_SendBuffer2(pstParser);
			}
			/* The header is located in one TS packet */
			else
			{
				TSDEMUXSTParser_WriteBuffer2(pstParser, &pData[0], Count-sizeof(pstParser->m_ulliQueue));
				TSDEMUXSTParser_SendBuffer2(pstParser);
			}
		}

		/* Make new packet */
		if (!pstParser->m_SendData.m_pBuffer || !pstParser->m_SendData.m_BufferSize)
		{
            if( pstParser->getBuffer(pstParser->appData, &pstParser->m_SendData.m_pBuffer, &pstParser->m_SendData.m_BufferSize) )
                return MPEGDEMUX_ERR_BADPARAMETER;
			pstParser->m_uiDeliveryDataLength = 0;
			//ALOGI("m_pBuffer[%08X] m_BufferSize[%d] baseBuffer[%08X] bufferSize[%d]", slot->tempBuffer, slot->pstParser->m_SendData.m_BufferSize, slot->baseBuffer, slot->bufferSize);
		}
		
		for ( i = 0; i < sizeof(pstParser->m_ulliQueue); ++i )
			(*(UINT8*)(pstParser->m_SendData.m_pBuffer+i)) = (UINT8)((pstParser->m_ulliQueue>>((sizeof(pstParser->m_ulliQueue)-1-i)*8))&0xFF);
		pstParser->m_uiDeliveryDataLength = sizeof(pstParser->m_ulliQueue);
		TSDEMUXSTParser_WriteBuffer2(pstParser,&pData[Count],uiDataSize-Count);
	}
	else
	{	
		if ( pstParser->m_uiFindStartHeader )
			TSDEMUXSTParser_WriteBuffer2(pstParser, &pData[0], uiDataSize);
	}

	return TRUE;
}

INT32 TSDEMUXSTParser_ProcessData(ST_STREAM_PARSER *pstParser, UINT8 *pData, UINT32 uiDataSize, INT64 lPTS, INT32 pusi)
{
	INT32 ret = MPEGDEMUX_ERR_NOERROR;

	if(!pstParser || !pstParser->m_Enable)
	{
		return ret;
	}

	if( pstParser->m_uiStreamType == STREAMTYPE_VIDEO_AVCH264 )
		if (pstParser->m_uiPlayMode == 0)
			ret = TSDEMUXSTParser_ProcessH264Data(pstParser, pData, uiDataSize, lPTS);
		else
			ret = TSDEMUXSTParser_ProcessH264Data_Reverse(pstParser, pData, uiDataSize, lPTS);
	else if (pstParser->m_uiStreamType == STREAMTYPE_VIDEO)
		if (pstParser->m_uiPlayMode == 0)
			ret = TSDEMUXSTParser_ProcessMPEG2Data(pstParser, pData, uiDataSize, lPTS);
		else
			ret = TSDEMUXSTParser_ProcessMPEG2Data_Reverse(pstParser, pData, uiDataSize, lPTS);
	else if (pstParser->m_uiStreamType == STREAMTYPE_AUDIO_AC3_2)
		ret = TSDEMUXSTParser_ProcessAC3Data(pstParser, pData, uiDataSize, lPTS, pusi);
	else
		ret = MPEGDEMUX_ERR_NOTSUPORTSTEAMPARSER;
	
	return ret;
}

#define MPG_CODE_SEQUENCE_START		0x000001B3
#define MPG_CODE_PICTURE_START		0x00000100
#define MPG_CODE_SEQUENCE_EXT_START	0x000001B5

INT32 IsMpegKeyFrame(UINT8 *pbyBuffer, UINT32 lBuffLen)
{
	UINT8   *buff = pbyBuffer;
	UINT8   *buff_end = pbyBuffer + lBuffLen;
	UINT32	sync;

	sync = ((UINT32)buff[0] << 16) | ((UINT32)buff[1] << 8) | ((UINT32)buff[2]);

	while( (buff+5) < buff_end )
	{
		sync = (sync << 8) | buff[3];
		if(0)//sync == MPG_CODE_SEQUENCE_START)
			return TRUE;
		else if(sync == MPG_CODE_PICTURE_START)
		{
			if( ((buff[5] >> 3) & 0x7) == 1)	// frame_type I
				return 1;
		}
		buff++;
	}

	return 0;
}

#define NAL_UNSPECIFIED1	0
#define NAL_SLICE			1
#define NAL_SLICE_DPA		2
#define NAL_SLICE_DPB		3
#define NAL_SLICE_DPC		4
#define NAL_SLICE_IDR		5	 // ref_idc != 0 
#define NAL_SEI				6    // ref_idc == 0 
#define NAL_SPS				7
#define NAL_PPS				8
#define NAL_AUD				9
#define NAL_END_OF_SEQUENCE 10
#define NAL_END_OF_STREAM	11
#define NAL_FILLER_DATA		12
#define NAL_SPS_EXTENSION	13
#define NAL_SUBSET_SPS		15
#define NAL_AUX_PICTURE		19
#define NAL_AUD_EXT			24	// HDMV dependent-view AUD (?)

INT32 IsH264KeyFrame(UINT8 *pbyBuffer, UINT32 lBuffLen)
{
	UINT8   *buff = pbyBuffer;
	UINT8   *buff_end = pbyBuffer + lBuffLen;
	UINT32		sync;

	sync = ((UINT32)buff[0] << 8) | ((UINT32)buff[1]);

	while( buff+4 < buff_end )
	{
		sync <<= 8;
		sync |= buff[2];

		if( (sync & 0x00FFFFFF) == 0x000001 )
		{
			switch( buff[3] & 0x1F )
			{
			case NAL_SLICE_IDR:
			case NAL_SPS:
				return TRUE;
			case NAL_AUD:
				if( (buff[4] >> 5) == 0 )
					return 1;
				break;
			}
		}
		buff++;
	}

	return 0;
}

INT32 TSDEMUXSTParser_IsKeyFrame(ST_STREAM_PARSER *pstParser, UINT8 *pbyBuffer, UINT32 lBuffLen)
{
	if( pstParser->m_uiStreamType == STREAMTYPE_VIDEO_AVCH264 )
		return IsH264KeyFrame(pbyBuffer, lBuffLen);
	else if (pstParser->m_uiStreamType == STREAMTYPE_VIDEO)
		return IsMpegKeyFrame(pbyBuffer, lBuffLen);

	return 1;
}
