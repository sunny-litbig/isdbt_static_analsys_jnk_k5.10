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
extern void CCPARS_Parse_Pos_Init(T_SUB_CONTEXT *p_sub_ctx, int mngr, int index, int data);
extern void CCPARS_Parse_Pos_CRLF(T_SUB_CONTEXT *p_sub_ctx, int mngr, int count, int dir_check);
extern void CCPARS_Parse_Pos_Forward(T_SUB_CONTEXT *p_sub_ctx, int mngr, int count, int need_init);
extern void CCPARS_Parse_Pos_Backward(T_SUB_CONTEXT *p_sub_ctx, int mngr, int count, int need_init);
extern void CCPARS_Parse_Pos_Up(T_SUB_CONTEXT *p_sub_ctx, int mngr, int count, int need_init);
extern void CCPARS_Parse_Pos_Down(T_SUB_CONTEXT *p_sub_ctx, int mngr, int count, int need_init);
