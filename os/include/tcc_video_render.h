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
#ifndef _TCC_VIDEO_RENDER_H_
#define _TCC_VIDEO_RENDER_H_

class TCCVideoRender
{
public:
	TCCVideoRender(int flag);
	~TCCVideoRender();
	int SetVideoSurface(void *native);
	int ReleaseVideoSurface(void);
	int SetNativeWindow(int width, int height, int format, int outmode);
	int dequeueBuffer(unsigned char **buf, int width, int height, int pts);
	void enqueueBuffer(unsigned char *buf, int width, int height, int pts);
	int IsVSyncEnabled();
	void setCrop(int left, int top, int width, int height);
	void *GetPrivateAddr(int fd_val, int width, int height);
	int SetDisplaySize(void);
	int getfromsysfs(const char *sysfspath, char *val);


private:
	class CDisplayPos
	{
	public:
		friend class TCCVideoRender;
		CDisplayPos() { m_iWidth = 1024; m_iHeight = 600;};
		virtual ~CDisplayPos() {};

	private:
		int m_iWidth;
		int m_iHeight;
	};

private:
	CDisplayPos m_clDisplayPos;

};

#endif//_TCC_VIDEO_RENDER_H_
