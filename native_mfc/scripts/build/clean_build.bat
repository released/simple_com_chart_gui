@echo off
setlocal

set "ROOT=%~dp0..\.."
set "BUILD_DIR=%ROOT%\build"

call :clean_keep_release "%BUILD_DIR%\cmake"
call :clean_keep_release "%BUILD_DIR%\vs"

if exist "%BUILD_DIR%" (
  for /f %%A in ('dir /b "%BUILD_DIR%" 2^>nul') do (
    goto :done
  )
  rmdir "%BUILD_DIR%" >nul 2>nul
)

:done
endlocal
exit /b 0

:clean_keep_release
set "TARGET=%~1"
if not exist "%TARGET%" exit /b 0

if exist "%TARGET%\Release" (
  for /f "delims=" %%F in ('dir /b /a "%TARGET%" 2^>nul') do (
    if /i not "%%F"=="Release" (
      if exist "%TARGET%\%%F\" (
        rmdir /s /q "%TARGET%\%%F"
      ) else (
        del /q "%TARGET%\%%F" >nul 2>nul
      )
    )
  )
  for /f "delims=" %%F in ('dir /b /a "%TARGET%\Release" 2^>nul') do (
    if /i not "%%F"=="simple_com_chart_gui_mfc.exe" (
      if exist "%TARGET%\Release\%%F\" (
        rmdir /s /q "%TARGET%\Release\%%F"
      ) else (
        del /q "%TARGET%\Release\%%F" >nul 2>nul
      )
    )
  )
) else (
  rmdir /s /q "%TARGET%"
)
exit /b 0
