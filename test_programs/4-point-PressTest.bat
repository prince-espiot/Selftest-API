@echo off

set EXE_PATH=..\x64\Release\SelfTest-API.exe

echo Running command sequence

REM Run the first two change_parameters commands
echo Running command: %EXE_PATH% change_parameters 7 29
%EXE_PATH% change_parameters 7 25
echo Running command: %EXE_PATH% change_parameters 8 51
%EXE_PATH% change_parameters 8 60
echo Running command: %EXE_PATH% run_afpt
%EXE_PATH% run_afpt

REM Continue with the rest of the commands
for %%i in ("8 17", "7 86", "8 51") do (
    echo Running command: %EXE_PATH% change_parameters %%i
    %EXE_PATH% change_parameters %%i
    echo Running command: %EXE_PATH% run_afpt
    %EXE_PATH% run_afpt
)

echo Command sequence complete
