# Microsoft Developer Studio Project File - Name="MonitorTool" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=MonitorTool - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "MonitorTool.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "MonitorTool.mak" CFG="MonitorTool - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MonitorTool - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "MonitorTool - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/ftdsrc/gui/MonitorTool", ILFAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "MonitorTool - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
F90=df.exe
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /Z7 /O2 /I "..\..\lib\libftd" /I "..\..\lib\liblst" /I "..\..\lib\libsock" /I "..\..\lib\libutil" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL" /d "MTI"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 msvcrt.lib LIBCMT.lib $(TOP)\$(LIBLSTREL) $(TOP)\$(LIBFTDREL) $(TOP)\$(LIBSOCKREL) $(TOP)\$(LIBUTILREL) ws2_32.lib /nologo /subsystem:windows /debug /debugtype:both /machine:I386 /nodefaultlib:"msvcrt.lib LIBCMT.lib" /opt:ref,icf
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
TargetPath=.\Release\MonitorTool.exe
TargetName=MonitorTool
SOURCE="$(InputPath)"
PostBuild_Cmds=md $(TOP)\"$(PRODUCTNAME)"	copy $(TargetPath) $(TOP)\"$(PRODUCTNAME)"\$(QNM)$(TargetName).exe
# End Special Build Tool

!ELSEIF  "$(CFG)" == "MonitorTool - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
F90=df.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\lib\libftd" /I "..\..\lib\liblst" /I "..\..\lib\libsock" /I "..\..\lib\libutil" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 $(TOP)\$(LIBLSTDBG) $(TOP)\$(LIBFTDDBG) $(TOP)\$(LIBSOCKDBG) $(TOP)\$(LIBUTILDBG) ws2_32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /map
# Begin Special Build Tool
TargetPath=.\Debug\MonitorTool.exe
TargetName=MonitorTool
SOURCE="$(InputPath)"
PostBuild_Cmds=md $(TOP)\DEBUG\"$(PRODUCTNAME)"	copy $(TargetPath) $(TOP)\DEBUG\"$(PRODUCTNAME)"\$(QNM)$(TargetName).exe
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "MonitorTool - Win32 Release"
# Name "MonitorTool - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ETSLayout.cpp
# End Source File
# Begin Source File

SOURCE=.\MonitorTool.cpp
# End Source File
# Begin Source File

SOURCE=.\MonitorTool.rc
# End Source File
# Begin Source File

SOURCE=.\MonitorToolDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Splash.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ETSLayout.h
# End Source File
# Begin Source File

SOURCE=.\MonitorTool.h
# End Source File
# Begin Source File

SOURCE=.\MonitorToolDlg.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\Splash.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\drivestack.ico
# End Source File
# Begin Source File

SOURCE=.\res\drivestack2.ico
# End Source File
# Begin Source File

SOURCE=.\res\drivestackred.ico
# End Source File
# Begin Source File

SOURCE=.\Green.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ico00001.ico
# End Source File
# Begin Source File

SOURCE=.\res\ico00002.ico
# End Source File
# Begin Source File

SOURCE=.\res\ico00003.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon_dtc.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon_gre.ico
# End Source File
# Begin Source File

SOURCE=.\res\Icon_legato.ico
# End Source File
# Begin Source File

SOURCE=.\res\MonitorTool.ico
# End Source File
# Begin Source File

SOURCE=.\res\MonitorTool.rc2
# End Source File
# Begin Source File

SOURCE=.\res\mti.ico
# End Source File
# Begin Source File

SOURCE=.\Red.bmp
# End Source File
# Begin Source File

SOURCE=.\res\sftk.ico
# End Source File
# Begin Source File

SOURCE=.\res\singledrivegreen.ico
# End Source File
# Begin Source File

SOURCE=.\res\singledrivered.ico
# End Source File
# Begin Source File

SOURCE=.\res\stk.ico
# End Source File
# Begin Source File

SOURCE=.\res\TDMFlogo202.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Trffc10a.ico
# End Source File
# Begin Source File

SOURCE=.\res\Trffc10c.ico
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
