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
#define LOG_TAG	"ISDBT_INTERFACE_SC"
#include <utils/Log.h>
#include <pthread.h>
#include <fcntl.h>

#if 1 //jktest
// IOCTL Command Definition
enum
{
		PL131_SC_IOCTL_SET_VCC_LEVEL = 10,
		PL131_SC_IOCTL_ENABLE,
		PL131_SC_IOCTL_ACTIVATE,
		PL131_SC_IOCTL_RESET,
		PL131_SC_IOCTL_DETECT_CARD,
		PL131_SC_IOCTL_SET_CONFIG,
		PL131_SC_IOCTL_GET_CONFIG,
		PL131_SC_IOCTL_SET_IO_PIN_CONFIG,
		PL131_SC_IOCTL_GET_IO_PIN_CONFIG,
		PL131_SC_IOCTL_SEND,
		PL131_SC_IOCTL_RECV,
		PL131_SC_IOCTL_SEND_RCV,
		PL131_SC_IOCTL_NDS_SEND_RCV,
		PL131_SC_IOCTL_MAX
};
// Send/Receive Buffer
typedef struct
{
		unsigned char			*pucTxBuf;
		unsigned int			uiTxBufLen;
		unsigned char			*pucRxBuf;
		unsigned int			*puiRxBufLen;
		unsigned int			uiDirection;
		unsigned int			uiTimeOut;
} stPL131_SC_BUF;
// Configuration Parameter
typedef struct
{
		unsigned int			uiProtocol;
		unsigned int			uiConvention;
		unsigned int			uiParity;
		unsigned int			uiErrorSignal;
		unsigned int			uiFlowControl;

		unsigned int			uiClock;
		unsigned int			uiBaudrate;

		unsigned int			uiWaitTime;
		unsigned int			uiGuardTime;
} stPL131_SC_CONFIG;

#define TCC_SC_IOCTL_ENABLE			PL131_SC_IOCTL_ENABLE
#define TCC_SC_IOCTL_DETECT_CARD	PL131_SC_IOCTL_DETECT_CARD
#define TCC_SC_IOCTL_SEND_RCV		PL131_SC_IOCTL_SEND_RCV
#define TCC_SC_IOCTL_GET_CONFIG		PL131_SC_IOCTL_GET_CONFIG
#define TCC_SC_IOCTL_RESET			PL131_SC_IOCTL_RESET
#define TCC_SC_IOCTL_SET_VCC_LEVEL	PL131_SC_IOCTL_SET_VCC_LEVEL
#define TCC_SC_IOCTL_ACTIVATE		PL131_SC_IOCTL_ACTIVATE
#define stTCC_SC_BUF				stPL131_SC_BUF
#define stTCC_SC_CONFIG				stPL131_SC_CONFIG
#else
#include <tcc_sc_ioctl.h>
#endif

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_Types.h>
#include <OMX_Audio.h>
#include "OMX_Other.h"
#include "tcc_dxb_interface_type.h"
#include "tcc_dxb_interface_sc.h"

#define SCDBG(msg...)		//ALOGD(msg)

#define SC_DEVICE "/dev/tcc-smartcard"

#define MAX_SC_NUM		1
#define MAX_ERR_NUM		10

static int iSC_Fd 				= -1;
static int iSC_Detected 		= 0;
static int iSC_Activated 		= 0;
static int iSC_Thread_Runing	= 0;
static pthread_t SC_Thread_Id;
static pfnDxB_SC_EVT_CALLBACK pfnSC_Evnent_Callback = NULL;

void TCC_DxB_SC_Thread(void *arg)
{
	UINT32 uiCardDetect = DxB_SC_NOT_PRESENT;
	UINT32 uiErrCount = 0;

	unsigned char ucATR[33];
	unsigned int iATRLength;

	/* Open SmartCard Device Driver */
	if(iSC_Fd < 0)
	{
		iSC_Fd = open(SC_DEVICE, O_RDWR);
		if(iSC_Fd < 0)
		{
			ALOGE("Error: Unable to open sc device\n");
			iSC_Thread_Runing = 0;
		}
	}

	/* Run Thread for Card Detection */
	while(iSC_Thread_Runing)
	{
		if(ioctl(iSC_Fd, TCC_SC_IOCTL_DETECT_CARD, &uiCardDetect) != 0)
		{
			uiCardDetect = DxB_SC_NOT_PRESENT;
			uiErrCount++;
			if(uiErrCount >= MAX_ERR_NUM)
			{
				ALOGE("Error: Escape SC thread\n");
				iSC_Thread_Runing = 0;
			}
			ALOGE("Error: TCC_SC_IOCTL_DETECT_CARD, ErrCount=%d\n", uiErrCount);
		}
		else
		{
			if(uiCardDetect == DxB_SC_PRESENT)
			{
				if(iSC_Detected == DxB_SC_NOT_PRESENT)
				{
					ALOGD("SmartCard is inserted\n");

					iSC_Detected = DxB_SC_PRESENT;
					if(pfnSC_Evnent_Callback != NULL)
						pfnSC_Evnent_Callback(0, DxB_SC_PRESENT);

					#if 0 // for test
					TCC_DxB_SC_Activate(0);
					TCC_DxB_SC_Reset(0, ucATR, &iATRLength);
					#endif
				}
			}
			else
			{
				if(iSC_Detected == DxB_SC_PRESENT)
				{
					ALOGD("SmartCard is removed\n");

					iSC_Detected = DxB_SC_NOT_PRESENT;
					if(pfnSC_Evnent_Callback != NULL)
						pfnSC_Evnent_Callback(0, DxB_SC_NOT_PRESENT);

					TCC_DxB_SC_Deactivate(0);
				}
			}
		}

		usleep(100000);
	}

	/* Close SmartCard Device Driver */
	if(iSC_Fd > 0)
	{
		close(iSC_Fd);
		iSC_Fd = -1;
	}
}

DxB_ERR_CODE TCC_DxB_SC_GetCapability(UINT32*pNumDevice)
{
	SCDBG("In %s\n", __func__);
	
	*pNumDevice = (UINT32)MAX_SC_NUM;

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_SC_NegotiatePTS(UINT32 unDeviceId, unsigned char *pucWriteBuf, int iNumToWrite, unsigned char *pucReadBuf, unsigned int *piNumRead)
{
	stTCC_SC_BUF stScBuffer;

	SCDBG("In %s\n", __func__);
	
	stScBuffer.pucTxBuf 		= pucWriteBuf;
	stScBuffer.uiTxBufLen	 	= iNumToWrite;
	stScBuffer.pucRxBuf 		= pucReadBuf;
	stScBuffer.puiRxBufLen	 	= piNumRead;

	if(iSC_Fd < 0)
		return DxB_ERR_ERROR;

	if(ioctl(iSC_Fd, TCC_SC_IOCTL_SEND_RCV, &stScBuffer) != 0)
	{
		ALOGE("Error: TCC_SC_IOCTL_SEND_RCV\n");
		return DxB_ERR_ERROR;
	}
  
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_SC_SetParams(UINT32 unDeviceId, DxB_SC_PROTOCOL nProtocol, unsigned long ulMinClock, unsigned long ulSrcBaudrate, unsigned char ucFI, unsigned char ucDI, unsigned char ucN, unsigned char ucCWI, unsigned char ucBWI)
{
	SCDBG("In %s\n", __func__);
	
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_SC_GetParams(UINT32 unDeviceId, DxB_SC_PROTOCOL *pnProtocol, unsigned long *pulClock, unsigned long *pulBaudrate, unsigned char *pucFI, unsigned char *pucDI, unsigned char *pucN, unsigned char *pucCWI, unsigned char *pucBWI)
{
	stTCC_SC_CONFIG stSCConfig;

	SCDBG("In %s\n", __func__);
	
	if(iSC_Fd < 0)
		return DxB_ERR_ERROR;

	if(ioctl(iSC_Fd, TCC_SC_IOCTL_GET_CONFIG, &stSCConfig) != 0)
	{
		ALOGE("Error: TCC_SC_IOCTL_GET_PARAMS\n");
		return DxB_ERR_ERROR;
	}
  
	*pnProtocol = stSCConfig.uiProtocol;

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_SC_GetCardStatus(UINT32 unDeviceId, DxB_SC_STATUS *pnStatus)
{
	if(iSC_Fd < 0)
		return DxB_ERR_ERROR;

	*pnStatus = iSC_Detected;
  
	SCDBG("In %s\n, DetectStatus:%d", __func__, *pnStatus);
	
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_SC_Reset (UINT32 unDeviceId, UINT8 *pucAtr, UINT32 *pucAtrlen)
{
	stTCC_SC_BUF stScBuffer;

	SCDBG("In %s\n", __func__);
	
	stScBuffer.pucTxBuf = NULL;
	stScBuffer.uiTxBufLen = 0x00;
	stScBuffer.pucRxBuf = pucAtr;
	stScBuffer.puiRxBufLen = pucAtrlen;

	if(iSC_Fd < 0)
		return DxB_ERR_ERROR;

	if(ioctl(iSC_Fd, TCC_SC_IOCTL_RESET, &stScBuffer))
	{
		SCDBG("Error: Reset sc device\n");
		return DxB_ERR_ERROR;
	}
  
	if(*stScBuffer.puiRxBufLen == 0)
	{
		SCDBG("Error: Can't receive ATR\n");
		return DxB_ERR_ERROR;
	}

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_SC_TransferData(UINT32 unDeviceId, unsigned char *pucWriteBuf, int iNumToWrite, unsigned char *pucReadBuf, unsigned int *piNumRead)
{
	stTCC_SC_BUF stScBuffer;

	SCDBG("In %s\n", __func__);
	
	if(iSC_Fd < 0)
		return DxB_ERR_ERROR;

	stScBuffer.pucTxBuf 		= pucWriteBuf;
	stScBuffer.uiTxBufLen	 	= iNumToWrite;
	stScBuffer.pucRxBuf 		= pucReadBuf;
	stScBuffer.puiRxBufLen	 	= piNumRead;

	if(ioctl(iSC_Fd, TCC_SC_IOCTL_SEND_RCV, &stScBuffer) != 0)
	{
		SCDBG("Error: TCC_SC_IOCTL_SEND_RCV\n");
		return DxB_ERR_ERROR;
	}
  
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_SC_ReadData(UINT32 unDeviceId, unsigned char *pucWriteBuf, int iNumToWrite, unsigned char *pucReadBuf, unsigned int *piNumRead)
{
	stTCC_SC_BUF stScBuffer;
	
	if(iSC_Fd < 0)
		return DxB_ERR_ERROR;

	stScBuffer.pucTxBuf 		= pucWriteBuf;
	stScBuffer.uiTxBufLen	 	= iNumToWrite;
	stScBuffer.pucRxBuf 		= pucReadBuf;
	stScBuffer.puiRxBufLen	 	= piNumRead;

	if(ioctl(iSC_Fd, TCC_SC_IOCTL_SEND_RCV, &stScBuffer) != 0)
	{
		ALOGE("Error: TCC_SC_IOCTL_SEND_RCV\n");
		return DxB_ERR_ERROR;
	}
  
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_SC_WriteData(UINT32 unDeviceId, unsigned char *pucWriteBuf, int iNumToWrite, unsigned char *pucReadBuf,unsigned int *piNumRead)
{
	stTCC_SC_BUF stScBuffer;

	SCDBG("In %s\n", __func__);
	
	if(iSC_Fd < 0)
		return DxB_ERR_ERROR;

	stScBuffer.pucTxBuf 		= pucWriteBuf;
	stScBuffer.uiTxBufLen	 	= iNumToWrite;
	stScBuffer.pucRxBuf 		= pucReadBuf;
	stScBuffer.puiRxBufLen	 	= piNumRead;

	if(ioctl(iSC_Fd, TCC_SC_IOCTL_SEND_RCV, &stScBuffer) != 0)
	{
		SCDBG("Error: TCC_SC_IOCTL_SEND_RCV\n");
		return DxB_ERR_ERROR;
	}

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_SC_RegisterCallback(UINT32 unDeviceId, pfnDxB_SC_EVT_CALLBACK pfnSCCallback)
{
	int iStatus;

	SCDBG("In %s\n", __func__);

	iSC_Thread_Runing = 1;

	if ((iStatus = pthread_create(&SC_Thread_Id, NULL, (void *)&TCC_DxB_SC_Thread, NULL)))
	{
		SCDBG("Error: fail to create SC thread, status=%d\n", iStatus);
		return DxB_ERR_ERROR;
	}
	
	if(pfnSCCallback != NULL)
	{
		pfnSC_Evnent_Callback = pfnSCCallback;
	}

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_SC_SetVccLevel(UINT32 unDeviceId, DxB_SC_VccLevel in_vccLevel)
{
	SCDBG("In %s\n", __func__);
	
	if(iSC_Fd < 0)
		return DxB_ERR_ERROR;

	if(ioctl(iSC_Fd, TCC_SC_IOCTL_SET_VCC_LEVEL &in_vccLevel) != 0)
	{
		SCDBG("Error: TCC_SC_IOCTL_SELECT_VOL\n");
		return DxB_ERR_ERROR;
	}

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_SC_DownUpVCC(UINT32 unDeviceId, unsigned int unDownTime /*ms*/)
{
	SCDBG("In %s\n", __func__);
	
	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_SC_Activate(UINT32 unDeviceId)
{
	UINT32 uiActivate = 1;

	SCDBG("In %s\n", __func__);
	
	#if 0
	/* Open SmartCard Device Driver */
	if(iSC_Fd < 0)
	{
		iSC_Fd = open(SC_DEVICE, O_RDWR);
		if(iSC_Fd < 0)
		{
			ALOGE("Unable to open sc device\n");
			return DxB_ERR_ERROR;
		}
	}
	#endif

	if(iSC_Fd < 0)
		return DxB_ERR_ERROR;
	/* Set Activated Flag */
	iSC_Activated = 1;

	/* Activate SC Interface */
	if(ioctl(iSC_Fd, TCC_SC_IOCTL_ACTIVATE, &uiActivate) != 0)
	{
		ALOGE("Error: TCC_SC_IOCTL_ACTIVATE, uiActivate=%d\n", uiActivate);
		return DxB_ERR_ERROR;
	}

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_SC_Deactivate(UINT32 unDeviceId)
{
	UINT32 uiActivate = 0;

	SCDBG("In %s\n", __func__);
	
	#if 0
	/* Close SmartCard Device Driver */
	if(iSC_Fd > 0)
	{
		close(iSC_Fd);
		iSC_Fd = -1;
	}
	#endif

	if(iSC_Fd < 0)
		return DxB_ERR_ERROR;

	/* Deactivate SC Interface */
	if(ioctl(iSC_Fd, TCC_SC_IOCTL_ACTIVATE, &uiActivate) != 0)
	{
		SCDBG("Error: TCC_SC_IOCTL_ACTIVATE, uiActivate=%d\n", uiActivate);
		return DxB_ERR_ERROR;
	}

	/* Clear Activated Flag */
	iSC_Activated = 0;

	/* Clear Deteted Flag */
	iSC_Detected = DxB_SC_NOT_PRESENT;

	return DxB_ERR_OK;
}

DxB_ERR_CODE TCC_DxB_SC_CardDetect(void)
{
	unsigned int uiCardDetect = DxB_SC_NOT_PRESENT;
	unsigned char ucATR[33];
	unsigned int iATRLength;
	stTCC_SC_BUF stScBuffer;
	unsigned int ret = 0;
	
	SCDBG("In %s\n", __func__);
	
	if(ioctl(iSC_Fd, TCC_SC_IOCTL_DETECT_CARD, &uiCardDetect) == 0)
	{
		//ALOGE("%s %d uiCardDetect = %d \n", __func__, __LINE__, uiCardDetect);
		if (uiCardDetect == DxB_SC_NOT_PRESENT)
			ret = DxB_ERR_ERROR;
		else
			ret = DxB_ERR_OK;
	}
	else
	{
		//ALOGE("%s %d \n", __func__, __LINE__);
		ret =  DxB_ERR_ERROR;
	}
		
	return ret;
}

DxB_ERR_CODE TCC_DxB_SC_Open(void)
{
	SCDBG("In %s\n", __func__);

	if(iSC_Fd < 0)
	{
		iSC_Fd = open(SC_DEVICE, O_RDWR);
		if(iSC_Fd < 0)
		{
			SCDBG("Error: Unable to open sc device\n");
			return -1;
		}
		else
		{
			unsigned int enable = 1;
			if(ioctl(iSC_Fd, TCC_SC_IOCTL_ENABLE, &enable) != 0)
			{
				ALOGE("%s %d enable error\n", __func__, __LINE__);
			}
		}
	}
	else
	{
		SCDBG("Error: sc device opened \n");
		return -1;
	}	
	return DxB_ERR_OK;
}
DxB_ERR_CODE TCC_DxB_SC_Close(void)
{
	unsigned int enable = 0;
	SCDBG("In %s\n", __func__);

	if(ioctl(iSC_Fd, TCC_SC_IOCTL_ENABLE, &enable) != 0)
	{
		ALOGE("%s %d enable error\n", __func__, __LINE__);
	}
	
	if(iSC_Fd > 0)
	{
		close(iSC_Fd);
		iSC_Fd = -1;
	}

	return DxB_ERR_OK;
}
