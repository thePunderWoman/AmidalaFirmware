// esp32_compat.h
// Compatibility shims for third-party libraries (HCR) that target Arduino AVR
// but are compiled for ESP32-S3.  Force-included via -include in build_flags.
//
// Do NOT include this file manually — it is injected by the build system.

#pragma once

#ifdef ARDUINO_ARCH_ESP32

// ESP32's TwoWire uses setTimeout(ms) instead of AVR's setWireTimeout(us, reset).
// The HCR library calls _i2c->setWireTimeout(3000, true) (3000 µs = 3 ms).
#ifndef setWireTimeout
#define setWireTimeout(timeout_us, reset_on_timeout) setTimeout((timeout_us) / 1000)
#endif

#endif // ARDUINO_ARCH_ESP32
