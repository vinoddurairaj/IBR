// Arguments.h: interface for the Arguments class.
//
// (C) 2001 NOVACOM GmbH, Berlin, www.novacom.net
// Author: Patrick Hoffmann, 03/21/2001
//
//////////////////////////////////////////////////////////////////////

#pragma warning (disable : 4786) 

#if !defined(AFX_ARGUMENTS_H__1F0E328D_C72D_4709_8A74_7654888217A6__INCLUDED_)
#define AFX_ARGUMENTS_H__1F0E328D_C72D_4709_8A74_7654888217A6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <string>
#include <vector>
#include <map>

using namespace std;

#ifndef TCHAR
typedef char TCHAR;
#ifdef _T 
#undef _T
#endif

#define _T(a) a
typedef string tstring;
#else
typedef basic_string<TCHAR> tstring;
#endif

inline ostream &operator<<(ostream& ostr, tstring& tstr )
{
	ostr << tstr.c_str();
	return ostr;
}

class Arguments  
{
	tstring	m_strOptionmarkers;
	tstring	m_strDescription;
	tstring	m_strCommandName;

public:
	static tstring	UnknownArgument;

	class Option;

	class Argument
	{
		friend class Arguments;
		friend class Option;
		
		tstring	m_strName;
		tstring	m_strDescription;
		tstring	m_strValue;
		tstring	m_strDefault;
		bool	m_bOptional;
        bool    m_bHidden;

	public:
		Argument( tstring strName, tstring strDescription=_T(""), tstring strDefault=_T("") , bool bHide = false);
        void Hide( bool bHide = true );
        bool IsHidden(void)
        { return m_bHidden;}
	};

	typedef vector<Argument>	ArgVector;

	class Option
	{
		friend class Arguments;
		static Option	Empty;
		
		TCHAR			m_chName;
		ArgVector		m_vArguments;
		tstring			m_strDescription;
		bool			m_bSet;
        bool            m_bHidden;
		
	public:
		Option( TCHAR chName, tstring strDescription=_T("") );
		bool AddArgument( tstring strName, tstring strDescription=_T(""), tstring strDefault = _T("") );
		tstring &operator[]( int n );
		tstring &operator[]( tstring strArgumentName );
		tstring &operator[]( const TCHAR *pszArgumentName );
		operator bool();
		void Set( bool bSet = true );
        void Hide( bool bHide = true );
        bool IsHidden(void)
        { return m_bHidden;}
		tstring GetName();
	};

private:
	typedef map<TCHAR,Option,less<TCHAR>,allocator<Option> > OptionMap;
	
	OptionMap			m_mOptions;
	ArgVector			m_vArguments;

public:
	bool IsOption( TCHAR chOptionName );
	bool Usage();
	bool AddOption( TCHAR chOptionName, tstring strDescription=_T("") , bool bHidden = false);
	bool AddOption( Option &option );
	bool AddArgument( tstring strName, tstring strDescription=_T(""), tstring strDefault = _T("") );
	bool Parse(int argc, TCHAR* argv[]);
	bool Parse(TCHAR *pszCommandLine);
	bool Parse();
	
	tstring &operator[]( int n );
	tstring &operator[]( tstring strArgumentName );
	Option &operator[]( TCHAR chOptionName );
	
	Arguments( tstring strCommandName, tstring strDescription=_T(""), tstring strOptionmarkers=_T("-/") );
	virtual ~Arguments();
};

#pragma warning (disable : 4786) 

#endif // !defined(AFX_ARGUMENTS_H__1F0E328D_C72D_4709_8A74_7654888217A6__INCLUDED_)
