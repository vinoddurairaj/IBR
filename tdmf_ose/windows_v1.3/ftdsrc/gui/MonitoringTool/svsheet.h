#ifndef _SVSheet
#define _SVSheet
#define MAXNUMCOLUMNS 50
#include <afxtempl.h>  // carray

#include "SVColumn.h"

class SVSheet: public CObject
{
	DECLARE_SERIAL (SVSheet)
	CString csName;
	void* pView;					
	bool bUpdatable;
	bool bUseDefaults;
	CArray<SVColumn, SVColumn> caColumns;
	int nColumnOrder[MAXNUMCOLUMNS];
	SVSheet() 
	{
		Clear();
		pView = 0;					
	}
	~SVSheet()
	{
		// the SVBase OnDestroy routine turns the pView to zero. 
		// This ensures that the view is destroyed prior to the sheet
#ifdef _DEBUG 
		if (pView != 0)
		{
			// if it abends here, pView is junk, cannot be if view was destroyed before sheet was 
			CRuntimeClass* pc = ((CView*)pView)->GetRuntimeClass();
			CString cs;
			cs.Format(_T("View %s has not closed this sheet"),pc->m_lpszClassName);
			AfxMessageBox(cs);
			ASSERT (false); // look above
		}
#endif
	}

	SVSheet(SVSheet& pNew) 		// copy constructor
	{
		Copy(&pNew);
	}
	void operator=(SVSheet& pNew ) 
	{
		Copy(&pNew);
	}
	void Copy(SVSheet* p);
	void Clear();
	virtual void Serialize( CArchive& archive );
};


#endif

