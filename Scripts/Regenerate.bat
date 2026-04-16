@echo off
setlocal

cd /d %~dp0..

echo.
echo ========================================================
echo       Force Engine Regenerate CMake Script
echo ========================================================
echo.

if not exist "Build" mkdir Build

cd Build

echo Regenerating CMake project...
echo.

:: Detect Visual Studio version
set "VS_FOUND=0"
if exist "%ProgramFiles%\Microsoft Visual Studio\2022" set "VS_FOUND=2022"
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022" set "VS_FOUND=2022"

if "%VS_FOUND%"=="2022" (
    cmake .. -G "Visual Studio 17 2022" -A x64
) else (
    cmake .. -G "Visual Studio 16 2019" -A x64
)

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] CMake generation failed
    pause
    exit /b 1
)

echo.
echo ========================================================
echo              REGENERATION SUCCESSFUL
echo ========================================================
echo.
pause
