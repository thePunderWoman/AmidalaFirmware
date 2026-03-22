#include "controller.h"
#include "config.h"

// servoDispatch is defined in src/globals.cpp as a ServoDispatch& reference to
// the concrete ServoDispatchDirect instance.  Using the base-class reference
// avoids including ServoDispatchDirect.h here, which would duplicate the ISR
// handlers defined in ServoDispatchPrivate.h.
extern ServoDispatch& servoDispatch;

void AmidalaConsole::printServoPos(uint16_t num) {
  if (servoDispatch.isActive(num)) {
    printNum(servoDispatch.currentPos(num));
  } else {
    print("----");
  }
}

void AmidalaConsole::setServo() {
  // Not supported
  println(F("Invalid"));
}

void AmidalaConfig::applyServoConfig(unsigned num, uint16_t minpulse,
                                     uint16_t maxpulse, float neutral) {
  servoDispatch.setServo(num, SERVO1_PIN + num, minpulse, maxpulse,
                         neutral * maxpulse, 0);
}
