/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES


#include <stdio.h>
#include <io.h>

#include <winsock2.h>
#include <winioctl.h>


#include "getopt.h"
#pragma pack(push, 2)
#include "TTCPApi.h"
#include "tdiioctl.h"
#pragma pack(pop)

// Copyright And Configuration Management ----------------------------------
//
//                     TDI Client Samples For Windows NT
//
//                        MAIN Entry Point - TestApp.c
//
//      Copyright (c) 1999-2001 Printing Communications Associates, Inc.
//                               - PCAUSA -
//
//                             Thomas F. Divine
//                           4201 Brunswick Court
//                        Smyrna, Georgia 30080 USA
//                              (770) 432-4580
//                            tdivine@pcausa.com
// 
// End ---------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
//// GLOBAL VARIABLES
//

HANDLE   g_hTcpClientHandle = INVALID_HANDLE_VALUE;
WSADATA  g_WsaData;

#ifndef IPPORT_TTCP
#define IPPORT_TTCP          5001
#endif

char *g_RemoteHostName = NULL;      // ptr to name of remote host
struct hostent *addr;


VOID IntitializeTestDefaults( PTTCP_TEST_START_CMD pStartCmd )
{
   memset( pStartCmd, 0x00, sizeof( TTCP_TEST_START_CMD ) );

   pStartCmd->m_TestParams.m_bTransmit = FALSE;
   pStartCmd->m_TestParams.m_Protocol = IPPROTO_TCP;

   pStartCmd->m_TestParams.m_Port = htons( IPPORT_TTCP );

   //
   // Setup Size Of Send/Receive Buffer
   //
   pStartCmd->m_TestParams.m_nBufferSize = 8 * 1024;

   //
   // Setup Pattern Generator Default
   // -------------------------------
   // The default mode for sending is to use a pattern generator to fill
   // the send buffer. This is done once during initialization.
   //
   // m_nNumBuffersToSend specifies the number of pattern buffers to be sent.
   // The size of each buffer sent is specified by m_nBufferSize.
   //
   // The alternative to using the pattern generator is to specify a filename
   // that the TDITTCP driver will open and read from in kernel mode. This
   // capability is not yet iplemented.
   //
   pStartCmd->m_TestParams.m_bUsePatternGenerator = TRUE;
   pStartCmd->m_TestParams.m_bSendContinuously = FALSE;
   pStartCmd->m_TestParams.m_nNumBuffersToSend = 2 * 1024; // number of buffers to send

   //
   // Setup Send Test Mode
   // --------------------
   // Some of the tests include code that illustrate more than one way to send
   // on a transport address or connection endpoint. The method to be used
   // for sending is specified by TTCP_SEND_MODE:
   //
   //   TTCP_SEND_SYNCHRONOUS - This is the simplest, but least flexible,
   //     approach to sending. The sending test thread calls a simple support
   //     routine that does not return until the data has been sent and
   //     acknowledged.
   //
   //   TTCP_SEND_NEXT_FROM_COMPLETION - This method is more flexible. The
   //     sending implementation uses a callback routine that is called
   //     when the previous data has been sent and acknowledged. Sending
   //     of the first buffer is initiated from the test thread, which then
   //     waits on an event that will be signaled when the last buffer has
   //     been sent.
   //
   //     Sending of each subsequent buffer is initiated from within the
   //     callback routine. The callback sets the event that unblocks the
   //     initiating thread when the buffer has been sent and acknowledged.
   //
   pStartCmd->m_TestParams.m_SendMode = TTCP_SEND_NEXT_FROM_COMPLETION;
//   pStartCmd->m_TestParams.m_SendMode = TTCP_SEND_SYNCHRONOUS;

   pStartCmd->m_TestParams.m_nNoDelay = 0;
}

VOID TDITTCP_DumpTestParams( PTDITTCP_TEST_PARAMS pTestParams )
{
   if( pTestParams->m_bTransmit )
   {
      printf( "TTCP Transmit Test\n" );

      if( pTestParams->m_nTestNumber )
      {
         printf( "  Test Number       : \n", pTestParams->m_nTestNumber );
      }

      if( pTestParams->m_Protocol == IPPROTO_TCP )
      {
         printf( "  Protocol       : TCP\n" );
      }
      else if( pTestParams->m_Protocol == IPPROTO_UDP )
      {
         printf( "  Protocol       : UDP\n" );
      }
      else
      {
         printf( "  Protocol       : Invalid\n" );
         exit( 0 );
      }

      printf( "  Port           : %d\n",
         ntohs( pTestParams->m_Port )
         );

      //
      // Setup Remote IP Addresses For Test
      //
      if( atoi( g_RemoteHostName ) <= 0 )
      {
         printf( "  Remote Host    : \042%s\042\n", g_RemoteHostName );
      }

      printf( "  Remote Address : %s\n", inet_ntoa( pTestParams->m_RemoteAddress ) );

      printf( "  Buffer Size    : %d\n", pTestParams->m_nBufferSize );

      if( pTestParams->m_bUsePatternGenerator )
      {
         if( pTestParams->m_bSendContinuously )
         {
            printf( "  Buffers To Send: Continuous\n" );
         }
         else
         {
            printf( "  Buffers To Send: %d\n", pTestParams->m_nNumBuffersToSend );
         }
      }
      else
      {
         //
         // Display File For Input
         //
      }

      if( pTestParams->m_Protocol == IPPROTO_TCP )
      {
         printf( "  NO_DELAY       : %d\n", pTestParams->m_nNoDelay );
      }
   }
   else
   {
      printf( "TTCP Receive Test\n" );

      if( pTestParams->m_nTestNumber )
      {
         printf( "  Test Number       : \n", pTestParams->m_nTestNumber );
      }

      if( pTestParams->m_Protocol == IPPROTO_TCP )
      {
         printf( "  Protocol       : TCP\n" );
      }
      else if( pTestParams->m_Protocol == IPPROTO_UDP )
      {
         printf( "  Protocol       : UDP\n" );
      }
      else
      {
         printf( "  Protocol       : Invalid\n" );
         exit( 0 );
      }

      printf( "  Port           : %d\n",
         ntohs( pTestParams->m_Port )
         );

      printf( "  Buffer Size    : %d\n", pTestParams->m_nBufferSize );

      if( !pTestParams->m_bUsePatternGenerator )
      {
         //
         // Display File For Output
         //
      }
   }
}

//
// Usage Message
//
char Usage[] = "\
Usage: ttcpctrl -t [-options] host\n\
       ttcpctrl -r [-options]\n\
Common options:\n\
	-l ##	length of bufs read from or written to network (default 8192)\n\
	-u	use UDP instead of TCP\n\
	-p ##	port number to send to or listen at (default 5001)\n\
	-s	-t: source a pattern to network\n\
		-r: sink (discard) all data from network\n\
Options specific to -t:\n\
	-n ##	number of source bufs written to network (default 2048)\n\
	-D	don't buffer TCP writes (sets TCP_NODELAY TCP option)\n\
Options specific to -r:\n\
";


void DUMP_ConnectionInfo(PCONNECTION_DETAILS lpConnectionDetails)
{
	PCONNECTION_INFO pConnectionInfo = NULL;
	int i =0;

	printf("The SendWindow = %d , ReceiveWindow = %d , Number of Connections = %d\n",
		lpConnectionDetails->nSendWindowSize, lpConnectionDetails->nReceiveWindowSize,
		lpConnectionDetails->nConnections);

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

int main( int argc, char **argv )
{
   HANDLE   hAdapterDevice = INVALID_HANDLE_VALUE;
   char     szDeviceName[ _MAX_PATH ];
   BOOLEAN  bRc;
   DWORD    bytesReturned;
   TTCP_TEST_START_CMD  StartCmd;
   int      c;
   DWORD    StartTestIoctl;
   PCONNECTION_DETAILS lpConnectionDetails = NULL;
   PCONNECTION_INFO lpConnectionInfo = NULL;
   DWORD size = 0;

   printf( "PCAUSA TDI TTCP Test Control Application\n" );

   IntitializeTestDefaults( &StartCmd );

	if (argc < 2)
   {
      fprintf(stderr,Usage);
      exit( 0 );
   }

   while (optind != argc)
   {
	   c = getopt(argc, argv, "cdrstuvBDTb:f:l:n:p:A:O:" );

      switch (c)
      {
         case 't':
            StartCmd.m_TestParams.m_bTransmit = TRUE;
            break;

         case 'r':
            StartCmd.m_TestParams.m_bTransmit = FALSE;
            break;

         case 'u':
            StartCmd.m_TestParams.m_Protocol = IPPROTO_UDP;
            break;

         case 'p':
            StartCmd.m_TestParams.m_Port = htons( (USHORT )atoi(optarg) );
            break;

         case 's':
            StartCmd.m_TestParams.m_bUsePatternGenerator =
               !StartCmd.m_TestParams.m_bUsePatternGenerator;
            break;

         case 'c':
            StartCmd.m_TestParams.m_bSendContinuously =
               !StartCmd.m_TestParams.m_bSendContinuously;
            break;

         case 'D':
            StartCmd.m_TestParams.m_nNoDelay = !0;
            break;

         case 'n':
            StartCmd.m_TestParams.m_nNumBuffersToSend = atoi(optarg);
            break;

         case 'l':
            StartCmd.m_TestParams.m_nBufferSize = atoi(optarg);
            break;

         case EOF:
            optarg = argv[optind];
            optind++;
            break;

		   default:
            fprintf(stderr,Usage);
            exit( 0 );
      }
   }

   if( WSAStartup( MAKEWORD(0x02,0x00), &g_WsaData ) == SOCKET_ERROR )
   {
      fprintf(stderr,Usage);
      fprintf(stderr,"WSAStartup Failed\n" );
      exit(0);
   }



//	size = sizeof(CONNECTION_DETAILS)+sizeof(CONNECTION_INFO)*4;
	size = sizeof(CONNECTION_DETAILS);
	lpConnectionDetails = (PCONNECTION_DETAILS)malloc(size);
	memset((void*)lpConnectionDetails,0,size);
//	lpConnectionDetails->nConnections = 5;
	lpConnectionDetails->nConnections = 1;
//	lpConnectionDetails->nReceiveWindowSize = 262144;	//This one MB
//	lpConnectionDetails->nSendWindowSize = 262144;	//This is one MB

	lpConnectionDetails->nReceiveWindowSize = 1024*1700;	//This one MB
	lpConnectionDetails->nSendWindowSize = 1024*1700;	//This is one MB


   //
   // Setup For Test
   //
   if( StartCmd.m_TestParams.m_bTransmit )
   {
      //
      // Specify Correct TDITTCP Device
      //
      if( StartCmd.m_TestParams.m_Protocol == IPPROTO_TCP )
      {
         //
         // TDI TTCP TCP Client Device
         //
         sprintf( szDeviceName, "%s%s", "\\\\.\\",
            SFTK_TDI_TCP_CLIENT_BASE_NAME
            );

         StartTestIoctl = (DWORD)SFTK_IOCTL_TCP_CLIENT_START_CONNECTIONS;


		 lpConnectionInfo = lpConnectionDetails->ConnectionDetails;

//		 lpConnectionInfo->ipLocalAddress.in_addr = inet_addr("129.212.66.15");
//		 lpConnectionInfo->ipLocalAddress.sin_port = 0;

//		 lpConnectionInfo->ipRemoteAddress.in_addr = inet_addr("129.212.66.1");
//		 lpConnectionInfo->ipRemoteAddress.sin_port = htons(5002);
//		 lpConnectionInfo->nNumberOfSessions = 1;

//		 lpConnectionInfo+=1;

		 lpConnectionInfo->ipLocalAddress.in_addr = inet_addr("10.0.0.16");
		 lpConnectionInfo->ipLocalAddress.sin_port = 0;

		 lpConnectionInfo->ipRemoteAddress.in_addr = inet_addr("10.0.0.15");
		 lpConnectionInfo->ipRemoteAddress.sin_port = htons(5003);
		 lpConnectionInfo->nNumberOfSessions = 1;

//		 lpConnectionInfo+=1;

//		 lpConnectionInfo->ipLocalAddress.in_addr = inet_addr("10.0.0.15");
//		 lpConnectionInfo->ipLocalAddress.sin_port = 0;

//		 lpConnectionInfo->ipRemoteAddress.in_addr = inet_addr("10.0.0.16");
//		 lpConnectionInfo->ipRemoteAddress.sin_port = htons(5003);
//		 lpConnectionInfo->nNumberOfSessions = 1;


//		 lpConnectionInfo+=1;

//		 lpConnectionInfo->ipLocalAddress.in_addr = inet_addr("10.0.0.15");
//		 lpConnectionInfo->ipLocalAddress.sin_port = 0;

//		 lpConnectionInfo->ipRemoteAddress.in_addr = inet_addr("10.0.0.16");
//		 lpConnectionInfo->ipRemoteAddress.sin_port = htons(5004);
//		 lpConnectionInfo->nNumberOfSessions = 1;

//		 lpConnectionInfo+=1;

//		 lpConnectionInfo->ipLocalAddress.in_addr = inet_addr("10.0.0.15");
//		 lpConnectionInfo->ipLocalAddress.sin_port = 0;

//		 lpConnectionInfo->ipRemoteAddress.in_addr = inet_addr("10.0.0.16");
//		 lpConnectionInfo->ipRemoteAddress.sin_port = htons(5005);
//		 lpConnectionInfo->nNumberOfSessions = 1;


	  }
      else
      {
         //
         // TDI TTCP UDP Client Device
         //
         sprintf( szDeviceName, "%s%s", "\\\\.\\",
            TDI_UDP_CLIENT_BASE_NAME
            );

         StartTestIoctl = (DWORD)IOCTL_UDP_CLIENT_START_TEST;
      }

      //
      // Setup Remote IP Addresses For Test
      //
      g_RemoteHostName = argv[argc - 1];

      if( atoi( g_RemoteHostName ) > 0 )
      {
         // Numeric
         StartCmd.m_TestParams.m_RemoteAddress.s_addr = inet_addr( g_RemoteHostName );
      }
      else
      {
         if ((addr = gethostbyname( g_RemoteHostName )) == NULL)
         {
            fprintf( stderr, "Bad Remote Host Name\n" );
            exit( 0 );
         }

         memcpy(
            (char*)&StartCmd.m_TestParams.m_RemoteAddress.s_addr,
            addr->h_addr,
            sizeof( ULONG )
            );
      }
   }
   else
   {
      //
      // Specify Correct TDITTCP Device
      //
      if( StartCmd.m_TestParams.m_Protocol == IPPROTO_TCP )
      {
         //
         // TDI TTCP TCP Server Device
         //
         sprintf( szDeviceName, "%s%s", "\\\\.\\",
            SFTK_TDI_TCP_SERVER_BASE_NAME
            );

         StartTestIoctl = (DWORD)SFTK_IOCTL_TCP_SERVER_START;



		 lpConnectionInfo = lpConnectionDetails->ConnectionDetails;

//		 lpConnectionInfo->ipLocalAddress.in_addr = inet_addr("129.212.66.1");
//		 lpConnectionInfo->ipLocalAddress.sin_port = htons(5002);

//		 lpConnectionInfo->nNumberOfSessions = 1;

//		 lpConnectionInfo+=1;

		 lpConnectionInfo->ipLocalAddress.in_addr = inet_addr("10.0.0.15");
		 lpConnectionInfo->ipLocalAddress.sin_port = htons(5003);

		 lpConnectionInfo->nNumberOfSessions = 1;

//		 lpConnectionInfo+=1;

//		 lpConnectionInfo->ipLocalAddress.in_addr = inet_addr("10.0.0.16");
//		 lpConnectionInfo->ipLocalAddress.sin_port = htons(5003);

//		 lpConnectionInfo->nNumberOfSessions = 1;

//		 lpConnectionInfo+=1;

//		 lpConnectionInfo->ipLocalAddress.in_addr = inet_addr("10.0.0.16");
//		 lpConnectionInfo->ipLocalAddress.sin_port = htons(5004);

//		 lpConnectionInfo->nNumberOfSessions = 1;

//		 lpConnectionInfo+=1;

//		 lpConnectionInfo->ipLocalAddress.in_addr = inet_addr("10.0.0.16");
//		 lpConnectionInfo->ipLocalAddress.sin_port = htons(5005);

//		 lpConnectionInfo->nNumberOfSessions = 1;


      }
      else
      {
         //
         // TDI TTCP UDP Server Device
         //
         sprintf( szDeviceName, "%s%s", "\\\\.\\",
            TDI_UDP_SERVER_BASE_NAME
            );

         StartTestIoctl = (DWORD)IOCTL_UDP_SERVER_START_TEST;
      }

   }

   printf("ThE size of Buffer is %d\n",size);
   //
   // Display Selected Test Parameters
   //
   TDITTCP_DumpTestParams( &StartCmd.m_TestParams );

   DUMP_ConnectionInfo(lpConnectionDetails);

   //
   // Open The TDITTCP Device
   //
   g_hTcpClientHandle = CreateFile(
                           szDeviceName,
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_EXISTING,
                           FILE_FLAG_NO_BUFFERING | FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                           NULL
                           );

   if( g_hTcpClientHandle == INVALID_HANDLE_VALUE )
   {
      printf( "Unable To Open %s Handle\n", szDeviceName );
	  goto Last;
   }

   printf( "Opened %s Handle\n", szDeviceName );

   

   //
   // Start The TDI TTCP Test
   //
   bRc = DeviceIoControl(
            g_hTcpClientHandle, 
            StartTestIoctl,
            (LPVOID)lpConnectionDetails,
            size,
            (LPVOID)lpConnectionDetails, 
            size,
            &bytesReturned,
            NULL 
            );

	if ( !bRc )
	{
      printf ( "Error in DeviceIoControl : %d", GetLastError());
   }
   else
   {
      printf( "\nTDI TTCP Test Started\n" );
   }

   //
   // Close Handle And Exit
   //
   CloseHandle( g_hTcpClientHandle );
   g_hTcpClientHandle = INVALID_HANDLE_VALUE;

   printf( "Closed %s Handle\n", szDeviceName );

Last:
   free((void*)lpConnectionDetails);

   return( 0 );
}
