# Microsoft Developer Studio Generated NMAKE File, Based on DTCConfigTool.dsp
!IF "$(CFG)" == ""
CFG=DTCConfigTool - Win32 Debug
!MESSAGE No configuration specified. Defaulting to DTCConfigTool - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "DTCConfigTool - Win32 Release" && "$(CFG)" != "DTCConfigTool - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
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
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "DTCConfigTool - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\configtool.exe"

!ELSE 

ALL : "liblic - Win32 Release" "LibLic2 - Win32 Release" "libutil - Win32 Release" "libsock - Win32 Release" "libproc - Win32 Release" "libmd5 - Win32 Release" "liblst - Win32 Release" "libftd - Win32 Release" "liberr - Win32 Release" "libcomp - Win32 Release" "$(OUTDIR)\configtool.exe"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"libcomp - Win32 ReleaseCLEAN" "liberr - Win32 ReleaseCLEAN" "libftd - Win32 ReleaseCLEAN" "liblst - Win32 ReleaseCLEAN" "libmd5 - Win32 ReleaseCLEAN" "libproc - Win32 ReleaseCLEAN" "libsock - Win32 ReleaseCLEAN" "libutil - Win32 ReleaseCLEAN" "LibLic2 - Win32 ReleaseCLEAN" "liblic - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\AddGroup.obj"
	-@erase "$(INTDIR)\AddModMirrors.obj"
	-@erase "$(INTDIR)\Config.obj"
	-@erase "$(INTDIR)\DeleteGroup.obj"
	-@erase "$(INTDIR)\DTCConfigPropSheet.obj"
	-@erase "$(INTDIR)\DTCConfigTool.obj"
	-@erase "$(INTDIR)\DTCConfigTool.pch"
	-@erase "$(INTDIR)\DTCConfigTool.res"
	-@erase "$(INTDIR)\DTCConfigToolDlg.obj"
	-@erase "$(INTDIR)\DTCDevices.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\System.obj"
	-@erase "$(INTDIR)\Throttles.obj"
	-@erase "$(INTDIR)\TunableParams.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\configtool.exe"
	-@erase "$(OUTDIR)\configtool.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90=df.exe
CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /Z7 /O2 /I "..\..\lib\libutil" /I "..\..\driver" /I "..\..\lib\libftd" /I "..\..\lib\libsock" /I "..\..\lib\liblic" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /Fp"$(INTDIR)\DTCConfigTool.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\DTCConfigTool.res" /d "NDEBUG" /d "MTI" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\DTCConfigTool.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=$(TOP)\$(LIBSOCKREL) $(TOP)\$(LIBUTILREL) $(TOP)\$(LIBFTDREL) $(TOP)\$(LIBLICREL) ws2_32.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\configtool.pdb" /debug /debugtype:both /machine:I386 /out:"$(OUTDIR)\configtool.exe" /opt:ref,icf 
LINK32_OBJS= \
	"$(INTDIR)\AddGroup.obj" \
	"$(INTDIR)\AddModMirrors.obj" \
	"$(INTDIR)\Config.obj" \
	"$(INTDIR)\DeleteGroup.obj" \
	"$(INTDIR)\DTCConfigPropSheet.obj" \
	"$(INTDIR)\DTCConfigTool.obj" \
	"$(INTDIR)\DTCConfigToolDlg.obj" \
	"$(INTDIR)\DTCDevices.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\System.obj" \
	"$(INTDIR)\Throttles.obj" \
	"$(INTDIR)\TunableParams.obj" \
	"$(INTDIR)\DTCConfigTool.res" \
	"..\..\lib\libcomp\Release\libcomp.lib" \
	"..\..\lib\liberr\Release\liberr.lib" \
	"..\..\lib\libftd\Release\libftd.lib" \
	"..\..\lib\liblst\Release\liblst.lib" \
	"..\..\lib\libmd5\Release\libmd5.lib" \
	"..\..\lib\libproc\Release\libproc.lib" \
	"..\..\lib\libsock\Release\libsock.lib" \
	"..\..\lib\libutil\Release\libutil.lib" \
	"..\..\lib\Liblic2\Release\LibLic2.lib" \
	"..\..\lib\liblic\Release\liblic.lib"

"$(OUTDIR)\configtool.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

TargetPath=.\Release\configtool.exe
TargetName=ConfigTool
SOURCE="$(InputPath)"
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

$(DS_POSTBUILD_DEP) : "liblic - Win32 Release" "LibLic2 - Win32 Release" "libutil - Win32 Release" "libsock - Win32 Release" "libproc - Win32 Release" "libmd5 - Win32 Release" "liblst - Win32 Release" "libftd - Win32 Release" "liberr - Win32 Release" "libcomp - Win32 Release" "$(OUTDIR)\configtool.exe"
   md C:\TDMF\Rep_5_01_NT\ftdsrc\"Replication"
	copy .\Release\configtool.exe C:\TDMF\Rep_5_01_NT\ftdsrc\"Replication"\lrconfigtool.exe
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "DTCConfigTool - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\configtool.exe" "$(OUTDIR)\DTCConfigTool.bsc"

!ELSE 

ALL : "liblic - Win32 Debug" "LibLic2 - Win32 Debug" "libutil - Win32 Debug" "libsock - Win32 Debug" "libproc - Win32 Debug" "libmd5 - Win32 Debug" "liblst - Win32 Debug" "libftd - Win32 Debug" "liberr - Win32 Debug" "libcomp - Win32 Debug" "$(OUTDIR)\configtool.exe" "$(OUTDIR)\DTCConfigTool.bsc"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"libcomp - Win32 DebugCLEAN" "liberr - Win32 DebugCLEAN" "libftd - Win32 DebugCLEAN" "liblst - Win32 DebugCLEAN" "libmd5 - Win32 DebugCLEAN" "libproc - Win32 DebugCLEAN" "libsock - Win32 DebugCLEAN" "libutil - Win32 DebugCLEAN" "LibLic2 - Win32 DebugCLEAN" "liblic - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\AddGroup.obj"
	-@erase "$(INTDIR)\AddGroup.sbr"
	-@erase "$(INTDIR)\AddModMirrors.obj"
	-@erase "$(INTDIR)\AddModMirrors.sbr"
	-@erase "$(INTDIR)\Config.obj"
	-@erase "$(INTDIR)\Config.sbr"
	-@erase "$(INTDIR)\DeleteGroup.obj"
	-@erase "$(INTDIR)\DeleteGroup.sbr"
	-@erase "$(INTDIR)\DTCConfigPropSheet.obj"
	-@erase "$(INTDIR)\DTCConfigPropSheet.sbr"
	-@erase "$(INTDIR)\DTCConfigTool.obj"
	-@erase "$(INTDIR)\DTCConfigTool.pch"
	-@erase "$(INTDIR)\DTCConfigTool.res"
	-@erase "$(INTDIR)\DTCConfigTool.sbr"
	-@erase "$(INTDIR)\DTCConfigToolDlg.obj"
	-@erase "$(INTDIR)\DTCConfigToolDlg.sbr"
	-@erase "$(INTDIR)\DTCDevices.obj"
	-@erase "$(INTDIR)\DTCDevices.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\System.obj"
	-@erase "$(INTDIR)\System.sbr"
	-@erase "$(INTDIR)\Throttles.obj"
	-@erase "$(INTDIR)\Throttles.sbr"
	-@erase "$(INTDIR)\TunableParams.obj"
	-@erase "$(INTDIR)\TunableParams.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\configtool.exe"
	-@erase "$(OUTDIR)\configtool.ilk"
	-@erase "$(OUTDIR)\configtool.pdb"
	-@erase "$(OUTDIR)\DTCConfigTool.bsc"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90=df.exe
CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\lib\libutil" /I "..\..\driver" /I "..\..\lib\libftd" /I "..\..\lib\libsock" /I "..\..\lib\liblic" /D "WIN32" /D "_MBCS" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\DTCConfigTool.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\DTCConfigTool.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\DTCConfigTool.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\AddGroup.sbr" \
	"$(INTDIR)\AddModMirrors.sbr" \
	"$(INTDIR)\Config.sbr" \
	"$(INTDIR)\DeleteGroup.sbr" \
	"$(INTDIR)\DTCConfigPropSheet.sbr" \
	"$(INTDIR)\DTCConfigTool.sbr" \
	"$(INTDIR)\DTCConfigToolDlg.sbr" \
	"$(INTDIR)\DTCDevices.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\System.sbr" \
	"$(INTDIR)\Throttles.sbr" \
	"$(INTDIR)\TunableParams.sbr"

"$(OUTDIR)\DTCConfigTool.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=$(TOP)\$(LIBLICDBG) $(TOP)\$(LIBSOCKDBG) $(TOP)\$(LIBUTILDBG) $(TOP)\$(LIBFTDDBG) ws2_32.lib /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\configtool.pdb" /debug /machine:I386 /out:"$(OUTDIR)\configtool.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\AddGroup.obj" \
	"$(INTDIR)\AddModMirrors.obj" \
	"$(INTDIR)\Config.obj" \
	"$(INTDIR)\DeleteGroup.obj" \
	"$(INTDIR)\DTCConfigPropSheet.obj" \
	"$(INTDIR)\DTCConfigTool.obj" \
	"$(INTDIR)\DTCConfigToolDlg.obj" \
	"$(INTDIR)\DTCDevices.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\System.obj" \
	"$(INTDIR)\Throttles.obj" \
	"$(INTDIR)\TunableParams.obj" \
	"$(INTDIR)\DTCConfigTool.res" \
	"..\..\lib\libcomp\Debug\libcomp.lib" \
	"..\..\lib\liberr\Debug\liberr.lib" \
	"..\..\lib\libftd\Debug\libftd.lib" \
	"..\..\lib\liblst\Debug\liblst.lib" \
	"..\..\lib\libmd5\Debug\libmd5.lib" \
	"..\..\lib\libproc\Debug\libproc.lib" \
	"..\..\lib\libsock\Debug\libsock.lib" \
	"..\..\lib\libutil\Debug\libutil.lib" \
	"..\..\lib\Liblic2\Debug\LibLic2.lib" \
	"..\..\lib\liblic\Debug\liblic.lib"

"$(OUTDIR)\configtool.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("DTCConfigTool.dep")
!INCLUDE "DTCConfigTool.dep"
!ELSE 
!MESSAGE Warning: cannot find "DTCConfigTool.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "DTCConfigTool - Win32 Release" || "$(CFG)" == "DTCConfigTool - Win32 Debug"
SOURCE=.\AddGroup.cpp

!IF  "$(CFG)" == "DTCConfigTool - Win32 Release"


"$(INTDIR)\AddGroup.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\DTCConfigTool.pch"


!ELSEIF  "$(CFG)" == "DTCConfigTool - Win32 Debug"


"$(INTDIR)\AddGroup.obj"	"$(INTDIR)\AddGroup.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\DTCConfigTool.pch"


!ENDIF 

SOURCE=.\AddModMirrors.cpp

!IF  "$(CFG)" == "DTCConfigTool - Win32 Release"


"$(INTDIR)\AddModMirrors.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\DTCConfigTool.pch"


!ELSEIF  "$(CFG)" == "DTCConfigTool - Win32 Debug"


"$(INTDIR)\AddModMirrors.obj"	"$(INTDIR)\AddModMirrors.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\DTCConfigTool.pch"


!ENDIF 

SOURCE=.\Config.cpp

!IF  "$(CFG)" == "DTCConfigTool - Win32 Release"


"$(INTDIR)\Config.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\DTCConfigTool.pch"


!ELSEIF  "$(CFG)" == "DTCConfigTool - Win32 Debug"


"$(INTDIR)\Config.obj"	"$(INTDIR)\Config.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\DTCConfigTool.pch"


!ENDIF 

SOURCE=.\DeleteGroup.cpp

!IF  "$(CFG)" == "DTCConfigTool - Win32 Release"


"$(INTDIR)\DeleteGroup.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\DTCConfigTool.pch"


!ELSEIF  "$(CFG)" == "DTCConfigTool - Win32 Debug"


"$(INTDIR)\DeleteGroup.obj"	"$(INTDIR)\DeleteGroup.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\DTCConfigTool.pch"


!ENDIF 

SOURCE=.\DTCConfigPropSheet.cpp

!IF  "$(CFG)" == "DTCConfigTool - Win32 Release"


"$(INTDIR)\DTCConfigPropSheet.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\DTCConfigTool.pch"


!ELSEIF  "$(CFG)" == "DTCConfigTool - Win32 Debug"


"$(INTDIR)\DTCConfigPropSheet.obj"	"$(INTDIR)\DTCConfigPropSheet.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\DTCConfigTool.pch"


!ENDIF 

SOURCE=.\DTCConfigTool.cpp

!IF  "$(CFG)" == "DTCConfigTool - Win32 Release"


"$(INTDIR)\DTCConfigTool.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\DTCConfigTool.pch"


!ELSEIF  "$(CFG)" == "DTCConfigTool - Win32 Debug"


"$(INTDIR)\DTCConfigTool.obj"	"$(INTDIR)\DTCConfigTool.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\DTCConfigTool.pch"


!ENDIF 

SOURCE=.\DTCConfigTool.rc

"$(INTDIR)\DTCConfigTool.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\DTCConfigToolDlg.cpp

!IF  "$(CFG)" == "DTCConfigTool - Win32 Release"


"$(INTDIR)\DTCConfigToolDlg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\DTCConfigTool.pch"


!ELSEIF  "$(CFG)" == "DTCConfigTool - Win32 Debug"


"$(INTDIR)\DTCConfigToolDlg.obj"	"$(INTDIR)\DTCConfigToolDlg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\DTCConfigTool.pch"


!ENDIF 

SOURCE=.\DTCDevices.cpp

!IF  "$(CFG)" == "DTCConfigTool - Win32 Release"


"$(INTDIR)\DTCDevices.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\DTCConfigTool.pch"


!ELSEIF  "$(CFG)" == "DTCConfigTool - Win32 Debug"


"$(INTDIR)\DTCDevices.obj"	"$(INTDIR)\DTCDevices.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\DTCConfigTool.pch"


!ENDIF 

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "DTCConfigTool - Win32 Release"

CPP_SWITCHES=/nologo /MT /W3 /GX /Z7 /O2 /I "..\..\lib\libutil" /I "..\..\driver" /I "..\..\lib\libftd" /I "..\..\lib\libsock" /I "..\..\lib\liblic" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /Fp"$(INTDIR)\DTCConfigTool.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\DTCConfigTool.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "DTCConfigTool - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\lib\libutil" /I "..\..\driver" /I "..\..\lib\libftd" /I "..\..\lib\libsock" /I "..\..\lib\liblic" /D "WIN32" /D "_MBCS" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\DTCConfigTool.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\StdAfx.sbr"	"$(INTDIR)\DTCConfigTool.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\System.cpp

!IF  "$(CFG)" == "DTCConfigTool - Win32 Release"


"$(INTDIR)\System.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\DTCConfigTool.pch"


!ELSEIF  "$(CFG)" == "DTCConfigTool - Win32 Debug"


"$(INTDIR)\System.obj"	"$(INTDIR)\System.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\DTCConfigTool.pch"


!ENDIF 

SOURCE=.\Throttles.cpp

!IF  "$(CFG)" == "DTCConfigTool - Win32 Release"


"$(INTDIR)\Throttles.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\DTCConfigTool.pch"


!ELSEIF  "$(CFG)" == "DTCConfigTool - Win32 Debug"


"$(INTDIR)\Throttles.obj"	"$(INTDIR)\Throttles.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\DTCConfigTool.pch"


!ENDIF 

SOURCE=.\TunableParams.cpp

!IF  "$(CFG)" == "DTCConfigTool - Win32 Release"


"$(INTDIR)\TunableParams.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\DTCConfigTool.pch"


!ELSEIF  "$(CFG)" == "DTCConfigTool - Win32 Debug"


"$(INTDIR)\TunableParams.obj"	"$(INTDIR)\TunableParams.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\DTCConfigTool.pch"


!ENDIF 

!IF  "$(CFG)" == "DTCConfigTool - Win32 Release"

"libcomp - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libcomp"
   $(MAKE) /$(MAKEFLAGS) /F .\libcomp.mak CFG="libcomp - Win32 Release" 
   cd "..\..\gui\configtool"

"libcomp - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libcomp"
   $(MAKE) /$(MAKEFLAGS) /F .\libcomp.mak CFG="libcomp - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\gui\configtool"

!ELSEIF  "$(CFG)" == "DTCConfigTool - Win32 Debug"

"libcomp - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libcomp"
   $(MAKE) /$(MAKEFLAGS) /F .\libcomp.mak CFG="libcomp - Win32 Debug" 
   cd "..\..\gui\configtool"

"libcomp - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libcomp"
   $(MAKE) /$(MAKEFLAGS) /F .\libcomp.mak CFG="libcomp - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\gui\configtool"

!ENDIF 

!IF  "$(CFG)" == "DTCConfigTool - Win32 Release"

"liberr - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liberr"
   $(MAKE) /$(MAKEFLAGS) /F .\liberr.mak CFG="liberr - Win32 Release" 
   cd "..\..\gui\configtool"

"liberr - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liberr"
   $(MAKE) /$(MAKEFLAGS) /F .\liberr.mak CFG="liberr - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\gui\configtool"

!ELSEIF  "$(CFG)" == "DTCConfigTool - Win32 Debug"

"liberr - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liberr"
   $(MAKE) /$(MAKEFLAGS) /F .\liberr.mak CFG="liberr - Win32 Debug" 
   cd "..\..\gui\configtool"

"liberr - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liberr"
   $(MAKE) /$(MAKEFLAGS) /F .\liberr.mak CFG="liberr - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\gui\configtool"

!ENDIF 

!IF  "$(CFG)" == "DTCConfigTool - Win32 Release"

"libftd - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libftd"
   $(MAKE) /$(MAKEFLAGS) /F .\libftd.mak CFG="libftd - Win32 Release" 
   cd "..\..\gui\configtool"

"libftd - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libftd"
   $(MAKE) /$(MAKEFLAGS) /F .\libftd.mak CFG="libftd - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\gui\configtool"

!ELSEIF  "$(CFG)" == "DTCConfigTool - Win32 Debug"

"libftd - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libftd"
   $(MAKE) /$(MAKEFLAGS) /F .\libftd.mak CFG="libftd - Win32 Debug" 
   cd "..\..\gui\configtool"

"libftd - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libftd"
   $(MAKE) /$(MAKEFLAGS) /F .\libftd.mak CFG="libftd - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\gui\configtool"

!ENDIF 

!IF  "$(CFG)" == "DTCConfigTool - Win32 Release"

"liblst - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liblst"
   $(MAKE) /$(MAKEFLAGS) /F .\liblst.mak CFG="liblst - Win32 Release" 
   cd "..\..\gui\configtool"

"liblst - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liblst"
   $(MAKE) /$(MAKEFLAGS) /F .\liblst.mak CFG="liblst - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\gui\configtool"

!ELSEIF  "$(CFG)" == "DTCConfigTool - Win32 Debug"

"liblst - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liblst"
   $(MAKE) /$(MAKEFLAGS) /F .\liblst.mak CFG="liblst - Win32 Debug" 
   cd "..\..\gui\configtool"

"liblst - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liblst"
   $(MAKE) /$(MAKEFLAGS) /F .\liblst.mak CFG="liblst - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\gui\configtool"

!ENDIF 

!IF  "$(CFG)" == "DTCConfigTool - Win32 Release"

"libmd5 - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libmd5"
   $(MAKE) /$(MAKEFLAGS) /F .\libmd5.mak CFG="libmd5 - Win32 Release" 
   cd "..\..\gui\configtool"

"libmd5 - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libmd5"
   $(MAKE) /$(MAKEFLAGS) /F .\libmd5.mak CFG="libmd5 - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\gui\configtool"

!ELSEIF  "$(CFG)" == "DTCConfigTool - Win32 Debug"

"libmd5 - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libmd5"
   $(MAKE) /$(MAKEFLAGS) /F .\libmd5.mak CFG="libmd5 - Win32 Debug" 
   cd "..\..\gui\configtool"

"libmd5 - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libmd5"
   $(MAKE) /$(MAKEFLAGS) /F .\libmd5.mak CFG="libmd5 - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\gui\configtool"

!ENDIF 

!IF  "$(CFG)" == "DTCConfigTool - Win32 Release"

"libproc - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libproc"
   $(MAKE) /$(MAKEFLAGS) /F .\libproc.mak CFG="libproc - Win32 Release" 
   cd "..\..\gui\configtool"

"libproc - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libproc"
   $(MAKE) /$(MAKEFLAGS) /F .\libproc.mak CFG="libproc - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\gui\configtool"

!ELSEIF  "$(CFG)" == "DTCConfigTool - Win32 Debug"

"libproc - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libproc"
   $(MAKE) /$(MAKEFLAGS) /F .\libproc.mak CFG="libproc - Win32 Debug" 
   cd "..\..\gui\configtool"

"libproc - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libproc"
   $(MAKE) /$(MAKEFLAGS) /F .\libproc.mak CFG="libproc - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\gui\configtool"

!ENDIF 

!IF  "$(CFG)" == "DTCConfigTool - Win32 Release"

"libsock - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libsock"
   $(MAKE) /$(MAKEFLAGS) /F .\libsock.mak CFG="libsock - Win32 Release" 
   cd "..\..\gui\configtool"

"libsock - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libsock"
   $(MAKE) /$(MAKEFLAGS) /F .\libsock.mak CFG="libsock - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\gui\configtool"

!ELSEIF  "$(CFG)" == "DTCConfigTool - Win32 Debug"

"libsock - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libsock"
   $(MAKE) /$(MAKEFLAGS) /F .\libsock.mak CFG="libsock - Win32 Debug" 
   cd "..\..\gui\configtool"

"libsock - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libsock"
   $(MAKE) /$(MAKEFLAGS) /F .\libsock.mak CFG="libsock - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\gui\configtool"

!ENDIF 

!IF  "$(CFG)" == "DTCConfigTool - Win32 Release"

"libutil - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libutil"
   $(MAKE) /$(MAKEFLAGS) /F .\libutil.mak CFG="libutil - Win32 Release" 
   cd "..\..\gui\configtool"

"libutil - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libutil"
   $(MAKE) /$(MAKEFLAGS) /F .\libutil.mak CFG="libutil - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\gui\configtool"

!ELSEIF  "$(CFG)" == "DTCConfigTool - Win32 Debug"

"libutil - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libutil"
   $(MAKE) /$(MAKEFLAGS) /F .\libutil.mak CFG="libutil - Win32 Debug" 
   cd "..\..\gui\configtool"

"libutil - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libutil"
   $(MAKE) /$(MAKEFLAGS) /F .\libutil.mak CFG="libutil - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\gui\configtool"

!ENDIF 

!IF  "$(CFG)" == "DTCConfigTool - Win32 Release"

"LibLic2 - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\Liblic2"
   $(MAKE) /$(MAKEFLAGS) /F .\LibLic2.mak CFG="LibLic2 - Win32 Release" 
   cd "..\..\gui\configtool"

"LibLic2 - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\Liblic2"
   $(MAKE) /$(MAKEFLAGS) /F .\LibLic2.mak CFG="LibLic2 - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\gui\configtool"

!ELSEIF  "$(CFG)" == "DTCConfigTool - Win32 Debug"

"LibLic2 - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\Liblic2"
   $(MAKE) /$(MAKEFLAGS) /F .\LibLic2.mak CFG="LibLic2 - Win32 Debug" 
   cd "..\..\gui\configtool"

"LibLic2 - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\Liblic2"
   $(MAKE) /$(MAKEFLAGS) /F .\LibLic2.mak CFG="LibLic2 - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\gui\configtool"

!ENDIF 

!IF  "$(CFG)" == "DTCConfigTool - Win32 Release"

"liblic - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liblic"
   $(MAKE) /$(MAKEFLAGS) /F .\liblic.mak CFG="liblic - Win32 Release" 
   cd "..\..\gui\configtool"

"liblic - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liblic"
   $(MAKE) /$(MAKEFLAGS) /F .\liblic.mak CFG="liblic - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\gui\configtool"

!ELSEIF  "$(CFG)" == "DTCConfigTool - Win32 Debug"

"liblic - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liblic"
   $(MAKE) /$(MAKEFLAGS) /F .\liblic.mak CFG="liblic - Win32 Debug" 
   cd "..\..\gui\configtool"

"liblic - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liblic"
   $(MAKE) /$(MAKEFLAGS) /F .\liblic.mak CFG="liblic - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\gui\configtool"

!ENDIF 


!ENDIF 

