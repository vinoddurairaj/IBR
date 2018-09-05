// ContextMenuBase.h: interface for the CContextMenuBase class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CONTEXTMENUBASE_H__58BC72DF_C7CA_4580_A2F3_0FDEEFB837D7__INCLUDED_)
#define AFX_CONTEXTMENUBASE_H__58BC72DF_C7CA_4580_A2F3_0FDEEFB837D7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TdmfCommonGuiDoc.h"


static enum ContextMenuCommands
{
	ID_REPLICATION_PAIRS = 1,
	ID_REPLICATION_GROUP,
	ID_SERVERS,
	ID_DOMAIN,
	ID_REFRESH,
	ID_DELETE,
	ID_PROPERTIES,
	ID_CONNECT,
	ID_DISCONNECT,
	ID_IMPORT,
	ID_REPLICATION_SYMMETRIC_ADD,
	ID_REPLICATION_SYMMETRIC_REMOVE,
	ID_REPLICATION_SYMMETRIC_START,
	ID_REPLICATION_SYMMETRIC_STOP,
	ID_REPLICATION_SYMMETRIC_FAILOVER,
	ID_CMD_NONE	            = 1000,
	ID_CMD_LAUNCH_BACKFRESH = 1001,
	ID_CMD_KILL_BACKFRESH   = 1002,
	ID_CMD_LAUNCH_PMD       = 1003,
	ID_CMD_KILL_PMD         = 1004,
	ID_CMD_LAUNCH_REFRESH_SMART    = 1005,
	ID_CMD_LAUNCH_REFRESH_FULL     = 1006,
	ID_CMD_LAUNCH_REFRESH_CHECKSUM = 1007,
	ID_CMD_KILL_REFRESH     = 1008,
	ID_CMD_START            = 1009,
	ID_CMD_STOP             = 1010,
	ID_CMD_CHECKPOINT_ON    = 1011,
	ID_CMD_CHECKPOINT_OFF   = 1012,
	ID_CMD_KILL_RMD         = 1013,
	ID_CMD_STOP_IMMEDIATE   = 1014,
	ID_CMD_OVERRIDE         = 1015,
};


class CContextMenuBase  
{
protected:
	CMenu m_Menu;
	CTdmfCommonGuiDoc* m_pDoc;

	CContextMenuBase(CTdmfCommonGuiDoc* pDoc) : m_pDoc(pDoc)
	{
	}

	virtual void AddEntries() {}
	virtual void OnCommand(UINT nCmd) {}

	virtual void AddCommandEntries(char* pszKey, BOOL bEnable = TRUE, int nMode = 0, int nConnectionStatus = 0, int nPlatform = 0)
	{
		CMenu MenuCommand;
		MenuCommand.CreatePopupMenu();

		int nNbEntries = m_pDoc->m_pSystem->GetNbCommandMenuEntries(pszKey);
		
		// For each entries defined
		for (int nIndex = 0; nIndex < nNbEntries; nIndex++)
		{
			// Enable/disable cmd
			BOOL bCmdEnabled = m_pDoc->m_pSystem->IsCommandMenuEntryEnabled(pszKey, nIndex, nMode, nConnectionStatus, nPlatform);

			// add it to the menu
			UINT uFlag = MF_STRING | (bCmdEnabled) ? MF_ENABLED : MF_GRAYED;
			MenuCommand.AppendMenu(uFlag,
								   m_pDoc->m_pSystem->GetCommandMenuEntryId(pszKey, nIndex),
								   m_pDoc->m_pSystem->GetCommandMenuEntryName(pszKey, nIndex));
		}

		UINT uFlag = MF_STRING | (bEnable) ? MF_ENABLED | MF_POPUP : MF_GRAYED;
		m_Menu.AppendMenu(uFlag, (UINT)MenuCommand.m_hMenu, "Commands");
		MenuCommand.Detach();
	}

public:
	CContextMenuBase() {}
	virtual ~CContextMenuBase() {}

	virtual LRESULT Show(CPoint* pPoint = NULL)
	{
		VERIFY(m_Menu.CreatePopupMenu());
		
		// Fill Context Menu
		AddEntries();
		
		// Display Context Menu
		CPoint Point;
		if (pPoint == NULL)
		{
			GetCursorPos(&Point);
		}
		else
		{
			Point = *pPoint;
		}

		UINT nCmd = ::TrackPopupMenuEx(m_Menu.m_hMenu,
									   TPM_LEFTALIGN|TPM_RIGHTBUTTON|TPM_NONOTIFY|TPM_RETURNCMD,
									   Point.x, Point.y, GetActiveWindow(), NULL);
		
		// Take action		
		SendCommand(nCmd);
		
		return 0;
	}

	virtual void SendCommand(UINT nCmd)
	{
		// If the selection hasn't been cancelled
		if (nCmd != 0)
		{
			// Take action
			OnCommand(nCmd);
		}
	}
};


#endif // !defined(AFX_CONTEXTMENUBASE_H__58BC72DF_C7CA_4580_A2F3_0FDEEFB837D7__INCLUDED_)
