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

 ****************************************************************************/

#ifndef _TCC_DXB_MULTI2_H_
#define _TCC_DXB_MULTI2_H_

#ifdef WIN32
typedef char			int8_t;
typedef unsigned char   uint8_t;
typedef short           int16_t;
typedef unsigned short  uint16_t;
typedef int				int32_t;
typedef unsigned int    uint32_t;
typedef __int64			int64_t;
typedef __int64			uint64_t;
#else
#include <stdint.h>
#endif

#define MULTI2_ERROR_INVALID_PARAMETER		-1
#define MULTI2_ERROR_UNSET_SYSTEM_KEY		-2
#define MULTI2_ERROR_UNSET_CBC_INIT			-3
#define MULTI2_ERROR_UNSET_SCRAMBLE_KEY		-4

#define MULTI2_STATE_CBC_INIT_SET			(0x0001)
#define MULTI2_STATE_SYSTEM_KEY_SET			(0x0002)
#define MULTI2_STATE_SCRAMBLE_KEY_SET		(0x0004)
//#define MULTI2_STATE_SCRAMBLE_KEY_SET2		(0x0008)


typedef struct
{
	uint32_t	key[8];
} CORE_PARAM;

typedef struct
{
	uint32_t	l;
	uint32_t	r;
} CORE_DATA;

typedef struct
{
	CORE_DATA	cbc_init;

	CORE_PARAM	sys;
	CORE_DATA	scr[2];	/* 0: odd, 1: even */
	CORE_PARAM	wrk[2];	/* 0: odd, 1: even */

	uint32_t	round;
	uint32_t	state;
} MULTI2_PRIVATE_DATA;

typedef struct
{
	void *private_data;

	int (* set_round)(void *m2, int32_t val);

	int (* set_system_key)(void *m2, uint8_t *val);
	int (* set_init_cbc)(void *m2, uint8_t *val);
	int (* set_scramble_key)(void *m2, uint8_t parity, uint8_t *val);
	int (* clear_scramble_key)(void *m2);

	int (* decrypt)(void *m2, int32_t type, uint8_t *buf, int32_t size);
} MULTI2;
void release_multi2(void *m2);
MULTI2 *create_multi2(void);
#endif
