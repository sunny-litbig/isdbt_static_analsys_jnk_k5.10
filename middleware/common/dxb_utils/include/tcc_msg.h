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
#ifndef	_TCC_MSG_H__
#define	_TCC_MSG_H__
#include <stdlib.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif


class TccMessage
{
	class TccMessageData
	{
	public:
		void *m_MsgData;
		TccMessageData *m_Next;
	};

public:
	TccMessage(unsigned int uiMaxMsgCount): m_uiMaxMsgCount(uiMaxMsgCount), m_uiMsgCount(0)
	{
		m_MsgHeader = NULL;
		m_MsgTail = NULL;
		pthread_mutex_init(&m_mutex,NULL);	
	}
	~TccMessage()
	{
		TccMessageData *p;
		p = m_MsgHeader;
		for(unsigned int i=0;i<m_uiMsgCount;i++)
		{
			if(p == NULL)
				break;

			m_MsgHeader = m_MsgHeader->m_Next;
			delete p;			
			p = m_MsgHeader;						
		}
		pthread_mutex_destroy(&m_mutex);
	}
	unsigned int TccMessageGetCount(void)
	{
		return m_uiMsgCount;
	}

	unsigned int TccMessageGetMaxCount(void)
	{
		return m_uiMaxMsgCount;
	}

	int TccMessagePutFirst(void *pMsg)
	{
		TccMessageLock();
		if(m_uiMsgCount >= m_uiMaxMsgCount)
		{
			TccMessageUnLock();
			return 1;
		}
		TccMessageData *pback;		
		TccMessageData *p = new TccMessageData;		
		p->m_Next = NULL;
		p->m_MsgData = pMsg;
		if(m_uiMsgCount == 0)
		{
			m_MsgHeader = p;
			m_MsgTail = p;
		}
		else
		{			
			pback = m_MsgHeader;			
			m_MsgHeader = p;	
			m_MsgHeader->m_Next = pback;
		}
		m_uiMsgCount++;
		TccMessageUnLock();
		return 0;
	}

	int TccMessagePut(void *pMsg)
	{
		TccMessageLock();
		if(m_uiMsgCount >= m_uiMaxMsgCount)
		{
			TccMessageUnLock();
			return 1;
		}
		TccMessageData *p = new TccMessageData;		
		p->m_Next = NULL;
		p->m_MsgData = pMsg;
		if(m_uiMsgCount == 0)
		{
			m_MsgHeader = p;
			m_MsgTail = p;
		}
		else
		{			
			m_MsgTail->m_Next = p;
			m_MsgTail = p;			
		}
		m_uiMsgCount++;
		TccMessageUnLock();
		return 0;
	}

	void *TccMessageGet()
	{		
		TccMessageLock();
		if(m_uiMsgCount == 0)
		{
			TccMessageUnLock();
			return NULL;
		}
		TccMessageData *p;
		void *pdata;		
		p = m_MsgHeader;
		pdata = m_MsgHeader->m_MsgData;
		if(p->m_Next != NULL)
		{
			m_MsgHeader = m_MsgHeader->m_Next;
			m_uiMsgCount--;
		}
		else
		{
			m_MsgHeader = NULL;
			m_MsgTail = NULL;
			m_uiMsgCount = 0;	
		}		
		delete p;
		TccMessageUnLock();
		return pdata;
	}

	void *TccMessageHandleFirst()
	{
		void *ret;
		TccMessageLock();
		ret = (void *)m_MsgHeader;
		TccMessageUnLock();
		return ret;
	}

	void *TccMessageHandleNext(void *handle)
	{	
		void *ret;
		TccMessageData *TccMsgHandle;
		if(handle == NULL)
			return NULL;
		TccMessageLock();
		TccMsgHandle = (TccMessageData *)handle;
		ret = (void *)(TccMsgHandle->m_Next);
		TccMessageUnLock();
		return ret;
	}
	
	void *TccMessageHandleMsg(void *handle)
	{
		void *pdata;	
		TccMessageData *TccMsgHandle;
		if(handle == NULL)
			return NULL;
		TccMessageLock();
		TccMsgHandle = (TccMessageData *)handle;
		pdata = (void *)TccMsgHandle->m_MsgData;
		TccMessageUnLock();
		return pdata;
	}

private:
	unsigned int m_uiMsgCount;
	unsigned int m_uiMaxMsgCount;
	TccMessageData *m_MsgHeader;
	TccMessageData *m_MsgTail;
	pthread_mutex_t m_mutex;
	void TccMessageLock()
	{
		pthread_mutex_lock(&m_mutex);
	}
	void TccMessageUnLock()
	{
		pthread_mutex_unlock(&m_mutex);
	}
};

#ifdef __cplusplus
}
#endif

#endif //_TCC_MESSAGE_H__

