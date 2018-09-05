//tdilisten.c
//Worker Thread that Listens to the incomming connections and Accepts them
//The Connection Callback will Add the Sessions to the UsedSessionList
//If Connection Fails the Sessions are Added back to the FreeList

#include "tdiutil.h"



LONG InitializeListenThread(PSESSION_MANAGER pSessionManager)
{
	LONG lErrorCode=0;
	PCONNECTION_DETAILS pConnectionDetails = pSessionManager->lpConnectionDetails;
	int i=0,j=0;
	PCONNECTION_INFO pConnectionInfo = NULL;
	PSERVER_ELEMENT	pServer=NULL;
	PTCP_SESSION pSession=NULL;
	KIRQL oldIrql;

	try
	{
		ASSERT(pConnectionDetails);

		InitializeListHead(&pSessionManager->ServerList);

		InitializeSendBufferList(&pSessionManager->sendBufferList,pSessionManager->lpConnectionDetails);
		pSessionManager->sendBufferList.pSessionManager = pSessionManager;
		InitializeReceiveBufferList(&pSessionManager->receiveBufferList,pSessionManager->lpConnectionDetails);
		pSessionManager->receiveBufferList.pSessionManager = pSessionManager;


		pSessionManager->nLiveSessions =0;

		KeInitializeEvent(&pSessionManager->GroupUninitializedEvent,NotificationEvent,FALSE);

		KeInitializeEvent(&pSessionManager->GroupConnectedEvent,NotificationEvent,FALSE);

		KeInitializeEvent(&pSessionManager->IOExitEvent,NotificationEvent,FALSE);

		ExInitializeResourceLite(&pSessionManager->ServerListLock);

		ExInitializeNPagedLookasideList(&pSessionManager->ProtocolList,NULL,NULL,0,PROTOCOL_PACKET_SIZE,MEM_TAG,0);

		InitializeListHead(&pSessionManager->ProtocolQueue);

		KeInitializeSpinLock(&pSessionManager->ProtocolQueueLock);

		//Initialize the Send Lock
		ExInitializeFastMutex(&pSessionManager->SendLock);
		//Initialize the Receive Lock
		ExInitializeFastMutex(&pSessionManager->ReceiveLock);

		pConnectionInfo = pConnectionDetails->ConnectionDetails;

		for(i=0;i<pConnectionDetails->nConnections;i++)
		{
		
			pServer = NULL;
			lErrorCode =0;

			if(pServer == NULL)
			{
				lErrorCode = InitializeServer(&pServer,pConnectionInfo);
				if(lErrorCode==1)
				{
					//Memory Allocation Failed No Use Going Any further;
					break;
				}
				InsertTailList(&pSessionManager->ServerList,&pServer->ListElement);
				pServer->pSessionManager = pSessionManager;
			}

			ASSERT(pServer);

			lErrorCode = 0;
			pSession = NULL;

			if(pConnectionInfo->nNumberOfSessions > 1)
			{
				pConnectionInfo->nNumberOfSessions = 1;
			}
			for(j=0;j<pConnectionInfo->nNumberOfSessions;j++)
			{
				pSession = NULL;
				lErrorCode =0;

				lErrorCode = InitializeSession(&pSession,pServer,pConnectionInfo,pConnectionDetails->nSendWindowSize,pConnectionDetails->nReceiveWindowSize,ACCEPT);
				if(lErrorCode == 1)
				{
					break;;
				}
				pServer->pSession = pSession;
			}

			if(lErrorCode == 1)
			{
				break;
			}
			pConnectionInfo+=1;
		}
	}
	finally
	{
	}
	pSessionManager->bInitialized = 1;
	return lErrorCode;
}


NTSTATUS CreateListenThread(PSESSION_MANAGER pSessionManager)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PSERVER_ELEMENT	pServerElement = NULL;
	PLIST_ENTRY pTemp;
	LARGE_INTEGER iWait;
	LARGE_INTEGER iZeroWait;
	BOOLEAN bExit = FALSE;
	PTCP_SESSION pSession = NULL;
	PLIST_ENTRY pTempEntry;
	KIRQL oldIrql;
	PDEVICE_OBJECT             pDeviceObject;
	BOOLEAN bSendReceiveInitialized = FALSE;

	KdPrint(("Enter CreateListenThread\n"));
	try
	{
		//Added Here for Initialization of the Address and Endpoint
		InitializeListenThread(pSessionManager);
		while(!bExit)
		{
			//Acquire the resource as shared Lock
			//This Resource will not be acquired if there is a call for Exclusive Lock

			KeEnterCriticalRegion();	//Disable the Kernel APC's
			ExAcquireResourceSharedLite(&pSessionManager->ServerListLock,TRUE);

			//30 Milli-Seconds in Nano Seconds
			iWait.QuadPart = -(30*10*1000);
			iZeroWait.QuadPart = 0;

			Status = KeWaitForSingleObject(&pSessionManager->GroupConnectedEvent,Executive,KernelMode,FALSE,&iZeroWait);
			if((Status == STATUS_SUCCESS) && !bSendReceiveInitialized)
			{
				ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
				//Commenting out for test
				Status = CreateSendReceiveThreads(pSessionManager);
				if(NT_SUCCESS(Status))
				{
					bSendReceiveInitialized = TRUE;
				}
				KdPrint(("Enter CreateSendReceive\n"));
			}
			else if(bSendReceiveInitialized && (Status != STATUS_SUCCESS))
			{
				StopSendReceiveThreads(pSessionManager);
				bSendReceiveInitialized = FALSE;
				KdPrint(("Enter LeaveSendReceive\n"));
			}

			if(!IsListEmpty(&pSessionManager->ServerList))
			{
				pTemp = pSessionManager->ServerList.Flink;

				while(pTemp != &pSessionManager->ServerList && !bExit)
				{
					pServerElement = CONTAINING_RECORD(pTemp,SERVER_ELEMENT,ListElement);

					if(pServerElement == NULL)
					{
						KdPrint(("The SERVER_ELEMENT is Null and hence cannot Proceed\n"));
						Status = STATUS_MEMORY_NOT_ALLOCATED;
						break;
			//			goto End;
					}
					
					ASSERT(pServerElement);

					if(pServerElement->pSession != NULL && 
						pServerElement->pSession->bSessionEstablished == SFTK_DISCONNECTED)
					{
						pDeviceObject = NULL;
						ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
						Status = KeWaitForSingleObject(&pSessionManager->GroupUninitializedEvent,Executive,KernelMode,FALSE,&iWait);
						if(Status == STATUS_SUCCESS)
						{
							KdPrint(("The pSessionManager->GroupUninitializedEvent Got signalled so Exiting\n"));
							bExit = TRUE;
							break;
						}

						pSession = pServerElement->pSession;

						if(pSession == NULL)
						{
							KdPrint(("The TCP_SESSION is Null and hence cannot Proceed"));
						}
						ASSERT(pSession);

						pDeviceObject = IoGetRelatedDeviceObject( pSession->KSEndpoint.m_pFileObject);

						try
						{
							pSession->bSessionEstablished = SFTK_INPROGRESS;
							TdiBuildListen(
								pSession->pListenIrp,
								pDeviceObject,
								pSession->KSEndpoint.m_pFileObject,
								SftkTCPCConnectedCallback, // Completion Routine
								pSession,               // Completion Context
								0,                      // Flags
								NULL,                   // Request Connection Info
								&pSession->RemoteConnectionInfo
								);

							Status = IoCallDriver( pDeviceObject, pSession->pListenIrp);
						}
						except(EXCEPTION_EXECUTE_HANDLER)
						{
							KdPrint(("An Exception Occured in CreateListenThread() Error Code is %lx\n",GetExceptionCode()));
							Status = GetExceptionCode();
							pSession->bSessionEstablished = SFTK_DISCONNECTED;
						}

					}	//while(!IsListEmpty(pServerElement->FreeSessionList))
					pTemp = pTemp->Flink;
				}	//while(!IsListEmpty(lpListIndex))
			}	//if(!IsListEmpty(&pDeviceExtension->pSessionManager->ServerList))

			if(!bExit)
			{
				ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
				Status = KeWaitForSingleObject(&pSessionManager->GroupUninitializedEvent,Executive,KernelMode,FALSE,&iWait);
				if(Status == STATUS_SUCCESS)
				{
					KdPrint(("The pSessionManager->GroupUninitializedEvent Got signalled so Exiting\n"));
					bExit = TRUE;
					break;
				}
			}
			//Release the Resource 
			ExReleaseResourceLite(&pSessionManager->ServerListLock);
			KeLeaveCriticalRegion();	//Enable the Kernel APC's

		}	//while(!bExit)
	}
	finally
	{
		//Check if the Resource is still acquired for Shared Access
		//If So release the resource
		if(ExIsResourceAcquiredSharedLite(&pSessionManager->ServerListLock) >0)
		{
			ExReleaseResourceLite(&pSessionManager->ServerListLock);
			//Check if the Kernel APC's are Disabled 
			//if so Leave the Critical Section

//			if(KeAreApcsDisabled())
			{
				KeLeaveCriticalRegion();
			}
		}
		KdPrint(("Exiting the CreateListenThread()\n"));
	}

	return PsTerminateSystemThread(Status);
}


//Code with Optimizations
//This code will dynamically add remove sessions, the lock used is the exclusive lock
//ERESOURCE, Whenever a session needs to be added, the lock is obtained and then a link
//is either added or deleted from the sessions.
//A session is defined as { hostname , hostport , remotename , remoteport }
//Optionally N number of sessions might be added and removed as required.
#define __NEW_LISTEN__
#ifdef __NEW_LISTEN__

NTSTATUS ListenThread2(PSESSION_MANAGER pSessionManager)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PSERVER_ELEMENT	pServerElement = NULL;
	PLIST_ENTRY pTemp;
	LARGE_INTEGER iWait;
	LARGE_INTEGER iZeroWait;
	BOOLEAN bExit = FALSE;
	PTCP_SESSION pSession = NULL;
	PLIST_ENTRY pTempEntry;
	KIRQL oldIrql;
	PDEVICE_OBJECT             pDeviceObject;
	BOOLEAN bSendReceiveInitialized = FALSE;

	KdPrint(("Enter CreateConnectThread()\n"));
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

				Status = KeWaitForSingleObject(&pSessionManager->GroupConnectedEvent,Executive,KernelMode,FALSE,&iZeroWait);
				if((Status == STATUS_SUCCESS) && !bSendReceiveInitialized)
				{
					ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
					Status = CreateSendReceiveThreads(pSessionManager);
					if(NT_SUCCESS(Status))
					{
						bSendReceiveInitialized = TRUE;
					}//if
					KdPrint(("Enter CreateSendReceive\n"));
				}//if
				else if(bSendReceiveInitialized && (Status != STATUS_SUCCESS))
				{
					StopSendReceiveThreads(pSessionManager);
					bSendReceiveInitialized = FALSE;
					KdPrint(("Enter LeaveSendReceive\n"));
				}//else if
				pTemp = pSessionManager->ServerList.Flink;

				while(pTemp != &pSessionManager->ServerList && !bExit)
				{
					pServerElement = CONTAINING_RECORD(pTemp,SERVER_ELEMENT,ListElement);

					//Check if the Server Element and the Session Element are not NULL
					ASSERT(pServerElement);
					ASSERT(pServerElement->pSession);

					//ENABLE THE SESSION
					//Check if the Server Element is Initialized or not
					if((pServerElement->pSession->bSessionEstablished == SFTK_UNINITIALIZED) &&
						pServerElement->bEnable)
					{
						KdPrint(("The Session is not Enabled and not Initialized\n"));
						KdPrint(("This is the First Time it is called\n"));

						Status = InitializeServer1(pServerElement,ACCEPT);
						if(!NT_SUCCESS(Status))
						{
							KdPrint(("InitializeServer1() Failed So Just setting to UNINITIALIZED\n"));
							//Initialization of Server Element Failed and hence just set the
							//Session as Disconnected and then set the Server as disabled
							pServerElement->pSession->bSessionEstablished = SFTK_UNINITIALIZED;
							pServerElement->bEnable = FALSE;
							pServerElement->bIsEnabled = FALSE;
						}//if
						else
						{
							//Set the Session Status to DISCONNECTED and the Enabled to TRUE
							//So that we can go ahead and establish connection.
							pServerElement->pSession->bSessionEstablished = SFTK_DISCONNECTED;
							pServerElement->bIsEnabled = TRUE;
						}//else
					}//if

					//DISABLE THE SERVER_ELEMENT
					//Check to see if they want the Sessions to be Disabled
					if((pServerElement->pSession->bSessionEstablished != SFTK_UNINITIALIZED) &&
						!pServerElement->bEnable &&
						pServerElement->bIsEnabled == TRUE)
					{
						KdPrint(("We got the Command to Disable the Server %lx\n",pServerElement));

						UninitializeServer1(pServerElement);
						pServerElement->pSession->bSessionEstablished = SFTK_UNINITIALIZED;
						pServerElement->bEnable = FALSE;
						pServerElement->bIsEnabled = FALSE;
					}//if


					//PROCESS the NORMAL CONNECTIONS
					//Check if the Session Needs to be established
					if((pServerElement->pSession != NULL) && 
						(pServerElement->pSession->bSessionEstablished == SFTK_DISCONNECTED) &&
						pServerElement->bIsEnabled)
					{

						ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
						Status = KeWaitForSingleObject(&pSessionManager->GroupUninitializedEvent,Executive,KernelMode,FALSE,&iWait);
						if(Status == STATUS_SUCCESS)
						{
							KdPrint(("The pSessionManager->GroupUninitializedEvent Got signalled so Exiting\n"));
							bExit = TRUE;
							break;
						}

						pSession = pServerElement->pSession;

						pDeviceObject = IoGetRelatedDeviceObject( pSession->KSEndpoint.m_pFileObject);
						try
						{
							pSession->bSessionEstablished = SFTK_INPROGRESS;
							TdiBuildListen(
								pSession->pListenIrp,
								pDeviceObject,
								pSession->KSEndpoint.m_pFileObject,
								SftkTCPCConnectedCallback, // Completion Routine
								pSession,               // Completion Context
								0,                      // Flags
								NULL,                   // Request Connection Info
								&pSession->RemoteConnectionInfo
								);
							Status = IoCallDriver( pDeviceObject, pSession->pListenIrp);
						}//try
						except(EXCEPTION_EXECUTE_HANDLER)
						{
							KdPrint(("An Exception Occured in CreateConnectThread() Error Code is %lx\n",GetExceptionCode()));
							Status = GetExceptionCode();
							pSession->bSessionEstablished = SFTK_DISCONNECTED;
						}//except

					}	//while(!IsListEmpty(pServerElement->FreeSessionList))
					pTemp = pTemp->Flink;
				}	//while(!IsListEmpty(lpListIndex))

				if(!bExit)
				{
					ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
					Status = KeWaitForSingleObject(&pSessionManager->GroupUninitializedEvent,Executive,KernelMode,FALSE,&iWait);
					if(Status == STATUS_SUCCESS)
					{
						KdPrint(("The pSessionManager->GroupUninitializedEvent Got signalled so Exiting\n"));
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
		KdPrint(("Exiting the CreateConnectThread() Status = %lx\n",Status));
	}//finally

	return PsTerminateSystemThread(Status);
}


#endif //__NEW_LISTEN__