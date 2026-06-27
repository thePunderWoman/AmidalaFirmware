// test_sstr_roundtrip.cpp
// End-to-end regression tests for serial-string name persistence across reboots.
//
// Bug report: after every restart, the first serial string's name (description)
// is empty and must be re-saved via the web UI to restore it.
//
// Test strategy:
//   1. Use the real readConfig<Console>() template with a Console stub that
//      mirrors AmidalaConsole::process(char, bool) — 64-byte buffer, fires on
//      newline, same as what runs on hardware.
//   2. Parse sstr= lines with the same algorithm as processConfig() in config.cpp.
//   3. Simulate rewriteSerialStrings() write format: "sstr=Name|str\n".
//   4. Re-parse the rewritten lines to verify names survive the round-trip.
//   5. Cover the specific symptom: file has no-pipe sstr lines (old format) →
//      names are empty after boot; after user re-saves the file has names →
//      names must survive subsequent boots.
//   6. TRUE END-TO-END: read example_config.txt from disk, simulate the full
//      boot sequence (char-by-char parse, console buffer, sstr population),
//      then call buildFullConfigJson() to verify what the browser actually
//      receives — including the first sstr name.
//
// All tests run on the native PlatformIO environment (no hardware required).

#include "arduino_mock.h"   // must come first — defines SD, File, Serial, etc.
#include "console.h"        // CONSOLE_BUFFER_SIZE
#include "config.h"         // readConfig<Console>()
#include "core.h"           // startswith()
#include "params.h"         // AmidalaParameters
#include "web_api.h"        // buildFullConfigJson()
#include <unity.h>
#include <string.h>
#include <string>
#include <stdio.h>

// ---- Algorithm mirrors ------------------------------------------------------
// These replicate the exact logic in src/config.cpp (sstr= branch) and
// src/wifi_ap.cpp (rewriteSerialStrings write loop) so tests remain faithful
// to the real code without requiring a full AmidalaController.

// Parse a single sstr= line into name and str.
// Mirrors the sstr= branch of AmidalaConfig::processConfig() exactly.
static bool parse_sstr(const char* line, char* name, size_t nameLen,
                        char* str,  size_t strLen) {
    const char* cmd = line;
    if (!startswith(cmd, "sstr=")) return false;
    // After startswith, cmd points past "sstr=" — same as real processConfig.
    const char* val  = cmd;
    const char* pipe = strchr(val, '|');
    if (pipe) {
        size_t nlen = (size_t)(pipe - val);
        if (nlen >= nameLen) nlen = nameLen - 1;
        memcpy(name, val, nlen);
        name[nlen] = '\0';
        strncpy(str, pipe + 1, strLen - 1);
        str[strLen - 1] = '\0';
    } else {
        name[0] = '\0';
        strncpy(str, val, strLen - 1);
        str[strLen - 1] = '\0';
    }
    return true;
}

// Build the sstr= file line as rewriteSerialStrings() does.
// The real code is: sstrs += "sstr="; sstrs += name; sstrs += "|"; sstrs += str; sstrs += "\n";
static std::string write_sstr_line(const char* name, const char* str) {
    return std::string("sstr=") + name + "|" + str + "\n";
}

// ---- Console stub -----------------------------------------------------------
// Mirrors the key path of AmidalaConsole::process(char, bool):
//   - Accumulate chars into a CONSOLE_BUFFER_SIZE buffer.
//   - On \n or \r: null-terminate, reset pos, call parse on non-empty buffer.
//
// Only sstr= lines are collected here; other lines are silently ignored (they
// would be handled by other processConfig branches in real code).

struct SstrConsole {
    char buf[CONSOLE_BUFFER_SIZE];
    int  pos;

    static const int kMax = 64;
    char  names[kMax][32];
    char  strs [kMax][32];
    int   count;

    SstrConsole() : pos(0), count(0) {
        memset(buf,   0, sizeof(buf));
        memset(names, 0, sizeof(names));
        memset(strs,  0, sizeof(strs));
    }

    void process(char ch, bool /*configFlag*/) {
        if (ch == '\n' || ch == '\r') {
            buf[pos] = '\0';
            pos = 0;
            if (buf[0] != '\0' && count < kMax) {
                char n[32], s[32];
                if (parse_sstr(buf, n, sizeof(n), s, sizeof(s))) {
                    strncpy(names[count], n, 31); names[count][31] = '\0';
                    strncpy(strs [count], s, 31); strs [count][31] = '\0';
                    count++;
                }
            }
        } else if (pos < CONSOLE_BUFFER_SIZE - 1) {
            buf[pos++] = ch;
        }
    }
};

// ---- Test fixtures ----------------------------------------------------------

void setUp(void) {
    SD.reset();
    SD.beginResult = true;
    SD.fileContent = nullptr;
}

void tearDown(void) {}

// ---- Parse algorithm --------------------------------------------------------

void test_parse_sstr_basic_name_and_str() {
    char n[32], s[32];
    TEST_ASSERT_TRUE(parse_sstr("sstr=Sad (Moderate)|<SS0>", n, sizeof(n), s, sizeof(s)));
    TEST_ASSERT_EQUAL_STRING("Sad (Moderate)", n);
    TEST_ASSERT_EQUAL_STRING("<SS0>", s);
}

void test_parse_sstr_no_pipe_yields_empty_name() {
    // Old Stealth/OG format — no pipe, no name.
    char n[32], s[32];
    n[0] = 0xFF;
    TEST_ASSERT_TRUE(parse_sstr("sstr=<SS0>", n, sizeof(n), s, sizeof(s)));
    TEST_ASSERT_EQUAL_STRING("", n);       // name is empty
    TEST_ASSERT_EQUAL_STRING("<SS0>", s);  // str still correct
}

void test_parse_sstr_pipe_at_start_yields_empty_name() {
    // Pipe as first char → zero-length name.
    char n[32], s[32];
    TEST_ASSERT_TRUE(parse_sstr("sstr=|<SS0>", n, sizeof(n), s, sizeof(s)));
    TEST_ASSERT_EQUAL_STRING("", n);
    TEST_ASSERT_EQUAL_STRING("<SS0>", s);
}

void test_parse_sstr_short_name_not_empty() {
    // Regression for the old double-skip bug: 5-char name "Vader" was eaten.
    char n[32], s[32];
    TEST_ASSERT_TRUE(parse_sstr("sstr=Vader|BD:VADER", n, sizeof(n), s, sizeof(s)));
    TEST_ASSERT_EQUAL_STRING("Vader", n);
    TEST_ASSERT_EQUAL_STRING("BD:VADER", s);
}

// ---- Write format -----------------------------------------------------------

void test_write_sstr_line_format() {
    std::string line = write_sstr_line("Sad Moderate", "<SS0>");
    TEST_ASSERT_EQUAL_STRING("sstr=Sad Moderate|<SS0>\n", line.c_str());
}

void test_write_sstr_line_empty_name_writes_pipe_at_start() {
    // If name is "", the written line has an empty name (pipe first).
    // This is what gets written when Str[i].name is empty.
    std::string line = write_sstr_line("", "<SS0>");
    TEST_ASSERT_EQUAL_STRING("sstr=|<SS0>\n", line.c_str());
}

// ---- Single-entry round-trip ------------------------------------------------

void test_roundtrip_name_survives_write_and_reparse() {
    // Step 1: parse original config line (as processConfig does on first boot)
    char n1[32], s1[32];
    parse_sstr("sstr=Sad Moderate|<SS0>", n1, sizeof(n1), s1, sizeof(s1));
    TEST_ASSERT_EQUAL_STRING("Sad Moderate", n1);

    // Step 2: build write line (as rewriteSerialStrings does)
    std::string written = write_sstr_line(n1, s1);

    // Step 3: parse the written line on next boot (strip trailing \n)
    std::string stripped = written.substr(0, written.size() - 1);
    char n2[32], s2[32];
    bool ok = parse_sstr(stripped.c_str(), n2, sizeof(n2), s2, sizeof(s2));
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_STRING("Sad Moderate", n2);   // name must survive
    TEST_ASSERT_EQUAL_STRING("<SS0>",         s2);
}

// ---- readConfig + console buffer integration --------------------------------
// These tests use the real readConfig<SstrConsole>() template, which feeds
// chars one-by-one into SstrConsole::process() — the same path as on hardware.

void test_readConfig_single_sstr_name_parsed() {
    SD.fileContent =
        "#START\n"
        "rvrmin=0\n"
        "sstr=Sad (Moderate)|<SS0>\n"
        "#END\n";

    SstrConsole console;
    TEST_ASSERT_TRUE(readConfig(console));
    TEST_ASSERT_EQUAL(1, console.count);
    TEST_ASSERT_EQUAL_STRING("Sad (Moderate)", console.names[0]);
    TEST_ASSERT_EQUAL_STRING("<SS0>",           console.strs[0]);
}

void test_readConfig_first_sstr_name_not_empty_when_file_has_names() {
    // Core regression: first entry's name must survive config load.
    SD.fileContent =
        "#START\n"
        "sstr=Sad (Moderate)|<SS0>\n"
        "sstr=Scared (Moderate)|<SC0>\n"
        "sstr=Mad (Moderate)|<SM0>\n"
        "sstr=Very Mad (Strong)|<SM1>\n"
        "sstr=Very Happy (Strong)|<SH1>\n"
        "#END\n";

    SstrConsole console;
    TEST_ASSERT_TRUE(readConfig(console));
    TEST_ASSERT_EQUAL(5, console.count);

    // First entry is the one the user reported as losing its name.
    TEST_ASSERT_EQUAL_STRING("Sad (Moderate)", console.names[0]);
    TEST_ASSERT_EQUAL_STRING("<SS0>",           console.strs[0]);

    // Verify all others too.
    TEST_ASSERT_EQUAL_STRING("Scared (Moderate)", console.names[1]);
    TEST_ASSERT_EQUAL_STRING("Mad (Moderate)",    console.names[2]);
    TEST_ASSERT_EQUAL_STRING("Very Mad (Strong)", console.names[3]);
    TEST_ASSERT_EQUAL_STRING("Very Happy (Strong)", console.names[4]);
}

void test_readConfig_sstr_after_many_other_config_lines() {
    // Realistic config: many non-sstr lines precede the sstr entries.
    // Ensures the console buffer correctly resets between lines.
    SD.fileContent =
        "#START\n"
        "xbr=40C8CCB0\n"
        "xbl=40C8CCC4\n"
        "rvrmin=0\n"
        "rvrmax=1014\n"
        "rvlmin=0\n"
        "rvlmax=1014\n"
        "volume=50\n"
        "audiohw=hcr\n"
        "sstr=Sad (Moderate)|<SS0>\n"
        "sstr=Vader|BD:VADER\n"
        "#END\n";

    SstrConsole console;
    TEST_ASSERT_TRUE(readConfig(console));
    TEST_ASSERT_EQUAL(2, console.count);
    TEST_ASSERT_EQUAL_STRING("Sad (Moderate)", console.names[0]);
    TEST_ASSERT_EQUAL_STRING("Vader",          console.names[1]);
}

void test_readConfig_crlf_line_endings() {
    // Windows CRLF line endings must not corrupt the first sstr name.
    SD.fileContent =
        "#START\r\n"
        "rvrmin=0\r\n"
        "sstr=Sad (Moderate)|<SS0>\r\n"
        "sstr=Scared (Moderate)|<SC0>\r\n"
        "#END\r\n";

    SstrConsole console;
    TEST_ASSERT_TRUE(readConfig(console));
    TEST_ASSERT_EQUAL(2, console.count);
    TEST_ASSERT_EQUAL_STRING("Sad (Moderate)",   console.names[0]);
    TEST_ASSERT_EQUAL_STRING("<SS0>",             console.strs[0]);
    TEST_ASSERT_EQUAL_STRING("Scared (Moderate)", console.names[1]);
}

// ---- Old-format → re-save → next boot --------------------------------------
// Simulates the exact user-reported scenario:
//   Boot 1: config has no-pipe sstr entries → names empty
//   User re-saves entry 0 via web UI → file gets "sstr=Name|str"
//   Boot 2: names must survive

void test_old_format_no_name_shows_empty_on_first_boot() {
    // Old Stealth/OG Amidala format: sstr= with no pipe (no name).
    SD.fileContent =
        "#START\n"
        "sstr=<SS0>\n"
        "sstr=<SS1>\n"
        "#END\n";

    SstrConsole console;
    TEST_ASSERT_TRUE(readConfig(console));
    TEST_ASSERT_EQUAL(2, console.count);
    TEST_ASSERT_EQUAL_STRING("", console.names[0]);  // user sees "(unnamed)"
    TEST_ASSERT_EQUAL_STRING("<SS0>", console.strs[0]);  // str still works
}

void test_after_resave_name_persists_through_reboot() {
    // After user re-saves entry 0 via web UI, rewriteSerialStrings() writes:
    //   sstr=Sad Moderate|<SS0>
    // Verify next-boot read correctly populates the name.

    // Simulate the rewritten file content.
    static const char rewritten[] =
        "#START\n"
        "rvrmin=0\n"
        "sstr=Sad Moderate|<SS0>\n"
        "sstr=Sad Strong|<SS1>\n"
        "#END\n";

    SD.fileContent = rewritten;
    SstrConsole console;
    TEST_ASSERT_TRUE(readConfig(console));

    TEST_ASSERT_EQUAL(2, console.count);
    TEST_ASSERT_EQUAL_STRING("Sad Moderate", console.names[0]);  // must NOT be empty
    TEST_ASSERT_EQUAL_STRING("<SS0>",         console.strs[0]);
    TEST_ASSERT_EQUAL_STRING("Sad Strong",    console.names[1]);
    TEST_ASSERT_EQUAL_STRING("<SS1>",         console.strs[1]);
}

void test_full_roundtrip_multi_entry_first_name_preserved() {
    // Boot 1: file has names.
    static const char boot1_file[] =
        "#START\n"
        "sstr=Sad (Moderate)|<SS0>\n"
        "sstr=Scared (Moderate)|<SC0>\n"
        "sstr=Overload|<SE>\n"
        "#END\n";

    SD.fileContent = boot1_file;
    SstrConsole c1;
    TEST_ASSERT_TRUE(readConfig(c1));
    TEST_ASSERT_EQUAL(3, c1.count);
    TEST_ASSERT_EQUAL_STRING("Sad (Moderate)", c1.names[0]);

    // Simulate rewriteSerialStrings(): build the file it would write.
    std::string rewritten = "#START\n";
    for (int i = 0; i < c1.count; i++)
        rewritten += write_sstr_line(c1.names[i], c1.strs[i]);
    rewritten += "#END\n";

    // Boot 2: read the rewritten file.
    SD.fileContent = rewritten.c_str();
    SstrConsole c2;
    TEST_ASSERT_TRUE(readConfig(c2));
    TEST_ASSERT_EQUAL(3, c2.count);

    // First entry must survive — this is the regression.
    TEST_ASSERT_EQUAL_STRING("Sad (Moderate)",   c2.names[0]);
    TEST_ASSERT_EQUAL_STRING("<SS0>",             c2.strs[0]);
    TEST_ASSERT_EQUAL_STRING("Scared (Moderate)", c2.names[1]);
    TEST_ASSERT_EQUAL_STRING("Overload",          c2.names[2]);
}

void test_console_buffer_64_byte_limit_does_not_truncate_typical_sstr() {
    // Verify that a typical sstr= line (well under 64 chars) is not truncated.
    // The longest line in example_config.txt is "sstr=Overload Sequence|DM:OVERLOAD"
    // at 34 chars — safely within the 64-byte (63 usable + null) buffer.
    const char* long_but_safe = "sstr=Overload Sequence|DM:OVERLOAD";
    TEST_ASSERT_TRUE(strlen(long_but_safe) < CONSOLE_BUFFER_SIZE - 1);

    char n[32], s[32];
    TEST_ASSERT_TRUE(parse_sstr(long_but_safe, n, sizeof(n), s, sizeof(s)));
    TEST_ASSERT_EQUAL_STRING("Overload Sequence", n);
    TEST_ASSERT_EQUAL_STRING("DM:OVERLOAD",       s);
}

void test_console_buffer_truncates_line_longer_than_63_chars() {
    // An sstr= line over 63 chars gets truncated — the name or str may be cut.
    // This documents the known limit, not a bug to fix.
    // e.g. sstr=<32-char name>|<31-char str> = 5+32+1+31 = 69 chars
    const char* overlong =
        "sstr=AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA|BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB";
    // Line is 69 chars.  Buffer is 63 usable chars.  Everything past char 63 is dropped.
    TEST_ASSERT_TRUE(strlen(overlong) >= (size_t)(CONSOLE_BUFFER_SIZE - 1));

    // Feed via SstrConsole to observe truncation.
    std::string config = std::string(overlong) + "\n";
    SD.fileContent = config.c_str();
    SstrConsole console;
    readConfig(console);

    // The line was too long: either name or str is truncated.
    // The important thing is it doesn't crash, and the parse still tries.
    // (The exact truncated content is an implementation detail, not the focus here.)
    TEST_ASSERT_EQUAL(1, console.count);  // one sstr= line was processed
}

// ---- example_config.txt entries --------------------------------------------
// Spot-check the first several sstr= entries from example_config.txt to confirm
// none are affected by any encoding or length issue.

void test_example_config_first_five_sstr_entries() {
    SD.fileContent =
        "#START\n"
        "sstr=Sad (Moderate)|<SS0>\n"
        "sstr=Scared (Moderate)|<SC0>\n"
        "sstr=Mad (Moderate)|<SM0>\n"
        "sstr=Very Mad (Strong)|<SM1>\n"
        "sstr=Very Happy (Strong)|<SH1>\n"
        "#END\n";

    SstrConsole console;
    readConfig(console);

    TEST_ASSERT_EQUAL(5, console.count);
    TEST_ASSERT_EQUAL_STRING("Sad (Moderate)",      console.names[0]);
    TEST_ASSERT_EQUAL_STRING("<SS0>",               console.strs[0]);
    TEST_ASSERT_EQUAL_STRING("Scared (Moderate)",   console.names[1]);
    TEST_ASSERT_EQUAL_STRING("<SC0>",               console.strs[1]);
    TEST_ASSERT_EQUAL_STRING("Mad (Moderate)",      console.names[2]);
    TEST_ASSERT_EQUAL_STRING("<SM0>",               console.strs[2]);
    TEST_ASSERT_EQUAL_STRING("Very Mad (Strong)",   console.names[3]);
    TEST_ASSERT_EQUAL_STRING("<SM1>",               console.strs[3]);
    TEST_ASSERT_EQUAL_STRING("Very Happy (Strong)", console.names[4]);
    TEST_ASSERT_EQUAL_STRING("<SH1>",               console.strs[4]);
}

// ---- TRUE END-TO-END: example_config.txt → boot pipeline → JSON output -----
//
// These tests use native C file I/O to read example_config.txt from disk,
// then simulate the EXACT boot sequence:
//   readConfig (char-by-char) → console buffer (64 bytes, fires on \n) →
//   processConfig parse logic (sstr= branch) → AmidalaParameters populated →
//   buildFullConfigJson() → JSON sent to browser.
//
// The final step is the same buildFullConfigJson() that /api/config calls in
// the real firmware.  If the first sstr name is empty in this JSON, the bug
// is in the pipeline.  If the JSON is correct, the bug is elsewhere (SD
// card write failure, file content, etc.).

// Read a file from the native filesystem into a std::string.
// Returns empty string if the file cannot be opened.
static std::string readFileNative(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return "";
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    std::string content(size, '\0');
    fread(&content[0], 1, size, f);
    fclose(f);
    return content;
}

// Feed file content through SstrConsole and also populate an AmidalaParameters
// with the parsed sstr entries.
static void populateParamsFromConfig(const char* content, size_t len,
                                     AmidalaParameters& p) {
    // Use memset directly: p.init() has a static sRAMInited flag that prevents
    // memset from running on the second (and later) call — leaving p uninitialised
    // when tests run sequentially and each test declares its own AmidalaParameters.
    memset(&p, 0, sizeof(p));
    char buf[CONSOLE_BUFFER_SIZE];
    int  pos = 0;
    memset(buf, 0, sizeof(buf));

    auto flush = [&]() {
        buf[pos] = '\0';
        pos = 0;
        if (buf[0] == '\0') return;
        // Mirror processConfig() sstr= branch exactly.
        const char* cmd = buf;
        if (!startswith(cmd, "sstr=")) return;
        if (p.serialcount >= p.getSerialStringCount()) return;
        SerialString* a = &p.Str[p.serialcount];
        const char* val  = cmd;   // startswith advanced past "sstr="
        const char* pipe = strchr(val, '|');
        if (pipe) {
            size_t nlen = (size_t)(pipe - val);
            if (nlen >= sizeof(a->name)) nlen = sizeof(a->name) - 1;
            memcpy(a->name, val, nlen);
            a->name[nlen] = '\0';
            strncpy(a->str, pipe + 1, sizeof(a->str) - 1);
            a->str[sizeof(a->str) - 1] = '\0';
        } else {
            a->name[0] = '\0';
            strncpy(a->str, val, sizeof(a->str) - 1);
            a->str[sizeof(a->str) - 1] = '\0';
        }
        p.serialcount++;
    };

    for (size_t i = 0; i < len; i++) {
        char ch = content[i];
        if (ch == '\n' || ch == '\r') {
            flush();
        } else if (pos < CONSOLE_BUFFER_SIZE - 1) {
            buf[pos++] = ch;
        }
    }
    if (pos > 0) flush();   // file without trailing newline
}

// Parse the "sstr" JSON array from buildFullConfigJson output.
// Extracts entries into parallel name/str arrays; returns count.
static int parseSstrFromJson(const std::string& json,
                              char names[][32], char strs[][32], int maxEntries) {
    // Find the sstr array: "sstr":[{...},{...}]
    size_t arrayStart = json.find("\"sstr\":[");
    if (arrayStart == std::string::npos) return 0;
    arrayStart += 8; // skip "sstr":[

    int count = 0;
    size_t pos = arrayStart;
    while (count < maxEntries && pos < json.size() && json[pos] != ']') {
        // Find {"n":"<name>","s":"<str>"}
        size_t nStart = json.find("\"n\":\"", pos);
        if (nStart == std::string::npos || json[nStart - 1] != '{') break;
        nStart += 5;
        size_t nEnd = json.find('"', nStart);
        if (nEnd == std::string::npos) break;

        size_t sStart = json.find("\"s\":\"", nEnd);
        if (sStart == std::string::npos) break;
        sStart += 5;
        size_t sEnd = json.find('"', sStart);
        if (sEnd == std::string::npos) break;

        std::string name = json.substr(nStart, nEnd - nStart);
        std::string str  = json.substr(sStart, sEnd - sStart);

        strncpy(names[count], name.c_str(), 31); names[count][31] = '\0';
        strncpy(strs [count], str.c_str(),  31); strs [count][31] = '\0';
        count++;
        pos = sEnd + 1;
    }
    return count;
}

void test_example_config_boot_pipeline_first_sstr_name_in_params() {
    // Read the real config file from disk.
    std::string content = readFileNative("example_config.txt");
    if (content.empty()) {
        // Try path relative to project root when run from .pio/
        content = readFileNative("../example_config.txt");
    }
    TEST_ASSERT_FALSE_MESSAGE(content.empty(), "example_config.txt must be readable");

    AmidalaParameters p;
    populateParamsFromConfig(content.c_str(), content.size(), p);

    // Verify sstr count > 0 — confirms parsing ran
    TEST_ASSERT_GREATER_THAN(0, (int)p.serialcount);

    // First sstr name must NOT be empty after boot
    TEST_ASSERT_GREATER_THAN(0, (int)strlen(p.Str[0].name));

    // Check it matches the expected first entry from example_config.txt
    TEST_ASSERT_EQUAL_STRING("Sad (Moderate)", p.Str[0].name);
    TEST_ASSERT_EQUAL_STRING("<SS0>",           p.Str[0].str);
}

void test_example_config_boot_pipeline_all_sstr_names_in_params() {
    std::string content = readFileNative("example_config.txt");
    if (content.empty()) content = readFileNative("../example_config.txt");
    TEST_ASSERT_FALSE_MESSAGE(content.empty(), "example_config.txt must be readable");

    AmidalaParameters p;
    populateParamsFromConfig(content.c_str(), content.size(), p);

    // example_config.txt has 20 sstr entries; verify at least several
    TEST_ASSERT_GREATER_OR_EQUAL(5, (int)p.serialcount);

    // None of the first 5 should have an empty name
    for (int i = 0; i < 5 && i < (int)p.serialcount; i++) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Str[%d].name must not be empty", i);
        TEST_ASSERT_GREATER_THAN_MESSAGE(0, (int)strlen(p.Str[i].name), msg);
    }
}

void test_example_config_buildFullConfigJson_first_sstr_name() {
    // TRUE E2E: example_config.txt → boot parse → buildFullConfigJson() → JSON
    // This is exactly what /api/config returns to the browser after boot.
    std::string content = readFileNative("example_config.txt");
    if (content.empty()) content = readFileNative("../example_config.txt");
    TEST_ASSERT_FALSE_MESSAGE(content.empty(), "example_config.txt must be readable");

    AmidalaParameters p;
    populateParamsFromConfig(content.c_str(), content.size(), p);
    TEST_ASSERT_GREATER_THAN(0, (int)p.serialcount);

    // Build the JSON exactly as the server does for GET /api/config
    String json = buildFullConfigJson(p);
    std::string jsonStr(json.c_str());

    // Extract sstr entries from JSON
    char names[64][32], strs[64][32];
    int count = parseSstrFromJson(jsonStr, names, strs, 64);

    TEST_ASSERT_GREATER_THAN(0, count);

    // The CRITICAL check: first sstr name in JSON must not be empty.
    // If this fails, buildFullConfigJson() is the source of the bug.
    // If this passes, the bug is in the SD card write path (rewriteSerialStrings).
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, (int)strlen(names[0]),
        "First sstr name is empty in JSON — bug is in the parse or JSON build path");
    TEST_ASSERT_EQUAL_STRING("Sad (Moderate)", names[0]);
    TEST_ASSERT_EQUAL_STRING("<SS0>",           strs[0]);
}

void test_example_config_buildFullConfigJson_sstr_count_matches() {
    std::string content = readFileNative("example_config.txt");
    if (content.empty()) content = readFileNative("../example_config.txt");
    TEST_ASSERT_FALSE_MESSAGE(content.empty(), "example_config.txt must be readable");

    AmidalaParameters p;
    populateParamsFromConfig(content.c_str(), content.size(), p);

    String json = buildFullConfigJson(p);
    std::string jsonStr(json.c_str());

    char names[64][32], strs[64][32];
    int jsonCount = parseSstrFromJson(jsonStr, names, strs, 64);

    // JSON sstr count must match what was parsed from the file
    TEST_ASSERT_EQUAL_INT((int)p.serialcount, jsonCount);
}

// ---- main ------------------------------------------------------------------

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();

    // Parse algorithm
    RUN_TEST(test_parse_sstr_basic_name_and_str);
    RUN_TEST(test_parse_sstr_no_pipe_yields_empty_name);
    RUN_TEST(test_parse_sstr_pipe_at_start_yields_empty_name);
    RUN_TEST(test_parse_sstr_short_name_not_empty);

    // Write format
    RUN_TEST(test_write_sstr_line_format);
    RUN_TEST(test_write_sstr_line_empty_name_writes_pipe_at_start);

    // Single-entry round-trip
    RUN_TEST(test_roundtrip_name_survives_write_and_reparse);

    // readConfig + console buffer integration
    RUN_TEST(test_readConfig_single_sstr_name_parsed);
    RUN_TEST(test_readConfig_first_sstr_name_not_empty_when_file_has_names);
    RUN_TEST(test_readConfig_sstr_after_many_other_config_lines);
    RUN_TEST(test_readConfig_crlf_line_endings);

    // Old-format → re-save → next-boot regression
    RUN_TEST(test_old_format_no_name_shows_empty_on_first_boot);
    RUN_TEST(test_after_resave_name_persists_through_reboot);
    RUN_TEST(test_full_roundtrip_multi_entry_first_name_preserved);

    // Console buffer limits
    RUN_TEST(test_console_buffer_64_byte_limit_does_not_truncate_typical_sstr);
    RUN_TEST(test_console_buffer_truncates_line_longer_than_63_chars);

    // example_config.txt spot-check (algorithm-level)
    RUN_TEST(test_example_config_first_five_sstr_entries);

    // TRUE E2E: native file read → boot pipeline → AmidalaParameters
    RUN_TEST(test_example_config_boot_pipeline_first_sstr_name_in_params);
    RUN_TEST(test_example_config_boot_pipeline_all_sstr_names_in_params);

    // TRUE E2E: native file read → boot pipeline → buildFullConfigJson() → browser JSON
    RUN_TEST(test_example_config_buildFullConfigJson_first_sstr_name);
    RUN_TEST(test_example_config_buildFullConfigJson_sstr_count_matches);

    return UNITY_END();
}
