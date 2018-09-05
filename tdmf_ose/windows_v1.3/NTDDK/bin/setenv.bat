@echo off

if "%1"=="/?" goto usage
if "%1"=="-?" goto usage
if "%1"=="\?" goto usage

if "%1"=="" goto usage

rem Check for new or updated install of MSVC
%1\bin\vccheck /p %1 /e

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
set BUILD_DEFAULT_TARGETS=-alpha
set ALPHA=1
set JENSEN=1
set NTALPHADEFAULT=1

goto envtest

:i386

if "%Cpu%" == "" set Cpu=i386
set BUILD_DEFAULT_TARGETS=-386

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
set NTDEBUG=ntsdnodbg
set NTDEBUGTYPE=windbg
set MSC_OPTIMIZATION=

goto done

:checked

rem set up an NT checked build environment

set BUILD_ALT_DIR=chk
set NTDBGFILES=1
set NTDEBUG=ntsd
set NTDEBUGTYPE=windbg
set MSC_OPTIMIZATION=

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

if "%OS%" == "Windows_NT" goto WinNT
if not "%OS%" == "Windows_NT" goto Win9x
goto end

:WinNT
cd /d %BASEDIR%
doskey /macrofile=%BASEDIR%\bin\generic.mac
doskey /macrofile=%BASEDIR%\bin\ddktree.mac

goto end

:Win9x
doskey /echo:off /bufsize:6144 /file:%BASEDIR%\bin\ddktree.mac
doskey /echo:off /file:%BASEDIR%\bin\generic.mac

%BASEDIR%\bin\MkCDir %BASEDIR%
if exist %BASEDIR%\bin\ChngeDir.bat call %BASEDIR%\bin\ChngeDir.bat
goto end

:cpuerror

echo.
echo Error: PROCESSOR_ARCHITECTURE environment variable not recognized.
echo.
echo.

goto end

:no_msvc40
@echo Error: MSDEVDIR environment variable not recognized.
@echo        MS Visual C++ 5.0 or 6.0 must be installed.

goto end

:usage

echo.
echo usage: setenv ^<directory^> [free^|^checked]
echo.
echo   Example:  setenv d:\ddk checked     set checked environment
echo   Example:  setenv d:\ddk             defaults to free environment
echo.
echo.


:end
