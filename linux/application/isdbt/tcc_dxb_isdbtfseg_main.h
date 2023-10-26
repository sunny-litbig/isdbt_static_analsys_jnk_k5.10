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
#ifndef __TCC_DXB_ISDBTFSEG_MAIN_H__
#define __TCC_DXB_ISDBTFSEG_MAIN_H__


/******************************************************************************
* include 
******************************************************************************/


/******************************************************************************
* defines 
******************************************************************************/
#define	_APPLICATION_VERSION_   "001"
#define _DXB_SERVICE_VER_        "DXB_SERVICE_VER_"_APPLICATION_VERSION_
#define _DXB_SERVICE_PATCH_VER_  _DXB_SERVICE_VER_""

/******************************************************************************
* typedefs & structure
******************************************************************************/

/******************************************************************************
* globals
******************************************************************************/
extern int Init_Video_Output_Path;
extern int Init_CountryCode;
extern int Init_ProcessMonitoring;
extern int Init_Support_UART_Console;
extern int Init_stand_alone_processing;
extern int Init_Not_Skip_Same_program;
extern int Init_Disable_DualDecode;
extern int Init_Mute_GPIO_Port;
extern int Init_Mute_GPIO_Polarity;
extern int Init_Support_PVR;
extern int Init_StrengthFullToOne;
extern int Init_StrengthOneToFull;
extern int Init_CountSecFullToOne;
extern int Init_CountSecOneToFull;
extern int Init_Support_PVR;
extern int Init_PVR_CountryCode_Mode;
extern int Init_PrintLog_SignalStrength;

/******************************************************************************
* locals
******************************************************************************/


/******************************************************************************
* declarations
******************************************************************************/


#endif
