// rdh_serial.h
// Serial interface for the Robbie the Robot / RDH dome position sensor.
//
// To enable autodome serial, uncomment the line below (or define it before
// including this header):
//
//   #define RDH_SERIAL  Serial3
//
// RDHSerial wraps a Stream (typically a hardware UART) and provides:
//   - Command methods to move the dome (setRelativePosition,
//     setAbsolutePosition, setDomeDefaultMode, setDomeHomePosition,
//     toggleRandomDome, sendCommand)
//   - A process() method that parses incoming "#DP<mode><angle>\r\n" packets
//     and maintains a median-filtered position estimate
//   - Accessors: ready(), getAngle(), getMode(), getHome()
//
// Depends on: Stream (Arduino / arduino_mock.h),
//             DEBUG_PRINTLN (ReelTwo.h / arduino_mock.h),
//             core/MedianSampleBuffer.h (Reeltwo library)

#pragma once

#include "core/MedianSampleBuffer.h"

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
