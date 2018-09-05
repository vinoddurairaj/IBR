// PropertyPageBase.h: interface for the CPropertyPageBase class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PROPERTYPAGEBASE_H__6C92E835_DE61_49C4_823F_5BCE8BB21850__INCLUDED_)
#define AFX_PROPERTYPAGEBASE_H__6C92E835_DE61_49C4_823F_5BCE8BB21850__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CPropertyPageBase
{
protected:
	BOOL m_bDisableSetActive;

public:
	CPropertyPageBase();
	virtual ~CPropertyPageBase();

	virtual BOOL CaptureTabKey() {return FALSE;}

	virtual void DisableSetActive() {m_bDisableSetActive = true;}
};

#endif // !defined(AFX_PROPERTYPAGEBASE_H__6C92E835_DE61_49C4_823F_5BCE8BB21850__INCLUDED_)
