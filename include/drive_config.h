// drive_config.h
// Drive and dome system configuration for Amidala Firmware.
//
// Edit the two "Active selection" lines to switch hardware configurations.
// Everything below the "Drive system resolution" comment is derived
// automatically — do not edit those lines.

#pragma once

// ---- Drive system type constants --------------------------------------------

#define DRIVE_SYSTEM_PWM                1
#define DRIVE_SYSTEM_SABER              2
#define DRIVE_SYSTEM_ROBOTEQ_PWM        3
#define DRIVE_SYSTEM_ROBOTEQ_SERIAL     4
#define DRIVE_SYSTEM_ROBOTEQ_PWM_SERIAL 5

// Active selection (uncomment the one in use):
// #define DRIVE_SYSTEM DRIVE_SYSTEM_ROBOTEQ_PWM_SERIAL
#define DRIVE_SYSTEM DRIVE_SYSTEM_ROBOTEQ_PWM

// ---- Dome drive type constants -----------------------------------------------

#define DOME_DRIVE_SABER 2
#define DOME_DRIVE_PWM   3

// Active selection (uncomment the one in use):
#define DOME_DRIVE DOME_DRIVE_PWM
// #define DOME_DRIVE DOME_DRIVE_SABER

// ---- Speed and acceleration parameters --------------------------------------

#define MAXIMUM_SPEED       1.0f  // percentage 0.0 - 1.0. default 50%
#define MAXIMUM_GUEST_SPEED 0.5f  // percentage 0.0 - 1.0. default 30%
#define DOME_MAXIMUM_SPEED  1.0f  // percentage 0.0 - 1.0. default 100%

// Scale value of 1 means instant. Scale value of 100 means that the throttle
// will increase 1/100 every 25ms
#define ACCELERATION_SCALE 100
// Scale value of 1 means instant. Scale value of 20 means that the throttle
// will decrease 1/20 every 25ms
#define DECELRATION_SCALE  20

// Set to true if acceleration/deceleration should be applied
#define SCALING false

// ---- Dome motion defaults ---------------------------------------------------

#define DEFAULT_DOME_HOME_POSITION      50
#define DEFAULT_DOME_ACCELERATION_SCALE 20
#define DEFAULT_DOME_DECELERATION_SCALE 50
#define DEFAULT_DOME_HOME_MIN_DELAY     6
#define DEFAULT_DOME_HOME_MAX_DELAY     8
#define DEFAULT_DOME_SEEK_MIN_DELAY     6
#define DEFAULT_DOME_SEEK_MAX_DELAY     8
#define DEFAULT_DOME_TARGET_MIN_DELAY   0
#define DEFAULT_DOME_TARGET_MAX_DELAY   1
#define DEFAULT_DOME_SEEK_LEFT          80
#define DEFAULT_DOME_SEEK_RIGHT         80
#define DEFAULT_DOME_FUDGE              2
#define DEFAULT_DOME_SPEED_HOME         40
#define DEFAULT_DOME_SPEED_SEEK         60
#define DEFAULT_DOME_SPEED_TARGET       100
#define DEFAULT_DOME_SPEED_MIN          15
#define DEFAULT_DOME_TIMEOUT            5
#define DEFAULT_DOME_INVERTED           false
#define DEFAULT_DOME_MIN_PULSE          1000
#define DEFAULT_DOME_MAX_PULSE          2000

// ---- Drive system resolution (derived — do not edit) ------------------------

#if DRIVE_SYSTEM == DRIVE_SYSTEM_SABER
#define DRIVE_SERIAL      Serial3
#define DOME_DRIVE_SERIAL DRIVE_SERIAL
#define CHANNEL_MIXING    true  // premix channels before sending commands
#elif DRIVE_SYSTEM == DRIVE_SYSTEM_PWM
#define CHANNEL_MIXING    false // motor controller will mix the channels
#elif DRIVE_SYSTEM == DRIVE_SYSTEM_ROBOTEQ_PWM
#define CHANNEL_MIXING    false // motor controller will mix the channels
#elif DRIVE_SYSTEM == DRIVE_SYSTEM_ROBOTEQ_SERIAL
#define DRIVE_BAUD_RATE   115200
#define CHANNEL_MIXING    false // motor controller will mix the channels
#elif DRIVE_SYSTEM == DRIVE_SYSTEM_ROBOTEQ_PWM_SERIAL
#define DRIVE_BAUD_RATE   115200
#define CHANNEL_MIXING    false // motor controller will mix the channels
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
