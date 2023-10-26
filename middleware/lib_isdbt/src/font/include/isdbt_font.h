/*******************************************************************************

*   FileName : isdbt_font.h

*   Copyright (c) Telechips Inc.

*   Description : isdbt_font

********************************************************************************
*
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



/*
 *                ***** NOTE *****
 * All inforamtion of this file should be consistent with an internal configuration of middleware.
 * Please don't modify anything.
 */

/*----- code table of character code set ------*/
#define	UNIPAGE_CODENUM		7808
	/* total row = 84 (0x21 ~ 0x74)
	 * cell per each row = 94
	 * cells for row84 = 6
	 * => (total_row - 1) * (cell per each row) + (cells for row84) */

#define USE_EUCJPMAP_FOR_LATINEXT_CODEMAPPING
#define USE_EUCJPMAP_FOR_SPECIAL_CODEMAPPING


/*----- vertical direction in caption & superimpose -----*/
enum {
	CS_NONE,
	CS_KANJI,
	CS_ALPHANUMERIC,
	CS_LATIN_EXTENSION,
	CS_SPECIAL,
	CS_HIRAGANA,
	CS_KATAKANA
};
typedef struct _VerRotCode_ {
	unsigned short usIn;	/* code values of character */ 
	unsigned short usOut;	/* mapping to font file */
} VerRotCode;

struct _VerRotTable_ {
	unsigned char	char_set;
	int total_no;
	VerRotCode *pVerRotCode;
};

