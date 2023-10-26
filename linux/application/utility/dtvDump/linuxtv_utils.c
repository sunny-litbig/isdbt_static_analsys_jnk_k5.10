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
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <dmx.h>
#include <pthread.h>

#include "linuxtv_utils.h"

#define DEVICE     "/dev/dvb0.demux1"

#define MBYTE             1024*1024
#define KERNEL_BUFF_SIZE  10*MBYTE
#define CACHE_SIZE        50*MBYTE

struct linuxtv {
	int hDemux;
	int iError;
	int iThreadState;
	int iThreadID;
	char *pBasePoint;
	int iEndPoint;
	int iReadPoint;
	int iWritePoint;
	pthread_mutex_t tLock;
	pthread_cond_t tEvent;
};

static void *linuxtv_thread(void *arg)
{
	struct linuxtv *pLinuxTV = (struct linuxtv*) arg;
	struct pollfd pfd;
	int size, readsize;

	pfd.fd = pLinuxTV->hDemux;
	pfd.events = POLLIN;

	if (ioctl(pLinuxTV->hDemux, DMX_START, 0) == 0)
	{
		while (pLinuxTV->iThreadState == 1)
		{
			if (poll(&pfd, 1, 1000)) 
			{
				if (pfd.revents & POLLIN) 
				{
					readsize = pLinuxTV->iReadPoint - pLinuxTV->iWritePoint;
					if (0 < readsize && readsize < MBYTE)
					{
						printf("Cache Overflow!\n");
						pLinuxTV->iError = -1;
						break;
					}
					readsize = pLinuxTV->iEndPoint - pLinuxTV->iWritePoint;
					if (readsize > MBYTE)
						readsize = MBYTE;

					size = read(pLinuxTV->hDemux, pLinuxTV->pBasePoint + pLinuxTV->iWritePoint, readsize);
					if (size < 0)
					{
						printf("Kernel Buffer Overflow\n");
						pLinuxTV->iError = -1;
						break;
					}

					if ((pLinuxTV->iWritePoint + size) == pLinuxTV->iEndPoint)
					{
						pLinuxTV->iWritePoint = 0;
					}
					else
					{
						pLinuxTV->iWritePoint += size;
					}
					pthread_mutex_lock(&pLinuxTV->tLock);
					pthread_cond_signal(&pLinuxTV->tEvent);
					pthread_mutex_unlock(&pLinuxTV->tLock);
				}
			}
		}

		pthread_mutex_lock(&pLinuxTV->tLock);
		pthread_cond_signal(&pLinuxTV->tEvent);
		pthread_mutex_unlock(&pLinuxTV->tLock);

		ioctl(pLinuxTV->hDemux, DMX_STOP, 0);
	}
}

LINUXTV* linuxtv_open(int devid, int *pid, int ipidnum)
{
	struct linuxtv *pLinuxTV = (struct linuxtv*) malloc(sizeof(struct linuxtv));
	int i = 0;
	dmx_source_t src = (devid == 0) ? DMX_SOURCE_FRONT0 : DMX_SOURCE_FRONT1;
	struct dmx_pes_filter_params pesFilterParams;

	pLinuxTV->iError = 0;
	pLinuxTV->iThreadState = 0;

	pLinuxTV->hDemux = open(DEVICE, O_RDWR | O_NONBLOCK);
	if (pLinuxTV->hDemux >= 0)
	{
		if ((ioctl(pLinuxTV->hDemux, DMX_SET_SOURCE, &src)) == 0)
		{
			if ((ioctl(pLinuxTV->hDemux, DMX_SET_BUFFER_SIZE, KERNEL_BUFF_SIZE)) < 0)
				printf("Failed to set buffer size!![%s]", DEVICE);

			memset(&pesFilterParams, 0, sizeof(struct dmx_pes_filter_params));

			pesFilterParams.pid = pid[0];
			pesFilterParams.pes_type = DMX_PES_OTHER;
			pesFilterParams.input = DMX_IN_FRONTEND;
			pesFilterParams.output = DMX_OUT_TSDEMUX_TAP; //DMX_OUT_DECODER, DMX_OUT_TAP
			pesFilterParams.flags = 0;//DMX_IMMEDIATE_START

			if (ioctl(pLinuxTV->hDemux, DMX_SET_PES_FILTER, &pesFilterParams) == 0)
			{
				for (i = 1; i < ipidnum; i++)
				{
					ioctl(pLinuxTV->hDemux, DMX_ADD_PID, &pid[i]);
				}
				return (LINUXTV*) pLinuxTV;
			}
		}
		linuxtv_close(pLinuxTV);
	}
	return NULL;
}

void linuxtv_close(LINUXTV *linuxtv)
{
	struct linuxtv *pLinuxTV = (struct linuxtv*) linuxtv;

	while (pLinuxTV->iThreadState != 0);

	close(pLinuxTV->hDemux);
	free(pLinuxTV);
}

int linuxtv_start(LINUXTV *linuxtv)
{
	struct linuxtv *pLinuxTV = (struct linuxtv*) linuxtv;

	if (pLinuxTV->iThreadState == 0)
	{
		pthread_mutex_init(&pLinuxTV->tLock, NULL);
		pthread_cond_init(&pLinuxTV->tEvent, NULL);

		pLinuxTV->iError = 0;
		pLinuxTV->iReadPoint = 0;
		pLinuxTV->iWritePoint = 0;
		pLinuxTV->iEndPoint = CACHE_SIZE;
		pLinuxTV->pBasePoint = malloc(CACHE_SIZE);

		pLinuxTV->iThreadState = 1;
		if (pthread_create(&pLinuxTV->iThreadID, NULL, linuxtv_thread, pLinuxTV) != 0)
		{
			pLinuxTV->iThreadState = 0;

			free(pLinuxTV->pBasePoint);
			pthread_mutex_destroy(&pLinuxTV->tLock);
			pthread_cond_destroy(&pLinuxTV->tEvent);

			return -1;
		}
	}
	return 0;
}

void linuxtv_stop(LINUXTV *linuxtv)
{
	struct linuxtv *pLinuxTV = (struct linuxtv*) linuxtv;

	if (pLinuxTV->iThreadState == 1)
	{
		pLinuxTV->iThreadState = 0;

		pthread_join(pLinuxTV->iThreadID, NULL);

		free(pLinuxTV->pBasePoint);
		pthread_mutex_destroy(&pLinuxTV->tLock);
		pthread_cond_destroy(&pLinuxTV->tEvent);
	}
}

int linuxtv_read(LINUXTV *linuxtv, unsigned char *buff, int readsize)
{
	struct linuxtv *pLinuxTV = (struct linuxtv*) linuxtv;
	int iRead  = pLinuxTV->iReadPoint;
	int iWrite = pLinuxTV->iWritePoint;
	int iTemp  = pLinuxTV->iEndPoint;

	if (pLinuxTV->iError < 0 || pLinuxTV->iThreadState == 0)
		return -1;

	if (iRead <= iWrite)
	{
		if ((iWrite - iRead) < readsize)
		{
			pthread_mutex_lock(&pLinuxTV->tLock);
			pthread_cond_wait(&pLinuxTV->tEvent, &pLinuxTV->tLock);
			pthread_mutex_unlock(&pLinuxTV->tLock);
			return 0;
		}
		memcpy(buff, (pLinuxTV->pBasePoint + iRead), readsize);
		pLinuxTV->iReadPoint += readsize;
	}
	else
	{
		iTemp -= iRead;
		if (iTemp >= readsize)
		{
			memcpy(buff, (pLinuxTV->pBasePoint + iRead), readsize);
			pLinuxTV->iReadPoint += readsize;
		}
		else
		{
			if ((iTemp + iWrite) < readsize)
			{
				pthread_mutex_lock(&pLinuxTV->tLock);
				pthread_cond_wait(&pLinuxTV->tEvent, &pLinuxTV->tLock);
				pthread_mutex_unlock(&pLinuxTV->tLock);
				return 0;
			}
			memcpy(buff, (pLinuxTV->pBasePoint + iRead), iTemp);
			memcpy(buff + iTemp, pLinuxTV->pBasePoint, (readsize - iTemp));
			pLinuxTV->iReadPoint = readsize - iTemp;
		}
	}

	return readsize;
}
