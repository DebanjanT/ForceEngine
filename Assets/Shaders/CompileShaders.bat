@echo off
REM Compile GLSL shaders to SPIR-V using glslangValidator or glslc
REM Requires Vulkan SDK to be installed and in PATH

setlocal enabledelayedexpansion

REM Try to find glslc (preferred) or glslangValidator
where glslc >nul 2>&1
if %ERRORLEVEL% == 0 (
    set COMPILER=glslc
    echo Using glslc compiler
) else (
    where glslangValidator >nul 2>&1
    if %ERRORLEVEL% == 0 (
        set COMPILER=glslangValidator
        echo Using glslangValidator compiler
    ) else (
        echo ERROR: Could not find glslc or glslangValidator
        echo Please install the Vulkan SDK and ensure it is in your PATH
        exit /b 1
    )
)

REM Create output directory
if not exist "Compiled" mkdir Compiled

echo.
echo Compiling shaders...
echo.

REM Compile each shader
for %%f in (*.vert *.frag *.comp *.geom *.tesc *.tese) do (
    if exist "%%f" (
        echo Compiling %%f...
        if "!COMPILER!"=="glslc" (
            glslc -o "Compiled/%%~nf%%~xf.spv" "%%f"
        ) else (
            glslangValidator -V -o "Compiled/%%~nf%%~xf.spv" "%%f"
        )
        if !ERRORLEVEL! NEQ 0 (
            echo ERROR: Failed to compile %%f
            exit /b 1
        )
    )
)

echo.
echo All shaders compiled successfully!
echo Output directory: Compiled/
echo.

REM List compiled shaders
echo Compiled shaders:
dir /b Compiled\*.spv 2>nul

endlocal
