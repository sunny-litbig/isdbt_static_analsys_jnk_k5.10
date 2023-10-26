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
#include <stdio.h>
#include <pmap.h>

#define PMAP_NAME "subtitle"
static pmap_t pmap_video;
#define DEST_PHY_BASE_ADDR	pmap_video.base
#define DEST_PHY_BASE_SIZE		pmap_video.size

int main(void);
void test_pmap(void);


int main(void)
{
	test_pmap();

	return 0;
}

void test_pmap(void)
{

	pmap_get_info(PMAP_NAME, &pmap_video);
	printf("[%s] BASE_ADDR : 0x%08X, BASE_SIZE : 0x%08X\n", PMAP_NAME, DEST_PHY_BASE_ADDR, DEST_PHY_BASE_SIZE);
}

