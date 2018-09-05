//tdiutil.h
#ifndef _SFTK_SESSION_MANAGER_H_
#define _SFTK_SESSION_MANAGER_H_

#define MAKETAG(a,b,c,d) ((ULONG)(d<<24|c<<16|b<<8|a))

#define MEM_TAG MAKETAG('Y','E','S','S')

#define CLIENT_SESSION	1
#define SERVER_SESSION	2

struct _SERVER_ELEMENT;
struct _RECEIVE_BUFFER;
struct _SEND_BUFFER;

struct _rplcc_iorp_;
typedef struct _rplcc_iorp_ rplcc_iorp_t;

struct ftd_lg_header_s;
typedef struct ftd_lg_header_s ftd_lg_header_t;

struct ftd_header_s;
typedef struct ftd_header_s ftd_header_t;

//This Packet holds the sent Parameters that will be hanging around in the Protocol Queue
//Until an ACK Comes back or a Time out happens and the Protocol Packtes are Discarded.



typedef struct _PROTOCOL_PACKET
{
	LIST_ENTRY ListElement;	//Element in the Linked List
	rplcc_iorp_t* pIoPacket;	//The IO Packet given by the Cache
	ftd_header_t ProtocolHeader;	//The Protocol header either given by the Cache or 
									//Given By the Out Of band Protocol Commands
	ftd_header_t ReceivedProtocolHeader;	//The Protocol header is receieved as reply to the
											//Sent Message

	PUCHAR pOutputBuffer; //The Output of the ACK Buffer.
						  //This Buffer is only discerned on calling the Receive function
						  //If there is no receive function, then this will be deleted 
						  //after a timeout value.
	LONG OutputBufferLenget;	//Length of the pOutputBuffer
	LARGE_INTEGER tStartTime; //This is the Start Time when the Packet is actually Sent
	LARGE_INTEGER tEndTime;	//This isthe time when it is actually received back.
	KEVENT ReceiveAckEvent;
} PROTOCOL_PACKET, *PPROTOCOL_PACKET;

#define PROTOCOL_PACKET_SIZE sizeof(PROTOCOL_PACKET)

#define PROTOCOL_HEADER_SIZE sizeof(ftd_header_t)



//This TCP_SESSION structure will be used by both the Server and the Client
typedef struct _TCP_SESSION
{
	LIST_ENTRY ListElement;	//This is a Double Linked List that will be used 
							//to traverse the Sessions.
	ULONG sessionID;	//The Unique Session id used to identify the session
	
	struct _SERVER_ELEMENT *pServer;

	USHORT bSessionEstablished;	//SFTK_DISCONNECTED | SFTK_CONNECTED | SFTK_INPROGRESS

//	KEVENT DisconnectEvent;	//A Notification Event that will be signalled in the
							//R/W thread when there is a disconnect or if there is 
							//a Error. Once this Event Happens the Read Write Thread Exits 
							//And Resets the Event. This Session is placed on the FreeList
							//Again, to try and Connect Again or Listen Again.


	SFTK_TDI_ENDPOINT       KSEndpoint;	//The TDI EndPoint Structure.
	TA_IP_ADDRESS     RemoteAddress;	//Remote Address to Connect to
	USHORT			 RemotePort;	//The Remote Port to which to connect to
	TDI_CONNECTION_INFORMATION	RemoteConnectionInfo;	//The Remote Connection Info

	//This is used if the Session is Listen
	PIRP              pListenIrp;	//The IRP that is used to listen to the Connects

	USHORT bReceiveStatus;	//SFTK_PROCESSING_RECEIVE_HEADER | SFTK_PROCESSING_RECEIVE_NOTINIT
	USHORT bSendStatus;	//SFTK_PROCESSING_SEND_DATA | SFTK_PROCESSING_SEND_NOTINIT
	KEVENT IOReceiveEvent;
	
	struct _RECEIVE_BUFFER* pReceiveHeader;	//The Receive Header
	struct _RECEIVE_BUFFER* pReceiveBuffer;	//The Receive the Rest of the Data
	struct _SEND_BUFFER* pSendBuffer;	//This is the Current Send Buffer

}TCP_SESSION, *PTCP_SESSION;

struct _SESSION_MANAGER;
struct _SEND_BUFFER_LIST;
struct _RECEIVE_BUFFER_LIST;

//These are for the Connect and Listen Lists

#define SFTK_DISCONNECTED 0		//The Session is initialized but not yet connected
#define SFTK_CONNECTED 1		//Connection is successfully established
#define SFTK_INPROGRESS 2		//When  the Connection is called for
#define SFTK_UNINITIALIZED 3	//This is when the sessions are not yet initialized

#define SFTK_PROCESSING_RECEIVE_NOTINIT 0
#define SFTK_PROCESSING_RECEIVE_HEADER 1
#define SFTK_PROCESSING_RECEIVE_DATA 2

#define SFTK_PROCESSING_SEND_NOTINIT 0
#define SFTK_PROCESSING_SEND_DATA 1

#define SFTK_MAGICVALUE 0xABABABAB

//These are for the Send and Receive Lists.

#define SFTK_BUFFER_FREE	0
#define SFTK_BUFFER_USED	1
#define SFTK_BUFFER_INUSE	2

//This Structure will be used for storing the Local Address and the Address Id
//This is Stored for all the same Local Address
//For Server this Local Address includes the IP Address and the Port
//For Client this Local Address include the IP Address and Port = 0.
//Optionally the Client IP Address could be INADDR_ANY
typedef struct _SERVER_ELEMENT
{
	LIST_ENTRY			ListElement;
	TA_IP_ADDRESS        LocalAddress;   // TDI Address
	SFTK_TDI_ADDRESS           KSAddress;  //Server Local Address
	struct _SESSION_MANAGER* pSessionManager;

	PTCP_SESSION	pSession;
	BOOLEAN bEnable;	//Set this flag to TRUE if you want it to be Enabled
	BOOLEAN bIsEnabled;	//This Flag will be TRUE if the Session is enabled,
						//The Session Could be Enabled but not Connected.
//	LIST_ENTRY	SessionListHead; //For Mutiple Sessions

	LIST_ENTRY	ServerListElement;
} SERVER_ELEMENT, *PSERVER_ELEMENT;

typedef struct _SEND_BUFFER
{
	LIST_ENTRY		    ListElement;
	PMDL				pSendMdl;		//The MDL that is used to send the data
	unsigned char*		pSendBuffer;	//Pointer to the TCP Window in LONG pointer
//	ULONG				SendWindow;	//Size of the TCP Window in LONG.
	IO_STATUS_BLOCK		IoSendStatus;		//Returns the status of the Session Connection
//	FAST_MUTEX			mReadLock;		//Protects Multiple Thread Calls to ReadCache()
	struct _SEND_BUFFER_LIST* pSendList;
	USHORT				index;
	USHORT				state;
	//This is for testing
	PIRP				pSendIrp;
	ULONG TotalSendLength;	//This is the Total send size of the Buffer
	ULONG ActualSendLength;	//This is the actual send Offset.

	rplcc_iorp_t* pIoPacket;	//The IO Packet given by the Cache
	PTCP_SESSION pSession;

}SEND_BUFFER, *PSEND_BUFFER;

typedef struct _RECEIVE_BUFFER
{
	LIST_ENTRY		  ListElement;
//	ULONG             ReceiveWindow;	//The Size of the Receiving TDI Window
	unsigned char*	  pReceiveBuffer;	//The actual TDI Window Buffer
	PMDL              pReceiveMdl;	//The MDL that will be used to receive Data
	IO_STATUS_BLOCK   IoReceiveStatus;	//The Status of the Connection
//	FAST_MUTEX		  mWriteLock;	//The Lock that is used to write from different 
									//Server Sessions to the Same File or Buffer
									//This is a point of Synchronization used for 
									//making the Data to come in Sequence
   									//This is for a future release 
	struct _RECEIVE_BUFFER_LIST* pReceiveList;
	USHORT			index;
	USHORT				state;
	//This is for testing
	PIRP				pReceiveIrp;
	ULONG TotalReceiveLength;	//This is the Total Data that needs tobe received
	ULONG ActualReceiveLength;	//This is the actual received data

	PTCP_SESSION pSession;
}RECEIVE_BUFFER, *PRECEIVE_BUFFER;

typedef struct _SEND_BUFFER_LIST
{
	ULONG SendWindow;	//Size of the TCP Window in Bytes.
	USHORT MaxNumberOfSendBuffers;	//The Max number of Send Buffers used
	USHORT NumberOfSendBuffers;		//The Actual number of Send Buffers used
	LIST_ENTRY FreeSessionList;		//The Send Session List
	struct _SESSION_MANAGER* pSessionManager;	//Pointer to the Parent
	ULONG ChunkSize;		//This is the Value used to form PAckts and send accross the wire
}SEND_BUFFER_LIST, *PSEND_BUFFER_LIST;

typedef struct _RECEIVE_BUFFER_LIST
{
	ULONG ReceiveWindow;	//The Size of the Receiving TDI Window in Bytes
	USHORT MaxNumberOfReceiveBuffers;
	USHORT NumberOfReceiveBuffers;
	LIST_ENTRY FreeSessionList;
	struct _SESSION_MANAGER* pSessionManager;
}RECEIVE_BUFFER_LIST, *PRECEIVE_BUFFER_LIST;


//This is the List Of Connections Either Clients or Servers.
typedef struct _SESSION_MANAGER
{
	BOOLEAN bInitialized;			//This indicates if the Session Manager is initialized
									//or not
	KEVENT GroupConnectedEvent;		//This event is Signalled if atleast one Connection 
									//Exists
	KEVENT IOExitEvent;				//The Exit Event used for Send/Receive Threads
	KEVENT GroupUninitializedEvent;	//The Stop Event used for Connect/Listen Threads
	LONG nLiveSessions;			//The Total Number of Live Sessions
	LIST_ENTRY ServerList;		//The List of Servers
	PFILE_OBJECT pConnectListenThread;	//This is the Object Pointer to Connect/Listen Thread
	PCONNECTION_DETAILS lpConnectionDetails;	//Added Connection Details Here
	PFILE_OBJECT pReceiveThreadObject;	//This is the Object Pointer to the Thread.
	PFILE_OBJECT pSendThreadObject;	//This is the Object Pointer to the Receive Thread.
	SEND_BUFFER_LIST sendBufferList;	//The Send Buffer List
	RECEIVE_BUFFER_LIST receiveBufferList;		//The Receive Buffer List

	ERESOURCE ServerListLock;	//This will Protect against the Session Additions
								//and Removals
	NPAGED_LOOKASIDE_LIST ProtocolList;	//This is the Memory Lookaside list that will hold
										//the Protocol headers.
	LIST_ENTRY ProtocolQueue; //This is the actual Queue of the Protocol headers 
							  //that are sent.
	KSPIN_LOCK ProtocolQueueLock;	//This protects the ProtocolQueue
	FAST_MUTEX SendLock;	//This will be used to exclusive access to the Send Thread
	FAST_MUTEX ReceiveLock;	//This will be used for exclusive access to the receive Thread

	//Tunable Parameters are spceified here

	ULONG nSendWindowSize;	//The size of the Send Window
	ULONG nReceiveWindowSize;	//The Size of the Receive Window
	ULONG nChunkSize;	//The Size of each Chunk when sending data over the wire.
}SESSION_MANAGER, *PSESSION_MANAGER;


typedef enum {CONNECT = 1 , ACCEPT = 2} eSessionType;

//Thread Functions

NTSTATUS CreateSendThread(PVOID lpContext);
NTSTATUS CreateReceiveThread(PVOID lpContext);




LONG InitializeServer(PSERVER_ELEMENT *ppServer,PCONNECTION_INFO pConnectionInfo);
LONG InitializeSession(PTCP_SESSION *ppSession, PSERVER_ELEMENT pServer, PCONNECTION_INFO pConnectionInfo,LONG nSendWindowSize, LONG nReceiveWindowSize, eSessionType type);
LONG InitializeConnectThread(PSESSION_MANAGER pSessionManager);
LONG InitializeListenThread(PSESSION_MANAGER pSessionManager);
NTSTATUS CreateConnectThread(PSESSION_MANAGER pSessionManager);
NTSTATUS CreateListenThread(PSESSION_MANAGER pSessionManager);
void UninitializeSession(PTCP_SESSION pSession);
void UninitializeServer(PSERVER_ELEMENT pServer);
void UninitializeConnectListenThread(PSESSION_MANAGER pSessionManager);
void CleanUpSession(PTCP_SESSION pSession);
PSERVER_ELEMENT FindServerElement(PSESSION_MANAGER pSessionManager, PCONNECTION_INFO pConnectionInfo);

NTSTATUS CreateSendThread1(PSESSION_MANAGER pSessionManager);
NTSTATUS CreateReceiveThread1(PSESSION_MANAGER pSessionManager);
VOID StopSendReceiveThreads(PSESSION_MANAGER pSessionManager);
PSEND_BUFFER GetNextSendBuffer(PSEND_BUFFER_LIST pSendList);
PRECEIVE_BUFFER GetNextReceiveBuffer(PRECEIVE_BUFFER_LIST pReceiveList);
NTSTATUS CreateSendReceiveThreads(PSESSION_MANAGER pSessionManager);
void InitializeSendBufferList(PSEND_BUFFER_LIST pSendList,PCONNECTION_DETAILS pConnectionDetails);
void InitializeReceiveBufferList(PRECEIVE_BUFFER_LIST pReceiveList,PCONNECTION_DETAILS pConnectionDetails);
PRECEIVE_BUFFER GetNextStaleReceiveBuffer(PRECEIVE_BUFFER_LIST pReceiveList);
PSEND_BUFFER GetNextStaleSendBuffer(PSEND_BUFFER_LIST pSendList);
void ClearSendBuffer(PSEND_BUFFER pSendBuffer);
void ClearReceiveBuffer(PRECEIVE_BUFFER pReceiveBuffer);
void ClearSendBufferList(PSEND_BUFFER_LIST pSendList);
void ClearReceiveBufferList(PRECEIVE_BUFFER_LIST pReceiveList);
void CleanUpServer(PSERVER_ELEMENT pServer);

NDIS_STATUS GetSessionReceiveBuffer(PTCP_SESSION pSession);

typedef struct _SFTK_IOVEC_
{
	PVOID pBuffer;
	ULONG nLength;
}SFTK_IOVEC;

////////////////////////////////////////////////////////////////////
//These are the Commands and their ACK's that are sent and received.
////////////////////////////////////////////////////////////////////

// sends FTDCHUP command over to the other server
NTSTATUS Send_HUP(
IN PSESSION_MANAGER pSessionManger
);

// sends FTDACKHUP command over to the other server
NTSTATUS Send_ACKHUP( 
IN int iDeviceID,
IN PSESSION_MANAGER pSessionManger
);

// sends FTDCRFFSTART command over to the other server
NTSTATUS Send_RFFSTART(
IN PSESSION_MANAGER pSessionManger
);

// sends FTDACKRFSTART command over to the PRIMARY server
NTSTATUS Send_ACKRFSTART( 
IN PSESSION_MANAGER pSessionManger 
);

// sends FTDCRFFEND command over to the secondary server
NTSTATUS Send_RFFEND(
IN int iLgnum, 
IN PSESSION_MANAGER pSessionManger
);

// sends FTDACKCPSTART command over to the other server
NTSTATUS Send_ACKCPSTART(
IN PSESSION_MANAGER pSessionManger
);

// sends FTDACKCPSTOP command over to the other server
NTSTATUS Send_ACKCPSTOP(
IN PSESSION_MANAGER pSessionManger
);

// sends FTDACKCPONERR command over to the other server
NTSTATUS Send_ACKCPONERR(
IN PSESSION_MANAGER pSessionManger
);

// sends FTDACKCPOFFERR command over to the other server
NTSTATUS Send_ACKCPOFFERR(
IN PSESSION_MANAGER pSessionManger
);

// Is sent as part of Handshake mechanism when starting the PMD. 
// If RMD is not alive it is spawned. sends FTDCNOOP command over to the other server
NTSTATUS Send_NOOP(
IN PSESSION_MANAGER pSessionManger
);

// Is sent as part of Handshake mechanism when starting the PMD. 
// The API returns the Remote Version String. sends FTDCVERSION command over to the other server
NTSTATUS Send_VERSION(IN int iLgnumIN, 
IN CONST PCHAR strVersion,
OUT CONST PCHAR strSecondaryVersion,
IN PSESSION_MANAGER pSessionManger
);

// Is sent as part of Handshake mechanism when starting the PMD. 
// The API returns STATUS_SUCCESS indicating whether the Secondary is in CP mode or not.
NTSTATUS Send_HANDSHAKE(IN unsigned int nFlags,
					  IN ULONG ulHostID,
					  IN CONST PCHAR strConfigFilePath,
					  IN CONST PCHAR strLocalHostName,
					  IN ULONG ulLocalIP,
					  OUT int CP,
					  IN PSESSION_MANAGER pSessionManger);

// This ack is prepared and sent from the RMD in response to FTDCHANDSHAKE command
NTSTATUS Send_ACKHANDSHAKE(IN unsigned int nFlags, IN int iDeviceID, IN PSESSION_MANAGER pSessionManger);

// Is sent as part of Handshake mechanism when starting the PMD. 
// For each device in the group, one command is sent. If the remote side can not find the device 
// specified, an error is returned.
NTSTATUS Send_CHKCONFIG(IN int iDeviceID,
					  IN dev_t ulDevNum,
					  IN dev_t ulFtdNum,
					  IN CONST PCHAR strRemoteDeviceName,
					  OUT PULONG ulRemoteDevSize,
					  OUT PULONG ulDevId,
					  IN PSESSION_MANAGER pSessionManger);


NTSTATUS SftkSendVector(SFTK_IOVEC vectorArray[] , LONG len , PSESSION_MANAGER pSessionManager);

NTSTATUS SftkProcessReceiveHeader(ftd_header_t* pHeader, PULONG pLen, PTCP_SESSION pSession);
NTSTATUS SftkProcessReceiveData(IN ftd_header_t* pHeader , IN PUCHAR pDataBuffer , IN ULONG nDataLength , OUT PULONG pLen , IN PTCP_SESSION pSession);

NTSTATUS SftkReceiveHUP(IN ftd_header_t* pHeader , IN PTCP_SESSION pSession);
NTSTATUS SftkReceiveACKHUP(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession);
NTSTATUS SftkReceiveRFFSTART(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession);
NTSTATUS SftkReceiveACKRSYNC(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession);
NTSTATUS SftkReceiveRFFEND(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession);
NTSTATUS SftkReceiveBFSTART(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession);
NTSTATUS SftkReceiveACKCLI(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession);
NTSTATUS SftkReceiveRSYNCDEVS(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession);
NTSTATUS SftkReceiveBFEND(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession);
NTSTATUS SftkReceiveACKRFSTART(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession);
NTSTATUS SftkReceiveACKCPSTART(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession);
NTSTATUS SftkReceiveACKCPSTOP(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession);
NTSTATUS SftkReceiveACKCPON(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession);
NTSTATUS SftkReceiveACKCPOFF(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession);
NTSTATUS SftkReceiveCPONERR(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession);
NTSTATUS SftkReceiveCPOFFERR(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession);
NTSTATUS SftkReceiveACKCHUNK(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession);
NTSTATUS SftkReceiveNOOP(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession);
NTSTATUS SftkReceiveACKHANDSHAKE(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession);
NTSTATUS SftkReceiveACKCONFIG(IN ftd_header_t* pHeader,IN PTCP_SESSION pSession);

NTSTATUS SftkReceiveRFBLK(IN ftd_header_t* pHeader , IN PUCHAR pDataBuffer , IN ULONG nDataLength , IN PTCP_SESSION pSession);
NTSTATUS SftkReceiveACKERR(IN ftd_header_t* pHeader , IN PUCHAR pDataBuffer , IN ULONG nDataLength , IN PTCP_SESSION pSession);
NTSTATUS SftkReceiveCHKSUM(IN ftd_header_t* pHeader , IN PUCHAR pDataBuffer , IN ULONG nDataLength , IN PTCP_SESSION pSession);
NTSTATUS SftkReceiveBFBLK(IN ftd_header_t* pHeader , IN PUCHAR pDataBuffer , IN ULONG nDataLength , IN PTCP_SESSION pSession);
NTSTATUS SftkReceiveACKCHKSUM(IN ftd_header_t* pHeader , IN PUCHAR pDataBuffer , IN ULONG nDataLength , IN PTCP_SESSION pSession);
NTSTATUS SftkReceiveCHUNK(IN ftd_header_t* pHeader , IN PUCHAR pDataBuffer , IN ULONG nDataLength , IN PTCP_SESSION pSession);
NTSTATUS SftkReceiveVERSION(IN ftd_header_t* pHeader , IN PUCHAR pDataBuffer , IN ULONG nDataLength , IN PTCP_SESSION pSession);
NTSTATUS SftkReceiveHANDSHAKE(IN ftd_header_t* pHeader , IN PUCHAR pDataBuffer , IN ULONG nDataLength , IN PTCP_SESSION pSession);
NTSTATUS SftkReceiveCHKCONFIG(IN ftd_header_t* pHeader , IN PUCHAR pDataBuffer , IN ULONG nDataLength , IN PTCP_SESSION pSession);
NTSTATUS SftkReceiveACKVERSION1(IN ftd_header_t* pHeader , IN PUCHAR pDataBuffer , IN ULONG nDataLength , IN PTCP_SESSION pSession);

NTSTATUS SftkCompressBuffer(IN PUCHAR pInputBuffer , IN ULONG nInputLength , OUT PUCHAR* pOutBuffer ,OUT PULONG pOutputLength , IN LONG algorithm);
NTSTATUS SftkDecompressBuffer(IN PUCHAR pInputBuffer , IN ULONG nInputLength , OUT PUCHAR* pOutBuffer , OUT PULONG pOutputLength , IN LONG algorithm);
NTSTATUS SftkCreateChecksum(IN PUCHAR pInputBuffer , IN ULONG nInputLength , OUT PUCHAR* pOutBuffer , OUT PULONG pOutputLength , IN LONG algorithm);
NTSTATUS SftkCompareChecksum(IN PUCHAR pInputBuffer1 , IN PUCHAR pInputBuffer2 , IN ULONG nDataLength , IN LONG algorithm);


NTSTATUS FindConnectionsCount(IN PSESSION_MANAGER pSessionManager ,IN PCONNECTION_DETAILS pConnectionDetails , IN BOOLEAN bEnabled, OUT PULONG pServerCount, OUT PLIST_ENTRY pServerList);
NTSTATUS AddConnections(PSESSION_MANAGER pSessionManager ,PCONNECTION_DETAILS pConnectionDetails);
NTSTATUS EnableConnections(PSESSION_MANAGER pSessionManager ,PCONNECTION_DETAILS pConnectionDetails, BOOLEAN bEnable);
NTSTATUS RemoveConnections(PSESSION_MANAGER pSessionManager ,PCONNECTION_DETAILS pConnectionDetails);
VOID FreeConnections(PLIST_ENTRY pServerList);
NTSTATUS AllocateServer(PSERVER_ELEMENT *ppServer,PCONNECTION_INFO pConnectionInfo);
VOID FreeServer(PSERVER_ELEMENT pServer);
NTSTATUS AllocateSession(PTCP_SESSION *ppSession, PSERVER_ELEMENT pServer, PCONNECTION_INFO pConnectionInfo);
VOID FreeSession(PTCP_SESSION pSession);

NTSTATUS InitializeServer1(PSERVER_ELEMENT pServer, eSessionType type);
LONG InitializeSession1(PTCP_SESSION pSession, eSessionType type);
void UninitializeSession1(PTCP_SESSION pSession);
void UninitializeServer1(PSERVER_ELEMENT pServer);


//These are the new threads that will be used along with the InitialzeSessionManager()
LONG InitializeSessionManager(PSESSION_MANAGER pSessionManager);
NTSTATUS SendThread2(PSESSION_MANAGER pSessionManager);
NTSTATUS ReceiveThread2(PSESSION_MANAGER pSessionManager);
NTSTATUS ListenThread2(PSESSION_MANAGER pSessionManager);
NTSTATUS ConnectThread2(PSESSION_MANAGER pSessionManager);



#endif //_SFTK_SESSION_MANAGER_H_