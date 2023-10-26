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
#define LOG_TAG	"TsParse_ISDBT_PngDec"
#include <utils/Log.h>
#endif

#include <stdio.h>
#include <TsParser_Subtitle.h>
#include <TsParse_ISDBT_PngDec.h>


/****************************************************************************
DEFINITION
****************************************************************************/
#define PNGERR		ALOGE
#define PNGDBG		//ALOGD

/****************************************************************************
DEFINITION OF TYPE
****************************************************************************/


/****************************************************************************
DEFINITION OF EXTERNAL VARIABLES
****************************************************************************/

/****************************************************************************
DEFINITION OF GLOBAL VARIABLES
****************************************************************************/
static unsigned int g_png_readbytes = 0;
static unsigned int *gp_png_dec_buf = NULL;
static unsigned int g_png_buf_pitch = 0;
static unsigned int g_png_dec_x = 0;
static unsigned int g_png_dec_y = 0;
static unsigned int g_png_dec_w = 0;
static unsigned int g_png_dec_h = 0;


/****************************************************************************
DEFINITION OF LOCAL FUNCTIONS
****************************************************************************/
void TCC_CustomOutput_ARGB4444_ISDBT(IM_PIX_INFO out_info)
{
	unsigned int line = 0;
	unsigned int pos = 0;

	line = (out_info.Offset/g_png_dec_w) + g_png_dec_y;
	pos = (out_info.Offset%g_png_dec_w) + g_png_dec_x;

	*(gp_png_dec_buf + pos + (line * g_png_buf_pitch))=\
		(unsigned int)g_CMLA_Table[(out_info.Comp_1 > (MAX_COLOR-1))?0:out_info.Comp_1];
}

void TCC_CustomOutput_RGBA4444_ISDBT(IM_PIX_INFO out_info)
{
	unsigned int line = 0;
	unsigned int pos = 0;

	line = (out_info.Offset/g_png_dec_w) + g_png_dec_y;
	pos = (out_info.Offset%g_png_dec_w) + g_png_dec_x;

	*(gp_png_dec_buf + pos + (line * g_png_buf_pitch))=\
		ConvertARGB4444toRGBA4444((unsigned int)g_CMLA_Table[(out_info.Comp_1 > (MAX_COLOR-1))?0:out_info.Comp_1]);
}

void TCC_CustomOutput_ARGB4444_SBTVD(IM_PIX_INFO out_info)
{
	int R, G, B;
	unsigned int RGB;

	unsigned int line = 0;
	unsigned int pos = 0;
	unsigned int color = 0;

	line = (out_info.Offset/g_png_dec_w) + g_png_dec_y;
	pos = (out_info.Offset%g_png_dec_w) + g_png_dec_x;

	/* RGB565 format(MSB)*/
	R = out_info.Comp_1;
	G = out_info.Comp_2;
	B = out_info.Comp_3;

	RGB = (0xf<<12);
	RGB |= (R & 0xf0)<<4;
	RGB |= (G & 0xf0);
	RGB |= (B & 0xf0)>>4;

	*(gp_png_dec_buf + pos + (line * g_png_buf_pitch)) = RGB;
}

void TCC_CustomOutput_RGBA4444_SBTVD(IM_PIX_INFO out_info)
{
	int R, G, B;
	unsigned int RGB;

	unsigned int line = 0;
	unsigned int pos = 0;
	unsigned int color = 0;

	line = (out_info.Offset/g_png_dec_w) + g_png_dec_y;
	pos = (out_info.Offset%g_png_dec_w) + g_png_dec_x;

	/* RGB565 format(MSB)*/
	R = out_info.Comp_1;
	G = out_info.Comp_2;
	B = out_info.Comp_3;

	RGB = (R & 0xf0)<<8;
	RGB |= (G & 0xf0)<<4;
	RGB |= (B & 0xf0)<<4;
	RGB |= 0xff;

	*(gp_png_dec_buf + pos + (line * g_png_buf_pitch)) = RGB;
}

void TCC_CustomOutput_ARGB888_ISDBT(IM_PIX_INFO out_info)
{
	int R, G, B;
	unsigned int RGB = 0;

	unsigned int line = 0;
	unsigned int pos = 0;
	unsigned int color = 0;

	line = (out_info.Offset/g_png_dec_w) + g_png_dec_y;
	pos = (out_info.Offset%g_png_dec_w) + g_png_dec_x;

	//Color Conversion
	if(out_info.Src_Fmt == IM_SRC_YUV)
	{
		out_info.Comp_2 -= 128;
		out_info.Comp_3 -= 128;
		R = out_info.Comp_1 + ((1470103 * out_info.Comp_3) >> 20);
		if(R > 255 ) R = 255;	if(R < 0) R = 0;

		G = out_info.Comp_1 - ((360857 * out_info.Comp_2) >> 20) - ((748830 * out_info.Comp_3) >> 20);
		if(G > 255) G = 255;	if(G < 0) G = 0;

		B = out_info.Comp_1 + ((1858076 * out_info.Comp_2) >> 20);
		if(B > 255) B = 255;	if (B < 0) B = 0;
	}
	else if(out_info.Src_Fmt == IM_SRC_RGB)
	{
		RGB = ((out_info.Comp_4 & 0xF0) << 20) | (out_info.Comp_1 << 16) | (out_info.Comp_2 << 8) | out_info.Comp_3;
	}
	else//src_fmt == JD_SRC_OTHER
	{
	}

	*(gp_png_dec_buf + pos + (line * g_png_buf_pitch)) = (unsigned int)g_CMLA_Table[(out_info.Comp_1 > (MAX_COLOR-1))?0:out_info.Comp_1];
}

void TCC_CustomOutput_PNG_ARGB888_ISDBT(IM_PIX_INFO out_info)
{
	int R, G, B;
	unsigned int RGB = 0;

	unsigned int line = 0;
	unsigned int pos = 0;
	unsigned int color = 0;

	line = (out_info.Offset/g_png_dec_w) + g_png_dec_y;
	pos = (out_info.Offset%g_png_dec_w) + g_png_dec_x;

	//Color Conversion
	if(out_info.Src_Fmt == IM_SRC_YUV)
	{
		out_info.Comp_2 -= 128;
		out_info.Comp_3 -= 128;
		R = out_info.Comp_1 + ((1470103 * out_info.Comp_3) >> 20);
		if(R > 255 ) R = 255;	if(R < 0) R = 0;

		G = out_info.Comp_1 - ((360857 * out_info.Comp_2) >> 20) - ((748830 * out_info.Comp_3) >> 20);
		if(G > 255) G = 255;	if(G < 0) G = 0;
		
		B = out_info.Comp_1 + ((1858076 * out_info.Comp_2) >> 20);
		if(B > 255) B = 255;	if (B < 0) B = 0;
	}
	else if(out_info.Src_Fmt == IM_SRC_RGB)
	{
		RGB = ((out_info.Comp_4 & 0xF0) << 20) | (out_info.Comp_1 << 16) | (out_info.Comp_2 << 8) | out_info.Comp_3;
	}
	else//src_fmt == JD_SRC_OTHER
	{
	}

	*(gp_png_dec_buf + pos + (line * g_png_buf_pitch)) = (unsigned int)g_png_CMLA_Table[(out_info.Comp_1 > (MAX_COLOR-1))?0:out_info.Comp_1];
}


void TCC_CustomOutput_RGBA888_ISDBT(IM_PIX_INFO out_info)
{
	int R, G, B;
	unsigned int RGB = 0;

	unsigned int line = 0;
	unsigned int pos = 0;
	unsigned int color = 0;
		
	line = (out_info.Offset/g_png_dec_w) + g_png_dec_y;
	pos = (out_info.Offset%g_png_dec_w) + g_png_dec_x;

	//Color Conversion
	if(out_info.Src_Fmt == IM_SRC_YUV)
	{
		out_info.Comp_2 -= 128;
		out_info.Comp_3 -= 128;
		R = out_info.Comp_1 + ((1470103 * out_info.Comp_3) >> 20);
		if(R > 255 ) R = 255;	if(R < 0) R = 0;
		
		G = out_info.Comp_1 - ((360857 * out_info.Comp_2) >> 20) - ((748830 * out_info.Comp_3) >> 20);
		if(G > 255) G = 255;	if(G < 0) G = 0;
		
		B = out_info.Comp_1 + ((1858076 * out_info.Comp_2) >> 20);
		if(B > 255) B = 255;	if (B < 0) B = 0;
	}
	else if(out_info.Src_Fmt == IM_SRC_RGB)
	{
		RGB = ((out_info.Comp_4 & 0xF0) << 20) | (out_info.Comp_1 << 16) | (out_info.Comp_2 << 8) | out_info.Comp_3;
	}
	else//src_fmt == JD_SRC_OTHER
	{
	}

	*(gp_png_dec_buf + pos + (line * g_png_buf_pitch)) = ConvertARGB8888toRGBA4444(RGB);
}

size_t tcc_isdbt_png_data_read( void * ptr, size_t size, size_t count, void * stream )
{
	int read_bytes = 0, remain_bytes = 0;
	FILE_CTX *p = (FILE_CTX*)stream;

	read_bytes = size*count;

	if((read_bytes > 0) && (p->total_size > 0)){
		remain_bytes = p->total_size - read_bytes;

		if(remain_bytes < 0){
			read_bytes = p->total_size;
		}

		memcpy((char*)ptr, (char*)&p->pBuf[p->cur_pos], read_bytes);
		p->cur_pos += read_bytes;
		p->total_size -= read_bytes;
	}else{
		PNGERR("[%s] read_bytes:%d, total_size:%d\n", __func__, read_bytes, p->total_size);
		read_bytes = 0;
	}

	return read_bytes;
}

int tcc_isdbt_png_init(void *pSrc, int SrcBufSize, unsigned int *pWidth, unsigned int *pHeight)
{
	int ret_value = -1;
	FILE_CTX		file_ctx;
	char *pInstanceMem = NULL;
	PD_INIT *pPD_init_Mem = NULL;
	PD_CUSTOM_DECODE output_setting;
	PD_CALLBACKS callbacks = {(int (*)(void *, int, int, void *))tcc_isdbt_png_data_read};

	if((pSrc == NULL) || (SrcBufSize==0) || (pWidth == NULL) || (pHeight == NULL)){
		PNGERR("[%s] Input parameter error(0x%p, %d)\n", __func__, pSrc, SrcBufSize);
		return -1;
	}

	*pWidth = 0;
	*pHeight = 0;

	memset(&output_setting, 0x0, sizeof(PD_CUSTOM_DECODE));
	memset(&file_ctx, 0x0, sizeof(FILE_CTX));

	file_ctx.pBuf = pSrc;
	file_ctx.total_size = SrcBufSize;
	file_ctx.cur_pos = 0;

	pInstanceMem = (void *)tcc_mw_malloc(__FUNCTION__, __LINE__, PD_INSTANCE_MEM_SIZE);
	if( pInstanceMem == NULL ) {
		PNGERR("[%s] pInstanceMem alloc ERROR\n", __func__);
		goto END;
	}
	memset( pInstanceMem, 0, PD_INSTANCE_MEM_SIZE );

	pPD_init_Mem = (PD_INIT *)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(PD_INIT) );
	if( pPD_init_Mem == NULL ) {
		PNGERR("[%s] pPD_init_Mem alloc ERROR\n", __func__);
		goto END;
	}
	memset( pPD_init_Mem, 0, sizeof(PD_INIT) );

	pPD_init_Mem->pInstanceBuf 	= pInstanceMem;
	pPD_init_Mem->datasource 		= (void *)&file_ctx;
	pPD_init_Mem->lcd_width 		= 960;				//dummy
	pPD_init_Mem->lcd_height 		= 540;				//dummy
	pPD_init_Mem->iTotFileSize 		= SrcBufSize;
	pPD_init_Mem->iOption 		= (1<<4);			//set isdbt png decode option

	ret_value = TCCXXX_PNG_Decode( PD_DEC_INIT, pPD_init_Mem, &callbacks, 0 );
	if(ret_value == PD_RETURN_INIT_FAIL) {
		PNGERR("[%s] Initialization Failure\n", __func__);
		goto END;
	}

	*pWidth = pPD_init_Mem->image_width;
	*pHeight = pPD_init_Mem->image_height;
	ret_value= 0;

END:
	if(pInstanceMem)
		tcc_mw_free(__FUNCTION__, __LINE__, pInstanceMem);

	if(pPD_init_Mem)
		tcc_mw_free(__FUNCTION__, __LINE__, pPD_init_Mem);

	return ret_value;
}


#if defined(DUMP_PNG_INPUT_TO_FILE)
static FILE *gp_file_handle = NULL;
void png_dump_file(char *data, int size)
{
	static unsigned int index = 0;
	char szName[40] = {0,};

	if(gp_file_handle == NULL){
		sprintf(szName, "/mnt/sdcard/dump_%03d.raw", index++);
		gp_file_handle = fopen(szName, "wb");
		if(gp_file_handle == NULL){
			LOGE("===>> subtitle dump file open error %d\n", gp_file_handle);
		}else{
			fwrite(data, 1, size, gp_file_handle);
			LOGE("===>> write size %d - handle %d\n", size, gp_file_handle);
			fclose(gp_file_handle);
			sync();
			gp_file_handle = NULL;
		}
	}
}
#endif /* DUMP_PNG_INPUT_TO_FILE */


int tcc_isdbt_png_dec(void *pSrc, int SrcBufSize, void *pDst, int dst_w_pitch, int png_x, int png_y, int png_w, int png_h, int format, int cc, int logo)
{
	int ret_value = -1;
	FILE_CTX		file_ctx;
	char *pInstanceMem = NULL;
	PD_INIT *pPD_init_Mem = NULL;
	PD_CUSTOM_DECODE output_setting;
	PD_CALLBACKS callbacks = {(int (*)(void *, int, int, void *))tcc_isdbt_png_data_read};

	char *pHeapBuf = NULL;

	if((pSrc == NULL) || (SrcBufSize==0) || (pDst == NULL) || (png_w*png_h == 0) || (png_w > dst_w_pitch)){
		PNGERR("[%s] Input parameter error(src:0x%p, %d, dst:0x%p, %d, %d, %d)\n", __func__, \
			pSrc, SrcBufSize, pDst, dst_w_pitch, png_w, png_h);
		return -1;
	}

	memset(&output_setting, 0x0, sizeof(PD_CUSTOM_DECODE));
	memset(&file_ctx, 0x0, sizeof(FILE_CTX));

	file_ctx.pBuf = pSrc;
	file_ctx.total_size = SrcBufSize;
	file_ctx.cur_pos = 0;

	pInstanceMem = (void *)tcc_mw_malloc(__FUNCTION__, __LINE__, PD_INSTANCE_MEM_SIZE);
	if( pInstanceMem == NULL ) {
		PNGERR("[%s] pInstanceMem alloc ERROR\n", __func__);
		goto END;
	}
	memset( pInstanceMem, 0, PD_INSTANCE_MEM_SIZE );

	pPD_init_Mem = (PD_INIT *)tcc_mw_malloc(__FUNCTION__, __LINE__, sizeof(PD_INIT) );
	if( pPD_init_Mem == NULL ) {
		PNGERR("[%s] pPD_init_Mem alloc ERROR\n", __func__);
		goto END;
	}
	memset( pPD_init_Mem, 0, sizeof(PD_INIT) );

	pPD_init_Mem->pInstanceBuf 	= pInstanceMem;
	pPD_init_Mem->datasource 		= (void *)&file_ctx;
	pPD_init_Mem->lcd_width 		= png_w;
	pPD_init_Mem->lcd_height 		= png_h;
	pPD_init_Mem->iTotFileSize 		= SrcBufSize;

	if(cc==0){
		//japan
	pPD_init_Mem->iOption 		= (1<<4);
	}

	ret_value = TCCXXX_PNG_Decode( PD_DEC_INIT, pPD_init_Mem, &callbacks, 0 );
	if(ret_value == PD_RETURN_INIT_FAIL) {
		PNGERR("[%s] Initialization Failure\n", __func__);
		goto END;
	}

	if(((unsigned int)png_w != pPD_init_Mem->image_width) || ((unsigned int)png_h != pPD_init_Mem->image_height)){
		PNGERR("[%s] Invalid image size(%d:%d != %d:%d)\n", __func__, \
			png_w, png_h, pPD_init_Mem->image_width, pPD_init_Mem->image_height);
		goto END;
	}

	g_png_dec_x = png_x;
	g_png_dec_y = png_y;
	g_png_dec_w = png_w;
	g_png_dec_h = png_h;
	gp_png_dec_buf = (unsigned int*)pDst;
	g_png_buf_pitch = dst_w_pitch;

	PNGDBG("[%s] PNG(%d:%d), alpha:%d, bpp:%d, heapSize:%d\n", __func__,
		pPD_init_Mem->image_width, pPD_init_Mem->image_height,
		pPD_init_Mem->alpha_available, 	pPD_init_Mem->pixel_depth, pPD_init_Mem->heap_size);

	/* Error Detection Mode Setting */
	output_setting.ERROR_DET_MODE = PD_ERROR_CHK_NONE;

	/* Resource Occupation Degree Setting */
	output_setting.RESOURCE_OCCUPATION = PD_RESOURCE_LEVEL_NONE;

	/* Heap_Memory Setting */
	pHeapBuf = tcc_mw_malloc(__FUNCTION__, __LINE__, pPD_init_Mem->heap_size);
	if( pHeapBuf == NULL ) {
		PNGERR("==> pHeapBuf allocation error\n");
		goto END;
	}
	memset(pHeapBuf, 0, pPD_init_Mem->heap_size);

	output_setting.Heap_Memory = (unsigned char*)pHeapBuf;

	/* Modify location of output image */
	output_setting.MODIFY_IMAGE_POS = 0;
	output_setting.IMAGE_POS_X = 0;
	output_setting.IMAGE_POS_Y = 0;

	/* Output format setting */
	output_setting.USE_ALPHA_DATA = PD_ALPHA_DISABLE;

	PNGDBG("===> [%s] cc:%d, format:%d\n", __func__, cc, format);

	if(cc){//brazil
		if( format == ISDBT_PNGDEC_ARGB4444) {
			output_setting.write_func = (void (*)(IM_PIX_INFO))TCC_CustomOutput_ARGB4444_SBTVD;
		}
		else if( format == ISDBT_PNGDEC_ARGB8888) {
			output_setting.write_func = (void (*)(IM_PIX_INFO))TCC_CustomOutput_ARGB888_ISDBT;
		}
		else {
			output_setting.write_func = (void (*)(IM_PIX_INFO))TCC_CustomOutput_RGBA4444_SBTVD;
		}
	}
	else{//japan
		if( format == ISDBT_PNGDEC_ARGB4444) {
			output_setting.write_func = (void (*)(IM_PIX_INFO))TCC_CustomOutput_ARGB4444_ISDBT;
		}
		else if( format == ISDBT_PNGDEC_ARGB8888) {
			if(logo) {
				output_setting.write_func = (void (*)(IM_PIX_INFO))TCC_CustomOutput_ARGB888_ISDBT; //for Logo PNG
			}else{
				output_setting.write_func = (void (*)(IM_PIX_INFO))TCC_CustomOutput_PNG_ARGB888_ISDBT; //for Subtitle PNG
			}
		}
		else {
			output_setting.write_func = (void (*)(IM_PIX_INFO))TCC_CustomOutput_RGBA4444_ISDBT;
		}
	}

	ret_value = 1;
	while(ret_value > 0){
		ret_value = TCCXXX_PNG_Decode( PD_DEC_DECODE, &output_setting, 0, 0 );
	}

	if(ret_value != PD_RETURN_DECODE_DONE) {
		PNGERR(" Decoding Failure [Error_code: %d]!!!\n", ret_value);
	}
#if defined(DUMP_PNG_INPUT_TO_FILE)
	else{
		PNGERR("===> [%s] w:%d, h:%d\n", __func__, g_png_dec_w, g_png_dec_h);
		png_dump_file(gp_png_dec_buf, g_png_dec_w*g_png_dec_h*sizeof(short));
	}
#endif /* 0 */

END:
	if(pHeapBuf)
		tcc_mw_free(__FUNCTION__, __LINE__, pHeapBuf);

	if(pInstanceMem)
		tcc_mw_free(__FUNCTION__, __LINE__, pInstanceMem);

	if(pPD_init_Mem)
		tcc_mw_free(__FUNCTION__, __LINE__, pPD_init_Mem);

	return ret_value;
}

