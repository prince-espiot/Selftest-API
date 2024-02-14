# Set the path to the directory
$directory_path = "x64\Release"

# Full path to the executable
$executablePath = Join-Path -Path $PWD.Path -ChildPath "$directory_path\SelfTest-API.exe"

# Wait time between commands (adjust as needed)
$sleep_duration = 1

# Variable to track overall testing status
$testing_status = "Pass"

# Function to execute a command
function Execute-Command {
    param (
        [string]$cmd
    )

    $output = & "$executablePath" "$cmd" 2>&1 | ForEach-Object { $_ -replace "`0", "" }

    # Check the exit status of the last command
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Command successful: $cmd"
        if ($output -ne $null) {
            Write-Host "Output: $output"
        } else {
            Write-Host "Output is empty"
        }
    } else {
        Write-Host "Command failed: $cmd"
        Write-Host "Error Output: $output"
        $testing_status = "fail"
    }

    # Introduce a wait time
    Start-Sleep -Seconds $sleep_duration
}

# List of commands
$commands = @("read_build_id", "read_controller_id", "read_vdd_voltage", "read_vreff_voltage", "read_booster_voltage", "read_piezo_adc_voltages",
    "start_integral_storage", "read_calibration", "read_calibration_row", "init_piezo_foil_test", "play_haptics", "disable_fw_haptics",
    "store_integral_values", "read_stored_integrals", "read_nvm_page",  "erase_nvm_page", "read_capsense_flag", "read_bootloader_version",
    "read_aito_controller_sn", "test_piezo_taus", "test_piezos", "test_booster", "read_asic_serial_number", "return_true", "return_false", "go_to_active_idle",
    "reset")

# Iterate through the commands
foreach ($cmd in $commands) {
    Execute-Command $cmd
}

# Display overall testing status
Write-Host "Overall Testing Status: $testing_status"
