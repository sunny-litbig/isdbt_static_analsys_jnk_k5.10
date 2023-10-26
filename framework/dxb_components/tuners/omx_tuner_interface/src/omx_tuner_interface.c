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
#define LOG_NDEBUG 1
#define LOG_TAG "TUNER_INTERFACE"
#include <utils/Log.h>

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_Types.h>
#include <OMX_Audio.h>
#include "OMX_Other.h"

#include <user_debug_levels.h>

#include "omx_tuner_interface.h"

#ifndef	NULL
#define	NULL	(0)
#endif

/*
  * 		ISDB-T
  *
  */
OMX_ERRORTYPE (*tcc_omx_isdbt_tuner_default (void))(OMX_COMPONENTTYPE *openaxStandComp)
{
	return	dxb_omx_tcc353x_CSPI_STS_tuner_component_Init;
}
OMX_ERRORTYPE  (*(pomx_isdbt_tuner_component_init[]))(OMX_COMPONENTTYPE *openmaxStandComp) =
{
	NULL,												// 0 - none
	NULL,		// 1 - TCC351X CSPI+STS
	NULL,					// 2 -Dib10096
	NULL,			// 3 - TCC351X I2C+STS
	NULL,					// 4 - NMI326
	NULL,		// 5 - TCC351X I2C+SPIMS
	NULL,			// 6 - MTV818
	NULL,	// 7 - Toshiba TC90517
	dxb_omx_tcc353x_CSPI_STS_tuner_component_Init,  // 8 - TCC353X CSPI+STS
	dxb_omx_tcc353x_I2C_STS_tuner_component_Init,   // 9 - TCC353X CSPI+STS
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
};
 int	tcc_omx_isdbt_tuner_count (void)
{
	return 	(sizeof(pomx_isdbt_tuner_component_init)/sizeof(pomx_isdbt_tuner_component_init[0]));
}
