// test_serial_output.cpp
// Tests for writeEolTo() and sendSerialStringTo() in serial_output.h.
// These free functions contain all the EOL logic that AmidalaController
// delegates to — tested here in isolation via MockStream.

#include "arduino_mock.h"
#include "serial_output.h"
#include <unity.h>
#include <string.h>

void setUp()    {}
void tearDown() {}

// ---------------------------------------------------------------------------
// writeEolTo
// ---------------------------------------------------------------------------

void test_writeEol_lf_writes_single_lf() {
    MockStream s;
    writeEolTo(s, 10);
    TEST_ASSERT_EQUAL(1, (int)s.outLen);
    TEST_ASSERT_EQUAL('\n', s.outBuf[0]);
}

void test_writeEol_cr_writes_single_cr() {
    MockStream s;
    writeEolTo(s, 13);
    TEST_ASSERT_EQUAL(1, (int)s.outLen);
    TEST_ASSERT_EQUAL('\r', s.outBuf[0]);
}

void test_writeEol_crlf_sentinel_writes_cr_then_lf() {
    MockStream s;
    writeEolTo(s, 0); // 0 = CRLF sentinel
    TEST_ASSERT_EQUAL(2, (int)s.outLen);
    TEST_ASSERT_EQUAL('\r', s.outBuf[0]);
    TEST_ASSERT_EQUAL('\n', s.outBuf[1]);
}

void test_writeEol_arbitrary_byte_writes_that_byte() {
    MockStream s;
    writeEolTo(s, '|');
    TEST_ASSERT_EQUAL(1, (int)s.outLen);
    TEST_ASSERT_EQUAL('|', s.outBuf[0]);
}

// ---------------------------------------------------------------------------
// sendSerialStringTo
// ---------------------------------------------------------------------------

void test_sendSerial_appends_eol_after_string() {
    MockStream s;
    sendSerialStringTo(s, "ABC", ':', 10);
    TEST_ASSERT_EQUAL_STRING("ABC\n", s.outBuf);
}

void test_sendSerial_replaces_delimiter_with_eol() {
    MockStream s;
    // Colon delimiter, LF EOL: "A:B" → "A\nB\n"
    sendSerialStringTo(s, "A:B", ':', 10);
    TEST_ASSERT_EQUAL_STRING("A\nB\n", s.outBuf);
}

void test_sendSerial_delimiter_with_crlf_eol() {
    MockStream s;
    // Colon delimiter, CRLF (0) EOL: "A:B" → "A\r\nB\r\n"
    sendSerialStringTo(s, "A:B", ':', 0);
    TEST_ASSERT_EQUAL(6, (int)s.outLen);
    TEST_ASSERT_EQUAL('A',  s.outBuf[0]);
    TEST_ASSERT_EQUAL('\r', s.outBuf[1]);
    TEST_ASSERT_EQUAL('\n', s.outBuf[2]);
    TEST_ASSERT_EQUAL('B',  s.outBuf[3]);
    TEST_ASSERT_EQUAL('\r', s.outBuf[4]);
    TEST_ASSERT_EQUAL('\n', s.outBuf[5]);
}

void test_sendSerial_empty_string_writes_only_eol() {
    MockStream s;
    sendSerialStringTo(s, "", ':', 13);
    TEST_ASSERT_EQUAL(1, (int)s.outLen);
    TEST_ASSERT_EQUAL('\r', s.outBuf[0]);
}

void test_sendSerial_multiple_delimiters_each_become_eol() {
    MockStream s;
    // "A:B:C" with LF → "A\nB\nC\n"
    sendSerialStringTo(s, "A:B:C", ':', 10);
    TEST_ASSERT_EQUAL_STRING("A\nB\nC\n", s.outBuf);
}

void test_sendSerial_no_delimiter_in_string_just_appends_eol() {
    MockStream s;
    sendSerialStringTo(s, "HELLO", ':', 10);
    TEST_ASSERT_EQUAL_STRING("HELLO\n", s.outBuf);
}

void test_sendSerial_crlf_eol_at_end_of_plain_string() {
    MockStream s;
    // "HI" with CRLF → "HI\r\n"
    sendSerialStringTo(s, "HI", ':', 0);
    TEST_ASSERT_EQUAL(4, (int)s.outLen);
    TEST_ASSERT_EQUAL('H',  s.outBuf[0]);
    TEST_ASSERT_EQUAL('I',  s.outBuf[1]);
    TEST_ASSERT_EQUAL('\r', s.outBuf[2]);
    TEST_ASSERT_EQUAL('\n', s.outBuf[3]);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();

    RUN_TEST(test_writeEol_lf_writes_single_lf);
    RUN_TEST(test_writeEol_cr_writes_single_cr);
    RUN_TEST(test_writeEol_crlf_sentinel_writes_cr_then_lf);
    RUN_TEST(test_writeEol_arbitrary_byte_writes_that_byte);

    RUN_TEST(test_sendSerial_appends_eol_after_string);
    RUN_TEST(test_sendSerial_replaces_delimiter_with_eol);
    RUN_TEST(test_sendSerial_delimiter_with_crlf_eol);
    RUN_TEST(test_sendSerial_empty_string_writes_only_eol);
    RUN_TEST(test_sendSerial_multiple_delimiters_each_become_eol);
    RUN_TEST(test_sendSerial_no_delimiter_in_string_just_appends_eol);
    RUN_TEST(test_sendSerial_crlf_eol_at_end_of_plain_string);

    return UNITY_END();
}
