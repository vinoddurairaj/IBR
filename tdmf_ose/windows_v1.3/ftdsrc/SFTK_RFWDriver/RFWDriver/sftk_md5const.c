/*
 * md5const.c - MD5 construction/destruction	
 * 
 * Copyright (c) 2000 Legato Systems, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */
#include    "NTDDK.h"
#include	"NDIS.H"
#include	"sftk_md5.h"
#include	"sftk_md5const.h"


MD5_CTX *MD5Create (void)
{
	MD5_CTX *context = NULL;
	NDIS_STATUS nNdisStatus = NDIS_STATUS_SUCCESS;

	nNdisStatus = NdisAllocateMemoryWithTag(
								&context,
								sizeof(MD5_CTX),
								'A5DM'
								);

	if(nNdisStatus != NDIS_STATUS_SUCCESS)
	{
		KdPrint(("Unable to Allocate the MD5_CTX memory\n"));
		return NULL;
	}
	KdPrint(("Successfully Created the MD5_CTX\n"));
	return context;
}

int MD5Delete (MD5_CTX *context)
{

	if(context != NULL)
	{
		NdisFreeMemory(context,sizeof(MD5_CTX),0);
	}
	KdPrint(("Successfully Deleted the MD5_CTX\n"));
	return 0;
}

