// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__62586757_2322_48DC_A815_6F519C9F29D9__INCLUDED_)
#define AFX_STDAFX_H__62586757_2322_48DC_A815_6F519C9F29D9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


// Insert your headers here
#define WIN32_LEAN_AND_MEAN	// Exclude rarely-used stuff from Windows headers
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afx.h>
#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

//#include <afxv_w32.h>
#include <winsvc.h>

// TODO: reference additional headers your program requires here
#include "TdmfInstall.h"
#include "FsTdmfDb.h"
#include "FsTdmfRecNvpNames.h"

extern "C"
{
#include "iputil.h"
#include "sock.h"
}

#include <vector>
#include <string>
#include <set>

// TODO: reference additional headers your program requires here

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__62586757_2322_48DC_A815_6F519C9F29D9__INCLUDED_)
