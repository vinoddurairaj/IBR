// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__F27A7C05_45D9_4629_9E57_E570B212A8A7__INCLUDED_)
#define AFX_STDAFX_H__F27A7C05_45D9_4629_9E57_E570B212A8A7__INCLUDED_

#define _WIN32_WINNT 0x0500
#define WINVER 0x0500

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxcview.h>
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT


#if defined(_DEBUG)
   #define TRACERECT(s, r) TRACE(_T("%s - " #r " - L: %d, T: %d, R: %d, B: %d, W: %d, H: %d\n"), \
   s, r.left, r.top, r.right, r.bottom, r.right - r.left, r.bottom - r.top)
#endif

#include "TDMFObjects.h"

#define TypeServer	1
#define TypeLGroup	2
#define TypeRPair		3

const COLORREF COLOR_LEVEL1 = RGB(247, 243, 234);
const COLORREF COLOR_LEVEL2 = RGB(232, 228, 220);
const COLORREF COLOR_LEVEL3 = RGB(255, 252, 242);

// commonly used enum types
enum UPDATE_VIEW_HINT
		{
			UVH_NULL,
			UVH_FIRST_UPDATE,
			UVH_UPDATE,
			UVH_CHANGE_GRAPH_TYPE,
			UVH_CHANGE_MODE,

		};

//Type of state
#define StateInactive			0
#define StateActive				1
#define StateStarted				2
#define StateRefresh				3
#define StateConnectionLost	4
#define StateCheckpoint			5
#define StateBackfresh			6
#define StateTracking			7
#define StatePassthru			8
#define StateConnect				9
#define StateUndefined			10
#define StateNormal				11

//Type of Mode
#define ModeNormal				0
#define ModeTracking				1
#define ModeUndefined			2

//Type of Action
#define ActionStart				0
#define ActionStop				1

//Type of Screen mode
#define ConfigurationMode			0
#define MonitoringMode				1

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__F27A7C05_45D9_4629_9E57_E570B212A8A7__INCLUDED_)
