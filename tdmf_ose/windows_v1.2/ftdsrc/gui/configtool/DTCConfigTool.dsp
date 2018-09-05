# Microsoft Developer Studio Project File - Name="DTCConfigTool" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=DTCConfigTool - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "DTCConfigTool.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DTCConfigTool.mak" CFG="DTCConfigTool - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DTCConfigTool - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "DTCConfigTool - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/ftdsrc/gui/configtool", PJFAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "DTCConfigTool - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
F90=df.exe
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /Z7 /O2 /I "..\..\lib\libutil" /I "..\..\driver" /I "..\..\lib\libftd" /I "..\..\lib\libsock" /I "..\..\lib\liblic" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "MTI"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 $(TOP)\$(LIBSOCKREL) $(TOP)\$(LIBUTILREL) $(TOP)\$(LIBFTDREL) $(TOP)\$(LIBLICREL) ws2_32.lib /nologo /subsystem:windows /debug /debugtype:both /machine:I386 /out:"Release\configtool.exe" /opt:ref,icf
# SUBTRACT LINK32 /pdb:none /nodefaultlib
# Begin Special Build Tool
TargetPath=.\Release\configtool.exe
TargetName=configtool
SOURCE="$(InputPath)"
PostBuild_Cmds=md $(TOP)\"$(PRODUCTNAME)"	copy $(TargetPath) $(TOP)\"$(PRODUCTNAME)"\$(QNM)$(TargetName).exe
# End Special Build Tool

!ELSEIF  "$(CFG)" == "DTCConfigTool - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
F90=df.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\lib\libutil" /I "..\..\driver" /I "..\..\lib\libftd" /I "..\..\lib\libsock" /I "..\..\lib\liblic" /D "WIN32" /D "_MBCS" /D "_DEBUG" /D "_WINDOWS" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 $(TOP)\$(LIBLICDBG) $(TOP)\$(LIBSOCKDBG) $(TOP)\$(LIBUTILDBG) $(TOP)\$(LIBFTDDBG) ws2_32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Debug/configtool.exe" /pdbtype:sept
# SUBTRACT LINK32 /verbose /nodefaultlib
# Begin Special Build Tool
TargetPath=.\Debug\configtool.exe
TargetName=configtool
SOURCE="$(InputPath)"
PostBuild_Cmds=md $(TOP)\DEBUG\"$(PRODUCTNAME)"	copy $(TargetPath) $(TOP)\DEBUG\"$(PRODUCTNAME)"\$(QNM)$(TargetName).exe
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "DTCConfigTool - Win32 Release"
# Name "DTCConfigTool - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AddGroup.cpp
# End Source File
# Begin Source File

SOURCE=.\AddModMirrors.cpp
# End Source File
# Begin Source File

SOURCE=.\Config.cpp
# End Source File
# Begin Source File

SOURCE=.\DeleteGroup.cpp
# End Source File
# Begin Source File

SOURCE=.\DTCConfigPropSheet.cpp
# End Source File
# Begin Source File

SOURCE=.\DTCConfigTool.cpp
# End Source File
# Begin Source File

SOURCE=.\DTCConfigTool.rc
# End Source File
# Begin Source File

SOURCE=.\DTCConfigToolDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\DTCDevices.cpp
# End Source File
# Begin Source File

SOURCE=.\Hostname.cpp
# End Source File
# Begin Source File

SOURCE=.\Splash.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\System.cpp
# End Source File
# Begin Source File

SOURCE=.\Throttles.cpp
# End Source File
# Begin Source File

SOURCE=.\TunableParams.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AddGroup.h
# End Source File
# Begin Source File

SOURCE=.\AddModMirrors.h
# End Source File
# Begin Source File

SOURCE=.\Config.h
# End Source File
# Begin Source File

SOURCE=.\DeleteGroup.h
# End Source File
# Begin Source File

SOURCE=.\DTCConfigPropSheet.h
# End Source File
# Begin Source File

SOURCE=.\DTCConfigTool.h
# End Source File
# Begin Source File

SOURCE=.\DTCConfigToolDlg.h
# End Source File
# Begin Source File

SOURCE=.\DTCDevices.h
# End Source File
# Begin Source File

SOURCE=.\Hostname.h
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
# Begin Source File

SOURCE=.\System.h
# End Source File
# Begin Source File

SOURCE=.\Throttles.h
# End Source File
# Begin Source File

SOURCE=.\TunableParams.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\DTCConfigTool.ico
# End Source File
# Begin Source File

SOURCE=.\res\DTCConfigTool.rc2
# End Source File
# Begin Source File

SOURCE=.\res\DTCLogo.ico
# End Source File
# Begin Source File

SOURCE=.\res\ico00001.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon_dtc.ico
# End Source File
# Begin Source File

SOURCE=.\res\Icon_legato.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon_log.ico
# End Source File
# Begin Source File

SOURCE=.\res\mti.ico
# End Source File
# Begin Source File

SOURCE=.\res\sftk.ico
# End Source File
# Begin Source File

SOURCE=.\res\stk.ico
# End Source File
# Begin Source File

SOURCE=.\res\TDMFlogo202.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
