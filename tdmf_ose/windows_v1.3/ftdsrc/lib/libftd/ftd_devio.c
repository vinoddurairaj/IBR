/*
 * ftd_devio.c - FTD device i/o interface
 * 
 * Copyright (c) 2000 Legato Systems, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */

#include "ftd_port.h"
#include "ftd_devio.h"
#include "ftdio.h"
#include "ftd_trace.h"
#include "misc.h"

#if defined(_WINDOWS)
#include "ftd_error.h"

char * ftd_GetIoctlValue(unsigned int uiIoctlValue);

//#define PSTORE_MUTEX_WITH_DRIVER
static  HANDLE g_hEvent = INVALID_HANDLE_VALUE;

#ifdef _DEBUG
static char dbgbuf[256];
#endif

HANDLE ftd_open(char *device, int AccessFlags, int Permissions)
{
    HANDLE hndFile;
    DWORD dwDesiredAccess = 0;
    DWORD dwShareMode = 0;
    DWORD dwCreationDisposition = 0;
    DWORD dwFlagsAndAttributes = 0;
#ifdef _DEBUG
    error_tracef(TRACEINF5,"ftd_open(): device=%s",device);
#endif
#ifdef OLD_PSTORE
/* XXX JRL this should be put back in?
    if ( !(AccessFlags & O_SYNC) ) {
        return (HANDLE)-1;
    }
*/
#else
    if ( (AccessFlags & O_SYNC) ) {
        dwDesiredAccess |= SYNCHRONIZE;
    }
#endif
    if ( (AccessFlags & 0xF) == O_RDWR) {
        dwDesiredAccess |= GENERIC_READ | GENERIC_WRITE;
    }
    else if ((AccessFlags & 0xF) == O_RDONLY) {
        dwDesiredAccess |= GENERIC_READ;
    }
    else if ((AccessFlags & 0xF) == O_WRONLY) {
        dwDesiredAccess |= GENERIC_WRITE;
    }

    if ( !(AccessFlags & O_EXCL) ) {
        dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    }

    if (AccessFlags & O_CREAT) {
        dwCreationDisposition = CREATE_ALWAYS;
    } else {
#ifdef OLD_PSTORE
        dwCreationDisposition = OPEN_EXISTING;
#else
        dwCreationDisposition = OPEN_ALWAYS;
#endif
    }

#ifdef PSTORE_MUTEX_WITH_DRIVER
    if ( g_hEvent != INVALID_HANDLE_VALUE ){
        error_tracef(TRACEINF5,"ftd_open(): g_hEvent already has a value !!");
    }
    g_hEvent = OpenEvent(0,0,"syncaccesspstorefile");
    if ( g_hEvent != INVALID_HANDLE_VALUE )
    {
        error_tracef(TRACEINF5,"ftd_open(): before Wait...");
        DWORD status = WaitForSingleObject(g_hEvent,INFINITE);
        error_tracef(TRACEINF5,"ftd_open(): after  Wait... this thread owns the event");
    }
#endif //#ifdef PSTORE_MUTEX_WITH_DRIVER
  
    hndFile = CreateFile(
        device,
        dwDesiredAccess,
        dwShareMode,
        NULL,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        NULL );

    if (hndFile == INVALID_HANDLE_VALUE)
    {   // Don't report error if we try to get an \\Global\tdmf\ctl handle , could be a SECONDARY mode.
		if ( strcmp( device, FTD_CTLDEV ) ) 
		{
			DWORD err = GetLastError();     
			error_tracef(TRACEERR,"ftd_open(%s): ERROR=%d handle=%x ",device, err, hndFile);
		}
    }

#ifdef _DEBUG
    sprintf(dbgbuf, "ftd_open(): device=%s, handle=%x\n", device, hndFile);
    OutputDebugString(dbgbuf);

    error_tracef(TRACEINF5,"ftd_open(): device=%s, handle=%x", device, hndFile);
#endif
    return hndFile;
}

int ftd_ioctl(HANDLE hndFile, int ioctl, void *buf, int size)
{                
    DWORD   dwNumberOfBytesReturned;
    int     IoctlResult;

    //
    // In Traceinf6 mode show what ioctl is being sent to the driver!
    //
    error_tracef(TRACEINF6, "ftd_ioctl(): handle=0x%08x ioctl=%s [0x%08x]",
                            hndFile,
                            ftd_GetIoctlValue(ioctl),
                            ioctl);

    IoctlResult = DeviceIoControl(
                            hndFile,    // Handle to device
                            ioctl,      // IO Control code for Read
                            buf,        // Input Buffer to driver.
                            size,       // Length of buffer in bytes.
                            buf,        // Output Buffer from driver.
                            size,       // Length of buffer in bytes.
                            &dwNumberOfBytesReturned,    // Bytes placed in DataBuffer.
                            NULL        // NULL means wait till op. completes.
                            );

    if ( !IoctlResult)                        // Did the IOCTL succeed?
    {
        error_tracef(TRACEERR   ,"ftd_ioctl(): handle=0x%08x  ioctl=%s [0x%08x] error=0x%08x", 
                                hndFile, 
                                ftd_GetIoctlValue(ioctl),
                                ioctl,
                                GetLastError());
        return -1;
    }

    return 0;
}

ftd_uint64_t ftd_llseek(HANDLE hndFile, ftd_uint64_t offset, int origin)
{
    LARGE_INTEGER nBytes;

#ifdef _DEBUG
    error_tracef(TRACEINF5,"ftd_llseek(): handle=%x ", hndFile);
#endif

    nBytes.QuadPart = offset;

    nBytes.LowPart = SetFilePointer(hndFile, nBytes.LowPart, &nBytes.HighPart, (DWORD)origin);
    if ( (nBytes.LowPart == 0xFFFFFFFF) && (GetLastError() != NO_ERROR) )
        nBytes.QuadPart = -1;

    return nBytes.QuadPart;
}

int ftd_lockf(HANDLE hndFile, int ioctl, unsigned __int64 offset, unsigned __int64 length)
{
    LARGE_INTEGER       nOffset, nBytes;
    OVERLAPPED          ol;
    int                 ret;
#ifdef _DEBUG
    error_tracef(TRACEINF5,"ftd_lockf(): handle=%x ", hndFile);
#endif
    memset(&ol, 0, sizeof(OVERLAPPED));

    nOffset.QuadPart = offset;
    nBytes.QuadPart = length;

    ol.Offset = nOffset.LowPart;
    ol.OffsetHigh = nOffset.HighPart;

    if (ioctl == F_TLOCK) {
        ret = !LockFileEx(hndFile, LOCKFILE_EXCLUSIVE_LOCK, 0, nBytes.LowPart, nBytes.HighPart, &ol);
    }
    else {
        ret = !UnlockFileEx(hndFile, 0, nBytes.LowPart, nBytes.HighPart, &ol);
    }
    return ret;
}

int ftd_fsync(HANDLE hndFile)
{
    return FlushFileBuffers(hndFile);
}

int ftd_write(HANDLE hndFile, void *buffer, unsigned int count)
{
    DWORD   Retries;
    DWORD   dwNumberOfBytesToWrite  = count;
    DWORD   dwNumberOfBytesWriten; 
    DWORD   err;
    DWORD   ret;
    DWORD   ipieces                 = 0;
    DWORD   BytesWritten;
    char *  TempBuffer;
#ifdef _DEBUG
    error_tracef(TRACEINF5,"ftd_write(): handle=%x ", hndFile);
#endif
    ret = WriteFile(hndFile, 
                    buffer, 
                    dwNumberOfBytesToWrite, // number of bytes to write! 
                    &dwNumberOfBytesToWrite,// pointer to number of bytes written
                    NULL );
    if (ret)
    {
        return count;
    }


    err = GetLastError();

    //
    // Only continue on quota errors
    //
    if (err != ERROR_WORKING_SET_QUOTA)
    {
        error_tracef(TRACEERR,"ftd_fwrite(): ERROR=%d handle=%x ", err, hndFile);
        return -1;
    }
    else
    {
#ifdef _DEBUG
        sprintf(dbgbuf, "ftd_write(): handle=%x ERROR_WORKING_SET_QUOTA\n", hndFile);
        OutputDebugString(dbgbuf);
#endif
        error_tracef(TRACEINF4,"ftd_write(): ERROR=%d handle=%x Attempting retries", err, hndFile);
    }

    //
    // Ok, if we had problems writing the file because of quota problems
    // we will try to write the file in in segments rather than in a big
    // block. This will reduce our needs in terms of quota
    //
    // We also sleep between retries to be sure we are able to do everything!
    //

    {
        BytesWritten            = 0;
        dwNumberOfBytesToWrite  = 4096; // write a multiple of sector size (512/1024)

        while(BytesWritten < count)
        {
            //
            // Write dwNumberOfBytesWrite bytes or whatever is left to write
            //
            if (dwNumberOfBytesToWrite > (count - BytesWritten))
            {
                dwNumberOfBytesToWrite = (count - BytesWritten);
			}

            Retries = 10;

            while (Retries--)
            {
                TempBuffer = (char *)buffer + BytesWritten;

                ret = WriteFile(hndFile, 
                                (void *)TempBuffer, 
                                dwNumberOfBytesToWrite, // number of bytes to write! 
                                &dwNumberOfBytesWriten,// pointer to number of bytes written
                                NULL );

#ifdef _DEBUG
                if(dwNumberOfBytesWriten != dwNumberOfBytesToWrite)
                {
                    sprintf(dbgbuf, 
                            "ASSERT! Number of bytes written=%ld number wanted=%ld\n",
                            dwNumberOfBytesWriten,
                            dwNumberOfBytesToWrite);
                    OutputDebugString(dbgbuf);
                }
#endif
                if (ret)
                {
                    ipieces++;
                    // writefile succeeded! We are so out of here!
                    break; // breaks out of while(retries--) loop
                }
                else
                {
                    err = GetLastError();
                    //
                    // On NON QUOTA errors, GET OUT OF HERE!
                    // 
                    if (err != ERROR_WORKING_SET_QUOTA)
                    {
#ifdef _DEBUG
                        sprintf(dbgbuf, "ftd_write(): ERROR=%d\n", err);
                        OutputDebugString(dbgbuf);
#endif
                        error_tracef(TRACEERR,"ftd_fwrite(): ERROR=%d handle=%x ", err, hndFile);
                        return -1;
                    }
                    else
                    {
#ifdef _DEBUG
                        sprintf(dbgbuf, "ftd_write(): ERROR=%d RETRYING\n", err);
                        OutputDebugString(dbgbuf);
#endif
                        error_tracef(TRACEINF4,"ftd_write(): ERROR=%d handle=%x RETRYING", err, hndFile);
                    }

                    //
                    // We sleep 100ms and try again!
                    //
                    Sleep(100);
                }
            }// end while(retries--)

            if (!ret)
            {
#ifdef _DEBUG
                sprintf(dbgbuf, "ftd_write(): ERROR=%d RAN OUT OF RETRIES!\n", err);
                OutputDebugString(dbgbuf);
#endif
                error_tracef(TRACEERR,"ftd_fwrite(): ERROR=%d handle=%x Ran out of retries!", err, hndFile);
                return -1;
            }

            //
            // Ok we wrote the bytes, add the value...
            //
            BytesWritten += dwNumberOfBytesToWrite;
        }//end while(BytesWritten < Count)

    }

#ifdef _DEBUG
    sprintf(dbgbuf, "ftd_write(): size=%d in %d pieces\n", BytesWritten, ipieces);
    OutputDebugString(dbgbuf);
#endif

    return count;
}

ssize_t ftd_writev(HANDLE hndFile, const struct iovec *iov, int size)
{
    DWORD dwNumberOfBytesToWrite;
    ssize_t dwNumberOfBytesWritten = 0;
    int i;
#ifdef _DEBUG
    error_tracef(TRACEINF5,"ftd_writev(): handle=%x ", hndFile);
#endif
    for (i = 0; i < size; i++)
    {
        dwNumberOfBytesToWrite = iov[i].iov_len;
        if (!WriteFile(hndFile, 
            iov[i].iov_base, 
            dwNumberOfBytesToWrite,  // number of bytes to write ! not 512
            &dwNumberOfBytesToWrite,  // pointer to number of bytes written
            NULL ) )
        {
            DWORD err = GetLastError();
            error_tracef(TRACEERR,"ftd_writev(): ERROR=%d handle=%x ", err, hndFile);
            return -1;
        }
        else {
            dwNumberOfBytesWritten += iov[i].iov_len;
        }
    }

    return dwNumberOfBytesWritten;
}

int ftd_read(HANDLE hndFile, void *buffer, unsigned int count)
{
    DWORD   Retries;
    DWORD   dwNumberOfBytesRead;
    DWORD dwNumberOfBytesToRead = count;
    DWORD   err;
    DWORD   ret;
    DWORD   ipieces                 = 0;
    DWORD   BytesRead;
    char *  TempBuffer;


    error_tracef(TRACEINF5,"ftd_read(): handle=%x ", hndFile);

    ret = ReadFile( hndFile, 
        buffer, 
                    dwNumberOfBytesToRead,  // number of bytes to read! 
        &dwNumberOfBytesToRead,  // pointer to number of bytes written
                    NULL );
    if (ret)
    {
        return count;
    }


    err = GetLastError();

    //
    // Only continue on quota errors
    //
    if (err != ERROR_WORKING_SET_QUOTA)
    {
        error_tracef(TRACEERR,"ftd_read(): ERROR=%d handle=%x ", err, hndFile);
        return -1;
    }
    else
    {
#ifdef _DEBUG
        sprintf(dbgbuf, "ftd_read(): ERROR_WORKING_SET_QUOTA\n", hndFile);
        OutputDebugString(dbgbuf);
#endif
        
        error_tracef(TRACEINF4,"ftd_read(): ERROR=%d handle=%x Attempting retries", err, hndFile);
    }

    //
    // Ok, if we had problems reading the file because of quota problems
    // we will try to read the file in in segments rather than in a big
    // block. This will reduce our needs in terms of quota
    //
    // We also sleep between retries to be sure we are able to do everything!
    //

    {
        BytesRead               = 0;
        dwNumberOfBytesToRead   = 4096; // read a multiple of sector size (512/1024)

        while(BytesRead < count)
        {
            //
            // Read dwNumberOfBytesToRead bytes or whatever is left to read
            //
            if (dwNumberOfBytesToRead > (count - BytesRead))
            {
                dwNumberOfBytesToRead = (count - BytesRead);
            }

            Retries = 10;

            while (Retries--)
            {
                TempBuffer = (char *)buffer + BytesRead;

                ret = ReadFile( hndFile, 
                                (void *)TempBuffer, 
                                dwNumberOfBytesToRead,  // number of bytes to read ! 
                                &dwNumberOfBytesRead,  // pointer to number of bytes written
                                NULL );

#ifdef _DEBUG
                if(dwNumberOfBytesRead != dwNumberOfBytesToRead)
                {
                    sprintf(dbgbuf, 
                            "ASSERT! Number of bytes read=%ld number wanted=%ld\n",
                            dwNumberOfBytesRead,
                            dwNumberOfBytesToRead);
                    OutputDebugString(dbgbuf);
                }
#endif

                if (ret)
                {
                    ipieces++;
                    // readfile succeeded! We are so out of here!
                    break; // breaks out of while(retries--) loop
                }
                else
                {
                    err = GetLastError();
                    //
                    // On NON QUOTA errors, GET OUT OF HERE!
                    // 
                    if (err != ERROR_WORKING_SET_QUOTA)
                    {
#ifdef _DEBUG
                        sprintf(dbgbuf, "ftd_read(): ERROR=%d\n", err);
                        OutputDebugString(dbgbuf);
#endif
                        error_tracef(TRACEERR,"ftd_read(): ERROR=%d handle=%x ", err, hndFile);
                        return -1;
                    }
                    else
                    {
#ifdef _DEBUG
                        sprintf(dbgbuf, "ftd_read(): ERROR=%d RETRYING\n", err);
                        OutputDebugString(dbgbuf);
#endif
                        error_tracef(TRACEINF4,"ftd_read(): ERROR=%d handle=%x RETRYING", err, hndFile);
                    }

                    //
                    // We sleep 100ms and try again!
                    //
                    Sleep(100);
                }
            }// end while(retries--)

            if (!ret)
            {
#ifdef _DEBUG
                sprintf(dbgbuf, "ftd_read(): ERROR=%d RAN OUT OF RETRIES\n", err);
                OutputDebugString(dbgbuf);
#endif
                error_tracef(TRACEERR,"ftd_read(): ERROR=%d handle=%x Ran out of retries!", err, hndFile);
                return -1;
            }

            //
            // Ok we read the bytes, add the value...
            //
            BytesRead += dwNumberOfBytesToRead;
        }//end while(BytesRead < Count)

    }

#ifdef _DEBUG
    sprintf(dbgbuf, "ftd_read(): size=%d in %d pieces\n", BytesRead, ipieces);
    OutputDebugString(dbgbuf);
#endif

    return count;
}

#ifdef TDMF_TRACE
int ftd_close(char *mod,int line,HANDLE hndFile)
#else
int ftd_close(HANDLE hndFile)
#endif
{
    int lbClosed;
#ifdef _DEBUG
    error_tracef(TRACEINF5,"ftd_close(): handle=%x ", hndFile);
#endif

    //
    // Only close a valid handle!
    //
    if (hndFile && (hndFile != INVALID_HANDLE_VALUE))
    {
    lbClosed = CloseHandle(hndFile);
    }
    else
    {
        lbClosed = FALSE; 
    }

#ifdef PSTORE_MUTEX_WITH_DRIVER
    if ( g_hEvent != INVALID_HANDLE_VALUE )
    {
        BOOL b= SetEvent(g_hEvent);
        CloseHandle(g_hEvent);
        g_hEvent = INVALID_HANDLE_VALUE;
    }
    else
        error_tracef(TRACEERR,"ftd_close(): INVALID_HANDLE_VALUE ");
#endif // PSTORE_MUTEX_WITH_DRIVER

    if ( !lbClosed )
    {
        DWORD err = GetLastError();
#if _DEBUG
        sprintf(dbgbuf, "ftd_close(): ERROR=%d handle=%x\n", err, hndFile);
        OutputDebugString(dbgbuf);
#endif
        #ifdef TDMF_TRACE
        error_tracef(TRACEINF5,"mod %s, line %d,ftd_close(): ERROR=%d handle=%x ", mod,line,err, hndFile);
        #else
        error_tracef(TRACEINF5,"ftd_close(): ERROR=%d handle=%x ", err, hndFile);
        #endif
    }

#if _DEBUG
    sprintf(dbgbuf, "ftd_close(): handle=%x\n", hndFile);
    OutputDebugString(dbgbuf);
#endif

    return lbClosed;
} // ftd_close ()


/*
 * force_dsk_or_rdsk --
 * force a path to have //./ in it
 * and add it to the end if it isn't there
 */
void
force_dsk_or_rdsk (char* pathout, char* pathin, int tordskflag)
{
    if (tordskflag) {
        strcpy(pathout, pathin + (strlen(pathin) - 2));
    } else {
        sprintf(pathout, TEXT("\\\\.\\%c:"), pathin[0]);
    }
}

#else /* defined(_WINDOWS) */

#define INVALID_HANDLE_VALUE (HANDLE)-1

HANDLE
ftd_open(char *name, int mode, int permis)
{

    return open(name, mode);
}

ftd_uint64_t
ftd_llseek(HANDLE fd, ftd_uint64_t offset, int whence)
{

    return llseek(fd, offset, whence);
}

int
ftd_read(HANDLE fd, void *buf, int len)
{

    return read(fd, buf, len);
}

int
ftd_write(HANDLE fd, void *buf, int len)
{

    return write(fd, buf, len);
}

int
ftd_writev(HANDLE fd, struct iovec *iov, int iovcnt)
{

    return writev(fd, iov, iovcnt);
}

int
ftd_close(HANDLE fd)
{

    return close(fd);
}

int
ftd_fsync(HANDLE fd)
{

    return fsync(fd);
}

int
ftd_lockf(HANDLE fd, int ioctl, ftd_uint64_t offset, ftd_uint64_t len)
{

    return lockf(fd, ioctl, len);
}

int
ftd_ioctl(HANDLE fd, int cmd, void *buf, int len)
{

    return ioctl(fd, cmd, buf);
}
#endif

#if defined(_WINDOWS)
/*
 * capture_all_devs -- write out all devices found on a system to
 *                     the specified output device
 */
void
capture_all_devs (SOCKET fd)
{
    DWORD   dwDriveMask;
    LPTSTR  lpszRootPathName=TEXT("?:\\");
    char    szWindowsDir[_MAX_PATH];
    long    sectors;
    double  size;
    char    outbuf[256];

    if ( !GetWindowsDirectory(szWindowsDir, sizeof(szWindowsDir)) )
        return;

    dwDriveMask=GetLogicalDrives();

    //
    // walk list
    //
    for (*lpszRootPathName=TEXT('a'); *lpszRootPathName<=TEXT('z'); (*lpszRootPathName)++) {
        if (dwDriveMask & 1) {   // drive exists

        if ( GetDriveType(lpszRootPathName) != DRIVE_FIXED ) // we only want fixed
            continue;

        if ( !strncmp(lpszRootPathName, szWindowsDir, strlen(lpszRootPathName)) ) // not the system disk
            continue;

        sectors = disksize(lpszRootPathName);
        size = ((double) sectors * (double) DEV_BSIZE) / 1024.0;
        if (size >= 1.0 && size < 1024) {
            sprintf (outbuf, "{%s %d %.2f KB / %lu SECT %s} ",
                     lpszRootPathName, 0, size, sectors, lpszRootPathName);
        }
        size = size / 1024.0;
        if (size >= 1.0 && size < 1024) {
            sprintf (outbuf, "{%s %d %.2f MB / %lu SECT %s} ",
                     lpszRootPathName, 0, size, sectors, lpszRootPathName);
        }
        size = size / 1024.0;
        if (size >= 1.0 && size < 1024) {
            sprintf (outbuf, "{%s %d %.2f GB / %lu SECT %s} ",
                     lpszRootPathName, 0, size, sectors, lpszRootPathName);
        }

        send (fd, (void*) outbuf, strlen(outbuf), 0);
        }
    }
}
#else

#ifdef NEED_SYS_MNTTAB
#include "ftd_mnttab.h"
#else
#include <sys/mnttab.h>
#endif

extern daddr_t fdisksize (int fd);
extern daddr_t disksize (char* devicepath); 

/*
 * force_dsk_or_rdsk_path -- force a path to have either /rdsk or /dsk in it
 *                           and add it to the end if it isn't there
 */
#if defined(_AIX)
/*-
 * for AIX, force_dsk_or_rdsk() is functionally equivalent
 * to HPUX convert_lv_name(), with some additional paranoia. 
 *
 * the HPUX vers of convert_lv_name() tests whether a device
 * node name is S_ISCHR by parsing the name. 
 * for AIX, stat(2) the in-parameter device node name to make 
 * this determination. 
 * this will hopefully avoid confused behavior in cases where
 * device node names start with 'r'.
 */
void
force_dsk_or_rdsk(char *pathout, char *pathin, int to_char)
{
    int             len;
    char           *src, *dest;
    struct stat     sb;
    int             is_char;

    /* whether name is a char device node */
    if (!stat(pathin, &sb))
        is_char = S_ISCHR(sb.st_mode) ? 1 : 0;
    else
        is_char = -1;

    if (is_char >= 0) {

        /*-
         * name exists, and node type is determined
         */

        /* check for trivial cases */
        if ((is_char && to_char) ||
            (!is_char && !to_char)) {
            strcpy(pathout, pathin);
            return;
        }

        /*-
         * special case.
         * look for /dsk and /rdsk 
         * 
         */
        {
            char *rdsk;
            char *dsk;
            rdsk = strstr(pathin, "/rdsk");
            dsk = strstr(pathin, "/dsk");
            if( dsk || rdsk) {
                if ( rdsk )
                    src = rdsk;
                else
                    src = dsk;
            }
        }

        /* conversion cases */
        src = strrchr(pathin, '/');
        len = (src - pathin) + 1;
        strncpy(pathout, pathin, len);
        dest = pathout + len;
        if (to_char) 
            *dest++ = 'r'; /* add an 'r' */
        else 
            src++;  /* skip an 'r' */
        strcpy(dest, src + 1);

        return;

    } else {

        /*-
         * name doesn't exist, type isn't known with 
         * certainty since it hasn't been created yet.
         * just parse the name ala HPUX...
         */
        if ((src = strrchr(pathin, '/')) != NULL) {

            /*-
             * special case.
             * look for /dsk and /rdsk 
             * 
             */
            {
                char *rdsk;
                char *dsk;
                rdsk = strstr(pathin, "/rdsk");
                dsk = strstr(pathin, "/dsk");
                if( dsk || rdsk) {
                    if ( rdsk )
                        src = rdsk;
                    else
                        src = dsk;
                }
            }
      
            if (*(src + 1) == 'r' && to_char != 0) {
                /*
                 * -- if the device is already a raw device,
                 * do nothing 
                 */
                strcpy(pathout, pathin);
                return;
            }
            if (*(src + 1) != 'r' && to_char == 0) {
                /*
                 * -- the device is already not a raw device,
                 * do nothing 
                 */
                strcpy(pathout, pathin);
                return;
            }
            len = (src - pathin) + 1;
            strncpy(pathout, pathin, len);
            dest = pathout + len;
            if (to_char) {
                *dest++ = 'r';
            } else {
                src++;  /* skip the 'r' */
            }
            strcpy(dest, src + 1);

        } else {    /* can't happen */
            strcpy(pathout, pathin);
        }

        return;
    }
}
#else /* defined(_AIX) */
void
force_dsk_or_rdsk (char* pathout, char* pathin, int tordskflag)
{
    int i, len, foundflag;

    foundflag = 0;
    len = strlen(pathin);
    for (i=len-4; i>3; i--) {
        if (pathin[i] == '/') {
            if (0 == strncmp(&pathin[i+1], "rdsk/", 5)) {
                if (tordskflag) {
                    strcpy (pathout, pathin);
                } else {
                    strncpy (pathout, pathin, i+1);
                    strncpy (&pathout[i+1], "dsk/", 4);
                    strncpy (&pathout[i+5], &pathin[i+6], (len-i)-6);
                    pathout[len-1] = '\0';
                }
                foundflag = 1;
                break;
            }
            if (0 == strncmp(&pathin[i+1], "dsk/", 4)) {
                if (!tordskflag) {
                    strcpy (pathout, pathin);
                } else {
                    strncpy (pathout, pathin, i+1);
                    strncpy (&pathout[i+1], "rdsk/", 5);
                    strncpy (&pathout[i+6], &pathin[i+5], (len-i)-5);
                    pathout[len+1] = '\0';
                }
                foundflag = 1;
                break;
            }
        }
    }
    if (!foundflag) {
        strcpy (pathout, pathin);
        len--;
        while (pathout[len] == '/')
            len--;
        strcpy (&pathout[len+1], (tordskflag) ? "/rdsk": "/dsk");
    }
}
#endif /* !defined(_AIX) */

#if defined(HPUX)

#include <sys/diskio.h>
#include <sys/param.h>
#include <sys/pstat.h>

void
convert_lv_name (char *pathout, char *pathin, int to_char)
{
    int len;
    char *src, *dest;

    if ((src = strrchr(pathin, '/')) != NULL) {
        if (*(src+1) == 'r' && to_char != 0) {
            /* -- if the device is already a raw device, do nothing */
            strcpy (pathout, pathin);
            return;
        }
        if (*(src+1) != 'r' && to_char == 0) {
            /* -- the device is already not a raw device, do nothing */
            strcpy (pathout, pathin);
            return;
        }
        len = (src - pathin) + 1;
        strncpy(pathout, pathin, len);
        dest = pathout + len;
        if (to_char) {
            *dest++ = 'r';
        } else { 
            src++; /* skip the 'r' */
        }
        strcpy(dest, src+1);
    } else { /* can't happen */
        strcpy(pathout, pathin);
    }
}

/*
 * Return TRUE if devname is a logical volume.
 */
int 
is_logical_volume(char *devname)
{
    int         lvm_major;
    struct stat statbuf;

    if (stat(devname, &statbuf) != 0) {
        return 0;
    }

    /* this is hard coded for now, but we really should find it dynamically */
    lvm_major = 64;

    if ((statbuf.st_rdev >> 24) == lvm_major) {
        return 1;
    }
    return 0;
}

/*
 * Given a block device name, get the size of the device. The character
 * device must be inquired for this information, so we convert the name.
 */
static int
get_char_device_info(char *dev_name, int is_lv, unsigned long *sectors)
{
    capacity_type cap;
    disk_describe_type describe;
    char raw_name[MAXPATHLENLEN];
    int fd;

    if (is_lv) {
        convert_lv_name(raw_name, dev_name, 1);
    } else  {
        force_dsk_or_rdsk(raw_name, dev_name, 1);
    }

    if ((fd = open(raw_name, O_RDWR | O_EXCL)) == -1) { 
        return -1;
    }
    if (ioctl(fd, DIOC_DESCRIBE, &describe) < 0) {
        close(fd);
        return -2;
    }
    /* if the device is a CD-ROM, blow it off */
    if (describe.dev_type == CDROM_DEV_TYPE) {
        close(fd);
        return -3;
    }
    if (ioctl(fd, DIOC_CAPACITY, &cap) < 0) {
        close(fd);
        return -4;
    }

    *sectors = cap.lba;

    close(fd);
    return 0;
}

/*
 * dev_info -- return information about a specific character device
 * This is a bit different from the Solaris version. HPUX allows access to
 * the underlying disk devices owned by the logical volume manager. Logical
 * volumes can have any name underneath the /dev tree, as long as they are
 * unique, so we can't rely on any naming convention or location for logical 
 * volumes. We know the device is a logical volume because it's major number
 * matches the major number of the lvm driver (64). lvm uses minor numbers to
 * uniquely identify the logical volumes. The lvm driver always uses 64 as
 * it's major number as far as we know. The character device file for logical
 * volumes is stored in the same directory as the block device, but it is
 * prefaced with an 'r' (e.g. /dev/vg00/rlvol1 vs. /dev/vg00/lvol1).
 */
int
dev_info(char *devname, int *inuseflag, u_long *sectors, char *mntpnt)
{
    struct stat statbuf;
    struct pst_lvinfo lv;
    struct pst_diskinfo disk;
    char block_name[MAXPATHLENLEN];
    unsigned int device_num, lvm_major;
    int idx, status;

    FILE* fd;
    struct mnttab mp;
    struct mnttab mpref;

    *inuseflag = -1;
    *sectors = 0;
    *mntpnt = 0;
    if (stat(devname, &statbuf) != 0) {
        return -1;
    }
    device_num = statbuf.st_rdev;

    /* this is hard coded for now, but we really should find it dynamically */
    lvm_major = 64;

    if ((device_num >> 24) == lvm_major) {
        if (!S_ISBLK(statbuf.st_mode)) {
            convert_lv_name(block_name, devname, 0);
        } else {
            strcpy(block_name, devname);
        }
        status = get_char_device_info(block_name, 1, sectors);
        if (status < 0) {
            return status;
        }
        *inuseflag = 0; 

        /* search all logical volumes for a matching minor number */
        idx = 0;
        while ((status = pstat_getlv(&lv, sizeof(lv), 1, idx)) > 0) {
            if (lv.psl_dev.psd_minor == (device_num & 0xffffff)) {
                *inuseflag = 1; 
                break;
            }
            idx++;
        }
    } else {
        /* force the device name to be a block device name */
        if (!S_ISBLK(statbuf.st_mode)) {
            force_dsk_or_rdsk(block_name, devname, 0);

            /* we need the device number for the block device */
            if (stat(block_name, &statbuf) != 0) {
                return -1;
            }
            device_num = statbuf.st_rdev;
        } else {
            strcpy(block_name, devname);
        }
        /* hideous bug found in HPUX: the disk table is sometimes bogus and
           an entry may be missing. Assume it's in use just to be safe. */
        *inuseflag = -1;

        status = get_char_device_info(block_name, 0, sectors);
        if (status < 0) {
            return status;
        }

        /* search the disk devices for a matching device number */
        idx = 0;
        while ((status = pstat_getdisk(&disk, sizeof(disk), 1, idx)) > 0) {
            if ((disk.psd_dev.psd_minor == (device_num & 0xffffff)) &&
                (disk.psd_dev.psd_major == (device_num >> 24))) {
                *inuseflag = disk.psd_status;
                break;
            }
            idx++;
        }
    }
    /* check the mount table for the mount directory, if its in use */
    *mntpnt = 0;
    if (*inuseflag) {
        if ((FILE*)NULL != (fd = fopen ("/etc/mnttab", "r"))) 
        {
            mpref.mnt_special = block_name;
            mpref.mnt_mountp = (char*)NULL;
            mpref.mnt_fstype = (char*)NULL;
            mpref.mnt_mntopts = (char*)NULL;
            mpref.mnt_time = (char*)NULL;
            if (0 == getmntany (fd, &mp, &mpref)) {
                strcpy (mntpnt, mp.mnt_mountp);
            }
            fclose (fd);
        }
    }
    return 0;
}

/*
 * capture_logical_volumes -- return a buffer of all device information for 
 *                     all logical volumes
 */
int
capture_logical_volumes (int fd)
{
    FILE *f;
    int status;
    char command[] = "/etc/vgdisplay -v";
    char tempbuf[MAXPATHLENLEN];
    char raw_name[MAXPATHLENLEN];
    char *ptr, *end;
    int inuseflag;
    u_long sectors;
    char mntpnt[MAXPATHLENLEN];
    char outbuf[MAXPATHLENLEN];
    double size;

    if ((f = popen(command, "r")) != NULL) {
        while (fgets(tempbuf, MAXPATHLENLEN, f) != NULL) {
            if ((ptr = strstr(tempbuf, "LV Name")) != NULL) {
                ptr += 8;
                while (*ptr && isspace(*ptr)) ptr++;
                end = ptr;
                while (*end && !isspace(*end)) end++;
                *end = 0;
                convert_lv_name (raw_name, ptr, 1);
                /* -- get sundry information about the device */
                if (0 <= dev_info (raw_name, &inuseflag, &sectors, mntpnt)) {
                    size = ((double) sectors * (double) DEV_BSIZE) / 1024.0;
                    if (size >= 1.0 && size < 1024) {
                        sprintf (outbuf, "{%s %d %.2f KB / %lu SECT %s} ",
                                 raw_name, inuseflag, size, sectors, mntpnt);
                    }
                    size = size / 1024.0;
                    if (size >= 1.0 && size < 1024) {
                        sprintf (outbuf, "{%s %d %.2f MB / %lu SECT %s} ",
                                 raw_name, inuseflag, size, sectors, mntpnt);
                    }
                    size = size / 1024.0;
                    if (size >= 1.0 && size < 1024) {
                        sprintf (outbuf, "{%s %d %.2f GB / %lu SECT %s} ",
                                 raw_name, inuseflag, size, sectors, mntpnt);
                    }
                } else {
                    sprintf (outbuf, "{%s 0 0.00 MB / 0 SECT } ", raw_name);
                }
                write (fd, (void*) outbuf, strlen(outbuf));
            }
        }
        status = pclose(f);
    }
    return 0;
}

#endif /* HPUX */

#if defined(_AIX)
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <lvm.h>
#include <sys/types.h>
#include <sys/sysconfig.h>
#include "aixcmn.h"
#include "ftd_mnttab.h"

/*-
 * for AIX, things are, well, *real* different.
 * 
 * for one, all process access to physical DAD 
 * storage is through the logical volume driver. 
 * this simplifies the implementation of the
 * AIX vers of this module. however...
 *
 * the disk driver node naming convention bears 
 * no semblance to that for SOLARIS or HPUX.
 * here, rather than rely on pattern matching
 * device node names, liblvm subroutines are used 
 * to enumerate all of the lv's available to the
 * system. 
 * 
 * here, interesting properties for each found lv 
 * are determined, and output as with other vers.
 *
 */

/* format of output string */
#define OUTFMTSTR "{%s %d %.2f %s / %lu SECT %s} "

/*-
 * get the address of the logical volume
 * driver ddconfig entry point. this is
 * used with liblbm subr's.
 */
mid_t
hd_config_addr()
{
    struct cfg_load cfg_ld;
    char            hddrvnm[64];

    memset(&cfg_ld, 0, sizeof(struct cfg_load));

    cfg_ld.kmid = 0;
    cfg_ld.path = "/etc/drivers/hd_pin";
    cfg_ld.libpath = NULL;

    if (sysconfig(SYS_QUERYLOAD, &cfg_ld, sizeof(struct cfg_load)) == -1) {
        return (0);
    }
    return (cfg_ld.kmid);

}

/*-
 * use liblvm subr's to enumerate all available 
 * lv's configured in the system. for each found 
 * lv: determine some interesting properties,
 * format and write output to the descriptor
 * given by the fd parm.
 */
static char            pbuf[1024];
static char            bdevice[128];
static char            cdevice[128];
static char            mntedon[1024];
int
queryvgs(int fd, mid_t addr)
{
    struct queryvgs *qvgs;
    struct queryvg *qvgp;
    struct querylv *qlv;
    struct lv_array *lva;
    long            num_vgs;
    long            num_lvs;
    int             lvmerr;
    int             vgndx;
    int             lvndx;
    long            csize;
    double          fsize;
    char           *units;
    int             inuseflag;
    unsigned long   sectcnt;
    int             tfd;
    struct mnttab   mp;
    struct mnttab   mpref;


    
    /* query for all available volume groups */
    lvmerr = lvm_queryvgs(&qvgs, addr);
    if (lvmerr)
        return (lvmerr);
    num_vgs = qvgs->num_vgs;

    /* enumerate each volume group */
    for (vgndx = 0; vgndx < num_vgs; vgndx++) {

        lvmerr = lvm_queryvg(&qvgs->vgs[vgndx].vg_id, &qvgp, NULL);
        if (lvmerr)
            return (lvmerr);
        num_lvs = qvgp->num_lvs;

        /* enumerate each logical volume */
        for (lvndx = 0; lvndx < num_lvs; lvndx++) {

            lvmerr = lvm_querylv(&qvgp->lvs[lvndx].lv_id,
                         &qlv, (char *) NULL);
            if (lvmerr)
                return (lvmerr);

            /*-
             * presume the logical volume was created with 
             * a default path name, that it lives in "/dev/".
             */
            sprintf(bdevice, "/dev/%s", qlv->lvname);
            sprintf(cdevice, "/dev/r%s", qlv->lvname);

            /*-
             * in the SOLARIS and HPUX vers, the following
             * code is used to check whether some process
             * is using the device directly. apparently there,
             * if the device contains a mounted filesystem
             * the open(2) fails, and in subsequent processing
             * whether and where it is mounted is determined.
             * well, for AIX, this open(2) always succeeds,
             * regardless of whether the device is mounted.
             * for AIX then, check whether the device is 
             * mounted, regardless of what open(2) returns. 
             * if so, then revise the notion of "in use"
             * accordingly, presuming that the intended
             * semantics of "in use" apply to devices mounted
             * as filesystems...
             */
            inuseflag = 0;
            if (-1 == (tfd = open(cdevice, O_RDWR | O_EXCL))) {
                inuseflag = 1;
            } else {
                (void) close(tfd);
            }
            if (-1 == (tfd = open(bdevice, O_RDWR | O_EXCL))) {
                inuseflag = 1;
            } else {
                (void) close(tfd);
            }

            /*-
             * whether and where device contains mounted filesystem
             */
            memset(&mp, 0, sizeof(mp));
            mpref.mnt_special = bdevice;
            mpref.mnt_mountp = (char *) NULL;
            mpref.mnt_fstype = (char *) NULL;
            mpref.mnt_mntopts = (char *) NULL;
            mpref.mnt_time = (char *) NULL;
            if ((0 == getmntany(0, &mp, &mpref)) &&
                (NULL != mp.mnt_mountp) &&
                (0 < strlen(mp.mnt_mountp))) {
                strcpy(mntedon, mp.mnt_mountp);
                inuseflag=1;
            }
            else
                mntedon[0] = 0;

            /*-
             * compute sizes of things, format and write output
             */
            csize = qlv->currentsize * (2 << (qlv->ppsize - 1));
            sectcnt = (csize >> DEV_BSHIFT);
            fsize = (double) csize;
            fsize = fsize / 1024.0;
            if (fsize >= 1.0 && fsize < 1024.0) {
                units = "KB";
                sprintf(&pbuf[0], OUTFMTSTR, cdevice,
                 inuseflag, fsize, units, sectcnt, mntedon);
            }
            fsize = fsize / 1024.0;
            if (fsize >= 1.0 && fsize < 1024.0) {
                units = "MB";
                sprintf(&pbuf[0], OUTFMTSTR, cdevice,
                 inuseflag, fsize, units, sectcnt, mntedon);
            }
            fsize = fsize / 1024.0;
            if (fsize >= 1.0) {
                units = "GB";
                sprintf(&pbuf[0], OUTFMTSTR, cdevice,
                 inuseflag, fsize, units, sectcnt, mntedon);
            }
            write(fd, &pbuf[0], strlen(&pbuf[0]));

        }
    }
}

void
enum_lvs(int fd)
{
    queryvgs(fd, hd_config_addr());
}
#endif  /* defined (_AIX) */

/*
 * capture_devs_in_dir -- reports on all devices in the given directories
 */
void
capture_devs_in_dir (int fd, char* cdevpath, char* bdevpath)
{
    char cdevice[MAXPATHLEN];
    char bdevice[MAXPATHLEN];
    DIR* dfd;
    struct dirent* dent;
    struct stat cstatbuf;
    struct stat bstatbuf;
    FILE* f;
    struct mnttab mp;
    struct mnttab mpref;
    int inuseflag; 
    unsigned long sectors;
    long tsectors;
    char mntpnt[MAXPATHLEN];
    char templine[256];
    double size;
    int tfd;

    if (((DIR*)NULL) == (dfd = opendir (cdevpath))) return;
    f = fopen ("/etc/mnttab", "r");
    while (NULL != (dent = readdir(dfd))) {
        if (strcmp(dent->d_name, ".") == 0) continue;
        if (strcmp(dent->d_name, "..") == 0) continue;
        sprintf (cdevice, "%s/%s", cdevpath, dent->d_name);
        sprintf (bdevice, "%s/%s", bdevpath, dent->d_name);
#if !defined(_WINDOWS)
        errno = 0;
#endif
        inuseflag = 0;
        mntpnt[0] = '\0';
        templine[0] = '\0';

        if (0 != stat (cdevice, &cstatbuf)) continue;
        if (!(S_ISCHR(cstatbuf.st_mode))) continue;
        if (0 != stat (bdevice, &bstatbuf)) continue;
        if (!(S_ISBLK(bstatbuf.st_mode))) continue;
    
        tsectors = (long) disksize (cdevice);
        sectors = (unsigned long) tsectors;
        if (tsectors == -1 || 0L == sectors || 0xffffffff == sectors) {
            continue;
        }
        if (-1 == (tfd = open (cdevice, O_RDWR | O_EXCL))) {
            inuseflag = 1;
        } else {
            (void) close (tfd);
        }
        if (-1 == (tfd = open (bdevice, O_RDWR | O_EXCL))) {
            inuseflag = 1;
        } else {
            (void) close (tfd);
        }
        if (inuseflag && f != (FILE*)NULL) {
            rewind(f);
            mpref.mnt_special = bdevice;
            mpref.mnt_mountp = (char*)NULL;
            mpref.mnt_fstype = (char*)NULL;
            mpref.mnt_mntopts = (char*)NULL;
            mpref.mnt_time = (char*)NULL;
            if (0 == getmntany (f, &mp, &mpref)) {
                if ((char*)NULL != mp.mnt_mountp && 0 < strlen(mp.mnt_mountp)) {
                    strcpy (mntpnt, mp.mnt_mountp);
                }
            }
        }
        /* calculate the size in gigabytes. */
        size = ((double)sectors * (double)DEV_BSIZE) / 1024.0;
        if (size >= 1.0 && size < 1024.0) {
            sprintf (templine, "{%s %d %.2f KB / %lu SECT %s} ", cdevice,
                inuseflag, size, sectors, mntpnt);
        } 
        size = size / 1024.0;
        if  (size >= 1.0 && size < 1024.0) {
            sprintf (templine, "{%s %d %.2f MB / %lu SECT %s} ", cdevice,
                inuseflag, size, sectors, mntpnt);
        }
        size = size / 1024.0;
        if  (size >= 1.0) {
            sprintf (templine, "{%s %d %.2f GB / %lu SECT %s} ", cdevice,
                inuseflag, size, sectors, mntpnt);
        }
        write (fd, (void*) templine, strlen(templine));
    }
    if (f != (FILE*)NULL) fclose(f);
}

/*
 * walk_rdsk_subdirs_for_devs -- recursively walks a directory tree that
 *                               already has rdsk in it for their devices
 */
void
walk_rdsk_subdirs_for_devs (int fd, char* rootdir)
{
    DIR* dfd;
    DIR* cdevfd;
    DIR* bdevfd;
    struct dirent* dent;
    char cdevdirpath[MAXPATHLEN];
    char bdevdirpath[MAXPATHLEN];

    if (((DIR*)NULL) == (dfd = opendir (rootdir))) return;
    while (NULL != (dent = readdir(dfd))) {
        if (0 == strcmp(dent->d_name, ".")) continue;
        if (0 == strcmp(dent->d_name, "..")) continue;
        sprintf (cdevdirpath, "%s/%s", rootdir, dent->d_name);
        force_dsk_or_rdsk (bdevdirpath, cdevdirpath, 0);
        /* -- open the character device directory */
        if (((DIR*)NULL) == (cdevfd = opendir (cdevdirpath))) continue;
        (void) closedir (cdevfd);
        if (((DIR*)NULL) == (bdevfd = opendir (bdevdirpath))) continue;
        (void) closedir (bdevfd);
        capture_devs_in_dir (fd, cdevdirpath, bdevdirpath);
        walk_rdsk_subdirs_for_devs (fd, cdevdirpath);
    }
    (void) closedir (dfd);
}

/*
 * walk_dirs_for_devs -- walks a directory tree, seeing if rdsk/dsk 
 *                          subdirectories exist, and report on their
 *                          devices
 */
void
walk_dirs_for_devs (int fd, char* rootdir)
{
    DIR* dfd;
    DIR* cdevfd;
    DIR* bdevfd;
    struct dirent* dent;
    char cdevdirpath[MAXPATHLEN];
    char bdevdirpath[MAXPATHLEN];

    if (((DIR*)NULL) == (dfd = opendir (rootdir))) return;
    while (NULL != (dent = readdir(dfd))) {
        if (0 == strcmp(dent->d_name, ".")) continue;
        if (0 == strcmp(dent->d_name, "..")) continue;
        /* -- create character and block device directory paths */
        if (0 == strcmp(dent->d_name, "rdsk")) continue;
        if (0 == strcmp(dent->d_name, "dsk")) continue;
        sprintf (cdevdirpath, "%s/%s/rdsk", rootdir, dent->d_name);
        sprintf (bdevdirpath, "%s/%s/dsk", rootdir, dent->d_name);
        /* -- open the character device directory */
        if (((DIR*)NULL) == (cdevfd = opendir (cdevdirpath))) continue;
        (void) closedir (cdevfd);
        if (((DIR*)NULL) == (bdevfd = opendir (bdevdirpath))) continue;
        (void) closedir (bdevfd);
        capture_devs_in_dir (fd, cdevdirpath, bdevdirpath);
    }
    (void) closedir (dfd);
}

/*
 * capture_all_devs -- write out all devices found on a system to
 *                     the specified output device
 */
void
capture_all_devs (SOCKET fd)
{
    return;
}

#endif /* defined(_WINDOWS) */

char pUnknownIoctlString[] = "Unknown Ioctl";
//
// return ioctl string for specified value
//
char * ftd_GetIoctlValue(unsigned int uiIoctlValue)
{
    int     i,
            result = -1;

    for (i = 0; i < MAX_TDMF_IOCTL; i++)
    {
        if (tdmf_ioctl_text_array[i].cmd ==  uiIoctlValue)
        {
            result = i;
            break;
        }
    }

    if (result)
    {
        return tdmf_ioctl_text_array[result].cmd_txt;
    }
    else
    {
        return pUnknownIoctlString;
    }
}

