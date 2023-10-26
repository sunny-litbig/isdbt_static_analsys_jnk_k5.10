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
#ifndef	_TCC_ISDBT_MANAGER_VIDEO_H_
#define	_TCC_ISDBT_MANAGER_VIDEO_H_

#ifdef __cplusplus
extern "C" {
#endif

int tcc_manager_video_init(void);
int tcc_manager_video_deinit(void);
int tcc_manager_video_start(unsigned int uidevid, unsigned int ulVideoFormat);
int tcc_manager_video_stop(unsigned int uidevid);
int tcc_manager_video_pause(unsigned int uidevid, unsigned int uiOnOff);
int tcc_manager_video_initSurface(int arg);
int tcc_manager_video_deinitSurface();
int tcc_manager_video_setSurface(void *nativeWidow);
int tcc_manager_video_useSurface(void);
int tcc_manager_video_releaseSurface(void);
//int tcc_manager_video_capture(unsigned int uidevid,unsigned char *pucFileName);
int tcc_manager_video_select_display_output(unsigned int uidevid);
int tcc_manager_video_issupport_country(unsigned int uidevid, unsigned int uiCountry);
//int tcc_manager_video_enable_display(unsigned int uidevid, unsigned int uiOnOff);
//int tcc_manager_video_refreshDisplay(unsigned int uidevid);
int tcc_manager_video_set_proprietarydata (unsigned int channel_index);
extern int tcc_manager_video_serviceID_disable_display(int check_flag);
int tcc_manager_video_setfirstframebypass(unsigned int uiOnOff);
int tcc_manager_video_setfirstframeafterseek(unsigned int uiOnOff);
int tcc_manager_video_setframedropflag(unsigned int check_flag);
int tcc_manager_video_getdisplayedfirstframe(void);
#ifdef __cplusplus
}
#endif

#endif
