
#include "../../../tdmf.inc"

#include "stdafx.h"

#include "util.h"
#include "Arguments.h"
#include "TDMFANALYZER.h"

//
// Main entry point of our application
// 
// Create and parse arguments here!
//
int main(int argc, char* argv[])
{
    //
    // Adding arguments to the file is done here
    //
    // Please add arguments as they are seen here
    //
    // i.e. "arg","what the arg does", bhidden 
    //
	Arguments cArg( argv[0], "", "-/" );
    {
        cArg.AddOption('h', "display usage help");

        cArg.AddOption('l', "display all volume names");

        cArg.AddOption('a', "display number of file handles on all drives");

        cArg.AddOption('v', "verbose mode: display all file handles and files");

        Arguments::Option   cOpt1( 'p', "return path and name for pid" );
        cOpt1.AddArgument( "pid", " process id ex.:1255","");
        cArg.AddOption( cOpt1 );

        Arguments::Option	cOpt2( 'f', "check selected volume for open handles" );
	    cOpt2.AddArgument( "volumename", "Volume name ex.: c:" , "c:" );
	    cArg.AddOption( cOpt2 );

        //
        // Add argument for termination string
        //
        cArg.AddOption('~', "string: for termination", true);

        //
        // Parse command line for options
        //
        if( !cArg.Parse( argc, argv ) )
        {
    		exit(0);
        }
    }

    //
    // We display the date this application was compiled!
    //
    printf("--------------------------------------------\n");
    printf("Analyzer %20s %9s\n",__DATE__,__TIME__);
    printf("--------------------------------------------\n");

    //
    // This is where we actually check which arguments were selected by the users
    //

    if( cArg['h'] )     // DISPLAY USAGE!
	{
		cArg.Usage();
		exit(0);
	}

    if ( cArg['~'] )
    {
        DisplayTerminationString();
        exit(0);
    }

    if ( cArg['v'] )    // SET VERBOSE MODE!
    {
        gbVerbose = TRUE;

        printf("--------------------------------------------\n");
        printf("----------- verbose mode ON ----------------\n");
        printf("--------------------------------------------\n");
    }

    if ( cArg['a'] )    // Display all drives!
    {
        LoadDriverNow();
        //
        // Get the list of volumes and their GUID's
        // Get the list of handles and their GUID's
        //
        CreateAllLists();

        //
        // List all handles by GUID 
        // (this fct will display volume names)
        //
        ListFileHandleListOnAllVolumes();
    }

    if ( cArg['p'] )    // Display selected pid!
    {
        unsigned int hModule = atoi(cArg['p']["pid"].c_str());
        DisplayFullModuleName(hModule);
    }

    if ( cArg['f'] )    // Display selected drive!
    {
        //
        // We should check the validity of the name passed in argv[1] here 
        // (has to be a valid drive name)
        //
        //
        // Change device name with it's HARD DISK letter!
        //
        char szGUID[1024];
        char szDosDevice[1024];
        char szDriveToCheck[1024];

        strcpy(szDriveToCheck,cArg['f']["volumename"].c_str() );
        //
        // Append \ to name
        //
        strcat(szDriveToCheck,"\\");
        //
        // Convert to upper case
        //
        strupr(szDriveToCheck); 

        sprintf(szDosDevice,"\\DosDevices\\%s",cArg['f']["volumename"].c_str());

        //
        // Check to see it exists
        //
	    if ( UtilGetGUIDforVolumeMountPoint(szDriveToCheck,szGUID,sizeof(szGUID)))
        {
            printf("--------------------------------------------\n");
            printf("%s GUID = %s\n",szDosDevice,szGUID);
            printf("--------------------------------------------\n");

            LoadDriverNow();
            //
            // Get the list of volumes and their GUID's
            // Get the list of handles and their GUID's
            //
            CreateAllLists();

            //
            // Get the handle list for this GUID
            //
            ListFileHandleListOnGUID(szGUID);
        }
        else
        {
            printf("--------------------------------------------\n");
            printf("Invalid parameter %s - %s\n",cArg['f']["volumename"].c_str(),szDosDevice);
            printf("--------------------------------------------\n");
            exit(0);
        }
    }

    if ( cArg['l'] )    // Display a list of all the drives...
    {

        ListVolumes();
    }

    return 0;
}


//
// This small piece of code allows you to start 
// a windows application from a console app as 
// ourselves
//
// This may be very usefull in the future, so
// we leave it in the code (commented out)
// for future reference!
//
/*  START WINMAIN!
{
    STARTUPINFO si = { sizeof(si) };
    GetStartupInfo(&si);
    WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), si.wShowWindow);
}
*/
