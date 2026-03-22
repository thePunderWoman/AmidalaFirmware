#include "controller.h"

void AmidalaConsole::process(ButtonAction &button) {
  AmidalaParameters &params = fController->params;
  switch (button.action) {
  case button.kSound:
    Serial.print("PLAY SOUND BANK ");
    Serial.print(button.sound.soundbank);
    Serial.print(" # ");
    Serial.println(button.sound.sound);
    if (button.sound.sound != 0) {
      fController->fAudio.playSound(button.sound.soundbank, button.sound.sound);
    } else {
      fController->fAudio.playSound(button.sound.soundbank);
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
    fController->fAudio.playEmote(button.emote.emotion, button.emote.level);
    break;
  case button.kHCRMuse:
    fController->fAudio.toggleMuse();
    break;
  }
  // Play ack emote for non-audio actions when ackon is enabled
  if (button.action != ButtonAction::kNone &&
      button.action != ButtonAction::kHCREmote &&
      button.action != ButtonAction::kHCRMuse) {
    fController->fAudio.playAck();
  }
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
