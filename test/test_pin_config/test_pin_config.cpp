// test_pin_config.cpp
// Compilation + value tests for include/pin_config.h on ESP32-S3.
//
// A failure here means a pin number was accidentally changed; a compile error
// means a constant was removed entirely. Both are regressions we want to catch.

#include "arduino_mock.h"
#include "pin_config.h"
#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

// ---- Servo output pins (8 channels on the Amidala custom PCB) ---------------

void test_servo_pins() {
    TEST_ASSERT_EQUAL(3,  SERVO1_PIN);
    TEST_ASSERT_EQUAL(4,  SERVO2_PIN);
    TEST_ASSERT_EQUAL(5,  SERVO3_PIN);
    TEST_ASSERT_EQUAL(6,  SERVO4_PIN);
    TEST_ASSERT_EQUAL(7,  SERVO5_PIN);
    TEST_ASSERT_EQUAL(15, SERVO6_PIN);
    TEST_ASSERT_EQUAL(16, SERVO7_PIN);
    TEST_ASSERT_EQUAL(46, SERVO8_PIN);
}

// ---- Digital output pins (6 channels) ---------------------------------------

void test_dout_pins() {
    TEST_ASSERT_EQUAL(11, DOUT1_PIN);
    TEST_ASSERT_EQUAL(12, DOUT2_PIN);
    TEST_ASSERT_EQUAL(13, DOUT3_PIN);
    TEST_ASSERT_EQUAL(14, DOUT4_PIN);
    TEST_ASSERT_EQUAL(9,  DOUT5_PIN);
    TEST_ASSERT_EQUAL(10, DOUT6_PIN);
}

// ---- Dome hall-effect sensor -------------------------------------------------

void test_dome_hall_pin() {
    TEST_ASSERT_EQUAL(14, DOME_HALL_PIN);
}

// ---- PPM RC input -----------------------------------------------------------

void test_ppmin_pin() {
    TEST_ASSERT_EQUAL(47, PPMIN_PIN);
}

// ---- Analog inputs (raw GPIO numbers, not Arduino A0/A1 aliases) ------------

void test_analog_pins() {
    TEST_ASSERT_EQUAL(1, ANALOG1_PIN);
    TEST_ASSERT_EQUAL(2, ANALOG2_PIN);
}

// ---- SD card ----------------------------------------------------------------

void test_sd_cs_pin() {
    TEST_ASSERT_EQUAL(39, SD_CS_PIN);
}

// ---- SPI bus ----------------------------------------------------------------

void test_spi_pins() {
    TEST_ASSERT_EQUAL(35, SPI_MOSI_PIN);
    TEST_ASSERT_EQUAL(36, SPI_MISO_PIN);
    TEST_ASSERT_EQUAL(37, SPI_SCK_PIN);
    TEST_ASSERT_EQUAL(40, XBEE_CS_PIN);
    TEST_ASSERT_EQUAL(41, SPI_SPARE_CS_PIN);
    TEST_ASSERT_EQUAL(42, XBEE_ATTN_PIN);
}

// ---- I2C bus ----------------------------------------------------------------

void test_i2c_pins() {
    TEST_ASSERT_EQUAL(45, I2C_SDA_PIN);
    TEST_ASSERT_EQUAL(48, I2C_SCL_PIN);
}

// ---- Mode-select pins: conditional on build flags ---------------------------

void test_rcsel_pin_undefined_by_default() {
    // RCSEL_PIN is only defined when ESP32_RCSEL_ENABLE is set.
    // Without it, GPIO12/13 serve as DOUT2/3 instead.
#ifdef RCSEL_PIN
    TEST_FAIL_MESSAGE("RCSEL_PIN should be undefined without -DESP32_RCSEL_ENABLE");
#else
    TEST_PASS();
#endif
}

void test_sel2_pin_defined_by_default() {
    // SEL2_PIN (GPIO21) is available unless AUX serial is enabled.
#ifdef SEL2_PIN
    TEST_ASSERT_EQUAL(21, SEL2_PIN);
#else
    TEST_FAIL_MESSAGE("SEL2_PIN should be defined unless -DESP32_AUX_SERIAL is set");
#endif
}

// ---- SERIAL defined when drive serials absent -------------------------------

void test_serial_defined_when_drive_serials_absent() {
#ifndef SERIAL
    TEST_FAIL_MESSAGE("SERIAL must be defined when DOME_DRIVE_SERIAL and RDH_SERIAL are absent");
#else
    TEST_PASS();
#endif
}

// ---- main -------------------------------------------------------------------

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();

    RUN_TEST(test_servo_pins);
    RUN_TEST(test_dout_pins);
    RUN_TEST(test_dome_hall_pin);
    RUN_TEST(test_ppmin_pin);
    RUN_TEST(test_analog_pins);
    RUN_TEST(test_sd_cs_pin);
    RUN_TEST(test_spi_pins);
    RUN_TEST(test_i2c_pins);
    RUN_TEST(test_rcsel_pin_undefined_by_default);
    RUN_TEST(test_sel2_pin_defined_by_default);
    RUN_TEST(test_serial_defined_when_drive_serials_absent);

    return UNITY_END();
}
