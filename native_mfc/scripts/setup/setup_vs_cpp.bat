@echo off
setlocal
net session >nul 2>nul
if errorlevel 1 (
  powershell -NoProfile -ExecutionPolicy Bypass -Command "Start-Process -FilePath \"%~f0\" -Verb RunAs"
  exit /b 0
)
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0setup_vs_cpp.ps1"
pause
endlocal
