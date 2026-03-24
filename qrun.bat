@echo off
setlocal

rem ── qrun.bat ──────────────────────────────────────────────────────────────────
rem  Launcher for the qrun interpreter.
rem  Interprets a .sa script directly — no .exe is generated.
rem  Priority: root directory first, then build subdirectories.
rem  The root copy is always the freshest — build.bat copies it there last.

set QRUN_EXE=

if exist "%~dp0qrun.exe" (
    set QRUN_EXE=%~dp0qrun.exe
    goto :run
)
if exist "%~dp0build\qrun.exe" (
    set QRUN_EXE=%~dp0build\qrun.exe
    goto :run
)
if exist "%~dp0build\Release\qrun.exe" (
    set QRUN_EXE=%~dp0build\Release\qrun.exe
    goto :run
)
if exist "%~dp0build\Debug\qrun.exe" (
    set QRUN_EXE=%~dp0build\Debug\qrun.exe
    goto :run
)

echo [Error] qrun.exe not found. Run build.bat first.
exit /b 1

:run
"%QRUN_EXE%" %*
exit /b %errorlevel%