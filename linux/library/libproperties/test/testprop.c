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
#include <properties.h>

/* check create property folder */
void test_prop_1(void)
{
	char value[PROPERTY_VALUE_MAX];
	int ret = 0;
	int len = 0;
	int val = 0;

	#define KEY1 "tcc.test.create.folder"
	#define VALUE1 "created"

	printf("\n\n[Test1] create property folder\n");
	printf("\tproperty_set(%s, %s)\n", KEY1, VALUE1);
	ret = property_set(KEY1, VALUE1);

	printf("[Test1] check result 'ls -alR /tmp/'\n");
	system("ls -alR /tmp/");

	printf("[Test1] cat /tmp/property/tcc.test.create.folder\n");
	system("cat /tmp/property/tcc.test.create.folder");
}

/* test property_set() */
void test_prop_2(void)
{
	char value[PROPERTY_VALUE_MAX];
	int ret = 0;
	int len = 0;
	int val = 0;

	#define KEY2 "tcc.test.set.property"
	#define VALUE2 "created2nd"

	printf("\n\n[Test2] set property after creating folder '/tmp/property'\n");
	printf("\tproperty_set(%s, %s)\n", KEY2, VALUE2);

	ret = property_set(KEY2, VALUE2);

	printf("[Test2] check result 'ls -alR /tmp/'\n");
	system("ls -alR /tmp/");

	printf("[Test2] cat /tmp/property/tcc.test.set.property\n");
	system("cat /tmp/property/tcc.test.set.property");

}

/* test property_get() which exist*/
void test_prop_3(void)
{
	char value[PROPERTY_VALUE_MAX];
	int ret = 0;
	int len = 0;
	int val = 0;

	#define VALUE3 "val3"

	printf("\n\n[Test3] get property\n");
	printf("\tproperty_get(%s, value, %s)\n", KEY2, VALUE3);

	ret = property_get( KEY2, value, VALUE3 );

	printf("[Test3]	result KEY[%s] len[%d] value[%s]\n", KEY2, ret, value);
}

/* test property_get() which non exist*/
void test_prop_4(void)
{
	char value[PROPERTY_VALUE_MAX];
	int ret = 0;
	int len = 0;
	int val = 0;

	#define KEY4   "tcc.test.non.exist.key"
	#define VALUE4 "val4"

	printf("\n\n[Test4] get property\n");
	printf("\tproperty_get(%s, value, %s)\n", KEY4, VALUE4);

	ret = property_get( KEY4, value, VALUE4 );

	printf("[Test4]	result KEY[%s] len[%d] value[%s]\n", KEY4, ret, value);

	printf("[Test4] cat /tmp/property/tcc.test.non.exist.key\n");
	system("cat /tmp/property/tcc.test.non.exist.key");

}

/* test while use "" value */
void test_prop_5(void)
{
	char value[PROPERTY_VALUE_MAX];
	int ret = 0;
	int len = 0;
	int val = 0;

	#define KEY5   "tcc.test.set.test5"
	#define KEY5_2   "tcc.test.set.test5-2"
	#define EMPTY_VALUE ""
	#define DEFAULT_VALUE_5 "def5"
	printf("\n\n[Test5] set property empty string\n");
	
	printf("\tproperty_set(key=[%s],value[%s])\n", KEY5, EMPTY_VALUE);
	ret = property_set(KEY5, EMPTY_VALUE);
	printf("[Test5] result. cat /tmp/property/tcc.test.set.test5\n[");
	system("cat /tmp/property/tcc.test.set.test5");
	printf("]\n");

	printf("\n\n[Test5-1] get property empty string\n");
	printf("\tproperty_get(key=[%s], value, default[%s])\n", KEY5, DEFAULT_VALUE_5);
	ret = property_get( KEY5, value, DEFAULT_VALUE_5);
	printf("[Test5-1]	result get KEY[%s] len[%d] value[%s]\n", KEY5, ret, value);

	ret = property_set(KEY5_2, EMPTY_VALUE);

	printf("\n\n[Test5-2] get property empty string with empty default string\n");
	printf("\tproperty_get(key=[%s], value, default[%s])\n", KEY5_2, "");
	ret = property_get( KEY5_2, value, EMPTY_VALUE);
	printf("[Test5-2]	result get KEY[%s] len[%d] value[%s]\n", KEY5_2, ret, value);
	printf("[Test5] result. cat /tmp/property/tcc.test.set.test5-2\n[");
	system("cat /tmp/property/tcc.test.set.test5-2");
	printf("]\n");

}

/* test to make empty of exist key which has vaild value
   ex) key='test', value='123' => value = ''
*/
void test_prop_6(void)
{
	char value[PROPERTY_VALUE_MAX];
	int ret = 0;
	int len = 0;
	int val = 0;

	#define KEY6   "tcc.test.key6"
	#define VALUE6 "valid6"
	#define EMPTY_VALUE ""
	printf("\n\n[Test6] test to make empty of exist key which has vaild value\n");
	printf("[Test6]set key[%s] value[%s]\n", KEY6, VALUE6);

	ret = property_set(KEY6, VALUE6);

	printf("[Test6]	result, cat /tmp/property/%s\n[",KEY6);
	system("cat /tmp/property/tcc.test.key6");
	printf("]\n");

	printf("[Test6]set key[%s] value[%s]\n", KEY6, EMPTY_VALUE);
	
	ret = property_set(KEY6, EMPTY_VALUE);
	printf("[Test6]	result, cat /tmp/property/%s\n[",KEY6);
	system("cat /tmp/property/tcc.test.key6");
	printf("]\n");
	
}

/* 141203:bug fixed. when repeat call get_property with default empty value, file open fail. 
   becaused opened file isn't closed, so can't open file any more.
   FIX : add fclose() 
*/
void test_prop_7(void)
{
	char value[PROPERTY_VALUE_MAX];
	int i;
	int ret=0;
	#define KEY7   "tcc.test.empty.open"


	printf("\n\n[Test7] repeat call get_property with default empty value\n");
	printf("[Test7]set key[%s] value[%s]\n", KEY7, "EMPTY_VALUE");

	for(i=1; i<10000; i++)
	{
		printf("\tcount=%d\n",i);
		ret = property_get( KEY7, value, EMPTY_VALUE);
		printf("\	ret=%d\n",ret);
	}
}

int main(void)
{
	test_prop_1();
	test_prop_2();
	test_prop_3();
	test_prop_4();
	test_prop_5();
	test_prop_6();
	test_prop_7();
	return 0;
}


