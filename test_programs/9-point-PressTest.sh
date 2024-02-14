#!/bin/bash

# Set the path to the executable
EXE_PATH=../x64/Release/SelfTest-API.exe

# Command sequence
commands=("return_haptic_xy_1" "return_haptic_xy_1" "return_haptic_xy_1" "return_haptic_xy_1"
          "return_haptic_xy_1" "return_haptic_xy_1" "return_haptic_xy_1" "return_haptic_xy_1"
          "return_haptic_xy_1" "return_haptic_xy_1" "return_haptic_xy_1" "return_haptic_xy_1"
          "return_haptic_xy_1" "return_haptic_xy_1" "return_haptic_xy_1" "return_haptic_xy_1")

# Iterate through the commands
for cmd in "${commands[@]}"; do
    echo "Running command: $EXE_PATH $cmd"
    $EXE_PATH $cmd
done

echo "Command sequence complete"
