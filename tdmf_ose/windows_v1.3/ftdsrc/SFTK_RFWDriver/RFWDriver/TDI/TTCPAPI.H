#ifndef __TTCPAPI_H__
#define __TTCPAPI_H__

/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

//
// The following preprocessor directive includes the <WinIoctl.h> header
// when compiling a Win32 application or DLL. The driver includes DevIoctl.h
// through a different mechanism.
//
#ifdef _WINDOWS
#include <WinIoctl.h>   // Compiling Win32 Applications Or DLL's
#endif // _WINDOWS

// Copyright And Configuration Management ----------------------------------
//
//                  TDITTCP Driver IOCTL Codes - TTCPAPI.H
//
//                      Win32 NDIS Framework (WinDis 32)
//                                    For
//                         Windows NT And Windows 95
//
//     Copyright (c) 1999-2000 Printing Communications Associates, Inc.
//                                - PCAUSA -
//                          http://www.pcausa.com
//
//                             Thomas F. Divine
//                           4201 Brunswick Court
//                        Smyrna, Georgia 30080 USA
//                              (770) 432-4580
//                         mailto:tdivine@pcausa.com
// 
// End ---------------------------------------------------------------------

//
// TDI TTCP API Version Information
// --------------------------------
// Make sure that this is coordinated with information in the VERSION
// resource.
//
#define TDITTCP_API_VERSION     0x02000310


//
// Header For IOCTL's
// ------------------
// This header references the CTL_CODE macro and the METHOD_XYZ definitions.
// These are defined in the VC/SDK WinIoctl.h header for applications and
// in the DDK DevIoctl.h for drivers. Unfortunately, including WinIoctl.h
// when compiling a NT device drivers causes problems.
//

//
// TDITTCP Device Types (For IOCTL)
//
#define FILE_DEVICE_TDITTCP_BASE 0x00008000  // First User Device Type


//
// TDITTCP Driver Device Name Strings
//
#define TDITTCP_BASE_NAME_A        "TDITTCP"
#define TDITTCP_BASE_NAME_W        L"TDITTCP"
#define TDITTCP_DISPLAY_NAME_A     "TDI Test TCP (TTCP) Driver"
#define TDITTCP_DISPLAY_NAME_W     L"TDI Test TCP (TTCP) Driver"

#ifdef UNICODE

#define TDITTCP_BASE_NAME       TDITTCP_BASE_NAME_W
#define TDITTCP_DISPLAY_NAME    TDITTCP_DISPLAY_NAME_W

#else

#define TDITTCP_BASE_NAME       TDITTCP_BASE_NAME_A
#define TDITTCP_DISPLAY_NAME    TDITTCP_DISPLAY_NAME_A

#endif


/////////////////////////////////////////////////////////////////////////////
//                 TDI TTCP TCP Server API Section                         //
/////////////////////////////////////////////////////////////////////////////

//
// TDI TCP Server Device Name Strings
//
#define TDI_TCP_SERVER_BASE_NAME_A        "TdiTcpServer"
#define TDI_TCP_SERVER_BASE_NAME_W        L"TdiTcpServer"
#define TDI_TCP_SERVER_DISPLAY_NAME_A     "PCAUSA TDI TTCP TCP Server"
#define TDI_TCP_SERVER_DISPLAY_NAME_W     L"PCAUSA TDI TTCP TCP Server"
#define TDI_TCP_SERVER_DEVICE_NAME_A      "\\Device\\TdiTcpServer"
#define TDI_TCP_SERVER_DEVICE_NAME_W      L"\\Device\\TdiTcpServer"

#ifdef UNICODE

#define TDI_TCP_SERVER_BASE_NAME       TDI_TCP_SERVER_BASE_NAME_W
#define TDI_TCP_SERVER_DISPLAY_NAME    TDI_TCP_SERVER_DISPLAY_NAME_W
#define TDI_TCP_SERVER_DEVICE_NAME     TDI_TCP_SERVER_DEVICE_NAME_W

#else

#define TDI_TCP_SERVER_BASE_NAME       TDI_TCP_SERVER_BASE_NAME_A
#define TDI_TCP_SERVER_DISPLAY_NAME    TDI_TCP_SERVER_DISPLAY_NAME_A
#define TDI_TCP_SERVER_DEVICE_NAME     TDI_TCP_SERVER_DEVICE_NAME_A

#endif

/////////////////////////////////////////////////////////////////////////////
//                       TDI TTCP TEST PARAMETERS                          //
/////////////////////////////////////////////////////////////////////////////

//
// Definitions For Send Test Mode
// ------------------------------
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
typedef
enum _TTCP_SEND_MODE
{
   TTCP_SEND_SYNCHRONOUS,
   TTCP_SEND_NEXT_FROM_COMPLETION
}
   TTCP_SEND_MODE;

// Notes
// -----
// This single structure is used to initiate all TDI TTCP tests as well as
// to query current and final test results.
//
// In addition, the m_nTestNumber reference number can be used to perform
// some other operations, such as cancelling the test.
//
#pragma pack(push,1)      // x86, MS compiler; MIPS, MIPS compiler

typedef
struct _TDITTCP_TEST_PARAMS
{
   ULONG    m_nTestNumber;    // Assigned by TDITTCP driver
   //
   // Basic Test Specifier
   // --------------------
   //   TRUE  -> Transmit Test. Start TDITCP TCP/UDP Client Test
   //   FALSE -> Receive Test. Start TDITCP TCP/UDP Server Test
   //
   BOOLEAN  m_bTransmit;

   //
   // Protocol Specifier
   // ------------------
   //   IPPROT_TCP -> TCP Test
   //   IPPROT_UDP -> UDP Test
   //
   USHORT   m_Protocol;

   //
   // Remote Internet Address
   //
   IN_ADDR  m_RemoteAddress;  // Network Byte Order

   USHORT   m_Port;           // Network Byte Order

   //
   // Test Implementation Options
   // ---------------------------
   // These are options, such as buffer sizes, bytes to be sent, etc.,
   // that are controled by the implementation.
   //
   TTCP_SEND_MODE  m_SendMode;

   //
   // Setup Size Of Send/Receive Buffer
   //
   ULONG    m_nBufferSize;

   //
   // Pattern Generator Parameters (Transmit Options)
   // -----------------------------------------------
   // The default mode for sending is to use a pattern generator to fill
   // the send buffer. This is done once during initialization.
   //
   // m_nNumBuffersToSend specifies the number of pattern buffers to be sent.
   // The size of each buffer sent is specified by m_nBufferSize.
   //
   BOOLEAN  m_bUsePatternGenerator;    // A Transmit Option
   BOOLEAN  m_bSendContinuously;       // A Transmit Option
   ULONG    m_nNumBuffersToSend;       // A Transmit Option

   //
   // Test TCP Options
   // ----------------
   ULONG    m_nNoDelay;                // A Transmit TCP Option

   //
   // Test TDI Options
   // ----------------
}
   TDITTCP_TEST_PARAMS, *PTDITTCP_TEST_PARAMS;

typedef
struct _TTCP_TEST_START_CMD
{
   ULONG                m_Status;
   TDITTCP_TEST_PARAMS  m_TestParams;
}
   TTCP_TEST_START_CMD, *PTTCP_TEST_START_CMD;

#pragma pack(pop)                  // x86, MS compiler; MIPS, MIPS compiler


/////////////////////////////////////////////////////////////////////////////
//// Function Codes For TDI TTCP TCP Server IOCTL
//

#define FILE_DEVICE_TCP_SERVER   (FILE_DEVICE_TDITTCP_BASE)
#define IOCTL_TCP_SERVER_BASE    FILE_DEVICE_TCP_SERVER

#define IOCTL_TCP_SERVER_START_TEST\
   CTL_CODE(IOCTL_TCP_SERVER_BASE, 2048, METHOD_BUFFERED, FILE_ANY_ACCESS)


/////////////////////////////////////////////////////////////////////////////
//                 TDI TTCP UDP Server API Section                         //
/////////////////////////////////////////////////////////////////////////////

//
// TDI UDP Server Device Name Strings
//
#define TDI_UDP_SERVER_BASE_NAME_A        "TdiUdpServer"
#define TDI_UDP_SERVER_BASE_NAME_W        L"TdiUdpServer"
#define TDI_UDP_SERVER_DISPLAY_NAME_A     "PCAUSA TDI TTCP UDP Server"
#define TDI_UDP_SERVER_DISPLAY_NAME_W     L"PCAUSA TDI TTCP UDP Server"
#define TDI_UDP_SERVER_DEVICE_NAME_A      "\\Device\\TdiUdpServer"
#define TDI_UDP_SERVER_DEVICE_NAME_W      L"\\Device\\TdiUdpServer"

#ifdef UNICODE

#define TDI_UDP_SERVER_BASE_NAME       TDI_UDP_SERVER_BASE_NAME_W
#define TDI_UDP_SERVER_DISPLAY_NAME    TDI_UDP_SERVER_DISPLAY_NAME_W
#define TDI_UDP_SERVER_DEVICE_NAME     TDI_UDP_SERVER_DEVICE_NAME_W

#else

#define TDI_UDP_SERVER_BASE_NAME       TDI_UDP_SERVER_BASE_NAME_A
#define TDI_UDP_SERVER_DISPLAY_NAME    TDI_UDP_SERVER_DISPLAY_NAME_A
#define TDI_UDP_SERVER_DEVICE_NAME     TDI_UDP_SERVER_DEVICE_NAME_A

#endif

#define FILE_DEVICE_UDP_SERVER   (FILE_DEVICE_TDITTCP_BASE+1)
#define IOCTL_UDP_SERVER_BASE    FILE_DEVICE_UDP_SERVER

#define IOCTL_UDP_SERVER_START_TEST\
   CTL_CODE(IOCTL_UDP_SERVER_BASE, 2048, METHOD_BUFFERED, FILE_ANY_ACCESS)


/////////////////////////////////////////////////////////////////////////////
//                   TDI TTCP TCP Client API Section                       //
/////////////////////////////////////////////////////////////////////////////

//
// TDI TCP Client Device Name Strings
//
#define TDI_TCP_CLIENT_BASE_NAME_A        "TdiTcpClient"
#define TDI_TCP_CLIENT_BASE_NAME_W        L"TdiTcpClient"
#define TDI_TCP_CLIENT_DISPLAY_NAME_A     "PCAUSA TDI TTCP TCP Client"
#define TDI_TCP_CLIENT_DISPLAY_NAME_W     L"PCAUSA TDI TTCP TCP Client"
#define TDI_TCP_CLIENT_DEVICE_NAME_A      "\\Device\\TdiTcpClient"
#define TDI_TCP_CLIENT_DEVICE_NAME_W      L"\\Device\\TdiTcpClient"

#ifdef UNICODE

#define TDI_TCP_CLIENT_BASE_NAME       TDI_TCP_CLIENT_BASE_NAME_W
#define TDI_TCP_CLIENT_DISPLAY_NAME    TDI_TCP_CLIENT_DISPLAY_NAME_W
#define TDI_TCP_CLIENT_DEVICE_NAME     TDI_TCP_CLIENT_DEVICE_NAME_W

#else

#define TDI_TCP_CLIENT_BASE_NAME       TDI_TCP_CLIENT_BASE_NAME_A
#define TDI_TCP_CLIENT_DISPLAY_NAME    TDI_TCP_CLIENT_DISPLAY_NAME_A
#define TDI_TCP_CLIENT_DEVICE_NAME     TDI_TCP_CLIENT_DEVICE_NAME_A

#endif

#define FILE_DEVICE_TCP_CLIENT   (FILE_DEVICE_TDITTCP_BASE+2)
#define IOCTL_TCP_CLIENT_BASE    FILE_DEVICE_TCP_CLIENT

#define IOCTL_TCP_CLIENT_START_TEST\
   CTL_CODE(IOCTL_TCP_CLIENT_BASE, 2048, METHOD_BUFFERED, FILE_ANY_ACCESS)


/////////////////////////////////////////////////////////////////////////////
//                   TDI TTCP UDP Client API Section                       //
/////////////////////////////////////////////////////////////////////////////

//
// TDI UDP Client Device Name Strings
//
#define TDI_UDP_CLIENT_BASE_NAME_A        "TdiUdpClient"
#define TDI_UDP_CLIENT_BASE_NAME_W        L"TdiUdpClient"
#define TDI_UDP_CLIENT_DISPLAY_NAME_A     "PCAUSA TDI TTCP UDP Client"
#define TDI_UDP_CLIENT_DISPLAY_NAME_W     L"PCAUSA TDI TTCP UDP Client"
#define TDI_UDP_CLIENT_DEVICE_NAME_A      "\\Device\\TdiUdpClient"
#define TDI_UDP_CLIENT_DEVICE_NAME_W      L"\\Device\\TdiUdpClient"

#ifdef UNICODE

#define TDI_UDP_CLIENT_BASE_NAME       TDI_UDP_CLIENT_BASE_NAME_W
#define TDI_UDP_CLIENT_DISPLAY_NAME    TDI_UDP_CLIENT_DISPLAY_NAME_W
#define TDI_UDP_CLIENT_DEVICE_NAME     TDI_UDP_CLIENT_DEVICE_NAME_W

#else

#define TDI_UDP_CLIENT_BASE_NAME       TDI_UDP_CLIENT_BASE_NAME_A
#define TDI_UDP_CLIENT_DISPLAY_NAME    TDI_UDP_CLIENT_DISPLAY_NAME_A
#define TDI_UDP_CLIENT_DEVICE_NAME     TDI_UDP_CLIENT_DEVICE_NAME_A

#endif

#define FILE_DEVICE_UDP_CLIENT   (FILE_DEVICE_TDITTCP_BASE+3)
#define IOCTL_UDP_CLIENT_BASE    FILE_DEVICE_UDP_CLIENT

#define IOCTL_UDP_CLIENT_START_TEST\
   CTL_CODE(IOCTL_UDP_CLIENT_BASE, 2048, METHOD_BUFFERED, FILE_ANY_ACCESS)

#ifdef __TTCPCTRL__

typedef UNALIGNED struct _TA_ADDRESS {
    USHORT AddressLength;       // length in bytes of Address[] in this
    USHORT AddressType;         // type of this address
    UCHAR Address[1];           // actually AddressLength bytes long
} TA_ADDRESS, *PTA_ADDRESS;

typedef struct _TRANSPORT_ADDRESS {
    LONG TAAddressCount;            // number of addresses following
    TA_ADDRESS Address[1];          // actually TAAddressCount elements long
} TRANSPORT_ADDRESS, *PTRANSPORT_ADDRESS;


typedef struct _TDI_ADDRESS_IP {
    USHORT sin_port;
    ULONG  in_addr;
    UCHAR  sin_zero[8];
} TDI_ADDRESS_IP, *PTDI_ADDRESS_IP;


#define TDI_ADDRESS_LENGTH_IP sizeof (TDI_ADDRESS_IP)

#endif //__TTCPCTRL__

#endif // __TTCPAPI_H__

