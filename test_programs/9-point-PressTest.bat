@echo off

REM Set the path to the executable
set EXE_PATH=..\x64\Release\SelfTest-API.exe

echo Running command sequence

REM Command sequence
for %%i in (
    "return_haptic_xy_1"
    "return_haptic_xy_1"
    "return_haptic_xy_1"
    "return_haptic_xy_1"
    "return_haptic_xy_1"
    "return_haptic_xy_1"
    "return_haptic_xy_1"
    "return_haptic_xy_1"
    "return_haptic_xy_1"
    "return_haptic_xy_1"
    "return_haptic_xy_1"
    "return_haptic_xy_1"
    "return_haptic_xy_1"
    "return_haptic_xy_1"
    "return_haptic_xy_1"
    "return_haptic_xy_1"
) do (
    echo Running command: %EXE_PATH% %%i
    %EXE_PATH% %%i
)

echo Command sequence complete
