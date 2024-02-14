#!/bin/bash

EXE_PATH="../x64/Release/SelfTest-API.exe"

echo "Running command sequence"

# Run the first two change_parameters commands
echo "Running command: $EXE_PATH change_parameters 7 29"
$EXE_PATH change_parameters 7 29
echo "Running command: $EXE_PATH change_parameters 8 51"
$EXE_PATH change_parameters 8 51
echo "Running command: $EXE_PATH run_afpt"
$EXE_PATH run_afpt

# Continue with the rest of the commands
commands=("8 17" "7 86" "8 61")

for cmd in "${commands[@]}"; do
    echo "Running command: $EXE_PATH change_parameters $cmd"
    $EXE_PATH change_parameters $cmd
    echo "Running command: $EXE_PATH run_afpt"
    $EXE_PATH run_afpt
done

echo "Command sequence complete"
