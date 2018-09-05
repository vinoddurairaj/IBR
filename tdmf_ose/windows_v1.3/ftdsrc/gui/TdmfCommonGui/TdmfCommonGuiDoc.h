// TdmfCommonGuiDoc.h : interface of the CTdmfCommonGuiDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_TDMFCOMMONGUIDOC_H__5A28E786_62E9_495D_B029_F1CC96DFB21D__INCLUDED_)
#define AFX_TDMFCOMMONGUIDOC_H__5A28E786_62E9_495D_B029_F1CC96DFB21D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000




class CTdmfCommonGuiDoc : public CDocument
{
protected: // create from serialization only
	CTdmfCommonGuiDoc();
	DECLARE_DYNCREATE(CTdmfCommonGuiDoc)

protected:
	TDMFOBJECTSLib::IDomainPtr           m_pDomainSelected;
	TDMFOBJECTSLib::IServerPtr           m_pServerSelected;
	TDMFOBJECTSLib::IReplicationGroupPtr m_pRGSelected;

	BOOL m_bConnected;
	BOOL m_bReadOnly;

public:
	BOOL m_bShowServersWarnings;

// Attributes
public:
	TDMFOBJECTSLib::ISystemPtr m_pSystem;

	BOOL GetConnectedFlag() {return m_bConnected;}
	void SetConnectedFlag(BOOL bConnected)
	{
		m_bConnected = bConnected;

		if ((m_bConnected) && (m_pSystem->UserRole != _bstr_t("User")))
		{
			m_bReadOnly = FALSE;
		}
		else
		{
			m_bReadOnly = TRUE;
		}
	}
	BOOL GetReadOnlyFlag() {return m_bReadOnly;}

	std::map<std::string, CAdapt<TDMFOBJECTSLib::IDeviceListPtr> > m_mapDeviceList;

	void SelectDomain(TDMFOBJECTSLib::IDomain* pDomain) {m_pDomainSelected = pDomain;}
	TDMFOBJECTSLib::IDomainPtr& GetSelectedDomain() {return m_pDomainSelected;}

	void SelectServer(TDMFOBJECTSLib::IServer* pServer)
	{
		// Before we change server, cancel performance notifications
		if ((m_pServerSelected != NULL) && (TRUE == m_pServerSelected->PerformanceNotifications))
		{
			m_pServerSelected->PerformanceNotifications = FALSE;
		}

		m_pServerSelected = pServer;

		if (pServer != NULL)
		{
			// Request Notification for performance datas
			m_pServerSelected->PerformanceNotifications = TRUE;
		}
	}
	TDMFOBJECTSLib::IServerPtr& GetSelectedServer() {return m_pServerSelected;}

	void SelectReplicationGroup(TDMFOBJECTSLib::IReplicationGroup* pRG) {m_pRGSelected = pRG;}
	TDMFOBJECTSLib::IReplicationGroupPtr& GetSelectedReplicationGroup() {return m_pRGSelected;}

	BOOL CTdmfCommonGuiDoc::UserIsAdministrator();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTdmfCommonGuiDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual void OnCloseDocument();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTdmfCommonGuiDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
public:
	//{{AFX_MSG(CTdmfCommonGuiDoc)
	afx_msg void OnToolsRefresh();
	afx_msg void OnToolsOptions();
	afx_msg void OnUpdateToolsOptions(CCmdUI* pCmdUI);
	afx_msg void OnUpdateToolsRefresh(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileConnect(CCmdUI* pCmdUI);
	afx_msg void OnFileConnect();
	afx_msg void OnFileDisconnect();
	afx_msg void OnUpdateFileDisconnect(CCmdUI* pCmdUI);
	afx_msg void OnToolsShowServersWarnings();
	afx_msg void OnToolsShowserverswarning();
	afx_msg void OnUpdateToolsShowserverswarning(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TDMFCOMMONGUIDOC_H__5A28E786_62E9_495D_B029_F1CC96DFB21D__INCLUDED_)
