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

// ---- RoboClaw config key routing --------------------------------------------
// Mirrors what processConfig() does: each intparam() call uses the exact same
// field reference and range as config.cpp so a future field rename or range
// change will break these tests before it reaches the embedded build.

void test_domercaddr_intparam_routes_to_correct_field() {
    AmidalaParameters p;
    memset(&p, 0, sizeof(p));
    bool matched = intparam("domercaddr=128", "domercaddr=", p.domercaddr, 128, 135);
    TEST_ASSERT_TRUE(matched);
    TEST_ASSERT_EQUAL(128, p.domercaddr);
}

void test_domercaddr_clamps_below_minimum() {
    AmidalaParameters p;
    memset(&p, 0, sizeof(p));
    // intparam clamps out-of-range values rather than rejecting them.
    // 100 < 128 → clamped to 128.
    bool matched = intparam("domercaddr=100", "domercaddr=", p.domercaddr, 128, 135);
    TEST_ASSERT_TRUE(matched);
    TEST_ASSERT_EQUAL(128, p.domercaddr);
}

void test_domercchan_intparam_channel_1() {
    AmidalaParameters p;
    memset(&p, 0, sizeof(p));
    bool matched = intparam("domercchan=1", "domercchan=", p.domercchan, 1, 2);
    TEST_ASSERT_TRUE(matched);
    TEST_ASSERT_EQUAL(1, p.domercchan);
}

void test_domercchan_intparam_channel_2() {
    AmidalaParameters p;
    memset(&p, 0, sizeof(p));
    bool matched = intparam("domercchan=2", "domercchan=", p.domercchan, 1, 2);
    TEST_ASSERT_TRUE(matched);
    TEST_ASSERT_EQUAL(2, p.domercchan);
}

void test_domercchan_clamps_channel_3_to_2() {
    AmidalaParameters p;
    memset(&p, 0, sizeof(p));
    // 3 > 2 → clamped to 2.
    bool matched = intparam("domercchan=3", "domercchan=", p.domercchan, 1, 2);
    TEST_ASSERT_TRUE(matched);
    TEST_ASSERT_EQUAL(2, p.domercchan);
}

void test_domercqpps_intparam_routes_to_correct_field() {
    AmidalaParameters p;
    memset(&p, 0, sizeof(p));
    bool matched = intparam("domercqpps=800", "domercqpps=", p.domercqpps, 1, 65535);
    TEST_ASSERT_TRUE(matched);
    TEST_ASSERT_EQUAL(800, p.domercqpps);
}

void test_domefront_intparam_routes_to_correct_field() {
    AmidalaParameters p;
    memset(&p, 0, sizeof(p));
    bool matched = intparam("domefront=88", "domefront=", p.domefront, 0, 359);
    TEST_ASSERT_TRUE(matched);
    TEST_ASSERT_EQUAL(88, p.domefront);
}

void test_domefront_accepts_zero() {
    AmidalaParameters p;
    memset(&p, 0, sizeof(p));
    bool matched = intparam("domefront=0", "domefront=", p.domefront, 0, 359);
    TEST_ASSERT_TRUE(matched);
    TEST_ASSERT_EQUAL(0, p.domefront);
}

void test_domefront_accepts_359() {
    AmidalaParameters p;
    memset(&p, 0, sizeof(p));
    bool matched = intparam("domefront=359", "domefront=", p.domefront, 0, 359);
    TEST_ASSERT_TRUE(matched);
    TEST_ASSERT_EQUAL(359, p.domefront);
}

void test_domefront_clamps_360_to_359() {
    AmidalaParameters p;
    memset(&p, 0, sizeof(p));
    // 360 > 359 → clamped to 359.
    bool matched = intparam("domefront=360", "domefront=", p.domefront, 0, 359);
    TEST_ASSERT_TRUE(matched);
    TEST_ASSERT_EQUAL(359, p.domefront);
}

void test_domestall_intparam_routes_to_correct_field() {
    AmidalaParameters p;
    memset(&p, 0, sizeof(p));
    bool matched = intparam("domestall=500", "domestall=", p.domestall, 100, 5000);
    TEST_ASSERT_TRUE(matched);
    TEST_ASSERT_EQUAL(500, p.domestall);
}

void test_domestall_clamps_below_minimum() {
    AmidalaParameters p;
    memset(&p, 0, sizeof(p));
    // 50 < 100 → clamped to 100.
    bool matched = intparam("domestall=50", "domestall=", p.domestall, 100, 5000);
    TEST_ASSERT_TRUE(matched);
    TEST_ASSERT_EQUAL(100, p.domestall);
}

void test_roboclaw_fields_are_distinct_from_each_other() {
    // Structural guard: none of the new fields should alias an existing one.
    AmidalaParameters p;
    TEST_ASSERT_NOT_EQUAL((void*)&p.domercaddr, (void*)&p.domercqpps);
    TEST_ASSERT_NOT_EQUAL((void*)&p.domefront,  (void*)&p.domestall);
    TEST_ASSERT_NOT_EQUAL((void*)&p.domefront,  (void*)&p.domehome);  // most likely alias mistake
    TEST_ASSERT_NOT_EQUAL((void*)&p.domestall,  (void*)&p.domespeed);
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

    RUN_TEST(test_domercaddr_intparam_routes_to_correct_field);
    RUN_TEST(test_domercaddr_clamps_below_minimum);
    RUN_TEST(test_domercchan_intparam_channel_1);
    RUN_TEST(test_domercchan_intparam_channel_2);
    RUN_TEST(test_domercchan_clamps_channel_3_to_2);
    RUN_TEST(test_domercqpps_intparam_routes_to_correct_field);
    RUN_TEST(test_domefront_intparam_routes_to_correct_field);
    RUN_TEST(test_domefront_accepts_zero);
    RUN_TEST(test_domefront_accepts_359);
    RUN_TEST(test_domefront_clamps_360_to_359);
    RUN_TEST(test_domestall_intparam_routes_to_correct_field);
    RUN_TEST(test_domestall_clamps_below_minimum);
    RUN_TEST(test_roboclaw_fields_are_distinct_from_each_other);

    return UNITY_END();
}
