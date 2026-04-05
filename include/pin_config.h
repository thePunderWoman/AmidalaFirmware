// pin_config.h
// Hardware pin assignments and serial port mappings for Amidala Firmware.
// All physical I/O pin numbers and serial port aliases are defined here so
// that wiring changes require edits in exactly one place.
//
// Note: SERIAL is conditionally defined based on DOME_DRIVE_SERIAL and
// RDH_SERIAL. Those symbols come from drive_config.h and rdh_serial.h,
// which must be included (via controller.h) before this header is included.
//
// Note: If VMUSIC_SERIAL is ever enabled, move #include "pin_config.h" in the
// .ino to before the #ifndef VMUSIC_SERIAL / #include <hcr.h> block so that
// the guard evaluates correctly.

#pragma once

// ---- Servo output pins (PWM) ------------------------------------------------

#define SERVO1_PIN  2
#define SERVO2_PIN  3
#define SERVO3_PIN  4
#define SERVO4_PIN  5
#define SERVO5_PIN  6
#define SERVO6_PIN  7
#define SERVO7_PIN  8
#define SERVO8_PIN  9
#define SERVO9_PIN  10
#define SERVO10_PIN 11
#define SERVO11_PIN 12
#define SERVO12_PIN 13

// ---- Digital output pins ----------------------------------------------------

#define DOUT1_PIN 22
#define DOUT2_PIN 23
#define DOUT3_PIN 24
#define DOUT4_PIN 25
#define DOUT5_PIN 26
#define DOUT6_PIN 27
#define DOUT7_PIN 28
#define DOUT8_PIN 29

#define DRIVE_ACTIVE_PIN DOUT7_PIN
#define DOME_ACTIVE_PIN  DOUT8_PIN

// ---- PPM RC input -----------------------------------------------------------

#define PPMIN_PIN 49

// ---- Analog inputs ----------------------------------------------------------

#define ANALOG1_PIN A0
#define ANALOG2_PIN A1

// ---- Mode select pins (active-low with INPUT_PULLUP) ------------------------

#define RCSEL_PIN 30
#define SEL2_PIN  31

// ---- Status LED / indicator pins --------------------------------------------

#define STATUS_J1_PIN 32
#define STATUS_J2_PIN 33
#define STATUS_RC_PIN 34
#define STATUS_S1_PIN 35
#define STATUS_S2_PIN 36
#define STATUS_S3_PIN 37
#define STATUS_S4_PIN 38

// ---- Dome hall-effect sensor (RoboClaw dome drive only) --------------------
// Pin 18 = Arduino Mega INT5 (hardware interrupt 5), currently unassigned.
// The sensor output is active-LOW; attach to INPUT_PULLUP and trigger on FALLING.
// Only used when DOME_DRIVE == DOME_DRIVE_ROBOCLAW.
#define DOME_HALL_PIN 18

// ---- I2C bus pins (Arduino Mega: SDA = pin 20, SCL = pin 21) ---------------

#define I2C_SDA_PIN 20
#define I2C_SCL_PIN 21

// ---- Serial port assignments ------------------------------------------------

#define CONSOLE_SERIAL Serial
#define XBEE_SERIAL    Serial1

// Uncomment the line below to enable the VMusic2 audio board on Serial2.
// See note at the top of this file about include ordering when enabling this.
// #define VMUSIC_SERIAL  Serial2

// SERIAL is Serial3 when Serial3 is not already claimed by the drive or
// dome controller. DOME_DRIVE_SERIAL and RDH_SERIAL are resolved from the
// drive-system configuration before this header is included.
#if !defined(DOME_DRIVE_SERIAL) && !defined(RDH_SERIAL)
#define SERIAL Serial3
#endif
