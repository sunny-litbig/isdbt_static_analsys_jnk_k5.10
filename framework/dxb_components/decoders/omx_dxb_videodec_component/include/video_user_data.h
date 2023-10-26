/*******************************************************************************

*   FileName : video_user_data.h

*   Copyright (c) Telechips Inc.

*   Description : video_user_data.h

********************************************************************************
*
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
#ifndef __VIDEO_USER_DATA__
#define __VIDEO_USER_DATA__

#if 0//     MOVE_VPU_IN_KERNEL
#define VIDEO_USER_DATA_PROCESS
#endif

#define MAX_USERDATA 4
#define MAX_USERDATA_LIST_ARRAY 16

typedef struct _video_userdata_t
{
	unsigned char *pData;
	int            iDataSize;
} video_userdata_t;

typedef struct _video_userdata_list_s_
{
	int iIndex;
	int iValidCount;
	unsigned char fDiscontinuity;
	unsigned long long ullPTS;
	int iUserDataNum;
	video_userdata_t arUserData[MAX_USERDATA];
} video_userdata_list_t;

extern int UserDataCtrl_Clear( video_userdata_list_t *pUserDataList );
extern int UserDataCtrl_Init( video_userdata_list_t *pUserDataListArray );
extern int UserDataCtrl_ResetAll( video_userdata_list_t *pUserDataListArray );
extern int UserDataCtrl_Put( video_userdata_list_t *pUserDataListArray, int iIndex,
                             unsigned long long ullPTS, unsigned char fDiscontinuity, video_userdata_list_t *pUserDataList );
extern int UserDataCtrl_Get( video_userdata_list_t *pUserDataListArray, int iIndex,
							 video_userdata_list_t **ppUserDataList );

#endif //__VIDEO_USER_DATA__
