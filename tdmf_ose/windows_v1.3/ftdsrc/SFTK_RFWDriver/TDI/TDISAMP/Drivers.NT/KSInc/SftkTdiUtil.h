/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#include "INetInc.h"
#include "SftkTdiTcpEx.h"

#ifndef __KSUTIL_H__
#define __KSUTIL_H__

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
(*PSFTK_TDI_REQUEST_COMPLETION_ROUTINE) (
    PVOID UserCompletionContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    ULONG Reserved
    );

typedef 
NTSTATUS
(*PSFTK_TDI_CONNECT_COMPLETION_ROUTINUE)(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    );

//
// Be sure to zero the SFTK_TDI_ADDRESS structure before initializing selected
// fields.
//
typedef
struct _SFTK_TDI_ADDRESS
{
   NTSTATUS       m_nStatus;        // Status Of SFTK_TDI_OpenTransportAddress Call

   LONG           m_ReferenceCount; // Number Of References To This Object

   HANDLE         m_hAddress;       // Handle to the address object
   PFILE_OBJECT   m_pFileObject;    // The Transport Address File Object

   PIRP           m_pAtomicIrp;

   LIST_ENTRY     m_ConnectionList;
}
   SFTK_TDI_ADDRESS, *PSFTK_TDI_ADDRESS;


typedef
struct _SFTK_TDI_ENDPOINT
{
   LIST_ENTRY     m_ListElement;       // For Linking To m_ConnectionList

   NTSTATUS       m_nAssociateStatus;  // Status Of SFTK_TDI_AssociateAddress Call

   LONG           m_ReferenceCount;    // Number Of References To This Object

   //
   // TDI Address Object Information
   //
   PSFTK_TDI_ADDRESS    m_pSFTK_TDI_Address;       // Pointer To Address Information

   //
   // TDI Connection Context Information
   //
   NTSTATUS       m_nOpenStatus;    // Status Of SFTK_TDI_OpenConnectionContext Call

   HANDLE         m_hContext;       // Handle to the connection context object
   PFILE_OBJECT   m_pFileObject;    // The Connection Endpoint File Object
}
   SFTK_TDI_ENDPOINT, *PSFTK_TDI_ENDPOINT;


typedef
struct _SFTK_TDI_CHANNEL
{
   HANDLE         m_hControlChannel; // Handle to the Control Channel object
   PFILE_OBJECT   m_pFileObject;    // The Control Channel File Object
}
   SFTK_TDI_CHANNEL, *PSFTK_TDI_CHANNEL;

/////////////////////////////////////////////////////////////////////////////
//// FUNCTION PROTOTYPES

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
//                    A D D R E S S  F U N C T I O N S                     //
/////////////////////////////////////////////////////////////////////////////

NTSTATUS
SFTK_TDI_OpenTransportAddress(
   IN PWSTR                TransportDeviceNameW,// Zero-terminated String
   IN PTRANSPORT_ADDRESS   pTransportAddress,   // Local Transport Address
   PSFTK_TDI_ADDRESS             pSFTK_TDI_Address
   );

NTSTATUS
SFTK_TDI_CloseTransportAddress(
   PSFTK_TDI_ADDRESS             pSFTK_TDI_Address
   );

NTSTATUS
SFTK_TDI_SetEventHandlers(
   PSFTK_TDI_ADDRESS             pSFTK_TDI_Address,
   PVOID                   pEventContext,
   PTDI_IND_CONNECT        ConnectEventHandler,
   PTDI_IND_DISCONNECT     DisconnectEventHandler,
   PTDI_IND_ERROR          ErrorEventHandler,
   PTDI_IND_RECEIVE        ReceiveEventHandler,
   PTDI_IND_RECEIVE_DATAGRAM  ReceiveDatagramEventHandler,
   PTDI_IND_RECEIVE_EXPEDITED ReceiveExpeditedEventHandler
   );

NTSTATUS
SFTK_TDI_SendDatagramOnAddress(
   PSFTK_TDI_ADDRESS                   pSFTK_TDI_Address,
   HANDLE                        UserCompletionEvent,    // Optional
   PSFTK_TDI_REQUEST_COMPLETION_ROUTINE   UserCompletionRoutine,  // Optional
   PVOID                         UserCompletionContext,  // Optional
   PIO_STATUS_BLOCK              pIoStatusBlock,         // Required
   PMDL                          pSendMdl,
   PTDI_CONNECTION_INFORMATION   pSendDatagramInfo
   );

NTSTATUS
SFTK_TDI_ReceiveDatagramOnAddress(
   PSFTK_TDI_ADDRESS                   pSFTK_TDI_Address,
   HANDLE                        UserCompletionEvent,    // Optional
   PSFTK_TDI_REQUEST_COMPLETION_ROUTINE   UserCompletionRoutine,  // Optional
   PVOID                         UserCompletionContext,  // Optional
   PIO_STATUS_BLOCK              pIoStatusBlock,         // Required
   PMDL                          pReceiveMdl,
   PTDI_CONNECTION_INFORMATION   pReceiveDatagramInfo,
   PTDI_CONNECTION_INFORMATION   pReturnInfo,
   ULONG                         InFlags
   );

NTSTATUS
SFTK_TDI_QueryAddressInfo(
   PFILE_OBJECT      pFileObject,
   PVOID             pInfoBuffer,
   PULONG            pInfoBufferSize
   );

/////////////////////////////////////////////////////////////////////////////
//                      E N D P O I N T  F U N C T I O N S                 //
/////////////////////////////////////////////////////////////////////////////

NTSTATUS
SFTK_TDI_OpenConnectionEndpoint(
   PWSTR             TransportDeviceNameW,// Zero-terminated String
   PSFTK_TDI_ADDRESS       pSFTK_TDI_Address,
   PSFTK_TDI_ENDPOINT      pSFTK_TDI_Endpoint,
   PVOID             pContext
   );

NTSTATUS
SFTK_TDI_CloseConnectionEndpoint(
   PSFTK_TDI_ENDPOINT   pSFTK_TDI_Endpoint
   );
///////////
//This is added by Veera

NTSTATUS
SFTK_TDI_NewConnect(
   PSFTK_TDI_ENDPOINT          pSFTK_TDI_Endpoint,
   IN PTRANSPORT_ADDRESS   pTransportAddress, // Remote Transport Address
   IN PSFTK_TDI_CONNECT_COMPLETION_ROUTINUE UserCompletionRoutine,	//Optional
   PVOID Context
   );

//This is added by Veera
//////////
NTSTATUS
SFTK_TDI_Connect(
   PSFTK_TDI_ENDPOINT          pSFTK_TDI_Endpoint,
   IN PTRANSPORT_ADDRESS   pTransportAddress // Remote Transport Address
   );

NTSTATUS
SFTK_TDI_Disconnect(
   PSFTK_TDI_ENDPOINT                   pSFTK_TDI_Endpoint,
   HANDLE                           UserCompletionEvent,    // Optional
   PSFTK_TDI_REQUEST_COMPLETION_ROUTINE   UserCompletionRoutine,  // Optional
   PVOID                            UserCompletionContext,  // Optional
   PIO_STATUS_BLOCK                 pIoStatusBlock,         // Required
   ULONG                            Flags
   );

NTSTATUS
SFTK_TDI_SendOnEndpoint(
   PSFTK_TDI_ENDPOINT                pSFTK_TDI_Endpoint,
   HANDLE                        UserCompletionEvent,    // Optional
   PSFTK_TDI_REQUEST_COMPLETION_ROUTINE   UserCompletionRoutine,  // Optional
   PVOID                         UserCompletionContext,  // Optional
   PIO_STATUS_BLOCK              pIoStatusBlock,          // Required
   PMDL                          pSendMdl,
   ULONG                         SendFlags
   );

NTSTATUS
SFTK_TDI_SendOnEndpoint1(
   PSFTK_TDI_ENDPOINT                     pSFTK_TDI_Endpoint,
   HANDLE                           UserCompletionEvent,    // Optional
   PIO_COMPLETION_ROUTINE   UserCompletionRoutine,  // Optional
   PVOID                            UserCompletionContext,  // Optional
   PIO_STATUS_BLOCK                 pIoStatusBlock,         // Required
   PMDL                             pSendMdl,
   ULONG                            SendFlags,
   PIRP								*pRequestIrp
   );

NTSTATUS
SFTK_TDI_ReceiveOnEndpoint(
   PSFTK_TDI_ENDPOINT                pSFTK_TDI_Endpoint,
   HANDLE                        UserCompletionEvent,    // Optional
   PSFTK_TDI_REQUEST_COMPLETION_ROUTINE   UserCompletionRoutine,  // Optional
   PVOID                         UserCompletionContext,  // Optional
   PIO_STATUS_BLOCK              pIoStatusBlock,          // Required
   PMDL                          pReceiveMdl,
   ULONG                         ReceiveFlags
   );

NTSTATUS
SFTK_TDI_ReceiveOnEndpoint1(
   PSFTK_TDI_ENDPOINT                     pSFTK_TDI_Endpoint,
   HANDLE                           UserCompletionEvent,    // Optional
   PIO_COMPLETION_ROUTINE   UserCompletionRoutine,  // Optional
   PVOID                            UserCompletionContext,  // Optional
   PIO_STATUS_BLOCK                 pIoStatusBlock,          // Required
   PMDL                             pReceiveMdl,
   ULONG                            ReceiveFlags,
   PIRP								*pRequestIrp
   );

NTSTATUS
SFTK_TDI_QueryConnectionInfo(
   PSFTK_TDI_ENDPOINT      pSFTK_TDI_Endpoint,
   PVOID             pInfoBuffer,
   PULONG            pInfoBufferSize
   );

NTSTATUS
SFTK_TDI_SetConnectionInfo(
   PSFTK_TDI_ENDPOINT       pSFTK_TDI_Endpoint,
   PTDI_CONNECTION_INFO pConnectionInfo
   );

/////////////////////////////////////////////////////////////////////////////
//                T C P  E X T E N S I O N  F U N C T I O N S              //
/////////////////////////////////////////////////////////////////////////////

NTSTATUS
SFTK_TDI_TCPSetInformation(
   PFILE_OBJECT   pFileObject,
   ULONG          Entity,
   ULONG          Class,
   ULONG          Type,
   ULONG          Id,
   PVOID          pValue,
   ULONG          ValueLength
   );

NTSTATUS
SFTK_TDI_TCPQueryInformation(
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
SFTK_TDI_TCPGetIPSNMPInfo(
   PFILE_OBJECT   pFileObject,
   IPSNMPInfo     *pIPSnmpInfo,
   PULONG         pBufferSize
   );

NTSTATUS
SFTK_TDI_TCPGetIPAddrTable(
   PFILE_OBJECT   pFileObject,
   IPAddrEntry    *pIPAddrTable,
   PULONG         pBufferSize
   );

NTSTATUS
SFTK_TDI_TCPGetIPRouteTable(
   PFILE_OBJECT   pFileObject,
   IPRouteEntry   *pIPRouteEntry,
   PULONG         pBufferSize
   );

NTSTATUS
SFTK_TDI_TCPGetIPInterfaceInfoForAddr(
   PFILE_OBJECT      pFileObject,
   IPAddr            theIPAddr,
   PUCHAR            pQueryBuffer,// IPInterfaceInfo size + MAX_PHYSADDR_SIZE for address bytes
   PULONG            pBufferSize
   );

/////////////////////////////////////////////////////////////////////////////
//                     U T I L I T Y  F U N C T I O N S                    //
/////////////////////////////////////////////////////////////////////////////

PMDL
SFTK_TDI_AllocateAndProbeMdl(
   PVOID VirtualAddress,
   ULONG Length,
   BOOLEAN SecondaryBuffer,
   BOOLEAN ChargeQuota,
   PIRP Irp OPTIONAL
   );

VOID
SFTK_TDI_UnlockAndFreeMdl(
   PMDL pMdl
   );

NTSTATUS
SFTK_TDI_MakeSimpleTdiRequest(
   IN PDEVICE_OBJECT pDeviceObject,
   IN PIRP pIrp
   );

NTSTATUS
SFTK_TDI_BuildEaBuffer(
    IN  ULONG                     EaNameLength,
    IN  PVOID                     pEaName,
    IN  ULONG                     EaValueLength,
    IN  PVOID                     pEaValue,
    OUT PFILE_FULL_EA_INFORMATION *pEaBufferPointer,
    OUT PULONG                    pEaBufferLength
    );

ULONG
SFTK_TDI_TransportAddressLength(
   PTRANSPORT_ADDRESS pTransportAddress
   );

VOID SFTK_TDI_InitIPAddress(
   PTA_IP_ADDRESS pTransportAddress,
   ULONG          IPAddress,  // As 4-byte ULONG, Network Byte Order
   USHORT         IPPort      // Network Byte Order
   );

/////////////////////////////////////////////////////////////////////////////
//           C O N T R O L  C H A N N E L  F U N C T I O N S               //
/////////////////////////////////////////////////////////////////////////////

NTSTATUS
SFTK_TDI_OpenControlChannel(
   IN PWSTR       TransportDeviceNameW,// Zero-terminated String
   PSFTK_TDI_CHANNEL    pSFTK_TDI_Channel
   );

NTSTATUS
SFTK_TDI_CloseControlChannel(
   PSFTK_TDI_CHANNEL    pSFTK_TDI_Channel
   );

NTSTATUS
SFTK_TDI_QueryProviderInfo(
   PWSTR                TransportDeviceNameW,// Zero-terminated String
   PTDI_PROVIDER_INFO   pProviderInfo
   );

NTSTATUS
SFTK_TDI_QueryProviderStatistics(
   PWSTR                    TransportDeviceNameW,// Zero-terminated String
   PTDI_PROVIDER_STATISTICS pProviderStatistics
   );

NTSTATUS
SFTK_TDI_CreateSymbolicLink(
   IN PUNICODE_STRING  DeviceName,      // "\\Device\\TDITTCP"
   IN BOOLEAN bCreate
   );

VOID SFTK_TDI_FreePacketAndBuffers(
   PNDIS_PACKET Packet
   );

BOOLEAN  SFTK_TDI_IsBigEndian();

USHORT   SFTK_TDI_htons( USHORT hostshort );
USHORT   SFTK_TDI_ntohs( USHORT netshort );
ULONG    SFTK_TDI_htonl( ULONG hostlong );
ULONG    SFTK_TDI_ntohl( ULONG netlong );

USHORT   SFTK_TDI_in_cksum( PUCHAR pStartingByte, int nByteCount );

//
// Maximum length of NT process name
//
#define NT_PROCNAMELEN  16

ULONG    SFTK_TDI_InitProcessNameOffset( VOID );
BOOLEAN  SFTK_TDI_GetCurrentProcessName( PCHAR ProcessName );


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

#endif // __KSUTIL_H__

