// AitoCommander.cpp : This file contains the 'main' function. Program execution
// begins and ends there.
//
#pragma comment( lib, "setupapi" )
#pragma comment( lib, "hid" )
#pragma comment( lib, "version" )
#pragma comment( lib, "imagehlp" )
#pragma warning( disable : 4996 ) // for std::codecvt deprecation

#include "../AitoTestingCommon/Common.h"
#include "../AitoTestingCommon/Nvm.h"
#include "../AitoTestingCommon/Selftest.h"
#include "Nvmprocessor.cpp"
#include "framework.h"
#include <algorithm>
#include <cfgmgr32.h>
#include <codecvt>
#include <conio.h>
#include <cstdint>
#include <fcntl.h>
#include <fstream>
#include <io.h>
#include <iostream>
#include <locale>
#include <regex>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wtypes.h>
#include <cfgmgr32.h>
#include "framework.h"
#include "../AitoTestingCommon/Common.h"
#include "../AitoTestingCommon/Selftest.h"
#include "../AitoTestingCommon/Nvm.h"

static const wchar_t *kNotAppl = L"N/A";
static const wchar_t *kQualifierError = L"Error: Wrong qualifier";
static const wchar_t *kMissingParameter = L"Error: Parameter missing";
static const wchar_t *kNoAitoDevicesError = L"No Aito\u00AE devices found";

static const uint32_t commit_id_data_size = 20;
enum class DisplayFormat : int { notDefined, decimal, hex, human };
static auto gStopped = false;

struct Job {
    bool toFile = false;    // write results to a date specific file
    bool chipModel = false; // read controller id
    bool chipSerNr = false; // read aito controller serial number
    bool asicSerNr = false; // read asic serial number
    bool bootVer = false;   // read bootloader version
    bool fwVer = false;     // read build id
    bool fwUpdate = false;  // update FW, possibly before and after the tests
    bool test = false;      // perform tests and write report file
    bool displayCalibrationsFromNvm = false; // read NVM and display it as a calibration data
    bool displayResultsFromNvm = false;
    bool read_stored_integrals = false; // read NVM and display it as a memory block
    DisplayFormat displayFormat = DisplayFormat::notDefined; // used format in output
    bool displayTestResultsFromNvm = false; // read NVM and display its structured self test result
    bool read_temperature = false;
    bool vddVoltage = false;
    bool read_vreff_voltage = false;
    bool read_booster_voltage = false;
    bool read_piezo_adc_voltages = false;
    bool start_integral_storage = false;
    bool read_calibration = false;
    bool play_haptics = false;
    bool disable_fw_haptics = false;
    bool enable_fw_haptics = false;
    bool store_integral_values = false;
    bool initpiezofoiltest = false;
    bool readstiffestim = false;
    bool readcapsenseflag = false;
    bool test_piezos = false;
    bool test_piezo_taus = false;
    bool test_booster = false;
    bool return_true = false;
    bool return_false = false;
    bool go_to_active_idle = false;
    bool erase_nvm_page = false;
    bool reset = false;
    bool run_afpt = false;
    bool return_haptic_xy = false;
    bool displayTestlimitsFile =  false; // displays TestLimits override values from the file
    bool generateTestlimitsFile = false; // genarates TestLimits override file for running self tests
    int minDistortion = 0;
    int minCapacitance = 0;
    int minStiffness = 0;
    int maxDistortion = 0;
    int maxCapacitance = 0;
    int maxStiffness = 0;

    uint8_t changeableParam = 0;
    uint8_t value = 0;

    /* Should the self-test interface be avoided in favor of the bootloader
     * interface to accommodate devices without an application? */
    bool avoidSelfTestInterface = false;

    explicit Job() = default;
};

using std::endl;
using std::hex;
using std::ostringstream;
using std::setfill;
using std::setprecision;
using std::setw;
using std::string;
using std::vector;
using std::wcout;
using std::wostringstream;
using std::wstring;

static bool SetSelfTestModeOn( TestRunner const &testRunner ) {
    auto unlockResult = testRunner.RunSingleSelfTest( SelfTestCommand::passcode_mask );
    if ( unlockResult.empty() ) {
        Error( L"Could not enable self-test mode" );
        return false;
    }
    return true;
}

static uint32_t ReadControllerId( TestRunner const &testRunner ) {
    auto controllerIdBuffer = testRunner.RunSingleSelfTest( SelfTestCommand::read_controller_id );
    if ( controllerIdBuffer.empty() ) {
        Error( L"Could not read controller ID" );
        return 0;
    }
    uint32_t controllerId = controllerIdBuffer[0] << 16 |controllerIdBuffer[1] << 8 | controllerIdBuffer[2];
    Log( L"Controller ID: %#x", controllerId );
    return controllerId;
}

static wstring ReadControllerSerialNumber( TestRunner const &testRunner ) {
    auto serialNumberBuffer = testRunner.RunSingleSelfTest( SelfTestCommand::read_aito_controller_serial_number );
    size_t serialNumberSize = 0;
    for ( auto const &b : serialNumberBuffer ) {
        if ( b == 0 || b == 255 ) {
            break;
        }
        serialNumberSize++;
    }
    if ( serialNumberSize == serialNumberBuffer.size() ) {
        serialNumberBuffer.emplace_back( 0 );
    } else {
        serialNumberBuffer[serialNumberSize] = 0;
    }
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    wstring serialNumberStr(converter.from_bytes( (const char *)serialNumberBuffer.data() ) );
    Log( L"Controller Serial No: %s", serialNumberStr.c_str() );
    return serialNumberStr;
}

static wstring ReadAsicSerialNumber( TestRunner const &testRunner ) {
    auto asicSerialNumberBuffer = testRunner.RunSingleSelfTest(SelfTestCommand::read_asic_serial_number );
    if ( asicSerialNumberBuffer.empty() ) {
        Error( L"Could not read ASIC serial number" );
        return {};
    }
    wostringstream asicSerialNumberStr;
    for ( auto i = 0; i < asicSerialNumberBuffer.size(); i++ ) {
        asicSerialNumberStr << hex << setw( 2 ) << setfill( L'0' ) << (int)asicSerialNumberBuffer[i];
    }
    Log( L"ASIC Serial No: %s", asicSerialNumberStr.str().c_str() );
    return asicSerialNumberStr.str();
}

static wstring ReadDeviceFirmwareStr( TestRunner const &testRunner ) {
    auto buildIdBuffer = testRunner.RunSingleSelfTest( SelfTestCommand::read_build_id );
    if ( buildIdBuffer.empty() ) {
        Error( L"Could not read firmware version" );
        return {};
    }
    // NUL-terminate
    buildIdBuffer.push_back( 0 );
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    wstring buildId(converter.from_bytes( (const char *)buildIdBuffer.data() ) );
    Log( L"Current firmware: %s", buildId.c_str() );
    return buildId;
}

static wstring ReadVddVoltage( TestRunner const &testRunner ) {
    auto VddVoltageBuffer = testRunner.RunSingleSelfTest( SelfTestCommand::read_vdd_voltage );
    if ( VddVoltageBuffer.empty() ) {
        Error( L"Could not read Vdd voltage" );
        return {};
    }
    std::wostringstream VddVoltage;
    for ( size_t i = 0; i < VddVoltageBuffer.size(); i += 2 ) {
        int result = ( VddVoltageBuffer[i + 1] << 8 ) + VddVoltageBuffer[i];
        VddVoltage << std::dec << result;
    }
    Log( L"Vdd Voltage: %s", VddVoltage.str().c_str() );
    return VddVoltage.str();
}

static wstring ReadVreffVoltage( TestRunner const &testRunner ) {
    auto VVoltageBuffer = testRunner.RunSingleSelfTest( SelfTestCommand::read_vref_voltage );
    if ( VVoltageBuffer.empty() ) {
        Error( L"Could not read Vdd voltage" );
        return {};
    }
    std::wostringstream VddVoltage;
    for ( size_t i = 0; i < VVoltageBuffer.size(); i += 2 ) {
        int result = ( VVoltageBuffer[i + 1] << 8 ) + VVoltageBuffer[i];
        VddVoltage << std::dec << result;
    }
    Log( L"Vref Voltage: %s", VddVoltage.str().c_str() );
    return VddVoltage.str();
}

static wstring ReadBoosterVoltage( TestRunner const &testRunner ) {
    auto VVoltageBuffer = testRunner.RunSingleSelfTest( SelfTestCommand::read_booster_voltage );
    if ( VVoltageBuffer.empty() ) {
        Error( L"Could not read Vdd voltage" );
        return {};
    }
    std::wostringstream VddVoltage;
    for ( size_t i = 0; i < VVoltageBuffer.size(); i += 2 ) {
        int result = ( VVoltageBuffer[i + 1] << 8 ) + VVoltageBuffer[i];
        VddVoltage << std::dec << result;
    }
    Log( L"Vref Voltage: %s", VddVoltage.str().c_str() );
    return VddVoltage.str();
}

static wstring ReadPiezoAdcVoltage( TestRunner const &testRunner ) {
    auto VVoltageBuffer = testRunner.RunSingleSelfTest( SelfTestCommand::read_piezo_adc_voltages );
    if ( VVoltageBuffer.empty() ) {
        Error( L"Could not read Vdd voltage" );
        return {};
    }
    std::wostringstream VddVoltage;
    for ( size_t i = 0; i < VVoltageBuffer.size(); i += 2 ) {
        int result = ( VVoltageBuffer[i + 1] << 8 ) + VVoltageBuffer[i];

        VddVoltage << std::dec << result << endl;
    }

    Log( L"Vref Voltage: %s", VddVoltage.str().c_str() );
    return VddVoltage.str();
}

static uint32_t ReadTemperature( TestRunner const &testRunner ) {
    auto readTemperatureBuffer = testRunner.RunSingleSelfTest( SelfTestCommand::read_temperature );
    if ( readTemperatureBuffer.empty() ) {
        Error( L"Could not read firmware version" );
        return {};
    }
    uint32_t temperature = readTemperatureBuffer[0];
    Log( L"Vdd Voltage: %#x", temperature );
    return temperature;
}

static bool InitialiseCalibration( TestRunner const &testRunner ) {
    auto initcalBuffer = testRunner.RunSingleSelfTest( SelfTestCommand::start_integral_storage );
    if ( initcalBuffer.empty() ) {
        Error( L"Could not read firmware version" );
        return false;
    }
    return true;
}

static wstring ReadCalibration( TestRunner const &testRunner ) {
    auto readcalibrationBuffer = testRunner.RunSingleSelfTest( SelfTestCommand::read_stiffestim );
    if ( readcalibrationBuffer.empty() ) {
        Error( L"Could not read Calibration" );
        return {};
    }
    wostringstream readCalNumberStr;
    for ( auto i = 0; i < readcalibrationBuffer.size(); i++ ) {
        readCalNumberStr << std::dec << readcalibrationBuffer[i]; // Using dec for decimal representation
        if ( i < readcalibrationBuffer.size() - 1 ) {
            readCalNumberStr << " "; // Add space between integers
        }
    }
    Log( L"Read Stiffestim: %s", readCalNumberStr.str().c_str() );
    return readCalNumberStr.str();
}

static bool PlayHaptics( TestRunner const &testRunner ) {
    auto playhapticsBuffer = testRunner.RunSingleSelfTest( SelfTestCommand::play_haptics );
    if ( playhapticsBuffer.empty() ) {
        Error( L"Could not read firmware version" );
        return false;
    }
    return true;
}

static bool DisableFwHaptics( TestRunner const &testRunner ) {
    auto DisablefwhapticsBuffer = testRunner.RunSingleSelfTest( SelfTestCommand::disable_fw_haptics );
    if ( DisablefwhapticsBuffer.empty() ) {
        Error( L"Could not read firmware version" );
        return false;
    }
    return true;
}
static bool EnableFwHaptics( TestRunner const &testRunner ) {
    auto EnablefwhapticsBuffer = testRunner.RunSingleSelfTest( SelfTestCommand::enable_fw_haptics );
    if ( EnablefwhapticsBuffer.empty() ) {
        Error( L"Could not read firmware version" );
        return false;
    }
    return true;
}

static wstring storeIntegralValues( TestRunner const &testRunner ) {
    auto storecalibrationBuffer = testRunner.RunSingleSelfTest( SelfTestCommand::store_integral_values );
    if ( storecalibrationBuffer.empty() ) {
        Error( L"Could not read the store calibration" );
        return {};
    }
    wostringstream store_integrals;
    for ( auto i = 0; i < storecalibrationBuffer.size(); i++ ) {
        store_integrals << std::dec << (int)storecalibrationBuffer[i];
    }
    Log( L"store_integral_values: %s", store_integrals.str().c_str() );
    return store_integrals.str();
}

static wstring ReadStiffestim( TestRunner const &testRunner ) {
    auto readStiffBuffer = testRunner.RunSingleSelfTest( SelfTestCommand::read_stiffestim );
    if ( readStiffBuffer.empty() ) {
        Error( L"Could not read Stiffestim" );
        return {};
    }
    wostringstream asicSerialNumberStr;
    for ( auto i = 0; i < readStiffBuffer.size(); i++ ) {
        asicSerialNumberStr << std::dec << readStiffBuffer[i]; // Using dec for decimal representation
        if ( i < readStiffBuffer.size() - 1 ) {
            asicSerialNumberStr << "\n"; // Add space between integers
        }
    }
    Log( L"Read Stiffestim: %s", asicSerialNumberStr.str().c_str() );
    return asicSerialNumberStr.str();
}

static bool InitialPiezoFoilTest( TestRunner const &testRunner ) {
    auto initPiezoBuffer = testRunner.RunSingleSelfTest( SelfTestCommand::init_piezo_foil_test );
    if ( initPiezoBuffer.empty() ) {
        
        return true;
    }
    return false;
}

static wstring ReadCapsenseFlag( TestRunner const &testRunner ) {
    auto readCapsenseflag = testRunner.RunSingleSelfTest( SelfTestCommand::read_capsense_flag );
    if ( readCapsenseflag.empty() ) {
        Error( L"Could not read capsense flag" );
        return {};
    }
    wostringstream readCapsenseStr;
    for ( auto i = 0; i < readCapsenseflag.size(); i++ ) {
        readCapsenseStr << std::dec << readCapsenseflag[i]; // Using dec for decimal representation
        if ( i < readCapsenseflag.size() - 1 ) {
            readCapsenseStr << "\n"; // Add space between integers
        }
    }
    Log( L"Read Capsense flag: %s", readCapsenseflag );
    return readCapsenseStr.str().c_str();
}

static wstring TestPeizoTaus( TestRunner const &testRunner ) {
    auto testpiezoTausBuffer = testRunner.RunSingleSelfTest( SelfTestCommand::test_piezo_taus );
    if ( testpiezoTausBuffer.empty() ) {
        Error( L"Could not read Test booster" );
        return {};
    }
    std::wostringstream testPiezo;
    for ( size_t i = 0; i < testpiezoTausBuffer.size(); i += 2 ) {
        int result = ( testpiezoTausBuffer[i + 1] << 8 ) + testpiezoTausBuffer[i];
        testPiezo << std::dec << result;
        if ( i + 2 < testpiezoTausBuffer.size() ) {
            if ( ( i + 2 ) % 8 == 0 ) {
                testPiezo << "\n";
            } else {
                testPiezo << "\t ";
            }
        }
    }
    Log( L"Test Piezo Taus:\n%s", testPiezo.str().c_str() );
    return testPiezo.str();
}

static wstring TestPeizos( TestRunner const &testRunner ) {
    auto testpiezoBuffer = testRunner.RunSingleSelfTest( SelfTestCommand::test_piezos );
    if ( testpiezoBuffer.empty() ) {
        Error( L"Could not read Test booster" );
        return {};
    }
    const int columns = 3;
    std::wostringstream testPiezo;
    for ( auto i = 0; i < testpiezoBuffer.size(); i++ ) {
        testPiezo << std::dec << testpiezoBuffer[i];
        if ( ( i + 1 ) % columns == 0 && i < testpiezoBuffer.size() - 1 ) {
            testPiezo << "\n"; // Use a tab character to separate columns
        } else {
            testPiezo << "\t";
        }
    }
    Log( L"Test Piezos :\n%s", testPiezo.str().c_str() );
    return testPiezo.str();
}

static std::wstring TestBooster( const TestRunner &testRunner ) {
    auto testBoostBuffer = testRunner.RunSingleSelfTest( SelfTestCommand::test_booster );
    if ( testBoostBuffer.empty() ) {
        Error( L"Could not read Test booster" );
        return {};
    }
    std::wostringstream testBoost;
    for ( size_t i = 0; i < testBoostBuffer.size(); i += 2 ) {
        int result = ( testBoostBuffer[i + 1] << 8 ) + testBoostBuffer[i];
        testBoost << std::dec << result;
        if ( i + 2 < testBoostBuffer.size() ) {
            testBoost << " ";
        }
    }
    Log( L"Test Booster: %s", testBoost.str().c_str() );
    return testBoost.str();
}

static bool ReturnTrue( TestRunner const &testRunner ) {
    auto returntrue = testRunner.RunSingleSelfTest( SelfTestCommand::return_true );
    if ( returntrue.empty() ) {
        Error( L"Could not return true" );
        return false;
    }
    return true;
}

static bool ReturnFalse( TestRunner const &testRunner ) {
    auto returnfalse = testRunner.RunSingleSelfTest( SelfTestCommand::return_false );
    if ( returnfalse.empty() ) {
        Error( L"Could not return false" );
        return false;
    }
    return true;
}

static bool EraseNvmPage( TestRunner const &testRunner ) {
    auto erasenvmBuffer = testRunner.RunSingleSelfTest( SelfTestCommand::erase_nvm_page );
    if ( erasenvmBuffer.empty() ) {
        Error( L"Could not read firmware version" );
        return false;
    }
    return true;
}

static bool Reset( TestRunner const &testRunner ) {
    auto resetBuffer = testRunner.RunSingleSelfTest( SelfTestCommand::reset );
    if ( resetBuffer.empty() ) {
        Error( L"Could not reset" );
        return false;
    } 
    return true;
}

static std::wstring RunAfpt( const TestRunner &testRunner ) {
    auto runafptBuffer = testRunner.RunSingleSelfTest( SelfTestCommand::start_AFPT_test );
    if ( runafptBuffer.empty() ) {
        Error( L"Could not read Test booster" );
        return {};
    }
    std::wostringstream runafpt;
    for ( size_t i = 0; i < runafptBuffer.size(); i += 2 ) {
        if ( i < 8 ) { 
            // First four values (0 to 7) are not shifted
            runafpt << std::dec << runafptBuffer[i];
        } else {
            // Perform left bitshift by 8 bits for the remaining values
            int result = ( runafptBuffer[i + 1] << 8 ) + runafptBuffer[i];
            runafpt << std::dec << result;
        }
        if ( i + 2 < runafptBuffer.size() ) {
            runafpt << " ";
        }
    }
    Log( L"Run aftpt: %s", runafpt.str().c_str() );
    return runafpt.str();
}

static bool ChangeParamJob( const Job &job ) {
    wostringstream result;
    auto devices = FindSelfTestDevices();
    if ( devices.empty() ) {
        wcout << kNoAitoDevicesError << endl;
        return false;
    }
    TestRunner testRunner( devices[0] );
    if ( !EnableTestParametersChange( testRunner ) ) {
        return false;
    }
    // pass arguments to be changed
    auto paramsTask = FormChangeableParam(static_cast<ChangeableParams>( job.changeableParam ),static_cast<uint16_t>( job.value ) );
    Log( L"Min paramsTask: %d", paramsTask );
    auto paramsResponse = testRunner.RunSingleSelfTest( paramsTask );
    if ( paramsResponse.empty() || paramsResponse[0] != 1 ) {
        Log( L"could not set calibration year" );
        return false;
    }
    return true;
}

static wstring GoToActiveIdle( TestRunner const &testRunner ) {
    auto gotoactiveidleBuffer = testRunner.RunSingleSelfTest( SelfTestCommand::go_to_active_idle );
    if ( gotoactiveidleBuffer.empty() ) {
        Error( L"Could not GoToActiveIdle" );
        return {};
    }
    wostringstream go_active;
    for ( auto i = 0; i < gotoactiveidleBuffer.size(); i++ ) {
        go_active << std::dec << (int)gotoactiveidleBuffer[i];
    }
    Log( L"Go active idle: %s", go_active.str().c_str() );
    return go_active.str();
}

static wstring ReturnHapticsXY( const Job &job ) {
    auto devices = FindSelfTestDevices();
    if ( devices.empty() ) {
        wcout << kNoAitoDevicesError << endl;
        return {};
    }
    TestRunner testRunner( devices[0] );
    auto returnhapticBuffer = testRunner.RunSingleSelfTest( SelfTestCommand::return_haptic_xy );
    Log( L"Min paramsTask: %d", returnhapticBuffer );
    if ( returnhapticBuffer.empty() ) {
        Error( L"Could not Return Haptics XY" );
        return {};
    }
    std::wostringstream returnhaptic;
    for ( size_t i = 0; i < returnhapticBuffer.size(); i++ ) {
        returnhaptic << std::dec << returnhapticBuffer[i];
        if ( i < returnhapticBuffer.size() ) {
            returnhaptic << " ";
        }
    }
    Log( L"Return Haptics XY: %s", returnhaptic.str().c_str() );
    return returnhaptic.str();
}

static wstring ReturnHapticsXYOne( const TestRunner &testRunner ) {
    auto returnhapticBuffer = testRunner.RunSingleSelfTest( SelfTestCommand::return_haptic_xy );

    if ( returnhapticBuffer.empty() ) {
        Error( L"Could not Return Haptics XY" );
        return {};
    }
    std::wostringstream returnhaptic;
    for ( size_t i = 0; i < returnhapticBuffer.size(); i++ ) {  
        returnhaptic << std::dec << returnhapticBuffer[i];
        if ( i < returnhapticBuffer.size() ) {
            returnhaptic << " ";
        }
    }
    Log( L"Return Haptics XY: %s", returnhaptic.str().c_str() );
    return returnhaptic.str();
}

// A structure to hold all the information that can be read using bootloader
// commands
// As much as possible is read in one go to avoid having to reset the DUT more
// than once
struct BootloaderInfo {
    // The bootloader commit ID
    wstring commitId;
    // A name for a known set of key fingerprints.
    wstring chipId;
    // The firmware product name of the current application.
    // Unlike the two fields above, this characterizes the application and not
    // the bootloader, but it's still here since it's read using a bootloader
    // command.
    wstring productName;
    BootloaderInfo() = default;
};

static wstring ReadBootloaderVersion( TestRunner const &testRunner ) {
    auto bootVersionBuffer = testRunner.RunSingleSelfTest(SelfTestCommand::read_bootloader_version );
    if ( bootVersionBuffer.empty() ) {
        Error( L"Could not read bootloader version" );
        return {};
    }
    wostringstream bootVersionStr;
    for ( auto i = 0; i < commit_id_data_size; i++ ) {
        bootVersionStr << hex << setw( 2 ) << setfill( L'0' )
                       << (uint8_t)bootVersionBuffer[i];
    }
    Log( L"Current bootloader: %s", bootVersionStr.str().c_str() );
    return bootVersionStr.str();
}

template <class... Args> static wstring ToHexString( Args &&...args ) {
    wostringstream result;
    std::vector<uint8_t> bytes( args... );
    for ( uint8_t c : bytes ) {
        result << hex << setw( 2 ) << setfill( L'0' ) << c;
    }
    return result.str();
}

static constexpr uint8_t BootloaderCmdGetApplicationVersion = 'v';
static constexpr uint8_t BootloaderCmdGetCommitId = 'G';
static constexpr uint8_t BootloaderCmdGetKeyFingerprint = 'k';
static constexpr uint8_t BootloaderCmdStartApplication = 'a';

static std::optional<BootloaderInfo> ReadBootloaderInfo( TestRunner const &testRunner ) {

    const std::map<std::vector<std::vector<uint8_t>>, wstring>
        bootloaderKeyFingerprints = {
            // ATH8134 has 3 keys: The first two are identical to ATH8124 and
            // the third one is special
            { { { 0x1,  0x9b, 0x2b, 0xbb, 0x7c, 0xe6, 0x14, 0x7d, 0xa2,
                  0xda, 0x37, 0xd,  0xa2, 0x62, 0xdd, 0xa2, 0xb5, 0x9,
                  0xb3, 0x65, 0x51, 0x2c, 0x4d, 0xb8, 0x5,  0x2c, 0xd1,
                  0x8e, 0x50, 0x8e, 0x96, 0x50, 0x1a },
                { 0x1,  0x9b, 0x2b, 0xbb, 0x7c, 0xe6, 0x14, 0x7d, 0xa2,
                  0xda, 0x37, 0xd,  0xa2, 0x62, 0xdd, 0xa2, 0xb5, 0x9,
                  0xb3, 0x65, 0x51, 0x2c, 0x4d, 0xb8, 0x5,  0x2c, 0xd1,
                  0x8e, 0x50, 0x8e, 0x96, 0x50, 0x1a },
                { 0x1,  0x7b, 0x81, 0xda, 0xf9, 0xa6, 0x8a, 0xaa, 0xa4,
                  0xa3, 0x2f, 0xfc, 0xb2, 0x2,  0x6f, 0xf8, 0x62, 0xec,
                  0x36, 0xce, 0x9c, 0x54, 0x65, 0x9e, 0xc2, 0x27, 0xd5,
                  0x17, 0xe7, 0x7a, 0x4f, 0xfb, 0xa3 } },
              L"ATH8134" },
            { { { 0x1,  0x9b, 0x2b, 0xbb, 0x7c, 0xe6, 0x14, 0x7d, 0xa2,
                  0xda, 0x37, 0xd,  0xa2, 0x62, 0xdd, 0xa2, 0xb5, 0x9,
                  0xb3, 0x65, 0x51, 0x2c, 0x4d, 0xb8, 0x5,  0x2c, 0xd1,
                  0x8e, 0x50, 0x8e, 0x96, 0x50, 0x1a } },
              L"ATH8124" },
            { { { 0x1,  0x41, 0xdc, 0xaa, 0xbb, 0x77, 0x3d, 0x54, 0x67,
                  0x41, 0xc4, 0x35, 0xee, 0x88, 0xd5, 0xee, 0x7,  0xa2,
                  0x32, 0xec, 0xb2, 0x67, 0xf0, 0x69, 0xcf, 0xf1, 0xc4,
                  0xb2, 0xf8, 0x3a, 0x38, 0x6e, 0xdb } },
              L"ATH8144" },
        };

    const std::unordered_map<uint8_t, wstring> updateIdNames = {
        { uint8_t{ 0x77 }, L"AIT101" }, // Tributo (or EVK)
        { uint8_t{ 0x3C }, L"AIT112" }, // Diablo
        { uint8_t{ 0xC7 }, L"AIT402" }, // OLED keyboard
        { uint8_t{ 0xE2 }, L"AIT113" }, // Pista
        { uint8_t{ 0x6B }, L"AIT003" }, // Should this be ARE003?
        { uint8_t{ 0x13 }, L"AIT802" }, // Albatross
        { uint8_t{ 0xD5 }, L"RND007" }, // Low-cost (?)
        { uint8_t{ 0x24 }, L"ARE002" }, { uint8_t{ 0xFF }, L"None" },
    };

    testRunner.TryReset();
    // Send the 'v' command once here and ignore the response. We're going to
    // send it "for real" later, this is just to work around timing issues with
    // the Elan bridge.
    //
    // In case we're not using the Elan bridge, or in case we're already in the
    // bootloader and all the commands are getting through, it should be
    // harmless since the 'v' command is idempotent.
    testRunner.BootloaderCommand( BootloaderCmdGetApplicationVersion,
                                  std::nullopt );

    // (Try to) boot back into the application when this function returns
    struct Reset {
        TestRunner const &testRunner;
        Reset( TestRunner const &testRunner ) : testRunner( testRunner ) {}
        ~Reset() {
            testRunner.BootloaderCommand( BootloaderCmdStartApplication,
                                          std::nullopt );

            Sleep( 1000 );
        }
    } reset( testRunner );
    std::vector<uint8_t> vResponse;
    if ( !testRunner.BootloaderCommand( BootloaderCmdGetApplicationVersion, std::nullopt, vResponse ) ) {
        Log( L"Could not query the bootloader protocol version" );
        return std::nullopt;
    }

    if ( vResponse[0] != 'V' ) {
        // If the response is not 'V', it means the bootloader is too old to
        // support any of these commands anyway.
        Log( L"Unexpected bootloader response - bootloader too old?" );
        return std::nullopt;
    }

    BootloaderInfo result;

    if ( ( vResponse[0] ^ vResponse[1] ) != vResponse[2] ) {
        Log( L"Invalid V response: %02x, %02x, %02x", vResponse[0], vResponse[1], vResponse[2] );
        return std::nullopt;
    } else {
        Log( L"Update ID: %02x", vResponse[1] );
        if ( auto value = updateIdNames.find( vResponse[1] );
             value != updateIdNames.end() ) {
            result.productName = value->second;
        } else {
            result.productName = L"";
        }

        uint16_t sentinel = vResponse[3] | ( vResponse[4] << 8 );
        uint16_t version = vResponse[5] | ( vResponse[6] << 8 );

        Log( L"Sentinel: %04x, version: %04x", sentinel, version );
        // version is only valid if sentinel has the right magic value
        if ( sentinel == 0xa55a && version >= 2 ) {
            std::vector<uint8_t> gResponse;
            if ( !testRunner.BootloaderCommand( BootloaderCmdGetCommitId, std::nullopt, gResponse ) ) {
                return std::nullopt;
            }
            if ( ( gResponse[0] ^ gResponse[1] ) == gResponse[2] ) {
                result.commitId = ToHexString( gResponse.begin() + 3, gResponse.begin() + 3 + gResponse[1] );
            }
            // All the key fingerprints returned by the bootloader
            vector<vector<uint8_t>> keyFingerprints;
            for ( uint8_t keyIndex = 0;; keyIndex++ ) {
                std::vector<uint8_t> kResponse;
                if ( !testRunner.BootloaderCommand(BootloaderCmdGetKeyFingerprint, keyIndex, kResponse ) ) {
                    return std::nullopt;
                }
                if ( ( kResponse[0] ^ kResponse[1] ) == kResponse[2] && kResponse[1] != 0 ) {
                    keyFingerprints.emplace_back( kResponse.begin() + 3,  kResponse.begin() + 3 +  kResponse[1] );
                    wstring keyFingerprint = ToHexString( keyFingerprints.back() );
                    Log( L"Key fingerprint %d: %s", keyIndex,  keyFingerprint.c_str() );
                } else {
                    break;
                }
            }
            if ( auto value = bootloaderKeyFingerprints.find( keyFingerprints ); value != bootloaderKeyFingerprints.end() ) {
                result.chipId = value->second;
            } else {
                result.chipId = L"";
            }
        }
    }
    return result;
}

static vector<uint8_t> ReadFromNvm( TestRunner const &testRunner ) {
    auto nvmDataBuffer = testRunner.RunSingleSelfTest( SelfTestCommand::read_nvm_page );
    if ( nvmDataBuffer.empty() ) {
        Error( L"Could not read NVM data" );
        return {};
    }
    Log( L"NVM data length: %d", nvmDataBuffer.size() );
    return nvmDataBuffer;
}

struct calibrationDataElement {
    wstring name;
    std::function<void( wostringstream &, int )> print;
    uint8_t stored_flag;
};

template <int PiezoCount>
static wstring OutputCalibrationDataFromNvm( vector<uint8_t> const &nvmDataBuffer, DisplayFormat format ) {
    if ( nvmDataBuffer.empty() ) {
        return {};
    }
    const CalibrationData<PiezoCount> *calibrationData = reinterpret_cast<const CalibrationData<PiezoCount> *>( nvmDataBuffer.data() );
    std::vector<calibrationDataElement> cData = {
    { L"Piezo", [](auto& out, int index) { out << std::setw(8) << index + 1; }, 0 },
    { L"Stiffness", [](auto& out, int index) { out << std::setw(8) << calibrationData->g_reg_stiffnesses[index]; }, calibrationData->g_reg_stiffnesses_stored_flag },
    { L"Moving Aver", [](auto& out, int index) { out << std::setw(8) << calibrationData->mavg_sum[index]; }, calibrationData->mavg_sum_stored_flag },
    { L"Foil:         ", [](auto&, int) { return; }, 0 },  
    { L"Distortion", [](auto& out, int index) { out << std::setw(8) << calibrationData->st_distortion_foil[index]; }, calibrationData->st_distortion_foil_stored_flag },
    { L"Capacitance", [](auto& out, int index) { out << std::setw(8) << calibrationData->st_capacitance_foil[index]; }, calibrationData->st_capacitance_foil_stored_flag },
    { L"Charge", [](auto& out, int index) { out << std::setw(8) << calibrationData->st_stiffness_foil[index]; }, calibrationData->st_stiffness_foil_stored_flag },
    { L"Stack:        ", [](auto&, int) { return; }, 0 },  
    { L"Distortion", [](auto& out, int index) { out << std::setw(8) << calibrationData->st_distortion_stack[index]; }, calibrationData->st_distortion_stack_stored_flag },
    { L"Capacitance", [](auto& out, int index) { out << std::setw(8) << calibrationData->st_capacitance_stack[index]; }, calibrationData->st_capacitance_stack_stored_flag },
    { L"Charge", [](auto& out, int index) { out << std::setw(8) << calibrationData->st_stiffness_stack[index]; }, calibrationData->st_stiffness_stack_stored_flag },
};
    wostringstream lines;
    for ( auto l = 0; l < cData.size(); l++ ) {
        lines << setw( 16 ) << cData[l].name;
        for ( auto i = 0; i < PiezoCount; i++ ) {
            if ( cData[l].stored_flag == 0 ) {
                cData[l].print( lines, i );
            } else {
                lines << setw( 8 ) << kNotAppl;
            }
        }
        lines << endl;
    }
    return lines.str();
}

static wstring OutputSelfTestHistoryFromNvm( vector<uint8_t> const &nvmDataBuffer,  DisplayFormat format ) {
    if ( nvmDataBuffer.empty() ) {
        return {};
    }

    wostringstream selfTestData;
    for ( auto i = 0; i < nvmDataBuffer.size(); i++ ) {
        if ( i % 16 == 0 ) {
            selfTestData << setw( 4 ) << setfill( L'0' ) << i << L": ";
        } else if ( i % 16 == 8 ) {
            selfTestData << L' ';
        }
        if ( format == DisplayFormat::decimal ) {
            selfTestData << setw( 3 ) << setfill( L' ' )
                         << (uint8_t)nvmDataBuffer[i] << L' ';
        } else {
            selfTestData << hex << setw( 2 ) << setfill( L'0' )
                         << (uint8_t)nvmDataBuffer[i] << L' ';
        }
        if ( i % 16 == 15 ) {
            selfTestData << endl;
        }
    }
    return selfTestData.str();
}

static void LogAndPrintOut( const wchar_t *msg, wostringstream &result ) {
    result << endl << msg << endl;
    Log( msg );
}

static void LogAndErrorOut( const wchar_t *msg, wostringstream &result ) {
    result << endl << L"Error: " << msg << endl;
    Error( msg );
}

static std::optional<std::wstring> ReadFirmwareVersion( TestRunner const &testRunner ) {
    bool success = SetSelfTestModeOn( testRunner );
    if ( success ) {
        auto buildStr = ReadDeviceFirmwareStr( testRunner );
        if ( !buildStr.empty() ) {
            return buildStr;
        }
    }
    return std::nullopt;
}

static void OutputFirmwareVersion( std::optional<std::wstring> const &buildStr,  wostringstream &result ) {
    if ( buildStr.has_value() ) {
        result << buildStr.value() << endl;
    }
}

struct DeviceInfo {
    std::optional<uint32_t> controllerId; //< Identifier of very old aito modules (mcu + discrete components). Not used, shall be set to 0xFF 0xFF 0xFF.
    std::optional<std::wstring> serialNumber; //< Something ...
    std::optional<std::wstring> electronicsModuleId; //< DEL008-E02-07 or DEL008-E02-08 - identfier of PCBA
    std::optional<std::wstring> serialNumberSuffix; //< part of serial number in case of success identification of the PCBA
    std::optional<std::wstring> asicSerialNumber; //< Uniq id of the ASIC
    std::optional<BootloaderInfo> bootloaderInfo; //< Whole info from bootloader, struct
    std::optional<std::wstring> firmwareBuildId; //< Identifier of the application version
    std::vector<uint8_t> nvmContents;
    std::vector<uint8_t> read_stored_integrals;  //< Something ...
    std::optional<uint32_t> read_temperature;
    std::optional<std::wstring> vddVoltage;
    std::optional<std::wstring> read_vreff_voltage;
    std::optional<std::wstring> read_booster_voltage;
    std::optional<std::wstring> read_piezo_adc_voltages;
    std::optional<bool> erase_nvm_page;
    std::optional<bool> reset;
    std::optional<bool> initcalibration;
    std::optional<std::wstring> readcalibration;
    std::optional<bool> playhaptics;
    std::optional<bool> disable_fw_haptics;
    std::optional<bool> enable_fw_haptics;
    std::optional<std::wstring> store_integral_values;
    std::optional<bool> init_piezo_foil_test;
    std::optional<std::wstring> read_stiffestim;
    std::optional<std::wstring> read_capsense_flag;
    std::optional<std::wstring> test_piezo_taus;
    std::optional<std::wstring> test_piezos;
    std::optional<std::wstring> test_booster;
    std::optional<bool> return_true;
    std::optional<bool> return_false;
    std::optional<std::wstring> go_to_active_idle;
    std::optional<std::wstring> run_afpt;
    std::optional<std::wstring> return_haptic_xy;
    int piezoCount = 0;

    DeviceInfo() = default;

    std::wstring GetChipModel() const {
        if ( bootloaderInfo.has_value() && !bootloaderInfo->chipId.empty() ) {
            return bootloaderInfo->chipId;
        }
        if ( controllerId.has_value() ) {
            switch ( controllerId.value() ) {
            case 0x028c40:
                return L"ATH8124 (application)";
                break;
            case 0x028e40:
                return L"ATH8144 (application)";
                break;
            }
        }
        return L"";
    }

    /**
     * @brief Calculation of ASIC amount inside the connected device
     *
     * @param asicSerialNumbersFromDevice
     */
    void calculateAsicsAmount( std::wstring asicSerialNumbersFromDevice ) {
        asics_amount = asicSerialNumbersFromDevice.size() / ( kASICSerialNumberSize * sizeof( asicSerialNumbersFromDevice[0] ) );
    }
    /**
     * @brief Get the Amount Of Asics
     *
     * @return size_t
     */
    size_t getAmountOfAsics() const { return asics_amount; }
    /// @brief size of the serial number of ASIC in ASCII characters
    static constexpr size_t kASICSerialNumberSize = 16U;
  private:
    /// @brief Amount of ASICs in connected device
    size_t asics_amount = 0;
};

static void FormatRequests( DisplayFormat const &displayFormat,DeviceInfo const &info, wostringstream &result ) {
    /*
     * Used to format output before they are displayed
     */
    if ( auto chipModel = info.GetChipModel(); !chipModel.empty() ) {
        result << chipModel << endl;
    }

    if ( info.serialNumber.has_value() ) {
        result << info.serialNumber.value() << endl;
    }

    if ( info.electronicsModuleId.has_value() ) {
        result << info.electronicsModuleId.value() << endl;
    }

    if ( info.asicSerialNumber.has_value() ) {
        for ( auto i = 0; i < info.getAmountOfAsics(); i++ ) {
            result << info.asicSerialNumber.value().substr( i * DeviceInfo::kASICSerialNumberSize, DeviceInfo::kASICSerialNumberSize )
                   << " ";
        }
        result << endl;
    }

    if ( info.vddVoltage.has_value() ) {
        for ( auto value : info.vddVoltage.value() ) {
            result << value << "";
        }
        result << std::endl;
    }

    if ( info.read_vreff_voltage.has_value() ) {
        for ( auto value : info.read_vreff_voltage.value() ) {
            result << value << "";
        }
        result << std::endl;
    }

    if ( info.read_booster_voltage.has_value() ) {
        for ( auto value : info.read_booster_voltage.value() ) {
            result << value;
        }
        result << std::endl;
    }

    if ( info.read_piezo_adc_voltages.has_value() ) {
        for ( auto value : info.read_piezo_adc_voltages.value() ) {
            result << value;
        }
        result << std::endl;
    }

    if ( info.read_temperature.has_value() ) {
        result << info.read_temperature.value() << endl;
    }

    if ( info.initcalibration.has_value() ) {
        if ( info.initcalibration.value() == 1 ) {
            result << L"Pass";
        } else {
            result << L"Fail";
        }
        result << std::endl;
    }

    if ( info.readcalibration.has_value() ) {
        for ( auto value : info.readcalibration.value() ) {
            result << value << "";
        }
        result << std::endl;
    }

    if ( info.playhaptics.has_value() ) {
        if ( info.playhaptics.value() == 1 ) {
            result << L"Pass";
        } else {
            result << L"Fail";
        }
        result << std::endl;
    }

    if ( info.disable_fw_haptics.has_value() ) {
        if ( info.disable_fw_haptics.value() == 1 ) {
            result << L"Pass";
        } else {
            result << L"Fail";
        }
        result << std::endl;
    }
    if ( info.enable_fw_haptics.has_value() ) {
        if ( info.enable_fw_haptics.value() == 1 ) {
            result << L"Pass";
        } else {
            result << L"Fail";
        }
        result << std::endl;
    }

    if ( info.store_integral_values.has_value() ) {
        for ( auto value : info.store_integral_values.value() ) {
            result << value << " ";
        }
        result << std::endl;
    }

    if ( info.read_stiffestim.has_value() ) {
        for ( auto value : info.read_stiffestim.value() ) {
            result << value << "";
        }
        result << std::endl;
    }

    if ( info.init_piezo_foil_test.has_value() ) {
        if ( info.init_piezo_foil_test.value() == 1 ) {
            result << L"Pass";
        } else {
            result << L"Fail";

        }
        result << std::endl;
    }

    if ( info.read_capsense_flag.has_value() ) {
        if ( info.read_capsense_flag.value() == L"1" ) {
            result << L"True";
        } else {
            result << L"False";
        }
        result << std::endl;
    }

    if ( info.test_piezos.has_value() ) {
        for ( auto value : info.test_piezos.value() ) {
            result << value << "";
        }
        result << std::endl;
    }

    if ( info.test_piezo_taus.has_value() ) {
        for ( auto value : info.test_piezo_taus.value() ) {
            result << value << "";
        }
        result << std::endl;
    }

    if ( info.test_booster.has_value() ) {
        for ( auto value : info.test_booster.value() ) {
            result << value;
            result << "";
        }
        result << std::endl;
    }
    if ( info.return_true.has_value() ) {
        if ( info.return_true.value() == 1 ) {
            result << L"True";
        } else {
            result << L"False";
        }
        result << std::endl;
    }
    if ( info.return_false.has_value() ) {

        if ( info.return_false.value() == 1 ) {
            result << L"False";
        } else {
            result << L"True";
        }
        result << std::endl;
    }
    if ( info.go_to_active_idle.has_value() ) {

        for ( auto value : info.go_to_active_idle.value() ) {
            result << value << " ";
        }
        result << std::endl;
    }

    if ( info.bootloaderInfo.has_value() ) {
        if ( !info.bootloaderInfo->commitId.empty() ) {
            result << info.bootloaderInfo->commitId << endl;
        }
    }

    if ( info.erase_nvm_page.has_value() ) {
        if ( info.erase_nvm_page.value() == 1 ) {
            result << L"Pass";
        } else {
            result << L"Fail";
        }
        result << std::endl;
    }

    if ( info.reset.has_value() ) {

        if ( info.reset.value() == 1 ) {
            result << L"Pass";
        } else {
            result << L"Fail";
        }
        result << std::endl;
    }

    if ( info.run_afpt.has_value() ) {
        for ( auto value : info.run_afpt.value() ) {
            result << value;
            result << "";
        }
        result << std::endl;
    }

    if ( info.return_haptic_xy.has_value() ) {
        for ( auto value : info.return_haptic_xy.value() ) {
            result << value;
            result << "";
        }

        result << std::endl;
    }
    OutputFirmwareVersion( info.firmwareBuildId, result );
    if ( !info.nvmContents.empty() ) { // this one does nvm.
        auto nvmData = OutputSelfTestHistoryFromNvm( info.nvmContents, displayFormat );
        if ( !nvmData.empty() ) {
            result << L"NVM data:" << endl << nvmData << endl;
        }
    }
    if ( !info.read_stored_integrals.empty() ) {
        auto nvmData = OutputSelfTestHistoryFromNvm( info.read_stored_integrals, displayFormat );
        if ( !nvmData.empty() ) {
            HexDataFormatter formatter( nvmData );
            std::wstring formattedData = formatter.formatAndPrint();
            if ( !formattedData.empty() )
                result << formattedData;
            else {
                result << "No Nvm data to be read" << endl;
            }
        } else {
            result << "No Nvm data to be read" << endl;
        }
    }

}

static bool RunRequests( const Job &job ) {
    const unsigned int deviceType = job.avoidSelfTestInterface ? DeviceType::Any : DeviceType::SelfTest;
    wostringstream result;
    vector<SelfTestDevice> devices = FindSelfTestDevices( deviceType );
    wstring serNo;
    bool success = true;
    /*Makes a function call based on the active job element*/
    if ( devices.size() > 0 ) {
        wstring endOfSerNo = L"";
        std::vector<wstring> errors;
        DeviceInfo info;
        { // a block for testRunner
            TestRunner testRunner( devices[0], deviceType );
            if ( !job.avoidSelfTestInterface &&
                 !SetSelfTestModeOn( testRunner ) ) {
                errors.emplace_back(L"Could not enter self-test mode" );
                success = false;
            }
            if ( job.chipModel && success ) {
                auto controllerId = ReadControllerId( testRunner );
                if ( controllerId != 0 ) {
                    info.controllerId.emplace( controllerId );
                } else {
                    errors.emplace_back(L"Could not read the controller ID" );
                    success = false;
                }
            }

            if ( job.vddVoltage && success ) {
                auto vddVoltage = ReadVddVoltage( testRunner );
                if ( !vddVoltage.empty() ) {
                    info.vddVoltage.emplace( vddVoltage );
                } else {
                    errors.emplace_back(L"Could not read the Vdd voltage" );
                    success = false;
                }
            }

            if ( job.read_vreff_voltage && success ) {
                auto vVoltage = ReadVreffVoltage( testRunner );
                if ( !vVoltage.empty() ) {
                    info.read_vreff_voltage.emplace( vVoltage );
                } else {
                    errors.emplace_back(L"Could not read the Vref voltage" );
                    success = false;
                }
            }
            if ( job.read_booster_voltage && success ) {
                auto vVoltage = ReadBoosterVoltage( testRunner );
                if ( !vVoltage.empty() ) {
                    info.read_booster_voltage.emplace( vVoltage );
                } else {
                    errors.emplace_back(L"Could not read the Booster voltage" );
                    success = false;
                }
            }
            if ( job.read_piezo_adc_voltages && success ) {
                auto vVoltage = ReadPiezoAdcVoltage( testRunner );
                if ( !vVoltage.empty() ) {
                    info.read_piezo_adc_voltages.emplace( vVoltage );
                } else {
                    errors.emplace_back(L"Could not read the Piezo voltage" );
                    success = false;
                }
            }

            if ( job.read_temperature && success ) {
                auto temperature = ReadTemperature( testRunner );
                if ( temperature != 0 ) {
                    info.read_temperature.emplace( temperature );
                } else {
                    errors.emplace_back(L"Could not read the Vdd voltage" );
                    success = false;
                }
            }

            if ( job.start_integral_storage && success ) {
                auto initcalibration = InitialiseCalibration( testRunner );
                if ( initcalibration != 0 ) {
                    info.initcalibration.emplace( initcalibration );
                } else {
                    errors.emplace_back(L"Could not read initial calibration" );
                    success = false;
                }
            }

            if ( job.read_calibration && success ) {
                auto read_calibration = ReadCalibration( testRunner );
                if ( !read_calibration.empty() ) {
                    info.readcalibration.emplace( read_calibration );
                } else {
                    errors.emplace_back(L"Could not read calibration" );
                    success = false;
                }
            }

            if ( job.play_haptics && success ) {
                auto play_haptics = PlayHaptics( testRunner );
                if ( play_haptics != 0 ) {
                    info.playhaptics.emplace( play_haptics );
                } else {
                    errors.emplace_back(L"Could not read initial calibration" );
                    success = false;
                }
            }

            if ( job.disable_fw_haptics && success ) {
                auto disablefw = InitialiseCalibration( testRunner );
                if ( disablefw != 0 ) {
                    info.disable_fw_haptics.emplace( disablefw );
                } else {
                    errors.emplace_back(L"Could not read initial calibration" );
                    success = false;
                }
            }

            if ( job.enable_fw_haptics && success ) {
                auto enablefw = InitialiseCalibration( testRunner );
                if ( enablefw != 0 ) {
                    info.enable_fw_haptics.emplace( enablefw );
                } else {
                    errors.emplace_back(L"Could not read initial calibration" );
                    success = false;
                }
            }

            if ( job.store_integral_values && success ) {
                auto store = storeIntegralValues( testRunner );
                if ( !store.empty() ) {
                    info.store_integral_values.emplace( store );
                } else {
                    errors.emplace_back(L"Could not read store Calibration" );
                    success = false;
                }
            }

            if ( job.readstiffestim && success ) {
                auto readcal = ReadStiffestim( testRunner );
                if ( !readcal.empty() ) {
                    info.read_stiffestim.emplace( readcal );
                } else {
                    errors.emplace_back(L"Could not read stiffestim" );
                    success = false;
                }
            }

            if ( job.readcapsenseflag && success ) {
                auto readcal = ReadCapsenseFlag( testRunner );
                if ( !readcal.empty() ) {
                    info.read_capsense_flag.emplace( readcal );
                } else {
                    errors.emplace_back(L"Could not read capsense flag" );
                    success = false;
                }
            }

            if ( job.initpiezofoiltest && success ) {
                auto initPiezo = InitialPiezoFoilTest( testRunner );
                if ( initPiezo != 0 ) {
                    info.init_piezo_foil_test.emplace( initPiezo );
                } else {
                    errors.emplace_back(L"initial piezo foil test done!" );
                    success = false;
                }
            }

            if ( job.test_piezos && success ) {
                auto taus = TestPeizos( testRunner );
                if ( !taus.empty() ) {
                    info.test_piezos.emplace( taus );
                } else {
                    errors.emplace_back(L"Could not test piezo taus" );
                    success = false;
                }
            }

            if ( job.test_piezo_taus && success ) {
                auto taus = TestPeizoTaus( testRunner );
                if ( !taus.empty() ) {
                    info.test_piezo_taus.emplace( taus );
                } else {
                    errors.emplace_back(L"Could not test piezo taus" );
                    success = false;
                }
            }

            if ( job.return_true && success ) {
                auto returntrue = ReturnTrue( testRunner );
                if ( returntrue ) {
                    info.return_true.emplace( returntrue );
                } else {
                    errors.emplace_back(L"Could not return True" );
                    success = false;
                }
            }
            if ( job.return_false && success ) {
                auto returnfalse = ReturnFalse( testRunner );
                if ( returnfalse ) {
                    info.return_false.emplace( returnfalse );
                } else {
                    errors.emplace_back(L"Could not return True" );
                    success = false;
                }
            }

            if ( job.go_to_active_idle && success ) {
                auto store = GoToActiveIdle( testRunner );
                if ( !store.empty() ) {
                    info.go_to_active_idle.emplace( store );
                } else {
                    errors.emplace_back(L"Could not read store Calibration" );
                    success = false;
                }
            }

            if ( job.erase_nvm_page && success ) {
                auto erase_nvm_page = EraseNvmPage( testRunner );
                if ( erase_nvm_page != 0 ) {
                    info.playhaptics.emplace( erase_nvm_page );
                } else {
                    errors.emplace_back(L"Could not erase nvm page" );
                    success = false;
                }
            }

            if ( job.reset && success ) {
                auto reset = Reset( testRunner );
                if ( reset != 0 ) {
                    info.playhaptics.emplace( reset );
                } else {
                    errors.emplace_back(L"Could not reset" );
                    success = false;
                }
            }

            if ( job.test_booster && success ) {
                auto test_booster = TestBooster( testRunner );
                if ( !test_booster.empty() ) {
                    info.test_booster.emplace( test_booster );
                } else {
                    errors.emplace_back(L"Could not test booster" );
                    success = false;
                }
            }

            if ( job.chipSerNr && success ) {
                serNo = ReadControllerSerialNumber( testRunner );
                if ( !serNo.empty() ) {
                    wstring elModId = L"";
                    info.serialNumber.emplace( serNo );
                    if ( serNo.length() >= 3 ) {
                        endOfSerNo = serNo.substr( serNo.length() - 3, 3 );
                        if ( endOfSerNo.compare( L"A01" ) == 0 ||
                             endOfSerNo.compare( L"A02" ) == 0 ) {
                            elModId = L"DEL008-E02-08";
                        } else if ( endOfSerNo.compare( L"A00" ) == 0 ||
                                    endOfSerNo.compare( L"X00" ) == 0 ||
                                    endOfSerNo.compare( L"X01" ) == 0 ||
                                    endOfSerNo.compare( L"X10" ) == 0 ||
                                    endOfSerNo.compare( L"X20" ) ==
                                        0 ) { // DVT builds
                            elModId = L"DEL008-E02-07";
                        } else {
                            endOfSerNo = L"";
                        }
                    }
                    info.electronicsModuleId.emplace( elModId );
                    info.serialNumberSuffix.emplace( endOfSerNo );
                }
            }

            if ( job.asicSerNr && success ) {
                auto asicSerNo = ReadAsicSerialNumber( testRunner );
                if ( !asicSerNo.empty() ) {
                    // size of array with serial numbers of asic(s) is used to
                    // calculate asics amount
                    info.calculateAsicsAmount( asicSerNo );
                    info.asicSerialNumber.emplace( asicSerNo );
                } else {
                    errors.emplace_back(L"Could not read the ASIC serial number" );
                    success = false;
                }
            }

            if ( job.bootVer && success ) {
                info.bootloaderInfo = ReadBootloaderInfo( testRunner );
                if ( !info.bootloaderInfo.has_value() ) {
                    // Not an error because this is expected with old
                    // bootloaders
                    Log( L"Could not query the bootloader for information" );

                    // Fall back to reading the commit ID from the
                    // application.
                    if ( !SetSelfTestModeOn( testRunner ) ) {
                        errors.emplace_back(
                            L"Could not enter self-test mode to read the "
                            L"bootloader version" );
                        success = false;
                    } else {
                        if ( auto bootloaderVersion = ReadBootloaderVersion( testRunner );
                             !bootloaderVersion.empty() ) {
                            info.bootloaderInfo.emplace();
                            info.bootloaderInfo->commitId = bootloaderVersion;
                        } else {
                            errors.emplace_back(L"Could not read the bootloader version" );
                            success = false;
                        }
                    }
                }
            }

            // ReadFirmwareVersion will re-enter the self-test mode if we've
            // left it after resetting into the bootloader above
            if ( job.fwVer && success ) {
                info.firmwareBuildId = ReadFirmwareVersion( testRunner );
            }

            if ( job.displayResultsFromNvm &&
                 success ) { // read self-test results from NVM and display them
                info.nvmContents = ReadFromNvm( testRunner );
                if ( info.nvmContents.empty() ) {
                    errors.emplace_back( L"Could not read the NVM data" );
                    success = false;
                }
            }
            if ( job.read_stored_integrals &&
                 success ) { // read self-test results from NVM and display them
                info.read_stored_integrals = ReadFromNvm( testRunner );
                if ( info.read_stored_integrals.empty() ) {
                    errors.emplace_back( L"Could not read the NVM data" );
                    success = false;
                }
            }
            if ( job.return_haptic_xy && success ) {
                auto return_haptic_xy = ReturnHapticsXYOne( testRunner );
                if ( !return_haptic_xy.empty() ) {
                    info.return_haptic_xy.emplace( return_haptic_xy );
                } else {
                    errors.emplace_back( L"Could return haptic xy" );
                    success = false;
                }
            }

         

            if ( job.displayTestResultsFromNvm &&
                 success ) { // read self-test results from NVM and display them
                errors.emplace_back( L"Reading self-test results from the NVM is not implemented" );
            }
            if ( job.run_afpt && success ) {
                auto runafpt = RunAfpt( testRunner );
                if ( !runafpt.empty() ) {
                    info.run_afpt.emplace( runafpt );
                } else {
                    errors.emplace_back( L"Could not run afpt" );
                    success = false;
                }
            }
        }
        FormatRequests( job.displayFormat, info, result );
        for ( auto const &error : errors ) {
            result << L"Error: " << error << endl;
        }
    }
    Log( L"Jobs done, success = %d", success );
    static const wchar_t *baseFolderName = L".\\";
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    auto resultStr = converter.to_bytes( result.str() );
    if ( job.test_piezo_taus ) { // write DUT based report file
        wostringstream filePath;
        if ( !serNo.empty() ) {
            filePath << baseFolderName << serNo << ".txt";
        } else {
            filePath << baseFolderName << "UnknownSerialNumber.txt";
        }
        WriteTestReport( filePath.str(), resultStr );
    }
    if ( job.toFile ) {
        wostringstream filePath;
        filePath << baseFolderName << L"Aito-"<< MakeTimestamp( L"%Y%m%d-%H%M%S" ) << ".txt";
        WriteTestReport( filePath.str(), resultStr );
    } else {
        wcout << result.str();
    }
    return success;
}


static void Usage( const char *exe ) {
    wcout << L"Usage: " << exe;
    wcout << endl;
    wcout << L"  /?                             - Display this help." << endl;
    wcout << L"  /all                           - Display all info. (Default)" << endl;
    wcout << L"  get_api_verison                - Get the API version."<< endl;
    wcout << L"  change_parameters              - Changes parameter in the firware." << endl;
    wcout << L"  read_controller_id             - Display chip model." << endl;
    wcout << L"  read_aito_controller_sn        - Display chip serial number." << endl;
    wcout << L"  read_asic_serial_number        - Display ASIC serial number." << endl;
    wcout << L"  read_build_id                  - Display firmware version."  << endl;
    wcout << L"  read_vdd_voltage               - Read and display the VDD voltage." << std::endl;
    wcout << L"  read_vreff_voltage             - Read and display the VREFF voltage." << std::endl;
    wcout << L"  read_booster_voltage           - Read and display the booster voltage." << std::endl;
    wcout << L"  read_piezo_adc_voltages        - Read and display the piezo ADC voltages." << std::endl;
    wcout << L"  read_temperature               - Read and display the device  temperature." << std::endl;
    wcout << L"  start_integral_storage         - Start integral storage." << std::endl;
    wcout << L"  read_calibration_row           - Read and display calibration row." << std::endl;
    wcout << L"  play_haptics                   - Play haptics." << std::endl;
    wcout << L"  disable_fw_haptics             - Disable firmware haptics."  << endl;
    wcout << L"  enable_fw_haptics              - Enable firmware haptics." << endl;
    wcout << L"  store_integral_values          - Store integral values."  << endl;
    wcout << L"  init_piezo_foil_test           - Initialize piezo foil test." << endl;
    wcout << L"  read_calibration               - Read and display calibration."  << endl;
    wcout << L"  read_capsense_flag             - Read and display capsense flag." << endl;
    wcout << L"  test_piezo_taus                - Test piezo taus." << endl;
    wcout << L"  test_piezos                    - Returns information about the piezos (Distortion  Capacitance  Stiffness) ." << endl;
    wcout << L"  test_booster                   - Test booster." << endl;
    wcout << L"  return_haptic_xy               - Returns haptics at active press points." << endl;
    wcout << L"  return_haptic_xy_1             - Returns haptics at current press points." << endl;
    wcout << L"  return_haptic_xy_2             - Returns haptics at current press/release points." << endl;
    wcout << L"  return_true                    - Return true." << endl;
    wcout << L"  return_false                   - Return false." << endl;
    wcout << L"  go_to_active_idle              - Go to active idle state." << endl;
    wcout << L"  read_nvm_page                  - Read NVM page." << endl;
    wcout << L"  read_stored_integrals          - Read store integral." << endl;
    wcout << L"  erase_nvm_page                 - Erase NVM page." << endl;
    wcout << L"  reset                          - Reset the device." << endl;
    wcout << L"  run_afpt                       - Run AFPT (Advanced Functional Performance Test)." << endl;
}

int main( int argc, char *argv[] ) {
    (void)_setmode( _fileno( stdin ), _O_U16TEXT );
    (void)_setmode( _fileno( stdout ), _O_U16TEXT );
    (void)_setmode( _fileno( stderr ), _O_U16TEXT );
    wstring exeVersion( GetExecutableVersion() );
    Log( L"Aito%s started %s", devSpec.appName.c_str() ,exeVersion.c_str() );

    // wcout << L"Aito Haptic " << devSpec.appName << L' ' << exeVersion <<
    // endl; wcout << L"Copyright \u00a9 2024 Aito BV. All rights reserved." <<
    // endl << endl;
    /* Ignore program name. */
    argc--; const char *exeName = *argv++;

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    Job job;
    auto success = false;
    if constexpr ( AITO_COMMANDER_READ_VERSION ) {
        job.fwVer = true;
        return RunRequests( job ) ? 0 : 1;
    }

    if ( strcmp( argv[0], "return_haptic_xy" ) == 0 ) {
        job.return_haptic_xy = true;
        auto success = true;
        while ( success ) {
            auto result = ReturnHapticsXY( job );
            if ( result.empty() ) {
                success = false;
                break;
            }
            wcout << result << endl;
            Log( L"Result: %s", result.c_str() );
        }
        std::this_thread::sleep_for( std::chrono::seconds( 5 ) );
        return success ? 0 : 1;
    }

    if ( strcmp( argv[0], "return_haptic_xy_2" ) == 0 ) {
        job.return_haptic_xy = true;
        auto success = true;
        for ( size_t i = 0; i < 2; i++ ) {
            auto result = ReturnHapticsXY( job );
            if ( result.empty() ) {
                success = false;
                break;
            }
            wcout << result << endl;
            Log( L"Result: %s", result.c_str() );
        }
        return success ? 0 : 1;
    }

    if ( argc > 0 && strcmp( argv[0], "get_api_verison" ) == 0 ) {
        wstring exeVersions( GetExecutableVersion() );
        wcout << L"Aito" << devSpec.appName.c_str() << exeVersions.c_str() << endl;
        return 0;
    }

    if ( argc > 0 && strcmp( argv[0], "change_parameters" ) == 0 ) {
        if ( argc ==3 ) { // Ensure there are exactly four command-line arguments
            try {
                vector<int> values;
                for ( int i = 0; i < 2; i++ ) {
                    argc--, argv++;
                    try {
                        values.emplace_back( std::stoi( argv[0] ) );
                    } catch ( std::invalid_argument const & ) {
                        wcout << L"Invalid argument: " << argv[0] << endl;
                    } catch ( std::out_of_range const & ) {
                        wcout << L"Argument out of range: " << argv[0] << endl;
                    }
                }
                job.changeableParam = values.at( 0 );
                job.value = values.at( 1 );
                auto success = ChangeParamJob( job ); // Run the change parameter job
                if ( !success ) {
                    Log( L"could not perform changeable parameters" );
                }
                return success ? 0 : 1;
            } catch ( const std::invalid_argument &e ) {
                std::cerr << "Error: Invalid argument for conversion to uint8_t." << e.what() << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Error: Incorrect number of command line arguments." << std::endl;
            return 1;
        }
    }
    while ( argc ) {
        // Parse Selftest specific flags
         if ( strcmp( argv[0], "/all" ) == 0 ) {
            job.chipModel = true;
            job.chipSerNr = true;
            job.asicSerNr = true;
            job.bootVer = true;
            job.fwVer = true;
        }
         else if ( strcmp( argv[0], "read_controller_id" ) == 0 ) {
            job.chipModel = true;
        } else if ( strcmp( argv[0], "read_aito_controller_sn" ) == 0 ) {
            job.chipSerNr = true;
        } else if ( strcmp( argv[0], "read_asic_serial_number" ) == 0 ) {
            job.asicSerNr = true;
        } else if ( strcmp( argv[0], "read_build_id" ) == 0 ) {
            job.fwVer = true;
        } else if ( strcmp( argv[0], "read_bootloader_version" ) == 0 ) {
            job.bootVer = true;
            job.avoidSelfTestInterface = true;
        } else if ( strcmp( argv[0], "read_vdd_voltage" ) == 0 ) {
            job.vddVoltage = true;
        } else if ( strcmp( argv[0], "read_vreff_voltage" ) == 0 ) {
            job.read_vreff_voltage = true;
        } else if ( strcmp( argv[0], "read_booster_voltage" ) == 0 ) {
            job.read_booster_voltage = true;
        } else if ( strcmp( argv[0], "read_piezo_adc_voltages" ) == 0 ) {
            job.read_piezo_adc_voltages = true;
        } else if ( strcmp( argv[0], "read_temperature" ) == 0 ) {
            job.read_temperature = true;
        } else if ( strcmp( argv[0], "start_integral_storage" ) == 0 ) {
            job.start_integral_storage = true;
        } else if ( strcmp( argv[0], "read_calibration_row" ) == 0 ) {
            job.read_calibration = true;
        } else if ( strcmp( argv[0], "play_haptics" ) == 0 ) {
            job.play_haptics = true;
        } else if ( strcmp( argv[0], "disable_fw_haptics" ) == 0 ) {
            job.disable_fw_haptics = true;
        } else if ( strcmp( argv[0], "enable_fw_haptics" ) == 0 ) {
            job.enable_fw_haptics = true;
        } else if ( strcmp( argv[0], "store_integral_values" ) == 0 ) {
            job.store_integral_values = true;
        } else if ( strcmp( argv[0], "init_piezo_foil_test" ) == 0 ) {
            job.initpiezofoiltest = true;
        } else if ( strcmp( argv[0], "read_calibration" ) == 0 ) {
            job.readstiffestim = true;
        } else if ( strcmp( argv[0], "read_capsense_flag" ) == 0 ) {
            job.readcapsenseflag = true;
        } else if ( strcmp( argv[0], "test_piezos" ) == 0 ) {
            job.test_piezos = true;
        } else if ( strcmp( argv[0], "test_piezo_taus" ) == 0 ) {
            job.test_piezo_taus = true;
        } else if ( strcmp( argv[0], "test_booster" ) == 0 ) {
            job.test_booster = true;
        } else if ( strcmp( argv[0], "return_true" ) == 0 ) {
            job.return_true = true;
        } else if ( strcmp( argv[0], "return_false" ) == 0 ) {
            job.return_false = true;
        } else if ( strcmp( argv[0], "go_to_active_idle" ) == 0 ) {
            job.go_to_active_idle = true;
        } else if ( strcmp( argv[0], "erase_nvm_page" ) == 0 ) {
            job.erase_nvm_page = true;
        } else if ( strcmp( argv[0], "reset" ) == 0 ) {
            job.reset = true;
        } else if ( strcmp( argv[0], "run_afpt" ) == 0 ) {
            job.run_afpt = true;
        } else if ( strcmp( argv[0], "read_nvm_page" ) == 0 ) {
            job.displayResultsFromNvm = true;
            if ( job.displayFormat == DisplayFormat::notDefined ) {
                job.displayFormat = DisplayFormat::human;}
        } else if ( strcmp( argv[0], "read_stored_integrals" ) == 0 ) {
            job.read_stored_integrals = true;
        } else if ( strcmp( argv[0], "return_haptic_xy_1" ) == 0 ) {
            job.return_haptic_xy = true;
        } else { Usage( exeName );
            return 0;
        }
        argc--, argv++;
    }
    success = RunRequests( job );
    return success ? 0 : 1;
}
