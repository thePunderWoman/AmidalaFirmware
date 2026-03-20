// test_i2c_utils.cpp
// Tests for include/i2c_utils.h:
//   recoverI2CBus() — hardware recovery (smoke-tests only; no real bus)
//   sendI2CCmd()    — single-byte command; returns Wire error code
//   sendI2CStr()    — string payload;      returns Wire error code

#include "arduino_mock.h"
#include "pin_config.h"
#include "i2c_utils.h"
#include <unity.h>

void setUp(void) {
    Wire.endTransmissionResult = 0;  // default: success
}
void tearDown(void) {}

// ---- recoverI2CBus ----------------------------------------------------------

void test_recover_i2c_bus_does_not_crash() {
    // All hardware calls are no-ops in the mock; just verify no crash.
    recoverI2CBus();
    TEST_PASS();
}

void test_recover_i2c_bus_callable_multiple_times() {
    recoverI2CBus();
    recoverI2CBus();
    TEST_PASS();
}

// ---- sendI2CCmd -------------------------------------------------------------

void test_send_i2c_cmd_returns_zero_on_success() {
    Wire.endTransmissionResult = 0;
    byte result = sendI2CCmd(0x42, 0x01);
    TEST_ASSERT_EQUAL(0, result);
}

void test_send_i2c_cmd_returns_error_code_on_failure() {
    Wire.endTransmissionResult = 2;  // NACK on address
    byte result = sendI2CCmd(0x42, 0x01);
    TEST_ASSERT_EQUAL(2, result);
}

void test_send_i2c_cmd_error_code_4_returned() {
    Wire.endTransmissionResult = 4;  // other error
    byte result = sendI2CCmd(0x10, 0xFF);
    TEST_ASSERT_EQUAL(4, result);
}

void test_send_i2c_cmd_success_with_various_addresses() {
    Wire.endTransmissionResult = 0;
    TEST_ASSERT_EQUAL(0, sendI2CCmd(0x00, 0x00));
    TEST_ASSERT_EQUAL(0, sendI2CCmd(0x7F, 0xFF));
    TEST_ASSERT_EQUAL(0, sendI2CCmd(0x20, 0x10));
}

// ---- sendI2CStr -------------------------------------------------------------

void test_send_i2c_str_returns_zero_on_success() {
    Wire.endTransmissionResult = 0;
    byte result = sendI2CStr(0x42, "HELLO");
    TEST_ASSERT_EQUAL(0, result);
}

void test_send_i2c_str_returns_error_code_on_failure() {
    Wire.endTransmissionResult = 3;  // NACK on data
    byte result = sendI2CStr(0x42, "HELLO");
    TEST_ASSERT_EQUAL(3, result);
}

void test_send_i2c_str_empty_string_succeeds() {
    Wire.endTransmissionResult = 0;
    byte result = sendI2CStr(0x42, "");
    TEST_ASSERT_EQUAL(0, result);
}

void test_send_i2c_str_long_string_succeeds() {
    Wire.endTransmissionResult = 0;
    byte result = sendI2CStr(0x42, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    TEST_ASSERT_EQUAL(0, result);
}

// ---- main -------------------------------------------------------------------

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();

    RUN_TEST(test_recover_i2c_bus_does_not_crash);
    RUN_TEST(test_recover_i2c_bus_callable_multiple_times);

    RUN_TEST(test_send_i2c_cmd_returns_zero_on_success);
    RUN_TEST(test_send_i2c_cmd_returns_error_code_on_failure);
    RUN_TEST(test_send_i2c_cmd_error_code_4_returned);
    RUN_TEST(test_send_i2c_cmd_success_with_various_addresses);

    RUN_TEST(test_send_i2c_str_returns_zero_on_success);
    RUN_TEST(test_send_i2c_str_returns_error_code_on_failure);
    RUN_TEST(test_send_i2c_str_empty_string_succeeds);
    RUN_TEST(test_send_i2c_str_long_string_succeeds);

    return UNITY_END();
}
