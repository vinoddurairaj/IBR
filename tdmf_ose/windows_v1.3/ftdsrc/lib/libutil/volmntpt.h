#ifndef _VOLMNTPT_H_
#define _VOLMNTPT_H_

/*
 * volmntpt.h -- Volume Mount Points routines
 *
 * (c) Copyright 1999 Legato Systems, Inc. All Rights Reserved
 *
 */

#if defined(_WINDOWS)

#ifdef __cplusplus
extern "C"{ 
#endif

#define BUFSIZE 4096        
#define FILESYSNAMEBUFSIZE MAX_PATH
//
// Macro that defines what a "DosDevice" drive letter is.  This macro can
// be used to scan the incoming device name string before passing it to 
// Mount Point Manager.
//
#define IS_VALID_DRIVE_LETTER(s) (\
		s[0] == '\\' &&           \
		s[1] == 'D' &&            \
		s[2] == 'o' &&            \
		s[3] == 's' &&            \
		s[4] == 'D' &&            \
		s[5] == 'e' &&            \
		s[6] == 'v' &&            \
		s[7] == 'i' &&            \
		s[8] == 'c' &&            \
		s[9] == 'e' &&            \
		s[10] == 's' &&           \
		s[11] == '\\' &&          \
		s[12] >= 'A' &&           \
		s[12] <= 'Z' &&           \
		s[13] == ':')

#define IS_VALID_DEVICE_NAME(s) ( \
		s[0] == '\\' &&           \
		s[1] == 'D'  &&           \
		s[2] == 'e'  &&           \
		s[3] == 'v'  &&           \
		s[4] == 'i'  &&           \
		s[5] == 'c'  &&           \
		s[6] == 'e'  &&           \
		s[7] == '\\' &&           \
		s[8] == 'H'  &&           \
		s[9] == 'a'  &&           \
		s[10] == 'r' &&           \
		s[11] == 'd' &&           \
		s[12] == 'd' &&           \
		s[13] == 'i' &&           \
		s[14] == 's' &&           \
		s[15] == 'k')

//
// Macro that defines what a "volume name" mount point is.  This macro can
// be used to scan the result from QUERY_POINTS to discover which mount points
// are "volume name" mount points.
//
#define IS_VALID_VOLUME_NAME(s) (          \
		s[0] == '\\' &&					   \
	    (s[1] == '?' ||  s[1] == '\\') && \
		s[2] == '?' &&			           \
		s[3] == '\\' &&                    \
		s[4] == 'V' &&                     \
		s[5] == 'o' &&                     \
		s[6] == 'l' &&                     \
		s[7] == 'u' &&                     \
		s[8] == 'm' &&                     \
		s[9] == 'e' &&                     \
		s[10] == '{' &&                    \
		s[19] == '-' &&                    \
		s[24] == '-' &&                    \
		s[29] == '-' &&                    \
		s[34] == '-' &&                    \
		s[47] == '}')

#define NO_MNT_PT_INFO      0
#define VALID_MNT_PT_INFO	1
#define ERROR_MNT_PT_INFO  -1

enum  MntPtMngrData { 
	DEVICE_NAME = 1,
	DOS_DEVICE,
	VOLUME_GUID
}; 

#pragma pack()

extern int QueryVolMntPtInfoFromDevName(char *pszDeviceName, int piDevStrSize, char *pszMntPtInfo, int piGuidStrSize, 
					                    unsigned int piMntPtType);

extern HANDLE getFirstVolMntPtHandle( char *szVolumeGuid, char *PtBuf , unsigned long dwBuffer );
extern BOOL getVolumeNameForVolMntPt ( char *szVolumeMntPt, char *szVolumePath, unsigned long dwBuffer );
extern void closeVolMntPtHandle( HANDLE hPt );
extern BOOL getNextVolMntPt ( HANDLE hPt, char *PtBuf , unsigned long dwBuffer );
extern BOOL delVolMntPnt( char *szMountPoint  );
extern BOOL setVolMntPnt( char *lpszVolumeMountPoint, char *pszVolumeName );

#ifdef __cplusplus 
}
#endif

#endif /* _WINDOWS */

#endif

