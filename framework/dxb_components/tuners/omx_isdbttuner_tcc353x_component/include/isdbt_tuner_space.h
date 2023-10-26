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
 /******************************************************************************
* include 
******************************************************************************/


/*****************************************************************************
* define
******************************************************************************/

#ifndef TUNER_SPACE_H
#define TUNER_SPACE_H


#ifndef TRUE
#define TRUE                1
#endif
	
#ifndef FALSE
#define FALSE               0
#endif

#ifndef NULL
#define NULL               0
#endif

#define DEFAULT_ISDBT_COUNTRY	0 

#define DEFAULT_BANDWIDTH	  6000	
#define DEFAULT_SIGNAL_TYPE   1     // air


/*****************************************************************************
* structures
******************************************************************************/
typedef enum
{
	ISDBT_NIT_PID = 0x10,		//Network Information Table
	ISDBT_SDT_PID = 0x11,		//Service Description Table
	ISDBT_EIT_PID = 0x12, 		//Event Information Table
	ISDBT_RST_PID = 0x13,		//Running Status Table
	ISDBT_TDT_PID = 0x14		//Time and Date Table
}IsdbtDefaultPidType;


typedef struct
{
	char name[6];
	int frequency1; // primary frequency in Khz
	int frequency2; // alternative frequency
	int finetune; // finetune offset in Khz, 0=no finetune
	short bandwidth1; // 6/7/8 Mhz
	short bandwidth2; // alternative bandwidth
	short alt_selected; // alternative selected or not
	short flag;	// 1 - available, 0 - not available
}OneChannel;


typedef struct
{
	char name[32]; // name of the tune space
	unsigned long countryCode; // country code
	unsigned long space; // 0=terrestrial, 1=cable, 2=satellite
	unsigned long typicalBandwidth; // typical bandwidth
	unsigned long minChannel; // min channel
	unsigned long maxChannel; // max channel
	long finetunes[4]; // pilot finetunes
	unsigned long numbers; // number of channels
	OneChannel channels[128];
}TuneSpace;

typedef struct
{
	unsigned int	 SignalType;
	unsigned int	 DefaultBandWidth;
	unsigned int	 CountryCode;
	TuneSpace CurrentSpace;
}IsdbtTunerSpace;

typedef struct
{
	unsigned int	minChannel;	//
	unsigned int	maxChannel;
	unsigned int	numbers;
}TableIndexInfo;
typedef struct
{
	unsigned int	countryCode;
	unsigned int	tablenumbers;
	TableIndexInfo 	freqtableinfo[4];
}FrequencyRange;

/*****************************************************************************
* Variables
******************************************************************************/
IsdbtTunerSpace ISDBTTunerSpace;
/******************************************************************************
* declarations
******************************************************************************/
void SetSignalType(int signaltype);
void SetBandwidth(int bandwidth);
void CTuneSpace(int countrycode);
void SetCountryCode(unsigned long countryCode);
int GetFrequency(unsigned long chIndex);
int GetFinetune(unsigned long chIndex);
int GetBandwidth(unsigned long chIndex);
OneChannel* GetOneChannel(unsigned long chIndex);
void SetFinetune(unsigned long chIndex, long finetune);
void SetAltSelected(unsigned long chIndex, int makeAlt);
int GetPilotFinetunes(unsigned long* pNumbers, long* pFinetunes);
int GetFrequencyValidity(unsigned long chIndex);

int	GetMinChannel(void);
int	GetMaxChannel(void);
int SetFreqBand (int freq_band);

#endif		//TUNER_SPACE_H

