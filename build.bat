@echo off
REM fconvert Windows Build Script

setlocal enabledelayedexpansion

set "BUILD_TYPE=Release"
set "BUILD_DIR=build"

REM Parse arguments
:parse_args
if "%~1"=="" goto :done_args
if /i "%~1"=="debug" set "BUILD_TYPE=Debug"
if /i "%~1"=="release" set "BUILD_TYPE=Release"
if /i "%~1"=="clean" goto :clean
if /i "%~1"=="rebuild" goto :rebuild
shift
goto :parse_args
:done_args

REM Check for CMake
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: CMake not found. Please install CMake and add it to PATH.
    exit /b 1
)

REM Create build directory if it doesn't exist
if not exist "%BUILD_DIR%" (
    echo Creating build directory...
    mkdir "%BUILD_DIR%"
)

REM Configure with CMake
echo.
echo Configuring CMake (%BUILD_TYPE%)...
cmake -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if %errorlevel% neq 0 (
    echo ERROR: CMake configuration failed.
    exit /b 1
)

REM Build
echo.
echo Building fconvert...
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE%
if %errorlevel% neq 0 (
    echo ERROR: Build failed.
    exit /b 1
)

echo.
echo Build successful!
echo Binary: %BUILD_DIR%\%BUILD_TYPE%\fconvert.exe
exit /b 0

:clean
echo Cleaning build directory...
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
echo Clean complete.
exit /b 0

:rebuild
echo Rebuilding...
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
goto :done_args
