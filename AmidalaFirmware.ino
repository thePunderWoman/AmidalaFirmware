///////////////////////////////////////////////
//
///////////////////////////////////////////////

#define FIRMWARE_NAME F("Amidala RC")
#define VERSION_NUM F("1.3")
#define BUILD_NUM F("1")
#define BUILD_DATE F(__DATE__)

#define DRIVE_SYSTEM_PWM 1
#define DRIVE_SYSTEM_SABER 2
#define DRIVE_SYSTEM_ROBOTEQ_PWM 3
#define DRIVE_SYSTEM_ROBOTEQ_SERIAL 4
#define DRIVE_SYSTEM_ROBOTEQ_PWM_SERIAL 5
// #define DRIVE_SYSTEM         DRIVE_SYSTEM_ROBOTEQ_PWM_SERIAL
#define DRIVE_SYSTEM DRIVE_SYSTEM_ROBOTEQ_PWM

#define DOME_DRIVE_SABER 2
#define DOME_DRIVE_PWM 3

#define DOME_DRIVE DOME_DRIVE_PWM
// #define DOME_DRIVE           DOME_DRIVE_SABER

#define DEFAULT_BAUD_RATE 115200 /*57600*/

#define CONSOLE_BUFFER_SIZE 64

// #define RDH_SERIAL           Serial3
#define RDH_BAUD_RATE 9600

////////////////////////////////

#define MAXIMUM_SPEED 1.0f       // percentage 0.0 - 1.0. default 50%
#define MAXIMUM_GUEST_SPEED 0.5f // percentage 0.0 - 1.0. default 30%
#define DOME_MAXIMUM_SPEED 1.0f  // percentage 0.0 - 1.0. default 100%

// Scale value of 1 means instant. Scale value of 100 means that the throttle
// will increase 1/100 every 25ms
#define ACCELERATION_SCALE 100
// Scale value of 1 means instant. Scale value of 20 means that the throttle
// will decrease 1/20 every 25ms
#define DECELRATION_SCALE 20

#define SCALING                                                                \
  false // set to true if acceleration/decelleration should be applied

#define DEFAULT_DOME_HOME_POSITION 50
#define DEFAULT_DOME_ACCELERATION_SCALE 20
#define DEFAULT_DOME_DECELERATION_SCALE 50
#define DEFAULT_DOME_HOME_MIN_DELAY 6
#define DEFAULT_DOME_HOME_MAX_DELAY 8
#define DEFAULT_DOME_SEEK_MIN_DELAY 6
#define DEFAULT_DOME_SEEK_MAX_DELAY 8
#define DEFAULT_DOME_TARGET_MIN_DELAY 0
#define DEFAULT_DOME_TARGET_MAX_DELAY 1
#define DEFAULT_DOME_SEEK_LEFT 80
#define DEFAULT_DOME_SEEK_RIGHT 80
#define DEFAULT_DOME_FUDGE 2
#define DEFAULT_DOME_SPEED_HOME 40
#define DEFAULT_DOME_SPEED_SEEK 60
#define DEFAULT_DOME_SPEED_TARGET 100
#define DEFAULT_DOME_SPEED_MIN 15
#define DEFAULT_DOME_TIMEOUT 5
#define DEFAULT_DOME_INVERTED false

#if DRIVE_SYSTEM == DRIVE_SYSTEM_SABER
#define DRIVE_SERIAL Serial3
#define DOME_DRIVE_SERIAL DRIVE_SERIAL
#define CHANNEL_MIXING                                                         \
  true // set to true premix channels before sending commands
#elif DRIVE_SYSTEM == DRIVE_SYSTEM_PWM
#define CHANNEL_MIXING                                                         \
  false // set to false if motor controller will mix the channels
#elif DRIVE_SYSTEM == DRIVE_SYSTEM_ROBOTEQ_PWM
#define CHANNEL_MIXING                                                         \
  false // set to false if motor controller will mix the channels
#elif DRIVE_SYSTEM == DRIVE_SYSTEM_ROBOTEQ_SERIAL
#define DRIVE_BAUD_RATE 115200
#define CHANNEL_MIXING                                                         \
  false // set to false if motor controller will mix the channels
#elif DRIVE_SYSTEM == DRIVE_SYSTEM_ROBOTEQ_PWM_SERIAL
#define DRIVE_BAUD_RATE 115200
#define CHANNEL_MIXING                                                         \
  false // set to false if motor controller will mix the channels
#else
#error Unsupported DRIVE_SYSTEM
#endif

// #define EXPERIMENTAL_JEVOIS_STEERING

#if !defined(DRIVE_BAUD_RATE) && DOME_DRIVE == DOME_DRIVE_SABER
#define DRIVE_BAUD_RATE 9600
#endif

#if !defined(DOME_DRIVE_SERIAL) && DOME_DRIVE == DOME_DRIVE_SABER
#define DOME_DRIVE_SERIAL Serial3
#endif

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

#ifndef MAX_AUX_STRINGS
#define MAX_AUX_STRINGS 40
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

#include "ReelTwo.h"
// #include "ReelTwoAudio.h"
#include "SD.h"
#include "SPI.h"
// #include "audio/VMusic.h"
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
// #include "i2c/I2CReceiver.h"

////////////////////////////////

#define SERVO1_PIN 2
#define SERVO2_PIN 3
#define SERVO3_PIN 4
#define SERVO4_PIN 5
#define SERVO5_PIN 6
#define SERVO6_PIN 7
#define SERVO7_PIN 8
#define SERVO8_PIN 9
#define SERVO9_PIN 10
#define SERVO10_PIN 11
#define SERVO11_PIN 12
#define SERVO12_PIN 13

#define DOUT1_PIN 22
#define DOUT2_PIN 23
#define DOUT3_PIN 24
#define DOUT4_PIN 25
#define DOUT5_PIN 26
#define DOUT6_PIN 27
#define DOUT7_PIN 28
#define DOUT8_PIN 29

#define DRIVE_ACTIVE_PIN DOUT7_PIN
#define DOME_ACTIVE_PIN DOUT8_PIN

#define PPMIN_PIN 49

#define ANALOG1_PIN A0
#define ANALOG2_PIN A1

#define RCSEL_PIN 30
#define SEL2_PIN 31

#define STATUS_J1_PIN 32
#define STATUS_J2_PIN 33
#define STATUS_RC_PIN 34
#define STATUS_S1_PIN 35
#define STATUS_S2_PIN 36
#define STATUS_S3_PIN 37
#define STATUS_S4_PIN 38

#define CONSOLE_SERIAL Serial
#define XBEE_SERIAL Serial1
// #define VMUSIC_SERIAL       Serial2
#if !defined(DOME_DRIVE_SERIAL) && !defined(RDH_SERIAL)
#define AUX_SERIAL Serial3
#endif

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

class Gesture {
public:
  Gesture(const char *gestureStr = nullptr) { setGesture(gestureStr); }

  bool isEmpty() { return fGesture == 0; }

  uint8_t getGestureType(char ch) {
    switch (ch) {
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      return ch - '0';
    case 'A':
      return 10;
    case 'B':
      return 11;
    case 'C':
      return 12;
    case 'D':
      return 13;
    default:
      return 0;
    }
  }

  bool matches(const char *str) {
    if (fGesture == 0 && *str == '\0')
      return true;
    if (fGesture == 0)
      return false;
    const char *s = str;
    unsigned cnt = 0;
    uint32_t quotient = fGesture;
    while (quotient != 0 && cnt < MAX_GESTURE_LENGTH) {
      uint8_t val[] = {'\0', '1', '2', '3', '4', '5', '6',
                       '7',  '8', '9', 'A', 'B', 'C', 'D'};
      uint32_t remainder = quotient % 14;
      if (*s++ != val[remainder])
        return false;
      quotient = quotient / 14;
    }
    return (*s == '\0');
  }

  char *getGestureString(char *str) {
    char *s = str;
    unsigned cnt = 0;
    uint32_t quotient = fGesture;
    while (quotient != 0 && cnt < MAX_GESTURE_LENGTH) {
      uint8_t val[] = {'\0', '1', '2', '3', '4', '5', '6',
                       '7',  '8', '9', 'A', 'B', 'C', 'D'};
      uint32_t remainder = quotient % 14;
      *s++ = val[remainder];
      *s = '\0';
      quotient = quotient / 14;
    }
    return str;
  }

  void setGesture(const char *str) {
    fGesture = 0;
    const char *strend = str + strlen(str);
    while (str != strend) {
      // Base 16
      fGesture = fGesture * 14 + getGestureType(*--strend);
    }
  }

private:
  uint32_t fGesture;
};

////////////////////////////////

#include <EEPROM.h>
#include <XBee.h>

class PPMDecoder {
public:
  PPMDecoder(uint8_t pin, unsigned channelCount)
      : fPin(pin), fChannelCount(channelCount),
        fChannel(new uint16_t[fChannelCount]),
        fLastChannel(new uint16_t[fChannelCount]) {
    init();
  }

  ~PPMDecoder() {
    delete[] fChannel;
    delete[] fLastChannel;
  }

  void init() {
    fPass = 0;
    fPinState = LOW;
    fCurrent = fChannelCount;
    memset(fChannel, '\0', sizeof(fChannelCount) * sizeof(uint16_t));
    memset(fLastChannel, '\0', sizeof(fChannelCount) * sizeof(uint16_t));
  }

  bool decode() {
    uint32_t pulse = readPulse(PPMIN_PIN);
    if (!pulse)
      return false;
    if (fCurrent == fChannelCount) {
      if (pulse > 3000)
        fCurrent = 0;
    } else {
      fChannel[fCurrent++] = pulse;
      if (fCurrent == fChannelCount) {
        for (unsigned i = 0; i < fChannelCount; i++) {
          if (fChannel[i] > 2000 || fChannel[i] < 100) {
            fChannel[i] = fLastChannel[i];
          } else {
            fChannel[i] = (fLastChannel[i] + fChannel[i]) / 2;
            fPass++;
          }
        }
        if (fPass > 10) {
          for (unsigned i = 0; i < fChannelCount; i++) {
#ifdef USE_PPM_DEBUG
            DEBUG_PRINT("CH");
            DEBUG_PRINT(i + 1);
            DEBUG_PRINT.print(": ");
            DEBUG_PRINT(fChannel[i]);
            DEBUG_PRINT(" ");
#endif
            fLastChannel[i] = fChannel[i];
          }
#ifdef USE_PPM_DEBUG
          DEBUG_PRINT('\r');
#endif
          fPass = 0;
          return true;
        }
      }
    }
    return false;
  }

  uint16_t channel(unsigned ch, unsigned minvalue, unsigned maxvalue,
                   unsigned neutralvalue) {
    uint16_t pulse = (ch < fChannelCount) ? fChannel[ch] : 0;
    if (pulse != 0)
      return map(max(min(pulse, 1600), 600), 600, 1600, minvalue, maxvalue);
    return neutralvalue;
  }

private:
  uint8_t fPin;
  int fPinState;
  unsigned fPass;
  unsigned fCurrent;
  unsigned fChannelCount;
  uint32_t fRisingTime;
  uint16_t *fChannel;
  uint16_t *fLastChannel;

  uint32_t readPulse(uint8_t pin) {
    uint8_t state = digitalRead(pin);
    uint32_t pulseLength = 0;

    // On rising edge: record current time.
    if (fPinState == LOW && state == HIGH) {
      fRisingTime = micros();
    }

    // On falling edge: report pulse length.
    if (fPinState == HIGH && state == LOW) {
      unsigned long fallingTime = micros();
      pulseLength = fallingTime - fRisingTime;
    }

    fPinState = state;
    return pulseLength;
  }
};

////////////////////////////////

class AmidalaController;

////////////////////////////////

struct ButtonAction {
  enum {
    kNone = 0,
    kSound = 1,
    kServo = 2,
    kDigitalOut = 3,
    kI2CCmd = 4,
    kAuxStr = 5,
    kI2CStr = 6
  };
  uint8_t action;
  union {
    struct {
      uint8_t soundbank;
      uint8_t sound;
      uint8_t auxstring;
    } sound;
    struct {
      uint8_t num;
      uint8_t pos;
      uint8_t auxstring;
    } servo;
    struct {
      uint8_t num;
      uint8_t state;
      uint8_t auxstring;
    } dout;
    struct {
      uint8_t target;
      uint8_t cmd;
      uint8_t auxstring;
    } i2ccmd;
    struct {
      uint8_t unused1;
      uint8_t unused2;
      uint8_t auxstring;
    } aux;
    struct {
      uint8_t target;
      uint8_t cmd;
      uint8_t auxstring;
    } i2cstr;
  };

  void printDescription(Print *stream) {
    switch (action) {
    case kNone:
      break;
    case kSound:
      stream->print(F("Sound Bank #"));
      stream->print(sound.soundbank);
      if (sound.sound != 0) {
        stream->print(F(","));
        stream->print(sound.sound);
      }
      break;
    case kServo:
      stream->print(F("Servo #"));
      stream->print(servo.num);
      stream->print(F(", On Pos="));
      stream->print(servo.pos);
      break;
    case kDigitalOut:
      stream->print(F("DOut #"));
      stream->print(dout.num);
      stream->print(F(", Type="));
      switch (dout.state) {
      case 0:
        stream->print(F("NO"));
        break;
      case 1:
        stream->print(F("NC"));
        break;
      case 2:
        stream->print(F("MON"));
        break;
      }
      break;
    case kI2CCmd:
      stream->print(F("i2c Dest #"));
      stream->print(i2ccmd.target);
      stream->print(F(", Cmd="));
      stream->print(i2ccmd.cmd);
      break;
    case kAuxStr:
      stream->print(F("Aux #"));
      stream->print(aux.auxstring);
      break;
    case kI2CStr:
      stream->print(F("i2c Aux Output #"));
      stream->print(i2cstr.cmd);
      stream->print(F(", Dest "));
      stream->print(i2cstr.target);
      break;
    }
    if (action != kAuxStr && aux.auxstring != 0) {
      stream->print(F(", Aux #"));
      stream->print(aux.auxstring);
    }
    stream->println();
  }
};

struct GestureAction {
  Gesture gesture;
  ButtonAction action;

  void printDescription(Print *stream) {
    char buf[MAX_GESTURE_LENGTH + 1];
    if (!gesture.isEmpty()) {
      stream->print(gesture.getGestureString(buf));
      stream->print(F(": "));
      action.printDescription(stream);
    }
  }
};

struct AuxString {
  char str[32];
};

////////////////////////////////

class AmidalaConsole : public Print {
public:
  AmidalaConsole() : fPos(0) {}
  virtual ~AmidalaConsole() {}

  void init(AmidalaController *controller);
  void process();
  bool processConfig(const char *cmd);
  void processCommand(const char *cmd);
  bool process(char ch, bool config = false);

  void processGesture(const char *gesture);
  void process(ButtonAction &button);
  void processButton(unsigned num);
  void processLongButton(unsigned num);

  // inline void setVMusic(VMusic* vmusic)
  // {
  //     fVMusic = vmusic;
  // }

  virtual size_t write(uint8_t ch) { return write(&ch, 1); }

  virtual size_t write(const uint8_t *buffer, size_t size) override {
    if (fPrompt) {
      CONSOLE_SERIAL.println();
      fPrompt = false;
    }
    return CONSOLE_SERIAL.write(buffer, size);
  }

  void printNum(unsigned num, unsigned width = 4) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", num);
    for (size_t len = strlen(buf); len < width;) {
      buf[len++] = ' ';
      buf[len] = '\0';
    }
    print(buf);
  }

  void printServoPos(uint16_t num) {
    if (servoDispatch.isActive(num)) {
      printNum(servoDispatch.currentPos(num));
    } else {
      print("----");
    }
  }

  void randomToggle();
  void setVolumeNoResponse(uint8_t volume);
  void playSound(int sndbank, int snd = 0);
  void setServo();
  void setDigitalOut();
  void outputString();
  void showXBEE();
  void printVersion();
  void printHelp();
  void showLoadEEPROM(bool load = false);
  void showCurrentConfiguration();
  void writeCurrentConfiguration();
  void monitorToggle();
  void monitorOutput();
  void setMinimal(bool minimal) { fMinimal = minimal; }

private:
  AmidalaController *fController = nullptr;
  // VMusic* fVMusic = nullptr;
  unsigned fPos;
  char fBuffer[CONSOLE_BUFFER_SIZE];
  bool fMonitor = false;
  bool fPrompt = false;
  bool fMinimal = true;
};

class ServoPD {
public:
  ServoPD(long Kp, long Kd, long zero, long range, long scalebits = 8)
      : fKp(Kp), fKd(Kd), fPos(zero << scalebits),
        fPrevTarget(zero << scalebits), fZero(zero << scalebits),
        fRange(range << scalebits), fScaleBits(scalebits) {}

  // void attach(int pin, int pos)
  // {
  //     // itsServo.attach(pin);
  //     itsPos = (pos << itsScaleBits);
  //     // itsServo.write(pos);
  // }

  long get() const { return (fPos >> fScaleBits); }

  void update(long targetpos) {
    targetpos <<= fScaleBits;
    long diff = fKp * targetpos + fKd * (targetpos - fPrevTarget);
    fPos += (diff >> 16);
    fPos = constrain(fPos, fZero - fRange, fZero + fRange);
    // fServo.write(fPos >> fScaleBits);
    fPrevTarget = targetpos;
  }

  void reset(long targetpos) {
    targetpos <<= fScaleBits;
    fPos = constrain(fPos, fZero - fRange, fZero + fRange);
    fPrevTarget = targetpos;
  }

  long rawget() { return fPos >> fScaleBits; }

  void rawset(long rawval) {
    fPos = rawval << fScaleBits;
    // fServo.write(rawval);
  }

private:
  long fKp, fKd, fPos, fPrevTarget, fZero, fRange, fScaleBits;
};

// Pan and tilt servos zero values and +/- angular range, in degrees:
#define PANZERO 90
#define PANRANGE 60
#define TILTZERO 70
#define TILTRANGE 40

// Create one servo PD controler for camera pan and another for camera tilt:
ServoPD panservo(400, 200, PANZERO, PANRANGE);
ServoPD tiltservo(300, 100, TILTZERO, TILTRANGE);

////////////////////////////////

#ifdef EXPERIMENTAL_JEVOIS_STEERING

class JevoisConsole {
public:
  JevoisConsole() : fSteering(300) {}
  virtual ~JevoisConsole() {}

  void init(AmidalaController *controller);

  void processCommand(char *cmd = nullptr) {
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
  }

  void process() {
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

private:
  AmidalaController *fController = nullptr;
  TargetSteering fSteering;
  char fBuffer[128];
  unsigned fPos = 0;
  uint32_t fLastTime;
};
#endif

////////////////////////////////

class RDHSerial {
public:
  RDHSerial(Stream &stream) : fStream(stream) {}

  void setDomeDefaultMode(int mode) {
    switch (mode) {
    case 0:
      fStream.println("#DPHOME0\n#DPAUTO0");
      break;
    case 1:
      fStream.println("#DPHOME1\n#DPAUTO0");
      break;
    case 2:
      fStream.println("#DPHOME0\n#DPAUTO1");
      break;
    }
  }

  void setRelativePosition(int pos) {
    fStream.print(":DPDM");
    fStream.print(pos);
    fStream.println("+");
  }

  void setRelativePosition(int pos, int speed) {
    fStream.print(":DPDM");
    fStream.print(pos);
    fStream.print(",");
    fStream.print(speed);
    fStream.println("+");
  }

  void setAbsolutePosition(int pos) {
    fStream.print(":DPAM");
    fStream.print(pos);
    fStream.println("+");
  }

  void setAbsolutePosition(int pos, int speed) {
    fStream.print(":DPAM");
    fStream.print(pos);
    fStream.print(",");
    fStream.print(speed);
    fStream.println("+");
  }

  void setDomeHomePosition() { fStream.println("#DPHOMEPOS"); }

  void setDomeHomePosition(int pos) {
    fStream.print("#DPHOMEPOS");
    fStream.println(pos);
  }

  void toggleRandomDome() {
    DEBUG_PRINTLN("TOGGLE AUTO DOME RANDOM");
    fStream.println("#DPAUTO");
  }

  void sendCommand(const char *cmd) { fStream.println(cmd); }

  bool ready() { return (fPosition != -1); }

  int getAngle() { return fPosition; }

  int getMode() { return fMode; }

  int getHome() { return fMode; }

  void process() {
    // append commands to command buffer
    while (fStream.available()) {
      int ch = fStream.read();
      if (ch == '\r' || ch == '\n') {
        if (fState == 4) {
          // Update position
          fSamples.append(fValue);
          if (fSampleCount < 6) {
            fPosition = fValue;
            fSampleCount++;
          } else {
            // Return the filtered angle
            fPosition = fSamples.median();
          }
          // DOME_SENSOR_SERIAL_PRINT(" - ");
          // DOME_SENSOR_SERIAL_PRINTLN(fPosition);
        }
        fState = 0;
        if (fStream.available() < 10)
          return;
        continue;
      } else {
        // DOME_SENSOR_SERIAL_PRINT((char)ch);
      }
      if (fState == -1)
        continue;
      switch (fState) {
      case 0:
        fState = (ch == '#') ? fState + 1 : -1;
        break;
      case 1:
        fState = (ch == 'D') ? fState + 1 : -1;
        break;
      case 2:
        fState = (ch == 'P') ? fState + 1 : -1;
        break;
      case 3:
        switch (ch) {
        case '@':
          fMode = 0;
          fState = fState + 1;
          break;
        case '!':
          fMode = 1;
          fState = fState + 1;
          break;
        case '$':
          fMode = 2;
          fState = fState + 1;
          break;
        case '%':
          fMode = 3;
          fState = fState + 1;
          break;
        default:
          fState = -1;
          break;
        }
        fValue = 0;
        break;
      case 4:
        if (ch >= '0' && ch <= '9') {
          fValue = fValue * 10 + (ch - '0');
        } else {
          fState = -1;
        }
        break;
      }
      if (fState == -1) {
        // ERROR: Ignore remaining input
        DEBUG_PRINTLN("[DOME SENSOR] ERROR READING POSITION");
        fErrorCount++;
      }
    }
  }

protected:
  Stream &fStream;
  int fPosition = -1;
  int8_t fState = 0;
  int fValue = 0;
  int fMode = 0;
  int fSampleCount = 0;
  unsigned fErrorCount = 0;
  MedianSampleBuffer<short, 5> fSamples;
};

////////////////////////////////

class AmidalaController : public SetupEvent, public AnimatedEvent {
public:
  AmidalaController()
      : fConsole(),
        // fVMusic(VMUSIC_SERIAL),
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

  struct AmidalaParameters {
    unsigned getRadioChannelCount() {
      init();
      return rcchn;
    }

    struct SoundBank {
      char dir[9];
      uint8_t numfiles;
      uint8_t playindex;
      bool random;
    };

    struct Channel {
      uint8_t min; /* Min position you want the servo to reach. Always has to be
                      less than servo max. */
      uint8_t max; /* Max position you want the servo to reach. Always has to be
                      greater than servo min. */
      uint8_t n;   /* Center or Neutral position of the servo */
      uint8_t d; /* Number of degrees around center that will still register as
                    center */
      int16_t t; /* Logically shift the center position of the servo/joystick
                    either left or right */
      bool r;    /* Flip the direction of the servo */
      uint8_t s; /* How fast the servo will move or accelerate, 1 = Slowest,
                    100=Fastest */
      uint16_t minpulse; /* Optional value. Sets min pulse for servo channel on
                            startup */
      uint16_t maxpulse; /* Optional value. Sets max pulse for servo channel on
                            startup */
    };

    struct DigitalOut {
      bool state;
    };

    char serial[5];
    uint32_t xbr;      /* Right XBEE's unique serial number (lower address) */
    uint32_t xbl;      /* Left XBEE's unique serial number (lower address) */
    uint8_t rcchn;     /* How many channels does the RC radio have */
    uint16_t minpulse; /* Minimum pulse width for all servo outs (internal
                          default 1000) */
    uint16_t maxpulse; /* Maximum pulse width for all servo outs (internal
                          default 2000) */
    uint16_t rvrmin; /* Adjust Right Joystick Analog MIN "Reference Voltage" */
    uint16_t rvrmax; /* Adjust Right Joystick Analog MAX "Reference Voltage" */
    uint16_t rvlmin; /* Adjust Left Joystick Analog MIN "Reference Voltage" */
    uint16_t rvlmax; /* Adjust Left Joystick Analog MAX "Reference Voltage" */
    uint16_t fst; /* Adjust the failsafe timeout. You wouldn’t normally adjust
                     this */
    uint8_t rcd;
    uint8_t rcj;

    SoundBank SB[20];
    Channel S[12];
    ButtonAction B[9];
    ButtonAction LB[9];
    GestureAction G[10];
    DigitalOut D[8];
    AuxString A[MAX_AUX_STRINGS];
    uint8_t acount;
    uint8_t gcount;
    uint8_t sbcount;
    uint8_t volume;
    bool startup;
    bool rndon;
    bool ackon;
    char acktype;
    unsigned mindelay;
    unsigned maxdelay;
    bool mix12;
    uint8_t myi2c;
    uint32_t auxbaud;
    char auxinit[16];
    uint8_t auxdelim;
    uint8_t auxeol;
    bool autocorrect;
    char b9;
    bool goslow;
    uint8_t j1adjv;
    uint8_t j1adjh;

    Gesture rnd;
    Gesture ackgest;
    Gesture slowgest;
    Gesture domegest;

    uint16_t domepos; // delete?
    uint16_t domehome;
    uint8_t domemode;
    uint8_t domehomemin;
    uint8_t domehomemax;
    uint8_t domeseekmin;
    uint8_t domeseekmax;
    uint8_t domeseekr;
    uint8_t domeseekl;
    uint8_t domefudge;
    uint8_t domespeed;
    uint8_t domespeedhome;
    uint8_t domespeedseek;
    uint16_t domespmin; // only for analog
    uint16_t domespmax; // only for analog
    bool domech6;       // delete?
    bool domeimu;       // delete?
    bool domeflip;

    constexpr unsigned getSoundBankCount() {
      return sizeof(SB) / sizeof(SB[0]);
    }

    constexpr unsigned getServoCount() { return sizeof(S) / sizeof(S[0]); }

    constexpr unsigned getButtonCount() { return sizeof(B) / sizeof(B[0]); }

    constexpr unsigned getGestureCount() { return sizeof(G) / sizeof(G[0]); }

    constexpr unsigned getAuxStringCount() { return sizeof(A) / sizeof(A[0]); }

    void init(bool forceReload = false) {
      static bool sInited;
      static bool sRAMInited;
      if (sInited && !forceReload)
        return;
      if (!sRAMInited) {
        memset(this, '\0', sizeof(*this));
        volume = 50;
        startup = true;
        rndon = true;
        rnd.setGesture("3");
        ackon = false;
        ackgest.setGesture("252");
        //                acktype = "ads";
        mindelay = 60;
        maxdelay = 120;
        mix12 = false;
        rcd = 30;
        rcj = 5;
        myi2c = 0;
        auxbaud = 9600;
        auxdelim = ':';
        auxeol = 13;
        autocorrect = false;
        b9 = 'n';
        slowgest.setGesture("858");
        goslow = false;
        j1adjv = 0;
        j1adjh = 0;
        domehome = DEFAULT_DOME_HOME_POSITION;
        domepos = domehome;
        domemode = 1;
        domehomemin = DEFAULT_DOME_HOME_MIN_DELAY;
        domehomemax = DEFAULT_DOME_HOME_MAX_DELAY;
        domeseekmin = DEFAULT_DOME_SEEK_MIN_DELAY;
        domeseekmax = DEFAULT_DOME_SEEK_MAX_DELAY;
        domeseekl = DEFAULT_DOME_SEEK_LEFT;
        domeseekr = DEFAULT_DOME_SEEK_RIGHT;
        domefudge = DEFAULT_DOME_FUDGE;
        domespeed = DOME_MAXIMUM_SPEED;
        domespeedhome = DEFAULT_DOME_SPEED_HOME;
        domespeedseek = DEFAULT_DOME_SPEED_SEEK;
        domespmin = 42;
        domespmax = 935;
        domech6 = false;
        domeflip = DEFAULT_DOME_INVERTED;
        domeimu = true; /* no longer used */
      }
      size_t offs = 0;
      if (EEPROM.read(offs) == 'D' && EEPROM.read(offs + 1) == 'B' &&
          EEPROM.read(offs + 2) == '0' && EEPROM.read(offs + 3) == '1' &&
          EEPROM.read(offs + 4) == 0) {
        offs += 5;
        for (unsigned i = 0; i < sizeof(serial); i++, offs++)
          serial[i] = EEPROM.read(offs);
        // Ensure last character is zero
        if (serial[sizeof(serial) - 1] != 0)
          serial[sizeof(serial) - 1] = 0;
      }
      offs = 0x64;
      if (EEPROM.read(offs) == 'S' && EEPROM.read(offs + 1) == 'C' &&
          EEPROM.read(offs + 2) == '2' && EEPROM.read(offs + 3) == '3' &&
          EEPROM.read(offs + 4) == 0) {
        offs += 5;
        xbr = ((uint32_t)EEPROM.read(offs + 3) << 24) |
              ((uint32_t)EEPROM.read(offs + 2) << 16) |
              ((uint32_t)EEPROM.read(offs + 1) << 8) |
              ((uint32_t)EEPROM.read(offs + 0) << 0);
        offs += sizeof(uint32_t);

        xbl = ((uint32_t)EEPROM.read(offs + 3) << 24) |
              ((uint32_t)EEPROM.read(offs + 2) << 16) |
              ((uint32_t)EEPROM.read(offs + 1) << 8) |
              ((uint32_t)EEPROM.read(offs + 0) << 0);
        offs += sizeof(uint32_t);

        rcchn = EEPROM.read(offs++);

        unsigned unknown;
        // ?
        unknown = EEPROM.read(offs++);
        // CONSOLE_SERIAL.println("?: "+String(unknown));

        minpulse = ((uint16_t)EEPROM.read(offs + 1) << 8) |
                   ((uint16_t)EEPROM.read(offs + 0) << 0);
        offs += sizeof(uint16_t);

        maxpulse = ((uint16_t)EEPROM.read(offs + 1) << 8) |
                   ((uint16_t)EEPROM.read(offs + 0) << 0);
        offs += sizeof(uint16_t);

        // unknown  6?
        unknown = EEPROM.read(offs++);
        // CONSOLE_SERIAL.println("?: "+String(unknown));

        // unknown  1?
        unknown = EEPROM.read(offs++);
        // CONSOLE_SERIAL.println("?: "+String(unknown));

        // unknown  7?
        unknown = EEPROM.read(offs++);
        // CONSOLE_SERIAL.println("?: "+String(unknown));

        // unknown  2?
        unknown = EEPROM.read(offs++);
        // CONSOLE_SERIAL.println("?: "+String(unknown));

        // unknown  0?
        unknown = EEPROM.read(offs++);
        // CONSOLE_SERIAL.println("?: "+String(unknown));

        rvrmin = ((uint16_t)EEPROM.read(offs + 1) << 8) |
                 ((uint16_t)EEPROM.read(offs + 0) << 0);
        offs += sizeof(uint16_t);

        rvlmin = ((uint16_t)EEPROM.read(offs + 1) << 8) |
                 ((uint16_t)EEPROM.read(offs + 0) << 0);
        offs += sizeof(uint16_t);

        rvrmax = ((uint32_t)EEPROM.read(offs + 1) << 8) |
                 ((uint16_t)EEPROM.read(offs + 0) << 0);
        offs += sizeof(uint16_t);

        rvlmax = ((uint16_t)EEPROM.read(offs + 1) << 8) |
                 ((uint16_t)EEPROM.read(offs + 0) << 0);
        offs += sizeof(uint16_t);

        fst = ((uint16_t)EEPROM.read(offs + 1) << 8) |
              ((uint16_t)EEPROM.read(offs + 0) << 0);
        offs += sizeof(uint16_t);

        offs = 0xea;
        for (unsigned i = 0; i < sizeof(S) / sizeof(S[0]); i++) {
          S[i].min = EEPROM.read(offs++);
          S[i].max = EEPROM.read(offs++);
          S[i].n = EEPROM.read(offs++);
          S[i].d = EEPROM.read(offs++);
          S[i].t = (int16_t)((uint16_t)EEPROM.read(offs + 1) << 8) |
                   ((uint16_t)EEPROM.read(offs + 0) << 0);
          offs += sizeof(uint16_t);
          S[i].r = EEPROM.read(offs++);
          S[i].s = (uint8_t)ceil((float)EEPROM.read(offs++) / 255.0f * 10) * 10;
          S[i].minpulse = ((uint16_t)EEPROM.read(offs + 1) << 8) |
                          ((uint16_t)EEPROM.read(offs + 0) << 0);
          offs += sizeof(uint16_t);
          S[i].maxpulse = ((uint16_t)EEPROM.read(offs + 1) << 8) |
                          ((uint16_t)EEPROM.read(offs + 0) << 0);
          offs += sizeof(uint16_t);
        }
        // disable unused variable warning
        (void)unknown;
      }
      sInited = true;
    }
  };

  class XBeePocketRemote : public JoystickController {
  public:
    XBeePocketRemote() {
      memset(&state, '\0', sizeof(state));
      memset(&event, '\0', sizeof(event));
      memset(&longpress, '\0', sizeof(longpress));
      fConnecting = true;
      fConnected = false;
      failsafeNotice = true;
    }

    uint32_t addr;
    uint16_t y;
    uint16_t x;
    uint16_t w1;
    uint16_t w2;
    bool button[5];
    enum Type { kFailsafe, kXBee, kRC };
    struct LongPress {
      uint32_t pressTime;
      bool longPress;
    };
    struct {
      LongPress l3;
      LongPress triangle;
      LongPress circle;
      LongPress cross;
      LongPress square;
    } longpress;
    Type type;
    bool failsafeNotice;
    uint32_t lastPacket;

    bool failsafe() { return (type == XBeePocketRemote::kFailsafe); }

    void update() {
      Event evt = {};
      State prev = state;

      state.analog.stick.lx = map(x, 0, 1024, 127, -128);
      state.analog.stick.ly = map(y, 0, 1024, 127, -128);
      state.analog.stick.rx = state.analog.stick.lx;
      state.analog.stick.ry = state.analog.stick.ly;
      state.analog.button.l1 = map(w1, 0, 1024, 255, 0);
      state.analog.button.l2 = map(w2, 0, 1024, 255, 0);
      // Serial.print(state.analog.button.l1); Serial.print(" - ");
      // Serial.println(state.analog.button.l2);
      state.analog.button.r1 = state.analog.button.l1;
      state.analog.button.r2 = state.analog.button.l2;
      state.button.triangle = button[0];
      state.button.circle = button[1];
      state.button.cross = button[2];
      state.button.square = button[3];
      state.button.l3 = button[4];

      // for (int i = 0; i < SizeOfArray(button); i++)
      // {
      //     if (button[i])
      //     {
      //         CONSOLE_SERIAL.print("BUTTON_DOWN ");
      //         CONSOLE_SERIAL.println(i);
      //     }
      // }
#define CHECK_BUTTON_DOWN(b)                                                   \
  evt.button_down.b = (!prev.button.b && state.button.b)
      CHECK_BUTTON_DOWN(l3);
      CHECK_BUTTON_DOWN(triangle);
      CHECK_BUTTON_DOWN(circle);
      CHECK_BUTTON_DOWN(cross);
      CHECK_BUTTON_DOWN(square);
#define CHECK_BUTTON_UP(b) evt.button_up.b = (prev.button.b && !state.button.b)
      CHECK_BUTTON_UP(l3);
      CHECK_BUTTON_UP(triangle);
      CHECK_BUTTON_UP(circle);
      CHECK_BUTTON_UP(cross);
      CHECK_BUTTON_UP(square);
#define CHECK_BUTTON_LONGPRESS(b)                                              \
  {                                                                            \
    evt.long_button_up.b = false;                                              \
    if (evt.button_down.b) {                                                   \
      longpress.b.pressTime = millis();                                        \
      longpress.b.longPress = false;                                           \
    } else if (evt.button_up.b) {                                              \
      longpress.b.pressTime = 0;                                               \
      if (longpress.b.longPress)                                               \
        evt.button_up.b = false;                                               \
      longpress.b.longPress = false;                                           \
    } else if (longpress.b.pressTime != 0 && state.button.b) {                 \
      if (longpress.b.pressTime + LONG_PRESS_TIME < millis()) {                \
        longpress.b.pressTime = 0;                                             \
        longpress.b.longPress = true;                                          \
        evt.long_button_up.b = true;                                           \
      }                                                                        \
    }                                                                          \
  }
      CHECK_BUTTON_LONGPRESS(l3);
      CHECK_BUTTON_LONGPRESS(triangle);
      CHECK_BUTTON_LONGPRESS(circle);
      CHECK_BUTTON_LONGPRESS(cross);
      CHECK_BUTTON_LONGPRESS(square);

      /* Analog events */
      evt.analog_changed.stick.lx =
          state.analog.stick.lx - prev.analog.stick.lx;
      evt.analog_changed.stick.ly =
          state.analog.stick.ly - prev.analog.stick.ly;
      evt.analog_changed.button.l1 =
          state.analog.button.l1 - prev.analog.button.l1;
      evt.analog_changed.button.l2 =
          state.analog.button.l2 - prev.analog.button.l2;
      evt.analog_changed.button.r1 =
          state.analog.button.r1 - prev.analog.button.r1;
      evt.analog_changed.button.r2 =
          state.analog.button.r2 - prev.analog.button.r2;
      if (fConnecting) {
        fConnecting = false;
        fConnected = true;
        onConnect();
      }
      if (fConnected) {
        event = evt;
        notify();
        if (failsafe()) {
          fConnected = false;
          fConnecting = true;
          onDisconnect();
        }
      }
    }
  };

  class DriveController : public XBeePocketRemote {
  public:
    DriveController(AmidalaController *driver) : fDriver(driver) {}

    virtual void notify() override {
      uint32_t currentTime = millis();
      uint32_t lagTime =
          (currentTime > fLastTime) ? currentTime - fLastTime : 0;
      if (event.analog_changed.button.l2) {
        fDriver->setDriveThrottle(float(state.analog.button.l2) / 255.0);
      }
      if (lagTime > 5000) {
        DEBUG_PRINTLN("More than 5 seconds. Disconnect");
        fDriver->emergencyStop();
        disconnect();
      } else if (lagTime > 500) {
        DEBUG_PRINTLN("It has been 500ms. Shutdown motors");
        fDriver->emergencyStop();
      } else {
        if (event.button_up.l3)
          fDriver->processButton(5);
        if (event.button_up.cross)
          fDriver->processButton(3);
        if (event.button_up.circle)
          fDriver->processButton(2);
        if (event.button_up.triangle)
          fDriver->processButton(1);
        if (event.button_up.square)
          fDriver->processButton(4);

        if (event.long_button_up.l3)
          fDriver->processLongButton(5);
        if (event.long_button_up.cross)
          fDriver->processLongButton(3);
        if (event.long_button_up.circle)
          fDriver->processLongButton(2);
        if (event.long_button_up.triangle)
          fDriver->processLongButton(1);
        if (event.long_button_up.square)
          fDriver->processLongButton(4);
      }
      fLastTime = currentTime;
    }

    virtual void onConnect() override {
      DEBUG_PRINTLN("Drive Stick Connected");
      fDriver->enableController();
      fLastTime = millis();
    }

    virtual void onDisconnect() override {
      DEBUG_PRINTLN("Drive Stick Disconnected");
      fDriver->disableController();
    }

    uint32_t fLastTime = 0;
    AmidalaController *fDriver;
  };

  class DomeController : public XBeePocketRemote {
  public:
    DomeController(AmidalaController *driver) : fDriver(driver) {}

    virtual void notify() override {
      uint32_t currentTime = millis();
      uint32_t lagTime =
          (currentTime > fLastTime) ? currentTime - fLastTime : 0;
      if (lagTime > 5000) {
        DEBUG_PRINTLN("More than 5 seconds. Disconnect");
        fDriver->domeEmergencyStop();
        disconnect();
      } else if (lagTime > 500) {
        DEBUG_PRINTLN("It has been 500ms. Shutdown motors");
        fDriver->domeEmergencyStop();
      } else {
        process();
      }
      fLastTime = currentTime;
    }

    void process() {
      if (event.analog_changed.button.l1) {
        /* Volume */
        fDriver->setVolumeNoResponse(
            map(state.analog.button.l1, 0, 255, 0, 100));
      }
      if (event.analog_changed.button.l2) {
        /* Throttle */
        // Serial.println(state.analog.button.l2);//map(state.button.l1, 0, 255,
        // 100, 0));
        fDriver->setDomeThrottle(float(state.analog.button.l2) / 255.0);
      }

      if (!fGestureCollect) {
        if (event.button_up.l3) {
          DEBUG_PRINTLN("GESTURE START COLLECTING\n");
          fDriver->disableDomeController();
          fGestureCollect = true;
          fGesturePtr = fGestureBuffer;
          fGestureTimeOut = millis() + GESTURE_TIMEOUT_MS;
        } else {
          if (event.button_up.cross)
            fDriver->processButton(8);
          if (event.button_up.circle)
            fDriver->processButton(7);
          if (event.button_up.triangle)
            fDriver->processButton(6);
          if (event.button_up.square)
            fDriver->processButton(9);

          if (event.long_button_up.cross)
            fDriver->processLongButton(8);
          if (event.long_button_up.circle)
            fDriver->processLongButton(7);
          if (event.long_button_up.triangle)
            fDriver->processLongButton(6);
          if (event.long_button_up.square)
            fDriver->processLongButton(9);

          // if (event.long_button_up.triangle)
          // {
          //     CONSOLE_SERIAL.println("Processing Long Button 6");
          //     fDriver->setDomeHomePosition();
          // }
          // if (event.long_button_up.square)
          // {
          //     // toggle random dome mode
          //     CONSOLE_SERIAL.println("Processing Long Button 9");
          //     fDriver->toggleRandomDome();
          // }
        }
        return;
      } else if (fGestureTimeOut < millis()) {
        DEBUG_PRINTLN("GESTURE TIMEOUT\n");
        fDriver->enableDomeController();
        fGesturePtr = fGestureBuffer;
        fGestureCollect = false;
      } else {
        if (event.button_up.l3) {
          // delete trailing '5' from gesture
          unsigned glen = strlen(fGestureBuffer);
          if (glen > 0 && fGestureBuffer[glen - 1] == '5')
            fGestureBuffer[glen - 1] = 0;
          fDriver->enableDomeController();
          fGestureCollect = false;
          fDriver->processGesture(fGestureBuffer);
          return;
        }

        if (event.button_up.triangle)
          addGesture('A');
        if (event.button_up.circle)
          addGesture('B');
        if (event.button_up.cross)
          addGesture('C');
        if (event.button_up.square)
          addGesture('D');
        if (!fGestureAxis) {
          if (abs(state.analog.stick.lx) > 50 &&
              abs(state.analog.stick.ly) > 50) {
            // Diagonal
            if (state.analog.stick.lx < 0)
              fGestureAxis = (state.analog.stick.ly < 0) ? '1' : '7';
            else
              fGestureAxis = (state.analog.stick.ly < 0) ? '3' : '9';
            addGesture(fGestureAxis);
          } else if (abs(state.analog.stick.lx) > 100) {
            // Horizontal
            fGestureAxis = (state.analog.stick.lx < 0) ? '4' : '6';
            addGesture(fGestureAxis);
          } else if (abs(state.analog.stick.ly) > 100) {
            // Vertical
            fGestureAxis = (state.analog.stick.ly < 0) ? '2' : '8';
            addGesture(fGestureAxis);
          }
        }
        if (fGestureAxis && abs(state.analog.stick.lx) < 10 &&
            abs(state.analog.stick.ly) < 10) {
          addGesture('5');
          fGestureAxis = 0;
        }
      }
    }

    virtual void onConnect() override {
      DEBUG_PRINTLN("Dome Stick Connected");
      fDriver->enableDomeController();
      fLastTime = millis();
    }

    virtual void onDisconnect() override {
      DEBUG_PRINTLN("Dome Stick Disconnected");
      fDriver->disableDomeController();
    }

    uint32_t fLastTime = 0;
    AmidalaController *fDriver;

  protected:
    bool fGestureCollect = false;
    char fGestureBuffer[MAX_GESTURE_LENGTH + 1];
    char *fGesturePtr = fGestureBuffer;
    char fGestureAxis = 0;
    uint32_t fGestureTimeOut = 0;

    void addGesture(char ch) {
      if (size_t(fGesturePtr - fGestureBuffer) < sizeof(fGestureBuffer) - 1) {
        *fGesturePtr++ = ch;
        *fGesturePtr = '\0';
        fGestureTimeOut = millis() + GESTURE_TIMEOUT_MS;
      }
    }
  };

  AmidalaConsole fConsole;
  // VMusic fVMusic;
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
  // I2CReceiver fI2C;
#if DRIVE_SYSTEM == DRIVE_SYSTEM_SABER
  TankDriveSabertooth fTankDrive;
#elif DRIVE_SYSTEM == DRIVE_SYSTEM_PWM
  TankDrivePWM fTankDrive
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

  void emergencyStop() {
#ifdef DRIVE_SYSTEM
    fTankDrive.stop();
#endif
  }

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

  void domeEmergencyStop() {
#ifdef DOME_DRIVE
    fDomeDrive.stop();
#endif
  }

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

  bool readConfigFromSD() {
    if (!SD.begin(4)) {
      Serial.println("initialization failed!");
      return false;
    }
    Serial.println("initialization done.");
    File conf = SD.open("config.txt");

    if (conf) {
      ConfigParser parser(fConsole);

      // read from the file and feed each character through
      // the config parser (just like VMusic.parseTextFile did)
      while (conf.available()) {
        char ch = conf.read();
        parser.process(ch);
      }
      // close the file:
      conf.close();
      return true;
    } else {
      // if the file didn't open, print an error:
      Serial.println("error opening config.txt");
    }
    return false;
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

  virtual void setup() override {
    fConsole.println(F("Loading config from EEPROM"));
    params.init();

    fConsole.init(this);

#ifdef EXPERIMENTAL_JEVOIS_STEERING
    fJevois.init(this);
#endif

    fConsole.println(F("Reading Config File"));
    fConsole.setMinimal(!readConfigFromSD());
    fConsole.showCurrentConfiguration();

    fConsole.println(F("Activating Servos"));
    fConsole.println(F("Activating Digital Outputs"));
    fConsole.println(F("Init i2c Bus"));

    // fI2C.begin(params.myi2c);
    fConsole.println(F("No i2c devices configured"));
    if (params.autocorrect)
      fConsole.println(F("Auto Correct Gestures Enabled"));

#ifdef AUX_SERIAL
    AUX_SERIAL.begin(params.auxbaud);
    sendAuxString(params.auxinit);
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
      float neutral = float(params.S[i].n) / float(params.S[i].max);
      uint16_t minpulse = params.S[i].minpulse;
      uint16_t maxpulse = params.S[i].maxpulse;
      if (params.S[i].r) {
        maxpulse = params.S[i].minpulse;
        minpulse = params.S[i].maxpulse;
      }
      uint16_t neutralpulse =
          params.S[i].minpulse +
          neutral * (params.S[i].maxpulse - params.S[i].minpulse);
      servoDispatch.setServo(i, SERVO1_PIN + i, minpulse, maxpulse,
                             neutralpulse, 0);
    }
  }

  virtual void animate() override {
    fConsole.process();
#ifdef EXPERIMENTAL_JEVOIS_STEERING
    fJevois.process();
#endif
#ifdef RDH_SERIAL
    fAutoDome.process();
#endif
    // fVMusic.process();

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

private:
  XBee fXBee;
  ZBRxIoSampleResponse fResponse;
  PPMDecoder fPPMDecoder;
  bool fMinimal = true;
  uint32_t fDriveStateMillis = 0;
  uint32_t fDomeStateMillis = 0;
  float fDomeThrottle = 0;
  float fDriveThrottle = 0;

  class ConfigParser {
  public:
    ConfigParser(AmidalaConsole &console) : fConsole(console) {}

    virtual void process(char ch) {
      // if (ch == '\r')
      //     DEBUG_PRINTLN();
      // else
      //     DEBUG_PRINT(ch);
      fConsole.process(ch, true);
    }

  private:
    AmidalaConsole &fConsole;
  };

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
};

AmidalaController amidala;

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
  AmidalaController::AmidalaParameters &params = fController->params;
  params.rndon = !params.rndon;
  print(F("Random "));
  println((params.rndon) ? F("On") : F("Off"));
}

void AmidalaConsole::setVolumeNoResponse(uint8_t volume) {
  // if (fVMusic != nullptr)
  //     fVMusic->setVolumeNoResponse(volume);
}

void AmidalaConsole::playSound(int sndbank, int snd) {
  AmidalaController::AmidalaParameters &params = fController->params;
  if (false) {
    // if (fVMusic->isPlaying())
    // {
    //     println(F("Busy"));
    //     return;
    // }
    AmidalaController::AmidalaParameters::SoundBank *sb = params.SB;
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
      Serial.println(snd);
      if (snd <= sb->numfiles) {
        char fname[16];
        snprintf(fname, sizeof(fname), "%s-%d.MP3", sb->dir, snd + 1);
        Serial.print("PLAY: ");
        Serial.println(fname);
        // fVMusic->play(fname, sb->dir);
      }
    }
  } else {
    println(F("Invalid"));
  }
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
  AmidalaController::AmidalaParameters &params = fController->params;
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
  AmidalaController::AmidalaParameters &params = fController->params;
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
  }
  if (button.aux.auxstring != 0 &&
      button.aux.auxstring <= params.getAuxStringCount()) {
    DEBUG_PRINT("AUX #");
    DEBUG_PRINTLN(params.A[button.aux.auxstring - 1].str);
    fController->sendAuxString(params.A[button.aux.auxstring - 1].str);
  }
}

void AmidalaConsole::processGesture(const char *gesture) {
  AmidalaController::AmidalaParameters &params = fController->params;
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
  AmidalaController::AmidalaParameters &params = fController->params;

  if (num < params.getButtonCount()) {
    process(params.B[num - 1]);
  }
}

void AmidalaConsole::processLongButton(unsigned num) {
  print("Processing Long Button ");
  println(num);
  AmidalaController::AmidalaParameters &params = fController->params;

  if (num < params.getButtonCount()) {
    process(params.LB[num - 1]);
  }
}

void AmidalaConsole::showLoadEEPROM(bool load) {
  AmidalaController::AmidalaParameters &params = fController->params;
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
  AmidalaController::AmidalaParameters &params = fController->params;
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
    AmidalaController::AmidalaParameters::SoundBank *sb = &params.SB[0];

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
  AmidalaController::AmidalaParameters &params = fController->params;
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
  AmidalaController::AmidalaParameters &params = fController->params;
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

bool startswith(const char *&cmd, const char *str) {
  size_t len = strlen(str);
  if (strncmp(cmd, str, strlen(str)) == 0) {
    cmd += len;
    return true;
  }
  return false;
}

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

int32_t strtol(const char *cmd, const char **endptr) {
  bool sign = false;
  int32_t result = 0;
  if (*cmd == '-') {
    cmd++;
    sign = true;
  }
  while (isdigit(*cmd)) {
    result = result * 10L + (*cmd - '0');
    cmd++;
  }
  *endptr = cmd;
  return (sign) ? -result : result;
}

uint32_t strtolu(const char *cmd, const char **endptr) {
  uint32_t result = 0;
  while (isdigit(*cmd)) {
    result = result * 10L + (*cmd - '0');
    cmd++;
  }
  *endptr = cmd;
  return result;
}

bool boolparam(const char *cmd, const char *match, bool &value) {
  if (startswith(cmd, match)) {
    if (strcmp(cmd, "y") == 0) {
      value = true;
      return true;
    } else if (strcmp(cmd, "n") == 0) {
      value = false;
      return true;
    }
  }
  return false;
}

bool charparam(const char *cmd, const char *match, const char *oneof,
               char &value) {
  if (startswith(cmd, match) && cmd[1] == '\0') {
    while (*oneof != '\0') {
      if (*oneof++ == cmd[0]) {
        value = cmd[0];
        return true;
      }
    }
  }
  return false;
}

bool sintparam(const char *cmd, const char *match, int32_t &value) {
  if (startswith(cmd, match)) {
    int32_t val = strtol(cmd, &cmd);
    if (*cmd == '\0') {
      value = val;
      return true;
    }
  }
  return false;
}

bool sintparam2(const char *cmd, const char *match, int32_t &value1,
                int32_t &value2) {
  if (startswith(cmd, match)) {
    int32_t val = strtol(cmd, &cmd);
    if (*cmd == ',') {
      value1 = val;
      val = strtol(cmd + 1, &cmd);
      if (*cmd == '\0') {
        value2 = val;
        return true;
      }
    }
  }
  return false;
}

bool intparam(const char *cmd, const char *match, uint32_t &value,
              uint32_t minval, uint32_t maxval) {
  if (startswith(cmd, match)) {
    uint32_t val = strtolu(cmd, &cmd);
    if (*cmd == '\0') {
      value = min(max(val, minval), maxval);
      return true;
    }
  }
  return false;
}

bool intparam(const char *cmd, const char *match, uint16_t &value,
              uint16_t minval, uint16_t maxval) {
  if (startswith(cmd, match)) {
    uint16_t val = strtolu(cmd, &cmd);
    if (*cmd == '\0') {
      value = min(max(val, minval), maxval);
      return true;
    }
  }
  return false;
}

bool intparam(const char *cmd, const char *match, uint8_t &value,
              uint8_t minval, uint8_t maxval) {
  if (startswith(cmd, match)) {
    uint8_t val = strtolu(cmd, &cmd);
    if (*cmd == '\0') {
      value = min(max(val, minval), maxval);
      return true;
    }
  }
  return false;
}

bool gestureparam(const char *cmd, const char *match, Gesture &gesture) {
  if (startswith(cmd, match)) {
    gesture.setGesture(cmd);
    return true;
  }
  return false;
}

bool numberparams(const char *cmd, uint8_t &argcount, int *args,
                  uint8_t maxcount) {
  for (argcount = 0; argcount < maxcount; argcount++) {
    args[argcount] = strtol(cmd, &cmd);
    if (*cmd == '\0') {
      argcount++;
      return true;
    } else if (*cmd != ',') {
      return false;
    }
    cmd++;
  }
  return true;
}

bool numberparams(const char *cmd, uint8_t &argcount, uint8_t *args,
                  uint8_t maxcount) {
  for (argcount = 0; argcount < maxcount; argcount++) {
    args[argcount] = strtolu(cmd, &cmd);
    if (*cmd == '\0') {
      argcount++;
      return true;
    } else if (*cmd != ',') {
      return false;
    }
    cmd++;
  }
  return true;
}

bool AmidalaConsole::processConfig(const char *cmd) {
  bool boolarg;
  int32_t sintarg;
  int32_t sintarg2;
  AmidalaController::AmidalaParameters &params = fController->params;
#ifdef RDH_SERIAL
  RDHSerial &autoDome = fController->fAutoDome;
#endif
  DomeDrive *domeDrive = &fController->fDomeDrive;
  if (startswith(cmd, "sb=")) {
    if (params.sbcount < params.getSoundBankCount()) {
      AmidalaController::AmidalaParameters::SoundBank *sb =
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
    AmidalaController::AmidalaParameters::Channel *s = params.S;
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
    if (numberparams(cmd, argcount, args, sizeof(args)) && argcount >= 3 &&
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
      default:
        b->action = 0;
        break;
      }
      if (b->action != ButtonAction::kAuxStr && argcount >= 4)
        b->sound.auxstring = (argcount >= 5) ? args[4] : 0;
      return true;
    }
    return false;
  } else if (startswith(cmd, "lb=")) {
    uint8_t argcount;
    uint8_t args[5];
    memset(args, '\0', sizeof(args));
    ButtonAction *b = params.LB;
    if (numberparams(cmd, argcount, args, sizeof(args)) && argcount >= 3 &&
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
      default:
        b->action = 0;
        break;
      }
      if (b->action != ButtonAction::kAuxStr && argcount >= 4)
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
      if (numberparams(cmd, argcount, args, sizeof(args)) && argcount >= 2) {
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
        default:
          b->action = 0;
          break;
        }
        if (b->action != ButtonAction::kAuxStr && argcount >= 3)
          b->sound.auxstring = args[2];
        if (params.gcount < params.getGestureCount())
          params.gcount++;
        return true;
      }
    }
    return false;
  } else if (charparam(cmd, "acktype=", "gadsr", params.acktype) ||
             charparam(cmd, "b9=", "ynksdb", params.b9) ||
             intparam(cmd, "volume=", params.volume, 0, 100) ||
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
#endif

//////////////////////////////////////////////////////////////////////////

void setup() {
  REELTWO_READY();

  randomSeed(analogRead(3));

  CONSOLE_SERIAL.begin(DEFAULT_BAUD_RATE);
  XBEE_SERIAL.begin(57600);
  // VMUSIC_SERIAL.begin(9600);
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