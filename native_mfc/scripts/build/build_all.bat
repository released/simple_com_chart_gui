@echo off
setlocal

call "%~dp0build_cmake_release.bat"
if errorlevel 1 exit /b 1

call "%~dp0build_vs_release.bat"
if errorlevel 1 exit /b 1

endlocal
