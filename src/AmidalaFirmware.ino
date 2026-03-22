///////////////////////////////////////////////
//
///////////////////////////////////////////////

#define FIRMWARE_NAME F("Amidala RC")
#define VERSION_NUM F("1.3")
#define BUILD_NUM F("1")
#define BUILD_DATE F(__DATE__)

#include "drive_config.h"

#define DEFAULT_BAUD_RATE 115200 /*57600*/

// #define RDH_SERIAL           Serial3
#define RDH_BAUD_RATE 9600

#ifndef MARCDUINO_BAUD_RATE
#define MARCDUINO_BAUD_RATE 9600
#endif

#ifndef MAX_GESTURE_LENGTH
#define MAX_GESTURE_LENGTH 8
#endif

#ifndef GESTURE_TIMEOUT_MS
#define GESTURE_TIMEOUT_MS 2000
#endif

#ifndef LONG_PRESS_TIME
#define LONG_PRESS_TIME 3000
#endif

////////////////////////////////

#define USE_DEBUG
// #define USE_DOME_SENSOR_SERIAL_DEBUG
// #define USE_POCKET_REMOTE_DEBUG
// #define USE_PPM_DEBUG
// #define USE_MOTOR_DEBUG
// #define USE_DOME_DEBUG
// #define USE_SERVO_DEBUG
// #define USE_VERBOSE_SERVO_DEBUG

////////////////////////////////

#include "ServoDispatchDirect.h"
#include "ServoEasing.h"
#include "amidala_controller.h"

////////////////////////////////

// Group ID is used by the ServoSequencer and some ServoDispatch functions to
// identify a group of servos.
//
//   Pin  Group ID,      Min,  Max
const ServoSettings servoSettings[] PROGMEM = {
    {SERVO1_PIN, 1000, 2000, 0},  {SERVO2_PIN, 1000, 2000, 0},
    {SERVO3_PIN, 1000, 2000, 0},  {SERVO4_PIN, 1000, 2000, 0},
    {SERVO5_PIN, 1000, 2000, 0},  {SERVO6_PIN, 1000, 2000, 0},
    {SERVO7_PIN, 1000, 2000, 0},  {SERVO8_PIN, 1000, 2000, 0},
    {SERVO9_PIN, 1000, 2000, 0},  {SERVO10_PIN, 1000, 2000, 0},
    {SERVO11_PIN, 1000, 2000, 0}, {SERVO12_PIN, 1000, 2000, 0}};

ServoDispatchDirect<SizeOfArray(servoSettings)> servoDispatch(servoSettings);

////////////////////////////////

// ServoPD class is defined in amidala_core.h

// Pan and tilt servos zero values and +/- angular range, in degrees:
#define PANZERO 90
#define PANRANGE 60
#define TILTZERO 70
#define TILTRANGE 40

// Create one servo PD controler for camera pan and another for camera tilt:
ServoPD panservo(400, 200, PANZERO, PANRANGE);
ServoPD tiltservo(300, 100, TILTZERO, TILTRANGE);

////////////////////////////////

static void hcrDelayedInit();

////////////////////////////////
// AmidalaController method bodies that reference servoDispatch or other
// globals defined above.
////////////////////////////////

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
      fTankDrive(servoDispatch, 1, 0, 4 fDriveStick),
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

#ifdef EXPERIMENTAL_JEVOIS_STEERING
  fJevois.init(this);
#endif

  fConsole.println(F("Reading Config File"));
  fConsole.setMinimal(!loadConfig());
  fConsole.showCurrentConfiguration();

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

#ifdef AUX_SERIAL
  AUX_SERIAL.begin(params.auxbaud);
  sendAuxString(params.auxinit);
#ifndef VMUSIC_SERIAL
  if (params.audiohw == AUDIO_HW_HCR) {
    fHCR.begin();
    DelayCall::schedule(hcrDelayedInit, 5000);
  }
#endif
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
#ifndef VMUSIC_SERIAL
  if (params.audiohw == AUDIO_HW_HCR) {
    // fHCR.update();
  }
#else
  if (params.audiohw == AUDIO_HW_VMUSIC) {
    fVMusic.process();
  }
#endif

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

      // DEBUG_PRINT("OUT ");
      // DEBUG_PRINT(remote[0]->x);
      // DEBUG_PRINT(' ');
      // DEBUG_PRINT(remote[0]->y);
      // DEBUG_PRINTLN();

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

              // for (int i = 0; i < 12; i++)
              // {
              //     if (!fResponse.isDigitalOn(i))
              //     {
              //         CONSOLE_SERIAL.print("BUTTON ");
              //         CONSOLE_SERIAL.print(i);
              //         CONSOLE_SERIAL.print(": ");
              //         CONSOLE_SERIAL.println(!fResponse.isDigitalOn(i));
              //     }
              // }
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
      // Serial.print("R"); Serial.print(i); Serial.print(" F");
      // Serial.print(r->failsafe()); Serial.print(" N");
      // Serial.print(r->failsafeNotice);
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

////////////////////////////////

AmidalaController amidala;

#ifndef VMUSIC_SERIAL
static void hcrDelayedInit() {
  AmidalaParameters &params = amidala.params;
  amidala.fHCR.SetVolume(CH_V, params.volume);
  amidala.fHCR.SetVolume(CH_A, params.volume);
  amidala.fHCR.SetVolume(CH_B, params.volume);
  if (params.rndon) {
    amidala.fHCR.Muse(params.mindelay, params.maxdelay);
    amidala.fHCR.SetMuse(1);
  }
  if (params.startup) {
    amidala.fHCR.Trigger(params.startupem, params.startuplvl);
  }
}
#endif  // !VMUSIC_SERIAL

//////////////////////////////////////////////////////////////////////////

size_t AmidalaConsole::write(uint8_t ch) {
  return write(&ch, 1);
}

size_t AmidalaConsole::write(const uint8_t *buffer, size_t size) {
  if (fPrompt) {
    CONSOLE_SERIAL.println();
    fPrompt = false;
  }
  return CONSOLE_SERIAL.write(buffer, size);
}

void AmidalaConsole::printServoPos(uint16_t num) {
  if (servoDispatch.isActive(num)) {
    printNum(servoDispatch.currentPos(num));
  } else {
    print("----");
  }
}

//////////////////////////////////////////////////////////////////////////

void AmidalaConsole::init(AmidalaController *controller) {
  fController = controller;
  print(FIRMWARE_NAME);
  print(F(" - "));
  print(VERSION_NUM);
  print(F(" Build "));
  print(BUILD_NUM);
  print(F(" ("));
  print(BUILD_DATE);
  print(F(") Serial: "));
  println(fController->params.serial);
}

void AmidalaConsole::randomToggle() {
  AmidalaParameters &params = fController->params;
  params.rndon = !params.rndon;
  print(F("Random "));
  println((params.rndon) ? F("On") : F("Off"));
#ifndef VMUSIC_SERIAL
  if (params.audiohw == AUDIO_HW_HCR) {
    fController->fHCR.SetMuse(params.rndon ? 1 : 0);
  }
#endif
}

void AmidalaConsole::setVolumeNoResponse(uint8_t volume) {
  AmidalaParameters &params = fController->params;
#ifndef VMUSIC_SERIAL
  if (params.audiohw == AUDIO_HW_HCR) {
    fController->fHCR.SetVolume(CH_V, volume);
    fController->fHCR.SetVolume(CH_A, volume);
    fController->fHCR.SetVolume(CH_B, volume);
  }
#else
  if (params.audiohw == AUDIO_HW_VMUSIC) {
    fController->fVMusic.setVolumeNoResponse(volume);
  }
#endif
}

void AmidalaConsole::playSound(int sndbank, int snd) {
  AmidalaParameters &params = fController->params;
#ifdef VMUSIC_SERIAL
  if (params.audiohw == AUDIO_HW_VMUSIC) {
    if (fController->fVMusic.isPlaying()) {
      println(F("Busy"));
      return;
    }
    AmidalaParameters::SoundBank *sb = params.SB;
    if (sndbank >= 1 && sndbank <= params.sbcount) {
      sb += (sndbank - 1);
      if (snd == 0) {
        if (sb->random) {
          snd = random(0, sb->numfiles);
        } else if (sb->numfiles > 0) {
          snd = sb->playindex++;
          if (sb->playindex >= sb->numfiles)
            sb->playindex = 0;
        } else {
          snd = -1;
        }
      }
      if (snd >= 0 && snd <= sb->numfiles) {
        char fname[16];
        snprintf(fname, sizeof(fname), "%s-%d.MP3", sb->dir, snd + 1);
        DEBUG_PRINT("PLAY: ");
        DEBUG_PRINTLN(fname);
        fController->fVMusic.play(fname, sb->dir);
      }
    }
    return;
  }
#endif
  println(F("Invalid"));
}

void AmidalaConsole::setServo() {
  // Not supported
  println(F("Invalid"));
}

void AmidalaConsole::setDigitalOut() {
  // Not supported
  println(F("Invalid"));
}

void AmidalaConsole::outputString() {
  // Not supported
  println(F("Aux Out"));
}

void AmidalaConsole::showXBEE() {
  AmidalaParameters &params = fController->params;
  print(F("J1:"));
  print(params.xbr, HEX);
  print(F(" J2:"));
  println(params.xbl, HEX);
}

void AmidalaConsole::printVersion() {
  print(F("Amidala RC - "));
  print(VERSION_NUM);
  print(F(" Build "));
  print(BUILD_NUM);
  print(F(" ("));
  print(BUILD_DATE);
  print(F(") Serial: "));
  println(fController->params.serial);
}

void AmidalaConsole::printHelp() {
  printVersion();
  println();
  println(F("h - This message"));
  println(F("v - Firmware version"));
  println(F("d - Current configuration"));
  println(F("r - Random sounds on/off"));
  println(F("$ - Play sound, ${nn}[{mm}]. nn=bank, mm= file number"));
  println(F("s - Set Servo. s{nn},{mmm},{ooo}. nn=#, mmm=pos, ooo=spd"));
  println(F("o - Set DOUT. o{nn},{m}. nn=#, m=1 or 0, 2=toggle, 3=mom"));
  println(F("a - Output string to Aux serial, a[string]"));
  println(F("i - i2c Command, i{nnn},{mmm}. nnn=Dest Addr, mmm=Cmd"));
  println(F("x - Display/Set XBEE address,  x{[n]=[hhhhhhhh]}. n=1 or 2, "
            "hhhhhhhh=Hex address"));
  println(F("w - Write config to EEPROM"));
  println(F("l - Load config from EEPROM"));
  println(F("c - Show config in EEPROM"));
  println(F("m - Servo Monitor Toggle on/off"));
  println();
}

void AmidalaConsole::process(ButtonAction &button) {
  AmidalaParameters &params = fController->params;
  switch (button.action) {
  case button.kSound:
    Serial.print("PLAY SOUND BANK ");
    Serial.print(button.sound.soundbank);
    Serial.print(" # ");
    Serial.println(button.sound.sound);
    if (button.sound.sound != 0) {
      playSound(button.sound.soundbank, button.sound.sound);
    } else {
      playSound(button.sound.soundbank);
    }
    break;
  case button.kI2CCmd:
    DEBUG_PRINT("I2C CMD addr=");
    DEBUG_PRINT(button.i2ccmd.target);
    DEBUG_PRINT(" cmd=");
    DEBUG_PRINTLN(button.i2ccmd.cmd);
    sendI2CCmd(button.i2ccmd.target, button.i2ccmd.cmd);
    break;
  case button.kI2CStr:
    if (button.i2cstr.cmd != 0 &&
        button.i2cstr.cmd <= params.getAuxStringCount()) {
      const char *str = params.A[button.i2cstr.cmd - 1].str;
      DEBUG_PRINT("I2C STR addr=");
      DEBUG_PRINT(button.i2cstr.target);
      DEBUG_PRINT(" str=");
      DEBUG_PRINTLN(str);
      sendI2CStr(button.i2cstr.target, str);
    }
    break;
  case button.kHCREmote:
#ifndef VMUSIC_SERIAL
    if (params.audiohw == AUDIO_HW_HCR) {
      if (button.emote.emotion == OVERLOAD) {
        fController->fHCR.Overload();
      } else {
        fController->fHCR.Trigger(button.emote.emotion, button.emote.level);
      }
    }
#endif
    break;
  case button.kHCRMuse:
#ifndef VMUSIC_SERIAL
    if (params.audiohw == AUDIO_HW_HCR) {
      fController->fHCR.SetMuse(1 - fController->fHCR.GetMuse());
    }
#endif
    break;
  }
  // Play ack emote for non-audio actions when ackon is enabled
#ifndef VMUSIC_SERIAL
  if (params.ackon && params.audiohw == AUDIO_HW_HCR &&
      button.action != ButtonAction::kNone &&
      button.action != ButtonAction::kHCREmote &&
      button.action != ButtonAction::kHCRMuse) {
    fController->fHCR.Trigger(params.ackem, params.acklvl);
  }
#endif
  if (button.aux.auxstring != 0 &&
      button.aux.auxstring <= params.getAuxStringCount()) {
    DEBUG_PRINT("AUX #");
    DEBUG_PRINTLN(params.A[button.aux.auxstring - 1].str);
    fController->sendAuxString(params.A[button.aux.auxstring - 1].str);
  }
}

void AmidalaConsole::processGesture(const char *gesture) {
  AmidalaParameters &params = fController->params;
  print("GESTURE: ");
  println(gesture);
  for (unsigned i = 0; i < params.getGestureCount(); i++) {
    if (params.G[i].gesture.matches(gesture)) {
      process(params.G[i].action);
    }
  }
}

void AmidalaConsole::processButton(unsigned num) {
  print("Processing Button ");
  println(num);
  AmidalaParameters &params = fController->params;

  if (num < params.getButtonCount()) {
    process(params.B[num - 1]);
  }
}

void AmidalaConsole::processLongButton(unsigned num) {
  print("Processing Long Button ");
  println(num);
  AmidalaParameters &params = fController->params;

  if (num < params.getButtonCount()) {
    process(params.LB[num - 1]);
  }
}

void AmidalaConsole::showLoadEEPROM(bool load) {
  AmidalaParameters &params = fController->params;
  if (load) {
    println(F("Loading config from EEPROM"));
    fController->params.init(true);
  } else {
    println(F("EEPROM content"));
  }
  print(F("J1="));
  println(params.xbr, HEX);
  print(F("J2="));
  println(params.xbl, HEX);
  print(F("J1VREF:"));
  print(params.rvrmin);
  print('-');
  print(params.rvrmax);
  print(F(" J2VREF:"));
  print(params.rvlmin);
  print('-');
  println(params.rvlmax);
  for (unsigned i = 0; i < params.getServoCount(); i++) {
    print('S');
    print(i + 1);
    print(F(": min="));
    print(params.S[i].min);
    print(F(", max="));
    print(params.S[i].max);
    print(F(", n="));
    print(params.S[i].n);
    print(F(", d="));
    print(params.S[i].d);
    print(F(", t="));
    print(params.S[i].t);
    print(F(", s="));
    print(params.S[i].s);
    print(F(", minpulse="));
    print(params.S[i].minpulse);
    print(F(", maxpulse="));
    print(params.S[i].maxpulse);
    if (params.S[i].r)
      print(F(" rev"));
    println();
  }
  print(F("Failsafe:"));
  println(params.fst);
}

void AmidalaConsole::showCurrentConfiguration() {
  char gesture[MAX_GESTURE_LENGTH + 1];
  AmidalaParameters &params = fController->params;
  if (fMinimal)
    println(F("Config (Minimal EEPROM Mode)"));
  else
    println(F("Config"));
  println();
  print(F("J1:"));
  print(params.xbr, HEX);
  print(F(" J2:"));
  println(params.xbl, HEX);
  print(F("J1VREF:"));
  print(params.rvrmin);
  print('-');
  print(params.rvrmax);
  print(F(" J2VREF:"));
  print(params.rvlmin);
  print('-');
  println(params.rvlmax);
  print(F("J1 V/H Adjust:")); // NOT SUPPORTED
  print(45);
  print(',');
  println(45);
  if (!fMinimal) {
    AmidalaParameters::SoundBank *sb = &params.SB[0];

    print(F("Vol: "));
    println(params.volume);
    print(F("Rnd Sound: "));
    println(params.rndon ? F("On") : F("Off"));
    print(F("Random Sound Delay: "));
    print(params.mindelay);
    print(F(" - "));
    println(params.maxdelay);
    print(F("Ack Sound: "));
    println(params.ackon ? F("On") : F("Off"));
    print(F("Ack Types: "));
    println("AutoDome,Disable,Slow-Mode");
    println();
    print(F("Sound Banks: "));
    print(params.sbcount);
    print(F(" ("));
    print(params.getSoundBankCount());
    println(F(" Max)"));
    for (unsigned i = 0; i < params.sbcount; i++, sb++) {
      print(i + 1);
      print(F(": "));
      print(sb->dir);
      print(F(" ("));
      print(sb->numfiles);
      print(F(")"));
      if (sb->numfiles > 1 && sb->random)
        print(F(" (rand)"));
      println();
    }
    println();
    print(F("My i2c Address: "));
    println(params.myi2c);
    print(F("Configured i2c devices: "));
    print(1);
    println(F(" (99)"));
    println();
    println(F("Buttons:"));
    for (unsigned i = 0; i < params.getButtonCount() - 1; i++) {
      if (params.B[i].action != 0) {
        print(i + 1);
        print(F(": "));
        params.B[i].printDescription(this);
      }
    }
    println();
    println(F("Long Buttons:"));
    for (unsigned i = 0; i < params.getButtonCount() - 1; i++) {
      if (params.LB[i].action != 0) {
        print(i + 1);
        print(F(": "));
        params.LB[i].printDescription(this);
      }
    }
    switch (params.b9) {
    case 'y':
    case 'b':
      print(9);
      print(F(": "));
      params.B[8].printDescription(this);
      break;
    case 'n':
    case 'k':
      println(F("Enable/Disable Remotes"));
      break;
    case 's':
      println(F("Slow/Fast Mode"));
      break;
    case 'd':
      println(F("Enable/Disable Drives"));
      break;
    }
    println();
    println(F("Gestures:"));
    if (!params.slowgest.isEmpty()) {
      print(params.slowgest.getGestureString(gesture));
      println(F(" Go Slow On/Off"));
    }
    if (!params.ackgest.isEmpty()) {
      print(params.ackgest.getGestureString(gesture));
      println(F(" Ack On/Off"));
    }
    if (!params.rnd.isEmpty()) {
      print(params.rnd.getGestureString(gesture));
      println(F(" Rnd Sound On/Off"));
    }
    for (unsigned i = 0; i < params.getGestureCount(); i++) {
      params.G[i].printDescription(this);
    }

    println();
    if (params.autocorrect)
      println(F("Auto Correct Gestures Enabled"));

    print(F("Aux Serial Baud: "));
    println(params.auxbaud);
    print(F("Aux Delimiter: "));
    println((char)params.auxdelim);
    print(F("Aux EOL: "));
    println(params.auxeol);
    print(F("Aux Init: "));
    println();
    println(F("Aux Serial String Commands:"));
    for (unsigned i = 0; i < params.getAuxStringCount(); i++) {
      println(params.A[i].str);
    }
    println();
    print(F("domemode: "));
    println(params.domemode);
    print(F("domehome: "));
    println(params.domehome);
    print(F("domeflip: "));
    println(params.domeflip ? F("true") : F("false"));
    print(F("domegest: "));
    println(params.domegest.getGestureString(gesture));
    print(F("domehomemin: "));
    println(params.domehomemin);
    print(F("domehomemax: "));
    println(params.domehomemax);
    print(F("domeseekmin: "));
    println(params.domeseekmin);
    print(F("domeseekmax: "));
    println(params.domeseekmax);
    print(F("domefudge: "));
    println(params.domefudge);
    print(F("domeseekl: "));
    println(params.domeseekl);
    print(F("domeseekr: "));
    println(params.domeseekr);
    print(F("domespeed: "));
    println(params.domespeed);
    print(F("domespeedhome: "));
    println(params.domespeedhome);
    print(F("domespeedseek: "));
    println(params.domespeedseek);
    print(F("domech6: "));
    println(params.domech6 ? F("true") : F("false"));
    println();
  }
  println();
  println(F("Channel/Servos:"));
  for (unsigned i = 0; i < params.getServoCount(); i++) {
    print('S');
    print(i + 1);
    print(F(": min="));
    print(params.S[i].min);
    print(F(", max="));
    print(params.S[i].max);
    print(F(", n="));
    print(params.S[i].n);
    print(F(", d="));
    print(params.S[i].d);
    print(F(", t="));
    print(params.S[i].t);
    print(F(", s="));
    print(params.S[i].s);
    print(F(", minpulse="));
    print(params.S[i].minpulse);
    print(F(", maxpulse="));
    print(params.S[i].maxpulse);
    if (params.S[i].r)
      print(F(" rev"));
    println();
  }
  print(F("Failsafe:"));
  println(params.fst);
  // Not supported
  println(F("Remote Console Cmd On"));
}

void AmidalaConsole::writeCurrentConfiguration() {
  AmidalaParameters &params = fController->params;
  size_t offs = 0xea;
  for (unsigned i = 0; i < sizeof(params.S) / sizeof(params.S[0]); i++) {
    /*EEPROM.put(offs, S[i].min);*/ offs += sizeof(params.S[i].min);
    /*EEPROM.put(offs, S[i].max);*/ offs += sizeof(params.S[i].max);
    /*EEPROM.put(offs, S[i].n);*/ offs += sizeof(params.S[i].n);
    /*EEPROM.put(offs, S[i].d);*/ offs += sizeof(params.S[i].d);
    /*EEPROM.put(offs, S[i].t);*/ offs += sizeof(params.S[i].t);
    /*EEPROM.put(offs, S[i].r);*/ offs += sizeof(params.S[i].r);
    // S[i].s = (uint8_t)ceil((float)EEPROM.read(offs++) / 255.0f * 10) * 10;
    offs += sizeof(params.S[i].s);
    EEPROM.put(offs, params.S[i].minpulse);
    offs += sizeof(params.S[i].minpulse);
    EEPROM.put(offs, params.S[i].maxpulse);
    offs += sizeof(params.S[i].maxpulse);
  }
  println(F("Updated"));
}

void AmidalaConsole::monitorOutput() {
  AmidalaParameters &params = fController->params;
  if (!fMonitor)
    return;
  // Servos  1 2 3 4
  print(F("\e[3;5H"));
  printServoPos(0);
  print(F("\e[3;14H"));
  printServoPos(1);
  print(F("\e[3;23H"));
  printServoPos(2);
  print(F("\e[3;32H"));
  printServoPos(3);
  // Servos  5 6 7 8
  print(F("\e[4;5H"));
  printServoPos(4);
  print(F("\e[4;14H"));
  printServoPos(5);
  print(F("\e[4;23H"));
  printServoPos(6);
  print(F("\e[4;32H"));
  printServoPos(7);
  // Servos  9 10 11 12
  print(F("\e[5;5H"));
  printServoPos(8);
  print(F("\e[5;14H"));
  printServoPos(9);
  print(F("\e[5;23H"));
  printServoPos(10);
  print(F("\e[5;32H"));
  printServoPos(11);

  // Digital out status
  print(F("\e[9;4H"));
  printNum(params.D[0].state);
  print(F("\e[9;10H"));
  printNum(params.D[1].state);
  print(F("\e[9;16H"));
  printNum(params.D[2].state);
  print(F("\e[9;22H"));
  printNum(params.D[3].state);
  print(F("\e[9;28H"));
  printNum(params.D[4].state);
  print(F("\e[9;34H"));
  printNum(params.D[5].state);
  print(F("\e[9;40H"));
  printNum(params.D[6].state);
  print(F("\e[9;46H"));
  printNum(params.D[7].state);

  // Volume
  print(F("\e[11;6H"));
  printNum(fController->getVolume(), 3);
  // Sound delay
  print("\e[11;24H60 to 120 secs");
  // Dome mode
  print(F("\e[11;44H"));
  printNum(fController->getDomeMode(), 1);
  // Dome position
  print(F("\e[12;6H"));
  printNum(fController->getDomePosition(), 3);
  // Dome delay
  print("\e[12;24H1 to 10 secs");
  // Dome home
  print(F("\e[12;44H"));
  printNum(fController->getDomeHome(), 3);
  // Dome imu
  print(F("\e[13;6H"));
  printNum(fController->getDomeIMU(), 1);

  print(F("\e[0m"));
  print(F("\e[16;1H"));
}

void AmidalaConsole::monitorToggle() {
  fMonitor = !fMonitor;
  print(F("\ec"));
  if (!fMonitor) {
    println("Monitor Off");
    return;
  }
  print(F("\e[?25l"));
  print(F("\e[1;63H=============="));
  print(F("\e[2;63H"));
  print(FIRMWARE_NAME);
  print(' ');
  print(VERSION_NUM);
  print(F("\e[3;63H=============="));
  print(F("\e[1;1H"));
  print(F("\e[0m"));
  print(F("\e[1mServo Output:"));
  print(F("\e[3;1H1:"));
  print(F("\e[3;10H2:"));
  print(F("\e[3;19H3:"));
  print(F("\e[3;28H4:"));
  print(F("\e[4;1H5:"));
  print(F("\e[4;10H6:"));
  print(F("\e[4;19H7:"));
  print(F("\e[4;28H8:"));
  print(F("\e[5;1H9:"));
  print(F("\e[5;10H10:"));
  print(F("\e[5;19H11:"));
  print(F("\e[5;28H12:"));
  print(F("\e[1m"));
  print(F("\e[7;1HDigital Out:"));
  print(F("\e[9;1H1:"));
  print(F("\e[9;7H2:"));
  print(F("\e[9;13H3:"));
  print(F("\e[9;19H4:"));
  print(F("\e[9;25H5:"));
  print(F("\e[9;31H6:"));
  print(F("\e[9;37H7:"));
  print(F("\e[9;43H8:"));
  print(F("\e[11;1HVol:"));
  print(F("\e[11;11HSound Delay:"));
  print(F("\e[11;39HMode:"));
  print(F("\e[12;1HDome:"));
  print(F("\e[12;11HDome Delay:"));
  print(F("\e[12;39HHome:"));
  print(F("\e[13;1HIMU:"));
}

// startswith, strtol, strtolu, boolparam, intparam, charparam, sintparam,
// sintparam2, gestureparam, and numberparams are defined in amidala_core.h
// isdigit(const char*, int) and atoi(const char*, int) are used only by
// processCommand() for servo/digital-out command parsing and remain here.

bool isdigit(const char *cmd, int numdigits) {
  for (int i = 0; i < numdigits; i++)
    if (!isdigit(cmd[i]))
      return false;
  return true;
}

int atoi(const char *cmd, int numdigits) {
  int result = 0;
  for (int i = 0; i < numdigits; i++)
    result = result * 10 + (cmd[i] - '0');
  return result;
}

bool AmidalaConsole::processConfig(const char *cmd) {
  bool boolarg;
  int32_t sintarg;
  int32_t sintarg2;
  AmidalaParameters &params = fController->params;
#ifdef RDH_SERIAL
  RDHSerial &autoDome = fController->fAutoDome;
#endif
  DomeDrive *domeDrive = &fController->fDomeDrive;
  if (startswith(cmd, "sb=")) {
    if (params.sbcount < params.getSoundBankCount()) {
      AmidalaParameters::SoundBank *sb =
          &params.SB[params.sbcount];
      char *dirname = sb->dir;
      memset(sb, '\0', sizeof(*sb));
      for (unsigned i = 0;
           *cmd != '\0' && *cmd != ',' && i < sizeof(sb->dir) - 1; i++) {
        dirname[i] = *cmd++;
        dirname[i + 1] = '\0';
      }
      if (*cmd == ',') {
        sb->numfiles = strtolu(++cmd, &cmd);
        if (*cmd == ',') {
          if (cmd[1] == 's')
            sb->random = false;
          else if (cmd[1] == 'r')
            sb->random = true;
          else
            return false;
        } else {
          sb->random = true;
        }
        if (*cmd == '\0') {
          params.sbcount++;
          return true;
        }
      }
    }
  } else if (startswith(cmd, "s=")) {
    uint8_t argcount;
    int args[10];
    memset(args, '\0', sizeof(args));
    AmidalaParameters::Channel *s = params.S;
    if (numberparams(cmd, argcount, args, sizeof(args)) && argcount >= 3 &&
        args[0] >= 1 && args[0] <= int(params.getServoCount())) {
      unsigned num = args[0] - 1;
      s += num;
      s->min = min(max(args[1], 0), 180);
      s->max = max(min(args[2], 180), 0);
      s->n = (argcount >= 4) ? max(min(args[3], 180), 0)
                             : (s->min + (s->max - s->min) / 2);
      s->d = (argcount >= 5) ? max(min(args[4], 180), 0) : 0;
      s->t = (argcount >= 6) ? args[5] : 0;
      s->s = (argcount >= 7) ? max(min(args[6], 100), 0) : 100;
      s->r = (argcount >= 8) ? max(min(args[7], 1), 0) : 0;
      s->minpulse =
          (argcount >= 9) ? min(max(args[8], 800), 2400) : params.minpulse;
      s->maxpulse =
          (argcount >= 10) ? min(max(args[9], 800), 2400) : params.maxpulse;

      float neutral = float(s->n) / float(s->max);
      uint16_t minpulse = s->minpulse;
      uint16_t maxpulse = s->maxpulse;
      if (s->r) {
        maxpulse = s->minpulse;
        minpulse = s->maxpulse;
      }
      servoDispatch.setServo(num, SERVO1_PIN + num, minpulse, maxpulse,
                             neutral * s->maxpulse, 0);
      return true;
    }
  } else if (startswith(cmd, "b=")) {
    uint8_t argcount;
    uint8_t args[5];
    memset(args, '\0', sizeof(args));
    ButtonAction *b = params.B;
    if (numberparams(cmd, argcount, args, sizeof(args)) && argcount >= 2 &&
        args[0] >= 1 && args[0] <= params.getButtonCount()) {
      b += args[0] - 1;
      memset(b, '\0', sizeof(*b));
      b->action = args[1];
      b->sound.auxstring = 0;
      switch (args[1]) {
      case ButtonAction::kSound:
        b->sound.soundbank = max(1, min(args[2], params.sbcount));
        b->sound.sound = (argcount >= 4) ? args[3] : 0;
        b->sound.sound =
            min(b->sound.sound, params.SB[b->sound.soundbank].numfiles);
        b->action = args[1];
        break;
      case ButtonAction::kServo:
        b->servo.num = max(1, min(args[2], 8));
        b->servo.pos = (argcount >= 4) ? args[3] : 0;
        b->servo.pos = min(max(b->servo.pos, 180), 90);
        b->action = args[1];
        break;
      case ButtonAction::kDigitalOut:
        b->dout.num = max(1, min(args[2], 8));
        b->dout.state = (argcount >= 4) ? args[3] : 0;
        b->dout.state = min(2, b->dout.state);
        b->action = args[1];
        break;
      case ButtonAction::kI2CCmd:
        b->i2ccmd.target = min(args[2], 100);
        b->i2ccmd.cmd = (argcount >= 4) ? args[3] : 0;
        b->action = args[1];
        break;
      case ButtonAction::kAuxStr:
        b->aux.auxstring = min(args[2], MAX_AUX_STRINGS);
        Serial.print("BUTTON AUX #");
        Serial.println(b->aux.auxstring);
        if (b->action == 0)
          b->action = args[1];
        break;
      case ButtonAction::kI2CStr:
        b->i2cstr.target = min(args[2], 100);
        b->i2cstr.cmd = (argcount >= 4) ? args[3] : 0;
        b->action = args[1];
        break;
      case ButtonAction::kHCREmote:
        b->emote.emotion = min(args[2], (uint8_t)4);
        b->emote.level = (argcount >= 4) ? min(args[3], (uint8_t)1) : 0;
        b->action = args[1];
        break;
      case ButtonAction::kHCRMuse:
        b->action = args[1];
        break;
      default:
        b->action = 0;
        break;
      }
      if (b->action != ButtonAction::kAuxStr &&
          b->action != ButtonAction::kHCREmote &&
          b->action != ButtonAction::kHCRMuse && argcount >= 4)
        b->sound.auxstring = (argcount >= 5) ? args[4] : 0;
      return true;
    }
    return false;
  } else if (startswith(cmd, "lb=")) {
    uint8_t argcount;
    uint8_t args[5];
    memset(args, '\0', sizeof(args));
    ButtonAction *b = params.LB;
    if (numberparams(cmd, argcount, args, sizeof(args)) && argcount >= 2 &&
        args[0] >= 1 && args[0] <= params.getButtonCount()) {
      b += args[0] - 1;
      memset(b, '\0', sizeof(*b));
      b->action = args[1];
      b->sound.auxstring = 0;
      switch (args[1]) {
      case ButtonAction::kSound:
        b->sound.soundbank = max(1, min(args[2], params.sbcount));
        b->sound.sound = (argcount >= 4) ? args[3] : 0;
        b->sound.sound =
            min(b->sound.sound, params.SB[b->sound.soundbank].numfiles);
        b->action = args[1];
        break;
      case ButtonAction::kServo:
        b->servo.num = max(1, min(args[2], 8));
        b->servo.pos = (argcount >= 4) ? args[3] : 0;
        b->servo.pos = min(max(b->servo.pos, 180), 90);
        b->action = args[1];
        break;
      case ButtonAction::kDigitalOut:
        b->dout.num = max(1, min(args[2], 8));
        b->dout.state = (argcount >= 4) ? args[3] : 0;
        b->dout.state = min(2, b->dout.state);
        b->action = args[1];
        break;
      case ButtonAction::kI2CCmd:
        b->i2ccmd.target = min(args[2], 100);
        b->i2ccmd.cmd = (argcount >= 4) ? args[3] : 0;
        b->action = args[1];
        break;
      case ButtonAction::kAuxStr:
        b->aux.auxstring = min(args[2], MAX_AUX_STRINGS);
        if (b->action == 0)
          b->action = args[1];
        break;
      case ButtonAction::kI2CStr:
        b->i2cstr.target = min(args[2], 100);
        b->i2cstr.cmd = (argcount >= 4) ? args[3] : 0;
        b->action = args[1];
        break;
      case ButtonAction::kHCREmote:
        b->emote.emotion = min(args[2], (uint8_t)4);
        b->emote.level = (argcount >= 4) ? min(args[3], (uint8_t)1) : 0;
        b->action = args[1];
        break;
      case ButtonAction::kHCRMuse:
        b->action = args[1];
        break;
      default:
        b->action = 0;
        break;
      }
      if (b->action != ButtonAction::kAuxStr &&
          b->action != ButtonAction::kHCREmote &&
          b->action != ButtonAction::kHCRMuse && argcount >= 4)
        b->sound.auxstring = args[3];
      return true;
    }
    return false;
  } else if (startswith(cmd, "a=")) {
    AuxString *a =
        &params.A[min(params.acount, params.getAuxStringCount() - 1)];
    strncpy(a->str, cmd, sizeof(a->str) - 1);
    a->str[sizeof(a->str) - 1] = '\0';
    if (params.acount < params.getAuxStringCount())
      params.acount++;
  } else if (startswith(cmd, "g=")) {
    char gesture[MAX_GESTURE_LENGTH + 1];
    char *gesture_end = &gesture[sizeof(gesture) - 1];
    char *gest = gesture;
    while (*cmd != ',' && *cmd != '\0') {
      if (gest <= gesture_end) {
        *gest++ = *cmd;
        *gest = '\0';
      }
      cmd++;
    }
    if (*cmd == ',')
      cmd++;
    GestureAction *g =
        &params.G[min(params.gcount, params.getGestureCount() - 1)];
    ButtonAction *b = &g->action;
    g->gesture.setGesture(gesture);
    if (!g->gesture.isEmpty()) {
      uint8_t argcount;
      uint8_t args[5];
      memset(args, '\0', sizeof(args));
      if (numberparams(cmd, argcount, args, sizeof(args)) && argcount >= 1) {
        memset(b, '\0', sizeof(*b));
        b->action = args[0];
        b->sound.auxstring = 0;
        switch (b->action = args[0]) {
        case ButtonAction::kSound:
          b->sound.soundbank = max(1, min(args[1], params.sbcount));
          b->sound.sound = (argcount >= 3) ? args[2] : 0;
          b->sound.sound =
              min(b->sound.sound, params.SB[b->sound.soundbank].numfiles);
          break;
        case ButtonAction::kServo:
          b->servo.num = max(1, min(args[1], 8));
          b->servo.pos = (argcount >= 3) ? args[2] : 0;
          b->servo.pos = min(max(b->servo.pos, 180), 90);
          break;
        case ButtonAction::kDigitalOut:
          b->dout.num = max(1, min(args[1], 8));
          b->dout.state = (argcount >= 3) ? args[2] : 0;
          b->dout.state = min(2, b->dout.state);
          break;
        case ButtonAction::kI2CCmd:
          b->i2ccmd.target = min(args[1], 100);
          b->i2ccmd.cmd = (argcount >= 3) ? args[2] : 0;
          break;
        case ButtonAction::kAuxStr:
          b->aux.auxstring = min(args[1], MAX_AUX_STRINGS);
          Serial.print("GESTURE AUX #");
          Serial.println(b->aux.auxstring);
          break;
        case ButtonAction::kI2CStr:
          b->i2cstr.target = min(args[1], 100);
          b->i2cstr.cmd = (argcount >= 3) ? args[2] : 0;
          break;
        case ButtonAction::kHCREmote:
          b->emote.emotion = min(args[1], (uint8_t)4);
          b->emote.level = (argcount >= 3) ? min(args[2], (uint8_t)1) : 0;
          break;
        case ButtonAction::kHCRMuse:
          break;
        default:
          b->action = 0;
          break;
        }
        if (b->action != ButtonAction::kAuxStr &&
            b->action != ButtonAction::kHCREmote &&
            b->action != ButtonAction::kHCRMuse && argcount >= 3)
          b->sound.auxstring = args[2];
        if (params.gcount < params.getGestureCount())
          params.gcount++;
        return true;
      }
    }
    return false;
  } else if (startswith(cmd, "audiohw=")) {
    if (startswith(cmd, "hcr"))    params.audiohw = AUDIO_HW_HCR;
    else if (startswith(cmd, "vmusic")) params.audiohw = AUDIO_HW_VMUSIC;
    return true;
  } else if (charparam(cmd, "acktype=", "gadsr", params.acktype) ||
             charparam(cmd, "b9=", "ynksdb", params.b9) ||
             intparam(cmd, "volume=", params.volume, 0, 100) ||
             intparam(cmd, "startupem=", params.startupem, 0, 4) ||
             intparam(cmd, "startuplvl=", params.startuplvl, 0, 1) ||
             intparam(cmd, "ackem=", params.ackem, 0, 4) ||
             intparam(cmd, "acklvl=", params.acklvl, 0, 1) ||
             intparam(cmd, "mindelay=", params.mindelay, 0, 1000) ||
             intparam(cmd, "maxdelay=", params.maxdelay, 0, 1000) ||
             intparam(cmd, "rvrmin=", params.rvrmin, 0, 100) ||
             intparam(cmd, "rvrmax=", params.rvrmin, 900, 1023) ||
             intparam(cmd, "rvlmin=", params.rvrmin, 0, 100) ||
             intparam(cmd, "rvlmax=", params.rvrmin, 900, 1023) ||
             intparam(cmd, "minpulse=", params.minpulse, 0, 2500) ||
             intparam(cmd, "maxpulse=", params.maxpulse, 0, 2500) ||
             intparam(cmd, "rcchn=", params.rcchn, 6, 8) ||
             intparam(cmd, "rcd=", params.rcd, 1, 50) ||
             intparam(cmd, "rcj=", params.rcj, 1, 40) ||
             intparam(cmd, "myi2c=", params.myi2c, 0, 100) ||
             intparam(cmd, "auxbaud=", params.auxbaud, 300, 115200) ||
             intparam(cmd, "auxdelim=", params.auxdelim, 0, 255) ||
             intparam(cmd, "auxeol=", params.auxeol, 0, 255) ||
             intparam(cmd, "fst=", params.fst, 1000, 3000) ||
             intparam(cmd, "j1adjv=", params.j1adjv, 0, 80) ||
             intparam(cmd, "j1adjh=", params.j1adjh, 0, 80) ||
             gestureparam(cmd, "rnd=", params.rnd) ||
             gestureparam(cmd, "ackgest=", params.ackgest) ||
             gestureparam(cmd, "slowgest=", params.slowgest) ||
             gestureparam(cmd, "domegest=", params.domegest) ||
             boolparam(cmd, "startup=", params.startup) ||
             boolparam(cmd, "rndon=", params.rndon) ||
             boolparam(cmd, "ackon=", params.ackon) ||
             boolparam(cmd, "mix12=", params.mix12) ||
             boolparam(cmd, "auto=", params.autocorrect) ||
             boolparam(cmd, "goslow=", params.goslow) ||
             boolparam(cmd, "domeflip=", params.domeflip) ||
             boolparam(cmd, "domech6=", params.domech6)) {
    return true;
  } else if (startswith(cmd, "xbr=")) {
    params.xbr = strtoul(cmd, NULL, 16);
    return true;
  } else if (startswith(cmd, "xbl=")) {
    params.xbl = strtoul(cmd, NULL, 16);
    return true;
  } else if (boolparam(cmd, "domeimu=", boolarg)) {
    return true;
  } else if (intparam(cmd, "domespeed=", params.domespeed, 0, 100)) {
    domeDrive->setMaxSpeed(params.domespeed);
    return true;
  } else if (sintparam(cmd, "domepos=", sintarg)) {
#ifdef RDH_SERIAL
    Serial.print("NEWPOS: ");
    Serial.println(sintarg);
    autoDome.setAbsolutePosition(sintarg);
    return true;
#else
    return false;
#endif
  } else if (sintparam2(cmd, "domepos=", sintarg, sintarg2)) {
#ifdef RDH_SERIAL
    Serial.print("NEWPOS: ");
    Serial.println(sintarg);
    autoDome.setAbsolutePosition(sintarg, sintarg2);
    return true;
#else
    return false;
#endif
  } else if (sintparam(cmd, "domerpos=", sintarg)) {
#ifdef RDH_SERIAL
    Serial.print("NEWPOS: ");
    Serial.println(sintarg);
    autoDome.setRelativePosition(sintarg);
    return true;
#else
    return false;
#endif
  } else if (sintparam2(cmd, "domerpos=", sintarg, sintarg2)) {
#ifdef RDH_SERIAL
    Serial.print("NEWPOS: ");
    Serial.println(sintarg);
    autoDome.setRelativePosition(sintarg, sintarg2);
    return true;
#else
    return false;
#endif
  } else if (intparam(cmd, "domehome=", params.domehome, 0, 360)) {
#ifdef RDH_SERIAL
    autoDome.setDomeHomePosition(params.domehome);
    return true;
#else
    return false;
#endif
  } else if (intparam(cmd, "domemode=", params.domemode, 1, 5)) {
#ifdef RDH_SERIAL
    autoDome.setDomeDefaultMode(params.domemode);
    return true;
#else
    return false;
#endif
  } else if (intparam(cmd, "domehomemin=", params.domehomemin, 1, 255)) {
    // autoDome.setDomeHomeMinDelay(params.domehomemin);
    return true;
  } else if (intparam(cmd, "domehomemax=", params.domehomemax, 1, 255)) {
    // autoDome.setDomeHomeMaxDelay(params.domehomemax);
    return true;
  } else if (intparam(cmd, "domeseekmin=", params.domehomemin, 1, 255)) {
    // autoDome.setDomeSeekMinDelay(params.domehomemin);
    return true;
  } else if (intparam(cmd, "domeseekmax=", params.domehomemax, 1, 255)) {
    // autoDome.setDomeSeekMaxDelay(params.domehomemax);
    return true;
  } else if (intparam(cmd, "domeseekr=", params.domeseekr, 1, 180)) {
    // autoDome.setDomeSeekRightDegrees(params.domeseekr);
    return true;
  } else if (intparam(cmd, "domeseekl=", params.domeseekl, 1, 180)) {
    // autoDome.setDomeSeekLeftDegrees(params.domeseekl);
    return true;
  } else if (intparam(cmd, "domefudge=", params.domefudge, 1, 20)) {
    // autoDome.setDomeFudgeFactor(params.domefudge);
    return true;
  } else if (intparam(cmd, "domespeedhome=", params.domespeedhome, 1, 100)) {
    // autoDome.setDomeHomeSpeed(params.domespeedhome);
    return true;
  } else if (intparam(cmd, "domespeedseek=", params.domespeedseek, 1, 100)) {
    // autoDome.setDomeSeekSpeed(params.domespeedhome);
    return true;
  } else if (strcmp(cmd, "reboot") == 0) {
    void (*resetArduino)() = NULL;
    resetArduino();
  }
  // else if (intparam(cmd, "domespmin=", params.domespmin, 0, 100))
  // {
  // }
  // else if (intparam(cmd, "domespmax=", params.domespmin, 900, 1023))
  // {
  // }
  return false;
}

void AmidalaConsole::processCommand(const char *cmd) {
  if (cmd[1] == '\0') {
    switch (cmd[0]) {
    case '?':
    case 'h':
      printHelp();
      return;
    case 'v':
      printVersion();
      return;
    case 'd':
      showCurrentConfiguration();
      return;
    case 'r':
      randomToggle();
      return;
    case 'x':
      showXBEE();
      return;
    case 'l':
      showLoadEEPROM(true);
      return;
    case 'c':
      showLoadEEPROM(false);
      return;
    case 'm':
      monitorToggle();
      return;
    case 'w':
      writeCurrentConfiguration();
      return;
    }
  } else if (cmd[0] == '*') {
    if (!fMonitor) {
      println("Interpreting as config command");
      println(cmd + 1);
    }
    if (processConfig(cmd + 1)) {
      if (!fMonitor)
        println("Done");
    } else {
      if (!fMonitor) {
        println("Invalid Param:");
      } else {
        print("Invalid Param: ");
        println(cmd);
      }
    }
    return;
  } else if (cmd[0] == 's' && isdigit(cmd + 1, 2) && cmd[3] == ',' &&
             isdigit(cmd + 4, 3) && cmd[7] == ',' && isdigit(cmd + 8, 3) &&
             cmd[11] == '\0') {
    // Servo Command. Set Servo Position/Speed.
    // s{nn},{mmm},{ooo}. nn=Servo # (06-12), mmm=position (000-180), ooo=speed
    // (001-100)
    int snum = atoi(cmd + 1, 2);
    int sval = atoi(cmd + 4, 3);
    int sspd = atoi(cmd + 8, 3);
    println("SERVO snum=" + String(snum) + ", " + String(sval) + ", " +
            String(sspd));
    return;
  } else if (cmd[0] == 'o' && isdigit(cmd + 1, 2) && cmd[3] == ',' &&
             isdigit(cmd + 4, 1) && cmd[5] == '\0') {
    // Digital Out Command.
    // o{nn},{m}. nn=Digital Out # (01-08), m=1 (on), m=0 (off), 2=toggle,
    // 3=momentary/blink/blip
    int dout = atoi(cmd + 1, 2);
    int dval = atoi(cmd + 4, 1);
    println("DIGITAL dout=" + String(dout) + ", " + String(dval));
    return;
  } else if (cmd[0] == 'i' && isdigit(cmd + 1, 3) && cmd[4] == ',' &&
             isdigit(cmd + 5, 3) && cmd[8] == '\0') {
    // i2c Command
    // i{nnn},{mmm}. nnn=Dest Addr, mmm=Cmd
    int i2caddr = atoi(cmd + 1, 3);
    int i2ccmd = atoi(cmd + 5, 3);
    println("I2C addr=" + String(i2caddr) + ", " + String(i2ccmd));
    sendI2CCmd(i2caddr, i2ccmd);
    return;
  } else if (startswith(cmd, "autod=") && isdigit(cmd, 1) && cmd[1] == '\0') {
    int autod = atoi(cmd, 1);
    println("AUTOD=" + String(autod));
    return;
  } else if (startswith(cmd, "tmpvol=") && isdigit(cmd, 3) && cmd[3] == ',' &&
             isdigit(cmd + 4, 2)) {
    int tmpvol = atoi(cmd, 3);
    int tmpsec = atoi(cmd + 4, 2);
    println("TMPVOL=" + String(tmpvol) + " for " + String(tmpsec));
    return;
  } else if (startswith(cmd, "tmprnd=") && isdigit(cmd, 2) && cmd[2] == '\0') {
    int tmprnd = atoi(cmd, 2);
    println("TMPRND=" + String(tmprnd));
    return;
  } else if (startswith(cmd, "dp=") && isdigit(cmd, 3) && cmd[3] == '\0') {
    // set dome position
    int dp = atoi(cmd, 3);
    println("DP=" + String(dp));
    return;
  } else if (cmd[0] == 'a') {
    print("Aux Out");
    return;
  } else if (cmd[0] == '$' && isdigit(cmd + 1, 2)) {
    // Play Sound from Sound Bank nn
    int sndbank = atoi(cmd + 1, 2);
    int snd = -1;
    if (isdigit(cmd + 3, 2) && cmd[5] == '\0') {
      // Play Sound mm from Sound Bank nn
      snd = atoi(cmd + 3, 2);
      playSound(sndbank, snd);
      return;
    } else if (cmd[3] == '\0') {
      print("SB #");
      println(sndbank);
      playSound(sndbank);
      return;
    }
  }
  println("Invalid");
}

bool AmidalaConsole::process(char ch, bool config) {
  if (ch == 0x0A || ch == 0x0D) {
    fBuffer[fPos] = '\0';
    fPos = 0;
    if (*fBuffer != '\0') {
      if (config)
        processConfig(fBuffer);
      else
        processCommand(fBuffer);
      return true;
    }
  } else if (fPos < SizeOfArray(fBuffer) - 1) {
    fBuffer[fPos++] = ch;
  }
  return false;
}

void AmidalaConsole::process() {
  monitorOutput();
  if (!fPrompt) {
    if (!fMonitor)
      print("> ");
    fPrompt = true;
  }
  if (CONSOLE_SERIAL.available()) {
    static bool reentry;
    if (reentry)
      return;
    char ch = CONSOLE_SERIAL.read();
    if (ch != '\r' && !fMonitor)
      CONSOLE_SERIAL.print(ch);
    reentry = true;
    process(ch);
    reentry = false;
  }
}

//////////////////////////////////////////////////////////////////////////

#ifdef EXPERIMENTAL_JEVOIS_STEERING
void JevoisConsole::init(AmidalaController *controller) {
  fController = controller;
  fController->setTargetSteering(&fSteering);
}

void JevoisConsole::processCommand(char *cmd) {
  int id, targx, targy, targw, targh;
  char *tok = (cmd != nullptr) ? strtok(cmd, " \r") : nullptr;
  int state = 0;

  while (tok) {
    // State machine:
    // 0: start parsing
    // 1: N2 command, parse id
    // 2: N2 command, parse targx
    // 3: N2 command, parse targy
    // 4: N2 command, parse targw
    // 5: N2 command, parse targh
    // 6: N2 command complete
    // 1000: unknown command
    switch (state) {
    case 0:
      state = (strcmp(tok, "N2") == 0) ? 1 : 1000;
      break;
    case 1:
      id = atoi(&tok[1]);
      state = 2;
      break;
    case 2:
      targx = atoi(tok);
      state = 3;
      break;
    case 3:
      targy = atoi(tok);
      state = 4;
      break;
    case 4:
      targw = atoi(tok);
      state = 5;
      break;
    case 5:
      targh = atoi(tok);
      state = 6;
      break;
    default:
      break;
    }
    tok = strtok(0, " \r\n");
  }

  // If a complete new N2 command was received, act:
  if (state == 6) {
    // panservo.update(-targx);
    tiltservo.update(targy);

    fSteering.setCurrentDistance(targw);
    // fSteering.setCurrentAngle(panservo.get() - PANZERO);
    fSteering.setCurrentAngle(targx);

    // CONSOLE_SERIAL.print("[");
    // CONSOLE_SERIAL.print(fSteering.getThrottle());
    // CONSOLE_SERIAL.print(",");
    // CONSOLE_SERIAL.print(fSteering.getTurning());
    // CONSOLE_SERIAL.print(","); CONSOLE_SERIAL.print(targx);
    // CONSOLE_SERIAL.println("]");
  } else {
    // Slow down if we lost track:
    fSteering.lost();
  }
  (void)id; (void)targh; // suppress unused-variable warnings
}

void JevoisConsole::process() {
  while (AUX_SERIAL.available()) {
    int ch = AUX_SERIAL.read();
    if (ch == '\n') {
      fBuffer[fPos] = '\0';
      fPos = 0;
      if (*fBuffer != '\0') {
        processCommand(fBuffer);
      }
    } else if (fPos < SizeOfArray(fBuffer) - 1) {
      fBuffer[fPos++] = ch;
    }
    fLastTime = millis();
  }
  if (fLastTime + 50 < millis()) {
    processCommand();
    fLastTime = millis();
  }
}
#endif

//////////////////////////////////////////////////////////////////////////

void setup() {
  REELTWO_READY();

  randomSeed(analogRead(3));

  CONSOLE_SERIAL.begin(DEFAULT_BAUD_RATE);
  XBEE_SERIAL.begin(57600);
#ifdef VMUSIC_SERIAL
  VMUSIC_SERIAL.begin(9600);
#endif
#ifdef DRIVE_SERIAL
  DRIVE_SERIAL.begin(DRIVE_BAUD_RATE);
#elif defined(DOME_DRIVE_SERIAL)
  DOME_DRIVE_SERIAL.begin(DRIVE_BAUD_RATE);
#elif defined(RDH_SERIAL)
  RDH_SERIAL.begin(RDH_BAUD_RATE);
#else
  AUX_SERIAL.begin(115200);
#endif

  SetupEvent::ready();
}

void loop() { AnimatedEvent::process(); }