# Microsoft Developer Studio Generated NMAKE File, Based on license.dsp
!IF "$(CFG)" == ""
CFG=license - Win32 Debug
!MESSAGE No configuration specified. Defaulting to license - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "license - Win32 Release" && "$(CFG)" != "license - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "license.mak" CFG="license - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "license - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "license - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "license - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\license.dll"


CLEAN :
	-@erase "$(INTDIR)\license.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\license.dll"
	-@erase "$(OUTDIR)\license.exp"
	-@erase "$(OUTDIR)\license.lib"
	-@erase "$(OUTDIR)\license.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /Z7 /O2 /I "..\..\lib\liblic2" /I "..\..\lib\liblic" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LICENSE_EXPORTS" /Fp"$(INTDIR)\license.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\license.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=$(TOP)\$(LIBERRREL) $(TOP)\$(LIBLSTREL) $(TOP)\$(LIBUTILREL) $(TOP)\$(LIBLICREL) $(TOP)\$(LIBSOCKREL) $(TOP)\$(LIBFTDREL) kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\license.pdb" /debug /debugtype:both /machine:I386 /def:".\license.def" /out:"$(OUTDIR)\license.dll" /implib:"$(OUTDIR)\license.lib" /opt:ref,icf 
LINK32_OBJS= \
	"$(INTDIR)\license.obj"

"$(OUTDIR)\license.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "license - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\license.dll"


CLEAN :
	-@erase "$(INTDIR)\license.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\license.dll"
	-@erase "$(OUTDIR)\license.exp"
	-@erase "$(OUTDIR)\license.ilk"
	-@erase "$(OUTDIR)\license.lib"
	-@erase "$(OUTDIR)\license.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\lib\liblic2 ..\..\lib\liblic" /I "..\..\lib\liblic2" /I "..\..\lib\liblic" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LICENSE_EXPORTS" /Fp"$(INTDIR)\license.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\license.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=$(TOP)\$(LIBERRDBG) $(TOP)\$(LIBLSTDBG) $(TOP)\$(LIBUTILDBG) $(TOP)\$(LIBLICDBG) $(TOP)\$(LIBSOCKDBG) $(TOP)\$(LIBFTDDBG) kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\license.pdb" /debug /machine:I386 /def:".\license.def" /out:"$(OUTDIR)\license.dll" /implib:"$(OUTDIR)\license.lib" /pdbtype:sept 
DEF_FILE= \
	".\license.def"
LINK32_OBJS= \
	"$(INTDIR)\license.obj"

"$(OUTDIR)\license.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("license.dep")
!INCLUDE "license.dep"
!ELSE 
!MESSAGE Warning: cannot find "license.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "license - Win32 Release" || "$(CFG)" == "license - Win32 Debug"
SOURCE=.\license.c

"$(INTDIR)\license.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

