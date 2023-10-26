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
#if defined(HAVE_LINUX_PLATFORM)
#define LOG_TAG "TSPARSE_DB"
#include <utils/Log.h>
#include <sys/stat.h>
#include <stdlib.h>

#define DB_DBG		//ALOGE
#endif

#include <TsParse_ISDBT.h>
#include "TsParse_ISDBT_DBLayer.h"
#include "TsParse_ISDBT_DBInterface.h"
#include "TsParse_ISDBT_EPGDb.h"
#include "TsParse_ISDBT_ChannelDb.h"
#include "tcc_isdbt_manager_demux.h"
#include "subtitle_display.h"
#include "ISDBT_Region.h"
#include "ISDBT_Common.h"
#include "ISDBT_EPG.h"
#include "tcc_isdbt_event.h"
#include "subtitle_draw.h"

#include "TsParse_ISDBT_MailDb.h"

#define PRESET_DBG	//ALOGE

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

/****************************************************************************
DEFINITION
****************************************************************************/
#if defined(HAVE_LINUX_PLATFORM)
#define ISDBT_DB_PATH_SIZE 128
static char *isdbt_db_path = NULL;
static char *default_isdbt_db_path = "/tccdxb_data";
static char isdbt_channeldb_path0[ISDBT_DB_PATH_SIZE] = {0, };
static char isdbt_channeldb_path1[ISDBT_DB_PATH_SIZE] = {0, };
static char isdbt_channeldb_path2[ISDBT_DB_PATH_SIZE] = {0, };
static char isdbt_epgdb_path0[ISDBT_DB_PATH_SIZE] = {0, };
static char isdbt_epgdb_path1[ISDBT_DB_PATH_SIZE] = {0, };
static char isdbt_epgdb_path2[ISDBT_DB_PATH_SIZE] = {0, };
static char isdbt_logodb_path[ISDBT_DB_PATH_SIZE] = {0, };
static char isdbt_maildb_path[ISDBT_DB_PATH_SIZE] = {0, };
static char isdbt_channeldb_backup_path0[ISDBT_DB_PATH_SIZE] = {0, };
static char isdbt_dfsdb_path[ISDBT_DB_PATH_SIZE] = {0, };
#endif

#define	ISDBT_PRESET_MODE_MAX	3

#define ISDBT_SQLITE_BUSY_TIMEOUT 200

/****************************************************************************
DEFINITION OF EXTERNAL VARIABLES
****************************************************************************/
extern int gCurrentServiceID, gCurrentChannel, gCurrentFrequency, gCurrentCountryCode;

/****************************************************************************
DEFINITION OF EXTERNAL FUNCTIONS
****************************************************************************/

/****************************************************************************
DEFINITION OF GLOVAL VARIABLES
****************************************************************************/
static int gCurrentNITAreaCode = 0;
static DATE_TIME_STRUCT gstCurrentDateTime = {0, {0, 0, 0}};
static LOCAL_TIME_OFFSET_STRUCT gstCurrentLocalTimeOffset;

static long long g_llSYSTimeOfTOTArrival; // msec

static int gCurrentUserAreaCode = 0;
static int gCurrentUserLanguage = 0;
static sqlite3 *g_pISDBTDB = NULL;
static sqlite3 *g_pISDBTDB_Handle[3] = { NULL, NULL, NULL};
static sqlite3 *g_pISDBTEPGDB = NULL;
static sqlite3 *g_pISDBTEPGDB_Handle[3] = { NULL, NULL, NULL};
static sqlite3 *g_pISDBTLogoDB = NULL;
static int gCurrentDBIndex = -1;
static int gCurrentEPGDBIndex = -1;

static sqlite3 *g_pISDBTMailDB = NULL;

static sqlite3 *g_pISDBTDB_BackUp_Handle = NULL;
static sqlite3 *g_pISDBTDfsDB = NULL;


#if defined(HAVE_LINUX_PLATFORM)
static char *sISDBTChDBPath[ISDBT_PRESET_MODE_MAX] = {NULL, };
static char *sISDBTEPGDBPath[ISDBT_PRESET_MODE_MAX] = {NULL, };
#endif

unsigned int g_nHowMany_FsegServices = 0;    // IM478A-21, Multi or Common Event
unsigned int g_nServiceID_FsegServices[16] = { 0, };    // IM478A-21, Multi or Common Event, 16 is temp.

/*------------------ mutex -----------------*/
static pthread_mutex_t lockDxbChDB;
static pthread_mutex_t lockDxbEpgDB;
static pthread_mutex_t lockDxbSeamlessDB;

void tcc_dxb_channel_db_lock_init(void)
{
	pthread_mutex_init(&lockDxbChDB, NULL);
}

void tcc_dxb_channel_db_lock_deinit(void)
{
	pthread_mutex_destroy(&lockDxbChDB);
}

void tcc_dxb_channel_db_lock_on(void)
{
	pthread_mutex_lock(&lockDxbChDB);
}

void tcc_dxb_channel_db_lock_off(void)
{
	pthread_mutex_unlock(&lockDxbChDB);
}

void tcc_dxb_epg_db_lock_init(void)
{
	pthread_mutex_init(&lockDxbEpgDB, NULL);
}

void tcc_dxb_epg_db_lock_deinit(void)
{
	pthread_mutex_destroy(&lockDxbEpgDB);
}

void tcc_dxb_epg_db_lock_on(void)
{
	pthread_mutex_lock(&lockDxbEpgDB);
}

void tcc_dxb_epg_db_lock_off(void)
{
	pthread_mutex_unlock(&lockDxbEpgDB);
}

void tcc_dxb_seamless_db_lock_init(void)
{
	pthread_mutex_init(&lockDxbSeamlessDB, NULL);
}

void tcc_dxb_seamless_db_lock_deinit(void)
{
	pthread_mutex_destroy(&lockDxbSeamlessDB);
}

void tcc_dxb_seamless_db_lock_on(void)
{
	pthread_mutex_lock(&lockDxbSeamlessDB);
}

void tcc_dxb_seamless_db_lock_off(void)
{
	pthread_mutex_unlock(&lockDxbSeamlessDB);
}


void Set_User_Area_Code(int area)
{
	gCurrentUserAreaCode = area;
}

int Get_User_Area_Code(void)
{
	return gCurrentUserAreaCode;
}

void Set_NIT_Area_Code(int area)
{
	gCurrentNITAreaCode = area;
}

int Get_NIT_Area_Code(void)
{
	return gCurrentNITAreaCode;
}

void Set_Language_Code(int lang)
{
	gCurrentUserLanguage = lang;
}

int Get_Language_Code(void)
{
	return gCurrentUserLanguage;
}

/*--------------------- SQLITE --------------*/
int TCC_SQLITE3_EXEC(
	sqlite3* h,							/* An open database */
	const char *sql,                           		/* SQL to be evaluated */
	int (*callback)(void*, int, char**, char**),  	/* Callback function */
	void *arg,                                    		/* 1st argument to callback */
	char **errmsg                              		/* Error msg written here */
)
{
	int ret, i;
	char *errmsgTemp = NULL;

	ret = sqlite3_exec(h, sql, callback, arg, &errmsgTemp);
	if(errmsg == NULL && errmsgTemp != NULL) {
		ALOGE("%s:%d %s", __func__, __LINE__, errmsgTemp);
		sqlite3_free(errmsgTemp);
	}
	if(errmsg != NULL)
		*errmsg = errmsgTemp;
	return ret;
}

int backupDb(
	sqlite3 *pDb,               /* Backup source */
	sqlite3 *pBackUp_Db,               /* Backup source */
	const char *zFilename,      /* Name of file to back up to */
	const char *zBackupFilename,      /* Name of file to back up to */
	int	backup_status		/*"0" :back-up, "1" :recover */
){
	int rc= SQLITE_OK;                     /* Function return code */
	sqlite3_backup *pBackup;    /* Backup handle used to copy data */

	/* Open the database file identified by zFilename. */

	if(pBackUp_Db == NULL)
		rc = sqlite3_open_v2(zBackupFilename, &pBackUp_Db, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE|SQLITE_OPEN_FULLMUTEX, NULL);
	if(backup_status)
		rc = sqlite3_open_v2(zFilename, &pDb, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE|SQLITE_OPEN_FULLMUTEX, NULL);

	if( rc==SQLITE_OK ){
		/* Open the sqlite3_backup object used to accomplish the transfer */
		sqlite3_busy_timeout(pBackUp_Db, ISDBT_SQLITE_BUSY_TIMEOUT);
		sqlite3_busy_timeout(pDb, ISDBT_SQLITE_BUSY_TIMEOUT);
		pBackup = sqlite3_backup_init(pBackUp_Db, "main", pDb, "main");
		if( pBackup ){
			do
			{
				rc = sqlite3_backup_step(pBackup, -1);
				if( rc==SQLITE_OK || rc==SQLITE_BUSY || rc==SQLITE_LOCKED ){
					sqlite3_sleep(250);
				}
			}
			while(rc==SQLITE_OK || rc==SQLITE_BUSY || rc==SQLITE_LOCKED );
			(void)sqlite3_backup_finish(pBackup);
		}
		rc = sqlite3_errcode(pBackUp_Db);
	}

	/* Close the database connection opened on database file zFilename
	** and return the result of this function. */

	if(!backup_status)
		(void)sqlite3_close(pBackUp_Db);

	if(backup_status)
		rc = sqlite3_close(pDb);

	return rc;
}

int	ChannelDB_BackUp(void)
{
	int rc =0;

	if(g_pISDBTDB_Handle[gCurrentDBIndex] != NULL) {
#if defined(HAVE_LINUX_PLATFORM)
		if(isdbt_channeldb_backup_path0[0] == 0) {
			isdbt_db_path = getenv("DXB_DATA_PATH");
			if(isdbt_db_path == NULL) {
				isdbt_db_path = default_isdbt_db_path;
			}

			memset(isdbt_channeldb_backup_path0, 0x0, ISDBT_DB_PATH_SIZE);
			if( (strlen(isdbt_db_path) + strlen("/ISDBT0_BackUp.db")) < ISDBT_DB_PATH_SIZE ) // Noah, To avoid Codesonar's warning, Buffer Overrun
				sprintf(isdbt_channeldb_backup_path0, "%s%s", isdbt_db_path, "/ISDBT0_BackUp.db");
		}
		rc = backupDb(g_pISDBTDB_Handle[gCurrentDBIndex], g_pISDBTDB_BackUp_Handle, sISDBTChDBPath[gCurrentDBIndex], isdbt_channeldb_backup_path0, 0);
#endif
	}
	return rc;
}

int	ChannelDB_Recover(void)
{
	int rc =0;
#if defined(HAVE_LINUX_PLATFORM)
	if(isdbt_channeldb_backup_path0[0] == 0) {
		isdbt_db_path = getenv("DXB_DATA_PATH");
		if(isdbt_db_path == NULL) {
			isdbt_db_path = default_isdbt_db_path;
		}

		memset(isdbt_channeldb_backup_path0, 0x0, ISDBT_DB_PATH_SIZE);
		if( (strlen(isdbt_db_path) + strlen("/ISDBT0_BackUp.db")) < ISDBT_DB_PATH_SIZE ) // Noah, To avoid Codesonar's warning, Buffer Overrun
			sprintf(isdbt_channeldb_backup_path0, "%s%s", isdbt_db_path, "/ISDBT0_BackUp.db");
	}
	rc = backupDb(g_pISDBTDB_BackUp_Handle, g_pISDBTDB_Handle[gCurrentDBIndex], isdbt_channeldb_backup_path0, sISDBTChDBPath[gCurrentDBIndex], 1);
#endif
	return rc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Date/Time
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int UpdateDB_TDTTime(DATE_TIME_STRUCT *pstDateTime)
{
	if (pstDateTime){
#if 0
	LOGE(">>>>>>>>> %s [TDT] (%d - %d:%d:%d)", __func__, \
		 pstDateTime->uiMJD, pstDateTime->stTime.ucHour,
		 pstDateTime->stTime.ucMinute, pstDateTime->stTime.ucSecond);
#endif /* 0 */
		memcpy(&gstCurrentDateTime, pstDateTime, sizeof(DATE_TIME_STRUCT));
}

	return 0;
}

int UpdateDB_TOTTime(DATE_TIME_STRUCT *pstDateTime, LOCAL_TIME_OFFSET_STRUCT *pstLocalTimeOffset)
{
	struct timespec systimespec;

	if (pstDateTime){
#if 0
	LOGE(">>>>>>>>> %s [TDT] (%d - %d:%d:%d)", __func__, \
		 pstDateTime->uiMJD, pstDateTime->stTime.ucHour,
		 pstDateTime->stTime.ucMinute, pstDateTime->stTime.ucSecond);
#endif /* 0 */
		memcpy(&gstCurrentDateTime, pstDateTime, sizeof(DATE_TIME_STRUCT));
	}

	if (pstLocalTimeOffset){
		memcpy(&gstCurrentLocalTimeOffset, pstLocalTimeOffset, sizeof(LOCAL_TIME_OFFSET_STRUCT));
	}

	clock_gettime(CLOCK_MONOTONIC , &systimespec);
	g_llSYSTimeOfTOTArrival = (long long)(systimespec.tv_sec)*1000 + (long long)systimespec.tv_nsec/1000000;	//msec

	return 0;
}

int tcc_db_get_current_time(int *piMJD, int *piHour, int *piMin, int *piSec, int *piPolarity, int *piHourOffset, int *piMinOffset)
{

	TCCISDBT_EPGDB_GetDateTime(gCurrentNITAreaCode, &gstCurrentDateTime, &gstCurrentLocalTimeOffset,
								piMJD, piHour, piMin, piSec, piPolarity, piHourOffset, piMinOffset);
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Channel
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
sqlite3 *GetDB_Handle(void)
{
	return g_pISDBTDB;
}

int CreateDfsDB_Table (sqlite3 **handle, char *path)
{
	int err = SQLITE_ERROR;

	if (*handle == NULL) {
		if ((err = sqlite3_open_v2(path, handle, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE|SQLITE_OPEN_FULLMUTEX, NULL)) != SQLITE_OK) {
			ALOGE("[%s] handle open fail\n", __func__);
			return err;
		}
		else {
			sqlite3_busy_timeout(*handle, ISDBT_SQLITE_BUSY_TIMEOUT);
		}
		err = TCC_SQLITE3_EXEC(*handle, \
								"create table if not exists seamless (_id integer primary key, \
									NetworkID integer, \
									Total integer, \
									Average integer)",
									NULL, NULL, NULL);
		if (err != SQLITE_OK) {
			ALOGE("[%s] CREATE TABLE seamless failed", __func__);
			return err;
		}
	}
	return err;
}


int CreateMailDB_Table (sqlite3 **handle, char *path)
{
	int err = SQLITE_ERROR;

	if (*handle == NULL) {
		if ((err = sqlite3_open_v2(path, handle, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE|SQLITE_OPEN_FULLMUTEX, NULL)) != SQLITE_OK) {
			ALOGE("[%s] handle open fail\n", __func__);
			return err;
		}
		else {
			sqlite3_busy_timeout(*handle, ISDBT_SQLITE_BUSY_TIMEOUT);
		}
		err = TCC_SQLITE3_EXEC(*handle, \
								"create table if not exists Mail (_id integer primary key, \
									CA_system_id integer, \
									Message_ID integer, \
									Year integer, \
									Month integer, \
									Day integer, \
									Mail_length integer, \
									Mail text,\
									UserView_status integer,\
									Delete_status integer)",
									NULL, NULL, NULL);
		if (err != SQLITE_OK) {
			ALOGE("[%s] CREATE TABLE Mail failed", __func__);
			return err;
		}
	}
	return err;
}

int CreateLogoDB_Table (sqlite3 **handle, char *path)
{
	int err = SQLITE_ERROR;

	if (*handle == NULL) {
		if ((err = sqlite3_open_v2(path, handle, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE|SQLITE_OPEN_FULLMUTEX, NULL)) != SQLITE_OK) {
			ALOGE("[%s] handle open fail\n", __func__);
			return err;
		}
		else {
			sqlite3_busy_timeout(*handle, ISDBT_SQLITE_BUSY_TIMEOUT);
		}
		err = TCC_SQLITE3_EXEC(*handle, \
							"create table if not exists logo (_id integer primary key, \
									NetID integer, \
									DWNL_ID integer, \
									LOGO_ID integer, \
									LOGO_Type integer, \
									Ver integer, \
									Size integer, \
									Image blob)",
									NULL, NULL, NULL);
		if (err != SQLITE_OK) 	{
			ALOGE("[%s] CREATE TABLE Logo failed", __func__);
			return err;
		}
	}
	return err;
}

int CreateDB_Table(sqlite3 **handle, char *path)
{
	int err = SQLITE_ERROR;

	if (*handle == NULL)
	{
		if ((err = sqlite3_open_v2(path, handle, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE|SQLITE_OPEN_FULLMUTEX, NULL)) != SQLITE_OK)
		{
			ALOGE("[%s] handle open fail\n", __func__);
			return err;
		}
		else
		{
			sqlite3_busy_timeout(*handle, ISDBT_SQLITE_BUSY_TIMEOUT);
		}

		if(TCC_Isdbt_IsSupportPrimaryService())
		{
			// Noah / 20170522 / Added primaryServiceFlag for IM478A-31 (Primary Service)
			err = TCC_SQLITE3_EXEC(*handle, \
									"create table if not exists channels (_id integer primary key, \
										channelNumber integer, \
										countryCode integer, \
										audioPID integer, \
										videoPID integer, \
										stPID integer, \
										siPID integer,\
										PMT_PID integer, \
										remoconID integer, \
										serviceType integer, \
										serviceID integer, \
										regionID integer, \
										threedigitNumber integer, \
										TStreamID integer, \
										berAVG integer, \
										channelName text, \
										TSName text, \
										preset integer, \
										networkID integer, \
										areaCode integer, \
										frequency blob, \
										affiliationID blob, \
										uiTotalAudio_PID integer, \
										uiTotalVideo_PID integer, \
										usTotalCntSubtLang integer, \
										DWNL_ID integer, \
										LOGO_ID integer, \
										strLogo_Char text, \
										uiPCR_PID integer, \
										ucAudio_StreamType integer, \
										ucVideo_StreamType integer, \
										usOrig_Net_ID integer, \
										CA_ECM_PID integer, \
										AC_ECM_PID integer, \
										primaryServiceFlag integer)", \
										NULL, NULL, NULL);
		}
		else
		{
			err = TCC_SQLITE3_EXEC(*handle, \
									"create table if not exists channels (_id integer primary key, \
										channelNumber integer, \
										countryCode integer, \
										audioPID integer, \
										videoPID integer, \
										stPID integer, \
										siPID integer,\
										PMT_PID integer, \
										remoconID integer, \
										serviceType integer, \
										serviceID integer, \
										regionID integer, \
										threedigitNumber integer, \
										TStreamID integer, \
										berAVG integer, \
										channelName text, \
										TSName text, \
										preset integer, \
										networkID integer, \
										areaCode integer, \
										frequency blob, \
										affiliationID blob, \
										uiTotalAudio_PID integer, \
										uiTotalVideo_PID integer, \
										usTotalCntSubtLang integer, \
										DWNL_ID integer, \
										LOGO_ID integer, \
										strLogo_Char text, \
										uiPCR_PID integer, \
										ucAudio_StreamType integer, \
										ucVideo_StreamType integer, \
										usOrig_Net_ID integer, \
										CA_ECM_PID integer, \
										AC_ECM_PID integer)", \
										NULL, NULL, NULL);
		}

		if (err != SQLITE_OK)
		{
			ALOGE("[%s] CREATE TABLE channels failed", __func__);
			return err;
		}

		err = TCC_SQLITE3_EXEC(*handle, \
								"create table if not exists VideoPID (_id integer primary key, \
									usServiceID integer, \
									uiCurrentChannelNumber integer, \
									uiCurrentCountryCode integer, \
									usNetworkID integer, \
									uiVideo_PID integer, \
									ucVideo_IsScrambling integer, \
									ucVideo_StreamType integer, \
									ucVideoType integer, \
									ucComponent_Tag integer, \
									acLangCode text)",
									NULL, NULL, NULL);
		if (err != SQLITE_OK)
		{
			ALOGE("CREATE TABLE videoPID failed");
			return err;
		}

		err = TCC_SQLITE3_EXEC(*handle, \
								"create table if not exists audioPID (_id integer primary key, \
									usServiceID integer, \
									uiCurrentChannelNumber integer, \
									uiCurrentCountryCode integer, \
									usNetworkID integer, \
									uiAudio_PID integer, \
									ucAudio_IsScrambling integer, \
									ucAudio_StreamType integer, \
									ucAudioType integer, \
									ucComponent_Tag integer, \
									acLangCode text)",
									NULL, NULL, NULL);
		if (err != SQLITE_OK)
		{
			ALOGE("CREATE TABLE audioPID failed");
			return err;
		}

		err = TCC_SQLITE3_EXEC(*handle, \
								"create table if not exists subtitlePID (_id integer primary key, \
									usServiceID integer, \
									uiCurrentChannelNumber integer, \
									uiCurrentCountryCode integer, \
									usNetworkID integer, \
									uiSt_PID integer, \
									ucComponent_Tag integer)",
									NULL, NULL, NULL);
		if (err != SQLITE_OK)
		{
			ALOGE("CREATE TABLE subtitlePID failed");
			return err;
		}

		err = TCC_SQLITE3_EXEC(*handle, \
								"create table if not exists userDatas ( \
									_id integer primary key, \
									region_ID integer DEFAULT 0, \
									prefectural_Code BIGINT DEFAULT 0, \
									area_Code integer DEFAULT 0, \
									postal_Code text DEFAULT '000-0000')",
									NULL, NULL, NULL);
		TCC_SQLITE3_EXEC(*handle, "INSERT INTO userDatas ( _id ) SELECT 1 WHERE not exists(SELECT * FROM userDatas WHERE _id = 1)", NULL, NULL, NULL);
		if (err != SQLITE_OK)
		{
			ALOGE("CREATE TABLE userDatas failed");
			return err;
		}
	}

	return err;
}

int OpenDB_Table(void)
{
	int err = SQLITE_OK;
	int cnt;

	tcc_dxb_channel_db_lock_init();
	tcc_dxb_channel_db_lock_on();
#if defined(HAVE_LINUX_PLATFORM)
	if(isdbt_channeldb_path0[0] == 0) {
		isdbt_db_path = getenv("DXB_DATA_PATH");
		if(isdbt_db_path == NULL) {
			isdbt_db_path = default_isdbt_db_path;
		}

		memset(isdbt_channeldb_path0, 0x0, ISDBT_DB_PATH_SIZE);
		if( (strlen(isdbt_db_path) + strlen("/ISDBT0.db")) < ISDBT_DB_PATH_SIZE ) // Noah, To avoid Codesonar's warning, Buffer Overrun
			sprintf(isdbt_channeldb_path0, "%s%s", isdbt_db_path, "/ISDBT0.db");

		memset(isdbt_channeldb_path1, 0x0, ISDBT_DB_PATH_SIZE);
		if( (strlen(isdbt_db_path) + strlen("/ISDBT1.db")) < ISDBT_DB_PATH_SIZE ) // Noah, To avoid Codesonar's warning, Buffer Overrun
			sprintf(isdbt_channeldb_path1, "%s%s", isdbt_db_path, "/ISDBT1.db");

		memset(isdbt_channeldb_path2, 0x0, ISDBT_DB_PATH_SIZE);
		if( (strlen(isdbt_db_path) + strlen("/ISDBT2.db")) < ISDBT_DB_PATH_SIZE ) // Noah, To avoid Codesonar's warning, Buffer Overrun
			sprintf(isdbt_channeldb_path2, "%s%s", isdbt_db_path, "/ISDBT2.db");

		sISDBTChDBPath[0] = isdbt_channeldb_path0;
		sISDBTChDBPath[1] = isdbt_channeldb_path1;
		sISDBTChDBPath[2] = isdbt_channeldb_path2;
	}
#endif

	g_pISDBTDB = NULL;
	for (cnt=0; cnt < ISDBT_PRESET_MODE_MAX; cnt++)
	{
		if (g_pISDBTDB_Handle[cnt] == NULL) {
			err = CreateDB_Table(&g_pISDBTDB_Handle[cnt], sISDBTChDBPath[cnt]);
			if (err != SQLITE_OK) {
				ALOGE("CREATE TABLE (%d) Basic DB failed", cnt);
				tcc_dxb_channel_db_lock_off();
				return err;
			}
		} else
			ALOGE("[%s] WARNING - g_pISDBTDB (%d) is already created.", __func__, cnt);
	}
	if(gCurrentDBIndex == -1) {
		gCurrentDBIndex = 0;
		g_pISDBTDB = g_pISDBTDB_Handle[0];
	} else if (gCurrentDBIndex >= 0 && gCurrentDBIndex < ISDBT_PRESET_MODE_MAX)
		g_pISDBTDB = g_pISDBTDB_Handle[gCurrentDBIndex];
	else {
		ALOGD("In %s, gCurrentDBIndex is invalid", __func__);
		g_pISDBTDB = g_pISDBTDB_Handle[0];
		gCurrentDBIndex = 0;
	}

	if (g_pISDBTLogoDB == NULL) {
#if defined(HAVE_LINUX_PLATFORM)
		if(isdbt_logodb_path[0] == 0) {
			isdbt_db_path = getenv("DXB_DATA_PATH");
			if(isdbt_db_path == NULL) {
				isdbt_db_path = default_isdbt_db_path;
			}

			memset(isdbt_logodb_path, 0x0, ISDBT_DB_PATH_SIZE);
			if( (strlen(isdbt_db_path) + strlen("/ISDBTLogo.db")) < ISDBT_DB_PATH_SIZE ) // Noah, To avoid Codesonar's warning, Buffer Overrun
				sprintf(isdbt_logodb_path, "%s%s", isdbt_db_path, "/ISDBTLogo.db");
		}
		err = CreateLogoDB_Table (&g_pISDBTLogoDB, isdbt_logodb_path);
#endif
		if (err != SQLITE_OK) {
			ALOGE("CREATE TABLE Logo DB failed");
			tcc_dxb_channel_db_lock_off();
			return err;
		}
	}
	else {
		ALOGE("[%s] WARNING - g_pISDBTLogoDB is already created.", __func__);
	}

	tcc_dxb_channel_db_lock_off();
	TCCISDBT_ChannelDB_Init();

	return err;
}

int CloseDB_Table(void)
{
	int err = SQLITE_ERROR;
	int cnt;

	TCCISDBT_ChannelDB_Deinit();

    tcc_dxb_channel_db_lock_on();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)

	for(cnt=0; cnt < ISDBT_PRESET_MODE_MAX; cnt++)
	{
		if (g_pISDBTDB_Handle[cnt] != NULL) {
			err = sqlite3_close(g_pISDBTDB_Handle[cnt]);
			if (err != SQLITE_OK)
				ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);
			g_pISDBTDB_Handle[cnt] = NULL;
		}
	}
	g_pISDBTDB = NULL;
	gCurrentDBIndex = -1;

	if (g_pISDBTLogoDB != NULL) {
		err = sqlite3_close(g_pISDBTLogoDB);
		if (err != SQLITE_OK) {
			ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);
		}
		g_pISDBTLogoDB = NULL;
	}

    tcc_dxb_channel_db_lock_off();    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)

	tcc_dxb_channel_db_lock_deinit();

	return 0;
}

int DeleteDB_Table()
{
	int err = SQLITE_ERROR;

	if (g_pISDBTDB != NULL)
	{
		tcc_dxb_channel_db_lock_on();
		err = TCC_SQLITE3_EXEC(g_pISDBTDB, "DELETE FROM channels", NULL, NULL, NULL);
		if (err != SQLITE_OK) ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);

		err = TCC_SQLITE3_EXEC(g_pISDBTDB, "DELETE FROM videoPID", NULL, NULL, NULL);
		if (err != SQLITE_OK) ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);

		err = TCC_SQLITE3_EXEC(g_pISDBTDB, "DELETE FROM audioPID", NULL, NULL, NULL);
		if (err != SQLITE_OK) ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);

		err = TCC_SQLITE3_EXEC(g_pISDBTDB, "DELETE FROM subtitlePID", NULL, NULL, NULL);
		if (err != SQLITE_OK) ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);
		tcc_dxb_channel_db_lock_off();
	}

	return err;
}

int OpenMailDB_Table(void)
{
	int err = SQLITE_ERROR;
	int cnt;

	tcc_dxb_channel_db_lock_init();
	tcc_dxb_channel_db_lock_on();
	g_pISDBTMailDB = NULL;

#if defined(HAVE_LINUX_PLATFORM)
	if(isdbt_maildb_path[0] == 0) {
		isdbt_db_path = getenv("DXB_DATA_PATH");
		if(isdbt_db_path == NULL) {
			isdbt_db_path = default_isdbt_db_path;
		}

		memset(isdbt_maildb_path, 0x0, ISDBT_DB_PATH_SIZE);
		if( (strlen(isdbt_db_path) + strlen("/ISDBTMail.db")) < ISDBT_DB_PATH_SIZE ) // Noah, To avoid Codesonar's warning, Buffer Overrun
			sprintf(isdbt_maildb_path, "%s%s", isdbt_db_path, "/ISDBTMail.db");
	}
	err = CreateMailDB_Table (&g_pISDBTMailDB, isdbt_maildb_path);
#endif

	if (err != SQLITE_OK) {
		ALOGE("CREATE TABLE MAILDB failed");
		tcc_dxb_channel_db_lock_off();
		return err;
	}

	tcc_dxb_channel_db_lock_off();
	return err;
}

int CloseMailDB_Table(void)
{
	int err = SQLITE_ERROR;
	int cnt;

	if (g_pISDBTMailDB != NULL) {
		err = sqlite3_close(g_pISDBTMailDB);
		if (err != SQLITE_OK) {
			ALOGE("[%s:%d] g_pISDBTMailDB Close err:%d\n", __func__, __LINE__, err);
		}
		g_pISDBTMailDB = NULL;
	}

	tcc_dxb_channel_db_lock_deinit();

	return 0;
}

int DeleteMailDB_Table()
{
	int err = SQLITE_ERROR;

	if (g_pISDBTMailDB != NULL)
	{
		tcc_dxb_channel_db_lock_on();
		err = TCC_SQLITE3_EXEC(g_pISDBTMailDB, "DELETE FROM Mail", NULL, NULL, NULL);
		if (err != SQLITE_OK) ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);

		tcc_dxb_channel_db_lock_off();
	}
	return err;
}


BOOL UpdateDB_MailTable(
	int uiCA_system_id,
	int uiMessage_ID,
	int uiYear,
	int uiMonth,
	int uiDay,
	int uiMail_length,
	char *ucMailpayload,
	int uiUserView_status,
	int uiDelete_status
)
{
	int err = SQLITE_ERROR;

	tcc_dxb_channel_db_lock_on();

	if (g_pISDBTDB != NULL)
	{
		err = TCCISDBT_MailDB_Update(g_pISDBTMailDB,
									uiCA_system_id,
									uiMessage_ID,
									uiYear,
									uiMonth,
									uiDay,
									uiMail_length,
									ucMailpayload,
									uiUserView_status,
									uiDelete_status);

	}

	tcc_dxb_channel_db_lock_off();

	return (err == SQLITE_OK);
}

BOOL UpdateDB_MailTable_DeleteStatus(int uiMessageID, int uiDelete_status)
{
	int err = SQLITE_ERROR;

	tcc_dxb_channel_db_lock_on();

	if (g_pISDBTDB != NULL)
	{
		err = TCCISDBT_MailDB_DeleteStatus_Update(g_pISDBTMailDB, uiMessageID, uiDelete_status);
	}

	tcc_dxb_channel_db_lock_off();

	return (err == SQLITE_OK);
}

BOOL UpdateDB_MailTable_UserViewStatus(int uiMessageID, int uiUserView_status)
{
	int err = SQLITE_ERROR;

	tcc_dxb_channel_db_lock_on();

	if (g_pISDBTDB != NULL)
	{
		err = TCCISDBT_MailDB_UserViewStatus_Update(g_pISDBTMailDB, uiMessageID, uiUserView_status);
	}

	tcc_dxb_channel_db_lock_off();

	return (err == SQLITE_OK);
}

/*Seamless DB*/
int OpenDfsDB_Table(void)
{
	int err = SQLITE_ERROR;
	int cnt;
	ALOGD("\033[32m In[%s] \033[m\n", __func__);
	tcc_dxb_seamless_db_lock_init();

	g_pISDBTDfsDB = NULL;

	if(isdbt_dfsdb_path[0] == 0) {
		isdbt_db_path = getenv("DXB_DATA_PATH");
		if(isdbt_db_path == NULL) {
			isdbt_db_path = default_isdbt_db_path;
		}

		memset(isdbt_dfsdb_path, 0x0, ISDBT_DB_PATH_SIZE);
		if( (strlen(isdbt_db_path) + strlen("/ISDBTDFS.db")) < ISDBT_DB_PATH_SIZE ) // Noah, To avoid Codesonar's warning, Buffer Overrun
			sprintf(isdbt_dfsdb_path, "%s%s", isdbt_db_path, "/ISDBTDFS.db");
	}
	err = CreateDfsDB_Table (&g_pISDBTDfsDB, isdbt_dfsdb_path);

	if (err != SQLITE_OK) {
		ALOGE("CREATE TABLE DFSDB failed");
		return err;
	}
	ALOGD("\033[32m Out[%s] \033[m\n", __func__);
	return err;
}

int CloseDfsDB_Table(void)
{
	int err = SQLITE_ERROR;
	int cnt;
	ALOGD("\033[32m In[%s] \033[m\n", __func__);
	if (g_pISDBTDfsDB != NULL) {
		err = sqlite3_close(g_pISDBTDfsDB);
		if (err != SQLITE_OK) {
			ALOGE("[%s:%d] g_pISDBTDfsDB Close err:%d\n", __func__, __LINE__, err);
		}
		g_pISDBTDfsDB = NULL;
	}

	tcc_dxb_seamless_db_lock_deinit();
	ALOGD("\033[32m Out[%s] \033[m\n", __func__);
	return 0;
}

int DeleteDfsDB_Table()
{
	int err = SQLITE_ERROR;

	if (g_pISDBTDfsDB != NULL)
	{
		tcc_dxb_seamless_db_lock_on();
		err = TCC_SQLITE3_EXEC(g_pISDBTDfsDB, "DELETE FROM seamless", NULL, NULL, NULL);
		if (err != SQLITE_OK) ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);

		tcc_dxb_seamless_db_lock_off();
	}
	return err;
}

BOOL UpdateDB_DfsDB_Table(
	unsigned int uiChannelNumber,
	int uiTotal,
	int uiAverage
)
{
	int err = SQLITE_ERROR;
	char szSQL[1024] = { 0 };
	sqlite3_stmt *stmt = NULL;
	int	i, rc = SQLITE_ERROR, Ver = 0;
	int TsID, iCount;

	tcc_dxb_seamless_db_lock_on();
	TsID = tcc_db_get_channel_tstreamid(uiChannelNumber);

	if (g_pISDBTDfsDB != NULL)
	{
		sprintf(szSQL, "SELECT COUNT(_id) FROM seamless WHERE NetworkID=%d", TsID);
		if (sqlite3_prepare_v2(g_pISDBTDfsDB, szSQL, -1, &stmt, NULL) == SQLITE_OK) {
			err = sqlite3_step(stmt);
			if (err == SQLITE_ROW) {
				iCount = sqlite3_column_int(stmt, 0);
			}
			if ((err != SQLITE_ROW) && (err != SQLITE_DONE)){
				ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
			stmt = NULL;
		}

		ALOGE("\033[32m [%s] iCount[%d] [%d:%d]\033[m\n", __func__, iCount, uiTotal, uiAverage);
		if (iCount == 0) {
			sprintf(szSQL, "INSERT INTO seamless (NetworkID, Total, Average) VALUES (%d, %d, %d)", TsID, uiTotal, uiAverage);
		}else{
			sprintf(szSQL, "UPDATE seamless SET Total=%d, Average=%d WHERE NetworkID=%d", uiTotal, uiAverage, TsID);
		}

		if (sqlite3_prepare_v2(g_pISDBTDfsDB, szSQL, -1, &stmt, NULL) == SQLITE_OK)
		{
//			err = sqlite3_reset(stmt);
//			if (err != SQLITE_OK) ALOGE("[%s:%d] ERROR : %d\n", __func__, __LINE__, err);

			err = sqlite3_step(stmt);
			if ((err != SQLITE_ROW) && (err != SQLITE_DONE)) ALOGE("[%s:%d] ERROR : %d\n", __func__, __LINE__, err);
		}
		if (stmt) {
			sqlite3_finalize(stmt);
//			stmt = NULL;	// I think 'stmt = NULL' is so meaningless at this location.
		}

	}

	tcc_dxb_seamless_db_lock_off();

	return (err == SQLITE_OK);
}

int GetDFSValue_DB_Table(int uiChannelNumber, int *piDFSval)
{
	char szSQL[1024] = { 0 };
	sqlite3_stmt *stmt = NULL;
	int rc = SQLITE_ERROR;

	tcc_dxb_seamless_db_lock_on();
	*piDFSval = 0;
	sprintf(szSQL, "SELECT Average FROM seamless WHERE NetworkID=%d", tcc_db_get_channel_tstreamid(uiChannelNumber));
	if(sqlite3_prepare_v2(g_pISDBTDfsDB, szSQL, strlen(szSQL), &stmt,NULL) == SQLITE_OK)
	{
		rc = sqlite3_step(stmt);
		if (rc == SQLITE_ROW)
		{
			*piDFSval = sqlite3_column_int(stmt, 0);
		}
	}
	if (stmt) {
		sqlite3_finalize(stmt);
//		stmt = NULL;	// I think 'stmt = NULL' is so meaningless at this location.
	}
	tcc_dxb_seamless_db_lock_off();

	if (rc != SQLITE_ROW)
	{
		return -1;
	}

	return 0;
}

int GetDFSCnt_DB_Table(int uiChannelNumber, int *piDFScnt)
{
	char szSQL[1024] = { 0 };
	sqlite3_stmt *stmt = NULL;
	int rc = SQLITE_ERROR;

	*piDFScnt = 0;
	tcc_dxb_seamless_db_lock_on();
	sprintf(szSQL, "SELECT Total FROM seamless WHERE NetworkID=%d", tcc_db_get_channel_tstreamid(uiChannelNumber));
	if(sqlite3_prepare_v2(g_pISDBTDfsDB, szSQL, strlen(szSQL), &stmt,NULL) == SQLITE_OK)
	{
		rc = sqlite3_step(stmt);
		if (rc == SQLITE_ROW)
		{
			*piDFScnt = sqlite3_column_int(stmt, 0);
		}
	}
	if (stmt) {
		sqlite3_finalize(stmt);
//		stmt = NULL;	// I think 'stmt = NULL' is so meaningless at this location.
	}
	tcc_dxb_seamless_db_lock_off();

	if (rc != SQLITE_ROW)
	{
		return -1;
	}

	return 0;
}



/*
  * i have to clean all informatin of channel and service on memory
  */
int ChangeDB_Handle (int index, int *prev_handle)
{
	int err = SQLITE_ERROR;

	ALOGD("ChangeDB_Handle,%d", index);
	if (index < 0 || index >= ISDBT_PRESET_MODE_MAX)
		return err;

	tcc_dxb_channel_db_lock_on();
	if (g_pISDBTDB_Handle[index] != NULL) {
		*prev_handle = gCurrentDBIndex;
		g_pISDBTDB = g_pISDBTDB_Handle[index];
		gCurrentDBIndex = index;
		err = SQLITE_OK;

		TCCISDBT_ChannelDB_Clear();
		ALOGD("ChangeDB_Handle ok");
	}
	tcc_dxb_channel_db_lock_off();
	return err;
}

int UpdateDB_ChannelDB_Clear()
{
	int err = SQLITE_ERROR;
	tcc_dxb_channel_db_lock_on();
	if (g_pISDBTDB != NULL)
		err = TCCISDBT_ChannelDB_Clear();
	tcc_dxb_channel_db_lock_off();
	return err;
}

BOOL InsertDB_ChannelTable(P_ISDBT_SERVICE_STRUCT pServiceList, int fUpdateType)
{
	int err = SQLITE_ERROR, cnt;

	tcc_dxb_channel_db_lock_on();

	gCurrentChannel = pServiceList->uiCurrentChannelNumber;
	gCurrentCountryCode = pServiceList->uiCurrentCountryCode;

	//add a channel table to DB which will be used in UI.
	if (g_pISDBTDB != NULL)
	{
		err = TCCISDBT_ChannelDB_Insert(pServiceList,fUpdateType);
	}

	tcc_dxb_channel_db_lock_off();

	return (err == SQLITE_OK);
}

BOOL UpdateDB_ChannelTable
(
	int uiServiceID,
	int uiCurrentChannelNumber,
	int uiCurrentCountryCode,
	P_ISDBT_SERVICE_STRUCT pServiceList
)
{
	int err = SQLITE_ERROR;

	tcc_dxb_channel_db_lock_on();

	if (g_pISDBTDB != NULL)
	{
		err = TCCISDBT_ChannelDB_Update(uiCurrentChannelNumber, uiCurrentCountryCode, uiServiceID, pServiceList);
		err = TCCISDBT_ChannelDB_UpdateSQLfile_Partially(g_pISDBTDB, uiCurrentChannelNumber, uiCurrentCountryCode, uiServiceID);
	}

	tcc_dxb_channel_db_lock_off();

	return (err == SQLITE_OK);
}

/*
  *	BOOL UpdateDB_PMTPID_ChannelTable(int iPMT)
  *
  *	This function is useful only for 1seg product.
  *	update PMT_PID field of channel table.
  *
  *	parameter
  *		iPMT		pid of pmt.
  *				pid of pmt is defined as 0x1FC8 + least significant 3 bits of service_id from japanese and brazil regulation.
  *				In case of brazil, there is a broadcaster that doesn't obey this rule.
  *				So, pid of pmt should be confirmed after on receiving pmt.
  */
BOOL UpdateDB_PMTPID_ChannelTable(int uiServiceID, int iPMT)
{
	int err = SQLITE_ERROR;
	int 	Service_ID =0;

	tcc_dxb_channel_db_lock_on();

	if (g_pISDBTDB != NULL)
	{
		if(uiServiceID ==0)
			Service_ID = gCurrentServiceID;
		else
			Service_ID = uiServiceID;

		err = TCCISDBT_ChannelDB_UpdatePMTPID(gCurrentChannel,
									gCurrentCountryCode,
									Service_ID,
									iPMT);
	}

	tcc_dxb_channel_db_lock_off();

	return (err == SQLITE_OK);
}

BOOL UpdateDB_StreamID_ChannelTable(int iServiceID, int iStreamID)
{
	int err = SQLITE_ERROR;

	tcc_dxb_channel_db_lock_on();

	if (g_pISDBTDB != NULL)
	{
		err = TCCISDBT_ChannelDB_UpdateStreamID(gCurrentChannel,
												gCurrentCountryCode,
												iServiceID,
												iStreamID);
	}

	tcc_dxb_channel_db_lock_off();

	return (err == SQLITE_OK);
}


BOOL UpdateDB_RemoconID_ChannelTable
(
	int uiServiceID,
	int uiCurrentChannelNumber,
	int uiCurrentCountryCode,
	P_ISDBT_SERVICE_STRUCT pServiceList
)
{
	int err = SQLITE_ERROR;
	int regionID, threedigitNumber;

	if (uiCurrentCountryCode == 1) {	//brazil
		regionID = 0;
		threedigitNumber = (((uiServiceID & 0x0018) >> 3) * 200)
			+ (pServiceList->remocon_ID * 10)
			+ ((uiServiceID & 0x0007) + 1);
	} else {	//japan and default
		regionID = (uiServiceID & 0xFC00) >> 10;
		threedigitNumber = (((uiServiceID & 0x0180) >> 7) * 200) + \
			(pServiceList->remocon_ID * 10) + \
			((uiServiceID & 0x0007) + 1);
	}

	tcc_dxb_channel_db_lock_on();

	if (g_pISDBTDB != NULL)
	{
		err = TCCISDBT_ChannelDB_UpdateRemoconID(uiCurrentChannelNumber, \
													uiCurrentCountryCode, \
													uiServiceID, \
													pServiceList->remocon_ID, \
													regionID, threedigitNumber);
	}

	tcc_dxb_channel_db_lock_off();

	return (err == SQLITE_OK);
}

BOOL UpdateDB_ChannelName_ChannelTable
(
	int uiServiceID,
	int uiCurrentChannelNumber,
	int uiCurrentCountryCode,
	P_ISDBT_SERVICE_STRUCT pServiceList
)
{
	int err = SQLITE_ERROR;
	unsigned short pUniStr[256] = {0, };

	tcc_dxb_channel_db_lock_on();
	if (g_pISDBTDB != NULL)
	{
		ISDBTString_MakeUnicodeString((char *)pServiceList->strServiceName, (short*)pUniStr, Get_Language_Code());
		err = TCCISDBT_ChannelDB_UpdateChannelName(uiCurrentChannelNumber,
						uiCurrentCountryCode,
						uiServiceID,
						pUniStr);
	}
	tcc_dxb_channel_db_lock_off();

	return (err == SQLITE_OK);
}

BOOL UpdateDB_TSName_ChannelTable (int uiServiceID, int uiCurrentChannelNumber,int uiCurrentCountryCode, P_ISDBT_SERVICE_STRUCT pSvcList)
{
	int err = SQLITE_ERROR;
	unsigned short pUniTSName[256] = {0, };

	tcc_dxb_channel_db_lock_on();
	if (g_pISDBTDB != NULL) {
		ISDBTString_MakeUnicodeString((char *)pSvcList->strTSName, (short*)pUniTSName, Get_Language_Code());
		err = TCCISDBT_ChannelDB_UpdateTSName(uiCurrentChannelNumber,
						uiCurrentCountryCode,
						uiServiceID,
						pUniTSName);
	}
	tcc_dxb_channel_db_lock_off();

	return (err == SQLITE_OK);
}

BOOL UpdateDB_NetworkID_ChannelTable
(
	int uiServiceID,
	int uiCurrentChannelNumber,
	int uiCurrentCountryCode,
	P_ISDBT_SERVICE_STRUCT pServiceList
)
{
	int err = SQLITE_ERROR;

	tcc_dxb_channel_db_lock_on();

	if (g_pISDBTDB != NULL)
	{
		err = TCCISDBT_ChannelDB_UpdateNetworkID(uiCurrentChannelNumber, uiCurrentCountryCode, uiServiceID, pServiceList->usNetworkID);
	}

	tcc_dxb_channel_db_lock_off();

	return (err == SQLITE_OK);
}

BOOL UpdateDB_Frequency_ChannelTable
(
	int uiCurrentChannelNumber,
	int uiCurrentCountryCode,
	P_ISDBT_SERVICE_STRUCT pServiceList
)
{
	T_DESC_TDSD stDescTDSD;
	int err = SQLITE_ERROR;

	if(ISDBT_Get_DescriptorInfo(E_DESC_TERRESTRIAL_INFO, (void *)&stDescTDSD) == 0) {
		ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);
		return 0;
	}

	tcc_dxb_channel_db_lock_on();

	if (g_pISDBTDB != NULL)
	{
		err = TCCISDBT_ChannelDB_UpdateFrequency(uiCurrentChannelNumber, uiCurrentCountryCode, &stDescTDSD);
	}

	tcc_dxb_channel_db_lock_off();

	return (err == SQLITE_OK);
}

BOOL UpdateDB_AffiliationID_ChannelTable
(
	int uiCurrentChannelNumber,
	int uiCurrentCountryCode,
	P_ISDBT_SERVICE_STRUCT pServiceList
)
{
	T_DESC_EBD stDescEBD;
	int err = SQLITE_ERROR;

	memset (&stDescEBD, 0, sizeof(T_DESC_EBD));
	if(ISDBT_Get_DescriptorInfo(E_DESC_EXT_BROADCAST_INFO, (void *)&stDescEBD) == 0) {
		ALOGE("[%s:%d] no extended_broadcast_descriptor\n", __func__, __LINE__);
		return 0;
	}

	tcc_dxb_channel_db_lock_on();

	if (g_pISDBTDB != NULL)
	{
		err = TCCISDBT_ChannelDB_UpdateAffiliationID(uiCurrentChannelNumber, uiCurrentCountryCode, &stDescEBD);
	}

	tcc_dxb_channel_db_lock_off();

	return (err == SQLITE_OK);
}

int UpdateDB_AreaCode_ChannelTable
(
	int uiCurrentChannelNumber,
	int uiCurrentCountryCode,
	int AreaCode
)
{
	int err = SQLITE_ERROR;

	gCurrentNITAreaCode = AreaCode;

	if (g_pISDBTDB != NULL) {
		tcc_dxb_channel_db_lock_on();
		err = TCCISDBT_ChannelDB_UpdateAreaCode(uiCurrentChannelNumber, uiCurrentCountryCode, AreaCode);
		tcc_dxb_channel_db_lock_off();
	}
	return (err == SQLITE_OK);
}

int UpdateDB_OriginalNetworID_ChannelTable (int uiServiceID, int uiCurrentChannelNumber, int uiCurrentCountryCode, P_ISDBT_SERVICE_STRUCT pServiceList)
{
	int err = SQLITE_ERROR;

	if (g_pISDBTDB != NULL) {
		tcc_dxb_channel_db_lock_on();
		err = TCCISDBT_ChannelDB_UpdateOriginalNetworkID(uiCurrentChannelNumber, uiCurrentCountryCode, uiServiceID, (unsigned int)(pServiceList->usOrig_Net_ID));
		tcc_dxb_channel_db_lock_off();
	}
	return (err == SQLITE_OK);
}

int UpdateDB_Channel_SQL(int channel, int country_code, int service_id, int del_option)
{
	int err = SQLITE_ERROR;

	if (g_pISDBTDB != NULL) {
		tcc_dxb_channel_db_lock_on();
		err = TCCISDBT_ChannelDB_UpdateSQLfile(g_pISDBTDB, channel, country_code, service_id, del_option);
		if (err != SQLITE_OK) ALOGE("[%s:%d] ERROR : %d\n", __func__, __LINE__, err);
		tcc_dxb_channel_db_lock_off();
	}

	return 0;
}

int UpdateDB_Channel_SQL_Partially(unsigned int channel, unsigned int country_code, unsigned int service_id)
{
	int err = SQLITE_ERROR;

	if (g_pISDBTDB != NULL) {
		tcc_dxb_channel_db_lock_on();
		err = TCCISDBT_ChannelDB_UpdateSQLfile_Partially(g_pISDBTDB, channel, country_code, service_id);
		if (err != SQLITE_OK) ALOGE("[%s:%d] ERROR : %d\n", __func__, __LINE__, err);
		tcc_dxb_channel_db_lock_off();
	}

	return 0;
}

int UpdateDB_Channel_SQL_AutoSearch(int channel, int country_code, int service_id)
{
	int err = SQLITE_ERROR;

	if (g_pISDBTDB != NULL) {
		tcc_dxb_channel_db_lock_on();
		err = TCCISDBT_ChannelDB_UpdateSQLfile_AutoSearch(g_pISDBTDB, channel, country_code, service_id);
		if (err != SQLITE_OK) ALOGE("[%s:%d] ERROR : %d\n", __func__, __LINE__, err);
		tcc_dxb_channel_db_lock_off();
	}

	return 0;
}

int UpdateDB_CheckDoneUpdateInfo(int iCurrentChannelNumber, int iCurrentCountryCode, int iServiceID)
{
	int err = SQLITE_ERROR;
	tcc_dxb_channel_db_lock_on();
	if (g_pISDBTDB != NULL)
		err = TCCISDBT_ChannelDB_CheckDoneUpdateInfo(iCurrentChannelNumber, iCurrentCountryCode, iServiceID);
	tcc_dxb_channel_db_lock_off();
	return err;
}

int UpdateDB_Channel_Is1SegService(unsigned int channel, unsigned int country_code, unsigned int service_id)
{
	int err = 0;
	if (g_pISDBTDB != NULL) {
		tcc_dxb_channel_db_lock_on();
		err = TCCISDBT_ChannelDB_Is1SegService(channel, country_code, service_id);
		tcc_dxb_channel_db_lock_off();
	}
	return err;
}

int UpdateDB_Channel_Get1SegServiceCount(unsigned int channel, unsigned int country_code)
{
	int rtn = 0;
	if(g_pISDBTDB != NULL) {
		tcc_dxb_channel_db_lock_on();
		rtn = TCCISDBT_ChannelDB_Get1SegServiceCount(channel, country_code);
		tcc_dxb_channel_db_lock_off();
	}
	return rtn;
}

int UpdateDB_Channel_GetDataServiceCount(unsigned int channel, unsigned int country_code)
{
	int rtn = 0;
	if(g_pISDBTDB != NULL) {
		tcc_dxb_channel_db_lock_on();
		rtn = TCCISDBT_ChannelDB_GetDataServiceCount(channel, country_code);
		tcc_dxb_channel_db_lock_off();
	}
	return rtn;
}

int UpdateDB_Channel_GetSpecialServiceCount(unsigned int channel, unsigned int country_code)
{
	int rtn = 0;
	if(g_pISDBTDB != NULL) {
		tcc_dxb_channel_db_lock_on();
		rtn = TCCISDBT_ChannelDB_GetSpecialServiceCount(channel, country_code);
		tcc_dxb_channel_db_lock_off();
	}
	return rtn;
}

int UpdateDB_Channel_Get1SegServiceID (unsigned int channel, unsigned int country_code, unsigned int *service_id)
{
	int rtn = 0;
	if(g_pISDBTDB != NULL) {
		tcc_dxb_channel_db_lock_on();
		rtn = TCCISDBT_ChannelDB_Get1SegServiceID(channel, country_code, service_id);
		tcc_dxb_channel_db_lock_off();
	}
	return rtn;
}

int UpdateDB_Channel_GetDataServiceID (unsigned int channel, unsigned int country_code, unsigned int *service_id)
{
	int rtn = 0;
	if(g_pISDBTDB != NULL) {
		tcc_dxb_channel_db_lock_on();
		rtn = TCCISDBT_ChannelDB_GetDataServiceID(channel, country_code, service_id);
		tcc_dxb_channel_db_lock_off();
	}
	return rtn;
}

int GetCountDB_PMT_PID_ChannelTable(void)
{
	int listCount = 0;

	if (g_pISDBTDB != NULL)
	{
		tcc_dxb_channel_db_lock_on();
		listCount = TCCISDBT_ChannelDB_GetPMTCount(gCurrentChannel);
		tcc_dxb_channel_db_lock_off();
	}
	return listCount;
}

BOOL GetPMT_PID_ChannelTable(unsigned short* pPMT_PID, int iPMT_PID)
{
	int err = SQLITE_ERROR;

	if (g_pISDBTDB != NULL)
	{
		tcc_dxb_channel_db_lock_on();
		TCCISDBT_ChannelDB_GetPMTPID(gCurrentChannel, pPMT_PID, iPMT_PID);
		tcc_dxb_channel_db_lock_off();
	}
	return (err == SQLITE_DONE);
}

BOOL GetServiceID_ChannelTable(unsigned short *pSERVICE_ID, int iSERVICE_ID_SIZE)
{
	int err = SQLITE_ERROR;

	if (g_pISDBTDB != NULL)
	{
		tcc_dxb_channel_db_lock_on();
		TCCISDBT_ChannelDB_GetServiceID(gCurrentChannel, pSERVICE_ID, iSERVICE_ID_SIZE);
		tcc_dxb_channel_db_lock_off();
	}
	return (err == SQLITE_DONE);
}

BOOL UpdateDB_Channel_LogoInfo
(
	int uiServiceID,
	int uiCurrentChannelNumber,
	int uiCurrentCountryCode,
	P_ISDBT_SERVICE_STRUCT pServiceList
) {
	T_DESC_LTD      stDescLTD;
	int err = SQLITE_ERROR;
	unsigned short pUniStr[256] = { 0 };
	int lang_code = 0;
	int iStringSize;

	tcc_dxb_channel_db_lock_on();
	if (g_pISDBTDB != NULL) {
		if(pServiceList->logo_char != NULL) {
			lang_code = Get_Language_Code();
			iStringSize = ISDBTString_MakeUnicodeString((char *)pServiceList->logo_char, (short*)pUniStr, lang_code);
		}
		TCCISDBT_ChannelDB_UpdateLogoInfo(uiServiceID, uiCurrentChannelNumber, uiCurrentCountryCode, pServiceList, pUniStr);
	}
	tcc_dxb_channel_db_lock_off();

	return (err == SQLITE_OK);
}

BOOL InsertDB_VideoPIDTable
(
	int uiCurrentChannelNumber,
	int uiCurrentCountryCode,
	P_ISDBT_SERVICE_STRUCT pServiceList
)
{
	int err = SQLITE_ERROR;
	sqlite3_stmt *stmt = NULL;
	char szSQL[1024] = { 0 };
	int fInsert = 0, i, j;
	int uiVideo_PID[16], uiTotalVideo_PID, tempPID;
	E_DEMUX_STATE eDemuxState;

	if (g_pISDBTDB == NULL)
		return 0;

	tcc_dxb_channel_db_lock_on();

	eDemuxState = tcc_manager_demux_get_state();
	if ((eDemuxState != E_DEMUX_STATE_SCAN)
		&& (eDemuxState != E_DEMUX_STATE_INFORMAL_SCAN)) {
		sprintf(szSQL, "SELECT uiVideo_PID FROM videoPID WHERE usServiceID=%d AND uiCurrentChannelNumber=%d",
                pServiceList->usServiceID, uiCurrentChannelNumber);


		if (sqlite3_prepare_v2(g_pISDBTDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK) {
			uiTotalVideo_PID = 0;
			err = sqlite3_step(stmt);
			while (err == SQLITE_ROW)
			{
				uiVideo_PID[uiTotalVideo_PID] = sqlite3_column_int(stmt, 0);
				uiTotalVideo_PID++;
				if (uiTotalVideo_PID >= 16)
					break;
				err = sqlite3_step(stmt);
			}

			if (uiTotalVideo_PID != pServiceList->uiTotalVideo_PID) {
				ALOGD("Video ES Count changed (%d,%d)\n", uiTotalVideo_PID, pServiceList->uiTotalVideo_PID);
				fInsert = 1;
			}
			else {
				for (i=0; i < pServiceList->uiTotalVideo_PID; i++) {
					tempPID = pServiceList->stVideo_PIDInfo[i].uiVideo_PID;
					for (j=0; j < uiTotalVideo_PID; j++) {
						if (tempPID == uiVideo_PID[j]) break;
					}
					if (j == uiTotalVideo_PID) {
						ALOGD("New Video ES found(%d)\n", tempPID);
						fInsert = 1;
						break;
					}
				}
			}
		}
		else {
			fInsert = 1;
		}
		if (stmt) {
			sqlite3_finalize(stmt);
//			stmt = NULL;	// I think 'stmt = NULL' is so meaningless at this location.
		}
	} else
		fInsert  = 1;

	if (fInsert ==1) {
		//delete audio pids
		sprintf(szSQL, "DELETE FROM videoPID WHERE usServiceID=%d AND uiCurrentChannelNumber=%d", pServiceList->usServiceID, uiCurrentChannelNumber);
		err = TCC_SQLITE3_EXEC(g_pISDBTDB, szSQL, NULL, NULL, NULL);
		if (err != SQLITE_OK) ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);

		// Insert AudioPID List into DB.
		for (i=0; i < pServiceList->uiTotalVideo_PID && i < MAX_VIDEOPID_SUPPORT; i++) {
			sprintf(szSQL,"INSERT INTO videoPID \
(usServiceID, uiCurrentChannelNumber, uiCurrentCountryCode, usNetworkID, \
uiVideo_PID, ucVideo_IsScrambling, ucVideo_StreamType, ucVideoType, ucComponent_Tag, acLangCode) \
VALUES (%d, %d, %d, %d, %d, %d, %d, %d, %d, '%s')",
pServiceList->usServiceID,
uiCurrentChannelNumber,
uiCurrentCountryCode,
pServiceList->usNetworkID,
pServiceList->stVideo_PIDInfo[i].uiVideo_PID,
pServiceList->stVideo_PIDInfo[i].ucVideo_IsScrambling,
pServiceList->stVideo_PIDInfo[i].ucVideo_StreamType,
pServiceList->stVideo_PIDInfo[i].ucVideoType,
pServiceList->stVideo_PIDInfo[i].ucComponent_Tag,
pServiceList->stVideo_PIDInfo[i].acLangCode);

			err = TCC_SQLITE3_EXEC(g_pISDBTDB, szSQL, NULL, NULL, NULL);
			if (err != SQLITE_OK) ALOGE("[%s:%d] ERROR : %d\n", __func__, __LINE__, err);
		}
	}

	tcc_dxb_channel_db_lock_off();

	return (err == SQLITE_OK);
}

BOOL InsertDB_AudioPIDTable
(
	int uiCurrentChannelNumber,
	int uiCurrentCountryCode,
	P_ISDBT_SERVICE_STRUCT pServiceList
)
{
	int err = SQLITE_ERROR;
	sqlite3_stmt *stmt = NULL;
	char szSQL[1024] = { 0 };
	int fInsert = 0, i, j;
	int uiAudio_PID[16], uiTotalAudio_PID, tempPID;
	E_DEMUX_STATE eDemuxState;

	if (g_pISDBTDB == NULL)
		return 0;

	tcc_dxb_channel_db_lock_on();

	eDemuxState = tcc_manager_demux_get_state();
	if ((eDemuxState != E_DEMUX_STATE_SCAN)
		&& (eDemuxState != E_DEMUX_STATE_INFORMAL_SCAN)
		&& (eDemuxState != E_DEMUX_STATE_IDLE)) {
		sprintf(szSQL, "SELECT uiAudio_PID FROM audioPID WHERE usServiceID=%d AND uiCurrentChannelNumber=%d",
                pServiceList->usServiceID, uiCurrentChannelNumber);


		if (sqlite3_prepare_v2(g_pISDBTDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK) {
			uiTotalAudio_PID = 0;
			err = sqlite3_step(stmt);
			while (err == SQLITE_ROW)
			{
				uiAudio_PID[uiTotalAudio_PID] = sqlite3_column_int(stmt, 0);
				uiTotalAudio_PID++;
				if (uiTotalAudio_PID >= 16)
					break;
				err = sqlite3_step(stmt);
			}

			if (uiTotalAudio_PID != pServiceList->uiTotalAudio_PID) {
				ALOGD("Audio ES Count changed (%d,%d)\n", uiTotalAudio_PID, pServiceList->uiTotalAudio_PID);
				fInsert = 1;
			}
			else {
				for (i=0; i < pServiceList->uiTotalAudio_PID; i++) {
					tempPID = pServiceList->stAudio_PIDInfo[i].uiAudio_PID;
					for (j=0; j < uiTotalAudio_PID; j++) {
						if (tempPID == uiAudio_PID[j]) break;
					}
					if (j == uiTotalAudio_PID) {
						ALOGD("New Audio ES found(%d)\n", tempPID);
						fInsert = 1;
						break;
					}
				}
			}
		}
		else {
			fInsert = 1;
		}
		if (stmt) {
			sqlite3_finalize(stmt);
//			stmt = NULL;	// I think 'stmt = NULL' is so meaningless at this location.
		}
	} else
		fInsert  = 1;

	if (fInsert ==1) {
		//delete audio pids
		sprintf(szSQL, "DELETE FROM audioPID WHERE usServiceID=%d AND uiCurrentChannelNumber=%d", pServiceList->usServiceID, uiCurrentChannelNumber);
		err = TCC_SQLITE3_EXEC(g_pISDBTDB, szSQL, NULL, NULL, NULL);
		if (err != SQLITE_OK) ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);

		// Insert AudioPID List into DB.
		for (i=0; i < pServiceList->uiTotalAudio_PID && i < MAX_AUDIOPID_SUPPORT; i++) {
			sprintf(szSQL,"INSERT INTO audioPID \
(usServiceID, uiCurrentChannelNumber, uiCurrentCountryCode, usNetworkID, uiAudio_PID, \
ucAudio_IsScrambling, ucAudio_StreamType, ucAudioType, ucComponent_Tag, acLangCode) \
VALUES (%d, %d, %d, %d, %d, %d, %d, %d, %d, '%s')",
pServiceList->usServiceID,
uiCurrentChannelNumber,
uiCurrentCountryCode,
pServiceList->usNetworkID,
pServiceList->stAudio_PIDInfo[i].uiAudio_PID,
pServiceList->stAudio_PIDInfo[i].ucAudio_IsScrambling,
pServiceList->stAudio_PIDInfo[i].ucAudio_StreamType,
pServiceList->stAudio_PIDInfo[i].ucAudioType,
pServiceList->stAudio_PIDInfo[i].ucComponent_Tag,
pServiceList->stAudio_PIDInfo[i].acLangCode);

			err = TCC_SQLITE3_EXEC(g_pISDBTDB, szSQL, NULL, NULL, NULL);
			if (err != SQLITE_OK) ALOGE("[%s:%d] ERROR : %d\n", __func__, __LINE__, err);
		}
	}

	tcc_dxb_channel_db_lock_off();

	return (err == SQLITE_OK);
}

BOOL InsertDB_SubTitlePIDTable
(
	int uiCurrentChannelNumber,
	int uiCurrentCountryCode,
	P_ISDBT_SERVICE_STRUCT pServiceList
)
{
	int err = SQLITE_ERROR;
	sqlite3_stmt *stmt = NULL;
	char szSQL[1024] = { 0 };
	int fInsert = 0, i = 0, j = 0;
	int uiSt_PID = 0, tempPID = 0;
	E_DEMUX_STATE eDemuxState;

	if (g_pISDBTDB == NULL)
		return 0;

	tcc_dxb_channel_db_lock_on();

	eDemuxState = tcc_manager_demux_get_state();
	if ((eDemuxState != E_DEMUX_STATE_SCAN)
		&& (eDemuxState != E_DEMUX_STATE_INFORMAL_SCAN)
		&& (eDemuxState != E_DEMUX_STATE_IDLE)) {
		sprintf(szSQL, "SELECT uiSt_PID FROM subtitlePID WHERE usServiceID=%d AND uiCurrentChannelNumber=%d",
                pServiceList->usServiceID, uiCurrentChannelNumber);


		if (sqlite3_prepare_v2(g_pISDBTDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK) {
			err = sqlite3_step(stmt);
			if (err == SQLITE_ROW)
			{
				uiSt_PID = sqlite3_column_int(stmt, 0);
			}

			if (uiSt_PID != pServiceList->stSubtitleInfo[0].ucSubtitle_PID) {
				ALOGD("SubTitle PID changed (%d->%d)\n", uiSt_PID, pServiceList->stSubtitleInfo[0].ucSubtitle_PID);
				fInsert = 1;
			}
		}
		else {
			fInsert = 1;
		}
		if (stmt) {
			sqlite3_finalize(stmt);
//			stmt = NULL;	// I think 'stmt = NULL' is so meaningless at this location.
		}
	} else
		fInsert  = 1;

	if (fInsert ==1) {
		//delete subtitle pids
		sprintf(szSQL, "DELETE FROM subtitlePID WHERE usServiceID=%d AND uiCurrentChannelNumber=%d", pServiceList->usServiceID, uiCurrentChannelNumber);
		err = TCC_SQLITE3_EXEC(g_pISDBTDB, szSQL, NULL, NULL, NULL);
		if (err != SQLITE_OK) ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);

		// Insert SubTitlePID List into DB.
		sprintf(szSQL,"INSERT INTO subtitlePID (usServiceID, uiCurrentChannelNumber, uiCurrentCountryCode, usNetworkID, uiSt_PID, ucComponent_Tag) VALUES (%d, %d, %d, %d, %d, %d)",
                pServiceList->usServiceID,
                uiCurrentChannelNumber,
                uiCurrentCountryCode,
                pServiceList->usNetworkID,
                pServiceList->stSubtitleInfo[0].ucSubtitle_PID,
                pServiceList->stSubtitleInfo[0].ucComponent_Tag);

		err = TCC_SQLITE3_EXEC(g_pISDBTDB, szSQL, NULL, NULL, NULL);
		if (err != SQLITE_OK) ALOGE("[%s:%d] ERROR : %d\n", __func__, __LINE__, err);
	}

	tcc_dxb_channel_db_lock_off();

	return (err == SQLITE_OK);
}


BOOL UpdateDB_Logo_Info (T_ISDBT_LOGO_INFO *pstLogoInfo)
{
	int err = 0;
	char szSQL[1024] = { 0 };
	sqlite3_stmt *stmt = NULL;
	int	i, rc = SQLITE_ERROR, Ver = 0;

	/*----- check if parsing CDT is correct or not -------*/
	if (pstLogoInfo == NULL) {
		ALOGE("In %s, parameter is null\n", __func__);
		return err;
	}

#if 0
	if ((tcc_manager_demux_get_state() == E_DEMUX_STATE_SCAN)
		|| (tcc_manager_demux_get_state() == E_DEMUX_STATE_INFORMAL_SCAN)) {
		return 0;
	}
#endif

#if 0
	ALOGE("==== CDT logo ===\n");
	ALOGE("download_data_id=%d\n", pstLogoInfo->download_data_id);
	ALOGE("section_no=%d/%d\n", pstLogoInfo->ucSection, pstLogoInfo->ucLastSection);
	ALOGE("network_id=0x%x\n", pstLogoInfo->original_network_id);
	ALOGE("logo_type=%d\n", pstLogoInfo->logo_type);
	ALOGE("logo_id=%d\n", pstLogoInfo->logo_id);
	ALOGE("logo_ver=%d\n", pstLogoInfo->logo_version);
	ALOGE("logo_length=%d\n", pstLogoInfo->data_size_length);

{
	int i=0;
	for (i=0; i < 20; i++)
		ALOGE("%02x", pstLogoInfo->data_byte[i]);
}
#endif

	tcc_dxb_channel_db_lock_on();

	if (g_pISDBTLogoDB != NULL) {
		/* quary logo_ver */
		sprintf(szSQL, "SELECT Ver FROM logo WHERE NetID=%d AND DWNL_ID=%d AND LOGO_ID=%d AND LOGO_TYPE=%d",
						pstLogoInfo->original_network_id,
						pstLogoInfo->download_data_id,
						pstLogoInfo->logo_id,
						pstLogoInfo->logo_type);
		if (sqlite3_prepare_v2(g_pISDBTLogoDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK) {
			rc = sqlite3_step(stmt);
			if (rc == SQLITE_ROW) {
				Ver = sqlite3_column_int(stmt, 0);
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
			stmt = NULL;
		}

		if (rc == SQLITE_ROW)	{
			/* found logo */
			if (Ver != pstLogoInfo->logo_version) {
				sprintf(szSQL, "UPDATE logo SET Size=%d, Image=? WHERE  NetID=%d AND DWNL_ID=%d AND LOGO_ID=%d AND LOGO_TYPE=%d",
					pstLogoInfo->data_size_length,
					pstLogoInfo->original_network_id,
					pstLogoInfo->download_data_id,
					pstLogoInfo->logo_id,
					pstLogoInfo->logo_type);
				if (sqlite3_prepare_v2(g_pISDBTLogoDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK) {
					sqlite3_reset (stmt);
					err = sqlite3_bind_blob (stmt, 1, pstLogoInfo->data_byte, pstLogoInfo->data_size_length, SQLITE_TRANSIENT);
					if (err != SQLITE_OK) ALOGE("[%s:%d] ERROR : %d\n", __func__, __LINE__, err);

					err = sqlite3_step(stmt);
					if ((err != SQLITE_ROW) && (err != SQLITE_DONE)) ALOGE("[%s:%d] ERROR : %d\n", __func__, __LINE__, err);

					err = 1;
				}
				if (stmt) {
					sqlite3_finalize(stmt);
//					stmt = NULL;	// I think 'stmt = NULL' is so meaningless at this location.
				}
			} else {
				/* there is already same logo */
				err = 1;
			}
		} else if (rc == SQLITE_DONE) {
			/* there is no logo */
			sprintf(szSQL, "INSERT INTO logo (NetID, DWNL_ID, LOGO_ID, LOGO_Type, Ver, Size, Image) VALUES(%d, %d, %d, %d, %d, %d, ?)",
                    pstLogoInfo->original_network_id,
                    pstLogoInfo->download_data_id,
                    pstLogoInfo->logo_id,
                    pstLogoInfo->logo_type,
                    pstLogoInfo->logo_version,
                    pstLogoInfo->data_size_length);

			if (sqlite3_prepare_v2(g_pISDBTLogoDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK) {
				sqlite3_reset (stmt);
				err = sqlite3_bind_blob (stmt, 1, pstLogoInfo->data_byte, pstLogoInfo->data_size_length, SQLITE_TRANSIENT);
				if (err != SQLITE_OK) ALOGE("[%s:%d] ERROR : %d\n", __func__, __LINE__, err);

				err = sqlite3_step(stmt);
				if ((err != SQLITE_ROW) && (err != SQLITE_DONE)) ALOGE("[%s:%d] ERROR : %d\n", __func__, __LINE__, err);

				err = 1;
			}
			if (stmt) {
				sqlite3_finalize(stmt);
//				stmt = NULL;	// I think 'stmt = NULL' is so meaningless at this location.
			}
		} else {
			/* error */
			err = 0;
		}
	}

	tcc_dxb_channel_db_lock_off();

#if 0
	/*** test code ***/
	if (pstLogoInfo->logo_type == 5) {
		sprintf(szSQL, "SELECT Image FROM logo WHERE NetID=%d AND DWNL_ID=%d AND LOGO_ID=%d AND LOGO_TYPE=%d",
								pstLogoInfo->original_network_id,
								pstLogoInfo->download_data_id,
								pstLogoInfo->logo_id,
								pstLogoInfo->logo_type);
		if (sqlite3_prepare_v2(g_pISDBTLogoDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK) {
			sqlite3_reset(stmt);
			err = sqlite3_step(stmt);
			if (err == SQLITE_ROW) {
				int i;
				int image_size;
				unsigned char arrImage[1024];

				image_size = sqlite3_column_bytes(stmt, 0);
				ALOGE("image_size=%d\n", image_size);
				if (image_size > 0) {
 					if (image_size > 1024) image_size = 1024; //check size of local buffer. In fact, size of decoded PNG ARGB888 could be 10KB.
					memcpy(arrImage, sqlite3_column_blob(stmt, 0), image_size);
					if (image_size > 20) image_size = 20;
 					for (i=0; i < image_size; i++)
						ALOGE("%02x ", arrImage[i]);
				}
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
			stmt = NULL;
		}
	}
#endif

	return err;
}

int UpdateDB_ChannelTablePreset (int area_code)
{
	int err = 0;
	int iValidChannelNo, cnt;

	unsigned char channelNumber, countryCode, remoconID, serviceType, regionID;
	unsigned short TStreamID, networkID, serviceID;
	char *TSName;
	int audioPID = 0x1FFF;
	int videoPID = 0x1FFF;
	int stPID = 0x1FFF;
	int siPID = 0x1FFF;
	int PMT_PID = 0x1FFF;
	int CA_ECM_PID = 0x1FFF;
	int AC_ECM_PID = 0x1FFF;
	int PCR_PID = 0x1FFF;
	int threedigitNumber = 0;
	int def_video_type;
	int berAVG = -500;

	int nameLength;
	unsigned short uniLength;

	sqlite3_stmt *stmt = NULL;
	char szSQL[1024] = { 0 };
	short pUniStr[256];

	if (TCC_Isdbt_IsSupportJapan())	//it's useful only for japan.
	{

		//ALOGD("In %s (%d)\n", __func__, area_code);

		ISDBT_Init_AllPresetInfo(0);	//0 - Japna, 1 - Brazil

		err =  ISDBT_Set_CurPresetChannelListByRegion ((unsigned char)area_code);
		if (err == 0)	// if fail
		{
			PRESET_DBG("In %s error while making preset\n", __func__);
			return -1;
		}

		iValidChannelNo = ISDBT_Get_ValidChannelNums();

		if (iValidChannelNo <= 0)
		{
			PRESET_DBG ("In %s %d no channels\n", __func__, __LINE__);
			return -2;
		}

		if (g_pISDBTDB != NULL)
		{
			DeleteDB_Table();

			tcc_dxb_channel_db_lock_on();

			for (cnt=0; cnt < iValidChannelNo; cnt++)
			{
				channelNumber = ISDBT_Get_CurChannelFrequencyIndex (cnt);
				countryCode = 0;		//Japan
				remoconID = ISDBT_Get_CurChannelRemoconIDIndex(cnt);
				if (TCC_Isdbt_IsSupportProfileA())
				{
					serviceType = SERVICE_TYPE_FSEG;		//fullseg
					def_video_type = 2;		//mpeg2
					serviceID = ISDBT_Get_CurTVServiceIDIndex(cnt);
				}
				else
				{
					serviceType = SERVICE_TYPE_1SEG;		//1seg
					def_video_type = 0x1B;		//H.264
					serviceID = ISDBT_Get_CurPRServiceIDIndex(cnt);
					if(area_code == 23 && channelNumber == 15 && remoconID == 12)
						continue;
				}
				regionID = ISDBT_Get_CurPresetRegionID(cnt);
				TStreamID = networkID = ISDBT_Get_CurTStreamIDIndex(cnt);
				TSName = (char *)ISDBT_Get_CurChannelNameIndex(cnt);
				memset(pUniStr, 0, 512);
				uniLength = ISDBTString_MakeUnicodeString(TSName, pUniStr, Get_Language_Code());

				/*------ channel -------*/
				PRESET_DBG("In %s INSERT channels, ch=%d, rmc=%d, reginID=%d\n", __func__, channelNumber, remoconID, regionID);
				sprintf(szSQL,  "INSERT INTO channels (channelNumber, countryCode, remoconID, serviceType, regionID, TStreamID, berAVG, audioPID, videoPID, stPID, siPID, PMT_PID, CA_ECM_PID, AC_ECM_PID, serviceID, threedigitNumber, channelName, preset, networkID, uiTotalAudio_PID, uiTotalVideo_PID, uiPCR_PID, ucAudio_StreamType, ucVideo_StreamType, usOrig_Net_ID) VALUES (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, '%s', %d, %d, %d, %d, %d, %d, %d, %d)", \
						channelNumber, countryCode, remoconID, serviceType, regionID, TStreamID, berAVG, audioPID, videoPID, stPID, siPID, PMT_PID, CA_ECM_PID, AC_ECM_PID, serviceID, threedigitNumber, "NoName", 1, networkID, 1, 1, PCR_PID, 0xF, def_video_type, networkID);
				err = TCC_SQLITE3_EXEC(g_pISDBTDB, szSQL, NULL, NULL, NULL);
				if (err != SQLITE_OK)
				{
					PRESET_DBG("In %s %d failed err = %x TCC_SQLITE3_EXEC\n", __func__, __LINE__, err);
					break;
				}

				sprintf(szSQL, "UPDATE channels SET TSName=?, channelName=? WHERE channelNumber=%d AND TStreamID=%d",
						channelNumber, TStreamID);
				if(sqlite3_prepare_v2(g_pISDBTDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK)
				{
					sqlite3_reset(stmt);
					sqlite3_bind_text16(stmt, 1, pUniStr, -1, SQLITE_STATIC);
					sqlite3_bind_text16(stmt, 2, pUniStr, -1, SQLITE_STATIC);
					sqlite3_step(stmt);
				}
				if (stmt) {
					sqlite3_finalize(stmt);
//					stmt = NULL;	// I think 'stmt = NULL' is so meaningless at this location.
				}
			}

			tcc_dxb_channel_db_lock_off();
		}
		else
		{
			ALOGE ("%s %d no db handle\n", __func__, __LINE__);
		}
	}
	else
	{
		//empty
	}

	return err;
}

int UpdateDB_Strength_ChannelTable (int strength_index, int service_id)
{
	int	err = SQLITE_ERROR;

	tcc_dxb_channel_db_lock_on();
	if (g_pISDBTDB != NULL)
	{
		err = TCCISDBT_ChannelDB_UpdateStrength(strength_index, service_id);
	}
	tcc_dxb_channel_db_lock_off();

	return (err == SQLITE_OK);
}

int UpdateDB_Arrange_ChannelTable(void)
{
	int	err = SQLITE_ERROR;
	SERVICE_ARRANGE_INFO_T ServiceArrangeInfo;

	if (g_pISDBTDB != NULL) {
		tcc_dxb_channel_db_lock_on();
		err = TCCISDBT_ChannelDB_ArrangeTable(g_pISDBTDB);
		tcc_dxb_channel_db_lock_off();
	}
	return (err==SQLITE_OK);
}

int UpdateDB_Get_ChannelInfo (SERVICE_FOUND_INFO_T *p)
{
	int err = 0;
	if (g_pISDBTDB != NULL) {
		tcc_dxb_channel_db_lock_on();
		err = TCCISDBT_ChannelDB_GetChannelInfo (p);
		tcc_dxb_channel_db_lock_off();
	}
	return err;
}

int UpdateDB_Set_ArrangementType(int iType)
{
	int err = SQLITE_ERROR;
	tcc_dxb_channel_db_lock_on();
	if (g_pISDBTDB != NULL)
		err = TCCISDBT_ChannelDB_SetArrangementType(iType);
	tcc_dxb_channel_db_lock_off();
	return err;
}

int UpdateDB_Get_ChannelSvcCnt (void)
{
	int err = 0;
	if (g_pISDBTDB != NULL) {
		tcc_dxb_channel_db_lock_on();
		err = TCCISDBT_ChannelDB_GetChannelSvcCnt();
		tcc_dxb_channel_db_lock_off();
	}
	return err;
}

int UpdateDB_Del_AutoSearch (void)
{
	int err = 0;

	if (g_pISDBTDB != NULL) {
		tcc_dxb_channel_db_lock_on();
		err = TCCISDBT_ChannelDB_DelAutoSearch(g_pISDBTDB);
		tcc_dxb_channel_db_lock_off();
	}
	return err;
}

int UpdateDB_Set_AutosearchInfo (SERVICE_FOUND_INFO_T *p)
{
	int err_channel = 0;

	if (p == NULL) {
		ALOGD("In %s, arg error", __func__);
		return -1;
	}
	if (p->found_no <=0) {
		ALOGD("In %s, no channel found",__func__);
		return -1;
	}
	if (g_pISDBTDB != NULL) {
		tcc_dxb_channel_db_lock_on();
		err_channel = TCCISDBT_ChannelDB_SetAutosearchInfo(g_pISDBTDB, p);
		tcc_dxb_channel_db_lock_off();
	}
	if (err_channel == SQLITE_OK)
		return 0;
	else
		return -1;
}

/* check whether the channel info found by autosearch should be skipped or not. */
int UpdateDB_ArrangeAutoSearch (SERVICE_FOUND_INFO_T *p)
{
	int err=0;
	if (p==NULL) return -1;
	if (p->found_no <= 0)	return -1;
	if (g_pISDBTDB != NULL) {
		tcc_dxb_channel_db_lock_on();
		err = TCCISDBT_ChannelDB_ArrangeAutoSearch(g_pISDBTDB, p);
		tcc_dxb_channel_db_lock_off();
	}
	return err;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EPG P/F and Schedule
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
sqlite3 *GetDB_EPG_Handle(void)
{
	return g_pISDBTEPGDB;
}

int EPGDB_BeginTransaction(void)
{
	int err = SQLITE_ERROR;
	err = TCC_SQLITE3_EXEC(g_pISDBTEPGDB, "begin;", NULL, NULL, NULL);
	if (err != SQLITE_OK) ALOGE("[%s:%d] ERROR : %d\n", __func__, __LINE__, err);

	return 0;
}

int EPGDB_EndTransaction(void)
{
	int err = SQLITE_ERROR;
	err = TCC_SQLITE3_EXEC(g_pISDBTEPGDB, "commit;", NULL, NULL, NULL);
	if (err != SQLITE_OK) ALOGE("[%s:%d] ERROR : %d\n", __func__, __LINE__, err);

	return 0;
}

int CreateDB_EPG_Table(int iTableType, int iChannelNumber, int iNetworkID, int iServiceID, int iServiceType)
{
	char *szTableType[2] = {"EPG_PF", "EPG_Sche"};
	char szTableName[64] = { 0 };
	char szSQL[1024] = { 0 };
	int iCount = 0;
	sqlite3_stmt *stmt = NULL;
	int err = SQLITE_ERROR;

	if (iTableType > EPG_SCHEDULE) {
		ALOGE("[%s] CREATE TABLE EPGDB failed", __func__);
		return SQLITE_ERROR;
	}

	if (g_pISDBTEPGDB != NULL) {
		sprintf(szTableName, "%s_%d_0x%x", szTableType[iTableType], iChannelNumber, iServiceID);
		sprintf(szSQL, "create table if not exists %s (_id integer primary key, \
uiUpdatedFlag integer, uiTableId integer, uiCurrentChannelNumber integer, uiCurrentCountryCode integer, \
ucVersionNumber integer, ucSection integer, ucLastSection integer, ucSegmentLastSection integer, OrgNetworkID integer, \
TStreamID integer, usServiceID integer, EventID integer, Start_MJD integer, Start_HH integer, Start_MM integer, \
Start_SS integer, Duration_HH integer, Duration_MM integer, Duration_SS integer, iLen_EvtName integer, \
EvtName text, iLen_EvtText integer, EvtText text, iLen_ExtEvtItem integer, ExtEvtItem text, iLen_ExtEvtText integer, \
ExtEvtText text, Genre integer, AudioMode integer, AudioSr integer, AudioLang1 integer, AudioLang2 integer, \
VideoMode integer, iRating integer, RefServiceID integer, RefEventID integer, MultiOrCommonEvent integer)", 
szTableName);

		err = TCC_SQLITE3_EXEC(g_pISDBTEPGDB, szSQL, NULL, NULL, NULL);
		if (err != SQLITE_OK) {
			ALOGE("[%s] CREATE TABLE EPGDB failed", __func__);
			return err;
		}

		sprintf(szSQL, "SELECT COUNT(_id) FROM EPG_Table WHERE serviceID=%d AND channelNumber=%d AND networkID=%d AND tableType=%d", iServiceID, iChannelNumber, iNetworkID, iTableType);
		if (sqlite3_prepare_v2(g_pISDBTEPGDB, szSQL, -1, &stmt, NULL) == SQLITE_OK) {
			err = sqlite3_step(stmt);
			if (err == SQLITE_ROW) {
				iCount = sqlite3_column_int(stmt, 0);
			}
			if ((err != SQLITE_ROW) && (err != SQLITE_DONE)){
				ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
			stmt = NULL;
		}

		if (iCount == 0) {
			sprintf(szSQL,
					"INSERT INTO EPG_Table (channelNumber, networkID, serviceID, tableType, MJD, iLen_TableName, TableName) VALUES (%d, %d, %d, %d, 0, %d, ?)",
					iChannelNumber,	iNetworkID, iServiceID,	iTableType, strlen(szTableName));

			if (sqlite3_prepare_v2(g_pISDBTEPGDB, szSQL, -1, &stmt, NULL) == SQLITE_OK)
			{
				err = sqlite3_reset(stmt);
				if (err != SQLITE_OK) ALOGE("[%s:%d] ERROR : %d\n", __func__, __LINE__, err);

				err = sqlite3_bind_text(stmt, 1, szTableName, -1, SQLITE_STATIC);
				if (err != SQLITE_OK) ALOGE("[%s:%d] ERROR : %d\n", __func__, __LINE__, err);

				err = sqlite3_step(stmt);
				if ((err != SQLITE_ROW) && (err != SQLITE_DONE)) ALOGE("[%s:%d] ERROR : %d\n", __func__, __LINE__, err);
			}
			if (stmt) {
				sqlite3_finalize(stmt);
//				stmt = NULL;	//	I think 'stmt = NULL' is so meaningless at this location.
			}
		}
	}

	return err;
}

int DestroyDB_EPG_Table(char *szTableName)
{
	char szSQL[1024] = { 0 };
//	sqlite3_stmt *stmt = NULL;
	int err = SQLITE_ERROR;

	if (g_pISDBTEPGDB != NULL) {
		sprintf(szSQL, "drop table if exists %s", szTableName);
		err = TCC_SQLITE3_EXEC(g_pISDBTEPGDB, szSQL, NULL, NULL, NULL);
		if (err != SQLITE_OK) ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);
	}

	return err;
}

int OpenDB_EPG_Table(void)
{
	int err = SQLITE_OK;
	int cnt;

	tcc_dxb_epg_db_lock_init();

	TCCISDBT_EPGDB_Init();

	tcc_dxb_epg_db_lock_on();
#if defined(HAVE_LINUX_PLATFORM)
	if(isdbt_epgdb_path0[0] == 0) {
		isdbt_db_path = getenv("DXB_DATA_PATH");
		if(isdbt_db_path == NULL) {
			isdbt_db_path = default_isdbt_db_path;
		}

		memset(isdbt_epgdb_path0, 0x0, ISDBT_DB_PATH_SIZE);
		if( (strlen(isdbt_db_path) + strlen("/ISDBTEPG0.db")) < ISDBT_DB_PATH_SIZE ) // Noah, To avoid Codesonar's warning, Buffer Overrun
			sprintf(isdbt_epgdb_path0, "%s%s", isdbt_db_path, "/ISDBTEPG0.db");

		memset(isdbt_epgdb_path1, 0x0, ISDBT_DB_PATH_SIZE);
		if( (strlen(isdbt_db_path) + strlen("/ISDBTEPG1.db")) < ISDBT_DB_PATH_SIZE ) // Noah, To avoid Codesonar's warning, Buffer Overrun
			sprintf(isdbt_epgdb_path1, "%s%s", isdbt_db_path, "/ISDBTEPG1.db");

		memset(isdbt_epgdb_path2, 0x0, ISDBT_DB_PATH_SIZE);
		if( (strlen(isdbt_db_path) + strlen("/ISDBTEPG2.db")) < ISDBT_DB_PATH_SIZE ) // Noah, To avoid Codesonar's warning, Buffer Overrun
			sprintf(isdbt_epgdb_path2, "%s%s", isdbt_db_path, "/ISDBTEPG2.db");

		sISDBTEPGDBPath[0] = isdbt_epgdb_path0;
		sISDBTEPGDBPath[1] = isdbt_epgdb_path1;
		sISDBTEPGDBPath[2] = isdbt_epgdb_path2;
	}
#endif
	tcc_dxb_epg_db_lock_off();

	g_pISDBTEPGDB = NULL;
	for (cnt = 0; cnt < ISDBT_PRESET_MODE_MAX;cnt++)
	{
		if (g_pISDBTEPGDB_Handle[cnt] == NULL) {
			if ((err = sqlite3_open_v2(sISDBTEPGDBPath[cnt], &g_pISDBTEPGDB_Handle[cnt], SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE|SQLITE_OPEN_FULLMUTEX, NULL)) != SQLITE_OK) {
				ALOGE("[%s] g_pISDBTEPGDB open fail\n", __func__);
				return err;
			}
			else {
				sqlite3_busy_timeout(g_pISDBTEPGDB_Handle[cnt], ISDBT_SQLITE_BUSY_TIMEOUT);
			}
			err = TCC_SQLITE3_EXEC(g_pISDBTEPGDB_Handle[cnt],
							   "create table if not exists EPG_Table (_id integer primary key, \
							   	channelNumber integer, \
								networkID integer, \
							   	serviceID integer, \
								tableType integer, \
								MJD integer, \
							   	iLen_TableName integer, \
							   	TableName text)", \
							   	NULL, NULL, NULL);
			if (err != SQLITE_OK) {
				ALOGE("CANNOT CREATE EPG_Table Tables !!! %d", err);
				return err;
			}
		}
		else{
			ALOGE("[%s] WARNING - g_pISDBTEPGDB[%d] is already created.", __func__, cnt);
		}
	}
	if (gCurrentEPGDBIndex == -1) {
		g_pISDBTEPGDB = g_pISDBTEPGDB_Handle[0];
		gCurrentEPGDBIndex = 0;
	} else if (gCurrentEPGDBIndex >= 0 && gCurrentEPGDBIndex < ISDBT_PRESET_MODE_MAX)
		g_pISDBTEPGDB = g_pISDBTEPGDB_Handle[gCurrentEPGDBIndex];
	else {
		ALOGD("In %s, gCurrentEPGDBIndex is invalid", __func__);
		g_pISDBTEPGDB = g_pISDBTEPGDB_Handle[0];
		gCurrentEPGDBIndex = 0;
	}

	return err;
}

int CloseDB_EPG_Table(void)
{
	int err = SQLITE_ERROR;
	int cnt;

	tcc_dxb_epg_db_lock_deinit();

	TCCISDBT_EPGDB_DeInit();

	for(cnt=0; cnt < ISDBT_PRESET_MODE_MAX; cnt++)
	{
		if (g_pISDBTEPGDB_Handle[cnt] != NULL) {
			err = sqlite3_close(g_pISDBTEPGDB_Handle[cnt]);
		if (err != SQLITE_OK) ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);
			g_pISDBTEPGDB_Handle[cnt] = NULL;
		}
	}
	g_pISDBTEPGDB = NULL;
	gCurrentEPGDBIndex = -1;

	return 0;
}

int ChangeDB_EPG_Handle (int index, int *prev_handle)
{
	int err = SQLITE_ERROR;

	ALOGD("ChangeDB_EPG_Handle:%d",index);
	if (index < 0 || index >= ISDBT_PRESET_MODE_MAX)
		return err;

	tcc_dxb_epg_db_lock_on();
	if (g_pISDBTEPGDB_Handle[index] != NULL) {
		*prev_handle = gCurrentEPGDBIndex;
		g_pISDBTEPGDB = g_pISDBTEPGDB_Handle[index];
		gCurrentEPGDBIndex = index;
		err = SQLITE_OK;

		TCCISDBT_EPGDB_DeleteAll();
		ALOGD("ChangeDB_EPG_Handle ok");
	}
	tcc_dxb_epg_db_lock_off();
	return err;
}

int DeleteDB_EPG_PF_Table(int iChannelNumber, int iServiceID, int iTableID)
{
	char szSQL[1024] = { 0 };
	int err = SQLITE_ERROR;

	tcc_dxb_epg_db_lock_on();

	TCCISDBT_EPGDB_Delete(EPG_PF, iServiceID, iTableID);

	if (g_pISDBTEPGDB != NULL) {
		sprintf(szSQL, "DELETE FROM EPG_PF_%d_0x%x WHERE uiTableId=%d", iChannelNumber, iServiceID, iTableID);
		err = TCC_SQLITE3_EXEC(g_pISDBTEPGDB, szSQL, NULL, NULL, NULL);
		if (err != SQLITE_OK) ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);
	}

	tcc_dxb_epg_db_lock_off();

	return err;
}

int DeleteDB_EPG_Schedule_Table(int iChannelNumber, int iServiceID, int iTableID, int iSectionNumber, int iVersionNumber)
{
	char szSQL[1024] = { 0 };
	int err = SQLITE_ERROR;

	tcc_dxb_epg_db_lock_on();

	TCCISDBT_EPGDB_Delete(EPG_SCHEDULE, iServiceID, iTableID);

	if (g_pISDBTEPGDB != NULL) {
		if (gstCurrentDateTime.uiMJD != 0) {
			sprintf(szSQL, "DELETE FROM EPG_Sche_%d_0x%x WHERE Start_MJD<%d", iChannelNumber, iServiceID, gstCurrentDateTime.uiMJD);
			err = TCC_SQLITE3_EXEC(g_pISDBTEPGDB, szSQL, NULL, NULL, NULL);
			if (err != SQLITE_OK) ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);
		}

		sprintf(szSQL, "DELETE FROM EPG_Sche_%d_0x%x \
WHERE uiTableId=%d AND ucSection=%d AND ucVersionNumber!=%d",
iChannelNumber, iServiceID, iTableID, iSectionNumber, iVersionNumber);

		err = TCC_SQLITE3_EXEC(g_pISDBTEPGDB, szSQL, NULL, NULL, NULL);
		if (err != SQLITE_OK) ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);
	}

	tcc_dxb_epg_db_lock_off();

	return err;
}

int GetCountDB_EPG_Table
(
	int iTableType,
	int uiCurrentChannelNumber,
	int uiCurrentCountryCode,
	int uiEventID,
	P_ISDBT_EIT_STRUCT pstEIT
)
{
	int err = SQLITE_ERROR;
	int listCount = 0;
//	sqlite3_stmt *stmt = NULL;

	tcc_dxb_epg_db_lock_on();

	if (g_pISDBTEPGDB != NULL) {
		listCount = TCCISDBT_EPGDB_GetCount(iTableType, uiCurrentChannelNumber, uiCurrentCountryCode, uiEventID, pstEIT);
	}

	tcc_dxb_epg_db_lock_off();

	return listCount;
}

BOOL InsertDB_EPG_Table
(
	int iTableType,
	int uiCurrentChannelNumber,
	int uiCurrentCountryCode,
	P_ISDBT_EIT_STRUCT pstEIT,
	P_ISDBT_EIT_EVENT_DATA pstEITEvent
)
{
	int err = SQLITE_ERROR;

	tcc_dxb_epg_db_lock_on();

	if (g_pISDBTEPGDB != NULL) {
		err = TCCISDBT_EPGDB_Insert(iTableType, uiCurrentChannelNumber, uiCurrentCountryCode, pstEIT, pstEITEvent);
	}

	tcc_dxb_epg_db_lock_off();

	return (err == SQLITE_OK);
}

BOOL UpdateDB_EPG_XXXTable
(
	int iTableType,
	int uiEvtNameLen,
	char *pEvtName,
	int uiEvtTextLen,
	char *pEvtText,
	int uiCurrentChannelNumber,
	int uiCurrentCountryCode,
	P_ISDBT_EIT_STRUCT pstEIT,
	P_ISDBT_EIT_EVENT_DATA pstEITEvent
)
{
	int err = SQLITE_ERROR;

	tcc_dxb_epg_db_lock_on();

	if (g_pISDBTEPGDB != NULL) {
		err = TCCISDBT_EPGDB_UpdateEvent(iTableType, uiEvtNameLen, pEvtName, uiEvtTextLen, pEvtText, \
										uiCurrentChannelNumber, uiCurrentCountryCode, pstEIT, pstEITEvent);
	}

	tcc_dxb_epg_db_lock_off();

	return (err == SQLITE_OK);
}

BOOL UpdateDB_EPG_XXXTable_ExtEvent
(
	int iTableType,
	int uiCurrentChannelNumber,
	int uiCurrentCountryCode,
	P_ISDBT_EIT_STRUCT pstEIT,
	P_ISDBT_EIT_EVENT_DATA pstEITEvent,
	EXT_EVT_DESCR *pstEED
)
{
	int err = SQLITE_ERROR;

	tcc_dxb_epg_db_lock_on();

	if (g_pISDBTEPGDB != NULL) {
		err = TCCISDBT_EPGDB_UpdateExtEvent(iTableType, uiCurrentChannelNumber, uiCurrentCountryCode, pstEIT, pstEITEvent, pstEED);
	}

	tcc_dxb_epg_db_lock_off();

	return (err == SQLITE_OK);
}

BOOL UpdateDB_EPG_XXXTable_EvtGroup
(
	int iTableType,
	int uiRefServiceID,
	int uiRefEventID,
	int uiCurrentChannelNumber,
	int uiCurrentCountryCode,
	P_ISDBT_EIT_STRUCT pstEIT,
	P_ISDBT_EIT_EVENT_DATA pstEITEvent
)
{
	int err = SQLITE_ERROR;

	tcc_dxb_epg_db_lock_on();

	if (g_pISDBTEPGDB != NULL) {
		err = TCCISDBT_EPGDB_UpdateEvtGroup(iTableType, uiRefServiceID, uiRefEventID, \
											uiCurrentChannelNumber, uiCurrentCountryCode, pstEIT, pstEITEvent);
	}

	tcc_dxb_epg_db_lock_off();

	return (err == SQLITE_OK);
}

/* content_descriptor in EIT
  * uiContentGenre
  *		0 = News
  *		1 = Sorts
  *		2 = Information/tabloid show
  *		3 = Drama
  *		4 = Music
  *		5 = Variety show
  *		6 = Movie
  *		7 = Animaiton
  *		8 = Documentary
  *		9 = Play/Performance
  *		A = Hobby/Education
  *		B = Welfare
  *		C/D = (spare)
  *		E = (extended info for BS/CS)
  *		F = Others
  */
BOOL UpdateDB_EPG_Content(
	int iTableType,
	unsigned int uiContentGenre,
	int uiCurrentChannelNumber,
	int uiCurrentCountryCode,
	P_ISDBT_EIT_STRUCT pstEIT,
	P_ISDBT_EIT_EVENT_DATA pstEITEvent)
{
	int	err = -1;

	tcc_dxb_epg_db_lock_on();

	if ((uiContentGenre <= 0xF) && (uiContentGenre != 0xE) && (uiContentGenre != 0xC) && (uiContentGenre != 0xD)) {
		if (g_pISDBTEPGDB != NULL) {
			err = TCCISDBT_EPGDB_UpdateContent(iTableType,
						uiContentGenre,
						uiCurrentChannelNumber,
						uiCurrentCountryCode,
						pstEIT,
						pstEITEvent );
		}
}

	tcc_dxb_epg_db_lock_off();

	return (err==0);
}

BOOL UpdateDB_EPG_Rating(
	int iTableType,
	unsigned int uiRating,
	int uiCurrentChannelNumber,
	int uiCurrentCountryCode,
	P_ISDBT_EIT_STRUCT pstEIT,
	P_ISDBT_EIT_EVENT_DATA pstEITEvent)
{
	return 0;
}

/*  audio_component_descriptor in EIT
  *  component_type
  *		01 = 1/0 (single mono)
  *		02 = 1/0+1/0 (dual mono)
  *		03 = 2/0 (stereo)
  *		04 = 2/1
  *		05 = 3/0
  *		06 = 2/2
  *		07 = 3/1
  *		08 = 3/2
  *		09 = 3/2+LFE
  *		0A = 3/4+LFE
  */
static unsigned int uiaLangCodeTable[11] = {
	0x6A706E,		/* jpn */
	0x656E67,		/* eng */
	0x646575,		/* deu */
	0x667261,		/* fra */
	0x697461, 		/* ita */
	0x727573,		/* rus */
	0x7A686F,		/* zho */
	0x6B6F72,		/* kor */
	0x737061,		/* spa */
	0x627261,		/* bra */
	0x657463,		/* etc */
};
static int LangCodeValid (unsigned int uiLangCode)
{
	int size, i;
	size = sizeof(uiaLangCodeTable) / sizeof(unsigned int);
	for (i=0; i < size; i++)
	{
		if (uiaLangCodeTable[i] == uiLangCode)
			break;
	}
	if (i==size)	return 0;
	else			return 1;
}

BOOL UpdateDB_EPG_AudioInfo (
	int iTableType,
	unsigned int component_tag,
	int stream_type,					/* 0xF=ADTS, 0x11=LATM */
	int component_type,
	int fMultiLingual, 					/* 1=bilingual (dual-mono) */
	int sampling_rate, 				/* 1=16KHz, 2=22.05KHz, 3=24KHz, 5=32KHz, 6=44.1KHz, 7=48KHz */
	unsigned char *psLangCode1,
	unsigned char *psLangCode2,
	int uiCurrentChannelNumber,
	int uiCurrentCountryCode,
	P_ISDBT_EIT_STRUCT pstEIT,
	P_ISDBT_EIT_EVENT_DATA pstEITEvent)
{
	int err = -1, i;
	unsigned int uiLangCode1, uiLangCode2;

	tcc_dxb_epg_db_lock_on();

	uiLangCode1 = uiLangCode2 = 0;
	for ( i=0; i < 3; i++)
	{
		uiLangCode1 = (uiLangCode1 << 8) | *psLangCode1++;
		if (fMultiLingual)
			uiLangCode2 = (uiLangCode2 << 8) | *psLangCode2++;
	}
	if (!LangCodeValid(uiLangCode1)) uiLangCode1 = 0;
	if (!LangCodeValid(uiLangCode2)) uiLangCode2 = 0;
	if ((0<= component_type && component_type <= 0xA) && (stream_type==0x3 || stream_type==0xF || stream_type==0x11)) {
		if (g_pISDBTEPGDB != NULL) {
			err = TCCISDBT_EPGDB_UpdateAudioInfo (iTableType,
					component_tag,
					component_type,
					sampling_rate,
					uiLangCode1,
					uiLangCode2,
					uiCurrentChannelNumber	,
					uiCurrentCountryCode,
					pstEIT,
					pstEITEvent);
		}
	}

	tcc_dxb_epg_db_lock_off();

	return (err==0);
}

/* component_descriptor in EIT
  * component_type
  *		01 = 480i, 4:3
  *		02 = 480i, 16:9 with pan vector
  *		03 = 480i, 16:9 without pan vector
  *		04 = 480i, > 16:9
  *		A1 = 480p, 4:3
  *		A2 = 480p, 16:9 with pan vector
  *		A3 = 480p, 16:0 without pan vector
  *		A4 = 480p, > 16:9
  *		B1 = 1080i, 4:3
  *		B2 = 1080i, 16:9 with pan vector
  *		B3 = 1080i, 16:9 without pan vector
  *		B4 = 1080i, > 16:9
  *		C1 = 720p, 4:3
  *		C2 = 720p, 16:9 with pan vector
  *		C3 = 720p, 16:9 without pan vector
  *		C4 = 720p, > 16:9
  *		D1 = 240p, 4:3
  *		D2 = 240p, 4:3 with pan vector
  *		D3 = 240p, 4:3 without pan vector
  *		D4 = 240p, > 16:9
  */
BOOL UpdateDB_EPG_VideoInfo (
	int iTableType,
	unsigned int component_tag,
	unsigned int component_type,
	int uiCurrentChannelNumber,
	int uiCurrentCountryCode,
	P_ISDBT_EIT_STRUCT pstEIT,
	P_ISDBT_EIT_EVENT_DATA pstEITEvent)
{
	int err = -1;
	unsigned int niH, niL;

	tcc_dxb_epg_db_lock_on();

	niH = (component_type & 0xF0) >> 4;
	niL = (component_type & 0x0F);
	if ((niH <= 0xD) && (niL >= 1 && niL <= 4)) {
		if (g_pISDBTEPGDB != NULL) {
			err = TCCISDBT_EPGDB_UpdateVideoInfo (iTableType,
					component_tag,
					component_type,
					uiCurrentChannelNumber	,
					uiCurrentCountryCode,
					pstEIT,
					pstEITEvent);
		}
	}

	tcc_dxb_epg_db_lock_off();

	return (err==0);
}

void UpdateDB_EPG_Commit (
	int iTotalEventCount,
	int *piEventIDList,
	int uiCurrentChannelNumber,
	int uiCurrentCountryCode,
	P_ISDBT_EIT_STRUCT pstEIT)
{
// Noah, This function is called so many times at EIT
// Access 2 ~ 7 times -> 1 time

	char szSQL[1024] = { 0 };
	sqlite3_stmt *stmt = NULL;
	int rc=SQLITE_ERROR;
	int i=0;
	int	rowID = -1;
	
	tcc_dxb_epg_db_lock_on();

	sprintf(szSQL, "SELECT _id FROM EPG_Table WHERE channelNumber=%d AND networkID=%d AND serviceID=%d", uiCurrentChannelNumber, pstEIT->uiOrgNetWorkID, pstEIT->uiService_ID);
	if (sqlite3_prepare_v2(g_pISDBTEPGDB, szSQL, -1, &stmt, NULL) == SQLITE_OK)
	{
		rc = sqlite3_step(stmt);
		if(rc == SQLITE_ROW)
		{
			rowID = sqlite3_column_int(stmt, 0);
		}
	}

	if (stmt)
	{
		sqlite3_finalize(stmt);
//		stmt = NULL;
	}

	if (-1 != rowID)
	{
		EPGDB_BeginTransaction();

		for (i = 0; i < iTotalEventCount; i++)
		{
			TCCISDBT_EPGDB_CommitEvent(g_pISDBTEPGDB, uiCurrentChannelNumber, uiCurrentCountryCode, pstEIT, piEventIDList[i]);
		}

		EPGDB_EndTransaction();
	}

	tcc_dxb_epg_db_lock_off();

	return;
}


int tcc_db_get_default_video(int iIndex, int iTotalCount, int *pPID, int *pStreamType, int *pComponentTag, int iServiceType)
{
	int i=0;

	if(iTotalCount <= 0) {
		ALOGE("%s Video is not exist\n", __func__);
		return 0;
	}

	if(pPID == NULL || pStreamType == NULL || pComponentTag == NULL) {
		ALOGE("%s Error invalid param(%p, %p, %p, %d)\n", __func__, pPID, pStreamType, pComponentTag, iServiceType);
		return 0;
	}

	if(iTotalCount > 1) {
		if (iIndex == -1) {	/* select default video ES */
			for (i=0; i < iTotalCount; i++)
			{
				if(iServiceType == SERVICE_TYPE_FSEG) {
					if (pComponentTag[i] == 0x00) break;
				} else {
					if (pComponentTag[i] == 0x81) break;
				}
			}

			if ((i < iTotalCount) && (pPID[i] != 0xFFFF) && (pStreamType[i] != 0xFFFF)) {
				ALOGI("%s Found default video index[%d, %d]\n", __func__, iIndex, iTotalCount);
				return i;
			}

			return 0;
		}
		else if (iIndex >= iTotalCount || iIndex < 0) {
			ALOGE("%s Error iIndex is out of range(%d/%d)\n", __func__, iIndex, iTotalCount);
			return 0;
		}

		return iIndex;
	}

	return 0;
}

int tcc_db_get_default_audio(int iIndex, int iTotalCount, int *pPID, int *pStreamType, int *pComponentTag, int iServiceType)
{
	int i=0;

	if(iTotalCount <= 0) {
		ALOGE("%s Audio is not exist\n", __func__);
		return 0;
	}

	if (pPID == NULL || pStreamType == NULL || pComponentTag == NULL) {
		ALOGE("%s Error invalid param(%p, %p, %p, %d)\n", __func__, pPID, pStreamType, pComponentTag, iServiceType);
		return 0;
	}

	if(iTotalCount > 1) {
		if (iIndex == -1) {	/* select default audio ES */
			for (i=0; i < iTotalCount; i++)
			{
				if(iServiceType == SERVICE_TYPE_FSEG) {
					if (pComponentTag[i] == 0x10) break;
				} else {
					if ((pComponentTag[i] == 0x83) || (pComponentTag[i] == 0x85) || (pComponentTag[i] == 0x90)) break;
				}
			}

			if ((i < iTotalCount) && (pPID[i] != 0xFFFF) && (pStreamType[i] != 0xFFFF)) {
				ALOGI("%s Found default audio index[%d, %d]\n", __func__, iIndex, iTotalCount);
				return i;
			}

			return 0;
		}
		else if (iIndex >= iTotalCount || iIndex < 0) {
			ALOGE("%s Error iIndex is out of range(%d/%d)\n", __func__, iIndex, iTotalCount);
			return 0;
		}

		return iIndex;
	}

	return 0;
}

int tcc_db_get_channel_count(void)
{
	int err = SQLITE_ERROR;
	int count = 0;
	sqlite3_stmt *stmt = NULL;
	const char *pszSQL = "SELECT COUNT(DISTINCT channelNumber) FROM channels";

	if (g_pISDBTDB != NULL)
	{
		tcc_dxb_channel_db_lock_on();
		if (sqlite3_prepare_v2(g_pISDBTDB, pszSQL, strlen(pszSQL), &stmt, NULL) == SQLITE_OK){
			if ((err = sqlite3_step(stmt)) == SQLITE_ROW){
				count = sqlite3_column_int(stmt, 0);
			}
			if ((err != SQLITE_ROW) && (err != SQLITE_DONE)){
				ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
//			stmt = NULL;	//	I think 'stmt = NULL' is so meaningless at this location.
		}
		tcc_dxb_channel_db_lock_off();
	}

	return count;
}

int tcc_db_get_channel_data(int *pData, int max_count)
{
	int err = SQLITE_ERROR;
	sqlite3_stmt *stmt = NULL;
	int ch_num;
	const char *pszSQL = "SELECT DISTINCT channelNumber FROM channels";
	int ch_count = 0;

	if (max_count == 0) return 1;
	if (g_pISDBTDB != NULL)
	{
		tcc_dxb_channel_db_lock_on();
		if (sqlite3_prepare_v2(g_pISDBTDB, pszSQL, strlen(pszSQL), &stmt, NULL) == SQLITE_OK)
		{
			err = sqlite3_step(stmt);
			while (err == SQLITE_ROW)
			{
				ch_num = sqlite3_column_int(stmt, 0);
				*pData++ = ch_num;
				ch_count++;
				if (ch_count >= max_count)
					break;

				err = sqlite3_step(stmt);
			}
			if ((err != SQLITE_ROW) && (err != SQLITE_DONE)){
				ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
//			stmt = NULL;	//	I think 'stmt = NULL' is so meaningless at this location.
		}
		tcc_dxb_channel_db_lock_off();
	}

	return (int)(((err == SQLITE_OK)||(err == SQLITE_DONE) || (err == SQLITE_ROW))?0:-1);
}

int tcc_db_get_video(int uiServiceID, int uiCurrentChannelNumber, int* piVideoMaxCount, int *piVideoPID, int *piVideoStreamType, int *piComponentTag)
{
	char szSQL[1024] = { 0 };
	sqlite3_stmt *stmt = NULL;
	int iVideoMaxCount;
	int rc;
	int i;
	int ret = 0;

	if (g_pISDBTDB != NULL)
	{
		iVideoMaxCount = *piVideoMaxCount;

		for(i=0; i<iVideoMaxCount; i++)
		{
			piVideoPID[i] = 0xFFFF;
			piVideoStreamType[i] = 0xFFFF;
			piComponentTag[i] = -1;
		}

		sprintf(szSQL, "SELECT uiVideo_PID, ucVideo_StreamType, ucComponent_Tag FROM videoPID WHERE usServiceID=%d AND uiCurrentChannelNumber=%d", uiServiceID, uiCurrentChannelNumber);
		tcc_dxb_channel_db_lock_on();
		ret = sqlite3_prepare_v2(g_pISDBTDB, szSQL, strlen(szSQL), &stmt, NULL);
		if(ret == SQLITE_OK)
		{
			for(i=0; i<iVideoMaxCount;)
			{
				rc = sqlite3_step(stmt);
				if (rc == SQLITE_DONE)
				{
					break;
				}

				if (rc == SQLITE_ROW)
				{
					piVideoPID[i] = sqlite3_column_int(stmt, 0);
					piVideoStreamType[i] = sqlite3_column_int(stmt, 1);
					piComponentTag[i] = sqlite3_column_int(stmt, 2);
					ALOGI("%s %d (%d) VideoPID=%d, StreamType=%d, component_tag=%d", __func__, __LINE__, i, piVideoPID[i], piVideoStreamType[i], piComponentTag[i]);
					i++;
				}
			}
		}		
		if (stmt) {
			sqlite3_finalize(stmt);
//			stmt = NULL;	// I think 'stmt = NULL' is so meaningless at this location.
		}
		tcc_dxb_channel_db_lock_off();
		*piVideoMaxCount = i;
	}else{
		ret = -1;
	}

	return ret;
}

int tcc_db_get_video_byTag(int uiServiceID, int uiCurrentChannelNumber, int iComponentTag, int *piVideoPID)
{
	char szSQL[1024] = { 0 };
	sqlite3_stmt *stmt = NULL;
	int rc;

	if (g_pISDBTDB != NULL)
	{
		sprintf(szSQL, "SELECT uiVideo_PID FROM videoPID WHERE usServiceID=%d AND uiCurrentChannelNumber=%d AND ucComponent_Tag=%d", uiServiceID, uiCurrentChannelNumber, iComponentTag);
		tcc_dxb_channel_db_lock_on();
		if(sqlite3_prepare_v2(g_pISDBTDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK)
		{
			rc = sqlite3_step(stmt);
			if (rc == SQLITE_ROW)
			{
				*piVideoPID = sqlite3_column_int(stmt, 0);
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
//			stmt = NULL;	//	I think 'stmt = NULL' is so meaningless at this location.
		}
		tcc_dxb_channel_db_lock_off();
	}
	return 0;
}

int tcc_db_get_audio(int uiServiceID, int uiCurrentChannelNumber, int* piAudioMaxCount, int *piAudioPID, int *piAudioStreamType, int *piComponentTag)
{
	char szSQL[1024] = { 0 };
	sqlite3_stmt *stmt = NULL;
	int iAudioMaxCount;
	int rc;
	int i;
	int ret = 0;

	if (g_pISDBTDB != NULL)
	{
		iAudioMaxCount = *piAudioMaxCount;

		for(i=0; i<iAudioMaxCount; i++)
		{
			piAudioPID[i] = 0xFFFF;
			piAudioStreamType[i] = 0xFFFF;
			piComponentTag[i] = -1;
		}

		sprintf(szSQL, "SELECT uiAudio_PID, ucAudio_StreamType, ucComponent_Tag FROM audioPID WHERE usServiceID=%d AND uiCurrentChannelNumber=%d", uiServiceID, uiCurrentChannelNumber);
		tcc_dxb_channel_db_lock_on();
		ret = sqlite3_prepare_v2(g_pISDBTDB, szSQL, strlen(szSQL), &stmt, NULL);
		if(ret == SQLITE_OK)
		{
			for(i=0; i<iAudioMaxCount;)
			{
				rc = sqlite3_step(stmt);
				if (rc == SQLITE_DONE)
				{
					break;
				}

				if (rc == SQLITE_ROW)
				{
					piAudioPID[i] = sqlite3_column_int(stmt, 0);
					piAudioStreamType[i] = sqlite3_column_int(stmt, 1);
					piComponentTag[i] = sqlite3_column_int(stmt, 2);
					ALOGI("%s %d (%d) AudioPID=%d, StreamType=%d, component_tag=%d", __func__, __LINE__, i, piAudioPID[i], piAudioStreamType[i], piComponentTag[i]);
					i++;
				}
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
//			stmt = NULL;	//	I think 'stmt = NULL' is so meaningless at this location.
		}
		tcc_dxb_channel_db_lock_off();
		*piAudioMaxCount = i;
	}else{
		ret = -1;
	}

	return ret;
}

int tcc_db_get_audio_byTag(int uiServiceID, int uiCurrentChannelNumber, int iComponentTag, int *piAudioPID)
{
	char szSQL[1024] = { 0 };
	sqlite3_stmt *stmt = NULL;
	int rc;

	if (g_pISDBTDB != NULL)
	{
		sprintf(szSQL, "SELECT uiAudio_PID FROM audioPID WHERE usServiceID=%d AND uiCurrentChannelNumber=%d AND ucComponent_Tag=%d", uiServiceID, uiCurrentChannelNumber, iComponentTag);
		tcc_dxb_channel_db_lock_on();
		if(sqlite3_prepare_v2(g_pISDBTDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK)
		{
			rc = sqlite3_step(stmt);
			if (rc == SQLITE_ROW)
			{
				*piAudioPID = sqlite3_column_int(stmt, 0);
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
//			stmt = NULL;	//	I think 'stmt = NULL' is so meaningless at this location.
		}
		tcc_dxb_channel_db_lock_off();
	}
	return 0;
}

int tcc_db_get_preset(int iRowID, int *piChannelNumber, int *piPreset)
{
	int iChannelNumber=0, iPreset=0;
	char szSQL[1024] = { 0 };
	sqlite3_stmt *stmt = NULL;
	int rc = SQLITE_ERROR;
	int i;

	*piChannelNumber = 0;
	*piPreset = 0;

	sprintf(szSQL, "SELECT preset, channelNumber FROM channels WHERE _id=%d", iRowID);
	tcc_dxb_channel_db_lock_on();
	if(sqlite3_prepare_v2(g_pISDBTDB, szSQL, strlen(szSQL), &stmt,NULL) == SQLITE_OK)
	{
		rc = sqlite3_step(stmt);
		if (rc == SQLITE_ROW)
		{
			*piPreset = sqlite3_column_int(stmt, 0);
			*piChannelNumber = sqlite3_column_int(stmt, 1);
		}
	}
	if (stmt) {
		sqlite3_finalize(stmt);
//		stmt = NULL;	//	I think 'stmt = NULL' is so meaningless at this location.
	}
	tcc_dxb_channel_db_lock_off();

	if (rc != SQLITE_ROW)
	{
		return -1;
	}

	return 0;
}

int tcc_db_get_channel_byChannelNumber(int iChannelNumber, int *piCountryCode, int *piNetworkID)
{
	char szSQL[1024] = { 0 };
	sqlite3_stmt *stmt = NULL;
	int rc = SQLITE_ERROR;

	sprintf(szSQL, "SELECT countryCode, networkID FROM channels WHERE channelNumber=%d", iChannelNumber);
	tcc_dxb_channel_db_lock_on();
	if(sqlite3_prepare_v2(g_pISDBTDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK)
	{
		rc = sqlite3_step(stmt);
		if (rc == SQLITE_ROW)
		{
			*piCountryCode = sqlite3_column_int(stmt, 0);
			*piNetworkID = sqlite3_column_int(stmt, 1);
		}
	}
	if (stmt) {
		sqlite3_finalize(stmt);
//		stmt = NULL;	// I think 'stmt = NULL' is so meaningless at this location.
	}
	tcc_dxb_channel_db_lock_off();

	return 0;
}

int tcc_db_get_channel(int iRowID, \
						int *piChannelNumber, \
						int piAudioPID[], int piVideoPID[], \
						int *piStPID, int *piSiPID, \
						int *piServiceType, int *piServiceID, \
						int *piPMTPID, int *piCAECMPID, int *piACECMPID, int *piNetworkID, \
						int piAudioStreamType[], int piVideoStreamType[], \
						int piAudioComponentTag[], int piVideoComponentTag[], \
						int *piPCRPID, int *piTotalAudioCount, int *piTotalVideoCount, \
						int *piAudioIndex, int *piAudioRealIndex, int *piVideoIndex, int *piVideoRealIndex)
{
	int iPreset=-1;
	int iMaxVideoCount=0;
	int iMaxAudioCount=0;
	char szSQL[1024] = { 0 };
	sqlite3_stmt *stmt = NULL;
	int rc = SQLITE_ERROR;
	int i;

	iMaxVideoCount = *piTotalVideoCount;
	iMaxAudioCount = *piTotalAudioCount;

	sprintf(szSQL, "SELECT channelNumber, audioPID, videoPID, stPID, siPID, serviceType, serviceID, PMT_PID, CA_ECM_PID, AC_ECM_PID, networkID, preset, uiTotalAudio_PID, uiTotalVideo_PID, uiPCR_PID, ucAudio_StreamType, ucVideo_StreamType FROM channels WHERE _id=%d", iRowID);
	tcc_dxb_channel_db_lock_on();
	if(sqlite3_prepare_v2(g_pISDBTDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK)
	{
		rc = sqlite3_step(stmt);
		if (rc == SQLITE_ROW)
		{
			*piChannelNumber = sqlite3_column_int(stmt, 0);
			piAudioPID[0] = sqlite3_column_int(stmt, 1);
			piVideoPID[0] = sqlite3_column_int(stmt, 2);
			*piStPID = sqlite3_column_int(stmt, 3);
			*piSiPID = sqlite3_column_int(stmt, 4);
			*piServiceType = sqlite3_column_int(stmt, 5);
			*piServiceID = sqlite3_column_int(stmt, 6);
			*piPMTPID = sqlite3_column_int(stmt, 7);
			*piCAECMPID = sqlite3_column_int(stmt, 8);
			*piACECMPID = sqlite3_column_int(stmt, 9);
			*piNetworkID = sqlite3_column_int(stmt, 10);
			iPreset = sqlite3_column_int(stmt, 11);
			*piTotalAudioCount = sqlite3_column_int(stmt, 12);
			*piTotalVideoCount = sqlite3_column_int(stmt, 13);
			*piPCRPID = sqlite3_column_int(stmt, 14);
			piAudioStreamType[0] = sqlite3_column_int(stmt, 15);
			piVideoStreamType[0] = sqlite3_column_int(stmt, 16);
		}
	}
	if (stmt) {
		sqlite3_finalize(stmt);
//		stmt = NULL;	//	I think 'stmt = NULL' is so meaningless at this location.
	}
	tcc_dxb_channel_db_lock_off();

	if (rc != SQLITE_ROW || iPreset == 1){
		return -1;
	}

	if(*piTotalVideoCount > 1){
		*piTotalVideoCount = iMaxVideoCount;
		tcc_db_get_video(*piServiceID, *piChannelNumber, piTotalVideoCount, piVideoPID, piVideoStreamType, piVideoComponentTag);
	}

	*piVideoRealIndex = tcc_db_get_default_video(*piVideoIndex, *piTotalVideoCount, piVideoPID, piVideoStreamType, piVideoComponentTag, *piServiceType);
	if(*piVideoIndex < 0 || *piVideoIndex >= iMaxVideoCount) {
		*piVideoIndex = *piVideoRealIndex;
	}

	if(*piTotalVideoCount > 1){
		ALOGI("%s cnt = %d, videoIndex = %d, videoPID = %d, streamtype = %d\n", __func__, *piTotalVideoCount, *piVideoRealIndex, piVideoPID[*piVideoRealIndex], piVideoStreamType[*piVideoRealIndex]);
	}

	if(*piTotalAudioCount > 1){
		*piTotalAudioCount = iMaxAudioCount;
		tcc_db_get_audio(*piServiceID, *piChannelNumber, piTotalAudioCount, piAudioPID, piAudioStreamType, piAudioComponentTag);
	}

	*piAudioRealIndex = tcc_db_get_default_audio(*piAudioIndex, *piTotalAudioCount, piAudioPID, piAudioStreamType, piAudioComponentTag, *piServiceType);
	if(*piAudioIndex < 0 || *piAudioIndex >= iMaxAudioCount) {
		*piAudioIndex = *piAudioRealIndex;
	}

	if(*piTotalAudioCount > 1){
		ALOGI("%s cnt = %d, audioIndex = %d, audioPID = %d, streamtype = %d\n", __func__, *piTotalAudioCount, *piAudioRealIndex, piAudioPID[*piAudioRealIndex], piAudioStreamType[*piAudioRealIndex]);
	}

	return 0;
}

int tcc_db_get_channel_info(int iChannelNumber, int iServiceID, \
							int *piRemoconID, int *piThreeDigitNumber, int *piServiceType, \
							unsigned short *pusChannelName, int *piChannelNameLen, \
							int *piTotalAudioPID, int *piTotalVideoPID, int *piTotalSubtitleLang, \
							int *piAudioMode, int *piVideoMode, int *piAudioLang1, int *piAudioLang2, \
							int *piAudioComponentTag, int *piVideoComponentTag, int *piSubTitleComponentTag, \
							int *piStartMJD, int *piStartHH, int *piStartMM, int *piStartSS, int *piDurationHH, int *piDurationMM, int *piDurationSS, \
							unsigned short *pusEvtName, int *piEvtNameLen, \
							unsigned char *pucLogoStream, int *piLogoStreamSize, unsigned short *pusSimpleLogo, int *piSimpleLogoLength)
{
	char szSQL[1024] = { 0 };
	sqlite3_stmt *stmt = NULL;
	int iText16Length;
	unsigned short *pusText16;
	const unsigned char *pucBlob;
	int iLogoStreamSize, iNetworkID, iDWNL_ID, iLOGO_ID;
	int rc;
	int i;

	//channels
	*piRemoconID = -1;
	*piThreeDigitNumber = -1;
	*piServiceType = -1;
	*piChannelNameLen = -1;
	*piTotalAudioPID = -1;
	*piTotalVideoPID = -1;
	*piTotalSubtitleLang = -1;
	*piAudioComponentTag = -1;
	*piVideoComponentTag = -1;
	iDWNL_ID = -1;
	iLOGO_ID = -1;
	iNetworkID = -1;

	sprintf(szSQL, "SELECT remoconID, threedigitNumber, serviceType, channelName, uiTotalAudio_PID, uiTotalVideo_PID, DWNL_ID, LOGO_ID, strLogo_Char, usTotalCntSubtLang, networkID FROM channels WHERE serviceID=%d AND channelNumber=%d", iServiceID, iChannelNumber);
	tcc_dxb_channel_db_lock_on();
	if (g_pISDBTDB != NULL) {
		if(sqlite3_prepare_v2(g_pISDBTDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK)
		{
			rc = sqlite3_step(stmt);
			if (rc == SQLITE_ROW)
			{
				*piRemoconID = sqlite3_column_int(stmt, 0);
				*piThreeDigitNumber = sqlite3_column_int(stmt, 1);
				*piServiceType = sqlite3_column_int(stmt, 2);
				*piChannelNameLen = sqlite3_column_bytes16(stmt, 3);
				pusText16 = (unsigned short*)sqlite3_column_text16(stmt, 3);
				if (pusText16) {
					memcpy(pusChannelName, pusText16, *piChannelNameLen);
				}
				*piTotalAudioPID = sqlite3_column_int(stmt, 4);
				*piTotalVideoPID = sqlite3_column_int(stmt, 5);
				iDWNL_ID = sqlite3_column_int(stmt, 6);
				iLOGO_ID = sqlite3_column_int(stmt, 7);
				*piSimpleLogoLength = sqlite3_column_bytes16(stmt, 8);
				pusText16 = (unsigned short*)sqlite3_column_text16(stmt, 8);
				if (pusText16) {
					memcpy(pusSimpleLogo, pusText16, *piSimpleLogoLength);
				}
				*piTotalSubtitleLang = sqlite3_column_int(stmt, 9);
				iNetworkID = sqlite3_column_int(stmt, 10);
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
			stmt = NULL;
		}
	}
//	tcc_dxb_channel_db_lock_off();

	sprintf(szSQL, "SELECT ucComponent_Tag FROM audioPID WHERE usServiceID=%d AND uiCurrentChannelNumber=%d", iServiceID, iChannelNumber);
//	tcc_dxb_channel_db_lock_on();
	if (g_pISDBTDB != NULL) {
		if(sqlite3_prepare_v2(g_pISDBTDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK)
		{
			rc = sqlite3_step(stmt);
			if (rc == SQLITE_ROW)
			{
				*piAudioComponentTag = sqlite3_column_int(stmt, 0);
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
			stmt = NULL;
		}
	}
//	tcc_dxb_channel_db_lock_off();

	sprintf(szSQL, "SELECT ucComponent_Tag FROM VideoPID WHERE usServiceID=%d AND uiCurrentChannelNumber=%d", iServiceID, iChannelNumber);
//	tcc_dxb_channel_db_lock_on();
	if (g_pISDBTDB != NULL) {
		if(sqlite3_prepare_v2(g_pISDBTDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK)
		{
			rc = sqlite3_step(stmt);
			if (rc == SQLITE_ROW)
			{
				*piVideoComponentTag = sqlite3_column_int(stmt, 0);
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
			stmt = NULL;
		}
	}
//	tcc_dxb_channel_db_lock_off();

	sprintf(szSQL, "SELECT ucComponent_Tag FROM subtitlePID WHERE usServiceID=%d AND uiCurrentChannelNumber=%d", iServiceID, iChannelNumber);
//	tcc_dxb_channel_db_lock_on();
	if (g_pISDBTDB != NULL) {
		if(sqlite3_prepare_v2(g_pISDBTDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK)
		{
			rc = sqlite3_step(stmt);
			if (rc == SQLITE_ROW)
			{
				*piSubTitleComponentTag = sqlite3_column_int(stmt, 0);
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
			stmt = NULL;
		}
	}
//	tcc_dxb_channel_db_lock_off();

	//logo
	iLogoStreamSize = *piLogoStreamSize;
	*piLogoStreamSize = 0;
	if (iNetworkID != -1 && iDWNL_ID != -1 && iLOGO_ID != -1)
	{
		sprintf(szSQL, "SELECT Image FROM logo WHERE NetID=%d AND DWNL_ID=%d AND LOGO_ID=%d ORDER BY LOGO_TYPE DESC", iNetworkID, iDWNL_ID, iLOGO_ID);
//		tcc_dxb_channel_db_lock_on();
		if(g_pISDBTLogoDB != NULL) {
			if (sqlite3_prepare_v2 (g_pISDBTLogoDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK) {
				sqlite3_reset(stmt);
				rc = sqlite3_step(stmt);
				if (rc == SQLITE_ROW)
				{
					*piLogoStreamSize = sqlite3_column_bytes(stmt, 0);
					pucBlob = sqlite3_column_blob(stmt, 0);
					if (pucBlob && iLogoStreamSize > *piLogoStreamSize) {
						memcpy(pucLogoStream, pucBlob, *piLogoStreamSize);
					}
					else {
						*piLogoStreamSize = -1;
					}
				}
			}
			if (stmt) {
				sqlite3_finalize(stmt);
//				stmt = NULL;	//	I think 'stmt = NULL' is so meaningless at this location.
			}
		}
//		tcc_dxb_channel_db_lock_off();
	}

	tcc_dxb_channel_db_lock_off();

	//epg p/f
	*piAudioMode = -1;
	*piVideoMode = -1;
	*piAudioLang1 = -1;
	*piAudioLang2 = -1;
	*piStartMJD = -1;
	*piStartHH = -1;
	*piStartMM = -1;
	*piStartSS = -1;
	*piDurationHH = -1;
	*piDurationMM = -1;
	*piDurationSS = -1;
	*piEvtNameLen = -1;

	tcc_dxb_epg_db_lock_on();

	TCCISDBT_EPGDB_GetChannelInfo(EPG_PF, iChannelNumber, iServiceID, piAudioMode, piVideoMode, piAudioLang1, piAudioLang2, \
									piStartMJD, piStartHH, piStartMM, piStartSS, piDurationHH, piDurationMM, piDurationSS, pusEvtName, piEvtNameLen);

	tcc_dxb_epg_db_lock_off();

	return 0;
}

int tcc_db_get_channel_info2(int iChannelNumber, int iServiceID, int *piGenre)
{
	tcc_dxb_epg_db_lock_on();

	TCCISDBT_EPGDB_GetChannelInfo2(EPG_PF, iChannelNumber, iServiceID, piGenre);

	tcc_dxb_epg_db_lock_off();

	return 0;
}


int tcc_db_get_handover_info(int iChannelNumber, int iServiceID, void *pDescTDSD, void *pDescEBD)
{
	char szSQL[1024] = { 0 };
	sqlite3_stmt *stmt = NULL;
	const T_DESC_TDSD *pstDescTDSD;
	const T_DESC_EBD *pstDescEBD;
	int rc=SQLITE_ERROR;

	if (pDescTDSD == NULL || pDescEBD == NULL) {
		return 0;
	}

	if (g_pISDBTDB != NULL)
	{
		sprintf(szSQL, "SELECT frequency, affiliationID FROM channels WHERE serviceID=%d AND channelNumber=%d", iServiceID, iChannelNumber);
		tcc_dxb_channel_db_lock_on();
		if (sqlite3_prepare_v2 (g_pISDBTDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK) {
			sqlite3_reset(stmt);
			rc = sqlite3_step(stmt);
			if (rc == SQLITE_ROW) {
				pstDescTDSD = sqlite3_column_blob(stmt, 0);
				if (pstDescTDSD) {
					memcpy(pDescTDSD, pstDescTDSD, sizeof(T_DESC_TDSD));
				}
				pstDescEBD = sqlite3_column_blob(stmt, 1);
				if (pstDescEBD) {
					memcpy(pDescEBD, pstDescEBD, sizeof(T_DESC_EBD));
				}
			}
		}
		sqlite3_finalize(stmt);
		tcc_dxb_channel_db_lock_off();
	}

	return 0;
}

int tcc_db_get_representative_channel(int iChannelNumber, int iServiceType, \
								int *piRowID, int *piTotalAudioCount, int piAudioPID[], int *piTotalVideoCount, int piVideoPID[], \
								int *piStPID, int *piSiPID, int *piServiceType, int *piServiceID, int *piPMTPID, int *piCAECMPID, int *piACECMPID, int *piNetworkID, \
								int piAudioStreamType[], int piAudioComponentTag[], int piVideoStreamType[], int piVideoComponentTag[], int *piPCRPID, int *piThreeDigitNumber, \
								int *piAudioIndex, int *piAudioRealIndex, int *piVideoIndex, int *piVideoRealIndex)
{
	int iMaxVideoCount=0;
	int iMaxAudioCount=0;
	int iThreeDigitNumber=0;
	char szSQL[1024] = { 0 };
	sqlite3_stmt *stmt = NULL;
	int rc=SQLITE_ERROR;
	int i;

	if (g_pISDBTDB != NULL)
	{
		iMaxVideoCount = *piTotalVideoCount;
		iMaxAudioCount = *piTotalAudioCount;

		if(iServiceType == SERVICE_TYPE_FSEG) {
			sprintf(szSQL, "SELECT _id, audioPID, videoPID, stPID, siPID, serviceType, serviceID, PMT_PID, CA_ECM_PID, AC_ECM_PID, networkID, uiTotalAudio_PID, uiTotalVideo_PID, uiPCR_PID, ucAudio_StreamType, ucVideo_StreamType, threedigitNumber \
FROM channels WHERE channelNumber=%d AND (serviceType=1 OR serviceType=161) AND (audioPID!=65535 OR videoPID!=65535) ORDER BY threedigitNumber", iChannelNumber);
		}
		else {
			sprintf(szSQL, "SELECT _id, audioPID, videoPID, stPID, siPID, serviceType, serviceID, PMT_PID, CA_ECM_PID, AC_ECM_PID, networkID, uiTotalAudio_PID, uiTotalVideo_PID, uiPCR_PID, ucAudio_StreamType, ucVideo_StreamType, threedigitNumber \
FROM channels WHERE channelNumber=%d AND serviceType=192 AND (audioPID!=65535 OR videoPID!=65535) AND (PMT_PID>8135 AND PMT_PID<8145) ORDER BY threedigitNumber", iChannelNumber);
		}

		tcc_dxb_channel_db_lock_on();
		if(sqlite3_prepare_v2(g_pISDBTDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK) {
			rc = sqlite3_step(stmt);
			if (rc == SQLITE_ROW)
			{
				iThreeDigitNumber = sqlite3_column_int(stmt, 16);
				if((iServiceType == SERVICE_TYPE_FSEG)
					|| ((iServiceType == SERVICE_TYPE_1SEG) && (*piThreeDigitNumber == -1))
					|| ((iServiceType == SERVICE_TYPE_1SEG) && (*piThreeDigitNumber%10) == (iThreeDigitNumber%10))) {
					*piRowID = sqlite3_column_int(stmt, 0);
					piAudioPID[0] = sqlite3_column_int(stmt, 1);
					piVideoPID[0] = sqlite3_column_int(stmt, 2);
					*piStPID = sqlite3_column_int(stmt, 3);
					*piSiPID = sqlite3_column_int(stmt, 4);
					*piServiceType = sqlite3_column_int(stmt, 5);
					*piServiceID = sqlite3_column_int(stmt, 6);
					*piPMTPID = sqlite3_column_int(stmt, 7);
					*piCAECMPID = sqlite3_column_int(stmt, 8);
					*piACECMPID = sqlite3_column_int(stmt, 9);
					*piNetworkID = sqlite3_column_int(stmt, 10);
					*piTotalAudioCount = sqlite3_column_int(stmt, 11);
					*piTotalVideoCount = sqlite3_column_int(stmt, 12);
					*piPCRPID = sqlite3_column_int(stmt, 13);
					*piAudioStreamType = sqlite3_column_int(stmt, 14);
					*piVideoStreamType = sqlite3_column_int(stmt, 15);
					*piThreeDigitNumber = sqlite3_column_int(stmt, 16);
				}
			}
			else if (rc == SQLITE_DONE) {
				ALOGI("[%d] representative channel is not exist(%d, %d)\n", __LINE__, iChannelNumber, iServiceType);
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
//			stmt = NULL;	//	I think 'stmt = NULL' is so meaningless at this location.
		}

		tcc_dxb_channel_db_lock_off();

		if (rc != SQLITE_ROW || *piRowID == 0)
		{
			return -1;
		}

		if(*piTotalVideoCount > 1){
			*piTotalVideoCount = iMaxVideoCount;
			tcc_db_get_video(*piServiceID, iChannelNumber, piTotalVideoCount, piVideoPID, piVideoStreamType, piVideoComponentTag);
		}

		*piVideoRealIndex = tcc_db_get_default_video(*piVideoIndex, *piTotalVideoCount, piVideoPID, piVideoStreamType, piVideoComponentTag, iServiceType);
		if(*piVideoIndex < 0 || *piVideoIndex >= iMaxVideoCount) {
			*piVideoIndex = *piVideoRealIndex;
		}

		if(*piTotalVideoCount > 1){
			ALOGI("%s cnt = %d, videoIndex = %d, videoPID = %d, streamtype = %d\n", __func__, *piTotalVideoCount, *piVideoRealIndex, piVideoPID[*piVideoRealIndex], piVideoStreamType[*piVideoRealIndex]);
		}

		if(*piTotalAudioCount > 1){
			*piTotalAudioCount = iMaxAudioCount;
			tcc_db_get_audio(*piServiceID, iChannelNumber, piTotalAudioCount, piAudioPID, piAudioStreamType, piAudioComponentTag);
		}

		*piAudioRealIndex = tcc_db_get_default_audio(*piAudioIndex, *piTotalAudioCount, piAudioPID, piAudioStreamType, piAudioComponentTag, iServiceType);
		if(*piAudioIndex < 0 || *piAudioIndex >= iMaxAudioCount) {
			*piAudioIndex = *piAudioRealIndex;
		}

		if(*piTotalAudioCount > 1){
			ALOGI("%s cnt = %d, audioIndex = %d, audioPID = %d, streamtype = %d\n", __func__, *piTotalAudioCount, *piAudioRealIndex, piAudioPID[*piAudioRealIndex], piAudioStreamType[*piAudioRealIndex]);
		}
	}

	return 0;
}

/* Noah / 20170522 / Added for IM478A-31 (Primary Service)
	This function is based on 'tcc_db_get_representative_channel'.
*/
int tcc_db_get_primaryService(int iChannelNumber, int iServiceType, \
								int *piRowID, int *piTotalAudioCount, int piAudioPID[], int *piTotalVideoCount, int piVideoPID[], \
								int *piStPID, int *piSiPID, int *piServiceType, int *piServiceID, int *piPMTPID, int *piCAECMPID, int *piACECMPID, int *piNetworkID, \
								int piAudioStreamType[], int piAudioComponentTag[], int piVideoStreamType[], int piVideoComponentTag[], int *piPCRPID, int *piThreeDigitNumber, \
								int *piAudioIndex, int *piAudioRealIndex, int *piVideoIndex, int *piVideoRealIndex)
{
	int iMaxVideoCount=0;
	int iMaxAudioCount=0;
	int iThreeDigitNumber=0;
	char szSQL[1024] = { 0 };
	sqlite3_stmt *stmt = NULL;
	int rc=SQLITE_ERROR;
	int i;

	if (g_pISDBTDB != NULL)
	{
		iMaxVideoCount = *piTotalVideoCount;
		iMaxAudioCount = *piTotalAudioCount;

		if(iServiceType == SERVICE_TYPE_FSEG)
		{
			sprintf(szSQL, "SELECT _id, audioPID, videoPID, stPID, siPID, serviceType, serviceID, PMT_PID, CA_ECM_PID, AC_ECM_PID, networkID, uiTotalAudio_PID, uiTotalVideo_PID, uiPCR_PID, ucAudio_StreamType, ucVideo_StreamType, threedigitNumber, primaryServiceFlag \
FROM channels WHERE channelNumber=%d AND (serviceType=1 OR serviceType=161) AND (audioPID!=65535 OR videoPID!=65535) AND primaryServiceFlag=1", iChannelNumber);
		}
		else
		{
			sprintf(szSQL, "SELECT _id, audioPID, videoPID, stPID, siPID, serviceType, serviceID, PMT_PID, CA_ECM_PID, AC_ECM_PID, networkID, uiTotalAudio_PID, uiTotalVideo_PID, uiPCR_PID, ucAudio_StreamType, ucVideo_StreamType, threedigitNumber, primaryServiceFlag \
FROM channels WHERE channelNumber=%d AND serviceType=192 AND (audioPID!=65535 OR videoPID!=65535) AND (PMT_PID>8135 AND PMT_PID<8145) AND primaryServiceFlag=2", iChannelNumber);
		}

		tcc_dxb_channel_db_lock_on();

		if(sqlite3_prepare_v2(g_pISDBTDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK)
		{
			rc = sqlite3_step(stmt);
			if (rc == SQLITE_ROW)
			{
				*piRowID = sqlite3_column_int(stmt, 0);
				piAudioPID[0] = sqlite3_column_int(stmt, 1);
				piVideoPID[0] = sqlite3_column_int(stmt, 2);
				*piStPID = sqlite3_column_int(stmt, 3);
				*piSiPID = sqlite3_column_int(stmt, 4);
				*piServiceType = sqlite3_column_int(stmt, 5);
				*piServiceID = sqlite3_column_int(stmt, 6);
				*piPMTPID = sqlite3_column_int(stmt, 7);
				*piCAECMPID = sqlite3_column_int(stmt, 8);
				*piACECMPID = sqlite3_column_int(stmt, 9);
				*piNetworkID = sqlite3_column_int(stmt, 10);
				*piTotalAudioCount = sqlite3_column_int(stmt, 11);
				*piTotalVideoCount = sqlite3_column_int(stmt, 12);
				*piPCRPID = sqlite3_column_int(stmt, 13);
				*piAudioStreamType = sqlite3_column_int(stmt, 14);
				*piVideoStreamType = sqlite3_column_int(stmt, 15);
				*piThreeDigitNumber = sqlite3_column_int(stmt, 16);
			}
			else if (rc == SQLITE_DONE)
			{
				ALOGI("[%d] primaryService channel is not exist(%d, %d)\n", __LINE__, iChannelNumber, iServiceType);
			}
		}

		if (stmt)
		{
			sqlite3_finalize(stmt);
//			stmt = NULL;	// I think 'stmt = NULL' is so meaningless at this location.
		}

		tcc_dxb_channel_db_lock_off();

		if (rc != SQLITE_ROW || *piRowID == 0)
		{
			return -1;
		}

		if(*piTotalVideoCount > 1)
		{
			*piTotalVideoCount = iMaxVideoCount;
			tcc_db_get_video(*piServiceID, iChannelNumber, piTotalVideoCount, piVideoPID, piVideoStreamType, piVideoComponentTag);
		}

		*piVideoRealIndex = tcc_db_get_default_video(*piVideoIndex, *piTotalVideoCount, piVideoPID, piVideoStreamType, piVideoComponentTag, iServiceType);

		if(*piVideoIndex < 0 || *piVideoIndex >= iMaxVideoCount)
		{
			*piVideoIndex = *piVideoRealIndex;
		}

		if(*piTotalVideoCount > 1)
		{
			ALOGI("%s cnt = %d, videoIndex = %d, videoPID = %d, streamtype = %d\n", __func__, *piTotalVideoCount, *piVideoRealIndex, piVideoPID[*piVideoRealIndex], piVideoStreamType[*piVideoRealIndex]);
		}

		if(*piTotalAudioCount > 1)
		{
			*piTotalAudioCount = iMaxAudioCount;
			tcc_db_get_audio(*piServiceID, iChannelNumber, piTotalAudioCount, piAudioPID, piAudioStreamType, piAudioComponentTag);
		}

		*piAudioRealIndex = tcc_db_get_default_audio(*piAudioIndex, *piTotalAudioCount, piAudioPID, piAudioStreamType, piAudioComponentTag, iServiceType);

		if(*piAudioIndex < 0 || *piAudioIndex >= iMaxAudioCount)
		{
			*piAudioIndex = *piAudioRealIndex;
		}

		if(*piTotalAudioCount > 1)
		{
			ALOGI("%s cnt = %d, audioIndex = %d, audioPID = %d, streamtype = %d\n", __func__, *piTotalAudioCount, *piAudioRealIndex, piAudioPID[*piAudioRealIndex], piAudioStreamType[*piAudioRealIndex]);
		}
	}

	return 0;
}

int tcc_db_get_channel_data_ForChannel(ST_CHANNEL_DATA *p, unsigned int uiChannelNo, unsigned int uiChannelServiceID)
{
// This function is called in TCCDxBProc_Update when "[PMT-Change] Others".
// The efficiency of the following 2 kind routine the same.

	int err = SQLITE_ERROR;
	int find_channel = 0;
	sqlite3_stmt *stmt = NULL;
	int ch_num;
	int ch_count = 0;
	int i;
	char szSQL[1024] = { 0 };

	if (g_pISDBTDB != NULL)
	{
		sprintf(szSQL, "SELECT	\
channelNumber, countryCode, audioPID, videoPID, stPID, siPID, PMT_PID, remoconID,	\
serviceType, serviceID, regionID, threedigitNumber, TStreamID, berAVG, networkID, channelName	\
FROM channels WHERE serviceID=%d AND channelNumber=%d AND (serviceType=1 OR serviceType=192)",
uiChannelServiceID, uiChannelNo);

		tcc_dxb_channel_db_lock_on();

		if (sqlite3_prepare_v2(g_pISDBTDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK)
		{
			err = sqlite3_step(stmt);
			if (err == SQLITE_ROW)
			{
				p->uiCurrentChannelNumber	= sqlite3_column_int(stmt, 0);
				p->uiCurrentCountryCode		= sqlite3_column_int(stmt, 1);
				p->uiAudioPID				= sqlite3_column_int(stmt, 2);
				p->uiVideoPID				= sqlite3_column_int(stmt, 3);
				p->uiStPID					= sqlite3_column_int(stmt, 4);
				p->uiSiPID					= sqlite3_column_int(stmt, 5);
				p->uiPMTPID					= sqlite3_column_int(stmt, 6);
				p->uiRemoconID				= sqlite3_column_int(stmt, 7);
				p->uiServiceType			= sqlite3_column_int(stmt, 8);
				p->uiServiceID				= sqlite3_column_int(stmt, 9);
				p->uiRegionID				= sqlite3_column_int(stmt, 10);
				p->uiThreeDigitNumber		= sqlite3_column_int(stmt, 11);
				p->uiTStreamID				= sqlite3_column_int(stmt, 12);
				p->uiBerAvg					= sqlite3_column_int(stmt, 13);
				p->uiNetworkID				= sqlite3_column_int(stmt, 14);
				p->pusChannelName			= (unsigned short *)sqlite3_column_text(stmt, 15);

				find_channel = 1;

				err = sqlite3_step(stmt);
			}

			if ((err != SQLITE_ROW) && (err != SQLITE_DONE))
			{
				ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);
			}
		}

		if (stmt)
		{
			sqlite3_finalize(stmt);
//			stmt = NULL;
		}

		tcc_dxb_channel_db_lock_off();
	}
	
	return (int)((find_channel == 1) ? 0 : -1);
}

int tcc_db_get_channel_current_date_precision(int *mjd, int *year, int *month, int *day, int *hour, int *minute, int *second, int *msec)
{
	long long llCurrDXBTime;

	long long llCurrSYSTime;
	long long llSYSTimeDiff;
	DATE_TIME_T	current_date;

	struct timespec systimespec;

	if( gstCurrentDateTime.uiMJD )
	{
		clock_gettime(CLOCK_MONOTONIC , &systimespec);
		llCurrSYSTime = (long long)(systimespec.tv_sec)*1000 + (long long)systimespec.tv_nsec/1000000;
		llSYSTimeDiff = llCurrSYSTime - g_llSYSTimeOfTOTArrival; //msec

		llCurrDXBTime = (long long)gstCurrentDateTime.uiMJD*(24*60*60)
						+ (long long)( gstCurrentDateTime.stTime.ucHour*(60*60)
						+ gstCurrentDateTime.stTime.ucMinute*60 + gstCurrentDateTime.stTime.ucSecond);
		llCurrDXBTime = llCurrDXBTime*1000 + llSYSTimeDiff;

	 	*msec		= (int)(llCurrDXBTime%1000);
		{
			int iRemain;
			long long llMJDsec = llCurrDXBTime/1000;
			*mjd = llMJDsec/86400;
			iRemain = (int)(llMJDsec%86400);
			*hour = iRemain/3600;
			iRemain %= 3600;
			*minute = iRemain/60;
			iRemain %= 60;
			*second = iRemain;
		}

		ISDBT_TIME_GetRealDate(&current_date, *mjd);
		*year		= (int) current_date.year;
		*month		= (int) current_date.month;
		*day		= (int) current_date.day;
		return 0;
	}
	else
	{
		return -1;
	}
}

int tcc_db_get_channel_rowid (unsigned int uiChannelNumber, unsigned int  uiCountryCode, unsigned int uiNetworkID, unsigned int uiServiceID, unsigned int uiServiceType)
{
	char	szSQL[1024] = {0};
	int	err = SQLITE_ERROR, rowID = -1;
	sqlite3_stmt *stmt = NULL;

	if (g_pISDBTDB != NULL) {
		tcc_dxb_channel_db_lock_on();
		sprintf(szSQL, "SELECT _id FROM channels WHERE serviceID=%d AND channelNumber=%d AND networkID=%d",
				uiServiceID, uiChannelNumber, uiNetworkID);
		if (sqlite3_prepare_v2(g_pISDBTDB, szSQL, -1, &stmt, NULL) == SQLITE_OK) {
			err = sqlite3_step(stmt);
			if (err == SQLITE_ROW) {
				rowID = sqlite3_column_int(stmt, 0);
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
//			stmt = NULL;	// I think 'stmt = NULL' is so meaningless at this location.
		}
		tcc_dxb_channel_db_lock_off();
	}
	return rowID;
}

/* Noah / 20171026 / Added tcc_db_get_channel_rowid_new function for IM478A-52 (database less tuning)
	Parameter
		uiChannelNumber
			Input parameter
			Index of frequency table. Ex) 0 : 473.143MHz, 1 : 479.143MHz ...
		uiTranportStreamID
			Input parameter
			transport_stream_id
		uiServiceID
			Input parameter
			service_id

	Return Value
		rowID
			_id of ChannelDB
*/
int tcc_db_get_channel_rowid_new(unsigned int uiChannelNumber, unsigned int uiServiceID)
{
	char szSQL[1024] = {0};
	int	err = SQLITE_ERROR;
	int rowID = -1;
	sqlite3_stmt *stmt = NULL;

	if (g_pISDBTDB != NULL)
	{
		tcc_dxb_channel_db_lock_on();

		sprintf(szSQL, "SELECT _id FROM channels WHERE serviceID=%d AND channelNumber=%d",	uiServiceID, uiChannelNumber);

		if (sqlite3_prepare_v2(g_pISDBTDB, szSQL, -1, &stmt, NULL) == SQLITE_OK)
		{
			err = sqlite3_step(stmt);
			if (err == SQLITE_ROW)
			{
				rowID = sqlite3_column_int(stmt, 0);
			}
		}

		if (stmt)
		{
			sqlite3_finalize(stmt);
//			stmt = NULL;	// I think 'stmt = NULL' is so meaningless at this location.
		}

		tcc_dxb_channel_db_lock_off();
	}

	return rowID;
}

int tcc_db_get_service_info(int iChannelNumber, int iCountryCode, int *piServiceID, int *piServiceType)
{
	char	szSQL[1024] = {0};
	int	err = SQLITE_ERROR;
	int service_count = 0;
	sqlite3_stmt *stmt = NULL;

	if (g_pISDBTDB != NULL) {
		tcc_dxb_channel_db_lock_on();
		sprintf(szSQL, "SELECT serviceType, serviceID FROM channels WHERE channelNumber=%d ", iChannelNumber);
		if (sqlite3_prepare_v2(g_pISDBTDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK)
		{
			err = sqlite3_step(stmt);
			while (err == SQLITE_ROW)
			{
				piServiceType[service_count] = sqlite3_column_int(stmt, 0);
				piServiceID[service_count] = sqlite3_column_int(stmt, 1);
				service_count++;
				if (service_count >= MAX_SUPPORT_CURRENT_SERVICE)
					break;

				err = sqlite3_step(stmt);
			}
			if ((err != SQLITE_ROW) && (err != SQLITE_DONE)){
				ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
//			stmt = NULL;	// I think 'stmt = NULL' is so meaningless at this location.
		}
		tcc_dxb_channel_db_lock_off();
	}
	return service_count;
}

int tcc_db_get_service_num(int iChannelNumber, int iCountryCode, int NetworkID)
{
// Noah, after setChannel is called, this function executes once. tcc_manager_demux_detect_service_num

	char	szSQL[1024] = {0};
	int	err = SQLITE_ERROR;
	int service_count = 0;
	int service_type;
	sqlite3_stmt *stmt = NULL;

	if (g_pISDBTDB != NULL)
	{
		tcc_dxb_channel_db_lock_on();

		sprintf(szSQL, "SELECT serviceType FROM channels WHERE channelNumber=%d AND countryCode=%d AND networkID=%d AND serviceID<=63935",	// 63935 == 0xF9BF
			iChannelNumber, iCountryCode, NetworkID);

		if (sqlite3_prepare_v2(g_pISDBTDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK)
		{
			err = sqlite3_step(stmt);
			while (err == SQLITE_ROW)
			{
				service_type = sqlite3_column_int(stmt, 0);

				if (TCC_Isdbt_IsSupportSpecialService())
				{
					if (service_type != SERVICE_TYPE_TEMP_VIDEO)
					{
						service_count++;
					}
				}
				else
				{
					service_count++;
				}

				if (service_count >= MAX_SUPPORT_CURRENT_SERVICE)
				{
					break;
				}
				
				err = sqlite3_step(stmt);
			}

			if ((err != SQLITE_ROW) && (err != SQLITE_DONE))
			{
				ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);
			}
		}

		if (stmt)
		{
			sqlite3_finalize(stmt);
//			stmt = NULL;
		}

		tcc_dxb_channel_db_lock_off();
	}

	return service_count;
}

int tcc_db_get_channel_tstreamid (unsigned int uiChannelNumber)
{
	char	szSQL[1024] = {0};
	int	err = SQLITE_ERROR, TStreamID = -1;
	sqlite3_stmt *stmt = NULL;

	if (g_pISDBTDB != NULL) {
		tcc_dxb_channel_db_lock_on();
		sprintf(szSQL, "SELECT TStreamID FROM channels WHERE channelNumber=%d", uiChannelNumber);
		if (sqlite3_prepare_v2(g_pISDBTDB, szSQL, -1, &stmt, NULL) == SQLITE_OK) {
			err = sqlite3_step(stmt);
			if (err == SQLITE_ROW) {
				TStreamID = sqlite3_column_int(stmt, 0);
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
//			stmt = NULL;	// I think 'stmt = NULL' is so meaningless at this location.
		}
		tcc_dxb_channel_db_lock_off();
	}
	return TStreamID;
}

int tcc_db_get_channel_primary (int uiServiceID)
{
	char	szSQL[1024] = {0};
	int	err = SQLITE_ERROR, primaryVal = -1;
	sqlite3_stmt *stmt = NULL;

	if (g_pISDBTDB != NULL) {
		tcc_dxb_channel_db_lock_on();
		sprintf(szSQL, "SELECT primaryServiceFlag FROM channels WHERE serviceID=%d", uiServiceID);
		if (sqlite3_prepare_v2(g_pISDBTDB, szSQL, -1, &stmt, NULL) == SQLITE_OK) {
			err = sqlite3_step(stmt);
			if (err == SQLITE_ROW) {
				primaryVal = sqlite3_column_int(stmt, 0);
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
//			stmt = NULL;	// I think 'stmt = NULL' is so meaningless at this location.
		}
		tcc_dxb_channel_db_lock_off();
	}
	return primaryVal;
}


void tcc_db_delete_channel(int iChannelNumber, int iNetworkID)
{
	char szSQL[1024] = { 0 };
	int err = SQLITE_ERROR;

	if (g_pISDBTDB != NULL)
	{
		tcc_dxb_channel_db_lock_on();
		sprintf(szSQL, "DELETE FROM channels WHERE channelNumber=%d AND networkId=%d", iChannelNumber, iNetworkID);
		err = TCC_SQLITE3_EXEC(g_pISDBTDB, szSQL, NULL, NULL, NULL);
		if (err != SQLITE_OK) ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);

		sprintf(szSQL, "DELETE FROM videoPID WHERE uiCurrentChannelNumber=%d AND usNetworkId=%d", iChannelNumber, iNetworkID);
		err = TCC_SQLITE3_EXEC(g_pISDBTDB, szSQL, NULL, NULL, NULL);
		if (err != SQLITE_OK) ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);

		sprintf(szSQL, "DELETE FROM audioPID WHERE uiCurrentChannelNumber=%d AND usNetworkId=%d", iChannelNumber, iNetworkID);
		err = TCC_SQLITE3_EXEC(g_pISDBTDB, szSQL, NULL, NULL, NULL);
		if (err != SQLITE_OK) ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);

		sprintf(szSQL, "DELETE FROM subtitlePID WHERE uiCurrentChannelNumber=%d AND usNetworkId=%d", iChannelNumber, iNetworkID);
		err = TCC_SQLITE3_EXEC(g_pISDBTDB, szSQL, NULL, NULL, NULL);
		if (err != SQLITE_OK) ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);
		tcc_dxb_channel_db_lock_off();
	}

	return;
}

void tcc_db_create_epg(int iChannelNumber, int iNetworkID)
{
	char szSQL[1024] = { 0 };
	sqlite3_stmt *stmt =  NULL;
	int rc=SQLITE_ERROR;
	int iServiceID = 0;
	int iServiceType = 0;

	tcc_dxb_epg_db_lock_on();
	tcc_dxb_channel_db_lock_on();

	memset(&gstCurrentDateTime, 0, sizeof(DATE_TIME_STRUCT));
	memset(&gstCurrentLocalTimeOffset, 0, sizeof(LOCAL_TIME_OFFSET_STRUCT));

	TCCISDBT_EPGDB_DeleteAll();

	g_nHowMany_FsegServices = 0;    // for IM478A-21_MultiCommonEvent
	memset(g_nServiceID_FsegServices, 0, sizeof(unsigned int) * 16);    // for IM478A-21_MultiCommonEvent

	sprintf(szSQL, "SELECT serviceID, serviceType FROM channels WHERE channelNumber=%d AND networkID=%d", iChannelNumber, iNetworkID);
	if (sqlite3_prepare_v2(g_pISDBTDB, szSQL, -1, &stmt, NULL) == SQLITE_OK)
	{
		rc = sqlite3_step(stmt);
		while(rc == SQLITE_ROW)
		{
			iServiceID = sqlite3_column_int(stmt, 0);
			iServiceType = sqlite3_column_int(stmt, 1);
			if ((iServiceType == SERVICE_TYPE_FSEG) || (iServiceType == SERVICE_TYPE_TEMP_VIDEO) || (iServiceType == SERVICE_TYPE_1SEG))
			{
				CreateDB_EPG_Table(EPG_PF, iChannelNumber, iNetworkID, iServiceID, iServiceType);
				if ((iServiceType == SERVICE_TYPE_FSEG) || (iServiceType == SERVICE_TYPE_TEMP_VIDEO))
				{
					CreateDB_EPG_Table(EPG_SCHEDULE, iChannelNumber, iNetworkID, iServiceID, iServiceType);

					if (iServiceType == SERVICE_TYPE_FSEG)    // for IM478A-21_MultiCommonEvent
					{
						g_nServiceID_FsegServices[ g_nHowMany_FsegServices ] = iServiceID;
						g_nHowMany_FsegServices++;

						//printf("Noah XXX / [%s] g_nHowMany_FsegServices(%d), g_nServiceID_FsegServices[%d](0x%x)\n",
						//	__func__, g_nHowMany_FsegServices, g_nHowMany_FsegServices - 1, g_nServiceID_FsegServices[g_nHowMany_FsegServices - 1] );

						if (16 <= g_nHowMany_FsegServices)
						{
							g_nHowMany_FsegServices = 15;
							ALOGE("[%s] Please check this log !!!!!\n", __func__);
						}
					}
				}
			}	
			rc = sqlite3_step(stmt);
		}
	}

	if (stmt)
	{
		sqlite3_finalize(stmt);
//		stmt = NULL;	// I think 'stmt = NULL' is so meaningless at this location.
	}

	tcc_dxb_channel_db_lock_off();
	tcc_dxb_epg_db_lock_off();

	return;
}

void tcc_db_delete_epg(int iCheckMJD)
{
	char szSQL[1024] = { 0 };
	sqlite3_stmt *stmt = NULL;
	int iTableCount = 0;
	int iID = 0;
	int iMJD = 0;
	int iTableNameLen = 0;
	char szTableName[256] = { 0 };
	const char *pText = NULL;
	int err = SQLITE_ERROR;
	int i;

	if (iCheckMJD) {
		if (gstCurrentDateTime.uiMJD < 32) {
			ALOGE("[%s] Error MJD is %d", __func__, gstCurrentDateTime.uiMJD);
			return;
		}
		iMJD = gstCurrentDateTime.uiMJD - 32;
	}

	tcc_dxb_epg_db_lock_on();

	if (iCheckMJD) {
		sprintf(szSQL, "SELECT COUNT(_id) FROM EPG_Table WHERE MJD<%d", iMJD);
	}
	else {
		sprintf(szSQL, "SELECT COUNT(_id) FROM EPG_Table");
	}
	if (sqlite3_prepare_v2(g_pISDBTEPGDB, szSQL, -1, &stmt, NULL) == SQLITE_OK) {
		err = sqlite3_step(stmt);
		if (err == SQLITE_ROW) {
			iTableCount = sqlite3_column_int(stmt, 0);
		}
	}
	if (stmt) {
		sqlite3_finalize(stmt);
		stmt = NULL;
	}

	for(i=0; i<iTableCount; i++)
	{
		if (iCheckMJD) {
			sprintf(szSQL, "SELECT _id, iLen_TableName, TableName FROM EPG_Table WHERE MJD<%d", iMJD);
		}
		else {
			sprintf(szSQL, "SELECT _id, iLen_TableName, TableName FROM EPG_Table");
		}
		if (sqlite3_prepare_v2(g_pISDBTEPGDB, szSQL, -1, &stmt, NULL) == SQLITE_OK) {
			err = sqlite3_step(stmt);
			if (err == SQLITE_ROW) {
				iID = sqlite3_column_int(stmt, 0);
				iTableNameLen = sqlite3_column_int(stmt, 1);
				pText = (char *)sqlite3_column_text(stmt, 2);
				if (pText) {
					memcpy(szTableName, pText, iTableNameLen);
					szTableName[iTableNameLen] = 0;
				}
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
//			stmt = NULL;	// I think 'stmt = NULL' is so meaningless at this location.
		}

		if (err == SQLITE_ROW) {
			DestroyDB_EPG_Table(szTableName);

			sprintf(szSQL, "DELETE FROM EPG_Table WHERE _id=%d", iID);
			err = TCC_SQLITE3_EXEC(g_pISDBTEPGDB, szSQL, NULL, NULL, NULL);
			if (err != SQLITE_OK) ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);
		}
	}

	tcc_dxb_epg_db_lock_off();

	return;
}

#if 0    // Noah, 20180611, NOT support now
void tcc_db_commit_epg(int iDayOffset, int iChannel, int iNetworkID)
{
	char szSQL[1024] = { 0 };
	sqlite3_stmt *stmt = NULL;
	int iTableCount = 0;
	int iTableNameLen = 0;
	char szTableName[256] = { 0 };
	const char *pText = NULL;
	int err = SQLITE_ERROR;
	int i, j;

	tcc_dxb_epg_db_lock_on();

	if (iDayOffset >= 0) {
		sprintf(szSQL, "SELECT COUNT(_id) FROM EPG_Table WHERE channelNumber=%d AND networkID=%d", iChannel, iNetworkID);
		if (sqlite3_prepare_v2(g_pISDBTEPGDB, szSQL, -1, &stmt, NULL) == SQLITE_OK) {
			err = sqlite3_step(stmt);
			if (err == SQLITE_ROW) {
				iTableCount = sqlite3_column_int(stmt, 0);
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
			stmt = NULL;
		}

		for(i=0; i<iTableCount; i++)
		{
			sprintf(szSQL, "SELECT iLen_TableName, TableName FROM EPG_Table WHERE channelNumber=%d AND networkID=%d", iChannel, iNetworkID);
			if (sqlite3_prepare_v2(g_pISDBTEPGDB, szSQL, -1, &stmt, NULL) == SQLITE_OK) {
				for(j=0; j<=i; j++)
				{
					err = sqlite3_step(stmt);
				}

				if (err == SQLITE_ROW) {
					iTableNameLen = sqlite3_column_int(stmt, 0);
					pText = (char *)sqlite3_column_text(stmt, 1);
					if (pText) {
						memcpy(szTableName, pText, iTableNameLen);
						szTableName[iTableNameLen] = 0;
					}
				}
			}
			if (stmt) {
				sqlite3_finalize(stmt);
//				stmt = NULL;	// I think 'stmt = NULL' is so meaningless at this location.
			}

			if (err == SQLITE_ROW) {
				sprintf(szSQL, "DELETE FROM %s", szTableName);
				err = TCC_SQLITE3_EXEC(g_pISDBTEPGDB, szSQL, NULL, NULL, NULL);
				if (err != SQLITE_OK) ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);
			}
		}
	}

	EPGDB_BeginTransaction();
	TCCISDBT_EPGDB_Commit(g_pISDBTEPGDB, iDayOffset, iChannel, gCurrentNITAreaCode, &gstCurrentDateTime, &gstCurrentLocalTimeOffset);
	EPGDB_EndTransaction();

	sprintf(szSQL, "UPDATE EPG_Table SET MJD=%d WHERE channelNumber=%d AND networkID=%d", gstCurrentDateTime.uiMJD, iChannel, iNetworkID);
	err = TCC_SQLITE3_EXEC(g_pISDBTEPGDB, szSQL, NULL, NULL, NULL);
	if (err != SQLITE_OK) ALOGE("[%s:%d] ERROR : %d\n", __func__, __LINE__, err);

	tcc_dxb_epg_db_lock_off();

	return;
}
#endif    // Noah, 20180611, NOT support now

void tcc_db_delete_logo(int iChannelNumber, int iServiceID)
{
	char szSQL[1024] = {0, };
	sqlite3_stmt *stmt = NULL;
	int iNetworkID, iDWNL_ID, iLOGO_ID;
	int rc;
	int err;

	iNetworkID = -1;
	iDWNL_ID = -1;
	iLOGO_ID = -1;

	sprintf(szSQL, "SELECT networkID FROM channels WHERE serviceID=%d AND channelNumber=%d", iServiceID, iChannelNumber);
	tcc_dxb_channel_db_lock_on();
	if (g_pISDBTDB != NULL) {
		if(sqlite3_prepare_v2(g_pISDBTDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK)
		{
			rc = sqlite3_step(stmt);
			if (rc == SQLITE_ROW)
			{
				iNetworkID = sqlite3_column_int(stmt, 0);
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
			stmt = NULL;
		}
	}

	sprintf(szSQL, "SELECT DWNL_ID, LOGO_ID FROM channels WHERE serviceID=%d AND channelNumber=%d", iServiceID, iChannelNumber);
	if (g_pISDBTDB != NULL) {
		if(sqlite3_prepare_v2(g_pISDBTDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK)
		{
			rc = sqlite3_step(stmt);
			if (rc == SQLITE_ROW)
			{
				iDWNL_ID = sqlite3_column_int(stmt, 0);
				iLOGO_ID = sqlite3_column_int(stmt, 1);
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
//			stmt = NULL;	// I think 'stmt = NULL' is so meaningless at this location.
		}
	}

	if(iNetworkID != -1 && iDWNL_ID != -1 && iLOGO_ID != -1) {
		sprintf(szSQL, "DELETE FROM logo WHERE NetID=%d AND DWNL_ID=%d AND LOGO_ID=%d", iNetworkID, iDWNL_ID, iLOGO_ID);
		err = TCC_SQLITE3_EXEC(g_pISDBTLogoDB, szSQL, NULL, NULL, NULL);
		if (err != SQLITE_OK) ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);
	}
	tcc_dxb_channel_db_lock_off();

}


unsigned int tcc_uni_strcpy(unsigned short *pDst, unsigned short *pSrc)
{
	unsigned int i=0;

	if(pSrc!=NULL && pDst!=NULL){
		while(*pSrc != 0){
			*(pDst+i) = *(pSrc+i);
			i++;
			if (i>= 255) {
				*(pDst+i) = 0;
				break;
			}
		}
	}

	return i;
}

void tcc_db_get_channel_for_pvr(void *pInfo, unsigned int ch_num, unsigned int service_id)
{
	int rc, i;
	char szSQL[1024] = { 0 };
	sqlite3_stmt *stmt = NULL;
	CHANNEL_HEADER_INFORMATION *p = (CHANNEL_HEADER_INFORMATION*)pInfo;

	memset(&p->dbCh, 0x00, sizeof(DB_CHANNEL_ELEMENT));

	if (g_pISDBTDB != NULL){
		sprintf(szSQL, "SELECT * FROM channels WHERE serviceID=%d AND channelNumber=%d", service_id, ch_num);
		//ALOGE("%s\n", szSQL);

		tcc_dxb_channel_db_lock_on();
		if(sqlite3_prepare_v2(g_pISDBTDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK) {
			rc = sqlite3_step(stmt);
			if (rc == SQLITE_ROW){
				int index = 1;

				p->dbCh.channelNumber = sqlite3_column_int(stmt, index++);
				p->dbCh.countryCode = sqlite3_column_int(stmt, index++);
				p->dbCh.audioPID = sqlite3_column_int(stmt, index++);
				p->dbCh.videoPID = sqlite3_column_int(stmt, index++);
				p->dbCh.stPID = sqlite3_column_int(stmt, index++);
				p->dbCh.siPID = sqlite3_column_int(stmt, index++);
				p->dbCh.PMT_PID = sqlite3_column_int(stmt, index++);
				p->dbCh.remoconID = sqlite3_column_int(stmt, index++);
				p->dbCh.serviceType = sqlite3_column_int(stmt, index++);
				p->dbCh.serviceID = sqlite3_column_int(stmt, index++);
				p->dbCh.regionID = sqlite3_column_int(stmt, index++);
				p->dbCh.threedigitNumber = sqlite3_column_int(stmt, index++);
				p->dbCh.TStreamID = sqlite3_column_int(stmt, index++);
				p->dbCh.berAVG = sqlite3_column_int(stmt, index++);

			#if 0
				ALOGE("[%s] pmtPID:%d, audioPID:%d, videoPID:%d, stPID:%d, siPID:%d\n", __func__,
					p->dbCh.PMT_PID, p->dbCh.audioPID, p->dbCh.videoPID, p->dbCh.stPID, p->dbCh.siPID);
			#endif

			#if 0
				ALOGE("[%s] threedigitNumber:%d, TStreamID:%d, berAVG:%d\n", __func__,
					p->dbCh.threedigitNumber, p->dbCh.TStreamID, p->dbCh.berAVG);
			#endif /* 0 */

				tcc_uni_strcpy(p->dbCh.channelName, (unsigned short *)sqlite3_column_text(stmt, index++));

				p->dbCh.preset = sqlite3_column_int(stmt, index++);
				p->dbCh.networkID = sqlite3_column_int(stmt, index++);
				p->dbCh.areaCode = sqlite3_column_int(stmt, index++);

			#if 0
				ALOGE("[%s] preset:%d, networkID:%d, areaCode:%d\n", __func__,
					p->dbCh.preset, p->dbCh.networkID, p->dbCh.areaCode);
			#endif /* 0 */

			#if 0
				T_DESC_EBD stDescEBD;
				T_DESC_TDSD stDescTDSD;
			#endif /* 0 */
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
//			stmt = NULL;	// I think 'stmt = NULL' is so meaningless at this location.
		}
		tcc_dxb_channel_db_lock_off();
	}
}

void tcc_db_get_service_for_pvr(void *pInfo, unsigned int ch_num, unsigned int service_id)
{
	int rc, i;
	char szSQL[1024] = { 0 };
	sqlite3_stmt *stmt = NULL;
	CHANNEL_HEADER_INFORMATION *p = (CHANNEL_HEADER_INFORMATION*)pInfo;

	memset(&p->dbService, 0x00, sizeof(DB_SERVICE_ELEMENT));

	if (g_pISDBTDB != NULL){
		tcc_dxb_channel_db_lock_on();
		sprintf(szSQL, "SELECT uiPCR_PID, ucAudio_StreamType, ucVideo_StreamType FROM channels WHERE serviceID=%d AND channelNumber=%d", service_id, ch_num);
		if (sqlite3_prepare_v2(g_pISDBTDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK) {
			rc = sqlite3_step(stmt);
			if (rc == SQLITE_ROW) {
				int index = 0;
				p->dbService.uiPCRPID = sqlite3_column_int(stmt, index++);
				p->dbService.audio_stream_type = sqlite3_column_int(stmt, index++);
				p->dbService.video_stream_type = sqlite3_column_int(stmt, index++);
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
//			stmt = NULL;	// I think 'stmt = NULL' is so meaningless at this location.
		}
		tcc_dxb_channel_db_lock_off();
	}
}

void tcc_db_get_audio_for_pvr(void *pInfo, unsigned int ch_num, unsigned int service_id)
{
	int rc, i;
	char szSQL[1024] = { 0 };
	sqlite3_stmt *stmt = NULL;
	CHANNEL_HEADER_INFORMATION *p = (CHANNEL_HEADER_INFORMATION*)pInfo;

	memset(&p->dbAudio[0], 0x00, sizeof(DB_AUDIO_ELEMENT)*MAX_AUDIOTRACK_SUPPORT);

	if (g_pISDBTDB != NULL){
		sprintf(szSQL, "SELECT * FROM audioPID WHERE usServiceID=%d AND uiCurrentChannelNumber=%d", service_id, ch_num);
		//ALOGE("%s\n", szSQL);

		tcc_dxb_channel_db_lock_on();
		if(sqlite3_prepare_v2(g_pISDBTDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK) {
			for(i=0;i<MAX_AUDIOTRACK_SUPPORT;i++)
			{
				rc = sqlite3_step(stmt);
				if (rc == SQLITE_DONE){
					break;
				}

				if (rc == SQLITE_ROW)
				{
					int index = 1;

					p->dbAudio[i].uiServiceID = sqlite3_column_int(stmt, index++);
					p->dbAudio[i].uiCurrentChannelNumber = sqlite3_column_int(stmt, index++);
					p->dbAudio[i].uiCurrentCountryCode = sqlite3_column_int(stmt, index++);
					p->dbAudio[i].usNetworkID = sqlite3_column_int(stmt, index++);
					p->dbAudio[i].uiAudio_PID = sqlite3_column_int(stmt, index++);
					p->dbAudio[i].ucAudio_IsScrambling = sqlite3_column_int(stmt, index++);
					p->dbAudio[i].ucAudio_StreamType = sqlite3_column_int(stmt, index++);
					p->dbAudio[i].ucAudioType = sqlite3_column_int(stmt, index++);
					strcpy((char *)p->dbAudio[i].acLangCode, (const char *)sqlite3_column_text(stmt, index++));

				#if 0
					ALOGE("[%s] i:%d, serviceId:%d, ch:%d, cc:%d, net:%d, a_pid:%d, scram:%d, type:%d, %s\n", __func__,
						i,
						p->dbAudio[i].uiServiceID,
						p->dbAudio[i].uiCurrentChannelNumber,
						p->dbAudio[i].uiCurrentCountryCode,
						p->dbAudio[i].usNetworkID,
						p->dbAudio[i].uiAudio_PID,
						p->dbAudio[i].ucAudio_IsScrambling,
						p->dbAudio[i].ucAudio_StreamType,
						p->dbAudio[i].ucAudioType,
						p->dbAudio[i].acLangCode);
				#endif /* 0 */
				}
			}
		}
		if(stmt) {
			sqlite3_finalize(stmt);
//			stmt = NULL;	// I think 'stmt = NULL' is so meaningless at this location.
		}

		tcc_dxb_channel_db_lock_off();
	}
}

int tcc_db_get_one_service_num(int iChannelNumber)
{
// In case of 'Primary Service' ON, this function is called in seamless.
// This function is called once after setChannel.

	char szSQL[1024] = {0};
	int	err = SQLITE_ERROR;
	int service_count = 0;
	int service_type;
	int threedigitNum;
	sqlite3_stmt *stmt = NULL;

	if (g_pISDBTDB != NULL)
	{
		tcc_dxb_channel_db_lock_on();

		sprintf(szSQL, "SELECT COUNT(*) FROM channels WHERE channelNumber=%d AND serviceType=192 AND threedigitNumber NOT BETWEEN 200 AND 599", iChannelNumber);
		if (sqlite3_prepare_v2(g_pISDBTDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK)
		{
			err = sqlite3_step(stmt);
			if (err == SQLITE_ROW)
			{
				service_count = sqlite3_column_int(stmt, 0);

				if (service_count >= MAX_SUPPORT_CURRENT_SERVICE)
				{
					service_count = MAX_SUPPORT_CURRENT_SERVICE;
				}
			}
			
			if ((err != SQLITE_ROW) && (err != SQLITE_DONE))
			{
				ALOGE("[%s:%d] err:%d\n", __func__, __LINE__, err);
			}
		}

		if (stmt)
		{
			sqlite3_finalize(stmt);
//			stmt = NULL;
		}

		tcc_dxb_channel_db_lock_off();
	}

	return service_count;
}


int UpdateDB_UserData_RegionID(unsigned int uiRegionID) {
	int err = SQLITE_OK;
	char szSQL[1024] = { 0 };
	sqlite3_stmt *stmt = NULL;

	sprintf(szSQL, "UPDATE userDatas SET region_id=%d WHERE _id = 1 ", uiRegionID);

	tcc_dxb_channel_db_lock_on();
	if (g_pISDBTDB != NULL) {
		err |= sqlite3_prepare_v2(g_pISDBTDB, szSQL, -1, &stmt, NULL);
		err |= sqlite3_reset(stmt);
		err |= sqlite3_step(stmt);
		err |= sqlite3_finalize(stmt);
	}
	tcc_dxb_channel_db_lock_off();
	if(err == SQLITE_OK || err == SQLITE_DONE)
		return SQLITE_OK;
	else
		return err;
}

int UpdateDB_UserData_PrefecturalCode(unsigned int uiPrefecturalBitOrder) {
	int err = SQLITE_OK;
	char szSQL[1024] = { 0 };
	int local_index;
	unsigned long long ullPrefecturalCode = 0b1;
	unsigned int uiAreaCode;
	sqlite3_stmt *stmt = NULL;

	ullPrefecturalCode = ullPrefecturalCode << (56 - uiPrefecturalBitOrder);

	sprintf(szSQL, "UPDATE userDatas SET prefectural_Code=%llu WHERE _id = 1 ", ullPrefecturalCode);

	tcc_dxb_channel_db_lock_on();
	if (g_pISDBTDB != NULL) {
		err |= sqlite3_prepare_v2(g_pISDBTDB, szSQL, -1, &stmt, NULL);
		err |= sqlite3_reset(stmt);
		err |= sqlite3_step(stmt);
		err |= sqlite3_finalize(stmt);
	}
	tcc_dxb_channel_db_lock_off();
	if(err == SQLITE_OK || err == SQLITE_DONE)
		return SQLITE_OK;
	else
		return err;
}

int UpdateDB_UserData_AreaCode(unsigned int uiAreaCode) {
	int err = SQLITE_OK;
	char szSQL[1024] = { 0 };
	sqlite3_stmt *stmt = NULL;

	sprintf(szSQL, "UPDATE userDatas SET area_Code=%d WHERE _id = 1 ", uiAreaCode);

	tcc_dxb_channel_db_lock_on();
	if (g_pISDBTDB != NULL) {
		err |= sqlite3_prepare_v2(g_pISDBTDB, szSQL, -1, &stmt, NULL);
		err |= sqlite3_reset(stmt);
		err |= sqlite3_step(stmt);
		err |= sqlite3_finalize(stmt);
	}
	tcc_dxb_channel_db_lock_off();
	if(err == SQLITE_OK || err == SQLITE_DONE)
		return SQLITE_OK;
	else
		return err;
}

int UpdateDB_UserData_PostalCode(char *pcPostalCode) {
	int err = SQLITE_OK;
	char szSQL[1024] = { 0 };
	sqlite3_stmt *stmt = NULL;

	sprintf(szSQL, "UPDATE userDatas SET postal_Code = ? WHERE _id = 1 ");

	tcc_dxb_channel_db_lock_on();
	if (g_pISDBTDB != NULL) {
		err |= sqlite3_prepare_v2(g_pISDBTDB, szSQL, -1, &stmt, NULL);
		err |= sqlite3_reset(stmt);
		err |= sqlite3_bind_text(stmt, 1, pcPostalCode, -1, SQLITE_STATIC);
		err |= sqlite3_step(stmt);
		err |= sqlite3_finalize(stmt);
	}
	tcc_dxb_channel_db_lock_off();
	if(err == SQLITE_OK || err == SQLITE_DONE)
		return SQLITE_OK;
	else
		return err;
}

void tcc_db_get_userDatas(unsigned int *puiRegionID, unsigned long long *pullPrefecturalCode, unsigned int *puiAreaCode, char *pcPostalCode) {
	int rc;
	char szSQL[1024] = { 0 };
	int iPostalCodeLen;
	char *pcBuff = NULL;
	sqlite3_stmt *stmt = NULL;

	if (g_pISDBTDB != NULL){
		sprintf(szSQL, "SELECT region_ID, prefectural_Code, area_Code, postal_Code FROM userDatas WHERE _id = 1 ");
		tcc_dxb_channel_db_lock_on();
		if(sqlite3_prepare_v2(g_pISDBTDB, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK) {
			rc = sqlite3_step(stmt);
			if (rc == SQLITE_ROW) {
				*puiRegionID = sqlite3_column_int(stmt, 0);
				*pullPrefecturalCode = sqlite3_column_int64(stmt, 1);
				*puiAreaCode= sqlite3_column_int(stmt, 2);
				iPostalCodeLen = sqlite3_column_bytes(stmt, 3);
				pcBuff = (char *)sqlite3_column_text(stmt, 3);
				if(pcBuff) {
					memcpy(pcPostalCode, pcBuff, iPostalCodeLen);
					pcPostalCode[iPostalCodeLen] = '\0';
				}
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
//			stmt = NULL;	// I think 'stmt = NULL' is so meaningless at this location.
		}
		tcc_dxb_channel_db_lock_off();
	}
}

int UpdateDB_ArrangeDataService(void)
{
	int err = SQLITE_OK;

	tcc_dxb_channel_db_lock_on();
	if (g_pISDBTDB != NULL) {
		err = TCCISDBT_ChannelDB_ArrangeDataService();
	}
	tcc_dxb_channel_db_lock_off();
	return err;
}

/*--------- custom search for JK : start --------------*/
int UpdateDB_CustomSearch_DelInfo (int preset, int network_id)
{
	int err = SQLITE_ERROR;

	if(preset==-1 && network_id==-1) return -1;

	err = TCCISDBT_ChannelDB_CustomSearch_DelInfo(g_pISDBTDB, preset, network_id);
	return err;
}
int UpdateDB_CustomSearch_AddReport_SignalStrength(int signal_strength, void *ptr)
{
	CUSTOMSEARCH_FOUND_INFO_T *pFoundInfo = (CUSTOMSEARCH_FOUND_INFO_T*)ptr;;
	if (pFoundInfo == NULL) return -1;
	pFoundInfo->strength = signal_strength;
	return 0;
}
/*
  * option
  *	1 = make a report indicating channels which is already existing
  *   2 = make a report indicating channels which is inserted by custom search
  */
int UpdateDB_CustomSearch_MakeReport (int option, int channel, int fullseg_id, int oneseg_id, int tsid, void *ptr)
{
	CUSTOMSEARCH_FOUND_INFO_T *pFoundInfo = (CUSTOMSEARCH_FOUND_INFO_T*)ptr;;
	int count =0;

	if (pFoundInfo == NULL) return -1;

	memset (pFoundInfo, 0, sizeof(CUSTOMSEARCH_FOUND_INFO_T));
	if (option == 1) {
		pFoundInfo->status = 1;
		pFoundInfo->ch = channel+13;	/* makeing UHF channel no */
		pFoundInfo->tsid = tsid;
		if (fullseg_id) {
			pFoundInfo->fullseg_id = fullseg_id;
			pFoundInfo->all_id[count] = fullseg_id;
			count++;
		}
		if (oneseg_id) {
			pFoundInfo->oneseg_id = oneseg_id;
			pFoundInfo->all_id[count] = oneseg_id;
			count++;
		}
		pFoundInfo->total_svc = count;
	} else if (option==2) {
	}
	return 0;
}
int UpdateDB_CustomSearch_UpdateInfo (int search_kind, int channel, int tsid, void *ptr)
{
	int preset;
	CUSTOMSEARCH_FOUND_INFO_T *pFoundInfo = (CUSTOMSEARCH_FOUND_INFO_T*)ptr;

	if (pFoundInfo == NULL) return SQLITE_ERROR;

	if (search_kind == ISDBT_SCANOPTION_CUSTOM_RELAY) preset = RECORD_PRESET_CUSTOM_RELAY;
	else preset = RECORD_PRESET_CUSTOM_AFFILIATION;

	TCCISDBT_ChannelDB_CustomSearch_UpdateInfo (g_pISDBTDB, preset, pFoundInfo);
	pFoundInfo->ch = channel;
	pFoundInfo->tsid = tsid;
	if (pFoundInfo->total_svc > 0) pFoundInfo->status = 1;
	else pFoundInfo->status = -1;
	return SQLITE_OK;
}

/*
  * return	1=success, tsid matched, 0=tsid of found service is not matched
  */
int UpdateDB_CustomSearch_CheckNetworkID(int network_id, int *tsid)
{
	int cnt, *ptsid, rtn=0;
	ptsid = tsid;
	if (ptsid == NULL) return 0;
	for (cnt=0; cnt < 128 && *ptsid != -1; cnt++) {
		if (network_id == *ptsid) {
			rtn = 1;
			break;
		}
		ptsid++;
	}
	return rtn;
}
int UpdateDB_CustomSearch(int channel, int country_code, int option, int *tsid, void *ptr)
{
	SERVICE_FOUND_INFO_T stServiceInfo;
	int err = -2;
	int iNetworkID;
	int fullseg_id, oneseg_id;

	if(ptr == NULL) return -1;

	tcc_dxb_channel_db_lock_on();
	if (g_pISDBTDB != NULL) {
		/* 1 - check if the tsid is same or not */
		iNetworkID = (int)TCCISDBT_ChannelDB_GetNetworkID (channel, country_code, -1);
		if (iNetworkID == 0) {
			/* netowrk_id of found channel is not valid */
			UpdateDB_CustomSearch_MakeReport (1, channel, 0, 0, iNetworkID, ptr);
			tcc_dxb_channel_db_lock_off();
			return -2;
		}

		if (!UpdateDB_CustomSearch_CheckNetworkID(iNetworkID, tsid)) {
			/* netowrk_id of found channel is different with tsid */
			UpdateDB_CustomSearch_MakeReport (1, channel, 0, 0, iNetworkID, ptr);
			tcc_dxb_channel_db_lock_off();
			return -2;
		}

		/* 2 - delete channel infromatin from DB if necessary */
		if (option & ISDBT_SCANOPTION_CUSTOM_AFFILIATION) {
			/* delete records that a value of preset is 2(autosearch) or 4 (affiliation search) */
			UpdateDB_CustomSearch_DelInfo (RECORD_PRESET_AUTOSEARCH, -1);
			UpdateDB_CustomSearch_DelInfo (RECORD_PRESET_CUSTOM_AFFILIATION, -1);

			err = TCCISDBT_ChannelDB_CustomSearch_SearchSameService(g_pISDBTDB, channel, RECORD_PRESET_INITSEARCH, iNetworkID, &fullseg_id, &oneseg_id, 0);
			if (err == SQLITE_OK) {
				ALOGD("[UpdateDB_CustomSearch][A] service found in db(%d)", RECORD_PRESET_INITSEARCH);
				UpdateDB_CustomSearch_MakeReport (1, channel, fullseg_id, oneseg_id, iNetworkID, ptr);
				tcc_dxb_channel_db_lock_off();
				return 0;
			}
			err = TCCISDBT_ChannelDB_CustomSearch_SearchSameService (g_pISDBTDB, channel, RECORD_PRESET_CUSTOM_RELAY, iNetworkID, &fullseg_id, &oneseg_id, 0);
			if (err == SQLITE_OK) {
				ALOGD("[UpdateDB_CustomSearch][A] service found in db(%d)", RECORD_PRESET_CUSTOM_RELAY);
				UpdateDB_CustomSearch_MakeReport (1, channel, fullseg_id, oneseg_id, iNetworkID, ptr);
				tcc_dxb_channel_db_lock_off();
				return 0;
			}
			/* insert found service */
			ALOGD("[UpdateDB_CustomSearch][A] UpdateInfo in db(%d)", ISDBT_SCANOPTION_CUSTOM_AFFILIATION);
			err = UpdateDB_CustomSearch_UpdateInfo((int)(ISDBT_SCANOPTION_CUSTOM_AFFILIATION), channel, iNetworkID, ptr);
			if (err != SQLITE_OK) ALOGE("UpdateDB_CustomSearch_UpdateInfo fail!");
			tcc_dxb_channel_db_lock_off();
			return 0;
		} else {
			/* if not affiliation search, be treated as relay search */
			/* delete records that a vlaue of preset is 3 and tsid is same */
			UpdateDB_CustomSearch_DelInfo (RECORD_PRESET_CUSTOM_RELAY, iNetworkID);
			/* check service of same channel and tsid in preset=0 (initscan or rescan) */
			err = TCCISDBT_ChannelDB_CustomSearch_SearchSameService(g_pISDBTDB, channel, RECORD_PRESET_INITSEARCH, iNetworkID, &fullseg_id, &oneseg_id, 0);
			if (err == SQLITE_OK) {
				ALOGD("[UpdateDB_CustomSearch][R] service found in db(%d)", RECORD_PRESET_INITSEARCH);
				UpdateDB_CustomSearch_MakeReport (1, channel, fullseg_id, oneseg_id, iNetworkID, ptr);
				tcc_dxb_channel_db_lock_off();
				return 0;
			}
			/* check service of same tsid in preset=2 (autosearch) */
			err = TCCISDBT_ChannelDB_CustomSearch_SearchSameService (g_pISDBTDB, -1, RECORD_PRESET_AUTOSEARCH, iNetworkID, &fullseg_id, &oneseg_id, 1);
			if (err == SQLITE_OK) {
				ALOGD("[UpdateDB_CustomSearch][R] service found in db(%d)", RECORD_PRESET_AUTOSEARCH);
				TCCISDBT_ChannelDB_CustomSearch_GetChannelInfoReplace (&stServiceInfo);
				TCCISDBT_ChannelDB_CustomSearch_ChannelInfoReplace(g_pISDBTDB, &stServiceInfo, RECORD_PRESET_AUTOSEARCH, iNetworkID);

				if (stServiceInfo.pServiceFound)
					tcc_mw_free(__FUNCTION__, __LINE__, stServiceInfo.pServiceFound);

				UpdateDB_CustomSearch_MakeReport (1, channel, fullseg_id, oneseg_id, iNetworkID, ptr);
				tcc_dxb_channel_db_lock_off();
				return 0;
			}
			/* check service of same tsid in preset=4 (affiliation search) */
			err = TCCISDBT_ChannelDB_CustomSearch_SearchSameService (g_pISDBTDB, -1, RECORD_PRESET_CUSTOM_AFFILIATION, iNetworkID, &fullseg_id, &oneseg_id, 1);
			if (err == SQLITE_OK) {
				ALOGD("[UpdateDB_CustomSearch][R] service found in db(%d)", RECORD_PRESET_CUSTOM_AFFILIATION);
				TCCISDBT_ChannelDB_CustomSearch_GetChannelInfoReplace (&stServiceInfo);
				TCCISDBT_ChannelDB_CustomSearch_ChannelInfoReplace(g_pISDBTDB, &stServiceInfo, RECORD_PRESET_CUSTOM_AFFILIATION, iNetworkID);

				if (stServiceInfo.pServiceFound)
					tcc_mw_free(__FUNCTION__, __LINE__, stServiceInfo.pServiceFound);

				UpdateDB_CustomSearch_MakeReport (1, channel, fullseg_id, oneseg_id, iNetworkID, ptr);
				tcc_dxb_channel_db_lock_off();
				return 0;
			}
			/* insert found service */
			ALOGD("[UpdateDB_CustomSearch][R] UpdateInfo in db(%d)", ISDBT_SCANOPTION_CUSTOM_RELAY);
			err = UpdateDB_CustomSearch_UpdateInfo((int)(ISDBT_SCANOPTION_CUSTOM_RELAY), channel, iNetworkID, ptr);
			if (err != SQLITE_OK) ALOGE("UpdateDB_CustomSearch_UpdateInfo fail!");
			tcc_dxb_channel_db_lock_off();
			return 0;
		}
	}
	tcc_dxb_channel_db_lock_off();
	return err;
}

int UpdateDB_Del_CustomSearch (int preset)
{
	int err = SQLITE_ERROR;
	if (preset < 0) return err;

	tcc_dxb_channel_db_lock_on();
	if (g_pISDBTDB != NULL) {
		UpdateDB_CustomSearch_DelInfo (preset, -1);
	}
	tcc_dxb_channel_db_lock_off();
	return err;
}
/*--------- custom search for JK : end --------------*/
int UpdateDB_ArrangeSpecialService (void)
{
	int err = SQLITE_ERROR;
	tcc_dxb_channel_db_lock_on();
	if (g_pISDBTDB != NULL) err = TCCISDBT_ChannelDB_ArrangeSpecialService();
	tcc_dxb_channel_db_lock_off();
	return err;
}
int UpdateDB_SpecialService_DelInfo (void)
{
	int err = SQLITE_ERROR;
	tcc_dxb_channel_db_lock_on();
	if (g_pISDBTDB != NULL) err = TCCISDBT_ChannelDB_DelSpecialServiceInfo(g_pISDBTDB);
	tcc_dxb_channel_db_lock_off();
	return err;
}
int UpdateDB_SpecialService_UpdateInfo (int channel_number, int country_code, unsigned short service_id, int *row_id)
{
	int err = SQLITE_ERROR;
	tcc_dxb_channel_db_lock_on();
	if (g_pISDBTDB != NULL) err = TCCISDBT_ChannelDB_UpdateSQLFfile_SpecialService(g_pISDBTDB, channel_number, country_code, service_id, row_id);
	tcc_dxb_channel_db_lock_off();
	return err;
}

int UpdateDB_CheckNetworkID (int network_id)
{
	int err = SQLITE_ERROR;
	tcc_dxb_channel_db_lock_on();
	if (g_pISDBTDB != NULL) err = TCCISDBT_ChannelDB_CheckNetworkID(network_id);
	tcc_dxb_channel_db_lock_off();
	return err;
}
int UpdateDB_GetInfoLogoDB (unsigned int channel, unsigned int country_code, unsigned int network_id, unsigned int service_id, unsigned int *dwnl_id, unsigned int *logo_id, unsigned short *sSimpleLogo, unsigned int *simplelogo_len)
{
	int err = SQLITE_ERROR;
	tcc_dxb_channel_db_lock_on();
	if (g_pISDBTDB != NULL)
		err = TCCISDBT_ChannelDB_GetInfoLogoDB(g_pISDBTDB, channel, country_code, network_id, service_id, dwnl_id, logo_id, sSimpleLogo, simplelogo_len);
	tcc_dxb_channel_db_lock_off();
	return err;
}

int UpdateDB_UpdateInfoLogoDB (unsigned int channel, unsigned int country_code, unsigned int network_id, unsigned int dwnl_id, unsigned int logo_id, unsigned short *sSimpleLogo, unsigned int simplelogo_len)
{
	int err = SQLITE_ERROR;
	tcc_dxb_channel_db_lock_on();
	if (g_pISDBTDB != NULL)
		err = TCCISDBT_ChannelDB_UpdateInfoLogoDB(g_pISDBTDB, channel, country_code, network_id, dwnl_id, logo_id, sSimpleLogo, simplelogo_len);
	tcc_dxb_channel_db_lock_off();
	return err;
}
int UpdateDB_GetInfoChannelNameDB (unsigned int channel, unsigned int country_code, unsigned network_id, unsigned service_id, unsigned short *sServiceName, unsigned int *service_name_len)
{
	int err = SQLITE_ERROR;
	tcc_dxb_channel_db_lock_on();
	if (g_pISDBTDB != NULL)
		err = TCCISDBT_ChannelDB_GetInfoChannelNameDB(g_pISDBTDB, channel, country_code, network_id, service_id, sServiceName, service_name_len);
	tcc_dxb_channel_db_lock_off();
	return err;
}
int UpdateDB_UpdateChannelNameDB (unsigned int channel, unsigned int country_code, unsigned int network_id)
{
	int err = SQLITE_ERROR;
	tcc_dxb_channel_db_lock_on();
	if (g_pISDBTDB != NULL)
		err = TCCISDBT_ChannelDB_UpdateChannelNameDB (g_pISDBTDB, channel, country_code, network_id);
	tcc_dxb_channel_db_lock_off();
	return err;
}

// Noah / 20170522 / Added for IM478A-31 (Primary Service)
BOOL UpdateDB_PrimaryServiceFlag_ChannelTable(int uiServiceID, int uiCurrentChannelNumber, int uiCurrentCountryCode, int uiPrimaryServiceFlagValue)
{
	int err = SQLITE_ERROR;

	tcc_dxb_channel_db_lock_on();

	if (g_pISDBTDB != NULL)
	{
		err = TCCISDBT_ChannelDB_UpdatePrimaryServiceFlag(uiCurrentChannelNumber, uiCurrentCountryCode, uiServiceID, uiPrimaryServiceFlagValue);
	}

	tcc_dxb_channel_db_lock_off();

	return (err == SQLITE_OK);
}

/* Noah / 20180131 / IM478A-77 (relay station search in the background using 2TS)
	Description
		This fucntion is based on 'InsertDB_ChannelTable' and is for 'setRelayStationChDataIntoDB' API.
		If J&K App finds "Relay Station Channel" by using 2TS,
			1. J&K App calls 'setRelayStationChDataIntoDB' API to save some data of NIT to DB
			2. 'setRelayStationChDataIntoDB' API calls the following 2 functions.
				2.1. InsertDB_ChannelTable_2
					In order to save the data to Queue (g_pChannelMsg) first
				2.2. UpdateDB_CustomSearch_2
					2.2.1. In order to fit J&K's specification (refer to Requirement_Specification_for_RelayStaion_Search_20140620_V1.06.pdf)
					2.2.2. In order to save the data to real DB.
			3. After 'setRelayStationChDataIntoDB' API is done, J&K App calls 'setChannel' to play.

	Parameter
		All of them : Input

	Return
		0 (SQLITE_OK) : success
		else          : error
*/
int InsertDB_ChannelTable_2(int channelNumber,
										int countryCode,
										int PMT_PID,
										int remoconID,
										int serviceType,
										int serviceID,
										int regionID,
										int threedigitNumber,
										int TStreamID,
										int berAVG,
										char *channelName,
										char *TSName,
										int networkID,
										int areaCode,
										unsigned char *frequency)
{
	int err = SQLITE_ERROR;

	tcc_dxb_channel_db_lock_on();

	gCurrentChannel = channelNumber;
	gCurrentCountryCode = countryCode;

	if (g_pISDBTDB != NULL)
	{
		err = TCCISDBT_ChannelDB_Insert_2(channelNumber,
											countryCode,
											PMT_PID,
											remoconID,
											serviceType,
											serviceID,
											regionID,
											threedigitNumber,
											TStreamID,
											berAVG,
											channelName,
											TSName,
											//preset,
											networkID,
											areaCode,
											frequency);

		tcc_dxb_channel_db_lock_off();

		return err;
	}

	tcc_dxb_channel_db_lock_off();

	return err;
}

/* Noah / 20180131 / IM478A-77 (relay station search in the background using 2TS)
	Description
		This fucntion is based on 'UpdateDB_CustomSearch' and is for 'setRelayStationChDataIntoDB' API.
		If J&K App finds "Relay Station Channel" by using 2TS,
			1. J&K App calls 'setRelayStationChDataIntoDB' API to save some data of NIT to DB
			2. 'setRelayStationChDataIntoDB' API calls the following 2 functions.
				2.1. InsertDB_ChannelTable_2
					In order to save the data to Queue (g_pChannelMsg) first
				2.2. UpdateDB_CustomSearch_2
					2.2.1. In order to fit J&K's specification (refer to Requirement_Specification_for_RelayStaion_Search_20140620_V1.06.pdf)
					2.2.2. In order to save the data to real DB.
			3. After 'setRelayStationChDataIntoDB' API is done, J&K App calls 'setChannel' to play.

	Parameter
		int channel      : Input

		int country_code : Input

		int preset       : Input
			RECORD_PRESET_CUSTOM_RELAY          : 3
			RECORD_PRESET_CUSTOM_AFFILIATION    : 4
			else                                : error

		int *tsid        : Input

		void *ptr        : Output

	Return
		0 (SQLITE_OK) : success
		-1            : error, invalid parameter
		-2            : error, invalid channel & country_code, it means there is no NetworkID in g_pChannelMsg.
        -3            : error, invalid tsid, NetworkID is different with tsid.
		-4            : error, fail to update DB
		-5            : error, g_pISDBTDB is NULL
*/
int UpdateDB_CustomSearch_2(int channel, int country_code, int preset, int tsid, void *ptr)
{
	SERVICE_FOUND_INFO_T stServiceInfo;
	int err = -5;
	int iNetworkID;
	int fullseg_id, oneseg_id;

	if(ptr == NULL)
		return -1;

	if(preset != RECORD_PRESET_CUSTOM_RELAY && preset != RECORD_PRESET_CUSTOM_AFFILIATION)
	{
		ALOGD("[%s] preset(%d) is NOT RECORD_PRESET_CUSTOM_RELAY or RECORD_PRESET_CUSTOM_AFFILIATION, Error !!!!!\n", __func__, preset);
		return -1;
	}

	tcc_dxb_channel_db_lock_on();

	if(g_pISDBTDB != NULL)
	{
		/* 1 - check if the tsid is same or not */
		iNetworkID = (int)TCCISDBT_ChannelDB_GetNetworkID (channel, country_code, -1);
		if(iNetworkID == 0)
		{
			ALOGD("[%s] netowrk_id of found channel is not valid / channel(%d), country_code(%d)\n", __func__, channel, country_code);

			/* netowrk_id of found channel is not valid */
			UpdateDB_CustomSearch_MakeReport (1, channel, 0, 0, iNetworkID, ptr);

			tcc_dxb_channel_db_lock_off();
			return -2;
		}

		//if(!UpdateDB_CustomSearch_CheckNetworkID(iNetworkID, tsid))
		if(iNetworkID != tsid)
		{
			ALOGD("[%s] netowrk_id of found channel is different with tsid / iNetworkID(0x%x), tsid(0x%x)\n", __func__, iNetworkID, tsid);

			/* netowrk_id of found channel is different with tsid */
			UpdateDB_CustomSearch_MakeReport (1, channel, 0, 0, iNetworkID, ptr);

			tcc_dxb_channel_db_lock_off();
			return -3;
		}

		/* 2 - delete channel infromatin from DB if necessary */
		if(preset == RECORD_PRESET_CUSTOM_AFFILIATION)
		{
			/* delete records that a value of preset is 2(autosearch) or 4 (affiliation search) */
			UpdateDB_CustomSearch_DelInfo (RECORD_PRESET_AUTOSEARCH, -1);
			UpdateDB_CustomSearch_DelInfo (RECORD_PRESET_CUSTOM_AFFILIATION, -1);

			err = TCCISDBT_ChannelDB_CustomSearch_SearchSameService(g_pISDBTDB, channel, RECORD_PRESET_INITSEARCH, iNetworkID, &fullseg_id, &oneseg_id, 0);
			if(err == SQLITE_OK)
			{
				ALOGD("[%s][A] service found in db(%d)", __func__, RECORD_PRESET_INITSEARCH);

				UpdateDB_CustomSearch_MakeReport (1, channel, fullseg_id, oneseg_id, iNetworkID, ptr);

				tcc_dxb_channel_db_lock_off();
				return 0;
			}

			err = TCCISDBT_ChannelDB_CustomSearch_SearchSameService (g_pISDBTDB, channel, RECORD_PRESET_CUSTOM_RELAY, iNetworkID, &fullseg_id, &oneseg_id, 0);
			if(err == SQLITE_OK)
			{
				ALOGD("[%s][A] service found in db(%d)", __func__, RECORD_PRESET_CUSTOM_RELAY);

				UpdateDB_CustomSearch_MakeReport (1, channel, fullseg_id, oneseg_id, iNetworkID, ptr);

				tcc_dxb_channel_db_lock_off();
				return 0;
			}

			/* insert found service */
			ALOGD("[%s][A] UpdateInfo in db(%d)", __func__, ISDBT_SCANOPTION_CUSTOM_AFFILIATION);

			err = UpdateDB_CustomSearch_UpdateInfo((int)(ISDBT_SCANOPTION_CUSTOM_AFFILIATION), channel, iNetworkID, ptr);
			if(err != SQLITE_OK)
			{
				ALOGE("UpdateDB_CustomSearch_UpdateInfo fail, Error !!!!!");
				tcc_dxb_channel_db_lock_off();
				return -4;
			}

			tcc_dxb_channel_db_lock_off();
			return 0;
		}
		/* 2 - delete channel infromatin from DB if necessary */
		else    // RECORD_PRESET_CUSTOM_RELAY
		{
			/* if not affiliation search, be treated as relay search */
			/* delete records that a vlaue of preset is 3 and tsid is same */
			UpdateDB_CustomSearch_DelInfo(RECORD_PRESET_CUSTOM_RELAY, iNetworkID);

			/* check service of same channel and tsid in preset=0 (initscan or rescan) */
			err = TCCISDBT_ChannelDB_CustomSearch_SearchSameService(g_pISDBTDB, channel, RECORD_PRESET_INITSEARCH, iNetworkID, &fullseg_id, &oneseg_id, 0);
			if(err == SQLITE_OK)
			{
				ALOGD("[%s][R] service found in db(%d)", __func__, RECORD_PRESET_INITSEARCH);

				UpdateDB_CustomSearch_MakeReport(1, channel, fullseg_id, oneseg_id, iNetworkID, ptr);

				tcc_dxb_channel_db_lock_off();
				return 0;
			}

			/* check service of same tsid in preset=2 (autosearch) */
			err = TCCISDBT_ChannelDB_CustomSearch_SearchSameService(g_pISDBTDB, -1, RECORD_PRESET_AUTOSEARCH, iNetworkID, &fullseg_id, &oneseg_id, 1);
			if(err == SQLITE_OK)
			{
				ALOGD("[%s][R] service found in db(%d)", __func__, RECORD_PRESET_AUTOSEARCH);

				TCCISDBT_ChannelDB_CustomSearch_GetChannelInfoReplace(&stServiceInfo);
				TCCISDBT_ChannelDB_CustomSearch_ChannelInfoReplace(g_pISDBTDB, &stServiceInfo, RECORD_PRESET_AUTOSEARCH, iNetworkID);

				if(stServiceInfo.pServiceFound)
					tcc_mw_free(__FUNCTION__, __LINE__, stServiceInfo.pServiceFound);

				UpdateDB_CustomSearch_MakeReport(1, channel, fullseg_id, oneseg_id, iNetworkID, ptr);

				tcc_dxb_channel_db_lock_off();
				return 0;
			}

			/* check service of same tsid in preset=4 (affiliation search) */
			err = TCCISDBT_ChannelDB_CustomSearch_SearchSameService(g_pISDBTDB, -1, RECORD_PRESET_CUSTOM_AFFILIATION, iNetworkID, &fullseg_id, &oneseg_id, 1);
			if(err == SQLITE_OK)
			{
				ALOGD("[%s][R] service found in db(%d)", __func__, RECORD_PRESET_CUSTOM_AFFILIATION);

				TCCISDBT_ChannelDB_CustomSearch_GetChannelInfoReplace (&stServiceInfo);
				TCCISDBT_ChannelDB_CustomSearch_ChannelInfoReplace(g_pISDBTDB, &stServiceInfo, RECORD_PRESET_CUSTOM_AFFILIATION, iNetworkID);

				if(stServiceInfo.pServiceFound)
					tcc_mw_free(__FUNCTION__, __LINE__, stServiceInfo.pServiceFound);

				UpdateDB_CustomSearch_MakeReport(1, channel, fullseg_id, oneseg_id, iNetworkID, ptr);

				tcc_dxb_channel_db_lock_off();
				return 0;
			}

			/* insert found service */
			ALOGD("[%s][R] UpdateInfo in db(%d)", __func__,  ISDBT_SCANOPTION_CUSTOM_RELAY);

			err = UpdateDB_CustomSearch_UpdateInfo((int)(ISDBT_SCANOPTION_CUSTOM_RELAY), channel, iNetworkID, ptr);
			if(err != SQLITE_OK)
			{
				ALOGE("UpdateDB_CustomSearch_UpdateInfo fail, Error !!!!!");
				tcc_dxb_channel_db_lock_off();
				return -4;
			}

			tcc_dxb_channel_db_lock_off();
			return 0;
		}
	}

	tcc_dxb_channel_db_lock_off();
	return err;
}

