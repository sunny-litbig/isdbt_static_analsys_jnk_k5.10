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
#include <string.h>
#include <BitParser.h>
#include <ISDBT_Caption.h>
#include <TsParser_Subtitle.h>
#include "TsParser_Subtitle_Debug.h"

/****************************************************************************
DEFINITION
****************************************************************************/
/* The line below should be uncommented to display any Brazil characters as right. */
/* If you comment the line below, ONLY JAPAN characters could be displayed correctly. */
#define USE_EUCJPMAP_FOR_LATINEXT_CODEMAPPING

/* The line below should be uncommented to display any Brazil characters as right. */
/* If you comment the line below, ONLY JAPAN characters could be displayed correctly. */
#define USE_EUCJPMAP_FOR_SPECIAL_CODEMAPPING

#define	ISDBT_CODESET_MAX		5

/* Control code used in SI */
#define	SP				0x20	// Space
#define	ESC				0x1b	// Extended code system
#define LS0				0x0f 	// G0 -> GL Locking shift
#define	LS1				0x0e	// G1 -> GL Locking shift
#define	SS2				0x19	// G2 -> GL Single shift
#define	SS3				0x1d	// G3 -> GL Single shift
#define	APR				0x0d	// Linefeed at operation position
#define LS2				0x6e	// G2 -> GL Locking shift (should follow ESC)
#define	LS3				0x6f	// G3 -> GL Locking shift (should follow ESC)
#define LS1R			0x7e	// G1 -> GR Locking shift (should follow ESC)
#define	LS2R			0x7d	// G2 -> GR Locking shift (should follow ESC)
#define	LS3R			0x7c	// G3 -> GR Locking shift (should follow ESC)
#define MSZ				0x89	// Specifies the chatacter size is middle
#define	NSZ				0x8a	// Specifies the chatacter size is normal

 /* macro for invocation/designation of char code */
#define	SET_GLG0		{GL = G0;}
#define	SET_GLG1		{GL = G1;}
#define	SET_GLG2		{GL = G2;}
#define	SET_GLG3		{GL = G3;}

#define	SET_GRG1		{GR = G1;}
#define	SET_GRG2		{GR = G2;}
#define	SET_GRG3		{GR = G3;}

#define	SET_GLINX(x)	{ iGLReadByte = (x); }

#define	NEXTINDEX1		{ i++; }
#define	NEXTINDEX2		{NEXTINDEX1 NEXTINDEX1}
#define	NEXTINDEX3		{NEXTINDEX1 NEXTINDEX2}
#define	NEXTGLINDEX		{ i += iGLReadByte; }

#define	DO_LS0			{SET_GLG0 SET_GLINX(1)}
#define	DO_LS1			{SET_GLG1 SET_GLINX(0)}
#define	DO_APR			{pOutBuf[iOffset++] = '\n';}
#define	DO_SP			{pOutBuf[iOffset++] = ' ';}
#define	DO_MSZ			{pOutBuf[iOffset++] = MSZ;}

#define	DO_LS2			{SET_GLG2 SET_GLINX(0) NEXTINDEX1}
#define	DO_LS3			{SET_GLG3 SET_GLINX(0) NEXTINDEX1}
#define	DO_LS1R			{SET_GRG1 NEXTINDEX1}
#define	DO_LS2R			{SET_GRG2 NEXTINDEX1}
#define	DO_LS3R			{SET_GRG3 NEXTINDEX1}

/* magic number explanation */
/*
#define	CODENUM_PER_PAGE	94
#define	TOTAL_PAGE_NUM		84  // (p.0x21 ~ p.0x74)
#define	CODENUM_IN_PAGE74	6

	//| (*) UNIPAGE_CODENUM =
	//| 			CODENUM_PER_PAGE * (TOTAL_PAGE_NUM-1)
	//| 			+ CODENUM_IN_PAGE74
*/
#define	UNIPAGE_CODENUM		7808

#define FIRST_BYTE_OF_HIRAGANA				(0xA4) // 0x24
#define FIRST_BYTE_OF_KATAKANA				(0xA5) // 0x25
#ifdef USE_EUCJPMAP_FOR_LATINEXT_CODEMAPPING
#define FIRST_BYTE_OF_LATINEXTENSION		(0xA9) // 0x29
#endif
#ifdef USE_EUCJPMAP_FOR_SPECIAL_CODEMAPPING
#define FIRST_BYTE_OF_SPECIAL				(0xAA) // 0x2A
#endif
#define	FIRST_BYTE_OF_ALPHA					(0xA3)


/* G Area Check Macro */
#define CHECK_GL_CODE_RANGE(code)		((0x21 <= (unsigned char)(code)) && ((unsigned char)(code) <= 0x7E))
#define CHECK_GR_CODE_RANGE(code)		((0xA1 <= (unsigned char)(code)) && ((unsigned char)(code) <= 0xFE))

#define	TARGET_DISPLAY_WIDTH		800
#define	TARGET_DISPLAY_HEIGHT		480
#define	ISDBT_DISPLAY_WIDTH			720
#define	ISDBT_DISPLAY_HEIGHT			576

/* ----------------------------------------------------------------------
**  ARIB Parsing Delimiter Characters
** ---------------------------------------------------------------------- */
#define ARIB_PARSING_MIDDLE_CHAR			(0x3B)	// Intermediate/Middle Character
#define ARIB_PARSING_END_CHAR				(0x20)	// End Character

/* Maximum Numbers of Closed Caption Data Unit */
#define	MAX_NUMS_OF_CC_DATA_GROUP			(256)

/* Maximum Numbers of Closed Caption Data Unit */
#define	MAX_NUMS_OF_CC_DATA_UNIT			(7)

/* Maximum Numbers of Parsing Parameter */
#define MAX_NUMS_OF_CSI_PARAMS				(4)


/* --------------------------------------------------
**  PES
** -------------------------------------------------- */
/* Synchronized PES Packet */
#define SYNC_PES_DATA_ID					(0x80)
#define SYNC_PES_PRIVATE_STREAM_ID			(0xFF)

/* Asynchronous PES Packet */
#define ASYNC_PES_DATA_ID					(0x81)
#define ASYNC_PES_PRIVATE_STREAM_ID			(0xFF)

/* --------------------------------------------------
**  Data Group
** -------------------------------------------------- */
/* Data Group ID */
#define DATA_GROUP_ID_GROUP_A				(0x00)
#define DATA_GROUP_ID_GROUP_B				(0x20)
#define DATA_GROUP_ID_MASK					(0xF0)

/* Caption Data Type */
#define CAPTION_DATA_MANAGE					(0x00)
#define CAPTION_DATA_STATE_1ST_LANG			(0x01)
#define CAPTION_DATA_STATE_2ND_LANG			(0x02)
#define CAPTION_DATA_STATE_3RD_LANG			(0x03)
#define CAPTION_DATA_STATE_4TH_LANG			(0x04)
#define CAPTION_DATA_STATE_5TH_LANG			(0x05)
#define CAPTION_DATA_STATE_6TH_LANG			(0x06)
#define CAPTION_DATA_STATE_7TH_LANG			(0x07)
#define CAPTION_DATA_STATE_8TH_LANG			(0x08)
#define CAPTION_DATA_TYPE_MASK				(0x0F)

/* Display Mode */
#define DMF_RECEPTION_AUTO					(0x00)
#define DMF_RECEPTION_NON_DISP_AUTO			(0x04)
#define DMF_RECEPTION_SELECT_DISP			(0x08)
#define DMF_RECEPTION_SPECIFIC_CONDITION	(0x0C)

#define DMF_RECORDING_PLAYBACK_AUTO			(0x00)
#define DMF_RECORDING_PLAYBACK_NON_DISP_AUTO	(0x01)
#define DMF_RECORDING_PLAYBACK_SELECT_DISP	(0x02)
#define DMF_RECORDING_PLAYBACK_RESERVED		(0x03)

/* Character Coding */
#define TCS_8BITS_CODE						(0x00)
#define TCS_RESERVED_FOR_UCS				(0x01)
#define TCS_RESERVED_1						(0x02)
#define TCS_RESERVED_2						(0x03)

/* --------------------------------------------------
**  Data Unit
** -------------------------------------------------- */
#define DATA_UNIT_SEPARATOR_CODE			(0x1F)

/* --------------------------------------------------
**  Graphic Set
**	    - The most graphic set is 1-byte code.
** -------------------------------------------------- */
/* G SET */
#define GS_KANJI							(0x42)	// 2-byte code
#define GS_ALPHANUMERIC						(0x4A)
#define GS_LATIN_EXTENSION					(0x4B)
#define GS_SPECIAL							(0x4C)
#define GS_HIRAGANA							(0x30)
#define GS_KATAKANA							(0x31)
#define GS_MOSAIC_A							(0x32)
#define GS_MOSAIC_B							(0x33)
#define GS_MOSAIC_C							(0x34)	// 1-byte code, non-spacing
#define GS_MOSAIC_D							(0x35)	// 1-byte code, non-spacing
#define GS_PROPORTIONAL_ALPHANUMERIC		(0x36)					// Proportional alphanumeric
#define GS_PROPORTIONAL_HIRAGANA			(0x37)					// Proportional hiragana
#define GS_PROPORTIONAL_KATAKANA			(0x38)					// Proportional katakana
#define GS_JIS_X_0201_KATAKANA				(0x49)					// JIS X 0201 katakana
#define GS_JIS_COMPATIBLE_KANJI_PLANE_1		(0x39)	// 2-byte code	// JIS compatible Kanji Plane 1
#define GS_JIS_COMPATIBLE_KANJI_PLANE_2		(0x3A)	// 2-byte code	// JIS compatible Kanji Plane 2
#define GS_ADDITIONAL_SYMBOLS				(0x3B)	// 2-byte code	// Additional symbols
/* DRCS */
#define GS_DRCS_0							(0x40)	// 2-byte code
#define GS_DRCS_1							(0x41)
#define GS_DRCS_2							(0x42)
#define GS_DRCS_3							(0x43)
#define GS_DRCS_4							(0x44)
#define GS_DRCS_5							(0x45)
#define GS_DRCS_6							(0x46)
#define GS_DRCS_7							(0x47)
#define GS_DRCS_8							(0x48)
#define GS_DRCS_9							(0x49)
#define GS_DRCS_10							(0x4A)
#define GS_DRCS_11							(0x4B)
#define GS_DRCS_12							(0x4C)
#define GS_DRCS_13							(0x4D)
#define GS_DRCS_14							(0x4E)
#define GS_DRCS_15							(0x4F)
#define GS_MACRO							(0x70)

/* ----------------------------------------------------------------------
**  Extension Control Code (CSI - Control Sequence Introducer)
** ---------------------------------------------------------------------- */
#define ARIB_CSI_GSM						(0x42)	// Character Deformation
#define ARIB_CSI_SWF						(0x53)	// Set Writing Format
//#define ARIB_CSI_CCC						(0x54)	// Composite Character Composition
#define ARIB_CSI_SDF						(0x56)	// Set Display Format
//#define ARIB_CSI_SSM						(0x57)	// Character Composition Dot Designation
#define ARIB_CSI_SHS						(0x58)	// Set Horizontal Spacing
#define ARIB_CSI_SVS						(0x59)	// Set Vertical Spacing
#define ARIB_CSI_PLD						(0x5B)	// Partial Line Down
#define ARIB_CSI_PLU						(0x5C)	// Partial Line Up
#define ARIB_CSI_GAA						(0x5D)	// Coloring Block
#define ARIB_CSI_SRC						(0x5E)	// Raster Color Designation
#define ARIB_CSI_SDP						(0x5F)	// Set Display Position
#define ARIB_CSI_ACPS						(0x61)	// Active Coordinate
#define ARIB_CSI_TCC						(0x62)	// Switch Control
#define ARIB_CSI_ORN						(0x63)	// Ornament Control
#define ARIB_CSI_MDF						(0x64)	// Font
#define ARIB_CSI_CFS						(0x65)	// Character Font Set
#define ARIB_CSI_XCS						(0x66)	// External Character Set
#define ARIB_CSI_SCR						(0x67)	// Scroll designation
#define ARIB_CSI_PRA						(0x68)	// Built-in Sound Replay
#define ARIB_CSI_ACS						(0x69)	// Alternative Character Set
#define ARIB_CSI_RCS						(0x6E)	// Raster Color Command
#define ARIB_CSI_SCS						(0x6F)	// Skip Character Set

/* Control Code Macro */
#define CTRLCODE_LS0(a)				stGSet[a].GL = CE_G0
#define CTRLCODE_LS1(a)				stGSet[a].GL = CE_G1
#define CTRLCODE_LS2(a)				stGSet[a].GL = CE_G2
#define CTRLCODE_LS3(a)				stGSet[a].GL = CE_G3
#define CTRLCODE_LS1R(a)			stGSet[a].GR = CE_G1
#define CTRLCODE_LS2R(a)			stGSet[a].GR = CE_G2
#define CTRLCODE_LS3R(a)			stGSet[a].GR = CE_G3
#define CTRLCODE_SS2(a)				stGSet[a].GL = CE_G2
#define CTRLCODE_SS3(a)				stGSet[a].GL = CE_G3

#define MAX_COLOR      128

#define MACRO_GSET(a, g0,g1,g2,g3,gl,gr)	{stGSet[a].G[0] = g0; stGSet[a].G[1] = g1; stGSet[a].G[2] = g2; stGSet[a].G[3] = g3; stGSet[a].GL = gl; stGSet[a].GR = gr;}


/****************************************************************************
DEFINITION OF TYPE
****************************************************************************/
enum
{
	ST_NO_ERROR = 0x00,
	ST_ERROR_BAD_PARAMETER,             /* Bad parameter passed       */
	ST_ERROR_NO_MEMORY,                 /* Memory allocation failed   */
	ST_ERROR_NO_FREE_HANDLES,           /* Cannot open device again   */
	ST_ERROR_OPEN_HANDLE,               /* At least one open handle   */
	ST_ERROR_INVALID_HANDLE,            /* Handle is not valid        */
	ST_ERROR_TIMEOUT,                  /* Timeout occured            */
	ST_ERROR_BAD_SEGMENT,
	ST_ERROR_PROCESSING_SEGMENT,
	ST_ERROR_DISPLAYING,		//temp

	STSUBT_EVENT_SEGMENT_AVAILABLE,/* resumption of valid segment in the PID  */
	STSUBT_EVENT_UNKNOWN_REGION,   /* Undefined region referenced             */
	STSUBT_EVENT_UNKNOWN_CLUT,     /* Undefined CLUT referenced               */
	STSUBT_EVENT_UNKNOWN_OBJECT,   /* Undefined object referenced             */
	STSUBT_EVENT_NOT_SUPPORTED_OBJECT_TYPE,/* object type is not supported    */
	STSUBT_EVENT_NOT_SUPPORTED_OBJECT_CODING,/* object coding is not supported*/
	STSUBT_EVENT_COMP_BUFFER_NO_SPACE, /* no space in Composition Buffer      */
	STSUBT_EVENT_PIXEL_BUFFER_NO_SPACE/* no space in Pixel Buffer            */
};

/* Code Element */
typedef enum
{
	CE_G0,
	CE_G1,
	CE_G2,
	CE_G3,

	CE_MAX
} E_CODE_ELEMENT;

typedef enum
{
	KANJI_SET,
	ALPHANUMERIC_SET,
	HIRAGANA_SET,
	KATAKANA_SET,
	LATIN_EXT_SET,
	UNDEFINED_SET
} ISDBT_CHAR_SET;

/* G Set, ARIB STD-B24 */
typedef struct
{
	int				G[4];
	int				GL;
	int				GR;
} T_GSET;

typedef struct{
	unsigned int r;
	unsigned int g;
	unsigned int b;
	unsigned int a;
}CMLA_TYPE;

typedef struct
{
	unsigned char			terminationCode;
	ISDBT_CHAR_SET			codeSet;
} CODESET_MAPPER;

typedef struct {
	ISDBT_CHAR_SET		codeSet;
	int 			(*CodeConv) (unsigned char *, unsigned char *);
} JPN_CODESET_CMD;


extern void CCPARS_Parse_Pos_Init(T_SUB_CONTEXT *p_sub_ctx, int mngr, int index, int param);
extern void subtitle_get_internal_sound(int index);

static T_GSET stGSet[2];

static int g_SingleShift[2] = {0, 0};
static int g_OldGL[2] = {CS_NONE, CS_NONE};

ISDBT_CAPTION_INFO		gIsdbtCapInfo[2];
ISDBT_CAPTION_DRCS*	gDRCSBaseStart[16] = {NULL, };

static T_GRP_DATA_TYPE g_grp_data_type[2];

#if 0
static CMLA_TYPE cmla_grp[MAX_COLOR] = {
	//Index value, R,	G, B, Alpha,	Name/Comments
	{0,		0,		0,		255},	//Black (0)
	{255,	0,		0,		255},	//Red
	{0,		255,	0,		255},	//Green
	{255,	255,	0,		255},	//Yellow
	{0,		0,		255,	255},	//Blue
	{255,	0,		255,	255},	//Magenta
	{0,		255,	255,	255},	//Cyan
	{255,	255,	255,	255},	//White
	{0,		0,		0,		0},		//Transparent
	{170,	0,		0,		255},	//Half brightness Red
	{0,		170,	0,		255},	//Half brightness Green (10)
	{170,	170,	0,		255},	//Half brightness Yellow
	{0,		0,		170,	255},	//Half brightness Blue
	{170,	0,		170,	255},	//Half brightness magenta
	{0,		170,	170,	255},	//Half brightness Cyan
	{170,	170,	170,	255},	//Half brightness White(Gray)
	{0,		0,		85,		255},	//
	{0,		85,		0,		255},	//
	{0,		85,		85,		255},	//
	{0,		85,		170,	255},	//
	{0,		85,		255,	255},	// (20)
	{0,		170,	85,		255},	//
	{0,		170,	255,	255},	//
	{0,		255,	85,		255},	//
	{0,		255,	170,	255},	//
	{85,	0,		0,		255},	//
	{85,	0,		85,		255},	//
	{85,	0,		170,	255},	//
	{85,	0,		255,	255},	//
	{85,	85,		0,		255},	//
	{85,	85,		85,		255},	// (30)
	{85,	85,		170,	255},	//
	{85,	85,		255,	255},	//
	{85,	170	,0	,255},//
	{85,	170	,85 ,255},//
	{85,	170	,170	,255},//
	{85,	170	,255	,255},//
	{85,	255	,0	,255},//
	{85,	255	,85 ,255},//
	{85,	255	,170	,255},//
	{85,	255	,255	,255},	// (40)
	{170,	0	,85 ,255},//
	{170,	0	,255	,255},//
	{170,	85 ,0	,255},//
	{170,	85 ,85 ,255},//
	{170,	85 ,170	,255},//
	{170,	85 ,255	,255},//
	{170,	170	,85 ,255},//
	{170,	170	,255	,255},//
	{170,	255	,0	,255},//
	{170,	255	,85 ,255},// (50)
	{170,	255	,170	,255},//
	{170,	255	,255	,255},//
	{255,	0	,85 ,255},//
	{255,	0	,170	,255},//
	{255,	85 ,0	,255},//
	{255,	85 ,85 ,255},	//
	{255,	85 ,170	,255},	//
	{255,	85 ,255	,255},	//
	{255,	170	,0	,255},	//
	{255,	170	,85 ,255},	// (60)
	{255,	170	,170	,255},	//
	{255,	170	,255	,255},	//
	{255,	255	,85 ,255},	//
	{255,	255	,170	,255},	//
	{0,		0	,0	,128},	//Black
	{255,	0	,0	,128},	//Red
	{0,		255	,0	,128},	//Green
	{255,	255	,0	,128},	//Yellow
	{0,		0,		255,	128},	//Blue
	{255,	0,		255,	128},	//magenta (70)
	{0,		255,	255,	128},	//Cyan
	{255,	255	,255	,128},	//White
	{170,	0	,0	,128},	//Half brightness Red
	{0,		170	,0	,128},	//Half brightness Green
	{170,	170	,0	,128},	//Half brightness Yellow
	{0,		0	,170	,128},	//Half brightness Blue
	{170,	0	,170	,128},	//Half brightness magenta
	{0,		170	,170	,128},	//Half brightness Cyan
	{170,	170	,170	,128},	//Half brightness White(Gray)
	{0,		0	,85 ,128},	// (80)
	{0,		85 ,0	,128},	//
	{0,		85 	,85 	,128},	//
	{0,		85 	,170	,128},	//
	{0,		85 	,255	,128},	//
	{0,		170	,85 	,128},	//
	{0,		170	,255	,128},	//
	{0,		255	,85 	,128},	//
	{0,		255	,170	,128},//
	{85,	0	,0	,128},//
	{85,	0	,85 	,128},// (90)
	{85,	0	,170	,128},//
	{85,	0	,255	,128},//
	{85,	85 	,0	,128},//
	{85,	85 	,85 	,128},//
	{85,	85 	,170	,128},//
	{85,	85 	,255	,128},//
	{85,	170	,0	,128},//
	{85,	170	,85 	,128},//
	{85,	170	,170	,128},//
	{85,	170	,255	,128},// (100)
	{85,	255	,0	,128},//
	{85,	255	,85 	,128},//
	{85,	255	,170	,128},//
	{85,	255	,255	,128},//
	{170,	0	,85 	,128},//
	{170,	0	,255	,128},//
	{170,	85 	,0	,128},//
	{170,	85 	,85 	,128},//
	{170,	85 	,170	,128},//
	{170,	85 	,255	,128},// (110)
	{170,	170	,85 	,128},//
	{170,	170	,255	,128},//
	{170,	255	,0	,128},//
	{170,	255	,85 	,128},//
	{170,	255	,170	,128},//
	{170,	255	,255	,128},//
	{255,	0	,85 	,128},//
	{255,	0	,170	,128},	//
	{255,	85 	,0	,128},	//
	{255,	85 	,85 ,128},	// (120)
	{255,	85 	,170	,128},//
	{255,	85 	,255	,128},//
	{255,	170	,0	,128},//
	{255,	170	,85 	,128},//
	{255,	170	,170	,128},//
	{255,	170	,255	,128},//
	{255,	255	,85 	,128}	//
};
unsigned int g_CMLA_Table[MAX_COLOR];
#else
static CMLA_TYPE cmla_grp[MAX_COLOR] = {
	//Index value, R,	G, B, Alpha,	Name/Comments
	{0,		0,		0,		255},	//Black (0)
	{255,	0,		0,		255},	//Red
	{0,		255,	0,		255},	//Green
	{255,	255,	0,		255},	//Yellow
	{0,		0,		255,	255},	//Blue
	{255,	0,		255,	255},	//Magenta
	{0,		255,	255,	255},	//Cyan
	{255,	255,	255,	255},	//White
	{0,		0,		0,		0},		//Transparent
	{170,	0,		0,		255},	//Half brightness Red
	{0,		170,	0,		255},	//Half brightness Green (10)
	{170,	170,	0,		255},	//Half brightness Yellow
	{0,		0,		170,	255},	//Half brightness Blue
	{170,	0,		170,	255},	//Half brightness magenta
	{0,		170,	170,	255},	//Half brightness Cyan
	{170,	170,	170,	255},	//Half brightness White(Gray)
	{0,		0,		85,		255},	//
	{0,		85,		0,		255},	//
	{0,		85,		85,		255},	//
	{0,		85,		170,	255},	//
	{0,		85,		255,	255},	// (20)
	{0,		170,	85,		255},	//
	{0,		170,	255,	255},	//
	{0,		255,	85,		255},	//
	{0,		255,	170,	255},	//
	{85,	0,		0,		255},	//
	{85,	0,		85,		255},	//
	{85,	0,		170,	255},	//
	{85,	0,		255,	255},	//
	{85,	85,		0,		255},	//
	{85,	85,		85,		255},	// (30)
	{85,	85,		170,	255},	//
	{85,	85,		255,	255},	//
	{85,	170	,0	,255},//
	{85,	170	,85 ,255},//
	{85,	170	,170	,255},//
	{85,	170	,255	,255},//
	{85,	255	,0	,255},//
	{85,	255	,85 ,255},//
	{85,	255	,170	,255},//
	{85,	255	,255	,255},	// (40)
	{170,	0	,85 ,255},//
	{170,	0	,255	,255},//
	{170,	85 ,0	,255},//
	{170,	85 ,85 ,255},//
	{170,	85 ,170	,255},//
	{170,	85 ,255	,255},//
	{170,	170	,85 ,255},//
	{170,	170	,255	,255},//
	{170,	255	,0	,255},//
	{170,	255	,85 ,255},// (50)
	{170,	255	,170	,255},//
	{170,	255	,255	,255},//
	{255,	0	,85 ,255},//
	{255,	0	,170	,255},//
	{255,	85 ,0	,255},//
	{255,	85 ,85 ,255},	//
	{255,	85 ,170	,255},	//
	{255,	85 ,255	,255},	//
	{255,	170	,0	,255},	//
	{255,	170	,85 ,255},	// (60)
	{255,	170	,170	,255},	//
	{255,	170	,255	,255},	//
	{255,	255	,85 ,255},	//
	{255,	255	,170	,255},	//
	{0,		0	,0	,128},	//Black
	{255,	0	,0	,128},	//Red
	{0,		255	,0	,128},	//Green
	{255,	255	,0	,128},	//Yellow
	{0,		0,		255,	128},	//Blue
	{255,	0,		255,	128},	//magenta (70)
	{0,		255,	255,	128},	//Cyan
	{255,	255	,255	,128},	//White
	{170,	0	,0	,128},	//Half brightness Red
	{0,		170	,0	,128},	//Half brightness Green
	{170,	170	,0	,128},	//Half brightness Yellow
	{0,		0	,170	,128},	//Half brightness Blue
	{170,	0	,170	,128},	//Half brightness magenta
	{0,		170	,170	,128},	//Half brightness Cyan
	{170,	170	,170	,128},	//Half brightness White(Gray)
	{0,		0	,85 ,128},	// (80)
	{0,		85 ,0	,128},	//
	{0,		85 	,85 	,128},	//
	{0,		85 	,170	,128},	//
	{0,		85 	,255	,128},	//
	{0,		170	,85 	,128},	//
	{0,		170	,255	,128},	//
	{0,		255	,85 	,128},	//
	{0,		255	,170	,128},//
	{85,	0	,0	,128},//
	{85,	0	,85 	,128},// (90)
	{85,	0	,170	,128},//
	{85,	0	,255	,128},//
	{85,	85 	,0	,128},//
	{85,	85 	,85 	,128},//
	{85,	85 	,170	,128},//
	{85,	85 	,255	,128},//
	{85,	170	,0	,128},//
	{85,	170	,85 	,128},//
	{85,	170	,170	,128},//
	{85,	170	,255	,128},// (100)
	{85,	255	,0	,128},//
	{85,	255	,85 	,128},//
	{85,	255	,170	,128},//
	{85,	255	,255	,128},//
	{170,	0	,85 	,128},//
	{170,	0	,255	,128},//
	{170,	85 	,0	,128},//
	{170,	85 	,85 	,128},//
	{170,	85 	,170	,128},//
	{170,	85 	,255	,128},// (110)
	{170,	170	,85 	,128},//
	{170,	170	,255	,128},//
	{170,	255	,0	,128},//
	{170,	255	,85 	,128},//
	{170,	255	,170	,128},//
	{170,	255	,255	,128},//
	{255,	0	,85 	,128},//
	{255,	0	,170	,128},	//
	{255,	85 	,0	,128},	//
	{255,	85 	,85 ,128},	// (120)
	{255,	85 	,170	,128},//
	{255,	85 	,255	,128},//
	{255,	170	,0	,128},//
	{255,	170	,85 	,128},//
	{255,	170	,170	,128},//
	{255,	170	,255	,128},//
	{255,	255	,85 	,128}	//
};
unsigned int g_CMLA_Table[MAX_COLOR];


static CMLA_TYPE png_cmla_grp[MAX_COLOR] = {
	//Index value, R,	G, B, Alpha,	Name/Comments
	{0,		0,		0,		255},	//Black (0)
	{255,	0,		0,		255},	//Red
	{0,		255,	0,		255},	//Green
	{255,	255,	0,		255},	//Yellow
	{0,		0,		255,	255},	//Blue
	{255,	0,		255,	255},	//Magenta
	{0,		255,	255,	255},	//Cyan
	{255,	255,	255,	255},	//White
	{0,		0,		0,		0},		//Transparent
	{170,	0,		0,		255},	//Half brightness Red
	{0,		170,	0,		255},	//Half brightness Green (10)
	{170,	170,	0,		255},	//Half brightness Yellow
	{0,		0,		170,	255},	//Half brightness Blue
	{170,	0,		170,	255},	//Half brightness magenta
	{0,		170,	170,	255},	//Half brightness Cyan
	{170,	170,	170,	255},	//Half brightness White(Gray)
	{0,		0,		85,		255},	//
	{0,		85,		0,		255},	//
	{0,		85,		85,		255},	//
	{0,		85,		170,	255},	//
	{0,		85,		255,	255},	// (20)
	{0,		170,	85,		255},	//
	{0,		170,	255,	255},	//
	{0,		255,	85,		255},	//
	{0,		255,	170,	255},	//
	{85,	0,		0,		255},	//
	{85,	0,		85,		255},	//
	{85,	0,		170,	255},	//
	{85,	0,		255,	255},	//
	{85,	85,		0,		255},	//
	{85,	85,		85,		255},	// (30)
	{85,	85,		170,	255},	//
	{85,	85,		255,	255},	//
	{85,	170	,0	,255},//
	{85,	170	,85 ,255},//
	{85,	170	,170	,255},//
	{85,	170	,255	,255},//
	{85,	255	,0	,255},//
	{85,	255	,85 ,255},//
	{85,	255	,170	,255},//
	{85,	255	,255	,255},	// (40)
	{170,	0	,85 ,255},//
	{170,	0	,255	,255},//
	{170,	85 ,0	,255},//
	{170,	85 ,85 ,255},//
	{170,	85 ,170	,255},//
	{170,	85 ,255	,255},//
	{170,	170	,85 ,255},//
	{170,	170	,255	,255},//
	{170,	255	,0	,255},//
	{170,	255	,85 ,255},// (50)
	{170,	255	,170	,255},//
	{170,	255	,255	,255},//
	{255,	0	,85 ,255},//
	{255,	0	,170	,255},//
	{255,	85 ,0	,255},//
	{255,	85 ,85 ,255},	//
	{255,	85 ,170	,255},	//
	{255,	85 ,255	,255},	//
	{255,	170	,0	,255},	//
	{255,	170	,85 ,255},	// (60)
	{255,	170	,170	,255},	//
	{255,	170	,255	,255},	//
	{255,	255	,85 ,255},	//
	{255,	255	,170	,255},	//
	{0,		0	,0	,195},	//Black
	{255,	0	,0	,195},	//Red
	{0,		255	,0	,195},	//Green
	{255,	255	,0	,195},	//Yellow
	{0,		0,		255,	195},	//Blue
	{255,	0,		255,	195},	//magenta (70)
	{0,		255,	255,	195},	//Cyan
	{255,	255	,255	,195},	//White
	{170,	0	,0	,195},	//Half brightness Red
	{0,		170	,0	,195},	//Half brightness Green
	{170,	170	,0	,195},	//Half brightness Yellow
	{0,		0	,170	,195},	//Half brightness Blue
	{170,	0	,170	,195},	//Half brightness magenta
	{0,		170	,170	,195},	//Half brightness Cyan
	{170,	170	,170	,195},	//Half brightness White(Gray)
	{0,		0	,85 ,195},	// (80)
	{0,		85 ,0	,195},	//
	{0,		85 	,85 	,195},	//
	{0,		85 	,170	,195},	//
	{0,		85 	,255	,195},	//
	{0,		170	,85 	,195},	//
	{0,		170	,255	,195},	//
	{0,		255	,85 	,195},	//
	{0,		255	,170	,195},//
	{85,	0	,0	,195},//
	{85,	0	,85 	,195},// (90)
	{85,	0	,170	,195},//
	{85,	0	,255	,195},//
	{85,	85 	,0	,195},//
	{85,	85 	,85 	,195},//
	{85,	85 	,170	,195},//
	{85,	85 	,255	,195},//
	{85,	170	,0	,195},//
	{85,	170	,85 	,195},//
	{85,	170	,170	,195},//
	{85,	170	,255	,195},// (100)
	{85,	255	,0	,195},//
	{85,	255	,85 	,195},//
	{85,	255	,170	,195},//
	{85,	255	,255	,195},//
	{170,	0	,85 	,195},//
	{170,	0	,255	,195},//
	{170,	85 	,0	,195},//
	{170,	85 	,85 	,195},//
	{170,	85 	,170	,195},//
	{170,	85 	,255	,195},// (110)
	{170,	170	,85 	,195},//
	{170,	170	,255	,195},//
	{170,	255	,0	,195},//
	{170,	255	,85 	,195},//
	{170,	255	,170	,195},//
	{170,	255	,255	,195},//
	{255,	0	,85 	,195},//
	{255,	0	,170	,195},	//
	{255,	85 	,0	,195},	//
	{255,	85 	,85 ,195},	// (120)
	{255,	85 	,170	,195},//
	{255,	85 	,255	,195},//
	{255,	170	,0	,195},//
	{255,	170	,85 	,195},//
	{255,	170	,170	,195},//
	{255,	170	,255	,195},//
	{255,	255	,85 	,195}	//
};
unsigned int g_png_CMLA_Table[MAX_COLOR];

#endif
/* code mapping table moved to middleware/lib_isdbt/src/font/src/isdbt_font.c */
extern const unsigned short s_KanjiSet[UNIPAGE_CODENUM];
extern const unsigned short s_AddedSymbolsOfKanjiSet[10 * 94];
#if !defined(USE_EUCJPMAP_FOR_LATINEXT_CODEMAPPING)
extern const unsigned short s_LatinExtensionSet[94];	/* Latin Extension */
#endif
#if !defined(USE_EUCJPMAP_FOR_SPECIAL_CODEMAPPING)
extern const unsigned short s_SpecialSet[94];	/* Special */
#endif

/**************************************************************************
*  FUNCTION NAME :
*	   static int ISDBT_Caption_jisx0208_mbtowc()
*
*  DESCRIPTION :
*
*  INPUT:
*  OUTPUT:
*  REMARK  :
**************************************************************************/
static int ISDBT_Caption_jisx0208_mbtowc(const unsigned char *s, unsigned int *pUCSCode)
{
	unsigned int unicode = 0x0000;
	unsigned char c1 = s[0];

	if ((c1 >= 0x21) && (c1 <= 0x74))
	{
		unsigned char c2 = s[1];

		if (c2 >= 0x21 && c2 < 0x7f)
		{
			unsigned int i = 94 * (c1 - 0x21) + (c2 - 0x21);

			if (i < UNIPAGE_CODENUM)
			{
				unicode = (unsigned int)s_KanjiSet[i];
				if (unicode == 0x0000)
				{
					return -1;
				}

				*pUCSCode = unicode;

				return 1;
			}
		}
	}

	return -1;
}

/**************************************************************************
*  FUNCTION NAME :
*	   static unsigned short ISDBT_EPG_jisx0208_mbtowc()
*
*  DESCRIPTION :
*
*  INPUT:
*  OUTPUT:
*  REMARK  :
**************************************************************************/
static unsigned short ISDBT_EPG_jisx0208_mbtowc(unsigned char ucFirstByte, unsigned char ucSecondByte)
{
	ucFirstByte &=	0x7f;
	ucSecondByte &=  0x7f;

	/* if(FirstByte == 0x29) Do_MapUnicodeOfLatinExt */
	if ((ucFirstByte >= 0x21 && ucFirstByte <= (0x28 + 0x01)) || (ucFirstByte >= 0x30 && ucFirstByte <= 0x74))
	{
		if (ucSecondByte >= 0x21 && ucSecondByte < 0x7f)
		{
			unsigned int i = 94 * (ucFirstByte - 0x21) + (ucSecondByte - 0x21);

			if (i < UNIPAGE_CODENUM)
			{
				return (unsigned short)s_KanjiSet[i];
			}
		}
	}

	return 0;
}


static const CODESET_MAPPER s_CodeSetMap[ISDBT_CODESET_MAX] =
{
	{0x42, KANJI_SET},		{0x4A, ALPHANUMERIC_SET},
	{0x30, HIRAGANA_SET},	{0x31, KATAKANA_SET},
	{0x4B, LATIN_EXT_SET}
};

static ISDBT_CHAR_SET ISDBT_ExtractCodeSet(unsigned char ucTerminationCode)
{
	int i;

	for(i=0; i<ISDBT_CODESET_MAX; i++)
	{
		if(ucTerminationCode == s_CodeSetMap[i].terminationCode)
			return s_CodeSetMap[i].codeSet;
	}

	return KANJI_SET; // default code set
}

static int ConvertCode_Kanji2EucJP(unsigned char *buf1, unsigned char *buf2)
{
	*buf2 = (*buf1) | 0x80;
	*(buf2+1) = (*(buf1+1)) | 0x80;

	return 2;
}

static int	ConvertCode_Alpha2EucJP(unsigned char *buf1, unsigned char *buf2)
{
	*buf2 = FIRST_BYTE_OF_ALPHA;
	*(buf2+1) = *buf1 | 0x80;
	if (*(buf2+1) == (0x5C | 0x80)) *(buf2+1) = 0xA5 | 0x80;	/* reverse solidus -> yen */
	if (*(buf2+1) == (0x7E | 0x80)) *(buf2+1) = 0xAF | 0x80;	/* tilde -> macron */
	return 2;
}

static int	ConvertCode_Alpha2EucJP_MSZ(unsigned char *buf1, unsigned char *buf2)
{
	*buf2 = *buf1& 0x7F;
	if (*buf2 == 0x5C) *buf2 = 0xA5;	/* reverse solidus -> yen */
	if (*buf2 == 0x7E) *buf2 = 0xAF;	/* tilde -> macron */
	return 1;
}

static int	ConvertCode_Hira2EucJP(unsigned char *buf1, unsigned char *buf2)
{
	*buf2 = FIRST_BYTE_OF_HIRAGANA;
	*(buf2+1) = *buf1 | 0x80;

	return 2;
}

static int	ConvertCode_Kata2EucJP(unsigned char *buf1, unsigned char *buf2)
{
	*buf2 = FIRST_BYTE_OF_KATAKANA;
	*(buf2+1) = *buf1 | 0x80;

	return 2;
}

static int	ConvertCode_LatinExt2EucJP(unsigned char *buf1, unsigned char *buf2)
{
#ifdef USE_EUCJPMAP_FOR_LATINEXT_CODEMAPPING
	*buf2 = FIRST_BYTE_OF_LATINEXTENSION;
	*(buf2+1) = *buf1 | 0x80;
#else
	/* TBD */
#endif

	return 2;
}

const JPN_CODESET_CMD JPNCodeSetCmd[ISDBT_CODESET_MAX] =
{
	{ KANJI_SET,			ConvertCode_Kanji2EucJP },
	{ ALPHANUMERIC_SET, 	ConvertCode_Alpha2EucJP },
	{ HIRAGANA_SET, 		ConvertCode_Hira2EucJP		},
	{ KATAKANA_SET, 	ConvertCode_Kata2EucJP		},
	{ LATIN_EXT_SET,		ConvertCode_LatinExt2EucJP	}
};

static int ISDBT_ConvCodeByCodeSet(unsigned char *pOutBuf, unsigned char *pInBuf, ISDBT_CHAR_SET codeSet)
{
	int i;

	for(i=0; i<ISDBT_CODESET_MAX; i++)
	{
		if(codeSet == JPNCodeSetCmd[i].codeSet)
			return (*JPNCodeSetCmd[i].CodeConv)(pInBuf, pOutBuf);
	}
	return 0;
}

static unsigned short ISDBT_EPG_aribAddKanji_mbtowc(unsigned char ucFirstByte, unsigned char ucSecondByte)
{
	unsigned short usRetChar = 0;

	int hi = (ucFirstByte & 0xff) - (0xf5 & 0xff);
	int low = (ucSecondByte & 0xff) - (0xa1 & 0xff);

	usRetChar = s_AddedSymbolsOfKanjiSet[hi*94 + low];

	return usRetChar;
}

int ISDBT_EPG_DecodeCharString( unsigned char *pInBuf, unsigned char *pOutBuf, int nLen, unsigned char ucLangCode)
{
	int i, iOffset = 0;

	ISDBT_CHAR_SET G0, G1, G2, G3;
	ISDBT_CHAR_SET GL, GR;

	unsigned char		fFontSize;

	int iGLReadByte = 1;

	// Added by Noah / 2017071X / To process "Single shift"
	unsigned int isSetSingleShift = 0;                        // 0 is 'not set', 1 is 'set'
	ISDBT_CHAR_SET enBackup_GL_ForSingleShift = KANJI_SET;    // Initial value( KANJI_SET ) is not important.
	int iBackup_iGLReadByte_ForSingleShift = iGLReadByte;

	if (ucLangCode == 0x08) 	// 0x08: UI_LANG_JAPANESE (ui_def.h)
	{
		G0 = KANJI_SET;
		G1 = ALPHANUMERIC_SET;
		G2 = HIRAGANA_SET;
		G3 = KATAKANA_SET;

		GL = G0;
		GR = G2;

		fFontSize = NSZ;
	}
	else
	{
		G0 = ALPHANUMERIC_SET;
		G1 = ALPHANUMERIC_SET;
		G2 = LATIN_EXT_SET;
		G3 = LATIN_EXT_SET;

		GL = G0;
		GR = G2;

		fFontSize = MSZ;
	}

	memset(pOutBuf, '\0', MAX_EPG_CONV_TXT_BYTE);

	for (i=0; i<nLen; i++)
	{
		switch(pInBuf[i])
		{
			case MSZ:    // 0x89
				fFontSize = MSZ;
			break;

			case NSZ:    // 0x8A
				fFontSize = NSZ;
			break;

			case LS1:    // 0x0E
				DO_LS1;

				/* Added by Noah / 2017071X / IM478A-63 / The invalid character of EPG / J&K / TV_tokyo_23ch_20170706.ts / Problem Case 2 / EventID 10444
					'DO_LS1' makes that 'iGLReadByte' is 0.
					However, KANJI_SET is "2-byte G set", so 'iGLReadByte' must be set to 1.
					FYI, if "1-byte G set", 'iGLReadByte' is 0.
					The following if statement is not real solution.
					But, in order to get the real solution, many codes should be modified.
					To avoid any side effects, I focused on the issue and added only the following if statement.
				*/
				if(KANJI_SET == G1)
				{
					iGLReadByte = 1;
				}
			break;

			case LS0:    // 0x0F
				DO_LS0;
			break;

			case APR:    // 0x0D
				DO_APR;
			break;

			case SP:    // 0x20
				if(fFontSize == MSZ)
				{
					*(pOutBuf + iOffset) = MSZ;
					*(pOutBuf + iOffset + 1) = SP;
					iOffset += 2;
				}
				else
				{
					DO_SP;
				}
			break;

			case SS2:    // 0x19, G2 to GL, Single shift !!!!!
#if 0    // Original codes
				if(i+1 > nLen)
					return iOffset;

				iOffset += ISDBT_ConvCodeByCodeSet(pOutBuf+iOffset, pInBuf+i+1, G2);
				NEXTINDEX1;
#else	// Added by Noah / 2017071X / IM478A-63 / The invalid character of EPG / J&K / TV_tokyo_23ch_20170706.ts / Problem Case 1 / EventID 10636
				isSetSingleShift = 1;
				enBackup_GL_ForSingleShift = GL;
				iBackup_iGLReadByte_ForSingleShift = iGLReadByte;

				SET_GLG2;

				if(KANJI_SET == G2)    // Problem case 4 / 10442
				{
					iGLReadByte = 1;
				}
				else                   // Just in case
				{
					iGLReadByte = 0;
				}
#endif
			break;

			case SS3:    // 0x1D, G3 to GL, Single shift !!!!!
#if 0    // Original codes
				if(i+1 > nLen)
					return iOffset;

				iOffset += ISDBT_ConvCodeByCodeSet(pOutBuf + iOffset, pInBuf + i + 1, G3);
				NEXTINDEX1;
#else	// Added by Noah / 2017071X / IM478A-63 / The invalid character of EPG / J&K / TV_tokyo_23ch_20170706.ts / Problem Case 3 / EventID 10580
				isSetSingleShift = 1;
				enBackup_GL_ForSingleShift = GL;
				iBackup_iGLReadByte_ForSingleShift = iGLReadByte;

				SET_GLG3;

				if(KANJI_SET == G3)    // Just in case
				{
					iGLReadByte = 1;
				}
				else                   // Just in case
				{
					iGLReadByte = 0;
				}
#endif
			break;

			case ESC:    // 0x1B
				{
					if(i+1 > nLen)
						return iOffset;

					switch(pInBuf[i+1])
					{
						case LS2:    // 0x6E
							DO_LS2;
						break;

						case LS3:    // 0x6F
							DO_LS3;
						break;

						case LS1R:    // 7E
							DO_LS1R;
						break;

						case LS2R:    // 7D
							DO_LS2R;
						break;

						case LS3R:    // 7C
							DO_LS3R;
						break;

						case 0x28:
							if(i+2 > nLen)
								return iOffset;

							G0 = ISDBT_ExtractCodeSet(pInBuf[i+2]);
							NEXTINDEX2;
						break;

						case 0x29:
							if(i+2 > nLen)
								return iOffset;

							G1 = ISDBT_ExtractCodeSet(pInBuf[i+2]);
							NEXTINDEX2;
						break;

						case 0x2A:
							if(i+2 > nLen)
								return iOffset;

							G2 = ISDBT_ExtractCodeSet(pInBuf[i+2]);
							NEXTINDEX2;
						break;

						case 0x2B:
							if(i+2 > nLen)
								return iOffset;

							G3 = ISDBT_ExtractCodeSet(pInBuf[i+2]);
							NEXTINDEX2;
						break;

						case 0x24:
							if(i+2 > nLen)
								return iOffset;

							switch(pInBuf[i+2])
							{
								case 0x29:
									G1 = ISDBT_ExtractCodeSet(pInBuf[i+3]);
									NEXTINDEX3;
								break;

								case 0x2A:
									G2 = ISDBT_ExtractCodeSet(pInBuf[i+3]);
									NEXTINDEX3;
								break;

								case 0x2B:
									G3 = ISDBT_ExtractCodeSet(pInBuf[i+3]);
									NEXTINDEX3;
								break;

								default:
									G0 = ISDBT_ExtractCodeSet(pInBuf[i+2]);
									NEXTINDEX2;
								break;
							}
						break;

						default:
							return iOffset;
					}
				}
			break;

			default:
				if (0x20 < pInBuf[i] && pInBuf[i] < 0x7F)
				{
					if(fFontSize == MSZ)
					{
						if (GL == ALPHANUMERIC_SET)
						{
							iOffset += ConvertCode_Alpha2EucJP_MSZ(pInBuf + i, pOutBuf + iOffset);

							// Added by Noah / 2017071X / IM478A-63
							if (1 == isSetSingleShift)
							{
								isSetSingleShift = 0;
								GL = enBackup_GL_ForSingleShift;
								iGLReadByte = iBackup_iGLReadByte_ForSingleShift;
							}
						}
						else if (GL == KANJI_SET)
						{
							if (pInBuf[i] == 0x21)
							{
								if(i+1 > nLen)
									return iOffset;

								if(pInBuf[i+1] == 0x21)
								{
									*(pOutBuf + iOffset) = SP;
									iOffset += 1;
									NEXTINDEX1;

									// Added by Noah / 2017071X / IM478A-63
									if (1 == isSetSingleShift)
									{
										isSetSingleShift = 0;
										GL = enBackup_GL_ForSingleShift;
										iGLReadByte = iBackup_iGLReadByte_ForSingleShift;
									}
								}
							}
						}
					}
					else
					{
						iOffset += ISDBT_ConvCodeByCodeSet(pOutBuf + iOffset, pInBuf + i, GL);
						NEXTGLINDEX;

						// Added by Noah / 2017071X / IM478A-63
						if (1 == isSetSingleShift)
						{
							isSetSingleShift = 0;
							GL = enBackup_GL_ForSingleShift;
							iGLReadByte = iBackup_iGLReadByte_ForSingleShift;
						}
					}
				}
				else if (0xA0 < pInBuf[i] && pInBuf[i] < 0xFF)
				{
					if(fFontSize == MSZ && GR == ALPHANUMERIC_SET)
					{
						iOffset += ConvertCode_Alpha2EucJP_MSZ(pInBuf + i, pOutBuf + iOffset);

						// Added by Noah / 2017071X / IM478A-63
						if (1 == isSetSingleShift)
						{
							isSetSingleShift = 0;
							GL = enBackup_GL_ForSingleShift;
							iGLReadByte = iBackup_iGLReadByte_ForSingleShift;
						}
					}
					else
					{
						iOffset += ISDBT_ConvCodeByCodeSet(pOutBuf + iOffset, pInBuf + i, GR);

						// Added by Noah / 2017071X / IM478A-63
						if (1 == isSetSingleShift)
						{
							isSetSingleShift = 0;
							GL = enBackup_GL_ForSingleShift;
							iGLReadByte = iBackup_iGLReadByte_ForSingleShift;
						}
					}

					if(GR == KANJI_SET)
						NEXTINDEX1;
				}
			break;
		}
	}

	return iOffset;
}

int ISDBT_EPG_Convert_Unicode(
	const unsigned char*	pucInStr,
	int 					iInIdx,
	unsigned short* 		pusUniStr,
	int 					iOutByteNum
)
{
	if (pucInStr[iInIdx] < 0x80)		// detect ASCII code
	{
		pusUniStr[iOutByteNum] = (unsigned short) pucInStr[iInIdx];
		return 0;
	}
	else if (pucInStr[iInIdx] == 0x8e)	// detect JIS X 0201 code
	{
		unsigned char tmpChar = 0;
		if (pucInStr[iInIdx + 1] >= 0xa1 && pucInStr[iInIdx + 1] <= 0xdf)
			tmpChar = pucInStr[iInIdx + 1] - 0x40;

		pusUniStr[iOutByteNum] = (unsigned short) ((tmpChar<<8) | 0xff);

		return 1;
	}
	else if (pucInStr[iInIdx] == 0x8f)	// detect JIS X 0212 code
	{
		//PRINTF("[ISDBT_EPG_Convert_Unicode] detect JIS X 0212 code : 0x8f, 0x%x, 0x%x\n",
											//pucInStr[iInIdx+1], pucInStr[iInIdx+2]);
		pusUniStr[iOutByteNum] = 0;

		return 2;
	}
	else if ((pucInStr[iInIdx] >= 0xf5) && (pucInStr[iInIdx] <= 0xfe))
	{ // detect code of user-defined range (additional synbal/kanji)
		unsigned short tmpChar = 0;

		if (pucInStr[iInIdx + 1] >= 0xa1 && pucInStr[iInIdx + 1] <= 0xfe)
		{
			tmpChar = ISDBT_EPG_aribAddKanji_mbtowc(pucInStr[iInIdx], pucInStr[iInIdx+1]);

			pusUniStr[iOutByteNum] = tmpChar;
		}
		else
			//PRINTF("[ISDBT_EPG_Convert_Unicode] undefined additional character : 0x%x, 0x%x\n",
												//pucInStr[iInIdx], pucInStr[iInIdx + 1]);

		return 1;
	}
	else // detect JIX X 0208 code
	{
		unsigned short tmpChar = 0;

		tmpChar = ISDBT_EPG_jisx0208_mbtowc(pucInStr[iInIdx], pucInStr[iInIdx+1]);
		pusUniStr[iOutByteNum] = tmpChar;

		return 1;
	}

	return 1; // default
}

/**************************************************************************
*  FUNCTION NAME :
*	   unsigned short ISDBT_EPG_ConvertCharString()
*
*  DESCRIPTION :
*  INPUT:
*			const unsigned char* - outstring_buf
*			int - usOutStringSize
*			unsigned short* - unicode_buf
*
*  OUTPUT:	unsigned short - usUniStringSize
*  REMARK  :
**************************************************************************/
unsigned short	ISDBT_EPG_ConvertCharString
(
	unsigned char * 	inbuf,	// euc-jp code buffer
	unsigned short	inlen,	// euc-jp code length
	unsigned short *	outbuf	// unicode buffer
)
{
	int j;
	int iRet;
	unsigned short outlen = 0;

	memset(outbuf, '\0', MAX_EPG_CONV_TXT_BYTE);

	for (j = 0; j < inlen; j++)
	{
		if(*(inbuf+j) == MSZ)
		{
			continue;
		}

		iRet = ISDBT_EPG_Convert_Unicode(inbuf, j, outbuf, outlen);

		j += iRet;
		outlen++;
	}

	return outlen;
}

/**************************************************************************
*  FUNCTION NAME :
*	   ISDBT_EPG_Convert_2ByteCode
*
*  DESCRIPTION :
*
*  INPUT:
*  OUTPUT:
*  REMARK  :
**************************************************************************/
int ISDBT_EPG_Convert_2ByteCode(const unsigned char *pSrcStr, int iSrcStrIdx, unsigned short *pDstStr, int iDstStrIdx)
{
	int iUsedIndex = 0;

	/* ASCII code */
	if (pSrcStr[iSrcStrIdx] < 0x80)
	{
		pDstStr[iDstStrIdx] = (unsigned short) pSrcStr[iSrcStrIdx];

		iUsedIndex = 0;
	}
	/* JIS X 0201 code */
	else if (pSrcStr[iSrcStrIdx] == 0x8E)
	{
#if (0)
		unsigned char tmpChar = 0;

		if ((pSrcStr[iSrcStrIdx + 1] >= 0xA1) && (pSrcStr[iSrcStrIdx + 1] <= 0xDF))
		{
			tmpChar = pSrcStr[iSrcStrIdx + 1] - 0x40;
		}

		pDstStr[iDstStrIdx] = (unsigned short) ((tmpChar << 8) | 0xFF);
#else
		pDstStr[iDstStrIdx] = 0xFFFE;
#endif

		iUsedIndex = 1;
	}
	/* JIS X 0212 code */
	else if (pSrcStr[iSrcStrIdx] == 0x8F)
	{
#if (0)
		PRINTF("[ISDBT_EPG_Convert_Unicode] detect JIS X 0212 code : 0x8f, 0x%x, 0x%x\n",
											pSrcStr[iSrcStrIdx+1], pSrcStr[iSrcStrIdx+2]);
#endif

		pDstStr[iDstStrIdx] = 0xFFFF;

		iUsedIndex = 2;
	}
#if (1)
	else
	{
		pDstStr[iDstStrIdx] = ((pSrcStr[iSrcStrIdx + 1] & 0xFF) << 8) | (pSrcStr[iSrcStrIdx] & 0xFF);

		iUsedIndex = 1;
	}
#else
	/* code of user-defined range (additional synbal/kanji) */
	else if ((pSrcStr[iSrcStrIdx] >= 0xf5) && (pSrcStr[iSrcStrIdx] <= 0xfe))
	{
		unsigned short tmpChar = 0;

		if (pSrcStr[iSrcStrIdx + 1] >= 0xa1 && pSrcStr[iSrcStrIdx + 1] <= 0xfe)
		{
			tmpChar = ISDBT_EPG_aribAddKanji_mbtowc(pSrcStr[iSrcStrIdx], pSrcStr[iSrcStrIdx+1]);

			pDstStr[iDstStrIdx] = tmpChar;
		}
		else
		{
			//PRINTF("[ISDBT_EPG_Convert_Unicode] undefined additional character : 0x%x, 0x%x\n",
												//pSrcStr[iSrcStrIdx], pSrcStr[iSrcStrIdx + 1]);
		}

		iUsedIndex = 1;
	}
	/* JIX X 0208 code */
	else
	{
		unsigned short tmpChar = 0;

		tmpChar = ISDBT_EPG_jisx0208_mbtowc(pSrcStr[iSrcStrIdx], pSrcStr[iSrcStrIdx+1]);
		pDstStr[iDstStrIdx] = tmpChar;

		iUsedIndex = 1;
	}
#endif

	return iUsedIndex;
}

/**************************************************************************
*  FUNCTION NAME :
*	   ISDBT_EPG_Adjust_CharString
*
*  DESCRIPTION :
*  INPUT:
*			const unsigned char* - outstring_buf
*			int - usOutStringSize
*			unsigned short* - unicode_buf
*
*  OUTPUT:	unsigned short - usUniStringSize
*  REMARK  :
**************************************************************************/
unsigned short ISDBT_EPG_Adjust_CharString
(
	unsigned char	*inbuf, 	// euc-jp code buffer
	unsigned short	inlen,		// euc-jp code length
	unsigned short	*outbuf 	// unicode buffer
)
{
	int j;
	int iRet;
	unsigned short outlen = 0;

	memset(outbuf, '\0', MAX_EPG_CONV_TXT_BYTE);

	for (j = 0; j < inlen; j++)
	{
		if (*(inbuf+j) == MSZ)
		{
			continue;
		}

		iRet = ISDBT_EPG_Convert_2ByteCode(inbuf, j, outbuf, outlen);

		j += iRet;
		outlen++;
	}

	return outlen;
}

int ISDBT_CC_Is_NonSpace(unsigned char *pInCharCode)
{
	int i;
	unsigned short *p = NULL;
	unsigned short  non_space_char[7] = {0x2d21, 0x2e21, 0x2f21, 0x3021, 0x3121, 0x3221, 0x7e22};

	if(pInCharCode == NULL)
		return 0;

	p = (unsigned short*)pInCharCode;

	for (i = 0; i < 7; i++) {
		if (*p == non_space_char[i]) {
			return 1;
		}
	}

	return 0;
}

int ISDBT_CC_Is_RuledLineChar(unsigned int iCharData)
{
	int i;
	unsigned int ruled_line_char[32] = {0x2500, 0x2502, 0x250c, 0x2510, 0x2518, 0x2514, 0x251c, 0x252c,
											0x2524, 0x2534, 0x253c, 0x2501, 0x2503, 0x250f, 0x2513, 0x251b,
											0x2517, 0x2523, 0x2533, 0x252b, 0x253b, 0x254b, 0x2520, 0x252f,
											0x2528, 0x2537, 0x253f, 0x251d, 0x2530, 0x2525, 0x2538, 0x2542};
	for (i = 0; i <32 ; i++) {
		if (iCharData == ruled_line_char[i]) {
			return 1;
		}
	}
	return 0;
}

#if (1) // NEW_1SEG_CLOSED_CAPTION_CONVERTER
/**************************************************************************
*  FUNCTION NAME :
*	   ISDBT_CC_Convert_Kanji2Unicode
*
*  DESCRIPTION :
*	   ...
*
*  INPUT :
*	   ...
*
*  OUTPUT :
*	   ...
*
*  REMARK :
*	   None
**************************************************************************/
int ISDBT_CC_Convert_Kanji2Unicode(const unsigned char *pInCharCode, unsigned int *pOutUCSCode)
{
	unsigned char ucFirstCode;
	unsigned char ucSecondCode;

	/* Get the EUC-JP code. */
	ucFirstCode = pInCharCode[0] & 0x7F;
	ucSecondCode = pInCharCode[1] & 0x7F;

	/* Check the 1st code. */
	if ( ((0x21 <= ucFirstCode) && (ucFirstCode <= 0x7E)) &&
		 ((0x21 <= ucSecondCode) && (ucSecondCode <= 0x7E)) )
	{
#ifdef USE_EUCJPMAP_FOR_LATINEXT_CODEMAPPING
		if (ucFirstCode == 0x29)
		{
			return -1;
		}
		else
#endif
#ifdef USE_EUCJPMAP_FOR_SPECIAL_CODEMAPPING
		if (ucFirstCode == 0x2A)
		{
			return -1;
		}
		else
#endif
		if (ucFirstCode < 0x75)
		{
			unsigned char buf[2];

			buf[0] = ucFirstCode;
			buf[1] = ucSecondCode;

			return ISDBT_Caption_jisx0208_mbtowc(buf, pOutUCSCode);
		}
		else
		{
			int idx;
			unsigned int unicode = 0x0000;

			idx = 94 * (ucFirstCode - 0x21 - 0x54) + (ucSecondCode - 0x21);
			if ((0 < idx) && (idx < (10 * 94)))
			{
				unicode = (unsigned int)s_AddedSymbolsOfKanjiSet[idx];
				if (unicode == 0x0000)
				{
					return -1;
				}

				*pOutUCSCode = unicode;

				return 1;
			}
		}
	}

	return 0;
}


/**************************************************************************
*  FUNCTION NAME :
*	   ISDBT_CC_Convert_Hiragana2Unicode
*
*  DESCRIPTION :
*	   ...
*
*  INPUT :
*	   ...
*
*  OUTPUT :
*	   ...
*
*  REMARK :
*	   None
**************************************************************************/
int ISDBT_CC_Convert_Hiragana2Unicode(const unsigned char *pInCharCode, unsigned int *pOutUCSCode)
{
	unsigned char ucFirstCode;
	unsigned char ucSecondCode;

	/* Get the EUC-JP code. */
	ucFirstCode = FIRST_BYTE_OF_HIRAGANA & 0x7F;
	ucSecondCode = pInCharCode[0] & 0x7F;

	/* Check the 1st code. */
	if ((0x21 <= ucSecondCode) && (ucSecondCode <= 0x7E))
	{
		unsigned char buf[2];

		buf[0] = ucFirstCode;
		buf[1] = ucSecondCode;

		return ISDBT_Caption_jisx0208_mbtowc(buf, pOutUCSCode);
	}

	return 0;
}


/**************************************************************************
*  FUNCTION NAME :
*	   ISDBT_CC_Convert_Katakana2Unicode
*
*  DESCRIPTION :
*	   ...
*
*  INPUT :
*	   ...
*
*  OUTPUT :
*	   ...
*
*  REMARK :
*	   None
**************************************************************************/
int ISDBT_CC_Convert_Katakana2Unicode(const unsigned char *pInCharCode, unsigned int *pOutUCSCode)
{
	unsigned char ucFirstCode;
	unsigned char ucSecondCode;

	/* Get the EUC-JP code. */
	ucFirstCode = FIRST_BYTE_OF_KATAKANA & 0x7F;
	ucSecondCode = pInCharCode[0] & 0x7F;

	/* Check the 1st code. */
	if ((0x21 <= ucSecondCode) && (ucSecondCode <= 0x7E))
	{
		unsigned char buf[2];

		buf[0] = ucFirstCode;
		buf[1] = ucSecondCode;

		return ISDBT_Caption_jisx0208_mbtowc(buf, pOutUCSCode);
	}

	return 0;
}


/**************************************************************************
*  FUNCTION NAME :
*	   ISDBT_CC_Convert_LatinExtension2Unicode
*
*  DESCRIPTION :
*	   ...
*
*  INPUT :
*	   ...
*
*  OUTPUT :
*	   ...
*
*  REMARK :
*	   None
**************************************************************************/
int ISDBT_CC_Convert_LatinExtension2Unicode(const unsigned char *pInCharCode, unsigned int *pOutUCSCode)
{
#ifdef USE_EUCJPMAP_FOR_LATINEXT_CODEMAPPING
	unsigned char ucFirstCode;
	unsigned char ucSecondCode;

	/* Get the EUC-JP code. */
	ucFirstCode = FIRST_BYTE_OF_LATINEXTENSION & 0x7F;
	ucSecondCode = pInCharCode[0] & 0x7F;

	/* Check the 1st code. */
	if ((0x21 <= ucSecondCode) && (ucSecondCode <= 0x7E))
	{
		unsigned char buf[2];

		buf[0] = ucFirstCode;
		buf[1] = ucSecondCode;

		return ISDBT_Caption_jisx0208_mbtowc(buf, pOutUCSCode);
	}
#else
	unsigned char ucCode;
	int iIdx;
	unsigned int unicode = 0x0000;

	ucCode = pInCharCode[0] & 0x7F;

	/* Check the latin extension code. */
	if ((0x21 <= ucCode) && (ucCode <= 0x7E))
	{
		iIdx = (int)(ucCode - 0x21);
		unicode = (unsigned int)s_LatinExtensionSet[iIdx];
		if (unicode == 0x0000)
		{
			return -1;
		}

		return 1;
	}
#endif

	return 0;
}


/**************************************************************************
*  FUNCTION NAME :
*	   ISDBT_CC_Convert_Special2Unicode
*
*  DESCRIPTION :
*	   ...
*
*  INPUT :
*	   ...
*
*  OUTPUT :
*	   ...
*
*  REMARK :
*	   None
**************************************************************************/
int ISDBT_CC_Convert_Special2Unicode(const unsigned char *pInCharCode, unsigned int *pOutUCSCode)
{
#ifdef USE_EUCJPMAP_FOR_SPECIAL_CODEMAPPING
	unsigned char ucFirstCode;
	unsigned char ucSecondCode;

	/* Get the EUC-JP code. */
	ucFirstCode = FIRST_BYTE_OF_SPECIAL & 0x7F;
	ucSecondCode = pInCharCode[0] & 0x7F;

	/* Check the 1st code. */
	if ((0x21 <= ucSecondCode) && (ucSecondCode <= 0x7E))
	{
		unsigned char buf[2];

		buf[0] = ucFirstCode;
		buf[1] = ucSecondCode;

		return ISDBT_Caption_jisx0208_mbtowc(buf, pOutUCSCode);
	}
#else
	unsigned char ucCode;
	int idx;
	unsigned int unicode = 0x0000;

	ucCode = pInCharCode[0] & 0x7F;

	/* Check the latin extension code. */
	if ((0x21 <= ucCode) && (ucCode <= 0x7E))
	{
		idx = (int)(ucCode - 0x21);
		if ((0 < idx) && (idx < 94))
		{
			unicode = (unsigned int)s_SpecialSet[idx];
			if (unicode == 0x0000)
			{
				return -1;
			}

			return 1;
		}
	}

#endif

	return 0;
}
#endif /* END of NEW_1SEG_CLOSED_CAPTION_CONVERTER */


#if (1) // OLD_1SEG_CLOSED_CAPTION_CONVERTER
/**************************************************************************
*  FUNCTION NAME :
*	   int ISDBT_Euc2UniConv()
*
*  DESCRIPTION :
*
*  INPUT:
*  OUTPUT:
*  REMARK  :
**************************************************************************/
int ISDBT_Euc2UniConv(const unsigned char *s, unsigned int *pUCSCode, int n)
{
	unsigned char c = *s;

	/* Code set 0 (ASCII or JIS X 0201) */
	if (c < 0x80)
	{
		return (-1);
	}

	/* Code set 1 (JIS X 0208) */
	if ((c >= 0xa1) && (c < 0xff))
	{
		if (n < 2)
		{
			return (-1);
		}

		if (c < 0xf5)
		{
			unsigned char c2 = s[1];

			if ((c2 >= 0xa1) && (c2 < 0xff))
			{
				unsigned char buf[2];

				buf[0] = c - 0x80;
				buf[1] = c2 - 0x80;

				return ISDBT_Caption_jisx0208_mbtowc(buf, pUCSCode);
			}
		}
		else
		{
			/* User-defined range */
			unsigned char c2 = s[1];

			if (c2 >= 0xa1 && c2 < 0xff)
			{
		#if (1) // Lasted Update 2007.05.29
				unsigned int idx;

				idx = 94 * (c - 0x80 - 0x21 - 0x54) + (c2 - 0x80 - 0x21);
				if (idx < (10 * 94))
				{
					*pUCSCode = (unsigned int)s_AddedSymbolsOfKanjiSet[idx];
					return (1);
				}
		#else
				// *pwc = 0xe000 + 94*(c-0xf5) + (c2-0xa1);
				return 2;
		#endif
			}
		}
	}

	return (-1);
}
#endif /* END of NEW_1SEG_CLOSED_CAPTION_CONVERTER */

static unsigned int subtitle_display_make_color
(
	SUB_ARCH_TYPE arch_type,
	unsigned int a, unsigned int r, unsigned int g, unsigned int b
)
{
	unsigned int color;

	if(arch_type == 	SUB_ARCH_TCC92XX){
		color = (unsigned int)((a<<24)|(b<<16)|(g<<8)|(r<<0));
	}else{
		color = (unsigned int)((a<<24)|(r<<16)|(g<<8)|(b<<0)); //ARGB
	}

	return color;
}

static int subtitle_draw_init_palette(SUB_ARCH_TYPE arch)
{
	int i, j;

	for(i=0 ; i<MAX_COLOR ; i++){
		g_CMLA_Table[i] = subtitle_display_make_color(arch, \
						cmla_grp[i].a, cmla_grp[i].r, cmla_grp[i].g, cmla_grp[i].b);
	}

	for(j=0 ; j<MAX_COLOR ; j++){
		g_png_CMLA_Table[j] = subtitle_display_make_color(arch, \
						png_cmla_grp[j].a, png_cmla_grp[j].r, png_cmla_grp[j].g, png_cmla_grp[j].b);
	}
	return 0;
}

int CCPARS_Init(SUB_ARCH_TYPE type)
{
	int i;
	subtitle_draw_init_palette(type);

	for(i=0 ; i<2 ; i++)
	{
		g_grp_data_type[i].mngr_grp_type = 0xff;
		g_grp_data_type[i].mngr_grp_changed = 0;
		g_grp_data_type[i].sts_grp_type = 0xff;
		g_grp_data_type[i].sts_grp_changed = 0;
	}

	return 0;
}

int CCPARS_Exit(void)
{
	int i;

	for(i=0 ; i<2 ; i++)
	{
		g_grp_data_type[i].mngr_grp_type = 0xff;
		g_grp_data_type[i].mngr_grp_changed = 0;
		g_grp_data_type[i].sts_grp_type = 0xff;
		g_grp_data_type[i].sts_grp_changed = 0;
	}

	return 0;
}

int subtitle_lib_mngr_grp_changed(int type)
{
	if((type<0) || (type>1)){
		LIB_DBG(DBG_ERR, "[%s] Invalid type(%d)\n", __func__, type);
		return -1;
	}

	return g_grp_data_type[type].mngr_grp_changed;
}

void subtitle_lib_mngr_grp_set(int type, int set)
{
	if((type<0) || (type>1)){
		LIB_DBG(DBG_ERR, "[%s] Invalid type(%d)\n", __func__, type);
		return;
}

	g_grp_data_type[type].mngr_grp_changed = set;
}

int subtitle_lib_mngr_grp_get(int type)
{
	return g_grp_data_type[type].mngr_grp_type;
}

int subtitle_lib_sts_grp_changed(int type)
{
	if((type<0) || (type>1)){
		LIB_DBG(DBG_ERR, "[%s] Invalid type(%d)\n", __func__, type);
		return -1;
	}

	return g_grp_data_type[type].sts_grp_changed;
}

void subtitle_lib_sts_grp_set(int type, int set)
{
	if((type<0) || (type>1)){
		LIB_DBG(DBG_ERR, "[%s] Invalid type(%d)\n", __func__, type);
		return;
	}

	g_grp_data_type[type].sts_grp_changed = set;
}

int subtitle_lib_sts_grp_get(int type)
{
	return g_grp_data_type[type].sts_grp_type;
}

int subtitle_lib_comp_grp_type(int type)
{
	if((type<0) || (type>1)){
		LIB_DBG(DBG_ERR, "[%s] Invalid type(%d)\n", __func__, type);
		return 0;
	}

	LIB_DBG(DBG_GRP, "[%s] type:%d, mngr_grp_type:0x%02X, sts_grp_type:0x%02X\n", __func__, \
		type,
		g_grp_data_type[type].mngr_grp_type,
		g_grp_data_type[type].sts_grp_type);

	if((g_grp_data_type[type].mngr_grp_type == 0xff) || (g_grp_data_type[type].sts_grp_type == 0xff))
		return 0;

	if(g_grp_data_type[type].mngr_grp_type == g_grp_data_type[type].sts_grp_type)
		return 1;

	return 0;
}

void CCPARS_Set_DtvMode(int data_type, E_DTV_MODE_TYPE dtv_type)
{
	if((data_type<0) || (data_type>1)){
		LIB_DBG(DBG_ERR, "[%s] Invalid data_type(%d)\n", __func__, data_type);
		return;
	}

	//g_DtvMode = dtv_mode;
	gIsdbtCapInfo[data_type].isdbt_type = dtv_type;
	gIsdbtCapInfo[data_type].ucSelectLangCode = ISDBT_CAP_1ST_STATE_DATA;
}

E_DTV_MODE_TYPE CCPARS_Get_DtvMode(int data_type)
{
	if((data_type<0) || (data_type>1)){
		LIB_DBG(DBG_ERR, "[%s] Invalid data_type(%d)\n", __func__, data_type);
		return SEG_UNKNOWN;
}

	return gIsdbtCapInfo[data_type].isdbt_type;
}

void CCPARS_Set_Lang(int data_type, int lang_index)
{
	if((data_type<0) || (data_type>1)){
		LIB_DBG(DBG_ERR, "[%s] Invalid data_type(%d)\n", __func__, data_type);
		return;
	}

	gIsdbtCapInfo[data_type].ucSelectLangCode = lang_index;
}

int CCPARS_Get_Lang(int data_type)
{
	if((data_type<0) || (data_type>1)){
		LIB_DBG(DBG_ERR, "[%s] Invalid data_type(%d)\n", __func__, data_type);
		return -1;
	}

	return gIsdbtCapInfo[data_type].ucSelectLangCode;
}

void CCPARS_Init_GSet(int data_type, E_DTV_MODE_TYPE dtv_type)
{
	if ((dtv_type == ONESEG_SBTVD_T) || (dtv_type == FULLSEG_SBTVD_T))
	{
		stGSet[data_type].G[CE_G0] = CS_ALPHANUMERIC;
		stGSet[data_type].G[CE_G1] = CS_ALPHANUMERIC;
		stGSet[data_type].G[CE_G2] = CS_LATIN_EXTENSION;
		stGSet[data_type].G[CE_G3] = CS_SPECIAL;

		CTRLCODE_LS0(data_type);
		CTRLCODE_LS2R(data_type);
	}
	else if(dtv_type == FULLSEG_ISDB_T)
	{
		stGSet[data_type].G[CE_G0] = CS_KANJI;
		stGSet[data_type].G[CE_G1] = CS_ALPHANUMERIC;
		stGSet[data_type].G[CE_G2] = CS_HIRAGANA;
		stGSet[data_type].G[CE_G3] = CS_MACRO;

		CTRLCODE_LS0(data_type);
		CTRLCODE_LS2R(data_type);
	}
	else /* if (dtv_mode == ONESEG_ISDB_T) and so on. */
	{
		stGSet[data_type].G[CE_G0] = CS_KANJI;
		stGSet[data_type].G[CE_G1] = CS_ALPHANUMERIC;
		stGSet[data_type].G[CE_G2] = CS_HIRAGANA;
		stGSet[data_type].G[CE_G3] = CS_DRCS_1;

		/* The code set of GL & GR are fixed by DRCS & Kanji Character Set in Closed Caption. */
		stGSet[data_type].GL = CE_G3;
		stGSet[data_type].GR = CE_G0; //CS_KANJI;
	}
}

static void *CCPARS_Allocate_DataGroup(void *ptr)
{
	void 					*pointer;
	TS_PES_CAP_DATAGROUP 	*stPrevCap, *ptrCurrCap = NULL;

	pointer = (TS_PES_CAP_DATAGROUP *)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(TS_PES_CAP_DATAGROUP));
	if (pointer == NULL)
	{
		LIB_DBG(DBG_ERR, "[%s] mem alloc error!!\n", __func__);
		return NULL;
	}
	//CC_PRINTF("[5MPEGPARSE_ISDBT:%d] mem alloc DGroup Ptr [0x%08x]\n", __LINE__, pointer);

	if (ptr == NULL)
	{
		ptrCurrCap = (TS_PES_CAP_DATAGROUP *)pointer;
	}
	else
	{
		stPrevCap  = (TS_PES_CAP_DATAGROUP *)ptr;
		ptrCurrCap = (TS_PES_CAP_DATAGROUP *)pointer;

		stPrevCap->ptrNextDGroup = ptrCurrCap;
	}
	ptrCurrCap->ptrNextDGroup = NULL;
	ptrCurrCap->DState.DUnit = NULL;
	ptrCurrCap->DMnge.DUnit = NULL;

	return ptrCurrCap;
}

static void *CCPARS_Allocate_DataUnit(void *ptr)
{
	void				*pointer;
	TS_PES_CAP_DATAUNIT	*stPrevUnit, *ptrCurrUnit = NULL;

	pointer = (TS_PES_CAP_DATAUNIT *)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(TS_PES_CAP_DATAUNIT));
	if (pointer == NULL)
	{
		LIB_DBG(DBG_ERR, "[%s] mem alloc error!!\n", __func__);
		return NULL;
	}
	//CC_PRINTF("[5MPEGPARSE_ISDBT:%d] mem alloc DUnit Ptr [0x%08x]\n", __LINE__, pointer);

	if (ptr == NULL)
	{
		ptrCurrUnit = (TS_PES_CAP_DATAUNIT *)pointer;
	}
	else
	{
		stPrevUnit = (TS_PES_CAP_DATAUNIT *)ptr;
		ptrCurrUnit = (TS_PES_CAP_DATAUNIT *)pointer;

		stPrevUnit->ptrNextDUnit = ptrCurrUnit;
	}
	ptrCurrUnit->ptrNextDUnit = NULL;
	ptrCurrUnit->pData = NULL;

	return ptrCurrUnit;
}

static int CCPARS_Parse_DataUnit
(
	void *handle,
	void *pRawData,
	unsigned char ucCaptionDataType,
	TS_PES_CAP_DATAUNIT **pDUnit,
	unsigned int uiLoopLen
)
{
	t_BITPARSER_DRIVER_INFO_	*stDriverInfo;
	TS_PES_CAP_DATAUNIT			*stPrevUnit, *ptrCurrUnit;

	void			*pointer;
	unsigned int 	i, unit_loop_length;;
	int			iDUnitNum = 0;

	if (handle == NULL) return -1;

	stDriverInfo = (t_BITPARSER_DRIVER_INFO_ *)handle;

	stPrevUnit = NULL;
	unit_loop_length = uiLoopLen;

	/* At least, one data unit should be more than 5 bytes.
	**  - unit_separator : 8 bits
	**  - data_unit_parameter : 8 bits
	**  - data_unit_size : 24 bits
	**  Total Bits = 40 bits (5 bytes)
	*/
	while (unit_loop_length >= 5)
	{
		ptrCurrUnit = CCPARS_Allocate_DataUnit(stPrevUnit);
		if (ptrCurrUnit == NULL)
		{
			LIB_DBG(DBG_ERR, "[%s] Failed to allocate New Data Unit Structure Memory!\n", __func__);
			iDUnitNum = -1;
			break;
		}

		if (stPrevUnit == NULL){
			*pDUnit = ptrCurrUnit;
		}
		ptrCurrUnit->data_unit_size = 0;

		BITPARS_GetBits(stDriverInfo, 8);
		ptrCurrUnit->unit_separator =stDriverInfo->ucCurrent;
		if (ptrCurrUnit->unit_separator != DATA_UNIT_SEPARATOR_CODE)
		{
			LIB_DBG(DBG_ERR, "[%s] Data Unit Separator Code is Inavlid! [0x%X]\n", __func__, ptrCurrUnit->unit_separator);
			iDUnitNum = -1;
			break;
		}

		BITPARS_GetBits(stDriverInfo, 8);
		ptrCurrUnit->data_unit_parameter = stDriverInfo->ucCurrent;
		if (CAPTION_DATA_MANAGE == ucCaptionDataType)
		{
			if ( (ptrCurrUnit->data_unit_parameter != ISDBT_DUNIT_STATEMENT_BODY) &&
				 (ptrCurrUnit->data_unit_parameter != ISDBT_DUNIT_1BYTE_DRCS) &&
				 (ptrCurrUnit->data_unit_parameter != ISDBT_DUNIT_2BYTE_DRCS) &&
				 (ptrCurrUnit->data_unit_parameter != ISDBT_DUNIT_COLOR_MAP) )
			{
				LIB_DBG(DBG_ERR, "[%s] Data Unit Parameter is Invalid! [0x%02X]\n", __func__,
							ptrCurrUnit->data_unit_parameter);
				iDUnitNum = -1;
				break;
			}
		}

		BITPARS_GetBits(stDriverInfo, 24);
		ptrCurrUnit->data_unit_size = stDriverInfo->ulCurrent;
		if (ptrCurrUnit->data_unit_size > MAX_SIZE_OF_CAPTION_PES)
		{
			LIB_DBG(DBG_ERR, "[%s] Statement Data Unit Size (%d) exceeds %d bytes!\n", __func__,
						ptrCurrUnit->data_unit_size, MAX_SIZE_OF_CAPTION_PES);
			iDUnitNum = -1;
			break;
		}

		unit_loop_length -= 5;

		if (ptrCurrUnit->data_unit_size > unit_loop_length)
		{
			LIB_DBG(DBG_ERR, "[%s] Data Unit Size is more than remainded data length!\n", __func__);
			iDUnitNum = -1;
			break;
		}
		else if (ptrCurrUnit->data_unit_size > 0)
		{
			unit_loop_length -= ptrCurrUnit->data_unit_size;

			//CC_PRINTF("@@@ 0x%02X :: Data Unit Size = %d @@@\n", ptrCurrUnit->data_unit_parameter, ptrCurrUnit->data_unit_size);
			pointer = (unsigned char *)tcc_mw_malloc(__FUNCTION__, __LINE__, ptrCurrUnit->data_unit_size);
			if (pointer == NULL)
			{
				LIB_DBG(DBG_ERR, "[%s] mem alloc error!!\n", __func__);
				iDUnitNum = -1;
				break;
			}

			ptrCurrUnit->pData = (unsigned char *)pointer;
			memcpy(ptrCurrUnit->pData, stDriverInfo->pucRawData, ptrCurrUnit->data_unit_size);

			for (i = 0; i < ptrCurrUnit->data_unit_size; i++)
			{
				BITPARS_DecrementGlobalLength(handle);
			}
		}

		iDUnitNum++;
		stPrevUnit = ptrCurrUnit;
	}

	if (iDUnitNum < 0)
	{
		LIB_DBG(DBG_ERR, "[%s] WARNING!! Data Unit Parsing Failed => Remain Data Unit Length is %d!\n", __func__,
					unit_loop_length);
		if (unit_loop_length > 0)
		{
			for (i = 0; i < unit_loop_length; i++)
			{
				BITPARS_DecrementGlobalLength(handle);
			}
		}
	}
	else
	{
		if (unit_loop_length > 0)
		{
			LIB_DBG(DBG_ERR, "[%s] WARNING!! Data Unit is %d. Remain Data Unit Length is %d.\n", __func__,
						iDUnitNum, unit_loop_length);

			for (i = 0; i < unit_loop_length; i++)
			{
				BITPARS_DecrementGlobalLength(handle);
			}
		}
	}

	return	iDUnitNum;
}


/**************************************************************************
*  FUNCTION NAME :
*      CCPARS_Get_GSetByFinalByte
*
*  DESCRIPTION :
*      ...
*
*  INPUT :
*  	   ...
*
*  OUTPUT :
*      ...
*
*  REMARK :
*      Classification of code set and Final byte, table7-3
**************************************************************************/
static int CCPARS_Get_GSetByFinalByte(unsigned char FinalByte)
{
	int iRetGSet;

	switch (FinalByte)
	{
		case GS_KANJI:
		case GS_JIS_COMPATIBLE_KANJI_PLANE_1:
		case GS_JIS_COMPATIBLE_KANJI_PLANE_2:
		case GS_ADDITIONAL_SYMBOLS:
			iRetGSet = CS_KANJI;
			break;

		case GS_ALPHANUMERIC:
		case GS_PROPORTIONAL_ALPHANUMERIC:
			iRetGSet = CS_ALPHANUMERIC;
			break;

		case GS_LATIN_EXTENSION:
			iRetGSet = CS_LATIN_EXTENSION;
			break;

		case GS_SPECIAL:
			iRetGSet = CS_SPECIAL;
			break;

		case GS_HIRAGANA:
		case GS_PROPORTIONAL_HIRAGANA:
			iRetGSet = CS_HIRAGANA;
			break;

		case GS_KATAKANA:
		case GS_PROPORTIONAL_KATAKANA:
			iRetGSet = CS_KATAKANA;
			break;

		case GS_MOSAIC_A:
			iRetGSet = CS_MOSAIC_A;
			break;

		case GS_MOSAIC_B:
			iRetGSet = CS_MOSAIC_B;
			break;

		case GS_MOSAIC_C:
			iRetGSet = CS_MOSAIC_C;
			break;

		case GS_MOSAIC_D:
			iRetGSet = CS_MOSAIC_D;
			break;

		case GS_JIS_X_0201_KATAKANA:
			iRetGSet = CS_JISX0201_KATAKANA;
			break;

		default:
			{
				int dtv_mode = CCPARS_Get_DtvMode(0);
				LIB_DBG(DBG_ERR, " GSET_FINAL_BYTE] The Final Byte is wrong!set to default\n");
				if (dtv_mode == ONESEG_SBTVD_T)
				{
					iRetGSet = CS_ALPHANUMERIC;
				}
				else /* if (dtv_mode == ONESEG_ISDB_T) and so on. */
				{
					iRetGSet = CS_KANJI;
				}
			}
			break;
	}

	return iRetGSet;
}


/**************************************************************************
*  FUNCTION NAME :
*      CCPARS_Get_DRCSByFinalByte
*
*  DESCRIPTION :
*      ...
*
*  INPUT :
*  	   ...
*
*  OUTPUT :
*      ...
*
*  REMARK :
*      Classification of code set and Final byte, table7-3
**************************************************************************/
static int CCPARS_Get_DRCSByFinalByte(unsigned char FinalByte)
{
	int iRetDRCS;

	switch (FinalByte)
	{
		case GS_DRCS_0:
			iRetDRCS = CS_DRCS_0;
			break;

		case GS_DRCS_1:
			iRetDRCS = CS_DRCS_1;
			break;

		case GS_DRCS_2:
			iRetDRCS = CS_DRCS_2;
			break;

		case GS_DRCS_3:
			iRetDRCS = CS_DRCS_3;
			break;

		case GS_DRCS_4:
			iRetDRCS = CS_DRCS_4;
			break;

		case GS_DRCS_5:
			iRetDRCS = CS_DRCS_5;
			break;

		case GS_DRCS_6:
			iRetDRCS = CS_DRCS_6;
			break;

		case GS_DRCS_7:
			iRetDRCS = CS_DRCS_7;
			break;

		case GS_DRCS_8:
			iRetDRCS = CS_DRCS_8;
			break;

		case GS_DRCS_9:
			iRetDRCS = CS_DRCS_9;
			break;

		case GS_DRCS_10:
			iRetDRCS = CS_DRCS_10;
			break;

		case GS_DRCS_11:
			iRetDRCS = CS_DRCS_11;
			break;

		case GS_DRCS_12:
			iRetDRCS = CS_DRCS_12;
			break;

		case GS_DRCS_13:
			iRetDRCS = CS_DRCS_13;
			break;

		case GS_DRCS_14:
			iRetDRCS = CS_DRCS_14;
			break;

		case GS_DRCS_15:
			iRetDRCS = CS_DRCS_15;
			break;

		case GS_MACRO:
			iRetDRCS = CS_MACRO;
			break;

		default:
			{
				int dtv_mode = CCPARS_Get_DtvMode(0);
				LIB_DBG(DBG_ERR, " DRCS_FINAL_BYTE] The Final Byte is wrong!\n");
				if (dtv_mode == ONESEG_SBTVD_T)
				{
					iRetDRCS = CS_LATIN_EXTENSION;
				}
				else /* if (dtv_mode == ONESEG_ISDB_T) and so on. */
				{
					iRetDRCS = CS_DRCS_1;
				}
			}
			break;
	}

	return iRetDRCS;
}


/**************************************************************************
*  FUNCTION NAME :
*      CCPARS_Parse_CtrlCodeESC
*
*  DESCRIPTION :
*      ...
*
*  INPUT :
*  	   ...
*
*  OUTPUT :
*      ...
*
*  REMARK :
*      None
**************************************************************************/
static void *CCPARS_Parse_CtrlCodeESC(int data_type, void *handle, void *pRawData)
{
	t_BITPARSER_DRIVER_INFO_ *stDriverInfo;
	unsigned char ucChar;
	int iElementCode = CE_MAX;

	if (handle == NULL){
		return NULL;
	}

	/* Initialize the bit parser. */
	stDriverInfo = (t_BITPARSER_DRIVER_INFO_ *)handle;
	BITPARS_InitBitParser(stDriverInfo, pRawData, 9);

	/* Get 1 byte. */
	BITPARS_GetBits(stDriverInfo, 8);
	ucChar = stDriverInfo->ucCurrent;

	LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_ESC(0x%X)\n", ucChar);

	/* Check a byte number of code. */
	if (ucChar == 0x24) /* 2-byte code */
	{
		/* Get 1 byte. */
		BITPARS_GetBits(stDriverInfo, 8);
		ucChar = stDriverInfo->ucCurrent;

		/* Check the 2nd code representation. */
		if ( (ucChar == 0x28) ||
		     (ucChar == 0x29) ||
		     (ucChar == 0x2A) ||
		     (ucChar == 0x2B) )
		{
			/* Get an element code. */
			iElementCode = ucChar - 0x28;

			/* Get 1 byte. */
			BITPARS_GetBits(stDriverInfo, 8);
			ucChar = stDriverInfo->ucCurrent;

			/* Check a classification of graphic sets. */
			if (ucChar == 0x20)
			{
				/* Get 1 byte. */
				BITPARS_GetBits(stDriverInfo, 8);
				ucChar = stDriverInfo->ucCurrent;

				LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_ESC(1byte - DRCS)\n");

				/* Get the 2-byte DRCS. */
				stGSet[data_type].G[iElementCode] = CCPARS_Get_DRCSByFinalByte(ucChar);
			}
			else
			{
				LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_ESC(2byte - GSet)\n");

				/* Get the 2-byte G set. */
				stGSet[data_type].G[iElementCode] = CCPARS_Get_GSetByFinalByte(ucChar);
			}
		}
		else
		{
			/* Get the 2-byte G set. */
			stGSet[data_type].G[CE_G0] = CCPARS_Get_GSetByFinalByte(ucChar);
		}

	}
	else if ( (ucChar == 0x28) || /* 1-byte code */
	          (ucChar == 0x29) ||
	          (ucChar == 0x2A) ||
	          (ucChar == 0x2B) )
	{
		/* Get an element code. */
		iElementCode = ucChar - 0x28;

		/* Get 1 byte. */
		BITPARS_GetBits(stDriverInfo, 8);
		ucChar = stDriverInfo->ucCurrent;

		/* Check a classification of graphic sets. */
		if (ucChar == 0x20)
		{
			/* Get 1 byte. */
			BITPARS_GetBits(stDriverInfo, 8);
			ucChar = stDriverInfo->ucCurrent;

			LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_ESC(1byte - DRCS)\n");

			/* Get the 1-byte DRCS. */
			stGSet[data_type].G[iElementCode] = CCPARS_Get_DRCSByFinalByte(ucChar);
		}
		else
		{
			LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_ESC(1byte - GSet)\n");

			/* Get the 1-byte G set. */
			stGSet[data_type].G[iElementCode] = CCPARS_Get_GSetByFinalByte(ucChar);
		}
	}
	/* ESC Locking Shift ... */
	else if (ucChar == 0x6E)
	{
		LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_ESC(CTRLCODE_LS2)\n");
		CTRLCODE_LS2(data_type);
	}
	else if (ucChar == 0x6F)
	{
		LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_ESC(CTRLCODE_LS3)\n");
		CTRLCODE_LS3(data_type);
	}
	else if (ucChar == 0x7E)
	{
		LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_ESC(CTRLCODE_LS1R)\n");
		CTRLCODE_LS1R(data_type);
	}
	else if (ucChar == 0x7D)
	{
		LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_ESC(CTRLCODE_LS2R)\n");
		CTRLCODE_LS2R(data_type);
	}
	else if (ucChar == 0x7C)
	{
		LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_ESC(CTRLCODE_LS3R)\n");
		CTRLCODE_LS3R(data_type);
	}

	return stDriverInfo->pucRawData;
}

int CCPARS_Parse_CSI(T_SUB_CONTEXT *p_sub_ctx, int mngr, unsigned char ucCode, int iParamCount, int iParams[])
{
	T_CHAR_DISP_INFO *param = (T_CHAR_DISP_INFO*)&p_sub_ctx->disp_info;

	/* Handle the CSI according to final character code. */
	switch (ucCode)
	{
		case ARIB_CSI_SWF:
			// CSI  P11..P1i  I1  F
			// CSI  P1  I1  P2  I2  P31..P3i  [I3  P41..P4j]  I4  F
			LIB_DBG(DBG_CSI, " EXT-CTRL-CODE] ARIB_CSI_SWF\n");

			if (iParamCount == 1)
			{
				int format = iParams[0];
				LIB_DBG(DBG_CSI, " EXT-CTRL-CODE] ARIB_CSI_SWF(%d)\n", format);

				if((format==7) || (format==8) || (format==9) || (format==10)){
					CCPARS_Parse_Pos_Init(p_sub_ctx, mngr, E_INIT_CTRL_CODE_SWF, format);
				}else{
					LIB_DBG(DBG_ERR, " EXT-CTRL-CODE] ARIB_CSI_SWF(Unknow:%d)\n", format);
				}
			}
			else if ((iParamCount == 3) || (iParamCount == 4))
			{
				LIB_DBG(DBG_CSI, " EXT-CTRL-CODE] ARIB_CSI_SWF(%d,%d,%d,%d) - Not implemented yet!!\n",
							iParams[0], iParams[1], iParams[2], iParams[3]);
			}
			break;

#ifdef USE_OUTPUT_COMPONENT_COMPOSITE   
		case ARIB_CSI_CCC:
			// CSI  P1  I1  F
			LIB_DBG(DBG_ERR, " EXT-CTRL-CODE] ARIB_CSI_CCC - Not implemented yet!!\n");
			break;
#endif

		case ARIB_CSI_RCS:
			// CSI  P11..P1i  I1  F
			LIB_DBG(DBG_CSI, " EXT-CTRL-CODE] ARIB_CSI_RCS (%d)\n", iParams[0]);
			if(iParams[0] < MAX_COLOR){
				param->uiRasterColor = g_CMLA_Table[iParams[0]];
				ISDBTCap_ControlCharDisp(p_sub_ctx, mngr, (unsigned int)E_DISP_CTRL_CLR_REC, 0, 0, 0);
			}
			break;

		case ARIB_CSI_ACPS:
			if(iParamCount != 2){
				LIB_DBG(DBG_ERR, "EXT-CTRL-CODE] ARIB_CSI_ACPS - ERROR\n");
			}else{
				// CSI  P11..P1i  I1  P21..P2j  I2  F
				LIB_DBG(DBG_CSI, " EXT-CTRL-CODE] ARIB_CSI_ACPS (%dx%d)\n", iParams[0], iParams[1]);
				param->act_pos_x = iParams[0];
				param->act_pos_y = iParams[1];
			}
			break;

		case ARIB_CSI_SDF:
			if(iParamCount != 2){
				LIB_DBG(DBG_ERR, "EXT-CTRL-CODE] ARIB_CSI_SDF - ERROR\n");
			}else{
				// CSI  P11..P1i  I1  P21..P2j  I2  F
				LIB_DBG(DBG_CSI, " EXT-CTRL-CODE] ARIB_CSI_SDF (mgr:%d, %dx%d)\n", mngr, iParams[0], iParams[1]);
				if(mngr){
					param->mngr_disp_w = iParams[0];
					param->mngr_disp_h = iParams[1];
				}else{
					param->act_disp_w = iParams[0];
					param->act_disp_h = iParams[1];
				}
			}
			break;

		case ARIB_CSI_SDP:
			if(iParamCount != 2){
				LIB_DBG(DBG_ERR, "EXT-CTRL-CODE] ARIB_CSI_SDP - ERROR\n");
			}else{
				int char_path_len = 0, line_dir_len = 0;
				int font_ratio_w = 1, font_ratio_h =1;
				int vs_ratio = 1;

				// CSI  P11..P1i  I1  P21..P2j  I2  F
				LIB_DBG(DBG_CSI, " EXT-CTRL-CODE] ARIB_CSI_SDP (mgr:%d, %dx%d)\n", mngr, iParams[0], iParams[1]);
				if(mngr){
					param->mngr_pos_x = iParams[0];
					param->mngr_pos_y = iParams[1];
				}else{
					param->origin_pos_x = iParams[0];
					param->origin_pos_y = iParams[1];

					if(param->usDispMode & CHAR_DISP_MASK_DOUBLE_WIDTH){	font_ratio_w = 2;	}
					if(param->usDispMode & CHAR_DISP_MASK_DOUBLE_HEIGHT){	font_ratio_h = vs_ratio = 2;	}

					if(param->usDispMode == CHAR_DISP_MASK_HWRITE){
						char_path_len = (param->act_font_w * font_ratio_w + param->act_hs);
						line_dir_len = (param->act_font_h * font_ratio_h + param->act_vs * vs_ratio);
					}else{
						char_path_len = (param->act_font_w * font_ratio_w + param->act_vs * vs_ratio);
						line_dir_len = (param->act_font_h * font_ratio_h + param->act_hs);
					}

					if(param->usDispMode & CHAR_DISP_MASK_HALF_WIDTH){		char_path_len >>= 1;	}
					if(param->usDispMode & CHAR_DISP_MASK_HALF_HEIGHT){		line_dir_len >>= 1;	}

					/* management의 SDP가 설정된후 statement의 SDP가 왔을 경우 처리 */
					if(param->usDispMode == CHAR_DISP_MASK_HWRITE){
						param->act_pos_x = param->origin_pos_x;
						param->act_pos_y = param->origin_pos_y + line_dir_len;
					}else{
						param->act_pos_x = param->origin_pos_x + param->act_disp_w - (char_path_len>>1);
						param->act_pos_y = param->origin_pos_y;
					}
				}
			}
			break;

#ifdef USE_OUTPUT_COMPONENT_COMPOSITE 
		case ARIB_CSI_SSM:
			// CSI  P11..P1i  I1  P21..P2j  I2  F
			LIB_DBG(DBG_CSI, " EXT-CTRL-CODE] ARIB_CSI_SSM (mgr:%d, H:%d, V:%d)\n",mngr, iParams[0], iParams[1]);
				/* Only 16x16, 20x20, 24x24, 30x30, 36x36 dots can be specified. */
			if(mngr){
				param->mngr_font_w = iParams[0];
				param->mngr_font_h = iParams[1];
			}else{
				int font_w=0, font_h=0, diff_w=0, diff_h=0;

				font_w = iParams[0];
				font_h = iParams[1];

				diff_w = param->act_font_w - font_w;
				diff_h = param->act_font_h - font_h;

				if(param->usDispMode == CHAR_DISP_MASK_HWRITE){
					/* original font size is larger than SSM */
					if((param->act_pos_y - diff_h) >= 0){
						param->act_pos_y -= diff_h;
					}
				}else{
					if((unsigned int)(param->act_pos_x + diff_w) < (param->origin_pos_x + param->act_disp_w)){
						param->act_pos_x += (diff_w>>1);
					}
				}

				param->act_font_w = font_w;
				param->act_font_h = font_h;
			}
			break;
#endif  

		case ARIB_CSI_SHS:
			// CSI  P11..P1i  I1  F
			LIB_DBG(DBG_CSI, " EXT-CTRL-CODE] ARIB_CSI_SHS (mgr:%d, Dot Number:%d)\n", mngr, iParams[0]);
			if(mngr){
				param->mngr_hs = iParams[0];
			}else{
				param->act_hs = iParams[0];
			}
			break;

		case ARIB_CSI_SVS:
			// CSI  P11..P1i  I1  F
			LIB_DBG(DBG_CSI, " EXT-CTRL-CODE] ARIB_CSI_SVS (mgr:%d, Dot Number:%d)\n", mngr, iParams[0]);
			if(mngr){
				param->mngr_vs = iParams[0];
			}else{
				param->act_vs = iParams[0];
			}
			break;

		case ARIB_CSI_GSM:
			// CSI  P11..P1i  I1  P21..P2j  I2  F
			LIB_DBG(DBG_ERR, " EXT-CTRL-CODE] ARIB_CSI_GSM - Not implemented yet!!\n");
			break;

		case ARIB_CSI_GAA:
			// CSI  P1  I1  F
			LIB_DBG(DBG_ERR, " EXT-CTRL-CODE] ARIB_CSI_GAA - Not implemented yet!!\n");
			break;

		case ARIB_CSI_SRC:
			// CSI  P1  I1  P21  P22  P23  P24  I2  F
			LIB_DBG(DBG_ERR, " EXT-CTRL-CODE] ARIB_CSI_SRC - Not implemented yet!!\n");
			break;

		case ARIB_CSI_TCC:
			// CSI  P1  I1  P2  I2  P31..P3i  I3  F
			LIB_DBG(DBG_ERR, " EXT-CTRL-CODE] ARIB_CSI_TCC - Not implemented yet!!\n");
			break;

		case ARIB_CSI_CFS:
			// CSI  P11..P1i  I1  F
			LIB_DBG(DBG_ERR, " EXT-CTRL-CODE] ARIB_CSI_CFS - Not implemented yet!!\n");
			break;

		case ARIB_CSI_ORN:
			// CSI  P1  [I1  P21  P22  P23  P24]  I2  F
			LIB_DBG(DBG_CSI, " EXT-CTRL-CODE] ARIB_CSI_ORN\n");
			if ((iParamCount == 1) || (iParamCount == 2))
			{
				LIB_DBG(DBG_CSI, "ORN:%d, Upper 4 Bit of Color Map Address:%d, Lower 4 Bit of Color Map Address:%d\n",
							iParams[0], ((iParams[1] & 0xF0) >> 8), (iParams[1] & 0xF));
				/* TBD */
			}
			break;

		case ARIB_CSI_MDF:
			// CSI  P1  I1  F
			LIB_DBG(DBG_ERR, " EXT-CTRL-CODE] ARIB_CSI_MDF - Not implemented yet!!\n");
			break;

		case ARIB_CSI_XCS:
			// CSI  P1  I1  F
			LIB_DBG(DBG_ERR, " EXT-CTRL-CODE] ARIB_CSI_XCS - Not implemented yet!!\n");
			break;

		case ARIB_CSI_SCR:
			// CSI  P1  I1  F
			LIB_DBG(DBG_ERR, " EXT-CTRL-CODE] ARIB_CSI_SCR[%d, %d] - Not implemented yet!!\n", iParams[0], iParams[1]);
			break;

		case ARIB_CSI_PRA:
			// CSI  P11..P1i  I1  F
			LIB_DBG(DBG_ERR, " EXT-CTRL-CODE] ARIB_CSI_PRA[%d]\n", iParams[0]);
			param->internal_sound = iParams[0];
			if(param->internal_sound < MAX_SOUND){
				subtitle_get_internal_sound(param->internal_sound);
			}
			break;

		case ARIB_CSI_ACS:
			// CSI  P1  I1  F
			LIB_DBG(DBG_ERR, " EXT-CTRL-CODE] ARIB_CSI_ACS - Not implemented yet!!\n");
			break;

		default:
			LIB_DBG(DBG_ERR, " EXT-CTRL-CODE] Unknown Code = 0x%02X\n", ucCode);
			break;
	}

	return 0;
}

/**************************************************************************
*  FUNCTION NAME :
*      CCPARS_Parse_CtrlCodeCSI
*
*  DESCRIPTION :
*      This function parses the extension control code.
*      (CSI - control sequence introducer.)
*
*  INPUT :
*  	   ...
*
*  OUTPUT :
*      ...
*
*  REMARK :
*      Classification of code set and Final byte, table7-3
**************************************************************************/
static void *CCPARS_Parse_CtrlCodeCSI(T_SUB_CONTEXT *p_sub_ctx, int mngr, void *handle, void *pRawData)
{
	t_BITPARSER_DRIVER_INFO_ *stDriverInfo;

	unsigned char ucCode;
	int iParamCount = 0;
	int iParams[MAX_NUMS_OF_CSI_PARAMS] = { 0, };

	/* Check the BitParser handle. */
	if (handle == NULL)
	{
		return NULL;//BITPARSER_DRV_ERR_INVALID_HANDLE;
	}

	stDriverInfo = (t_BITPARSER_DRIVER_INFO_ *)handle;
	BITPARS_InitBitParser(stDriverInfo, pRawData, 64); /* Is 64 ok? */

	/* Get the 1st character code. */
	BITPARS_GetBits(stDriverInfo, 8);
	ucCode = stDriverInfo->ucCurrent;

	if (ucCode == ARIB_CSI_PLD){
		LIB_DBG(DBG_CSI, " EXT-CTRL-CODE] ARIB_CSI_PLD - Not implemented.\n");
	}else if (ucCode == ARIB_CSI_PLU){
		LIB_DBG(DBG_CSI, " EXT-CTRL-CODE] ARIB_CSI_PLU - Not implemented.\n");
	}else if (ucCode == ARIB_CSI_SCS){
		LIB_DBG(DBG_CSI, " EXT-CTRL-CODE] ARIB_CSI_SCS - Not implemented.\n");
	}
	else
	{
		/* Parse the CSI parameters. */
		iParams[iParamCount] = (int)(ucCode - 0x30); /* Set an initial value because it gets one character code. */
		do
		{
			/* Get next character code. */
			BITPARS_GetBits(stDriverInfo, 8);
			ucCode = stDriverInfo->ucCurrent;

			while ((ucCode != ARIB_PARSING_MIDDLE_CHAR) && (ucCode != ARIB_PARSING_END_CHAR))
			{
				/* Save the parameter value. */
				iParams[iParamCount] = (iParams[iParamCount] * 10) + (int)(ucCode - 0x30);

				/* Get next character code. */
				BITPARS_GetBits(stDriverInfo, 8);
				ucCode = stDriverInfo->ucCurrent;
			}

			/* Increase the parameter count. */
			iParamCount++;
		} while (ucCode != ARIB_PARSING_END_CHAR);

		/* Get the final character code. */
		BITPARS_GetBits(stDriverInfo, 8);
		ucCode = stDriverInfo->ucCurrent;
		CCPARS_Parse_CSI(p_sub_ctx, mngr, ucCode, iParamCount, iParams);
	}

	return stDriverInfo->pucRawData;
}

int CCPARS_Parse_CCPES(T_SUB_CONTEXT *p_sub_ctx, void *handle, void *pRawData, int RawDataSize, TS_PES_CAP_PARM *pstCapParmStart, int *DUnitNum)
{
	TS_PES_CAP_PARM				*stCapParm = NULL;
	t_BITPARSER_DRIVER_INFO_	*stDriverInfo;
	TS_PES_CAP_DATAGROUP		*ptrCurrCap = NULL, *ptrPrevCap = NULL;

	unsigned int	i, j;
	int				Nbytes;
	int data_type;

	unsigned char	data_group_id_group_type = DATA_GROUP_ID_MASK;
	unsigned char	data_group_caption_data_type = CAPTION_DATA_TYPE_MASK;

	if (handle == NULL)
	{
		return BITPARSER_DRV_ERR_INVALID_HANDLE;
	}

	data_type = p_sub_ctx->disp_info.data_type;

	*DUnitNum = 0;

	/* Check a pointer of PES caption parameter structure. */
	if (pstCapParmStart == NULL)
	{
		LIB_DBG(DBG_ERR, "Pointer of PES caption parameter structure is NULL!\n");
		return (-1);
	}
	stCapParm = pstCapParmStart;

	stDriverInfo = (t_BITPARSER_DRIVER_INFO_ *)handle;
	stDriverInfo->fLengthError = 0; //initialize
	BITPARS_InitBitParser(stDriverInfo, pRawData, RawDataSize);

	Nbytes = RawDataSize;

	/* ---------------------------------------------------------------------- */
	Nbytes--;
	if (Nbytes <= 0)
	{
		LIB_DBG(DBG_ERR, "[CCPARS][%d] PES Data Size is ERROR! (Data Size = %d Byte(s))\n", __LINE__, Nbytes);
		return CCPARS_ERROR_BAD_DATA_SIZE;
	}

	BITPARS_GetBits(stDriverInfo, 8);
	stCapParm->data_identifier = stDriverInfo->ucCurrent;

	if(stCapParm->data_identifier == SYNC_PES_DATA_ID){
		LIB_DBG(DBG_INFO, "<<<<<<< SUBTITLE >>>>>>>\n");
	}else if(stCapParm->data_identifier == ASYNC_PES_DATA_ID){
		LIB_DBG(DBG_INFO, "<<<<<<< SUPERIMPOSE >>>>>>>\n");
	}else{
		LIB_DBG(DBG_ERR, " PES Data Identifier is Invalid! (0x%02X)\n", stCapParm->data_identifier);
		return (-2);
	}

	/* ---------------------------------------------------------------------- */
	Nbytes--;
	if (Nbytes <= 0)
	{
		LIB_DBG(DBG_ERR, "PES Data Size is ERROR!- 1 (Data Size = %d Byte(s))\n", Nbytes);
		return CCPARS_ERROR_BAD_DATA_SIZE;
	}

	BITPARS_GetBits(stDriverInfo, 8);
	stCapParm->private_stream_id = stDriverInfo->ucCurrent;
	if (stCapParm->private_stream_id != SYNC_PES_PRIVATE_STREAM_ID)
	{
		LIB_DBG(DBG_ERR, " Private Stream ID is Invalid! (0x%02X)\n", stCapParm->private_stream_id);
		return (-3);
	}

	/* ---------------------------------------------------------------------- */
	Nbytes--;
	if (Nbytes <= 0)
	{
		LIB_DBG(DBG_ERR, "PES Data Size is ERROR!- 2 (Data Size = %d Byte(s))\n", Nbytes);
		return CCPARS_ERROR_BAD_DATA_SIZE;
	}

	BITPARS_GetBits(stDriverInfo, 4);
	stCapParm->reserved_future_use = stDriverInfo->ucCurrent;

	BITPARS_GetBits(stDriverInfo, 4);
	stCapParm->PES_data_packet_header_length = stDriverInfo->ucCurrent;

	if (stCapParm->PES_data_packet_header_length > 0)
	{
		for (i = 0; i < stCapParm->PES_data_packet_header_length; i++)
		{
			Nbytes--;
			if (Nbytes <= 0)
			{
				LIB_DBG(DBG_ERR, "[CCPARS][%d] PES Data Size is ERROR! (Data Size = %d Byte(s))\n", __LINE__, Nbytes);
				return CCPARS_ERROR_BAD_DATA_SIZE;
			}

			BITPARS_GetBits(stDriverInfo, 8); // PES_data_private_data_byte - 8 bits
		}
	}

	for (i = 0; i < MAX_NUMS_OF_CC_DATA_GROUP; i++)
	{
		if (Nbytes == 0)
		{
			break;
		}
		else if (Nbytes < 0)
		{
			LIB_DBG(DBG_ERR, "[CCPARS][%d] PES Data Size is ERROR! (Data Size = %d Byte(s))\n", __LINE__, Nbytes);
			return CCPARS_ERROR_BAD_DATA_SIZE;
		}

		ptrCurrCap = CCPARS_Allocate_DataGroup(ptrPrevCap);
		if (ptrCurrCap == NULL)
		{
			LIB_DBG(DBG_ERR, "Failed the memory allocation of caption data group!\n");
			return CCPARS_ERROR_FAIL_MEM_ALLOC;
		}

		if (ptrPrevCap == NULL)
		{
			stCapParm->DGroup = (TS_PES_CAP_DATAGROUP *)ptrCurrCap;
		}

		ptrCurrCap->DMnge.DUnit = NULL;
		ptrCurrCap->DState.DUnit = NULL;

		/* ------------------------------------------------------------------ */
		Nbytes--;
		if (Nbytes <= 0)
		{
			LIB_DBG(DBG_ERR, "[CCPARS][%d] PES Data Size is ERROR! (Data Size = %d Byte(s))\n", __LINE__, Nbytes);
			return CCPARS_ERROR_BAD_DATA_SIZE;
		}

		BITPARS_GetBits(stDriverInfo, 6);
		ptrCurrCap->data_group_id = stDriverInfo->ucCurrent;
		data_group_id_group_type = ptrCurrCap->data_group_id & DATA_GROUP_ID_MASK;
		if ((data_group_id_group_type != DATA_GROUP_ID_GROUP_A) && (data_group_id_group_type != DATA_GROUP_ID_GROUP_B))
		{
			LIB_DBG(DBG_ERR, "Group Type of Data Group ID is Inavlid! (0x%02X)\n", data_group_id_group_type);
			return (-6);
		}

		data_group_caption_data_type = ptrCurrCap->data_group_id & CAPTION_DATA_TYPE_MASK;
		if (data_group_caption_data_type > CAPTION_DATA_STATE_8TH_LANG)
		{
			LIB_DBG(DBG_ERR, "Caption Data Type of Data Group ID is Inavlid! (0x%02X)\n", data_group_caption_data_type);
			return (-61);
		}

		if(data_group_caption_data_type == CAPTION_DATA_MANAGE){
			LIB_DBG(DBG_GRP, "[MGR] (%d) GRP_TYPE_CHECK : 0x%02X(%s) -> 0x%02X(%s)\n", \
						data_type,
						g_grp_data_type[data_type].mngr_grp_type,
						(g_grp_data_type[data_type].mngr_grp_type == 0x20)?"GroupB":"GroupA",
						data_group_id_group_type,
						(data_group_id_group_type == 0x20)?"GroupB":"GroupA");

			if(g_grp_data_type[data_type].mngr_grp_type != data_group_id_group_type){
				if((g_grp_data_type[data_type].mngr_grp_type != 0xff) && (data_group_id_group_type != 0xff)){
					g_grp_data_type[data_type].mngr_grp_changed = 1;

					LIB_DBG(DBG_GRP, "[MGR] (%d) GRP_TYPE_CHANGED : 0x%02X(%s) -> 0x%02X(%s)\n", \
						data_type,
						g_grp_data_type[data_type].mngr_grp_type, (g_grp_data_type[data_type].mngr_grp_type == 0x20)?"GroupB":"GroupA",
						data_group_id_group_type, (data_group_id_group_type == 0x20)?"GroupB":"GroupA");
				}
				g_grp_data_type[data_type].mngr_grp_type = data_group_id_group_type;
			}
		}else{
			LIB_DBG(DBG_GRP, "[STS] (%d) GRP_TYPE_CHECK : 0x%02X(%s) -> 0x%02X(%s)\n", \
						data_type,
						g_grp_data_type[data_type].mngr_grp_type,
						(g_grp_data_type[data_type].mngr_grp_type == 0x20)?"GroupB":"GroupA",
						data_group_id_group_type,
						(data_group_id_group_type == 0x20)?"GroupB":"GroupA");

			if(g_grp_data_type[data_type].sts_grp_type != data_group_id_group_type){
				if((g_grp_data_type[data_type].sts_grp_type != 0xff) && (data_group_id_group_type != 0xff)){
					g_grp_data_type[data_type].sts_grp_changed = 1;

					LIB_DBG(DBG_GRP, "[STS] (%d) GRP_TYPE_CHANGED : 0x%02X(%s) -> 0x%02X(%s)\n", \
						data_type,
						g_grp_data_type[data_type].sts_grp_type, (g_grp_data_type[data_type].sts_grp_type == 0x20)?"GroupB":"GroupA",
						data_group_id_group_type, (data_group_id_group_type == 0x20)?"GroupB":"GroupA");
				}
				g_grp_data_type[data_type].sts_grp_type = data_group_id_group_type;
			}
		}

		BITPARS_GetBits(stDriverInfo, 2);
		ptrCurrCap->data_group_version =stDriverInfo->ucCurrent;

		/* ------------------------------------------------------------------ */
		Nbytes--;
		if (Nbytes <= 0)
		{
			LIB_DBG(DBG_ERR, "[CCPARS][%d] PES Data Size is ERROR! (Data Size = %d Byte(s))\n", __LINE__, Nbytes);
			return CCPARS_ERROR_BAD_DATA_SIZE;
		}

		BITPARS_GetBits(stDriverInfo, 8);
		ptrCurrCap->data_group_link_number = stDriverInfo->ucCurrent;

		/* ------------------------------------------------------------------ */
		Nbytes--;
		if (Nbytes <= 0)
		{
			LIB_DBG(DBG_ERR, "[CCPARS][%d] PES Data Size is ERROR! (Data Size = %d Byte(s))\n", __LINE__, Nbytes);
			return CCPARS_ERROR_BAD_DATA_SIZE;
		}

		BITPARS_GetBits(stDriverInfo, 8);
		ptrCurrCap->last_data_group_link_number = stDriverInfo->ucCurrent;

		/* ------------------------------------------------------------------ */
		Nbytes -= 2;
		if (Nbytes <= 0)
		{
			LIB_DBG(DBG_ERR, "[CCPARS][%d] PES Data Size is ERROR! (Data Size = %d Byte(s))\n", __LINE__, Nbytes);
			return CCPARS_ERROR_BAD_DATA_SIZE;
		}

		BITPARS_GetBits(stDriverInfo, 16);
		ptrCurrCap->data_group_size = stDriverInfo->uiCurrent;
		if (ptrCurrCap->data_group_size > MAX_SIZE_OF_CAPTION_PES)
		{
			LIB_DBG(DBG_ERR, "Data Group Size(%d) exceeds %d bytes!\n", \
					ptrCurrCap->data_group_size, MAX_SIZE_OF_CAPTION_PES);
			return (-9);
		}

		/* Caption Management Data. */
		if (CAPTION_DATA_MANAGE == data_group_caption_data_type)
		{
			/* -------------------------------------------------------------- */
			Nbytes--;
			if (Nbytes <= 0)
			{
				LIB_DBG(DBG_ERR, "[CCPARS][%d] PES Data Size is ERROR! (Data Size = %d Byte(s))\n", __LINE__, Nbytes);
				return CCPARS_ERROR_BAD_DATA_SIZE;
			}

			BITPARS_GetBits(stDriverInfo, 2);
			ptrCurrCap->DMnge.TMD = stDriverInfo->ucCurrent;

			BITPARS_GetBits(stDriverInfo, 6);	// reserved

			if (ptrCurrCap->DMnge.TMD == TMD_OFFSET_TIME)
			{
				/* ---------------------------------------------------------- */
				Nbytes -= 5;
				if (Nbytes <= 0)
				{
					LIB_DBG(DBG_ERR, "[CCPARS][%d] PES Data Size is ERROR! (Data Size = %d Byte(s))\n", __LINE__, Nbytes);
					return CCPARS_ERROR_BAD_DATA_SIZE;
				}

				BITPARS_GetBits(stDriverInfo, 24);
				ptrCurrCap->DMnge.OTM_time = stDriverInfo->ulCurrent;
				BITPARS_GetBits(stDriverInfo, 12);
				ptrCurrCap->DMnge.OTM_etc = stDriverInfo->uiCurrent;
				BITPARS_GetBits(stDriverInfo, 4);	// reserved
			}

			/* -------------------------------------------------------------- */
			Nbytes--;
			if (Nbytes <= 0)
			{
				LIB_DBG(DBG_ERR, "[CCPARS][%d] PES Data Size is ERROR! (Data Size = %d Byte(s))\n", __LINE__, Nbytes);
				return CCPARS_ERROR_BAD_DATA_SIZE;
			}

			BITPARS_GetBits(stDriverInfo, 8);
			ptrCurrCap->DMnge.num_languages = stDriverInfo->ucCurrent;
			if (ptrCurrCap->DMnge.num_languages > MAX_NUM_OF_ISDBT_CAP_LANG)
			{
				LIB_DBG(DBG_ERR, "Management Data Language Numbers [%d] exceeds %d!\n", ptrCurrCap->DMnge.num_languages, MAX_NUM_OF_ISDBT_CAP_LANG);
				return (-11);
			}

			for (j = 0; j < (int)ptrCurrCap->DMnge.num_languages; j++)
			{
				/* ---------------------------------------------------------- */
				Nbytes--;
				if (Nbytes <= 0)
				{
					LIB_DBG(DBG_ERR, "[CCPARS][%d] PES Data Size is ERROR! (Data Size = %d Byte(s))\n", __LINE__, Nbytes);
					return CCPARS_ERROR_BAD_DATA_SIZE;
				}

				BITPARS_GetBits(stDriverInfo, 3);
				ptrCurrCap->DMnge.LCode[j].language_tag = stDriverInfo->ucCurrent;
				if (ptrCurrCap->DMnge.LCode[j].language_tag >= MAX_NUM_OF_ISDBT_CAP_LANG)
				{
					LIB_DBG(DBG_ERR, "Management Data %d Language Tag is Inavlid! [%d] / [%d]\n", \
						j, ptrCurrCap->DMnge.LCode[j].language_tag, (MAX_NUM_OF_ISDBT_CAP_LANG - 1));
					return (-12);
				}

				BITPARS_GetBits(stDriverInfo, 1);	//reserved

				BITPARS_GetBits(stDriverInfo, 4);
				ptrCurrCap->DMnge.LCode[j].DMF = stDriverInfo->ucCurrent;
				if ( (ptrCurrCap->DMnge.LCode[j].DMF == (DMF_RECEPTION_SPECIFIC_CONDITION | DMF_RECORDING_PLAYBACK_AUTO)) ||
				     (ptrCurrCap->DMnge.LCode[j].DMF == (DMF_RECEPTION_SPECIFIC_CONDITION | DMF_RECORDING_PLAYBACK_NON_DISP_AUTO)) ||
				     (ptrCurrCap->DMnge.LCode[j].DMF == (DMF_RECEPTION_SPECIFIC_CONDITION | DMF_RECORDING_PLAYBACK_SELECT_DISP)) )
				{
					/* ------------------------------------------------------ */
					Nbytes--;
					if (Nbytes <= 0)
					{
						LIB_DBG(DBG_ERR, "[CCPARS][%d] PES Data Size is ERROR! (Data Size = %d Byte(s))\n", __LINE__, Nbytes);
						return CCPARS_ERROR_BAD_DATA_SIZE;
					}

					BITPARS_GetBits(stDriverInfo, 8);
					ptrCurrCap->DMnge.LCode[j].DC = stDriverInfo->ucCurrent;
				}

				/* ---------------------------------------------------------- */
				Nbytes -= 3;
				if (Nbytes <= 0)
				{
					LIB_DBG(DBG_ERR, "[CCPARS][%d] PES Data Size is ERROR! (Data Size = %d Byte(s))\n", __LINE__, Nbytes);
					return CCPARS_ERROR_BAD_DATA_SIZE;
				}

				BITPARS_GetBits(stDriverInfo, 24);
				ptrCurrCap->DMnge.LCode[j].ISO_639_language_code = stDriverInfo->ulCurrent;

				/* ---------------------------------------------------------- */
				Nbytes--;
				if (Nbytes <= 0)
				{
					LIB_DBG(DBG_ERR, "[CCPARS][%d] PES Data Size is ERROR! (Data Size = %d Byte(s))\n", __LINE__, Nbytes);
					return CCPARS_ERROR_BAD_DATA_SIZE;
				}

				BITPARS_GetBits(stDriverInfo, 4);
				ptrCurrCap->DMnge.LCode[j].Format = stDriverInfo->ucCurrent;

				switch(ptrCurrCap->DMnge.LCode[j].Format)
				{
					case 0x8: p_sub_ctx->disp_info.disp_format = SUB_DISP_H_960X540;	break;
					case 0x9: p_sub_ctx->disp_info.disp_format = SUB_DISP_V_960X540;	break;
					case 0xa: p_sub_ctx->disp_info.disp_format = SUB_DISP_H_720X480;	break;
					case 0xb: p_sub_ctx->disp_info.disp_format = SUB_DISP_V_720X480;	break;
					default: p_sub_ctx->disp_info.disp_format = SUB_DISP_H_320X60;	break;
				}

				LIB_DBG(DBG_INFO, "[%s] disp_format : %d\n", __func__, p_sub_ctx->disp_info.disp_format );

				BITPARS_GetBits(stDriverInfo, 2);
				ptrCurrCap->DMnge.LCode[j].TCS = stDriverInfo->ucCurrent;
				if (ptrCurrCap->DMnge.LCode[j].TCS != TCS_8BITS_CODE)
				{
					LIB_DBG(DBG_ERR, "Character Coding is NOT 8bit-code! (0x%02X)\n", ptrCurrCap->DMnge.LCode[j].TCS);
					return (-17);
				}

				BITPARS_GetBits(stDriverInfo, 2);
				ptrCurrCap->DMnge.LCode[j].rollup_mode = stDriverInfo->ucCurrent;

				if(ptrCurrCap->DMnge.LCode[j].rollup_mode == 0x01){
					p_sub_ctx->disp_info.disp_rollup = 1;
				}else{
					p_sub_ctx->disp_info.disp_rollup = 0;
				}
			}

			/* -------------------------------------------------------------- */
			Nbytes -= 3;
			if (Nbytes <= 0)
			{
				LIB_DBG(DBG_ERR, "[CCPARS][%d] PES Data Size is ERROR! (Data Size = %d Byte(s))\n", __LINE__, Nbytes);
				return CCPARS_ERROR_BAD_DATA_SIZE;
			}

			BITPARS_GetBits(stDriverInfo, 24);
			ptrCurrCap->DMnge.data_unit_loop_length = stDriverInfo->ulCurrent;

			if (ptrCurrCap->DMnge.data_unit_loop_length > MAX_SIZE_OF_CAPTION_PES)
			{
				LIB_DBG(DBG_ERR, "Management Data Unit Length(%d) exceeds %d bytes.\n", \
					ptrCurrCap->DState.data_unit_loop_length, MAX_SIZE_OF_CAPTION_PES);
				return (-171);
			}

			if (ptrCurrCap->DMnge.data_unit_loop_length > 0)
			{
				/* ---------------------------------------------------------- */
				Nbytes -= ptrCurrCap->DMnge.data_unit_loop_length;
				if (Nbytes <= 0)
				{
					LIB_DBG(DBG_ERR, "[CCPARS][%d] PES Data Size is ERROR! (Data Size = %d Byte(s))\n", __LINE__, Nbytes);
					return CCPARS_ERROR_BAD_DATA_SIZE;
				}

				*DUnitNum = CCPARS_Parse_DataUnit( handle,
				                                   (void*)stDriverInfo->pucRawData,
				                                   data_group_caption_data_type,
				                                   (TS_PES_CAP_DATAUNIT **)&ptrCurrCap->DMnge.DUnit,
				                                   ptrCurrCap->DMnge.data_unit_loop_length );
			}
		}
		/* Caption Statement Data */
		else if ((CAPTION_DATA_STATE_1ST_LANG <= data_group_caption_data_type) && (data_group_caption_data_type <= CAPTION_DATA_STATE_8TH_LANG))
		{
			/* -------------------------------------------------------------- */
			Nbytes--;
			if (Nbytes <= 0)
			{
				LIB_DBG(DBG_ERR, "[CCPARS][%d] PES Data Size is ERROR! (Data Size = %d Byte(s))\n", __LINE__, Nbytes);
				return CCPARS_ERROR_BAD_DATA_SIZE;
			}

			BITPARS_GetBits(stDriverInfo, 2);
			ptrCurrCap->DState.TMD = stDriverInfo->ucCurrent;

			BITPARS_GetBits(stDriverInfo, 6);	// reserved

			if ((ptrCurrCap->DState.TMD == TMD_REAL_TIME) || (ptrCurrCap->DState.TMD == TMD_OFFSET_TIME))
			{
				/* ---------------------------------------------------------- */
				Nbytes -= 5;
				if (Nbytes <= 0)
				{
					LIB_DBG(DBG_ERR, "[CCPARS][%d] PES Data Size is ERROR! (Data Size = %d Byte(s))\n", __LINE__, Nbytes);
					return CCPARS_ERROR_BAD_DATA_SIZE;
				}

				BITPARS_GetBits(stDriverInfo, 24);
				ptrCurrCap->DState.STM_time = stDriverInfo->ulCurrent;

				BITPARS_GetBits(stDriverInfo, 12);
				ptrCurrCap->DState.STM_etc = stDriverInfo->uiCurrent;

				BITPARS_GetBits(stDriverInfo, 4);	// reserved
			}

			/* -------------------------------------------------------------- */
			Nbytes -= 3;
			if (Nbytes <= 0)
			{
				LIB_DBG(DBG_ERR, "[CCPARS][%d] PES Data Size is ERROR! (Data Size = %d Byte(s))\n", __LINE__, Nbytes);
				return CCPARS_ERROR_BAD_DATA_SIZE;
			}

			BITPARS_GetBits(stDriverInfo, 24);
			ptrCurrCap->DState.data_unit_loop_length = stDriverInfo->ulCurrent;
			if (ptrCurrCap->DState.data_unit_loop_length > MAX_SIZE_OF_CAPTION_PES)
			{
				LIB_DBG(DBG_ERR, "Statement Data Unit Length(%d) exceeds %d bytes!\n", \
					ptrCurrCap->DState.data_unit_loop_length, MAX_SIZE_OF_CAPTION_PES);
				return (-20);
			}

			if (ptrCurrCap->DState.data_unit_loop_length > 0)
			{
				/* ---------------------------------------------------------- */
				Nbytes -= ptrCurrCap->DState.data_unit_loop_length;
				if (Nbytes <= 0)
				{
					LIB_DBG(DBG_ERR, "[CCPARS][%d] PES Data Size is ERROR! (Data Size = %d Byte(s))\n", __LINE__, Nbytes);
					return CCPARS_ERROR_BAD_DATA_SIZE;
				}

				*DUnitNum = CCPARS_Parse_DataUnit( handle,
				                                   stDriverInfo->pucRawData,
				                                   data_group_caption_data_type,
				                                   (TS_PES_CAP_DATAUNIT **)&ptrCurrCap->DState.DUnit,
				                                   ptrCurrCap->DState.data_unit_loop_length );

				/* At least one data unit should be placed! */
				if (*DUnitNum <= 0)
				{
					LIB_DBG(DBG_ERR, "There is NOT The Statement Data Unit!\n");
					return (-21);
				}
			}
		}
		else /* Don't cate! */
		{
			LIB_DBG(DBG_ERR, "Caption Data Type of Data Group ID is Inavlid! (0x%02X)\n", data_group_caption_data_type);
			return (-22);
		}

		/* ------------------------------------------------------------------ */
		Nbytes -= 2;
		if (Nbytes < 0)
		{
			LIB_DBG(DBG_ERR, "[CCPARS][%d] PES Data Size is ERROR! (Data Size = %d Byte(s))\n", __LINE__, Nbytes);
			return CCPARS_ERROR_BAD_DATA_SIZE;
		}

		BITPARS_GetBits(stDriverInfo, 16);
		ptrCurrCap->CRC_16 = stDriverInfo->uiCurrent;

		ptrPrevCap = ptrCurrCap;
	}

	if (stDriverInfo->fLengthError == 1)
	{
		LIB_DBG(DBG_ERR, "[CCPARS][%d] WARNING!! Bit Parser Length Error!\n", __LINE__);
	}

	return i;
}


/**************************************************************************
*  FUNCTION NAME :
*      CCPARS_Deallocate_CCData
*
*  DESCRIPTION : Free the data group memory.
*      ...
*
*  INPUT :
*  	   ...
*
*  OUTPUT :
*      ...
*
*  REMARK :
*      None
**************************************************************************/
int	CCPARS_Deallocate_CCData(void *ptr)
{
	TS_PES_CAP_DATAGROUP	*ptrCurrCap = NULL, *ptrNextCap = NULL;
	TS_PES_CAP_DATAUNIT		*ptrCurrDUNIT = NULL, *ptrNextDUNIT = NULL;

	if (ptr == NULL)
	{
		return (0);// error;
	}

	ptrCurrCap = (TS_PES_CAP_DATAGROUP *)ptr;
	while (ptrCurrCap != NULL)
	{
		ptrNextCap = ptrCurrCap->ptrNextDGroup;

		/* Statement Data */
		if (ptrCurrCap->DState.DUnit != NULL)
		{
			ptrCurrDUNIT = (TS_PES_CAP_DATAUNIT *)ptrCurrCap->DState.DUnit;
			while (ptrCurrDUNIT != NULL)
			{
				ptrNextDUNIT = (TS_PES_CAP_DATAUNIT *)ptrCurrDUNIT->ptrNextDUnit;
				if (ptrCurrDUNIT->pData != NULL)
				{
					tcc_mw_free(__FUNCTION__, __LINE__, ptrCurrDUNIT->pData);
					//CC_PRINTF("[1MPEGPARSE_ISDBT:%d] mem free pData Ptr [0x%08x]\n", __LINE__, ptrCurrDUNIT);
				}
				tcc_mw_free(__FUNCTION__, __LINE__, ptrCurrDUNIT);
				//CC_PRINTF("[1MPEGPARSE_ISDBT:%d] mem free DUnit Ptr [0x%08x]\n", __LINE__, ptrCurrDUNIT);
				ptrCurrDUNIT = ptrNextDUNIT;
			}
		}

		/* Management Data */
		if (ptrCurrCap->DMnge.DUnit != NULL)
		{
			ptrCurrDUNIT = (TS_PES_CAP_DATAUNIT *)ptrCurrCap->DMnge.DUnit;
			while (ptrCurrDUNIT != NULL)
			{
				ptrNextDUNIT = (TS_PES_CAP_DATAUNIT *)ptrCurrDUNIT->ptrNextDUnit;
				if (ptrCurrDUNIT->pData != NULL)
				{
					tcc_mw_free(__FUNCTION__, __LINE__, ptrCurrDUNIT->pData);
					//CC_PRINTF("[1MPEGPARSE_ISDBT:%d] mem free pData Ptr [0x%08x]\n", __LINE__, ptrCurrDUNIT);
				}
				tcc_mw_free(__FUNCTION__, __LINE__, ptrCurrDUNIT);
				//CC_PRINTF("[1MPEGPARSE_ISDBT:%d] mem free DUnit Ptr [0x%08x]\n", __LINE__, ptrCurrCap->Data);
				ptrCurrDUNIT = ptrNextDUNIT;
			}
		}
		//CC_PRINTF("[2MPEGPARSE_ISDBT:%d] mem free DGroup Ptr [0x%08x]\n", __LINE__, ptrCurrCap);
		tcc_mw_free(__FUNCTION__, __LINE__, ptrCurrCap);
		ptrCurrCap = ptrNextCap;
	}

	return (1);
}

void CCPARS_Color_Set(T_SUB_CONTEXT *p_sub_ctx, int mngr, unsigned int ucControlType, unsigned int param1, unsigned int param2, unsigned int param3)
{
	unsigned char index = 0;
	T_CHAR_DISP_INFO *param = (T_CHAR_DISP_INFO*)&p_sub_ctx->disp_info;

	/* param1: fg or bg color , param2 : transparancy , param : color value */
	LIB_DBG(DBG_C1, "Color Setting!(%d,%d,%d)\n", param1, param2, param3);

	if (param1 == (unsigned int)E_SET_COLOR_FORE){
		index = (unsigned char)((param->uiPaletteNumber<<4)|(param3&0xf));
		LIB_DBG(DBG_C1, "E_SET_COLOR_FORE(palette:0x%X, CMLA:0x%X, index:%d)\n", param->uiPaletteNumber, param3, index);

		if(param2 == COLOR_SET_FULL){
			param->uiForeColor = g_CMLA_Table[index];
		}else if(param2 == COLOR_SET_HALF){
			param->uiHalfForeColor = g_CMLA_Table[index];
		}else{
			LIB_DBG(DBG_ERR, "E_DISP_CTRL_SET_COLOR - Invalid intensity of color(%d)\n", param2);
		}
	}else if (param1 == (unsigned int)E_SET_COLOR_BACK){
		index = (unsigned char)((param->uiPaletteNumber<<4)|(param3&0xf));
		LIB_DBG(DBG_C1, "E_SET_COLOR_BACK(palette:0x%X, CMLA:0x%X, index:%d)\n", param->uiPaletteNumber, param3, index);
		if(param2 == COLOR_SET_FULL){
			param->uiBackColor = (unsigned int)g_CMLA_Table[index];
		}else if(param2 == COLOR_SET_HALF){
			param->uiHalfBackColor = (unsigned int)g_CMLA_Table[index];
		}else{
			LIB_DBG(DBG_ERR, "E_DISP_CTRL_SET_COLOR - Invalid intensity of color(%d)\n", param2);
		}
	}
	LIB_DBG(DBG_C1, "E_DISP_CTRL_SET_COLOR - [0x%08X:0x%08X - 0x%08X:0x%08X]\n",
						param->uiForeColor, param->uiBackColor,
						param->uiHalfForeColor, param->uiHalfBackColor);
}

void *CCPARS_Control_Process(T_SUB_CONTEXT *p_sub_ctx, int mngr, void *handle, void *pRawData, T_CC_CHAR_INFO *pstCharInfo)
{
	t_BITPARSER_DRIVER_INFO_	*stDriverInfo;
	int			dtv_mode, iRet;
	int data_type;
	unsigned char	ucCode;
	unsigned int	uiTmpCode;
	unsigned char 	*ptrRawData;

	if (handle == NULL) return NULL;

	data_type = p_sub_ctx->disp_info.data_type;

	dtv_mode = CCPARS_Get_DtvMode(data_type);

	ptrRawData = pRawData;

	stDriverInfo = (t_BITPARSER_DRIVER_INFO_ *)handle;
	BITPARS_InitBitParser(stDriverInfo, ptrRawData, 9);

	pstCharInfo->CharSet = CS_NONE;

	BITPARS_GetBits(stDriverInfo, 8);
	ucCode = stDriverInfo->ucCurrent;

	if((mngr == 0) && (p_sub_ctx->ctrl_char_num == 0)){
		ISDBTCap_ControlCharDisp(p_sub_ctx, mngr, (unsigned int)E_DISP_CTRL_INIT, (ucCode == ARIB_CS), 0, 0);
	}
	p_sub_ctx->ctrl_char_num++;

	LIB_DBG(DBG_INFO, "==>> ISDBT_CAPTION] Control Code = 0x%X\n", ucCode);

	switch (ucCode)
	{
		/****************************************************************************
		  * C0 controls set
		  ****************************************************************************/

		case ARIB_NUL:
			LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_NUL\n");
			/* Insert the blank. */
			//ISDBTCap_ControlCharDisp(p_sub_ctx, (unsigned int)E_DISP_CTRL_SPACE, 0, 0, 0);
			break;

		case ARIB_BEL:
			LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_BEL\n");
			if ((dtv_mode != ONESEG_ISDB_T) && (dtv_mode != ONESEG_SBTVD_T))
			{
				ISDBTCap_ControlCharDisp(p_sub_ctx, mngr, (unsigned int)E_DISP_CTRL_ALERT, 0, 0, 0);
			}
			break;

		case ARIB_APB:
			LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_APB\n");
			if ((dtv_mode != ONESEG_ISDB_T) && (dtv_mode != ONESEG_SBTVD_T))
			{
				//ISDBTCap_ControlCharDisp(p_sub_ctx, mngr, E_DISP_CTRL_ACTPOS_BACKWARD, 0, 0, 0);
				CCPARS_Parse_Pos_Backward(p_sub_ctx, mngr, 0, 0);
			}
			break;

		case ARIB_APF:
			LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_APF\n");
			if ((dtv_mode != ONESEG_ISDB_T) && (dtv_mode != ONESEG_SBTVD_T))
			{
				//ISDBTCap_ControlCharDisp(p_sub_ctx, mngr, E_DISP_CTRL_ACTPOS_FORWARD, 0, 0, 0);
				CCPARS_Parse_Pos_Forward(p_sub_ctx, mngr, 1, 0);
			}
			break;

		case ARIB_APD:
			LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_APD\n");
			if ((dtv_mode != ONESEG_ISDB_T) && (dtv_mode != ONESEG_SBTVD_T))
			{
				//ISDBTCap_ControlCharDisp(p_sub_ctx, mngr, (unsigned int)E_DISP_CTRL_ACTPOS_DOWN, -1, 0, 0);
				CCPARS_Parse_Pos_Down(p_sub_ctx, mngr, 1, 0);
			}
			break;

		case ARIB_APU:
			LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_APU\n");
			if ((dtv_mode != ONESEG_ISDB_T) && (dtv_mode != ONESEG_SBTVD_T))
			{
				//DBTCap_ControlCharDisp(p_sub_ctx, mngr, (unsigned int)E_DISP_CTRL_ACTPOS_UP, -1, 0, 0);
				CCPARS_Parse_Pos_Up(p_sub_ctx, mngr, 1, 0);
			}
			break;

		case ARIB_APR:
			LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_APR\n");
			//ISDBTCap_ControlCharDisp(p_sub_ctx, mngr, (unsigned int)E_DISP_CTRL_CRLF, 0, 0, 0);
			CCPARS_Parse_Pos_CRLF(p_sub_ctx, mngr, 0, 0);
			break;

		case ARIB_PAPF:
			LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_PAPF\n");
			if ((dtv_mode != ONESEG_ISDB_T) && (dtv_mode != ONESEG_SBTVD_T))
			{
				BITPARS_GetBits(stDriverInfo, 8);
				ucCode = stDriverInfo->ucCurrent;
				if ((0x40 <= ucCode) && (ucCode <= 0x7F))
				{
					unsigned int uiTimesOfForward = 0;

					uiTimesOfForward = (unsigned int)(BIT_MASK_OF_B6_TO_B1 & ucCode); // 0 ~ 63
					if(uiTimesOfForward){
						//ISDBTCap_ControlCharDisp(p_sub_ctx, mngr, E_DISP_CTRL_ACTPOS_FORWARD, 0, uiTimesOfForward, 0);
						CCPARS_Parse_Pos_Forward(p_sub_ctx, mngr, uiTimesOfForward, 0);
					}
				}
			}
			break;

		case ARIB_APS:
			LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_APS\n");
			if ((dtv_mode != ONESEG_ISDB_T) && (dtv_mode != ONESEG_SBTVD_T))
			{
				unsigned int uiDownCount = 0;
				unsigned int uiForwardCount = 0;

				BITPARS_GetBits(stDriverInfo, 8);
				ucCode = stDriverInfo->ucCurrent;

				LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_APS (DN:0x%02X)\n", ucCode);

				if ((0x40 <= ucCode) && (ucCode <= 0x7F))
				{
					uiDownCount = (unsigned int)(BIT_MASK_OF_B6_TO_B1 & ucCode); // 0 ~ 63
					if(uiDownCount <= 63){
						CCPARS_Parse_Pos_Down(p_sub_ctx, mngr, uiDownCount, 1);
					}
				}

				BITPARS_GetBits(stDriverInfo, 8);
				ucCode = stDriverInfo->ucCurrent;

				LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_APS (FD:0x%02X)\n", ucCode);

				if ((0x40 <= ucCode) && (ucCode <= 0x7F))
				{
					uiForwardCount = (unsigned int)(BIT_MASK_OF_B6_TO_B1 & ucCode); // 0 ~ 63
					if(uiForwardCount <= 63){
						CCPARS_Parse_Pos_Forward(p_sub_ctx, mngr, uiForwardCount, 1);
					}
				}
			}
			break;

		case ARIB_CS:
			LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_CS\n");
			p_sub_ctx->need_new_handle = 1;
			p_sub_ctx->disp_info.usDispMode &= ~(CHAR_DISP_MASK_DOUBLE_WIDTH|CHAR_DISP_MASK_DOUBLE_HEIGHT|
													CHAR_DISP_MASK_HALF_WIDTH | CHAR_DISP_MASK_HALF_HEIGHT|
												CHAR_DISP_MASK_UNDERLINE);
			ISDBTCap_ControlCharDisp(p_sub_ctx, mngr, (unsigned int)E_DISP_CTRL_CLR_SCR, 0, 0, 0);
			break;

		case ARIB_CAN:
			LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_CAN\n");
			if ((dtv_mode != ONESEG_ISDB_T) && (dtv_mode != ONESEG_SBTVD_T))
			{
				ISDBTCap_ControlCharDisp(p_sub_ctx, mngr, E_DISP_CTRL_CLR_LINE, E_CLR_LINE_FROM_CUR_POS, 0, 0);
			}
			break;

		case ARIB_ESC:
			LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_ESC\n");
			if (dtv_mode != ONESEG_ISDB_T)
			{
				stDriverInfo->pucRawData = CCPARS_Parse_CtrlCodeESC(data_type, handle, stDriverInfo->pucRawData);
				if (stDriverInfo->pucRawData == NULL)
				{
					return NULL;//BITPARSER_DRV_ERR_INVALID_HANDLE;
				}
			}
			break;

		case ARIB_LS1:
			LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_LS1\n");
			if (dtv_mode != ONESEG_ISDB_T)
			{
				CTRLCODE_LS1(data_type);
			}
			break;

		case ARIB_LS0:
			LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_LS0\n");
			if (dtv_mode != ONESEG_ISDB_T)
			{
				CTRLCODE_LS0(data_type);
			}
			break;

		case ARIB_SS2:
			LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_SS2\n");
			if (dtv_mode != ONESEG_ISDB_T)
			{
				g_SingleShift[data_type] = 1;
				g_OldGL[data_type] = stGSet[data_type].GL;
				CTRLCODE_SS2(data_type);
			}
			break;

		case ARIB_SS3:
			LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_SS3\n");
			if (dtv_mode == ONESEG_SBTVD_T)
			{
				/* 2007-11
				** 	DRAFT STANDARD SBTVD FORUM TECHNICAL MODULE - N06-1
				**     11.6 Subtitles and Superimposed Characters
				**         * G3 is used by means of SS3(0x1D)
				**           SS3 means to invoke one G3 code following to it in the GL area temporary.
				*/

				/* Set the characters set to G3 one. */
				pstCharInfo->CharSet = stGSet[data_type].G[CE_G3];

				/* Get one G3 code. */
				BITPARS_GetBits(stDriverInfo, 8);
				pstCharInfo->CharCode[0] = stDriverInfo->ucCurrent;
			}
			else
			{
				LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_SS3\n");
				if (dtv_mode != ONESEG_ISDB_T)
				{
					g_SingleShift[data_type] = 1;
					g_OldGL[data_type] = stGSet[data_type].GL;
					CTRLCODE_SS3(data_type);
				}
			}
			break;

		case ARIB_RS:
			LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_RS\n");
			if ((dtv_mode != ONESEG_ISDB_T) && (dtv_mode != ONESEG_SBTVD_T))
			{
				/* --------------------------------------------------
				** TBD - Not Implement
				** -------------------------------------------------- */
			}
			break;

		case ARIB_US:
			LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_US\n");
			if (dtv_mode == ONESEG_ISDB_T)
			{
				/* --------------------------------------------------
				** Used for identification of data unit,
				** but cannot be used at 8-bit character codes for C-profile.
				** -------------------------------------------------- */
			}
			else
			{
				/* --------------------------------------------------
				** TBD - Not Implement
				** -------------------------------------------------- */
			}
			break;


		/****************************************************************************
		  * Other controls set
		  ****************************************************************************/

		case ARIB_SP:
			LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_SP\n");
			ISDBTCap_ControlCharDisp(p_sub_ctx, mngr, (unsigned int)E_DISP_CTRL_SPACE, 0, 0, 0);
			break;

		case ARIB_DEL:
			LIB_DBG(DBG_C0, " CONTROL-CODE] ARIB_DEL\n");
			ISDBTCap_ControlCharDisp(p_sub_ctx, mngr, (unsigned int)E_DISP_CTRL_DELETE, 0, 0, 0);
			break;


		/****************************************************************************
		  * C1 controls set
		  ****************************************************************************/

		case ARIB_BKF: // Black Foreground Color
			LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_BKF\n");
			CCPARS_Color_Set(p_sub_ctx, mngr, E_DISP_CTRL_SET_COLOR, E_SET_COLOR_FORE, COLOR_SET_FULL, E_CMLA_0);
			break;

		case ARIB_RDF: // Red Foreground Color
			LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_RDF\n");
			CCPARS_Color_Set(p_sub_ctx, mngr, E_DISP_CTRL_SET_COLOR, E_SET_COLOR_FORE, COLOR_SET_FULL, E_CMLA_1);
			break;

		case ARIB_GRF: // Green Foreground Color
			LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_GRF\n");
			CCPARS_Color_Set(p_sub_ctx, mngr, E_DISP_CTRL_SET_COLOR, E_SET_COLOR_FORE, COLOR_SET_FULL, E_CMLA_2);
			break;

		case ARIB_YLF: // Yellow Foreground Color
			LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_YLF\n");
			CCPARS_Color_Set(p_sub_ctx, mngr, E_DISP_CTRL_SET_COLOR, E_SET_COLOR_FORE, COLOR_SET_FULL, E_CMLA_3);
			break;

		case ARIB_BLF: // Blue Foreground Color
			LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_BLF\n");
			CCPARS_Color_Set(p_sub_ctx, mngr, E_DISP_CTRL_SET_COLOR, E_SET_COLOR_FORE, COLOR_SET_FULL, E_CMLA_4);
			break;

		case ARIB_MGF: // Magenta Foreground Color
			LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_MGF\n");
			CCPARS_Color_Set(p_sub_ctx, mngr, E_DISP_CTRL_SET_COLOR, E_SET_COLOR_FORE, COLOR_SET_FULL, E_CMLA_5);
			break;

		case ARIB_CNF: // Cyan Foreground Color
			LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_CNF\n");
			CCPARS_Color_Set(p_sub_ctx, mngr, E_DISP_CTRL_SET_COLOR, E_SET_COLOR_FORE, COLOR_SET_FULL, E_CMLA_6);
			break;

		case ARIB_WHF: // White Foreground Color
			LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_WHF\n");
			CCPARS_Color_Set(p_sub_ctx, mngr, E_DISP_CTRL_SET_COLOR, E_SET_COLOR_FORE, COLOR_SET_FULL, E_CMLA_7);
			break;

		case ARIB_COL: // Color Controls
			LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_COL\n");
			if ((dtv_mode != ONESEG_ISDB_T) && (dtv_mode != ONESEG_SBTVD_T))
			{
				BITPARS_GetBits(stDriverInfo, 8);
				ucCode = stDriverInfo->ucCurrent;

				LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_COL(0x%02X)\n", ucCode);

				if ((0x48 <= ucCode) && (ucCode <= 0x7F))
				{
					unsigned int uiColorType;
					unsigned int uiSetType;
					unsigned int uiCMLA;

					uiColorType = (ucCode < 0x60) ? COLOR_SET_FULL : COLOR_SET_HALF;
					uiSetType = (((ucCode & 0xF0) == 0x40) || ((ucCode & 0xF0) == 0x60)) ? E_SET_COLOR_FORE : E_SET_COLOR_BACK;
					uiCMLA = (ucCode & BIT_MASK_OF_B4_TO_B1);
					CCPARS_Color_Set(p_sub_ctx, mngr, E_DISP_CTRL_SET_COLOR, uiSetType, uiColorType, uiCMLA);
				}
				else if (ucCode == 0x20)
				{
					BITPARS_GetBits(stDriverInfo, 8);
					ucCode = stDriverInfo->ucCurrent;

					LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_COL - PALETTE (0x%02X)\n", ucCode);

					if ((0x40 <= ucCode) && (ucCode <= 0x4F))
					{
						unsigned char ucPaletteNumber;

						ucPaletteNumber = (ucCode & BIT_MASK_OF_B4_TO_B1);
						p_sub_ctx->disp_info.uiPaletteNumber = ucPaletteNumber;
					}
				}
			}
			break;

		case ARIB_POL:
			LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_POL\n");
			if ((dtv_mode != ONESEG_ISDB_T) && (dtv_mode != ONESEG_SBTVD_T))
			{
				unsigned int uiTmpColor = 0;
				T_CHAR_DISP_INFO *param = (T_CHAR_DISP_INFO*)&p_sub_ctx->disp_info;

				BITPARS_GetBits(stDriverInfo, 8);
				ucCode = stDriverInfo->ucCurrent;

				if (ucCode == 0x40){
					LIB_DBG(DBG_C1, "[POL0] cur[0x%08X, 0x%08X, 0x%08X, 0x%08X]\n",
							param->uiForeColor, param->uiBackColor, param->uiHalfForeColor, param->uiHalfBackColor);

					if(param->uiPolChanges){
						param->uiForeColor = param->uiOriForeColor;
						param->uiBackColor = param->uiOriBackColor;
						param->uiHalfForeColor = param->uiOriHalfForeColor;
						param->uiHalfBackColor = param->uiOriHalfBackColor;
					}

					LIB_DBG(DBG_C1, "[POL0] 0x%08X, 0x%08X, 0x%08X, 0x%08X\n",
							param->uiForeColor, param->uiBackColor, param->uiHalfForeColor, param->uiHalfBackColor);
				}else if (ucCode == 0x41){
					LIB_DBG(DBG_C1, "[POL1] cur[0x%08X, 0x%08X, 0x%08X, 0x%08X]\n",
							param->uiForeColor, param->uiBackColor, param->uiHalfForeColor, param->uiHalfBackColor);

					if(param->uiPolChanges == 0){
						param->uiOriForeColor = param->uiForeColor;
						param->uiOriBackColor = param->uiBackColor;
						param->uiOriHalfForeColor = param->uiHalfForeColor;
						param->uiOriHalfBackColor = param->uiHalfBackColor;
						param->uiPolChanges = 1;
					}

					uiTmpColor = param->uiForeColor;
					param->uiForeColor = param->uiBackColor;
					param->uiBackColor = uiTmpColor;

					uiTmpColor = param->uiHalfForeColor;
					param->uiHalfForeColor = param->uiHalfBackColor;
					param->uiHalfBackColor = uiTmpColor;

					LIB_DBG(DBG_C1, "[POL1] 0x%08X, 0x%08X, 0x%08X, 0x%08X\n",
							param->uiForeColor, param->uiBackColor, param->uiHalfForeColor, param->uiHalfBackColor);
				}else if (ucCode == 0x42){
					LIB_DBG(DBG_ERR, "[POL2] Not used.\n");
				}
			}
			break;

		case ARIB_SSZ: // Character Size is SMALL.
			LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_SSZ\n");
			p_sub_ctx->disp_info.usDispMode &= ~(CHAR_DISP_MASK_DOUBLE_WIDTH|CHAR_DISP_MASK_DOUBLE_HEIGHT|
													CHAR_DISP_MASK_HALF_WIDTH | CHAR_DISP_MASK_HALF_HEIGHT);
			p_sub_ctx->disp_info.usDispMode |= (CHAR_DISP_MASK_HALF_WIDTH | CHAR_DISP_MASK_HALF_HEIGHT);
			break;

		case ARIB_MSZ: // Character Size is MIDDLE.
			LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_MSZ\n");
			p_sub_ctx->disp_info.usDispMode &= ~(CHAR_DISP_MASK_DOUBLE_WIDTH|CHAR_DISP_MASK_DOUBLE_HEIGHT|
													CHAR_DISP_MASK_HALF_WIDTH | CHAR_DISP_MASK_HALF_HEIGHT);
			p_sub_ctx->disp_info.usDispMode |= (CHAR_DISP_MASK_HALF_WIDTH);
			break;

		case ARIB_NSZ: // Character Size is NORMAL.
			LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_NSZ\n");
			p_sub_ctx->disp_info.usDispMode &= ~(CHAR_DISP_MASK_DOUBLE_WIDTH|CHAR_DISP_MASK_DOUBLE_HEIGHT|
													CHAR_DISP_MASK_HALF_WIDTH | CHAR_DISP_MASK_HALF_HEIGHT);
			break;

		case ARIB_SZX: // Character Size Control
			LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_SZX\n");
			if ((dtv_mode != ONESEG_ISDB_T) && (dtv_mode != ONESEG_SBTVD_T))
			{
				BITPARS_GetBits(stDriverInfo, 8);
				ucCode = stDriverInfo->ucCurrent;
				LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_SZX (0x%X)\n", ucCode);

				p_sub_ctx->disp_info.usDispMode &= ~(CHAR_DISP_MASK_DOUBLE_WIDTH|CHAR_DISP_MASK_DOUBLE_HEIGHT|
													CHAR_DISP_MASK_HALF_WIDTH | CHAR_DISP_MASK_HALF_HEIGHT);

				if(ucCode == ARIB_SZX_DH){
					p_sub_ctx->disp_info.usDispMode |= CHAR_DISP_MASK_DOUBLE_HEIGHT;
				}else if(ucCode == ARIB_SZX_DW){
					p_sub_ctx->disp_info.usDispMode |= CHAR_DISP_MASK_DOUBLE_WIDTH;
				}else if(ucCode == ARIB_SZX_DHDW){
					p_sub_ctx->disp_info.usDispMode |= (CHAR_DISP_MASK_DOUBLE_WIDTH|CHAR_DISP_MASK_DOUBLE_HEIGHT);
				}
			}
			break;

		case ARIB_FLC:
			LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_FLC\n");
			BITPARS_GetBits(stDriverInfo, 8);
			ucCode = stDriverInfo->ucCurrent;
			if ((ucCode == ARIB_FLC_START_NORMAL) || (ucCode == ARIB_FLC_START_INVERTED)){
				p_sub_ctx->disp_info.usDispMode |= CHAR_DISP_MASK_FLASHING;
					p_sub_ctx->disp_info.disp_flash = 1;
			}
			else {
				p_sub_ctx->disp_info.usDispMode &= ~CHAR_DISP_MASK_FLASHING;
			}
			break;

		case ARIB_CDC:
			LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_CDC\n");
			if ((dtv_mode != ONESEG_ISDB_T) && (dtv_mode != ONESEG_SBTVD_T))
			{
				BITPARS_GetBits(stDriverInfo, 8);
				ucCode = stDriverInfo->ucCurrent;
				if (ucCode == 0x40)
				{
					/* START Single Concealment Mode */
				}
				else if (ucCode == 0x20)
				{
					BITPARS_GetBits(stDriverInfo, 8);
					ucCode = stDriverInfo->ucCurrent;
					if ((0x40 <= ucCode) && (ucCode <= 0x4A))
					{
						/* START Replacing Conceal Mode */
					}
					else
					{
						//LIB_DBG(DBG_C1, "X - ARIB_CDC - Replacing Conceal - OTHERS (0x%02X)\n", ucCode);
					}
				}
				else if (ucCode == 0x4F)
				{
					/* STOP Concealment Mode */
				}
				else
				{
					//LIB_DBG(DBG_C1, "X - ARIB_CDC - OTHERS (0x%02X)\n", ucCode);
				}
			}
			break;

		case ARIB_WMM:
			LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_WMM\n");
			if ((dtv_mode != ONESEG_ISDB_T) && (dtv_mode != ONESEG_SBTVD_T))
			{
				BITPARS_GetBits(stDriverInfo, 8);
				ucCode = stDriverInfo->ucCurrent;
				if (ucCode == 0x40)
				{
					/* WMM 1 */
				}
				else if (ucCode == 0x44)
				{
					/* WMM 2 */
				}
				else if (ucCode == 0x45)
				{
					/* WMM 3 */
				}
				else
				{
					//LIB_DBG(DBG_C1, "X - ARIB_WMM - OTHERS (0x%02X)\n", ucCode);
				}
			}
			break;

		case ARIB_TIME: // Time Controls
			LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_TIME\n");
			BITPARS_GetBits(stDriverInfo, 8);
			ucCode = stDriverInfo->ucCurrent;
			if (ucCode == 0x20)
			{
				unsigned int uiTimeCtrlPTS = 0;

				//LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_TIME - Pass 1\n");

				BITPARS_GetBits(stDriverInfo, 8);
				ucCode = stDriverInfo->ucCurrent;
				if ((0x40 <= ucCode) && (ucCode <= 0x7F))
				{
					uiTimeCtrlPTS = (unsigned int)(BIT_MASK_OF_B6_TO_B1 & ucCode) * 100; // 100 : 0.1 sec
					iRet = ISDBTCap_ControlCharDisp(p_sub_ctx, mngr, (unsigned int)E_DISP_CTRL_NEW_FIFO, 0, 0, 0);
					if (iRet != 1)
					{
						return NULL;
					}
					ISDBTCap_ControlCharDisp(p_sub_ctx, mngr, E_DISP_CTRL_CHANGE_PTS, E_PTS_CHANGE_INC, uiTimeCtrlPTS, 0);
				}
			}
			else if (ucCode == 0x28)
			{
				LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_TIME - Pass 2\n");

				if ((dtv_mode != ONESEG_ISDB_T) && (dtv_mode != ONESEG_SBTVD_T))
				{
					BITPARS_GetBits(stDriverInfo, 8);
					ucCode = stDriverInfo->ucCurrent;
					if (ucCode == ARIB_TIME_FREE)
					{
						/* --------------------------------------------------
						** TBD - Time Control Mode : Free
						** -------------------------------------------------- */
					}
					else if (ucCode == ARIB_TIME_REAL)
					{
						/* --------------------------------------------------
						** TBD - Time Control Mode : Real
						** -------------------------------------------------- */
					}
					else if (ucCode == ARIB_TIME_OFFSET)
					{
						/* --------------------------------------------------
						** TBD - Time Control Mode : Offset
						** -------------------------------------------------- */
					}
					else if (ucCode == ARIB_TIME_UNIQUE)
					{
						/* --------------------------------------------------
						** TBD - Time Control Mode : Unique
						** -------------------------------------------------- */
					}
				}
			}
			else if (ucCode == 0x29)
			{
				/* TIME, P, P11~P1i, I1, P21~P2j, I2, P31~P3k, I3, P41~P4m, I, F P
				** P = 02/9
				** I1~I3 = 03//11
				** I = 02/0
				** F = 04/0 : Presentation Start Time, Playback Time
				**     04/1 : Offset Time
				**     04/2 : Performance Time
				**     04/3 : Display End Time
				** (* At performance time, 'I3 & P41~P4m' is not sent out.)
				*/
				int iParamCount = 0;
				int iParams[MAX_NUMS_OF_ARIB_TIME_PARAMS] = { 0, };

				LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_TIME - Pass 3\n");

				if ((dtv_mode != ONESEG_ISDB_T) && (dtv_mode != ONESEG_SBTVD_T))
				{
					/* Parse the ARIB_TIME parameters. */
					do
					{
						/* Get next character code. */
						BITPARS_GetBits(stDriverInfo, 8);
						ucCode = stDriverInfo->ucCurrent;

						while ((ucCode != ARIB_PARSING_MIDDLE_CHAR) && (ucCode != ARIB_PARSING_END_CHAR))
						{
							/* Save the parameter value. */
							iParams[iParamCount] = (iParams[iParamCount] * 10) + (int)(ucCode - 0x30);

							/* Get next character code. */
							BITPARS_GetBits(stDriverInfo, 8);
							ucCode = stDriverInfo->ucCurrent;
						}

						/* Increase the parameter count. */
						iParamCount++;
					} while (ucCode != ARIB_PARSING_END_CHAR);

					/* Get the final character code. */
					BITPARS_GetBits(stDriverInfo, 8);
					ucCode = stDriverInfo->ucCurrent;
					if (ucCode == 0x40)
					{
						/* --------------------------------------------------
						** TBD - Presentation Start Time, Playback Time
						** -------------------------------------------------- */
					}
					else if (ucCode == 0x41)
					{
						/* --------------------------------------------------
						** TBD - Offset Time
						** -------------------------------------------------- */
					}
					else if (ucCode == 0x42)
					{
						/* --------------------------------------------------
						** TBD - Performance Time
						** -------------------------------------------------- */
					}
					else if (ucCode == 0x43)
					{
						/* --------------------------------------------------
						** TBD - Display End Time
						** -------------------------------------------------- */
					}
					else
					{
						/* --------------------------------------------------
						** TBD - Unknown Code
						** -------------------------------------------------- */
					}

					BITPARS_GetBits(stDriverInfo, 8);
					ucCode = stDriverInfo->ucCurrent;
					if (ucCode == 0x29)
					{
						/* --------------------------------------------------
						** TBD - P : END
						** -------------------------------------------------- */
					}
					else
					{
						/* --------------------------------------------------
						** TBD - Unknown Code
						** -------------------------------------------------- */
					}
				}
			}
			break;

		case ARIB_MACRO:
			LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_MACRO\n");
			if ((dtv_mode != ONESEG_ISDB_T) && (dtv_mode != ONESEG_SBTVD_T))
			{
				BITPARS_GetBits(stDriverInfo, 8); // P1
				ucCode = stDriverInfo->ucCurrent;
				if (ucCode == 0x40)
				{
					/* Macro definition starts. */
					unsigned char ucPrevCode;

					do
					{
						ucPrevCode = ucCode;
						BITPARS_GetBits(stDriverInfo, 8);
						ucCode = stDriverInfo->ucCurrent;
					} while ((ucPrevCode != 0x95/*ARIB_MACRO*/) && (ucCode != 0x4F));
				}
				else if (ucCode == 0x41)
				{
					/* Macro definition starts and defined macro statement is executed once. */
					unsigned char ucPrevCode;

					do
					{
						ucPrevCode = ucCode;
						BITPARS_GetBits(stDriverInfo, 8);
						ucCode = stDriverInfo->ucCurrent;
					} while ((ucPrevCode != 0x95/*ARIB_MACRO*/) && (ucCode != 0x4F));
				}
				else if (ucCode == 0x4F)
				{
					/* The definition or execution of macro ends. */
					//LIB_DBG(DBG_C1, "    ARIB_MACRO - OOPS! First Code is Macro Stop! (0x%02X)\n", ucCode);
				}
				else
				{
					//LIB_DBG(DBG_C1, "@ - ARIB_MACRO - OTHERS (0x%02X)\n", ucCode);
				}
			}
			break;

		case ARIB_RPC:
			LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_RPC\n");
			if ((dtv_mode != ONESEG_ISDB_T) && (dtv_mode != ONESEG_SBTVD_T))
			{
				BITPARS_GetBits(stDriverInfo, 8);
				ucCode = stDriverInfo->ucCurrent;
				if (ucCode == 0x40){
					p_sub_ctx->disp_info.usDispMode |= CHAR_DISP_MASK_REPEAT;
				}
				else if ((0x41 <= ucCode) && (ucCode <= 0x7E)){
					p_sub_ctx->disp_info.uiRepeatCharCount = (unsigned int)(BIT_MASK_OF_B6_TO_B1 & ucCode);
				}
			}
			break;

		case ARIB_STL:
			LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_STL\n");
			if ((dtv_mode != ONESEG_ISDB_T) && (dtv_mode != ONESEG_SBTVD_T)){
				p_sub_ctx->disp_info.usDispMode |= CHAR_DISP_MASK_UNDERLINE;
			}
			break;

		case ARIB_SPL:
			LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_SPL\n");
			if ((dtv_mode != ONESEG_ISDB_T) && (dtv_mode != ONESEG_SBTVD_T)){
				p_sub_ctx->disp_info.usDispMode &= ~CHAR_DISP_MASK_UNDERLINE;
			}
			break;

		case ARIB_HLC: // Highlighting Character Block
			LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_HLC\n");
			if ((dtv_mode != ONESEG_ISDB_T) && (dtv_mode != ONESEG_SBTVD_T))
			{
				BITPARS_GetBits(stDriverInfo, 8);
				ucCode = stDriverInfo->ucCurrent;
				if ((0x40 <= ucCode) && (ucCode <= 0x4E))
				{
					unsigned int uiEnclosureInfo;

					uiEnclosureInfo = (unsigned int)(BIT_MASK_OF_B4_TO_B1 & ucCode);
					if(uiEnclosureInfo == 0){
						p_sub_ctx->disp_info.usDispMode &= ~CHAR_DISP_MASK_HIGHLIGHT;
					}else{
						p_sub_ctx->disp_info.usDispMode |= CHAR_DISP_MASK_HIGHLIGHT;
					}
					p_sub_ctx->disp_info.disp_highlight = uiEnclosureInfo;
				}
			}
			break;

		case ARIB_CSI: // Control Sequence Introducer
			LIB_DBG(DBG_C1, " CONTROL-CODE] ARIB_CSI\n");
			if ((dtv_mode != ONESEG_ISDB_T) && (dtv_mode != ONESEG_SBTVD_T))
			{
				stDriverInfo->pucRawData = CCPARS_Parse_CtrlCodeCSI(p_sub_ctx, mngr, handle, stDriverInfo->pucRawData);
			}
			break;

		default:
			if (CHECK_GR_CODE_RANGE(ucCode)) /* GR Code Range */
			{
				int gr = stGSet[data_type].GR;
				//CS_PRINTF(" CONTROL-CODE] GR Code Range\n");
				pstCharInfo->CharSet = stGSet[data_type].G[gr];
			}
			else if (CHECK_GL_CODE_RANGE(ucCode)) /* GL Code Range */
			{
				int gl = stGSet[data_type].GL;
				//CS_PRINTF(" CONTROL-CODE] GL Code Range\n");
				pstCharInfo->CharSet = stGSet[data_type].G[gl];
			}
			else
			{
				LIB_DBG(DBG_ERR, " CONTROL-CODE] Undefined Control Code! (0x%02X)\n", ucCode);
				break;
			}

			/* Set the 1st byte. */
			pstCharInfo->CharCode[0] = ucCode;

			/* Check the 2-Byte Code character set. */
			if ((pstCharInfo->CharSet == CS_KANJI) || (pstCharInfo->CharSet == CS_DRCS_0))
			{
				/* Get 1 byte. */
				BITPARS_GetBits(stDriverInfo, 8);
				ucCode = stDriverInfo->ucCurrent;

				/* Set the 2nd byte. */
				pstCharInfo->CharCode[1] = ucCode;
			}
			break;
	}

	return stDriverInfo->pucRawData;
}

static void *CCPARS_Alloc_DRCS_Font(DRCS_PATTERN_DATA *ptrPatternData)
{
	void *pointer;
	DRCS_FONT_DATA *ptrTempFontData;

	if (ptrPatternData == NULL)
	{
		LIB_DBG(DBG_ERR, "[ISDBT_Caption:%d] ptrPatternData is NULL!!\n", __LINE__);
		return NULL;
	}

	pointer = tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(DRCS_FONT_DATA));
	if (pointer == NULL)
	{
		LIB_DBG(DBG_ERR, "[ISDBT_Caption:%d] mem alloc error!!\n", __LINE__);
		return NULL;
	}
	memset(pointer, 0, sizeof(DRCS_FONT_DATA));

	if (ptrPatternData->ptrFontData == NULL)
	{
		ptrPatternData->ptrFontData = (DRCS_FONT_DATA *)pointer;
	}
	else
	{
		ptrTempFontData = ptrPatternData->ptrFontData;
		while(ptrTempFontData->ptrNext)
		{
			ptrTempFontData = (DRCS_FONT_DATA *)ptrTempFontData->ptrNext;
		}

		ptrTempFontData->ptrNext = pointer;
	}

	return pointer;
}

static void *CCPARS_Alloc_DRCS(void *ptr)
{
	void *pointer;
	ISDBT_CAPTION_DRCS *ptrCurrDRCS, *ptrPrevDRCS;

	pointer = (ISDBT_CAPTION_DRCS *)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(ISDBT_CAPTION_DRCS));
	if (pointer == NULL)
	{
		LIB_DBG(DBG_ERR, "[ISDBT_Caption:%d]mem alloc error!!\n", __LINE__);
		return NULL;
	}
	memset(pointer, 0, sizeof(ISDBT_CAPTION_DRCS));

	ptrCurrDRCS = (ISDBT_CAPTION_DRCS *)pointer;
	if (ptr == NULL)
	{
		ptrCurrDRCS->ptrPrev = NULL;
	}
	else
	{
		ptrPrevDRCS = (ISDBT_CAPTION_DRCS *)ptr;
		ptrPrevDRCS->ptrNext = ptrCurrDRCS;
		ptrCurrDRCS->ptrPrev = ptrPrevDRCS;
	}

	ptrCurrDRCS->ptrNext = NULL;

	return ptrCurrDRCS;
}

void *CCPARS_Dealloc_DRCS(void *ptr)
{
	ISDBT_CAPTION_DRCS *ptrCurrDRCS, *ptrPrevDRCS, *ptrNextDRCS;
	DRCS_FONT_DATA *ptrCurrFontData, *ptrNextFontData;

	if (ptr == NULL)
	{
		return NULL;//-1;// error;
	}

	ptrCurrDRCS = (ISDBT_CAPTION_DRCS *)ptr;
	ptrPrevDRCS = ptrCurrDRCS->ptrPrev;
	ptrNextDRCS = ptrCurrDRCS->ptrNext;

	if (ptrCurrDRCS->PatternHeader.ptrFontData != NULL)
	{
		ptrCurrFontData = ptrCurrDRCS->PatternHeader.ptrFontData;
		while(ptrCurrFontData)
		{
			ptrNextFontData = (DRCS_FONT_DATA *)ptrCurrFontData->ptrNext;

			if (ptrCurrFontData->patternData != NULL)
			{
				tcc_mw_free(__FUNCTION__, __LINE__, (void *)ptrCurrFontData->patternData);
			}

			if (ptrCurrFontData->geometricData != NULL)
			{
				tcc_mw_free(__FUNCTION__, __LINE__, (void *)ptrCurrFontData->geometricData);
			}

			tcc_mw_free(__FUNCTION__, __LINE__, (void *)ptrCurrFontData);

			ptrCurrFontData = ptrNextFontData;
		}
	}

	tcc_mw_free(__FUNCTION__, __LINE__, ptrCurrDRCS);

	if (ptrPrevDRCS != NULL)
	{
		if (ptrNextDRCS != NULL)
		{
			ptrPrevDRCS->ptrNext = ptrNextDRCS;
			ptrNextDRCS->ptrPrev = ptrPrevDRCS;
			return ptrPrevDRCS;
		}
		else
		{
			ptrPrevDRCS->ptrNext = NULL;
			return ptrPrevDRCS;
		}
	}
	else if (ptrNextDRCS != NULL)
	{
		ptrNextDRCS->ptrPrev = NULL;
		return ptrNextDRCS;
	}

	return NULL;
}

void *CCPARS_Get_DRCS(unsigned char *pCharCode, unsigned int iCharSet, unsigned int iWidth, unsigned int iHeight)
{
	ISDBT_CAPTION_DRCS *ptrCurrDRCS;
	DRCS_FONT_DATA *ptrTempFontData;
	unsigned short iCharCode, iCharCodeMask;

	if (iCharSet == CS_DRCS_0) {
		iCharCode = (pCharCode[0] & 0x7F) << 8;
		iCharCode |= (pCharCode[1] & 0x7F);
		iCharCodeMask = 0xFFFF;
	} else {
		iCharCode = (pCharCode[0] & 0x7F);
		iCharCodeMask = 0x00FF;
	}

	ptrCurrDRCS = gDRCSBaseStart[iCharSet - CS_DRCS_0];
	while (ptrCurrDRCS != NULL)
	{
		if (iCharCode == (ptrCurrDRCS->PatternHeader.CharacterCode & iCharCodeMask))
		{
			ptrTempFontData = ptrCurrDRCS->PatternHeader.ptrFontData;
			if (ptrTempFontData->ptrNext == NULL)
			{
				LIB_DBG(DBG_INFO, "CCPARS_Get_DRCS Success [0x%04X, %d(%d), %d(%d)]\n",iCharCode, iWidth, ptrTempFontData->width, iHeight, ptrTempFontData->height);
				return ptrTempFontData;
			}
			else
			{
				while(ptrTempFontData)
				{
					if ((iWidth == ptrTempFontData->width) && (iHeight == ptrTempFontData->height))
					{
						LIB_DBG(DBG_INFO, "CCPARS_Get_DRCS Success [0x%04X, %d, %d]\n",iCharCode, iWidth, iHeight);
						return ptrTempFontData;
					}

					ptrTempFontData = (DRCS_FONT_DATA *)ptrTempFontData->ptrNext;
				}
			}
		}

		ptrCurrDRCS = ptrCurrDRCS->ptrNext;
	}

	LIB_DBG(DBG_ERR, "[CCPARS][%d] CCPARS_Get_DRCS Fail[0x%04X, %d, %d]\n", __LINE__, iCharCode, iWidth, iHeight);

	return NULL;
}

void *CCPARS_Control_DRCS(void *handle, void *pRawData, int RawDataSize, int ByteType)
{
	ISDBT_CAPTION_DRCS			*pDRCSPrev[16];
	ISDBT_CAPTION_DRCS			*pDRCSBase = NULL;
	t_BITPARSER_DRIVER_INFO_	*stDriverInfo;
	DRCS_PATTERN_DATA			*drcs_pattern = NULL;
	DRCS_FONT_DATA				*drcs_font = NULL;

	void			*pointer;
	unsigned char 	*ptrRawData;
	unsigned int	i, j, k;
	unsigned int	NumberOfCode;
	unsigned int 	NumberOfDRCS;
	int				iPatternDataSize;
	int				iPixelNumsPerByte;
	int 			iCode;
	int				iRawDataBytes;

	if(handle == NULL)
	{
		LIB_DBG(DBG_ERR, "[CCPARS][%d] DRCS handle is ERROR!\n", __LINE__);
		return NULL;
	}

	ptrRawData = pRawData;
	stDriverInfo = (t_BITPARSER_DRIVER_INFO_ *)handle;
	BITPARS_InitBitParser(stDriverInfo, ptrRawData, RawDataSize);
	iRawDataBytes = RawDataSize;

	iRawDataBytes--;
	if (iRawDataBytes < 0)
	{
		LIB_DBG(DBG_ERR, "[CCPARS][%d] DRCS Data Size is ERROR! (Data Size = %d Byte(s))\n", __LINE__, iRawDataBytes);
		return NULL;
	}

	memset(pDRCSPrev, 0, sizeof(void*)*16);

	BITPARS_GetBits(stDriverInfo, 8);
	NumberOfCode = stDriverInfo->ucCurrent;

	for (i = 0; i < NumberOfCode; i++)
	{
		BITPARS_GetBits(stDriverInfo, 16);

		if (ByteType == ISDBT_DUNIT_1BYTE_DRCS)
		{
			NumberOfDRCS = ((stDriverInfo->uiCurrent & 0x0000FF00) >> 8);
			if(NumberOfDRCS < GS_DRCS_1 || NumberOfDRCS > GS_DRCS_15)
			{
				LIB_DBG(DBG_ERR, "[CCPARS][%d] DRCS Number is ERROR! (Number = %d)\n", __LINE__, NumberOfDRCS);
				return NULL;
			}
			NumberOfDRCS -= GS_DRCS_0;
		}
		else
		{
			NumberOfDRCS = 0;
		}

		if (pDRCSPrev[NumberOfDRCS] == NULL)
		{
			pDRCSPrev[NumberOfDRCS] = gDRCSBaseStart[NumberOfDRCS];
			while (pDRCSPrev[NumberOfDRCS] != NULL){
				pDRCSPrev[NumberOfDRCS] = CCPARS_Dealloc_DRCS(pDRCSPrev[NumberOfDRCS]);
			}
			gDRCSBaseStart[NumberOfDRCS] = NULL;

			pDRCSPrev[NumberOfDRCS] = gDRCSBaseStart[NumberOfDRCS];
		}

		pDRCSBase = CCPARS_Alloc_DRCS(pDRCSPrev[NumberOfDRCS]);
		if (pDRCSBase == NULL)
		{
			LIB_DBG(DBG_ERR, "Failed to allocate New DRCS Structure Memory!\n");
			return NULL;
		}

		if (gDRCSBaseStart[NumberOfDRCS] == NULL)
		{
			gDRCSBaseStart[NumberOfDRCS] = pDRCSBase;
		}

		drcs_pattern = &pDRCSBase->PatternHeader;
		drcs_pattern->CharacterCode = stDriverInfo->uiCurrent;
		iRawDataBytes -= 2;
		if (iRawDataBytes <= 0)
		{
			LIB_DBG(DBG_ERR, "[CCPARS][%d] DRCS Data Size is ERROR! (Data Size = %d Byte(s))\n", __LINE__, iRawDataBytes);
			return NULL;
		}

		iRawDataBytes--;
		if (iRawDataBytes < 0)
		{
			LIB_DBG(DBG_ERR, "[CCPARS][%d] DRCS Data Size is ERROR! (Data Size = %d Byte(s))\n", __LINE__, iRawDataBytes);
			return NULL;
		}

		LIB_DBG(DBG_INFO, "DRCS:0x%02X, CharCode:0x%04X\n", NumberOfDRCS, drcs_pattern->CharacterCode);
		BITPARS_GetBits(stDriverInfo, 8);
		drcs_pattern->NumberOfFont = stDriverInfo->ucCurrent;
		for (j = 0; j < drcs_pattern->NumberOfFont; j++)
		{
			drcs_font = (DRCS_FONT_DATA *)CCPARS_Alloc_DRCS_Font(drcs_pattern);
			if (drcs_font == NULL){
				LIB_DBG(DBG_ERR, "[CCPARS][%d] drcs_font is NULL\n", __LINE__);
				return NULL;
			}

			iRawDataBytes--;
			if (iRawDataBytes <= 0)
			{
				LIB_DBG(DBG_ERR, "[CCPARS][%d] DRCS Data Size is ERROR! (Data Size = %d Byte(s))\n", __LINE__, iRawDataBytes);
				return NULL;
			}

			BITPARS_GetBits(stDriverInfo, 4);
			drcs_font->fontId = stDriverInfo->ucCurrent;
			BITPARS_GetBits(stDriverInfo, 4);
			drcs_font->mode = stDriverInfo->ucCurrent;

			if ((drcs_font->mode == 0x00) || (drcs_font->mode == 0x01))
			{
				iRawDataBytes -= 3;
				if (iRawDataBytes <= 0)
				{
					LIB_DBG(DBG_ERR, "[CCPARS][%d] DRCS Data Size is ERROR! (Data Size = %d Byte(s))\n", __LINE__, iRawDataBytes);
					return NULL;
				}

				BITPARS_GetBits(stDriverInfo, 8);
				drcs_font->depth = stDriverInfo->ucCurrent;
				BITPARS_GetBits(stDriverInfo, 8);
				drcs_font->width = stDriverInfo->ucCurrent;
				BITPARS_GetBits(stDriverInfo, 8);
				drcs_font->height =stDriverInfo->ucCurrent;
				LIB_DBG(DBG_INFO, "%d, %d, %d\n", drcs_font->depth, drcs_font->width, drcs_font->height);

				iPixelNumsPerByte = (drcs_font->mode == 0x00) ? 8 : 4;
				iPatternDataSize = ((drcs_font->width * drcs_font->height) + (iPixelNumsPerByte - 1)) / iPixelNumsPerByte;
				if (iPatternDataSize > iRawDataBytes)
				{
					LIB_DBG(DBG_ERR, "Pattern Data Size(%d) is more than remainded Raw Data Length(%d)!\n", iPatternDataSize, iRawDataBytes);
					return NULL;
				}
				else if (iPatternDataSize > 0)
				{
					iRawDataBytes -= iPatternDataSize;

					pointer = (unsigned char *)tcc_mw_malloc(__FUNCTION__, __LINE__, (unsigned int)iPatternDataSize);
					if (pointer == NULL)
					{
						LIB_DBG(DBG_ERR, "Failed to allocate The DRCS Data Memory!\n");
						return NULL;
					}

					drcs_font->patternData = (unsigned char *)pointer;
					memcpy(drcs_font->patternData, stDriverInfo->pucRawData, iPatternDataSize);
				#if (1)
					for (k = 0; k < (unsigned int)iPatternDataSize; k++)
					{
						BITPARS_DecrementGlobalLength(handle);
					}
				#else
					stDriverInfo->pucRawData += iPatternDataSize;
					stDriverInfo->iGlobalLength -= iPatternDataSize;
				#endif
				}
			}
			else
			{
				iRawDataBytes -= 4;
				if (iRawDataBytes < 0)
				{
					LIB_DBG(DBG_ERR, "[CCPARS][%d] DRCS Data Size is ERROR! (Data Size = %d Byte(s))\n", __LINE__, iRawDataBytes);
					return NULL;
				}

				BITPARS_GetBits(stDriverInfo, 8);
				drcs_font->regionX = stDriverInfo->ucCurrent;
				BITPARS_GetBits(stDriverInfo, 8);
				drcs_font->regionY = stDriverInfo->ucCurrent;
				BITPARS_GetBits(stDriverInfo, 16);
				drcs_font->geometricData_length = stDriverInfo->uiCurrent;
				if (drcs_font->geometricData_length > 0)
				{
					iRawDataBytes -= (int)drcs_font->geometricData_length;
					if (iRawDataBytes < 0)
					{
						LIB_DBG(DBG_ERR, "[CCPARS][%d] DRCS Data Size is ERROR! (Data Size = %d Byte(s))\n", __LINE__, iRawDataBytes);
						return NULL;
					}

					for (k = 0; k < (int)drcs_font->geometricData_length; k++)
					{
						BITPARS_GetBits(stDriverInfo, 8);
					}
				}
			}
		}

		pDRCSPrev[NumberOfDRCS] = pDRCSBase;
	}

	if (i != NumberOfCode)
	{
		LIB_DBG(DBG_ERR, "[CCPARS][%d] WARNING!! DRCS Code Number is BAD! (TARGET = %d, SOURCE = %d)\n", __LINE__, NumberOfCode, i);
	}

	if (iRawDataBytes != 0)
	{
		LIB_DBG(DBG_ERR, "[CCPARS][%d] WARNING!! Ramain DRCS Raw Data is NOT 0! (%d)\n", __LINE__, iRawDataBytes);
	}

	return stDriverInfo->pucRawData;
}

int CCPARS_Check_SS(int data_type)
{
	if(g_SingleShift[data_type]){
		LIB_DBG(DBG_INFO, "[CCPARS:%d] restore GL(%d) -> GL(%d)\n", __LINE__, stGSet[data_type].GL, g_OldGL[data_type]);
		stGSet[data_type].GL = g_OldGL[data_type];
		g_SingleShift[data_type] = 0;
	}

	return 0;
}

void CCPARS_Macro_Process(T_SUB_CONTEXT *p_sub_ctx, unsigned char *pCharCode)
{
	int data_type;

	LIB_DBG(DBG_INFO, "[CCPARS:%d] macro:0x%X\n", __LINE__, *pCharCode);

	if(p_sub_ctx == NULL){
		LIB_DBG(DBG_ERR, "[%s] Null pointer error\n", __func__);
		return;
}

	data_type = p_sub_ctx->disp_info.data_type;

	switch(*pCharCode)
	{
		case 0x60:
			MACRO_GSET(data_type, CS_KANJI, CS_ALPHANUMERIC, CS_HIRAGANA, CS_MACRO, CE_G0, CE_G2);
			break;
		case 0x61:
			MACRO_GSET(data_type, CS_KANJI, CS_KATAKANA, CS_HIRAGANA, CS_MACRO, CE_G0, CE_G2);
			break;
		case 0x62:
			MACRO_GSET(data_type, CS_KANJI, CS_DRCS_1, CS_HIRAGANA, CS_MACRO, CE_G0, CE_G2);
			break;
		case 0x63:
			MACRO_GSET(data_type, CS_MOSAIC_A, CS_MOSAIC_C, CS_MOSAIC_D, CS_MACRO, CE_G0, CE_G2);
			break;
		case 0x64:
			MACRO_GSET(data_type, CS_MOSAIC_A, CS_MOSAIC_B, CS_MOSAIC_D, CS_MACRO, CE_G0, CE_G2);
			break;
		case 0x65:
			MACRO_GSET(data_type, CS_MOSAIC_A, CS_DRCS_1, CS_MOSAIC_D, CS_MACRO, CE_G0, CE_G2);
			break;
		case 0x66:
			MACRO_GSET(data_type, CS_DRCS_1, CS_DRCS_2, CS_DRCS_3, CS_MACRO, CE_G0, CE_G2);
			break;
		case 0x67:
			MACRO_GSET(data_type, CS_DRCS_4, CS_DRCS_5, CS_DRCS_6, CS_MACRO, CE_G0, CE_G2);
			break;
		case 0x68:
			MACRO_GSET(data_type, CS_DRCS_7, CS_DRCS_8, CS_DRCS_9, CS_MACRO, CE_G0, CE_G2);
			break;
		case 0x69:
			MACRO_GSET(data_type, CS_DRCS_10, CS_DRCS_11, CS_DRCS_12, CS_MACRO, CE_G0, CE_G2);
			break;
		case 0x6a:
			MACRO_GSET(data_type, CS_DRCS_13, CS_DRCS_14, CS_DRCS_15, CS_MACRO, CE_G0, CE_G2);
			break;
		case 0x6b:
			MACRO_GSET(data_type, CS_KANJI, CS_DRCS_2, CS_HIRAGANA, CS_MACRO, CE_G0, CE_G2);
			break;
		case 0x6c:
			MACRO_GSET(data_type, CS_KANJI, CS_DRCS_3, CS_HIRAGANA, CS_MACRO, CE_G0, CE_G2);
			break;
		case 0x6d:
			MACRO_GSET(data_type, CS_KANJI, CS_DRCS_4, CS_HIRAGANA, CS_MACRO, CE_G0, CE_G2);
			break;
		case 0x6e:
			MACRO_GSET(data_type, CS_KATAKANA, CS_HIRAGANA, CS_ALPHANUMERIC, CS_MACRO, CE_G0, CE_G2);
			break;
		case 0x6f:
			MACRO_GSET(data_type, CS_ALPHANUMERIC, CS_MOSAIC_A, CS_DRCS_1, CS_MACRO, CE_G0, CE_G2);
			break;
	}
}


int CCPARS_Bitmap_Process(T_SUB_CONTEXT *p_sub_ctx, void *handle, void *pRawData, int size)
{
	t_BITPARSER_DRIVER_INFO_ *stDriverInfo;
	unsigned char *ptrRawData=NULL;
	T_BITMAP_DATA_TYPE	bmp_data;
	int i = 0;

	if ((p_sub_ctx == NULL)||(handle == NULL) || (pRawData == NULL) || (size == 0)){
		LIB_DBG(DBG_ERR, "[%s] p_sub_ctx:#%p, handle:#%p, raw:#%p, size:%d\n", __func__, \
					p_sub_ctx, handle, pRawData, size);
		return -1;
	}

	ptrRawData = pRawData;

	stDriverInfo = (t_BITPARSER_DRIVER_INFO_ *)handle;
	BITPARS_InitBitParser(stDriverInfo, ptrRawData, size);

	memset(&bmp_data, 0x0, sizeof(T_BITMAP_DATA_TYPE));

	BITPARS_GetBits(stDriverInfo, 16);
	bmp_data.x = (short)stDriverInfo->uiCurrent;
	size -= 2;

	BITPARS_GetBits(stDriverInfo, 16);
	bmp_data.y = (short)stDriverInfo->uiCurrent;
	size -= 2;

	if(bmp_data.x < 0){
		bmp_data.x = 0;
	}

	if(bmp_data.y < 0){
		bmp_data.y = 0;
	}

	BITPARS_GetBits(stDriverInfo, 8);
	bmp_data.flc_idx_num = stDriverInfo->ucCurrent;

	LIB_DBG(DBG_INFO, "[%s] x:%d, y:%d, flc_idx_num:%d\n", __func__, \
		bmp_data.x, bmp_data.y, bmp_data.flc_idx_num);

	if(bmp_data.flc_idx_num != 0){
		bmp_data.p_flc_idx_grp = (char*)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(char)*bmp_data.flc_idx_num);
		if(bmp_data.p_flc_idx_grp == NULL){
			LIB_DBG(DBG_ERR, "[%s] allocation error\n", __func__);
			goto END;
		}
	}

	for(i=0 ; i<bmp_data.flc_idx_num ; i++){
		BITPARS_GetBits(stDriverInfo, 8);
		size -= 1;
		bmp_data.p_flc_idx_grp[i] = stDriverInfo->ucCurrent;
	}
	bmp_data.size = size;
	bmp_data.p_data = (char*)stDriverInfo->pucRawData;

	ISDBTCap_ControlCharDisp(p_sub_ctx, 0, E_DISP_CTRL_BMP, 0, 0, (unsigned int)&bmp_data);

END:
	if(bmp_data.p_flc_idx_grp){
		tcc_mw_free(__FUNCTION__, __LINE__, bmp_data.p_flc_idx_grp);
	}

	return 0;
}
