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
#ifndef	_TCC_ISDBT_MANAGER_TUNER_H_
#define	_TCC_ISDBT_MANAGER_TUNER_H_

#ifdef __cplusplus
extern "C" {
#endif

int tcc_manager_tuner_init(int uiBBSelect);
int tcc_manager_tuner_deinit(void);
int tcc_manager_tuner_open(int icountrycode);
int tcc_manager_tuner_close(void);
int tcc_manager_tuner_is_suspending(void);
int tcc_manager_tuner_scanflag_init(void);
int tcc_manager_tuner_scanflag_get (void);
int tcc_manager_tuner_informal_scan(int country_code, int channel_num, int tsid, int options);
int tcc_manager_tuner_scan(int scan_type, int country_code, int region_code, int channel_num, int options);
int tcc_manager_tuner_scan_manual (int channel_num, int country_code, int done_flag);
void tcc_manager_tuner_scan_cancel_notify (void);
int tcc_manager_tuner_set_channel(int ich);
int tcc_manager_tuner_get_strength(int *sqinfo);
int tcc_manager_tuner_scan_cancel(void);
int tcc_manager_tuner_register_pid(int pid);
int tcc_manager_tuner_unregister_pid(int pid);
int tcc_manager_tuner_set_area (unsigned int uiArea);
int tcc_manager_tuner_get_ews_flag(void *pStartFlag);
int tcc_manager_tuner_handover_load(int iChannelNumber, int iServiceID);
int tcc_manager_tuner_handover_update(int iNetworkID);
int tcc_manager_tuner_handover_clear(void);
int tcc_manager_tuner_handover_scan(int channel_num, int country_code, int search_affiliation, int *same_channel);
int tcc_manager_tuner_cas_open(unsigned char _casRound, unsigned char * _systemKey);
int tcc_manager_tuner_cas_key_multi2(unsigned char _parity, unsigned char *_key, unsigned char _keyLength, unsigned char *_initVector,unsigned char _initVectorLength);
int tcc_manager_tuner_cas_set_pid(unsigned int *_pids, unsigned int _numberOfPids);
int tcc_manager_tuner_set_FreqBand(int freq_band);
int tcc_manager_tuner_reset(int countrycode, int ich);
int tcc_manager_tuner_set_CustomTuner(int size, void *arg);
int tcc_manager_tuner_get_CustomTuner(int *size, void *arg);
int tcc_manager_tuner_UserLoopStopCmd(int moduleIndex);
int tcc_manager_tuner_get_lastchannel(void);

#ifdef __cplusplus
}
#endif

#endif

