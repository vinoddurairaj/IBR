#ifndef __TDIIOCTL_H__
#define __TDIIOCTL_H__

/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

//
// The following preprocessor directive includes the <WinIoctl.h> header
// when compiling a Win32 application or DLL. The driver includes DevIoctl.h
// through a different mechanism.
//
//#ifdef _WINDOWS
//#include <WinIoctl.h>   // Compiling Win32 Applications Or DLL's
//#endif // _WINDOWS




//////////////////////////////////////
//Added From Here Veera
/////////////////////////////////////

#define SFTK_FILE_DEVICE_TDITTCP_BASE 0x00008004  // First User Device Type


/////////////////////////////////////////////////////////////////////////////
//                 SFTK TDI TTCP TCP Server API Section                         //
/////////////////////////////////////////////////////////////////////////////

//This Server device will be created in the Driver
//The Symbolic Link to \Device\SftkTdiTcpServer will be created in the \DosDevices\ So that
//this device can be accessed from the User Space.
//
// TDI TCP Server Device Name Strings
//
#define TDI_TCP_SERVER_BASE_NAME_A        "SftkTdiTcpServer"
#define TDI_TCP_SERVER_BASE_NAME_W        L"SftkTdiTcpServer"
#define TDI_TCP_SERVER_DISPLAY_NAME_A     "Sftk TDI TTCP TCP Server"
#define TDI_TCP_SERVER_DISPLAY_NAME_W     L"Sftk TDI TTCP TCP Server"
#define TDI_TCP_SERVER_DEVICE_NAME_A      "\\Device\\SftkTdiTcpServer"
#define TDI_TCP_SERVER_DEVICE_NAME_W      L"\\Device\\SftkTdiTcpServer"

#ifdef UNICODE

#define TDI_TCP_SERVER_BASE_NAME       STFK_TDI_TCP_SERVER_BASE_NAME_W
#define TDI_TCP_SERVER_DISPLAY_NAME    TDI_TCP_SERVER_DISPLAY_NAME_W
#define TDI_TCP_SERVER_DEVICE_NAME     TDI_TCP_SERVER_DEVICE_NAME_W

#else

#define TDI_TCP_SERVER_BASE_NAME       TDI_TCP_SERVER_BASE_NAME_A
#define TDI_TCP_SERVER_DISPLAY_NAME    TDI_TCP_SERVER_DISPLAY_NAME_A
#define TDI_TCP_SERVER_DEVICE_NAME     TDI_TCP_SERVER_DEVICE_NAME_A

#endif

/////////////////////////////////////////////////////////////////////////////
//// Function Codes For TDI TTCP TCP Server IOCTL
//

#define SFTK_FILE_DEVICE_TCP_SERVER   (SFTK_FILE_DEVICE_TDITTCP_BASE)
#define SFTK_IOCTL_TCP_SERVER_BASE    SFTK_FILE_DEVICE_TCP_SERVER

//Starts the Server. Creates Threads to Listen and Accept.
#define SFTK_IOCTL_TCP_SERVER_START\
   CTL_CODE(SFTK_IOCTL_TCP_SERVER_BASE, 2048, METHOD_BUFFERED, FILE_ANY_ACCESS)

//Closes all Connections and Exists all Threads.
#define SFTK_IOCTL_TCP_SERVER_STOP\
   CTL_CODE(SFTK_IOCTL_TCP_SERVER_BASE, 2049, METHOD_BUFFERED, FILE_ANY_ACCESS)


/////////////////////////////////////////////////////////////////////////////
//                   TDI TTCP TCP Client API Section                       //
/////////////////////////////////////////////////////////////////////////////

//
// TDI TCP Client Device Name Strings
//
#define TDI_TCP_CLIENT_BASE_NAME_A        "SftkTdiTcpClient"
#define TDI_TCP_CLIENT_BASE_NAME_W        L"SftkTdiTcpClient"
#define TDI_TCP_CLIENT_DISPLAY_NAME_A     "Sftk TDI TTCP TCP Client"
#define TDI_TCP_CLIENT_DISPLAY_NAME_W     L"Sftk TDI TTCP TCP Client"
#define TDI_TCP_CLIENT_DEVICE_NAME_A      "\\Device\\SftkTdiTcpClient"
#define TDI_TCP_CLIENT_DEVICE_NAME_W      L"\\Device\\SftkTdiTcpClient"

#ifdef UNICODE

#define TDI_TCP_CLIENT_BASE_NAME       TDI_TCP_CLIENT_BASE_NAME_W
#define TDI_TCP_CLIENT_DISPLAY_NAME    TDI_TCP_CLIENT_DISPLAY_NAME_W
#define TDI_TCP_CLIENT_DEVICE_NAME     TDI_TCP_CLIENT_DEVICE_NAME_W

#else

#define TDI_TCP_CLIENT_BASE_NAME       TDI_TCP_CLIENT_BASE_NAME_A
#define TDI_TCP_CLIENT_DISPLAY_NAME    TDI_TCP_CLIENT_DISPLAY_NAME_A
#define TDI_TCP_CLIENT_DEVICE_NAME     TDI_TCP_CLIENT_DEVICE_NAME_A

#endif

#define SFTK_FILE_DEVICE_TCP_CLIENT   (SFTK_FILE_DEVICE_TDITTCP_BASE+1)
#define SFTK_IOCTL_TCP_CLIENT_BASE    SFTK_FILE_DEVICE_TCP_CLIENT

//This IOCTL Initializes the cache and then Creates a Thread that writes to the Simulated Cache
#define SFTK_IOCTL_TCP_CLIENT_START_CACHE\
   CTL_CODE(SFTK_IOCTL_TCP_CLIENT_BASE, 2048, METHOD_BUFFERED, FILE_ANY_ACCESS)

//The Parameters that are passed to this IOCTL is 
typedef struct _CACHE_START_PARAM
{
	LONG nCacheMaxSize;		//The Maximum Size of the Cache that has to be used.

}CACHE_START_PARAM, *PCACHE_START_PARAM;

//This IOCTL Initializes all the Sockets and then Tries to connect to the S
#define SFTK_IOCTL_TCP_CLIENT_START_CONNECTIONS\
   CTL_CODE(SFTK_IOCTL_TCP_CLIENT_BASE, 2049, METHOD_BUFFERED, FILE_ANY_ACCESS)


//This Structure is passed to the Client
//Client Spawns one Worker Thread per Socket and then calls the Cache Manager to Give Data
//If No Data is available the Worker Threads Just Wait and Sleep
//The Therads will wakeup either when there is some data in the cache
//OR the User issues a Kill Operation stopping all operations.
//The Worker Thread Calls the TDI_SEND once there is data in the Cache.
//All the Worker Threads are locked using a FAST_MUTEX until an Event is signalled 
//for the data in the Cache.


typedef struct _CONNECTION_INFO
{
	USHORT nNumberOfSessions;
	TDI_ADDRESS_IP ipLocalAddress;
	TDI_ADDRESS_IP ipRemoteAddress;
}CONNECTION_INFO, *PCONNECTION_INFO;

typedef struct _CONNECTION_DETAILS
{
	LONG nSendWindowSize;		//This is the Window Size that will be used per Socket Connection in MB
	LONG nReceiveWindowSize;
	USHORT nConnections;
	CONNECTION_INFO ConnectionDetails[1];
}CONNECTION_DETAILS, *PCONNECTION_DETAILS;

typedef struct _CONNECTION_TUNABLES
{
	ULONG nSendWindowSize;
	ULONG nReceiveWindowSize;
	ULONG nChunkSize;
}CONNECTION_TUNABLES , *PCONNECTION_TUNABLES;


//This IOCTL Stops all the Threads, Bothe
#define SFTK_IOCTL_TCP_CLIENT_STOP_ALL\
   CTL_CODE(SFTK_IOCTL_TCP_CLIENT_BASE, 2050, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define SFTK_IOCTL_TCP_ADD_CONNECTIONS\
   CTL_CODE(SFTK_IOCTL_TCP_CLIENT_BASE, 2051, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define SFTK_IOCTL_TCP_REMOVE_CONNECTIONS\
   CTL_CODE(SFTK_IOCTL_TCP_CLIENT_BASE, 2052, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define SFTK_IOCTL_TCP_ENABLE_CONNECTIONS\
   CTL_CODE(SFTK_IOCTL_TCP_CLIENT_BASE, 2053, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define SFTK_IOCTL_TCP_DISABLE_CONNECTIONS\
   CTL_CODE(SFTK_IOCTL_TCP_CLIENT_BASE, 2054, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define SFTK_IOCTL_TCP_QUERY_CONNECTIONS\
   CTL_CODE(SFTK_IOCTL_TCP_CLIENT_BASE, 2055, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define SFTK_IOCTL_TCP_SET_CONNECTIONS_TUNABLES\
   CTL_CODE(SFTK_IOCTL_TCP_CLIENT_BASE, 2056, METHOD_BUFFERED, FILE_ANY_ACCESS)


#endif // __TDIIOCTL_H__

