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
#include <tcc_dxb_isdbt_cui_common.h>
#include <sqlite3.h>
#include <tcc_dxb_isdbtfseg_cmd_list.h>

/******************************************************************************
* defines
******************************************************************************/
#define MAX_SIZE_OF_NAME 63

/******************************************************************************
* globals
******************************************************************************/
static char *default_isdbt_db_path = "/tccdxb_data";
static char *env_isdbt_db_path = NULL;
static char isdbt_channeldb_path0[128] = {0, };

/******************************************************************************
* locals
******************************************************************************/
sqlite3 *g_pISDBTDB = NULL;


static int tcc_app_db_get_channel_count(void)
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

static int tcc_app_db_get_service_count(void)
{
	int err = SQLITE_ERROR;
	int count = 0;
	sqlite3_stmt *stmt;
	const char *pszSQL = "select count(*) from channels";

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
				if(pText && iSize > 0){
					memcpy(pChannelName, pText, iSize+1);
				}
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
	return 0;
}

/******************************************************************************
*	FUNCTIONS			: void TCC_CUI_Display_Show_Channel_DB(void)
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_CUI_Display_Show_Channel_DB(void)
{
	int i,j;
	int err;
	int num_of_channel;
	int num_of_service;

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
		if(isdbt_channeldb_path0[0] == 0) {
			env_isdbt_db_path = getenv("DXB_DATA_PATH");
			if(env_isdbt_db_path == NULL) {
				env_isdbt_db_path = default_isdbt_db_path;
			}

			memset(isdbt_channeldb_path0, 0x0, 128);
			if( (strlen(env_isdbt_db_path) + strlen("/ISDBT0.db")) < 128 ) // Noah, To avoid Codesonar's warning, Buffer Overrun
				sprintf(isdbt_channeldb_path0, "%s%s", env_isdbt_db_path, "/ISDBT0.db");
		}
		if ((err = sqlite3_open(isdbt_channeldb_path0, &g_pISDBTDB)) != SQLITE_OK) {
			printf("[%s] DB [%s] is not exist.\n\n", __func__, isdbt_channeldb_path0);
			return err;
		}
	}

	num_of_channel = tcc_app_db_get_channel_count();
	num_of_service = tcc_app_db_get_service_count();

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
		
		for(i=1;err==0 || i<=num_of_service;i++)
		{
			err = tcc_app_db_get_channel( i,&iChannelNumber, &i3DigitNum, &iAudioPID, &iVideoPID, &iStPID, &iSiPID,
					&iServiceType, &iServiceID, &iPMTPID, &iNetworkID, &iAudioStreamType, &iVideoStreamType, &iPCRPID, &iTotalAudioCount, &iTotalVideoCount, 
					szChannelname, &iChannelNameLen );

			if(err == 0)
			{
				if(iChannelNameLen > 0){
					printf(" %3d  %3d  %3d  %3d  %5d  %5d  [%s]",
							i, iChannelNumber, i3DigitNum, iServiceType, iServiceID, iPMTPID, szChannelname);
				}else{
					printf(" %3d  %3d  %3d  %3d  %5d  %5d  [not yet parsing]",
							i, iChannelNumber, i3DigitNum, iServiceType, iServiceID, iPMTPID);
				}
				
				if(iChannelNameLen > 0){
					for(j=0; j<=iChannelNameLen;j++ )
					{
						printf(" %02X", szChannelname[j]);
					}
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


/******************************************************************************
*	FUNCTIONS			: TCC_CUI_Display_Media_Volume_Info
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
#if 0
void TCC_CUI_Display_Media_Volume_Info(double volume_level)
{
	if ((volume_level < -0.0) || (volume_level > 1.0))
	{
		printf("############ Set Volume Level 0.0 ~ 1.0 #############\n");
		return;
	}

	printf("############## Volume Information ############  \n");
	printf("#Current Volume Level = %lf \n", volume_level);
	printf("#############################################\n");
}
#endif

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_CUI_Display_GetCurrentDateTime(int *piMJD, int *piHour, int *piMin, int *piSec, int *piPolarity, int *piHourOffset, int *piMinOffset)
{
	printf("############## Date Time Info #################\n");
	printf("# MJD = %d, Time %02d:%02d:%02d\n", *piMJD, *piHour, *piMin, *piSec);
	printf("# iPolarity=%d, iHourOffset=%d, iMinOffset=%d\n", *piPolarity, *piHourOffset, *piMinOffset);
	printf("###############################################\n");
}

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_CUI_Display_IsPlaying(int iRet)
{
	printf("############## Is Playing Info ################\n");
	printf("# Is Playing = %d\n", iRet);
	printf("###############################################\n");
}

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_CUI_Display_GetVideoWidth(int *piWidth)
{
	printf("############## Video Width Info ###############\n");
	printf("# Width = %d\n", *piWidth);
	printf("###############################################\n");
}

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_CUI_Display_GetVideoHeight(int *piHeight)
{
	printf("############## Video Height Info ##############\n");
	printf("# Height = %d\n", *piHeight);
	printf("###############################################\n");
}

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_CUI_Display_GetSignalStrength(int *piSQInfo)
{
	printf("############## Signal Strength Info ###########\n");
	printf("# A Signal Length = %d\n", piSQInfo[1]);
	printf("# B Signal Length = %d\n", piSQInfo[2]);
	printf("###############################################\n");
}

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_CUI_Display_GetCountryCode(int iRet)
{
	printf("############## Country Code Info ##############\n");
	printf("# Country Code = %d\n", iRet);
	printf("###############################################\n");
}

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_CUI_Display_GetAudioCnt(int iRet)
{
	printf("############## Audio Count Info ###############\n");
	printf("# Audio Count = %d\n", iRet);
	printf("###############################################\n");
}

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_CUI_Display_GetChannelInfo(int iChannelNumber, int iServiceID,
									int *piRemoconID, int *piThreeDigitNumber, int *piServiceType,
									unsigned short *pusChannelName, int *piChannelNameLen,
									int *piTotalAudioPID, int *piTotalVideoPID, int *piTotalSubtitleLang,
									int *piAudioMode, int *piVideoMode,
									int *piAudioLang1, int *piAudioLang2,
									int *piStartMJD, int *piStartHH, int *piStartMM, int *piStartSS,
									int *piDurationHH, int *piDurationMM, int *piDurationSS,
									unsigned short *pusEvtName, int *piEvtNameLen,
									int *piLogoImageWidth, int *piLogoImageHeight, unsigned int *pusLogoImage,
									unsigned short *pusSimpleLogo, int *piSimpleLogoLength,
									int *piMVTVGroupType)
{
	printf("############## Channel Info ###################\n");
	printf("# Channel Number = %d\n", iChannelNumber);
	printf("# Service ID = %d\n", iServiceID);
	printf("# Remocon ID = %d\n", *piRemoconID);
	printf("# Three Digit Number = %d\n", *piThreeDigitNumber);
	printf("# Service Type = %d\n", *piServiceType);
	printf("# Channel Name = %s\n", pusChannelName);
	printf("# Total Audio PID = %d\n", *piTotalAudioPID);
	printf("# Total Video PID = %d\n", *piTotalVideoPID);
	printf("# Total Subtitle Language = %d\n", *piTotalSubtitleLang);
	printf("# Audio Mode = %d\n", *piAudioMode);
	printf("# Video Mode = %d\n", *piVideoMode);
	printf("# Audio Language 1 = %d\n", *piAudioLang1);
	printf("# Audio language 2 = %d\n", *piAudioLang2);
	printf("# Start MJD = %d\n", *piStartMJD);
	printf("# Start HH = %d\n", *piStartHH);
	printf("# Start MM = %d\n", *piStartMM);
	printf("# Start SS = %d\n", *piStartSS);
	printf("# Duration HH = %d\n", *piDurationHH);
	printf("# Duration MM = %d\n", *piDurationMM);
	printf("# Duration SS = %d\n", *piDurationSS);
	printf("# Event Name = %s\n", pusEvtName);
	printf("# Logo Image Width = %d\n", *piLogoImageWidth);
	printf("# Logo Image Height = %d\n", *piLogoImageHeight);
	printf("# Simple Logo = %s\n", pusSimpleLogo);
	printf("# MVTV Type = %d\n", *piMVTVGroupType);
	printf("###############################################\n");
}

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_CUI_Display_ReqTRMPInfo(unsigned char **ppucInfo, int *piInfoSize)
{
	printf("############## TRMP Info ######################\n");
	printf("# Size = %d\n", *piInfoSize);
	printf("###############################################\n");
}

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_CUI_Display_GetDuration(int iRet)
{
	printf("############## Durattion Info #################\n");
	printf("# Duration = %d\n", iRet);
	printf("###############################################\n");
}

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_CUI_Display_GetCurrentPosition(int iRet)
{
	printf("############## Current Position Info ##########\n");
	printf("# Current Position = %d\n", iRet);
	printf("###############################################\n");
}

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_CUI_Display_GetCurrentReadPosition(int iRet)
{
	printf("############## Current Read Position Info #####\n");
	printf("# Current Read Position = %d\n", iRet);
	printf("###############################################\n");
}

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_CUI_Display_GetCurrentWritePosition(int iRet)
{
	printf("############## Current Write Position Info ####\n");
	printf("# Current Write Position = %d\n", iRet);
	printf("###############################################\n");
}

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_CUI_Display_GetTotalSize(int iRet)
{
	printf("############## Total Size Info #################\n");
	printf("# Total Size = %d\n", iRet);
	printf("###############################################\n");
}

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_CUI_Display_GetMaxSize(int iRet)
{
	printf("############## Maximum Size Info #################\n");
	printf("# Maximum Size = %d\n", iRet);
	printf("###############################################\n");
}

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_CUI_Display_GetFreeSize(int iRet)
{
	printf("############## Free Size Info #################\n");
	printf("# Free Size = %d\n", iRet);
	printf("###############################################\n");
}

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_CUI_Display_GetBlockSize(int iRet)
{
	printf("############## Block Size Info #################\n");
	printf("# Block Size = %d\n", iRet);
	printf("###############################################\n");
}

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_CUI_Display_GetFSType(char *pcFSType)
{
	printf("############## File System Type Info ##########\n");
	printf("# File System Type = %s\n", pcFSType);
	printf("###############################################\n");
}

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_CUI_Display_GetSubtitleLangInfo(int *piSubtitleLangNum, int *piSuperimposeLangNum, unsigned int *puiSubtitleLang1, unsigned int *puiSubtitleLang2, unsigned int *puiSuperimposeLang1, unsigned int *puiSuperimposeLang2)
{
	printf("############## Subtitle Language Info #########\n");
	printf("# Subtitle Language Number = %d\n", *piSubtitleLangNum);
	printf("# Superimpose Language Number = %d\n", *piSuperimposeLangNum);
	printf("# Subtitle Language 1 = %d\n", *puiSubtitleLang1);
	printf("# Subtitle Language 2 = %d\n", *puiSubtitleLang2);
	printf("# Superimpose Language 1 = %d\n", *puiSuperimposeLang1);
	printf("# Superimpose Language 2 = %d\n", *puiSuperimposeLang2);
	printf("###############################################\n");
}

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_CUI_Display_GetUserData(unsigned int *puiRegionID, unsigned long long *pullPrefecturalCode, unsigned int *puiAreaCode, char *pcPostalCode)
{
	printf("############## User Data Info #################\n");
	printf("# Region Id = %d\n", *puiRegionID);
	printf("# Prefectural Code = %llu\n", *pullPrefecturalCode);
	printf("# Area Code = %d\n", *puiAreaCode);
	printf("# Postal Code = %s\n", pcPostalCode);
	printf("###############################################\n");
}

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_CUI_Display_GetDigitalCopyControlDescriptor(unsigned short usServiceID, unsigned char *pucDigital_recording_control_data, unsigned char *pucMaximum_bitrate_flag, unsigned char *pucComponent_control_flag, unsigned char *pucCopy_control_type, unsigned char *pucAPS_control_data, unsigned char *maximum_bitrate, unsigned char *pucIsAvailable)
{
	printf("############## Digital Copy Control Descriptor#\n");
	printf("# Service ID = %d\n", usServiceID);
	printf("# digital_recording_controldata = %d\n", *pucDigital_recording_control_data);
	printf("# maximum_bitrate_flag = %d\n", *pucMaximum_bitrate_flag);
	printf("# component_control_flag = %d\n", *pucComponent_control_flag);
	printf("# copy_control_type = %d\n", *pucCopy_control_type);
	printf("# APS_control_data = %d\n", *pucAPS_control_data);
	printf("# maximum_bitrate = %d\n", *pucMaximum_bitrate_flag);
	printf("###############################################\n");
}

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_CUI_Display_GetContentAvailabilityDescriptor(unsigned short usServiceID, unsigned char *pucImage_constraint_token, unsigned char *pucRetention_mode, unsigned char *pucRetention_state, unsigned char *pucEncryption_mode, unsigned char *pucIsAvailable)
{
	printf("############## Content Availability Descriptor#\n");
	printf("# Service ID = %d\n", usServiceID);
	printf("# image_contsraint_token = %d\n", *pucImage_constraint_token);
	printf("# retention_mode = %d\n", *pucRetention_mode);
	printf("# retention_state = %d\n", *pucRetention_state);
	printf("# encryption_mode = %d\n", *pucEncryption_mode);
	printf("###############################################\n");
}

char *szEventString[] = {
"EVENT_NOP", //0
"EVENT_PREPARED", //1
"EVENT_SEARCH_COMPLETE", //2
"EVENT_CHANNEL_UPDATE", //3
"EVENT_TUNNING_COMPLETE", //4
"EVENT_SEARCH_PERCENT", //5
"EVENT_VIDEO_OUTPUT", //6
"EVENT_SUPERIMPOSE_UPDATE", //7
"EVENT_RECORDING_COMPLETE", //8
"EVENT_SUBTITLE_UPDATE", //9
"EVENT_VIDEODEFINITION_UPDATE", //10
"EVENT_DEBUGMODE_UPDATE", //11
"EVENT_PLAYING_COMPLETE", //12
"EVENT_PLAYING_CURRENT_TIME", //13
"EVENT_DBINFO_UPDATE", //14
"EVENT_DBINFORMATION_UPDATE", //15
"EVENT_ERROR", //16
"EVENT_PNG_UPDATE", //17
"", //18
"", //19
"", //20
"", //21
"", //22
"", //23
"", //24
"", //25
"", //26
"", //27
"", //28
"", //29
"EVENT_EPG_UPDATE", //30
"EVENT_PARENTLOCK_COMPLETE", //31
"EVENT_HANDOVER_CHANNEL", //32
"EVENT_EMERGENCY_INFO", //33
"EVENT_SMARTCARD_ERROR", //34
"EVENT_SMARTCARDINFO_UPDATE", //35
"EVENT_AUTOSEARCH_UPDATE", //36
"EVENT_AUTOSEARCH_DONE", //37
"EVENT_DESC_UPDATE", //38
"EVENT_EVENT_RELAY", //39
"EVENT_MVTV_UPDATE", //40
"EVENT_SERVICEID_CHECK", //41
"EVENT_AUTOMESSAGE_UPDATE", //42
"EVENT_TRMP_ERROR", //43
"EVENT_CUSTOMSEARCH_STATUS", //44
"EVENT_SPECIALSERVICE_UPDATE", //45
"EVENT_UPDATE_CUSTOM_TUNER", //46
"EVENT_EPGSEARCH_COMPLETE", //47
"EVENT_AV_INDEX_UPDATE", //48
"EVENT_SECTION_UPDATE", //49
"EVENT_SEARCH_AND_SETCHANNEL", //50
"" // last
};

/******************************************************************************
*	FUNCTIONS			: 
*	SYNOPSIS			:
*	EXTERNAL EFFECTS	:
*	PARAMETERS			:
*	RETURNS				:
*	ERRNO				:
******************************************************************************/
void TCC_CUI_Display_EventNotify(int msg, int ext1, int ext2, int objsize, char *obj)
{
	int i; //index of array

	//printf("[CUI_EventNotify] %s, ext1=%d, ext2=%d, objsize=%d, obj=%p\n", szEventString[msg], ext1, ext2, objsize, obj);

	switch(msg)
	{
		case EVENT_NOP :
		{
			break;
		}
		case EVENT_PREPARED :
		{
			printf("[CUI_EventNotify] EVENT_PREPARED, specific_info=%08X\n", ext1);
			break;
		}
		case EVENT_SEARCH_COMPLETE :
		{
			printf("[CUI_EventNotify] EVENT_SEARCH_COMPLETE, search_result=%d\n", ext1);
			TCC_CUI_Display_Show_Channel_DB();
			break;
		}
		case EVENT_CHANNEL_UPDATE :
		{
			printf("[CUI_EventNotify] EVENT_CHANNEL_UPDATE, request_ch_update=%d\n", ext1);
			break;
		}
		case EVENT_TUNNING_COMPLETE :
		{
			break;
		}
		case EVENT_SEARCH_PERCENT :
		{
			printf("[CUI_EventNotify] EVENT_SEARCH_PERCENT, progress=%d%, channel=%d\n", ext1, ext2);
			break;
		}
		case EVENT_VIDEO_OUTPUT :
		{
			printf("[CUI_EventNotify] EVENT_VIDEO_OUTPUT, channel=%d\n", ext1);
			break;
		}
		case EVENT_RECORDING_COMPLETE :
		{
			printf("[CUI_EventNotify] EVENT_RECORDING_COMPLETE, record_result=%d\n", ext1);
			break;
		}
		case EVENT_SUBTITLE_UPDATE :
		{
/*
			SUBTITLE_DATA_t *pstSubtitleData = (SUBTITLE_DATA_t *)obj;
			objsize = sizeof(SUBTITLE_DATA_t);
			printf("[CUI_EventNotify] EVENT_SUBTITLE_UPDATE, ext1=%d, ext2=%d, objsize==%d\n", ext1, ext2, objsize);
*/
 			break;
		}		
		case EVENT_VIDEODEFINITION_UPDATE :
		{
			VIDEO_DEFINITION_DATA_t *pstVideoDefinitionData = (VIDEO_DEFINITION_DATA_t *)obj;
			objsize = sizeof(VIDEO_DEFINITION_DATA_t);
			printf("[CUI_EventNotify] EVENT_VIDEODEFINITION_UPDATE, ext1=%d, ext2=%d, objsize==%d\n", ext1, ext2, objsize);
			printf("width[%d] height[%d] aspect_ratio[%d] frame_rate[%d] main_DecoderID[%d]\n", pstVideoDefinitionData->width, pstVideoDefinitionData->height, pstVideoDefinitionData->aspect_ratio, pstVideoDefinitionData->frame_rate, pstVideoDefinitionData->main_DecoderID);
			printf("sub_width[%d] sub_height[%d] sub_aspect_ratio[%d] sub_DecoderID[%d]\n", pstVideoDefinitionData->sub_width, pstVideoDefinitionData->sub_height, pstVideoDefinitionData->sub_aspect_ratio, pstVideoDefinitionData->sub_DecoderID);
			printf("DisplayID[%d]\n", pstVideoDefinitionData->DisplayID);
			break;
		}
		case EVENT_DEBUGMODE_UPDATE :
		{
			break;
		}
		case EVENT_PLAYING_COMPLETE :
		{
			printf("[CUI_EventNotify] EVENT_PLAYING_COMPLETE, result=%d\n", ext1);
			break;
		}
		case EVENT_PLAYING_CURRENT_TIME :
		{
			printf("[CUI_EventNotify] EVENT_PLAYING_CURRENT_TIME, current_play_time=%d\n", ext1);
			break;
		}
		case EVENT_DBINFO_UPDATE :
		{
			/*
			objsize = sizeof(DBInfoData);
			*/
			printf("[CUI_EventNotify] EVENT_DBINFO_UPDATE, ext1=%d, ext2=%d, objsize==%d\n", ext1, ext2, objsize);
			break;
		}
		case EVENT_DBINFORMATION_UPDATE :
		{
			break;
		}
		case EVENT_ERROR :
		{
			break;
		}
		case EVENT_EPG_UPDATE:
		{
			printf("[CUI_EventNotify] EVENT_EPG_UPDATE, ServiceID=%d(0x%04X), TableID=%d\n", ext1, ext1, ext2 );
			break;
		}
		case EVENT_PARENTLOCK_COMPLETE :
		{
			break;
		}
		case EVENT_HANDOVER_CHANNEL :
		{
			printf("[CUI_EventNotify] EVENT_HANDOVER_CHANNEL, SearchAffiliation=%d%, Channel=%d\n", ext1, ext2 );
			break;
		}
		case EVENT_EMERGENCY_INFO :
		{
			EWS_DATA_t *pstEWSData = (EWS_DATA_t *)obj;
			objsize = sizeof(EWS_DATA_t);
			printf("[CUI_EventNotify] EVENT_EMERGENCY_INFO, start/stop=%d, ext2=%d, objsize==%d\n", ext1, ext2, objsize);
			printf("serviceID[%d] startendflag[%d] signaltype[%d] area_code_length[%d]\n", pstEWSData->serviceID, pstEWSData->startendflag, pstEWSData->signaltype, pstEWSData->area_code_length);
			for(i=0;i<pstEWSData->area_code_length;i++)
				printf("localcode[%d]\n", pstEWSData->localcode[i]);
			break;
		}
		case EVENT_SMARTCARD_ERROR :
		{
			printf("[CUI_EventNotify] EVENT_SMARTCARD_ERROR, smartcard_error_number=%d\n", ext1);
			break;
		}
		case EVENT_SMARTCARDINFO_UPDATE :
		{
			SC_INFO_t *pstSCInfo = (SC_INFO_t *)obj;
			objsize = sizeof(SC_INFO_t);
			printf("[CUI_EventNotify] EVENT_SMARTCARDINFO_UPDATE, ext1=%d, ext2=%d, objsize==%d\n", ext1, ext2, objsize);
			printf("SCDataSize[%d]\n", pstSCInfo->SCDataSize);
			break;
		}
		case EVENT_AUTOSEARCH_UPDATE :
		{
			int *piAutoSearchEvt = (int *)obj;
			objsize = sizeof(int)*2;
			printf("[CUI_EventNotify] EVENT_AUTOSEARCH_UPDATE, status=%d, percent=%d, objsize==%d\n", ext1, ext2, objsize);
			printf("AutoSearchEvt1[%d] AutoSearchEvt2[%d]\n", piAutoSearchEvt[0], piAutoSearchEvt[1]);
			break;
		}
		case EVENT_AUTOSEARCH_DONE :
		{
			int *piArraySvcRowID = (int *)obj;
			objsize = sizeof(int)*(32+4);
			printf("[CUI_EventNotify] EVENT_AUTOSEARCH_UPDATE, ArraySvcRowID[0]=%d, ext2=%d, objsize==%d\n", ext1, ext2, objsize);
			for(i=0;i<36;i++)
				printf("ArraySvcRowID[%d]=%d\n", i, piArraySvcRowID[i]);
			break;
		}
		case EVENT_DESC_UPDATE :
		{
			if( ext2 == 0xc1 )
			{
				DIGITAL_COPY_CONTROL_DESCR_t *pstDCCD = (DIGITAL_COPY_CONTROL_DESCR_t *)obj;
				objsize = sizeof(DIGITAL_COPY_CONTROL_DESCR_t);
				printf("[CUI_EventNotify] EVENT_DESC_UPDATE, ServiceID=%d, TableID=%d, objsize==%d\n", ext1, ext2, objsize);
				printf("digital_recording_control_dat[%d] maximum_biterate_flag[%d]\n", pstDCCD->digital_recording_control_data, pstDCCD->maximum_bitrate_flag);
				printf("component_control_flag[%d] copy_control_type[%d]\n", pstDCCD->component_control_flag, pstDCCD->copy_control_type);
				printf("APS_control_data[%d] maximum_bitrate[%d]\n", pstDCCD->APS_control_data, pstDCCD->maximum_bitrate);
			}
			else if ( ext2 == 0xde )
			{
				CONTENT_AVAILABILITY_DESCR_t *pstCAD = (CONTENT_AVAILABILITY_DESCR_t *)obj;
				objsize = sizeof(CONTENT_AVAILABILITY_DESCR_t);
				printf("[CUI_EventNotify] EVENT_DESC_UPDATE, ServiceID=%d, TableID=%d, objsize==%d\n", ext1, ext2, objsize);
				printf("copy_restriction_mode[%d] image_constraint_token[%d]\n", pstCAD->copy_restriction_mode, pstCAD->image_constraint_token);
				printf("retention_mode[%d] retention_state[%d]\n", pstCAD->retention_mode, pstCAD->retention_state);
			}
			break;
		}
		case EVENT_EVENT_RELAY :
		{
			int *piEventRelayData = (int *)obj;
			objsize = sizeof(int)*(4);
			printf("[CUI_EventNotify] EVENT_EVENT_RELAY, ext1=%d, ext2=%d, objsize==%d\n", ext1, ext2, objsize);
			printf("serviceID[%d] EventID[%d] OriginalNetworkID[%d] TransportStreamID[%d]\n", piEventRelayData[0], piEventRelayData[1], piEventRelayData[2], piEventRelayData[3]);
			break;
		}
		case EVENT_MVTV_UPDATE :
		{
			int *piMVTVData = (int *)obj;
			objsize = sizeof(int)*(5);
			printf("[CUI_EventNotify] EVENT_MVTV_UPDATE, ext1=%d, ext2=%d, objsize==%d\n", ext1, ext2, objsize);
			printf("component_group_type[%d] num_of_group[%d] main_group_id[%d] sub1_group_id[%d] sub2_group_id[%d]\n", piMVTVData[0], piMVTVData[1], piMVTVData[2], piMVTVData[3], piMVTVData[4]);
			break;
		}
		case EVENT_SERVICEID_CHECK :
		{
			printf("[CUI_EventNotify] EVENT_SERVICEID_CHECK, valid_serviceID_flag=%d\n", ext1);
			break;
		}
		case EVENT_AUTOMESSAGE_UPDATE :
		{
			AUTO_MESSAGE_INFO_t *pstAutoMessageInfo = (AUTO_MESSAGE_INFO_t *)obj;
			objsize = sizeof(AUTO_MESSAGE_INFO_t);
			printf("[CUI_EventNotify] EVENT_AUTOMESSAGE_UPDATE, ext1=%d, ext2=%d, objsize==%d\n", ext1, ext2, objsize);
			printf("MessageID[%d] Broadcaster_GroupID[%d] Deletion_Status[%d]\n", pstAutoMessageInfo->MessageID, pstAutoMessageInfo->Broadcaster_GroupID, pstAutoMessageInfo->Deletion_Status);
			printf("Disp_Duration1[%d] Disp_Duration2[%d] Disp_Duration3[%d]\n", pstAutoMessageInfo->Disp_Duration1, pstAutoMessageInfo->Disp_Duration2, pstAutoMessageInfo->Disp_Duration3);
			printf("Disp_Cycle[%d] Format_Version[%d] MessageLen[%d]\n", pstAutoMessageInfo->Disp_Cycle, pstAutoMessageInfo->Format_Version, pstAutoMessageInfo->MessageLen);
			printf("Disp_Horizontal_PositionInfo[%d] Disp_Vertical_PositionInfo[%d]\n", pstAutoMessageInfo->Disp_Horizontal_PositionInfo, pstAutoMessageInfo->Disp_Vertical_PositionInfo);
			break;
		}
		case EVENT_TRMP_ERROR :
		{
			printf("[CUI_EventNotify] EVENT_TRMP_ERROR, TRMP_error_code=%d\n", ext1);
			break;
		}
		case EVENT_CUSTOMSEARCH_STATUS :
		{
			CUSTOM_SEARCH_INFO_DATA_t *pstCustomSearchInfoData = (CUSTOM_SEARCH_INFO_DATA_t *)obj;
			objsize = sizeof(CUSTOM_SEARCH_INFO_DATA_t);
			printf("[CUI_EventNotify] EVENT_CUSTOMSEARCH_STATUS, status=%d, ext2=%d, objsize==%d\n", ext1, ext2, objsize);
			printf("status[%d] ch[%d] strength[%d] fullseg_id[%d] oneseg_id[%d] total_cnt[%d]\n", pstCustomSearchInfoData->status, pstCustomSearchInfoData->ch, pstCustomSearchInfoData->strength, pstCustomSearchInfoData->fullseg_id, pstCustomSearchInfoData->oneseg_id, pstCustomSearchInfoData->total_cnt);
			for(i=0;i<pstCustomSearchInfoData->total_cnt;i++)
				printf("all_id[%d]=%d\n", i, pstCustomSearchInfoData->all_id[i]);
			break;
		}
		case EVENT_SPECIALSERVICE_UPDATE :
		{
			printf("[CUI_EventNotify] EVENT_SPECIALSERVICE_UPDATE, status=%d, RowID=%d\n", ext1, ext2);
			break;
		}
		case EVENT_UPDATE_CUSTOM_TUNER :
		{
			printf("[CUI_EventNotify] EVENT_UPDATE_CUSTOM_TUNER, ext1=%d, ext2=%d\n", ext1, ext2);
			break;
		}
		case EVENT_EPGSEARCH_COMPLETE :
		{
			printf("[CUI_EventNotify] EVENT_EPGSEARCH_COMPLETE, result=%d\n", ext1);
			break;
		}
		case EVENT_AV_INDEX_UPDATE :
		{
			AV_INDEX_DATA_t *pstAVIndexData = (AV_INDEX_DATA_t *)obj;
			objsize = sizeof(AV_INDEX_DATA_t);
			printf("[CUI_EventNotify] EVENT_AV_INDEX_UPDATE, ext1=%d, ext2=%d, objsize=%d\n", ext1, ext2, objsize);
			printf("channelNumber[%d] networkID[%d] serviceID[%d] serviceSubID[%d]\n", pstAVIndexData->channelNumber, pstAVIndexData->networkID, pstAVIndexData->serviceID, pstAVIndexData->serviceSubID);
			printf("audioIndex[%d] videoIndex[%d]\n", pstAVIndexData->audioIndex, pstAVIndexData->videoIndex);
			break;
		}
		case EVENT_SECTION_UPDATE :
		{
			printf("[CUI_EventNotify] EVENT_SECTION_UPDATE, channelNumber=%d, sectionType=%d\n", ext1, ext2);
			break;
		}
		case EVENT_SEARCH_AND_SETCHANNEL :
		{
			SET_CHANNEL_DATA_t *pstSetChannelData = (SET_CHANNEL_DATA_t *)obj;
			objsize = sizeof(SET_CHANNEL_DATA_t);
			printf("[CUI_EventNotify] EVENT_SEARCH_AND_SETCHANNEL, status=%d, objsize=%d\n", ext1, objsize);
			printf("countryCode[%d] channelNumber[%d] mainRowID[%d] subRowId[%d]\n", pstSetChannelData->countryCode, pstSetChannelData->channelNumber, pstSetChannelData->mainRowID, pstSetChannelData->subRowID);
			break;
		}
		case EVENT_DETECT_SERVICE_NUM_CHANGE :
		{
			printf("[CUI_EventNotify] EVENT_DETECT_SERVICE_NUM_CHANGE\n");
			break;
		}
		default:
		{
			printf("[CUI_EventNotify] message received msg=%d, ext1=%d, ext2=%d, obj=%p\n", msg, ext1, ext2, obj);
			objsize = 0;
			obj = NULL;
			break;
		}
	}
}

/* end of file */
