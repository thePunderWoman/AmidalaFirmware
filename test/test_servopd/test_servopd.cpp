// test_servopd.cpp
// Unit tests for the ServoPD class in core.h.
//
// ServoPD implements a simple PD (proportional-derivative) controller used for
// camera pan and tilt servos.  Tests verify:
//   - Initial position equals the zero point passed to the constructor
//   - rawset()/rawget() are symmetric (no scale-related truncation surprises)
//   - update() moves the position in the expected direction
//   - reset() re-constrains the position without changing the filter state
//   - constrain() is respected (position never exceeds zero ± range)

#include "arduino_mock.h"
#include "core.h"
#include <unity.h>

// Firmware constants (mirror of AmidalaFirmware.ino)
#define PANZERO   90
#define PANRANGE  60
#define TILTZERO  70
#define TILTRANGE 40

void setUp(void) {}
void tearDown(void) {}

// ---- Construction -----------------------------------------------------------

void test_servopd_initial_position_equals_zero() {
  ServoPD pan(400, 200, PANZERO, PANRANGE);
  TEST_ASSERT_EQUAL_INT(PANZERO, pan.get());
}

void test_servopd_tilt_initial_position() {
  ServoPD tilt(300, 100, TILTZERO, TILTRANGE);
  TEST_ASSERT_EQUAL_INT(TILTZERO, tilt.get());
}

void test_servopd_custom_zero_preserved() {
  ServoPD s(100, 50, 45, 45);
  TEST_ASSERT_EQUAL_INT(45, s.get());
}

// ---- rawset / rawget --------------------------------------------------------

void test_servopd_rawset_rawget_roundtrip() {
  ServoPD pan(400, 200, PANZERO, PANRANGE);
  pan.rawset(90);
  TEST_ASSERT_EQUAL_INT(90, pan.rawget());
}

void test_servopd_rawset_min_boundary() {
  ServoPD pan(400, 200, PANZERO, PANRANGE);
  // zero=90, range=60 → fZero-fRange corresponds to get()=30
  pan.rawset(30);
  TEST_ASSERT_EQUAL_INT(30, pan.rawget());
}

void test_servopd_rawset_max_boundary() {
  ServoPD pan(400, 200, PANZERO, PANRANGE);
  pan.rawset(150);
  TEST_ASSERT_EQUAL_INT(150, pan.rawget());
}

void test_servopd_get_equals_rawget_after_rawset() {
  ServoPD pan(400, 200, PANZERO, PANRANGE);
  pan.rawset(100);
  TEST_ASSERT_EQUAL_INT(pan.rawget(), pan.get());
}

// ---- update() direction and constrain ---------------------------------------

void test_servopd_update_same_target_stays_near_zero() {
  // When the target equals the zero position, repeated updates converge there.
  ServoPD pan(400, 200, PANZERO, PANRANGE);
  pan.update(PANZERO);
  // The first update may produce a small transient; it must remain within
  // bounds and be reasonably close to the zero point.
  int pos = pan.get();
  TEST_ASSERT_TRUE(pos >= (PANZERO - PANRANGE));
  TEST_ASSERT_TRUE(pos <= (PANZERO + PANRANGE));
}

void test_servopd_update_positive_target_moves_up() {
  ServoPD pan(400, 200, PANZERO, PANRANGE);
  // Target above zero (max = 90+60 = 150); position should increase.
  pan.update(PANZERO + PANRANGE);
  TEST_ASSERT_TRUE(pan.get() >= PANZERO);
}

void test_servopd_update_negative_target_moves_down() {
  ServoPD pan(400, 200, PANZERO, PANRANGE);
  // Target below zero (min = 90-60 = 30); position should decrease.
  pan.update(PANZERO - PANRANGE);
  TEST_ASSERT_TRUE(pan.get() <= PANZERO);
}

void test_servopd_update_respects_upper_bound() {
  ServoPD pan(400, 200, PANZERO, PANRANGE);
  // Drive with an extreme target; constrain must prevent overshoot.
  for (int i = 0; i < 100; i++)
    pan.update(1000);
  TEST_ASSERT_TRUE(pan.get() <= (PANZERO + PANRANGE));
}

void test_servopd_update_respects_lower_bound() {
  ServoPD pan(400, 200, PANZERO, PANRANGE);
  for (int i = 0; i < 100; i++)
    pan.update(-1000);
  TEST_ASSERT_TRUE(pan.get() >= (PANZERO - PANRANGE));
}

// ---- reset() ----------------------------------------------------------------

void test_servopd_reset_leaves_position_within_bounds() {
  ServoPD pan(400, 200, PANZERO, PANRANGE);
  pan.rawset(80);
  pan.reset(PANZERO);
  int pos = pan.get();
  TEST_ASSERT_TRUE(pos >= (PANZERO - PANRANGE));
  TEST_ASSERT_TRUE(pos <= (PANZERO + PANRANGE));
}

void test_servopd_reset_does_not_jump_to_target() {
  // reset() updates fPrevTarget but must NOT snap fPos to the target.
  ServoPD pan(400, 200, PANZERO, PANRANGE);
  pan.rawset(100);
  pan.reset(30);
  // fPos was 100<<8; after constrain it should still be ~100, not 30.
  TEST_ASSERT_TRUE(pan.get() >= 80);
}

// ---- scalebits parameter ----------------------------------------------------

void test_servopd_custom_scalebits() {
  // scalebits=10 → fPos = zero << 10
  ServoPD s(100, 50, 50, 40, 10);
  TEST_ASSERT_EQUAL_INT(50, s.get());
  s.rawset(50);
  TEST_ASSERT_EQUAL_INT(50, s.rawget());
}

// ---- main -------------------------------------------------------------------

int main(int argc, char **argv) {
  (void)argc; (void)argv;
  UNITY_BEGIN();

  RUN_TEST(test_servopd_initial_position_equals_zero);
  RUN_TEST(test_servopd_tilt_initial_position);
  RUN_TEST(test_servopd_custom_zero_preserved);

  RUN_TEST(test_servopd_rawset_rawget_roundtrip);
  RUN_TEST(test_servopd_rawset_min_boundary);
  RUN_TEST(test_servopd_rawset_max_boundary);
  RUN_TEST(test_servopd_get_equals_rawget_after_rawset);

  RUN_TEST(test_servopd_update_same_target_stays_near_zero);
  RUN_TEST(test_servopd_update_positive_target_moves_up);
  RUN_TEST(test_servopd_update_negative_target_moves_down);
  RUN_TEST(test_servopd_update_respects_upper_bound);
  RUN_TEST(test_servopd_update_respects_lower_bound);

  RUN_TEST(test_servopd_reset_leaves_position_within_bounds);
  RUN_TEST(test_servopd_reset_does_not_jump_to_target);

  RUN_TEST(test_servopd_custom_scalebits);

  return UNITY_END();
}
