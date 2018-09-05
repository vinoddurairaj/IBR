// SVSheet.cpp : implementation file
//

#include "stdafx.h"
#include "SVGlobal.h"
#include "MonitorRes.h"
#include "SVSheet.h"

IMPLEMENT_SERIAL(SVSheet, CObject, 1)
// should use ar.GetObjectSchema(), but doesn't work
static const UINT nVersion = 1;	// so do it by hand

void SVSheet::Copy(SVSheet* p)
{
	csName = p->csName;
	bUseDefaults = p->bUseDefaults;
	caColumns.RemoveAll();
	int nCount = p->caColumns.GetSize();
	caColumns.SetSize(nCount);
	for (int i=0; i<nCount; i++)
		caColumns[i] = p->caColumns[i];
}

void SVSheet::Clear()
{
	csName.Empty(); 
	caColumns.RemoveAll();
	bUpdatable = true;
	bUseDefaults = true;
	nColumnOrder[0] = -1;

}
/*
//Q168440
#include<iostream>

using namespace std;
#define WORKAROUND 
#ifdef WORKAROUND
std::ostream& operator<<(std::ostream& os, __int64 i )
{
    char buf[20];
    sprintf(buf,"%I64d", i );
    os << buf;
    return os;
}

#endif

*/





void SVSheet::Serialize( CArchive& ar )
{

    CObject::Serialize( ar );
	int nCount;
    if( ar.IsStoring() )
	{
		nCount = caColumns.GetSize();
		
		
		ar << csName;
		ar << nVersion;
//		ar << nColumnOrder;
		ar << (BYTE&)bUpdatable;
		ar << (BYTE&)bUseDefaults;
		ar << nCount;
		for (int i=0; i<nCount; i++)
		{

			

			CString csn64MinValue;
			csn64MinValue.Format(_T("%I64d"),caColumns[i].n64MinValue); 
			CString csn64MaxValue;
			csn64MaxValue.Format(_T("%I64d"),caColumns[i].n64MaxValue); 
			CString csint64RuleValue;
			csint64RuleValue.Format(_T("%I64d"),caColumns[i].int64RuleValue); 
			CString csint64RuleValue2;
			csint64RuleValue2.Format(_T("%I64d"),caColumns[i].int64RuleValue2); 

			


			ar	<< caColumns[i].csHeaderText	
				<< nColumnOrder[i]
				<< caColumns[i].nHeaderJustify
				<< caColumns[i].nJustify
				<< caColumns[i].clr
				<< caColumns[i].dMinValue
				<< caColumns[i].dMaxValue
				<< caColumns[i].nMinValue
				<< caColumns[i].nMaxValue

				<< csn64MinValue
//				<< caColumns[i].n64MinValue

				<< csn64MaxValue
//				<< caColumns[i].n64MaxValue

				<< caColumns[i].nSpike
				<< caColumns[i].cdtMin
				<< caColumns[i].cdtMax
				<< caColumns[i].rt
				<< caColumns[i].rs
				<< caColumns[i].clrRule[COLORRULE0]
				<< caColumns[i].clrRule[COLORRULE1]
				<< caColumns[i].clrRule[COLORRULE2]
				<< caColumns[i].csRuleValue
				<< caColumns[i].cdtRuleValue
				<< caColumns[i].cdtRuleValue2
				
				<< csint64RuleValue
//				<< caColumns[i].int64RuleValue

				<< csint64RuleValue2
//				<< caColumns[i].int64RuleValue2

				<< caColumns[i].nDateRadio
				<< caColumns[i].nValueFromColumn
				<< caColumns[i].dRuleValue
				<< caColumns[i].dRuleValue2
				<< caColumns[i].cShowStatus
				<< caColumns[i].ut
				<< caColumns[i].ct
				<< caColumns[i].sc
				<< caColumns[i].nSubColumns
				<< caColumns[i].nImageTrue
				<< caColumns[i].nImageFalse
				<< (BYTE&)caColumns[i].bText
				<< (BYTE&)caColumns[i].bTextBlack
				<< (BYTE&)caColumns[i].bGraphic
				<< (BYTE&)caColumns[i].bFixedText
				<< (BYTE&)caColumns[i].bRule
				<< (BYTE&)caColumns[i].bMinMaxDirty
				<< (BYTE&)caColumns[i].bMinMaxSet
				<< (BYTE&)caColumns[i].bColumnWidthDirty
				<< (BYTE&)caColumns[i].bGraphable
				<< (BYTE&)caColumns[i].bIgnoreCase
				<< caColumns[i].nPixelWidth
				<< caColumns[i].nPixelFixedWidth
				<< caColumns[i].gdt;
		}
	}
	else
	{
		try
		{
			UINT nVer;
			CString csNameFromArchive;
			ar >> csNameFromArchive;
//			if (csNameFromArchive != csName)
//				return;
			ar >> nVer;
			switch (nVer)
			{
			case 1:
			{
//				ar >> nColumnOrder;
				ar >> (BYTE&)bUpdatable;
				ar >> (BYTE&)bUseDefaults;
				ar >> nCount;
				CString csn64MinValue;
				CString csn64MaxValue;
				CString csint64RuleValue;
				CString csint64RuleValue2;

				for (int i=0; i<nCount; i++)
				{

					SVColumn svCol;
					ar	>> svCol.csHeaderText
						>> nColumnOrder[i]

						>> svCol.nHeaderJustify
						>> svCol.nJustify
						>> svCol.clr
						>> svCol.dMinValue
						>> svCol.dMaxValue
						>> svCol.nMinValue
						>> svCol.nMaxValue

						>> csn64MinValue
//						>> svCol.n64MinValue

						>> csn64MaxValue
//						>> svCol.n64MaxValue

						>> svCol.nSpike
						>> svCol.cdtMin
						>> svCol.cdtMax
						>> (int&)svCol.rt
						>> (int&)svCol.rs
						>> svCol.clrRule[COLORRULE0]
						>> svCol.clrRule[COLORRULE1]
						>> svCol.clrRule[COLORRULE2]
						>> svCol.csRuleValue
						>> svCol.cdtRuleValue
						>> svCol.cdtRuleValue2

						>> csint64RuleValue
//						>> svCol.int64RuleValue

						>> csint64RuleValue2
//						>> svCol.int64RuleValue2

						>> svCol.nDateRadio
						>> svCol.nValueFromColumn
						>> svCol.dRuleValue
						>> svCol.dRuleValue2
						>> svCol.cShowStatus
						>> (int&)svCol.ut
						>> (int&)svCol.ct
						>> (int&)svCol.sc
						>> svCol.nSubColumns
						>> svCol.nImageTrue
						>> svCol.nImageFalse
						>> (BYTE&)svCol.bText
						>> (BYTE&)svCol.bTextBlack
						>> (BYTE&)svCol.bGraphic
						>> (BYTE&)svCol.bFixedText
						>> (BYTE&)svCol.bRule
						>> (BYTE&)svCol.bMinMaxDirty
						>> (BYTE&)svCol.bMinMaxSet
						>> (BYTE&)svCol.bColumnWidthDirty
						>> (BYTE&)svCol.bGraphable
						>> (BYTE&)svCol.bIgnoreCase
						>> svCol.nPixelWidth
						>> svCol.nPixelFixedWidth
						>> (int&)svCol.gdt;

					svCol.n64MinValue = _ttoi64(csn64MinValue );
					svCol.n64MaxValue = _ttoi64(csn64MaxValue );
					svCol.int64RuleValue = _ttoi64(csint64RuleValue);
					svCol.int64RuleValue2 = _ttoi64(csint64RuleValue2);


					caColumns.Add(svCol);
				}
				break;
			}
			case 2:
				return; // quietly use defaults, previous test 

			default: // Unknown version
				AfxThrowArchiveException (CArchiveException::badSchema);
				break;
			}//switch
		}
		// need multiple catch for more granular message
		catch(...)
		{
			// assumes serialize is to a file
			CString cs;
			cs = RESCHAR(IDS_V_ARCHIVELOADERROR);
			cs += ar.m_strFileName;
			AfxMessageBox(cs,MB_ICONSTOP);
		}
	}
}