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
#ifndef _ISDBT_TUNER_TCC353X_H_
#define _ISDBT_TUNER_TCC353X_H_
/******************************************************************************
* include 
******************************************************************************/
#include "tcpal_types.h"
#include "tcc353x_monitoring.h"

/*****************************************************************************
* define
******************************************************************************/
#ifndef min
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif

/*****************************************************************************
* structures
******************************************************************************/
typedef enum
{
	SEARCH_START = 0,
	SEARCH_CONTINUE,
	SEARCH_STOP
} E_SEARCH_CMD;

typedef struct
{
	I32U uiMinChannel;
	I32U uiMaxChannel;
	I32U uiCountryCode;
}TUNER_SEARCH_INFO;

typedef struct
{
	I32U Channel;
	I32U Freq;
	I32U Search_Stop_Flag;
}ISDBT_CHANNEL_INFO;

/*****************************************************************************
* Variables
******************************************************************************/
I32U    ISDBT_Tunertype;
ISDBT_CHANNEL_INFO *ISDBT_ChannelInfo;

/******************************************************************************
* declarations
******************************************************************************/
I32S tcc353x_init(I32S commandInterface, I32S streamInterface);
I32S tcc353x_deinit(I32S commandInterface, I32S streamInterface);
I32S tcc353x_search_channel(I32S searchmode, I32S channel ,I32S countrycode);
I32S tcc353x_channel_set(I32S channel);
I32S tcc353x_channel_tune(I32S channel);
I32S tcc353x_frequency_set(I32S iFreqKhz, I32S iBWKhz);
int tcc353x_frequency_get (I32S channel, I32S *pFreq);
I32S tcc353x_get_signal_strength(Tcc353xStatus_t *_monitorValues);
I32S tcc353x_get_ews_flag (void);
I32S tcc353x_get_tmcc_information(tmccInfo_t *pTmccinfo);
I32S tcc353x_get_lock_status (I32S *arg);
I32S tcc353x_UserLoopStopCmd(I32S _moduleIndex);

#endif

