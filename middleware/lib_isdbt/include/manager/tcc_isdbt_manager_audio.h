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
#ifndef	_TCC_ISDBT_MANAGER_AUDIO_H_
#define	_TCC_ISDBT_MANAGER_AUDIO_H_

#ifdef __cplusplus
extern "C" {
#endif

int tcc_manager_audio_init(void);
int tcc_manager_audio_deinit(void);
int tcc_manager_audio_start(unsigned int uidevid, unsigned int ulAudioFormat);
int tcc_manager_audio_stop(unsigned int uidevid);
int tcc_manager_audio_alsa_close(unsigned int uidevid, unsigned int uiStop);
int tcc_manager_audio_alsa_close_flag(unsigned int uiClose);
//int tcc_manager_audio_stereo(unsigned int uidevid, unsigned int ulMode);
//int tcc_manager_audio_volume(unsigned int uidevid, unsigned int ulVolume);
int tcc_manager_audio_mute(unsigned int uidevid, unsigned int bOnOff);
int tcc_manager_audio_select_output(unsigned int uidevid, unsigned int isEnableAudioOutput);
int tcc_manager_audio_set_dualmono (unsigned int uidevid, unsigned int audio_mode);
int tcc_manager_audio_issupport_country(unsigned int uidevid, unsigned int uiCountry);
int tcc_manager_audio_get_audiotype(int devid, int *piNumCh, int *piAudioMode);
extern int tcc_manager_audio_serviceID_disable_output(int check_flag);
int tcc_manager_audio_set_AudioStartSyncWithVideo(unsigned int uiOnOff);
int tcc_manager_audio_setframedropflag(unsigned int check_flag);
int tcc_manager_audio_setSeamlessSwitchCompensation(int iOnOff, int iInterval, int iStrength, int iNtimes, int iRange, int iGapadjust, int iGapadjust2, int iMuliplier);
int tcc_manager_audio_set_proprietarydata (unsigned int channel_index, unsigned int service_id, unsigned int sub_service_id, int dual_mode, int supportPrimary);
int tcc_manager_audio_get_SeamlessValue(int *state, int *cval, int *pval);

#ifdef __cplusplus
}
#endif
#endif


