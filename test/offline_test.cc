// Offline sanity check: drive GranularProcessor the same way the plugin
// does and verify non-silent output for every playback mode.

#include "supercell/dsp/granular_processor.h"

#include <cmath>
#include <cstdio>
#include <cstring>

using namespace clouds;

int main() {
  static uint8_t large_buffer[118784];
  static uint8_t small_buffer[65536 - 128];
  static GranularProcessor processor_storage;

  bool all_ok = true;
  for (int mode = 0; mode < PLAYBACK_MODE_LAST; ++mode) {
    memset(large_buffer, 0, sizeof(large_buffer));
    memset(small_buffer, 0, sizeof(small_buffer));
    memset(static_cast<void*>(&processor_storage), 0, sizeof(processor_storage));

    GranularProcessor& p = processor_storage;
    p.Init(large_buffer, sizeof(large_buffer),
           small_buffer, sizeof(small_buffer));
    p.set_num_channels(2);
    p.set_low_fidelity(false);
    p.set_playback_mode(static_cast<PlaybackMode>(mode));
    p.set_silence(false);
    p.set_bypass(false);
    p.Prepare();

    Parameters* params = p.mutable_parameters();
    // In Spectral Cloud, position sets the band gate threshold (1 - position);
    // keep it high there so a test sine passes through.
    params->position = mode == 7 ? 0.95f : 0.3f;
    params->size = 0.5f;
    params->pitch = 0.0f;
    params->density = 0.7f;
    params->texture = 0.5f;
    params->dry_wet = 1.0f;  // fully wet: output must come from the engine
    params->stereo_spread = 0.0f;
    params->feedback = 0.0f;
    params->reverb = 0.0f;
    params->freeze = false;
    params->capture = false;
    params->gate = false;
    params->granular.reverse = false;
    params->kammerl.slice_selection = 0.0f;
    params->kammerl.slice_modulation = 0.5f;
    params->kammerl.size_modulation = 0.7f;
    params->kammerl.probability = 1.0f;
    params->kammerl.clock_divider = 0.0f;
    params->kammerl.pitch_mode = 0.0f;
    params->kammerl.distortion = 0.0f;
    params->kammerl.pitch = 0.5f;

    const int kBlocks = 2000;  // 2 s at 32 kHz
    double sum_sq = 0.0;
    long n = 0;
    bool has_nan = false;
    float phase = 0.0f;

    for (int b = 0; b < kBlocks; ++b) {
      ShortFrame in[kMaxBlockSize];
      ShortFrame out[kMaxBlockSize];
      for (size_t i = 0; i < kMaxBlockSize; ++i) {
        phase += 220.0f / 32000.0f;
        if (phase >= 1.0f) phase -= 1.0f;
        in[i].l = in[i].r = static_cast<int16_t>(
            16384.0f * sinf(phase * 2.0f * static_cast<float>(M_PI)));
      }
      params->capture = (b % 250) == 0;  // regular triggers
      p.Process(in, out, kMaxBlockSize);
      p.Prepare();
      if (b >= 1000) {  // measure the second half only
        for (size_t i = 0; i < kMaxBlockSize; ++i) {
          const float l = out[i].l / 32768.0f;
          const float r = out[i].r / 32768.0f;
          if (l != l || r != r) has_nan = true;
          sum_sq += l * l + r * r;
          n += 2;
        }
      }
    }
    const double rms = sqrt(sum_sq / n);
    const bool ok = !has_nan && rms > 0.001;
    all_ok = all_ok && ok;
    printf("mode %d: rms=%.4f nan=%d -> %s\n", mode, rms, has_nan,
           ok ? "OK" : "FAIL");
  }
  return all_ok ? 0 : 1;
}
