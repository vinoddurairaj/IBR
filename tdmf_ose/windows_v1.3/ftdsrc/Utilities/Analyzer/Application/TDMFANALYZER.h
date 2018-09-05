#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"

void    LoadDriverNow(void);
void    CreateAllLists(void);
void    ListFileHandleListOnAllVolumes(void);
void    ListFileHandleListOnGUID(char * pcGUID);
void    ListVolumes(void);
void    DisplayFullModuleName(unsigned int hModule);

extern bool    gbVerbose;
