// audio.h
// AmidalaAudio: audio hardware abstraction layer (HCR / VMusic).
//
// Provides a unified API for all audio interactions — volume, sound playback,
// emotes, muse toggling, and acknowledgement triggers.
// Method bodies are defined in src/audio.cpp.
//
// Depends on: AmidalaController (via forward declaration only)

#pragma once

class AmidalaController;

class AmidalaAudio {
public:
  AmidalaAudio() {}
  virtual ~AmidalaAudio() {}

  // Called from AmidalaController::setup() to start the audio hardware and
  // schedule delayed initialisation.
  void init(AmidalaController *controller);

  // Called from AmidalaController::animate() to service audio hardware.
  void process();

  // Toggle random sound playback on/off.
  void randomToggle();

  // Set volume without printing a response (0–100).
  void setVolumeNoResponse(uint8_t volume);

  // Play a sound from the given sound bank (1-based).
  // If snd == 0, selects the next/random file based on the bank's mode.
  void playSound(int sndbank, int snd = 0);

  // Trigger an HCR emote (emotion + level).
  // OVERLOAD emotion triggers Overload() instead of Trigger().
  void playEmote(uint8_t emotion, uint8_t level);

  // Toggle HCR muse on/off.
  void toggleMuse();

  // Trigger the configured acknowledgement emote (if ackon is enabled).
  void playAck();

private:
  AmidalaController *fController = nullptr;
};
