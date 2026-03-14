// test_helpers.cpp
// Unit tests for the config-parsing helper functions in amidala_core.h:
//   startswith, strtol, strtolu, boolparam, intparam (u8/u16/u32),
//   charparam, sintparam, sintparam2, gestureparam, numberparams

#include "arduino_mock.h"
#include "amidala_core.h"
#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

// ---- startswith -------------------------------------------------------------

void test_startswith_match_advances_pointer() {
  const char *s = "volume=50";
  TEST_ASSERT_TRUE(startswith(s, "volume="));
  TEST_ASSERT_EQUAL_STRING("50", s);
}

void test_startswith_no_match_leaves_pointer() {
  const char *s = "volume=50";
  const char *orig = s;
  TEST_ASSERT_FALSE(startswith(s, "speed="));
  TEST_ASSERT_EQUAL_PTR(orig, s);
}

void test_startswith_empty_needle_always_matches() {
  const char *s = "abc";
  TEST_ASSERT_TRUE(startswith(s, ""));
  TEST_ASSERT_EQUAL_STRING("abc", s);
}

void test_startswith_exact_match() {
  const char *s = "reboot";
  TEST_ASSERT_TRUE(startswith(s, "reboot"));
  TEST_ASSERT_EQUAL_STRING("", s);
}

void test_startswith_partial_no_match() {
  const char *s = "vol";
  TEST_ASSERT_FALSE(startswith(s, "volume="));
}

// ---- strtolu ----------------------------------------------------------------

void test_strtolu_simple() {
  const char *end;
  TEST_ASSERT_EQUAL_UINT32(42, strtolu("42", &end));
  TEST_ASSERT_EQUAL_STRING("", end);
}

void test_strtolu_zero() {
  const char *end;
  TEST_ASSERT_EQUAL_UINT32(0, strtolu("0", &end));
  TEST_ASSERT_EQUAL_STRING("", end);
}

void test_strtolu_stops_at_non_digit() {
  const char *end;
  TEST_ASSERT_EQUAL_UINT32(123, strtolu("123abc", &end));
  TEST_ASSERT_EQUAL_STRING("abc", end);
}

void test_strtolu_no_digits_returns_zero() {
  const char *end;
  TEST_ASSERT_EQUAL_UINT32(0, strtolu("abc", &end));
  TEST_ASSERT_EQUAL_STRING("abc", end);
}

void test_strtolu_large_value() {
  const char *end;
  TEST_ASSERT_EQUAL_UINT32(65535, strtolu("65535", &end));
}

// ---- strtol (signed) --------------------------------------------------------

void test_strtol_positive() {
  const char *end;
  TEST_ASSERT_EQUAL_INT32(100, strtol("100", &end));
  TEST_ASSERT_EQUAL_STRING("", end);
}

void test_strtol_negative() {
  const char *end;
  TEST_ASSERT_EQUAL_INT32(-45, strtol("-45", &end));
  TEST_ASSERT_EQUAL_STRING("", end);
}

void test_strtol_zero() {
  const char *end;
  TEST_ASSERT_EQUAL_INT32(0, strtol("0", &end));
}

void test_strtol_stops_at_comma() {
  const char *end;
  TEST_ASSERT_EQUAL_INT32(90, strtol("90,30", &end));
  TEST_ASSERT_EQUAL_STRING(",30", end);
}

// ---- boolparam --------------------------------------------------------------

void test_boolparam_true() {
  bool val = false;
  TEST_ASSERT_TRUE(boolparam("startup=y", "startup=", val));
  TEST_ASSERT_TRUE(val);
}

void test_boolparam_false() {
  bool val = true;
  TEST_ASSERT_TRUE(boolparam("startup=n", "startup=", val));
  TEST_ASSERT_FALSE(val);
}

void test_boolparam_wrong_key() {
  bool val = false;
  TEST_ASSERT_FALSE(boolparam("other=y", "startup=", val));
}

void test_boolparam_invalid_value() {
  bool val = false;
  TEST_ASSERT_FALSE(boolparam("startup=1", "startup=", val));
  TEST_ASSERT_FALSE(boolparam("startup=yes", "startup=", val));
}

// ---- intparam (uint8) -------------------------------------------------------

void test_intparam_u8_basic() {
  uint8_t v = 0;
  TEST_ASSERT_TRUE(intparam("volume=50", "volume=", v, 0, 100));
  TEST_ASSERT_EQUAL_UINT8(50, v);
}

void test_intparam_u8_clamp_max() {
  uint8_t v = 0;
  TEST_ASSERT_TRUE(intparam("volume=150", "volume=", v, 0, 100));
  TEST_ASSERT_EQUAL_UINT8(100, v);
}

void test_intparam_u8_clamp_min() {
  uint8_t v = 99;
  TEST_ASSERT_TRUE(intparam("volume=0", "volume=", v, 10, 100));
  TEST_ASSERT_EQUAL_UINT8(10, v);
}

void test_intparam_u8_wrong_key() {
  uint8_t v = 0;
  TEST_ASSERT_FALSE(intparam("speed=50", "volume=", v, 0, 100));
}

void test_intparam_u8_trailing_chars_rejected() {
  uint8_t v = 0;
  // "volume=50abc" has trailing non-digit after the number — strtolu stops at
  // 'a', leaving cmd != '\0', so intparam should return false.
  TEST_ASSERT_FALSE(intparam("volume=50abc", "volume=", v, 0, 100));
}

// ---- intparam (uint16) ------------------------------------------------------

void test_intparam_u16_basic() {
  uint16_t v = 0;
  TEST_ASSERT_TRUE(intparam("minpulse=1000", "minpulse=", v, 0, 2500));
  TEST_ASSERT_EQUAL_UINT16(1000, v);
}

void test_intparam_u16_clamp() {
  uint16_t v = 0;
  TEST_ASSERT_TRUE(intparam("minpulse=3000", "minpulse=", v, 0, 2500));
  TEST_ASSERT_EQUAL_UINT16(2500, v);
}

// ---- intparam (uint32) ------------------------------------------------------

void test_intparam_u32_basic() {
  uint32_t v = 0;
  TEST_ASSERT_TRUE(intparam("auxbaud=9600", "auxbaud=", v, 300, 115200));
  TEST_ASSERT_EQUAL_UINT32(9600, v);
}

void test_intparam_u32_clamp_min() {
  uint32_t v = 0;
  TEST_ASSERT_TRUE(intparam("auxbaud=100", "auxbaud=", v, 300, 115200));
  TEST_ASSERT_EQUAL_UINT32(300, v);
}

// ---- charparam --------------------------------------------------------------

void test_charparam_valid_char() {
  char v = '\0';
  TEST_ASSERT_TRUE(charparam("acktype=g", "acktype=", "gadsr", v));
  TEST_ASSERT_EQUAL('g', v);
}

void test_charparam_valid_last_char() {
  char v = '\0';
  TEST_ASSERT_TRUE(charparam("acktype=r", "acktype=", "gadsr", v));
  TEST_ASSERT_EQUAL('r', v);
}

void test_charparam_invalid_char() {
  char v = '\0';
  TEST_ASSERT_FALSE(charparam("acktype=z", "acktype=", "gadsr", v));
}

void test_charparam_multi_char_rejected() {
  char v = '\0';
  // "ga" after stripping prefix — cmd[1] != '\0', should fail
  TEST_ASSERT_FALSE(charparam("acktype=ga", "acktype=", "gadsr", v));
}

void test_charparam_b9_options() {
  char v = '\0';
  TEST_ASSERT_TRUE(charparam("b9=y", "b9=", "ynksdb", v));
  TEST_ASSERT_EQUAL('y', v);
  TEST_ASSERT_TRUE(charparam("b9=n", "b9=", "ynksdb", v));
  TEST_ASSERT_EQUAL('n', v);
  TEST_ASSERT_TRUE(charparam("b9=s", "b9=", "ynksdb", v));
  TEST_ASSERT_EQUAL('s', v);
}

// ---- sintparam --------------------------------------------------------------

void test_sintparam_positive() {
  int32_t v = 0;
  TEST_ASSERT_TRUE(sintparam("domepos=90", "domepos=", v));
  TEST_ASSERT_EQUAL_INT32(90, v);
}

void test_sintparam_negative() {
  int32_t v = 0;
  TEST_ASSERT_TRUE(sintparam("domepos=-45", "domepos=", v));
  TEST_ASSERT_EQUAL_INT32(-45, v);
}

void test_sintparam_wrong_key() {
  int32_t v = 0;
  TEST_ASSERT_FALSE(sintparam("other=90", "domepos=", v));
}

void test_sintparam_trailing_chars_rejected() {
  int32_t v = 0;
  // Comma after the number means cmd != '\0' at the end check
  TEST_ASSERT_FALSE(sintparam("domepos=90,30", "domepos=", v));
}

// ---- sintparam2 -------------------------------------------------------------

void test_sintparam2_two_values() {
  int32_t v1 = 0, v2 = 0;
  TEST_ASSERT_TRUE(sintparam2("domepos=90,30", "domepos=", v1, v2));
  TEST_ASSERT_EQUAL_INT32(90, v1);
  TEST_ASSERT_EQUAL_INT32(30, v2);
}

void test_sintparam2_negative_values() {
  int32_t v1 = 0, v2 = 0;
  TEST_ASSERT_TRUE(sintparam2("domepos=-90,-30", "domepos=", v1, v2));
  TEST_ASSERT_EQUAL_INT32(-90, v1);
  TEST_ASSERT_EQUAL_INT32(-30, v2);
}

void test_sintparam2_single_value_fails() {
  int32_t v1 = 0, v2 = 0;
  TEST_ASSERT_FALSE(sintparam2("domepos=90", "domepos=", v1, v2));
}

// ---- gestureparam -----------------------------------------------------------

void test_gestureparam_sets_gesture() {
  Gesture g("");
  TEST_ASSERT_TRUE(gestureparam("rnd=3", "rnd=", g));
  TEST_ASSERT_TRUE(g.matches("3"));
}

void test_gestureparam_wrong_key() {
  Gesture g("");
  TEST_ASSERT_FALSE(gestureparam("ack=3", "rnd=", g));
  TEST_ASSERT_TRUE(g.isEmpty());
}

void test_gestureparam_multichar_gesture() {
  Gesture g("");
  TEST_ASSERT_TRUE(gestureparam("slowgest=858", "slowgest=", g));
  TEST_ASSERT_TRUE(g.matches("858"));
}

// ---- numberparams (signed int) -----------------------------------------------

void test_numberparams_int_single() {
  uint8_t cnt = 0;
  int args[5] = {};
  TEST_ASSERT_TRUE(numberparams("7", cnt, args, 5));
  TEST_ASSERT_EQUAL_UINT8(1, cnt);
  TEST_ASSERT_EQUAL_INT(7, args[0]);
}

void test_numberparams_int_three_values() {
  uint8_t cnt = 0;
  int args[5] = {};
  TEST_ASSERT_TRUE(numberparams("1,2,3", cnt, args, 5));
  TEST_ASSERT_EQUAL_UINT8(3, cnt);
  TEST_ASSERT_EQUAL_INT(1, args[0]);
  TEST_ASSERT_EQUAL_INT(2, args[1]);
  TEST_ASSERT_EQUAL_INT(3, args[2]);
}

void test_numberparams_int_negative() {
  uint8_t cnt = 0;
  int args[5] = {};
  TEST_ASSERT_TRUE(numberparams("-10,20", cnt, args, 5));
  TEST_ASSERT_EQUAL_UINT8(2, cnt);
  TEST_ASSERT_EQUAL_INT(-10, args[0]);
  TEST_ASSERT_EQUAL_INT(20, args[1]);
}

void test_numberparams_int_invalid_stops() {
  uint8_t cnt = 0;
  int args[5] = {};
  TEST_ASSERT_FALSE(numberparams("1,abc", cnt, args, 5));
}

// ---- numberparams (uint8) ---------------------------------------------------

void test_numberparams_u8_single() {
  uint8_t cnt = 0;
  uint8_t args[5] = {};
  TEST_ASSERT_TRUE(numberparams("5", cnt, args, 5));
  TEST_ASSERT_EQUAL_UINT8(1, cnt);
  TEST_ASSERT_EQUAL_UINT8(5, args[0]);
}

void test_numberparams_u8_servo_config() {
  // Typical servo config: s=1,0,180,90  → num=1, min=0, max=180, neutral=90
  uint8_t cnt = 0;
  uint8_t args[5] = {};
  TEST_ASSERT_TRUE(numberparams("1,0,180,90", cnt, args, 5));
  TEST_ASSERT_EQUAL_UINT8(4, cnt);
  TEST_ASSERT_EQUAL_UINT8(1,   args[0]);
  TEST_ASSERT_EQUAL_UINT8(0,   args[1]);
  TEST_ASSERT_EQUAL_UINT8(180, args[2]);
  TEST_ASSERT_EQUAL_UINT8(90,  args[3]);
}

void test_numberparams_u8_button_config() {
  // b=1,1,2 → button 1, action=kSound(1), soundbank=2
  uint8_t cnt = 0;
  uint8_t args[5] = {};
  TEST_ASSERT_TRUE(numberparams("1,1,2", cnt, args, 5));
  TEST_ASSERT_EQUAL_UINT8(3, cnt);
  TEST_ASSERT_EQUAL_UINT8(1, args[0]);
  TEST_ASSERT_EQUAL_UINT8(1, args[1]);
  TEST_ASSERT_EQUAL_UINT8(2, args[2]);
}

// ---- main -------------------------------------------------------------------

int main(int argc, char **argv) {
  (void)argc; (void)argv;
  UNITY_BEGIN();

  RUN_TEST(test_startswith_match_advances_pointer);
  RUN_TEST(test_startswith_no_match_leaves_pointer);
  RUN_TEST(test_startswith_empty_needle_always_matches);
  RUN_TEST(test_startswith_exact_match);
  RUN_TEST(test_startswith_partial_no_match);

  RUN_TEST(test_strtolu_simple);
  RUN_TEST(test_strtolu_zero);
  RUN_TEST(test_strtolu_stops_at_non_digit);
  RUN_TEST(test_strtolu_no_digits_returns_zero);
  RUN_TEST(test_strtolu_large_value);

  RUN_TEST(test_strtol_positive);
  RUN_TEST(test_strtol_negative);
  RUN_TEST(test_strtol_zero);
  RUN_TEST(test_strtol_stops_at_comma);

  RUN_TEST(test_boolparam_true);
  RUN_TEST(test_boolparam_false);
  RUN_TEST(test_boolparam_wrong_key);
  RUN_TEST(test_boolparam_invalid_value);

  RUN_TEST(test_intparam_u8_basic);
  RUN_TEST(test_intparam_u8_clamp_max);
  RUN_TEST(test_intparam_u8_clamp_min);
  RUN_TEST(test_intparam_u8_wrong_key);
  RUN_TEST(test_intparam_u8_trailing_chars_rejected);

  RUN_TEST(test_intparam_u16_basic);
  RUN_TEST(test_intparam_u16_clamp);

  RUN_TEST(test_intparam_u32_basic);
  RUN_TEST(test_intparam_u32_clamp_min);

  RUN_TEST(test_charparam_valid_char);
  RUN_TEST(test_charparam_valid_last_char);
  RUN_TEST(test_charparam_invalid_char);
  RUN_TEST(test_charparam_multi_char_rejected);
  RUN_TEST(test_charparam_b9_options);

  RUN_TEST(test_sintparam_positive);
  RUN_TEST(test_sintparam_negative);
  RUN_TEST(test_sintparam_wrong_key);
  RUN_TEST(test_sintparam_trailing_chars_rejected);

  RUN_TEST(test_sintparam2_two_values);
  RUN_TEST(test_sintparam2_negative_values);
  RUN_TEST(test_sintparam2_single_value_fails);

  RUN_TEST(test_gestureparam_sets_gesture);
  RUN_TEST(test_gestureparam_wrong_key);
  RUN_TEST(test_gestureparam_multichar_gesture);

  RUN_TEST(test_numberparams_int_single);
  RUN_TEST(test_numberparams_int_three_values);
  RUN_TEST(test_numberparams_int_negative);
  RUN_TEST(test_numberparams_int_invalid_stops);

  RUN_TEST(test_numberparams_u8_single);
  RUN_TEST(test_numberparams_u8_servo_config);
  RUN_TEST(test_numberparams_u8_button_config);

  return UNITY_END();
}
