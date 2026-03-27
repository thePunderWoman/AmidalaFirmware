# Amidala Firmware Mark 2

A firmware control system for an astromech or similar droid that uses dual XBee remote joystick controllers.

This is a fork of [Skelmir's (rimim) original Amidala Firmware](https://github.com/reeltwo/AmidalaFirmware).

## Hardware

- **Microcontroller:** Arduino Mega 2560
- **Shield:** DFRobot Mega Sensor Shield V2.4
- **Configuration storage:** Micro SD card (config.txt loaded at startup)

## Features

### Remote Control

Dual [XBee Pocket Remotes](https://reeltwo.github.io/Reeltwo) provide wireless control:

- **Drive controller (left XBee):** Drive throttle and button inputs
- **Dome controller (right XBee):** Dome rotation, volume control, and gesture input
- Graceful failsafe handling on signal loss

An optional analog **RC receiver (PPM)** is also supported for drive control.

### Drive Systems

Drive system is selected at compile time. Supported options:

- PWM (default)
- Sabertooth serial
- RoboteQ PWM, serial, or PWM+Serial hybrid

### Audio

Two audio systems are supported, selectable at runtime via configuration:

- **Human Cyborg Relations (HCR) board** (default) — three independent sound channels (Voice, Ambient, Background) with emotion/emote support and Muse mode for ambient random playback
- **VMusic2** — USB-based audio with directory-organized sound banks on a USB drive

### Button Configuration

Up to nine buttons are configurable, each supporting a short-press and long-press action. Button actions include:

- Sound bank playback
- Servo positioning
- Digital output control
- I2C command or string output
- Auxiliary serial string output
- HCR emote trigger
- HCR Muse toggle

### Gestures

Up to 10 custom gestures can be mapped to any button action. Built-in gestures include toggles for random sound playback, slow/fast speed mode, and acknowledgement sounds.

### Roam-a-Dome

Autonomous dome rotation via an optional RDH serial position sensor. Supports home positioning, random seek, and target positioning modes with configurable speeds, angles, and delays.

### RC Control

PPM RC receiver input for drive and dome control, with configurable failsafe timeout.

### Serial and Auxiliary I2C

- **Auxiliary serial** (Serial3): 40 configurable string templates sent to downstream devices on button/gesture actions
- **I2C**: Send command bytes or strings to I2C devices by configured address

## Configuration

Runtime configuration is loaded from `config.txt` on the micro SD card at startup. Settings control feature toggles, audio hardware selection, sound banks, button and gesture mappings, dome behavior, volume, and more — no recompilation required for most changes. See [current_config.txt](current_config.txt) for a working example.

Legacy calibration data (XBee addresses, servo calibration, joystick calibration) is stored in EEPROM.

## Wiring

### Dependencies

Install [PlatformIO](https://platformio.org/) — it handles all library dependencies automatically on first build. The firmware depends on:

- [Reeltwo](https://github.com/reeltwo/Reeltwo)
- [ReeltwoAudio](https://github.com/reeltwo/ReeltwoAudio)

### XBee receiver

Connect the XBee module to the **COM1** header on the DFRobot Mega Sensor Shield. This maps to Serial1 (Arduino Mega pins 18/19).

### Auxiliary serial output (Serial3 / COM3)

Downstream devices that accept serial commands (e.g. sound controllers, lighting controllers) connect to the **COM3** header on the shield. This maps to Serial3 (Arduino Mega pins 14/15), running at 115200 baud by default.

> COM3 is only available as auxiliary serial when neither the Sabertooth drive system nor the RDH dome position sensor is in use (both also claim Serial3). See `drive_config.h` for details.

### Motor PWM outputs

For PWM-based drive systems (PWM, RoboteQ PWM, or RoboteQ PWM+Serial), the motor controller's PWM inputs connect to the servo headers on the shield:

| Function   | Servo channel | Arduino Mega pin |
|------------|:-------------:|:----------------:|
| Drive Right | S1           | 2                |
| Drive Left  | S2           | 3                |
| Dome        | S4           | 5                |

For Sabertooth serial drive, Serial3 (COM3) is used instead of PWM.

### Other connections

| Function              | Pin |
|-----------------------|:---:|
| PPM RC receiver input | 49  |
| RC mode select        | 30  |
| I2C SDA               | 20  |
| I2C SCL               | 21  |
| SD card CS            | 4   |

## Building

This project uses [PlatformIO](https://platformio.org/).

Build the firmware:

```
pio run
```

Run unit tests (no hardware required):

```
pio test -e native
```
