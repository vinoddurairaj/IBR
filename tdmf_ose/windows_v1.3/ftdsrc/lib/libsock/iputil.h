/*
 * iputil.h - IP utilities
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

typedef struct __ConsoleOutputCtrl
{
    HANDLE          hOutputRead,    //provide this handle to child process
                    hOutputWrite,   //provide this handle to child process
                    hErrorWrite;    //provide this handle to child process
    char            *pData;
    unsigned int    iSize, 
                    iTotalDataRead;

} ConsoleOutputCtrl;

/* external prototypes */
extern int name_is_ipstring(char *name);
extern int getnetconfcount(void);
extern void getnetconfs(unsigned long *iplist);
extern int name_to_ip(char *name, unsigned long *ip);
extern int ipstring_to_ip(char *ipstring, unsigned long *ip);
extern int ip_to_name(unsigned long ip, char *name);
extern int ip_to_ipstring(unsigned long ip, char *ipstring);
extern unsigned long GetHostId(void);
extern char *GetIpConfig_Response(ConsoleOutputCtrl *pctrl)	;
extern void util_consoleOutput_delete(ConsoleOutputCtrl *ctrl);
extern BOOL util_consoleOutput_read(ConsoleOutputCtrl *pctrl);
extern BOOL util_consoleOutput_init(ConsoleOutputCtrl *pctrl);
extern int GetMacAddress(char *pstr,char *pMac);
extern unsigned long Get_Selected_DtcIP_from_Install(void)		;
extern char * util_find_ip_addr(char* pstr);
extern char * util_find_mac_addr(char* pstr);
extern unsigned int GetAllIpAddresses(unsigned long * pIpList, unsigned long MaxSize);
unsigned long IsHostIdValid(unsigned long ulHostId);




