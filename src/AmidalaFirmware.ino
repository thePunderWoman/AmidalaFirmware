///////////////////////////////////////////////
//
///////////////////////////////////////////////

#include "drive_config.h"

// Uncomment to enable RDH auto-dome serial:
// #define RDH_SERIAL           Serial3

////////////////////////////////
// User-editable debug switches:

#define USE_DEBUG
// #define USE_DOME_SENSOR_SERIAL_DEBUG
// #define USE_POCKET_REMOTE_DEBUG
// #define USE_PPM_DEBUG
// #define USE_MOTOR_DEBUG
// #define USE_DOME_DEBUG
// #define USE_SERVO_DEBUG
// #define USE_VERBOSE_SERVO_DEBUG

////////////////////////////////

// ServoDispatchDirect.h must be included here — ServoDispatchPrivate.h defines
// hardware ISR handlers that must live in exactly one translation unit.
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

// Create one servo PD controller for camera pan and another for camera tilt:
ServoPD panservo(400, 200, PANZERO, PANRANGE);
ServoPD tiltservo(300, 100, TILTZERO, TILTRANGE);

////////////////////////////////

AmidalaController amidala;

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
