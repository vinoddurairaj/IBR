// DomainContextMenu.h: interface for the CDomainContextMenu class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DOMAINCONTEXTMENU_H__E5075558_7540_445C_92B8_C1D03718CA27__INCLUDED_)
#define AFX_DOMAINCONTEXTMENU_H__E5075558_7540_445C_92B8_C1D03718CA27__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ContextMenuBase.h"
#include "TdmfCommonGuiDoc.h"


class CDomainContextMenu : public CContextMenuBase  
{
private:
	TDMFOBJECTSLib::IDomainPtr m_pDomain;

	virtual void AddEntries();
	virtual void OnCommand(UINT nCmd);

public:
	CDomainContextMenu(CTdmfCommonGuiDoc* pDoc, TDMFOBJECTSLib::IDomain* pDomain);
	virtual ~CDomainContextMenu();
};


#endif // !defined(AFX_DOMAINCONTEXTMENU_H__E5075558_7540_445C_92B8_C1D03718CA27__INCLUDED_)
