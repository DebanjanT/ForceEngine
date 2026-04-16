@echo off
setlocal

cd /d %~dp0..

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Debug"

set "EXE_PATH=Build\bin\%CONFIG%\ForceEditor.exe"

if not exist "%EXE_PATH%" (
    echo.
    echo [ERROR] ForceEditor.exe not found at: %EXE_PATH%
    echo.
    echo Please build the project first:
    echo   Scripts\Build.bat %CONFIG%
    echo.
    pause
    exit /b 1
)

echo.
echo Starting Force Editor (%CONFIG%)...
echo.

"%EXE_PATH%"
