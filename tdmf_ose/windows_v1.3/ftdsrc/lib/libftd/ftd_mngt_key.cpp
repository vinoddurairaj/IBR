/*
 * ftd_mngt_key.cpp - ftd management code related to license key.
 *
 * Copyright (c) 2002 Fujitsu SoftTek, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */
#include <time.h>
#include <limits.h>
extern "C" 
{
#include "ftd_mngt_key.h"
#include "ftd_cfgsys.h"
#include "ftd_mngt.h"
#include "license.h"
}
#include "libmngtdef.h"
#include "libmngtmsg.h"

#if defined(_WINDOWS) && defined(_DEBUG)
#include <crtdbg.h>
#include "errors.h"
#define ASSERT(exp)     _ASSERT(exp)
#else
#define ASSERT(exp)     ((void)0)
#endif
#define DBGPRINT(a)     ftd_mngt_tracef a 

void ftd_mngt_tracef( unsigned char pucLevel, char* pcpMsg, ... );

/*
 * Expected format of registration key is "G8A6 G8PF W2XD CBXD K4TD WSG3"
 * where letters must be capital, with N groups of NP_CRYPT_GROUP_SIZE characters each.
 */
#include "license.h"	//for NP_CRYPT_GROUP_SIZE
int
isRegistrationKeyValid(const char *szRegistKey)
{
	int 	    pos = 1;
	const char 	*p  = szRegistKey;
	while( *p != '\0' )
	{
		if ( pos % (NP_CRYPT_GROUP_SIZE+1) == 0 && pos != 1 )
		{
			if ( *p != ' ' )
				return 0;//invalid key
		}
		else if ( pos % (NP_CRYPT_GROUP_SIZE+1) != 0 || pos == 1 )
		{
			if ( !(*p >= 'A' && *p <= 'Z') && !(*p >= '0' && *p <= '9') )
					return 0;//invalid char in key
		}
		pos++;
		p++;
	}
	//there must be a multiple of (NP_CRYPT_GROUP_SIZE+1) characters in the license.
    return pos % (NP_CRYPT_GROUP_SIZE+1) == 0 ? 1 : 0;
}

int
ftd_mngt_key_get_licence_key_expiration_date( char* szRegKey )
{
    license_data_t lic;
    int     result;
    time_t  now,when;
	struct tm expires;
    int     cs;
	char	*lickey[2];
    int     iKeyExpirationTime ;

	lickey[0] = szRegKey;
	lickey[1] = NULL;
	result = check_key_r(&lickey[0], np_crypt_key_pmd, NP_CRYPT_KEY_LEN, &lic);

    switch (result) {
    //////////////////////////////////////////////////
    //// INVALID license/registration key cases : ////
    //////////////////////////////////////////////////
    case LICKEY_NULL:
    case LICKEY_EMPTY:
    case LICKEY_GENERICERR:
        //intentionally without break statement
    case LICKEY_BADCHECKSUM:
        break;
    ////////////////////////////////////////////////
    //// VALID license/registration key cases : ////
    ////////////////////////////////////////////////
    case LICKEY_OK:
    case LICKEY_EXPIRED:
    case LICKEY_WRONGHOST:
    case LICKEY_BADSITELIC:
    case LICKEY_WRONGMACHINETYPE:
    case LICKEY_BADFEATUREMASK:
        break;
    default:
        break;
    }

    iKeyExpirationTime = result;

    if ( result == LICKEY_OK || result == LICKEY_EXPIRED )
    {
        if (lic.license_expires) 
        {
            when = lic.expiration_date << 16;
			/* Force expiration hour to 13:00:00 */
			expires = *localtime( &when );  
			expires.tm_sec = 00;
			expires.tm_min = 00;
			expires.tm_hour = 13;
			when = mktime(&expires);

            iKeyExpirationTime = when;
            now = time((time_t *) NULL);
            if (when > now) {
#ifdef _DEBUG
                printf("\nValid Non Permanent License -- expires: %s\n", ctime(&when));
#endif
            } else {
#ifdef _DEBUG
                printf("\nExpired Non Permanent License -- expired: %s\n", ctime(&when));
#endif
                iKeyExpirationTime = LICKEY_EXPIRED;
            }            
        } else if (lic.hostid == 0) {   /* Site license */
            cs = lic.customer_name_checksum;
            if (cs != 0 && get_customer_name_checksum() != cs) {
                iKeyExpirationTime = LICKEY_BADSITELIC;
            } else {
                iKeyExpirationTime = INT_MAX;
            }
        } else {    /* Otherwise we're a host license */
            if (lic.hostid != my_gethostid()) {
                iKeyExpirationTime = LICKEY_WRONGHOST;
            } else {
                iKeyExpirationTime = INT_MAX;
            }
        }
    }

    return iKeyExpirationTime;
}


/*
 * Received a mmp_mngt_header_t indicating a MMP_MNGT_REGISTRATION_KEY command
 * read and process cmd
 *
 */
int
ftd_mngt_registration_key_req(sock_t *sockp)
{
    mmp_mngt_RegistrationKeyMsg_t      cmdmsg;
    int r;
    char                            *pWk;

    pWk  = (char*)&cmdmsg;
    pWk += sizeof(mmp_mngt_header_t);
    //////////////////////////////////
    //at this point, mmp_mngt_header_t header is read.
    //now read the remainder of the mmp_mngt_TdmfCommandMsg_t structure 
    r = mmp_mngt_sock_recv(sockp,(char*)pWk,sizeof(mmp_mngt_RegistrationKeyMsg_t)-sizeof(mmp_mngt_header_t));ASSERT(r == sizeof(mmp_mngt_RegistrationKeyMsg_t)-sizeof(mmp_mngt_header_t));
    if ( r != sizeof(mmp_mngt_RegistrationKeyMsg_t)-sizeof(mmp_mngt_header_t) )
    {
        DBGPRINT((1,"****Error, bad format Registration key message received from 0x%x \n",sockp->rip));
        return -1;
    }
    //no multi-byte value to convert from network bytes order to host byte order
    DBGPRINT((3,"%s Registration Key message received %s %s\n"
        , cmdmsg.keydata.szRegKey[0] == 0 ? "GET" :"SET"
        , cmdmsg.keydata.szRegKey[0] != 0 ? ", key =" : ""
        , cmdmsg.keydata.szRegKey    ));

    //if key is empty, iut is a request to get the existing.
    //otherwise , it is a request to set the existing key
    if ( cmdmsg.keydata.szRegKey[0] == 0 )
    {   //get key
        if ((cfg_get_software_key_value("license", cmdmsg.keydata.szRegKey, CFG_IS_STRINGVAL)) == CFG_OK)
        {
            cmdmsg.keydata.iKeyExpirationTime = ftd_mngt_key_get_licence_key_expiration_date( cmdmsg.keydata.szRegKey );
        }
        else
        {   //error !
            DBGPRINT((1,"****Error, unable to retreive current Agent registration key!\n"));
            cmdmsg.keydata.szRegKey[0] = 0;//make sure key is empty
        }

    }
    else
    {   //set key
		if ( isRegistrationKeyValid(cmdmsg.keydata.szRegKey) )
		{
	        if ((cfg_set_software_key_value("license", cmdmsg.keydata.szRegKey, CFG_IS_STRINGVAL)) == CFG_OK)
            {
                cmdmsg.keydata.iKeyExpirationTime = ftd_mngt_key_get_licence_key_expiration_date( cmdmsg.keydata.szRegKey );
            }
            else
	        {   //error !
	            DBGPRINT((1,"****Error, unable to save the received registration key <%s> !\n",cmdmsg.keydata.szRegKey));
	            cmdmsg.keydata.szRegKey[0] = 0;//make sure key is empty
	        }
		}
		else
		{
			DBGPRINT((2,"****Error,  the received registration key received <%s> is INVALID and is discarded !\n",cmdmsg.keydata.szRegKey));
	        cmdmsg.keydata.szRegKey[0] = 0;//make sure key is empty
		}
    }

    /////////////////////////////////////////////////////
    //send registration key to requester
    cmdmsg.hdr.magicnumber    = MNGT_MSG_MAGICNUMBER;
    cmdmsg.hdr.mngttype       = MMP_MNGT_REGISTRATION_KEY;
    cmdmsg.hdr.sendertype     = SENDERTYPE_TDMF_AGENT;
    cmdmsg.hdr.mngtstatus     = MMP_MNGT_STATUS_OK; 
    //convert to network byte order before sending on socket
    mmp_convert_mngt_hdr_hton(&cmdmsg.hdr);
    cmdmsg.keydata.iKeyExpirationTime = htonl(cmdmsg.keydata.iKeyExpirationTime);
    r = mmp_mngt_sock_send(sockp,(char*)&cmdmsg,sizeof(cmdmsg));
    if ( r != sizeof(cmdmsg) )
    {
        DBGPRINT((1,"****Error (%d) while sending registration key to requester!\n",r));
    }
    else
    {
        DBGPRINT((3,"Registration key sent as response = <%s>\n",cmdmsg.keydata.szRegKey));
    }

    return 0;
}

// PUSH license key to Collector
void ftd_mngt_send_registration_key()
{
    mmp_mngt_RegistrationKeyMsg_t   msg;
    int r,towrite;
    int collectorIP, collectorPort;

    if ((cfg_get_software_key_value("license", msg.keydata.szRegKey, CFG_IS_STRINGVAL)) != CFG_OK)
    {   //error !
        DBGPRINT((1,"****Error, unable to retreive current Agent registration key!\n"));
        return ;
    }
    ftd_mngt_getServerId( msg.keydata.szServerUID );
    msg.keydata.iKeyExpirationTime = ftd_mngt_key_get_licence_key_expiration_date( msg.keydata.szRegKey );

    /////////////////////////////////////////////////////
    //send registration key to requester
    msg.hdr.magicnumber    = MNGT_MSG_MAGICNUMBER;
    msg.hdr.mngttype       = MMP_MNGT_REGISTRATION_KEY;
    msg.hdr.sendertype     = SENDERTYPE_TDMF_AGENT;
    msg.hdr.mngtstatus     = MMP_MNGT_STATUS_OK; 
    //convert to network byte order before sending on socket
    mmp_convert_mngt_hdr_hton(&msg.hdr);
    msg.keydata.iKeyExpirationTime = htonl(msg.keydata.iKeyExpirationTime);
    towrite = sizeof(msg);
    ftd_mngt_GetCollectorInfo(&collectorIP, &collectorPort);
	
	// By Saumya Tripathi 03/05/04
	// With these modifications it will run in a collector less environment also
	// Big GUI won't work without the collector; But the Mini GUI and the Config tool will
	// still work; all the CLIs will work too
	if( collectorIP != 0 )
	{
    r = mmp_sendmsg(collectorIP,collectorPort,(char*)&msg,towrite,0,0);
    if ( r != 0 )
    {
        DBGPRINT((1,"****Error (%d) while sending registration key to Collector!\n",r));
    }
}
}
