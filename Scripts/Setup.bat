@echo off
setlocal enabledelayedexpansion

echo.
echo ========================================================
echo              Force Engine Setup Script
echo ========================================================
echo.

cd /d %~dp0..
set "ROOT_DIR=%CD%"

:: ========================================
:: Check Prerequisites
:: ========================================

echo [1/5] Checking prerequisites...
echo.

:: Check Git
where git >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Git is not installed or not in PATH
    echo.
    echo Please install Git from: https://git-scm.com/download/win
    echo.
    pause
    exit /b 1
)
echo   [OK] Git found

:: Check CMake
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake is not installed or not in PATH
    echo.
    echo Please install CMake from: https://cmake.org/download/
    echo Make sure to add CMake to PATH during installation.
    echo.
    pause
    exit /b 1
)
echo   [OK] CMake found

:: Check Vulkan SDK
if not defined VULKAN_SDK (
    echo [ERROR] Vulkan SDK is not installed or VULKAN_SDK environment variable is not set
    echo.
    echo Please install Vulkan SDK from: https://vulkan.lunarg.com/sdk/home
    echo After installation, restart your terminal or computer.
    echo.
    pause
    exit /b 1
)
echo   [OK] Vulkan SDK found at: %VULKAN_SDK%

:: Check for Visual Studio (optional but recommended)
set "VS_FOUND=0"
if exist "%ProgramFiles%\Microsoft Visual Studio\2022" set "VS_FOUND=1"
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022" set "VS_FOUND=1"
if %VS_FOUND%==1 (
    echo   [OK] Visual Studio 2022 found
) else (
    echo   [WARN] Visual Studio 2022 not found - will try to use available generator
)

echo.

:: ========================================
:: Clone Dependencies
:: ========================================

echo [2/5] Cloning external dependencies...
echo.

if not exist "External" mkdir External

:: GLFW
if not exist "External\glfw\.git" (
    echo   Cloning GLFW...
    if exist "External\glfw" rmdir /s /q "External\glfw"
    git clone --depth 1 https://github.com/glfw/glfw.git External/glfw
    if %ERRORLEVEL% NEQ 0 (
        echo [ERROR] Failed to clone GLFW
        pause
        exit /b 1
    )
) else (
    echo   [SKIP] GLFW already exists
)

:: GLM
if not exist "External\glm\.git" (
    echo   Cloning GLM...
    if exist "External\glm" rmdir /s /q "External\glm"
    git clone --depth 1 https://github.com/g-truc/glm.git External/glm
    if %ERRORLEVEL% NEQ 0 (
        echo [ERROR] Failed to clone GLM
        pause
        exit /b 1
    )
) else (
    echo   [SKIP] GLM already exists
)

:: spdlog
if not exist "External\spdlog\.git" (
    echo   Cloning spdlog...
    if exist "External\spdlog" rmdir /s /q "External\spdlog"
    git clone --depth 1 https://github.com/gabime/spdlog.git External/spdlog
    if %ERRORLEVEL% NEQ 0 (
        echo [ERROR] Failed to clone spdlog
        pause
        exit /b 1
    )
) else (
    echo   [SKIP] spdlog already exists
)

:: VMA
if not exist "External\VulkanMemoryAllocator\.git" (
    echo   Cloning Vulkan Memory Allocator...
    if exist "External\VulkanMemoryAllocator" rmdir /s /q "External\VulkanMemoryAllocator"
    git clone --depth 1 https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git External/VulkanMemoryAllocator
    if %ERRORLEVEL% NEQ 0 (
        echo [ERROR] Failed to clone VMA
        pause
        exit /b 1
    )
) else (
    echo   [SKIP] VMA already exists
)

:: Tracy
if not exist "External\tracy\.git" (
    echo   Cloning Tracy Profiler...
    if exist "External\tracy" rmdir /s /q "External\tracy"
    git clone --depth 1 https://github.com/wolfpld/tracy.git External/tracy
    if %ERRORLEVEL% NEQ 0 (
        echo [ERROR] Failed to clone Tracy
        pause
        exit /b 1
    )
) else (
    echo   [SKIP] Tracy already exists
)

:: EnTT
if not exist "External\entt\.git" (
    echo   Cloning EnTT...
    if exist "External\entt" rmdir /s /q "External\entt"
    git clone --depth 1 https://github.com/skypjack/entt.git External/entt
    if %ERRORLEVEL% NEQ 0 (
        echo [ERROR] Failed to clone EnTT
        pause
        exit /b 1
    )
) else (
    echo   [SKIP] EnTT already exists
)

echo.

:: ========================================
:: Generate Build Files
:: ========================================

echo [3/5] Creating build directory...
if not exist "Build" mkdir Build
echo.

echo [4/5] Generating CMake project...
echo.

cd Build

:: Try VS 2022 first, then 2019
if %VS_FOUND%==1 (
    cmake .. -G "Visual Studio 17 2022" -A x64
) else (
    cmake .. -G "Visual Studio 16 2019" -A x64
)

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] CMake generation failed
    echo.
    pause
    exit /b 1
)

cd ..

echo.

:: ========================================
:: Done
:: ========================================

echo [5/5] Setup complete!
echo.
echo ========================================================
echo                    SETUP SUCCESSFUL
echo ========================================================
echo.
echo Project location: %ROOT_DIR%
echo Solution file:    %ROOT_DIR%\Build\ForceEngine.sln
echo.
echo Next steps:
echo   1. Open Build\ForceEngine.sln in Visual Studio
echo   2. Set ForceEditor as startup project
echo   3. Build and run (F5)
echo.
echo Or build from command line:
echo   Scripts\Build.bat Debug
echo   Scripts\Build.bat Release
echo.
echo ========================================================
echo.
pause
