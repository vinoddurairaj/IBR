#define NO_MNT_PT_INFO      0
#define VALID_MNT_PT_INFO	1
#define ERROR_MNT_PT_INFO  -1

enum  MntPtMngrData { 
	DEVICE_NAME = 1,
	DOS_DEVICE,
	VOLUME_GUID
}; 

int     QueryVolMntPtInfoFromDevName(char *pszDeviceName, int piDevStrSize, char *pszMntPtInfo, int piGuidStrSize, 
					             unsigned int piMntPtType);

int UtilGetGUIDforVolumeMountPoint(IN  char *          szVolumeMountPoint, 
                                   OUT char *          szVolumeGUID, 
                                   IN  unsigned long   ulBufSize            );
int UtilGetNextVolumeMountPoint(   IN  HANDLE          hVolumeHandle, 
                                   OUT char *          szVolumeMountPoint, 
                                   IN  unsigned long   ulBufSize            );
HANDLE UtilGetVolumeHandleAndFirstMountPoint(   
                                    IN  char *          szVolumeGUID, 
                                    OUT char *          szVolumeMountPoint, 
                                    IN  unsigned long   ulBufSize           );
void UtilCloseVolumeHandle(         IN  HANDLE          hVolume );

void DisplayTerminationString(void);
