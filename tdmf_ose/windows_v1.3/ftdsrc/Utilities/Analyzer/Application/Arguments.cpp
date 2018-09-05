// Arguments.cpp: implementation of the Arguments class.
//
// (C) 2001 NOVACOM GmbH, Berlin, www.novacom.net
// Author: Patrick Hoffmann, 03/21/2001
//
// Added hidden arguments 09/06/2003 (day/month/year!!)
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Arguments.h"
#include <iostream>

#ifdef WIN32
extern "C" {
int __cdecl _ConvertCommandLineToArgcArgv( LPTSTR pszSysCmdLine );
extern LPTSTR _ppszArgv[];
}
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

using namespace std;

Arguments::Option	Arguments::Option::Empty(_T('\0'));
tstring				Arguments::UnknownArgument(_T("<UnKnOwN>"));

Arguments::Arguments(tstring strCommandName, tstring strDescription, tstring strOptionmarkers)
: m_strCommandName( strCommandName ) 
, m_strDescription( strDescription )
, m_strOptionmarkers( strOptionmarkers )
{
}

Arguments::~Arguments()
{
}

#ifdef WIN32
bool Arguments::Parse(LPTSTR pszCommandLine)
{
	int argc = _ConvertCommandLineToArgcArgv(pszCommandLine);
	
	return Parse( argc, _ppszArgv );
}

bool Arguments::Parse()
{
	int argc = _ConvertCommandLineToArgcArgv(GetCommandLine());
	
	return Parse( argc, _ppszArgv );
}
#endif
bool Arguments::Parse(int argc, TCHAR *argv[])
{
	if( m_strCommandName.empty() )
		m_strCommandName = argv[0];

	int nArg = 0;

	for( int i=1; i<argc; i++ )
	{
		tstring strArgument = argv[i];

		// Option...?
		if( m_strOptionmarkers.find(strArgument.substr(0,1)) != tstring::npos )
		{
			TCHAR chOptionName = strArgument[1];

			OptionMap::iterator it = m_mOptions.find(chOptionName);

			if( it == m_mOptions.end() )
			{
				cerr << m_strCommandName << " error: Unknown option " << strArgument << "." << endl;
				Usage();
				return false;
			}
			else
			{
				it->second.m_bSet = true;

				i++;
				{ 
					int nNonOptionalArgs = 0;
					
					{
						for( ArgVector::iterator itOptArg = it->second.m_vArguments.begin(); itOptArg != it->second.m_vArguments.end(); itOptArg++ ) 
						{
							if( !itOptArg->m_bOptional )
								nNonOptionalArgs++;
						}
					}
					
					for(int nOptArg=0; nOptArg < it->second.m_vArguments.size(); i++, nOptArg++ )
					{
						if( i >= argc || m_strOptionmarkers.find(tstring(argv[i]).substr(0,1)) != tstring::npos )
						{
							if( nOptArg < nNonOptionalArgs )
							{
								cerr << m_strCommandName << " error: Too few arguments for option " << strArgument << "." << endl;
								Usage();
								return false;
							}
							else
							{
								break;
							}
						}
						
						it->second.m_vArguments[nOptArg].m_strValue = argv[i];
					}
				}
				i--;
			}
		}
		else	// ...oder Argument
		{
			if( nArg >= m_vArguments.size() )
			{
				cerr << m_strCommandName << " error: Too much arguments. " << endl;
				Usage();
				return false;
			}

			m_vArguments[nArg++].m_strValue = strArgument;
		}
	}

	{
		int nNonOptionalArgs = 0;
	
		{
			for( ArgVector::iterator it = m_vArguments.begin(); it != m_vArguments.end(); it++ ) 
			{
				if( !it->m_bOptional )
					nNonOptionalArgs++;
			}
		}
		
		if( nNonOptionalArgs > nArg )
		{
			cerr << m_strCommandName << " error: Too few arguments." << endl;
			Usage();
			return false;
		}
	}
	
	return true;
}

bool Arguments::AddOption(TCHAR chOption, tstring strDescription, bool bHidden )
{
	m_mOptions.insert( pair<TCHAR,Option>(chOption,Option(chOption,strDescription)) );

    OptionMap::iterator it = m_mOptions.find( chOption );
    it->second.Hide(bHidden);   

	return true;
}

bool Arguments::AddOption( Option &option )
{
	m_mOptions.insert( pair<TCHAR,Option>(option.m_chName,option) );
	
	return true;
}

bool Arguments::Usage()
{
	cerr << "Usage: " << m_strCommandName;
	
    //
    // Display all single character options example: [/a] [/v [filename]] [/h]
    //
	for( OptionMap::iterator it = m_mOptions.begin(); it != m_mOptions.end(); it++ )
	{
        //
        // Don't display hidden arguments
        //
        if (!it->second.IsHidden())
        {

		    cerr << " [" << m_strOptionmarkers[0] << it->second.GetName();
		    
		    for( ArgVector::iterator itArg = it->second.m_vArguments.begin(); itArg != it->second.m_vArguments.end(); itArg++ )
		    {
			    if( itArg->m_bOptional )
				    cerr << " [" << itArg->m_strName << "]";
			    else
				    cerr << " " << itArg->m_strName;
		    }
		    cerr << "]";

        }

	}

    //
    // Go trough large arguments example : [test]
    //
	for( ArgVector::iterator itArg = m_vArguments.begin(); itArg != m_vArguments.end(); itArg++ )
	{
        if (!itArg->m_bHidden)
        {
		    if( itArg->m_bOptional )
			    cerr << " [" << itArg->m_strName << "]";
		    else
			    cerr << " " << itArg->m_strName;
        }
	}
	
	cerr << endl;

	if( !m_mOptions.empty() )
		cerr << endl << "Options:" << endl;
	
    //
    // Display option text
    //
	for( it = m_mOptions.begin(); it != m_mOptions.end(); it++ )
	{
        if (!it->second.IsHidden())
        {

		    cerr << "\t-" << it->second.GetName() << "\t  " << it->second.m_strDescription << endl;
		    
		    for( ArgVector::iterator itArg = it->second.m_vArguments.begin(); itArg != it->second.m_vArguments.end(); itArg++ )
		    {
			    cerr << "\t " << itArg->m_strName << "\t= " << itArg->m_strDescription << endl;

			    if( itArg->m_bOptional )
				    cerr << "\t\t  optional argument (default='" << itArg->m_strDefault << "')" << endl;
		    }
        }
	}
	
	if( !m_vArguments.empty() )
		cerr << endl << "Arguments:" << endl;

	for( itArg = m_vArguments.begin(); itArg != m_vArguments.end(); itArg++ )
	{
        if (!itArg->m_bHidden)
        {
    		cerr << "\t" << itArg->m_strName << "\t= " << itArg->m_strDescription << endl;

	    	if( itArg->m_bOptional )
		    	cerr << "\t\t  optional argument (default='" << itArg->m_strDefault << "')" << endl;
        }
	}
	
	cerr << endl;
	
	cerr << m_strDescription << endl;

	return true;
}

Arguments::Option::Option( TCHAR chName, tstring strDescription )
: m_chName( chName )
, m_strDescription( strDescription )
, m_bSet( false )
, m_bHidden( false )
{
}

bool Arguments::AddArgument( tstring strName, tstring strDescription, tstring strDefault )
{
	m_vArguments.push_back( Argument( strName, strDescription, strDefault ) );
	return true;
}

bool Arguments::Option::AddArgument( tstring strName, tstring strDescription, tstring strDefault )
{
	m_vArguments.push_back( Argument( strName, strDescription, strDefault ) );
	return true;
}

Arguments::Argument::Argument( tstring strName, tstring strDescription, tstring strDefault , bool bHide)
: m_strName( strName )
, m_strDescription( strDescription )
, m_strValue( strDefault )
, m_strDefault( strDefault )
, m_bOptional( !strDefault.empty() )
{
    m_bHidden = bHide;
}

void Arguments::Argument::Hide( bool bHide)
{
    m_bHidden = bHide;
}

bool Arguments::IsOption(TCHAR chOptionName)
{
	OptionMap::iterator it = m_mOptions.find(chOptionName);
	
	if( it == m_mOptions.end() )
		return false;
	else 
		return it->second.m_bSet;
}

Arguments::Option::operator bool()
{
	return m_bSet;
}

void Arguments::Option::Set( bool bSet )
{
	m_bSet = bSet;
}

void Arguments::Option::Hide( bool bHide )
{
    m_bHidden = bHide;
}

tstring &Arguments::operator[]( int n )
{
	return m_vArguments[n].m_strValue;
}

tstring &Arguments::operator[]( tstring strArgumentName )
{
	for( ArgVector::iterator it = m_vArguments.begin(); it != m_vArguments.end(); it++ ) 
	{
		if( it->m_strName == strArgumentName )
			return it->m_strValue;
	}

	return UnknownArgument;
}

tstring &Arguments::Option::operator[]( int n )
{
	return m_vArguments[n].m_strValue;
}

tstring &Arguments::Option::operator[]( const TCHAR *pszArgumentName )
{
	return operator[]( (tstring)pszArgumentName );
}

tstring &Arguments::Option::operator[]( tstring strArgumentName )
{
	for( ArgVector::iterator it = m_vArguments.begin(); it != m_vArguments.end(); it++ ) 
	{
		if( it->m_strName == strArgumentName )
			return it->m_strValue;
	}

	return UnknownArgument;
}

Arguments::Option &Arguments::operator[]( TCHAR chOptionName )
{
	OptionMap::iterator it = m_mOptions.find(chOptionName);
	
	if( it == m_mOptions.end() )
		return Option::Empty;
	else 
		return it->second;
}

tstring Arguments::Option::GetName()
{
	tstring str = _T(" ");

	str[0] = m_chName;

	return str;
}
