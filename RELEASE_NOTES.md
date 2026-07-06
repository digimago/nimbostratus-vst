# Nimbostratus 1.3.0

Granular texture processor for VST3 and CLAP hosts, based on the excellent
open-source work of Émilie Gillet, with the community Parasites and Kammerl
extensions. Windows x64 and macOS universal (Apple Silicon + Intel) builds
attached below.

## New: Tempo Sync

- New **SYNC** switch (automatable "Tempo Sync" parameter). While engaged,
  Nimbostratus fires its trigger on musical division boundaries, locked to
  the host transport — grains, beat repeats and resonator bursts land on
  the beat with zero configuration.
- While synced, the **Density knob becomes the clock divider**: 4 bars,
  2 bars, 1 bar, 1/2, 1/4, 1/8, 1/8T, 1/16 or 1/32. Its label shows the
  active division. The engine's own grain density is parked at neutral, so
  what you hear is purely the clocked stream.
- The clock re-anchors to the host's bar/beat/tick while playing (loop
  jumps stay in time) and free-runs at the last known tempo when the
  transport is stopped.
- The manual TRIG button and Trigger automation still work on top of the
  synced clock — great for fills. Try SYNC with **Beat Repeat** mode and
  Probability up for instant hardware-with-a-clock energy.

## Fixed

- **Hi-DPI rendering**: text no longer overflows buttons and the layout no
  longer overflows the window on Retina displays and scaled Windows
  desktops. The plugin window is now fixed-size and follows the host/OS
  UI scale instead of scaling its controls when resized.

## Install

- **Windows**: unzip and copy `nimbostratus.vst3` to
  `C:\Program Files\Common Files\VST3` (or your custom VST3 folder).
- **macOS**: unzip and copy `nimbostratus.vst3` to
  `~/Library/Audio/Plug-Ins/VST3`, then clear the download quarantine:
  `xattr -cr ~/Library/Audio/Plug-Ins/VST3/nimbostratus.vst3`.

Full attribution and licensing in [LICENSE](LICENSE) and the
[README](README.md).
