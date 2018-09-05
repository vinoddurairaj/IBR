# Microsoft Developer Studio Generated NMAKE File, Based on checkpoint.dsp
!IF "$(CFG)" == ""
CFG=checkpoint - Win32 Debug
!MESSAGE No configuration specified. Defaulting to checkpoint - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "checkpoint - Win32 Release" && "$(CFG)" != "checkpoint - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "checkpoint.mak" CFG="checkpoint - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "checkpoint - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "checkpoint - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "checkpoint - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\checkpoint.exe"

!ELSE 

ALL : "libutil - Win32 Release" "libsock - Win32 Release" "libproc - Win32 Release" "libmd5 - Win32 Release" "liblst - Win32 Release" "libftd - Win32 Release" "liberr - Win32 Release" "libcomp - Win32 Release" "$(OUTDIR)\checkpoint.exe"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"libcomp - Win32 ReleaseCLEAN" "liberr - Win32 ReleaseCLEAN" "libftd - Win32 ReleaseCLEAN" "liblst - Win32 ReleaseCLEAN" "libmd5 - Win32 ReleaseCLEAN" "libproc - Win32 ReleaseCLEAN" "libsock - Win32 ReleaseCLEAN" "libutil - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\ftd_cp.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(OUTDIR)\checkpoint.exe"
	-@erase "$(OUTDIR)\checkpoint.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90=df.exe
CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /Z7 /O2 /I "..\..\lib\liblic" /I "..\..\lib\libftd" /I "..\..\driver" /I "..\..\lib\libutil" /I "..\..\lib\liberr" /I "..\..\lib\liblst" /I "..\..\lib\libproc" /I "..\..\lib\libcomp" /I "..\..\lib\libsock" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_WINDOWS" /Fp"$(INTDIR)\checkpoint.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\checkpoint.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=$(TOP)\$(LIBPROCREL) $(TOP)\$(LIBFTDREL) $(TOP)\$(LIBERRREL) $(TOP)\$(LIBUTILREL) $(TOP)\$(LIBSOCKREL) $(TOP)\$(LIBCOMPREL) $(TOP)\$(LIBLSTREL) $(TOP)\$(LIBMD5REL) ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\checkpoint.pdb" /debug /debugtype:both /machine:I386 /out:"$(OUTDIR)\checkpoint.exe" /opt:ref,icf 
LINK32_OBJS= \
	"$(INTDIR)\ftd_cp.obj" \
	"$(INTDIR)\version.obj" \
	"..\..\lib\libcomp\Release\libcomp.lib" \
	"..\..\lib\liberr\Release\liberr.lib" \
	"..\..\lib\libftd\Release\libftd.lib" \
	"..\..\lib\liblst\Release\liblst.lib" \
	"..\..\lib\libmd5\Release\libmd5.lib" \
	"..\..\lib\libproc\Release\libproc.lib" \
	"..\..\lib\libsock\Release\libsock.lib" \
	"..\..\lib\libutil\Release\libutil.lib"

"$(OUTDIR)\checkpoint.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

TargetPath=.\Release\checkpoint.exe
TargetName=Checkpoint
SOURCE="$(InputPath)"
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

$(DS_POSTBUILD_DEP) : "libutil - Win32 Release" "libsock - Win32 Release" "libproc - Win32 Release" "libmd5 - Win32 Release" "liblst - Win32 Release" "libftd - Win32 Release" "liberr - Win32 Release" "libcomp - Win32 Release" "$(OUTDIR)\checkpoint.exe"
   md C:\TDMF\Rep_5_01_NT\ftdsrc\"Replication"
	copy .\Release\checkpoint.exe C:\TDMF\Rep_5_01_NT\ftdsrc\"Replication"\lrcheckpoint.exe
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "checkpoint - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\checkpoint.exe"

!ELSE 

ALL : "libutil - Win32 Debug" "libsock - Win32 Debug" "libproc - Win32 Debug" "libmd5 - Win32 Debug" "liblst - Win32 Debug" "libftd - Win32 Debug" "liberr - Win32 Debug" "libcomp - Win32 Debug" "$(OUTDIR)\checkpoint.exe"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"libcomp - Win32 DebugCLEAN" "liberr - Win32 DebugCLEAN" "libftd - Win32 DebugCLEAN" "liblst - Win32 DebugCLEAN" "libmd5 - Win32 DebugCLEAN" "libproc - Win32 DebugCLEAN" "libsock - Win32 DebugCLEAN" "libutil - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\ftd_cp.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(OUTDIR)\checkpoint.exe"
	-@erase "$(OUTDIR)\checkpoint.ilk"
	-@erase "$(OUTDIR)\checkpoint.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90=df.exe
CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\lib\libcomp" /I "..\..\lib\libproc" /I "..\..\lib\libsock" /I "..\..\driver" /I "..\..\lib\libftd" /I "..\..\lib\libutil" /I "..\..\lib\liblst" /I "..\..\lib\liberr" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_WINDOWS" /Fp"$(INTDIR)\checkpoint.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\checkpoint.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=$(TOP)\$(LIBPROCDBG) $(TOP)\$(LIBFTDDBG) $(TOP)\$(LIBERRDBG) $(TOP)\$(LIBUTILDBG) $(TOP)\$(LIBSOCKDBG) $(TOP)\$(LIBCOMPDBG) $(TOP)\$(LIBLSTDBG) $(TOP)\$(LIBMD5DBG) kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\checkpoint.pdb" /debug /machine:I386 /out:"$(OUTDIR)\checkpoint.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\ftd_cp.obj" \
	"$(INTDIR)\version.obj" \
	"..\..\lib\libcomp\Debug\libcomp.lib" \
	"..\..\lib\liberr\Debug\liberr.lib" \
	"..\..\lib\libftd\Debug\libftd.lib" \
	"..\..\lib\liblst\Debug\liblst.lib" \
	"..\..\lib\libmd5\Debug\libmd5.lib" \
	"..\..\lib\libproc\Debug\libproc.lib" \
	"..\..\lib\libsock\Debug\libsock.lib" \
	"..\..\lib\libutil\Debug\libutil.lib"

"$(OUTDIR)\checkpoint.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("checkpoint.dep")
!INCLUDE "checkpoint.dep"
!ELSE 
!MESSAGE Warning: cannot find "checkpoint.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "checkpoint - Win32 Release" || "$(CFG)" == "checkpoint - Win32 Debug"
SOURCE=.\ftd_cp.c

"$(INTDIR)\ftd_cp.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=..\..\version.c

"$(INTDIR)\version.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!IF  "$(CFG)" == "checkpoint - Win32 Release"

"libcomp - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libcomp"
   $(MAKE) /$(MAKEFLAGS) /F .\libcomp.mak CFG="libcomp - Win32 Release" 
   cd "..\..\libexec\checkpoint"

"libcomp - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libcomp"
   $(MAKE) /$(MAKEFLAGS) /F .\libcomp.mak CFG="libcomp - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\libexec\checkpoint"

!ELSEIF  "$(CFG)" == "checkpoint - Win32 Debug"

"libcomp - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libcomp"
   $(MAKE) /$(MAKEFLAGS) /F .\libcomp.mak CFG="libcomp - Win32 Debug" 
   cd "..\..\libexec\checkpoint"

"libcomp - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libcomp"
   $(MAKE) /$(MAKEFLAGS) /F .\libcomp.mak CFG="libcomp - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\libexec\checkpoint"

!ENDIF 

!IF  "$(CFG)" == "checkpoint - Win32 Release"

"liberr - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liberr"
   $(MAKE) /$(MAKEFLAGS) /F .\liberr.mak CFG="liberr - Win32 Release" 
   cd "..\..\libexec\checkpoint"

"liberr - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liberr"
   $(MAKE) /$(MAKEFLAGS) /F .\liberr.mak CFG="liberr - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\libexec\checkpoint"

!ELSEIF  "$(CFG)" == "checkpoint - Win32 Debug"

"liberr - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liberr"
   $(MAKE) /$(MAKEFLAGS) /F .\liberr.mak CFG="liberr - Win32 Debug" 
   cd "..\..\libexec\checkpoint"

"liberr - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liberr"
   $(MAKE) /$(MAKEFLAGS) /F .\liberr.mak CFG="liberr - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\libexec\checkpoint"

!ENDIF 

!IF  "$(CFG)" == "checkpoint - Win32 Release"

"libftd - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libftd"
   $(MAKE) /$(MAKEFLAGS) /F .\libftd.mak CFG="libftd - Win32 Release" 
   cd "..\..\libexec\checkpoint"

"libftd - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libftd"
   $(MAKE) /$(MAKEFLAGS) /F .\libftd.mak CFG="libftd - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\libexec\checkpoint"

!ELSEIF  "$(CFG)" == "checkpoint - Win32 Debug"

"libftd - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libftd"
   $(MAKE) /$(MAKEFLAGS) /F .\libftd.mak CFG="libftd - Win32 Debug" 
   cd "..\..\libexec\checkpoint"

"libftd - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libftd"
   $(MAKE) /$(MAKEFLAGS) /F .\libftd.mak CFG="libftd - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\libexec\checkpoint"

!ENDIF 

!IF  "$(CFG)" == "checkpoint - Win32 Release"

"liblst - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liblst"
   $(MAKE) /$(MAKEFLAGS) /F .\liblst.mak CFG="liblst - Win32 Release" 
   cd "..\..\libexec\checkpoint"

"liblst - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liblst"
   $(MAKE) /$(MAKEFLAGS) /F .\liblst.mak CFG="liblst - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\libexec\checkpoint"

!ELSEIF  "$(CFG)" == "checkpoint - Win32 Debug"

"liblst - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liblst"
   $(MAKE) /$(MAKEFLAGS) /F .\liblst.mak CFG="liblst - Win32 Debug" 
   cd "..\..\libexec\checkpoint"

"liblst - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\liblst"
   $(MAKE) /$(MAKEFLAGS) /F .\liblst.mak CFG="liblst - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\libexec\checkpoint"

!ENDIF 

!IF  "$(CFG)" == "checkpoint - Win32 Release"

"libmd5 - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libmd5"
   $(MAKE) /$(MAKEFLAGS) /F .\libmd5.mak CFG="libmd5 - Win32 Release" 
   cd "..\..\libexec\checkpoint"

"libmd5 - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libmd5"
   $(MAKE) /$(MAKEFLAGS) /F .\libmd5.mak CFG="libmd5 - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\libexec\checkpoint"

!ELSEIF  "$(CFG)" == "checkpoint - Win32 Debug"

"libmd5 - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libmd5"
   $(MAKE) /$(MAKEFLAGS) /F .\libmd5.mak CFG="libmd5 - Win32 Debug" 
   cd "..\..\libexec\checkpoint"

"libmd5 - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libmd5"
   $(MAKE) /$(MAKEFLAGS) /F .\libmd5.mak CFG="libmd5 - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\libexec\checkpoint"

!ENDIF 

!IF  "$(CFG)" == "checkpoint - Win32 Release"

"libproc - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libproc"
   $(MAKE) /$(MAKEFLAGS) /F .\libproc.mak CFG="libproc - Win32 Release" 
   cd "..\..\libexec\checkpoint"

"libproc - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libproc"
   $(MAKE) /$(MAKEFLAGS) /F .\libproc.mak CFG="libproc - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\libexec\checkpoint"

!ELSEIF  "$(CFG)" == "checkpoint - Win32 Debug"

"libproc - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libproc"
   $(MAKE) /$(MAKEFLAGS) /F .\libproc.mak CFG="libproc - Win32 Debug" 
   cd "..\..\libexec\checkpoint"

"libproc - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libproc"
   $(MAKE) /$(MAKEFLAGS) /F .\libproc.mak CFG="libproc - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\libexec\checkpoint"

!ENDIF 

!IF  "$(CFG)" == "checkpoint - Win32 Release"

"libsock - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libsock"
   $(MAKE) /$(MAKEFLAGS) /F .\libsock.mak CFG="libsock - Win32 Release" 
   cd "..\..\libexec\checkpoint"

"libsock - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libsock"
   $(MAKE) /$(MAKEFLAGS) /F .\libsock.mak CFG="libsock - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\libexec\checkpoint"

!ELSEIF  "$(CFG)" == "checkpoint - Win32 Debug"

"libsock - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libsock"
   $(MAKE) /$(MAKEFLAGS) /F .\libsock.mak CFG="libsock - Win32 Debug" 
   cd "..\..\libexec\checkpoint"

"libsock - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libsock"
   $(MAKE) /$(MAKEFLAGS) /F .\libsock.mak CFG="libsock - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\libexec\checkpoint"

!ENDIF 

!IF  "$(CFG)" == "checkpoint - Win32 Release"

"libutil - Win32 Release" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libutil"
   $(MAKE) /$(MAKEFLAGS) /F .\libutil.mak CFG="libutil - Win32 Release" 
   cd "..\..\libexec\checkpoint"

"libutil - Win32 ReleaseCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libutil"
   $(MAKE) /$(MAKEFLAGS) /F .\libutil.mak CFG="libutil - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\libexec\checkpoint"

!ELSEIF  "$(CFG)" == "checkpoint - Win32 Debug"

"libutil - Win32 Debug" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libutil"
   $(MAKE) /$(MAKEFLAGS) /F .\libutil.mak CFG="libutil - Win32 Debug" 
   cd "..\..\libexec\checkpoint"

"libutil - Win32 DebugCLEAN" : 
   cd "\TDMF\Rep_5_01_NT\ftdsrc\lib\libutil"
   $(MAKE) /$(MAKEFLAGS) /F .\libutil.mak CFG="libutil - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\libexec\checkpoint"

!ENDIF 


!ENDIF 

