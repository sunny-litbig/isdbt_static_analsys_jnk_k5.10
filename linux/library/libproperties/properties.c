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
#include <stdio.h>
#include <properties.h>
#include <sys/stat.h> //for mkdir


#if 1//for debug
#define PRINTF_ERROR printf
#else
#define PRINTF_ERROR
#endif 

#if 0//for debug
#define PRINTF_DEBUG printf
#else
#define PRINTF_DEBUG
#endif 

#define PROPERTY_KEY_MAX    32/*PROP_NAME_MAX*/
#define PROPERTY_VALUE_MAX  92/*PROP_VALUE_MAX*/


#define CMD_CAT "cat "
#define PROPERTY_PATH "/tmp/property/"

/* property_get: returns the length of the value which will never be
** greater than PROPERTY_VALUE_MAX - 1 and will always be zero terminated.
** (the length does not include the terminating zero).
**
** If the property read fails or returns an empty value, the default
** value is used (if nonnull).
*/
int property_get(const char *key, char *value, const char *default_value)
{
	FILE *fp;
	char *ret;
	char szCmd[256];
	int len = 0;

	snprintf(szCmd, 256, "%s%s", PROPERTY_PATH, key);
	fp = fopen(szCmd, "r");
	if( fp == NULL )
	{
		if(default_value)
		{
			property_set(key, default_value);
	        len = strlen(default_value);
	        memcpy(value, default_value, len + 1);
		}
		return len;
	}

	ret = fgets(value, PROPERTY_VALUE_MAX, fp);
	PRINTF_DEBUG("[%s] ret=%d, value[%s] %2X %2X %2X %2X\n", __func__, ret, value, value[0], value[1], value[2], value[3]);
	if( ret == NULL ) 
	{	/* return value of fgets() : s on success, and NULL on error or when end of file occurs while no characters have been read. */
		if(default_value)
		{
			property_set(key, default_value);
	        len = strlen(default_value);
	        memcpy(value, default_value, len + 1);
		}
		fclose(fp);
		return len;
	}
	len = strlen(value);

	/* remove CR&LF of end of string, if property is created by command line(echo) */
	while( len>0 && value[len-1] == 0x0A )
	{
		value[len-1] = 0x00;
		len--;
	}

	fclose(fp);

	return len;
}

/* property_set: returns 0 on success, < 0 on failure
*/
int property_set(const char *key, const char *value)
{
	FILE *fp;
	int iRet;
	char szCmd[256];

	snprintf(szCmd, 256, "%s%s", PROPERTY_PATH, key);
	PRINTF_DEBUG("[%s] fopen[%s]\n", __func__, szCmd );

	fp = fopen(szCmd, "w");
	PRINTF_DEBUG("[%s] result fp=[%p]\n", __func__, fp );

	if( fp == NULL )
	{
		mkdir(PROPERTY_PATH, 0732);
		fp = fopen(szCmd, "w");
		if( fp == NULL )
		{
			PRINTF_ERROR("[%s] Fail to create properties folder [%s], key[%s], value[%s]\n", __func__,PROPERTY_PATH, key, value);
			return -1;
		}
	}

	iRet = fputs(value, fp);
	if( iRet < 0 )
	{
		/* fail */
		PRINTF_ERROR("[%s] Fail to put property key[%s], value[%s]\n", __func__, key, value);
		fclose(fp);
		return -2;
	}
	
	fclose(fp);
	return 0;
}

