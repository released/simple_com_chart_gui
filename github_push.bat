@echo off
setlocal
pushd "%~dp0" || exit /b 1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\push_github.ps1" %*
set "PUBLISH_EXIT=%ERRORLEVEL%"
popd
if not "%PUBLISH_EXIT%"=="0" echo GitHub update stopped. Exit code: %PUBLISH_EXIT%
exit /b %PUBLISH_EXIT%
