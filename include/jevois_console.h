// jevois_console.h
// JevoisConsole: autonomous steering via Jevois camera (experimental).
//
// Only compiled when EXPERIMENTAL_JEVOIS_STEERING is defined.
// Class declaration only — processCommand(), process(), and init() bodies are
// defined in src/jevois_console.cpp (they reference globals tiltservo,
// AUX_SERIAL, and AmidalaController).
//
// Depends on: drive/TargetSteering.h (TargetSteering)

#pragma once

#ifdef EXPERIMENTAL_JEVOIS_STEERING

#ifdef ARDUINO
// Real implementation — pulls in Reeltwo/Arduino headers.
#include "drive/TargetSteering.h"
#else
// Minimal stub for native unit tests.  The full TargetSteering requires
// Reeltwo/Arduino headers (ReelTwo.h → Arduino.h) that are unavailable on the
// host.  All JevoisConsole methods that call TargetSteering are out-of-class
// and compiled only in the embedded build (src/jevois_console.cpp).
#ifndef TargetSteering_h
#define TargetSteering_h
struct TargetSteering {
    TargetSteering(int, int = 0) {}
    void setCurrentDistance(int) {}
    void setCurrentAngle(int) {}
    void lost() {}
    int getThrottle() const { return 0; }
    int getTurning() const { return 0; }
};
#endif
#endif

class AmidalaController;

class JevoisConsole {
public:
  JevoisConsole() : fSteering(300), fPos(0), fLastTime(0) {}
  virtual ~JevoisConsole() {}

  void init(AmidalaController *controller);
  void processCommand(char *cmd = nullptr);
  void process();

private:
  AmidalaController *fController = nullptr;
  TargetSteering fSteering;
  char fBuffer[128];
  unsigned fPos = 0;
  uint32_t fLastTime;
};

#endif // EXPERIMENTAL_JEVOIS_STEERING
