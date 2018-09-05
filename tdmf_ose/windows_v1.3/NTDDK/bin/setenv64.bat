@echo off

if "%1"=="/?" goto usage
if "%1"=="-?" goto usage
if "%1"=="\?" goto usage

if "%1"=="" goto usage

rem Check for new or updated install of MSVC
%1\bin\vccheck /p %1 /e /s

rem Set build environment from msvc

if NOT EXIST %1\bin\ddkvars.bat goto devdir
call %1\bin\ddkvars.bat
if "%MSVCDIR%"=="" set MSVCDIR=%MSDEVDIR%
goto testbasedir

:devdir
if "%MSDEVDIR%"=="" goto no_msvc40
call %MSDEVDIR%\bin\vcvars32 x86
if "%MSVCDIR%"=="" set MSVCDIR=%MSDEVDIR%

:testbasedir

set BASEDIR=%1
set Path=%BASEDIR%\bin;%path%
set Lib=%BASEDIR%\lib;%lib%
set Include=%BASEDIR%\inc;%include%

set NTMAKEENV=%BASEDIR%\bin
set BUILD_MAKE_PROGRAM=nmake.exe
set BUILD_DEFAULT=-ei -nmake -i


if "%tmp%"=="" set tmp=%TEMP%

if "%PROCESSOR_ARCHITECTURE%"=="ALPHA" goto alpha
if "%PROCESSOR_ARCHITECTURE%"=="x86" goto i386
set PROCESSOR_ARCHITECTURE=x86
goto i386

@rem goto cpuerror

:alpha

if "%Cpu%" == "" set Cpu=ALPHA
set BUILD_DEFAULT_TARGETS=-axp64
set HOST_TARGETCPU=alpha
set HOST_TARGET_DIRECTORY=axp64
set HOST_TARGET_DEFINES=AXP64=1 ALPHA=0 GENAXP64=1
set NTAXP64DEFAULT=1
set NTALPHADEFAULT=
set AXP64=1
set PATH=%BASEDIR%\bin\AXP64\;%PATH%
set AXP64_WARNING_LEVEL=%AXP64_WARNING_LEVEL% -Wp64 -W3 -Ap64 -D_M_AXP64

goto envtest

:i386

if "%Cpu%" == "" set Cpu=i386
set BUILD_DEFAULT_TARGETS=-IA64
set HOST_TARGETCPU=i386
set HOST_TARGET_DIRECTORY=IA64
set HOST_TARGET_DEFINES=IA64=1 i386=0 GENIA64=1
set IA64=1
set NTIA-64DEFAULT=1
set PATH=%BASEDIR%\bin\IA64\;%PATH%
set IA64_WARNING_LEVEL=%IA64_WARNING_LEVEL% -Wp64 -W3 -Zs -D_IA64_

:envtest

if "%DDKBUILDENV%"=="" set DDKBUILDENV=%2

if "%DDKBUILDENV%"=="" goto free
if "%DDKBUILDENV%"=="free" goto free
if "%DDKBUILDENV%"=="FREE" goto free
if "%DDKBUILDENV%"=="checked" goto checked
if "%DDKBUILDENV%"=="CHECKED" goto checked
goto usage

:free

rem set up an NT free build environment

set BUILD_ALT_DIR=fre
set NTDBGFILES=1
set NTDEBUG=
set NTDEBUGTYPE=windbg
set MSC_OPTIMIZATION=

goto done

:checked

rem set up an NT checked build environment

set BUILD_ALT_DIR=chk
set NTDBGFILES=
set NTDEBUG=ntsd
set NTDEBUGTYPE=windbg
set MSC_OPTIMIZATION=/Od /Oi

:done

set NEW_CRTS=1

set SDK_INC_PATH=%BASEDIR%\inc
set DDK_INC_PATH=%BASEDIR%\inc\ddk
set WDM_INC_PATH=%BASEDIR%\inc\ddk\wdm
set CRT_INC_PATH=%BASEDIR%\inc
set OAK_INC_PATH=%BASEDIR%\inc

set SDK_LIB_DEST=%BASEDIR%\lib%BUILD_ALT_DIR%
set DDK_LIB_DEST=%BASEDIR%\lib%BUILD_ALT_DIR%

set SDK_LIB_PATH=%BASEDIR%\lib%BUILD_ALT_DIR%\*
set DDK_LIB_PATH=%BASEDIR%\lib%BUILD_ALT_DIR%\*
set CRT_LIB_PATH=%BASEDIR%\lib%BUILD_ALT_DIR%\*

set COFFBASE_TXT_FILE=%BASEDIR%\bin\coffbase.txt
%1\bin\vccheck /p %1 /d

doskey /macrofile=%BASEDIR%\bin\generic.mac
doskey /macrofile=%BASEDIR%\bin\ddktree.mac

cd /d %BASEDIR%
goto end

:cpuerror

echo.
echo Error: PROCESSOR_ARCHITECTURE environment variable not recognized.
echo.
echo.

goto end

:no_msvc40
@echo Error: MSDEVDIR environment variable not recognized.
@echo        MS VC++ 5.0 or 6.0 must be installed.

goto end

:usage

echo.
echo usage: setenv64 ^<directory^> [free^|^checked]
echo.
echo   Example:  setenv64 d:\ddk checked     set checked environment
echo   Example:  setenv64 d:\ddk             defaults to free environment
echo.
echo.


:end
