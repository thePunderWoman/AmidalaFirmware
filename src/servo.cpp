#include "controller.h"
#include "config.h"

// servoDispatch is the global ServoDispatchDirect instance defined in
// src/globals.cpp. The concrete size (12) matches SizeOfArray(servoSettings).
extern ServoDispatchDirect<12> servoDispatch;

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
