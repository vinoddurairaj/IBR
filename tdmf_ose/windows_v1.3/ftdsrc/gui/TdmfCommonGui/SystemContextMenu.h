// SystemContextMenu.h: interface for the CSystemContextMenu class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SYSTEMCONTEXTMENU_H__8CB33C09_177D_46C3_9833_4AAC2F50E6E7__INCLUDED_)
#define AFX_SYSTEMCONTEXTMENU_H__8CB33C09_177D_46C3_9833_4AAC2F50E6E7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ContextMenuBase.h"
#include "TdmfCommonGuiDoc.h"


class CSystemContextMenu : public CContextMenuBase  
{
private:
	virtual void AddEntries();
	virtual void OnCommand(UINT nCmd);

public:
	CSystemContextMenu(CTdmfCommonGuiDoc* pDoc);
	virtual ~CSystemContextMenu();

};

#endif // !defined(AFX_SYSTEMCONTEXTMENU_H__8CB33C09_177D_46C3_9833_4AAC2F50E6E7__INCLUDED_)
