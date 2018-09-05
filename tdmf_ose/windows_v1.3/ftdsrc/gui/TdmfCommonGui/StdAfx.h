// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__431664F2_2676_4CF8_BE31_5D23184CB100__INCLUDED_)
#define AFX_STDAFX_H__431664F2_2676_4CF8_BE31_5D23184CB100__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers


#pragma warning(disable: 4786)
#pragma warning(disable: 4503)


#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

//#include <atlbase.h>
//#include <atlcom.h>
#include <afxrich.h>
#include <afxcview.h>
#include <Windowsx.h>

#include <atlbase.h>

#include <string>
#include <list>
#include <vector>
#include <map>
#include <sstream>
#include <strstream>
#include <set>
#include <fstream>
#include <iomanip>
#include <Imagehlp.h>
#include <process.h>
#include <Shlwapi.h>
#include <Winsock2.h>
#include <htmlhelp.h>


#import "..\..\lib\TdmfObjects\TdmfObjects.tlb" named_guids

#import "ChartFX\sfxbar.dll" no_namespace no_implementation named_guids
#import "ChartFX\Cfx4032.ocx" no_namespace no_implementation named_guids
#import "ChartFX\SfxXMLData.dll" no_namespace no_implementation named_guids


#pragma warning ( 3: 4018 )

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__431664F2_2676_4CF8_BE31_5D23184CB100__INCLUDED_)
