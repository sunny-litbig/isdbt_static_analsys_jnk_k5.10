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
#define LOG_TAG	"DXB_INTERFACE_REMOTEPLAY"
#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_Types.h>

#include <utils/Log.h>
#include <user_debug_levels.h>
#include <omx_base_component.h>

#include "tcc_dxb_interface_type.h"
#include "tcc_dxb_interface_omxil.h"
#include "tcc_dxb_interface_remoteplay.h"

#ifdef      ENABLE_REMOTE_PLAYER

#ifdef      ENABLE_TCC_TRANSCODER
#include "mediastream.h"
static int giStartTransCoder=0;
void TSOuputCallback(unsigned char *pbuffer, long len, long pts, unsigned long estype)
{
//    ALOGD("%s:%d:[%d:%d][0x%X][0x%X][0x%X][0x%X]", __func__, __LINE__, len, estype, pbuffer[0],pbuffer[1],pbuffer[2],pbuffer[3]);
}
#endif


#include <interface_client.h>
using namespace android;

InterfaceClient *pClient = NULL;
DxB_ERR_CODE TCC_DxB_REMOTEPLAY_Open(void)
{
#ifdef      ENABLE_TCC_TRANSCODER
    TrscoderInfo TrsStream;
    TrsStream.bps = 1*1024*1024;
    TrsStream.fps = 25;
    TrsStream.keyframe_interval = 0;
    TrsStream.target_width = 640;
    TrsStream.target_height = 480;
    TCC_HLS_Trscoder_Init(&TrsStream);
    giStartTransCoder = 0;
    TCC_HLS_SetOutputCallback(TSOuputCallback);
	return DxB_ERR_OK;
#endif    
    if(pClient)
	    return DxB_ERR_OK;

    pClient = new InterfaceClient();
    if(pClient == NULL)
        return DxB_ERR_ERROR;
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_REMOTEPLAY_Close(void)
{
#ifdef      ENABLE_TCC_TRANSCODER
    TCC_HLS_Trscoder_Stop();    
	return DxB_ERR_OK;
#endif        
    if(pClient)
        delete pClient;

    pClient = NULL;
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_REMOTEPLAY_SetVideoInfo(unsigned int uiWidth, unsigned int uiHeight, unsigned int uiFrameRate)    
{
    DxB_ERR_CODE ret = DxB_ERR_OK;

#ifdef      ENABLE_TCC_TRANSCODER
    TCC_HLS_SendVideoInfo(uiWidth, uiHeight, uiFrameRate*1000);    
	return DxB_ERR_OK;
#endif            
    if(pClient)
        pClient->WriteVideoFrameInfo(uiWidth, uiHeight, uiFrameRate*1000);
    else
        ret = DxB_ERR_ERROR;  
	return ret;
}

DxB_ERR_CODE TCC_DxB_REMOTEPLAY_WriteVideo(unsigned char *pucData, unsigned int uiSize, unsigned int uiPTS, unsigned int uiPicType)
{
    DxB_ERR_CODE ret = DxB_ERR_OK;

#ifdef      ENABLE_TCC_TRANSCODER
    if(giStartTransCoder == 0)
    {
//        TCC_HLS_Trscoder_Start();
        giStartTransCoder = 1;
    }
    TCC_HLS_WriteVideoFrame(pucData, uiSize, uiPTS, uiPicType);    
	return DxB_ERR_OK;
#endif                
    if(pClient)
        pClient->WriteVideoFrame(pucData, uiSize, uiPTS, uiPicType, NULL);
    else
        ret = DxB_ERR_ERROR;  
	return ret;
}

DxB_ERR_CODE TCC_DxB_REMOTEPLAY_WriteAudio(unsigned char *pucData, unsigned int uiSize, unsigned int uiPTS)
{
    DxB_ERR_CODE ret = DxB_ERR_OK;

#ifdef      ENABLE_TCC_TRANSCODER
    if(giStartTransCoder == 0)
    {
//        TCC_HLS_Trscoder_Start();
        giStartTransCoder = 1;
    }    
    TCC_HLS_WriteAudioFrame(pucData, uiSize, uiPTS);    
	return DxB_ERR_OK;
#endif                    
    if(pClient)
        pClient->WriteAudioFrame(pucData, uiSize, uiPTS, NULL);
    else
        ret = DxB_ERR_ERROR;  
	return ret;
}

DxB_ERR_CODE TCC_DxB_REMOTEPLAY_SetAudioInfo(unsigned int uiSampleRate, unsigned int uiChannelCounts)
{
    DxB_ERR_CODE ret = DxB_ERR_OK;
#ifdef      ENABLE_TCC_TRANSCODER
    TCC_HLS_SendAudioInfo(uiSampleRate, uiChannelCounts);    
	return DxB_ERR_OK;
#endif                        
    if(pClient)
        pClient->WriteAudioInfo(uiSampleRate, uiChannelCounts);
    else
        ret = DxB_ERR_ERROR;  
	return ret;
}

DxB_ERR_CODE TCC_DxB_REMOTEPLAY_WriteSubtitle(void *pData, unsigned int uiFormat)
{
    DxB_ERR_CODE ret = DxB_ERR_OK;
    if(pClient)
        pClient->WriteSubtitleFrame(pData, uiFormat, NULL);
    else
        ret = DxB_ERR_ERROR;  
	return ret;
}

#endif
