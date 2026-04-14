#pragma once

///////////////////////////////////////////////////////////////////////////////
// AmidalaController class declaration.
//
// The constructor, setup(), animate(), emergencyStop(), and domeEmergencyStop()
// are declared here and defined in src/controller.cpp — they reference
// servoDispatch, panservo, and tiltservo, which are globals defined in
// src/globals.cpp (accessed via extern declarations in controller.cpp).
///////////////////////////////////////////////////////////////////////////////

#include "drive_config.h"
#include "ReelTwo.h"
#if DRIVE_SYSTEM == DRIVE_SYSTEM_PWM
#include "drive/TankDrivePWM.h"
#endif
#if DRIVE_SYSTEM >= DRIVE_SYSTEM_ROBOTEQ_PWM &&                                \
    DRIVE_SYSTEM <= DRIVE_SYSTEM_ROBOTEQ_PWM_SERIAL
#include "drive/TankDriveRoboteq.h"
#endif
#if DOME_DRIVE == DRIVE_SYSTEM_SABER
#include "drive/TankDriveSabertooth.h"
#endif
#if DOME_DRIVE == DOME_DRIVE_PWM
#include "drive/DomeDrivePWM.h"
#elif DOME_DRIVE == DOME_DRIVE_SABER
#include "drive/DomeDriveSabertooth.h"
#elif DOME_DRIVE == DOME_DRIVE_ROBOCLAW
#include "dome_drive_roboclaw.h"
#endif
#include "core/MedianSampleBuffer.h"
#include "core/DelayCall.h"
#include <Wire.h>
#ifndef VMUSIC_SERIAL
#include <hcr.h>
#endif
#include "core.h"
#include "version.h"
#include "pin_config.h"
#include <EEPROM.h>
#include <XBee.h>
#include "ppm_decoder.h"
#include "i2c_utils.h"

// Forward-declare AmidalaController before the headers that use it as a pointer.
class AmidalaController;

#include "button_actions.h"
#include "audio.h"
#include "config.h"
#include "console.h"
#include "jevois_console.h"
#include "rdh_serial.h"
#include "xbee_remote.h"
#include "params.h"

class AmidalaController : public SetupEvent, public AnimatedEvent {
public:
  // Defined in src/controller.cpp — initialiser list references
  // servoDispatch (extern declared in that translation unit).
  AmidalaController();

  inline void processGesture(const char *gesture) {
    fConsole.processGesture(gesture);
  }

  inline void processButton(unsigned num) { fConsole.processButton(num); }

  inline void processLongButton(unsigned num) {
    fConsole.processLongButton(num);
  }

  inline void processAltButton(unsigned num) { fConsole.processAltButton(num); }

  inline bool isAltHeld() const { return fAltHeld; }
  inline void setAltHeld(bool held) { fAltHeld = held; }

  inline void setVolumeNoResponse(unsigned volume) {
    fAudio.setVolumeNoResponse(volume);
  }

  AmidalaConsole fConsole;
  AmidalaAudio fAudio;
  AmidalaConfig fConfig;
#ifdef VMUSIC_SERIAL
  VMusic fVMusic;
#else
  HCRVocalizer fHCR;
#endif
#ifdef EXPERIMENTAL_JEVOIS_STEERING
  JevoisConsole fJevois;
#endif
  DriveController fDriveStick;
  DomeController fDomeStick;
  XBeePocketRemote *remote[2] = {&fDriveStick, &fDomeStick};
  AmidalaParameters params;
#ifdef RDH_SERIAL
  RDHSerial fAutoDome;
#endif

#if DRIVE_SYSTEM == DRIVE_SYSTEM_SABER
  TankDriveSabertooth fTankDrive;
#elif DRIVE_SYSTEM == DRIVE_SYSTEM_PWM
  TankDrivePWM fTankDrive;
#elif DRIVE_SYSTEM == DRIVE_SYSTEM_ROBOTEQ_PWM
  TankDriveRoboteq fTankDrive;
#elif DRIVE_SYSTEM == DRIVE_SYSTEM_ROBOTEQ_SERIAL
  TankDriveRoboteq fTankDrive;
#elif DRIVE_SYSTEM == DRIVE_SYSTEM_ROBOTEQ_PWM_SERIAL
  TankDriveRoboteq fTankDrive;
#elif defined(DRIVE_SYSTEM)
#error Unsupported DRIVE_SYSTEM
#endif

#if DOME_DRIVE == DOME_DRIVE_SABER
  DomeDriveSabertooth fDomeDrive;
#elif DOME_DRIVE == DOME_DRIVE_PWM
  DomeDrivePWM fDomeDrive;
#elif DOME_DRIVE == DOME_DRIVE_ROBOCLAW
  DomeDriveRoboClaw fDomeDrive;
#endif

  bool checkRCMode() {
    static bool sRCMode;
    if (digitalRead(RCSEL_PIN) == LOW) {
      if (!sRCMode) {
        fConsole.println("RC Enabled (" +
                         String(params.getRadioChannelCount()) + " Channels)");
        digitalWrite(STATUS_RC_PIN, HIGH);
        sRCMode = true;
      }
      return true;
    }
    if (sRCMode) {
      digitalWrite(STATUS_RC_PIN, LOW);
      sRCMode = false;
    }
    return false;
  }

  bool checkSel2Mode() {
    static bool sSel2Mode;
    if (digitalRead(SEL2_PIN) == LOW) {
      if (!sSel2Mode) {
        // digitalWrite?
        sSel2Mode = true;
      }
      return true;
    }
    if (sSel2Mode) {
      // digitalWrite?
      sSel2Mode = false;
    }
    return false;
  }

  unsigned getDomeMode() {
#ifdef RDH_SERIAL
    return fAutoDome.getMode();
#else
    return 0;
#endif
  }

  unsigned getDomeHome() {
#ifdef RDH_SERIAL
    return fAutoDome.getHome();
#else
    return 0;
#endif
  }

  unsigned getDomePosition() {
#if DOME_DRIVE == DOME_DRIVE_ROBOCLAW
    return fDomeDrive.getCurrentDegrees();
#elif defined(RDH_SERIAL)
    return fAutoDome.getAngle();
#else
    return 0;
#endif
  }

  /**
   * Process a "dome=<cmd>" console command for the RoboClaw dome drive.
   * Commands: home, calibrate, stop, front, rand, status, <N>, +<N>, -<N>
   * Defined in src/controller.cpp.
   */
  void processDomeCommand(const char* cmd);

  /**
   * Execute a dome command from a button or gesture action.
   * subcmd is a ButtonAction::DomeCmdType value; arg is the angle or delta
   * in degrees (only used for kDomeGotoAbs, kDomeRelPos, kDomeRelNeg).
   * Defined in src/controller.cpp.
   */
  void processDomeCmd(uint8_t subcmd, uint8_t arg);

  bool getDomeIMU() { return params.domeimu; }

  void setDomeHome(unsigned pos) {
#if DOME_DRIVE == DOME_DRIVE_ROBOCLAW
    // No-op: the RoboClaw drive derives home from the hall sensor trigger.
    (void)pos;
#elif defined(RDH_SERIAL)
    fAutoDome.setDomeHomePosition(pos);
#else
    (void)pos;
#endif
  }

  unsigned getVolume() { return params.volume; }

  void setTargetSteering(TargetSteering *steering) {
#ifdef DRIVE_SYSTEM
    fTankDrive.setTargetSteering(steering);
#endif
  }

  void enableController() {
#ifdef DRIVE_SYSTEM
    fTankDrive.setEnable(true);
#endif
  }

  void disableController() {
    emergencyStop();
#ifdef DRIVE_SYSTEM
    fTankDrive.setEnable(false);
#endif
  }

  // Defined in src/controller.cpp — references servoDispatch global.
  void emergencyStop();

  void enableDomeController() {
#ifdef DOME_DRIVE
    fDomeDrive.setEnable(true);
#endif
  }

  void disableDomeController() {
#ifdef DOME_DRIVE
    domeEmergencyStop();
    fDomeDrive.setEnable(false);
#endif
  }

  // Defined in src/controller.cpp — references servoDispatch global.
  void domeEmergencyStop();

  void setDigitalPin(int pin, bool state) {
    if (pin >= 1 && pin <= 8) {
      params.D[--pin].state = state;
      digitalWrite(DOUT1_PIN + pin, (state) ? HIGH : LOW);
    }
  }

  bool getDigitalPin(int pin) {
    if (pin >= 1 && pin <= 8) {
      return params.D[--pin].state;
    }
    return false;
  }

  bool loadConfig() {
#ifdef VMUSIC_SERIAL
    fConsole.println(F("Waiting for VMusic"));
    if (!fVMusic.init()) {
      fConsole.println(F("VMusic unavailable"));
      return false;
    }
    fConsole.println(F("Reading Config File"));
    return readConfig(fVMusic, fConsole);
#else
    return readConfig(fConsole);
#endif
  }

  void sendSerialString(const char *str) {
    char ch;
    while ((ch = *str++) != '\0') {
      if (ch == params.serialdelim)
        ch = params.serialeol;
      SERIAL.write(ch);
    }
    ch = params.serialeol;
    SERIAL.write(ch);
  }

  // Defined in src/controller.cpp — references servoDispatch, panservo,
  // and tiltservo (extern declared in that translation unit).
  virtual void setup() override;

  // Defined in src/controller.cpp — references servoDispatch global.
  virtual void animate() override;

private:
  // DriveController and DomeController call setDriveThrottle/setDomeThrottle,
  // which are intentionally private to all other callers.
  void executeDomeAction(uint8_t subcmd, int arg);
  friend class DriveController;
  friend class DomeController;

  XBee fXBee;
  ZBRxIoSampleResponse fResponse;
  PPMDecoder fPPMDecoder;
  bool fMinimal = true;
  bool fAltHeld = false;
  uint32_t fDriveStateMillis = 0;
  uint32_t fDomeStateMillis = 0;
  float fDomeThrottle = 0;
  float fDriveThrottle = 0;

  inline float getDomeThrottle() { return fDomeThrottle; }

  inline void setDomeThrottle(float throttle) { fDomeThrottle = throttle; }

  inline float getDriveThrottle() { return fDriveThrottle; }

  inline void setDriveThrottle(float throttle) { fDriveThrottle = throttle; }

  void setDomeHomePosition() {
#if DOME_DRIVE == DOME_DRIVE_ROBOCLAW
    fDomeDrive.startHoming();
#elif defined(RDH_SERIAL)
    fAutoDome.setDomeHomePosition();
#endif
  }

  void toggleRandomDome() {
#if DOME_DRIVE == DOME_DRIVE_ROBOCLAW
    if (fDomeDrive.isHomed())
        fDomeDrive.enableRandomMode();
    else
        fDomeDrive.disableAutoMode();
#elif defined(RDH_SERIAL)
    fAutoDome.toggleRandomDome();
#endif
  }

};
