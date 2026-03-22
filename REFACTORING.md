# Amidala Firmware Modularization

## Phase 1 — Header extraction ✅ (PRs #8–#17)
## Phase 2 — Split `.ino` into `.cpp` files ✅ (PRs #19–#26)

---

## Phase 3 — True entry point

### Goal

`AmidalaFirmware.ino` is currently **466 lines**.  The remaining work has two
parts:

1. **Clean up dead `#define`s** — several defines at the top of the `.ino` are
   redundant: the same constant with the same default value already exists in a
   library header.  Others (`FIRMWARE_NAME`, `VERSION_NUM`, etc.) belong in a
   dedicated header so `.cpp` files can reach them without pulling in the whole
   `.ino`.
2. **Extract `AmidalaController` method bodies** — the constructor, `setup()`,
   `animate()`, `emergencyStop()`, and `domeEmergencyStop()` are still defined
   in the `.ino` because they reference the `servoDispatch`, `panservo`, and
   `tiltservo` globals, which are also defined there.  Moving the method bodies
   to `src/amidala_controller.cpp` (with `extern` declarations for those
   globals) leaves the `.ino` as a clean, ~80-line entry point.

Target end state of `AmidalaFirmware.ino`:

```
#define USE_DEBUG  // (and other user-editable debug switches)

#include "ServoDispatchDirect.h"   // must stay here — defines hardware ISRs
#include "ServoEasing.h"
#include "amidala_controller.h"

const ServoSettings servoSettings[] PROGMEM = { ... };
ServoDispatchDirect<12> servoDispatch(servoSettings);

ServoPD panservo(...);
ServoPD tiltservo(...);

AmidalaController amidala;

void setup() { serial inits; SetupEvent::ready(); }
void loop()  { AnimatedEvent::process(); }
```

---

### Redundant `#define` inventory

| Define | Currently in `.ino` | Already defined elsewhere | Action |
|--------|--------------------|-----------------------------|--------|
| `DEFAULT_BAUD_RATE 115200` | ✓ | `lib/Reeltwo/src/ReelTwo.h` (same value) | **Remove** |
| `MAX_GESTURE_LENGTH 8` | ✓ | `include/amidala_core.h` (same value) | **Remove** |
| `GESTURE_TIMEOUT_MS 2000` | ✓ | `include/xbee_remote.h` (same value) | **Remove** |
| `LONG_PRESS_TIME 3000` | ✓ | `include/xbee_remote.h` (same value) | **Remove** |
| `MARCDUINO_BAUD_RATE 9600` | ✓ | Nowhere — appears unused | **Remove** |
| `FIRMWARE_NAME` | ✓ | Nowhere | **Move to `amidala_version.h`** |
| `VERSION_NUM` | ✓ | Nowhere | **Move to `amidala_version.h`** |
| `BUILD_NUM` | ✓ | Nowhere | **Move to `amidala_version.h`** |
| `BUILD_DATE` | ✓ | Nowhere | **Move to `amidala_version.h`** |
| `RDH_BAUD_RATE 9600` | ✓ | Nowhere | **Move to `amidala_version.h`** |
| `USE_DEBUG` (and friends) | ✓ | Nowhere | **Keep in `.ino`** — intentionally user-editable |

---

### Step-by-step plan

Work one PR at a time.  Run `pio test -e native` after each step before
committing.

---

#### PR 3a — `amidala-version-header`: `include/amidala_version.h`

Create a small header that owns the firmware's identity and communication
constants:

```cpp
// include/amidala_version.h
#pragma once

#define FIRMWARE_NAME        F("Amidala RC")
#define VERSION_NUM          F("1.3")
#define BUILD_NUM            F("1")
#define BUILD_DATE           F(__DATE__)

#define RDH_BAUD_RATE        9600
```

Include it from `amidala_controller.h` (so every `.cpp` that includes the
controller header automatically gets it — `amidala_console.cpp` needs
`FIRMWARE_NAME`/`VERSION_NUM`, and `amidala_controller.cpp` will need
`RDH_BAUD_RATE`).

Remove from `AmidalaFirmware.ino`:
- All five version/baud defines that move to the new header
- The four redundant defines (`DEFAULT_BAUD_RATE`, `MAX_GESTURE_LENGTH`,
  `GESTURE_TIMEOUT_MS`, `LONG_PRESS_TIME`) and the unused `MARCDUINO_BAUD_RATE`

---

#### PR 3b — `amidala-controller-impl`: `src/amidala_controller.cpp`

Move the five `AmidalaController` method bodies out of the `.ino`:

```
AmidalaController::AmidalaController()   ← ctor + long initialiser list
AmidalaController::emergencyStop()
AmidalaController::domeEmergencyStop()
AmidalaController::setup()               ← all hardware init
AmidalaController::animate()             ← main loop: XBee, PPM, RC, console
```

**`src/amidala_controller.cpp`** preamble:
```cpp
#include "amidala_controller.h"

// Hardware globals defined in AmidalaFirmware.ino:
extern ServoDispatchDirect<12> servoDispatch;
extern ServoPD panservo;
extern ServoPD tiltservo;
```

`AmidalaFirmware.ino` after this PR (~80 lines):
- `#define USE_DEBUG` and friends (user debug switches)
- `#include "ServoDispatchDirect.h"` / `"ServoEasing.h"` / `"amidala_controller.h"`
- `servoSettings[]` PROGMEM array + `servoDispatch` global
- `PANZERO`/`PANRANGE`/`TILTZERO`/`TILTRANGE` defines + `panservo`/`tiltservo` globals
- `AmidalaController amidala;`
- `void setup()` (serial port init + `SetupEvent::ready()`)
- `void loop()`

> **Note:** `ServoDispatchDirect.h` and `ServoEasing.h` must stay in the
> `.ino` — `ServoDispatchPrivate.h` defines hardware ISR handlers that must
> live in exactly one translation unit, which remains the `.ino`.

---

### After Phase 3 — resulting file structure

| File | Contents |
|------|---------|
| `src/AmidalaFirmware.ino` | Debug switches, hardware globals, entry points (~80 lines) |
| `include/amidala_version.h` | `FIRMWARE_NAME`, `VERSION_NUM`, `BUILD_*`, `RDH_BAUD_RATE` ✅ |
| `src/amidala_controller.cpp` | `AmidalaController` ctor, `setup()`, `animate()`, `emergencyStop()`, `domeEmergencyStop()` |
| `src/drive_controllers.cpp` | `DriveController::*`, `DomeController::*` ✅ |
| `include/amidala_audio.h` + `src/amidala_audio.cpp` | `AmidalaAudio::*` ✅ |
| `src/amidala_buttons.cpp` | Button/gesture dispatch ✅ |
| `src/amidala_servo.cpp` | Servo console methods + `AmidalaConfig::applyServoConfig` ✅ |
| `include/amidala_config.h` + `src/amidala_config.cpp` | `AmidalaConfig::*` + `readConfig()` ✅ |
| `src/amidala_console.cpp` | `AmidalaConsole::*` ✅ |
| `src/jevois_console.cpp` | `JevoisConsole::*` ✅ |

---

### Optional Phase 3c — extract hardware globals

If a fully minimal `.ino` (≈30 lines) is the goal, the three hardware globals
can move to `src/amidala_globals.cpp`:

```cpp
// src/amidala_globals.cpp
#include "ServoDispatchDirect.h"   // ISRs now defined here
#include "ServoEasing.h"
#include "pin_config.h"
#include "amidala_core.h"

const ServoSettings servoSettings[] PROGMEM = { ... };
ServoDispatchDirect<12> servoDispatch(servoSettings);

ServoPD panservo(400, 200, PANZERO, PANRANGE);
ServoPD tiltservo(300, 100, TILTZERO, TILTRANGE);
```

This is feasible (moving the ISR-containing include to `amidala_globals.cpp`
satisfies the one-TU constraint), but it adds a new file for relatively little
gain.  Lower priority.
