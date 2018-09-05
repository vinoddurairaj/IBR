/*****************************************************************************
 *
 *  MODULE NAME : GETOPT.H
 *
 *  COPYRIGHTS:
 *             This module contains code made available by IBM
 *             Corporation on an AS IS basis.  Any one receiving the
 *             module is considered to be licensed under IBM copyrights
 *             to use the IBM-provided source code in any way he or she
 *             deems fit, including copying it, compiling it, modifying
 *             it, and redistributing it, with or without
 *             modifications.  No license under any IBM patents or
 *             patent applications is to be implied from this copyright
 *             license.
 *
 *             A user of the module should understand that IBM cannot
 *             provide technical support for the module and will not be
 *             responsible for any consequences of use of the program.
 *
 *             Any notices, including this one, are not to be removed
 *             from the module without the prior written consent of
 *             IBM.
 *
 *  AUTHOR:   Original author:
 *                 G. R. Blair (BOBBLAIR at AUSVM1)
 *                 Internet: bobblair@bobblair.austin.ibm.com
 *
 *            Extensively revised by:
 *                 John Q. Walker II, Ph.D. (JOHHQ at RALVM6)
 *                 Internet: johnq@ralvm6.vnet.ibm.com
 *
 *****************************************************************************/
#ifndef _TEST_TDI_GETOPT_H_
#define _TEST_TDI_GETOPT_H_

extern char * optarg;
extern int    optind;

#define SFTK_TDI_TCP_SERVER_BASE_NAME_A        "SftkTdiTcpServer"
#define SFTK_TDI_TCP_SERVER_BASE_NAME_W        L"SftkTdiTcpServer"
#define SFTK_TDI_TCP_SERVER_DISPLAY_NAME_A     "Sftk TDI TTCP TCP Server"
#define SFTK_TDI_TCP_SERVER_DISPLAY_NAME_W     L"Sftk TDI TTCP TCP Server"
#define SFTK_TDI_TCP_SERVER_DEVICE_NAME_A      "\\Device\\SftkTdiTcpServer"
#define SFTK_TDI_TCP_SERVER_DEVICE_NAME_W      L"\\Device\\SftkTdiTcpServer"

#ifdef UNICODE

#define SFTK_TDI_TCP_SERVER_BASE_NAME       SFTK_STFK_TDI_TCP_SERVER_BASE_NAME_W
#define SFTK_TDI_TCP_SERVER_DISPLAY_NAME    SFTK_TDI_TCP_SERVER_DISPLAY_NAME_W
#define SFTK_TDI_TCP_SERVER_DEVICE_NAME     SFTK_TDI_TCP_SERVER_DEVICE_NAME_W

#else

#define SFTK_TDI_TCP_SERVER_BASE_NAME       SFTK_TDI_TCP_SERVER_BASE_NAME_A
#define SFTK_TDI_TCP_SERVER_DISPLAY_NAME    SFTK_TDI_TCP_SERVER_DISPLAY_NAME_A
#define SFTK_TDI_TCP_SERVER_DEVICE_NAME     SFTK_TDI_TCP_SERVER_DEVICE_NAME_A

#endif

//
// TDI TCP Client Device Name Strings
//
#define SFTK_TDI_TCP_CLIENT_BASE_NAME_A        "SftkTdiTcpClient"
#define SFTK_TDI_TCP_CLIENT_BASE_NAME_W        L"SftkTdiTcpClient"
#define SFTK_TDI_TCP_CLIENT_DISPLAY_NAME_A     "Sftk TDI TTCP TCP Client"
#define SFTK_TDI_TCP_CLIENT_DISPLAY_NAME_W     L"Sftk TDI TTCP TCP Client"
#define SFTK_TDI_TCP_CLIENT_DEVICE_NAME_A      "\\Device\\SftkTdiTcpClient"
#define SFTK_TDI_TCP_CLIENT_DEVICE_NAME_W      L"\\Device\\SftkTdiTcpClient"

#ifdef UNICODE

#define SFTK_TDI_TCP_CLIENT_BASE_NAME       SFTK_TDI_TCP_CLIENT_BASE_NAME_W
#define SFTK_TDI_TCP_CLIENT_DISPLAY_NAME    SFTK_TDI_TCP_CLIENT_DISPLAY_NAME_W
#define SFTK_TDI_TCP_CLIENT_DEVICE_NAME     SFTK_TDI_TCP_CLIENT_DEVICE_NAME_W

#else

#define SFTK_TDI_TCP_CLIENT_BASE_NAME       SFTK_TDI_TCP_CLIENT_BASE_NAME_A
#define SFTK_TDI_TCP_CLIENT_DISPLAY_NAME    SFTK_TDI_TCP_CLIENT_DISPLAY_NAME_A
#define SFTK_TDI_TCP_CLIENT_DEVICE_NAME     SFTK_TDI_TCP_CLIENT_DEVICE_NAME_A

#endif

enum sm_commands 
{
sm_add = 1 , 
sm_remove , 
sm_enable , 
sm_disable , 
sm_init , 
sm_start , 
sm_stop , 
sm_uninit , 
sm_logicalgroup , 
sm_sendwindowsize , 
sm_receivewindowsize , 
sm_maxnumberofsendbuffers , 
sm_maxnumberofreceivebuffers , 
sm_chunksize , 
sm_chunkdelay , 
sm_primary , 
sm_secondary , 
sm_sourceipaddress , 
sm_remoteipaddress , 
sm_sourceport , 
sm_remoteport ,
sm_sendnormal ,
sm_sendrefresh
};

#define MAX_COMMAND_STRING_LENGTH	20

struct Command_t {
	enum sm_commands eCommand;
	char strCommand[MAX_COMMAND_STRING_LENGTH];
	BOOL bOption;
};


typedef struct _TDITTCP_TEST_PARAMS
{
	enum commands		m_eCommandType;
	CONNECTION_DETAILS	m_ConnDetails;
	SM_INIT_PARAMS		m_SMInitParams;
   // Inidicates whether it is the Connect Side or the Listen Side
   BOOLEAN  m_bTransmit;
}TDITTCP_TEST_PARAMS, *PTDITTCP_TEST_PARAMS;

typedef struct _TTCP_TEST_START_CMD
{
   ULONG                m_Status;
   TDITTCP_TEST_PARAMS  m_TestParams;

}TTCP_TEST_START_CMD, *PTTCP_TEST_START_CMD;


int getopt(int argc, char *argv[]);
char* getcommand(int command);

#endif //_TEST_TDI_GETOPT_H_