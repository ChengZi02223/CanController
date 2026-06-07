@echo off
chcp 65001 >nul
echo ==============================================
echo     Qt + MinGW 项目一键编译脚本 (Debug)
echo ==============================================
echo.

:: --------------------------
:: 配置（和你的 CMakeLists.txt 对应）
:: --------------------------
set QT_DIR=C:\Qt\Qt5.12.8\5.12.8\msvc2017_64
set PATH=%QT_DIR%\bin;%PATH%
echo [信息] Qt 路径：%QT_DIR%
echo.

:: 1. 清理旧构建目录
if exist "build" (
    echo [1/4] 清理旧构建目录...
    rd /s /q build
)

:: 2. 创建构建目录
echo [2/4] 创建 build 目录...
mkdir build

:: 3. CMake 配置（MinGW 专用）
echo [3/4] CMake 配置 Debug 模式...
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug

if %errorlevel% neq 0 (
    echo.
    echo 错误：CMake 配置失败！
    pause
    exit /b 1
)

:: 4. 编译（MinGW Makefiles 不需要 --config）
echo.
echo [4/4] 编译 Debug 模式...
cmake --build .

if %errorlevel% neq 0 (
    echo.
    echo 错误：编译失败！
    pause
    exit /b 1
)

echo.
echo ==============================================
echo          Debug 编译完成！✅
echo  可执行文件在 build/ 目录下
echo ==============================================
echo.
pause