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
/* #ident "@(#)$Id: Agent_uty.c,v 1.18 2018/02/28 00:25:45 paulclou Exp $" */
/* 
 * Copyright (C) Softek Technology Corporation. 2002, 2003.
 * All Rights Reserved.
 */
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <pwd.h>
#include <stdlib.h>
#include <dirent.h>
#if defined(SOLARIS) || defined(HPUX)
#include <macros.h>
#endif
#if defined(HPUX)
#include <mntent.h>
#endif
#include <fcntl.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

#include "tdmfAgent.h"
#include "tdmfAgent_trace.h"

#include "common.h"

#if 0
#if defined(HPUX)
#define FTD_SYSTEM_CONFIG_FILE DTCCFGDIR"/" QNM ".conf"
#elif defined(SOLARIS)                
#define FTD_SYSTEM_CONFIG_FILE "/usr/kernel/drv/" QNM ".conf"
#elif defined(_AIX)
#define FTD_SYSTEM_CONFIG_FILE "/usr/lib/drivers/" QNM ".conf"
#elif defined(linux)
#define MODULES_CONFIG_PATH "/etc/modprobe.d/sftkdtc.conf"
#define FTD_SYSTEM_CONFIG_FILE PATH_DRIVER_FILES "/" QNM ".conf"
#endif
#endif

/* function return values, if anyone cares */
#define CFG_OK                  0
#define CFG_BOGUS_PS_NAME      -1
#define CFG_MALLOC_ERROR       -2
#define CFG_READ_ERROR         -3
#define CFG_WRITE_ERROR        -4
#define CFG_BOGUS_CONFIG_FILE  -5

#define CFG_IS_NOT_STRINGVAL   0
#define CFG_IS_STRINGVAL       1

#define CFG_MAX_KEY_SIZE       32
#define CFG_MAX_DATA_SIZE      1024 
 
int cfg_get_key_value(char *key, char *value, int stringval);
int cfg_set_key_value(char *key, char *value, int stringval);

extern int agent_getnetconfs(unsigned int *iplist);
void cnvmsg(char *,int);
char *parmcpy(char *,char *,int,char*,char*);
void abort_Agn();
void atollx(char [],unsigned long long *);
void lltoax(char [],unsigned long long);

int getversion_num();								   
void htonI64(ftd_int64 *);

int     dbg;
// FILE	*log;
int  Debug;
int RequestID;
int Alarm;

extern char    logmsg[];

#if defined(linux)
static char *getline_linux (char **buffer, int *linelen);
#else
static char *getline (char **buffer, int *linelen);
#endif

/*
 * cfg_get_software_key_value --
 */
int cfg_get_software_key_value(char *key,       /* get key */
            char *data, int size)               /* read data, data size */
{
	FILE *cfg;
	int rc = -1;
	int i,j;
	int keylen;
	char text[CFG_MAX_KEY_SIZE + CFG_MAX_DATA_SIZE];
	
	cfg = fopen(AGN_CFGFILE,"r");
	if (cfg == NULL)
	{
		return (-1);
	}
	if ((keylen = strlen(key)) > CFG_MAX_KEY_SIZE) {
		sprintf(logmsg, "The key length of %s exceeds %d.\n", key, CFG_MAX_KEY_SIZE);
		logout(6, F_cfg_get_software_key_value, logmsg);
		fclose(cfg);
		return (-1);
	}
	for(;;)
	{
		memset(text,0x00,sizeof(text));
		fgets(text,sizeof(text),cfg);
		if ((i = strlen(text)) == 0)
		{
			break;
		}
                for(j=0;j<i;j++)
                {
                    if (text[j] == '\n') text[j] = 0x00;
                }
		if (strncmp(key,text,keylen) == 0) 
		{
			memset(data,0x00,size);
			i = keylen;
			j = strlen(text);
			if (j-i-1 > size)
			{
				sprintf(logmsg, "The data length of %s exceeds %d.\n", key, size);
				logout(6, F_cfg_get_software_key_value, logmsg);
				fclose(cfg);
				return (-1);
			}
				
			strcpy(data,&text[i+1]);
			rc = 0;
		}
	}
	fclose(cfg);
	return rc;
	
}

/*
 * cfg_set_software_key_value --
 */
int cfg_set_software_key_value(char *key,       /* get key */
			char *data)                      /* read data */
{
	int     rc = 0;
	int     flag = 0;
	int     freeflag = 1;
	FILE    *infd;
	FILE    *outfd;
	char    buf[1024+1];
#if defined(linux)
	char    tmpfile[PATH_MAX] = AGTTMPDIR"/"QAGN"_tmpcfg_XXXXXX";
	int     tmp_flag = -1;
#else
	char    *tmpfile = NULL;
#endif
	/* copy services file */
#if defined(linux)
	tmp_flag = mkstemp(tmpfile);
	if (tmp_flag < 0)
#else
	tmpfile = (char *)tempnam(AGTTMPDIR,QAGN);
	if (tmpfile == NULL)
#endif
	{
		/* tmp file alloc error */
#if defined(linux)
		strcpy(tmpfile, PRE_AGN_CFGFILE);
#else
		tmpfile = PRE_AGN_CFGFILE;
#endif
		freeflag = 0;
	}
#if defined(linux)
	sprintf(buf,"/bin/cp %s %s 1>/dev/null 2> /dev/null",AGN_CFGFILE, tmpfile);
#else
	sprintf(buf,"/usr/bin/cp %s %s 1>/dev/null 2> /dev/null",AGN_CFGFILE, tmpfile);
#endif
	system(buf);

	/* opem config file */
	outfd = fopen(AGN_CFGFILE,"w");
	if (outfd <= (FILE *)NULL)
	{
		/* config file open error */
		unlink(tmpfile);
#if !defined(linux)
		if (freeflag != 0) free(tmpfile);
#endif
		return (-1);
	}
	infd = fopen(tmpfile,"r");
	if (infd <= (FILE *)0)
	{
		/* file open error */
		fclose (outfd);
		outfd = NULL;
		unlink(tmpfile);
#if !defined(linux)
		if (freeflag != 0) free(tmpfile);
#endif
		return (-1);
	}

	/* copy servies data */
	for(;;)
	{
		memset(buf,0x00,sizeof(buf));
		fgets(buf,sizeof(buf),infd);
		if (strlen(buf) == 0)
		{
			break;
		}
		if (strncmp(key,buf,strlen(key)) == 0)
		{
			sprintf(buf,"%s:%s\n",key,data);
			flag = 1;
		}
		fputs(buf,outfd);
	}

	/* request key not sprcified ? */
	if (flag == 0)
	{
		sprintf(buf,"%s:%s\n",key,data);
		fputs(buf,outfd);
	}
	fclose(infd);
	fclose(outfd);
	unlink(tmpfile);
#if !defined(linux)
	if (freeflag != 0) free(tmpfile);
#endif
	return(rc);
}

/*
 * ipstring_to_ip -- convert ip address string to ip address
 */	
int ipstring_to_ip_uint(char *name, unsigned int *ip)
{
	if (name == NULL || strlen(name) == 0) {
		return -1;
	}

	{
		int             n1=0, n2=0, n3=0, n4=0;

		char    *s, *lname = strdup(name);

		if ((s = strtok(lname, ".\n")))
			n1 = atoi(s);
		if ((s = strtok(NULL, ".\n")))
			n2 = atoi(s);
		if ((s = strtok(NULL, ".\n")))
			n3 = atoi(s);
		if ((s = strtok(NULL, ".\n")))
			n4 = atoi(s);

		free(lname);

		*ip = n4 + (n3 << 8) + (n2 << 16) + (n1 << 24);

	}

	return 0;
}  

/*
 * ip_to_name -- convert ip address to hostname
 */
int ip_to_name(ipAddress_t ip, char *name)
{
	struct hostent  *host;
	unsigned int   addr;
	struct in6_addr16* addr6;
	 
	char                    ipstring[48], **p;

	ip_to_ipstring(ip, ipstring);

	if(ip.Version == IPVER_4) {
		addr = inet_addr(ipstring);
		host = gethostbyaddr((const char *)&addr, sizeof(addr), AF_INET);
	
		if (host == NULL) {
			return -1;
		}
		p = host->h_addr_list;
		strcpy(name, host->h_name);
	}
	else {
#if !defined(FTD_IPV4)
		inet_pton(AF_INET6,ipstring,addr6);
		host = gethostbyaddr((const char *)&addr6, sizeof(addr6), AF_INET6);
	
		if (host == NULL) {
			return -1;
		}
		p = host->h_addr_list;
		strcpy(name, host->h_name);
#endif

	}

   
	return 0;
}

/*
 * sock_ip_to_ipstring -- convert ip to ipstring in dot notation  
 */
int sock_ip_to_ipstring(ipAddress_t ip, char *ipstring) {
   
	ip_to_ipstring(ip, ipstring);

	return 0;
}

/*
 * ip_to_ipstring -- convert ip address to ipstring in dot notation
 *
 *                   AC, 2002-04-10 : bug correction:
 *                   Make sure ip value is parsed as a network byte order
 *                   and not Intel (little endian) byte order
 */	 
int ip_to_ipstring_uint(unsigned int ip, char *ipstring)
{ 
	int a1, a2, a3, a4;
	a1 = 0x000000ff & (ip >> 24);
	a2 = 0x000000ff & (ip >> 16);
	a3 = 0x000000ff & (ip >> 8);
	a4 = 0x000000ff & ip;

	sprintf(ipstring, "%d.%d.%d.%d", a1, a2, a3, a4);
 	
	return (0);
}
		 
/*
 * name_is_ipstring -- determine if the hostname is an ip address string
 */
int name_is_ipstring(char *strip)
{
	long ip_1, ip_2, ip_3, ip_4;
	int  i, len;
	char *ptr;
	char *endp;
	char ipaddr[IP_ADDR_LEN+1];
    
	/* Only "XXX.XXX.XXX.XXX" form is permitted. */
	if ((len = strlen(strip)) > IP_ADDR_LEN) return 0;

	ip_1 = strtol(strip, &endp, 10);
	if (endp == NULL || endp[0] != '.') return 0;
	ptr = endp + 1;
	ip_2 = strtol(ptr, &endp, 10);
	if (endp == NULL || endp[0] != '.') return 0;
	ptr = endp + 1;
	ip_3 = strtol(ptr, &endp, 10);
	if (endp == NULL || endp[0] != '.') return 0;
	ptr = endp + 1;
	ip_4 = strtol(ptr, &endp, 10);

	if (ip_1 < 0 || ip_1 > MAX_IP_ADDRESS_VALUE ||
			ip_2 < 0 || ip_2 > MAX_IP_ADDRESS_VALUE ||
			ip_3 < 0 || ip_3 > MAX_IP_ADDRESS_VALUE ||
			ip_4 < 0 || ip_4 > MAX_IP_ADDRESS_VALUE) {
		return 0;
	}
	memset(ipaddr, 0, sizeof(ipaddr));
	sprintf(ipaddr, "%ld.%ld.%ld.%ld", ip_1, ip_2, ip_3, ip_4);
	if (strcmp(strip, ipaddr) != 0) return 0;

	return 1;
} 

/***********************IPV6********************************/
/***********************************************************/
/***********************************************************/
/***********************************************************/
/***********************************************************/
/***********************************************************/



/***************************************************************************************\

Function:       is_unspecified

Description:    Check address validity.

Parameters:     pip

Return Value:   int

Comments:       None.

\***************************************************************************************/
int is_unspecified(ipAddress_t pIp)
{
    int iUnspecified = 1;

ABORT_TRY
    {
      
    iUnspecified = 0;

    switch(pIp.Version)
        {
        case IPVER_4:
            if (pIp.Addr.V4 == 0)
                {
                iUnspecified = 1;
                }
            break;

        case IPVER_6:
            if ( (pIp.Addr.V6.Word[0] == 0) && 
                 (pIp.Addr.V6.Word[1] == 0) && 
                 (pIp.Addr.V6.Word[2] == 0) && 
                 (pIp.Addr.V6.Word[3] == 0) && 
                 (pIp.Addr.V6.Word[4] == 0) && 
                 (pIp.Addr.V6.Word[5] == 0) && 
                 (pIp.Addr.V6.Word[6] == 0) &&
                 (pIp.Addr.V6.Word[7] == 0))
               {
               iUnspecified = 1;
               }
            break;

        case IPVER_UNDEFINED:
        default:
            iUnspecified = 1;
            break;
        }
    }
ABORT_CATCH_FINALLY
    {
    iUnspecified = 1;
    }
ABORT_FINALLY
    {
    }
 
    return iUnspecified;
}

/***************************************************************************************\

Function:       ipReset

Description:    Reset IP address.

Parameters:     poIpAddress

Return Value:   int

Comments:       None.

\***************************************************************************************/
int ipReset(ipAddress_t* poIpAddress)
{
    int iRet = -1;

ABORT_TRY
    {
    ABORT_IF(poIpAddress == NULL);

    memset(poIpAddress, 0, sizeof(ipAddress_t));
    
    iRet = 0;
    }
ABORT_CATCH_FINALLY
    {
    iRet = -1;
    }
ABORT_FINALLY
    {
    }

    return 0;
} 


/***************************************************************************************\

Function:       extractLong10

Description:    Read number (unsigned long, base 10)

Parameters:     currChar: first character of number to read
				extracted (out): extracted number

Return Value:   char*: new position in string, past the extracted value (NULL if error)

\***************************************************************************************/
static char* extractLong10(char *currChar, unsigned long *extracted)
{
	char *initialPos = currChar;
	char currDigit;
	*extracted = 0;

	while (isdigit(*currChar))
	{
		currDigit = (*currChar - '0');

		// Check for potential overflow
		if (*extracted > (0xffffffff-currDigit)/10)
		{
			return NULL;
		}

		*extracted *= 10;
		*extracted += currDigit;
		++currChar;
	}

	// Make sure we actually read something
	if (currChar > initialPos)
	{
		return currChar;
	}

	return NULL;
}

/***************************************************************************************\

Function:       extractByte10

Description:    Read number (unsigned char, base 10)

Parameters:     currChar: first character of number to read
				extracted (out): extracted number

Return Value:   char*: new position in string, past the extracted value (NULL if error)

\***************************************************************************************/
static char* extractByte10(char *currChar, unsigned char *extracted)
{
	unsigned long temp;
	currChar = extractLong10(currChar, &temp);

	if (currChar != NULL && temp < 256)
	{
		*extracted = (unsigned char)temp;
		return currChar;
	}

	return NULL;
}
/***************************************************************************************\

Function:       getHexValue

Description:    Read ASCII hex digit

Parameters:     hexDigit: ASCII hexadecimal digit [0..9A..Fa..f]

Return Value:   char: value if valid digit, -1 if not

\***************************************************************************************/
static char getHexValue(char hexDigit)
{
	static const char hexNumbers[] = "0123456789ABCDEF";

	char *hexIndex = strchr(hexNumbers, toupper(hexDigit));
	if (hexIndex != NULL)
	{
		return (char)(hexIndex-hexNumbers);
	}

	return 'z';
}

/***************************************************************************************\

Function:       extractIPv4

Description:    Read IPv4 address from string

Parameters:     currChar: first character of IP  to read
				ipv4 (out): address extracted

Return Value:   char*: new position in string, past the extracted IP (NULL if error)

\***************************************************************************************/
static char* extractIPv4(char *currChar, unsigned char ipv4[IPV4_BYTE_LEN])
{
	int tokens;

	// Expecting well-formed IPv4 address: a.b.c.d
	for (tokens=0; tokens<IPV4_BYTE_LEN; ++tokens)
	{
		currChar = extractByte10(currChar, ipv4+tokens);

		// Check for unexpected end of string or missing '.'
		if (currChar == NULL || *currChar != '.')
		{
			break;
		}

		++currChar; // Skip '.'
	}

	// Check for 
	//  1) Parsing error
	//  2) Correct number of tokens (4)
	if (currChar != NULL &&
		tokens == (IPV4_BYTE_LEN-1))
	{
		return currChar; // Everything is OK
	}

	return NULL;
}

/***************************************************************************************\

Function:       extractIPv4InIPv6

Description:    Read IPv4 address embedded in IPv6 (i.e. 1:2:3::5.6.7.8)

Parameters:     currChar: first character of IP  to read
				addr (in/out): extracted address will be put in the current position 
					of the IPv6 address being parsed (an IPv4 address fills up two Words 
					of the IPv6 structure)
				currWordIndex (in/out): current word being read in the IPv6 address.  
					This is updated if a valid address is extracted

Return Value:   char*: new position in string, past the extracted IP (NULL if error)

\***************************************************************************************/
static char* extractIPv4InIPv6(char *currChar, ipAddress_t *addr, int *currWordIndex)
{
	unsigned char IPv4[IPV4_BYTE_LEN];

	// Check for enough space (need 2 Words)
	if (*currWordIndex > IPV6_SHORT_LEN-2)
	{
		return NULL;
	}

	currChar = extractIPv4(currChar, IPv4);

	if (currChar != NULL && (*currChar == '\0' || *currChar == '%'))
	{
		memcpy(&addr->Addr.V6.Word[*currWordIndex], IPv4, sizeof(IPv4));
		*currWordIndex += 2;
	}

	return currChar;
}

/***************************************************************************************\

Function:       extractWord16 

Description:    Get IPv6 word (16 bytes block in hex) from string.

Parameters:     currChar: first character of word to read
				addr (in/out): extracted word will be put in the current position 
					of the IPv6 address being parsed
				currWordIndex (in/out): current word being read in the IPv6 address.  
					This is updated if a word is extracted

Return Value:   char*: new position in string, past the extracted IP (NULL if error)

\***************************************************************************************/


static char* extractWord16(char *currChar, ipAddress_t *addr, int *currWordIndex)
{
	char nibble;
	int currNibble = 0;
	int extracted = 0;

	char *initialPos = currChar;
	unsigned short *currWord = addr->Addr.V6.Word + *currWordIndex;

	// Check if IP is already full
	if (*currWordIndex >= IPV6_SHORT_LEN)
		return NULL;

	// Skip leading '0'
	while (*currChar  != '\0')
	{
		nibble = getHexValue(*currChar);
		if (nibble != 'z') // This will break on error (-1) or first non-zero number
		{
			break;
		}

		++currChar;
		++currNibble;
		extracted = 1;
	}

	while (*currChar != '\0')
	{
		nibble = getHexValue(*currChar);

		if (nibble == 'z') // Break on error
		{
			break;
		}

		*currWord <<= 4;
		*currWord |= nibble;

		++currChar;
		++currNibble;
		extracted = 1;
	}

	// Special processing for IPv4 addresses embedded at the end of IPv6 ones 
	if (*currChar == '.')
	{
		return extractIPv4InIPv6(initialPos, addr, currWordIndex);
	}

	// No more than 4 hex digits in a word
	if (currNibble > 4)
	{
		return NULL;
	}

	// Fix byte order
	*currWord = htons(*currWord);

	if (extracted)
	{
		++(*currWordIndex);
	}

	return currChar;	

}
										 
 /***************************************************************************************\

Function:       get_ip_version

Description:    none.

Parameters:     ipStr

Return Value:   ip_address_version_t

Comments:       None.

\***************************************************************************************/
ip_address_version_t get_ip_version(char* ipStr)
{
	if (strchr(ipStr, ':') != NULL) // IPv6
	{
		return IPVER_6;
	}
	else if (strchr(ipStr, '.') != NULL) // IPv4
	{
		return IPVER_4;
	}

	return IPVER_UNDEFINED;
}
/***************************************************************************************\

Function:       ipstring_to_ip 

Description:    Get IP address from string.  Supports IPv4 & IPv6 addresses

Parameters:     ipStr: string to parse
				addr (out): extracted address

Return Value:   int: 0 if valid, -1 if invalid

\***************************************************************************************/
int ipstring_to_ip(char* ipStr, ipAddress_t *ip)
{
	if (ipStr == NULL || ip == NULL)
	{
		return -1;
	}

	ipReset(ip);
	ip->Version = get_ip_version(ipStr);

	switch(ip->Version)
	{
	case IPVER_4:
		return ipstring_to_ipv4(ipStr, ip);
	case IPVER_6:
#if !defined(FTD_IPV4)
		return ipstring_to_ipv6(ipStr, ip);
#endif
	default:
		return -1;
	}
}

/***************************************************************************************\

Function:       ipstring_to_ipv4 

Description:    Get IPv4 address from string.

Parameters:     ipStr: string to parse
				addr (out): extracted IPv4 address

Return Value:   int: 0 if valid, -1 if invalid

\***************************************************************************************/
int ipstring_to_ipv4(char* ipStr, ipAddress_t *ip)
{
	unsigned char IPv4[IPV4_BYTE_LEN];
	char *currChar = extractIPv4(ipStr, IPv4);

   	if (currChar != NULL && *currChar == '\0' )
	{
		memcpy(&ip->Addr.V4, IPv4, sizeof(IPv4));
		ip->Version = IPVER_4;
		return 0;
 	}
	else
	{
		ip->Version = IPVER_UNDEFINED;
		return -1;
	}
}
#if !defined(FTD_IPV4)
/***************************************************************************************\

Function:       ipstring_to_ipv6 

Description:    Get IPv6 address from string.

Parameters:     ipStr: string to parse
				addr (out): extracted IPv6 address

Return Value:   int: 0 if valid, -1 if invalid

\***************************************************************************************/
int ipstring_to_ipv6(char* ipStr, ipAddress_t *ip)
{
	char* currChar;
	int wildIndex = -1;
	int toPad;
	int toMove;
	int currWordIndex = 0;
	int colonCount;

	currChar = ipStr;
	ip->Version = IPVER_6;

	while (*currChar)
	{
		currChar = extractWord16(currChar, ip, &currWordIndex);

		if (currChar == NULL) // Read error
		{
			ip->Version = IPVER_UNDEFINED;
			return -1;
		}
		else if (*currChar == '\0')
		{
			break;
		}
		else if (*currChar == ':')
		{
			colonCount = 0;

			// Count consecutive ':'
			while (*currChar == ':')
			{
				++colonCount;
				++currChar;
			}
			
			if (colonCount == 2 && wildIndex == -1)
			{
				// Flag wildcard position for later processing
				wildIndex = currWordIndex;
			}
			else if (colonCount == 1)
			{
				// Check if beginning or end, this shouldn't be valid
				if ((currChar-ipStr) == 1 || *currChar == '\0' || *currChar == '%')
				{
					ip->Version = IPVER_UNDEFINED;
					return -1;
				}
		 	}
			else  
			{
				// Invalid number or colons, or already have '::'
				ip->Version = IPVER_UNDEFINED;						  
				return -1;
			}
		}
		else if (*currChar == '%') // Scope ID for link-local addresses
		{
			++currChar;
			currChar = extractLong10(currChar, &ip->Addr.V6.ScopeID);

			// Check if we read something and if we're now at the end of the string
			if (currChar == NULL || *currChar != '\0')
			{
				ip->Version = IPVER_UNDEFINED;
				return -1;
			}
		}
		else
		{
			// Invalid character
			ip->Version = IPVER_UNDEFINED;
			return -1;
		}
	}

	// Check if we're done
	if (currWordIndex == IPV6_SHORT_LEN && wildIndex == -1) 
	{
		return 0;
	}
	
	// Need to expand wildcard?
	if (wildIndex != -1 && currWordIndex < IPV6_SHORT_LEN) 
	{
		toMove = currWordIndex-wildIndex;
		if (toMove > 0)
		{
			toPad = IPV6_SHORT_LEN-toMove;

			memmove(
				ip->Addr.V6.Word+toPad, 
				ip->Addr.V6.Word+wildIndex, 
				toMove*sizeof(unsigned short));

			memset(
				ip->Addr.V6.Word+wildIndex, 
				0, 
				(toPad-wildIndex)*sizeof(unsigned short));
		}
		return 0;
	}

	// If we got here, the address is incomplete
	ip->Version = IPVER_UNDEFINED;
	return -1;
}
#endif
/***************************************************************************************\

Function:       ip_to_sockaddr

Description:    converts an ipAddress_t to a SOCKADDR_STORAGE.

Parameters:     ip
                sockaddrStorage

Return Value:   int

Comments:       None.

\***************************************************************************************/
int ip_to_sockaddr(ipAddress_t *ip, struct sockaddr_storage *sockaddrStorage)
{
	int i =0;
	if (ip != NULL && sockaddrStorage != NULL)
	{
		if (ip->Version == IPVER_4)
		{

			struct sockaddr_in* sockaddrV4 = (struct sockaddr_in *)sockaddrStorage;

			sockaddrV4->sin_family = AF_INET;
			sockaddrV4->sin_addr.s_addr = ip->Addr.V4;
			return 0;
		}
		else if (ip->Version == IPVER_6)
		{
#if !defined(FTD_IPV4)

			struct sockaddr_in6* sockaddrV6 = (struct sockaddr_in6 *)sockaddrStorage;

			sockaddrV6->sin6_family = AF_INET6;
			memcpy(sockaddrV6->sin6_addr.s6_addr,ip->Addr.V6.Word, sizeof(ip->Addr.V6.Word));

			return 0;
#endif
		}
		else
		{


			return -1;

		}				
	}

	return -1;
}

/***************************************************************************************\

Function:       ip_to_ipstring

Description:    convert ip address to ip address string.

Parameters:     ip
                ipstring

Return Value:   int

Comments:       None.

\***************************************************************************************/
int ip_to_ipstring(ipAddress_t ip, char* ipstring)
{
	
#if defined(FTD_IPV4)
ip_to_ipstring_uint(ip.Addr.V4, ipstring);
return 0;


#else	
	char nameTemp[1025];
	struct sockaddr_storage sockaddrStorage;
	int storageLength = sizeof(struct sockaddr_storage);
	memset(&sockaddrStorage, 0, storageLength);


  	if (ip.Addr.V4 == 0 || ipstring == NULL)
	{
		return -1;
	} 

	ipstring[0] = '\0';

    if (ip_to_sockaddr(&ip, &sockaddrStorage) == -1)
	{
		return -1;
	}

	if (getnameinfo((struct sockaddr*) &sockaddrStorage, storageLength, nameTemp, sizeof(nameTemp), NULL, 0, NI_NUMERICHOST) != 0)
	{
		return -1;
	}

	strcpy(ipstring, nameTemp);

#endif


	return 0;
}

void mmp_convert_mngt_hdrEx_hton(mmp_MessageHeaderEx* hdrEx)
{
    hdrEx->ulMessageSize    = htonl(hdrEx->ulMessageSize);
    hdrEx->ulMessageVersion = htonl(hdrEx->ulMessageVersion);
    hdrEx->ulInstanceCount  = htonl(hdrEx->ulInstanceCount);
}

void mmp_convert_mngt_hdrEx_ntoh(mmp_MessageHeaderEx* hdrEx)
{
    hdrEx->ulMessageSize    = ntohl(hdrEx->ulMessageSize);
    hdrEx->ulMessageVersion = ntohl(hdrEx->ulMessageVersion);
    hdrEx->ulInstanceCount  = ntohl(hdrEx->ulInstanceCount);
}

void mmp_convert_TdmfServerInfo2_hton(mmp_TdmfServerInfo2* srvrInfo2)
{
    srvrInfo2->AgentListenPort      = htonl(srvrInfo2->AgentListenPort);
    srvrInfo2->TCPWindowSize        = htonl(srvrInfo2->TCPWindowSize);
    srvrInfo2->CPUCount             = htonl(srvrInfo2->CPUCount);
    srvrInfo2->RAMSize              = htonl(srvrInfo2->RAMSize);
    srvrInfo2->AvailableRAMSize     = htonl(srvrInfo2->AvailableRAMSize);
    srvrInfo2->Features.BAB.RequestedSize                   = htonl(srvrInfo2->Features.BAB.RequestedSize);
    srvrInfo2->Features.BAB.ActualSize                      = htonl(srvrInfo2->Features.BAB.ActualSize);
    srvrInfo2->Features.PStore.HighResolutionBitmap.Size    = htonl(srvrInfo2->Features.PStore.HighResolutionBitmap.Size); // <<<
    srvrInfo2->Features.PStore.LowResolutionBitmap.Size     = htonl(srvrInfo2->Features.PStore.LowResolutionBitmap.Size);
}

void mmp_convert_TdmfServerInfo2_ntoh(mmp_TdmfServerInfo2* srvrInfo2)
{
    srvrInfo2->AgentListenPort      = ntohl(srvrInfo2->AgentListenPort);
    srvrInfo2->TCPWindowSize        = ntohl(srvrInfo2->TCPWindowSize);
    srvrInfo2->CPUCount             = ntohl(srvrInfo2->CPUCount);
    srvrInfo2->RAMSize              = ntohl(srvrInfo2->RAMSize);
    srvrInfo2->AvailableRAMSize     = ntohl(srvrInfo2->AvailableRAMSize);
    srvrInfo2->Features.BAB.RequestedSize                   = ntohl(srvrInfo2->Features.BAB.RequestedSize);
    srvrInfo2->Features.BAB.ActualSize                      = ntohl(srvrInfo2->Features.BAB.ActualSize);
    srvrInfo2->Features.PStore.HighResolutionBitmap.Size    = ntohl(srvrInfo2->Features.PStore.HighResolutionBitmap.Size); // <<<
    srvrInfo2->Features.PStore.LowResolutionBitmap.Size     = ntohl(srvrInfo2->Features.PStore.LowResolutionBitmap.Size);
}

void mmp_convert_TdmfServerConfig_ntoh(mmp_TdmfServerConfig* srvrConfig)
{
    srvrConfig->iPort = ntohl(srvrConfig->iPort);
    srvrConfig->iTCPWindowSize = ntohl(srvrConfig->iTCPWindowSize);
    srvrConfig->iBABSizeReq = ntohl(srvrConfig->iBABSizeReq);
    srvrConfig->iLargePStoreSupport = ntohl(srvrConfig->iLargePStoreSupport); // <<<
}

      
/***********************END OF IPV6********************************/

void mmp_convert_mngt_hdr_ntoh(mmp_mngt_header_t *hdr)
{
	hdr->magicnumber = ntohl(hdr->magicnumber);
	hdr->mngttype    = ntohl(hdr->mngttype);
	hdr->sendertype  = ntohl(hdr->sendertype);
	hdr->mngtstatus  = ntohl(hdr->mngtstatus);
}

void mmp_convert_mngt_hdr_hton(mmp_mngt_header_t *hdr)
{
	hdr->magicnumber = htonl(hdr->magicnumber);
	hdr->mngttype    = htonl(hdr->mngttype);
	hdr->sendertype  = htonl(hdr->sendertype);
	hdr->mngtstatus  = htonl(hdr->mngtstatus);
}

void mmp_convert_TdmfServerInfo_hton(mmp_TdmfServerInfo *srvrInfo)
{
	srvrInfo->iPort             =   htonl(srvrInfo->iPort);
	srvrInfo->iTCPWindowSize    =   htonl(srvrInfo->iTCPWindowSize);
	srvrInfo->iBABSizeReq       =   htonl(srvrInfo->iBABSizeReq);
	srvrInfo->iBABSizeAct       =   htonl(srvrInfo->iBABSizeAct);
	srvrInfo->iNbrCPU           =   htonl(srvrInfo->iNbrCPU);
	srvrInfo->iRAMSize          =   htonl(srvrInfo->iRAMSize);
	srvrInfo->iAvailableRAMSize =   htonl(srvrInfo->iAvailableRAMSize);
}       

void mmp_convert_TdmfPerfConfig_ntoh(mmp_TdmfPerfConfig *perfCfg)
{
	perfCfg->iPerfUploadPeriod   = ntohl(perfCfg->iPerfUploadPeriod);
	perfCfg->iReplGrpMonitPeriod = ntohl(perfCfg->iReplGrpMonitPeriod);
}

void mmp_convert_TdmfAgentDeviceInfoEx_hton(mmp_TdmfDeviceInfoEx *devInfo)
{
	devInfo->sFileSystem = htons(devInfo->sFileSystem);
    htonI64((ftd_int64*)&(devInfo->liDriveId));
    htonI64((ftd_int64*)&(devInfo->liStartOffset));
    htonI64((ftd_int64*)&(devInfo->liLength));
    htonI64((ftd_int64*)&(devInfo->liDeviceMajor));
    htonI64((ftd_int64*)&(devInfo->liDeviceMinor));
}

void mmp_convert_TdmfAlert_hton(mmp_TdmfAlertHdr *alertData)
{
	alertData->sLGid        = htons(alertData->sLGid);
	alertData->sDeviceId    = htons(alertData->sDeviceId);
	alertData->uiTimeStamp  = htonl(alertData->uiTimeStamp);
	alertData->cSeverity    = (char)htons((short)alertData->cSeverity);
	alertData->cType        = (char)htons((short)alertData->cType);
}

void mmp_convert_TdmfAlert_ntoh(mmp_TdmfAlertHdr *alertData)
{
	alertData->sLGid        = ntohs(alertData->sLGid);
	alertData->sDeviceId    = ntohs(alertData->sDeviceId);
	alertData->uiTimeStamp  = ntohl(alertData->uiTimeStamp);
	alertData->cSeverity    = (char)ntohs((short)alertData->cSeverity);
	alertData->cType        = (char)ntohs((short)alertData->cType);
}

void mmp_convert_TdmfStatusMsg_hton(mmp_TdmfStatusMsg *statusMsg)
{
	statusMsg->iLength      = htonl(statusMsg->iLength);
	statusMsg->iTdmfCmd     = htonl(statusMsg->iTdmfCmd);
	statusMsg->iTimeStamp   = htonl(statusMsg->iTimeStamp);
}

void mmp_convert_TdmfReplGroupMonitor_hton(mmp_TdmfReplGroupMonitor *GrpMon)
{
	htonI64(&(GrpMon->liActualSz));
	htonI64(&(GrpMon->liDiskTotalSz));
	htonI64(&(GrpMon->liDiskFreeSz));
	// Do not touch this one: GrpMon->iReplGrpSourceIP	= htonl(GrpMon->iReplGrpSourceIP);
	GrpMon->sReplGrpNbr		= htons(GrpMon->sReplGrpNbr);
	GrpMon->isSource		= (char)htons(GrpMon->isSource);
}

void mmp_convert_TdmfGroupState_hton(mmp_TdmfGroupState *state)
{ 
	state->sRepGrpNbr	    = ntohs(state->sRepGrpNbr);
	state->sState	    = ntohs(state->sState);
}

/* This function works perfect with optimization turned on to -O2 */
void htonI64(ftd_int64 *int64InVal)
{
	int n = 1;
	if( *(char*)&n ){	/* Little endian */
	    *int64InVal =((uint64_t)htonl((uint32_t)*int64InVal) << 32) + (uint32_t)htonl((uint32_t)(*int64InVal >> 32));
	} else {		/* Big endian */
        /* Nothing to do as we're already in network order. */
	}
}

void mmp_convert_perf_instance_hton(ftd_perf_instance_t *perf)
{ 
	perf->connection =   (int)htonl(perf->connection);
	perf->drvmode    =   (int)htonl(perf->drvmode);
	perf->lgnum      =   (int)htonl(perf->lgnum);
	perf->insert     =   (int)htonl(perf->insert);
	perf->devid      =   (int)htonl(perf->devid);
	htonI64(&(perf->actual));
	htonI64(&(perf->effective));
	perf->rsyncoff   =   (int)htonl(perf->rsyncoff);
	perf->rsyncdelta =   (int)htonl(perf->rsyncdelta);
	perf->entries    =   (int)htonl(perf->entries);
	perf->sectors    =   (int)htonl(perf->sectors);
	perf->pctdone    =   (int)htonl(perf->pctdone);
	perf->pctbab     =   (int)htonl(perf->pctbab);
	htonI64(&(perf->bytesread));
	htonI64(&(perf->byteswritten));
}

void mmp_convert_TdmfServerInfo_ntoh(mmp_TdmfServerInfo *srvrInfo)
{           
	srvrInfo->iPort             =   ntohl(srvrInfo->iPort);
	srvrInfo->iTCPWindowSize    =   ntohl(srvrInfo->iTCPWindowSize);
	srvrInfo->iBABSizeReq       =   ntohl(srvrInfo->iBABSizeReq);
	srvrInfo->iBABSizeAct       =   ntohl(srvrInfo->iBABSizeAct);
	srvrInfo->iNbrCPU           =   ntohl(srvrInfo->iNbrCPU);
	srvrInfo->iRAMSize          =   ntohl(srvrInfo->iRAMSize);
	srvrInfo->iAvailableRAMSize =   ntohl(srvrInfo->iAvailableRAMSize);
}

/* WR15560 : add function  start */
void    mmp_convert_hton(ftd_perf_instance_t *perf)
{
	perf->connection =   (int)htonl(perf->connection);
	perf->drvmode    =   (int)htonl(perf->drvmode);
	perf->lgnum      =   (int)htonl(perf->lgnum);
	perf->insert     =   (int)htonl(perf->insert);
	perf->devid      =   (int)htonl(perf->devid);
	htonI64(&(perf->actual));
	htonI64(&(perf->effective));
	perf->rsyncoff   =   (int)htonl(perf->rsyncoff);
	perf->rsyncdelta =   (int)htonl(perf->rsyncdelta);
	perf->entries    =   (int)htonl(perf->entries);
	perf->sectors    =   (int)htonl(perf->sectors);
	perf->pctdone    =   (int)htonl(perf->pctdone);
	perf->pctbab     =   (int)htonl(perf->pctbab);
	htonI64(&(perf->bytesread));
	htonI64(&(perf->byteswritten));
}
/* WR15560 : add function  end */

void mmp_convert_FileTransferData_hton(mmp_TdmfFileTransferData *data)
{
	data->iType =	(int)htonl(data->iType);
	data->uiSize =	(unsigned int)htonl(data->uiSize);
}

/*
 * cfg_intr.c - system config file interface
 *
 * Copyright (c) 1998 FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */

/*
 * Parse the system config file for something like: key=value;
 *
 * If stringval is TRUE, then remove quotes from the value.
 */
int cfg_get_key_value(char *key, char *value, int stringval)
{
	int         fd, len;
	char        *ptr, *buffer, *temp, *line;
	struct stat statbuf;

	if (stat(FTD_SYSTEM_CONFIG_FILE, &statbuf) != 0) {
		return CFG_BOGUS_CONFIG_FILE;
	}

	/* open the config file and look for the requested key */
	if ((fd = open(FTD_SYSTEM_CONFIG_FILE, O_RDONLY)) == -1) {
		return CFG_BOGUS_CONFIG_FILE;
	}

	if ((buffer = (char*)malloc(2*statbuf.st_size)) == NULL) { /* TODO: Why allocate twice the size of the file? */
		close(fd);
		return CFG_MALLOC_ERROR;
	}

	/* read the entire file into the buffer. */
	if (read(fd, buffer, statbuf.st_size) != statbuf.st_size) {
		close(fd);
		free(buffer);
		return CFG_READ_ERROR;
	}
	close(fd);
    // The read value needs to be NULL terminated, otherwise, we'll keep looking for unfound values (and modify things) out of the allocated array.
    buffer[statbuf.st_size] = 0x0;
	temp = buffer;

	/* get lines until we find the right one */
#if defined(linux)
	while ((line = getline_linux(&temp, &len)) != NULL) {
#else
	while ((line = getline(&temp, &len)) != NULL) {
#endif
		if (line[0] == '#') {
			continue;
		} else if ((ptr = strstr(line, key)) != NULL) {
			/* search for quotes, if this is a string */
			if (stringval) {
				while (*ptr) {
					if (*ptr++ == '\"') {
						while (*ptr && (*ptr != '\"')) {
							*value++ = *ptr++;
						}
						*value = 0;
						free(buffer);
						return CFG_OK;
					}
				}
			} else {
				while (*ptr) {
					if (*ptr++ == '=') {
						while (*ptr && (*ptr != ';')) {
							*value++ = *ptr++;
						}
						free(buffer);
						return CFG_OK;
					}
				}
			}
		}
	}
	free(buffer);
	return CFG_BOGUS_CONFIG_FILE;
}

/*
 * Parse the system config file for something like: key=value;
 * Replace old value with new value. If key doesn't exist, add key/value.
 *
 * If stringval is TRUE, put quotes around the value.
 */
int cfg_set_key_value(char *key, char *value, int stringval)
{
	int         fd, found, linelen;
	char        *inbuffer, *outbuffer, *line;
	char        *tempin, *tempout;
	struct stat statbuf;

	if (stat(FTD_SYSTEM_CONFIG_FILE, &statbuf) != 0) {
		return CFG_BOGUS_CONFIG_FILE;
	}

	if ((fd = open(FTD_SYSTEM_CONFIG_FILE, O_RDWR)) < 0) {
		return CFG_BOGUS_CONFIG_FILE;
	}

	if ((inbuffer = (char *)calloc(statbuf.st_size+1, 1)) == NULL) {
		close(fd);
		return CFG_MALLOC_ERROR;
	}

	if ((outbuffer = (char *)malloc(statbuf.st_size+MAXPATHLEN)) == NULL) {
		close(fd);
		free(inbuffer);
		inbuffer = NULL;
		return CFG_MALLOC_ERROR;
	}

	/* read the entire file into the buffer. */
	if (read(fd, inbuffer, statbuf.st_size) != statbuf.st_size) {
		close(fd);
		free(inbuffer);
		free(outbuffer);
		inbuffer = NULL;
		outbuffer = NULL;
		return CFG_READ_ERROR;
	}
	close(fd);
	tempin = inbuffer;
	tempout = outbuffer;
	found = 0;

	/* get lines until we find one with: key=something */
#if defined(linux)
	while ((line = getline_linux(&tempin, &linelen)) != NULL) {
#else
	while ((line = getline(&tempin, &linelen)) != NULL) {
#endif
		/* if this is a comment line or it doesn't have the magic word ... */
		if ((line[0] == '#') || (strstr(line, key) == NULL)) {
			/* copy the line to the output buffer */
			strncpy(tempout, line, linelen);
			tempout[linelen] = '\n';
			tempout += linelen+1;
		} else {
			if (stringval) {
				sprintf(tempout, "%s=\"%s\";\n", key, value);
			} else {
				sprintf(tempout, "%s=%s;\n", key, value);
			}
			tempout += strlen(tempout);
			found = 1;
		}
	}
	free(inbuffer);

	if (!found) {
		if (stringval) {
			sprintf(tempout, "%s=\"%s\";\n", key, value);
		} else {
			sprintf(tempout, "%s=%s;\n", key, value);
		}
		tempout += strlen(tempout);
	}

	/* flush the output buffer to disk */
	if ((fd = creat(FTD_SYSTEM_CONFIG_FILE, S_IRUSR | S_IWUSR)) == -1) {
		free(outbuffer);
		outbuffer = NULL;
		return CFG_WRITE_ERROR;
	}
	lseek(fd, 0, SEEK_SET);
	write(fd, outbuffer, tempout - outbuffer);
	close(fd);

	free(outbuffer);

	return CFG_OK;
}

/*
 * Yet another "getline" function. Parses a buffer looking for the
 * EOL or a NULL. Bumps the buffer pointer to the character following
 * EOL or to NULL, if end-of-buffer. Replaces the EOL with a NULL.
 *
 * Returns NULL, if parsing failed (end-of-buffer reached).
 * Returns pointer to start-of-line, if parsing succeeded.
 */
static char *
#if defined(linux)
getline_linux (char **buffer, int *outlen)
#else
getline (char **buffer, int *outlen)
#endif
{
	int  len;
	char *tempbuf;

	tempbuf = *buffer;
	if (tempbuf == NULL) {
		return NULL;
	}

	/* search for EOL or NULL */
	len = 0;
	while (1) {
		if (tempbuf[len] == '\n') {
			tempbuf[len] = 0;
			*buffer = &tempbuf[len+1];
			break;
		} else if (tempbuf[len] == 0) {
			/* must be done! */
			*buffer = NULL;
			break;
		}
		len++;
	}
	if ((*outlen = len) == 0) {
		return NULL;
	}

	/* done */
	return tempbuf;
}

/*************************************************************/
/*                                                           */
/*    file lock func                                         */
/*                                                           */
/*************************************************************/

static struct sembuf op_lock[2] = {
	{2,0,0},
	{2,1,SEM_UNDO}
};

static struct sembuf op_endcreate[2] = {
	{1,-1,SEM_UNDO},
	{2,-1,SEM_UNDO}
};

static struct sembuf op_open[1] = {
	{1,-1,SEM_UNDO},
};

static struct sembuf op_close[3] = {
	{2,0,0},
	{2,1,SEM_UNDO},
	{1,1,SEM_UNDO}
};

static struct sembuf op_unlock[1] = {
	{2,-1,SEM_UNDO}
};

static struct sembuf op_op[1] = {
	{0,99,SEM_UNDO}
};

int sem_create(key_t key,int initval)
{
	register int id,semval;
	union semun {
		int val;
		struct semid_ds *buf;
		ushort		*array;
	}semctl_arg;

	if (key == IPC_PRIVATE)
		return (-1);
	else if (key == (key_t) -1)
		return (-1);
again:
	if ((id = semget(key,3,0666 | IPC_CREAT)) < 0)
		return (-1);
	if (semop(id,&op_lock[0],2) < 0) {
		if (errno == EINVAL)
			goto again;
		logout(19,F_sem_create,"can not lock.\n");
	}

	if ((semval = semctl(id,1,GETVAL,0)) < 0)
		logout(19,F_sem_create,"semctl GETVAL error\n");

	if (semval == 0) {
		semctl_arg.val = initval;
		if (semctl(id,0,SETVAL,semctl_arg) < 0)
		        logout(19,F_sem_create,"semctl SETVAL[0] error\n");
		semctl_arg.val = BIGCOUNT;
		if (semctl(id,1,SETVAL,semctl_arg) < 0)
		        logout(19,F_sem_create,"semctl SETVAL[1] error\n");
	}

	if (semop(id, &op_endcreate[0],2) < 0)
		logout(19,F_sem_create,"can not endcreate.\n");
	return(id);
}

void
sem_rm(int id)
{
	if(semctl(id,0,IPC_RMID,0) < 0)
		logout(19,F_sem_create,"semctl IPC_RMID error\n");
}

void
sem_close(int id)
{
	register int semval;

	if (semop(id, &op_close[0],3) < 0)
		logout(19,F_sem_create,"can not close.\n");

	if ((semval = semctl(id,1,GETVAL,0)) < 0)
		logout(19,F_sem_create,"semctl GETVAL error\n");

	if (semval > BIGCOUNT)
		logout(19,F_sem_create,"sem[1] > BIGCOUNT.\n");
	else if (semval == BIGCOUNT)
		sem_rm(id);
	else 
		if (semop(id, &op_unlock[0],1) <0)
			logout(19,F_sem_create,"can not unlock.\n");
	
}

void
sem_op(int id,int value)
{
	if ((op_op[0].sem_op = value) == 0)
		logout(19,F_sem_op,"value = 0.\n");

	if (semop(id, &op_op[0],1) < 0)
		logout(19,F_sem_op,"can not op.\n");
}

void
sem_wait(int id)
{
	sem_op(id, -1);
}

void
sem_signal(int id)
{
	sem_op(id,1);
}

void cnvmsg(char *text,int size)
{
	/* format expected:   "[date-time...] [proc:...] [pid,...] [src,line...] DTC: [...]: [....]" */
	int textlen = strlen(text);
	char    *p0,*p1;
	char    datetime[64];
	char    hostname[40];     /* Was 20, causing problems for longer hostnames (pc070817) */
	char    proc[256];
	char    *msg, *msgcopy;
	int     msgcopylen,n;
#if defined(HPUX) && SYSVERS == 1020
	int		msglen;
	char	*tmptxt;
#endif

	p0 = strchr(text,'[');
	if ( p0 == NULL )   return; /*not the expected format*/
	p1 = strchr(p0,']');
	if ( p1 == NULL )   return; /*not the expected format*/
	strncpy(datetime, p0, p1 - p0 + 1 );
	datetime[ p1 - p0 + 1 ] = 0;


	p0 = strstr(p1,"[proc:");
	if ( p0 == NULL )   return; /*not the expected format*/
	p1 = strchr(p0,']');
	if ( p1 == NULL )   return; /*not the expected format*/
	strncpy(proc, p0+6, p1 - (p0+6) );
	proc[ p1 - (p0+6) ] = 0;


	p0 = strstr(p1, CAPQ":");
	if ( p0 == NULL )   return; /*not the expected format*/
	msg = p0 + 4;

	msgcopylen = textlen-(msg-text);
	msgcopy = (char*)malloc( msgcopylen + 1);
	if ( msgcopy == NULL )  return; 
	strcpy( msgcopy, msg );

	memset(hostname,0x00,sizeof(hostname));
	gethostname(hostname,sizeof(hostname));

	/* rebuild text */
#if defined(HPUX) && SYSVERS == 1020
	msglen = strlen(datetime) + strlen(hostname) + strlen(proc) + msgcopylen + 12;
	tmptxt = (char*)malloc(msglen);
	sprintf(tmptxt, "%s "PRODUCTNAME": [%s:%s] %s",datetime, hostname, proc, msgcopy);
	strncpy(text, tmptxt, size);
	text[size - 1] = '\0';
	free(tmptxt);
#else
	snprintf(text, size, "%s "PRODUCTNAME": [%s:%s] %s",datetime, hostname, proc, msgcopy);
#endif

	free(msgcopy);
}

char *parmcpy(char *buf,char *copyarea,int arealen,char *start,char *end)
{
	int        copylen;
	int        pare = 0;
	/* 1.    PARAMETA CHECK             */
	memset(copyarea,0x00,arealen);
	for(copylen = 0;copylen <= arealen;copylen++) {
		if (*buf == *start) {
			pare = pare + 1;
			if (*start == 0x20)
			{
				buf = buf +1;
				continue;
			}
		}
		if (*buf == *end) {
			pare = pare -1;
			if (pare == 0) {
				if (copylen == 0) {
					goto proc_end;
				}
				else
				{
					*copyarea = *buf;
					buf = buf + 1;
				}
			}
			break;
		}
		else if (*buf == '\0') {
			goto proc_end;
		}
		*copyarea = *buf;
		copyarea = copyarea + 1;
		buf = buf + 1;
	}
	if (copylen > arealen) {
	}
proc_end:
	buf = buf +1;
	return(buf);
}

void atollx(char convtext[],unsigned long long *bin)
{
	unsigned long long      rc = 0;
	int                     i = 0;


	for(i = 0 ; isdigit(convtext[i]) != 0 ; i++)
	{
		rc = rc * 10;
		switch(convtext[i])
		{
		case '1' :
			rc = rc + 1;
			break;
		case '2' :
			rc = rc + 2;
			break;
		case '3' :
			rc = rc + 3;
			break;
		case '4' :
			rc = rc + 4;
			break;
		case '5' :
			rc = rc + 5;
			break;
		case '6' :
			rc = rc + 6;
			break;
		case '7' :
			rc = rc + 7;
			break;
		case '8' :
			rc = rc + 8;
			break;
		case '9' :
			rc = rc + 9;
			break;
		}
	}
	*bin = rc;
	return;
}


void lltoax(char text[],unsigned long long convbin)
{
	int                     i = 0;
	int                     j = 0;
	char                    tmpbuf[512+1];
	char                    tmpint[4+1];

	memset(tmpbuf,0x00,sizeof(tmpbuf));
	/* Zero check */
	if (convbin == 0)
	{
		sprintf(text,"0");
	}
	else
	{
		for(i = 0 ; convbin != 0 ; i++)
		{
			j = convbin%10;
		 	convbin = convbin /10;
			sprintf(tmpint,"%d",j);
			tmpbuf[i] = tmpint[0];
		}
		j = 0;
		text[i] = 0x00;
		i--;
		for (;i >= 0; i--)
		{
			text[j] = tmpbuf[i];
			j++;
		}
	}
	return;
}

void execCommand(char* cmd, char* result)
{
	int len;
	char data[256+1];
	FILE *fp;

	memset(data, '\0', sizeof(data));
	if( (fp = popen( cmd, "r" )) == NULL )
		return;
	fgets(data, sizeof(data), fp);
	if( (len = strlen(data)) <= 0 ){
		pclose( fp );
		return;
	}
	if( data[len - 1] == '\n' )
		data[len - 1] = '\0';
	if( strlen(data) <= 0 ){
		pclose( fp );
		return;
	}
	strcpy( result, data );
	pclose( fp );
	return;
}

void getversion(char *vbuf, int vbuflen)
{
	char	cmdline[256];
	char    buf[256];
	char    version[32];
	char    build[32];
	FILE	*f;
	char    *bp = &buf[0];
	char    *s;

	/* Make sure there is room for version, space, build and null */
	if (sizeof version + sizeof build > vbuflen) {
	    logout(4, F_getversion, "invalid version string length\n");
	}

	memset( buf, '\0', sizeof buf);
	version[0] = version[sizeof version - 1] = '\0';
	build[0] = build[sizeof build - 1] = '\0';

	sprintf(cmdline, "%s/"QNM"info -v 2>/dev/null", DTCCMDDIR);

	/*
	 * Parse through the version string looking for version and build
	 */

	if ((f = popen(cmdline, "r")) != NULL) {
	    if (fgets(buf, sizeof buf, f) == NULL) {
		    logout(4, F_getversion, "cannot get version info.\n");
		    *bp='\0';
	    }
	    pclose(f);
	    while((s = strtok( bp, " \t" )) != NULL) {
		if (strcasecmp(s, "Version") == 0) {
		    s = strtok( bp, " \t" );
		    if (s) strncpy( version, s, sizeof version - 1);
		}
		else if (strcasecmp(s, "Build") == 0) {
		    s = strtok( bp, " \t" );
		    if (s) strncpy( build, s, sizeof build - 1);
		}
		bp = NULL;
	    }
	} else {
		logoutx(4, F_getversion, "popen failed", "errno", errno);
	}
	if (version[0] == '\0')
	    strcpy( version, "unknown" );
	if (build[0] == '\0')
	    strcpy( build, "unknown" );
	sprintf(vbuf, "%s %s", version, build);
	return;
}

int getversion_num()
{
	static int ver_n = 0;
	char	version[64];
	char	ver[32], build[32];
	char	*s;
	int		v1=0, v2=0, v3=0;
	int		b=0;

	if (ver_n != 0) {
		return ver_n;
	}
	memset(version, 0, sizeof(version));
	getversion(version, sizeof version);
	if (sscanf(version, "%s %s", ver, build) > 0) {
		if ((s = (char *)strtok(ver, ".\n")))
			v1 = (int)strtol(s, NULL, 0);
		if ((s = (char *)strtok(NULL, ".\n")))
			v2 = (int)strtol(s, NULL, 0);
		if ((s = (char *)strtok(NULL, ".\n")))
			v3 = (int)strtol(s, NULL, 0);
		b = (int)strtol(build, NULL, 10);

		ver_n = (v1 * 10000000) + (v2 * 100000) + (v3 * 1000) + b;
		return ver_n;
	} else {
		return 0;
	}
}

int check_cfg_filename(struct dirent *dent, int type)
{
	int i;
	char lgn[4];

	if (strlen(dent->d_name) != 8) return -1;
	switch (type) {
		case PRIMARY_CFG:
			if (dent->d_name[0] != 'p') return -1;
			break;
		case SECONDARY_CFG:
			if (dent->d_name[0] != 's') return -1;
			break;
		case ALL_CFG:
			if ((dent->d_name[0] != 'p') && (dent->d_name[0] != 's')) return -1;
			break;
	}
	for (i = 1; i <= 3; i++) {
		if (isdigit(dent->d_name[i]) == FALSE) {
			return -1;
		}
		lgn[i-1] = dent->d_name[i];
	}
	lgn[3] = '\0';
	if (strcmp(&(dent->d_name[i]),".cfg") != 0) return -1;
	/* filename check OK */
	return atoi(lgn);
}

int survey_cfg_file(char *location, int cfg_file_type)
{
	struct dirent *dent;
	DIR *dfd;
	int r = 0;

	if ((dfd = opendir(location)) == NULL) {
		return 0;
	}

	while ((dent = readdir(dfd)) != NULL) {
		if (cfg_file_type == AGENT_CFG) {
			if (strcmp(dent->d_name, QNM"Agent.cfg") != 0) continue;
			r = 1;
			break;
		} else {
			if (check_cfg_filename(dent, cfg_file_type) == -1) continue;
			r = 1;
			break;
		}
	}
	closedir (dfd);
	return r;
}

int yesorno(char *message)
{
	char	in;
	int		ans = -1;

	while (1) {
		printf("%s [y/n] : ", message);
		in = getchar();
		if (in == '\n') continue;
		if (getchar() != '\n') {
			while(getchar() != '\n');
			continue;
		}
		switch (in) {
			case 'y':
			case 'Y':
				ans = 1;
				break;
			case 'n':
			case 'N':
				ans = 0;
				break;
			default :
				break;
		}
		if (ans == 1 || ans == 0) return ans;
	}
}

