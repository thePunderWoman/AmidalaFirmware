#include "amidala_controller.h"

// servoDispatch is the global ServoDispatchDirect instance defined in
// AmidalaFirmware.ino. The concrete size (12) matches SizeOfArray(servoSettings).
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
