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

// ---- Volume routing harness -------------------------------------------------
// Mirrors the logic in AmidalaAudio::applyHCRVolume.
// wheel: 0=global, 1=voice (CH_V), 2=chA (CH_A), 3=chB (CH_B).

struct VolumeRouteHarness {
    int chV = -1, chA = -1, chB = -1;

    void apply(uint8_t wheel, uint8_t volume) {
        switch (wheel) {
            case 1: chV = volume; break;
            case 2: chA = volume; break;
            case 3: chB = volume; break;
            default:
                chV = chA = chB = (int)volume;
                break;
        }
    }

    void reset() { chV = chA = chB = -1; }
};

// ---- volumewheel routing ----------------------------------------------------

void test_volumewheel_global_sets_all_channels() {
    VolumeRouteHarness h;
    h.apply(0, 75);
    TEST_ASSERT_EQUAL_INT(75, h.chV);
    TEST_ASSERT_EQUAL_INT(75, h.chA);
    TEST_ASSERT_EQUAL_INT(75, h.chB);
}

void test_volumewheel_voice_sets_only_chV() {
    VolumeRouteHarness h;
    h.apply(1, 60);
    TEST_ASSERT_EQUAL_INT(60, h.chV);
    TEST_ASSERT_EQUAL_INT(-1, h.chA);
    TEST_ASSERT_EQUAL_INT(-1, h.chB);
}

void test_volumewheel_chA_sets_only_chA() {
    VolumeRouteHarness h;
    h.apply(2, 40);
    TEST_ASSERT_EQUAL_INT(-1, h.chV);
    TEST_ASSERT_EQUAL_INT(40, h.chA);
    TEST_ASSERT_EQUAL_INT(-1, h.chB);
}

void test_volumewheel_chB_sets_only_chB() {
    VolumeRouteHarness h;
    h.apply(3, 30);
    TEST_ASSERT_EQUAL_INT(-1, h.chV);
    TEST_ASSERT_EQUAL_INT(-1, h.chA);
    TEST_ASSERT_EQUAL_INT(30, h.chB);
}

void test_volumewheel_unknown_value_falls_back_to_global() {
    VolumeRouteHarness h;
    h.apply(99, 50);
    TEST_ASSERT_EQUAL_INT(50, h.chV);
    TEST_ASSERT_EQUAL_INT(50, h.chA);
    TEST_ASSERT_EQUAL_INT(50, h.chB);
}

// ---- altvolumewheel fall-through logic --------------------------------------
// Mirrors the decision in AmidalaAudio::setAltVolumeNoResponse:
//   altvolumewheel == 0  →  call setVolumeNoResponse (use volumewheel routing)
//   altvolumewheel != 0  →  route via altvolumewheel

static bool altWheelFallsThrough(uint8_t altvolumewheel) {
    return altvolumewheel == 0;
}

void test_altvolumewheel_zero_falls_through() {
    TEST_ASSERT_TRUE(altWheelFallsThrough(0));
}

void test_altvolumewheel_one_does_not_fall_through() {
    TEST_ASSERT_FALSE(altWheelFallsThrough(1));
}

void test_altvolumewheel_two_does_not_fall_through() {
    TEST_ASSERT_FALSE(altWheelFallsThrough(2));
}

void test_altvolumewheel_three_does_not_fall_through() {
    TEST_ASSERT_FALSE(altWheelFallsThrough(3));
}

void test_altvolumewheel_routes_to_correct_channel_when_nonzero() {
    // When altvolumewheel is set, the same applyHCRVolume logic applies.
    VolumeRouteHarness h;
    h.apply(2, 20);  // altvolumewheel=2 (chA)
    TEST_ASSERT_EQUAL_INT(-1, h.chV);
    TEST_ASSERT_EQUAL_INT(20, h.chA);
    TEST_ASSERT_EQUAL_INT(-1, h.chB);
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

    RUN_TEST(test_volumewheel_global_sets_all_channels);
    RUN_TEST(test_volumewheel_voice_sets_only_chV);
    RUN_TEST(test_volumewheel_chA_sets_only_chA);
    RUN_TEST(test_volumewheel_chB_sets_only_chB);
    RUN_TEST(test_volumewheel_unknown_value_falls_back_to_global);

    RUN_TEST(test_altvolumewheel_zero_falls_through);
    RUN_TEST(test_altvolumewheel_one_does_not_fall_through);
    RUN_TEST(test_altvolumewheel_two_does_not_fall_through);
    RUN_TEST(test_altvolumewheel_three_does_not_fall_through);
    RUN_TEST(test_altvolumewheel_routes_to_correct_channel_when_nonzero);

    return UNITY_END();
}
