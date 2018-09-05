/**************************************************************************************

Module Name: sftk_Queue.C   
Author Name: Parag sanghvi 
Description: Queue Manager APIS are deinfed here.
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2002 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/
#include <sftk_main.h>

VOID
QM_Init( PSFTK_LG	Sftk_Lg)
{
	PQUEUE_MANAGER	pQueueMgr = &Sftk_Lg->QueueMgr;

	OS_INITIALIZE_LOCK( &pQueueMgr->Lock, OS_SPIN_LOCK, NULL);
	ANCHOR_InitializeListHead( pQueueMgr->PendingList );
	ANCHOR_InitializeListHead( pQueueMgr->CommitList );
	ANCHOR_InitializeListHead( pQueueMgr->RefreshList );
	ANCHOR_InitializeListHead( pQueueMgr->RefreshPendingList );
	ANCHOR_InitializeListHead( pQueueMgr->MigrateList );

#if TARGET_SIDE
	ANCHOR_InitializeListHead( pQueueMgr->TRecieveList );
	ANCHOR_InitializeListHead( pQueueMgr->TSendList );
#endif

	pQueueMgr->SR_SendCommitRecordsPerSRRecords = DEFAULT_SR_SENDCIMMITRECORDS_PER_SRRECORDS;
	pQueueMgr->SR_SentCommitRecordCounter		= 0;

	OS_SetFlag(Sftk_Lg->flags, SFTK_LG_FLAG_QUEUE_MANAGER_INIT);
} // QM_Init()

VOID
QM_DeInit( PSFTK_LG	Sftk_Lg)
{
	PQUEUE_MANAGER	pQueueMgr = &Sftk_Lg->QueueMgr;

	if (OS_IsFlagSet(Sftk_Lg->flags, SFTK_LG_FLAG_QUEUE_MANAGER_INIT) == TRUE)
	{
		OS_DEINITIALIZE_LOCK( &pQueueMgr->Lock, NULL);
		ANCHOR_InitializeListHead( pQueueMgr->PendingList );
		ANCHOR_InitializeListHead( pQueueMgr->CommitList );
		ANCHOR_InitializeListHead( pQueueMgr->RefreshList );
		ANCHOR_InitializeListHead( pQueueMgr->RefreshPendingList );
		ANCHOR_InitializeListHead( pQueueMgr->MigrateList );

#if TARGET_SIDE
		ANCHOR_InitializeListHead( pQueueMgr->TRecieveList );
		ANCHOR_InitializeListHead( pQueueMgr->TSendList );
#endif

		OS_ClearFlag(Sftk_Lg->flags, SFTK_LG_FLAG_QUEUE_MANAGER_INIT);
	}
} // QM_DeInit()


NTSTATUS
QM_Insert( PSFTK_LG	Sftk_Lg, PVOID	MmContext, QUEUE_TYPE DstQueue, BOOLEAN GrabLock)
{
	PQUEUE_MANAGER		pQueueMgr = &Sftk_Lg->QueueMgr;
	PMM_HOLDER			pMmHolder = MmContext;
	PSFTK_DEV			pSftkDev  = NULL;
	PANCHOR_LINKLIST	pAnchorList;
	PLIST_ENTRY			pListEntry;
	PMM_PROTO_HDR		pMmProtoHdr;
	PMM_SOFT_HDR		pMmSoftHdr;
	
	switch(DstQueue)
	{
		case PENDING_QUEUE:		
								pAnchorList = &pQueueMgr->PendingList;	
								OS_SetFlag( pMmHolder->FlagLink, MM_HOLDER_FLAG_LINK_PENDING_LIST);
								pListEntry = &pMmHolder->MmHolderLink;

								Sftk_Lg->Statistics.QM_WrPending ++;
								Sftk_Lg->Statistics.QM_BlksWrPending += Get_Values_InSectorsFromBytes(pMmHolder->Size);
								MM_GetSftkDevFromMM_Holder(pMmHolder, pSftkDev);
								if (pSftkDev)
								{
									pSftkDev->Statistics.QM_WrPending ++;
									pSftkDev->Statistics.QM_BlksWrPending += Get_Values_InSectorsFromBytes(pMmHolder->Size);
								}
								break;

		case COMMIT_QUEUE:		
								pMmSoftHdr = (PMM_SOFT_HDR) MM_GetHdr(pMmHolder);
								OS_ASSERT( MM_IsSoftHdr(pMmSoftHdr) == TRUE);

								if (GSftk_Config.Mmgr.MM_UnInitializedInProgress == TRUE)
								{
									DebugPrint((DBG_ERROR, "___ QM_Insert: LG %d, MM Uninitialized State %d changed, Freeing CommitPkts, left Num %d, O %I64d S %d !!!\n",
															Sftk_Lg->LGroupNumber, Sftk_Lg->state, pQueueMgr->CommitList.NumOfNodes, 
															MM_GetOffsetFromSoftHdr(pMmSoftHdr),MM_GetLengthFromSoftHdr(pMmSoftHdr) ));

									// Free this pkts, For safe side, Just update Bitmap also....
									if (MM_GetOffsetInBlocksFromSoftHdr(pMmSoftHdr) != -1)
									{
										OS_ASSERT( pMmSoftHdr->SftkDev != NULL );
										sftk_Update_bitmap(	pMmSoftHdr->SftkDev, 
															MM_GetOffsetFromSoftHdr(pMmSoftHdr),
															MM_GetLengthFromSoftHdr(pMmSoftHdr));
									}
									mm_free_buffer( Sftk_Lg, pMmHolder);
									return STATUS_NO_MEMORY;
								}

								if ( (sftk_lg_get_state(Sftk_Lg) != SFTK_MODE_SMART_REFRESH) &&
									 (sftk_lg_get_state(Sftk_Lg) != SFTK_MODE_NORMAL) )
								{ // Ops, We jumped to tracking mode while we had pkts in pending Queue
									DebugPrint((DBG_ERROR, "___ QM_Insert: LG %d, State %d changed, Freeing CommitPkts, left Num %d, O %I64d S %d !!!\n",
															Sftk_Lg->LGroupNumber, Sftk_Lg->state, pQueueMgr->CommitList.NumOfNodes, 
															MM_GetOffsetFromSoftHdr(pMmSoftHdr),MM_GetLengthFromSoftHdr(pMmSoftHdr) ));

									// Free this pkts, For safe side, Just update Bitmap also....
									if (MM_GetOffsetInBlocksFromSoftHdr(pMmSoftHdr) != -1)
									{
										OS_ASSERT( pMmSoftHdr->SftkDev != NULL );

										sftk_Update_bitmap(	pMmSoftHdr->SftkDev, 
															MM_GetOffsetFromSoftHdr(pMmSoftHdr),
															MM_GetLengthFromSoftHdr(pMmSoftHdr));
									}
									mm_free_buffer( Sftk_Lg, pMmHolder);
									return STATUS_NO_MEMORY;
								}

								pAnchorList = &pQueueMgr->CommitList;	
								OS_SetFlag( pMmHolder->FlagLink, MM_HOLDER_FLAG_LINK_COMMIT_LIST);
								pListEntry = &pMmHolder->MmHolderLink;

								Sftk_Lg->Statistics.QM_WrCommit ++;
								if (MM_GetOffsetInBlocksFromSoftHdr(pMmSoftHdr) != -1)
									Sftk_Lg->Statistics.QM_BlksWrCommit += MM_GetLengthInBlocksFromSoftHdr(pMmSoftHdr); // Get_Values_InSectorsFromBytes(pMmHolder->Size);
								MM_GetSftkDevFromMM_Holder(pMmHolder, pSftkDev);
								if (pSftkDev)
								{
									pSftkDev->Statistics.QM_WrCommit ++;
									if (MM_GetOffsetInBlocksFromSoftHdr(pMmSoftHdr) != -1)
										pSftkDev->Statistics.QM_BlksWrCommit += MM_GetLengthInBlocksFromSoftHdr(pMmSoftHdr); // Get_Values_InSectorsFromBytes(pMmHolder->Size);
								}
								// Some Packet is avalible to send so we can use this to wait for the Packet Retrival
								KeSetEvent(&Sftk_Lg->EventPacketsAvailableForRetrival, 0, FALSE);
								break;

		case REFRESH_PENDING_QUEUE: 
								/*
								if (Sftk_Lg->ReleaseIsWaiting == TRUE)
								{
									mm_free_buffer_to_refresh_pool( Sftk_Lg, pMmHolder );
									return STATUS_NO_MEMORY;
								}
								*/
								OS_ASSERT(Sftk_Lg->ReserveIsActive == TRUE);

								pMmProtoHdr = (PMM_PROTO_HDR) MM_GetHdr(pMmHolder);
								OS_ASSERT( MM_IsProtoHdr(pMmProtoHdr) == TRUE);
								
								pAnchorList = &pQueueMgr->RefreshPendingList;	
								OS_SetFlag( pMmHolder->FlagLink, MM_HOLDER_FLAG_LINK_REFRESH_LIST);
								pListEntry = &pMmHolder->MmHolderLink;

								Sftk_Lg->Statistics.QM_WrRefreshPending ++;
								Sftk_Lg->Statistics.QM_BlksWrRefreshPending += MM_GetLengthInBlocksFromProtoHdr(pMmProtoHdr);
								MM_GetSftkDevFromMM_Holder( pMmHolder, pSftkDev);
								if (pSftkDev)
								{
									pSftkDev->Statistics.QM_WrRefreshPending ++;
									pSftkDev->Statistics.QM_BlksWrRefreshPending += MM_GetLengthInBlocksFromProtoHdr(pMmProtoHdr);
								}
								break;
							
		case REFRESH_QUEUE:		
								pMmProtoHdr = (PMM_PROTO_HDR) MM_GetHdr(pMmHolder);
								OS_ASSERT( MM_IsProtoHdr(pMmProtoHdr) == TRUE);
								
								if (Sftk_Lg->ReleaseIsWaiting == TRUE)
								{
									DebugPrint((DBG_ERROR, "___ QM_Insert: LG %d, ReleaseIsWaiting State %d, Freeing RefreshPkts, left Num %d, O %I64d S %d !!!\n",
															Sftk_Lg->LGroupNumber, Sftk_Lg->state, pQueueMgr->RefreshList.NumOfNodes, 
															MM_GetOffsetFromProtoHdr(pMmProtoHdr),MM_GetLengthFromProtoHdr(pMmProtoHdr) ));

									mm_free_buffer_to_refresh_pool( Sftk_Lg, pMmHolder );
									return STATUS_NO_MEMORY;
								}
								OS_ASSERT(Sftk_Lg->ReserveIsActive == TRUE);

								pAnchorList = &pQueueMgr->RefreshList;	
								OS_SetFlag( pMmHolder->FlagLink, MM_HOLDER_FLAG_LINK_REFRESH_LIST);
								pListEntry = &pMmHolder->MmHolderLink;

								Sftk_Lg->Statistics.QM_WrRefresh ++;
								Sftk_Lg->Statistics.QM_BlksWrRefresh += MM_GetLengthInBlocksFromProtoHdr(pMmProtoHdr);
								MM_GetSftkDevFromMM_Holder( pMmHolder, pSftkDev);
								if (pSftkDev)
								{
									pSftkDev->Statistics.QM_WrRefresh ++;
									pSftkDev->Statistics.QM_BlksWrRefresh += MM_GetLengthInBlocksFromProtoHdr(pMmProtoHdr);
								}
								// Some Packet is avalible to send so we can use this to wait for the Packet Retrival
								KeSetEvent(&Sftk_Lg->EventPacketsAvailableForRetrival, 0, FALSE);
								break;

		case MIGRATION_QUEUE:	
								OS_ASSERT( GSftk_Config.Mmgr.MM_UnInitializedInProgress == FALSE);

								pMmProtoHdr = (PMM_PROTO_HDR) pMmHolder;
								pAnchorList = &pQueueMgr->MigrateList;	
								pListEntry = &pMmProtoHdr->MmProtoHdrLink;

								Sftk_Lg->Statistics.QM_WrMigrate ++;
								Sftk_Lg->Statistics.QM_BlksWrMigrate += Get_Values_InSectorsFromBytes(pMmProtoHdr->RawDataSize);
								pSftkDev = pMmProtoHdr->SftkDev;
								if (pSftkDev)
								{
									pSftkDev->Statistics.QM_WrMigrate ++;
									pSftkDev->Statistics.QM_BlksWrMigrate += Get_Values_InSectorsFromBytes(pMmHolder->Size);
								}
								break;

		case TRECEIVE_QUEUE:
								// Check to see if we are in Secondary Mode or not.
								// This Queue insert should happen only if we are in Secondary Mode
								OS_ASSERT(LG_IS_SECONDARY_MODE(Sftk_Lg));

								pMmProtoHdr = (PMM_PROTO_HDR) MM_GetHdr(pMmHolder);
								OS_ASSERT(pMmProtoHdr != NULL);
								OS_ASSERT( MM_IsProtoHdr(pMmProtoHdr) == TRUE);

								pAnchorList = &pQueueMgr->TRecieveList;	
								OS_SetFlag( pMmHolder->FlagLink, MM_HOLDER_FLAG_LINK_TRECEIVE_LIST);
								pListEntry = &pMmHolder->MmHolderLink;

								Sftk_Lg->Statistics.QM_WrTReceive ++;
								Sftk_Lg->Statistics.QM_BlksWrTReceive += MM_GetLengthInBlocksFromProtoHdr(pMmProtoHdr); // Get the Size of the Packet

								// Some Packet is avalible to Process by the Target Write Thread So Signal the Event
								KeSetEvent(&Sftk_Lg->Secondary.TWriteWorkEvent, 0, FALSE);
								break;
		case TSEND_QUEUE:
								// Check to see if we are in Secondary Mode or not.
								// This Queue insert should happen only if we are in Secondary Mode
								OS_ASSERT(LG_IS_SECONDARY_MODE(Sftk_Lg));

								pMmProtoHdr = (PMM_PROTO_HDR) MM_GetHdr(pMmHolder);
								OS_ASSERT(pMmProtoHdr != NULL);
								OS_ASSERT( MM_IsProtoHdr(pMmProtoHdr) == TRUE);

								pAnchorList = &pQueueMgr->TSendList;
								OS_SetFlag( pMmHolder->FlagLink, MM_HOLDER_FLAG_LINK_TSEND_LIST);
								pListEntry = &pMmHolder->MmHolderLink;

								Sftk_Lg->Statistics.QM_WrTSend ++;
								Sftk_Lg->Statistics.QM_BlksWrTSend += MM_GetLengthInBlocksFromProtoHdr(pMmProtoHdr); // Get the Size of the Packet

								// Some Packet is avalible to Send So Signal the Send Thread
								KeSetEvent(&Sftk_Lg->EventPacketsAvailableForRetrival, 0, FALSE);
								break;
		case NO_QUEUE:			
		default:
								DebugPrint((DBG_ERROR, "FIXME FIXME : BUG: QM_Insert :: QueueType %d: is undefined, Failing to ASSERT FIXME FIXME !! \n",
														DstQueue));
								OS_ASSERT(FALSE);
								return STATUS_NO_MEMORY;
	} // switch(DstQueue)

	// Insert node into Tail of Specified list
	if (GrabLock == TRUE)
		OS_ACQUIRE_LOCK( &pQueueMgr->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
	InsertTailList( &pAnchorList->ListEntry, pListEntry );
	pAnchorList->NumOfNodes ++;
	if (GrabLock == TRUE)
		OS_RELEASE_LOCK( &pQueueMgr->Lock, NULL);

	return STATUS_SUCCESS;

} // QM_Insert()

VOID
QM_Remove( PSFTK_LG	Sftk_Lg, PVOID	MmContext, QUEUE_TYPE SrcQueue, BOOLEAN GrabLock)
{
	PQUEUE_MANAGER		pQueueMgr = &Sftk_Lg->QueueMgr;
	PMM_HOLDER			pMmHolder = MmContext;
	PSFTK_DEV			pSftkDev = NULL;
	PANCHOR_LINKLIST	pAnchorList;
	PLIST_ENTRY			pListEntry;
	PMM_PROTO_HDR		pMmProtoHdr;
	PMM_SOFT_HDR		pMmSoftHdr;
	
	switch(SrcQueue)
	{
		case PENDING_QUEUE:		pAnchorList = &pQueueMgr->PendingList;	
								OS_SetFlag( pMmHolder->FlagLink, MM_HOLDER_FLAG_LINK_PENDING_LIST);
								pListEntry = &pMmHolder->MmHolderLink;

								Sftk_Lg->Statistics.QM_WrPending --;
								Sftk_Lg->Statistics.QM_BlksWrPending -= Get_Values_InSectorsFromBytes(pMmHolder->Size);
								MM_GetSftkDevFromMM_Holder(pMmHolder, pSftkDev);
								if (pSftkDev)
								{
									pSftkDev->Statistics.QM_WrPending --;
									pSftkDev->Statistics.QM_BlksWrPending -= Get_Values_InSectorsFromBytes(pMmHolder->Size);
								}
								break;

		case COMMIT_QUEUE:		pAnchorList = &pQueueMgr->CommitList;	
								OS_SetFlag( pMmHolder->FlagLink, MM_HOLDER_FLAG_LINK_COMMIT_LIST);
								pListEntry = &pMmHolder->MmHolderLink;

								pMmSoftHdr = (PMM_SOFT_HDR) MM_GetHdr(pMmHolder);
								OS_ASSERT( MM_IsSoftHdr(pMmSoftHdr) == TRUE);

								Sftk_Lg->Statistics.QM_WrCommit --;
								if (MM_GetOffsetInBlocksFromSoftHdr(pMmSoftHdr) != -1)
									Sftk_Lg->Statistics.QM_BlksWrCommit -= MM_GetLengthInBlocksFromSoftHdr(pMmSoftHdr); // Get_Values_InSectorsFromBytes(pMmHolder->Size);
								MM_GetSftkDevFromMM_Holder(pMmHolder, pSftkDev);
								if (pSftkDev)
								{
									pSftkDev->Statistics.QM_WrCommit --;
									if (MM_GetOffsetInBlocksFromSoftHdr(pMmSoftHdr) != -1)
										pSftkDev->Statistics.QM_BlksWrCommit -= MM_GetLengthInBlocksFromSoftHdr(pMmSoftHdr); // Get_Values_InSectorsFromBytes(pMmHolder->Size);
								}
								break;

		case REFRESH_PENDING_QUEUE: 
								pAnchorList = &pQueueMgr->RefreshPendingList;	
								OS_SetFlag( pMmHolder->FlagLink, MM_HOLDER_FLAG_LINK_REFRESH_LIST);
								pListEntry = &pMmHolder->MmHolderLink;

								pMmProtoHdr = (PMM_PROTO_HDR) MM_GetHdr(pMmHolder);
								OS_ASSERT( MM_IsProtoHdr(pMmProtoHdr) == TRUE);

								Sftk_Lg->Statistics.QM_WrRefreshPending --;
								Sftk_Lg->Statistics.QM_BlksWrRefreshPending -= MM_GetLengthInBlocksFromProtoHdr(pMmProtoHdr);
								MM_GetSftkDevFromMM_Holder(pMmHolder, pSftkDev);
								if (pSftkDev)
								{
									pSftkDev->Statistics.QM_WrRefreshPending --;
									pSftkDev->Statistics.QM_BlksWrRefreshPending -= MM_GetLengthInBlocksFromProtoHdr(pMmProtoHdr);
								}
								break;


		case REFRESH_QUEUE:		pAnchorList = &pQueueMgr->RefreshList;	
								OS_SetFlag( pMmHolder->FlagLink, MM_HOLDER_FLAG_LINK_REFRESH_LIST);
								pListEntry = &pMmHolder->MmHolderLink;

								pMmProtoHdr = (PMM_PROTO_HDR) MM_GetHdr(pMmHolder);
								OS_ASSERT( MM_IsProtoHdr(pMmProtoHdr) == TRUE);

								Sftk_Lg->Statistics.QM_WrRefresh --;
								Sftk_Lg->Statistics.QM_BlksWrRefresh -= MM_GetLengthInBlocksFromProtoHdr(pMmProtoHdr);
								MM_GetSftkDevFromMM_Holder(pMmHolder, pSftkDev);
								if (pSftkDev)
								{
									pSftkDev->Statistics.QM_WrRefresh --;
									pSftkDev->Statistics.QM_BlksWrRefresh -= MM_GetLengthInBlocksFromProtoHdr(pMmProtoHdr);
								}
								break;

		case MIGRATION_QUEUE:	pMmProtoHdr = (PMM_PROTO_HDR) pMmHolder;
								pAnchorList = &pQueueMgr->MigrateList;	
								pListEntry = &pMmProtoHdr->MmProtoHdrLink;

								Sftk_Lg->Statistics.QM_WrMigrate --;
								Sftk_Lg->Statistics.QM_BlksWrMigrate -= Get_Values_InSectorsFromBytes(pMmProtoHdr->RawDataSize);
								if (pSftkDev)
								{
									pSftkDev->Statistics.QM_WrMigrate --;
									pSftkDev->Statistics.QM_BlksWrMigrate -= Get_Values_InSectorsFromBytes(pMmProtoHdr->RawDataSize);
								}
								break;
		case TRECEIVE_QUEUE:
								// Check to see if we are in Secondary Mode or not.
								// This Queue insert should happen only if we are in Secondary Mode
								OS_ASSERT(LG_IS_SECONDARY_MODE(Sftk_Lg));

								pMmProtoHdr = (PMM_PROTO_HDR) MM_GetHdr(pMmHolder);
								OS_ASSERT(pMmProtoHdr != NULL);
								OS_ASSERT( MM_IsProtoHdr(pMmProtoHdr) == TRUE);

								pAnchorList = &pQueueMgr->TRecieveList;	
								OS_SetFlag( pMmHolder->FlagLink, MM_HOLDER_FLAG_LINK_TRECEIVE_LIST);
								pListEntry = &pMmHolder->MmHolderLink;

								Sftk_Lg->Statistics.QM_WrTReceive --;
								Sftk_Lg->Statistics.QM_BlksWrTReceive -= MM_GetLengthInBlocksFromProtoHdr(pMmProtoHdr); // Get the Size of the Packet

								// Some Packet is avalible to Process by the Target Write Thread So Signal the Event
								KeSetEvent(&Sftk_Lg->Secondary.TWriteWorkEvent, 0, FALSE);
								break;
		case TSEND_QUEUE:
								// Check to see if we are in Secondary Mode or not.
								// This Queue insert should happen only if we are in Secondary Mode
								OS_ASSERT(LG_IS_SECONDARY_MODE(Sftk_Lg));

								pMmProtoHdr = (PMM_PROTO_HDR) MM_GetHdr(pMmHolder);
								OS_ASSERT(pMmProtoHdr != NULL);
								OS_ASSERT( MM_IsProtoHdr(pMmProtoHdr) == TRUE);

								pAnchorList = &pQueueMgr->TSendList;
								OS_SetFlag( pMmHolder->FlagLink, MM_HOLDER_FLAG_LINK_TSEND_LIST);
								pListEntry = &pMmHolder->MmHolderLink;

								Sftk_Lg->Statistics.QM_WrTSend --;
								Sftk_Lg->Statistics.QM_BlksWrTSend -= MM_GetLengthInBlocksFromProtoHdr(pMmProtoHdr); // Get the Size of the Packet
								break;

		case NO_QUEUE:			
		default:
								DebugPrint((DBG_ERROR, "FIXME FIXME : BUG: QM_Remove:: QueueType %d: is undefined, Failing to ASSERT FIXME FIXME !! \n",
														SrcQueue));
								OS_ASSERT(FALSE);
								return;
	} // switch(DstQueue)

	// Insert node into Tail of Specified list
	if (GrabLock == TRUE)
		OS_ACQUIRE_LOCK( &pQueueMgr->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
	RemoveEntryList( pListEntry );
	pAnchorList->NumOfNodes --;
	if (GrabLock == TRUE)
		OS_RELEASE_LOCK( &pQueueMgr->Lock, NULL);
} // QM_Remove()

VOID
QM_Move( PSFTK_LG	Sftk_Lg, PVOID	MmContext, QUEUE_TYPE SrcQueue, QUEUE_TYPE DstQueue, BOOLEAN GrabLock)
{
	QM_Remove(Sftk_Lg, MmContext, SrcQueue, GrabLock);
	QM_Insert(Sftk_Lg, MmContext, DstQueue, GrabLock);
} // QM_Move()

NTSTATUS
QM_ScanAllQList(PSFTK_LG	Sftk_Lg, BOOLEAN FreeAllEntries, BOOLEAN UpdateBitmaps)
{
	PQUEUE_MANAGER		pQueueMgr = &Sftk_Lg->QueueMgr;
	PANCHOR_LINKLIST	pAnchorList;
	PLIST_ENTRY			plistEntry, pListEntry;
	ULONG				i, totalQMPkts;
	PMM_HOLDER			pMmHolder;
	PMM_SOFT_HDR		pMmSoftHdr;
	PMM_PROTO_HDR		pMmProtoHdr;
	PSFTK_DEV			pSftkDev;
	LONGLONG			offset;
	ULONG				length;
	OS_PERF;

	OS_PERF_STARTTIME;
	
	// - scan Migration list
	// - scan Refresh list
	// - scan Commit list
	// - scan Pending list
	OS_ACQUIRE_LOCK( &pQueueMgr->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);

	QM_GetTotalPkts( Sftk_Lg, totalQMPkts);

	for (i=0; i < 3; i++)
	{ // for : Scan thru all list
		switch(i)
		{
			case 0: pAnchorList = &pQueueMgr->MigrateList;	break;
			case 1: pAnchorList = &pQueueMgr->RefreshList;	break;
			case 2: pAnchorList = &pQueueMgr->CommitList;	break;
			// case 3: pAnchorList = &pQueueMgr->PendingList;	break;
			default : OS_ASSERT(FALSE); break;
		} // switch(i)

		if (FreeAllEntries == FALSE)
		{ // if (FreeAllEntries == FALSE)
			// Scan from Head List
			for( plistEntry = pAnchorList->ListEntry.Flink;
				 plistEntry != &pAnchorList->ListEntry;
				 plistEntry = plistEntry->Flink )
			{ // for :scan thru each and every Node of current list 
				offset = (LONGLONG) -1;
				length = 0;
				pSftkDev = NULL;

				if (pAnchorList == &pQueueMgr->MigrateList)
				{ // if : Migration list which uses PMM_PROTO_HDR as entry node
					pMmProtoHdr = CONTAINING_RECORD( plistEntry, MM_PROTO_HDR, MmProtoHdrLink);

					OS_ASSERT( MM_IsProtoHdr(pMmProtoHdr) == TRUE);
					
					if (Proto_IsMsgRefhreshIOPktProtoHdr(Proto_MMGetProtoHdr(pMmProtoHdr)) == TRUE)
					{ // Valid Refrefsh Data
						OS_ASSERT( pMmProtoHdr->SftkDev != NULL );
						pSftkDev= pMmProtoHdr->SftkDev;

						if (MM_GetOffsetInBlocksFromProtoHdr(pMmProtoHdr) != -1)
						{
							offset  = MM_GetOffsetFromProtoHdr(pMmProtoHdr);
							length  = MM_GetLengthFromProtoHdr(pMmProtoHdr);
						}
					}
					else
					{ // Its BAB IO
						if (MM_IsProtoHdrHasDataPkt(pMmProtoHdr) == TRUE)
						{  // This pkt has valid IO Data pkts in it or in its soft hdr 
							for(	pListEntry = pMmProtoHdr->MmSoftHdrList.ListEntry.Flink;
									pListEntry != &pMmProtoHdr->MmSoftHdrList.ListEntry;
									pListEntry = pListEntry->Flink )
							{ // for : Scan Soft Hdr inside Proto HDr and update Bitmap if needed
								pMmSoftHdr = CONTAINING_RECORD( pListEntry, MM_SOFT_HDR, MmSoftHdrLink);

								OS_ASSERT( MM_IsSoftHdr(pMmSoftHdr) == TRUE);
								OS_ASSERT( pMmSoftHdr->SftkDev != NULL );

								pSftkDev= pMmSoftHdr->SftkDev;
								if (MM_GetOffsetInBlocksFromSoftHdr(pMmSoftHdr) != -1)
								{
									offset  = MM_GetOffsetFromSoftHdr(pMmSoftHdr);
									length  = MM_GetLengthFromSoftHdr(pMmSoftHdr);
								}

								if ( (offset != (LONGLONG) -1) && (length > 0) && (UpdateBitmaps == TRUE) && (pSftkDev))
								{
									#if DBG
									sftk_ack_update_bitmap(pSftkDev, offset, length);
									#else
									SFTK_ACK_UPDATE_BITMAP(pSftkDev, offset, length);
									#endif
								}						
							} // for : Scan Soft Hdr inside Proto HDr and update Bitmap if needed
						}
						continue;	// since we have freeed go next
					} // Its BAB IO
				} // if : Migration list which uses PMM_PROTO_HDR as entry node
				else
				{ // Else Other list which uses PMM_HOLDER as entry node
					pMmHolder = CONTAINING_RECORD( plistEntry, MM_HOLDER, MmHolderLink);

					if ( OS_IsFlagSet( pMmHolder->Proto_type, MM_HOLDER_PROTO_TYPE_PROTO_HDR) )
					{ // Refresh List: it has Proto hdr instead of Soft header
						pMmProtoHdr = (PMM_PROTO_HDR) MM_GetHdr(pMmHolder);

						OS_ASSERT( MM_IsProtoHdr(pMmProtoHdr) == TRUE);
						//OS_ASSERT( pMmProtoHdr->SftkDev != NULL );
						if (Proto_IsMsgRefhreshIOPktProtoHdr(Proto_MMGetProtoHdr(pMmProtoHdr)) == TRUE)
						{ // Valid Refrefsh Data
							OS_ASSERT( pMmProtoHdr->SftkDev != NULL );
							pSftkDev= pMmProtoHdr->SftkDev;
							if (MM_GetOffsetInBlocksFromProtoHdr(pMmProtoHdr) != -1)
							{
								offset  = MM_GetOffsetFromProtoHdr(pMmProtoHdr);
								length  = MM_GetLengthFromProtoHdr(pMmProtoHdr);
							}
						}
					}
					else
					{ // Else : Pending List and Commit List may have Soft Header !! TODO Confirm these With Veera
						OS_ASSERT( OS_IsFlagSet( pMmHolder->Proto_type, MM_HOLDER_PROTO_TYPE_SOFT_HDR) == TRUE);

						pMmSoftHdr = (PMM_SOFT_HDR) MM_GetHdr(pMmHolder);

						OS_ASSERT( MM_IsSoftHdr(pMmSoftHdr) == TRUE);
						OS_ASSERT( pMmSoftHdr->SftkDev != NULL );

						pSftkDev= pMmSoftHdr->SftkDev;
						if (MM_GetOffsetInBlocksFromSoftHdr(pMmSoftHdr) != -1)
						{
							offset  = MM_GetOffsetFromSoftHdr(pMmSoftHdr);
							length  = MM_GetLengthFromSoftHdr(pMmSoftHdr);
						}
					}
				} // Else Other list which uses PMM_HOLDER as entry node

				if ( (offset != (LONGLONG) -1) && (length > 0) && (UpdateBitmaps == TRUE) && (pSftkDev) )
				{
					#if DBG
					sftk_ack_update_bitmap(pSftkDev, offset, length);
					#else
					SFTK_ACK_UPDATE_BITMAP(pSftkDev, offset, length);
					#endif
				}
			} // for :scan thru each and every Node of current list 
		} // if (FreeAllEntries == FALSE)
		else
		{ // else (FreeAllEntries == TRUE)
			// Scan from Head List
			while( !(IsListEmpty(&pAnchorList->ListEntry)) )
			{ // While : scan thru each and every Node of current list and free it after processing
				plistEntry = RemoveHeadList(&pAnchorList->ListEntry);
				pAnchorList->NumOfNodes --;

				offset = (LONGLONG) -1;
				length = 0;
				pSftkDev = NULL;

				if (pAnchorList == &pQueueMgr->MigrateList)
				{ // if : Migration list which uses PMM_PROTO_HDR as entry node
					// update stats
					Sftk_Lg->Statistics.QM_WrMigrate --;

					pMmProtoHdr = CONTAINING_RECORD( plistEntry, MM_PROTO_HDR, MmProtoHdrLink);

					OS_ASSERT( MM_IsProtoHdr(pMmProtoHdr) == TRUE);
					
					if (Proto_IsMsgRefhreshIOPktProtoHdr(Proto_MMGetProtoHdr(pMmProtoHdr)) == TRUE)
					{ // Valid Refrefsh Data
						OS_ASSERT( pMmProtoHdr->SftkDev != NULL );
						pSftkDev= pMmProtoHdr->SftkDev;

						if (MM_GetOffsetInBlocksFromProtoHdr(pMmProtoHdr) != -1)
						{
							offset  = MM_GetOffsetFromProtoHdr(pMmProtoHdr);
							length  = MM_GetLengthFromProtoHdr(pMmProtoHdr);
						}

						mm_free_ProtoHdr(pSftkDev->SftkLg, pMmProtoHdr);

						// update stats
						Sftk_Lg->Statistics.QM_BlksWrMigrate -= Get_Values_InSectorsFromBytes(length);
						if (pSftkDev)
						{
							pSftkDev->Statistics.QM_WrMigrate --;
							pSftkDev->Statistics.QM_BlksWrMigrate -= Get_Values_InSectorsFromBytes(length);
						}
					}
					else
					{ // Its BAB IO
						if (MM_IsProtoHdrHasDataPkt(pMmProtoHdr) == TRUE)
						{  // This pkt has valid IO Data pkts in it or in its soft hdr 
							// Check Proto Command and uses if soft header is present to make 
							// Bitmap update  
							for(	pListEntry = pMmProtoHdr->MmSoftHdrList.ListEntry.Flink;
									pListEntry != &pMmProtoHdr->MmSoftHdrList.ListEntry;
									pListEntry = pListEntry->Flink )
							{ // for : Scan Soft Hdr inside Proto HDr and update Bitmap if needed
								pMmSoftHdr = CONTAINING_RECORD( pListEntry, MM_SOFT_HDR, MmSoftHdrLink);

								OS_ASSERT( MM_IsSoftHdr(pMmSoftHdr) == TRUE);
								OS_ASSERT( pMmSoftHdr->SftkDev != NULL );

								pSftkDev= pMmSoftHdr->SftkDev;
								if (MM_GetOffsetInBlocksFromSoftHdr(pMmSoftHdr) != -1)
								{
									offset  = MM_GetOffsetFromSoftHdr(pMmSoftHdr);
									length  = MM_GetLengthFromSoftHdr(pMmSoftHdr);
								}

								if ( (offset != (LONGLONG) -1) && (length > 0) && (UpdateBitmaps == TRUE) && (pSftkDev) )
								{
									#if DBG
									sftk_ack_update_bitmap(pSftkDev, offset, length);
									#else
									SFTK_ACK_UPDATE_BITMAP(pSftkDev, offset, length);
									#endif
								}						

								// update stats
								Sftk_Lg->Statistics.QM_BlksWrMigrate -= Get_Values_InSectorsFromBytes(length);
								if (pSftkDev)
								{
									pSftkDev->Statistics.QM_WrMigrate --;
									pSftkDev->Statistics.QM_BlksWrMigrate -= Get_Values_InSectorsFromBytes(length);
								}
							} // for : Scan Soft Hdr inside Proto HDr and update Bitmap if needed
						}
						// Free Memory 
						mm_free_ProtoHdr(Sftk_Lg, pMmProtoHdr);
						continue;	// since we have freeed go next
					} // Its BAB IO
				} // if : Migration list which uses PMM_PROTO_HDR as entry node
				else
				{ // Else Other list which uses PMM_HOLDER as entry node

					pMmHolder = CONTAINING_RECORD( plistEntry, MM_HOLDER, MmHolderLink);

					if ( OS_IsFlagSet( pMmHolder->Proto_type, MM_HOLDER_PROTO_TYPE_PROTO_HDR) )
					{ // Refresh List: it has Proto hdr instead of Soft header
						pMmProtoHdr = (PMM_PROTO_HDR) MM_GetHdr(pMmHolder);

						OS_ASSERT( MM_IsProtoHdr(pMmProtoHdr) == TRUE);
						//OS_ASSERT( pMmProtoHdr->SftkDev != NULL );
						if (Proto_IsMsgRefhreshIOPktProtoHdr(Proto_MMGetProtoHdr(pMmProtoHdr)) == TRUE)
						{ // Valid Refrefsh Data
							OS_ASSERT( pMmProtoHdr->SftkDev != NULL );
							pSftkDev= pMmProtoHdr->SftkDev;

							if (MM_GetOffsetInBlocksFromProtoHdr(pMmProtoHdr) != -1)
							{
								offset  = MM_GetOffsetFromProtoHdr(pMmProtoHdr);
								length  = MM_GetLengthFromProtoHdr(pMmProtoHdr);
							}
						}

						if (Proto_IsMsgRefhreshIOPktProtoHdr(Proto_MMGetProtoHdr(pMmProtoHdr)) == TRUE)
						{ // Valid Refrefsh Data
							OS_ASSERT(pAnchorList == &pQueueMgr->RefreshList);
							mm_free_buffer_to_refresh_pool(Sftk_Lg, pMmHolder);
						}
						else
						{
							DebugPrint((DBG_ERROR, "FIXME FIXME : BUG: QM_ScanAllQList:: In Refresh Queue this pkt must be only IO Pkt going to ASSERT FIXME FIXME !! \n"));
							OS_ASSERT(FALSE);
						}
					}
					else
					{ // Else : Pending List and Commit List may have Soft Header !! TODO Confirm these With Veera
						OS_ASSERT( OS_IsFlagSet( pMmHolder->Proto_type, MM_HOLDER_PROTO_TYPE_SOFT_HDR) == TRUE);

						pMmSoftHdr = (PMM_SOFT_HDR) MM_GetHdr(pMmHolder);

						OS_ASSERT( MM_IsSoftHdr(pMmSoftHdr) == TRUE);
						OS_ASSERT( pMmSoftHdr->SftkDev != NULL );

						pSftkDev= pMmSoftHdr->SftkDev;
						if (MM_GetOffsetInBlocksFromSoftHdr(pMmSoftHdr) != -1)
						{
							offset  = MM_GetOffsetFromSoftHdr(pMmSoftHdr);
							length  = MM_GetLengthFromSoftHdr(pMmSoftHdr);
						}

						OS_ASSERT(pAnchorList != &pQueueMgr->RefreshList);
						mm_free_buffer(Sftk_Lg, pMmHolder);
					} // Else : Pending List and Commit List may have Soft Header !! TODO Confirm these With Veera

					OS_ASSERT(length >= 0);

					// update stats
					if (pAnchorList == &pQueueMgr->PendingList)
					{
						Sftk_Lg->Statistics.QM_WrPending --;
						Sftk_Lg->Statistics.QM_BlksWrPending -= Get_Values_InSectorsFromBytes(length);
						if (pSftkDev)
						{
							pSftkDev->Statistics.QM_WrPending --;
							pSftkDev->Statistics.QM_BlksWrPending -= Get_Values_InSectorsFromBytes(length);
						}
					}
					if (pAnchorList == &pQueueMgr->CommitList)
					{
						Sftk_Lg->Statistics.QM_WrCommit --;
						Sftk_Lg->Statistics.QM_BlksWrCommit -= Get_Values_InSectorsFromBytes(length);
						if (pSftkDev)
						{
							pSftkDev->Statistics.QM_WrCommit --;
							pSftkDev->Statistics.QM_BlksWrCommit -= Get_Values_InSectorsFromBytes(length);
						}
					}
					if (pAnchorList == &pQueueMgr->RefreshList)
					{
						OS_ASSERT(length > 0);

						Sftk_Lg->Statistics.QM_WrRefresh --;
						Sftk_Lg->Statistics.QM_BlksWrRefresh -= Get_Values_InSectorsFromBytes(length);
						if (pSftkDev)
						{
							pSftkDev->Statistics.QM_WrRefresh --;
							pSftkDev->Statistics.QM_BlksWrRefresh -= Get_Values_InSectorsFromBytes(length);
						}
					}
				} // Else Other list which uses PMM_HOLDER as entry node

				if ( (offset != (LONGLONG) -1) && (length > 0) && (UpdateBitmaps == TRUE) && (pSftkDev) )
				{
					#if DBG
					sftk_ack_update_bitmap(pSftkDev, offset, length);
					#else
					SFTK_ACK_UPDATE_BITMAP(pSftkDev, offset, length);
					#endif
				}
			} // While : scan thru each and every Node of current list and free it after processing
		} // else (FreeAllEntries == TRUE)

	} // for : Scan thru all list

	if (FreeAllEntries == TRUE)
	{
		PSFTK_LG		pLg			= NULL;
		PLIST_ENTRY		plistEntry	= NULL;

		if (UpdateBitmaps == TRUE)
		{
			OS_PERF_ENDTIME(QM_FREE_ALL_PKTS_AND_UPDATE_BITMAPS,totalQMPkts);	
		}
		else
		{
			OS_PERF_ENDTIME(QM_FREE_ALL_PKTS_AND_NO_UPDATE_BITMAPS,totalQMPkts);	
		}

		OS_ASSERT(Sftk_Lg->Statistics.QM_WrCommit == 0);
		OS_ASSERT( IsListEmpty(&pQueueMgr->CommitList.ListEntry) == TRUE);
		OS_ASSERT(Sftk_Lg->Statistics.QM_BlksWrCommit == 0);

		Sftk_Lg->Statistics.QM_BlksWrCommit = 0;	// make explictly zeros quick fix !!
	}
	else
	{ // onlu scan entries
		OS_PERF_ENDTIME(QM_SCAN_ALL_PKTS_AND_UPDATE_BITMAPS,totalQMPkts);	
	}
	OS_RELEASE_LOCK( &pQueueMgr->Lock, NULL);

	// if : RefreshList : Free Refresh buffer to MM
	if (FreeAllEntries == TRUE)
	{
		mm_release_buffer_for_refresh(Sftk_Lg);
	}

	return STATUS_SUCCESS;
} // QM_ScanAllQList()

// This function is required to create a chain of MDL's that can be sent over the TDI.
// It takes all the Packets in the SEND_QUEUE and creates a chain from them.
//
// Returns the Root MDL of the Chain and also the Root of the MM_HOLDER List. 
// This will be used by the TDI_SEND.


BOOLEAN
QM_MakeMdlChainOfSendQueuePackets(IN PANCHOR_LINKLIST			MmHolderAnchorList,
								  OUT OPTIONAL PMDL*			MdlPtr,	// Can be NULL
								  OUT PULONG					SizePtr,	// Returns Size of the Total Buffer
								  OUT PMM_HOLDER*				MmHolderPtr	// Can be NULL
								 )
{
	PANCHOR_LINKLIST	pAnchorList;
	PLIST_ENTRY			plistEntry, plistEntry1;
	PMM_HOLDER			pMmHolder;
	PMM_SOFT_HDR		pMMSoftHdr;
	PMM_PROTO_HDR		pMMProtoHdr;	// MM holding Structure of Proto Hdr Pointer
	PVOID				pProtoHdr;	// Protocol Structure representing Proto Hdr and SOFT HDr
	PMDL				pRootMdl = NULL;
	PMDL				pCurrentMdl = NULL;
	BOOLEAN				bRetStatus = TRUE;

	//Validate the Input Parameters.

	OS_ASSERT(MmHolderAnchorList != NULL);
	OS_ASSERT(SizePtr != NULL);
	OS_ASSERT(MmHolderPtr != NULL);
	
	try
	{
		if(MmHolderAnchorList == NULL || 
			SizePtr == NULL || 
			MmHolderPtr == NULL){
			DebugPrint((DBG_ERROR, "QM_MakeMdlChainOfSendQueuePackets: Invalid input Parameters Error\n"));
			bRetStatus = FALSE;
			leave;
		}
		// Initialize the Anchor List
		pAnchorList = MmHolderAnchorList;
		// Initialize the Size Ptr
		*SizePtr = 0;
		// Initialize the Mdl Ptr
		if(MdlPtr != NULL){
			*MdlPtr = NULL;
		}
		// Initialize the MM_HOLDER Ptr
		*MmHolderPtr = NULL;

		// Scan from Head List
		plistEntry = pAnchorList->ListEntry.Flink;
		while( plistEntry != &pAnchorList->ListEntry ){
			plistEntry = plistEntry->Flink;
			pMmHolder = CONTAINING_RECORD( plistEntry, MM_HOLDER, MmHolderLink);

			if(pRootMdl == NULL){
				// This is the first time this routinue has been called. The first Packet Shoild have 
				// a Protocol Header Or else this is a wrong sequence.
				OS_ASSERT(MM_GetHdr(pMmHolder) != NULL);
				OS_ASSERT(OS_IsFlagSet( pMmHolder->Proto_type, MM_HOLDER_PROTO_TYPE_PROTO_HDR) == TRUE);
				OS_ASSERT(MM_IsProtoHdr((PMM_PROTO_HDR)MM_GetHdr(pMmHolder)) == TRUE);
			}
			
			if( OS_IsFlagSet( pMmHolder->Proto_type, MM_HOLDER_PROTO_TYPE_PROTO_HDR) == TRUE ){
				pMMProtoHdr = MM_GetHdr(pMmHolder);
				OS_ASSERT(pMMProtoHdr != NULL);
				OS_ASSERT( MM_IsProtoHdr(pMMProtoHdr) == TRUE);

				if(pRootMdl == NULL){
					// Set the Root of the Mdl to the protocol MDL
					OS_ASSERT(pMMProtoHdr->Mdl != NULL);
					pRootMdl = pMMProtoHdr->Mdl;
					pCurrentMdl = pRootMdl;
					*SizePtr += MmGetMdlByteCount(pMMProtoHdr->Mdl);

					// This is the First MM_HOLDER so Get this Pointer. This Pointer will go
					// into the Send Queue List.
					*MmHolderPtr = pMmHolder;
				}
				else{
					// Set the Proto Mdl if not the first Packet
					OS_ASSERT(pMMProtoHdr->Mdl != NULL);
					pCurrentMdl->Next = pMMProtoHdr->Mdl;
					pCurrentMdl = pCurrentMdl->Next;
					*SizePtr += MmGetMdlByteCount(pMMProtoHdr->Mdl);
				}

				MM_GetSoftHdrFromProtoHdr(pMMProtoHdr,pMMSoftHdr);
				if(pMMSoftHdr != NULL){ 
					// Contains a Soft Header Should Be an Sentinel
					OS_ASSERT(MM_IsSoftHdr(pMMSoftHdr) == TRUE);
					OS_ASSERT(pMMSoftHdr->Mdl != NULL);
					// Set the Soft Header Mdl in the chain
					pCurrentMdl->Next = pMMSoftHdr->Mdl;
					pCurrentMdl = pCurrentMdl->Next;
					*SizePtr += MmGetMdlByteCount(pMMSoftHdr->Mdl);
				}// Contains a Soft Header Should Be an Sentinel

				// Set the Data in the Chain.
				if(pMmHolder->Mdl != NULL){
					pCurrentMdl->Next = pMmHolder->Mdl;
					pCurrentMdl = pCurrentMdl->Next;
					*SizePtr += MmGetMdlByteCount(pMmHolder->Mdl);
				}
			} // Protocol Packets
			else
			{ // IO Packets so add the Soft Header and the Data
				pMMSoftHdr = MM_GetHdr(pMmHolder);
				OS_ASSERT( pMMSoftHdr != NULL);
				OS_ASSERT( MM_IsSoftHdr(pMMSoftHdr) == TRUE);

				// The Root Mdl Should have already been set.
				OS_ASSERT(pRootMdl != NULL);
				// The Current Mdl Should have already been set.
				OS_ASSERT(pCurrentMdl != NULL);
				// Set the MDl for not the First Packet
				OS_ASSERT(pMMSoftHdr->Mdl != NULL);
				pCurrentMdl->Next = pMMSoftHdr->Mdl;
				pCurrentMdl = pCurrentMdl->Next;
				*SizePtr += MmGetMdlByteCount(pMMSoftHdr->Mdl);

				// Set the Data in the Chain.
				if(pMmHolder->Mdl != NULL){
					pCurrentMdl->Next = pMmHolder->Mdl;
					pCurrentMdl = pCurrentMdl->Next;
					*SizePtr += MmGetMdlByteCount(pMmHolder->Mdl);
				}
			} // IO Packets so add the Soft Header and the Data
		} // while( plistEntry != &pAnchorList->ListEntry )

		// return the RootMdl.
		// Root MDL will allways be the MDL of the PMM_PROTO_HDR of the MM_HOLDER. After that
		// the rest of the MDL's will follow.
		// The First One should allways be the Protocol Header.
		if(MdlPtr != NULL){
			*MdlPtr = pRootMdl;
		}
	}
	finally
	{
	}
return bRetStatus;
}// QM_MakeMdlChainOfSendQueuePackets
//
// Queue Dispatch API used only for Inband
//
// Return TRUE means Buffer got data else Buffer is untouch empty.
// Return FALSE Event will have waid Pointer, Caller can wait
// on this event, once get signalled caller can call back this routine for work.
//
BOOLEAN
QM_RetrievePkts(IN PSFTK_LG			Sftk_Lg, 
			   IN OUT PVOID			Buffer, // If DontFreePackets == TRUE then Buffer = PSEND_BUFFER else Buffer = UCHAR[]
											// This Argument cannot be NULL.
			   IN OUT PULONG		Size,	// In Buffer Max size, OUT actual buffer data filled size
			   IN BOOLEAN			DontFreePackets,	// If TRUE will use the Memory Manager else uses locally allocated buffers
			   OUT PMM_HOLDER		*MmRootMmHolderPtr, // if DontFreePackets is TRUE then this will not be NULL, else NULL.
			   IN OUT PKSEMAPHORE	*WorkWaitEvent)
{
	PQUEUE_MANAGER		pQueueMgr = &Sftk_Lg->QueueMgr;
	PANCHOR_LINKLIST	pAnchorList, pAnchorMigrateQueue;
	PLIST_ENTRY			plistEntry;
	PMM_HOLDER			pMmHolder;
	PMM_SOFT_HDR		pMMSoftHdr;
	PMM_PROTO_HDR		pMMProtoHdr, pMMmigrateProtoHdr;	// MM holding Structure of Proto Hdr Pointer
	PVOID				pProtoHdr;	// Protocol Structure representing Proto Hdr and SOFT HDr
	LONG				currentstate;		
	ULONG				sizeLeft, length;
	ULONG				protoHdrSize = 0;
	ULONG				softHdrSize = 0;
	PUCHAR				buffer;
	PVOID				pData;
	BOOLEAN				bBufferFull = FALSE;
	BOOLEAN				bret		= TRUE;	
	PSFTK_DEV			pSftkDev	= NULL;
	ANCHOR_LINKLIST		tempMmHolderList;
	PMM_HOLDER			pMmRootMmHolder = NULL;
	PSEND_BUFFER		pSendBuffer = NULL;

	OS_ASSERT(Buffer != NULL);

	if(DontFreePackets == TRUE){
		OS_ASSERT(MmRootMmHolderPtr != NULL);
	}
//	OS_ASSERT(WorkWaitEvent != NULL);
	sizeLeft		= *Size;

	if(DontFreePackets == FALSE){
		// This is normal case with locally allocated buffers
		buffer = Buffer;
	}else{
		// Using the Memory Manager allocated Buffers.
		pSendBuffer = (PSEND_BUFFER)Buffer;
	}

	*WorkWaitEvent	= &Sftk_Lg->RefreshStateChangeSemaphore;
	// Initialize this only if we are using the Send Buffers, Or else this buffer can be NULL.
	if(DontFreePackets == FALSE){
		OS_ZeroMemory(Buffer, sizeLeft);
	}

	// Use this as Anchor for all the buffers that will be used for creating a chain of HH_HOLER Mdl's
	ANCHOR_InitializeListHead(tempMmHolderList);

	OS_ASSERT(sizeLeft != 0);

	OS_ACQUIRE_LOCK( &pQueueMgr->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);

	pAnchorMigrateQueue = &pQueueMgr->MigrateList;

	// Get State from LG
	currentstate = sftk_lg_get_state(Sftk_Lg);
	switch(currentstate)
	{
		case SFTK_MODE_FULL_REFRESH: 
		case SFTK_MODE_SMART_REFRESH:
				if (currentstate == SFTK_MODE_FULL_REFRESH)
				{
					pAnchorList = &pQueueMgr->RefreshList;	// send only SMArt Refresh/Full Refresh Pkts
					break;
				}
#if TARGET_SIDE
				if (pQueueMgr->SR_SentCommitRecordCounter < pQueueMgr->SR_SendCommitRecordsPerSRRecords )
				{
					pAnchorList = &pQueueMgr->CommitList;	
					pQueueMgr->SR_SentCommitRecordCounter ++;
				}
				else
				{
					pAnchorList = &pQueueMgr->RefreshList;	
					pQueueMgr->SR_SentCommitRecordCounter = 0;
				}
#else
				pAnchorList = &pQueueMgr->RefreshList;	// send only SMArt Refresh/Full Refresh Pkts
#endif
				break;

		case SFTK_MODE_NORMAL:
				pAnchorList = &pQueueMgr->CommitList;	break;

		case SFTK_MODE_TRACKING:
		case SFTK_MODE_PASSTHRU:
		case SFTK_MODE_BACKFRESH:
		default:
				pAnchorList = NULL; 
				break;
	} // switch(currentstate)

	if (pAnchorList == &pQueueMgr->CommitList)
	{ // if Anchor is Refresh List 
		bret = QM_RetrievePktsFromCommitQueue( Sftk_Lg, Buffer, Size, DontFreePackets, MmRootMmHolderPtr );
#if TARGET_SIDE
		if ( (*Size == 0) && (sftk_lg_get_state(Sftk_Lg) == SFTK_MODE_SMART_REFRESH))
		{ // nothing sending, so try Smart Refresh Pkts 
			pAnchorList = &pQueueMgr->RefreshList;	
			pQueueMgr->SR_SentCommitRecordCounter = 0;
		}
		else
			goto done;

#else
		goto done;
#endif
	}
	*Size = 0;

	if (pAnchorList == NULL)
	{
		bret = FALSE;
		goto done;
	}
	OS_ASSERT(pAnchorList == &pQueueMgr->RefreshList);

	if ( pQueueMgr->MigrateList.NumOfNodes >= Sftk_Lg->throtal_refresh_send_pkts )
	{
		DebugPrint((DBG_QUEUE, "^^^ QM_RetrievePkts: Holding Pkts...TotalPkts Left: Refresh:%d, RefreshPendingList:%d, Migrate: %d throtal_refresh_send_pkts %d, Commit:%d, ^^^ \n",
						pQueueMgr->RefreshList.NumOfNodes, pQueueMgr->RefreshPendingList.NumOfNodes,
						pQueueMgr->MigrateList.NumOfNodes, Sftk_Lg->throtal_refresh_send_pkts, pQueueMgr->CommitList.NumOfNodes));
		goto done;
	}
	DebugPrint((DBG_QUEUE, "QM_RetrievePkts: Refresh:%d, RefreshPendingList: %d, Commit:%d, Migrate: %d\n",
			pQueueMgr->RefreshList.NumOfNodes, pQueueMgr->RefreshPendingList.NumOfNodes, 
			pQueueMgr->CommitList.NumOfNodes, pQueueMgr->MigrateList.NumOfNodes));

	// Scan from Head List
	while( !(IsListEmpty(&pAnchorList->ListEntry)) )
	{ // While : scan thru each and every Node of current list and free it after processing
		if ( pQueueMgr->MigrateList.NumOfNodes >= Sftk_Lg->throtal_refresh_send_pkts )
		{
			DebugPrint((DBG_QUEUE, "^^^ QM_RetrievePkts: Holding Pkts, TotalPkts Left: Refresh:%d, RefreshPendingList:%d Migrate: %d throtal_refresh_send_pkts %d, Commit:%d, ^^^ \n",
							pQueueMgr->RefreshList.NumOfNodes, pQueueMgr->RefreshPendingList.NumOfNodes, 
							pQueueMgr->MigrateList.NumOfNodes, 
							Sftk_Lg->throtal_refresh_send_pkts,pQueueMgr->CommitList.NumOfNodes));
			break;
		}

		plistEntry = RemoveHeadList(&pAnchorList->ListEntry);
		pAnchorList->NumOfNodes --;

		pMmHolder = CONTAINING_RECORD( plistEntry, MM_HOLDER, MmHolderLink);
		
		OS_ASSERT( OS_IsFlagSet( pMmHolder->Proto_type, MM_HOLDER_PROTO_TYPE_PROTO_HDR) == TRUE );

		pMMProtoHdr = MM_GetHdr(pMmHolder);
		OS_ASSERT( MM_IsProtoHdr(pMMProtoHdr) == TRUE);

		pProtoHdr = Proto_MMGetProtoHdr(pMMProtoHdr);

		if (protoHdrSize == 0)
			protoHdrSize = Proto_GetProtoHdrSize(pMMProtoHdr);

		pData  = MM_GetDataBufferPointer(pMmHolder);
		length = MM_GetLengthFromProtoHdr(pMMProtoHdr);

		OS_ASSERT(pData != NULL);

		// update Stats
		Sftk_Lg->Statistics.QM_WrRefresh --;
		Sftk_Lg->Statistics.QM_BlksWrRefresh -= Get_Values_InSectorsFromBytes(length);
		MM_GetSftkDevFromMM_Holder(pMmHolder, pSftkDev);
		if (pSftkDev)
		{
			pSftkDev->Statistics.QM_WrRefresh --;
			pSftkDev->Statistics.QM_BlksWrRefresh -= Get_Values_InSectorsFromBytes(length);
		}

		if (Proto_IsMsgRefhreshIOPktProtoHdr(pProtoHdr) == TRUE)
		{ // Its RAW Refresh Read Data Pkts, without any soft header
			OS_ASSERT( pData != NULL);
			OS_ASSERT( length > 0);

			if ( (protoHdrSize + length) > sizeLeft)
			{ // UnderRun Buffer : insert back unprocessed pkt in Queue at HEAD List
				InsertHeadList( &pAnchorList->ListEntry, plistEntry);
				pAnchorList->NumOfNodes ++;

				// update Stats
				Sftk_Lg->Statistics.QM_WrRefresh ++;
				Sftk_Lg->Statistics.QM_BlksWrRefresh += Get_Values_InSectorsFromBytes(length);
				MM_GetSftkDevFromMM_Holder(pMmHolder, pSftkDev);
				if (pSftkDev)
				{
					pSftkDev->Statistics.QM_WrRefresh ++;
					pSftkDev->Statistics.QM_BlksWrRefresh += Get_Values_InSectorsFromBytes(length);
				}

				bBufferFull = TRUE;
				break; // UnderRun Buffer
			} // UnderRun Buffer

			// Allocate and copy Migrate Proto Hdr and move it to Migration Queue
			pMMmigrateProtoHdr = mm_type_alloc(MM_TYPE_PROTOCOL_HEADER);
			if (pMMmigrateProtoHdr == NULL)
			{
				DebugPrint((DBG_ERROR, "QM_RetrievePkts: For Migartion Queue mm_type_alloc(MM_TYPE_PROTOCOL_HEADER %d) Failed BUG CHECK !! \n",
												MM_TYPE_PROTOCOL_HEADER));
				OS_ASSERT(FALSE);

				// update Stats
				Sftk_Lg->Statistics.QM_WrRefresh ++;
				Sftk_Lg->Statistics.QM_BlksWrRefresh += Get_Values_InSectorsFromBytes(length);
				MM_GetSftkDevFromMM_Holder(pMmHolder, pSftkDev);
				if (pSftkDev)
				{
					pSftkDev->Statistics.QM_WrRefresh ++;
					pSftkDev->Statistics.QM_BlksWrRefresh += Get_Values_InSectorsFromBytes(length);
				}

				InsertHeadList( &pAnchorList->ListEntry, plistEntry);
				pAnchorList->NumOfNodes ++;

				bBufferFull = TRUE;
				break; // UnderRun Buffer
			}
			
			// Copy Proto Hdr First
#if DBG
			DebugPrint((DBG_PROTO, "**** QM: pMmHolder %x Sending:MsgId %d,devid:0x%08x,O:%d,len:%d, protoHdrSize:%d,length:%d *** \n",
					pMmHolder, ((ftd_header_t *) pProtoHdr)->msgtype, ((ftd_header_t *) pProtoHdr)->msg.lg.devid, ((ftd_header_t *) pProtoHdr)->msg.lg.offset, 
					((ftd_header_t *) pProtoHdr)->msg.lg.len, protoHdrSize,length ));
#endif
			if(DontFreePackets == FALSE){
				OS_RtlCopyMemory( buffer, pProtoHdr, protoHdrSize);
				buffer	=  (PUCHAR) ( ((ULONG) buffer) + (ULONG) (protoHdrSize));
			}
			*Size	+= protoHdrSize;
			sizeLeft -= protoHdrSize;

			if(DontFreePackets == FALSE){
				// Than Copy Data Pkts
				OS_RtlCopyMemory( buffer, pData, length);
				buffer	=  (PUCHAR) ( ((ULONG) buffer) + (ULONG) (length));
			}
			*Size	+= length;
			sizeLeft -= length;
			
			pMMmigrateProtoHdr->SftkDev	= pMMProtoHdr->SftkDev;
			OS_RtlCopyMemory( Proto_MMGetProtoHdr(pMMmigrateProtoHdr), pProtoHdr, protoHdrSize);

			// Free Data Pkt to MM
			OS_ASSERT(pAnchorList == &pQueueMgr->RefreshList);
			if(DontFreePackets == FALSE){
				mm_free_buffer_to_refresh_pool(Sftk_Lg, pMmHolder);
			}else{
				// Insert the MM_HOLDER with the Proto Header into the Temporary MM_HOLDER list
				InsertTailList(&tempMmHolderList.ListEntry , &pMmHolder->MmHolderLink);
			}

			// Insert Proto Hdr into Migrate Queue at Tail List
			pMMmigrateProtoHdr->RawDataSize = length;

			Set_MM_ProtoHdrMsgType(pMMmigrateProtoHdr, Proto_Get_ProtoHdrMsgType(pMMmigrateProtoHdr) );// save original msgtype.

			QM_Insert(Sftk_Lg, (PMM_HOLDER) pMMmigrateProtoHdr, MIGRATION_QUEUE, FALSE);
			continue;
		} // Its RAW Refresh Read Data Pkts, without any soft header
		else
		{ // Sentinal Pkts : No IO Data Pkts
#if DBG
			if ( (Sftk_Lg->state == SFTK_MODE_FULL_REFRESH) || (Sftk_Lg->state == FTD_MODE_SMART_REFRESH) )
			{
				DebugPrint((DBG_PROTO, "??? MUST NOT COME HERE QMRefresh: pMmHolder %x  ??? \n",pMmHolder ));
				OS_ASSERT(FALSE);
			}
#endif
			MM_GetSoftHdrFromProtoHdr(pMMProtoHdr, pMMSoftHdr);

			if (softHdrSize == 0)
				softHdrSize = Proto_GetSoftHdrSize(pMMSoftHdr);
		
			OS_ASSERT( pMMSoftHdr != NULL);
			OS_ASSERT( pData != NULL);
			OS_ASSERT( length > 0);

			if ( (protoHdrSize + softHdrSize + length) > sizeLeft)
			{ // UnderRun Buffer : insert back unprocessed pkt in Queue at HEAD List
				// update Stats
				Sftk_Lg->Statistics.QM_WrRefresh ++;
				Sftk_Lg->Statistics.QM_BlksWrRefresh += Get_Values_InSectorsFromBytes(length);
				MM_GetSftkDevFromMM_Holder(pMmHolder, pSftkDev);
				if (pSftkDev)
				{
					pSftkDev->Statistics.QM_WrRefresh ++;
					pSftkDev->Statistics.QM_BlksWrRefresh += Get_Values_InSectorsFromBytes(length);
				}
				InsertHeadList( &pAnchorList->ListEntry, plistEntry);
				pAnchorList->NumOfNodes ++;

				bBufferFull = TRUE;
				break; // UnderRun Buffer
			} // UnderRun Buffer
			// Allocate and copy Migrate Proto Hdr and move it to Migration Queue
			pMMmigrateProtoHdr = mm_type_alloc(MM_TYPE_PROTOCOL_HEADER);
			if (pMMmigrateProtoHdr == NULL)
			{
				DebugPrint((DBG_ERROR, "QM_RetrievePkts: For Migartion Queue mm_type_alloc(MM_TYPE_PROTOCOL_HEADER %d) Failed BUG CHECK !! \n",
												MM_TYPE_PROTOCOL_HEADER));
				OS_ASSERT(FALSE);

				// update Stats
				Sftk_Lg->Statistics.QM_WrRefresh ++;
				Sftk_Lg->Statistics.QM_BlksWrRefresh += Get_Values_InSectorsFromBytes(length);
				MM_GetSftkDevFromMM_Holder(pMmHolder, pSftkDev);
				if (pSftkDev)
				{
					pSftkDev->Statistics.QM_WrRefresh ++;
					pSftkDev->Statistics.QM_BlksWrRefresh += Get_Values_InSectorsFromBytes(length);
				}

				InsertHeadList( &pAnchorList->ListEntry, plistEntry);
				pAnchorList->NumOfNodes ++;

				bBufferFull = TRUE;
				break; // UnderRun Buffer
			}

			if(DontFreePackets == FALSE){
				// Copy Proto Hdr First
				OS_RtlCopyMemory( buffer, pProtoHdr, protoHdrSize);
				buffer	=  (PUCHAR) ( ((ULONG) buffer) + (ULONG) (protoHdrSize));
			}
			*Size	+= protoHdrSize;
			sizeLeft -= protoHdrSize;

			if(DontFreePackets == FALSE){
				// Copy Soft Hdr First
				OS_RtlCopyMemory( buffer, Proto_MMGetSoftHdr(pMMSoftHdr), softHdrSize);
				buffer	=  (PUCHAR) ( ((ULONG) buffer) + (ULONG) (softHdrSize));
			}
			*Size	+= softHdrSize;
			sizeLeft -= softHdrSize;

			if(DontFreePackets == FALSE){
				// Than Copy Data Pkts
				OS_RtlCopyMemory( buffer, pData, length);
				buffer	=  (PUCHAR) ( ((ULONG) buffer) + (ULONG) (length));
			}
			*Size	+= length;
			sizeLeft -= length;

			OS_ASSERT(pMMProtoHdr->SftkDev != NULL);

			pMMmigrateProtoHdr->SftkDev	= pMMProtoHdr->SftkDev;
			OS_RtlCopyMemory( Proto_MMGetProtoHdr(pMMmigrateProtoHdr), pProtoHdr, protoHdrSize);

			// Free Data Pkt to MM
			OS_ASSERT(pAnchorList != &pQueueMgr->RefreshList);
			if(DontFreePackets == FALSE){
				mm_free_buffer(Sftk_Lg, pMmHolder);
			}else {
				// Insert the MM_HOLDER with the Proto Header into the Temporary MM_HOLDER list
				InsertTailList(&tempMmHolderList.ListEntry , &pMmHolder->MmHolderLink);
			}

			// Insert Proto Hdr into Migrate Queue at Tail List
			pMMmigrateProtoHdr->RawDataSize = length;
			Set_MM_ProtoHdrMsgType(pMMmigrateProtoHdr, Proto_Get_ProtoHdrMsgType(pMMmigrateProtoHdr) );// save original msgtype.
			QM_Insert(Sftk_Lg, pMMmigrateProtoHdr, MIGRATION_QUEUE, FALSE);
			continue;
		} // else : Sentinal Pkts : No IO Data Pkts
	} // While : scan thru each and every Node of current list and free it after processing

done:		
	if (*Size == 0)
	{
		bret = FALSE;
		// Clear the Packets Avalibale Events so that the Sending Threads can Wait on this event until they
		// have something to send
		KeClearEvent(&Sftk_Lg->EventPacketsAvailableForRetrival);
		if(DontFreePackets == TRUE){
			*MmRootMmHolderPtr = NULL;
			InitializeListHead(&pSendBuffer->MmHolderList);
		}
	}
	else
	{
		DebugPrint((DBG_QUEUE, "QM_RetrievePkts: TotalPkts Left: Refresh:%d, RefreshPendingList:%d,Commit:%d, Migrate: %d\n",
				pQueueMgr->RefreshList.NumOfNodes, pQueueMgr->RefreshPendingList.NumOfNodes, 
				pQueueMgr->CommitList.NumOfNodes, pQueueMgr->MigrateList.NumOfNodes));
		if(DontFreePackets == TRUE){
			QM_MakeMdlChainOfSendQueuePackets(&tempMmHolderList,NULL,Size,&pMmRootMmHolder);
			*MmRootMmHolderPtr = pMmRootMmHolder;
			
			// Move the MM_HOLDER List to the SEND_BUFFER, and reinitialize the temp List
			pSendBuffer->MmHolderList.Flink = tempMmHolderList.ListEntry.Flink;
			tempMmHolderList.ListEntry.Flink->Blink = &pSendBuffer->MmHolderList;
			pSendBuffer->MmHolderList.Blink = tempMmHolderList.ListEntry.Blink;
			tempMmHolderList.ListEntry.Blink->Flink = &pSendBuffer->MmHolderList;
			ANCHOR_InitializeListHead(tempMmHolderList);
		}
	}

	OS_RELEASE_LOCK( &pQueueMgr->Lock, NULL);

	if (bret == FALSE) 
	{ // No Data to process !!!
		*Size = 0;
	}

	return bret;	// TRUE means Returning valid Data packets
} // QM_RetrievePkts()

//
// Commit Queue Dispatch API used only for Inband
//
// Return TRUE means Buffer got data else Buffer is untouch/empty.
//
BOOLEAN
QM_RetrievePktsFromCommitQueue(	IN PSFTK_LG		Sftk_Lg,
								IN OUT PVOID	Buffer, 
								IN OUT	PULONG	Size,
								IN BOOLEAN		DontFreePackets,
								OUT PMM_HOLDER	*MmRootMmHolderPtr)	// In Buffer Max size, OUT actual buffer data filled size) 
{
	PQUEUE_MANAGER		pQueueMgr			= &Sftk_Lg->QueueMgr;
	PMM_PROTO_HDR		pMMmigrateProtoHdr	= NULL;	// MM holding Structure of Proto Hdr Pointer for the Migration Queue
	PMM_HOLDER			pMMHolderProtoHdr			= NULL;	// MM holding Structure of Proto Hdr Pointer for the Temp Send Queue
	BOOLEAN				bUsedSoftHdr		= FALSE;
	ULONG				protoHdrSize		= 0;
	ULONG				softHdrSize			= 0;
	PSFTK_DEV			pSftkDev			= NULL;
	PMM_SOFT_HDR		pMMSoftHdr;
	PANCHOR_LINKLIST	pAnchorList, pAnchorMigrateQueue;
	PLIST_ENTRY			plistEntry;
	PMM_HOLDER			pMmHolder;
	ULONG				sizeLeft, length, totalLength, protoTotalLength;
	PUCHAR				buffer, outProtoBuffer;
	PVOID				pData;
	NTSTATUS			status;
	ANCHOR_LINKLIST		tempMmHolderList;
	PMM_HOLDER			pMmRootMmHolder = NULL;
	PSEND_BUFFER		pSendBuffer = NULL;

	// The Buffer is only vaild if we are not using the DontFreePackets Falg, 
	// In this case will copy the Packets to the Send Queue so that they will be
	// Dispatched directly instead of going through the Copy Operation.
	OS_ASSERT( Buffer != NULL);
	if(DontFreePackets == TRUE){
		OS_ASSERT(MmRootMmHolderPtr != NULL);
	}

	sizeLeft		= *Size;
	if(DontFreePackets == FALSE){
		buffer = Buffer;
	}else{
		// Using the Memory Manager allocated Buffers.
		pSendBuffer = (PSEND_BUFFER)Buffer;
	}
	*Size			= 0;
	totalLength		= 0;
	protoTotalLength= 0;

	if(DontFreePackets == FALSE){
		OS_ZeroMemory(Buffer, sizeLeft);
	}
	
	// Use this as Anchor for all the buffers that will be used for creating a chain of HH_HOLER Mdl's
	ANCHOR_InitializeListHead(tempMmHolderList);

	OS_ASSERT( sizeLeft != 0 );

	pAnchorMigrateQueue	= &pQueueMgr->MigrateList;
	pAnchorList		= &pQueueMgr->CommitList;	

	if ( pQueueMgr->MigrateList.NumOfNodes >= Sftk_Lg->throtal_commit_send_pkts )
	{
		DebugPrint((DBG_QUEUE, "^^^ QM_RetrievePktsFromCommitQueue: Holding Pkts...TotalPkts Left: , Commit:%d, Migrate: %d throtal_commit_send_pkts %d, Refresh:%d^^^ \n",
						pQueueMgr->CommitList.NumOfNodes, pQueueMgr->MigrateList.NumOfNodes, 
						Sftk_Lg->throtal_commit_send_pkts,pQueueMgr->RefreshList.NumOfNodes));

		if (pMMmigrateProtoHdr != NULL)
			mm_free_ProtoHdr(Sftk_Lg, pMMmigrateProtoHdr); // mm_type_free( pMMmigrateProtoHdr, MM_TYPE_PROTOCOL_HEADER);

		*Size = 0;
		return FALSE;	// No Data to process !!!
	}

#if DBG
	if ( (Sftk_Lg->state == SFTK_MODE_FULL_REFRESH) || (Sftk_Lg->state == FTD_MODE_SMART_REFRESH) )
	{
		DebugPrint((DBG_PROTO, "??? MUST NOT COME HERE QMCommit: LG %d ??? \n", Sftk_Lg->LGroupNumber));
		OS_ASSERT(FALSE);
	}
#endif

	// Scan from Head List
	while( !(IsListEmpty(&pAnchorList->ListEntry)) )
	{ // While : scan thru each and every Node of current list and free it after processing
		if ( pQueueMgr->MigrateList.NumOfNodes >= Sftk_Lg->throtal_commit_send_pkts )
		{
			DebugPrint((DBG_QUEUE, "^^^ QM_RetrievePktsFromCommitQueue: Holding Pkts...TotalPkts Left: , Commit:%d, Migrate: %d throtal_commit_send_pkts %d, Refresh:%d^^^ \n",
							pQueueMgr->CommitList.NumOfNodes, pQueueMgr->MigrateList.NumOfNodes, 
							Sftk_Lg->throtal_commit_send_pkts,pQueueMgr->RefreshList.NumOfNodes));
			break;
		}

		plistEntry = RemoveHeadList(&pAnchorList->ListEntry);
		pAnchorList->NumOfNodes --;

		pMmHolder = CONTAINING_RECORD( plistEntry, MM_HOLDER, MmHolderLink);
		
		OS_ASSERT( OS_IsFlagSet( pMmHolder->Proto_type, MM_HOLDER_PROTO_TYPE_SOFT_HDR) == TRUE );

		pMMSoftHdr = MM_GetHdr(pMmHolder);
		OS_ASSERT( MM_IsSoftHdr(pMMSoftHdr) == TRUE);

		// Allocate a MM_HOLDER with a MM_PROTO_HDR so that it can be inserted into the Send Queue
		// Which will be used to send data over TDI.
		if(pMMHolderProtoHdr == NULL){
			pMMHolderProtoHdr = mm_alloc_buffer(Sftk_Lg, 0, PROTO_HDR, TRUE);
			if(pMMHolderProtoHdr == NULL){
				DebugPrint((DBG_ERROR, "QM_RetrievePktsFromCommitQueue: For Send Queue mm_alloc_buffer(PROTO_HDR %d) Failed BUG CHECK !! \n",
												PROTO_HDR));
				OS_ASSERT(FALSE);
				InsertHeadList( &pAnchorList->ListEntry, plistEntry);
				pAnchorList->NumOfNodes ++;
				break; // UnderRun Buffer
			}
		}

		if (pMMmigrateProtoHdr == NULL)
		{ // Migration Pkts Allocate once 
			// Allocate and copy Migrate Proto Hdr and move it to Migration Queue
			pMMmigrateProtoHdr = mm_type_alloc(MM_TYPE_PROTOCOL_HEADER);
			if (pMMmigrateProtoHdr == NULL)
			{
				DebugPrint((DBG_ERROR, "QM_RetrievePktsFromCommitQueue: For Migartion Queue mm_type_alloc(MM_TYPE_PROTOCOL_HEADER %d) Failed BUG CHECK !! \n",
												MM_TYPE_PROTOCOL_HEADER));
				OS_ASSERT(FALSE);
				InsertHeadList( &pAnchorList->ListEntry, plistEntry);
				pAnchorList->NumOfNodes ++;
				break; // UnderRun Buffer
			}

			OS_ASSERT(pMMSoftHdr->SftkDev != NULL);

			pMMmigrateProtoHdr->SftkDev	= pMMSoftHdr->SftkDev;

			if (protoHdrSize == 0)
				protoHdrSize = Proto_GetProtoHdrSize(pMMmigrateProtoHdr);

			outProtoBuffer = buffer;
			sizeLeft	-= protoHdrSize;
			*Size		+= protoHdrSize;
			if(DontFreePackets == FALSE){
				buffer	=  (PUCHAR) ( ((ULONG) buffer) + (ULONG) (protoHdrSize));
			}
		} // Migration Pkts Allocate once 

		length = MM_GetLengthFromSoftHdr(pMMSoftHdr);

		if (softHdrSize == 0)
			softHdrSize = Proto_GetSoftHdrSize(pMMSoftHdr);
		
		OS_ASSERT( length > 0);

		if ( (softHdrSize + length) > sizeLeft)
		{ // UnderRun Buffer : insert back unprocessed pkt in Queue at HEAD List
			InsertHeadList( &pAnchorList->ListEntry, plistEntry);
			pAnchorList->NumOfNodes ++;
			break; // UnderRun Buffer
		} // UnderRun Buffer

		if(DontFreePackets == FALSE){
			// Copy Soft Hdr First
			OS_RtlCopyMemory( buffer, Proto_MMGetSoftHdr(pMMSoftHdr), softHdrSize);
			buffer	=  (PUCHAR) ( ((ULONG) buffer) + (ULONG) (softHdrSize));
		}
		*Size	+= softHdrSize;
		sizeLeft -= softHdrSize;

		if(DontFreePackets == FALSE){
			// Than Copy Data Pkts from MM to TDI Buffer
			status = MM_COPY_BUFFER( pMmHolder, buffer, length, FALSE);
			if (status != STATUS_SUCCESS){ // Failed : PANIC !!!
				buffer	=  (PUCHAR) ( ((ULONG) buffer) - (ULONG) (softHdrSize));
				*Size	-= softHdrSize;
				sizeLeft += softHdrSize;

				InsertHeadList( &pAnchorList->ListEntry, plistEntry);
				pAnchorList->NumOfNodes ++;	// try again later this pkt....

				DebugPrint((DBG_ERROR, "FIXME FIXME :: QM_RetrievePktsFromCommitQueue: MM_COPY_BUFFER() Failed with status 0x%08x for Size %d, Ignoring this pkt, will try next time !!! \n",
												status, length));
				OS_ASSERT(FALSE);
				break; // Consider as a UnderRun Buffer
			}
		}
		// update Stats
		Sftk_Lg->Statistics.QM_WrCommit --;
		Sftk_Lg->Statistics.QM_BlksWrCommit -= Get_Values_InSectorsFromBytes(length);
		MM_GetSftkDevFromMM_Holder(pMmHolder, pSftkDev);
		if (pSftkDev)
		{
			pSftkDev->Statistics.QM_WrCommit --;
			pSftkDev->Statistics.QM_BlksWrCommit -= Get_Values_InSectorsFromBytes(length);
		}
		totalLength += length;

		// protoTotalLength = Total SoftHdr size + Total Raw Data size
		protoTotalLength += softHdrSize;
		protoTotalLength += length;

		if (bUsedSoftHdr == FALSE)
			bUsedSoftHdr = TRUE;	// We will insert Soft hdr pkt 
/*
		pData  = MM_GetDataBufferPointer(pMmHolder);
		OS_ASSERT( pData != NULL);
		OS_RtlCopyMemory( buffer, pData, length);
*/
		if(DontFreePackets == FALSE){
			// update buffer after data copy
			buffer	=  (PUCHAR) ( ((ULONG) buffer) + (ULONG) (length));
		}
		*Size	+= length;
		sizeLeft -= length;

		OS_ASSERT(pMMSoftHdr->SftkDev != NULL);

		if(DontFreePackets == FALSE){
			// Remove the Soft Header from the MM_HOLDER and free the MM_HOLDER 
			// Move the Soft header to the Migration packet.
			MM_RemoveSoftHdrFromMMHolder(pMmHolder);
			// Free Data Pkt to MM
			mm_free_buffer(Sftk_Lg, pMmHolder);
			InitializeListHead( &pMMSoftHdr->MmSoftHdrLink );
			// Insert Soft Hdr into Proto Hdr which will go into migration queue
			MM_InsertSoftHdrIntoProtoHdr(pMMmigrateProtoHdr, pMMSoftHdr);
		}else {
			// Insert the MM_HOLDER with the Soft Header into the Temporary MM_HOLDER list
			InsertTailList(&tempMmHolderList.ListEntry , &pMmHolder->MmHolderLink);
		}
		
		if (sizeLeft == 0)
			break;
	} // While : scan thru each and every Node of current list and free it after processing

	if ( (*Size > 0) && (bUsedSoftHdr == TRUE) )
	{
		// TODO Call API To Initialize Migarion ProtoHdr 
		OS_ASSERT(pMMmigrateProtoHdr != NULL );

		// Initialized pMMmigrateProtoHdr
		Proto_MMInitProtoHdrForIOData( pMMmigrateProtoHdr, protoTotalLength,0 );
		Set_MM_ProtoHdrMsgType(pMMmigrateProtoHdr, Proto_Get_ProtoHdrMsgType(pMMmigrateProtoHdr) );// save original msgtype.

		if(DontFreePackets == FALSE){
			// Copy Pkts to 
			OS_RtlCopyMemory( outProtoBuffer, Proto_MMGetProtoHdr(pMMmigrateProtoHdr), protoHdrSize);
		}else{
			// Insert the MM_HOLDER with the Proto Header into the Temporary MM_HOLDER list
			InsertHeadList(&tempMmHolderList.ListEntry , &pMMHolderProtoHdr->MmHolderLink);
		}
		// Insert Proto Hdr into Migration Pkt
		pMMmigrateProtoHdr->RawDataSize = totalLength;
		
		// set flag in it mentioning that it contains proper valid I/O data
		MM_SetProtoHdrHasDataPkt(pMMmigrateProtoHdr);

		QM_Insert(Sftk_Lg, pMMmigrateProtoHdr, MIGRATION_QUEUE, FALSE);

		DebugPrint((DBG_QUEUE, "QM_RetrievePktsFromCommitQueue: TotalPkts Left: Refresh:%d, Commit:%d, Migrate: %d\n",
				pQueueMgr->RefreshList.NumOfNodes, pQueueMgr->CommitList.NumOfNodes, pQueueMgr->MigrateList.NumOfNodes));

		if(DontFreePackets == TRUE){
			QM_MakeMdlChainOfSendQueuePackets(&tempMmHolderList,NULL,Size,&pMmRootMmHolder);
			*MmRootMmHolderPtr = pMmRootMmHolder;

			// Move the MM_HOLDER List to the SEND_BUFFER, and reinitialize the temp List
			pSendBuffer->MmHolderList.Flink = tempMmHolderList.ListEntry.Flink;
			tempMmHolderList.ListEntry.Flink->Blink = &pSendBuffer->MmHolderList;
			pSendBuffer->MmHolderList.Blink = tempMmHolderList.ListEntry.Blink;
			tempMmHolderList.ListEntry.Blink->Flink = &pSendBuffer->MmHolderList;
			ANCHOR_InitializeListHead(tempMmHolderList);
		}

		return TRUE;	// Returning valid Data packets
	}
	else
	{
		if (pMMmigrateProtoHdr != NULL){
			mm_free_ProtoHdr(Sftk_Lg, pMMmigrateProtoHdr); // mm_type_free( pMMmigrateProtoHdr, MM_TYPE_PROTOCOL_HEADER);
		}
		*Size = 0;
		if(DontFreePackets == TRUE){
			if(pMMHolderProtoHdr != NULL){
				mm_free_ProtoHdr(Sftk_Lg, ((PMM_PROTO_HDR)pMMHolderProtoHdr->pProtocolHdr));
				pMMHolderProtoHdr->pProtocolHdr = NULL;
				mm_free(pMMHolderProtoHdr);
				pMMHolderProtoHdr = NULL;
			}
			*MmRootMmHolderPtr = NULL;
			InitializeListHead(&pSendBuffer->MmHolderList);
		}
		return FALSE;	// No Data to process !!!
	}
} // QM_RetrievePktsFromCommitQueue()

//
// Remove Ack Packet from Migration Queue and if its Packet is synchronous than
// signal Pck completion event also.
//
// RetBuffer and RetBuferSize are optional and used only if secondary has sent some extra buffer along with 
// msg ack, this happens for Payload Checksum Refresh case or back refresh case only.
//
// Return TRUE means Ack Packet Pocess succefully, FALSE means ACK packet did not find or 
// error in processing.
// This API will disconnect Connection if this API Fails, Causes some seriouse 
// problem occured in ACK processing, Connection State may get unstable because of that !!
//
// This API supports only and only if Ack packets belong to sender and sender has that packet
// moved into Migration Queue waiting for ACK.
// 
NTSTATUS
QM_RemovePktsFromMigrateQueue(	IN				PSFTK_LG	Sftk_Lg,
								IN	OUT			PVOID		AckPacket,
								IN	OPTIONAL	PVOID		RetBuffer,		// Optional, Recieved extra buffer as payload along with Ack Pkt in recieve thread
								IN	OPTIONAL	ULONG		RetBufferSize )	// Optional, Size of Recieved extra buffer as payload along with Ack Pkt in recieve thread
{
	NTSTATUS			status				= STATUS_SUCCESS;	// Default Success
	PQUEUE_MANAGER		pQueueMgr			= &Sftk_Lg->QueueMgr;
	PMM_PROTO_HDR		pMMmigrateProtoHdr	= NULL;	// MM holding Structure of Proto Hdr Pointer
	PSFTK_DEV			pSftkDev			= NULL;
	PANCHOR_LINKLIST	pAnchorMigrateQueue;
	PLIST_ENTRY			plistEntry;
	
	pAnchorMigrateQueue	= &pQueueMgr->MigrateList;

	OS_ACQUIRE_LOCK( &pQueueMgr->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);

	// Scan from Head List
	if(IsListEmpty(&pAnchorMigrateQueue->ListEntry))
	{ // if List is empty !! BUG 
		status = STATUS_INVALID_DEVICE_REQUEST;
		DebugPrint((DBG_ERROR, "BUG FIXME FIXME *** QM_RemovePktsFromMigrateQueue: Queue is Empty, Why caller called me !! FIXME FIXME, Returning error 0x%08x !! \n",
												MM_TYPE_PROTOCOL_HEADER, status ));
		DebugPrint((DBG_PROTO | DBG_ERROR, "+++ Proto_Compare_and_updateAckHdr(): Recv:MsgId %d,devid:0x%08x,O:%d,len:%d +++\n",
					( (ftd_header_t *) AckPacket)->msgtype, ( (ftd_header_t *) AckPacket)->msg.lg.devid, 
					( (ftd_header_t *) AckPacket)->msg.lg.offset, ( (ftd_header_t *) AckPacket)->msg.lg.len));

		OS_ASSERT(FALSE);
		goto done; // error
	} // if List is empty !! BUG 

	DebugPrint((DBG_QUEUE, "QM_RemovePktsFromMigrateQueue: TotalPkts Left: Refresh:%d, Commit:%d, Migrate: %d\n",
				pQueueMgr->RefreshList.NumOfNodes, pQueueMgr->CommitList.NumOfNodes, pQueueMgr->MigrateList.NumOfNodes));

	plistEntry = RemoveHeadList(&pAnchorMigrateQueue->ListEntry);
	pAnchorMigrateQueue->NumOfNodes --;

	pMMmigrateProtoHdr = CONTAINING_RECORD( plistEntry, MM_PROTO_HDR, MmProtoHdrLink);
	
	OS_ASSERT( MM_IsProtoHdr(pMMmigrateProtoHdr) == TRUE);

	// update stats
	Sftk_Lg->Statistics.QM_WrMigrate --;
	Sftk_Lg->Statistics.QM_BlksWrMigrate -= Get_Values_InSectorsFromBytes(pMMmigrateProtoHdr->RawDataSize);
	pSftkDev = pMMmigrateProtoHdr->SftkDev;
	if (pSftkDev)
	{
		pSftkDev->Statistics.QM_WrMigrate --;
		pSftkDev->Statistics.QM_BlksWrMigrate -= Get_Values_InSectorsFromBytes(pMMmigrateProtoHdr->RawDataSize);
	}

	// Call API 
	status = Proto_Compare_and_updateAckHdr(	Sftk_Lg,
												pMMmigrateProtoHdr, 
												AckPacket, 
												RetBuffer, 
												RetBufferSize );
	if (status == STATUS_TIMER_RESUME_IGNORED)
	{ // reinsert this pkt causes another Ack will come for the same pkt
		// reinsert pkt
		InsertHeadList( &pAnchorMigrateQueue->ListEntry, plistEntry );
		pAnchorMigrateQueue->NumOfNodes ++;

		// update stats
		Sftk_Lg->Statistics.QM_WrMigrate ++;
		Sftk_Lg->Statistics.QM_BlksWrMigrate += Get_Values_InSectorsFromBytes(pMMmigrateProtoHdr->RawDataSize);
		pSftkDev = pMMmigrateProtoHdr->SftkDev;
		if (pSftkDev)
		{
			pSftkDev->Statistics.QM_WrMigrate ++;
			pSftkDev->Statistics.QM_BlksWrMigrate += Get_Values_InSectorsFromBytes(pMMmigrateProtoHdr->RawDataSize);
		}
	}
	else
	{ // status other than STATUS_TIMER_RESUME_IGNORED
		if (!NT_SUCCESS(status))
		{ // failed...
			DebugPrint((DBG_ERROR, "BUG FIXME FIXME *** QM_RemovePktsFromMigrateQueue: Proto_Compare_and_updateAckHdr() Failed FIXME FIXME, Returning error 0x%08x !! \n",
													MM_TYPE_PROTOCOL_HEADER, status ));
			OS_ASSERT(FALSE);

			// insert back this packet and let handled by caller to switch state 
			if ( MM_IsEventValidInMMProtoHdr(pMMmigrateProtoHdr) )
			{	// Synchronous command so signal this completion
				// pEvent = MM_GetEventFromMMProtoHdr( pMMmigrateProtoHdr );
				MM_SignalEventInMMProtoHdr(pMMmigrateProtoHdr);
				// No need to free this packet, Outband Caller will get signalled and then process it and free it later....
			}
			else
			{ // Asynchronous : (may be Inband) : free packet from migrate Queue
				// reinsert pkt
				InsertHeadList( &pAnchorMigrateQueue->ListEntry, plistEntry );
				pAnchorMigrateQueue->NumOfNodes ++;

				// update stats
				Sftk_Lg->Statistics.QM_WrMigrate ++;
				Sftk_Lg->Statistics.QM_BlksWrMigrate += Get_Values_InSectorsFromBytes(pMMmigrateProtoHdr->RawDataSize);
				pSftkDev = pMMmigrateProtoHdr->SftkDev;
				if (pSftkDev)
				{
					pSftkDev->Statistics.QM_WrMigrate ++;
					pSftkDev->Statistics.QM_BlksWrMigrate += Get_Values_InSectorsFromBytes(pMMmigrateProtoHdr->RawDataSize);
				}
				// QM_Insert(Sftk_Lg, pMMmigrateProtoHdr, MIGRATION_QUEUE, FALSE);
			}
			goto done; // error
		}
	}

	if (status == STATUS_SUCCESS)
	{
		// success
		if ( MM_IsEventValidInMMProtoHdr(pMMmigrateProtoHdr) )
		{	// Synchronous command so signal this completion
			// pEvent = MM_GetEventFromMMProtoHdr( pMMmigrateProtoHdr );
			MM_SignalEventInMMProtoHdr(pMMmigrateProtoHdr);
			// No need to free this packet, Outband Caller will get signalled and then process it and free it later....
		}
		else
		{ // Asynchronous : (may be Inband) : free packet from migrate Queue
			mm_free_ProtoHdr( Sftk_Lg, pMMmigrateProtoHdr);
		}
	}

	// Signal event for resfresh mentioning that refresh is done.
	if ( (Sftk_Lg->RefreshFinishedParseI == TRUE)	&&									// Refresh Read is completely done for this LG
		 (Sftk_Lg->RefreshIOPkts.NumOfNodes == Sftk_Lg->NumOfAllocatedRefreshIOPkts) && // RefreshMMPool has all pkts free	
		 (pQueueMgr->RefreshList.NumOfNodes == 0)	&&									// RefreshQueue Is empty
		 (pQueueMgr->MigrateList.NumOfNodes == 0)	)									// MigrateQueue Is empty
	{
		DebugPrint((DBG_ERROR, "QM_RemovePktsFromMigrateQueue:: Lg %d Signalling RefreshParse Done !!! \n", Sftk_Lg->LGroupNumber));
		KeSetEvent( &Sftk_Lg->RefreshEmptyAckQueueEvent, 0, FALSE);
	}

	status = STATUS_SUCCESS;	// successfully done

done:
	OS_RELEASE_LOCK( &pQueueMgr->Lock, NULL);	

	if (!NT_SUCCESS(status))
	{ // failed migrate retrieval, ops disconnect thread and free all packets
		NTSTATUS	tmpStatus;
		WCHAR		wchar[35];

		DebugPrint((DBG_ERROR, "QM_RemovePktsFromMigrateQueue: Failed status 0x%08x, caller will do Reconnection and Changing state so all Pkt gets free after Reseting connections !! \n",
												status ));
		swprintf(wchar, L"%d", Sftk_Lg->LGroupNumber);
		sftk_LogEventString1(GSftk_Config.DriverObject, MSG_REPL_DRIVER_PROTOCOL_EXCHANGE_ORDER_ERROR, status, 0, wchar);

		// Ntw needed Reset Connection, caller will do that....
		/*
		DebugPrint((DBG_ERROR, "QM_RemovePktsFromMigrateQueue: Failed status 0x%08x, Changing state to tracking so all Pkt gets free and Reseting connections !! \n",
												status ));
		Sftk_Lg->DoNotSendOutBandCommandForTracking = TRUE; // so not need to send Tracking outband FTDCHUP command
		tmpStatus = sftk_lg_change_State( Sftk_Lg, Sftk_Lg->state, SFTK_MODE_TRACKING, FALSE);
		if (!NT_SUCCESS(tmpStatus))
		{
			DebugPrint((DBG_ERROR, "QM_RemovePktsFromMigrateQueue: sftk_lg_change_State: failed status 0x%08x, ignoring it !!! FIXME FIXME !! \n",
												tmpStatus ));
			OS_ASSERT(FALSE);
		}
		tmpStatus = COM_ResetAllConnections( &Sftk_Lg->SessionMgr );
		if (!NT_SUCCESS(tmpStatus))
		{
			DebugPrint((DBG_ERROR, "QM_RemovePktsFromMigrateQueue: COM_ResetAllConnections: failed status 0x%08x, ignoring it !!! FIXME FIXME !! \n",
												tmpStatus ));
			OS_ASSERT(FALSE);
		}
		*/
	} // failed migrate retrieval, ops disconnect thread and free all packets

	return status;
} // QM_RemovePktsFromMigrateQueue()

NTSTATUS
QM_SendOutBandPkt(	PSFTK_LG		Sftk_Lg, 
					BOOLEAN			WaitForExecute, 
					BOOLEAN			AckExepected, 
					enum protocol	MsgType)
{
	NTSTATUS		status		= STATUS_SUCCESS;
	PMM_PROTO_HDR	pMMProtoHdr	= NULL;
	PMM_SOFT_HDR	pMMSoftHdr	= NULL;
	PVOID			pRawBuffer	= NULL;
	PQUEUE_MANAGER	pQueueMgr	= &Sftk_Lg->QueueMgr;
	ULONG			length		= 0;
	KEVENT			event;
	SFTK_IO_VECTOR	vector[4];
	LARGE_INTEGER	wait_timeout;
	
	// check connection is on, if not return error....
	if (sftk_lg_is_socket_alive( Sftk_Lg ) == FALSE)
	{ // connection is OFF !! Wait for connection and then Return error, can't do anything here...
		wait_timeout.QuadPart = DEFAULT_TIMEOUT_FOR_CONNECTION_WAIT_SLEEP; // relative 30 seconds

		DebugPrint((DBG_ERROR, "QM_SendOutBandPkt: sftk_lg_is_socket_alive(LGNum %d) == FALSE, waiting for Connection with timeout %I64x !!! \n",
												Sftk_Lg->LGroupNumber, wait_timeout.QuadPart));
		// Wait here with timeout if handshake gets done then continue else return error
		status = KeWaitForSingleObject(	(PVOID) &Sftk_Lg->SessionMgr.GroupConnectedEvent,
												Executive,
												KernelMode,
												FALSE,
												&wait_timeout );
		switch(status)
		{
			case STATUS_TIMEOUT:
								status = STATUS_INVALID_CONNECTION;
								DebugPrint((DBG_THREAD, "QM_SendOutBandPkt:: LG %d,KeWaitForSingleObject(Connection, Timeout %I64x occurred): Connection Is not done...returning error 0x%08x!!\n", 
														Sftk_Lg->LGroupNumber, wait_timeout.QuadPart, status ));
								return status;
		}
		if (sftk_lg_is_socket_alive( Sftk_Lg ) == FALSE)
		{ // still handshake not done....
			status = STATUS_REMOTE_DISCONNECT;
			DebugPrint((DBG_ERROR, "QM_SendOutBandPkt: sftk_lg_is_socket_alive(LGNum %d, After Timeout 0x%I64x) == FALSE, returning status 0x%08x!!! \n",
												Sftk_Lg->LGroupNumber, wait_timeout.QuadPart, status ));
			return status;
		}
	} // connection is OFF !! Wait for connection and then Return error, can't do anything here...

	// Check Handshake is done if not wait for it with timeour then return error if it fails
	if (sftk_lg_is_sessionmgr_Handshack_done( Sftk_Lg ) == FALSE)
	{ // Handshake is not done yet!! 
		wait_timeout.QuadPart = DEFAULT_TIMEOUT_FOR_OUTBAND_HANDSHAKE_SLEEP; // relative 30 seconds

		DebugPrint((DBG_ERROR, "QM_SendOutBandPkt: sftk_lg_is_sessionmgr_Handshack_done(LGNum %d) == FALSE, waiting for handshake timeout %I64x !!! \n",
												Sftk_Lg->LGroupNumber, wait_timeout.QuadPart));
		// Wait here with timeout if handshake gets done then continue else return error
		status = KeWaitForSingleObject(	(PVOID) &Sftk_Lg->SessionMgr.HandshakeEvent,
												Executive,
												KernelMode,
												FALSE,
												&wait_timeout );
		switch(status)
		{
			case STATUS_TIMEOUT:
								status = STATUS_INVALID_CONNECTION;
								DebugPrint((DBG_THREAD, "QM_SendOutBandPkt:: LG %d,KeWaitForSingleObject(Timeout %I64x occurred): HANDSHAKE Is not done...returning error 0x%08x!!\n", 
														Sftk_Lg->LGroupNumber, wait_timeout.QuadPart, status ));
								return status;
		}
		if (sftk_lg_is_sessionmgr_Handshack_done( Sftk_Lg ) == FALSE)
		{ // still handshake not done....
			status = STATUS_INVALID_CONNECTION;
			DebugPrint((DBG_THREAD, "QM_SendOutBandPkt:: LG %d, HandSHake is not done, returning error 0x%08x!!\n", 
														Sftk_Lg->LGroupNumber, status ));
			return status;
		}
	} // Handshake is not done yet!! 
	
	if (PROTO_GetAckOfCommand(MsgType) == -1)
	{ // we can't get Ack back !!!
		WaitForExecute	= FALSE;
		AckExepected	= FALSE;
	}
	else
	{
		AckExepected = TRUE;
	}

	OS_ZeroMemory( vector, sizeof(vector));

	// Allocate memory for proto-hdr and raw memory of 512 bytes
	pMMProtoHdr = mm_type_alloc(MM_TYPE_PROTOCOL_HEADER);
	if (!pMMProtoHdr)
	{ // Error 
		status = STATUS_INSUFFICIENT_RESOURCES;
		DebugPrint((DBG_ERROR, "QM_SendOutBandPkt: mm_alloc_buffer(%d Alloc for ProtoHdr) failed status 0x%08x, ignoring it !!! FIXME FIXME !! \n",
												512, status ));
		goto done;
	}

	OS_ASSERT( MM_IsProtoHdr(pMMProtoHdr) == TRUE);

	if (WaitForExecute == TRUE)
	{
		KeInitializeEvent( &event, NotificationEvent, FALSE);
		MM_SetEventInMMProtoHdr(pMMProtoHdr, &event);	// Set event 
	}

	if ( Proto_IsSoftHdrNeedForMsg(MsgType) == TRUE)
	{
		length = 512;
		pRawBuffer = OS_AllocMemory(NonPagedPool, 512);
		if (!pRawBuffer)
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			DebugPrint((DBG_ERROR, "QM_SendOutBandPkt: OS_AllocMemory(%d Alloc for RawBuffer) failed status 0x%08x, ignoring it !!! FIXME FIXME !! \n",
													512, status ));
			if (pMMProtoHdr)
				mm_free_ProtoHdr(Sftk_Lg, pMMProtoHdr);
			goto done;
		}
		OS_ZeroMemory( pRawBuffer, 512);

		pMMSoftHdr = mm_type_alloc(MM_TYPE_SOFT_HEADER);
		if (!pMMSoftHdr)
		{ // Error 
			status = STATUS_INSUFFICIENT_RESOURCES;
			DebugPrint((DBG_ERROR, "QM_SendOutBandPkt: mm_type_alloc(Alloc for SOFTHDR) failed status 0x%08x, ignoring it !!! FIXME FIXME !! \n",
													status ));
			if (pMMProtoHdr)
				mm_free_ProtoHdr(Sftk_Lg, pMMProtoHdr);

			goto done;
		}
		// Insert Soft Hdr into Proto Hdr which will go into migration queue
		InitializeListHead( &pMMSoftHdr->MmSoftHdrLink );
		MM_InsertSoftHdrIntoProtoHdr(pMMProtoHdr, pMMSoftHdr);

		// Now We have ProtoHdr + SoftHdr + 512 Raw Data memory
		// Prepare Protocol Pkts by calling this API
		status = PROTO_PrepareProtocolSentinal(	MsgType,							// eProtocol, 
												Proto_MMGetProtoHdr(pMMProtoHdr),	// *protocolHeader		
												Proto_MMGetSoftHdr(pMMSoftHdr),		// *softHeader,	
												pRawBuffer,							//RawBuffer,			
												512,								// RawBufferSize,	// RawBufferSize
												-1,									// Offset,	
												0,									// Size,
												Sftk_Lg->LGroupNumber,
												-1);			// Dev_Number
	
	}
	else
	{
		// sending only ProtoHdr 
		// Prepare Protocol Pkts by calling this API
		status = PROTO_PrepareProtocolSentinal(	MsgType,							// eProtocol, 
												Proto_MMGetProtoHdr(pMMProtoHdr),	// *protocolHeader		
												NULL,		// *softHeader,	
												NULL,// RawBuffer,			
												0,								// RawBufferSize,	// RawBufferSize
												-1,									// Offset,	
												0,									// Size,
												Sftk_Lg->LGroupNumber,
												-1);			// Dev_Number
	}

	if (!NT_SUCCESS(status))
	{ // Error 
		DebugPrint((DBG_ERROR, "QM_SendOutBandPkt: PROTO_PrepareProtocolSentinal(LG %d Protocol %d) failed status 0x%08x, ignoring it !!! FIXME FIXME !! \n",
												Sftk_Lg->LGroupNumber, MsgType, status ));
		if (pMMProtoHdr)
			mm_free_ProtoHdr(Sftk_Lg, pMMProtoHdr);
			
		goto done;
	}

	Proto_MMSetStatusInProtoHdr( pMMProtoHdr, STATUS_REMOTE_DISCONNECT);

	OS_ACQUIRE_LOCK( &pQueueMgr->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);

	// TODO : Send this pkt to available session directly, and this api will move this pkt 
	// to migration queue once it sent.
	if ( Proto_IsSoftHdrNeedForMsg(MsgType) == TRUE)
	{
		vector[0].pBuffer = Proto_MMGetProtoHdr(pMMProtoHdr);	// Proto Hdr
		vector[0].uLength = Proto_GetProtoHdrSize(pMMProtoHdr);

		vector[1].pBuffer = Proto_MMGetSoftHdr(pMMSoftHdr);	// Soft Hdr
		vector[1].uLength = Proto_GetSoftHdrSize(pMMSoftHdr);

		vector[2].pBuffer = pRawBuffer;	// Raw Data 
		vector[2].uLength = 512;

		// Send Proto HDr
		status = COM_SendBufferVector( vector, 3, &Sftk_Lg->SessionMgr );
	}
	else
	{ // send ProtoHdr Only
		vector[0].pBuffer = Proto_MMGetProtoHdr(pMMProtoHdr);	// Proto Hdr
		vector[0].uLength = Proto_GetProtoHdrSize(pMMProtoHdr);

		// Send Proto HDr
		status = COM_SendBufferVector( vector, 1, &Sftk_Lg->SessionMgr );
		// Send Raw Data 
	}

	if (!NT_SUCCESS(status))
	{ // Error 
		OS_RELEASE_LOCK( &pQueueMgr->Lock, NULL);	

		DebugPrint((DBG_ERROR, "QM_SendOutBandPkt: COM_SendBufferVector(LG %d Protocol %d) failed status 0x%08x, ignoring it !!! FIXME FIXME !! \n",
												Sftk_Lg->LGroupNumber, MsgType, status ));
		if (pMMProtoHdr)
			mm_free_ProtoHdr(Sftk_Lg, pMMProtoHdr);
			
		goto done;
	}

	if (AckExepected == FALSE)
	{ // Ack is not expected !!
		OS_RELEASE_LOCK( &pQueueMgr->Lock, NULL);	

		// Free Pkts here 
		if (pMMProtoHdr)
			mm_free_ProtoHdr(Sftk_Lg, pMMProtoHdr);

		goto done;
	}

	// This is the case where Ack Expected
	// Now move this ProtoHDr into Migartion queue for Ack Wanted pkt
	pMMProtoHdr->RawDataSize = length;
	
	Set_MM_ProtoHdrMsgType( pMMProtoHdr, MsgType );// save original msgtype.
	QM_Insert(Sftk_Lg, pMMProtoHdr, MIGRATION_QUEUE, FALSE);

	OS_RELEASE_LOCK( &pQueueMgr->Lock, NULL);	
	
	if (WaitForExecute == TRUE)
	{ // Wait here to complete this pkt ack from secondary
		if (Sftk_Lg->MaxOutBandPktWaitTimeout.QuadPart == 0)
			Sftk_Lg->MaxOutBandPktWaitTimeout.QuadPart = DEFAULT_TIMEOUT_FOR_MAX_WAIT_FOR_OUTBAND_PKT;

		status = KeWaitForSingleObject(	MM_GetEventFromMMProtoHdr(pMMProtoHdr),
										Executive,
										KernelMode,
										FALSE,
										&Sftk_Lg->MaxOutBandPktWaitTimeout );	// wait... indifinte for time being 
		if (status != STATUS_SUCCESS)
		{ // Timeout may happened, or any other error
			// no need to remove pkt causes anyhow connection will get reset, queue will get empty
			// just set event = NULL and do not free this pkt, which will get free when Queue will get empty by change state...
			DebugPrint((DBG_ERROR, "QM_SendOutBandPkt: KeWaitForSingleObject(LG %d Protocol %d) failed due to timeout status 0x%08x, Satisfiing request with error !! \n",
												Sftk_Lg->LGroupNumber, MsgType, status ));
		}

		if ( MM_IsEventValidInMMProtoHdr(pMMProtoHdr) )
		{	// Synchronous command so signal this completion
			if (KeReadStateEvent( MM_GetEventFromMMProtoHdr(pMMProtoHdr)) != 0)
			{ // its in signal state. we have to free this....
				status = STATUS_SUCCESS; // so we can free this....
			}
			MM_SetEventInMMProtoHdr(pMMProtoHdr, NULL);	// Set event to NULL not needed anymore... so mm_free_ProtoHdr() will free this packet
		}
	}
	else
	{ // for Asynchronouse, just returned 
		// QM will free this pkt later once Asyn Ack arrives
		goto done;
	}

	if (status == STATUS_SUCCESS)
	{ // free pkt here explictly...
		// synchonous execution done successfully...
		// we are done. so retrieve and return status to caller for this pkt
		status = Proto_MMGetStatusFromProtoHdr( pMMProtoHdr );	// get original status of pkt
		if (pMMProtoHdr)
			mm_free_ProtoHdr(Sftk_Lg, pMMProtoHdr);
	}

done:
	if (pRawBuffer)
		OS_FreeMemory(pRawBuffer);

	return status;
} // QM_SendOutBandPkt

