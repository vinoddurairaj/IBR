// ReplicationGroupContextMenu.h: interface for the ReplicationGroupContextMenu class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CREPLICATIONGROUPCONTEXTMENU_H__9F9AE176_6B6A_463D_9A04_5FCA8666A891__INCLUDED_)
#define AFX_CREPLICATIONGROUPCONTEXTMENU_H__9F9AE176_6B6A_463D_9A04_5FCA8666A891__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ContextMenuBase.h"


class CReplicationGroupContextMenu : public CContextMenuBase
{
private:
	TDMFOBJECTSLib::IReplicationGroupPtr m_pRG;

	virtual void AddEntries();
	virtual void OnCommand(UINT nCmd);

	long LaunchCommand(enum ContextMenuCommands eId, enum TDMFOBJECTSLib::tagTdmfCommand eCommand, const char* pszArg = "", bool bSymmetric = false);

public:
	CReplicationGroupContextMenu(CTdmfCommonGuiDoc* pDoc, TDMFOBJECTSLib::IReplicationGroup* pRG);
	virtual ~CReplicationGroupContextMenu();
};


#endif // !defined(AFX_REPLICATIONGROUPCONTEXTMENU_H__9F9AE176_6B6A_463D_9A04_5FCA8666A891__INCLUDED_)
