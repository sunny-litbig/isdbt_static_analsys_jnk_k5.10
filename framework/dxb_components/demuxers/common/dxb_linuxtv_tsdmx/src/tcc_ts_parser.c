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
#define LOG_TAG	"TCC_TS_PARSER"

#include <utils/Log.h>
#include "tcc_ts_parser.h"

int parseAdaptationField (unsigned char *p, MpegTsAdaptation *adap)
{
	adap->length = *p++;

	if (adap->length == 0)
	{
		/* Noah / 2017.10.23 / Change the return value 1 to 0.
			This case is an error, but it changes the return value 1 to 0 because of UTY Japan stream.
			ISO/IEC 13818-1 & ITU- Rec. H.222.0 say, if adaptation_field_control is '11',
			the value of the adapation_field_length shall be in the range 0 to 182.
			Please refer to IM478A-75 (The audio issue).
		*/
		return 0;
	}

	*(unsigned char *) &adap->flag = *p++;

	if (adap->flag.PCR) {
		adap->PCR = (long long) * p++ << 24;
		adap->PCR |= *p++ << 16;
		adap->PCR |= *p++ << 8;
		adap->PCR |= *p++;
		adap->PCR <<= 1;
		adap->PCR |= (*p & 0x80) >> 7;
		/* Just need 90KHz tick count */
		p += 2;
	}

	return 0;
}

int parsePayload(TS_PARSER *pTsParser, unsigned char *buf, unsigned int size, MpegPesHeader *pes)
{
	unsigned short tempShort;
	long long temp;
	unsigned char *p = buf;
	unsigned char PTS_DTS_FLAG_temp = 0;

	memset(pes, 0x0, sizeof(MpegPesHeader));

	pes->payload = buf;
	pes->payload_size = size;

	if (pTsParser->nPayLoadStart) {
		if (p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x01) {
			p += 3;

			pes->stream_id = *p++;
			pes->length = *p++ << 8;
			pes->length |= *p++;

			if (pes->stream_id == STREAM_ID_PROGRAM_STREAM_MAP ||
				pes->stream_id == STREAM_ID_PRIVATE_STREAM_2 ||
				pes->stream_id == STREAM_ID_ECM_STREAM ||
				pes->stream_id == STREAM_ID_EMM_STREAM ||
				pes->stream_id == STREAM_ID_PROGRAM_STREAM_DIRECTORY ||
				pes->stream_id == STREAM_ID_DSMCC ||
				pes->stream_id == STREAM_ID_ITU_T_REC_H222_1_TYPE_E)
			{
				pes->payload = p;
				pes->payload_size = size - 6;
			}
			else if (pes->stream_id == STREAM_ID_PADDING_STREAM)
			{
				pes->payload = NULL;
				pes->payload_size = 0;
			}
			else
			{
				tempShort = *p++ << 8;
				tempShort |= *p++;
				*(unsigned short *) &pes->flag = tempShort;
				pes->header_length = *p++;
				pes->payload = p + pes->header_length;
				pes->payload_size = size - (6 + 3 + pes->header_length);

#if 1
				PTS_DTS_FLAG_temp = (tempShort & 0x00C0) >> 6;				
				switch (PTS_DTS_FLAG_temp)
#else
 				switch (pes->flag.PTS_DTS_flags)
#endif
 				{
					case 0:	/* No PTS and DTS */
					case 1:
						pes->pts = 0;
						pes->dts = 0;
						break;

					case 2:	/* PTS only */
						temp = 0;
						temp = (long long) ((*p++ & 0x0e) >> 1) << 30;
						temp |= *p++ << 22;
						temp |= ((*p++ & 0xfe) >> 1) << 15;
						temp |= *p++ << 7;
						temp |= ((*p++ & 0xfe) >> 1);
						pes->pts = temp;
 						break;

					case 3:	/* PTS and DTS */
						temp = (long long) ((*p++ & 0x0e) >> 1) << 30;
						temp |= *p++ << 22;
						temp |= ((*p++ & 0xfe) >> 1) << 15;
						temp |= *p++ << 7;
						temp |= ((*p++ & 0xfe) >> 1);
						pes->pts = temp;
						
						temp = (long long) ((*p++ & 0x0e) >> 1) << 30;
						temp |= *p++ << 22;
						temp |= ((*p++ & 0xfe) >> 1) << 15;
						temp |= *p++ << 7;
						temp |= ((*p++ & 0xfe) >> 1);
						pes->dts = temp;
						break;
				}

				if (pes->length){
					pes->length -= (3 + pes->header_length);
				}

				if (pes->length < 0 || pes->payload_size < 0){
					return 1; //error
				}

				pTsParser->nPrevPTS = pTsParser->nPTS;
#if 1
				pTsParser->nPESScrambled = (tempShort & 0x0300) >> 12;
#else
				pTsParser->nPESScrambled = !!(pes->flag.PES_scrambling_control);
#endif
			}

			pTsParser->nContentSize = pes->length;
			pTsParser->nPTS= pes->pts;
		}
		else
		{
			pTsParser->nContentSize = 0;
			pTsParser->nPrevPTS = pTsParser->nPTS =0;
		}
			pTsParser->nPTS_flag = 1; /*for case of PTS is zero.*/
	}

	pTsParser->pBuf = pes->payload;
	pTsParser->nLen = pes->payload_size;

	return 0;
}

int parseTs(TS_PARSER *pTsParser, unsigned char *buf, int len, int bOnlyAdapt)
{
	int ret = 0;
	int pid;
	unsigned char *p = buf, *base = NULL;
	int check = 0;
	MpegPesHeader pesHeader;

	base = buf;

	pTsParser->nLen = 0;
	pTsParser->nAdaptation.length = 0;

	pTsParser->iPacketCount++;

	if (p[1] & TRANS_ERROR) {
		#if 0
		ALOGE("Trans Error[PID=%d]", pTsParser->nPID);
		#endif
		pTsParser->iCountTSError++;
		return 1;
	}

	pid = ((int)(p[1] & 0x1F)) << 8;
	pid |= ((int)p[2]);

	if (pTsParser->nPID != pid) {
		return 3;
	}

	pTsParser->nTSScrambled = !!((p[3] & 0xc0) >> 6);

	len -= 4;
	buf += 4;

	if (p[3] & ADAPT_FIELD) {
		if (parseAdaptationField (buf, &pTsParser->nAdaptation) == 0) {
			len = PACKETSIZE - (((int) buf - (int) base) + pTsParser->nAdaptation.length + 1);
			buf += (pTsParser->nAdaptation.length + 1);
		} else {
			#if 0
			ALOGE ("Failed to parse adaptation[PID=%d]\n", pTsParser->nPID);
			#endif
			return 2;
		}
	}

	if (p[3] & PAYLOAD) {
		if (pTsParser->nNextCC >=0 && pTsParser->nNextCC != (p[3] & 0x0f)) {
			#if 0
			ALOGE("Continuity Error[PID=%d][Wait CC %d != %d]", pTsParser->nPID, pTsParser->nNextCC, p[3] & 0x0f);
			#endif
			pTsParser->iCountDiscontinuityError++;
			pTsParser->nNextCC = -1;
			return 4;
		}
		pTsParser->nNextCC = (p[3] + 1) & 0x0f;
		pTsParser->nPayLoadStart = (p[1] & PAY_START) ? 1 : 0;
		if (bOnlyAdapt == 0 && p[3] & PAYLOAD && len > 0) {
			parsePayload(pTsParser, buf, len, &pesHeader);
		} else {
			pTsParser->nNextCC = -1;
		}
	}

	return 0;
}

