# Microsoft Developer Studio Project File - Name="TDMFGUI" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=TDMFGUI - Win32 Remote Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "TDMFGUI.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "TDMFGUI.mak" CFG="TDMFGUI - Win32 Remote Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TDMFGUI - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "TDMFGUI - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "TDMFGUI - Win32 Remote Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "TDMFGUI - Win32 Release"

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
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0xc0c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0xc0c /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "TDMFGUI - Win32 Debug"

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
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0xc0c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0xc0c /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "TDMFGUI - Win32 Remote Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "TDMFGUI___Win32_Remote_Debug"
# PROP BASE Intermediate_Dir "TDMFGUI___Win32_Remote_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0xc0c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0xc0c /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "TDMFGUI - Win32 Release"
# Name "TDMFGUI - Win32 Debug"
# Name "TDMFGUI - Win32 Remote Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "GUI Framework"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Doc.cpp
# End Source File
# Begin Source File

SOURCE=.\GenericPropPage.cpp
# End Source File
# Begin Source File

SOURCE=.\LeftView.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\PageSplitter.cpp
# End Source File
# Begin Source File

SOURCE=.\PageView.cpp
# End Source File
# Begin Source File

SOURCE=.\PropPage.cpp
# End Source File
# Begin Source File

SOURCE=.\PropSheet.cpp
# End Source File
# Begin Source File

SOURCE=.\PropSheetFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\SplitterFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\TDMFGUI.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\BaseTreeView.cpp
# End Source File
# Begin Source File

SOURCE=.\BlankView.cpp
# End Source File
# Begin Source File

SOURCE=.\ChooseItemDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\DetailsServerPage.cpp
# End Source File
# Begin Source File

SOURCE=.\EmptyView.cpp
# End Source File
# Begin Source File

SOURCE=.\Graph.cpp
# End Source File
# Begin Source File

SOURCE=.\GraphDataColor.cpp
# End Source File
# Begin Source File

SOURCE=.\GraphDataSet.cpp
# End Source File
# Begin Source File

SOURCE=.\GraphLegend.cpp
# End Source File
# Begin Source File

SOURCE=.\GraphLegendSet.cpp
# End Source File
# Begin Source File

SOURCE=.\GraphPieLabel.cpp
# End Source File
# Begin Source File

SOURCE=.\GraphSeries.cpp
# End Source File
# Begin Source File

SOURCE=.\InfoBar.cpp
# End Source File
# Begin Source File

SOURCE=.\LeftTreeView.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFilterView.cpp
# End Source File
# Begin Source File

SOURCE=.\MainView.cpp
# End Source File
# Begin Source File

SOURCE=.\MonitorFilterView.cpp
# End Source File
# Begin Source File

SOURCE=.\PerformanceToolbarView.cpp
# End Source File
# Begin Source File

SOURCE=.\propsht.cpp
# End Source File
# Begin Source File

SOURCE=.\ReplicationPairView.cpp
# End Source File
# Begin Source File

SOURCE=.\RightBottomToolbarView.cpp
# End Source File
# Begin Source File

SOURCE=.\RightBottomView.cpp
# End Source File
# Begin Source File

SOURCE=.\RightBottomViewConfiguration.cpp
# End Source File
# Begin Source File

SOURCE=.\RightTopToolbarView.cpp
# End Source File
# Begin Source File

SOURCE=.\RightTopView.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerCommandView.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerPerformanceView.cpp
# End Source File
# Begin Source File

SOURCE=.\SHyperlink.cpp
# End Source File
# Begin Source File

SOURCE=.\SHyperlinkComboBox.cpp
# End Source File
# Begin Source File

SOURCE=.\svbase.cpp
# End Source File
# Begin Source File

SOURCE=.\svcolumn.cpp
# End Source File
# Begin Source File

SOURCE=.\SVDColumnSelector.cpp
# End Source File
# Begin Source File

SOURCE=.\SVFormatNumber.cpp
# End Source File
# Begin Source File

SOURCE=.\SVHeaderCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\svregkey.cpp
# End Source File
# Begin Source File

SOURCE=.\svsheet.cpp
# End Source File
# Begin Source File

SOURCE=.\TDMFEventView.cpp
# End Source File
# Begin Source File

SOURCE=.\TDMFGUI.rc
# End Source File
# Begin Source File

SOURCE=.\TDMFLogicalGroupsView.cpp
# End Source File
# Begin Source File

SOURCE=.\TDMFMonitorView.cpp
# End Source File
# Begin Source File

SOURCE=.\TDMFObjects.cpp
# End Source File
# Begin Source File

SOURCE=.\TDMFServerEventView.cpp
# End Source File
# Begin Source File

SOURCE=.\TDMFServersView.cpp
# End Source File
# Begin Source File

SOURCE=.\TestView.cpp
# End Source File
# Begin Source File

SOURCE=.\ViewHostBottom.cpp
# End Source File
# Begin Source File

SOURCE=.\ViewHostTop.cpp
# End Source File
# Begin Source File

SOURCE=.\ViewOverviewBottom.cpp
# End Source File
# Begin Source File

SOURCE=.\ViewOverviewTop.cpp
# End Source File
# Begin Source File

SOURCE=.\ViewPairBottom.cpp
# End Source File
# Begin Source File

SOURCE=.\ViewPairTop.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "GUI Framework Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Doc.h
# End Source File
# Begin Source File

SOURCE=.\GenericPropPage.h
# End Source File
# Begin Source File

SOURCE=.\LeftView.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\PageSplitter.h
# End Source File
# Begin Source File

SOURCE=.\PageView.h
# End Source File
# Begin Source File

SOURCE=.\PropPage.h
# End Source File
# Begin Source File

SOURCE=.\PropSheet.h
# End Source File
# Begin Source File

SOURCE=.\PropSheetFrame.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\SplitterFrame.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\TDMFGUI.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\BaseTreeView.h
# End Source File
# Begin Source File

SOURCE=.\BlankView.h
# End Source File
# Begin Source File

SOURCE=.\ChooseItemDlg.h
# End Source File
# Begin Source File

SOURCE=.\DetailsServerPage.h
# End Source File
# Begin Source File

SOURCE=.\EmptyView.h
# End Source File
# Begin Source File

SOURCE=.\Graph.h
# End Source File
# Begin Source File

SOURCE=.\GraphDataColor.h
# End Source File
# Begin Source File

SOURCE=.\GraphDataSet.h
# End Source File
# Begin Source File

SOURCE=.\GraphLegend.h
# End Source File
# Begin Source File

SOURCE=.\GraphLegendSet.h
# End Source File
# Begin Source File

SOURCE=.\GraphPieLabel.h
# End Source File
# Begin Source File

SOURCE=.\GraphSeries.h
# End Source File
# Begin Source File

SOURCE=.\InfoBar.h
# End Source File
# Begin Source File

SOURCE=.\LeftTreeView.h
# End Source File
# Begin Source File

SOURCE=.\MainFilterView.h
# End Source File
# Begin Source File

SOURCE=.\MainView.h
# End Source File
# Begin Source File

SOURCE=.\MonitorFilterView.h
# End Source File
# Begin Source File

SOURCE=.\MonitorRes.h
# End Source File
# Begin Source File

SOURCE=.\PerformanceToolbarView.h
# End Source File
# Begin Source File

SOURCE=.\propsht.h
# End Source File
# Begin Source File

SOURCE=.\ReplicationPairView.h
# End Source File
# Begin Source File

SOURCE=.\RightBottomToolbarView.h
# End Source File
# Begin Source File

SOURCE=.\RightBottomView.h
# End Source File
# Begin Source File

SOURCE=.\RightBottomViewConfiguration.h
# End Source File
# Begin Source File

SOURCE=.\RightTopToolbarView.h
# End Source File
# Begin Source File

SOURCE=.\RightTopView.h
# End Source File
# Begin Source File

SOURCE=.\ServerCommandView.h
# End Source File
# Begin Source File

SOURCE=.\ServerPerformanceView.h
# End Source File
# Begin Source File

SOURCE=.\SHyperlink.h
# End Source File
# Begin Source File

SOURCE=.\SHyperlinkComboBox.h
# End Source File
# Begin Source File

SOURCE=.\svbase.h
# End Source File
# Begin Source File

SOURCE=.\svcolumn.h
# End Source File
# Begin Source File

SOURCE=.\SVDColumnSelector.h
# End Source File
# Begin Source File

SOURCE=.\SVenUnitType.h
# End Source File
# Begin Source File

SOURCE=.\SVFormatNumber.h
# End Source File
# Begin Source File

SOURCE=.\svglobal.h
# End Source File
# Begin Source File

SOURCE=.\SVHeaderCtrl.h
# End Source File
# Begin Source File

SOURCE=.\svregkey.h
# End Source File
# Begin Source File

SOURCE=.\svsheet.h
# End Source File
# Begin Source File

SOURCE=.\TDMFEventView.h
# End Source File
# Begin Source File

SOURCE=.\TDMFLogicalGroupsView.h
# End Source File
# Begin Source File

SOURCE=.\TDMFMonitorView.h
# End Source File
# Begin Source File

SOURCE=.\TDMFObjects.h
# End Source File
# Begin Source File

SOURCE=.\TDMFServerEventView.h
# End Source File
# Begin Source File

SOURCE=.\TDMFServersView.h
# End Source File
# Begin Source File

SOURCE=.\TestView.h
# End Source File
# Begin Source File

SOURCE=.\ViewHostBottom.h
# End Source File
# Begin Source File

SOURCE=.\ViewHostTop.h
# End Source File
# Begin Source File

SOURCE=.\ViewOverviewBottom.h
# End Source File
# Begin Source File

SOURCE=.\ViewOverviewTop.h
# End Source File
# Begin Source File

SOURCE=.\ViewPairBottom.h
# End Source File
# Begin Source File

SOURCE=.\ViewPairTop.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\BackgroundLogo.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bmp00001.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bmp00002.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Doc.ico
# End Source File
# Begin Source File

SOURCE=.\res\DOMAIN.ICO
# End Source File
# Begin Source File

SOURCE=.\res\DRIVE.ICO
# End Source File
# Begin Source File

SOURCE=.\res\list_too.bmp
# End Source File
# Begin Source File

SOURCE=.\res\MACHINE.ICO
# End Source File
# Begin Source File

SOURCE=.\res\SV_3Bars.ico
# End Source File
# Begin Source File

SOURCE=.\res\SV_3Values.ico
# End Source File
# Begin Source File

SOURCE=.\res\SV_checkoff.bmp
# End Source File
# Begin Source File

SOURCE=.\res\SV_checkon.bmp
# End Source File
# Begin Source File

SOURCE=.\res\SV_DownNorule.ico
# End Source File
# Begin Source File

SOURCE=.\res\SV_DownRule.ico
# End Source File
# Begin Source File

SOURCE=.\res\SV_False.ico
# End Source File
# Begin Source File

SOURCE=.\res\SV_RuleOnly.ico
# End Source File
# Begin Source File

SOURCE=.\res\sv_sc1bar.ico
# End Source File
# Begin Source File

SOURCE=.\res\SV_True.ico
# End Source File
# Begin Source File

SOURCE=.\res\SV_UpNorule.ico
# End Source File
# Begin Source File

SOURCE=.\res\SV_UpRule.ico
# End Source File
# Begin Source File

SOURCE=.\res\TDMFGUI.ico
# End Source File
# Begin Source File

SOURCE=.\res\TDMFGUI.rc2
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\toolbar1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\tree_image.bmp
# End Source File
# End Group
# End Target
# End Project
