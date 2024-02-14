@echo off

REM Set the path to the executable
set EXE_PATH=..\x64\Release\SelfTest-API.exe

echo This is a testing script to test the SelfTestAPI! Update

REM List of commands
set "commands=read_build_id read_controller_id read_bootloader_version test_booster test_piezos"

REM Loop through each command and execute
for %%i in (%commands%) do (
    echo Running command: %%i
    %EXE_PATH% %%i

    REM Check the exit code of the last command
    if %errorlevel% neq 0 (
        echo Error: Command %%i failed with exit code %errorlevel%
        exit /b %errorlevel%
    )
)

echo All commands executed successfully!
