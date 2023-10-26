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

/****************************************************************************

Revision History

****************************************************************************

****************************************************************************/

#ifndef _TCC_DXB_MANAGER_SC_H_
#define _TCC_DXB_MANAGERE_SC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <tcc_sc_ioctl.h>


#define 	SC_BLOCK_DEF_NAD		0x00		//Node Address
#define 	SC_BLOCK_DEF_PCB		0x00		//Protocol controll byte

/*************** Protocol control byte(PCB) default value***********************************************/
#define 	SC_BLOCK_DEF_IPCB		0x00		//information block(i-block) default value
#define 	SC_BLOCK_DEF_RPCB		0x80		//receive read block(R-Block) default value
#define 	SC_BLOCK_DEF_SPCB		0xC0		//supervisory block(S-Block) default value
/***********************************************************************************************/

#define 	SC_RBLOCK_MASK		0x80		//R-Block(PCB) mask 
#define 	SC_SBLOCK_MASK		0xc0		//R-Block(PCB) mask 

#define 	SC_IBLOCK_MOREDATA_MASK		0x20		//I-Block(PCB) More data bit 
#define 	SC_RBLOCK_ERR_EDC_MASK		0x01		//R-Block(PCB) Parity or EDC error 
#define 	SC_RBLOCK_ERR_OTHER_MASK	0x02		//R-Block(PCB) Other error (sequence error, protocol violation , etc)
#define 	SC_SBLOCK_BLOCK_TYPE_MASK	0x20		//S-Block(PCB) Block type mask(0:request, 1:response)

#define SC_RBLOCK_SEQNO_MASK			0x10		//R-Block(PCB) Sequence no.
/*************** S-Block(PCB) Block Type  *********************************************************/
#define 	SC_SBLOCK_BLOCK_TYPE_SHIFT	5				
#define 	SC_SBLOCK_BLOCK_TYPE_LEVEL(X)	((X)<<SC_SBLOCK_BLOCK_TYPE_SHIFT)				
#define 	SC_SBLOCK_BLOCK_TYPE_REQ_MASK	SC_SBLOCK_BLOCK_TYPE_LEVEL(0)		//S-Block(PCB) Block Type, (Request)
#define 	SC_SBLOCK_BLOCK_TYPE_REPS_MASK	SC_SBLOCK_BLOCK_TYPE_LEVEL(1)		//S-Block(PCB) Block Type, (Response)
/***********************************************************************************************/
/*************** S-Block(PCB) Control Param *******************************************************/
#define	SC_SBLOCK_CON_RES		0x00		//S-block control :Resynch
#define	SC_SBLOCK_CON_IFS		0x01		//S-block control :IFS(Information field size)
#define	SC_SBLOCK_CON_WTX	0x03		//S-block control :WTX(waiting tine extension)
#define	SC_SBLOCK_CON_MASK	0x03		//S-block control :WTX(waiting tine extension)
#define	SC_SBLOKC_BLOKC_TYPE_REQUEST	0x00	//S-Block Type (Request)
#define	SC_SBLOKC_BLOKC_TYPE_RESPONSE	0x20	//S-Block Type (Response)
/***********************************************************************************************/

/*************** B-CAS Default APDU Parameter ****************************************************/
#define 	BCAS_CMD_HEADER_DEF_CLA		0x90
#define 	BCAS_CMD_HEADER_DEF_P1		0x00
#define 	BCAS_CMD_HEADER_DEF_P2		0x00
#define 	BCAS_CMD_BODY_DEF_Le			0x00
/***********************************************************************************************/
#define 	BCAS_SC_PROTOCOL_TYPE		1	

/*************** B-CAS INS Parameter ************************************************************/
#define	SC_INS_PARAM_INT		0x30	//initail setting conditions
#define	SC_INS_PARAM_ECM		0x34	//receive ECM
#define	SC_INS_PARAM_EMM		0x36	//receive EMM
#define	SC_INS_PARAM_CHK		0x3c	//confirm contract
#define	SC_INS_PARAM_EMG		0x38	//receive EMM individual message
#define	SC_INS_PARAM_EMD		0x3A	//ACQUITE AUTOMATIC DISPLAY MESSAGE DISPLAY INFORMATION
#define	SC_INS_PARAM_PVS		0x40	//request PPv status
#define	SC_INS_PARAM_PPV		0x42	//purchase PPv status
#define	SC_INS_PARAM_PRP		0x44	//confirm prepaid balance
#define	SC_INS_PARAM_CRQ		0x50	//confirm card request
#define	SC_INS_PARAM_TLS		0x52	//report call-in connection status
#define	SC_INS_PARAM_RQD		0x54	//request data
#define	SC_INS_PARAM_CRD		0x56	//center response
#define	SC_INS_PARAM_UDT		0x58	//request call-in date/time
#define	SC_INS_PARAM_UTN		0x5A	//confirm call-in destination 
#define	SC_INS_PARAM_UUR		0x5C	//request user call-in
#define	SC_INS_PARAM_IRS		0x70	//start DIRD data communication
#define	SC_INS_PARAM_CRY		0x72	//encrypt DIRD data
#define	SC_INS_PARAM_UNC		0x74	//decode DIRD response data
#define	SC_INS_PARAM_IRR		0x76	//end DIRD data communications
#define	SC_INS_PARAM_WUI		0x80	//request power-on control information
#define	SC_INS_PARAM_IDI		0x32	//acquire card id information
/***********************************************************************************************/

/*************** BCAS Shared by all commands Return code  meaning table ****************************************/
#define BCASC_SHARED_RETURN_CODE_CARDERROR				0xa1fe // Card other(Unusable card) or Other error
/***********************************************************************************************/
/*************** BCAS Initila setting Return code  meaning table ****************************************/
#define BCASC_INT_RETURN_CODE_NORMAL				0x2100 //Normal termination
/***********************************************************************************************/

/*************** BCAS ECM Return Code meaning Table ***********************************************/
#define BCASC_ECM_RETURN_CODE_NOP					0xa102 //Non_operational protocol number
#define BCASC_ECM_RETURN_CODE_NC						0xa103 //No Contract (No Kw)
#define BCASC_ECM_RETURN_CODE_NC_TIER_1				0x8901 //No Contract (Tier)
#define BCASC_ECM_RETURN_CODE_NC_TIER_2				0x8903 //No Contract (Tier)
#define BCASC_ECM_RETURN_CODE_PURCHASED_TIER		0x0800 //Purchased (Tier)
#define BCASC_ECM_RETURN_CODE_SE_ERR					0xa106 //Security error(ECM tampering error)
/***********************************************************************************************/

/*************** BCAS EMM Return Code meaning Table ***********************************************/
#define BCASC_EMM_RETURN_CODE_NORMAL				0x2100 //Normmal termination
#define BCASC_EMM_RETURN_CODE_NOP					0xa102 //Non_operational protocol number
#define BCASC_EMM_RETURN_CODE_SE_ERR				0xa107 //Security error(EMM tampering error)
/***********************************************************************************************/
/*************** BCAS EMD Return Code meaning Table ***********************************************/
#define BCASC_EMD_RETURN_CODE_NORMAL				0x2100 //Normmal termination
#define BCASC_EMD_RETURN_CODE_NCD					0xa101 //No corresponding data
/***********************************************************************************************/
/*************** SW1/SW2 Shared by all commands meaning table **********************************************************/
#define SC_SW1SW2_CMD_NORMAL					0x9000 //commmand terminated normally
#define SC_SW1SW2_CMD_MEM_ERROR				0x6400 //memorry error (memory scrambling detected)
#define SC_SW1SW2_CMD_MEM_WRITE_ERROR		0x6581 //Memory write error
#define SC_SW1SW2_CMD_LEN_ERROR				0x6700 //command length error (Lc/Le coding error)
#define SC_SW1SW2_CMD_UNDF_CLA_LOW			0x6800 //undefined CLA(CLA lower nibble!=0)
#define SC_SW1SW2_CMD_INCORR_P_VALUE		0x6A86 //incorrect P1/P2 value
#define SC_SW1SW2_CMD_UNDF_INS				0x6D00 //undefined INS
#define SC_SW1SW2_CMD_UNDF_CLA_UPPER		0x6E00 //undefined CLA(CLA upper nibble!=9)
/***********************************************************************************************/

/*************** Block Scrutre Field Position *********************************************************/
#define INITFIELD_NAD_POS	0		//node address position
#define INITFIELD_PCB_POS 	1		// Protocol control byte position
#define INITFIELD_LEN_POS	2		// length positon
#define INFOFIELD_INF_ST_POS	3	//Information filed start position
#define INFOFIELD_CLA_POS	INFOFIELD_INF_ST_POS	//Command APDU CLA Position
#define INFOFIELD_INS_POS	INFOFIELD_INF_ST_POS+1	//Command APDU INS Position
#define INFOFIELD_P1_POS	INFOFIELD_INF_ST_POS+2	//Command APDU P1 Position
#define INFOFIELD_P2_POS	INFOFIELD_INF_ST_POS+3	//Command APDU P2 Position
#define INFOFIELD_CMDDATA_LEN_POS	INFOFIELD_INF_ST_POS+4	//Command APDU command data length Position
#define INFOFIELD_CMDDATA_POS			INFOFIELD_INF_ST_POS+5	//Command APDU command data Position
/***********************************************************************************************/

/*************** Response Data Field Position ********************************************************/
#define RES_DATAFIELD_PRO_UNITNUM	0		//protocol unit num
#define RES_DATAFIELD_UNIT_LEN 		1		///unit length
#define RES_DATAFIELD_CARD_INS		2		//IC Crad Instruction (2Byte)
#define RES_DATAFIELD_RETURN_CODE	4		//Return Code	(2byte)
/***********************************************************************************************/

/*************** Initial Setting conditions Response Field Position ****************************************/
#define INIT_SET_CA_SYSTEM_ID_POS		6		//CA System ID (2byte)
#define INIT_SET_CARD_ID_POS			8		//Card ID (6byte) (Individual card ID)
#define INIT_SET_CARD_TYPE_POS		14		//Card type (1byte)(0x00:prepaid, 0x01:standdard)
#define INIT_SET_MESS_PARTI_LNG_POS	15		//Message Partition length (1byte)
#define INIT_SET_SYSTEM_KEY_POS		16		//Descrambling system key (32byte)
#define INIT_SET_CBC_INITVAL_POS		48		//Descrambling CBC Init Value (8byte)
#define INIT_SET_MANAGE_IDCOUNT_POS	56		//system management id count (1byte)
/***********************************************************************************************/
/*************** ECM Receive Response Field Position *************************************************/
#define ECM_KS_ODD_POS		6		//Ecm Ks(odd)
#define ECM_KS_EVEN_POS	14		//Ecm Ks(Even)
#define ECM_RECORDING_CON_POS		22		//Ecm Recording Control
/***********************************************************************************************/
/*************** Card ID Receive Response Field Position *************************************************/
#define CARDID_NUMOFIDS_POS		6		//Number of Card IDs
#define CARDID_DISPLAY_CARDID_POS		7		//Display card id (10*(Num_of_CardIds))
/***********************************************************************************************/
/*************** EMM Individual Message Receive Command Data Field Position *************************************************/
#define EMM_MESSAGE_CARDID_NUMOFIDS_POS		0		//Card ID
#define EMM_MESSAGE_PROTOCOL_NUM				6		//protocil number
#define EMM_MESSAGE_BR_GR_ID					7		//broadcaster group identifier
#define EMM_MESSAGE_CONTROL					8		//message control
#define EMM_MESSAGE_CODE						9		//Message Code
/***********************************************************************************************/
/*************** EMM Individual Message Receive Command Data Field default length *************************************************/
#define ISDBT_EMM_INDI_MESSSAGE_COM_DEF_LEN	9
/***********************************************************************************************/
/*************** Automatic Display Message Display Information Acquire Command Data Field Position *************************************************/
#define AUOT_DISP_CURRENT_DATA				0	//Current date (MJD:Year/month/day  from SI)
#define AUOT_DISP_BROADCAST_GR_ID			2	// Broadcaster group id
#define AUTO_DISP_DELAYINTERVAL				3	//Delay Interval
/***********************************************************************************************/
/*************** Automatic Display Message Display Information Acquire Response Field Position *************************************************/
#define AUOT_DISP_EXPIRATION_DATA	6			//Expiration date 
#define AUOT_DISP_MESSAGE_PRESET_TEXT_NO	8	//Message preset text no.
#define AUTO_DISP_DIFF_FORMAT_NUM			10	//Differential format number 
#define AUTO_DISP_DIFF_INFO_LEN				11	//Differential info. length 
#define AUTO_DISP_DIFF_INFO					13	//Differential information
/***********************************************************************************************/
/*************AutoMatic Display Message Display Information Acquire Command  Data Field default length******************************/
#define 	ISDBT_EMM_AUTODISP_MESSAGE_COM_DEF_LEN		4		//Automatic Display Message Information Acquire Command ( Default Command_data_length)  
/***********************************************************************************************/

#define DEFAULT_BLCOK_SIZE		4
#define DEFAULT_CMD_APDU_LENGTH	5

#define BCAS_SYSTEM_KEY_LEN	32
#define BCAS_ECM_KS_LEN		8
#define BCAS_CBC_INIT_LEN		8

#define BCAS_CARDID_LEGTH		6
#define BCAS_CARDID_MAX_COUNT	8
#define BCAS_MAX_ERROR_COUNT	2
#define BCAS_SEND_RETRY_CNT	3
#define BCAS_INFO_BYTE_LEN		1

#define	EDC_TYPE_LRC			0
#define	EDC_TYPE_CRC			1

/*************ISDB-T EMM Common & Individual Message Define ********************************************************************************/
#define	ISDBT_EMM_INDIVIDUAL_MESSAGE_TABLEID_EXTENSION		0x0000
#define	ISDBT_EMM_MAX_MESSAGE_CODE_LEN					256		
#define 	ISDBT_EMM_SECTON_HEAER_LENGTH					8
#define 	ISDBT_EMM_SECTON_DEFAULT_FIELD_LEN				5		//table_id_extension ~ last_section_number (5byte)
#define	ISDBT_EMM_INDI_MESSAGE_DEFAULT_FIELD_LEN		4		//Protocol_neumber(1byte) + ca_br~_Group_id(1byte) + Mess~_ID(1byte) +  Mess~_control(1byte) 
#define	ISDBT_EMM_INDI_MESSAGE_RECEIVECMD_DEFAULT_LEN	6
#define	ISDBT_EMM_MESSAGECODE_EXTRA_MESSAGE_CODE_POSITION	9
#define	ISDBT_EMM_SW1SW2_LEN								2
/***********************************************************************************************/

#define	ISDBT_EMM_INDIIDAUL_MESSAGE_ENCRYPT_CHECK		0xFF	//EMM Individual Message Protocol nummber (if is 0xff , no encryption)
#define	ISDBT_SC_MAX_IFSD							254
#define	ISDBT_EMM_MAX_MESSAGE_CNT				13
#define	ISDBT_EMMMESSAGE_DIFFDATA_INSERT_VALUE	0x1A
#define	ISDBT_EMM_MAX_MAIL_SIZE				800*2	//Max 800 character
#define	ISDBT_EMM_MAX_AUTOMESSAGE_SIZE		400
#define 	ISDBT_MAIL_NO_PRESET_MESSAGE			0x0000
#define 	ISDBT_AUTOMESSAGE_NO_DIFF_INFORMATION			0x0000
#define	ISDBT_MAX_BLOCK_SIZE			(ISDBT_SC_MAX_IFSD + DEFAULT_BLCOK_SIZE)

#define 	BCAS_MAX_EMM_COM_MESSAGE_CNT	ISDBT_EMM_MAX_MESSAGE_CNT	*2

#define TA_CHECK_BIT    0x10
#define TB_CHECK_BIT    0x20
#define TC_CHECK_BIT    0x40
#define TD_CHECK_BIT    0x80

#if 1//0 //jktest
typedef struct
{
    unsigned char   ucTS;
    unsigned char   ucT0;
    unsigned short  usTA[4];
    unsigned short  usTB[4];
    unsigned short  usTC[4];
    unsigned short  usTD[4];
    unsigned char   ucHC[15]; // Historical Characters - Max 15 Bytes
    unsigned int    ucTCK;
} sTCC_SC_ATR_CHARS;
#endif

typedef struct
{
	unsigned char	CLA;		//CLA Code, always 0x90.   (ISDB-T Private defined by ARIB STD)
	unsigned char INS;		//INS Code  (ISDB-T exception value by ARIB STD)
	unsigned char P1;			//Parammeter 1.
	unsigned char P2;			//Parammeter 2.
}SC_Cmd_APDU_Header_Architecture_t;

typedef struct
{
	unsigned char	CLA;		//CLA Code, always 0x90.   (ISDB-T Private defined by ARIB STD)
	unsigned char INS;		//INS Code  (ISDB-T exception value by ARIB STD)
	unsigned char P1;			//Parammeter 1.
	unsigned char P2;			//Parammeter 2.
	unsigned char Lc;			//Data length
	unsigned char *Data;				//
	unsigned char Le;			//Response data length.
}SC_Cmd_Basic_Architecture_t;

typedef struct
{
 	unsigned char *Data;		//
	unsigned char SW1;		// Status Byte 1
	unsigned char SW2;		// Status Byte 2
}SC_Response_Architecture_t;

typedef struct
{
	unsigned char	Protocol_Unit_Num;
	unsigned char	Unit_Len;
	unsigned short IC_Card_Instruction;
	unsigned short Return_Code;
	unsigned short CA_System_ID;
	unsigned char Card_ID[BCAS_CARDID_LEGTH];
	unsigned char	Card_Type;
	unsigned char	Message_Partition_len;
	unsigned char Desc_System_Key[BCAS_SYSTEM_KEY_LEN];
	unsigned char Desc_CBC_InitValue[BCAS_CBC_INIT_LEN];
	unsigned short SW1_SW2;
}BCAS_InitSet_Architecture_t;

typedef struct
{
	unsigned char	Protocol_Unit_Num;
	unsigned char	Unit_Len;
	unsigned short IC_Card_Instruction;
	unsigned short Return_Code;
	unsigned char	Ks_Odd[BCAS_ECM_KS_LEN];
	unsigned char	Ks_Even[BCAS_ECM_KS_LEN];
	unsigned char	Recording_Control;
	unsigned short SW1_SW2;
}BCAS_Ecm_Architecture_t;

typedef struct
{
	unsigned char Manufacturer_identifier;		//ASCII character
	unsigned char Version;						
}BCAS_CardID_Type_Architecture_t;

typedef struct
{
 	unsigned char	Arch_Align_1;
	unsigned char ID_Identifier;		// 3bit(ID identifier)
	unsigned char ID[BCAS_CARDID_LEGTH];	// 45bits(ID)
}BCAS_CardID_ID_Architecture_t;

typedef struct
{
	BCAS_CardID_Type_Architecture_t Card_Type;
	BCAS_CardID_ID_Architecture_t	   Card_Id;
	unsigned short Check_Code;
}BCAS_CardID_Item_Architecture_t;

typedef struct
{
	unsigned char	Protocol_Unit_Num;
	unsigned char	Unit_Len;
	unsigned short IC_Card_Instruction;
	unsigned short Return_Code;
	unsigned char	 Number_of_CardIDs;
	unsigned char	 Arch_Align_1;
	BCAS_CardID_Item_Architecture_t	Display_Card_ID[BCAS_CARDID_MAX_COUNT];		//Carrd Id "count =1 : Individual card id ", "count>1 : Group id".
	unsigned short SW1_SW2;
}BCAS_CardID_Architecture_t;

typedef struct
{
	unsigned char	Protocol_Unit_Num;
	unsigned char	Unit_Len;
	unsigned short IC_Card_Instruction;
	unsigned short Return_Code;
	unsigned short SW1_SW2;
}BCAS_Default_Architecture_t;

typedef struct
{
	unsigned char	Protocol_Unit_Num;
	unsigned char	Unit_Len;
	unsigned short IC_Card_Instruction;
	unsigned short Return_Code;
	unsigned char Response_message_code[ISDBT_EMM_MAX_MESSAGE_CODE_LEN];
	unsigned short SW1_SW2;
}BCAS_EMM_Indi_Message_Architecture_t;

typedef struct
{
	unsigned char	Protocol_Unit_Num;
	unsigned char	Unit_Len;
	unsigned short IC_Card_Instruction;
	unsigned short Return_Code;
	unsigned short Expiration_date;
	unsigned short Message_preset_text_no;
	unsigned char Differential_format_number;
	unsigned short Differential_info_length;
	unsigned char* Differential_information;
	unsigned short SW1_SW2;
	unsigned char	ucMessageInfoGetStatus;
}BCAS_Auto_Disp_Message_Architecture_t;

typedef struct
{
	unsigned char	ucTable_Id;					//8bit
	unsigned char	ucSection_Syntax_Indicator;		//1bit
	unsigned char	ucPrivate_Indicator;			//1bit
	unsigned char	ucReserved1;					//2bit
	unsigned short	uiSection_len;				//12bit
	unsigned short	uiTable_Id_Extension;			//16bit
	unsigned char	ucReserved2;					//2bit
	unsigned char	ucVersionNum;				//5bit
	unsigned char	ucCurr_Next_Indicator;			//1bit
	unsigned char	ucSectionNum;				//8bit
	unsigned char	ucLastSectionNum;				//8bit
}EMM_Message_Section_Header_t;

typedef struct
{
	unsigned char	ucBroadcaster_GroupID;		//Broadcaster group identifier
	unsigned char	ucDeletion_Status;			//Automatic message erasure type	(refer to EMM_COMMESSAGE_DELETION_STATUS)
	unsigned char	ucDisp_Duration1;			//Automatic display duration 1
	unsigned char	ucDisp_Duration2;			//Automatic display duration 2
	unsigned char	ucDisp_Duration3;			//Automatic display duration 3
	unsigned char	ucDisp_Cycle;				//Automatic display count
	unsigned char	ucFormat_Version;			//Format number
	unsigned short	uiMessageLen;			//Message length
	unsigned char	ucMessage_Area[ISDBT_EMM_MAX_MAIL_SIZE];		//Message code payload
}EMM_Common_Message_Field_t;


typedef struct _EMM_Individual_Message_Field_t
{
	unsigned char ucCard_ID[BCAS_CARDID_LEGTH];		// Card ID 48bits
	unsigned short	uiMessage_Len;					//Message byte length
	unsigned char	ucProtocol_Num;						//Protocol number
	unsigned char	ucCa_Broadcaster_Gr_ID;			//Broadcaster group identifier
	unsigned char	ucMessage_ID;						//Message ID
	unsigned char	ucMessage_Control;					//Message control
	unsigned char ucMessage_Area[ISDBT_EMM_MAX_MAIL_SIZE];					//Message Code Region
	struct _EMM_Individual_Message_Field_t *Prev_EMM_IndiMessage;
}EMM_Individual_Message_Field_t;

typedef struct
{
	EMM_Message_Section_Header_t 		EMM_MessSection_Header;
	EMM_Common_Message_Field_t		EMM_ComMessage_Field;
	unsigned int						uiEMM_Message_Section_CRC;	//CRC error detection (4byte)
}EMM_Common_Message_t;

typedef struct
{
	EMM_Message_Section_Header_t 		EMM_MessSection_Header;
	EMM_Individual_Message_Field_t		*EMM_IndividualMessage_Field;
	unsigned int						uiEMM_Message_Section_CRC;	//CRC error detection (4byte)
	unsigned int						uiEMM_IndividualMessage_PayloadCnt;
}EMM_Individual_Message_t;

typedef struct
{
	unsigned short uiAlternation_Detector;			//Tampering check(automaic Message)
	unsigned short uiLimmit_Date;					//Expiration date (automaic Message)
	unsigned short uiFixed_Mmessage_ID;			// IF of preset Mmessage
	unsigned char ucExtraMessage_FormatVersion;	//Differential format number
	unsigned short uiExtra_Message_Len;				//Differential information Length
	unsigned char ucExtra_MessageCode[ISDBT_EMM_MAX_MAIL_SIZE];				//Differential information
}EMM_AutoMessage_Code_Region_t;

typedef struct
{
	unsigned short uiReserved1;			//Reserved (Mail)
	unsigned short uiReserved2;					//Reserved (Mail)
	unsigned short uiFixed_Mmessage_ID;			// IF of preset Mmessage
	unsigned char ucExtraMessage_FormatVersion;	//Differential format number
	unsigned short uiExtra_Message_Len;				//Differential information Length
	unsigned char ucExtra_MessageCode[ISDBT_EMM_MAX_MAIL_SIZE];				//Differential information
}EMM_MailMessage_Code_Region_t;

typedef struct
{
	unsigned int uiMessageID;				//ID of preset message (table_id_extension)
	unsigned int	ucBroadcaster_GroupID;		//Broadcaster group identifier
	unsigned int	ucDeletion_Status;			//Automatic message erasure type	(refer to EMM_COMMESSAGE_DELETION_STATUS)
	unsigned int	ucDisp_Duration1;			//Automatic display duration 1
	unsigned int	ucDisp_Duration2;			//Automatic display duration 2
	unsigned int	ucDisp_Duration3;			//Automatic display duration 3
	unsigned int	ucDisp_Cycle;				//Automatic display count
	unsigned int	ucFormat_Version;			//Format number
	unsigned int	uiMessageLen;			//Message length
	unsigned int	ucDisp_Horizontal_PositionInfo;			//display position
	unsigned int	ucDisp_Vertical_PositionInfo;			//display position
	unsigned char	ucMessagePayload[ISDBT_EMM_MAX_MAIL_SIZE];		//Message data
}EMM_Message_Info_t;

typedef struct
{
	EMM_Message_Info_t		EMM_MessageInfo[ISDBT_EMM_MAX_MESSAGE_CNT];
	unsigned char ucMessage_Cnt;				//message count
}EMM_Message_Disp_Info_t;

typedef struct
{
	unsigned int	uiSend_Cnt;
	unsigned int	uiReceive_Cnt;
	unsigned int	uiTotal_Cnt;
}EMG_ResCMD_MessageCode_LengthCheck_t;

typedef struct
{
	unsigned int	uiMessageType;
	unsigned int	uiEncrypted_status;
}EMM_Message_Receive_Status_t;

/*************** Smart Card Error Type define ******************************************************/
typedef enum
{
	SC_ERR_OK		= 0,
	SC_ERR_INVALID_NAD,
	SC_ERR_RECEIVE_EDC,
	SC_ERR_RECEIVE_OTHER,
	SC_ERR_PROTOCOL_TYPE,
	SC_ERR_CMD_FAIL,				//5
	SC_ERR_OPEN,
	SC_ERR_CLOSE,
	SC_ERR_RESET,
	SC_ERR_GET_ATR,
	SC_ERR_CARD_DETECT,			//10
	SC_ERR_RETURN_CODE,
	SC_ERR_MASSAGECODE_NONE,
	SC_ERR_INPUTLENGTH_ERROR,
	SC_ERR_MAX,	
}BCAS_ERROR_TYPE;
/***********************************************************************************************/
typedef enum
{
	SC_CHANGE_IFSD_OK = 0,
	SC_CHANGE_IFSD_ERROR,
	SC_CHANGE_IFSD_MAX,
}
BCAS_CHANGE_IFSD_ERROR_TYPE;
/***********************************************************************************************/
typedef enum
{
	EMM_Message_Get_Success =100,
	EMM_Message_Get_Fail,
	EMM_Message_Exist,
	EMM_Message_Parsing_Err,
	EMM_Message_Individual_Message,
	EMM_Message_Err_MAX,
}ISDBT_EMM_MESSAGE_ERROR_TYPE;
/***********************************************************************************************/

typedef enum
{
	BCAS_IBLCOK =0,
	BCAS_IBLCOK_MODREDATA,
	BCAS_RBLCOK,
	BCAS_SBLCOK_IFS_REQ,
	BCAS_SBLCOK_WTX_REQ,
	BCAS_SBLCOK_RESYNC_REQ,
	BCAS_SBLCOK_RESPONSE,
}BCAS_BLOCKTYPE;

typedef enum
{
	MULTI2_MODE_HW=0,
	MULTI2_MODE_SW,	
	MULTI2_MODE_HWDEMUX,	
	MULTI2_MODE_NONE,	
}BCAS_MULTI2_MODE;

typedef enum
{
	EMM_BASIC	=0,
	EMM_MAIL_MESSAGE,
	EMM_AUTODISPLAY_MESSAGE,
}ISDBT_EMM_TYPE;

typedef enum
{
	EMM_DSTATUS_DELETABLE = 0,
	EMM_DSTATUS_NOT_DELETABLE,
	EMM_DSTATUS_ERASE,
}EMM_COMMESSAGE_DELETION_STATUS;

typedef enum
{
	EMM_COMMESSAGE_STORED_ICCARD =1,
	EMM_COMMESSAGE_STORED_DIRD =2,
	EMM_COMMESSAGE_STORED_RESERVED,
}EMM_COMMESSAGE_STORED_TPYE;

typedef enum
{
	EMM_INDIMESSAGE_ENCRYPTED=0,
	EMM_INDIMESSAGE_UNENCRYPTED,
}EMM_INDIVIDUAL_MESSAGE_TYPE;

/********************************************************************************************/
/********************************************************************************************
						FOR MW LAYER FUNCTION
********************************************************************************************/
/********************************************************************************************/

int 	TCC_Dxb_SC_Manager_Open(void);
int 	TCC_Dxb_SC_Manager_Close(void);
int 	TCC_Dxb_SC_Manager_Reset(void);
int 	TCC_DxB_SC_Manager_CardDetect(void);
void 	TCC_Dxb_SC_Manager_InitParam(void);
int 	TCC_Dxb_SC_Manager_AutoDisp_Message_Get(unsigned short uiCurrDate, unsigned char ucCA_Gr_ID, unsigned char ucDelay_Interval);
int 	TCC_Dxb_SC_Manager_AutoDisp_Message_GetStatus(void);
void 	TCC_Dxb_SC_Manager_EMM_ComMessageInit(void);
void 	TCC_Dxb_SC_Manager_EMM_IndiMessageInit(void);
void 	TCC_DxB_SC_Manager_SetDescriptionMode(int desc_mode);
int 	TCC_DxB_SC_Manager_Init(void);
int 	TCC_Dxb_SC_Manager_GetATR(unsigned int dev_id, unsigned char*ucAtrBuf,  unsigned int* uiAtrLength);
int 	TCC_Dxb_SC_Manager_Parse_Atr(unsigned char *pATRData, unsigned int*uATRLength, sTCC_SC_ATR_CHARS *pstATRParse);
int 	TCC_Dxb_SC_Manager_GetCASystemID(void);
void 	TCC_Dxb_SC_Manager_CmdHeaderSet(char cla, char p1, char p2);
int 	TCC_Dxb_SC_Manager_GenEDC(char edc, char data);
int 	TCC_Dxb_SC_Manager_DefSetEDC(int type);
int 	TCC_Dxb_SC_Manager_ProcessResINT(unsigned char *data);
int 	TCC_DxB_SC_Manager_ProcessResponse(char ins, unsigned char *data);
int 	TCC_DxB_SC_Manager_ReceiveErrCheck(unsigned char *data);
int 	TCC_DxB_SC_Manager_ReceiveBlockCheck(unsigned char *data);
int 	TCC_DxB_SC_Manager_SendT1(char ins, int data_len, char *data);
int 	TCC_DxB_SC_Manager_SendT1_Glue(char ins, int data_len, char *data);
int 	TCC_Dxb_SC_Manager_ChangeIFSD(void);
int 	TCC_Dxb_SC_Manager_CalcEDC (unsigned char *buf, int len, unsigned char *edc);
int 	TCC_DxB_SC_Manager_GetCardInfo (void **pCardInfo, int *pSize);
int		TCC_DxB_SC_Manager_EMM_SectionParsing(int data_len, char *data);
int 	TCC_DxB_SC_Manager_EMM_MessageSectionParsing(int data_len, char *data);
void 	TCC_DxB_SC_Manager_ClearCardInfo (void);
void 	TCC_Dxb_SC_Manager_EMM_ComMessageDeInit(void);
void 	TCC_Dxb_SC_Manager_EMM_IndiMessageDeInit(int cnt);
int 	TCC_Dxb_SC_Manager_EMM_MessageInfoGet(EMM_Message_Info_t *DispMessage);
void 	TCC_Dxb_SC_Manager_EMM_DispMessageInit(void);



#ifdef __cplusplus
}
#endif

#endif
