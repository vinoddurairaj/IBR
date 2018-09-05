// TdmfOseMain.h: interface for the CTdmfOseMain class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SAMPLEMAIN_H__01379F67_7B6F_4A14_8024_097140B1C4AB__INCLUDED_)
#define AFX_TdmfDllModule_H__01379F67_7B6F_4A14_8024_097140B1C4AB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "..\..\..\..\..\SSM\ConsoleFramework\Modules\SpMCommon\SpMBaseModule.h"


class CTdmfDllModule : public CSpMBaseModule
{
public:
	CTdmfDllModule(HWND hWnd, LPTSTR parms);
	virtual ~CTdmfDllModule();

	int  ActivateModule();
	int	 DeactivateModule();
	int	 DerivedActivateSubModule(int moduleID, const CRect &rect);
	void MenuItemClicked(int menuID, int itemID);
	void RecalcLayout();
};


#endif // !defined(AFX_TdmfDllModule_H__01379F67_7B6F_4A14_8024_097140B1C4AB__INCLUDED_)
