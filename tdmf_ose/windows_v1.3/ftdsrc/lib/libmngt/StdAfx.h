// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__041CBD71_972E_43FD_9D42_DB14F1B093E8__INCLUDED_)
#define AFX_STDAFX_H__041CBD71_972E_43FD_9D42_DB14F1B093E8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


// Insert your headers here
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

//#include <windows.h>
#include <afx.h>

// TODO: reference additional headers your program requires here

//////////////////////////////////////////////////////////////////
// section begin
// if precompiler header not in use, this section must appear
// in libmngt.cpp
//
#include "libmngtdef.h"    //declarations of some structures
extern "C" {
#include "sock.h"	// use the libsock API
}
#include "libmngtmsg.h"    //declarations of message structures

class mmp_handle
{
public:
    mmp_handle(unsigned int iTDMFSrvIP, unsigned int iLocalIP, int iPort)
        :iTDMFSrvIP(iTDMFSrvIP) 
        ,iLocalIP(iLocalIP) 
        ,iPort(iPort)
        ,iSignature(iReferenceSignature)
        {};

    static int          iReferenceSignature;      

    int                 iSignature;
    unsigned int        iTDMFSrvIP;//ip address of TDMF Collector
    unsigned int        iLocalIP;//local ip address 
    int                 iPort;

};

#define	MMP_HANDLE			        mmp_handle*
#include "libmngt.h"	
//
// section end
//////////////////////////////////////////////////////////////////



//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__041CBD71_972E_43FD_9D42_DB14F1B093E8__INCLUDED_)
