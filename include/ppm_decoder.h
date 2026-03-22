// ppm_decoder.h
// PPM pulse-position modulation decoder for Amidala Firmware.
//
// Reads a PPM signal from PPMIN_PIN (defined in pin_config.h) and decodes
// up to channelCount RC channels from the frame.  After channelCount valid
// channels have accumulated and a 10-frame warm-up period has passed,
// decode() returns true and channel() returns the mapped pulse values.
//
// Depends on: PPMIN_PIN (pin_config.h must be included before this header),
//             digitalRead / micros (Arduino / arduino_mock.h),
//             map / min / max (Arduino / arduino_mock.h)

#pragma once

#include <stdint.h>
#include <string.h>

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
    memset(fChannel,     0, fChannelCount * sizeof(uint16_t));
    memset(fLastChannel, 0, fChannelCount * sizeof(uint16_t));
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
            DEBUG_PRINT(": ");
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
