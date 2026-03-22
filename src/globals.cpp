// amidala_globals.cpp — hardware-level globals.
//
// ServoDispatchDirect.h is included HERE and nowhere else.
// ServoDispatchPrivate.h (pulled in transitively) defines hardware ISR
// handlers that must live in exactly one translation unit.

#include "ServoDispatchDirect.h"
#include "ServoEasing.h"
#include "controller.h"

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

// Experimental camera pan/tilt servo PD controllers.
// Pan and tilt zero positions and +/- angular range, in degrees:
#define PANZERO  90
#define PANRANGE 60
#define TILTZERO 70
#define TILTRANGE 40

ServoPD panservo(400, 200, PANZERO, PANRANGE);
ServoPD tiltservo(300, 100, TILTZERO, TILTRANGE);
