@echo off
@REM 进入项目源码根目录
cd /d D:\Desktop\yc\PartTimeJobs\Windows\CanController

@REM 删除旧构建目录
if exist .\build (
    rmdir /s /q .\build
)
@REM if exist .\build (
@REM     echo ERROR: build Faild
@REM     pause
@REM     exit /b 1
@REM )

@REM cmake Release
cmake -B .\build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

@REM 编译
cmake --build .\build