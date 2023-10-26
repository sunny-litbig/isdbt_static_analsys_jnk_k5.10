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
#ifndef _TCC_DXB_INTERFACE_OMXIL_H_
#define _TCC_DXB_INTERFACE_OMXIL_H_

#include "tsemaphore.h"
#include "OMX_Types.h"
#include "OMX_Core.h"
#include "OMX_Component.h"

/******************************************************************************
* defines 
******************************************************************************/
#define	TCC_DXB_DEC_OMX_COMPONENT_NUM		10	

#ifndef MAX_FILE_NAME
#define MAX_FILE_NAME	256
#endif

/******************************************************************************
* typedefs & structure
******************************************************************************/
typedef enum Component_Type
{
	COMPONENTS_NULL = 0,
	COMPONENTS_DXB_TUNER,
	COMPONENTS_DXB_DEMUX,
	COMPONENTS_DXB_CLOCK_SRC,
	COMPONENTS_DXB_VIDEO,
	COMPONENTS_DXB_FBDEV_SINK,
	COMPONENTS_DXB_AUDIO,
	COMPONENTS_DXB_ALSA_SINK,
	COMPONENTS_DXB_REC_SINK,
	COMPONENTS_DXB_PVR,
	COMPONENTS_TYPE_NUM
}Component_Type;

typedef enum Component_Type_MASK
{
	COMP_TUNER      = 0x01,
	COMP_DEMUX      = 0x02,
	COMP_CLOCK_SRC  = 0x04,
	COMP_VIDEO      = 0x08,
	COMP_FBDEV_SINK = 0x10,
	COMP_AUDIO      = 0x20,
	COMP_ALSA_SINK  = 0x40,
	COMP_REC_SINK   = 0x80,
	COMP_PVR	   = 0x100,
	COMP_ALL       = 0xFFFF
}E_COMP_MASK;


typedef enum TCC_OMX_DXB_DEC_STATE_TYPE
{
    TCC_OMX_NULL =0,
    TCC_OMX_LOADED,    
    TCC_OMX_TUNNEL,
    TCC_OMX_IDLE,
    TCC_OMX_EXECUTING,
    TCC_OMX_PAUSE
} TCC_OMX_DXB_DEC_STATE_TYPE;


typedef	struct	OpenMax_IL_Components_Struct
{
	unsigned int Type;
	OMX_ERRORTYPE           (*Init)( OMX_COMPONENTTYPE *);
}OpenMax_IL_Components_t;

typedef	struct	OpenMax_IL_Contents_Struct
{
	unsigned int Type;
	OpenMax_IL_Components_t Components[ TCC_DXB_DEC_OMX_COMPONENT_NUM] ;
}OpenMax_IL_Contents_t;


typedef struct Omx_Dxb_Dec_App_Private_Type
{
	OpenMax_IL_Contents_t tDxBContents;

	tsem_t* tunerEventSem;
	tsem_t* demuxEventSem;
	tsem_t* clocksrcEventSem;
	tsem_t* videoEventSem;
	tsem_t* fbdevEventSem;
	tsem_t* audioEventSem;
	tsem_t* alsasinkEventSem;
	tsem_t* pvrEventSem;
	tsem_t* eofSem;  
	
	OMX_HANDLETYPE tunerhandle;
	OMX_HANDLETYPE demuxhandle;
	OMX_HANDLETYPE clocksrchandle;
	OMX_HANDLETYPE videohandle;
	OMX_HANDLETYPE fbdevhandle;
	OMX_HANDLETYPE audiohandle;
	OMX_HANDLETYPE alsasinkhandle;
	OMX_HANDLETYPE pvrhandle;
	
	unsigned int		current_time;
	unsigned int		state_type;

	void *pvParent;
	int iDxBStandard;
	int iContentsType;
}Omx_Dxb_Dec_App_Private_Type;


typedef enum 
{
	StateInvalid =0, 
	StateLoaded,    
	StateTunneled,	
	StateIdle,       
	StateExecuting,
	StatePause,     
	StateWaitForResources, 
	StateMax,
} COMPONENT_STATETYPE;

/** Specification version*/
#define VERSIONMAJOR    1
#define VERSIONMINOR    1
#define VERSIONREVISION 0
#define VERSIONSTEP     0

/******************************************************************************
* declarations
******************************************************************************/
int tcc_omx_select_audiovideo_type(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv, int iAudioType, int iVideoType);
OMX_ERRORTYPE tcc_omx_disable_unused_port (Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv);
int tcc_omx_setup_tunnel(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv);
int tcc_omx_statechange_from_loading_to_idle(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv);
int tcc_omx_statechange_from_idle_to_execute(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv);
int tcc_omx_statechange_from_execute_to_idle(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv);
int tcc_omx_statechange_from_idle_to_load(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv);
int tcc_omx_component_free(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv, E_COMP_MASK eMask);
Omx_Dxb_Dec_App_Private_Type* tcc_omx_init(void *pvParent);
void tcc_omx_deinit(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv);
int tcc_omx_execute(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv);
int tcc_omx_stop(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv);
int tcc_omx_get_current_time(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv);
void tcc_omx_set_state(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv, int type);
int tcc_omx_get_state(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv);
int tcc_omx_port_enable(OMX_HANDLETYPE handle,int port_id);
int tcc_omx_port_disable(OMX_HANDLETYPE handle,int port_id);
OpenMax_IL_Contents_t*	tcc_omx_get_components_of_contents(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv);
OpenMax_IL_Components_t * tcc_omx_get_components(OpenMax_IL_Contents_t * Contents,int type);
OMX_ERRORTYPE tcc_omx_get_handle(OpenMax_IL_Components_t *Component, OMX_HANDLETYPE* pHandle,  OMX_PTR pAppData,  OMX_CALLBACKTYPE* pCallBacks);
OMX_ERRORTYPE tcc_omx_free_handle(OMX_IN OMX_HANDLETYPE hComponent);
int tcc_omx_init_component(Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv, OpenMax_IL_Contents_t *Contents,int  Type, OMX_HANDLETYPE* pHandle,OMX_CALLBACKTYPE* pCallBacks);
int tcc_omx_load_component(Omx_Dxb_Dec_App_Private_Type *OMX_Dxb_Dec_AppPriv, E_COMP_MASK eMask);
int tcc_omx_select_baseband(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv, unsigned int uiBaseBand);
int tcc_omx_select_demuxer(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv, unsigned int standard);
int tcc_omx_set_contents_type(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv, unsigned int ContentsType);
unsigned int tcc_omx_get_contents_type(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv);
int tcc_omx_clear_port_buffer(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv);
int tcc_omx_get_dxb_type(Omx_Dxb_Dec_App_Private_Type* OMX_Dxb_Dec_AppPriv);

#ifdef      ENABLE_REMOTE_PLAYER
extern OMX_BOOL TCC_DxB_Get_Remote_Service_Status(void);
#endif

#endif

