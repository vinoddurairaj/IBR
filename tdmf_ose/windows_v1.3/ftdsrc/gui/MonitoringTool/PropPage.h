// PropPage.h : header file
//

#ifndef __PROPPAGE_H__
#define __PROPPAGE_H__

#include "GenericPropPage.h"
#include "Resource.h"
#include "TDMFEventView.h"

/////////////////////////////////////////////////////////////////////////////
// PropPg_Host dialog

/////////////////////////////////////////////////////////////////////////////
// PropPg_Replications dialog

class PropPg_Replications : public GenericPropPage
{
	   DECLARE_DYNCREATE(PropPg_Replications)
	   enum
      {
         eIDS = IDS_TAB_REPLICATIONS,
         eROWS = 2,
         eCOLS = 1
      };
      SPanelData m_aPanel[eROWS][eCOLS];
		CMenu m_Menu;

   public:
	   PropPg_Replications();
	   ~PropPg_Replications();
      BOOL Construct(PropSheet* pParentSheet);

   // Dialog Data
	   //{{AFX_DATA(PropPg_Replications)
		   // NOTE - ClassWizard will add data members here.
		   //    DO NOT EDIT what you see in these blocks of generated code !
	   //}}AFX_DATA


   // Overrides
	   // ClassWizard generate virtual function overrides
	   //{{AFX_VIRTUAL(PropPg_Replications)
		public:
      virtual BOOL OnSetActive();  
		protected:
	   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	   //}}AFX_VIRTUAL

   // Implementation
   protected:
	   // Generated message map functions
	   //{{AFX_MSG(PropPg_Replications)
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	//}}AFX_MSG
	   DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// PropPg_Events dialog

class PropPg_Events : public GenericPropPage
{
	   DECLARE_DYNCREATE(PropPg_Events)
	   enum
      {
         eIDS = IDS_TAB_EVENTS,
         eROWS = 2,
         eCOLS = 1
      };
      SPanelData m_aPanel[eROWS][eCOLS];

   public:
	   PropPg_Events();
	   ~PropPg_Events();
      BOOL Construct(PropSheet* pParentSheet);

   // Dialog Data
	   //{{AFX_DATA(PropPg_Events)
		   // NOTE - ClassWizard will add data members here.
		   //    DO NOT EDIT what you see in these blocks of generated code !
	   //}}AFX_DATA


   // Overrides
	   // ClassWizard generate virtual function overrides
	   //{{AFX_VIRTUAL(PropPg_Events)
		public:
      virtual BOOL OnSetActive();
		protected:
	   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	   //}}AFX_VIRTUAL

   // Implementation
   protected:
	   // Generated message map functions
	   //{{AFX_MSG(PropPg_Events)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	//}}AFX_MSG
	   DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// PropPg_DetailsServer dialog

class PropPg_DetailsServer : public GenericPropPage
{
	   DECLARE_DYNCREATE(PropPg_DetailsServer)
	   enum
      {
         eIDS = IDS_TAB_DETAILS,
         eROWS = 2,
         eCOLS = 1
      };
      SPanelData m_aPanel[eROWS][eCOLS];

   public:
	   PropPg_DetailsServer();
	   ~PropPg_DetailsServer();
      BOOL Construct(PropSheet* pParentSheet);

   // Dialog Data
	   //{{AFX_DATA(PropPg_DetailsServer)
		   // NOTE - ClassWizard will add data members here.
		   //    DO NOT EDIT what you see in these blocks of generated code !
	   //}}AFX_DATA


   // Overrides
	   // ClassWizard generate virtual function overrides
	   //{{AFX_VIRTUAL(PropPg_DetailsServer)
		public:
      virtual BOOL OnSetActive();
	   protected:
	   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	   //}}AFX_VIRTUAL

   // Implementation
   protected:
	   // Generated message map functions
	   //{{AFX_MSG(PropPg_DetailsServer)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	//}}AFX_MSG
	   DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// PropPg_ReplicationGroups dialog

class PropPg_ReplicationGroups : public GenericPropPage
{
	   DECLARE_DYNCREATE(PropPg_ReplicationGroups)
	   enum
      {
         eIDS = IDS_TAB_REPLICATIONGROUP,
         eROWS = 2,
         eCOLS = 1
      };
      SPanelData m_aPanel[eROWS][eCOLS];

   public:
	   PropPg_ReplicationGroups();
	   ~PropPg_ReplicationGroups();
      BOOL Construct(PropSheet* pParentSheet);

   // Dialog Data
	   //{{AFX_DATA(PropPg_ReplicationGroups)
		   // NOTE - ClassWizard will add data members here.
		   //    DO NOT EDIT what you see in these blocks of generated code !
	   //}}AFX_DATA


   // Overrides
	   // ClassWizard generate virtual function overrides
	   //{{AFX_VIRTUAL(PropPg_ReplicationGroups)
		public:
      virtual BOOL OnSetActive();
	   protected:
	   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	   //}}AFX_VIRTUAL

   // Implementation
   protected:
	   // Generated message map functions
	   //{{AFX_MSG(PropPg_ReplicationGroups)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	//}}AFX_MSG
	   DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// PropPg_Monitoring dialog

class PropPg_Monitoring : public GenericPropPage
{
	   DECLARE_DYNCREATE(PropPg_Monitoring)
	   enum
      {
         eIDS = IDS_TAB_MONITORING,
         eROWS = 3,
         eCOLS = 1
      };
      SPanelData m_aPanel[eROWS][eCOLS];

   public:
	   PropPg_Monitoring();
	   ~PropPg_Monitoring();
      BOOL Construct(PropSheet* pParentSheet);

   // Dialog Data
	   //{{AFX_DATA(PropPg_Monitoring)
		   // NOTE - ClassWizard will add data members here.
		   //    DO NOT EDIT what you see in these blocks of generated code !
	   //}}AFX_DATA


   // Overrides
	   // ClassWizard generate virtual function overrides
	   //{{AFX_VIRTUAL(PropPg_Monitoring)
		public:
      virtual BOOL OnSetActive();
	   protected:
	   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	   //}}AFX_VIRTUAL

   // Implementation
   protected:
	   // Generated message map functions
	   //{{AFX_MSG(PropPg_Monitoring)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	//}}AFX_MSG
	   DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// PropPg_Administration dialog

class PropPg_Administration : public GenericPropPage
{
	   DECLARE_DYNCREATE(PropPg_Administration)
	   enum
      {
         eIDS = IDS_TAB_ADMINISTRATION,
         eROWS = 2,
         eCOLS = 1
      };
      SPanelData m_aPanel[eROWS][eCOLS];

   public:
	   PropPg_Administration();
	   ~PropPg_Administration();
      BOOL Construct(PropSheet* pParentSheet);

   // Dialog Data
	   //{{AFX_DATA(PropPg_Administration)
		   // NOTE - ClassWizard will add data members here.
		   //    DO NOT EDIT what you see in these blocks of generated code !
	   //}}AFX_DATA


   // Overrides
	   // ClassWizard generate virtual function overrides
	   //{{AFX_VIRTUAL(PropPg_Administration)
		public:
      virtual BOOL OnSetActive();
	   protected:
	   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	   //}}AFX_VIRTUAL

   // Implementation
   protected:
	   // Generated message map functions
	   //{{AFX_MSG(PropPg_Administration)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	//}}AFX_MSG
	   DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// PropPg_ServerEvents dialog

class PropPg_ServerEvents : public GenericPropPage
{
	   DECLARE_DYNCREATE(PropPg_ServerEvents)
	   enum
      {
         eIDS = IDS_TAB_SERVER_EVENT,
         eROWS = 2,
         eCOLS = 1
      };
      SPanelData m_aPanel[eROWS][eCOLS];

   public:
	   PropPg_ServerEvents();
	   ~PropPg_ServerEvents();
      BOOL Construct(PropSheet* pParentSheet);

   // Dialog Data
	   //{{AFX_DATA(PropPg_ServerEvents)
		   // NOTE - ClassWizard will add data members here.
		   //    DO NOT EDIT what you see in these blocks of generated code !
	   //}}AFX_DATA


   // Overrides
	   // ClassWizard generate virtual function overrides
	   //{{AFX_VIRTUAL(PropPg_ServerEvents)
		public:
      virtual BOOL OnSetActive();
		protected:
	   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	   //}}AFX_VIRTUAL

   // Implementation
   protected:
	   // Generated message map functions
	   //{{AFX_MSG(PropPg_ServerEvents)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	//}}AFX_MSG
	   DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// PropPg_Performance dialog

class PropPg_Performance : public GenericPropPage
{
	   DECLARE_DYNCREATE(PropPg_Performance)
	   enum
      {
         eIDS = IDS_TAB_PERFORMANCE,
         eROWS = 2,
         eCOLS = 1
      };
      SPanelData m_aPanel[eROWS][eCOLS];

   public:
	   PropPg_Performance();
	   ~PropPg_Performance();
      BOOL Construct(PropSheet* pParentSheet);

   // Dialog Data
	   //{{AFX_DATA(PropPg_Performance)
		   // NOTE - ClassWizard will add data members here.
		   //    DO NOT EDIT what you see in these blocks of generated code !
	   //}}AFX_DATA


   // Overrides
	   // ClassWizard generate virtual function overrides
	   //{{AFX_VIRTUAL(PropPg_Performance)
		public:
      virtual BOOL OnSetActive();
		protected:
	   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	   //}}AFX_VIRTUAL

   // Implementation
   protected:
	   // Generated message map functions
	   //{{AFX_MSG(PropPg_Performance)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	//}}AFX_MSG
	   DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// PropPg_ServerCommand dialog

class PropPg_ServerCommand : public GenericPropPage
{
	   DECLARE_DYNCREATE(PropPg_ServerCommand)
	   enum
      {
         eIDS = IDS_TAB_SERVER_COMMAND,
         eROWS = 2,
         eCOLS = 1
      };
      SPanelData m_aPanel[eROWS][eCOLS];

   public:
	   PropPg_ServerCommand();
	   ~PropPg_ServerCommand();
      BOOL Construct(PropSheet* pParentSheet);

   // Dialog Data
	   //{{AFX_DATA(PropPg_ServerCommand)
		   // NOTE - ClassWizard will add data members here.
		   //    DO NOT EDIT what you see in these blocks of generated code !
	   //}}AFX_DATA


   // Overrides
	   // ClassWizard generate virtual function overrides
	   //{{AFX_VIRTUAL(PropPg_ServerCommand)
		public:
      virtual BOOL OnSetActive();
		protected:
	   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	   //}}AFX_VIRTUAL

   // Implementation
   protected:
	   // Generated message map functions
	   //{{AFX_MSG(PropPg_ServerCommand)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	//}}AFX_MSG
	   DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// PropPg_ServerVolumes dialog

class PropPg_ServerVolumes : public GenericPropPage
{
	   DECLARE_DYNCREATE(PropPg_ServerVolumes)
	   enum
      {
         eIDS = IDS_TAB_VOLUMES,
         eROWS = 2,
         eCOLS = 1
      };
      SPanelData m_aPanel[eROWS][eCOLS];

   public:
	   PropPg_ServerVolumes();
	   ~PropPg_ServerVolumes();
      BOOL Construct(PropSheet* pParentSheet);

   // Dialog Data
	   //{{AFX_DATA(PropPg_ServerVolumes)
		   // NOTE - ClassWizard will add data members here.
		   //    DO NOT EDIT what you see in these blocks of generated code !
	   //}}AFX_DATA


   // Overrides
	   // ClassWizard generate virtual function overrides
	   //{{AFX_VIRTUAL(PropPg_ServerVolumes)
		public:
      virtual BOOL OnSetActive();
		protected:
	   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	   //}}AFX_VIRTUAL

   // Implementation
   protected:
	   // Generated message map functions
	   //{{AFX_MSG(PropPg_ServerVolumes)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	   DECLARE_MESSAGE_MAP()
};

class PropPg_Host : public GenericPropPage
{
	   DECLARE_DYNCREATE(PropPg_Host)
	   enum
      {
         eIDS = IDS_TAB_HOST,
         eROWS = 2,
         eCOLS = 1
      };
      SPanelData m_aPanel[eROWS][eCOLS];

   public:
	   PropPg_Host();
	   ~PropPg_Host();
      BOOL Construct(PropSheet* pParentSheet);

   // Dialog Data
	   //{{AFX_DATA(PropPg_Host)
		   // NOTE - ClassWizard will add data members here.
		   //    DO NOT EDIT what you see in these blocks of generated code !
	   //}}AFX_DATA


   // Overrides
	   // ClassWizard generate virtual function overrides
	   //{{AFX_VIRTUAL(PropPg_Host)
	   protected:
	   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	   //}}AFX_VIRTUAL

   // Implementation
   protected:
	   // Generated message map functions
	   //{{AFX_MSG(PropPg_Host)
         // virtual BOOL OnInitDialog();
	   //}}AFX_MSG
	   DECLARE_MESSAGE_MAP()
};

// PropPg_1 dialog

class PropPg_1 : public GenericPropPage
{
	   DECLARE_DYNCREATE(PropPg_1)
	   enum
      {
         eIDS = IDS_TAB_REPLICATIONS,
         eROWS = 2,
         eCOLS = 1
      };
      SPanelData m_aPanel[eROWS][eCOLS];

   public:
	   PropPg_1();
	   ~PropPg_1();
      BOOL Construct(PropSheet* pParentSheet);

   // Dialog Data
	   //{{AFX_DATA(PropPg_1)
		   // NOTE - ClassWizard will add data members here.
		   //    DO NOT EDIT what you see in these blocks of generated code !
	   //}}AFX_DATA


   // Overrides
	   // ClassWizard generate virtual function overrides
	   //{{AFX_VIRTUAL(PropPg_1)
	   protected:
	   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	   //}}AFX_VIRTUAL

   // Implementation
   protected:
	   // Generated message map functions
	   //{{AFX_MSG(PropPg_1)
         // virtual BOOL OnInitDialog();
	   //}}AFX_MSG
	   DECLARE_MESSAGE_MAP()
};




/////////////////////////////////////////////////////////////////////////////
// PropPg_Overview dialog

class PropPg_Overview : public GenericPropPage
{
	   DECLARE_DYNCREATE(PropPg_Overview)
	   enum
      {
         eIDS = IDS_TAB_OVVW,
         eROWS = 2,
         eCOLS = 1
      };
      SPanelData m_aPanel[eROWS][eCOLS];

   public:
	   PropPg_Overview();
	   ~PropPg_Overview();
      BOOL Construct(PropSheet* pParentSheet);

   // Dialog Data
	   //{{AFX_DATA(PropPg_Overview)
		   // NOTE - ClassWizard will add data members here.
		   //    DO NOT EDIT what you see in these blocks of generated code !
	   //}}AFX_DATA


   // Overrides
	   // ClassWizard generate virtual function overrides
	   //{{AFX_VIRTUAL(PropPg_Overview)
	   protected:
	   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	   //}}AFX_VIRTUAL

   // Implementation
   protected:
	   // Generated message map functions
	   //{{AFX_MSG(PropPg_Overview)
         // virtual BOOL OnInitDialog();
	   //}}AFX_MSG
	   DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// PropPg_5 dialog

class PropPg_5 : public GenericPropPage
{
	   DECLARE_DYNCREATE(PropPg_5)
	   enum
      {
         eIDS = 0,
         eROWS = 3,
         eCOLS = 5
      };
      SPanelData m_aPanel[eROWS][eCOLS];

   public:
	   PropPg_5();
	   ~PropPg_5();
      BOOL Construct(PropSheet* pParentSheet);

   // Dialog Data
	   //{{AFX_DATA(PropPg_5)
	   //enum { IDD = IDD_PROPPAGE5 };
		   // NOTE - ClassWizard will add data members here.
		   //    DO NOT EDIT what you see in these blocks of generated code !
	   //}}AFX_DATA


   // Overrides
	   // ClassWizard generate virtual function overrides
	   //{{AFX_VIRTUAL(PropPg_5)
	   protected:
	   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	   //}}AFX_VIRTUAL

   // Implementation
   protected:
	   // Generated message map functions
	   //{{AFX_MSG(PropPg_5)
         // virtual BOOL OnInitDialog();
	   //}}AFX_MSG
	   DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// PropPg_Monitor dialog

class PropPg_Monitor : public GenericPropPage
{
	   DECLARE_DYNCREATE(PropPg_Monitor)
	   enum
      {
         eIDS = IDS_TAB_MONITOR,
         eROWS = 3,
         eCOLS = 1
      };
      SPanelData m_aPanel[eROWS][eCOLS];

   public:
	   PropPg_Monitor();
	   ~PropPg_Monitor();
      BOOL Construct(PropSheet* pParentSheet);


   // Dialog Data
	   //{{AFX_DATA(PropPg_Monitor)
	   //enum { IDD = IDD_PROPPAGE5 };
		   // NOTE - ClassWizard will add data members here.
		   //    DO NOT EDIT what you see in these blocks of generated code !
	   //}}AFX_DATA


   // Overrides
	   // ClassWizard generate virtual function overrides
	   //{{AFX_VIRTUAL(PropPg_Monitor)
	   protected:
	   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	   //}}AFX_VIRTUAL

   // Implementation
   protected:
	   // Generated message map functions
	   //{{AFX_MSG(PropPg_Monitor)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG
	   DECLARE_MESSAGE_MAP()
};



#endif // __PROPPAGE_H__
