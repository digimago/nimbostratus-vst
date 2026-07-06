# Nimbostratus 1.3.1

UI affordances release: the panel now explains itself and shows what the
engine is doing. Windows x64 and macOS universal builds attached below.

## Improved

- **Contextual tooltips everywhere.** Hover any knob, button or selector for
  a short explanation — and the knob tooltips follow the active playback
  mode, so in Oliverb the Density knob explains decay, in Beat Repeat it
  explains repeat-length modulation, and so on.
- **Stepped Rate knob while synced.** With SYNC engaged, the Density knob
  now clicks through the clock divisions (4 bars … 1/32) as discrete steps
  instead of sweeping a continuous range — no more guessing how far to drag.
- **TRIG button flashes on every trigger**, including tempo-synced and
  automated ones, giving direct visual feedback of the engine's activity.
  (Exposed as a read-only "Trigger Activity" parameter, so hosts can see it
  too.)

## Install

- **Windows**: unzip and copy `nimbostratus.vst3` to
  `C:\Program Files\Common Files\VST3` (or your custom VST3 folder).
- **macOS**: unzip and copy `nimbostratus.vst3` to
  `~/Library/Audio/Plug-Ins/VST3`, then clear the download quarantine:
  `xattr -cr ~/Library/Audio/Plug-Ins/VST3/nimbostratus.vst3`.

Full attribution and licensing in [LICENSE](LICENSE) and the
[README](README.md).
