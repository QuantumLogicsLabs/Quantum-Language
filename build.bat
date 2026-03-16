@echo off
setlocal EnableDelayedExpansion

echo.
echo   Building Quantum Language v2.0.0  ^|  Bytecode VM
echo.

rem ── Add MSYS2 ucrt64 to PATH for this session ─────────────────────────────────
set "PATH=C:\msys64\ucrt64\bin;C:\msys64\mingw64\bin;C:\msys64\usr\bin;C:\MinGW\bin;C:\MinGW64\bin;C:\TDM-GCC-64\bin;%PATH%"

rem ── Locate make ───────────────────────────────────────────────────────────────
set "MAKE_EXE="
where mingw32-make >nul 2>&1 && set "MAKE_EXE=mingw32-make" && goto :make_found
where make         >nul 2>&1 && set "MAKE_EXE=make"         && goto :make_found

echo.
echo   [ERROR] mingw32-make still not found after searching common paths.
echo.
echo   Please run this in PowerShell (no admin needed) then reopen terminal:
echo.
echo   [System.Environment]::SetEnvironmentVariable("PATH","C:\msys64\ucrt64\bin;$env:PATH","User")
echo.
pause
exit /b 1

:make_found
echo   Using: !MAKE_EXE!

rem ── Locate cmake ──────────────────────────────────────────────────────────────
set "CMAKE_EXE=cmake"
where cmake >nul 2>&1 || (
    if exist "C:\Program Files\CMake\bin\cmake.exe" set "CMAKE_EXE=C:\Program Files\CMake\bin\cmake.exe"
)

rem ── Configure + build ─────────────────────────────────────────────────────────
if not exist build mkdir build
cd build

"%CMAKE_EXE%" .. -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles" -DCMAKE_MAKE_PROGRAM="!MAKE_EXE!" > ..\build_log.txt 2>&1
if errorlevel 1 (
    cd ..
    echo   [ERROR] CMake configure failed:
    type build_log.txt
    pause
    exit /b 1
)

"!MAKE_EXE!" 2> ..\build_errors.txt
if errorlevel 1 (
    cd ..
    echo.
    echo   [ERROR] Compile failed:
    type build_errors.txt
    pause
    exit /b 1
)

cd ..
echo.
echo   Build successful!  -^>  build\quantum.exe
echo.
echo   quantum hello.sa
echo   quantum --test examples
echo   quantum --debug hello.sa
echo.
endlocal