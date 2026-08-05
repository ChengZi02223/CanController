@echo off
@REM 进入项目源码根目录
cd /d D:\Desktop\yc\PartTimeJobs\Windows\CanController

@REM 删除旧构建目录
if exist .\build (
    rmdir /s /q .\build
)
if exist .\build (
    echo ERROR: build目录删除失败！请关闭占用该目录的程序/资源管理器！
    pause
    exit /b 1
)

@REM cmake Release
cmake -B .\build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

@REM 编译
cmake --build .\build