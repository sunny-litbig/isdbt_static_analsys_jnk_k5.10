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
#define LOG_TAG	"DXB_INTERFACE_SYSTEM"
#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_Types.h>

#include <utils/Log.h>
#include <user_debug_levels.h>
#include <omx_base_component.h>

#include "tcc_dxb_interface_type.h"
#include "tcc_dxb_interface_omxil.h"
#include "tcc_dxb_interface_system.h"
 
DxB_ERR_CODE TCC_DxB_SYSTEM_Init(void)
{
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_SYSTEM_Exit(void)
{

	return DxB_ERR_OK;
}
