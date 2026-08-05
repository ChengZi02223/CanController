@echo off
chcp 65001 >nul
set "SCRIPT_ROOT=%~dp0"
cd /d "%SCRIPT_ROOT%"

echo ==============================================
echo     Qt5.12.8 + MinGW73_64 Build Script (Debug)
echo ==============================================
echo.

:: Config area
set "QT_BASE=C:\Qt\Qt5.12.8\5.12.8\mingw73_64"
set "MINGW_TOOL=C:\Qt\Tools\mingw730_64\bin"
set "CMAKE_TOOL=C:\Qt\Tools\CMake_64\bin"

:: Set environment PATH
set "PATH=%QT_BASE%\bin;%MINGW_TOOL%;%CMAKE_TOOL%;%PATH%"
echo [INFO] Qt path: %QT_BASE%
echo [INFO] MinGW toolchain path: %MINGW_TOOL%
echo.

:: 1. Clean old build dir
@REM if exist "build" (
@REM     echo [1/4] Cleaning old build directory...
@REM     rd /s /q build
@REM )
:: 2. Create new build dir
@REM echo [2/4] Creating build directory...
@REM mkdir build
cd build

:: 3. CMake configure for Debug
echo [3/4] Running CMake configure (Debug)...
cmake .. ^
-G "MinGW Makefiles" ^
-DCMAKE_BUILD_TYPE=Debug ^
-DCMAKE_PREFIX_PATH=%QT_BASE%

if %errorlevel% neq 0 (
    echo.
    echo ERROR: CMake configure failed! Check Qt/MinGW paths
    exit /b 1
)

:: 4. Build project
echo.
echo [4/4] Starting build (Debug)...
cmake --build .
if %errorlevel% neq 0 (
    echo.
    echo ERROR: Source build failed, check source code and CMakeLists.txt

    exit /b 1
)

echo.
echo ==============================================
echo          Build completed successfully!
echo  Output binary path: %SCRIPT_ROOT%build
echo ==============================================