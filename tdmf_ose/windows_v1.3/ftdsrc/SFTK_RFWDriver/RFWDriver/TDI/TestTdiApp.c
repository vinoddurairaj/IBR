// TestTdiApp.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include <winsock2.h>
#include <winioctl.h>

// Add all the definitions that are required in the User Space
#define USER_SPACE

#include <pshpack1.h>
#include "../ftdio.h"
#include "TestTdi_Getopt.h"
#include <packoff.h>

//
// Usage Message
//
char Usage[] = "\
Usage: ttcpctrl -t [-options] host\n\
       ttcpctrl -r [-options]\n\
Common options:\n\
	-l ##	length of bufs read from or written to network (default 8192)\n\
	-s	-t: source a pattern to network\n\
		-r: sink (discard) all data from network\n\
Options specific to -t:\n\
	-n ##	number of source bufs written to network (default 2048)\n\
	-D	don't buffer TCP writes (sets TCP_NODELAY TCP option)\n\
Options specific to -r:\n\
";

VOID IntitializeTestDefaults( PTTCP_TEST_START_CMD pStartCmd )
{
   memset( pStartCmd, 0x00, sizeof( TTCP_TEST_START_CMD ) );

   pStartCmd->m_TestParams.m_bTransmit = TRUE;
   pStartCmd->m_TestParams.m_eCommandType = sm_add;

   pStartCmd->m_TestParams.m_ConnDetails.nConnections = 1;
   pStartCmd->m_TestParams.m_ConnDetails.ConnectionDetails[0].nNumberOfSessions = 1;

   pStartCmd->m_TestParams.m_SMInitParams.nSendWindowSize = SFTK_MAX_IO_SIZE;
   pStartCmd->m_TestParams.m_SMInitParams.nReceiveWindowSize = SFTK_MAX_IO_SIZE;
   pStartCmd->m_TestParams.m_SMInitParams.nMaxNumberOfSendBuffers = DEFAULT_MAX_SEND_BUFFERS;
   pStartCmd->m_TestParams.m_SMInitParams.nMaxNumberOfReceiveBuffers = 1;
   pStartCmd->m_TestParams.m_SMInitParams.nChunkSize = 0;
   pStartCmd->m_TestParams.m_SMInitParams.nChunkDelay = 0;

}

void DUMP_ConnectionInfo(PCONNECTION_DETAILS lpConnectionDetails)
{
	PCONNECTION_INFO pConnectionInfo = NULL;
	int i =0;

	printf("The Logical Group is %d \n",lpConnectionDetails->lgnum);
	printf( "Number of Connections = %d\n" , lpConnectionDetails->nConnections );

	pConnectionInfo = lpConnectionDetails->ConnectionDetails;
	for(i=0;i<lpConnectionDetails->nConnections;i++)
	{
		printf("The Connection Local Address = %x , Local Port = %x , \n RemoteAddres = %x , Remote Port = %x , \n NumberOfSession = %x\n",
			   pConnectionInfo->ipLocalAddress.in_addr, pConnectionInfo->ipLocalAddress.sin_port,
			   pConnectionInfo->ipRemoteAddress.in_addr, pConnectionInfo->ipRemoteAddress.sin_port,
			   pConnectionInfo->nNumberOfSessions);
		pConnectionInfo +=1;
	}
}

VOID TDITTCP_DumpTestParams( PTDITTCP_TEST_PARAMS pTestParams )
{
   if( pTestParams->m_bTransmit )
   {
      printf( "Connetion Side Testing\n" );
   }
   else
   {
      printf( "Listen Side Testing\n" );
   }

   printf("The SessionManager Initialization Parameters are \n");
   printf("\n\tnSendWindowSize = %d \n, \tnMaxNumberOfSendBuffers = %d \n, \tnReceiveWindowSize = %d \n, \tnMaxNumberOfReceiveBuffers = %d \n, \tnChunkSize = %d \n, \tnChunkDelay = %d\n\n",
	   pTestParams->m_SMInitParams.nSendWindowSize,
	   pTestParams->m_SMInitParams.nMaxNumberOfSendBuffers,
	   pTestParams->m_SMInitParams.nReceiveWindowSize,
	   pTestParams->m_SMInitParams.nMaxNumberOfReceiveBuffers,
	   pTestParams->m_SMInitParams.nChunkSize,
	   pTestParams->m_SMInitParams.nChunkDelay);


   printf("The Type of command is %d\n",pTestParams->m_eCommandType);
   DUMP_ConnectionInfo(&pTestParams->m_ConnDetails);
}

int main(int argc, CHAR* argv[])
{
	HANDLE   hTcpClientHandle = INVALID_HANDLE_VALUE;
	WSADATA  WsaData;
	CHAR     szDeviceName[ _MAX_PATH ];
	BOOLEAN  bRc;
	DWORD    bytesReturned;
	TTCP_TEST_START_CMD  StartCmd;
	LONG      c;
	DWORD    StartTestIoctl = 0;
	PCONNECTION_DETAILS lpConnectionDetails = NULL;
	PCONNECTION_INFO lpConnectionInfo = NULL;
	DWORD size = 0;
	DWORD dwRet = 0;
	PCHAR lpopt = NULL;
	CHAR strLocalHostName[20];
	ULONG ulLocalIP =0;

	if (argc < 2)
	{
		fprintf(stderr,Usage);
		exit( 0 );
	}

	printf("The Commmand Syntax is \n");
	printf("TestTdiApp.exe <-l lg>  [ <-t> | <-r> ] \
		   [ <-a> | <-b> ] \
		   [ <-w> | <-x> | <-y> | <-z> ] <-S SrcIp> <-s SrcPort> <-D DstIp> <-d DspPort> \n");

	IntitializeTestDefaults(&StartCmd);

   while (optind != argc)
   {

	   c = getopt(argc, argv);

	   lpopt = getcommand(c);

	   if(lpopt != NULL)
	   {
			printf("comamnd = %s\n",lpopt);

			if(optarg != NULL)
			{
				printf("optarg = %s\n",optarg);
			}
	   }
	   else
	   {
		   printf("Error in input data exit\n");
		   exit(0);
	   }

      switch (c)
      {
		 case sm_add:	// Add Command
			 StartCmd.m_TestParams.m_eCommandType = sm_add;
			 StartTestIoctl = SFTK_IOCTL_TCP_ADD_CONNECTIONS;
			 break;

		 case sm_remove:	// Remove Command
			 StartCmd.m_TestParams.m_eCommandType = sm_remove;
			 StartTestIoctl = SFTK_IOCTL_TCP_REMOVE_CONNECTIONS;
			 break;

		 case sm_enable:	// Enable Command
			 StartCmd.m_TestParams.m_eCommandType = sm_enable;
			 StartTestIoctl = SFTK_IOCTL_TCP_ENABLE_CONNECTIONS;
			 break;

		 case sm_disable:	// Disable Command
			 StartCmd.m_TestParams.m_eCommandType = sm_disable;
			 StartTestIoctl = SFTK_IOCTL_TCP_DISABLE_CONNECTIONS;
			 break;

		 case sm_init:	// Init
			 StartCmd.m_TestParams.m_eCommandType = sm_init;
            break;

		 case sm_start:	// Start Command
			 StartCmd.m_TestParams.m_eCommandType = sm_start;
            break;

		 case sm_stop:	// Stop Command
			 StartCmd.m_TestParams.m_eCommandType = sm_stop;
            break;

		 case sm_uninit:	// Uninit
			 StartCmd.m_TestParams.m_eCommandType = sm_uninit;
            break;

		 case sm_logicalgroup:	// Logical Group Number
			 StartCmd.m_TestParams.m_ConnDetails.lgnum = atoi(optarg);
            break;

		 case sm_sendwindowsize:	// Send Window Size
			 StartCmd.m_TestParams.m_SMInitParams.nSendWindowSize = atoi(optarg);
            break;

		 case sm_receivewindowsize:	// Receive Window Size
			 StartCmd.m_TestParams.m_SMInitParams.nReceiveWindowSize = atoi(optarg);
            break;

		 case sm_maxnumberofsendbuffers:	// Number of Send Buffers
			 StartCmd.m_TestParams.m_SMInitParams.nMaxNumberOfSendBuffers = atoi(optarg);
            break;

		 case sm_maxnumberofreceivebuffers:	// Number of Receive Buffers
			 StartCmd.m_TestParams.m_SMInitParams.nMaxNumberOfReceiveBuffers = atoi(optarg);
            break;

		 case sm_chunksize:	// Chunk Size for the Send Thread
			 StartCmd.m_TestParams.m_SMInitParams.nChunkSize = atoi(optarg);
            break;

		 case sm_chunkdelay:	// Throttle for the Send Thread
			 StartCmd.m_TestParams.m_SMInitParams.nChunkDelay = atoi(optarg);
            break;

         case sm_primary:	// Primary Side
            StartCmd.m_TestParams.m_bTransmit = TRUE;
            break;

         case sm_secondary:	// Secondary Side
            StartCmd.m_TestParams.m_bTransmit = FALSE;
            break;

		 case sm_sourceipaddress:	// Primary Side IP Address
			 StartCmd.m_TestParams.m_ConnDetails.ConnectionDetails[0].ipLocalAddress.in_addr = inet_addr(optarg);
			 ulLocalIP = StartCmd.m_TestParams.m_ConnDetails.ConnectionDetails[0].ipLocalAddress.in_addr;
			 sprintf(strLocalHostName, "%d.%d.%d.%d",
									(BYTE)(	(ulLocalIP)		& 0x000000FF), 
									(BYTE)(	(ulLocalIP>>8)	& 0x000000FF), 
									(BYTE)(	(ulLocalIP>>16) & 0x000000FF), 
									(BYTE)(	(ulLocalIP>>24)	& 0x000000FF));

			printf("The Src Ip Address is %s\n",strLocalHostName);
			 break;

		 case sm_remoteipaddress:	// Secondary Side IP Address
			 StartCmd.m_TestParams.m_ConnDetails.ConnectionDetails[0].ipRemoteAddress.in_addr = inet_addr(optarg);
			 ulLocalIP = StartCmd.m_TestParams.m_ConnDetails.ConnectionDetails[0].ipRemoteAddress.in_addr;
			 sprintf(strLocalHostName, "%d.%d.%d.%d",
									(BYTE)(	(ulLocalIP)		& 0x000000FF), 
									(BYTE)(	(ulLocalIP>>8)	& 0x000000FF), 
									(BYTE)(	(ulLocalIP>>16) & 0x000000FF), 
									(BYTE)(	(ulLocalIP>>24)	& 0x000000FF));

			printf("The Rem Ip Address is %s\n",strLocalHostName);
			 break;

		 case sm_sourceport:	// Primary Side Port Number
			 StartCmd.m_TestParams.m_ConnDetails.ConnectionDetails[0].ipLocalAddress.sin_port = htons((USHORT)atoi(optarg));
			 break;

		 case sm_remoteport:	// Secondary Side Port Number
			 StartCmd.m_TestParams.m_ConnDetails.ConnectionDetails[0].ipRemoteAddress.sin_port = htons((USHORT)atoi(optarg));
			 break;

		 case sm_sendnormal:
			 StartCmd.m_TestParams.m_eCommandType = sm_sendnormal;
			 StartTestIoctl = SFTK_IOCTL_TCP_SEND_NORMAL_DATA;
			 break;

		 case sm_sendrefresh:
			 StartCmd.m_TestParams.m_eCommandType = sm_sendrefresh;
			 StartTestIoctl = SFTK_IOCTL_TCP_SEND_REFRESH_DATA;
			 break;

         case EOF:	// End of Command
            optarg = argv[optind];
            optind++;
            break;

		   default:
            fprintf(stderr,Usage);
            exit( 0 );
      }
   }

   if( WSAStartup( MAKEWORD(0x02,0x00), &WsaData ) == SOCKET_ERROR )
   {
      fprintf(stderr,Usage);
      fprintf(stderr,"WSAStartup Failed\n" );
      exit(0);
   }

   __try
   {
		//
		// Setup For Test
		//
		if( StartCmd.m_TestParams.m_bTransmit )
		{
			// TCP Client Connects to the Server
			sprintf( szDeviceName, "%s%s", "\\\\.\\",
				SFTK_TDI_TCP_CLIENT_BASE_NAME
				);

		}
		else
		{
			// TCP Server Listens for the connections
			sprintf( szDeviceName, "%s%s", "\\\\.\\",
				SFTK_TDI_TCP_SERVER_BASE_NAME
				);
		}
   }
   __finally
   {
   }

   //
   // Display Selected Test Parameters
   //
   TDITTCP_DumpTestParams( &StartCmd.m_TestParams );

   __try
   {
		//
		// Open The TDITTCP Device
		//
		hTcpClientHandle = CreateFile(
								szDeviceName,
								GENERIC_READ | GENERIC_WRITE,
								0,
								NULL,
								OPEN_EXISTING,
								FILE_FLAG_NO_BUFFERING | FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
								NULL
								);

		if( hTcpClientHandle == INVALID_HANDLE_VALUE )
		{
			dwRet = 1;
			printf( "Unable To Open %s Handle\n", szDeviceName );
			__leave;
		}

		printf( "Opened %s Handle\n", szDeviceName );


		if((StartCmd.m_TestParams.m_eCommandType == sm_start) || 
			(StartCmd.m_TestParams.m_eCommandType == sm_stop) ||
			(StartCmd.m_TestParams.m_eCommandType == sm_init) ||
			(StartCmd.m_TestParams.m_eCommandType == sm_uninit))
		{
			if(StartCmd.m_TestParams.m_eCommandType == sm_start)
			{
				if(StartCmd.m_TestParams.m_bTransmit == TRUE)
				{
					size = sizeof(StartCmd.m_TestParams.m_SMInitParams);
					bRc = DeviceIoControl(
								hTcpClientHandle, 
								SFTK_IOCTL_START_PMD,
								(LPVOID)&StartCmd.m_TestParams.m_SMInitParams,
								size,
								NULL, 
								0,
								&bytesReturned,
								NULL 
								);

					if ( !bRc )
					{
						dwRet = 1;
						printf ( "Error in DeviceIoControl : %d", GetLastError());
						__leave;
					}
				}
				else
				{
					size = sizeof(StartCmd.m_TestParams.m_SMInitParams);
					bRc = DeviceIoControl(
								hTcpClientHandle, 
								SFTK_IOCTL_START_RMD,
								(LPVOID)&StartCmd.m_TestParams.m_SMInitParams,
								size,
								NULL, 
								0,
								&bytesReturned,
								NULL 
								);

					if ( !bRc )
					{
						dwRet = 1;
						printf ( "Error in DeviceIoControl : %d", GetLastError());
						__leave;
					}
				}
			} // if sm_start
			else if(StartCmd.m_TestParams.m_eCommandType == sm_stop)
			{
				if(StartCmd.m_TestParams.m_bTransmit == TRUE)
				{
					bRc = DeviceIoControl(
								hTcpClientHandle, 
								SFTK_IOCTL_STOP_PMD,
								NULL,
								0,
								NULL, 
								0,
								&bytesReturned,
								NULL 
								);

					if ( !bRc )
					{
						dwRet = 1;
						printf ( "Error in DeviceIoControl : %d", GetLastError());
						__leave;
					}
				}
				else
				{
					bRc = DeviceIoControl(
								hTcpClientHandle, 
								SFTK_IOCTL_STOP_RMD,
								NULL,
								0,
								NULL, 
								0,
								&bytesReturned,
								NULL 
								);
					if ( !bRc )
					{
						dwRet = 1;
						printf ( "Error in DeviceIoControl : %d", GetLastError());
						__leave;
					}
				}
			} // if sm_stop
			else if(StartCmd.m_TestParams.m_eCommandType == sm_init)
			{
				if(StartCmd.m_TestParams.m_bTransmit == TRUE)
				{
					size = sizeof(StartCmd.m_TestParams.m_SMInitParams);
					bRc = DeviceIoControl(
								hTcpClientHandle, 
								SFTK_IOCTL_INIT_PMD,
								(LPVOID)&StartCmd.m_TestParams.m_SMInitParams,
								size,
								NULL, 
								0,
								&bytesReturned,
								NULL 
								);

					if ( !bRc )
					{
						dwRet = 1;
						printf ( "Error in DeviceIoControl : %d", GetLastError());
						__leave;
					}
				}
				else
				{
					size = sizeof(StartCmd.m_TestParams.m_SMInitParams);
					bRc = DeviceIoControl(
								hTcpClientHandle, 
								SFTK_IOCTL_INIT_RMD,
								(LPVOID)&StartCmd.m_TestParams.m_SMInitParams,
								size,
								NULL, 
								0,
								&bytesReturned,
								NULL 
								);

					if ( !bRc )
					{
						dwRet = 1;
						printf ( "Error in DeviceIoControl : %d", GetLastError());
						__leave;
					}
				}
			} // if sm_init
			else if(StartCmd.m_TestParams.m_eCommandType == sm_uninit)
			{
				if(StartCmd.m_TestParams.m_bTransmit == TRUE)
				{
					bRc = DeviceIoControl(
								hTcpClientHandle, 
								SFTK_IOCTL_UNINIT_PMD,
								NULL,
								0,
								NULL, 
								0,
								&bytesReturned,
								NULL 
								);

					if ( !bRc )
					{
						dwRet = 1;
						printf ( "Error in DeviceIoControl : %d", GetLastError());
						__leave;
					}
				}
				else
				{
					bRc = DeviceIoControl(
								hTcpClientHandle, 
								SFTK_IOCTL_UNINIT_RMD,
								NULL,
								0,
								NULL, 
								0,
								&bytesReturned,
								NULL 
								);

					if ( !bRc )
					{
						dwRet = 1;
						printf ( "Error in DeviceIoControl : %d", GetLastError());
						__leave;
					}
				}
			} // if sm_uninit
			__leave;
		} // if sm_init | sm_uninit | sm_start | sm_stop
		else if( 
				(	StartCmd.m_TestParams.m_eCommandType == sm_add		) || 
				(	StartCmd.m_TestParams.m_eCommandType == sm_remove	) ||
				(	StartCmd.m_TestParams.m_eCommandType == sm_enable	) ||
				(	StartCmd.m_TestParams.m_eCommandType == sm_disable	) 
			)
		{
			// Else deal with ( sm_add | sm_remove | sm_enable | sm_disable ) commands

			size = sizeof(StartCmd.m_TestParams.m_ConnDetails);
			bRc = DeviceIoControl(
						hTcpClientHandle, 
						StartTestIoctl,
						(LPVOID)&StartCmd.m_TestParams.m_ConnDetails,
						size,
						NULL, 
						0,
						&bytesReturned,
						NULL 
						);

			if ( !bRc )
			{
				dwRet = 1;
				printf ( "Error in DeviceIoControl : %d", GetLastError());
				__leave;
			}
		} // else if( sm_add | sm_remove | sm_enable | sm_disable ) commands
		else if(
				(	StartCmd.m_TestParams.m_eCommandType == sm_sendnormal	) || 
				(	StartCmd.m_TestParams.m_eCommandType == sm_sendrefresh	)
			)
		{
			// else if( sm_sendnormal | sm_sendrefresh)

			size = 0;
			bRc = DeviceIoControl(
						hTcpClientHandle, 
						StartTestIoctl,
						NULL,
						0,
						NULL, 
						0,
						&bytesReturned,
						NULL 
						);

			if ( !bRc )
			{
				dwRet = 1;
				printf ( "Error in DeviceIoControl : %d", GetLastError());
				__leave;
			}
			__leave;
		} // else if( sm_sendnormal | sm_sendrefresh)
		else
		{
			// These are the rest of the commands

		} //else
   }
   __finally
   {
	   if(hTcpClientHandle != INVALID_HANDLE_VALUE)
	   {
			//
			// Close Handle And Exit
			//
			CloseHandle( hTcpClientHandle );
			hTcpClientHandle = INVALID_HANDLE_VALUE;
			printf( "Closed %s Handle\n", szDeviceName );
	   }
   }
	return 0;
}

