// version.h
// Firmware identity and communication-baud constants.
//
// Included by controller.h so every .cpp that pulls in the controller
// header automatically gets these definitions.  FIRMWARE_NAME, VERSION_NUM,
// BUILD_NUM, and BUILD_DATE are used by AmidalaConsole; RDH_BAUD_RATE is used
// by AmidalaController::setup() and the .ino entry point.

#pragma once

#define FIRMWARE_NAME   F("Amidala RC")
#define VERSION_NUM     F("1.3")
#define BUILD_NUM       F("1")
#define BUILD_DATE      F(__DATE__)

#define RDH_BAUD_RATE   9600
