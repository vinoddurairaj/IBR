// FsTdmfDbDoc.cpp : implementation of the CFsTdmfDbDoc class
//

#include "stdafx.h"
#include "FsTdmfDbApp.h"

#include "FsTdmfDbDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFsTdmfDbDoc

IMPLEMENT_DYNCREATE(CFsTdmfDbDoc, CDocument)

BEGIN_MESSAGE_MAP(CFsTdmfDbDoc, CDocument)
	//{{AFX_MSG_MAP(CFsTdmfDbDoc)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFsTdmfDbDoc construction/destruction

CFsTdmfDbDoc::CFsTdmfDbDoc()
{
}

CFsTdmfDbDoc::~CFsTdmfDbDoc()
{
}

BOOL CFsTdmfDbDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

/*
   POSITION lPos = GetFirstViewPosition ();
   while (lPos != NULL)
   {
		CFormView* lpFv = (CFormView*) GetNextView ( lPos );
		lpFv->ResizeParentToFit ( FALSE );
		lpFv->ShowWindow ( SW_SHOWMAXIMIZED );
   }

   CDocTemplate* pDocTemplate = GetDocTemplate ();
*/
   return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CFsTdmfDbDoc serialization

void CFsTdmfDbDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
	}
	else
	{
	}
}

/////////////////////////////////////////////////////////////////////////////
// CFsTdmfDbDoc diagnostics

#ifdef _DEBUG
void CFsTdmfDbDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CFsTdmfDbDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CFsTdmfDbDoc commands

