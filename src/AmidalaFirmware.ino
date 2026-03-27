// AmidalaFirmware.ino — entry point.
// Hardware globals live in src/globals.cpp.
// Controller logic lives in src/controller.cpp.

#include "debug.h"
#include "drive_config.h"
#include "controller.h"

AmidalaController amidala;

void setup() {
  REELTWO_READY();

  randomSeed(analogRead(3));

  CONSOLE_SERIAL.begin(DEFAULT_BAUD_RATE);
  XBEE_SERIAL.begin(57600);
#ifdef VMUSIC_SERIAL
  VMUSIC_SERIAL.begin(9600);
#endif
#ifdef DRIVE_SERIAL
  DRIVE_SERIAL.begin(DRIVE_BAUD_RATE);
#elif defined(DOME_DRIVE_SERIAL)
  DOME_DRIVE_SERIAL.begin(DRIVE_BAUD_RATE);
#elif defined(RDH_SERIAL)
  RDH_SERIAL.begin(RDH_BAUD_RATE);
#else
  SERIAL.begin(115200);
#endif

  SetupEvent::ready();
}

void loop() { AnimatedEvent::process(); }
