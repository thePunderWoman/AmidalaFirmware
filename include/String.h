// String.h — platform compatibility shim.
//
// HumanCyborgRelationsAPI uses #include <String.h> (capital S) outside its
// #ifdef ARDUINO guard.  On case-insensitive macOS the compiler resolves this
// to the standard C <string.h>; on case-sensitive Linux it fails.
//
// Placing this file in the project include/ directory (which is on the
// compiler's search path before the system headers) intercepts the include on
// all platforms and delegates to the lowercase standard header.  Arduino builds
// already have the String class from Arduino.h, so this shim is harmless.
#pragma once
#include_next <string.h>
