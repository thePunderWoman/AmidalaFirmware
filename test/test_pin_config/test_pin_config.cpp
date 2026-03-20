// test_pin_config.cpp
// Compilation + value tests for include/pin_config.h.
//
// These tests confirm that every pin constant has the expected numeric value.
// A failure here means a pin number was accidentally changed; a compile error
// means a constant was removed entirely. Both are regressions we want to catch.

#include "arduino_mock.h"
#include "pin_config.h"
#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

// ---- Servo output pins ------------------------------------------------------

void test_servo_pins_sequential_from_2() {
    TEST_ASSERT_EQUAL(2,  SERVO1_PIN);
    TEST_ASSERT_EQUAL(3,  SERVO2_PIN);
    TEST_ASSERT_EQUAL(4,  SERVO3_PIN);
    TEST_ASSERT_EQUAL(5,  SERVO4_PIN);
    TEST_ASSERT_EQUAL(6,  SERVO5_PIN);
    TEST_ASSERT_EQUAL(7,  SERVO6_PIN);
    TEST_ASSERT_EQUAL(8,  SERVO7_PIN);
    TEST_ASSERT_EQUAL(9,  SERVO8_PIN);
    TEST_ASSERT_EQUAL(10, SERVO9_PIN);
    TEST_ASSERT_EQUAL(11, SERVO10_PIN);
    TEST_ASSERT_EQUAL(12, SERVO11_PIN);
    TEST_ASSERT_EQUAL(13, SERVO12_PIN);
}

// ---- Digital output pins ----------------------------------------------------

void test_dout_pins_sequential_from_22() {
    TEST_ASSERT_EQUAL(22, DOUT1_PIN);
    TEST_ASSERT_EQUAL(23, DOUT2_PIN);
    TEST_ASSERT_EQUAL(24, DOUT3_PIN);
    TEST_ASSERT_EQUAL(25, DOUT4_PIN);
    TEST_ASSERT_EQUAL(26, DOUT5_PIN);
    TEST_ASSERT_EQUAL(27, DOUT6_PIN);
    TEST_ASSERT_EQUAL(28, DOUT7_PIN);
    TEST_ASSERT_EQUAL(29, DOUT8_PIN);
}

void test_drive_active_pin_is_dout7() {
    TEST_ASSERT_EQUAL(DOUT7_PIN, DRIVE_ACTIVE_PIN);
    TEST_ASSERT_EQUAL(28, DRIVE_ACTIVE_PIN);
}

void test_dome_active_pin_is_dout8() {
    TEST_ASSERT_EQUAL(DOUT8_PIN, DOME_ACTIVE_PIN);
    TEST_ASSERT_EQUAL(29, DOME_ACTIVE_PIN);
}

// ---- PPM input --------------------------------------------------------------

void test_ppmin_pin() {
    TEST_ASSERT_EQUAL(49, PPMIN_PIN);
}

// ---- Analog inputs ----------------------------------------------------------

void test_analog_pins_alias_arduino_constants() {
    TEST_ASSERT_EQUAL(A0, ANALOG1_PIN);
    TEST_ASSERT_EQUAL(A1, ANALOG2_PIN);
    // Confirm the mock values match the Mega 2560 mapping
    TEST_ASSERT_EQUAL(54, ANALOG1_PIN);
    TEST_ASSERT_EQUAL(55, ANALOG2_PIN);
}

// ---- Mode select pins -------------------------------------------------------

void test_rc_select_pins() {
    TEST_ASSERT_EQUAL(30, RCSEL_PIN);
    TEST_ASSERT_EQUAL(31, SEL2_PIN);
}

// ---- Status indicator pins --------------------------------------------------

void test_status_pins_sequential_from_32() {
    TEST_ASSERT_EQUAL(32, STATUS_J1_PIN);
    TEST_ASSERT_EQUAL(33, STATUS_J2_PIN);
    TEST_ASSERT_EQUAL(34, STATUS_RC_PIN);
    TEST_ASSERT_EQUAL(35, STATUS_S1_PIN);
    TEST_ASSERT_EQUAL(36, STATUS_S2_PIN);
    TEST_ASSERT_EQUAL(37, STATUS_S3_PIN);
    TEST_ASSERT_EQUAL(38, STATUS_S4_PIN);
}

// ---- I2C bus pins -----------------------------------------------------------

void test_i2c_pins() {
    TEST_ASSERT_EQUAL(20, I2C_SDA_PIN);
    TEST_ASSERT_EQUAL(21, I2C_SCL_PIN);
}

// ---- AUX_SERIAL conditional -------------------------------------------------

void test_aux_serial_defined_when_drive_serials_absent() {
    // In the native test environment neither DOME_DRIVE_SERIAL nor RDH_SERIAL
    // is defined, so pin_config.h must define AUX_SERIAL.
#ifndef AUX_SERIAL
    TEST_FAIL_MESSAGE("AUX_SERIAL must be defined when drive serials are absent");
#else
    TEST_PASS();
#endif
}

// ---- VMUSIC_SERIAL must remain commented out by default ---------------------

void test_vmusic_serial_not_defined_by_default() {
    // VMUSIC_SERIAL is intentionally disabled by default. If it were
    // accidentally uncommented this test would fail, prompting the engineer to
    // also update the hcr.h include ordering in the .ino.
#ifdef VMUSIC_SERIAL
    TEST_FAIL_MESSAGE("VMUSIC_SERIAL should be undefined by default");
#else
    TEST_PASS();
#endif
}

// ---- main -------------------------------------------------------------------

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();

    RUN_TEST(test_servo_pins_sequential_from_2);

    RUN_TEST(test_dout_pins_sequential_from_22);
    RUN_TEST(test_drive_active_pin_is_dout7);
    RUN_TEST(test_dome_active_pin_is_dout8);

    RUN_TEST(test_ppmin_pin);
    RUN_TEST(test_analog_pins_alias_arduino_constants);
    RUN_TEST(test_rc_select_pins);
    RUN_TEST(test_status_pins_sequential_from_32);
    RUN_TEST(test_i2c_pins);

    RUN_TEST(test_aux_serial_defined_when_drive_serials_absent);
    RUN_TEST(test_vmusic_serial_not_defined_by_default);

    return UNITY_END();
}
