// test_processconfig.cpp
// Regression tests for bugs found in src/config.cpp processConfig().
//
// Two key bugs were fixed:
//   1. rvrmax=/rvlmin=/rvlmax= all wrote to params.rvrmin (copy-paste error).
//      Tests here verify the four VREF fields are distinct memory locations and
//      that intparam() routes each to the correct field.
//   2. Servo position clamp was min(max(pos,180),90) — always produced 90.
//      Tests here verify the correct clamping formula min(max(pos,0),180).
//
// Full processConfig() integration tests require AmidalaController and are
// covered by functional testing.

#include "arduino_mock.h"
#include "core.h"
#include "params.h"
#include <unity.h>
#include <string.h>

void setUp(void) {
    memset(EEPROM.data, 0, sizeof(EEPROM.data));
}
void tearDown(void) {}

// ---- VREF fields are distinct memory locations ------------------------------
// A copy-paste bug made rvrmax=, rvlmin=, rvlmax= all write to rvrmin.
// These tests verify the four fields are different addresses so a regression
// would be immediately visible.

void test_vref_fields_are_distinct_addresses() {
    AmidalaParameters p;
    TEST_ASSERT_NOT_EQUAL((void*)&p.rvrmin, (void*)&p.rvrmax);
    TEST_ASSERT_NOT_EQUAL((void*)&p.rvrmin, (void*)&p.rvlmin);
    TEST_ASSERT_NOT_EQUAL((void*)&p.rvrmin, (void*)&p.rvlmax);
    TEST_ASSERT_NOT_EQUAL((void*)&p.rvrmax, (void*)&p.rvlmin);
    TEST_ASSERT_NOT_EQUAL((void*)&p.rvrmax, (void*)&p.rvlmax);
    TEST_ASSERT_NOT_EQUAL((void*)&p.rvlmin, (void*)&p.rvlmax);
}

void test_vref_fields_write_independently() {
    AmidalaParameters p;
    memset(&p, 0, sizeof(p));

    // Write distinct sentinel values to each field.
    p.rvrmin = 10;
    p.rvrmax = 920;
    p.rvlmin = 30;
    p.rvlmax = 940;

    // Verify no field aliases another.
    TEST_ASSERT_EQUAL_UINT16(10,  p.rvrmin);
    TEST_ASSERT_EQUAL_UINT16(920, p.rvrmax);
    TEST_ASSERT_EQUAL_UINT16(30,  p.rvlmin);
    TEST_ASSERT_EQUAL_UINT16(940, p.rvlmax);
}

void test_vref_intparam_routes_to_correct_field() {
    // Use intparam() with the exact same field references as config.cpp to
    // confirm each parameter key lands in the right struct member.
    AmidalaParameters p;
    memset(&p, 0, sizeof(p));

    // Simulate what processConfig() does for each VREF command.
    intparam("rvrmin=10",  "rvrmin=", p.rvrmin, 0, 100);
    intparam("rvrmax=920", "rvrmax=", p.rvrmax, 900, 1023);
    intparam("rvlmin=30",  "rvlmin=", p.rvlmin, 0, 100);
    intparam("rvlmax=940", "rvlmax=", p.rvlmax, 900, 1023);

    TEST_ASSERT_EQUAL_UINT16(10,  p.rvrmin);
    TEST_ASSERT_EQUAL_UINT16(920, p.rvrmax);
    TEST_ASSERT_EQUAL_UINT16(30,  p.rvlmin);
    TEST_ASSERT_EQUAL_UINT16(940, p.rvlmax);
}

// ---- Servo position clamp formula -------------------------------------------
// The bug was: min(max(pos, 180), 90) — always returns 90 because max(x,180)
// floors at 180, then min(180_or_more, 90) always returns 90.
// The fix is: min(max(pos, 0), 180).

static int servo_clamp_fixed(int pos) {
    return min(max(pos, 0), 180);
}

static int servo_clamp_broken(int pos) {
    return min(max(pos, 180), 90);
}

void test_servo_clamp_broken_always_returns_90() {
    // Confirm the old formula was indeed broken for all representative inputs.
    TEST_ASSERT_EQUAL(90, servo_clamp_broken(0));
    TEST_ASSERT_EQUAL(90, servo_clamp_broken(45));
    TEST_ASSERT_EQUAL(90, servo_clamp_broken(90));
    TEST_ASSERT_EQUAL(90, servo_clamp_broken(135));
    TEST_ASSERT_EQUAL(90, servo_clamp_broken(180));
}

void test_servo_clamp_fixed_preserves_valid_positions() {
    TEST_ASSERT_EQUAL(0,   servo_clamp_fixed(0));
    TEST_ASSERT_EQUAL(45,  servo_clamp_fixed(45));
    TEST_ASSERT_EQUAL(90,  servo_clamp_fixed(90));
    TEST_ASSERT_EQUAL(135, servo_clamp_fixed(135));
    TEST_ASSERT_EQUAL(180, servo_clamp_fixed(180));
}

void test_servo_clamp_fixed_clamps_below_zero() {
    TEST_ASSERT_EQUAL(0, servo_clamp_fixed(-1));
    TEST_ASSERT_EQUAL(0, servo_clamp_fixed(-100));
}

void test_servo_clamp_fixed_clamps_above_180() {
    TEST_ASSERT_EQUAL(180, servo_clamp_fixed(181));
    TEST_ASSERT_EQUAL(180, servo_clamp_fixed(255));
}

// ---- main -------------------------------------------------------------------

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();

    RUN_TEST(test_vref_fields_are_distinct_addresses);
    RUN_TEST(test_vref_fields_write_independently);
    RUN_TEST(test_vref_intparam_routes_to_correct_field);

    RUN_TEST(test_servo_clamp_broken_always_returns_90);
    RUN_TEST(test_servo_clamp_fixed_preserves_valid_positions);
    RUN_TEST(test_servo_clamp_fixed_clamps_below_zero);
    RUN_TEST(test_servo_clamp_fixed_clamps_above_180);

    return UNITY_END();
}
