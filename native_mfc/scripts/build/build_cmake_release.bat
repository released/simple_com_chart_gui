@echo off
setlocal
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "CMAKE_EXE="
if exist "%VSWHERE%" (
  for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.Component.VC.Tools.x86.x64 -find Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`) do (
    set "CMAKE_EXE=%%i"
  )
  if not defined CMAKE_EXE (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -find Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`) do (
      set "CMAKE_EXE=%%i"
    )
  )
)
if defined CMAKE_EXE (
  if not exist "%CMAKE_EXE%" set "CMAKE_EXE="
)
if not defined CMAKE_EXE (
  set "CMAKE_EXE=cmake"
)
if /i "%CMAKE_EXE%"=="cmake" (
  where cmake >nul 2>nul
  if errorlevel 1 (
    echo [ERROR] CMake not found.
    echo Please install Visual Studio component: "CMake tools for Windows".
    echo Or install CMake and add it to PATH.
    exit /b 1
  )
) else (
  if not exist "%CMAKE_EXE%" (
  echo [ERROR] CMake not found.
  echo Please install Visual Studio component: "CMake tools for Windows".
  echo Or install CMake and add it to PATH.
  exit /b 1
  )
)
"%CMAKE_EXE%" -S "%~dp0..\.." -B "%~dp0..\..\build\cmake" -G "Visual Studio 17 2022"
if errorlevel 1 (
  echo [ERROR] CMake configure failed. Check Visual Studio C++ workload and Windows SDK.
  exit /b 1
)
"%CMAKE_EXE%" --build "%~dp0..\..\build\cmake" --config Release
if errorlevel 1 (
  echo [ERROR] Build failed. Ensure "Desktop development with C++" and "MSVC v143" are installed.
  exit /b 1
)
endlocal
