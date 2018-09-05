#include "ReportConfigDlg.h"
#include "tdmfcommonguiDoc.h"


//{{AFX_INCLUDES()
//}}AFX_INCLUDES

#if !defined(AFX_SERVERPERFORMANCEREPORTERPAGE_H__5A384C5F_42D8_4052_AF1A_ECA2BFB57532__INCLUDED_)
#define AFX_SERVERPERFORMANCEREPORTERPAGE_H__5A384C5F_42D8_4052_AF1A_ECA2BFB57532__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ServerPerformanceReporterPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
//

class CReporterCounterInfo
{
public:
	enum Stats {
		eBABEntries = 0,
		ePctBABFull = 1,
		ePctDone    = 2,
		eReadBytes  = 3,
		eWriteBytes = 4,
        eActualNet = 5,
        eEffectiveNet = 6,
		//eBABSectors = 7,
	};

	// Info to retrieve values from DB
	int         m_nLgGroupId;
    long        m_SerieNbr;
	enum Stats  m_ePerfData;
    CString     m_strName;
};


/////////////////////////////////////////////////////////////////////////////
// CServerPerformanceReporterPage dialog

class CServerPerformanceReporterPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CServerPerformanceReporterPage)

// Construction
public:
	CServerPerformanceReporterPage();
	~CServerPerformanceReporterPage();

	void AddCounter(CReporterCounterInfo& CounterInfo);

// Dialog Data
	//{{AFX_DATA(CServerPerformanceReporterPage)
	enum { IDD = IDD_SERVER_PERFORMANCE_REPORTER };
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CServerPerformanceReporterPage)
	public:
	virtual BOOL OnKillActive();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CServerPerformanceReporterPage)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	afx_msg void OnUserCommandChart1(long wParam, long lParam, short FAR* nRes);
	DECLARE_EVENTSINK_MAP()
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	CWnd m_ChartFX; // Chart FX Window
	IChartFXPtr m_pChartFX; // Chart FX Object Pointer

	// Counters info
	std::list<CReporterCounterInfo> m_listCounterInfo;

	void GenerateReport();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERVERPERFORMANCEREPORTERPAGE_H__5A384C5F_42D8_4052_AF1A_ECA2BFB57532__INCLUDED_)
