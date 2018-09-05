# Microsoft Developer Studio Project File - Name="TdmfCommonGui" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=TDMFCOMMONGUI - WIN32 DEBUG
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "TdmfCommonGui.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "TdmfCommonGui.mak" CFG="TDMFCOMMONGUI - WIN32 DEBUG"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TdmfCommonGui - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "TdmfCommonGui - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "TdmfCommonGui - Win32 Release"

# PROP BASE Use_MFC 5
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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GR /GX /O2 /I "..\..\lib\libftd" /I "..\..\lib\libmngt" /I "..\..\libsock" /I "..\..\lib\liblic" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 Shlwapi.lib Ws2_32.lib Pdh.lib Htmlhelp.lib version.lib $(TOP)\lib\libResMgr\Release\libResMgr.lib /nologo /subsystem:windows /pdb:none /machine:I386 /out:"Release/DtcCommonGui.exe"
# Begin Special Build Tool
TargetPath=.\Release\DtcCommonGui.exe
SOURCE="$(InputPath)"
PostBuild_Cmds=md $(TOP)\"$(PRODUCTNAME)"\$(TOS)	copy $(TargetPath) $(TOP)\"$(PRODUCTNAME)"\$(TOS)	copy ContextMenus.reg $(TOP)\"$(PRODUCTNAME)"\$(TOS)
# End Special Build Tool

!ELSEIF  "$(CFG)" == "TdmfCommonGui - Win32 Debug"

# PROP BASE Use_MFC 5
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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GR /GX /ZI /Od /I "..\..\lib\libftd" /I "..\..\lib\libmngt" /I "..\..\libsock" /I "..\..\lib\liblic" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 Shlwapi.lib Ws2_32.lib Pdh.lib Htmlhelp.lib version.lib $(TOP)\lib\libResMgr\Debug\libResMgr.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Debug/DtcCommonGui.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "TdmfCommonGui - Win32 Release"
# Name "TdmfCommonGui - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ColumnSelectionDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\DebugMonitor.cpp
# End Source File
# Begin Source File

SOURCE=.\DomainDetailsPage.cpp
# End Source File
# Begin Source File

SOURCE=.\DomainGeneralPage.cpp
# End Source File
# Begin Source File

SOURCE=.\DomainPropertySheet.cpp
# End Source File
# Begin Source File

SOURCE=.\EmptyPage.cpp
# End Source File
# Begin Source File

SOURCE=.\EventProperties.cpp
# End Source File
# Begin Source File

SOURCE=.\exports.cpp
# End Source File
# Begin Source File

SOURCE=.\ImportScriptDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\IncludeSpMBaseModule.cpp
# End Source File
# Begin Source File

SOURCE=.\LocationEdit.cpp
# End Source File
# Begin Source File

SOURCE=.\LoginDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\LogViewerDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\MessageByType.cpp
# End Source File
# Begin Source File

SOURCE=.\Messenger.cpp
# End Source File
# Begin Source File

SOURCE=.\NewScriptServerFileName.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionAdminPage.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionGeneralPage.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionPropertySheet.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionsDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionsRegKeyPage.cpp
# End Source File
# Begin Source File

SOURCE=.\ProgressInfoDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\PropertyPageBase.cpp
# End Source File
# Begin Source File

SOURCE=.\ReplicationGroupPropertySheet.cpp
# End Source File
# Begin Source File

SOURCE=.\ReplicationGroupView.cpp
# End Source File
# Begin Source File

SOURCE=.\ReportConfigDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\RGCommandsPage.cpp
# End Source File
# Begin Source File

SOURCE=.\RGDetailsPage.cpp
# End Source File
# Begin Source File

SOURCE=.\RGEventsPage.cpp
# End Source File
# Begin Source File

SOURCE=.\RGGeneralPage.cpp
# End Source File
# Begin Source File

SOURCE=.\RGReplicationPairsAdmPage.cpp
# End Source File
# Begin Source File

SOURCE=.\RGReplicationPairsPage.cpp
# End Source File
# Begin Source File

SOURCE=.\RGSelectDevicesDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\RGSelectLocationDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\RGSelectServerDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\RGSymmetricPage.cpp
# End Source File
# Begin Source File

SOURCE=.\RGThrottlePage.cpp
# End Source File
# Begin Source File

SOURCE=.\RGTunablePage.cpp
# End Source File
# Begin Source File

SOURCE=.\ScriptEditorDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ScriptEditorPage.cpp
# End Source File
# Begin Source File

SOURCE=.\SelectCounterDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\SelectServerNameDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerCommandsPage.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerDetailsPage.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerEventsPage.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerGeneralPAge.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerPerformanceMonitorPage.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerPerformanceReporterPage.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerPropertySheet.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerRegistrationPage.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerView.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerWarningDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\SeverSelectionDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\SoftekDialogBar.cpp
# End Source File
# Begin Source File

SOURCE=.\Splash.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\SystemDetailsPage.cpp
# End Source File
# Begin Source File

SOURCE=.\SystemEventsPage.cpp
# End Source File
# Begin Source File

SOURCE=.\SystemGeneralPage.cpp
# End Source File
# Begin Source File

SOURCE=.\SystemPropertySheet.cpp
# End Source File
# Begin Source File

SOURCE=.\SystemSheet.cpp
# End Source File
# Begin Source File

SOURCE=.\SystemUsersPage.cpp
# End Source File
# Begin Source File

SOURCE=.\SystemView.cpp
# End Source File
# Begin Source File

SOURCE=.\TdmfCommonGui.cpp
# End Source File
# Begin Source File

SOURCE=.\TdmfCommonGui.rc
# End Source File
# Begin Source File

SOURCE=.\TdmfCommonGuiDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\TdmfDllModule.cpp
# End Source File
# Begin Source File

SOURCE=.\TdmfDocTemplate.cpp
# End Source File
# Begin Source File

SOURCE=.\TimeRangeDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ToolsView.cpp
# End Source File
# Begin Source File

SOURCE=.\ViewNotification.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ColumnSelectionDlg.h
# End Source File
# Begin Source File

SOURCE=.\DebugMonitor.h
# End Source File
# Begin Source File

SOURCE=.\DomainDetailsPage.h
# End Source File
# Begin Source File

SOURCE=.\DomainGeneralPage.h
# End Source File
# Begin Source File

SOURCE=.\DomainPropertySheet.h
# End Source File
# Begin Source File

SOURCE=.\EmptyPage.h
# End Source File
# Begin Source File

SOURCE=.\EventProperties.h
# End Source File
# Begin Source File

SOURCE=.\ImportScriptDlg.h
# End Source File
# Begin Source File

SOURCE=.\LocationEdit.h
# End Source File
# Begin Source File

SOURCE=.\LoginDlg.h
# End Source File
# Begin Source File

SOURCE=.\LogViewerDlg.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\MessageByType.h
# End Source File
# Begin Source File

SOURCE=.\Messenger.h
# End Source File
# Begin Source File

SOURCE=.\NewScriptServerFileName.h
# End Source File
# Begin Source File

SOURCE=.\OptionAdminPage.h
# End Source File
# Begin Source File

SOURCE=.\OptionGeneralPage.h
# End Source File
# Begin Source File

SOURCE=.\OptionPropertySheet.h
# End Source File
# Begin Source File

SOURCE=.\OptionsDialog.h
# End Source File
# Begin Source File

SOURCE=.\OptionsRegKeyPage.h
# End Source File
# Begin Source File

SOURCE=.\ProgressInfoDlg.h
# End Source File
# Begin Source File

SOURCE=.\PropertyPageBase.h
# End Source File
# Begin Source File

SOURCE=.\ReplicationGroupPropertySheet.h
# End Source File
# Begin Source File

SOURCE=.\ReplicationGroupView.h
# End Source File
# Begin Source File

SOURCE=.\ReportConfigDlg.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\RGCommandsPage.h
# End Source File
# Begin Source File

SOURCE=.\RGDetailsPage.h
# End Source File
# Begin Source File

SOURCE=.\RGEventsPage.h
# End Source File
# Begin Source File

SOURCE=.\RGGeneralPage.h
# End Source File
# Begin Source File

SOURCE=.\RGReplicationPairsAdmPage.h
# End Source File
# Begin Source File

SOURCE=.\RGReplicationPairsPage.h
# End Source File
# Begin Source File

SOURCE=.\RGSelectDevicesDialog.h
# End Source File
# Begin Source File

SOURCE=.\RGSelectLocationDialog.h
# End Source File
# Begin Source File

SOURCE=.\RGSelectServerDialog.h
# End Source File
# Begin Source File

SOURCE=.\RGSymmetricPage.h
# End Source File
# Begin Source File

SOURCE=.\RGThrottlePage.h
# End Source File
# Begin Source File

SOURCE=.\RGTunablePage.h
# End Source File
# Begin Source File

SOURCE=.\ScriptEditorDlg.h
# End Source File
# Begin Source File

SOURCE=.\ScriptEditorPage.h
# End Source File
# Begin Source File

SOURCE=.\SelectCounterDlg.h
# End Source File
# Begin Source File

SOURCE=.\SelectServerNameDlg.h
# End Source File
# Begin Source File

SOURCE=.\ServerCommandsPage.h
# End Source File
# Begin Source File

SOURCE=.\ServerDetailsPage.h
# End Source File
# Begin Source File

SOURCE=.\ServerEventsPage.h
# End Source File
# Begin Source File

SOURCE=.\ServerGeneralPAge.h
# End Source File
# Begin Source File

SOURCE=.\ServerPerformanceMonitorPage.h
# End Source File
# Begin Source File

SOURCE=.\ServerPerformanceReporterPage.h
# End Source File
# Begin Source File

SOURCE=.\ServerPropertySheet.h
# End Source File
# Begin Source File

SOURCE=.\ServerRegistrationPage.h
# End Source File
# Begin Source File

SOURCE=.\ServerView.h
# End Source File
# Begin Source File

SOURCE=.\ServerWarningDlg.h
# End Source File
# Begin Source File

SOURCE=.\SeverSelectionDialog.h
# End Source File
# Begin Source File

SOURCE=.\SoftekDialogBar.h
# End Source File
# Begin Source File

SOURCE=.\Splash.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\SystemDetailsPage.h
# End Source File
# Begin Source File

SOURCE=.\SystemEventsPage.h
# End Source File
# Begin Source File

SOURCE=.\SystemGeneralPage.h
# End Source File
# Begin Source File

SOURCE=.\SystemPropertySheet.h
# End Source File
# Begin Source File

SOURCE=.\SystemSheet.h
# End Source File
# Begin Source File

SOURCE=.\SystemUsersPage.h
# End Source File
# Begin Source File

SOURCE=.\SystemView.h
# End Source File
# Begin Source File

SOURCE=.\TdmfCommonGui.h
# End Source File
# Begin Source File

SOURCE=.\TdmfCommonGuiDoc.h
# End Source File
# Begin Source File

SOURCE=.\TdmfDllModule.h
# End Source File
# Begin Source File

SOURCE=.\TdmfDocTemplate.h
# End Source File
# Begin Source File

SOURCE=.\TimeRangeDlg.h
# End Source File
# Begin Source File

SOURCE=.\ToolsView.h
# End Source File
# Begin Source File

SOURCE=.\ViewNotification.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap2.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bmp00001.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bmp00002.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bmp00003.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bmp00004.bmp
# End Source File
# Begin Source File

SOURCE=.\res\BrandingStripRight.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btn_10.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btn_30.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btn_40.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btn_50.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btn_60.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btn_clea.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btn_loga.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btn_paus.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btn_time.bmp
# End Source File
# Begin Source File

SOURCE=.\res\clock.avi
# End Source File
# Begin Source File

SOURCE=.\res\Copy.ico
# End Source File
# Begin Source File

SOURCE=.\res\downa.ico
# End Source File
# Begin Source File

SOURCE=.\res\empty_tree.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ico00001.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon_emptree.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon_upa.ico
# End Source File
# Begin Source File

SOURCE=.\res\sftk.ico
# End Source File
# Begin Source File

SOURCE=.\res\softek_logo.bmp
# End Source File
# Begin Source File

SOURCE=.\res\tdmf_ose.bmp
# End Source File
# Begin Source File

SOURCE=.\res\TdmfCommonGui.ico
# End Source File
# Begin Source File

SOURCE=.\res\TdmfCommonGui.rc2
# End Source File
# Begin Source File

SOURCE=.\res\TdmfCommonGuiDoc.ico
# End Source File
# Begin Source File

SOURCE=.\res\TDMFlogo.bmp
# End Source File
# Begin Source File

SOURCE=.\res\TDMFTab.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\toolbar1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\upa.ico
# End Source File
# Begin Source File

SOURCE=".\res\update app.avi"
# End Source File
# End Group
# Begin Group "Context Menus"

# PROP Default_Filter ""
# Begin Group "Context Menu Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ContextMenuBase.cpp
# End Source File
# Begin Source File

SOURCE=.\DomainContextMenu.cpp
# End Source File
# Begin Source File

SOURCE=.\ReplicationGroupContextMenu.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerContextMenu.cpp
# End Source File
# Begin Source File

SOURCE=.\SystemContextMenu.cpp
# End Source File
# End Group
# Begin Group "Context Menu Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ContextMenuBase.h
# End Source File
# Begin Source File

SOURCE=.\DomainContextMenu.h
# End Source File
# Begin Source File

SOURCE=.\ReplicationGroupContextMenu.h
# End Source File
# Begin Source File

SOURCE=.\ServerContextMenu.h
# End Source File
# Begin Source File

SOURCE=.\SystemContextMenu.h
# End Source File
# End Group
# End Group
# Begin Source File

SOURCE=.\res\14527.emf
# End Source File
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
