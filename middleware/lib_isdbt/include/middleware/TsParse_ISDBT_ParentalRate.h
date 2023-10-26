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
#include <ISDBT_Common.h>

#define AGE_RATE_ER		AGE_RATE_MAX
#define AGE_RATE_L		AGE_RATE_MAX

#define ISDBT_PARENT_RATE_MAX_CONTENT 	(7)
#define ISDBT_PASSWD_MAX_LEN          		(4)

typedef enum
{
	AGE_RATE_10,	// Block if the age will be less than 10 years old
	AGE_RATE_12,	// Block if the age will be less than 12 years old
	AGE_RATE_14,	// Block if the age will be less than 14 years old
	AGE_RATE_16,	// Block if the age will be less than 16 years old
	AGE_RATE_18,	// Block if the age will be less than 18 years old
	AGE_RATE_MAX
} E_ISDBT_VIEWER_RATE;

typedef struct
{
	unsigned char				AgeClass;
	E_ISDBT_VIEWER_RATE	Rate;
	char 					*RateString;
} ISDBT_PR_AGE_CLASS;

typedef struct
{
	unsigned char	Desc;
	char 		*DescString;
} ISDBT_PR_CONTENT_DESC;

extern void ISDBT_AccessCtrl_ProcessParentalRating(void);
extern unsigned char ISDBT_AccessCtrl_ExtractContentDesc(unsigned char ratingField);
extern unsigned char ISDBT_AccessCtrl_ExtractAgeRate(unsigned char ratingField);
extern E_ISDBT_VIEWER_RATE ISDBT_AccessCtrl_GetAgeRate(void);
extern void ISDBT_AccessCtrl_SetAgeRate(unsigned char user_setting_age_from_UI);
extern char * ISDBT_AccessCtrl_GetParentAgeRateString(void);
extern char * ISDBT_AccessCtrl_GetTSAgeRateString(unsigned char ageClass);
extern E_ISDBT_VIEWER_RATE ISDBT_AccessCtrl_GetRateClass(unsigned char ucAgeClass);
extern void ISDBT_AccessCtrl_ProcessParentalRating(void);

