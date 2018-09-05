# Microsoft Developer Studio Generated NMAKE File, Based on libftd.dsp
!IF "$(CFG)" == ""
CFG=libftd - Win32 Debug
!MESSAGE No configuration specified. Defaulting to libftd - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "libftd - Win32 Release" && "$(CFG)" != "libftd - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
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
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "libftd - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\libftd.lib"


CLEAN :
	-@erase "$(INTDIR)\ftd_cfgsys.obj"
	-@erase "$(INTDIR)\ftd_config.obj"
	-@erase "$(INTDIR)\ftd_cpoint.obj"
	-@erase "$(INTDIR)\ftd_dev.obj"
	-@erase "$(INTDIR)\ftd_devio.obj"
	-@erase "$(INTDIR)\ftd_devlock.obj"
	-@erase "$(INTDIR)\ftd_error.obj"
	-@erase "$(INTDIR)\ftd_fsm.obj"
	-@erase "$(INTDIR)\ftd_ioctl.obj"
	-@erase "$(INTDIR)\ftd_journal.obj"
	-@erase "$(INTDIR)\ftd_lg.obj"
	-@erase "$(INTDIR)\ftd_lic.obj"
	-@erase "$(INTDIR)\ftd_pathnames.obj"
	-@erase "$(INTDIR)\ftd_perf.obj"
	-@erase "$(INTDIR)\ftd_platform.obj"
	-@erase "$(INTDIR)\ftd_proc.obj"
	-@erase "$(INTDIR)\ftd_ps.obj"
	-@erase "$(INTDIR)\ftd_rsync.obj"
	-@erase "$(INTDIR)\ftd_sock.obj"
	-@erase "$(INTDIR)\ftd_stat.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(OUTDIR)\libftd.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90=df.exe
CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /Z7 /O2 /I "..\liblic2" /I "..\liberr" /I "..\liblic" /I "..\liblst" /I "..\libcomp" /I "..\libsock" /I "..\libutil" /I "..\libproc" /I "..\..\driver" /I "..\libmd5" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "_WINDOWS" /Fp"$(INTDIR)\libftd.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libftd.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\libftd.lib" 
LIB32_OBJS= \
	"$(INTDIR)\ftd_cfgsys.obj" \
	"$(INTDIR)\ftd_config.obj" \
	"$(INTDIR)\ftd_cpoint.obj" \
	"$(INTDIR)\ftd_dev.obj" \
	"$(INTDIR)\ftd_devio.obj" \
	"$(INTDIR)\ftd_devlock.obj" \
	"$(INTDIR)\ftd_error.obj" \
	"$(INTDIR)\ftd_fsm.obj" \
	"$(INTDIR)\ftd_ioctl.obj" \
	"$(INTDIR)\ftd_journal.obj" \
	"$(INTDIR)\ftd_lg.obj" \
	"$(INTDIR)\ftd_lic.obj" \
	"$(INTDIR)\ftd_pathnames.obj" \
	"$(INTDIR)\ftd_perf.obj" \
	"$(INTDIR)\ftd_platform.obj" \
	"$(INTDIR)\ftd_proc.obj" \
	"$(INTDIR)\ftd_ps.obj" \
	"$(INTDIR)\ftd_rsync.obj" \
	"$(INTDIR)\ftd_sock.obj" \
	"$(INTDIR)\ftd_stat.obj" \
	"$(INTDIR)\version.obj"

"$(OUTDIR)\libftd.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "libftd - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\libftd.lib"


CLEAN :
	-@erase "$(INTDIR)\ftd_cfgsys.obj"
	-@erase "$(INTDIR)\ftd_config.obj"
	-@erase "$(INTDIR)\ftd_cpoint.obj"
	-@erase "$(INTDIR)\ftd_dev.obj"
	-@erase "$(INTDIR)\ftd_devio.obj"
	-@erase "$(INTDIR)\ftd_devlock.obj"
	-@erase "$(INTDIR)\ftd_error.obj"
	-@erase "$(INTDIR)\ftd_fsm.obj"
	-@erase "$(INTDIR)\ftd_ioctl.obj"
	-@erase "$(INTDIR)\ftd_journal.obj"
	-@erase "$(INTDIR)\ftd_lg.obj"
	-@erase "$(INTDIR)\ftd_lic.obj"
	-@erase "$(INTDIR)\ftd_pathnames.obj"
	-@erase "$(INTDIR)\ftd_perf.obj"
	-@erase "$(INTDIR)\ftd_platform.obj"
	-@erase "$(INTDIR)\ftd_proc.obj"
	-@erase "$(INTDIR)\ftd_ps.obj"
	-@erase "$(INTDIR)\ftd_rsync.obj"
	-@erase "$(INTDIR)\ftd_sock.obj"
	-@erase "$(INTDIR)\ftd_stat.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(OUTDIR)\libftd.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90=df.exe
CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\liblic2" /I "..\liberr" /I "..\liblic" /I "..\liblst" /I "..\libcomp" /I "..\libsock" /I "..\libutil" /I "..\libproc" /I "..\..\driver" /I "..\libmd5" /D "_DEBUG" /D "_WINDOWS" /D "WIN32" /D "_MBCS" /D "_LIB" /Fp"$(INTDIR)\libftd.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libftd.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\libftd.lib" 
LIB32_OBJS= \
	"$(INTDIR)\ftd_cfgsys.obj" \
	"$(INTDIR)\ftd_config.obj" \
	"$(INTDIR)\ftd_cpoint.obj" \
	"$(INTDIR)\ftd_dev.obj" \
	"$(INTDIR)\ftd_devio.obj" \
	"$(INTDIR)\ftd_devlock.obj" \
	"$(INTDIR)\ftd_error.obj" \
	"$(INTDIR)\ftd_fsm.obj" \
	"$(INTDIR)\ftd_ioctl.obj" \
	"$(INTDIR)\ftd_journal.obj" \
	"$(INTDIR)\ftd_lg.obj" \
	"$(INTDIR)\ftd_lic.obj" \
	"$(INTDIR)\ftd_pathnames.obj" \
	"$(INTDIR)\ftd_perf.obj" \
	"$(INTDIR)\ftd_platform.obj" \
	"$(INTDIR)\ftd_proc.obj" \
	"$(INTDIR)\ftd_ps.obj" \
	"$(INTDIR)\ftd_rsync.obj" \
	"$(INTDIR)\ftd_sock.obj" \
	"$(INTDIR)\ftd_stat.obj" \
	"$(INTDIR)\version.obj"

"$(OUTDIR)\libftd.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("libftd.dep")
!INCLUDE "libftd.dep"
!ELSE 
!MESSAGE Warning: cannot find "libftd.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "libftd - Win32 Release" || "$(CFG)" == "libftd - Win32 Debug"
SOURCE=.\ftd_cfgsys.c

"$(INTDIR)\ftd_cfgsys.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ftd_config.c

"$(INTDIR)\ftd_config.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ftd_cpoint.c

"$(INTDIR)\ftd_cpoint.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ftd_dev.c

"$(INTDIR)\ftd_dev.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ftd_devio.c

"$(INTDIR)\ftd_devio.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ftd_devlock.c

"$(INTDIR)\ftd_devlock.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ftd_error.c

"$(INTDIR)\ftd_error.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ftd_fsm.c

"$(INTDIR)\ftd_fsm.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ftd_ioctl.c

"$(INTDIR)\ftd_ioctl.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ftd_journal.c

"$(INTDIR)\ftd_journal.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ftd_lg.c

"$(INTDIR)\ftd_lg.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ftd_lic.c

"$(INTDIR)\ftd_lic.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ftd_pathnames.c

"$(INTDIR)\ftd_pathnames.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ftd_perf.c

"$(INTDIR)\ftd_perf.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ftd_platform.c

"$(INTDIR)\ftd_platform.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ftd_proc.c

"$(INTDIR)\ftd_proc.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ftd_ps.c

"$(INTDIR)\ftd_ps.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ftd_rsync.c

"$(INTDIR)\ftd_rsync.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ftd_sock.c

"$(INTDIR)\ftd_sock.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ftd_stat.c

"$(INTDIR)\ftd_stat.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=..\..\version.c

"$(INTDIR)\version.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)



!ENDIF 

