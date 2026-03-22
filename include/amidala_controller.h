#pragma once

///////////////////////////////////////////////////////////////////////////////
// AmidalaController class declaration.
//
// The constructor, setup(), animate(), emergencyStop(), and domeEmergencyStop()
// are declared here but defined in src/AmidalaFirmware.ino — they reference
// servoDispatch, which is a global defined in that translation unit.
///////////////////////////////////////////////////////////////////////////////

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
#endif
#include "ServoDispatchDirect.h"
#include "ServoEasing.h"
#include "core/MedianSampleBuffer.h"
#include "core/DelayCall.h"
#include <Wire.h>
#ifndef VMUSIC_SERIAL
#include <hcr.h>
#endif
#include "amidala_core.h"
#include "config_reader.h"
#include "pin_config.h"
#include <EEPROM.h>
#include <XBee.h>
#include "ppm_decoder.h"
#include "i2c_utils.h"

// Forward-declare AmidalaController before the headers that use it as a pointer.
class AmidalaController;

#include "button_actions.h"
#include "console.h"
#include "jevois_console.h"
#include "rdh_serial.h"
#include "xbee_remote.h"
#include "amidala_params.h"

class AmidalaController : public SetupEvent, public AnimatedEvent {
public:
  // Defined in src/AmidalaFirmware.ino — initialiser list references
  // servoDispatch (a global in that translation unit).
  AmidalaController();

  inline void processGesture(const char *gesture) {
    fConsole.processGesture(gesture);
  }

  inline void processButton(unsigned num) { fConsole.processButton(num); }

  inline void processLongButton(unsigned num) {
    fConsole.processLongButton(num);
  }

  inline void setVolumeNoResponse(unsigned volume) {
    fConsole.setVolumeNoResponse(volume);
  }

  AmidalaConsole fConsole;
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
#elif defined(DOME_DRIVE)
#error Unsupported DOME_DRIVE
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
#ifdef RDH_SERIAL
    return fAutoDome.getAngle();
#else
    return 0;
#endif
  }

  bool getDomeIMU() { return params.domeimu; }

  void setDomeHome(unsigned pos) {
#ifdef RDH_SERIAL
    fAutoDome.setDomeHomePosition(pos);
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

  // Defined in src/AmidalaFirmware.ino — references servoDispatch global.
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

  // Defined in src/AmidalaFirmware.ino — references servoDispatch global.
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

  void sendAuxString(const char *str) {
    char ch;
    while ((ch = *str++) != '\0') {
      if (ch == params.auxdelim)
        ch = params.auxeol;
      AUX_SERIAL.write(ch);
    }
    ch = params.auxeol;
    AUX_SERIAL.write(ch);
  }

  // Defined in src/AmidalaFirmware.ino — references servoDispatch, panservo,
  // tiltservo, hcrDelayedInit, and other globals in that translation unit.
  virtual void setup() override;

  // Defined in src/AmidalaFirmware.ino — references servoDispatch global.
  virtual void animate() override;

private:
  XBee fXBee;
  ZBRxIoSampleResponse fResponse;
  PPMDecoder fPPMDecoder;
  bool fMinimal = true;
  uint32_t fDriveStateMillis = 0;
  uint32_t fDomeStateMillis = 0;
  float fDomeThrottle = 0;
  float fDriveThrottle = 0;

  inline float getDomeThrottle() { return fDomeThrottle; }

  inline void setDomeThrottle(float throttle) { fDomeThrottle = throttle; }

  inline float getDriveThrottle() { return fDriveThrottle; }

  inline void setDriveThrottle(float throttle) { fDriveThrottle = throttle; }

  void setDomeHomePosition() {
#ifdef RDH_SERIAL
    fAutoDome.setDomeHomePosition();
#endif
  }

  void toggleRandomDome() {
#ifdef RDH_SERIAL
    fAutoDome.toggleRandomDome();
#endif
  }

  friend class AmidalaConsole;
  friend class DriveController;
  friend class DomeController;
};
