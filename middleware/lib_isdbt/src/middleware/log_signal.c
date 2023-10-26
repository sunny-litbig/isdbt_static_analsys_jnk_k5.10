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
#define LOG_TAG "FIELD_LOG"
#include <utils/Log.h>
#include <sys/stat.h>
#endif

#include <stdio.h>
#include <pthread.h>
#include "log_signal.h"

#define DEBUG

#define LOG_FILENAME "log"
#define LOG_FILE_EXT "log"
#define LOG_QSIZE_BUFFER	4096
#define LOG_QSIZE_FLUSH		2048

#define MAX_LOG_FILEPATH_SIZE 64

typedef struct tag_log_signal_status
{
	int f_debug_enable;
	FILE *handle;
	int written_size;
	char filepath[MAX_LOG_FILEPATH_SIZE];
	pthread_mutex_t lock_mutex;
	int write_duration;
	long long ll_prev_write_mjdsec;
} dxb_log_signal_status;

typedef struct {
	unsigned int tmccLock;
	unsigned int mer[3];
	unsigned int vBer[3];
	unsigned int pcber[3];
	unsigned int tsper[3];
	int rssi;
	unsigned int bbLoopGain;
	unsigned int rfLoopGain;
	unsigned int snr;
	unsigned int frequency;
} _Tcc353xStatusToUi;

char szStringLable[] = "#0,UTCExtra,LSI,ConfInd,UTCFlag,MJD,UTC,LocalTime"
",frequency,snr,rssi,rfLoopGain,bbLoopGain,tmccLock"
",mer0,vBer0,pcber0,tsper0,mer1,vBer1,pcber1,tsper1,mer2,vBer2,pcber2,tsper2\n";

dxb_log_signal_status log_handle = { 0, 0, 0, };

/*	ret value
		0  : file opened successfully
		-1 : fail to open file
		-2 : already opened
		-3 : time is not set
*/

static void log_signal_lock_on(void)
{
	dxb_log_signal_status *p_log = &log_handle;
	pthread_mutex_lock(&p_log->lock_mutex);
}

static void log_signal_lock_off(void)
{
	dxb_log_signal_status *p_log = &log_handle;
	pthread_mutex_unlock(&p_log->lock_mutex);
}

void log_signal_init(void)
{
	dxb_log_signal_status *p_log = &log_handle;

	pthread_mutex_init(&p_log->lock_mutex, NULL);
	p_log->write_duration = 1;
	p_log->ll_prev_write_mjdsec = 0;
}

void log_signal_deinit(void)
{
	dxb_log_signal_status *p_log = &log_handle;
	int ret;

	if( p_log->handle )
	{
		log_signal_close();
	}
	pthread_mutex_destroy(&p_log->lock_mutex);
}

int log_signal_open(char *sPath)
{
	dxb_log_signal_status *p_log = &log_handle;
	int ret;
	int mjd, year, month, day, hour, minute, second, msec;

	log_signal_lock_on();

	if( p_log->handle ) {
		log_signal_lock_off();
		ALOGD("[%s] already opened. Filename[%s]\n", __func__, p_log->filepath );
		return -2;
	}

	if(sPath == (char*)NULL) {
		log_signal_lock_off();
		ALOGD("[%s] path error (%s)\n", __func__, sPath);
		return -2;
	}

	ret = tcc_db_get_channel_current_date_precision(&mjd, &year, &month, &day, &hour, &minute, &second, &msec);
	if( ret != 0 ) {
		log_signal_lock_off();
		ALOGD("[%s] time is not set.\n", __func__ );
		return -3;
	}

	/* calc current real time */
	char szFileName[64];
	p_log->written_size = 0;

	sprintf(szFileName, "%s%02d%02d%02d_%02d%02d%02d.%s", sPath,
		year-2000, month, day, hour, minute, second,
		LOG_FILE_EXT );

	/* try to open in SD0 */
	sprintf(p_log->filepath, "%s", szFileName);
	p_log->handle = fopen(p_log->filepath, "w+t");
	if( p_log->handle == NULL )
	{
		ALOGD("[%s] File open fail.\n", __func__ );
		p_log->handle = 0;
		log_signal_lock_off();
		return -1;
	}

	fwrite(szStringLable, sizeof(char), strlen(szStringLable), p_log->handle);

	p_log->write_duration = 1;
	log_signal_lock_off();
	return 0;
}

/* ret value
		0  : close file successfully
		-1 : file is not opened
*/
int log_signal_close(void)
{
	dxb_log_signal_status *p_log = &log_handle;
	int ret;

	log_signal_lock_on();

	if( !p_log->handle )
	{
		ALOGD("[%s] file is not opened\n", __func__ );
		log_signal_lock_off();
		return -1;
	}

	ret = fclose( p_log->handle );
	p_log->handle = 0;

	log_signal_lock_off();

	return 0;
}

/* ret value
		0  : close file successfully
		-1 : file is not opened
*/
int log_signal_write(int *p_data)
{
	dxb_log_signal_status *p_log = &log_handle;
	int ret;
	char szString[1024];

	int mjd, year, month, day, hour, minute, second, msec;
	long long ll_curr_time_mjdsec;
	int i_mjdsec_diff;
	//LOGD("[%s]\n", __func__);

	if( !p_log->handle && !p_log->f_debug_enable )
	{
		return 0;
	}

	ret = tcc_db_get_channel_current_date_precision(&mjd, &year, &month, &day, &hour, &minute, &second, &msec);
	if( ret != 0 )
	{
		ALOGD("[%s] time is not set.\n", __func__ );
		return -3;
	}

	ll_curr_time_mjdsec = second + minute*60 + hour*3600 + mjd*864000;
	i_mjdsec_diff = (int)(ll_curr_time_mjdsec - p_log->ll_prev_write_mjdsec);
	p_log->ll_prev_write_mjdsec = ll_curr_time_mjdsec;

	if( i_mjdsec_diff >= p_log->write_duration || i_mjdsec_diff < -30 )
	{
		if( p_log->f_debug_enable )
		{
			_Tcc353xStatusToUi *pStatus = (_Tcc353xStatusToUi *)(p_data+3);
		#if 0
			LOGD("%02d:%02d:%02d, %d,%d, %d,%d,%d"
					", %d,%d,%d,%d, %d,%d,%d,%d, %d,%d,%d,%d %d,%d\n"
					, hour, minute, second
					, pStatus->snr, pStatus->rssi, pStatus->rfLoopGain, pStatus->bbLoopGain, pStatus->tmccLock
					, pStatus->mer[0], pStatus->vBer[0], pStatus->pcber[0], pStatus->tsper[0]
					, pStatus->mer[1], pStatus->vBer[1], pStatus->pcber[1], pStatus->tsper[1]
					, pStatus->mer[2], pStatus->vBer[2], pStatus->pcber[2], pStatus->tsper[2]
					, *(p_data+1), *(p_data+2)
					);
		#endif
		}

		log_signal_lock_on();

		if( !p_log->handle )
		{
			//ALOGD("[%s] file is not opened\n", __func__ );
			log_signal_lock_off();
			return -1;
		}

		szString[0] = 0;
		{
			_Tcc353xStatusToUi *pStatus = (_Tcc353xStatusToUi *)(p_data+3);

			ret = sprintf(szString,"0,0,0,0,0,0,0,%04d %02d %02d %02d %02d %02d %04d"
									",%d,%d,%d,%d,%d,%d"
									",%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n"
				, year, month, day, hour, minute, second, msec
				, pStatus->frequency, pStatus->snr, pStatus->rssi, pStatus->rfLoopGain, pStatus->bbLoopGain, pStatus->tmccLock
				, pStatus->mer[0], pStatus->vBer[0], pStatus->pcber[0], pStatus->tsper[0]
				, pStatus->mer[1], pStatus->vBer[1], pStatus->pcber[1], pStatus->tsper[1]
				, pStatus->mer[2], pStatus->vBer[2], pStatus->pcber[2], pStatus->tsper[2]
				);
		}
		ret = fwrite (szString, sizeof(char), ret, p_log->handle);
		p_log->written_size += ret;

		log_signal_lock_off();
	}

	return 0;
}

/* ret value
		0  : close file successfully
		-1 : file is not opened
*/
int log_signal_get_status(void)
{
	dxb_log_signal_status *p_log = &log_handle;
	int ret;
	long long llWrittenSize;

	log_signal_lock_on();

	if( !p_log->handle )
	{
		ALOGD("[%s] file is not opened\n", __func__ );
		log_signal_lock_off();
		return -1;
	}
	ALOGD("[%s] filename[%s] written_size=%d\n", __func__, p_log->filepath, p_log->written_size);
	log_signal_lock_off();
	return 0;
}

