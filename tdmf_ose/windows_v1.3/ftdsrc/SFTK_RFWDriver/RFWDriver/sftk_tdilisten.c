/**************************************************************************************

Module Name: sftk_tdilisten.c   
Author Name: Veera Arja
Description: Describes Modules: Protocol and CommunicationFunctions
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2004 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/

#include "sftk_main.h"

//Code with Optimizations
//This code will dynamically add remove sessions, the lock used is the exclusive lock
//ERESOURCE, Whenever a session needs to be added, the lock is obtained and then a link
//is either added or deleted from the sessions.
//A session is defined as { hostname , hostport , remotename , remoteport }
//Optionally N number of sessions might be added and removed as required.
#define __NEW_LISTEN__
#ifdef __NEW_LISTEN__

NTSTATUS 
COM_ListenThread(
				 PANCHOR_LINKLIST pLg_GroupList
				 )
{
	NTSTATUS		status			= STATUS_SUCCESS;
	PSFTK_CONFIG	pSftk_Config	= NULL;
	ROLE_TYPE		roleType		= PRIMARY;
	LARGE_INTEGER	iWait;
	BOOLEAN			bExit			= FALSE;

	OS_ASSERT(pLg_GroupList != NULL);
	OS_ASSERT(pLg_GroupList->ParentPtr != NULL);
	pSftk_Config = (PSFTK_CONFIG)pLg_GroupList->ParentPtr;

	if(&pSftk_Config->Lg_GroupList == pLg_GroupList)
	{
		roleType = PRIMARY;
	}
	else if(&pSftk_Config->TLg_GroupList == pLg_GroupList)
	{
		roleType = SECONDARY;
	}
	else
	{
		DebugPrint((DBG_LISTEN, "COM_ListenThread(): Passed an invalid GroupList"));
		OS_ASSERT(FALSE);
	}

	iWait.QuadPart = DEFAULT_TIMEOUT_FOR_LISTEN_THREAD; // DEFAULT_TIMEOUT_FOR_LISTEN_THREAD = -(10*1000*1000)  // relative 1 seconds
	while(!bExit)
	{
		if(roleType == PRIMARY)
		{
			status = KeWaitForSingleObject(&pSftk_Config->ListenThreadExitEvent,Executive,KernelMode,FALSE,&iWait);	
		}
		else
		{
			status = KeWaitForSingleObject(&pSftk_Config->TListenThreadExitEvent,Executive,KernelMode,FALSE,&iWait);	
		}

		switch(status)
		{
		case STATUS_TIMEOUT:
			break;
		case STATUS_SUCCESS:
			bExit = TRUE;
			break;
		default:
			break;
		}
	}
	return PsTerminateSystemThread(status);
}

NTSTATUS 
COM_ListenThread2(
			PSESSION_MANAGER pSessionManager
			)
{
	NTSTATUS status = STATUS_SUCCESS;
	PSERVER_ELEMENT	pServerElement = NULL;
	PLIST_ENTRY pTemp;
	LARGE_INTEGER iWait;
	LARGE_INTEGER iZeroWait;
	BOOLEAN bExit = FALSE;
	PDEVICE_OBJECT             pDeviceObject = NULL;
	BOOLEAN bSendReceiveInitialized = FALSE;

	DebugPrint((DBG_LISTEN, "COM_ListenThread2(): Enter COM_ListenThread2()\n"));
	try
	{
		while(!bExit)
		{
			try
			{
				// Acquire the resource as shared Lock
				// This Resource will not be acquired if there is a call for Exclusive Lock

				KeEnterCriticalRegion();	//Disable the Kernel APC's
				ExAcquireResourceSharedLite(&pSessionManager->ServerListLock,TRUE);

				// 100 Milli-Seconds in 100*Nano Seconds
				iWait.QuadPart = -(100*10*1000);
				iZeroWait.QuadPart = 0;

				status = KeWaitForSingleObject(&pSessionManager->GroupConnectedEvent,Executive,KernelMode,FALSE,&iZeroWait);
				if((status == STATUS_SUCCESS) && !bSendReceiveInitialized)
				{
					OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
					status = COM_CreateSendReceiveThreads(pSessionManager);
					if(NT_SUCCESS(status))
					{
						bSendReceiveInitialized = TRUE;
					}//if
					DebugPrint((DBG_LISTEN, "COM_ListenThread2(): Enter CreateSendReceive\n"));
				}//if
				else if(bSendReceiveInitialized && (status != STATUS_SUCCESS))
				{
					COM_StopSendReceiveThreads(pSessionManager);
					bSendReceiveInitialized = FALSE;
					DebugPrint((DBG_LISTEN, "COM_ListenThread2(): Enter LeaveSendReceive\n"));
				}//else if
				pTemp = pSessionManager->ServerList.Flink;

				while(pTemp != &pSessionManager->ServerList && !bExit)
				{
					pServerElement = CONTAINING_RECORD(pTemp,SERVER_ELEMENT,ListElement);
					pTemp = pTemp->Flink;

					//Check if the Server Element and the Session Element are not NULL
					OS_ASSERT(pServerElement);
					OS_ASSERT(pServerElement->pSession);

					//ENABLE THE SESSION
					//Check if the Server Element is Initialized or not
					if((pServerElement->pSession->bSessionEstablished == SFTK_UNINITIALIZED) &&
						pServerElement->bEnable)
					{
						DebugPrint((DBG_LISTEN, "COM_ListenThread2(): The Session is not Enabled and not Initialized\n"));
						DebugPrint((DBG_LISTEN, "COM_ListenThread2(): This is the First Time it is called\n"));

						// Release the Resource 
						ExReleaseResourceLite(&pSessionManager->ServerListLock);
						KeLeaveCriticalRegion();	//Enable the Kernel APC's

						status = COM_StartServer(pServerElement,ACCEPT);
						if(!NT_SUCCESS(status))
						{
							DebugPrint((DBG_LISTEN, "COM_ListenThread2(): COM_StartServer() Failed So Just setting to UNINITIALIZED\n"));
							//Initialization of Server Element Failed and hence just set the
							//Session as Disconnected and then set the Server as disabled
							pServerElement->pSession->bSessionEstablished = SFTK_UNINITIALIZED;
							pServerElement->bEnable = FALSE;
							pServerElement->bIsEnabled = FALSE;
						}//if
						else
						{
							//Set the Session status to DISCONNECTED and the Enabled to TRUE
							//So that we can go ahead and establish connection.
							pServerElement->pSession->bSessionEstablished = SFTK_DISCONNECTED;
							pServerElement->bIsEnabled = TRUE;
						}//else

						// Reaquire the resource as shared
						KeEnterCriticalRegion();	//Disable the Kernel APC's
						ExAcquireResourceSharedLite(&pSessionManager->ServerListLock,TRUE);

					}//if ENABLE

					//DISABLE THE SERVER_ELEMENT
					//Check to see if they want the Sessions to be Disabled
					if((pServerElement->pSession->bSessionEstablished != SFTK_UNINITIALIZED) &&
						!pServerElement->bEnable &&
						pServerElement->bIsEnabled == TRUE)
					{
						DebugPrint((DBG_LISTEN, "COM_ListenThread2(): We got the Command to Disable the Server %lx\n",pServerElement));

						// Release the Resource 
						ExReleaseResourceLite(&pSessionManager->ServerListLock);
						KeLeaveCriticalRegion();	//Enable the Kernel APC's

						COM_StopServer(pServerElement);

						pServerElement->pSession->bSessionEstablished = SFTK_UNINITIALIZED;
						pServerElement->bEnable = FALSE;
						pServerElement->bIsEnabled = FALSE;

						// Reaquire the resource as shared
						KeEnterCriticalRegion();	//Disable the Kernel APC's
						ExAcquireResourceSharedLite(&pSessionManager->ServerListLock,TRUE);

					}//if DISABLE

					// RESET the Server Element, Stop the Server and Restart it.
					if((pServerElement->pSession->bSessionEstablished != SFTK_UNINITIALIZED) &&
						pServerElement->bReset &&
						pServerElement->bIsEnabled == TRUE)
					{
						pServerElement->bReset = FALSE;
						DebugPrint((DBG_LISTEN, "COM_ListenThread2(): We got the Command to Disable the Server %lx\n",pServerElement));

						// Release the Resource 
						ExReleaseResourceLite(&pSessionManager->ServerListLock);
						KeLeaveCriticalRegion();	//Enable the Kernel APC's

						// We will get an exclusive lock here as we dont want Send and Receive Threads
						// to work on the same ServerElement while we are changing it.

						COM_StopServer(pServerElement);

						pServerElement->pSession->bSessionEstablished = SFTK_UNINITIALIZED;
						pServerElement->bEnable = FALSE;
						pServerElement->bIsEnabled = FALSE;

						status = COM_StartServer(pServerElement,CONNECT);

						if(!NT_SUCCESS(status))
						{
							DebugPrint((DBG_LISTEN, "COM_ListenThread2(): COM_StartServer() Failed So Just setting to UNINITIALIZED\n"));
							//Initialization of Server Element Failed and hence just set the
							//Session as Disconnected and then set the Server as disabled
							pServerElement->pSession->bSessionEstablished = SFTK_UNINITIALIZED;
							pServerElement->bEnable = FALSE;
							pServerElement->bIsEnabled = FALSE;
						}//if
						else
						{
							//Set the Session status to DISCONNECTED and the Enabled to TRUE
							//So that we can go ahead and establish connection.
							pServerElement->pSession->bSessionEstablished = SFTK_DISCONNECTED;
							pServerElement->bIsEnabled = TRUE;
						}//else

						// Reaquire the resource as shared
						KeEnterCriticalRegion();	//Disable the Kernel APC's
						ExAcquireResourceSharedLite(&pSessionManager->ServerListLock,TRUE);

					}//if RESET

					//PROCESS the NORMAL CONNECTIONS
					//Check if the Session Needs to be established
					if((pServerElement->pSession != NULL) && 
						(pServerElement->pSession->bSessionEstablished == SFTK_DISCONNECTED) &&
						pServerElement->bIsEnabled)
					{

						OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
						status = KeWaitForSingleObject(&pSessionManager->GroupUninitializedEvent,Executive,KernelMode,FALSE,&iWait);
						if(status == STATUS_SUCCESS)
						{
							DebugPrint((DBG_LISTEN, "COM_ListenThread2(): The pSessionManager->GroupUninitializedEvent Got signalled so Exiting\n"));
							bExit = TRUE;
							break;
						}

//						pSession = pServerElement->pSession;

						pDeviceObject = IoGetRelatedDeviceObject( pServerElement->pSession->KSEndpoint.m_pFileObject);
						try
						{
							pServerElement->pSession->bSessionEstablished = SFTK_INPROGRESS;
							TdiBuildListen(
								pServerElement->pSession->pListenIrp,
								pDeviceObject,
								pServerElement->pSession->KSEndpoint.m_pFileObject,
								TDI_ConnectedCallback, // Completion Routine
								pServerElement->pSession,               // Completion Context
								0,                      // Flags
								NULL,                   // Request Connection Info
								&pServerElement->pSession->RemoteConnectionInfo
								);
							status = IoCallDriver( pDeviceObject, pServerElement->pSession->pListenIrp);

							if( !NT_SUCCESS(status) )
							{
								DebugPrint((DBG_LISTEN, "COM_ListenThread2(): IoCallDriver(pDeviceObject = %lx) returned %lx\n",pDeviceObject,status));
							}

						}//try
						except(EXCEPTION_EXECUTE_HANDLER)
						{
							DebugPrint((DBG_ERROR, "COM_ListenThread2(): An Exception Occured in COM_ListenThread2() Error Code is %lx\n",GetExceptionCode()));
							status = GetExceptionCode();
							pServerElement->pSession->bSessionEstablished = SFTK_DISCONNECTED;
						}//except

					}	//while(!IsListEmpty(pServerElement->pSession))
				}	//while(!IsListEmpty(lpListIndex))

				if(!bExit)
				{
					OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
					status = KeWaitForSingleObject(&pSessionManager->GroupUninitializedEvent,Executive,KernelMode,FALSE,&iWait);
					if(status == STATUS_SUCCESS)
					{
						DebugPrint((DBG_LISTEN, "COM_ListenThread2(): The pSessionManager->GroupUninitializedEvent Got signalled so Exiting\n"));
						bExit = TRUE;
						break;
					}//if
				}//if
			}//try
			finally
			{
				//Release the Resource 
				ExReleaseResourceLite(&pSessionManager->ServerListLock);
				KeLeaveCriticalRegion();	//Enable the Kernel APC's
			}//finally
		}	//while(!bExit)
	}//try
	finally
	{
		DebugPrint((DBG_LISTEN, "COM_ListenThread2(): Exiting the COM_ListenThread2() status = %lx\n",status));
	}//finally

	return PsTerminateSystemThread(status);
}//COM_ListenThread2()


NTSTATUS 
COM_ListenThreadForServer(
			PSESSION_MANAGER pSessionManager
			)
{
	NTSTATUS status = STATUS_SUCCESS;
	PSERVER_ELEMENT	pServerElement = NULL;
	PLIST_ENTRY pTemp;
	LARGE_INTEGER iWait;
	LARGE_INTEGER iZeroWait;
	BOOLEAN bExit = FALSE;
	PDEVICE_OBJECT             pDeviceObject = NULL;
	BOOLEAN bSendReceiveInitialized = FALSE;

	DebugPrint((DBG_LISTEN, "COM_ListenThreadForServer(): Enter COM_ListenThreadForServer()\n"));
	try
	{
		while(!bExit)
		{
			try
			{
				// Acquire the resource as shared Lock
				// This Resource will not be acquired if there is a call for Exclusive Lock

				KeEnterCriticalRegion();	//Disable the Kernel APC's
				ExAcquireResourceSharedLite(&pSessionManager->ServerListLock,TRUE);

				//30 Milli-Seconds in 100*Nano Seconds
				iWait.QuadPart = -(30*10*1000);
				iZeroWait.QuadPart = 0;

				pTemp = pSessionManager->ServerList.Flink;

				while(pTemp != &pSessionManager->ServerList && !bExit)
				{
					pServerElement = CONTAINING_RECORD(pTemp,SERVER_ELEMENT,ListElement);
					pTemp = pTemp->Flink;

					//Check if the Server Element and the Session Element are not NULL
					OS_ASSERT(pServerElement);
					OS_ASSERT(pServerElement->pSession);

					//ENABLE THE SESSION
					//Check if the Server Element is Initialized or not
					if((pServerElement->pSession->bSessionEstablished == SFTK_UNINITIALIZED) &&
						pServerElement->bEnable)
					{
						DebugPrint((DBG_LISTEN, "COM_ListenThreadForServer(): The Session is not Enabled and not Initialized\n"));
						DebugPrint((DBG_LISTEN, "COM_ListenThreadForServer(): This is the First Time it is called\n"));

						// Release the Resource 
						ExReleaseResourceLite(&pSessionManager->ServerListLock);
						KeLeaveCriticalRegion();	//Enable the Kernel APC's

						status = COM_StartServer(pServerElement,ACCEPT);
						if(!NT_SUCCESS(status))
						{
							DebugPrint((DBG_LISTEN, "COM_ListenThreadForServer(): COM_StartServer() Failed So Just setting to UNINITIALIZED\n"));
							//Initialization of Server Element Failed and hence just set the
							//Session as Disconnected and then set the Server as disabled
							pServerElement->pSession->bSessionEstablished = SFTK_UNINITIALIZED;
							pServerElement->bEnable = FALSE;
							pServerElement->bIsEnabled = FALSE;
						}//if
						else
						{
							//Set the Session status to DISCONNECTED and the Enabled to TRUE
							//So that we can go ahead and establish connection.
							pServerElement->pSession->bSessionEstablished = SFTK_DISCONNECTED;
							pServerElement->bIsEnabled = TRUE;
						}//else

						// Reaquire the resource as shared
						KeEnterCriticalRegion();	//Disable the Kernel APC's
						ExAcquireResourceSharedLite(&pSessionManager->ServerListLock,TRUE);

					}//if ENABLE

					//DISABLE THE SERVER_ELEMENT
					//Check to see if they want the Sessions to be Disabled
					if((pServerElement->pSession->bSessionEstablished != SFTK_UNINITIALIZED) &&
						!pServerElement->bEnable &&
						pServerElement->bIsEnabled == TRUE)
					{
						DebugPrint((DBG_LISTEN, "COM_ListenThreadForServer(): We got the Command to Disable the Server %lx\n",pServerElement));

						// Release the Resource 
						ExReleaseResourceLite(&pSessionManager->ServerListLock);
						KeLeaveCriticalRegion();	//Enable the Kernel APC's

						COM_StopServer(pServerElement);

						pServerElement->pSession->bSessionEstablished = SFTK_UNINITIALIZED;
						pServerElement->bEnable = FALSE;
						pServerElement->bIsEnabled = FALSE;

						// Reaquire the resource as shared
						KeEnterCriticalRegion();	//Disable the Kernel APC's
						ExAcquireResourceSharedLite(&pSessionManager->ServerListLock,TRUE);

					}//if DISABLE

					// RESET the Server Element, Stop the Server and Restart it.
					if((pServerElement->pSession->bSessionEstablished != SFTK_UNINITIALIZED) &&
						pServerElement->bReset &&
						pServerElement->bIsEnabled == TRUE)
					{
						pServerElement->bReset = FALSE;
						DebugPrint((DBG_LISTEN, "COM_ListenThreadForServer(): We got the Command to Disable the Server %lx\n",pServerElement));

						// Release the Resource 
						ExReleaseResourceLite(&pSessionManager->ServerListLock);
						KeLeaveCriticalRegion();	//Enable the Kernel APC's

						// We will get an exclusive lock here as we dont want Send and Receive Threads
						// to work on the same ServerElement while we are changing it.

						COM_StopServer(pServerElement);

						pServerElement->pSession->bSessionEstablished = SFTK_UNINITIALIZED;
						pServerElement->bEnable = FALSE;
						pServerElement->bIsEnabled = FALSE;

						status = COM_StartServer(pServerElement,CONNECT);

						if(!NT_SUCCESS(status))
						{
							DebugPrint((DBG_LISTEN, "COM_ListenThreadForServer(): COM_StartServer() Failed So Just setting to UNINITIALIZED\n"));
							//Initialization of Server Element Failed and hence just set the
							//Session as Disconnected and then set the Server as disabled
							pServerElement->pSession->bSessionEstablished = SFTK_UNINITIALIZED;
							pServerElement->bEnable = FALSE;
							pServerElement->bIsEnabled = FALSE;
						}//if
						else
						{
							//Set the Session status to DISCONNECTED and the Enabled to TRUE
							//So that we can go ahead and establish connection.
							pServerElement->pSession->bSessionEstablished = SFTK_DISCONNECTED;
							pServerElement->bIsEnabled = TRUE;
						}//else

						// Reaquire the resource as shared
						KeEnterCriticalRegion();	//Disable the Kernel APC's
						ExAcquireResourceSharedLite(&pSessionManager->ServerListLock,TRUE);

					}//if RESET

					if(pServerElement->pSession->bSessionEstablished == SFTK_CONNECTED)
					{
						// If the Session is Connected then we have to verify if the send and 
						// Receive threads for this Session are created or not, if they are not created
						// the threads will be created

						// Create the Send Thread
						status = COM_StartSendThreadForServer(pServerElement);
						// Create the Receive Thread
						status = COM_StartReceiveThreadForServer(pServerElement);
					}//if SFTK_CONNECTED
					else if(pServerElement->pSession->bSessionEstablished == SFTK_DISCONNECTED)
					{
						// If the Session is Disconnected then we have to verify if the Send and Receive threads
						// are Terminited or not and wait until we get out of them.

						// Stop the Send receive Threads
						status = COM_StopSendReceiveThreadForServer(pServerElement);
					}//else if SFTK_DISCONNECTED


					//PROCESS the NORMAL CONNECTIONS
					//Check if the Session Needs to be established
					if((pServerElement->pSession != NULL) && 
						(pServerElement->pSession->bSessionEstablished == SFTK_DISCONNECTED) &&
						pServerElement->bIsEnabled)
					{

						OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
						status = KeWaitForSingleObject(&pSessionManager->GroupUninitializedEvent,Executive,KernelMode,FALSE,&iWait);
						if(status == STATUS_SUCCESS)
						{
							DebugPrint((DBG_LISTEN, "COM_ListenThreadForServer(): The pSessionManager->GroupUninitializedEvent Got signalled so Exiting\n"));
							bExit = TRUE;
							break;
						}

//						pSession = pServerElement->pSession;

						pDeviceObject = IoGetRelatedDeviceObject( pServerElement->pSession->KSEndpoint.m_pFileObject);
						try
						{
							pServerElement->pSession->bSessionEstablished = SFTK_INPROGRESS;
							TdiBuildListen(
								pServerElement->pSession->pListenIrp,
								pDeviceObject,
								pServerElement->pSession->KSEndpoint.m_pFileObject,
								TDI_ConnectedCallback, // Completion Routine
								pServerElement->pSession,               // Completion Context
								0,                      // Flags
								NULL,                   // Request Connection Info
								&pServerElement->pSession->RemoteConnectionInfo
								);
							status = IoCallDriver( pDeviceObject, pServerElement->pSession->pListenIrp);

							if( !NT_SUCCESS(status) )
							{
								DebugPrint((DBG_LISTEN, "COM_ListenThreadForServer(): IoCallDriver(pDeviceObject = %lx) returned %lx\n",pDeviceObject,status));
							}

						}//try
						except(EXCEPTION_EXECUTE_HANDLER)
						{
							DebugPrint((DBG_ERROR, "COM_ListenThreadForServer(): An Exception Occured in COM_ListenThreadForServer() Error Code is %lx\n",GetExceptionCode()));
							status = GetExceptionCode();
							pServerElement->pSession->bSessionEstablished = SFTK_DISCONNECTED;
						}//except

					}	//while(!IsListEmpty(pServerElement->pSession))
				}	//while(!IsListEmpty(lpListIndex))

				if(!bExit)
				{
					OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
					status = KeWaitForSingleObject(&pSessionManager->GroupUninitializedEvent,Executive,KernelMode,FALSE,&iWait);
					if(status == STATUS_SUCCESS)
					{
						DebugPrint((DBG_LISTEN, "COM_ListenThreadForServer(): The pSessionManager->GroupUninitializedEvent Got signalled so Exiting\n"));
						bExit = TRUE;
						break;
					}//if
				}//if
			}//try
			finally
			{
				//Release the Resource 
				ExReleaseResourceLite(&pSessionManager->ServerListLock);
				KeLeaveCriticalRegion();	//Enable the Kernel APC's
			}//finally
		}	//while(!bExit)
	}//try
	finally
	{
		DebugPrint((DBG_LISTEN, "COM_ListenThreadForServer(): Exiting the COM_ListenThreadForServer() status = %lx\n",status));
	}//finally

	return PsTerminateSystemThread(status);
}//COM_ListenThreadForServer()


#endif //__NEW_LISTEN__
