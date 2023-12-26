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
#define LOG_TAG	"tcc_dxb_timecheck"
#include <utils/Log.h>
#include <tcc_dxb_timecheck.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#if 0
#define tclog ALOGE
#else
#define tclog
#endif

#define TRUE	1
#define FALSE	0
#define FILEROOT "/storage/sdcard0/timecheck.txt"

#define MAXIMUM_FIELD_NUM		100
#define MAXIMUM_RECORD_NUM 		100000

static TTABLE *header;
static pthread_mutex_t timecheckLock;

int calculateTime(struct timespec otime) {
	return (otime.tv_sec&0x0000ffff)*1000 + otime.tv_nsec/1000000;
}

static void tcc_dxb_timecheck_lock() {
	tclog("%s", __func__);
	pthread_mutex_lock(&timecheckLock);
}

static void tcc_dxb_timecheck_unlock() {
	tclog("%s", __func__);
	pthread_mutex_unlock(&timecheckLock);
}

void tcc_dxb_timecheck_record_init(TRECORD **record) {
	tclog("%s", __func__);
	TRECORD *recordTemp;

	*record = (TRECORD *)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(TRECORD));
	if(*record == NULL)
	{
		tclog("[%s] *record is NULL Error !!!!!", __func__);
		return;
	}
	
	recordTemp = *record;
	recordTemp->nextRecord 		= NULL;
	recordTemp->recordTime 		= -1;
}

void tcc_dxb_timecheck_field_init(TFIELD **field) {
	tclog("%s", __func__);
	TFIELD *fieldTemp;

	*field = (TFIELD *)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(TFIELD));
	if(*field == NULL)
	{
		tclog("[%s] *field is NULL Error !!!!!", __func__);
		return;
	}
	
	fieldTemp = *field;
	fieldTemp->fieldName 		= NULL;
	fieldTemp->nextField 		= NULL;
	fieldTemp->recordHeader 	= NULL;
	fieldTemp->recordTail 		= NULL;
	fieldTemp->startTime 		= -1;
	fieldTemp->stopTime 		= -1;
	fieldTemp->maxTime 			= -1;
	fieldTemp->minTime 			= -1;
	fieldTemp->avgTime 			= -1;
	fieldTemp->recordCount 		= 0;
}

void tcc_dxb_timecheck_table_init(TTABLE **table) {
	tclog("%s", __func__);
	TTABLE *tableTemp;

	*table = (TTABLE *)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(TTABLE));
	if(*table == NULL)
	{
		tclog("[%s] *table is NULL Error !!!!!", __func__);
		return;
	}

	tableTemp = *table;
	tableTemp->tableName		= NULL;
	tableTemp->nextTable		= NULL;
	tableTemp->fieldHeader		= NULL;
	tableTemp->fieldCount		= 0;
}

void tcc_dxb_timecheck_free(void *ptr) {
	tclog("%s", __func__);
	if(ptr == NULL)
		return;
	tcc_mw_free(__FUNCTION__, __LINE__, ptr);
}

void tcc_dxb_timecheck_init() {
	tclog("%s", __func__);
	tcc_dxb_timecheck_table_init(&header);
	pthread_mutex_init(&timecheckLock, NULL);
}
void tcc_dxb_timecheck_deinit() {
	tclog("%s", __func__);
	TTABLE *tableTemp;
	TTABLE *prevTableTemp;
	TFIELD *fieldTemp;
	TFIELD *prevFieldTemp;
	TRECORD *recordTemp;
	TRECORD *prevRecordTemp;
	
	if(header == NULL)
		return;
	tableTemp = header;
	while(tableTemp != NULL) {
		fieldTemp = tableTemp->fieldHeader;
		while(fieldTemp != NULL) {
			recordTemp = fieldTemp->recordHeader;
			while(recordTemp != NULL) {
				prevRecordTemp = recordTemp;
				recordTemp = recordTemp->nextRecord;
				tcc_dxb_timecheck_free((void *)prevRecordTemp);
			}
			prevFieldTemp = fieldTemp;
			fieldTemp = fieldTemp->nextField;
			tcc_dxb_timecheck_free((void *)prevFieldTemp);
		}
		prevTableTemp = tableTemp;
		tableTemp = tableTemp->nextTable;
		tcc_dxb_timecheck_free((void *)prevTableTemp);
	}
	header = NULL;
	pthread_mutex_destroy(&timecheckLock);
}

void tcc_dxb_timecheck_set(const char *tableName, const char *fieldName, unsigned char ssFlag) {
	tclog("%s", __func__);
#if USETIMECHECK
	TTABLE *tableTemp;
	TFIELD *fieldTemp;
	TRECORD *recordTemp;
	struct timespec tspec;

	if(header == NULL) {
		tclog("header is null!!!");
		tcc_dxb_timecheck_init();
	}

	tcc_dxb_timecheck_lock();

	if(clock_gettime(CLOCK_MONOTONIC , &tspec)) {
		tclog("failed clock_gettime !!!");
		tcc_dxb_timecheck_unlock();
		return;
	}

	tableTemp = header;
	while(tableTemp != NULL) {
		if(tableTemp->tableName == NULL) {
			tableTemp->tableName = tableName;
			tcc_dxb_timecheck_field_init(&(tableTemp->fieldHeader));
			tableTemp->fieldCount++;
		}			
		else if(strcmp(tableName, tableTemp->tableName)) {
			if(tableTemp->nextTable == NULL)
				tcc_dxb_timecheck_table_init(&(tableTemp->nextTable));
			tableTemp = tableTemp->nextTable;
			continue;
		}
		else
			break;
	}
	
	if(tableTemp->fieldCount > MAXIMUM_FIELD_NUM) {
		tclog("FIELD COUNT OVER %d", MAXIMUM_FIELD_NUM);
		tcc_dxb_timecheck_unlock();
		return;
	}
	
	fieldTemp = tableTemp->fieldHeader;
	while(fieldTemp != NULL) {
		if(fieldTemp->fieldName == NULL) {
			fieldTemp->fieldName = fieldName;
			tcc_dxb_timecheck_record_init(&(fieldTemp->recordHeader));
			fieldTemp->recordCount++;
		}
		else if(strcmp(fieldName, fieldTemp->fieldName)) {
			if(fieldTemp->nextField == NULL)
				tcc_dxb_timecheck_field_init(&(fieldTemp->nextField));
			fieldTemp = fieldTemp->nextField;
			continue;
		}
		else
			break;
	}
	
	if(fieldTemp->recordCount > MAXIMUM_RECORD_NUM) {
		tclog("RECORD COUNT OVER %d", MAXIMUM_RECORD_NUM);
		tcc_dxb_timecheck_unlock();
		return;
	}
	
	if(ssFlag == TIMECHECK_START) {
		tclog("%s case timecheck_start", __func__);
		fieldTemp->startTime = calculateTime(tspec);
	}
	else {
		tclog("%s case timecheck_stop", __func__);
		if(fieldTemp->startTime == -1 || (fieldTemp->startTime > calculateTime(tspec))) {
			tclog("something wrong!!!");
			tcc_dxb_timecheck_unlock();
			return;
		}
		fieldTemp->stopTime = calculateTime(tspec);
		if(fieldTemp->recordTail == NULL) {
			tclog("%s case timecheck_stop first", __func__);
			fieldTemp->recordTail = fieldTemp->recordHeader;
		}
		else {
			tclog("%s case timecheck_stop other", __func__);
			if(fieldTemp->recordTail->nextRecord == NULL) {
				tclog("%s case timecheck_stop record init", __func__);
				tcc_dxb_timecheck_record_init(&(fieldTemp->recordTail->nextRecord));
				fieldTemp->recordCount++;
			}
			fieldTemp->recordTail = fieldTemp->recordTail->nextRecord;
		}
		fieldTemp->recordTail->recordTime = fieldTemp->stopTime - fieldTemp->startTime;
		fieldTemp->maxTime = fieldTemp->maxTime == -1 || (fieldTemp->recordTail->recordTime > fieldTemp->maxTime) ? fieldTemp->recordTail->recordTime : fieldTemp->maxTime;
		fieldTemp->minTime = fieldTemp->minTime == -1 || (fieldTemp->recordTail->recordTime < fieldTemp->minTime) ? fieldTemp->recordTail->recordTime : fieldTemp->minTime;
		fieldTemp->avgTime = (fieldTemp->avgTime * (fieldTemp->recordCount - 1) + fieldTemp->recordTail->recordTime) / fieldTemp->recordCount;
		fieldTemp->startTime 	= -1;
		fieldTemp->stopTime 	= -1;
	}
	tcc_dxb_timecheck_unlock();
#else
	return;
#endif
}

void tcc_dxb_timecheck_printlogAVG() {
	tclog("%s", __func__);
#if USETIMECHECK
	TTABLE *tableTemp;
	TFIELD *fieldTemp;
	if(header == NULL)
		return;

	tableTemp = header;
	while(tableTemp != NULL) {
		ALOGD("%s", tableTemp->tableName);
		fieldTemp = tableTemp->fieldHeader;
		while(fieldTemp != NULL) {
			if(fieldTemp->avgTime == -1 || fieldTemp->maxTime == -1 || fieldTemp->minTime == -1) {
				fieldTemp = fieldTemp->nextField;
				continue;
			}
			ALOGD("FIELD_NAME : %s ( count = %d)", fieldTemp->fieldName, fieldTemp->recordCount);
			ALOGD("AVERAGE    : %10d", fieldTemp->avgTime);
			ALOGD("MAXIMUM    : %10d", fieldTemp->maxTime);
			ALOGD("MINIMUM    : %10d", fieldTemp->minTime);
			fieldTemp = fieldTemp->nextField;
		}
		tableTemp = tableTemp->nextTable;
	}
#else
	return;
#endif
}
void tcc_dxb_timecheck_printlog() {
	tclog("%s", __func__);
#if USETIMECHECK
	TTABLE *tableTemp;
	TFIELD *fieldTemp;
	TRECORD *recordTemp;
	if(header == NULL)
		return;

	tableTemp = header;
	while(tableTemp != NULL) {
		ALOGD("%s", tableTemp->tableName);
		fieldTemp = tableTemp->fieldHeader;
		while(fieldTemp != NULL) {
			ALOGD("%s", fieldTemp->fieldName);
			recordTemp = fieldTemp->recordHeader;
			while(recordTemp != NULL) {
				ALOGD("\t%10d", recordTemp->recordTime);
				recordTemp = recordTemp->nextRecord;
			}
			fieldTemp = fieldTemp->nextField;
		}
		tableTemp = tableTemp->nextTable;
	}
#else
	return;
#endif
}

void tcc_dxb_timecheck_printfile() {
	tclog("%s", __func__);
#if USETIMECHECK
	TTABLE *tableTemp;
	TFIELD *fieldTemp;
	TRECORD *recordTemp;
	FILE *fd = fopen(FILEROOT, "wb");
	if(header == NULL) {
		return;
	}
	tableTemp = header;
	while(tableTemp != NULL) {
		fprintf(fd, "\n%s", tableTemp->tableName);
		fieldTemp = tableTemp->fieldHeader;
		while(fieldTemp != NULL) {
			fprintf(fd, "\n%s", fieldTemp->fieldName);
			recordTemp = fieldTemp->recordHeader;
			while(recordTemp != NULL) {
				fprintf(fd, "\t%d", recordTemp->recordTime);
				recordTemp = recordTemp->nextRecord;
			}
			fprintf(fd, "\n\tAVG\t%d\tMAX\t%d\tMIN\t%d", fieldTemp->avgTime, fieldTemp->maxTime, fieldTemp->minTime);
			fieldTemp = fieldTemp->nextField;
		}
		tableTemp = tableTemp->nextTable;
	}
	fclose(fd);
	tcc_dxb_timecheck_deinit();
#else
	return;
#endif
}
