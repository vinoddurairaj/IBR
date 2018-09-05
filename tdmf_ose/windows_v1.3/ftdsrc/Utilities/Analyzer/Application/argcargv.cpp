//==========================================
// TINYCRT - Matt Pietrek 1996
// Microsoft Systems Journal, October 1996
// FILE: ARGCARGV.CPP
//==========================================
//#include "stdafx.h"
#include "windows.h"
#include "tchar.h"

#ifdef WIN32
extern "C" {
#define _MAX_CMD_LINE_ARGS  128 

LPTSTR _ppszArgv[_MAX_CMD_LINE_ARGS+1];



int __cdecl _ConvertCommandLineToArgcArgv( LPTSTR pszSysCmdLine )
{
    int cbCmdLine;
    int argc;
    LPTSTR pszCmdLine;
    
    // Set to no argv elements, in case we have to bail out
    _ppszArgv[0] = 0;

    // First get a pointer to the system's version of the command line, and
    // figure out how long it is.
    cbCmdLine = lstrlen( pszSysCmdLine );

    // Allocate memory to store a copy of the command line.  We'll modify
    // this copy, rather than the original command line.  Yes, this memory
    // currently doesn't explicitly get freed, but it goes away when the
    // process terminates.
    pszCmdLine = (LPTSTR)HeapAlloc( GetProcessHeap(), 0, (cbCmdLine+1)*sizeof(TCHAR) );
    if ( !pszCmdLine )
        return 0;

    // Copy the system version of the command line into our copy
    lstrcpy( pszCmdLine, pszSysCmdLine );

    if ( _T('"') == *pszCmdLine )   // If command line starts with a quote ("),
    {                           // it's a quoted filename.  Skip to next quote.
        pszCmdLine++;
    
        _ppszArgv[0] = pszCmdLine;  // argv[0] == executable name
    
        while ( *pszCmdLine && (*pszCmdLine != _T('"')) )
            pszCmdLine++;

        if ( *pszCmdLine )      // Did we see a non-NULL ending?
            *pszCmdLine++ = 0;  // Null terminate and advance to next char
        else
            return 0;           // Oops!  We didn't see the end quote
    }
    else    // A regular (non-quoted) filename
    {
        _ppszArgv[0] = pszCmdLine;  // argv[0] == executable name

        while ( *pszCmdLine && (_T(' ') != *pszCmdLine) && (_T('\t') != *pszCmdLine) )
            pszCmdLine++;

        if ( *pszCmdLine )
            *pszCmdLine++ = 0;  // Null terminate and advance to next char
    }

    // Done processing argv[0] (i.e., the executable name).  Now do th
    // actual arguments

    argc = 1;

    while ( 1 )
    {
        // Skip over any whitespace
        while ( *pszCmdLine && (_T(' ') == *pszCmdLine) || (_T('\t') == *pszCmdLine) )
            pszCmdLine++;

        if ( 0 == *pszCmdLine ) // End of command line???
            return argc;

        if ( _T('"') == *pszCmdLine )   // Argument starting with a quote???
        {
            pszCmdLine++;   // Advance past quote character

            _ppszArgv[ argc++ ] = pszCmdLine;
            _ppszArgv[ argc ] = 0;

            // Scan to end quote, or NULL terminator
            while ( *pszCmdLine && (*pszCmdLine != _T('"')) )
                pszCmdLine++;
                
            if ( 0 == *pszCmdLine )
                return argc;
            
            if ( *pszCmdLine )
                *pszCmdLine++ = 0;  // Null terminate and advance to next char
        }
        else                        // Non-quoted argument
        {
            _ppszArgv[ argc++ ] = pszCmdLine;
            _ppszArgv[ argc ] = 0;

            // Skip till whitespace or NULL terminator
            while ( *pszCmdLine && (_T(' ')!=*pszCmdLine) && (_T('\t')!=*pszCmdLine) )
                pszCmdLine++;
            
            if ( 0 == *pszCmdLine )
                return argc;
            
            if ( *pszCmdLine )
                *pszCmdLine++ = 0;  // Null terminate and advance to next char
        }

        if ( argc >= (_MAX_CMD_LINE_ARGS) )
            return argc;
    }
}
}
#endif
