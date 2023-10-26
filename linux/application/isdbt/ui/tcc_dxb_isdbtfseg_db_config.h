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
#ifndef __TCC_DXB_ISDBTFSEG_DB_CONFIG_H__
#define __TCC_DXB_ISDBTFSEG_DB_CONFIG_H__

/* dupliacated from TsParse_ISDBT_DBLayer.h */

#define ISDBT_DB_PATH			"/tcc_data"
#define ISDBT_LOG_DB_PATH		"/tcc_gtk"
#define ISDBT_SHM_PATH			"/dev/shm"
#define ISDBT_MEMORYDB_PATH		":memory:"

#ifdef EPGDB_ON_RAMDISK
#define ISDBT_EPGDB_PATH		ISDBT_SHM_PATH"/ISDBTEPG.db"//ISDBT_DB_PATH"/ISDBTEPG.db"
#else
#define ISDBT_EPGDB_PATH		ISDBT_DB_PATH"/ISDBTEPG.db"
#endif
#define ISDBT_BASICDB_PATH		ISDBT_DB_PATH"/ISDBT.db"
#define ISDBT_CHDB_HOME_PATH	ISDBT_DB_PATH"/ISDBT_CH_HOME.db"
#define ISDBT_CHDB_AUTO_PATH	ISDBT_DB_PATH"/ISDBT_CH_AUTO.db"
#define ISDBT_LOGODB_PATH		ISDBT_LOG_DB_PATH"/ISDBTLogo.db"

#define SQLITE_DXB_ISDBTFSEG_FILTER_1			"((serviceID > 0 ) AND ((serviceType == 1) OR ((serviceType == 192) AND ((audioPID > 2 AND audioPID < 8191) OR (videoPID > 2 AND videoPID < 8191)))))"

#endif //__TCC_DXB_ISDBTFSEG_DB_CONFIG_H__
