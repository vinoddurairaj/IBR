/*HDR************************************************************************
 *                                                                         
 * Softek -                                     
 *
 *===========================================================================
 *
 * N sftkprotocol.c
 * P Replicator 
 * S Cache & FIFO
 * V Generic
 * A J. Christatos - jchristatos@softek.fujitsu.com
 * D 03.05.2004
 * O This file contains the Protocol API that will be used to exchange 
 *   data between the primary and the secondary.
 * T Cache design requirements and design specifications - v 1.0.0.
 * C DBG - _KERNEL_
 * H 04.28.2004 - Creation - 
 *
 *===========================================================================
 *
 * rcsid[]="@(#) $Id: sftkprotocol.c,v 1.1 2004/05/29 00:45:28 vqba0 Exp $"
 *
 *HDR************************************************************************/

#include "common.h"

#ifdef   WIN32
#ifdef   _KERNEL
#include <ntddk.h>
#include <wchar.h>      // for time_t, dev_t
#include "mmgr_ntkrnl.h"
#else    /* !_KERNEL_*/
#include <windows.h>
#include <stdio.h>
#include <sys/types.h>
#include "mmgr_ntusr.h"
#endif   /* _KERNEL_ */
#endif   /* _WINDOWS */

#include "slab.h"
#include "mmg.h"
#include "dtb.h"
#include "sftkprotocol.h"

/*
 * SftkPrepareSentinal - 
 *
 */
OS_NTSTATUS 
SftkPrepareSentinal( IN enum sentinals eSentinal, 
					 IN OUT wlheader_t *softHeader, 
					 IN rplcc_iorp_t   *inputData,
					 OUT PVOID outputData)

/**/
{
	OS_NTSTATUS Status = STATUS_SUCCESS;

	MMGDEBUG(MMGDBG_LOW, ("Enter SftkPrepareSentinal()\n"));

	switch(eSentinal)
	{
	case MSGINCO:
	case MSGCO:
	case MSGCPON:
	case MSGCPOFF:
	case MSGAVOIDJOURNALS:
		softHeader->majicnum = DATASTAR_MAJIC_NUM;
		softHeader->offset = -1;
		softHeader->length = inputData->Size >> DEV_BSHIFT;	
							//The Length that is passed is in Bytes
		softHeader->dev = -1;
		softHeader->diskdev = -1;
		softHeader->group_ptr = NULL; //lgp;	//For now this Unknown
		softHeader->complete = 1;
		softHeader->flags = 0;
		softHeader->bp = 0;
		//These are to be decided
//		lgp->wlentries++;
//		lgp->wlsectors += softHeader->length;
		//These are to be decided
		break;
	default:
		Status = STATUS_UNSUCCESSFUL;
		break;
	}

	if(!OS_NT_SUCCESS(Status))
	{
		return Status;
	}

	switch(eSentinal)
	{
	case MSGINCO:
		RtlCopyMemory(inputData->pBuffer, MSG_INCO, sizeof(MSG_INCO));
		break;
	case MSGCO:
		RtlCopyMemory(inputData->pBuffer, MSG_CO, sizeof(MSG_CO));
		break;
	case MSGCPON:
		RtlCopyMemory(inputData->pBuffer, MSG_CPON, sizeof(MSG_CPON));
		break;
	case MSGCPOFF:
		RtlCopyMemory(inputData->pBuffer, MSG_CPOFF,sizeof(MSG_CPOFF));
		break;
	case MSGAVOIDJOURNALS:
		RtlCopyMemory(inputData->pBuffer, MSG_AVOID_JOURNALS, sizeof(MSG_AVOID_JOURNALS));
		break;
	default:
		Status = STATUS_UNSUCCESSFUL;
		break;
	}

	MMGDEBUG(MMGDBG_LOW, ("Leave SftkPrepareSentinal()\n"));
	return Status;

} /* SftkPrepareSentinal */ 

/*
 * SftkPrepareProtocolHeader - 
 *
 */
OS_NTSTATUS 
SftkPrepareProtocolHeader( IN enum protocol     eProtocol,
 					       IN OUT ftd_header_t  *protocolHeader,
						   IN rplcc_iorp_t      *inputData,
						   OUT PVOID            outputData,
						   LONG                 ackwanted )

/**/
{
	OS_NTSTATUS Status = STATUS_SUCCESS;

	MMGDEBUG(MMGDBG_LOW, ("Enter SftkPrepareProtocolHeader()\n"));

	switch(eProtocol)
	{
	case FTDCRFBLK:
		protocolHeader->magicvalue = MAGICHDR;
		protocolHeader->msgtype    = FTDCRFBLK;
		protocolHeader->msg.lg.devid  = inputData->DeviceId;
		protocolHeader->msg.lg.offset = (int)inputData->Blk_Start;	//In Blocks
		protocolHeader->msg.lg.len    = inputData->Size;	    //In Blocks

		protocolHeader->ackwanted = ackwanted;
		protocolHeader->compress = 0;	//for Now Compression is Disabled
		protocolHeader->uncomplen = inputData->Size<< DEV_BSHIFT;	//This is length in Bytes
		protocolHeader->len = protocolHeader->msg.lg.len = inputData->Size;

		break;
	case FTDCBFBLK:
		break;
	case FTDCCHUNK:
	case FTDSENTINAL:
		protocolHeader->magicvalue = MAGICHDR;
		protocolHeader->msgtype = FTDCCHUNK;
		protocolHeader->msg.lg.devid = 0;
		protocolHeader->ts = 0; //lgp->ts;	//This is UnKnown
		protocolHeader->ackwanted = ackwanted;
		protocolHeader->compress = 0;	//for Now Compression is Disabled
		protocolHeader->uncomplen = inputData->Size;	//This is length in Bytes
		protocolHeader->len = protocolHeader->msg.lg.len = inputData->Size;

		break;
	default:
		Status = STATUS_UNSUCCESSFUL;
		break;
	}

	MMGDEBUG(MMGDBG_LOW, ("Leave SftkPrepareProtocolHeader()\n"));

return Status;
} /* SftkPrepareProtocolHeader */

/*
 * SftkPrepareProtocolHeader - 
 *
 */
OS_NTSTATUS
SftkPrepareSoftHeader(IN PVOID           *p_data,
					  IN IN rplcc_iorp_t *p_iorequest)

/**/
{
	OS_NTSTATUS ret = STATUS_SUCCESS;
	wlheader_t *hp = NULL;

	MMGDEBUG(MMGDBG_LOW, ("Entering SftkPrepareSoftHeader\n"));

	hp = (wlheader_t *)p_data;
	hp->complete = 0;
	hp->bp = NULL;
	hp->majicnum = DATASTAR_MAJIC_NUM;

	/*
	* Original Code 
	* hp->offset = (u_int)(PtrCurrentStackLocation->Parameters.Write.ByteOffset.QuadPart >> DEV_BSHIFT);
	* hp->length = PtrCurrentStackLocation->Parameters.Write.Length >> DEV_BSHIFT;
	* hp->group_ptr = lgp
	*/
	hp->offset   = (int)p_iorequest->Blk_Start >> DEV_BSHIFT;
	hp->length   = p_iorequest->Size      >> DEV_BSHIFT;
	hp->diskdev  = p_iorequest->DeviceId;  //softp->bdev;
	hp->dev      = p_iorequest->DeviceId;  //softp->bdev;

	hp->flags = 0;

	MMGDEBUG(MMGDBG_LOW, ("Entering SftkPrepareSoftHeader\n"));

return(ret);
} /* SftkPrepareSoftHeader */

/*EOF*/
