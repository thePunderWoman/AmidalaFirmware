#include "controller.h"

#ifdef EXPERIMENTAL_JEVOIS_STEERING

// tiltservo is a ServoPD global defined in AmidalaFirmware.ino.
extern ServoPD tiltservo;

void JevoisConsole::init(AmidalaController *controller) {
  fController = controller;
  fController->setTargetSteering(&fSteering);
}

void JevoisConsole::processCommand(char *cmd) {
  int id, targx, targy, targw, targh;
  char *tok = (cmd != nullptr) ? strtok(cmd, " \r") : nullptr;
  int state = 0;

  while (tok) {
    // State machine:
    // 0: start parsing
    // 1: N2 command, parse id
    // 2: N2 command, parse targx
    // 3: N2 command, parse targy
    // 4: N2 command, parse targw
    // 5: N2 command, parse targh
    // 6: N2 command complete
    // 1000: unknown command
    switch (state) {
    case 0:
      state = (strcmp(tok, "N2") == 0) ? 1 : 1000;
      break;
    case 1:
      id = atoi(&tok[1]);
      state = 2;
      break;
    case 2:
      targx = atoi(tok);
      state = 3;
      break;
    case 3:
      targy = atoi(tok);
      state = 4;
      break;
    case 4:
      targw = atoi(tok);
      state = 5;
      break;
    case 5:
      targh = atoi(tok);
      state = 6;
      break;
    default:
      break;
    }
    tok = strtok(0, " \r\n");
  }

  // If a complete new N2 command was received, act:
  if (state == 6) {
    // panservo.update(-targx);
    tiltservo.update(targy);

    fSteering.setCurrentDistance(targw);
    // fSteering.setCurrentAngle(panservo.get() - PANZERO);
    fSteering.setCurrentAngle(targx);

    // CONSOLE_SERIAL.print("[");
    // CONSOLE_SERIAL.print(fSteering.getThrottle());
    // CONSOLE_SERIAL.print(",");
    // CONSOLE_SERIAL.print(fSteering.getTurning());
    // CONSOLE_SERIAL.print(","); CONSOLE_SERIAL.print(targx);
    // CONSOLE_SERIAL.println("]");
  } else {
    // Slow down if we lost track:
    fSteering.lost();
  }
  (void)id; (void)targh; // suppress unused-variable warnings
}

void JevoisConsole::process() {
  while (AUX_SERIAL.available()) {
    int ch = AUX_SERIAL.read();
    if (ch == '\n') {
      fBuffer[fPos] = '\0';
      fPos = 0;
      if (*fBuffer != '\0') {
        processCommand(fBuffer);
      }
    } else if (fPos < SizeOfArray(fBuffer) - 1) {
      fBuffer[fPos++] = ch;
    }
    fLastTime = millis();
  }
  if (fLastTime + 50 < millis()) {
    processCommand();
    fLastTime = millis();
  }
}

#endif  // EXPERIMENTAL_JEVOIS_STEERING
