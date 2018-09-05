// ServerContextMenu.h: interface for the CServerContextMenu class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SERVERCONTEXTMENU_H__D9EA527B_8823_40DB_8F88_9F95C5E9C3D1__INCLUDED_)
#define AFX_SERVERCONTEXTMENU_H__D9EA527B_8823_40DB_8F88_9F95C5E9C3D1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "ContextMenuBase.h"


class CServerContextMenu : public CContextMenuBase
{
private:
	TDMFOBJECTSLib::IServerPtr m_pServer;

	virtual void AddEntries();
	virtual void OnCommand(UINT nCmd);

	long LaunchCommand(enum ContextMenuCommands nId, enum TDMFOBJECTSLib::tagTdmfCommand eCommand, const char* pszArg = "");

public:
	CServerContextMenu(CTdmfCommonGuiDoc* pDoc, TDMFOBJECTSLib::IServer* pServer);

	virtual ~CServerContextMenu();
};

#endif // !defined(AFX_SERVERCONTEXTMENU_H__D9EA527B_8823_40DB_8F88_9F95C5E9C3D1__INCLUDED_)
