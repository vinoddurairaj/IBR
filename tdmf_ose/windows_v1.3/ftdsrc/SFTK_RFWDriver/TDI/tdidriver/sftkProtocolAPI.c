
/*
 * sftkProtocolAPI.c - Protocol APIs
 * 
 * Copyright (c) 2004 Softek Storage Solutions, Inc.  All Rights Reserved.
 * Confidential property of Softek Storage Solutions, Inc. May not be used or 
 * distributed without proper authorization.
 *
 *
 * This file implements Communication protocol APIs between two servers
 *
 * AUTHOR:	Saumya Tripathi
 * Date:	May 24, 2004
 */


#include "sftkProtocolAPI.h"

// sends FTDCHUP command over to the other server
NTSTATUS Send_HUP( IN PSESSION_MANAGER pSessionManger )
{
	ftd_header_t	header;
	SFTK_IO_VECTOR	iovec[1];
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&header, 0, sizeof(header));

	header.magicvalue = MAGICHDR;
	header.msgtype = FTDCHUP;
	header.ackwanted = 1;

	iovec[0].pBuffer = &header;
	iovec[0].uLength = sizeof(ftd_header_t);

	if( !NT_SUCCESS( status = SftkSendVector( iovec, 1, pSessionManger ) ))
	{
		KdPrint(("Send_HUP:: SftkSendVector failed"));
		return status;
	}

	return status;
}

// sends FTDACKHUP command over to the other server
NTSTATUS Send_ACKHUP( IN int iDeviceID, IN PSESSION_MANAGER pSessionManger)
{
	ftd_header_t	ack;
	SFTK_IO_VECTOR	iovec[1];
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&ack, 0, sizeof(ack));

	ack.msgtype = FTDACKHUP;
	ack.msg.lg.devid = iDeviceID;
	ack.msg.lg.data = FTDACKHUP;

	iovec[0].pBuffer = &ack;
	iovec[0].uLength = sizeof(ftd_header_t);

	if( !NT_SUCCESS( status = SftkSendVector( iovec, 1, pSessionManger ) ))
	{
		KdPrint(("Send_ACKHUP:: SftkSendVector failed"));
		return status;
	}

	return status;
}

// sends FTDCRFFSTART command over to the secondary server
NTSTATUS Send_RFFSTART( IN PSESSION_MANAGER pSessionManger )
{
	ftd_header_t	header;
	SFTK_IO_VECTOR	iovec[1];
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&header, 0, sizeof(header));
	header.msgtype = FTDCRFFSTART;

	iovec[0].pBuffer = &header;
	iovec[0].uLength = sizeof(ftd_header_t);

	if( !NT_SUCCESS( status = SftkSendVector( iovec, 1, pSessionManger ) ))
	{
		KdPrint(("Send_RFFSTART:: SftkSendVector failed"));
		return status;
	}

	return status;
}

// sends FTDACKRFSTART command over to the PRIMARY server
NTSTATUS Send_ACKRFSTART( IN PSESSION_MANAGER pSessionManger )
{
	ftd_header_t	ack;
	SFTK_IO_VECTOR	iovec[1];
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&ack, 0, sizeof(ack));
	ack.msgtype = FTDACKRFSTART;

	iovec[0].pBuffer = &ack;
	iovec[0].uLength = sizeof(ftd_header_t);

	if( !NT_SUCCESS( status = SftkSendVector( iovec, 1, pSessionManger ) ))
	{
		KdPrint(("Send_ACKRFSTART:: SftkSendVector failed"));
		return status;
	}

	return status;
}


// sends FTDCRFFEND command over to the secondary server
NTSTATUS Send_RFFEND(IN int iLgnum, IN PSESSION_MANAGER pSessionManger)
{
	ftd_header_t	header;
	SFTK_IO_VECTOR	iovec[1];
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&header, 0, sizeof(header));

	header.msgtype = FTDCRFFSTART;
	header.msg.lg.devid = iLgnum;
    header.ackwanted = 0;

	iovec[0].pBuffer = &header;
	iovec[0].uLength = sizeof(ftd_header_t);

	if( !NT_SUCCESS( status = SftkSendVector( iovec, 1, pSessionManger ) ))
	{
		KdPrint(("Send_RFFEND:: SftkSendVector failed"));
		return status;
	}

	return status;
}
 
// sends FTDCCHKSUM command over to the secondary server
NTSTATUS Send_CHKSUM( IN  int iLgnumIN,
					  IN  int iDeviceId, 
					  IN  ftd_dev_t* ptrDeviceStructure,
					  OUT PVOID* ptrDeltaMap,
					  IN PSESSION_MANAGER pSessionManger)
{
	ftd_header_t	header;
	SFTK_IO_VECTOR	iovec[3];
	int             iovcnt, rc, digestlen;
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&header, 0, sizeof(header));

	header.magicvalue = MAGICHDR;
    header.msgtype = FTDCCHKSUM;
    header.ackwanted = 1;
    header.len = 1;
	header.msg.lg.lgnum = iLgnumIN;

	if (ptrDeviceStructure->sumlen == 0) {
            ptrDeviceStructure->sumoff = 0;
			ptrDeviceStructure->sumlen = 0;
			return status;
       }

    digestlen = ptrDeviceStructure->sumnum * DIGESTSIZE;
    header.msg.lg.devid = ptrDeviceStructure->devid;

	iovec[0].pBuffer = &header;
	iovec[0].uLength = sizeof(ftd_header_t);
	iovec[1].pBuffer = ptrDeviceStructure;
    iovec[1].uLength = sizeof(ftd_dev_t);
	iovec[2].pBuffer = ptrDeviceStructure->sumbuf;
    iovec[2].uLength = digestlen;

	ptrDeviceStructure->statp->actual += sizeof(ftd_dev_t) + digestlen;
    ptrDeviceStructure->statp->effective += sizeof(ftd_dev_t) + digestlen;

	if( !NT_SUCCESS( status = SftkSendVector( iovec, 3, pSessionManger ) ))
	{
		KdPrint(("Send_CHKSUM:: SftkSendVector failed"));
		return status;
	}

	if( !NT_SUCCESS( status = SftkRecvVector_CheckSum( ptrDeltaMap, pSessionManger ) ))
	{
		KdPrint(("Send_CHKSUM:: SftkRecvVector_CheckSum failed"));
		return status;
	}

	return status;
}

// sends FTDACKCHKSUM command over to the primary server
NTSTATUS Send_ACKCHKSUM(IN  ftd_dev_t* ptrDeviceStructure,
						IN	int iDeltamap_len,
						IN	PVOID ptrDeltaMap,
						IN	PSESSION_MANAGER pSessionManger)
{
	ftd_header_t	ack;
	SFTK_IO_VECTOR	iovec[3];
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&ack, 0, sizeof(ack));

    ack.msgtype = FTDACKCHKSUM;
    ack.msg.lg.devid = ptrDeviceStructure->devid;
    ack.msg.lg.len = (ptrDeviceStructure->sumlen >> DEV_BSHIFT);
    ack.len = iDeltamap_len;

	iovec[0].pBuffer = &ack;
	iovec[0].uLength = sizeof(ftd_header_t);
	iovec[1].pBuffer = ptrDeviceStructure;
    iovec[1].uLength = sizeof(ftd_dev_t);
	iovec[2].pBuffer = ptrDeltaMap;
    iovec[2].uLength = iDeltamap_len;

	if( !NT_SUCCESS( status = SftkSendVector( iovec, 1, pSessionManger ) ))
	{
		KdPrint(("Send_ACKCHKSUM:: SftkSendVector failed"));
		return status;
	}

	return status;
}

// sends FTDACKCPSTART command over to the other server
NTSTATUS Send_ACKCPSTART(IN PSESSION_MANAGER pSessionManger)
{
	ftd_header_t	ack;
	SFTK_IO_VECTOR	iovec[1];
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&ack, 0, sizeof(ack));

	ack.msgtype = FTDACKCPSTART;

	iovec[0].pBuffer = &ack;
	iovec[0].uLength = sizeof(ftd_header_t);

	if( !NT_SUCCESS( status = SftkSendVector( iovec, 1, pSessionManger ) ))
	{
		KdPrint(("Send_ACKCPSTART:: SftkSendVector failed"));
		return status;
	}

	return status;
}

// sends FTDACKCPSTOP command over to the other server
NTSTATUS Send_ACKCPSTOP(IN PSESSION_MANAGER pSessionManger)
{
	ftd_header_t	ack;
	SFTK_IO_VECTOR	iovec[1];
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&ack, 0, sizeof(ack));

	ack.msgtype = FTDACKCPSTOP;

	iovec[0].pBuffer = &ack;
	iovec[0].uLength = sizeof(ftd_header_t);

	if( !NT_SUCCESS( status = SftkSendVector( iovec, 1, pSessionManger ) ))
	{
		KdPrint(("Send_ACKCPSTOP:: SftkSendVector failed"));
		return status;
	}

	return status;
}

// sends FTDCCPONERR command over to the other server
NTSTATUS Send_CPONERR(IN PSESSION_MANAGER pSessionManger)
{
	ftd_header_t	header;
	SFTK_IO_VECTOR	iovec[1];
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&header, 0, sizeof(header));
	header.msgtype = FTDCCPONERR;

	iovec[0].pBuffer = &header;
	iovec[0].uLength = sizeof(ftd_header_t);

	if( !NT_SUCCESS( status = SftkSendVector( iovec, 1, pSessionManger ) ))
	{
		KdPrint(("Send_CPONERR:: SftkSendVector failed"));
		return status;
	}

	return status;
}

// sends FTDCCPOFFERR command over to the other server
NTSTATUS Send_CPOFFERR(IN PSESSION_MANAGER pSessionManger)
{
	ftd_header_t	header;
	SFTK_IO_VECTOR	iovec[1];
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&header, 0, sizeof(header));
	header.msgtype = FTDCCPOFFERR;

	iovec[0].pBuffer = &header;
	iovec[0].uLength = sizeof(ftd_header_t);

	if( !NT_SUCCESS( status = SftkSendVector( iovec, 1, pSessionManger ) ))
	{
		KdPrint(("Send_CPOFFERR:: SftkSendVector failed"));
		return status;
	}

	return status;
}

// Is sent as part of Handshake mechanism when starting the PMD. 
// If RMD is not alive it is spawned. sends FTDCNOOP command over to the other server
NTSTATUS Send_NOOP(IN PSESSION_MANAGER pSessionManger)
{
	ftd_header_t	header;
	SFTK_IO_VECTOR	iovec[1];
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&header, 0, sizeof(header));
	header.msgtype = FTDCNOOP;

	iovec[0].pBuffer = &header;
	iovec[0].uLength = sizeof(ftd_header_t);

	if( !NT_SUCCESS( status = SftkSendVector( iovec, 1, pSessionManger ) ))
	{
		KdPrint(("Send_NOOP:: SftkSendVector failed"));
		return status;
	}

	return status;
}


// Is sent as part of Handshake mechanism when starting the PMD. 
// The API returns the Remote Version String. sends FTDCVERSION command over to the other server
NTSTATUS Send_VERSION(IN int iLgnumIN,
					  OUT PCHAR* strSecondaryVersion,
					  IN PSESSION_MANAGER pSessionManger)
{
	ftd_header_t	header;
	ftd_version_t	version;
	SFTK_IO_VECTOR	iovec[2];
	PCHAR           versionstr = NULL, remversionstr = NULL;
	LARGE_INTEGER   CurrentTime;
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&header, 0, sizeof(header));
	header.magicvalue = MAGICHDR;
    header.msgtype = FTDCVERSION;
    header.ackwanted = 1;
    header.msg.lg.lgnum = iLgnumIN;

	KeQuerySystemTime( &CurrentTime );
	header.ts = CurrentTime.LowPart;
	version.pmdts = CurrentTime.LowPart;

	sprintf(version.configpath, "%s%03d.%s\n",
        SECONDARY_CFG_PREFIX, iLgnumIN, PATH_CFG_SUFFIX);

	/* set default protocol version numbers */
    strcpy(versionstr, VERSIONSTR);// hard code it here
    strcpy(version.version, versionstr);

	iovec[0].pBuffer = &header;
	iovec[0].uLength = sizeof(ftd_header_t);
	iovec[1].pBuffer = &version;
    iovec[1].uLength = sizeof(ftd_version_t);

	if( !NT_SUCCESS( status = SftkSendVector( iovec, 2, pSessionManger ) ))
	{
		KdPrint(("Send_VERSION:: SftkSendVector failed"));
		return status;
	}

	if( !NT_SUCCESS( status = SftkRecvVector_Version( strSecondaryVersion, pSessionManger ) ))
	{
		KdPrint(("Send_VERSION:: SftkRecvVector_Version failed"));
		return status;
	}

	return status;
}

// Is sent as part of Handshake mechanism when starting the PMD. 
// The API returns STATUS_SUCCESS indicating whether the Secondary is in CP mode or not.
NTSTATUS Send_HANDSHAKE(IN unsigned int nFlags,
					  IN ULONG ulHostID,
					  IN CONST PCHAR strConfigFilePath,
					  IN CONST PCHAR strLocalHostName,
					  IN ULONG ulLocalIP,
					  OUT int* CP,
					  IN PSESSION_MANAGER pSessionManger)
{
	ftd_header_t	header;
	ftd_auth_t		auth;
	SFTK_IO_VECTOR	iovec[2];
	LARGE_INTEGER   CurrentTime;
	ULONG			ts;
	NTSTATUS		status = STATUS_SUCCESS;

	KeQuerySystemTime( &CurrentTime );
	ts = CurrentTime.LowPart;

	memset(&auth, 0, sizeof(auth));

	ftd_sock_encode_auth(ts, strLocalHostName,
        ulHostID, ulLocalIP, &auth.len, auth.auth);

	strcpy(auth.configpath, strConfigFilePath);

	memset(&header, 0, sizeof(header));

	header.magicvalue = MAGICHDR;
    header.msgtype = FTDCHANDSHAKE;
	header.ts = ts;
    header.ackwanted = 1;
    header.msg.lg.flags = nFlags;
    header.msg.lg.data = ulHostID;

	iovec[0].pBuffer = &header;
	iovec[0].uLength = sizeof(ftd_header_t);
	iovec[1].pBuffer = &auth;
    iovec[1].uLength = sizeof(ftd_auth_t);

	if( !NT_SUCCESS( status = SftkSendVector( iovec, 2, pSessionManger ) ))
	{
		KdPrint(("Send_HANDSHAKE:: SftkSendVector failed"));
		return status;
	}

	if( !NT_SUCCESS( status = SftkRecvVector_Handshake( CP, pSessionManger ) ))
	{
		KdPrint(("Send_HANDSHAKE:: SftkRecvVector_Handshake failed"));
		return status;
	}

	return status;
}

// This ack is prepared and sent from the RMD in response to FTDCHANDSHAKE command
NTSTATUS Send_ACKHANDSHAKE(IN unsigned int nFlags, 
						   IN int iDeviceID, 
						   IN PSESSION_MANAGER pSessionManger)
{
	ftd_header_t	ack;
	SFTK_IO_VECTOR	iovec[1];
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&ack, 0, sizeof(ack));
	ack.msgtype = FTDACKHANDSHAKE;

    if (GET_LG_CPON(nFlags)) {
        // tell primary that we are in checkpoint mode 
        SET_LG_CPON(ack.msg.lg.flags);
    }

    ack.msg.lg.devid = iDeviceID;

	iovec[0].pBuffer = &ack;
	iovec[0].uLength = sizeof(ftd_header_t);

	if( !NT_SUCCESS( status = SftkSendVector( iovec, 1, pSessionManger ) ))
	{
		KdPrint(("Send_ACKHANDSHAKE:: SftkSendVector failed"));
		return status;
	}

	return status;
}

// Is sent as part of Handshake mechanism when starting the PMD. 
// For each device in the group, one command is sent. If the remote side can not find the device 
// specified, an error is returned.
NTSTATUS Send_CHKCONFIG(IN int iDeviceID,
					  IN dev_t ulDevNum,
					  IN dev_t ulFtdNum,
					  IN CONST PCHAR strRemoteDeviceName,
					  OUT PULONG ulRemoteDevSize,
					  OUT PULONG ulDevId,
					  IN PSESSION_MANAGER pSessionManger)
{
	ftd_header_t	header;
	ftd_rdev_t		rdev;
	SFTK_IO_VECTOR	iovec[2];
	LARGE_INTEGER  CurrentTime;
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&header, 0, sizeof(header));

	// send mirror device name to the remote system for verification 
    header.magicvalue = MAGICHDR;
    header.msgtype = FTDCCHKCONFIG;
	KeQuerySystemTime( &CurrentTime );
	header.ts = CurrentTime.LowPart;
    header.ackwanted = 1;
    header.msg.lg.devid = iDeviceID;

    rdev.devid = iDeviceID;
    rdev.minor = ulDevNum;
    rdev.ftd = ulFtdNum;

    strcpy(rdev.path, strRemoteDeviceName);
    rdev.len = strlen(strRemoteDeviceName);

	iovec[0].pBuffer = &header;
	iovec[0].uLength = sizeof(ftd_header_t);
	iovec[1].pBuffer = &rdev;
    iovec[1].uLength = sizeof(ftd_rdev_t);

	if( !NT_SUCCESS( status = SftkSendVector( iovec, 2, pSessionManger ) ))
	{
		KdPrint(("Send_CHKCONFIG:: SftkSendVector failed"));
		return status;
	}

	if( !NT_SUCCESS( status = SftkRecvVector_CheckConfig( ulRemoteDevSize, ulDevId, pSessionManger ) ))
	{
		KdPrint(("Send_CHKCONFIG:: SftkRecvVector_CheckConfig failed"));
		return status;
	}

	return status;
}

// This ack is prepared and sent from the RMD in response to FTDCCHKCONFIG command
NTSTATUS Send_ACKCONFIG(IN int iSize, 
						IN int iDeviceID, 
						IN PSESSION_MANAGER pSessionManger)
{
	ftd_header_t	ack;
	SFTK_IO_VECTOR	iovec[1];
	NTSTATUS		status = STATUS_SUCCESS;

	memset(&ack, 0, sizeof(ack));

	/* return mirror volume size for a configuration query */
    ack.msgtype = FTDACKCONFIG;
    ack.msg.lg.data = iSize;
    ack.msg.lg.devid = iDeviceID;

	iovec[0].pBuffer = &ack;
	iovec[0].uLength = sizeof(ftd_header_t);

	if( !NT_SUCCESS( status = SftkSendVector( iovec, 1, pSessionManger ) ))
	{
		KdPrint(("Send_ACKCONFIG:: SftkSendVector failed"));
		return status;
	}

	return status;
}

static void
ftd_sock_encode_auth(ULONG ts, PCHAR hostname, ULONG hostid, ULONG ip,
            int *encodelen, PCHAR encode)
{
    encodeunion *kp, key1, key2;
    int         i, j;
    u_char      k, t;

    key1.ul = ((u_long) ts) ^ ((u_long) hostid);
    key2.ul = ((u_long) ts) ^ ((u_long) ip);
  
    i = j = 0;
    while (i < (int)strlen(hostname)) {
        kp = ((i%8) > 3) ? &key1 : &key2;
        k = kp->uc[i%4];
        t = (u_char) (0x000000ff & ((u_long)k ^ (u_long) hostname[i++]));
        sprintf (&(encode[j]), "%02x", t);
        j += 2;
    }
    encode[j] = '\0';
    *encodelen = j;

    return;
}

NTSTATUS
SftkSendVector( 
IN SFTK_IO_VECTOR iovector[], 
IN ULONG length,
IN PSESSION_MANAGER pSessionManger)
{
	return STATUS_SUCCESS;
}

NTSTATUS
SftkRecvVector_Version( 
OUT PCHAR* strSecondaryVersion,
IN PSESSION_MANAGER pSessionManger)
{
	return STATUS_SUCCESS;
}

NTSTATUS
SftkRecvVector_Handshake( 
OUT int* CP,
IN PSESSION_MANAGER pSessionManger)
{
	return STATUS_SUCCESS;
}

NTSTATUS
SftkRecvVector_CheckConfig( 
OUT PULONG ulRemoteDevSize,
OUT PULONG ulDevId,
IN PSESSION_MANAGER pSessionManger)
{
	return STATUS_SUCCESS;
}

NTSTATUS
SftkRecvVector_CheckSum( 
OUT PVOID* ptrDeltaMap,
IN PSESSION_MANAGER pSessionManger)
{
	return STATUS_SUCCESS;
}
