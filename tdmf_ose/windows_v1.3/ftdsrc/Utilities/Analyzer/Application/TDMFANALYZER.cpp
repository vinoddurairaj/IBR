#include "stdafx.h"

#include "resource.h"
#include <commctrl.h>
#include "NTQuery.h"
#include "FileObjInfoIoCtl.h"
#include "dynamic.h"

#include "util.h"

#include "TDMFANALYZER.h"

#include "stdio.h"

#include "psapi.h"

#include <vector>
#include <string>
#include <algorithm>
#include <functional>

using namespace std;

//
// handle used to talk to our device driver
//
static HANDLE hDriver = INVALID_HANDLE_VALUE;

//
// Driver is loaded?
// lists are created?
// send verbose output (list all filenames locked by apps)?
//
bool    gbDriverLoaded = false;
bool    gbListsCreated = false;
bool    gbVerbose      = false;


void LoadDriverNow(void)
{
    OFSTRUCT    of;
    BOOL        fNTDynaLoaded;
    char* srvexe = "TDMFANALYZER.sys";
    char* srvnam = "TDMFANALYZER";
    char* srvdos = "\\\\.\\TDMFANALYZER";

    if (!gbDriverLoaded)
    {
	    // load driver...
	    HINSTANCE ghInst    = GetModuleHandle(NULL);
	    HRSRC hRsrc         = FindResource(ghInst, MAKEINTRESOURCE(DRVRIMG),"BINRES");
	    HGLOBAL hDrvRsrc    = LoadResource(ghInst, hRsrc);
	    DWORD dwDriverSize  = SizeofResource(ghInst, hRsrc);
	    LPVOID lpvDriver    = LockResource(hDrvRsrc);

        HFILE hfTempFile = _lcreat(srvexe, NULL);
	    _hwrite(hfTempFile, (char*)lpvDriver, dwDriverSize); 
	    _lclose(hfTempFile);

	    // Try loading the driver
        hDriver = LoadDriver(&fNTDynaLoaded, srvdos, srvnam, srvexe);

        // Delete the temp file...
        OpenFile(srvexe, &of, OF_DELETE);

        if(hDriver == INVALID_HANDLE_VALUE)
        {
            MessageBox(NULL, "Can't load driver", "ERROR!", MB_OK);
		    CloseHandle(hDriver);
		    UnloadDriver(srvnam);
		    exit(1);
        }

	    if (!EnablePrivilege (GetCurrentProcess(), SE_DEBUG_NAME, TRUE))
        {
            MessageBox(NULL, "Can't enable required privilege(SE_DEBUG)", "ERROR!", MB_OK);
		    exit(1);
        }

        gbDriverLoaded = true;
    }
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//  Class declarations
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//
// The File Handle class declaration (it's simply a structure, but what the hey...)
//
class FileHandleClass
{
public:

    //
    // When someone compares a filehandleclass object,
    // we check to see that the GUID's are the same
    //
    bool operator== (const FileHandleClass &obj) const
    {
    	return this->sGUID == obj.sGUID;
    }

    unsigned int        uiProcessId;
    string              sProcessName;
    string              sGUID;
    string              sFileName;
}; 

//
// The Volume class declaration (it's simply a structure, but...)
//
class VolumeClass
{
public:

    //
    // When someone compares a VolumeClass object,
    // we check to see that the GUID's are the same
    //
    bool operator== (const FileHandleClass &obj) const
    {
    	return this->sGUID == obj.sGUID;
    }

    string              sVolumeName;
    string              sGUID;
};

//
// We want a vector of our handles to be able to search easily
//
typedef vector<FileHandleClass> FileHandlesVector;
typedef vector<VolumeClass>     VolumeClassVector;


//
// We also need an iterator to easily go trough our list
//
typedef FileHandlesVector::iterator FileHandlesVector_IT;
typedef VolumeClassVector::iterator VolumeClassVector_IT;
 
//
// Finally add some sorting support classes to allow us
// to sort our class by each of it's elements
//
template <class T> class SortByPorcessId:
 std::binary_function<T, T, bool>
{
public:
 result_type operator()(first_argument_type a, second_argument_type b)
 {
  return (result_type) (a.uiProcessId < b.uiProcessId);
 }
};

template <class T> class SortByProcessName:
 std::binary_function<T, T, bool>
{
public:
 result_type operator()(first_argument_type a, second_argument_type b)
 {
  return (result_type) (a.sProcessName < b.sProcessName);
 }
};

template <class T> class SortByGUID:
 std::binary_function<T, T, bool>
{
public:
 result_type operator()(first_argument_type a, second_argument_type b)
 {
  return (result_type) (a.sGUID < b.sGUID);
 }
};

template <class T> class SortByFileName:
 std::binary_function<T, T, bool>
{
public:
 result_type operator()(first_argument_type a, second_argument_type b)
 {
  return (result_type) (a.sFileName < b.sFileName);
 }
};

//
// some forward declarations
//

static void CreateFileHandlesList( FileHandlesVector * pFileHandlesVector );
static void GetAllVolumes(VolumeClassVector * pVolumeClassVector);

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// Global values
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//
// Our global vector containing all the file handles!
//
FileHandlesVector gFileHandlesVector;
//
// Our global vector containing all the volume names 
// (including mount points)
//
VolumeClassVector gVolumeClassVector;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// Function decalarations
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void CreateAllLists(void)
{
    if (!gbListsCreated)
    {
        CreateFileHandlesList(&gFileHandlesVector);
        GetAllVolumes(&gVolumeClassVector);
        gbListsCreated = true;
    }
}
//
// This function gets a count of all the file handles on a drive, 
// and dumps the results to a window or stdout
//
void CreateFileHandlesList( FileHandlesVector * pFileHandlesVector )
{
    FileHandleClass            current_FileHandle;

    //gFileHandlesVector 

	FOI_RESOLVE_INPUT           fi;
	FOI_RESOLVE_OUTPUT          fo;

	NtSystemProcessInformation  pi;
	NtSystemHandleInformation   hi;

    PSYSTEM_HANDLE              psh;

    BOOL                        bDontLog        = FALSE;
    
    DWORD                       dw;

    //
    // If this list is compiled get out of here!
    // 
    if (gbListsCreated)
        return;

    //
    // Get complete handle list
    //
	pi.Refresh();
	hi.Refresh();

    for (unsigned int i = 0; i < hi.Count(); i++)
    {
		psh                 = hi.Get(i);
        bDontLog            = FALSE;
        fi.FObjAddress      = (DWORD)psh->pObject;

        //
        // Get actual information about object by asking
        // device driver to tell us the parameters
        //
        if (!DeviceIoControl(hDriver, IOCTL_FOI_RESOLVE, &fi, sizeof(fi), &fo, sizeof(fo), &dw, NULL))
        {
            //
            // Impossible to get the info... next...
            //
			// lets keep error info for debug...
			dw = GetLastError();
        }
        else 
        {
			/* fobj.type is always IO_TYPE_FILE */
			/* dobj.type is always IO_TYPE_DEVICE */

			char    cFileName[1024],
                    cProcessName[255],
                    cGUIDName[255];

			if (fo.fobj.isValid && fo.fobj.FileName[0]) 
            {
                WideCharToMultiByte( CP_ACP, 0, fo.fobj.FileName , -1, cFileName, 1024, NULL, NULL );
            }
			else
            {
				sprintf(cFileName, "<no name>");
            }

            current_FileHandle.sFileName = cFileName;

			PSYSTEM_PROCESS_INFORMATION psi = pi.Find(psh->dIdProcess);

            //
            // Add process ID
            //
            current_FileHandle.uiProcessId = psh->dIdProcess;

            //
            // Eliminate the following names from the list:
            //
            //  SERVICES.EXE 
            //  WINLOGON.EXE
            //  SVCHOST.EXE
            //  System
            //
            //  Pid of 0
            //
            if (psi)
            {
                if (        (psi->usName.Length >= 13 * sizeof(TCHAR)) 
                        &&  (wcsnicmp(psi->usName.Buffer, L"WINLOGON.EXE",13 * sizeof(TCHAR))==0)   )
                {
                    bDontLog = TRUE;
                }

                if (        (psi->usName.Length >= 13 * sizeof(TCHAR)) 
                        &&  (wcsnicmp(psi->usName.Buffer, L"SERVICES.EXE",13 * sizeof(TCHAR))==0)   )
                {
                    bDontLog = TRUE;
                }

                if (        (psi->usName.Length >= 6 * sizeof(TCHAR)) 
                        &&  (wcsnicmp(psi->usName.Buffer, L"System",6 * sizeof(TCHAR))==0)      )
                {
                    bDontLog = TRUE;
                }

                if (        (psi->usName.Length >= 11 * sizeof(TCHAR)) 
                        &&  (wcsnicmp(psi->usName.Buffer, L"SVCHOST.EXE",11 * sizeof(TCHAR))==0)    )
                {
                    bDontLog = TRUE;
                }

                if ( !psh->dIdProcess )
                {
                    bDontLog = TRUE;
                }
            }

            //
            // Only want hard drives showing!
            //
            if (wcsnicmp(fo.dobj.DeviceName,L"\\device\\hard",12)!=0)
            {
                bDontLog = TRUE;
            }

            if (!bDontLog)
            {

				if (psi) 
                {
					wsprintf(cProcessName, "%ls", psi->usName.Buffer);
				}
				else 
                {
					wsprintf(cProcessName, "n.a.");
				}

                current_FileHandle.sProcessName = cProcessName;

				if (fo.dobj.isValid) 
                {
                    //
                    // Change device name with it's HARD DISK letter!
                    //
                    char    szDosDevice[1024];
                    char    szDeviceName[1024];
                    
                    BOOL    bMntPtName  = FALSE;
                    BOOL    bDosDevice  = FALSE;

                    //
                    // Convert Unicode to normalcode ;) ok convert to char...
                    //
                    WideCharToMultiByte( CP_ACP, 0, fo.dobj.DeviceName, -1, szDeviceName, 256, NULL, NULL );

                    //
                    // If this device is valid it will have a GUID that we will store
                    //
              		if (bDosDevice = QueryVolMntPtInfoFromDevName( szDeviceName, MAX_FOBJ_FILE_LEN, szDosDevice, sizeof(szDosDevice), VOLUME_GUID ) == VALID_MNT_PT_INFO)
                    {
                        strcpy(cGUIDName,szDosDevice);

                        current_FileHandle.sGUID = cGUIDName;

                        pFileHandlesVector->push_back(current_FileHandle);
                    
                    }
				}
            }
        }
    }
}

//
// Will create a vector containing the list of all devices and their corresponding GUID value
//
void GetAllVolumes(VolumeClassVector * pVolumeClassVector)
{
    char            szDriveString   [_MAX_PATH];
    char            szDosDevice     [_MAX_PATH];
    char            szCurDosDevice  [_MAX_PATH];
    char            szGUIDName      [_MAX_PATH];
    char            szMountPt       [_MAX_PATH];
    char            szDrive         [4];

    VolumeClass     currentVolumeClass;
    
    int             i                   =   0,
                    iDrive              =   0;

    //
    // If this list is compiled get out of here!
    // 
    if (gbListsCreated)
        return;
    //
    // Get all drives on this system
    //
	GetLogicalDriveStrings(sizeof(szDriveString), szDriveString);
	
    //
    // Get drive letter of each drive...
    //
	while(szDriveString[i] != 0 && szDriveString[i+1] != 0)
	{
        szDrive[0] = szDriveString[i];    //Drive Letter is here !
        szDrive[1] = szDriveString[i+1];
        szDrive[2] = szDriveString[i+2];
        szDrive[3] = 0;
		i = i + 4;

		memset(szDosDevice, 0, sizeof(szDosDevice));
        memset(szCurDosDevice, 0, sizeof(szCurDosDevice));

		strcpy(szDosDevice , "\\DosDevices\\");
		strncat(szDosDevice , &szDrive[0], 1);
		strcat(szDosDevice , ":");

        //
        // validate drivetype (Don't care about CD's, ramdisks, remotes, and removables!)
        //
		iDrive = GetDriveType(szDrive);

		if(     iDrive != DRIVE_REMOVABLE 
            &&  iDrive != DRIVE_CDROM 
            &&  iDrive != DRIVE_RAMDISK 
            &&  iDrive != DRIVE_REMOTE      )
		{   
            HANDLE  hDrive                  = NULL;                  
            bool    bDriveNotFinished       = true;
            bool    bGUIDName               = true;
            bool    bAreMountPointsPresent  = false;

            strcpy(szCurDosDevice,szDosDevice);

            //
            // Process drive and any mount points on this drive
            //
            while (bDriveNotFinished && bGUIDName)
            {
                memset(szMountPt,0,sizeof(szMountPt));
                memset(szGUIDName,0,sizeof(szGUIDName));
                //
                // Get Name and GUID of current device!
                //
                if (    
                        (QueryVolMntPtInfoFromDevName( szCurDosDevice, sizeof(szCurDosDevice), szGUIDName, sizeof(szGUIDName), VOLUME_GUID ) == VALID_MNT_PT_INFO) 
                     ||
                        (UtilGetGUIDforVolumeMountPoint( szCurDosDevice, szGUIDName, sizeof(szGUIDName) ) )
                   )
                {
                    //
                    // Some GUID may come back as \\?\ instead of \??\
                    // and finish with a \ instead of }
                    //
                    // so change it back to \??\ and no \ behind the }!!!
                    //
                    szGUIDName[1] = '?';  
					szGUIDName[48] = 0;
					
                    //
                    // Store drivename and guid!
                    //
                    if (bAreMountPointsPresent)
                    {
                        currentVolumeClass.sVolumeName = szCurDosDevice;
                    }
                    else
                    {
                        currentVolumeClass.sVolumeName = szDrive;
                    }
                    currentVolumeClass.sGUID = szGUIDName;
                    gVolumeClassVector.push_back(currentVolumeClass);

                    //
                    // Check for any more volume mount points
                    //
                    if (!hDrive)
                    {
                        hDrive = UtilGetVolumeHandleAndFirstMountPoint( szDrive, szMountPt, _MAX_PATH );
                        if (hDrive == INVALID_HANDLE_VALUE)
                        {
                            bDriveNotFinished = false;
                        }
                    }
                    else
                    {
                        if (bAreMountPointsPresent)
                        {
                            bDriveNotFinished = (UtilGetNextVolumeMountPoint(hDrive,szMountPt,_MAX_PATH ) >0);
                        }
                    }

    			    if (hDrive && !bDriveNotFinished) 
	    		    {
        			    UtilCloseVolumeHandle(hDrive);
                    }
                    else
                    {
                        //
                        // Initialize szCurDosDevice with drivename followed by mountpoint!
                        //
                        bAreMountPointsPresent = true;
                		strcpy(szCurDosDevice,szDrive);
		                strcat(szCurDosDevice,szMountPt);
                    }
                }
                else
                {
                    bDriveNotFinished = false;
                }
            }
        }
    }

}

void    ListFileHandleListOnGUID(char * pcGUID)
{
    FileHandlesVector_IT    it;
    VolumeClassVector_IT    itv;
    string                  VolumeGUID          =   pcGUID;
    string                  CurrentProcess      =   "none";
    unsigned int            CurrentPID          =   0;
    unsigned int            TotalHandlesOnDrive =   0;
    unsigned int            ProcessHandleCount  =   0;

    //
    // Sort all handles by GUID
    //
    sort(gFileHandlesVector.begin(), gFileHandlesVector.end(), SortByGUID<FileHandleClass>());

    //
    // Find the volume name of the specified GUID
    //
    for (itv = gVolumeClassVector.begin(); itv < gVolumeClassVector.end() ; itv++)
    {
        if (itv->sGUID==VolumeGUID)
        {
            printf("------------------------------------\n");
            printf("VOLUME HANDLES ON DRIVE %s\n", itv->sVolumeName.c_str());
            printf("------------------------------------\n");
        }
    }

    //
    //  Get current GUID/PROCESS
    //
    it                  = gFileHandlesVector.begin();
    CurrentPID          = 0;

    while (it < gFileHandlesVector.end())
    {
        //
        // Are we still looking at our GUID?
        //
        if (VolumeGUID == it->sGUID)
        {
            TotalHandlesOnDrive++;
            
            //
            // if this is the first PID on 
            // this volume set all first values
            //
            if (!CurrentPID)
            {
                CurrentPID          = it->uiProcessId;
                CurrentProcess      = it->sProcessName;
                ProcessHandleCount  = 0;
            }

            //
            // Are we still looking at the same PID?
            //
            if (CurrentPID != it->uiProcessId)
            {
                if (ProcessHandleCount)
                {
                    //
                    // Display old process's values
                    //
                    printf( "  -------------------------------------------------\n");
                    printf( "  PID[%04ld] - Process[%12s] - Handles[%4ld]\n",
                            CurrentPID,
                            CurrentProcess.c_str(),
                            ProcessHandleCount          );
                    printf( "  -------------------------------------------------\n");
                }

#ifdef DEBUG
                //
                // To check if we are calculating correctly!
                //
                // p.s. until we get out of this while, our 
                // total is always wrong by 1...
                //
                printf( " --- TOTAL [%04ld] ---\n",TotalHandlesOnDrive-1);
#endif
                //
                // New Process ID, display #of handles!
                // 
                CurrentPID          = it->uiProcessId;
                CurrentProcess      = it->sProcessName;
                //
                // This is the first handle of this process
                //
                ProcessHandleCount  = 1;
            }
            else
            {
                //
                // Add up handles for this PID
                //
                ProcessHandleCount++;

            }

            //
            // In verbose mode display all the info!
            //
            // PID - process name - open handle's filename
            //
            if (gbVerbose)
            {
                printf( "  PID[%04ld] - Process[%12s] - File[%s]\n",
                        it->uiProcessId,
                        it->sProcessName.c_str(),
                        it->sFileName.c_str()       );
            }
          
        }

        it++;
    }

    //
    // We parsed all the info, display the last items!
    //

    //
    // Display old process's values
    //
    if (ProcessHandleCount)
    {
        printf( "  -------------------------------------------------\n");
        printf( "  PID[%04ld] - Process[%12s] - Handles[%4ld]\n",
                CurrentPID,
                CurrentProcess.c_str(),
                ProcessHandleCount          );
        printf( "  -------------------------------------------------\n");
    }

    //
    // Total handles on current drive 
    //
    printf("--------------------\n");
    printf("Open handles[%6ld]\n",
            TotalHandlesOnDrive     );
    printf("--------------------\n");


}



//
// List all file handles according to their volume name (change guid for volume)
//
void ListFileHandleListOnAllVolumes()
{
    FileHandlesVector_IT    it;
    VolumeClassVector_IT    itv;
    string                  CurrentGUID         =   "none";
    string                  CurrentVolume       =   "none";
    string                  CurrentProcess      =   "none";
    unsigned int            CurrentPID          =   0;
    unsigned int            ProcessHandleCount  =   0;
    unsigned int            TotalHandlesOnDrive =   0;
    unsigned int            TotalHandlesOnAll   =   0;

    //
    // Sort all handles by GUID
    //
    sort(gFileHandlesVector.begin(), gFileHandlesVector.end(), SortByGUID<FileHandleClass>());

    //
    // We want a file handle table in the type: 
    //
    // Drive A:
    //
    // if verbose ->
    //
    //  PID A   NAME    FILENAME OPEN ON DRIVE
    //  PID A   NAME    FILENAME OPEN ON DRIVE
    //
    // <- if verbose
    //
    //  PID A   NAME    #of handles on drive A
    //
    //  PID B   ...
    //
    // Drive B:
    // ...
    //

    printf("-------------------\n");
    printf("ALL VOLUMES HANDLES\n");
    printf("-------------------\n");

    //
    // Get first process and it's name
    // 
    it                  = gFileHandlesVector.begin();
    CurrentPID          = it->uiProcessId;
    CurrentProcess      = it->sProcessName;

    while (it < gFileHandlesVector.end())
    {
        TotalHandlesOnDrive++;
        TotalHandlesOnAll++;
        
        //
        // Are we still looking at the same GUID?
        //
        if (CurrentGUID != it->sGUID)
        {
            //
            // If this is not the first drive, 
            // display:
            //
            // totals of last PID
            // total handles on drive
            //
            if (strcmp(CurrentGUID.c_str(),"none") )
            {
                //
                // Display old process's values
                //
                printf( "  -------------------------------------------------\n");
                printf( "  PID[%04ld] - Process[%12s] - Handles[%4ld]\n",
                        CurrentPID,
                        CurrentProcess.c_str(),
                        ProcessHandleCount          );
                printf( "  -------------------------------------------------\n");
                //
                // New Process ID, display #of handles!
                // 
                CurrentPID          = it->uiProcessId;
                CurrentProcess      = it->sProcessName;
                //
                // This is the first handle of this process
                //
                ProcessHandleCount  = 0; 
                // 
                // this is a special case, 
                // processhandlecount will be ++ later
                // in this code!
                // 

                //
                // Total handles on current drive 
                //
                printf("--------------------\n");
                printf("Open handles[%6ld]\n",
                        TotalHandlesOnDrive-1     );
                printf("--------------------\n");
                //
                // reset value
                //
                TotalHandlesOnDrive = 1;
            }

            //
            // New GUID, find it's volume!
            //
            for (itv = gVolumeClassVector.begin(); itv < gVolumeClassVector.end() ; itv++)
            {
                if (itv->sGUID==it->sGUID)
                {
                    CurrentGUID     = it->sGUID;
                    CurrentVolume   = itv->sVolumeName;
                    printf("DRIVE: %s\n",CurrentVolume.c_str());
                    printf("----------------\n");
                }
            }
        }


        //
        // Are we still looking at the same PID?
        //
        if (CurrentPID != it->uiProcessId)
        {
            if (ProcessHandleCount)
            {
                //
                // Display old process's values
                //
                printf( "  -------------------------------------------------\n");
                printf( "  PID[%04ld] - Process[%12s] - Handles[%4ld]\n",
                        CurrentPID,
                        CurrentProcess.c_str(),
                        ProcessHandleCount          );
                printf( "  -------------------------------------------------\n");
            }

#ifdef DEBUG
            //
            // To check if we are calculating correctly!
            //
            // p.s. until we get out of this while, our 
            // total is always wrong by 1...
            //
            printf( " --- TOTAL [%04ld] ---\n",TotalHandlesOnDrive-1);
#endif
            //
            // New Process ID, display #of handles!
            // 
            CurrentPID          = it->uiProcessId;
            CurrentProcess      = it->sProcessName;
            //
            // This is the first handle of this process
            //
            ProcessHandleCount  = 1;
        }
        else
        {
            //
            // Add up handles for this PID
            //
            ProcessHandleCount++;

        }

        //
        // In verbose mode display all the info!
        //
        // PID - process name - open handle's filename
        //
        if (gbVerbose)
        {
            printf( "  PID[%04ld] - Process[%12s] - File[%s]\n",
                    it->uiProcessId,
                    it->sProcessName.c_str(),
                    it->sFileName.c_str()       );
        }


        it++;
    }

    //
    // We parsed all drives, display the last items!
    //

    //
    // Display old process's values
    //
    if (ProcessHandleCount)
    {
        printf( "  -------------------------------------------------\n");
        printf( "  PID[%04ld] - Process[%12s] - Handles[%4ld]\n",
                CurrentPID,
                CurrentProcess.c_str(),
                ProcessHandleCount          );
        printf( "  -------------------------------------------------\n");
    }

    //
    // Total handles on current drive 
    //
    printf("--------------------\n");
    printf("Open handles[%6ld]\n",
            TotalHandlesOnDrive     );
    printf("--------------------\n");

    //
    // Now display total of all handles!
    //
    printf("Open handles on ALL DRIVES [%6ld]\n",
            TotalHandlesOnAll     );
    printf("-----------------------------------\n");
}

//
// Display a list of our volumes
//
void ListVolumes(void)
{
    VolumeClassVector_IT itv;

    GetAllVolumes(&gVolumeClassVector);

    printf("Volume list\n");
    printf("-----------\n");

    for (itv = gVolumeClassVector.begin(); itv < gVolumeClassVector.end(); itv++)
    {
        printf("Volume [%s]\n",
                itv->sVolumeName.c_str() );
    }
  
}

void DisplayFullModuleName(unsigned int uiPID)
{
    char    szProcessName[MAX_PATH] = "unknown";
    DWORD   dwRetValue              = 0;
    DWORD   dwSize                  = MAX_PATH;

    if (uiPID)
    {   
        HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, uiPID );

        if (hProcess)
        {
            HMODULE hMod;
            DWORD cbNeeded;

            if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), &cbNeeded) )
            {
                if (GetModuleFileNameEx( hProcess, hMod, szProcessName, sizeof(szProcessName) ))
                {
                    //
                    // We found the name!!
                    //
                    dwRetValue = 1;
                }
            }
        }
    }

    if (dwRetValue)
    {
       printf("PID[%04ld] = %s\n",uiPID,szProcessName);
    }
    else
    {
       printf("PID[%04ld] Unable to locate application\n",uiPID);
    }

}

