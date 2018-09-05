/********************************************************* {COPYRIGHT-TOP} ***
* IBM Confidential
* OCO Source Materials
* 6949-32F - Softek Replicator for Unix and 6949-32K - Softek TDMF (IP) for Unix
*
*
* (C) Copyright IBM Corp. 2006, 2011  All Rights Reserved.
* The source code for this program is not published or otherwise  
* divested of its trade secrets, irrespective of what has been 
* deposited with the U.S. Copyright Office.
********************************************************* {COPYRIGHT-END} **/
/* #ident "@(#)$Id: ftd_mngt_registration_key_req.c,v 1.6 2017/02/01 00:11:39 paulclou Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */

#ifndef _FTD_MNGT_REGISTRATION_KEY_REQ_C_
#define _FTD_MNGT_REGISTRATION_KEY_REQ_C_

extern  char    gszServerUID[];

/*
 * Expected format of registration key is "G8A6 G8PF W2XD CBXD K4TD WSG3"  
 * where letters must be capital, with N groups of NP_CRYPT_GROUP_SIZE 
 * characters each.
 */

static int
isRegistrationKeyValid(const char *szRegistKey)
{
        int         pos = 1;
        const char      *p  = szRegistKey;
		char TDMFIP280_free_for_all_license[LENGTH_TDMFIP_280_FREEFORALL_LICKEY+1];
		int found_free_for_all_license;
		int j, n;

		/* In TDMFIP 2.9.0, reject the free-for-all permanent license that was installed in release 2.8.0 */
		strcpy( TDMFIP280_free_for_all_license, TDMFIP_280_FREEFORALL_LICKEY );
		TDMFIP280_free_for_all_license[LENGTH_TDMFIP_280_FREEFORALL_LICKEY] = NULL;
		found_free_for_all_license = 1;
		for ( j = 0, n = LENGTH_TDMFIP_280_FREEFORALL_LICKEY-1; n >= 0, j < strlen(szRegistKey); j++, n-- )
		{
			if(	szRegistKey[j] == ' ' )  // skip spaces in source string
			{
				n++;
				continue;
			}
			if( szRegistKey[j] != (char)(TDMFIP280_free_for_all_license[n] + 1) )
			{
				found_free_for_all_license = 0;
				break;
			}
		}

		if( found_free_for_all_license )
	    {
        	logout(9,F_ftd_mngt_registration_key_req,"Unrestricted any-host any-site permanent license is not allowed.\n");
		    return 0;  // Return false
	    }

        while( *p != '\0' )
        {
                if ( pos % (NP_CRYPT_GROUP_SIZE+1) == 0 && pos != 1 )
                {
                	if ( *p != ' ' )
                		return 0; /* invalid key */
                }
                else if ( pos % (NP_CRYPT_GROUP_SIZE+1) != 0 || pos == 1 )
                {
                        if ( !(*p >= 'A' && *p <= 'Z') && !(*p >= '0' && *p <= '9') )
                        	return 0;/* invalid char in key */
                }
                pos++;
                p++;
        }
        /* there must be a multiple of (NP_CRYPT_GROUP_SIZE+1) characters 
	in the license. */
	return pos % (NP_CRYPT_GROUP_SIZE+1) == 0 ? 1 : 0;
}

static int
ftd_mngt_key_get_licence_key_expiration_date( char* szRegKey )
{
    license_data_t lic;
    int     result;
    time_t  now,when;
    int     cs;
    char    *lickey[2];
    int     iKeyExpirationTime ;

    lickey[0] = szRegKey;
    lickey[1] = NULL;
    result = check_key_r(&lickey[0], np_crypt_key_pmd, NP_CRYPT_KEY_LEN, &lic);

    switch (result) {
    /*
     * INVALID license/registration key cases :
     */
    case LICKEY_NULL:
    case LICKEY_EMPTY:
    case LICKEY_GENERICERR:
    case LICKEY_BADCHECKSUM:
        break;
    /*
     * VALID license/registration key cases :
     */
    case LICKEY_OK:
    case LICKEY_EXPIRED:
    case LICKEY_WRONGHOST:
    case LICKEY_BADSITELIC:
    case LICKEY_WRONGMACHINETYPE:
    case LICKEY_BADFEATUREMASK:
    case LICKEY_NOT_ALLOWED:
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
            iKeyExpirationTime = when;
            now = time((time_t *) NULL);
            if (when > now) {
#ifdef _DEBUG
                printf("\nValid " CAPQ " Demo License -- expires: %s\n", ctime(&when));
#endif
            } else {
#ifdef _DEBUG
                printf("\nExpired " CAPQ "Demo License -- expired: %s\n", ctime(&when));
#endif
                iKeyExpirationTime = LICKEY_EXPIRED;
            }
        } else if (lic.hostid == 0) {   /* Site license */
            cs = lic.customer_name_checksum;
            if (cs != 0 && get_customer_name_checksum() != cs) {
                /* printf("\t" CAPQ " Site license with bad checksum\n"); */
                iKeyExpirationTime = LICKEY_BADSITELIC;
            } else {
                /* printf("\tValid permanent " CAPQ " site license.\n"); */
                iKeyExpirationTime = INT_MAX;
            }
        } else {    /* Otherwise we're a host license */
            if (lic.hostid != my_gethostid(HOSTID_LICENSE)) {
                /* printf("\tPermanent " CAPQ " license is _NOT_ for this system\n"); */
                iKeyExpirationTime = LICKEY_WRONGHOST;
            } else {
                /* printf("\tPermanent " CAPQ " license is valid for this system\n"); */
                iKeyExpirationTime = INT_MAX;
            }
        }
    }
    return iKeyExpirationTime;
}






int ftd_mngt_registration_key_req(sock_t *sockID)
{

    mmp_mngt_RegistrationKeyMsg_t      cmdmsg;
    int r,i,j,k;
    char                            *pWk;
    char licbuf[256 +1];
    FILE *Licfile;

    pWk  = (char*)&cmdmsg;
    pWk += sizeof(mmp_mngt_header_t);
    /****************************************/
    /* at this point, mmp_mngt_header_t header is read.
     * now read the remainder of the mmp_mngt_TdmfCommandMsg_t structure
     */
    r = ftd_sock_recv(sockID,(char*)pWk,
	sizeof(mmp_mngt_RegistrationKeyMsg_t)-sizeof(mmp_mngt_header_t));
    if ( r != sizeof(mmp_mngt_RegistrationKeyMsg_t)-sizeof(mmp_mngt_header_t) )
    {
        logout(4,F_ftd_mngt_registration_key_req,"bad format Registration key message received.\n");
        return -1;
    }

    /* no multi-byte value to convert from 
	network bytes order to host byte order */

    /* if key is empty, iut is a request to get the existing. */
    /* otherwise , it is a request to set the existing key */
    if ( cmdmsg.keydata.szRegKey[0] == 0 )
    {   /* get key */
	if ((Licfile = fopen(LIC_FILE,"r")) == NULL)
	{
		/* license file open error */
        	logout(4,F_ftd_mngt_registration_key_req,"unable to retreive current "PRODUCTNAME" Agent registration key.\n");
		cmdmsg.keydata.szRegKey[0] = 0; /* make sure key is empty */
	}
 	else 
	{
		
		while(!feof(Licfile) && NULL != fgets(licbuf, 256, Licfile)) {	
		    	if (licbuf[strlen(licbuf)-1] == 0x0a)
				licbuf[strlen(licbuf)-1] = 0x00;
			i = 0;
			memset(cmdmsg.keydata.szRegKey,0x00,sizeof(cmdmsg.keydata.szRegKey));
			if (0 < strlen(licbuf)) {
				cmdmsg.keydata.szRegKey[0] = 0; /* make sure key is empty */
				licbuf[strlen(licbuf)-1] = 0x00;
				if ('#' != licbuf[i] &&
				    ' ' != licbuf[i] &&
				   '\t' != licbuf[i] &&
				   '\n' != licbuf[i]) {		
				   if (0 == strncmp(licbuf, CAPQ"_LICENSE:", 12)) 
				   {
					    i=12;
				   }
				   /* key data copy (24charcter) */
				   k = j = 0;
				   for(;j < 29 && strlen(licbuf) > i;i++)
				   {
					if (isalnum(licbuf[i]) != 0)
					{
					    /* data copy */
					    cmdmsg.keydata.szRegKey[j] =licbuf[i];
					    j++; 
					    if ((j == k+4) && (j != 29))
					    {
						cmdmsg.keydata.szRegKey[j] = ' ';
						j++;
						k = j;
						
					    }
					}
				   }
				   if (!isRegistrationKeyValid(cmdmsg.keydata.szRegKey))
				   {
					/* license check error. */
					cmdmsg.keydata.szRegKey[0] = 0;
				   }
				   else
				   {
					break;
				   }
				}
			}
			memset(licbuf,0x00,sizeof(licbuf));
		}
					
		cmdmsg.keydata.iKeyExpirationTime = 
			ftd_mngt_key_get_licence_key_expiration_date( cmdmsg.keydata.szRegKey );
		fclose(Licfile);
		Licfile = NULL;
	}
    }
    else
    {   /* set key */
	if ( isRegistrationKeyValid(cmdmsg.keydata.szRegKey) )
	{
		/* license check success.license data save to DTC.lic */
		if ((Licfile = fopen(LIC_FILE,"w")) == NULL)
		{
			/* license file open error */
        		logout(4,F_ftd_mngt_registration_key_req,"unable to retreive current "PRODUCTNAME" Agent registration key.\n");
			cmdmsg.keydata.szRegKey[0] = 0; /* make sure key is empty */
		}
 		else 
		{
			sprintf(logmsg,"license data = %s\n", cmdmsg.keydata.szRegKey);
        		logout(17,F_ftd_mngt_registration_key_req,logmsg);
			/* license file open success */
			if (fprintf(Licfile,"%s",cmdmsg.keydata.szRegKey) != 0)
			{
				/* license data read success */
				logout(17,F_ftd_mngt_registration_key_req,"license data put success.\n");
				cmdmsg.keydata.iKeyExpirationTime = 
					ftd_mngt_key_get_licence_key_expiration_date( cmdmsg.keydata.szRegKey );
			}
			else 
			{
				/* license file write error */
        			logout(4,F_ftd_mngt_registration_key_req,"license data write error.\n");
				cmdmsg.keydata.szRegKey[0] = 0; /* make sure key is empty */
			}
			fclose(Licfile);
			Licfile = NULL;
		}
	}
	else
	{
		/* license check error. */
   		logout(4,F_ftd_mngt_registration_key_req,"license data check error.\n");
		cmdmsg.keydata.szRegKey[0] = 0; /* make sure key is empty */
	}
    }

    /***************************************************/
    /* send registration key to requester              */
    cmdmsg.hdr.magicnumber    = MNGT_MSG_MAGICNUMBER;
    cmdmsg.hdr.mngttype       = MMP_MNGT_REGISTRATION_KEY;
    cmdmsg.hdr.sendertype     = SENDERTYPE_TDMF_AGENT;
    cmdmsg.hdr.mngtstatus     = MMP_MNGT_STATUS_OK;
    /* convert to network byte order before sending on socket */
    mmp_convert_mngt_hdr_hton(&cmdmsg.hdr);
    cmdmsg.keydata.iKeyExpirationTime = htonl(cmdmsg.keydata.iKeyExpirationTime)
;
    r = ftd_sock_send(sockID,(char*)&cmdmsg,sizeof(cmdmsg));
    if ( r != sizeof(cmdmsg) )
    {
   	logout(4,F_ftd_mngt_registration_key_req,"while sending registration key to requester.\n");
    }
    else
    {
   	logout(17,F_ftd_mngt_registration_key_req,"Registration key sent as response.\n");
    }
    return 0;
}

/* PUSH license key to Collector */
void ftd_mngt_send_registration_key()
{
    char licbuf[256 +1];
    FILE *Licfile;
    mmp_mngt_RegistrationKeyMsg_t   msg;
    int r,i,j,k,towrite;

    // This szServerUID field is the only one where two values are inserted.
    // It's a hack to allow both the identifier and license hostids to reach the collector/DMC.
    sprintf(msg.keydata.szServerUID, "%s %08x", gszServerUID, my_gethostid(HOSTID_LICENSE) );

    if ((Licfile = fopen(LIC_FILE,"r")) == NULL)
    {
            /* license file open error */
            logoutx(4,F_ftd_mngt_send_registration_key,"fopen error", "errno", errno);
            msg.keydata.szRegKey[0] = 0; /* make sure key is empty */
	    msg.keydata.iKeyExpirationTime = -1; /* KEY_NULL  */
    }
    else
    {
            /* license file open success */
	    while(!feof(Licfile) && NULL != fgets(licbuf, 256, Licfile)) {	
		    if (licbuf[strlen(licbuf)-1] == 0x0a)
				licbuf[strlen(licbuf)-1] = 0x00;
		    memset(msg.keydata.szRegKey,0x00,sizeof(msg.keydata.szRegKey));
	    	    i = 0;
		    if (0 < strlen(licbuf)) {
			    if ('#' != licbuf[i] &&
			        ' ' != licbuf[i] &&
			       '\t' != licbuf[i] &&
			       '\n' != licbuf[i]) {		
			       if (0 == strncmp(licbuf, CAPQ"_LICENSE:", 12)) 
			       {
				    i=12;
			       }
				/* key data copy (24charcter) */
				k = j = 0;
				for(;j < 29 && strlen(licbuf) > i;i++)
				{
				    if (isalnum(licbuf[i]) != 0)
				    {
				        /* data copy */
				        msg.keydata.szRegKey[j] =licbuf[i];
				        j++; 
					if ((j == k+4) && (j != 29))
					{
					    msg.keydata.szRegKey[j] = ' ';
					    j++;
					    k = j;
					}
				    }
			       }
			       if (!isRegistrationKeyValid(msg.keydata.szRegKey))
			       {
				    /* license check error. */
				    msg.keydata.szRegKey[0] = 0;
			       }
			       else
			       {
			           break;
			       }
			    }
		    }
		    memset(licbuf,0x00,sizeof(licbuf));
	    }
					
	    msg.keydata.iKeyExpirationTime = 
		ftd_mngt_key_get_licence_key_expiration_date( msg.keydata.szRegKey );
            fclose(Licfile);
            Licfile = NULL;
    }

    /***************************************************/
    /* send registration key to requester              */
    msg.hdr.magicnumber    = MNGT_MSG_MAGICNUMBER;
    msg.hdr.mngttype       = MMP_MNGT_REGISTRATION_KEY;
    msg.hdr.sendertype     = SENDERTYPE_TDMF_AGENT;
    msg.hdr.mngtstatus     = MMP_MNGT_STATUS_OK;
    /* convert to network byte order before sending on socket */
    mmp_convert_mngt_hdr_hton(&msg.hdr);
    msg.keydata.iKeyExpirationTime = htonl(msg.keydata.iKeyExpirationTime);
    towrite = sizeof(msg);
    r = sock_sendtox(giTDMFCollectorIP,
			giTDMFCollectorPort,(char*)&msg,towrite,0,0);
    if ( r != towrite )
    {
        logout(4,F_ftd_mngt_send_registration_key,"while sending registration key to Collector.\n");
    }
}

#endif /* _FTD_MNGT_REGISTRATION_KEY_REQ_C_ */
