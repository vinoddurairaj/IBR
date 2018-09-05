/**************************************************************************************

Module Name: COMProto.h.h   
Author Name: Veera Arja
Description: Define all APIs used for Modules: Com, OsTdi,Proto Structures and macro definations 
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2002 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/

#ifndef _COMPROTO_H_
#define _COMPROTO_H_


NTSTATUS 
TDI_ErrorEventHandler(
						IN PVOID TdiEventContext,  // The endpoint's file object.
						IN NTSTATUS Status         // Status code indicating error type.
						);

NTSTATUS 
TDI_ReceiveEventHandler(
						IN PVOID				TdiEventContext,     // Context From SetEventHandler
						IN CONNECTION_CONTEXT	ConnectionContext,
						IN ULONG				ReceiveFlags,
						IN ULONG				BytesIndicated,
						IN ULONG				BytesAvailable,
						OUT ULONG *				BytesTaken,
						IN PVOID				Tsdu,				// pointer describing this TSDU, typically a lump of bytes
						OUT PIRP				*IoRequestPacket	// TdiReceive IRP if MORE_PROCESSING_REQUIRED.
						);

NTSTATUS 
TDI_ReceiveExpeditedEventHandler(
									IN PVOID				TdiEventContext,     // Context From SetEventHandler
									IN CONNECTION_CONTEXT	ConnectionContext,
									IN ULONG				ReceiveFlags,          //
									IN ULONG				BytesIndicated,        // number of bytes in this indication
									IN ULONG				BytesAvailable,        // number of bytes in complete Tsdu
									OUT ULONG				*BytesTaken,          // number of bytes used by indication routine
									IN PVOID				Tsdu,                  // pointer describing this TSDU, typically a lump of bytes
									OUT PIRP				*IoRequestPacket        // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
   );


NTSTATUS
TDI_ConnectedCallback(
						IN PDEVICE_OBJECT	pDeviceObject,
						IN PIRP				pIrp,
						IN PVOID			Context
						);

///Query Functions that Test for Address and Connection Information

VOID
TDI_DoQueryAddressInfoTest(
						PTCP_SESSION  pSession
						);

VOID
TDI_DoQueryConnectionInfoTest(
							PTCP_SESSION  pSession
							);

//
// Function prototype definations defined in file_name2.c 
//


NTSTATUS 
TDI_DisconnectEventHandler(
							IN PVOID				TdiEventContext,     // Context From SetEventHandler
							IN CONNECTION_CONTEXT	ConnectionContext,
							IN LONG					DisconnectDataLength,
							IN PVOID				DisconnectData,
							IN LONG					DisconnectInformationLength,
							IN PVOID				DisconnectInformation,
							IN ULONG				DisconnectFlags
							);


NTSTATUS 
TDI_EndpointReceiveRequestComplete(
								IN PDEVICE_OBJECT		pDeviceObject,
								IN PIRP					pIrp,
								IN PVOID				Context
								);

////////////////////PnP Events
#if _WIN32_WINNT >= 0x0500

NTSTATUS
TDI_ClientPnPPowerChange(
					IN PUNICODE_STRING			DeviceName,
					IN PNET_PNP_EVENT			PowerEvent,
					IN PTDI_PNP_CONTEXT			Context1,
					IN PTDI_PNP_CONTEXT			Context2
					);
VOID
TDI_ClientPnPBindingChange(
					IN TDI_PNP_OPCODE			PnPOpcode,
					IN PUNICODE_STRING			DeviceName,
					IN PWSTR					MultiSZBindList
					);

VOID
TDI_ClientPnPAddNetAddress(
					IN PTA_ADDRESS				Address,
					IN PUNICODE_STRING			DeviceName,
					IN PTDI_PNP_CONTEXT			Context
					);
VOID
TDI_ClientPnPDelNetAddress(
					IN PTA_ADDRESS				Address,
					IN PUNICODE_STRING			DeviceName,
					IN PTDI_PNP_CONTEXT			Context
					);

#endif
//Thread Functions


VOID 
COM_CleanUpSession(
			PTCP_SESSION					pSession
			);


VOID 
COM_StopSendReceiveThreads(
					PSESSION_MANAGER		pSessionManager
					);

PSEND_BUFFER 
COM_GetNextSendBuffer(
				PSEND_BUFFER_LIST			pSendList
				);

PRECEIVE_BUFFER 
COM_GetNextReceiveBuffer(
					PRECEIVE_BUFFER_LIST	pReceiveList
					);

PSEND_BUFFER 
COM_InsertBufferIntoSendListTail(
				IN PSEND_BUFFER_LIST pSendList,
				IN PSEND_BUFFER pSendBuffer
				);

PRECEIVE_BUFFER
COM_InsertBufferIntoReceiveListTail(
						IN PRECEIVE_BUFFER_LIST pReceiveList,
						IN PRECEIVE_BUFFER pReceiveBuffer
						);

NTSTATUS 
COM_CreateSendReceiveThreads(
						PSESSION_MANAGER	pSessionManager
						);

VOID 
COM_InitializeSendBufferList(
						IN PSEND_BUFFER_LIST pSendList,
						IN PSM_INIT_PARAMS pSessionManagerParameters
						);

VOID 
COM_InitializeReceiveBufferList(
						IN PRECEIVE_BUFFER_LIST	pReceiveList,
						IN PSM_INIT_PARAMS pSessionManagerParameters
						);

PRECEIVE_BUFFER 
COM_GetNextStaleReceiveBuffer(
						PRECEIVE_BUFFER_LIST	pReceiveList
						);

PSEND_BUFFER 
COM_GetNextStaleSendBuffer(
					PSEND_BUFFER_LIST		pSendList
					);

VOID 
COM_ClearSendBuffer(
			PSEND_BUFFER pSendBuffer
			);

VOID 
COM_ClearReceiveBuffer(
				PRECEIVE_BUFFER				pReceiveBuffer
				);

VOID 
COM_ClearSendBufferList(
				PSEND_BUFFER_LIST			pSendList
				);

VOID 
COM_ClearReceiveBufferList(
					PRECEIVE_BUFFER_LIST	pReceiveList
					);

VOID 
COM_CleanUpServer(
			PSERVER_ELEMENT				pServer
			);

NDIS_STATUS 
COM_GetSessionReceiveBuffer(
					PTCP_SESSION		pSession
					);

NTSTATUS 
COM_SendVector(
			SFTK_IO_VECTOR				vectorArray[] , 
			LONG						len , 
			PSESSION_MANAGER			pSessionManager
			);

NTSTATUS 
COM_SendBufferVector(
			IN SFTK_IO_VECTOR vectorArray[] , 
			IN LONG				len , 
			IN PSESSION_MANAGER pSessionManager
			);

NTSTATUS 
COM_SendBuffer(
			IN PUCHAR			lpBuffer , 
			IN LONG				len , 
			IN PSESSION_MANAGER pSessionManager
			);

NTSTATUS 
COM_FindConnectionsCount(	IN	PSESSION_MANAGER		pSessionManager ,
						IN	PCONNECTION_DETAILS		pConnectionDetails , 
						IN	BOOLEAN					bEnabled, 
						OUT PULONG					pServerCount, 
						OUT PLIST_ENTRY				pServerList);

NTSTATUS 
COM_AddConnections(
			PSESSION_MANAGER pSessionManager ,
			PCONNECTION_DETAILS pConnectionDetails
			);

NTSTATUS 
COM_EnableConnections(
				PSESSION_MANAGER pSessionManager ,
				PCONNECTION_DETAILS pConnectionDetails, 
				BOOLEAN bEnable
				);

NTSTATUS 
COM_RemoveConnections(
				PSESSION_MANAGER pSessionManager ,
				PCONNECTION_DETAILS pConnectionDetails
				);

VOID 
COM_FreeConnections(
			PLIST_ENTRY pServerList
			);

NTSTATUS 
COM_AllocateServer(
			PSERVER_ELEMENT *ppServer,
			PCONNECTION_INFO pConnectionInfo
			);

VOID 
COM_FreeServer(
		PSERVER_ELEMENT pServer
		);

NTSTATUS 
COM_AllocateSession(
			PTCP_SESSION *ppSession, 
			PSERVER_ELEMENT pServer, 
			PCONNECTION_INFO pConnectionInfo
			);

VOID 
COM_FreeSession(
		PTCP_SESSION pSession
		);

NTSTATUS 
COM_StartServer(
				PSERVER_ELEMENT pServer, 
				eSessionType type
				);

NTSTATUS 
COM_StartServerExclusive(
				IN PSERVER_ELEMENT pServer, 
				IN eSessionType type
				);

LONG 
COM_InitializeSession1(
				PTCP_SESSION pSession, 
				eSessionType type
				);

VOID 
COM_UninitializeSession1(
					PTCP_SESSION pSession
					);

VOID 
COM_UninitializeServer1(
				PSERVER_ELEMENT pServer
				);

VOID COM_StopServer(
					PSERVER_ELEMENT pServer
					);

VOID COM_StopServerExclusive(
					IN PSERVER_ELEMENT pServer
					);


//These are the new threads that will be used along with the InitialzeSessionManager()

NTSTATUS 
COM_InitializeSessionManager(
						IN PSESSION_MANAGER pSessionManager,		
						IN PSM_INIT_PARAMS pSessionManagerParameters
						);

NTSTATUS 
COM_SendThread2(
		PSESSION_MANAGER pSessionManager
		);

NTSTATUS 
COM_ReceiveThread2(
			PSESSION_MANAGER pSessionManager
			);

NTSTATUS 
COM_ListenThread(
				 PANCHOR_LINKLIST pLg_GroupList
				 );

NTSTATUS 
COM_ListenThread2(
			PSESSION_MANAGER pSessionManager
			);

NTSTATUS 
COM_ConnectThread2(
			PSESSION_MANAGER pSessionManager
			);

NTSTATUS
COM_EnableAllSessionManager(
						IN PSESSION_MANAGER pSessionManager
						);

NTSTATUS
COM_DisableAllSessionManager(
						IN PSESSION_MANAGER pSessionManager
						);

NTSTATUS
COM_ResetAllConnections( 
					IN PSESSION_MANAGER pSessionManager
					);

NTSTATUS
COM_ResetAllSessionManager( 
						  IN PSESSION_MANAGER pSessionManager
						  );

VOID 
COM_StopSessionManager(
					IN PSESSION_MANAGER pSessionManager
					);

VOID 
COM_UninitializeSessionManager(
							IN PSESSION_MANAGER pSessionManager
							);
NTSTATUS
COM_StartSendThreadForServer(
						  PSERVER_ELEMENT pServerElement
						  );

NTSTATUS
COM_StartReceiveThreadForServer(
						  PSERVER_ELEMENT pServerElement
						  );

NTSTATUS
COM_StopSendReceiveThreadForServer(
						  PSERVER_ELEMENT pServerElement
						  );

NTSTATUS 
COM_ConnectThreadForServer(
			PSESSION_MANAGER pSessionManager
			);

NTSTATUS 
COM_ListenThreadForServer(
			PSESSION_MANAGER pSessionManager
			);

NTSTATUS 
COM_SendThreadForServer(
		PSERVER_ELEMENT pServerElement
		);

NTSTATUS 
COM_ReceiveThreadForServer(
			PSERVER_ELEMENT pServerElement
			);


NTSTATUS 
COM_StartPMD(
			IN PSESSION_MANAGER pSessionManager,
			IN PSM_INIT_PARAMS pSessionManagerParameters
			);

VOID 
COM_StopPMD(
		  PSESSION_MANAGER pSessionManager
		  );


NTSTATUS 
COM_StartRMD(
			IN PSESSION_MANAGER pSessionManager,
			IN PSM_INIT_PARAMS pSessionManagerParameters
			);

VOID 
COM_StopRMD(
		  PSESSION_MANAGER pSessionManager
		  );


NTSTATUS 
PROTO_ProcessReceiveHeader(
						ftd_header_t*	pHeader, 
						PULONG			pLen, 
						PTCP_SESSION	pSession
						);

NTSTATUS 
PROTO_ProcessReceiveData(
					IN ftd_header_t*	pHeader , 
					IN PUCHAR			pDataBuffer , 
					IN ULONG			nDataLength , 
					OUT PULONG			pLen , 
					IN PTCP_SESSION		pSession
					);

NTSTATUS 
PROTO_ReceiveHUP(
			IN ftd_header_t*			pHeader , 
			IN PTCP_SESSION				pSession
			);

NTSTATUS 
PROTO_ReceiveACKHUP(
				IN ftd_header_t*		pHeader,
				IN PTCP_SESSION			pSession
				);

NTSTATUS 
PROTO_ReceiveRFFSTART(
				IN ftd_header_t*		pHeader,
				IN PTCP_SESSION			pSession
				);

NTSTATUS 
PROTO_ReceiveACKRSYNC(
				IN ftd_header_t*		pHeader,
				IN PTCP_SESSION			pSession
				);

NTSTATUS 
PROTO_ReceiveRFFEND(
				IN ftd_header_t*		pHeader,
				IN PTCP_SESSION			pSession
				);

NTSTATUS 
PROTO_ReceiveBFSTART(
				IN ftd_header_t*		pHeader,
				IN PTCP_SESSION			pSession
				);

NTSTATUS 
PROTO_ReceiveACKCLI(
				IN ftd_header_t*		pHeader,
				IN PTCP_SESSION			pSession
				);

NTSTATUS 
PROTO_ReceiveRSYNCDEVS(
				IN ftd_header_t*		pHeader,
				IN PTCP_SESSION			pSession
				);

NTSTATUS 
PROTO_ReceiveBFEND(
				IN ftd_header_t*		pHeader,
				IN PTCP_SESSION			pSession
				);

NTSTATUS 
PROTO_ReceiveACKRFSTART(
					IN ftd_header_t*	pHeader,
					IN PTCP_SESSION		pSession
					);

NTSTATUS 
PROTO_ReceiveACKCPSTART(
					IN ftd_header_t*	pHeader,
					IN PTCP_SESSION		pSession
					);

NTSTATUS 
PROTO_ReceiveACKCPSTOP(
					IN ftd_header_t*	pHeader,
					IN PTCP_SESSION		pSession
					);

NTSTATUS 
PROTO_ReceiveACKCPON(
				IN ftd_header_t*		pHeader,
				IN PTCP_SESSION			pSession
				);

NTSTATUS 
PROTO_ReceiveACKCPOFF(
				IN ftd_header_t*		pHeader,
				IN PTCP_SESSION			pSession
				);

NTSTATUS 
PROTO_ReceiveCPONERR(
				IN ftd_header_t*		pHeader,
				IN PTCP_SESSION			pSession
				);

NTSTATUS 
PROTO_ReceiveCPOFFERR(
				IN ftd_header_t*		pHeader,
				IN PTCP_SESSION			pSession
				);

NTSTATUS 
PROTO_ReceiveACKCHUNK(
				IN ftd_header_t*		pHeader,
				IN PTCP_SESSION			pSession
				);

NTSTATUS 
PROTO_ReceiveNOOP(
			IN ftd_header_t*			pHeader,
			IN PTCP_SESSION				pSession
			);

NTSTATUS 
PROTO_ReceiveACKHANDSHAKE(
					IN ftd_header_t*	pHeader,
					IN PTCP_SESSION		pSession
					);

NTSTATUS 
PROTO_ReceiveACKCONFIG(
					IN ftd_header_t*	pHeader,
					IN PTCP_SESSION		pSession
					);

NTSTATUS 
PROTO_ReceiveRFBLK(
				IN ftd_header_t*		pHeader , 
				IN PUCHAR				pDataBuffer , 
				IN ULONG				nDataLength , 
				IN PTCP_SESSION			pSession
				);

NTSTATUS 
PROTO_ReceiveACKERR(
				IN ftd_header_t*		pHeader , 
				IN PUCHAR				pDataBuffer , 
				IN ULONG				nDataLength , 
				IN PTCP_SESSION			pSession
				);

NTSTATUS 
PROTO_ReceiveCHKSUM(
				IN ftd_header_t*		pHeader , 
				IN PUCHAR				pDataBuffer , 
				IN ULONG				nDataLength , 
				IN PTCP_SESSION			pSession
				);

NTSTATUS 
PROTO_ReceiveBFBLK(
				IN ftd_header_t*		pHeader , 
				IN PUCHAR				pDataBuffer , 
				IN ULONG				nDataLength , 
				IN PTCP_SESSION			pSession
				);

NTSTATUS 
PROTO_ReceiveACKCHKSUM(
				IN ftd_header_t*		pHeader , 
				IN PUCHAR				pDataBuffer , 
				IN ULONG				nDataLength , 
				IN PTCP_SESSION			pSession
				);

NTSTATUS 
PROTO_ReceiveCHUNK(
				IN ftd_header_t*		pHeader , 
				IN PUCHAR				pDataBuffer , 
				IN ULONG				nDataLength , 
				IN PTCP_SESSION			pSession
				);

NTSTATUS 
PROTO_ReceiveVERSION(
				IN ftd_header_t*		pHeader , 
				IN PUCHAR				pDataBuffer , 
				IN ULONG				nDataLength , 
				IN PTCP_SESSION			pSession
				);

NTSTATUS 
PROTO_ReceiveHANDSHAKE(
					IN ftd_header_t*	pHeader , 
					IN PUCHAR			pDataBuffer , 
					IN ULONG			nDataLength , 
					IN PTCP_SESSION		pSession
					);

NTSTATUS 
PROTO_ReceiveCHKCONFIG(
					IN ftd_header_t*	pHeader , 
					IN PUCHAR			pDataBuffer , 
					IN ULONG			nDataLength , 
					IN PTCP_SESSION		pSession
					);

NTSTATUS 
PROTO_ReceiveACKVERSION1(
					IN ftd_header_t*	pHeader , 
					IN PUCHAR			pDataBuffer , 
					IN ULONG			nDataLength , 
					IN PTCP_SESSION		pSession
					);

NTSTATUS 
PROTO_CompressBuffer(
				IN PUCHAR		pInputBuffer , 
				IN ULONG		nInputLength , 
				OUT PUCHAR*		pOutBuffer ,
				OUT PULONG		pOutputLength , 
				IN LONG			algorithm
				);

NTSTATUS 
PROTO_DecompressBuffer(
					IN PUCHAR	pInputBuffer , 
					IN ULONG	nInputLength , 
					OUT PUCHAR* pOutBuffer , 
					OUT PULONG	pOutputLength , 
					IN LONG		algorithm
					);

NTSTATUS 
PROTO_CreateChecksum(
				IN PUCHAR	pInputBuffer , 
				IN ULONG	nInputLength , 
				OUT PUCHAR*	pOutBuffer , 
				OUT PULONG	pOutputLength , 
				IN LONG		algorithm
				);

BOOLEAN 
PROTO_CompareChecksum(
				IN PUCHAR	pInputBuffer1 , 
				IN PUCHAR	pInputBuffer2 , 
				IN ULONG	nDataLength , 
				IN LONG		algorithm
				);

BOOLEAN 
PROTO_IsAckCheckRequired(
						 IN ftd_header_t* pHeader
						 );

////////////////////////////////////////////////////////////////////
//These are the Commands and their ACK's that are sent and received.
////////////////////////////////////////////////////////////////////

NTSTATUS 
PROTO_Perform_Handshake(
					IN PSESSION_MANAGER pSessionManager
					);

// sends FTDCHUP command over to the other server
NTSTATUS 
PROTO_Send_HUP(
		IN PSESSION_MANAGER pSessionManger,
		IN LONG AckWanted
		);

// sends FTDACKHUP command over to the other server
NTSTATUS
PROTO_Send_ACKHUP( 
		IN int iDeviceID,
		IN PSESSION_MANAGER pSessionManger,
		IN LONG AckWanted
		);

// sends FTDCRFFSTART command over to the other server
NTSTATUS 
PROTO_Send_RFFSTART(
			IN PSESSION_MANAGER pSessionManger,
			IN LONG AckWanted
			);

// sends FTDACKRFSTART command over to the PRIMARY server
NTSTATUS 
PROTO_Send_ACKRFSTART( 
			IN PSESSION_MANAGER pSessionManger ,
			IN LONG AckWanted
			);

// sends FTDCRFFEND command over to the secondary server
NTSTATUS 
PROTO_Send_RFFEND(
		IN int iLgnum, 
		IN PSESSION_MANAGER pSessionManger,
		IN LONG AckWanted
		);

// sends FTDCCHKSUM command over to the secondary server
NTSTATUS 
PROTO_Send_CHKSUM( 
		IN  int iLgnumIN,
		IN  int iDeviceId, 
		IN  ftd_dev_t* ptrDeviceStructure,
		OUT PVOID* ptrDeltaMap,
		IN PSESSION_MANAGER pSessionManger,
		IN LONG AckWanted
		);

// sends FTDACKCHKSUM command over to the primary server
NTSTATUS 
PROTO_Send_ACKCHKSUM(
			IN  ftd_dev_t* ptrDeviceStructure,
			IN	int iDeltamap_len,
			IN	PVOID ptrDeltaMap,
			IN	PSESSION_MANAGER pSessionManger,
			IN LONG AckWanted
			);

// sends FTDACKCPSTART command over to the other server
NTSTATUS 
PROTO_Send_ACKCPSTART(
			IN PSESSION_MANAGER pSessionManger,
			IN LONG AckWanted
			);

// sends FTDACKCPSTOP command over to the other server
NTSTATUS 
PROTO_Send_ACKCPSTOP(
			IN PSESSION_MANAGER pSessionManger,
			IN LONG AckWanted
			);

// sends FTDACKCPONERR command over to the other server
NTSTATUS 
PROTO_Send_ACKCPONERR(
			IN PSESSION_MANAGER pSessionManger,
			IN LONG AckWanted
			);

// sends FTDACKCPOFFERR command over to the other server
NTSTATUS 
PROTO_Send_ACKCPOFFERR(
				IN PSESSION_MANAGER pSessionManger,
				IN LONG AckWanted
				);

// Is sent as part of Handshake mechanism when starting the PMD. 
// If RMD is not alive it is spawned. sends FTDCNOOP command over to the other server
NTSTATUS 
PROTO_Send_NOOP(
		IN PSESSION_MANAGER pSessionManger,
		IN LONG LGnum,
		IN LONG AckWanted
		);

// Is sent as part of Handshake mechanism when starting the PMD. 
// The API returns the Remote Version String. sends FTDCVERSION command over to the other server
NTSTATUS 
PROTO_Send_VERSION(IN int iLgnumIN,
				  IN PCHAR strConfigFile,
				  OUT PCHAR* strSecondaryVersion,
				  IN PSESSION_MANAGER pSessionManger, 
				  IN LONG AckWanted
				  );

static void
ftd_sock_encode_auth(ULONG ts, PCHAR hostname, ULONG hostid, ULONG ip,
            int *encodelen, PCHAR encode);

// Is sent as part of Handshake mechanism when starting the PMD. 
// The API returns STATUS_SUCCESS indicating whether the Secondary is in CP mode or not.
NTSTATUS 
PROTO_Send_HANDSHAKE(
			IN unsigned int nFlags,
			IN ULONG ulHostID,
			IN CONST PCHAR strConfigFilePath,
			IN CONST PCHAR strLocalHostName,
			IN ULONG ulLocalIP,
			OUT int* CP,
			IN PSESSION_MANAGER pSessionManger,
			IN LONG AckWanted
			);

// This ack is prepared and sent from the RMD in response to FTDCHANDSHAKE command
NTSTATUS 
PROTO_Send_ACKHANDSHAKE(
				IN unsigned int nFlags, 
				IN int iDeviceID, 
				IN PSESSION_MANAGER pSessionManger,
				IN LONG AckWanted
				);

// Is sent as part of Handshake mechanism when starting the PMD. 
// For each device in the group, one command is sent. If the remote side can not find the device 
// specified, an error is returned.
NTSTATUS 
PROTO_Send_CHKCONFIG(
			IN int iDeviceID,
			IN dev_t ulDevNum,
			IN dev_t ulFtdNum,
			IN CONST PCHAR strRemoteDeviceName,
			OUT PULONG ulRemoteDevSize,
			OUT PULONG ulDevId,
			IN PSESSION_MANAGER pSessionManger,
			IN LONG AckWanted
			);

// This ack is prepared and sent from the RMD in response to FTDCCHKCONFIG command
NTSTATUS 
PROTO_Send_ACKCONFIG(
			IN int iSize, 
			IN int iDeviceID, 
			IN PSESSION_MANAGER pSessionManger,
			IN LONG AckWanted
			);

#if 0 
NTSTATUS
PROTO_RecvVector_Version( 
					OUT PCHAR* strSecondaryVersion, 
					IN PSESSION_MANAGER pSessionManger
					);

NTSTATUS
PROTO_RecvVector_Handshake( 
						OUT int* CP, 
						IN PSESSION_MANAGER pSessionManger
						);

NTSTATUS
PROTO_RecvVector_CheckConfig( 
						OUT PULONG ulRemoteDevSize, 
						OUT PULONG ulDevId, 
						IN PSESSION_MANAGER pSessionManger
						);

NTSTATUS
PROTO_RecvVector_CheckSum( 
					OUT PVOID* ptrDeltaMap, 
					IN PSESSION_MANAGER pSessionManger
					);
#endif 
#if 1 // VEERA : Add This API
NTSTATUS
Proto_Compare_and_updateAckHdr( PSFTK_LG		Sftk_Lg,
								PMM_PROTO_HDR	DestProtoHdr,		// compare or update
								ftd_header_t	*AckProtoHdr,		// Ack Recieved Proto Hdr
								PVOID			RetBuffer,			// Optional, for Payload, Copy this to new allocated buffer
								ULONG			RetBufferSize );	// optional : Size of RetBuffer
#endif

#endif // _COMPROTO_H_