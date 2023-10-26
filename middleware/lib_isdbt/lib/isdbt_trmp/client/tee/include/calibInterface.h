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
#ifndef CALIB_INTERFACE_H
#define CALIB_INTERFACE_H

#include <tee_client_api.h>

#define CALIB_PARAM_NONE 0
#define CALIB_PARAM_BUF_IN 1
#define CALIB_PARAM_BUF_OUT 2
#define CALIB_PARAM_BUF_INOUT 3
#define CALIB_PARAM_SEC_BUF_IN 4
#define CALIB_PARAM_SEC_BUF_OUT 5
#define CALIB_PARAM_SEC_BUF_INOUT 6
#define CALIB_PARAM_VALUE_IN 7
#define CALIB_PARAM_VALUE_OUT 8
#define CALIB_PARAM_VALUE_INOUT 9

#define CALIB_PARAM_NUM 4

struct CALibParam {
	struct {
		void* m_pBuffer;
		uint32_t m_uSize;
	} CALibMemRef;
	struct {
		uint32_t m_uA;
		uint32_t m_uB;
	} CALibValue;

	uint32_t m_uType;
};

struct CALibParams {
	struct CALibParam m_apParams[4];
};

typedef void* CALibContextHdl;

TEEC_Result CALib_OpenTA(TEEC_UUID* pUUID, struct CALibParams* pParams, CALibContextHdl* ppContext);
void CALib_CloseTA(CALibContextHdl pContext);
TEEC_Result
CALib_ExecuteCommand(CALibContextHdl pContext, struct CALibParams* pParams, uint32_t uCommandID);
void CALib_InitializeParam(struct CALibParams* pParams);

#endif
