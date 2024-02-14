@echo off

REM Set the path to the executable
set EXE_PATH=..\x64\Release\SelfTest-API.exe

echo Running command sequence

REM Command sequence
set "commands=run_afpt"

REM Output file
set "output_file=rawtest.txt"

REM Iterate through the commands
for %%i in (%commands%) do (
    echo Running command: %EXE_PATH% %%i
    %EXE_PATH% %%i >> %output_file%
)

echo Command sequence complete
