#include "stdafx.h"

#ifdef TDMF_IN_A_DLL

#include "TdmfDllModule.h"

static CTdmfDllModule* gTdmfDllModule;

extern "C"
{

_declspec(dllexport) int DLLInitializeWindow(HWND hWnd, LPCWSTR cmdLine)
{
	USES_CONVERSION;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	::CoInitialize(NULL);
	gTdmfDllModule = new CTdmfDllModule(hWnd, W2T(cmdLine));
	return 0;
}

_declspec(dllexport) LPCWSTR DLLName()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (gTdmfDllModule != NULL)
	{
		static CComBSTR bstr = gTdmfDllModule->GetModuleName();
		return bstr;
	}
	else
		return L"";
}

_declspec(dllexport) int DLLNumSubModules()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (gTdmfDllModule != NULL)
		return gTdmfDllModule->NumSubModules();
	else
		return 0;
}

_declspec(dllexport) LPCWSTR  DLLGetSubModuleName(int moduleID)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (gTdmfDllModule != NULL)
	{
		static CComBSTR bstr = gTdmfDllModule->GetSubModuleName(moduleID);
		return bstr;
	}
	else
		return L"";
}

_declspec(dllexport) CImageList * DLLGetSmallBitmap()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (gTdmfDllModule != NULL)
		return gTdmfDllModule->GetSmallBitmap();
	else
		return 0;
}

_declspec(dllexport) CImageList * DLLGetLargeBitmap (  ) {
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (gTdmfDllModule != NULL)
		return gTdmfDllModule->GetLargeBitmap();
	else
		return 0;
}

_declspec(dllexport) int DLLActivateModule()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (gTdmfDllModule != NULL)
		return gTdmfDllModule->ActivateModule();
	else
		return 0;
}

_declspec(dllexport) int DLLDeactivateModule()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (gTdmfDllModule != NULL)
	{
		int nRet = gTdmfDllModule->DeactivateModule();
		delete gTdmfDllModule;
		gTdmfDllModule = NULL;

		return nRet;
	}
	else
		return 0;
}

_declspec(dllexport) int DLLActivateSubModule(int moduleID)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (gTdmfDllModule != NULL)
	{
		int retVal = gTdmfDllModule->ActivateSubModule(moduleID);
		gTdmfDllModule->RecalcLayout();
		return retVal;
	}
	else
		return 0;
}

_declspec(dllexport) int DLLDeActivateSubModule()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (gTdmfDllModule != NULL)
	{
		int retVal = gTdmfDllModule->DeActivateSubModule();
		return retVal;
	}
	else
		return 0;
}

_declspec(dllexport) void DLLRecalcLayout()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (gTdmfDllModule != NULL)
		gTdmfDllModule->RecalcLayout();
}

_declspec(dllexport) T_MENU_INFO* DLLGetFileMenuItems()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (gTdmfDllModule != NULL)
		return gTdmfDllModule->GetFileMenuItems();
	return NULL;
}

_declspec(dllexport) T_MENU_INFO* DLLGetViewMenuItems()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (gTdmfDllModule != NULL)
		return gTdmfDllModule->GetViewMenuItems();
	return NULL;
}

_declspec(dllexport) T_MENU_INFO* DLLGetModuleMenuItems()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (gTdmfDllModule != NULL)
		return gTdmfDllModule->GetModuleMenuItems();
	return NULL;
}

_declspec(dllexport) T_MENU_INFO* DLLGetHelpMenuItems()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (gTdmfDllModule != NULL)
		return gTdmfDllModule->GetHelpMenuItems();
	return NULL;
}

_declspec(dllexport) void DLLMenuItemClicked(int menuID, int itemID)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (gTdmfDllModule != NULL)
		gTdmfDllModule->MenuItemClicked(menuID, itemID);
}

_declspec(dllexport) BOOL DLLFilterMsg(MSG *pMsg)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (gTdmfDllModule != NULL)
		return gTdmfDllModule->FilterMsg(pMsg);

	return false;
}

_declspec(dllexport) void DLLProcessIdle()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (gTdmfDllModule!= NULL)
		gTdmfDllModule->ProcessIdle();
}

_declspec(dllexport) void DLLShowHelp()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (gTdmfDllModule != NULL)
		gTdmfDllModule->ShowHelp();
}


}

#endif
