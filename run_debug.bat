@echo off
chcp 65001 >nul
title Qt Debug 启动

set EXE_PATH=build\CanController.exe
if not exist "%EXE_PATH%" (
    echo 错误：未找到 %EXE_PATH%
    echo 请先执行 build.bat 编译
    pause
    exit /b 1
)
start "" "%EXE_PATH%"
echo 正在启动 Debug 程序：%EXE_PATH%
