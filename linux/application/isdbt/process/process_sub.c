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
#include <stdio.h>
#include <sqlite3.h>
//#include <locale.h>
//#include <wchar.h>
#include <process_sub.h>

#if 0

#define ISDBT_DB_PATH	"/tccdxb_data"
#define ISDBT_CHANNELDB_PATH0	ISDBT_DB_PATH"/ISDBT0.db"
sqlite3 *g_pISDBTDB = NULL;
#define MAX_SIZE_OF_NAME 63


int tcc_app_db_get_channel_count(void)
{
	int err = SQLITE_ERROR;
	int count = 0;
	sqlite3_stmt *stmt;
	const char *pszSQL = "select count(distinct channelNumber) from channels";

	if (g_pISDBTDB != NULL)
	{
		if (sqlite3_prepare_v2(g_pISDBTDB, pszSQL, strlen(pszSQL), &stmt, NULL) == SQLITE_OK){
			if ((err = sqlite3_step(stmt)) == SQLITE_ROW){
				count = sqlite3_column_int(stmt, 0);
			}
			if ((err != SQLITE_ROW) && (err != SQLITE_DONE)){
				printf("[%s:%d] err:%d\n", __func__, __LINE__, err);
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
			stmt = NULL;
		}
	}
	
	return count;
}

int tcc_app_db_get_channel(int iRowID, \
						int *piChannelNumber, int *pi3DigitNum,\
						int *piAudioPID, int *piVideoPID, \
						int *piStPID, int *piSiPID, \
						int *piServiceType, int *piServiceID, \
						int *piPMTPID, int *piNetworkID, \
						int *piAudioStreamType, int *piVideoStreamType, \
						int *piPCRPID, int *piTotalAudioCount, int *piTotalVideoCount,
						unsigned char *pChannelName, int *piChannelNameLen )
{
	int iPreset=-1;
	char szSQL[1024] = { 0 };
	sqlite3_stmt *stmt;
	int rc = SQLITE_ERROR;
	int i;
	unsigned char *pText;

	sprintf(szSQL, "SELECT channelNumber, audioPID, videoPID, stPID, siPID, serviceType, serviceID, PMT_PID, networkID, preset, threedigitNumber, channelName FROM channels WHERE _id=%d", iRowID);
	//tcc_dxb_channel_db_lock_on();
	if(sqlite3_prepare_v2(g_pISDBTDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK)
	{
		for(i=0; i<5; i++)
		{
			rc = sqlite3_step(stmt);
			if (rc == SQLITE_ROW)
			{
				*piChannelNumber = sqlite3_column_int(stmt, 0);
				*piAudioPID = sqlite3_column_int(stmt, 1);
				*piVideoPID = sqlite3_column_int(stmt, 2);
				*piStPID = sqlite3_column_int(stmt, 3);
				*piSiPID = sqlite3_column_int(stmt, 4);
				*piServiceType = sqlite3_column_int(stmt, 5);
				*piServiceID = sqlite3_column_int(stmt, 6);
				*piPMTPID = sqlite3_column_int(stmt, 7);
				*piNetworkID = sqlite3_column_int(stmt, 8);
				iPreset = sqlite3_column_int(stmt, 9);
				*pi3DigitNum = sqlite3_column_int(stmt, 10);

				unsigned char *pText;
				int iSize;
				iSize = sqlite3_column_bytes(stmt, 11);
				pText = sqlite3_column_text(stmt, 11);
				*piChannelNameLen = iSize;
				memcpy(pChannelName, pText, iSize+1);
				break;
			}

			//printf("[%d]Retry i=%d, sqlite3_step rc=%d\n", __LINE__, i, rc);
			usleep(5000);	
		}
	}
	if (stmt) {
		sqlite3_finalize(stmt);
		stmt = NULL;
	}
	//tcc_dxb_channel_db_lock_off();

	if (rc != SQLITE_ROW || iPreset == 1)
	{
		return -1;
	}

	sprintf(szSQL, "SELECT ucAudio_StreamType, ucVideo_StreamType, uiPCR_PID, uiTotalAudio_PID, uiTotalVideo_PID  FROM services WHERE usServiceID=%d AND uiCurrentChannelNumber=%d", *piServiceID, *piChannelNumber);
	//tcc_dxb_channel_db_lock_on();
	if(sqlite3_prepare_v2(g_pISDBTDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK)
	{
		for(i=0; i<5; i++)
		{
			rc = sqlite3_step(stmt);
			if (rc == SQLITE_ROW)
			{
				*piAudioStreamType = sqlite3_column_int(stmt, 0);
				*piVideoStreamType = sqlite3_column_int(stmt, 1);
				*piPCRPID = sqlite3_column_int(stmt, 2);
				*piTotalAudioCount = sqlite3_column_int(stmt, 3);
				*piTotalVideoCount = sqlite3_column_int(stmt, 4);

				break;
			}

			//printf("[%d]Retry i=%d, sqlite3_step rc=%d/n", __LINE__, i, rc);
			usleep(5000);
		}
	}
	if (stmt) {
		sqlite3_finalize(stmt);
		stmt = NULL;
	}
	//tcc_dxb_channel_db_lock_off();

	return 0;
}

int process_show_channelDB(void)
{
	int i,j;
	int err;
	int num_of_channel;

	int iChannelNumber;
	int i3DigitNum;
	int iAudioPID;
	int iVideoPID;
	int iStPID;
	int iSiPID;
	int iServiceType;
	int iServiceID;
	int iPMTPID;
	int iNetworkID;
	int iAudioStreamType;
	int iVideoStreamType;
	int iPCRPID;
	int iTotalAudioCount;
	int iTotalVideoCount;
	unsigned char szChannelname[MAX_SIZE_OF_NAME+1];
	int iChannelNameLen;

	if ( g_pISDBTDB == NULL)
	{
		if ((err = sqlite3_open(ISDBT_CHANNELDB_PATH0, &g_pISDBTDB)) != SQLITE_OK) {
			printf("[%s] DB [%s] is not exist.\n\n", __func__, ISDBT_CHANNELDB_PATH0);
			return err;
		}
	}

	num_of_channel = tcc_app_db_get_channel_count();

	printf("===================================================================================\n");
	printf(" PROGRAM LIST  [Number of Channels = %d]\n", num_of_channel);
	printf("===================================================================================\n");

	if( num_of_channel > 0 )
	{
		printf(" Row  Ch   3Dg  Svc   Svc    PMT   Prog\n");
		printf(" ID   Num  Num  Typ   ID     PID   Name\n");
		printf("-----------------------------------------------------------------------------------\n");
	/*	printf(" 000  000  000  000  00000  00000 [%s]\n"); */
	/*	printf("-----------------------------------------------------------------------------------\n");	*/

		err = 0;
		for(i=1;err==0;i++)
		{
			err = tcc_app_db_get_channel( i,&iChannelNumber, &i3DigitNum, &iAudioPID, &iVideoPID, &iStPID, &iSiPID,
					&iServiceType, &iServiceID, &iPMTPID, &iNetworkID, &iAudioStreamType, &iVideoStreamType, &iPCRPID, &iTotalAudioCount, &iTotalVideoCount, 
					szChannelname, &iChannelNameLen );

			if(err == 0)
			{
				printf(" %3d  %3d  %3d  %3d  %5d  %5d  [%s]",
						i, iChannelNumber, i3DigitNum, iServiceType, iServiceID, iPMTPID, szChannelname);
				for(j=0; j<=iChannelNameLen;j++ )
				{
					printf(" %02X", szChannelname[j]);
				}
				printf("\n");
			}
		}
	}
	else
	{
		printf("No programs. \n");
	}
	printf("===================================================================================\n");
	printf("\n");
	printf("\n");

	sqlite3_close(g_pISDBTDB);
	g_pISDBTDB = NULL;
}


#endif
