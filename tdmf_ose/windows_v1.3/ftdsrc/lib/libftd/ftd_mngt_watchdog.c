/*
 * ftd_mngt_watchdog.cpp - ftd thread watchdog
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
#if defined(_WINDOWS) && defined(_DEBUG)
#include <crtdbg.h>
#define ASSERT(exp)     _ASSERT(exp)
#define DBGPRINT(a)     fprintf a 
#else
#define ASSERT(exp)     ((void)0)
#define DBGPRINT(a)     ((void)0)
#endif

#include "ftd_proc.h"
#include "ftd_mngt.h"
#include "errmsg_list.h"

/////////////////////////////////////////////////////////////////////////////
typedef struct pmd_rmd_thread_info_s {
    int proctype;
    int lgnum;
} pmd_rmd_thread_info;      
/*
 * This thread is responsible for sending perf data to TDMF collector
 */
static DWORD WINAPI ftd_mngt_watchdog_Thread(PVOID context)
{
    LList       *proclist = (LList *)context;
	ftd_proc_t	**fpp;
    DWORD       timeout = 1000;//get timeout
    int         iPrevListSz = -1,iNbrHandles;
    HANDLE      *pHandles = 0, *pWk;
    pmd_rmd_thread_info *pTInfo = 0, *pWkTInfo;
    DWORD       status;

    do
    {
        //wake up periodically to refresh list 
        int iListSz = SizeOfLL(proclist);

        if ( iListSz != iPrevListSz )
        {
            free(pTInfo);
            free(pHandles);

            pHandles   = (HANDLE*) malloc(sizeof(HANDLE)*iListSz);
            pTInfo     = (pmd_rmd_thread_info*) malloc(sizeof(pmd_rmd_thread_info)*iListSz);

            iPrevListSz = iListSz;
        }
        pWk        = pHandles;
        pWkTInfo   = pTInfo;

		ForEachLLElement(proclist, fpp) 
        {
            //fpp = (ftd_proc_t	**)pvoid;
			if ( (*fpp)->proctype == FTD_PROC_PMD ||
                 (*fpp)->proctype == FTD_PROC_RMD ) 
            {   
				*pWk                = (*fpp)->procp->pid;//pid = thread HANDLE
                pWkTInfo->lgnum     = (*fpp)->lgnum;
                pWkTInfo->proctype  = (*fpp)->proctype;

                pWk ++;
                pWkTInfo ++;
			}
		}

        iNbrHandles = pWk - pHandles;

        timeout = 1000;//todo : get timeout from configuration

        if ( iNbrHandles > 0 )
        {
            //check if one of then PMD-RMD threads quits...
            status = WaitForMultipleObjects(iNbrHandles,pHandles,0,timeout);
            if ( status >= WAIT_OBJECT_0 && status < WAIT_OBJECT_0 + iNbrHandles )
            {
                //a PMD or RMD thread has exited.
                char msg[120];
                sprintf(msg,"*** Thread servicing %s %d has stopped.***\n"
                            ,pTInfo[ status - WAIT_OBJECT_0 ].proctype == FTD_PROC_PMD ? "PMD" : "RMD"
                            ,pTInfo[ status - WAIT_OBJECT_0 ].lgnum
                            );
                DBGPRINT((stderr,msg));
                //append to msg list to be TX to Collector
                error_msg_list_addMessage(LOG_WARNING, msg);
            }
        }
        else
        {
            Sleep(timeout);//to pace thread
        }

    }while(1);

    free(pTInfo);
    free(pHandles);

    return 0;
}


void    ftd_mngt_watchdog_initialize(LList *proclist)
{
    DWORD tid;
    HANDLE hThread = CreateThread(0,0,ftd_mngt_watchdog_Thread,proclist,0,&tid);
    CloseHandle(hThread);
}



