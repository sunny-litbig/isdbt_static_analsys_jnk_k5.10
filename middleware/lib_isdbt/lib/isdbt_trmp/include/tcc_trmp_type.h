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
#ifndef _TCC_TRMP_TYPE_H_
#define _TCC_TRMP_TYPE_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

// Define type of return value
#define TCC_TRMP_SUCCESS 0
#define TCC_TRMP_ERROR (-1)
#define TCC_TRMP_ERROR_UNSUPPORTED (-2)
#define TCC_TRMP_ERROR_INVALID_PARAM (-3)
#define TCC_TRMP_ERROR_INVALID_MEMORY (-4)
#define TCC_TRMP_ERROR_INVALID_STATE (-5)
#define TCC_TRMP_ERROR_STORAGE (-6)
#define TCC_TRMP_ERROR_CIPHER (-7)
#define TCC_TRMP_ERROR_FALSIFICATION_DETECT (-8)
#define TCC_TRMP_ERROR_UPDATE_KEY (-9)
#define TCC_TRMP_ERROR_INVALID_KEY (-10)
#define TCC_TRMP_ERROR_TEE (-11)
#define TCC_TRMP_ERROR_TEE_OPEN_SESSION (-12)
#define TCC_TRMP_ERROR_CRC (-13)
#define TCC_TRMP_ERROR_INVALID_PROTOCOL (-14)

// Define type of event
#define TCC_TRMP_EVENT_OK 0
#define TCC_TRMP_EVENT_ERROR_EC21 1
#define TCC_TRMP_EVENT_ERROR_EC22 2
#define TCC_TRMP_EVENT_ERROR_EC23 3
#define TCC_TRMP_EVENT_ERROR_LIBRARY_FALSIFICATION 4
#define TCC_TRMP_EVENT_ERROR_UNEXPECTED 5
#define TCC_TRMP_EVENT_ERROR_CRC 6
#define TCC_TRMP_EVENT_ERROR_TRANSMISSION_TYPE 7
#define TCC_TRMP_EVENT_ERROR_PROTOCAL_NUMBER 8

// Define length of data
#define TCC_TRMP_DEVICE_KEY_LENGTH 16
#define TCC_TRMP_DEVICE_ID_LENGTH 8

#define TCC_TRMP_WORK_KEY_LENGTH 16

#define TCC_TRMP_SCRAMBLE_KEY_LENGTH 8

#define TCC_TRMP_FALSIFICATION_DETECTION_KEY_LENGTH 16

#define TCC_TRMP_MULTI2_SYSTEM_KEY_LENGTH 32
#define TCC_TRMP_MULTI2_CBC_DEFAULT_VALUE_LENGTH 8

#define TCC_TRMP_CBC_DEFALUT_VALUE_COUNT 4
#define TCC_TRMP_CBC_DEFAULT_VALUE_LENGTH 16

#define TCC_TRMP_EMM_SECTION_MIN_LENGTH 30
#define TCC_TRMP_EMM_SECTION_MAX_LENGTH 4096
#define TCC_TRMP_ECMF1_SECTION_LENGTH 170

#define TCC_TRMP_EMM_FIXED_PART_LENGTH 14
#define TCC_TRMP_ECMF1_FIXED_PART_LENGTH 10

#define TCC_TRMP_ASSOCIATED_INFORMAION_BYTE_LENGTH 1
#define TCC_TRMP_PROTOCOL_NUMBER_LENGTH 1
#define TCC_TRMP_BROADCASTER_GROUP_IDENTIFIER_LENGTH 2
#define TCC_TRMP_UPDATE_NUMBER_LENGTH 2

#define TCC_TRMP_DATEANDTIME_LENGTH 5

#define TCC_TRMP_NETWORK_ID_LENGTH 2

// Define structure of TRMP
typedef enum _TCCTRMPState {
	TCC_TRMP_STATE_NONE,
	TCC_TRMP_STATE_INIT

} TCCTRMPState;

typedef enum _TCCTRMPKey { TCC_TRMP_KEY_ODD, TCC_TRMP_KEY_EVEN, TCC_TRMP_KEY_MAX } TCCTRMPKey;

typedef enum _TCCTRMPCIPHER { TCC_TRMP_CIPHER_HW, TCC_TRMP_CIPHER_SW } TCCTRMPCIPHER;

// Random number
typedef struct _TCCTRMPRandomNumberInfo {
	unsigned char ucGenerationNumber;
	unsigned int usUpdateParameter;
	unsigned char aRandomNumber[TCC_TRMP_DEVICE_KEY_LENGTH];
} TCCTRMPRandomNumberInfo;

// Common Data
typedef struct _TCCTRMPIDInfo {
	unsigned char aDeviceID[TCC_TRMP_DEVICE_ID_LENGTH];
	unsigned char aDeviceKey[TCC_TRMP_DEVICE_KEY_LENGTH];
	unsigned char aEMMFalsificationDetectionKey[TCC_TRMP_FALSIFICATION_DETECTION_KEY_LENGTH];
} TCCTRMPIDInfo;

typedef struct _TCCTRMPCommonData {
	unsigned char aMulti2SystemKey[TCC_TRMP_MULTI2_SYSTEM_KEY_LENGTH];
	unsigned char aMulti2CBCDefaultValue[TCC_TRMP_MULTI2_CBC_DEFAULT_VALUE_LENGTH];
	unsigned char aCBCDefaultValues[TCC_TRMP_CBC_DEFALUT_VALUE_COUNT]
								   [TCC_TRMP_CBC_DEFAULT_VALUE_LENGTH];

	TCCTRMPIDInfo stRMPModelID;
	TCCTRMPIDInfo stRMPManufacturerID;
} TCCTRMPCommonData;

// Broadcaster Individual Data
typedef struct _TCCTRMPF0WorkKeyInfo {
	unsigned char ucWorkKeyIdentifier;
	unsigned char aWorkkey[TCC_TRMP_WORK_KEY_LENGTH];
} TCCTRMPF0WorkKeyInfo;

typedef struct _TCCTRMPF1WorkKeyInfo {
	unsigned char ucWorkKeyIdentifier;
	unsigned char ucF1KsPointer;
	unsigned char aWorkkey[TCC_TRMP_WORK_KEY_LENGTH];
} TCCTRMPF1WorkKeyInfo;

typedef struct _TCCTRMPDeviceKeyInfo {
	unsigned char ucGenerationNumber;
	unsigned int usUpdateParameter;
	unsigned char aDeviceID[TCC_TRMP_DEVICE_ID_LENGTH];
	unsigned char aDeviceKey[TCC_TRMP_DEVICE_KEY_LENGTH];
} TCCTRMPDeviceKeyInfo;

typedef struct _TCCTRMPBroadcasterIndividualData {
	unsigned short usNetworkID;
	unsigned short usBroadcasterGroupIdentifier;

	TCCTRMPDeviceKeyInfo stRMPModelID;
	TCCTRMPDeviceKeyInfo stRMPManufacturerID;

	unsigned short usUpdateNumber;
	unsigned char ucWorkKeyInvalidFlag;

	TCCTRMPF0WorkKeyInfo stF0Workkey[TCC_TRMP_KEY_MAX];
	TCCTRMPF1WorkKeyInfo stF1Workkey[TCC_TRMP_KEY_MAX];

	unsigned int uiCRC32;
} TCCTRMPBroadcasterIndividualData;

typedef unsigned char* (*updateDeviceKey)(
	unsigned char ucGenerationNumber, unsigned int uiDeviceKeyUpdateParam);

#ifdef __cplusplus
}
#endif

#endif
