@echo off
setlocal

rem ── quantum.bat ───────────────────────────────────────────────────────────────
rem  Launcher wrapper — finds quantum.exe in the standard CMake output locations
rem  and forwards all arguments unchanged.
rem
rem  Priority:
rem    1. build\Release\quantum.exe   (MSVC Release)
rem    2. build\Debug\quantum.exe     (MSVC Debug)
rem    3. build\quantum.exe           (GCC / Ninja / single-config)
rem    4. quantum.exe                 (same directory as this .bat)

set QUANTUM_EXE=

if exist "%~dp0build\Release\quantum.exe" (
    set QUANTUM_EXE=%~dp0build\Release\quantum.exe
    goto :run
)
if exist "%~dp0build\Debug\quantum.exe" (
    set QUANTUM_EXE=%~dp0build\Debug\quantum.exe
    goto :run
)
if exist "%~dp0build\quantum.exe" (
    set QUANTUM_EXE=%~dp0build\quantum.exe
    goto :run
)
if exist "%~dp0quantum.exe" (
    set QUANTUM_EXE=%~dp0quantum.exe
    goto :run
)

echo [Error] quantum.exe not found. Run build.bat first.
exit /b 1

:run
"%QUANTUM_EXE%" %*
exit /b %errorlevel%