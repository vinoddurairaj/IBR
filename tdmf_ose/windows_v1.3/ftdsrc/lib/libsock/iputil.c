/*
 * iputil.c - 
 * 
 * Copyright (c) 2000 Legato Systems, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 *
 * utilities for determining all IP addresses associated with
 * the current system that the program is running on.  This is
 * used for determining identity in with regards to
 * configuration files and not use hostids.
 *            
 * This module was borrowed from in.named and contains a
 * Berkeley style copyright notice.
 *
 * The external function prototypes live in config.h
 *
 * int getnetconfcount(); returns the number of IP addresses
 *                        associated with the current system
 *
 * void getnetconfs(u_long* ipaddrs);
 *                        fills in an array of unsigned longs
 *                        with the IP addresses of the current
 *                        system (malloc with getnetconfcount)
 */

#include <winsock2.h>
#include <errno.h>

#include <stdio.h>
#include <windows.h>

#include <assert.h>

#include <Iprtrmib.h>
#include <Iphlpapi.h>

#include "iputil.h"
#include "ftd_cfgsys.h"
#include "ftd_error.h"



char dbgbuf[100];
static unsigned long ulMACAddrCache = 0;
static unsigned long ulSelectedTCPIP = 0;

#define num1(x) ((x>>24)&255)
#define num2(x) ((x>>16)&255)
#define num3(x) ((x>>8)&255)
#define num4(x) (x&255)

/*
 * name_is_ipstring -- determine if the hostname is an ip address string
 */
int
name_is_ipstring(char *name)
{
    int i;

    int len = strlen(name);

    for (i=0;i<len;i++) {
        if (isalpha(name[i])) {
            return 0;
        }
    }

    return 1; 
}
/*
 * ip_to_name -- convert ip address to hostname
 */
int
ip_to_name(unsigned long ip, char *name)
{
    struct hostent  *host;
    unsigned long   addr;
    char            ipstring[32], **p;

    ip_to_ipstring(ip, ipstring);

    addr = inet_addr(ipstring);
    host = gethostbyaddr((const char *)&addr, sizeof(addr), AF_INET);

    if (host == NULL) {
        return -1;
    }
    p = host->h_addr_list;
    strcpy(name, host->h_name);

    return 0; 
}

int name_to_ip(char *name, unsigned long *ip)
{
    struct hostent *host;
    struct in_addr in;
    char **p;

    if (name == NULL || strlen(name) == 0) {
        return -1;
    } 
    
    host = gethostbyname(name);
    
    if (host == NULL) {
        return -1;
    }
    
    p = host->h_addr_list;

    memcpy(&in.s_addr, *p, sizeof(in.s_addr));
    *ip = in.s_addr; 

    return 0; 
}
/*
 * ipstring_to_ip -- convert ip address string to ip address
 */
int
ipstring_to_ip(char *name, unsigned long *ip)
{
    int n1, n2, n3, n4;
    char    *s, *lname;


    *ip =0;

    if (name == NULL || strlen(name) == 0) 
        {
        return -1;
        } 

    lname = strdup(name);
        
    if ((s = strtok(lname, ".\n")))
        n1 = atoi(s);
    if ((s = strtok(NULL, ".\n")))
        n2 = atoi(s);
    if ((s = strtok(NULL, ".\n")))
        n3 = atoi(s);
    if ((s = strtok(NULL, ".\n")))
        n4 = atoi(s);

    free(lname);

    //sg
    // Add a check to see if all the individual numbers
    // are valid! All ip individual numbers must be 0<=x<=255
    //
    if (( 0 > n1 ) || ( n1 > 255 ) || 
        ( 0 > n2 ) || ( n2 > 255 ) || 
        ( 0 > n3 ) || ( n3 > 255 ) || 
        ( 0 > n4 ) || ( n4 > 255 )    )
    {
        return -1;
    }

    *ip = n1 + (n2 << 8) + (n3 << 16) + (n4 << 24); 
    

    return 0; 
}

/*
 * ip_to_ipstring -- convert ip address to ipstring in dot notation
 *
 *                   AC, 2002-04-10 : bug correction:
 *                   Make sure ip value is parsed as a network byte order 
 *                   and not Intel (little endian) byte order
 */
int
ip_to_ipstring(unsigned long ip, char *ipstring)
{
    int a1, a2, a3, a4;

    a1 = 0x000000ff &  ip;
    a2 = 0x000000ff & (ip >> 8);
    a3 = 0x000000ff & (ip >> 16);
    a4 = 0x000000ff & (ip >> 24);
    
    sprintf(ipstring, "%d.%d.%d.%d", a1, a2, a3, a4);

    return 0; 
}




int GetAdapterIpAddress(char *szAdapter, char *szIpAddress)
{
    char tcpip[1024];
    HKEY hKey;
    int  ret = 0;

    strcpy(tcpip, "System\\CurrentControlSet\\Services\\");
    strcat(tcpip, szAdapter);
    strcat(tcpip, "\\Parameters\\Tcpip");

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, tcpip, 0L, KEY_READ, &hKey) == ERROR_SUCCESS) 
    {
        //DHCP or static address ?
        char    pszIpAddr[24],*pszValueName;
        LONG    size = sizeof(pszIpAddr);
        DWORD   dwType,dwDHCP;

        dwType  = REG_DWORD;
        size    = sizeof(dwDHCP);
        if ( RegQueryValueEx(hKey,
                        "EnableDHCP",
                        NULL,
                        &dwType,
                        (BYTE*)&dwDHCP,
                        &size) != ERROR_SUCCESS)
        {
            dwDHCP = 0;//"EnableDHCP" value not found.
        }

        if ( dwDHCP )
            pszValueName = "DhcpIPAddress";
        else
            pszValueName = "IPAddress";

        dwType  = REG_SZ;
        size    = sizeof(pszIpAddr);
        if ( RegQueryValueEx(hKey,
                        pszValueName,
                        NULL,
                        &dwType,
                        pszIpAddr,
                        &size) == ERROR_SUCCESS)
        {
            strcpy(szIpAddress, pszIpAddr);
#ifdef _DEBUG
            OutputDebugString("GetAdapterIpAddress() returns ip <");
            OutputDebugString(szIpAddress);
            OutputDebugString(">\n");
#endif
            ret = 1;
        }

        RegCloseKey(hKey);
    }

    return ret;//0 = error, 1 = success
}

//
// GetNicList
// fill NIC/IP list and return to client
//
void GetNicList(char szNicList[99][256])
{
    HKEY            hKey, hKeyCard;
    DWORD           iValue, iList = 0;
    LONG            Status;
    FILETIME        ftLastWrite;
    DWORD           NameLen;
    char            NameBuf[MAX_PATH];
    char            szKey[] = "Software\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards";

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey,
                          0L, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        char    lpszClass[MAX_PATH];    // address of buffer for class string 
        DWORD   lpcchClass = sizeof(lpszClass); // address of size of class string buffer 
        DWORD   lpcSubKeys; // address of buffer for number of subkeys 
        DWORD   lpcchMaxSubkey; // address of buffer for longest subkey name length  
        DWORD   lpcchMaxClass;  // address of buffer for longest class string length 
        DWORD   lpcValues;  // address of buffer for number of value entries 
        DWORD   lpcchMaxValueName;  // address of buffer for longest value name length 
        DWORD   lpcbMaxValueData;   // address of buffer for longest value data length 
        DWORD   lpcbSecurityDescriptor; // address of buffer for security descriptor length 
        FILETIME  lpftLastWriteTime;    // address of buffer for last write time 
    
        Status = RegQueryInfoKey (
            hKey,   // handle of key to query 
            lpszClass,  // address of buffer for class string 
            &lpcchClass,    // address of size of class string buffer 
            NULL,   // reserved 
            &lpcSubKeys,    // address of buffer for number of subkeys 
            &lpcchMaxSubkey,    // address of buffer for longest subkey name length  
            &lpcchMaxClass, // address of buffer for longest class string length 
            &lpcValues, // address of buffer for number of value entries 
            &lpcchMaxValueName, // address of buffer for longest value name length 
            &lpcbMaxValueData,  // address of buffer for longest value data length 
            &lpcbSecurityDescriptor,    // address of buffer for security descriptor length 
            &lpftLastWriteTime  // address of buffer for last write time 
           );   

        for (iValue = 0; Status == ERROR_SUCCESS; iValue++)
        {
            NameLen = sizeof(NameBuf);
            if ((Status = RegEnumKeyEx(
                hKey,   // handle of key to enumerate 
                iValue, // index of subkey to enumerate 
                NameBuf,    // address of buffer for subkey name 
                &NameLen,   // address for size of subkey buffer 
                NULL,   // reserved 
                NULL,   // address of buffer for class string 
                NULL,   // address for size of class buffer 
                &ftLastWrite    // address for time key last written to 
               )) == ERROR_SUCCESS)
            {
                if ((Status = RegOpenKeyEx(hKey, NameBuf,
                                      0L, KEY_READ, &hKeyCard)) == ERROR_SUCCESS)
                {
                    char pszSrvName[256];
                    LONG size = sizeof(pszSrvName);
                    DWORD dwType;

                    if ((Status = RegQueryValueEx(hKeyCard, // handle of key to query 
                        "ServiceName",              // address of name of value to query 
                        NULL,
                        &dwType,
                        pszSrvName,       // pointer to put buffer in
                        &size)) == ERROR_SUCCESS)
                    {
                        strcpy(szNicList[iList++], pszSrvName);
                    } /* if RegQueryValue */
                    RegCloseKey(hKeyCard);
                } /* if RegOpenKeyEx */
            } /* if RegEnumKeyEx */
        } /* for iValue */

        RegCloseKey(hKey);
    } /* if RegOpenKeyEx */

    szNicList[iList][0] = '\0';
}

int
getnetconfcount(void)
{
    HKEY            hKey;
    DWORD           iValue, iCount = 0;
    LONG            Status;
    FILETIME        ftLastWrite;
    DWORD           NameLen;
    char            NameBuf[MAX_PATH];
    char            szKey[] = "Software\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards";

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey,
                          0L, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        char    lpszClass[MAX_PATH];    // address of buffer for class string 
        DWORD   lpcchClass = sizeof(lpszClass); // address of size of class string buffer 
        DWORD   lpcSubKeys; // address of buffer for number of subkeys 
        DWORD   lpcchMaxSubkey; // address of buffer for longest subkey name length  
        DWORD   lpcchMaxClass;  // address of buffer for longest class string length 
        DWORD   lpcValues;  // address of buffer for number of value entries 
        DWORD   lpcchMaxValueName;  // address of buffer for longest value name length 
        DWORD   lpcbMaxValueData;   // address of buffer for longest value data length 
        DWORD   lpcbSecurityDescriptor; // address of buffer for security descriptor length 
        FILETIME  lpftLastWriteTime;    // address of buffer for last write time 

        Status = RegQueryInfoKey (
            hKey,   // handle of key to query 
            lpszClass,  // address of buffer for class string 
            &lpcchClass,    // address of size of class string buffer 
            NULL,   // reserved 
            &lpcSubKeys,    // address of buffer for number of subkeys 
            &lpcchMaxSubkey,    // address of buffer for longest subkey name length  
            &lpcchMaxClass, // address of buffer for longest class string length 
            &lpcValues, // address of buffer for number of value entries 
            &lpcchMaxValueName, // address of buffer for longest value name length 
            &lpcbMaxValueData,  // address of buffer for longest value data length 
            &lpcbSecurityDescriptor,    // address of buffer for security descriptor length 
            &lpftLastWriteTime  // address of buffer for last write time 
           );   

        for (iValue = 0; Status == ERROR_SUCCESS; iValue++)
        {
            NameLen = sizeof(NameBuf);

            if ((Status = RegEnumKeyEx(
                hKey,   // handle of key to enumerate 
                iValue, // index of subkey to enumerate 
                NameBuf,    // address of buffer for subkey name 
                &NameLen,   // address for size of subkey buffer 
                NULL,   // reserved 
                NULL,   // address of buffer for class string 
                NULL,   // address for size of class buffer 
                &ftLastWrite    // address for time key last written to 
               )) == ERROR_SUCCESS)
            {
                iCount++;               
            } /* if RegEnumKeyEx */
        } /* for iValue */

        RegCloseKey(hKey);
    } /* if RegOpenKeyEx */

    return iCount;
}

void
getnetconfs(u_long* iplist)
{
    char szNicList[99][256];
    char szIpAddress[256];
    int i = 0, count = 0;

    GetNicList(szNicList);

    while (szNicList[i][0]) {
        if (GetAdapterIpAddress(szNicList[i], szIpAddress)) {
            iplist[count++] = inet_addr(szIpAddress);
        }

        i++;
    }
}


//////////////////////////////////////////////////////////////
// pCur must be a zero-terminated string.
char * util_find_ip_addr(char* pstr)
{


    //printf("\n debug: parse_to_find_ip_addr IN = %s ",pstr);
    // ' ipconfig /all '  outputs text information , 
    // in which "IP Address" can be found, as many times as the host as IP Addresses.
    //
    // looking for :  "IP Address.....................: aaa.bbb.c.dd "
    //
    pstr = strstr( pstr, "IP Address" );
    if ( pstr == NULL )
        return NULL;

    pstr = strchr( pstr, ':' );
    if ( pstr == NULL )
        return NULL;

    while( !isdigit(*pstr) )
        pstr++;

#ifdef _DEBUG
    if ( pstr )
        printf("\ndebug: parse_to_find_ip_addr returns %.15s ",pstr);
    else
        printf("\ndebug: parse_to_find_ip_addr returns NULL.");
#endif
    

    //ptr to text IP address.
    return pstr;
}

/////////////////////////////////////////////////////////
char * util_find_mac_addr(char* pstr) //pstr must be a zero-terminated string.
{

    //
    pstr = strstr( pstr, "Physical Address" );
    if ( pstr == NULL )
        return NULL;

    pstr = strchr( pstr, ':' );
    if ( pstr == NULL )
        return NULL;

    while( !isdigit(*pstr) )
        pstr++;

#ifdef _DEBUG
    if ( pstr )
        printf("\ndebug: parse_to_find_ip_addr returns %.15s ",pstr);
    else
        printf("\ndebug: parse_to_find_ip_addr returns NULL.");
#endif
    error_tracef( TRACEINF6,">>parse_to_find_mac_addr => %.20s",pstr ); 

    //ptr to text IP address.
    return pstr;
}


///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
unsigned long Get_Selected_DtcIP_from_Install(void)
{
    char   tmp [80];


    if (ulSelectedTCPIP == 0)
        {
        if ( cfg_get_software_key_value("DtcIP", tmp, CFG_IS_STRINGVAL) == CFG_OK )
            {
            ipstring_to_ip(tmp,(unsigned long*)&ulSelectedTCPIP);
            
            }
        else 
            {
            error_tracef( TRACEERR, "Get_Selected_DtcIP_from_Install: registry DtcIP is not found. Reinstall.");
            return (0);
            }
        }
    

    return (ulSelectedTCPIP);
}


//
// Function returns Mac Address from Ip address
// will return 0 if unable to find one
//
ULONG GetMacAddrFromIPAddr(ULONG ulIPAddress)
{
    ULONG               ulMac = 0;
    DWORD               dwSize = 0;
    MIB_IPADDRTABLE*    pIpAddrTable = NULL;
    unsigned int        i;

    GetIpAddrTable(pIpAddrTable, &dwSize, FALSE); // Get table size


    pIpAddrTable = (MIB_IPADDRTABLE*)malloc(dwSize);

    if (GetIpAddrTable(pIpAddrTable, &dwSize, FALSE) == NO_ERROR)
    {
        for (i = 0; i < pIpAddrTable->dwNumEntries; i++)
{
            // Check if IP Address is a valid one
            if (        (pIpAddrTable->table[i].dwAddr != 0) 
                    &&  (pIpAddrTable->table[i].dwAddr != 16777343)) // 127.0.0.1

            {
                // Compare IP Address with the searched one
                if (pIpAddrTable->table[i].dwAddr == ulIPAddress)
                {
                    // Get Adapter info
                    MIB_IFROW IFRow;
                    IFRow.dwIndex = pIpAddrTable->table[i].dwIndex;

                    if (GetIfEntry(&IFRow) == NO_ERROR)
                    {
                        ulMac = (IFRow.bPhysAddr[2] << 24) + 
                                (IFRow.bPhysAddr[3] << 16) +
                                (IFRow.bPhysAddr[4] << 8)  + 
                                IFRow.bPhysAddr[5];

                    }
                    break;
                }
            }
        }
    }

    free((char*)pIpAddrTable);
    return ulMac;
}

//
// Function returns Ip Address from Mac address
// will return 0 if unable to find one
//
ULONG GetIPAddrFromMacAddr(ULONG ulMacAddress)
{
    ULONG               ulIP = 0;
    DWORD               dwSize = 0;
    MIB_IPADDRTABLE*    pIpAddrTable = NULL;
    unsigned int        i;

    GetIpAddrTable(pIpAddrTable, &dwSize, FALSE); // Get table size
    
    pIpAddrTable = (MIB_IPADDRTABLE*) malloc(dwSize);//new BYTE[dwSize];

    if (GetIpAddrTable(pIpAddrTable, &dwSize, FALSE) == NO_ERROR)
    {
        for (i = 0; i < pIpAddrTable->dwNumEntries; i++)
        {
            // Check if IP Address is a valid one
            if (    (pIpAddrTable->table[i].dwAddr != 0) 
                &&  (pIpAddrTable->table[i].dwAddr != 16777343)) // 127.0.0.1
            {
                ULONG       ulMac = 0;
                // Get Adapter info
                MIB_IFROW   IFRow;

                IFRow.dwIndex = pIpAddrTable->table[i].dwIndex;

                if (GetIfEntry(&IFRow) == NO_ERROR)
				{
                    ulMac = (IFRow.bPhysAddr[2] << 24) + 
                            (IFRow.bPhysAddr[3] << 16) +
                            (IFRow.bPhysAddr[4] << 8)  + 
                            IFRow.bPhysAddr[5];
                }

                if (ulMac == ulMacAddress)
        {                   
                    ulIP = pIpAddrTable->table[i].dwAddr;
                    break;
                }

            }
        }
    }

    free((char*)pIpAddrTable);
    
    return ulIP;
}

unsigned int GetAllIpAddresses(unsigned long * pIpList, unsigned long MaxSize)
    {       
    ULONG               ulMac = 0;
    DWORD               dwSize = 0;
    MIB_IPADDRTABLE*    pIpAddrTable = NULL;
    unsigned int        i;
    unsigned int        NumAddresses = 0;

    GetIpAddrTable(pIpAddrTable, &dwSize, FALSE); // Get table size
    
    pIpAddrTable = (MIB_IPADDRTABLE*) malloc(dwSize);//new BYTE[dwSize];

    if (GetIpAddrTable(pIpAddrTable, &dwSize, FALSE) == NO_ERROR)
            {   
        for (i = 0; i < pIpAddrTable->dwNumEntries; i++)
                {
            // Check if IP Address is a valid one
            if (    (pIpAddrTable->table[i].dwAddr != 0) 
                &&  (pIpAddrTable->table[i].dwAddr != 16777343)) // 127.0.0.1
                    {
                if (NumAddresses < MaxSize)
                {
                    pIpList[NumAddresses] = pIpAddrTable->table[i].dwAddr;
                    NumAddresses++;
                }
            }
        }
    }
    free((char*)pIpAddrTable);

    return NumAddresses;
}

//
// IsHostIdValid(Value)
//
// This checks to see if the hostid returned is a valid one.
// i.e. is there any IP address corresponding to the value
//
//
//
unsigned long IsHostIdValid(unsigned long ulHostId)
{
    return (GetIPAddrFromMacAddr(ulHostId));
}

/////////////////////////////////////////////////////////
// GetHostId()
//
// There is one open issue with this code:
// 
// It assumes that the IP address that was used at 
// install is still valid when calling this gethostid 
// call. Since the time between the install choice of the 
// ip, and the start of the service is relativly short, 
// the hole is not that big
//
unsigned long GetHostId(void)
{
    char            *pstr       = NULL;
    unsigned long   ulTempMac   = 0;
    char            MacStr[20];
    HANDLE          hMutex;

    //
    // This function can be called by multiple threads, 
    // simultaneously. This is the reason for this Mutex.
    //
    hMutex = CreateMutex(0,0,"GetHostID_Mutex");
    if ( hMutex != 0 )
    {
        WaitForSingleObject(hMutex,INFINITE);
    }
    else
    {
        assert(0);
    }
             
    //
    // We cache the Mac Address 
    // Once we've read it, we don't read it again.
    //
    if ( ulMACAddrCache == 0 )
    {       
        //
        // If there is a Mac Address stored in the registry use it
        //
        if ( cfg_get_software_key_value("HostID", MacStr, CFG_IS_STRINGVAL) == CFG_OK )  
        {
            //
            // We read a hostid, verify to see if 
            //
            // this adapter is still present
            //
            // (we require the adapter to be present for license management)
            //
            sscanf (MacStr, "%08lx", &ulTempMac);       
            if (ulTempMac)
            {
                //
                // Get the IP value based on the MAC address 
                // This allows us to change the value that was
                // previously used for IP to the actual one
                // (in case of DHCP)
                //
                ulSelectedTCPIP = GetIPAddrFromMacAddr(ulTempMac);
                if (!ulSelectedTCPIP)
                {
                    //
                    // We could not find a valid IP for this hostid,
                    // which means that the card which has this hostid 
                    // is missing or disabled
                    //
                    error_tracef( TRACEERR, "GetHostId: No valid IP address was found for this HostID:0x%08x\nPlease make sure that this card is not disabled or missing in the system.",ulTempMac);                
                }
            }
        }
        else
        { 
            //
            // We don't have a MAC address, so deduce it from the IP address
            //

            //
            // Read in the selected IP from install
            //
            if (ulSelectedTCPIP == 0)
            {
                Get_Selected_DtcIP_from_Install();

                //
                // Must have a selected IP or we are OUT OF HERE!
                //
                if (ulSelectedTCPIP == 0 )
                {
                    error_tracef( TRACEERR, "GetHostId: No IP address is available or IP=0 for this replication agent\nCheck registry entries or re-install product.");                

                    if ( hMutex != 0 )
                    {
                        ReleaseMutex(hMutex);
                        if (!CloseHandle(hMutex)) 
                        {
                            assert(0);
                        }
                    }            
                    return (0);
                }

            // we should only pass trough here once, as we will store this 
            // value in the registry anyhow.
            //
            //ulTempMac = GetMacAddrFromIPAddr(ulSelectedTCPIP);
            sprintf(MacStr,"%12lx",ulTempMac);

            if (ulTempMac)
            {
                //
                // Store found MAC address into the registry
                //
                cfg_set_software_key_value("HostID", &MacStr[4], CFG_IS_STRINGVAL);
                error_tracef( TRACEWRN, "HostId is valid %s", &MacStr[4]);             
            }
            else
            {
                error_tracef(   TRACEERR, 
                                "GetHostId: No valid MAC address was found for this IP address:%ld.%ld.%ld.%ld\nPlease make sure that this card is not disabled or missing in the system.",
                                num4(ulSelectedTCPIP),
                                num3(ulSelectedTCPIP),
                                num2(ulSelectedTCPIP),
                                num1(ulSelectedTCPIP));                
            }
        }

        if (        (ulTempMac == 0) 
                ||  (ulSelectedTCPIP == 0)  )
        {
            error_tracef( TRACEERR, "GetHostId: Hostid (MAC address) is not found or invalid.");                
        }
		}
    }

    if (!ulMACAddrCache)
        ulMACAddrCache = ulTempMac;

    if ( hMutex != 0 )
    {
        ReleaseMutex(hMutex);
        if (!CloseHandle(hMutex)) 
        {
            assert(0);
        }
    }

    return (ulMACAddrCache);
}

///////////////////////////////////////////////////////////////////////////

int GetMacAddress(char *pstr,char *pMac)
{
    double  n;
    char    *s, *lname;
    int     i,len;
    

    len =0;

    if (pstr == NULL || strlen(pstr) == 0) 
        return 0;

    lname = strdup(pstr);

    

    for (i=0;i<6;i++)
    {
        if (i==0)
           s = strtok(lname, "-\n");
        else
            s = strtok(NULL ,"-\n");

        n = atof(s);
        if (s && (n >= 0x00 && n <= 0xFF))
        {
            memcpy(&(pMac[len]),s,2);
            len += 2;
        }
    }

    

    free(lname);

    return (len); 
}                                                                                                           

#ifdef NOT_TO_COMPILE
#pragma comment(lib,"Netapi32.lib")
typedef struct _ASTAT_
{
  ADAPTER_STATUS adapt;
  NAME_BUFFER    NameBuff [30];

}ASTAT, * PASTAT;

unsigned long getMACAddr()
{
    ASTAT Adapter;
    NCB Ncb;
    UCHAR uRetCode;
    //char NetName[50];
    LANA_ENUM   lenum;
    int      i;

    memset( &Ncb, 0, sizeof(Ncb) );
    Ncb.ncb_command = NCBENUM;
    Ncb.ncb_buffer = (UCHAR *)&lenum;
    Ncb.ncb_length = sizeof(lenum);
    uRetCode = Netbios( &Ncb );
    //printf( "The NCBENUM return code is: 0x%x \n", uRetCode );

    for(i=0; i < lenum.length ;i++)
    {
        memset( &Ncb, 0, sizeof(Ncb) );
        Ncb.ncb_command = NCBRESET;
        Ncb.ncb_lana_num = lenum.lana[i];

        uRetCode = Netbios( &Ncb );
        //printf( "The NCBRESET on LANA %d return code is: 0x%x \n",
        //        lenum.lana[i], uRetCode );

        memset( &Ncb, 0, sizeof (Ncb) );
        Ncb.ncb_command = NCBASTAT;
        Ncb.ncb_lana_num = lenum.lana[i];

        strcpy( (char*)(Ncb.ncb_callname),  "*               " );
        Ncb.ncb_buffer = (unsigned char *) &Adapter;
        Ncb.ncb_length = sizeof(Adapter);

        uRetCode = Netbios( &Ncb );
        //printf( "The NCBASTAT on LANA %d return code is: 0x%x \n",
        //        lenum.lana[i], uRetCode );
        if ( uRetCode == 0 )
        {
            /*
            sprintf( szMACaddr, "%02x%02x%02x%02x%02x%02x",
              Adapter.adapt.adapter_address[0],
              Adapter.adapt.adapter_address[1],
              Adapter.adapt.adapter_address[2],
              Adapter.adapt.adapter_address[3],
              Adapter.adapt.adapter_address[4],
              Adapter.adapt.adapter_address[5] );
              */

            unsigned long ulHostId = 0;
            ulHostId += (((unsigned long)(Adapter.adapt.adapter_address[2])) << 24); 
            ulHostId += (((unsigned long)(Adapter.adapt.adapter_address[3])) << 16); 
            ulHostId += (((unsigned long)(Adapter.adapt.adapter_address[4])) << 8); 
            ulHostId += (((unsigned long)(Adapter.adapt.adapter_address[5])) << 0); 
            return ulHostId;
        }
    }

    return 0;
}
#endif

////////////////////////////////////////////////////////////////

BOOL util_consoleOutput_init(ConsoleOutputCtrl *pctrl)
{
    HANDLE hOutputReadTmp;
    SECURITY_ATTRIBUTES sa;

    // Set up the security attributes struct.
    sa.nLength= sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;


    pctrl->iSize = 1024;
    pctrl->pData = (char*)malloc( pctrl->iSize );
    pctrl->iTotalDataRead = 0;

    

    // Create the child process output pipe.
    if (!CreatePipe(&hOutputReadTmp,&pctrl->hOutputWrite,&sa,0))
    {
        //assert(0);
        return FALSE;
    }
    // Create a duplicate of the output write handle for the std error
    // write handle. This is necessary in case the child application
    // closes one of its std output handles.
    if (!DuplicateHandle(GetCurrentProcess(), pctrl->hOutputWrite,
                         GetCurrentProcess(),&pctrl->hErrorWrite,
                         0,TRUE,
                         DUPLICATE_SAME_ACCESS))
    {
        //assert(0);
        return FALSE;
    }

    // Create new output read handle and the input write handles. Set
    // the Properties to FALSE. Otherwise, the child inherits the
    // properties and, as a result, non-closeable handles to the pipes
    // are created.
    if (!DuplicateHandle(GetCurrentProcess(), hOutputReadTmp,
                         GetCurrentProcess(),&pctrl->hOutputRead, // Address of new handle.
                         0,FALSE, // Make it uninheritable.
                         DUPLICATE_SAME_ACCESS))
    {
        //assert(0);
        return FALSE;
    }

    // Close inheritable copies of the handles you do not want to be
    // inherited.
    if (!CloseHandle(hOutputReadTmp)) 
    {
        //assert(0);
        return FALSE;
    }

    

    return TRUE;
}

BOOL util_consoleOutput_read(ConsoleOutputCtrl *pctrl)
{
    DWORD read = 0;

    if ( pctrl->pData == 0 )
    {
        return FALSE;
    }
    // Close pipe handles (do not continue to modify the parent).
    // You need to make sure that no handles to the write end of the
    // output pipe are maintained in this process or else the pipe will
    // not close when the child process exits and the ReadFile will hang.
    if (!CloseHandle(pctrl->hOutputWrite)) 
    {
        //assert(0);
        //return FALSE;
    }
    if (!CloseHandle(pctrl->hErrorWrite)) 
    {
        //assert(0);
        //return FALSE;
    }

    read = 0;
    pctrl->iTotalDataRead = 0;
    while ( ReadFile(pctrl->hOutputRead, 
                     pctrl->pData + pctrl->iTotalDataRead, 
                     pctrl->iSize - pctrl->iTotalDataRead, 
                     &read,0) )
    {
        pctrl->iTotalDataRead += read;
        if ( pctrl->iTotalDataRead == pctrl->iSize )
        {   //enlarge buffer by 1KB and continue reading from child output stream Pipe
            char *pNewData = (char*)malloc( pctrl->iSize + 1024 );
            memmove(pNewData, pctrl->pData, pctrl->iSize);
            pctrl->iSize += 1024;
            free( pctrl->pData );
            pctrl->pData = pNewData;
        }
    }
    //No more data received from process.

    //because console output data is text-based, append '\0' at end of data received
    if ( pctrl->iTotalDataRead == pctrl->iSize )
    {   //enlarge buffer by 1 byte to ensure EOS
        char *pNewData = (char*)malloc( pctrl->iSize + 1 );
        memmove(pNewData, pctrl->pData, pctrl->iSize);
        pctrl->iSize += 1;
        free( pctrl->pData );
        pctrl->pData = pNewData;
    }
    pctrl->pData[ pctrl->iTotalDataRead ] = 0;//ensure end of text string
    pctrl->iTotalDataRead++;

    return TRUE;
}

void util_consoleOutput_delete(ConsoleOutputCtrl *pctrl)
{
    //
    // Added some protection against crashes...
    //
    if (!pctrl)
    {
        return; 
    }

    if (pctrl->hOutputRead)
    {
        CloseHandle(pctrl->hOutputRead);
        pctrl->hOutputRead = 0;
    }

    if (pctrl->pData)  
    {
        free( pctrl->pData );
        pctrl->pData = NULL;
    }

    pctrl->iSize = 0;
    pctrl->pData = 0;
    pctrl->iTotalDataRead = 0;


    

}



char *GetIpConfig_Response(ConsoleOutputCtrl *pctrl)
{
    int                     ret = 0;
    BOOL                    b;
    PROCESS_INFORMATION     pInfo;
    STARTUPINFO             sInfo;

    if (!util_consoleOutput_init(pctrl))
    {
        error_tracef( TRACEERR, "Error in GetIpConfig_Response-util_consoleOutput_init tid=0x%08x",GetCurrentThreadId() );
        return (NULL);
    }


    //launch process command 
    memset(&sInfo,0,sizeof(sInfo));
    sInfo.cb = sizeof(sInfo);

    sInfo.dwFlags    = STARTF_USESTDHANDLES;
    sInfo.hStdError  = pctrl->hErrorWrite;//GetStdHandle(STD_ERROR_HANDLE);
    sInfo.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
    sInfo.hStdOutput = pctrl->hOutputWrite;//GetStdHandle(STD_OUTPUT_HANDLE);

    b = CreateProcess(  NULL,       // name of executable module
            "ipconfig.exe /all ",  // command line string
            NULL,                 // SD
            NULL,                 // SD
            TRUE,                 // handle inheritance option
            CREATE_NO_WINDOW,     // creation flags
            NULL,                 // new environment block
            NULL,                 // current directory name
            &sInfo,               // startup information
            &pInfo                // process information
            );//ASSERT(b);

    if ( b ) 
    {
        //accumulate all console output into pData buffer
        //while waiting for cmd process to end ...
        b = util_consoleOutput_read(pctrl);


        CloseHandle(pInfo.hProcess);
        CloseHandle(pInfo.hThread);

        if (pctrl->hOutputRead)
        {
            CloseHandle(pctrl->hOutputRead);
            pctrl->hOutputRead = 0;
        }


        if (!b)     
        {
            error_tracef( TRACEERR, "Error in GetIpConfig_Response-util_consoleOutput_read tid=0x%08x",GetCurrentThreadId() );
            return (NULL);  
        }

        error_tracef( TRACEINF6, "ipconfig[%d] %.20s ",GetCurrentThreadId(),strlen(pctrl->pData));;

        return (pctrl->pData);  
        //pctrl->pData;//pctrl->pData is a zero-terminated string
    }
    else
    {
        error_tracef( TRACEERR, "Error in GetIpConfig_Response-CreateProcess tid=0x%08x",GetCurrentThreadId() );
        return (NULL);
    }

}

