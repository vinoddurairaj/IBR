# Microsoft Developer Studio Generated NMAKE File, Based on driver.dsp
!IF "$(CFG)" == ""
CFG=driver - Win32 Debug
!MESSAGE No configuration specified. Defaulting to driver - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "driver - Win32 Release" && "$(CFG)" != "driver - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "driver.mak" CFG="driver - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "driver - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "driver - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "driver - Win32 Release"

OUTDIR=.\Free
INTDIR=.\Free
# Begin Custom Macros
OutDir=.\Free
# End Custom Macros

ALL : "$(OUTDIR)\$(CAPQ)block.sys" "$(TOP)\driver\Free\$(CAPQ)block.dbg"


CLEAN :
	-@erase "$(INTDIR)\ftd_all.obj"
	-@erase "$(INTDIR)\ftd_bab.obj"
	-@erase "$(INTDIR)\ftd_bits.obj"
	-@erase "$(INTDIR)\ftd_ddi.obj"
	-@erase "$(INTDIR)\ftd_ioctl.obj"
	-@erase "$(INTDIR)\ftd_klog.obj"
	-@erase "$(INTDIR)\ftd_nt.obj"
	-@erase "$(INTDIR)\mapmem.obj"
	-@erase "$(INTDIR)\memset.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(OUTDIR)\$(CAPQ)block.map"
	-@erase "$(OUTDIR)\$(CAPQ)block.sys"
	-@erase "$(TOP)\driver\Free\$(CAPQ)block.dbg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90=df.exe
CPP=cl.exe
CPP_PROJ=/nologo /Gz /MLd /W3 /Oi /Gy /I "g:\Ddk\inc" /FI"warning.h" /D WIN32=100 /D "_WINDOWS" /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D _NT1X_=100 /D WINNT=1 /D _WIN32_WINNT=0x0400 /D WIN32_LEAN_AND_MEAN=1 /D DEVL=1 /D FPO=0 /D "NDEBUG" /D _DLL=1 /D _X86_=1 /D "UNICODE" /D "_CPU_X86_" /D "KERNEL" /D POOL_TAGGING=1 /Fp"$(INTDIR)\Ftd.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /Zel -cbstring /QIfdiv- /QIf /GF /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\driver.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=$(BaseDir)\lib\i386\free\int64.lib $(BaseDir)\lib\i386\free\ntoskrnl.lib $(BaseDir)\lib\i386\free\hal.lib /nologo /base:"0x10000" /version:4.0 /entry:"DriverEntry" /pdb:none /map:"$(INTDIR)\$(CAPQ)block.map" /debug /debugtype:both /machine:IX86 /nodefaultlib /out:"$(OUTDIR)\$(CAPQ)block.sys" /libpath:"$(BASEDIR)\lib\i386\checked" /driver /IGNORE:4001,4037,4039,4065,4078,4087,4089,4096 /MERGE:_PAGE=PAGE /MERGE:_TEXT=.text /SECTION:INIT,d /MERGE:.rdata=.text /FULLBUILD /RELEASE /FORCE:MULTIPLE /OPT:REF /OPTIDATA /align:0x20 /osversion:4.00 /subsystem:native 
LINK32_OBJS= \
	"$(INTDIR)\ftd_all.obj" \
	"$(INTDIR)\ftd_bab.obj" \
	"$(INTDIR)\ftd_bits.obj" \
	"$(INTDIR)\ftd_ddi.obj" \
	"$(INTDIR)\ftd_ioctl.obj" \
	"$(INTDIR)\ftd_klog.obj" \
	"$(INTDIR)\ftd_nt.obj" \
	"$(INTDIR)\mapmem.obj" \
	"$(INTDIR)\memset.obj" \
	"$(INTDIR)\version.obj"

"$(OUTDIR)\$(CAPQ)block.sys" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

InputPath=.\Free\$(CAPQ)block.sys
SOURCE="$(InputPath)"

"$(OUTDIR)\LRblock.dbg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	rebase -x .. -b 0x10000 $(TOP)\driver\Free\$(CAPQ)block.sys
<< 
	
SOURCE="$(InputPath)"
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\Free
# End Custom Macros

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\$(CAPQ)block.sys" "$(TOP)\driver\Free\$(CAPQ)block.dbg"
   md C:\TDMF\Rep_5_01_NT\ftdsrc\"Replication"
	copy Free\LRblock.sys C:\TDMF\Rep_5_01_NT\ftdsrc\"Replication"\LRblock.sys
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "driver - Win32 Debug"

OUTDIR=.\Checked
INTDIR=.\Checked
# Begin Custom Macros
OutDir=.\Checked
# End Custom Macros

ALL : "$(OUTDIR)\$(CAPQ)block.sys" "$(TOP)\driver\Checked\$(CAPQ)block.dbg"


CLEAN :
	-@erase "$(INTDIR)\ftd_all.obj"
	-@erase "$(INTDIR)\ftd_bab.obj"
	-@erase "$(INTDIR)\ftd_bits.obj"
	-@erase "$(INTDIR)\ftd_ddi.obj"
	-@erase "$(INTDIR)\ftd_ioctl.obj"
	-@erase "$(INTDIR)\ftd_klog.obj"
	-@erase "$(INTDIR)\ftd_nt.obj"
	-@erase "$(INTDIR)\mapmem.obj"
	-@erase "$(INTDIR)\memset.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(OUTDIR)\$(CAPQ)block.map"
	-@erase "$(OUTDIR)\$(CAPQ)block.sys"
	-@erase "$(TOP)\driver\Checked\$(CAPQ)block.dbg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90=df.exe
CPP=cl.exe
CPP_PROJ=/nologo /Gz /MLd /W3 /Z7 /Oi /Gy /FI"warning.h" /D DBG=1 /D WIN32=100 /D "_DEBUG" /D "_WINDOWS" /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D _NT1X_=100 /D WINNT=1 /D _WIN32_WINNT=0x0400 /D WIN32_LEAN_AND_MEAN=1 /D DEVL=1 /D FPO=0 /D "NDEBUG" /D _DLL=1 /D _X86_=1 /D "UNICODE" /D "_CPU_X86_" /D "KERNEL" /D POOL_TAGGING=1 /Fp"$(INTDIR)\Ftd.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /Zel -cbstring /QIfdiv- /QIf /GF /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\driver.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=$(BaseDir)\lib\i386\checked\osrddk.lib $(BaseDir)\lib\i386\checked\int64.lib $(BaseDir)\lib\i386\checked\ntoskrnl.lib $(BaseDir)\lib\i386\checked\hal.lib /nologo /base:"0x10000" /version:4.0 /entry:"DriverEntry" /pdb:none /map:"$(INTDIR)\$(CAPQ)block.map" /debug /debugtype:both /machine:IX86 /nodefaultlib /out:"$(OUTDIR)\$(CAPQ)block.sys" /libpath:"$(BASEDIR)\lib\i386\checked" /driver /debug:notmapped,FULL /IGNORE:4001,4037,4039,4065,4078,4087,4089,4096 /MERGE:_PAGE=PAGE /MERGE:_TEXT=.text /SECTION:INIT,d /MERGE:.rdata=.text /FULLBUILD /RELEASE /FORCE:MULTIPLE /OPT:REF /OPTIDATA /align:0x20 /osversion:4.00 /subsystem:native 
LINK32_OBJS= \
	"$(INTDIR)\ftd_all.obj" \
	"$(INTDIR)\ftd_bab.obj" \
	"$(INTDIR)\ftd_bits.obj" \
	"$(INTDIR)\ftd_ddi.obj" \
	"$(INTDIR)\ftd_ioctl.obj" \
	"$(INTDIR)\ftd_klog.obj" \
	"$(INTDIR)\ftd_nt.obj" \
	"$(INTDIR)\mapmem.obj" \
	"$(INTDIR)\memset.obj" \
	"$(INTDIR)\version.obj"

"$(OUTDIR)\$(CAPQ)block.sys" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

InputPath=.\Checked\$(CAPQ)block.sys
SOURCE="$(InputPath)"

"$(OUTDIR)\LRblock.dbg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	rebase -x .. -b 0x10000 $(TOP)\driver\Checked\$(CAPQ)block.sys
<< 
	

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("driver.dep")
!INCLUDE "driver.dep"
!ELSE 
!MESSAGE Warning: cannot find "driver.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "driver - Win32 Release" || "$(CFG)" == "driver - Win32 Debug"
SOURCE=.\ftd_all.c

"$(INTDIR)\ftd_all.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ftd_bab.c

"$(INTDIR)\ftd_bab.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ftd_bits.c

"$(INTDIR)\ftd_bits.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ftd_ddi.c

"$(INTDIR)\ftd_ddi.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ftd_ioctl.c

"$(INTDIR)\ftd_ioctl.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ftd_klog.c

"$(INTDIR)\ftd_klog.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ftd_nt.c

"$(INTDIR)\ftd_nt.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\mapmem.c

"$(INTDIR)\mapmem.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\memset.c

"$(INTDIR)\memset.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=..\version.c

"$(INTDIR)\version.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)



!ENDIF 

