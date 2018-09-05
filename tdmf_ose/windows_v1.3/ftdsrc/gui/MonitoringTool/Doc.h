// Doc.h : interface of the Doc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_DOC_H__2A8822D9_F2DB_4388_834B_6DF4CCD30394__INCLUDED_)
#define AFX_DOC_H__2A8822D9_F2DB_4388_834B_6DF4CCD30394__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

enum enumItemType
{
   eDOMAIN, eMACHINE, eLGGROUP
};

struct SIPAddress
{
   BYTE nField0;
   BYTE nField1;
   BYTE nField2;
   BYTE nField3; 
};

struct STempAndDummyServerInfo
{
   int nNodeSequence;
   int nNodeType;
   SIPAddress ip;
   int nBABUsed;
};

struct DocServerInfo
{
   bool bValid;
   SIPAddress IPAddress;
   CString strName;
   CString strOS;
   CString strDomain;
   CString strTDMFVersion;
   CString strState;
   CString strPStore;
   CString strJournal;
   bool IsSource;
   bool IsTarget;
   int nPort;
   int nTotVolumes;
   int nReplicatedVols;
   int nActiveReplSets;
   int nFreeSpace;
   int nBABSize;
   int nBABUsed;
};

class Doc : public CDocument
{
      // t e m p o r a r y:      
      STempAndDummyServerInfo m_aTDSI[765];
      int m_nTotNodes;
      
      DocServerInfo m_ServerInfo;
   protected: // create from serialization only
      Doc();
      DECLARE_DYNCREATE(Doc)

   // Attributes
   public:
      BOOL GetAllDomainNames(CTreeCtrl& TreeCtrl);
      // t e m p o r a r y:      
      const DocServerInfo& GetServerInfo()
      {
         return m_ServerInfo;
      }
      void GetServerInfo(CTreeCtrl& TreeCtrl);


   // Operations
   public:

   // Overrides
      // ClassWizard generated virtual function overrides
      //{{AFX_VIRTUAL(Doc)
      public:
      virtual BOOL OnNewDocument();
      virtual void Serialize(CArchive& ar);
      //}}AFX_VIRTUAL

   // Implementation
   public:
      virtual ~Doc();
   #ifdef _DEBUG
      virtual void AssertValid() const;
      virtual void Dump(CDumpContext& dc) const;
   #endif

   protected:

   // Generated message map functions
   protected:
      //{{AFX_MSG(Doc)
         // NOTE - the ClassWizard will add and remove member functions here.
         //    DO NOT EDIT what you see in these blocks of generated code !
      //}}AFX_MSG
      DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DOC_H__2A8822D9_F2DB_4388_834B_6DF4CCD30394__INCLUDED_)
