@echo off
setlocal
if /I "%~1"=="/?" goto usage
if /I "%~1"=="-h" goto usage
if /I "%~1"=="--help" goto usage
if not "%~1"=="" (
  echo Unsupported clean argument.
  exit /b 1
)
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0clean_build.ps1"
exit /b %errorlevel%

:usage
echo Usage: %~nx0
echo Removes verified compiler/CMake/test outputs; preserves delivery/data/evidence/unknown files.
exit /b 0
