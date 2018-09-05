/**************************************************************************************

Module Name: sftk_OS.C   
Author Name: Parag sanghvi
Description: Os Dependant API are defined here.
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2002 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/
#include <sftk_main.h>

#ifdef WINDOWS_NT

BOOLEAN	
OS_INITIALIZE_LOCK( PVOID	PtrLock, UCHAR TypeOfLock, PVOID Reserved1)
{
	switch( TypeOfLock )
	{
		case OS_ERESOURCE_LOCK:
			{
				POS_ERESOURCE pLock = (POS_ERESOURCE) PtrLock;
				pLock->TypeOfLock = TypeOfLock;

				ExInitializeResourceLite( &pLock->Lock );
				return TRUE;
			}
			break;

		case OS_FAST_MUTEX_LOCK:
			{
				POS_FAST_MUTEX pLock = (POS_FAST_MUTEX) PtrLock;
				pLock->TypeOfLock = TypeOfLock;

				ExInitializeFastMutex( &pLock->Lock);
				return TRUE;
			}
			break;

		case OS_SPIN_LOCK:
			{
				POS_KSPIN_LOCK pLock = (POS_KSPIN_LOCK) PtrLock;
				pLock->TypeOfLock = TypeOfLock;

				KeInitializeSpinLock( &pLock->Lock );
				return TRUE;
			}
			break;

		case OS_SEMAPHORE_LOCK:
			{
				POS_KSEMAPHORE pLock = (POS_KSEMAPHORE) PtrLock;
				pLock->TypeOfLock = TypeOfLock;

				KeInitializeSemaphore(	&pLock->Lock, 
										0,				// LONG Count:Specifies the initial count value to be assigned to the semaphore
										MAXLONG);		// LONG Limit:Specifies the maximum count value that the semaphore can attain
				return TRUE;
			}
			break;

		default:
			DebugPrint((DBG_OS_APIS, "OS_INITIALIZE_LOCK: FIXME FIXME: Unknown TypeOfLock 0x%x....\n",TypeOfLock ));
			break;
	} //  switch( ( TypeOfLock )

	return FALSE;	// Failed 
} // OS_INITIALIZE_LOCK()


VOID
OS_DEINITIALIZE_LOCK( PVOID	PtrLock, PVOID Reserved1)
{
	switch( ( ((POS_ERESOURCE)PtrLock)->TypeOfLock) )
	{
		case OS_ERESOURCE_LOCK:
			{
				POS_ERESOURCE pLock = (POS_ERESOURCE) PtrLock;
				ExDeleteResourceLite( &pLock->Lock );
			}
			break;

		case OS_FAST_MUTEX_LOCK:
			break;	// Noting to do... ...

		case OS_SPIN_LOCK:
			break;	// Noting to do... ...

		case OS_SEMAPHORE_LOCK:
			break;	// Noting to do... ...

		default:
			DebugPrint((DBG_OS_APIS, "OS_DEINITIALIZE_LOCK: FIXME FIXME: Unknown TypeOfLock 0x%x....\n",(UCHAR) ( ((POS_ERESOURCE)PtrLock)->TypeOfLock) ));
			break;
	} //  switch( ( ((POS_ERESOURCE)PtrLock)->TypeOfLock) )

	return;	
} // OS_DEINITIALIZE_LOCK()


BOOLEAN	
OS_ACQUIRE_LOCK( PVOID	PtrLock, UCHAR LockAccess, BOOLEAN Wait, PVOID Reserved1)
{
	switch( ( ((POS_ERESOURCE)PtrLock)->TypeOfLock) )
	{
		case OS_ERESOURCE_LOCK:
			{
				POS_ERESOURCE pLock = (POS_ERESOURCE) PtrLock;

				switch(LockAccess)
				{
					case OS_ACQUIRE_EXCLUSIVE:

						// ExIsResourceAcquiredExclusiveLite returns TRUE if the caller already has exclusive 
						// access to the given resource
						if ( ExIsResourceAcquiredExclusiveLite(&pLock->Lock) == FALSE)
						{ // if Lock is not acquired by current thread than only acquired
								
							// Callers of ExAcquireResourceExclusiveLite must be running at IRQL < DISPATCH_LEVEL
							OS_ASSERT( (KeGetCurrentIrql() < DISPATCH_LEVEL) );
							return ExAcquireResourceExclusiveLite(&pLock->Lock, Wait);
						}
						else
						{
							DebugPrint((DBG_OS_APIS, "OS_ACQUIRE_LOCK: FIXME FIXME: Recursive Lock OS_ERESOURCE_LOCK, for OS_ACQUIRE_EXCLUSIVE....\n"));
							OS_ASSERT(FALSE);
						}
						break;

					case OS_ACQUIRE_SHARED:

						//	ExIsResourceAcquiredSharedLite returns the number of times the caller has acquired 
						// the given resource for shared or exclusive access
						if ( ExIsResourceAcquiredSharedLite(&pLock->Lock) == 0 )
						{ // if Lock is not acquired by current thread than only acquired
							// Callers of ExAcquireResourceSharedLite must be running at IRQL < DISPATCH_LEVEL
							OS_ASSERT( (KeGetCurrentIrql() < DISPATCH_LEVEL) );
							return ExAcquireResourceSharedLite(&pLock->Lock, Wait);
						}
						else
						{
							DebugPrint((DBG_OS_APIS, "OS_ACQUIRE_LOCK: FIXME FIXME: Recursive Lock OS_ERESOURCE_LOCK, for OS_ACQUIRE_SHARED....\n"));
							OS_ASSERT(FALSE);
						}
						break;

					default:
						DebugPrint((DBG_OS_APIS, "OS_ACQUIRE_LOCK: FIXME FIXME: Type OS_ERESOURCE_LOCK: Unknown LockAccess 0x%x....\n",LockAccess));
						break;
				}
			}
			break;

		case OS_FAST_MUTEX_LOCK:
			{
				POS_FAST_MUTEX pLock = (POS_FAST_MUTEX) PtrLock;
				switch(LockAccess)
				{
					case OS_ACQUIRE_EXCLUSIVE:
						
						// Callers of ExAcquireFastMutex must be running at IRQL < DISPATCH_LEVEL
						OS_ASSERT( (KeGetCurrentIrql() < DISPATCH_LEVEL) );

						// The ExAcquireFastMutex routine acquires the given fast mutex with APCs to the current thread disabled. 
						// ExAcquireFastMutex sets the IRQL to APC_LEVEL, and the caller continues to run at APC_LEVEL 
						// after ExAcquireFastMutex returns
						ExAcquireFastMutex(&pLock->Lock);
						return TRUE;

					case OS_ACQUIRE_SHARED:
					default:
						DebugPrint((DBG_OS_APIS, "OS_ACQUIRE_LOCK: FIXME FIXME: Type OS_FAST_MUTEX_LOCK: Unknown LockAccess 0x%x....\n",LockAccess));
						OS_ASSERT(FALSE);
						break;
				}
			}
			break;

		case OS_SPIN_LOCK:
			{
				POS_KSPIN_LOCK pLock = (POS_KSPIN_LOCK) PtrLock;

				switch(LockAccess)
				{
					case OS_ACQUIRE_EXCLUSIVE:
					case OS_ACQUIRE_SHARED:
					default:
						// Callers of KeAcquireSpinLock must be running at IRQL <= DISPATCH_LEVEL
						OS_ASSERT( (KeGetCurrentIrql() <= DISPATCH_LEVEL) );

						KeAcquireSpinLock( &pLock->Lock, &pLock->Irql);
						return TRUE;
						// DebugPrint((DBG_OS_APIS, "OS_ACQUIRE_LOCK: FIXME FIXME: Type OS_SPIN_LOCK: Unknown LockAccess 0x%x....\n",LockAccess));
				}
			}
			break;

		case OS_SEMAPHORE_LOCK:
			{
				POS_KSEMAPHORE pLock = (POS_KSEMAPHORE) PtrLock;

				switch(LockAccess)
				{
					case OS_ACQUIRE_EXCLUSIVE:

						// Callers of KeWaitForSingleObject must be running at IRQL <= DISPATCH_LEVEL. Usually, the 
						// caller must be running at IRQL = PASSIVE_LEVEL and in a nonarbitrary thread context. A call 
						// while running at IRQL = DISPATCH_LEVEL is valid if and only if the caller specifies a Timeout 
						// of zero. That is, a driver must not wait for a nonzero interval at IRQL = DISPATCH_LEVEL.
						OS_ASSERT( (KeGetCurrentIrql() <= DISPATCH_LEVEL) );

						KeWaitForSingleObject(	(PVOID) &pLock->Lock,
												Executive,
												KernelMode,
												FALSE,			// No Alert from APC or other APC level thread while in wait
												NULL );			// indefinite timeout

						return TRUE;
						
					case OS_ACQUIRE_SHARED:
					default:
						DebugPrint((DBG_OS_APIS, "OS_ACQUIRE_LOCK: FIXME FIXME: Type OS_SEMAPHORE_LOCK: Unknown LockAccess 0x%x....\n",LockAccess));
						OS_ASSERT(FALSE);
						break;
				}
			}
			break;

		default:
			DebugPrint((DBG_OS_APIS, "OS_ACQUIRE_LOCK: FIXME FIXME: Unknown TypeOfLock 0x%x....\n",(UCHAR) ( ((POS_ERESOURCE)PtrLock)->TypeOfLock) ));
			break;
	} //  switch( ( ((POS_ERESOURCE)PtrLock)->TypeOfLock) )

	OS_ASSERT(FALSE);
	return FALSE;	// Failed 
} // OS_ACQUIRE_LOCK()

// TRUE means Release lock successfully else Failed.
BOOLEAN	
OS_RELEASE_LOCK( PVOID	PtrLock, PVOID Reserved1)
{
	switch( ( ((POS_ERESOURCE)PtrLock)->TypeOfLock) )
	{
		case OS_ERESOURCE_LOCK:
			{
				POS_ERESOURCE pLock = (POS_ERESOURCE) PtrLock;

				// Callers of ExReleaseResourceLite must be running at IRQL <= DISPATCH_LEVEL
				OS_ASSERT( (KeGetCurrentIrql() <= DISPATCH_LEVEL) );	

				ExReleaseResourceLite( &pLock->Lock );	// Releasing calling thread based acquired Eresource
				return TRUE;
			}
			break;

		case OS_FAST_MUTEX_LOCK:
			{
				POS_FAST_MUTEX pLock = (POS_FAST_MUTEX) PtrLock;

				// Callers of ExReleaseFastMutex must be running at IRQL = APC_LEVEL. In most cases the IRQL will 
				// already be set to APC_LEVEL before ExReleaseFastMutex is called. This is because 
				// ExAcquireFastMutex has already set the IRQL to the appropriate value automatically. 
				// However, if the caller changes the IRQL after ExAcquireFastMutex returns, the caller must 
				// explicitly set the IRQL to APC_LEVEL prior to calling ExReleaseFastMutex
				OS_ASSERT( (KeGetCurrentIrql() == APC_LEVEL) );

				ExReleaseFastMutex(&pLock->Lock);
				return TRUE;
			}
			break;

		case OS_SPIN_LOCK:
			{
				POS_KSPIN_LOCK pLock = (POS_KSPIN_LOCK) PtrLock;
				
				// Callers of this routine are running at IRQL = DISPATCH_LEVEL. On return from 
				// KeReleaseSpinLock, IRQL is restored to the NewIrql value.
				OS_ASSERT( (KeGetCurrentIrql() == DISPATCH_LEVEL) );

				KeReleaseSpinLock( &pLock->Lock, pLock->Irql);
				return TRUE;
			}
			break;

		case OS_SEMAPHORE_LOCK:
			{
				POS_KSEMAPHORE pLock = (POS_KSEMAPHORE) PtrLock;

				// Callers of KeReleaseSemaphore must be running at IRQL <= DISPATCH_LEVEL provided that Wait is 
				// set to FALSE. Otherwise, the caller must be running at IRQL = PASSIVE_LEVEL.
				OS_ASSERT( (KeGetCurrentIrql() <= DISPATCH_LEVEL) );

				KeReleaseSemaphore(	&pLock->Lock, 
									0,				// KPRIORITY Increment, : Specifies the priority increment to be applied if releasing the semaphore causes a wait to be satisfied
									1,				// LONG Adjustment : Specifies a value to be added to the current semaphore count. This value must be positive.
									FALSE);			// Specifies whether the call to KeReleaseSemaphore is to be followed immediately by a call to one of the KeWaitXxx

				// If the KeReleaseSemaphore() return value is zero, the previous state of the semaphore object is not signaled.
				return TRUE;
			}
			break;

		default:
			OS_ASSERT(FALSE);
			DebugPrint((DBG_OS_APIS, "OS_RELEASE_LOCK: FIXME FIXME: Unknown TypeOfLock 0x%x....\n",(UCHAR) ( ((POS_ERESOURCE)PtrLock)->TypeOfLock) ));
			break;
	} // switch( ( ((POS_ERESOURCE)PtrLock)->TypeOfLock) )

	OS_ASSERT(FALSE);
	return FALSE;
} // OS_RELEASE_LOCK()

#endif // WINDOWS_NT

#ifndef _BITMAP_API_
#define _BITMAP_API_

#define _BITMAP_BYTE_OFFSET(bit)            ((bit) >> 3)    // bit / 8

#define _BITMAP_BYTE_MASK(bit)      (UCHAR) (1 << ((bit) & 0x7))

// _BITMAP_GETSIZE - makes nbits 8 byte allinged !!!
#define _BITMAP_GETSIZE(nbits)      (ULONG) ((((nbits) - 1) >> 3) + 1)

//
// ULONG Bitmap Macros
//
#define _BITMAP_ULONG_OFFSET(bit)					((bit) >> 5)    // bit / 8
#define _BITMAP_ULONG_MASK(bit)				(ULONG) (1 << ((bit) & 0x1F))
#define _BITMAP_ULONG_GETSIZE(nbits)		(ULONG) ((((nbits) - 1) >> 5) + 1)

#define MAX_ULONG_BIT_NUMBER				32
#define INVALID_ULONG_BIT_NUMBER			(MAX_ULONG_BIT_NUMBER + 1)

// TODO : define following lines, resolve compilation error when enable this line
// #define INLINE __inline
#define INLINE

//
// NOTE: Bitnumber range is 0-31 for ULONG Bitmap
//
INLINE
VOID
OS_ULong_SetBit( PULONG	UBit, UCHAR	BitNo)
{
	*UBit |= _BITMAP_ULONG_MASK(BitNo);
}

INLINE
VOID
OS_ULong_ClearBit( PULONG	UBit, UCHAR	BitNo)
{
	*UBit &= ~(_BITMAP_ULONG_MASK(BitNo));
}

INLINE
VOID
OS_ULong_SetBitRange( PULONG	UBit, UCHAR	StartingBitNo, UCHAR NumOfBit)
{
	UCHAR i;
	for (i=StartingBitNo; i < NumOfBit; i++)
		OS_ULong_SetBit( UBit, i);
}

INLINE
VOID
OS_ULong_ClearBitRange( PULONG	UBit, UCHAR	StartingBitNo, UCHAR NumOfBit)
{
	UCHAR i;

	for (i=StartingBitNo; i < NumOfBit; i++)
		OS_ULong_ClearBit( UBit, i);
}

INLINE
BOOLEAN
OS_ULong_IsBitSet( PULONG	UBit, UCHAR	BitNo)
{
	return((*UBit & (_BITMAP_ULONG_MASK(BitNo))) != 0);
}

INLINE
BOOLEAN
OS_ULong_IsBitClear( PULONG	UBit, UCHAR	BitNo)
{
	return( !OS_ULong_IsBitSet(UBit, BitNo) );
}
INLINE
UCHAR
OS_ULong_GetNextSetBit( PULONG	UBit, UCHAR	StartingBitIndex)
{
	UCHAR	bitno;
	
	for (bitno=StartingBitIndex; bitno < MAX_ULONG_BIT_NUMBER; bitno++ )
        {
        if (OS_ULong_IsBitSet(UBit, bitno) == TRUE)
            return bitno;
        }
  return INVALID_ULONG_BIT_NUMBER;
}

UCHAR
OS_ULong_GetNextClearBit( PULONG	UBit, UCHAR	StartingBitIndex)
{
	UCHAR	bitno;
	
	for (bitno=StartingBitIndex; bitno < MAX_ULONG_BIT_NUMBER; bitno++ )
        {
        if (OS_ULong_IsBitSet(UBit, bitno) == FALSE)
            return bitno;
        }
  return INVALID_ULONG_BIT_NUMBER;
}
#endif // _BITMAP_API_