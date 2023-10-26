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
#ifndef _TCC_MAIL_DB_H_
#define	_TCC_MAIL_DB_H_

#include "sqlite3.h"


#ifdef __cplusplus
extern "C" {
#endif



/******************************************************************************
* defines 
******************************************************************************/
#define ISDBT_MAIL_MAX_CNT		13
#define ISDB_MAX_MAIL_SIZE		800

/******************************************************************************
* typedefs & structure
******************************************************************************/


/******************************************************************************
* globals
******************************************************************************/

/******************************************************************************
* locals
******************************************************************************/

/******************************************************************************
* declarations
******************************************************************************/
extern int TCCISDBT_MailDB_Update(
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
);
extern int TCCISDBT_MailDB_DeleteStatus_Update(sqlite3 *sqlhandle, int uiMessageID, int uiDeleteStatus);
extern int TCCISDBT_MailDB_UserViewStatus_Update(sqlite3 *sqlhandle, int uiMessageID, int uiUserView_status);


#ifdef __cplusplus
}
#endif

#endif

