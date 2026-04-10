// dome_drive_roboclaw.h
// Closed-loop dome drive using a Pololu encoder motor, RoboClaw motor
// controller, and a single hall-effect sensor for homing.
//
// Architecture
// ------------
// DomeDriveRoboClaw inherits DomeDrive (Reeltwo), which already provides:
//   - Joystick-to-motor throttle mapping, deadband, and acceleration scaling
//   - Autonomous movement modes (kHome, kRandom, kTarget) via DomePosition
//   - moveDomeToTarget() with near-target deceleration
//
// DomeDriveRoboClaw also implements DomePositionProvider so it can feed its
// own encoder-derived position back into DomeDrive's auto-movement logic.
//
// Position coordinate system
// --------------------------
//  0° = dome "front" (configured via domefront= in config.txt)
//  Angles increase clockwise when viewed from above (consistent with the
//  joystick: stick right → positive angle).
//  The hall sensor trigger establishes the raw encoder zero reference; the
//  front offset is subtracted so that getAngle() returns 0 at the front.
//
// State machine
// -------------
//  kStateManual     - no home reference established; joystick works, auto
//                     modes disabled
//  kStateHoming     - slow autonomous sweep looking for the hall sensor
//  kStateHomed      - hall found; all auto modes available
//  kStateCalibrating- slow autonomous sweep counting 10 dome revolutions to
//                     compute and persist the motor-to-dome encoder ratio
//  kStateObstructed - stall detected; motor stopped; auto modes locked out
//                     until the joystick is moved manually
//
// Calibration persistence (EEPROM)
// ---------------------------------
// The motor-to-dome ratio (ticksPerDomeRev) is written to EEPROM at offset
// DOME_ROBOCLAW_EEPROM_ADDR with the 4-byte signature defined in
// drive_config.h (see DOME_ROBOCLAW_EEPROM_* constants).  On every boot the
// firmware checks for the signature; if present, calibration is restored
// automatically so the user only needs to run "dome=calibrate" once per
// mechanical assembly.

#pragma once

#if DOME_DRIVE == DOME_DRIVE_ROBOCLAW

#include "drive/DomeDrive.h"
#include "drive/DomePosition.h"
#include "drive/DomePositionProvider.h"
#include "drive_config.h"
#include "pin_config.h"

#ifndef UNIT_TEST
#include <RoboClaw.h>
#include <EEPROM.h>
#endif

class DomeDriveRoboClaw : public DomeDrive, public DomePositionProvider {
public:
    // ---- Operational states -------------------------------------------------

    enum State {
        kStateManual,       ///< No home reference yet; joystick only
        kStateHoming,       ///< Sweeping for hall sensor
        kStateHomed,        ///< Homed; auto modes available
        kStateCalibrating,  ///< Counting dome revolutions for ratio
        kStateObstructed,   ///< Stall detected; awaiting manual clearance
    };

    // ---- Constructor --------------------------------------------------------

    /**
     * @param serial   Hardware serial port wired to the RoboClaw (e.g. Serial2).
     * @param address  RoboClaw packet-serial address (default 128 / 0x80).
     * @param channel  Motor channel: 1 = M1, 2 = M2.
     * @param hallPin  Digital pin connected to the hall sensor output (active-LOW).
     * @param stick    Dome joystick controller.
     */
#ifndef UNIT_TEST
    DomeDriveRoboClaw(HardwareSerial& serial,
                      uint8_t address,
                      uint8_t channel,
                      uint8_t hallPin,
                      JoystickController& stick);
#else
    // Test-only constructor: no hardware required.
    DomeDriveRoboClaw(uint8_t address,
                      uint8_t channel,
                      uint8_t hallPin,
                      JoystickController& stick);
#endif

    // ---- DomeDrive / AnimatedEvent overrides --------------------------------

    virtual void setup() override;
    virtual void animate() override;
    virtual void stop() override;

    // ---- DomePositionProvider interface -------------------------------------
    // Called by DomeDrive's DomePosition every animate() cycle.

    /** Returns true once the hall sensor has been found (homing complete). */
    virtual bool ready() override { return fState == kStateHomed; }

    /** Current dome angle in degrees (0 = front, increases clockwise). */
    virtual int getAngle() override { return fCurrentDegrees; }

    // ---- RoboClaw-specific public API ---------------------------------------

    /**
     * Start an automatic homing sweep.  The dome spins slowly until the hall
     * sensor fires, then stops.  Called automatically during setup() when the
     * dome drive is active.
     */
    void startHoming();

    /**
     * Start the 10-revolution calibration procedure.
     * The dome must already be homed (hall sensor functional).
     * On completion the motor-to-dome ratio is saved to EEPROM automatically.
     */
    void startCalibration();

    /**
     * Command the dome to an absolute angle relative to the configured front.
     * Requires: homed + calibrated.
     * @param degrees 0–359, where 0 = front.
     */
    void goToAngle(int degrees);

    /**
     * Command a relative move from the current position.
     * Positive = clockwise, negative = counterclockwise.
     * Requires: homed + calibrated.
     */
    void goToRelative(int delta);

    /** Switch to random wander mode (bounded by domeseekl/domeseekr). */
    void enableRandomMode();

    /** Return to manual / idle mode (cancels all autonomous movement). */
    void disableAutoMode();

    /** Set the hall-sensor-to-front offset (degrees). Applied immediately. */
    void setFrontOffset(uint16_t degrees) { fFrontOffset = degrees; }

    /** Set max speed as fraction of QPPS (0.0–1.0). */
    void setMaxSpeedPct(float pct);

    /** Update the QPPS value used to scale SpeedM1/M2 commands. */
    void setQPPS(uint16_t qpps) { fQPPS = qpps; }

    /** Update the stall-detection timeout (ms). */
    void setStallTimeout(uint16_t ms) { fStallTimeoutMs = ms; }

    /** Configure DomePosition auto-mode timing from loaded params. */
    void applyDomePositionParams(uint8_t homeMinDelay, uint8_t homeMaxDelay,
                                 uint8_t seekMinDelay, uint8_t seekMaxDelay,
                                 uint8_t seekLeft, uint8_t seekRight,
                                 uint8_t fudge,
                                 uint8_t speedHome, uint8_t speedTarget,
                                 uint8_t speedSeek, uint8_t speedMin);

    State getState() const { return fState; }
    bool  isHomed()       const { return fState >= kStateHomed; }
    bool  isCalibrated()  const { return fTicksPerDomeRev > 0; }
    int   getCurrentDegrees() const { return fCurrentDegrees; }
    int32_t getTicksPerDomeRev() const { return fTicksPerDomeRev; }

    /** Load calibration from EEPROM. Called during setup(). */
    void loadCalibrationFromEEPROM();

    /** Persist current calibration to EEPROM. Called after calibration. */
    void saveCalibrationToEEPROM();

    /** Print one-line status to the supplied output stream. */
    template <typename TOutput>
    void printStatus(TOutput& out) const {
        out.print(F("Dome: "));
        out.print(fCurrentDegrees);
        out.print(F("deg state="));
        switch (fState) {
            case kStateManual:      out.print(F("Manual"));      break;
            case kStateHoming:      out.print(F("Homing"));      break;
            case kStateHomed:       out.print(F("Homed"));       break;
            case kStateCalibrating: out.print(F("Calibrating")); break;
            case kStateObstructed:  out.print(F("OBSTRUCTED"));  break;
        }
        out.print(F(" cal="));
        out.print(isCalibrated() ? F("yes tpr=") : F("no"));
        if (isCalibrated()) out.print(fTicksPerDomeRev);
        out.println();
    }

    // ---- ISR glue (must be public for the static free function) -------------

    /** Called from the hall-sensor interrupt. Do not call directly. */
    void onHallTrigger();

    // ---- Pure-logic static helpers (fully unit-testable) --------------------

    /**
     * Convert a signed encoder tick count (relative to the home reference)
     * into a dome angle in degrees (0–359), applying the front offset.
     *
     * @param relTicks      enc - homeEncoderTick (may be negative)
     * @param ticksPerRev   motor encoder ticks per one full dome revolution
     * @param frontOffset   degrees from hall-sensor zero to dome "front"
     * @return              Dome angle, 0–359, where 0 = front
     */
    static int encoderTicksToDegrees(int32_t relTicks,
                                     int32_t ticksPerRev,
                                     int     frontOffset);

    /**
     * Compute ticks-per-dome-revolution from raw calibration measurements.
     * This is the motor encoder ticks counted over nRotations dome revolutions.
     *
     * @param totalEncoderTicks  enc(end) - enc(start), always positive
     * @param nRotations         number of complete dome revolutions recorded
     * @return                   ticks per dome revolution (0 if inputs invalid)
     */
    static int32_t computeTicksPerDomeRev(int32_t totalEncoderTicks,
                                          int     nRotations);

    /**
     * Normalize any integer to the 0–359 range.
     */
    static int normalizeDegrees(int degrees);

private:
    // ---- Hardware state (excluded from UNIT_TEST builds) --------------------

#ifndef UNIT_TEST
    RoboClaw fRoboClaw;
#endif

    uint8_t  fAddress;
    uint8_t  fChannel;       ///< 1 = M1, 2 = M2
    uint8_t  fHallPin;

    // ---- Configuration ------------------------------------------------------

    uint16_t fFrontOffset    = 0;     ///< Hall-to-front offset in degrees
    uint16_t fQPPS           = DEFAULT_DOME_ROBOCLAW_QPPS;
    uint16_t fStallTimeoutMs = DEFAULT_DOME_STALL_TIMEOUT_MS;

    // ---- State machine ------------------------------------------------------

    State fState = kStateManual;

    // ---- Position tracking --------------------------------------------------

    int32_t fHomeEncoderTick  = 0;    ///< Encoder value at last hall trigger
    int32_t fTicksPerDomeRev  = 0;    ///< 0 = not calibrated
    int     fCurrentDegrees   = 0;    ///< Last computed dome angle (0 = front)
    float   fLastCommandedSpeed = 0.0f; ///< For obstruction detection

    // ---- Hall sensor --------------------------------------------------------

    volatile bool     fHallTriggered      = false;
    volatile uint32_t fHallLastTriggerMs  = 0;

    static constexpr uint32_t kHallDebounceMs = DEFAULT_DOME_HALL_DEBOUNCE_MS;

    // ---- Homing state -------------------------------------------------------

    uint32_t fHomingStartMs = 0;
    static constexpr uint32_t kHomingTimeoutMs = DEFAULT_DOME_HOMING_TIMEOUT_MS;
    static constexpr float    kHomingSpeed     = DEFAULT_DOME_HOMING_SPEED;

    // ---- Calibration state --------------------------------------------------

    int     fCalibrationTriggerCount = 0;
    int32_t fCalibrationStartTick    = 0;

    static constexpr int   kCalibrationRotations = DOME_CALIBRATION_ROTATIONS;
    static constexpr float kCalibrationSpeed     = DEFAULT_DOME_CALIBRATE_SPEED;

    // ---- Obstruction detection ----------------------------------------------

    uint32_t fStallStartMs      = 0;
    bool     fStallTimerActive  = false;

    // ---- DomePosition hook --------------------------------------------------

    // fDomePos wraps *this (as DomePositionProvider) and is registered with the
    // DomeDrive base via setDomePosition(&fDomePos) in setup().
    // Initialising with *this in the member initialiser is safe because
    // DomePosition's constructor only stores the reference — it does not call
    // any virtual methods.
    DomePosition fDomePos;

    // ---- ISR singleton ------------------------------------------------------

    static DomeDriveRoboClaw* sInstance;

    static void hallISR();

    // ---- Private helpers ----------------------------------------------------

    int32_t readEncoder();
    /**
     * Read the current encoder velocity from the RoboClaw.
     * @param[out] valid  Set to true if the read succeeded.
     * @return            Signed ticks/sec; 0 and valid=false on comms failure.
     *
     * In UNIT_TEST builds this returns fMockEncoderSpeed with valid=true so
     * the full checkObstruction() path (including state transitions) can be
     * exercised without hardware.
     */
    int32_t readEncoderSpeed(bool& valid);
    void    sendMotorCommand(float m);
    void    updatePosition();
    bool    processHallTrigger();   ///< Returns true if a trigger was consumed
    void    checkObstruction();
    void    handleHoming(bool hallFired);
    void    handleCalibrating(bool hallFired);

#ifdef UNIT_TEST
public:
    /// Inject a synthetic encoder speed for testing checkObstruction().
    void    setMockEncoderSpeed(int32_t s)  { fMockEncoderSpeed = s; }
    /// Expose state setter so tests can put the drive into kStateHomed etc.
    void    setStateForTest(State s)        { fState = s; }
    /// Expose commanded-speed setter (normally set via motor() callback).
    void    setLastCommandedSpeedForTest(float s) { fLastCommandedSpeed = s; }
    /// Expose stall-timer state for assertion.
    bool    getStallTimerActiveForTest() const { return fStallTimerActive; }
    State   getStateForTest()            const { return fState; }
    /// Directly exercise the full checkObstruction() wrapper.
    void    testCheckObstruction() { checkObstruction(); }
private:
    int32_t fMockEncoderSpeed = 0;
#endif
};

// ---------------------------------------------------------------------------
// DomeDriveRoboClaw is a no-op compile-time stub when a different dome drive
// is selected, so controller.h can always include this header safely.
// ---------------------------------------------------------------------------
#else // DOME_DRIVE != DOME_DRIVE_ROBOCLAW

// Empty stub so #include "dome_drive_roboclaw.h" is always valid.
class DomeController; // forward-declare to satisfy IDE parsers

#endif // DOME_DRIVE == DOME_DRIVE_ROBOCLAW
