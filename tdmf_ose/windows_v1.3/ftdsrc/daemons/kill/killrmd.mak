# Microsoft Developer Studio Generated NMAKE File, Based on killrmd.dsp
!IF "$(CFG)" == ""
CFG=killrmd - Win32 Debug
!MESSAGE No configuration specified. Defaulting to killrmd - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "killrmd - Win32 Release" && "$(CFG)" != "killrmd - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "killrmd.mak" CFG="killrmd - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "killrmd - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "killrmd - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "killrmd - Win32 Release"

OUTDIR=.\killrmd_Release
INTDIR=.\killrmd_Release
# Begin Custom Macros
OutDir=.\killrmd_Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\killrmd.exe"

!ELSE 

ALL : "libutil - Win32 Release" "libsock - Win32 Release" "libproc - Win32 Release" "libmd5 - Win32 Release" "liblst - Win32 Release" "libftd - Win32 Release" "liberr - Win32 Release" "libcomp - Win32 Release" "$(OUTDIR)\killrmd.exe"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"libcomp - Win32 ReleaseCLEAN" "liberr - Win32 ReleaseCLEAN" "libftd - Win32 ReleaseCLEAN" "liblst - Win32 ReleaseCLEAN" "libmd5 - Win32 ReleaseCLEAN" "libproc - Win32 ReleaseCLEAN" "libsock - Win32 ReleaseCLEAN" "libutil - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\ftd_kill.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(OUTDIR)\killrmd.exe"
	-@erase "$(OUTDIR)\killrmd.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90=df.exe
CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /Z7 /O2 /I "..\..\lib\libftd" /I "..\..\lib\libutil" /I "..\..\lib\liblst" /I "..\..\lib\libsock" /I "..\..\lib\libcomp" /I "..\..\lib\libproc" /I "..\..\lib\liberr" /I "..\..\driver" /D "NDEBUG" /D "_WINDOWS" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "FTD_KILL_RMD" /Fp"$(INTDIR)\killrmd.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\killrmd.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=$(TOP)\$(LIBLIC2REL) $(TOP)\$(LIBPROCREL) $(TOP)\$(LIBFTDREL) $(TOP)\$(LIBERRREL) $(TOP)\$(LIBUTILREL) $(TOP)\$(LIBSOCKREL) $(TOP)\$(LIBCOMPREL) $(TOP)\$(LIBLSTREL) $(TOP)\$(LIBMD5REL) ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\killrmd.pdb" /debug /debugtype:both /machine:I386 /out:"$(OUTDIR)\killrmd.exe" /opt:ref,icf 
LINK32_OBJS= \
	"$(INTDIR)\ftd_kill.obj" \
	"$(INTDIR)\version.obj" \
	"..\..\lib\libcomp\Release\libcomp.lib" \
	"..\..\lib\liberr\Release\liberr.lib" \
	"..\..\lib\libftd\Release\libftd.lib" \
	"..\..\lib\liblst\Release\liblst.lib" \
	"..\..\lib\libmd5\Release\libmd5.lib" \
	"..\..\lib\libproc\Release\libproc.lib" \
	"..\..\lib\libsock\Release\libsock.lib" \
	"..\..\lib\libutil\Release\libutil.lib"

"$(OUTDIR)\killrmd.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

TargetPath=.\killrmd_Release\killrmd.exe
TargetName=KillRMD
SOURCE="$(InputPath)"
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\killrmd_Release
# End Custom Macros

$(DS_POSTBUILD_DEP) : "libutil - Win32 Release" "libsock - Win32 Release" "libproc - Win32 Release" "libmd5 - Win32 Release" "liblst - Win32 Release" "libftd - Win32 Release" "liberr - Win32 Release" "libcomp - Win32 Release" "$(OUTDIR)\killrmd.exe"
   md C:\TDMF\Rep_5_01_NT\ftdsrc\"Replication"
	copy .\killrmd_Release\killrmd.exe C:\TDMF\Rep_5_01_NT\ftdsrc\"Replication"\lrkillrmd.exe
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "killrmd - Win32 Debug"

OUTDIR=.\killrmd_Debug
INTDIR=.\killrmd_Debug
# Begin Custom Macros
OutDir=.\killrmd_Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\killrmd.exe"

!ELSE 

ALL : "libutil - Win32 Debug" "libsock - Win32 Debug" "libproc - Win32 Debug" "libmd5 - Win32 Debug" "liblst - Win32 Debug" "libftd - Win32 Debug" "liberr - Win32 Debug" "libcomp - Win32 Debug" "$(OUTDIR)\killrmd.exe"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"libcomp - Win32 DebugCLEAN" "liberr - Win32 DebugCLEAN" "libftd - Win32 DebugCLEAN" "liblst - Win32 DebugCLEAN" "libmd5 - Win32 DebugCLEAN" "libproc - Win32 DebugCLEAN" "libsock - Win32 DebugCLEAN" "libutil - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\ftd_kill.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(OUTDIR)\killrmd.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90=df.exe
CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\lib\libftd" /I "..\..\lib\libutil" /I "..\..\lib\liblst" /I "..\..\lib\libsock" /I "..\..\lib\libcomp" /I "..\..\lib\libproc" /I "..\..\lib\liberr" /I "..\..\driver" /D "_DEBUG" /D "FTD_KILL_RMD" /D "_WINDOWS" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\killrmd.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\killrmd.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=$(TOP)\$(LIBPROCDBG) $(TOP)\$(LIBFTDDBG) $(TOP)\$(LIBERRDBG) $(TOP)\$(LIBUTILDBG) $(TOP)\$(LIBSOCKDBG) $(TOP)\$(LIBCOMPDBG) $(TOP)\$(LIBLSTDBG) $(TOP)\$(LIBMD5DBG) $(TOP)\$(LIBLIC2DBG) ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /profile /debug /machine:I386 /out:"$(OUTDIR)\killrmd.exe" 
LINK32_OBJS= \
	"$(INTDIR)\ftd_kill.obj" \
	"$(INTDIR)\version.obj" \
	"..\..\lib\libcomp\Debug\libcomp.lib" \
	"..\..\lib\liberr\Debug\liberr.lib" \
	"..\..\lib\libftd\Debug\libftd.lib" \
	"..\..\lib\liblst\Debug\liblst.lib" \
	"..\..\lib\libmd5\Debug\libmd5.lib" \
	"..\..\lib\libproc\Debug\libproc.lib" \
	"..\..\lib\libsock\Debug\libsock.lib" \
	"..\..\lib\libutil\Debug\libutil.lib"

"$(OUTDIR)\killrmd.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("killrmd.dep")
!INCLUDE "killrmd.dep"
!ELSE 
!MESSAGE Warning: cannot find "killrmd.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "killrmd - Win32 Release" || "$(CFG)" == "killrmd - Win32 Debug"
SOURCE=.\ftd_kill.c

"$(INTDIR)\ftd_kill.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=..\..\version.c

"$(INTDIR)\version.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!IF  "$(CFG)" == "killrmd - Win32 Release"

"libcomp - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libcomp"
   $(MAKE) /$(MAKEFLAGS) /F .\libcomp.mak CFG="libcomp - Win32 Release" 
   cd "..\..\daemons\kill"

"libcomp - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libcomp"
   $(MAKE) /$(MAKEFLAGS) /F .\libcomp.mak CFG="libcomp - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\daemons\kill"

!ELSEIF  "$(CFG)" == "killrmd - Win32 Debug"

"libcomp - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libcomp"
   $(MAKE) /$(MAKEFLAGS) /F .\libcomp.mak CFG="libcomp - Win32 Debug" 
   cd "..\..\daemons\kill"

"libcomp - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libcomp"
   $(MAKE) /$(MAKEFLAGS) /F .\libcomp.mak CFG="libcomp - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\daemons\kill"

!ENDIF 

!IF  "$(CFG)" == "killrmd - Win32 Release"

"liberr - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liberr"
   $(MAKE) /$(MAKEFLAGS) /F .\liberr.mak CFG="liberr - Win32 Release" 
   cd "..\..\daemons\kill"

"liberr - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liberr"
   $(MAKE) /$(MAKEFLAGS) /F .\liberr.mak CFG="liberr - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\daemons\kill"

!ELSEIF  "$(CFG)" == "killrmd - Win32 Debug"

"liberr - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liberr"
   $(MAKE) /$(MAKEFLAGS) /F .\liberr.mak CFG="liberr - Win32 Debug" 
   cd "..\..\daemons\kill"

"liberr - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liberr"
   $(MAKE) /$(MAKEFLAGS) /F .\liberr.mak CFG="liberr - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\daemons\kill"

!ENDIF 

!IF  "$(CFG)" == "killrmd - Win32 Release"

"libftd - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libftd"
   $(MAKE) /$(MAKEFLAGS) /F .\libftd.mak CFG="libftd - Win32 Release" 
   cd "..\..\daemons\kill"

"libftd - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libftd"
   $(MAKE) /$(MAKEFLAGS) /F .\libftd.mak CFG="libftd - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\daemons\kill"

!ELSEIF  "$(CFG)" == "killrmd - Win32 Debug"

"libftd - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libftd"
   $(MAKE) /$(MAKEFLAGS) /F .\libftd.mak CFG="libftd - Win32 Debug" 
   cd "..\..\daemons\kill"

"libftd - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libftd"
   $(MAKE) /$(MAKEFLAGS) /F .\libftd.mak CFG="libftd - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\daemons\kill"

!ENDIF 

!IF  "$(CFG)" == "killrmd - Win32 Release"

"liblst - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liblst"
   $(MAKE) /$(MAKEFLAGS) /F .\liblst.mak CFG="liblst - Win32 Release" 
   cd "..\..\daemons\kill"

"liblst - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liblst"
   $(MAKE) /$(MAKEFLAGS) /F .\liblst.mak CFG="liblst - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\daemons\kill"

!ELSEIF  "$(CFG)" == "killrmd - Win32 Debug"

"liblst - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liblst"
   $(MAKE) /$(MAKEFLAGS) /F .\liblst.mak CFG="liblst - Win32 Debug" 
   cd "..\..\daemons\kill"

"liblst - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liblst"
   $(MAKE) /$(MAKEFLAGS) /F .\liblst.mak CFG="liblst - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\daemons\kill"

!ENDIF 

!IF  "$(CFG)" == "killrmd - Win32 Release"

"libmd5 - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libmd5"
   $(MAKE) /$(MAKEFLAGS) /F .\libmd5.mak CFG="libmd5 - Win32 Release" 
   cd "..\..\daemons\kill"

"libmd5 - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libmd5"
   $(MAKE) /$(MAKEFLAGS) /F .\libmd5.mak CFG="libmd5 - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\daemons\kill"

!ELSEIF  "$(CFG)" == "killrmd - Win32 Debug"

"libmd5 - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libmd5"
   $(MAKE) /$(MAKEFLAGS) /F .\libmd5.mak CFG="libmd5 - Win32 Debug" 
   cd "..\..\daemons\kill"

"libmd5 - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libmd5"
   $(MAKE) /$(MAKEFLAGS) /F .\libmd5.mak CFG="libmd5 - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\daemons\kill"

!ENDIF 

!IF  "$(CFG)" == "killrmd - Win32 Release"

"libproc - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libproc"
   $(MAKE) /$(MAKEFLAGS) /F .\libproc.mak CFG="libproc - Win32 Release" 
   cd "..\..\daemons\kill"

"libproc - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libproc"
   $(MAKE) /$(MAKEFLAGS) /F .\libproc.mak CFG="libproc - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\daemons\kill"

!ELSEIF  "$(CFG)" == "killrmd - Win32 Debug"

"libproc - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libproc"
   $(MAKE) /$(MAKEFLAGS) /F .\libproc.mak CFG="libproc - Win32 Debug" 
   cd "..\..\daemons\kill"

"libproc - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libproc"
   $(MAKE) /$(MAKEFLAGS) /F .\libproc.mak CFG="libproc - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\daemons\kill"

!ENDIF 

!IF  "$(CFG)" == "killrmd - Win32 Release"

"libsock - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libsock"
   $(MAKE) /$(MAKEFLAGS) /F .\libsock.mak CFG="libsock - Win32 Release" 
   cd "..\..\daemons\kill"

"libsock - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libsock"
   $(MAKE) /$(MAKEFLAGS) /F .\libsock.mak CFG="libsock - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\daemons\kill"

!ELSEIF  "$(CFG)" == "killrmd - Win32 Debug"

"libsock - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libsock"
   $(MAKE) /$(MAKEFLAGS) /F .\libsock.mak CFG="libsock - Win32 Debug" 
   cd "..\..\daemons\kill"

"libsock - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libsock"
   $(MAKE) /$(MAKEFLAGS) /F .\libsock.mak CFG="libsock - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\daemons\kill"

!ENDIF 

!IF  "$(CFG)" == "killrmd - Win32 Release"

"libutil - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libutil"
   $(MAKE) /$(MAKEFLAGS) /F .\libutil.mak CFG="libutil - Win32 Release" 
   cd "..\..\daemons\kill"

"libutil - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libutil"
   $(MAKE) /$(MAKEFLAGS) /F .\libutil.mak CFG="libutil - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\daemons\kill"

!ELSEIF  "$(CFG)" == "killrmd - Win32 Debug"

"libutil - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libutil"
   $(MAKE) /$(MAKEFLAGS) /F .\libutil.mak CFG="libutil - Win32 Debug" 
   cd "..\..\daemons\kill"

"libutil - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libutil"
   $(MAKE) /$(MAKEFLAGS) /F .\libutil.mak CFG="libutil - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\daemons\kill"

!ENDIF 


!ENDIF 

