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
#include <stdlib.h>

#include "OMX_Component.h"
#include "OMX_TCC_Index.h"

#include "isdbt_tuner.h"

//ISDBT
OMX_ERRORTYPE omx_tcc353x_CSPI_STS_tuner_component_Init(OMX_COMPONENTTYPE *openmaxStandComp);
OMX_ERRORTYPE omx_tcc353x_CSPI_STS_tuner_component_Deinit(OMX_COMPONENTTYPE *openmaxStandComp);
OMX_ERRORTYPE omx_tcc353x_I2C_STS_tuner_component_Init(OMX_COMPONENTTYPE *openmaxStandComp);
OMX_ERRORTYPE omx_tcc353x_I2C_STS_tuner_component_Deinit(OMX_COMPONENTTYPE *openmaxStandComp);

static int ISDBT_BBType = -1;

static int isdbt_tuner_set(OMX_COMPONENTTYPE *pOpenmaxStandComp, int iFreq, int iBW)
{
	OMX_ERRORTYPE eError;
	int iARGS[4];

	iARGS[0] = iFreq;
	iARGS[1] = iBW;
	iARGS[2] = 1;
	iARGS[3] = 0;

	eError = OMX_SetParameter(pOpenmaxStandComp, (OMX_INDEXTYPE)OMX_IndexVendorParamTunerFrequencySet, iARGS);
	if (eError == OMX_ErrorNone)
	{
		return 0;
	}

	return -1;
}

OMX_COMPONENTTYPE* isdbt_tuner_open(int iFreq, int iBW)
{
	OMX_ERRORTYPE eError;
	int iARGS[3];
	OMX_COMPONENTTYPE *pOpenmaxStandComp;

	iARGS[0] = 0;
	iARGS[1] = 1;
	iARGS[2] = 2;

	pOpenmaxStandComp = (OMX_COMPONENTTYPE *)calloc(1,sizeof(OMX_COMPONENTTYPE));
	if (pOpenmaxStandComp == NULL)
	{
		return NULL;
	}

	ISDBT_BBType = 0;
	eError = omx_tcc353x_CSPI_STS_tuner_component_Init(pOpenmaxStandComp);
	if (eError == OMX_ErrorNone)
	{
		eError = OMX_SetParameter(pOpenmaxStandComp, (OMX_INDEXTYPE)OMX_IndexVendorParamTunerOpen, iARGS);
		if (eError == OMX_ErrorNone)
		{
			if (isdbt_tuner_set(pOpenmaxStandComp, iFreq, iBW) == 0)
			{
				return pOpenmaxStandComp;
			}
		}
		OMX_SetParameter(pOpenmaxStandComp, (OMX_INDEXTYPE)OMX_IndexVendorParamTunerClose, NULL);

	}
	omx_tcc353x_CSPI_STS_tuner_component_Deinit(pOpenmaxStandComp);

	ISDBT_BBType = 1;
	eError = omx_tcc353x_I2C_STS_tuner_component_Init(pOpenmaxStandComp);
	if (eError == OMX_ErrorNone)
	{	
		eError = OMX_SetParameter(pOpenmaxStandComp, (OMX_INDEXTYPE)OMX_IndexVendorParamTunerOpen, iARGS);
		if (eError == OMX_ErrorNone)
		{
			if (isdbt_tuner_set(pOpenmaxStandComp, iFreq, iBW) == 0)
			{
				return pOpenmaxStandComp;
			}
		}
		OMX_SetParameter(pOpenmaxStandComp, (OMX_INDEXTYPE)OMX_IndexVendorParamTunerClose, NULL);
	}
	omx_tcc353x_I2C_STS_tuner_component_Deinit(pOpenmaxStandComp);

	free(pOpenmaxStandComp);
	ISDBT_BBType = -1;

	return NULL;
}

void isdbt_tuner_close(OMX_COMPONENTTYPE *pOpenmaxStandComp)
{
	if (ISDBT_BBType < 0 || pOpenmaxStandComp == NULL)
	{
		return;
	}

	OMX_SetParameter(pOpenmaxStandComp, (OMX_INDEXTYPE)OMX_IndexVendorParamTunerClose, NULL);

	if (ISDBT_BBType == 0)
	{
		omx_tcc353x_CSPI_STS_tuner_component_Deinit(pOpenmaxStandComp);
	}
	else if (ISDBT_BBType == 1)
	{
		omx_tcc353x_I2C_STS_tuner_component_Deinit(pOpenmaxStandComp);
	}

	free(pOpenmaxStandComp);
	ISDBT_BBType = -1;
}
