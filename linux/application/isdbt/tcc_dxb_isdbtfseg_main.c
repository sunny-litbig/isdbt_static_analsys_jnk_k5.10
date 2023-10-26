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
/******************************************************************************
* include 
******************************************************************************/
//#include <main.h>
//#include <time.h>
//#include "tcc_message.h"
#include "tcc_dxb_isdbtfseg_cui.h"
#include "tcc_dxb_isdbtfseg_process.h"
//#include "tcc_dxb_isdbtfseg_ui_process.h"
//#include "tcc_ipc_dxb_isdbtfseg_player_process.h"

//#include "tcc_monitoring_client.h"
#include "tcc_dxb_isdbtfseg_main.h"

/******************************************************************************
* locals
******************************************************************************/
int Run_flag = TCC_DXB_ISDBTFSEG_STATE_EXIT;

int Init_Video_Output_Path = 2;	// 0:LCD, 1:COMPOSITE, 2:HDMI
int Init_CountryCode = 0;//TCC_ISDBT_FEATURE_COUNTRY_JAPAN;	// 0:JAPAN, 1:BRAZIL
int Init_ProcessMonitoring = 0;// 0:Disable, 1:Enable
int Init_Support_UART_Console = 0;	// 0:Disable, 1:Enable 
int Init_stand_alone_processing	= 0;
int Init_Not_Skip_Same_program	= 0;
int Init_Disable_DualDecode = 0;
int Init_Mute_GPIO_Port = -1;	/* -1[Disable] */
int Init_Mute_GPIO_Polarity = 1;	/* 1[ON:1,OFF:0] 0[ON:0,OFF:1] */
int Init_StrengthFullToOne = 0;
int Init_StrengthOneToFull = 0;
int Init_CountSecFullToOne = 0;
int Init_CountSecOneToFull = 0;
int Init_Support_PVR = 0;
int Init_PVR_CountryCode_Mode = 0;	/* 0:FromFile 1:FromSystemSetting */
int Init_PrintLog_SignalStrength = 0;

/******************************************************************************
* Functions
******************************************************************************/
#if 0
int monitoring_register_DXB_SERVICE(int argc, char *argv[])
{
	unsigned char szExecString[256];
	unsigned char *szTemp;
	int i;
	
	szTemp = szExecString;
	szTemp += sprintf(szTemp, "/tcc_gtk/bin/./TCC_DXB_ISDBTFSEG_SERVICE");

	for(i=1; i<argc; i++) {
		szTemp += sprintf(szTemp, " %s", argv[i]);
	}

	if(TCC_Monitoring_Client_Init(szExecString, MONITORING_DXB_PROCESS_IDX, getpid())) {
		if(Init_ProcessMonitoring) {
			TCC_Monitoring_Client_Start();
		}
	}

	return 0;
}
#endif

int	main(int argc, char *argv[])
{
	int i;

	/*----------------------------------*/
	// Init Processes
	/*----------------------------------*/	
	printf("\n\n");
	printf("##########################################################################\n");	
	printf("##########################################################################\n");		
	printf("########### Welcome to TELECHIPS ISDBT-FULLSEG Framework Service #########\n");
	printf("##########################################################################\n");	
	printf("##########################################################################\n");			
	printf("DXB_SERVICE_VER [%s]\n", _DXB_SERVICE_PATCH_VER_ );

	/* input argument parsing */
	for (i = 1; i < argc; ++i)
	{	
		if ((!strcmp(argv[i], "-vout")) && ((i+1) < argc))
		{
			Init_Video_Output_Path = atoi(argv[++i]);
			if ( Init_Video_Output_Path > 2 )
				Init_Video_Output_Path = 2;
			printf("#### Init_Video_Output_Path(%d) \n", Init_Video_Output_Path);
		}
		else if ((!strcmp(argv[i], "-country")) && ((i+1) < argc))
		{
			Init_CountryCode = atoi(argv[++i]);
			if ( Init_CountryCode >= 2/*TCC_ISDBT_FEATURE_COUNTRY_MAX*/ )
				Init_CountryCode = 0/*TCC_ISDBT_FEATURE_COUNTRY_JAPAN*/;  // 0:JAPAN, 1:BRAZIL
			printf("#### Init_CountryCode(%d) \n", Init_CountryCode);
		}
		else if ((!strcmp(argv[i], "-monitor")) && ((i+1) < argc))
		{
			Init_ProcessMonitoring = atoi(argv[++i]);
			if ( Init_ProcessMonitoring > 1 )
				Init_ProcessMonitoring = 1;
			printf("#### Init_ProcessMonitoring(%d) \n", Init_ProcessMonitoring);
		}
		else if ((!strcmp(argv[i], "-uartconsole")) && ((i+1) < argc))
		{
			Init_Support_UART_Console = atoi(argv[++i]);
			if ( Init_Support_UART_Console > 1 )
				Init_Support_UART_Console = 1;
			printf("#### Init_Support_UART_Console(%d) \n", Init_Support_UART_Console);
		}
		else if ((!strcmp(argv[i], "-standalone")) && ((i+1) < argc))
		{
			Init_stand_alone_processing = atoi(argv[++i]);
			if ( Init_stand_alone_processing > 1 )
				Init_stand_alone_processing = 1;
			printf("#### Init_stand_alone_processing(%d) \n", Init_stand_alone_processing);
		}
		else if ((!strcmp(argv[i], "-notskipsameprog")) && ((i+1) < argc))
		{
			Init_Not_Skip_Same_program = atoi(argv[++i]);
			if ( Init_Not_Skip_Same_program > 1 )
				Init_Not_Skip_Same_program = 1;
			printf("#### Init_Not_Skip_Same_program(%d) \n", Init_Not_Skip_Same_program);
		}
		else if ((!strcmp(argv[i], "-notdualdec")) && ((i+1) < argc))
		{
			Init_Disable_DualDecode = atoi(argv[++i]);
			if ( Init_Disable_DualDecode > 1 )
				Init_Disable_DualDecode = 1;
			printf("#### Init_Disable_DualDecode(%d) \n", Init_Disable_DualDecode);
		}
		else if ((!strcmp(argv[i], "-mutegpioport")) && ((i+1) < argc))
		{
			Init_Mute_GPIO_Port = atoi(argv[++i]);
			printf("#### Init_Mute_GPIO_Port(%d) \n", Init_Mute_GPIO_Port);
		}
		else if ((!strcmp(argv[i], "-mutegpiopol")) && ((i+1) < argc))
		{
			Init_Mute_GPIO_Polarity = atoi(argv[++i]);
			if ( Init_Mute_GPIO_Polarity > 1 )
				Init_Mute_GPIO_Polarity = 1;
			printf("#### Init_Mute_GPIO_Polarity(%d) \n", Init_Mute_GPIO_Polarity);
		}
		else if ((!strcmp(argv[i], "-strengthFto1")) && ((i+1) < argc))
		{
			Init_StrengthFullToOne = atoi(argv[++i]);
			if ( Init_StrengthFullToOne > 50 )
				Init_StrengthFullToOne = 50;
			printf("#### Init_StrengthFullToOne(%d) \n", Init_StrengthFullToOne);
		}
		else if ((!strcmp(argv[i], "-strength1toF")) && ((i+1) < argc))
		{
			Init_StrengthOneToFull = atoi(argv[++i]);
			if ( Init_StrengthOneToFull > 50 )
				Init_StrengthOneToFull = 50;
			printf("#### Init_StrengthOneToFull(%d) \n", Init_StrengthOneToFull);
		}
		else if ((!strcmp(argv[i], "-countFto1")) && ((i+1) < argc))
		{
			Init_CountSecFullToOne = atoi(argv[++i]);
			if ( Init_CountSecFullToOne > 10 )
				Init_CountSecFullToOne = 10;
			printf("#### Init_CountSecFullToOne(%d) \n", Init_CountSecFullToOne);
		}
		else if ((!strcmp(argv[i], "-count1toF")) && ((i+1) < argc))
		{
			Init_CountSecOneToFull = atoi(argv[++i]);
			if ( Init_CountSecOneToFull > 10 )
				Init_CountSecOneToFull = 10;
			printf("#### Init_CountSecOneToFull(%d) \n", Init_CountSecOneToFull);
		}
		else if ((!strcmp(argv[i], "-pvr")) && ((i+1) < argc))
		{
			Init_Support_PVR = atoi(argv[++i]);
			if ( Init_Support_PVR > 1 )
				Init_Support_PVR = 1;
			printf("#### Init_Support_PVR(%d) \n", Init_Support_PVR);
		}
		else if ((!strcmp(argv[i], "-fplaycountrymode")) && ((i+1) < argc))
		{
			Init_PVR_CountryCode_Mode = atoi(argv[++i]);
			if ( Init_PVR_CountryCode_Mode > 1 )
				Init_PVR_CountryCode_Mode = 1;
			printf("#### Init_PVR_CountryCode_Mode(%d) \n", Init_PVR_CountryCode_Mode);
		}
		else if ((!strcmp(argv[i], "-logsignal")) && ((i+1) < argc))
		{
			Init_PrintLog_SignalStrength = atoi(argv[++i]);
			printf("#### Init_PrintLog_SignalStrength(%d) \n", Init_PrintLog_SignalStrength);
		}
	}



	
	printf("##########################################################################\n");			
	printf("\n\n");

#if 0
	monitoring_register_DXB_SERVICE( argc, argv );
#endif

	#ifdef TCC_CUI_INCLUDE
	Init_Support_UART_Console	= 1;
	#else
	Init_Support_UART_Console	= 0;
	#endif

	if (Init_stand_alone_processing > 0)
	{
		Init_Support_UART_Console = 1;

#if 0
		if (TCC_ExtendDisplay_GetHPDDetect())
			Init_Video_Output_Path = 2;
		else
			Init_Video_Output_Path = 1;
#endif
	}
	
	#ifdef TCC_TIMER_INCLUDE
	tcc_timer_init();
	#endif

#if 0
	tcc_ipc_dxb_isdbtfseg_player_init();
#endif

	if (TCC_DXB_ISDBTFSEG_PROCESS_Init(Init_CountryCode))
	{
		printf("[ISDBTFSEG_PROC] Process Init Failed \n");
		return -1;
	}
	
	if (TCC_DXB_ISDBTFSEG_UI_PROCESS_Init())
	{
		printf("[ISDBTFSEG_PROC] UI Process Init Failed \n");
		return -1;
	}

	/*----------------------------------*/
	// Main Processes
	/*----------------------------------*/	
	Run_flag = TCC_DXB_ISDBTFSEG_STATE_RUNNING;
	while (Run_flag == TCC_DXB_ISDBTFSEG_STATE_RUNNING)
	{
		#ifdef TCC_TIMER_INCLUDE	
		tcc_timer_process();
		#endif		

		Run_flag = TCC_DXB_ISDBTFSEG_PROCESS_ProcMessage();
		if ( Run_flag != TCC_DXB_ISDBTFSEG_STATE_RUNNING )
			break;
			
#if 0
		Run_flag = tcc_ipc_dxb_isdbtfseg_player_recv_msg();
#endif
	};

	/*----------------------------------*/
	// DeInit Processes
	/*----------------------------------*/	

	#ifdef TCC_TIMER_INCLUDE
	tcc_timer_deinit();
	#endif

	TCC_DXB_ISDBTFSEG_UI_PROCESS_DeInit();
	TCC_DXB_ISDBTFSEG_PROCESS_Deinit();

#if 0
	tcc_ipc_dxb_isdbtfseg_player_deinit();
#endif
	
	printf("\n\n");
	printf("##########################################################################\n");
	printf("##########################################################################\n");
	printf("############### Close TELECHIPS ISDBT-FULLSEG Framework Service ##########\n");
	printf("##########################################################################\n");
	printf("##########################################################################\n");
	printf("\n\n");

#if 0
	if( Init_ProcessMonitoring )
	{
		TCC_Monitoring_Client_Stop();
	}
	TCC_Monitoring_Client_DeInit();
#endif	
	return 0;
}

