# Microsoft Developer Studio Generated NMAKE File, Based on launchpmd.dsp
!IF "$(CFG)" == ""
CFG=launchpmd - Win32 Debug
!MESSAGE No configuration specified. Defaulting to launchpmd - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "launchpmd - Win32 Release" && "$(CFG)" != "launchpmd - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "launchpmd.mak" CFG="launchpmd - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "launchpmd - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "launchpmd - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "launchpmd - Win32 Release"

OUTDIR=.\launchpmd_Release
INTDIR=.\launchpmd_Release
# Begin Custom Macros
OutDir=.\launchpmd_Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\launchpmd.exe"

!ELSE 

ALL : "libutil - Win32 Release" "libsock - Win32 Release" "libproc - Win32 Release" "libmd5 - Win32 Release" "liblst - Win32 Release" "libftd - Win32 Release" "liberr - Win32 Release" "libcomp - Win32 Release" "$(OUTDIR)\launchpmd.exe"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"libcomp - Win32 ReleaseCLEAN" "liberr - Win32 ReleaseCLEAN" "libftd - Win32 ReleaseCLEAN" "liblst - Win32 ReleaseCLEAN" "libmd5 - Win32 ReleaseCLEAN" "libproc - Win32 ReleaseCLEAN" "libsock - Win32 ReleaseCLEAN" "libutil - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\ftd_launch.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(OUTDIR)\launchpmd.exe"
	-@erase "$(OUTDIR)\launchpmd.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90=df.exe
CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /Z7 /O2 /I "..\..\lib\libftd" /I "..\..\lib\libutil" /I "..\..\lib\liblst" /I "..\..\lib\libsock" /I "..\..\lib\libcomp" /I "..\..\lib\libproc" /I "..\..\lib\liberr" /I "..\..\driver" /D "NDEBUG" /D "_WINDOWS" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "FTD_LAUNCH_NORMAL" /Fp"$(INTDIR)\launchpmd.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\launchpmd.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=$(TOP)\$(LIBLIC2REL) $(TOP)\$(LIBPROCREL) $(TOP)\$(LIBFTDREL) $(TOP)\$(LIBERRREL) $(TOP)\$(LIBUTILREL) $(TOP)\$(LIBSOCKREL) $(TOP)\$(LIBCOMPREL) $(TOP)\$(LIBLSTREL) $(TOP)\$(LIBMD5REL) ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\launchpmd.pdb" /debug /debugtype:both /machine:I386 /out:"$(OUTDIR)\launchpmd.exe" /opt:ref,icf 
LINK32_OBJS= \
	"$(INTDIR)\ftd_launch.obj" \
	"$(INTDIR)\version.obj" \
	"..\..\lib\libcomp\Release\libcomp.lib" \
	"..\..\lib\liberr\Release\liberr.lib" \
	"..\..\lib\libftd\Release\libftd.lib" \
	"..\..\lib\liblst\Release\liblst.lib" \
	"..\..\lib\libmd5\Release\libmd5.lib" \
	"..\..\lib\libproc\Release\libproc.lib" \
	"..\..\lib\libsock\Release\libsock.lib" \
	"..\..\lib\libutil\Release\libutil.lib"

"$(OUTDIR)\launchpmd.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

TargetPath=.\launchpmd_Release\launchpmd.exe
TargetName=LaunchPMD
SOURCE="$(InputPath)"
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\launchpmd_Release
# End Custom Macros

$(DS_POSTBUILD_DEP) : "libutil - Win32 Release" "libsock - Win32 Release" "libproc - Win32 Release" "libmd5 - Win32 Release" "liblst - Win32 Release" "libftd - Win32 Release" "liberr - Win32 Release" "libcomp - Win32 Release" "$(OUTDIR)\launchpmd.exe"
   md C:\TDMF\Rep_5_01_NT\ftdsrc\"Replication"
	copy .\launchpmd_Release\launchpmd.exe C:\TDMF\Rep_5_01_NT\ftdsrc\"Replication"\lrlaunchpmd.exe
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "launchpmd - Win32 Debug"

OUTDIR=.\launchpmd_Debug
INTDIR=.\launchpmd_Debug
# Begin Custom Macros
OutDir=.\launchpmd_Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\launchpmd.exe"

!ELSE 

ALL : "libutil - Win32 Debug" "libsock - Win32 Debug" "libproc - Win32 Debug" "libmd5 - Win32 Debug" "liblst - Win32 Debug" "libftd - Win32 Debug" "liberr - Win32 Debug" "libcomp - Win32 Debug" "$(OUTDIR)\launchpmd.exe"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"libcomp - Win32 DebugCLEAN" "liberr - Win32 DebugCLEAN" "libftd - Win32 DebugCLEAN" "liblst - Win32 DebugCLEAN" "libmd5 - Win32 DebugCLEAN" "libproc - Win32 DebugCLEAN" "libsock - Win32 DebugCLEAN" "libutil - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\ftd_launch.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(OUTDIR)\launchpmd.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90=df.exe
CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\lib\libftd" /I "..\..\lib\libutil" /I "..\..\lib\liblst" /I "..\..\lib\libsock" /I "..\..\lib\libcomp" /I "..\..\lib\libproc" /I "..\..\lib\liberr" /I "..\..\driver" /D "_DEBUG" /D "FTD_LAUNCH_NORMAL" /D "_WINDOWS" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\launchpmd.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\launchpmd.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=$(TOP)\$(LIBPROCDBG) $(TOP)\$(LIBFTDDBG) $(TOP)\$(LIBERRDBG) $(TOP)\$(LIBUTILDBG) $(TOP)\$(LIBSOCKDBG) $(TOP)\$(LIBCOMPDBG) $(TOP)\$(LIBLSTDBG) $(TOP)\$(LIBMD5DBG) $(TOP)\$(LIBLIC2DBG) ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /profile /debug /machine:I386 /out:"$(OUTDIR)\launchpmd.exe" 
LINK32_OBJS= \
	"$(INTDIR)\ftd_launch.obj" \
	"$(INTDIR)\version.obj" \
	"..\..\lib\libcomp\Debug\libcomp.lib" \
	"..\..\lib\liberr\Debug\liberr.lib" \
	"..\..\lib\libftd\Debug\libftd.lib" \
	"..\..\lib\liblst\Debug\liblst.lib" \
	"..\..\lib\libmd5\Debug\libmd5.lib" \
	"..\..\lib\libproc\Debug\libproc.lib" \
	"..\..\lib\libsock\Debug\libsock.lib" \
	"..\..\lib\libutil\Debug\libutil.lib"

"$(OUTDIR)\launchpmd.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("launchpmd.dep")
!INCLUDE "launchpmd.dep"
!ELSE 
!MESSAGE Warning: cannot find "launchpmd.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "launchpmd - Win32 Release" || "$(CFG)" == "launchpmd - Win32 Debug"
SOURCE=.\ftd_launch.c

"$(INTDIR)\ftd_launch.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=..\..\version.c

"$(INTDIR)\version.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!IF  "$(CFG)" == "launchpmd - Win32 Release"

"libcomp - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libcomp"
   $(MAKE) /$(MAKEFLAGS) /F .\libcomp.mak CFG="libcomp - Win32 Release" 
   cd "..\..\daemons\launch"

"libcomp - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libcomp"
   $(MAKE) /$(MAKEFLAGS) /F .\libcomp.mak CFG="libcomp - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\daemons\launch"

!ELSEIF  "$(CFG)" == "launchpmd - Win32 Debug"

"libcomp - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libcomp"
   $(MAKE) /$(MAKEFLAGS) /F .\libcomp.mak CFG="libcomp - Win32 Debug" 
   cd "..\..\daemons\launch"

"libcomp - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libcomp"
   $(MAKE) /$(MAKEFLAGS) /F .\libcomp.mak CFG="libcomp - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\daemons\launch"

!ENDIF 

!IF  "$(CFG)" == "launchpmd - Win32 Release"

"liberr - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liberr"
   $(MAKE) /$(MAKEFLAGS) /F .\liberr.mak CFG="liberr - Win32 Release" 
   cd "..\..\daemons\launch"

"liberr - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liberr"
   $(MAKE) /$(MAKEFLAGS) /F .\liberr.mak CFG="liberr - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\daemons\launch"

!ELSEIF  "$(CFG)" == "launchpmd - Win32 Debug"

"liberr - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liberr"
   $(MAKE) /$(MAKEFLAGS) /F .\liberr.mak CFG="liberr - Win32 Debug" 
   cd "..\..\daemons\launch"

"liberr - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liberr"
   $(MAKE) /$(MAKEFLAGS) /F .\liberr.mak CFG="liberr - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\daemons\launch"

!ENDIF 

!IF  "$(CFG)" == "launchpmd - Win32 Release"

"libftd - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libftd"
   $(MAKE) /$(MAKEFLAGS) /F .\libftd.mak CFG="libftd - Win32 Release" 
   cd "..\..\daemons\launch"

"libftd - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libftd"
   $(MAKE) /$(MAKEFLAGS) /F .\libftd.mak CFG="libftd - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\daemons\launch"

!ELSEIF  "$(CFG)" == "launchpmd - Win32 Debug"

"libftd - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libftd"
   $(MAKE) /$(MAKEFLAGS) /F .\libftd.mak CFG="libftd - Win32 Debug" 
   cd "..\..\daemons\launch"

"libftd - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libftd"
   $(MAKE) /$(MAKEFLAGS) /F .\libftd.mak CFG="libftd - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\daemons\launch"

!ENDIF 

!IF  "$(CFG)" == "launchpmd - Win32 Release"

"liblst - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liblst"
   $(MAKE) /$(MAKEFLAGS) /F .\liblst.mak CFG="liblst - Win32 Release" 
   cd "..\..\daemons\launch"

"liblst - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liblst"
   $(MAKE) /$(MAKEFLAGS) /F .\liblst.mak CFG="liblst - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\daemons\launch"

!ELSEIF  "$(CFG)" == "launchpmd - Win32 Debug"

"liblst - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liblst"
   $(MAKE) /$(MAKEFLAGS) /F .\liblst.mak CFG="liblst - Win32 Debug" 
   cd "..\..\daemons\launch"

"liblst - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liblst"
   $(MAKE) /$(MAKEFLAGS) /F .\liblst.mak CFG="liblst - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\daemons\launch"

!ENDIF 

!IF  "$(CFG)" == "launchpmd - Win32 Release"

"libmd5 - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libmd5"
   $(MAKE) /$(MAKEFLAGS) /F .\libmd5.mak CFG="libmd5 - Win32 Release" 
   cd "..\..\daemons\launch"

"libmd5 - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libmd5"
   $(MAKE) /$(MAKEFLAGS) /F .\libmd5.mak CFG="libmd5 - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\daemons\launch"

!ELSEIF  "$(CFG)" == "launchpmd - Win32 Debug"

"libmd5 - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libmd5"
   $(MAKE) /$(MAKEFLAGS) /F .\libmd5.mak CFG="libmd5 - Win32 Debug" 
   cd "..\..\daemons\launch"

"libmd5 - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libmd5"
   $(MAKE) /$(MAKEFLAGS) /F .\libmd5.mak CFG="libmd5 - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\daemons\launch"

!ENDIF 

!IF  "$(CFG)" == "launchpmd - Win32 Release"

"libproc - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libproc"
   $(MAKE) /$(MAKEFLAGS) /F .\libproc.mak CFG="libproc - Win32 Release" 
   cd "..\..\daemons\launch"

"libproc - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libproc"
   $(MAKE) /$(MAKEFLAGS) /F .\libproc.mak CFG="libproc - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\daemons\launch"

!ELSEIF  "$(CFG)" == "launchpmd - Win32 Debug"

"libproc - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libproc"
   $(MAKE) /$(MAKEFLAGS) /F .\libproc.mak CFG="libproc - Win32 Debug" 
   cd "..\..\daemons\launch"

"libproc - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libproc"
   $(MAKE) /$(MAKEFLAGS) /F .\libproc.mak CFG="libproc - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\daemons\launch"

!ENDIF 

!IF  "$(CFG)" == "launchpmd - Win32 Release"

"libsock - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libsock"
   $(MAKE) /$(MAKEFLAGS) /F .\libsock.mak CFG="libsock - Win32 Release" 
   cd "..\..\daemons\launch"

"libsock - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libsock"
   $(MAKE) /$(MAKEFLAGS) /F .\libsock.mak CFG="libsock - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\daemons\launch"

!ELSEIF  "$(CFG)" == "launchpmd - Win32 Debug"

"libsock - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libsock"
   $(MAKE) /$(MAKEFLAGS) /F .\libsock.mak CFG="libsock - Win32 Debug" 
   cd "..\..\daemons\launch"

"libsock - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libsock"
   $(MAKE) /$(MAKEFLAGS) /F .\libsock.mak CFG="libsock - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\daemons\launch"

!ENDIF 

!IF  "$(CFG)" == "launchpmd - Win32 Release"

"libutil - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libutil"
   $(MAKE) /$(MAKEFLAGS) /F .\libutil.mak CFG="libutil - Win32 Release" 
   cd "..\..\daemons\launch"

"libutil - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libutil"
   $(MAKE) /$(MAKEFLAGS) /F .\libutil.mak CFG="libutil - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\daemons\launch"

!ELSEIF  "$(CFG)" == "launchpmd - Win32 Debug"

"libutil - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libutil"
   $(MAKE) /$(MAKEFLAGS) /F .\libutil.mak CFG="libutil - Win32 Debug" 
   cd "..\..\daemons\launch"

"libutil - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libutil"
   $(MAKE) /$(MAKEFLAGS) /F .\libutil.mak CFG="libutil - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\daemons\launch"

!ENDIF 


!ENDIF 

