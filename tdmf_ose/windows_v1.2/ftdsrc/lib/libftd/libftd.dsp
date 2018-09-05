# Microsoft Developer Studio Project File - Name="libftd" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libftd - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libftd.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libftd.mak" CFG="libftd - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libftd - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libftd - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/SafeData/lib/libftd", OWAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libftd - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
F90=df.exe
# ADD BASE F90 /include:"Release/"
# ADD F90 /include:"Release/"
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /Z7 /O2 /I "..\liblic2" /I "..\liberr" /I "..\liblic" /I "..\liblst" /I "..\libcomp" /I "..\libsock" /I "..\libutil" /I "..\libproc" /I "..\..\driver" /I "..\libmd5" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "_WINDOWS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libftd - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
F90=df.exe
# ADD BASE F90 /include:"Debug/"
# ADD F90 /include:"Debug/"
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\liblic2" /I "..\liberr" /I "..\liblic" /I "..\liblst" /I "..\libcomp" /I "..\libsock" /I "..\libutil" /I "..\libproc" /I "..\..\driver" /I "..\libmd5" /D "_DEBUG" /D "_WINDOWS" /D "WIN32" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libftd - Win32 Release"
# Name "libftd - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ftd_cfgsys.c
# End Source File
# Begin Source File

SOURCE=.\ftd_config.c
# End Source File
# Begin Source File

SOURCE=.\ftd_cpoint.c
# End Source File
# Begin Source File

SOURCE=.\ftd_dev.c
# End Source File
# Begin Source File

SOURCE=.\ftd_devio.c
# End Source File
# Begin Source File

SOURCE=.\ftd_devlock.c
# End Source File
# Begin Source File

SOURCE=.\ftd_error.c
# End Source File
# Begin Source File

SOURCE=.\ftd_fsm.c
# End Source File
# Begin Source File

SOURCE=.\ftd_ioctl.c
# End Source File
# Begin Source File

SOURCE=.\ftd_journal.c
# End Source File
# Begin Source File

SOURCE=.\ftd_lg.c
# End Source File
# Begin Source File

SOURCE=.\ftd_lic.c
# End Source File
# Begin Source File

SOURCE=.\ftd_pathnames.c
# End Source File
# Begin Source File

SOURCE=.\ftd_perf.c
# End Source File
# Begin Source File

SOURCE=.\ftd_platform.c
# End Source File
# Begin Source File

SOURCE=.\ftd_proc.c
# End Source File
# Begin Source File

SOURCE=.\ftd_ps.c
# End Source File
# Begin Source File

SOURCE=.\ftd_rsync.c
# End Source File
# Begin Source File

SOURCE=.\ftd_sock.c
# End Source File
# Begin Source File

SOURCE=.\ftd_stat.c
# End Source File
# Begin Source File

SOURCE=..\..\version.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ftd.h
# End Source File
# Begin Source File

SOURCE=.\ftd_cfgsys.h
# End Source File
# Begin Source File

SOURCE=.\ftd_config.h
# End Source File
# Begin Source File

SOURCE=.\ftd_cpoint.h
# End Source File
# Begin Source File

SOURCE=.\ftd_dev.h
# End Source File
# Begin Source File

SOURCE=.\ftd_devio.h
# End Source File
# Begin Source File

SOURCE=.\ftd_devlock.h
# End Source File
# Begin Source File

SOURCE=.\ftd_error.h
# End Source File
# Begin Source File

SOURCE=.\ftd_fsm.h
# End Source File
# Begin Source File

SOURCE=.\ftd_fsm_tab.h
# End Source File
# Begin Source File

SOURCE=.\ftd_ioctl.h
# End Source File
# Begin Source File

SOURCE=.\ftd_journal.h
# End Source File
# Begin Source File

SOURCE=.\ftd_lg.h
# End Source File
# Begin Source File

SOURCE=.\ftd_lg_states.h
# End Source File
# Begin Source File

SOURCE=.\ftd_lic.h
# End Source File
# Begin Source File

SOURCE=.\ftd_pathnames.h
# End Source File
# Begin Source File

SOURCE=.\ftd_perf.h
# End Source File
# Begin Source File

SOURCE=.\ftd_platform.h
# End Source File
# Begin Source File

SOURCE=.\ftd_port.h
# End Source File
# Begin Source File

SOURCE=.\ftd_port_unix.h
# End Source File
# Begin Source File

SOURCE=.\ftd_port_win.h
# End Source File
# Begin Source File

SOURCE=.\ftd_proc.h
# End Source File
# Begin Source File

SOURCE=.\ftd_ps.h
# End Source File
# Begin Source File

SOURCE=.\ftd_ps_pvt.h
# End Source File
# Begin Source File

SOURCE=.\ftd_rsync.h
# End Source File
# Begin Source File

SOURCE=.\ftd_sock.h
# End Source File
# Begin Source File

SOURCE=.\ftd_sock_t.h
# End Source File
# Begin Source File

SOURCE=.\ftd_stat.h
# End Source File
# Begin Source File

SOURCE=.\ftd_throt.h
# End Source File
# Begin Source File

SOURCE=..\..\driver\ftdio.h
# End Source File
# End Group
# End Target
# End Project
