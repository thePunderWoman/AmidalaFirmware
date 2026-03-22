#include "amidala_controller.h"

void DriveController::notify() {
  uint32_t lagTime = millis() - lastPacket;
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
}

void DriveController::onConnect() {
  DEBUG_PRINTLN("Drive Stick Connected");
  fDriver->enableController();
}

void DriveController::onDisconnect() {
  DEBUG_PRINTLN("Drive Stick Disconnected");
  fDriver->disableController();
}

void DomeController::notify() {
  uint32_t lagTime = millis() - lastPacket;
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
}

void DomeController::process() {
  if (event.analog_changed.button.l1) {
    /* Volume */
    fDriver->setVolumeNoResponse(
        map(state.analog.button.l1, 0, 255, 0, 100));
  }
  if (event.analog_changed.button.l2) {
    /* Throttle */
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

void DomeController::onConnect() {
  DEBUG_PRINTLN("Dome Stick Connected");
  fDriver->enableDomeController();
}

void DomeController::onDisconnect() {
  DEBUG_PRINTLN("Dome Stick Disconnected");
  fDriver->disableDomeController();
}

