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
#ifndef _VPU_CLK_CTRL_H_
#define _VPU_CLK_CTRL_H_

#include "../cdk/cdk_pre_define.h"
#ifdef VPU_CLK_CONTROL

/************************************************************************/
/*						                                                */
/* Defines				                                                */
/*						                                                */
/************************************************************************/


#ifndef _TCC9300_
#define VPU_CLK_CTRL_VBUS_PWR_DOWN_REG			0xF0702000
#define VPU_CLK_CTRL_CLKCTRL_REG_BASE			0xF0400000	//CLK Register Base Address
#define VPU_CLK_CTRL_CLKCTRL5_REG				0xF0400014	//Bus Clock Control Register for VIdeo Codec
#define VPU_CLK_CTRL_CLKCTRL6_REG				0xF0400018	//Core Clock Control Register for VIdeo Codec
#else
#define VPU_CLK_CTRL_VBUS_PWR_DOWN_REG			0xB0920000
#define VPU_CLK_CTRL_CLKCTRL_REG_BASE			0xB0500000	//CLK Register Base Address
#define VPU_CLK_CTRL_CLKCTRL5_REG				0xB0500014	//Bus Clock Control Register for VIdeo Codec
#define VPU_CLK_CTRL_CLKCTRL6_REG				0xB0500018	//Core Clock Control Register for VIdeo Codec
#endif
#define VPU_CLK_CTRL_VBUS_PWR_DOWN_CODEC_MASK	(1<<2)
#define VPU_CLK_CTRL_CKLCTRL_SEL				( 0)
#define VPU_CLK_CTRL_CKLCTRL_SYNC				( 3)
#define VPU_CLK_CTRL_CKLCTRL_CONFIG				( 4)
#define VPU_CLK_CTRL_CKLCTRL_MD					(20)
#define VPU_CLK_CTRL_CKLCTRL_EN					(21)

/*!
 ***********************************************************************
 * \brief
 *		vpu clock init. function for LINUX
 ***********************************************************************
 */
int
vpu_clock_init(void);

/*!
 ***********************************************************************
 * \brief
 *		vpu clock deinit. function for LINUX
 ***********************************************************************
 */
int
vpu_clock_deinit(void);

#endif
#endif //_VPU_CLK_CTRL_H_
