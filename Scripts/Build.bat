@echo off
setlocal

cd /d %~dp0..

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Debug"

echo.
echo ========================================================
echo          Force Engine Build Script
echo ========================================================
echo.
echo Configuration: %CONFIG%
echo.

:: Check if Build directory exists
if not exist "Build" (
    echo [ERROR] Build directory not found. Run Setup.bat first.
    pause
    exit /b 1
)

:: Check if solution exists
if not exist "Build\ForceEngine.sln" (
    echo [ERROR] Solution not found. Run Setup.bat first.
    pause
    exit /b 1
)

echo Building Force Engine...
echo.

cmake --build Build --config %CONFIG% --parallel

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================================
echo                  BUILD SUCCESSFUL
echo ========================================================
echo.
echo Output: Build\bin\%CONFIG%\
echo.
echo To run the editor:
echo   Build\bin\%CONFIG%\ForceEditor.exe
echo.
pause
