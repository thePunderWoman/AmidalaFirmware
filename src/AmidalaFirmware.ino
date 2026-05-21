// AmidalaFirmware.ino — entry point.
// Hardware globals live in src/globals.cpp.
// Controller logic lives in src/controller.cpp.

#include "debug.h"
#include "drive_config.h"
#include "controller.h"

AmidalaController amidala;

void setup() {
  REELTWO_READY();

  randomSeed(analogRead(ANALOG1_PIN));

  // Initialise the SPI bus with the custom GPIOs before any SPI device (SD,
  // XBee) calls begin().
  SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, -1);

  CONSOLE_SERIAL.begin(DEFAULT_BAUD_RATE);
  // XBee serial init omitted — XBee 3 uses SPI (see PR esp32-2).
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
