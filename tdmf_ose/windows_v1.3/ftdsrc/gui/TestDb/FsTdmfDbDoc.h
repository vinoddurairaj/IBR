// FsTdmfDbDoc.h : interface of the CFsTdmfDbDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_FSTDMFDBDOC_H__26F46178_7E6E_42D1_9D4E_9FEC54A9DDD4__INCLUDED_)
#define AFX_FSTDMFDBDOC_H__26F46178_7E6E_42D1_9D4E_9FEC54A9DDD4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CFsTdmfDbDoc : public CDocument
{
protected: // create from serialization only
	CFsTdmfDbDoc();
	DECLARE_DYNCREATE(CFsTdmfDbDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFsTdmfDbDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CFsTdmfDbDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CFsTdmfDbDoc)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FSTDMFDBDOC_H__26F46178_7E6E_42D1_9D4E_9FEC54A9DDD4__INCLUDED_)
