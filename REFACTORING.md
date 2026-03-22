# Amidala Firmware Modularization

## Phase 1 — Header extraction (complete)

All class/struct definitions extracted from `AmidalaFirmware.ino` into focused
`include/` headers.  Each header has a companion `test/test_<name>/` test suite.
Binary footprint never changed: **85.1% RAM / 28.5% Flash** throughout.

| Header | Contents | PR |
|--------|----------|----|
| `include/pin_config.h` | All hardware pin `#define`s | #8 |
| `include/drive_config.h` | Drive/dome system selection, speed/accel constants, `DEFAULT_DOME_*` | #9 |
| `include/button_actions.h` | `ButtonAction`, `GestureAction`, `AuxString` structs | #10 |
| `include/ppm_decoder.h` | `PPMDecoder` class | #11 |
| `include/rdh_serial.h` | `RDHSerial` class | #12 |
| `include/i2c_utils.h` | `recoverI2CBus()`, `sendI2CCmd()`, `sendI2CStr()` | #13 |
| `include/xbee_remote.h` | `XBeePocketRemote`, `DriveController`, `DomeController` (declarations) | #14 |
| `include/amidala_params.h` | `AmidalaParameters` struct + EEPROM load logic | #15 |
| `include/console.h` | `AmidalaConsole` class declaration | #16 |
| `include/jevois_console.h` | `JevoisConsole` class declaration (experimental, `#ifdef EXPERIMENTAL_JEVOIS_STEERING`) | #17 |

---

## Phase 2 — Split `AmidalaFirmware.ino` into `.cpp` files (pending)

### Goal

`AmidalaFirmware.ino` is currently **2275 lines**.  All the method *bodies* for
`AmidalaConsole`, `DriveController`, `DomeController`, and `JevoisConsole` live
there because they need access to the fully-defined `AmidalaController` class.
The plan is to extract `AmidalaController`'s class *declaration* into a header
so the method bodies can move into separate, independently-compiled `.cpp` files.

Target end state:

| File | What it contains | Est. lines |
|------|-----------------|-----------|
| `src/AmidalaFirmware.ino` | Includes, globals, `AmidalaController` ctor + `setup()` + `animate()`, Arduino `setup()`/`loop()` | ~420 |
| `src/amidala_console.cpp` | All `AmidalaConsole::*` method bodies | ~1240 |
| `src/drive_controllers.cpp` | `DriveController::*`, `DomeController::*`, `hcrDelayedInit()` | ~175 |
| `src/jevois_console.cpp` | `JevoisConsole::*` method bodies (under `#ifdef EXPERIMENTAL_JEVOIS_STEERING`) | ~90 |
| `include/amidala_controller.h` | `AmidalaController` class declaration | ~250 |

---

### Current `.ino` map (branch: `main`, commit after PR #17)

| Lines | Content |
|-------|---------|
| 1–9 | Firmware name / version `#define`s |
| 10 | `#include "drive_config.h"` |
| 12 | `#define DEFAULT_BAUD_RATE` |
| 16–33 | Serial baud / gesture timeout `#define`s |
| 37–43 | Debug `#define` switches (`USE_DEBUG`, etc.) |
| 46–72 | Framework includes (ReelTwo, servo, XBee, Wire, HCR, amidala_core, config_reader, pin_config) |
| 82–88 | `servoSettings[]` PROGMEM array |
| 90 | `ServoDispatchDirect servoDispatch(servoSettings)` |
| 96–99 | `#include <EEPROM.h>`, `<XBee.h>`, `ppm_decoder.h`, `i2c_utils.h` |
| 103–104 | Forward-declare `AmidalaController`, `hcrDelayedInit()` |
| 108–109 | `#include "button_actions.h"`, `#include "console.h"` |
| 113–121 | `PANZERO`/`PANRANGE`/`TILTZERO`/`TILTRANGE` `#define`s + `ServoPD panservo`, `tiltservo` globals |
| 125 | `#include "jevois_console.h"` |
| 129–131 | `#include "rdh_serial.h"`, `xbee_remote.h"`, `amidala_params.h"` |
| 135–724 | **`AmidalaController` class body** (public methods inline, private members, friend declarations) |
| 726–728 | Section separator comment |
| 731–775 | `DriveController::notify/onConnect/onDisconnect` bodies |
| 778–890 | `DomeController::notify/process/onConnect/onDisconnect` bodies |
| 892–910 | `static void hcrDelayedInit()` |
| 912–932 | `AmidalaConsole::write(uint8_t)`, `write(const uint8_t*,size_t)`, `printServoPos()` |
| 934–2153 | All remaining `AmidalaConsole::*` method bodies |
| 2155–2249 | `JevoisConsole::init/processCommand/process` bodies (under `#ifdef EXPERIMENTAL_JEVOIS_STEERING`) |
| 2251–2275 | Arduino `setup()` + `loop()` |

---

### Step-by-step plan

Work one PR at a time.  Run `pio test -e native` and `pio run -e megaatmega2560`
after each step before committing.  Footprint must remain 85.1%/28.5%.

---

#### ~~PR 2a — `amidala-controller-header`: create `include/amidala_controller.h`~~ ✅ PR #19

This is the key enabler.  No method bodies move yet — just extract the class
*declaration* so that other `.cpp` files can include it.

**`include/amidala_controller.h`** will contain:
- All framework includes that `AmidalaController` depends on:
  ```cpp
  #include "ReelTwo.h"
  #include "drive/TankDriveSabertooth.h"   // #if DRIVE_SYSTEM == DRIVE_SYSTEM_SABER
  #include "drive/TankDrivePWM.h"          // #if DRIVE_SYSTEM == DRIVE_SYSTEM_PWM
  #include "drive/TankDriveRoboteq.h"      // #if DRIVE_SYSTEM >= ROBOTEQ_PWM
  #include "drive/DomeDrivePWM.h"          // #if DOME_DRIVE == DOME_DRIVE_PWM
  #include "drive/DomeDriveSabertooth.h"   // #if DOME_DRIVE == DOME_DRIVE_SABER
  #include <XBee.h>
  #ifndef VMUSIC_SERIAL
  #include <hcr.h>
  #endif
  #include "amidala_params.h"
  #include "console.h"
  #include "xbee_remote.h"
  #include "rdh_serial.h"
  #include "ppm_decoder.h"
  #include "jevois_console.h"
  ```
- The full `AmidalaController` class declaration, with:
  - All small inline methods kept inline (they have no dependency on
    globals like `servoDispatch` — they only touch `this`)
  - `AmidalaController()` constructor declared (not inline — the initialiser
    list references `params.getRadioChannelCount()` and named constants from
    `drive_config.h` / `pin_config.h`, which will be available wherever the
    header is included)
  - `virtual void setup() override` — declared, NOT inline (references
    `servoDispatch`, `hcrDelayedInit`, `REELTWO_READY()` — globals defined in
    the `.ino`, not in any header)
  - `virtual void animate() override` — declared, NOT inline (same reason)
  - Private data members
  - `friend class AmidalaConsole; friend class DriveController; friend class DomeController;`

**`src/AmidalaFirmware.ino`** changes:
- Replace the entire `class AmidalaController { ... };` block (lines 135–724)
  with `#include "amidala_controller.h"`
- The constructor body, `setup()` body, and `animate()` body stay in the `.ino`
  as out-of-class definitions (they reference globals `servoDispatch`, `panservo`,
  `tiltservo`, `hcrDelayedInit` that are defined earlier in the same file)
- The `.ino` becomes: includes → globals → `#include "amidala_controller.h"` →
  constructor definition → `setup()` definition → `animate()` definition →
  DriveController/DomeController bodies → hcrDelayedInit → AmidalaConsole bodies
  → JevoisConsole bodies → Arduino `setup()`/`loop()`

**No test suite needed** for this PR (no behavioural change; `AmidalaController`
cannot be unit-tested in isolation because it depends on the full Arduino
framework).  Just verify `pio test -e native` (all existing tests still pass)
and `pio run -e megaatmega2560` succeeds with identical footprint.

> **Constructor gotcha**: The `#if DRIVE_SYSTEM == DRIVE_SYSTEM_PWM` branch in
> the initialiser list has a typo: `fTankDrive(servoDispatch, 1, 0, 4 fDriveStick)`
> — missing comma before `fDriveStick`.  This exists in the original `.ino` and
> is harmless because `DRIVE_SYSTEM_PWM` is not the active configuration.
> Do NOT fix it; just preserve it verbatim to avoid a behaviour/scope creep.

---

#### ~~PR 2b — `drive-controllers-impl`: `src/drive_controllers.cpp`~~ ✅ PR #20

Move from `.ino` to new file:
- `DriveController::notify()` / `::onConnect()` / `::onDisconnect()`
- `DomeController::notify()` / `::process()` / `::onConnect()` / `::onDisconnect()`

**`src/drive_controllers.cpp`** preamble:
```cpp
#include "amidala_controller.h"
```

`hcrDelayedInit()` was initially considered for this file but belongs in
`AmidalaFirmware.ino` — it initialises the HCR audio system and has no
connection to the drive/dome controllers.  It stays `static` in the `.ino`.

> **Lessons from PR 2b**: including `amidala_controller.h` in a second TU
> exposed two latent bugs:
> 1. `ServoDispatchPrivate.h` defines ISR handlers — included via
>    `ServoDispatchDirect.h`.  Fix: move `ServoDispatchDirect.h` and
>    `ServoEasing.h` back to `AmidalaFirmware.ino` (the only TU that needs
>    them).  `amidala_controller.h` no longer includes those files.
> 2. 13 free functions in `amidala_core.h` had definitions without `inline`,
>    causing ODR violations when compiled into two TUs.  Fixed by adding
>    `inline` to all of them.

---

#### ~~PR 2c — `amidala-audio-impl`: `src/amidala_audio.cpp`~~ ✅ PR #21

Introduce a dedicated `AmidalaAudio` class (declared in `include/amidala_audio.h`)
that owns the complete audio API.  All audio concerns — HCR hardware init,
VMusic polling, volume, sound playback, emotes, muse, and ack — move out of
`AmidalaConsole` and into `AmidalaAudio`.

`AmidalaController` gains an `fAudio` member; `process(ButtonAction&)`,
`processCommand()`, and `animate()` all delegate to `fController->fAudio.*`.
`hcrDelayedInit` becomes file-static inside `amidala_audio.cpp`.

```
include/amidala_audio.h        ← new: AmidalaAudio class declaration
src/amidala_audio.cpp          ← new: AmidalaAudio:: method bodies
  AmidalaAudio::init()         ← replaces inline HCR begin/schedule in setup()
  AmidalaAudio::process()      ← replaces inline HCR/VMusic block in animate()
  AmidalaAudio::randomToggle()
  AmidalaAudio::setVolumeNoResponse()
  AmidalaAudio::playSound()
  AmidalaAudio::playEmote()    ← extracted from process(ButtonAction&)
  AmidalaAudio::toggleMuse()   ← extracted from process(ButtonAction&)
  AmidalaAudio::playAck()      ← extracted from process(ButtonAction&)
```

---

#### ~~PR 2d — `amidala-buttons-impl`: `src/amidala_buttons.cpp`~~ ✅ PR #22

Move the button/gesture dispatch pipeline out of the `.ino`.  These four
methods are the "input → action" layer: they receive a button press or gesture
string and translate it into hardware commands.  They are cleanly separable
from configuration and command-parsing, which stays in `amidala_console.cpp`.

```
AmidalaConsole::process(ButtonAction&)   ← dispatches one action (sound/I2C/servo/aux/emote)
AmidalaConsole::processGesture()         ← matches gesture string → configured action
AmidalaConsole::processButton()          ← maps button number → configured ButtonAction
AmidalaConsole::processLongButton()      ← maps long-press number → configured ButtonAction
```

Note: the button-related `case` branches in `processConfig()` (which configure
what button N does) and the `$nn,mm` sound command in `processCommand()` are
configuration/parsing logic — they stay in `amidala_console.cpp`.

**`src/amidala_buttons.cpp`** preamble:
```cpp
#include "amidala_controller.h"
```
All needed types are available through that single include.

---

#### PR 2e — `amidala-console-impl`: `src/amidala_console.cpp`

Move from `.ino` to new file — all remaining `AmidalaConsole` methods (~1070
lines after audio and button methods have been extracted):

```
AmidalaConsole::write(uint8_t)
AmidalaConsole::write(const uint8_t*, size_t)
AmidalaConsole::printServoPos()
AmidalaConsole::init()
AmidalaConsole::setServo()
AmidalaConsole::setDigitalOut()
AmidalaConsole::outputString()
AmidalaConsole::showXBEE()
AmidalaConsole::printVersion()
AmidalaConsole::printHelp()
AmidalaConsole::showLoadEEPROM()
AmidalaConsole::showCurrentConfiguration()
AmidalaConsole::writeCurrentConfiguration()
AmidalaConsole::monitorOutput()
AmidalaConsole::monitorToggle()
static bool isdigit(const char*, int)     ← file-scope helper, keep static
AmidalaConsole::processConfig()           ← configures button/gesture/audio params
AmidalaConsole::processCommand()          ← parses runtime text commands
AmidalaConsole::process(char, bool)
AmidalaConsole::process()
```

**`src/amidala_console.cpp`** preamble:
```cpp
#include "amidala_controller.h"
extern ServoDispatchDirect<12> servoDispatch;  // defined in AmidalaFirmware.ino
```

Use the concrete size `12` rather than `SizeOfArray(servoSettings)`.
`CONSOLE_SERIAL` is a `#define Serial` from `pin_config.h` — already available.
The `static bool isdigit(const char*, int)` helper stays `static` to remain
translation-unit-local and avoid shadowing `std::isdigit` in other TUs.

---

#### PR 2f — `jevois-console-impl`: `src/jevois_console.cpp`

Move `JevoisConsole::init/processCommand/process` (lines 2155–2249) to a new
file.  Wrap in `#ifdef EXPERIMENTAL_JEVOIS_STEERING / #endif` as they are now.

**`src/jevois_console.cpp`** preamble:
```cpp
#include "amidala_controller.h"
#ifdef EXPERIMENTAL_JEVOIS_STEERING
extern ServoPD tiltservo;   // defined in AmidalaFirmware.ino
#endif
```
`tiltservo` is a global defined in the `.ino`; it must be `extern`-declared here.

---

### After Phase 2 — resulting file structure

| File | Contents |
|------|---------|
| `src/AmidalaFirmware.ino` | Includes, globals, `AmidalaController` ctor/setup/animate, `hcrDelayedInit`, Arduino `setup()`/`loop()` |
| `src/drive_controllers.cpp` | `DriveController::*`, `DomeController::*` ✅ |
| `include/amidala_audio.h` | `AmidalaAudio` class declaration ✅ |
| `src/amidala_audio.cpp` | `AmidalaAudio::*` — all audio hardware concerns ✅ |
| `src/amidala_buttons.cpp` | `AmidalaConsole::process(ButtonAction&)`, `processGesture`, `processButton`, `processLongButton` ✅ |
| `src/amidala_console.cpp` | All remaining `AmidalaConsole::*` methods (I/O, config, command parsing) |
| `src/jevois_console.cpp` | `JevoisConsole::*` (under `#ifdef EXPERIMENTAL_JEVOIS_STEERING`) |
| `include/amidala_controller.h` | `AmidalaController` class declaration ✅ |

### Remaining `.ino` structure after all PRs

```
AmidalaFirmware.ino  (~400 lines)
─────────────────────────────────────────────────────
  1–  9   Version / name #defines
 10        #include "drive_config.h"
 12        #define DEFAULT_BAUD_RATE
 16– 43    Baud / gesture / debug #defines
 46– 72    Framework includes
 82– 90    servoSettings[] + servoDispatch global
 96– 99    EEPROM/XBee/ppm_decoder/i2c_utils includes
103–104    Forward decls (AmidalaController, hcrDelayedInit)
108–131    button_actions.h, console.h, jevois_console.h,
            rdh_serial.h, xbee_remote.h, amidala_params.h
           ─ REPLACED BY ─
           #include "amidala_controller.h"
113–121    panservo, tiltservo globals + PANZERO etc.
           AmidalaController constructor (out-of-class)
           AmidalaController::setup() (out-of-class)
           AmidalaController::animate() (out-of-class)
           static AmidalaController controller;
           Arduino setup() + loop()
```

---

### Phase 2e (optional, later) — make the `.ino` a true entry point

If the goal is a minimal `.ino` (≈60 lines), extract `servoDispatch`,
`panservo`, `tiltservo` and the `AmidalaController::setup()`/`animate()` bodies
into `src/amidala_controller.cpp` and `src/amidala_globals.cpp`.  This requires
`extern` declarations for those globals in a new `include/amidala_globals.h`.
This phase is lower priority — it adds complexity for relatively little gain.

---

### Lessons from Phase 1 (patterns to carry forward)

- **One PR at a time.**  Run both `pio test -e native` and
  `pio run -e megaatmega2560` before committing.  Footprint must not change.
- **Update `REFACTORING.md`** and cross off items in the same commit that
  makes the change — do not forget this.
- **Push to remote `thePunderWoman`**, not `origin`.
- **`extern` for cross-file globals**: when a `.cpp` file needs a global
  defined in another TU, use `extern` declarations.  Do not use `static` on
  globals that need to cross TU boundaries.
- **`static` file-scope helpers stay `static`** in the new `.cpp` to preserve
  internal linkage.
- **The `amidala_console.cpp` `isdigit` helper** shadows the standard library
  `isdigit(int)`.  The `static` keyword keeps it internal and avoids the
  collision in other TUs.
- **No test suite for `AmidalaController` itself** — it requires the full
  Arduino/Reeltwo framework.  Native tests only cover the extracted headers.
