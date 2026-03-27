@echo off
REM =============================================================
REM docs\scripts\dev.bat — Start GestureMouse in development mode (Windows)
REM
REM Usage:
REM   dev.bat            — build Debug + run
REM   dev.bat --build    — only build
REM   dev.bat --test     — build + run tests
REM   dev.bat --release  — build Release
REM =============================================================

setlocal EnableDelayedExpansion

set BUILD_TYPE=Debug
set RUN_APP=1
set RUN_TESTS=0
set BUILD_DIR=build

for %%A in (%*) do (
    if "%%A"=="--build"   set RUN_APP=0
    if "%%A"=="--test"    set RUN_TESTS=1 & set RUN_APP=0
    if "%%A"=="--release" set BUILD_TYPE=Release
)

echo.
echo  ===========================================
echo   GestureMouse -- Development Mode (Windows)
echo  ===========================================
echo.

REM ── Check cmake ───────────────────────────────────────────
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo  ERROR: cmake not found. Install from https://cmake.org/
    exit /b 1
)
echo  [OK] cmake found

REM ── Configure ─────────────────────────────────────────────
echo.
echo  [1/3] Configuring (%BUILD_TYPE%)...
cmake -G "Visual Studio 16 2019" -A x64 ^
      -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ^
      -B %BUILD_DIR% ^
      -Wno-dev
if %errorlevel% neq 0 ( echo  ERROR: cmake configure failed & exit /b 1 )
echo  [OK] Configured

REM ── Build ─────────────────────────────────────────────────
echo.
echo  [2/3] Building...
cmake --build %BUILD_DIR% --config %BUILD_TYPE%
if %errorlevel% neq 0 ( echo  ERROR: Build failed & exit /b 1 )
echo  [OK] Build successful

REM ── Tests ─────────────────────────────────────────────────
if %RUN_TESTS%==1 (
    echo.
    echo  [3/3] Running tests...
    cmake --build %BUILD_DIR% --config %BUILD_TYPE% --target gesture_mouse_tests
    cd %BUILD_DIR%
    ctest --output-on-failure -C %BUILD_TYPE%
    cd ..
    goto :done
)

REM ── Run ───────────────────────────────────────────────────
if %RUN_APP%==1 (
    echo.
    echo  [3/3] Starting GestureMouse...
    echo  Tip: Close the console window to stop
    echo.
    %BUILD_DIR%\%BUILD_TYPE%\gesture_mouse.exe
)

:done
endlocal
