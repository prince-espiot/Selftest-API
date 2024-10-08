##Introduction
This page describes various things about the Application Interface (API) that provides a testing interface for production testing tools.
The API is based on the individual commands and test programs that can be run in a command prompt against a .exe program.
#Prerequisites
Production Testing API is based on C++ scripts. The program is well integrated and as such that do not require any library or dependences.
#API content
The first phase implementation of the API contains six-point press test for 12-piezo devices, and four-point press test for 10-piezo devices in addition to the individual self-test-commands, like read serial numbers and versions.
•
This file describes the API content in html and pdf format.
•
Release-folder contains API as an .exe program file.
•
src/selftest_settings/Test_info contains the commands that have been implemented in the API.
•
src/test_programs contain textual program is both windows batch and bash format for Linux environment.
The API can be used by running “.exe” program “SelfTest-API.exe” from the folder “Release”.
An example of running a part of the 6-point press test:
D:\Test\Selftest-API\src> Set-PressPoint1.sh / Set-PressPoint1.bat
D:\Test\Selftest-API\src> Return-PressIntegrals.sh / Return-PressIntegrals.bat
12 10 11 9 65535 1270 430 33521

##Test Programs
Test programs are text files that can contain several individual self-test commands conforming a test sequence. They exist in the folder src\test_programs.
The command syntax, when running a test program, is:
SelfTest-API.exe <command>
E.g. running the 6-point press test, from src-folder:
6-Point-PressTest.sh / 6-Point-PressTest.bat
6-point-PressTest
•
runs 6-point press test and prints the closest 4 piezo numbers and their integral values from each press point.
4
•
targeted for 12-piezo devices testing.
4-point-PressTest
•
runs 4-point press test and prints the closest 4 piezo numbers and their integral values from each press point.
•
targeted for 10-piezo devices testing.
Set-PressPoint1 … 6
•
Sets location parameters for an individual press point
•
Can be used together with Return-PressIntegrals, when the tester synchronizes press event reading with the actuator function.
Return-PressIntegrals
•
Prints the closest 4 piezo numbers and the integrals values of the set individual press location.
9-point-XY-Press-test
•
runs 9-point press test and prints the event (press = 1, release = 0) and XY coordinate values
•
targeted for any device.
Return-XY-Press
•
Prints the event (press = 1, release = 0), X and Y coordinate values of one press event
