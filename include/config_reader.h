#pragma once

// Config Reader
// Provides readConfig() for loading settings from either:
//   - VMusic2 USB board via serial (when VMUSIC_SERIAL is defined)
//   - SD card (default)
//
// Usage in AmidalaController:
//   #ifdef VMUSIC_SERIAL
//     fConsole.setMinimal(!readConfig(fVMusic, fConsole));
//   #else
//     fConsole.setMinimal(!readConfig(fConsole));
//   #endif

#ifdef VMUSIC_SERIAL
// ============================================================
// VMusic2 path
// ============================================================

#include <audio/VMusic.h>

/// Bridges any Console's process(char, bool) method to the VMusic::Parser
/// interface so parseTextFile() can feed characters into the config pipeline.
template <typename Console>
class VMusicConfigParser : public VMusic::Parser {
public:
    explicit VMusicConfigParser(Console& console) : fConsole(console) {}

    virtual void process(char ch) override {
        fConsole.process(ch, true);
    }

private:
    Console& fConsole;
};

/// Read config.txt from the USB drive attached to the VMusic2 board.
/// The VMusic object must already be initialised before calling.
/// Returns true if the file was found and fully processed.
template <typename Console>
bool readConfig(VMusic& vm, Console& console) {
    VMusicConfigParser<Console> parser(console);
    return vm.parseTextFile(parser, "config.txt");
}

#else
// ============================================================
// SD card path
// ============================================================

#ifndef UNIT_TEST
#  include "SD.h"
#  include "SPI.h"
#endif

/// Read config.txt from the SD card (chip-select pin 4).
/// Returns true if the SD initialised, the file opened, and all characters
/// were processed.
template <typename Console>
bool readConfig(Console& console) {
    if (!SD.begin(4)) {
        Serial.println("initialization failed!");
        return false;
    }
    Serial.println("initialization done.");
    File conf = SD.open("config.txt");
    if (conf) {
        while (conf.available()) {
            char ch = conf.read();
            console.process(ch, true);
        }
        conf.close();
        return true;
    }
    Serial.println("error opening config.txt");
    return false;
}

#endif  // VMUSIC_SERIAL
