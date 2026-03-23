// test_audio.cpp
// Unit tests for volume-update throttle logic in AmidalaAudio::setVolumeNoResponse.
//
// The throttle prevents flooding the HCR with SetVolume commands when the
// potentiometer moves rapidly — only one burst of commands is sent per
// VOLUME_THROTTLE_MS window.
//
// Because setVolumeNoResponse() calls into AmidalaController / HCRVocalizer
// when the throttle passes, the tests below exercise the throttle decision
// logic directly using the same pattern as the implementation.  This is the
// same approach used in test_buttons.cpp for guard-condition coverage.

#include "arduino_mock.h"
#include "audio.h"
#include <unity.h>

// ---- Throttle harness -------------------------------------------------------
// Mirrors the logic in AmidalaAudio::setVolumeNoResponse exactly.
// If the implementation changes its pattern, this harness and the tests must
// be updated to match.

struct VolumeThrottleHarness {
    uint32_t lastUpdate = (uint32_t)(0u - AmidalaAudio::VOLUME_THROTTLE_MS);
    int callCount = 0;  // represents number of SetVolume bursts that went through

    bool tryUpdate() {
        uint32_t now = millis();
        if (now - lastUpdate < AmidalaAudio::VOLUME_THROTTLE_MS) return false;
        lastUpdate = now;
        callCount++;
        return true;
    }
};

// ---- setUp / tearDown -------------------------------------------------------

void setUp(void) {
    mock_millis_value = 0;
}
void tearDown(void) {}

// ---- Constant value ---------------------------------------------------------

void test_volume_throttle_ms_is_80() {
    TEST_ASSERT_EQUAL_UINT32(80, AmidalaAudio::VOLUME_THROTTLE_MS);
}

// ---- First call always passes -----------------------------------------------

void test_first_update_passes_at_time_zero() {
    mock_millis_value = 0;
    VolumeThrottleHarness h;
    TEST_ASSERT_TRUE(h.tryUpdate());
    TEST_ASSERT_EQUAL_INT(1, h.callCount);
}

void test_first_update_passes_at_nonzero_time() {
    mock_millis_value = 500;
    VolumeThrottleHarness h;
    TEST_ASSERT_TRUE(h.tryUpdate());
    TEST_ASSERT_EQUAL_INT(1, h.callCount);
}

// ---- Rapid calls are throttled ----------------------------------------------

void test_immediate_second_call_is_blocked() {
    mock_millis_value = 0;
    VolumeThrottleHarness h;
    h.tryUpdate();                     // first: passes
    TEST_ASSERT_FALSE(h.tryUpdate());  // same ms: blocked
    TEST_ASSERT_EQUAL_INT(1, h.callCount);
}

void test_call_just_before_window_is_blocked() {
    mock_millis_value = 0;
    VolumeThrottleHarness h;
    h.tryUpdate();                              // t=0: passes
    mock_millis_value = AmidalaAudio::VOLUME_THROTTLE_MS - 1;  // t=79ms
    TEST_ASSERT_FALSE(h.tryUpdate());           // still within window: blocked
    TEST_ASSERT_EQUAL_INT(1, h.callCount);
}

// ---- Call at exactly the window boundary passes ----------------------------

void test_call_at_exact_window_boundary_passes() {
    mock_millis_value = 0;
    VolumeThrottleHarness h;
    h.tryUpdate();                                    // t=0: passes
    mock_millis_value = AmidalaAudio::VOLUME_THROTTLE_MS;  // t=80ms
    TEST_ASSERT_TRUE(h.tryUpdate());                  // exactly at boundary: passes
    TEST_ASSERT_EQUAL_INT(2, h.callCount);
}

// ---- Call well past the window passes ---------------------------------------

void test_call_well_past_window_passes() {
    mock_millis_value = 1000;
    VolumeThrottleHarness h;
    h.tryUpdate();                   // t=1000ms: passes
    mock_millis_value = 1200;        // t=1200ms: 200ms later
    TEST_ASSERT_TRUE(h.tryUpdate()); // well past window: passes
    TEST_ASSERT_EQUAL_INT(2, h.callCount);
}

// ---- Burst of rapid calls only lets one through per window -----------------

void test_burst_of_10_rapid_calls_lets_only_one_through() {
    mock_millis_value = 0;
    VolumeThrottleHarness h;
    for (int i = 0; i < 10; i++) {
        h.tryUpdate();  // all at t=0
    }
    TEST_ASSERT_EQUAL_INT(1, h.callCount);
}

// ---- Multiple windows accumulate correctly ----------------------------------

void test_three_windows_produce_three_updates() {
    mock_millis_value = 0;
    VolumeThrottleHarness h;
    h.tryUpdate();                                        // t=0
    mock_millis_value = AmidalaAudio::VOLUME_THROTTLE_MS; // t=80
    h.tryUpdate();
    mock_millis_value = AmidalaAudio::VOLUME_THROTTLE_MS * 2; // t=160
    h.tryUpdate();
    TEST_ASSERT_EQUAL_INT(3, h.callCount);
}

// ---- main -------------------------------------------------------------------

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();

    RUN_TEST(test_volume_throttle_ms_is_80);

    RUN_TEST(test_first_update_passes_at_time_zero);
    RUN_TEST(test_first_update_passes_at_nonzero_time);

    RUN_TEST(test_immediate_second_call_is_blocked);
    RUN_TEST(test_call_just_before_window_is_blocked);

    RUN_TEST(test_call_at_exact_window_boundary_passes);
    RUN_TEST(test_call_well_past_window_passes);

    RUN_TEST(test_burst_of_10_rapid_calls_lets_only_one_through);
    RUN_TEST(test_three_windows_produce_three_updates);

    return UNITY_END();
}
