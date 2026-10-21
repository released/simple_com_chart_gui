@echo off
setlocal
call "%~dp0..\build_release.bat"
exit /b %errorlevel%
