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
#ifndef _TS_PARSE_H_
#define _TS_PARSE_H_

typedef unsigned int	BOOL;
typedef unsigned char	U8;
typedef signed char		S8;
typedef unsigned short	U16;
typedef signed short	S16;
typedef unsigned int	U32;
typedef signed int		S32;
typedef void			VOID;

#ifdef WIN32
#define	__func__		__FUNCTION__
#define LOGE(...) printf(__VA_ARGS__)
#define LOGD(...) printf(__VA_ARGS__)
#endif

#ifndef NULL
	#define NULL 	(0)
#endif

#ifndef	FALSE
#define	FALSE	(0)
#endif

#ifndef	TRUE
#define	TRUE	(!FALSE)
#endif

#define PRINTF


extern void* tcc_mw_malloc(const char* functionName, unsigned int line, unsigned int size);
extern void tcc_mw_free(const char* functionName, unsigned int line, void* ptr);


enum
{
	AUDIO_PES_PACKET = 0,
	VIDEO_PES_PACKET,
	RA_HEADER_PES_PACKET,	// used for RMVB
	PRIVATE_PES_PACKET,
	TELETEXT_PES_PACKET,
	DSMCC_PACKET,
	OTHER_PACKET,

	LAST_PES_PACKET
};

typedef enum
{
	SCAN_NONE_PID = 0x00,
	SCAN_PAT_PID  = 0x01,
	SCAN_PMT_PID  = 0x02,
	SCAN_SDT_PID  = 0x04,
	SCAN_DONE_PID = 0x08,
	SCAN_STATUS_MAX
} SCAN_STATUS;

#endif	// _TS_PARSE_H_

/* end of file */

