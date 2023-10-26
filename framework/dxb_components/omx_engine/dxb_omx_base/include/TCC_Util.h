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
#ifndef	_TCC_UTIL_H__
#define	_TCC_UTIL_H__

/******************************************************************************
* include
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <sys/vfs.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <unistd.h>

/******************************************************************************
* defines
******************************************************************************/
#define STORAGE_NAND     "/storage/sdcard0"
#define STORAGE_SDCARD   "/storage/sdcard1"

#define DEVICE_NAME_LENGTH		80
#define MOUNT_DIRECTORY_LENGTH	80
#define FILESYSTEM_LENGTH		12

/******************************************************************************
* typedefs & structure
******************************************************************************/
struct f_size{
	long byteperblock;
	long blocks;
	long avail;
};

typedef struct _mountinfo {
	FILE *fp;			// 파일 스트림 포인터
	char devname[DEVICE_NAME_LENGTH];			// 장치 이름
	char mountdir[MOUNT_DIRECTORY_LENGTH];		// 마운트 디렉토리 이름
	char fstype[FILESYSTEM_LENGTH];				// 파일 시스템 타입
	struct f_size size;	// 파일 시스템의 총크기/사용율
} MOUNTP;


/******************************************************************************
* declarations
******************************************************************************/
MOUNTP *dfopen();
MOUNTP *dfget(MOUNTP *MP, int deviceType);
MOUNTP *dfgetex(MOUNTP *MP, unsigned char *pStrPath);
int dfclose(MOUNTP *MP);

#endif //_TCC_UTIL_H__
