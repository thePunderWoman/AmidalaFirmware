# Amidala Firmware Mark 2

A firmware control system for an astromech or similar droid that uses dual XBee remote joystick controllers.

This is a fork of [Skelmir's (rimim) original Amidala Firmware](https://github.com/reeltwo/AmidalaFirmware).

> **Getting Started guide coming soon.** For now, see [example_config.txt](example_config.txt) for a complete, annotated example of every available configuration option.

## Hardware

- **Microcontroller:** Arduino Mega 2560
- **Shield:** DFRobot Mega Sensor Shield V2.4
- **Configuration storage:** Micro SD card (`config.txt` loaded at startup)

## Features

### Remote Control

Dual [XBee Pocket Remotes](https://reeltwo.github.io/Reeltwo) provide wireless control:

- **Drive controller (left XBee):** Drive throttle and button inputs
- **Dome controller (right XBee):** Dome rotation, volume control, and gesture input
- Graceful failsafe handling on signal loss

An optional analog **RC receiver (PPM)** is also supported for drive control.

### Drive Systems

Drive system is selected at compile time in `drive_config.h`. Supported options:

- PWM (default)
- Sabertooth serial
- RoboteQ PWM, serial, or PWM+Serial hybrid

### Audio

Two audio systems are supported, selectable at runtime:

- **Human Cyborg Relations (HCR) board** (default) — three independent sound channels (Voice, Ambient, Background) with emotion/emote support and Muse mode for ambient random playback
- **VMusic2** — USB-based audio with directory-organised sound banks on a USB drive

### Button Configuration

Up to nine buttons are configurable. Each button independently supports three layers:

| Layer | Key | Trigger |
|-------|-----|---------|
| Short press | `b=` | Button release (tap) |
| Long press | `lb=` | Held for **3 seconds**; short-press release is suppressed |
| Alt press | `ab=` | Button release while the alt modifier button is held |

All three layers use the same argument format: `b=ButtonNum,Action,Arg1,Arg2,SerialStr`

Available actions: sound bank playback, servo positioning, digital output control, serial string output, auxiliary I2C command or string, HCR emote trigger, HCR Muse toggle, dome command.

#### Alt Button

One button can be designated as a hold-to-activate **modifier** (`altbtn=N`, default disabled). While it is held:

- All other button presses dispatch their **alt-layer** action (`ab=`) instead of their normal action
- Long-press actions are suppressed
- The alt button itself does not fire its own `b=` or `lb=` action

On the dome controller, `altdomestick=1` additionally puts the dome stick into **absolute-stick mode** while the alt button is held — the stick angle maps directly to a dome heading. Normal dome control resumes when the modifier is released.

Button numbering: drive-stick triangle=1, circle=2, cross=3, square=4, L3=5; dome-stick triangle=6, circle=7, cross=8, square=9.

### Gestures

Up to 10 custom gestures can be mapped to any button action. Gestures are composed by moving the dome stick through a sequence of directions. Built-in gestures include toggles for random sound playback, slow/fast speed mode, and acknowledgement sounds.

### Dome Drive

Three dome drive systems are supported:

#### Encoder Dome Drive

Uses a **Pololu encoder motor**, **RoboClaw motor controller**, and a single **hall-effect sensor** for homing. All position tracking is encoder-derived, giving precise closed-loop control.

See more details on how this works in the detailed guide, coming soon!

**State machine:**

| State | Description |
|-------|-------------|
| Manual | No home reference yet; joystick works, auto modes disabled |
| Homing | Slow sweep looking for the hall sensor |
| Homed | Hall found; all autonomous modes available |
| Calibrating | Counting 10 dome revolutions to compute the motor-to-dome encoder ratio |
| Obstructed | Stall detected; motor stopped until the joystick is moved manually |
| Absolute-Stick | Joystick angle maps directly to a dome heading |
| Random | Random wander mode between configurable left/right bounds |

**Calibration** (first-time setup):

1. Send `dome=home` — the dome sweeps until the hall sensor fires, establishing position zero
2. Send `dome=calibrate` — the dome makes 10 full revolutions; the computed ticks-per-revolution ratio is saved to EEPROM automatically
3. Calibration only needs to be run once per mechanical assembly. It is restored from EEPROM on every boot.

**Dome console commands** (send via serial console):

| Command | Effect |
|---------|--------|
| `dome=home` | Re-run homing sweep |
| `dome=calibrate` | Run 10-revolution calibration |
| `dome=front` | Go to configured front position (0°) |
| `dome=stop` | Emergency stop |
| `dome=rand` | Toggle random wander mode |
| `dome=abstick` | Toggle absolute-stick mode |
| `dome=N` | Go to absolute angle N (degrees) |
| `dome=+N` / `dome=-N` | Relative move ±N degrees |
| `dome=status` | Print current state, angle, and calibration |

**Dome button commands** (assign via `b=`, `lb=`, or `ab=` using action `9`):

Format: `b=ButtonNum,9,subcmd[,angle]`

| Subcmd | Example | Effect |
|:------:|---------|--------|
| `0` | `b=1,9,0` | Toggle random wander mode on/off |
| `1` | `b=2,9,1` | Emergency stop |
| `2` | `b=3,9,2` | Go to front (0°) |
| `3` | `lb=4,9,3` | Re-run homing sweep |
| `4` | `lb=5,9,4` | Run 10-revolution encoder calibration |
| `5` | `b=6,9,5,90` | Go to absolute angle (0–359°) |
| `6` | `b=7,9,6,45` | Rotate +N degrees from current position |
| `7` | `b=8,9,7,45` | Rotate −N degrees from current position |
| `8` | `b=9,9,8` | Toggle absolute-stick mode |

**Key config keys** (full reference in [example_config.txt](example_config.txt)):

| Key | Description |
|-----|-------------|
| `domercaddr=` | RoboClaw packet-serial address (default 128) |
| `domercchan=` | Motor channel: 1=M1, 2=M2 |
| `domercqpps=` | Encoder pulses per second at full speed (tune via Motion Studio) |
| `domefront=` | Degrees from hall sensor to dome "front" (0–359) |
| `domestall=` | Milliseconds of zero movement before obstruction is declared |
| `domedecelzone=` | Degrees before target at which deceleration begins (5–90, default 30) |
| `domefudge=` | Dead zone around target in degrees; prevents hunting |
| `domespeedhome=` | Motor speed during homing (1–100) |
| `domespeedseek=` | Motor speed during random seek (1–100) |
| `domespeedmin=` | Minimum speed during deceleration approach (0–30) |

#### RDH Serial (position-sensor based)

Autonomous dome rotation via an optional RDH serial position sensor. Supports home positioning, random seek, and target positioning modes.

#### Basic PWM Control

Standard dome movement via PWM is supported via a basic speed controller, like the Syren 10.

### RC Control

PPM RC receiver input for drive and dome control, with configurable failsafe timeout.

### Primary Serial and Auxiliary I2C

- **Primary serial** (Serial3 / COM3): up to 40 configurable string templates sent to downstream devices on button or gesture actions
- **Auxiliary I2C**: send command bytes or strings to I2C devices by configured address as a secondary output path

## Configuration

All runtime settings are loaded from `config.txt` on the micro SD card at startup. No recompilation is required for most changes.

**[example_config.txt](example_config.txt) is the complete reference** — every supported configuration key is listed with its format, valid range, and an example. It is also a working config used on a real build.

Legacy calibration data (XBee addresses, servo calibration, joystick calibration) is stored in EEPROM and is not part of `config.txt`.

## Wiring

### Dependencies

Install [PlatformIO](https://platformio.org/) — it handles all library dependencies automatically on first build. The firmware depends on:

- [Reeltwo](https://github.com/reeltwo/Reeltwo)
- [ReeltwoAudio](https://github.com/reeltwo/ReeltwoAudio)

### XBee receiver

Connect the XBee module to the **COM1** header on the DFRobot Mega Sensor Shield. This maps to Serial1 (Arduino Mega pins 18/19).

### Primary serial output (Serial3 / COM3)

Downstream devices that accept serial commands (e.g. sound controllers, lighting controllers) connect to the **COM3** header on the shield. This maps to Serial3 (Arduino Mega pins 14/15), running at 115200 baud by default. This is the primary output path; I2C is auxiliary/backup.

> COM3 is only available as primary serial when neither the Sabertooth drive system nor the RDH dome position sensor is in use (both also claim Serial3). See `drive_config.h` for details.

### Encoder Dome Drive

| Connection | Details |
|------------|---------|
| RoboClaw serial | Serial2 (COM2 header, pins 16/17), 38400 baud |
| Hall-effect sensor | Configured via `DOME_HALL_PIN` in `pin_config.h` |
| Motor encoder | Connected to RoboClaw encoder input |

### Motor PWM outputs

For PWM-based drive systems (PWM, RoboteQ PWM, or RoboteQ PWM+Serial), the motor controller's PWM inputs connect to the servo headers on the shield:

| Function    | Servo channel | Arduino Mega pin |
|-------------|:-------------:|:----------------:|
| Drive Right | S1            | 2                |
| Drive Left  | S2            | 3                |
| Dome        | S4            | 5                |

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
