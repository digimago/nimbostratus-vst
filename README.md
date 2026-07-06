# Nimbostratus

A granular texture processor in VST3 and CLAP form, based on the excellent
open-source work of Émilie Gillet — inspired by a certain much-loved cloudy
eurorack module — running the community SuperParasites firmware DSP (stock +
Parasites by Matthias Puech + beat repeat by Julian Kammerl), wrapped
unmodified with the [DISTRHO Plugin Framework](https://github.com/DISTRHO/DPF)
and a Dear ImGui interface.

The engine runs internally at 32 kHz on int16 frames in 32-sample blocks,
exactly like the hardware; host audio is resampled with the speexdsp
resampler (same approach as the VCV Rack port).

## Playback modes (all 8)

1. **Granular** — classic Clouds
2. **Pitch/Stretch** — WSOLA pitch shifter / time stretcher
3. **Looping Delay**
4. **Spectral** — FFT magnitude warping ("spectral madness")
5. **Oliverb** — Parasites modulated reverb
6. **Resonestor** — Parasites polyphonic resonator (feed it triggers)
7. **Beat Repeat** — Kammerl slice looper (feed it regular triggers)
8. **Spectral Cloud** — band-gated spectral freeze/blur

The UI relabels the knobs per mode, mirroring how the hardware repurposes its
pots. Extra parameters vs hardware: Reverse (granular/looping), and a
dedicated Slice knob for Beat Repeat (CV-only on hardware).

## Build

Portable toolchain lives in `tools/` (winlibs MinGW-w64 GCC, CMake, Ninja —
no Visual Studio needed). From Git Bash:

```sh
T="$PWD/tools"
PATH="$T/mingw64/bin:$PATH" "$T/cmake-4.3.3-windows-x86_64/bin/cmake.exe" \
  -G Ninja -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER="$T/mingw64/bin/gcc.exe" \
  -DCMAKE_CXX_COMPILER="$T/mingw64/bin/g++.exe" \
  -DCMAKE_MAKE_PROGRAM="$T/ninja.exe"
PATH="$T/mingw64/bin:$PATH" "$T/ninja.exe" -C build
```

Outputs: `build/bin/nimbostratus.vst3` (bundle) and `build/bin/nimbostratus.clap`.
Install: copy to `%USERPROFILE%\Documents\VST3` (Live custom VST3 folder) or
`C:\Program Files\Common Files\VST3` (admin).

## Test

`test/offline_test.cc` drives the engine standalone through all 8 modes and
checks RMS/NaN:

```sh
S=superparasites
tools/mingw64/bin/g++.exe -O2 -DTEST -Wno-narrowing -w -I$S \
  test/offline_test.cc $S/supercell/dsp/{correlator,granular_processor,mu_law,kammerl_player}.cc \
  $S/supercell/dsp/pvoc/{frame_transformation,phase_vocoder,spectral_clouds_transformation,stft}.cc \
  $S/supercell/resources.cc $S/stmlib/utils/random.cc $S/stmlib/dsp/{atan,units}.cc \
  -o test/offline_test.exe -static && ./test/offline_test.exe
```

## Sources

- `superparasites/` — patrickdowling/superparasites (supercell DSP + mqtthiqs
  stmlib submodule), MIT. Already contains the Window::Start `done_` fix.
- `dpf/` — DISTRHO/DPF, ISC. `dpf-widgets/` — DISTRHO/DPF-Widgets (Dear ImGui).
- `speexdsp/` — xiph/speexdsp resampler, BSD.
- `plugin/` — the wrapper: CloudsPlugin.cpp (DSP), CloudsUI.cpp (ImGui UI),
  CloudsParams.h (shared ids), DistrhoPluginInfo.h.
- `eurorack/` — original stock Clouds sources (no longer compiled; kept for
  reference, carries the same window.h fix locally).

## Desktop-port gotchas (learned the hard way)

- The firmware relies on zero-initialized globals: the `GranularProcessor`
  object must be memset to 0 before `Init()` (silence_/freeze_lp_ otherwise
  uninitialized → permanent silence or NaN).
- Stock Clouds' `Window::Start()` never clears `done_` → silent stretch mode;
  SuperParasites/VCV already fixed it.
- `-Wno-narrowing` needed: the firmware brace-initializes bools from `0.0f`.

## Attribution & trademarks

Original DSP by Émilie Gillet (MIT licensed), Parasites extensions by
Matthias Puech, beat repeat by Julian Kammerl, SuperParasites integration by
Patrick Dowling; plugin wrapper and UI by Sebastiaan Visser. "Mutable
Instruments" and the original module names are trademarks of Émilie Gillet
and are deliberately not used to name or brand this plugin, following
Mutable Instruments' own open-source guidelines.

## macOS builds (CI)

Every push builds a universal (arm64 + x86_64) `nimbostratus.vst3` and
`nimbostratus.clap` on a macOS GitHub Actions runner — see `.github/workflows/build.yml`;
grab them from the workflow run's artifacts. After copying to
`~/Library/Audio/Plug-Ins/VST3`, clear quarantine if downloaded via browser:
`xattr -cr ~/Library/Audio/Plug-Ins/VST3/nimbostratus.vst3`.
