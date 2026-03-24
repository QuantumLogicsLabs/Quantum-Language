@echo off
setlocal

rem ── qrun.bat ──────────────────────────────────────────────────────────────────
rem  Launcher for the qrun interpreter.
rem  Interprets a .sa script directly — no .exe is generated.
rem
rem  Usage:  qrun hello.sa
rem
rem  Priority order for qrun.exe:
rem    1. build\Release\qrun.exe   (MSVC Release)
rem    2. build\Debug\qrun.exe     (MSVC Debug)
rem    3. build\qrun.exe           (GCC / Ninja / single-config)
rem    4. qrun.exe                 (same directory as this .bat)

set QRUN_EXE=

if exist "%~dp0build\Release\qrun.exe" (
    set QRUN_EXE=%~dp0build\Release\qrun.exe
    goto :run
)
if exist "%~dp0build\Debug\qrun.exe" (
    set QRUN_EXE=%~dp0build\Debug\qrun.exe
    goto :run
)
if exist "%~dp0build\qrun.exe" (
    set QRUN_EXE=%~dp0build\qrun.exe
    goto :run
)
if exist "%~dp0qrun.exe" (
    set QRUN_EXE=%~dp0qrun.exe
    goto :run
)

echo [Error] qrun.exe not found. Run build.bat first.
exit /b 1

:run
"%QRUN_EXE%" %*
exit /b %errorlevel%