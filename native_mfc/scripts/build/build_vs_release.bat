@echo off
setlocal EnableDelayedExpansion
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "MSBUILD="
set "VSDEVCMD="
if exist "%VSWHERE%" (
  for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.Component.VC.Tools.x86.x64 -find Common7\Tools\VsDevCmd.bat`) do (
    set "VSDEVCMD=%%i"
  )
  if not defined VSDEVCMD (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -find Common7\Tools\VsDevCmd.bat`) do (
      set "VSDEVCMD=%%i"
    )
  )
  for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do (
    set "MSBUILD=%%i"
  )
  if not defined MSBUILD (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -find MSBuild\**\Bin\MSBuild.exe`) do (
      set "MSBUILD=%%i"
    )
  )
)
if defined VSDEVCMD (
  call "%VSDEVCMD%" -arch=x64 -host_arch=x64
 ) else (
  echo [ERROR] Visual Studio C++ tools not found.
  echo Please install "Desktop development with C++" and "MSVC v143".
  exit /b 1
)
if defined MSBUILD (
  if not exist "%MSBUILD%" set "MSBUILD="
)
if not defined MSBUILD (
  set "MSBUILD=msbuild"
)
if /i "%MSBUILD%"=="msbuild" (
  where msbuild >nul 2>nul
  if errorlevel 1 (
    echo [ERROR] MSBuild not found.
    echo Please install Visual Studio or Build Tools with MSBuild.
    exit /b 1
  )
) else (
  if not exist "%MSBUILD%" (
  echo [ERROR] MSBuild not found.
  echo Please install Visual Studio or Build Tools with MSBuild.
  exit /b 1
  )
)
set "ROOT=%~dp0..\.."
for %%i in ("%ROOT%") do set "ROOT=%%~fi"

set "OUTDIR=%ROOT%\build\vs\Release\"
set "INTDIR=%ROOT%\build\vs\obj\"
if not exist "%OUTDIR%" mkdir "%OUTDIR%" >nul 2>nul
if not exist "%INTDIR%" mkdir "%INTDIR%" >nul 2>nul

set "SLN=%ROOT%\vs\SimpleComChartMfc.sln"
if not exist "%SLN%" (
  set "SLN="
  for %%i in ("%ROOT%\build\cmake\*.sln") do set "SLN=%%~fi"
  if defined SLN (
    echo [INFO] VS solution not found. Using CMake-generated solution: !SLN!
  ) else (
    echo [ERROR] VS solution not found. Run scripts\build\build_cmake_release.bat first.
    exit /b 1
  )
)

"%MSBUILD%" "%SLN%" /t:Build ^
  /p:Configuration=Release ^
  /p:Platform=x64 ^
  /p:OutDir=%OUTDIR% ^
  /p:BaseIntermediateOutputPath=%INTDIR% ^
  /p:IntermediateOutputPath=%INTDIR%
if errorlevel 1 (
  echo [ERROR] Build failed. Check installed Windows SDK and MSVC toolset.
  exit /b 1
)
endlocal
