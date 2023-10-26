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
#ifndef	_TSPARSE_ISDBT_H_
#define	_TSPARSE_ISDBT_H_

#ifdef __cplusplus
extern "C" {
#endif
    
#include "TsParse.h"
#include "mpeg_ts.h"
#include "mpeg_def.h"
#include "mpeg_ext.h"
#include "mpegpars.h"
#include "mpegpars_DVB.h"
#include "BitParser.h"

enum
{
	SERVICE_TYPE_FSEG		= 0x01,
	SERVICE_TYPE_TEMP_VIDEO	= 0xA1,
	SERVICE_TYPE_1SEG		= 0xC0
};
#define MAX_SUPPORT_CURRENT_SERVICE	32

void ISDBT_TSPARSE_Init(void);
void ISDBT_TSPARSE_Deinit(void);    // Noah / 20180705 / Added for IM478A-51 (Memory Access Timing)

int ISDBT_TSPARSE_PES_data_field (void *handle, unsigned char *pucRawData, int iLenData);
int ISDBT_TSPARSE_ProcessTable (void *handle, unsigned char *pucRawData, unsigned int uiRawDataSize, int RequestID);
void ISDBT_Init_DescriptorInfo(void);
void ISDBT_Init_CA_DescriptorInfo(void);
void ISDBT_TSPARSE_ServiceList_Init (void);

int isdbt_emergency_info_get_data(int *service_id, int *area_code, int *signal_type, int *start_end_flag,  int *area_code_length);
void isdbt_emergency_info_clear(void);
int isdbt_mvtv_info_get_data(int iServiceID, int index, unsigned char *pucComponentTag, int *piComponentCount);
extern int ISDBT_TSPARSE_CURDESCINFO_set(unsigned short usServiceID, unsigned char ucDescID, void *pvDesc);
extern int ISDBT_TSPARSE_CURDCCDESCINFO_get(unsigned short usServiceID, void **pDCCInfo);
extern int ISDBT_TSPARSE_CURDESCINFO_get(unsigned short usServiceID, DESCRIPTOR_IDS ucDescID, void *pvDesc);
extern int ISDBT_TSPARSE_CURDESCINFO_setBitMap(unsigned short usServiceID, unsigned char ucTableID, unsigned char uciDescBitMap);
int ISDBT_GetTOTMJD(void);
int UpdateDB_SpecialService_DelInfo (void);
int UpdateDB_SpecialService_UpdateInfo (int channel_number, int country_code, unsigned short service_id, int *row_id);

extern int ISDBT_Get_1SegServiceCount(void);
extern int ISDBT_Get_TotalServiceCount(void);

#ifdef __cplusplus
}
#endif
#endif	//_TSPARSE_ISDBT_H_

