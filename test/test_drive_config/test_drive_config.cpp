// test_drive_config.cpp
// Compilation + value tests for include/drive_config.h.
//
// Tests run against the default configuration (DRIVE_SYSTEM_ROBOTEQ_PWM,
// DOME_DRIVE_PWM). Any accidental change to a constant or the active
// selection will be caught here before it reaches the embedded build.

#include "arduino_mock.h"
#include "drive_config.h"
#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

// ---- Drive system type constants --------------------------------------------

void test_drive_system_type_constants() {
    TEST_ASSERT_EQUAL(1, DRIVE_SYSTEM_PWM);
    TEST_ASSERT_EQUAL(2, DRIVE_SYSTEM_SABER);
    TEST_ASSERT_EQUAL(3, DRIVE_SYSTEM_ROBOTEQ_PWM);
    TEST_ASSERT_EQUAL(4, DRIVE_SYSTEM_ROBOTEQ_SERIAL);
    TEST_ASSERT_EQUAL(5, DRIVE_SYSTEM_ROBOTEQ_PWM_SERIAL);
}

void test_drive_system_default_selection() {
    // Default must be ROBOTEQ_PWM. A change here requires a deliberate edit
    // to drive_config.h and a matching update to this test.
    TEST_ASSERT_EQUAL(DRIVE_SYSTEM_ROBOTEQ_PWM, DRIVE_SYSTEM);
    TEST_ASSERT_EQUAL(3, DRIVE_SYSTEM);
}

// ---- Dome drive type constants -----------------------------------------------

void test_dome_drive_type_constants() {
    TEST_ASSERT_EQUAL(2, DOME_DRIVE_SABER);
    TEST_ASSERT_EQUAL(3, DOME_DRIVE_PWM);
    TEST_ASSERT_EQUAL(4, DOME_DRIVE_ROBOCLAW);
}

void test_dome_drive_default_selection() {
    TEST_ASSERT_EQUAL(DOME_DRIVE_ROBOCLAW, DOME_DRIVE);
    TEST_ASSERT_EQUAL(4, DOME_DRIVE);
}

// ---- Speed and acceleration -------------------------------------------------

void test_speed_constants() {
    TEST_ASSERT_EQUAL_FLOAT(1.0f, MAXIMUM_SPEED);
    TEST_ASSERT_EQUAL_FLOAT(0.5f, MAXIMUM_GUEST_SPEED);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, DOME_MAXIMUM_SPEED);
}

void test_acceleration_constants() {
    TEST_ASSERT_EQUAL(100, ACCELERATION_SCALE);
    TEST_ASSERT_EQUAL(20,  DECELRATION_SCALE);
}

void test_scaling_disabled_by_default() {
    TEST_ASSERT_FALSE(SCALING);
}

// ---- CHANNEL_MIXING derived from default drive system -----------------------

void test_channel_mixing_false_for_roboteq_pwm() {
    // ROBOTEQ_PWM uses a motor controller that handles mixing internally.
    TEST_ASSERT_FALSE(CHANNEL_MIXING);
}

// ---- Dome motion defaults ---------------------------------------------------

void test_dome_position_defaults() {
    TEST_ASSERT_EQUAL(50, DEFAULT_DOME_HOME_POSITION);
}

void test_dome_acceleration_defaults() {
    TEST_ASSERT_EQUAL(20, DEFAULT_DOME_ACCELERATION_SCALE);
    TEST_ASSERT_EQUAL(50, DEFAULT_DOME_DECELERATION_SCALE);
}

void test_dome_delay_defaults() {
    TEST_ASSERT_EQUAL(6, DEFAULT_DOME_HOME_MIN_DELAY);
    TEST_ASSERT_EQUAL(8, DEFAULT_DOME_HOME_MAX_DELAY);
    TEST_ASSERT_EQUAL(6, DEFAULT_DOME_SEEK_MIN_DELAY);
    TEST_ASSERT_EQUAL(8, DEFAULT_DOME_SEEK_MAX_DELAY);
    TEST_ASSERT_EQUAL(0, DEFAULT_DOME_TARGET_MIN_DELAY);
    TEST_ASSERT_EQUAL(1, DEFAULT_DOME_TARGET_MAX_DELAY);
}

void test_dome_seek_range_defaults() {
    TEST_ASSERT_EQUAL(80, DEFAULT_DOME_SEEK_LEFT);
    TEST_ASSERT_EQUAL(80, DEFAULT_DOME_SEEK_RIGHT);
    TEST_ASSERT_EQUAL(2,  DEFAULT_DOME_FUDGE);
}

void test_dome_speed_defaults() {
    TEST_ASSERT_EQUAL(40,  DEFAULT_DOME_SPEED_HOME);
    TEST_ASSERT_EQUAL(60,  DEFAULT_DOME_SPEED_SEEK);
    TEST_ASSERT_EQUAL(100, DEFAULT_DOME_SPEED_TARGET);
    TEST_ASSERT_EQUAL(15,  DEFAULT_DOME_SPEED_MIN);
    TEST_ASSERT_EQUAL(5,   DEFAULT_DOME_TIMEOUT);
}

void test_dome_inversion_and_pulse_defaults() {
    TEST_ASSERT_FALSE(DEFAULT_DOME_INVERTED);
    TEST_ASSERT_EQUAL(1000, DEFAULT_DOME_MIN_PULSE);
    TEST_ASSERT_EQUAL(2000, DEFAULT_DOME_MAX_PULSE);
}

// ---- RoboClaw dome drive defaults -------------------------------------------

void test_roboclaw_dome_constants() {
    TEST_ASSERT_EQUAL(128, DEFAULT_DOME_ROBOCLAW_ADDRESS);
    TEST_ASSERT_EQUAL(1,   DEFAULT_DOME_ROBOCLAW_CHANNEL);
    TEST_ASSERT_EQUAL(800, DEFAULT_DOME_ROBOCLAW_QPPS);
    TEST_ASSERT_EQUAL(1200, DOME_MOTOR_CPR);
    TEST_ASSERT_EQUAL(500, DEFAULT_DOME_STALL_TIMEOUT_MS);
    TEST_ASSERT_EQUAL(15000, DEFAULT_DOME_HOMING_TIMEOUT_MS);
    TEST_ASSERT_EQUAL(10,  DOME_CALIBRATION_ROTATIONS);
    TEST_ASSERT_EQUAL(50,  DEFAULT_DOME_HALL_DEBOUNCE_MS);
    TEST_ASSERT_EQUAL(0x200, DOME_ROBOCLAW_EEPROM_ADDR);
    TEST_ASSERT_EQUAL_FLOAT(0.20f, DEFAULT_DOME_HOMING_SPEED);
    TEST_ASSERT_EQUAL_FLOAT(0.30f, DEFAULT_DOME_CALIBRATE_SPEED);
}

void test_roboclaw_eeprom_signature_bytes() {
    TEST_ASSERT_EQUAL('R', DOME_ROBOCLAW_EEPROM_SIG0);
    TEST_ASSERT_EQUAL('C', DOME_ROBOCLAW_EEPROM_SIG1);
    TEST_ASSERT_EQUAL('0', DOME_ROBOCLAW_EEPROM_SIG2);
    TEST_ASSERT_EQUAL('1', DOME_ROBOCLAW_EEPROM_SIG3);
}

// ---- Derived serial defines absent for default config -----------------------

void test_drive_serial_not_defined_for_roboteq_pwm() {
    // DRIVE_SERIAL is only set for DRIVE_SYSTEM_SABER. With ROBOTEQ_PWM it
    // must not be defined so the build does not accidentally reference Serial3.
#ifdef DRIVE_SERIAL
    TEST_FAIL_MESSAGE("DRIVE_SERIAL should not be defined for ROBOTEQ_PWM");
#else
    TEST_PASS();
#endif
}

void test_dome_drive_serial_not_defined_for_pwm_dome() {
    // DOME_DRIVE_SERIAL is only set for the Sabertooth dome path.
#ifdef DOME_DRIVE_SERIAL
    TEST_FAIL_MESSAGE("DOME_DRIVE_SERIAL should not be defined for DOME_DRIVE_PWM");
#else
    TEST_PASS();
#endif
}

// ---- main -------------------------------------------------------------------

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();

    RUN_TEST(test_drive_system_type_constants);
    RUN_TEST(test_drive_system_default_selection);

    RUN_TEST(test_dome_drive_type_constants);
    RUN_TEST(test_dome_drive_default_selection);

    RUN_TEST(test_speed_constants);
    RUN_TEST(test_acceleration_constants);
    RUN_TEST(test_scaling_disabled_by_default);
    RUN_TEST(test_channel_mixing_false_for_roboteq_pwm);

    RUN_TEST(test_dome_position_defaults);
    RUN_TEST(test_dome_acceleration_defaults);
    RUN_TEST(test_dome_delay_defaults);
    RUN_TEST(test_dome_seek_range_defaults);
    RUN_TEST(test_dome_speed_defaults);
    RUN_TEST(test_dome_inversion_and_pulse_defaults);

    RUN_TEST(test_roboclaw_dome_constants);
    RUN_TEST(test_roboclaw_eeprom_signature_bytes);

    RUN_TEST(test_drive_serial_not_defined_for_roboteq_pwm);
    RUN_TEST(test_dome_drive_serial_not_defined_for_pwm_dome);

    return UNITY_END();
}
