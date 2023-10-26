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
#ifndef __PROPERTIES_H__
#define __PROPERTIES_H__

#ifdef __cplusplus
extern "C" {
#endif

/* PROP_NAME_MAX  is defined ANDROID_SDK/libc/include/sys/system_properties.h:38:#define PROP_NAME_MAX   32
   PROP_VALUE_MAX is defined ANDROID_SDK/libc/include/sys/system_properties.h:39:#define PROP_VALUE_MAX  92
*/
#define PROPERTY_KEY_MAX    32/*PROP_NAME_MAX*/
#define PROPERTY_VALUE_MAX  92/*PROP_VALUE_MAX*/

/* property_get: returns the length of the value which will never be
** greater than PROPERTY_VALUE_MAX - 1 and will always be zero terminated.
** (the length does not include the terminating zero).
**
** If the property read fails or returns an empty value, the default
** value is used (if nonnull).
*/
int property_get(const char *key, char *value, const char *default_value);

/* property_set: returns 0 on success, < 0 on failure
*/
int property_set(const char *key, const char *value);

/* // not defined in linux 
int property_list(void (*propfn)(const char *key, const char *value, void *cookie), void *cookie);    
*/

#ifdef __cplusplus
}
#endif

#endif //__PROPERTIES_H__
