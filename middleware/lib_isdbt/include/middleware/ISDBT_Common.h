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
#ifndef __ISDBT_COMMON_H__
#define __ISDBT_COMMON_H__

typedef enum
{
	E_DESC_DIGITAL_COPY_CONTROL,
	E_DESC_CONTENT_AVAILABILITY,
	E_DESC_SERVICE_LIST,
	E_DESC_DTCP,
	E_DESC_PARENT_RATING,
	E_DESC_PMT_CA,
	E_DESC_CAT_CA,
	E_DESC_CAT_SERVICE,
	E_DESC_PMT_AC,
	E_DESC_CAT_AC,
	E_DESC_EMERGENCY_INFO,
	E_DESC_TERRESTRIAL_INFO,
	E_DESC_EXT_BROADCAST_INFO,
	E_DESC_LOGO_TRANSMISSION_INFO,
	E_DESC_ARIB_BXML_INFO,

	E_DESC_MAX
} E_DESC_TYPE;

/* -------------------------------------------------------
**  Copy Digital Control Descriptor - 0xC1
** ------------------------------------------------------- */
typedef struct SUB_T_DESC_DCC_s
{
	struct SUB_T_DESC_DCC_s* pNext;

	unsigned char	component_tag;
	unsigned char	digital_recording_control_data;
	unsigned char	maximum_bitrate_flag;
	unsigned char	copy_control_type;
	unsigned char	APS_control_data;
	unsigned char	maximum_bitrate;
} SUB_T_DESC_DCC;

typedef struct
{
	unsigned char	ucTableID;

	unsigned char	digital_recording_control_data;
	unsigned char	maximum_bitrate_flag;
	unsigned char	component_control_flag;
	unsigned char	copy_control_type;
	unsigned char	APS_control_data;
	unsigned char	maximum_bitrate;

	SUB_T_DESC_DCC* sub_info;
} T_DESC_DCC;


/* -------------------------------------------------------
**  Content Availability - 0xDE
** ------------------------------------------------------- */
typedef struct
{
	unsigned char	ucTableID;

	unsigned char	copy_restriction_mode;
	unsigned char	image_constraint_token;
	unsigned char	retention_mode;
	unsigned char	retention_state;
	unsigned char	encryption_mode;
} T_DESC_CONTA;


/* -------------------------------------------------------
**  Service List Descriptor - 0x41
** ------------------------------------------------------- */
typedef struct
{
	unsigned short	service_id;
	unsigned char	service_type;
	unsigned char	uc_PADDING_1; // For align padding (unsigned char)
} T_SERVICE_LIST;

typedef struct
{
	unsigned char	ucTableID;

	unsigned char	service_list_count;
	unsigned char	us_PADDING_1[2]; // For align padding (unsigned short * 2)

	T_SERVICE_LIST	*pstSvcList;
} T_DESC_SERVICE_LIST;


/* -------------------------------------------------------
**  DTCP(Digital Transmission Content Protection) Descriptor - 0x88
** ------------------------------------------------------- */
typedef struct
{
	unsigned char	ucTableID;

	unsigned short	CA_System_ID;
	unsigned char	Retention_Move_mode;
	unsigned char	Retention_State;
	unsigned char	EPN;
	unsigned char	DTCP_CCI;
	unsigned char	Image_Constraint_Token;
	unsigned char	APS;
} T_DESC_DTCP;

/* -------------------------------------------------------
**  PR(Parental Rating ) Descriptor - 0x55
** ------------------------------------------------------- */
typedef struct
{
	unsigned char	ucTableID;
	unsigned char	Rating;
} T_DESC_PR;

/* -------------------------------------------------------
**  CA Descriptor - 0x09
** ------------------------------------------------------- */
typedef struct
{
	unsigned char		ucTableID;
	unsigned short	uiCASystemID;
	unsigned short	uiCA_PID;
	unsigned char		ucEMM_TransmissionFormat;
} T_DESC_CA;

/* -------------------------------------------------------
**  CA Service Descriptor - 0xCC
** ------------------------------------------------------- */
typedef struct
{
	unsigned char		ucTableID;
	unsigned char		ucCA_Service_Tag;
	unsigned char		ucCA_Service_DescLen;
	unsigned short	uiCA_Service_SystemID;
	unsigned char		ucCA_Service_Broadcast_GroutID;
	unsigned char		ucCA_Service_MessageCtrl;
	unsigned short	uiCA_SerivceID;
} T_DESC_CA_SERVICE;

/* -------------------------------------------------------
**  Access control Descriptor - 0xf6
** ------------------------------------------------------- */
typedef struct
{
	unsigned char	ucTableID;
	unsigned short	uiCASystemID;
	unsigned short	uiTransmissionType;
	unsigned short	uiPID;
} T_DESC_AC;

/* -------------------------------------------------------
**  Emergency Information Descriptor - 0xfc
** ------------------------------------------------------- */
typedef struct _t_desc_eid{
	struct _t_desc_eid *pNext;
	int service_id;
	int start_end_flag;
	int signal_type;
	int area_code_length;
	int area_code[256];
}T_DESC_EID;

typedef struct {
	T_DESC_EID	*pEID;
}T_EID_TYPE;

/* -------------------------------------------------------
**  Terrestrial Delivery System Descriptor - 0xfa
** ------------------------------------------------------- */
#ifndef MAX_FREQ_TBL_NUM
#define MAX_FREQ_TBL_NUM 58	/* Japan:13~56, Brazil:14~69 */
#endif

typedef struct{
	unsigned short	area_code;
	unsigned char	guard_interval;
	unsigned char	transmission_mode;
	unsigned char	freq_ch_num;
	unsigned char	freq_ch_table[MAX_FREQ_TBL_NUM];
}T_DESC_TDSD;

/* -------------------------------------------------------
**  Extended Broadcaster Descriptor - 0xce
** ------------------------------------------------------- */
#ifndef AFFILIATION_ID_MAX 
#define AFFILIATION_ID_MAX 255
#endif

typedef struct{
	unsigned char	broadcaster_type;
	
	unsigned short	terrestrial_broadcaster_id;
	unsigned char	affiliation_id_num;
	unsigned char	affiliation_id_array[AFFILIATION_ID_MAX+1];

	unsigned char	broadcaster_id_num;
	unsigned short	original_network_id[AFFILIATION_ID_MAX+1];
	unsigned char	broadcaster_id[AFFILIATION_ID_MAX+1];
}T_DESC_EBD;

/*-------------------------------------------------------
  * Logo Transmission Descriptor - 0xcf
  *------------------------------------------------------*/
typedef struct {
	unsigned char		ucTableID;
	unsigned char		logo_transmission_type;
	unsigned short	logo_id;
	unsigned short	logo_version;
	unsigned short	download_data_id;
	unsigned char	*logo_char;
} T_DESC_LTD;

/*-------------------------------------------------------
  * Component Group Descriptor - 0xd9
  *------------------------------------------------------*/
#define MAX_SUPPORT_COMPONENT_GROUP						3
#define MAX_SUPPORT_COMPONENT_GROUP_CA_UNIT				15
#define MAX_SUPPORT_COMPONENT_GROUP_CA_UNIT_COMPONENT	15
#define MAX_SUPPORT_COMPONENT_GROUP_TEXT_CHAR			16
typedef struct {
	unsigned char	component_group_type;
	unsigned char	total_bit_rate_flag;
	unsigned char	num_of_group;
	unsigned char	component_group_id[MAX_SUPPORT_COMPONENT_GROUP];
	unsigned char	num_of_CA_unit[MAX_SUPPORT_COMPONENT_GROUP];
	unsigned char	CA_unit_id[MAX_SUPPORT_COMPONENT_GROUP][MAX_SUPPORT_COMPONENT_GROUP_CA_UNIT];
	unsigned char	num_of_component[MAX_SUPPORT_COMPONENT_GROUP][MAX_SUPPORT_COMPONENT_GROUP_CA_UNIT];
	unsigned char	component_tag[MAX_SUPPORT_COMPONENT_GROUP][MAX_SUPPORT_COMPONENT_GROUP_CA_UNIT][MAX_SUPPORT_COMPONENT_GROUP_CA_UNIT_COMPONENT];
	unsigned char	total_bit_rate[MAX_SUPPORT_COMPONENT_GROUP];
	unsigned char	text_length[MAX_SUPPORT_COMPONENT_GROUP];
	unsigned char	text_char[MAX_SUPPORT_COMPONENT_GROUP][MAX_SUPPORT_COMPONENT_GROUP_TEXT_CHAR+2];
} T_DESC_CGD;

/*-------------------------------------------------------
  * Data Content Descriptor - 0xc7
  *------------------------------------------------------*/
typedef struct {
	unsigned char	ucTableID;
	unsigned char	content_id_flag;
	unsigned char	associated_content_flag;
	unsigned int	content_id;
} T_DESC_BXML;


typedef int (*p_ews_notification)(int service_id, int start_end_flag, int signal_type, int area_code);

extern int ISDBT_Get_DescriptorInfo(unsigned char ucDescType, void *pDescData);
extern int ISDBT_Get_PIDInfo(int *piTotalAudioPID, int piAudioPID[], int piAudioStreamType[], int *piTotalVideoPID, int piVideoPID[], int piVideoStreamType[], int *piStPID, int *piSiPID);
extern int ISDBT_Get_NetworkIDInfo(int *piNetworkID);
extern void ISDBT_Clear_NetworkIDInfo(void);

#endif // __ISDBT_COMMON_H__

