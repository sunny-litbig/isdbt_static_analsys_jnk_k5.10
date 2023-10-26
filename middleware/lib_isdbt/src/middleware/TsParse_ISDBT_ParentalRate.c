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
#define LOG_TAG	"ISDBT_PARENTALRATE"
#include <utils/Log.h>

#include <TsParse_ISDBT.h>
#include <TsParse_ISDBT_ParentalRate.h>

/****************************************************************************
DEFINE
****************************************************************************/


/****************************************************************************
TYPE DEFINE
****************************************************************************/
/* 
Below enumeration is already defined in isdbt.h. (Actually, duplication error)
TODO : We have to seprate common header file for using app and dxb_library
*/
typedef enum
{
	E_TABLE_ID_PMT,
	E_TABLE_ID_NIT,
	E_TABLE_ID_SDT,
	E_TABLE_ID_BAT,
	E_TABLE_ID_EIT,
	E_TABLE_ID_BIT,
	E_TABLE_ID_CAT,

	E_TABLE_ID_MAX
} E_TABLE_ID_TYPE;


/****************************************************************************
STATIC VARIABLES
****************************************************************************/
const ISDBT_PR_AGE_CLASS ISDBT_PR_AgeClass[] = {
	{0x02, AGE_RATE_10, "Block if the age is less than 10 years old"}, 
	{0x03, AGE_RATE_12, "Block if the age is less than 12 years old"}, 
	{0x04, AGE_RATE_14, "Block if the age is less than 14 years old"}, 
	{0x05, AGE_RATE_16, "Block if the age is less than 16 years old"}, 
	{0x06, AGE_RATE_18, "Block if the age is less than 18 years old"}
};

const ISDBT_PR_CONTENT_DESC ISDBT_PR_ContentDesc[ISDBT_PARENT_RATE_MAX_CONTENT] = {
	{0x01, "Drug"},
	{0x02, "Violence"},
	{0x03, "Violence and drug"},
	{0x04, "Sex"},
	{0x05, "Sex and drug"},
	{0x06, "Violence and sex"},
	{0x07, "Violence, sex and drug"}
};

/* Used for setting/checking Age/Password for blocking/unblocking the airplay */
unsigned char g_ParentRating_Passwd[ISDBT_PASSWD_MAX_LEN] = {0x00, 0x00, 0x00, 0x00};
E_ISDBT_VIEWER_RATE g_ParentRating_Age = AGE_RATE_MAX;

/****************************************************************************
EXTERNAL VARIABLES
****************************************************************************/


/****************************************************************************
EXTERNAL FUNCTIONS
****************************************************************************/
extern int ISDBT_Get_DescriptorInfo(unsigned char ucDescType, void *pDescData);
extern int tcc_manager_demux_set_parentlock(unsigned int lock);
extern void notify_parentlock_complete(int age);


/****************************************************************************
LOCAL FUNCTIONS
****************************************************************************/
unsigned char ISDBT_AccessCtrl_ExtractContentDesc(unsigned char ratingField)
{
	return (ratingField & 0xF0)>>4;
}

unsigned char ISDBT_AccessCtrl_ExtractAgeRate(unsigned char ratingField)
{	
	return (ratingField & 0xF);
}

E_ISDBT_VIEWER_RATE ISDBT_AccessCtrl_GetAgeRate(void)
{	
	return g_ParentRating_Age;
}

void ISDBT_AccessCtrl_SetAgeRate(unsigned char age_from_UI)
{
	switch(age_from_UI)
	{
		case 0:	g_ParentRating_Age = AGE_RATE_10;	break;
		case 1:	g_ParentRating_Age = AGE_RATE_12;	break;
		case 2:	g_ParentRating_Age = AGE_RATE_14;	break;
		case 3:	g_ParentRating_Age = AGE_RATE_16;	break;
		case 4:	g_ParentRating_Age = AGE_RATE_18;	break;

		default:	g_ParentRating_Age = AGE_RATE_MAX;	break;
	}
	
	ALOGI("[Parental Rating] %s\n", ISDBT_AccessCtrl_GetParentAgeRateString());
}

char * ISDBT_AccessCtrl_GetParentAgeRateString(void)
{	
	char * retString = "No restiriction";

	if(g_ParentRating_Age<=AGE_RATE_18)
	{
		retString = ISDBT_PR_AgeClass[g_ParentRating_Age].RateString;
	}

	return retString;
}

char * ISDBT_AccessCtrl_GetTSAgeRateString(unsigned char ageClass)
{
	unsigned char idx;
	char * retString = "No restiriction";

	for(idx=0; idx<AGE_RATE_MAX; idx++)
	{
		if(ageClass == ISDBT_PR_AgeClass[idx].AgeClass)
			return ISDBT_PR_AgeClass[idx].RateString;
	}

	return retString;
}

E_ISDBT_VIEWER_RATE ISDBT_AccessCtrl_GetRateClass(unsigned char ucAgeClass)
{
	int i;

	for(i=0; i<AGE_RATE_MAX; i++)
	{
		if(ucAgeClass == ISDBT_PR_AgeClass[i].AgeClass)
			return ISDBT_PR_AgeClass[i].Rate;
	}

	return AGE_RATE_MAX;
}

void ISDBT_AccessCtrl_ProcessParentalRating(void)
{
	unsigned char ucClass;
	char * pClassStr = NULL;
	T_DESC_PR stDescPR;
	E_ISDBT_VIEWER_RATE eTSRate;

	/* Get the setting value of device user's age rate */
	E_ISDBT_VIEWER_RATE eParentalRate = ISDBT_AccessCtrl_GetAgeRate();
	if(eParentalRate == AGE_RATE_MAX)
	{
		tcc_manager_demux_set_parentlock(FALSE);
		return;
	}

	/* Get the structure value of parent rating descriptor (from PMT section or present section of EIT) */
	if(ISDBT_Get_DescriptorInfo(E_DESC_PARENT_RATING, (void *)&stDescPR))
	{
		if(stDescPR.ucTableID == E_TABLE_ID_MAX)
		{
			//ALOGI("[Parental Rating] Not yet recieved E_DESC_PARENT_RATE\n");
			return;
		}
		
		if(stDescPR.Rating == 0xff) // fail to find any parental rating info. in the current TS stream
		{
			pClassStr = "No block";
			eTSRate = AGE_RATE_MAX;
		}
		else
		{
			/* Extract 4-bits 'classfication for age' field from 1 byte 'Rating' */
			/* (b7-b4) Description of the content, (b3-b0) Classfication for age */
			ucClass = ISDBT_AccessCtrl_ExtractAgeRate(stDescPR.Rating);

			pClassStr = ISDBT_AccessCtrl_GetTSAgeRateString(ucClass);
			
			/* Compare age rate from TS with one from device-setting & set currnet status of TS blockage */
			eTSRate =ISDBT_AccessCtrl_GetRateClass(ucClass);
		}

		ALOGI("[Parental Rating] %s (parent:%d, ts:%d)\n", pClassStr, (int)eParentalRate, (int)eTSRate);

		if((eTSRate < AGE_RATE_MAX) && (eParentalRate <= eTSRate))
		{
			ALOGI("[Parental Rating] Show block message\n");
			tcc_manager_demux_set_parentlock(TRUE);
			//ljh TCC_Isdbt_Dec_ParentalRate_DONE_Reply_CMD(TRUE);
		}
		else
		{
			ALOGI("[Parental Rating] Clear block message\n");
			tcc_manager_demux_set_parentlock(FALSE);
			//ljh TCC_Isdbt_Dec_ParentalRate_DONE_Reply_CMD(FALSE);
		}
	}
}

