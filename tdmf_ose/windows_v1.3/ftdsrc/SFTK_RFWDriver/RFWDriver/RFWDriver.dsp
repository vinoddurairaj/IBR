# Microsoft Developer Studio Project File - Name="RFWDriver" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=RFWDriver - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "RFWDriver.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "RFWDriver.mak" CFG="RFWDriver - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "RFWDriver - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "RFWDriver - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE "RFWDriver - Win32 Release NT4" (based on "Win32 (x86) External Target")
!MESSAGE "RFWDriver - Win32 Debug NT4" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "RFWDriver - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f RFWDriver.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "RFWDriver.exe"
# PROP BASE Bsc_Name "RFWDriver.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "ddkbuild ..\..\..\NETDDK\3790 -W2K free . /a"
# PROP Rebuild_Opt "-cef"
# PROP Target_File "sftk_Block.sys"
# PROP Bsc_Name "RFWDriver.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "RFWDriver - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f RFWDriver.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "RFWDriver.exe"
# PROP BASE Bsc_Name "RFWDriver.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "ddkbuild ..\..\..\NETDDK\3790 -W2K checked . /a"
# PROP Rebuild_Opt "-cef"
# PROP Target_File "sftk_Block.sys"
# PROP Bsc_Name "RFWDriver.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "RFWDriver - Win32 Release NT4"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f RFWDriver.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "RFWDriver.exe"
# PROP BASE Bsc_Name "RFWDriver.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "ddkbuild ..\..\..\NETDDK\3790 -NT4 free . /a"
# PROP Rebuild_Opt "-cef"
# PROP Target_File "sftk_Block.sys"
# PROP Bsc_Name "RFWDriver.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "RFWDriver - Win32 Debug NT4"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f RFWDriver.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "RFWDriver.exe"
# PROP BASE Bsc_Name "RFWDriver.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "ddkbuild ..\..\..\NETDDK\3790 -NT4 checked . /a"
# PROP Rebuild_Opt "-cef"
# PROP Target_File "sftk_Block.sys"
# PROP Bsc_Name "RFWDriver.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "RFWDriver - Win32 Release"
# Name "RFWDriver - Win32 Debug"
# Name "RFWDriver - Win32 Release NT4"
# Name "RFWDriver - Win32 Debug NT4"

!IF  "$(CFG)" == "RFWDriver - Win32 Release"

!ELSEIF  "$(CFG)" == "RFWDriver - Win32 Debug"

!ELSEIF  "$(CFG)" == "RFWDriver - Win32 Release NT4"

!ELSEIF  "$(CFG)" == "RFWDriver - Win32 Debug NT4"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\sftk_Bitmap.c
# End Source File
# Begin Source File

SOURCE=.\sftk_Dev.c
# End Source File
# Begin Source File

SOURCE=.\sftk_Device.c
# End Source File
# Begin Source File

SOURCE=.\sftk_driver.c
# End Source File
# Begin Source File

SOURCE=.\sftk_Ioctl.c
# End Source File
# Begin Source File

SOURCE=.\sftk_lg.c
# End Source File
# Begin Source File

SOURCE=.\sftk_MM.c
# End Source File
# Begin Source File

SOURCE=.\sftk_OS.c
# End Source File
# Begin Source File

SOURCE=.\sftk_pstore.c
# End Source File
# Begin Source File

SOURCE=.\sftk_Queue.c
# End Source File
# Begin Source File

SOURCE=.\sftk_Refresh.c
# End Source File
# Begin Source File

SOURCE=.\sftk_Registry.c
# End Source File
# Begin Source File

SOURCE=.\sftk_SM.c
# End Source File
# Begin Source File

SOURCE=.\sftk_Thread.c
# End Source File
# Begin Source File

SOURCE=.\sftk_WMI.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ftdio.h
# End Source File
# Begin Source File

SOURCE=.\sftk_com.h
# End Source File
# Begin Source File

SOURCE=.\sftk_comProto.h
# End Source File
# Begin Source File

SOURCE=.\sftk_def.h
# End Source File
# Begin Source File

SOURCE=.\sftk_Macro.h
# End Source File
# Begin Source File

SOURCE=.\sftk_main.h
# End Source File
# Begin Source File

SOURCE=.\sftk_MM.h
# End Source File
# Begin Source File

SOURCE=.\sftk_NT.h
# End Source File
# Begin Source File

SOURCE=.\sftk_os.h
# End Source File
# Begin Source File

SOURCE=.\sftk_Proto.h
# End Source File
# Begin Source File

SOURCE=.\sftk_ps.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\sftk_block.mc
# End Source File
# Begin Source File

SOURCE=.\sftk_block.rc
# End Source File
# End Group
# Begin Group "Build Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\buildchk.log
# End Source File
# Begin Source File

SOURCE=.\CSym.bat
# End Source File
# Begin Source File

SOURCE=.\ddkbuild.bat
# End Source File
# Begin Source File

SOURCE=.\makefile
# End Source File
# Begin Source File

SOURCE=.\sbrList.txt
# End Source File
# Begin Source File

SOURCE=.\sftkBlk.inf
# End Source File
# Begin Source File

SOURCE=.\sftkBlk.ini
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# End Group
# Begin Group "TDI"

# PROP Default_Filter ""
# Begin Group "TDI Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\sftk_comutil.c
# End Source File
# Begin Source File

SOURCE=.\sftk_tdiconnect.c
# End Source File
# Begin Source File

SOURCE=.\sftk_tdilisten.c
# End Source File
# Begin Source File

SOURCE=.\sftk_tdireceive.c
# End Source File
# Begin Source File

SOURCE=.\sftk_tdisend.c
# End Source File
# Begin Source File

SOURCE=.\Sftk_TdiTcpEx.c
# End Source File
# Begin Source File

SOURCE=.\Sftk_TdiUtil.c
# End Source File
# End Group
# Begin Group "TDI Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\TDI\ethertype.h
# End Source File
# Begin Source File

SOURCE=.\TDI\IGMP.H
# End Source File
# Begin Source File

SOURCE=.\TDI\IN.H
# End Source File
# Begin Source File

SOURCE=.\TDI\IN_SYSTM.H
# End Source File
# Begin Source File

SOURCE=.\TDI\INETINC.H
# End Source File
# Begin Source File

SOURCE=.\TDI\IP.H
# End Source File
# Begin Source File

SOURCE=.\TDI\IP_ICMP.H
# End Source File
# Begin Source File

SOURCE=.\TDI\IP_MROUT.H
# End Source File
# Begin Source File

SOURCE=.\TDI\Sftk_TdiTcpEx.h
# End Source File
# Begin Source File

SOURCE=.\TDI\Sftk_TdiUtil.h
# End Source File
# Begin Source File

SOURCE=.\TDI\SMPLETCP.H
# End Source File
# Begin Source File

SOURCE=.\TDI\TCP.H
# End Source File
# Begin Source File

SOURCE=.\TDI\TDIHApi.h
# End Source File
# Begin Source File

SOURCE=.\TDI\TDIIOCTL.H
# End Source File
# Begin Source File

SOURCE=.\TDI\TDIQHelp.h
# End Source File
# Begin Source File

SOURCE=.\TDI\TTCPAPI.H
# End Source File
# Begin Source File

SOURCE=.\TDI\UDP.H
# End Source File
# End Group
# End Group
# Begin Group "Protocol"

# PROP Default_Filter ""
# Begin Group "Protocol Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\sftk_comp.c
# End Source File
# Begin Source File

SOURCE=.\sftk_lzhl.c
# End Source File
# Begin Source File

SOURCE=.\sftk_md5c.c
# End Source File
# Begin Source File

SOURCE=.\sftk_md5const.c
# End Source File
# Begin Source File

SOURCE=.\sftk_pred.c
# End Source File
# Begin Source File

SOURCE=.\sftk_protocol.c
# End Source File
# End Group
# Begin Group "Protocol Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Hdec_g.tbl
# End Source File
# Begin Source File

SOURCE=.\Hdec_s.tbl
# End Source File
# Begin Source File

SOURCE=.\Hdisp.tbl
# End Source File
# Begin Source File

SOURCE=.\Henc.tbl
# End Source File
# Begin Source File

SOURCE=.\sftk__lzhl.h
# End Source File
# Begin Source File

SOURCE=.\sftk_comp.h
# End Source File
# Begin Source File

SOURCE=.\sftk_huff.h
# End Source File
# Begin Source File

SOURCE=.\sftk_lz.h
# End Source File
# Begin Source File

SOURCE=.\sftk_lzhl.h
# End Source File
# Begin Source File

SOURCE=.\sftk_md5.h
# End Source File
# Begin Source File

SOURCE=.\sftk_md5const.h
# End Source File
# Begin Source File

SOURCE=.\sftk_pred.h
# End Source File
# Begin Source File

SOURCE=.\sftk_protocol.h
# End Source File
# End Group
# End Group
# End Target
# End Project
