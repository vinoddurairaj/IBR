# Microsoft Developer Studio Project File - Name="libDB" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libDB - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libDB.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libDB.mak" CFG="libDB - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libDB - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libDB - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libDB - Win32 Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
F90=df.exe
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0xc0c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libDB - Win32 Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
F90=df.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0xc0c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libDB - Win32 Release"
# Name "libDB - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\FsTdmfDb.cpp
# End Source File
# Begin Source File

SOURCE=.\FsTdmfRec.cpp
# End Source File
# Begin Source File

SOURCE=.\FsTdmfRecAlert.cpp
# End Source File
# Begin Source File

SOURCE=.\FsTdmfRecCmd.cpp
# End Source File
# Begin Source File

SOURCE=.\FsTdmfRecDom.cpp
# End Source File
# Begin Source File

SOURCE=.\FsTdmfRecHum.cpp
# End Source File
# Begin Source File

SOURCE=.\FsTdmfRecKeyLog.cpp
# End Source File
# Begin Source File

SOURCE=.\FsTdmfRecLgGrp.cpp
# End Source File
# Begin Source File

SOURCE=.\FsTdmfRecNvp.cpp
# End Source File
# Begin Source File

SOURCE=.\FsTdmfRecPair.cpp
# End Source File
# Begin Source File

SOURCE=.\FsTdmfRecPerf.cpp
# End Source File
# Begin Source File

SOURCE=.\FsTdmfRecSrvInf.cpp
# End Source File
# Begin Source File

SOURCE=.\FsTdmfRecSrvScript.cpp
# End Source File
# Begin Source File

SOURCE=.\FsTdmfRecSysLog.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\FsTdmfDb.h
# End Source File
# Begin Source File

SOURCE=.\FsTdmfEtc.h
# End Source File
# Begin Source File

SOURCE=.\FsTdmfRec.h
# End Source File
# Begin Source File

SOURCE=.\FsTdmfRecAlert.h
# End Source File
# Begin Source File

SOURCE=.\FsTdmfRecCmd.h
# End Source File
# Begin Source File

SOURCE=.\FsTdmfRecDom.h
# End Source File
# Begin Source File

SOURCE=.\FsTdmfRecGroup.h
# End Source File
# Begin Source File

SOURCE=.\FsTdmfRecHum.h
# End Source File
# Begin Source File

SOURCE=.\FsTdmfRecKeyLog.h
# End Source File
# Begin Source File

SOURCE=.\FsTdmfRecLgGrp.h
# End Source File
# Begin Source File

SOURCE=.\FsTdmfRecNvp.h
# End Source File
# Begin Source File

SOURCE=.\FsTdmfRecNvpNames.h
# End Source File
# Begin Source File

SOURCE=.\FsTdmfRecPair.h
# End Source File
# Begin Source File

SOURCE=.\FsTdmfRecPerf.h
# End Source File
# Begin Source File

SOURCE=.\FsTdmfRecSrvInf.h
# End Source File
# Begin Source File

SOURCE=.\FsTdmfRecSrvScript.h
# End Source File
# Begin Source File

SOURCE=.\FsTdmfRecSysLog.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\FsTdmfDbUpgrade.txt
# End Source File
# Begin Source File

SOURCE=.\Readme.txt
# End Source File
# End Target
# End Project
