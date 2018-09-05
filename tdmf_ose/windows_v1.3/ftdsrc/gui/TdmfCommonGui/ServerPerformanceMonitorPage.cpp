// ServerPerformanceMonitorPage.cpp : implementation file
//

#include "stdafx.h"

#include "tdmfcommongui.h"
#include "ServerPerformanceMonitorPage.h"
#include "ToolsView.h"
#include "TimeRangeDlg.h"
#include <afxctl.h>

#define CHART_HIDDEN 1.0e+308

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TimeOutDuration 1000 //15 seconds
#define DefaultTimeSlice 10


/////////////////////////////////////////////////////////////////////////////
// CServerPerformanceMonitorPage property page

IMPLEMENT_DYNCREATE(CServerPerformanceMonitorPage, CPropertyPage)

CServerPerformanceMonitorPage::CServerPerformanceMonitorPage(TDMFOBJECTSLib::ISystem* pSystem)
	: CPropertyPage(CServerPerformanceMonitorPage::IDD), m_pSystem(pSystem) 
{
	//{{AFX_DATA_INIT(CServerPerformanceMonitorPage)
	//}}AFX_DATA_INIT
    m_nCurrentPositionInSerie = 0;
    m_bFirstPassDone = FALSE;
    m_bSelectDialogInUse = FALSE;
    m_bAllGroups = TRUE;
    m_bPause = FALSE;
    m_pRG = NULL;
    m_nTimeSlice = DefaultTimeSlice;    
	m_nMaxPointPerSerie = m_nTimeSlice * 4;
    m_parySelStats = NULL;
  
}

CServerPerformanceMonitorPage::~CServerPerformanceMonitorPage()
{
    // Now delete the elements
    POSITION pos = m_mapServerIDToArrayOfStatistics.GetStartPosition();
    while( pos != NULL )
    {
        CUIntArray* pTheArrayOfStats;
        WORD key;
        // Gets key (Word) and value (pTheArrayOfStats)
        m_mapServerIDToArrayOfStatistics.GetNextAssoc( pos, key, (CObject*&) pTheArrayOfStats );
        delete pTheArrayOfStats;
    }
    // RemoveAll deletes the keys
    m_mapServerIDToArrayOfStatistics.RemoveAll();

	 // Now delete the Groups
    pos = m_mapServerIDToArrayOfGroupNumber.GetStartPosition();
    while( pos != NULL )
    {
        CUIntArray* pArrayOfGroups;
        WORD key;
        // Gets key (Word) and value (pTheArrayOfStats)
        m_mapServerIDToArrayOfGroupNumber.GetNextAssoc( pos, key, (CObject*&) pArrayOfGroups );
        delete pArrayOfGroups;
    }
    // RemoveAll deletes the keys
    m_mapServerIDToArrayOfGroupNumber.RemoveAll();
}

void CServerPerformanceMonitorPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServerPerformanceMonitorPage)
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_CHART1, m_ChartFX); // Link variable to control 
	if (!pDX->m_bSaveAndValidate) // Link Chart FX pointer to control window 
	{
		m_pChartFX = m_ChartFX.GetControlUnknown(); 
	}
}


BEGIN_MESSAGE_MAP(CServerPerformanceMonitorPage, CPropertyPage)
	//{{AFX_MSG_MAP(CServerPerformanceMonitorPage)
	ON_WM_SIZE()
	ON_WM_SHOWWINDOW()
	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServerPerformanceMonitorPage message handlers

void CServerPerformanceMonitorPage::OnSize(UINT nType, int cx, int cy) 
{
	CPropertyPage::OnSize(nType, cx, cy);	

	// Move (resize) control's wnd
	if (IsWindow(m_ChartFX.m_hWnd))
	{
		m_ChartFX.MoveWindow(5, 5, cx - 10, cy - 10);
	}
}

BOOL CServerPerformanceMonitorPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	m_pChartFX->Language("Cfx4TDMFLang.dll");

	InitialyzeTheDefaultArrayOfSelectedStats();

    //Add 5 custom buttons to the toolbar
	m_pChartFX->GetToolBarObj()->AddItems(5,0);

	// create commands for the buttons
	ICommandItemPtr pCmdItemAdd = m_pChartFX->GetCommands()->AddCommand(1);
	pCmdItemAdd->Text = "Select groups and statistics";
	pCmdItemAdd->Picture =  0;

	int nIndex;
	CPictureHolder* pictHolder = new CPictureHolder();
	pictHolder->CreateEmpty();
	pictHolder->CreateFromBitmap(IDB_BTN_CLEAR);
	nIndex  = m_pChartFX->Commands->AddPicture(pictHolder->GetPictureDispatch());

	ICommandItemPtr pCmdItemClear = m_pChartFX->GetCommands()->AddCommand(2);
	pCmdItemClear->Text = "Clear the datas";
	pCmdItemClear->Picture = nIndex;

	pictHolder->CreateEmpty();
	pictHolder->CreateFromBitmap(IDB_BTN_LOGARITHM);
	nIndex  = m_pChartFX->Commands->AddPicture(pictHolder->GetPictureDispatch());

	ICommandItemPtr pCmdItemLog = m_pChartFX->GetCommands()->AddCommand(3);
	pCmdItemLog->Text = "Logarithm view";
	pCmdItemLog->Style = CBIS_TWOSTATE;
	pCmdItemLog->Picture = nIndex;

	pictHolder->CreateEmpty();
	pictHolder->CreateFromBitmap(IDB_BTN_PAUSE);
	nIndex  = m_pChartFX->Commands->AddPicture(pictHolder->GetPictureDispatch());

	ICommandItemPtr pCmdItemPause = m_pChartFX->GetCommands()->AddCommand(4);
	pCmdItemPause->Text = "Pause";
	pCmdItemPause->Style = CBIS_TWOSTATE;
	pCmdItemPause->Picture = nIndex;

	pictHolder->CreateEmpty();
	pictHolder->CreateFromBitmap(IDB_BTN_TIMERANGE);
	nIndex  = m_pChartFX->Commands->AddPicture(pictHolder->GetPictureDispatch());
	
	ICommandItemPtr pCmdItemTimeSlice = m_pChartFX->GetCommands()->AddCommand(5);
	pCmdItemTimeSlice->Text = "Time Range";
	pCmdItemTimeSlice->Picture = nIndex;

	delete pictHolder;
 	
    //Associate generated number after push on custom button in the toolbar
	m_pChartFX->GetToolBarObj()->GetItem(0)->CommandID = 1;
	m_pChartFX->GetToolBarObj()->GetItem(1)->CommandID = 2;
	m_pChartFX->GetToolBarObj()->GetItem(2)->CommandID = 3;
	m_pChartFX->GetToolBarObj()->GetItem(3)->CommandID = 4;
	m_pChartFX->GetToolBarObj()->GetItem(4)->CommandID = 5;
  
	// Make all custom button visible
	m_pChartFX->ToolBarObj->Item[0]->Visible = -1;
	m_pChartFX->ToolBarObj->Item[1]->Visible = -1;
	m_pChartFX->ToolBarObj->Item[2]->Visible = -1;
 	m_pChartFX->ToolBarObj->Item[3]->Visible = -1;
	m_pChartFX->ToolBarObj->Item[4]->Visible = -1;
	m_pChartFX->ToolBarObj->Item[5]->Visible = -1;


   	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();

	if(m_bAllGroups)
	{
		m_pChartFX->PutTitle(CHART_TOPTIT,pDoc->GetSelectedServer()->Name);
	}
	else
	{
		CString StrTitle = (BSTR)pDoc->GetSelectedServer()->Name;
        StrTitle += _T(" (Group: ");
		StrTitle +=  (BSTR)m_pRG->Name;
		StrTitle += _T(")");
		m_pChartFX->PutTitle(CHART_TOPTIT,StrTitle.AllocSysString());

	}
	m_pChartFX->Scrollable = FALSE;
	m_pChartFX->Cluster = -1;

	m_pChartFX->GetDataEditorObj()->Moveable = FALSE;
	m_pChartFX->GetDataEditorObj()->BorderStyle = BBS_SPLITTER;

	m_pChartFX->GetAxis()->Item[AXIS_X]->Visible = -1;
	m_pChartFX->GetAxis()->Item[AXIS_Y]->Visible = -1;


	m_pChartFX->DblClk(CHART_NONECLK,0);
	m_pChartFX->GetToolBarObj()->Moveable = FALSE;

	SetXAxisFormat();

	SetYAxisFormat();

	m_pChartFX->TypeMask = (enum CfxType) (m_pChartFX->GetTypeMask() | CT_TRACKMOUSE) ;

	// set the Constant Line to disconnected
	m_pChartFX->OpenDataEx((enum CfxCod)(COD_CONSTANTS), 1,0);
	ICfxConstPtr pConstantLine;
	pConstantLine = m_pChartFX->GetConstantLine()->GetItem(0);
	pConstantLine->Value = m_nCurrentPositionInSerie;
	pConstantLine->LineColor = RED;
	pConstantLine->Axis = AXIS_X;
	pConstantLine->Label = "Server is disconnected";
	pConstantLine->Style = CC_HIDE;
	pConstantLine->LineWidth = 1;
	pConstantLine->LineStyle = CHART_PS_TRANSPARENT;
	m_pChartFX->CloseData((enum CfxCod)(COD_CONSTANTS));


	InitialyzeTheMonitor();

	FillTheListOfCounters();

	UpdateTheValueInChartFX();

	return TRUE;  // return TRUE unless you set the focus to a control
			   // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CServerPerformanceMonitorPage::UpdateTheTimeStampFromTheCollector()
{
   bool bResult = false;

   TDMFOBJECTSLib::ICollectorStatsPtr pICollectorStats ;

   pICollectorStats = m_pSystem->GetCollectorStats();


   if(pICollectorStats != NULL)
   {

      COleDateTime TimeStamp(pICollectorStats->GetTimeCollector());

      if(TimeStamp.m_status == COleDateTime::valid)
         m_CollectorTimestamp = TimeStamp;

      UpdateTheValueInChartFX();

      bResult = true;
   }

   return bResult;
}

BOOL CServerPerformanceMonitorPage::OnSetActive() 
{
	((CToolsView*)(GetParent()->GetParent()))->EnableScrollBars();

 	return CPropertyPage::OnSetActive();
}

void CServerPerformanceMonitorPage::ResetLabelOfTheXAxis()
{ 
    CString szValueEmpty = _T("");
    for (int i = 0;i < m_nMaxPointPerSerie; i++)
    {
       m_pChartFX->GetAxis()->GetItem(AXIS_X)->Label[i] = szValueEmpty.AllocSysString();
    }
}
 
void CServerPerformanceMonitorPage::InitialyzeTheSeries()
{
	USES_CONVERSION;

	m_pChartFX->ClearData(CD_DATA);

	if (m_listCounterInfo.size() > 0)
	{ 
		int nSerieNbr = 0;
		CString szDate;

		m_pChartFX->OpenDataEx((enum CfxCod)(COD_VALUES ), m_listCounterInfo.size(),m_nMaxPointPerSerie);

		//Fill all the series with empty point
		ResetLabelOfTheXAxis();

		CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();

		std::list<CCounterInfo>::iterator it;
		for (it = m_listCounterInfo.begin(); it != m_listCounterInfo.end(); it++)
		{
			it->m_NumberOfSameConcurrentTimeStamp = 0;
			// Find group with id = it->m_nLgGroupId
			TDMFOBJECTSLib::IReplicationGroupPtr pRG;
			for (int i = pDoc->GetSelectedServer()->ReplicationGroupCount - 1; i >= 0; i--)
			{
				if (pDoc->GetSelectedServer()->GetReplicationGroup(i)->GroupNumber == it->m_nLgGroupId)
				{
                    
					pRG = pDoc->GetSelectedServer()->GetReplicationGroup(i);

                    it->m_SerieNbr = nSerieNbr;
                    CString strStat;
			        if (pRG != NULL)
			        {
						// Get the selected stat (it->m_ePerfData)
						double dVal = 0;
						switch(it->m_ePerfData)
						{
						case CCounterInfo::eBABEntries:
							strStat = _T("Entries in BAB ");
							dVal = pRG->BABEntries;
							break;
						case CCounterInfo::ePctBABFull:
							dVal = pRG->PctBAB;
							strStat = _T("% BAB Full ");
							break;
						case CCounterInfo::ePctDone:
							dVal = pRG->PctDone;
							strStat = _T("% Done ");
							break;
						case CCounterInfo::eReadBytes:
							dVal = atof(OLE2A(pRG->ReadKbps));
							strStat = _T("Read Bytes ");
							break;
						case CCounterInfo::eWriteBytes:
							dVal = atof(OLE2A(pRG->WriteKbps));
							strStat = _T("Write Bytes ");
							break;
						case CCounterInfo::eActualNet:
							dVal = atof(OLE2A(pRG->ActualNet));
							strStat = _T("Actual Net Bytes ");
							break;
						case CCounterInfo::eEffectiveNet:
							dVal = atof(OLE2A(pRG->EffectiveNet));
							strStat = _T("Effective Net Bytes ");
							break;
	            
				        }
                    }
                    CString strValue(it->m_strName);
                    strValue += _T(" - ") + strStat;
                    m_pChartFX->GetSeries()->GetItem((short)it->m_SerieNbr)->PutLegend(strValue.AllocSysString());

					for (int i = 0;i < m_nMaxPointPerSerie; i++)
					{
						m_pChartFX->ValueEx[(short)it->m_SerieNbr][i] = CHART_HIDDEN;
					}
                }
            }
             nSerieNbr++;
        }
         m_pChartFX->CloseData((enum CfxCod)(COD_VALUES | COD_SMOOTH ));
    }
}

void CServerPerformanceMonitorPage::UpdateTheValueInChartFX()
{
	USES_CONVERSION;

    if(m_ChartFX.m_hWnd == NULL)
        return;
     
	if(m_bSelectDialogInUse)
        return;

    if(m_bPause)
        return;
    
	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();

    TDMFOBJECTSLib::IServerPtr pServer = pDoc->GetSelectedServer();
    if(pServer)
    {
		m_pChartFX->GetToolBarObj()->GetCommands()->GetItem(1)->Enabled = (m_pRG == NULL);
      
	    CString szDate;
 		if(!m_bFirstPassDone)
		{
			m_nCurrentPositionInSerie = 0;
			InitialyzeTheSeries();
			szDate  = m_CollectorTimestamp.Format("%H:%M:%S");
			m_pChartFX->GetAxis()->GetItem(AXIS_X)->Label[m_nCurrentPositionInSerie] = szDate.AllocSysString();
			m_bFirstPassDone = true;
		}
		else
		{
			szDate  = m_CollectorTimestamp.Format("%H:%M:%S");
			m_pChartFX->GetAxis()->GetItem(AXIS_X)->Label[m_nCurrentPositionInSerie] = _T("");
			m_pChartFX->GetAxis()->GetItem(AXIS_X)->Label[m_nCurrentPositionInSerie] = szDate.AllocSysString();
		}   

		// set the stripe
		m_pChartFX->OpenDataEx((enum CfxCod)(COD_STRIPES), 1,0);
		ICfxStripePtr pStripe;
		pStripe = m_pChartFX->GetStripe()->GetItem(0);
		pStripe->From = m_nCurrentPositionInSerie - 1 ;
		pStripe->To = m_nCurrentPositionInSerie + 1;
		pStripe->Color = LIGHT_GREY;
		pStripe->Axis = AXIS_X;
		m_pChartFX->CloseData((enum CfxCod)(COD_STRIPES));
			
		enum tagElementState state = (enum tagElementState)pServer->GetState(); 
		if(!pServer->Connected)
		{
			 // set the Constant Line to disconnected
			m_pChartFX->OpenDataEx((enum CfxCod)(COD_CONSTANTS), 1,0);
			ICfxConstPtr pConstantLine;
			pConstantLine = m_pChartFX->GetConstantLine()->GetItem(0);
			pConstantLine->Label = "Server is disconnected";
			pConstantLine->Value = m_nCurrentPositionInSerie;
			pConstantLine->Style = CC_COLORTEXT;
			m_pChartFX->CloseData((enum CfxCod)(COD_CONSTANTS));
		}
		else
		{
			// set the Constant Line Hidden
			m_pChartFX->OpenDataEx((enum CfxCod)(COD_CONSTANTS), 1,0);
			ICfxConstPtr pConstantLine;
			pConstantLine = m_pChartFX->GetConstantLine()->GetItem(0);
			pConstantLine->Style = CC_HIDE;
			m_pChartFX->CloseData((enum CfxCod)(COD_CONSTANTS));
		}

		m_pChartFX->OpenDataEx((enum CfxCod)(COD_VALUES ), COD_UNCHANGE ,COD_UNCHANGE);
		for (std::list<CCounterInfo>::iterator it = m_listCounterInfo.begin();
			 it != m_listCounterInfo.end(); it++)
		{
			// Find group with id = it->m_nLgGroupId
			TDMFOBJECTSLib::IReplicationGroupPtr pRG;
			for (int i = pDoc->GetSelectedServer()->ReplicationGroupCount - 1; i >= 0; i--)
			{
				if (pDoc->GetSelectedServer()->GetReplicationGroup(i)->GroupNumber == it->m_nLgGroupId)
				{
  					pRG = pDoc->GetSelectedServer()->GetReplicationGroup(i);
					break;
				}
			}

			CString strStat;
			if (pRG != NULL)
			{
				double dVal = 0;
				enum tagElementState state = (enum tagElementState)pRG->GetState();
				if( !pRG->Parent->Connected || 
					(state != TDMFOBJECTSLib::ElementError) &&
					(state != TDMFOBJECTSLib::ElementWarning) &&
					(state != TDMFOBJECTSLib::ElementOk))
				{
				  dVal = CHART_HIDDEN;
				}
				else
				{
					// Get the selected stat (it->m_ePerfData)
					
					switch(it->m_ePerfData)
					{
					case CCounterInfo::eBABEntries:
						strStat = _T("Entries in BAB ");
						dVal = pRG->BABEntries;
						break;
					case CCounterInfo::ePctBABFull:
						dVal = pRG->PctBAB;
						strStat = _T("% BAB Full ");
						break;
					case CCounterInfo::ePctDone:
						dVal = pRG->PctDone;
						strStat = _T("% Done ");
						break;
					case CCounterInfo::eReadBytes:
						dVal = atof(OLE2A(pRG->ReadKbps));
						strStat = _T("Read Bytes ");
						break;
					case CCounterInfo::eWriteBytes:
						dVal = atof(OLE2A(pRG->WriteKbps));
						strStat = _T("Write Bytes ");
						break;
					case CCounterInfo::eActualNet:
						dVal = atof(OLE2A(pRG->ActualNet));
						strStat = _T("Actual Net Bytes ");
						break;
					case CCounterInfo::eEffectiveNet:
						dVal = atof(OLE2A(pRG->EffectiveNet));
						strStat = _T("Effective Net Bytes ");
						break;
						//case CCounterInfo::eBABSectors:
						//dVal = atof(OLE2A(pRG->BABSectors));
						//break;
					}
				}

 				if(_bstr_t(it->m_OldTimeStamp) != pRG->StateTimeStamp)
				{
				   m_pChartFX->ValueEx[(short)it->m_SerieNbr][m_nCurrentPositionInSerie] = dVal;
				   it->m_NumberOfSameConcurrentTimeStamp = 0;
				}
				else
				{
					if(it->m_NumberOfSameConcurrentTimeStamp > 1)
					{
						m_pChartFX->ValueEx[(short)it->m_SerieNbr][m_nCurrentPositionInSerie] = CHART_HIDDEN;
						it->m_NumberOfSameConcurrentTimeStamp = 0;
					}
					else
					{
					  m_pChartFX->ValueEx[(short)it->m_SerieNbr][m_nCurrentPositionInSerie] = dVal;
					}
					it->m_NumberOfSameConcurrentTimeStamp += 1;
				}

				it->m_OldTimeStamp = (BSTR)pRG->StateTimeStamp;
          
			}
			else
			{  
			  m_pChartFX->ValueEx[(short)it->m_SerieNbr][m_nCurrentPositionInSerie] = CHART_HIDDEN;
			}
			
		}
		
		// Close the channel forcing scroll
		m_pChartFX->CloseData((enum CfxCod)(COD_VALUES | COD_SMOOTH));
		
		m_nCurrentPositionInSerie++;

		if(m_nCurrentPositionInSerie >= m_nMaxPointPerSerie)
		{
		   m_bFirstPassDone = TRUE;
		   m_nCurrentPositionInSerie = 0;
		}
	}
	if(m_pChartFX->GetSeries()->Count > 0)
	{
		if(m_ChartFXSetting.m_bDataEditorVisible)
		{
			m_pChartFX->GetDataEditorObj()->Visible = -1;
			m_pChartFX->GetToolBarObj()->GetCommands()->GetItem(CFX_ID_DATAEDITOR)->Checked = TRUE;
		}
		else
		{
			m_pChartFX->GetDataEditorObj()->Visible = FALSE;
			m_pChartFX->GetToolBarObj()->GetCommands()->GetItem(CFX_ID_DATAEDITOR)->Checked = FALSE;
		}
	}
	else
	{
		m_pChartFX->GetDataEditorObj()->Visible = FALSE;
		m_pChartFX->GetToolBarObj()->GetCommands()->GetItem(CFX_ID_DATAEDITOR)->Checked = FALSE;
	}
	
}

BEGIN_EVENTSINK_MAP(CServerPerformanceMonitorPage, CPropertyPage)
    //{{AFX_EVENTSINK_MAP(CServerPerformanceMonitorPage)
	ON_EVENT(CServerPerformanceMonitorPage, IDC_CHART1, 27 /* UserCommand */, OnUserCommandChart1, VTS_I4 VTS_I4 VTS_PI2)
	ON_EVENT(CServerPerformanceMonitorPage, IDC_CHART1, 17 /* InternalCommand */, OnInternalCommandChart1, VTS_I4 VTS_I4 VTS_PI2)
	//}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()

void CServerPerformanceMonitorPage::OnUserCommandChart1(long wParam, long lParam, short FAR* nRes) 
{
	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();

	switch (wParam)
	{
	case 1:
		{
            m_bSelectDialogInUse = TRUE;

            TDMFOBJECTSLib::IServerPtr pServer = pDoc->GetSelectedServer();
            if(pServer)
            {
               
                CSelectCounterDlg SelectCounterDlg(this);
	 		    SelectCounterDlg.m_pServer = pServer;
                SelectCounterDlg.m_Edit_Time = m_nTimeSlice;

                if( SelectCounterDlg.m_pServer->ReplicationGroupCount <= 0)
                {
                    CString strMsg;
                    strMsg.Format("The server '%s' has no group assigned to it ",(LPCTSTR)SelectCounterDlg.m_pServer->Name);
		            AfxMessageBox(strMsg, MB_OK | MB_ICONSTOP);
           
                }
                else
                {
                    if(!m_bAllGroups)
                        SelectCounterDlg.m_pRG = m_pRG;

                    if(SelectCounterDlg.DoModal() == IDOK)
                    {
                	    FillTheListOfCounters();
			            m_nTimeSlice = SelectCounterDlg.m_Edit_Time;
                        m_nMaxPointPerSerie = m_nTimeSlice * 4; /*60 / (TimeOutDuration / 1000);*/
                        AdjustTheYAxisScale();
                        SetXAxisFormat();
                        m_bFirstPassDone = FALSE; 
                        m_bSelectDialogInUse = FALSE;
					    UpdateTheValueInChartFX();
                  }
                }
            }
		
            m_bSelectDialogInUse = FALSE;
 
		}
		break;

	case 2:
        m_bFirstPassDone = FALSE;
        m_pChartFX->ClearData(CD_DATA);
		UpdateTheValueInChartFX();
 		break;
    
    case 3:
        ChangeScaleType();
		break;
    case 4:
        m_bPause = (!m_pChartFX->GetToolBarObj()->GetCommands()->GetItem(4)->GetChecked()? FALSE : TRUE);
      	break;
	case 5:
		{
			CTimeRangeDlg TimeRangeDlg;
			TimeRangeDlg.m_value = m_nTimeSlice;
			if(TimeRangeDlg.DoModal() == IDOK)
			{
   				m_nTimeSlice = TimeRangeDlg.m_value;
				m_nMaxPointPerSerie = m_nTimeSlice * 4;
				m_bFirstPassDone = FALSE; 
				SetXAxisFormat();
				UpdateTheValueInChartFX();
		   }
		}
      	break;
   	}
}

//Command from the predefined button in toolbar
void CServerPerformanceMonitorPage::OnInternalCommandChart1(long wParam, long lParam, short FAR* nRes) 
{
	switch (wParam)
	{
		case CFX_ID_DATAEDITOR:
		{
			m_ChartFXSetting.m_bDataEditorVisible	=  (!m_pChartFX->GetToolBarObj()->GetCommands()->GetItem(CFX_ID_DATAEDITOR)->GetChecked()? FALSE : -1);
        }
		break;
	}
}

void CServerPerformanceMonitorPage::ChangeScaleType()
{
    if( m_pChartFX->GetAxis()->Item[AXIS_Y]->LogBase == 0)
    { 
        m_pChartFX->GetAxis()->Item[AXIS_Y]->LogBase = 10;
    }
    else
    { 
        m_pChartFX->GetAxis()->Item[AXIS_Y]->LogBase = 0;
    }
}

// set log if necessary
void CServerPerformanceMonitorPage::AdjustTheYAxisScale()
{
   
  	for (std::list<CCounterInfo>::iterator it = m_listCounterInfo.begin();
			 it != m_listCounterInfo.end(); it++)
		{
			if(it->m_ePerfData == CCounterInfo::eBABEntries ||
                it->m_ePerfData == CCounterInfo::eReadBytes ||
                it->m_ePerfData == CCounterInfo::eWriteBytes ||
                it->m_ePerfData == CCounterInfo::eActualNet ||
                it->m_ePerfData == CCounterInfo::eEffectiveNet)
                {
                    m_pChartFX->GetAxis()->Item[AXIS_Y]->LogBase = 10;
                    m_pChartFX->GetToolBarObj()->GetCommands()->GetItem(3)->Checked = -1;
                    return;
                }
	    }
         m_pChartFX->GetAxis()->Item[AXIS_Y]->LogBase = 0;
         m_pChartFX->GetAxis()->Item[AXIS_Y]->AutoScale = -1;
 
}

void CServerPerformanceMonitorPage::SetXAxisFormat()
{

	m_pChartFX->GetAxis()->GetItem(AXIS_X)->Format		= "%H:%M:%S ";
	m_pChartFX->GetAxis()->GetItem(AXIS_X)->LabelAngle	= 90;
    m_pChartFX->GetAxis()->GetItem(AXIS_X)->STEP		= m_nTimeSlice / 10 ;
    m_pChartFX->GetAxis()->GetItem(AXIS_X)->Decimals	= 0;
 
}

void CServerPerformanceMonitorPage::SetYAxisFormat()
{

	m_pChartFX->GetAxis()->GetItem(AXIS_Y)->Decimals	= 0;
	m_pChartFX->GetAxis()->GetItem(AXIS_Y)->STEP		= 0;
	m_pChartFX->GetAxis()->GetItem(AXIS_Y)->Title		= _T("");
	m_pChartFX->GetAxis()->GetItem(AXIS_Y)->AutoScale	= -1;
 
}

void CServerPerformanceMonitorPage::InitialyzeTheMonitor()
{
 
	m_pChartFX->Chart3D								= m_ChartFXSetting.m_b3DVisible;
	m_pChartFX->Gallery								= m_ChartFXSetting.m_GraphStyle;
	m_pChartFX->Grid								= m_ChartFXSetting.m_GridStyle;
	m_pChartFX->Zoom								= m_ChartFXSetting.m_bZoom;
	m_pChartFX->GetAxis()->Item[AXIS_Y]->LogBase	= m_ChartFXSetting.m_LogBase;

	m_pChartFX->GetDataEditorObj()->PutVisible(m_ChartFXSetting.m_bDataEditorVisible);
    m_pChartFX->GetToolBarObj()->GetCommands()->GetItem(CFX_ID_DATAEDITOR)->PutChecked(m_ChartFXSetting.m_bDataEditorVisible);

  	m_pChartFX->GetDataEditorObj()->put_Left(m_ChartFXSetting.m_DataEditorLeft);
    m_pChartFX->GetDataEditorObj()->PutTop(m_ChartFXSetting.m_DataEditorTop);
    m_pChartFX->GetDataEditorObj()->PutWidth(m_ChartFXSetting.m_DataEditorWidth);
    m_pChartFX->GetDataEditorObj()->PutHeight(m_ChartFXSetting.m_DataEditorHeight);
    m_pChartFX->GetDataEditorObj()->PutDocked(m_ChartFXSetting.m_DataEditorDocked);
	
 	if(m_ChartFXSetting.m_LogBase == 0)
		m_pChartFX->GetToolBarObj()->GetCommands()->GetItem(3)->Checked = FALSE;
	else
       	m_pChartFX->GetToolBarObj()->GetCommands()->GetItem(3)->Checked = -1;
}

void CServerPerformanceMonitorPage::SaveToolbarSettings()
{
	if( m_pChartFX->GetDataEditorObj()->GetVisible())
	{
		if(m_pChartFX->GetSeries()->Count > 0)
		{
		
			if(m_ChartFXSetting.m_bDataEditorVisible)
			{
				m_ChartFXSetting.m_DataEditorLeft       = m_pChartFX->GetDataEditorObj()->Left;
				m_ChartFXSetting.m_DataEditorTop        = m_pChartFX->GetDataEditorObj()->Top;
				m_ChartFXSetting.m_DataEditorWidth      = m_pChartFX->GetDataEditorObj()->Width;

		
				CRect rect;
				m_ChartFX.GetWindowRect(&rect);
				int nHeight = rect.bottom -  rect.top ;

				if(m_pChartFX->GetDataEditorObj()->Height > (nHeight / 2))
				{
				  m_ChartFXSetting.m_DataEditorHeight  = nHeight / 2;
				}
				else if(m_pChartFX->GetDataEditorObj()->Height < 60)
					{
					  m_pChartFX->GetDataEditorObj()->Height = 60;
					}
					else
					{
						m_ChartFXSetting.m_DataEditorHeight  = m_pChartFX->GetDataEditorObj()->Height;
					}
				m_ChartFXSetting.m_DataEditorDocked     = m_pChartFX->GetDataEditorObj()->Docked;
			}
		}
	}
 
   	m_ChartFXSetting.m_b3DVisible			= m_pChartFX->Chart3D;
	m_ChartFXSetting.m_GraphStyle			= m_pChartFX->Gallery;
    m_ChartFXSetting.m_GridStyle			= m_pChartFX->Grid;
	m_ChartFXSetting.m_bZoom				= m_pChartFX->Zoom;
	m_ChartFXSetting.m_LogBase				= m_pChartFX->GetAxis()->Item[AXIS_Y]->LogBase;

}

void CServerPerformanceMonitorPage::FillTheListOfCounters()
{
	m_listCounterInfo.clear();
	m_pChartFX->ClearData(CD_DATA);
	m_bFirstPassDone = FALSE;       
	int nStatCount = 0;


	CTdmfCommonGuiDoc* pDoc = (CTdmfCommonGuiDoc*)((CView*)(GetParent()->GetParent()))->GetDocument();
	TDMFOBJECTSLib::IServerPtr pServer = pDoc->GetSelectedServer();

	//Set the array of statistics
	CUIntArray *pArrayStats;
	if(m_mapServerIDToArrayOfStatistics.Lookup((WORD)pServer->Key,( CObject*& ) pArrayStats) == 0)
    {
       m_parySelStats = &m_aryDefaultsSelStats;
    }
    else
    {
       m_parySelStats = pArrayStats;
    }
    nStatCount = m_parySelStats->GetSize();
 
    //Set the array of groups
    CUIntArray *parySelGroups;
    if(m_mapServerIDToArrayOfGroupNumber.Lookup((WORD)pServer->Key,( CObject*& ) parySelGroups) == 0)
    {
       m_parySelGroups = NULL;
    }
    else
    {
       m_parySelGroups = parySelGroups;
    }
 
    if(!m_bAllGroups)
	{
	 //For specific groups

 		for(int j = 0; j < nStatCount; j++)
		{ 
			if (m_pRG != NULL)
			{
				CCounterInfo CounterInfo;
				CounterInfo.m_nLgGroupId = m_pRG->GroupNumber;
				CounterInfo.m_strName = (BSTR)m_pRG->Name;
				CounterInfo.m_ePerfData =  (enum CCounterInfo::Stats)m_parySelStats->GetAt(j);
				m_listCounterInfo.push_back(CounterInfo);
			}
		}
	}
	else
	{
		if(m_parySelGroups == NULL)
		{
		/*	//take all the groups
			TDMFOBJECTSLib::IServerPtr pServer = pDoc->GetSelectedServer();
			if (pServer != NULL)
			{
				for (int nIndex = 0; nIndex < pServer->ReplicationGroupCount; nIndex++)
				{
					TDMFOBJECTSLib::IReplicationGroupPtr pGroup = pServer->GetReplicationGroup(nIndex);
					for(int j = 0; j < nStatCount; j++)
					{
					   CCounterInfo CounterInfo;
					   CounterInfo.m_nLgGroupId      = pGroup->GroupNumber;
					   CounterInfo.m_strName         = (BSTR)pGroup->Name;
					   CounterInfo.m_ePerfData       =  (enum CCounterInfo::Stats)m_parySelStats->GetAt(j);
					   m_listCounterInfo.push_back(CounterInfo);
					}
				}
			}*/
		}
		else
		{
			BOOL bIsSource;
			int nGroupNumber;
			int nGroupsCount = m_parySelGroups->GetSize();
			for(int i = 0; i < nGroupsCount; i++)
			{
				nGroupNumber = m_parySelGroups->GetAt(i);
				if(nGroupNumber >= 10000000)
				{
					bIsSource = FALSE;
					nGroupNumber = nGroupNumber - 10000000;
				}
				else
				{
					bIsSource = TRUE;
				}

				for (int nIndex = 0; nIndex < pServer->ReplicationGroupCount; nIndex++)
				{
					TDMFOBJECTSLib::IReplicationGroupPtr pGroup = pServer->GetReplicationGroup(nIndex);

					if(pGroup != NULL)
					{
						if(pGroup->GroupNumber == nGroupNumber)
						{
							if(pGroup->IsSource == bIsSource)
							{
								for(int j = 0; j < nStatCount; j++)
								{
								   CCounterInfo CounterInfo;
								   CounterInfo.m_nLgGroupId      = pGroup->GroupNumber;
								   CounterInfo.m_strName         = (BSTR)pGroup->Name;
								   CounterInfo.m_ePerfData       =  (enum CCounterInfo::Stats)m_parySelStats->GetAt(j);
								   m_listCounterInfo.push_back(CounterInfo);
								}
								break;
							}
							
						}
					}
				}
			}
		}
	}
}

void CServerPerformanceMonitorPage::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CPropertyPage::OnShowWindow(bShow, nStatus);
	
    if(!bShow)
	{
	  SaveToolbarSettings();
	}
  
}

void CServerPerformanceMonitorPage::InitialyzeTheDefaultArrayOfSelectedStats()
{
    m_aryDefaultsSelStats.RemoveAll();
/*	m_aryDefaultsSelStats.Add( CCounterInfo::ePctDone);
	m_aryDefaultsSelStats.Add( CCounterInfo::eReadBytes);
	m_aryDefaultsSelStats.Add( CCounterInfo::eWriteBytes);
	m_aryDefaultsSelStats.Add( CCounterInfo::eActualNet);
	m_aryDefaultsSelStats.Add( CCounterInfo::eEffectiveNet);
	m_aryDefaultsSelStats.Add( CCounterInfo::eBABEntries);
	m_aryDefaultsSelStats.Add( CCounterInfo::ePctBABFull);
	*/
}


void CServerPerformanceMonitorPage::OnMouseMove(UINT nFlags, CPoint point) 
{
	CRect rect;
	m_ChartFX.GetWindowRect(&rect);
    int nHeight = rect.bottom -  rect.top ;

	if(m_pChartFX->GetDataEditorObj()->Height > nHeight / 2)
	{
      m_pChartFX->GetDataEditorObj()->Height = nHeight / 2;
	}
	else if(m_pChartFX->GetDataEditorObj()->Height < 60)
			{
			  m_pChartFX->GetDataEditorObj()->Height = 60;
			}

	CPropertyPage::OnMouseMove(nFlags, point);
}



