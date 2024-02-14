#!/bin/bash

EXE_PATH="../x64/Release/SelfTest-API.exe"

echo "Running command sequence"

commands=("7 25" "8 60" "7 75" "8 20" "7 125" "8 20")

for cmd in "${commands[@]}"; do
    echo "Running command: $EXE_PATH change_parameters $cmd"
    $EXE_PATH change_parameters $cmd
    echo "Running command: $EXE_PATH run_afpt"
    $EXE_PATH run_afpt
done

echo "Command sequence complete"
