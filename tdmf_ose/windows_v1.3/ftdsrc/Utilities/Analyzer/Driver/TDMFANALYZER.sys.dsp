# Microsoft Developer Studio Project File - Name="TDMFANALYZER.sys" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=TDMFANALYZER.sys - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "TDMFANALYZER.sys.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "TDMFANALYZER.sys.mak" CFG="TDMFANALYZER.sys - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TDMFANALYZER.sys - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TDMFANALYZER.sys - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "Desktop"
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "TDMFANALYZER.sys - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "HSComm_D"
# PROP BASE Intermediate_Dir "HSComm_D"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\i386\free"
# PROP Intermediate_Dir ".\i386\free"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /Gz /MT /W4 /Zi /Od /Gf /Gy /I "." /I "..\include" /I "..\common" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_X86_" /D "_I386" /D "STD_CALL" /D "CONDITION_HANDLING" /D "WIN32_LEAN_AND_MEAN" /D "NT_UP" /D WIN32=100 /D _NTIX_=100 /D "_IDWBUILD" /D "RDRDBG" /D "SRVDBG" /U "NT_INST" /U "FPO" /YX /FD /c
# ADD CPP /nologo /G5 /Gz /MT /W4 /ZI /Od /I "..\include" /I "..\common" /I "$(BASEDIR)\inc" /I "$(BASEDIR)\inc\ddk" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_X86_" /D "_I386" /D "STD_CALL" /D "CONDITION_HANDLING" /D "WIN32_LEAN_AND_MEAN" /D "NT_UP" /D WIN32=100 /D _NTIX_=100 /D "_IDWBUILD" /D "RDRDBG" /D "SRVDBG" /U "NT_INST" /U "FPO" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /fo".\i386\free\HSComm.res" /d "NDEBUG"
# ADD RSC /l 0x409 /fo".\i386\free\HSComm.res" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o".\i386\free/HSComm.bsc"
# ADD BSC32 /nologo /o".\i386\free/HSComm.bsc"
LINK32=link.exe
# ADD BASE LINK32 Z:\lib\i386\free\ntoskrnl.lib Z:\lib\i386\free\hal.lib Z:\lib\i386\free\int64.lib /nologo /entry:"DriverEntry@8" /dll /debug /machine:I386 /nodefaultlib /out:".\i386\free/HSComm.sys" /pdbtype:sept /subsystem:native
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 $(BASEDIR)\libfre\i386\ntoskrnl.lib $(BASEDIR)\libfre\i386\hal.lib $(BASEDIR)\libfre\i386\int64.lib /nologo /entry:"DriverEntry@8" /dll /debug /machine:I386 /nodefaultlib /out:".\i386\free/TDMFANALYZER.sys" /pdbtype:sept /subsystem:native
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "TDMFANALYZER.sys - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "HSComm_0"
# PROP BASE Intermediate_Dir "HSComm_0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\i386\checked"
# PROP Intermediate_Dir ".\i386\checked"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /Gz /MTd /W4 /Zi /Od /Gf /Gy /I "." /I "..\include" /I "..\common" /D "_DEBUG" /D "DBG" /D "WIN32" /D "_WINDOWS" /D "_X86_" /D "_I386" /D "STD_CALL" /D "CONDITION_HANDLING" /D "WIN32_LEAN_AND_MEAN" /D "NT_UP" /D WIN32=100 /D _NTIX_=100 /D "_IDWBUILD" /D "RDRDBG" /D "SRVDBG" /U "NT_INST" /U "FPO" /Fr /YX /FD /c
# ADD CPP /nologo /G5 /Gz /MTd /W4 /ZI /Od /I "..\include" /I "..\common" /I "$(BASEDIR)\inc" /I "$(BASEDIR)\inc\ddk" /D "_DEBUG" /D "DBG" /D "WIN32" /D "_WINDOWS" /D "_X86_" /D "_I386" /D "STD_CALL" /D "CONDITION_HANDLING" /D "WIN32_LEAN_AND_MEAN" /D "NT_UP" /D WIN32=100 /D _NTIX_=100 /D "_IDWBUILD" /D "RDRDBG" /D "SRVDBG" /U "NT_INST" /U "FPO" /Fr /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /fo".\i386\checked\HSComm.res" /d "_DEBUG"
# ADD RSC /l 0x409 /fo".\i386\checked\HSComm.res" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o".\i386\checked/HSComm.bsc"
# ADD BSC32 /nologo /o".\i386\checked/HSComm.bsc"
LINK32=link.exe
# ADD BASE LINK32 Z:\lib\i386\checked\ntoskrnl.lib Z:\lib\i386\checked\hal.lib Z:\lib\i386\checked\int64.lib /nologo /entry:"DriverEntry@8" /dll /incremental:no /debug /machine:I386 /nodefaultlib /out:".\i386\checked/HSComm.sys" /pdbtype:sept /subsystem:native
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 $(BASEDIR)\libchk\i386\ntoskrnl.lib $(BASEDIR)\libchk\i386\hal.lib $(BASEDIR)\libchk\i386\int64.lib /nologo /entry:"DriverEntry@8" /dll /incremental:no /debug /machine:I386 /nodefaultlib /out:".\i386\checked/TDMFANALYZER.sys" /pdbtype:sept /subsystem:native
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "TDMFANALYZER.sys - Win32 Release"
# Name "TDMFANALYZER.sys - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "*.cpp; *.c"
# Begin Source File

SOURCE=.\TDMFANALYZER.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\TDMFANALYZER.h
# End Source File
# End Group
# Begin Group "Resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\TDMFANALYZER.rc
# End Source File
# End Group
# End Target
# End Project
