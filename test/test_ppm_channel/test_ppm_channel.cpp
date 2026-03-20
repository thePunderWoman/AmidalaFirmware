// test_ppm_channel.cpp
// Unit tests for PPMDecoder (include/ppm_decoder.h).
//
// Section 1 — Direct class tests: construction, init, decode with no signal,
//             and channel() neutral behaviour.
// Section 2 — Mapping formula tests: reproduce the channel() formula as a
//             free function to exhaustively test the clamping + map logic
//             independently of private state.

#include "arduino_mock.h"
#include "pin_config.h"
#include "ppm_decoder.h"
#include <unity.h>

// Mapping formula extracted from PPMDecoder::channel():
//   uint16_t channel(unsigned ch, unsigned minvalue, unsigned maxvalue,
//                    unsigned neutralvalue)
//   {
//       uint16_t pulse = fChannel[ch];
//       if (pulse != 0)
//           return map(max(min(pulse, 1600), 600), 600, 1600, minvalue, maxvalue);
//       return neutralvalue;
//   }
static uint16_t ppm_channel_map(uint16_t pulse,
                                 unsigned minvalue,
                                 unsigned maxvalue,
                                 unsigned neutralvalue) {
  if (pulse == 0)
    return (uint16_t)neutralvalue;
  uint16_t clamped = (pulse > 1600u) ? 1600u : (pulse < 600u ? 600u : pulse);
  return (uint16_t)map((long)clamped, 600L, 1600L,
                       (long)minvalue, (long)maxvalue);
}

void setUp(void) {}
void tearDown(void) {}

// ---- PPMDecoder class -------------------------------------------------------

void test_decoder_constructs_without_crash() {
  PPMDecoder dec(PPMIN_PIN, 6);
  // If we get here without crashing, construction and init() succeeded.
  TEST_PASS();
}

void test_decoder_decode_returns_false_when_no_signal() {
  // digitalRead() stub always returns LOW so readPulse() never fires an edge;
  // decode() must return false.
  PPMDecoder dec(PPMIN_PIN, 6);
  TEST_ASSERT_FALSE(dec.decode());
}

void test_decoder_channel_returns_neutral_before_any_signal() {
  PPMDecoder dec(PPMIN_PIN, 6);
  // fChannel[] is zero-initialised; channel() must return neutralvalue.
  TEST_ASSERT_EQUAL_UINT16(512, dec.channel(0, 0, 1024, 512));
  TEST_ASSERT_EQUAL_UINT16(512, dec.channel(5, 0, 1024, 512));
}

void test_decoder_channel_out_of_range_returns_neutral() {
  PPMDecoder dec(PPMIN_PIN, 6);
  // ch >= channelCount → pulse treated as 0 → neutralvalue returned.
  TEST_ASSERT_EQUAL_UINT16(512, dec.channel(6,  0, 1024, 512));
  TEST_ASSERT_EQUAL_UINT16(512, dec.channel(99, 0, 1024, 512));
}

void test_decoder_init_resets_state() {
  PPMDecoder dec(PPMIN_PIN, 6);
  dec.init();  // second call must not crash
  TEST_ASSERT_EQUAL_UINT16(0, dec.channel(0, 0, 1024, 0));
}

// ---- Zero pulse → neutral --------------------------------------------------

void test_ppm_zero_pulse_returns_neutral() {
  TEST_ASSERT_EQUAL_UINT16(512, ppm_channel_map(0, 0, 1023, 512));
}

void test_ppm_zero_pulse_neutral_at_center() {
  TEST_ASSERT_EQUAL_UINT16(500, ppm_channel_map(0, 0, 1000, 500));
}

// ---- Boundary mapping (in-range) -------------------------------------------

void test_ppm_min_pulse_maps_to_min_value() {
  // pulse=600 → maps to minvalue
  TEST_ASSERT_EQUAL_UINT16(0, ppm_channel_map(600, 0, 1000, 500));
}

void test_ppm_max_pulse_maps_to_max_value() {
  // pulse=1600 → maps to maxvalue
  TEST_ASSERT_EQUAL_UINT16(1000, ppm_channel_map(1600, 0, 1000, 500));
}

void test_ppm_midpoint_maps_to_midvalue() {
  // pulse=1100 is the midpoint of [600,1600]; maps to midpoint of [0,1000]=500
  TEST_ASSERT_EQUAL_UINT16(500, ppm_channel_map(1100, 0, 1000, 500));
}

// ---- Standard RC range [1000, 2000] → output [0, 255] ----------------------
// Pulses within [600,1600] window; real RC pulses are 1000-2000 but the
// firmware clamps them at 1600 (max) and 600 (min).

void test_ppm_rc_center_1100_to_127() {
  // 1100 µs midpoint → ~127 in [0,255]
  uint16_t result = ppm_channel_map(1100, 0, 255, 127);
  TEST_ASSERT_INT_WITHIN(2, 127, (int)result);
}

void test_ppm_rc_low_1000_maps_inside_range() {
  // 1000 µs is above 600 min; should map to some value in [0,255]
  uint16_t result = ppm_channel_map(1000, 0, 255, 127);
  TEST_ASSERT_TRUE(result >= 0 && result <= 255);
}

// ---- Clamping behaviour ----------------------------------------------------

void test_ppm_pulse_below_600_clamped_to_min() {
  // Pulse of 100 µs → clamped to 600 → maps to minvalue
  TEST_ASSERT_EQUAL_UINT16(0, ppm_channel_map(100, 0, 1000, 500));
}

void test_ppm_pulse_above_1600_clamped_to_max() {
  // Pulse of 2500 µs → clamped to 1600 → maps to maxvalue
  TEST_ASSERT_EQUAL_UINT16(1000, ppm_channel_map(2500, 0, 1000, 500));
}

void test_ppm_pulse_just_below_600_clamped() {
  TEST_ASSERT_EQUAL_UINT16(ppm_channel_map(600, 0, 1000, 500),
                            ppm_channel_map(599, 0, 1000, 500));
}

void test_ppm_pulse_just_above_1600_clamped() {
  TEST_ASSERT_EQUAL_UINT16(ppm_channel_map(1600, 0, 1000, 500),
                            ppm_channel_map(1601, 0, 1000, 500));
}

// ---- Output range [0, 255] (typical joystick axis) -------------------------

void test_ppm_full_range_0_255() {
  TEST_ASSERT_EQUAL_UINT16(0,   ppm_channel_map(600,  0, 255, 127));
  TEST_ASSERT_EQUAL_UINT16(255, ppm_channel_map(1600, 0, 255, 127));
}

// ---- Output range [127, -128] (reversed axis) ------------------------------
// The firmware maps sticks with map(... 127, -128) for inverted axes.

void test_ppm_reversed_axis_min_maps_to_max_output() {
  // pulse=600 → output=127 (high end because axis is reversed in firmware)
  TEST_ASSERT_EQUAL_UINT16(127, ppm_channel_map(600, 127, 0, 63));
  // Note: map() for reversed range: 127 at pulse=600, 0 at pulse=1600
}

// ---- main -------------------------------------------------------------------

int main(int argc, char **argv) {
  (void)argc; (void)argv;
  UNITY_BEGIN();

  RUN_TEST(test_decoder_constructs_without_crash);
  RUN_TEST(test_decoder_decode_returns_false_when_no_signal);
  RUN_TEST(test_decoder_channel_returns_neutral_before_any_signal);
  RUN_TEST(test_decoder_channel_out_of_range_returns_neutral);
  RUN_TEST(test_decoder_init_resets_state);

  RUN_TEST(test_ppm_zero_pulse_returns_neutral);
  RUN_TEST(test_ppm_zero_pulse_neutral_at_center);

  RUN_TEST(test_ppm_min_pulse_maps_to_min_value);
  RUN_TEST(test_ppm_max_pulse_maps_to_max_value);
  RUN_TEST(test_ppm_midpoint_maps_to_midvalue);

  RUN_TEST(test_ppm_rc_center_1100_to_127);
  RUN_TEST(test_ppm_rc_low_1000_maps_inside_range);

  RUN_TEST(test_ppm_pulse_below_600_clamped_to_min);
  RUN_TEST(test_ppm_pulse_above_1600_clamped_to_max);
  RUN_TEST(test_ppm_pulse_just_below_600_clamped);
  RUN_TEST(test_ppm_pulse_just_above_1600_clamped);

  RUN_TEST(test_ppm_full_range_0_255);
  RUN_TEST(test_ppm_reversed_axis_min_maps_to_max_output);

  return UNITY_END();
}
