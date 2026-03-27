#include "controller.h"

// Hardware globals defined in src/globals.cpp.
// ServoDispatch& avoids including ServoDispatchDirect.h here, which would
// pull in ServoDispatchPrivate.h and duplicate ISR definitions.
extern ServoDispatch& servoDispatch;
extern ServoPD panservo;
extern ServoPD tiltservo;

AmidalaController::AmidalaController()
    : fConsole(),
      fHCR(&Serial3, HCR_BAUD_RATE),
#ifdef VMUSIC_SERIAL
      fVMusic(VMUSIC_SERIAL),
#endif
      fDriveStick(this), fDomeStick(this),
#ifdef RDH_SERIAL
      fAutoDome(RDH_SERIAL),
#endif
#if DRIVE_SYSTEM == DRIVE_SYSTEM_SABER
      fTankDrive(128, DRIVE_SERIAL, fDriveStick),
#elif DRIVE_SYSTEM == DRIVE_SYSTEM_PWM
      fTankDrive(servoDispatch, 1, 0, 4, fDriveStick),
#elif DRIVE_SYSTEM == DRIVE_SYSTEM_ROBOTEQ_PWM
      fTankDrive(servoDispatch, 1, 0, 4, fDriveStick),
#elif DRIVE_SYSTEM == DRIVE_SYSTEM_ROBOTEQ_SERIAL
      fTankDrive(DRIVE_SERIAL, fDriveStick),
#elif DRIVE_SYSTEM == DRIVE_SYSTEM_ROBOTEQ_PWM_SERIAL
      fTankDrive(DRIVE_SERIAL, servoDispatch, 1, 0, 4, fDriveStick),
#elif defined(DRIVE_SYSTEM)
#error Unsupported DRIVE_SYSTEM
#endif
#if DOME_DRIVE == DOME_DRIVE_SABER
      fDomeDrive(129, DOME_DRIVE_SERIAL, fDomeStick),
#elif DOME_DRIVE == DOME_DRIVE_PWM
      fDomeDrive(servoDispatch, 3, fDomeStick),
#elif defined(DOME_DRIVE)
#error Unsupported DOME_DRIVE
#endif
      fPPMDecoder(PPMIN_PIN, params.getRadioChannelCount()) {
}

void AmidalaController::emergencyStop() {
#ifdef DRIVE_SYSTEM
  fTankDrive.stop();
  // Force neutral PWM signals as a safety backup
  servoDispatch.moveTo(1, 0.5); // Drive Left
  servoDispatch.moveTo(0, 0.5); // Drive Right
  // If using serial, the Roboteq script handles its own RWD 100 watchdog
#endif
}

void AmidalaController::domeEmergencyStop() {
#ifdef DOME_DRIVE
  fDomeDrive.stop();
  // Force neutral PWM signal as a safety backup
  servoDispatch.moveTo(3, 0.5); // Dome
#endif
}

void AmidalaController::setup() {
  fConsole.println(F("Loading config from EEPROM"));
  params.init();

  fConsole.init(this);
  fConfig.init(this);

#ifdef EXPERIMENTAL_JEVOIS_STEERING
  fJevois.init(this);
#endif

  fConsole.println(F("Reading Config File"));
  fConsole.setMinimal(!loadConfig());
  fConfig.showCurrentConfiguration();

  fConsole.println(F("Activating Servos"));
  fConsole.println(F("Activating Digital Outputs"));
  fConsole.println(F("Init i2c Bus"));

  Wire.begin();
  Wire.setClock(100000L);
#if defined(WIRE_HAS_TIMEOUT)
  Wire.setWireTimeout(3000, true);
#endif

  if (params.autocorrect)
    fConsole.println(F("Auto Correct Gestures Enabled"));

#ifdef SERIAL
  SERIAL.begin(params.serialbaud);
  sendSerialString(params.serialinit);
  fAudio.init(this);
#endif

  remote[0]->addr = params.xbr;
  remote[1]->addr = params.xbl;
  remote[0]->type = remote[0]->kFailsafe;
  remote[1]->type = remote[1]->kFailsafe;

  pinMode(STATUS_J1_PIN, OUTPUT);
  pinMode(STATUS_J2_PIN, OUTPUT);
  pinMode(STATUS_RC_PIN, OUTPUT);
  pinMode(STATUS_S1_PIN, OUTPUT);
  pinMode(STATUS_S2_PIN, OUTPUT);
  pinMode(STATUS_S3_PIN, OUTPUT);
  pinMode(STATUS_S4_PIN, OUTPUT);

  pinMode(DOUT1_PIN, OUTPUT);
  pinMode(DOUT2_PIN, OUTPUT);
  pinMode(DOUT3_PIN, OUTPUT);
  pinMode(DOUT4_PIN, OUTPUT);
  pinMode(DOUT5_PIN, OUTPUT);
  pinMode(DOUT6_PIN, OUTPUT);
  pinMode(DOUT7_PIN, OUTPUT);
  pinMode(DOUT8_PIN, OUTPUT);

  pinMode(ANALOG2_PIN, INPUT);

  pinMode(PPMIN_PIN, INPUT);

  pinMode(SEL2_PIN, INPUT_PULLUP);
  pinMode(RCSEL_PIN, INPUT_PULLUP);

  setDigitalPin(1, false);
  setDigitalPin(2, false);
  setDigitalPin(3, false);
  setDigitalPin(4, false);
  setDigitalPin(5, false);
  setDigitalPin(6, false);
  setDigitalPin(7, false);
  setDigitalPin(8, false);

  fXBee.setSerial(XBEE_SERIAL);

  fTankDrive.setMaxSpeed(MAXIMUM_SPEED);
  fTankDrive.setThrottleAccelerationScale(ACCELERATION_SCALE);
  fTankDrive.setThrottleDecelerationScale(DECELRATION_SCALE);
  fTankDrive.setTurnAccelerationScale(ACCELERATION_SCALE * 2);
  fTankDrive.setTurnDecelerationScale(DECELRATION_SCALE);
  fTankDrive.setGuestSpeedModifier(MAXIMUM_GUEST_SPEED);
  fTankDrive.setScaling(SCALING);
  fTankDrive.setChannelMixing(CHANNEL_MIXING);

#ifdef DOME_DRIVE
  fDomeDrive.setMaxSpeed(DOME_MAXIMUM_SPEED);
  fDomeDrive.setInverted(DEFAULT_DOME_INVERTED);
  fDomeDrive.setScaling(false);
  fDomeDrive.setThrottleAccelerationScale(DEFAULT_DOME_ACCELERATION_SCALE);
  fDomeDrive.setThrottleDecelerationScale(DEFAULT_DOME_DECELERATION_SCALE);
  // fDomeDrive.setDomePosition(&fAutoDome);
#endif

  for (unsigned i = 0; i < params.getServoCount(); i++) {
    uint16_t minpulse =
        params.S[i].minpulse ? params.S[i].minpulse : params.minpulse;
    uint16_t maxpulse =
        params.S[i].maxpulse ? params.S[i].maxpulse : params.maxpulse;

    float neutral_percent = float(params.S[i].n) / 180.0f;
    bool reversed = params.S[i].r;

    if (i == 3) {
      if (!params.domeflip) {
        reversed = !reversed;
      }
    } else if (reversed) {
      uint16_t temp = maxpulse;
      maxpulse = minpulse;
      minpulse = temp;
    }

    uint16_t neutralpulse =
        (uint16_t)((int32_t)minpulse +
                   (int32_t)(neutral_percent *
                             ((int32_t)maxpulse - (int32_t)minpulse)));

    servoDispatch.setServo(i, SERVO1_PIN + i, minpulse, maxpulse,
                           neutralpulse, 0);
  }
}

void AmidalaController::animate() {
  fConsole.process();
#ifdef EXPERIMENTAL_JEVOIS_STEERING
  fJevois.process();
#endif
#ifdef RDH_SERIAL
  fAutoDome.process();
#endif
  fAudio.process();

  // TODO THIS IS HARDCODED FOR PWM!!!
  if (servoDispatch.currentPos(0) != 1450 ||
      servoDispatch.currentPos(1) != 1500) {
    // digital out 7
    if (fDriveStateMillis + 1000 < millis()) {
      setDigitalPin(7, true);
      fDriveStateMillis = millis();
    }
  }
  // TODO THIS IS HARDCODED FOR PWM!!!
  else if (getDigitalPin(7) && fDriveStateMillis + 1000 < millis()) {
    setDigitalPin(7, false);
    fDriveStateMillis = millis();
  }

  // TODO THIS IS HARDCODED FOR PWM!!!
  if (fDomeDrive.isMoving() && fDomeDrive.idle()) {
    if (getDomeThrottle() < 0.1 && fDomeStateMillis + 1000 < millis()) {
      // digital out 8
      DEBUG_PRINTLN("DOME ACTIVE");
      setDigitalPin(8, true);
      fDomeStateMillis = millis();
    }
  }
  // TODO THIS IS HARDCODED FOR PWM!!!
  else if (getDigitalPin(8)) {
    DEBUG_PRINTLN("DOME INACTIVE");
    setDigitalPin(8, false);
    fDomeStateMillis = millis();
  }

  if (checkRCMode() && remote[0]->failsafe() && remote[0]->failsafeNotice &&
      remote[1]->failsafe() && remote[1]->failsafeNotice) {
    // Both Pocket Remotes are disabled enable RC controller
    fPPMDecoder.init();
    remote[0]->type = remote[0]->kRC;
    remote[1]->type = remote[1]->kRC;
  }
  if (checkRCMode() && remote[0]->type == remote[0]->kRC &&
      !XBEE_SERIAL.available()) {
    if (fPPMDecoder.decode()) {
      remote[0]->x = fPPMDecoder.channel(0, 0, 1024, 512);
      remote[0]->y = fPPMDecoder.channel(1, 0, 1024, 512);
      remote[0]->w1 = fPPMDecoder.channel(4, 0, 1024, 0);
      remote[0]->update();

      remote[1]->x = fPPMDecoder.channel(2, 0, 1024, 512);
      remote[1]->y = fPPMDecoder.channel(3, 0, 1024, 512);
      remote[1]->w1 = fPPMDecoder.channel(5, 0, 1024, 0);
      remote[1]->update();
    }
  } else {
    fXBee.readPacket();
    if (fXBee.getResponse().isAvailable()) {
      if (fXBee.getResponse().getApiId() == ZB_IO_SAMPLE_RESPONSE) {
        fXBee.getResponse().getZBRxIoSampleResponse(fResponse);
        if (fResponse.containsAnalog() && fResponse.containsDigital()) {
          uint32_t addr = fResponse.getRemoteAddress64().getLsb();
          for (unsigned i = 0; i < sizeof(remote) / sizeof(remote[0]); i++) {
            auto r = remote[i];
            if (addr == r->addr) {
              r->y = fResponse.getAnalog(0);
              r->x = fResponse.getAnalog(1);
              r->w1 = fResponse.getAnalog(2);
              r->w2 = fResponse.getAnalog(3);

              bool *b = r->button;
              b[0] = !fResponse.isDigitalOn(5);
              b[1] = !fResponse.isDigitalOn(6);
              b[2] = !fResponse.isDigitalOn(10);
              b[3] = !fResponse.isDigitalOn(11);
              b[4] = !fResponse.isDigitalOn(4);
              r->lastPacket = millis();
              if (r->type != r->kXBee)
                r->type = r->kXBee;
              break;
            }
          }
        }
      } else {
        DEBUG_PRINTLN();
        DEBUG_PRINT("Expected I/O Sample, but got ");
        DEBUG_PRINT_HEX(fXBee.getResponse().getApiId());
        DEBUG_PRINTLN();
      }
    } else if (fXBee.getResponse().isError()) {
#ifdef USE_POCKET_REMOTE_DEBUG
      DEBUG_PRINTLN();
      DEBUG_PRINT("Error reading packet.  Error code: ");
      DEBUG_PRINTLN(fXBee.getResponse().getErrorCode());
#endif
      for (unsigned i = 0; i < sizeof(remote) / sizeof(remote[0]); i++)
        remote[i]->lastPacket = 0;
    }
    bool stickActive = false;
    for (unsigned i = 0; i < sizeof(remote) / sizeof(remote[0]); i++) {
      auto r = remote[i];
      if (r->type == r->kXBee) {
        stickActive = true;
#ifdef USE_POCKET_REMOTE_DEBUG
        DEBUG_PRINT('J');
        DEBUG_PRINT(i + 1);
        DEBUG_PRINT('[');
        DEBUG_PRINT(r->y);
        DEBUG_PRINT(',');
        DEBUG_PRINT(r->x);
        DEBUG_PRINT(']');
        for (unsigned b = 0; b < sizeof(r->button) / sizeof(r->button[0]);
             b++) {
          DEBUG_PRINT(" B");
          DEBUG_PRINT(i * 5 + b + 1);
          DEBUG_PRINT('=');
          DEBUG_PRINT(r->button[b]);
        }
        DEBUG_PRINT(" W[");
        DEBUG_PRINT(r->w1);
        DEBUG_PRINT(",");
        DEBUG_PRINT(r->w2);
        DEBUG_PRINT("] ");
#endif
        if (r->lastPacket + params.fst < millis())
          r->type = r->kFailsafe;
        r->update();
      }
    }
    if (stickActive) {
#ifdef USE_POCKET_REMOTE_DEBUG
      DEBUG_PRINT('\r');
#endif
    }
    for (unsigned i = 0; i < sizeof(remote) / sizeof(remote[0]); i++) {
      auto r = remote[i];
      if (r->failsafe() != r->failsafeNotice) {
        if (stickActive)
          fConsole.println();
        fConsole.print('J');
        fConsole.print(i + 1);
        fConsole.print(F(" FS "));
        fConsole.println(r->failsafe() ? F("ON") : F("OFF"));
        if (i == 0)
          digitalWrite(STATUS_J1_PIN, r->failsafe() ? LOW : HIGH);
        else if (i == 1)
          digitalWrite(STATUS_J2_PIN, r->failsafe() ? LOW : HIGH);
        r->failsafeNotice = r->failsafe();
      }
    }
  }
}
