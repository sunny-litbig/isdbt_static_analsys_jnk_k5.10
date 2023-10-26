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
#if defined(HAVE_LINUX_PLATFORM)
#define LOG_NDEBUG 0
#define LOG_TAG	"subtitle_parser"
#include <utils/Log.h>
#endif

#include <ISDBT_Caption.h>
#include <subtitle_parser.h>
#include <subtitle_debug.h>
#include <subtitle_display.h>

/****************************************************************************
DEFINITION
****************************************************************************/
#define CC_PRINTF	//LOGE
#define ERR_DBG		LOGE

#ifndef NULL
	#define NULL 0
#endif

/****************************************************************************
DEFINITION OF EXTERNAL VARIABLES
****************************************************************************/
extern ISDBT_CAPTION_INFO gIsdbtCapInfo[2];


/****************************************************************************
DEFINITION OF GLOVAL VARIABLES
****************************************************************************/


/****************************************************************************
DEFINITION OF LOCAL FUNCTIONS
****************************************************************************/
int ISDBTCC_Check_DataGroupData(T_SUB_CONTEXT *p_sub_ctx, TS_PES_CAP_DATAGROUP *pDataGroup)
{
	TS_PES_CAP_DATAGROUP	*pCurDataGroup = NULL;
	TS_PES_CAP_DATAUNIT 	*pCurrDataUnit = NULL;
	E_DTV_MODE_TYPE dtv_type;

	int iCount = 0;

	dtv_type = p_sub_ctx->disp_info.dtv_type;

	//CC_PRINTF(" ISDBT_CAPTION][%d] In %s\n", __LINE__, __func__);

	/* Parameter Check */
	if (NULL == pDataGroup)
	{
		ERR_DBG(" ISDBT_CAPTION][%d] invalid parameter\n", __LINE__);
		return ISDBT_CAPTION_INVALID_PARAMETER;
	}

	/* Set the first data group pointer. */
	pCurDataGroup = pDataGroup;
	while (pCurDataGroup != NULL)
	{
		/* Last Group Link Number Check */
		if (pCurDataGroup->last_data_group_link_number != 0)
		{
			CC_PRINTF("[%s] Group Link Number - Current:%d, Last:%d\n", __func__,
				pCurDataGroup->data_group_link_number, pCurDataGroup->last_data_group_link_number);
		}

		/* Caption Management Data */
		if ((pCurDataGroup->data_group_id & ISDBT_CAP_LANGUAGE_MASK) == ISDBT_CAP_MANAGE_DATA)
		{
			/* Check the real time. */
			if (pCurDataGroup->DMnge.TMD != 0x00)
			{
				ERR_DBG("[%s] Time Control Mode is NOT Real Time! [0x%02X]\n", __func__, pCurDataGroup->DMnge.TMD);
				return ISDBT_CAPTION_ERROR;
			}

			if((p_sub_ctx->disp_info.dtv_type == ONESEG_ISDB_T) || (p_sub_ctx->disp_info.dtv_type == ONESEG_SBTVD_T))
			{
				/* Check the language. */
				for (iCount = 0; iCount < pCurDataGroup->DMnge.num_languages; iCount++)
				{
					if ((ONESEG_ISDB_T == dtv_type)||(FULLSEG_ISDB_T == dtv_type))
					{
						if (pCurDataGroup->DMnge.LCode[iCount].ISO_639_language_code != ISO639_LANGUAGE_JAPANESE)
						{
							ERR_DBG("[%s] ISDBT Language Code is NOT 0x%08X! (0x%08X)\n", __func__,
								ISO639_LANGUAGE_JAPANESE, pCurDataGroup->DMnge.LCode[iCount].ISO_639_language_code);
							return ISDBT_CAPTION_ERROR;
						}
					}
					else if ((ONESEG_SBTVD_T == dtv_type)||(FULLSEG_SBTVD_T == dtv_type))
					{
						if (pCurDataGroup->DMnge.LCode[iCount].ISO_639_language_code != ISO639_LANGUAGE_PORTUGUESE)
						{
							ERR_DBG("[%s] SBTVD Language Code is NOT 0x%08X! (0x%08X)\n", __func__,
								ISO639_LANGUAGE_PORTUGUESE, pCurDataGroup->DMnge.LCode[iCount].ISO_639_language_code);
							return ISDBT_CAPTION_ERROR;
						}
					}
					else
					{
						ERR_DBG("[%s] 1-Seg Mode [%d] is Invalid!\n", __func__, dtv_type);
						return ISDBT_CAPTION_ERROR;
					}
				}
			}

			/* Check a caption management data. */
			if (pCurDataGroup->DMnge.DUnit != NULL)
			{
				//CC_PRINTF("[ISDBT_CAPTION][%d] The Caption Management Data has Data Unit!\n", __LINE__);

				pCurrDataUnit = (TS_PES_CAP_DATAUNIT *)pCurDataGroup->DMnge.DUnit;
				while (pCurrDataUnit != NULL)
				{
					CC_PRINTF("[%s] The Data Unit Parameter = [0x%02X]\n", __func__, pCurrDataUnit->data_unit_parameter);
					pCurrDataUnit = (TS_PES_CAP_DATAUNIT *)pCurrDataUnit->ptrNextDUnit;
				}
			}
		}
		/* Caption Statement Data */
		else
		{
			/* Check a caption satatement data */
			if (pCurDataGroup->DState.DUnit != NULL)
			{
				pCurrDataUnit = (TS_PES_CAP_DATAUNIT *)pCurDataGroup->DState.DUnit;
				while (pCurrDataUnit != NULL)
				{
					if ((ONESEG_ISDB_T == dtv_type)||(FULLSEG_ISDB_T == dtv_type))
					{
						if (	(pCurrDataUnit->data_unit_parameter != ISDBT_DUNIT_STATEMENT_BODY)
							 && (pCurrDataUnit->data_unit_parameter != ISDBT_DUNIT_1BYTE_DRCS)
							 && (pCurrDataUnit->data_unit_parameter != ISDBT_DUNIT_2BYTE_DRCS)
							 && (pCurrDataUnit->data_unit_parameter != ISDBT_DUNIT_BIT_MAP))
						{
							ERR_DBG("[%s] Data Unit Parameter is Invalid! [0x%02X]\n", __func__, pCurrDataUnit->data_unit_parameter);
							return ISDBT_CAPTION_ERROR;
						}
					}
					else if ((ONESEG_SBTVD_T == dtv_type)||(FULLSEG_SBTVD_T == dtv_type))
					{
						if (pCurrDataUnit->data_unit_parameter != ISDBT_DUNIT_STATEMENT_BODY)
						{
							ERR_DBG("[%s] Data Unit Parameter is Invalid! [0x%02X]\n", __func__, pCurrDataUnit->data_unit_parameter);
							return ISDBT_CAPTION_ERROR;
						}
					}
					else
					{
						ERR_DBG("[%s] 1-Seg Mode [%d] is Invalid!\n", __func__, dtv_type);
						return ISDBT_CAPTION_ERROR;
					}

					pCurrDataUnit = (TS_PES_CAP_DATAUNIT *)pCurrDataUnit->ptrNextDUnit;
				}
			}
		}

		pCurDataGroup = (TS_PES_CAP_DATAGROUP *)pCurDataGroup->ptrNextDGroup;
	}

	return ISDBT_CAPTION_SUCCESS;
}

void ISDBTCC_Handle_CaptionManagementData(int data_type, TS_PES_CAP_DATAGROUP *pDataGroup)
{
	int i;

	/* Handle the information of Caption Management Data. */
	if ((pDataGroup != NULL) && ((pDataGroup->data_group_id & ISDBT_CAP_LANGUAGE_MASK) == ISDBT_CAP_MANAGE_DATA))
	{
		/* Update the number of language. */
		if (gIsdbtCapInfo[data_type].ucNumsOfLangCode != pDataGroup->DMnge.num_languages)
		{
			CC_PRINTF("[%s] Change the Total Number of Language! (%d -> %d)\n", __func__,
				gIsdbtCapInfo[data_type].ucNumsOfLangCode, pDataGroup->DMnge.num_languages);

			/* Update the number of language. */
			gIsdbtCapInfo[data_type].ucNumsOfLangCode = pDataGroup->DMnge.num_languages;

			/* Set the language information. */
			for (i = 0; i < gIsdbtCapInfo[data_type].ucNumsOfLangCode; i++)
			{
				gIsdbtCapInfo[data_type].LanguageInfo[i].language_tag          	= pDataGroup->DMnge.LCode[i].language_tag;
				gIsdbtCapInfo[data_type].LanguageInfo[i].DMF                   		= pDataGroup->DMnge.LCode[i].DMF;
				gIsdbtCapInfo[data_type].LanguageInfo[i].DC                    		= pDataGroup->DMnge.LCode[i].DC;
				gIsdbtCapInfo[data_type].LanguageInfo[i].ISO_639_language_code	= pDataGroup->DMnge.LCode[i].ISO_639_language_code;
				gIsdbtCapInfo[data_type].LanguageInfo[i].Format                		= pDataGroup->DMnge.LCode[i].Format;
				gIsdbtCapInfo[data_type].LanguageInfo[i].TCS                   		= pDataGroup->DMnge.LCode[i].TCS;
				gIsdbtCapInfo[data_type].LanguageInfo[i].rollup_mode			= pDataGroup->DMnge.LCode[i].rollup_mode;

				/*DMF(display mode) setting ARIB STD-B24 Table9-5 Display mode*/
				// set automatic display
				if(gIsdbtCapInfo[data_type].LanguageInfo[i].DMF == 0x0
				|| gIsdbtCapInfo[data_type].LanguageInfo[i].DMF == 0x1
				|| gIsdbtCapInfo[data_type].LanguageInfo[i].DMF == 0x2
				|| gIsdbtCapInfo[data_type].LanguageInfo[i].DMF == 0x3){
					if(data_type == 0){
						ALOGD("Caption automatic display");
					}else{
						ALOGD("Superimpose automatic display");
					}
					subtitle_set_dmf_display(data_type, 1);
				}

				// set non-display
				if(gIsdbtCapInfo[data_type].LanguageInfo[i].DMF == 0x4
				|| gIsdbtCapInfo[data_type].LanguageInfo[i].DMF == 0x5
				|| gIsdbtCapInfo[data_type].LanguageInfo[i].DMF == 0x6
				|| gIsdbtCapInfo[data_type].LanguageInfo[i].DMF == 0x7){
					if(data_type == 0){
						ALOGD("Caption non-display");
					}else{
						ALOGD("Superimpose non-display");
					}
					subtitle_set_dmf_display(data_type, 0);
				}

				// set selective display
				if(gIsdbtCapInfo[data_type].LanguageInfo[i].DMF == 0x8
				|| gIsdbtCapInfo[data_type].LanguageInfo[i].DMF == 0x9
				|| gIsdbtCapInfo[data_type].LanguageInfo[i].DMF == 0xA
				|| gIsdbtCapInfo[data_type].LanguageInfo[i].DMF == 0xB){
					if(data_type == 0){
						ALOGD("Caption selective display");
					}else{
						ALOGD("Superimpose selective display");
					}
					subtitle_set_dmf_display(data_type, 0);
				}

				// set automaic/non-display under specific condition.
				if(gIsdbtCapInfo[data_type].LanguageInfo[i].DMF == 0xC
				|| gIsdbtCapInfo[data_type].LanguageInfo[i].DMF == 0xD
				|| gIsdbtCapInfo[data_type].LanguageInfo[i].DMF == 0xE){
					if(gIsdbtCapInfo[data_type].LanguageInfo[i].DC == 0x0){
						if(data_type == 0){
							ALOGD("Caption specific automaic display");
						}else{
							ALOGD("Superimpose specific automaic display");
						}
						subtitle_set_dmf_display(data_type, 1);
					}
				}

				CC_PRINTF("[%s] lang_tag:%d, dmf:0x%X, dc:%d, iso639:0x%X, format:0x%X, tcs:0x%X, rollup:0x%X", __func__,
						gIsdbtCapInfo[data_type].LanguageInfo[i].language_tag,
						gIsdbtCapInfo[data_type].LanguageInfo[i].DMF,
						gIsdbtCapInfo[data_type].LanguageInfo[i].DC,
						gIsdbtCapInfo[data_type].LanguageInfo[i].ISO_639_language_code,
						gIsdbtCapInfo[data_type].LanguageInfo[i].Format,
						gIsdbtCapInfo[data_type].LanguageInfo[i].TCS,
						gIsdbtCapInfo[data_type].LanguageInfo[i].rollup_mode);
			}

			/* Get the display caption language. */
			//gIsdbtCapInfo[data_type].ucSelectLangCode = UI_API_Get_DispCapLangIndex();
			/* Check the range of display caption language. */
			if (gIsdbtCapInfo[data_type].ucSelectLangCode > gIsdbtCapInfo[data_type].ucNumsOfLangCode)
			{
				/* Initialize the select language index. */
				gIsdbtCapInfo[data_type].ucSelectLangCode = ISDBT_CAP_1ST_STATE_DATA;
			}
		}
	}
}

void ISDBTCC_Handle_CaptionStatementData(int data_type, TS_PES_CAP_DATAGROUP *pDataGroup)
{
	unsigned int convert_stm =0;
	unsigned int ten_hour=0, hour =0;
	unsigned int ten_minute=0, minute =0;
	unsigned int ten_sec=0, sec =0;

	/* Handle the information of Caption Statement Data. */
	if ((pDataGroup != NULL) && (ISDBT_CAP_1ST_STATE_DATA <= (pDataGroup->data_group_id & ISDBT_CAP_LANGUAGE_MASK))
	&& ((pDataGroup->data_group_id & ISDBT_CAP_LANGUAGE_MASK) <= ISDBT_CAP_8TH_STATE_DATA)){
		/* TMD (Time Control Mode)*/
		gIsdbtCapInfo[data_type].State.TMD = pDataGroup->DState.TMD;
		if((gIsdbtCapInfo[data_type].State.TMD == TMD_REAL_TIME) || (gIsdbtCapInfo[data_type].State.TMD == TMD_OFFSET_TIME))
		{
			/* STM (Presentation start-time) */
			gIsdbtCapInfo[data_type].State.STM_time = pDataGroup->DState.STM_time;
			gIsdbtCapInfo[data_type].State.STM_etc = pDataGroup->DState.STM_etc;

			/* STM_time(24-bit) (nine 4-bit BCD) convert to milli-second */
			ten_hour = ((gIsdbtCapInfo[data_type].State.STM_time >> 20) & 0xf)*3600*10;
			hour = ((gIsdbtCapInfo[data_type].State.STM_time >> 16) & 0xf)*3600;
			ten_minute = ((gIsdbtCapInfo[data_type].State.STM_time >> 12) & 0xf)*60*10;
			minute = ((gIsdbtCapInfo[data_type].State.STM_time >> 8) & 0xf)*60;
			ten_sec = ((gIsdbtCapInfo[data_type].State.STM_time >> 4) & 0xf)*10;
			sec = ((gIsdbtCapInfo[data_type].State.STM_time) & 0xf);
			convert_stm = (ten_hour + hour + ten_minute + minute + ten_sec + sec) *1000; // unit: milli-second
		}

		/* set a new playback time */
		gIsdbtCapInfo[data_type].State.new_pts = (unsigned long long)convert_stm;

		CC_PRINTF("[%s] TMD:0x%X, STM_time:%d, STM_etc:%d, pts:%d, [ten_hour=%ds, hour=%ds, ten_minute=%ds, minute=%ds, ten_sec=%ds, sec=%ds]", __func__,
				gIsdbtCapInfo[data_type].State.TMD, gIsdbtCapInfo[data_type].State.STM_time, gIsdbtCapInfo[data_type].State.STM_etc,
				convert_stm, ten_hour, hour, ten_minute, minute, ten_sec, sec);
	}
}
