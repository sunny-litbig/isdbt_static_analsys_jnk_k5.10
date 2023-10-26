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
#include "globals.h"
#include <fcntl.h>
#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_Types.h>
#include <OMX_Audio.h>
#include "OMX_Other.h"

#include <user_debug_levels.h>
#include <OMX_TCC_Index.h>

#include <omx_base_component.h>
#include <tcc_video_common.h>
#include <omx_alsasink_component.h>
#include <omx_fbdev_sink_component.h>

#include "tcc_dxb_interface_omxil.h"
#include "omx_videodec_component.h"
#include "tcc_dxb_interface_omx_events.h"
#include "tcc_dxb_interface_demux.h"
#include "tcc_dxb_interface_tuner.h"
#include "tcc_dxb_interface_type.h"

#define LOG_NDEBUG 0
#define LOG_TAG	"DxB-OMXIL"
#include <utils/Log.h>
#include <cutils/properties.h>

#ifdef      ENABLE_REMOTE_PLAYER
OMX_BOOL global_running = OMX_FALSE;
#include "tcc_dxb_interface_remoteplay.h"

void TCC_DxB_Set_Remote_Service_Status(OMX_BOOL bFlag)
{
	global_running = bFlag;
}

OMX_BOOL TCC_DxB_Get_Remote_Service_Status(void)
{
	return global_running;
}
#endif

extern OMX_ERRORTYPE  (*(pomx_isdbt_tuner_component_init[]))(OMX_COMPONENTTYPE *openmaxStandComp);
extern OMX_ERRORTYPE (*tcc_omx_isdbt_tuner_default (void))(OMX_COMPONENTTYPE *openaxStandComp);
extern int	tcc_omx_isdbt_tuner_count (void);

extern OMX_ERRORTYPE dxb_omx_dxb_linuxtv_demux_component_Init (OMX_COMPONENTTYPE * openmaxStandComp);

extern OMX_ERRORTYPE dxb_OMX_DXB_AudioDec_AACComponentInit(OMX_COMPONENTTYPE *openmaxStandComp);
extern OMX_ERRORTYPE dxb_OMX_DXB_AudioDec_AACLATM_ComponentInit(OMX_COMPONENTTYPE *openmaxStandComp);
extern OMX_ERRORTYPE dxb_OMX_DXB_AudioDec_AACCommonComponentInit(OMX_COMPONENTTYPE *openmaxStandComp);
extern OMX_ERRORTYPE dxb_OMX_DXB_AudioDec_Default_ComponentInit(OMX_COMPONENTTYPE *openmaxStandComp);
/******************************************************************
 * define
 *******************************************************************/

/******************************************************************
 * local
 *******************************************************************/
OpenMax_IL_Contents_t DXB_Contents[] =
{
    0,
    {
        {COMPONENTS_DXB_TUNER, NULL /*Not Yet define*/},
        {COMPONENTS_DXB_DEMUX, dxb_omx_dxb_linuxtv_demux_component_Init},
        {COMPONENTS_DXB_VIDEO, dxb_omx_videodec_default_component_Init},
        {COMPONENTS_DXB_FBDEV_SINK, dxb_omx_fbdev_sink_no_clockcomp_component_Init},
        {COMPONENTS_DXB_AUDIO, dxb_OMX_DXB_AudioDec_Default_ComponentInit},
        {COMPONENTS_DXB_ALSA_SINK, dxb_omx_alsasink_no_clockcomp_component_Init},
        {COMPONENTS_NULL, NULL}
    }
};

OpenMax_IL_Contents_t DAB_Contents[] =
{
    1,
    {
        {COMPONENTS_DXB_TUNER, NULL /*Not Yet define*/},
        {COMPONENTS_DXB_AUDIO, dxb_OMX_DXB_AudioDec_Default_ComponentInit},
        {COMPONENTS_DXB_ALSA_SINK, dxb_omx_alsasink_no_clockcomp_component_Init},
        {COMPONENTS_NULL, NULL}
    }
};

int tcc_omx_check_audio_type(int iAudioType)
{
    if( STREAMTYPE_AUDIO1!=iAudioType &&
            STREAMTYPE_AUDIO2!=iAudioType &&
            STREAMTYPE_AUDIO_AC3_1!=iAudioType &&
            STREAMTYPE_AUDIO_AC3_2!=iAudioType &&
            STREAMTYPE_AUDIO_AAC != iAudioType &&
            STREAMTYPE_AUDIO_AAC_LATM != iAudioType &&
            STREAMTYPE_AUDIO_BSAC != iAudioType )
    {
        DEBUG(DEB_LEV_ERR,"Can NOT Support Audio Type 0x%X\n", iAudioType);
        return -1;
    }

    return 0;
}

#if 0   // sunny : not use.
int tcc_omx_check_video_type(int iVideoType)
{
    if( STREAMTYPE_MPEG1_VIDEO!=iVideoType &&
            STREAMTYPE_VIDEO!=iVideoType &&
            STREAMTYPE_VIDEO_AVCH264!=iVideoType )
    {
        DEBUG(DEB_LEV_ERR,"Can NOT Support Video Type 0x%X\n", iVideoType);
        return -1;
    }

    return 0;
}
#endif

int tcc_omx_select_audiovideo_type(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv, int iAudioType, int iVideoType)
{

    int err = -1;
    OpenMax_IL_Components_t * Component;
    DEBUG(DEFAULT_MESSAGES,"In %s\n", __func__);

    err = TCC_DxB_DEMUX_SetESType(OMX_Dxb_Dec_AppPriv->pvParent, 0, iAudioType, iVideoType);
    if(err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR,"Can NOT Set audio/video Stream(%d)...!!!\n", err);
        return err;
    }

    if(iAudioType != -1)
    {
        Component = tcc_omx_get_components(tcc_omx_get_components_of_contents(OMX_Dxb_Dec_AppPriv),COMPONENTS_DXB_AUDIO);
        if(Component == NULL){
        	return -1;
        }

        if( iAudioType == STREAMTYPE_AUDIO_AAC )
        {
            if( Component->Init != dxb_OMX_DXB_AudioDec_AACComponentInit )
            {
                DEBUG(DEB_LEV_ERR,"Loanded Audio Comp is different. Try to change to AAC\n");
                tcc_omx_component_free(OMX_Dxb_Dec_AppPriv, COMP_AUDIO);
                Component->Init = dxb_OMX_DXB_AudioDec_Default_ComponentInit;//dxb_OMX_DXB_AudioDec_AACComponentInit;
                tcc_omx_load_component(OMX_Dxb_Dec_AppPriv, COMP_AUDIO);
            }
        }
		else if( iAudioType == STREAMTYPE_AUDIO_AAC_PLUS )
		{
            if( Component->Init != dxb_OMX_DXB_AudioDec_AACCommonComponentInit )
            {
                DEBUG(DEB_LEV_ERR,"Loanded Audio Comp is different. Try to change to AAC\n");
                tcc_omx_component_free(OMX_Dxb_Dec_AppPriv, COMP_AUDIO);
                Component->Init = dxb_OMX_DXB_AudioDec_Default_ComponentInit;//dxb_OMX_DXB_AudioDec_AACCommonComponentInit;
                tcc_omx_load_component(OMX_Dxb_Dec_AppPriv, COMP_AUDIO);
            }
		}
        else if( iAudioType == STREAMTYPE_AUDIO_AAC_LATM )
        {
            if( Component->Init != dxb_OMX_DXB_AudioDec_AACLATM_ComponentInit )
            {
                DEBUG(DEB_LEV_ERR,"Loanded Audio Comp is different. Try to change to AAC LATM\n");
                tcc_omx_component_free(OMX_Dxb_Dec_AppPriv, COMP_AUDIO);
                Component->Init = dxb_OMX_DXB_AudioDec_AACLATM_ComponentInit;
                tcc_omx_load_component(OMX_Dxb_Dec_AppPriv, COMP_AUDIO);
            }
        }
    }

    if(iVideoType != -1)
    {
        Component = tcc_omx_get_components(tcc_omx_get_components_of_contents(OMX_Dxb_Dec_AppPriv),COMPONENTS_DXB_VIDEO);
        if(Component == NULL){
        	return -1;
        }
        if( iVideoType == STREAMTYPE_VIDEO_AVCH264 )
        {
            if( Component->Init != dxb_omx_videodec_h264_component_Init )
            {
                DEBUG(DEB_LEV_ERR,"Loanded Video Comp is different. Try to change form MPEG2 to H264\n");
                tcc_omx_component_free(OMX_Dxb_Dec_AppPriv, COMP_VIDEO);
                Component->Init = dxb_omx_videodec_h264_component_Init;
                tcc_omx_load_component(OMX_Dxb_Dec_AppPriv, COMP_VIDEO);
            }
        }
        else
        {
            if( Component->Init != dxb_omx_videodec_mpeg2_component_Init )
            {
                DEBUG(DEB_LEV_ERR,"Loanded Video Comp is different. Try to change form H264 to MPEG2\n");
                tcc_omx_component_free(OMX_Dxb_Dec_AppPriv, COMP_VIDEO);
                Component->Init = dxb_omx_videodec_mpeg2_component_Init;
                tcc_omx_load_component(OMX_Dxb_Dec_AppPriv, COMP_VIDEO);
            }
        }
    }
    return 0;
}

int tcc_omx_setup_tunnel_linuxtv(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv, int adec_cnt, int vdec_cnt)
{
	int	err = -1;

	if (adec_cnt >= 1)
	{
		// Audio 0
		err = OMX_SetupTunnel(OMX_Dxb_Dec_AppPriv->demuxhandle, 0, OMX_Dxb_Dec_AppPriv->audiohandle, 0);
		if(err != OMX_ErrorNone) {
			DEBUG(DEB_LEV_ERR, "Set up Tunnel (demux-audio 0) Failed\n");
			return err;
		}

		err = OMX_SetupTunnel(OMX_Dxb_Dec_AppPriv->audiohandle, 2, OMX_Dxb_Dec_AppPriv->alsasinkhandle, 0);
		if(err != OMX_ErrorNone) {
			DEBUG(DEB_LEV_ERR, "Set up Tunnel (audio-sink 0) Failed\n");
	            return err;
	        }
	}

	if (vdec_cnt >= 1)
	{
		// Video 0
		err = OMX_SetupTunnel(OMX_Dxb_Dec_AppPriv->demuxhandle, 2, OMX_Dxb_Dec_AppPriv->videohandle, 0);
		if(err != OMX_ErrorNone) {
			DEBUG(DEB_LEV_ERR, "Set up Tunnel (demux-video 0) Failed\n");
			return err;
		}

		err = OMX_SetupTunnel(OMX_Dxb_Dec_AppPriv->videohandle, 2, OMX_Dxb_Dec_AppPriv->fbdevhandle, 0);
		if(err != OMX_ErrorNone) {
			DEBUG(DEB_LEV_ERR, "Set up Tunnel (video-sink 0) Failed\n");
			return err;
		}
	}

	if (adec_cnt >= 2)
	{
		// Audio 1
		err = OMX_SetupTunnel(OMX_Dxb_Dec_AppPriv->demuxhandle, 1, OMX_Dxb_Dec_AppPriv->audiohandle, 1);
		if(err != OMX_ErrorNone) {
			DEBUG(DEB_LEV_ERR, "Set up Tunnel (demux-audio 1) Failed\n");
			return err;
		}

		err = OMX_SetupTunnel(OMX_Dxb_Dec_AppPriv->audiohandle, 3, OMX_Dxb_Dec_AppPriv->alsasinkhandle, 1);
		if(err != OMX_ErrorNone) {
			DEBUG(DEB_LEV_ERR, "Set up Tunnel (audio-sink 1) Failed\n");
			return err;
		}
	}

	if (vdec_cnt >= 2)
	{
		// Video 1
		err = OMX_SetupTunnel(OMX_Dxb_Dec_AppPriv->demuxhandle, 3, OMX_Dxb_Dec_AppPriv->videohandle, 1);
		if(err != OMX_ErrorNone) {
			DEBUG(DEB_LEV_ERR, "Set up Tunnel (demux-video 1) Failed\n");
			return err;
		}

		err = OMX_SetupTunnel(OMX_Dxb_Dec_AppPriv->videohandle, 3, OMX_Dxb_Dec_AppPriv->fbdevhandle, 1);
		if(err != OMX_ErrorNone) {
			DEBUG(DEB_LEV_ERR, "Set up Tunnel (video-sink 1) Failed\n");
			return err;
		}
	}

	return err;
}

static int tcc_omx_setup_tunnel_isdbt(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv)
{
	return tcc_omx_setup_tunnel_linuxtv(OMX_Dxb_Dec_AppPriv, 2, 2);
}

int tcc_omx_setup_tunnel(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv)
{
	int err = -1;
	DxB_STANDARDS	dxb_standard = OMX_Dxb_Dec_AppPriv->iDxBStandard;

	DEBUG(DEFAULT_MESSAGES,"In %s\n", __func__);

	if (dxb_standard == DxB_STANDARD_ISDBT)    err = tcc_omx_setup_tunnel_isdbt(OMX_Dxb_Dec_AppPriv);
	else DEBUG(DEB_LEV_ERR, "In %s, undefined standard (%d) is specified\n", __func__, dxb_standard);

	tcc_omx_set_state(OMX_Dxb_Dec_AppPriv, TCC_OMX_TUNNEL);

	return err;
}

/**************************************************************************
 *  FUNCTION NAME :
 *	int tcc_omx_statechange_from_loading_to_idle(void)
 *
 *  DESCRIPTION :
 *  OUTPUT:
 **************************************************************************/
int tcc_omx_statechange_from_loading_to_idle(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv)
{
    int err = OMX_ErrorNone;

    unsigned int screen_mode;
    unsigned int SetVal = 2;
    int color_format;
	DxB_STANDARDS dxb_standard = OMX_Dxb_Dec_AppPriv->iDxBStandard;

    DEBUG(DEFAULT_MESSAGES,"In %s\n", __func__);

    if(OMX_Dxb_Dec_AppPriv->fbdevhandle)
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
    if(err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR,"dxbfbdevhandle idle error\n");
        return err;
    }

    if(OMX_Dxb_Dec_AppPriv->videohandle)
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->videohandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
    if(err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR,"dxbvideohandle idle error\n");
        return err;
    }

    if(OMX_Dxb_Dec_AppPriv->alsasinkhandle)
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->alsasinkhandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
    if(err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR,"dxbalsasinkhandle idle error\n");
        return err;
    }

    if(OMX_Dxb_Dec_AppPriv->audiohandle)
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->audiohandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
    if(err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR,"dxbaudiohandle idle error\n");
        return err;
    }

    if(OMX_Dxb_Dec_AppPriv->demuxhandle)
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
    if(err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR,"dxbdemuxhandle idle error\n");
        return err;
    }

    if(OMX_Dxb_Dec_AppPriv->demuxhandle)
        tsem_down(OMX_Dxb_Dec_AppPriv->demuxEventSem);
    if(OMX_Dxb_Dec_AppPriv->videohandle)
        tsem_down(OMX_Dxb_Dec_AppPriv->videoEventSem);
    if(OMX_Dxb_Dec_AppPriv->fbdevhandle)
        tsem_down(OMX_Dxb_Dec_AppPriv->fbdevEventSem);
    if(OMX_Dxb_Dec_AppPriv->audiohandle)
        tsem_down(OMX_Dxb_Dec_AppPriv->audioEventSem);
    if(OMX_Dxb_Dec_AppPriv->alsasinkhandle)
        tsem_down(OMX_Dxb_Dec_AppPriv->alsasinkEventSem);
    tcc_omx_set_state(OMX_Dxb_Dec_AppPriv, TCC_OMX_IDLE);

    return err;
}

/**************************************************************************
 *  FUNCTION NAME :
 *	int tcc_omx_statechange_from_idle_to_execute(void)
 *
 *  DESCRIPTION :
 *  OUTPUT:
 **************************************************************************/
int tcc_omx_statechange_from_idle_to_execute(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv)
{
    unsigned int err = OMX_ErrorNone;
	DxB_STANDARDS dxb_standard = OMX_Dxb_Dec_AppPriv->iDxBStandard;
    //OMX_TIME_CONFIG_CLOCKSTATETYPE		sClockState;
    DEBUG(DEFAULT_MESSAGES,"In %s\n", __func__);

    if(OMX_Dxb_Dec_AppPriv->demuxhandle)
    {
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
        if(err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,"dxbdemuxhandle state executing failed\n");
            return err;
        }
        tsem_down(OMX_Dxb_Dec_AppPriv->demuxEventSem);
    }

    if(OMX_Dxb_Dec_AppPriv->videohandle)
    {
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->videohandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
        if(err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,"dxbvideohandle state executing failed\n");
            return err;
        }
        tsem_down(OMX_Dxb_Dec_AppPriv->videoEventSem);
    }

    if(OMX_Dxb_Dec_AppPriv->fbdevhandle)
    {
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
        if(err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,"dxbfbdevhandle state executing failed\n");
            return err;
        }
        tsem_down(OMX_Dxb_Dec_AppPriv->fbdevEventSem);
    }

    if(OMX_Dxb_Dec_AppPriv->audiohandle)
    {
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->audiohandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
        if(err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,"dxbaudiohandle state executing failed\n");
            return err;
        }
        tsem_down(OMX_Dxb_Dec_AppPriv->audioEventSem);
    }

    if(OMX_Dxb_Dec_AppPriv->alsasinkhandle)
    {
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->alsasinkhandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
        if(err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,"dxbalsasinkhandle state executing failed\n");
            return err;
        }
        tsem_down(OMX_Dxb_Dec_AppPriv->alsasinkEventSem);
    }

    tcc_omx_set_state(OMX_Dxb_Dec_AppPriv, TCC_OMX_EXECUTING);

    return err;
}

/**************************************************************************
 *  FUNCTION NAME :
 *	int tcc_omx_statechange_from_execute_to_idle(void)
 *
 *  DESCRIPTION :
 *  OUTPUT:
 **************************************************************************/
int tcc_omx_statechange_from_execute_to_idle(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv)
{
    int err = OMX_ErrorNone;
	DxB_STANDARDS dxb_standard = OMX_Dxb_Dec_AppPriv->iDxBStandard;

    DEBUG(DEFAULT_MESSAGES,"In %s\n", __func__);

    if(OMX_Dxb_Dec_AppPriv->demuxhandle)
    {
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
        if(err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,"dxbdemuxhandle state executing failed\n");
            return err;
        }
        tsem_down(OMX_Dxb_Dec_AppPriv->demuxEventSem);
    }

    if(OMX_Dxb_Dec_AppPriv->videohandle)
    {
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->videohandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
        if(err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,"dxbvideohandle state executing failed\n");
            return err;
        }
        tsem_down(OMX_Dxb_Dec_AppPriv->videoEventSem);
    }

    if(OMX_Dxb_Dec_AppPriv->audiohandle)
    {
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->audiohandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
        if(err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,"dxbaudiohandle state executing failed\n");
            return err;
        }
        tsem_down(OMX_Dxb_Dec_AppPriv->audioEventSem);
    }

    if(OMX_Dxb_Dec_AppPriv->fbdevhandle)
    {
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
        if(err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,"dxbfbdevhandle state executing failed\n");
            return err;
        }
        tsem_down(OMX_Dxb_Dec_AppPriv->fbdevEventSem);
    }

    if(OMX_Dxb_Dec_AppPriv->alsasinkhandle)
    {
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->alsasinkhandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
        if(err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,"dxbalsasinkhandle state executing failed\n");
            return err;
        }
        tsem_down(OMX_Dxb_Dec_AppPriv->alsasinkEventSem);
    }

    usleep(5000);
    tcc_omx_set_state(OMX_Dxb_Dec_AppPriv, TCC_OMX_IDLE);

    return err;
}

#if 0   // sunny : not use.
/**************************************************************************
 *  FUNCTION NAME :
 *	int tcc_omx_statechange_from_execute_to_idle(void)
 *
 *  DESCRIPTION :
 *  OUTPUT:
 **************************************************************************/
static int tcc_omx_statechange_from_execute_to_pause(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv)
{
    int err = -1;
	DxB_STANDARDS dxb_standard = OMX_Dxb_Dec_AppPriv->iDxBStandard;

    if(OMX_Dxb_Dec_AppPriv->demuxhandle)
    {
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_CommandStateSet, OMX_StatePause, NULL);
        if(err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,"dxbdemuxhandle state executing failed\n");
            return err;
        }
        tsem_down(OMX_Dxb_Dec_AppPriv->demuxEventSem);
    }

    if(OMX_Dxb_Dec_AppPriv->videohandle)
    {
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->videohandle, OMX_CommandStateSet, OMX_StatePause, NULL);
        if(err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,"dxbvideohandle state executing failed\n");
            return err;
        }
        tsem_down(OMX_Dxb_Dec_AppPriv->videoEventSem);
    }

    if(OMX_Dxb_Dec_AppPriv->fbdevhandle)
    {
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_CommandStateSet, OMX_StatePause, NULL);
        if(err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,"dxbfbdevhandle state executing failed\n");
            return err;
        }
        tsem_down(OMX_Dxb_Dec_AppPriv->fbdevEventSem);
    }

    if(OMX_Dxb_Dec_AppPriv->audiohandle)
    {
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->audiohandle, OMX_CommandStateSet, OMX_StatePause, NULL);
        if(err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,"dxbaudiohandle state executing failed\n");
            return err;
        }
        tsem_down(OMX_Dxb_Dec_AppPriv->audioEventSem);
    }

    if(OMX_Dxb_Dec_AppPriv->alsasinkhandle)
    {
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->alsasinkhandle, OMX_CommandStateSet, OMX_StatePause, NULL);
        if(err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,"dxbalsasinkhandle state executing failed\n");
            return err;
        }
        tsem_down(OMX_Dxb_Dec_AppPriv->alsasinkEventSem);
    }

    tcc_omx_set_state(OMX_Dxb_Dec_AppPriv, TCC_OMX_PAUSE);
    return err;
}

/**************************************************************************
 *  FUNCTION NAME :
 *	int tcc_omx_statechange_from_execute_to_idle(void)
 *
 *  DESCRIPTION :
 *  OUTPUT:
 **************************************************************************/
int tcc_omx_statechange_from_pause_to_execute(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv)
{
    int err = -1;
	DxB_STANDARDS dxb_standard = OMX_Dxb_Dec_AppPriv->iDxBStandard;

    if(OMX_Dxb_Dec_AppPriv->demuxhandle)
    {
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
        if(err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,"dxbdemuxhandle state executing failed\n");
            return err;
        }
        tsem_down(OMX_Dxb_Dec_AppPriv->demuxEventSem);
    }

    if(OMX_Dxb_Dec_AppPriv->videohandle)
    {
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->videohandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
        if(err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,"dxbvideohandle state executing failed\n");
            return err;
        }
        tsem_down(OMX_Dxb_Dec_AppPriv->videoEventSem);
    }

    if(OMX_Dxb_Dec_AppPriv->fbdevhandle)
    {
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
        if(err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,"dxbfbdevhandle state executing failed\n");
            return err;
        }
        tsem_down(OMX_Dxb_Dec_AppPriv->fbdevEventSem);
    }

    if(OMX_Dxb_Dec_AppPriv->audiohandle)
    {
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->audiohandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
        if(err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,"dxbaudiohandle state executing failed\n");
            return err;
        }
        tsem_down(OMX_Dxb_Dec_AppPriv->audioEventSem);
    }

    if(OMX_Dxb_Dec_AppPriv->alsasinkhandle)
    {
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->alsasinkhandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
        if(err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,"dxbalsasinkhandle state executing failed\n");
            return err;
        }
        tsem_down(OMX_Dxb_Dec_AppPriv->alsasinkEventSem);
    }

    tcc_omx_set_state(OMX_Dxb_Dec_AppPriv, TCC_OMX_EXECUTING);

    return err;
}
#endif


/**************************************************************************
 *  FUNCTION NAME :
 *	int tcc_omx_statechange_from_idle_to_load(void)
 *
 *  DESCRIPTION :
 *  OUTPUT:
 **************************************************************************/
int tcc_omx_statechange_from_idle_to_load(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv)
{
    int err = -1;
	DxB_STANDARDS dxb_standard = OMX_Dxb_Dec_AppPriv->iDxBStandard;

    DEBUG(DEFAULT_MESSAGES,"In %s\n", __func__);

    if(OMX_Dxb_Dec_AppPriv->demuxhandle)
    {
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->demuxhandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
        if(err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,"dxbdemuxhandle state loaded failed\n");
            return err;
        }
        tsem_down(OMX_Dxb_Dec_AppPriv->demuxEventSem);
    }

    if(OMX_Dxb_Dec_AppPriv->videohandle)
    {
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->videohandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
        if(err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,"dxbvideohandle state loaded failed\n");
            return err;
        }
        tsem_down(OMX_Dxb_Dec_AppPriv->videoEventSem);
    }

    if(OMX_Dxb_Dec_AppPriv->fbdevhandle)
    {
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
        if(err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,"dxbfbdevhandle state loaded failed\n");
            return err;
        }
        tsem_down(OMX_Dxb_Dec_AppPriv->fbdevEventSem);
    }

    if(OMX_Dxb_Dec_AppPriv->audiohandle)
    {
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->audiohandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
        if(err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,"dxbaudiohandle state loaded failed\n");
            return err;
        }
        tsem_down(OMX_Dxb_Dec_AppPriv->audioEventSem);
    }

    if(OMX_Dxb_Dec_AppPriv->alsasinkhandle)
    {
        err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->alsasinkhandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
        if(err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,"dxbalsasinkhandle state loaded failed\n");
            return err;
        }
        tsem_down(OMX_Dxb_Dec_AppPriv->alsasinkEventSem);
    }

    tcc_omx_set_state(OMX_Dxb_Dec_AppPriv, TCC_OMX_LOADED);

    return err;
}

/**************************************************************************
 *  FUNCTION NAME :
 *	int tcc_omx_component_free(E_COMP_MASK eMask)
 *
 *  DESCRIPTION :
 *  OUTPUT:
 **************************************************************************/
int tcc_omx_component_free(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv, E_COMP_MASK eMask)
{
    int	err = OMX_ErrorNone;
    DEBUG(DEFAULT_MESSAGES,"In %s mask 0x%x\n", __func__, eMask);
    /** freeing all handles and deinit omx */
    if( eMask & COMP_TUNER )
    {
        if(OMX_Dxb_Dec_AppPriv->tunerhandle != NULL)
        {
            err = tcc_omx_free_handle(OMX_Dxb_Dec_AppPriv->tunerhandle);
            OMX_Dxb_Dec_AppPriv->tunerhandle = NULL;
            if(err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR,"dxbtunerhandle state loaded  failed\n");
                return err;
            }
            DEBUG(DEFAULT_MESSAGES,"In %s -freed tuner\n", __func__);
        }
    }

    if( eMask & COMP_DEMUX )
    {
        if(OMX_Dxb_Dec_AppPriv->demuxhandle != NULL)
        {
            err = tcc_omx_free_handle(OMX_Dxb_Dec_AppPriv->demuxhandle);
            OMX_Dxb_Dec_AppPriv->demuxhandle = NULL;
            if(err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR,"dxbdemuxhandle state loaded  failed\n");
                return err;
            }
            DEBUG(DEFAULT_MESSAGES,"In %s -freed demux\n", __func__);
        }
    }

    if( eMask & COMP_VIDEO )
    {
        if(OMX_Dxb_Dec_AppPriv->videohandle != NULL)
        {
            err = tcc_omx_free_handle(OMX_Dxb_Dec_AppPriv->videohandle);
            OMX_Dxb_Dec_AppPriv->videohandle = NULL;
            if(err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR,"dxbvideohandle state loaded  failed\n");
                return err;
            }
            DEBUG(DEFAULT_MESSAGES,"In %s -freed video\n", __func__);
        }
    }

    if( eMask & COMP_FBDEV_SINK )
    {
        if(OMX_Dxb_Dec_AppPriv->fbdevhandle != NULL)
        {
            err = tcc_omx_free_handle(OMX_Dxb_Dec_AppPriv->fbdevhandle);
            OMX_Dxb_Dec_AppPriv->fbdevhandle = NULL;
            if(err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR,"dxbfbdevhandle state loaded  failed\n");
                return err;
            }
            DEBUG(DEFAULT_MESSAGES,"In %s -freed fbdev\n", __func__);
        }
    }

    if( eMask & COMP_AUDIO )
    {
        if(OMX_Dxb_Dec_AppPriv->audiohandle != NULL)
        {
            err = tcc_omx_free_handle(OMX_Dxb_Dec_AppPriv->audiohandle);
            OMX_Dxb_Dec_AppPriv->audiohandle = NULL;
            if(err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR,"dxbaudiohandle state loaded  failed\n");
                return err;
            }
            DEBUG(DEFAULT_MESSAGES,"In %s -freed audio\n", __func__);
        }
    }

    if( eMask & COMP_ALSA_SINK )
    {
        if(OMX_Dxb_Dec_AppPriv->alsasinkhandle != NULL)
        {
            err = tcc_omx_free_handle(OMX_Dxb_Dec_AppPriv->alsasinkhandle);
            OMX_Dxb_Dec_AppPriv->alsasinkhandle = NULL;
            if(err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR,"dxbalsasinkhandle state loaded  failed\n");
                return err;
            }
            DEBUG(DEFAULT_MESSAGES,"In %s -freed alsa\n", __func__);
        }
    }

    return err;
}


/**************************************************************************
 *  FUNCTION NAME :
 *      tcc_omx_init()
 *
 *  DESCRIPTION :
 *  INPUT:
 *
 *  OUTPUT:	void - Return Type
 *  REMARK  :
 **************************************************************************/
Omx_Dxb_Dec_App_Private_Type* tcc_omx_init(void *pvParent)
{
	Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv = TCC_fo_malloc(__func__, __LINE__,sizeof(Omx_Dxb_Dec_App_Private_Type));
	if(OMX_Dxb_Dec_AppPriv == NULL){
		return OMX_Dxb_Dec_AppPriv;
	}
	memcpy(&OMX_Dxb_Dec_AppPriv->tDxBContents, &DXB_Contents, sizeof(OpenMax_IL_Contents_t));

    OMX_Dxb_Dec_AppPriv->tunerEventSem	 = TCC_fo_malloc(__func__, __LINE__,sizeof(tsem_t));
    OMX_Dxb_Dec_AppPriv->demuxEventSem	 = TCC_fo_malloc(__func__, __LINE__,sizeof(tsem_t));
    OMX_Dxb_Dec_AppPriv->videoEventSem	 = TCC_fo_malloc(__func__, __LINE__,sizeof(tsem_t));
    OMX_Dxb_Dec_AppPriv->fbdevEventSem	 = TCC_fo_malloc(__func__, __LINE__,sizeof(tsem_t));
    OMX_Dxb_Dec_AppPriv->audioEventSem	 = TCC_fo_malloc(__func__, __LINE__,sizeof(tsem_t));
    OMX_Dxb_Dec_AppPriv->alsasinkEventSem	 = TCC_fo_malloc(__func__, __LINE__,sizeof(tsem_t));

    tsem_init(OMX_Dxb_Dec_AppPriv->tunerEventSem	, 0);
    tsem_init(OMX_Dxb_Dec_AppPriv->demuxEventSem	, 0);
    tsem_init(OMX_Dxb_Dec_AppPriv->videoEventSem	, 0);
    tsem_init(OMX_Dxb_Dec_AppPriv->fbdevEventSem	, 0);
    tsem_init(OMX_Dxb_Dec_AppPriv->audioEventSem	, 0);
    tsem_init(OMX_Dxb_Dec_AppPriv->alsasinkEventSem	, 0);

    OMX_Dxb_Dec_AppPriv->tunerhandle = NULL;
    OMX_Dxb_Dec_AppPriv->demuxhandle = NULL;
    OMX_Dxb_Dec_AppPriv->videohandle = NULL;
    OMX_Dxb_Dec_AppPriv->fbdevhandle = NULL;
    OMX_Dxb_Dec_AppPriv->audiohandle = NULL;
    OMX_Dxb_Dec_AppPriv->alsasinkhandle = NULL;

    OMX_Dxb_Dec_AppPriv->state_type =0;
    OMX_Dxb_Dec_AppPriv->current_time =0;
	OMX_Dxb_Dec_AppPriv->iContentsType = 0; // 0:Main contents, 1:Sub contents(DAB)
	OMX_Dxb_Dec_AppPriv->pvParent = pvParent;
	OMX_Dxb_Dec_AppPriv->iDxBStandard = 0;

#ifdef      ENABLE_REMOTE_PLAYER
{
    char value[PROPERTY_VALUE_MAX];
	unsigned int uiLen = 0, val = -1;
	static int current = -1;
	uiLen = property_get ("init.svc.tcc_r_service", value, "");
	if(uiLen)
	{
		if(!strcmp(value, "running"))
		{
			TCC_DxB_Set_Remote_Service_Status(OMX_TRUE);
		}
		else
		{
			TCC_DxB_Set_Remote_Service_Status(OMX_FALSE);
		}
	}
}

#ifdef      ENABLE_TCC_TRANSCODER
    //make enable forcelly. because it doesn't need remote service
    TCC_DxB_Set_Remote_Service_Status(OMX_TRUE);
#endif

	if(TCC_DxB_Get_Remote_Service_Status() == OMX_TRUE)
	    TCC_DxB_REMOTEPLAY_Open();
#endif
	return OMX_Dxb_Dec_AppPriv;
}

/**************************************************************************
 *  FUNCTION NAME :
 *      void tcc_omx_deinit(void)
 *
 *  DESCRIPTION :
 *  INPUT:
 *
 *  OUTPUT:	void - Return Type
 *  REMARK  :
 **************************************************************************/
void tcc_omx_deinit(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv)
{
    if(OMX_Dxb_Dec_AppPriv->tunerEventSem != NULL)
        TCC_fo_free (__func__, __LINE__,OMX_Dxb_Dec_AppPriv->tunerEventSem);
    OMX_Dxb_Dec_AppPriv->tunerEventSem = NULL;

    if(OMX_Dxb_Dec_AppPriv->demuxEventSem != NULL)
        TCC_fo_free (__func__, __LINE__,OMX_Dxb_Dec_AppPriv->demuxEventSem);
    OMX_Dxb_Dec_AppPriv->demuxEventSem = NULL;

    if(OMX_Dxb_Dec_AppPriv->videoEventSem != NULL)
        TCC_fo_free (__func__, __LINE__,OMX_Dxb_Dec_AppPriv->videoEventSem);
    OMX_Dxb_Dec_AppPriv->videoEventSem = NULL;

    if(OMX_Dxb_Dec_AppPriv->fbdevEventSem != NULL)
        TCC_fo_free (__func__, __LINE__,OMX_Dxb_Dec_AppPriv->fbdevEventSem);
    OMX_Dxb_Dec_AppPriv->fbdevEventSem = NULL;

    if(OMX_Dxb_Dec_AppPriv->audioEventSem != NULL)
        TCC_fo_free (__func__, __LINE__,OMX_Dxb_Dec_AppPriv->audioEventSem);
    OMX_Dxb_Dec_AppPriv->audioEventSem = NULL;

    if(OMX_Dxb_Dec_AppPriv->alsasinkEventSem != NULL)
        TCC_fo_free (__func__, __LINE__,OMX_Dxb_Dec_AppPriv->alsasinkEventSem);
    OMX_Dxb_Dec_AppPriv->alsasinkEventSem = NULL;

    if(OMX_Dxb_Dec_AppPriv != NULL)
        TCC_fo_free (__func__, __LINE__,OMX_Dxb_Dec_AppPriv);
    OMX_Dxb_Dec_AppPriv = NULL;
#ifdef      ENABLE_REMOTE_PLAYER
	if(TCC_DxB_Get_Remote_Service_Status() == OMX_TRUE)
	    TCC_DxB_REMOTEPLAY_Close();
#endif
}



int tcc_omx_execute(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv)
{
    int err;
    DEBUG(DEFAULT_MESSAGES,"In %s\n", __func__);
    if(tcc_omx_get_state(OMX_Dxb_Dec_AppPriv) == TCC_OMX_EXECUTING)
        return 1;
    err = tcc_omx_setup_tunnel(OMX_Dxb_Dec_AppPriv);
    err = tcc_omx_statechange_from_loading_to_idle(OMX_Dxb_Dec_AppPriv);
    err = tcc_omx_statechange_from_idle_to_execute(OMX_Dxb_Dec_AppPriv);
    return 0;
}

int tcc_omx_stop(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv)
{
    DEBUG(DEFAULT_MESSAGES,"In %s\n", __func__);
    if(	tcc_omx_get_state(OMX_Dxb_Dec_AppPriv) == TCC_OMX_LOADED)
        return 1;

    tcc_omx_statechange_from_execute_to_idle(OMX_Dxb_Dec_AppPriv);
    tcc_omx_statechange_from_idle_to_load(OMX_Dxb_Dec_AppPriv);
    tcc_omx_component_free(OMX_Dxb_Dec_AppPriv, COMP_ALL & (~COMP_TUNER));
    tcc_omx_set_state(OMX_Dxb_Dec_AppPriv, TCC_OMX_NULL);
    return 0;
}

#if 0   // sunny : not use.
int tcc_omx_unload(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv)
{
    DEBUG(DEFAULT_MESSAGES,"In %s\n", __func__);
    tcc_omx_statechange_from_execute_to_idle(OMX_Dxb_Dec_AppPriv);
    tcc_omx_statechange_from_idle_to_load(OMX_Dxb_Dec_AppPriv);
    tcc_omx_component_free(OMX_Dxb_Dec_AppPriv, COMP_ALL);
	tcc_omx_set_state(OMX_Dxb_Dec_AppPriv, TCC_OMX_NULL);
	tcc_omx_deinit(OMX_Dxb_Dec_AppPriv);
    return 0;
}
#endif

int tcc_omx_clear_port_buffer(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv)
{
/*
    int err;
	if(OMX_Dxb_Dec_AppPriv->alsasinkhandle)
	{
		err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->alsasinkhandle, OMX_CommandFlush, OMX_ALL, NULL);
	}
    if(err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR,"%s:%d:failed..\n",__func__,__LINE__);
    }
	if(OMX_Dxb_Dec_AppPriv->fbdevhandle)
	{
	    err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->fbdevhandle, OMX_CommandFlush, OMX_ALL, NULL);
	}
    if(err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR,"%s:%d:failed..\n",__func__,__LINE__);
    }

	if(OMX_Dxb_Dec_AppPriv->videohandle)
	{
		err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->videohandle, OMX_CommandFlush, OMX_ALL, NULL);
	}
    if(err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR,"%s:%d:failed..\n",__func__,__LINE__);
    }
	if(OMX_Dxb_Dec_AppPriv->audiohandle)
	{
	    err = OMX_SendCommand(OMX_Dxb_Dec_AppPriv->audiohandle, OMX_CommandFlush, OMX_ALL, NULL);
	}
    if(err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR,"%s:%d:failed..\n",__func__,__LINE__);
    }
    return err;
*/
    return 0;
}

#if 0   // sunny : not use.
int tcc_omx_get_current_time(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv)
{
    return(OMX_Dxb_Dec_AppPriv->current_time);
}
#endif


void tcc_omx_set_state(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv, int type)
{
    OMX_Dxb_Dec_AppPriv->state_type=type;
}

/**************************************************************************
 *  FUNCTION NAME :
 *      tcc_omx_get_state()
 *
 *  DESCRIPTION :
 *  INPUT:
 *
 *  OUTPUT:	void - Return Type
 *  REMARK  :
 **************************************************************************/
int tcc_omx_get_state(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv)
{
    return OMX_Dxb_Dec_AppPriv->state_type;
}

#if 0   // sunny : not use.
/**************************************************************************
 *  FUNCTION NAME :
 *	 int tcc_omx_port_enable(OMX_HANDLETYPE handle,int port_id)
 *
 *  DESCRIPTION :
 *  OUTPUT:
 **************************************************************************/
int tcc_omx_port_enable(OMX_HANDLETYPE handle,int port_id)
{
    int err;

    err = OMX_SendCommand(handle, OMX_CommandPortEnable, port_id, NULL);

    if(err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR,"port enable failed\n");
    }
    return err;
}
#endif

/**************************************************************************
 *  FUNCTION NAME :
 *	 int tcc_omx_port_disable(OMX_HANDLETYPE handle,int port_id)
 *
 *  DESCRIPTION :
 *  OUTPUT:
 **************************************************************************/
int tcc_omx_port_disable(OMX_HANDLETYPE handle,int port_id)
{
    int err;

    err = OMX_SendCommand(handle, OMX_CommandPortDisable, port_id, NULL);

    if(err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR,"port disable failed\n");
    }
    return err;
}

/**************************************************************************
 *  FUNCTION NAME :
 *      OpenMax_IL_Contents_t*	tcc_omx_get_components_of_contents(void)
 *
 *  DESCRIPTION :
 *  INPUT:
 *
 *  OUTPUT:	void - Return Type
 *  REMARK  :
 **************************************************************************/
OpenMax_IL_Contents_t*	tcc_omx_get_components_of_contents(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv)
{
    OpenMax_IL_Contents_t * Contents;
	DxB_STANDARDS dxb_standard = OMX_Dxb_Dec_AppPriv->iDxBStandard;

    Contents = &OMX_Dxb_Dec_AppPriv->tDxBContents;
    return( Contents );
}

/**************************************************************************
 *  FUNCTION NAME :
 *      tcc_omx_get_components()
 *
 *  DESCRIPTION :
 *  INPUT:
 *
 *  OUTPUT:	void - Return Type
 *  REMARK  :
 **************************************************************************/
OpenMax_IL_Components_t * tcc_omx_get_components(OpenMax_IL_Contents_t * Contents,int type)
{
    DEBUG(DEB_LEV_FUNCTION_NAME, "In %s\n", __func__);
    int i;

    OpenMax_IL_Components_t * Component;
    for( i = 0 ; i < TCC_DXB_DEC_OMX_COMPONENT_NUM ; i ++ )
    {
        if( Contents->Components[i].Type != COMPONENTS_NULL )
        {
            if( Contents->Components[i].Type == (unsigned int)type )
            {
                Component=&Contents->Components[i];
                return( Component );
            }
        }
        else
        {
            return( NULL );
        }
    }
    return( NULL );
}


/**************************************************************************
 *  FUNCTION NAME :
 *      tcc_omx_get_handle()
 *
 *  DESCRIPTION :
 *  INPUT:
 *
 *  OUTPUT:	void - Return Type
 *  REMARK  :
 **************************************************************************/
OMX_ERRORTYPE tcc_omx_get_handle(OpenMax_IL_Components_t *Component, OMX_HANDLETYPE* pHandle,  OMX_PTR pAppData,  OMX_CALLBACKTYPE* pCallBacks)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *openmaxStandComp;
    omx_base_component_PrivateType * priv;

    if(Component->Init == NULL)
    {
        *pHandle = NULL;
        return OMX_ErrorComponentNotFound;
    }

    openmaxStandComp = TCC_fo_calloc (__func__, __LINE__,1,sizeof(OMX_COMPONENTTYPE));
    if (!openmaxStandComp)
    {
        return OMX_ErrorInsufficientResources;
    }
    eError =Component->Init((OMX_COMPONENTTYPE *)openmaxStandComp);
    if (eError != OMX_ErrorNone)
    {
        if (eError == OMX_ErrorInsufficientResources)
        {
            *pHandle = openmaxStandComp;
            priv = (omx_base_component_PrivateType *) openmaxStandComp->pComponentPrivate;
            return OMX_ErrorInsufficientResources;
        }
        priv = (omx_base_component_PrivateType *) openmaxStandComp->pComponentPrivate;
        DEBUG(DEB_LEV_ERR, "in %s Error during component construction(%s)\n",__func__, priv->name);
        openmaxStandComp->ComponentDeInit(openmaxStandComp);
        TCC_fo_free (__func__, __LINE__,openmaxStandComp);
        openmaxStandComp = NULL;
        return OMX_ErrorComponentNotFound;
    }
    priv = (omx_base_component_PrivateType *) openmaxStandComp->pComponentPrivate;
    *pHandle = openmaxStandComp;
    ((OMX_COMPONENTTYPE*)*pHandle)->SetCallbacks(*pHandle, pCallBacks, pAppData);
    DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s (%s)\n", __func__, priv->name);

    return eError;

}

/**************************************************************************
 *  FUNCTION NAME :
 *      tcc_omx_free_handle(OMX_IN OMX_HANDLETYPE hComponent)
 *
 *  DESCRIPTION :
 *  INPUT:
 *
 *  OUTPUT:	void - Return Type
 *  REMARK  :
 **************************************************************************/
OMX_ERRORTYPE tcc_omx_free_handle(OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE err = OMX_ErrorNone;

    if(hComponent == NULL)
        return OMX_ErrorComponentNotFound;

    omx_base_component_PrivateType * priv = (omx_base_component_PrivateType *) ((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;

    /* check if this component was actually loaded from this loader */
    err = ((OMX_COMPONENTTYPE*)hComponent)->ComponentDeInit(hComponent);
    TCC_fo_free (__func__, __LINE__,(OMX_COMPONENTTYPE*)hComponent);
    hComponent = NULL;
    return err;
}

int tcc_omx_disable_unused_port_linuxtv(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv, int adec_cnt, int vdec_cnt)
{
	int err = OMX_ErrorNone;

	if (adec_cnt < 2)
	{
		tcc_omx_port_disable (OMX_Dxb_Dec_AppPriv->demuxhandle, 1);
//		if (err != OMX_ErrorNone)	return err;    Noah, To avoid CodeSonar Warning, Redundant Condition, the if statement is always false.
		if(OMX_Dxb_Dec_AppPriv->demuxEventSem)
		        tsem_down(OMX_Dxb_Dec_AppPriv->demuxEventSem);

		tcc_omx_port_disable (OMX_Dxb_Dec_AppPriv->audiohandle, 1);
//		if (err != OMX_ErrorNone)	return err;    Noah, To avoid CodeSonar Warning, Redundant Condition, the if statement is always false.
		if(OMX_Dxb_Dec_AppPriv->audioEventSem)
		        tsem_down(OMX_Dxb_Dec_AppPriv->audioEventSem);

		tcc_omx_port_disable (OMX_Dxb_Dec_AppPriv->audiohandle, 3);
//		if (err != OMX_ErrorNone)	return err;    Noah, To avoid CodeSonar Warning, Redundant Condition, the if statement is always false.
		if(OMX_Dxb_Dec_AppPriv->audioEventSem)
		        tsem_down(OMX_Dxb_Dec_AppPriv->audioEventSem);

		tcc_omx_port_disable (OMX_Dxb_Dec_AppPriv->alsasinkhandle, 1);
//		if (err != OMX_ErrorNone)	return err;    Noah, To avoid CodeSonar Warning, Redundant Condition, the if statement is always false.
		if(OMX_Dxb_Dec_AppPriv->alsasinkEventSem)
		        tsem_down(OMX_Dxb_Dec_AppPriv->alsasinkEventSem);
	}

	if (vdec_cnt < 2)
	{
		tcc_omx_port_disable (OMX_Dxb_Dec_AppPriv->demuxhandle, 3);
//		if (err != OMX_ErrorNone)	return err;    Noah, To avoid CodeSonar Warning, Redundant Condition, the if statement is always false.
		if(OMX_Dxb_Dec_AppPriv->demuxEventSem)
		        tsem_down(OMX_Dxb_Dec_AppPriv->demuxEventSem);

		tcc_omx_port_disable (OMX_Dxb_Dec_AppPriv->videohandle, 1);
//		if (err != OMX_ErrorNone)	return err;    Noah, To avoid CodeSonar Warning, Redundant Condition, the if statement is always false.
		if(OMX_Dxb_Dec_AppPriv->videoEventSem)
		        tsem_down(OMX_Dxb_Dec_AppPriv->videoEventSem);

		tcc_omx_port_disable (OMX_Dxb_Dec_AppPriv->videohandle, 3);
//		if (err != OMX_ErrorNone)	return err;    Noah, To avoid CodeSonar Warning, Redundant Condition, the if statement is always false.
		if(OMX_Dxb_Dec_AppPriv->videoEventSem)
		        tsem_down(OMX_Dxb_Dec_AppPriv->videoEventSem);

		tcc_omx_port_disable (OMX_Dxb_Dec_AppPriv->fbdevhandle, 1);
//		if (err != OMX_ErrorNone)	return err;    Noah, To avoid CodeSonar Warning, Redundant Condition, the if statement is always false.
		if(OMX_Dxb_Dec_AppPriv->fbdevEventSem)
			tsem_down(OMX_Dxb_Dec_AppPriv->fbdevEventSem);
	}

	return err;
}

int tcc_omx_disable_unused_port_isdbt(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv)
{
	return tcc_omx_disable_unused_port_linuxtv(OMX_Dxb_Dec_AppPriv, 2, 2);
}

OMX_ERRORTYPE tcc_omx_disable_unused_port(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv)
{
	int	err = OMX_ErrorNone;
	DxB_STANDARDS dxb_standard = OMX_Dxb_Dec_AppPriv->iDxBStandard;

	if (dxb_standard == DxB_STANDARD_ISDBT)    err = tcc_omx_disable_unused_port_isdbt(OMX_Dxb_Dec_AppPriv);
	else DEBUG(DEB_LEV_ERR, "In %s, undefined standard (%d) is specified\n", __func__, dxb_standard);

	return err;
}

/**************************************************************************
 *  FUNCTION NAME :
 *      tcc_omx_init_component()
 *
 *  DESCRIPTION :
 *  INPUT:
 *
 *  OUTPUT:	void - Return Type
 *  REMARK  :
 **************************************************************************/
int tcc_omx_init_component(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv, OpenMax_IL_Contents_t *Contents,int  Type, OMX_HANDLETYPE* pHandle,OMX_CALLBACKTYPE* pCallBacks)
{
    int i ;
    int err = -1;

    for( i = 0 ; i < TCC_DXB_DEC_OMX_COMPONENT_NUM ; i ++ )
    {
        if( Contents->Components[i].Type != COMPONENTS_NULL )
        {
            if( Contents->Components[i].Type == (unsigned int)Type )
            {

                err = tcc_omx_get_handle(&Contents->Components[i],pHandle, OMX_Dxb_Dec_AppPriv, pCallBacks);

                if(err != OMX_ErrorNone)
                {
                    DEBUG(DEB_LEV_ERR, "Component Not Found\n");
                }
                return err;
            }
        }
        else
            return (OMX_FALSE)	;

    }
    return (OMX_FALSE);
}

/**************************************************************************
 *  FUNCTION NAME :
 *      int tcc_omx_load_component(E_COMP_MASK eMask)
 *
 *  DESCRIPTION :
 *  INPUT:
 *
 *  OUTPUT:	void - Return Type
 *  REMARK  :
 **************************************************************************/
int tcc_omx_load_component(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv, E_COMP_MASK eMask)
{
    int 		err=-1;
    OpenMax_IL_Contents_t *Contents = NULL;
    DEBUG(DEFAULT_MESSAGES,"In %s\n", __func__);
    Contents=tcc_omx_get_components_of_contents(OMX_Dxb_Dec_AppPriv);
	DxB_STANDARDS dxb_standard = OMX_Dxb_Dec_AppPriv->iDxBStandard;

    if( eMask & COMP_TUNER )
        err = tcc_omx_init_component(OMX_Dxb_Dec_AppPriv, Contents, COMPONENTS_DXB_TUNER, &OMX_Dxb_Dec_AppPriv->tunerhandle, &dxbtunercallbacks);
    if( eMask & COMP_DEMUX )
        err = tcc_omx_init_component(OMX_Dxb_Dec_AppPriv, Contents, COMPONENTS_DXB_DEMUX, &OMX_Dxb_Dec_AppPriv->demuxhandle, &dxbdemuxcallbacks);
    if( eMask & COMP_VIDEO )
        err = tcc_omx_init_component(OMX_Dxb_Dec_AppPriv, Contents, COMPONENTS_DXB_VIDEO, &OMX_Dxb_Dec_AppPriv->videohandle, &dxbvideocallbacks);
    if( eMask & COMP_FBDEV_SINK )
        err = tcc_omx_init_component(OMX_Dxb_Dec_AppPriv, Contents, COMPONENTS_DXB_FBDEV_SINK, &OMX_Dxb_Dec_AppPriv->fbdevhandle, &dxbfbdevcallbacks);
    if( eMask & COMP_AUDIO )
        err = tcc_omx_init_component(OMX_Dxb_Dec_AppPriv, Contents, COMPONENTS_DXB_AUDIO, &OMX_Dxb_Dec_AppPriv->audiohandle, &dxbaudiocallbacks);
    if( eMask & COMP_ALSA_SINK )
        err = tcc_omx_init_component(OMX_Dxb_Dec_AppPriv, Contents, COMPONENTS_DXB_ALSA_SINK, &OMX_Dxb_Dec_AppPriv->alsasinkhandle, &dxbalsasinkcallbacks);
    tcc_omx_set_state(OMX_Dxb_Dec_AppPriv, TCC_OMX_LOADED);

    return( (int)Contents );

}

int tcc_omx_select_baseband(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv, unsigned int uiBaseBand)
{
    //char value[PROPERTY_VALUE_MAX];
    OpenMax_IL_Components_t * Component;
    OMX_ERRORTYPE  (*tuner_init_fn)(OMX_COMPONENTTYPE *openmaxStandComp) = NULL;
    unsigned int	tuner_count;
    Component = tcc_omx_get_components(tcc_omx_get_components_of_contents(OMX_Dxb_Dec_AppPriv),COMPONENTS_DXB_TUNER);
	if(Component == NULL){
		return -1;
	}

    switch(OMX_Dxb_Dec_AppPriv->iDxBStandard)
    {
        case DxB_STANDARD_ISDBT:
            tuner_count = tcc_omx_isdbt_tuner_count();
            if (uiBaseBand >= tuner_count)
                uiBaseBand = 0;		//set default tuner

            tuner_init_fn = pomx_isdbt_tuner_component_init[uiBaseBand];
            if (tuner_init_fn == NULL)	// if null, set default tuner
            {
                tuner_init_fn = tcc_omx_isdbt_tuner_default();
				DEBUG(DEB_LEV_ERR,"Default tuner is selected.\n");
            }
            Component->Init = tuner_init_fn;
            break;
        default:
            return -2;
            break;
    }
    return 0;
}

int tcc_omx_select_demuxer(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv, unsigned int standard)
{
    OpenMax_IL_Components_t * Component;
    Component = tcc_omx_get_components(tcc_omx_get_components_of_contents(OMX_Dxb_Dec_AppPriv),COMPONENTS_DXB_DEMUX);
	if(Component == NULL){
		return -1;
	}
	OMX_Dxb_Dec_AppPriv->iDxBStandard = standard;

    switch(standard)
    {
        case DxB_STANDARD_ISDBT:
            Component->Init = dxb_omx_dxb_linuxtv_demux_component_Init;
            break;
        default:
            return -2;
            break;
    }
    return 0;
}

int tcc_omx_set_contents_type(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv, unsigned int ContentsType)
{
    OMX_Dxb_Dec_AppPriv->iContentsType = ContentsType; // 0:Main contents, 1:Sub contents(DAB)
    return 0;
}

int tcc_omx_get_dxb_type(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv)
{
	return OMX_Dxb_Dec_AppPriv->iDxBStandard;
}

unsigned int tcc_omx_get_contents_type(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv)
{
    return OMX_Dxb_Dec_AppPriv->iContentsType; // 0:Main contents, 1:Sub contents(DAB)
}

