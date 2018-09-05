// ViewNotification.cpp: implementation of the CViewNotification class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "tdmfcommongui.h"
#include "ViewNotification.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CViewNotification::CViewNotification() : m_nMessageId(EMPTY), m_pUnk(NULL),
	m_eParam(NONE), m_dwParam1(0), m_dwParam2(0)
{
}

CViewNotification::~CViewNotification()
{
}

