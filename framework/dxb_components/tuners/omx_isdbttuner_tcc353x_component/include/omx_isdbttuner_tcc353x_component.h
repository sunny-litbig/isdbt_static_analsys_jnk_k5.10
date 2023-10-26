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
#include <OMX_Types.h>
#include <OMX_Component.h>
#include <OMX_Core.h>
#include <OMX_Audio.h>
#include <pthread.h>
#include <omx_base_source.h>
#include <omx_base_audio_port.h>
//#include "tcc_file.h"
//#include "tcc_memory.h"


/** Alsasrcport component private structure.
 * see the define above
 */

#ifndef TRUE
#define TRUE                1
#endif
	
#ifndef FALSE
#define FALSE               0
#endif


#define BROADCAST_TCC353X_I2C_STS_TUNER_NAME "OMX.tcc.broadcast.tcc353x.i2c.sts"
#define BROADCAST_TCC353X_CSPI_STS_TUNER_NAME "OMX.tcc.broadcast.tcc353x.cspi.sts"


DERIVEDCLASS(omx_isdbt_tuner_tcc353x_component_PrivateType, omx_base_source_PrivateType)
#define omx_isdbt_tuner_tcc353x_component_PrivateType_FIELDS omx_base_source_PrivateType_FIELDS \
  int		countrycode;
ENDCLASS(omx_isdbt_tuner_tcc353x_component_PrivateType)


/* Component private entry points declaration */
OMX_ERRORTYPE dxb_omx_isdbt_tuner_tcc353x_component_Constructor(OMX_COMPONENTTYPE *openmaxStandComp,OMX_STRING cComponentName);
OMX_ERRORTYPE dxb_omx_isdbt_tuner_tcc353x_component_Destructor(OMX_COMPONENTTYPE *openmaxStandComp);

void dxb_omx_isdbt_tuner_tcc353x_component_BufferMgmtCallback(
	OMX_COMPONENTTYPE *openmaxStandComp,
	OMX_BUFFERHEADERTYPE* inputbuffer);

OMX_ERRORTYPE dxb_omx_isdbt_tuner_tcc353x_component_GetParameter(
	OMX_IN  OMX_HANDLETYPE hComponent,
	OMX_IN  OMX_INDEXTYPE nParamIndex,
	OMX_INOUT OMX_PTR ComponentParameterStructure);

OMX_ERRORTYPE dxb_omx_isdbt_tuner_tcc353x_component_SetParameter(
	OMX_IN  OMX_HANDLETYPE hComponent,
	OMX_IN  OMX_INDEXTYPE nParamIndex,
	OMX_IN  OMX_PTR ComponentParameterStructure);

OMX_ERRORTYPE dxb_omx_isdbt_tuner_tcc353x_component_MessageHandler(
	OMX_COMPONENTTYPE* openmaxStandComp, 
	internalRequestMessageType *message);


OMX_ERRORTYPE dxb_omx_isdbt_tuner_tcc353x_component_GetConfig(
  OMX_IN  OMX_HANDLETYPE hComponent,
  OMX_IN  OMX_INDEXTYPE nIndex,
  OMX_IN  OMX_PTR pComponentConfigStructure);

OMX_ERRORTYPE dxb_omx_isdbt_tuner_tcc353x_component_SetConfig(
  OMX_IN  OMX_HANDLETYPE hComponent,
  OMX_IN  OMX_INDEXTYPE nIndex,
  OMX_IN  OMX_PTR pComponentConfigStructure);

OMX_ERRORTYPE dxb_omx_isdbt_tuner_tcc353x_component_GetExtensionIndex(
  OMX_IN  OMX_HANDLETYPE hComponent,
  OMX_IN  OMX_STRING cParameterName,
  OMX_OUT OMX_INDEXTYPE* pIndexType);

void dxb_omx_isdbt_tuner_tcc353x_spi_thread(void* param);

OMX_ERRORTYPE dxb_omx_tcc353x_I2C_STS_tuner_component_Init (OMX_COMPONENTTYPE *openmaxStandComp);
OMX_ERRORTYPE dxb_omx_tcc353x_CSPI_STS_tuner_component_Init (OMX_COMPONENTTYPE *openmaxStandComp);
OMX_ERRORTYPE dxb_omx_tcc353x_I2C_STS_tuner_component_Deinit(OMX_COMPONENTTYPE *openmaxStandComp);
OMX_ERRORTYPE dxb_omx_tcc353x_CSPI_STS_tuner_component_Deinit(OMX_COMPONENTTYPE *openmaxStandComp);


