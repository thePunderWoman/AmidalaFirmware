// test_amidala_params.cpp
// Tests for include/amidala_params.h:
//   AmidalaParameters — default values, EEPROM load, capacity constants

#include "arduino_mock.h"
#include "amidala_params.h"
#include <unity.h>
#include <string.h>

// Each test gets a fresh AmidalaParameters to avoid static init() caching.
// We reinitialise the mock EEPROM in setUp.
void setUp(void) {
    // Reset EEPROM mock to all zeros before each test.
    memset(EEPROM.data, 0, sizeof(EEPROM.data));
}
void tearDown(void) {}

// ---- Audio hardware constants -----------------------------------------------

void test_audio_hw_constants() {
    TEST_ASSERT_EQUAL(1, AUDIO_HW_HCR);
    TEST_ASSERT_EQUAL(2, AUDIO_HW_VMUSIC);
}

// ---- Array capacities --------------------------------------------------------

void test_sound_bank_count() {
    AmidalaParameters p;
    TEST_ASSERT_EQUAL(20, p.getSoundBankCount());
}

void test_servo_count() {
    AmidalaParameters p;
    TEST_ASSERT_EQUAL(12, p.getServoCount());
}

void test_button_count() {
    AmidalaParameters p;
    TEST_ASSERT_EQUAL(9, p.getButtonCount());
}

void test_gesture_count() {
    AmidalaParameters p;
    TEST_ASSERT_EQUAL(10, p.getGestureCount());
}

void test_aux_string_count() {
    AmidalaParameters p;
    TEST_ASSERT_EQUAL(MAX_AUX_STRINGS, (int)p.getAuxStringCount());
}

// ---- Default values after init() (no EEPROM signature) ----------------------
// The static sInited/sRAMInited flags mean we can only test one instance per
// process run — use a single AmidalaParameters here for the default tests.

static AmidalaParameters gDefaultParams;

void test_default_volume() {
    gDefaultParams.init();
    TEST_ASSERT_EQUAL(50, gDefaultParams.volume);
}

void test_default_startup_true() {
    gDefaultParams.init();
    TEST_ASSERT_TRUE(gDefaultParams.startup);
}

void test_default_rndon_true() {
    gDefaultParams.init();
    TEST_ASSERT_TRUE(gDefaultParams.rndon);
}

void test_default_ackon_false() {
    gDefaultParams.init();
    TEST_ASSERT_FALSE(gDefaultParams.ackon);
}

void test_default_mindelay() {
    gDefaultParams.init();
    TEST_ASSERT_EQUAL(60, gDefaultParams.mindelay);
}

void test_default_maxdelay() {
    gDefaultParams.init();
    TEST_ASSERT_EQUAL(120, gDefaultParams.maxdelay);
}

void test_default_auxbaud() {
    gDefaultParams.init();
    TEST_ASSERT_EQUAL(9600UL, gDefaultParams.auxbaud);
}

void test_default_auxdelim() {
    gDefaultParams.init();
    TEST_ASSERT_EQUAL(':', gDefaultParams.auxdelim);
}

void test_default_auxeol() {
    gDefaultParams.init();
    TEST_ASSERT_EQUAL(13, gDefaultParams.auxeol);
}

void test_default_audiohw_is_hcr() {
    gDefaultParams.init();
    TEST_ASSERT_EQUAL(AUDIO_HW_HCR, gDefaultParams.audiohw);
}

void test_default_dome_home_position() {
    gDefaultParams.init();
    TEST_ASSERT_EQUAL(DEFAULT_DOME_HOME_POSITION, gDefaultParams.domehome);
}

void test_default_dome_speed_home() {
    gDefaultParams.init();
    TEST_ASSERT_EQUAL(DEFAULT_DOME_SPEED_HOME, gDefaultParams.domespeedhome);
}

void test_default_min_pulse() {
    gDefaultParams.init();
    TEST_ASSERT_EQUAL(DEFAULT_DOME_MIN_PULSE, gDefaultParams.minpulse);
}

void test_default_max_pulse() {
    gDefaultParams.init();
    TEST_ASSERT_EQUAL(DEFAULT_DOME_MAX_PULSE, gDefaultParams.maxpulse);
}

// ---- EEPROM serial number load (DB01 signature) -----------------------------

void test_eeprom_serial_loaded_when_db01_signature_present() {
    // Write 'D','B','0','1',0 signature then serial "R2D2"
    EEPROM.data[0] = 'D'; EEPROM.data[1] = 'B';
    EEPROM.data[2] = '0'; EEPROM.data[3] = '1'; EEPROM.data[4] = 0;
    EEPROM.data[5] = 'R'; EEPROM.data[6] = '2';
    EEPROM.data[7] = 'D'; EEPROM.data[8] = '2'; EEPROM.data[9] = 0;

    // sInited is already set by the default-value tests above; use forceReload
    // and zero the instance first so unloaded fields start clean.
    AmidalaParameters p;
    memset(&p, 0, sizeof(p));
    p.init(true);
    TEST_ASSERT_EQUAL('R', p.serial[0]);
    TEST_ASSERT_EQUAL('2', p.serial[1]);
    TEST_ASSERT_EQUAL('D', p.serial[2]);
    TEST_ASSERT_EQUAL('2', p.serial[3]);
}

void test_eeprom_serial_not_loaded_when_no_signature() {
    // All zeros — no DB01 signature.
    AmidalaParameters p;
    memset(&p, 0, sizeof(p));
    p.init(true);
    TEST_ASSERT_EQUAL(0, p.serial[0]);
}

// ---- EEPROM XBee address load (SC23 signature) ------------------------------

void test_eeprom_xbr_loaded_when_sc23_signature_present() {
    // SC23 signature at offset 0x64
    int base = 0x64;
    EEPROM.data[base + 0] = 'S'; EEPROM.data[base + 1] = 'C';
    EEPROM.data[base + 2] = '2'; EEPROM.data[base + 3] = '3';
    EEPROM.data[base + 4] = 0;
    // xbr little-endian at base+5: 0x01020304
    EEPROM.data[base + 5] = 0x04;  // byte 0 (LSB)
    EEPROM.data[base + 6] = 0x03;
    EEPROM.data[base + 7] = 0x02;
    EEPROM.data[base + 8] = 0x01;  // byte 3 (MSB)
    // xbl little-endian at base+9: 0x05060708
    EEPROM.data[base + 9]  = 0x08;
    EEPROM.data[base + 10] = 0x07;
    EEPROM.data[base + 11] = 0x06;
    EEPROM.data[base + 12] = 0x05;

    AmidalaParameters p;
    memset(&p, 0, sizeof(p));
    p.init(true);
    TEST_ASSERT_EQUAL_HEX32(0x01020304, p.xbr);
    TEST_ASSERT_EQUAL_HEX32(0x05060708, p.xbl);
}

// ---- getRadioChannelCount lazy-init -----------------------------------------

void test_get_radio_channel_count_returns_default_zero_when_eeprom_absent() {
    // Zero the instance explicitly — sInited is already set so init() will
    // return early; rcchn must come from our memset, not from defaults.
    AmidalaParameters p;
    memset(&p, 0, sizeof(p));
    TEST_ASSERT_EQUAL(0, p.getRadioChannelCount());
}

// ---- main -------------------------------------------------------------------

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();

    RUN_TEST(test_audio_hw_constants);

    RUN_TEST(test_sound_bank_count);
    RUN_TEST(test_servo_count);
    RUN_TEST(test_button_count);
    RUN_TEST(test_gesture_count);
    RUN_TEST(test_aux_string_count);

    RUN_TEST(test_default_volume);
    RUN_TEST(test_default_startup_true);
    RUN_TEST(test_default_rndon_true);
    RUN_TEST(test_default_ackon_false);
    RUN_TEST(test_default_mindelay);
    RUN_TEST(test_default_maxdelay);
    RUN_TEST(test_default_auxbaud);
    RUN_TEST(test_default_auxdelim);
    RUN_TEST(test_default_auxeol);
    RUN_TEST(test_default_audiohw_is_hcr);
    RUN_TEST(test_default_dome_home_position);
    RUN_TEST(test_default_dome_speed_home);
    RUN_TEST(test_default_min_pulse);
    RUN_TEST(test_default_max_pulse);

    RUN_TEST(test_eeprom_serial_loaded_when_db01_signature_present);
    RUN_TEST(test_eeprom_serial_not_loaded_when_no_signature);
    RUN_TEST(test_eeprom_xbr_loaded_when_sc23_signature_present);

    RUN_TEST(test_get_radio_channel_count_returns_default_zero_when_eeprom_absent);

    return UNITY_END();
}
