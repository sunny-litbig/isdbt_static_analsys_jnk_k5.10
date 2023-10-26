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
#ifndef __SUBTITLE_PARSER_H__
#define __SUBTITLE_PARSER_H__

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
DEFINITION OF STRUCTURE
****************************************************************************/
#include <ISDBT_Caption.h>

extern int ISDBTCC_Check_DataGroupData(T_SUB_CONTEXT *p_sub_ctx, TS_PES_CAP_DATAGROUP *pDataGroup);
extern void ISDBTCC_Handle_CaptionManagementData(int data_type, TS_PES_CAP_DATAGROUP *pDataGroup);
extern void ISDBTCC_Handle_CaptionStatementData(int data_type, TS_PES_CAP_DATAGROUP *pDataGroup);

#ifdef __cplusplus
}
#endif

#endif	/* __SUBTITLE_PARSER_H__ */

