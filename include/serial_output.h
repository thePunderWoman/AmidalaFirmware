#pragma once
// Lightweight serial output helpers — depend only on Print (Arduino / mock).
// Kept in a separate header so they can be unit-tested without pulling in
// the full AmidalaController / ReelTwo dependency chain.

// writeEolTo: write the configured EOL byte(s) to a Print destination.
// eol == 0 is the CRLF sentinel (\r\n); any other value writes that byte.
inline void writeEolTo(Print& out, uint8_t eol) {
  if (eol == 0) {
    out.write('\r');
    out.write('\n');
  } else {
    out.write(eol);
  }
}

// sendSerialStringTo: write str to out, replacing every occurrence of delim
// with the configured EOL, then append the EOL at the end.
inline void sendSerialStringTo(Print& out, const char* str,
                                uint8_t delim, uint8_t eol) {
  char ch;
  while ((ch = *str++) != '\0') {
    if (ch == (char)delim)
      writeEolTo(out, eol);
    else
      out.write((uint8_t)ch);
  }
  writeEolTo(out, eol);
}
