@echo off
chcp 65001 >nul
title Qt Debug Startup

set EXE_PATH=build\CanController.exe
if not exist "%EXE_PATH%" (
    echo Error: NNot found %EXE_PATH%
    echo Exec build.bat to build the project first.
    pause
    exit /b 1
)
start "" "%EXE_PATH%"
echo Starting Debug program: %EXE_PATH%
