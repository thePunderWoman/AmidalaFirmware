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

    // 3. Sequence-pause watchdog: if the expiry has passed, auto-clear so the
    //    dome resumes auto-behaviors even if dome=seqoff was never received.
    if (fSequenceActive &&
        (int32_t)(millis() - fSequencePauseExpiryMs) >= 0) {
        endSequencePause();
    }

    // 4. Run per-state logic.
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
            // Direct closed-loop control — same pattern as homing/calibration.
            // DomeDrive::animate() → domeStick() requires abs(m) != 0 or
            // fAutoDrive != 0 to enter the motor block.  With the stick zeroed
            // both are 0, so the entire motor path (including DomePosition kTarget)
            // is skipped.  Drive the motor directly instead and return early.
            handleAbsoluteStick();
            return;

        case kStateRandom:
            if (fSequenceActive) {
                // Auto-wander suppressed: skip handleRandomMode() so it can't
                // move the dome.  Do NOT command motor=0 — the sequence may
                // be issuing its own programmed moves, and we shouldn't fight
                // them.
                return;
            }
            handleRandomMode();
            return;

        default:
            break;
    }

    // 5. Obstruction detection (only while the motor is being commanded).
    if (fState == kStateHomed) {
        checkObstruction();
    }

    // 6. Let the DomeDrive base handle joystick + DomePosition auto modes.
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
    if (!isHomed() || !isCalibrated()) {
        DEBUG_PRINTLN("DOME: Cannot enable random — not homed/calibrated");
        return;
    }
    fRandomAtTarget   = true;
    fRandomNextMoveMs = millis();   // pick first target immediately
    fState            = kStateRandom;
}

void DomeDriveRoboClaw::disableRandomMode() {
    if (fState == kStateRandom) {
        fState = kStateHomed;
        disableAutoMode();
    }
}

void DomeDriveRoboClaw::toggleRandomMode() {
    if (isRandomMode()) {
        disableRandomMode();
    } else {
        enableRandomMode();
    }
}

void DomeDriveRoboClaw::disableAutoMode() {
    fDomePos.setDomeDefaultMode(DomePosition::kOff);
}

void DomeDriveRoboClaw::enableAbsoluteStickMode() {
    if (!isHomed() || !isCalibrated()) return;
    // Hold current position until the stick gives a new target.
    fAbsStickTargetDegrees = fCurrentDegrees;
    fState = kStateAbsoluteStick;
}

void DomeDriveRoboClaw::disableAbsoluteStickMode() {
    if (fState == kStateAbsoluteStick) {
        fState = kStateHomed;
        disableAutoMode();
    }
}

void DomeDriveRoboClaw::toggleAbsoluteStickMode() {
    if (fState == kStateAbsoluteStick) {
        disableAbsoluteStickMode();
    } else {
        enableAbsoluteStickMode();
    }
}

void DomeDriveRoboClaw::startSequencePause(uint32_t durationMs) {
    if (!fSequenceActive) {
        // Snapshot the user's current auto-mode so endSequencePause() can
        // restore it.  setDomeDefaultMode() also resets the active mode, so
        // random wander stops immediately on this cycle.
        fPreSequenceDefaultMode = fDomePos.getDomeDefaultMode();
        fDomePos.setDomeDefaultMode(DomePosition::kOff);
        // One-shot stop so the motor doesn't coast on its last commanded speed
        // until the sequence issues its own move (or the RoboClaw packet-serial
        // timeout elapses).  Only done on the initial activation — subsequent
        // refresh calls to extend the window must not interrupt in-flight moves.
        sendMotorCommand(0.0f);
        fSequenceActive = true;
    }
    fSequencePauseExpiryMs = millis() + durationMs;
    DEBUG_PRINT("DOME: Sequence pause active for ");
    DEBUG_PRINT(durationMs);
    DEBUG_PRINTLN(" ms");
}

void DomeDriveRoboClaw::endSequencePause() {
    if (!fSequenceActive) return;
    fDomePos.setDomeDefaultMode(fPreSequenceDefaultMode);
    fSequenceActive        = false;
    fSequencePauseExpiryMs = 0;
    DEBUG_PRINTLN("DOME: Sequence pause cleared");
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
        uint8_t speedSeek,    uint8_t speedMin,
        uint8_t decelZone) {
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
    // Cache params used by handleAbsoluteStick() and handleRandomMode().
    fAbsStickFudge       = fudge;
    fAbsStickSpeedMin    = speedMin;
    fAbsStickSpeedTarget = speedTarget;
    fRandomSeekMinDelay  = seekMinDelay;
    fRandomSeekMaxDelay  = seekMaxDelay;
    fRandomSeekLeft      = seekLeft;
    fRandomSeekRight     = seekRight;
    fAbsStickDecelZone   = decelZone;
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
    if (!valid) return fHomeEncoderTick;
    return (int32_t)raw;
#else
    return 0;
#endif
}

void DomeDriveRoboClaw::sendMotorCommand(float m) {
    fLastCommandedSpeed = m;
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
        // The hall sensor rarely sits at the dome's front; after finding it,
        // drive to 0° (configured front) so the dome rests facing forward at
        // startup.  Requires calibration to translate degrees to ticks.
        if (isCalibrated() && fFrontOffset != 0) {
            DEBUG_PRINTLN("DOME: Parking at front (0 deg)");
            goToAngle(0);
        }
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
    // silently stall without a controller.
    // Do NOT call DomeDrive::animate() here: when a controller is connected,
    // domeStick() sees idle input (m=0) and immediately calls motor(0),
    // cancelling the homing command every cycle.  Joystick is intentionally
    // ignored during homing — it resumes normally once kStateHomed is reached.
    sendMotorCommand(kHomingSpeed);
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
        // Same reason as handleHoming: drive directly, and do NOT call
        // DomeDrive::animate() — a connected joystick at idle would call
        // motor(0) and cancel the calibration sweep every cycle.
        sendMotorCommand(kCalibrationSpeed);
    }
}

bool DomeDriveRoboClaw::driveClosedLoop(int targetDegrees) {
    int   error = dome_angular_error(targetDegrees, fCurrentDegrees);
    float speed = dome_abs_stick_speed(error,
                                       fAbsStickFudge,
                                       fAbsStickSpeedMin,
                                       fAbsStickSpeedTarget,
                                       fAbsStickDecelZone);
    sendMotorCommand(speed);
    checkObstruction();
    return abs(error) <= fAbsStickFudge;
}

void DomeDriveRoboClaw::handleAbsoluteStick() {
    // Update target from stick if connected and outside the deadband.
    if (fDomeStick.isConnected()) {
        int lx = useLeftStick() ? fDomeStick.state.analog.stick.lx
                                : fDomeStick.state.analog.stick.rx;
        int ly = useLeftStick() ? fDomeStick.state.analog.stick.ly
                                : fDomeStick.state.analog.stick.ry;
        // JoystickController ly is positive = pushed toward user (down/back).
        // dome_stick_to_angle expects positive = forward, so negate ly.
        int newTarget = dome_stick_to_angle(lx, -ly);
        if (newTarget >= 0)
            fAbsStickTargetDegrees = newTarget;
    }

    // Proportional closed-loop: drive toward fAbsStickTargetDegrees.
    // Do NOT call DomeDrive::animate() — domeStick() gates all motor commands
    // on abs(m) != 0 or fAutoDrive != 0; with neither set the motor would stall.
    driveClosedLoop(fAbsStickTargetDegrees);
}

void DomeDriveRoboClaw::handleRandomMode() {
    if (fRandomAtTarget) {
        if (millis() < fRandomNextMoveMs) {
            // Still in the inter-move pause — hold position.
            sendMotorCommand(0.0f);
            return;
        }
        // Pick a target angle outside the fudge zone.  Retry up to 10 times to
        // avoid picking an angle we're already sitting at (which would cause an
        // immediate "arrived" and a rapid series of zero-distance moves).
        int target = fCurrentDegrees;
        for (int i = 0; i < 10; i++) {
            bool  goLeft   = (random(2) == 0);
            int   maxRange = goLeft ? (int)fRandomSeekLeft : (int)fRandomSeekRight;
            if (maxRange < DOME_RANDOM_MOVE_MIN_DEGREES)
                maxRange = DOME_RANDOM_MOVE_MIN_DEGREES;
            int dist = (int)random(DOME_RANDOM_MOVE_MIN_DEGREES, maxRange + 1);
            target   = normalizeDegrees(goLeft ? -dist : dist);
            if (abs(dome_angular_error(target, fCurrentDegrees)) > fAbsStickFudge)
                break;
        }
        fRandomTargetDegrees = target;
        fRandomAtTarget      = false;
        DEBUG_PRINT("DOME: Random target=");
        DEBUG_PRINTLN(fRandomTargetDegrees);
    }

    // Drive toward the target; arrived = within fudge zone.
    if (driveClosedLoop(fRandomTargetDegrees)) {
        fRandomAtTarget = true;
        long minMs = (long)fRandomSeekMinDelay * 1000L;
        long maxMs = (long)fRandomSeekMaxDelay * 1000L;
        fRandomNextMoveMs = millis() + (uint32_t)random(minMs, maxMs + 1);
        DEBUG_PRINT("DOME: Random arrived, next move in ");
        DEBUG_PRINT((fRandomNextMoveMs - millis()) / 1000);
        DEBUG_PRINTLN("s");
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
