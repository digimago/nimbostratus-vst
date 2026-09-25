# Nimbostratus 1.4.0

Adds gain trims either side of the engine, and fixes numeric entry on both
platforms. Windows x64 and macOS universal builds attached below, both
verified in Ableton Live.

Existing sessions load unchanged: the two new parameters are appended after
the existing ones, so automation lanes keep pointing at the same knobs.

## Added

- **Input Gain and Output Gain** (-24 to +24 dB), standing in for the input
  and output pots on the hardware's panel. **In Gain** sits ahead of the
  engine, so pushing it drives the recording buffer and the feedback path
  harder rather than just raising the level; **Out Gain** sits after it. Both
  are smoothed, so automating them does not click.

  These give you a way to deal with a long-standing surprise: the engine runs
  about 9 dB below unity, so 100% dry is quieter than bypass. That is 6 dB of
  internal headroom in the firmware plus 3 dB from the equal-power Dry/Wet
  crossfade — inherited Mutable Instruments gain staging, not a porting bug,
  and the hardware compensates the same way with its panel pots. Set **Out
  Gain** to +9 dB to match bypass at 100% dry. No single setting flattens the
  whole Dry/Wet sweep, since the dry and wet extremes need different makeup.

## Fixed

- **Confirming a typed value with Return no longer kills the field.** Pressing
  Return committed the value, and from then on every numeric field closed the
  instant it was clicked, looking dead until the plugin window was closed and
  reopened. Escape did the same. The key press reached the UI but its release
  did not — committing the value handed the keyboard back to the host in
  between — so the UI was left holding the key down, and its auto-repeat shut
  each field as it opened. Affected **Pitch** and both gain knobs on Windows
  and macOS alike.

- **Knob moves are reported to the host as complete edit gestures.** Typing a
  value, or dragging one in a numeric field, told the host an edit had ended
  without ever telling it one had begun, and an ordinary knob drag only
  balanced by luck. Hosts that record automation from those gestures now see
  each one whole.

- **Typing a value works more than once per window (macOS).** Numeric entry
  worked on the first try after opening the plugin and then stopped: the field
  would highlight when clicked but silently swallow everything typed into it.
  Handing unused key presses back to the host, added in 1.3.4, meant the host
  took keyboard focus the first time it acted on one — pressing the spacebar
  once was enough — after which the plugin never saw another keystroke. The
  **Pitch** knob has been affected since 1.3.4.

- **Tooltips wrap** instead of running off the side of the window as a single
  long line.

- **Double-clicking a numeric field no longer resets the knob.** Double-click
  still resets to the default on the knob face, but inside the value field —
  where it selects what you typed — it now leaves the value alone. Affected
  **Pitch** as well as the new gain knobs.

## Install

- **Windows**: unzip and copy `nimbostratus.vst3` to your custom VST3 folder
  (e.g. `%USERPROFILE%\Documents\VST3`) or `C:\Program Files\Common Files\VST3`.
- **macOS**: unzip and copy `nimbostratus.vst3` to
  `~/Library/Audio/Plug-Ins/VST3`, then clear the download quarantine:
  `xattr -cr ~/Library/Audio/Plug-Ins/VST3/nimbostratus.vst3`.

Full attribution and licensing in [LICENSE](LICENSE) and the
[README](README.md).
