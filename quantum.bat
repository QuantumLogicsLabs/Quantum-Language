@echo off
setlocal

rem ── quantum.bat ───────────────────────────────────────────────────────────────
rem  Launcher wrapper for quantum.exe (compiler + bundler).
rem  Priority: root directory first, then build subdirectories.
rem  The root copy is always the freshest — build.bat copies it there last.

set QUANTUM_EXE=

if exist "%~dp0quantum.exe" (
    set QUANTUM_EXE=%~dp0quantum.exe
    goto :run
)
if exist "%~dp0build\quantum.exe" (
    set QUANTUM_EXE=%~dp0build\quantum.exe
    goto :run
)
if exist "%~dp0build\Release\quantum.exe" (
    set QUANTUM_EXE=%~dp0build\Release\quantum.exe
    goto :run
)
if exist "%~dp0build\Debug\quantum.exe" (
    set QUANTUM_EXE=%~dp0build\Debug\quantum.exe
    goto :run
)

echo [Error] quantum.exe not found. Run build.bat first.
exit /b 1

:run
"%QUANTUM_EXE%" %*
exit /b %errorlevel%