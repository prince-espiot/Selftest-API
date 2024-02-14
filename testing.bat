@echo off
setlocal enabledelayedexpansion

echo This is a testing script to test the SelfTestAPI! Update

REM List of commands
set "commands=read_build_id read_controller_id read_vdd_voltage read_vreff_voltage read_booster_voltage read_piezo_adc_voltages
              start_integral_storage read_calibration read_calibration_row init_piezo_foil_test play_haptics disable_fw_haptics
              store_integral_values read_stored_integrals read_nvm_page erase_nvm_page read_capsense_flag read_bootloader_version
              read_aito_controller_sn test_piezo_taus test_piezos test_booster read_asic_serial_number return_true return_false go_to_active_idle
              reset"

REM Set the path to the directory
set "directory_path=x64\Release"

REM Wait time between commands (adjust as needed)
set "sleep_duration=1"

REM Variable to track overall testing status
set "testing_status=Pass"

REM Function to execute a command
:ExecuteCommand
if "%commands%" == "" goto EndScript

REM Extract the first command
for /f "tokens=1,* delims= " %%a in ("%commands%") do (
    set "cmd=%%a"
    set "commands=%%b"
)

echo Executing command: %cmd%

REM Execute the command with the correct path
"%directory_path%\SelfTest-API.exe" %cmd%

REM Check the exit status of the last command
if %errorlevel% equ 0 (
    echo Command successful
) else (
    echo Command failed
    set "testing_status=Fail"
)

REM Introduce a wait time
ping -n %sleep_duration% 127.0.0.1 > nul
goto ExecuteCommand

:EndScript
echo Overall Testing Status: %testing_status%
endlocal
