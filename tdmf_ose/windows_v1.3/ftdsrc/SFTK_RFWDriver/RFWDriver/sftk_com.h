/**************************************************************************************

Module Name: sftk_com.h   
Author Name: Veera Arja
Description: Describes Modules: Com, OsTdi,Proto Structures and macro definations 
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2004 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/

#ifndef _SFTK_COM_H_
#define _SFTK_COM_H_

//
// Include Header File 
//
#include <Tdi\Sftk_TdiUtil.h>

//
// Macro Definations
//

#define MAKETAG(a,b,c,d)					((ULONG)(d<<24|c<<16|b<<8|a))

#define MEM_TAG								MAKETAG('Y','E','S','S')

#define DEFAULT_CACHE_SIZE					16		// This is in MB

#define DEFAULT_TCP_WINDOW_SIZE				4		// This is the TCP Window Size in MB 

#define DEFAULT_CACHE_SIZE_IN_LONG			1048576  // This is the size of Cache in Number of uLongs

#define LONGSHIFT							2

#define CLIENT_SESSION						1
#define SERVER_SESSION						2

//
// Structure protype pre declaration
//
struct _SERVER_ELEMENT;
struct _RECEIVE_BUFFER;
struct _SEND_BUFFER;

//
// Enum Definations
//
typedef enum eSessionType
{	CONNECT		= 1 , 
	ACCEPT		= 2
} eSessionType, *PeSessionType;

#define PROTOCOL_HEADER_SIZE				sizeof(ftd_header_t)

// This TCP_SESSION structure will be used by both the Server and the Client
typedef struct _TCP_SESSION
{
	LIST_ENTRY								ListElement;			// This is a Double Linked List that will be used 
																	// to traverse the Sessions.

	ULONG									sessionID;				// The Unique Session id used to identify the session
	
	struct _SERVER_ELEMENT					*pServer;				// Back Pointer to the Parent

	USHORT									bSessionEstablished;	// SFTK_DISCONNECTED | SFTK_CONNECTED | SFTK_INPROGRESS

	TDI_ENDPOINT							KSEndpoint;				// The TDI EndPoint Structure.

	TA_IP_ADDRESS							RemoteAddress;			// Remote Address to Connect to

	USHORT									RemotePort;				// The Remote Port to which to connect to

	TDI_CONNECTION_INFORMATION				RemoteConnectionInfo;	// The Remote Connection Info

	PIRP									pConnectIrp;			// The IRP that is used to connect to a remote side

	//This is used if the Session is Listen

	PIRP									pListenIrp;				// The IRP that is used to listen to the Connects

	USHORT									bReceiveStatus;			// SFTK_PROCESSING_RECEIVE_HEADER | SFTK_PROCESSING_RECEIVE_NOTINIT

	USHORT									bSendStatus;			// SFTK_PROCESSING_SEND_DATA | SFTK_PROCESSING_SEND_NOTINIT

	KEVENT									IOReceiveEvent;			// 

	struct _RECEIVE_BUFFER*					pReceiveHeader;			// The Receive Header

	struct _RECEIVE_BUFFER*					pReceiveBuffer;			// The Receive the Rest of the Data

	struct _SEND_BUFFER*					pSendBuffer;			// This is the Current Send Buffer

	LIST_ENTRY								sendIrpList;			// This will have all the List Element 
																	// SessionListElemet of SEND_BUFFER
																	// It represents all the irps currently 
																	// pending on the send Queue.

	KSPIN_LOCK								sendIrpListLock;			// Used to protect the sendIrpList


}TCP_SESSION, *PTCP_SESSION;

struct _SESSION_MANAGER;
struct _SEND_BUFFER_LIST;
struct _RECEIVE_BUFFER_LIST;

//These are for the Connect and Listen Lists

#define SFTK_DISCONNECTED					0	// The Session is initialized but not yet connected

#define SFTK_CONNECTED						1	// Connection is successfully established

#define SFTK_INPROGRESS						2	// When  the Connection is called for

#define SFTK_UNINITIALIZED					3	// This is when the sessions are not yet initialized

#define SFTK_PROCESSING_RECEIVE_NOTINIT		0
#define SFTK_PROCESSING_RECEIVE_HEADER		1
#define SFTK_PROCESSING_RECEIVE_DATA		2

#define SFTK_PROCESSING_SEND_NOTINIT		0
#define SFTK_PROCESSING_SEND_DATA			1


//These are for the Send and Receive Lists.

#define SFTK_BUFFER_FREE					0
#define SFTK_BUFFER_USED					1
#define SFTK_BUFFER_INUSE					2

typedef struct _SERVER_PERFORMANCE_INFO
{
	UINT64		nPacketsSent;					// The total TDI Packets Sent

	UINT64		nBytesSent;						// The total Bytes Sent in those Packets

	UINT64		nPacketsReceived;				// The total TDI Packets Received

	UINT64		nBytesReceived;					// The total Bytes Received

    ULONG		nAverageSendPacketSize;			// Average Send Packet Size.

    ULONG		nMaximumSendPacketSize;		// Maximum send Packet Size.

	ULONG		nMinimumSendPacketSize;			// Average Send Packet Size.

    ULONG		nAverageReceivedPacketSize;		// The Average Received Packet Size.

    ULONG		nMaximumReceivedPacketSize;	// Maximum Received packet Size.

	ULONG		nMinimumReceivedPacketSize;		// Minimum Received Packet Size.

	UINT64		nAverageSendDelay;				// estimated delay on this connection.

	UINT64		nEffectiveBytesSent;			// Effective Bytes Sent along with the compressed data

	UINT64		Throughput;						// estimated throughput on this connection.

}SERVER_PERFORMANCE_INFO, *PSERVER_PERFORMANCE_INFO;

//This Structure will be used for storing the Local Address and the Address Id
//This is Stored for all the same Local Address
//For Server this Local Address includes the IP Address and the Port
//For Client this Local Address include the IP Address and Port = 0.
//Optionally the Client IP Address could be INADDR_ANY
typedef struct _SERVER_ELEMENT
{
	LIST_ENTRY								ListElement;					// List Element to the ServerList of SESSION_MANAGER

	ULONG									nServerId;						// The Unique Id of this Server, this is nothing 
																			// but the Hex Value of the Pointer of the EndPoint 
																			// File Object


	TA_IP_ADDRESS							LocalAddress;					// TDI Address

	TDI_ADDRESS								KSAddress;						// Server Local Address

	struct _SESSION_MANAGER*				pSessionManager;				// Back pointer to the Parent

	PTCP_SESSION							pSession;						// The Enpoint Connection Session

	BOOLEAN									bEnable;						// Set this flag to TRUE if you want it to be Enabled

	BOOLEAN									bIsEnabled;						// This Flag will be TRUE if the Session is enabled,
																			// The Session Could be Enabled but not Connected.

	BOOLEAN									bReset;							// Stops and starts the connection. This is used as error 
																			// Recovery techniuqe whenever there is a problem with the
																			// Send receive.

//	LIST_ENTRY								SessionListHead;				// For Mutiple Sessions

	LIST_ENTRY								ServerListElement;				//	List Element used in the COM_FindConnections()

//	FAST_MUTEX								SessionSendLock;				// This will be used to exclusive access to the Send Thread per Session

//	FAST_MUTEX								SessionReceiveLock;				// This will be used for exclusive access to the receive Thread per Session

	PFILE_OBJECT							pSessionReceiveThreadObject;	// This is the Object Pointer to the Send Thread for this server.

	PFILE_OBJECT							pSessionSendThreadObject;		// This is the Object Pointer to the Receive Thread for this server.

	KEVENT									SessionIOExitEvent;				// This Event is signalled incase of Exiting the Send and Receive Threads 
																			// for this Session

	SERVER_PERFORMANCE_INFO					SessionPerformanceInformation;	// Provides the Statistice about the connection, like Number of Bytes Sent etc..

	LARGE_INTEGER							LastSentTime;					// The Time in 100 Nanoseconds when some data is sent

	LARGE_INTEGER							LastReceiveTime;				// The Time in 100 Nanoseconds when some data is received

} SERVER_ELEMENT, *PSERVER_ELEMENT;

typedef struct _SEND_BUFFER
{
	LIST_ENTRY								ListElement;		// List Element to the SEND_BUFFER_LIST

	LIST_ENTRY								SessionListElemet;	// List Element for the TCP_SESSION

	LIST_ENTRY								MmHolderList;		// Holds the Head of the MM_HOLDER list. Used incase of Direct Memory Usage
																// if otherwise this will not be used.

	PMDL									pSendMdl;			// The MDL that is used to send the data
																// This also Represents the Root MDL incase of using MM_HOLDER List.

	PUCHAR									pSendBuffer;		// Pointer to the TCP Window in LONG pointer

	IO_STATUS_BLOCK							IoSendStatus;		// Returns the status of the Session Connection

	struct _SEND_BUFFER_LIST*				pSendList;			// Back Pointer to the Parent

	ULONG									index;				// The Index of this Send Buffer

	USHORT									state;				// The Current state of the Send Buffer

	PIRP									pSendIrp;			// The IRP used to Send Data 

	ULONG									nCancelFlag;		// Flag setting the Cancelation status

	ULONG									TotalSendLength;	// This is the Total send size of the Buffer

	ULONG									ActualSendLength;	// This is the actual send Offset.

	PTCP_SESSION							pSession;			// The Session that is currently processing this SEND_BUFFER

	BOOLEAN									bPhysicalMemory;	// If this Flag is set it means that user has called Send_Buffer() 
																// directely and the Buffer at allocated from the System directly
																// Instead of depending on the Stale Buffer's.
																// The Completion routinue has to free up this buffer and also free 
																// up the IRP.
																// This Falg is always set when using MmHolderList.

	ULONG									SendWindow;			// Size of the Send Window in Bytes.

}SEND_BUFFER, *PSEND_BUFFER;


typedef struct _RECEIVE_BUFFER
{
	LIST_ENTRY								ListElement;			// The List Element used for RECEIVE_BUFFER_LIST

	PUCHAR									pReceiveBuffer;			// The actual TDI Window Buffer

	PMDL									pReceiveMdl;			// The MDL that will be used to receive Data

	IO_STATUS_BLOCK							IoReceiveStatus;		// The status of the Connection

	struct _RECEIVE_BUFFER_LIST*			pReceiveList;			// Back Pointer to the Parent

	ULONG									index;					// Index of the Received Buffer

	USHORT									state;					// The current state of the received Buffer

	PIRP									pReceiveIrp;			// The IRP used to Receive the Data 

	ULONG									nCancelFlag;			// Flag setting the Cancelation status

	ULONG									TotalReceiveLength;		// This is the Total Data that needs tobe received

	ULONG									ActualReceiveLength;	// This is the actual received data

	PTCP_SESSION							pSession;				// The Session that is currently processing this Buffer
	
	ULONG									ReceiveWindow;			// The Size of the Receiving Receive Window in Bytes

	ULONG									ActualReceiveWindow;	// The Modifiable Window Size Paramater that will be Varied whenever
																	// We want to change the Receive Window Size

}RECEIVE_BUFFER, *PRECEIVE_BUFFER;

typedef struct _SEND_BUFFER_LIST
{
	ULONG									SendWindow;				// Size of the Send Window in Bytes.

	USHORT									MaxNumberOfSendBuffers;	// The Max number of Send Buffers used

	USHORT									NumberOfSendBuffers;	// The Actual number of Send Buffers used

//	FAST_MUTEX								SendBufferListLock;		// This will be used to exclusive access to the SendBufferList

	KSPIN_LOCK								SendBufferListLock;		// This will be used to exclusive access to the SendBufferList

	LIST_ENTRY								SendBufferList;			// The Send Session List

	struct _SESSION_MANAGER*				pSessionManager;		// Pointer to the Parent

	ULONG									LastBufferIndex;		// This is used to store the Allocated Buffer index that will be 
																	// incremented as needed whenver a new buffer is requested and will
																	// be assigned to the SEND_BUFFER index

}SEND_BUFFER_LIST, *PSEND_BUFFER_LIST;

typedef struct _RECEIVE_BUFFER_LIST
{
	ULONG									ReceiveWindow;				// The Size of the Receiving Receive Window in Bytes

	USHORT									MaxNumberOfReceiveBuffers;

	USHORT									NumberOfReceiveBuffers;

//	FAST_MUTEX								ReceiveBufferListLock;		// This will be used for exclusive access to the ReceiveBufferList

	KSPIN_LOCK								ReceiveBufferListLock;		// This will be used for exclusive access to the ReceiveBufferList

	LIST_ENTRY								ReceiveBufferList;

	struct _SESSION_MANAGER*				pSessionManager;

}RECEIVE_BUFFER_LIST, *PRECEIVE_BUFFER_LIST;

struct SFTK_LG;

//This is the List Of Connections Either Clients or Servers.
typedef struct _SESSION_MANAGER
{
	BOOLEAN									bInitialized;				// This indicates if the Session Manager is initialized
																		// or not

	struct SFTK_LG							*pLogicalGroupPtr;			// Back Pointer to the Logical Group

	KEVENT									GroupConnectedEvent;		// This event is Signalled if atleast one Connection 
																		// Exists

	KEVENT									GroupDisconnectedEvent;		// This event is Signalled when all the connections are gone. 

	KEVENT									IOExitEvent;				// The Exit Event used for Send/Receive Threads

	KEVENT									GroupUninitializedEvent;	// The Stop Event used for Connect/Listen Threads

	LONG									nTotalSessions;				// The Total Number of Sessions

	LONG									nLiveSessions;				// The Total Number of Live Sessions

	LIST_ENTRY								ServerList;					// The List of Servers

	PFILE_OBJECT							pConnectListenThread;		// This is the Object Pointer to Connect/Listen Thread

	PCONNECTION_DETAILS						lpConnectionDetails;		// Added Connection Details Here

	PFILE_OBJECT							pReceiveThreadObject;		// This is the Object Pointer to the Thread.

	PFILE_OBJECT							pSendThreadObject;			// This is the Object Pointer to the Receive Thread.

	SEND_BUFFER_LIST						sendBufferList;				// The Send Buffer List

	RECEIVE_BUFFER_LIST						receiveBufferList;			// The Receive Buffer List

	ERESOURCE								ServerListLock;				// This will Protect against the Session Additions
																		//and Removals

	LIST_ENTRY								ProtocolQueue;				// This is the actual Queue of the Protocol headers 
																		// that are sent.

//	KSPIN_LOCK								ProtocolQueueLock;			// This protects the ProtocolQueue

//	FAST_MUTEX								SendLock;					// This will be used to exclusive access to the Send Thread

//	FAST_MUTEX								ReceiveLock;				// This will be used for exclusive access to the receive Thread

	KSPIN_LOCK								StatisticsLock;				// This protects Statistics count Locking

	BOOLEAN									bSendHandshakeInformation;	// Specifies whether the Handshake information needs to be
																		// Sent or not.

	ULONG									LastPacketSentTime;			// The time in Seconds when the last Packet was sent, 
																		// this is useful incase of sending the NOOP Packet

	ULONG									LastPacketReceiveTime;		// The time in Seconds when the last Packet was received, 
																		// this is useful to check for HeartBeat and see if the connection
																		// is alive or not.
	BOOLEAN									Reset;

	BOOLEAN									Stop;

	KEVENT									HandshakeEvent;				// This event is used to signal whenever Handshake is complete.

	LONG									ConnectionType;				// Specifies This is Either Connect or Listen Type.

	KEVENT									IOSendPacketsAvailableEvent; // Signals when there are SEND_BUFFER Packets available

	BOOLEAN									UseMemoryManager;			// This indicates that all the sessions shouldnt allocate memory for the
																		// Send and Receive Buffers but should use the Memory Manager Support for 
																		// these Allocations. We still Allocate SEND_BUFFER Structure directly from 
																		// NonPagedPool. Default is FALSE;

	// All these members will be used in a Syncronous mode.
	// The Caller of the Send will Wait until this event is signalled, 
	// So only one Caller can wait on the Event, Only one Sync Packet is supported
	// The Event will be reset by the Send Function Once the Wait is done.
	// The Event is Synchronous Event.

	PKEVENT									pSyncReceivedEvent;			// This event is used only for Synchronous Wait for Received ACK.

	ftd_header_t							ReceivedProtocolHeader;		// The Protocol header is receieved as reply to the
																		// Sent Message

	PUCHAR									pOutputBuffer;				// The Output of the ACK Buffer.
																		// This Buffer is only discerned on calling the Receive function
																		// If there is no receive function, then this will be deleted 
																		// after a timeout value.

	LONG									OutputBufferLenget;			// Length of the pOutputBuffer


	//Tunable Parameters are spceified here

	ULONG									nChunkSize;					// The Size of each Chunk when sending data over the wire.

	ULONG									nChunkDelay;				// This is the Throttle Delay that is applied to the network

} SESSION_MANAGER, *PSESSION_MANAGER;


// Macros Related to the Communication Module
// This Macro defines if Atleast one session is connected or not.

#define COM_IsLGConnected(pSessionManager)	( ( ( ( PSESSION_MANAGER ) pSessionManager )->nLiveSessions > 0 ) ? TRUE : FALSE )

#endif // _SFTK_COM_H_