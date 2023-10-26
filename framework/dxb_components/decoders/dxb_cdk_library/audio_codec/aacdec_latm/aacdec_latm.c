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

void* latm_parser_init( unsigned char *pucPacketDataPtr, unsigned int piDataLength, int *piSamplingFrequency, int *piChannelNumber, void *psCallback, int eFormat );
int latm_parser_close( void *pLatm );
int latm_parser_get_frame( void *pLatmHandle, unsigned char *pucPacketDataPtr, int iStreamLength, unsigned char **pucAACRawDataPtr, int *piAACRawDataLength, unsigned int uiInitFlag );
int latm_parser_get_header_type( void *pLatmHandle );
int latm_parser_flush_buffer( void *pLatmHandle );


void* Latm_Init( unsigned char *pucPacketDataPtr, unsigned int piDataLength, int *piSamplingFrequency, int *piChannelNumber, void *psCallback, int eFormat )
{
	return latm_parser_init( pucPacketDataPtr, piDataLength, piSamplingFrequency, piChannelNumber, psCallback, eFormat );
}

int Latm_Close( void *pLatm )
{
	return latm_parser_close( pLatm );
}

int Latm_GetFrame( void *pLatmHandle, unsigned char *pucPacketDataPtr, int iStreamLength, unsigned char **pucAACRawDataPtr, int *piAACRawDataLength, unsigned int uiInitFlag )
{
	return latm_parser_get_frame( pLatmHandle, pucPacketDataPtr, iStreamLength, pucAACRawDataPtr, piAACRawDataLength, uiInitFlag );
}

int Latm_GetHeaderType( void *pLatmHandle )
{
	return latm_parser_get_header_type( pLatmHandle );
}

int Latm_FlushBuffer( void *pLatmHandle )
{
	return latm_parser_flush_buffer( pLatmHandle );
}
