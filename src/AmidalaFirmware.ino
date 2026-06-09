// AmidalaFirmware.ino — entry point.
// Hardware globals live in src/globals.cpp.
// Controller logic lives in src/controller.cpp.

#include "debug.h"
#include "drive_config.h"
#include "controller.h"

AmidalaController amidala;

// Temporary SPI/SD diagnostic.  Sends CMD0 directly at 400 kHz and prints
// the raw response so we can tell whether SPI signals are reaching the card.
// 0x01 = card responded (idle)  |  0xFF = no response (wiring/power issue)
// Remove once SD card connectivity is confirmed.
static void spiSdDiag() {
    SPI.beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0));
    // 80-clock wake-up with CS deasserted (SD spec requirement)
    digitalWrite(SD_CS_PIN, HIGH);
    for (int i = 0; i < 10; i++) SPI.transfer(0xFF);
    // CMD0 GO_IDLE_STATE
    digitalWrite(SD_CS_PIN, LOW);
    SPI.transfer(0x40);
    SPI.transfer(0x00); SPI.transfer(0x00);
    SPI.transfer(0x00); SPI.transfer(0x00);
    SPI.transfer(0x95);  // CRC7 for CMD0 + stop bit
    // Poll up to 8 bytes for a non-0xFF response
    uint8_t resp = 0xFF;
    for (int i = 0; i < 8 && resp == 0xFF; i++) {
        resp = SPI.transfer(0xFF);
    }
    digitalWrite(SD_CS_PIN, HIGH);
    SPI.endTransaction();
    CONSOLE_SERIAL.printf("[SD diag] CMD0 raw response: 0x%02X  %s\n",
        resp,
        resp == 0x01 ? "-> OK, card in idle state" :
        resp == 0xFF ? "-> no response — check SPI wiring / SD power" :
                       "-> unexpected value");
}

void setup() {
  REELTWO_READY();

  randomSeed(analogRead(ANALOG1_PIN));

  // Drive all SPI CS pins HIGH before touching the bus so no slave sees a
  // spurious chip-select during initialisation.  GPIOs boot as floating
  // inputs; without this, SD_CS can sit LOW long enough to confuse the card.
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  pinMode(XBEE_CS_PIN, OUTPUT);
  digitalWrite(XBEE_CS_PIN, HIGH);
  pinMode(SPI_SPARE_CS_PIN, OUTPUT);
  digitalWrite(SPI_SPARE_CS_PIN, HIGH);

  SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, -1);
  // MISO is open-drain on most SD cards; pull it up so the line isn't
  // floating when no device is actively driving it.
  pinMode(SPI_MISO_PIN, INPUT_PULLUP);

  CONSOLE_SERIAL.begin(DEFAULT_BAUD_RATE);
  // Wait up to 3 s for USB-CDC to connect so boot log messages (including SD
  // init warnings) are visible on the monitor before SD.begin() is called.
  { uint32_t t = millis(); while (!CONSOLE_SERIAL && millis() - t < 3000) delay(10); }
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

  spiSdDiag();
}

void loop() { AnimatedEvent::process(); }
