// Doc.cpp : implementation of the Doc class
//

#include "stdafx.h"
#include "TDMFGUI.h"

#include "Doc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Doc

IMPLEMENT_DYNCREATE(Doc, CDocument)

BEGIN_MESSAGE_MAP(Doc, CDocument)
	//{{AFX_MSG_MAP(Doc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Doc construction/destruction

Doc::Doc()
{
   m_ServerInfo.bValid = false;
}

Doc::~Doc()
{
}

BOOL Doc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// Doc serialization

void Doc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// Doc diagnostics

#ifdef _DEBUG
void Doc::AssertValid() const
{
	CDocument::AssertValid();
}

void Doc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// Doc commands

const static LPCTSTR aPostFix[50] =
{
   _T("01"), _T("02"), _T("03"), _T("04"), _T("05"),
   _T("06"), _T("07"), _T("08"), _T("09"), _T("10"),
   _T("11"), _T("12"), _T("13"), _T("14"), _T("15"),
   _T("16"), _T("17"), _T("18"), _T("19"), _T("20"),
   _T("21"), _T("22"), _T("23"), _T("24"), _T("25"),
   _T("26"), _T("27"), _T("28"), _T("29"), _T("30"),
   _T("31"), _T("32"), _T("33"), _T("34"), _T("35"),
   _T("36"), _T("37"), _T("38"), _T("39"), _T("40"),
   _T("41"), _T("42"), _T("43"), _T("44"), _T("45"),
   _T("46"), _T("47"), _T("48"), _T("49"), _T("50")
};

BOOL Doc::GetAllDomainNames(CTreeCtrl& TreeCtrl)
{
   TVINSERTSTRUCT tvi;
   ZeroMemory(&tvi.item, sizeof(TVITEM));
   tvi.item.mask = 0
               // | TVIF_CHILDREN
               // | TVIF_DI_SETITEM
               // | TVIF_HANDLE
               | TVIF_IMAGE
               | TVIF_PARAM
               | TVIF_SELECTEDIMAGE
               // | TVIF_STATE
               | TVIF_TEXT
               | 0;

   // tvi.item.hItem;
   // tvi.item.state;
   // tvi.item.stateMask;
   // tvi.item.pszText;
   // tvi.item.cchTextMax;
   // tvi.item.iImage = 0;
   // tvi.item.iSelectedImage;
   // tvi.item.cChildren;
   // tvi.item.lParam;

   static LPTSTR aptzColorNames[] =
   {
      _T("Red"),
      _T("Black"),
      _T("White"),
      _T("Green"),
      _T("Blue"),
      _T("Orange"),
      _T("Yellow"),
   };

   static LPTSTR aptzPlanetNames[] =
   {
      _T("Mercury"),
      _T("Venus"),
      _T("Earth"),
      _T("Marth"),
      _T("Jupiter"),
      _T("Saturn"),
      _T("Uranus"),
      _T("Neptune"),
      _T("Pluto"),
   };

   static LPTSTR aptzDwarfNames[] =
   {
      _T("Grumpy"),
      _T("Doc"),
      _T("Bashful"),
      _T("Sleepy"),
      _T("Sneezy"),
      _T("Happy"),
      _T("Dopey")
   };

   static LPTSTR aptzMouseNames[] =
   {
      _T("Mickey"),
      _T("Minnie"),
      _T("Morty"),
      _T("Ferdy"),
      _T("Millie"),
      _T("Mellody"),
   };

   struct SDummyDomainInfo
   {
      LPCTSTR ptzDomainName;
      int nTotHosts;
      int nPreAllocNames;
      LPTSTR *pptzNames;
   };

   const int nTotDomains = 6;
   const static SDummyDomainInfo aDomainInfo[nTotDomains] =
   {
      { _T("Accounting"),  10, sizeof(aptzColorNames)  / sizeof LPTSTR, aptzColorNames  },
      { _T("Engineering"), 10, sizeof(aptzPlanetNames) / sizeof LPTSTR, aptzPlanetNames },
      { _T("Logistics"),   10, sizeof(aptzMouseNames)  / sizeof LPTSTR, aptzMouseNames  },
      { _T("Marketing"),   10, sizeof(aptzDwarfNames)  / sizeof LPTSTR, aptzDwarfNames  },
      { _T("Production"),  50, 0, NULL },
      { _T("Operations"),  50, 0, NULL },
   };

   STempAndDummyServerInfo TDSI;
   TDSI.ip.nField0 = 192;
   TDSI.ip.nField1 = 168;
   TDSI.ip.nField2 = 0;
   TDSI.ip.nField3 = 0;
   TDSI.nNodeSequence = 0;

   CString str;
   for(int nDomain = 0; nDomain < nTotDomains; ++nDomain)
   {
      TDSI.nNodeType = eDOMAIN;
      m_aTDSI[TDSI.nNodeSequence] = TDSI;
      tvi.item.iImage = 0;
      tvi.item.iSelectedImage = 0;
      tvi.hParent = TVI_ROOT;
      tvi.hInsertAfter = TVI_ROOT;
      tvi.item.pszText = LPTSTR(aDomainInfo[nDomain].ptzDomainName);
      tvi.item.lParam = (LPARAM)TDSI.nNodeSequence++;
      HTREEITEM hItemDomain = TreeCtrl.InsertItem(&tvi);

      for(int nPrimary = 0; nPrimary < aDomainInfo[nDomain].nTotHosts; ++nPrimary)
      {
         if(nPrimary < aDomainInfo[nDomain].nPreAllocNames)
            str = aDomainInfo[nDomain].pptzNames[nPrimary];
         else
         {
            str = _T("Primary-");
            str += aPostFix[nDomain];
            str += _T('_');
            str += aPostFix[nPrimary];
         }

         if(!(++TDSI.ip.nField3 % 256))
            ++TDSI.ip.nField2;

         TDSI.nNodeType = eMACHINE;
         TDSI.nBABUsed = int(100.0 * (rand() / float(RAND_MAX)));
         m_aTDSI[TDSI.nNodeSequence] = TDSI;
         tvi.item.iImage = 1;
         tvi.item.iSelectedImage = 1;
         tvi.hParent = hItemDomain;
         tvi.hInsertAfter = TVI_LAST;
         tvi.item.pszText = LPTSTR(LPCTSTR(str));
         tvi.item.lParam = (LPARAM)TDSI.nNodeSequence++;
         HTREEITEM hItemPrimary = TreeCtrl.InsertItem(&tvi);

         // const static LPCTSTR aVolumeName[] =
         // {
         //    _T("C"), _T("D"), _T("E"), _T("F"), _T("G"),
         //    _T("H"), _T("I"), _T("J"), _T("K"), _T("L"),
         //    _T("M"), _T("N"), _T("O"), _T("P"), _T("Q"),
         //    _T("R"), _T("S"), _T("T"), _T("U"), _T("V"),
         //    _T("W"), _T("X"), _T("Y"), _T("Z")
         // };

         const int nMaxMirros = 8;
         const static LPCTSTR aMirrorName[nMaxMirros] =
         {
            _T(" | C: -> \\\\SomeHost\\Volume"), 
            _T(" | D: -> \\\\SomeHost\\Volume"), 
            _T(" | E: -> \\\\SomeHost\\Volume"), 
            _T(" | F: -> \\\\SomeHost\\Volume"), 
            _T(" | G: -> \\\\SomeHost\\Volume"), 
            _T(" | H: -> \\\\SomeHost\\Volume"), 
            _T(" | I: -> \\\\SomeHost\\Volume"), 
            _T(" | J: -> \\\\SomeHost\\Volume"), 
         };

         int nTotLgGoups = int(nMaxMirros * (rand() / float(RAND_MAX)));
         for(int nLgGoup = 0; nLgGoup <= nTotLgGoups; ++nLgGoup)
         {
            str = _T("LG");
            str += aPostFix[nLgGoup];
            str += aMirrorName[nLgGoup];

            TDSI.nNodeType = eLGGROUP;
            m_aTDSI[TDSI.nNodeSequence] = TDSI;
            tvi.item.iImage = 2;
            tvi.item.iSelectedImage = 2;
            tvi.hParent = hItemPrimary;
            tvi.hInsertAfter = TVI_LAST;
            tvi.item.pszText = LPTSTR(LPCTSTR(str));
            tvi.item.lParam = (LPARAM)TDSI.nNodeSequence++;
            TreeCtrl.InsertItem(&tvi);
         }
      }
   }
   m_nTotNodes = TDSI.nNodeSequence;
   return TRUE;
}

void Doc::GetServerInfo(CTreeCtrl& TreeCtrl)
{
   TVITEM item;
   CString strNodeName;
   item.cchTextMax = 100;
   item.pszText = strNodeName.GetBufferSetLength(item.cchTextMax);
   item.hItem = TreeCtrl.GetSelectedItem();
   item.mask = 0
               // | TVIF_CHILDREN
               // | TVIF_DI_SETITEM
               // | TVIF_HANDLE
               // | TVIF_IMAGE
               | TVIF_PARAM
               // | TVIF_SELECTEDIMAGE
               // | TVIF_STATE
               | TVIF_TEXT
               | 0;

   TreeCtrl.GetItem(&item);
   ASSERT(item.mask & TVIF_PARAM);
   ASSERT(item.lParam < m_nTotNodes);
   STempAndDummyServerInfo& TDSI = m_aTDSI[item.lParam];

   switch(TDSI.nNodeType)
   {
      case eDOMAIN:
      break;

      case eMACHINE:
         m_ServerInfo.bValid = true;
         m_ServerInfo.strName = item.pszText;
         m_ServerInfo.IPAddress = TDSI.ip;
         m_ServerInfo.nBABUsed = TDSI.nBABUsed;
         item.hItem = TreeCtrl.GetParentItem(item.hItem);
         TreeCtrl.GetItem(&item);
         m_ServerInfo.strDomain = item.pszText;
         UpdateAllViews(NULL);
      break;

      case eLGGROUP:
      break;

      default:
      ASSERT(FALSE);
   }
}
