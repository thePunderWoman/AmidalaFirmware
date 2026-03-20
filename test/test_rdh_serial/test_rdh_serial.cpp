// test_rdh_serial.cpp
// Tests for include/rdh_serial.h:
//   RDHSerial — dome position sensor / command interface
//
// Tests cover:
//   - Initial state (not ready, position -1)
//   - Command output format for all send methods
//   - process() parsing of valid #DP packets (modes @!$%)
//   - process() position update and median filtering after 6+ samples
//   - process() error recovery on invalid input

#include "arduino_mock.h"
#include "rdh_serial.h"
#include <unity.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

// ---- Initial state ----------------------------------------------------------

void test_not_ready_before_any_packet() {
    MockStream s;
    RDHSerial rdh(s);
    TEST_ASSERT_FALSE(rdh.ready());
    TEST_ASSERT_EQUAL(-1, rdh.getAngle());
}

void test_initial_mode_is_zero() {
    MockStream s;
    RDHSerial rdh(s);
    TEST_ASSERT_EQUAL(0, rdh.getMode());
}

// ---- Command output format --------------------------------------------------

void test_set_relative_position_pos_only() {
    MockStream s;
    RDHSerial rdh(s);
    rdh.setRelativePosition(45);
    TEST_ASSERT_NOT_NULL(strstr(s.outBuf, ":DPDM"));
    TEST_ASSERT_NOT_NULL(strstr(s.outBuf, "45"));
    TEST_ASSERT_NOT_NULL(strstr(s.outBuf, "+"));
}

void test_set_relative_position_with_speed() {
    MockStream s;
    RDHSerial rdh(s);
    rdh.setRelativePosition(30, 75);
    TEST_ASSERT_NOT_NULL(strstr(s.outBuf, ":DPDM"));
    TEST_ASSERT_NOT_NULL(strstr(s.outBuf, "30"));
    TEST_ASSERT_NOT_NULL(strstr(s.outBuf, ","));
    TEST_ASSERT_NOT_NULL(strstr(s.outBuf, "75"));
}

void test_set_absolute_position_pos_only() {
    MockStream s;
    RDHSerial rdh(s);
    rdh.setAbsolutePosition(180);
    TEST_ASSERT_NOT_NULL(strstr(s.outBuf, ":DPAM"));
    TEST_ASSERT_NOT_NULL(strstr(s.outBuf, "180"));
    TEST_ASSERT_NOT_NULL(strstr(s.outBuf, "+"));
}

void test_set_absolute_position_with_speed() {
    MockStream s;
    RDHSerial rdh(s);
    rdh.setAbsolutePosition(90, 50);
    TEST_ASSERT_NOT_NULL(strstr(s.outBuf, ":DPAM"));
    TEST_ASSERT_NOT_NULL(strstr(s.outBuf, "90"));
    TEST_ASSERT_NOT_NULL(strstr(s.outBuf, ","));
    TEST_ASSERT_NOT_NULL(strstr(s.outBuf, "50"));
}

void test_set_dome_home_position_no_arg() {
    MockStream s;
    RDHSerial rdh(s);
    rdh.setDomeHomePosition();
    TEST_ASSERT_NOT_NULL(strstr(s.outBuf, "#DPHOMEPOS"));
}

void test_set_dome_home_position_with_arg() {
    MockStream s;
    RDHSerial rdh(s);
    rdh.setDomeHomePosition(100);
    TEST_ASSERT_NOT_NULL(strstr(s.outBuf, "#DPHOMEPOS"));
    TEST_ASSERT_NOT_NULL(strstr(s.outBuf, "100"));
}

void test_toggle_random_dome() {
    MockStream s;
    RDHSerial rdh(s);
    rdh.toggleRandomDome();
    TEST_ASSERT_NOT_NULL(strstr(s.outBuf, "#DPAUTO"));
}

void test_send_command_passes_through() {
    MockStream s;
    RDHSerial rdh(s);
    rdh.sendCommand("#MYTEST");
    TEST_ASSERT_NOT_NULL(strstr(s.outBuf, "#MYTEST"));
}

void test_set_dome_default_mode_0() {
    MockStream s;
    RDHSerial rdh(s);
    rdh.setDomeDefaultMode(0);
    TEST_ASSERT_NOT_NULL(strstr(s.outBuf, "#DPHOME0"));
    TEST_ASSERT_NOT_NULL(strstr(s.outBuf, "#DPAUTO0"));
}

void test_set_dome_default_mode_1() {
    MockStream s;
    RDHSerial rdh(s);
    rdh.setDomeDefaultMode(1);
    TEST_ASSERT_NOT_NULL(strstr(s.outBuf, "#DPHOME1"));
    TEST_ASSERT_NOT_NULL(strstr(s.outBuf, "#DPAUTO0"));
}

void test_set_dome_default_mode_2() {
    MockStream s;
    RDHSerial rdh(s);
    rdh.setDomeDefaultMode(2);
    TEST_ASSERT_NOT_NULL(strstr(s.outBuf, "#DPHOME0"));
    TEST_ASSERT_NOT_NULL(strstr(s.outBuf, "#DPAUTO1"));
}

// ---- process() parsing ------------------------------------------------------

void test_process_valid_packet_mode_at_sets_angle() {
    // Mode '@' → fMode=0, angle parsed from digits
    MockStream s;
    RDHSerial rdh(s);
    s.feedInput("#DP@123\r\n");
    rdh.process();
    TEST_ASSERT_TRUE(rdh.ready());
    TEST_ASSERT_EQUAL(123, rdh.getAngle());
    TEST_ASSERT_EQUAL(0, rdh.getMode());
}

void test_process_valid_packet_mode_bang() {
    MockStream s;
    RDHSerial rdh(s);
    s.feedInput("#DP!45\r\n");
    rdh.process();
    TEST_ASSERT_EQUAL(45, rdh.getAngle());
    TEST_ASSERT_EQUAL(1, rdh.getMode());
}

void test_process_valid_packet_mode_dollar() {
    MockStream s;
    RDHSerial rdh(s);
    s.feedInput("#DP$90\r\n");
    rdh.process();
    TEST_ASSERT_EQUAL(90, rdh.getAngle());
    TEST_ASSERT_EQUAL(2, rdh.getMode());
}

void test_process_valid_packet_mode_percent() {
    MockStream s;
    RDHSerial rdh(s);
    s.feedInput("#DP%270\r\n");
    rdh.process();
    TEST_ASSERT_EQUAL(270, rdh.getAngle());
    TEST_ASSERT_EQUAL(3, rdh.getMode());
}

void test_process_zero_angle() {
    MockStream s;
    RDHSerial rdh(s);
    s.feedInput("#DP@0\r\n");
    rdh.process();
    TEST_ASSERT_TRUE(rdh.ready());
    TEST_ASSERT_EQUAL(0, rdh.getAngle());
}

void test_process_invalid_prefix_not_ready() {
    // Garbage input should not update position.
    MockStream s;
    RDHSerial rdh(s);
    s.feedInput("GARBAGE\r\n");
    rdh.process();
    TEST_ASSERT_FALSE(rdh.ready());
}

void test_process_bad_mode_char_not_ready() {
    // Valid prefix but unknown mode char should be rejected.
    MockStream s;
    RDHSerial rdh(s);
    s.feedInput("#DPX100\r\n");
    rdh.process();
    TEST_ASSERT_FALSE(rdh.ready());
}

// ---- Median filtering after 6+ samples -------------------------------------

// Feed the same packet N times (using fresh feedInput each call) and collect
// the angle reported after each.
static void feed_packet(RDHSerial &rdh, MockStream &s, int angle) {
    char buf[32];
    snprintf(buf, sizeof(buf), "#DP@%d\r\n", angle);
    s.feedInput(buf);
    rdh.process();
}

void test_process_median_filter_engages_after_six_samples() {
    // Feed 7 identical packets — after the 6th the median filter kicks in.
    // With all identical values the median equals the value.
    MockStream s;
    RDHSerial rdh(s);
    for (int i = 0; i < 7; i++)
        feed_packet(rdh, s, 100);
    TEST_ASSERT_EQUAL(100, rdh.getAngle());
}

void test_process_median_filters_outlier() {
    // Seed with 5 good samples at 100, then one outlier at 300.
    // The median of the 5-element buffer should reject the outlier.
    MockStream s;
    RDHSerial rdh(s);
    // First 5 samples set fPosition directly and warm up the buffer.
    for (int i = 0; i < 5; i++)
        feed_packet(rdh, s, 100);
    // 6th sample: outlier.  At sampleCount==5 we still assign directly;
    // from sampleCount==6 the median is used.
    feed_packet(rdh, s, 300);
    // 7th: back to 100.  Now median filter is active.
    feed_packet(rdh, s, 100);
    // The buffer [100,100,100,100,100,300,100] — median should be 100.
    TEST_ASSERT_EQUAL(100, rdh.getAngle());
}

// ---- main -------------------------------------------------------------------

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();

    RUN_TEST(test_not_ready_before_any_packet);
    RUN_TEST(test_initial_mode_is_zero);

    RUN_TEST(test_set_relative_position_pos_only);
    RUN_TEST(test_set_relative_position_with_speed);
    RUN_TEST(test_set_absolute_position_pos_only);
    RUN_TEST(test_set_absolute_position_with_speed);
    RUN_TEST(test_set_dome_home_position_no_arg);
    RUN_TEST(test_set_dome_home_position_with_arg);
    RUN_TEST(test_toggle_random_dome);
    RUN_TEST(test_send_command_passes_through);
    RUN_TEST(test_set_dome_default_mode_0);
    RUN_TEST(test_set_dome_default_mode_1);
    RUN_TEST(test_set_dome_default_mode_2);

    RUN_TEST(test_process_valid_packet_mode_at_sets_angle);
    RUN_TEST(test_process_valid_packet_mode_bang);
    RUN_TEST(test_process_valid_packet_mode_dollar);
    RUN_TEST(test_process_valid_packet_mode_percent);
    RUN_TEST(test_process_zero_angle);
    RUN_TEST(test_process_invalid_prefix_not_ready);
    RUN_TEST(test_process_bad_mode_char_not_ready);

    RUN_TEST(test_process_median_filter_engages_after_six_samples);
    RUN_TEST(test_process_median_filters_outlier);

    return UNITY_END();
}
