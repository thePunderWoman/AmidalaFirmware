// arduino_mock.h
// Minimal Arduino API stubs for native (host) unit testing via PlatformIO.
// Include this BEFORE any project headers in test files.

#pragma once

#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

// ---- Arduino integer types ----
typedef bool    boolean;
typedef uint8_t byte;

// ---- LOW / HIGH ----
#define LOW  0
#define HIGH 1

// ---- Analog pin numbers (Arduino Mega 2560 mapping) ----
#ifndef A0
#define A0 54
#define A1 55
#define A2 56
#define A3 57
#endif

// ---- Arduino digital I/O and timing stubs ----
static inline uint8_t digitalRead(uint8_t /*pin*/) { return LOW; }
static inline uint32_t micros() { return 0; }

// ---- Arduino min / max / map() ----
// Arduino.h defines min/max as macros (not templates) so they handle
// mixed-type arguments.  Use macros here and set the amidala_core.h sentinel
// so it does not try to add template versions on top.
#ifndef _AMIDALA_MINMAX_DEFINED
#define _AMIDALA_MINMAX_DEFINED
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#endif
static inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// ---- F() macro: on native just passes the string through ----
#define F(s) (s)

// ---- Minimal Print stub (used by ButtonAction::printDescription) ----
class Print {
public:
  virtual size_t write(uint8_t) = 0;
  virtual size_t write(const uint8_t *buf, size_t size) {
    size_t n = 0;
    while (size--) n += write(*buf++);
    return n;
  }
  size_t print(const char *s)  { return write((const uint8_t*)s, strlen(s)); }
  size_t print(char c)         { return write((uint8_t)c); }
  size_t print(int n, int = 10) {
    char buf[32]; snprintf(buf, sizeof(buf), "%d", n);
    return print(buf);
  }
  size_t print(unsigned int n, int base = 10) {
    char buf[32];
    if (base == 16) snprintf(buf, sizeof(buf), "%x", n);
    else snprintf(buf, sizeof(buf), "%u", n);
    return print(buf);
  }
  size_t print(long n, int = 10) {
    char buf[32]; snprintf(buf, sizeof(buf), "%ld", n);
    return print(buf);
  }
  size_t print(unsigned long n, int base = 10) {
    char buf[32];
    if (base == 16) snprintf(buf, sizeof(buf), "%lx", n);
    else snprintf(buf, sizeof(buf), "%lu", n);
    return print(buf);
  }
  size_t println(const char *s) { size_t n = print(s); return n + print("\n"); }
  size_t println(char c)        { size_t n = print(c); return n + print("\n"); }
  size_t println()              { return print("\n"); }
  size_t println(int n, int b = 10)          { size_t r = print(n, b); return r + print("\n"); }
  size_t println(unsigned int n, int b = 10) { size_t r = print(n, b); return r + print("\n"); }
  size_t println(long n, int b = 10)         { size_t r = print(n, b); return r + print("\n"); }
  size_t println(unsigned long n, int b = 10){ size_t r = print(n, b); return r + print("\n"); }
};

// ---- Minimal String stub (capture output in tests) ----
class StringPrint : public Print {
public:
  char buf[512];
  size_t len;
  StringPrint() : len(0) { buf[0] = '\0'; }
  void reset() { len = 0; buf[0] = '\0'; }
  virtual size_t write(uint8_t c) override {
    if (len < sizeof(buf) - 1) { buf[len++] = (char)c; buf[len] = '\0'; }
    return 1;
  }
};

// ---- Serial stub (used by config_reader.h) ----
struct SerialClass {
  void println(const char*) {}
  void println()            {}
  void print(const char*)   {}
  void print(char)          {}
  template<typename T> void println(T) {}
  template<typename T> void print(T)   {}
};
static SerialClass Serial;

// ---- SD / File stubs (used by config_reader.h in SD path) ----

class File {
public:
  File() : fData(nullptr), fPos(0), fValid(false) {}
  explicit File(const char* data)
      : fData(data), fPos(0), fValid(data != nullptr) {}

  explicit operator bool() const { return fValid; }
  bool available() const { return fValid && fData && fData[fPos] != '\0'; }
  char read()            { return (fData && fValid) ? fData[fPos++] : '\0'; }
  void close()           { fValid = false; }

private:
  const char* fData;
  size_t      fPos;
  bool        fValid;
};

struct MockSDClass {
  bool        beginResult  = true;
  const char* fileContent  = nullptr;

  bool begin(int /*pin*/)        { return beginResult; }
  File open(const char* /*name*/) {
    return fileContent ? File(fileContent) : File();
  }
};
static MockSDClass SD;
