@echo off

cd /d %~dp0..

echo.
echo ========================================================
echo          Force Engine Clean Script
echo ========================================================
echo.

echo Cleaning build directory...

if exist "Build" (
    rmdir /s /q "Build"
    echo   [OK] Build directory removed
) else (
    echo   [SKIP] Build directory does not exist
)

echo.
echo Clean complete!
echo.
echo Run Setup.bat to regenerate the project.
echo.
pause
