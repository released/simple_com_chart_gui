@echo off
pushd "%~dp0.." || exit /b 1
python main.pyw
set "TOOL_EXIT=%errorlevel%"
popd
exit /b %TOOL_EXIT%
