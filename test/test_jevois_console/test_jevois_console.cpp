// test_jevois_console.cpp
// Tests for include/jevois_console.h:
//   JevoisConsole — construction and type smoke-test (EXPERIMENTAL_JEVOIS_STEERING)
//
// Method bodies call AmidalaController / hardware globals and are defined in
// AmidalaFirmware.ino; stubs satisfy the linker for the native test build.

#define EXPERIMENTAL_JEVOIS_STEERING

#include "arduino_mock.h"
#include "jevois_console.h"
#include <unity.h>

// ---- Linker stubs -----------------------------------------------------------

void JevoisConsole::init(AmidalaController *) {}
void JevoisConsole::processCommand(char *) {}
void JevoisConsole::process() {}

// -----------------------------------------------------------------------------

void setUp(void) {}
void tearDown(void) {}

// ---- Construction -----------------------------------------------------------

void test_jevois_console_constructs() {
    JevoisConsole j;
    (void)j;
}

// ---- nullptr controller pointer is safe to set --------------------------------

void test_jevois_console_fcontroller_default_null() {
    // Accessing fController is private; we just verify construction doesn't
    // crash and the object is valid enough to call a (stubbed) method on.
    JevoisConsole j;
    j.init(nullptr);
}

// ---- main -------------------------------------------------------------------

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();

    RUN_TEST(test_jevois_console_constructs);
    RUN_TEST(test_jevois_console_fcontroller_default_null);

    return UNITY_END();
}
