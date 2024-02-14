#!/bin/bash

EXE_PATH="../x64/Release/SelfTest-API.exe"

echo "Running command sequence"

# Run the first two change_parameters commands
echo "Running command: $EXE_PATH change_parameters 7 25"
$EXE_PATH change_parameters 7 25
echo "Running command: $EXE_PATH change_parameters 8 60"
$EXE_PATH change_parameters 8 60
echo "Running command: $EXE_PATH run_afpt"
$EXE_PATH run_afpt

# Continue with the rest of the commands
commands=("8 20" "7 75" "8 60" "7 125" "8 20")

for cmd in "${commands[@]}"; do
    echo "Running command: $EXE_PATH change_parameters $cmd"
    $EXE_PATH change_parameters $cmd
    echo "Running command: $EXE_PATH run_afpt"
    $EXE_PATH run_afpt
done

echo "Command sequence complete"
