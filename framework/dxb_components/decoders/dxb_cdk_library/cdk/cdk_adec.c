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
#include "cdk_core.h"
#include "cdk.h"
#include "adec_vars.h"
#include "cdk_audio.h"
#include <dlfcn.h>

#if defined(HAVE_LINUX_PLATFORM)
#define LOG_TAG	"CDK_AUDIO"

#undef DPRINTF
#undef DSTATUS

//#define DEBUG_ON
#ifdef DEBUG_ON
#define DPRINTF(x...)	 ALOGE(x)
#define DSTATUS(x...)	 ALOGD(x)
#else
#define DPRINTF(x...)
#define DSTATUS(x...)
#endif

#endif

//#define AUDIO_DEC_PROFILE
#ifdef AUDIO_DEC_PROFILE
#include "time.h"
static unsigned int dec_time[30] = {0,};
static unsigned int time_cnt = 0;
#endif

#define gspfADec						gsADec->gspfADec
#define gsADecInput 					gsADec->gsADecInput
#define gsADecOutput 					gsADec->gsADecOutput
#define gsADecInit 						gsADec->gsADecInit
#define gpucAudioBuffer 				gsADec->gpucAudioBuffer
#define guiAudioMinDataSize 			gsADec->guiAudioMinDataSize
#define guiAudioInBufferSize 			gsADec->guiAudioInBufferSize
#define guiAudioContinueThroughErrors 	gsADec->guiAudioContinueThroughErrors
#define giAudioErrorCount 				gsADec->giAudioErrorCount
#define gspLatmHandle					gsADec->gspLatmHandle

cdk_result_t
cdk_adec_init( cdk_core_t* pCdk, cdmx_info_t *pCdmxInfo, int iAdecType, int iCtype, cdk_audio_func_t* callback, ADEC_VARS* gsADec)
{
	int ret = CDK_ERR_NONE;
	if (gsADecInit.m_unAudioCodecParams.m_unAAC.m_uiChannelMasking) {
		gsADecInit.m_unAudioCodecParams.m_unAAC.m_uiChannelMasking = 0;
	}

#if defined(HAVE_LINUX_PLATFORM)
	gspfADec = callback;
#else
	gspfADec = gspfADecList[iAdecType];
#endif
#ifdef AUDIO_DEC_PROFILE
	time_cnt = 0;
#endif

//#define AAC_LIB_NAME  "libTCC.DxB.aacdec.so"
#define AAC_LIB_NAME  "libtccaacdec.so"
#define AC3_LIB_NAME  "libTCC.DxB.ac3dec.so"
#define MP2_LIB_NAME  "libTCC.DxB.mp2dec.so"
#define MP3_LIB_NAME  "libTCC.DxB.mp3dec.so"
#define BSAC_LIB_NAME "libTCC.DxB.bsacdec.so"

	if (gspfADec == NULL)
	{
		switch (iAdecType)
		{
		case AUDIO_ID_AAC:
			pCdk->pfDLLModule = dlopen(AAC_LIB_NAME, RTLD_LAZY | RTLD_GLOBAL);
			break;
		case AUDIO_ID_AC3:
		case AUDIO_ID_DDP:
			pCdk->pfDLLModule = dlopen(AC3_LIB_NAME, RTLD_LAZY | RTLD_GLOBAL);
			break;
		case AUDIO_ID_MP2:
			pCdk->pfDLLModule = dlopen(MP2_LIB_NAME, RTLD_LAZY | RTLD_GLOBAL);
			break;
		case AUDIO_ID_MP3:
			pCdk->pfDLLModule = dlopen(MP3_LIB_NAME, RTLD_LAZY | RTLD_GLOBAL);
			break;
		case AUDIO_ID_BSAC:
			pCdk->pfDLLModule = dlopen(BSAC_LIB_NAME, RTLD_LAZY | RTLD_GLOBAL);
			break;
		default:
			pCdk->pfDLLModule = NULL;
			break;
		}

		if( pCdk->pfDLLModule == NULL )
		{
			ALOGE("Load DEC library failed(Audio Type = %d)", iAdecType);
		}
		else
		{
			ALOGI("DEC Library Loaded (Audio Type = %d)", iAdecType);
			gspfADec = dlsym(pCdk->pfDLLModule, "TCC_AAC_DEC");// TCC_AAC_DEC
		}
	}

	if( gspfADec )
	{
		/* audio common */
		gsADecInput.m_eSampleRate = pCdmxInfo->m_sAudioInfo.m_iSamplePerSec;
		gsADecInput.m_uiNumberOfChannel = pCdmxInfo->m_sAudioInfo.m_iChannels;
		gsADecOutput.m_uiBitsPerSample = pCdmxInfo->m_sAudioInfo.m_iBitsPerSample;
		if (pCdmxInfo->m_sAudioInfo.m_iSamplesPerFrame == 960)
		{
			gsADecInput.m_uiSamplesPerChannel = pCdmxInfo->m_sAudioInfo.m_iSamplesPerFrame;
		}

		if (gsADecOutput.m_uiBitsPerSample == 0)
		{
			gsADecOutput.m_uiBitsPerSample = 16;
		}
		/* audio common extra */
		gsADecInit.m_pucExtraData = pCdmxInfo->m_sAudioInfo.m_pExtraData;
		gsADecInit.m_iExtraDataLen = pCdmxInfo->m_sAudioInfo.m_iExtraDataLen;

		/* out-pcm setting */
		gsADecOutput.m_ePcmInterLeaved = 1;	// 0 or 1
		gsADecOutput.m_iNchannelOrder[CH_LEFT_FRONT] = 1;	//first channel
		gsADecOutput.m_iNchannelOrder[CH_RIGHT_FRONT] = 2;	//second channel
		//gsADecOutput.nChannelOrder[CH_CENTER] = ;
		//gsADecOutput.nChannelOrder[CH_LEFT_REAR] = ;
		//gsADecOutput.nChannelOrder[CH_RIGHT_REAR] = ;
		//gsADecOutput.nChannelOrder[CH_LEFT_SIDE] = ;
		//gsADecOutput.nChannelOrder[CH_RIGHT_SIDE] = ;
		//gsADecOutput.nChannelOrder[CH_LFE] = ;
		//gsADecInit.m_iDownMixMode = ;

		/* audio callback */
		gsADecInit.m_pfMalloc = pCdk->m_psCallback->m_pfMalloc;
		gsADecInit.m_pfRealloc = pCdk->m_psCallback->m_pfRealloc;
		gsADecInit.m_pfFree = pCdk->m_psCallback->m_pfFree;
		gsADecInit.m_pfMemcpy = pCdk->m_psCallback->m_pfMemcpy;
		gsADecInit.m_pfMemmove = pCdk->m_psCallback->m_pfMemmove;
		gsADecInit.m_pfMemset = pCdk->m_psCallback->m_pfMemset;

		gsADecInit.m_psAudiodecInput = &gsADecInput;
		gsADecInit.m_psAudiodecOutput = &gsADecOutput;

		guiAudioContinueThroughErrors = 1;
		giAudioErrorCount = 0;
		guiAudioMinDataSize = 0;

		if( gpucAudioBuffer == NULL )
		{
			guiAudioInBufferSize = AUDIO_MAX_INPUT_SIZE;
			gpucAudioBuffer = gsADecInit.m_pfMalloc(guiAudioInBufferSize);

			if( gpucAudioBuffer == NULL )
			{
				DPRINTF( "[CDK_CORE] audio DECODER gpucAudioBuffer allocation fail\n");
				return -1;
			}
		}

		//=====*=====*=====*   C O D E C - D E P E N D E N T    C O D E  =====*=====*=====*=====*=====*=====*=====*=START.

		switch ( iAdecType )
		{

#ifdef INCLUDE_AAC_DEC
		/* AAC Decoder */
		case AUDIO_ID_AAC:
			//if(ch > 2) downmix to stereo
			gsADecInit.m_iDownMixMode = 1;

			gspLatmHandle = NULL;

#define LATM_PARSER_LIB_NAME "libTCC.DxB.latmdmx.so"

			pCdk->pfLatmDLLModule = dlopen(LATM_PARSER_LIB_NAME, RTLD_LAZY | RTLD_GLOBAL);
		    if( pCdk->pfLatmDLLModule == NULL )
			{
		        ALOGE("Load library '%s' failed", LATM_PARSER_LIB_NAME);
		        return -1;
		    }
			else
		    {
				ALOGI("Library '%s' Loaded", LATM_PARSER_LIB_NAME);
		    }

			pCdk->pfLatmInit = dlsym(pCdk->pfLatmDLLModule, "Latm_Init");
		    if( pCdk->pfLatmInit == NULL )
			{
				ALOGE("Load symbol Latm_Init failed");
		        return -1;
		    }

			pCdk->pfLatmClose = dlsym(pCdk->pfLatmDLLModule, "Latm_Close");
		    if( pCdk->pfLatmClose == NULL )
			{
				ALOGE("Load symbol Latm_Close failed");
		        return -1;
		    }

			pCdk->pfLatmGetFrame = dlsym(pCdk->pfLatmDLLModule, "Latm_GetFrame");
		    if( pCdk->pfLatmGetFrame == NULL )
			{
				ALOGE("Load symbol Latm_GetFrame failed");
		        return -1;
		    }

			pCdk->pfLatmFlushBuffer =  dlsym(pCdk->pfLatmDLLModule, "Latm_FlushBuffer");
		    if( pCdk->pfLatmFlushBuffer == NULL )
			{
				ALOGE("Load symbol Latm_FlushBuffer failed");
		        return -1;
		    }

			/* START : latm demuxer init -----------------------------------------*/
			 if( ( pCdmxInfo->m_sAudioInfo.m_ulSubType == AUDIO_SUBTYPE_AAC_ADTS ) ||
			 ( pCdmxInfo->m_sAudioInfo.m_ulSubType == AUDIO_SUBTYPE_AAC_LATM ) )
			{
				unsigned char* pucLatmstream;
				int iStreamSize;

				pucLatmstream = NULL;
				iStreamSize = 0;

				if( pCdmxInfo->m_sAudioInfo.m_pExtraData && pCdmxInfo->m_sAudioInfo.m_iExtraDataLen )
				{
					pucLatmstream = pCdmxInfo->m_sAudioInfo.m_pExtraData;
					iStreamSize = pCdmxInfo->m_sAudioInfo.m_iExtraDataLen;
				}

				if( gsADecInput.m_eSampleRate == 0 && pucLatmstream == 0 && iStreamSize == 0 )
				{
					DPRINTF( TC_MAGENTA"[CDK_CORE] need latm stream for codec-init \n"T_DEFAULT );
					return -1;
				}

				gspLatmHandle = pCdk->pfLatmInit( pucLatmstream, iStreamSize, (int *)&gsADecInput.m_eSampleRate, (int *)&gsADecInput.m_uiNumberOfChannel, (void*)pCdk->m_psCallback, TF_AAC_LOAS);
				if( gspLatmHandle == NULL )
				{
					DPRINTF( TC_MAGENTA"[CDK_CORE] latm demuxer init failed \n"T_DEFAULT );
					return -1;
				}

#if 1 //This modification is for TCC803X (open aacdecoder)
				if(gsADecInput.m_uiNumberOfChannel == 0){
					gsADecInput.m_uiNumberOfChannel = 2;
				}

				if(gsADecInit.m_unAudioCodecParams.m_unAAC.m_iAACObjectType == 0){
					gsADecInit.m_unAudioCodecParams.m_unAAC.m_iAACObjectType = 2;
				}
#endif
				DPRINTF( TC_MAGENTA"[CDK_CORE] latm demuxer init ok \n"T_DEFAULT );

				gsADecInit.m_pucExtraData = NULL;	// No GASpecificConfig
				gsADecInit.m_iExtraDataLen = 0;
			}
			/* End : latm demuxer init -----------------------------------------*/

			if( iCtype == CONTAINER_TYPE_EXT_F && ( pCdmxInfo->m_sAudioInfo.m_iActualRate == (gsADecInput.m_eSampleRate*2)) )
			{
				//SBR present
				gsADecInit.m_unAudioCodecParams.m_unAAC.m_iAACObjectType = 5;
			}
			//if(ch == 1) force upmix to stereo
			gsADecInit.m_unAudioCodecParams.m_unAAC.m_iAACForceUpmix = 1;
			//if(samplerate < 32000) samplerate *= 2
			gsADecInit.m_unAudioCodecParams.m_unAAC.m_iAACForceUpsampling = 1;
			guiAudioMinDataSize = 4;
			if( ( pCdmxInfo->m_sAudioInfo.m_iMinDataSize != 0 ) && ( iCtype == CONTAINER_TYPE_AUDIO) )
			{
				guiAudioMinDataSize = pCdmxInfo->m_sAudioInfo.m_iMinDataSize;
			}

			if( iCtype == CONTAINER_TYPE_MP4 )
			{
				guiAudioMinDataSize = 0;
			}
			break;
#endif

#ifdef INCLUDE_MP3_DEC
		/* MP3 Decoder */
		case AUDIO_ID_MP3:
			guiAudioMinDataSize = 32;
			break;
#endif

//#ifdef INCLUDE_AC3_DEC
		/* AC3 Decoder */
		case AUDIO_ID_AC3:
			/* if all values are zero, set as typical values, in order that customer change values of m_unAC3Dec. */
			#define unAC3Dec gsADecInit.m_unAudioCodecParams.m_unAC3Dec
			if( unAC3Dec.m_iAC3Compmode == 0
				&& unAC3Dec.m_iAC3Lfe == 0
				&& unAC3Dec.m_iAC3DualmonoMode == 0
				&& unAC3Dec.m_iAC3DynamicScaleHi == 0
				&& unAC3Dec.m_iAC3DynamicScaleLow == 0
				&& unAC3Dec.m_iAC3KaraokeMode == 0
				&& unAC3Dec.m_iAC3Outputmode == 0
				&& unAC3Dec.m_iAC3Stereomode == 0
				&& unAC3Dec.m_iAC3DepositSyncinfo == 0
				)
			{
				unAC3Dec.m_iAC3Compmode = 2;
				unAC3Dec.m_iAC3Lfe = 0;
				unAC3Dec.m_iAC3DualmonoMode = 0;
				unAC3Dec.m_iAC3DynamicScaleHi = 0x40000000;
				unAC3Dec.m_iAC3DynamicScaleLow = 0x40000000;
				unAC3Dec.m_iAC3KaraokeMode = 3;
				unAC3Dec.m_iAC3Outputmode = 2;
				unAC3Dec.m_iAC3Stereomode = 0;
				unAC3Dec.m_iAC3DepositSyncinfo = 0xff;
			}

			guiAudioMinDataSize = 64;
			break;

#ifdef INCLUDE_MP2_DEC
		/* MP2 Decoder */
		case AUDIO_ID_MP2:
			guiAudioMinDataSize = 32;
			gsADecInit.m_unAudioCodecParams.m_unMP2.m_iDABMode = (pCdk->m_iAudioProcessMode == AUDIO_BROADCAST_MODE) ? 1 : 0;
			break;
#endif

#ifdef INCLUDE_DTS_DEC
		/* DTS Decoder */
		case AUDIO_ID_DTS:

			gsADecInit.m_unAudioCodecParams.m_unDTS.m_iDTSSetOutCh = TCAS_CODEC_LR;
			gsADecInit.m_unAudioCodecParams.m_unDTS.m_iDTSInterleavingFlag = 1;
			gsADecInit.m_unAudioCodecParams.m_unDTS.m_iDTSUpSampleRate = 48000;
			gsADecInit.m_unAudioCodecParams.m_unDTS.m_iDTSDrcPercent = 0;
			gsADecInit.m_unAudioCodecParams.m_unDTS.m_iDTSLFEMix = TCAS_CODEC_NOLFEMIX;
			break;
#endif

#ifdef INCLUDE_ARMWBPLUS_DEC
		/* ARM-wb+ Decoder */
		case AUDIO_ID_AMRWBP:
			gsADecInit.m_unAudioCodecParams.m_unAWP.m_iAWPOutFS			    =	gsADecInput.m_eSampleRate;
			gsADecInit.m_unAudioCodecParams.m_unAWP.m_iAWPSelectCodec		=	TCAS_CODEC_AMRWB;
			gsADecInit.m_unAudioCodecParams.m_unAWP.m_iAWPStereoThruMono	=	0;
			gsADecInit.m_unAudioCodecParams.m_unAWP.m_iAWPUseLimitter		=	1;
			gsADecInit.m_unAudioCodecParams.m_unAWP.m_pcOpenFileName		=	0;
			break;
#endif

#ifdef INCLUDE_WMA_DEC
		/* WMA Decoder */
		case AUDIO_ID_WMA:
			gsADecInput.m_uiBitRates = pCdmxInfo->m_sAudioInfo.m_iAvgBytesPerSec<<3;
			gsADecInit.m_unAudioCodecParams.m_unWMADec.m_uiNBlockAlign = pCdmxInfo->m_sAudioInfo.m_iBlockAlign;
			gsADecInit.m_unAudioCodecParams.m_unWMADec.m_uiWFormatTag = pCdmxInfo->m_sAudioInfo.m_iFormatId;
			gsADecInit.m_iExtraDataLen = pCdmxInfo->m_sAudioInfo.m_iExtraDataLen;
			gsADecInit.m_pucExtraData = pCdmxInfo->m_sAudioInfo.m_pExtraData;
			gsADecInit.m_iDownMixMode = 1;	//if(ch > 2) downmix to stereo
			guiAudioMinDataSize = 0;
			break;
#endif

#ifdef INCLUDE_FLAC_DEC
		/* FLAC Decoder */
		case AUDIO_ID_FLAC:
			gsADecInit.m_iDownMixMode = 1; //if(ch > 2) downmix to stereo
			gsADecInit.m_unAudioCodecParams.m_unFLAC.m_iFlacInitMode = 1;
			guiAudioMinDataSize = 0;
			break;
#endif

#ifdef INCLUDE_RAG2_DEC
		/* RAG2 (COOK) Decoder */
		case AUDIO_ID_COOK:
			gsADecInit.m_unAudioCodecParams.m_unRAG2.m_iRAG2NSamples	=	pCdmxInfo->m_sAudioInfo.m_iSamplesPerFrame;
			gsADecInit.m_unAudioCodecParams.m_unRAG2.m_iRAG2NChannels	=	pCdmxInfo->m_sAudioInfo.m_iChannels;
			gsADecInit.m_unAudioCodecParams.m_unRAG2.m_iRAG2NRegions	=	pCdmxInfo->m_sAudioInfo.m_snRegions;
			gsADecInit.m_unAudioCodecParams.m_unRAG2.m_iRAG2NFrameBits	=	pCdmxInfo->m_sAudioInfo.m_iframeSizeInBits;
			gsADecInit.m_unAudioCodecParams.m_unRAG2.m_iRAG2SampRate	=	pCdmxInfo->m_sAudioInfo.m_iSampleRate;
			gsADecInit.m_unAudioCodecParams.m_unRAG2.m_iRAG2CplStart	=	pCdmxInfo->m_sAudioInfo.m_scplStart;
			gsADecInit.m_unAudioCodecParams.m_unRAG2.m_iRAG2CplQbits	=	pCdmxInfo->m_sAudioInfo.m_scplQBits;
			guiAudioMinDataSize = 24;//when flavor of cook is "8". Refer to "RealAudio Depacketization and Deinterleaving Documentation"
			break;
#endif

#ifdef INCLUDE_APE_DEC
		/* APE Decoder */
		case AUDIO_ID_APE:
			guiAudioMinDataSize = AUDIO_MAX_INPUT_SIZE;
			if( ( pCdmxInfo->m_sAudioInfo.m_iMinDataSize != 0 ) && ( iCtype == CONTAINER_TYPE_AUDIO) )
			{
				guiAudioMinDataSize = pCdmxInfo->m_sAudioInfo.m_iMinDataSize;
			}

			break;
#endif

#ifdef INCLUDE_WAV_DEC
		/* WAV (PCM, ADPCM_MS, ADPCM_IMA, ALAW, MULAW) Decoder */
		case AUDIO_ID_WAV:
			gsADecInit.m_unAudioCodecParams.m_unWAV.m_iWAVForm_TagID  =  pCdmxInfo->m_sAudioInfo.m_iFormatId;
			gsADecInit.m_unAudioCodecParams.m_unWAV.m_uiNBlockAlign  =  pCdmxInfo->m_sAudioInfo.m_iBlockAlign;		//20091121_Helena

			if(pCdmxInfo->m_sAudioInfo.m_iFormatId == AV_AUDIO_MS_PCM)
			{
				//output sample is greater than 4
				guiAudioMinDataSize = pCdmxInfo->m_sAudioInfo.m_iChannels * pCdmxInfo->m_sAudioInfo.m_iBitsPerSample / 2;
			}

			break;
#endif

#ifdef INCLUDE_VORBIS_DEC
		/* VORBIS (OGG) Decoder */
		case AUDIO_ID_VORBIS:
		{
			// search for "vorbis" string
			int	run=0;
			char		*p = (char*)gsADecInit.m_pucExtraData;
			while(run<gsADecInit.m_iExtraDataLen)
			{
				if(	(p[0] == 0x1) &&
					(p[1] == 'v') &&
					(p[2] == 'o') &&
					(p[3] == 'r') &&
					(p[4] == 'b') &&
					(p[5] == 'i') &&
					(p[6] == 's') )
					{
						break;
					}
				run++;
				p++;

			}
			gsADecInit.m_iDownMixMode = 1;	//if(ch > 2) downmix to stereo
			gsADecInit.m_pucExtraData	=	gsADecInit.m_pucExtraData	+		run;
			gsADecInit.m_iExtraDataLen	=	gsADecInit.m_iExtraDataLen	-		run;
			gsADecInit.m_unAudioCodecParams.m_unVORBIS.m_pfCalloc	=	pCdk->m_psCallback->m_pfCalloc;
			break;
		}
#endif
//#ifdef INCLUDE_DDP_DEC
		case AUDIO_ID_DDP:
		{
			/* if all values are zero, set as typical values, in order that customer change values of m_unDDPDec. */
			#define unDDPDec gsADecInit.m_unAudioCodecParams.m_unDDPDec

			if ( unDDPDec.m_iDDPStreamBufNwords == 0
				&& unDDPDec.m_pcMixdataTextbuf == 0
				&& unDDPDec.m_iMixdataTextNsize == 0
				&& unDDPDec.m_iCompMode == 0
				&& unDDPDec.m_iOutLfe == 0
				&& unDDPDec.m_iOutChanConfig == 0
				&& unDDPDec.m_iPcmScale == 0
				&& unDDPDec.m_iStereMode == 0
				&& unDDPDec.m_iDualMode == 0
				&& unDDPDec.m_iDynScaleHigh == 0
				&& unDDPDec.m_iDynScaleLow == 0
				&& unDDPDec.m_iNumOutChannels == 0
				&& unDDPDec.m_imixdataout == 0
				)
			{
				unDDPDec.m_iDDPStreamBufNwords = guiAudioInBufferSize/2;
				unDDPDec.m_pcMixdataTextbuf = NULL;
				unDDPDec.m_iMixdataTextNsize = 0;
				unDDPDec.m_iCompMode = 2;
				unDDPDec.m_iOutLfe = 0;
				unDDPDec.m_iOutChanConfig = 2;
				unDDPDec.m_iPcmScale = 100;
				unDDPDec.m_iStereMode = 0;
				unDDPDec.m_iDualMode = 0;
				unDDPDec.m_iDynScaleHigh = 100;
				unDDPDec.m_iDynScaleLow = 100;
				unDDPDec.m_iNumOutChannels = 2;
				unDDPDec.m_imixdataout = 0;
			}
//			gsADecInput.m_pvExtraInfo = &gstStreaminfoExtra;
//			gsADecOutput.m_pvExtraInfo = &gstPcminfoExtra;

			break;
		}
//#endif

		default:
			break;

		}

		//=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=====*=END.

		ret = gspfADec( AUDIO_INIT, &pCdk->m_iAudioHandle, &gsADecInit, NULL);
		if( ret < 0 )
		{
			DPRINTF( TC_MAGENTA"[CDK_CORE:Err%d] gspfADec AUDIO_INIT failed \n"T_DEFAULT, ret );
			//gspfADec( AUDIO_CLOSE, &pCdk->m_iAudioHandle, NULL, NULL );
			cdk_adec_close( pCdk, gsADec);
			return ret;
		}
	}
	else
	{
		DPRINTF( TC_MAGENTA"[CDK_CORE] audio decoder not found (iAdecType: %d) \n"T_DEFAULT, iAdecType );
		return -1;
	}

	return ret;
}

cdk_result_t
cdk_adec_decode( cdk_core_t* pCdk, cdmx_info_t *pCdmxInfo, cdmx_output_t *pCdmxOut, int iAdecType, int iSeekFlag, int * alsa_status, ADEC_VARS* gsADec)
{
	int ret = CDK_ERR_NONE;

	pCdmxOut->m_iDecodedSamples = 0;

		//
		//	Audio decoding mode
		//

		if( iSeekFlag != 0 )
		{
			gsADecInput.m_iStreamLength = 0;
			gspfADec( AUDIO_FLUSH, &pCdk->m_iAudioHandle, NULL, NULL );
		}

		/* START : latm demuxer get rawdata -----------------------------------------*/
		if( ( gspLatmHandle != NULL ) && ( iAdecType == AUDIO_ID_AAC ) )
		{
			unsigned char* pAACRawData;
			int iAACRawDataSize;

			if( iSeekFlag != 0 )
			{
				ret = pCdk->pfLatmFlushBuffer ( gspLatmHandle );
				DPRINTF( TC_MAGENTA"[CDK_CORE] latm_parser_flush_buffer\n"T_DEFAULT);
				if( ret != 0 )
				{
					DPRINTF( TC_MAGENTA"[CDK_CORE:Err%d] latm_parser_flush_buffer: error!\n"T_DEFAULT, ret);
				}
			}

			ret = pCdk->pfLatmGetFrame( gspLatmHandle, pCdmxOut->m_pPacketData, pCdmxOut->m_iPacketSize, &pAACRawData, &iAACRawDataSize, 0 );
			if( ret < 0 )
			{
				DPRINTF( TC_MAGENTA"[CDK_CORE:Err%d] latm_parser_get_frame: Fatal error!\n"T_DEFAULT, ret);
				return ret;
			}

			if( iAACRawDataSize <= 0 )
			{
				DPRINTF( TC_MAGENTA"[CDK_CORE:Err%d] latm_parser_get_frame: Need more data!\n"T_DEFAULT, ret );
				return 1;	//need more data
			}

			pCdmxOut->m_pPacketData = pAACRawData;
			pCdmxOut->m_iPacketSize = iAACRawDataSize;
		}
		/* END : latm demuxer get rawdata -----------------------------------------*/

/*
		if (iAdecType == AUDIO_ID_APE)
		{
			gsADecInput.m_pvExtraInfo = (void *)pCdmxOut->m_uiUseCodecSpecific;
		}
*/
		if( gsADecInput.m_iStreamLength < 0 )
		{
			gsADecInput.m_iStreamLength = 0;
		}

		if( (gsADecInput.m_iStreamLength + pCdmxOut->m_iPacketSize) > (int)guiAudioInBufferSize)
		{
			unsigned char *pucTmpBuffer = NULL;

			DSTATUS( TC_MAGENTA"cdk_audio_dec_run : Allocated size of gpucAudioBuffer is over: allocated: %d, needed: %d\n"T_DEFAULT, guiAudioInBufferSize, gsADecInput.m_iStreamLength + pCdmxOut->m_iPacketSize);
			DSTATUS( TC_MAGENTA"cdk_audio_dec_run : Realloc gpucAudioBuffer\n"T_DEFAULT );

			guiAudioInBufferSize = gsADecInput.m_iStreamLength + pCdmxOut->m_iPacketSize;
			pucTmpBuffer = gsADecInit.m_pfMalloc(guiAudioInBufferSize);
			if( pucTmpBuffer == NULL )
			{
				DPRINTF( TC_MAGENTA"[CDK_CORE] audio DECODER gpucAudioBuffer allocation fail\n"T_DEFAULT );
				return -0x1001;
			}
			if( gpucAudioBuffer != NULL )
			{
				gsADecInit.m_pfMemcpy(pucTmpBuffer, gpucAudioBuffer, gsADecInput.m_iStreamLength);
				gsADecInit.m_pfFree(gpucAudioBuffer);
			}
			gpucAudioBuffer = pucTmpBuffer;
		}

		gsADecInit.m_pfMemcpy(gpucAudioBuffer + gsADecInput.m_iStreamLength, pCdmxOut->m_pPacketData, pCdmxOut->m_iPacketSize);
		gsADecInput.m_pcStream = (TCAS_S8*)gpucAudioBuffer;
		gsADecInput.m_iStreamLength += pCdmxOut->m_iPacketSize;

		do
		{
			// cdk decoder do not move buffer start position to the end of the last decoded data
			gsADecOutput.m_pvChannel[0] = (void *)(pCdk->m_pOutWav + gsADecOutput.m_uiNumberOfChannel * pCdmxOut->m_iDecodedSamples * sizeof(short) );

			ret = gspfADec( AUDIO_DECODE, &pCdk->m_iAudioHandle, &gsADecInput, &gsADecOutput );
			DSTATUS("cdk_adec decode ret %d", ret);

			if( ret < 0 && ret != TCAS_ERROR_CONCEALMENT_APPLIED )
			{
				giAudioErrorCount++;

				if( giAudioErrorCount > AUDIO_ERROR_THRESHOLD )
				{
					DSTATUS( TC_MAGENTA"[CDK_CORE] cdk audio decode: too many errors!\n"T_DEFAULT );
					return ret;
				}

				switch( ret )
				{
				case TCAS_ERROR_SKIP_FRAME:
				case TCAS_ERROR_CONCEALMENT_APPLIED:
				case TCAS_ERROR_DDP_NOT_SUPPORT_FRAME:
					giAudioErrorCount = 0;
					break;

				case TCAS_ERROR_MORE_DATA:			// need more data
				case TCAS_ERROR_INVALID_BUFSTATE:	// need more data

					if(pCdk->m_iAudioProcessMode != AUDIO_BROADCAST_MODE && pCdmxOut->m_iEndOfFile == 1)
					{
						break;
					}

					DSTATUS( TC_MAGENTA"[CDK_CORE] need more data, bytes left: %d\n"T_DEFAULT, gsADecInput.m_iStreamLength);

					if( gsADecInput.m_iStreamLength > 0 )
					{
						gsADecInit.m_pfMemmove(gpucAudioBuffer, gsADecInput.m_pcStream, gsADecInput.m_iStreamLength);
					}

					return 1;	//need more data

				default:
					DPRINTF( TC_MAGENTA"[CDK_CORE] cdk_audio_dec_run error: %d\n"T_DEFAULT , ret);

					if(pCdk->m_iAudioProcessMode == AUDIO_BROADCAST_MODE)
					{
						gsADecInput.m_iStreamLength = 0;

						gspfADec( AUDIO_FLUSH, &pCdk->m_iAudioHandle, NULL, NULL );

						if( ( gspLatmHandle != NULL ) && ( iAdecType == AUDIO_ID_AAC ) )
						{
							int latm_ret = pCdk->pfLatmFlushBuffer ( gspLatmHandle );
							DPRINTF( TC_MAGENTA"[CDK_CORE] latm_parser_flush_buffer\n"T_DEFAULT);
							if( latm_ret != 0 )
							{
								DPRINTF( TC_MAGENTA"[CDK_CORE:Err%d] latm_parser_flush_buffer: error!\n"T_DEFAULT, latm_ret);
							}
						}
						return ret;
					}

					if( iAdecType == AUDIO_ID_WMA )
					{
						gsADecInput.m_iStreamLength = 0;
					}

					if( gsADecInput.m_iStreamLength > 0 )
					{
						gsADecInit.m_pfMemmove(gpucAudioBuffer, gsADecInput.m_pcStream, gsADecInput.m_iStreamLength);
					}

					if(guiAudioContinueThroughErrors == 1)
					{
						return 2;	// keep going
					}

					return ret;
				}
			}

			if( ret == TCAS_SUCCESS || ret == TCAS_ERROR_CONCEALMENT_APPLIED )
			{
				if(	giAudioErrorCount != 0 )
				{
					giAudioErrorCount = 0;	//reset error count
				}

				pCdmxOut->m_iDecodedSamples += gsADecOutput.m_uiSamplesPerChannel;
				DSTATUS("m_iDecodedSamples %d, m_uiSamplesPerChannel %d", pCdmxOut->m_iDecodedSamples, gsADecOutput.m_uiSamplesPerChannel);
			}

		} while( ( gsADecInput.m_iStreamLength > (int)guiAudioMinDataSize ) || ( ( gsADecInput.m_iStreamLength > 0 ) && ( pCdmxOut->m_iEndOfFile == 1 ) ) );

		if( gsADecInput.m_iStreamLength > 0 )
			gsADecInit.m_pfMemmove(gpucAudioBuffer, gsADecInput.m_pcStream, gsADecInput.m_iStreamLength);

		pCdmxOut->m_iPacketSize = gsADecInput.m_iStreamLength;

		if( ( ret == TCAS_ERROR_DDP_NOT_SUPPORT_FRAME ) && ( guiAudioContinueThroughErrors == 1 ) )
			return 1; /* check ddp dependent frame */

		if( ( ret < 0 ) && ( guiAudioContinueThroughErrors == 1) )
			return 2;

		return ret;
}

cdk_result_t
cdk_adec_close( cdk_core_t* pCdk , ADEC_VARS* gsADec)
{
	cdk_result_t ret = CDK_ERR_NONE;

	/* START : latm demuxer close -----------------------------------------*/
	if( gspLatmHandle != NULL )
	{
		pCdk->pfLatmClose( gspLatmHandle );
		gspLatmHandle = NULL;
		if( pCdk->pfLatmDLLModule != NULL)
		{
			dlclose(pCdk->pfLatmDLLModule);
			pCdk->pfLatmDLLModule = NULL;
		}
	}
	/* END : latm demuxer close -----------------------------------------*/

	if(gspfADec)
	{
		if(pCdk->m_iAudioHandle!=0)
		{
			ret = gspfADec( AUDIO_CLOSE, &pCdk->m_iAudioHandle, NULL, NULL );
			pCdk->m_iAudioHandle = 0;
		}
	}

	if( gpucAudioBuffer )
	{
		pCdk->m_psCallback->m_pfFree(gpucAudioBuffer);
		gpucAudioBuffer = NULL;
	}

	if (pCdk->pfDLLModule != NULL)
	{
		dlclose(pCdk->pfDLLModule);
		pCdk->pfDLLModule = NULL;
	}

	return ret;
}
