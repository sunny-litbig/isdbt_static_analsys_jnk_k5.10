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
#define LOG_NDEBUG 0
#define LOG_TAG	"ISDBT_MANAGER_TUNER"
#include <utils/Log.h>
#include <cutils/properties.h>

#include <fcntl.h>
#include <pthread.h>

#include "BitParser.h"
#include "ISDBT_Common.h"

#include "tcc_isdbt_manager_tuner.h" 
#include "tcc_isdbt_manager_demux.h" 
#include "tcc_dxb_interface_tuner.h"
#include "tcc_isdbt_event.h"
#include "tcc_isdbt_proc.h"
#include "TsParse_ISDBT.h"
#include "TsParse_ISDBT_DBLayer.h"
#include "ISDBT_Region.h"
#include "TsParse_ISDBT_SI.h"

#include <tcc_dxb_memory.h>
#include <tcc_dxb_queue.h>
#include <tcc_dxb_semaphore.h>
#include <tcc_dxb_thread.h>

#ifndef MNG_DBG 
#define MNG_DBG(msg...) ALOGD(msg)
#endif

typedef struct {
	unsigned int uiMinChannel;
	unsigned int uiMaxChannel;
	unsigned int uiCountryCode;
}TUNER_SEARCH_INFO;

static int giTunerScanStop = 0;
static int giTunerSuspend = 0;

static int giNetworkID;
static T_DESC_EBD gstDescEBD;
static T_DESC_TDSD gstDescTDSD;
static int giISTSFileInput = 0; //0:Tuner Input, 1:Input from TS file

int iArraySvcRowID[32+4];
int	iAutoSearchEvt[3];

int iArrayCustomSearchStatus[32+7];
static int giTunerLastChannel = -1;

static DxBInterface gInterface;
DxBInterface *hInterface = &gInterface;

int tcc_tuner_set_countrycode(int countrycode)
{
	int err=0;
	if(giISTSFileInput)
	    return 0;

	err = TCC_DxB_TUNER_SetCountrycode(hInterface, 0, countrycode);
	return 0;
}

int tcc_tuner_open(int countrycode)
{
	int err=0;
	unsigned int iARGS[2];
	MNG_DBG("In %s\n", __func__);
	if(giISTSFileInput)
	    return 0;

	giTunerScanStop = 0;
	giTunerSuspend = 0;
	giTunerLastChannel = -1;
	iARGS[0] = (unsigned int)countrycode;
	iARGS[1] = 1; // 1 means ISDBT of TCC351X
	err = TCC_DxB_TUNER_Open(hInterface, 0, iARGS);
	if(err != 0) {
		ALOGE("error in %s \n", __func__);
	}

	return err;
}

int tcc_tuner_close(void)
{
	int err=0;
	MNG_DBG("In %s\n", __func__);
	if(giISTSFileInput)
	    return 0;
	
	giTunerLastChannel = -1;
	err = TCC_DxB_TUNER_Close(hInterface, 0);
	if(err != 0) {
		ALOGE("error in %s \n", __func__);
 	}

	return err;
}

/*
  * option  b[0] = 1 : check cancel
  */
int tcc_tuner_select_channel(int iCh, int lockOn, int option)
{
	int err=0;
	int wait_count = 2500/50; /* for 2.5 sec, 50ms unit */
	int lock_status;

	MNG_DBG("In %s\n", __func__);
	if(giISTSFileInput)
	    return 0;
	
	giTunerSuspend = 1;

	if(iCh != giTunerLastChannel) {
#if 1
	err = TCC_DxB_TUNER_SetChannel(hInterface, 0, iCh, lockOn);
	if (err == DxB_ERR_TIMEOUT) {
		while (wait_count > 0) {	/* try to get RF lock status */
			if (option & 0x01) {	/* check cancel */
				if (giTunerScanStop) {
					break;
				}
			}
			usleep(50*1000);
			wait_count--;
			if ((wait_count % 4)==0) {	/* check lock status per every 200ms */
				err = TCC_DxB_TUNER_GetLockStatus(hInterface, 0, &lock_status);
				if (err != DxB_ERR_TIMEOUT)
					break;
			}
		}
	}
	if (err != DxB_ERR_OK)
		err = DxB_ERR_ERROR;
#else
	err = TCC_DxB_TUNER_SetChannel(0, iCh, lockOn);
#endif
	if(err != 0) {
		ALOGE("error in %s \n", __func__);
	}

		giTunerLastChannel = iCh;

	}

	giTunerSuspend = 0;

	return err;
}

int tcc_tuner_get_channel_validity(int *pCh)
{
	int err = -1;
	if (giISTSFileInput)
		return err;

	if (pCh != NULL) {
		err = TCC_DxB_TUNER_GetChannelValidity (hInterface, 0, pCh);
		if (err != 0)
			err = -1;
	}
	return err;
}

int tcc_tuner_get_strength(int *sqinfo)
{
	int err=0;

	if (giISTSFileInput) {
		int *ptr = sqinfo;
		*ptr++ = sizeof(int)*2;
		*ptr++ = 100;
		*ptr = 100;
        return 0;
    }

	err = TCC_DxB_TUNER_GetSignalStrength(hInterface, 0, (void *)sqinfo);
	if (err != 0) {
		ALOGE("error in %s \n", __func__);
		return err;
	}

	return err;
}

int tcc_tuner_get_strength_index (int *strength_index)
{
	int rtn = -1, err;
	int	tuner_strength_index;

	if (giISTSFileInput) {
		*strength_index = -100;
		rtn = -1; 
	} else {
		*strength_index = -100;
		err = TCC_DxB_TUNER_GetSignalStrengthIndex(hInterface, 0, &tuner_strength_index);
		if (err == 0) {
			*strength_index = tuner_strength_index;
			rtn = 0;
		}
	}
	return rtn;
}

/*
 * to get required time to retrieve correct rssi from tuner after RF lock
 */
int tcc_tuner_get_strength_indx_time (int *strength_index_time)
{
	int rtn = -1, err;
	int tuner_str_idx_time;

	*strength_index_time = 0;
	if (giISTSFileInput) {
		rtn = -1;
	} else {
		err = TCC_DxB_TUNER_GetSignalStrengthIndexTime(hInterface, 0, &tuner_str_idx_time);
		if (err == 0) {
			*strength_index_time = tuner_str_idx_time;
			rtn = 0;
		}
	}
	return rtn;
}

int tcc_tuner_get_ews_flag(void *pStartFlag)
{
	int err = 0;
   	if(giISTSFileInput)
	    return 0;

	err = TCC_DxB_TUNER_Get_EWS_Flag(hInterface, 0, pStartFlag);
	if(err != 0){
		ALOGE("error in %s \n", __func__);
	}
	return err;
}

void tcc_tuner_event(DxB_TUNER_NOTIFY_EVT nEvent, void *pEventData, void *pUserData)
{
	TCCDxBEvent_TunerCustomEvent(pEventData);
}

void tcc_manager_tuner_set_strength_index_time(void)
{
	int strength_index_time;

	tcc_tuner_get_strength_indx_time (&strength_index_time);
	tcc_manager_demux_set_strengthIndexTime(strength_index_time);
}
int tcc_manager_tuner_init(int uiBBSelect)
{
	int status;
    char    value[PROPERTY_VALUE_MAX];
	unsigned int   uiLen = 0;

	MNG_DBG("tcc_mananager_tuner_init");
   	uiLen = property_get ("tcc.dxb.file.input", value, "");
	if (uiLen) {
        FILE *fp;
        fp = fopen (value, "r"); //TSFILE_NAME
		if (fp) {
    	    giISTSFileInput = 1;
    	    fclose(fp);
        }
	    ALOGE("%s:File Input[%s][valid:%d]", __func__, value, giISTSFileInput);
    } else
		giISTSFileInput = 0;
    if(giISTSFileInput)
        return 0;

	TCC_DxB_TUNER_Init(hInterface, DxB_STANDARD_ISDBT, uiBBSelect);
	TCC_DxB_TUNER_RegisterEventCallback(hInterface, tcc_tuner_event, 0);

	tcc_manager_demux_set_callback_getBER(tcc_tuner_get_strength_index);

	return 0;
}

int tcc_manager_tuner_deinit(void)
{
	TCC_DxB_TUNER_Deinit(hInterface);
	return 0;
}

int tcc_manager_tuner_open(int icountrycode)
{
	int error = 0;
	int strength_index_time;

	error = tcc_tuner_open(icountrycode);
	return error;
}
	
int tcc_manager_tuner_close(void)
{
	int error = 0;
	error = tcc_tuner_close();
	return error;
}

int tcc_manager_tuner_is_suspending(void)
{
	return giTunerSuspend;
}

int tcc_manager_tuner_scanflag_init(void)
{
	giTunerScanStop = 0;
	return 0;
}

int tcc_manager_tuner_scanflag_get(void)
{
	return giTunerScanStop;
}

int tcc_manager_tuner_select_channel (int iCh, int lockOn, int option)
{
	int rtn;

	rtn = tcc_tuner_select_channel(iCh, lockOn, option);
	tcc_manager_tuner_set_strength_index_time();	//because of customer's request, call this api regardless of success/failure of locking
	return rtn;
}

int tcc_manager_tuner_scan_one_channel (int scan_type, int channel_num, int country_code, int area_code, int options)
{
	int scan_options = 0;
	int 	err = 0;

	MNG_DBG("In %s\n", __func__);

	if (giTunerScanStop)
	{
		giTunerScanStop = 0;
		return 1;
	}

	if (options & 1)
	{
		tcc_manager_demux_delete_db();
	}

	tcc_tuner_set_countrycode(country_code);

	err = tcc_manager_tuner_select_channel(channel_num, 1, 1);

    /* Noah / 20180717 / IM478A-30 / (Improvement No3) At a weak signal, improvement of "searchAndSetChannel"
        Repoened IM478A-30 last week becuase as following.
            - the searchAndSetChannel API was called.
            - tcc_manager_tuner_select_channel returned DxB_ERR_ERROR due to lock failure.
            - as a result, the searchAndSetChannel API did NOT work.
        Tasaka said as following.
            Our improvement image is similar to 'setChannel'.
                ex) (1). InitScan
                    (2). set disable Relay station search'
                    (3). Disconnect Antenna.
                    (4). We call setChannel (NHK).
                        --> error in tcc_tuner_select_channel
                    (5). Wait for a few ten seconds
                    (6). Connected Antenna  -->  NHK OK
            In short, we would like to eliminate the difference in behavior between 'setChannel' and 'searchAndSetChannel' as much as possible.
        So, I added the following if statement to ignore return value of tcc_manager_tuner_select_channel in case of searchAndSetChannel(INFORMAL_SCAN).
    */
	if(INFORMAL_SCAN == scan_type)
	{
        err = 0;
	}

	if (err == 0)
	{
		MNG_DBG("Frequency find !!!, Let's find PAT, PMT, SDT ...\n");

		tcc_manager_demux_set_tunerinfo(channel_num, 0, country_code);

		if(scan_type == MANUAL_SCAN)
		{
			scan_options = ISDBT_SCANOPTION_ARRANGE_RSSI | ISDBT_SCANOPTION_MANUAL;
		}
		else if(scan_type == INFORMAL_SCAN)    // Noah / 20170706 / IM478A-30 / In case of the searchAndSetChannel, NIT & BIT have infinite timer.
		{
			scan_options = ISDBT_SCANOPTION_ARRANGE_RSSI | ISDBT_SCANOPTION_INFORMAL | ISDBT_SCANOPTION_INFINITE_TIMER;
		}
		else
		{
			scan_options = ISDBT_SCANOPTION_ARRANGE_RSSI | ISDBT_SCANOPTION_INFORMAL;
		}

		err = tcc_manager_demux_scan(channel_num, country_code, scan_options, (void*)&area_code);
		if (err == 0)
		{
			MNG_DBG("OK found PAT, PMT, SDT ...\n");
		}
	}
	else
		err = -1;

	if (giTunerScanStop)
	{
		giTunerScanStop = 0;
		err = 1;
	}

	return err;
}

int tcc_manager_tuner_init_scan(int country_code)
{
	int err=0, percent=0, channel=0;
	unsigned int i;
	TUNER_SEARCH_INFO search_info;
	int	dmx_scan_rtn;

	MNG_DBG("In %s\n", __func__);

	if (giISTSFileInput) {
        search_info.uiMinChannel = 0;
        search_info.uiMaxChannel = 0;
        search_info.uiCountryCode = country_code;
	} else {
    	tcc_tuner_set_countrycode(country_code);
	    err = TCC_DxB_TUNER_GetChannelInfo(hInterface, 0, &search_info);
    	if(err != 0){
	    	ALOGE("error in %s \n", __func__);
		    return -1;
    	}
    }

	MNG_DBG("%s ContryCode(%d)CH(%d~%d)\n", __func__,\
			search_info.uiCountryCode, search_info.uiMinChannel, search_info.uiMaxChannel);	

	if (giTunerScanStop) {
		giTunerScanStop = 0;
		err = 1;
	} else {
		for (i = search_info.uiMinChannel; i <= search_info.uiMaxChannel; i++) {
			err = tcc_manager_tuner_select_channel(i, 1, 1);
			if (err == 0) {
				MNG_DBG("Frequency find !!!, Let's find PAT, PMT, SDT ...\n");
				tcc_manager_demux_set_tunerinfo(i, 0, search_info.uiCountryCode);
				dmx_scan_rtn = tcc_manager_demux_scan(i, search_info.uiCountryCode, ISDBT_SCANOPTION_ARRANGE_RSSI, NULL);
				if( dmx_scan_rtn == 0 ) {
					MNG_DBG("OK found PAT, PMT, SDT ...\n");
				} else if (dmx_scan_rtn == 1) {
					MNG_DBG("Scan canceled");
				} else {
					MNG_DBG("Fail to found PSI");
				}
			}
			percent  = ((i-search_info.uiMinChannel+1)*100)/(search_info.uiMaxChannel - search_info.uiMinChannel + 1);
			channel = (err == 0) ? (int)i : -1;
			TCCDxBEvent_SearchingProgress((void*)percent, channel);		
			if(giTunerScanStop){
				giTunerScanStop = 0;
				err = 1;
				break;
			} else
				err = 0;
		}
	}

	return err;
}

int tcc_manager_tuner_re_scan(int country_code)
{
	int err=0, percent=0, channel=0;
	int i=0, j=0;
	int ch_count=0, *pCh = NULL;
	TUNER_SEARCH_INFO search_info;
	
	MNG_DBG("In %s\n", __func__);

	if (giISTSFileInput) {
        search_info.uiMinChannel = 0;
        search_info.uiMaxChannel = 0;
        search_info.uiCountryCode = country_code;
	} else {
        tcc_tuner_set_countrycode(country_code);
        err = TCC_DxB_TUNER_GetChannelInfo(hInterface, 0, &search_info);
        if(err != 0){
            ALOGE("error in %s \n", __func__);
            return -1;
        }
    }    

	MNG_DBG("%s ContryCode(%d)CH(%d~%d)\n", __func__,\
			search_info.uiCountryCode, search_info.uiMinChannel, search_info.uiMaxChannel);	

	if (giTunerScanStop) {
		giTunerScanStop = 0;
		err = 1;
	} else {
		for (i = (int)search_info.uiMinChannel; i <= (int)search_info.uiMaxChannel; i++) {
			err = tcc_manager_tuner_select_channel(i, 1, 1);
			if (err == 0) {
				MNG_DBG("Frequency find !!!, Let's find PAT, PMT, SDT ...\n");
				tcc_manager_demux_set_tunerinfo(i, 0, search_info.uiCountryCode);
				if( tcc_manager_demux_scan(i, search_info.uiCountryCode, ISDBT_SCANOPTION_RESCAN, NULL) == 0 ){
					MNG_DBG("OK found PAT, PMT, SDT ...\n");
				}
			}

			percent  = ((i-search_info.uiMinChannel+1)*100)/(search_info.uiMaxChannel - search_info.uiMinChannel + 1);
			channel = (err == 0) ? i : -1;
			TCCDxBEvent_SearchingProgress((void*)percent, channel);
			if(giTunerScanStop){
				giTunerScanStop = 0;
				err = 1;
				break;
			} else
				err = 0;
		}
	}

	return err;
}

int tcc_manager_tuner_region_scan(int country_code, int area_code)
{
	int err=0, percent=0, channel=0, ch=0, region=0;
	unsigned int i;
	unsigned int total_ch_num=0;
	
	MNG_DBG("In %s\n", __func__);
	if(giISTSFileInput)
	    return 0;

	if (area_code >= E_REGION_ID_HOKKAIDO_SAPPORO){
		region = area_code -E_REGION_ID_HOKKAIDO_SAPPORO;
	}else{
		region = area_code;
	}
	
	tcc_tuner_set_countrycode(country_code);

	total_ch_num = ISDBT_Get_TotalChNum_from_RegionInfo(region);
	if(total_ch_num > 0){
		if (giTunerScanStop) {
			giTunerScanStop = 0;
			err = 1;
		} else {
			for(i=0 ; i<total_ch_num ; i++){
				ch = ISDBT_Get_FreqIndex_from_RegionInfo(i, region);
				if(ch >= 0){
					/* actual ch number = ch + 13(ie. ch index start from zero) */
					err = tcc_manager_tuner_select_channel(ch, 1, 1);
					if (err == 0) {
						MNG_DBG("Frequency find !!!, Let's find PAT, PMT, SDT ...\n");
						tcc_manager_demux_set_tunerinfo(ch, 0, country_code);
						if( tcc_manager_demux_scan(ch, country_code, ISDBT_SCANOPTION_NONE, NULL) == 0 ){
							MNG_DBG("OK found PAT, PMT, SDT ...\n");
						}
					}
				}

				percent  = ((i+1)*100)/(total_ch_num);
				channel = (err == 0) ? (int)i : -1;
				TCCDxBEvent_SearchingProgress((void*)percent, channel);
				if(giTunerScanStop){
					giTunerScanStop = 0;
					err = 1;
					break;
				} else
					err = 0;
			}
		}
	}
	return err;
}

int tcc_manager_tuner_autosearch_get_rowid(SERVICE_FOUND_INFO_T *pSvc, int *piRowArray, int max_no)
{
	int i, err;
	int svc_no = pSvc->found_no;
	int row_id, found_count;

	int	fullseg_row_id,oneseg_row_id;
	unsigned int fullseg_service_id, oneseg_service_id;

	found_count = 0;
	for (i=0; i < max_no;i++) piRowArray[i+4] = 0;	//clear svc 

	for (i=0; i <svc_no && i < max_no; i++) {
		row_id = tcc_db_get_channel_rowid (pSvc->pServiceFound[i].uiCurrentChannelNumber, pSvc->pServiceFound[i].uiCurrentCountryCode,
				pSvc->pServiceFound[i].uiNetworkID, pSvc->pServiceFound[i].uiServiceID, pSvc->pServiceFound[i].uiServiceType);
		pSvc->pServiceFound[i].rowID = row_id;
		if (row_id != -1) {
			piRowArray[found_count+4] = row_id;
			found_count++;
		}
	}
	piRowArray[3] = found_count;
	if (found_count > 0) {
		//search primary service
		fullseg_row_id = oneseg_row_id = 0;
		fullseg_service_id = oneseg_service_id = 0;

		for (i=0; i < svc_no && i < max_no; i++) {
			if ((int)pSvc->pServiceFound[i].rowID != -1) {
				if(pSvc->pServiceFound[i].uiServiceType == SERVICE_TYPE_FSEG || pSvc->pServiceFound[i].uiServiceType == SERVICE_TYPE_TEMP_VIDEO) {
					if (fullseg_row_id == 0) {
						fullseg_row_id = pSvc->pServiceFound[i].rowID;
						fullseg_service_id = pSvc->pServiceFound[i].uiServiceID;
					}
				} else if (pSvc->pServiceFound[i].uiServiceType == SERVICE_TYPE_1SEG) {
					if (oneseg_row_id == 0) {
						oneseg_row_id = pSvc->pServiceFound[i].rowID;
						oneseg_service_id = pSvc->pServiceFound[i].uiServiceID;
					}
				}
			}
		}
		piRowArray[1] = fullseg_row_id;
		piRowArray[2] = oneseg_row_id;
	} else {
		piRowArray[1] = 0;
		piRowArray[2] = 0;
	}
	return found_count;
}

/*
*    Search any channels from the next of current channel in specified direction.
*    If finding any channel, scan stops.
*
*        country_code = 0 (Japan) or 1 (Brazil)
*        channel_num - current channel number. -1 means no channel was selected.
*            Japan
*                Min : 0 (473.143)
*                Max : 102 (Ref. invalid from 40 ~ 102 during scanning)
*                      40 is 713.143
*                      39 is 707.143
*
*        options - direction. 0=down (forwarding to lower UHF channel no.), 1=up (increasing UHF channel no)
*
*    Examples in Japan
*            channel_num    options
*            lower than -1  don't care   Error
*            -1             0            from Max to 473.143
*            -1             1            from 473.143 to Max
*            0              0            Error
*            0              1			 from 479.143 to Max
*            10             0            from 527.143 to 473.143
*            10             1            from 539.143 to Max
*            50             0            from 49 to 473.143 (invalid from 49 ~ 40 during scanning), so real is 707.143 to 473.143
*            50             1            from 51 to Max (all channels are invalid during scanning)

*            110            don't care   Error
*/
int tcc_manager_tuner_autosearch (int country_code, int channel_num, int options)
{
	int err = 0, percent = 0, channel = -1;
	int i = 0, searchable_ch_cnt = 0, try_cnt = 0, found_cnt = 0, channel_validity = 0;
	TUNER_SEARCH_INFO search_info = { 0, };
	SERVICE_FOUND_INFO_T stSvcFound = { 0, };
		//[0] = status 0=being searching, -1=failure, 1=success
		//[1] = scan percentage in case of being searching
		//        rowID of channel DB for primary fullseg service in success
		//[2] = rowID of channel DB for primary 1seg service in success
		//[3] = total count for services
		//[4~35] = rowID of channel DB for all found services

	if (giISTSFileInput)
		return err;

	for (i=0; i < 32+4; i++)
		iArraySvcRowID[i] = 0;

	if (giTunerScanStop)
	{
		giTunerScanStop = 0;

		iArraySvcRowID[0] = -1;
		TCCDxBEvent_AutoSearchDone (iArraySvcRowID);
		return 1;
	}

	if(channel_num < -1)
	{
		ALOGE("[%s] channel_num < -1 Error !!!!!\n", __func__);
		goto AUTOSEARCH_END;
	}

	if(options != 0 && options != 1)
	{
		ALOGD("[%s] options : 0x%d is not valid", __func__, options);
		goto AUTOSEARCH_END;
	}

	tcc_tuner_set_countrycode(country_code);
	err = TCC_DxB_TUNER_GetChannelInfo(hInterface, 0, &search_info);
	if(err != 0)
	{
		ALOGE("error in %s \n", __func__);
		goto AUTOSEARCH_END;
	}
	ALOGD("[%s] search_info / uiMinChannel : %d, uiMaxChannel : %d\n",
		__func__, search_info.uiMinChannel, search_info.uiMaxChannel);

	if(channel_num == -1)    // -1 is to search from max or min.
	{
		if(options == 0)    // From max to 0
			channel_num = search_info.uiMaxChannel+1;
		else                // From 0 to max
			channel_num = search_info.uiMinChannel-1;

		searchable_ch_cnt = search_info.uiMaxChannel - search_info.uiMinChannel + 1;
	}
	else
	{
/*		
		if((unsigned int)channel_num < search_info.uiMinChannel)
		{
			ALOGD("[%s] case 1 / channel_num=%d is out of range (%d ~ %d)",
				__func__, channel_num, search_info.uiMinChannel, search_info.uiMaxChannel);
			goto AUTOSEARCH_END;
		}
*/
		if(search_info.uiMaxChannel < (unsigned int)channel_num)
		{
			ALOGD("[%s] case 2 / channel_num=%d is out of range (%d ~ %d)", __func__,
				channel_num, search_info.uiMinChannel, search_info.uiMaxChannel);
			goto AUTOSEARCH_END;
		}

		if(options == 0 && (unsigned int)channel_num <= search_info.uiMinChannel)
		{
			ALOGD("[%s] case 3 / no channels to search (%d,%d)", __func__, channel_num, options);
			goto AUTOSEARCH_END;
		}
/*
		if(options == 1 && search_info.uiMaxChannel <= (unsigned int)channel_num)
		{
			ALOGD("[%s] case 4 / no channels to search (%d,%d)", __func__, channel_num, options);
			goto AUTOSEARCH_END;
		}
*/

		//search_ch_cnt, try_cnt - to informa progressive rate to UI
		if(options == 0)
			searchable_ch_cnt = channel_num - search_info.uiMinChannel;
		else
			searchable_ch_cnt = search_info.uiMaxChannel - channel_num;

		if(searchable_ch_cnt == 0 ||  (int)(search_info.uiMaxChannel - search_info.uiMinChannel) < searchable_ch_cnt)
		{
			ALOGD("In %s, invalid searchable_ch_cnt. channel_num=%d, option=%d, searchable_ch_cnt=%d, (%d,%d)", 
				__func__, channel_num, options, searchable_ch_cnt, search_info.uiMinChannel, search_info.uiMaxChannel);
			searchable_ch_cnt = 1;
		}
	}
	try_cnt = 0;

	stSvcFound.found_no = 0;
	stSvcFound.pServiceFound = NULL;

	while(1)
	{
		if(giTunerScanStop)
		{
			giTunerScanStop = 0;
			break;
		}

		if(options == 0)
			channel_num--;
		else
			channel_num++;

		channel_validity = tcc_tuner_get_channel_validity(&channel_num);
		if(channel_validity == 0)
		{
			err = tcc_manager_tuner_select_channel(channel_num, 1, 1);
			if(err == 0)
			{
				MNG_DBG("In %s, frequency lock(%d)", __func__, channel_num);

				tcc_manager_demux_set_tunerinfo(channel_num, 0, search_info.uiCountryCode);

				if( tcc_manager_demux_scan(channel_num, search_info.uiCountryCode, ISDBT_SCANOPTION_AUTOSEARCH, &stSvcFound) == 0 )
				{
					if (stSvcFound.found_no > 0 && stSvcFound.pServiceFound != NULL)
					{
						channel = channel_num;
						found_cnt = tcc_manager_tuner_autosearch_get_rowid (&stSvcFound, iArraySvcRowID, 32);
						if (found_cnt == 0)
							continue;

						break;
					}
				}
			}
		}
		else
		{
			ALOGD("In %s, invalid channel(%d)", __func__, channel_num);
		}

		if( (options == 0 && (unsigned int)channel_num <= search_info.uiMinChannel) ||
			(options == 1 && (unsigned int)channel_num >= search_info.uiMaxChannel) )
		{
			break;
		}

		try_cnt++;
		percent = (try_cnt * 100) / searchable_ch_cnt;

		if(channel_validity==0)
		{
			iAutoSearchEvt[0] = 0;
			iAutoSearchEvt[1] = percent;
			iAutoSearchEvt[2] = channel_num;
			TCCDxBEvent_AutoSearchProgress(iAutoSearchEvt);
		}
	}

AUTOSEARCH_END:
	if(channel == -1)    //if fail to search
	{
		iArraySvcRowID[0] = -1;
	}
	else
	{
		iArraySvcRowID[0] = 1;
	}
	
	if(stSvcFound.found_no >0 && stSvcFound.pServiceFound != NULL)
		tcc_mw_free(__FUNCTION__, __LINE__, stSvcFound.pServiceFound);

	TCCDxBEvent_AutoSearchDone (iArraySvcRowID);

	return 0;
}

int tcc_manager_tuner_customscan_checkList(int *plist)
{
	int cnt=0;
	if (plist==NULL) cnt=0;
	else {
		for (cnt=0; cnt < 128; cnt++)
		{
			if (*plist == -1) break;
			else plist++;
		}
	}
	return cnt;
}

int tcc_manager_tuner_customscan_makeresult(int *ptr, int status, int ch, CUSTOMSEARCH_FOUND_INFO_T *pSvcInfo)
{
	int *pRtnData = ptr;
	int cnt;
	if (ptr==NULL) return 0;

	*pRtnData++ = status;
	*pRtnData++ = ch;
	*pRtnData++ = 0; //strength
	*pRtnData++ = 0; //tsid
	if (pSvcInfo) {
		*pRtnData++ = pSvcInfo->fullseg_id;
		*pRtnData++ = pSvcInfo->oneseg_id;
		*pRtnData++ = pSvcInfo->total_svc;
		for (cnt=0; cnt < pSvcInfo->total_svc; cnt++)
			*pRtnData++ = pSvcInfo->all_id[cnt];
	}
	return 1;
}
void tcc_manager_tuner_customscan_addresult_signalstrength (int *ptr, int signal_strength)
{
	CUSTOMSEARCH_FOUND_INFO_T *pRtnData = (CUSTOMSEARCH_FOUND_INFO_T*)ptr;
	if (pRtnData != NULL) {
		pRtnData->strength = signal_strength;
	}
}
void tcc_manager_tuner_customscan_addresult_tsid(int *ptr, int tsid)
{
	CUSTOMSEARCH_FOUND_INFO_T *pRtnData = (CUSTOMSEARCH_FOUND_INFO_T*)ptr;
	if (pRtnData != NULL) {
		pRtnData->tsid = tsid;
	}
}
int tcc_manager_tuner_customscan(int country_code, int region_code, int channel_num, int options)
{
	int search_kind = country_code;		/* 1=relay station, 2=affiliation station */
	int *pListCh = (int *)region_code;
	int *pListTSID = (int *)channel_num;
	int search_repeat = options;
	int err=-2;	/* -2=invalid parameter, -1=failure, 0=success, 1=cancel */
	int dmx_scan_rtn;
	int	cnt_ch, cnt;
	int fEndlessSearch = 0,fSearch = 0;
	int *p_ch, *p_tsid, ch, tsid, uhf_ch;
	CUSTOMSEARCH_FOUND_INFO_T stCustomSearchFoundInfo;

	TUNER_SEARCH_INFO search_info;

	p_ch = pListCh;
	if (p_ch != NULL) uhf_ch = *p_ch;	/* ch number for error report */
	else uhf_ch = 0;

	if (giISTSFileInput) {
		err = -1;
		goto CUSTOMSEARCH_END;
	}

	/* check parameters */
	if (!(search_kind==1 || search_kind==2)) {
		ALOGD("[tcc_manager_tuner_customscan] invalid type=%d", search_kind);
		goto CUSTOMSEARCH_END;
	}
	if (pListCh == NULL || pListTSID == NULL) {
		ALOGD("[tcc_manager_tuner_customscan] invalid list");
		goto CUSTOMSEARCH_END;
	}
	cnt_ch = tcc_manager_tuner_customscan_checkList(pListCh); //count of channels
	cnt = tcc_manager_tuner_customscan_checkList(pListTSID); //count of network_id
	if (cnt_ch==0 || cnt==0) {
		ALOGD("[tcc_manager_tuner_customscan] invalid data in ch(%d) or tsid(%d)", cnt_ch, cnt);
		goto CUSTOMSEARCH_END;
	}
	if (search_repeat < 0) {
		ALOGD("[tcc_manager_tuner_customscan] invalid search count(%d)", search_repeat);
		goto CUSTOMSEARCH_END;
	}

	/* search */
	if (search_repeat==0) {
		fEndlessSearch = 1;
		search_repeat = 1;
	}
	tcc_tuner_set_countrycode(0);	//only for Japan
	err = TCC_DxB_TUNER_GetChannelInfo(hInterface, 0, &search_info);
	if (err != 0) {
		err = -2;
		goto CUSTOMSEARCH_END;
	}

	fSearch = 1;
	while (fSearch > 0)
	{
		p_ch = pListCh;
		p_tsid = pListTSID;
		for (cnt=0; cnt < cnt_ch; cnt++)
		{
			/* check search-cancel */
			if (giTunerScanStop) break;

			/* do search */
			err = -1;
			ch = *p_ch;		/* 0 means UHF13 */
			uhf_ch = *p_ch;	/* 0 menas UHF13 */
			if (ch >= (int)search_info.uiMinChannel && ch <= (int)search_info.uiMaxChannel) {
				/*  issue an event indicating that trying to tune RF */
				memset(iArrayCustomSearchStatus, 0, sizeof(iArrayCustomSearchStatus));
				tcc_manager_tuner_customscan_makeresult(iArrayCustomSearchStatus, 1, uhf_ch, NULL);
				TCCDxBEvent_CustomSearchStatus (3, iArrayCustomSearchStatus); /* on-going, channel lock */
				err = tcc_manager_tuner_select_channel(ch, 1, 1);
				if (err == 0) {
					MNG_DBG("[CustomSearch]Frequency found at UHF[%d]", uhf_ch);
					tcc_manager_demux_set_tunerinfo(ch, 0, search_info.uiCountryCode);
					if (search_kind == 1)
						dmx_scan_rtn = tcc_manager_demux_scan_customize(ch, search_info.uiCountryCode, ISDBT_SCANOPTION_CUSTOM_RELAY, p_tsid, (void*)&stCustomSearchFoundInfo);
					else
						dmx_scan_rtn = tcc_manager_demux_scan_customize(ch, search_info.uiCountryCode, ISDBT_SCANOPTION_CUSTOM_AFFILIATION, p_tsid, (void*)&stCustomSearchFoundInfo);
					if (dmx_scan_rtn == 0) {
						MNG_DBG("[CustomSearch]OK found service");
						err = 0; fSearch = 0;
						break;
					} else if (dmx_scan_rtn ==1) {
						MNG_DBG("[CustomSearch]Scan canceled");
						err = 1; fSearch = 0;
						break;
					} else if (dmx_scan_rtn==-2){	/* founded service is not matched with relay station */
						MNG_DBG("[CustomSearch]tsid not matched");
						err = -1;
						memset(iArrayCustomSearchStatus, 0, sizeof(iArrayCustomSearchStatus));
						tcc_manager_tuner_customscan_makeresult(iArrayCustomSearchStatus, 4, uhf_ch, NULL);
						tcc_manager_tuner_customscan_addresult_tsid(iArrayCustomSearchStatus, stCustomSearchFoundInfo.tsid);
						tcc_manager_tuner_customscan_addresult_signalstrength(iArrayCustomSearchStatus, stCustomSearchFoundInfo.strength);
						TCCDxBEvent_CustomSearchStatus (3, iArrayCustomSearchStatus); /* on-going, tsid not matched */
					} else { /* no service is found */
						MNG_DBG("[CustomSearch]Fail to find service");
						err = -1;
						/* issue an event indicating no service */
						memset(iArrayCustomSearchStatus, 0, sizeof(iArrayCustomSearchStatus));
						tcc_manager_tuner_customscan_makeresult(iArrayCustomSearchStatus, 3, uhf_ch, NULL);
						tcc_manager_tuner_customscan_addresult_signalstrength(iArrayCustomSearchStatus, stCustomSearchFoundInfo.strength);
						TCCDxBEvent_CustomSearchStatus (3, iArrayCustomSearchStatus); /* on-going, skip for not service */
					}
				} else {
					err = -1;
					/* issue an event indicating that CH not found */
					memset(iArrayCustomSearchStatus, 0, sizeof(iArrayCustomSearchStatus));
					stCustomSearchFoundInfo.strength =tcc_manager_demux_scan_customize_read_strength(); /* stCustomSearchFoundInfo.strength in this line is used only to store the strength temporary */
					tcc_manager_tuner_customscan_makeresult(iArrayCustomSearchStatus, 2, uhf_ch, NULL);
					tcc_manager_tuner_customscan_addresult_signalstrength(iArrayCustomSearchStatus, stCustomSearchFoundInfo.strength);
					TCCDxBEvent_CustomSearchStatus (3, iArrayCustomSearchStatus); /* on-going, skip for not detect station */
				}
			}

			p_ch++;
		}
		/* check search count */
		if (fSearch) {
			if (!fEndlessSearch) {
				search_repeat--;
				if (search_repeat <= 0) {
					err = -1;	/* set search failure */
					fSearch = 0;
				}
			}
		}
		if (giTunerScanStop) {
			giTunerScanStop = 0;
			err = 1;
			fSearch = 0;
		}
	}
CUSTOMSEARCH_END:
	memset(iArrayCustomSearchStatus, 0, sizeof(iArrayCustomSearchStatus));
	if (err == 1) {
		/* issue an event indicating cancel */
		tcc_manager_tuner_customscan_makeresult(iArrayCustomSearchStatus, 2, uhf_ch, NULL);
		TCCDxBEvent_CustomSearchStatus (2, iArrayCustomSearchStatus); /* cancel */
	} else if (err == 0) {
		/* issue an event indicating success */
		tcc_manager_tuner_customscan_makeresult(iArrayCustomSearchStatus, 1, uhf_ch, &stCustomSearchFoundInfo);
		tcc_manager_tuner_customscan_addresult_tsid(iArrayCustomSearchStatus, stCustomSearchFoundInfo.tsid);
		tcc_manager_tuner_customscan_addresult_signalstrength(iArrayCustomSearchStatus, stCustomSearchFoundInfo.strength);
		TCCDxBEvent_CustomSearchStatus (1, iArrayCustomSearchStatus); /* success */
	} else {
		/* issue an event indicating failure */
		tcc_manager_tuner_customscan_makeresult(iArrayCustomSearchStatus, -1, uhf_ch, NULL);
		TCCDxBEvent_CustomSearchStatus (-1, iArrayCustomSearchStatus);
	}
	return err;
}

int tcc_manager_tuner_epgscan (int country_code, int networkID, int channel_num,  int options)
{
	MNG_DBG("In %s\n", __func__);
	int 	err = 0;

	if (giTunerScanStop) {
		giTunerScanStop = 0;
		return 1;
	}

	err = tcc_tuner_select_channel(channel_num, 1, 1);
	if (err == 0) {
		MNG_DBG("Frequency find !!!, Let's find EIT ...\n");
		tcc_manager_demux_set_tunerinfo(channel_num, 0, country_code);
		if (tcc_manager_demux_scan_epg(channel_num, networkID, options) == 0) {
			MNG_DBG("OK found EIT ...\n");
		}
	} else 
		err = -1;
	if (giTunerScanStop) {
		giTunerScanStop = 0;
		err = 1;
	}
	return err;
}


/*
  * return 0 = success
  *           1 = canceled
  *          -1 = failure
  */
int tcc_manager_tuner_scan(int scan_type, int country_code, int region_code, int channel_num, int options)
{
	int err=0, ch_count=0, *pCh = NULL;

	if(scan_type == INIT_SCAN)
	{
		tcc_manager_demux_delete_db();
		err= tcc_manager_tuner_init_scan(country_code);
	}
	else if(scan_type == RE_SCAN)
	{
		err= tcc_manager_tuner_re_scan(country_code);
	}
	else if(scan_type == REGION_SCAN)
	{
		tcc_manager_demux_delete_db();
		err= tcc_manager_tuner_region_scan(country_code, region_code);
	}
	else if((scan_type == MANUAL_SCAN) || (scan_type == INFORMAL_SCAN))
	{
		err= tcc_manager_tuner_scan_one_channel(scan_type, channel_num, country_code, region_code, options);
	}
	else if (scan_type == AUTOSEARCH)
	{
		err = tcc_manager_tuner_autosearch(country_code, channel_num, options);
	}
	else if (scan_type == CUSTOM_SCAN)
	{
		err = tcc_manager_tuner_customscan(country_code, region_code, channel_num, options);
	}
	else if (scan_type == EPG_SCAN)
	{
		err = tcc_manager_tuner_epgscan(country_code, region_code, channel_num, options);
	}
	else
	{
		ALOGE("Invalid scan type\n");
		err = -1;
	}

	/* in case of autosearch, custom search and epg search, informal search event will be issued in sub-function */
	if ((scan_type != AUTOSEARCH) && (scan_type != CUSTOM_SCAN) && (scan_type != EPG_SCAN) && (scan_type != INFORMAL_SCAN))
	{
		if (err != 0 && err != 1 && err != -1 && err != -2)
			err = 0;

		TCCDxBEvent_SearchingDone(&err);
	}

	return err;
}

/*
  * this function will be used only when channel information was built from informations of broadcasting station for each region without scan operation.
  */
int tcc_manager_tuner_scan_manual (int channel_num, int country_code, int done_flag)
{
	int	error = 0;
	if(giISTSFileInput)
	    return 0;

	if (done_flag) {
		tcc_manager_set_updatechannel_flag(0);
	}

	error = tcc_manager_tuner_scan_one_channel (MANUAL_SCAN, channel_num, country_code, 0, 0);
	if (done_flag) {
		if (tcc_manager_get_updatechannel_flag()) {
			tcc_manager_set_updatechannel_flag(0);
			TCCDxBEvent_ChannelUpdate(NULL);
		}
	}

	return error;
}

int tcc_manager_tuner_handover_load(int iChannelNumber, int iServiceID)
{
   	if(giISTSFileInput)
	    return -1;

	memset(&gstDescTDSD, 0, sizeof(T_DESC_TDSD));
	memset(&gstDescEBD, 0, sizeof(T_DESC_EBD));

	tcc_db_get_handover_info(iChannelNumber, iServiceID, &gstDescTDSD, &gstDescEBD);

	return 0;
}

int tcc_manager_tuner_handover_update(int iNetworkID)
{
	ISDBT_Get_NetworkIDInfo(&giNetworkID);

	if(giNetworkID == iNetworkID) {
	if(ISDBT_Get_DescriptorInfo(E_DESC_EXT_BROADCAST_INFO, (void*)&gstDescEBD) == 0) {
			ALOGD("[%s] EXT_BROADDCAST_INFO isn't updated!\n", __func__);
	}

	if(ISDBT_Get_DescriptorInfo(E_DESC_TERRESTRIAL_INFO, (void*)&gstDescTDSD) == 0)	{
			ALOGD("[%s] TERRESTRIAL_INFO isn't updated!\n", __func__);
		}
	} else {
		ALOGD("[%s] Handover use DB channel info(%d)!!!\n", __func__, iNetworkID);
		giNetworkID = iNetworkID;
	}	

	ALOGD("affiliation_id_num:%d, freq_ch_num:%d\n", gstDescEBD.affiliation_id_num, gstDescTDSD.freq_ch_num);

	return 0;
}

int tcc_manager_tuner_handover_clear(void)
{
/*
	giNetworkID = 0;
	memset(&gstDescEBD, 0, sizeof(T_DESC_EBD));
	memset(&gstDescTDSD, 0, sizeof(T_DESC_TDSD));
*/
	return 0;
}

int tcc_manager_tuner_handover_scan(int channel_num, int country_code, int search_affiliation, int *same_channel)
{
	int iNetworkID;
	T_DESC_EBD stDescEBD;
	T_DESC_TDSD stDescTDSD; 	
	TUNER_SEARCH_INFO search_info;
	int min_ch, max_ch=0, ch=0;
	int networkID_ch = -1;
	int affiliationID_ch = -1;
	int sqinfo[20] = {0, };
	int signal_strength_sum = 0;
	int signal_strength_avg = 0;
	int i, j;
	int err=0;

	*same_channel = 0;

	MNG_DBG("In %s\n", __func__);

	giTunerScanStop = 0;
	
	tcc_tuner_set_countrycode(country_code);

	if((gstDescTDSD.freq_ch_num > 0) && (search_affiliation == 0)) {
		min_ch = 0;
		max_ch = gstDescTDSD.freq_ch_num;
		if(max_ch >= MAX_FREQ_TBL_NUM) {
			max_ch = MAX_FREQ_TBL_NUM-1;
		}
		gstDescTDSD.freq_ch_table[max_ch] = channel_num;
	} else {
		err = TCC_DxB_TUNER_GetChannelInfo(hInterface, 0, &search_info);
		if(err != 0){
			ALOGE("error in %s \n", __func__);
			return err;
		}
		min_ch = search_info.uiMinChannel;
		max_ch = search_info.uiMaxChannel;
	}

	for (i = min_ch; i <= max_ch; i++) {
		if(giTunerScanStop) {
			giTunerScanStop = 0;
			break;
		}

		if(search_affiliation == 0) {
			ch = gstDescTDSD.freq_ch_table[i];
		} else {
			ch = i;
		}

//		TCCDxBEvent_HandoverChannel((void*)search_affiliation, ch);

		err = tcc_manager_tuner_select_channel(ch, 1, 1);
		if(err == 0) {
			MNG_DBG("[%s] find a channel(%d)!!!, Let's find NIT, BIT ...\n", __func__, ch);

			// check signal strength (250ms * 8turns = 2sec)
			signal_strength_sum = 0;
			signal_strength_avg = 0;
			for(j = 0; j < 8; j++)
			{
				if(giTunerScanStop) {
					giTunerScanStop = 0;
					return -1;
				}

				err = tcc_tuner_get_strength(sqinfo);
				if (err == 0) {
					signal_strength_sum += sqinfo[1];
				}

				usleep(250000);
			}
	
			signal_strength_avg = signal_strength_sum / 8;
			if(signal_strength_avg < 30) {
				MNG_DBG("[%s] signal is too weak(%d)!!!\n", __func__, signal_strength_avg);
				continue;
			}

			tcc_manager_demux_set_tunerinfo(ch, 0, country_code);
			tcc_manager_demux_set_state(E_DEMUX_STATE_HANDOVER);
			tcc_manager_demux_set_channel_service_info_clear();
			TSPARSE_ISDBT_SiMgmt_Clear();
			ISDBT_Init_DescriptorInfo();

			if(tcc_manager_demux_handover() == 0) {
				ISDBT_Get_NetworkIDInfo(&iNetworkID);
				if((channel_num == ch) && (giNetworkID == iNetworkID)) {
					MNG_DBG("[%s] detected a same channel(%d)...\n", __func__, ch);
					*same_channel = 1;
					return ch;
				} else if((networkID_ch == -1) && (giNetworkID == iNetworkID)) {
					// separate case in order to detect a difference between DPA and others
					if((gstDescEBD.affiliation_id_num > 0) && (ISDBT_Get_DescriptorInfo(E_DESC_EXT_BROADCAST_INFO, (void*)&stDescEBD) != 0)) {
						MNG_DBG("[%s] detected a same networkID channel what has affiliationID(%d)...\n", __func__, ch);
						networkID_ch = ch;
					} else if((gstDescEBD.affiliation_id_num == 0) && (ISDBT_Get_DescriptorInfo(E_DESC_EXT_BROADCAST_INFO, (void*)&stDescEBD) == 0)) {
						MNG_DBG("[%s] detected a same networkID channel what has not affiliationID(%d)...\n", __func__, ch);
					networkID_ch = ch;
					} else {
						MNG_DBG("[%s] Both of them have a same network ID but affiliation ID is different...\n", __func__);
					}
				}

				if(networkID_ch == -1 && affiliationID_ch == -1 && search_affiliation) {
					if((gstDescEBD.affiliation_id_num > 0) && (ISDBT_Get_DescriptorInfo(E_DESC_EXT_BROADCAST_INFO, (void*)&stDescEBD) != 0)) {
						if(gstDescEBD.affiliation_id_array[0] == stDescEBD.affiliation_id_array[0]) {
							MNG_DBG("[%s] detected a same affiliationID channnel(%d)...\n", __func__, ch);
							affiliationID_ch = ch;
						}
					}
				}
			}
		}
	}

	if(networkID_ch != -1 || affiliationID_ch != -1) {
		ch = (networkID_ch != -1) ? networkID_ch : affiliationID_ch;

		err = tcc_manager_tuner_select_channel(ch, 1, 1);
		if(err == 0) {
			MNG_DBG("[%s] find a channel(%d)!!!, Let's find NIT, BIT ...\n", __func__, ch);
			tcc_manager_demux_set_tunerinfo(ch, 0, country_code);
			tcc_manager_demux_set_state(E_DEMUX_STATE_HANDOVER);
			tcc_manager_demux_set_channel_service_info_clear();
			TSPARSE_ISDBT_SiMgmt_Clear();
			ISDBT_Init_DescriptorInfo();

			tcc_manager_demux_set_state(E_DEMUX_STATE_SCAN);
			if(tcc_manager_demux_scan(ch, country_code, ISDBT_SCANOPTION_HANDOVER, NULL) == 0) {
				MNG_DBG("[%s] success a scanning (%d)!!!\n", __func__, ch);
			}
			
			return ch;
		}
	}

	return -1;
}

int tcc_manager_tuner_set_channel(int ich)
{
	int error = 0;
	error = tcc_manager_tuner_select_channel(ich, 0, 0);
	return error;
}

int tcc_manager_tuner_get_strength(int *sqinfo)
{
	int error;
	error = tcc_tuner_get_strength(sqinfo);
	return error;
}

int tcc_manager_tuner_scan_cancel(void)
{
	giTunerScanStop = 1;
	return 0;
}

void tcc_manager_tuner_scan_cancel_notify (void)
{
	int err = 1;
	TCCDxBEvent_SearchingDone(&err);
}

int tcc_manager_tuner_get_ews_flag(void *pStartFlag)
{
	int error;
	error = tcc_tuner_get_ews_flag(pStartFlag);
	return error;
}

int tcc_manager_tuner_cas_open(unsigned char _casRound, unsigned char * _systemKey)
{
	int error;
	if(giISTSFileInput) return 0;
	error = TCC_DxB_TUNER_CasOpen(hInterface, _casRound, _systemKey);
	return error;
}

int tcc_manager_tuner_cas_key_multi2(unsigned char _parity, unsigned char *_key, unsigned char _keyLength, unsigned char *_initVector,unsigned char _initVectorLength)
{
	int error;
	if(giISTSFileInput) return 0;
	error = TCC_DxB_TUNER_CasKeyMulti2(hInterface, _parity, _key, _keyLength, _initVector, _initVectorLength);
	return error;
}

int tcc_manager_tuner_cas_set_pid(unsigned int *_pids, unsigned int _numberOfPids)
{
	int error;
	if(giISTSFileInput) return 0;
	error = TCC_DxB_TUNER_CasSetPID(hInterface, _pids, _numberOfPids);
	return error;
}

int tcc_manager_tuner_set_FreqBand (int freq_band)
{
	int error;
	if(giISTSFileInput) return 0;
	error = TCC_DxB_TUNER_SetFreqBand (hInterface, 0, freq_band);
	return error;
}

int tcc_manager_tuner_reset(int countrycode, int ich)
{
	unsigned int iARGS[2];
	int err;
	MNG_DBG("In %s\n", __func__);
	if(giISTSFileInput)
	    return 0;

	err = TCC_DxB_TUNER_Close(hInterface, 0);
	if(err != 0) {
		ALOGE("[%s] Error TCC_DxB_TUNER_Close(%d)\n", __func__, err);
		return err;
 	}

	iARGS[0] = (unsigned int)countrycode;
	iARGS[1] = 1; // 1 means ISDBT of TCC351X
	err = TCC_DxB_TUNER_Open(hInterface, 0, iARGS);
	if(err != 0) {
		ALOGE("[%s] Error TCC_DxB_TUNER_Open(%d)\n", __func__, err);
		return err;
	}

	err = TCC_DxB_TUNER_SetChannel(hInterface, 0, ich, 0);
	if(err != 0) {
		ALOGE("[%s] Error TCC_DxB_TUNER_SetChannel(%d)\n", __func__, err);
	}

	return err;
}

int tcc_manager_tuner_set_CustomTuner(int size, void *arg)
{
	return TCC_DxB_TUNER_SetCustomTuner(hInterface, 0, size, arg);
}

int tcc_manager_tuner_get_CustomTuner(int *size, void *arg)
{
	return TCC_DxB_TUNER_GetCustomTuner(hInterface, 0, size, arg);
}

int tcc_manager_tuner_UserLoopStopCmd(int moduleIndex)
{
	if(giISTSFileInput) return 0;
	return TCC_DxB_TUNER_UserLoopStopCmd(hInterface, moduleIndex);
}

int tcc_manager_tuner_get_lastchannel(void)
{
	return giTunerLastChannel;
}
