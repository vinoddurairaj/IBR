#include <crtdbg.h>
#include <stdarg.h>
#include <stdio.h>
extern "C"
{
#include "ftd_mngt.h"
#include "ftd_error.h"
}

extern errfac_t * gMngtErrfac ;

//
// SVG2004
// Removed:
// possible memory corruption that was not necessary anyhow
void ftd_mngt_tracef( unsigned char pucLevel, char* pcpMsg, ... )
{
    #define MAX_MSG_SZ  4096     

	va_list     lVaLst;
    char        szMsg[ MAX_MSG_SZ ],
                *pszMsg             = szMsg;
    int         len, 
                size                = MAX_MSG_SZ;

   	va_start ( lVaLst, pcpMsg );

	while( (len = _vsnprintf ( pszMsg, size, pcpMsg, lVaLst )) == -1 )
    {   // _vsnprintf() returns -1 when destination buffer is not large enough
        if ( pszMsg != szMsg )
            delete [] pszMsg;
        //enlarge dest. buffer and retry _vsnprintf()
        size += MAX_MSG_SZ;
        pszMsg = new char [ size ];

        if ( pszMsg == NULL )
            return;
    }

    // add date-time hdr in front of message. Final msg available in to pMsgAndTime
    char *pMsgAndTime = new char [len + 512];

    error_format_datetime (ERRFAC, "INFO", "", pszMsg, pMsgAndTime);
    error_tracef_msg (pucLevel, pMsgAndTime);

#ifdef _DEBUG
    printf("\n");
    printf(pMsgAndTime);
#endif

    delete [] pMsgAndTime ;

    if ( pszMsg != szMsg )
        delete [] pszMsg ;
}


