#!/bin/bash

STRING="This is a testing script to test the SelfTestAPI! Update"
echo "$STRING"

# List of commands
commands=("read_build_id" "read_controller_id" "read_vdd_voltage" "read_vreff_voltage" "read_booster_voltage" "read_piezo_adc_voltages"
          "start_integral_storage" "read_calibration" "read_calibration_row" "init_piezo_foil_test" "play_haptics" "disable_fw_haptics"
          "store_integral_values" "read_stored_integrals" "read_nvm_page"  "erase_nvm_page" "read_capsense_flag" "read_bootloader_version"
          "read_aito_controller_sn" "test_piezo_taus" "test_piezos" "test_booster" "read_asic_serial_number" "return_true" "return_false" "go_to_active_idle"
            "reset")

# Set the path to the directory
directory_path="x64/Release"

# Wait time between commands (adjust as needed)
sleep_duration=1

# Variable to track overall testing status
testing_status="Pass"

# Function to execute a command
execute_command() {
    local cmd="$1"
    local output

    # Execute the command with the correct path and capture the output (both stdout and stderr)
    output=$("./$directory_path/SelfTest-API.exe" "$cmd" 2>&1 | tr -d '\0')  # Remove null bytes

    # Check the exit status of the last command
    if [ $? -eq 0 ]; then
        echo "Command successful: $cmd"
        [ -n "$output" ] && echo "Output: $output" || echo "Output is empty"
    else
        echo "Command failed: $cmd"
        echo "Error Output: $output"
        testing_status="fail"
    fi

    # Introduce a wait time
    sleep "$sleep_duration"
}

# Iterate through the commands
for cmd in "${commands[@]}"; do
    execute_command "$cmd"
done

# Display overall testing status
echo "Overall Testing Status: $testing_status"
