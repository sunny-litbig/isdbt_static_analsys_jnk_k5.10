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
#ifndef _OMX_LINUXTV_DEMUX_COMPONENT_H_
#define _OMX_LINUXTV_DEMUX_COMPONENT_H_

#include <omx_base_source.h>
#include "OMX_TCC_Index.h"

#define DxB_AUDIO_PORT	0
#define DxB_AUDIO2_PORT	1
#define DxB_VIDEO_PORT	2
#define DxB_VIDEO2_PORT	3

#define BROADCAST_DXB_DEMUX_NAME "OMX.tcc.broadcast.dxb_demux"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

enum TCCSWDEMUXER_REQUEST_BEGIN_ID
{
	TCCSWDEMUXER_SECTIONFILTER_BEGIN = 100000,
	TCCSWDEMUXER_PESFILTER_BEGIN     = 200000
};

enum TCCSWDEMUXER_AV_REQUEST_ID
{
    TCCSWDEMUXER_VIDEO_MAIN = -1, 
    TCCSWDEMUXER_VIDEO_SUB = -2, 
    TCCSWDEMUXER_AUDIO_MAIN = -3, 
    TCCSWDEMUXER_AUDIO_SUB = -4, 
    TCCSWDEMUXER_PCR_MAIN = -5, 
    TCCSWDEMUXER_PCR_SUB = -6
};

DERIVEDCLASS(omx_dxb_demux_component_PrivateType, omx_base_source_PrivateType)
#define omx_dxb_demux_component_PrivateType_FIELDS omx_base_source_PrivateType_FIELDS \
	OMX_PTR pPrivateData;
ENDCLASS(omx_dxb_demux_component_PrivateType)

/* Component private entry points declaration */
OMX_ERRORTYPE dxb_omx_dxb_demux_component_Constructor(OMX_COMPONENTTYPE *openmaxStandComp,OMX_STRING cComponentName);
OMX_ERRORTYPE dxb_omx_dxb_demux_component_Destructor(OMX_COMPONENTTYPE *openmaxStandComp);
OMX_ERRORTYPE dxb_omx_dxb_linuxtv_demux_component_Init(OMX_COMPONENTTYPE *openmaxStandComp);
OMX_ERRORTYPE dxb_omx_dxb_demux_component_Deinit(OMX_COMPONENTTYPE *openmaxStandComp);
OMX_ERRORTYPE dxb_omx_dxb_demux_component_MessageHandler(OMX_COMPONENTTYPE*,internalRequestMessageType*);

void dxb_omx_dxb_demux_component_BufferMgmtCallback(
	OMX_COMPONENTTYPE *openmaxStandComp,
	OMX_BUFFERHEADERTYPE* outputbuffer);

OMX_ERRORTYPE dxb_omx_dxb_demux_component_GetParameter(
  OMX_IN  OMX_HANDLETYPE hComponent,
  OMX_IN  OMX_INDEXTYPE nParamIndex,
  OMX_INOUT OMX_PTR ComponentParameterStructure);

OMX_ERRORTYPE dxb_omx_dxb_demux_component_SetParameter(
  OMX_IN  OMX_HANDLETYPE hComponent,
  OMX_IN  OMX_INDEXTYPE nParamIndex,
  OMX_IN  OMX_PTR ComponentParameterStructure);

OMX_ERRORTYPE dxb_omx_dxb_demux_component_SetConfig(
  OMX_IN  OMX_HANDLETYPE hComponent,
  OMX_IN  OMX_INDEXTYPE nIndex,
  OMX_IN  OMX_PTR ComponentParameterStructure);

OMX_ERRORTYPE dxb_omx_dxb_demux_component_GetConfig(
  OMX_IN  OMX_HANDLETYPE hComponent,
  OMX_IN  OMX_INDEXTYPE nIndex,
  OMX_IN  OMX_PTR ComponentParameterStructure);


OMX_ERRORTYPE dxb_omx_dxb_demux_component_GetExtensionIndex(
  OMX_IN  OMX_HANDLETYPE hComponent,
  OMX_IN  OMX_STRING cParameterName,
  OMX_OUT OMX_INDEXTYPE* pIndexType);

void* dxb_omx_dxb_demux_twoport_BufferMgmtFunction(void* param);

#endif /* _OMX_LINUXTV_DEMUX_COMPONENT_H_ */





