#include "controller.h"

// amidala is the global AmidalaController instance defined in AmidalaFirmware.ino.
extern AmidalaController amidala;

#ifndef VMUSIC_SERIAL
static void hcrDelayedInit() {
  AmidalaParameters &params = amidala.params;
  amidala.fHCR.SetVolume(CH_V, params.volume);
  amidala.fHCR.SetVolume(CH_A, params.volume);
  amidala.fHCR.SetVolume(CH_B, params.volume);
  if (params.rndon) {
    amidala.fHCR.Muse(params.mindelay, params.maxdelay);
    amidala.fHCR.SetMuse(1);
  }
  if (params.startup) {
    amidala.fHCR.Trigger(params.startupem, params.startuplvl);
  }
}
#endif  // !VMUSIC_SERIAL

void AmidalaAudio::init(AmidalaController *controller) {
  fController = controller;
#ifndef VMUSIC_SERIAL
  AmidalaParameters &params = fController->params;
  if (params.audiohw == AUDIO_HW_HCR) {
    fController->fHCR.begin();
    DelayCall::schedule(hcrDelayedInit, 5000);
  }
#endif
}

void AmidalaAudio::process() {
  if (!fController) return;
#ifndef VMUSIC_SERIAL
  AmidalaParameters &params = fController->params;
  if (params.audiohw == AUDIO_HW_HCR) {
    // fController->fHCR.update();
  }
#else
  AmidalaParameters &params = fController->params;
  if (params.audiohw == AUDIO_HW_VMUSIC) {
    fController->fVMusic.process();
  }
#endif
}

void AmidalaAudio::randomToggle() {
  AmidalaParameters &params = fController->params;
  params.rndon = !params.rndon;
  fController->fConsole.print(F("Random "));
  fController->fConsole.println((params.rndon) ? F("On") : F("Off"));
#ifndef VMUSIC_SERIAL
  if (params.audiohw == AUDIO_HW_HCR) {
    fController->fHCR.SetMuse(params.rndon ? 1 : 0);
  }
#endif
}

void AmidalaAudio::setVolumeNoResponse(uint8_t volume) {
  AmidalaParameters &params = fController->params;
#ifndef VMUSIC_SERIAL
  if (params.audiohw == AUDIO_HW_HCR) {
    fController->fHCR.SetVolume(CH_V, volume);
    fController->fHCR.SetVolume(CH_A, volume);
    fController->fHCR.SetVolume(CH_B, volume);
  }
#else
  if (params.audiohw == AUDIO_HW_VMUSIC) {
    fController->fVMusic.setVolumeNoResponse(volume);
  }
#endif
}

void AmidalaAudio::playSound(int sndbank, int snd) {
  AmidalaParameters &params = fController->params;
#ifdef VMUSIC_SERIAL
  if (params.audiohw == AUDIO_HW_VMUSIC) {
    if (fController->fVMusic.isPlaying()) {
      fController->fConsole.println(F("Busy"));
      return;
    }
    AmidalaParameters::SoundBank *sb = params.SB;
    if (sndbank >= 1 && sndbank <= params.sbcount) {
      sb += (sndbank - 1);
      if (snd == 0) {
        if (sb->random) {
          snd = random(0, sb->numfiles);
        } else if (sb->numfiles > 0) {
          snd = sb->playindex++;
          if (sb->playindex >= sb->numfiles)
            sb->playindex = 0;
        } else {
          snd = -1;
        }
      }
      if (snd >= 0 && snd <= sb->numfiles) {
        char fname[16];
        snprintf(fname, sizeof(fname), "%s-%d.MP3", sb->dir, snd + 1);
        DEBUG_PRINT("PLAY: ");
        DEBUG_PRINTLN(fname);
        fController->fVMusic.play(fname, sb->dir);
      }
    }
    return;
  }
#endif
  fController->fConsole.println(F("Invalid"));
}

void AmidalaAudio::playEmote(uint8_t emotion, uint8_t level) {
#ifndef VMUSIC_SERIAL
  AmidalaParameters &params = fController->params;
  if (params.audiohw == AUDIO_HW_HCR) {
    if (emotion == OVERLOAD) {
      fController->fHCR.Overload();
    } else {
      fController->fHCR.Trigger(emotion, level);
    }
  }
#endif
}

void AmidalaAudio::toggleMuse() {
#ifndef VMUSIC_SERIAL
  AmidalaParameters &params = fController->params;
  if (params.audiohw == AUDIO_HW_HCR) {
    fController->fHCR.SetMuse(1 - fController->fHCR.GetMuse());
  }
#endif
}

void AmidalaAudio::playAck() {
#ifndef VMUSIC_SERIAL
  AmidalaParameters &params = fController->params;
  if (params.ackon && params.audiohw == AUDIO_HW_HCR) {
    fController->fHCR.Trigger(params.ackem, params.acklvl);
  }
#endif
}
