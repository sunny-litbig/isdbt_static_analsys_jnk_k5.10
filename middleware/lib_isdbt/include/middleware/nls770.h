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

#ifndef	__NLS770_H__
#define	__NLS770_H__

#define LANG_JAPANESE_INCLUDE

#if defined	(LANG_KOREAN_INCLUDE)		// Korean KSC 5601
#define	NLS_INCLUDE		1

#elif defined	(LANG_KOREAN_EXT_INCLUDE)	// Korean KSC 5601 and Extended Korean code
#define NLS_INCLUDE		1

#elif defined	(LANG_CHINESE_INCLUDE)		// Chinese Simplified same as LANG_CHINESE_SIMP_INCLUDE
#define NLS_INCLUDE		1

#elif defined	(LANG_CHINESE_SIMP_INCLUDE)	// Chinese Simplified
#define NLS_INCLUDE		1

#elif defined	(LANG_CHINESE_TRAD_INCLUDE)	// Chinese Traditional
#define	NLS_INCLUDE		1

#elif defined	(LANG_JAPANESE_INCLUDE)		// Japanese JIS X208
#define	NLS_INCLUDE		1

#elif defined	(LANG_ENGLISH_INCLUDE)		// English
#define	NLS_INCLUDE		1

#elif defined (LANG_MULTILINGUAL_INCLUDE)	// Multilingual Latin I
#define	NLS_INCLUDE		1

#elif defined	(LANG_RUSSIAN_INCLUDE)		// Cyrillic (CP_1251)
#define	NLS_INCLUDE		1

#elif defined	(LANG_CENTRAL_EUROPE_INCLUDE)	// Central Europe (CP_1250)
#define	NLS_INCLUDE		1

#elif defined	(LANG_LATIN_I_INCLUDE)		// Latin I (CP_1252)
#define	NLS_INCLUDE		1

#endif



#define CP_437      437     // MS-DOS United States 
//#define CP_737      737     // Greek
//#define CP_775      775     // Baltic 
#define CP_850      850     // MS-DOS Multilingual (Latin I) 
//#define CP_852      852     // MS-DOS Slavic (Latin II) 
//#define CP_855      855     // IBM Cyrillic (primarily Russian) 
//#define CP_857      857     // IBM Turkish 
//#define CP_860      860     // MS-DOS Portuguese 
//#define CP_861      861     // MS-DOS Icelandic 
//#define CP_862      862     // Hebrew 
//#define CP_863      863     // MS-DOS Canadian-French 
//#define CP_864      864     // Arabic 
//#define CP_865      865     // MS-DOS Nordic 
//#define CP_866      866     // MS-DOS Russian (former USSR) 
//#define CP_869      869     // IBM Modern Greek 
//#define CP_874      874     // Thai 
#define CP_932      932		// Japanese
#define CP_936      936     // Chinese (PRC, Singapore) 
#define CP_949      949     // Korean 
#define CP_950      950     // Chinese (Taiwan Region; Hong Kong SAR, PRC)  
#define CP_1250     1250    // Central Europe
#define CP_1251     1251    // Cyrillic
#define CP_1252     1252    // Latin I


/*For supporting DVB European Language
*/
#define CP_ISO6937			6937 
#define CP_ISO8859_1		88591      
#define CP_ISO8859_2		88592      
#define CP_ISO8859_3		88593      
#define CP_ISO8859_4		88594      
#define CP_ISO8859_5		88595      
#define CP_ISO8859_6		88596      
#define CP_ISO8859_7		88597      
#define CP_ISO8859_8		88598      
#define CP_ISO8859_9		88599      
#define CP_ISO8859_10		885910      
#define CP_ISO8859_11		885911      
#define CP_ISO8859_13		885913      
#define CP_ISO8859_14		885914      
#define CP_ISO8859_15		885915      
#define CP_ISO8859_16		885916

// Locale code
#define CP_ENGLISH			CP_437
#define CP_MULTILINGUAL	CP_850
#define CP_JAPANESE			CP_932
#define CP_CHINESE			CP_936
#define CP_KOREAN			CP_949

// Chinese Simplified and Traditional
#define CP_CHINESE_SIMP     CP_936
#define CP_CHINESE_TRAD     CP_950

#define CP_CENTRAL_EUROPE   CP_1250
#define CP_RUSSIAN          CP_1251
#define CP_LATIN_I			CP_1252


///////////////////////////////////////////////////////////////////////////////

#define UNKNOWN_ANSICODE    '?'
#define UNKNOWN_UNICODE     0xff1f			// full with '?'

///////////////////////////////////////////////////////////////////////////////


extern int NLS_nCurCodePage;
extern int (*char2uni)(unsigned char *ansi, unsigned short *uni);

#if defined(UNI2ANSI_INCLUDE)
extern int (*Uni2Ansi)(unsigned short uni, unsigned char *ansi);
#endif



#if defined(__cplusplus)
extern "C" {
#endif


enum{
	LANG_NONE,
	LANG_KOREAN,
	LANG_ENGLISH,
	LANG_MULTILANGUAL,
	LANG_JAPANESE,
	LANG_CHINESE_SIMP,
	LANG_CHINESE_TRAD,
	LANG_CENTRAL_EUROPE,
	LANG_RUSSIAN,
	LANG_LATIN_I,
	SAVE_FONT_FILE
};



void InitNLS(void);
int SetCodePage(int nCodePage);
int GetCodePage(void);

#if defined(__cplusplus)
}
#endif

#endif

/* end of file */

