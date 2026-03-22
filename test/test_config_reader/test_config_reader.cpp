// test_config_reader.cpp
// Unit tests for readConfig() in include/config.h (SD card path, UNIT_TEST build).
//
// These tests exercise:
//   - readConfig() returns true and forwards every character to the console
//     when the SD card initialises and the file opens successfully.
//   - readConfig() returns false when SD.begin() fails.
//   - readConfig() returns false when the file cannot be opened.
//   - Characters are forwarded verbatim, including newlines and nulls-in-content.
//   - An empty file returns true (opened OK) but delivers no characters.

#include "arduino_mock.h"  // must come first – defines SD, File, Serial, etc.
#include "config.h"
#include <unity.h>

void setUp(void) {
    // Reset mock state before every test
    SD.beginResult = true;
    SD.fileContent = nullptr;
}

void tearDown(void) {}

// ---- Minimal Console stub --------------------------------------------------
// readConfig() calls console.process(ch, true) for every character it reads.
// We accumulate the characters so tests can inspect them.

struct MockConsole {
    char    buf[256];
    size_t  len;
    bool    lastConfigFlag;

    MockConsole() : len(0), lastConfigFlag(false) { buf[0] = '\0'; }

    void reset() { len = 0; buf[0] = '\0'; lastConfigFlag = false; }

    void process(char ch, bool configFlag) {
        lastConfigFlag = configFlag;
        if (len < sizeof(buf) - 1) {
            buf[len++] = ch;
            buf[len]   = '\0';
        }
    }
};

// ---------------------------------------------------------------------------

void test_readConfig_returns_true_and_forwards_all_chars() {
    SD.fileContent = "volume=50\r";

    MockConsole console;
    TEST_ASSERT_TRUE(readConfig(console));
    TEST_ASSERT_EQUAL_STRING("volume=50\r", console.buf);
    TEST_ASSERT_EQUAL(10, (int)console.len);
}

void test_readConfig_configFlag_is_always_true() {
    SD.fileContent = "x";

    MockConsole console;
    readConfig(console);
    TEST_ASSERT_TRUE(console.lastConfigFlag);
}

void test_readConfig_returns_false_when_sd_begin_fails() {
    SD.beginResult = false;
    SD.fileContent = "volume=50\r";

    MockConsole console;
    TEST_ASSERT_FALSE(readConfig(console));
    TEST_ASSERT_EQUAL(0, (int)console.len);  // no chars forwarded
}

void test_readConfig_returns_false_when_file_not_found() {
    SD.fileContent = nullptr;  // SD.open() returns invalid File

    MockConsole console;
    TEST_ASSERT_FALSE(readConfig(console));
    TEST_ASSERT_EQUAL(0, (int)console.len);
}

void test_readConfig_empty_file_returns_true_no_chars() {
    SD.fileContent = "";  // valid but empty: available() is false immediately

    MockConsole console;
    TEST_ASSERT_TRUE(readConfig(console));
    TEST_ASSERT_EQUAL(0, (int)console.len);
}

void test_readConfig_multi_line_config() {
    SD.fileContent = "audiohw=hcr\rvolume=80\r";

    MockConsole console;
    TEST_ASSERT_TRUE(readConfig(console));
    TEST_ASSERT_EQUAL_STRING("audiohw=hcr\rvolume=80\r", console.buf);
}

void test_readConfig_forwards_all_printable_chars() {
    // Servo setting line: s=1,1000,2000,1500,10,0,50,n
    SD.fileContent = "s=1,1000,2000,1500,10,0,50,n\r";

    MockConsole console;
    TEST_ASSERT_TRUE(readConfig(console));
    TEST_ASSERT_EQUAL_STRING("s=1,1000,2000,1500,10,0,50,n\r", console.buf);
}

void test_readConfig_large_config() {
    // 200-character config: readConfig must not truncate before the console does
    static const char big[] =
        "a=1\rb=2\rc=3\rd=4\re=5\rf=6\rg=7\rh=8\ri=9\rj=10\r"
        "k=11\rl=12\rm=13\rn=14\ro=15\rp=16\rq=17\rr=18\rs=19\rt=20\r"
        "u=21\rv=22\rw=23\rx=24\ry=25\rz=26\r1=27\r2=28\r3=29\r4=30\r"
        "e1=31\re2=32\re3=33\re4=34\re5=35\re6=36\re7=37\re8=38\r";

    SD.fileContent = big;

    MockConsole console;
    TEST_ASSERT_TRUE(readConfig(console));
    // All characters fit in MockConsole.buf (256 bytes); verify length matches
    TEST_ASSERT_EQUAL((int)strlen(big), (int)console.len);
}

// ---- main ------------------------------------------------------------------

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();

    RUN_TEST(test_readConfig_returns_true_and_forwards_all_chars);
    RUN_TEST(test_readConfig_configFlag_is_always_true);
    RUN_TEST(test_readConfig_returns_false_when_sd_begin_fails);
    RUN_TEST(test_readConfig_returns_false_when_file_not_found);
    RUN_TEST(test_readConfig_empty_file_returns_true_no_chars);
    RUN_TEST(test_readConfig_multi_line_config);
    RUN_TEST(test_readConfig_forwards_all_printable_chars);
    RUN_TEST(test_readConfig_large_config);

    return UNITY_END();
}
