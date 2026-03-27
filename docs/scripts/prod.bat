@echo off
REM =============================================================
REM docs\scripts\prod.bat — Production management (Windows)
REM
REM Usage:
REM   prod.bat install   — build Release + install
REM   prod.bat start     — start GestureMouse
REM   prod.bat stop      — stop GestureMouse
REM   prod.bat status    — show status
REM =============================================================

setlocal EnableDelayedExpansion

set BINARY_NAME=gesture_mouse.exe
set INSTALL_DIR=C:\Program Files\GestureMouse
set BUILD_DIR=build
set COMMAND=%1

if "%COMMAND%"=="" (
    echo Usage: prod.bat [install^|start^|stop^|status]
    exit /b 1
)

if "%COMMAND%"=="install" goto :install
if "%COMMAND%"=="start"   goto :start
if "%COMMAND%"=="stop"    goto :stop
if "%COMMAND%"=="status"  goto :status
echo Unknown command: %COMMAND%
exit /b 1

:install
echo Building Release...
cmake -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Release -B %BUILD_DIR%
cmake --build %BUILD_DIR% --config Release
if %errorlevel% neq 0 ( echo Build FAILED & exit /b 1 )

echo Installing to %INSTALL_DIR%...
if not exist "%INSTALL_DIR%" mkdir "%INSTALL_DIR%"
copy /Y "%BUILD_DIR%\Release\%BINARY_NAME%" "%INSTALL_DIR%\%BINARY_NAME%"
copy /Y "assets\default_config.txt" "%INSTALL_DIR%\settings.txt"

echo Installation complete.
"%INSTALL_DIR%\%BINARY_NAME%" --version
goto :eof

:start
echo Starting GestureMouse (production)...
tasklist /fi "imagename eq %BINARY_NAME%" 2>NUL | find /I "%BINARY_NAME%" >NUL
if %errorlevel%==0 (
    echo GestureMouse is already running.
    goto :eof
)
start "" "%INSTALL_DIR%\%BINARY_NAME%"
timeout /t 2 /nobreak >NUL
echo GestureMouse started.
goto :eof

:stop
echo Stopping GestureMouse...
taskkill /F /IM %BINARY_NAME% >NUL 2>&1
if %errorlevel%==0 ( echo GestureMouse stopped. ) else ( echo GestureMouse was not running. )
goto :eof

:status
echo GestureMouse Status:
echo ───────────────────
tasklist /fi "imagename eq %BINARY_NAME%" 2>NUL | find /I "%BINARY_NAME%" >NUL
if %errorlevel%==0 (
    echo   Status: RUNNING
    for /f "tokens=2" %%i in ('tasklist /fi "imagename eq %BINARY_NAME%" ^| find /I "%BINARY_NAME%"') do echo   PID: %%i
) else (
    echo   Status: STOPPED
)
echo   Install dir: %INSTALL_DIR%
endlocal
