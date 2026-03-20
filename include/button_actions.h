// button_actions.h
// Data types for button and gesture action mappings in Amidala Firmware.
//
// ButtonAction  — what happens when a button is pressed or a gesture fires.
// GestureAction — a (Gesture, ButtonAction) pair stored in config.
// AuxString     — a short string sent to the auxiliary serial port.
//
// Depends on: Gesture (amidala_core.h), Print (Arduino / arduino_mock.h)
// In embedded builds hcr.h is included before this header and supplies the
// HCR constants. The #ifndef guards below provide fallbacks for native tests.

#pragma once

#include "amidala_core.h"

// ---- HCR emotion / level constants (fallbacks for non-embedded builds) ------

#ifndef HAPPY
#define HAPPY    0
#define SAD      1
#define MAD      2
#define SCARED   3
#define OVERLOAD 4
#endif

#ifndef EMOTE_MODERATE
#define EMOTE_MODERATE 0
#define EMOTE_STRONG   1
#endif

// ---- ButtonAction -----------------------------------------------------------

struct ButtonAction {
  enum {
    kNone      = 0,
    kSound     = 1,
    kServo     = 2,
    kDigitalOut = 3,
    kI2CCmd    = 4,
    kAuxStr    = 5,
    kI2CStr    = 6,
    kHCREmote  = 7,  // Trigger an HCR emotion (emotion, level)
    kHCRMuse   = 8   // Toggle HCR musing on/off
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
    struct {
      uint8_t emotion;   // HAPPY=0, SAD=1, MAD=2, SCARED=3, OVERLOAD=4
      uint8_t level;     // EMOTE_MODERATE=0, EMOTE_STRONG=1
      uint8_t auxstring;
    } emote;
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
    case kHCREmote:
      stream->print(F("HCR "));
      switch (emote.emotion) {
        case HAPPY:    stream->print(F("Happy"));    break;
        case SAD:      stream->print(F("Sad"));      break;
        case MAD:      stream->print(F("Mad"));      break;
        case SCARED:   stream->print(F("Scared"));   break;
        case OVERLOAD: stream->print(F("Overload")); break;
        default:       stream->print(emote.emotion); break;
      }
      if (emote.emotion != OVERLOAD)
        stream->print(emote.level ? F(" Strong") : F(" Moderate"));
      break;
    case kHCRMuse:
      stream->print(F("HCR Muse Toggle"));
      break;
    }
    if (action != kAuxStr && action != kHCREmote && action != kHCRMuse &&
        aux.auxstring != 0) {
      stream->print(F(", Aux #"));
      stream->print(aux.auxstring);
    }
    stream->println();
  }
};

// ---- GestureAction ----------------------------------------------------------

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

// ---- AuxString --------------------------------------------------------------

struct AuxString {
  char str[32];
};
