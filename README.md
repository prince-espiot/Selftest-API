# Production Testing API

## Introduction

This API provides a testing interface for production testing tools. It is based on individual commands and test programs that can be run in a command prompt against an .exe program.

## Prerequisites

The Production Testing API is based on C++ scripts. The program is well integrated and does not require any additional libraries or dependencies.

## API Content

The first phase implementation of the API includes:
- Six-point press test for 12-piezo devices
- Four-point press test for 10-piezo devices
- Individual self-test commands (e.g., read serial numbers and versions)

### Repository Structure

- `README.md`: This file (API description in markdown format)
- `Release/`: Contains the API as an .exe program file
- `src/selftest_settings/Test_info/`: Contains the commands implemented in the API
- `src/test_programs/`: Contains textual programs in both Windows batch and Linux bash formats

## Usage

The API can be used by running the "SelfTest-API.exe" program from the `Release` folder.

Example of running part of the 6-point press test:
D:\Test\Selftest-API\src> Set-PressPoint1.sh / Set-PressPoint1.bat
D:\Test\Selftest-API\src> Return-PressIntegrals.sh / Return-PressIntegrals.bat
12 10 11 9 65535 1270 430 33521

## Test Programs

Test programs are text files containing several individual self-test commands forming a test sequence. They are located in the `src/test_programs` folder.

Command syntax:
Example (running the 6-point press test from the src folder):

### Available Test Programs

1. **6-point-PressTest**
   - Runs 6-point press test
   - Prints the closest 4 piezo numbers and their integral values from each press point
   - Targeted for 12-piezo devices testing

2. **4-point-PressTest**
   - Runs 4-point press test
   - Prints the closest 4 piezo numbers and their integral values from each press point
   - Targeted for 10-piezo devices testing

3. **Set-PressPoint1 ... 6**
   - Sets location parameters for an individual press point
   - Can be used with Return-PressIntegrals when the tester synchronizes press event reading with the actuator function

4. **Return-PressIntegrals**
   - Prints the closest 4 piezo numbers and the integral values of the set individual press location

5. **9-point-XY-Press-test**
   - Runs 9-point press test
   - Prints the event (press = 1, release = 0) and XY coordinate values
   - Targeted for any device

6. **Return-XY-Press**
   - Prints the event (press = 1, release = 0), X and Y coordinate values of one press event
