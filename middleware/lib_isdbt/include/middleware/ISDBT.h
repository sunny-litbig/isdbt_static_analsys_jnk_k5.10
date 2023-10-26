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
#ifndef __ISDBT_H__
#define __ISDBT_H__

typedef unsigned int I32U;
typedef int			 I32S;
typedef unsigned char I08U;
typedef signed char   I08S;
typedef I32S		  STATUS;

#ifndef FALSE	
	#define FALSE	(0) 
#endif

#ifndef TRUE		
	#define TRUE    (1)
#endif

/* ==========================================================================
**  CONDITIONAL COMPILATION 
** ========================================================================== */
/* Extend the access point to search a service ID. (not recommended) */
//#define ISDBT_DETECT_EXT_SERVICE_ID

/* ==========================================================================
**  MACRO CONSTANT
** ========================================================================== */

/* [DMP:0190] */
#define ISDBT_AACP_GET_ERROR_NONE                (0)
#define ISDBT_AACP_GET_ERROR_NO_PES              (-1)
#define ISDBT_AACP_GET_ERROR_HEADER              (-2)
#define ISDBT_AACP_GET_ERROR_RAWDATA_SIZE_OVER   (-3)

/* General Processing Return Value */
#define ISDBT_SUCCESS	                (1)
#define ISDBT_ERROR			(0)

/* Auto-scan processing Return Value */
#define ISDBT_FREQ_LOCK_SUCCESS       (100)
#define ISDBT_FREQ_LOCK_FAIL          (0)

/*Max BER Value to allow*/
#define ISDBT_MAX_BER_LIMIT_ErrMsg    (5000)
#define ISDBT_MAX_BER_LIMIT_Scan      (10000)

/*Min CNR Value to allow*/
#define ISDBT_MIN_CNR_LIMIT_ErrMsg    (6)
#define ISDBT_MIN_CNR_LIMIT_Scan      (6)

/*Min CNR Value to allow for TCC351X*/
#define ISDBT_MIN_CNR_LIMIT_ErrMsg_TCC35    (ISDBT_MIN_CNR_LIMIT_ErrMsg-2)
#define ISDBT_MIN_CNR_LIMIT_Scan_TCC35      (ISDBT_MIN_CNR_LIMIT_Scan-2)

/*Error Packet Count*/
#define ISDBT_MIN_ERR_PKT_CNT         (5)
#define ISDBT_MAX_ERR_PKT_CNT         (50)

/* Select checking factor(s) for the signal monitoring */
//#define ISDBT_MONITORING_CHECK_BER			
#define ISDBT_MONITORING_CHECK_CNR			
#define ISDBT_MONITORING_CHECK_TS

/* Airplay Retry Count */
#define ISDBT_MAX_AIRPLAY_RETRY_CNT   (3)

/* Max. num. of DMA buffer */
#define ISDBT_DMA_MAX_BUF_NUM         (1)

/* Max. len. of password for parental rating */
#define ISDBT_PASSWD_MAX_LEN          (4)

/* Max, value of device-user's age */
#define ISDBT_PARENT_RATE_MAXAGE      (110)

#define ISDBT_PARENT_RATE_MAX_CONTENT (7)

/* Timeout count(unit: sec) to determine area-change on a same channel */
#define	ISDBT_AREA_CHANGE_TIMEOUT     (5)

#ifndef DxB_SBTVD_INCLUDE
#define ISDBT_MAX_REMOTE_CONTROL_KEY  (12)
#else
#define ISDBT_MAX_REMOTE_CONTROL_KEY  (99)
#endif

/* Total number of ISDB-T 1-Seg channel frequency is 50. */
#define ISDBT_FREQ_NUM		          (E_ISDBT_FREQ_MAX)
#define ISDBT_MAX_CHDB_IDX		      (E_ISDBT_FREQ_MAX-1)
#define ISDBT_MAX_REMOCON_NUM 		  (24)

#define CHANNEL_NUM                   (0)
#define CH_FREQUENCY                  (1)
#define TRANPORT_STREAM_ID            (2)
#define REMOTE_CONTROL_KEY_ID         (3)
#define SERVICE_ID                    (4)
#define REGION_ID                     (5)
#define ALL_INFO                      (6)

/* ==========================================================================
**  ENUMERATION
** ========================================================================== */

enum
{
	PARENT_RATE_BLOCKED,
	PARENT_RATE_UNBLOCKED
};

/* --------------------------------------------------
**  Classification by age of appropriate parental rating
** -------------------------------------------------- */
typedef enum
{
	AGE_RATE_10,	// Block if the age will be less than 10 years old
	AGE_RATE_12,	// Block if the age will be less than 12 years old
	AGE_RATE_14,	// Block if the age will be less than 14 years old
	AGE_RATE_16,	// Block if the age will be less than 16 years old
	AGE_RATE_18,	// Block if the age will be less than 18 years old
	AGE_RATE_MAX
} E_ISDBT_VIEWER_RATE;

#define AGE_RATE_ER		AGE_RATE_MAX
#define AGE_RATE_L		AGE_RATE_MAX


/* --------------------------------------------------
**  Panasonic BB module Type
** -------------------------------------------------- */
typedef enum
{
	MT_DQPSK,
	MT_QPSK,
	MT_QUM16,
	MT_QUM64,
	MT_UnusedLayer,
	MT_Dummy
} ISDBT_ModulationType;

/* --------------------------------------------------
**  TS Table ID (Related to E_DESC_TYPE)
**   - Ref. MPEG_TABLE_IDS
** -------------------------------------------------- */
typedef enum
{
	E_TABLE_ID_NONE,
	E_TABLE_ID_PMT,
	E_TABLE_ID_NIT,
	E_TABLE_ID_SDT,
	E_TABLE_ID_BAT,
	E_TABLE_ID_EIT,
	E_TABLE_ID_BIT,
	E_TABLE_ID_CAT,

	E_TABLE_ID_MAX
} E_TABLE_ID_TYPE;


/* --------------------------------------------------
**  Frequency Number Enum
** -------------------------------------------------- */
#ifndef DxB_SBTVD_INCLUDE
typedef enum
{
	E_ISDBT_FREQ_13,
	E_ISDBT_FREQ_14,
	E_ISDBT_FREQ_15,
	E_ISDBT_FREQ_16,
	E_ISDBT_FREQ_17,
	E_ISDBT_FREQ_18,
	E_ISDBT_FREQ_19,
	E_ISDBT_FREQ_20,
	E_ISDBT_FREQ_21,
	E_ISDBT_FREQ_22,
	E_ISDBT_FREQ_23,
	E_ISDBT_FREQ_24,
	E_ISDBT_FREQ_25,
	E_ISDBT_FREQ_26,
	E_ISDBT_FREQ_27,
	E_ISDBT_FREQ_28,
	E_ISDBT_FREQ_29,
	E_ISDBT_FREQ_30,
	E_ISDBT_FREQ_31,
	E_ISDBT_FREQ_32,
	E_ISDBT_FREQ_33,
	E_ISDBT_FREQ_34,
	E_ISDBT_FREQ_35,
	E_ISDBT_FREQ_36,
	E_ISDBT_FREQ_37,
	E_ISDBT_FREQ_38,
	E_ISDBT_FREQ_39,
	E_ISDBT_FREQ_40,
	E_ISDBT_FREQ_41,
	E_ISDBT_FREQ_42,
	E_ISDBT_FREQ_43,
	E_ISDBT_FREQ_44,
	E_ISDBT_FREQ_45,
	E_ISDBT_FREQ_46,
	E_ISDBT_FREQ_47,
	E_ISDBT_FREQ_48,
	E_ISDBT_FREQ_49,
	E_ISDBT_FREQ_50,
	E_ISDBT_FREQ_51,
	E_ISDBT_FREQ_52,
	E_ISDBT_FREQ_53,
	E_ISDBT_FREQ_54,
	E_ISDBT_FREQ_55,
	E_ISDBT_FREQ_56,
	E_ISDBT_FREQ_57,
	E_ISDBT_FREQ_58,
	E_ISDBT_FREQ_59,
	E_ISDBT_FREQ_60,
	E_ISDBT_FREQ_61,
	E_ISDBT_FREQ_62,

	E_ISDBT_FREQ_MAX
} E_ISDBT_FREQ_TYPE;
#else
typedef enum
{
	E_ISDBT_FREQ_14,
	E_ISDBT_FREQ_15,
	E_ISDBT_FREQ_16,
	E_ISDBT_FREQ_17,
	E_ISDBT_FREQ_18,
	E_ISDBT_FREQ_19,
	E_ISDBT_FREQ_20,
	E_ISDBT_FREQ_21,
	E_ISDBT_FREQ_22,
	E_ISDBT_FREQ_23,
	E_ISDBT_FREQ_24,
	E_ISDBT_FREQ_25,
	E_ISDBT_FREQ_26,
	E_ISDBT_FREQ_27,
	E_ISDBT_FREQ_28,
	E_ISDBT_FREQ_29,
	E_ISDBT_FREQ_30,
	E_ISDBT_FREQ_31,
	E_ISDBT_FREQ_32,
	E_ISDBT_FREQ_33,
	E_ISDBT_FREQ_34,
	E_ISDBT_FREQ_35,
	E_ISDBT_FREQ_36,
	E_ISDBT_FREQ_37,
	E_ISDBT_FREQ_38,
	E_ISDBT_FREQ_39,
	E_ISDBT_FREQ_40,
	E_ISDBT_FREQ_41,
	E_ISDBT_FREQ_42,
	E_ISDBT_FREQ_43,
	E_ISDBT_FREQ_44,
	E_ISDBT_FREQ_45,
	E_ISDBT_FREQ_46,
	E_ISDBT_FREQ_47,
	E_ISDBT_FREQ_48,
	E_ISDBT_FREQ_49,
	E_ISDBT_FREQ_50,
	E_ISDBT_FREQ_51,
	E_ISDBT_FREQ_52,
	E_ISDBT_FREQ_53,
	E_ISDBT_FREQ_54,
	E_ISDBT_FREQ_55,
	E_ISDBT_FREQ_56,
	E_ISDBT_FREQ_57,
	E_ISDBT_FREQ_58,
	E_ISDBT_FREQ_59,
	E_ISDBT_FREQ_60,
	E_ISDBT_FREQ_61,
	E_ISDBT_FREQ_62,
	E_ISDBT_FREQ_63,
	E_ISDBT_FREQ_64,
	E_ISDBT_FREQ_65,
	E_ISDBT_FREQ_66,
	E_ISDBT_FREQ_67,
	E_ISDBT_FREQ_68,
	E_ISDBT_FREQ_69,
	
	E_ISDBT_FREQ_MAX
} E_ISDBT_FREQ_TYPE;
#endif

/* ==========================================================================
**  STRUCTURE
** ========================================================================== */
typedef struct
{
	unsigned char			AgeClass;
	E_ISDBT_VIEWER_RATE		Rate;
	unsigned char *		RateString;
} ISDBT_PR_AGE_CLASS;

typedef struct
{
	unsigned char			Desc;
	unsigned char *		DescString;
} ISDBT_PR_CONTENT_DESC;

typedef struct
{
	int 	addr;
	int 	data;
} TISDBTINIT, *PISDBTINIT;

typedef struct
{
	I08U cnst;			/* TMCC Layer Carrier Modulation */
						/* 0 : DQPSK, 1:PSK, 2:16QAM, 3:64QAM, 7:No Layer, others: Reserve */

	I08U rate;			/* TMCC LAyer Convolution Code Rate */
						/* 0: 1/2, 1:2/3, 3:5/6, 4:7/8, 7: No Layer, others:Reserve */

	I08U ileav;			/* TMCC Laayer Time Interleave */
						/* Mode 1 */
						/* 0:I=0, 1:I=4, 2:I=8, 3:I=16, 4:I=32, 7:No Layer, others:Reserve */
						/* Mode 2 */
						/* 0:I=0, 1:I=2, 2:I=4, 3:I=8, 4:I=16, 7:No Layer, others:Reserve */
						/* Mode 3 */
						/* 0:I=0, 1:I=1, 2:I=2, 3:I=4, 4:I=8, 7:No Layer, others:Reserve */
	unsigned char seg;	/* Number of the Segments using in Layer */

} TISDBTSEGMENTINFO, *PISDBTSEGMENTINFO;


typedef struct
{
	I08U sysid;			/* 0 : TV , 1 : Sound. other : reserved */

	I08U part;			/* TVMode : */
						/* 0 : No partial stream reception, 1 : Partial stream reception */
						/* Sound Mode */
						/* 0 : 1 Segment, 1 : 3 Segment */

	I08U phcomp;		/* Phase error correction value of connected transmission */
						/* 0: -PHI/8, 1:-2PHI/8, 2:-3PHI/8, 3:-4PHI/8 */
						/* 4: -5PHI/8, 5:-6PHI/8, 6:-7PHI/8, 7:No correction */

	I08U mmode;		/* FFT size & Guard length */
						/* 0: Automatic operation , 1:Manual */

	I08U ffsize;			/* The output of the moniter of FFT size */
						/* 0:2048(Mode1), 1:4096(Mode2), 2:8192(Mode3), 3:No Define */

	I08U gdleng;			/* the output of the moniter of Guard length */
						/* 0:1/32, 1:1/16, 2:1/8, 3:1/4 */
						
	TISDBTSEGMENTINFO a_seg;                    /* TMCC Layer A */
	TISDBTSEGMENTINFO b_seg;                    /* TMCC Layer B */
	TISDBTSEGMENTINFO c_seg;                    /* TMCC layer C */
}TISDBTCHANNELINFO, *PISDBTCHANNELINFO;

#ifdef NMI_BB_SUPPORT
typedef struct 
{
	I08U ch;
	I32U freq;
}TISDBTFREQ,*PISDBTFREQ;
#endif // NMI_BB_SUPPORT

#ifdef TCC351X_SUPPORT_ISDBT
typedef struct 
{
	I08U ch;
	I32U freq;
}TISDBTFREQ,*PISDBTFREQ;
#endif // TCC351X_SUPPORT_ISDBT

#ifdef SHARP_BB_SUPPORT
typedef struct 
{
	I08U ch;
	I32U freq;
	I08U N;
	I08U A;
	I08U V;
}TISDBTFREQ,*PISDBTFREQ;
#endif // SHARP_BB_SUPPORT

#ifdef PANASONIC_BB_SUPPORT
#if defined(ISDBT_PANASONIC_MODULE_VER_1_75G)
typedef struct 
{
	I08U 	ch;
	I32U 	freq;
	I08U 	reg_06;
	I08U 	reg_07;
} TISDBTFREQ, *PISDBTFREQ;

#elif defined(ISDBT_PANASONIC_MODULE_VER_2_5G)
typedef struct 
{
	I08U 	ch;
	I32U 	freq;
	I08U 	rh33;
	I08U 	rh34;
	I08U 	rh35;
	I08U 	rh36;
} TISDBTFREQ, *PISDBTFREQ;
#endif // ISDBT_PANASONIC_MODULE_VER_1_75G
#endif // PANASONIC_BB_SUPPORT

#ifdef FCI_BB_SUPPORT
typedef struct
{
	unsigned char	ch;			//ch. No.
	unsigned		freq;		//frequency. ex)479143
} TISDBTFREQ, *PISDBTFREQ;

typedef struct
{
	// TMCC Info
	unsigned int	fft;			// fft = 0: mode2, 1 : mode3
	unsigned int	gi;			// 0 : no output, 1 : 1/16, 2 : 1/8, 3: 1/4
	unsigned int	ews;		// 0 : OFF, 1: ON
	unsigned int	cd;
	unsigned int	mod_mode;	// 0 : DQPSK, 1 : QPSK, 2: 16QAM, 3 : 64QAN
	unsigned int	code_rate;	// 0 : 1/2, 1 : 2/3, 2: 3/4,  3 : 5/6, 4 : 7/8
	unsigned int	ti;			// mode2(0) : {0,2,4,8}, mode3(1): {0,1,2,4}
	unsigned int	seg_size;
	unsigned int	tv_system;	// 0 : ISDB-T, 1: ISDB-T(SB)
} TISDBTFCITMCCINFO, *PISDBTFCITMCCINFO;
#endif	//FCI_BB_SUPPORT

typedef struct tagTCC_H264DEC_OneFrameInfo 
{
		unsigned int frame_len;
		unsigned int ptr;
		unsigned int ipts; 
		unsigned int I_P_frame;
			
}TCC_H264DEC_OneFrameInfo;

typedef struct ISDBT_CHDB
{
	unsigned char ch;								//UHF ch num
	unsigned int freq;								//real frequency ex)473143
	unsigned int		uiBER_avg;					//BER value
	unsigned char 	chName[21];					//TS name
	unsigned short 	chNum;						// 3 digit ch num ex)681
	unsigned short 	usRemote_control_key_id;	//remote control key id
	unsigned short	usRegion_id;				//region id
	unsigned short  usTransport_stream_id;      // TS ID (unique value)
	unsigned short 	usRegion_broadcaster_id;

	unsigned short	usNumService_id;
	unsigned short 	usServiceIdList[8];

	unsigned short 	usService_id;
	unsigned short 	usExtService_id;
	
	unsigned short  usUsed;                     
	unsigned short	areaCode;

}ISDBT_CHDB;

/* ==========================================================================
**  GLOBAL FUNCTION
** ========================================================================== */

/****************************************************************************
*
*    int ISDBT_AACP_Get_Data_Init()
* 
*	 Input   :  None
*
*	 Output  :  None
* 
*	 Return  :  0
*
*	 Description : Initialze internal variables used for ISDBT_AACP_Get_Data() 
*                  (Audio PES Parser).
*                  This function must be called when Audio decoder open.
*
*****************************************************************************/
extern int ISDBT_AACP_Get_Data_Init(void);


/****************************************************************************
*
*    int ISDBT_AACP_Get_Data( unsigned char *pucAACRawDataPtr, 
*                             int *piAACRawDataLength, 
*                             int *piPTS,
*                             int *piSamplingFrequency, 
*                             int *piChannelNumber,
*                             int *piADTSHeaderSize, 
*                             unsigned char *pucADTSHeader )
* 
*	 Input   :   pucAACRawDataPtr    = buffer pointer of raw data buffer to be copied
*               *piAACRawDataLength  = max size of raw data buffer to be copied
*
*	 Output  :   pucAACRawDataPtr[]  = raw data will be copied to this buffer
*               *piAACRawDataLength  = size of rawdata copied into pucAACRawDataPtr
*               *piSamplingFrequency = Sampling Frequency[Hz] that extracted from ADTS Frame header
*                                      if 'sampling_fequency_index' value in ADTS Frame header is invalid, 
*                                      this variable will be returned with value as extracted field
*                                      (sampling_fequency_index, not in [Hz]).
*               *piChannelNumber     = Channel Config that extracted from ADTS Frame header
*               *piADTSHeaderSize    = size of ADTS Frame Header
*                pucADTSHeader       = ADTS Frame Header data
* 
*	 Return  :  ISDBT_AACP_GET_ERROR_NONE = no error. one raw frame is copied normally.
*               ISDBT_AACP_GET_ERROR_NO_PES = PES is not arrived
*               ISDBT_AACP_GET_ERROR_ADTS_HEADER 
*                                    = ADTS Frame has the field with invalid value.
*                                      (especially, unsupported sampling_fequency_index )
*               ISDBT_AACP_GET_ERROR_RAWDATA_SIZE_OVER 
*                                    = Raw data frame size > given buffer size(*piAACRawDataLength). 
*                                      In this case, raw data will not be copied, 
*                                      but other parameter will filled as extracted.
*
*	 Description : Get one Raw AAC Frame from TS_FIFO_BUFFER ( Audio PES Parser )
*
*****************************************************************************/
extern int ISDBT_AACP_Get_Data(unsigned char * pucAACRawDataPtr, 
                                      int *piAACRawDataLength, 
                                      int *piPTS,
                                      int *piSamplingFrequency, 
                                      int *piChannelNumber,
                                      int *piADTSHeaderSize, 
                                      unsigned char *pucADTSHeader);


/****************************************************************************
*
*	 Function of 
*          int ISDBT_H264_Get_Data_Init(void);*  
*
*	 Input   : NONE
*               
*	 Output  : NONE
* 
*	 Return  : NONE
*
*	 Description : Initialze internal variables for ISDBT_H264_Get_Data(). (Video PES Parser)
*                  This function must be called when Video decoder open.
*
*****************************************************************************/
extern int ISDBT_H264_Get_Data_Init(void);

/****************************************************************************
*
*	 Function of 
*      int ISDBT_H264_Get_Data( unsigned int pInputBuffer,
*                               int videopts,
*                               int nBitStreamSize, 
*                               int iNextPTS, 
*                               unsigned char fNextPTSValid )
*  
*	 Input   :  pInputBuffer   = pointer of PES Packet
*               videopts       = PTS of PES Packet
*               nBitStreamSize = Size of PES Packet
*               iNextPTS       = PTS of Next PES Packet
*               fNextPTSValid  = Validity flag of iNextPTS
*               
*	 Output  :  TCC_H264DEC_OneFrameInfo 
*                   frame_info[] = All Nal units Information
*               unsigned int 
*                   dec_data[20] = index of frame_info[] which is Usable NAL unit
*               unsigned int 
*                   dec_data_cnt = Count of Usable NAL unit
* 
*	 Return  : Total number of Usable NAL units in PES Paket.
*              ( SPS, PPS, I-Picture, P-Picture )
*
*	 Description : Gather informations of NAL units in PES Packet
*
*****************************************************************************/
extern int ISDBT_H264_Get_Data(unsigned int pInputBuffer,
                                     int videopts,
                                     int nBitStreamSize, 
                                     int iNextPTS, 
                                     unsigned char fNextPTSValid,
                                     int max_vframe_idx );


extern TCC_H264DEC_OneFrameInfo frame_info[];
extern unsigned int dec_data[];
extern unsigned int dec_data_cnt;


/****************************************************************************
*
*  FUNCTION NAME : 
*      void ISDBT_TES_StreamEncryption(unsigned char *pEncStreamData);
*  
*  DESCRIPTION :  This function handles the TES stream encryption.
*  INPUT:  pEncStreamData = Encryption Stream Data
*  
*  OUTPUT:	None
*  REMARK  :	TES Encryption APIs
*****************************************************************************/
extern void ISDBT_TES_StreamEncryption(unsigned char *pEncStreamData);

/****************************************************************************
*  FUNCTION NAME : 
*      unsigned int ISDBT_Get_Frequency(unsigned int freqIdx);
*  
*  DESCRIPTION : This function handles the TES stream decryption.
*  INPUT: pDecStreamData = Decryption Stream Data
*  
*  OUTPUT: None	
*  REMARK  :	TES Decription APIs
****************************************************************************/
extern void ISDBT_TES_StreamDecryption(unsigned char *pDecStreamData);

/****************************************************************************
*  
*  FUNCTION NAME
*      int ISDBT_Get_DescriptorInfo(unsigned char ucDescType, void *pDescData);
*  
*  DESCRIPTION 
*      Returns the descriptor data according to input descriptor type.
*  
*  INPUT
*      ucDescType = Descriptor Type.
*                   (Refer to E_DESC_TYPE.)
*      pDescData  = Pointer of Descriptor Structure.
*                   - E_DESC_DIGITAL_COPY_CONTROL -> T_DESC_DCC
*                   - E_DESC_CONTENT_AVAILABILITY -> T_DESC_CONTA
*                   - E_DESC_SERVICE_LIST -> T_DESC_SERVICE_LIST
*                     ** After you needs pstSvcList, MUST free its memory
*                        by calling TC_Deallocate_Memory().
*                   - E_DESC_DTCP -> T_DESC_DTCP
*                   - E_DESC_PARENT_RATING -> T_DESC_PR
*  
*  OUTPUT
*      int			= Return type.
*      pDescData	= Descriptor Data.
*  
*  REMARK
*      - If ucTableID is E_TABLE_ID_MAX, it is the garbage information.
*      - MUST free the memory of pstSvcList which is the member of T_DESC_SERVICE_LIST.
*
****************************************************************************/
extern int ISDBT_Get_DescriptorInfo(unsigned char ucDescType, void *pDescData);

/**************************************************************************
*  
*  FUNCTION NAME
*      int ISDBT_Get_PIDInfo(int *piTotalAudioPID, int piAudioPID[], int piAudioStreamType[], int *piTotalVideoPID, int piVideoPID[], int piVideoStreamType[], int *piStPID, int *piSiPID)
*  
*  DESCRIPTION 
*      This function gets the PID value of audio, video and subtitle.
*  
*  INPUT
*      piTotalAudioPID  = Pointer of count of audio PID.
*      piAudioPID  = Pointer array of audio PID.
*      piAudioStreamType  = Pointer array of audio stream type.
*      piTotalVideoPID  = Pointer of count of video PID.
*      piVideoPID  = Pointer array of video PID.
*      piVideoStreamType  = Pointer array of video stream type.
*      piStPID  = Pointer of subtitle PID.
*	piSiPID =   Pointer of superimpose PID.
*  
*  OUTPUT
*      int			= Return type.
*  
*  REMARK
*      None
*
**************************************************************************/
extern int ISDBT_Get_PIDInfo(int *piTotalAudioPID, int piAudioPID[], int piAudioStreamType[], int *piTotalVideoPID, int piVideoPID[], int piVideoStreamType[], int *piStPID, int *piSiPID);

/**************************************************************************
*  
*  FUNCTION NAME
*      void ISDBT_Reset_AC_DescriptorInfo(void);
*  
*  DESCRIPTION 
*      This function resets the access control descriptor inforamtion.
*  
*  INPUT
*      None
*  
*  OUTPUT
*      None
*  
*  REMARK
*      None
*
**************************************************************************/
extern void ISDBT_Reset_AC_DescriptorInfo(void);

/**************************************************************************
*  FUNCTION NAME : 
*      void ISDBT_IOInitialAtSystemPowerOn(void);
*  
*  DESCRIPTION : 
*  INPUT:
*			None
*  
*  OUTPUT:	void - Return Type
*  REMARK  :	
**************************************************************************/
extern void ISDBT_IOInitialAtSystemPowerOn(void);

/**************************************************************************
*  FUNCTION NAME : 
*      unsigned int ISDBT_Get_Frequency(unsigned int freqIdx);
*  
*  DESCRIPTION : return frequency.
*  INPUT: freqIdx = frequency index
*  
*  OUTPUT:	I32U - Return Type
*  REMARK  :	
**************************************************************************/
extern unsigned int ISDBT_Get_Frequency(unsigned int freqIdx);

/**************************************************************************
*  FUNCTION NAME : 
*      unsigned char ISDBT_Get_ChannelNumber(unsigned int freqIdx);
*  
*  DESCRIPTION : return channel number.
*  INPUT: freqIdx = frequency index
*  
*  OUTPUT:	I08U - Return Type
*  REMARK  :	
**************************************************************************/
extern unsigned char ISDBT_Get_ChannelNumber(unsigned int freqIdx);

/**************************************************************************
*  FUNCTION NAME : 
*      void ISDBT_ResetChInfo (void);
*  
*  DESCRIPTION : return frequency.
*  INPUT: freqIdx = frequency index
*  
*  OUTPUT:	I32U - Return Type
*  REMARK  :	
**************************************************************************/
extern void ISDBT_ResetChInfo (void);

/**************************************************************************
*  FUNCTION NAME : 
*      void ISDBT_Check_Service_List();
*  
*  DESCRIPTION : check same remote control key
*  INPUT:
*  
*  OUTPUT:	void - Return Type
*  REMARK  :	
**************************************************************************/
extern int ISDBT_Check_Service_List(void);

/**************************************************************************
*  FUNCTION NAME : 
*      void ISDBT_Arrange_Service_List();
*  
*  DESCRIPTION : get ch info
*  INPUT:
*  
*  OUTPUT:	void - Return Type
*  REMARK  :	
**************************************************************************/
extern void ISDBT_Arrange_Service_List(void); 

/**************************************************************************
*  FUNCTION NAME : 
*      unsigned char ISDBT_GetAirPCBER(void);
*  
*  DESCRIPTION : 
*  INPUT:
*			None
*  
*  OUTPUT:	char - Return Type
*  			= 
*  REMARK  :	
**************************************************************************/
extern unsigned char ISDBT_GetAirPCBER(void);

/**************************************************************************
*  FUNCTION NAME : 
*      unsigned char ISDBT_GetAirPCCNR(void);
*  
*  DESCRIPTION : 
*  INPUT:
*			None
*  
*  OUTPUT:	char - Return Type
*  			= 
*  REMARK  :	
**************************************************************************/
extern unsigned char ISDBT_GetAirPCCNR(void);

/**************************************************************************
*  FUNCTION NAME : 
*      unsigned int ISDBT_Acqisition_BER();
*  
*  DESCRIPTION : get BER for scanning
*  INPUT:
*  
*  OUTPUT:	void - Return Type
*  REMARK  :	
**************************************************************************/
extern unsigned int ISDBT_Acqisition_BER(int dec_ch_cnt);

/**************************************************************************
*  FUNCTION NAME : 
*      void ISDBT_DRV_BB_Init(void);
*  
*  DESCRIPTION : 
*  INPUT:
*			None
*  
*  OUTPUT:	void - Return Type
*  REMARK  :	
**************************************************************************/
extern void ISDBT_DRV_BB_Init(void);

/**************************************************************************
*  FUNCTION NAME : 
*      void ISDBT_GetSignalQuality( unsigned int *puiPCBER, unsigned int *puiCnr, unsigned char *pucSTSFLG );
*  
*  DESCRIPTION : 
*  INPUT:
*			pucSTSFLG	= 
*			puiCnr	= 
*			puiPCBER	= 
*  
*  OUTPUT:	void - Return Type
*  REMARK  :	
**************************************************************************/
extern void ISDBT_GetSignalQuality( unsigned int *puiPCBER, unsigned int *puiCnr, unsigned char *pucSTSFLG );

/**************************************************************************
*  FUNCTION NAME : 
*      void ISDBT_ApplicationStart(void);
*  
*  DESCRIPTION : 
*  INPUT:
*			None
*  
*  OUTPUT:	void - Return Type
*  REMARK  :	
**************************************************************************/
extern void ISDBT_ApplicationStart(void);

/**************************************************************************
*  FUNCTION NAME : 
*      void ISDBT_ApplicationFinish(void);
*  
*  DESCRIPTION : 
*  INPUT:
*			None
*  
*  OUTPUT:	void - Return Type
*  REMARK  :	
**************************************************************************/
extern void ISDBT_ApplicationFinish(void);

/**************************************************************************
*  FUNCTION NAME : 
*      int ISDBT_AirPlayStart( void);
*  
*  DESCRIPTION : 
*  INPUT:
*			None
*  
*  OUTPUT:	void - Return Type
*  REMARK  :	
**************************************************************************/
extern int ISDBT_AirPlayStart(void);

/**************************************************************************
*  FUNCTION NAME : 
*      void ISDBT_AirPlayStop(void);
*  
*  DESCRIPTION : 
*  INPUT:
*			SyncOption	= 
*  
*  OUTPUT:	void - Return Type
*  REMARK  :	
**************************************************************************/
extern void ISDBT_AirPlayStop(void);

/**************************************************************************
*  FUNCTION NAME : 
*      unsigned char ISDBT_AirPlay_Restart(void);
*  
*  DESCRIPTION : 
*  INPUT:
*			None
*  
*  OUTPUT:	unsigned char - Return Type
*			= ISDBT_SUCCESS : Succeed to restart airplay of current channel
*			= ISDBT_ERROR : Fail to restart
*  REMARK  :	
**************************************************************************/
extern unsigned char ISDBT_AirPlay_Restart(void);

/**************************************************************************
*  FUNCTION NAME : 
*      void ISDBT_ScanInit(void);
*  
*  DESCRIPTION : 
*  INPUT:
*			None
*  
*  OUTPUT:	void - Return Type
*  REMARK  :	
**************************************************************************/
extern void ISDBT_ScanInit(void);

/**************************************************************************
*  FUNCTION NAME : 
*      int ISDBT_ScanCurrentFrequency(void * pCurrFreq);
*  
*  DESCRIPTION : 
*  INPUT:
*			None
*  
*  OUTPUT:	int - Return Type
*  			= 
*  REMARK  :	
**************************************************************************/
extern int ISDBT_ScanCurrentFrequency(void * pCurrFreq);

/**************************************************************************
*  FUNCTION NAME : 
*      void ISDBT_AutoScanStop(void);
*  
*  DESCRIPTION : 
*  INPUT:
*			None
*  
*  OUTPUT:	void - Return Type
*  REMARK  :	
**************************************************************************/
extern void ISDBT_AutoScanStop(void);

/**************************************************************************
*  FUNCTION NAME : 
*      void ISDBT_MonitoringStart(void);
*  
*  DESCRIPTION : 
*  INPUT:
*			None
*  
*  OUTPUT:	void - Return Type
*  REMARK  :	
**************************************************************************/
extern void ISDBT_MonitoringStart(void);

/**************************************************************************
*  FUNCTION NAME : 
*      void ISDBT_Monitoring(void);
*  
*  DESCRIPTION : 
*  INPUT:
*			None
*  
*  OUTPUT:	void - Return Type
*  REMARK  :	
**************************************************************************/
extern void ISDBT_Monitoring(void);

/**************************************************************************
*  FUNCTION NAME : 
*  
*      void ISDBT_STA_RegularProcessing(void);
*  
*  DESCRIPTION : 
*  
*  INPUT:
*			None
*  
*  OUTPUT:	void - Return Type
*  
*  REMARK:	
**************************************************************************/
extern void ISDBT_STA_RegularProcessing(void);

/**************************************************************************
*  FUNCTION NAME : 
*      void ISDBT_ResetNMIDeviceDriverHandle(void)
*  
*  DESCRIPTION : 
*  INPUT:		void
*  
*  OUTPUT:	void - Return Type
*  REMARK  :	
**************************************************************************/
extern void ISDBT_ResetNMIDeviceDriverHandle(void);

/**************************************************************************
*  FUNCTION NAME : 
*      void * ISDBT_GetNMIDeviceDriverHandle(void)
*  
*  DESCRIPTION : 
*  INPUT:		void
*  
*  OUTPUT:	void * - Return Type
*  REMARK  :	
**************************************************************************/
extern void * ISDBT_GetNMIDeviceDriverHandle(void);


#if defined (CHECK_VIEW_RATING_BEFORE_AVSTART)

/* -----------------------------------------------------------------------*/
/* [Parental Rating] 1-Seg Access Ctrl./Mgmt. API for viewer                                        */
/* -----------------------------------------------------------------------*/

/**************************************************************************
*  FUNCTION NAME : 
*      void ISDBT_AccessCtrl_SetTSBlockageAdminMode(unsigned char flag)
*  
*  DESCRIPTION : Set the mode of TS Blockage administrator
*                         
*  INPUT:
*			unsigned char flag - administrator mode (TRUE or FALSE)
*
*
*  OUTPUT:
*  REMARK  :	
**************************************************************************/
extern void ISDBT_AccessCtrl_SetTSBlockageAdminMode(unsigned char flag);

/**************************************************************************
*  FUNCTION NAME : 
*      void ISDBT_AccessCtrl_GetTSBlockageAdminMode(unsigned char flag)
*  
*  DESCRIPTION : Get the mode of TS Blockage administrator
*                         
*  INPUT:
*
*
*  OUTPUT:	unsigned char - g_TSBlockage_AdminMode
*  REMARK  :	
**************************************************************************/
extern unsigned char ISDBT_AccessCtrl_GetTSBlockageAdminMode(void);

/**************************************************************************
*  FUNCTION NAME : 
*      void ISDBT_AccessCtrl_SetTSBlockageStatus(unsigned char flag)
*  
*  DESCRIPTION : Set the status of TS Blockage
*                         
*  INPUT:
*			unsigned char flag - status of TS Blockage (TRUE or FALSE)
*
*
*  OUTPUT:	unsigned char - g_TSBlockage
*  REMARK  :	
**************************************************************************/
extern void ISDBT_AccessCtrl_SetTSBlockageStatus(unsigned char flag);

/**************************************************************************
*  FUNCTION NAME : 
*      unsigned char ISDBT_AccessCtrl_CheckTSBlockageStatus(void)
*  
*  DESCRIPTION : Get the status of TS Blockage
*                         
*  INPUT:
*
*  OUTPUT:	unsigned char - g_TSBlockage (TRUE or FALSE)
*  REMARK  :	
**************************************************************************/
extern unsigned char ISDBT_AccessCtrl_CheckTSBlockageStatus(void);

/**************************************************************************
*  FUNCTION NAME : 
*      E_ISDBT_VIEWER_RATE ISDBT_AccessCtrl_GetRateClass()
*  
*  DESCRIPTION : Get the value of classification for age
*                         
*  INPUT:
*			unsigned char ucAge - classification for age
*
*  
*  OUTPUT:	E_ISDBT_VIEWER_RATE * - enumeration
*  REMARK  :	
**************************************************************************/
extern E_ISDBT_VIEWER_RATE ISDBT_AccessCtrl_GetRateClass(unsigned char ucAgeClass);

/**************************************************************************
*  FUNCTION NAME : 
*      unsigned char ISDBT_AccessCtrl_ExtractContentDesc(unsigned char ratingField)
*  
*  DESCRIPTION : Extract the bits of 'description fo the content' field
*                         
*  INPUT:
*
*  
*  OUTPUT:	unsigned char - Description fo the content
*  REMARK  :	
**************************************************************************/
extern unsigned char ISDBT_AccessCtrl_ExtractContentDesc(unsigned char ratingField);

/**************************************************************************
*  FUNCTION NAME : 
*      unsigned char ISDBT_AccessCtrl_ExtractAgeRate(unsigned char ratingField)
*  
*  DESCRIPTION : Extract the bits of classification for age field
*                         
*  INPUT:
*
*  
*  OUTPUT:	unsigned char - Classification for Age
*  REMARK  :	
**************************************************************************/
extern unsigned char ISDBT_AccessCtrl_ExtractAgeRate(unsigned char ratingField);

/**************************************************************************
*  FUNCTION NAME : 
*      E_ISDBT_VIEWER_RATE ISDBT_AccessCtrl_GetAgeRate()
*  
*  DESCRIPTION : Get the value of classification for age from the value of user setting age 
*                         
*  INPUT:
*
*  
*  OUTPUT:	E_ISDBT_VIEWER_RATE * - enumeration
*  REMARK  :	
**************************************************************************/
extern E_ISDBT_VIEWER_RATE ISDBT_AccessCtrl_GetAgeRate(void);

/**************************************************************************
*  FUNCTION NAME : 
*      void ISDBT_AccessCtrl_SetAgeRate(unsigned char user_setting_age)
*  
*  DESCRIPTION : Set the value of classification for age from the value of user setting age 
*                         
*  INPUT:
*			unsigned char user_setting_age - the value of user setting age
*
*  
*  OUTPUT:	void - return type
*  REMARK  :	
**************************************************************************/
extern void ISDBT_AccessCtrl_SetAgeRate(unsigned char user_setting_age);

/**************************************************************************
*  FUNCTION NAME : 
*      unsigned char * ISDBT_AccessCtrl_GetSettingAgeRateString(void)
*  
*  DESCRIPTION : Return the string of classification for age from initial setting of device
*                         
*  INPUT:
*  OUTPUT:	unsigned char * - return type
*  REMARK  :	
**************************************************************************/
extern unsigned char * ISDBT_AccessCtrl_GetSettingAgeRateString(void);

/**************************************************************************
*  FUNCTION NAME : 
*      unsigned char * ISDBT_AccessCtrl_GetTSContentDescString()
*  
*  DESCRIPTION : Return the string of Description of the content from current TS
*                         
*  INPUT:
*  OUTPUT:	unsigned char * - return type
*  REMARK  :	
**************************************************************************/
extern unsigned char * ISDBT_AccessCtrl_GetTSContentDescString(unsigned char desc);

/**************************************************************************
*  FUNCTION NAME : 
*      unsigned char * ISDBT_AccessCtrl_GetTSAgeRateString(void)
*  
*  DESCRIPTION : Return the string of classification for age from current TS
*                         
*  INPUT:
*  OUTPUT:	unsigned char * - return type
*  REMARK  :	
**************************************************************************/
extern unsigned char * ISDBT_AccessCtrl_GetTSAgeRateString(unsigned char ageClass);

/**************************************************************************
*  FUNCTION NAME : 
*      void ISDBT_AccessCtrl_ProcessParentalRating(void)
*  
*  DESCRIPTION : process the parental rating status 
*                         
*  INPUT:
*
*  
*  OUTPUT:	void - return type 
*  REMARK  :	
**************************************************************************/
extern void ISDBT_AccessCtrl_ProcessParentalRating(void);

#endif // CHECK_VIEW_RATING_BEFORE_AVSTART

/**************************************************************************
*  
*  FUNCTION NAME
*      void ISDBT_DRV_SetScanFreqTable(void);
*  
*  DESCRIPTION 
*      This function sets the scan frequency table according to frequency group
*      which is selected.
*  
*  INPUT
*      None
*  
*  OUTPUT
*      None
*  
*  REMARK
*      None
**************************************************************************/
extern void ISDBT_DRV_SetScanFreqTable(void);

extern int ISDBT_DRV_SetScanFreq(unsigned int *pFreqIndexList, int nFreqIndexNums);

/**************************************************************************
*  FUNCTION NAME : 
*      void ISDBT_SetCurrentServiceId(unsigned int uiServiceId)
*  
*  DESCRIPTION : Used for filtering TS unrelated to service ID
*                         
*  INPUT:
*
*  
*  OUTPUT:	void - return type 
*  REMARK  :	
**************************************************************************/
extern void ISDBT_SetCurrentServiceId(unsigned int uiServiceId);

/**************************************************************************
*  FUNCTION NAME : 
*      unsigned int ISDBT_GetCurrentServiceId(void)
*  
*  DESCRIPTION : Used for filtering TS unrelated to service ID 
*                         
*  INPUT:
*
*  
*  OUTPUT:	void - return type 
*  REMARK  :	
**************************************************************************/
extern unsigned int ISDBT_GetCurrentServiceId(void);

#ifdef ISDBT_DETECT_EXT_SERVICE_ID
/**************************************************************************
*  FUNCTION NAME : 
*      void ISDBT_SetCurrentExtServiceId(unsigned int uiServiceId)
*  
*  DESCRIPTION : Used for filtering TS unrelated to ext. service ID
*                         
*  INPUT:
*
*  
*  OUTPUT:	void - return type 
*  REMARK  :	
**************************************************************************/
extern void ISDBT_SetCurrentExtServiceId(unsigned int uiExtServiceId);

/**************************************************************************
*  FUNCTION NAME : 
*      unsigned int ISDBT_GetCurrentExtServiceId(void)
*  
*  DESCRIPTION : Used for filtering TS unrelated to ext. service ID 
*                         
*  INPUT:
*
*  
*  OUTPUT:	void - return type 
*  REMARK  :	
**************************************************************************/
extern unsigned int ISDBT_GetCurrentExtServiceId(void);
#endif

#endif /* __ISDBT_H__ */
		
