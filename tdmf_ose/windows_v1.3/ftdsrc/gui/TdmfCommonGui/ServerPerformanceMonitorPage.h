#include "SelectCounterDlg.h"
#include "tdmfcommonguiDoc.h"


//{{AFX_INCLUDES()
//}}AFX_INCLUDES

#if !defined(AFX_SERVERPERFORMANCEMONITORPAGE_H__5A384C5F_42D8_4052_AF1A_ECA2BFB57532__INCLUDED_)
#define AFX_SERVERPERFORMANCEMONITORPAGE_H__5A384C5F_42D8_4052_AF1A_ECA2BFB57532__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ServerPerformanceMonitorPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
//
//color definitions
#define RED				RGB(255,0,0)
#define GREEN			RGB(0,255,0) 
#define BLUE			RGB(0,0,255)
#define YELLOW			RGB(255,255,0)
#define ORANGE			RGB(255,153,51)
#define HOT_PINK		RGB(255,51,153)
#define PURPLE			RGB(153,0,204)
#define CYAN			RGB(0,255,255)
#define BLACK			RGB(0,0,0)
#define WHITE			RGB(255,255,255)
#define LAVENDER		RGB(199,177,255)
#define PEACH			RGB(255,226,177)
#define SKY_BLUE		RGB(142,255,255)
#define FOREST_GREEN	RGB(0,192,0)
#define BROWN			RGB(80,50,0)
#define TURQUOISE		RGB(0,192,192)
#define ROYAL_BLUE		RGB(0,0,192)
#define GREY			RGB(192,192,192)
#define DARK_GREY		RGB(128,128,128)
#define TAN				RGB(255,198,107)
#define DARK_BLUE		RGB(0,0,128)
#define MAROON			RGB(128,0,0)
#define DUSK			RGB(255,143,107)
#define LIGHT_GREY		RGB(225,225,225)	//only for 3D graph lines



class CCounterInfo
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

	int         m_nLgGroupId;
    long        m_SerieNbr;
	enum Stats  m_ePerfData;
    CString     m_strName;
    CString     m_OldTimeStamp;
	int			m_NumberOfSameConcurrentTimeStamp;

	CCounterInfo():m_NumberOfSameConcurrentTimeStamp(0) 
	{
	}
};

class CChartFXSetting
{
public:

 	enum CfxGrid		m_GridStyle;
	BOOL				m_b3DVisible;
	enum CfxGallery     m_GraphStyle;
	BOOL				m_bZoom;
    double				m_LogBase;

	BOOL				m_bSerLegBoxVisible;
    short               m_SerLegBoxLeft;
    short               m_SerLegBoxTop;
    short               m_SerLegBoxWidth;
    short               m_SerLegBoxHeight;
    BarWndDockedPos     m_SerLegBoxDocked;

  	BOOL				m_bDataEditorVisible;
    short               m_DataEditorLeft;
    short               m_DataEditorTop;
    short               m_DataEditorWidth;
    short               m_DataEditorHeight;
    BarWndDockedPos     m_DataEditorDocked;

	CChartFXSetting():  m_bSerLegBoxVisible(FALSE),
                        m_SerLegBoxLeft(-1),
                        m_SerLegBoxTop(-1),
                        m_SerLegBoxWidth(171),
                        m_SerLegBoxHeight(32),
                        m_SerLegBoxDocked(TGFP_BOTTOM),
					    m_bDataEditorVisible(-1),
                        m_DataEditorLeft(-1),
                        m_DataEditorTop(-1),
                        m_DataEditorWidth(24),
                        m_DataEditorHeight(100),
                        m_DataEditorDocked(TGFP_BOTTOM),
					    m_GridStyle(CHART_HORZGRID),
					    m_b3DVisible(FALSE),
					    m_GraphStyle(LINES),
					    m_bZoom(FALSE),
					    m_LogBase(0)
	{
	}
 
};

/////////////////////////////////////////////////////////////////////////////
// CServerPerformanceMonitorPage dialog

class CServerPerformanceMonitorPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CServerPerformanceMonitorPage)

    TDMFOBJECTSLib::ISystemPtr m_pSystem;

// Construction
public:
	CServerPerformanceMonitorPage(TDMFOBJECTSLib::ISystem* pSystem = NULL);
	~CServerPerformanceMonitorPage();

	void SetSystem(TDMFOBJECTSLib::ISystem* pSystem)
	{
		m_pSystem = pSystem;
	}

    void ChangeScaleType();
    void AdjustTheYAxisScale();
    void SetXAxisFormat();
	void SetYAxisFormat();
    void ResetLabelOfTheXAxis();
    void InitialyzeTheDefaultArrayOfSelectedStats();
    void InitialyzeTheMonitor();
    void InitialyzeTheSeries();
    void SaveToolbarSettings();
    void FillTheListOfCounters();
    BOOL UpdateTheTimeStampFromTheCollector();
	void UpdateTheValueInChartFX();


// Dialog Data
	//{{AFX_DATA(CServerPerformanceMonitorPage)
	enum { IDD = IDD_SERVER_PERFORMANCE_MONITOR };
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CServerPerformanceMonitorPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CServerPerformanceMonitorPage)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	afx_msg void OnUserCommandChart1(long wParam, long lParam, short FAR* nRes);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnInternalCommandChart1(long wParam, long lParam, short FAR* nRes);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	DECLARE_EVENTSINK_MAP()
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	CWnd m_ChartFX; // Chart FX Window
	IChartFXPtr m_pChartFX; // Chart FX Object Pointer


    enum CCounterInfo::Stats  m_curePerfData;
    int  m_nMaxPointPerSerie;
    int  m_nCurrentPositionInSerie;
    UINT m_nTimeSlice;
 
	CChartFXSetting             m_ChartFXSetting;
	CUIntArray                  m_arrayOfToolbarSetting;
	CUIntArray                  m_aryDefaultsSelStats;
	CUIntArray *                m_parySelStats;
	CUIntArray *                m_parySelGroups;
	CUIntArray                  m_arySelGroups;
	CMap<int, int, int, int>	  m_mapRGNumberToNumberOfSameTimeStamp;
	CMapWordToOb                m_mapServerIDToArrayOfStatistics;
	CMapWordToOb                m_mapServerIDToArrayOfGroupNumber;

    COleDateTime m_CollectorTimestamp;
    int  m_bFirstPassDone;
    BOOL m_bAllGroups;
    BOOL m_bSelectDialogInUse;
    BOOL m_bPause;
    TDMFOBJECTSLib::IReplicationGroupPtr m_pRG;
	// Counters info
	std::list<CCounterInfo> m_listCounterInfo;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERVERPERFORMANCEMONITORPAGE_H__5A384C5F_42D8_4052_AF1A_ECA2BFB57532__INCLUDED_)
