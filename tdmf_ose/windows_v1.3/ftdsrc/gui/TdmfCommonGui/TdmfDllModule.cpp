// TdmfOseMain.cpp: implementation of the CTdmfDllModule class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#ifdef TDMF_IN_A_DLL

#include "resource.h"
#include "TdmfCommonGui.h"
#include "TdmfDllModule.h"
#include "TdmfCommonGuiDoc.h"
#include "MainFrm.h"
#include "TdmfDocTemplate.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


enum TdmfDllModuleMenu
{
	FILE_MENU_CONNECT = 101,
	FILE_MENU_DISCONNECT,
	TDMF_MENU_REFRESH,
	TDMF_MENU_OPTIONS,
	HELP_MENU_CONTENTS,
	HELP_MENU_SEARCH,
	HELP_MENU_INDEX,
	HELP_MENU_ABOUT_TDMF,
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTdmfDllModule::CTdmfDllModule(HWND hWnd, LPTSTR parms) :
	CSpMBaseModule(hWnd)
{
	SetModuleName(_T("TDMF"));

	AddSubModule(CString("TDMF OSE"), NULL);

	CBitmap bms;
	bms.LoadBitmap(IDB_SMALL_ICONS);
	m_smallList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 0, 1);
	m_smallList.Add(&bms,  RGB(255,255,0));

	CBitmap bm;
	bm.LoadBitmap(IDB_LARGE_ICONS);
	m_largeList.Create(32, 32, ILC_COLOR24 | ILC_MASK, 0, 1);
	m_largeList.Add(&bm, RGB(255,255,0));

	///////////////////////////////////////////////////////////
	//Menu addition

	m_fileMenuItems.second.push_back(T_MENU_ITEM(FILE_MENU_CONNECT,    L"&Connect..."));
	m_fileMenuItems.second.push_back(T_MENU_ITEM(FILE_MENU_DISCONNECT, L"&Disconnect..."));

	m_moduleMenuItems.first = L"Tools";
	m_moduleMenuItems.second.push_back(T_MENU_ITEM(TDMF_MENU_REFRESH,  L"&Refresh"));
	m_moduleMenuItems.second.push_back(T_MENU_ITEM(TDMF_MENU_OPTIONS,  L"&Options..."));

	m_helpMenuItems.second.push_back(T_MENU_ITEM(HELP_MENU_CONTENTS,   L"&Contents..."));
	m_helpMenuItems.second.push_back(T_MENU_ITEM(HELP_MENU_SEARCH,     L"&Search..."));
	m_helpMenuItems.second.push_back(T_MENU_ITEM(HELP_MENU_INDEX,      L"&Index..."));
	//m_helpMenuItems.second.push_back(T_MENU_ITEM(SEPARATOR, L""));
	m_helpMenuItems.second.push_back(T_MENU_ITEM(HELP_MENU_ABOUT_TDMF, L"&About TDMF..."));
}

CTdmfDllModule::~CTdmfDllModule()
{
	DeactivateModule();
}

int CTdmfDllModule::ActivateModule()
{
	return 0;
}

int CTdmfDllModule::DeactivateModule()
{
	int nbSubs = NumSubModules();
	for (int i = 0; i < nbSubs; i++)
	{
		CFrameWnd* pFrm = m_SubModules[i].second;
		if (m_SubModules[i].second != NULL)
		{
			m_SubModules[i].second->DestroyWindow();
			m_SubModules[i].second = NULL;
		}
	}
	m_SubModules.clear();

	return 0;
}

int	CTdmfDllModule::DerivedActivateSubModule(int moduleID, const CRect &rect)
{
	if ( m_SubModules[moduleID].second == NULL )
	{
		switch ( moduleID )
		{
		case 0:
			{
				if (AfxGetApp()->m_pDocManager != NULL)
				{
					POSITION Pos = AfxGetApp()->m_pDocManager->GetFirstDocTemplatePosition();
					CTdmfDocTemplate* pTemplate = (CTdmfDocTemplate*)AfxGetApp()->m_pDocManager->GetNextDocTemplate(Pos);
					ASSERT(pTemplate != NULL);
					ASSERT_KINDOF(CDocTemplate, pTemplate);

					pTemplate->SetParentWnd(GetParentCWnd());

					pTemplate->OpenDocumentFile(NULL);
				}

				m_SubModules[moduleID].second = (CFrameWnd*)AfxGetApp()->GetMainWnd();
			}
			break;
		default:
			return -1;
		}
	}
	m_CurModuleID = moduleID;
	return 0;
}

void CTdmfDllModule::MenuItemClicked(int menuID, int itemID)
{
	CWnd* pWnd = AfxGetApp()->GetMainWnd();

	switch(menuID)
	{
	case FILE_MENU:
		switch(itemID)
		{
		case FILE_MENU_CONNECT:
			pWnd->SendMessage(WM_COMMAND, ID_FILE_CONNECT);
			break;

		case FILE_MENU_DISCONNECT:
			pWnd->SendMessage(WM_COMMAND, ID_FILE_DISCONNECT);
			break;
		}
		break;

	case MODULE_MENU:
		switch(itemID)
		{
		case TDMF_MENU_REFRESH:
			pWnd->SendMessage(WM_COMMAND, ID_TOOLS_REFRESH);
			break;

		case TDMF_MENU_OPTIONS:
			pWnd->SendMessage(WM_COMMAND, ID_TOOLS_OPTIONS);
			break;
		}
		break;

	case HELP_MENU:
		switch(itemID)
		{
		case HELP_MENU_CONTENTS:
			pWnd->SendMessage(WM_COMMAND, ID_HELP_CONTENTS);
			break;

		case HELP_MENU_SEARCH:
			pWnd->SendMessage(WM_COMMAND, ID_HELP_SEARCH);
			break;

		case HELP_MENU_INDEX:
			pWnd->SendMessage(WM_COMMAND, ID_HELP_INDEX);
			break;

		case HELP_MENU_ABOUT_TDMF:
			pWnd->SendMessage(WM_COMMAND, ID_APP_ABOUT);
			break;
		}
		break;
	}
}

void CTdmfDllModule::RecalcLayout()
{
	if (m_CurModuleID != -1)
	{
		if (m_SubModules[m_CurModuleID].second != NULL)
		{
			CRect	rect;
			if (IsWindow(GetParentCWnd()->m_hWnd))
			{
				GetParentCWnd()->GetClientRect(&rect);
				m_SubModules[m_CurModuleID].second->RecalcLayout();
				// Move window: hide border
				m_SubModules[m_CurModuleID].second->SetWindowPos ( NULL, -2, -2, rect.Width()+2, rect.Height()+2, SWP_SHOWWINDOW | SWP_NOZORDER );
			}
		}
	}
}


#endif
