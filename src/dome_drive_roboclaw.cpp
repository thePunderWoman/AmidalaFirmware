// dome_drive_roboclaw.cpp
// See include/dome_drive_roboclaw.h for architecture notes.

#include "drive_config.h"

#if DOME_DRIVE == DOME_DRIVE_ROBOCLAW

#include "dome_drive_roboclaw.h"
#include "dome_position_math.h"
#include "ReelTwo.h"   // DEBUG_PRINT / DEBUG_PRINTLN macros

// Uncomment to enable high-frequency per-cycle logging (sendMotorCommand,
// readEncoder).  These fire every 25 ms so leave this off in normal use.
// #define DOME_DEBUG

// ---------------------------------------------------------------------------
// Static singleton for ISR glue
// ---------------------------------------------------------------------------

DomeDriveRoboClaw* DomeDriveRoboClaw::sInstance = nullptr;

/*static*/
void DomeDriveRoboClaw::hallISR() {
    if (sInstance != nullptr)
        sInstance->onHallTrigger();
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

#ifndef UNIT_TEST

DomeDriveRoboClaw::DomeDriveRoboClaw(HardwareSerial& serial,
                                     uint8_t address,
                                     uint8_t channel,
                                     uint8_t hallPin,
                                     JoystickController& stick)
    : DomeDrive(stick),
      fRoboClaw(&serial, 10000),
      fAddress(address),
      fChannel(channel),
      fHallPin(hallPin),
      fDomePos(*this)           // safe: DomePosition ctor only stores the ref
{
    setMaxSpeed(1.0f);
    setScaling(true);
    setThrottleAccelerationScale(DEFAULT_DOME_ACCELERATION_SCALE);
    setThrottleDecelerationScale(DEFAULT_DOME_DECELERATION_SCALE);
}

#else // UNIT_TEST

DomeDriveRoboClaw::DomeDriveRoboClaw(uint8_t address,
                                     uint8_t channel,
                                     uint8_t hallPin,
                                     JoystickController& stick)
    : DomeDrive(stick),
      fAddress(address),
      fChannel(channel),
      fHallPin(hallPin),
      fDomePos(*this)
{
    setMaxSpeed(1.0f);
    setScaling(true);
    setThrottleAccelerationScale(DEFAULT_DOME_ACCELERATION_SCALE);
    setThrottleDecelerationScale(DEFAULT_DOME_DECELERATION_SCALE);
}

#endif // UNIT_TEST

// ---------------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------------

void DomeDriveRoboClaw::setup() {
    // Register this instance as the ISR target (only one dome drive per build).
    sInstance = this;

#ifndef UNIT_TEST
    // Configure hall sensor pin: active-LOW output, use internal pull-up.
    pinMode(fHallPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(fHallPin), hallISR, FALLING);

    // Bring up the RoboClaw serial link and reset its encoder counters.
    ROBOCLAW_SERIAL.begin(ROBOCLAW_BAUD_RATE);
    fRoboClaw.begin(ROBOCLAW_BAUD_RATE);
    fRoboClaw.ResetEncoders(fAddress);
#endif

    // Restore calibration from EEPROM (works with real or mock EEPROM).
    loadCalibrationFromEEPROM();

    // Wire our encoder-derived position into DomeDrive's auto-movement logic.
    setDomePosition(&fDomePos);

    // Apply sensible defaults; caller may override via applyDomePositionParams().
    fDomePos.setDomeHomeMinDelay(DEFAULT_DOME_HOME_MIN_DELAY);
    fDomePos.setDomeHomeMaxDelay(DEFAULT_DOME_HOME_MAX_DELAY);
    fDomePos.setDomeAutoMinDelay(DEFAULT_DOME_SEEK_MIN_DELAY);
    fDomePos.setDomeAutoMaxDelay(DEFAULT_DOME_SEEK_MAX_DELAY);
    fDomePos.setDomeAutoLeftDegrees(DEFAULT_DOME_SEEK_LEFT);
    fDomePos.setDomeAutoRightDegrees(DEFAULT_DOME_SEEK_RIGHT);
    fDomePos.setDomeFudgeFactor(DEFAULT_DOME_FUDGE);
    fDomePos.setDomeHomeSpeed(DEFAULT_DOME_SPEED_HOME);
    fDomePos.setDomeTargetSpeed(DEFAULT_DOME_SPEED_TARGET);
    fDomePos.setDomeAutoSpeed(DEFAULT_DOME_SPEED_SEEK);
    fDomePos.setDomeMinSpeed(DEFAULT_DOME_SPEED_MIN);
    fDomePos.setTimeout(DEFAULT_DOME_TIMEOUT);
    // Home angle is 0°: "facing front" after homing completes.
    fDomePos.setDomeHomePosition(0);

    // Kick off homing so the dome establishes its position reference on boot.
    startHoming();
}

// ---------------------------------------------------------------------------
// animate()  — called every ~25 ms from the main AnimatedEvent loop
// ---------------------------------------------------------------------------

void DomeDriveRoboClaw::animate() {
    // 1. Refresh encoder-derived position (no-op until calibrated).
    updatePosition();

    // 2. Consume any pending hall-sensor trigger.
    bool hallFired = processHallTrigger();

    // 3. Run per-state logic.
    switch (fState) {
        case kStateHoming:
            handleHoming(hallFired);
            return;

        case kStateCalibrating:
            handleCalibrating(hallFired);
            return;

        case kStateObstructed: {
            // Clear obstruction when the user moves the joystick past the deadband.
            auto stickx = useLeftStick()
                              ? fDomeStick.state.analog.stick.lx
                              : fDomeStick.state.analog.stick.rx;
            float m = (float)(stickx + 128) / 127.5f - 1.0f;
            if (fDomeStick.isConnected() && abs(m) >= 0.2f) {
                DEBUG_PRINTLN("DOME: Obstruction cleared by manual input");
                fState = kStateHomed;
                // Fall through to normal processing below.
            } else {
                // Keep motor stopped; do not call base animate().
                stop();
                return;
            }
            break;
        }

        case kStateAbsoluteStick:
            if (fDomeStick.isConnected()) {
                // Compute dome heading from the 2-D stick position.
                int lx = useLeftStick() ? fDomeStick.state.analog.stick.lx
                                        : fDomeStick.state.analog.stick.rx;
                int ly = useLeftStick() ? fDomeStick.state.analog.stick.ly
                                        : fDomeStick.state.analog.stick.ry;
                int target = dome_stick_to_angle(lx, ly);
                if (target >= 0)
                    goToAngle(target);
                // Zero the rotation (X) axis before calling DomeDrive::animate()
                // so its domeStick() handler treats the stick as neutral and
                // defers to the DomePosition target-following path rather than
                // interpreting the stick as a direct motor throttle.
                // The value is refreshed from the next XBee packet so this only
                // affects this one animate() cycle.
                if (useLeftStick())
                    fDomeStick.state.analog.stick.lx = 0;
                else
                    fDomeStick.state.analog.stick.rx = 0;
            }
            break;

        default:
            break;
    }

    // 4. Obstruction detection (only while the motor is being commanded).
    if (fState == kStateHomed || fState == kStateAbsoluteStick) {
        checkObstruction();
    }

    // 5. Let the DomeDrive base handle joystick + DomePosition auto modes.
    DomeDrive::animate();
}

// ---------------------------------------------------------------------------
// stop()
// ---------------------------------------------------------------------------

void DomeDriveRoboClaw::stop() {
    sendMotorCommand(0.0f);
    DomeDrive::stop();
}

// ---------------------------------------------------------------------------
// motor()  — called by DomeDrive::domeStick() with -1.0 to +1.0
// ---------------------------------------------------------------------------

/*virtual*/
void DomeDriveRoboClaw::motor(float m) {
    fLastCommandedSpeed = m;
    sendMotorCommand(m);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void DomeDriveRoboClaw::startHoming() {
    DEBUG_PRINTLN("DOME: Starting homing sweep");
    stop();
    fState        = kStateHoming;
    fHomingStartMs = millis();
}

void DomeDriveRoboClaw::startCalibration() {
    if (fState == kStateObstructed) {
        DEBUG_PRINTLN("DOME: Cannot calibrate — obstruction must be cleared first");
        return;
    }
    DEBUG_PRINTLN("DOME: Starting calibration (10 revolutions)");
    stop();
    fCalibrationTriggerCount = 0;
    fCalibrationStartTick    = 0;
    fState = kStateCalibrating;
}

void DomeDriveRoboClaw::goToAngle(int degrees) {
    if (!isHomed()) {
        DEBUG_PRINTLN("DOME: Cannot seek — not homed");
        return;
    }
    if (!isCalibrated()) {
        DEBUG_PRINTLN("DOME: Cannot seek — run dome=calibrate first");
        return;
    }
    degrees = normalizeDegrees(degrees);
    fDomePos.setDomeTargetPosition(degrees);
    fDomePos.setDomeMode(DomePosition::kTarget);
}

void DomeDriveRoboClaw::goToRelative(int delta) {
    goToAngle(fCurrentDegrees + delta);
}

void DomeDriveRoboClaw::enableRandomMode() {
    if (!isHomed()) {
        DEBUG_PRINTLN("DOME: Cannot enable random — not homed");
        return;
    }
    fDomePos.setDomeDefaultMode(DomePosition::kRandom);
}

void DomeDriveRoboClaw::disableAutoMode() {
    fDomePos.setDomeDefaultMode(DomePosition::kOff);
}

void DomeDriveRoboClaw::enableAbsoluteStickMode() {
    if (!isHomed() || !isCalibrated()) return;
    fState = kStateAbsoluteStick;
    // Start in target mode so DomePosition immediately begins tracking.
    fDomePos.setDomeTargetPosition(fCurrentDegrees);
    fDomePos.setDomeMode(DomePosition::kTarget);
}

void DomeDriveRoboClaw::disableAbsoluteStickMode() {
    if (fState == kStateAbsoluteStick) {
        fState = kStateHomed;
        disableAutoMode();
    }
}

void DomeDriveRoboClaw::setMaxSpeedPct(float pct) {
    pct = max(0.0f, min(pct, 1.0f));
    setMaxSpeed(pct);
}

void DomeDriveRoboClaw::applyDomePositionParams(
        uint8_t homeMinDelay, uint8_t homeMaxDelay,
        uint8_t seekMinDelay, uint8_t seekMaxDelay,
        uint8_t seekLeft,     uint8_t seekRight,
        uint8_t fudge,
        uint8_t speedHome,    uint8_t speedTarget,
        uint8_t speedSeek,    uint8_t speedMin) {
    fDomePos.setDomeHomeMinDelay(homeMinDelay);
    fDomePos.setDomeHomeMaxDelay(homeMaxDelay);
    fDomePos.setDomeAutoMinDelay(seekMinDelay);
    fDomePos.setDomeAutoMaxDelay(seekMaxDelay);
    fDomePos.setDomeAutoLeftDegrees(seekLeft);
    fDomePos.setDomeAutoRightDegrees(seekRight);
    fDomePos.setDomeFudgeFactor(fudge);
    fDomePos.setDomeHomeSpeed(speedHome);
    fDomePos.setDomeTargetSpeed(speedTarget);
    fDomePos.setDomeAutoSpeed(speedSeek);
    fDomePos.setDomeMinSpeed(speedMin);
}

// ---------------------------------------------------------------------------
// EEPROM persistence
// ---------------------------------------------------------------------------

void DomeDriveRoboClaw::loadCalibrationFromEEPROM() {
    int32_t tpr = dome_load_calibration();
    if (tpr > 0) {
        fTicksPerDomeRev = tpr;
        DEBUG_PRINT("DOME: Calibration loaded from EEPROM tpr=");
        DEBUG_PRINTLN(fTicksPerDomeRev);
    } else {
        DEBUG_PRINTLN("DOME: No calibration in EEPROM — run dome=calibrate");
    }
}

void DomeDriveRoboClaw::saveCalibrationToEEPROM() {
    dome_save_calibration(fTicksPerDomeRev);
    DEBUG_PRINT("DOME: Calibration saved to EEPROM tpr=");
    DEBUG_PRINTLN(fTicksPerDomeRev);
}

// ---------------------------------------------------------------------------
// ISR handler
// ---------------------------------------------------------------------------

void DomeDriveRoboClaw::onHallTrigger() {
    uint32_t now = millis();
    if (now - fHallLastTriggerMs < kHallDebounceMs)
        return;  // debounce: ignore rapid re-triggers
    fHallLastTriggerMs = now;
    fHallTriggered     = true;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

int32_t DomeDriveRoboClaw::readEncoder() {
#ifndef UNIT_TEST
    uint8_t status = 0;
    bool    valid  = false;
    uint32_t raw = (fChannel == 1)
                       ? fRoboClaw.ReadEncM1(fAddress, &status, &valid)
                       : fRoboClaw.ReadEncM2(fAddress, &status, &valid);
#ifdef DOME_DEBUG
    DEBUG_PRINT(F("DOME: readEncoder valid="));
    DEBUG_PRINT(valid);
    DEBUG_PRINT(F(" raw="));
    DEBUG_PRINTLN((int32_t)raw);
#endif
    if (!valid) return fLastCommandedSpeed == 0.0f ? fHomeEncoderTick : fHomeEncoderTick;
    return (int32_t)raw;
#else
    return 0;
#endif
}

void DomeDriveRoboClaw::sendMotorCommand(float m) {
#ifndef UNIT_TEST
    // Clamp to -1..1 and apply max-speed modifier from DomeDrive base.
    m = max(-1.0f, min(m, 1.0f));
    int32_t speed = (int32_t)(m * (float)fQPPS);
#ifdef DOME_DEBUG
    DEBUG_PRINT(F("DOME: sendMotorCommand m="));
    DEBUG_PRINT(m);
    DEBUG_PRINT(F(" speed="));
    DEBUG_PRINTLN(speed);
#endif
    if (fChannel == 1)
        fRoboClaw.SpeedM1(fAddress, speed);
    else
        fRoboClaw.SpeedM2(fAddress, speed);
#else
    (void)m;
#endif
}

void DomeDriveRoboClaw::updatePosition() {
    if (!isCalibrated()) return;

    int32_t enc      = readEncoder();
    int32_t relTicks = enc - fHomeEncoderTick;

    fCurrentDegrees = dome_encoder_to_degrees(relTicks, fTicksPerDomeRev,
                                              (int)fFrontOffset);
}

bool DomeDriveRoboClaw::processHallTrigger() {
    if (!fHallTriggered) return false;

    // Atomically consume the flag.
    noInterrupts();
    fHallTriggered = false;
    interrupts();

    // Snapshot the encoder at the moment of the trigger.
    fHomeEncoderTick = readEncoder();
    return true;
}

int32_t DomeDriveRoboClaw::readEncoderSpeed(bool& valid) {
#ifndef UNIT_TEST
    uint8_t  status   = 0;
    uint32_t rawSpeed = (fChannel == 1)
                            ? fRoboClaw.ReadSpeedM1(fAddress, &status, &valid)
                            : fRoboClaw.ReadSpeedM2(fAddress, &status, &valid);
    return (int32_t)rawSpeed;
#else
    valid = true;
    return fMockEncoderSpeed;
#endif
}

void DomeDriveRoboClaw::checkObstruction() {
    bool    valid = false;
    int32_t actualSpeed = readEncoderSpeed(valid);
    if (!valid) return;

    uint32_t stallElapsed = fStallTimerActive ? (millis() - fStallStartMs) : 0;

    DomeObstructionResult r = dome_obstruction_check(
        fLastCommandedSpeed, actualSpeed,
        fStallTimerActive, stallElapsed,
        (uint32_t)fStallTimeoutMs
    );

    if (r.startTimer) {
        fStallTimerActive = true;
        fStallStartMs     = millis();
    }
    if (r.declareObstruction) {
        DEBUG_PRINTLN("DOME: Obstruction detected — motor stopped");
        stop();
        fState            = kStateObstructed;
        fStallTimerActive = false;
    }
    if (r.clearTimer) {
        fStallTimerActive = false;
        fStallStartMs     = 0;
    }
}

void DomeDriveRoboClaw::handleHoming(bool hallFired) {
    DomeHomingAction action = dome_homing_step(
        hallFired, millis() - fHomingStartMs, kHomingTimeoutMs);

    if (action == kHomingComplete) {
        stop();
        fState = kStateHomed;
        fDomePos.setDomeHomePosition(0);
        DEBUG_PRINTLN("DOME: Homing complete");
        checkObstruction();
        DomeDrive::animate();
        return;
    }

    if (action == kHomingTimeout) {
        stop();
        fState = kStateManual;
        DEBUG_PRINTLN("DOME: Homing timeout — hall sensor not detected. "
                      "Check wiring. Use dome=home to retry.");
        return;
    }

    // kHomingContinue: keep sweeping.
    // Drive the motor directly — autonomousDriveDome() is only consumed by
    // DomeDrive::animate() when a joystick is connected, so homing would
    // silently stall without a controller.  Call sendMotorCommand() directly
    // and still call DomeDrive::animate() to service any connected joystick.
    sendMotorCommand(kHomingSpeed);
    DomeDrive::animate();
}

void DomeDriveRoboClaw::handleCalibrating(bool hallFired) {
    if (hallFired) {
        fCalibrationTriggerCount++;
        int32_t currentTick = readEncoder();

        DomeCalibrationResult result = dome_calibration_trigger(
            fCalibrationTriggerCount,
            currentTick - fCalibrationStartTick,
            kCalibrationRotations
        );

        switch (result.action) {
            case kCalibrationSetReference:
                fCalibrationStartTick = currentTick;
                DEBUG_PRINTLN("DOME: Calibration reference set — counting revolutions");
                break;

            case kCalibrationContinue:
                break;

            case kCalibrationComplete:
                fTicksPerDomeRev = result.tpr;
                stop();
                fState = kStateHomed;
                saveCalibrationToEEPROM();
                DEBUG_PRINT("DOME: Calibration done. tpr=");
                DEBUG_PRINTLN(fTicksPerDomeRev);
                break;

            case kCalibrationError:
                DEBUG_PRINTLN("DOME: Calibration error — invalid tick count. "
                              "Ensure dome spins freely and retry.");
                stop();
                startHoming();
                return;
        }
    }

    if (fState == kStateCalibrating) {
        // Same reason as handleHoming: drive directly so calibration works
        // even without a connected joystick controller.
        sendMotorCommand(kCalibrationSpeed);
        DomeDrive::animate();
    }
}

// ---------------------------------------------------------------------------
// Static pure-logic helpers
// ---------------------------------------------------------------------------

/*static*/
int DomeDriveRoboClaw::encoderTicksToDegrees(int32_t relTicks,
                                              int32_t ticksPerRev,
                                              int     frontOffset) {
    return dome_encoder_to_degrees(relTicks, ticksPerRev, frontOffset);
}

/*static*/
int32_t DomeDriveRoboClaw::computeTicksPerDomeRev(int32_t totalEncoderTicks,
                                                   int     nRotations) {
    return dome_compute_ticks_per_rev(totalEncoderTicks, nRotations);
}

/*static*/
int DomeDriveRoboClaw::normalizeDegrees(int degrees) {
    return dome_normalize_degrees(degrees);
}

#endif // DOME_DRIVE == DOME_DRIVE_ROBOCLAW
