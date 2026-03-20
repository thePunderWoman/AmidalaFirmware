# Amidala Firmware Modularization

Tracking the incremental extraction of `src/AmidalaFirmware.ino` into focused header files.
Each item gets its own branch, new `.h` file, tests, and a confirmed clean build before merging.

| Priority | Module | Header | What it contains | Status |
|----------|--------|--------|-----------------|--------|
| High | Pin configuration | ~~`include/pin_config.h`~~ | All hardware pin `#define`s (servos, digital outs, PPM, analog, serial ports) | ~~Done — PR #8~~ |
| High | Drive configuration | ~~`include/drive_config.h`~~ | Drive/dome system selection, speed/accel constants, all `DEFAULT_DOME_*` constants, derived `CHANNEL_MIXING`/`DRIVE_SERIAL` | ~~Done — PR #9~~ |
| High | Button & gesture actions | ~~`include/button_actions.h`~~ | `ButtonAction`, `GestureAction`, `AuxString` structs | ~~Done — PR #10~~ |
| High | PPM decoder | ~~`include/ppm_decoder.h`~~ | `PPMDecoder` class | ~~Done — PR #11~~ |
| High | RDH serial | ~~`include/rdh_serial.h`~~ | `RDHSerial` class (dome position sensor + command interface) | ~~Done — PR #12~~ |
| High | I2C utilities | ~~`include/i2c_utils.h`~~ | `recoverI2CBus()`, `sendI2CCmd()`, `sendI2CStr()` | ~~Done — PR #13~~ |
| Medium | XBee remote | `include/xbee_remote.h` | `XBeePocketRemote`, `DriveController`, `DomeController` | To do |
| Medium | Amidala parameters | `include/amidala_params.h` | `AmidalaParameters` struct + EEPROM load/save | To do |
| Medium | Console | `include/console.h` | `AmidalaConsole` class | To do |
| Low | Drive & dome controllers | `include/xbee_controllers.h` | `DriveController` and `DomeController` (inner classes of `AmidalaController` that extend `XBeePocketRemote`) | To do |
| Low | Jevois camera steering | `include/jevois_console.h` | `JevoisConsole` class (experimental autonomous steering via Jevois camera, guarded by `EXPERIMENTAL_JEVOIS_STEERING`) | To do |
