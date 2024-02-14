#!/bin/bash

# Set the path to the executable
EXE_PATH=../x64/Release/SelfTest-API.exe

echo "This is a testing script to test the SelfTestAPI! Update"

# List of commands
commands=("change_parameters 7 25" "change_parameters 8 60")

# Loop through each command and execute
for command in "${commands[@]}"; do
    echo "Running command: $command"
    $EXE_PATH $command

    # Check the exit code of the last command
    if [ $? -ne 0 ]; then
        echo "Error: Command $command failed with exit code $?"
        exit $?
    fi
done

echo "All commands executed successfully!"
