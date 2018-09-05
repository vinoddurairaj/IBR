/**************************************************************************************

Module Name: Sftk_TdiUtil.h   
Author Name: Veera Arja
Description: OS TDI definations goes here
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2004 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/
#include "INetInc.h"
#include "Sftk_TdiTcpEx.h"

#ifndef __SFTK_TDIUTIL_H__
#define __SFTK_TDIUTIL_H__

// Copyright And Configuration Management ----------------------------------
//
//             Header For TDI Test TCP (TTCP) Utilities - KSUTIL.h
//
//                  PCAUSA TDI Client Samples For Windows NT
//
//      Copyright (c) 1999-2001 Printing Communications Associates, Inc.
//                                - PCAUSA -
//
//                             Thomas F. Divine
//                           4201 Brunswick Court
//                        Smyrna, Georgia 30080 USA
//                              (770) 432-4580
//                            tdivine@pcausa.com
//
// End ---------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
//// DEFINITIONS

// This constant is used for places where NdisAllocateMemory
// needs to be called and the HighestAcceptableAddress does
// not matter.
//
extern NDIS_PHYSICAL_ADDRESS HighestAcceptableMax;

//
// TDI Transport Device Names
// --------------------------
// These are some of the TDI device names. They are used with ZwCreateFile
// to open address, connection and control channel objects.
//

//
// TCP/IP TDI Transport Device Names
//
#define TCP_DEVICE_NAME_W              L"\\Device\\Tcp"
#define UDP_DEVICE_NAME_W              L"\\Device\\Udp"
#define RAWIP_DEVICE_NAME_W            L"\\Device\\RawIp"   // Base Name

//
// AppleTalk TDI Transport Device Names
//
#define ATALK_DDP_DEVICE_NAME_W          L"\\Device\\AtalkDdp"
#define ATALK_ADSP_DEVICE_NAME_W         L"\\Device\\AtalkAdsp"
#define ATALK_ASP_SERVER_DEVICE_NAME_W   L"\\Device\\AtalkAspServer"
#define ATALK_ASP_CLIENT_DEVICE_NAME_W   L"\\Device\\AtalkAspClient"
#define ATALK_PAP_DEVICE_NAME_W          L"\\Device\\AtalkPap"

//
// Definitions for TCP states.
//
#define TCB_CLOSED     0   // Closed.
#define TCB_LISTEN     1   // Listening.
#define TCB_SYN_SENT   2   // SYN Sent.
#define TCB_SYN_RCVD   3   // SYN received.
#define TCB_ESTAB      4   // Established.
#define TCB_FIN_WAIT1  5   // FIN-WAIT-1
#define TCB_FIN_WAIT2  6   // FIN-WAIT-2
#define TCB_CLOSE_WAIT 7   // Close waiting.
#define TCB_CLOSING    8   // Closing state.
#define TCB_LAST_ACK   9   // Last ack state.
#define TCB_TIME_WAIT  10  // Time wait state.

//
// SPX/IPX TDI Transport Device Names
//

//
// NetBEUI TDI Transport Device Names
// ----------------------------------
// These are not predefined. Nbf.sys exports a device name for each bound
// adapter. The name consists of a "\Device\Nbf_" prefix followed by the
// adapter base name.
//
#define NBF_DEVICE_NAME_PREFIX         L"\\Device\\Nbf_" // Append Adapter Name
//#define NBF_TYPICAL_DEVICE_NAME_1      L"\\Device\\Nbf_DC21X41"
//#define NBF_TYPICAL_DEVICE_NAME_2      L"\\Device\\Nbf_PCASIMMP2"


/////////////////////////////////////////////////////////////////////////////
//// STRUCTURES

typedef
VOID
(*PTDI_REQUEST_COMPLETION_ROUTINE) (
    PVOID UserCompletionContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    ULONG Reserved
    );

typedef 
NTSTATUS
(*PTDI_CONNECT_COMPLETION_ROUTINUE)(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    );

//
// Be sure to zero the TDI_ADDRESS structure before initializing selected
// fields.
//
typedef
struct _TDI_ADDRESS
{
   NTSTATUS       m_nStatus;        // Status Of TDI_OpenTransportAddress Call

   LONG           m_ReferenceCount; // Number Of References To This Object

   HANDLE         m_hAddress;       // Handle to the address object
   PFILE_OBJECT   m_pFileObject;    // The Transport Address File Object

   PIRP           m_pAtomicIrp;

   LIST_ENTRY     m_ConnectionList;
}
   TDI_ADDRESS, *PTDI_ADDRESS;


typedef
struct _TDI_ENDPOINT
{
   LIST_ENTRY     m_ListElement;       // For Linking To m_ConnectionList

   NTSTATUS       m_nAssociateStatus;  // Status Of TDI_AssociateAddress Call

   LONG           m_ReferenceCount;    // Number Of References To This Object

   //
   // TDI Address Object Information
   //
   PTDI_ADDRESS    m_pTDI_Address;       // Pointer To Address Information

   //
   // TDI Connection Context Information
   //
   NTSTATUS       m_nOpenStatus;    // Status Of TDI_OpenConnectionContext Call

   HANDLE         m_hContext;       // Handle to the connection context object
   PFILE_OBJECT   m_pFileObject;    // The Connection Endpoint File Object
}
   TDI_ENDPOINT, *PTDI_ENDPOINT;


typedef
struct _TDI_CHANNEL
{
   HANDLE         m_hControlChannel; // Handle to the Control Channel object
   PFILE_OBJECT   m_pFileObject;    // The Control Channel File Object
}
   TDI_CHANNEL, *PTDI_CHANNEL;

/////////////////////////////////////////////////////////////////////////////
//// FUNCTION PROTOTYPES

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
//                    A D D R E S S  F U N C T I O N S                     //
/////////////////////////////////////////////////////////////////////////////

NTSTATUS
TDI_OpenTransportAddress(
   IN PWSTR                TransportDeviceNameW,// Zero-terminated String
   IN PTRANSPORT_ADDRESS   pTransportAddress,   // Local Transport Address
   PTDI_ADDRESS             pTDI_Address
   );

NTSTATUS
TDI_CloseTransportAddress(
   PTDI_ADDRESS             pTDI_Address
   );

NTSTATUS
TDI_SetEventHandlers(
   PTDI_ADDRESS             pTDI_Address,
   PVOID                   pEventContext,
   PTDI_IND_CONNECT        ConnectEventHandler,
   PTDI_IND_DISCONNECT     DisconnectEventHandler,
   PTDI_IND_ERROR          ErrorEventHandler,
   PTDI_IND_RECEIVE        ReceiveEventHandler,
   PTDI_IND_RECEIVE_DATAGRAM  ReceiveDatagramEventHandler,
   PTDI_IND_RECEIVE_EXPEDITED ReceiveExpeditedEventHandler
   );

NTSTATUS
TDI_SendDatagramOnAddress(
   PTDI_ADDRESS                   pTDI_Address,
   HANDLE                        UserCompletionEvent,    // Optional
   PTDI_REQUEST_COMPLETION_ROUTINE   UserCompletionRoutine,  // Optional
   PVOID                         UserCompletionContext,  // Optional
   PIO_STATUS_BLOCK              pIoStatusBlock,         // Required
   PMDL                          pSendMdl,
   PTDI_CONNECTION_INFORMATION   pSendDatagramInfo
   );

NTSTATUS
TDI_ReceiveDatagramOnAddress(
   PTDI_ADDRESS                   pTDI_Address,
   HANDLE                        UserCompletionEvent,    // Optional
   PTDI_REQUEST_COMPLETION_ROUTINE   UserCompletionRoutine,  // Optional
   PVOID                         UserCompletionContext,  // Optional
   PIO_STATUS_BLOCK              pIoStatusBlock,         // Required
   PMDL                          pReceiveMdl,
   PTDI_CONNECTION_INFORMATION   pReceiveDatagramInfo,
   PTDI_CONNECTION_INFORMATION   pReturnInfo,
   ULONG                         InFlags
   );

NTSTATUS
TDI_QueryAddressInfo(
   PFILE_OBJECT      pFileObject,
   PVOID             pInfoBuffer,
   PULONG            pInfoBufferSize
   );

/////////////////////////////////////////////////////////////////////////////
//                      E N D P O I N T  F U N C T I O N S                 //
/////////////////////////////////////////////////////////////////////////////

NTSTATUS
TDI_OpenConnectionEndpoint(
   PWSTR             TransportDeviceNameW,// Zero-terminated String
   PTDI_ADDRESS       pTDI_Address,
   PTDI_ENDPOINT      pTDI_Endpoint,
   PVOID             pContext
   );

NTSTATUS
TDI_CloseConnectionEndpoint(
   PTDI_ENDPOINT   pTDI_Endpoint
   );
///////////
//This is added by Veera

NTSTATUS
TDI_NewConnect(
   PTDI_ENDPOINT          pTDI_Endpoint,
   IN PTRANSPORT_ADDRESS   pTransportAddress, // Remote Transport Address
   IN PTDI_CONNECT_COMPLETION_ROUTINUE UserCompletionRoutine,	//Optional
   PVOID Context
   );

//This is added by Veera
//////////
NTSTATUS
TDI_Connect(
   PTDI_ENDPOINT          pTDI_Endpoint,
   IN PTRANSPORT_ADDRESS   pTransportAddress // Remote Transport Address
   );

NTSTATUS
TDI_Disconnect(
   PTDI_ENDPOINT                   pTDI_Endpoint,
   HANDLE                           UserCompletionEvent,    // Optional
   PTDI_REQUEST_COMPLETION_ROUTINE   UserCompletionRoutine,  // Optional
   PVOID                            UserCompletionContext,  // Optional
   PIO_STATUS_BLOCK                 pIoStatusBlock,         // Required
   ULONG                            Flags
   );

NTSTATUS
TDI_SendOnEndpoint(
   PTDI_ENDPOINT                pTDI_Endpoint,
   HANDLE                        UserCompletionEvent,    // Optional
   PTDI_REQUEST_COMPLETION_ROUTINE   UserCompletionRoutine,  // Optional
   PVOID                         UserCompletionContext,  // Optional
   PIO_STATUS_BLOCK              pIoStatusBlock,          // Required
   PMDL                          pSendMdl,
   ULONG                         SendFlags
   );

NTSTATUS
TDI_SendOnEndpoint1(
   PTDI_ENDPOINT                     pTDI_Endpoint,
   HANDLE                           UserCompletionEvent,    // Optional
   PIO_COMPLETION_ROUTINE   UserCompletionRoutine,  // Optional
   PVOID                            UserCompletionContext,  // Optional
   PIO_STATUS_BLOCK                 pIoStatusBlock,         // Required
   PMDL                             pSendMdl,
   ULONG                            SendFlags,
   PIRP								*pRequestIrp
   );

NTSTATUS
TDI_ReceiveOnEndpoint(
   PTDI_ENDPOINT                pTDI_Endpoint,
   HANDLE                        UserCompletionEvent,    // Optional
   PTDI_REQUEST_COMPLETION_ROUTINE   UserCompletionRoutine,  // Optional
   PVOID                         UserCompletionContext,  // Optional
   PIO_STATUS_BLOCK              pIoStatusBlock,          // Required
   PMDL                          pReceiveMdl,
   ULONG                         ReceiveFlags
   );

NTSTATUS
TDI_ReceiveOnEndpoint1(
   PTDI_ENDPOINT                     pTDI_Endpoint,
   HANDLE                           UserCompletionEvent,    // Optional
   PIO_COMPLETION_ROUTINE   UserCompletionRoutine,  // Optional
   PVOID                            UserCompletionContext,  // Optional
   PIO_STATUS_BLOCK                 pIoStatusBlock,          // Required
   PMDL                             pReceiveMdl,
   ULONG                            ReceiveFlags,
   PIRP								*pRequestIrp
   );

NTSTATUS
TDI_QueryConnectionInfo(
   PTDI_ENDPOINT      pTDI_Endpoint,
   PVOID             pInfoBuffer,
   PULONG            pInfoBufferSize
   );

NTSTATUS
TDI_SetConnectionInfo(
   PTDI_ENDPOINT       pTDI_Endpoint,
   PTDI_CONNECTION_INFO pConnectionInfo
   );

/////////////////////////////////////////////////////////////////////////////
//                T C P  E X T E N S I O N  F U N C T I O N S              //
/////////////////////////////////////////////////////////////////////////////

NTSTATUS
TDI_TCPSetInformation(
   PFILE_OBJECT   pFileObject,
   ULONG          Entity,
   ULONG          Class,
   ULONG          Type,
   ULONG          Id,
   PVOID          pValue,
   ULONG          ValueLength
   );

NTSTATUS
TDI_TCPQueryInformation(
   PFILE_OBJECT   pFileObject,
   ULONG          Entity,
   ULONG          Instance,
   ULONG          Class,
   ULONG          Type,
   ULONG          Id,
   PVOID          pOutputBuffer,
   PULONG         pdOutputLength
   );

NTSTATUS
TDI_TCPGetIPSNMPInfo(
   PFILE_OBJECT   pFileObject,
   IPSNMPInfo     *pIPSnmpInfo,
   PULONG         pBufferSize
   );

NTSTATUS
TDI_TCPGetIPAddrTable(
   PFILE_OBJECT   pFileObject,
   IPAddrEntry    *pIPAddrTable,
   PULONG         pBufferSize
   );

NTSTATUS
TDI_TCPGetIPRouteTable(
   PFILE_OBJECT   pFileObject,
   IPRouteEntry   *pIPRouteEntry,
   PULONG         pBufferSize
   );

NTSTATUS
TDI_TCPGetIPInterfaceInfoForAddr(
   PFILE_OBJECT      pFileObject,
   IPAddr            theIPAddr,
   PUCHAR            pQueryBuffer,// IPInterfaceInfo size + MAX_PHYSADDR_SIZE for address bytes
   PULONG            pBufferSize
   );

/////////////////////////////////////////////////////////////////////////////
//                     U T I L I T Y  F U N C T I O N S                    //
/////////////////////////////////////////////////////////////////////////////

PMDL
TDI_AllocateAndProbeMdl(
   PVOID VirtualAddress,
   ULONG Length,
   BOOLEAN SecondaryBuffer,
   BOOLEAN ChargeQuota,
   PIRP Irp OPTIONAL
   );

VOID
TDI_UnlockAndFreeMdl(
   PMDL pMdl
   );

NTSTATUS
TDI_MakeSimpleTdiRequest(
   IN PDEVICE_OBJECT pDeviceObject,
   IN PIRP pIrp
   );

NTSTATUS
TDI_BuildEaBuffer(
    IN  ULONG                     EaNameLength,
    IN  PVOID                     pEaName,
    IN  ULONG                     EaValueLength,
    IN  PVOID                     pEaValue,
    OUT PFILE_FULL_EA_INFORMATION *pEaBufferPointer,
    OUT PULONG                    pEaBufferLength
    );

ULONG
TDI_TransportAddressLength(
   PTRANSPORT_ADDRESS pTransportAddress
   );

VOID TDI_InitIPAddress(
   PTA_IP_ADDRESS pTransportAddress,
   ULONG          IPAddress,  // As 4-byte ULONG, Network Byte Order
   USHORT         IPPort      // Network Byte Order
   );

/////////////////////////////////////////////////////////////////////////////
//           C O N T R O L  C H A N N E L  F U N C T I O N S               //
/////////////////////////////////////////////////////////////////////////////

NTSTATUS
TDI_OpenControlChannel(
   IN PWSTR       TransportDeviceNameW,// Zero-terminated String
   PTDI_CHANNEL    pTDI_Channel
   );

NTSTATUS
TDI_CloseControlChannel(
   PTDI_CHANNEL    pTDI_Channel
   );

NTSTATUS
TDI_QueryProviderInfo(
   PWSTR                TransportDeviceNameW,// Zero-terminated String
   PTDI_PROVIDER_INFO   pProviderInfo
   );

NTSTATUS
TDI_QueryProviderStatistics(
   PWSTR                    TransportDeviceNameW,// Zero-terminated String
   PTDI_PROVIDER_STATISTICS pProviderStatistics
   );

NTSTATUS
TDI_CreateSymbolicLink(
   IN PUNICODE_STRING  DeviceName,      // "\\Device\\TDITTCP"
   IN BOOLEAN bCreate
   );

VOID TDI_FreePacketAndBuffers(
   PNDIS_PACKET Packet
   );

BOOLEAN  TDI_IsBigEndian();

USHORT   TDI_htons( USHORT hostshort );
USHORT   TDI_ntohs( USHORT netshort );
ULONG    TDI_htonl( ULONG hostlong );
ULONG    TDI_ntohl( ULONG netlong );

USHORT   TDI_in_cksum( PUCHAR pStartingByte, int nByteCount );

//
// Maximum length of NT process name
//
#define NT_PROCNAMELEN  16

ULONG    TDI_InitProcessNameOffset( VOID );
BOOLEAN  TDI_GetCurrentProcessName( PCHAR ProcessName );


/////////////////////////////////////////////////////////////////////////////
//                        D E B U G  F U N C T I O N S                     //
/////////////////////////////////////////////////////////////////////////////

#ifdef DBG

NTSTATUS
DEBUG_DumpProviderInfo(
   PWSTR pDeviceName,
   PTDI_PROVIDER_INFO pProviderInfo
   );

NTSTATUS
DEBUG_DumpProviderStatistics(
   PWSTR pDeviceName,
   PTDI_PROVIDER_STATISTICS pProviderStatistics
   );

VOID
DEBUG_DumpTAAddress(
   USHORT nAddressType,			// e.g., TDI_ADDRESS_TYPE_IP
   PUCHAR pTAAddr
   );

VOID
DEBUG_DumpTransportAddress(
   PTRANSPORT_ADDRESS pTransAddr
   );

VOID
DEBUG_DumpConnectionInfo(
   PTDI_CONNECTION_INFO pConnectionInfo
   );

VOID
DEBUG_DumpAddressInfo(
   PTDI_ADDRESS_INFO pAddressInfo
   );

#else

#define DEBUG_DumpProviderInfo(n,i)
#define DEBUG_DumpProviderStatistics(n,i)
#define DEBUG_DumpTAAddress(n,p)
#define DEBUG_DumpTransportAddress(p)
#define DEBUG_DumpConnectionInfo(p)
#define DEBUG_DumpAddressInfo(p)

#endif DBG

#ifdef __cplusplus
}
#endif

#endif // __TDIUTIL_H__

