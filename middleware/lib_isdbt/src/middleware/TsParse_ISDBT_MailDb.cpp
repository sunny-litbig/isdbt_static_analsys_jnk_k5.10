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
#define LOG_TAG "TSPARSE_MAIL_DB"
#include <utils/Log.h>
#endif

#ifdef HAVE_LINUX_PLATFORM
#include <string.h> /* for memset(), memcpy(), strlen()*/
#include <unistd.h> /* for usleep()*/
#endif

#include "TsParse_ISDBT_MailDb.h"
#include "tcc_msg.h"

#define ERR_MSG	ALOGE
#define DBG_MSG //ALOGD

int TCCISDBT_MailDB_Update(
	sqlite3 *sqlhandle,
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
	int err =SQLITE_ERROR;
	char szSQL[4096];
	sqlite3_stmt *stmt;
	int	i=0, rowID=-1, listCnt=0;
	unsigned short uiMail[ISDB_MAX_MAIL_SIZE];

	ALOGE("%s:%d mail_length = %d\n", __func__, __LINE__, uiMail_length);

	memset(uiMail, 0, sizeof(uiMail));

	if(uiMail_length>0 && uiMail_length<ISDB_MAX_MAIL_SIZE*2)
		memcpy((short*)uiMail, (short*)ucMailpayload, uiMail_length);
	else if(uiMail_length>ISDB_MAX_MAIL_SIZE*2)
		memcpy((short*)uiMail, (short*)ucMailpayload, ISDB_MAX_MAIL_SIZE);


	if (sqlhandle != NULL) {
		/* quary uiMessage_ID */
		sprintf(szSQL, "SELECT _id FROM Mail WHERE Message_ID=%d", uiMessage_ID);
		err = sqlite3_prepare_v2(sqlhandle, szSQL, -1, &stmt, NULL);
		if (err == SQLITE_OK) {
#if 1
			while(sqlite3_step(stmt) == SQLITE_ROW)
			{
				rowID = sqlite3_column_int(stmt, 0);
			}
#else
			err = sqlite3_step(stmt);
			if (err == SQLITE_ROW) {
				rowID = sqlite3_column_int(stmt, 0);
			}
#endif
		}
		if(stmt){
			sqlite3_finalize(stmt);
		}

		ALOGE("%s %d err = %d, rowID = %d \n", __func__, __LINE__, err, rowID);
		ALOGE("%s %d uiCA_system_id = %d, uiMessage_ID = %d, uiYear =%d, uiMonth = %d, uiDay =%d\n", __func__, __LINE__, uiCA_system_id, uiMessage_ID, uiYear, uiMonth, uiDay);
		ALOGE("%s %d uiMail_length = %d, uiUserView_status = %d, uiDelete_status=%d uiMail_length =%d \n", __func__, __LINE__, uiMail_length, uiUserView_status, uiDelete_status, uiMail_length);

		sprintf(szSQL, "SELECT COUNT(*) FROM Mail");
		if (sqlite3_prepare_v2(sqlhandle, szSQL, -1, &stmt, NULL) == SQLITE_OK) {
			err = sqlite3_step(stmt);
			if (err == SQLITE_ROW) {
				listCnt = sqlite3_column_int(stmt, 0);
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
			stmt = NULL;
		}
		ALOGE("%s %d listCnt = %d \n", __func__, __LINE__, listCnt);


		if(listCnt < ISDBT_MAIL_MAX_CNT)
		{
			if(rowID == -1)
			{
				/* no Mail */
				sprintf(szSQL, "INSERT INTO Mail (CA_system_id, Message_ID, Year, Month, Day, Mail_length, UserView_status, Delete_status, Mail) \
								VALUES(%d, %d, %d, %d, %d, %d, %d, %d,  ?)",
							uiCA_system_id,
							uiMessage_ID,
							uiYear,
							uiMonth,
							uiDay,
							uiMail_length,
							uiUserView_status,
							uiDelete_status);
				if (sqlite3_prepare_v2(sqlhandle, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK) {
					sqlite3_reset (stmt);
					err = sqlite3_bind_text16(stmt, 1, uiMail, -1, SQLITE_STATIC);
					if (err != SQLITE_OK)
						ALOGE("[%s:%d] ERROR : %d\n", __func__, __LINE__, err);
					err = sqlite3_step(stmt);
					if ((err != SQLITE_ROW) && (err != SQLITE_DONE))
						ALOGE("[%s:%d] ERROR : %d\n", __func__, __LINE__, err);
				}
				if (stmt) {
					sqlite3_finalize(stmt);
					stmt = NULL;
				}
			}
		}
		else
		{
	 			sprintf(szSQL, "UPDATE Mail SET CA_system_id=%d, \
							Year = %d,\
							Month = %d,\
							Day = %d,\
							Mail_length = %d,\
							UserView_status = %d,\
							Delete_status = %d,\
							Mail = ?\
							WHERE Message_ID = %d ",
							uiCA_system_id,
							uiYear,
							uiMonth,
							uiDay,
							uiMail_length,
							uiUserView_status,
							uiDelete_status,
							uiMessage_ID);
				if (sqlite3_prepare_v2(sqlhandle, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK) {
					sqlite3_reset (stmt);
					err = sqlite3_bind_text16(stmt, 1, uiMail, -1, SQLITE_STATIC);
					if (err != SQLITE_OK)
						ALOGE("[%s:%d] ERROR : %d\n", __func__, __LINE__, err);
					err = sqlite3_step(stmt);
					if ((err != SQLITE_ROW) && (err != SQLITE_DONE))
						ALOGE("[%s:%d] ERROR : %d\n", __func__, __LINE__, err);
				}
				if (stmt) {
					sqlite3_finalize(stmt);
					stmt = NULL;
				}
		}
	}
	return err;
}

int TCCISDBT_MailDB_Deletedata(sqlite3 *sqlhandle, int uiRowID)
{
	int err = SQLITE_ERROR;
	char szSQL[4096];
	sqlite3_stmt *stmt;
	char *errMsg;

	if (sqlhandle != NULL) {
		sprintf(szSQL, "delete _id FROM Mail WHERE _id =%d", uiRowID);
		if (sqlite3_prepare_v2(sqlhandle, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK) {
			err = sqlite3_exec(sqlhandle, szSQL, NULL, NULL, &errMsg);
			if(err != SQLITE_OK)
			{
				ALOGE("%s %d DB Delete error : Msg=%s\n", __func__,  __LINE__, errMsg);
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
			stmt = NULL;
		}
	}

	return err;
}

int TCCISDBT_MailDB_DeleteStatus_Update(sqlite3 *sqlhandle, int uiMessageID, int uiDeleteStatus)
{
	int err =SQLITE_ERROR;
	char szSQL[4096];
	sqlite3_stmt *stmt;
	int	i=0;

	if (sqlhandle != NULL) {
		/* quary uiMessage_ID */
		sprintf(szSQL, "SELECT _id FROM Mail WHERE Message_ID=%d", uiMessageID);
		if (sqlite3_prepare_v2(sqlhandle, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK) {
			for(i=0; i<ISDBT_MAIL_MAX_CNT; i++)
			{
				err = sqlite3_step(stmt);
				if (err == SQLITE_ROW) {
					err = sqlite3_column_int(stmt, 0);
				}
				if (err == SQLITE_DONE || err == SQLITE_ERROR)
					break;
				usleep(5000);
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
			stmt = NULL;
		}

 		if (err == SQLITE_ROW){
			/* found Mail */
 			sprintf(szSQL, "UPDATE Mail SET Delete_status = %d", uiDeleteStatus);
			if (sqlite3_prepare_v2(sqlhandle, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK) {
				sqlite3_reset (stmt);
				err = sqlite3_step(stmt);
				if ((err != SQLITE_ROW) && (err != SQLITE_DONE))
					ALOGE("[%s:%d] ERROR : %d\n", __func__, __LINE__, err);
			}
			if (stmt) {
				sqlite3_finalize(stmt);
				stmt = NULL;
			}
 		} else {
			/* error */
			err = SQLITE_ERROR;
		}
	}
	return err;
}

int TCCISDBT_MailDB_UserViewStatus_Update(sqlite3 *sqlhandle, int uiMessageID, int uiUserView_status)
{
	int err =SQLITE_ERROR;
	char szSQL[4096];
	sqlite3_stmt *stmt;
	int	i=0;

	if (sqlhandle != NULL) {
		/* quary uiMessage_ID */
		sprintf(szSQL, "SELECT _id FROM Mail WHERE Message_ID=%d", uiMessageID);
		if (sqlite3_prepare_v2(sqlhandle, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK) {
			for(i=0; i<ISDBT_MAIL_MAX_CNT; i++)
			{
				err = sqlite3_step(stmt);
				if (err == SQLITE_ROW) {
					err = sqlite3_column_int(stmt, 0);
				}
				if (err == SQLITE_DONE || err == SQLITE_ERROR)
					break;
				usleep(5000);
			}
		}
		if (stmt) {
			sqlite3_finalize(stmt);
			stmt = NULL;
		}

 		if (err == SQLITE_ROW){
			/* found Mail */
 			sprintf(szSQL, "UPDATE Mail SET UserView_status = %d", uiUserView_status);
			if (sqlite3_prepare_v2(sqlhandle, szSQL, strlen(szSQL), &stmt, NULL) == SQLITE_OK) {
				sqlite3_reset (stmt);
				err = sqlite3_step(stmt);
				if ((err != SQLITE_ROW) && (err != SQLITE_DONE))
					ALOGE("[%s:%d] ERROR : %d\n", __func__, __LINE__, err);
			}
			if (stmt) {
				sqlite3_finalize(stmt);
				stmt = NULL;
			}
 		} else {
			/* error */
			err = SQLITE_ERROR;
		}
	}
	return err;
}



